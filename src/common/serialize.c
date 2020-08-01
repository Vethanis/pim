#include "common/serialize.h"
#include "common/stringutil.h"
#include "common/fnv1a.h"
#include "allocator/allocator.h"
#include "io/fd.h"
#include "io/fmap.h"
#include <string.h>
#include <stdlib.h>

// ----------------------------------------------------------------------------

static char** GetString(ser_obj_t* obj);
static ser_array_t* GetArray(ser_obj_t* obj);
static ser_dict_t* GetDict(ser_obj_t* obj);
static void ser_array_del(ser_obj_t* obj);
static void ser_dict_del(ser_obj_t* obj);
static i32 ser_dict_find(const ser_dict_t* dict, const char* key);

// ----------------------------------------------------------------------------

static char** GetString(ser_obj_t* obj)
{
    if (!obj)
    {
        return NULL;
    }
    if (obj->type == sertype_string)
    {
        return &obj->u.asString;
    }
    ASSERT(obj->type == sertype_null);
    return NULL;
}

static ser_array_t* GetArray(ser_obj_t* obj)
{
    if (!obj)
    {
        return NULL;
    }
    sertype type = obj->type;
    if (type == sertype_array)
    {
        return &obj->u.asArray;
    }
    ASSERT(type == sertype_null);
    return NULL;
}

static ser_dict_t* GetDict(ser_obj_t* obj)
{
    if (!obj)
    {
        return NULL;
    }
    sertype type = obj->type;
    if (type == sertype_dict)
    {
        return &obj->u.asDict;
    }
    ASSERT(type == sertype_null);
    return NULL;
}

static void ser_array_del(ser_obj_t* obj)
{
    ser_array_t* arr = GetArray(obj);
    if (!arr)
    {
        return;
    }

    const i32 len = arr->itemCount;
    arr->itemCount = 0;

    ser_obj_t** values = arr->values;
    arr->values = NULL;

    for (i32 i = 0; i < len; ++i)
    {
        ser_obj_t* value = values[i];
        values[i] = NULL;
        ser_obj_del(value);
    }
    pim_free(values);
}

static void ser_dict_del(ser_obj_t* obj)
{
    ser_dict_t* dict = GetDict(obj);
    if (!dict)
    {
        return;
    }

    const i32 len = dict->itemCount;
    dict->itemCount = 0;

    pim_free(dict->hashes);
    dict->hashes = NULL;

    char** keys = dict->keys;
    for (i32 i = 0; i < len; ++i)
    {
        pim_free(keys[i]);
        keys[i] = NULL;
    }
    pim_free(keys);
    dict->keys = NULL;

    ser_obj_t** values = dict->values;
    for (i32 i = 0; i < len; ++i)
    {
        ser_obj_t* value = values[i];
        values[i] = NULL;
        ser_obj_del(value);
    }
    pim_free(values);
    dict->values = NULL;
}

static i32 ser_dict_find(const ser_dict_t* dict, const char* key)
{
    ASSERT(dict);
    if (!key || !key[0])
    {
        return -1;
    }
    const i32 len = dict->itemCount;
    if (!len)
    {
        return -1;
    }
    const u32 keyHash = HashStr(key);
    const u32* pim_noalias hashes = dict->hashes;
    const char** pim_noalias keys = dict->keys;
    for (i32 i = len - 1; i >= 0; --i)
    {
        if (hashes[i] == keyHash)
        {
            if (StrICmp(keys[i], PIM_PATH, key) == 0)
            {
                return i;
            }
        }
    }
    return -1;
}

// ----------------------------------------------------------------------------

ser_obj_t* ser_obj_num(double value)
{
    ser_obj_t* obj = perm_calloc(sizeof(*obj));
    obj->type = sertype_number;
    obj->u.asNumber = value;
    return obj;
}

ser_obj_t* ser_obj_bool(bool value)
{
    ser_obj_t* obj = perm_calloc(sizeof(*obj));
    obj->type = sertype_bool;
    obj->u.asBool = value;
    return obj;
}

ser_obj_t* ser_obj_str(const char* value)
{
    if (!value || !value[0])
    {
        return NULL;
    }
    ser_obj_t* obj = perm_calloc(sizeof(*obj));
    obj->type = sertype_string;
    obj->u.asString = StrDup(value, EAlloc_Perm);
    return obj;
}

