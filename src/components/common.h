#pragma once

#include "components/component.h"
#include "common/guid.h"

struct Identifier
{
    Guid Value;

    static constexpr ComponentId Id = ComponentId_Identifier;
};

#define COMPONENTS_COMMON(XX) \
    XX(Identifier)
