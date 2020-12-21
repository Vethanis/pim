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
    drawables_t* dr);
static bool CreateScene(
    const char* basePath,
    cgltf_scene* cgscene,
    drawables_t* dr);
static i32 CreateNode(
    const char* basePath,
    cgltf_scene* cgscene,
    cgltf_node* cgnode,
    drawables_t* dr);
static bool CreateMesh(
    const char* basePath,
    cgltf_mesh* cgmesh,
    drawables_t* dr,
    i32 iDrawable);
static bool CreateMaterial(
    const char* basePath,
    cgltf_material* cgmat,
    drawables_t* dr,
    i32 iDrawable);
static textureid_t CreateAlbedoTexture(
    const char* basePath,
    cgltf_image* cgalbedo);
static textureid_t CreateRomeTexture(
    const char* basePath,
    cgltf_image* cgmetallic_roughness,
    cgltf_image* cgocclusion,
    cgltf_image* cgemission);
static textureid_t CreateNormalTexture(
    const char* basePath,
    cgltf_image* cgnormal);
bool ResampleToAlbedoRome(
    const char* basePath,
    material_t* material,
    cgltf_image* cgdiffuse,
    cgltf_image* cgspecular,
    cgltf_image* cgocclusion,
    cgltf_image* cgemission,
    float glossiness);
static bool CreateLight(
    const char* basePath,
    cgltf_light* cglight,
    drawables_t* dr,
    i32 iDrawable);

// ----------------------------------------------------------------------------

static void CatNodeLineage(char* dst, i32 size, cgltf_node* cgnode);
static char const *const ShortenString(char const *const str, i32 maxLen);
static bool LoadImageUri(
    const char* basePath,
    const char* uri,
    texture_t* tex);

// ----------------------------------------------------------------------------

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

    success = CreateScenes(path, cgdata, dr);

cleanup:
    cgltf_free(cgdata);
    fmap_close(&map);
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
    drawables_t* dr)
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
    drawables_t* dr)
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
    drawables_t* dr)
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
    guid_t guid = guid_str(fullName);

    i32 iDrawable = drawables_find(dr, guid);
    if (iDrawable >= 0)
    {
        return iDrawable;
    }
    iDrawable = drawables_add(dr, guid);

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
    drawables_t* dr,
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
    guid_t guid = guid_str(fullName);

    meshid_t id = { 0 };
    if (mesh_find(guid, &id))
    {
        mesh_retain(id);
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
    drawables_t* dr,
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

    material_t material = { 0 };
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

    // TODO: add dedicated emission texture to material_t
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
        material.flags |= matflag_emissive;
    }
    if ((material.ior != 1.0f) || cgmat->has_transmission)
    {
        material.flags |= matflag_refractive;
    }

    dr->materials[iDrawable] = material;

    return true;
}

static bool LoadImageUri(
    const char* basePath,
    const char* uri,
    texture_t* tex)
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
            con_logf(LogSev_Error, "gltf", "Failed to load image at '%s' due to '%s'.", imagePath, reason);
        }
        memset(tex, 0, sizeof(*tex));
        return false;
    }
}

