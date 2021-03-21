#include "common/serialize.h"
#include "common/stringutil.h"
#include "common/fnv1a.h"
#include "allocator/allocator.h"
#include "io/fd.h"
#include <string.h>
#include <stdlib.h>
#include "cJSON/cJSON.h"

static void* ser_malloc(size_t bytes) { return Temp_Calloc((i32)bytes); }
static void ser_free(void* ptr) {  }

void SerSys_Init(void)
{
    cJSON_Hooks hooks =
    {
        .malloc_fn = ser_malloc,
        .free_fn = ser_free,
    };
    cJSON_InitHooks(&hooks);
}

void SerSys_Shutdown(void)
{
    cJSON_InitHooks(NULL);
}

// ----------------------------------------------------------------------------

SerObj* SerObj_Num(double value)
{
    return cJSON_CreateNumber(value);
}

SerObj* SerObj_Bool(bool value)
{
    return cJSON_CreateBool(value);
}

SerObj* SerObj_Str(const char* value)
{
    if (!value || !value[0])
    {
        return NULL;
    }
    return cJSON_CreateString(value);
}

SerObj* SerObj_Array(void)
{
    return cJSON_CreateArray();
}

SerObj* SerObj_Dict(void)
{
    return cJSON_CreateObject();
}

SerObj* SerObj_Null(void)
{
    return cJSON_CreateNull();
}

void SerObj_Del(SerObj* obj)
{
    if (obj)
    {
        cJSON_Delete(obj);
    }
}

// ----------------------------------------------------------------------------

const char* Ser_StrGet(const SerObj* obj)
{
	if (!cJSON_IsString(obj))
	{
		return NULL;
	}
	return cJSON_GetStringValue(obj);
}

bool Ser_StrSet(SerObj* obj, const char* value)
{
	if (!cJSON_IsString(obj))
	{
		return false;
	}
	return cJSON_SetValuestring(obj, value) != NULL;
}

// ----------------------------------------------------------------------------

bool Ser_BoolGet(const SerObj* obj)
{
    return cJSON_IsTrue(obj);
}

bool Ser_BoolSet(SerObj* obj, bool value)
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

double Ser_NumGet(const SerObj* obj)
{
    if (!cJSON_IsNumber(obj))
    {
        return 0.0;
    }
    return cJSON_GetNumberValue(obj);
}

bool Ser_NumSet(SerObj* obj, double value)
{
    if (!cJSON_IsNumber(obj))
    {
        return false;
	}
	cJSON_SetNumberHelper(obj, value);
	return true;
}

// ----------------------------------------------------------------------------

i32 Ser_ArrayLen(SerObj* obj)
{
    if (!cJSON_IsArray(obj))
    {
        return 0;
    }
    return cJSON_GetArraySize(obj);
}

SerObj* Ser_ArrayGet(SerObj* obj, i32 index)
{
	if (!cJSON_IsArray(obj))
	{
		return NULL;
	}
    return cJSON_GetArrayItem(obj, index);
}

bool Ser_ArraySet(SerObj* obj, i32 index, SerObj* value)
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

i32 Ser_ArrayAdd(SerObj* obj, SerObj* value)
{
	if (!cJSON_IsArray(obj))
	{
		return false;
	}
    return cJSON_AddItemToArray(obj, value);
}

bool Ser_ArrayRm(SerObj* obj, i32 index, SerObj** valueOut)
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
        SerObj* elem = cJSON_DetachItemFromArray(obj, index);
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

SerObj* Ser_DictGet(SerObj* obj, const char* key)
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

bool Ser_DictSet(SerObj* obj, const char* key, SerObj* value)
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
    SerObj* old = cJSON_GetObjectItemCaseSensitive(obj, key);
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

bool Ser_DictRm(SerObj* obj, const char* key, SerObj** valueOut)
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
	SerObj* old = cJSON_GetObjectItemCaseSensitive(obj, key);
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

SerObj* Ser_Read(const char* text)
{
    if (!text || !text[0])
    {
        return NULL;
    }
    return cJSON_Parse(text);
}

char* Ser_Write(SerObj* obj, i32* lenOut)
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

SerObj* Ser_ReadFile(const char* filename)
{
    SerObj* result = NULL;
    if (filename)
    {
        fd_t fd = fd_open(filename, false);
        if (fd_isopen(fd))
        {
            i32 size = (i32)fd_size(fd);
            if (size > 0)
            {
                char* text = Perm_Alloc(size + 1);
                i32 readSize = fd_read(fd, text, size);
                text[size] = 0;
                ASSERT(readSize == size);
                if (readSize == size)
                {
                    result = Ser_Read(text);
                }
                Mem_Free(text);
            }
            fd_close(&fd);
        }
    }
    return result;
}

// ----------------------------------------------------------------------------

bool Ser_WriteFile(const char* filename, SerObj* obj)
{
    bool wrote = false;
    if (filename && obj)
    {
        i32 len = 0;
        char* text = Ser_Write(obj, &len);
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
            Mem_Free(text);
        }
    }
    return wrote;
}

// ----------------------------------------------------------------------------

float Ser_GetFloat(SerObj* obj, const char* key)
{
    return (float)Ser_NumGet(Ser_DictGet(obj, key));
}

i32 Ser_GetInt(SerObj* obj, const char* key)
{
    return (i32)Ser_NumGet(Ser_DictGet(obj, key));
}

const char* Ser_GetStr(SerObj* obj, const char* key)
{
    return Ser_StrGet(Ser_DictGet(obj, key));
}

bool Ser_GetBool(SerObj* obj, const char* key)
{
    return Ser_BoolGet(Ser_DictGet(obj, key));
}

bool Ser_SetFloat(SerObj* obj, const char* key, float value)
{
    SerObj* dst = Ser_DictGet(obj, key);
    if (!dst)
    {
        return Ser_DictSet(obj, key, SerObj_Num(value));
    }
    return Ser_NumSet(dst, value);
}

bool Ser_SetInt(SerObj* obj, const char* key, i32 value)
{
    SerObj* dst = Ser_DictGet(obj, key);
    if (!dst)
    {
        return Ser_DictSet(obj, key, SerObj_Num(value));
    }
    return Ser_NumSet(dst, value);
}

bool Ser_SetStr(SerObj* obj, const char* key, const char* value)
{
    SerObj* dst = Ser_DictGet(obj, key);
    if (!dst)
    {
        return Ser_DictSet(obj, key, SerObj_Str(value));
    }
    return Ser_StrSet(dst, value);
}

bool Ser_SetBool(SerObj* obj, const char* key, bool value)
{
    SerObj* dst = Ser_DictGet(obj, key);
    if (!dst)
    {
        return Ser_DictSet(obj, key, SerObj_Bool(value));
    }
    return Ser_BoolSet(dst, value);
}

bool _Ser_LoadStr(char* dst, i32 size, SerObj* obj, const char* key)
{
    ASSERT(dst);
    ASSERT(size > 0);
    dst[0] = 0;
    const char* value = Ser_GetStr(obj, key);
    if (value)
    {
        StrCpy(dst, size, value);
        return true;
    }
    return false;
}
