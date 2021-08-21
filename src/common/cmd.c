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
    cmd_fn_t cmd;
    const char* help;
    const char* argDocs;
} cmddef_t;

static cmdstat_t cmd_text(const char* constText, bool immediate);
static cmdstat_t cmd_exec(const char* line);
static void cmd_log_usage(const char* name, cmddef_t def);
static char** cmd_tokenize(const char* text, i32* argcOut);

static cmdstat_t ExecCmds(void);
static bool IsSpecialChar(char c);
static bool IsLineEnding(char c);
static bool IsComment(const char* text);
static bool IsQuote(char c);
static cmdstat_t cmd_help_fn(i32 argc, const char** argv);
static cmdstat_t cmd_alias_fn(i32 argc, const char** argv);
static cmdstat_t cmd_execfile_fn(i32 argc, const char** argv);
static cmdstat_t cmd_wait_fn(i32 argc, const char** argv);
static cmdstat_t cmd_cmds_fn(i32 argc, const char** argv);
static cmdstat_t cmd_echo_fn(i32 argc, const char** argv);

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
    cmd_reg("help", "[<cmd>]", "show help information", cmd_help_fn);
    cmd_reg("alias", "[<key> \"<value>\"]", "0: list all. 2+: replace <key> with <value> when encountered in cmd parser.", cmd_alias_fn);
    cmd_reg("exec", "<file>", "executes a script file", cmd_execfile_fn);
    cmd_reg("wait", "[<frames>]", "wait 1 frame before executing the next command.", cmd_wait_fn);
    cmd_reg("cmds", "", "list all commands.", cmd_cmds_fn);
    cmd_reg("echo", "<text>", "prints to the console.", cmd_echo_fn);
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
        return cmd_text(alias.value, true);
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

cmdstat_t cmd_enqueue(const char* constText)
{
    return cmd_text(constText, false);
}

cmdstat_t cmd_immediate(const char* constText)
{
    return cmd_text(constText, true);
}

static cmdstat_t cmd_text(const char* constText, bool immediate)
{
    if (!constText)
    {
        ASSERT(false);
        return cmdstat_err;
    }

    char* text = StrDup(constText, EAlloc_Temp);
    char** lines = NULL;
    i32 lineCount = 0;

    i32 len = 0;
    i32 q = 0;
parseline:

    len = 0;
    q = 0;
    while (text[len])
    {
        char c = text[len];
        ++len;
        if (IsQuote(c))
        {
            ++q;
        }
        else if (!(q & 1) && IsLineEnding(c))
        {
            // line ending becomes null terminator
            text[len - 1] = 0;
            break;
        }
    }

    if (len > 0)
    {
        char* line = Perm_Calloc(len + 1);
        memcpy(line, text, len);
        text += len;
        len = 0;

        ++lineCount;
        lines = Temp_Realloc(lines, sizeof(lines[0]) * lineCount);
        lines[lineCount - 1] = line;

        goto parseline;
    }

    if (lineCount > 0)
    {
        if (immediate)
        {
            for (i32 i = lineCount - 1; i >= 0; --i)
            {
                Queue_PushFront(&ms_queue, &lines[i], sizeof(lines[i]));
            }
        }
        else
        {
            for (i32 i = 0; i < lineCount; ++i)
            {
                Queue_Push(&ms_queue, &lines[i], sizeof(lines[i]));
            }
        }
        return ExecCmds();
    }

    return cmdstat_ok;
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
    case '\0':
    case '\n':
    case '\r':
    case ';':
        return true;
    }
}

static bool IsComment(const char* text)
{
    return (text[0] == '/') && (text[1] == '/');
}

static bool IsQuote(char c)
{
    return c == '"';
}

