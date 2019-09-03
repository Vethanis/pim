#pragma once

#include "common/int_types.h"

// Streams are always binary 'b'
enum StreamMode : u32
{
    SM_Overwrite = 1u << 0,  // creates if file doesnt exist, overwrites file. 'r+' -> 'w+'
    SM_Read = 1u << 1,  // read from file. 'r'
    SM_Write = 1u << 2,  // write to file. 'w'
    SM_Commit = 1u << 3,  // flushes go to disk, 'c' MS extension
    SM_NoCommit = 1u << 4,  // reset commit state to flush to cache. 'n' MS extension
    SM_Sequential = 1u << 5,  // optimize for sequential access 'S'
    SM_Random = 1u << 6,  // optimize for random access 'R'
    SM_Temporary = 1u << 7,  // avoid flushing to disk. 'T' MS extension
    SM_Delete = 1u << 8,  // delete after last ref is closed. 'D'

    SM_Count
};

struct Stream
{
    void* m_handle;

    static Stream Open(cstr path, u32 mode);
    inline bool IsOpen() const { return m_handle != 0; }
    void Close();
    void Flush();
    i32 GetFileNumber() const;
    i32 Size() const;
    i32 Tell() const;
    void Rewind();
    void Seek(i32 offset);
    bool AtEnd() const;
    i32 GetChar();
    void PutChar(i32 c);
    void PutStr(cstr src);
    void Print(cstr fmt, ...);
    void Scan(cstr fmt, ...);
    void Write(const void* src, usize len);
    void Read(void* dst, usize len);

    template<typename T>
    inline void ReadRef(T& dst)
    {
        Read(&dst, sizeof(T));
    }
    template<typename T>
    inline void WriteRef(const T& src)
    {
        Write(&src, sizeof(T));
    }

    template<typename T>
    inline void ReadPtr(T* dst, usize len)
    {
        Read(dst, sizeof(T) * len);
    }
    template<typename T>
    inline void WritePtr(const T* src, usize len)
    {
        Write(src, sizeof(T) * len);
    }

    template<typename T, usize len>
    inline void ReadArr(T(&dst)[len])
    {
        Read(dst, sizeof(T) * len);
    }
    template<typename T, usize len>
    inline void WriteArr(const T(&dst)[len])
    {
        Write(dst, sizeof(T) * len);
    }
};
