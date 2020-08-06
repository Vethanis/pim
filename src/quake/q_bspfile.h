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

PIM_C_BEGIN

typedef enum
{
    // design limits of the bsp file format
    MAX_MAP_HULLS = 4,
    MAX_MAP_MODELS = 256,
    MAX_MAP_BRUSHES = 4096,
    MAX_MAP_ENTITIES = 1024,
    MAX_MAP_ENTSTRING = 65536,

    MAX_MAP_PLANES = 32767,
    MAX_MAP_NODES = 32767,

    MAX_MAP_CLIPNODES = 32767,
    MAX_MAP_LEAFS = 8192,
    MAX_MAP_VERTS = 65535,
    MAX_MAP_FACES = 65535,
    MAX_MAP_MARKSURFACES = 65535,
    MAX_MAP_TEXINFO = 4096,
    MAX_MAP_EDGES = 256000,
    MAX_MAP_SURFEDGES = 512000,
    MAX_MAP_TEXTURES = 512,
    MAX_MAP_MIPTEX = 0x200000,
    MAX_MAP_LIGHTING = 0x100000,
    MAX_MAP_VISIBILITY = 0x100000,

    MAX_MAP_PORTALS = 65536,

    MAX_KEY = 32,
    MAX_VALUE = 1024,

    // file format metadata
    BSPVERSION = 29,
    TOOLVERSION = 2,

    // lump categories within a bsp file
    LUMP_ENTITIES = 0,
    LUMP_PLANES = 1,
    LUMP_TEXTURES = 2,
    LUMP_VERTEXES = 3,
    LUMP_VISIBILITY = 4,
    LUMP_NODES = 5,
    LUMP_TEXINFO = 6,
    LUMP_FACES = 7,
    LUMP_LIGHTING = 8,
    LUMP_CLIPNODES = 9,
    LUMP_LEAFS = 10,
    LUMP_MARKSURFACES = 11,
    LUMP_EDGES = 12,
    LUMP_SURFEDGES = 13,
    LUMP_MODELS = 14,
    HEADER_LUMPS = 15,

    // bsp plane axii
    // 0-2 are axial planes
    PLANE_X = 0,
    PLANE_Y = 1,
    PLANE_Z = 2,

    // 3-5 are non-axial planes snapped to the nearest
    PLANE_ANYX = 3,
    PLANE_ANYY = 4,
    PLANE_ANYZ = 5,

    // clipnode contents
    CONTENTS_EMPTY = -1,
    CONTENTS_SOLID = -2,
    CONTENTS_WATER = -3,
    CONTENTS_SLIME = -4,
    CONTENTS_LAVA = -5,
    CONTENTS_SKY = -6,
    CONTENTS_ORIGIN = -7, // removed at csg time
    CONTENTS_CLIP = -8, // changed to contents_solid

    CONTENTS_CURRENT_0 = -9,
    CONTENTS_CURRENT_90 = -10,
    CONTENTS_CURRENT_180 = -11,
    CONTENTS_CURRENT_270 = -12,
    CONTENTS_CURRENT_UP = -13,
    CONTENTS_CURRENT_DOWN = -14,

    // sky or slime, no lightmap or 256 subdivision
    TEX_SPECIAL = 1,
    MAXLIGHTMAPS = 4,
    MIPLEVELS = 4,

    // automatic ambient sounds
    AMBIENT_WATER = 0,
    AMBIENT_SKY = 1,
    AMBIENT_SLIME = 2,
    AMBIENT_LAVA = 3,
    NUM_AMBIENTS = 4,

    ANGLE_UP = -1,
    ANGLE_DOWN = -2,

} bspfile_constants_t;

// indicates an offset and length within the file
typedef struct
{
    i32 fileofs;
    i32 filelen;
} lump_t;

// model on disk
typedef struct
{
    // bounds of entire model
    // in object space?
    float mins[3];
    float maxs[3];
    float origin[3];
    // for each hull, the index of the root node of its bsp tree
    i32 headnode[MAX_MAP_HULLS];
    // might point to leaf data, or to the PVS
    i32 visleafs; // not including the solid leaf 0
    // points to face array of entire model
    i32 firstface;
    i32 numfaces;
} dmodel_t;

// header with offset and size of each lump type within the file
typedef struct
{
    i32 version; // BSPVERSION
    lump_t lumps[HEADER_LUMPS];
} dheader_t;

typedef struct
{
    i32 nummiptex;
    i32 dataofs[4]; // actual size is nummiptex
} dmiptexlump_t;

// mipmapped texture with 4 mip levels stored at given offsets
typedef struct
{
    char name[16];
    // base width and height of the texture for mip 0
    // mipN width = width >> N
    u32 width;
    u32 height;
    // an offset for each mip level of the texture
    // unsure if relative or absolute file offset
    u32 offsets[MIPLEVELS];
} miptex_t;

