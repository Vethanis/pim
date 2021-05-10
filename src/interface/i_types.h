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
#include "interface/i_defines.h"

//=============================================================================
// engine interop

typedef enum
{
    QAlloc_Zone = 0,        // a basic heap allocator
    QAlloc_HunkNamed = 1,   // persistent side (low) of a two-sided stack allocator
    QAlloc_HunkTemp = 2,    // temporary side (high) of a two-sided stack allocator
    QAlloc_Cache = 3,       // intrusive linked list of LRU garbage collected memory between hunk low and high marks
    QAlloc_Stack = 4,       // stack memory or HunkTemp (often switched to .bss static memory in mods)

    QAlloc_COUNT
} QAlloc;

typedef struct filehdl_s { hdl_t h; } filehdl_t;
typedef struct meshhdl_s { hdl_t h; } meshhdl_t;
typedef struct texhdl_s { hdl_t h; } texhdl_t;
typedef struct sockhdl_s { hdl_t h; } sockhdl_t;
typedef struct enthdl_s { hdl_t h; } enthdl_t;
typedef struct cachehdl_s { hdl_t h; } cachehdl_t;
typedef struct buffer_s { void* ptr; i32 len; } buffer_t;

//=============================================================================
// type aliases

typedef u8 byte;
typedef bool qboolean;
typedef byte pixel_t; // a pixel can be one, two, or four bytes

typedef float vec_t;
typedef vec_t vec3_t[3];
typedef vec_t vec5_t[5];

typedef i32 fixed4_t;
typedef i32 fixed8_t;
typedef i32 fixed16_t;

typedef i32 func_t;
typedef i32 string_t;

//=====================================
// common

typedef struct sizebuf_s
{
    u8 *data;
    i32 cursize;
} sizebuf_t;

typedef struct link_s
{
    struct link_s *prev, *next;
} link_t;

//=====================================
// zone

typedef struct cache_user_s
{
    cachehdl_t hdl;
} cache_user_t;

//=====================================
// cvar

typedef struct cvar_s
{
    const char *name;
    const char *defaultValue;
    qboolean archive;   // set to true to cause it to be saved to vars.rc
    qboolean server;    // notifies players when changed
    float value;
    char* stringValue;
} cvar_t;

//=====================================
// cmd

typedef void(*xcommand_t)(void);

//=====================================
// quakedef

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use
typedef struct quakeparms_s
{
    const char *basedir;
    const char **argv;
    i32 argc;
    i32 memsize;
} quakeparms_t;

typedef struct entity_state_s
{
    vec3_t origin;
    vec3_t angles;
    i32 modelindex;
    i32 frame;
    i32 colormap;
    i32 skin;
    i32 effects;
} entity_state_t;

//=====================================
// wad

typedef struct qpic_s
{
    i32 width, height;
    u8 data[4]; // variably sized
} qpic_t;

typedef struct wadinfo_s
{
    char identification[4]; // should be WAD2 or 2DAW
    i32 numlumps;
    i32 infotableofs;
} wadinfo_t;

typedef struct lumpinfo_s
{
    i32 filepos;
    i32 disksize;
    i32 size; // uncompressed
    char type;
    char compression;
    char pad1, pad2;
    char name[16]; // must be null terminated
} lumpinfo_t;

//=====================================
// bspfile

typedef struct lump_s
{
    i32 fileofs, filelen;
} lump_t;

typedef struct dmodel_s
{
    float mins[3], maxs[3];
    float origin[3];
    i32 headnode[MAX_MAP_HULLS];
    i32 visleafs; // not including the solid leaf 0
    i32 firstface, numfaces;
} dmodel_t;

typedef struct dheader_s
{
    i32 version;
    lump_t lumps[HEADER_LUMPS];
} dheader_t;

typedef struct dmiptexlump_s
{
    i32 nummiptex;
    i32 dataofs[4]; // [nummiptex]
} dmiptexlump_t;

typedef struct miptex_s
{
    char name[16];
    u32 width, height;
    u32 offsets[MIPLEVELS]; // four mip maps stored
} miptex_t;

typedef struct dvertex_s
{
    float point[3];
} dvertex_t;

