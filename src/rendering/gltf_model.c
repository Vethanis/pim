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
    cgltf_data* cgdata,
    Entities* dr);
static bool CreateScene(
    const char* basePath,
    cgltf_scene* cgscene,
    Entities* dr);
static i32 CreateNode(
    const char* basePath,
    cgltf_scene* cgscene,
    cgltf_node* cgnode,
    Entities* dr);
static bool CreateMesh(
    const char* basePath,
    cgltf_mesh* cgmesh,
    Entities* dr,
    i32 iDrawable);
static bool CreateMaterial(
    const char* basePath,
    cgltf_material* cgmat,
    Entities* dr,
    i32 iDrawable);
static TextureId CreateAlbedoTexture(
    const char* basePath,
    cgltf_image* cgalbedo);
static TextureId CreateRomeTexture(
    const char* basePath,
    cgltf_image* cgmetallic_roughness,
    cgltf_image* cgocclusion,
    cgltf_image* cgemission);
static TextureId CreateNormalTexture(
    const char* basePath,
    cgltf_image* cgnormal);
bool ResampleToAlbedoRome(
    const char* basePath,
    Material* material,
    cgltf_image* cgdiffuse,
    cgltf_image* cgspecular,
    cgltf_image* cgocclusion,
    cgltf_image* cgemission,
    float glossiness);
static bool CreateLight(
    const char* basePath,
    cgltf_light* cglight,
    Entities* dr,
    i32 iDrawable);

// ----------------------------------------------------------------------------

static void CatNodeLineage(char* dst, i32 size, cgltf_node* cgnode);
static char const *const ShortenString(char const *const str, i32 maxLen);
static bool LoadImageUri(
    const char* basePath,
    const char* uri,
    Texture* tex);
static bool ResampleToFloat4(Texture* tex);
static bool ResampleToSrgb(Texture* tex);
static bool ResampleToEmission(Texture* tex);
static bool ImportColorspace(Texture* tex);
static bool ExportColorspace(Texture* tex);

// ----------------------------------------------------------------------------

bool Gltf_Load(const char* path, Entities* dr)
{
    bool success = true;
    FileMap map = { 0 };
    cgltf_data* cgdata = NULL;
    cgltf_options options = { 0 };
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

    success = CreateScenes(path, cgdata, dr);

cleanup:
    cgltf_free(cgdata);
    FileMap_Close(&map);
    return success;
}

// ----------------------------------------------------------------------------

static char const *const ShortenString(char const *const str, i32 maxLen)
{
    i32 i = 0;
    i32 len = StrLen(str);
    while (len > maxLen)
    {
        len = len >> 1;
        i += len;
    }
    return str + i;
}

static void ConcatNames(
    char *const dst, i32 size,
    char const *const *const names, i32 nameCount)
{
    const i32 maxLen = size / (1 + nameCount);
    for (i32 i = 0; i < nameCount; ++i)
    {
        char const *const name = names[i];
        if (name)
        {
            StrCat(dst, size, ShortenString(name, maxLen));
        }
        else
        {
            StrCat(dst, size, "0");
        }
        if ((i + 1) < nameCount)
        {
            StrCat(dst, size, ":");
        }
    }
}

static void CatNodeLineage(char* dst, i32 size, cgltf_node* cgnode)
{
    if (cgnode)
    {
        CatNodeLineage(dst, size, cgnode->parent);
        const char* name = ShortenString(cgnode->name, 16);
        StrCat(dst, size, name);
    }
}

static bool CreateScenes(
    const char* basePath,
    cgltf_data* cgdata,
    Entities* dr)
{
    const i32 sceneCount = (i32)cgdata->scenes_count;
    cgltf_scene* cgscenes = cgdata->scenes;
    for (i32 i = 0; i < sceneCount; ++i)
    {
        CreateScene(basePath, &cgscenes[i], dr);
    }
    return true;
}

static bool CreateScene(
    const char* basePath,
    cgltf_scene* cgscene,
    Entities* dr)
{
    const i32 nodeCount = (i32)cgscene->nodes_count;
    cgltf_node** cgnodes = cgscene->nodes;
    for (i32 i = 0; i < nodeCount; ++i)
    {
        CreateNode(basePath, cgscene, cgnodes[i], dr);
    }
    return true;
}

