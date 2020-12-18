#include "rendering/gltf_model.h"

#include "allocator/allocator.h"
#include "rendering/drawable.h"
#include "rendering/mesh.h"
#include "rendering/texture.h"
#include "rendering/material.h"
#include "io/fmap.h"
#include "common/console.h"
#include "common/guid.h"
#include "math/types.h"
#include "math/float4x4_funcs.h"

#include "cgltf/cgltf.h"
#include <string.h>

static bool CreateScenes(cgltf_data* cgdata, drawables_t* dr);
static bool CreateScene(cgltf_scene* cgscene, drawables_t* dr);
static bool CreateNode(cgltf_node* cgnode, drawables_t* dr);
static bool CreateMaterial(cgltf_material* cgmat, drawables_t* dr);
static bool CreateTexture(cgltf_texture* cgtex, drawables_t* dr);
static bool CreateMesh(cgltf_mesh* cgmesh, drawables_t* dr, i32 iDrawable);
static bool CreateLight(cgltf_light* cglight, drawables_t* dr, i32 iDrawable);

bool gltf_model_load(const char* path, drawables_t* dr)
{
    bool success = true;
    fmap_t map = { 0 };
    cgltf_data* cgdata = NULL;
    cgltf_options options = { 0 };
    cgltf_result result = cgltf_result_success;

    map = fmap_open(path, false);
    if (!fmap_isopen(map))
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }

    result = cgltf_parse(&options, map.ptr, map.size, &cgdata);
    if (result != cgltf_result_success)
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }
    result = cgltf_load_buffers(&options, cgdata, path);
    if (result != cgltf_result_success)
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }
#if _DEBUG
    result = cgltf_validate(cgdata);
    if (result != cgltf_result_success)
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }
#endif // _DEBUG

    success = CreateScenes(cgdata, dr);

cleanup:
    cgltf_free(cgdata);
    fmap_close(&map);
    return success;
}

static bool CreateScenes(cgltf_data* cgdata, drawables_t* dr)
{
    const i32 sceneCount = (i32)cgdata->scenes_count;
    cgltf_scene* cgscenes = cgdata->scenes;
    for (i32 i = 0; i < sceneCount; ++i)
    {
        if (!CreateScene(&cgscenes[i], dr))
        {
            return false;
        }
    }
    return true;
}

static bool CreateScene(cgltf_scene* cgscene, drawables_t* dr)
{
    const i32 nodeCount = (i32)cgscene->nodes_count;
    cgltf_node** cgnodes = cgscene->nodes;
    for (i32 i = 0; i < nodeCount; ++i)
    {
        if (!CreateNode(cgnodes[i], dr))
        {
            return false;
        }
    }
    return true;
}

static bool CreateNode(cgltf_node* cgnode, drawables_t* dr)
{
    if (!cgnode)
    {
        return false;
    }
    const char* name = cgnode->name;
    if (!name || !name[0])
    {
        ASSERT(false);
        return false;
    }

    const guid_t guid = guid_str(name);
    const i32 iDrawable = drawables_add(dr, guid);

    float4x4 localToWorld = f4x4_id;
    cgltf_node_transform_world(cgnode, &localToWorld.c0.x);
    dr->matrices[iDrawable] = localToWorld;

    CreateLight(cgnode->light, dr, iDrawable);
    CreateMesh(cgnode->mesh, dr, iDrawable);

    const i32 childCount = (i32)cgnode->children_count;
    cgltf_node** children = cgnode->children;
    for (i32 iChild = 0; iChild < childCount; ++iChild)
    {
        CreateNode(children[iChild], dr);
    }

    return true;
}

static bool CreateMaterial(cgltf_material* cgmat, drawables_t* dr)
{
    if (!cgmat)
    {
        return false;
    }
    const char* name = cgmat->name;
    if (!name || !name[0])
    {
        ASSERT(false);
        return false;
    }

    return true;
}

static bool CreateTexture(cgltf_texture* cgtex, drawables_t* dr)
{
    if (!cgtex)
    {
        return false;
    }
    cgltf_image* cgimage = cgtex->image;
    if (!cgimage)
    {
        ASSERT(false);
        return false;
    }
    const char* name = cgimage->name;
    if (!name || !name[0])
    {
        ASSERT(false);
        return false;
    }
    guid_t guid = guid_str(name);
    textureid_t id;
    if (texture_find(guid, &id))
    {
        texture_retain(id);
        return true;
    }


    return true;
}

static bool CreateMesh(cgltf_mesh* cgmesh, drawables_t* dr, i32 iDrawable)
{
    if (!cgmesh)
    {
        return false;
    }
    const char* name = cgmesh->name;
    if (!name || !name[0])
    {
        ASSERT(false);
        return false;
    }
    guid_t guid = guid_str(name);
    meshid_t id;
    if (mesh_find(guid, &id))
    {
        mesh_retain(id);
        return true;
    }

    const i32 primCount = (i32)cgmesh->primitives_count;
    cgltf_primitive* cgprims = cgmesh->primitives;
    for (i32 iPrim = 0; iPrim < primCount; ++iPrim)
    {

    }

    return true;
}

static bool CreateLight(cgltf_light* cglight, drawables_t* dr, i32 iDrawable)
{
    if (!cglight)
    {
        return false;
    }

    return true;
}
