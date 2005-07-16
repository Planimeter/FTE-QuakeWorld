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

#ifndef __MODEL__
#define __MODEL__

#include "modelgen.h"
#include "spritegn.h"

struct hull_s;
struct trace_s;
struct edict_s;

typedef struct {
//	qboolean (*RecursiveHullCheck) (struct hull_s *hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, struct trace_s *trace);
	int (*HullPointContents) (struct hull_s *hull, vec3_t p);	//return FTE contents
} hullfuncs_t;

typedef struct {
	qboolean (*Trace)			(struct model_s *model, int hulloverride, int frame, vec3_t p1, vec3_t p2, vec3_t mins, vec3_t maxs, struct trace_s *trace);

	void (*FatPVS)				(vec3_t org, qboolean add);
	qboolean (*EdictInFatPVS)	(struct edict_s *edict);
	void (*FindTouchedLeafs_Q1)	(struct edict_s *ent);	//edict system as opposed to q2 game dll system.

	void (*LightPointValues)	(vec3_t point, vec3_t res_diffuse, vec3_t res_ambient, vec3_t res_dir);
	void (*StainNode)			(struct mnode_s *node, float *parms);
	void (*MarkLights)			(struct dlight_s *light, int bit, struct mnode_s *node);

	qbyte *(*LeafPVS)			(int num, struct model_s *model, qbyte *buffer);
	int	(*LeafForPoint)			(vec3_t point, struct model_s *model);
} bspfuncs_t;

typedef int index_t;
typedef struct mesh_s
{
    int				numvertexes;
	vec3_t			*xyz_array;
	vec3_t			*normals_array;
	vec2_t			*st_array;
	vec2_t			*lmst_array;
	byte_vec4_t		*colors_array;

    int				numindexes;
    index_t			*indexes;
	int				*trneighbors;
	vec3_t			*trnormals;

	vec3_t			mins, maxs;
	float			radius;

	vec3_t			lightaxis[3];

	unsigned int	patchWidth;
	unsigned int	patchHeight;
} mesh_t;
struct meshbuffer_s;

void R_PushMesh ( mesh_t *mesh, int features );
void R_RenderMeshBuffer ( struct meshbuffer_s *mb, qboolean shadowpass );
qboolean R_MeshWillExceed(mesh_t *mesh);

extern int gl_canbumpmap;




/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

// entity effects

#define	EF_BRIGHTFIELD			1
#define	EF_MUZZLEFLASH 			2
#define	EF_BRIGHTLIGHT 			4
#define	EF_DIMLIGHT 			8
#define	QWEF_FLAG1	 			16	//only applies to player entities
#define NQEF_NODRAW				16	//so packet entities are free to get this instead
#define	QWEF_FLAG2	 			32	//only applies to player entities
#define NQEF_ADDATIVE			32	//so packet entities are free to get this instead
#define EF_BLUE					64
#define EF_RED					128

#define	H2EF_NODRAW				0x80	//this is going to get complicated...

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/


//
// in memory representation
//
// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	vec3_t		position;
} mvertex_t;

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2


// plane_t structure
// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct mplane_s
{
	vec3_t	normal;
	float	dist;
	qbyte	type;			// for texture axis selection and fast side tests
	qbyte	signbits;		// signx + signy<<1 + signz<<1
	qbyte	pad[2];
} mplane_t;

typedef struct texture_s
{
	char		name[64];
	unsigned	width, height;

	qbyte	pixbytes;
	qbyte	alphaed;	//gl_blend needed on this surface.

	int parttype;

	int			gl_texturenum;
	int			gl_texturenumfb;
	int			gl_texturenumbumpmap;
	int			gl_texturenumspec;

	struct shader_s	*shader;

	struct msurface_s	*texturechain;	// for gl_texsort drawing
	int			anim_total;				// total tenths in sequence ( 0 = no)
	int			anim_min, anim_max;		// time for this frame min <=time< max
	struct texture_s *anim_next;		// in the animation sequence
	struct texture_s *alternate_anims;	// bmodels in frmae 1 use these
	unsigned	offsets[MIPLEVELS];		// four mip maps stored
} texture_t;

#define SURF_DRAWSKYBOX		1
#define	SURF_PLANEBACK		2
#define	SURF_DRAWSKY		4
#define SURF_DRAWSPRITE		8
#define SURF_DRAWTURB		0x10
#define SURF_DRAWTILED		0x20
#define SURF_DRAWBACKGROUND	0x40
#define SURF_UNDERWATER		0x80
#define SURF_DONTWARP		0x100
#define SURF_BULLETEN		0x200
#define SURF_DRAWALPHA		0x10000

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct
{
	unsigned short	v[2];
	unsigned int	cachededgeoffset;
} medge_t;

