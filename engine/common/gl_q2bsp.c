#include "quakedef.h"
#ifdef RGLQUAKE
#include "glquake.h"
#include "shader.h"
#endif

#define MAX_Q3MAP_INDICES 0x80000
#define	MAX_Q3MAP_VERTEXES	0x80000
#define	MAX_Q3MAP_BRUSHSIDES	0x30000
#define MAX_CM_BRUSHSIDES		(MAX_Q3MAP_BRUSHSIDES << 1)
#define MAX_CM_BRUSHES			(MAX_Q2MAP_BRUSHES << 1)
#define MAX_CM_PATCH_VERTS		(4096)
#define MAX_CM_FACES			(MAX_Q2MAP_FACES)
#define MAX_CM_PATCHES			(0x10000)
#define MAX_CM_LEAFFACES		(MAX_Q2MAP_LEAFFACES)
#define	MAX_CM_AREAS			MAX_Q2MAP_AREAS

#define	Q3SURF_NODRAW			0x80	// don't generate a drawsurface at all
#define	Q3SURF_SKIP				0x200	// completely ignore, allowing non-closed brushes
#define	Q3SURF_NONSOLID			0x4000	// don't collide against curves with this set

#if Q3SURF_NODRAW != SURF_NODRAW
#error "nodraw isn't constant"
#endif

#ifdef SWQUAKE
extern qboolean r_usinglits;
#endif

extern cvar_t r_shadow_bumpscale_basetexture;

//these are in model.c (or gl_model.c)
qboolean GLMod_LoadVertexes (lump_t *l);
qboolean GLMod_LoadEdges (lump_t *l);
qboolean GLMod_LoadMarksurfaces (lump_t *l);
qboolean GLMod_LoadSurfedges (lump_t *l);
void GLMod_LoadLighting (lump_t *l);

qboolean SWMod_LoadVertexes (lump_t *l);
qboolean SWMod_LoadEdges (lump_t *l);
qboolean SWMod_LoadMarksurfaces (lump_t *l);
qboolean SWMod_LoadSurfedges (lump_t *l);
void SWMod_LoadLighting (lump_t *l);


void Q2BSP_SetHullFuncs(hull_t *hull);
qboolean CM_Trace(model_t *model, int forcehullnum, int frame, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, trace_t *trace);
qboolean CM_NativeTrace(model_t *model, int forcehullnum, int frame, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, unsigned int contents, trace_t *trace);
unsigned int CM_NativeContents(struct model_s *model, int hulloverride, int frame, vec3_t p, vec3_t mins, vec3_t maxs);
unsigned int Q2BSP_PointContents(model_t *mod, vec3_t p);

qbyte			areabits[MAX_Q2MAP_AREAS/8];


extern char	loadname[32];
extern model_t	*loadmodel;
float RadiusFromBounds (vec3_t mins, vec3_t maxs)
{
	int		i;
	vec3_t	corner;

	for (i=0 ; i<3 ; i++)
	{
		corner[i] = fabs(mins[i]) > fabs(maxs[i]) ? fabs(mins[i]) : fabs(maxs[i]);
	}

	return Length (corner);
}

void CalcSurfaceExtents (msurface_t *s)
{
	float	mins[2], maxs[2], val;
	int		i,j, e;
	mvertex_t	*v;
	mtexinfo_t	*tex;
	int		bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;

	for (i=0 ; i<s->numedges ; i++)
	{
		e = loadmodel->surfedges[s->firstedge+i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];

		for (j=0 ; j<2 ; j++)
		{
			val = v->position[0] * tex->vecs[j][0] +
				v->position[1] * tex->vecs[j][1] +
				v->position[2] * tex->vecs[j][2] +
				tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i=0 ; i<2 ; i++)
	{
		bmins[i] = floor(mins[i]/16);
		bmaxs[i] = ceil(maxs[i]/16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;

//		if ( !(tex->flags & TEX_SPECIAL) && s->extents[i] > 512 )	//q2 uses 512. probably for skys.
//			Sys_Error ("Bad surface extents");
	}
}

void AddPointToBounds (vec3_t v, vec3_t mins, vec3_t maxs)
{
	int		i;
	vec_t	val;

	for (i=0 ; i<3 ; i++)
	{
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}

void ClearBounds (vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}


#ifdef Q2BSPS

qbyte *ReadPCXPalette(qbyte *buf, int len, qbyte *out);

#ifdef SERVERONLY
#define Host_Error SV_Error
#endif

extern model_t	*loadmodel;
extern qbyte *mod_base;




unsigned char d_q28to24table[1024];


/*
typedef struct q2csurface_s
{
	char		name[16];
	int			flags;
	int			value;
} q2csurface_t;
*/

typedef struct q2mapsurface_s  // used internally due to name len probs //ZOID
{
	q2csurface_t	c;
	char		rname[32];
} q2mapsurface_t;

























typedef struct cmodel_s
{
	vec3_t		mins, maxs;
	vec3_t		origin;		// for sounds or lights
	int			headnode;
	int		numsurfaces;
	int		firstsurface;

	int firstbrush;	//q3 submodels are considered small enough that you will never need to walk any sort of tree.
	int num_brushes;//the brushes are checked instead.
} q2cmodel_t;







typedef struct
{
	mplane_t	*plane;
	q2mapsurface_t	*surface;
} q2cbrushside_t;

typedef struct
{
	int			contents;
	int			numsides;
	int			firstbrushside;
	int			checkcount;		// to avoid repeated testings
} q2cbrush_t;

typedef struct
{
	int		numareaportals;
	int		firstareaportal;
	int		floodnum;			// if two areas have equal floodnums, they are connected
	int		floodvalid;
} q2carea_t;

typedef struct
{
	int			numareaportals[MAX_CM_AREAS];
} q3carea_t;

typedef struct
{
	vec3_t		absmins, absmaxs;

	int			numbrushes;
	q2cbrush_t	*brushes;

	q2mapsurface_t	*surface;
	int			checkcount;		// to avoid repeated testings
} q3cpatch_t;

typedef struct
{
	int		facetype;

	int		numverts;
	int		firstvert;

	int		shadernum;
	int		patch_cp[2];
} q3cface_t;

int			checkcount;

//FIXME: Unlimit these.
char		map_name[MAX_QPATH];

int			numbrushsides;
q2cbrushside_t map_brushsides[MAX_Q2MAP_BRUSHSIDES];

int			numtexinfo;
q2mapsurface_t	*map_surfaces;

int			numplanes;
mplane_t	map_planes[MAX_Q2MAP_PLANES+6];		// extra for box hull

int			numleafs = 1;	// allow leaf funcs to be called without a map
mleaf_t		map_leafs[MAX_MAP_LEAFS];
int			emptyleaf;

int			numleafbrushes;
int			map_leafbrushes[MAX_Q2MAP_LEAFBRUSHES];

int			numcmodels;
q2cmodel_t	map_cmodels[MAX_Q2MAP_MODELS];

int			numbrushes;
q2cbrush_t	map_brushes[MAX_Q2MAP_BRUSHES];

int			numvisibility;
qbyte		map_visibility[MAX_Q2MAP_VISIBILITY];
q2dvis_t		*map_q2vis = (q2dvis_t *)map_visibility;
q3dvis_t		*map_q3pvs = (q3dvis_t *)map_visibility;
qbyte		map_hearability[MAX_Q2MAP_VISIBILITY];
q3dvis_t		*map_q3phs = (q3dvis_t *)map_hearability;

int			numentitychars;
char		*map_entitystring;

int			numareas = 1;
q2carea_t		map_q2areas[MAX_Q2MAP_AREAS];
q3carea_t		map_q3areas[MAX_CM_AREAS];

int			numareaportals;
q2dareaportal_t map_areaportals[MAX_Q2MAP_AREAPORTALS];

q3cpatch_t	map_patches[MAX_CM_PATCHES];
int			numpatches;

int			map_leafpatches[MAX_CM_LEAFFACES];
int			numleafpatches;

int			numclusters = 1;

q2mapsurface_t	nullsurface;

int			floodvalid;

qbyte	portalopen[MAX_Q2MAP_AREAPORTALS];	//memset will work if it's a qbyte, really it should be a qboolean


static int	mapisq3;
cvar_t		map_noareas			= SCVAR("map_noareas", "1");	//1 for lack of mod support.
cvar_t		map_noCurves		= SCVARF("map_noCurves", "0", CVAR_CHEAT);
cvar_t		map_autoopenportals	= SCVAR("map_autoopenportals", "1");	//1 for lack of mod support.
cvar_t		r_subdivisions		= SCVAR("r_subdivisions", "2");

int		CM_NumInlineModels (model_t *model);
q2cmodel_t	*CM_InlineModel (char *name);
void	CM_InitBoxHull (void);
void	FloodAreaConnections (void);


int		c_pointcontents;
int		c_traces, c_brush_traces;


vec3_t		*map_verts;	//3points
int			numvertexes;

vec2_t		*map_vertstmexcoords;
vec2_t		*map_vertlstmexcoords;
byte_vec4_t *map_colors_array;
vec3_t		*map_normals_array;

#ifdef Q3SHADERS
typedef struct {
	char		shader[MAX_QPATH];
	int			brushNum;
	int			visibleSide;	// the brush side that ray tests need to clip against (-1 == none)
} dfog_t;

mfog_t		*map_fogs;
int			map_numfogs;
#endif

q3cface_t	*map_faces;
int			numfaces;

index_t *map_surfindexes;
int	map_numsurfindexes;

int			*map_leaffaces;
int			numleaffaces;









int	PlaneTypeForNormal ( vec3_t normal )
{
	vec_t	ax, ay, az;

// NOTE: should these have an epsilon around 1.0?
	if ( normal[0] >= 1.0)
		return PLANE_X;
	if ( normal[1] >= 1.0 )
		return PLANE_Y;
	if ( normal[2] >= 1.0 )
		return PLANE_Z;

	ax = fabs( normal[0] );
	ay = fabs( normal[1] );
	az = fabs( normal[2] );

	if ( ax >= ay && ax >= az )
		return PLANE_ANYX;
	if ( ay >= ax && ay >= az )
		return PLANE_ANYY;
	return PLANE_ANYZ;
}

void CategorizePlane ( mplane_t *plane )
{
	int i;

	plane->signbits = 0;
	plane->type = PLANE_ANYZ;
	for (i = 0; i < 3; i++)
	{
		if (plane->normal[i] < 0)
			plane->signbits |= 1<<i;
		if (plane->normal[i] == 1.0f)
			plane->type = i;
	}
	plane->type = PlaneTypeForNormal(plane->normal);
}

void PlaneFromPoints ( vec3_t verts[3], mplane_t *plane )
{
	vec3_t	v1, v2;

	VectorSubtract( verts[1], verts[0], v1 );
	VectorSubtract( verts[2], verts[0], v2 );
	CrossProduct( v2, v1, plane->normal );
	VectorNormalize( plane->normal );
	plane->dist = DotProduct( verts[0], plane->normal );
}

qboolean BoundsIntersect (vec3_t mins1, vec3_t maxs1, vec3_t mins2, vec3_t maxs2)
{
	return (mins1[0] <= maxs2[0] && mins1[1] <= maxs2[1] && mins1[2] <= maxs2[2] &&
		 maxs1[0] >= mins2[0] && maxs1[1] >= mins2[1] && maxs1[2] >= mins2[2]);
}


#define VectorAvg(a,b,c)		((c)[0]=((a)[0]+(b)[0])*0.5f,(c)[1]=((a)[1]+(b)[1])*0.5f, (c)[2]=((a)[2]+(b)[2])*0.5f)
#define Vector4Copy(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3])
#define Vector4Scale(in,scale,out)		((out)[0]=(in)[0]*scale,(out)[1]=(in)[1]*scale,(out)[2]=(in)[2]*scale,(out)[3]=(in)[3]*scale)
#define Vector4Add(a,b,c)		((c)[0]=(((a[0])+(b[0]))),(c)[1]=(((a[1])+(b[1]))),(c)[2]=(((a[2])+(b[2]))),(c)[3]=(((a[3])+(b[3]))))

/*
===============
Patch_FlatnessTest
===============
*/
static int Patch_FlatnessTest ( float maxflat, const vec3_t point0, const vec3_t point1, const vec3_t point2 )
{
	vec3_t v1, v2, v3;
	vec3_t t, n;
	float dist, d, l;
	int ft0, ft1;

	VectorSubtract ( point2, point0, n );
	l = VectorNormalize ( n );

	if ( !l ) {
		return 0;
	}

	VectorSubtract ( point1, point0, t );
	d = -DotProduct ( t, n );
	VectorMA ( t, d, n, t );
	dist = VectorLength ( t );

	if ( fabs(dist) <= maxflat ) {
		return 0;
	}

	VectorAvg ( point1, point0, v1 );
	VectorAvg ( point2, point1, v2 );
	VectorAvg ( v1, v2, v3 );

	ft0 = Patch_FlatnessTest ( maxflat, point0, v1, v3 );
	ft1 = Patch_FlatnessTest ( maxflat, v3, v2, point2 );

	return 1 + (int)floor( max ( ft0, ft1 ) + 0.5f );
}

/*
===============
Patch_GetFlatness
===============
*/
void Patch_GetFlatness ( float maxflat, const vec3_t *points, int *patch_cp, int *flat )
{
	int i, p, u, v;

	flat[0] = flat[1] = 0;
	for (v = 0; v < patch_cp[1] - 1; v += 2)
	{
		for (u = 0; u < patch_cp[0] - 1; u += 2)
		{
			p = v * patch_cp[0] + u;

			i = Patch_FlatnessTest ( maxflat, points[p], points[p+1], points[p+2] );
			flat[0] = max ( flat[0], i );
			i = Patch_FlatnessTest ( maxflat, points[p+patch_cp[0]], points[p+patch_cp[0]+1], points[p+patch_cp[0]+2] );
			flat[0] = max ( flat[0], i );
			i = Patch_FlatnessTest ( maxflat, points[p+2*patch_cp[0]], points[p+2*patch_cp[0]+1], points[p+2*patch_cp[0]+2] );
			flat[0] = max ( flat[0], i );

			i = Patch_FlatnessTest ( maxflat, points[p], points[p+patch_cp[0]], points[p+2*patch_cp[0]] );
			flat[1] = max ( flat[1], i );
			i = Patch_FlatnessTest ( maxflat, points[p+1], points[p+patch_cp[0]+1], points[p+2*patch_cp[0]+1] );
			flat[1] = max ( flat[1], i );
			i = Patch_FlatnessTest ( maxflat, points[p+2], points[p+patch_cp[0]+2], points[p+2*patch_cp[0]+2] );
			flat[1] = max ( flat[1], i );
		}
	}
}

/*
===============
Patch_Evaluate_QuadricBezier
===============
*/
static void Patch_Evaluate_QuadricBezier ( float t, vec4_t point0, vec4_t point1, vec3_t point2, vec4_t out )
{
	float qt = t * t;
	float dt = 2.0f * t, tt;
	vec4_t tvec4;

	tt = 1.0f - dt + qt;
	Vector4Scale ( point0, tt, out );

	tt = dt - 2.0f * qt;
	Vector4Scale ( point1, tt, tvec4 );
	Vector4Add ( out, tvec4, out );

	Vector4Scale ( point2, qt, tvec4 );
	Vector4Add ( out, tvec4, out );
}

/*
===============
Patch_Evaluate
===============
*/
void Patch_Evaluate ( const vec4_t *p, const int *numcp, const int *tess, vec4_t *dest )
{
	int num_patches[2], num_tess[2];
	int index[3], dstpitch, i, u, v, x, y;
	float s, t, step[2];
	vec4_t *tvec, pv[3][3], v1, v2, v3;

	num_patches[0] = numcp[0] / 2;
	num_patches[1] = numcp[1] / 2;
	dstpitch = num_patches[0] * tess[0] + 1;

	step[0] = 1.0f / (float)tess[0];
	step[1] = 1.0f / (float)tess[1];

	for ( v = 0; v < num_patches[1]; v++ )
	{
		// last patch has one more row
		if ( v < num_patches[1] - 1 ) {
			num_tess[1] = tess[1];
		} else {
			num_tess[1] = tess[1] + 1;
		}

		for ( u = 0; u < num_patches[0]; u++ )
		{
			// last patch has one more column
			if ( u < num_patches[0] - 1 ) {
				num_tess[0] = tess[0];
			} else {
				num_tess[0] = tess[0] + 1;
			}

			index[0] = (v * numcp[0] + u) * 2;
			index[1] = index[0] + numcp[0];
			index[2] = index[1] + numcp[0];

			// current 3x3 patch control points
			for ( i = 0; i < 3; i++ )
			{
				Vector4Copy ( p[index[0]+i], pv[i][0] );
				Vector4Copy ( p[index[1]+i], pv[i][1] );
				Vector4Copy ( p[index[2]+i], pv[i][2] );
			}

			t = 0.0f;
			tvec = dest + v * tess[1] * dstpitch + u * tess[0];

			for ( y = 0; y < num_tess[1]; y++, t += step[1] )
			{
				Patch_Evaluate_QuadricBezier ( t, pv[0][0], pv[0][1], pv[0][2], v1 );
				Patch_Evaluate_QuadricBezier ( t, pv[1][0], pv[1][1], pv[1][2], v2 );
				Patch_Evaluate_QuadricBezier ( t, pv[2][0], pv[2][1], pv[2][2], v3 );

				s = 0.0f;
				for ( x = 0; x < num_tess[0]; x++, s += step[0] )
				{
					Patch_Evaluate_QuadricBezier ( s, v1, v2, v3, tvec[x] );
				}

				tvec += dstpitch;
			}
		}
	}
}




/*
===============================================================================

					PATCH LOADING

===============================================================================
*/

#define cm_subdivlevel	15

qboolean CM_CreateBrush ( q2cbrush_t *brush, vec3_t *verts, q2mapsurface_t *surface )
{
	int	i, j, k, sign;
	vec3_t v1, v2;
	vec3_t	absmins, absmaxs;
	q2cbrushside_t	*side;
	mplane_t *plane;
	static mplane_t mainplane, patchplanes[20];
	qboolean skip[20];
	int	numpatchplanes = 0;

	int matchplane;

	// calc absmins & absmaxs
	ClearBounds ( absmins, absmaxs );
	for (i = 0; i < 3; i++)
		AddPointToBounds ( verts[i], absmins, absmaxs );

	PlaneFromPoints ( verts, &mainplane );

	// front plane
	plane = &patchplanes[numpatchplanes++];
	*plane = mainplane;

	// back plane
	plane = &patchplanes[numpatchplanes++];
	VectorNegate (mainplane.normal, plane->normal);
	plane->dist = -mainplane.dist;

	// axial planes
	for ( i = 0; i < 3; i++ ) {
		for (sign = -1; sign <= 1; sign += 2) {
			plane = &patchplanes[numpatchplanes++];
			VectorClear ( plane->normal );
			plane->normal[i] = sign;
			plane->dist = sign > 0 ? absmaxs[i] : -absmins[i];
		}
	}

	// edge planes
	for ( i = 0; i < 3; i++ ) {
		vec3_t	normal;

		VectorCopy (verts[i], v1);
		VectorCopy (verts[(i + 1) % 3], v2);

		for ( k = 0; k < 3; k++ ) {
			normal[k] = 0;
			normal[(k+1)%3] = v1[(k+2)%3] - v2[(k+2)%3];
			normal[(k+2)%3] = -(v1[(k+1)%3] - v2[(k+1)%3]);

			if (VectorCompare (normal, vec3_origin))
				continue;

			plane = &patchplanes[numpatchplanes++];

			VectorNormalize ( normal );
			VectorCopy ( normal, plane->normal );
			plane->dist = DotProduct (plane->normal, v1);

			if ( DotProduct(verts[(i + 2) % 3], normal) - plane->dist > 0 )
			{	// invert
				VectorInverse ( plane->normal );
				plane->dist = -plane->dist;
			}
		}
	}

	// set plane->type and mark duplicate planes for removal
	for (i = 0; i < numpatchplanes; i++)
	{
		CategorizePlane ( &patchplanes[i] );
		skip[i] = false;

		for (j = i + 1; j < numpatchplanes; j++)
			if ( patchplanes[j].dist == patchplanes[i].dist
				&& VectorCompare (patchplanes[j].normal, patchplanes[i].normal) )
			{
				skip[i] = true;
				break;
			}
	}

	brush->numsides = 0;
	brush->firstbrushside = numbrushsides;

	for (k = 0; k < 2; k++) {
		for (i = 0; i < numpatchplanes; i++)	{
			if (skip[i])
				continue;

			// first, store all axially aligned planes
			// then store everything else
			// does it give a noticeable speedup?
			if (!k && patchplanes[i].type >= 3)
				continue;

			skip[i] = true;

			for (matchplane = 0; matchplane < numplanes; matchplane++)
			{
				if (!memcmp(&map_planes[matchplane], &patchplanes[i], sizeof(patchplanes[i])))
				{
					plane = &map_planes[matchplane];
					break;
				}
			}
			if (matchplane == numplanes)
			{
				if (numplanes == MAX_Q2MAP_PLANES)
				{
					Con_Printf (S_ERROR "CM_CreateBrush: numplanes == MAX_CM_PLANES");
					return false;
				}

				plane = &map_planes[numplanes++];
				*plane = patchplanes[i];
			}

			if (numbrushsides == MAX_CM_BRUSHSIDES)
			{
				Con_Printf (S_ERROR "CM_CreateBrush: numbrushsides == MAX_CM_BRUSHSIDES\n");
				return false;
			}

			side = &map_brushsides[numbrushsides++];
			side->plane = plane;

			if (DotProduct(plane->normal, mainplane.normal) >= 0)
				side->surface = surface;
			else
				side->surface = NULL;	// don't clip against this side

			brush->numsides++;
		}
	}

	return true;
}

qboolean CM_CreatePatch ( q3cpatch_t *patch, int numverts, const vec3_t *verts, int *patch_cp )
{
    int step[2], size[2], flat[2], i, u, v;
	vec4_t points[MAX_CM_PATCH_VERTS], pointss[MAX_CM_PATCH_VERTS];
	vec3_t tverts[4], tverts2[4];
	q2cbrush_t *brush;
	mplane_t mainplane;

// find the degree of subdivision in the u and v directions
	Patch_GetFlatness ( cm_subdivlevel, verts, patch_cp, flat );

	step[0] = (1 << flat[0]);
	step[1] = (1 << flat[1]);
	size[0] = (patch_cp[0] / 2) * step[0] + 1;
	size[1] = (patch_cp[1] / 2) * step[1] + 1;

	if ( size[0] * size[1] > MAX_CM_PATCH_VERTS ) 
	{
		Con_Printf (S_ERROR "CM_CreatePatch: patch has too many vertices\n");
		return false;
	}

	for (i = 0; i < numverts; i++)
		VectorCopy(verts[i], pointss[i]);
// fill in
//gcc warns without this cast
	Patch_Evaluate ( (const vec4_t *)pointss, patch_cp, step, points );
/*
	for (i = 0; i < numverts; i++)
	{
		points[i][0] = (int)(points[i][0]*20)/20.0f;
		points[i][1] = (int)(points[i][1]*20)/20.0f;
		points[i][2] = (int)(points[i][2]*20)/20.0f;
	}
*/
	patch->brushes = brush = map_brushes + numbrushes;
	patch->numbrushes = 0;

	ClearBounds (patch->absmins, patch->absmaxs);

// create a set of brushes
    for (v = 0; v < size[1]-1; v++)
    {
		for (u = 0; u < size[0]-1; u++)
		{
			if (numbrushes >= MAX_CM_BRUSHES)
			{
				Con_Printf (S_ERROR "CM_CreatePatch: too many patch brushes\n");
				return false;
			}

			i = v * size[0] + u;
			VectorCopy (points[i], tverts[0]);
			VectorCopy (points[i + size[0]], tverts[1]);
			VectorCopy (points[i + 1], tverts[2]);
			VectorCopy (points[i + size[0] + 1], tverts[3]);

			for (i = 0; i < 4; i++)
				AddPointToBounds (tverts[i], patch->absmins, patch->absmaxs);

			PlaneFromPoints (tverts, &mainplane);

			// create two brushes
			if (!CM_CreateBrush (brush, tverts, patch->surface))
				return false;

			brush->contents = patch->surface->c.value;
			brush++; numbrushes++; patch->numbrushes++;

			VectorCopy (tverts[2], tverts2[0]);
			VectorCopy (tverts[1], tverts2[1]);
			VectorCopy (tverts[3], tverts2[2]);
			if (!CM_CreateBrush (brush, tverts2, patch->surface))
				return false;

			brush->contents = patch->surface->c.value;
			brush++; numbrushes++; patch->numbrushes++;
		}
    }

	return true;
}


//======================================================

/*
=================
CM_CreatePatchesForLeafs
=================
*/
qboolean CM_CreatePatchesForLeafs (void)
{
	int i, j, k;
	mleaf_t *leaf;
	q3cface_t *face;
	q2mapsurface_t *surf;
	q3cpatch_t *patch;
	int checkout[MAX_CM_FACES];

	memset (checkout, -1, sizeof(int)*MAX_CM_FACES);

	for (i = 0, leaf = map_leafs; i < numleafs; i++, leaf++)
	{
		leaf->numleafpatches = 0;
		leaf->firstleafpatch = numleafpatches;

		if (leaf->cluster == -1 || map_noCurves.value)
			continue;

		for (j=0 ; j<leaf->numleaffaces ; j++)
		{
			k = leaf->firstleafface + j;
			if (k >= numleaffaces) {
				break;
			}

			k = map_leaffaces[k];
			face = &map_faces[k];

			if (face->facetype != MST_PATCH || face->numverts <= 0)
				continue;
			if (face->patch_cp[0] <= 0 || face->patch_cp[1] <= 0)
				continue;
			if (face->shadernum < 0 || face->shadernum >= numtexinfo)
				continue;

			surf = &map_surfaces[face->shadernum];
			if ( !surf->c.value || (surf->c.flags & Q3SURF_NONSOLID) )
				continue;

			if ( numleafpatches >= MAX_CM_LEAFFACES )
			{
				Con_Printf (S_ERROR "CM_CreatePatchesForLeafs: map has too many faces\n");
				return false;
			}

			// the patch was already built
			if (checkout[k] != -1)
			{
				map_leafpatches[numleafpatches] = checkout[k];
				patch = &map_patches[checkout[k]];
			}
			else
			{
				if (numpatches >= MAX_CM_PATCHES)
				{
					Con_Printf (S_ERROR "CM_CreatePatchesForLeafs: map has too many patches\n");
					return false;
				}

				patch = &map_patches[numpatches];
				patch->surface = surf;
				map_leafpatches[numleafpatches] = numpatches;
				checkout[k] = numpatches++;

//gcc warns without this cast
				if (!CM_CreatePatch ( patch, face->numverts, (const vec3_t *)map_verts + face->firstvert, face->patch_cp ))
					return false;
			}

			leaf->contents |= patch->surface->c.value;
			leaf->numleafpatches++;

			numleafpatches++;
		}
	}

	return true;
}



/*
===============================================================================

					MAP LOADING

===============================================================================
*/

qbyte	*cmod_base;

/*
=================
CMod_LoadSubmodels
=================
*/
qboolean CMod_LoadSubmodels (lump_t *l)
{
	q2dmodel_t	*in;
	q2cmodel_t	*out;
	int			i, j, count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Con_Printf (S_ERROR "Map with no models\n");
		return false;
	}
	if (count > MAX_Q2MAP_MODELS)
	{
		Con_Printf (S_ERROR "Map has too many models\n");
		return false;
	}

	numcmodels = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out = &map_cmodels[i];

		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			out->origin[j] = LittleFloat (in->origin[j]);
		}
		out->headnode = LittleLong (in->headnode);
		out->firstsurface = LittleLong (in->firstface);
		out->numsurfaces = LittleLong (in->numfaces);
	}

	return true;
}


