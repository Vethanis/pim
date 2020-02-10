#include "components/component_mgr.h"
#include "common/typeid.h"
#include "components/component_row.h"
#include "common/chunk_allocator.h"

namespace ComponentManager
{
    static TPool<ComponentRow> ms_pool = TPool<ComponentRow>();

    ComponentRow* EnsureRow(TypeData& typeData)
    {
        ComponentRow* pRow = LoadPtr(typeData.pRow, MO_Relaxed);
        if (!pRow)
        {
            ComponentRow* pNew = ms_pool.Allocate();
            pNew->Init(typeData.sizeOf);
            if (CmpExStrongPtr(typeData.pRow, pRow, pNew, MO_Acquire))
            {
                return pNew;
            }
            pNew->Reset();
            ms_pool.Free(pNew);
            pRow = LoadPtr(typeData.pRow);
        }
        ASSERT(pRow);
        return pRow;
    }

    void ReleaseRow(TypeData& typeData)
    {
        ComponentRow* pRow = LoadPtr(typeData.pRow, MO_Relaxed);
        if (pRow && CmpExStrongPtr(typeData.pRow, pRow, (ComponentRow*)0, MO_Acquire))
        {
            pRow->Reset();
            ms_pool.Free(pRow);
        }
    }
};
