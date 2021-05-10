#pragma once
/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

//=====================================
// compiler

#define GL_QUAKE            1
// gl_model.h, glquake.h replace model.h, d_iface.h

//=====================================
// quakedef

#define QUAKE_GAME  // as opposed to utilities

#define VERSION             1.09
#define GLQUAKE_VERSION     1.00
#define D3DQUAKE_VERSION    0.01
#define WINQUAKE_VERSION    0.996
#define LINUX_VERSION       1.30
#define X11_VERSION         1.10

//define PARANOID // speed sapping error checking

#define GAMENAME            "id1"
#define CACHE_SIZE          64          // used to align key data structures
#define UNUSED(x)           (void)(x)   // for pesky compiler / lint warnings

#define MINIMUM_MEMORY      0x550000
#define MINIMUM_MEMORY_LEVELPAK (MINIMUM_MEMORY + 0x100000)

#define MAX_NUM_ARGVS 50

// up / down
#define PITCH 0
// left / right
#define YAW  1
// fall over
#define ROLL 2

#define MAX_QPATH       64      // max length of a quake game pathname
#define MAX_OSPATH      128     // max length of a filesystem pathname

#define ON_EPSILON      0.1     // point on plane side epsilon

// precision epsilon to avoid NaNs and denormals from division
#define Q_EPSILON       (2.384185791e-7f) 

// per-level limits

#define MAX_EDICTS  600   // FIXME: ouch! ouch! ouch!
#define MAX_LIGHTSTYLES 64
#define MAX_MODELS  256   // these are sent over the net as bytes
#define MAX_SOUNDS  256   // so they cannot be blindly increased

#define SAVEGAME_COMMENT_LENGTH 39

#define MAX_STYLESTRING 64

// stats are integers communicated to the client by the server

#define MAX_CL_STATS  32
#define STAT_HEALTH   0
#define STAT_FRAGS   1
#define STAT_WEAPON   2
#define STAT_AMMO   3
#define STAT_ARMOR   4
#define STAT_WEAPONFRAME 5
#define STAT_SHELLS   6
#define STAT_NAILS   7
#define STAT_ROCKETS  8
#define STAT_CELLS   9
#define STAT_ACTIVEWEAPON 10
#define STAT_TOTALSECRETS 11
#define STAT_TOTALMONSTERS 12
#define STAT_SECRETS  13  // bumped on client side by svc_foundsecret
#define STAT_MONSTERS  14  // bumped by svc_killedmonster

// stock defines

#define IT_SHOTGUN    1
#define IT_SUPER_SHOTGUN  2
#define IT_NAILGUN    4
#define IT_SUPER_NAILGUN  8
#define IT_GRENADE_LAUNCHER  16
#define IT_ROCKET_LAUNCHER  32
#define IT_LIGHTNING   64
#define IT_SUPER_LIGHTNING      128
#define IT_SHELLS               256
#define IT_NAILS                512
#define IT_ROCKETS              1024
#define IT_CELLS                2048
#define IT_AXE                  4096
#define IT_ARMOR1               8192
#define IT_ARMOR2               16384
#define IT_ARMOR3               32768
#define IT_SUPERHEALTH          65536
#define IT_KEY1                 131072
#define IT_KEY2                 262144
#define IT_INVISIBILITY   524288
#define IT_INVULNERABILITY  1048576
#define IT_SUIT     2097152
#define IT_QUAD     4194304
#define IT_SIGIL1               (1<<28)
#define IT_SIGIL2               (1<<29)
#define IT_SIGIL3               (1<<30)
#define IT_SIGIL4               (1<<31)

//rogue changed and added defines

#define RIT_SHELLS              128
#define RIT_NAILS               256
#define RIT_ROCKETS             512
#define RIT_CELLS               1024
#define RIT_AXE                 2048
#define RIT_LAVA_NAILGUN        4096
#define RIT_LAVA_SUPER_NAILGUN  8192
#define RIT_MULTI_GRENADE       16384
#define RIT_MULTI_ROCKET        32768
#define RIT_PLASMA_GUN          65536
#define RIT_ARMOR1              8388608
#define RIT_ARMOR2              16777216
#define RIT_ARMOR3              33554432
#define RIT_LAVA_NAILS          67108864
#define RIT_PLASMA_AMMO         134217728
#define RIT_MULTI_ROCKETS       268435456
#define RIT_SHIELD              536870912
#define RIT_ANTIGRAV            1073741824
#define RIT_SUPERHEALTH         2147483648

