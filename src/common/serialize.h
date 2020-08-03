#pragma once

#include "common/macro.h"

PIM_C_BEGIN

typedef enum
{
    sertype_null,
    sertype_bool,
    sertype_number,
    sertype_string,
    sertype_dict,
    sertype_array,
} sertype;

typedef struct ser_obj_s ser_obj_t;

typedef struct ser_dict_s
{
    u32* hashes;
    char** keys;
    ser_obj_t** values;
    i32 itemCount;
} ser_dict_t;

typedef struct ser_array_s
{
    ser_obj_t** values;
    i32 itemCount;
} ser_array_t;

typedef struct ser_obj_s
{
    union
    {
        double asNumber;
        bool asBool;
        char* asString;
        ser_dict_t asDict;
        ser_array_t asArray;
    } u;
    sertype type;
} ser_obj_t;

static sertype ser_obj_type(const ser_obj_t* obj) { return obj->type; }
ser_obj_t* ser_obj_num(double value);
ser_obj_t* ser_obj_bool(bool value);
ser_obj_t* ser_obj_str(const char* value);
ser_obj_t* ser_obj_array(void);
ser_obj_t* ser_obj_dict(void);
ser_obj_t* ser_obj_null(void);
void ser_obj_del(ser_obj_t* obj);

bool ser_str_set(ser_obj_t* obj, const char* value);
const char* ser_str_get(ser_obj_t* obj);

bool ser_bool_get(ser_obj_t* obj);
bool ser_bool_set(ser_obj_t* obj, bool value);

double ser_num_get(ser_obj_t* obj);
bool ser_num_set(ser_obj_t* obj, double value);

i32 ser_array_len(ser_obj_t* obj);
ser_obj_t* ser_array_get(ser_obj_t* obj, i32 index);
bool ser_array_set(ser_obj_t* obj, i32 index, ser_obj_t* value);
i32 ser_array_add(ser_obj_t* obj, ser_obj_t* value);
bool ser_array_rm(ser_obj_t* obj, i32 index, ser_obj_t** valueOut);

ser_obj_t* ser_dict_get(ser_obj_t* obj, const char* key);
bool ser_dict_set(ser_obj_t* obj, const char* key, ser_obj_t* value);
bool ser_dict_rm(ser_obj_t* obj, const char* key, ser_obj_t** valueOut);
const char** ser_dict_keys(ser_obj_t* obj, i32* lenOut);
ser_obj_t** ser_dict_values(ser_obj_t* obj, i32* lenOut);

ser_obj_t* ser_read(const char* text);
char* ser_write(ser_obj_t* obj, i32* lenOut);

ser_obj_t* ser_fromfile(const char* filename);
bool ser_tofile(const char* filename, ser_obj_t* obj);

static float ser_get_f32(ser_obj_t* obj, const char* key) { return (float)ser_num_get(ser_dict_get(obj, key)); }
static i32 ser_get_i32(ser_obj_t* obj, const char* key) { return (i32)ser_num_get(ser_dict_get(obj, key)); }

static bool ser_set_f32(ser_obj_t* obj, const char* key, float value)
{
    ser_obj_t* dst = ser_dict_get(obj, key);
    if (!dst)
    {
        return ser_dict_set(obj, key, ser_obj_num(value));
    }
    return ser_num_set(dst, value);
}

static bool ser_set_i32(ser_obj_t* obj, const char* key, i32 value)
{
    ser_obj_t* dst = ser_dict_get(obj, key);
    if (!dst)
    {
        return ser_dict_set(obj, key, ser_obj_num(value));
    }
    return ser_num_set(dst, value);
}

#define ser_getfield_f32(ptr, obj, name)        (ptr)->name = ser_get_f32((obj), #name)
#define ser_getfield_i32(ptr, obj, name)        (ptr)->name = ser_get_i32((obj), #name)

#define ser_setfield_f32(ptr, obj, name)        ser_set_f32((obj), #name, (ptr)->name)
#define ser_setfield_i32(ptr, obj, name)        ser_set_i32((obj), #name, (ptr)->name)

#define ser_getfield_f4(ptr, obj, name) \
    (ptr)->name.x = ser_get_f32((obj), STR_TOK(name##.x)); \
    (ptr)->name.y = ser_get_f32((obj), STR_TOK(name##.y)); \
    (ptr)->name.z = ser_get_f32((obj), STR_TOK(name##.z)); \
    (ptr)->name.w = ser_get_f32((obj), STR_TOK(name##.w))

#define ser_setfield_f4(ptr, obj, name) \
    ser_set_f32((obj), STR_TOK(name##.x), (ptr)->name.x); \
    ser_set_f32((obj), STR_TOK(name##.y), (ptr)->name.y); \
    ser_set_f32((obj), STR_TOK(name##.z), (ptr)->name.z); \
    ser_set_f32((obj), STR_TOK(name##.w), (ptr)->name.w)

PIM_C_END
