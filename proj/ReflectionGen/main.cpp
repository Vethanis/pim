#include "common/io.h"
#include "common/stringutil.h"
#include "common/guid.h"
#include "common/chunk_allocator.h"
#include "common/hash.h"
#include "common/text.h"
#include "common/find.h"
#include "containers/array.h"
#include "containers/dict.h"
#include <clang-c/Index.h>
#include <stdio.h>

struct FieldInfo;
struct TypeInfo;
struct FuncInfo;

struct FieldInfo
{
    cstr name;
    i32 offset;
    Guid type;
};

struct FuncInfo
{
    cstr name;
    void* address;
    Array<Guid> paramTypes;
    Guid returnType;
};

struct TypeInfo
{
    Guid id;
    cstr name;
    i32 size;
    i32 align;
    Guid parentType;
    Guid derefType;
    Array<Guid> childTypes;
    Array<FieldInfo> fields;
    Array<FuncInfo> functions;
};

#define PIM_SRC_PATH "..\\..\\src"

static const char* ClangArgs[] =
{
    "-std=c++17",
};

struct Str
{
    CXString Value;
    cstr Ptr;

    inline Str(CXString str) : Value(str), Ptr(clang_getCString(str)) {}
    inline ~Str() { clang_disposeString(Value); }

    inline operator cstr () const { return Ptr; }
};

static CXIndex BeginArtifact()
{
    return clang_createIndex(1, 0);
}

static void EndArtifact(CXIndex index)
{
    clang_disposeIndex(index);
}

static CXTranslationUnit OpenFile(CXIndex index, cstr filename)
{
    return clang_parseTranslationUnit(
        index,
        filename,
        argof(ClangArgs),
        nullptr,
        0,
        CXTranslationUnit_SkipFunctionBodies);
}

static void CloseFile(CXTranslationUnit unit)
{
    clang_disposeTranslationUnit(unit);
}

using CursorList = Array<CXCursor>;

static bool IsNull(CXCursor cursor)
{
    return clang_Cursor_isNull(cursor);
}

static Str GetIdentifier(CXCursor cursor)
{
    return clang_getCursorUSR(cursor);
}

static CXCursorKind GetKind(CXCursor cursor)
{
    return clang_getCursorKind(cursor);
}

static CXTranslationUnit GetUnit(CXCursor cursor)
{
    return clang_Cursor_getTranslationUnit(cursor);
}

static CXCursor GetCursor(CXTranslationUnit unit)
{
    return clang_getTranslationUnitCursor(unit);
}

static CXType GetType(CXCursor cursor)
{
    return clang_getCursorType(cursor);
}

static CXType GetCanonType(CXType type)
{
    return clang_getCanonicalType(type);
}

static CXCursor GetCanonCursor(CXCursor cursor)
{
    return clang_getCanonicalCursor(cursor);
}

static CXType GetCanonType(CXCursor cursor)
{
    return GetCanonType(GetType(cursor));
}

static CXTypeKind GetTypeKind(CXType type)
{
    return type.kind;
}

static CXTypeKind GetTypeKind(CXCursor cursor)
{
    return GetTypeKind(GetType(cursor));
}

static Str GetName(CXTranslationUnit unit)
{
    return Str(clang_getTranslationUnitSpelling(unit));
}

static Str GetName(CXType type)
{
    return Str(clang_getTypeSpelling(type));
}

static Str GetTypedefName(CXType type)
{
    return clang_getTypedefName(type);
}

static Str GetTypedefName(CXCursor cursor)
{
    return GetTypedefName(GetType(cursor));
}

static Str GetName(CXTypeKind kind)
{
    return Str(clang_getTypeKindSpelling(kind));
}

static Str GetName(CXCursor cursor)
{
    return Str(clang_getCursorSpelling(cursor));
}

static Str GetUnitName(CXCursor cursor)
{
    return GetName(GetUnit(cursor));
}

static Str GetTypeName(CXCursor cursor)
{
    return GetName(GetType(cursor));
}

static Str GetTypeKindName(CXCursor cursor)
{
    return GetName(GetTypeKind(cursor));
}

static i32 ArgCount(CXCursor cursor)
{
    return clang_Cursor_getNumArguments(cursor);
}

static CXCursor GetArg(CXCursor cursor, i32 index)
{
    return clang_Cursor_getArgument(cursor, index);
}

static i32 ArgCount(CXType type)
{
    return clang_getNumArgTypes(type);
}

static CXType GetArg(CXType type, i32 index)
{
    return clang_getArgType(type, index);
}

static CXType GetReturnType(CXType type)
{
    return clang_getResultType(type);
}