//MED 01/04/97 added hipnotic defines
//hipnotic added defines
#define HIT_PROXIMITY_GUN_BIT 16
#define HIT_MJOLNIR_BIT       7
#define HIT_LASER_CANNON_BIT  23
#define HIT_PROXIMITY_GUN   (1<<HIT_PROXIMITY_GUN_BIT)
#define HIT_MJOLNIR         (1<<HIT_MJOLNIR_BIT)
#define HIT_LASER_CANNON    (1<<HIT_LASER_CANNON_BIT)
#define HIT_WETSUIT         (1<<(23+2))
#define HIT_EMPATHY_SHIELDS (1<<(23+3))

#define MAX_SCOREBOARD      16
#define MAX_SCOREBOARDNAME  32

#define SOUND_CHANNELS      8

//=====================================
// bspfile

// upper design bounds

#define MAX_MAP_HULLS           (4)
#define MAX_MAP_MODELS          (256)
#define MAX_MAP_BRUSHES         (4096)
#define MAX_MAP_ENTITIES        (1024)
#define MAX_MAP_ENTSTRING       (65536)
#define MAX_MAP_PLANES          (32767)
#define MAX_MAP_NODES           (32767)  // because negative shorts are contents
#define MAX_MAP_CLIPNODES       (32767)  //
#define MAX_MAP_LEAFS           (8192)
#define MAX_MAP_VERTS           (65535)
#define MAX_MAP_FACES           (65535)
#define MAX_MAP_MARKSURFACES    (65535)
#define MAX_MAP_TEXINFO         (4096)
#define MAX_MAP_EDGES           (256000)
#define MAX_MAP_SURFEDGES       (512000)
#define MAX_MAP_TEXTURES        (512)
#define MAX_MAP_MIPTEX          (0x200000)
#define MAX_MAP_LIGHTING        (0x100000)
#define MAX_MAP_VISIBILITY      (0x100000)
#define MAX_MAP_PORTALS         (65536)

// key / value pair sizes
#define MAX_KEY     (32)
#define MAX_VALUE   (1024)

#define BSPVERSION  29
#define TOOLVERSION 2

#define LUMP_ENTITIES       0
#define LUMP_PLANES         1
#define LUMP_TEXTURES       2
#define LUMP_VERTEXES       3
#define LUMP_VISIBILITY     4
#define LUMP_NODES          5
#define LUMP_TEXINFO        6
#define LUMP_FACES          7
#define LUMP_LIGHTING       8
#define LUMP_CLIPNODES      9
#define LUMP_LEAFS          10
#define LUMP_MARKSURFACES   11
#define LUMP_EDGES          12
#define LUMP_SURFEDGES      13
#define LUMP_MODELS         14
#define HEADER_LUMPS        15

// 0-2 are axial planes
#define PLANE_X         0
#define PLANE_Y         1
#define PLANE_Z         2
// 3-5 are non-axial planes snapped to the nearest
#define PLANE_ANYX      3
#define PLANE_ANYY      4
#define PLANE_ANYZ      5

#define CONTENTS_EMPTY          (-1)
#define CONTENTS_SOLID          (-2)
#define CONTENTS_WATER          (-3)
#define CONTENTS_SLIME          (-4)
#define CONTENTS_LAVA           (-5)
#define CONTENTS_SKY            (-6)
#define CONTENTS_ORIGIN         (-7) // removed at csg time
#define CONTENTS_CLIP           (-8) // changed to contents_solid
#define CONTENTS_CURRENT_0      (-9)
#define CONTENTS_CURRENT_90     (-10)
#define CONTENTS_CURRENT_180    (-11)
#define CONTENTS_CURRENT_270    (-12)
#define CONTENTS_CURRENT_UP     (-13)
#define CONTENTS_CURRENT_DOWN   (-14)

#define MIPLEVELS 4
#define TEX_SPECIAL 1 // sky or slime, no lightmap or 256 subdivision
#define MAXLIGHTMAPS 4