typedef struct mtexinfo_s
{
	float		vecs[2][4];
	float		mipadjust;
	texture_t	*texture;
	int			flags;

	//it's a q2 thing.
	int			numframes;
	struct mtexinfo_s	*next;
} mtexinfo_t;

#define SPECULAR
#ifdef SPECULAR
#define	VERTEXSIZE	10
#else
#define	VERTEXSIZE	7
#endif

typedef struct glpoly_s
{
	struct	glpoly_s	*next;
	int		numverts;
	float	verts[4][VERTEXSIZE];	// variable sized (xyz s1t1 s2t2 (ldir_xyz)
} glpoly_t;

#ifdef Q3SHADERS
typedef struct mfog_s
{
	struct shader_s		*shader;

	mplane_t		*visibleplane;

	int				numplanes;
	mplane_t		**planes;
} mfog_t;
#endif

#if MAX_SWDECALS
typedef struct decal_s {
	int xpos, ypos;
	struct msurface_s *owner;
	struct decal_s *next;
	struct decal_s *prev;
} decal_t;
#endif


typedef struct msurface_s
{
	int			visframe;		// should be drawn when node is crossed
	int			shadowframe;

	mplane_t	*plane;
	int			flags;

	int			firstedge;	// look up in model->surfedges[], negative numbers
	int			numedges;	// are backwards edges

#ifdef SWQUAKE
	struct surfcache_s	*cachespots[MIPLEVELS];
#endif
	struct	msurface_s	*nextalphasurface;
	
	short		texturemins[2];
	short		extents[2];

	int			light_s, light_t;	// gl lightmap coordinates

#ifdef Q3SHADERS
	mfog_t		*fog;
#endif
	mesh_t		*mesh;
	entity_t	*ownerent;
	struct	msurface_s	*texturechain;
#if 0
	vec3_t normal;
#endif
	mtexinfo_t	*texinfo;
	
// lighting info
	int			dlightframe;
	int			dlightbits;

	int			lightmaptexturenum;
	qbyte		styles[MAXLIGHTMAPS];
	int			cached_light[MAXLIGHTMAPS];	// values currently used in lightmap
	qboolean	cached_dlight;				// true if dynamic light in cache
#ifdef PEXT_LIGHTSTYLECOL
	qbyte		cached_colour[MAXLIGHTMAPS];
#endif
#ifndef NOSTAINS
	qboolean stained;
#endif
	qbyte		*samples;		// [numstyles*surfsize]
#ifdef MAX_SWDECALS
	decal_t		*decal;
#endif
} msurface_t;

typedef struct mnode_s
{
// common with leaf
	int			contents;		// 0, to differentiate from leafs
	int			visframe;		// node needs to be traversed if current
	int			shadowframe;
	
	float		minmaxs[6];		// for bounding box culling

	struct mnode_s	*parent;

// node specific
	mplane_t	*plane;
	struct mnode_s	*children[2];
#ifdef Q2BSPS
	int childnum[2];
#endif

	unsigned short		firstsurface;
	unsigned short		numsurfaces;
} mnode_t;



typedef struct mleaf_s
{
// common with node
	int			contents;		// wil be a negative contents number
	int			visframe;		// node needs to be traversed if current
	int			shadowframe;

	float		minmaxs[6];		// for bounding box culling

	struct mnode_s	*parent;

// leaf specific
	qbyte		*compressed_vis;
	struct efrag_s		*efrags;

	msurface_t	**firstmarksurface;
	int			nummarksurfaces;
	int			key;			// BSP sequence number for leaf's contents
	qbyte		ambient_sound_level[NUM_AMBIENTS];
#ifdef Q2BSPS
	//it's a q2 thing
	int			cluster;
	int			area;
	unsigned short	firstleafbrush;
	unsigned short	numleafbrushes;

	unsigned short	firstleafface;	//q3 addititions
	unsigned short	numleaffaces;

	unsigned short	numleafpatches;
	unsigned short	firstleafpatch;

	struct mleaf_s *vischain;
#endif
} mleaf_t;


typedef struct
{
	float		mins[3], maxs[3];
	float		origin[3];
	int			headnode[MAX_MAP_HULLSM];
	int			visleafs;		// not including the solid leaf 0
	int			firstface, numfaces;
	qboolean	hullavailable[MAX_MAP_HULLSM];
} mmodel_t;


// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct hull_s
{
	dclipnode_t	*clipnodes;
	mplane_t	*planes;
	int			firstclipnode;
	int			lastclipnode;
	vec3_t		clip_mins;
	vec3_t		clip_maxs;
	int			available;

	hullfuncs_t funcs;
} hull_t;


void Q1BSP_SetHullFuncs(hull_t *hull);

/*
==============================================================================

SPRITE MODELS

==============================================================================
*/


// FIXME: shorten these?
typedef struct mspriteframe_s
{
	int		width;
	int		height;
	float	up, down, left, right;
	int		gl_texturenum;
#ifdef SWQUAKE
	qbyte	pixels[4];
#endif
} mspriteframe_t;

typedef struct
{
	int				numframes;
	float			*intervals;
	mspriteframe_t	*frames[1];
} mspritegroup_t;

typedef struct
{
	spriteframetype_t	type;
	mspriteframe_t		*frameptr;
} mspriteframedesc_t;

typedef struct
{
	int					type;
	int					maxwidth;
	int					maxheight;
	int					numframes;
	float				beamlength;		// remove?
	void				*cachespot;		// remove?
	mspriteframedesc_t	frames[1];
} msprite_t;


/*
==============================================================================

ALIAS MODELS

Alias models are position independent, so the cache manager can move them.
==============================================================================
*/

typedef struct {
	int		s;
	int		t;
} mstvert_t;

typedef struct
{
#ifdef SWQUAKE
	aliasframetype_t	type;
#endif
	int					firstpose;
	int					numposes;
	float				interval;
	dtrivertx_t			bboxmin;
	dtrivertx_t			bboxmax;
	
	vec3_t		scale;
	vec3_t		scale_origin;

	int					frame;
	char				name[16];
} maliasframedesc_t;

typedef struct
{
	dtrivertx_t			bboxmin;
	dtrivertx_t			bboxmax;
	int					frame;
} maliasgroupframedesc_t;

typedef struct
{
	int						numframes;
	int						intervals;
	maliasgroupframedesc_t	frames[1];
} maliasgroup_t;

// !!! if this is changed, it must be changed in asm_draw.h too !!!
typedef struct mtriangle_s {
	int					xyz_index[3];
	int					st_index[3];
	
	int	pad[2];
} mtriangle_t;


#define	MAX_SKINS	32
typedef struct {
	int			ident;
	int			version;
	vec3_t		scale;
	vec3_t		scale_origin;
	float		boundingradius;
	vec3_t		eyeposition;
	int			numskins;
	int			skinwidth;
	int			skinheight;
	int			numverts;
	int			numtris;
	int			numframes;
	synctype_t	synctype;
	int			flags;
	float		size;
#ifdef SWQUAKE
	int					model;
	int					stverts;
	int					skindesc;
#endif
	int					numposes;
	int					poseverts;
	int					posedata;	// numposes*poseverts trivert_t

	int					baseposedata; //original verts for triangles to reference
	int					triangles; //we need tri data for shadow volumes

	int					commands;	// gl command list with embedded s/t
	int					gl_texturenum[MAX_SKINS][4];
	int					texels[MAX_SKINS];
	maliasframedesc_t	frames[1];	// variable sized
} aliashdr_t;

#define	MAXALIASVERTS	2048
#define ALIAS_Z_CLIP_PLANE	5
#define	MAXALIASFRAMES	256
#define	MAXALIASTRIS	2048
extern	aliashdr_t	*pheader;
extern	mstvert_t	stverts[MAXALIASVERTS*2];
extern	mtriangle_t	triangles[MAXALIASTRIS];
extern	dtrivertx_t	*poseverts[MAXALIASFRAMES];




/*
========================================================================

.MD2 triangle model file format

========================================================================
*/

// LordHavoc: grabbed this from the Q2 utility source,
// renamed a things to avoid conflicts

#define MD2IDALIASHEADER		(('2'<<24)+('P'<<16)+('D'<<8)+'I')
#define MD2ALIAS_VERSION	8

#define	MD2MAX_TRIANGLES	4096
#define MD2MAX_VERTS		2048
#define MD2MAX_FRAMES		512 
#define MD2MAX_SKINS		32
#define	MD2MAX_SKINNAME		64
// sanity checking size
#define MD2MAX_SIZE	(1024*4200)

typedef struct
{
	short	s;
	short	t;
} md2stvert_t;

typedef struct 
{
	short	index_xyz[3];
	short	index_st[3];
} md2triangle_t;