ser_obj_t* ser_obj_array(void)
{
    ser_obj_t* obj = perm_calloc(sizeof(*obj));
    obj->type = sertype_array;
    return obj;
}

ser_obj_t* ser_obj_dict(void)
{
    ser_obj_t* obj = perm_calloc(sizeof(*obj));
    obj->type = sertype_dict;
    return obj;
}

ser_obj_t* ser_obj_null(void)
{
    ser_obj_t* obj = perm_calloc(sizeof(*obj));
    obj->type = sertype_null;
    return obj;
}

void ser_obj_del(ser_obj_t* obj)
{
    if (obj)
    {
        switch (obj->type)
        {
        default:
            break;
        case sertype_array:
            ser_array_del(obj);
            break;
        case sertype_dict:
            ser_dict_del(obj);
            break;
        case sertype_string:
            pim_free(obj->u.asString);
            break;
        }
        memset(obj, 0, sizeof(*obj));
        pim_free(obj);
    }
}

// ----------------------------------------------------------------------------

bool ser_str_set(ser_obj_t* obj, const char* value)
{
    char** pstr = GetString(obj);
    if (!pstr || !value || !value[0])
    {
        return false;
    }
    char* prev = *pstr;
    if (prev != value)
    {
        pim_free(prev);
        *pstr = StrDup(value, EAlloc_Perm);
    }
    return true;
}

const char* ser_str_get(ser_obj_t* obj)
{
    char** pstr = GetString(obj);
    if (pstr)
    {
        return *pstr;
    }
    return NULL;
}

// ----------------------------------------------------------------------------

bool ser_bool_get(ser_obj_t* obj)
{
    if (obj && obj->type == sertype_bool)
    {
        return obj->u.asBool;
    }
    return false;
}

bool ser_bool_set(ser_obj_t* obj, bool value)
{
    if (obj && obj->type == sertype_bool)
    {
        obj->u.asBool = value;
        return true;
    }
    return false;
}

// ----------------------------------------------------------------------------

double ser_num_get(ser_obj_t* obj)
{
    if (obj && obj->type == sertype_number)
    {
        return obj->u.asNumber;
    }
    return 0.0;
}

bool ser_num_set(ser_obj_t* obj, double value)
{
    if (obj && obj->type == sertype_number)
    {
        obj->u.asNumber = value;
        return true;
    }
    return false;
}

// ----------------------------------------------------------------------------

i32 ser_array_len(ser_obj_t* obj)
{
    ser_array_t* arr = GetArray(obj);
    if (!arr)
    {
        return 0;
    }
    return arr->itemCount;
}

ser_obj_t* ser_array_get(ser_obj_t* obj, i32 index)
{
    ser_array_t* arr = GetArray(obj);
    if (!arr)
    {
        return NULL;
    }
    i32 len = arr->itemCount;
    if (index < 0 || index >= len)
    {
        return NULL;
    }
    return arr->values[index];
}

bool ser_array_set(ser_obj_t* obj, i32 index, ser_obj_t* value)
{
    ser_array_t* arr = GetArray(obj);
    if (!arr || !value)
    {
        return false;
    }
    i32 len = arr->itemCount;
    if (index < 0 || index >= len)
    {
        return false;
    }
    ser_obj_t** values = arr->values;
    ser_obj_t* prev = values[index];
    if (prev != value)
    {
        values[index] = value;
        ser_obj_del(prev);
    }
    return true;
}

i32 ser_array_add(ser_obj_t* obj, ser_obj_t* value)
{
    ser_array_t* arr = GetArray(obj);
    if (!arr || !value)
    {
        return -1;
    }

    i32 back = arr->itemCount;
    i32 len = back + 1;
    arr->itemCount = len;

    arr->values = perm_realloc(arr->values, sizeof(arr->values[0]) * len);
    arr->values[back] = value;

    return back;
}