#define AMBIENT_WATER   0
#define AMBIENT_SKY     1
#define AMBIENT_SLIME   2
#define AMBIENT_LAVA    3
#define NUM_AMBIENTS    4   // automatic ambient sounds

//=====================================
// vid

#define VID_CBITS       6
#define VID_GRADES      (1 << VID_CBITS)

//=====================================
// wad

#define CMP_NONE        0
#define CMP_LZSS        1

#define TYP_NONE        0
#define TYP_LABEL       1

#define TYP_LUMPY       64    // 64 + grab command number
#define TYP_PALETTE     64
#define TYP_QTEX        65
#define TYP_QPIC        66
#define TYP_SOUND       67
#define TYP_MIPTEX      68

//=====================================
// net

// This makes anyone on id's net privileged
// Use for multiplayer testing only - VERY dangerous!!!
// #define IDGODS

#define NET_NAMELEN             64

#define MAX_MSGLEN              8000    // max length of a reliable message
#define MAX_DATAGRAM            1024    // max length of unreliable message

#define NET_MAXMESSAGE          8192
#define NET_HEADERSIZE          (2 * sizeof(u32))
#define NET_DATAGRAMSIZE        (MAX_DATAGRAM + NET_HEADERSIZE)

// NetHeader flags
#define NETFLAG_LENGTH_MASK     0x0000ffff
#define NETFLAG_DATA            0x00010000
#define NETFLAG_ACK             0x00020000
#define NETFLAG_NAK             0x00040000
#define NETFLAG_EOM             0x00080000
#define NETFLAG_UNRELIABLE      0x00100000
#define NETFLAG_CTL             0x80000000

#define NET_PROTOCOL_VERSION    3

#define CCREQ_CONNECT           0x01
#define CCREQ_SERVER_INFO       0x02
#define CCREQ_PLAYER_INFO       0x03
#define CCREQ_RULE_INFO         0x04

#define CCREP_ACCEPT            0x81
#define CCREP_REJECT            0x82
#define CCREP_SERVER_INFO       0x83
#define CCREP_PLAYER_INFO       0x84
#define CCREP_RULE_INFO         0x85

#define MAX_NET_DRIVERS         8
#define HOSTCACHESIZE           8

//=====================================
// protocol

#define PROTOCOL_VERSION        15

// if the high bit of the servercmd is set, the low bits are fast update flags:
#define U_MOREBITS              (1<<0)
#define U_ORIGIN1               (1<<1)
#define U_ORIGIN2               (1<<2)
#define U_ORIGIN3               (1<<3)
#define U_ANGLE2                (1<<4)
#define U_NOLERP                (1<<5)  // don't interpolate movement
#define U_FRAME                 (1<<6)
#define U_SIGNAL                (1<<7)  // just differentiates from other updates

// svc_update can pass all of the fast update bits, plus more
#define U_ANGLE1                (1<<8)
#define U_ANGLE3                (1<<9)
#define U_MODEL                 (1<<10)
#define U_COLORMAP              (1<<11)
#define U_SKIN                  (1<<12)
#define U_EFFECTS               (1<<13)
#define U_LONGENTITY            (1<<14)

#define SU_VIEWHEIGHT           (1<<0)
#define SU_IDEALPITCH           (1<<1)
#define SU_PUNCH1               (1<<2)
#define SU_PUNCH2               (1<<3)
#define SU_PUNCH3               (1<<4)
#define SU_VELOCITY1            (1<<5)
#define SU_VELOCITY2            (1<<6)
#define SU_VELOCITY3            (1<<7)
//define SU_AIMENT              (1<<8)  AVAILABLE BIT
#define SU_ITEMS                (1<<9)
#define SU_ONGROUND             (1<<10)  // no data follows, the bit is it
#define SU_INWATER              (1<<11)  // no data follows, the bit is it
#define SU_WEAPONFRAME          (1<<12)
#define SU_ARMOR                (1<<13)
#define SU_WEAPON               (1<<14)

// a sound with no channel is a local only sound
#define SND_VOLUME              (1<<0)  // a byte
#define SND_ATTENUATION         (1<<1)  // a byte
#define SND_LOOPING             (1<<2)  // a long

// defaults for clientinfo messages
#define DEFAULT_VIEWHEIGHT      22

