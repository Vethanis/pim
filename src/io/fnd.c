#include "io/fnd.h"

bool Finder_IsOpen(Finder* fdr)
{
#if PLAT_WINDOWS
    return fdr->handle != -1;
#else
    return fdr->open;
#endif // PLAT_WINDOWS
}

bool Finder_Iterate(Finder* fdr, FinderData* data, const char* spec)
{
    ASSERT(fdr);
    if (Finder_IsOpen(fdr))
    {
        if (Finder_Next(fdr, data))
        {
            return true;
        }
        Finder_End(fdr);
        return false;
    }
    else
    {
        return Finder_Begin(fdr, data, spec);
    }
}

#if PLAT_WINDOWS
// ----------------------------------------------------------------------------
// Windows

#include <io.h>

bool Finder_Begin(Finder* fdr, FinderData* data, const char* spec)
{
    ASSERT(fdr);
    ASSERT(data);
    ASSERT(spec);
    fdr->handle = _findfirst64(spec, (struct __finddata64_t*)data);
    return Finder_IsOpen(fdr);
}

bool Finder_Next(Finder* fdr, FinderData* data)
{
    ASSERT(fdr);
    ASSERT(data);
    if (Finder_IsOpen(fdr))
    {
        if (!_findnext64(fdr->handle, (struct __finddata64_t*)data))
        {
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

bool Finder_Begin(Finder* fdr, FinderData* data, const char* spec)
{
    ASSERT(fdr);
    ASSERT(data);
    ASSERT(spec);

    fdr->open = (glob(spec, 0, NULL, (glob_t*)(&fdr->glob)) == 0);
    if (fdr->open)
    {
        data->name = fdr->glob.gl_pathv[0];
    }

    return fdr->open;
}

bool Finder_Next(Finder* fdr, FinderData* data)
{
    ASSERT(fdr);
    ASSERT(data);
    if (++fdr->index >= fdr->glob.gl_pathc)
    {
        return false;
    }

    data->name = fdr->glob.gl_pathv[fdr->index];
    return true;
}

void Finder_End(Finder* fdr)
{
    ASSERT(fdr);
    fdr->open = false;
    globfree((glob_t*)(&fdr->glob));
}

#endif // PLAT_X
