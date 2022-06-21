#include "assets/vfile.h"
#include "allocator/allocator.h"
#include "common/stringutil.h"
#include "containers/queue_i32.h"
#include <string.h>

typedef struct subfile_s
{
    VFileHdl owner;
    i32 offset;
    i32 size;
} subfile_t;

typedef struct vfile_s
{
    VFileType type;
    i32 cursor;
    union
    {
        fd_t descriptor;
        FStream stream;
        FileMap map;
        subfile_t subfile;
    };
} vfile_t;

static IntQueue ms_freelist;
static i32 ms_count;
static u8* ms_versions;
static char** ms_paths;
static vfile_t* ms_files;

static VFileHdl vfilehdl_new(vfile_t* file, char* path)
{
    ASSERT(file->type != VFileType_Null);
    ASSERT(path && path[0]);
    i32 index = 0;
    if (!IntQueue_TryPop(&ms_freelist, &index))
    {
        i32 len = ++ms_count;
        Perm_Reserve(ms_versions, len);
        Perm_Reserve(ms_paths, len);
        Perm_Reserve(ms_files, len);
        index = len - 1;
        ms_versions[index] = 0;
    }
    u8 version = ++ms_versions[index];
    ASSERT(version & 1);
    ms_paths[index] = path;
    ms_files[index] = *file;
    VFileHdl hdl = { 0 };
    hdl.version = version;
    hdl.index = index;
    return hdl;
}

static bool vfilehdl_del(VFileHdl hdl)
{
    if (VFile_Exists(hdl))
    {
        i32 index = hdl.index;
        ++ms_versions[index];
        ASSERT(!(ms_versions[index] & 1));
        IntQueue_Push(&ms_freelist, index);
        return true;
    }
    return false;
}

bool VFile_Exists(VFileHdl hdl)
{
    i32 i = hdl.index;
    u8 v = hdl.version;
    return (i < ms_count) && (v == ms_versions[i]);
}

bool VFile_IsOpen(VFileHdl hdl)
{
    if (VFile_Exists(hdl))
    {
        vfile_t* vf = &ms_files[hdl.index];
        switch (vf->type)
        {
        default:
            ASSERT(false);
            break;
        case VFileType_Null:
            break;
        case VFileType_Descriptor:
            return fd_isopen(vf->descriptor);
        case VFileType_Stream:
            return FStream_IsOpen(vf->stream);
        case VFileType_Map:
            return FileMap_IsOpen(&vf->map);
        case VFileType_SubFile:
            return VFile_IsOpen(vf->subfile.owner);
        }
    }
    return false;
}

VFileHdl VFile_New(const char* path, const char* mode, VFileType type)
{
    ASSERT(path && path[0]);
    ASSERT(mode && mode[0]);
    vfile_t vf = { 0 };
    vf.type = type;
    bool opened = false;
    bool writable = StrIChr(mode, 8, 'w') || StrIChr(mode, 4, '+');
    switch (type)
    {
    default:
        ASSERT(false);
        break;
    case VFileType_Descriptor:
    {
        vf.descriptor = fd_open(path, writable);
        opened = fd_isopen(vf.descriptor);
    }
    break;
    case VFileType_Stream:
    {
        vf.stream = FStream_Open(path, mode);
        opened = FStream_IsOpen(vf.stream);
    }
    break;
    case VFileType_Map:
    {
        vf.map = FileMap_Open(path, writable);
        opened = FileMap_IsOpen(&vf.map);
    }
    break;
    }
    if (opened)
    {
        return vfilehdl_new(&vf, StrDup(path, EAlloc_Perm));
    }
    return (VFileHdl) { 0 };
}

bool VFile_Del(VFileHdl hdl)
{
    if (vfilehdl_del(hdl))
    {
        i32 index = hdl.index;
        vfile_t* vf = &ms_files[index];
        char** pPath = &ms_paths[index];
        switch (vf->type)
        {
        default:
            ASSERT(false);
            break;
        case VFileType_Descriptor:
            fd_close(&vf->descriptor);
            break;
        case VFileType_Stream:
            FStream_Close(&vf->stream);
            break;
        case VFileType_Map:
            FileMap_Close(&vf->map);
            break;
        case VFileType_SubFile:
            break;
        }
        memset(vf, 0, sizeof(*vf));
        Mem_Free(*pPath);
        *pPath = NULL;
        return true;
    }
    return false;
}

