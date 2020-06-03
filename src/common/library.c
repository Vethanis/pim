#include "common/library.h"
#include "common/stringutil.h"
#include "common/console.h"
#include "allocator/allocator.h"
#include "io/dir.h"
#include <string.h>

static void* OS_dlopen(const char* name);
static void* OS_dlsym(void* hdl, const char* name);
static void OS_dlclose(void* hdl);
static void OS_libfmt(char* buffer, i32 size, const char* name);
static void OS_geterror(char* buffer, i32 size);

library_t Library_Open(const char* name)
{
    library_t lib = { 0 };
    if (name)
    {
        char relPath[PIM_PATH] = { 0 };
        OS_libfmt(ARGS(relPath), name);

        con_printf(C32_WHITE, "Loading library '%s'", relPath);

        lib.handle = OS_dlopen(relPath);

        if (!lib.handle)
        {
            con_printf(C32_RED, "Failed to load library '%s'", relPath);
            OS_geterror(ARGS(relPath));
            con_printf(C32_RED, "%s", relPath);
        }
        else
        {
            con_printf(C32_GREEN, "Loaded library '%s' to %p", relPath, lib.handle);
        }
    }
    return lib;
}

void Library_Close(library_t lib)
{
    if (lib.handle)
    {
        OS_dlclose(lib.handle);
    }
}

void* Library_Sym(library_t lib, const char* name)
{
    void* result = NULL;

    if (lib.handle && name)
    {
        result = OS_dlsym(lib.handle, name);
    }

    return result;
}

#if PLAT_WINDOWS
    #define LIB_EXT ".dll"
#elif PLAT_LINUX
    #define LIB_EXT ".so"
#elif PLAT_MAC
    #define LIB_EXT ".dylib"
#elif PLAT_ANDROID
    #define LIB_EXT ".so"
#elif PLAT_IOS
    #define LIB_EXT ".dylib"
    #error iOS AppStore does not accept anything but static linkage
#endif // PLAT_STR

static void OS_libfmt(char* buffer, i32 size, const char* name)
{
    StrCpy(buffer, size, name);
    StrCat(buffer, size, LIB_EXT);
    StrPath(buffer, size);
}

#if PLAT_WINDOWS
// WIN32 API

#include <Windows.h>

SASSERT(sizeof(FARPROC) == sizeof(void*));
SASSERT(_Alignof(FARPROC) == _Alignof(void*));

SASSERT(sizeof(HMODULE) == sizeof(void*));
SASSERT(_Alignof(HMODULE) == _Alignof(void*));

static void* OS_dlopen(const char* name)
{
    HMODULE hmod = LoadLibraryA(name);
    return hmod;
}

static void* OS_dlsym(void* hdl, const char* name)
{
    HMODULE hmod = hdl;
    FARPROC ptr = GetProcAddress(hmod, name);
    void* result = NULL;
    memcpy(&result, &ptr, sizeof(result));
    return result;
}

static void OS_dlclose(void* hdl)
{
    HMODULE hmod = hdl;
    FreeLibrary(hmod);
}

static void OS_geterror(char* buffer, i32 size)
{
    buffer[0] = 0;

    DWORD err = GetLastError();
    if (err != 0)
    {
        DWORD rv = FormatMessageA(
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            err,
            MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
            buffer,
            size,
            NULL);
        ASSERT((i32)rv < size);
        SetLastError(0);
    }
}

#else
// POSIX API

#include <dlcfn.h>
#include <errno.h>

static void* OS_dlopen(const char* name)
{
    void* handle = dlopen(name, RTLD_LAZY);
    return handle;
}

static void* OS_dlsym(void* hdl, const char* name)
{
    void* ptr = dlsym(hdl, name);
    return ptr;
}

static void OS_dlclose(void* hdl)
{
    dlclose(hdl);
}

static void OS_geterror(char* buffer, i32 size)
{
    buffer[0] = 0;

    char* src = strerror(errno);
    if (src)
    {
        StrCpy(buffer, size, src);
    }
}

#endif // PLAT_WINDOWS