static CXType GetReturnType(CXCursor cursor)
{
    return GetReturnType(GetType(cursor));
}

static CXType GetCanonReturnType(CXCursor cursor)
{
    return GetCanonType(GetReturnType(cursor));
}

static Str GetCanonReturnTypeName(CXCursor cursor)
{
    return GetName(GetCanonReturnType(cursor));
}

static bool IsVariadic(CXType type)
{
    return clang_isFunctionTypeVariadic(type);
}

static bool IsVariadic(CXCursor cursor)
{
    return clang_Cursor_isVariadic(cursor);
}

static bool IsPOD(CXType type)
{
    return clang_isPODType(type);
}

static CXType GetArrayElement(CXType type)
{
    return clang_getElementType(type);
}

static i64 GetArrayLength(CXType type)
{
    return clang_getNumElements(type);
}

static Str GetCanonTypeName(CXCursor cursor)
{
    return GetName(GetCanonType(cursor));
}

static CXType DerefType(CXType type)
{
    return clang_getPointeeType(type);
}

static CXCursor DerefCursor(CXCursor cursor)
{
    return clang_getCursorReferenced(cursor);
}

static bool IsDefinition(CXCursor cursor)
{
    return clang_isCursorDefinition(cursor);
}

static bool IsDeclaration(CXCursor cursor)
{
    return clang_isDeclaration(GetKind(cursor));
}

static CXCursor GetDefinition(CXCursor cursor)
{
    return clang_getCursorDefinition(cursor);
}

static CXCursor GetDeclaration(CXType type)
{
    return clang_getTypeDeclaration(type);
}

static CXCursor GetDeclaration(CXCursor cursor)
{
    return GetDeclaration(GetType(cursor));
}

static i32 GetAlignOf(CXType type)
{
    return (i32)clang_Type_getAlignOf(type);
}

static i32 GetAlignOf(CXCursor cursor)
{
    return GetAlignOf(GetType(cursor));
}

static i32 GetSizeOf(CXType type)
{
    return (i32)clang_Type_getSizeOf(type);
}

static i32 GetSizeOf(CXCursor cursor)
{
    return GetSizeOf(GetType(cursor));
}

static i32 GetOffsetOf(CXType type, cstr fieldName)
{
    return (i32)clang_Type_getOffsetOf(type, fieldName);
}

static i32 GetOffsetOf(CXCursor cursor)
{
    return (i32)clang_Cursor_getOffsetOfField(cursor);
}

static bool IsStatic(CXCursor cursor)
{
    return clang_Cursor_getStorageClass(cursor) == CX_SC_Static;
}

static bool IsExtern(CXCursor cursor)
{
    return clang_Cursor_getStorageClass(cursor) == CX_SC_Extern;
}

static bool IsVirtual(CXCursor cursor)
{
    return clang_Cursor_isDynamicCall(cursor);
}

static CXType GetEnumType(CXCursor cursor)
{
    return clang_getEnumDeclIntegerType(cursor);
}

static i64 GetEnumValue(CXCursor cursor)
{
    return clang_getEnumConstantDeclValue(cursor);
}

static i32 GetBitfieldWidth(CXCursor cursor)
{
    return clang_getFieldDeclBitWidth(cursor);
}

static void GetFields(CXType type, CursorList& dst)
{
    dst.Clear();
    clang_Type_visitFields(type,
        [](CXCursor C, CXClientData client_data) -> CXVisitorResult
    {
        static_cast<CursorList*>(client_data)->Grow() = C;
        return CXVisit_Continue;
    }, &dst);
}

static void GetFields(CXCursor cursor, CursorList& dst)
{
    GetFields(GetType(cursor), dst);
}

static void GetDeclOverloads(CXCursor cursor, Array<CXCursor>& dst)
{
    dst.Clear();
    i32 count = clang_getNumOverloadedDecls(cursor);
    for (i32 i = 0; i < count; ++i)
    {
        dst.Grow() = clang_getOverloadedDecl(cursor, i);
    }
}

static void GetFuncParams(CXCursor cursor, Array<CXCursor>& dst)
{
    dst.Clear();
    i32 count = clang_Cursor_getNumArguments(cursor);
    for (i32 i = 0; i < count; ++i)
    {
        dst.Grow() = clang_Cursor_getArgument(cursor, i);
    }
}

static void GetTemplateKinds(CXCursor cursor, Array<CXTemplateArgumentKind>& dst)
{
    dst.Clear();
    i32 count = clang_Cursor_getNumTemplateArguments(cursor);
    for (i32 i = 0; i < count; ++i)
    {
        dst.Grow() = clang_Cursor_getTemplateArgumentKind(cursor, i);
    }
}

