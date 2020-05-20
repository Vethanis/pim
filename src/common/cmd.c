#include "common/cmd.h"
#include "allocator/allocator.h"
#include "common/stringutil.h"
#include "common/cvar.h"
#include "containers/sdict.h"
#include "containers/queue.h"
#include "assets/asset_system.h"
#include "common/time.h"
#include "common/profiler.h"
#include "common/console.h"
#include <string.h>

// ----------------------------------------------------------------------------

typedef struct cmdalias_s
{
    char* value;
} cmdalias_t;

typedef struct cmdbrain_s
{
    cbuf_t cbuf;
    i32 yieldframe;
} cmdbrain_t;

// ----------------------------------------------------------------------------

static bool IsSpecialChar(char c);
static char** cmd_tokenize(const char* text, i32* argcOut);
static const char* cmd_parse(const char* text, char** tokenOut);
static cmdstat_t cmd_alias_fn(i32 argc, const char** argv);
static cmdstat_t cmd_execfile_fn(i32 argc, const char** argv);
static cmdstat_t cmd_wait_fn(i32 argc, const char** argv);

// ----------------------------------------------------------------------------

static sdict_t ms_cmds;
static sdict_t ms_aliases;
static queue_t ms_cmdqueue;

// ----------------------------------------------------------------------------

void cmd_sys_init(void)
{
    sdict_new(&ms_cmds, sizeof(cmdfn_t), EAlloc_Perm);
    sdict_new(&ms_aliases, sizeof(cmdalias_t), EAlloc_Perm);
    queue_create(&ms_cmdqueue, sizeof(cmdbrain_t), EAlloc_Perm);
    cmd_reg("alias", cmd_alias_fn);
    cmd_reg("exec", cmd_execfile_fn);
    cmd_reg("wait", cmd_wait_fn);
}

ProfileMark(pm_update, cmd_sys_update)
void cmd_sys_update(void)
{
    ProfileBegin(pm_update);

    const i32 curFrame = time_framecount();

    cmdbrain_t brain;
    while (queue_trypop(&ms_cmdqueue, &brain, sizeof(brain)))
    {
        if (brain.yieldframe == curFrame)
        {
            queue_push(&ms_cmdqueue, &brain, sizeof(brain));
            break;
        }
        else
        {
            brain.yieldframe = curFrame;
            cbuf_exec(&brain.cbuf);
        }
    }

    ProfileEnd(pm_update);
}

void cmd_sys_shutdown(void)
{
    sdict_del(&ms_cmds);
    sdict_del(&ms_aliases);
    cmdbrain_t brain;
    while (queue_trypop(&ms_cmdqueue, &brain, sizeof(brain)))
    {
        cbuf_del(&brain.cbuf);
    }
    queue_destroy(&ms_cmdqueue);
}

void cbuf_new(cbuf_t* buf, EAlloc allocator)
{
    ASSERT(buf);
    buf->ptr = NULL;
    buf->length = 0;
    buf->allocator = allocator;
}

void cbuf_del(cbuf_t* buf)
{
    ASSERT(buf);
    pim_free(buf->ptr);
    buf->ptr = NULL;
    buf->length = 0;
}

void cbuf_clear(cbuf_t* buf)
{
    ASSERT(buf);
    buf->length = 0;
    char* ptr = buf->ptr;
    if (ptr)
    {
        ptr[0] = 0;
    }
}

void cbuf_reserve(cbuf_t* buf, i32 size)
{
    ASSERT(buf);
    ASSERT(size >= 0);
    if (size > buf->length)
    {
        buf->ptr = pim_realloc(buf->allocator, buf->ptr, size + 1);
    }
}

void cbuf_pushfront(cbuf_t* buf, const char* text)
{
    ASSERT(buf);
    ASSERT(text);

    const i32 textLen = StrLen(text);
    const i32 bufLen = buf->length;
    const i32 newLen = bufLen + textLen;
    cbuf_reserve(buf, newLen);
    char* ptr = buf->ptr;
    memmove(ptr + bufLen, ptr, bufLen);
    memcpy(ptr, text, textLen);
    ptr[newLen] = 0;
    buf->length = newLen;
}