bool ser_array_rm(ser_obj_t* obj, i32 index, ser_obj_t** valueOut)
{
    ser_array_t* arr = GetArray(obj);
    if (!arr)
    {
        return false;
    }

    i32 len = arr->itemCount;
    if (index < 0 || index >= len)
    {
        return false;
    }

    i32 back = len - 1;
    arr->itemCount = back;

    ser_obj_t** values = arr->values;
    ser_obj_t* value = values[index];
    values[index] = values[back];
    values[back] = NULL;

    if (valueOut)
    {
        *valueOut = value;
    }
    else
    {
        ser_obj_del(value);
    }

    return true;
}

// ----------------------------------------------------------------------------

ser_obj_t* ser_dict_get(ser_obj_t* obj, const char* key)
{
    ser_dict_t* dict = GetDict(obj);
    if (!dict)
    {
        return NULL;
    }
    i32 index = ser_dict_find(dict, key);
    if (index >= 0)
    {
        return dict->values[index];
    }
    return NULL;
}

bool ser_dict_set(ser_obj_t* obj, const char* key, ser_obj_t* value)
{
    ser_dict_t* dict = GetDict(obj);
    if (!dict)
    {
        return false;
    }
    i32 index = ser_dict_find(dict, key);
    if (index < 0)
    {
        return false;
    }
    ser_obj_t** values = dict->values;
    ser_obj_t* prev = values[index];
    if (prev != value)
    {
        values[index] = value;
        ser_obj_del(prev);
    }
    return true;
}

bool ser_dict_add(ser_obj_t* obj, const char* key, ser_obj_t* value)
{
    ser_dict_t* dict = GetDict(obj);
    if (!dict || !key || !key[0] || !value)
    {
        return false;
    }

    i32 index = ser_dict_find(dict, key);
    if (index >= 0)
    {
        return false;
    }

    i32 len = dict->itemCount + 1;
    index = len - 1;
    dict->itemCount = len;
    dict->hashes = perm_realloc(dict->hashes, sizeof(dict->hashes[0]) * len);
    dict->keys = perm_realloc(dict->keys, sizeof(dict->keys[0]) * len);
    dict->values = perm_realloc(dict->values, sizeof(dict->values[0]) * len);

    dict->hashes[index] = HashStr(key);
    dict->keys[index] = StrDup(key, EAlloc_Perm);
    dict->values[index] = value;

    return true;
}

bool ser_dict_rm(ser_obj_t* obj, const char* key, ser_obj_t** valueOut)
{
    ser_dict_t* dict = GetDict(obj);
    if (!dict || !key || !key[0])
    {
        return false;
    }

    i32 index = ser_dict_find(dict, key);
    if (index < 0)
    {
        return false;
    }

    u32* hashes = dict->hashes;
    char** keys = dict->keys;
    ser_obj_t** values = dict->values;

    hashes[index] = 0;

    pim_free(keys[index]);
    keys[index] = NULL;

    ser_obj_t* value = values[index];
    values[index] = NULL;
    if (valueOut)
    {
        // move
        *valueOut = value;
    }
    else
    {
        // destruct
        ser_obj_del(value);
    }

    i32 back = dict->itemCount - 1;
    dict->itemCount = back;
    hashes[index] = hashes[back];
    keys[index] = keys[back];
    values[index] = values[back];

    return true;
}

const char** ser_dict_keys(ser_obj_t* obj, i32* lenOut)
{
    ser_dict_t* dict = GetDict(obj);
    if (dict)
    {
        *lenOut = dict->itemCount;
        return dict->keys;
    }
    *lenOut = 0;
    return NULL;
}

ser_obj_t** ser_dict_values(ser_obj_t* obj, i32* lenOut)
{
    ser_dict_t* dict = GetDict(obj);
    if (dict)
    {
        *lenOut = dict->itemCount;
        return dict->values;
    }
    *lenOut = 0;
    return NULL;
}

// ----------------------------------------------------------------------------

typedef enum
{
    toktype_null = 0,
    toktype_dict,
    toktype_key,
    toktype_value,
    toktype_array,
    toktype_element,
    toktype_string,
    toktype_number,
    toktype_bool,
    toktype_any,
} toktype;

typedef struct token_s
{
    const char* begin;
    i32 len;
    i32 id;
} token_t;

typedef struct tokarr_s
{
    i32 length;
    token_t* tokens;
} tokarr_t;