static i32 CreateNode(
    const char* basePath,
    cgltf_scene* cgscene,
    cgltf_node* cgnode,
    Entities* dr)
{
    if (!cgnode)
    {
        return -1;
    }
    const char* name = cgnode->name;
    if (!name || !name[0])
    {
        ASSERT(false);
        return -1;
    }

    char fullName[PIM_PATH] = { 0 };
    StrCpy(ARGS(fullName), cgscene->name);
    CatNodeLineage(ARGS(fullName), cgnode);
    Guid guid = Guid_FromStr(fullName);

    i32 iDrawable = Entities_Find(dr, guid);
    if (iDrawable >= 0)
    {
        return iDrawable;
    }
    iDrawable = Entities_Add(dr, guid);

    float4x4 localToWorld = f4x4_id;
    cgltf_node_transform_world(cgnode, &localToWorld.c0.x);
    dr->matrices[iDrawable] = localToWorld;

    CreateLight(basePath, cgnode->light, dr, iDrawable);
    CreateMesh(basePath, cgnode->mesh, dr, iDrawable);

    const i32 childCount = (i32)cgnode->children_count;
    cgltf_node** children = cgnode->children;
    for (i32 iChild = 0; iChild < childCount; ++iChild)
    {
        CreateNode(basePath, cgscene, children[iChild], dr);
    }

    return iDrawable;
}

static bool CreateMesh(
    const char* basePath,
    cgltf_mesh* cgmesh,
    Entities* dr,
    i32 iDrawable)
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
    char fullName[PIM_PATH] = { 0 };
    char const *const names[] = { basePath, name };
    ConcatNames(ARGS(fullName), ARGS(names));
    Guid guid = Guid_FromStr(fullName);

    MeshId id = { 0 };
    if (Mesh_Find(guid, &id))
    {
        Mesh_Retain(id);
        return true;
    }

    const i32 primCount = (i32)cgmesh->primitives_count;
    cgltf_primitive* cgprims = cgmesh->primitives;
    for (i32 iPrim = 0; iPrim < primCount; ++iPrim)
    {
        cgltf_primitive* cgprim = &cgprims[iPrim];
        CreateMaterial(basePath, cgprim->material, dr, iDrawable);
    }

    return true;
}

static bool CreateMaterial(
    const char* basePath,
    cgltf_material* cgmat,
    Entities* dr,
    i32 iDrawable)
{
    if (!cgmat)
    {
        return false;
    }
    char const *const name = cgmat->name;
    if (!name || !name[0])
    {
        ASSERT(false);
        return false;
    }
    char const *const names[] = { basePath, name };
    char fullName[PIM_PATH] = { 0 };
    ConcatNames(ARGS(fullName), ARGS(names));

    // TODO: Cache imported materials for reuse

    Material material = { 0 };
    material.ior = 1.0f;
    if (cgmat->has_ior)
    {
        material.ior = cgmat->ior.ior;
    }

    if (cgmat->normal_texture.texture)
    {
        cgltf_image* cgnormal = cgmat->normal_texture.texture->image;
        material.normal = CreateNormalTexture(basePath, cgnormal);
    }

    cgltf_image* cgocclusion = NULL;
    if (cgmat->occlusion_texture.texture)
    {
        cgocclusion = cgmat->occlusion_texture.texture->image;
    }

    // TODO: add dedicated emission texture to Material
    cgltf_image* cgemission = NULL;
    if (cgmat->emissive_texture.texture)
    {
        cgemission = cgmat->emissive_texture.texture->image;
    }

    if (cgmat->has_pbr_metallic_roughness)
    {
        cgltf_image* cgalbedo = NULL;
        cgltf_image* cgmetallicroughness = NULL;
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
        cgltf_image* cgdiffuse = NULL;
        cgltf_image* cgspecular = NULL;
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

    dr->materials[iDrawable] = material;

    return true;
}

static bool LoadImageUri(
    const char* basePath,
    const char* uri,
    Texture* tex)
{
    memset(tex, 0, sizeof(*tex));

    char imagePath[PIM_PATH] = { 0 };
    StrCpy(ARGS(imagePath), basePath);
    StrCat(ARGS(imagePath), uri);
    cgltf_decode_uri(imagePath);

    if (stbi_is_hdr(imagePath))
    {
        i32 channels = 0;
        tex->format = VK_FORMAT_R32G32B32A32_SFLOAT;
        tex->texels = stbi_loadf(imagePath, &tex->size.x, &tex->size.y, &channels, 4);
    }
    else if (stbi_is_16_bit(imagePath))
    {
        i32 channels = 0;
        tex->format = VK_FORMAT_R16G16B16A16_UNORM;
        tex->texels = stbi_load_16(imagePath, &tex->size.x, &tex->size.y, &channels, 4);
    }
    else
    {
        i32 channels = 0;
        tex->format = VK_FORMAT_R8G8B8A8_SRGB;
        tex->texels = stbi_load(imagePath, &tex->size.x, &tex->size.y, &channels, 4);
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
            Con_Logf(LogSev_Error, "gltf", "Failed to load image at '%s' due to '%s'.", imagePath, reason);
        }
        memset(tex, 0, sizeof(*tex));
        return false;
    }
}

