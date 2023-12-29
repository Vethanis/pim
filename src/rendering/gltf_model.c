#include "rendering/gltf_model.h"

#include "allocator/allocator.h"
#include "rendering/drawable.h"
#include "rendering/mesh.h"
#include "rendering/texture.h"
#include "rendering/material.h"
#include "rendering/sampler.h"
#include "io/fmap.h"
#include "common/console.h"
#include "common/guid.h"
#include "common/stringutil.h"
#include "common/random.h"
#include "common/nextpow2.h"
#include "stb/stb_image.h"
#include "math/types.h"
#include "math/int2_funcs.h"
#include "math/float4x4_funcs.h"
#include "math/color.h"

#include "cgltf/cgltf.h"
#include <string.h>

// ----------------------------------------------------------------------------

static bool CreateScenes(
    const char* basePath,
    const cgltf_data* cgdata,
    Entities* dr);
static bool CreateNode(
    const char* basePath,
    const char* parentPath,
    const cgltf_node* cgnode,
    Entities* dr);
static bool CreateMaterial(
    const char* basePath,
    const char* parentPath,
    const cgltf_material* cgmat,
    Material* matOut);
static TextureId CreateAlbedoTexture(
    const char* basePath,
    const cgltf_image* cgalbedo);
static TextureId CreateRomeTexture(
    const char* basePath,
    const cgltf_image* cgmetallic_roughness,
    const cgltf_image* cgocclusion,
    const cgltf_image* cgemission);
static TextureId CreateNormalTexture(
    const char* basePath,
    const cgltf_image* cgnormal);
bool ResampleToAlbedoRome(
    const char* basePath,
    Material* material,
    const cgltf_image* cgdiffuse,
    const cgltf_image* cgspecular,
    const cgltf_image* cgocclusion,
    const cgltf_image* cgemission,
    float glossiness);

// ----------------------------------------------------------------------------

static i32 CatNodeLineage(char* dst, i32 size, const cgltf_node* cgnode);
static char const* const ShortenString(char const* const str, i32 maxLen);
static bool LoadImageUri(
    const char* basePath,
    const char* uri,
    Texture* tex);
static bool ResampleTextureToFloat4(Texture* tex);
static bool ResampleToSrgb(Texture* tex);
static bool ResampleToEmission(Texture* tex);
static bool ImportColorspace(Texture* tex);
static bool ExportColorspace(Texture* tex);

static void* Gltf_Alloc(void* user, cgltf_size size)
{
    return Perm_Alloc((i32)size);
}
static void Gltf_Free(void* user, void* ptr)
{
    Mem_Free(ptr);
}

typedef struct GltfFileHandler_s
{
    i32 fileCount;
    Guid* names;
    FileMap* files;
    i32* refCounts;
} GltfFileHandler;
static GltfFileHandler s_fileHandler;

static cgltf_result Gltf_FileRead(
    const cgltf_memory_options* memory_options,
    const cgltf_file_options* file_options,
    const char* path,
    cgltf_size* size,
    void** data);
static void Gltf_FileRelease(
    const cgltf_memory_options* memory_options,
    const cgltf_file_options* file_options,
    void* data);

// ----------------------------------------------------------------------------

bool Gltf_Load(const char* path, Entities* dr)
{
    bool success = true;
    FileMap map = { 0 };
    cgltf_data* cgdata = NULL;
    const cgltf_options options =
    {
        .file.read = Gltf_FileRead,
        .file.release = Gltf_FileRelease,
        .file.user_data = &s_fileHandler,
    };
    cgltf_result result = cgltf_result_success;

    map = FileMap_Open(path, false);
    if (!FileMap_IsOpen(&map))
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

    char basePath[PIM_PATH];
    StrCpy(ARGS(basePath), path);
    char* ext = (char*)StrIStr(ARGS(basePath), ".gltf");
    if (ext)
    {
        while (ext >= basePath)
        {
            if (IsPathSeparator(*ext))
            {
                *ext = 0;
                break;
            }
            --ext;
        }
        if (ext < basePath)
        {
            basePath[0] = 0;
        }
    }

    success = CreateScenes(basePath, cgdata, dr);

cleanup:
    cgltf_free(cgdata);
    FileMap_Close(&map);
    return success;
}

// ----------------------------------------------------------------------------

static char const* const ShortenString(char const* const str, i32 maxLen)
{
    i32 len = StrLen(str);
    i32 diff = len - maxLen;
    return str + pim_max(diff, 0);
}

static i32 CatNodeLineage(char* dst, i32 size, const cgltf_node* cgnode)
{
    i32 len = 0;
    if (cgnode->parent)
    {
        CatNodeLineage(dst, size, cgnode->parent);
        len = StrCatf(dst, size, "/%s", ShortenString(cgnode->name, 16));
    }
    else
    {
        len = StrCatf(dst, size, "%s", ShortenString(cgnode->name, 16));
    }
    return len;
}

static bool CreateScenes(
    const char* basePath,
    const cgltf_data* cgdata,
    Entities* dr)
{
    for (cgltf_size iScene = 0; iScene < cgdata->scenes_count; ++iScene)
    {
        const cgltf_scene* cgscene = &cgdata->scenes[iScene];

        char selfPath[PIM_PATH];
        StrCpy(ARGS(selfPath), basePath);
        if (cgscene->name)
        {
            StrCatf(ARGS(selfPath), "/%s", cgscene->name);
        }
        else if (cgdata->scenes_count > 1)
        {
            SPrintf(ARGS(selfPath), "%s/%zu", basePath, iScene);
        }

        for (cgltf_size iNode = 0; iNode < cgscene->nodes_count; ++iNode)
        {
            CreateNode(basePath, selfPath, cgscene->nodes[iNode], dr);
        }
    }
    return true;
}

