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

// eventually the linear allocator will run out
#define kNodeLimit (1024)

// eventually the call stack will run out in the Gui
#define kDepthLimit (20)

// ----------------------------------------------------------------------------

typedef struct node_s
{
    ProfMark* mark;
    struct node_s* parent;
    struct node_s* fchild;
    struct node_s* lchild;
    struct node_s* sibling;
    u64 begin;
    u64 end;
    u64 hash;
    i32 depth;
} node_t;

typedef struct stat_s
{
    double mean;
    double variance;
} stat_t;

typedef struct ctx_s
{
    node_t prevroot;
    node_t root;
    node_t* current;
    u32 frame;
    u32 count;
    i32 depth;
} ctx_t;

// ----------------------------------------------------------------------------

static void VisitClr(node_t* node);
static void VisitSum(node_t* node);
static void VisitGui(const node_t* node);
static stat_t GetNodeStats(const node_t* node);
static stat_t UpdateNodeStats(const node_t* node);

// ----------------------------------------------------------------------------

static ctx_t ms_ctx[kMaxThreads];
static ctx_t* GetContext(void) { return &ms_ctx[Task_ThreadId()]; }

static Dict ms_stats =
{
    .keySize = sizeof(u64),
    .valueSize = sizeof(stat_t),
};
static i32 ms_avgWindow = 20;
static bool ms_progressive;

// ----------------------------------------------------------------------------

ProfileMark(pm_gui, ProfileSys_Gui)
void ProfileSys_Gui(bool* pEnabled)
{
    ProfileBegin(pm_gui);

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

        ctx_t* ctx = GetContext();
        node_t* root = ctx->prevroot.fchild;

        igSeparator();

        VisitClr(root);
        VisitSum(root);

        ImVec2 region;
        igGetContentRegionAvail(&region);
        igExColumns(5);
        igSetColumnWidth(0, region.x * 0.55f);
        igSetColumnWidth(1, region.x * (0.45f / 4.0f));
        igSetColumnWidth(2, region.x * (0.45f / 4.0f));
        igSetColumnWidth(3, region.x * (0.45f / 4.0f));
        igSetColumnWidth(4, region.x * (0.45f / 4.0f));
        {
            igText("Name"); igNextColumn();
            igText("Milliseconds"); igNextColumn();
            igText("Std Dev."); igNextColumn();
            igText("%%Root"); igNextColumn();
            igText("%%Parent"); igNextColumn();

            igSeparator();

            VisitGui(root);
        }
        igExColumns(1);
    }
    igEnd();

    ProfileEnd(pm_gui);
}

// ----------------------------------------------------------------------------

void _ProfileBegin(ProfMark *const mark)
{
    ASSERT(mark && mark->name);
    ctx_t *const pim_noalias ctx = GetContext();

    const u32 frame = Time_FrameCount();
    if (frame != ctx->frame)
    {
        ctx->frame = frame;
        ctx->count = 0;
        ctx->prevroot = ctx->root;
        ctx->root = (node_t) { .hash = Fnv64Bias };
        ctx->current = &ctx->root;
        ctx->depth = 0;
    }

    ctx->count++;
    ctx->depth++;
    if ((ctx->count < kNodeLimit) && (ctx->depth < kDepthLimit))
    {
        node_t *const parent = ctx->current ? ctx->current : &ctx->root;
        node_t *const pim_noalias next = Temp_Calloc(sizeof(*next));
        next->mark = mark;
        next->parent = parent;
        next->depth = ctx->depth;
        u64 prevHash = parent->hash;
        node_t *const sibling = parent->lchild;
        if (sibling)
        {
            sibling->sibling = next;
            prevHash = sibling->hash;
        }
        else
        {
            parent->fchild = next;
        }
        parent->lchild = next;
        next->hash = Fnv64Qword((u64)mark, prevHash) | 1;
        ctx->current = next;

        next->begin = Time_Now();
    }
}