static bool ResampleToFloat4(texture_t* tex)
{
    switch (tex->format)
    {
    default:
        ASSERT(false);
        return false;
    case VK_FORMAT_R8G8B8A8_SRGB:
    {
        const i32 len = tex->size.x * tex->size.y;
        const u8* pim_noalias src = tex->texels;
        float4* pim_noalias dst = tex_malloc(sizeof(dst[0]) * len);
        prng_t rng = prng_get();
        for (i32 i = 0; i < len; ++i)
        {
            float4 texel = f4_0;
            texel.x = f1_sat((src[i * 4 + 0] + prng_f32(&rng) - 0.5f) * (1.0f / 255.0f));
            texel.y = f1_sat((src[i * 4 + 1] + prng_f32(&rng) - 0.5f) * (1.0f / 255.0f));
            texel.z = f1_sat((src[i * 4 + 2] + prng_f32(&rng) - 0.5f) * (1.0f / 255.0f));
            texel.w = f1_sat((src[i * 4 + 3] + prng_f32(&rng) - 0.5f) * (1.0f / 255.0f));
            texel = f4_tolinear(texel);
            dst[i] = texel;
        }
        prng_set(rng);
        pim_free(tex->texels);
        tex->texels = dst;
        tex->format = VK_FORMAT_R32G32B32A32_SFLOAT;
    }
    return true;
    case VK_FORMAT_R8G8B8A8_UNORM:
    {
        const i32 len = tex->size.x * tex->size.y;
        const u8* pim_noalias src = tex->texels;
        float4* pim_noalias dst = tex_malloc(sizeof(dst[0]) * len);
        prng_t rng = prng_get();
        for (i32 i = 0; i < len; ++i)
        {
            float4 texel = f4_0;
            texel.x = f1_sat((src[i * 4 + 0] + prng_f32(&rng) - 0.5f) * (1.0f / 255.0f));
            texel.y = f1_sat((src[i * 4 + 1] + prng_f32(&rng) - 0.5f) * (1.0f / 255.0f));
            texel.z = f1_sat((src[i * 4 + 2] + prng_f32(&rng) - 0.5f) * (1.0f / 255.0f));
            texel.w = f1_sat((src[i * 4 + 3] + prng_f32(&rng) - 0.5f) * (1.0f / 255.0f));
            dst[i] = texel;
        }
        prng_set(rng);
        pim_free(tex->texels);
        tex->texels = dst;
        tex->format = VK_FORMAT_R32G32B32A32_SFLOAT;
    }
    return true;
    case VK_FORMAT_R16G16B16A16_UNORM:
    {
        const i32 len = tex->size.x * tex->size.y;
        const u16* pim_noalias src = tex->texels;
        float4* pim_noalias dst = tex_malloc(sizeof(dst[0]) * len);
        prng_t rng = prng_get();
        for (i32 i = 0; i < len; ++i)
        {
            float4 texel = f4_0;
            texel.x = f1_sat((src[i * 4 + 0] + prng_f32(&rng) - 0.5f) * (1.0f / 65535.0f));
            texel.y = f1_sat((src[i * 4 + 1] + prng_f32(&rng) - 0.5f) * (1.0f / 65535.0f));
            texel.z = f1_sat((src[i * 4 + 2] + prng_f32(&rng) - 0.5f) * (1.0f / 65535.0f));
            texel.w = f1_sat((src[i * 4 + 3] + prng_f32(&rng) - 0.5f) * (1.0f / 65535.0f));
            dst[i] = texel;
        }
        prng_set(rng);
        pim_free(tex->texels);
        tex->texels = dst;
        tex->format = VK_FORMAT_R32G32B32A32_SFLOAT;
    }
    return true;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
    return true;
    }
}

static bool ResampleToSrgb(texture_t* tex)
{
    if (tex->format == VK_FORMAT_R8G8B8A8_SRGB)
    {
        return true;
    }
    if (ResampleToFloat4(tex))
    {
        const i32 len = tex->size.x * tex->size.y;
        const float4* pim_noalias src = tex->texels;
        u32* pim_noalias dst = tex_malloc(sizeof(dst[0]) * len);
        for (i32 i = 0; i < len; ++i)
        {
            dst[i] = LinearToColor(src[i]);
        }
        pim_free(tex->texels);
        tex->texels = dst;
        return true;
    }
    return false;
}

static bool ResampleToEmission(texture_t* tex)
{
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

static textureid_t CreateAlbedoTexture(
    const char* basePath,
    cgltf_image* cgalbedo)
{
    textureid_t id = { 0 };
    if (!cgalbedo)
    {
        return id;
    }

    char fullName[PIM_PATH] = { 0 };
    char const *const names[] = { basePath, cgalbedo->uri };
    ConcatNames(ARGS(fullName), ARGS(names));
    cgltf_decode_uri(fullName);
    guid_t guid = guid_str(fullName);

    if (texture_find(guid, &id))
    {
        texture_retain(id);
        return id;
    }

    texture_t tex = { 0 };
    if (!LoadImageUri(basePath, cgalbedo->uri, &tex))
    {
        return id;
    }
    if (!ResampleToSrgb(&tex))
    {
        pim_free(tex.texels);
        return id;
    }
    texture_new(&tex, tex.format, guid, &id);
    return id;
}

static textureid_t CreateRomeTexture(
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
    guid_t guid = guid_str(fullName);

    textureid_t id = { 0 };
    if (texture_find(guid, &id))
    {
        texture_retain(id);
        return id;
    }

    texture_t mrtex = { 0 };
    texture_t occtex = { 0 };
    texture_t emtex = { 0 };
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
            ResampleToEmission(&emtex);
        }
    }
    if (!emtex.texels)
    {
        emtex.size = i2_1;
        emtex.texels = &defaultEm;
    }

    int2 size = i2_1;
    size = i2_max(size, mrtex.size);
    size = i2_max(size, occtex.size);
    size = i2_max(size, emtex.size);
    const i32 len = size.x * size.y;
    u32* pim_noalias dst = tex_malloc(sizeof(dst[0]) * len);
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
            u32 color = LinearToColor(rome);
            u32 index = x + y * size.x;
            dst[index] = color;
        }
    }
    texture_t tex = { 0 };
    tex.size = size;
    tex.texels = dst;

    if (mrtex.texels != &defaultMr)
    {
        pim_free(mrtex.texels);
    }
    if (occtex.texels != &defaultOcc)
    {
        pim_free(occtex.texels);
    }
    if (emtex.texels != &defaultEm)
    {
        pim_free(emtex.texels);
    }

    texture_new(&tex, VK_FORMAT_R8G8B8A8_SRGB, guid, &id);
    return id;
}