static const void* GetAccessorMemory(const cgltf_accessor* acc)
{
    const void* mem = NULL;
    if (!acc->is_sparse)
    {
        const cgltf_buffer_view* view = acc->buffer_view;
        if (view)
        {
            mem = view->data;
            if (!mem && view->buffer)
            {
                mem = view->buffer->data;
                if (mem)
                {
                    mem = (const u8*)(mem)+view->offset;
                }
            }
        }
        if (mem)
        {
            mem = (const u8*)(mem)+acc->offset;
        }
    }
    return mem;
}

static i32* ReadPrimitiveIndices(
    const cgltf_accessor* acc,
    i32* countOut)
{
    i32* indices = NULL;
    i32 count = 0;
    bool success = true;
    if (!acc)
    {
        success = false;
        goto cleanup;
    }

    count = (i32)acc->count;
    ASSERT(count >= 0);
    if (count > 0)
    {
        indices = Perm_Alloc(sizeof(indices[0]) * count);
        bool anyFail = false;
        for (i32 i = 0; i < count; ++i)
        {
            i32 index = (i32)cgltf_accessor_read_index(acc, (cgltf_size)i);
            indices[i] = index;
            anyFail |= ((u32)index >= (u32)count);
        }
        if (anyFail)
        {
            ASSERT(false);
            success = false;
            goto cleanup;
        }
    }

cleanup:
    if (!success)
    {
        Mem_Free(indices);
        indices = NULL;
        count = 0;
    }
    *countOut = count;
    return indices;
}

static float* ReadPrimitiveFloats(
    const cgltf_accessor* acc,
    i32* countOut,
    i32* floatsPerElementOut)
{
    float* values = NULL;
    i32 count = 0;
    i32 floatCount = 0;
    i32 bytes = 0;
    bool success = true;
    if (!acc)
    {
        success = false;
        goto cleanup;
    }
    floatCount = (i32)cgltf_accessor_unpack_floats(acc, NULL, 0);
    bytes = floatCount * sizeof(float);
    count = (i32)acc->count;
    ASSERT(bytes >= 0);
    if (bytes <= 0)
    {
        success = false;
        goto cleanup;
    }
    values = Perm_Calloc(bytes);
    if (!cgltf_accessor_unpack_floats(acc, values, floatCount))
    {
        success = false;
        goto cleanup;
    }

cleanup:
    if (!success)
    {
        Mem_Free(values);
        values = NULL;
        count = 0;
        floatCount = 0;
    }
    *countOut = count;
    *floatsPerElementOut = floatCount / pim_max(count, 1);
    return values;
}

typedef enum
{
    FlattenToFloat4s_PositionMode,
    FlattenToFloat4s_DirectionMode,
    FlattenToFloat4s_UvMode,
} FlattenToFloat4s_Mode;

static void FlattenToFloat4s(
    const float* __restrict srcFloats,
    i32 srcStride,
    float4* __restrict dstFloat4s,
    const i32* __restrict indices,
    i32 indCount,
    FlattenToFloat4s_Mode mode)
{
    const float wFillValue = (mode == FlattenToFloat4s_PositionMode) ? (1.0f) : (0.0f);
    if (!srcFloats)
    {
        for (i32 i = 0; i < indCount; ++i)
        {
            dstFloat4s[i] = f4_v(0.0f, 0.0f, 0.0f, wFillValue);
        }
        return;
    }
    switch (srcStride)
    {
    default:
        ASSERT(false);
        break;
    case 1:
    {
        for (i32 i = 0; i < indCount; ++i)
        {
            i32 index = indices[i];
            ASSERT((u32)index < (u32)indCount);
            const float* __restrict src = srcFloats + index;
            dstFloat4s[i] = f4_v(src[0], 0.0f, 0.0f, wFillValue);
        }
    }
    break;
    case 2:
    {
        const float2* __restrict src = (float2*)srcFloats;
        for (i32 i = 0; i < indCount; ++i)
        {
            i32 index = indices[i];
            ASSERT((u32)index < (u32)indCount);
            dstFloat4s[i] = f4_v(src[index].x, src[index].y, 0.0f, wFillValue);
        }
    }
    break;
    case 3:
    {
        const float3* __restrict src = (float3*)srcFloats;
        for (i32 i = 0; i < indCount; ++i)
        {
            i32 index = indices[i];
            ASSERT((u32)index < (u32)indCount);
            dstFloat4s[i] = f4_v(src[index].x, src[index].y, src[index].z, wFillValue);
        }
    }
    break;
    case 4:
    {
        const float4* __restrict src = (float4*)srcFloats;
        if (mode != FlattenToFloat4s_UvMode)
        {
            for (i32 i = 0; i < indCount; ++i)
            {
                i32 index = indices[i];
                ASSERT((u32)index < (u32)indCount);
                dstFloat4s[i] = f4_v(src[index].x, src[index].y, src[index].z, wFillValue);
            }
        }
        else
        {
            for (i32 i = 0; i < indCount; ++i)
            {
                i32 index = indices[i];
                ASSERT((u32)index < (u32)indCount);
                dstFloat4s[i] = src[index];
            }
        }
    }
    break;
    }
}

