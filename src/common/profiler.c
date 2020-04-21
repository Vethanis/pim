#include "common/profiler.h"

#if PIM_PROFILE

#include "threading/task.h"
#include "allocator/allocator.h"
#include "common/time.h"
#include "common/cvar.h"
#include "common/hashstring.h"
#include "common/fnv1a.h"
#include "containers/dict.h"
#include "ui/cimgui.h"
#include <string.h>

static cvar_t cv_profilegui = { cvar_bool, "profilegui", "1", "Show profiler GUI" };

// ----------------------------------------------------------------------------

typedef struct node_s
{
    profmark_t* mark;
    struct node_s* parent;
    struct node_s* fchild;
    struct node_s* lchild;
    struct node_s* sibling;
    u64 begin;
    u64 end;
    u32 hash;
} node_t;

// ----------------------------------------------------------------------------

static void OnGui(void);
static void VisitClr(node_t* node);
static void VisitSum(node_t* node);
static void VisitGui(const node_t* node);

// ----------------------------------------------------------------------------

static u32 ms_frame[kNumThreads];
static node_t ms_prevroots[kNumThreads];
static node_t ms_roots[kNumThreads];
static node_t* ms_top[kNumThreads];

static i32 ms_gui_tid;
static dict_t ms_node_dict;

// ----------------------------------------------------------------------------

static void EnsureDict(void)
{
    if (!ms_node_dict.valueSize)
    {
        cvar_reg(&cv_profilegui);
        dict_new(&ms_node_dict, sizeof(double), EAlloc_Perm);
    }
}

ProfileMark(pm_gui, profile_gui)
void profile_gui(void)
{
    ProfileBegin(pm_gui);
    EnsureDict();
    if (cv_profilegui.asFloat != 0.0f)
    {
        OnGui();
    }
    ProfileEnd(pm_gui);
}

// ----------------------------------------------------------------------------

void _ProfileBegin(profmark_t* mark)
{
    ASSERT(mark);
    const i32 tid = task_thread_id();
    const u32 frame = time_framecount();

    if (frame != ms_frame[tid])
    {
        ms_prevroots[tid] = ms_roots[tid];
        ms_roots[tid] = (node_t){ 0 };
        ms_frame[tid] = frame;
        ms_top[tid] = NULL;
    }

    node_t* top = ms_top[tid];
    if (!top)
    {
        top = &ms_roots[tid];
    }

    node_t* next = pim_calloc(EAlloc_Temp, sizeof(*next));
    next->mark = mark;
    next->parent = top;
    if (top->lchild)
    {
        top->lchild->sibling = next;
    }
    else
    {
        top->fchild = next;
    }
    top->lchild = next;
    ms_top[tid] = next;

    next->begin = time_now();
}

void _ProfileEnd(profmark_t* mark)
{
    const u64 end = time_now();

    ASSERT(mark);

    const i32 tid = task_thread_id();
    node_t* top = ms_top[tid];

    ASSERT(top);
    ASSERT(top->mark == mark);
    ASSERT(top->parent);
    ASSERT(top->begin);
    ASSERT(top->end == 0);
    ASSERT(time_framecount() == ms_frame[tid]);

    top->end = end;
    ms_top[tid] = top->parent;
}

// ----------------------------------------------------------------------------

static void VisitClr(node_t* node)
{
    if (!node)
    {
        return;
    }

    profmark_t* mark = node->mark;
    ASSERT(mark);
    mark->calls = 0;
    mark->sum = 0;
    u32 hash = mark->hash;
    if (hash == 0)
    {
        hash = HashStr(mark->name);
        mark->hash = hash;
    }
    node->hash = hash;

    VisitClr(node->sibling);
    VisitClr(node->fchild);
}

static void VisitSum(node_t* node)
{
    if (!node)
    {
        return;
    }

    profmark_t* mark = node->mark;
    ASSERT(mark);
    mark->calls += 1;
    mark->sum += node->end - node->begin;

    u32 hash = node->hash;
    ASSERT(node->parent);
    hash = Fnv32Dword(node->parent->hash, hash);
    if (node->sibling)
    {
        hash = Fnv32Dword(node->sibling->hash, hash);
    }
    if (node->fchild)
    {
        hash = Fnv32Dword(node->fchild->hash, hash);
    }
    node->hash = hash;

    VisitSum(node->sibling);
    VisitSum(node->fchild);
}

static void GetNodeKey(const node_t* node, char key[8])
{
    ASSERT(node);
    ASSERT(key);
    u32* dst = (u32*)key;
    dst[0] = node->hash;
    dst[1] = 0;
}

static double GetNodeAvgMs(const node_t* node, const char* key)
{
    ASSERT(node);
    ASSERT(key);

    double avgMs = 0.0;
    const profmark_t* mark = node->mark;
    ASSERT(mark);

    dict_get(&ms_node_dict, key, &avgMs);
    return avgMs;
}

static double UpdateNodeAvgMs(const node_t* node, const char* key)
{
    ASSERT(node);
    ASSERT(key);

    const double ms = time_milli(node->end - node->begin);
    double avgMs = ms;
    const profmark_t* mark = node->mark;
    ASSERT(mark);
    if (dict_get(&ms_node_dict, key, &avgMs))
    {
        const double alpha = 1.0 / 60.0;
        avgMs += (ms - avgMs) * alpha;
        dict_set(&ms_node_dict, key, &avgMs);
    }
    else
    {
        dict_add(&ms_node_dict, key, &avgMs);
    }
    return avgMs;
}

static void VisitGui(const node_t* node)
{
    if (!node)
    {
        return;
    }

    const profmark_t* mark = node->mark;
    ASSERT(mark);
    const char* name = mark->name;
    ASSERT(name);

    char key[8];
    GetNodeKey(node, key);

    double ms = UpdateNodeAvgMs(node, key);

    double pct = 0.0;
    const node_t* root = ms_prevroots[ms_gui_tid].fchild;
    ASSERT(root);

    char rootKey[8];
    GetNodeKey(root, rootKey);
    double rootMs = GetNodeAvgMs(root, rootKey);
    if (rootMs > 0.0)
    {
        pct = 100.0 * (ms / rootMs);
    }

    igText("%s", name); igNextColumn();
    igText("%3.2f", ms); igNextColumn();
    igText("%4.1f%%", pct); igNextColumn();

    igTreePushPtr(key);
    VisitGui(node->fchild);
    igTreePop();
    VisitGui(node->sibling);
}

static void OnGui(void)
{
    igBegin("Profiler", NULL, 0);

    igSliderInt("thread", &ms_gui_tid, 0, kNumThreads - 1, "%d");
    node_t* root = ms_prevroots[ms_gui_tid].fchild;

    igSeparator();

    VisitClr(root);
    VisitSum(root);

    igColumns(3);
    {
        igText("Name"); igNextColumn();
        igText("Milliseconds"); igNextColumn();
        igText("Percent"); igNextColumn();

        igSeparator();

        VisitGui(root);
    }
    igColumns(1);

    igEnd();
}

#else

void profile_gui(void) {}

void _ProfileBegin(profmark_t* mark) {}
void _ProfileEnd(profmark_t* mark) {}

#endif // PIM_PROFILE
