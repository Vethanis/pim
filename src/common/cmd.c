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

typedef struct cmddef_s
{
    const char* help;
    const char* argDocs;
    cmd_fn_t cmd;
} cmddef_t;

static cmdstat_t cmd_text(const char* constText, bool immediate);
static cmdstat_t cmd_exec(const char* line);
static void cmd_log_usage(const char* name, cmddef_t def);
static char** cmd_tokenize(const char* text, i32* argcOut);

static cmdstat_t ExecCmds(void);
static bool IsSpecialChar(char c);
static bool IsLineEnding(char c);
static cmdstat_t cmd_help_fn(i32 argc, const char** argv);
static cmdstat_t cmd_alias_fn(i32 argc, const char** argv);
static cmdstat_t cmd_execfile_fn(i32 argc, const char** argv);
static cmdstat_t cmd_wait_fn(i32 argc, const char** argv);
static cmdstat_t cmd_cmds_fn(i32 argc, const char** argv);

static StrDict ms_cmds;
static StrDict ms_aliases;
static Queue ms_queue;
static i32 ms_waits;

// ----------------------------------------------------------------------------

void cmd_sys_init(void)
{
    StrDict_New(&ms_cmds, sizeof(cmddef_t), EAlloc_Perm);
    StrDict_New(&ms_aliases, sizeof(cmdalias_t), EAlloc_Perm);
    Queue_New(&ms_queue, sizeof(char*), EAlloc_Perm);
    cmd_reg("help", "", "show help information", cmd_help_fn);
    cmd_reg("alias", "[<key> <command> [ <arg1> ... ]]", "no args: list all aliases. 1 or more args: alias the given key to the given command and argument list", cmd_alias_fn);
    cmd_reg("exec", "<file>", "executes a script file", cmd_execfile_fn);
    cmd_reg("wait", "[<ms>]", "wait the given number of milliseconds (1 by default) before executing the next command.", cmd_wait_fn);
    cmd_reg("cmds", "", "list all registered commands.", cmd_cmds_fn);
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
    StrDict_Del(&ms_cmds);
    StrDict_Del(&ms_aliases);
    char* line = NULL;
    while (Queue_TryPop(&ms_queue, &line, sizeof(line)))
    {
        Mem_Free(line);
    }
    Queue_Del(&ms_queue);
}

void cmd_reg(const char* name, const char* argDocs, const char* helpText, cmd_fn_t fn)
{
    ASSERT(name);
    ASSERT(argDocs);
    ASSERT(helpText);
    ASSERT(fn);

    cmddef_t def =
    {
        .help = helpText,
        .argDocs = argDocs,
        .cmd = fn
    };

    if (!StrDict_Add(&ms_cmds, name, &def))
    {
        StrDict_Set(&ms_cmds, name, &def);
    }
}

bool cmd_exists(const char* name)
{
    ASSERT(name);
    return StrDict_Find(&ms_cmds, name) != -1;
}

const char* cmd_complete(const char* namePart)
{
    ASSERT(namePart);
    const i32 partLen = StrLen(namePart);
    const u32 width = ms_cmds.width;
    char** names = ms_cmds.keys;
    for (u32 i = 0; i < width; ++i)
    {
        char* name = names[i];
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
    cmddef_t def = { 0 };
    if (StrDict_Get(&ms_cmds, name, &def))
    {
        return def.cmd(argc, argv);
    }

    // aliases (macros to expand into front of cbuf)
    cmdalias_t alias = { 0 };
    if (StrDict_Get(&ms_aliases, name, &alias))
    {
        char* toPush = StrDup(alias.value, EAlloc_Perm);
        Queue_PushFront(&ms_queue, &toPush, sizeof(toPush));
        return cmdstat_ok;
    }

    // cvars
    ConVar* cvar = ConVar_Find(name);
    if (cvar)
    {
        if (argc == 1)
        {
            Con_Logf(LogSev_Info, "cmd", "'%s' is '%s'", cvar->name, cvar->value);
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
                Con_Logf(LogSev_Error, "cmd", "cvar '%s' has unknown type '%d'", cvar->name, cvar->type);
                return cmdstat_err;
            case cvart_text:
                ConVar_SetStr(cvar, v0);
                break;
            case cvart_float:
                ConVar_SetFloat(cvar, (float)atof(v0));
                break;
            case cvart_int:
                ConVar_SetInt(cvar, atoi(v0));
                break;
            case cvart_bool:
                ConVar_SetBool(cvar, (v0[0] != '0') && (v0[0] != 'f') && (v0[0] != 'F'));
                break;
            case cvart_vector:
            case cvart_point:
            case cvart_color:
                ConVar_SetVec(cvar, (float4) { (float)atof(v0), (float)atof(v1), (float)atof(v2), (float)atof(v2) });
                break;
            }
            Con_Logf(LogSev_Info, "cmd", "'%s' = '%s'", cvar->name, cvar->value);
            return cmdstat_ok;
        }
    }

    Con_Logf(LogSev_Error, "cmd", "Unknown command '%s'", name);
    return cmdstat_err;
}