static bool CreateMesh(
    const char* name,
    const cgltf_primitive* cgprim,
    MeshId* meshIdOut)
{
    bool success = true;
    i32* indices = NULL;
    float* positions = NULL;
    float* normals = NULL;
    float* uvs = NULL;

    if (!cgprim)
    {
        success = false;
        goto cleanup;
    }
    if (cgprim->type != cgltf_primitive_type_triangles)
    {
        success = false;
        goto cleanup;
    }

    const Guid guid = Guid_FromStr(name);
    if (Mesh_Find(guid, meshIdOut))
    {
        Mesh_Retain(*meshIdOut);
        success = true;
        goto cleanup;
    }

    const cgltf_attribute* cgattributes = cgprim->attributes;
    const i32 attributeCount = (i32)cgprim->attributes_count;

    i32 attrInds[cgltf_attribute_type_weights + 1];
    for (i32 i = 0; i < NELEM(attrInds); ++i)
    {
        attrInds[i] = -1;
    }

    for (i32 iAttr = 0; iAttr < attributeCount; ++iAttr)
    {
        cgltf_attribute_type attrType = cgattributes[iAttr].type;
        ASSERT((u32)attrType < (u32)NELEM(attrInds));
        if ((u32)attrType < (u32)NELEM(attrInds))
        {
            ASSERT(attrInds[attrType] < 0);
            if (attrInds[attrType] < 0)
            {
                attrInds[attrType] = iAttr;
                ASSERT(cgattributes[iAttr].data);
            }
        }
    }
    ASSERT(attrInds[cgltf_attribute_type_invalid] < 0);

    const i32 iPosition = attrInds[cgltf_attribute_type_position];
    const i32 iNormal = attrInds[cgltf_attribute_type_normal];
    const i32 iTexcoord = attrInds[cgltf_attribute_type_texcoord];
    if (iPosition < 0)
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }
    if (iNormal < 0)
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }
    if (iTexcoord < 0)
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }

    i32 indCount = 0;
    indices = ReadPrimitiveIndices(cgprim->indices, &indCount);
    i32 positionCount = 0;
    i32 posStride = 0;
    positions = ReadPrimitiveFloats(cgattributes[iPosition].data, &positionCount, &posStride);
    i32 normalCount = 0;
    i32 normStride = 0;
    normals = ReadPrimitiveFloats(cgattributes[iNormal].data, &normalCount, &normStride);
    i32 uvCount = 0;
    i32 uvStride = 0;
    uvs = ReadPrimitiveFloats(cgattributes[iTexcoord].data, &uvCount, &uvStride);

    if (!positionCount)
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }

    if (!indCount)
    {
        indCount = positionCount;
        indices = Perm_Alloc(sizeof(indices[0]) * indCount);
        for (i32 i = 0; i < indCount; ++i)
        {
            indices[i] = i;
        }
    }

    if ((indCount % 3) != 0)
    {
        ASSERT(false);
        success = false;
        goto cleanup;
    }

    Mesh mesh = { 0 };
    mesh.positions = Perm_Alloc(sizeof(mesh.positions[0]) * indCount);
    mesh.normals = Perm_Alloc(sizeof(mesh.normals[0]) * indCount);
    mesh.uvs = Perm_Alloc(sizeof(mesh.uvs[0]) * indCount);
    mesh.texIndices = Perm_Calloc(sizeof(mesh.texIndices[0]) * indCount);
    mesh.length = indCount;

    // TODO: wasteful to flatten indexed mesh
    FlattenToFloat4s(
        positions,
        posStride,
        mesh.positions,
        indices,
        indCount,
        FlattenToFloat4s_PositionMode);
    FlattenToFloat4s(
        normals,
        normStride,
        mesh.normals,
        indices,
        indCount,
        FlattenToFloat4s_DirectionMode);
    FlattenToFloat4s(
        uvs,
        uvStride,
        mesh.uvs,
        indices,
        indCount,
        FlattenToFloat4s_UvMode);

    Mesh_New(&mesh, guid, meshIdOut);

cleanup:
    Mem_Free(indices);
    Mem_Free(positions);
    Mem_Free(normals);
    Mem_Free(uvs);

    return success;
}

static bool CreateNode(
    const char* basePath,
    const char* parentPath,
    const cgltf_node* cgnode,
    Entities* dr)
{
    if (!cgnode)
    {
        return false;
    }
    if (!cgnode->name)
    {
        ASSERT(false);
        return false;
    }

    char selfPath[PIM_PATH];
    SPrintf(ARGS(selfPath), "%s/%s", parentPath, cgnode->name);

    const cgltf_mesh* cgmesh = cgnode->mesh;
    if (cgmesh)
    {
        float4x4 localToWorld = f4x4_id;
        cgltf_node_transform_world(cgnode, &localToWorld.c0.x);

        for (cgltf_size iPrim = 0; iPrim < cgmesh->primitives_count; ++iPrim)
        {
            const cgltf_primitive* cgprim = &cgmesh->primitives[iPrim];

            char primPath[PIM_PATH];
            SPrintf(ARGS(primPath), "%s/%s_%d", selfPath, cgmesh->name, iPrim);

            Guid guid = Guid_FromStr(primPath);
            if (Entities_Find(dr, guid) >= 0)
            {
                ASSERT(false);
                continue;
            }

            MeshId meshId = { 0 };
            if (!CreateMesh(primPath, cgprim, &meshId))
            {
                continue;
            }

            i32 iDrawable = Entities_Add(dr, guid);

            dr->meshes[iDrawable] = meshId;

            // TODO: probably wrong, will need to debug.
            dr->translations[iDrawable] = f4x4_derive_translation(localToWorld);
            dr->rotations[iDrawable] = f4x4_derive_rotation(localToWorld);
            dr->scales[iDrawable] = f4x4_derive_scale(localToWorld);
            // Entities_UpdateTransforms will overwrite this with value from f4x4_trs().
            // maybe add an entity flag to skip Entities_UpdateTransforms, for debugging and perf sake.
            dr->matrices[iDrawable] = localToWorld;

            Material mat = { 0 };
            CreateMaterial(basePath, selfPath, cgprim->material, &mat);
            dr->materials[iDrawable] = mat;

        }
    }

    for (cgltf_size iChild = 0; iChild < cgnode->children_count; ++iChild)
    {
        CreateNode(basePath, selfPath, cgnode->children[iChild], dr);
    }

    return true;
}

// ----------------------------------------------------------------------------