static bool ResampleToFloat4(Texture* tex)
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
    if (ResampleToFloat4(tex))
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
    if (ResampleToFloat4(tex))
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
    cgltf_image* cgalbedo)
{
    TextureId id = { 0 };
    if (!cgalbedo)
    {
        return id;
    }

    char fullName[PIM_PATH] = { 0 };
    char const *const names[] = { basePath, cgalbedo->uri };
    ConcatNames(ARGS(fullName), ARGS(names));
    cgltf_decode_uri(fullName);
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
    if (!ResampleToFloat4(&tex))
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
    cgltf_image* cgmetallic_roughness,
    cgltf_image* cgocclusion,
    cgltf_image* cgemission)
{
    const char* mrname = cgmetallic_roughness ? cgmetallic_roughness->uri : NULL;
    const char* occname = cgocclusion ? cgocclusion->uri : NULL;
    const char* emname = cgemission ? cgemission->uri : NULL;
    char const *const names[] = { basePath, mrname, occname, emname };
    char fullName[PIM_PATH] = { 0 };
    ConcatNames(ARGS(fullName), ARGS(names));
    cgltf_decode_uri(fullName);
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
            ResampleToFloat4(&mrtex);
        }
    }
    if (!mrtex.texels)
    {
        mrtex.size = i2_1;
        mrtex.texels = &defaultMr;
    }

    if (cgocclusion)
    {
        if (LoadImageUri(basePath, cgocclusion->uri, &occtex))
        {
            ResampleToFloat4(&occtex);
        }
    }
    if (!occtex.texels)
    {
        occtex.size = i2_1;
        occtex.texels = &defaultOcc;
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
    }

    Texture tex = { 0 };
    {
        int2 size = i2_1;
        size = i2_max(size, mrtex.size);
        size = i2_max(size, occtex.size);
        size = i2_max(size, emtex.size);
        const i32 len = size.x * size.y;
        R8G8B8A8_t* pim_noalias dst = Tex_Alloc(sizeof(dst[0]) * len);
        for (i32 y = 0; y < size.y; ++y)
        {
            for (i32 x = 0; x < size.x; ++x)
            {
                float2 uv = CoordToUv(size, i2_v(x, y));
                float4 rome = f4_0;
                {
                    float4 sample = UvBilinearClamp_f4(mrtex.texels, mrtex.size, uv);
                    rome.x = sample.y;
                    rome.z = sample.x;
                }
                rome.y = UvBilinearClamp_f4(occtex.texels, occtex.size, uv).x;
                rome.w = PackEmission(UvBilinearClamp_f4(emtex.texels, emtex.size, uv));
                i32 index = x + y * size.x;
                dst[index] = GammaEncode_rgba8(rome);
            }
        }
        tex.size = size;
        tex.texels = dst;
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
    cgltf_image* cgnormal)
{
    TextureId id = { 0 };
    if (!cgnormal)
    {
        return id;
    }

    char fullName[PIM_PATH] = { 0 };
    char const *const names[] = { basePath, cgnormal->uri };
    ConcatNames(ARGS(fullName), ARGS(names));
    cgltf_decode_uri(fullName);
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
    if (!ResampleToFloat4(&tex))
    {
        Mem_Free(tex.texels);
        return id;
    }
    {
        const float4* pim_noalias src = tex.texels;
        const i32 len = tex.size.x * tex.size.y;
        short2* pim_noalias dst = Tex_Alloc(sizeof(dst[0]) * len);
        for (i32 i = 0; i < len; ++i)
        {
            dst[i] = NormalTsToXy16(f4_snorm(src[i]));
        }
        Mem_Free(tex.texels);
        tex.texels = dst;
        tex.format = VK_FORMAT_R16G16_SNORM;
    }
    Texture_New(&tex, tex.format, VK_SAMPLER_ADDRESS_MODE_REPEAT, guid, &id);
    return id;
}

