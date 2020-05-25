#include "rendering/denoise.h"
#include "allocator/allocator.h"
#include "common/library.h"
#include "common/console.h"
#include "OpenImageDenoise/oidn.h"
#include <string.h>

typedef struct oidnlib_s
{
    library_t lib;

    // -----------------------------------------------------------------------------
    // Device
    // -----------------------------------------------------------------------------

    // Creates a new device.
    OIDNDevice(*NewDevice)(OIDNDeviceType type);

    // Retains the device (increments the reference count).
    void(*RetainDevice)(OIDNDevice device);

    // Releases the device (decrements the reference count).
    void(*ReleaseDevice)(OIDNDevice device);

    // Sets a boolean parameter of the device.
    void(*SetDevice1b)(OIDNDevice device, const char* name, bool value);

    // Sets an integer parameter of the device.
    void(*SetDevice1i)(OIDNDevice device, const char* name, int value);

    // Gets a boolean parameter of the device.
    bool(*GetDevice1b)(OIDNDevice device, const char* name);

    // Gets an integer parameter of the device (e.g. "version").
    int(*GetDevice1i)(OIDNDevice device, const char* name);

    // Sets the error callback function of the device.
    void(*SetDeviceErrorFunction)(OIDNDevice device, OIDNErrorFunction func, void* userPtr);

    // Returns the first unqueried error code stored in the device for the current
    // thread, optionally also returning a string message (if not NULL), and clears
    // the stored error. Can be called with a NULL device as well to check why a
    // device creation failed.
    OIDNError(*GetDeviceError)(OIDNDevice device, const char** outMessage);

    // Commits all previous changes to the device.
    // Must be called before first using the device (e.g. creating filters).
    void(*CommitDevice)(OIDNDevice device);

    // -----------------------------------------------------------------------------
    // Buffer
    // -----------------------------------------------------------------------------

    // Creates a new buffer (data allocated and owned by the device).
    OIDNBuffer(*NewBuffer)(OIDNDevice device, size_t byteSize);

    // Creates a new shared buffer (data allocated and owned by the user).
    OIDNBuffer(*NewSharedBuffer)(OIDNDevice device, void* ptr, size_t byteSize);

    // Maps a region of the buffer to host memory.
    // If byteSize is 0, the maximum available amount of memory will be mapped.
    void* (*MapBuffer)(OIDNBuffer buffer, OIDNAccess access, size_t byteOffset, size_t byteSize);

    // Unmaps a region of the buffer.
    // mappedPtr must be a pointer returned by a previous call to oidnMapBuffer.
    void(*UnmapBuffer)(OIDNBuffer buffer, void* mappedPtr);

    // Retains the buffer (increments the reference count).
    void(*RetainBuffer)(OIDNBuffer buffer);

    // Releases the buffer (decrements the reference count).
    void(*ReleaseBuffer)(OIDNBuffer buffer);

    // -----------------------------------------------------------------------------
    // Filter
    // -----------------------------------------------------------------------------

    // Creates a new filter of the specified type (e.g. "RT").
    OIDNFilter(*NewFilter)(OIDNDevice device, const char* type);

    // Retains the filter (increments the reference count).
    void(*RetainFilter)(OIDNFilter filter);

    // Releases the filter (decrements the reference count).
    void(*ReleaseFilter)(OIDNFilter filter);

    // Sets an image parameter of the filter (stored in a buffer).
    // If bytePixelStride and/or byteRowStride are zero, these will be computed automatically.
    void(*SetFilterImage)(OIDNFilter filter, const char* name,
        OIDNBuffer buffer, OIDNFormat format,
        size_t width, size_t height,
        size_t byteOffset,
        size_t bytePixelStride, size_t byteRowStride);

    // Sets an image parameter of the filter (owned by the user).
    // If bytePixelStride and/or byteRowStride are zero, these will be computed automatically.
    void(*SetSharedFilterImage)(OIDNFilter filter, const char* name,
        void* ptr, OIDNFormat format,
        size_t width, size_t height,
        size_t byteOffset,
        size_t bytePixelStride, size_t byteRowStride);

    // Sets an opaque data parameter of the filter (owned by the user).
    void(*SetSharedFilterData)(OIDNFilter filter, const char* name,
        void* ptr, size_t byteSize);

    // Sets a boolean parameter of the filter.
    void(*SetFilter1b)(OIDNFilter filter, const char* name, bool value);

    // Gets a boolean parameter of the filter.
    bool(*GetFilter1b)(OIDNFilter filter, const char* name);

    // Sets an integer parameter of the filter.
    void(*SetFilter1i)(OIDNFilter filter, const char* name, int value);

    // Gets an integer parameter of the filter.
    int(*GetFilter1i)(OIDNFilter filter, const char* name);

    // Sets a float parameter of the filter.
    void(*SetFilter1f)(OIDNFilter filter, const char* name, float value);

    // Gets a float parameter of the filter.
    float(*GetFilter1f)(OIDNFilter filter, const char* name);

    // Sets the progress monitor callback function of the filter.
    void(*SetFilterProgressMonitorFunction)(OIDNFilter filter, OIDNProgressMonitorFunction func, void* userPtr);

    // Commits all previous changes to the filter.
    // Must be called before first executing the filter.
    void(*CommitFilter)(OIDNFilter filter);

    // Executes the filter.
    void(*ExecuteFilter)(OIDNFilter filter);

} oidnlib_t;

