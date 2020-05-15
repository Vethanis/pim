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

#include "common/macro.h"
#include "quake/q_bspfile.h"

typedef enum
{
    // entity effects
    EF_ROCKET = 1 << 0,
    EF_GRENADE = 1 << 1,
    EF_GIB = 1 << 2,
    EF_ROTATE = 1 << 3, // rotating item
    EF_TRACER = 1 << 4, // green split trail
    EF_ZOMGIB = 1 << 5, // small blood trail
    EF_TRACER2 = 1 << 6, // orange split trail and rotate
    EF_TRACER3 = 1 << 7, // purple trail

    // sides
    SIDE_FRONT = 0,
    SIDE_BACK = 1,
    SIDE_ON = 2,

    // surfaces
    SURF_PLANEBACK = 2,
    SURF_DRAWSKY = 4,
    SURF_DRAWSPRITE = 8,
    SURF_DRAWTURB = 0x10,
    SURF_DRAWTILED = 0x20,
    SURF_DRAWBACKGROUND = 0x40,

    ALIAS_VERSION = 6,
    ALIAS_ONSEAM = 0x0020,
    DT_FACES_FRONT = 0x0010,

    // little-endian "IDPO" -> Alias Model
    IDPOLYHEADER = (('O' << 24) | ('P' << 16) | ('D' << 8) | 'I'),
    // little-endian "IDSP" -> Sprite Model
    IDSPRITEHEADER = (('P' << 24) | ('S' << 16) | ('D' << 8) | 'I'),
    // brush model is the default if above are not recognized from dword 0

} model_constants_t;

// ----------------------------------------------------------------------------
// Brush Models (static bsp)

typedef struct mtexture_s
{
    char name[16];
    u32 width;
    u32 height;
    i32 anim_total; // total tenths in sequence (0 == no sequence)
    i32 anim_min; // keyframe begin
    i32 anim_max; // keyframe end
    struct mtexture_s* anim_next; // texture animation sequence
    struct mtexture_s* alternate_anims; // bmodels in frame 1 use these
    u32 offsets[MIPLEVELS]; // offset from end of this struct
} mtexture_t;

typedef struct
{
    u16 v[2];
    u32 cachededgeoffset;
} medge_t;

typedef struct
{
    float4 vecs[2];
    float mipadjust;
    mtexture_t* texture;
    i32 flags;
} mtexinfo_t;

typedef struct
{
    i32 visframe;

    i32 dlightframe;
    i32 dlightbits;

    float4 plane;
    i32 flags;

    i32 firstedge; // model->surfedges[]
    i32 numedges; // negative numbers are backwards edges

    // surface data
    struct surfcache_s* cachespots[MIPLEVELS];

    i16 texturemins[2];
    i16 extents[2];

    mtexinfo_t* texinfo;

    // lighting info
    u8 styles[MAXLIGHTMAPS];
    u8* samples; // [numstyles * surfsize]
} msurface_t;

typedef struct efrag_s
{
    struct mleaf_s* leaf;
    struct efrag_s* leafnext;
    struct entity_s* entity;
    struct efrag_s* entnext;
} efrag_t;

typedef struct mnode_s
{
// common with leaf
    i32 contents;
    i32 visframe;

    i16 mins[3];
    i16 maxs[3];

    struct mnode_s* parent;

// node specific
    float4* plane;
    struct mnode_s* children[2];

    u16 firstsurface;
    u16 numsurfaces;
} mnode_t;

typedef struct mleaf_s
{
// common with node
    i32 contents; // will be negative
    i32 visframe;

    i16 mins[3];
    i16 maxs[3];

    struct mnode_s* parent;

// leaf specific
    u8* compressed_vis;
    efrag_t* efrags;

    msurface_t** firstmarksurface;
    i32 nummarksurfaces;
    i32 key; // bsp sequence number for leaf contents
    u8 ambient_sound_level[NUM_AMBIENTS];
} mleaf_t;

typedef struct
{
    dclipnode_t* clipnodes;
    float4* planes;
    i32 firstclipnode;
    i32 lastclipnode;
    float3 clip_mins;
    float3 clip_maxs;
} mhull_t;

// ----------------------------------------------------------------------------
// Sprite Models

typedef struct
{
    i32 width;
    i32 height;
    void* pcachespot;
    float up;
    float down;
    float left;
    float right;
    u8 pixels[4];
} mspriteframe_t;

typedef struct
{
    i32 numframes;
    float* intervals;
    mspriteframe_t* frames[1];
} mspritegroup_t;

typedef enum
{
    SPR_SINGLE,
    SPR_GROUP
} spriteframetype_t;

