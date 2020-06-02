#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef enum
{
    vkrShaderType_Vertex,
    vkrShaderType_Fragment,
    vkrShaderType_Compute,

    vkrShaderType_COUNT
} vkrShaderType;

typedef struct vkrCompileInput
{
    const char* const* text;
    i32 lineCount;
    vkrShaderType type;
    // key and value: #define KEY VALUE
    // null value: #define KEY
    const char* const* macroKeys;
    const char* const* macroValues;
    i32 macroCount;
} vkrCompileInput;

typedef struct vkrCompileOutput
{
    u32* spvDwords;
    i32 dwordCount;
    char* errorText;
    char* infoText;
} vkrCompileOutput;

bool vkrCompile(const vkrCompileInput* input, vkrCompileOutput* output);

PIM_C_END
