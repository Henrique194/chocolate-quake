/*
 * Copyright (C) 1996-1997 Id Software, Inc.
 * Copyright (C) Henrique Barateli, <henriquejb194@gmail.com>, et al.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#ifndef __BSPFILE__
#define __BSPFILE__

// upper design bounds

#define MAX_MAP_HULLS 4

#define MAX_MAP_MODELS    256
#define MAX_MAP_BRUSHES   4096
#define MAX_MAP_ENTITIES  1024
#define MAX_MAP_ENTSTRING 65536

#define MAX_MAP_PLANES       32767
#define MAX_MAP_NODES        32767 // because negative shorts are contents
#define MAX_MAP_CLIPNODES    32767 //
#define MAX_MAP_LEAFS        8192
#define MAX_MAP_VERTS        65535
#define MAX_MAP_FACES        65535
#define MAX_MAP_MARKSURFACES 65535
#define MAX_MAP_TEXINFO      4096
#define MAX_MAP_EDGES        256000
#define MAX_MAP_SURFEDGES    512000
#define MAX_MAP_TEXTURES     512
#define MAX_MAP_MIPTEX       0x200000
#define MAX_MAP_LIGHTING     0x100000
#define MAX_MAP_VISIBILITY   0x100000

#define MAX_MAP_PORTALS 65536

// key / value pair sizes

#define MAX_KEY   32
#define MAX_VALUE 1024

//=============================================================================


#define BSPVERSION  29
#define TOOLVERSION 2

typedef struct {
    i32 fileofs, filelen;
} lump_t;

#define LUMP_ENTITIES     0
#define LUMP_PLANES       1
#define LUMP_TEXTURES     2
#define LUMP_VERTEXES     3
#define LUMP_VISIBILITY   4
#define LUMP_NODES        5
#define LUMP_TEXINFO      6
#define LUMP_FACES        7
#define LUMP_LIGHTING     8
#define LUMP_CLIPNODES    9
#define LUMP_LEAFS        10
#define LUMP_MARKSURFACES 11
#define LUMP_EDGES        12
#define LUMP_SURFEDGES    13
#define LUMP_MODELS       14

#define HEADER_LUMPS 15

typedef struct {
    float mins[3], maxs[3];
    float origin[3];
    i32 headnode[MAX_MAP_HULLS];
    i32 visleafs; // not including the solid leaf 0
    i32 firstface, numfaces;
} dmodel_t;

typedef struct {
    i32 version;
    lump_t lumps[HEADER_LUMPS];
} dheader_t;

typedef struct {
    i32 nummiptex;
    i32 dataofs[4]; // [nummiptex]
} dmiptexlump_t;

#define MIPLEVELS 4
typedef struct miptex_s {
    char name[16];
    u32 width, height;
    u32 offsets[MIPLEVELS]; // four mip maps stored
} miptex_t;


typedef struct {
    float point[3];
} dvertex_t;


// 0-2 are axial planes
#define PLANE_X 0
#define PLANE_Y 1
#define PLANE_Z 2

// 3-5 are non-axial planes snapped to the nearest
#define PLANE_ANYX 3
#define PLANE_ANYY 4
#define PLANE_ANYZ 5

typedef struct {
    float normal[3];
    float dist;
    i32 type; // PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dplane_t;


#define CONTENTS_EMPTY  -1
#define CONTENTS_SOLID  -2
#define CONTENTS_WATER  -3
#define CONTENTS_SLIME  -4
#define CONTENTS_LAVA   -5
#define CONTENTS_SKY    -6
#define CONTENTS_ORIGIN -7 // removed at csg time
#define CONTENTS_CLIP   -8 // changed to contents_solid

#define CONTENTS_CURRENT_0    -9
#define CONTENTS_CURRENT_90   -10
#define CONTENTS_CURRENT_180  -11
#define CONTENTS_CURRENT_270  -12
#define CONTENTS_CURRENT_UP   -13
#define CONTENTS_CURRENT_DOWN -14


// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct {
    i32 planenum;
    i16 children[2]; // negative numbers are -(leafs+1), not nodes
    i16 mins[3];     // for sphere culling
    i16 maxs[3];
    u16 firstface;
    u16 numfaces; // counting both sides
} dnode_t;

typedef struct {
    i32 planenum;
    i16 children[2]; // negative numbers are contents
} dclipnode_t;


typedef struct texinfo_s {
    float vecs[2][4]; // [s/t][xyz offset]
    i32 miptex;
    i32 flags;
} texinfo_t;
#define TEX_SPECIAL 1 // sky or slime, no lightmap or 256 subdivision

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct {
    u16 v[2]; // vertex numbers
} dedge_t;

#define MAXLIGHTMAPS 4
typedef struct {
    i16 planenum;
    i16 side;

    i32 firstedge; // we must support > 64k edges
    i16 numedges;
    i16 texinfo;

    // lighting info
    byte styles[MAXLIGHTMAPS];
    i32 lightofs; // start of [numstyles*surfsize] samples
} dface_t;


#define AMBIENT_WATER 0
#define AMBIENT_SKY   1
#define AMBIENT_SLIME 2
#define AMBIENT_LAVA  3

#define NUM_AMBIENTS 4 // automatic ambient sounds

// leaf 0 is the generic CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
typedef struct {
    i32 contents;
    i32 visofs; // -1 = no visibility info

    i16 mins[3]; // for frustum culling
    i16 maxs[3];

    u16 firstmarksurface;
    u16 nummarksurfaces;

    byte ambient_level[NUM_AMBIENTS];
} dleaf_t;


//============================================================================

#ifndef QUAKE_GAME

#define ANGLE_UP   -1
#define ANGLE_DOWN -2


// the utilities get to be lazy and just use large static arrays

extern i32 nummodels;
extern dmodel_t dmodels[MAX_MAP_MODELS];

extern i32 visdatasize;
extern byte dvisdata[MAX_MAP_VISIBILITY];

extern i32 lightdatasize;
extern byte dlightdata[MAX_MAP_LIGHTING];

extern i32 texdatasize;
extern byte dtexdata[MAX_MAP_MIPTEX]; // (dmiptexlump_t)

extern i32 entdatasize;
extern char dentdata[MAX_MAP_ENTSTRING];

extern i32 numleafs;
extern dleaf_t dleafs[MAX_MAP_LEAFS];

extern i32 numplanes;
extern dplane_t dplanes[MAX_MAP_PLANES];

extern i32 numvertexes;
extern dvertex_t dvertexes[MAX_MAP_VERTS];

extern i32 numnodes;
extern dnode_t dnodes[MAX_MAP_NODES];

extern i32 numtexinfo;
extern texinfo_t texinfo[MAX_MAP_TEXINFO];

extern i32 numfaces;
extern dface_t dfaces[MAX_MAP_FACES];

extern i32 numclipnodes;
extern dclipnode_t dclipnodes[MAX_MAP_CLIPNODES];

extern i32 numedges;
extern dedge_t dedges[MAX_MAP_EDGES];

extern i32 nummarksurfaces;
extern u16 dmarksurfaces[MAX_MAP_MARKSURFACES];

extern i32 numsurfedges;
extern i32 dsurfedges[MAX_MAP_SURFEDGES];


void DecompressVis(byte* in, byte* decompressed);
i32 CompressVis(byte* vis, byte* dest);

void LoadBSPFile(char* filename);
void WriteBSPFile(char* filename);
void PrintBSPFileSizes(void);

//===============


typedef struct epair_s {
    struct epair_s* next;
    char* key;
    char* value;
} epair_t;

typedef struct {
    vec3_t origin;
    i32 firstbrush;
    i32 numbrushes;
    epair_t* epairs;
} entity_t;

extern i32 num_entities;
extern entity_t entities[MAX_MAP_ENTITIES];

void ParseEntities(void);
void UnparseEntities(void);

void SetKeyValue(entity_t* ent, char* key, char* value);
char* ValueForKey(entity_t* ent, char* key);
// will return "" if not present

vec_t FloatForKey(entity_t* ent, char* key);
void GetVectorForKey(entity_t* ent, char* key, vec3_t vec);

epair_t* ParseEpair(void);

#endif

#endif