/*
=================
CMod_LoadSurfaces
=================
*/
qboolean CMod_LoadSurfaces (lump_t *l)
{
	q2texinfo_t	*in;
	q2mapsurface_t	*out;
	int			i, count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);
	if (count < 1)
	{
		Con_Printf (S_ERROR "Map with no surfaces\n");
		return false;
	}
//	if (count > MAX_Q2MAP_TEXINFO)
//		Host_Error ("Map has too many surfaces");

	numtexinfo = count;
	out = map_surfaces = Hunk_Alloc(count * sizeof(*map_surfaces));

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		Q_strncpyz (out->c.name, in->texture, sizeof(out->c.name));
		Q_strncpyz (out->rname, in->texture, sizeof(out->rname));
		out->c.flags = LittleLong (in->flags);
		out->c.value = LittleLong (in->value);
	}

	return true;
}
#ifndef SERVERONLY
qbyte *ReadPCXFile(qbyte *buf, int length, int *width, int *height);
qbyte *ReadTargaFile(qbyte *buf, int length, int *width, int *height, int asgrey);


qbyte *ReadTargaFile(qbyte *buf, int length, int *width, int *height, int asgrey);
qbyte *ReadPCXFile(qbyte *buf, int length, int *width, int *height);
void *Mod_LoadWall(char *name)
{
	qbyte *in, *oin;
	texture_t *tex;
	q2miptex_t *wal;
	int width, height;
	char ln[32];

	COM_FileBase(name, ln, sizeof(ln));

	wal = (void *)COM_LoadMallocFile (name);
	if (!wal)
		return NULL;

	wal->width = LittleLong(wal->width);
	wal->height = LittleLong(wal->height);
	{
		int i;

		for (i = 0; i < MIPLEVELS; i++)
			wal->offsets[i] = LittleLong(wal->offsets[i]);
	}

	wal->flags = LittleLong(wal->flags);
	wal->contents = LittleLong(wal->contents);
	wal->value = LittleLong(wal->value);

//FIXME: Is this needed?
	oin = in = ReadPCXFile((qbyte *)wal, com_filesize, &width, &height);
	if (!in)
		oin = in = ReadTargaFile((qbyte *)wal, com_filesize, &width, &height, false);
	if (in)	//this is a pcx.
	{
#ifdef RGLQUAKE
		if (qrenderer == QR_OPENGL)
		{
			tex = Hunk_AllocName(sizeof(texture_t), ln);

			tex->offsets[0] = sizeof(*tex);
			tex->width = width;
			tex->height = height;

			texture_mode = GL_LINEAR_MIPMAP_NEAREST; //_LINEAR;
			if (!(tex->gl_texturenum = Mod_LoadReplacementTexture(name, loadname, true, false, true)))
				if (!(tex->gl_texturenum = Mod_LoadReplacementTexture(name, "bmodels", true, false, true)))
					tex->gl_texturenum = GL_LoadTexture32 (name, width, height, (unsigned int *)in, true, false);
			texture_mode = GL_LINEAR;
		}
		else
#endif
#ifdef SWQUAKE
		if (qrenderer == QR_SOFTWARE)
		{
			int i, j;
			qbyte *out;

			tex = Hunk_AllocName(sizeof(texture_t) + width*r_pixbytes*height/64*85, ln);
			tex->pixbytes = r_pixbytes;
			tex->width = width;
			tex->height = height;

			tex->offsets[0] = sizeof(*tex);
			tex->offsets[1] = tex->offsets[0] + width*height*r_pixbytes;
			tex->offsets[2] = tex->offsets[1] + (width*height*r_pixbytes)/4;
			tex->offsets[3] = tex->offsets[2] + (width*height*r_pixbytes)/(4*4);

			out = (qbyte *)(tex+1);

			if (tex->pixbytes == 4)
			{
				for (i = 0; i < width*height; i++)
				{
					*out++ = in[2];
					*out++ = in[1];
					*out++ = in[0];
					*out++ = in[3];
					in+=4;
				}
			}
			else
			{
				for (i=0; i < tex->width*tex->height; i++)	//downgrade colour
				{
					*out++ = GetPaletteNoFB(in[0], in[1], in[2]);
					in+=4;
				}

				in = (qbyte *)tex+tex->offsets[0];	//shrink mips.

				for (j = 0; j < tex->height; j+=2)	//we could convert mip[1], but shrinking is probably faster.
				for (i = 0; i < tex->width; i+=2)
					*out++ = in[i + tex->width*j];

				for (j = 0; j < tex->height; j+=4)
				for (i = 0; i < tex->width; i+=4)
					*out++ = in[i + tex->width*j];

				for (j = 0; j < tex->height; j+=8)
				for (i = 0; i < tex->width; i+=8)
					*out++ = in[i + tex->width*j];
			}
		}
		else
#endif
		{
			Sys_Error("Mod_LoadWall with bad renderer\n");
			tex = NULL;
		}

		BZ_Free(oin);
		BZ_Free(wal);

		return tex;
	}

#if defined(RGLQUAKE)
	if (qrenderer == QR_OPENGL)
	{
		int j;
		tex = Hunk_AllocName(sizeof(texture_t), ln);

		tex->offsets[0] = wal->offsets[0];
		tex->width = wal->width;
		tex->height = wal->height;

		texture_mode = GL_LINEAR_MIPMAP_NEAREST; //_LINEAR;
		if (!(tex->gl_texturenum = Mod_LoadReplacementTexture(wal->name, loadname, true, false, true)))
			if (!(tex->gl_texturenum = Mod_LoadReplacementTexture(wal->name, "bmodels", true, false, true)))
				tex->gl_texturenum = GL_LoadTexture8Pal24 (wal->name, tex->width, tex->height, (qbyte *)wal+wal->offsets[0], d_q28to24table, true, false);

		in = Hunk_TempAllocMore(wal->width*wal->height);
		oin = (qbyte *)wal+wal->offsets[0];
		for (j = 0; j < wal->width*wal->height; j++)
			in[j] = (d_q28to24table[oin[j]*3+0] + d_q28to24table[oin[j]*3+1] + d_q28to24table[oin[j]*3+2])/3;
		tex->gl_texturenumbumpmap = GL_LoadTexture8Bump (va("%s_bump", wal->name), tex->width, tex->height, in, true, r_shadow_bumpscale_basetexture.value);
		texture_mode = GL_LINEAR;
	}
	else
#endif
#if defined(SWQUAKE)
	if (qrenderer == QR_SOFTWARE)
	{
		int i, j;
		qbyte *out;

		tex = Hunk_AllocName(sizeof(texture_t) + wal->width*r_pixbytes*wal->height/64*85, ln);
		tex->width = wal->width;
		tex->height = wal->height;

		tex->pixbytes = r_pixbytes;
		for (i = 0; i < MIPLEVELS; i++)
			tex->offsets[i] = (wal->offsets[i] - sizeof(*wal))*tex->pixbytes + sizeof(*tex);

		out = (qbyte *)(tex+1);
		in = (qbyte *)wal+wal->offsets[0];

		if (tex->pixbytes == 4)
		{
			for (i = 0; i < wal->width*wal->height/64*85; i++)
			{
				*out++ = d_q28to24table[*in*3+0];
				*out++ = d_q28to24table[*in*3+1];
				*out++ = d_q28to24table[*in*3+2];
				*out++ = 255;
				in++;
			}
		}
		else
		{
			for (i=0; i < tex->width*tex->height; i++)	//downgrade colour
			{
				*out++ = GetPaletteNoFB(d_q28to24table[*in*3+0], d_q28to24table[*in*3+1], d_q28to24table[*in*3+2]);
				in++;
			}
			in = (qbyte *)tex+tex->offsets[0];	//shrink mips.

			for (j = 0; j < tex->height; j+=2)	//we could convert mip[1], but shrinking is probably faster.
			for (i = 0; i < tex->width; i+=2)
				*out++ = in[i + tex->width*j];

			for (j = 0; j < tex->height; j+=4)
			for (i = 0; i < tex->width; i+=4)
				*out++ = in[i + tex->width*j];

			for (j = 0; j < tex->height; j+=8)
			for (i = 0; i < tex->width; i+=8)
				*out++ = in[i + tex->width*j];
		}
	}
	else
#endif
	{
		Sys_Error("Mod_LoadWall with bad renderer\n");
		tex = NULL;
	}

	BZ_Free(wal);

	return tex;
}

qboolean CMod_LoadTexInfo (lump_t *l)	//yes I know these load from the same place
{
	extern cvar_t gl_shadeq2;
	q2texinfo_t *in;
	mtexinfo_t *out;
	int 	i, j, count;
	char	name[MAX_QPATH], *lwr;
	float	len1, len2;
	int texcount;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf ("MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( count*sizeof(*out), loadname);

	loadmodel->textures = Hunk_AllocName(sizeof(texture_t *)*count, loadname);
	texcount = 0;

	loadmodel->texinfo = out;
	loadmodel->numtexinfo = count;

#if !defined(SERVERONLY) && defined(RGLQUAKE)
	skytexturenum = -1;
#endif

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->flags = LittleLong (in->flags);

		for (j=0 ; j<8 ; j++)
			out->vecs[0][j] = LittleFloat (in->vecs[0][j]);
		len1 = Length (out->vecs[0]);
		len2 = Length (out->vecs[1]);
		len1 = (len1 + len2)/2;
		if (len1 < 0.32)
			out->mipadjust = 4;
		else if (len1 < 0.49)
			out->mipadjust = 3;
		else if (len1 < 0.99)
			out->mipadjust = 2;
		else
			out->mipadjust = 1;

		//damn q2...
		for (j=0; j < texcount; j++)
		{
			if (!strcmp(in->texture, loadmodel->textures[j]->name))
			{
				out->texture = loadmodel->textures[j];
				break;
			}
		}
		if (j == texcount)	//load a new one
		{
			for (lwr = in->texture; *lwr; lwr++)
			{
				if (*lwr >= 'A' && *lwr <= 'Z')
					*lwr = *lwr - 'A' + 'a';
			}
			snprintf (name, sizeof(name), "textures/%s.wal", in->texture);

			out->texture = Mod_LoadWall (name);
			if (!out->texture || !out->texture->width || !out->texture->height)
			{
				out->texture = Hunk_Alloc(sizeof(texture_t) + 16*16+8*8+4*4+2*2);

				Con_Printf (S_WARNING "Couldn't load %s\n", name);
				memcpy(out->texture, r_notexture_mip, sizeof(texture_t) + 16*16+8*8+4*4+2*2);
	//			out->texture = r_notexture_mip; // texture not found
	//			out->flags = 0;
			}

#ifdef RGLQUAKE
			if (qrenderer == QR_OPENGL)
				if (gl_shadeq2.value)
					out->texture->shader = R_RegisterCustom (name, NULL);
#endif
			Q_strncpyz(out->texture->name, in->texture, sizeof(out->texture->name));

#if !defined(SERVERONLY) && defined(RGLQUAKE)
			if (out->flags & SURF_SKY)
				skytexturenum = texcount;
#endif
			loadmodel->textures[texcount++] = out->texture;
		}
#if !defined(SERVERONLY) && defined(RGLQUAKE)
		else if (out->flags & SURF_SKY && skytexturenum>=0)
			out->texture = loadmodel->textures[skytexturenum];
#endif
	}

	loadmodel->numtextures = texcount;

	// count animation frames
	/*
	for (i=0 ; i<count ; i++)
	{
		out = &loadmodel->texinfo[i];
//		out->numframes = 1;
//		for (step = out->next ; step && step != out ; step=step->next)
//			out->numframes++;
	}
	*/

	return true;
}
#endif
/*
void CalcSurfaceExtents (msurface_t *s)
{
	float	mins[2], maxs[2], val;
	int		i,j, e;
	mvertex_t	*v;
	mtexinfo_t	*tex;
	int		bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;

	for (i=0 ; i<s->numedges ; i++)
	{
		e = loadmodel->surfedges[s->firstedge+i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];

		for (j=0 ; j<2 ; j++)
		{
			val = v->position[0] * tex->vecs[j][0] +
				v->position[1] * tex->vecs[j][1] +
				v->position[2] * tex->vecs[j][2] +
				tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i=0 ; i<2 ; i++)
	{
		bmins[i] = floor(mins[i]/16);
		bmaxs[i] = ceil(maxs[i]/16);

		s->texturemins[i] = bmins[i] * 16;
		s->extents[i] = (bmaxs[i] - bmins[i]) * 16;

//		if ( !(tex->flags & TEX_SPECIAL) && s->extents[i] > 512 )// 256 )
//			Sys_Error ("Bad surface extents");
	}
}*/


/*
=================
Mod_LoadFaces
=================
*/
#ifndef SERVERONLY
qboolean CMod_LoadFaces (lump_t *l)
{
	dface_t		*in;
	msurface_t 	*out;
	int			i, count, surfnum;
	int			planenum, side;
	int			ti;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf ("MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( (count+6)*sizeof(*out), loadname);	//spare for skybox

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	for ( surfnum=0 ; surfnum<count ; surfnum++, in++, out++)
	{
		out->firstedge = LittleLong(in->firstedge);
		out->numedges = LittleShort(in->numedges);
		out->flags = 0;

		planenum = LittleShort(in->planenum);
		side = LittleShort(in->side);
		if (side)
			out->flags |= SURF_PLANEBACK;

		out->plane = loadmodel->planes + planenum;

		ti = LittleShort (in->texinfo);
		if (ti < 0 || ti >= loadmodel->numtexinfo)
		{
			Con_Printf (S_ERROR "MOD_LoadBmodel: bad texinfo number\n");
			return false;
		}
		out->texinfo = loadmodel->texinfo + ti;

#ifndef SERVERONLY
		if (out->texinfo->flags & SURF_SKY)
		{
			out->flags |= SURF_DRAWSKY;
		}
		if (out->texinfo->flags & SURF_WARP)
		{
			out->flags |= SURF_DRAWTURB|SURF_DRAWTILED;
		}
#endif

		CalcSurfaceExtents (out);

	// lighting info

		for (i=0 ; i<MAXLIGHTMAPS ; i++)
			out->styles[i] = in->styles[i];
		i = LittleLong(in->lightofs);
		if (i == -1)
			out->samples = NULL;
#ifdef RGLQUAKE
		else if (qrenderer == QR_OPENGL)
			out->samples = loadmodel->lightdata + i;
#endif
#ifdef SWQUAKE
		else if (r_usinglits)
			out->samples = loadmodel->lightdata + i;
#endif
		else
			out->samples = loadmodel->lightdata + i/3;

	// set the drawing flags

		if (out->texinfo->flags & SURF_WARP)
		{
			out->flags |= SURF_DRAWTURB;
			for (i=0 ; i<2 ; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
		}

	}

	return true;
}
#endif

void CMod_SetParent (mnode_t *node, mnode_t *parent)
{
	node->parent = parent;
	if (node->contents != -1)
		return;
	CMod_SetParent (node->children[0], node);
	CMod_SetParent (node->children[1], node);
}

/*
=================
CMod_LoadNodes

=================
*/
qboolean CMod_LoadNodes (lump_t *l)
{
	q2dnode_t		*in;
	int			child;
	mnode_t		*out;
	int			i, j, count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Con_Printf (S_ERROR "Map has no nodes\n");
		return false;
	}
	if (count > MAX_MAP_NODES)
	{
		Con_Printf (S_ERROR "Map has too many nodes\n");
		return false;
	}

	out = Hunk_Alloc(sizeof(mnode_t)*count);

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		memset(out, 0, sizeof(*out));

		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		out->plane = map_planes + LittleLong(in->planenum);

		out->firstsurface = LittleShort (in->firstface);
		out->numsurfaces = LittleShort (in->numfaces);
		out->contents = -1;	// differentiate from leafs

		for (j=0 ; j<2 ; j++)
		{
			child = LittleLong (in->children[j]);
			out->childnum[j] = child;
			if (child < 0)
				out->children[j] = (mnode_t *)(map_leafs + -1-child);
			else
				out->children[j] = loadmodel->nodes + child;
		}
	}

	CMod_SetParent (loadmodel->nodes, NULL);	// sets nodes and leafs

	return true;
}

