#include "containers/text.h"
#include "common/stringutil.h"

#include <string.h>

void Text_New(void* txt, i32 sizeOf, const char* str)
{
    ASSERT(txt);
    ASSERT(sizeOf > 0);
    ASSERT(str);
    // zero entire struct out, as Text is used for dictionary keys
    // which will hash past the null terminator.
    memset(txt, 0, sizeOf);
    StrCpy(txt, sizeOf, str);
}