typedef struct
{
	qbyte	v[3];			// scaled qbyte to fit in frame mins/maxs
	qbyte	lightnormalindex;
} md2trivertx_t;

#define MD2TRIVERTX_V0   0
#define MD2TRIVERTX_V1   1
#define MD2TRIVERTX_V2   2
#define MD2TRIVERTX_LNI  3
#define MD2TRIVERTX_SIZE 4

typedef struct
{
	float		scale[3];	// multiply qbyte verts by this
	float		translate[3];	// then add this
	char		name[16];	// frame name from grabbing
	md2trivertx_t	verts[1];	// variable sized
} md2frame_t;


// the glcmd format:
// a positive integer starts a tristrip command, followed by that many
// vertex structures.
// a negative integer starts a trifan command, followed by -x vertexes
// a zero indicates the end of the command list.
// a vertex consists of a floating point s, a floating point t,
// and an integer vertex index.


typedef struct
{
	int			ident;
	int			version;

	int			skinwidth;
	int			skinheight;
	int			framesize;		// qbyte size of each frame

	int			num_skins;
	int			num_xyz;
	int			num_st;			// greater than num_xyz for seams
	int			num_tris;
	int			num_glcmds;		// dwords in strip/fan command list
	int			num_frames;

	int			ofs_skins;		// each skin is a MAX_SKINNAME string
	int			ofs_st;			// qbyte offset from start for stverts
	int			ofs_tris;		// offset for dtriangles
	int			ofs_frames;		// offset for first frame
	int			ofs_glcmds;	
	int			ofs_end;		// end of file

	int			gl_texturenum[MAX_SKINS];
} md2_t;

#define ALIASTYPE_MDL 1
#define ALIASTYPE_MD2 2





//===================================================================


typedef struct
{
	qbyte			ambient[3];
	qbyte			diffuse[3];
	qbyte			direction[2];
} dq3gridlight_t;

typedef struct
{
	unsigned char	ambient[4][3];
	unsigned char	diffuse[4][3];
	unsigned char	styles[4];
	unsigned char	direction[2];
} rbspgridlight_t;

//q3 based
typedef struct {
	int gridBounds[4];	//3 = 0*1
	vec3_t gridMins;
	vec3_t gridSize;
	int numlightgridelems;
//rbsp specific
	rbspgridlight_t *rbspelements;
	unsigned short *rbspindexes;
//non-rbsp specific
	dq3gridlight_t *lightgrid;

	//the reason rbsp is seperate from the non-rbsp is because it allows better memory compression.
	//I chose not to expand at loadtime because q3 would suffer from greater cache misses.
} q3lightgridinfo_t;


//
// Whole model
//

typedef enum {mod_brush, mod_sprite, mod_alias, mod_dummy, mod_halflife} modtype_t;
typedef enum {fg_quake, fg_quake2, fg_quake3, fg_halflife, fg_new, fg_doom} fromgame_t;	//useful when we have very similar model types. (eg quake/halflife bsps)

#define	EF_ROCKET	1			// leave a trail
#define	EF_GRENADE	2			// leave a trail
#define	EF_GIB		4			// leave a trail
#define	EF_ROTATE	8			// rotate (bonus items)
#define	EF_TRACER	16			// green split trail
#define	EF_ZOMGIB	32			// small blood trail
#define	EF_TRACER2	64			// orange split trail + rotate
#define	EF_TRACER3	128			// purple trail

//hexen2.
#define  EF_FIREBALL		 256			// Yellow transparent trail in all directions
#define  EF_ICE				 512			// Blue-white transparent trail, with gravity
#define  EF_MIP_MAP			1024			// This model has mip-maps
#define  EF_SPIT			2048			// Black transparent trail with negative light
#define  EF_TRANSPARENT		4096			// Transparent sprite
#define  EF_SPELL           8192			// Vertical spray of particles
#define  EF_HOLEY		   16384			// Solid model with color 0
#define  EF_SPECIAL_TRANS  32768			// Translucency through the particle table
#define  EF_FACE_VIEW	   65536			// Poly Model always faces you
#define  EF_VORP_MISSILE  131072			// leave a trail at top and bottom of model
#define  EF_SET_STAFF     262144			// slowly move up and left/right
#define  EF_MAGICMISSILE  524288            // a trickle of blue/white particles with gravity
#define  EF_BONESHARD    1048576           // a trickle of brown particles with gravity
#define  EF_SCARAB       2097152           // white transparent particles with little gravity
#define  EF_ACIDBALL	 4194304			// Green drippy acid shit
#define  EF_BLOODSHOT	 8388608			// Blood rain shot trail

