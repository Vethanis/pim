#include "containers/text.h"
#include "common/stringutil.h"

#include <string.h>

void text_new(void* txt, i32 sizeOf, const char* str)
{
    ASSERT(txt);
    ASSERT(sizeOf);
    ASSERT(str);
    memset(txt, 0, sizeOf);
    StrCpy(txt, sizeOf, str);
}