static bool CreateMaterial(
    const char* basePath,
    const char* parentPath,
    const cgltf_material* cgmat,
    Material* matOut)
{
    if (!cgmat)
    {
        return false;
    }
    char const* const name = cgmat->name;
    if (!name)
    {
        ASSERT(false);
        return false;
    }

    // TODO: Cache imported materials for reuse

    Material material = { 0 };
    material.ior = 1.0f;
    material.bumpiness = 1.0f;
    if (cgmat->has_ior)
    {
        material.ior = cgmat->ior.ior;
    }

    if (cgmat->normal_texture.texture)
    {
        const cgltf_image* cgnormal = cgmat->normal_texture.texture->image;
        material.normal = CreateNormalTexture(basePath, cgnormal);
    }

    const cgltf_image* cgocclusion = NULL;
    if (cgmat->occlusion_texture.texture)
    {
        cgocclusion = cgmat->occlusion_texture.texture->image;
    }

    // TODO: add dedicated emission texture to Material
    const cgltf_image* cgemission = NULL;
    if (cgmat->emissive_texture.texture)
    {
        cgemission = cgmat->emissive_texture.texture->image;
    }

    if (cgmat->has_pbr_metallic_roughness)
    {
        const cgltf_image* cgalbedo = NULL;
        const cgltf_image* cgmetallicroughness = NULL;
        if (cgmat->pbr_metallic_roughness.base_color_texture.texture)
        {
            cgalbedo = cgmat->pbr_metallic_roughness.base_color_texture.texture->image;
        }
        if (cgmat->pbr_metallic_roughness.metallic_roughness_texture.texture)
        {
            cgmetallicroughness = cgmat->pbr_metallic_roughness.metallic_roughness_texture.texture->image;
        }
        material.albedo = CreateAlbedoTexture(basePath, cgalbedo);
        material.rome = CreateRomeTexture(basePath, cgmetallicroughness, cgocclusion, cgemission);
    }
    else if (cgmat->has_pbr_specular_glossiness)
    {
        const cgltf_image* cgdiffuse = NULL;
        const cgltf_image* cgspecular = NULL;
        float glossiness = cgmat->pbr_specular_glossiness.glossiness_factor;
        if (cgmat->pbr_specular_glossiness.diffuse_texture.texture)
        {
            cgdiffuse = cgmat->pbr_specular_glossiness.diffuse_texture.texture->image;
        }
        if (cgmat->pbr_specular_glossiness.specular_glossiness_texture.texture)
        {
            cgspecular = cgmat->pbr_specular_glossiness.specular_glossiness_texture.texture->image;
        }
        ResampleToAlbedoRome(basePath, &material, cgdiffuse, cgspecular, cgocclusion, cgemission, glossiness);
    }

    if (cgemission)
    {
        material.flags |= MatFlag_Emissive;
    }
    if ((material.ior != 1.0f) || cgmat->has_transmission)
    {
        material.flags |= MatFlag_Refractive;
    }

    *matOut = material;

    return true;
}

static bool LoadImageUri(
    const char* basePath,
    const char* uri,
    Texture* tex)
{
    memset(tex, 0, sizeof(*tex));

    char name[PIM_PATH] = { 0 };
    StrCpy(ARGS(name), uri);
    cgltf_decode_uri(name);
    char path[PIM_PATH] = { 0 };
    SPrintf(ARGS(path), "%s/%s", basePath, name);
    StrPath(ARGS(path));

    if (stbi_is_hdr(path))
    {
        i32 channels = 0;
        tex->format = VK_FORMAT_R32G32B32A32_SFLOAT;
        tex->texels = stbi_loadf(path, &tex->size.x, &tex->size.y, &channels, 4);
    }
    else if (stbi_is_16_bit(path))
    {
        i32 channels = 0;
        tex->format = VK_FORMAT_R16G16B16A16_UNORM;
        tex->texels = stbi_load_16(path, &tex->size.x, &tex->size.y, &channels, 4);
    }
    else
    {
        i32 channels = 0;
        tex->format = VK_FORMAT_R8G8B8A8_SRGB;
        tex->texels = stbi_load(path, &tex->size.x, &tex->size.y, &channels, 4);
    }

    if (tex->texels)
    {
        return true;
    }
    else
    {
        const char* reason = stbi_failure_reason();
        if (reason)
        {
            Con_Logf(LogSev_Error, "gltf", "Failed to load image at '%s' due to '%s'.", path, reason);
        }
        memset(tex, 0, sizeof(*tex));
        return false;
    }
}

// This is inefficient, but it reduces the number of format conversions to handle to N instead of N^2
static bool ResampleTextureToFloat4(Texture* tex)
{
    ASSERT(tex->texels);
    if (!tex->texels)
    {
        return false;
    }
    switch (tex->format)
    {
    default:
        ASSERT(false);
        return false;
    case VK_FORMAT_R8G8B8A8_SRGB:
    {
        const i32 len = tex->size.x * tex->size.y;
        const R8G8B8A8_t* pim_noalias src = tex->texels;
        float4* pim_noalias dst = Tex_Alloc(sizeof(dst[0]) * len);
        for (i32 i = 0; i < len; ++i)
        {
            dst[i] = GammaDecode_rgba8(src[i]);
        }
        Mem_Free(tex->texels);
        tex->texels = dst;
        tex->format = VK_FORMAT_R32G32B32A32_SFLOAT;
    }
    return true;
    case VK_FORMAT_R8G8B8A8_UNORM:
    {
        const i32 len = tex->size.x * tex->size.y;
        const R8G8B8A8_t* pim_noalias src = tex->texels;
        float4* pim_noalias dst = Tex_Alloc(sizeof(dst[0]) * len);
        for (i32 i = 0; i < len; ++i)
        {
            dst[i] = rgba8_f4(src[i]);
        }
        Mem_Free(tex->texels);
        tex->texels = dst;
        tex->format = VK_FORMAT_R32G32B32A32_SFLOAT;
    }
    return true;
    case VK_FORMAT_R16G16B16A16_UNORM:
    {
        const i32 len = tex->size.x * tex->size.y;
        const R16G16B16A16_t* pim_noalias src = tex->texels;
        float4* pim_noalias dst = Tex_Alloc(sizeof(dst[0]) * len);
        for (i32 i = 0; i < len; ++i)
        {
            dst[i] = rgba16_f4(src[i]);
        }
        Mem_Free(tex->texels);
        tex->texels = dst;
        tex->format = VK_FORMAT_R32G32B32A32_SFLOAT;
    }
    return true;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return true;
    }
}