static void GetTemplateTypes(CXCursor cursor, Array<CXType>& dst)
{
    dst.Clear();
    i32 count = clang_Cursor_getNumTemplateArguments(cursor);
    for (i32 i = 0; i < count; ++i)
    {
        dst.Grow() = clang_Cursor_getTemplateArgumentType(cursor, i);
    }
}

static i64 GetTemplateValue(CXCursor cursor, i32 index)
{
    return clang_Cursor_getTemplateArgumentValue(cursor, index);
}

struct Parser
{
    Array<CXCursor> m_fieldCache;
    Array<Guid> m_translationUnits;

    Array<Text<256>> m_texts;
    Array<Guid> m_guids;
    Array<Guid> m_parents;
    Array<Array<Guid>> m_children;
    Array<CXCursor> m_cursors;

    void Init()
    {
        m_fieldCache.Init(Alloc_Stdlib);
        m_translationUnits.Init(Alloc_Stdlib);

        m_texts.Init(Alloc_Stdlib);
        m_guids.Init(Alloc_Stdlib);
        m_parents.Init(Alloc_Stdlib);
        m_children.Init(Alloc_Stdlib);
        m_cursors.Init(Alloc_Stdlib);
    }

    CXChildVisitResult OnStruct(CXCursor c, CXCursor parent)
    {
        if (!IsDefinition(c))
        {
            return CXChildVisit_Continue;
        }
        Str typeName = GetTypeName(c);
        printf("struct def %s\n", typeName.Ptr);
        return CXChildVisit_Recurse;
    }

    CXChildVisitResult OnTemplate(CXCursor c, CXCursor parent)
    {
        if (!IsDefinition(c))
        {
            return CXChildVisit_Continue;
        }
        Str name = GetName(c);
        printf("template def %s\n", name.Ptr);
        return CXChildVisit_Recurse;
    }

    CXChildVisitResult OnField(CXCursor c, CXCursor parent)
    {
        if (!IsDefinition(c))
        {
            return CXChildVisit_Continue;
        }
        CXType canonType = GetCanonType(c);
        Str typeName = GetName(canonType);
        CXTypeKind typeKind = GetTypeKind(canonType);
        Str typeKindName = GetName(typeKind);
        Str name = GetName(c);
        i32 offset = GetOffsetOf(c);
        printf("    %s: %s %s;\n", typeKindName.Ptr, typeName.Ptr, name.Ptr);
        return CXChildVisit_Recurse;
    }

    CXChildVisitResult OnFunc(CXCursor c, CXCursor parent)
    {
        if (!IsDefinition(c))
        {
            return CXChildVisit_Continue;
        }

        Str name = GetName(c);
        Str retTypeName = GetCanonReturnTypeName(c);
        if (GetKind(c) == CXCursor_Constructor)
        {
            printf("    %s(", name.Ptr);
        }
        else if (GetKind(c) == CXCursor_Destructor)
        {
            printf("    ~%s(", name.Ptr);
        }
        else if (GetKind(parent) == CXCursor_StructDecl)
        {
            printf("    method def %s %s(", retTypeName.Ptr, name.Ptr);
        }
        else
        {
            printf("func def %s %s(", retTypeName.Ptr, name.Ptr);
        }
        GetFuncParams(c, m_fieldCache);
        const i32 back = m_fieldCache.Size() - 1;
        for (i32 i = 0; i <= back; ++i)
        {
            CXCursor x = m_fieldCache[i];
            Str xname = GetName(x);
            Str xtypeName = GetCanonTypeName(x);
            if (i != back)
            {
                printf("%s %s, ", xtypeName.Ptr, xname.Ptr);
            }
            else
            {
                printf("%s %s", xtypeName.Ptr, xname.Ptr);
            }
        }
        printf(");\n");
        return CXChildVisit_Recurse;
    }

    CXChildVisitResult OnUnion(CXCursor c, CXCursor parent)
    {
        if (!IsDefinition(c))
        {
            return CXChildVisit_Continue;
        }
        Str typeName = GetCanonTypeName(c);
        printf("union def %s\n", typeName.Ptr);
        return CXChildVisit_Recurse;
    }

    CXChildVisitResult OnEnum(CXCursor c, CXCursor parent)
    {
        if (!IsDefinition(c))
        {
            return CXChildVisit_Continue;
        }
        Str typeName = GetTypeName(c);
        Str baseTypeName = GetName(GetCanonType(GetEnumType(c)));
        printf("enum def %s : %s\n", typeName.Ptr, baseTypeName.Ptr);
        return CXChildVisit_Recurse;
    }

