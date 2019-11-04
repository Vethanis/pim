#pragma once

#include "components/entity.h"
#include "rendering/mesh.h"
#include "rendering/material.h"

struct Renderable
{
    MeshId mesh;
    MaterialId material;

    DeclComponent(Renderable,
        MeshSystem::Release(ptr->mesh);
        MaterialSystem::Release(ptr->material));
};
