#include "logic/progs.h"
#include "common/stringutil.h"
#include "common/cmd.h"
#include "allocator/allocator.h"
#include "containers/sdict.h"
#include "math/float3_funcs.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static char const *const kClassnames[] =
{
    "worldspawn",

    // info
    "info_player_start",
    "info_player_start2",
    "info_teleport_destination",
    "info_player_deathmatch",
    "info_player_coop",
    "info_intermission",
    "info_null",

    // lights
    "light",
    "light_fluoro",
    "light_fluorospark",
    "light_flame_large_yellow",
    "light_torch_small_walltorch",

    // triggers
    "trigger_multiple",
    "trigger_teleport",
    "trigger_changelevel",
    "trigger_setskill",
    "trigger_onlyregistered",

    // funcs
    "func_door",
    "func_wall",
    "func_bossgate",
    "func_episodegate",

    // monsters
    "monster_zombie",

    // items and weapons
    "item_health",
    "item_armorInv",
    "item_armor2",
    "item_spikes",
    "item_rockets",
    "item_shells",
    "item_cells",
    "item_artifact_super_damage",
    "weapon_nailgun",
    "weapon_supernailgun",
    "weapon_rocketlauncher",
    "weapon_grenadelauncher",
    "weapon_supershotgun",
    "weapon_lightning",

    // ambients
    "ambient_drip",
    "ambient_drone",

    // misc
    "misc_fireball",
};
SASSERT(NELEM(kClassnames) == pr_classname_COUNT);

static i32 FindClass(const char* txt)
{
    for (i32 i = 0; i < NELEM(kClassnames); ++i)
    {
        if (StrCmp(txt, PIM_PATH, kClassnames[i]) == 0)
        {
            return i;
        }
    }
    return -1;
}

static pr_string_t FindString(progs_t* progs, const char* str)
{
    i32 len = progs->numstrings;
    char** strings = progs->strings;
    for (i32 i = 0; i < len; ++i)
    {
        if (StrCmp(strings[i], PIM_PATH, str) == 0)
        {
            return (pr_string_t) { i };
        }
    }
    Perm_Reserve(strings, len + 1);
    strings[len] = StrDup(str, EAlloc_Perm);
    progs->numstrings = len + 1;
    progs->strings = strings;
    return (pr_string_t) { len };
}

static void AddEntity(progs_t* progs, pr_entity_t ent)
{
    i32 back = progs->numentities++;
    i32 len = back + 1;
    Perm_Reserve(progs->entities, len);
    progs->entities[back] = ent;
}

static float3 ParseOrigin(const StrDict* dict)
{
    float3 origin = f3_0;
    char* value = NULL;
    if (StrDict_Get(dict, "origin", &value))
    {
        sscanf(value, "%f %f %f", &origin.x, &origin.y, &origin.z);
    }
    return origin;
}

static float ParseAngle(const StrDict* dict)
{
    float angle = 0.0f;
    char* value = NULL;
    if (StrDict_Get(dict, "angle", &value))
    {
        angle = f1_radians((float)atof(value)) - kPi * 0.5f;
        angle = f1_mod(angle, kTau);
    }
    return angle;
}

static void ParseWorldspawn(progs_t* progs, const StrDict* dict)
{
    pr_worldspawn_t ws = { 0 };
    ws.message.index = -1;
    ws.wad.index = -1;
    ws.sounds = -1;
    ws.worldtype = -1;

    char* value = NULL;
    if (StrDict_Get(dict, "wad", &value))
    {
        ws.wad = FindString(progs, value);
    }
    if (StrDict_Get(dict, "message", &value))
    {
        ws.message = FindString(progs, value);
    }
    if (StrDict_Get(dict, "worldtype", &value))
    {
        ws.worldtype = (i16)atoi(value);
    }
    if (StrDict_Get(dict, "sounds", &value))
    {
        ws.sounds = (i16)atoi(value);
    }

    pr_entity_t ent = { 0 };
    ent.type = pr_classname_worldspawn;
    ent.worldspawn = ws;
    AddEntity(progs, ent);
}

static void ParsePlayerStart(progs_t* progs, const StrDict* dict)
{
    pr_player_start_t ps = { 0 };
    ps.angle = ParseAngle(dict);
    ps.origin = ParseOrigin(dict);
    pr_entity_t ent = { 0 };
    ent.type = pr_classname_info_player_start;
    ent.playerstart = ps;
    AddEntity(progs, ent);
}

