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
    char* filename;
    char* entrypoint;
    char* text;
    char** macroKeys;
    char** macroValues;
    i32 macroCount;
    vkrShaderType type;
} vkrCompileInput;

typedef struct vkrCompileOutput
{
    u32* dwords;
    i32 dwordCount;
    char* errors;
    char* disassembly;
} vkrCompileOutput;

bool vkrCompile(const vkrCompileInput* input, vkrCompileOutput* output);

PIM_C_END