// game types sent by serverinfo
// these determine which intermission screen plays
#define GAME_COOP               0
#define GAME_DEATHMATCH         1

// note that there are some defs.qc that mirror to these numbers
// also related to svc_strings[] in cl_parse

// server to client remote procedure calls
#define svc_bad                 0
#define svc_nop                 1
#define svc_disconnect          2
#define svc_updatestat          3   // [byte] [long]
#define svc_version             4   // [long] server version
#define svc_setview             5   // [short] entity number
#define svc_sound               6   // <see code>
#define svc_time                7   // [float] server time
#define svc_print               8   // [string] null terminated string
#define svc_stufftext           9   // [string] stuffed into client's console buffer
                                    // the string should be \n terminated
#define svc_setangle            10  // [angle3] set the view angle to this absolute value
#define svc_serverinfo          11  // [long] version
                                    // [string] signon string
                                    // [string]..[0]model cache
                                    // [string]...[0]sounds cache
#define svc_lightstyle          12  // [byte] [string]
#define svc_updatename          13  // [byte] [string]
#define svc_updatefrags         14  // [byte] [short]
#define svc_clientdata          15  // <shortbits + data>
#define svc_stopsound           16  // <see code>
#define svc_updatecolors        17  // [byte] [byte]
#define svc_particle            18  // [vec3] <variable>
#define svc_damage              19
#define svc_spawnstatic         20
// svc_spawnbinary              21
#define svc_spawnbaseline       22
#define svc_temp_entity         23
#define svc_setpause            24 // [byte] on / off
#define svc_signonnum           25 // [byte]  used for the signon sequence
#define svc_centerprint         26 // [string] to put in center of the screen
#define svc_killedmonster       27
#define svc_foundsecret         28
#define svc_spawnstaticsound    29 // [coord3] [byte] samp [byte] vol [byte] aten
#define svc_intermission        30  // [string] music
#define svc_finale              31  // [string] music [string] text
#define svc_cdtrack             32  // [byte] track [byte] looptrack
#define svc_sellscreen          33
#define svc_cutscene            34

// client to server remote procedure calls
#define clc_bad                 0
#define clc_nop                 1
#define clc_disconnect          2
#define clc_move                3   // [usercmd_t]
#define clc_stringcmd           4   // [string] message

// temp entity events
#define TE_SPIKE                0
#define TE_SUPERSPIKE           1
#define TE_GUNSHOT              2
#define TE_EXPLOSION            3
#define TE_TAREXPLOSION         4
#define TE_LIGHTNING1           5
#define TE_LIGHTNING2           6
#define TE_WIZSPIKE             7
#define TE_KNIGHTSPIKE          8
#define TE_LIGHTNING3           9
#define TE_LAVASPLASH           10
#define TE_TELEPORT             11
#define TE_EXPLOSION2           12
#define TE_BEAM                 13 // PGM 01/21/97

//=====================================
// sbar

#define SBAR_HEIGHT 24

//=====================================
// sound

#define DEFAULT_SOUND_PACKET_VOLUME         255
#define DEFAULT_SOUND_PACKET_ATTENUATION    1.0

#define MAX_CHANNELS                        128
#define MAX_DYNAMIC_CHANNELS                8

//=====================================
// render

#define MAXCLIPPLANES   11

#define TOP_RANGE       16   // soldier uniform colors
#define BOTTOM_RANGE    96

//=====================================
// client

#define CSHIFT_CONTENTS     0
#define CSHIFT_DAMAGE       1
#define CSHIFT_BONUS        2
#define CSHIFT_POWERUP      3
#define NUM_CSHIFTS         4

#define NAME_LENGTH         64
#define SIGNONS             4   // signon messages to receive before connected

#define MAX_DLIGHTS         32
#define MAX_BEAMS           24
#define MAX_EFRAGS          640
#define MAX_MAPSTRING       2048
#define MAX_DEMOS           8
#define MAX_DEMONAME        16
#define MAX_TEMP_ENTITIES   64   // lightning bolts, etc
#define MAX_STATIC_ENTITIES 128   // torches, etc
#define MAX_VISEDICTS       256

//=====================================
// progs

#define MAX_ENT_LEAFS           16

