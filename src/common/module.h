#pragma once

// pim code modules:
// implement and export this function in your dll:
// const your_module_t* your_module_export(void);

#include "common/macro.h"

PIM_C_BEGIN

typedef const void* (PIM_CDECL *pimod_export_t)(void);

i32 PIM_CDECL pimod_get(const char* name, void* dst, i32 bytes);
i32 PIM_CDECL pimod_release(const char* name);

void* PIM_CDECL pimod_dlopen(const char* name);
void  PIM_CDECL pimod_dlclose(void* dll);
void* PIM_CDECL pimod_dlsym(void* dll, const char* name);

PIM_C_END

#ifdef MODULE_IMPL

#include "common/hashstring.h"
#include "common/stringutil.h"
#include "common/pimcpy.h"

#define kMaxModules 256

static i32 ms_count;
static u32 ms_hashes[kMaxModules];
static void* ms_dlls[kMaxModules];
static const void* ms_modules[kMaxModules];

static i32 pimod_import(const char* name, u32 hash)
{
    void* dll = pimod_dlopen(name);
    if (!dll)
    {
        return -1;
    }

    char symname[256];
    SPrintf(ARGS(symname), "%s_export", name);
    symname[NELEM(symname) - 1] = 0;

    void* sym = pimod_dlsym(dll, symname);
    if (!sym)
    {
        pimod_dlclose(dll);
        return -1;
    }

    pimod_export_t exporter = 0;
    *(void**)(&exporter) = sym;
    const void* mod = exporter();
    if (!mod)
    {
        pimod_dlclose(dll);
        return -1;
    }

    ASSERT(ms_count < kMaxModules);
    i32 i = ms_count++;
    ms_hashes[i] = hash;
    ms_dlls[i] = dll;
    ms_modules[i] = mod;

    return i;
}

i32 PIM_CDECL pimod_get(const char* name, void* dst, i32 bytes)
{
    ASSERT(name);
    ASSERT(dst);
    ASSERT(bytes > 0);
    const u32 hash = HashStr(name);
    i32 i = HashFind(ms_hashes, ms_count, hash);
    if (i == -1)
    {
        i = pimod_import(name, hash);
    }
    if (i != -1)
    {
        const void* src = ms_modules[i];
        ASSERT(src);
        pimcpy(dst, src, bytes);
        return 1;
    }
    return 0;
}

i32 PIM_CDECL pimod_release(const char* name)
{
    ASSERT(name);
    const u32 hash = HashStr(name);
    const i32 i = HashFind(ms_hashes, ms_count, hash);
    if (i != -1)
    {
        const i32 back = --ms_count;
        ASSERT(back >= 0);
        void* dll = ms_dlls[i];
        ASSERT(dll);
        ms_hashes[i] = ms_hashes[back];
        ms_dlls[i] = ms_dlls[back];
        ms_modules[i] = ms_modules[back];
        pimod_dlclose(dll);
        return 1;
    }
    return 0;
}

#if PLAT_WINDOWS

#include <Windows.h>

void* PIM_CDECL pimod_dlopen(const char* name)
{
    HMODULE hmod = LoadLibraryA(name);
    return (void*)hmod;
}

void PIM_CDECL pimod_dlclose(void* dll)
{
    HMODULE hmod = (HMODULE)dll;
    FreeLibrary(hmod);
}

void* PIM_CDECL pimod_dlsym(void* dll, const char* name)
{
    HMODULE hmod = (HMODULE)dll;
    FARPROC proc = GetProcAddress(hmod, name);
    void* addr = *(void**)(&proc);
    return addr;
}

#else

#include <dlfcn.h>

// link with -ldl
// https://linux.die.net/man/3/dlopen

void* PIM_CDECL pimod_dlopen(const char* name)
{
    char libname[256];
    stbsp_sprintf(libname, "%s.so", name);
    libname[NELEM(libname) - 1] = 0;
    return dlopen(libname, RTLD_LAZY | RTLD_LOCAL);
}

void PIM_CDECL pimod_dlclose(void* dll)
{
    dlclose(dll);
}

void* PIM_CDECL pimod_dlsym(void* dll, const char* name)
{
    return dlsym(dll, name);
}

#endif // PLAT_WINDOWS

#endif // MODULE_IMPL
