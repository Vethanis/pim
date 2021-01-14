#pragma once

#include "common/macro.h"
#include "common/stringutil.h"

PIM_C_BEGIN

typedef struct cJSON ser_obj_t;

void ser_sys_init(void);
void ser_sys_shutdown(void);

ser_obj_t* ser_obj_num(double value);
ser_obj_t* ser_obj_bool(bool value);
ser_obj_t* ser_obj_str(const char* value);
ser_obj_t* ser_obj_array(void);
ser_obj_t* ser_obj_dict(void);
ser_obj_t* ser_obj_null(void);
void ser_obj_del(ser_obj_t* obj);

const char* ser_str_get(const ser_obj_t* obj);
bool ser_str_set(ser_obj_t* obj, const char* value);

bool ser_bool_get(const ser_obj_t* obj);
bool ser_bool_set(ser_obj_t* obj, bool value);

double ser_num_get(const ser_obj_t* obj);
bool ser_num_set(ser_obj_t* obj, double value);

i32 ser_array_len(ser_obj_t* obj);
ser_obj_t* ser_array_get(ser_obj_t* obj, i32 index);
bool ser_array_set(ser_obj_t* obj, i32 index, ser_obj_t* value);
i32 ser_array_add(ser_obj_t* obj, ser_obj_t* value);
bool ser_array_rm(ser_obj_t* obj, i32 index, ser_obj_t** valueOut);

ser_obj_t* ser_dict_get(ser_obj_t* obj, const char* key);
bool ser_dict_set(ser_obj_t* obj, const char* key, ser_obj_t* value);
bool ser_dict_rm(ser_obj_t* obj, const char* key, ser_obj_t** valueOut);

ser_obj_t* ser_read(const char* text);
char* ser_write(ser_obj_t* obj, i32* lenOut);

ser_obj_t* ser_fromfile(const char* filename);
bool ser_tofile(const char* filename, ser_obj_t* obj);

pim_inline float ser_get_f32(ser_obj_t* obj, const char* key) { return (float)ser_num_get(ser_dict_get(obj, key)); }
pim_inline i32 ser_get_i32(ser_obj_t* obj, const char* key) { return (i32)ser_num_get(ser_dict_get(obj, key)); }
pim_inline const char* ser_get_str(ser_obj_t* obj, const char* key) { return ser_str_get(ser_dict_get(obj, key)); }
pim_inline bool ser_get_bool(ser_obj_t* obj, const char* key) { return ser_bool_get(ser_dict_get(obj, key)); }

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

static bool ser_set_str(ser_obj_t* obj, const char* key, const char* value)
{
    ser_obj_t* dst = ser_dict_get(obj, key);
    if (!dst)
    {
        return ser_dict_set(obj, key, ser_obj_str(value));
    }
    return ser_str_set(dst, value);
}

static bool ser_set_bool(ser_obj_t* obj, const char* key, bool value)
{
    ser_obj_t* dst = ser_dict_get(obj, key);
    if (!dst)
    {
        return ser_dict_set(obj, key, ser_obj_bool(value));
    }
    return ser_bool_set(dst, value);
}

static bool _ser_load_str(char* dst, i32 size, ser_obj_t* obj, const char* key)
{
    ASSERT(dst);
    ASSERT(size > 0);
    dst[0] = 0;
    const char* value = ser_get_str(obj, key);
    if (value)
    {
        StrCpy(dst, size, value);
        return true;
    }
    return false;
}

#define ser_load_f32(ptr, obj, field)        (ptr)->field = ser_get_f32((obj), #field)
#define ser_load_i32(ptr, obj, field)        (ptr)->field = ser_get_i32((obj), #field)
#define ser_load_str(ptr, obj, field)        _ser_load_str(ARGS(ptr->field), obj, #field)
#define ser_load_bool(ptr, obj, field)       (ptr)->field = ser_get_bool((obj), #field)

#define ser_save_f32(ptr, obj, field)        ser_set_f32((obj), #field, (ptr)->field)
#define ser_save_i32(ptr, obj, field)        ser_set_i32((obj), #field, (ptr)->field)
#define ser_save_str(ptr, obj, field)        ser_set_str((obj), #field, (ptr)->field)
#define ser_save_bool(ptr, obj, field)       ser_set_bool((obj), #field, (ptr)->field)

#define ser_load_f4(ptr, obj, field) \
    (ptr)->field.x = ser_get_f32((obj), STR_TOK(field.x)); \
    (ptr)->field.y = ser_get_f32((obj), STR_TOK(field.y)); \
    (ptr)->field.z = ser_get_f32((obj), STR_TOK(field.z)); \
    (ptr)->field.w = ser_get_f32((obj), STR_TOK(field.w))

#define ser_save_f4(ptr, obj, field) \
    ser_set_f32((obj), STR_TOK(field.x), (ptr)->field.x); \
    ser_set_f32((obj), STR_TOK(field.y), (ptr)->field.y); \
    ser_set_f32((obj), STR_TOK(field.z), (ptr)->field.z); \
    ser_set_f32((obj), STR_TOK(field.w), (ptr)->field.w)

PIM_C_END