static i32 PushToken(tokarr_t* arr, const char* begin, i32 tokLen, i32 id)
{
    i32 back = arr->length++;
    i32 len = back + 1;
    PermReserve(arr->tokens, len);
    token_t token = { begin, tokLen, id };
    arr->tokens[back] = token;
    return back;
}

static i32 BeginToken(tokarr_t* arr, const char* begin, i32 id)
{
    return PushToken(arr, begin, 0, id);
}

static const char* EndToken(tokarr_t* arr, i32 mark, const char* end)
{
    i32 len = arr->length;
    if (mark < len)
    {
        if (end)
        {
            token_t* token = arr->tokens + mark;
            token->len = (i32)(end - token->begin);
            return end;
        }
        else
        {
            arr->length = mark;
        }
    }
    return NULL;
}

static const char* SkipWhitespace(const char* text)
{
    while (*text && IsSpace(*text))
    {
        ++text;
    }
    return text;
}

static const char* ParseNumber(tokarr_t* arr, const char* text);
static const char* ParsePair(tokarr_t* arr, const char* text, char delim, i32 id);
static const char* ParseString(tokarr_t* arr, const char* text);
static const char* ParseArray(tokarr_t* arr, const char* text);
static const char* ParseNull(tokarr_t* arr, const char* text);
static const char* ParseBool(tokarr_t* arr, const char* text);
static const char* ParseAny(tokarr_t* arr, const char* text);
static const char* ParseDictKey(tokarr_t* arr, const char* text);
static const char* ParseDictValue(tokarr_t* arr, const char* text);
static const char* ParseDict(tokarr_t* arr, const char* text);

static const char* ParseNumber(tokarr_t* arr, const char* text)
{
    text = SkipWhitespace(text);
    i32 mark = BeginToken(arr, text, toktype_number);
    char c = *text;
    if (IsDigit(c) || c == '-' || c == '+')
    {
        ++text;
        while (IsDigit(*text))
        {
            ++text;
        }
        c = *text;
        if (c == 'e' || c == 'E')
        {
            ++text;
            c = *text;
            if (IsDigit(c) || c == '-' || c == '+')
            {
                ++text;
                while (IsDigit(*text))
                {
                    ++text;
                }
            }
            else
            {
                return NULL;
            }
        }
        else if (c == '.')
        {
            ++text;
            while (IsDigit(*text))
            {
                ++text;
            }
        }
        return EndToken(arr, mark, text);
    }
    return EndToken(arr, mark, NULL);
}

static const char* ParseString(tokarr_t* arr, const char* text)
{
    const char kDelim = '"';
    const char kEscape = '\\';
    text = SkipWhitespace(text);
    if (*text == kDelim)
    {
        // skip first quote
        ++text;
        i32 mark = BeginToken(arr, text, toktype_string);
        bool escape = false;
        while (*text)
        {
            char c = text[0];
            char d = text[1];
            if ((c == kEscape) && (d == kDelim))
            {
                escape = !escape;
                ++text;
            }
            else if (!escape && (c == kDelim))
            {
                // skip last quote
                return EndToken(arr, mark, text) + 1;
            }
            else
            {
                ++text;
            }
        }
        return EndToken(arr, mark, NULL);
    }
    return NULL;
}

static const char* ParseArrayElem(tokarr_t* arr, const char* text)
{
    text = SkipWhitespace(text);
    i32 mark = BeginToken(arr, text, toktype_element);
    const char* elem = ParseAny(arr, text);
    return EndToken(arr, mark, elem);
}

static const char* ParseArray(tokarr_t* arr, const char* text)
{
    text = SkipWhitespace(text);
    i32 mark = BeginToken(arr, text, toktype_array);
    if (*text == '[')
    {
        ++text;
        while (*text)
        {
            text = SkipWhitespace(text);
            if (*text == ']')
            {
                break;
            }
            const char* element = ParseArrayElem(arr, text);
            if (!element)
            {
                goto onfail;
            }
            text = SkipWhitespace(element);
            if (*text == ',')
            {
                ++text;
            }
        }
        if (*text == ']')
        {
            ++text;
            return EndToken(arr, mark, text);
        }
    }
onfail:
    return EndToken(arr, mark, NULL);
}