/*
=================
CMod_LoadBrushes

=================
*/
qboolean CMod_LoadBrushes (lump_t *l)
{
	q2dbrush_t	*in;
	q2cbrush_t	*out;
	int			i, count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count > MAX_Q2MAP_BRUSHES)
	{
		Con_Printf (S_ERROR "Map has too many brushes");
		return false;
	}

	out = map_brushes;

	numbrushes = count;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->firstbrushside = LittleLong(in->firstside);
		out->numsides = LittleLong(in->numsides);
		out->contents = LittleLong(in->contents);
	}

	return true;
}

/*
=================
CMod_LoadLeafs
=================
*/
qboolean CMod_LoadLeafs (lump_t *l)
{
	int			i, j;
	mleaf_t		*out;
	q2dleaf_t 	*in;
	int			count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Con_Printf (S_ERROR "Map with no leafs\n");
		return false;
	}
	// need to save space for box planes
	if (count > MAX_Q2MAP_PLANES)
	{
		Con_Printf (S_ERROR "Map has too many planes\n");
		return false;
	}

	out = map_leafs;
	numleafs = count;
	numclusters = 0;

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		memset(out, 0, sizeof(*out));

		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleShort (in->mins[j]);
			out->minmaxs[3+j] = LittleShort (in->maxs[j]);
		}

		out->contents = LittleLong (in->contents);
		out->cluster = LittleShort (in->cluster);
		out->area = LittleShort (in->area);
		out->firstleafbrush = LittleShort (in->firstleafbrush);
		out->numleafbrushes = LittleShort (in->numleafbrushes);

		out->firstmarksurface = loadmodel->marksurfaces +
			LittleShort(in->firstleafface);
		out->nummarksurfaces = LittleShort(in->numleaffaces);

		if (out->cluster >= numclusters)
			numclusters = out->cluster + 1;
	}

	if (map_leafs[0].contents != Q2CONTENTS_SOLID)
	{
		Con_Printf (S_ERROR "Map leaf 0 is not CONTENTS_SOLID\n");
		return false;
	}

	emptyleaf = -1;
	for (i=1 ; i<numleafs ; i++)
	{
		if (!map_leafs[i].contents)
		{
			emptyleaf = i;
			break;
		}
	}
	if (emptyleaf == -1)
	{
		Con_Printf (S_ERROR "Map does not have an empty leaf\n");
		return false;
	}
	return true;
}

/*
=================
CMod_LoadPlanes
=================
*/
qboolean CMod_LoadPlanes (lump_t *l)
{
	int			i, j;
	mplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Con_Printf (S_ERROR "Map with no planes\n");
		return false;
	}
	// need to save space for box planes
	if (count >= MAX_Q2MAP_PLANES)
	{
		Con_Printf (S_ERROR "Map has too many planes\n");
		return false;
	}

	out = map_planes;
	numplanes = count;


	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		bits = 0;
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = LittleFloat (in->dist);
		out->type = LittleLong (in->type);
		out->signbits = bits;
	}

	return true;
}

/*
=================
CMod_LoadLeafBrushes
=================
*/
qboolean CMod_LoadLeafBrushes (lump_t *l)
{
	int			i;
	int	*out;
	unsigned short 	*in;
	int			count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Con_Printf (S_ERROR "Map with no planes\n");
		return false;
	}
	// need to save space for box planes
	if (count > MAX_Q2MAP_LEAFBRUSHES)
	{
		Con_Printf (S_ERROR "Map has too many leafbrushes\n");
		return false;
	}

	out = map_leafbrushes;
	numleafbrushes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
		*out = LittleShort (*in);

	return true;
}

/*
=================
CMod_LoadBrushSides
=================
*/
qboolean CMod_LoadBrushSides (lump_t *l)
{
	int			i, j;
	q2cbrushside_t	*out;
	q2dbrushside_t 	*in;
	int			count;
	int			num;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	// need to save space for box planes
	if (count > MAX_Q2MAP_BRUSHSIDES)
	{
		Con_Printf (S_ERROR "Map has too many planes\n");
		return false;
	}

	out = map_brushsides;
	numbrushsides = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		num = LittleShort (in->planenum);
		out->plane = &map_planes[num];
		j = LittleShort (in->texinfo);
		if (j >= numtexinfo)
		{
			Con_Printf (S_ERROR "Bad brushside texinfo\n");
			return false;
		}
		out->surface = &map_surfaces[j];
	}

	return true;
}

/*
=================
CMod_LoadAreas
=================
*/
qboolean CMod_LoadAreas (lump_t *l)
{
	int			i;
	q2carea_t		*out;
	q2darea_t 	*in;
	int			count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count > MAX_Q2MAP_AREAS)
	{
		Con_Printf (S_ERROR "Map has too many areas\n");
		return false;
	}

	out = map_q2areas;
	numareas = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->numareaportals = LittleLong (in->numareaportals);
		out->firstareaportal = LittleLong (in->firstareaportal);
		out->floodvalid = 0;
		out->floodnum = 0;
	}

	return true;
}

/*
=================
CMod_LoadAreaPortals
=================
*/
qboolean CMod_LoadAreaPortals (lump_t *l)
{
	int			i;
	q2dareaportal_t		*out;
	q2dareaportal_t 	*in;
	int			count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count > MAX_Q2MAP_AREAS)
	{
		Con_Printf (S_ERROR "Map has too many areas\n");
		return false;
	}

	out = map_areaportals;
	numareaportals = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->portalnum = LittleLong (in->portalnum);
		out->otherarea = LittleLong (in->otherarea);
	}

	return true;
}

/*
=================
CMod_LoadVisibility
=================
*/
qboolean CMod_LoadVisibility (lump_t *l)
{
	int		i;

	numvisibility = l->filelen;
	if (l->filelen > MAX_Q2MAP_VISIBILITY)
	{
		Con_Printf (S_ERROR "Map has too large visibility lump\n");
		return false;
	}

	memcpy (map_visibility, cmod_base + l->fileofs, l->filelen);

	loadmodel->vis = map_q2vis;

	map_q2vis->numclusters = LittleLong (map_q2vis->numclusters);
	for (i=0 ; i<map_q2vis->numclusters ; i++)
	{
		map_q2vis->bitofs[i][0] = LittleLong (map_q2vis->bitofs[i][0]);
		map_q2vis->bitofs[i][1] = LittleLong (map_q2vis->bitofs[i][1]);
	}

	return true;
}

/*
=================
CMod_LoadEntityString
=================
*/
void CMod_LoadEntityString (lump_t *l)
{
	numentitychars = l->filelen;
//	if (l->filelen > MAX_Q2MAP_ENTSTRING)
//		Host_Error ("Map has too large entity lump");

	map_entitystring = Hunk_Alloc(l->filelen+1);
	memcpy (map_entitystring, cmod_base + l->fileofs, l->filelen);

	loadmodel->entities = map_entitystring;
}




qboolean CModQ3_LoadMarksurfaces (lump_t *l)
{
	int		i, j, count;
	int		*in;
	msurface_t **out;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "CModQ3_LoadMarksurfaces: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( count*sizeof(*out), loadname);

	loadmodel->marksurfaces = out;
	loadmodel->nummarksurfaces = count;

	for ( i=0 ; i<count ; i++)
	{
		j = LittleLong(in[i]);
		if (j < 0 || j >= loadmodel->numsurfaces)
		{
			Con_Printf (S_ERROR "Mod_ParseMarksurfaces: bad surface number\n");
			return false;
		}
		out[i] = loadmodel->surfaces + j;
	}

	return true;
}

qboolean CModQ3_LoadSubmodels (lump_t *l)
{
	q3dmodel_t	*in;
	q2cmodel_t	*out;
	int			i, j, count;
	int			*leafbrush;
	mleaf_t		*bleaf;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Con_Printf (S_ERROR "Map with no models\n");
		return false;
	}
	if (count > MAX_Q2MAP_MODELS)
	{
		Con_Printf (S_ERROR "Map has too many models\n");
		return false;
	}

	numcmodels = count;

	mapisq3 = true;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out = &map_cmodels[i];

		for (j=0 ; j<3 ; j++)
		{	// spread the mins / maxs by a pixel
			out->mins[j] = LittleFloat (in->mins[j]) - 1;
			out->maxs[j] = LittleFloat (in->maxs[j]) + 1;
			out->origin[j] = (out->maxs[j] + out->mins[j])/2;
		}
		if (!i)
			out->headnode = 0;
		else
		{
//create a new leaf to hold the bruses and be directly clipped
			out->headnode = -1 - numleafs;
			out->firstsurface = LittleLong (in->firstsurface);
			out->numsurfaces = LittleLong (in->num_surfaces);

//			out->firstbrush = LittleLong(in->firstbrush);
//			out->num_brushes = LittleLong(in->num_brushes);

			bleaf = &map_leafs[numleafs++];
			bleaf->numleafbrushes = LittleLong ( in->num_brushes );
			bleaf->firstleafbrush = numleafbrushes;
			bleaf->contents = 0;

			leafbrush = &map_leafbrushes[numleafbrushes];
			for ( j = 0; j < bleaf->numleafbrushes; j++, leafbrush++ ) {
				*leafbrush = LittleLong ( in->firstbrush ) + j;
				bleaf->contents |= map_brushes[*leafbrush].contents;
			}
			numleafbrushes += bleaf->numleafbrushes;
		}
		//submodels
	}

	VectorCopy(map_cmodels[0].mins, loadmodel->mins);
	VectorCopy(map_cmodels[0].maxs, loadmodel->maxs);

	return true;
}

qboolean CModQ3_LoadShaders (lump_t *l, qboolean useshaders)
{
	dq3shader_t	*in;
	q2mapsurface_t	*out;
	int				i, count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Con_Printf (S_ERROR "Map with no shaders\n");
		return false;
	}
//	else if (count > MAX_Q2MAP_TEXINFO)
//		Host_Error ("Map has too many shaders");

	numtexinfo = count;
	out = map_surfaces = Hunk_Alloc(count*sizeof(*out));

#if !defined(SERVERONLY) && defined(RGLQUAKE)
	skytexturenum = -1;
#endif

	loadmodel->texinfo = Hunk_Alloc(sizeof(mtexinfo_t)*count);
	loadmodel->numtextures = count;
	loadmodel->textures = Hunk_Alloc(sizeof(texture_t*)*count);

	for ( i=0 ; i<count ; i++, in++, out++ )
	{
		loadmodel->texinfo[i].texture = Hunk_Alloc(sizeof(texture_t));
		Q_strncpyz(loadmodel->texinfo[i].texture->name, in->shadername, sizeof(loadmodel->texinfo[i].texture->name));
#ifdef RGLQUAKE
		if (qrenderer == QR_OPENGL && !useshaders)
		{
			loadmodel->texinfo[i].texture->gl_texturenum = Mod_LoadHiResTexture(in->shadername, loadname, true, false, true);
			if (!loadmodel->texinfo[i].texture->gl_texturenum)
				loadmodel->texinfo[i].texture->gl_texturenum = Mod_LoadHiResTexture(in->shadername, "bmodels", true, false, true);
			loadmodel->texinfo[i].texture->gl_texturenumfb = 0;
			loadmodel->texinfo[i].texture->gl_texturenumbumpmap = 0;

			if (!strncmp(in->shadername, "textures/skies/", 15))
			{
				loadmodel->texinfo[i].flags |= SURF_SKY;
				skytexturenum = i;
			}
		}
#endif
		loadmodel->textures[i] = loadmodel->texinfo[i].texture;

		out->c.flags = LittleLong ( in->surfflags );
		out->c.value = LittleLong ( in->contents );
	}

	return true;
}

qboolean CModQ3_LoadVertexes (lump_t *l)
{
	q3dvertex_t	*in;
	vec3_t		*out;
	vec3_t		*nout;
	int			i, count, j;
	vec2_t		*lmout, *stout;
	byte_vec4_t *cout;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "CMOD_LoadVertexes: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count > MAX_Q3MAP_VERTEXES)
	{
		Con_Printf (S_ERROR "Map has too many vertexes\n");
		return false;
	}

	out = Hunk_Alloc ( count*sizeof(*out) );
	stout = Hunk_Alloc ( count*sizeof(*stout) );
	lmout = Hunk_Alloc ( count*sizeof(*lmout) );
	cout = Hunk_Alloc ( count*sizeof(*cout) );
	nout = Hunk_Alloc ( count*sizeof(*nout) );
	map_verts = out;
	map_vertstmexcoords = stout;
	map_vertlstmexcoords = lmout;
	map_colors_array = cout;
	map_normals_array = nout;
	numvertexes = count;

	for ( i=0 ; i<count ; i++, in++)
	{
		for ( j=0 ; j < 3 ; j++)
		{
			out[i][j] = LittleFloat ( in->point[j] );
			nout[i][j] = LittleFloat (in->normal[j]);
		}
		for ( j=0 ; j < 2 ; j++)
		{
			stout[i][j] = LittleFloat ( ((float *)in->texcoords)[j] );
			lmout[i][j] = LittleFloat ( ((float *)in->texcoords)[j+2] );
		}
		for ( j=0 ; j < 4 ; j++)
		{
			cout[i][j] = in->color[j];
		}
	}

	return true;
}

qboolean CModRBSP_LoadVertexes (lump_t *l)
{
	rbspvertex_t	*in;
	vec3_t		*out;
	vec3_t		*nout;
	int			i, count, j;
	vec2_t		*lmout, *stout;
	byte_vec4_t *cout;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "CMOD_LoadVertexes: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count > MAX_Q3MAP_VERTEXES)
	{
		Con_Printf (S_ERROR "Map has too many vertexes\n");
		return false;
	}

	out = Hunk_Alloc ( count*sizeof(*out) );
	stout = Hunk_Alloc ( count*sizeof(*stout) );
	lmout = Hunk_Alloc ( count*sizeof(*lmout) );
	cout = Hunk_Alloc ( count*sizeof(*cout) );
	nout = Hunk_Alloc ( count*sizeof(*nout) );
	map_verts = out;
	map_vertstmexcoords = stout;
	map_vertlstmexcoords = lmout;
	map_colors_array = cout;
	map_normals_array = nout;
	numvertexes = count;

	for ( i=0 ; i<count ; i++, in++)
	{
		for ( j=0 ; j < 3 ; j++)
		{
			out[i][j] = LittleFloat ( in->point[j] );
			nout[i][j] = LittleFloat (in->normal[j]);
		}
		for ( j=0 ; j < 2 ; j++)
		{
			stout[i][j] = LittleFloat ( ((float *)in->texcoords)[j] );
			lmout[i][j] = LittleFloat ( ((float *)in->texcoords)[j+2] );
		}
		for ( j=0 ; j < 4 ; j++)
		{
			cout[i][j] = in->color[0][j];
		}
	}

	return true;
}


qboolean CModQ3_LoadIndexes (lump_t *l)
{
	int		i, count;
	int		*in, *out;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	if (count < 1 || count >= MAX_Q3MAP_INDICES)
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: bad surfedges count in %s: %i\n",
		loadmodel->name, count);
		return false;
	}

	out = Hunk_AllocName ( count*sizeof(*out), loadmodel->name );

	map_surfindexes = out;
	map_numsurfindexes = count;

	for ( i=0 ; i<count ; i++)
		out[i] = LittleLong (in[i]);

	return true;
}

/*
=================
CMod_LoadFaces
=================
*/
qboolean CModQ3_LoadFaces (lump_t *l)
{
	q3dface_t		*in;
	q3cface_t		*out;
	int			i, count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count > MAX_MAP_FACES)
	{
		Con_Printf (S_ERROR "Map has too many faces\n");
		return false;
	}

	out = BZ_Malloc ( count*sizeof(*out) );
	map_faces = out;
	numfaces = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->facetype = LittleLong ( in->facetype );
		out->shadernum = LittleLong ( in->shadernum );

		out->numverts = LittleLong ( in->num_vertices );
		out->firstvert = LittleLong ( in->firstvertex );

		out->patch_cp[0] = LittleLong ( in->patchwidth );
		out->patch_cp[1] = LittleLong ( in->patchheight );
	}

	loadmodel->numsurfaces = i;\

	return true;
}

qboolean CModRBSP_LoadFaces (lump_t *l)
{
	rbspface_t		*in;
	q3cface_t		*out;
	int			i, count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count > MAX_MAP_FACES)
	{
		Con_Printf (S_ERROR "Map has too many faces\n");
		return false;
	}

	out = BZ_Malloc ( count*sizeof(*out) );
	map_faces = out;
	numfaces = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->facetype = LittleLong ( in->facetype );
		out->shadernum = LittleLong ( in->shadernum );

		out->numverts = LittleLong ( in->num_vertices );
		out->firstvert = LittleLong ( in->firstvertex );

		out->patch_cp[0] = LittleLong ( in->patchwidth );
		out->patch_cp[1] = LittleLong ( in->patchheight );
	}

	loadmodel->numsurfaces = i;
	return true;
}

#ifdef RGLQUAKE

/*
=================
Mod_LoadFogs
=================
*/
#ifdef Q3SHADERS
qboolean CModQ3_LoadFogs (lump_t *l)
{
	dfog_t 	*in;
	mfog_t 	*out;
	q2cbrush_t *brush;
	q2cbrushside_t *visibleside, *brushsides;
	int		i, j, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	out = Hunk_Alloc ( count*sizeof(*out) );

	map_fogs = out;
	map_numfogs = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		if ( LittleLong ( in->visibleSide ) == -1 )
		{
			continue;
		}

		brush = map_brushes + LittleLong ( in->brushNum );
		brushsides = map_brushsides + brush->firstbrushside;
		visibleside = brushsides + LittleLong ( in->visibleSide );

		out->visibleplane = visibleside->plane;
		out->shader = R_RegisterShader ( in->shader );
		out->numplanes = brush->numsides;
		out->planes = Hunk_Alloc ( out->numplanes*sizeof(cplane_t *) );

		for ( j = 0; j < out->numplanes; j++ )
		{
			out->planes[j] = brushsides[j].plane;
		}
	}

	if (count)
		GL_InitFogTexture();

	return true;
}

mfog_t *CM_FogForOrigin(vec3_t org)
{
	int i, j;
	mfog_t 	*ret = map_fogs;
	float dot;
	if (!cl.worldmodel || cl.worldmodel->fromgame != fg_quake3)
		return NULL;

	for ( i=0 ; i<map_numfogs ; i++, ret++)
	{
		if (!ret->numplanes)
			continue;
		for (j = 0; j < ret->numplanes; j++)
		{
			dot = DotProduct(ret->planes[j]->normal, org);
			if (dot - ret->planes[j]->dist > 0)
				break;
		}
		if (j == ret->numplanes)
		{
			return ret;
		}
	}

	return NULL;
}
#endif

//Convert a patch in to a list of glpolys

#define MAX_ARRAY_VERTS 2048


glpoly_t *GL_MeshToGLPoly(mesh_t *mesh)
{
	int polysize = sizeof(glpoly_t) - (VERTEXSIZE)*sizeof(float);
	int gv;
	int v;
	int rv;
	glpoly_t *p, *ret;

	int numindx;
	if (!mesh)
		return NULL;

	numindx = mesh->numindexes;
	ret = NULL;

	p = Hunk_Alloc(polysize * numindx/3);

	for (gv = 0; gv < numindx; )
	{
		for (v = gv; v < gv+3; v++)
		{
			rv = mesh->indexes[v];
			p->verts[v%3][0] = mesh->xyz_array[rv][0];
			p->verts[v%3][1] = mesh->xyz_array[rv][1];
			p->verts[v%3][2] = mesh->xyz_array[rv][2];
			p->verts[v%3][3] = mesh->st_array[rv][0];
			p->verts[v%3][4] = mesh->st_array[rv][1];
			p->verts[v%3][5] = mesh->lmst_array[rv][0];
			p->verts[v%3][6] = mesh->lmst_array[rv][1];
		}
		gv+=3;

		p->next = ret;
		p->numverts = 3;
		ret = p;
		p = (glpoly_t *)((char *)p + polysize);
	}

	return ret;
}

#define Vector2Copy(a,b) {(b)[0]=(a)[0];(b)[1]=(a)[1];}

