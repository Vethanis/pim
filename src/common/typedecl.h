#pragma once

#include "common/type.h"

struct TypeBuilder
{
    TypeInfo info;

    inline operator TypeInfo () { return info; }

    template<typename T>
    inline static TypeBuilder Begin(cstr name)
    {
        return
        {
            TypeInfo
            {
                { name },
                name,
                sizeof(T),
                alignof(T),
                [](void* ptr) { *(T*)(ptr) = T(); },
                [](void* ptr) { T* t = (T*)ptr; t->~T(); MemClear(t, sizeof(T)); },
            }
        };
    }

    inline TypeBuilder& With(FieldInfo field)
    {
        info.Fields.Grow() = field;
        return *this;
    }

    inline TypeBuilder& With(FuncInfo func)
    {
        info.Funcs.Grow() = func;
        return *this;
    }
};

// ----------------------------------------------------------------------------

#define BasicDecl(T) TypeInfo { ToGuid(#T), #T, sizeof(T), alignof(T) }

#define FieldDecl(T, F, fname) FieldInfo { ToGuid(#F), #fname, offsetof(T, fname) }

#define FuncDecl(Fn) FuncInfo { ToGuid(#Fn), #Fn, &Fn }

#define MethodDecl(Fn, Wrapper) FuncInfo { ToGuid(#Fn), #Fn, &Wrapper }

#define TypeDecl(T) TypeBuilder::Begin<T>(#T)

// ----------------------------------------------------------------------------

//
//struct Foo
//{
//    i32 bar;
//    u32 baz;
//
//    i32 Size() { return bar; }
//    static i32 _Size(Foo* p) { return p->Size(); }
//};
//
//TypeInfo myType = TypeDecl(Foo)
//.With(FieldDecl(Foo, i32, bar))
//.With(FieldDecl(Foo, u32, baz))
//.With(MethodDecl(Foo::Size, Foo::_Size));
