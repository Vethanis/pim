#pragma once

#include "common/compiletimehash.hpp"

using TypeId = u32;

struct Unknown
{
    virtual ~Unknown() {}
    virtual TypeId GetTypeId() const = 0;
};

#define TYPEID_IMPL(T) \
    static constexpr TypeId ClassTypeId = CompileTimeHash32(#T); \
    TypeId GetTypeId() const { return ClassTypeId; }

