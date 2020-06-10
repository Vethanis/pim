#include "rendering/denoise.h"
#include "common/console.h"
#include "common/library.h"
#include <OpenImageDenoise/oidn.h>

typedef struct oidn_s
{
    library_t lib;

    OIDNDevice(*oidnNewDevice)(OIDNDeviceType type);
    void(*oidnCommitDevice)(OIDNDevice device);
    OIDNError (*oidnGetDeviceError)(OIDNDevice device, const char** outMessage);

    OIDNFilter (*oidnNewFilter)(OIDNDevice device, const char* type);
    void (*oidnSetSharedFilterImage)(OIDNFilter filter, const char* name,
        void* ptr, OIDNFormat format,
        size_t width, size_t height,
        size_t byteOffset,
        size_t bytePixelStride, size_t byteRowStride);
    void (*oidnSetFilter1b)(OIDNFilter filter, const char* name, bool value);
    void (*oidnCommitFilter)(OIDNFilter filter);
    void (*oidnExecuteFilter)(OIDNFilter filter);
} oidn_t;

static bool ms_once;
static oidn_t oidn;
static OIDNDevice ms_device;
static OIDNFilter ms_filter;

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

        ms_filter = oidn.oidnNewFilter(ms_device, "RT");
        ASSERT(ms_filter);
        if (!ms_filter)
        {
            LogErrors();
            return false;
        }
        oidn.oidnSetFilter1b(ms_filter, "hdr", true);

        LogErrors();
    }
    return ms_filter != NULL;
}

static void SetImage(const char* name, int2 size, float3* image)
{
    ASSERT(ms_filter);
    ASSERT(image);

    oidn.oidnSetSharedFilterImage(
        ms_filter, name, image, OIDN_FORMAT_FLOAT3, size.x, size.y, 0, 0, 0);
}

bool Denoise(
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

    SetImage("color", size, (float3*)color);
    if (albedo)
    {
        SetImage("albedo", size, (float3*)albedo);
    }
    if (normal)
    {
        SetImage("normal", size, (float3*)normal);
    }
    SetImage("output", size, output);
    oidn.oidnCommitFilter(ms_filter);
    oidn.oidnExecuteFilter(ms_filter);

    if (LogErrors())
    {
        return false;
    }
    return true;
}