index_t tempIndexesArray[MAX_ARRAY_VERTS*3];
vec4_t			tempxyz_array[MAX_ARRAY_VERTS];	//structure is used only at load.
vec3_t			tempnormals_array[MAX_ARRAY_VERTS];	//so what harm is there in doing this?
vec2_t			tempst_array[MAX_ARRAY_VERTS];
vec2_t			templmst_array[MAX_ARRAY_VERTS];
byte_vec4_t		tempcolors_array[MAX_ARRAY_VERTS];

#ifdef Q3SHADERS
#define Hunk_TempAllocMore Hunk_Alloc
#endif

//mesh_t *GL_CreateMeshForPatch ( model_t *mod, q3dface_t *surf )
mesh_t *GL_CreateMeshForPatch (model_t *mod, int patchwidth, int patchheight, int numverts, int firstvert)
{
    int numindexes, patch_cp[2], step[2], size[2], flat[2], i, u, v, p;
	vec4_t colors[MAX_ARRAY_VERTS], points[MAX_ARRAY_VERTS], normals[MAX_ARRAY_VERTS],
		lm_st[MAX_ARRAY_VERTS], tex_st[MAX_ARRAY_VERTS];
	vec4_t c, colors2[MAX_ARRAY_VERTS], points2[MAX_ARRAY_VERTS], normals2[MAX_ARRAY_VERTS], lm_st2[MAX_ARRAY_VERTS], tex_st2[MAX_ARRAY_VERTS];
	mesh_t *mesh;
	index_t	*indexes;
	float subdivlevel;

	patch_cp[0] = patchwidth;
	patch_cp[1] = patchheight;

	if ( !patch_cp[0] || !patch_cp[1] ) {
		return NULL;
	}

	subdivlevel = r_subdivisions.value;
	if ( subdivlevel < 1 )
		subdivlevel = 1;

	for ( i = 0; i < numverts; i++ ) {
		VectorCopy ( map_verts[firstvert + i], points[i] );
		VectorCopy ( map_normals_array[firstvert + i], normals[i] );
		Vector4Scale ( map_colors_array[firstvert + i], (1.0 / 255.0), colors[i] );
		Vector2Copy ( map_vertstmexcoords[firstvert + i], tex_st[i] );
		Vector2Copy ( map_vertlstmexcoords[firstvert + i], lm_st[i] );
	}

// find the degree of subdivision in the u and v directions
	Patch_GetFlatness ( subdivlevel, (const vec3_t *)map_verts+firstvert, patch_cp, flat );

// allocate space for mesh
	step[0] = (1 << flat[0]);
	step[1] = (1 << flat[1]);
	size[0] = (patch_cp[0] / 2) * step[0] + 1;
	size[1] = (patch_cp[1] / 2) * step[1] + 1;
	numverts = size[0] * size[1];

	if ( numverts > MAX_ARRAY_VERTS ) {
		return NULL;
	}

	mesh = (mesh_t *)Hunk_TempAllocMore ( sizeof(mesh_t));

	mesh->numvertexes = numverts;
	mesh->xyz_array = Hunk_TempAllocMore ( numverts * sizeof(vec3_t));
	mesh->normals_array = Hunk_TempAllocMore ( numverts * sizeof(vec3_t));
	mesh->st_array = Hunk_TempAllocMore ( numverts * sizeof(vec2_t));
	mesh->lmst_array = Hunk_TempAllocMore ( numverts * sizeof(vec2_t));
	mesh->colors_array = Hunk_TempAllocMore ( numverts * sizeof(byte_vec4_t));

	mesh->patchWidth = size[0];
	mesh->patchHeight = size[1];

// fill in
	Patch_Evaluate ( (const vec4_t *)points, patch_cp, step, points2 );
	Patch_Evaluate ( (const vec4_t *)colors, patch_cp, step, colors2 );
	Patch_Evaluate ( (const vec4_t *)normals, patch_cp, step, normals2 );
	Patch_Evaluate ( (const vec4_t *)lm_st, patch_cp, step, lm_st2 );
	Patch_Evaluate ( (const vec4_t *)tex_st, patch_cp, step, tex_st2 );

	for (i = 0; i < numverts; i++)
	{
		VectorCopy ( points2[i], mesh->xyz_array[i] );
		VectorNormalize2 ( normals2[i], mesh->normals_array[i] );
		ColorNormalize ( colors2[i], c );
		Vector4Scale ( c, 255.0, mesh->colors_array[i] );
		Vector2Copy ( tex_st2[i], mesh->st_array[i] );
		Vector2Copy ( lm_st2[i], mesh->lmst_array[i] );
    }

// compute new indexes avoiding adding invalid triangles
	numindexes = 0;
	indexes = tempIndexesArray;
    for (v = 0, i = 0; v < size[1]-1; v++)
	{
		for (u = 0; u < size[0]-1; u++, i += 6)
		{
			indexes[0] = p = v * size[0] + u;
			indexes[1] = p + size[0];
			indexes[2] = p + 1;

			if ( !VectorCompare(mesh->xyz_array[indexes[0]], mesh->xyz_array[indexes[1]]) &&
				!VectorCompare(mesh->xyz_array[indexes[0]], mesh->xyz_array[indexes[2]]) &&
				!VectorCompare(mesh->xyz_array[indexes[1]], mesh->xyz_array[indexes[2]]) ) {
				indexes += 3;
				numindexes += 3;
			}

			indexes[0] = p + 1;
			indexes[1] = p + size[0];
			indexes[2] = p + size[0] + 1;

			if ( !VectorCompare(mesh->xyz_array[indexes[0]], mesh->xyz_array[indexes[1]]) &&
				!VectorCompare(mesh->xyz_array[indexes[0]], mesh->xyz_array[indexes[2]]) &&
				!VectorCompare(mesh->xyz_array[indexes[1]], mesh->xyz_array[indexes[2]]) ) {
				indexes += 3;
				numindexes += 3;
			}
		}
	}

// allocate and fill index table
	mesh->numindexes = numindexes;

	mesh->indexes = (index_t *)Hunk_TempAllocMore ( numindexes * sizeof(index_t));
	memcpy (mesh->indexes, tempIndexesArray, numindexes * sizeof(index_t) );

	return mesh;
}

#ifdef Q3SHADERS
#undef Hunk_TempAllocMore
#endif


void CModQ3_SortShaders(void)
{
	texture_t *textemp;
	int i, j;
	//sort loadmodel->textures
	//correct pointers in loadmodel->texinfo

	for (i = 0; i < numtexinfo; i++)
	{
		for (j = i+1; j < numtexinfo; j++)
		{
			if ((loadmodel->textures[i]->shader && loadmodel->textures[j]->shader) && (loadmodel->textures[j]->shader->sort < loadmodel->textures[i]->shader->sort))
			{
				textemp = loadmodel->textures[j];
				loadmodel->textures[j] = loadmodel->textures[i];
				loadmodel->textures[i] = textemp;

				if (skytexturenum==i)
					skytexturenum=j;
				else if (skytexturenum==j)
					skytexturenum=i;
			}
		}
	}
}

mesh_t nullmesh;
qboolean CModQ3_LoadRFaces (lump_t *l, qboolean useshaders)
{
#ifndef Q3SHADERS
	int polysize = sizeof(glpoly_t) - VERTEXSIZE*sizeof(float);
	glpoly_t *p;
	int rv, fi;
	int gv, v;
#endif
	q3dface_t *in;
	msurface_t *out;
	mplane_t *pl;

	int count;
	int surfnum;

	int numverts, numindexes;
	int fv;

	mesh_t *mesh;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( count*sizeof(*out), loadmodel->name );
	pl = Hunk_AllocName (count*sizeof(*pl), loadmodel->name);//create a new array of planes for speed.

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	for (surfnum = 0; surfnum < count; surfnum++, out++, in++, pl++)
	{
		out->plane = pl;
		out->texinfo = loadmodel->texinfo + LittleLong(in->shadernum);
		out->lightmaptexturenum = LittleLong(in->lightmapnum);
		out->light_s = LittleLong(in->lightmap_x);
		out->light_t = LittleLong(in->lightmap_y);
		out->extents[0] = (LittleLong(in->lightmap_width)-1)<<4;
		out->extents[1] = (LittleLong(in->lightmap_height)-1)<<4;
		out->samples = loadmodel->lightdata + 3*(out->light_s + out->light_t*128 + out->lightmaptexturenum*128*128);
		if (out->lightmaptexturenum<0)
			out->samples=NULL;

		fv = LittleLong(in->firstvertex);
		{
			vec3_t v[3];
			VectorCopy(map_verts[fv+0], v[0]);
			VectorCopy(map_verts[fv+1], v[1]);
			VectorCopy(map_verts[fv+2], v[2]);
			PlaneFromPoints(v, pl);
			CategorizePlane(pl);
		}
		/*
		if (in->fognum!=-1)
		continue;
		*/

		if (map_surfaces[LittleLong(in->shadernum)].c.value == 0 || map_surfaces[LittleLong(in->shadernum)].c.value & Q3CONTENTS_TRANSLUCENT)
				//q3dm10's thingie is 0
			out->flags |= SURF_DRAWALPHA;

		if (loadmodel->texinfo[LittleLong(in->shadernum)].flags & SURF_SKY)
			out->flags |= SURF_DRAWSKY;

#ifdef Q3SHADERS
		if (!out->texinfo->texture->shader && useshaders)
		{
			extern cvar_t r_vertexlight;
			if (LittleLong(in->facetype) == MST_FLARE)
				out->texinfo->texture->shader = R_RegisterShader_Flare (out->texinfo->texture->name);
			else if (LittleLong(in->facetype) == MST_TRIANGLE_SOUP || r_vertexlight.value)
				out->texinfo->texture->shader = R_RegisterShader_Vertex (out->texinfo->texture->name);
			else
				out->texinfo->texture->shader = R_RegisterShader(out->texinfo->texture->name);


			if (out->texinfo->texture->shader->flags & SHADER_SKY)
			{
				out->texinfo->flags |= SURF_SKY;
				skytexturenum = out->texinfo - loadmodel->texinfo;
			}
		}

		if (LittleLong(in->fognum) == -1 || !map_numfogs)
			out->fog = NULL;
		else
			out->fog = map_fogs + LittleLong(in->fognum);
#endif
		if (map_surfaces[LittleLong(in->shadernum)].c.flags & (Q3SURF_NODRAW | Q3SURF_SKIP))
		{
			out->mesh = &nullmesh;
		}
		else if (LittleLong(in->facetype) == MST_PATCH)
		{
			out->mesh = GL_CreateMeshForPatch(loadmodel, LittleLong(in->patchwidth), LittleLong(in->patchheight), LittleLong(in->num_vertices), LittleLong(in->firstvertex));
		}
		else if (LittleLong(in->facetype) == MST_PLANAR || LittleLong(in->facetype) == MST_TRIANGLE_SOUP)
		{
			numindexes = LittleLong(in->num_indexes);
			numverts = LittleLong(in->num_vertices);
			if (numindexes%3)
			{
				Con_Printf(S_ERROR "mesh indexes should be multiples of 3\n");
				return false;
			}

			out->mesh = Hunk_Alloc(sizeof(mesh_t) + (sizeof(vec3_t)) * numverts);
			out->mesh->normals_array= map_normals_array + LittleLong(in->firstvertex);
			out->mesh->colors_array	= map_colors_array + LittleLong(in->firstvertex);
			out->mesh->indexes		= map_surfindexes + LittleLong(in->firstindex);
			out->mesh->xyz_array	= map_verts + LittleLong(in->firstvertex);
			out->mesh->st_array		= map_vertstmexcoords + LittleLong(in->firstvertex);
			out->mesh->lmst_array	= map_vertlstmexcoords + LittleLong(in->firstvertex);

			out->mesh->numindexes = numindexes;
			out->mesh->numvertexes = numverts;
		}
		else
		{
//			int r, g, b;
			extern index_t r_quad_indexes[6];

			mesh = out->mesh = (mesh_t *)Hunk_Alloc ( sizeof(mesh_t));
			mesh->xyz_array = (vec3_t *)Hunk_Alloc ( sizeof(vec3_t));
			mesh->numvertexes = 1;
			mesh->indexes = r_quad_indexes;
			mesh->numindexes = 6;

			VectorCopy ( in->lightmap_origin, mesh->xyz_array[0] );

/*			r = LittleFloat ( in->lightmapVecs[0][0] ) * 255.0f;
			r = bound ( 0, r, 255 );

			g = LittleFloat ( in->lightmapVecs[0][1] ) * 255.0f;
			g = bound ( 0, g, 255 );

			b = LittleFloat ( in->lightmapVecs[0][2] ) * 255.0f;
			b = bound ( 0, b, 255 );

			out->dlightbits = (unsigned int)COLOR_RGB ( r, g, b );
*/		}
	}

	if (useshaders)
		CModQ3_SortShaders();

	return true;
}

qboolean CModRBSP_LoadRFaces (lump_t *l, qboolean useshaders)
{
#ifndef Q3SHADERS
	int polysize = sizeof(glpoly_t) - VERTEXSIZE*sizeof(float);
	glpoly_t *p;
	int rv, fi;
	int gv, v;
#endif
	rbspface_t *in;
	msurface_t *out;
	mplane_t *pl;

	int count;
	int surfnum;

	int numverts, numindexes;
	int fv;

	mesh_t *mesh;


	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( count*sizeof(*out), loadmodel->name );
	pl = Hunk_AllocName (count*sizeof(*pl), loadmodel->name);//create a new array of planes for speed.

	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;

	for (surfnum = 0; surfnum < count; surfnum++, out++, in++, pl++)
	{
		out->plane = pl;
		out->texinfo = loadmodel->texinfo + LittleLong(in->shadernum);
		in->facetype = LittleLong(in->facetype);
		out->lightmaptexturenum = in->lightmapnum[0];
		out->light_s = in->lightmap_offs[0][0];
		out->light_t = in->lightmap_offs[0][0];
		out->extents[0] = (in->lightmap_width-1)<<4;
		out->extents[1] = (in->lightmap_height-1)<<4;
		out->samples = loadmodel->lightdata + 3*(out->light_s + out->light_t*128 + out->lightmaptexturenum*128*128);
		if (out->lightmaptexturenum<0)
			out->samples=NULL;

		fv = LittleLong(in->firstvertex);
		{
			vec3_t v[3];
			VectorCopy(map_verts[fv+0], v[0]);
			VectorCopy(map_verts[fv+1], v[1]);
			VectorCopy(map_verts[fv+2], v[2]);
			PlaneFromPoints(v, pl);
			CategorizePlane(pl);
		}
		/*
		if (in->fognum!=-1)
		continue;
		*/

		if (map_surfaces[in->shadernum].c.value == 0 || map_surfaces[in->shadernum].c.value & Q3CONTENTS_TRANSLUCENT)
				//q3dm10's thingie is 0
			out->flags |= SURF_DRAWALPHA;

		if (loadmodel->texinfo[in->shadernum].flags & SURF_SKY)
			out->flags |= SURF_DRAWSKY;

#ifdef Q3SHADERS
		if (!out->texinfo->texture->shader && useshaders)
		{
			extern cvar_t r_vertexlight;
			if (in->facetype == MST_FLARE)
				out->texinfo->texture->shader = R_RegisterShader_Flare (out->texinfo->texture->name);
			else if (in->facetype == MST_TRIANGLE_SOUP || r_vertexlight.value)
				out->texinfo->texture->shader = R_RegisterShader_Vertex (out->texinfo->texture->name);
			else
				out->texinfo->texture->shader = R_RegisterShader(out->texinfo->texture->name);
		}

		if (in->fognum < 0 || in->fognum >= map_numfogs)
			out->fog = NULL;
		else
			out->fog = map_fogs + in->fognum;
#endif
		if (map_surfaces[in->shadernum].c.flags & (Q3SURF_NODRAW | Q3SURF_SKIP))
		{
			if (map_surfaces[in->shadernum].c.flags & Q3SURF_SKIP)
				Con_Printf("Surface skip\n");
			out->mesh = NULL;
		}
		else if (in->facetype == MST_PATCH)
		{
			out->mesh = GL_CreateMeshForPatch(loadmodel, LittleLong(in->patchwidth), LittleLong(in->patchheight), LittleLong(in->num_vertices), LittleLong(in->firstvertex));
		}
		else if (in->facetype == MST_PLANAR || in->facetype == MST_TRIANGLE_SOUP)
		{
			numindexes = LittleLong(in->num_indexes);
			numverts = LittleLong(in->num_vertices);
			if (numindexes%3)
			{
				Con_Printf(S_ERROR "mesh indexes should be multiples of 3\n");
				return false;
			}

			out->mesh = Hunk_Alloc(sizeof(mesh_t) + (sizeof(vec3_t)) * numverts);
			out->mesh->normals_array= map_normals_array + LittleLong(in->firstvertex);
			out->mesh->colors_array	= map_colors_array + LittleLong(in->firstvertex);
			out->mesh->indexes		= map_surfindexes + LittleLong(in->firstindex);
			out->mesh->xyz_array	= map_verts + LittleLong(in->firstvertex);
			out->mesh->st_array		= map_vertstmexcoords + LittleLong(in->firstvertex);
			out->mesh->lmst_array	= map_vertlstmexcoords + LittleLong(in->firstvertex);

			out->mesh->numindexes = numindexes;
			out->mesh->numvertexes = numverts;
		}
		else
		{
//			int r, g, b;
			extern index_t r_quad_indexes[6];

			mesh = out->mesh = (mesh_t *)Hunk_Alloc ( sizeof(mesh_t));
			mesh->xyz_array = (vec3_t *)Hunk_Alloc ( sizeof(vec3_t));
			mesh->numvertexes = 1;
			mesh->indexes = r_quad_indexes;
			mesh->numindexes = 6;
		//	VectorCopy ( out->origin, mesh->xyz_array[0] );

/*			r = LittleFloat ( in->lightmapVecs[0][0] ) * 255.0f;
			r = bound ( 0, r, 255 );

			g = LittleFloat ( in->lightmapVecs[0][1] ) * 255.0f;
			g = bound ( 0, g, 255 );

			b = LittleFloat ( in->lightmapVecs[0][2] ) * 255.0f;
			b = bound ( 0, b, 255 );

			out->dlightbits = (unsigned int)COLOR_RGB ( r, g, b );
*/		}
	}

	if (useshaders)
		CModQ3_SortShaders();

	return true;
}
#endif

qboolean CModQ3_LoadLeafFaces (lump_t *l)
{
	int		i, j, count;
	int		*in;
	int		*out;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count > MAX_Q2MAP_LEAFFACES)
	{
		Con_Printf (S_ERROR "Map has too many leaffaces\n");
		return false;
	}

	out = BZ_Malloc ( count*sizeof(*out) );
	map_leaffaces = out;
	numleaffaces = count;

	for ( i=0 ; i<count ; i++)
	{
		j = LittleLong ( in[i] );

		if (j < 0 ||  j >= numfaces)
		{
			Con_Printf (S_ERROR "CMod_LoadLeafFaces: bad surface number\n");
			return false;
		}

		out[i] = j;
	}

	return true;
}

qboolean CModQ3_LoadNodes (lump_t *l)
{
	int			i, j, count, p;
	q3dnode_t	*in;
	mnode_t 	*out;
	//dnode_t

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	out = Hunk_AllocName ( count*sizeof(*out), loadname);

	if (count > MAX_MAP_NODES)
	{
		Con_Printf (S_ERROR "Too many nodes on map\n");
		return false;
	}

	loadmodel->nodes = out;
	loadmodel->numnodes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->minmaxs[j] = LittleLong (in->mins[j]);
			out->minmaxs[3+j] = LittleLong (in->maxs[j]);
		}

		p = LittleLong(in->plane);
		out->plane = loadmodel->planes + p;

		out->firstsurface = 0;//LittleShort (in->firstface);
		out->numsurfaces = 0;//LittleShort (in->numfaces);

		out->contents = -1;

		for (j=0 ; j<2 ; j++)
		{
			p = LittleLong (in->children[j]);
			out->childnum[j] = p;
			if (p >= 0)
			{
				out->children[j] = loadmodel->nodes + p;
			}
			else
				out->children[j] = (mnode_t *)(loadmodel->leafs + (-1 - p));
		}
	}

	CMod_SetParent (loadmodel->nodes, NULL);	// sets nodes and leafs

	return true;
}

qboolean CModQ3_LoadBrushes (lump_t *l)
{
	q3dbrush_t	*in;
	q2cbrush_t	*out;
	int			i, count;
	int			shaderref;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count > MAX_Q2MAP_BRUSHES)
	{
		Con_Printf (S_ERROR "Map has too many brushes");
		return false;
	}

	out = map_brushes;

	numbrushes = count;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		shaderref = LittleLong ( in->shadernum );
		out->contents = map_surfaces[shaderref].c.value;
		out->firstbrushside = LittleLong ( in->firstside );
		out->numsides = LittleLong ( in->num_sides );
	}

	return true;
}