static bool ResampleToSrgb(Texture* tex)
{
    ASSERT(tex->texels);
    if (!tex->texels)
    {
        return false;
    }
    if (tex->format == VK_FORMAT_R8G8B8A8_SRGB)
    {
        return true;
    }
    if (ResampleTextureToFloat4(tex))
    {
        const i32 len = tex->size.x * tex->size.y;
        const float4* pim_noalias src = tex->texels;
        R8G8B8A8_t* pim_noalias dst = Tex_Alloc(sizeof(dst[0]) * len);
        for (i32 i = 0; i < len; ++i)
        {
            dst[i] = GammaEncode_rgba8(src[i]);
        }
        Mem_Free(tex->texels);
        tex->texels = dst;
        tex->format = VK_FORMAT_R8G8B8A8_SRGB;
        return true;
    }
    return false;
}

static bool ResampleToEmission(Texture* tex)
{
    ASSERT(tex->texels);
    if (!tex->texels)
    {
        return false;
    }
    if (tex->format == VK_FORMAT_R32G32B32A32_SFLOAT)
    {
        return true;
    }
    if (ResampleTextureToFloat4(tex))
    {
        const i32 len = tex->size.x * tex->size.y;
        float4* pim_noalias texels = tex->texels;
        for (i32 i = 0; i < len; ++i)
        {
            float4 texel = texels[i];
            texel = f4_mul(texel, texel);
            texel = f4_mulvs(texel, kEmissionScale);
            texels[i] = texel;
        }
        return true;
    }
    return false;
}

static bool ImportColorspace(Texture* tex)
{
    ASSERT(tex->texels);
    ASSERT(tex->format == VK_FORMAT_R32G32B32A32_SFLOAT);
    if (tex->texels)
    {
        if (tex->format == VK_FORMAT_R32G32B32A32_SFLOAT)
        {
            float4* pim_noalias texels = tex->texels;
            const i32 len = tex->size.x * tex->size.y;
            for (i32 i = 0; i < len; ++i)
            {
                texels[i] = Color_SDRToScene(texels[i]);
            }
        }
    }
    return false;
}

static bool ExportColorspace(Texture* tex)
{
    ASSERT(tex->texels);
    ASSERT(tex->format == VK_FORMAT_R32G32B32A32_SFLOAT);
    if (tex->texels)
    {
        if (tex->format == VK_FORMAT_R32G32B32A32_SFLOAT)
        {
            float4* pim_noalias texels = tex->texels;
            const i32 len = tex->size.x * tex->size.y;
            for (i32 i = 0; i < len; ++i)
            {
                texels[i] = Color_SceneToSDR(texels[i]);
            }
            return true;
        }
    }
    return false;
}

static TextureId CreateAlbedoTexture(
    const char* basePath,
    const cgltf_image* cgalbedo)
{
    TextureId id = { 0 };
    if (!cgalbedo)
    {
        return id;
    }

    char albedoName[PIM_PATH] = { 0 };
    StrCpy(ARGS(albedoName), cgalbedo->uri);
    cgltf_decode_uri(albedoName);

    char fullName[PIM_PATH] = { 0 };
    char const* const names[] = { basePath, &albedoName[0] };
    SPrintf(ARGS(fullName), "%s/%s", basePath, albedoName);
    StrPath(ARGS(fullName));
    Guid guid = Guid_FromStr(fullName);

    if (Texture_Find(guid, &id))
    {
        Texture_Retain(id);
        return id;
    }

    Texture tex = { 0 };
    if (!LoadImageUri(basePath, cgalbedo->uri, &tex))
    {
        return id;
    }
    if (!ResampleTextureToFloat4(&tex))
    {
        Mem_Free(tex.texels);
        return id;
    }
    ImportColorspace(&tex);
    if (!ResampleToSrgb(&tex))
    {
        Mem_Free(tex.texels);
        return id;
    }
    // sRGB OETF encoded, but in scene colorspace
    ASSERT(tex.format == VK_FORMAT_R8G8B8A8_SRGB);
    Texture_New(&tex, VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLER_ADDRESS_MODE_REPEAT, guid, &id);
    return id;
}