cmdstat_t cmd_enqueue(const char* text)
{
    return cmd_text(text, false);
}

cmdstat_t cmd_immediate(const char* text)
{
    return cmd_text(text, true);
}

static cmdstat_t cmd_text(const char* constText, bool immediate)
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
        if (immediate)
        {
            Queue_PushFront(&ms_queue, &line, sizeof(line));
        }
        else
        {
            Queue_Push(&ms_queue, &line, sizeof(line));
        }
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
        while (Queue_TryPop(&ms_queue, &line, sizeof(line)))
        {
            status = cmd_exec(line);
            Mem_Free(line);
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

const char* cmd_parse(const char* text, char* token, i32 tokenSize)
{
    ASSERT(text);
    ASSERT(token);
    ASSERT(tokenSize > 0);
    i32 len = 0;
    char c = 0;

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
                NullTerminate(token, tokenSize, len);
                return text;
            }
            if (len < tokenSize)
            {
                token[len] = c;
                ++len;
            }
        }
    }

    // special characters
    if (IsSpecialChar(c))
    {
        if (len < tokenSize)
        {
            token[len] = c;
            ++len;
        }
        NullTerminate(token, tokenSize, len);
        return text + 1;
    }

    // words
    do
    {
        if (len < tokenSize)
        {
            token[len] = c;
            ++len;
        }
        ++text;
        c = *text;
    } while ((c > ' ') && !IsSpecialChar(c));

    NullTerminate(token, tokenSize, len);
    return text;
}

static char** cmd_tokenize(const char* text, i32* argcOut)
{
    ASSERT(text);
    ASSERT(argcOut);
    char stackToken[1024];
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
        stackToken[0] = 0;
        text = cmd_parse(text, stackToken, NELEM(stackToken));
        if (stackToken[0])
        {
            ++argc;
            argv = Mem_Realloc(EAlloc_Temp, argv, sizeof(*argv) * argc);
            argv[argc - 1] = StrDup(stackToken, EAlloc_Temp);
        }
    }
    *argcOut = argc;
    return argv;
}

static cmdstat_t cmd_help_fn(i32 argc, const char** argv)
{
    if (argc <= 1)
    {
        Con_Logf(LogSev_Info, "cmd", "cmds - Display a full list of commands.");
        Con_Logf(LogSev_Info, "cmd", "help - This help message.");
        Con_Logf(LogSev_Info, "cmd", "help <cmd> - Describes a command");

        return cmdstat_ok;
    }

    const char* name = argv[1];
    cmddef_t def = { 0 };
    if (!StrDict_Get(&ms_cmds, name, &def))
    {
        Con_Logf(LogSev_Error, "cmd", "No help for %s.", name);
        return cmdstat_err;
    }

    cmd_log_usage(name, def);

    return cmdstat_ok;
}

static void cmd_log_usage(const char* name, cmddef_t def)
{
    Con_Logf(LogSev_Info, "cmd", "%s %s - %s", name, def.argDocs, def.help);
}

static cmdstat_t cmd_alias_fn(i32 argc, const char** argv)
{
    if (argc <= 1)
    {
        Con_Logf(LogSev_Info, "cmd", "Current alias commands:");
        const u32 width = ms_aliases.width;
        const char** names = ms_aliases.keys;
        const cmdalias_t* aliases = ms_aliases.values;
        for (u32 i = 0; i < width; ++i)
        {
            const char* name = names[i];
            if (name)
            {
                Con_Logf(LogSev_Info, "cmd", "%s : %s", name, aliases[i].value);
            }
        }
        return cmdstat_ok;
    }

    const char* aliasKey = argv[1];

    cmdalias_t alias = { 0 };
    if (StrDict_Rm(&ms_aliases, aliasKey, &alias))
    {
        Mem_Free(alias.value);
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
    bool added = StrDict_Add(&ms_aliases, aliasKey, &alias);
    ASSERT(added);

    return cmdstat_ok;
}

static cmdstat_t cmd_execfile_fn(i32 argc, const char** argv)
{
    if (argc != 2)
    {
        Con_Logf(LogSev_Error, "cmd", "exec <filename> : executes a script file");
        return cmdstat_err;
    }
    const char* filename = argv[1];
    asset_t asset;
    if (Asset_Get(filename, &asset))
    {
        const char* text = asset.pData;
        cmd_enqueue(text);
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
            Con_Logf(LogSev_Error, "cmd", "unidentified wait count");
            return cmdstat_err;
        }
    }
    else
    {
        ++ms_waits;
        return cmdstat_ok;
    }
}

static cmdstat_t cmd_cmds_fn(i32 argc, const char** argv)
{
    Con_Logf(LogSev_Info, "cmd", "displaying %i commands (`help <cmd>` for more info):", ms_cmds.count);

    u32* indices = StrDict_Sort(&ms_cmds, SDictStrCmp, NULL);

    cmddef_t def = { 0 };
    for (u32 i = 0; i < ms_cmds.count; i++)
    {
        u32 index = indices[i];
        const char* key = ms_cmds.keys[index];
        Con_Logf(LogSev_Info, "cmd", "\t%s", key);
    }

    return cmdstat_ok;
}