qboolean CModQ3_LoadLeafs (lump_t *l)
{
	int			i, j;
	mleaf_t		*out;
	q3dleaf_t 	*in;
	int			count;
	q2cbrush_t	*brush;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Con_Printf (S_ERROR "Map with no leafs\n");
		return false;
	}
	// need to save space for box planes

	if (count > MAX_MAP_LEAFS)
	{
		Con_Printf (S_ERROR "Too many leaves on map");
		return false;
	}

	out = map_leafs;
	numleafs = count;
	numclusters = 0;

	loadmodel->leafs = out;
	loadmodel->numleafs = count;

	emptyleaf = -1;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j = 0; j < 3; j++)
		{
			out->minmaxs[0+j] = LittleLong(in->mins[j]);
			out->minmaxs[3+j] = LittleLong(in->maxs[j]);
		}
		out->cluster = LittleLong ( in->cluster );
		out->area = LittleLong ( in->area ) + 1;
		out->firstleafface = LittleLong ( in->firstleafsurface );
		out->numleaffaces = LittleLong ( in->num_leafsurfaces );
		out->contents = 0;
		out->firstleafbrush = LittleLong ( in->firstleafbrush );
		out->numleafbrushes = LittleLong ( in->num_leafbrushes );

		out->firstmarksurface = loadmodel->marksurfaces +
			LittleLong(in->firstleafsurface);
		out->nummarksurfaces = LittleLong(in->num_leafsurfaces);

		if (out->minmaxs[0] > out->minmaxs[3+0] || out->minmaxs[1] > out->minmaxs[3+1] ||
			out->minmaxs[2] > out->minmaxs[3+2] || VectorCompare (out->minmaxs, out->minmaxs+3))
		{
			out->nummarksurfaces = 0;
		}

		for ( j=0 ; j<out->numleafbrushes ; j++)
		{
			brush = &map_brushes[map_leafbrushes[out->firstleafbrush + j]];
			out->contents |= brush->contents;
		}

		if ( out->area >= numareas ) {
			numareas = out->area + 1;
		}

		if ( !out->contents ) {
			emptyleaf = i;
		}
	}

	// if map doesn't have an empty leaf - force one
	if ( emptyleaf == -1 ) {
		if (numleafs >= MAX_MAP_LEAFS-1)
		{
			Con_Printf (S_ERROR "Map does not have an empty leaf\n");
			return false;
		}

		out->cluster = -1;
		out->area = -1;
		out->numleafbrushes = 0;
		out->contents = 0;
		out->firstleafbrush = 0;

		Con_DPrintf ( "Forcing an empty leaf: %i\n", numleafs );
		emptyleaf = numleafs++;
	}

	return true;
}

qboolean CModQ3_LoadPlanes (lump_t *l)
{
	int			i, j;
	mplane_t	*out;
	Q3PLANE_t 	*in;
	int			count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	out = map_planes;//Hunk_AllocName ( count*2*sizeof(*out), loadname);

	if (count > MAX_MAP_PLANES)
	{
		Con_Printf (S_ERROR "Too many planes on map\n");
		return false;
	}

	numplanes = count;

	loadmodel->planes = out;
	loadmodel->numplanes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->n[j]);
		}
		out->dist = LittleFloat (in->d);

		CategorizePlane(out);
	}

	return true;
}

qboolean CModQ3_LoadLeafBrushes (lump_t *l)
{
	int			i;
	int	*out;
	int 	*in;
	int			count;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	if (count < 1)
	{
		Con_Printf (S_ERROR "Map with no leafbrushes\n");
		return false;
	}
	// need to save space for box planes
	if (count > MAX_Q2MAP_LEAFBRUSHES)
	{
		Con_Printf (S_ERROR "Map has too many leafbrushes\n");
		return false;
	}

	out = map_leafbrushes;
	numleafbrushes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
		*out = LittleLong (*in);

	return true;
}

qboolean CModQ3_LoadBrushSides (lump_t *l)
{
	int			i, j;
	q2cbrushside_t	*out;
	q3dbrushside_t 	*in;
	int			count;
	int			num;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	// need to save space for box planes
	if (count > MAX_Q2MAP_BRUSHSIDES)
	{
		Con_Printf (S_ERROR "Map has too many planes\n");
		return false;
	}

	out = map_brushsides;
	numbrushsides = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		num = LittleLong (in->planenum);
		out->plane = &map_planes[num];
		j = LittleLong (in->texinfo);
		if (j >= numtexinfo)
		{
			Con_Printf (S_ERROR "Bad brushside texinfo\n");
			return false;
		}
		out->surface = &map_surfaces[j];
	}

	return true;
}

qboolean CModRBSP_LoadBrushSides (lump_t *l)
{
	int			i, j;
	q2cbrushside_t	*out;
	rbspbrushside_t 	*in;
	int			count;
	int			num;

	in = (void *)(cmod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size\n");
		return false;
	}
	count = l->filelen / sizeof(*in);

	// need to save space for box planes
	if (count > MAX_Q2MAP_BRUSHSIDES)
	{
		Con_Printf (S_ERROR "Map has too many planes\n");
		return false;
	}

	out = map_brushsides;
	numbrushsides = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		num = LittleLong (in->planenum);
		out->plane = &map_planes[num];
		j = LittleLong (in->texinfo);
		if (j >= numtexinfo)
		{
			Con_Printf (S_ERROR "Bad brushside texinfo\n");
			return false;
		}
		out->surface = &map_surfaces[j];
	}

	return true;
}

qboolean CModQ3_LoadVisibility (lump_t *l)
{
	if (l->filelen == 0)
	{
		int i;
		numclusters = 0;
		for (i = 0; i < loadmodel->numleafs; i++)
			if (numclusters <= loadmodel->leafs[i].cluster)
				numclusters = loadmodel->leafs[i].cluster+1;

		numclusters++;

		memset (map_visibility, 0xff, sizeof(map_visibility));
		map_q3pvs->numclusters = numclusters;
		numvisibility = 0;
		map_q3pvs->rowsize = (map_q3pvs->numclusters+7)/8;
	}
	else
	{
		numvisibility = l->filelen;
		if (l->filelen > MAX_Q2MAP_VISIBILITY)
		{
			Con_Printf (S_ERROR "Map has too large visibility lump\n");
			return false;
		}

		loadmodel->vis = (q2dvis_t *)map_q3pvs;

		memcpy (map_visibility, cmod_base + l->fileofs, l->filelen);

		numclusters = map_q3pvs->numclusters = LittleLong (map_q3pvs->numclusters);
		map_q3pvs->rowsize = LittleLong (map_q3pvs->rowsize);
	}

	return true;
}

#ifndef SERVERONLY
qboolean CModQ3_LoadLightgrid (lump_t *l)
{
	dq3gridlight_t 	*in;
	dq3gridlight_t 	*out;
	q3lightgridinfo_t *grid;
	int	count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	grid = Hunk_AllocName (sizeof(q3lightgridinfo_t) + count*sizeof(*out), loadmodel->name );
	grid->lightgrid = (dq3gridlight_t*)(grid+1);
	out = grid->lightgrid;

	loadmodel->lightgrid = grid;
	grid->numlightgridelems = count;

	// lightgrid is all 8 bit
	memcpy ( out, in, count*sizeof(*out) );

	return true;
}
qboolean CModRBSP_LoadLightgrid (lump_t *elements, lump_t *indexes)
{
	unsigned short	*iin;
	rbspgridlight_t	*ein;
	unsigned short	*iout;
	rbspgridlight_t	*eout;
	q3lightgridinfo_t *grid;
	int	ecount;
	int icount;

	int i;

	ein = (void *)(mod_base + elements->fileofs);
	iin = (void *)(mod_base + indexes->fileofs);
	if (indexes->filelen % sizeof(*iin) || elements->filelen % sizeof(*ein))
	{
		Con_Printf (S_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	icount = indexes->filelen / sizeof(*iin);
	ecount = elements->filelen / sizeof(*ein);

	grid = Hunk_AllocName (sizeof(q3lightgridinfo_t) + ecount*sizeof(*eout) + icount*sizeof(*iout), loadmodel->name );
	grid->rbspelements = (rbspgridlight_t*)((char *)grid);
	grid->rbspindexes = (unsigned short*)((char *)grid + ecount*sizeof(*eout));
	eout = grid->rbspelements;
	iout = grid->rbspindexes;

	loadmodel->lightgrid = grid;

	grid->numlightgridelems = icount;

	// elements are all 8 bit
	memcpy ( eout, ein, ecount*sizeof(*eout) );

	for (i = 0; i < icount; i++)
		iout[i] = LittleShort(iin[i]);

	return true;
}
#endif


#ifndef SERVERONLY
qbyte *ReadPCXPalette(qbyte *buf, int len, qbyte *out);
int CM_GetQ2Palette (void)
{
	char *f = (void *)COM_LoadMallocFile("pics/colormap.pcx");
	if (!f)
	{
		Con_Printf (S_WARNING "Couldn't find pics/colormap.pcx\n");
		return -1;
	}
	if (!ReadPCXPalette(f, com_filesize, d_q28to24table))
	{
		Con_Printf (S_WARNING "Couldn't read pics/colormap.pcx\n");
		BZ_Free(f);
		return -1;
	}
	BZ_Free(f);


#ifdef RGLQUAKE
	{
		extern float vid_gamma;
		float	f, inf;
		qbyte	palette[768];
		qbyte *pal;
		int		i;

		pal = d_q28to24table;

		for (i=0 ; i<768 ; i++)
		{
			f = pow ( (pal[i]+1)/256.0 , vid_gamma );
			inf = f*255 + 0.5;
			if (inf < 0)
				inf = 0;
			if (inf > 255)
				inf = 255;
			palette[i] = inf;
		}

		memcpy (d_q28to24table, palette, sizeof(palette));
	}
#endif
	return 0;
}
#endif

void CM_OpenAllPortals(char *ents)	//this is a compleate hack. About as compleate as possible.
{	//q2 levels contain a thingie called area portals. Basically, doors can seperate two areas and
	//the engine knows when this portal is open, and weather to send ents from both sides of the door
	//or not. It's not just ents, but also walls. We want to just open them by default and hope the
	//progs knows how to close them.

	char style[8];
	char name[64];

	if (!map_autoopenportals.value)
		return;

	while(*ents)
	{
		if (*ents == '{')	//an entity
		{
			ents++;
			*style = '\0';
			*name = '\0';
			while (*ents)
			{
				ents = COM_Parse(ents);

				if (!strcmp(com_token, "classname"))
				{
					ents = COM_ParseOut(ents, name, sizeof(name));
				}
				else if (!strcmp(com_token, "style"))
				{
					ents = COM_ParseOut(ents, style, sizeof(style));
				}
				else if (*com_token == '}')
					break;
				else
					ents = COM_Parse(ents);	//other field
				ents++;
			}

			if (!strcmp(name, "func_areaportal"))
			{
				CMQ2_SetAreaPortalState(atoi(style), true);
			}
		}

		ents++;
	}
}


#ifndef CLIENTONLY
void CMQ3_CalcPHS (void)
{
	int		rowbytes, rowwords;
	int		i, j, k, l, index;
	int		bitbyte;
	unsigned	*dest, *src;
	qbyte	*scan;
	int		count, vcount;
	int		numclusters;

	Con_DPrintf ("Building PHS...\n");

	rowwords = map_q3pvs->rowsize / sizeof(long);
	rowbytes = map_q3pvs->rowsize;

	memset ( map_q3phs, 0, MAX_Q2MAP_VISIBILITY );

	map_q3phs->rowsize = map_q3pvs->rowsize;
	map_q3phs->numclusters = numclusters = map_q3pvs->numclusters;
	if (!numclusters)
		return;

	vcount = 0;
	for (i=0 ; i<numclusters ; i++)
	{
		scan = CM_ClusterPVS (sv.worldmodel, i, NULL);
		for (j=0 ; j<numclusters ; j++)
		{
			if ( scan[j>>3] & (1<<(j&7)) )
			{
				vcount++;
			}
		}
	}

	count = 0;
	scan = (qbyte *)map_q3pvs->data;
	dest = (unsigned *)((qbyte *)map_q3phs + 8);

	for (i=0 ; i<numclusters ; i++, dest += rowwords, scan += rowbytes)
	{
		memcpy (dest, scan, rowbytes);
		for (j=0 ; j<rowbytes ; j++)
		{
			bitbyte = scan[j];
			if (!bitbyte)
				continue;
			for (k=0 ; k<8 ; k++)
			{
				if (! (bitbyte & (1<<k)) )
					continue;
				// OR this pvs row into the phs
				index = (j<<3) + k;
//				if (index >= numclusters)
//					Host_Error ("CM_CalcPHS: Bad bit in PVS");	// pad bits should be 0
				src = (unsigned *)((qbyte*)map_q3pvs->data) + index*rowwords;
				for (l=0 ; l<rowwords ; l++)
					dest[l] |= src[l];
			}
		}
		for (j=0 ; j<numclusters ; j++)
			if ( ((qbyte *)dest)[j>>3] & (1<<(j&7)) )
				count++;
	}

	Con_Printf ("Average clusters visible / hearable / total: %i / %i / %i\n"
		, vcount/numclusters, count/numclusters, numclusters);
}
#endif

qbyte *CM_LeafnumPVS (model_t *model, int leafnum, qbyte *buffer)
{
	return CM_ClusterPVS(model, CM_LeafCluster(model, leafnum), buffer);
}

#ifndef SERVERONLY
#define GLQ2BSP_LightPointValues GLQ1BSP_LightPointValues
#define SWQ2BSP_LightPointValues SWQ1BSP_LightPointValues

extern int	r_dlightframecount;
void Q2BSP_MarkLights (dlight_t *light, int bit, mnode_t *node)
{
	mplane_t	*splitplane;
	float		dist;
	msurface_t	*surf;
	int			i;

	if (node->contents != -1)
	{
		mleaf_t *leaf = (mleaf_t *)node;
		msurface_t **mark;

		i = leaf->nummarksurfaces;
		mark = leaf->firstmarksurface;
		while(i--!=0)
		{
			surf = *mark++;
			if (surf->dlightframe != r_dlightframecount)
			{
				surf->dlightbits = 0;
				surf->dlightframe = r_dlightframecount;
			}
			surf->dlightbits |= bit;
		}
		return;
	}

	splitplane = node->plane;
	dist = DotProduct (light->origin, splitplane->normal) - splitplane->dist;

	if (dist > light->radius)
	{
		Q2BSP_MarkLights (light, bit, node->children[0]);
		return;
	}
	if (dist < -light->radius)
	{
		Q2BSP_MarkLights (light, bit, node->children[1]);
		return;
	}

// mark the polygons
	surf = cl.worldmodel->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->dlightframe != r_dlightframecount)
		{
			surf->dlightbits = 0;
			surf->dlightframe = r_dlightframecount;
		}
		surf->dlightbits |= bit;
	}

	Q2BSP_MarkLights (light, bit, node->children[0]);
	Q2BSP_MarkLights (light, bit, node->children[1]);
}

#ifndef SERVERONLY
#ifdef RGLQUAKE
void GLR_StainSurf (msurface_t *surf, float *parms);
void GLR_Q2BSP_StainNode (mnode_t *node, float *parms)
{
	mplane_t	*splitplane;
	float		dist;
	msurface_t	*surf;
	int			i;

	if (node->contents != -1)
		return;

	splitplane = node->plane;
	dist = DotProduct ((parms+1), splitplane->normal) - splitplane->dist;

	if (dist > (*parms))
	{
		GLR_Q2BSP_StainNode (node->children[0], parms);
		return;
	}
	if (dist < (-*parms))
	{
		GLR_Q2BSP_StainNode (node->children[1], parms);
		return;
	}

// mark the polygons
	surf = cl.worldmodel->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->flags&~(SURF_DONTWARP|SURF_PLANEBACK))
			continue;
		GLR_StainSurf(surf, parms);
	}

	GLR_Q2BSP_StainNode (node->children[0], parms);
	GLR_Q2BSP_StainNode (node->children[1], parms);
}
#endif
#ifdef SWQUAKE
void SWR_StainSurf (msurface_t *surf, float *parms);
void SWR_Q2BSP_StainNode (mnode_t *node, float *parms)
{
	mplane_t	*splitplane;
	float		dist;
	msurface_t	*surf;
	int			i;

	if (node->contents != -1)
		return;

	splitplane = node->plane;
	dist = DotProduct ((parms+1), splitplane->normal) - splitplane->dist;

	if (dist > (*parms))
	{
		SWR_Q2BSP_StainNode (node->children[0], parms);
		return;
	}
	if (dist < (-*parms))
	{
		SWR_Q2BSP_StainNode (node->children[1], parms);
		return;
	}

// mark the polygons
	surf = cl.worldmodel->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->flags&~(SURF_DONTWARP|SURF_PLANEBACK))
			continue;
		SWR_StainSurf(surf, parms);
	}

	SWR_Q2BSP_StainNode (node->children[0], parms);
	SWR_Q2BSP_StainNode (node->children[1], parms);
}
#endif
#endif

#endif

#ifndef CLIENTONLY
void Q2BSP_FatPVS (model_t *mod, vec3_t org, qboolean add);
qboolean Q2BSP_EdictInFatPVS(model_t *mod, edict_t *ent);
void Q2BSP_FindTouchedLeafs(model_t *mod, edict_t *ent);
#endif
void GLQ2BSP_LightPointValues(vec3_t point, vec3_t res_diffuse, vec3_t res_ambient, vec3_t res_dir);
void SWQ2BSP_LightPointValues(vec3_t point, vec3_t res_diffuse, vec3_t res_ambient, vec3_t res_dir);

