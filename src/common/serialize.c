#include "common/serialize.h"
#include "common/stringutil.h"
#include "common/fnv1a.h"
#include "allocator/allocator.h"
#include "io/fd.h"
#include <string.h>
#include <stdlib.h>
#include "cJSON/cJSON.h"

static void* ser_malloc(size_t bytes) { return tmp_calloc((i32)bytes); }
static void ser_free(void* ptr) {  }

void ser_sys_init(void)
{
    cJSON_Hooks hooks =
    {
        .malloc_fn = ser_malloc,
        .free_fn = ser_free,
    };
    cJSON_InitHooks(&hooks);
}

void ser_sys_shutdown(void)
{
    cJSON_InitHooks(NULL);
}

// ----------------------------------------------------------------------------

ser_obj_t* ser_obj_num(double value)
{
    return cJSON_CreateNumber(value);
}

ser_obj_t* ser_obj_bool(bool value)
{
    return cJSON_CreateBool(value);
}

ser_obj_t* ser_obj_str(const char* value)
{
    if (!value || !value[0])
    {
        return NULL;
    }
    return cJSON_CreateString(value);
}

ser_obj_t* ser_obj_array(void)
{
    return cJSON_CreateArray();
}

ser_obj_t* ser_obj_dict(void)
{
    return cJSON_CreateObject();
}

ser_obj_t* ser_obj_null(void)
{
    return cJSON_CreateNull();
}

void ser_obj_del(ser_obj_t* obj)
{
    if (obj)
    {
        cJSON_Delete(obj);
    }
}

// ----------------------------------------------------------------------------

const char* ser_str_get(const ser_obj_t* obj)
{
	if (!cJSON_IsString(obj))
	{
		return NULL;
	}
	return cJSON_GetStringValue(obj);
}

bool ser_str_set(ser_obj_t* obj, const char* value)
{
	if (!cJSON_IsString(obj))
	{
		return false;
	}
	return cJSON_SetValuestring(obj, value) != NULL;
}

// ----------------------------------------------------------------------------

bool ser_bool_get(const ser_obj_t* obj)
{
    return cJSON_IsTrue(obj);
}

bool ser_bool_set(ser_obj_t* obj, bool value)
{
    if (!cJSON_IsBool(obj))
    {
        return false;
	}
	obj->type &= ~(cJSON_True | cJSON_False);
	obj->type |= value ? cJSON_True : cJSON_False;
	return true;
}

// ----------------------------------------------------------------------------

double ser_num_get(const ser_obj_t* obj)
{
    if (!cJSON_IsNumber(obj))
    {
        return 0.0;
    }
    return cJSON_GetNumberValue(obj);
}

bool ser_num_set(ser_obj_t* obj, double value)
{
    if (!cJSON_IsNumber(obj))
    {
        return false;
	}
	cJSON_SetNumberHelper(obj, value);
	return true;
}

// ----------------------------------------------------------------------------

i32 ser_array_len(ser_obj_t* obj)
{
    if (!cJSON_IsArray(obj))
    {
        return 0;
    }
    return cJSON_GetArraySize(obj);
}

ser_obj_t* ser_array_get(ser_obj_t* obj, i32 index)
{
	if (!cJSON_IsArray(obj))
	{
		return NULL;
	}
    return cJSON_GetArrayItem(obj, index);
}

bool ser_array_set(ser_obj_t* obj, i32 index, ser_obj_t* value)
{
	if (!cJSON_IsArray(obj))
	{
		return false;
	}
	if (index < 0)
	{
		ASSERT(false);
		return false;
	}
    return cJSON_InsertItemInArray(obj, index, value);
}

i32 ser_array_add(ser_obj_t* obj, ser_obj_t* value)
{
	if (!cJSON_IsArray(obj))
	{
		return false;
	}
    return cJSON_AddItemToArray(obj, value);
}

bool ser_array_rm(ser_obj_t* obj, i32 index, ser_obj_t** valueOut)
{
	if (!cJSON_IsArray(obj))
	{
		return false;
	}
    if (index < 0)
	{
		ASSERT(false);
		return false;
    }
    i32 len = cJSON_GetArraySize(obj);
    if (index >= len)
    {
        ASSERT(false);
        return false;
    }
    if (valueOut)
    {
        ser_obj_t* elem = cJSON_DetachItemFromArray(obj, index);
        *valueOut = elem;
        return elem != NULL;
    }
    else
    {
        cJSON_DeleteItemFromArray(obj, index);
        return true;
    }
}

// ----------------------------------------------------------------------------

ser_obj_t* ser_dict_get(ser_obj_t* obj, const char* key)
{
    if (!cJSON_IsObject(obj))
    {
        return NULL;
    }
    if (!key || !key[0])
    {
        ASSERT(false);
        return NULL;
    }
    return cJSON_GetObjectItemCaseSensitive(obj, key);
}

bool ser_dict_set(ser_obj_t* obj, const char* key, ser_obj_t* value)
{
	if (!cJSON_IsObject(obj))
	{
		return false;
	}
	if (!key || !key[0])
	{
		ASSERT(false);
		return false;
	}
    ser_obj_t* old = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (old)
    {
        cJSON_DetachItemViaPointer(obj, old);
        cJSON_Delete(old);
    }
    if (value)
    {
        return cJSON_AddItemToObject(obj, key, value);
    }
    return false;
}

bool ser_dict_rm(ser_obj_t* obj, const char* key, ser_obj_t** valueOut)
{
	if (!cJSON_IsObject(obj))
	{
		return false;
	}
	if (!key || !key[0])
	{
		ASSERT(false);
		return false;
	}
	ser_obj_t* old = cJSON_GetObjectItemCaseSensitive(obj, key);
	if (old)
	{
		cJSON_DetachItemViaPointer(obj, old);
        if (valueOut)
        {
            *valueOut = old;
        }
        else
		{
			cJSON_Delete(old);
        }
        return true;
	}
    return false;
}

ser_obj_t* ser_read(const char* text)
{
    if (!text || !text[0])
    {
        return NULL;
    }
    return cJSON_Parse(text);
}

char* ser_write(ser_obj_t* obj, i32* lenOut)
{
    char* text = NULL;
    i32 len = 0;
    if (obj)
    {
        text = cJSON_Print(obj);
    }
    if (text)
    {
        len = StrLen(text);
    }
    if (lenOut)
    {
        *lenOut = len;
    }
    return text;
}

// ----------------------------------------------------------------------------

ser_obj_t* ser_fromfile(const char* filename)
{
    ser_obj_t* result = NULL;
    if (filename)
    {
        fd_t fd = fd_open(filename, false);
        if (fd_isopen(fd))
        {
            i32 size = (i32)fd_size(fd);
            if (size > 0)
            {
                char* text = perm_malloc(size + 1);
                i32 readSize = fd_read(fd, text, size);
                text[size] = 0;
                ASSERT(readSize == size);
                if (readSize == size)
                {
                    result = ser_read(text);
                }
                pim_free(text);
            }
            fd_close(&fd);
        }
    }
    return result;
}

// ----------------------------------------------------------------------------

bool ser_tofile(const char* filename, ser_obj_t* obj)
{
    bool wrote = false;
    if (filename && obj)
    {
        i32 len = 0;
        char* text = ser_write(obj, &len);
        if (text)
        {
            fd_t fd = fd_create(filename);
            if (fd_isopen(fd))
            {
                i32 writeSize = fd_write(fd, text, len);
                wrote = writeSize == len;
                ASSERT(wrote);
                fd_close(&fd);
            }
            pim_free(text);
        }
    }
    return wrote;
}
