#pragma once

#include "components/component.h"
#include "geometry/types.h"

enum class ColliderType : u8
{
    Sphere,
    Box,
    Capsule,
    Plane,

    COUNT
};

struct Collider
{
    ColliderType type;
    union
    {
        Sphere asSphere;
        Box asBox;
        Capsule asCapsule;
        Plane asPlane;
    };
    static constexpr ComponentId Id = ComponentId_Collider;
};

struct Rigidbody
{
    quaternion angularVelocity;
    float3 linearVelocity;
    float mass;
    float3 linearDamping;
    float angularDamping;
    static constexpr ComponentId Id = ComponentId_Rigidbody;
};

#define COMPONENTS_PHYSICS(XX) \
    XX(Collider) \
    XX(Rigidbody)