/*
==================
CM_LoadMap

Loads in the map and all submodels
==================
*/
q2cmodel_t *CM_LoadMap (char *name, char *filein, qboolean clientload, unsigned *checksum)
{
	unsigned		*buf;
	int				i,j;
	q2dheader_t		header;
	int				length;
	static unsigned	last_checksum;
	qboolean useshaders;
	qboolean noerrors = true;
	int start;

	// free old stuff
	numplanes = 0;
	numleafs = 0;
	numcmodels = 0;
	numvisibility = 0;
	numentitychars = 0;
	map_entitystring = NULL;
	map_name[0] = 0;

	loadmodel->type = mod_brush;

	if (!name || !name[0])
	{
		numleafs = 1;
		numclusters = 1;
		numareas = 1;
		*checksum = 0;
		return &map_cmodels[0];			// cinematic servers won't have anything at all
	}

	//
	// load the file
	//
	buf = (unsigned	*)filein;
	length = com_filesize;
	if (!buf)
	{
		Con_Printf (S_ERROR "Couldn't load %s\n", name);
		return NULL;
	}

	last_checksum = LittleLong (Com_BlockChecksum (buf, length));
	*checksum = last_checksum;

	header = *(q2dheader_t *)(buf);
	header.ident = LittleLong(header.ident);
	header.version = LittleLong(header.version);

	cmod_base = mod_base = (qbyte *)buf;
	start = Hunk_LowMark();

	switch(header.version)
	{
	default:
		Con_Printf (S_ERROR "Quake 2 or Quake 3 based BSP with unknown header (%i should be %i or %i)\n"
			, name, header.version, Q2BSPVERSION, Q3BSPVERSION);
		return NULL;
		break;
#if 1
	case 1: //rbsp
	case Q3BSPVERSION+1:	//rtcw
	case Q3BSPVERSION:

#ifdef Q3SHADERS
		{
			extern cvar_t gl_shadeq3;
			useshaders = gl_shadeq3.value;
		}
#else
		useshaders = false;
#endif

		mapisq3 = true;
		loadmodel->fromgame = fg_quake3;
		for (i=0 ; i<Q3LUMPS_TOTAL ; i++)
		{
			header.lumps[i].filelen = LittleLong (header.lumps[i].filelen);
			header.lumps[i].fileofs = LittleLong (header.lumps[i].fileofs);
		}
		/*
		#ifndef SERVERONLY
			GLMod_LoadVertexes		(&header.lumps[Q3LUMP_DRAWVERTS]);
//			GLMod_LoadEdges			(&header.lumps[Q3LUMP_EDGES]);
//			GLMod_LoadSurfedges		(&header.lumps[Q3LUMP_SURFEDGES]);
			GLMod_LoadLighting		(&header.lumps[Q3LUMP_LIGHTMAPS]);
		#endif
			CModQ3_LoadShaders		(&header.lumps[Q3LUMP_SHADERS]);
			CModQ3_LoadPlanes		(&header.lumps[Q3LUMP_PLANES]);
			CModQ3_LoadLeafBrushes	(&header.lumps[Q3LUMP_LEAFBRUSHES]);
			CModQ3_LoadBrushes		(&header.lumps[Q3LUMP_BRUSHES]);
			CModQ3_LoadBrushSides	(&header.lumps[Q3LUMP_BRUSHSIDES]);
		#ifndef SERVERONLY
			CMod_LoadTexInfo		(&header.lumps[Q3LUMP_SHADERS]);
			CMod_LoadFaces			(&header.lumps[Q3LUMP_SURFACES]);
//			GLMod_LoadMarksurfaces	(&header.lumps[Q3LUMP_LEAFFACES]);
		#endif
			CMod_LoadVisibility		(&header.lumps[Q3LUMP_VISIBILITY]);
			CModQ3_LoadSubmodels	(&header.lumps[Q3LUMP_MODELS]);
			CModQ3_LoadLeafs		(&header.lumps[Q3LUMP_LEAFS]);
			CModQ3_LoadNodes		(&header.lumps[Q3LUMP_NODES]);
//			CMod_LoadAreas			(&header.lumps[Q3LUMP_AREAS]);
//			CMod_LoadAreaPortals	(&header.lumps[Q3LUMP_AREAPORTALS]);
			CMod_LoadEntityString	(&header.lumps[Q3LUMP_ENTITIES]);
*/

		map_faces = NULL;
		map_leaffaces = NULL;

		switch(qrenderer)
		{
#if defined(RGLQUAKE)
		case QR_OPENGL:
#endif
		case QR_NONE:	//dedicated only
			mapisq3 = true;
			noerrors = noerrors && CModQ3_LoadShaders		(&header.lumps[Q3LUMP_SHADERS], useshaders);
			noerrors = noerrors && CModQ3_LoadPlanes		(&header.lumps[Q3LUMP_PLANES]);
			noerrors = noerrors && CModQ3_LoadLeafBrushes	(&header.lumps[Q3LUMP_LEAFBRUSHES]);
			noerrors = noerrors && CModQ3_LoadBrushes		(&header.lumps[Q3LUMP_BRUSHES]);
			if (header.version == 1)
			{
				noerrors = noerrors && CModRBSP_LoadBrushSides	(&header.lumps[Q3LUMP_BRUSHSIDES]);
				noerrors = noerrors && CModRBSP_LoadVertexes	(&header.lumps[Q3LUMP_DRAWVERTS]);
			}
			else
			{
				noerrors = noerrors && CModQ3_LoadBrushSides	(&header.lumps[Q3LUMP_BRUSHSIDES]);
				noerrors = noerrors && CModQ3_LoadVertexes		(&header.lumps[Q3LUMP_DRAWVERTS]);
			}
			if (header.version == 1)
				noerrors = noerrors && CModRBSP_LoadFaces		(&header.lumps[Q3LUMP_SURFACES]);
			else
				noerrors = noerrors && CModQ3_LoadFaces		(&header.lumps[Q3LUMP_SURFACES]);
#if defined(RGLQUAKE)
			if (qrenderer == QR_OPENGL)
			{
				if (noerrors)
					GLMod_LoadLighting		(&header.lumps[Q3LUMP_LIGHTMAPS]);	//fixme: duplicated loading.
				if (header.version == 1)
					noerrors = noerrors && CModRBSP_LoadLightgrid	(&header.lumps[Q3LUMP_LIGHTGRID], &header.lumps[RBSPLUMP_LIGHTINDEXES]);
				else
					noerrors = noerrors && CModQ3_LoadLightgrid	(&header.lumps[Q3LUMP_LIGHTGRID]);
				noerrors = noerrors && CModQ3_LoadIndexes		(&header.lumps[Q3LUMP_DRAWINDEXES]);
#ifdef Q3SHADERS
				if (header.version != Q3BSPVERSION+1)
					noerrors = noerrors && CModQ3_LoadFogs			(&header.lumps[Q3LUMP_FOGS]);
				else
					map_numfogs = 0;
#endif
				if (header.version == 1)
					noerrors = noerrors && CModRBSP_LoadRFaces	(&header.lumps[Q3LUMP_SURFACES], useshaders);
				else
					noerrors = noerrors && CModQ3_LoadRFaces	(&header.lumps[Q3LUMP_SURFACES], useshaders);
				noerrors = noerrors && CModQ3_LoadMarksurfaces (&header.lumps[Q3LUMP_LEAFSURFACES]);	//fixme: duplicated loading.
			}
#endif
			noerrors = noerrors && CModQ3_LoadLeafFaces	(&header.lumps[Q3LUMP_LEAFSURFACES]);
			noerrors = noerrors && CModQ3_LoadLeafs		(&header.lumps[Q3LUMP_LEAFS]);
			noerrors = noerrors && CModQ3_LoadNodes		(&header.lumps[Q3LUMP_NODES]);
			noerrors = noerrors && CModQ3_LoadSubmodels	(&header.lumps[Q3LUMP_MODELS]);
			noerrors = noerrors && CModQ3_LoadVisibility	(&header.lumps[Q3LUMP_VISIBILITY]);
			if (noerrors)
				CMod_LoadEntityString	(&header.lumps[Q3LUMP_ENTITIES]);

			if (!noerrors)
			{
				if (map_faces)
					BZ_Free(map_faces);
				if (map_leaffaces)
					BZ_Free(map_leaffaces);

				Hunk_FreeToLowMark(start);
				return NULL;
			}

#ifndef CLIENTONLY
			loadmodel->funcs.FatPVS					= Q2BSP_FatPVS;
			loadmodel->funcs.EdictInFatPVS			= Q2BSP_EdictInFatPVS;
			loadmodel->funcs.FindTouchedLeafs_Q1	= Q2BSP_FindTouchedLeafs;
#endif
			loadmodel->funcs.LeafPVS				= CM_LeafnumPVS;
			loadmodel->funcs.LeafnumForPoint			= CM_PointLeafnum;

#if defined(RGLQUAKE)
			loadmodel->funcs.LightPointValues		= GLQ3_LightGrid;
			loadmodel->funcs.StainNode				= GLR_Q2BSP_StainNode;
			loadmodel->funcs.MarkLights				= Q2BSP_MarkLights;
#endif
			loadmodel->funcs.Trace					= CM_Trace;
			loadmodel->funcs.PointContents			= Q2BSP_PointContents;
			loadmodel->funcs.NativeTrace			= CM_NativeTrace;
			loadmodel->funcs.NativeContents			= CM_NativeContents;

#ifndef SERVERONLY
			//light grid info
			if (loadmodel->lightgrid)
			{
				float maxs;
				q3lightgridinfo_t *lg = loadmodel->lightgrid;
				if ( lg->gridSize[0] < 1 || lg->gridSize[1] < 1 || lg->gridSize[2] < 1 )
				{
					lg->gridSize[0] = 64;
					lg->gridSize[1] = 64;
					lg->gridSize[2] = 128;
				}

				for ( i = 0; i < 3; i++ )
				{
					lg->gridMins[i] = lg->gridSize[i] * ceil( (map_cmodels->mins[i] + 1) / lg->gridSize[i] );
					maxs = lg->gridSize[i] * floor( (map_cmodels->maxs[i] - 1) / lg->gridSize[i] );
					lg->gridBounds[i] = (maxs - lg->gridMins[i])/lg->gridSize[i] + 1;
				}

				lg->gridBounds[3] = lg->gridBounds[1] * lg->gridBounds[0];
			}
#endif

			if (!CM_CreatePatchesForLeafs ())	//for clipping
			{
				BZ_Free(map_faces);
				BZ_Free(map_leaffaces);
				Hunk_FreeToLowMark(start);
				return NULL;
			}

#ifndef CLIENTONLY
			CMQ3_CalcPHS();
#endif

//			BZ_Free(map_verts);
			BZ_Free(map_faces);
			BZ_Free(map_leaffaces);

			break;
		default:
#ifdef SERVERONLY
			SV_Error("Cannot load q3bsps with the current renderer (only dedicated and opengl renderer)\n");
#else
			Con_Printf(S_ERROR "Cannot load q3bsps with the current renderer (only dedicated and opengl renderer)\n");
			return NULL;
#endif
		}
		break;
#endif
	case Q2BSPVERSION:
		mapisq3 = false;
		for (i=0 ; i<Q2HEADER_LUMPS ; i++)
		{
			header.lumps[i].filelen = LittleLong (header.lumps[i].filelen);
			header.lumps[i].fileofs = LittleLong (header.lumps[i].fileofs);
		}

#ifndef SERVERONLY
		if (CM_GetQ2Palette())
			memcpy(d_q28to24table, host_basepal, 768);
#endif

		switch(qrenderer)
		{
		case QR_NONE:	//dedicated only
			noerrors = noerrors && CMod_LoadSurfaces		(&header.lumps[Q2LUMP_TEXINFO]);
			noerrors = noerrors && CMod_LoadLeafBrushes	(&header.lumps[Q2LUMP_LEAFBRUSHES]);
			noerrors = noerrors && CMod_LoadPlanes			(&header.lumps[Q2LUMP_PLANES]);
			noerrors = noerrors && CMod_LoadVisibility		(&header.lumps[Q2LUMP_VISIBILITY]);
			noerrors = noerrors && CMod_LoadBrushes		(&header.lumps[Q2LUMP_BRUSHES]);
			noerrors = noerrors && CMod_LoadBrushSides		(&header.lumps[Q2LUMP_BRUSHSIDES]);
			noerrors = noerrors && CMod_LoadSubmodels		(&header.lumps[Q2LUMP_MODELS]);
			noerrors = noerrors && CMod_LoadLeafs			(&header.lumps[Q2LUMP_LEAFS]);
			noerrors = noerrors && CMod_LoadNodes			(&header.lumps[Q2LUMP_NODES]);
			noerrors = noerrors && CMod_LoadAreas			(&header.lumps[Q2LUMP_AREAS]);
			noerrors = noerrors && CMod_LoadAreaPortals	(&header.lumps[Q2LUMP_AREAPORTALS]);
			if (noerrors)
				CMod_LoadEntityString	(&header.lumps[Q2LUMP_ENTITIES]);

#ifndef CLIENTONLY
			loadmodel->funcs.FatPVS					= Q2BSP_FatPVS;
			loadmodel->funcs.EdictInFatPVS			= Q2BSP_EdictInFatPVS;
			loadmodel->funcs.FindTouchedLeafs_Q1	= Q2BSP_FindTouchedLeafs;
#endif
			loadmodel->funcs.LightPointValues		= NULL;
			loadmodel->funcs.StainNode				= NULL;
			loadmodel->funcs.MarkLights				= NULL;
			loadmodel->funcs.LeafPVS				= CM_LeafnumPVS;
			loadmodel->funcs.LeafnumForPoint		= CM_PointLeafnum;
			loadmodel->funcs.Trace					= CM_Trace;
			loadmodel->funcs.PointContents			= Q2BSP_PointContents;
			loadmodel->funcs.NativeTrace			= CM_NativeTrace;
			loadmodel->funcs.NativeContents			= CM_NativeContents;

			break;
#if defined(RGLQUAKE)
		case QR_OPENGL:
		// load into heap
		#ifndef SERVERONLY
			noerrors = noerrors && GLMod_LoadVertexes		(&header.lumps[Q2LUMP_VERTEXES]);
			noerrors = noerrors && GLMod_LoadEdges			(&header.lumps[Q2LUMP_EDGES]);
			noerrors = noerrors && GLMod_LoadSurfedges		(&header.lumps[Q2LUMP_SURFEDGES]);
			if (noerrors)
				GLMod_LoadLighting		(&header.lumps[Q2LUMP_LIGHTING]);
		#endif
			noerrors = noerrors && CMod_LoadSurfaces		(&header.lumps[Q2LUMP_TEXINFO]);
			noerrors = noerrors && CMod_LoadLeafBrushes	(&header.lumps[Q2LUMP_LEAFBRUSHES]);
			noerrors = noerrors && CMod_LoadPlanes			(&header.lumps[Q2LUMP_PLANES]);
		#ifndef SERVERONLY
			noerrors = noerrors && CMod_LoadTexInfo		(&header.lumps[Q2LUMP_TEXINFO]);
			noerrors = noerrors && CMod_LoadFaces			(&header.lumps[Q2LUMP_FACES]);
			noerrors = noerrors && GLMod_LoadMarksurfaces	(&header.lumps[Q2LUMP_LEAFFACES]);
		#endif
			noerrors = noerrors && CMod_LoadVisibility		(&header.lumps[Q2LUMP_VISIBILITY]);
			noerrors = noerrors && CMod_LoadBrushes		(&header.lumps[Q2LUMP_BRUSHES]);
			noerrors = noerrors && CMod_LoadBrushSides		(&header.lumps[Q2LUMP_BRUSHSIDES]);
			noerrors = noerrors && CMod_LoadSubmodels		(&header.lumps[Q2LUMP_MODELS]);
			noerrors = noerrors && CMod_LoadLeafs			(&header.lumps[Q2LUMP_LEAFS]);
			noerrors = noerrors && CMod_LoadNodes			(&header.lumps[Q2LUMP_NODES]);
			noerrors = noerrors && CMod_LoadAreas			(&header.lumps[Q2LUMP_AREAS]);
			noerrors = noerrors && CMod_LoadAreaPortals	(&header.lumps[Q2LUMP_AREAPORTALS]);
			if (noerrors)
				CMod_LoadEntityString	(&header.lumps[Q2LUMP_ENTITIES]);

			if (!noerrors)
			{
				Hunk_FreeToLowMark(start);
				return NULL;
			}
#ifndef CLIENTONLY
			loadmodel->funcs.FatPVS					= Q2BSP_FatPVS;
			loadmodel->funcs.EdictInFatPVS			= Q2BSP_EdictInFatPVS;
			loadmodel->funcs.FindTouchedLeafs_Q1	= Q2BSP_FindTouchedLeafs;
#endif
			loadmodel->funcs.LightPointValues		= GLQ2BSP_LightPointValues;
			loadmodel->funcs.StainNode				= GLR_Q2BSP_StainNode;
			loadmodel->funcs.MarkLights				= Q2BSP_MarkLights;
			loadmodel->funcs.LeafPVS				= CM_LeafnumPVS;
			loadmodel->funcs.LeafnumForPoint		= CM_PointLeafnum;
			loadmodel->funcs.Trace					= CM_Trace;
			loadmodel->funcs.PointContents			= Q2BSP_PointContents;
			loadmodel->funcs.NativeTrace			= CM_NativeTrace;
			loadmodel->funcs.NativeContents			= CM_NativeContents;
			break;
#endif
#if defined(SWQUAKE)
		case QR_SOFTWARE:
		// load into heap
		#ifndef SERVERONLY
			noerrors = noerrors && SWMod_LoadVertexes		(&header.lumps[Q2LUMP_VERTEXES]);
			noerrors = noerrors && SWMod_LoadEdges			(&header.lumps[Q2LUMP_EDGES]);
			noerrors = noerrors && SWMod_LoadSurfedges		(&header.lumps[Q2LUMP_SURFEDGES]);
			if (noerrors)
				SWMod_LoadLighting		(&header.lumps[Q2LUMP_LIGHTING]);
		#endif
			noerrors = noerrors && CMod_LoadSurfaces		(&header.lumps[Q2LUMP_TEXINFO]);
			noerrors = noerrors && CMod_LoadLeafBrushes	(&header.lumps[Q2LUMP_LEAFBRUSHES]);
			noerrors = noerrors && CMod_LoadPlanes			(&header.lumps[Q2LUMP_PLANES]);
		#ifndef SERVERONLY
			noerrors = noerrors && CMod_LoadTexInfo		(&header.lumps[Q2LUMP_TEXINFO]);
			noerrors = noerrors && CMod_LoadFaces			(&header.lumps[Q2LUMP_FACES]);
			noerrors = noerrors && SWMod_LoadMarksurfaces	(&header.lumps[Q2LUMP_LEAFFACES]);
		#endif
			noerrors = noerrors && CMod_LoadVisibility		(&header.lumps[Q2LUMP_VISIBILITY]);
			noerrors = noerrors && CMod_LoadBrushes		(&header.lumps[Q2LUMP_BRUSHES]);
			noerrors = noerrors && CMod_LoadBrushSides		(&header.lumps[Q2LUMP_BRUSHSIDES]);
			noerrors = noerrors && CMod_LoadSubmodels		(&header.lumps[Q2LUMP_MODELS]);
			noerrors = noerrors && CMod_LoadLeafs			(&header.lumps[Q2LUMP_LEAFS]);
			noerrors = noerrors && CMod_LoadNodes			(&header.lumps[Q2LUMP_NODES]);
			noerrors = noerrors && CMod_LoadAreas			(&header.lumps[Q2LUMP_AREAS]);
			noerrors = noerrors && CMod_LoadAreaPortals	(&header.lumps[Q2LUMP_AREAPORTALS]);
			if (noerrors)
				CMod_LoadEntityString	(&header.lumps[Q2LUMP_ENTITIES]);

			if (!noerrors)
			{
				Hunk_FreeToLowMark(start);
				return NULL;
			}

			loadmodel->funcs.FatPVS					= Q2BSP_FatPVS;
			loadmodel->funcs.EdictInFatPVS			= Q2BSP_EdictInFatPVS;
			loadmodel->funcs.FindTouchedLeafs_Q1	= Q2BSP_FindTouchedLeafs;
			loadmodel->funcs.LightPointValues		= SWQ2BSP_LightPointValues;
			loadmodel->funcs.StainNode				= SWR_Q2BSP_StainNode;
			loadmodel->funcs.MarkLights				= Q2BSP_MarkLights;
			loadmodel->funcs.LeafPVS				= CM_LeafnumPVS;
			loadmodel->funcs.LeafnumForPoint		= CM_PointLeafnum;
			loadmodel->funcs.Trace					= CM_Trace;
			loadmodel->funcs.PointContents			= Q2BSP_PointContents;
			loadmodel->funcs.NativeTrace			= CM_NativeTrace;
			loadmodel->funcs.NativeContents			= CM_NativeContents;
			break;
#endif
		default:
			Sys_Error("Bad internal renderer on q2 map load\n");
		}
	}

#ifndef SERVERONLY
	Mod_ParseInfoFromEntityLump(loadmodel->entities);	//only done for client's world model (or server if the server is loading it for client)
#endif

	CM_InitBoxHull ();

	if (map_autoopenportals.value)
		memset (portalopen, 1, sizeof(portalopen));	//open them all. Used for progs that havn't got a clue.
	else
		memset (portalopen, 0, sizeof(portalopen));	//make them start closed.
	FloodAreaConnections ();

	strcpy (map_name, name);



	loadmodel->checksum = loadmodel->checksum2 = *checksum;


	loadmodel->numsubmodels = CM_NumInlineModels(loadmodel);
	{
		model_t	*mod = loadmodel;

		mod->hulls[0].firstclipnode = map_cmodels[0].headnode;
		mod->hulls[0].available = true;
		Q2BSP_SetHullFuncs(&mod->hulls[0]);

		for (j=1 ; j<MAX_MAP_HULLSM ; j++)
		{
			mod->hulls[j].firstclipnode = map_cmodels[0].headnode;
			mod->hulls[j].available = false;
			Q2BSP_SetHullFuncs(&mod->hulls[j]);
		}

		for (i=1 ; i< loadmodel->numsubmodels ; i++)
		{
			q2cmodel_t	*bm;

			char	name[10];

			sprintf (name, "*%i", i);
			loadmodel = Mod_FindName (name);
			*loadmodel = *mod;
			strcpy (loadmodel->name, name);
			mod = loadmodel;

			bm = CM_InlineModel (name);


			mod->hulls[0].firstclipnode = bm->headnode;
			mod->hulls[j].available = true;
			mod->nummodelsurfaces = bm->numsurfaces;
			mod->firstmodelsurface = bm->firstsurface;
			Q2BSP_SetHullFuncs(&mod->hulls[0]);
			for (j=1 ; j<MAX_MAP_HULLSM ; j++)
			{
				mod->hulls[j].firstclipnode = bm->headnode;
				mod->hulls[j].lastclipnode = mod->numclipnodes-1;
				mod->hulls[j].available = false;
				Q2BSP_SetHullFuncs(&mod->hulls[j]);
			}

			VectorCopy (bm->maxs, mod->maxs);
			VectorCopy (bm->mins, mod->mins);
#ifndef SERVERONLY
			mod->radius = RadiusFromBounds (mod->mins, mod->maxs);

			P_DefaultTrail(mod);
#endif
		}
	}


	return &map_cmodels[0];
}

/*
==================
CM_InlineModel
==================
*/
q2cmodel_t	*CM_InlineModel (char *name)
{
	int		num;

	if (!name)
		Host_Error("Bad model\n");
	else if (name[0] != '*')
		Host_Error("Bad model\n");

	num = atoi (name+1);

	if (num < 1 || num >= numcmodels)
		Host_Error ("CM_InlineModel: bad number");

	return &map_cmodels[num];
}

int		CM_NumClusters (model_t *model)
{
	return numclusters;
}

int		CM_ClusterSize (model_t *model)
{
	return map_q3pvs->rowsize ? map_q3pvs->rowsize : MAX_MAP_LEAFS / 8;
}

int		CM_NumInlineModels (model_t *model)
{
	return numcmodels;
}

char	*CM_EntityString (model_t *model)
{
	return map_entitystring;
}

int		CM_LeafContents (model_t *model, int leafnum)
{
	if (leafnum < 0 || leafnum >= numleafs)
		Host_Error ("CM_LeafContents: bad number");
	return map_leafs[leafnum].contents;
}

int		CM_LeafCluster (model_t *model, int leafnum)
{
	if (leafnum < 0 || leafnum >= numleafs)
		Host_Error ("CM_LeafCluster: bad number");
	return map_leafs[leafnum].cluster;
}

int		CM_LeafArea (model_t *model, int leafnum)
{
	if (leafnum < 0 || leafnum >= numleafs)
		Host_Error ("CM_LeafArea: bad number");
	return map_leafs[leafnum].area;
}

//=======================================================================


mplane_t	*box_planes;
int			box_headnode;
q2cbrush_t	*box_brush;
mleaf_t		*box_leaf;
model_t		box_model;

