
#include "common/typedecl.h"

struct BasicTypes
{
    BasicTypes()
    {
        TypeInfo infos[] =
        {
            BasicDecl(u8),
            BasicDecl(i8),
            BasicDecl(u16),
            BasicDecl(i16),
            BasicDecl(u32),
            BasicDecl(i32),
            BasicDecl(u64),
            BasicDecl(i64),
            BasicDecl(usize),
            BasicDecl(isize),
            BasicDecl(char),
        };
        TypeRegistry::Register({ ARGS(infos) });
    }
};