i32 VFile_Size(VFileHdl hdl)
{
    if (VFile_IsOpen(hdl))
    {
        vfile_t* vf = &ms_files[hdl.index];
        switch (vf->type)
        {
        default:
            ASSERT(false);
            break;
        case VFileType_Descriptor:
            return (i32)fd_size(vf->descriptor);
        case VFileType_Stream:
            return (i32)FStream_Size(vf->stream);
        case VFileType_Map:
            return vf->map.size;
        case VFileType_SubFile:
            return vf->subfile.size;
        }
    }
    return 0;
}

i32 VFile_Read(VFileHdl hdl, i32 offset, i32 size, void* dst)
{
    ASSERT(offset >= 0);
    ASSERT(size >= 0);
    ASSERT(dst || !size);
    if (!VFile_IsOpen(hdl))
    {
        return 0;
    }
    vfile_t* vf = &ms_files[hdl.index];
    switch (vf->type)
    {
    default:
        ASSERT(false);
        return 0;
    case VFileType_Descriptor:
    {
        if (vf->cursor != offset)
        {
            fd_seek(vf->descriptor, offset);
            vf->cursor = offset;
        }
        return fd_read(vf->descriptor, dst, size);
    }
    case VFileType_Stream:
    {
        if (vf->cursor != offset)
        {
            FStream_Seek(vf->stream, offset);
            vf->cursor = offset;
        }
        return FStream_Read(vf->stream, dst, size);
    }
    break;
    case VFileType_Map:
    {
        ASSERT((offset + size) <= vf->map.size);
        const u8* src = vf->map.ptr;
        memcpy(dst, src + offset, size);
        return size;
    }
    break;
    case VFileType_SubFile:
    {
        ASSERT((offset + size) <= vf->subfile.size);
        return VFile_Read(vf->subfile.owner, offset + vf->subfile.offset, size, dst);
    }
    break;
    }
}

i32 VFile_Write(VFileHdl hdl, i32 offset, i32 size, const void* src)
{
    if (!VFile_IsOpen(hdl))
    {
        return 0;
    }
    ASSERT(offset >= 0);
    ASSERT(size >= 0);
    ASSERT(src || !size);
    vfile_t* vf = &ms_files[hdl.index];
    switch (vf->type)
    {
    default:
        ASSERT(false);
        return 0;
    case VFileType_Descriptor:
    {
        if (vf->cursor != offset)
        {
            fd_seek(vf->descriptor, offset);
            vf->cursor = offset;
        }
        return fd_write(vf->descriptor, src, size);
    }
    case VFileType_Stream:
    {
        if (vf->cursor != offset)
        {
            FStream_Seek(vf->stream, offset);
            vf->cursor = offset;
        }
        return FStream_Write(vf->stream, src, size);
    }
    break;
    case VFileType_Map:
    {
        ASSERT((offset + size) <= vf->map.size);
        u8* dst = vf->map.ptr;
        memcpy(dst + offset, src, size);
        return size;
    }
    break;
    case VFileType_SubFile:
    {
        ASSERT((offset + size) <= vf->subfile.size);
        return VFile_Write(vf->subfile.owner, offset + vf->subfile.offset, size, src);
    }
    break;
    }
}

VFileHdl SubFile_New(VFileHdl owner, i32 offset, i32 size, const char* name)
{
    ASSERT(offset >= 0);
    ASSERT(size >= 0);
    ASSERT(name);
    ASSERT((offset + size) <= VFile_Size(owner));
    if (VFile_IsOpen(owner))
    {
        vfile_t vf =
        {
            .type = VFileType_SubFile,
            .subfile =
            {
                .owner = owner,
                .offset = offset,
                .size = size,
            },
        };

        char path[PIM_PATH];
        StrCpy(ARGS(path), ms_paths[owner.index]);
        char* lastDot = (char*)StrRChr(ARGS(path), '.');
        if (lastDot)
        {
            *lastDot = 0;
        }
        StrCat(ARGS(path), "/");
        StrCat(ARGS(path), name);
        StrPath(ARGS(path));

        return vfilehdl_new(&vf, StrDup(path, EAlloc_Perm));
    }
    return (VFileHdl) { 0 };
}
