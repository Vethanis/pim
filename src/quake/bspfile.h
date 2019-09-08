#pragma once

#include "common/int_types.h"

namespace Quake
{
    static constexpr i32 BspVersion = 29;
    static constexpr i32 ToolVersion = 2;

    static constexpr i32 MaxKey = 32;
    static constexpr i32 MaxValue = 1024;

    static constexpr i32 NumMipLevels = 4;

    enum MaxMap : i32
    {
        MaxMap_Hulls = 4,
        MaxMap_Models = 256,
        MaxMap_Brushes = 4096,
        MaxMap_Entities = 1024,
        MaxMap_EntString = 65536,

        MaxMap_Planes = 32767,
        MaxMap_Nodes = 32767,
        MaxMap_ClipNodes = 32767,
        MaxMap_Leaves = 8192,
        MaxMap_Verts = 65535,
        MaxMap_Faces = 65535,
        MaxMap_MarkSurfaces = 65535,
        MaxMap_TexInfo = 4096,
        MaxMap_Edges = 256000,
        MaxMap_SurfEdges = 512000,
        MaxMap_Textures = 512,
        MaxMap_MipTex = 0x200000,
        MaxMap_Lighting = 0x100000,
        MaxMap_Visibility = 0x100000,

        MaxMap_Portals = 65536,
    };

    enum LumpId : i32
    {
        LumpId_Entities = 0,
        LumpId_Planes,
        LumpId_Textures,
        LumpId_Vertices,
        LumpId_Visibility,
        LumpId_Nodes,
        LumpId_TexInfo,
        LumpId_Faces,
        LumpId_Lighting,
        LumpId_ClipNodes,
        LumpId_Leaves,
        LumpId_MarkSurfaces,
        LumpId_Edges,
        LumpId_SurfEdges,
        LumpId_Models,

        LumpId_Count
    };

    struct Lump
    {
        i32 offset;
        i32 length;
    };

    struct Header
    {
        i32 version;
        Lump lumps[LumpId_Count];
    };

    struct Model
    {
        f32 mins[3];
        f32 maxs[3];
        f32 origin[3];
        i32 headNode[MaxMap_Hulls];
        i32 visLeaves; // not including the solid leaf 0
        i32 firstFace;
        i32 numFaces;
    };

    struct MipTexLump
    {
        i32 count;
        i32 offsets[4];
    };

    struct MipTex
    {
        char name[16];
        u32 width;
        u32 height;
        u32 offsets[NumMipLevels];
    };

    struct Vertex
    {
        f32 point[3];
    };

    enum PlaneId : i32
    {
        // axial planes
        PlaneId_X = 0,
        PlaneId_Y,
        PlaneId_Z,

        // non-axial, snapped to nearest
        PlaneId_AnyX,
        PlaneId_AnyY,
        PlaneId_AnyZ,

        PlaneId_Count
    };

    struct Plane
    {
        f32 normal[3];
        f32 distance;
        i32 type;   // PlaneId
    };

    enum Contents : i32
    {
        Contents_Empty = -1,
        Contents_Solid = -2,
        Contents_Water = -3,
        Contents_Slime = -4,
        Contents_Lava = -5,
        Contents_Sky = -6,
        Contents_Origin = -7, // removed at csg time
        Contents_Clip = -8,   // changed to Contents_Solid

        Contents_Current_0 = -9,
        Contents_Current_90 = -10,
        Contents_Current_180 = -11,
        Contents_Current_270 = -12,
        Contents_Current_Up = -13,
        Contents_Current_Down = -14,
    };

    struct Node
    {
        i32 planeNum;
        i16 children[2];    // negative numbers are -(leafs+1), not nodes
        i16 mins[3];        // for sphere culling
        i16 maxs[3];
        u16 firstFace;
        u16 numFaces;       // counting both sides
    };

    struct ClipNode
    {
        i32 planeNum;
        i16 children[2]; // negative numbers are contents
    };

    struct TexInfo
    {
        f32 vecs[2][4]; // [s/t][xyz offset]
        i32 mipTex;
        i32 flags;
    };
    static constexpr i32 TexSpecial = 1; // sky or slime, no lightmap or 256 subdivision

    // edge 0 is never used; negative edge nums are for CCW use of the edge in a face
    struct Edge
    {
        u16 v[2]; // vertex numbers
    };

    static constexpr i32 MaxLightMaps = 4;

    struct Face
    {
        i16 planeNum;
        i16 side;

        i32 firstEdge; // must support > 64k edges
        i16 numEdges;
        i16 texInfo;

        u8 styles[MaxLightMaps];
        i32 lightOffset; // start of [numStyles * surfSize] samples
    };

    enum Ambient : i32
    {
        Ambient_Water = 0,
        Ambient_Sky,
        Ambient_Slime,
        Ambient_Lava,

        Ambient_Count
    };

    struct Leaf
    {
        i32 contents;
        i32 visOffset;

        i16 mins[3];
        i16 maxs[3];

        u16 firstMarkSurface;
        u16 numMarkSurfaces;

        u8 ambientLevel[Ambient_Count];
    };

    struct Pair
    {
        Pair* next;
        char* key;
        char* value;
    };

    // https://quakewiki.org/wiki/Entity_guide
    struct Entity
    {
        f32 origin[3];
        i32 firstBrush;
        i32 numBrushes;
        Pair* pairs;
    };

    struct MapData
    {
        static constexpr i32 AngleUp = -1;
        static constexpr i32 AngleDown = -2;

        i32 numModels;
        Model models[MaxMap_Models];

        i32 visDataSize;
        u8 visData[MaxMap_Visibility];

        i32 lightDataSize;
        u8 lightData[MaxMap_Lighting];

        i32 texDataSize;
        u8 texData[MaxMap_MipTex]; // (MipTexLump)

        i32 entDataSize;
        char entData[MaxMap_EntString];

        i32 numLeaves;
        Leaf leaves[MaxMap_Leaves];

        i32 numPlanes;
        Plane planes[MaxMap_Planes];

        i32 numVertices;
        Vertex vertices[MaxMap_Verts];

        i32 numNodes;
        Node nodes[MaxMap_Nodes];

        i32 numTexInfo;
        TexInfo texInfo[MaxMap_TexInfo];

        i32 numFaces;
        Face faces[MaxMap_Faces];

        i32 numClipNodes;
        ClipNode clipNodes[MaxMap_ClipNodes];

        i32 numEdges;
        Edge edges[MaxMap_Edges];

        i32 numMarkSurfaces;
        u16 markSurfaces[MaxMap_MarkSurfaces];

        i32 numSurfEdges;
        i32 surfEdges[MaxMap_SurfEdges];

        i32 numEntities;
        Entity entities[MaxMap_Entities];
    };

    void DecompressVis(u8* input, u8* output);
    i32 CompressVis(u8* vis, u8* dst);

    void LoadBspFile(cstr filename);
    void WriteBspFile(cstr filename);
    void PrintBspFileSizes(void);
};

#undef def