typedef struct dplane_s
{
    float normal[3];
    float dist;
    i32 type; // PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dplane_t;

typedef struct dnode_s
{
    i32 planenum;
    i16 children[2]; // negative numbers are -(leafs+1), not nodes
    i16 mins[3]; // for sphere culling
    i16 maxs[3];
    u16 firstface;
    u16 numfaces; // counting both sides
} dnode_t;

typedef struct dclipnode_s
{
    i32 planenum;
    i16 children[2]; // negative numbers are contents
} dclipnode_t;

typedef struct texinfo_s
{
    float vecs[2][4]; // [s/t][xyz offset]
    i32 miptex;
    i32 flags;
} texinfo_t;

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct dedge_s
{
    u16 v[2]; // vertex numbers
} dedge_t;

typedef struct dface_s
{
    i16 planenum;
    i16 side;

    i32 firstedge; // we must support > 64k edges
    i16 numedges;
    i16 texinfo;

    // lighting info
    u8 styles[MAXLIGHTMAPS];
    i32 lightofs; // start of [numstyles*surfsize] samples
} dface_t;

// leaf 0 is the generic CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
typedef struct dleaf_s
{
    i32 contents;
    i32 visofs; // -1 = no visibility info

    i16 mins[3]; // for frustum culling
    i16 maxs[3];

    u16 firstmarksurface;
    u16 nummarksurfaces;

    u8 ambient_level[NUM_AMBIENTS];
} dleaf_t;

//=====================================
// modelgen

typedef struct mdl_s
{
    i32 ident;
    i32 version;
    vec3_t scale;
    vec3_t scale_origin;
    float boundingradius;
    vec3_t eyeposition;
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
    i32 vertindex[3];
} dtriangle_t;

typedef struct trivertx_s
{
    u8 v[3];
    u8 lightnormalindex;
} trivertx_t;

typedef struct daliasframe_s
{
    trivertx_t bboxmin; // lightnormal isn't used
    trivertx_t bboxmax; // lightnormal isn't used
    char name[16]; // frame name from grabbing
} daliasframe_t;

typedef struct daliasgroup_s
{
    i32 numframes;
    trivertx_t bboxmin; // lightnormal isn't used
    trivertx_t bboxmax; // lightnormal isn't used
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

//=====================================
// spritegen

typedef struct dsprite_s
{
    i32 ident;
    i32 version;
    i32 type;
    float boundingradius;
    i32 width;
    i32 height;
    i32 numframes;
    float beamlength;
    synctype_t synctype;
} dsprite_t;

typedef struct dspriteframe_s
{
    i32 origin[2];
    i32 width;
    i32 height;
} dspriteframe_t;

typedef struct dspritegroup_s
{
    i32 numframes;
} dspritegroup_t;

typedef struct dspriteinterval_s
{
    float interval;
} dspriteinterval_t;

typedef struct dspriteframetype_s
{
    spriteframetype_t type;
} dspriteframetype_t;

//=====================================
// gl_model

typedef struct mvertex_s
{
    vec3_t position;
} mvertex_t;

typedef struct mplane_s
{
    vec3_t normal;
    float dist;
    u8 type; // for texture axis selection and fast side tests
    u8 signbits; // signx + signy<<1 + signz<<1
    u8 pad[2];
} mplane_t;

typedef struct texture_s
{
    char name[16];
    u32 width, height;
    i32 gl_texturenum;
    struct msurface_s *texturechain;    // for gl_texsort drawing
    i32 anim_total;                     // total tenths in sequence ( 0 = no)
    i32 anim_min, anim_max;             // time for this frame min <=time< max
    struct texture_s *anim_next;        // in the animation sequence
    struct texture_s *alternate_anims;  // bmodels in frmae 1 use these
    u32 offsets[MIPLEVELS];             // four mip maps stored
} texture_t;

typedef struct medge_s
{
    u16 v[2];
    u32 cachededgeoffset;
} medge_t;

typedef struct mtexinfo_s
{
    float vecs[2][4];
    float mipadjust;
    struct texture_s *texture;
    i32 flags;
} mtexinfo_t;

typedef struct glpoly_s
{
    struct glpoly_s *next;
    struct glpoly_s *chain;
    i32 numverts;
    i32 flags; // for SURF_UNDERWATER
    float verts[4][VERTEXSIZE]; // variable sized (xyz s1t1 s2t2)
} glpoly_t;

typedef struct msurface_s
{
    i32 visframe;  // should be drawn when node is crossed
    struct mplane_s *plane;
    i32 flags;
    i32 firstedge;  // look up in model->surfedges[], negative numbers
    i32 numedges;   // are backwards edges
    i16 texturemins[2];
    i16 extents[2];
    i32 light_s, light_t;   // gl lightmap coordinates
    struct glpoly_s *polys;        // multiple if warped
    struct msurface_s *texturechain;
    struct mtexinfo_s *texinfo;
    // lighting info
    i32 dlightframe;
    i32 dlightbits;
    i32 lightmaptexturenum;
    u8 styles[MAXLIGHTMAPS];
    i32 cached_light[MAXLIGHTMAPS]; // values currently used in lightmap
    qboolean cached_dlight; // true if dynamic light in cache
    u8 *samples; // [numstyles*surfsize]
} msurface_t;

typedef struct mnode_s
{
// common with leaf
    i32 contents; // 0, to differentiate from leafs
    i32 visframe; // node needs to be traversed if current
    float minmaxs[6]; // for bounding box culling
    struct mnode_s *parent;
// node specific
    struct mplane_s *plane;
    struct mnode_s *children[2];
    u16 firstsurface;
    u16 numsurfaces;
} mnode_t;

typedef struct mleaf_s
{
// common with node
    i32 contents; // wil be a negative contents number
    i32 visframe; // node needs to be traversed if current
    float minmaxs[6]; // for bounding box culling
    struct mnode_s *parent;
// leaf specific
    u8 *compressed_vis;
    struct efrag_s *efrags;
    struct msurface_s **firstmarksurface;
    i32 nummarksurfaces;
    i32 key; // BSP sequence number for leaf's contents
    u8 ambient_sound_level[NUM_AMBIENTS];
} mleaf_t;

typedef struct hull_s
{
    struct dclipnode_s *clipnodes;
    struct mplane_s *planes;
    i32 firstclipnode;
    i32 lastclipnode;
    vec3_t clip_mins;
    vec3_t clip_maxs;
} hull_t;

// sprite models

typedef struct mspriteframe_s
{
    i32 width;
    i32 height;
    float up, down, left, right;
    i32 gl_texturenum;
} mspriteframe_t;

typedef struct mspritegroup_s
{
    i32 numframes;
    float *intervals;
    struct mspriteframe_s *frames[1];
} mspritegroup_t;

typedef struct mspriteframedesc_s
{
    spriteframetype_t type;
    struct mspriteframe_s *frameptr;
} mspriteframedesc_t;

typedef struct msprite_s
{
    i32 type;
    i32 maxwidth;
    i32 maxheight;
    i32 numframes;
    float beamlength; // remove?
    void *cachespot; // remove?
    mspriteframedesc_t frames[1];
} msprite_t;

// alias models (cached, movable / deletable)

typedef struct maliasframedesc_s
{
    i32 firstpose;
    i32 numposes;
    float interval;
    trivertx_t bboxmin;
    trivertx_t bboxmax;
    i32 frame;
    char name[16];
} maliasframedesc_t;

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

typedef struct mtriangle_s
{
    i32 facesfront;
    i32 vertindex[3];
} mtriangle_t;

typedef struct aliashdr_s
{
    i32 ident;
    i32 version;
    vec3_t scale;
    vec3_t scale_origin;
    float boundingradius;
    vec3_t eyeposition;
    i32 numskins;
    i32 skinwidth;
    i32 skinheight;
    i32 numverts;
    i32 numtris;
    i32 numframes;
    synctype_t synctype;
    i32 flags;
    float size;
    i32 numposes;
    i32 poseverts;
    i32 posedata; // numposes*poseverts trivert_t
    i32 commands; // gl command list with embedded s/t
    i32 gl_texturenum[MAX_SKINS][4];
    i32 texels[MAX_SKINS]; // only for player skins
    maliasframedesc_t frames[1]; // variable sized
} aliashdr_t;

// whole model

typedef struct model_s
{
    char name[MAX_QPATH];
    qboolean needload; // bmodels and sprites don't cache normally

    modtype_t type;
    i32 numframes;
    synctype_t synctype;

    i32 flags;

    // volume occupied by the model graphics
    vec3_t  mins, maxs;
    float  radius;

    // solid volume for clipping 
    qboolean clipbox;
    vec3_t  clipmins, clipmaxs;

    // brush model
    i32 firstmodelsurface, nummodelsurfaces;

    i32 numsubmodels;
    struct dmodel_s *submodels;

    i32 numplanes;
    struct mplane_s *planes;

    i32 numleafs;  // number of visible leafs, not counting 0
    struct mleaf_s *leafs;

    i32 numvertexes;
    struct mvertex_s *vertexes;

    i32 numedges;
    struct medge_s *edges;

    i32 numnodes;
    struct mnode_s *nodes;

    i32 numtexinfo;
    struct mtexinfo_s *texinfo;

    i32 numsurfaces;
    struct msurface_s *surfaces;

    i32 numsurfedges;
    i32 *surfedges;

    i32 numclipnodes;
    struct dclipnode_s *clipnodes;

    i32 nummarksurfaces;
    struct msurface_s **marksurfaces;

    hull_t hulls[MAX_MAP_HULLS];

    i32 numtextures;
    struct texture_s **textures;

    u8 *visdata;
    u8 *lightdata;
    char *entities;

    // additional model data
    cache_user_t cache;  // only access through Mod_Extradata
} model_t;

//=====================================
// glquake

typedef struct glvert_s
{
    float x, y, z;
    float s, t;
    float r, g, b;
} glvert_t;

typedef struct surfcache_s
{
    struct surfcache_s *next;
    struct surfcache_s **owner; // NULL is an empty chunk of memory
    i32 lightadj[MAXLIGHTMAPS]; // checked for strobe flush
    i32 dlight;
    i32 size; // including header
    u32 width;
    u32 height; // DEBUG only needed for debug
    float mipscale;
    struct texture_s *texture; // checked for animating textures
    u8 data[4]; // width*height elements
} surfcache_t;

typedef struct drawsurf_s
{
    pixel_t *surfdat; // destination for generated surface
    i32 rowbytes; // destination logical width in bytes
    struct msurface_s *surf;  // description for surface to generate
    fixed8_t lightadj[MAXLIGHTMAPS];
    // adjust for lightmap levels for dynamic lighting
    struct texture_s *texture; // corrected for animating textures
    i32 surfmip; // mipmapped ratio of surface texels / world pixels
    i32 surfwidth; // in mipmapped texels
    i32 surfheight; // in mipmapped texels
} drawsurf_t;

typedef struct particle_s
{
    // driver-usable fields
    vec3_t org;
    float color;
    // drivers never touch the following fields
    struct particle_s *next;
    vec3_t vel;
    float ramp;
    float die;
    ptype_t type;
} particle_t;

//=====================================
// vid

typedef struct vrect_s
{
    i32 x, y, width, height;
    struct vrect_s *pnext;
} vrect_t;

typedef struct viddef_s
{
    pixel_t *buffer; // invisible buffer
    pixel_t *colormap; // 256 * VID_GRADES size
    u16 *colormap16; // 256 * VID_GRADES size
    i32 fullbright; // index of first fullbright color
    u32 rowbytes; // may be > width if displayed in a window
    u32 width;
    u32 height;
    float aspect; // width / height -- < 0 is taller than wide
    i32 numpages;
    i32 recalc_refdef; // if true, recalc vid-based stuff
    pixel_t *conbuffer;
    i32 conrowbytes;
    u32 conwidth;
    u32 conheight;
    i32 maxwarpwidth;
    i32 maxwarpheight;
    pixel_t *direct; // direct drawing to framebuffer, if not NULL
} viddef_t;

//=====================================
// render

typedef struct efrag_s
{
    struct mleaf_s *leaf;
    struct efrag_s *leafnext;
    struct entity_s *entity;
    struct efrag_s *entnext;
} efrag_t;

typedef struct entity_s
{
    qboolean forcelink;             // model changed
    i32 update_type;
    entity_state_t baseline;    // to fill in defaults in updates
    double msgtime;             // time of last update
    vec3_t msg_origins[2];      // last two updates (0 is newest)
    vec3_t origin;
    vec3_t msg_angles[2];       // last two updates (0 is newest)
    vec3_t angles;
    struct model_s *model;      // NULL = no model
    struct efrag_s *efrag;      // linked list of efrags
    i32 frame;
    float syncbase;             // for client-side animations
    u8 *colormap;
    i32 effects;                // light, particals, etc
    i32 skinnum;                // for Alias models
    i32 visframe;               // last frame this entity was found in an active leaf
    i32 dlightframe;            // dynamic lighting
    i32 dlightbits;
    i32 trivial_accept;         // FIXME: could turn these into a union
    struct mnode_s *topnode;    // for bmodels, first world node
                                // that splits bmodel, or NULL if
                                // not split
} entity_t;

typedef struct refdef_s
{
    vrect_t vrect;                          // subwindow in video for refresh
                                            // FIXME: not need vrect next field here?
    vrect_t aliasvrect;                     // scaled Alias version
    i32 vrectright, vrectbottom;            // right & bottom screen coords
    i32 aliasvrectright, aliasvrectbottom;  // scaled Alias versions
    float vrectrightedge;                   // rightmost right edge we care about,
                                            //  for use in edge list
    float fvrectx, fvrecty;                 // for floating-point compares
    float fvrectx_adj, fvrecty_adj;         // left and top edges, for clamping
    i32 vrect_x_adj_shift20;                // (vrect.x + 0.5 - epsilon) << 20
    i32 vrectright_adj_shift20;             // (vrectright + 0.5 - epsilon) << 20
    float fvrectright_adj, fvrectbottom_adj;
    // right and bottom edges, for clamping
    float fvrectright;                      // rightmost edge, for Alias clamping
    float fvrectbottom;                     // bottommost edge, for Alias clamping
    float horizontalFieldOfView;            // at Z = 1.0, this many X is visible 
                                            // 2.0 = 90 degrees
    float xOrigin;                          // should probably allways be 0.5
    float yOrigin;                          // between be around 0.3 to 0.5
    vec3_t vieworg;
    vec3_t viewangles;
    float fov_x, fov_y;
    i32 ambientlight;
} refdef_t;

//=====================================
// sound

typedef struct portable_samplepair_s
{
    i32 left;
    i32 right;
} portable_samplepair_t;

typedef struct sfx_s
{
    cache_user_t cache;
    char name[MAX_QPATH];
} sfx_t;

typedef struct sfxcache_s
{
    i32 length;
    i32 loopstart;
    i32 speed;
    i32 width;
    i32 stereo;
    u8 data[4]; // variably sized
} sfxcache_t;

typedef struct dma_s
{
    qboolean gamealive;
    qboolean soundalive;
    qboolean splitbuffer;
    i32 channels;
    i32 samples;            // mono samples in buffer
    i32 submission_chunk;   // don't mix less than this #
    i32 samplepos;          // in mono samples
    i32 samplebits;
    i32 speed;
    u8 *buffer;
} dma_t;

typedef struct channel_s
{
    struct sfx_s *sfx;  // sfx number
    i32 leftvol;        // 0-255 volume
    i32 rightvol;       // 0-255 volume
    i32 end;            // end time in global paintsamples
    i32 pos;            // sample position in sfx
    i32 looping;        // where to loop, -1 = no looping
    i32 entnum;         // to allow overriding a specific sound
    i32 entchannel;     //
    vec3_t origin;      // origin of sound effect
    vec_t dist_mult;    // distance multiplier (attenuation/clipK)
    i32  master_vol;    // 0-255 master volume
} channel_t;

typedef struct wavinfo_s
{
    i32 rate;
    i32 width;
    i32 channels;
    i32 loopstart;
    i32 samples;
    i32 dataofs;    // chunk starts this many bytes from file start
} wavinfo_t;

//=====================================
// net

typedef struct qsockaddr_s
{
    i16 sa_family;
    u8 sa_data[14];
} qsockaddr_t;

typedef struct qsocket_s
{
    struct qsocket_s *next;
    double connecttime;
    double lastMessageTime;
    double lastSendTime;

    qboolean disconnected;
    qboolean canSend;
    qboolean sendNext;

    i32 driver;
    i32 landriver;
    i32 socket;
    void *driverdata;

    u32 ackSequence;
    u32 sendSequence;
    u32 unreliableSendSequence;
    i32 sendMessageLength;
    u8 sendMessage[NET_MAXMESSAGE];

    u32 receiveSequence;
    u32 unreliableReceiveSequence;
    i32 receiveMessageLength;
    u8 receiveMessage[NET_MAXMESSAGE];

    qsockaddr_t addr;
    char address[NET_NAMELEN];
} qsocket_t;

typedef struct net_landriver_s
{
    char *name;
    qboolean initialized;
    i32 controlSock;
    i32(*Init)(void);
    void(*Shutdown)(void);
    void(*Listen)(qboolean state);
    i32(*OpenSocket)(i32 port);
    i32(*CloseSocket)(i32 socket);
    i32(*Connect)(i32 socket, qsockaddr_t *addr);
    i32(*CheckNewConnections)(void);
    i32(*Read)(i32 socket, void *buf, i32 len, qsockaddr_t *addr);
    i32(*Write)(i32 socket, void *buf, i32 len, qsockaddr_t *addr);
    i32(*Broadcast)(i32 socket, void *buf, i32 len);
    char*(*AddrToString)(qsockaddr_t *addr);
    i32(*StringToAddr)(char *string, qsockaddr_t *addr);
    i32(*GetSocketAddr)(i32 socket, qsockaddr_t *addr);
    i32(*GetNameFromAddr)(qsockaddr_t *addr, char *name);
    i32(*GetAddrFromName)(char *name, qsockaddr_t *addr);
    i32(*AddrCompare)(qsockaddr_t *addr1, qsockaddr_t *addr2);
    i32(*GetSocketPort)(qsockaddr_t *addr);
    i32(*SetSocketPort)(qsockaddr_t *addr, i32 port);
} net_landriver_t;

typedef struct net_driver_s
{
    char *name;
    qboolean initialized;
    i32(*Init)(void);
    void(*Listen) (qboolean state);
    void(*SearchForHosts)(qboolean xmit);
    qsocket_t*(*Connect)(char *host);
    qsocket_t*(*CheckNewConnections)(void);
    i32(*QGetMessage)(qsocket_t *sock);
    i32(*QSendMessage)(qsocket_t *sock, sizebuf_t *data);
    i32(*SendUnreliableMessage)(qsocket_t *sock, sizebuf_t *data);
    qboolean(*CanSendMessage)(qsocket_t *sock);
    qboolean(*CanSendUnreliableMessage)(qsocket_t *sock);
    void(*Close)(qsocket_t *sock);
    void(*Shutdown)(void);
    i32 controlSock;
} net_driver_t;

typedef struct hostcache_s
{
    char name[16];
    char map[16];
    char cname[32];
    i32 users;
    i32 maxusers;
    i32 driver;
    i32 ldriver;
    qsockaddr_t addr;
} hostcache_t;

typedef struct _PollProcedure
{
    struct _PollProcedure *next;
    double nextTime;
    void(*procedure)(void);
    void *arg;
} PollProcedure;

//=====================================
// client

typedef struct usercmd_s
{
    vec3_t viewangles;
    // intended velocities
    float forwardmove;
    float sidemove;
    float upmove;
} usercmd_t;

typedef struct lightstyle_s
{
    i32 length;
    char map[MAX_STYLESTRING];
} lightstyle_t;

typedef struct scoreboard_s
{
    char name[MAX_SCOREBOARDNAME];
    float entertime;
    i32 frags;
    i32 colors;   // two 4 bit fields
    u8 translations[VID_GRADES * 256];
} scoreboard_t;

typedef struct cshift_s
{
    i32 destcolor[3];
    i32 percent;  // 0-256
} cshift_t;

typedef struct dlight_s
{
    vec3_t origin;
    float radius;
    float die;      // stop lighting after this time
    float decay;    // drop this each second
    float minlight; // don't add when contributing less
    i32 key;
} dlight_t;

typedef struct beam_s
{
    i32 entity;
    struct model_s *model;
    float endtime;
    vec3_t start, end;
} beam_t;

// the client_static_t structure is persistant through an arbitrary number
// of server connections
typedef struct client_static_s
{
    cactive_t state;

    // personalization data sent to server 
    char mapstring[MAX_QPATH];
    char spawnparms[MAX_MAPSTRING]; // to restart a level

// demo loop control
    i32 demonum;  // -1 = don't play demos
    char demos[MAX_DEMOS][MAX_DEMONAME];  // when not playing

// demo recording info must be here, because record is started before
// entering a map (and clearing client_state_t)
    qboolean demorecording;
    qboolean demoplayback;
    qboolean timedemo;
    i32 forcetrack;   // -1 = use normal cd track
    filehdl_t demofile;
    i32 td_lastframe;  // to meter out one message a frame
    i32 td_startframe;  // host_framecount at start
    float td_starttime;  // realtime at second frame of timedemo

// connection information
    i32 signon; // 0 to SIGNONS
    struct qsocket_s *netcon;
    sizebuf_t message; // writing buffer to send to server
} client_static_t;

// the client_state_t structure is wiped completely at every server signon
typedef struct client_state_s
{
    i32 movemessages;   // since connecting to this server
                        // throw out the first couple, so the player
                        // doesn't accidentally do something the first frame
    usercmd_t cmd; // last command sent to the server

// information for local display
    i32 stats[MAX_CL_STATS]; // health, etc
    i32 items; // inventory bit flags
    float item_gettime[32]; // cl.time of aquiring item, for blinking
    float faceanimtime; // use anim frame if cl.time < this

    cshift_t cshifts[NUM_CSHIFTS]; // color shifts for damage, powerups
    cshift_t prev_cshifts[NUM_CSHIFTS]; // and content types

// the client maintains its own idea of view angles, which are
// sent to the server each frame.  The server sets punchangle when
// the view is temporarliy offset, and an angle reset commands at the start
// of each level and after teleporting.
    vec3_t mviewangles[2]; // during demo playback viewangles is lerped between these
    vec3_t viewangles;
    vec3_t mvelocity[2]; // update by server, used for lean+bob, (0 is newest)
    vec3_t velocity; // lerped between mvelocity[0] and [1]
    vec3_t punchangle; // temporary offset

// pitch drifting vars
    float idealpitch;
    float pitchvel;
    qboolean nodrift;
    float driftmove;
    double laststop;
    float viewheight;
    float crouch; // local amount for smoothing stepups
    qboolean paused; // send over by server
    qboolean onground;
    qboolean inwater;
    i32 intermission;           // don't change view angle, full screen, etc
    i32 completed_time;         // latched at intermission start
    double mtime[2];            // the timestamp of last two messages 
    double time;                // clients view of time, should be between
                                // servertime and oldservertime to generate
                                // a lerp point for other data
    double oldtime;             // previous cl.time, time-oldtime is used
                                // to decay light values and smooth step ups
    float last_received_message; // (realtime) for net trouble icon
//
// information that is static for the entire time connected to a server
//
    struct model_s *model_precache[MAX_MODELS];
    struct sfx_s *sound_precache[MAX_SOUNDS];
    char levelname[40]; // for display on solo scoreboard
    i32 viewentity; // cl_entitites[cl.viewentity] = player
    i32 maxclients;
    i32 gametype;
    // refresh related state
    struct model_s *worldmodel; // cl_entitites[0].model
    struct efrag_s *free_efrags;
    i32 num_entities; // held in cl_entities array
    i32 num_statics; // held in cl_staticentities array
    entity_t viewent; // the gun model
    i32 cdtrack, looptrack; // cd audio
    // frag scoreboard
    struct scoreboard_s *scores; // [cl.maxclients]
} client_state_t;

// cl_input
typedef struct kbutton_s
{
    i32 down[2]; // key nums holding it down
    i32 state; // low bit is down state
} kbutton_t;

//=====================================
// pr_comp

typedef struct statement_s
{
    u16 op;
    i16 a, b, c;
} dstatement_t;

typedef struct ddef_s
{
    u16 type;   // if DEF_SAVEGLOBGAL bit is set
                // the variable needs to be saved in savegames
    u16 ofs;
    i32 s_name;
} ddef_t;

typedef struct dfunction_s
{
    i32 first_statement; // negative numbers are builtins
    i32 parm_start;
    i32 locals; // total ints of parms + locals
    i32 profile; // runtime
    i32 s_name;
    i32 s_file; // source file defined in
    i32 numparms;
    u8 parm_size[MAX_PARMS];
} dfunction_t;

typedef struct dprograms_s
{
    i32 version;
    i32 crc; // check of header file
    i32 ofs_statements;
    i32 numstatements; // statement 0 is an error
    i32 ofs_globaldefs;
    i32 numglobaldefs;
    i32 ofs_fielddefs;
    i32 numfielddefs;
    i32 ofs_functions;
    i32 numfunctions; // function 0 is an empty
    i32 ofs_strings;
    i32 numstrings; // first string is a null string
    i32 ofs_globals;
    i32 numglobals;
    i32 entityfields;
} dprograms_t;

//=====================================
// progdefs.q1

#define EXTERNAL_PROGDEFS 0
#if EXTERNAL_PROGDEFS
#   include "progdefs.q1"
#else
/* file generated by qcc, do not modify */

typedef struct globalvars_s
{
    i32 pad[28];
    i32 self;
    i32 other;
    i32 world;
    double time;
    double frametime;
    float force_retouch;
    string_t mapname;
    float deathmatch;
    float coop;
    float teamplay;
    float serverflags;
    float total_secrets;
    float total_monsters;
    float found_secrets;
    float killed_monsters;
    float parm1;
    float parm2;
    float parm3;
    float parm4;
    float parm5;
    float parm6;
    float parm7;
    float parm8;
    float parm9;
    float parm10;
    float parm11;
    float parm12;
    float parm13;
    float parm14;
    float parm15;
    float parm16;
    vec3_t v_forward;
    vec3_t v_up;
    vec3_t v_right;
    float trace_allsolid;
    float trace_startsolid;
    float trace_fraction;
    vec3_t trace_endpos;
    vec3_t trace_plane_normal;
    float trace_plane_dist;
    i32 trace_ent;
    float trace_inopen;
    float trace_inwater;
    i32 msg_entity;
    func_t main;
    func_t StartFrame;
    func_t PlayerPreThink;
    func_t PlayerPostThink;
    func_t ClientKill;
    func_t ClientConnect;
    func_t PutClientInServer;
    func_t ClientDisconnect;
    func_t SetNewParms;
    func_t SetChangeParms;
} globalvars_t;

typedef struct entvars_s
{
    float modelindex;
    vec3_t absmin;
    vec3_t absmax;
    float ltime;
    float movetype;
    float solid;
    vec3_t origin;
    vec3_t oldorigin;
    vec3_t velocity;
    vec3_t angles;
    vec3_t avelocity;
    vec3_t punchangle;
    string_t classname;
    string_t model;
    float frame;
    float skin;
    float effects;
    vec3_t mins;
    vec3_t maxs;
    vec3_t size;
    func_t touch;
    func_t use;
    func_t think;
    func_t blocked;
    float nextthink;
    i32 groundentity;
    float health;
    float frags;
    float weapon;
    string_t weaponmodel;
    float weaponframe;
    float currentammo;
    float ammo_shells;
    float ammo_nails;
    float ammo_rockets;
    float ammo_cells;
    float items;
    float takedamage;
    i32 chain;
    float deadflag;
    vec3_t view_ofs;
    float button0;
    float button1;
    float button2;
    float impulse;
    float fixangle;
    vec3_t v_angle;
    float idealpitch;
    string_t netname;
    i32 enemy;
    float flags;
    float colormap;
    float team;
    float max_health;
    float teleport_time;
    float armortype;
    float armorvalue;
    float waterlevel;
    float watertype;
    float ideal_yaw;
    float yaw_speed;
    i32 aiment;
    i32 goalentity;
    float spawnflags;
    string_t target;
    string_t targetname;
    float dmg_take;
    float dmg_save;
    i32 dmg_inflictor;
    i32 owner;
    vec3_t movedir;
    string_t message;
    float sounds;
    string_t noise;
    string_t noise1;
    string_t noise2;
    string_t noise3;
} entvars_t;

#define PROGHEADER_CRC 5927

#endif // EXTERNAL_PROGDEFS

//=====================================
// progs

typedef union eval_s
{
    string_t string;
    float _float;
    float vector[3];
    func_t function;
    i32 _int;
    i32 edict;
} eval_t;

typedef struct edict_s
{
    link_t area;        // linked to a division node or leaf
    qboolean free;
    i32 num_leafs;
    i16 leafnums[MAX_ENT_LEAFS];
    entity_state_t baseline;
    float freetime;     // sv.time when the object was freed
    entvars_t v;        // C exported fields from progs
    // other fields from progs come immediately after
} edict_t;

//=====================================
// server

typedef struct client_s
{
    qboolean active;        // false = client is free
    qboolean spawned;       // false = don't send datagrams
    qboolean dropasap;      // has been told to go to another level
    qboolean privileged;    // can execute any host command
    qboolean sendsignon;    // only valid before spawned
    double last_message;    // reliable messages must be sent periodically
    struct qsocket_s *netconnection; // communications handle
    usercmd_t cmd;          // movement
    vec3_t wishdir;         // intended motion calced from cmd
    sizebuf_t message;      // can be added to at any time,
                            // copied and clear once per frame
    u8 msgbuf[MAX_MSGLEN];
    struct edict_s *edict;  // EDICT_NUM(clientnum+1)
    char name[32];          // for printing to other people
    i32 colors;
    float ping_times[NUM_PING_TIMES];
    i32 num_pings;          // ping_times[num_pings%NUM_PING_TIMES]
    // spawn parms are carried from level to level
    float spawn_parms[NUM_SPAWN_PARMS];
    // client known data for deltas
    i32 old_frags;
} client_t;

typedef struct server_static_s
{
    i32 maxclients;
    client_t clients[MAX_SCOREBOARD]; // [maxclients]
    i32 serverflags; // episode completion information
    qboolean changelevel_issued; // cleared when at SV_SpawnServer
} server_static_t;

typedef struct server_s
{
    qboolean active;        // false if only a net client
    qboolean paused;
    qboolean loadgame;      // handle connections specially
    double time;
    i32 lastcheck;          // used by PF_checkclient
    double lastchecktime;
    char name[64];          // map name
    char modelname[64];     // maps/<name>.bsp, for model_precache[0]
    struct model_s *worldmodel;
    char *model_precache[MAX_MODELS];   // NULL terminated
    struct model_s *models[MAX_MODELS];
    char *sound_precache[MAX_SOUNDS];   // NULL terminated
    char *lightstyles[MAX_LIGHTSTYLES];
    i32 num_edicts;
    i32 max_edicts;
    // can NOT be array indexed, because edict_t is variable sized,
    // but can be used to reference the world ent
    struct edict_s *edicts;
    server_state_t state; // some actions are only valid during load
    sizebuf_t datagram;
    u8 datagram_buf[MAX_DATAGRAM];
    sizebuf_t reliable_datagram; // copied to all clients at end of frame
    u8 reliable_datagram_buf[MAX_DATAGRAM];
    sizebuf_t signon;
    u8 signon_buf[8192];
} server_t;

//=====================================
// world

typedef struct plane_s
{
    vec3_t normal;
    float dist;
} plane_t;

typedef struct trace_s
{
    qboolean allsolid; // if true, plane is not valid
    qboolean startsolid; // if true, the initial point was in a solid area
    qboolean inopen, inwater;
    float fraction; // time completed, 1.0 = didn't hit anything
    vec3_t endpos; // final position
    plane_t plane; // surface normal at impact
    struct edict_s *ent; // entity the surface is on
} trace_t;

//=====================================
