#pragma once

#include "common/macro.h"
#include "math/types.h"

PIM_C_BEGIN

typedef enum
{
    pr_classname_worldspawn = 0,

    // info
    pr_classname_info_player_start,
    pr_classname_info_player_start2,
    pr_classname_info_teleport_destination,
    pr_classname_info_player_deathmatch,
    pr_classname_info_player_coop,
    pr_classname_info_intermission,
    pr_classname_info_null,

    // lights
    pr_classname_light,
    pr_classname_light_fluoro,
    pr_classname_light_fluorospark,
    pr_classname_light_flame_large_yellow,
    pr_classname_light_torch_small_walltorch,

    // triggers
    pr_classname_trigger_multiple,
    pr_classname_trigger_teleport,
    pr_classname_trigger_changelevel,
    pr_classname_trigger_setskill,
    pr_classname_trigger_onlyregistered,

    // funcs
    pr_classname_func_door,
    pr_classname_func_wall,
    pr_classname_func_bossgate,
    pr_classname_func_episodegate,

    // monsters
    pr_classname_monster_zombie,

    // items and weapons
    pr_classname_item_health,
    pr_classname_item_armorInv,
    pr_classname_item_armor2,
    pr_classname_item_spikes,
    pr_classname_item_rockets,
    pr_classname_item_shells,
    pr_classname_item_cells,
    pr_classname_item_artifact_super_damage,
    pr_classname_weapon_nailgun,
    pr_classname_weapon_supernailgun,
    pr_classname_weapon_rocketlauncher,
    pr_classname_weapon_grenadelauncher,
    pr_classname_weapon_supershotgun,
    pr_classname_weapon_lightning,

    // ambients
    pr_classname_ambient_drip,
    pr_classname_ambient_drone,

    // misc
    pr_classname_fireball,

    pr_classname_COUNT
} pr_classname_t;

typedef struct pr_string_s
{
    i16 index; // < 0 == NULL
} pr_string_t;

typedef struct pr_worldspawn_s
{
    pr_string_t wad;
    pr_string_t message;
    i16 worldtype;
    i16 sounds;
} pr_worldspawn_t;

// info_player_start
typedef struct pr_player_start_s
{
    float3 origin;
    float angle;
} pr_player_start_t;

// light_*
typedef struct pr_light_s
{
    float3 origin;
    float light;
    i16 style;
} pr_light_t;

// func_wall
typedef struct pr_wall_s
{
    pr_string_t targetname;
    u16 spawnflags;
    i16 model;
} pr_wall_t;

// func_door
typedef struct pr_door_s
{
    float speed;
    float angle;
    pr_string_t targetname;
    u16 spawnflags;
    i16 model;
    i16 sounds;
    i16 wait;
} pr_door_t;

// func_bossgate
typedef struct pr_bossgate_s
{
    i16 model;
} pr_bossgate_t;

// func_episodegate
typedef struct pr_episodegate_s
{
    pr_string_t message;
    u16 spawnflags;
    i16 model;
} pr_episodegate_t;

// trigger_teleport
typedef struct pr_teleport_s
{
    pr_string_t target;
    u16 spawnflags;
    i16 model;
} pr_teleport_t;

// trigger_changelevel
typedef struct pr_changelevel_s
{
    pr_string_t map;
    u16 spawnflags;
    i16 model;
} pr_changelevel_t;

// trigger_setskill
typedef struct pr_setskill_s
{
    pr_string_t message;
    u16 spawnflags;
    i16 model;
    i16 wait;
} pr_setskill_t;

// trigger_multiple
typedef struct pr_trigmult_s
{
    float angle;
    pr_string_t targetname;
    pr_string_t message;
    u16 spawnflags;
    i16 model;
    i16 wait;
    i16 sounds;
    i16 health;
} pr_trigmult_t;

// trigger_onlyregistered
typedef struct pr_onlyregistered_s
{
    pr_string_t targetname;
    pr_string_t killtarget;
    pr_string_t target;
    pr_string_t message;
    i16 model;
    i16 wait;
} pr_onlyregistered_t;

// item_* and weapon_*
typedef struct pr_item_s
{
    float3 origin;
    u16 spawnflags;
} pr_item_t;

// ambient_*
typedef struct pr_ambient_s
{
    float3 origin;
} pr_ambient_t;

// misc_fireball
typedef struct pr_fireball_s
{
    float3 origin;
    float speed;
} pr_fireball_t;

typedef struct pr_entity_s
{
    pr_classname_t type;
    union
    {
        pr_worldspawn_t worldspawn;
        pr_player_start_t playerstart;
        pr_light_t light;
        pr_wall_t wall;
        pr_door_t door;
        pr_bossgate_t bossgate;
        pr_episodegate_t episodegate;
        pr_teleport_t teleport;
        pr_changelevel_t changelevel;
        pr_setskill_t setskill;
        pr_trigmult_t trigmult;
        pr_onlyregistered_t onlyregistered;
        pr_item_t item;
        pr_ambient_t ambient;
        pr_fireball_t fireball;
    };
} pr_entity_t;

typedef struct progs_s
{
    i32 numstrings;
    char** strings;

    i32 numentities;
    pr_entity_t* entities;
    pr_string_t* names; // aka targetname
} progs_t;

void progs_parse(progs_t* progs, const char* text);
void progs_del(progs_t* progs);

PIM_C_END