// vertex position
typedef struct
{
    float point[3];
} dvertex_t;

// bsp plane, subdivides space
typedef struct
{
    float normal[3];
    float dist; // dist == dot(normal, pointOnPlane)
    i32 type; // PLANE_X enum
} dplane_t;

// bsp node with payload
typedef struct
{
    // which plane this node uses
    i32 planenum;
    // >=0: points to child node
    //  <0: points to leaf. add 1 then negate to extract index.
    i16 children[2];
    // min and max bounds, used for sphere culling
    i16 mins[3];
    i16 maxs[3];
    // face array
    u16 firstface;
    u16 numfaces;
} dnode_t;

// leaf 0 is the generic CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
typedef struct
{
    // content type
    i32 contents;
    // index into PVS
    i32 visofs; // -1 = no visibility info

    // bounds for frustum culling
    i16 mins[3];
    i16 maxs[3];

    // array of mark surfaces
    u16 firstmarksurface;
    u16 nummarksurfaces;

    // ambient sound associated with this leaf
    u8 ambient_level[NUM_AMBIENTS];
} dleaf_t;

// lighter bsp node
// probably used for visibility
typedef struct
{
    // index of plane
    i32 planenum;
    // >= 0: child indices
    //  < 0: contents. increment and negate
    i16 children[2];
} dclipnode_t;

// renamed to dtexinfo_t, since it probably resides on disk
typedef struct
{
    float vecs[2][4]; // [s/t][xyz offset]
    // index of miptex this applies to
    i32 miptex;
    // only 1 known flag: TEX_SPECIAL
    i32 flags;
} dtexinfo_t;

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct
{
    u16 v[2]; // vertex numbers
} dedge_t;

typedef struct
{
    // index of plane this face is on
    i16 planenum;
    // which side of the plane this face is on
    i16 side;

    // edges[firstedge to firstedge + numedges]
    i32 firstedge;
    i16 numedges;
    // index of the texture info for this face
    i16 texinfo;

    // lighting info
    u8 styles[MAXLIGHTMAPS];
    // start offset of [numstyles * surfsize] lightmap samples
    i32 lightofs;
} dface_t;

typedef struct bsp_epair_s
{
    struct bsp_epair_s* next;
    char* key;
    char* value;
} bsp_epair_t;

typedef struct
{
    float origin[3];
    // array of brushes
    i32 firstbrush;
    i32 numbrushes;
    // key-value pair linked list
    bsp_epair_t* epairs;
} bsp_ent_t;

// ----------------------------------------------------------------------------
// tools can be lazy and use giant static arrays.
// useful for understanding structure of bsp file

typedef struct
{
    i32 length;
    dmodel_t arr[MAX_MAP_MODELS];
} tool_dmodels_t;

typedef struct
{
    i32 length;
    u8 arr[MAX_MAP_VISIBILITY];
} tool_dvisdata_t;

typedef struct
{
    i32 length;
    u8 arr[MAX_MAP_LIGHTING];
} tool_dlightdata_t;

typedef struct
{
    i32 length;
    u8 arr[MAX_MAP_MIPTEX]; // dmiptexlump_t
} tool_dtexdata_t;

typedef struct
{
    i32 length;
    char arr[MAX_MAP_ENTSTRING];
} tool_dentdata_t;

typedef struct
{
    i32 length;
    dleaf_t arr[MAX_MAP_LEAFS];
} tool_dleafs_t;

typedef struct
{
    i32 length;
    dplane_t arr[MAX_MAP_PLANES];
} tool_dplanes_t;

typedef struct
{
    i32 length;
    dvertex_t arr[MAX_MAP_VERTS];
} tool_dvertices_t;

typedef struct
{
    i32 length;
    dnode_t arr[MAX_MAP_NODES];
} tool_dnodes_t;

typedef struct
{
    i32 length;
    dtexinfo_t arr[MAX_MAP_TEXINFO];
} tool_dtexinfos_t;

typedef struct
{
    i32 length;
    dface_t arr[MAX_MAP_FACES];
} tool_dfaces_t;

typedef struct
{
    i32 length;
    dclipnode_t arr[MAX_MAP_CLIPNODES];
} tool_dclipnodes_t;

typedef struct
{
    i32 length;
    dedge_t arr[MAX_MAP_EDGES];
} tool_dedges_t;

typedef struct
{
    i32 length;
    u16 arr[MAX_MAP_MARKSURFACES];
} tool_dmarksurfaces_t;

typedef struct
{
    i32 length;
    i32 arr[MAX_MAP_SURFEDGES];
} tool_dsurfedges_t;

typedef struct
{
    i32 length;
    bsp_ent_t arr[MAX_MAP_ENTITIES];
} tool_bsp_ents_t;

PIM_C_END

