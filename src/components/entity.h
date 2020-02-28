#pragma once

#include "containers/genid.h"

struct Entity
{
    GenId id;

    bool operator==(Entity rhs) const { return id == rhs.id; }
    bool operator!=(Entity rhs) const { return id != rhs.id; }
    bool operator<(Entity rhs) const { return id < rhs.id; }
    bool IsNull() const { return id.IsNull(); }
    bool IsNotNull() const { return id.IsNotNull(); }
    i32 GetIndex() const { return id.GetIndex(); }
    void SetIndex(i32 i) { id.SetIndex(i); }
    u8 GetVersion() const { return id.GetVersion(); }
    void SetVersion(u8 v) { id.SetVersion(v); }
};
