#include "common/profiler.h"

#if PIM_PROFILE

#include "threading/task.h"
#include "allocator/allocator.h"
#include "common/time.h"
#include "common/cvar.h"
#include "ui/cimgui.h"
#include <string.h>
#include <stdlib.h>

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
} node_t;

typedef struct marks_s
{
    profmark_t** marks;
    i32 length;
} marks_t;

// ----------------------------------------------------------------------------

static void Collect(profinfo_t* info);
static i32 FindMark(const profmark_t** hay, i32 count, const profmark_t* needle);
static void Visit(const node_t* node, marks_t* info);
static i32 MarkCmp(void const* lhs, void const* rhs);

// ----------------------------------------------------------------------------

static u32 ms_frame[kNumThreads];
static node_t ms_roots[kNumThreads];
static node_t* ms_top[kNumThreads];
static profinfo_t ms_info;

// ----------------------------------------------------------------------------

void profile_sys_init(void)
{
    cvar_reg(&cv_profilegui);
    memset(ms_frame, 0, sizeof(ms_frame));
    memset(ms_roots, 0, sizeof(ms_roots));
    memset(ms_top, 0, sizeof(ms_top));
    memset(&ms_info, 0, sizeof(ms_info));
}

ProfileMark(pm_sys_gui, profile_sys_gui)
void profile_sys_gui(void)
{
    ProfileBegin(pm_sys_gui);
    if (cv_profilegui.asFloat != 0.0f)
    {
        igBegin("Profiler", NULL, 0);

        const i32 len = ms_info.length;
        profmark_t* marks = ms_info.marks;
        qsort(marks, len, sizeof(*marks), MarkCmp);

        igColumns(3);
        igText("Name"); igNextColumn();
        igText("ms"); igNextColumn();
        igText("calls"); igNextColumn();
        igSeparator();
        for (i32 i = 0; i < len; ++i)
        {
            igText(marks[i].name); igNextColumn();
            igText("%f", marks[i].average); igNextColumn();
            igText("%d", marks[i].calls); igNextColumn();
        }
        igColumns(1);

        igEnd();
    }
    ProfileEnd(pm_sys_gui);
}

void profile_sys_collect(void)
{
    Collect(&ms_info);
}

void profile_sys_shutdown(void)
{
    memset(ms_frame, 0, sizeof(ms_frame));
    memset(ms_roots, 0, sizeof(ms_roots));
    memset(ms_top, 0, sizeof(ms_top));
    pim_free(ms_info.marks);
    memset(&ms_info, 0, sizeof(ms_info));
}

// ----------------------------------------------------------------------------

void _ProfileBegin(profmark_t* mark)
{
    ASSERT(mark);
    const i32 tid = task_thread_id();
    const u32 frame = time_framecount();

    if (frame != ms_frame[tid])
    {
        ms_frame[tid] = frame;
        ms_top[tid] = NULL;
        ms_roots[tid] = (node_t){ 0 };
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

const profinfo_t* ProfileInfo(void)
{
    return &ms_info;
}

// ----------------------------------------------------------------------------

static void Collect(profinfo_t* info)
{
    ASSERT(info);
    info->length = 0;

    marks_t marks = { 0 };
    for (i32 tid = 0; tid < NELEM(ms_roots); ++tid)
    {
        Visit(ms_roots + tid, &marks);
    }

    const double alpha = 1.0 / 30.0;
    const i32 len = marks.length;
    info->length = len;
    if (len > 0)
    {
        info->marks = perm_realloc(info->marks, sizeof(info->marks[0]) * len);
        for (i32 i = 0; i < len; ++i)
        {
            profmark_t* pMark = marks.marks[i];
            double ms = time_milli(pMark->sum);
            pMark->average += (ms - pMark->average) * alpha;
            info->marks[i] = *pMark;
            pMark->calls = 0;
            pMark->sum = 0;
        }
    }
}

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

static void Visit(const node_t* node, marks_t* info)
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

        if (FindMark(info->marks, info->length, node->mark) == -1)
        {
            i32 len = info->length + 1;
            info->length = len;
            info->marks = pim_realloc(EAlloc_Temp, info->marks, sizeof(info->marks[0]) * len);
            info->marks[len - 1] = mark;
        }
    }

    Visit(node->sibling, info);
    Visit(node->fchild, info);
}

static i32 MarkCmp(void const* vlhs, void const* vrhs)
{
    const profmark_t* lhs = vlhs;
    const profmark_t* rhs = vrhs;
    if (lhs->average == rhs->average)
    {
        return 0;
    }
    return lhs->average > rhs->average ? -1 : 1;
}

#else

void profile_sys_init(void) {}
void profile_sys_gui(void) {}
void profile_sys_collect(void) {}
void profile_sys_shutdown(void) {}

void _ProfileBegin(profmark_t* mark) {}
void _ProfileEnd(profmark_t* mark) {}

const profinfo_t* ProfileInfo(void) { return NULL; }

#endif // PIM_PROFILE
