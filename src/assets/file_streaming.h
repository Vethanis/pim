
#include "common/int_types.h"
#include "common/guid.h"
#include "common/minmax.h"
#include "threading/task_group.h"

struct FileTask final : ITask
{
    Guid m_guid;
    i32 m_offset;
    i32 m_size;
    void* m_ptr;
    bool m_write;

    void Execute(u32, u32, u32) final;

    using CmpType = ITask*;
    static i32 Compare(const CmpType& lhs, const CmpType& rhs);
};