// These should all be made into functions
//#define EDICT_FROM_AREA(l)      STRUCT_FROM_LINK(l,edict_t,area)
//#define EDICT_NUM(n)            ((edict_t*)(sv.edicts+ (n)*pr_edict_size))
//#define NUM_FOR_EDICT(e)        (((byte*)(e) - sv.edicts)/pr_edict_size)
//
//#define G_FLOAT(o)              (pr_globals[o])
//#define G_INT(o)                (*(int *)&pr_globals[o])
//#define G_EDICT(o)              ((edict_t *)((byte *)sv.edicts+ *(int *)&pr_globals[o]))
//#define G_EDICTNUM(o)           NUM_FOR_EDICT(G_EDICT(o))
//#define G_VECTOR(o)             (&pr_globals[o])
//#define G_STRING(o)             (pr_strings + *(string_t *)&pr_globals[o])
//#define G_FUNCTION(o)           (*(func_t *)&pr_globals[o])
//
//#define E_FLOAT(e,o)            (((float*)&e->v)[o])
//#define E_INT(e,o)              (*(int *)&((float*)&e->v)[o])
//#define E_VECTOR(e,o)           (&((float*)&e->v)[o])
//#define E_STRING(e,o)           (pr_strings + *(string_t *)&((float*)&e->v)[o])

//=====================================
// server

#define NUM_PING_TIMES              16
#define NUM_SPAWN_PARMS             16

// edict->movetype values
#define MOVETYPE_NONE               0  // never moves
#define MOVETYPE_ANGLENOCLIP        1
#define MOVETYPE_ANGLECLIP          2
#define MOVETYPE_WALK               3  // gravity
#define MOVETYPE_STEP               4  // gravity, special edge handling
#define MOVETYPE_FLY                5
#define MOVETYPE_TOSS               6  // gravity
#define MOVETYPE_PUSH               7  // no clip to world, push and crush
#define MOVETYPE_NOCLIP             8
#define MOVETYPE_FLYMISSILE         9  // extra size to monsters
#define MOVETYPE_BOUNCE             10

// edict->solid values
#define SOLID_NOT                   0  // no interaction with other objects
#define SOLID_TRIGGER               1  // touch on edge, but not blocking
#define SOLID_BBOX                  2  // touch on edge, block
#define SOLID_SLIDEBOX              3  // touch on edge, but not an onground
#define SOLID_BSP                   4  // bsp clip, touch on edge, block

// edict->deadflag values
#define DEAD_NO                     0
#define DEAD_DYING                  1
#define DEAD_DEAD                   2

#define DAMAGE_NO                   0
#define DAMAGE_YES                  1
#define DAMAGE_AIM                  2

// edict->flags
#define FL_FLY                      1
#define FL_SWIM                     2
//#define FL_GLIMPSE                4
#define FL_CONVEYOR                 4
#define FL_CLIENT                   8
#define FL_INWATER                  16
#define FL_MONSTER                  32
#define FL_GODMODE                  64
#define FL_NOTARGET                 128
#define FL_ITEM                     256
#define FL_ONGROUND                 512
#define FL_PARTIALGROUND            1024 // not all corners are valid
#define FL_WATERJUMP                2048 // player jumping out of water
#define FL_JUMPRELEASED             4096 // for jump debouncing

// entity effects

#define EF_BRIGHTFIELD              1
#define EF_MUZZLEFLASH              2
#define EF_BRIGHTLIGHT              4
#define EF_DIMLIGHT                 8

#define SPAWNFLAG_NOT_EASY          256
#define SPAWNFLAG_NOT_MEDIUM        512
#define SPAWNFLAG_NOT_HARD          1024
#define SPAWNFLAG_NOT_DEATHMATCH    2048

//=====================================
// host

#define	SAVEGAME_VERSION    5

//=====================================
// gl_model

#define SIDE_FRONT          0
#define SIDE_BACK           1
#define SIDE_ON             2

#define SURF_PLANEBACK      2
#define SURF_DRAWSKY        4
#define SURF_DRAWSPRITE     8
#define SURF_DRAWTURB       0x10
#define SURF_DRAWTILED      0x20
#define SURF_DRAWBACKGROUND 0x40
#define SURF_UNDERWATER     0x80

#define VERTEXSIZE          7

#define MAX_SKINS           32