static textureid_t CreateNormalTexture(
    const char* basePath,
    cgltf_image* cgnormal)
{
    textureid_t id = { 0 };
    if (!cgnormal)
    {
        return id;
    }

    char fullName[PIM_PATH] = { 0 };
    char const *const names[] = { basePath, cgnormal->uri };
    ConcatNames(ARGS(fullName), ARGS(names));
    cgltf_decode_uri(fullName);
    guid_t guid = guid_str(fullName);

    if (texture_find(guid, &id))
    {
        texture_retain(id);
        return id;
    }

    texture_t tex = { 0 };
    if (!LoadImageUri(basePath, cgnormal->uri, &tex))
    {
        return id;
    }
    if (!ResampleToFloat4(&tex))
    {
        pim_free(tex.texels);
        return id;
    }
    {
        // TODO: encode tangent space as 16 bit xy, and regenerate z with sqrt(1 - (x*x + y*y));
        const float4* pim_noalias src = tex.texels;
        const i32 len = tex.size.x * tex.size.y;
        u32* pim_noalias dst = tex_malloc(sizeof(dst[0]) * len);
        for (i32 i = 0; i < len; ++i)
        {
            dst[i] = DirectionToColor(f4_snorm(src[i]));
        }
        pim_free(tex.texels);
        tex.texels = dst;
        tex.format = VK_FORMAT_R8G8B8A8_UNORM;
    }
    texture_new(&tex, VK_FORMAT_R8G8B8A8_UNORM, guid, &id);
    return id;
}

bool ResampleToAlbedoRome(
    const char* basePath,
    material_t* material,
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
    guid_t albedoGuid = guid_str(albedoFullname);
    guid_t romeGuid = guid_str(romeFullname);

    if (texture_find(albedoGuid, &material->albedo) && texture_find(romeGuid, &material->rome))
    {
        return true;
    }

    texture_t diffuseTex = { 0 };
    texture_t specularTex = { 0 };
    texture_t occtex = { 0 };
    texture_t emtex = { 0 };
    float4 defaultDiffuse = { 0.5f, 0.5f, 0.5f, 1.0f };
    float4 defaultSpecular = { 0.0f, 0.0f, 0.0f, 0.0f };
    float4 defaultOcclusion = { 1.0f, 0.0f, 0.0f, 0.0f };
    float4 defaultEmission = { 0.0f, 0.0f, 0.0f, 0.0f };

    if (cgdiffuse)
    {
        if (LoadImageUri(basePath, cgdiffuse->uri, &diffuseTex))
        {
            ResampleToFloat4(&diffuseTex);
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
            ResampleToFloat4(&specularTex);
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
            ResampleToEmission(&emtex);
        }
    }
    if (!emtex.texels)
    {
        emtex.size = i2_1;
        emtex.texels = &defaultEmission;
    }

    texture_t albedotex = { 0 };
    texture_t rometex = { 0 };
    {
        int2 size = i2_1;
        size = i2_max(size, diffuseTex.size);
        size = i2_max(size, specularTex.size);
        size = i2_max(size, occtex.size);
        size = i2_max(size, emtex.size);
        const i32 len = size.x * size.y;
        u32* pim_noalias albedoArr = tex_malloc(sizeof(albedoArr[0]) * len);
        u32* pim_noalias romeArr = tex_malloc(sizeof(romeArr[0]) * len);
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
                ConvertToMetallicRoughness(diffuse, specular, glossiness, &albedo, &roughness, &metallic);
                i32 index = x + y * size.x;
                albedoArr[index] = LinearToColor(albedo);
                romeArr[index] = LinearToColor(f4_v(roughness, occlusion, metallic, emission));
            }
        }
        albedotex.texels = albedoArr;
        albedotex.size = size;
        rometex.texels = romeArr;
        rometex.size = size;
    }

    if (diffuseTex.texels != &defaultDiffuse)
    {
        pim_free(diffuseTex.texels);
    }
    if (specularTex.texels != &defaultSpecular)
    {
        pim_free(specularTex.texels);
    }
    if (occtex.texels != &defaultOcclusion)
    {
        pim_free(occtex.texels);
    }
    if (emtex.texels != &defaultEmission)
    {
        pim_free(emtex.texels);
    }

    if (!texture_exists(material->albedo))
    {
        texture_new(&albedotex, VK_FORMAT_R8G8B8A8_SRGB, albedoGuid, &material->albedo);
    }
    else
    {
        pim_free(albedotex.texels);
    }
    if (!texture_exists(material->rome))
    {
        texture_new(&rometex, VK_FORMAT_R8G8B8A8_SRGB, romeGuid, &material->rome);
    }
    else
    {
        pim_free(rometex.texels);
    }

    return true;
}

static bool CreateLight(
    const char* basePath,
    cgltf_light* cglight,
    drawables_t* dr,
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
