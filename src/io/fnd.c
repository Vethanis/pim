#include "io/fnd.h"

#include "io/fd.h"
#include "common/stringutil.h"
#include <string.h>

bool Finder_IsOpen(Finder* fdr)
{
    return fdr->handle != NULL;
}

bool Finder_Iterate(Finder* fdr, FinderData* data, const char* path)
{
    ASSERT(fdr);
    if (Finder_IsOpen(fdr))
    {
        if (Finder_Next(fdr, data, path))
        {
            return true;
        }
        Finder_End(fdr);
        return false;
    }
    else
    {
        return Finder_Begin(fdr, data, path);
    }
}

#if PLAT_WINDOWS
// ----------------------------------------------------------------------------
// Windows

#include <io.h>

typedef struct __finddata64_t FinderDataRaw;

// translate windows API data to pim API data
static void Finder_Translate(FinderData* dst, const FinderDataRaw* src, const char* path)
{
    memset(dst, 0, sizeof(*dst));
    SPrintf(ARGS(dst->path), "%s/%s", path, src->name);
    StrPath(ARGS(dst->path));
    dst->size = src->size;
    dst->accessTime = src->time_access;
    dst->modifyTime = src->time_write;
    dst->createTime = src->time_create;
    dst->isNormal = (src->attrib == _A_NORMAL) ? 1 : 0;
    dst->isReadOnly = (src->attrib & _A_RDONLY) ? 1 : 0;
    dst->isHidden = (src->attrib & _A_HIDDEN) ? 1 : 0;
    dst->isSystem = (src->attrib & _A_SYSTEM) ? 1 : 0;
    dst->isFolder = (src->attrib & _A_SUBDIR) ? 1 : 0;
    dst->isArchive = (src->attrib & _A_ARCH) ? 1 : 0;
}

bool Finder_Begin(Finder* fdr, FinderData* data, const char* path)
{
    ASSERT(!Finder_IsOpen(fdr));
    ASSERT(path);
    memset(data, 0, sizeof(*data));
    fdr->handle = 0;

    char spec[PIM_PATH];
    // Win32 wildcard syntax, "*" just lists all files and folders. is not regex support.
    SPrintf(ARGS(spec), "%s/*", path);
    StrPath(ARGS(spec));

    FinderDataRaw rawData = { 0 };
    isize handle = _findfirst64(spec, &rawData);
    if (handle != -1)
    {
        fdr->handle = (void*)(handle + 1);
        Finder_Translate(data, &rawData, path);
        ASSERT(Finder_IsOpen(fdr));
        return true;
    }
    return false;
}

bool Finder_Next(Finder* fdr, FinderData* data, const char* path)
{
    memset(data, 0, sizeof(*data));
    if (Finder_IsOpen(fdr))
    {
        isize handle = (isize)(fdr->handle) - 1;
        ASSERT(handle != -1);
        FinderDataRaw rawData = { 0 };
        if (_findnext64(handle, &rawData) == 0)
        {
            Finder_Translate(data, &rawData, path);
            return true;
        }
    }
    return false;
}

void Finder_End(Finder* fdr)
{
    if (Finder_IsOpen(fdr))
    {
        isize handle = (isize)(fdr->handle) - 1;
        ASSERT(handle != -1);
        _findclose(handle);
        fdr->handle = NULL;
    }
}

#else
// ----------------------------------------------------------------------------
// POSIX

#include <dirent.h>

typedef struct dirent FinderDataRaw;

// https://www.gnu.org/software/libc/manual/html_node/Directory-Entries.html
static void Finder_Translate(FinderData* dst, const FinderDataRaw* src, const char* path)
{
    memset(dst, 0, sizeof(*dst));

    SPrintf(ARGS(dst->path), "%s/%s", path, src->d_name);
    StrPath(ARGS(dst->path));

    {
        fd_t fd = fd_open(dst->path, false);
        if (fd_isopen(fd))
        {
            fd_status_t status = { 0 };
            fd_stat(fd, &status);
            dst->size = status.size;
            dst->accessTime = status.accessTime;
            dst->modifyTime = status.modifyTime;
            dst->createTime = status.createTime;
            fd_close(&fd);
        }
    }

#ifdef _DIRENT_HAVE_D_TYPE
    u8 d_type = src->d_type;
    switch (d_type)
    {
    default:
        ASSERT(false); // unhandled DT? is it a bitfield or an enum?
        break;
    case DT_UNKNOWN:
        break;
    case DT_REG:
        dst->isNormal = 1;
        break;
    case DT_DIR:
        dst->isFolder = 1;
        break;
    case DT_FIFO:
        // named pipe or FIFO
        break;
    case DT_SOCK:
        // local-domain socket
        break;
    case DT_CHR:
        // character device
        break;
    case DT_BLK:
        // block device
        break;
    case DT_LNK:
        // symbolic link
        break;
    }
#endif // _DIRENT_HAVE_D_TYPE

}

bool Finder_Begin(Finder* fdr, FinderData* data, const char* path)
{
    ASSERT(path);
    ASSERT(!Finder_IsOpen(fdr));
    memset(data, 0, sizeof(*data));
    DIR* dp = opendir(path);
    if (dp)
    {
        const FinderDataRaw* entry = readdir(dp);
        if (entry)
        {
            Finder_Translate(data, entry, path);
        }
        else
        {
            closedir(dp); dp = NULL;
        }
    }
    fdr->handle = dp;
    return Finder_IsOpen(fdr);
}

bool Finder_Next(Finder* fdr, FinderData* data, const char* path)
{
    ASSERT(path);
    memset(data, 0, sizeof(*data));
    DIR* dp = fdr->handle;
    if (dp)
    {
        const FinderDataRaw* entry = readdir(dp);
        if (entry)
        {
            Finder_Translate(data, entry, path);
        }
        else
        {
            closedir(dp); dp = NULL;
        }
    }
    fdr->handle = dp;
    return Finder_IsOpen(fdr);
}

void Finder_End(Finder* fdr)
{
    ASSERT(fdr);
    DIR* dp = fdr->handle;
    fdr->handle = NULL;
    if (dp)
    {
        closedir(dp);
    }
}

#endif // PLAT_X
