#pragma once

#include "common/macro.h"
#include "common/int_types.h"

// ----------------------------------------------------------------------------

enum TableId : u16
{
    TableId_Default = 0,
    TableId_Map,
    TableId_Props,
    TableId_Characters,
    TableId_Weapons,
    TableId_Projectiles,
    TableId_UI,

    TableId_Count
};

// ----------------------------------------------------------------------------

struct Entity
{
    TableId table;
    u16 version;
    u16 index;

    inline bool IsNull() const { return version == 0; }
    inline bool IsNotNull() const { return version != 0; }
};

// ----------------------------------------------------------------------------

#define DeclComponent(T, DtorExpr) \
    static constexpr ComponentType C_Type = ComponentType_ ## T; \
    static void OnDestroy(T* ptr) { DtorExpr; }

// ----------------------------------------------------------------------------

enum ComponentType : u16
{
    ComponentType_Null = 0,
    ComponentType_Translation,
    ComponentType_Rotation,
    ComponentType_Scale,
    ComponentType_LocalToWorld,
    ComponentType_Children,

    ComponentType_Renderable,

    ComponentType_Count
};
