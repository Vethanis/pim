#include "common/profiler.h"

#if PIM_PROFILE

#include "threading/task.h"
#include "allocator/allocator.h"
#include "common/time.h"
#include "common/cvar.h"
#include "common/fnv1a.h"
#include "common/stringutil.h"
#include "containers/dict.h"
#include "ui/cimgui.h"
#include <string.h>

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

static void VisitClr(node_t *const pim_noalias node);
static void VisitSum(node_t *const pim_noalias node);
static void VisitGui(node_t const *const pim_noalias node);

// ----------------------------------------------------------------------------

static u32 ms_frame[kMaxThreads];
static node_t ms_prevroots[kMaxThreads];
static node_t ms_roots[kMaxThreads];
static node_t* ms_top[kMaxThreads];

static i32 ms_avgWindow = 20;
static dict_t ms_node_dict;

// ----------------------------------------------------------------------------

static void EnsureDict(void)
{
    if (!ms_node_dict.valueSize)
    {
        dict_new(&ms_node_dict, sizeof(u32), sizeof(double), EAlloc_Perm);
    }
}

ProfileMark(pm_gui, profile_gui)
void profile_gui(bool* pEnabled)
{
    ProfileBegin(pm_gui);
    EnsureDict();

    if (igBegin("Profiler", pEnabled, 0))
    {
        igSliderInt("avg over # frames", &ms_avgWindow, 1, 1000, "%d");

        node_t* root = ms_prevroots[0].fchild;

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
    }
    igEnd();

    ProfileEnd(pm_gui);
}

// ----------------------------------------------------------------------------

void _ProfileBegin(profmark_t *const mark)
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

    node_t *const next = tmp_calloc(sizeof(*next));
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

void _ProfileEnd(profmark_t *const mark)
{
    const u64 end = time_now();

    ASSERT(mark);

    const i32 tid = task_thread_id();
    node_t *const top = ms_top[tid];

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

static void VisitClr(node_t *const pim_noalias node)
{
    if (!node)
    {
        return;
    }

    profmark_t *const mark = node->mark;
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

static void VisitSum(node_t *const pim_noalias node)
{
    if (!node)
    {
        return;
    }

    profmark_t *const mark = node->mark;
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
    node->hash = hash;

    VisitSum(node->sibling);
    VisitSum(node->fchild);
}

static double GetNodeAvgMs(node_t const *const node)
{
    ASSERT(node);
    const u32 key = node->hash;
    ASSERT(key);

    double avgMs = 0.0;
    ASSERT(node->mark);

    dict_get(&ms_node_dict, &key, &avgMs);
    return avgMs;
}

static double UpdateNodeAvgMs(node_t const *const node)
{
    ASSERT(node);
    const u32 key = node->hash;
    ASSERT(key);

    const double ms = time_milli(node->end - node->begin);
    double avgMs = ms;
    ASSERT(node->mark);
    if (dict_get(&ms_node_dict, &key, &avgMs))
    {
        double alpha = 1.0 / ms_avgWindow;
        avgMs += (ms - avgMs) * alpha;
        dict_set(&ms_node_dict, &key, &avgMs);
    }
    else
    {
        dict_add(&ms_node_dict, &key, &avgMs);
    }
    return avgMs;
}

static void VisitGui(node_t const *const pim_noalias node)
{
    if (!node)
    {
        return;
    }

    profmark_t const *const mark = node->mark;
    ASSERT(mark);
    char const *const name = mark->name;
    ASSERT(name);

    double ms = UpdateNodeAvgMs(node);

    double pct = 0.0;
    const node_t* root = ms_prevroots[0].fchild;
    ASSERT(root);

    double rootMs = GetNodeAvgMs(root);
    if (rootMs > 0.0)
    {
        pct = 100.0 * (ms / rootMs);
    }

    igText("%s", name); igNextColumn();
    igText("%3.2f", ms); igNextColumn();
    igText("%4.1f%%", pct); igNextColumn();

    char key[32];
    SPrintf(ARGS(key), "%x", node->hash);

    igTreePushStr(key);
    VisitGui(node->fchild);
    igTreePop();
    VisitGui(node->sibling);
}

#else

void profile_gui(bool* pEnabled) {}

void _ProfileBegin(profmark_t *const mark) {}
void _ProfileEnd(profmark_t *const mark) {}

#endif // PIM_PROFILE
