#include "containers/graph.h"
#include "allocator/allocator.h"

typedef struct vertex_s
{
    i32* edges;
    i32 length;
    i32 mark;
} vertex_t;

static void graph_visit(vertex_t* verts, i32* order, i32* orderLen, i32 iVert)
{
    vertex_t* vert = verts + iVert;
    ASSERT(vert->mark != 1);
    if (vert->mark == 0)
    {
        vert->mark = 1;
        const i32* edges = vert->edges;
        const i32 len = vert->length;
        for (i32 i = 0; i < len; ++i)
        {
            graph_visit(verts, order, orderLen, edges[i]);
        }
        vert->mark = 2;
        const i32 back = orderLen[0]++;
        order[back] = iVert;
    }
}

void graph_new(graph_t* graph, EAlloc allocator)
{
    ASSERT(graph);
    graph->verts = NULL;
    graph->length = 0;
    graph->allocator = allocator;
}

void graph_del(graph_t* graph)
{
    ASSERT(graph);
    graph_clear(graph);
    pim_free(graph->verts);
    graph->verts = NULL;
    graph->length = 0;
}

void graph_clear(graph_t* graph)
{
    vertex_t* verts = graph->verts;
    const i32 len = graph->length;
    for (i32 i = 0; i < len; ++i)
    {
        pim_free(verts[i].edges);
        verts[i].edges = NULL;
        verts[i].length = 0;
    }
    graph->length = 0;
}

void graph_sort(graph_t* graph, i32* order, i32 length)
{
    const i32 graphLength = graph->length;
    vertex_t* verts = graph->verts;
    ASSERT(order);
    ASSERT(length == graphLength);

    for (i32 i = 0; i < graphLength; ++i)
    {
        verts[i].mark = 0;
    }

    length = 0;
    for (i32 i = 0; i < graphLength; ++i)
    {
        if (verts[i].mark == 0)
        {
            graph_visit(verts, order, &length, i);
        }
    }
    ASSERT(length == graphLength);
}

i32 graph_size(const graph_t* graph)
{
    return graph->length;
}

i32 graph_addvert(graph_t* graph)
{
    i32 back = graph->length++;
    graph->verts = pim_realloc(graph->allocator, graph->verts, sizeof(vertex_t) * (back + 1));
    vertex_t vert;
    vert.edges = NULL;
    vert.length = 0;
    vert.mark = 0;
    vertex_t* verts = graph->verts;
    verts[back] = vert;
    return back;
}

bool graph_addedge(graph_t* graph, i32 src, i32 dst)
{
    vertex_t* verts = graph->verts;
    ASSERT(src < graph->length);
    ASSERT(dst < graph->length);

    vertex_t* vert = verts + dst;
    const i32 back = vert->length;
    i32* edges = vert->edges;
    for (i32 i = back - 1; i >= 0; --i)
    {
        if (edges[i] == src)
        {
            return false;
        }
    }

    edges = pim_realloc(graph->allocator, edges, sizeof(i32) * (back + 1));
    edges[back] = src;

    vert->edges = edges;
    vert->length = back + 1;

    return true;
}

const i32* graph_edges(const graph_t* graph, i32 dstVert, i32* pLength)
{
    ASSERT(pLength);
    const vertex_t* verts = graph->verts;
    const i32 len = graph->length;
    ASSERT(dstVert < len);
    *pLength = verts[dstVert].length;
    return verts[dstVert].edges;
}
