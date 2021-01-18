#include "assets/vfile.h"
#include "allocator/allocator.h"
#include "common/stringutil.h"
#include "containers/queue_i32.h"
#include <string.h>

typedef struct subfile_s
{
    vfilehdl_t owner;
    i32 offset;
    i32 size;
} subfile_t;

typedef struct vfile_s
{
    vfiletype_t type;
    i32 cursor;
    union
    {
        fd_t descriptor;
        fstr_t stream;
        fmap_t map;
        subfile_t subfile;
    };
} vfile_t;

static queue_i32_t ms_freelist;
static i32 ms_count;
static u8* ms_versions;
static char** ms_paths;
static vfile_t* ms_files;

static vfilehdl_t vfilehdl_new(vfile_t* file, char* path)
{
    ASSERT(file->type != vfiletype_null);
    ASSERT(path && path[0]);
    i32 index = 0;
    if (!queue_i32_trypop(&ms_freelist, &index))
    {
        i32 len = ++ms_count;
        PermReserve(ms_versions, len);
        PermReserve(ms_paths, len);
        PermReserve(ms_files, len);
        index = len - 1;
        ms_versions[index] = 0;
    }
    u8 version = ++ms_versions[index];
    ASSERT(version & 1);
    ms_paths[index] = path;
    ms_files[index] = *file;
    vfilehdl_t hdl = { 0 };
    hdl.version = version;
    hdl.index = index;
    return hdl;
}

static bool vfilehdl_del(vfilehdl_t hdl)
{
    if (vfile_exists(hdl))
    {
        i32 index = hdl.index;
        u8 version = ++ms_versions[index];
        ASSERT(!(version & 1));
        queue_i32_push(&ms_freelist, index);
        return true;
    }
    return false;
}

bool vfile_exists(vfilehdl_t hdl)
{
    i32 i = hdl.index;
    u8 v = hdl.version;
    return (i < ms_count) && (v == ms_versions[i]);
}

bool vfile_isopen(vfilehdl_t hdl)
{
    if (vfile_exists(hdl))
    {
        vfile_t* vf = &ms_files[hdl.index];
        switch (vf->type)
        {
        default:
            ASSERT(false);
            break;
        case vfiletype_null:
            break;
        case vfiletype_descriptor:
            return fd_isopen(vf->descriptor);
        case vfiletype_stream:
            return fstr_isopen(vf->stream);
        case vfiletype_map:
            return fmap_isopen(vf->map);
        case vfiletype_subfile:
            return vfile_isopen(vf->subfile.owner);
        }
    }
    return false;
}

vfilehdl_t vfile_new(const char* path, const char* mode, vfiletype_t type)
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
    case vfiletype_descriptor:
    {
        vf.descriptor = fd_open(path, writable);
        opened = fd_isopen(vf.descriptor);
    }
    break;
    case vfiletype_stream:
    {
        vf.stream = fstr_open(path, mode);
        opened = fstr_isopen(vf.stream);
    }
    break;
    case vfiletype_map:
    {
        vf.map = fmap_open(path, writable);
        opened = fmap_isopen(vf.map);
    }
    break;
    }
    if (opened)
    {
        return vfilehdl_new(&vf, StrDup(path, EAlloc_Perm));
    }
    return (vfilehdl_t) { 0 };
}

bool vfile_del(vfilehdl_t hdl)
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
        case vfiletype_descriptor:
            fd_close(&vf->descriptor);
            break;
        case vfiletype_stream:
            fstr_close(&vf->stream);
            break;
        case vfiletype_map:
            fmap_close(&vf->map);
            break;
        case vfiletype_subfile:
            break;
        }
        memset(vf, 0, sizeof(*vf));
        pim_free(*pPath);
        *pPath = NULL;
        return true;
    }
    return false;
}

i32 vfile_size(vfilehdl_t hdl)
{
    if (vfile_isopen(hdl))
    {
        vfile_t* vf = &ms_files[hdl.index];
        switch (vf->type)
        {
        default:
            ASSERT(false);
            break;
        case vfiletype_descriptor:
            return (i32)fd_size(vf->descriptor);
        case vfiletype_stream:
            return (i32)fstr_size(vf->stream);
        case vfiletype_map:
            return vf->map.size;
        case vfiletype_subfile:
            return vf->subfile.size;
        }
    }
    return 0;
}

i32 vfile_read(vfilehdl_t hdl, i32 offset, i32 size, void* dst)
{
    ASSERT(offset >= 0);
    ASSERT(size >= 0);
    ASSERT(dst || !size);
    if (!vfile_isopen(hdl))
    {
        return 0;
    }
    vfile_t* vf = &ms_files[hdl.index];
    switch (vf->type)
    {
    default:
        ASSERT(false);
        return 0;
    case vfiletype_descriptor:
    {
        if (vf->cursor != offset)
        {
            fd_seek(vf->descriptor, offset);
            vf->cursor = offset;
        }
        return fd_read(vf->descriptor, dst, size);
    }
    case vfiletype_stream:
    {
        if (vf->cursor != offset)
        {
            fstr_seek(vf->stream, offset);
            vf->cursor = offset;
        }
        return fstr_read(vf->stream, dst, size);
    }
    break;
    case vfiletype_map:
    {
        ASSERT((offset + size) <= vf->map.size);
        const u8* src = vf->map.ptr;
        memcpy(dst, src + offset, size);
        return size;
    }
    break;
    case vfiletype_subfile:
    {
        ASSERT((offset + size) <= vf->subfile.size);
        return vfile_read(vf->subfile.owner, offset + vf->subfile.offset, size, dst);
    }
    break;
    }
}

i32 vfile_write(vfilehdl_t hdl, i32 offset, i32 size, const void* src)
{
    if (!vfile_isopen(hdl))
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
    case vfiletype_descriptor:
    {
        if (vf->cursor != offset)
        {
            fd_seek(vf->descriptor, offset);
            vf->cursor = offset;
        }
        return fd_write(vf->descriptor, src, size);
    }
    case vfiletype_stream:
    {
        if (vf->cursor != offset)
        {
            fstr_seek(vf->stream, offset);
            vf->cursor = offset;
        }
        return fstr_write(vf->stream, src, size);
    }
    break;
    case vfiletype_map:
    {
        ASSERT((offset + size) <= vf->map.size);
        u8* dst = vf->map.ptr;
        memcpy(dst + offset, src, size);
        return size;
    }
    break;
    case vfiletype_subfile:
    {
        ASSERT((offset + size) <= vf->subfile.size);
        return vfile_write(vf->subfile.owner, offset + vf->subfile.offset, size, src);
    }
    break;
    }
}

vfilehdl_t subfile_new(vfilehdl_t owner, i32 offset, i32 size, const char* name)
{
    ASSERT(offset >= 0);
    ASSERT(size >= 0);
    ASSERT(name);
    ASSERT((offset + size) <= vfile_size(owner));
    if (vfile_isopen(owner))
    {
        vfile_t vf =
        {
            .type = vfiletype_subfile,
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
    return (vfilehdl_t) { 0 };
}