typedef struct
{
    spriteframetype_t type;
    mspriteframe_t* frameptr;
} mspriteframedesc_t;

typedef struct
{
    i32 type;
    i32 maxwidth;
    i32 maxheight;
    i32 numframes;
    float beamlength;
    void* cachespot;
    mspriteframedesc_t frames[1];
} msprite_t;

// ----------------------------------------------------------------------------
// Alias Models (movable models)

typedef enum
{
    ST_SYNC,
    ST_RAND,
} synctype_t;

typedef enum
{
    ALIAS_SINGLE,
    ALIAS_GROUP,
} aliasframetype_t;

typedef enum
{
    ALIAS_SKIN_SINGLE,
    ALIAS_SKIN_GROUP,
} aliasskintype_t;

typedef struct
{
    i32 ident;
    i32 version;
    float3 scale;
    float3 scale_origin;
    float boundingradius;
    float3 eyeposition;
    i32 numskins;
    i32 skinwidth;
    i32 skinheight;
    i32 numverts;
    i32 numtris;
    i32 numframes;
    synctype_t synctype;
    i32 flags;
    float size;
} mdl_t;

typedef struct
{
    i32 onseam;
    i32 s;
    i32 t;
} stvert_t;

typedef struct
{
    i32 facesfront;
    int3 vertindex;
} dtriangle_t;

typedef struct
{
    u8 v[3];
    u8 lightnormalindex;
} trivertx_t;

typedef struct
{
    trivertx_t bboxmin;
    trivertx_t bboxmax;
    char name[16];
} daliasframe_t;

typedef struct
{
    i32 numframes;
    trivertx_t bboxmin;
    trivertx_t bboxmax;
} daliasgroup_t;

typedef struct
{
    i32 numskins;
} daliasskingroup_t;

typedef struct
{
    float interval;
} daliasinterval_t;

typedef struct
{
    float interval;
} daliasskininterval_t;

typedef struct
{
    aliasframetype_t type;
} daliasframetype_t;

typedef struct
{
    aliasskintype_t type;
} daliasskintype_t;

typedef struct
{
    aliasframetype_t type;
    trivertx_t bboxmin;
    trivertx_t bboxmax;
    i32 frame;
    char name[16];
} maliasframedesc_t;

typedef struct
{
    aliasskintype_t type;
    void *pcachespot;
    i32 skin;
} maliasskindesc_t;

typedef struct
{
    trivertx_t bboxmin;
    trivertx_t bboxmax;
    i32 frame;
} maliasgroupframedesc_t;

typedef struct
{
    i32 numframes;
    i32 intervals;
    maliasgroupframedesc_t frames[1];
} maliasgroup_t;

typedef struct
{
    i32 numskins;
    i32 intervals;
    maliasskindesc_t skindescs[1];
} maliasskingroup_t;

typedef struct
{
    i32 facesfront;
    i32 vertindex[3];
} mtriangle_t;

typedef struct
{
    i32 model;
    i32 stverts;
    i32 skindesc;
    i32 triangles;
    maliasframedesc_t frames[1];
} aliashdr_t;

// ----------------------------------------------------------------------------
// Whole Model

typedef enum
{
    mod_brush,  // bsp brush model
    mod_sprite, // 2D sprite model
    mod_alias,  // movable character model
} modtype_t;

typedef struct
{
// metadata
    char name[64];
    i32 needload;

    modtype_t type;
    i32 numframes;
    synctype_t synctype;

    i32 flags;

// model bounds
    float3 mins;
    float3 maxs;
    float radius;

// brush model data
    i32 firstmodelsurface;
    i32 nummodelsurfaces;

    i32 numsubmodels;
    dmodel_t* submodels;

    i32 numplanes;
    float4* planes;

    i32 numleafs;
    mleaf_t* leafs;

    i32 numvertices;
    float4* vertices;

    i32 numedges;
    medge_t* edges;

    i32 numnodes;
    mnode_t* nodes;

    i32 numtexinfo;
    mtexinfo_t* texinfo;

    // each surface points to a a section of surfedges
    // to flatten model, traverse surfaces, then surfedges, then vertices
    i32 numsurfaces;
    msurface_t* surfaces;

    i32 numsurfedges;
    i32* surfedges;

    i32 numclipnodes;
    dclipnode_t* clipnodes;

    i32 nummarksurfaces;
    msurface_t** marksurfaces;

    mhull_t hulls[MAX_MAP_HULLS];

    i32 numtextures;
    mtexture_t** textures;

    u8* visdata;
    u8* lightdata;
    char* entities;

// additional model data
    void* cache;

} mmodel_t;

mmodel_t* LoadModel(const void* buffer, EAlloc allocator);
void FreeModel(mmodel_t* model);