/*
===================
CM_InitBoxHull

Set up the planes and nodes so that the six floats of a bounding box
can just be stored out and get a proper clipping hull structure.
===================
*/
void CM_InitBoxHull (void)
{
	int			i;
	int			side;
	mnode_t		*c;
	mplane_t	*p;
	q2cbrushside_t	*s;


#ifndef CLIENTONLY
	box_model.funcs.FatPVS				= Q2BSP_FatPVS;
	box_model.funcs.EdictInFatPVS		= Q2BSP_EdictInFatPVS;
	box_model.funcs.FindTouchedLeafs_Q1	= Q2BSP_FindTouchedLeafs;
#endif

#ifndef SERVERONLY
	box_model.funcs.MarkLights			= Q2BSP_MarkLights;
#endif
	box_model.funcs.LeafPVS				= CM_LeafnumPVS;
	box_model.funcs.LeafnumForPoint		= CM_PointLeafnum;
	box_model.funcs.Trace				= CM_Trace;
	box_model.funcs.NativeContents		= CM_NativeContents;
	box_model.funcs.NativeTrace			= CM_NativeTrace;

	box_model.hulls[0].available = true;
	Q2BSP_SetHullFuncs(&box_model.hulls[0]);

	box_model.nodes = Hunk_Alloc(sizeof(mnode_t)*6);
	box_planes = &map_planes[numplanes];
	if (numbrushes+1 > MAX_Q2MAP_BRUSHES
		|| numleafbrushes+1 > MAX_Q2MAP_LEAFBRUSHES
		|| numbrushsides+6 > MAX_Q2MAP_BRUSHSIDES
		|| numplanes+12 > MAX_Q2MAP_PLANES)
		Host_Error ("Not enough room for box tree");

	box_brush = &map_brushes[numbrushes];
	box_brush->numsides = 6;
	box_brush->firstbrushside = numbrushsides;
	box_brush->contents = Q2CONTENTS_MONSTER;

	box_leaf = &map_leafs[numleafs];
	box_leaf->contents = Q2CONTENTS_MONSTER;
	box_leaf->firstleafbrush = numleafbrushes;
	box_leaf->numleafbrushes = 1;

	map_leafbrushes[numleafbrushes] = numbrushes;

	for (i=0 ; i<6 ; i++)
	{
		side = i&1;

		// brush sides
		s = &map_brushsides[numbrushsides+i];
		s->plane = 	map_planes + (numplanes+i*2+side);
		s->surface = &nullsurface;

		// nodes
		c = &box_model.nodes[i];
		c->plane = map_planes + (numplanes+i*2);
		c->childnum[side] = -1 - emptyleaf;
		if (i != 5)
			c->childnum[side^1] = box_headnode+i + 1;
		else
			c->childnum[side^1] = -1 - numleafs;

		// planes
		p = &box_planes[i*2];
		p->type = i>>1;
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = 1;

		p = &box_planes[i*2+1];
		p->type = 3 + (i>>1);
		p->signbits = 0;
		VectorClear (p->normal);
		p->normal[i>>1] = -1;
	}
}


/*
===================
CM_HeadnodeForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
void CM_SetTempboxSize (vec3_t mins, vec3_t maxs)
{
	box_planes[0].dist = maxs[0];
	box_planes[1].dist = -maxs[0];
	box_planes[2].dist = mins[0];
	box_planes[3].dist = -mins[0];
	box_planes[4].dist = maxs[1];
	box_planes[5].dist = -maxs[1];
	box_planes[6].dist = mins[1];
	box_planes[7].dist = -mins[1];
	box_planes[8].dist = maxs[2];
	box_planes[9].dist = -maxs[2];
	box_planes[10].dist = mins[2];
	box_planes[11].dist = -mins[2];
}

model_t *CM_TempBoxModel(vec3_t mins, vec3_t maxs)
{
	if (box_planes == NULL)
		CM_InitBoxHull();
	CM_SetTempboxSize(mins, maxs);
	return &box_model;
}

/*
==================
CM_PointLeafnum_r

==================
*/
int CM_PointLeafnum_r (model_t *mod, vec3_t p, int num)
{
	float		d;
	mnode_t		*node;
	mplane_t	*plane;

	while (num >= 0)
	{
		node = mod->nodes + num;
		plane = node->plane;

		if (plane->type < 3)
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct (plane->normal, p) - plane->dist;
		if (d < 0)
			num = node->childnum[1];
		else
			num = node->childnum[0];
	}

	c_pointcontents++;		// optimize counter

	return -1 - num;
}

int CM_PointLeafnum (model_t *mod, vec3_t p)
{
	if (!numplanes)
		return 0;		// sound may call this without map loaded
	return CM_PointLeafnum_r (mod, p, 0);
}

/*
=============
CM_BoxLeafnums

Fills in a list of all the leafs touched
=============
*/
int		leaf_count, leaf_maxcount;
int		*leaf_list;
float	*leaf_mins, *leaf_maxs;
int		leaf_topnode;

void CM_BoxLeafnums_r (model_t *mod, int nodenum)
{
	mplane_t	*plane;
	mnode_t		*node;
	int		s;

	while (1)
	{
		if (nodenum < 0)
		{
			if (leaf_count >= leaf_maxcount)
			{
//				Com_Printf ("CM_BoxLeafnums_r: overflow\n");
				return;
			}
			leaf_list[leaf_count++] = -1 - nodenum;
			return;
		}

		node = &mod->nodes[nodenum];
		plane = node->plane;
//		s = BoxOnPlaneSide (leaf_mins, leaf_maxs, plane);
		s = BOX_ON_PLANE_SIDE(leaf_mins, leaf_maxs, plane);
		if (s == 1)
			nodenum = node->childnum[0];
		else if (s == 2)
			nodenum = node->childnum[1];
		else
		{	// go down both
			if (leaf_topnode == -1)
				leaf_topnode = nodenum;
			CM_BoxLeafnums_r (mod, node->childnum[0]);
			nodenum = node->childnum[1];
		}

	}
}

int	CM_BoxLeafnums_headnode (model_t *mod, vec3_t mins, vec3_t maxs, int *list, int listsize, int headnode, int *topnode)
{
	leaf_list = list;
	leaf_count = 0;
	leaf_maxcount = listsize;
	leaf_mins = mins;
	leaf_maxs = maxs;

	leaf_topnode = -1;

	CM_BoxLeafnums_r (mod, headnode);

	if (topnode)
		*topnode = leaf_topnode;

	return leaf_count;
}

int	CM_BoxLeafnums (model_t *mod, vec3_t mins, vec3_t maxs, int *list, int listsize, int *topnode)
{
	return CM_BoxLeafnums_headnode (mod, mins, maxs, list,
		listsize, mod->hulls[0].firstclipnode, topnode);
}



/*
==================
CM_PointContents

==================
*/
#define PlaneDiff(point,plane) (((plane)->type < 3 ? (point)[(plane)->type] : DotProduct((point), (plane)->normal)) - (plane)->dist)
int CM_PointContents (model_t *mod, vec3_t p)
{
	int				i, j, contents;
	mleaf_t			*leaf;
	q2cbrush_t		*brush;
	q2cbrushside_t	*brushside;

	if (!mod)	// map not loaded
		return 0;

	i = CM_PointLeafnum_r (mod, p, mod->hulls[0].firstclipnode);

	if (!mapisq3)
		return map_leafs[i].contents;	//q2 is simple.

	leaf = &map_leafs[i];

//	if ( leaf->contents & CONTENTS_NODROP ) {
//		contents = CONTENTS_NODROP;
//	} else {
		contents = 0;
//	}

	for (i = 0; i < leaf->numleafbrushes; i++)
	{
		brush = &map_brushes[map_leafbrushes[leaf->firstleafbrush + i]];

		// check if brush actually adds something to contents
		if ( (contents & brush->contents) == brush->contents ) {
			continue;
		}

		brushside = &map_brushsides[brush->firstbrushside];
		for ( j = 0; j < brush->numsides; j++, brushside++ )
		{
			if ( PlaneDiff (p, brushside->plane) > 0 )
				break;
		}

		if (j == brush->numsides)
			contents |= brush->contents;
	}

	return contents;
}

unsigned int CM_NativeContents(struct model_s *model, int hulloverride, int frame, vec3_t p, vec3_t mins, vec3_t maxs)
{
	int	contents;
	if (!DotProduct(mins, mins) && !DotProduct(maxs, maxs))
		return CM_PointContents(model, p);

	if (!model)	// map not loaded
		return 0;


	{
		int i, j, k;
		mleaf_t			*leaf;
		q2cbrush_t		*brush;
		q2cbrushside_t	*brushside;
		vec3_t absmin, absmax;

		int leaflist[64];

		k = CM_BoxLeafnums (model, absmin, absmax, leaflist, 64, NULL);

		contents = 0;
		for (k--; k >= 0; k--)
		{
			leaf = &map_leafs[leaflist[k]];
			if (mapisq3)
			{
				for (i = 0; i < leaf->numleafbrushes; i++)
				{
					brush = &map_brushes[map_leafbrushes[leaf->firstleafbrush + i]];

					// check if brush actually adds something to contents
					if ( (contents & brush->contents) == brush->contents ) {
						continue;
					}
					
					brushside = &map_brushsides[brush->firstbrushside];
					for ( j = 0; j < brush->numsides; j++, brushside++ )
					{
						if ( PlaneDiff (p, brushside->plane) > 0 )
							break;
					}

					if (j == brush->numsides) 
						contents |= brush->contents;
				}
			}
			else
				contents |= leaf->contents;
		}
	}

	return contents;
}

/*
==================
CM_TransformedPointContents

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
int	CM_TransformedPointContents (model_t *mod, vec3_t p, int headnode, vec3_t origin, vec3_t angles)
{
	vec3_t		p_l;
	vec3_t		temp;
	vec3_t		forward, right, up;

	// subtract origin offset
	VectorSubtract (p, origin, p_l);

	// rotate start and end into the models frame of reference
	if (headnode != box_headnode &&
	(angles[0] || angles[1] || angles[2]) )
	{
		AngleVectors (angles, forward, right, up);

		VectorCopy (p_l, temp);
		p_l[0] = DotProduct (temp, forward);
		p_l[1] = -DotProduct (temp, right);
		p_l[2] = DotProduct (temp, up);
	}

	return CM_PointContents(mod, p);
}


/*
===============================================================================

BOX TRACING

===============================================================================
*/

// 1/32 epsilon to keep floating point happy
#define	DIST_EPSILON	(0.03125)

vec3_t	trace_start, trace_end;
vec3_t	trace_mins, trace_maxs;
vec3_t	trace_extents;
vec3_t	trace_absmins, trace_absmaxs;
float	trace_truefraction;
float	trace_nearfraction;

trace_t	trace_trace;
int		trace_contents;
qboolean	trace_ispoint;		// optimized case

/*
================
CM_ClipBoxToBrush
================
*/
void CM_ClipBoxToBrush (vec3_t mins, vec3_t maxs, vec3_t p1, vec3_t p2,
					  trace_t *trace, q2cbrush_t *brush)
{
	int			i, j;
	mplane_t	*plane, *clipplane;
	float		dist;
	float		enterfrac, leavefrac;
	vec3_t		ofs;
	float		d1, d2;
	qboolean	getout, startout;
	float		f;
	q2cbrushside_t	*side, *leadside;

	float nearfrac=0;
	enterfrac = -1;
	leavefrac = 2;
	clipplane = NULL;

	if (!brush->numsides)
		return;

	c_brush_traces++;

	getout = false;
	startout = false;
	leadside = NULL;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = &map_brushsides[brush->firstbrushside+i];
		plane = side->plane;

		// FIXME: special case for axial

		if (!trace_ispoint)
		{	// general box case

			// push the plane out apropriately for mins/maxs

			// FIXME: use signbits into 8 way lookup for each mins/maxs
			for (j=0 ; j<3 ; j++)
			{
				if (plane->normal[j] < 0)
					ofs[j] = maxs[j];
				else
					ofs[j] = mins[j];
			}
			dist = DotProduct (ofs, plane->normal);
			dist = plane->dist - dist;
		}
		else
		{	// special point case
			dist = plane->dist;
		}

		d1 = DotProduct (p1, plane->normal) - dist;
		d2 = DotProduct (p2, plane->normal) - dist;

		if (d2 > 0)
			getout = true;	// endpoint is not in solid
		if (d1 > 0)
			startout = true;

		// if completely in front of face, no intersection
		if (d1 > 0 && d2 >= d1)
			return;

		if (d1 <= 0 && d2 <= 0)
			continue;

		// crosses face
		if (d1 > d2)
		{	// enter
			f = (d1) / (d1-d2);
			if (f > enterfrac)
			{
				enterfrac = f;
				nearfrac = (d1-DIST_EPSILON) / (d1-d2);
				clipplane = plane;
				leadside = side;
			}
		}
		else
		{	// leave
			f = (d1) / (d1-d2);
			if (f < leavefrac)
				leavefrac = f;
		}
	}

	if (!startout)
	{	// original point was inside brush
		trace->startsolid = true;
		if (!getout)
			trace->allsolid = true;
		return;
	}
	if (enterfrac <= leavefrac)
	{
		if (enterfrac > -1 && enterfrac <= trace_truefraction)
		{
			if (enterfrac < 0)
				enterfrac = 0;

			trace_nearfraction = nearfrac;
			trace_truefraction = enterfrac;

			trace->plane.dist = clipplane->dist;
			VectorCopy(clipplane->normal, trace->plane.normal);
			trace->surface = &(leadside->surface->c);
			trace->contents = brush->contents;
		}
	}
}

void CM_ClipBoxToPatch (vec3_t mins, vec3_t maxs, vec3_t p1, vec3_t p2,
					  trace_t *trace, q2cbrush_t *brush)
{
	int			i, j;
	mplane_t	*plane, *clipplane;
	float		enterfrac, leavefrac, nearfrac = 0;
	vec3_t		ofs;
	float		d1, d2;
	float dist;
	qboolean	startout;
	float		f;
	q2cbrushside_t	*side, *leadside;

	if (!brush->numsides)
		return;

	c_brush_traces++;

	enterfrac = -1;
	leavefrac = 2;
	clipplane = NULL;
	startout = false;
	leadside = NULL;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = &map_brushsides[brush->firstbrushside+i];
		plane = side->plane;

		if (!trace_ispoint)
		{	// general box case

			// push the plane out apropriately for mins/maxs

			// FIXME: use signbits into 8 way lookup for each mins/maxs
			for (j=0 ; j<3 ; j++)
			{
				if (plane->normal[j] < 0)
					ofs[j] = maxs[j];
				else
					ofs[j] = mins[j];
			}
			dist = DotProduct (ofs, plane->normal);
			dist = plane->dist - dist;
		}
		else
		{	// special point case
			dist = plane->dist;
		}

		d1 = DotProduct (p1, plane->normal) - dist;
		d2 = DotProduct (p2, plane->normal) - dist;

		// if completely in front of face, no intersection
		if (d1 > 0 && d2 >= d1)
			return;

		if (d1 > 0)
			startout = true;

		if (d1 <= 0 && d2 <= 0)
			continue;

		// crosses face
		if (d1 > d2)
		{	// enter
			f = (d1) / (d1-d2);
			if (f > enterfrac)
			{
				enterfrac = f;
				nearfrac = (d1-DIST_EPSILON) / (d1-d2);
				clipplane = plane;
				leadside = side;
			}
		}
		else
		{	// leave
			f = (d1) / (d1-d2);
			if (f < leavefrac)
				leavefrac = f;
		}
	}

	if (!startout)
	{
		trace->startsolid = true;
		return;		// original point is inside the patch
	}

	if (nearfrac <= leavefrac)
	{
		if (leadside && leadside->surface
			&& enterfrac <= trace_truefraction)
		{
			if (enterfrac < 0)
				enterfrac = 0;
			trace_truefraction = enterfrac;
			trace_nearfraction = nearfrac;
			trace->plane.dist = clipplane->dist;
			VectorCopy(clipplane->normal, trace->plane.normal);
			trace->surface = &leadside->surface->c;
			trace->contents = brush->contents;
		}
		else if (enterfrac < trace_truefraction)
			leavefrac=0;
	}
}


/*
================
CM_TestBoxInBrush
================
*/
void CM_TestBoxInBrush (vec3_t mins, vec3_t maxs, vec3_t p1,
					  trace_t *trace, q2cbrush_t *brush)
{
	int			i, j;
	mplane_t	*plane;
	float		dist;
	vec3_t		ofs;
	float		d1;
	q2cbrushside_t	*side;

	if (!brush->numsides)
		return;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = &map_brushsides[brush->firstbrushside+i];
		plane = side->plane;

		// FIXME: special case for axial

		// general box case

		// push the plane out apropriately for mins/maxs

		// FIXME: use signbits into 8 way lookup for each mins/maxs
		for (j=0 ; j<3 ; j++)
		{
			if (plane->normal[j] < 0)
				ofs[j] = maxs[j];
			else
				ofs[j] = mins[j];
		}
		dist = DotProduct (ofs, plane->normal);
		dist = plane->dist - dist;

		d1 = DotProduct (p1, plane->normal) - dist;

		// if completely in front of face, no intersection
		if (d1 > 0)
			return;
	}

	// inside this brush
	trace->startsolid = trace->allsolid = true;
	trace->fraction = 0;
	trace->contents |= brush->contents;
}

void CM_TestBoxInPatch (vec3_t mins, vec3_t maxs, vec3_t p1,
					  trace_t *trace, q2cbrush_t *brush)
{
	int			i, j;
	mplane_t	*plane;
	vec3_t		ofs;
	float dist;
	float		d1, maxdist;
	q2cbrushside_t	*side;

	if (!brush->numsides)
		return;

	maxdist = -9999;

	for (i=0 ; i<brush->numsides ; i++)
	{
		side = &map_brushsides[brush->firstbrushside+i];
		plane = side->plane;

		// general box case

		// push the plane out apropriately for mins/maxs

		// FIXME: use signbits into 8 way lookup for each mins/maxs
		for (j=0 ; j<3 ; j++)
		{
			if (plane->normal[j] < 0)
				ofs[j] = maxs[j];
			else
				ofs[j] = mins[j];
		}

		dist = DotProduct (ofs, plane->normal);
		dist = plane->dist - dist;

		d1 = DotProduct (p1, plane->normal) - dist;

		// if completely in front of face, no intersection
		if (d1 > 0)
			return;

		if (side->surface && d1 > maxdist)
			maxdist = d1;
	}

// FIXME
	if (maxdist < -0.25)
		return;		// deep inside the patch

	// inside this patch
	trace->startsolid = trace->allsolid = true;
	trace->fraction = 0;
	trace->contents = brush->contents;
}


/*
================
CM_TraceToLeaf
================
*/
void CM_TraceToLeaf (int leafnum)
{
	int			k, j;
	int			brushnum;
	mleaf_t		*leaf;
	q2cbrush_t	*b;

	int patchnum;
	q3cpatch_t *patch;

	leaf = &map_leafs[leafnum];
	if ( !(leaf->contents & trace_contents))
		return;
	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numleafbrushes ; k++)
	{
		brushnum = map_leafbrushes[leaf->firstleafbrush+k];
		b = &map_brushes[brushnum];
		if (b->checkcount == checkcount)
			continue;	// already checked this brush in another leaf
		b->checkcount = checkcount;

		if ( !(b->contents & trace_contents))
			continue;
		CM_ClipBoxToBrush (trace_mins, trace_maxs, trace_start, trace_end, &trace_trace, b);
		if (trace_nearfraction <= 0)
			return;
	}

	if (!mapisq3 || map_noCurves.value)
		return;

	// trace line against all patches in the leaf
	for (k = 0; k < leaf->numleafpatches; k++)
	{
		patchnum = map_leafpatches[leaf->firstleafpatch+k];

		patch = &map_patches[patchnum];
		if (patch->checkcount == checkcount)
			continue;	// already checked this patch in another leaf
		patch->checkcount = checkcount;
		if ( !(patch->surface->c.value & trace_contents) )
			continue;
		if ( !BoundsIntersect(patch->absmins, patch->absmaxs, trace_absmins, trace_absmaxs) )
			continue;
		for (j = 0; j < patch->numbrushes; j++)
		{
			CM_ClipBoxToPatch (trace_mins, trace_maxs, trace_start, trace_end, &trace_trace, &patch->brushes[j]);
			if (trace_nearfraction<=0)
				return;
		}
	}

}


/*
================
CM_TestInLeaf
================
*/
void CM_TestInLeaf (int leafnum)
{
	int			k, j;
	int			brushnum;
	int patchnum;
	mleaf_t		*leaf;
	q2cbrush_t	*b;
	q3cpatch_t *patch;

	leaf = &map_leafs[leafnum];
	if ( !(leaf->contents & trace_contents))
		return;
	// trace line against all brushes in the leaf
	for (k=0 ; k<leaf->numleafbrushes ; k++)
	{
		brushnum = map_leafbrushes[leaf->firstleafbrush+k];
		b = &map_brushes[brushnum];
		if (b->checkcount == checkcount)
			continue;	// already checked this brush in another leaf
		b->checkcount = checkcount;

		if ( !(b->contents & trace_contents))
			continue;
		CM_TestBoxInBrush (trace_mins, trace_maxs, trace_start, &trace_trace, b);
		if (!trace_trace.fraction)
			return;
	}

	if (!mapisq3 || map_noCurves.value)
		return;

  	// trace line against all patches in the leaf
	for (k = 0; k < leaf->numleafpatches; k++)
	{
		patchnum = map_leafpatches[leaf->firstleafpatch+k];

		patch = &map_patches[patchnum];
		if (patch->checkcount == checkcount)
			continue;	// already checked this patch in another leaf
		patch->checkcount = checkcount;
		if ( !(patch->surface->c.value & trace_contents) )
			continue;
		if ( !BoundsIntersect(patch->absmins, patch->absmaxs, trace_absmins, trace_absmaxs) )
			continue;
		for (j = 0; j < patch->numbrushes; j++)
		{
			CM_TestBoxInPatch (trace_mins, trace_maxs, trace_start, &trace_trace, &patch->brushes[j]);
			if (!trace_trace.fraction)
				return;
		}
	}

}


