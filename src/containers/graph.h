#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef struct Graph_s
{
    void* verts;
    i32 length;
    EAlloc allocator;
} Graph;

void Graph_New(Graph* graph, EAlloc allocator);
void Graph_Del(Graph* graph);

void Graph_Clear(Graph* graph);

i32 Graph_Size(const Graph* graph);
void Graph_Sort(Graph* graph, i32* order, i32 length);

i32 Graph_AddVert(Graph* graph);
bool Graph_AddEdge(Graph* graph, i32 srcVert, i32 dstVert);
const i32* Graph_Edges(const Graph* graph, i32 dstVert, i32* pLength);

PIM_C_END
