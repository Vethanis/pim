#pragma once

#include "rendering/mesh.h"
#include "rendering/material.h"

struct Renderable
{
    MeshId mesh;
    MaterialId material;
};