static TextureId CreateRomeTexture(
    const char* basePath,
    const cgltf_image* cgmetallic_roughness,
    const cgltf_image* cgocclusion,
    const cgltf_image* cgemission)
{
    char mrName[PIM_PATH] = { 0 };
    if (cgmetallic_roughness && cgmetallic_roughness->uri)
    {
        StrCpy(ARGS(mrName), cgmetallic_roughness->uri);
        cgltf_decode_uri(mrName);
    }

    char occName[PIM_PATH] = { 0 };
    if (cgocclusion && cgocclusion->uri)
    {
        StrCpy(ARGS(occName), cgocclusion->uri);
        cgltf_decode_uri(occName);
    }

    char emName[PIM_PATH] = { 0 };
    if (cgemission && cgemission->uri)
    {
        StrCpy(ARGS(emName), cgemission->uri);
        cgltf_decode_uri(emName);
    }

    char fullName[PIM_PATH] = { 0 };
    StrCpy(ARGS(fullName), basePath);
    if (mrName[0])
    {
        StrCatf(ARGS(fullName), "/%s", ShortenString(mrName, 64));
    }
    if (occName[0])
    {
        StrCatf(ARGS(fullName), "/%s", ShortenString(occName, 64));
    }
    if (emName[0])
    {
        StrCatf(ARGS(fullName), "/%s", ShortenString(emName, 64));
    }

    Guid guid = Guid_FromStr(fullName);

    TextureId id = { 0 };
    if (Texture_Find(guid, &id))
    {
        Texture_Retain(id);
        return id;
    }

    Texture mrtex = { 0 };
    Texture occtex = { 0 };
    Texture emtex = { 0 };
    float4 defaultMr = { 0.0f, 0.5f, 0.0f, 0.0f };
    float4 defaultOcc = { 1.0f, 0.0f, 0.0f, 0.0f };
    float4 defaultEm = { 0.0f, 0.0f, 0.0f, 0.0f };

    if (cgmetallic_roughness)
    {
        if (LoadImageUri(basePath, cgmetallic_roughness->uri, &mrtex))
        {
            if (mrtex.format == VK_FORMAT_R8G8B8A8_SRGB)
            {
                mrtex.format = VK_FORMAT_R8G8B8A8_UNORM;
            }
            ResampleTextureToFloat4(&mrtex);
        }
    }
    if (!mrtex.texels)
    {
        mrtex.size = i2_1;
        mrtex.texels = &defaultMr;
        mrtex.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    }

    if (cgocclusion)
    {
        if (LoadImageUri(basePath, cgocclusion->uri, &occtex))
        {
            if (mrtex.format == VK_FORMAT_R8G8B8A8_SRGB)
            {
                mrtex.format = VK_FORMAT_R8G8B8A8_UNORM;
            }
            ResampleTextureToFloat4(&occtex);
        }
    }
    if (!occtex.texels)
    {
        occtex.size = i2_1;
        occtex.texels = &defaultOcc;
        mrtex.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    }

    if (cgemission)
    {
        if (LoadImageUri(basePath, cgemission->uri, &emtex))
        {
            if (ResampleToEmission(&emtex))
            {
                ImportColorspace(&emtex);
            }
        }
    }
    if (!emtex.texels)
    {
        emtex.size = i2_1;
        emtex.texels = &defaultEm;
        mrtex.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    }

    Texture tex = { 0 };
    {
        i32 size = 1;
        size = pim_max(size, mrtex.size.x);
        size = pim_max(size, mrtex.size.y);
        size = pim_max(size, occtex.size.x);
        size = pim_max(size, occtex.size.y);
        size = pim_max(size, emtex.size.x);
        size = pim_max(size, emtex.size.y);
        size = NextPow2(size);
        size = pim_min(size, 2048);

        const i32 len = size * size;
        const float rcpSize = 1.0f / (size - 1);
        R8G8B8A8_t* pim_noalias dst = Tex_Alloc(sizeof(dst[0]) * len);
        for (i32 y = 0; y < size; ++y)
        {
            for (i32 x = 0; x < size; ++x)
            {
                float2 uv = { x * rcpSize, y * rcpSize };
                float4 rome = f4_0;
                {
                    float4 mr = UvBilinearClamp_f4(mrtex.texels, mrtex.size, uv);
                    rome.x = mr.y;
                    rome.z = mr.x;
                }
                rome.y = UvBilinearClamp_f4(occtex.texels, occtex.size, uv).x;
                rome.w = PackEmission(UvBilinearClamp_f4(emtex.texels, emtex.size, uv));
                i32 index = x + y * size;
                dst[index] = f4_rgba8(rome);
            }
        }
        tex.size = i2_s(size);
        tex.texels = dst;
        tex.format = VK_FORMAT_R8G8B8A8_UNORM;
    }

    if (mrtex.texels != &defaultMr)
    {
        Mem_Free(mrtex.texels);
    }
    if (occtex.texels != &defaultOcc)
    {
        Mem_Free(occtex.texels);
    }
    if (emtex.texels != &defaultEm)
    {
        Mem_Free(emtex.texels);
    }

    Texture_New(&tex, VK_FORMAT_R8G8B8A8_SRGB, VK_SAMPLER_ADDRESS_MODE_REPEAT, guid, &id);
    return id;
}

static TextureId CreateNormalTexture(
    const char* basePath,
    const cgltf_image* cgnormal)
{
    TextureId id = { 0 };
    if (!cgnormal)
    {
        return id;
    }

    char normalName[PIM_PATH] = { 0 };
    StrCpy(ARGS(normalName), cgnormal->uri);
    cgltf_decode_uri(normalName);

    char fullName[PIM_PATH] = { 0 };
    SPrintf(ARGS(fullName), "%s/%s", basePath, normalName);
    StrPath(ARGS(fullName));
    Guid guid = Guid_FromStr(fullName);

    if (Texture_Find(guid, &id))
    {
        Texture_Retain(id);
        return id;
    }

    Texture tex = { 0 };
    if (!LoadImageUri(basePath, cgnormal->uri, &tex))
    {
        return id;
    }
    if (tex.format == VK_FORMAT_R8G8B8A8_SRGB)
    {
        tex.format = VK_FORMAT_R8G8B8A8_UNORM;
    }
    if (!ResampleTextureToFloat4(&tex))
    {
        Mem_Free(tex.texels);
        return id;
    }
    {
        ASSERT(tex.texels);
        i32 size = pim_max(tex.size.x, tex.size.y);
        ASSERT(size > 0);
        size = NextPow2(size);
        size = pim_min(size, 2048);
        const float rcpSize = 1.0f / (size - 1);
        const float4* pim_noalias src = tex.texels;
        const i32 len = size * size;
        short2* pim_noalias dst = Tex_Alloc(sizeof(dst[0]) * len);
        if (size == tex.size.x && size == tex.size.y)
        {
            for (i32 i = 0; i < len; ++i)
            {
                dst[i] = NormalTsToXy16(f4_snorm(src[i]));
            }
        }
        else
        {
            for (i32 y = 0; y < size; ++y)
            {
                for (i32 x = 0; x < size; ++x)
                {
                    float2 uv = { x * rcpSize, y * rcpSize };
                    float4 sample = UvBilinearClamp_f4(src, tex.size, uv);
                    i32 index = x + y * size;
                    dst[index] = NormalTsToXy16(f4_snorm(sample));
                }
            }
        }
        Mem_Free(tex.texels);
        tex.texels = dst;
        tex.size = i2_s(size);
        tex.format = VK_FORMAT_R16G16_SNORM;
    }
    Texture_New(&tex, tex.format, VK_SAMPLER_ADDRESS_MODE_REPEAT, guid, &id);
    return id;
}

