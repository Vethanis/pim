#pragma once

#include "stackwalker/StackWalker.h"
#include "common/guid.h"
#include "common/guid_util.h"
#include "common/text.h"
#include "containers/hash_dict.h"

struct LeakTracker
{
    StackWalker::Walker m_base;

    using StackText = Text<4096>;

    struct Item
    {
        void* ptr;
        i32 size;
        StackText text;
    };

    struct Leak
    {
        i32 count;
        i32 size;
        StackText text;
    };

    static constexpr auto PtrComparator = OpComparator<void*>();
    HashDict<void*, Item, PtrComparator> m_items;
    Item m_current;

    void ShowCallstack()
    {
        m_items.m_allocator = Alloc_Debug;
        m_base.OnCallstackEntry = SOnEntry;
        m_base.OnOutput = StackWalker::EmptyOnOutput;
        m_base.OnError = StackWalker::EmptyOnError;
        StackWalker::Init(m_base);
        StackWalker::ShowCallstack(m_base);
    }

    void OnAlloc(void* ptr, i32 size)
    {
        m_current.ptr = ptr;
        m_current.size = size;
        m_current.text.value[0] = 0;
        ShowCallstack();
    }

    void OnFree(void* ptr)
    {
        m_items.Remove(ptr);
    }

    void OnRealloc(void* pOld, void* pNew, i32 size)
    {
        OnFree(pOld);
        OnAlloc(pNew, size);
    }

    void OnEntry(
        StackWalker::CallstackEntryType type,
        StackWalker::CallstackEntry& entry)
    {
        if (type == StackWalker::firstEntry)
        {
            m_current.text.value[0] = 0;
        }

        StrCatf(
            ARGS(m_current.text.value),
            "%s (%d): %s\n",
            entry.lineFileName,
            entry.lineNumber,
            entry.name);

        if (type == StackWalker::lastEntry)
        {
            m_items[m_current.ptr] = m_current;
        }
    }

    static void SOnEntry(
        StackWalker::Walker& walker,
        StackWalker::CallstackEntryType type,
        StackWalker::CallstackEntry& entry)
    {
        reinterpret_cast<LeakTracker&>(walker).OnEntry(type, entry);
    }

    void ListLeaks()
    {
        HashDict<Guid, Leak, GuidComparator::Value> leaks;
        leaks.Init(Alloc_Debug);

        for (auto pair : m_items)
        {
            Guid id = ToGuid(pair.Value.text.value);
            Leak* pLeak = leaks.Get(id);
            if (!pLeak)
            {
                pLeak = &leaks[id];
                pLeak->count = 0;
                pLeak->size = 0;
                pLeak->text = pair.Value.text;
            }
            pLeak->count += 1;
            pLeak->size += pair.Value.size;
        }

        for (auto pair : leaks)
        {
            char buffer[64];
            SPrintf(ARGS(buffer), "LEAK: %d %d\n", pair.Value.count, pair.Value.size);
            StackWalker::DefaultOnOutput(m_base, buffer);
            StackWalker::DefaultOnOutput(m_base, pair.Value.text.value);
        }

        if (!m_items.size())
        {
            StackWalker::DefaultOnOutput(m_base, "No Leaks! : )\n");
        }

        leaks.Reset();
    }
};