    CXChildVisitResult OnEnumValue(CXCursor c, CXCursor parent)
    {
        if (!IsDefinition(c))
        {
            return CXChildVisit_Continue;
        }
        Str name = GetName(c);
        i64 value = GetEnumValue(c);
        printf("    %s = %zi,\n", name.Ptr, value);
        return CXChildVisit_Recurse;
    }

    CXChildVisitResult OnAlias(CXCursor c, CXCursor parent)
    {
        if (!IsDefinition(c))
        {
            return CXChildVisit_Continue;
        }
        Str defName = GetTypeName(c);
        Str typeName = GetCanonTypeName(c);
        printf("typedef %s %s;\n", typeName.Ptr, defName.Ptr);
        return CXChildVisit_Recurse;
    }

    CXChildVisitResult Parse(CXCursor c, CXCursor parent)
    {
        Str id = GetIdentifier(c);
        Str parentId = GetIdentifier(parent);
        Guid guid = Guids::AsHash(id);
        Guid parentGuid = Guids::AsHash(parentId);

        {
            Text<256> text = {};
            StrCpy(argof(text.value), id);
            i32 i = RFind(m_guids, guid);
            i32 t = RFind(m_texts, text);
            Assert(i == t);
            if (i == -1)
            {
                i = m_guids.Size();
                m_guids.Grow() = guid;
                m_texts.Grow() = text;
                m_parents.Grow() = parentGuid;
                m_children.Grow() = {};
                m_cursors.Grow() = c;
                i32 j = RFind(m_guids, parentGuid);
                if (j == -1)
                {
                    j = m_guids.Size();
                    m_guids.Grow() = parentGuid;
                    text = {};
                    StrCpy(argof(text.value), parentId);
                    m_texts.Grow() = text;
                    m_parents.Grow() = {};
                    m_children.Grow() = {};
                    m_cursors.Grow() = parent;
                }
                m_children[j].UniquePush(guid);
            }
            else
            {
                return CXChildVisit_Recurse;
            }
        }

        switch (GetKind(parent))
        {
        case CXCursor_TranslationUnit:
            m_translationUnits.UniquePush(parentGuid);
            break;
        }

        switch (GetKind(c))
        {
        case CXCursor_TranslationUnit:
            m_translationUnits.UniquePush(guid);
            break;
        case CXCursor_ClassTemplate:
        case CXCursor_ClassTemplatePartialSpecialization:
            return OnTemplate(c, parent);
        case CXCursor_StructDecl:
        case CXCursor_ClassDecl:
            return OnStruct(c, parent);
        case CXCursor_FieldDecl:
            return OnField(c, parent);
        case CXCursor_UnionDecl:
            return OnUnion(c, parent);
        case CXCursor_FunctionDecl:
        case CXCursor_CXXMethod:
        case CXCursor_Constructor:
        case CXCursor_Destructor:
            return OnFunc(c, parent);
        case CXCursor_EnumDecl:
            return OnEnum(c, parent);
        case CXCursor_EnumConstantDecl:
            return OnEnumValue(c, parent);
        case CXCursor_TypeAliasDecl:
        case CXCursor_TypedefDecl:
        case CXCursor_NamespaceAlias:
        case CXCursor_UsingDirective:
        case CXCursor_UsingDeclaration:
            return OnAlias(c, parent);
        }

        return CXChildVisit_Recurse;
    }
};

static Parser ms_parser;

static CXChildVisitResult Parse(CXCursor c, CXCursor parent, CXClientData data)
{
    return ms_parser.Parse(c, parent);
}

static void AddFile(CXIndex index, cstr filePath)
{
    auto unit = OpenFile(index, filePath);
    clang_visitChildren(
        GetCursor(unit),
        Parse,
        nullptr);
    CloseFile(unit);
}

static void List(CXIndex index, const IO::Directory& dir)
{
    for (const IO::FindData& fileInfo : dir.files)
    {
        if (IEndsWith(argof(fileInfo.name), ".cpp"))
        {
            AddFile(index, fileInfo.name);
        }
    }

    for (const IO::Directory& subdir : dir.subdirs)
    {
        List(index, subdir);
    }
}

int main()
{
    ms_parser.Init();

    EResult err = ESuccess;
    IO::Directory directory = {};
    IO::ChDir(PIM_SRC_PATH, err);

    char cwd[256] = {};
    IO::GetCwd(argof(cwd), err);

    StrCpy(argof(directory.data.name), cwd);
    IO::ListDir(directory, "*", err);

    CXIndex index = BeginArtifact();
    List(index, directory);
    directory.Reset();

    EndArtifact(index);

    return 0;
}
