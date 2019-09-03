#pragma once

#include "common/int_types.h"
#include "containers/array.h"

#define ROOT_DIR        ".."
#define FS_MAX_PATH     256

namespace Fs
{
    struct Attributes;
    struct Info;
    struct Results;
    struct Stream;

    struct Attributes
    {
        u32 readonly : 1;
        u32 directory : 1;
        u32 archive : 1;
        u32 compressed : 1;
        u32 encrypted : 1;
        u32 hidden : 1;
        u32 normal : 1;
        u32 sparse : 1;
        u32 system : 1;
        u32 temporary : 1;
    };

    struct Info
    {
        char path[FS_MAX_PATH];
        cstr extension;
        Attributes attributes;
        u64 size;
        u64 createTime;
        u64 lastReadTime;
        u64 lastWriteTime;
    };

    struct Results
    {
        Array<Info> files;
        Array<Info> directories;

        inline void Init(AllocatorType type)
        {
            files.init(type);
            directories.init(type);
        }
        inline void Clear()
        {
            files.clear();
            directories.clear();
        }
        inline void Reset()
        {
            files.reset();
            directories.reset();
        }
    };

    bool CreateFile(cstr path);
    bool CreateDir(cstr path);
    bool RemoveFile(cstr path);
    bool RemoveDir(cstr path);
    bool Find(cstr path, Results& results);
    inline bool GetChildren(const Info& info, Results& results)
    {
        if (info.attributes.directory)
        {
            return Find(info.path, results);
        }
        return false;
    }
};