static const char* ParseNull(tokarr_t* arr, const char* text)
{
    text = SkipWhitespace(text);
    i32 mark = BeginToken(arr, text, toktype_null);
    if (!memcmp(text, "null", 4))
    {
        text += 4;
        return EndToken(arr, mark, text);
    }
    return EndToken(arr, mark, NULL);
}

static const char* ParseBool(tokarr_t* arr, const char* text)
{
    text = SkipWhitespace(text);
    i32 mark = BeginToken(arr, text, toktype_bool);
    if (!memcmp(text, "true", 4))
    {
        text += 4;
        return EndToken(arr, mark, text);
    }
    if (!memcmp(text, "false", 5))
    {
        text += 5;
        return EndToken(arr, mark, text);
    }
    return EndToken(arr, mark, NULL);
}

static const char* ParseAny(tokarr_t* arr, const char* text)
{
    text = SkipWhitespace(text);
    i32 mark = BeginToken(arr, text, toktype_any);
    const char* result = NULL;
    char c = *text;
    if (c)
    {
        if (c == '{')
        {
            result = ParseDict(arr, text);
        }
        else if (c == '[')
        {
            result = ParseArray(arr, text);
        }
        else if (c == '"')
        {
            result = ParseString(arr, text);
        }
        else if (IsDigit(c) || (c == '-') || (c == '+'))
        {
            result = ParseNumber(arr, text);
        }
        else if (c == 't' || c == 'f')
        {
            result = ParseBool(arr, text);
        }
        else if (c == 'n')
        {
            result = ParseNull(arr, text);
        }
    }
    return EndToken(arr, mark, result);
}

static const char* ParseDictKey(tokarr_t* arr, const char* text)
{
    text = SkipWhitespace(text);
    i32 mark = BeginToken(arr, text, toktype_key);
    const char* key = ParseString(arr, text);
    if (key)
    {
        key = SkipWhitespace(key);
        if (*key == ':')
        {
            return EndToken(arr, mark, key);
        }
    }
    return EndToken(arr, mark, NULL);
}

static const char* ParseDictValue(tokarr_t* arr, const char* text)
{
    text = SkipWhitespace(text);
    i32 mark = BeginToken(arr, text, toktype_value);
    if (*text == ':')
    {
        ++text;
        return EndToken(arr, mark, ParseAny(arr, text));
    }
    return EndToken(arr, mark, NULL);
}

static const char* ParseDict(tokarr_t* arr, const char* text)
{
    text = SkipWhitespace(text);
    i32 mark = BeginToken(arr, text, toktype_dict);
    const char* begin = text;
    if (*text == '{')
    {
        ++text;
        while (*text)
        {
            text = SkipWhitespace(text);
            if (*text == '}')
            {
                break;
            }
            const char* key = ParseDictKey(arr, text);
            if (!key)
            {
                goto onfail;
            }
            const char* value = ParseDictValue(arr, key);
            if (!value)
            {
                goto onfail;
            }
            text = SkipWhitespace(value);
            if (*text == ',')
            {
                ++text;
            }
        }
        if (*text == '}')
        {
            ++text;
            return EndToken(arr, mark, text);
        }
    }
onfail:
    return EndToken(arr, mark, NULL);
}

// ----------------------------------------------------------------------------

static ser_obj_t* ReadToken(tokarr_t arr, i32* pCursor);
static ser_obj_t* ReadArray(tokarr_t arr, i32* pCursor);
static ser_obj_t* ReadBool(tokarr_t arr, i32* pCursor);
static char* ReadDictKey(tokarr_t arr, i32* pCursor);
static ser_obj_t* ReadDictValue(tokarr_t arr, i32* pCursor);
static ser_obj_t* ReadDict(tokarr_t arr, i32* pCursor);
static ser_obj_t* ReadNull(tokarr_t arr, i32* pCursor);
static ser_obj_t* ReadNumber(tokarr_t arr, i32* pCursor);
static ser_obj_t* ReadString(tokarr_t arr, i32* pCursor);

