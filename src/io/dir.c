#include "io/dir.h"

pim_inline void* NotNull(void* x)
{
    ASSERT(x != NULL);
    return x;
}

pim_inline i32 IsZero(i32 x)
{
    ASSERT(x == 0);
    return x;
}

static char* GetCwd(char* dst, i32 size);
static i32 ChDir(const char* path);
static i32 RmDir(const char* path);
static i32 ChMod(const char* filename, i32 mode);
static i32 MkDir(const char* path);

bool IO_GetCwd(char* dst, i32 size)
{
    ASSERT(dst);
    ASSERT(size > 0);
    return NotNull(GetCwd(dst, size - 1)) != NULL;
}

bool IO_ChDir(const char* path)
{
    ASSERT(path);
    return IsZero(ChDir(path)) == 0;
}

bool IO_MkDir(const char* path)
{
    ASSERT(path);
    return MkDir(path) == 0;
}

bool IO_RmDir(const char* path)
{
    ASSERT(path);
    return IsZero(RmDir(path)) == 0;
}

bool IO_ChMod(const char* path, i32 flags)
{
    ASSERT(path);
    return IsZero(ChMod(path, flags)) == 0;
}

#if PLAT_WINDOWS
#include <direct.h>
#include <io.h>

static char* GetCwd(char* dst, i32 size)
{
    return _getcwd(dst, size);
}
static i32 ChDir(const char* path)
{
    return _chdir(path);
}
static i32 RmDir(const char* path)
{
    return _rmdir(path);
}
static i32 ChMod(const char* filename, i32 mode)
{
    return _chmod(filename, mode);
}
static i32 MkDir(const char* path)
{
    return _mkdir(path);
}

#else
#include <unistd.h>
#include <sys/stat.h>

static char* GetCwd(char* dst, i32 size)
{
    return getcwd(dst, size);
}
static i32 ChDir(const char* path)
{
    return chdir(path);
}
static i32 RmDir(const char* path)
{
    return rmdir(path);
}
static i32 ChMod(const char* filename, i32 mode)
{
    return chmod(filename, mode);
}
static i32 MkDir(const char* path)
{
    return mkdir(path, 0755);
}

#endif // PLAT_XXX
