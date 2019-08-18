#include "tables/tables.h"

#include "tables/entity_table.h"

namespace Tables
{
    void Init()
    {
        EntityTable::Init();
    }
    void Shutdown()
    {
        EntityTable::Shutdown();
    }
    void Visualize()
    {
        EntityTable::Visualize();
    }
};
