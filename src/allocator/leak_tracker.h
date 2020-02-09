#pragma once

#include "stackwalker/StackWalker.h"
#include "common/guid.h"
#include "common/guid_util.h"
#include "common/text.h"
#include "containers/hash_dict.h"
#include "common/comparator_util.h"

struct LeakTracker final : StackWalker::Walker
{
    using StackText = Text<4096>;

    struct Allocation
    {
        Guid stack;
        i32 size;
    };

    struct Leak
    {
        i32 count;
        i32 size;
    };

    static constexpr auto VoidCmp = PtrComparator<void*>();
    HashDict<void*, Allocation, VoidCmp> m_items;
    HashDict<Guid, StackText, GuidComparator> m_stacks;
    HashDict<Guid, Leak, GuidComparator> m_leaks;

    void* m_ptr;
    Allocation m_current;
    StackText m_text;

    LeakTracker() : StackWalker::Walker()
    {
        m_items.Init(Alloc_Debug);
        m_stacks.Init(Alloc_Debug);
        m_leaks.Init(Alloc_Debug);
    }

    void OnAlloc(void* ptr, i32 size)
    {
        if (!ptr)
        {
            return;
        }

        m_ptr = ptr;
        m_current.size = size;
        m_current.stack = {};
        m_text.value[0] = 0;
        ShowCallstack();
    }

    void OnFree(void* ptr)
    {
        if (!ptr)
        {
            return;
        }

        Allocation item = {};
        if (m_items.Remove(ptr, item))
        {
            Leak leak = {};
            if (m_leaks.Get(item.stack, leak))
            {
                leak.count -= 1;
                leak.size -= item.size;
                ASSERT(leak.count >= 0);
                ASSERT(leak.size >= 0);
                m_leaks.Set(item.stack, leak);
            }
        }
    }

    void OnRealloc(void* pOld, void* pNew, i32 size)
    {
        OnFree(pOld);
        OnAlloc(pNew, size);
    }

    void OnCallstackEntry(
        StackWalker::CallstackEntryType type,
        StackWalker::CallstackEntry& entry) final
    {
        if (type == StackWalker::firstEntry)
        {
            m_text.value[0] = 0;
        }

        StrCatf(
            ARGS(m_text.value),
            "%s (%d): %s\n",
            entry.lineFileName,
            entry.lineNumber,
            entry.name);

        if (type == StackWalker::lastEntry)
        {
            Guid id = ToGuid(m_text.value);
            m_current.stack = id;
            m_stacks.Add(id, m_text);
            m_items.Add(m_ptr, m_current);
            Leak leak = {};
            if (m_leaks.Get(id, leak))
            {
                leak.count += 1;
                leak.size += m_current.size;
                m_leaks.Set(id, leak);
            }
            else
            {
                leak.count = 1;
                leak.size = m_current.size;
                m_leaks.Add(id, leak);
            }
        }
    }

    void ListLeaks()
    {
        Array<Guid> ids = CreateArray<Guid>(Alloc_Debug, 0);
        Array<Leak> leaks = CreateArray<Leak>(Alloc_Debug, 0);
        m_leaks.GetElements(ids, leaks);

        i32 leakCount = 0;
        for (i32 i = 0; i < ids.size(); ++i)
        {
            Guid id = ids[i];
            Leak leak = leaks[i];
            ASSERT(leak.count >= 0);
            if (leak.count > 0)
            {
                StackText stack = {};
                m_stacks.Get(id, stack);
                char buffer[64];
                SPrintf(ARGS(buffer), "LEAK: ct: %d sz: %d\n", leak.count, leak.size);
                OnOutput(buffer);
                OnOutput(stack.value);
                ++leakCount;
            }
        }

        if (leakCount == 0)
        {
            OnOutput("No leaks! : )\n");
        }

        ids.Reset();
        leaks.Reset();
    }
};