void _ProfileEnd(ProfMark *const mark)
{
    const u64 end = Time_Now();

    ASSERT(mark && mark->name);

    ctx_t *const pim_noalias ctx = GetContext();
    if ((ctx->count < kNodeLimit) && (ctx->depth < kDepthLimit))
    {
        node_t *const current = ctx->current;

        ASSERT(current);
        ASSERT(current->mark == mark);
        ASSERT(current->depth == ctx->depth);
        ASSERT(current->parent);
        ASSERT(current->begin);
        ASSERT(!current->end);
        ASSERT(current->hash);
        ASSERT(Time_FrameCount() == ctx->frame);

        current->end = end;
        ctx->current = current->parent;
    }

    ctx->depth--;
}

// ----------------------------------------------------------------------------

pim_inline double VEC_CALL f64_lerp(double a, double b, double t)
{
    return a + (b - a) * t;
}

static void VisitClr(node_t* node)
{
    while (node)
    {
        ProfMark *const mark = node->mark;
        ASSERT(mark);
        mark->calls = 0;
        mark->sum = 0;

        if (node->depth < kDepthLimit)
        {
            VisitClr(node->fchild);
        }

        node = node->sibling;
    }
}

static void VisitSum(node_t* node)
{
    while (node)
    {
        ProfMark *const mark = node->mark;
        ASSERT(mark);
        mark->calls += 1;
        mark->sum += node->end - node->begin;

        if (node->depth < kDepthLimit)
        {
            VisitSum(node->fchild);
        }

        node = node->sibling;
    }
}

static void VisitGui(const node_t* node)
{
    while (node)
    {
        ProfMark const *const mark = node->mark;
        ASSERT(mark);
        char const *const name = mark->name;
        ASSERT(name);

        stat_t st = UpdateNodeStats(node);
        if (st.mean > 0.0001)
        {
            double parentPct = 0.0;
            if (node->parent)
            {
                stat_t parentStat = GetNodeStats(node->parent);
                if (parentStat.mean > 0.0)
                {
                    parentPct = 100.0 * (st.mean / parentStat.mean);
                }
            }
            double rootPct = 0.0;
            const node_t* const root = GetContext()->prevroot.fchild;
            if (root)
            {
                stat_t rootStat = GetNodeStats(root);
                if (rootStat.mean > 0.0)
                {
                    rootPct = 100.0 * (st.mean / rootStat.mean);
                }
            }

            igText("%s", name); igNextColumn();
            igText("%03.4f", st.mean); igNextColumn();
            igText("%03.4f", sqrt(st.variance)); igNextColumn();
            igText("%4.1f%%", rootPct); igNextColumn();
            igText("%4.1f%%", parentPct); igNextColumn();

            igTreePushPtr((void*)(node->hash));
            if (node->depth < kDepthLimit)
            {
                VisitGui(node->fchild);
            }
            igTreePop();
        }

        node = node->sibling;
    }
}

static stat_t GetNodeStats(const node_t* node)
{
    ASSERT(node);
    ASSERT(node->hash);
    stat_t st = { 0 };
    Dict_Get(&ms_stats, &node->hash, &st);
    return st;
}

static stat_t UpdateNodeStats(const node_t* node)
{
    ASSERT(node);
    ASSERT(node->mark);
    ASSERT(node->hash);
    double x = Time_Milli(node->end - node->begin);
    stat_t st;
    if (Dict_Get(&ms_stats, &node->hash, &st))
    {
        const double alpha = 1.0 / ms_avgWindow;
        double mean = st.mean;
        double variance = (x - mean) * (x - mean);
        st.variance = f64_lerp(st.variance, variance, alpha);
        st.mean = f64_lerp(mean, x, alpha);
        Dict_Set(&ms_stats, &node->hash, &st);
    }
    else
    {
        st.mean = x;
        st.variance = 0.0;
        Dict_Add(&ms_stats, &node->hash, &st);
    }
    return st;
}

#else

void ProfileSys_Gui(bool* pEnabled) {}

void _ProfileBegin(ProfMark *const mark) {}
void _ProfileEnd(ProfMark *const mark) {}

#endif // PIM_PROFILE