static void ParseLight(progs_t* progs, const StrDict* dict, i32 iclass)
{
    pr_light_t light = { 0 };
    light.origin = ParseOrigin(dict);
    light.light = 200.0f;
    light.style = 0;

    char* value = NULL;
    if (StrDict_Get(dict, "light", &value))
    {
        light.light = (float)atof(value);
    }
    if (StrDict_Get(dict, "style", &value))
    {
        light.style = (i16)atoi(value);
    }

    pr_entity_t ent = { 0 };
    ent.type = iclass;
    ent.light = light;
    AddEntity(progs, ent);
}

static void ParseClass(progs_t* progs, const StrDict* dict, i32 iclass)
{
    switch (iclass)
    {
    default:
        break;
    case pr_classname_worldspawn:
        ParseWorldspawn(progs, dict);
        break;
    case pr_classname_info_player_start:
        ParsePlayerStart(progs, dict);
        break;
    case pr_classname_light:
    case pr_classname_light_fluoro:
    case pr_classname_light_fluorospark:
    case pr_classname_light_flame_large_yellow:
    case pr_classname_light_torch_small_walltorch:
        ParseLight(progs, dict, iclass);
        break;
    }
}

static void ParseDict(progs_t* progs, StrDict* dict)
{
    char* value = NULL;
    if (StrDict_Get(dict, "classname", &value))
    {
        i32 iclass = FindClass(value);
        if (iclass >= 0)
        {
            ParseClass(progs, dict, iclass);
        }
    }
    StrDict_Clear(dict);
}

static void ExpandEscapes(char* text)
{
    i32 len = StrLen(text);
    while (len > 0)
    {
        char c = text[0];
        ASSERT(c);
        char d = text[1];
        if (c == '\\')
        {
            bool escapeValid = true;
            switch (d)
            {
            default:
                escapeValid = false;
                break;
            case 'n':
                c = '\n';
                break;
            case 'r':
                c = '\r';
                break;
            case 't':
                c = '\t';
                break;
            case '\\':
                c = '\\';
                break;
            case '\'':
                c = '\'';
                break;
            case '\"':
                c = '\"';
                break;
            }

            if (escapeValid)
            {
                text[0] = c;
                --len;
                memmove(text + 1, text + 2, len);
            }
        }
        ++text;
        --len;
    }
}

void progs_parse(progs_t* progs, const char* text)
{
    ASSERT(progs);
    ASSERT(text);
    if (!progs)
    {
        return;
    }
    memset(progs, 0, sizeof(*progs));
    if (!text)
    {
        return;
    }

    StrDict dict;
    StrDict_New(&dict, sizeof(char*), EAlloc_Temp);
    StrDict_Reserve(&dict, 16);

    char key[PIM_PATH] = { 0 };
    i32 braces = 0;
    i32 count = 0;
    while (true)
    {
        char* token = NULL;
        text = Cmd_Parse(text, &token);
        if (!text)
        {
            break;
        }
        char c = *token;
        if (!c)
        {
            break;
        }
        if (c == '{')
        {
            ++braces;
            continue;
        }
        if (c == '}')
        {
            --braces;
            if (braces == 0)
            {
                ParseDict(progs, &dict);
            }
            continue;
        }
        if ((braces > 2) || (braces < 0))
        {
            // malformed file
            break;
        }
        if (braces == 2)
        {
            // brush not yet handled
            continue;
        }

        {
            ++count;

            ExpandEscapes(token);

            if (count & 1)
            {
                StrCpy(ARGS(key), token);
            }
            else
            {
                if (!StrDict_Add(&dict, key, &token))
                {
                    ASSERT(false);
                }
                memset(key, 0, sizeof(key));
            }
        }
    }
}

void progs_del(progs_t* progs)
{
    if (progs)
    {
        const i32 numstrings = progs->numstrings;
        char** strings = progs->strings;
        for (i32 i = 0; i < numstrings; ++i)
        {
            Mem_Free(strings[i]);
            strings[i] = NULL;
        }
        Mem_Free(strings);
        Mem_Free(progs->entities);
        Mem_Free(progs->names);
        memset(progs, 0, sizeof(*progs));
    }
}
