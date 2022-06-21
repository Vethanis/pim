#include "io/fnd.h"

bool Finder_IsOpen(Finder* fdr)
{
#if PLAT_WINDOWS
    return fdr->handle != -1;
#else
    return fdr->open;
#endif // PLAT_WINDOWS
}

bool Finder_Iterate(Finder* fdr, const char* path, const char* wildcard)
{
    ASSERT(fdr);
    if (Finder_IsOpen(fdr))
    {
        if (Finder_Next(fdr))
        {
            return true;
        }
        Finder_End(fdr);
        return false;
    }
    else
    {
        return Finder_Begin(fdr, path, wildcard);
    }
}

#if PLAT_WINDOWS
// ----------------------------------------------------------------------------
// Windows

#include <io.h>

bool Finder_Begin(Finder* fdr, const char* path, const char* wildcard)
{
    ASSERT(fdr);
    ASSERT(path);
    ASSERT(wildcard);
    char spec[PIM_PATH];

    StrCpy(ARGS(fdr->path), path);
    SPrintf(ARGS(spec), "%s\\%s", path, wildcard);
    StrPath(ARGS(spec));

    fdr->path = path;
    fdr->handle = _findfirst64(spec, (struct __finddata64_t*)fdr->data);
    return Finder_IsOpen(fdr);
}

bool Finder_Next(Finder* fdr)
{
    ASSERT(fdr);
    if (Finder_IsOpen(fdr))
    {
        if (!_findnext64(fdr->handle, (struct __finddata64_t*)fdr->data))
        {
            SPrintf(ARGS(fdr->relpath), "%s\\%s", fdr->path, fdr->data.name)
            StrPath(ARGS(fdr->relpath));
            return true;
        }
    }
    return false;
}

void Finder_End(Finder* fdr)
{
    ASSERT(fdr);
    if (Finder_IsOpen(fdr))
    {
        _findclose(fdr->handle);
        fdr->handle = -1;
    }
}

#else
// ----------------------------------------------------------------------------
// POSIX

#include <glob.h>

bool Finder_Begin(Finder* fdr, const char* path, const char* wildcard)
{
    ASSERT(fdr);
    ASSERT(path);
    ASSERT(wildcard);
    char spec[PIM_PATH];

    SPrintf(ARGS(spec), "%s/%s", path, wildcard);
    StrPath(ARGS(spec));

    fdr->open = (glob(spec, 0, NULL, (glob_t*)(&fdr->glob)) == 0);
    if (fdr->open)
    {
        ASSERT(fdr->glob.gl_pathc > 0);
        fdr->relPath = fdr->glob.gl_pathv[0];
    }

    return fdr->open;
}

bool Finder_Next(Finder* fdr)
{
    ASSERT(fdr);
    if (++fdr->index >= fdr->glob.gl_pathc)
    {
        return false;
    }

    fdr->relPath = fdr->glob.gl_pathv[fdr->index];
    return true;
}

void Finder_End(Finder* fdr)
{
    ASSERT(fdr);
    fdr->open = false;
    globfree((glob_t*)(&fdr->glob));
}

#endif // PLAT_X
