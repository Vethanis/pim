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
#include "math/types.h"

PIM_C_BEGIN

enum
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
    SURF_UNDERWATER = 0x80,

    ALIAS_VERSION = 6,
    ALIAS_ONSEAM = 0x0020,
    DT_FACES_FRONT = 0x0010,

    // little-endian "IDPO" -> Alias Model
    IDPOLYHEADER = (('O' << 24) | ('P' << 16) | ('D' << 8) | 'I'),
    // little-endian "IDSP" -> Sprite Model
    IDSPRITEHEADER = (('P' << 24) | ('S' << 16) | ('D' << 8) | 'I'),
    // brush model is the default if above are not recognized from dword 0

};

// ----------------------------------------------------------------------------
// Brush Models (static bsp)

typedef struct mtexture_s
{
    char name[16];
    i32 width;
    i32 height;
    i32 anim_total; // total tenths in sequence (0 == no sequence)
    i32 anim_min; // keyframe begin
    i32 anim_max; // keyframe end
    struct mtexture_s* anim_next; // texture animation sequence
    struct mtexture_s* alternate_anims; // bmodels in frame 1 use these
} mtexture_t;

typedef struct medge_s
{
    i32 v[2];
} medge_t;

typedef struct mtexinfo_s
{
    float4 vecs[2];
    mtexture_t const* texture;
} mtexinfo_t;

typedef struct msurface_s
{
    i32 firstedge; // model->surfedges[]
    i32 numedges; // negative numbers are backwards edges
    // texture info for this face
    mtexinfo_t const* texinfo;
} msurface_t;

// ----------------------------------------------------------------------------
// Sprite Models

typedef struct mspriteframe_s
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

typedef struct mspritegroup_s
{
    i32 numframes;
    float* intervals;
    mspriteframe_t* frames[1];
} mspritegroup_t;

typedef enum
{
    SPR_SINGLE,
    SPR_GROUP,
} spriteframetype_t;

typedef struct mspriteframedesc_s
{
    spriteframetype_t type;
    mspriteframe_t* frameptr;
} mspriteframedesc_t;

typedef struct msprite_s
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

typedef struct mdl_s
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

typedef struct stvert_s
{
    i32 onseam;
    i32 s;
    i32 t;
} stvert_t;

typedef struct dtriangle_s
{
    i32 facesfront;
    int3 vertindex;
} dtriangle_t;

typedef struct trivertx_s
{
    u8 v[3];
    u8 lightnormalindex;
} trivertx_t;

typedef struct daliasframe_s
{
    trivertx_t bboxmin;
    trivertx_t bboxmax;
    char name[16];
} daliasframe_t;

typedef struct daliasgroup_s
{
    i32 numframes;
    trivertx_t bboxmin;
    trivertx_t bboxmax;
} daliasgroup_t;

typedef struct daliasskingroup_s
{
    i32 numskins;
} daliasskingroup_t;

typedef struct daliasinterval_s
{
    float interval;
} daliasinterval_t;

typedef struct daliasskininterval_s
{
    float interval;
} daliasskininterval_t;

typedef struct daliasframetype_s
{
    aliasframetype_t type;
} daliasframetype_t;

typedef struct daliasskintype_s
{
    aliasskintype_t type;
} daliasskintype_t;

typedef struct maliasframedesc_s
{
    aliasframetype_t type;
    trivertx_t bboxmin;
    trivertx_t bboxmax;
    i32 frame;
    char name[16];
} maliasframedesc_t;

typedef struct maliasskindesc_s
{
    aliasskintype_t type;
    void *pcachespot;
    i32 skin;
} maliasskindesc_t;

typedef struct maliasgroupframedesc_s
{
    trivertx_t bboxmin;
    trivertx_t bboxmax;
    i32 frame;
} maliasgroupframedesc_t;

typedef struct maliasgroup_s
{
    i32 numframes;
    i32 intervals;
    maliasgroupframedesc_t frames[1];
} maliasgroup_t;

typedef struct maliasskingroup_s
{
    i32 numskins;
    i32 intervals;
    maliasskindesc_t skindescs[1];
} maliasskingroup_t;

typedef struct mtriangle_s
{
    i32 facesfront;
    i32 vertindex[3];
} mtriangle_t;

typedef struct aliashdr_s
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

typedef struct mmodel_s
{
    char name[64];

    i32 numvertices;
    float4* vertices;

    i32 numedges;
    medge_t* edges;

    i32 numtexinfo;
    mtexinfo_t* texinfo;

    i32 numsurfaces;
    msurface_t* surfaces;

    i32 numsurfedges;
    i32* surfedges;

    i32 numtextures;
    mtexture_t** textures;

    i32 entitiessize;
    char* entities;
} mmodel_t;

mmodel_t* LoadModel(const char* name, const void* buffer, EAlloc allocator);
void FreeModel(mmodel_t* model);

PIM_C_END
