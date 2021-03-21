#pragma once

#include "common/macro.h"
#include "common/stringutil.h"

PIM_C_BEGIN

typedef struct cJSON SerObj;

void SerSys_Init(void);
void SerSys_Shutdown(void);

SerObj* SerObj_Num(double value);
SerObj* SerObj_Bool(bool value);
SerObj* SerObj_Str(const char* value);
SerObj* SerObj_Array(void);
SerObj* SerObj_Dict(void);
SerObj* SerObj_Null(void);
void SerObj_Del(SerObj* obj);

const char* Ser_StrGet(const SerObj* obj);
bool Ser_StrSet(SerObj* obj, const char* value);

bool Ser_BoolGet(const SerObj* obj);
bool Ser_BoolSet(SerObj* obj, bool value);

double Ser_NumGet(const SerObj* obj);
bool Ser_NumSet(SerObj* obj, double value);

i32 Ser_ArrayLen(SerObj* obj);
SerObj* Ser_ArrayGet(SerObj* obj, i32 index);
bool Ser_ArraySet(SerObj* obj, i32 index, SerObj* value);
i32 Ser_ArrayAdd(SerObj* obj, SerObj* value);
bool Ser_ArrayRm(SerObj* obj, i32 index, SerObj** valueOut);

SerObj* Ser_DictGet(SerObj* obj, const char* key);
bool Ser_DictSet(SerObj* obj, const char* key, SerObj* value);
bool Ser_DictRm(SerObj* obj, const char* key, SerObj** valueOut);

SerObj* Ser_Read(const char* text);
char* Ser_Write(SerObj* obj, i32* lenOut);

SerObj* Ser_ReadFile(const char* filename);
bool Ser_WriteFile(const char* filename, SerObj* obj);

float Ser_GetFloat(SerObj* obj, const char* key);
i32 Ser_GetInt(SerObj* obj, const char* key);
const char* Ser_GetStr(SerObj* obj, const char* key);
bool Ser_GetBool(SerObj* obj, const char* key);

bool Ser_SetFloat(SerObj* obj, const char* key, float value);
bool Ser_SetInt(SerObj* obj, const char* key, i32 value);
bool Ser_SetStr(SerObj* obj, const char* key, const char* value);
bool Ser_SetBool(SerObj* obj, const char* key, bool value);

bool _Ser_LoadStr(char* dst, i32 size, SerObj* obj, const char* key);

// Helpers for loading and saving struct fields

#define Ser_LoadFloat(ptr, obj, field) \
    do { (ptr)->field = Ser_GetFloat((obj), #field); } while(0)

#define Ser_LoadInt(ptr, obj, field) \
    do { (ptr)->field = Ser_GetInt((obj), #field); } while(0)

#define Ser_LoadStr(ptr, obj, field) \
    do { _Ser_LoadStr(ARGS(ptr->field), obj, #field); } while(0)

#define Ser_LoadBool(ptr, obj, field) \
    do { (ptr)->field = Ser_GetBool((obj), #field); } while(0)

#define Ser_SaveFloat(ptr, obj, field) \
    do { Ser_SetFloat((obj), #field, (ptr)->field); } while(0)

#define Ser_SaveInt(ptr, obj, field) \
    do { Ser_SetInt((obj), #field, (ptr)->field); } while(0)

#define Ser_SaveStr(ptr, obj, field) \
    do { Ser_SetStr((obj), #field, (ptr)->field); } while(0)

#define Ser_SaveBool(ptr, obj, field) \
    do { Ser_SetBool((obj), #field, (ptr)->field); while(0)

#define Ser_LoadVector(ptr, obj, field) \
    do { \
    (ptr)->field.x = Ser_GetFloat((obj), STR_TOK(field.x)); \
    (ptr)->field.y = Ser_GetFloat((obj), STR_TOK(field.y)); \
    (ptr)->field.z = Ser_GetFloat((obj), STR_TOK(field.z)); \
    (ptr)->field.w = Ser_GetFloat((obj), STR_TOK(field.w)); \
    } while(0)

#define Ser_SaveVector(ptr, obj, field) \
    do { \
    Ser_SetFloat((obj), STR_TOK(field.x), (ptr)->field.x); \
    Ser_SetFloat((obj), STR_TOK(field.y), (ptr)->field.y); \
    Ser_SetFloat((obj), STR_TOK(field.z), (ptr)->field.z); \
    Ser_SetFloat((obj), STR_TOK(field.w), (ptr)->field.w); \
    } while(0)

PIM_C_END
