#pragma once

#include "common/macro.h"
#include "containers/array.hpp"
#include "containers/hash32.hpp"

class TaskGraph;

class TaskNode
{
    friend class ::TaskGraph;
public:
    TaskNode(const char* name) :
        m_name(name),
        m_visited(0),
        m_status(0),
        m_worksize(0),
        m_head(0),
        m_tail(0)
    {}
    virtual ~TaskNode() {}

    virtual void Execute(i32 a, i32 b) = 0;

    Hash32 GetName() const { return m_name; }

private:
    TaskNode(const TaskNode& other) = delete;
    TaskNode& operator=(const TaskNode& other) = delete;

    i32 m_status;
    i32 m_worksize;
    i32 m_head;
    i32 m_tail;
    i32 m_visited;
    Hash32 m_name;
    Array<TaskNode*> m_preds;
};
