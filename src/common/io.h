#pragma once

#include "common/macro.h"

PIM_C_BEGIN

#include <stdint.h>

int32_t pim_errno(void);

// file descriptor
typedef int32_t fd_t;
static int32_t fd_isopen(fd_t hdl) { return hdl >= 0; }

#define fd_stdin  0
#define fd_stdout 1
#define fd_stderr 2

typedef enum
{
    StMode_Regular = 0x8000,
    StMode_Dir = 0x4000,
    StMode_SpecChar = 0x2000,
    StMode_Pipe = 0x1000,
    StMode_Read = 0x0100,
    StMode_Write = 0x0080,
    StMode_Exec = 0x0040,
} StModeFlags;

// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fstat-fstat32-fstat64-fstati64-fstat32i64-fstat64i32
typedef struct fd_status_s
{
    uint32_t    st_dev;
    uint16_t    st_ino;
    uint16_t    st_mode;
    int16_t     st_nlink;
    int16_t     st_uid;
    int16_t     st_guid;
    uint32_t    st_rdev;
    int64_t     st_size;
    int64_t     st_atime;
    int64_t     st_mtime;
    int64_t     st_ctime;
} fd_status_t;

// creat
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/creat-wcreat
fd_t pim_create(const char* filename);

// open
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/open-wopen
fd_t pim_open(const char* filename, int32_t writable);

// close
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/close
void pim_close(fd_t* hdl);

// read
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/read
int32_t pim_read(fd_t hdl, void* dst, int32_t size);

// write
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/write
int32_t pim_write(fd_t hdl, const void* src, int32_t size);

int32_t pim_puts(const char* str, fd_t hdl);

int32_t pim_printf(fd_t hdl, const char* fmt, ...);

// seek
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/lseek-lseeki64
int32_t pim_seek(fd_t hdl, int32_t offset);

// tell
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/tell-telli64
int32_t pim_tell(fd_t hdl);

// pipe
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/pipe
void pim_pipe(fd_t* p0, fd_t* p1, int32_t bufferSize);

// fstat
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fstat-fstat32-fstat64-fstati64-fstat32i64-fstat64i32
void pim_stat(fd_t hdl, fd_status_t* status);

// fstat.st_size
static int64_t fd_size(fd_t fd)
{
    fd_status_t status;
    pim_stat(fd, &status);
    return status.st_size;
}

// ------------------------------------------------------------------------

// buffered file stream
typedef void* fstr_t;

// fopen
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fopen-wfopen
fstr_t pim_fopen(const char* filename, const char* mode);

// fclose
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fclose-fcloseall
void pim_fclose(fstr_t* stream);

// fflush
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fflush
void pim_fflush(fstr_t stream);

// fread
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fread
int32_t pim_fread(fstr_t stream, void* dst, int32_t size);

// fwrite
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fwrite
int32_t pim_fwrite(fstr_t stream, const void* src, int32_t size);

// fgets
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fgets-fgetws
void pim_fgets(fstr_t stream, char* dst, int32_t size);

// fputs
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fputs-fputws
int32_t pim_fputs(fstr_t stream, const char* src);

// fileno
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fileno
fd_t pim_fileno(fstr_t stream);

// fdopen
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/fdopen-wfdopen
fstr_t pim_fdopen(fd_t* hdl, const char* mode);

// fseek SEEK_SET
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/fseek-lseek-constants
void pim_fseek(fstr_t stream, int32_t offset);

// ftell
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/ftell-ftelli64
int32_t pim_ftell(fstr_t stream);

// popen
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/popen-wpopen
fstr_t pim_popen(const char* cmd, const char* mode);

// pclose
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/pclose
void pim_pclose(fstr_t* stream);

// fstat
static void pim_fstat(fstr_t stream, fd_status_t* status)
{
    fd_t fd = pim_fileno(stream);
    pim_stat(fd, status);
}

// fstat.st_size
static int64_t pim_fsize(fstr_t stream)
{
    fd_status_t status;
    pim_fstat(stream, &status);
    return status.st_size;
}

// ------------------------------------------------------------------------
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/directory-control

// getcwd
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/getcwd-wgetcwd
void pim_getcwd(char* dst, int32_t size);

// chdir
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/chdir-wchdir
void pim_chdir(const char* path);

// mkdir
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/mkdir-wmkdir
void pim_mkdir(const char* path);

// rmdir
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/rmdir-wrmdir
void pim_rmdir(const char* path);

typedef enum
{
    ChMod_Read = 0x0100,
    ChMod_Write = 0x0080,
    ChMod_Exec = 0x0040,
} ChModFlags;

// chmod
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/chmod-wchmod
void pim_chmod(const char* path, int32_t flags);

// ------------------------------------------------------------------------

// searchenv
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/searchenv-wsearchenv
void pim_searchenv(const char* filename, const char* varname, char* dst);

// getenv
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/getenv-wgetenv
const char* pim_getenv(const char* varname);

// putenv
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/putenv-wputenv
void pim_putenv(const char* varname, const char* value);

// ------------------------------------------------------------------------

typedef intptr_t fnd_t;
static int32_t fnd_isopen(fnd_t fdr) { return fdr != -1; }

typedef enum
{
    FAF_Normal = 0x00,
    FAF_ReadOnly = 0x01,
    FAF_Hidden = 0x02,
    FAF_System = 0x04,
    FAF_SubDir = 0x10,
    FAF_Archive = 0x20,
} FileAttrFlags;

// __finddata64_t, do not modify
typedef struct fnd_data_s
{
    uint32_t attrib;
    int64_t time_create;
    int64_t time_access;
    int64_t time_write;
    int64_t size;
    char name[260];
} fnd_data_t;

int32_t pim_fndfirst(fnd_t* fdr, fnd_data_t* data, const char* spec);
int32_t pim_fndnext(fnd_t* fdr, fnd_data_t* data);
void pim_fndclose(fnd_t* fdr);

// Begin + Next + Close
int32_t pim_fnditer(fnd_t* fdr, fnd_data_t* data, const char* spec);

// ------------------------------------------------------------------------

typedef struct fmap_s
{
    void* ptr;
    int32_t size;
} fmap_t;

fmap_t pim_mmap(fd_t fd, int32_t writable);
void pim_munmap(fmap_t* map);
void pim_msync(fmap_t map);

PIM_C_END
