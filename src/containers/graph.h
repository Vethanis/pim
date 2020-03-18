#pragma once

#include "containers/array.h"

struct Graph
{
    Array<u8> m_marks;
    Array<Array<i32>> m_edges;
    Array<i32> m_ids;

    void Init(EAlloc allocator = EAlloc_Perm)
    {
        m_marks.Init(allocator);
        m_edges.Init(allocator);
        m_ids.Init(allocator);
    }

    void Reset()
    {
        m_marks.Reset();
        for (Array<i32>& edges : m_edges)
        {
            edges.Reset();
        }
        m_edges.Reset();
        m_ids.Reset();
    }

    void Clear()
    {
        m_marks.Clear();
        m_ids.Clear();
        for (Array<i32>& edgelist : m_edges)
        {
            edgelist.Clear();
        }
    }

    i32 size() const { return m_ids.size(); }
    i32 operator[](i32 i) const { return m_ids[i]; }
    const i32* begin() const { return m_ids.begin(); }
    const i32* end() const { return m_ids.end(); }

    i32 AddVertex()
    {
        const i32 id = m_marks.size();
        m_marks.PushBack(0u);
        if (m_edges.size() == id)
        {
            m_edges.Grow().Init(m_edges.GetAllocator());
        }
        return id;
    }

    bool AddEdge(i32 src, i32 dst)
    {
        return m_edges[dst].FindAdd(src);
    }

    Slice<const i32> GetEdges(i32 id)
    {
        return m_edges[id];
    }

    void Sort()
    {
        const i32 len = m_marks.size();
        u8* marks = m_marks.begin();

        m_ids.Clear();
        m_ids.Reserve(len);

        memset(marks, 0, len);
        for (i32 i = 0; i < len; ++i)
        {
            if (!marks[i])
            {
                Visit(i);
            }
        }
    }

private:
    void Visit(i32 n)
    {
        u8& mark = m_marks[n];
        ASSERT(mark != 1u);
        if (!mark)
        {
            mark = 1u;
            for (i32 m : m_edges[n])
            {
                Visit(m);
            }
            mark = 2u;
            m_ids.PushBack(n);
        }
    }
};