typedef union {
	struct {
		int numlinedefs;
		int numsidedefs;
		int numsectors;
	} doom;
} specificmodeltype_t;

typedef struct model_s
{
	char		name[MAX_QPATH];
	qboolean	needload;		// bmodels and sprites don't cache normally

	modtype_t	type;
	fromgame_t	fromgame;
     
	int			numframes;
	synctype_t	synctype;
	
	int			flags;
	int			particleeffect;
	qboolean	particleengulphs;
	int			particletrail;
	qboolean	nodefaulttrail;

	qboolean rgblighting;	//.lit, halflife.

//
// volume occupied by the model graphics
//		
	vec3_t		mins, maxs;
	float		radius;

//
// solid volume for clipping 
//
	qboolean	clipbox;
	vec3_t		clipmins, clipmaxs;

//
// brush model
//
	int			firstmodelsurface, nummodelsurfaces;

	int			numsubmodels;
	mmodel_t	*submodels;

	int			numplanes;
	mplane_t	*planes;

	int			numleafs;		// number of visible leafs, not counting 0
	mleaf_t		*leafs;

	int			numvertexes;
	mvertex_t	*vertexes;

	int			numedges;
	medge_t		*edges;

	int			numnodes;
	mnode_t		*nodes;

	int			numtexinfo;
	mtexinfo_t	*texinfo;

	int			numsurfaces;
	msurface_t	*surfaces;

	int			numsurfedges;
	int			*surfedges;

	int			numclipnodes;
	dclipnode_t	*clipnodes;

	int			nummarksurfaces;
	msurface_t	**marksurfaces;

	hull_t		hulls[MAX_MAP_HULLSM];

	int			numtextures;
	texture_t	**textures;

	qbyte		*visdata;
	void	*vis;
	qbyte		*lightdata;
	qbyte		*deluxdata;
	q3lightgridinfo_t *lightgrid;
	char		*entities;

	struct terrain_s *terrain;

	unsigned	checksum;
	unsigned	checksum2;


	bspfuncs_t	funcs;
//
// additional model data
//
	cache_user_t	cache;		// only access through Mod_Extradata

} model_t;

//============================================================================
/*
void	Mod_Init (void);
void	Mod_ClearAll (void);
model_t *Mod_ForName (char *name, qboolean crash);
model_t *Mod_FindName (char *name);
void	*Mod_Extradata (model_t *mod);	// handles caching
void	Mod_TouchModel (char *name);

mleaf_t *Mod_PointInLeaf (float *p, model_t *model);
qbyte	*Mod_LeafPVS (mleaf_t *leaf, model_t *model);
*/
#endif	// __MODEL__












#ifdef Q2BSPS

#ifdef __cplusplus
//#pragma message ("                  c++ stinks")
#else

void CM_Init(void);

qboolean	CM_SetAreaPortalState (int portalnum, qboolean open);
qboolean	CM_HeadnodeVisible (int nodenum, qbyte *visbits);
qboolean	VARGS CM_AreasConnected (int area1, int area2);
int		CM_NumClusters (void);
int		CM_ClusterSize (void);
int		CM_LeafContents (int leafnum);
int		CM_LeafCluster (int leafnum);
int		CM_LeafArea (int leafnum);
int		CM_WriteAreaBits (qbyte *buffer, int area);
int		CM_PointLeafnum (vec3_t p);
qbyte	*CM_ClusterPVS (int cluster, qbyte *buffer);
qbyte	*CM_ClusterPHS (int cluster);
int		CM_BoxLeafnums (vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode);
int		CM_PointContents (vec3_t p, int headnode);
struct trace_s	CM_BoxTrace (vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int headnode, int brushmask);
int		CM_HeadnodeForBox (vec3_t mins, vec3_t maxs);
struct trace_s	CM_TransformedBoxTrace (vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, int headnode, int brushmask, vec3_t origin, vec3_t angles);
struct model_s *CM_TempBoxModel(vec3_t mins, vec3_t maxs);

void Mod_ParseInfoFromEntityLump(const char *data);

void	VARGS CMQ2_SetAreaPortalState (int portalnum, qboolean open);
#endif



#endif	//Q2BSPS




typedef struct
{
	aliasskintype_t		type;
	void				*pcachespot;
	int					skin;
} maliasskindesc_t;

typedef struct
{
	int					numskins;
	int					intervals;
	maliasskindesc_t	skindescs[1];
} maliasskingroup_t;