bool ResampleToAlbedoRome(
    const char* basePath,
    Material* material,
    cgltf_image* cgdiffuse,
    cgltf_image* cgspecular,
    cgltf_image* cgocclusion,
    cgltf_image* cgemission,
    float glossiness)
{
    const char* diffuseName = cgdiffuse ? cgdiffuse->uri : NULL;
    const char* specularName = cgspecular ? cgspecular->uri : NULL;
    const char* occlusionName = cgocclusion ? cgocclusion->uri : NULL;
    const char* emissionName = cgemission ? cgemission->uri : NULL;
    char albedoFullname[PIM_PATH] = { 0 };
    char romeFullname[PIM_PATH] = { 0 };
    char const *const albedoNames[] = { basePath, diffuseName, specularName };
    char const *const romeNames[] = { basePath, diffuseName, specularName, occlusionName, emissionName };
    ConcatNames(ARGS(albedoFullname), ARGS(albedoNames));
    ConcatNames(ARGS(romeFullname), ARGS(romeNames));
    cgltf_decode_uri(albedoFullname);
    cgltf_decode_uri(romeFullname);
    Guid albedoGuid = Guid_FromStr(albedoFullname);
    Guid romeGuid = Guid_FromStr(romeFullname);

    if (Texture_Find(albedoGuid, &material->albedo) && Texture_Find(romeGuid, &material->rome))
    {
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
            if (ResampleToFloat4(&diffuseTex))
            {
                ImportColorspace(&diffuseTex);
            }
        }
    }
    if (!diffuseTex.texels)
    {
        diffuseTex.size = i2_1;
        diffuseTex.texels = &defaultDiffuse;
    }

    if (cgspecular)
    {
        if (LoadImageUri(basePath, cgspecular->uri, &specularTex))
        {
            if (ResampleToFloat4(&specularTex))
            {
                ImportColorspace(&specularTex);
            }
        }
    }
    if (!specularTex.texels)
    {
        specularTex.size = i2_1;
        specularTex.texels = &defaultSpecular;
    }

    if (cgocclusion)
    {
        if (LoadImageUri(basePath, cgocclusion->uri, &occtex))
        {
            ResampleToFloat4(&occtex);
        }
    }
    if (!occtex.texels)
    {
        occtex.size = i2_1;
        occtex.texels = &defaultOcclusion;
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
        for (i32 y = 0; y < size.y; ++y)
        {
            for (i32 x = 0; x < size.x; ++x)
            {
                float2 uv = CoordToUv(size, i2_v(x, y));
                float4 diffuse = UvBilinearClamp_f4(diffuseTex.texels, diffuseTex.size, uv);
                float4 specular = UvBilinearClamp_f4(specularTex.texels, specularTex.size, uv);
                float occlusion = UvBilinearClamp_f4(occtex.texels, occtex.size, uv).x;
                float emission = PackEmission(UvBilinearClamp_f4(emtex.texels, emtex.size, uv));
                float4 albedo;
                float roughness;
                float metallic;
                SpecularToPBR(diffuse, specular, glossiness, &albedo, &roughness, &metallic);
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
        rometex.format = VK_FORMAT_R8G8B8A8_SRGB;
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
        Mem_Free(albedotex.texels);
    }
    if (!Texture_Exists(material->rome))
    {
        Texture_New(&rometex, rometex.format, VK_SAMPLER_ADDRESS_MODE_REPEAT, romeGuid, &material->rome);
    }
    else
    {
        Mem_Free(rometex.texels);
    }

    return true;
}

static bool CreateLight(
    const char* basePath,
    cgltf_light* cglight,
    Entities* dr,
    i32 iDrawable)
{
    if (!cglight)
    {
        return false;
    }
    const char* name = cglight->name;
    if (!name || !name[0])
    {
        ASSERT(false);
        return false;
    }
    // TODO: support punctual lights by converting to area lights.
    return false;
}
