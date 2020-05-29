#include "components/cubemaps.h"
#include "components/components.h"

// HashStr("Cubemaps"); see tools/prehash.py
static const u32 cubemaps_hash = 3261206463u;

table_t* Cubemaps_New(tables_t* tables)
{
    table_t* table = tables_add(tables, cubemaps_hash);
    table_add(table, TYPE_ARGS(Cubemap));
    table_add(table, TYPE_ARGS(BCubemap));
    table_add(table, TYPE_ARGS(bounds_t));
    return table;
}

void Cubemaps_Del(tables_t* tables)
{
    table_t* table = Cubemaps_Get(tables);
    if (table)
    {
        const i32 len = table_width(table);
        Cubemap* cubemaps = table_row(table, TYPE_ARGS(Cubemap));
        BCubemap* bakemaps = table_row(table, TYPE_ARGS(BCubemap));
        ASSERT(cubemaps);
        ASSERT(bakemaps);

        for (i32 i = 0; i < len; ++i)
        {
            Cubemap_Del(cubemaps + i);
            BCubemap_Del(bakemaps + i);
        }

        tables_rm(tables, cubemaps_hash);
    }
}

table_t* Cubemaps_Get(tables_t* tables)
{
    return tables_get(tables, cubemaps_hash);
}

ent_t Cubemaps_Add(tables_t* tables, i32 size, float4 pos, float radius)
{
    ent_t ent = { 0 };
    table_t* table = Cubemaps_Get(tables);
    if (table)
    {
        i32 i = col_add(table, &ent);
        Cubemap* cubemaps = table_row(table, TYPE_ARGS(Cubemap));
        BCubemap* bakemaps = table_row(table, TYPE_ARGS(BCubemap));
        bounds_t* bounds = table_row(table, TYPE_ARGS(bounds_t));

        ASSERT(cubemaps);
        ASSERT(bakemaps);
        ASSERT(bounds);

        Cubemap_New(cubemaps + i, size);
        BCubemap_New(bakemaps + i, size);
        pos.w = radius;
        bounds[i].Value.value = pos;
    }
    return ent;
}

bool Cubemaps_Rm(tables_t* tables, ent_t ent)
{
    bool removed = false;
    table_t* table = Cubemaps_Get(tables);
    if (table)
    {
        i32 i = col_get(table, ent);
        if (i >= 0)
        {
            Cubemap* cubemaps = table_row(table, TYPE_ARGS(Cubemap));
            BCubemap* bakemaps = table_row(table, TYPE_ARGS(BCubemap));

            Cubemap_Del(cubemaps + i);
            BCubemap_Del(bakemaps + i);

            removed = col_rm(table, ent);
        }
    }
    return removed;
}
