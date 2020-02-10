#pragma once

struct TypeData;
struct ComponentRow;

namespace ComponentManager
{
    ComponentRow* EnsureRow(TypeData& typeData);
    void ReleaseRow(TypeData& typeData);
};