static ser_obj_t* ReadArray(tokarr_t arr, i32* pCursor)
{
    i32 cursor = *pCursor;
    if (cursor < arr.length)
    {
        token_t token = arr.tokens[cursor];
        if (token.id == toktype_array)
        {
            ++cursor;
            ser_obj_t* parent = ser_obj_array();
            while (cursor < arr.length)
            {
                token = arr.tokens[cursor];
                if (token.id == toktype_element)
                {
                    ++cursor;
                    ser_obj_t* element = ReadToken(arr, &cursor);
                    ASSERT(element);
                    if (element)
                    {
                        i32 index = ser_array_add(parent, element);
                        ASSERT(index >= 0);
                    }
                }
                else
                {
                    break;
                }
            }
            *pCursor = cursor;
            return parent;
        }
    }
    return NULL;
}

static ser_obj_t* ReadBool(tokarr_t arr, i32* pCursor)
{
    i32 cursor = *pCursor;
    if (cursor < arr.length)
    {
        token_t token = arr.tokens[cursor];
        if (token.id == toktype_bool)
        {
            ++cursor;
            bool value = token.begin[0] == 't';
            ser_obj_t* obj = ser_obj_bool(value);
            *pCursor = cursor;
            return obj;
        }
    }
    return NULL;
}

static char* ReadDictKey(tokarr_t arr, i32* pCursor)
{
    i32 cursor = *pCursor;
    if (cursor < arr.length)
    {
        token_t token = arr.tokens[cursor];
        if (token.id == toktype_key)
        {
            ++cursor;
            token = arr.tokens[cursor];
            if (token.id == toktype_string)
            {
                ++cursor;
                const char* src = token.begin;
                i32 len = token.len;
                if (len > 0)
                {
                    char* key = perm_malloc(len + 1);
                    memcpy(key, src, len);
                    key[len] = 0;
                    *pCursor = cursor;
                    return key;
                }
            }
        }
    }
    return NULL;
}

static ser_obj_t* ReadDictValue(tokarr_t arr, i32* pCursor)
{
    i32 cursor = *pCursor;
    if (cursor < arr.length)
    {
        token_t token = arr.tokens[cursor];
        if (token.id == toktype_value)
        {
            ++cursor;
            ser_obj_t* value = ReadToken(arr, &cursor);
            if (value)
            {
                *pCursor = cursor;
                return value;
            }
            ASSERT(false);
        }
    }
    return NULL;
}

static ser_obj_t* ReadDict(tokarr_t arr, i32* pCursor)
{
    i32 cursor = *pCursor;
    if (cursor < arr.length)
    {
        token_t token = arr.tokens[cursor];
        if (token.id == toktype_dict)
        {
            ++cursor;
            ser_obj_t* parent = ser_obj_dict();
            while (cursor < arr.length)
            {
                token = arr.tokens[cursor];
                if (token.id == toktype_key)
                {
                    char* key = ReadDictKey(arr, &cursor);
                    ASSERT(key);
                    if (!key)
                    {
                        goto onfail;
                    }
                    ser_obj_t* value = ReadDictValue(arr, &cursor);
                    ASSERT(value);
                    if (!value)
                    {
                        goto onfail;
                    }
                    if (!ser_dict_add(parent, key, value))
                    {
                        ASSERT(false);
                        goto onfail;
                    }
                }
                else
                {
                    break;
                }
            }

            *pCursor = cursor;
            return parent;

        onfail:
            ser_obj_del(parent);
            return NULL;
        }
    }
    return NULL;
}

static ser_obj_t* ReadNull(tokarr_t arr, i32* pCursor)
{
    i32 cursor = *pCursor;
    if (cursor < arr.length)
    {
        token_t token = arr.tokens[cursor];
        if (token.id == toktype_null)
        {
            ++cursor;
            *pCursor = cursor;
            return ser_obj_null();
        }
    }
    return NULL;
}

static ser_obj_t* ReadNumber(tokarr_t arr, i32* pCursor)
{
    i32 cursor = *pCursor;
    if (cursor < arr.length)
    {
        token_t token = arr.tokens[cursor];
        if (token.id == toktype_number)
        {
            ++cursor;
            *pCursor = cursor;
            double value = atof(token.begin);
            return ser_obj_num(value);
        }
    }
    return NULL;
}