bool ResampleToAlbedoRome(
    const char* basePath,
    Material* material,
    const cgltf_image* cgdiffuse,
    const cgltf_image* cgspecular,
    const cgltf_image* cgocclusion,
    const cgltf_image* cgemission,
    float glossiness)
{
    char diffuseName[PIM_PATH] = { 0 };
    if (cgdiffuse && cgdiffuse->uri)
    {
        StrCpy(ARGS(diffuseName), cgdiffuse->uri);
        cgltf_decode_uri(diffuseName);
    }
    char specularName[PIM_PATH] = { 0 };
    if (cgspecular && cgspecular->uri)
    {
        StrCpy(ARGS(specularName), cgspecular->uri);
        cgltf_decode_uri(specularName);
    }
    char occlusionName[PIM_PATH] = { 0 };
    if (cgocclusion && cgocclusion->uri)
    {
        StrCpy(ARGS(occlusionName), cgocclusion->uri);
        cgltf_decode_uri(occlusionName);
    }
    char emissionName[PIM_PATH] = { 0 };
    if (cgemission && cgemission->uri)
    {
        StrCpy(ARGS(emissionName), cgemission->uri);
        cgltf_decode_uri(emissionName);
    }

    Guid albedoGuid = { 0 };
    {
        char albedoName[PIM_PATH] = { 0 };
        StrCpy(ARGS(albedoName), ShortenString(basePath, 120));
        if (diffuseName[0])
        {
            StrCatf(ARGS(albedoName), "/%s", ShortenString(diffuseName, 64));
        }
        if (specularName[0])
        {
            StrCatf(ARGS(albedoName), "/%s", ShortenString(specularName, 64));
        }
        StrPath(ARGS(albedoName));
        albedoGuid = Guid_FromStr(albedoName);
    }

    Guid romeGuid = { 0 };
    {
        char romeName[PIM_PATH] = { 0 };
        StrCpy(ARGS(romeName), ShortenString(basePath, 120));
        if (diffuseName[0])
        {
            StrCatf(ARGS(romeName), "/%s", ShortenString(diffuseName, 32));
        }
        if (specularName[0])
        {
            StrCatf(ARGS(romeName), "/%s", ShortenString(specularName, 32));
        }
        if (occlusionName[0])
        {
            StrCatf(ARGS(romeName), "/%s", ShortenString(occlusionName, 32));
        }
        if (emissionName[0])
        {
            StrCatf(ARGS(romeName), "/%s", ShortenString(emissionName, 32));
        }
        StrPath(ARGS(romeName));
        romeGuid = Guid_FromStr(romeName);
    }

    if (Texture_Find(albedoGuid, &material->albedo) && Texture_Find(romeGuid, &material->rome))
    {
        Texture_Retain(material->albedo);
        Texture_Retain(material->rome);
        return true;
    }

    Texture diffuseTex = { 0 };
    Texture specularTex = { 0 };
    Texture occtex = { 0 };
    Texture emtex = { 0 };
    float4 defaultDiffuse = { 0.5f, 0.5f, 0.5f, 1.0f };
    float4 defaultSpecular = { 0.0f, 0.0f, 0.0f, 0.0f };
    float4 defaultOcclusion = { 1.0f, 0.0f, 0.0f, 0.0f };
    float4 defaultEmission = { 0.0f, 0.0f, 0.0f, 0.0f };

    if (cgdiffuse)
    {
        if (LoadImageUri(basePath, cgdiffuse->uri, &diffuseTex))
        {
            if (ResampleTextureToFloat4(&diffuseTex))
            {
                ImportColorspace(&diffuseTex);
            }
        }
    }
    if (!diffuseTex.texels)
    {
        diffuseTex.size = i2_1;
        diffuseTex.texels = &defaultDiffuse;
        diffuseTex.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    }

    if (cgspecular)
    {
        if (LoadImageUri(basePath, cgspecular->uri, &specularTex))
        {
            if (ResampleTextureToFloat4(&specularTex))
            {
                ImportColorspace(&specularTex);
            }
        }
    }
    if (!specularTex.texels)
    {
        specularTex.size = i2_1;
        specularTex.texels = &defaultSpecular;
        specularTex.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    }

    if (cgocclusion)
    {
        if (LoadImageUri(basePath, cgocclusion->uri, &occtex))
        {
            if (occtex.format == VK_FORMAT_R8G8B8A8_SRGB)
            {
                occtex.format = VK_FORMAT_R8G8B8A8_UNORM;
            }
            ResampleTextureToFloat4(&occtex);
        }
    }
    if (!occtex.texels)
    {
        occtex.size = i2_1;
        occtex.texels = &defaultOcclusion;
        occtex.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    }

    if (cgemission)
    {
        if (LoadImageUri(basePath, cgemission->uri, &emtex))
        {
            if (ResampleToEmission(&emtex))
            {
                ImportColorspace(&emtex);
            }
        }
    }
    if (!emtex.texels)
    {
        emtex.size = i2_1;
        emtex.texels = &defaultEmission;
        emtex.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    }

    Texture albedotex = { 0 };
    Texture rometex = { 0 };
    {
        int2 size = i2_1;
        size = i2_max(size, diffuseTex.size);
        size = i2_max(size, specularTex.size);
        size = i2_max(size, occtex.size);
        size = i2_max(size, emtex.size);
        const i32 len = size.x * size.y;
        R8G8B8A8_t* pim_noalias albedoTexels = Tex_Alloc(sizeof(albedoTexels[0]) * len);
        R8G8B8A8_t* pim_noalias romeTexels = Tex_Alloc(sizeof(romeTexels[0]) * len);
        float deltaU = 1.0f / (size.x - 1);
        float deltaV = 1.0f / (size.y - 1);
        for (i32 y = 0; y < size.y; ++y)
        {
            for (i32 x = 0; x < size.x; ++x)
            {
                float2 uv = { x * deltaU, y * deltaV };
                float4 diffuse = UvBilinearClamp_f4(diffuseTex.texels, diffuseTex.size, uv);
                float4 specular = UvBilinearClamp_f4(specularTex.texels, specularTex.size, uv);
                float occlusion = UvBilinearClamp_f4(occtex.texels, occtex.size, uv).x;
                float emission = PackEmission(UvBilinearClamp_f4(emtex.texels, emtex.size, uv));
                float4 albedo;
                float roughness;
                float metallic;
                SpecularToMetallic(diffuse, specular, glossiness, &albedo, &roughness, &metallic);
                i32 index = x + y * size.x;
                albedoTexels[index] = GammaEncode_rgba8(albedo);
                romeTexels[index] = GammaEncode_rgba8(f4_v(roughness, occlusion, metallic, emission));
            }
        }
        albedotex.texels = albedoTexels;
        albedotex.size = size;
        albedotex.format = VK_FORMAT_R8G8B8A8_SRGB;
        rometex.texels = romeTexels;
        rometex.size = size;
        rometex.format = VK_FORMAT_R8G8B8A8_UNORM;
    }

    if (diffuseTex.texels != &defaultDiffuse)
    {
        Mem_Free(diffuseTex.texels);
    }
    if (specularTex.texels != &defaultSpecular)
    {
        Mem_Free(specularTex.texels);
    }
    if (occtex.texels != &defaultOcclusion)
    {
        Mem_Free(occtex.texels);
    }
    if (emtex.texels != &defaultEmission)
    {
        Mem_Free(emtex.texels);
    }

    if (!Texture_Exists(material->albedo))
    {
        Texture_New(&albedotex, albedotex.format, VK_SAMPLER_ADDRESS_MODE_REPEAT, albedoGuid, &material->albedo);
    }
    else
    {
        Texture_Retain(material->albedo);
        Mem_Free(albedotex.texels);
    }
    if (!Texture_Exists(material->rome))
    {
        Texture_New(&rometex, rometex.format, VK_SAMPLER_ADDRESS_MODE_REPEAT, romeGuid, &material->rome);
    }
    else
    {
        Texture_Retain(material->rome);
        Mem_Free(rometex.texels);
    }

    return true;
}

