#pragma once

#include "common/int_types.h"
#include "common/array.h"

#define ROOT_DIR        ".."
#define FS_MAX_PATH     260

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

    bool CreateFile(const char* path);
    bool CreateDir(const char* path);
    bool RemoveFile(const char* path);
    bool RemoveDir(const char* path);
    bool Find(const char* path, Results& results);
    inline bool GetChildren(const Info& info, Results& results)
    {
        results.Clear();
        if (info.attributes.directory)
        {
            return Find(info.path, results);
        }
        return false;
    }

    // Streams are always binary 'b'
    enum StreamMode : u32
    {
        SM_Overwrite    = 1u << 0,  // creates if file doesnt exist, overwrites file. 'r+' -> 'w+'
        SM_Read         = 1u << 1,  // read from file. 'r'
        SM_Write        = 1u << 2,  // write to file. 'w'
        SM_Commit       = 1u << 3,  // flushes go to disk, 'c' MS extension
        SM_NoCommit     = 1u << 4,  // reset commit state to flush to cache. 'n' MS extension
        SM_Sequential   = 1u << 5,  // optimize for sequential access 'S'
        SM_Random       = 1u << 6,  // optimize for random access 'R'
        SM_Temporary    = 1u << 7,  // avoid flushing to disk. 'T' MS extension
        SM_Delete       = 1u << 8,  // delete after last ref is closed. 'D'

        SM_Count
    };

    struct Stream
    {
        static Stream* OpenTemporary(u32 mode);
        static Stream* Open(cstr path, u32 mode);
        static Stream* OpenBuffered(
            cstr path,
            u32 mode,
            void* buffer,
            usize size);
        static void Close(Stream* stream);
        static void CloseAll();

        void Flush();
        void Lock();
        void Unlock();

        i32 GetFileNumber() const;
        i32 GetError() const;
        void ClearError();

        isize Size() const;
        isize Tell() const;
        void Rewind();
        void Seek(isize offset);
        bool AtEnd() const;

        i32 GetChar();
        void PutChar(i32 c);
        void PutStr(cstr src);
        void Print(cstr fmt, ...);
        void Scan(cstr fmt, ...);

        void _Write(const void* src, usize len);
        void _Read(void* dst, usize len);

        template<typename T>
        inline void ReadRef(T& dst)
        {
            _Read(&dst, sizeof(T));
        }

        template<typename T>
        inline void WriteRef(const T& src)
        {
            _Write(&src, sizeof(T));
        }

        template<typename T>
        inline void ReadPtr(T* dst, usize len)
        {
            _Read(dst, sizeof(T) * len);
        }
        template<typename T>
        inline void WritePtr(const T* src, usize len)
        {
            _Write(src, sizeof(T) * len);
        }

        template<typename T, usize len>
        inline void ReadArr(T(&dst)[len])
        {
            _Read(dst, sizeof(T) * len);
        }
        template<typename T, usize len>
        inline void WriteArr(const T(&dst)[len])
        {
            _Write(dst, sizeof(T) * len);
        }
    };
};