static ser_obj_t* ReadString(tokarr_t arr, i32* pCursor)
{
    i32 cursor = *pCursor;
    if (cursor < arr.length)
    {
        token_t token = arr.tokens[cursor];
        if (token.id == toktype_string)
        {
            ++cursor;
            *pCursor = cursor;
            const char* src = token.begin;
            i32 len = token.len;
            if (len > 0)
            {
                ser_obj_t* obj = perm_calloc(sizeof(*obj));
                obj->type = sertype_string;
                char* str = perm_malloc(len + 1);
                memcpy(str, src, len);
                str[len] = 0;
                obj->u.asString = str;
                return obj;
            }
        }
    }
    return NULL;
}

static ser_obj_t* ReadToken(tokarr_t arr, i32* pCursor)
{
    ser_obj_t* result = NULL;
    i32 cursor = *pCursor;
    if (cursor < arr.length)
    {
        token_t token = { 0 };
    tryread:
        token = arr.tokens[cursor];
        switch (token.id)
        {
        default:
            ASSERT(false);
            return NULL;
        case toktype_any:
            ++cursor;
            goto tryread;
            break;
        case toktype_dict:
            result = ReadDict(arr, &cursor);
            break;
        case toktype_array:
            result = ReadArray(arr, &cursor);
            break;
        case toktype_string:
            result = ReadString(arr, &cursor);
            break;
        case toktype_number:
            result = ReadNumber(arr, &cursor);
            break;
        case toktype_bool:
            result = ReadBool(arr, &cursor);
        case toktype_null:
            result = ReadNull(arr, &cursor);
            break;
        }
        if (result)
        {
            *pCursor = cursor;
        }
    }
    return result;
}

ser_obj_t* ser_read(const char* text)
{
    if (!text || !text[0])
    {
        return NULL;
    }
    tokarr_t tokarr = { 0 };
    if (ParseDict(&tokarr, text))
    {
        i32 cursor = 0;
        ser_obj_t* root = ReadDict(tokarr, &cursor);
        pim_free(tokarr.tokens);
        memset(&tokarr, 0, sizeof(tokarr));
        return root;
    }
    return NULL;
}

// ----------------------------------------------------------------------------

typedef struct textbuf_s
{
    i32 length;
    char* ptr;
} textbuf_t;

static void textbuf_indent(textbuf_t* buf, i32 indent)
{
    indent = (indent > 0) ? indent : 0;
    const i32 addLen = indent * 4;
    i32 back = buf->length;
    i32 len = back + addLen;
    char* ptr = buf->ptr;
    ptr = perm_realloc(ptr, len + 1);
    memset(ptr + back, ' ', addLen);
    ptr[len] = 0;
    buf->ptr = ptr;
    buf->length = len;
}

static void textbuf_char(textbuf_t* buf, char c)
{
    i32 back = buf->length;
    i32 len = back + 1;
    char* ptr = buf->ptr;
    ptr = perm_realloc(ptr, len + 1);
    ptr[back] = c;
    ptr[len] = 0;
    buf->length = len;
    buf->ptr = ptr;
}

static void textbuf_str(textbuf_t* buf, const char* str)
{
    i32 addLen = StrLen(str);
    i32 back = buf->length;
    i32 len = back + addLen;
    char* ptr = buf->ptr;
    ptr = perm_realloc(ptr, len + 1);
    memcpy(ptr + back, str, addLen);
    ptr[len] = 0;
    buf->length = len;
    buf->ptr = ptr;
}

static void textbuf_num(textbuf_t* buf, double num)
{
    char stack[PIM_PATH];
    SPrintf(ARGS(stack), "%f", num);
    textbuf_str(buf, stack);
}

// ----------------------------------------------------------------------------

static void WriteAny(ser_obj_t* obj, textbuf_t* buf, i32 ind);
static void WriteBool(ser_obj_t* obj, textbuf_t* buf, i32 ind);
static void WriteNull(ser_obj_t* obj, textbuf_t* buf, i32 ind);
static void WriteNumber(ser_obj_t* obj, textbuf_t* buf, i32 ind);
static void WriteString(ser_obj_t* obj, textbuf_t* buf, i32 ind);
static void WriteArray(ser_obj_t* obj, textbuf_t* buf, i32 ind);
static void WriteDict(ser_obj_t* obj, textbuf_t* buf, i32 ind);

// ----------------------------------------------------------------------------