void cbuf_pushback(cbuf_t* buf, const char* text)
{
    ASSERT(buf);
    ASSERT(text);

    const i32 textLen = StrLen(text);
    const i32 bufLen = buf->length;
    const i32 newLen = bufLen + textLen;
    cbuf_reserve(buf, newLen);
    char* ptr = buf->ptr;
    memcpy(ptr + bufLen, text, textLen);
    ptr[newLen] = 0;
    buf->length = newLen;
}

cmdstat_t cbuf_exec(cbuf_t* buf)
{
    cmdstat_t status = cmdstat_ok;
    ASSERT(buf);

    while (buf->length > 0)
    {
        char* text = buf->ptr;
        i32 i = 0;
        i32 q = 0;

        const i32 len = buf->length;
        for (; i < len; ++i)
        {
            char c = text[i];
            if (c == '"')
            {
                ++q;
            }
            else if (!(q & 1) && (c == ';'))
            {
                break;
            }
            else if (c == '\n')
            {
                break;
            }
        }

        char line[1024] = { 0 };
        ASSERT(i < NELEM(line));
        memcpy(line, text, i);
        line[i] = 0;

        if (i == len)
        {
            cbuf_clear(buf);
        }
        else
        {
            ++i; // consume line ending
            buf->length -= i;
            memmove(text, text + i, buf->length);
            text[buf->length] = 0;
        }

        status = cmd_exec(buf, line, cmdsrc_buffer);
        if (status == cmdstat_err)
        {
            con_printf(C32_RED, "Error executing line: %s", line);
        }
        if (status != cmdstat_ok)
        {
            break;
        }
    }

    if (status == cmdstat_yield)
    {
        cmdbrain_t brain = { 0 };
        brain.cbuf = *buf;
        brain.yieldframe = time_framecount();
        queue_push(&ms_cmdqueue, &brain, sizeof(brain));
    }
    else
    {
        cbuf_del(buf);
    }

    return status;
}

void cmd_reg(const char* name, cmdfn_t fn)
{
    ASSERT(name);
    ASSERT(fn);
    if (!sdict_add(&ms_cmds, name, &fn))
    {
        sdict_set(&ms_cmds, name, &fn);
    }
}

bool cmd_exists(const char* name)
{
    ASSERT(name);
    return sdict_find(&ms_cmds, name) != -1;
}

const char* cmd_complete(const char* namePart)
{
    ASSERT(namePart);
    const i32 partLen = StrLen(namePart);
    const u32 width = ms_cmds.width;
    const char** names = ms_cmds.keys;
    for (u32 i = 0; i < width; ++i)
    {
        const char* name = names[i];
        if (name && !StrCmp(namePart, partLen, name))
        {
            return name;
        }
    }
    return NULL;
}

cmdstat_t cmd_exec(cbuf_t* buf, const char* line, cmdsrc_t src)
{
    ASSERT(buf);
    ASSERT(line);

    i32 argc = 0;
    char** argv = cmd_tokenize(line, &argc);
    if (argc < 1)
    {
        // whitespace, comments, newlines, etc
        return cmdstat_ok;
    }
    ASSERT(argv);

    const char* name = argv[0];
    ASSERT(name);

    // commands
    cmdfn_t cmd = NULL;
    if (sdict_get(&ms_cmds, name, &cmd))
    {
        return cmd(argc, argv);
    }

    // aliases (macros to expand into front of cbuf)
    cmdalias_t alias = { 0 };
    if (sdict_get(&ms_aliases, name, &alias))
    {
        cbuf_pushfront(buf, alias.value);
        return cmdstat_ok;
    }

    // cvars
    cvar_t* cvar = cvar_find(name);
    if (cvar)
    {
        if (argc == 1)
        {
            con_printf(C32_WHITE, "\"%s\" is \"%s\"", cvar->name, cvar->value);
            return cmdstat_ok;
        }
        if (argc >= 2)
        {
            cvar_set_str(cvar, argv[1]);
            con_printf(C32_GREEN, "%s = %s", cvar->name, cvar->value);
            return cmdstat_ok;
        }
    }

    con_printf(C32_RED, "Unknown command \"%s\"", name);
    return cmdstat_err;
}

// ----------------------------------------------------------------------------

static bool IsSpecialChar(char c)
{
    switch (c)
    {
    default:
        return false;
    case '{':
    case '}':
    case '(':
    case ')':
    case '\'':
    case ':':
        return true;
    }
}