#define DENOISE_DYNAMIC 0

#if DENOISE_DYNAMIC
    #define LOAD_FN(dst, src, name) dst.name = Library_Sym(src, STR_TOK(oidn##name)); ASSERT(dst.name);
#else
    #define LOAD_FN(dst, src, name) dst.name = oidn##name
#endif // DENOISE_DYNAMIC

typedef struct denoise_filter_s
{
    void* handle;
    int2 size;
    const float3* color;
    const float3* albedo;
    const float3* normal;
    float3* output;
    DenoiseType type;
    bool hdr;
} denoise_filter_t;

static const char* const kFilterNames[] =
{
    "RT",
    "RTLightmap",
};

static bool ms_once;
static oidnlib_t oidn;

static void LoadLib(library_t lib)
{
    LOAD_FN(oidn, lib, NewDevice);
    LOAD_FN(oidn, lib, RetainDevice);
    LOAD_FN(oidn, lib, ReleaseDevice);
    LOAD_FN(oidn, lib, SetDevice1b);
    LOAD_FN(oidn, lib, SetDevice1i);
    LOAD_FN(oidn, lib, GetDevice1b);
    LOAD_FN(oidn, lib, GetDevice1i);
    LOAD_FN(oidn, lib, SetDeviceErrorFunction);
    LOAD_FN(oidn, lib, GetDeviceError);
    LOAD_FN(oidn, lib, CommitDevice);

    LOAD_FN(oidn, lib, NewBuffer);
    LOAD_FN(oidn, lib, NewSharedBuffer);
    LOAD_FN(oidn, lib, MapBuffer);
    LOAD_FN(oidn, lib, UnmapBuffer);
    LOAD_FN(oidn, lib, RetainBuffer);
    LOAD_FN(oidn, lib, ReleaseBuffer);

    LOAD_FN(oidn, lib, NewFilter);
    LOAD_FN(oidn, lib, RetainFilter);
    LOAD_FN(oidn, lib, ReleaseFilter);
    LOAD_FN(oidn, lib, SetFilterImage);
    LOAD_FN(oidn, lib, SetSharedFilterImage);
    LOAD_FN(oidn, lib, SetSharedFilterData);
    LOAD_FN(oidn, lib, SetFilter1b);
    LOAD_FN(oidn, lib, GetFilter1b);
    LOAD_FN(oidn, lib, SetFilter1i);
    LOAD_FN(oidn, lib, GetFilter1i);
    LOAD_FN(oidn, lib, SetFilter1f);
    LOAD_FN(oidn, lib, GetFilter1f);
    LOAD_FN(oidn, lib, SetFilterProgressMonitorFunction);
    LOAD_FN(oidn, lib, CommitFilter);
    LOAD_FN(oidn, lib, ExecuteFilter);
}

static bool EnsureInit(void)
{
    if (!ms_once)
    {
        ms_once = true;

        #if (DENOISE_DYNAMIC)
        {
            library_t lib = Library_Open("oidn/OpenImageDenoise");

            ASSERT(lib.handle);
            if (lib.handle)
            {
                oidn.lib = lib;
                LoadLib(lib);
            }
        }
        #else
        {
            LoadLib((library_t){0});
        }
        #endif
    }

    #if (DENOISE_DYNAMIC)
    {
        return oidn.lib.handle != NULL;
    }
    #else
    {
        return true;
    }
    #endif
}

