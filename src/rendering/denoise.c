#include "rendering/denoise.h"
#include "common/console.h"
#include "common/library.h"
#include "common/time.h"
#include <OpenImageDenoise/oidn.h>
#include <string.h>

typedef struct oidn_s
{
    library_t lib;

    OIDNDevice(*oidnNewDevice)(OIDNDeviceType type);
    void(*oidnCommitDevice)(OIDNDevice device);
    OIDNError(*oidnGetDeviceError)(OIDNDevice device, const char** outMessage);

    OIDNFilter(*oidnNewFilter)(OIDNDevice device, const char* type);
    void(*oidnReleaseFilter)(OIDNFilter filter);
    void(*oidnSetSharedFilterImage)(OIDNFilter filter, const char* name,
        void* ptr, OIDNFormat format,
        size_t width, size_t height,
        size_t byteOffset,
        size_t bytePixelStride, size_t byteRowStride);
    void(*oidnSetFilter1b)(OIDNFilter filter, const char* name, bool value);
    void(*oidnCommitFilter)(OIDNFilter filter);
    void(*oidnExecuteFilter)(OIDNFilter filter);
} oidn_t;

typedef struct CacheKey_s
{
    DenoiseType type;
    int2 size;
    const void* color;
    const void* albedo;
    const void* normal;
    const void* output;
} CacheKey;

#define kMaxCachedFilters 8

static bool ms_once;
static oidn_t oidn;
static OIDNDevice ms_device;

static u64 ms_cacheTicks[kMaxCachedFilters];
static CacheKey ms_cacheKeys[kMaxCachedFilters];
static OIDNFilter ms_cacheValues[kMaxCachedFilters];

static bool LogErrors(void)
{
    bool hadError = false;
    const char* msg = NULL;
    while (oidn.oidnGetDeviceError(ms_device, &msg) != OIDN_ERROR_NONE)
    {
        hadError = true;
        con_logf(LogSev_Error, "oidn", "%s", msg);
    }
    return hadError;
}

static bool EnsureInit(void)
{
    if (!ms_once)
    {
        ms_once = true;

        oidn.lib = Library_Open("OpenImageDenoise");
        if (!oidn.lib.handle)
        {
            return false;
        }

        oidn.oidnCommitDevice = Library_Sym(oidn.lib, "oidnCommitDevice");
        oidn.oidnCommitFilter = Library_Sym(oidn.lib, "oidnCommitFilter");
        oidn.oidnExecuteFilter = Library_Sym(oidn.lib, "oidnExecuteFilter");
        oidn.oidnGetDeviceError = Library_Sym(oidn.lib, "oidnGetDeviceError");
        oidn.oidnNewDevice = Library_Sym(oidn.lib, "oidnNewDevice");
        oidn.oidnNewFilter = Library_Sym(oidn.lib, "oidnNewFilter");
        oidn.oidnReleaseFilter = Library_Sym(oidn.lib, "oidnReleaseFilter");
        oidn.oidnSetFilter1b = Library_Sym(oidn.lib, "oidnSetFilter1b");
        oidn.oidnSetSharedFilterImage = Library_Sym(oidn.lib, "oidnSetSharedFilterImage");

        ms_device = oidn.oidnNewDevice(OIDN_DEVICE_TYPE_DEFAULT);
        ASSERT(ms_device);
        if (!ms_device)
        {
            LogErrors();
            return false;
        }
        oidn.oidnCommitDevice(ms_device);


        LogErrors();
    }
    return ms_device != NULL;
}

static void SetImage(OIDNFilter filter, const char* name, int2 size, const void* image)
{
    ASSERT(filter);
    ASSERT(image);

    oidn.oidnSetSharedFilterImage(
        filter, name, (void*)image, OIDN_FORMAT_FLOAT3, size.x, size.y, 0, 0, 0);
}

static OIDNFilter NewFilter(const CacheKey* key)
{
    ASSERT(ms_device);
    ASSERT(key);
    ASSERT(key->color);
    ASSERT(key->output);
    ASSERT(key->size.x > 0);
    ASSERT(key->size.y > 0);

    const char* typeStr = "RT";
    switch (key->type)
    {
    case DenoiseType_Image:
        typeStr = "RT";
        break;
    case DenoiseType_Lightmap:
        typeStr = "RTLightmap";
        break;
    }

    OIDNFilter filter = oidn.oidnNewFilter(ms_device, typeStr);
    ASSERT(filter);
    if (!filter)
    {
        LogErrors();
        return NULL;
    }

    oidn.oidnSetFilter1b(filter, "hdr", true);
    SetImage(filter, "color", key->size, key->color);
    SetImage(filter, "output", key->size, key->output);
    if (key->albedo)
    {
        SetImage(filter, "albedo", key->size, key->albedo);
    }
    if (key->normal)
    {
        SetImage(filter, "normal", key->size, key->normal);
    }
    oidn.oidnCommitFilter(filter);

    if (LogErrors())
    {
        oidn.oidnReleaseFilter(filter);
        return NULL;
    }
    return filter;
}

static i32 FindFilter(const CacheKey* key)
{
    const OIDNFilter* values = ms_cacheValues;
    const CacheKey* keys = ms_cacheKeys;
    for (i32 i = 0; i < kMaxCachedFilters; ++i)
    {
        if (values[i])
        {
            if (memcmp(key, keys + i, sizeof(CacheKey)) == 0)
            {
                return i;
            }
        }
    }
    return -1;
}

static i32 FindLRU(void)
{
    u64 now = time_now();
    u64 diff = 0;
    i32 chosen = 0;
    const u64* ticks = ms_cacheTicks;
    for (i32 i = 0; i < kMaxCachedFilters; ++i)
    {
        u64 iDiff = now - ticks[i];
        if (iDiff > diff)
        {
            chosen = i;
            diff = iDiff;
        }
    }
    return chosen;
}

static OIDNFilter GetFilter(const CacheKey* key)
{
    i32 i = FindFilter(key);
    if (i == -1)
    {
        OIDNFilter newFilter = NewFilter(key);
        if (!newFilter)
        {
            return NULL;
        }

        i = FindLRU();
        if (ms_cacheValues[i])
        {
            oidn.oidnReleaseFilter(ms_cacheValues[i]);
            ms_cacheValues[i] = NULL;
        }

        ms_cacheKeys[i] = *key;
        ms_cacheValues[i] = newFilter;
    }
    ms_cacheTicks[i] = time_now();
    return ms_cacheValues[i];
}

bool Denoise(
    DenoiseType type,
    int2 size,
    const float3* color,
    const float3* albedo,
    const float3* normal,
    float3* output)
{
    if (!EnsureInit())
    {
        return false;
    }

    if (!color || !output)
    {
        return false;
    }

    const CacheKey key =
    {
        .type = type,
        .size = size,
        .color = color,
        .albedo = albedo,
        .normal = normal,
        .output = output,
    };

    OIDNFilter filter = GetFilter(&key);
    if (!filter)
    {
        return false;
    }

    oidn.oidnExecuteFilter(filter);

    if (LogErrors())
    {
        return false;
    }
    return true;
}