static const char* cmd_parse(const char* text, char** tokenOut)
{
    ASSERT(text);
    ASSERT(tokenOut);
    *tokenOut = NULL;
    i32 len = 0;
    char c = 0;
    char token[1024] = { 0 };

wspace:
    // whitespace
    while ((c = *text) <= ' ')
    {
        if (!c)
        {
            return NULL;
        }
        ++text;
    }

    // comments
    if (c == '/' && text[1] == '/')
    {
        while (*text && (*text != '\n'))
        {
            ++text;
        }
        goto wspace;
    }

    // quotes
    if (c == '"')
    {
        ++text;
        while (true)
        {
            c = *text;
            ++text;
            if (!c || c == '"')
            {
                ASSERT(len < NELEM(token));
                token[len] = 0;
                *tokenOut = StrDup(token, EAlloc_Temp);
                return text;
            }
            token[len] = c;
            ++len;
        }
    }

    // special characters
    if (IsSpecialChar(c))
    {
        ASSERT(len < NELEM(token));
        token[len] = c;
        ++len;
        ASSERT(len < NELEM(token));
        token[len] = 0;
        *tokenOut = StrDup(token, EAlloc_Temp);
        return text + 1;
    }

    // words
    do
    {
        ASSERT(len < NELEM(token));
        token[len] = c;
        ++len;
        ++text;
        c = *text;
    } while (c > ' ' && !IsSpecialChar(c));

    ASSERT(len < NELEM(token));
    token[len] = 0;
    *tokenOut = StrDup(token, EAlloc_Temp);
    return text;
}

static char** cmd_tokenize(const char* text, i32* argcOut)
{
    ASSERT(text);
    ASSERT(argcOut);
    i32 argc = 0;
    char** argv = NULL;
    i32 i = 0;
    while (text)
    {
        char c = *text;
        while (c && (c <= ' ') && (c != '\n'))
        {
            ++text;
            c = *text;
        }
        if (!c || (c == '\n'))
        {
            break;
        }
        char* token = NULL;
        text = cmd_parse(text, &token);
        if (token)
        {
            ++argc;
            argv = pim_realloc(EAlloc_Temp, argv, sizeof(*argv) * argc);
            argv[argc - 1] = token;
        }
    }
    *argcOut = argc;
    return argv;
}

static cmdstat_t cmd_alias_fn(i32 argc, const char** argv)
{
    if (argc <= 1)
    {
        con_puts(C32_RED, "Current alias commands:");
        const u32 width = ms_aliases.width;
        const char** names = ms_aliases.keys;
        const cmdalias_t* aliases = ms_aliases.values;
        for (u32 i = 0; i < width; ++i)
        {
            const char* name = names[i];
            if (name)
            {
                con_printf(C32_RED, "%s : %s", name, aliases[i].value);
            }
        }
        return cmdstat_err;
    }

    const char* aliasKey = argv[1];

    cmdalias_t alias = { 0 };
    if (sdict_rm(&ms_aliases, aliasKey, &alias))
    {
        pim_free(alias.value);
        alias.value = NULL;
    }

    char cmd[1024] = { 0 };
    for (i32 i = 2; i < argc; ++i)
    {
        StrCat(ARGS(cmd), argv[i]);
        if ((i + 1) < argc)
        {
            StrCat(ARGS(cmd), " ");
        }
    }
    StrCat(ARGS(cmd), ";");

    alias.value = StrDup(cmd, EAlloc_Perm);
    bool added = sdict_add(&ms_aliases, aliasKey, &alias);
    ASSERT(added);

    return cmdstat_ok;
}

static cmdstat_t cmd_execfile_fn(i32 argc, const char** argv)
{
    if (argc != 2)
    {
        con_puts(C32_RED, "exec <filename> : executes a script file");
        return cmdstat_err;
    }
    const char* filename = argv[1];
    asset_t asset;
    if (asset_get(filename, &asset))
    {
        cbuf_t cbuf;
        cbuf_new(&cbuf, EAlloc_Perm);
        cbuf_pushback(&cbuf, asset.pData);
        return cbuf_exec(&cbuf);
    }
    return cmdstat_err;
}

static cmdstat_t cmd_wait_fn(i32 argc, const char** argv)
{
    if (argc != 1)
    {
        con_puts(C32_RED, "wait : yields execution for one frame");
        return cmdstat_err;
    }
    return cmdstat_yield;
}
