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
#include <stdlib.h>

typedef struct cmdalias_s
{
    char* value;
} cmdalias_t;

static cmdstat_t cmd_exec(const char* line);
static char** cmd_tokenize(const char* text, i32* argcOut);

static cmdstat_t ExecCmds(void);
static bool IsSpecialChar(char c);
static bool IsLineEnding(char c);
static cmdstat_t cmd_alias_fn(i32 argc, const char** argv);
static cmdstat_t cmd_execfile_fn(i32 argc, const char** argv);
static cmdstat_t cmd_wait_fn(i32 argc, const char** argv);

static sdict_t ms_cmds;
static sdict_t ms_aliases;
static queue_t ms_queue;
static i32 ms_waits;

// ----------------------------------------------------------------------------

void cmd_sys_init(void)
{
    sdict_new(&ms_cmds, sizeof(cmdfn_t), EAlloc_Perm);
    sdict_new(&ms_aliases, sizeof(cmdalias_t), EAlloc_Perm);
    queue_create(&ms_queue, sizeof(char*), EAlloc_Perm);
    cmd_reg("alias", cmd_alias_fn);
    cmd_reg("exec", cmd_execfile_fn);
    cmd_reg("wait", cmd_wait_fn);
}

void cmd_sys_update(void)
{
    if (ms_waits > 0)
    {
        --ms_waits;
    }

    ExecCmds();
}

void cmd_sys_shutdown(void)
{
    sdict_del(&ms_cmds);
    sdict_del(&ms_aliases);
    char* line = NULL;
    while (queue_trypop(&ms_queue, &line, sizeof(line)))
    {
        pim_free(line);
    }
    queue_destroy(&ms_queue);
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

static cmdstat_t cmd_exec(const char* line)
{
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
        char* toPush = StrDup(alias.value, EAlloc_Perm);
        queue_pushfront(&ms_queue, &toPush, sizeof(toPush));
        return cmdstat_ok;
    }

    // cvars
    cvar_t* cvar = cvar_find(name);
    if (cvar)
    {
        if (argc == 1)
        {
            con_logf(LogSev_Info, "cmd", "'%s' is '%s'", cvar->name, cvar->value);
            return cmdstat_ok;
        }
        if (argc >= 2)
        {
            bool explicit_set = (argc > 2) && (argv[1][0] == '=');
            i32 iArg0 = explicit_set ? 2 : 1;
            i32 iArg1 = iArg0 + 1;
            i32 iArg2 = iArg0 + 2;
            i32 iArg3 = iArg0 + 3;
            const char* v0 = argv[iArg0];
            const char* v1 = (argc > iArg1) ? argv[iArg1] : "0";
            const char* v2 = (argc > iArg2) ? argv[iArg2] : "0";
            const char* v3 = (argc > iArg3) ? argv[iArg3] : "0";
            switch (cvar->type)
            {
            default:
                ASSERT(false);
                con_logf(LogSev_Error, "cmd", "cvar '%s' has unknown type '%d'", cvar->name, cvar->type);
                return cmdstat_err;
            case cvart_text:
                cvar_set_str(cvar, v0);
                break;
            case cvart_float:
                cvar_set_float(cvar, (float)atof(v0));
                break;
            case cvart_int:
                cvar_set_int(cvar, atoi(v0));
                break;
            case cvart_bool:
                cvar_set_bool(cvar, (v0[0] != '0') && (v0[0] != 'f') && (v0[0] != 'F'));
                break;
            case cvart_vector:
            case cvart_point:
            case cvart_color:
                cvar_set_vec(cvar, (float4) { (float)atof(v0), (float)atof(v1), (float)atof(v2), (float)atof(v2) });
                break;
            }
            con_logf(LogSev_Info, "cmd", "'%s' = '%s'", cvar->name, cvar->value);
            return cmdstat_ok;
        }
    }

    con_logf(LogSev_Error, "cmd", "Unknown command '%s'", name);
    return cmdstat_err;
}

cmdstat_t cmd_text(const char* constText)
{
    i32 i, q;
    if (!constText)
    {
        ASSERT(false);
        return cmdstat_err;
    }

    char* text = StrDup(constText, EAlloc_Temp);

parseline:
    i = 0;
    q = 0;
    while (text[i])
    {
        char c = text[i];
        ++i;
        if (c == '"')
        {
            ++q;
        }
        else if (!(q & 1) && (c == '\n'))
        {
            text[i - 1] = 0;
            break;
        }
        else if (!(q & 1) && (c == ';'))
        {
            text[i - 1] = 0;
            break;
        }
    }

    if (i > 0)
    {
        char* line = StrDup(text, EAlloc_Perm);
        queue_push(&ms_queue, &line, sizeof(line));
        text += i;
        goto parseline;
    }

    return ExecCmds();
}

// ----------------------------------------------------------------------------

static cmdstat_t ExecCmds(void)
{
    cmdstat_t status = cmdstat_ok;
    if (!ms_waits)
    {
        char* line = NULL;
        while (queue_trypop(&ms_queue, &line, sizeof(line)))
        {
            status = cmd_exec(line);
            pim_free(line);
            if (ms_waits)
            {
                break;
            }
        }
    }
    return status;
}

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

static bool IsLineEnding(char c)
{
    switch (c)
    {
    default:
        return false;
    case '\n':
    case '\r':
    case ';':
        return true;
    }
}

const char* cmd_parse(const char* text, char** tokenOut)
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
        con_logf(LogSev_Info, "cmd", "Current alias commands:");
        const u32 width = ms_aliases.width;
        const char** names = ms_aliases.keys;
        const cmdalias_t* aliases = ms_aliases.values;
        for (u32 i = 0; i < width; ++i)
        {
            const char* name = names[i];
            if (name)
            {
                con_logf(LogSev_Info, "cmd", "%s : %s", name, aliases[i].value);
            }
        }
        return cmdstat_ok;
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
        con_logf(LogSev_Error, "cmd", "exec <filename> : executes a script file");
        return cmdstat_err;
    }
    const char* filename = argv[1];
    asset_t asset;
    if (asset_get(filename, &asset))
    {
        const char* text = asset.pData;
        cmd_text(text);
        return cmdstat_ok;
    }
    return cmdstat_err;
}

static cmdstat_t cmd_wait_fn(i32 argc, const char** argv)
{
    if (argc > 1)
    {
        i32 count = atoi(argv[1]);
        if (count > 0)
        {
            ms_waits += count;
            return cmdstat_ok;
        }
        else
        {
            con_logf(LogSev_Error, "cmd", "unidentified wait count");
            return cmdstat_err;
        }
    }
    else
    {
        ++ms_waits;
        return cmdstat_ok;
    }
}