#define MAXALIASVERTS       1024
#define MAXALIASFRAMES      256
#define MAXALIASTRIS        2048

#define EF_ROCKET           1   // leave a trail
#define EF_GRENADE          2   // leave a trail
#define EF_GIB              4   // leave a trail
#define EF_ROTATE           8   // rotate (bonus items)
#define EF_TRACER           16  // green split trail
#define EF_ZOMGIB           32  // small blood trail
#define EF_TRACER2          64  // orange split trail + rotate
#define EF_TRACER3          128 // purple trail

//=====================================
// world

#define MOVE_NORMAL     0
#define MOVE_NOMONSTERS 1
#define MOVE_MISSILE    2

//=====================================
// keys

// these are the key numbers that should be passed to Key_Event
#define K_TAB           9
#define K_ENTER         13
#define K_ESCAPE        27
#define K_SPACE         32

// normal keys (33-126) should be passed as lowercased ascii

#define K_BACKSPACE     127
#define K_UPARROW       128
#define K_DOWNARROW     129
#define K_LEFTARROW     130
#define K_RIGHTARROW    131

#define K_ALT           132
#define K_CTRL          133
#define K_SHIFT         134
#define K_F1            135
#define K_F2            136
#define K_F3            137
#define K_F4            138
#define K_F5            139
#define K_F6            140
#define K_F7            141
#define K_F8            142
#define K_F9            143
#define K_F10           144
#define K_F11           145
#define K_F12           146
#define K_INS           147
#define K_DEL           148
#define K_PGDN          149
#define K_PGUP          150
#define K_HOME          151
#define K_END           152

// mouse buttons generate virtual keys
#define K_MOUSE1        200
#define K_MOUSE2        201
#define K_MOUSE3        202

// joystick buttons
#define K_JOY1          203
#define K_JOY2          204
#define K_JOY3          205
#define K_JOY4          206

// aux keys are for multi-buttoned joysticks to generate so they can use
// the normal binding process
#define K_AUX1          207
#define K_AUX2          208
#define K_AUX3          209
#define K_AUX4          210
#define K_AUX5          211
#define K_AUX6          212
#define K_AUX7          213
#define K_AUX8          214
#define K_AUX9          215
#define K_AUX10         216
#define K_AUX11         217
#define K_AUX12         218
#define K_AUX13         219
#define K_AUX14         220
#define K_AUX15         221
#define K_AUX16         222
#define K_AUX17         223
#define K_AUX18         224
#define K_AUX19         225
#define K_AUX20         226
#define K_AUX21         227
#define K_AUX22         228
#define K_AUX23         229
#define K_AUX24         230
#define K_AUX25         231
#define K_AUX26         232
#define K_AUX27         233
#define K_AUX28         234
#define K_AUX29         235
#define K_AUX30         236
#define K_AUX31         237
#define K_AUX32         238

// JACK: Intellimouse(c) Mouse Wheel Support
#define K_MWHEELUP      239
#define K_MWHEELDOWN    240

#define K_PAUSE         255

#define K_COUNT         (K_PAUSE + 1)

//=====================================
// menu

#define MNET_IPX    1
#define MNET_TCP    2

//=====================================
// glquake

// normalizing factor so player model works out to about 1 pixel per triangle
#define ALIAS_BASE_SIZE_RATIO   (1.0 / 11.0)
#define MAX_LBM_HEIGHT          480

// size of textures generated by R_GenTiledSurf
#define TILE_SIZE               128

#define SKYSHIFT                7
#define SKYSIZE                 (1 << SKYSHIFT)
#define SKYMASK                 (SKYSIZE - 1)

#define BACKFACE_EPSILON        0.01

#define TEXTURE0_SGIS           0x835E
#define TEXTURE1_SGIS           0x835F

//=====================================
// pr_comp

#define OFS_NULL        0
#define OFS_RETURN      1
#define OFS_PARM0       4  // leave 3 ofs for each parm to hold vectors
#define OFS_PARM1       7
#define OFS_PARM2       10
#define OFS_PARM3       13
#define OFS_PARM4       16
#define OFS_PARM5       19
#define OFS_PARM6       22
#define OFS_PARM7       25
#define RESERVED_OFS    28

#define DEF_SAVEGLOBAL  (1<<15)
#define MAX_PARMS       8
#define PROG_VERSION    6