const char* cmd_parse(const char* text, char* token, i32 tokenSize)
{
    ASSERT(text);
    ASSERT(token);
    ASSERT(tokenSize > 0);
    token[0] = 0;
    i32 len = 0;
    char c = 0;

wspace:
    c = *text;

    // whitespace
    while (IsSpace(c))
    {
        ++text;
        c = *text;
    }
    if (!c)
    {
        NullTerminate(token, tokenSize, len);
        return NULL;
    }

    // comments
    if (IsComment(text))
    {
        text += 2;
        while (!IsLineEnding(*text))
        {
            ++text;
        }
        goto wspace;
    }

    // quotes
    if (IsQuote(c))
    {
        ++text; // skip first quote character
        while (true)
        {
            c = *text;
            if (!c)
            {
                // unmatched quote. discard token
                token[0] = 0;
                return NULL;
            }
            if (IsQuote(c))
            {
                // end of quote
                NullTerminate(token, tokenSize, len);
                // skip second quote character
                return text + 1;
            }
            if (len < tokenSize)
            {
                token[len++] = c;
            }
            ++text;
        }
    }

    // special characters
    if (IsSpecialChar(c))
    {
        if (len < tokenSize)
        {
            token[len++] = c;
        }
        NullTerminate(token, tokenSize, len);
        return text + 1;
    }

    // words
    do
    {
        if (len < tokenSize)
        {
            token[len++] = c;
        }
        ++text;
        c = *text;
    } while (!IsSpace(c) && !IsSpecialChar(c) && !IsLineEnding(c) && !IsQuote(c) && !IsComment(text));

    NullTerminate(token, tokenSize, len);
    return text;
}

const char* cmd_getopt(i32 argc, const char** argv, const char* key)
{
    ASSERT(argc >= 0);
    ASSERT(argv);
    ASSERT(key);
    const i32 keyLen = StrLen(key);
    for (i32 i = 0; i < argc; ++i)
    {
        const char* argvi = argv[i];
        ASSERT(argvi);
        if (argvi[0] == '-')
        {
            const char* maybeKey = argvi + 1;
            if (StrICmp(maybeKey, keyLen, key) == 0)
            {
                const char* endOfKey = maybeKey + keyLen;
                if (endOfKey[0] == '=')
                {
                    // -key=value
                    return endOfKey + 1;
                }
                else if (!endOfKey[0])
                {
                    // -key value
                    if (i + 1 < argc)
                    {
                        const char* value = argv[i + 1];
                        ASSERT(value);
                        if (value[0] == '-')
                        {
                            if (IsDigit(value[1]))
                            {
                                // -key -3
                                return value;
                            }
                            else
                            {
                                // -keyA -keyB
                                return "";
                            }
                        }
                        else
                        {
                            return value;
                        }
                    }
                    else
                    {
                        // -key 
                        return "";
                    }
                }
            }
        }
    }
    return NULL;
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
        while (IsSpace(c) && !IsLineEnding(c))
        {
            ++text;
            c = *text;
        }
        if (IsLineEnding(c))
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
    Con_Logf(LogSev_Info, "cmd", "%s %s : %s", name, def.argDocs, def.help);
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
    const i32 cmdCount = ms_cmds.count;
    const char** names = ms_cmds.keys;
    const cmddef_t* cmds = ms_cmds.values;

    Con_Logf(LogSev_Info, "cmd", "displaying %d commands (see also 'help <cmd>'):", cmdCount);

    const u32* indices = StrDict_Sort(&ms_cmds, SDictStrCmp, NULL);
    for (i32 iIndex = 0; iIndex < cmdCount; ++iIndex)
    {
        i32 iCmd = indices[iIndex];
        const char* name = names[iCmd];
        const char* args = cmds[iCmd].argDocs;
        const char* help = cmds[iCmd].help;
        Con_Logf(LogSev_Info, "cmd", "    %-16s: %-16s; %-16s", name, args, help);
    }

    return cmdstat_ok;
}

static cmdstat_t cmd_echo_fn(i32 argc, const char** argv)
{
    char text[1024] = { 0 };
    for (i32 i = 1; i < argc; ++i)
    {
        StrCatf(ARGS(text), "%s ", argv[i]);
    }

    Con_Logf(LogSev_Info, "cmd", text);

    return cmdstat_ok;
}
