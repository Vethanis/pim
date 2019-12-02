#include "common/io.h"
#include "common/stringutil.h"
#include <clang-c/Index.h>
#include <stdio.h>

#define PIM_SRC_PATH "..\\..\\src"

static const char* ClangArgs[] =
{
    "-std=c++17",
};

struct Parser
{
    CXChildVisitResult ParseStruct(CXCursor c, CXCursor parent)
    {

    }

    CXChildVisitResult ParseUnion(CXCursor c, CXCursor parent)
    {

    }

    CXChildVisitResult ParseEnum(CXCursor c, CXCursor parent)
    {

    }

    CXChildVisitResult Parse(CXCursor c, CXCursor parent)
    {
        CXCursorKind kind = clang_getCursorKind(c);

        switch (kind)
        {
        case CXCursor_StructDecl:
        case CXCursor_ClassDecl:
            return ParseStruct(c, parent);
        case CXCursor_UnionDecl:
            return ParseUnion(c, parent);
        case CXCursor_EnumDecl:
            return ParseEnum(c, parent);
        }

        return CXChildVisit_Continue;
    }


    static CXChildVisitResult Parse(CXCursor c, CXCursor parent, CXClientData data)
    {
        return ((Parser*)data)->Parse(c, parent);
    }
};


static void ParseFile(cstr filePath)
{
    Parser parser = {};

    CXIndex index = clang_createIndex(0, 0);
    Assert(index);
    CXTranslationUnit unit = clang_parseTranslationUnit(
        index,
        filePath,
        ClangArgs, countof(ClangArgs),
        nullptr, 0,
        CXTranslationUnit_None);
    Assert(unit);

    CXCursor cursor = clang_getTranslationUnitCursor(unit);
    clang_visitChildren(
        cursor,
        Parser::Parse,
        &parser);


    //    [](CXCursor c, CXCursor parent, CXClientData clientData)
    //{


    //    CXString cursorStr = clang_getCursorSpelling(c);
    //    CXString cursorKindStr = clang_getCursorKindSpelling(kind);

    //    cstr cursorName = clang_getCString(cursorStr);
    //    cstr cursorKindName = clang_getCString(cursorKindStr);

    //    printf("'%s' '%s'\n", cursorKindName, cursorName);

    //    clang_disposeString(cursorKindStr);
    //    clang_disposeString(cursorStr);

    //    return CXChildVisit_Recurse;
    //});

    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);
}

static void List(const IO::Directory& dir)
{
    puts(dir.data.name);

    for (const IO::FindData& fileInfo : dir.files)
    {
        if (IEndsWith(argof(fileInfo.name), ".h") ||
            IEndsWith(argof(fileInfo.name), ".cpp"))
        {
            puts(fileInfo.name);
            ParseFile(fileInfo.name);
        }
    }

    for (const IO::Directory& subdir : dir.subdirs)
    {
        List(subdir);
    }
}

int main()
{
    EResult err = ESuccess;
    IO::Directory directory = {};
    IO::ChDir(PIM_SRC_PATH, err);

    char cwd[256] = {};
    IO::GetCwd(argof(cwd), err);

    StrCpy(argof(directory.data.name), cwd);
    IO::ListDir(directory, "*", err);

    List(directory);

    directory.Reset();

    return 0;
}