typedef enum
{
    OP_DONE,
    OP_MUL_F,
    OP_MUL_V,
    OP_MUL_FV,
    OP_MUL_VF,
    OP_DIV_F,
    OP_ADD_F,
    OP_ADD_V,
    OP_SUB_F,
    OP_SUB_V,

    OP_EQ_F,
    OP_EQ_V,
    OP_EQ_S,
    OP_EQ_E,
    OP_EQ_FNC,

    OP_NE_F,
    OP_NE_V,
    OP_NE_S,
    OP_NE_E,
    OP_NE_FNC,

    OP_LE,
    OP_GE,
    OP_LT,
    OP_GT,

    OP_LOAD_F,
    OP_LOAD_V,
    OP_LOAD_S,
    OP_LOAD_ENT,
    OP_LOAD_FLD,
    OP_LOAD_FNC,

    OP_ADDRESS,

    OP_STORE_F,
    OP_STORE_V,
    OP_STORE_S,
    OP_STORE_ENT,
    OP_STORE_FLD,
    OP_STORE_FNC,

    OP_STOREP_F,
    OP_STOREP_V,
    OP_STOREP_S,
    OP_STOREP_ENT,
    OP_STOREP_FLD,
    OP_STOREP_FNC,

    OP_RETURN,
    OP_NOT_F,
    OP_NOT_V,
    OP_NOT_S,
    OP_NOT_ENT,
    OP_NOT_FNC,
    OP_IF,
    OP_IFNOT,
    OP_CALL0,
    OP_CALL1,
    OP_CALL2,
    OP_CALL3,
    OP_CALL4,
    OP_CALL5,
    OP_CALL6,
    OP_CALL7,
    OP_CALL8,
    OP_STATE,
    OP_GOTO,
    OP_AND,
    OP_OR,

    OP_BITAND,
    OP_BITOR,

    OP__COUNT
} OP_t;

//=====================================
// modelgen

#define ALIAS_VERSION               6
#define ALIAS_ONSEAM                0x0020
#define DT_FACES_FRONT              0x0010
#define IDPOLYHEADER                (('O'<<24)+('P'<<16)+('D'<<8)+'I')

//=====================================
// spritegen

#define SPRITE_VERSION              1

#define SPR_VP_PARALLEL_UPRIGHT     0
#define SPR_FACING_UPRIGHT          1
#define SPR_VP_PARALLEL             2
#define SPR_ORIENTED                3
#define SPR_VP_PARALLEL_ORIENTED    41

#define IDSPRITEHEADER              (('P'<<24)+('S'<<16)+('D'<<8)+'I')

//=====================================

typedef enum
{
    src_client, // Came in over a net connection as a clc_stringcmd, host_client will be valid during this state.
    src_command, // From the command buffer.
} cmd_source_t;

typedef enum
{
    key_game,
    key_console,
    key_message,
    key_menu,
} keydest_t;

typedef enum
{
    ST_SYNC = 0,
    ST_RAND,
} synctype_t;

typedef enum
{
    ALIAS_SINGLE = 0,
    ALIAS_GROUP,
} aliasframetype_t;

typedef enum
{
    ALIAS_SKIN_SINGLE = 0,
    ALIAS_SKIN_GROUP,
} aliasskintype_t;

typedef enum
{
    SPR_SINGLE = 0,
    SPR_GROUP,
} spriteframetype_t;

typedef enum
{
    mod_brush,
    mod_sprite,
    mod_alias,
} modtype_t;

typedef enum
{
    pt_static,
    pt_grav,
    pt_slowgrav,
    pt_fire,
    pt_explode,
    pt_explode2,
    pt_blob,
    pt_blob2,

    pt_COUNT
} ptype_t;

typedef enum
{
    ev_void,
    ev_string,
    ev_float,
    ev_vector,
    ev_entity,
    ev_field,
    ev_function,
    ev_pointer,

    ev_COUNT
} etype_t;

typedef enum
{
    ca_dedicated,   // a dedicated server with no ability to start a client
    ca_disconnected,  // full screen console with no connection
    ca_connected,  // valid netcon, talking to a server
} cactive_t;

typedef enum
{
    ss_loading,
    ss_active,
} server_state_t;
