#pragma once

#include "components/entity.h"
#include "components/component.h"
#include "rendering/mesh.h"
#include "rendering/material.h"

struct Renderable
{
    MeshId mesh;
    MaterialId material;

    inline void Drop()
    {
        MeshSystem::Release(mesh);
        MaterialSystem::Release(material);
    }
};
DeclComponent(Renderable, ptr->Drop());