// ----------------------------------------------------------------------------

static cgltf_result Gltf_FileRead(
    const cgltf_memory_options* memory_options,
    const cgltf_file_options* file_options,
    const char* path,
    cgltf_size* size,
    void** data)
{
    *size = 0;
    *data = NULL;
    ASSERT(path && path[0]);
    if (!path || !path[0])
    {
        return cgltf_result_file_not_found;
    }

    const Guid name = Guid_HashStr(path);

    i32 fileCount = s_fileHandler.fileCount;
    Guid* pim_noalias names = s_fileHandler.names;
    FileMap* pim_noalias files = s_fileHandler.files;
    i32* pim_noalias refCounts = s_fileHandler.refCounts;

    for (i32 i = fileCount - 1; i >= 0; --i)
    {
        if (Guid_Equal(name, names[i]))
        {
            refCounts[i]++;
            ASSERT(refCounts[i] > 1);
            ASSERT(files[i].ptr);
            ASSERT(files[i].size > 0);
            *data = files[i].ptr;
            *size = files[i].size;
            return cgltf_result_success;
        }
    }

    FileMap file = FileMap_Open(path, false);
    if (!FileMap_IsOpen(&file))
    {
        return cgltf_result_file_not_found;
    }

    ASSERT(file.ptr);
    ASSERT(file.size > 0);

    i32 b = fileCount++;
    Perm_Reserve(names, fileCount);
    Perm_Reserve(files, fileCount);
    Perm_Reserve(refCounts, fileCount);

    names[b] = name;
    files[b] = file;
    refCounts[b] = 1;

    s_fileHandler.names = names;
    s_fileHandler.files = files;
    s_fileHandler.refCounts = refCounts;
    s_fileHandler.fileCount = fileCount;

    *data = file.ptr;
    *size = file.size;
    return cgltf_result_success;
}
static void Gltf_FileRelease(
    const cgltf_memory_options* memory_options,
    const cgltf_file_options* file_options,
    void* data)
{
    if (!data)
    {
        return;
    }
    i32 fileCount = s_fileHandler.fileCount;
    Guid* pim_noalias names = s_fileHandler.names;
    FileMap* pim_noalias files = s_fileHandler.files;
    i32* pim_noalias refCounts = s_fileHandler.refCounts;

    i32 slot = -1;
    for (i32 i = fileCount - 1; i >= 0; --i)
    {
        if (files[i].ptr == data)
        {
            slot = i;
            break;
        }
    }

    ASSERT(slot >= 0);
    if (slot >= 0)
    {
        refCounts[slot] = refCounts[slot] - 1;
        ASSERT(refCounts[slot] >= 0);
        if (refCounts[slot] == 0)
        {
            FileMap_Close(&files[slot]);
            --fileCount;
            names[slot] = names[fileCount]; names[fileCount] = (Guid){ 0 };
            files[slot] = files[fileCount]; files[fileCount] = (FileMap){ 0 };
            refCounts[slot] = refCounts[fileCount]; refCounts[fileCount] = 0;
            s_fileHandler.fileCount = fileCount;
        }
    }
}