static void WriteBool(ser_obj_t* obj, textbuf_t* buf, i32 ind)
{
    if (obj && obj->type == sertype_bool)
    {
        bool value = obj->u.asBool;
        textbuf_str(buf, value ? "true" : "false");
    }
}

static void WriteNull(ser_obj_t* obj, textbuf_t* buf, i32 ind)
{
    if (obj && obj->type == sertype_null)
    {
        textbuf_str(buf, "null");
    }
}

static void WriteNumber(ser_obj_t* obj, textbuf_t* buf, i32 ind)
{
    if (obj && obj->type == sertype_number)
    {
        textbuf_num(buf, obj->u.asNumber);
    }
}

static void WriteString(ser_obj_t* obj, textbuf_t* buf, i32 ind)
{
    if (obj && obj->type == sertype_string)
    {
        const char* str = obj->u.asString;
        if (str)
        {
            textbuf_str(buf, str);
        }
    }
}

static void WriteArray(ser_obj_t* obj, textbuf_t* buf, i32 ind)
{
    if (obj && obj->type == sertype_array)
    {
        textbuf_char(buf, '\n');
        textbuf_indent(buf, ind);
        textbuf_char(buf, '[');
        ++ind;
        i32 len = ser_array_len(obj);
        for (i32 i = 0; i < len; ++i)
        {
            ser_obj_t* elem = ser_array_get(obj, i);
            if (elem)
            {
                textbuf_char(buf, '\n');
                textbuf_indent(buf, ind);
                WriteAny(elem, buf, ind);
                if ((i + 1) < len)
                {
                    textbuf_char(buf, ',');
                }
            }
        }
        --ind;
        textbuf_char(buf, '\n');
        textbuf_indent(buf, ind);
        textbuf_char(buf, ']');
    }
}

static void WriteDict(ser_obj_t* dict, textbuf_t* buf, i32 ind)
{
    if (dict && dict->type == sertype_dict)
    {
        i32 len = 0;
        const char** keys = ser_dict_keys(dict, &len);
        ser_obj_t** values = ser_dict_values(dict, &len);
        textbuf_char(buf, '\n');
        textbuf_indent(buf, ind);
        textbuf_char(buf, '{');
        ++ind;
        for (i32 i = 0; i < len; ++i)
        {
            const char* key = keys[i];
            ser_obj_t* value = values[i];
            if (key && value)
            {
                textbuf_char(buf, '\n');
                textbuf_indent(buf, ind);
                textbuf_char(buf, '"');
                textbuf_str(buf, key);
                textbuf_char(buf, '"');
                textbuf_char(buf, ':');
                textbuf_char(buf, ' ');
                WriteAny(value, buf, ind);
                if ((i + 1) < len)
                {
                    textbuf_char(buf, ',');
                }
            }
        }
        --ind;
        textbuf_char(buf, '\n');
        textbuf_indent(buf, ind);
        textbuf_char(buf, '}');
    }
}


static void WriteAny(ser_obj_t* obj, textbuf_t* buf, i32 ind)
{
    if (obj)
    {
        switch (obj->type)
        {
        default:
            ASSERT(false);
            break;
        case sertype_array:
            WriteArray(obj, buf, ind);
            break;
        case sertype_bool:
            WriteBool(obj, buf, ind);
            break;
        case sertype_dict:
            WriteDict(obj, buf, ind);
            break;
        case sertype_null:
            WriteNull(obj, buf, ind);
            break;
        case sertype_number:
            WriteNumber(obj, buf, ind);
            break;
        case sertype_string:
            WriteString(obj, buf, ind);
        }
    }
}

// ----------------------------------------------------------------------------

char* ser_write(ser_obj_t* obj, i32* lenOut)
{
    textbuf_t buf = { 0 };
    WriteDict(obj, &buf, 0);
    if (lenOut)
    {
        *lenOut = buf.length;
    }
    return buf.ptr;
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
            fmap_t map = { 0 };
            if (fmap_create(&map, fd, false))
            {
                const char* text = map.ptr;
                result = ser_read(text);
                fmap_destroy(&map);
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
                i32 bytesWritten = fd_write(fd, text, len);
                fd_close(&fd);
                wrote = bytesWritten == len;
            }
            pim_free(text);
        }
    }
    return wrote;
}
