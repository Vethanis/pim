#include "common/profiler.h"

#if PIM_PROFILE

#include "threading/task.h"
#include "allocator/allocator.h"
#include "common/time.h"
#include "common/cvar.h"
#include "common/fnv1a.h"
#include "common/stringutil.h"
#include "containers/dict.h"
#include "ui/cimgui_ext.h"
#include <string.h>
#include <math.h>

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

typedef struct stat_s
{
    double mean;
    double variance;
} stat_t;

// ----------------------------------------------------------------------------

static void VisitClr(node_t *const pim_noalias node, i32 depth);
static void VisitSum(node_t *const pim_noalias node, i32 depth);
static void VisitGui(node_t const *const pim_noalias node, i32 depth);

// ----------------------------------------------------------------------------

static u32 ms_frame[kMaxThreads];
static node_t ms_prevroots[kMaxThreads];
static node_t ms_roots[kMaxThreads];
static node_t* ms_top[kMaxThreads];

static i32 ms_avgWindow = 20;
static bool ms_progressive;
static dict_t ms_stats;

// ----------------------------------------------------------------------------

static void EnsureDict(void)
{
    if (!ms_stats.valueSize)
    {
        dict_new(&ms_stats, sizeof(u32), sizeof(stat_t), EAlloc_Perm);
    }
}

ProfileMark(pm_gui, profile_gui)
void profile_gui(bool* pEnabled)
{
    ProfileBegin(pm_gui);
    EnsureDict();

    if (igBegin("Profiler", pEnabled, 0))
    {
        if (igCheckbox("Progressive", &ms_progressive))
        {
            ms_avgWindow = ms_progressive ? 0 : 1;
        }
        if (ms_progressive)
        {
            ++ms_avgWindow;
            igText("Window: %d", ms_avgWindow);
        }
        else
        {
            igExSliderInt("Window", &ms_avgWindow, 1, 1000);
        }

        node_t* root = ms_prevroots[0].fchild;

        igSeparator();

        VisitClr(root, 0);
        VisitSum(root, 0);

        ImVec2 region;
        igGetContentRegionAvail(&region);
        igExColumns(4);
        igSetColumnWidth(0, region.x * 0.6f);
        igSetColumnWidth(1, region.x * 0.13333333333f);
        igSetColumnWidth(2, region.x * 0.13333333333f);
        igSetColumnWidth(3, region.x * 0.13333333333f);
        {
            igText("Name"); igNextColumn();
            igText("Milliseconds"); igNextColumn();
            igText("Std Dev."); igNextColumn();
            igText("Percent"); igNextColumn();

            igSeparator();

            VisitGui(root, 0);
        }
        igExColumns(1);
    }
    igEnd();

    ProfileEnd(pm_gui);
}

// ----------------------------------------------------------------------------

void _ProfileBegin(profmark_t *const mark)
{
    ASSERT(mark);
    const i32 tid = Task_ThreadId();
    const u32 frame = Time_FrameCount();

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

    node_t *const next = Temp_Calloc(sizeof(*next));
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

    next->begin = Time_Now();
}

void _ProfileEnd(profmark_t *const mark)
{
    const u64 end = Time_Now();

    ASSERT(mark);

    const i32 tid = Task_ThreadId();
    node_t *const top = ms_top[tid];

    ASSERT(top);
    ASSERT(top->mark == mark);
    ASSERT(top->parent);
    ASSERT(top->begin);
    ASSERT(top->end == 0);
    ASSERT(Time_FrameCount() == ms_frame[tid]);

    top->end = end;
    ms_top[tid] = top->parent;
}

// ----------------------------------------------------------------------------

static double Lerp64(double a, double b, double t)
{
    return a + (b - a) * t;
}

static void VisitClr(node_t *const pim_noalias node, i32 depth)
{
    if (!node || (depth > 100))
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

    VisitClr(node->sibling, depth + 1);
    VisitClr(node->fchild, depth + 1);
}

static void VisitSum(node_t *const pim_noalias node, i32 depth)
{
    if (!node || (depth > 100))
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

    VisitSum(node->sibling, depth + 1);
    VisitSum(node->fchild, depth + 1);
}

static stat_t GetNodeStats(node_t const *const node)
{
    ASSERT(node);
    ASSERT(node->mark);
    ASSERT(node->hash);
    stat_t st = { 0 };
    dict_get(&ms_stats, &node->hash, &st);
    return st;
}

static stat_t UpdateNodeStats(node_t const *const node)
{
    ASSERT(node);
    ASSERT(node->mark);
    ASSERT(node->hash);
    double x = Time_Milli(node->end - node->begin);
    stat_t st;
    if (dict_get(&ms_stats, &node->hash, &st))
    {
        const double alpha = 1.0 / ms_avgWindow;
        double m0 = st.mean;
        double m1 = Lerp64(m0, x, alpha);
        st.variance = Lerp64(st.variance, (x - m0) * (x - m1), alpha);
        st.mean = m1;
        dict_set(&ms_stats, &node->hash, &st);
    }
    else
    {
        st.mean = x;
        st.variance = 0.0;
        dict_add(&ms_stats, &node->hash, &st);
    }
    return st;
}

static void VisitGui(node_t const *const pim_noalias node, i32 depth)
{
    if (!node || (depth > 100))
    {
        return;
    }

    profmark_t const *const mark = node->mark;
    ASSERT(mark);
    char const *const name = mark->name;
    ASSERT(name);

    stat_t st = UpdateNodeStats(node);

    double pct = 0.0;
    const node_t* root = ms_prevroots[0].fchild;
    ASSERT(root);

    stat_t rootst = GetNodeStats(root);
    if (rootst.mean > 0.0)
    {
        pct = 100.0 * (st.mean / rootst.mean);
    }

    if (st.mean > 0.0001)
    {
        igText("%s", name); igNextColumn();
        igText("%03.4f", st.mean); igNextColumn();
        igText("%03.4f", sqrt(st.variance)); igNextColumn();
        igText("%4.1f%%", pct); igNextColumn();

        char key[32];
        SPrintf(ARGS(key), "%x", node->hash);

        igTreePushStr(key);
        VisitGui(node->fchild, depth + 1);
        igTreePop();
    }
    VisitGui(node->sibling, depth + 1);
}

#else

void profile_gui(bool* pEnabled) {}

void _ProfileBegin(profmark_t *const mark) {}
void _ProfileEnd(profmark_t *const mark) {}

#endif // PIM_PROFILE
