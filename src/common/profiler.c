#include "common/profiler.h"

#if PIM_PROFILE

#include "threading/task.h"
#include "allocator/allocator.h"
#include "common/time.h"
#include "common/cvar.h"
#include "common/hashstring.h"
#include "common/fnv1a.h"
#include "common/sort.h"
#include "common/stringutil.h"
#include "containers/dict.h"
#include "ui/cimgui.h"
#include <string.h>

static cvar_t cv_profilegui = { "profilegui", "1", "Show profiler GUI" };

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
static i32 FindMark(const profmark_t** hay, i32 count, const profmark_t* needle);
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

void profile_sys_init(void)
{
    cvar_reg(&cv_profilegui);
    dict_new(&ms_node_dict, sizeof(double), EAlloc_Perm);
    memset(ms_frame, 0, sizeof(ms_frame));
    memset(ms_roots, 0, sizeof(ms_roots));
    memset(ms_top, 0, sizeof(ms_top));
}

ProfileMark(pm_sys_gui, profile_sys_gui)
void profile_sys_gui(void)
{
    ProfileBegin(pm_sys_gui);
    if (cv_profilegui.asFloat != 0.0f)
    {
        OnGui();
    }
    ProfileEnd(pm_sys_gui);
}

void profile_sys_shutdown(void)
{
    memset(ms_frame, 0, sizeof(ms_frame));
    memset(ms_roots, 0, sizeof(ms_roots));
    memset(ms_top, 0, sizeof(ms_top));
    dict_del(&ms_node_dict);
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

static i32 FindMark(const profmark_t** hay, i32 count, const profmark_t* needle)
{
    for (i32 i = count - 1; i >= 0; --i)
    {
        if (hay[i] == needle)
        {
            return i;
        }
    }
    return -1;
}

static void VisitClr(node_t* node)
{
    if (!node)
    {
        return;
    }

    profmark_t* mark = node->mark;
    if (mark)
    {
        mark->calls = 0;
        mark->sum = 0;
        u32 hash = mark->hash;
        if (hash == 0)
        {
            hash = HashStr(mark->name);
            mark->hash = hash;
        }
        node->hash = hash;
    }

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
    if (mark)
    {
        mark->calls += 1;
        mark->sum += node->end - node->begin;
    }

    u32 hash = node->hash;
    if (node->parent)
    {
        hash = Fnv32Dword(node->parent->hash, hash);
    }
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

static void GetNodeKey(const node_t* node, char key[64])
{
    const profmark_t* mark = node->mark;
    const char* name = mark ? mark->name : "(null)";
    SPrintf(key, 64, "%s%x", name, node->hash);
}

static double GetNodeAvgMs(const node_t* node, const char* key)
{
    double avgMs = 0.0;
    const profmark_t* mark = node->mark;
    if (mark)
    {
        dict_get(&ms_node_dict, key, &avgMs);
    }
    return avgMs;
}

static double UpdateNodeAvgMs(const node_t* node, const char* key)
{
    const double ms = time_milli(node->end - node->begin);
    double avgMs = ms;
    const profmark_t* mark = node->mark;
    if (mark)
    {
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
    if (mark)
    {
        const char* name = mark->name;
        char key[64];
        GetNodeKey(node, key);

        double ms = UpdateNodeAvgMs(node, key);

        const i32 calls = (i32)mark->calls;
        double pct = 0.0;
        const node_t* root = ms_prevroots[ms_gui_tid].fchild;
        if (root)
        {
            char rootKey[64];
            GetNodeKey(root, rootKey);
            double rootMs = GetNodeAvgMs(root, rootKey);
            if (rootMs > 0.0)
            {
                pct = 100.0 * (ms / rootMs);
            }
        }

        igText("%s", name); igNextColumn();
        igText("%3.2f", ms); igNextColumn();
        igText("%4.1f%%", pct); igNextColumn();

        igTreePushPtr(key);
        VisitGui(node->fchild);
        igTreePop();
    }
    else
    {
        VisitGui(node->fchild);
    }
    VisitGui(node->sibling);
}

static void OnGui(void)
{
    igBegin("Profiler", NULL, 0);

    igSliderInt("thread", &ms_gui_tid, 0, kNumThreads - 1, "%d");
    node_t* root = ms_prevroots + ms_gui_tid;

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

void profile_sys_init(void) {}
void profile_sys_gui(void) {}
void profile_sys_shutdown(void) {}

void _ProfileBegin(profmark_t* mark) {}
void _ProfileEnd(profmark_t* mark) {}

#endif // PIM_PROFILE