/*
==================
CM_RecursiveHullCheck

==================
*/
void CM_RecursiveHullCheck (model_t *mod, int num, float p1f, float p2f, vec3_t p1, vec3_t p2)
{
	mnode_t		*node;
	mplane_t	*plane;
	float		t1, t2, offset;
	float		frac, frac2;
	float		idist;
	int			i;
	vec3_t		mid;
	int			side;
	float		midf;

	if (trace_truefraction <= p1f)
		return;		// already hit something nearer

	// if < 0, we are in a leaf node
	if (num < 0)
	{
		CM_TraceToLeaf (-1-num);
		return;
	}

	//
	// find the point distances to the seperating plane
	// and the offset for the size of the box
	//
	node = mod->nodes + num;
	plane = node->plane;

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
		offset = trace_extents[plane->type];
	}
	else
	{
		t1 = DotProduct (plane->normal, p1) - plane->dist;
		t2 = DotProduct (plane->normal, p2) - plane->dist;
		if (trace_ispoint)
			offset = 0;
		else
			offset = fabs(trace_extents[0]*plane->normal[0]) +
				fabs(trace_extents[1]*plane->normal[1]) +
				fabs(trace_extents[2]*plane->normal[2]);
	}


#if 0
CM_RecursiveHullCheck (node->childnum[0], p1f, p2f, p1, p2);
CM_RecursiveHullCheck (node->childnum[1], p1f, p2f, p1, p2);
return;
#endif

	// see which sides we need to consider
	if (t1 >= offset && t2 >= offset)
	{
		CM_RecursiveHullCheck (mod, node->childnum[0], p1f, p2f, p1, p2);
		return;
	}
	if (t1 < -offset && t2 < -offset)
	{
		CM_RecursiveHullCheck (mod, node->childnum[1], p1f, p2f, p1, p2);
		return;
	}

	// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < t2)
	{
		idist = 1.0/(t1-t2);
		side = 1;
		frac2 = (t1 + offset + DIST_EPSILON)*idist;
		frac = (t1 - offset + DIST_EPSILON)*idist;
	}
	else if (t1 > t2)
	{
		idist = 1.0/(t1-t2);
		side = 0;
		frac2 = (t1 - offset - DIST_EPSILON)*idist;
		frac = (t1 + offset + DIST_EPSILON)*idist;
	}
	else
	{
		side = 0;
		frac = 1;
		frac2 = 0;
	}

	// move up to the node
	if (frac < 0)
		frac = 0;
	if (frac > 1)
		frac = 1;

	midf = p1f + (p2f - p1f)*frac;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac*(p2[i] - p1[i]);

	CM_RecursiveHullCheck (mod, node->childnum[side], p1f, midf, p1, mid);


	// go past the node
	if (frac2 < 0)
		frac2 = 0;
	if (frac2 > 1)
		frac2 = 1;

	midf = p1f + (p2f - p1f)*frac2;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac2*(p2[i] - p1[i]);

	CM_RecursiveHullCheck (mod, node->childnum[side^1], midf, p2f, mid, p2);
}


//======================================================================

/*
==================
CM_BoxTrace
==================
*/
trace_t		CM_BoxTrace (model_t *mod, vec3_t start, vec3_t end,
						  vec3_t mins, vec3_t maxs,
						  int brushmask)
{
	int		i;
#if ADJ
	int moved;
#endif
	vec3_t point;


	checkcount++;		// for multi-check avoidance

	c_traces++;			// for statistics, may be zeroed

	// fill in a default trace
	memset (&trace_trace, 0, sizeof(trace_trace));
	trace_truefraction = 1;
	trace_nearfraction = 1;
	trace_trace.fraction = 1;
	trace_trace.surface = &(nullsurface.c);

	if (!mod)	// map not loaded
		return trace_trace;

	trace_contents = brushmask;
	VectorCopy (start, trace_start);
	VectorCopy (end, trace_end);
	VectorCopy (mins, trace_mins);
	VectorCopy (maxs, trace_maxs);

	// build a bounding box of the entire move (for patches)
	ClearBounds (trace_absmins, trace_absmaxs);
	VectorAdd (start, trace_mins, point);
	AddPointToBounds (point, trace_absmins, trace_absmaxs);
	VectorAdd (start, trace_maxs, point);
	AddPointToBounds (point, trace_absmins, trace_absmaxs);
	VectorAdd (end, trace_mins, point);
	AddPointToBounds (point, trace_absmins, trace_absmaxs);
	VectorAdd (end, trace_maxs, point);
	AddPointToBounds (point, trace_absmins, trace_absmaxs);

	//
	// check for position test special case
	//
	if (start[0] == end[0] && start[1] == end[1] && start[2] == end[2])
	{
		int		leafs[1024];
		int		i, numleafs;
		vec3_t	c1, c2;
		int		topnode;
#if ADJ
		if (-mins[2] != maxs[2])	//be prepared to move the thing up to counter the different min/max
		{
			moved = (trace_maxs[2] - trace_mins[2])/2;
			trace_mins[2] = -moved;
			trace_maxs[2] = moved;
			trace_extents[2] = -trace_mins[2] > trace_maxs[2] ? -trace_mins[2] : trace_maxs[2];
			moved = (maxs[2] - trace_maxs[2]);
		}

		trace_start[2]+=moved;
		trace_end[2]+=moved;
#endif



		VectorAdd (start, mins, c1);
		VectorAdd (start, maxs, c2);
		for (i=0 ; i<3 ; i++)
		{
			c1[i] -= 1;
			c2[i] += 1;
		}

		numleafs = CM_BoxLeafnums_headnode (mod, c1, c2, leafs, sizeof(leafs)/sizeof(leafs[0]), mod->hulls[0].firstclipnode, &topnode);
		for (i=0 ; i<numleafs ; i++)
		{
			CM_TestInLeaf (leafs[i]);
			if (trace_trace.allsolid)
				break;
		}
		VectorCopy (start, trace_trace.endpos);
#if ADJ
		trace_trace.endpos[2] -= moved;
#endif
		return trace_trace;
	}
#if ADJ
	moved = 0;
#endif
	//
	// check for point special case
	//
	if (trace_mins[0] == 0 && trace_mins[1] == 0 && trace_mins[2] == 0
		&& trace_maxs[0] == 0 && trace_maxs[1] == 0 && trace_maxs[2] == 0)
	{
		trace_ispoint = true;
		VectorClear (trace_extents);
	}
	else
	{
		trace_ispoint = false;
		trace_extents[0] = -trace_mins[0] > trace_maxs[0] ? -trace_mins[0] : trace_maxs[0]+1;
		trace_extents[1] = -trace_mins[1] > trace_maxs[1] ? -trace_mins[1] : trace_maxs[1]+1;
		trace_extents[2] = -trace_mins[2] > trace_maxs[2] ? -trace_mins[2] : trace_maxs[2]+1;
#if ADJ
		if (-mins[2] != maxs[2])	//be prepared to move the thing up to counter the different min/max
		{
			moved = (trace_maxs[2] - trace_mins[2])/2;
			trace_mins[2] = -moved;
			trace_maxs[2] = moved;
			trace_extents[2] = -trace_mins[2] > trace_maxs[2] ? -trace_mins[2] : trace_maxs[2];
			moved = (maxs[2] - trace_maxs[2]);
		}

		trace_start[2]+=moved;
		trace_end[2]+=moved;
#endif
	}

	//
	// general sweeping through world
	//
	CM_RecursiveHullCheck (mod, mod->hulls[0].firstclipnode, 0, 1, trace_start, trace_end);

	if (trace_nearfraction == 1)
	{
		trace_trace.fraction = 1;
		VectorCopy (trace_end, trace_trace.endpos);
	}
	else
	{
		if (trace_nearfraction<0)
			trace_nearfraction=0;
		trace_trace.fraction = trace_nearfraction;
		for (i=0 ; i<3 ; i++)
			trace_trace.endpos[i] = trace_start[i] + trace_trace.fraction * (trace_end[i] - trace_start[i]);
	}
#if ADJ
	trace_trace.endpos[2] -= moved;
#endif
	return trace_trace;
}

qboolean CM_Trace(model_t *model, int forcehullnum, int frame, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, trace_t *trace)
{
	*trace = CM_BoxTrace(model, start, end, mins, maxs, MASK_PLAYERSOLID);
	return trace->fraction != 1;
}
qboolean CM_NativeTrace(model_t *model, int forcehullnum, int frame, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, unsigned int contents, trace_t *trace)
{
	*trace = CM_BoxTrace(model, start, end, mins, maxs, contents);
	return trace->fraction != 1;
}

/*
==================
CM_TransformedBoxTrace

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
#ifdef _MSC_VER
#pragma optimize( "", off )
#endif


trace_t		CM_TransformedBoxTrace (model_t *mod, vec3_t start, vec3_t end,
						  vec3_t mins, vec3_t maxs,
						  int brushmask,
						  vec3_t origin, vec3_t angles)
{
	trace_t		trace;
	vec3_t		start_l, end_l;
	vec3_t		a;
	vec3_t		forward, right, up;
	vec3_t		temp;
	qboolean	rotated;

	// subtract origin offset
	VectorSubtract (start, origin, start_l);
	VectorSubtract (end, origin, end_l);

	// rotate start and end into the models frame of reference
	if (mod != &box_model &&
	(angles[0] || angles[1] || angles[2]) )
		rotated = true;
	else
		rotated = false;

	if (rotated)
	{
		AngleVectors (angles, forward, right, up);

		VectorCopy (start_l, temp);
		start_l[0] = DotProduct (temp, forward);
		start_l[1] = -DotProduct (temp, right);
		start_l[2] = DotProduct (temp, up);

		VectorCopy (end_l, temp);
		end_l[0] = DotProduct (temp, forward);
		end_l[1] = -DotProduct (temp, right);
		end_l[2] = DotProduct (temp, up);
	}

	// sweep the box through the model
	trace = CM_BoxTrace (mod, start_l, end_l, mins, maxs, brushmask);

	if (rotated && trace.fraction != 1.0)
	{
		// FIXME: figure out how to do this with existing angles
		VectorNegate (angles, a);
		AngleVectors (a, forward, right, up);

		VectorCopy (trace.plane.normal, temp);
		trace.plane.normal[0] = DotProduct (temp, forward);
		trace.plane.normal[1] = -DotProduct (temp, right);
		trace.plane.normal[2] = DotProduct (temp, up);
	}

	if (trace.fraction == 1)
	{
		VectorCopy(end, trace.endpos);
	}
	else
	{
		trace.endpos[0] = start[0] + trace.fraction * (end[0] - start[0]);
		trace.endpos[1] = start[1] + trace.fraction * (end[1] - start[1]);
		trace.endpos[2] = start[2] + trace.fraction * (end[2] - start[2]);
	}

	return trace;
}

#ifdef _MSC_VER
#pragma optimize( "", on )
#endif



/*
===============================================================================

PVS / PHS

===============================================================================
*/

/*
===================
CM_DecompressVis
===================
*/

/*
qbyte *Mod_Q2DecompressVis (qbyte *in, model_t *model)
{
	static qbyte	decompressed[MAX_MAP_LEAFS/8];
	int		c;
	qbyte	*out;
	int		row;

	row = (model->vis->numclusters+7)>>3;
	out = decompressed;

	if (!in)
	{	// no vis info, so make all visible
		while (row)
		{
			*out++ = 0xff;
			row--;
		}
		return decompressed;
	}

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);

	return decompressed;
}

#define	DVIS_PVS	0
#define	DVIS_PHS	1
qbyte *Mod_ClusterPVS (int cluster, model_t *model)
{
	if (cluster == -1 || !model->vis)
		return mod_novis;
	return Mod_Q2DecompressVis ( (qbyte *)model->vis + model->vis->bitofs[cluster][DVIS_PVS],
		model);
}
*/
void CM_DecompressVis (qbyte *in, qbyte *out)
{
	int		c;
	qbyte	*out_p;
	int		row;

	row = (numclusters+7)>>3;
	out_p = out;

	if (!in || !numvisibility)
	{	// no vis info, so make all visible
		while (row)
		{
			*out_p++ = 0xff;
			row--;
		}
		return;
	}

	do
	{
		if (*in)
		{
			*out_p++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		if ((out_p - out) + c > row)
		{
			c = row - (out_p - out);
			Con_DPrintf ("warning: Vis decompression overrun\n");
		}
		while (c)
		{
			*out_p++ = 0;
			c--;
		}
	} while (out_p - out < row);
}

qbyte	pvsrow[MAX_MAP_LEAFS/8];
qbyte	phsrow[MAX_MAP_LEAFS/8];



qbyte	*CM_ClusterPVS (model_t *mod, int cluster, qbyte *buffer)
{
	if (!buffer)
		buffer = pvsrow;

	if (mapisq3)
	{
		if (cluster != -1 && map_q3pvs->numclusters)
		{
			return (qbyte *)map_q3pvs->data + cluster * map_q3pvs->rowsize;
		}
		else
		{
			memset (buffer, 0, (numclusters+7)>>3);
			return buffer;
		}
	}

	if (cluster == -1)
		memset (buffer, 0, (numclusters+7)>>3);
	else
		CM_DecompressVis (map_visibility + map_q2vis->bitofs[cluster][DVIS_PVS], buffer);
	return buffer;
}

qbyte	*CM_ClusterPHS (model_t *mod, int cluster)
{
	if (mapisq3)	//phs not working yet.
	{
		if (cluster != -1 && map_q3phs->numclusters)
		{
			return (qbyte *)map_q3phs->data + cluster * map_q3phs->rowsize;
		}
		else
		{
			memset (phsrow, 0, (numclusters+7)>>3);
			return phsrow;
		}
	}

	if (cluster == -1)
		memset (phsrow, 0, (numclusters+7)>>3);
	else
		CM_DecompressVis (map_visibility + map_q2vis->bitofs[cluster][DVIS_PHS], phsrow);
	return phsrow;
}


/*
===============================================================================

AREAPORTALS

===============================================================================
*/

void FloodArea_r (q2carea_t *area, int floodnum)
{
	int		i;
	q2dareaportal_t	*p;

	if (area->floodvalid == floodvalid)
	{
		if (area->floodnum == floodnum)
			return;
		Host_Error ("FloodArea_r: reflooded");
	}

	area->floodnum = floodnum;
	area->floodvalid = floodvalid;
	p = &map_areaportals[area->firstareaportal];
	for (i=0 ; i<area->numareaportals ; i++, p++)
	{
		if (portalopen[p->portalnum])
			FloodArea_r (&map_q2areas[p->otherarea], floodnum);
	}
}

/*
====================
FloodAreaConnections


====================
*/
void	FloodAreaConnections (void)
{
	int		i, j;
	q2carea_t	*area;
	int		floodnum;

	if (mapisq3)
	{
		// area 0 is not used
		for (i=1 ; i<numareas ; i++)
		{
			for (  j = 1; j < numareas; j++ ) {
				map_q3areas[i].numareaportals[j] = ( j == i );
			}
		}
		return;
	}

	// all current floods are now invalid
	floodvalid++;
	floodnum = 0;

	// area 0 is not used
	for (i=1 ; i<numareas ; i++)
	{
		area = &map_q2areas[i];
		if (area->floodvalid == floodvalid)
			continue;		// already flooded into
		floodnum++;
		FloodArea_r (area, floodnum);
	}

}

void	VARGS CMQ2_SetAreaPortalState (int portalnum, qboolean open)
{
	if (mapisq3)
		Host_Error ("CMQ2_SetAreaPortalState on q3 map");
	if (portalnum > numareaportals)
		Host_Error ("areaportal > numareaportals");

	if (portalopen[portalnum] == open)
		return;
	portalopen[portalnum] = open;
	FloodAreaConnections ();

	return;
}

void	CMQ3_SetAreaPortalState (int area1, int area2, qboolean open)
{
	if (!mapisq3)
		return;
//		Host_Error ("CMQ3_SetAreaPortalState on non-q3 map");

	if (area1 > numareas || area2 > numareas)
		Host_Error ("CMQ3_SetAreaPortalState: area > numareas");

	if (open)
	{
		map_q3areas[area1].numareaportals[area2]++;
		map_q3areas[area2].numareaportals[area1]++;
	}
	else
	{
		map_q3areas[area1].numareaportals[area2]--;
		map_q3areas[area2].numareaportals[area1]--;
	}
}

qboolean	VARGS CM_AreasConnected (model_t *mod, int area1, int area2)
{
	if (map_noareas.value)
		return true;

	if (area1 > numareas || area2 > numareas)
		Host_Error ("area > numareas");

	if (mapisq3)
	{
		int		i;
		for (i=1 ; i<numareas ; i++)
		{
			if ( map_q3areas[i].numareaportals[area1] &&
				map_q3areas[i].numareaportals[area2] )
				return true;
		}
	}
	else
	{
		if (map_q2areas[area1].floodnum == map_q2areas[area2].floodnum)
			return true;
	}
	return false;
}


/*
=================
CM_WriteAreaBits

Writes a length qbyte followed by a bit vector of all the areas
that area in the same flood as the area parameter

This is used by the client refreshes to cull visibility
=================
*/
int CM_WriteAreaBits (model_t *mod, qbyte *buffer, int area)
{
	int		i;
	int		floodnum;
	int		bytes;

	bytes = (numareas+7)>>3;

	if (map_noareas.value)
	{	// for debugging, send everything
		memset (buffer, 255, bytes);
	}
	else
	{
		memset (buffer, 0, bytes);

		if (mapisq3)
		{
			for (i=0 ; i<numareas ; i++)
			{
				if (!area || CM_AreasConnected (mod, i, area ) || i == area)
					buffer[i>>3] |= 1<<(i&7);
			}
		}
		else
		{
			floodnum = map_q2areas[area].floodnum;
			for (i=0 ; i<numareas ; i++)
			{
				if (map_q2areas[i].floodnum == floodnum || !area)
					buffer[i>>3] |= 1<<(i&7);
			}
		}
	}

	return bytes;
}


/*
===================
CM_WritePortalState

Writes the portal state to a savegame file
===================
*/
void	CM_WritePortalState (FILE *f)
{
	fwrite (portalopen, sizeof(portalopen), 1, f);
}

/*
===================
CM_ReadPortalState

Reads the portal state from a savegame file
and recalculates the area connections
===================
*/
void	CM_ReadPortalState (FILE *f)
{
	fread (portalopen, 1, sizeof(portalopen), f);
	FloodAreaConnections ();
}

/*
=============
CM_HeadnodeVisible

Returns true if any leaf under headnode has a cluster that
is potentially visible
=============
*/
qboolean CM_HeadnodeVisible (model_t *mod, int nodenum, qbyte *visbits)
{
	int		leafnum;
	int		cluster;
	mnode_t	*node;

	if (nodenum < 0)
	{
		leafnum = -1-nodenum;
		cluster = map_leafs[leafnum].cluster;
		if (cluster == -1)
			return false;
		if (visbits[cluster>>3] & (1<<(cluster&7)))
			return true;
		return false;
	}

	node = &mod->nodes[nodenum];
	if (CM_HeadnodeVisible(mod, node->childnum[0], visbits))
		return true;
	return CM_HeadnodeVisible(mod, node->childnum[1], visbits);
}

/*
qboolean Q2BSP_RecursiveHullCheck (hull_t *hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_t *trace)
{
	trace_t ret = CM_BoxTrace(p1, p2, hull->clip_mins, hull->clip_maxs, hull->firstclipnode, MASK_SOLID);
	memcpy(trace, &ret, sizeof(trace_t));
	if (ret.fraction==1)
		return true;
	return false;
}*/
unsigned int Q2BSP_PointContents(model_t *mod, vec3_t p)
{
	int pc, ret = FTECONTENTS_EMPTY;
	pc = CM_PointContents (mod, p);
	if (pc & (Q2CONTENTS_SOLID|Q2CONTENTS_WINDOW))
		ret |= FTECONTENTS_SOLID;
	if (pc & Q2CONTENTS_LAVA)
		ret |= FTECONTENTS_LAVA;
	if (pc & Q2CONTENTS_SLIME)
		ret |= FTECONTENTS_SLIME;
	if (pc & Q2CONTENTS_WATER)
		ret |= FTECONTENTS_WATER;
	if (pc & Q2CONTENTS_LADDER)
		ret |= FTECONTENTS_LADDER;

	return ret;
}
void Q2BSP_SetHullFuncs(hull_t *hull)
{
//	hull->funcs.HullPointContents = Q2BSP_HullPointContents;
}








int map_checksum;
qboolean Mod_LoadQ2BrushModel (model_t *mod, void *buffer)
{
	mod->fromgame = fg_quake2;
	mod->engineflags |= MDLF_NEEDOVERBRIGHT;
	return CM_LoadMap(mod->name, buffer, true, &map_checksum) != NULL;
}

void CM_Init(void)	//register cvars.
{
#define MAPOPTIONS "Map Cvar Options"
	Cvar_Register(&map_noareas, MAPOPTIONS);
	Cvar_Register(&map_noCurves, MAPOPTIONS);
	Cvar_Register(&map_autoopenportals, MAPOPTIONS);
	Cvar_Register(&r_subdivisions, MAPOPTIONS);
}
#endif