static bool PrintErrors(OIDNDevice device)
{
    bool hadErrors = false;
    ASSERT(device);
    ASSERT(oidn.GetDeviceError);
    const char* errorMessage = NULL;
    while (oidn.GetDeviceError(device, &errorMessage) != OIDN_ERROR_NONE)
    {
        hadErrors = true;
        ASSERT(errorMessage);
        con_printf(C32_RED, "[OIDN][Error]: %s", errorMessage);
    }
    return hadErrors;
}

static void SetImage(
    OIDNFilter filter,
    const char* channel,
    float3* buffer,
    int2 size)
{
    oidn.SetSharedFilterImage(
        filter, channel, buffer, OIDN_FORMAT_FLOAT3, size.x, size.y, 0, 0, 0);
}

void Denoise_New(denoise_t* denoise)
{
    ASSERT(denoise);

    if (!EnsureInit())
    {
        ASSERT(false);
        return;
    }

    memset(denoise, 0, sizeof(*denoise));

    OIDNDevice device = oidn.NewDevice(OIDN_DEVICE_TYPE_DEFAULT);
    oidn.CommitDevice(device);
    PrintErrors(device);

    denoise->device = device;
    denoise->filter = perm_calloc(sizeof(denoise_filter_t));
}

void Denoise_Del(denoise_t* denoise)
{
    if (!EnsureInit())
    {
        ASSERT(false);
        return;
    }

    if (denoise)
    {
        denoise_filter_t* pFilter = denoise->filter;
        if (pFilter)
        {
            OIDNFilter filter = pFilter->handle;
            if (filter)
            {
                oidn.ReleaseFilter(filter);
            }
        }

        if (denoise->device)
        {
            oidn.ReleaseDevice(denoise->device);
        }

        pim_free(denoise->filter);
        denoise->filter = NULL;
        denoise->device = NULL;
    }
}

void Denoise_Execute(
    denoise_t* denoise,
    DenoiseType type,
    int2 size,
    bool hdr,
    const float3* color,
    const float3* albedo, // optional
    const float3* normal, // optional
    float3* output)
{
    ASSERT(denoise);
    ASSERT(type >= 0);
    ASSERT(type < DenoiseType_COUNT);
    ASSERT(size.x > 0);
    ASSERT(size.y > 0);
    ASSERT((size.x * size.y) > 0);
    ASSERT(color);
    ASSERT(output);

    if (!EnsureInit())
    {
        ASSERT(false);
        return;
    }

    OIDNDevice device = denoise->device;
    ASSERT(device);
    denoise_filter_t* pFilter = denoise->filter;
    ASSERT(pFilter);
    OIDNFilter filter = pFilter->handle;

    bool dirty = filter == NULL;
    if (!dirty)
    {
        denoise_filter_t args =
        {
            .handle = filter,
            .size = size,
            .color = color,
            .albedo = albedo,
            .normal = normal,
            .output = output,
            .type = type,
            .hdr = hdr,
        };
        dirty = memcmp(&args, pFilter, sizeof(args)) != 0;
    }

    if (dirty)
    {
        if (filter)
        {
            oidn.ReleaseFilter(filter);
            filter = NULL;
            pFilter->handle = NULL;
        }

        filter = oidn.NewFilter(device, kFilterNames[type]);

        SetImage(filter, "color", (float3*)color, size);
        if (albedo)
        {
            SetImage(filter, "albedo", (float3*)albedo, size);
        }
        if (normal)
        {
            SetImage(filter, "normal", (float3*)normal, size);
        }
        SetImage(filter, "output", output, size);

        oidn.SetFilter1b(filter, "hdr", hdr);

        oidn.CommitFilter(filter);

        PrintErrors(device);

        pFilter->albedo = albedo;
        pFilter->color = color;
        pFilter->handle = filter;
        pFilter->hdr = hdr;
        pFilter->normal = normal;
        pFilter->output = output;
        pFilter->size = size;
        pFilter->type = type;
    }

    oidn.ExecuteFilter(filter);

    PrintErrors(device);
}
