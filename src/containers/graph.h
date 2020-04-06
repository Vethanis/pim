#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct graph_s
{
    void* verts;
    i32 length;
    EAlloc allocator;
} graph_t;

void graph_new(graph_t* graph, EAlloc allocator);
void graph_del(graph_t* graph);

void graph_clear(graph_t* graph);

i32 graph_size(const graph_t* graph);
void graph_sort(graph_t* graph, i32* order, i32 length);

i32 graph_addvert(graph_t* graph);
bool graph_addedge(graph_t* graph, i32 srcVert, i32 dstVert);
const i32* graph_edges(const graph_t* graph, i32 dstVert, i32* pLength);

PIM_C_END
