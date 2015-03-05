#include "quakedef.h"

#include "pr_common.h"

/*

============================================================================

Physics functions (common)
*/

void Q1BSP_CheckHullNodes(hull_t *hull)
{
	int num, c;
	mclipnode_t	*node;
	for (num = hull->firstclipnode; num < hull->lastclipnode; num++)
	{
		node = hull->clipnodes + num;
		for (c = 0; c < 2; c++)
			if (node->children[c] >= 0)
				if (node->children[c] < hull->firstclipnode || node->children[c] > hull->lastclipnode)
					Sys_Error ("Q1BSP_CheckHull: bad node number");

	}
}

/*
==================
SV_HullPointContents

==================
*/
static int Q1_HullPointContents (hull_t *hull, int num, vec3_t p)
{
	float		d;
	mclipnode_t	*node;
	mplane_t	*plane;

	while (num >= 0)
	{
		node = hull->clipnodes + num;
		plane = hull->planes + node->planenum;

		if (plane->type < 3)
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct (plane->normal, p) - plane->dist;
		if (d < 0)
			num = node->children[1];
		else
			num = node->children[0];
	}

	return num;
}

static int Q1_ModelPointContents (mnode_t *node, vec3_t p)
{
	float d;
	mplane_t *plane;
	while(node->contents >= 0)
	{
		plane = node->plane;
		if (plane->type < 3)
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct(plane->normal, p) - plane->dist;
		node = node->children[d<0];
	}
	return node->contents;
}



#define	DIST_EPSILON	(0.03125)
#if 0
enum
{
	rht_solid,
	rht_empty,
	rht_impact
};
vec3_t rht_start, rht_end;
static int Q1BSP_RecursiveHullTrace (hull_t *hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_t *trace)
{
	mclipnode_t	*node;
	mplane_t	*plane;
	float		t1, t2;
	vec3_t		mid;
	int			side;
	float		midf;
	int rht;

reenter:

	if (num < 0)
	{
		/*hit a leaf*/
		if (num == Q1CONTENTS_SOLID)
		{
			if (trace->allsolid)
				trace->startsolid = true;
			return rht_solid;
		}
		else
		{
			trace->allsolid = false;
			if (num == Q1CONTENTS_EMPTY)
				trace->inopen = true;
			else
				trace->inwater = true;
			return rht_empty;
		}
	}

	/*its a node*/

	/*get the node info*/
	node = hull->clipnodes + num;
	plane = hull->planes + node->planenum;

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
	}
	else
	{
		t1 = DotProduct (plane->normal, p1) - plane->dist;
		t2 = DotProduct (plane->normal, p2) - plane->dist;
	}

	/*if its completely on one side, resume on that side*/
	if (t1 >= 0 && t2 >= 0)
	{
		//return Q1BSP_RecursiveHullTrace (hull, node->children[0], p1f, p2f, p1, p2, trace);
		num = node->children[0];
		goto reenter;
	}
	if (t1 < 0 && t2 < 0)
	{
		//return Q1BSP_RecursiveHullTrace (hull, node->children[1], p1f, p2f, p1, p2, trace);
		num = node->children[1];
		goto reenter;
	}

	if (plane->type < 3)
	{
		t1 = rht_start[plane->type] - plane->dist;
		t2 = rht_end[plane->type] - plane->dist;
	}
	else
	{
		t1 = DotProduct (plane->normal, rht_start) - plane->dist;
		t2 = DotProduct (plane->normal, rht_end) - plane->dist;
	}

	side = t1 < 0;

	midf = t1 / (t1 - t2);
	if (midf < p1f) midf = p1f;
	if (midf > p2f) midf = p2f;
	VectorInterpolate(rht_start, midf, rht_end, mid);

	rht = Q1BSP_RecursiveHullTrace(hull, node->children[side], p1f, midf, p1, mid, trace);
	if (rht != rht_empty)
		return rht;
	rht = Q1BSP_RecursiveHullTrace(hull, node->children[side^1], midf, p2f, mid, p2, trace);
	if (rht != rht_solid)
		return rht;

	if (side)
	{
		/*we impacted the back of the node, so flip the plane*/
		trace->plane.dist = -plane->dist;
		VectorNegate(plane->normal, trace->plane.normal);
		midf = (t1 + DIST_EPSILON) / (t1 - t2);
	}
	else
	{
		/*we impacted the front of the node*/
		trace->plane.dist = plane->dist;
		VectorCopy(plane->normal, trace->plane.normal);
		midf = (t1 - DIST_EPSILON) / (t1 - t2);
	}

	t1 = DotProduct (trace->plane.normal, rht_start) - trace->plane.dist;
	t2 = DotProduct (trace->plane.normal, rht_end) - trace->plane.dist;
	midf = (t1 - DIST_EPSILON) / (t1 - t2);


	trace->fraction = midf;
	VectorCopy (mid, trace->endpos);
	VectorInterpolate(rht_start, midf, rht_end, trace->endpos);

	return rht_impact;
}

qboolean Q1BSP_RecursiveHullCheck (hull_t *hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_t *trace)
{
	if (VectorEquals(p1, p2))
	{
		/*points cannot cross planes, so do it faster*/
		switch(Q1_HullPointContents(hull, num, p1))
		{
		case Q1CONTENTS_SOLID:
			trace->startsolid = true;
			break;
		case Q1CONTENTS_EMPTY:
			trace->allsolid = false;
			trace->inopen = true;
			break;
		default:
			trace->allsolid = false;
			trace->inwater = true;
			break;
		}
		return true;
	}
	else
	{
		VectorCopy(p1, rht_start);
		VectorCopy(p2, rht_end);
		return Q1BSP_RecursiveHullTrace(hull, num, p1f, p2f, p1, p2, trace) != rht_impact;
	}
}

#else
qboolean Q1BSP_RecursiveHullCheck (hull_t *hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_t *trace)
{
	mclipnode_t	*node;
	mplane_t	*plane;
	float		t1, t2;
	float		frac;
	int			i;
	vec3_t		mid;
	int			side;
	float		midf;

// check for empty
	if (num < 0)
	{
		if (num != Q1CONTENTS_SOLID)
		{
			trace->allsolid = false;
			if (num == Q1CONTENTS_EMPTY)
				trace->inopen = true;
			else
				trace->inwater = true;
		}
		else
			trace->startsolid = true;
		return true;		// empty
	}

//
// find the point distances
//
	node = hull->clipnodes + num;
	plane = hull->planes + node->planenum;

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
	}
	else
	{
		t1 = DotProduct (plane->normal, p1) - plane->dist;
		t2 = DotProduct (plane->normal, p2) - plane->dist;
	}

#if 1
	if (t1 >= 0 && t2 >= 0)
		return Q1BSP_RecursiveHullCheck (hull, node->children[0], p1f, p2f, p1, p2, trace);
	if (t1 < 0 && t2 < 0)
		return Q1BSP_RecursiveHullCheck (hull, node->children[1], p1f, p2f, p1, p2, trace);
#else
	if ( (t1 >= DIST_EPSILON && t2 >= DIST_EPSILON) || (t2 > t1 && t1 >= 0) )
		return Q1BSP_RecursiveHullCheck (hull, node->children[0], p1f, p2f, p1, p2, trace);
	if ( (t1 <= -DIST_EPSILON && t2 <= -DIST_EPSILON) || (t2 < t1 && t1 <= 0) )
		return Q1BSP_RecursiveHullCheck (hull, node->children[1], p1f, p2f, p1, p2, trace);
#endif

// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < 0)
		frac = (t1 + DIST_EPSILON)/(t1-t2);
	else
		frac = (t1 - DIST_EPSILON)/(t1-t2);
	if (frac < 0)
		frac = 0;
	if (frac > 1)
		frac = 1;

	midf = p1f + (p2f - p1f)*frac;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac*(p2[i] - p1[i]);

	side = (t1 < 0);

// move up to the node
	if (!Q1BSP_RecursiveHullCheck (hull, node->children[side], p1f, midf, p1, mid, trace) )
		return false;

#ifdef PARANOID
	if (Q1BSP_RecursiveHullCheck (sv_hullmodel, mid, node->children[side])
	== Q1CONTENTS_SOLID)
	{
		Con_Printf ("mid PointInHullSolid\n");
		return false;
	}
#endif

	if (Q1_HullPointContents (hull, node->children[side^1], mid)
	!= Q1CONTENTS_SOLID)
// go past the node
		return Q1BSP_RecursiveHullCheck (hull, node->children[side^1], midf, p2f, mid, p2, trace);

	if (trace->allsolid)
		return false;		// never got out of the solid area

//==================
// the other side of the node is solid, this is the impact point
//==================
	if (!side)
	{
		VectorCopy (plane->normal, trace->plane.normal);
		trace->plane.dist = plane->dist;
	}
	else
	{
		VectorNegate (plane->normal, trace->plane.normal);
		trace->plane.dist = -plane->dist;
	}

	while (Q1_HullPointContents (hull, hull->firstclipnode, mid)
	== Q1CONTENTS_SOLID)
	{ // shouldn't really happen, but does occasionally
		if (!(frac < 10000000) && !(frac > -10000000))
		{
			trace->fraction = 0;
			VectorClear (trace->endpos);
			Con_Printf ("nan in traceline\n");
			return false;
		}
		frac -= 0.1;
		if (frac < 0)
		{
			trace->fraction = midf;
			VectorCopy (mid, trace->endpos);
			Con_DPrintf ("backup past 0\n");
			return false;
		}
		midf = p1f + (p2f - p1f)*frac;
		for (i=0 ; i<3 ; i++)
			mid[i] = p1[i] + frac*(p2[i] - p1[i]);
	}

	trace->fraction = midf;
	VectorCopy (mid, trace->endpos);

	return false;
}
#endif


/*
the bsp tree we're walking through is the renderable hull
we need to trace a box through the world.
by its very nature, this will reach more nodes than we really want, and as we can follow a node sideways, the underlying bsp structure is no longer 100% reliable (meaning we cross planes that are entirely to one side, and follow its children too)
so all contents and solidity must come from the brushes and ONLY the brushes.
*/
struct traceinfo_s
{
	unsigned int solidcontents;
	trace_t trace;

	qboolean capsule;
	float radius;
	/*set even for sphere traces (used for bbox tests)*/
	vec3_t mins;
	vec3_t maxs;

	vec3_t start;
	vec3_t end;

	vec3_t	up;
	vec3_t	capsulesize;
	vec3_t	extents;
};

static void Q1BSP_ClipToBrushes(struct traceinfo_s *traceinfo, mbrush_t *brush)
{
	struct mbrushplane_s *plane;
	struct mbrushplane_s *enterplane;
	int i, j;
	vec3_t ofs;
	qboolean startout, endout;
	float d1,d2,dist,enterdist=0;
	float f, enterfrac, exitfrac;

	for (; brush; brush = brush->next)
	{
		/*ignore if its not solid to us*/
		if (!(traceinfo->solidcontents & brush->contents))
			continue;

		startout = false;
		endout = false;
		enterplane= NULL;
		enterfrac = -1;
		exitfrac = 10;
		for (i = brush->numplanes, plane = brush->planes; i; i--, plane++)
		{
			/*calculate the distance based upon the shape of the object we're tracing for*/
			if (traceinfo->capsule)
			{
				dist = DotProduct(traceinfo->up, plane->normal);
				dist = dist*(traceinfo->capsulesize[(dist<0)?1:2]) - traceinfo->capsulesize[0];
				dist = plane->dist - dist;

				//dist = plane->dist + traceinfo->radius;
			}
			else
			{
				for (j=0 ; j<3 ; j++)
				{
					if (plane->normal[j] < 0)
						ofs[j] = traceinfo->maxs[j];
					else
						ofs[j] = traceinfo->mins[j];
				}
				dist = DotProduct (ofs, plane->normal);
				dist = plane->dist - dist;
			}

			d1 = DotProduct (traceinfo->start, plane->normal) - dist;
			d2 = DotProduct (traceinfo->end, plane->normal) - dist;

			if (d1 >= 0)
				startout = true;
			if (d2 > 0)
				endout = true;

			//if we're fully outside any plane, then we cannot possibly enter the brush, skip to the next one
			if (d1 > 0 && d2 >= 0)
				goto nextbrush;

			//if we're fully inside the plane, then whatever is happening is not relevent for this plane
			if (d1 < 0 && d2 <= 0)
				continue;

			f = d1 / (d1-d2);
			if (d1 > d2)
			{
				//entered the brush. favour the furthest fraction to avoid extended edges (yay for convex shapes)
				if (enterfrac < f)
				{
					enterfrac = f;
					enterplane = plane;
					enterdist = dist;
				}
			}
			else
			{
				//left the brush, favour the nearest plane (smallest frac)
				if (exitfrac > f)
				{
					exitfrac = f;
				}
			}
		}

		if (!startout)
		{
			traceinfo->trace.startsolid = true;
			if (!endout)
				traceinfo->trace.allsolid = true;
			traceinfo->trace.contents |= brush->contents;
			return;
		}
		if (enterfrac != -1 && enterfrac < exitfrac)
		{
			//impact!
			if (enterfrac < traceinfo->trace.fraction)
			{
				traceinfo->trace.fraction = enterfrac;
				traceinfo->trace.plane.dist = enterdist;
				VectorCopy(enterplane->normal, traceinfo->trace.plane.normal);
				traceinfo->trace.contents = brush->contents;
			}
		}
nextbrush:
		;
	}
}
static void Q1BSP_InsertBrush(mnode_t *node, mbrush_t *brush, vec3_t bmins, vec3_t bmaxs)
{
	vec3_t near, far;
	float nd, fd;
	int i;
	while(1)
	{
		if (node->contents < 0) /*leaf, so no smaller node to put it in (I'd be surprised if it got this far)*/
		{
			brush->next = node->brushes;
			node->brushes = brush;
			return;
		}

		for (i = 0; i < 3; i++)
		{
			if (node->plane->normal[i] > 0)
			{
				near[i] = bmins[i];
				far[i] = bmaxs[i];
			}
			else
			{
				near[i] = bmaxs[i];
				far[i] = bmins[i];
			}
		}

		nd = DotProduct(node->plane->normal, near) - node->plane->dist;
		fd = DotProduct(node->plane->normal, far) - node->plane->dist;

		/*if its fully on either side, continue walking*/
		if (nd < 0 && fd < 0)
			node = node->children[1];
		else if (nd > 0 && fd > 0)
			node = node->children[0];
		else
		{
			/*plane crosses bbox, so insert here*/
			brush->next = node->brushes;
			node->brushes = brush;
			return;
		}
	}
}
static void Q1BSP_RecursiveBrushCheck (struct traceinfo_s *traceinfo, mnode_t *node, float p1f, float p2f, vec3_t p1, vec3_t p2)
{
	mplane_t	*plane;
	float		t1, t2;
	float		frac;
	int			i;
	vec3_t		mid;
	int			side;
	float		midf;
	float		offset;

	if (node->brushes)
	{
		Q1BSP_ClipToBrushes(traceinfo, node->brushes);
	}

	if (traceinfo->trace.fraction < p1f)
	{
		//already hit something closer than this node
		return;
	}

	if (node->contents < 0)
	{
		//we're in a leaf
		return;
	}

//
// find the point distances
//
	plane = node->plane;

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
		if (plane->normal[plane->type] < 0)
			offset = -traceinfo->mins[plane->type];
		else
			offset = traceinfo->maxs[plane->type];
	}
	else
	{
		t1 = DotProduct (plane->normal, p1) - plane->dist;
		t2 = DotProduct (plane->normal, p2) - plane->dist;
		offset = 0;
		for (i = 0; i < 3; i++)
		{
			if (plane->normal[i] < 0)
				offset += plane->normal[i] * -traceinfo->mins[i];
			else
				offset += plane->normal[i] * traceinfo->maxs[i];
		}
	}

	/*if we're fully on one side of the trace, go only down that side*/
	if (t1 >= offset && t2 >= offset)
	{
		Q1BSP_RecursiveBrushCheck (traceinfo, node->children[0], p1f, p2f, p1, p2);
		return;
	}
	if (t1 < -offset && t2 < -offset)
	{
		Q1BSP_RecursiveBrushCheck (traceinfo, node->children[1], p1f, p2f, p1, p2);
		return;
	}

// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < 0)
	{
		frac = (t1 + DIST_EPSILON)/(t1-t2);
		side = 1;
	}
	else
	{
		frac = (t1 - DIST_EPSILON)/(t1-t2);
		side = 0;
	}
	if (frac < 0)
		frac = 0;
	if (frac > 1)
		frac = 1;

	midf = p1f + (p2f - p1f)*frac;
	for (i=0 ; i<3 ; i++)
		mid[i] = p1[i] + frac*(p2[i] - p1[i]);

// move up to the node
	Q1BSP_RecursiveBrushCheck (traceinfo, node->children[side], p1f, midf, p1, mid);

// go past the node
	Q1BSP_RecursiveBrushCheck (traceinfo, node->children[side^1], midf, p2f, mid, p2);
}

static unsigned int Q1BSP_TranslateContents(int contents)
{
	switch(contents)
	{
	case Q1CONTENTS_EMPTY:
		return FTECONTENTS_EMPTY;
	case Q1CONTENTS_SOLID:
		return FTECONTENTS_SOLID;
	case Q1CONTENTS_WATER:
		return FTECONTENTS_WATER;
	case Q1CONTENTS_SLIME:
		return FTECONTENTS_SLIME;
	case Q1CONTENTS_LAVA:
		return FTECONTENTS_LAVA;
	case Q1CONTENTS_SKY:
		return FTECONTENTS_SKY;
	case Q1CONTENTS_LADDER:
		return FTECONTENTS_LADDER;
	case Q1CONTENTS_CLIP:
		return FTECONTENTS_PLAYERCLIP;
	case Q1CONTENTS_TRANS:
		return FTECONTENTS_SOLID;

	//q2 is better than nothing, right?
	case Q1CONTENTS_FLOW_1:
		return Q2CONTENTS_CURRENT_0;
	case Q1CONTENTS_FLOW_2:
		return Q2CONTENTS_CURRENT_90;
	case Q1CONTENTS_FLOW_3:
		return Q2CONTENTS_CURRENT_180;
	case Q1CONTENTS_FLOW_4:
		return Q2CONTENTS_CURRENT_270;
	case Q1CONTENTS_FLOW_5:
		return Q2CONTENTS_CURRENT_UP;
	case Q1CONTENTS_FLOW_6:
		return Q2CONTENTS_CURRENT_DOWN;

	default:
		Con_Printf("Q1BSP_TranslateContents: Unknown contents type - %i", contents);
		return FTECONTENTS_SOLID;
	}
}

int Q1BSP_HullPointContents(hull_t *hull, vec3_t p)
{
	return Q1BSP_TranslateContents(Q1_HullPointContents(hull, hull->firstclipnode, p));
}

unsigned int Q1BSP_PointContents(model_t *model, vec3_t axis[3], vec3_t point)
{
	int contents;
	if (axis)
	{
		vec3_t transformed;
		transformed[0] = DotProduct(point, axis[0]);
		transformed[1] = DotProduct(point, axis[1]);
		transformed[2] = DotProduct(point, axis[2]);
		return Q1BSP_PointContents(model, NULL, transformed);
	}
	else
	{
		if (!model->firstmodelsurface)
		{
			contents = Q1BSP_TranslateContents(Q1_ModelPointContents(model->nodes, point));
		}
		else
			contents = Q1BSP_HullPointContents(&model->hulls[0], point);
	}
#ifdef TERRAIN
	if (model->terrain)
		contents |= Heightmap_PointContents(model, NULL, point);
#endif
	return contents;
}

void Q1BSP_LoadBrushes(model_t *model)
{
	struct {
		unsigned int ver;
		unsigned int modelnum;
		unsigned int numbrushes;
		unsigned int numplanes;
	} *permodel;
	struct {
		float mins[3];
		float maxs[3];
		signed short contents;
		unsigned short numplanes;
	} *perbrush;
	/*
	Note to implementors:
	a pointy brush with angles pointier than 90 degrees will extend further than any adjacent brush, thus creating invisible walls with larger expansions.
	the engine inserts 6 axial planes acording to the bbox, thus the qbsp need not write any axial planes
	note that doing it this way probably isn't good if you want to query textures...
	*/
	struct {
		vec3_t normal;
		float dist;
	} *perplane;

	static vec3_t axis[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
	int br, pl, remainingplanes;
	mbrush_t *brush;
	mnode_t *rootnode;
	unsigned int lumpsizeremaining;

	model->engineflags &= ~MDLF_HASBRUSHES;

	permodel = Q1BSPX_FindLump("BRUSHLIST", &lumpsizeremaining);
	if (!permodel)
		return;

	while (lumpsizeremaining)
	{
		if (lumpsizeremaining < sizeof(*permodel))
			return;
		permodel->ver = LittleLong(permodel->ver);
		permodel->modelnum = LittleLong(permodel->modelnum);
		permodel->numbrushes = LittleLong(permodel->numbrushes);
		permodel->numplanes = LittleLong(permodel->numplanes);
		if (permodel->ver != 1 || lumpsizeremaining < sizeof(*permodel) + permodel->numbrushes*sizeof(*perbrush) + permodel->numplanes*sizeof(*perplane))
			return;

		//find the correct rootnode for the model
		rootnode = model->nodes;
		if (permodel->modelnum > model->numsubmodels)
			return;
		if (permodel->modelnum)
			rootnode += model->submodels[permodel->modelnum-1].headnode[0];

		brush = ZG_Malloc(&model->memgroup, (sizeof(*brush) - sizeof(brush->planes[0]))*permodel->numbrushes + sizeof(brush->planes[0])*(permodel->numbrushes*6+permodel->numplanes));
		remainingplanes = permodel->numplanes;
		perbrush = (void*)(permodel+1);
		for (br = 0; br < permodel->numbrushes; br++)
		{
			/*byteswap it all in place*/
			perbrush->mins[0] = LittleFloat(perbrush->mins[0]);
			perbrush->mins[1] = LittleFloat(perbrush->mins[1]);
			perbrush->mins[2] = LittleFloat(perbrush->mins[2]);
			perbrush->maxs[0] = LittleFloat(perbrush->maxs[0]);
			perbrush->maxs[1] = LittleFloat(perbrush->maxs[1]);
			perbrush->maxs[2] = LittleFloat(perbrush->maxs[2]);
			perbrush->contents = LittleShort(perbrush->contents);
			perbrush->numplanes = LittleShort(perbrush->numplanes);

			/*make sure planes don't overflow*/
			if (perbrush->numplanes > remainingplanes)
				return;
			remainingplanes-=perbrush->numplanes;

			/*set up the mbrush from the file*/
			brush->contents = Q1BSP_TranslateContents(perbrush->contents);
			brush->numplanes = perbrush->numplanes;
			for (pl = 0, perplane = (void*)(perbrush+1); pl < perbrush->numplanes; pl++, perplane++)
			{
				brush->planes[pl].normal[0] = LittleFloat(perplane->normal[0]);
				brush->planes[pl].normal[1] = LittleFloat(perplane->normal[1]);
				brush->planes[pl].normal[2] = LittleFloat(perplane->normal[2]);
				brush->planes[pl].dist = LittleFloat(perplane->dist);
			}

			/*and add axial planes acording to the brush's bbox*/
			for (pl = 0; pl < 3; pl++)
			{
				VectorCopy(axis[pl], brush->planes[brush->numplanes].normal);
				brush->planes[brush->numplanes].dist = perbrush->maxs[pl];
				brush->numplanes++;
			}
			for (pl = 0; pl < 3; pl++)
			{
				VectorNegate(axis[pl], brush->planes[brush->numplanes].normal);
				brush->planes[brush->numplanes].dist = -perbrush->mins[pl];
				brush->numplanes++;
			}
			
			/*link it in to the bsp tree*/
			Q1BSP_InsertBrush(rootnode, brush, perbrush->mins, perbrush->maxs);

			/*set up for the next brush*/
			brush = (void*)&brush->planes[brush->numplanes];
			perbrush = (void*)perplane;
		}
		/*move on to the next model*/
		lumpsizeremaining -= sizeof(*permodel) + permodel->numbrushes*sizeof(*perbrush) + permodel->numplanes*sizeof(*perplane);
		permodel = (void*)((char*)permodel + sizeof(*permodel) + permodel->numbrushes*sizeof(*perbrush) + permodel->numplanes*sizeof(*perplane));
	}
	/*parsing was successful! flag it as okay*/
	model->engineflags |= MDLF_HASBRUSHES;
}

hull_t *Q1BSP_ChooseHull(model_t *model, int forcehullnum, vec3_t mins, vec3_t maxs, vec3_t offset)
{
	hull_t *hull;
	vec3_t size;
	VectorSubtract (maxs, mins, size);
	if (forcehullnum >= 1 && forcehullnum <= MAX_MAP_HULLSM && model->hulls[forcehullnum-1].available)
		hull = &model->hulls[forcehullnum-1];
	else
	{
		if (model->hulls[5].available)
		{	//choose based on hexen2 sizes.

			if (size[0] < 3) // Point
				hull = &model->hulls[0];
			else if (size[0] <= 8.1 && model->hulls[4].available)
				hull = &model->hulls[4];	//Pentacles
			else if (size[0] <= 32.1 && size[2] <= 28.1)  // Half Player
				hull = &model->hulls[3];
			else if (size[0] <= 32.1)  // Full Player
				hull = &model->hulls[1];
			else // Golumn
				hull = &model->hulls[5];
		}
		else
		{
			if (size[0] < 3 || !model->hulls[1].available)
				hull = &model->hulls[0];
			else if (size[0] <= 32.1)
			{
				if (size[2] < 54.1 && model->hulls[3].available)
					hull = &model->hulls[3]; // 32x32x36 (half-life's crouch)
				else
					hull = &model->hulls[1];
			}
			else
				hull = &model->hulls[2];
		}
	}

	VectorSubtract (hull->clip_mins, mins, offset);
	return hull;
}
qboolean Q1BSP_Trace(model_t *model, int forcehullnum, int frame, vec3_t axis[3], vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, qboolean capsule, unsigned int hitcontentsmask, trace_t *trace)
{
	hull_t *hull;
	vec3_t start_l, end_l;
	vec3_t offset;

	if ((model->engineflags & MDLF_HASBRUSHES))// && (size[0] || size[1] || size[2]))
	{
		struct traceinfo_s traceinfo;
		memset (&traceinfo.trace, 0, sizeof(trace_t));
		traceinfo.trace.fraction = 1;
		traceinfo.trace.allsolid = false;
		VectorCopy(mins, traceinfo.mins);
		VectorCopy(maxs, traceinfo.maxs);
		VectorCopy(start, traceinfo.start);
		VectorCopy(end, traceinfo.end);
		traceinfo.capsule = capsule;

		if (traceinfo.capsule)
		{
			float ext;
			traceinfo.capsulesize[0] = ((maxs[0]-mins[0]) + (maxs[1]-mins[1]))/4.0;
			traceinfo.capsulesize[1] = maxs[2];
			traceinfo.capsulesize[2] = mins[2];
			ext = (traceinfo.capsulesize[1] > -traceinfo.capsulesize[2])?traceinfo.capsulesize[1]:-traceinfo.capsulesize[2];
			traceinfo.capsulesize[1] -= traceinfo.capsulesize[0];
			traceinfo.capsulesize[2] += traceinfo.capsulesize[0];
			traceinfo.extents[0] = ext+1;
			traceinfo.extents[1] = ext+1;
			traceinfo.extents[2] = ext+1;
			VectorSet(traceinfo.up, 0, 0, 1);
		}

/*		traceinfo.sphere = true;
		traceinfo.radius = 48;
		traceinfo.mins[0] = -traceinfo.radius;
		traceinfo.mins[1] = -traceinfo.radius;
		traceinfo.mins[2] = -traceinfo.radius;
		traceinfo.maxs[0] = traceinfo.radius;
		traceinfo.maxs[1] = traceinfo.radius;
		traceinfo.maxs[2] = traceinfo.radius;
*/
		traceinfo.solidcontents = hitcontentsmask;
		Q1BSP_RecursiveBrushCheck(&traceinfo, model->nodes, 0, 1, start, end);
		memcpy(trace, &traceinfo.trace, sizeof(trace_t));
		if (trace->fraction < 1)
		{
			float d1 = DotProduct(start, trace->plane.normal) - trace->plane.dist;
			float d2 = DotProduct(end, trace->plane.normal) - trace->plane.dist;
			float f = (d1 - DIST_EPSILON) / (d1 - d2);
			if (f < 0)
				f = 0;
			trace->fraction = f;
		}
		VectorInterpolate(start, trace->fraction, end, trace->endpos);
		return trace->fraction != 1;
	}

	memset (trace, 0, sizeof(trace_t));
	trace->fraction = 1;
	trace->allsolid = true;

	hull = Q1BSP_ChooseHull(model, forcehullnum, mins, maxs, offset);

//	offset[0] = 0;
//	offset[1] = 0;

	if (axis)
	{
		vec3_t tmp;
		VectorSubtract(start, offset, tmp);
		start_l[0] = DotProduct(tmp, axis[0]);
		start_l[1] = DotProduct(tmp, axis[1]);
		start_l[2] = DotProduct(tmp, axis[2]);
		VectorSubtract(end, offset, tmp);
		end_l[0] = DotProduct(tmp, axis[0]);
		end_l[1] = DotProduct(tmp, axis[1]);
		end_l[2] = DotProduct(tmp, axis[2]);
		Q1BSP_RecursiveHullCheck(hull, hull->firstclipnode, 0, 1, start_l, end_l, trace);

		if (trace->fraction == 1)
		{
			VectorCopy (end, trace->endpos);
		}
		else
		{
			vec3_t iaxis[3];
			vec3_t norm;
			Matrix3x3_RM_Invert_Simple((void *)axis, iaxis);
			VectorCopy(trace->plane.normal, norm);
			trace->plane.normal[0] = DotProduct(norm, iaxis[0]);
			trace->plane.normal[1] = DotProduct(norm, iaxis[1]);
			trace->plane.normal[2] = DotProduct(norm, iaxis[2]);

			/*just interpolate it, its easier than inverse matrix rotations*/
			VectorInterpolate(start, trace->fraction, end, trace->endpos);
		}
	}
	else
	{
		VectorSubtract(start, offset, start_l);
		VectorSubtract(end, offset, end_l);
		Q1BSP_RecursiveHullCheck(hull, hull->firstclipnode, 0, 1, start_l, end_l, trace);

		if (trace->fraction == 1)
		{
			VectorCopy (end, trace->endpos);
		}
		else
		{
			VectorAdd (trace->endpos, offset, trace->endpos);
		}
	}

#ifdef TERRAIN
	if (model->terrain && trace->fraction)
	{
		trace_t hmt;
		Heightmap_Trace(model, forcehullnum, frame, axis, start, end, mins, maxs, capsule, hitcontentsmask, &hmt);
		if (hmt.fraction < trace->fraction)
			*trace = hmt;
	}
#endif

	return trace->fraction != 1;
}

/*
Physics functions (common)

============================================================================

Utility function
*/

#define MAXFRAGMENTVERTS 360
int Fragment_ClipPolyToPlane(float *inverts, float *outverts, int incount, float *plane, float planedist)
{
#define C 4
	float dotv[MAXFRAGMENTVERTS+1];
	char keep[MAXFRAGMENTVERTS+1];
#define KEEP_KILL 0
#define KEEP_KEEP 1
#define KEEP_BORDER 2
	int i;
	int outcount = 0;
	int clippedcount = 0;
	float d, *p1, *p2, *out;
#define FRAG_EPSILON 0.5

	for (i = 0; i < incount; i++)
	{
		dotv[i] = DotProduct((inverts+i*C), plane) - planedist;
		if (dotv[i]<-FRAG_EPSILON)
		{
			keep[i] = KEEP_KILL;
			clippedcount++;
		}
		else if (dotv[i] > FRAG_EPSILON)
			keep[i] = KEEP_KEEP;
		else
			keep[i] = KEEP_BORDER;
	}
	dotv[i] = dotv[0];
	keep[i] = keep[0];

	if (clippedcount == incount)
		return 0;	//all were clipped
	if (clippedcount == 0)
	{	//none were clipped
		for (i = 0; i < incount; i++)
			VectorCopy((inverts+i*C), (outverts+i*C));
		return incount;
	}

	for (i = 0; i < incount; i++)
	{
		p1 = inverts+i*C;
		if (keep[i] == KEEP_BORDER)
		{
			out = outverts+outcount++*C;
			VectorCopy(p1, out);
			continue;
		}
		if (keep[i] == KEEP_KEEP)
		{
			out = outverts+outcount++*C;
			VectorCopy(p1, out);
		}
		if (keep[i+1] == KEEP_BORDER || keep[i] == keep[i+1])
			continue;
		p2 = inverts+((i+1)%incount)*C;
		d = dotv[i] - dotv[i+1];
		if (d)
			d = dotv[i] / d;

		out = outverts+outcount++*C;
		VectorInterpolate(p1, d, p2, out);
	}
	return outcount;
}

/*
========================

Rendering functions (Client only)
*/
#ifndef SERVERONLY

extern int	r_dlightframecount;

//goes through the nodes marking the surfaces near the dynamic light as lit.
void Q1BSP_MarkLights (dlight_t *light, int bit, mnode_t *node)
{
	mplane_t	*splitplane;
	float		dist;
	msurface_t	*surf;
	int			i;

	float		l, maxdist;
	int			j, s, t;
	vec3_t		impact;

	if (node->contents < 0)
		return;

	splitplane = node->plane;
	if (splitplane->type < 3)
		dist = light->origin[splitplane->type] - splitplane->dist;
	else
		dist = DotProduct (light->origin, splitplane->normal) - splitplane->dist;

	if (dist > light->radius)
	{
		Q1BSP_MarkLights (light, bit, node->children[0]);
		return;
	}
	if (dist < -light->radius)
	{
		Q1BSP_MarkLights (light, bit, node->children[1]);
		return;
	}

	maxdist = light->radius*light->radius;

// mark the polygons
	surf = currentmodel->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		//Yeah, you can blame LordHavoc for this alternate code here.
		for (j=0 ; j<3 ; j++)
			impact[j] = light->origin[j] - surf->plane->normal[j]*dist;

		// clamp center of light to corner and check brightness
		l = DotProduct (impact, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3] - surf->texturemins[0];
		s = l+0.5;if (s < 0) s = 0;else if (s > surf->extents[0]) s = surf->extents[0];
		s = (l - s)*surf->texinfo->vecscale[0];
		l = DotProduct (impact, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3] - surf->texturemins[1];
		t = l+0.5;if (t < 0) t = 0;else if (t > surf->extents[1]) t = surf->extents[1];
		t = (l - t)*surf->texinfo->vecscale[1];
		// compare to minimum light
		if ((s*s+t*t+dist*dist) < maxdist)
		{
			if (surf->dlightframe != r_dlightframecount)
			{
				surf->dlightbits = bit;
				surf->dlightframe = r_dlightframecount;
			}
			else
				surf->dlightbits |= bit;
		}
	}

	Q1BSP_MarkLights (light, bit, node->children[0]);
	Q1BSP_MarkLights (light, bit, node->children[1]);
}

#define MAXFRAGMENTTRIS 256
vec3_t decalfragmentverts[MAXFRAGMENTTRIS*3];

struct fragmentdecal_s
{
	vec3_t center;

	vec3_t normal;
//	vec3_t tangent1;
//	vec3_t tangent2;

	vec3_t planenorm[6];
	float planedist[6];
	int numplanes;

	vec_t radius;

	void (*callback)(void *ctx, vec3_t *fte_restrict points, size_t numpoints, shader_t *shader);
	void *ctx;
};
typedef struct fragmentdecal_s fragmentdecal_t;

//#define SHOWCLIPS
//#define FRAGMENTASTRIANGLES	//works, but produces more fragments.

#ifdef FRAGMENTASTRIANGLES

//if the triangle is clipped away, go recursive if there are tris left.
static void Fragment_ClipTriToPlane(int trinum, float *plane, float planedist, fragmentdecal_t *dec)
{
	float *point[3];
	float dotv[3];

	vec3_t impact1, impact2;
	float t;

	int i, i2, i3;
	int clippedverts = 0;

	for (i = 0; i < 3; i++)
	{
		point[i] = decalfragmentverts[trinum*3+i];
		dotv[i] = DotProduct(point[i], plane)-planedist;
		clippedverts += dotv[i] < 0;
	}

	//if they're all clipped away, scrap the tri
	switch (clippedverts)
	{
	case 0:
		return;	//plane does not clip the triangle.

	case 1:	//split into 3, disregard the clipped vert
		for (i = 0; i < 3; i++)
		{
			if (dotv[i] < 0)
			{	//This is the vertex that's getting clipped.

				if (dotv[i] > -DIST_EPSILON)
					return;	//it's only over the line by a tiny ammount.

				i2 = (i+1)%3;
				i3 = (i+2)%3;

				if (dotv[i2] < DIST_EPSILON)
					return;
				if (dotv[i3] < DIST_EPSILON)
					return;

				//work out where the two lines impact the plane
				t = (dotv[i]) / (dotv[i]-dotv[i2]);
				VectorInterpolate(point[i], t, point[i2], impact1);

				t = (dotv[i]) / (dotv[i]-dotv[i3]);
				VectorInterpolate(point[i], t, point[i3], impact2);

#ifdef SHOWCLIPS
				if (dec->numtris != MAXFRAGMENTTRIS)
				{
					VectorCopy(impact2,					decalfragmentverts[dec->numtris*3+0]);
					VectorCopy(decalfragmentverts[trinum*3+i],	decalfragmentverts[dec->numtris*3+1]);
					VectorCopy(impact1,					decalfragmentverts[dec->numtris*3+2]);
					dec->numtris++;
				}
#endif


				//shrink the tri, putting the impact into the killed vertex.
				VectorCopy(impact2, point[i]);


				if (dec->numtris == MAXFRAGMENTTRIS)
					return;	//:(

				//build the second tri
				VectorCopy(impact1,					decalfragmentverts[dec->numtris*3+0]);
				VectorCopy(decalfragmentverts[trinum*3+i2],	decalfragmentverts[dec->numtris*3+1]);
				VectorCopy(impact2,					decalfragmentverts[dec->numtris*3+2]);
				dec->numtris++;

				return;
			}
		}
		Sys_Error("Fragment_ClipTriToPlane: Clipped vertex not founc\n");
		return;	//can't handle it
	case 2:	//split into 3, disregarding both the clipped.
		for (i = 0; i < 3; i++)
		{
			if (!(dotv[i] < 0))
			{	//This is the vertex that's staying.

				if (dotv[i] < DIST_EPSILON)
					break;	//only just inside

				i2 = (i+1)%3;
				i3 = (i+2)%3;

				//work out where the two lines impact the plane
				t = (dotv[i]) / (dotv[i]-dotv[i2]);
				VectorInterpolate(point[i], t, point[i2], impact1);

				t = (dotv[i]) / (dotv[i]-dotv[i3]);
				VectorInterpolate(point[i], t, point[i3], impact2);

				//shrink the tri, putting the impact into the killed vertex.

#ifdef SHOWCLIPS
				if (dec->numtris != MAXFRAGMENTTRIS)
				{
					VectorCopy(impact1,					decalfragmentverts[dec->numtris*3+0]);
					VectorCopy(point[i2],	decalfragmentverts[dec->numtris*3+1]);
					VectorCopy(point[i3],					decalfragmentverts[dec->numtris*3+2]);
					dec->numtris++;
				}
				if (dec->numtris != MAXFRAGMENTTRIS)
				{
					VectorCopy(impact1,					decalfragmentverts[dec->numtris*3+0]);
					VectorCopy(point[i3],	decalfragmentverts[dec->numtris*3+1]);
					VectorCopy(impact2,					decalfragmentverts[dec->numtris*3+2]);
					dec->numtris++;
				}
#endif

				VectorCopy(impact1, point[i2]);
				VectorCopy(impact2, point[i3]);
				return;
			}
		}
	case 3://scrap it
		//fill the verts with the verts of the last and go recursive (due to the nature of Fragment_ClipTriangle, which doesn't actually know if we clip them away)
#ifndef SHOWCLIPS
		dec->numtris--;
		VectorCopy(decalfragmentverts[dec->numtris*3+0], decalfragmentverts[trinum*3+0]);
		VectorCopy(decalfragmentverts[dec->numtris*3+1], decalfragmentverts[trinum*3+1]);
		VectorCopy(decalfragmentverts[dec->numtris*3+2], decalfragmentverts[trinum*3+2]);
		if (trinum < dec->numtris)
			Fragment_ClipTriToPlane(trinum, plane, planedist, dec);
#endif
		return;
	}
}

static void Fragment_ClipTriangle(fragmentdecal_t *dec, float *a, float *b, float *c)
{
	//emit the triangle, and clip it's fragments.
	int start, i;

	int p;

	if (dec->numtris == MAXFRAGMENTTRIS)
		return;	//:(

	start = dec->numtris;

	VectorCopy(a, decalfragmentverts[dec->numtris*3+0]);
	VectorCopy(b, decalfragmentverts[dec->numtris*3+1]);
	VectorCopy(c, decalfragmentverts[dec->numtris*3+2]);
	dec->numtris++;

	//clip all the fragments to all of the planes.
	//This will produce a quad if the source triangle was big enough.

	for (p = 0; p < 6; p++)
	{
		for (i = start; i < dec->numtris; i++)
			Fragment_ClipTriToPlane(i, dec->planenorm[p], dec->plantdist[p], dec);
	}
}

#else

void Fragment_ClipPoly(fragmentdecal_t *dec, int numverts, float *inverts, shader_t *surfshader)
{
	//emit the triangle, and clip it's fragments.
	int p;
	float verts[MAXFRAGMENTVERTS*C];
	float verts2[MAXFRAGMENTVERTS*C];
	float *cverts;
	int flip;
	vec3_t d1, d2, n;
	size_t numtris;

	if (numverts > MAXFRAGMENTTRIS)
		return;

	VectorSubtract(inverts+C*1, inverts+C*0, d1);
	VectorSubtract(inverts+C*2, inverts+C*0, d2);
	CrossProduct(d1, d2, n);
	VectorNormalizeFast(n);
//	if (DotProduct(n, dec->normal) > 0.1)
//		return;	//faces too far way from the normal

	//clip to the first plane specially, so we don't have extra copys
	numverts = Fragment_ClipPolyToPlane(inverts, verts, numverts, dec->planenorm[0], dec->planedist[0]);

	//clip the triangle to the 6 planes.
	flip = 0;
	for (p = 1; p < dec->numplanes; p++)
	{
		flip^=1;
		if (flip)
			numverts = Fragment_ClipPolyToPlane(verts, verts2, numverts, dec->planenorm[p], dec->planedist[p]);
		else
			numverts = Fragment_ClipPolyToPlane(verts2, verts, numverts, dec->planenorm[p], dec->planedist[p]);

		if (numverts < 3)	//totally clipped.
			return;
	}

	if (flip)
		cverts = verts2;
	else
		cverts = verts;

	//decompose the resultant polygon into triangles.

	numtris = 0;
	while(numverts-->2)
	{
		if (numtris == MAXFRAGMENTTRIS)
		{
			dec->callback(dec->ctx, decalfragmentverts, numtris, NULL);
			numtris = 0;
			break;
		}

		VectorCopy((cverts+C*0),			decalfragmentverts[numtris*3+0]);
		VectorCopy((cverts+C*(numverts-1)),	decalfragmentverts[numtris*3+1]);
		VectorCopy((cverts+C*numverts),		decalfragmentverts[numtris*3+2]);
		numtris++;
	}
	if (numtris)
		dec->callback(dec->ctx, decalfragmentverts, numtris, surfshader);
}

#endif

//this could be inlined, but I'm lazy.
static void Fragment_Mesh (fragmentdecal_t *dec, mesh_t *mesh, shader_t *surfshader)
{
	int i;

	vecV_t verts[3];

	/*if its a triangle fan/poly/quad then we can just submit the entire thing without generating extra fragments*/
	if (mesh->istrifan)
	{
		Fragment_ClipPoly(dec, mesh->numvertexes, mesh->xyz_array[0], surfshader);
		return;
	}

	//Fixme: optimise q3 patches (quad strips with bends between each strip)

	/*otherwise it goes in and out in weird places*/
	for (i = 0; i < mesh->numindexes; i+=3)
	{
		VectorCopy(mesh->xyz_array[mesh->indexes[i+0]], verts[0]);
		VectorCopy(mesh->xyz_array[mesh->indexes[i+1]], verts[1]);
		VectorCopy(mesh->xyz_array[mesh->indexes[i+2]], verts[2]);
		Fragment_ClipPoly(dec, 3, verts[0], surfshader);
	}
}

static void Q1BSP_ClipDecalToNodes (model_t *mod, fragmentdecal_t *dec, mnode_t *node)
{
	mplane_t	*splitplane;
	float		dist;
	msurface_t	*surf;
	int			i;

	if (node->contents < 0)
		return;

	splitplane = node->plane;
	dist = DotProduct (dec->center, splitplane->normal) - splitplane->dist;

	if (dist > dec->radius)
	{
		Q1BSP_ClipDecalToNodes (mod, dec, node->children[0]);
		return;
	}
	if (dist < -dec->radius)
	{
		Q1BSP_ClipDecalToNodes (mod, dec, node->children[1]);
		return;
	}

// mark the polygons
	surf = mod->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->flags & SURF_PLANEBACK)
		{
			if (-DotProduct(surf->plane->normal, dec->normal) > -0.5)
				continue;
		}
		else
		{
			if (DotProduct(surf->plane->normal, dec->normal) > -0.5)
				continue;
		}
		Fragment_Mesh(dec, surf->mesh, surf->texinfo->texture->shader);
	}

	Q1BSP_ClipDecalToNodes (mod, dec, node->children[0]);
	Q1BSP_ClipDecalToNodes (mod, dec, node->children[1]);
}

#ifdef RTLIGHTS
extern int sh_shadowframe;
#else
static int sh_shadowframe;
#endif
#ifdef Q3BSPS
static void Q3BSP_ClipDecalToNodes (fragmentdecal_t *dec, mnode_t *node)
{
	mplane_t	*splitplane;
	float		dist;
	msurface_t	**msurf;
	msurface_t	*surf;
	mleaf_t		*leaf;
	int			i;

	if (node->contents != -1)
	{
		leaf = (mleaf_t *)node;
	// mark the polygons
		msurf = leaf->firstmarksurface;
		for (i=0 ; i<leaf->nummarksurfaces ; i++, msurf++)
		{
			surf = *msurf;

			//only check each surface once. it can appear in multiple leafs.
			if (surf->shadowframe == sh_shadowframe)
				continue;
			surf->shadowframe = sh_shadowframe;

			Fragment_Mesh(dec, surf->mesh, surf->texinfo->texture->shader);
		}
		return;
	}

	splitplane = node->plane;
	dist = DotProduct (dec->center, splitplane->normal) - splitplane->dist;

	if (dist > dec->radius)
	{
		Q3BSP_ClipDecalToNodes (dec, node->children[0]);
		return;
	}
	if (dist < -dec->radius)
	{
		Q3BSP_ClipDecalToNodes (dec, node->children[1]);
		return;
	}
	Q3BSP_ClipDecalToNodes (dec, node->children[0]);
	Q3BSP_ClipDecalToNodes (dec, node->children[1]);
}
#endif

void Mod_ClipDecal(struct model_s *mod, vec3_t center, vec3_t normal, vec3_t tangent1, vec3_t tangent2, float size, void (*callback)(void *ctx, vec3_t *fte_restrict points, size_t numpoints, shader_t *shader), void *ctx)
{	//quad marks a full, independant quad
	int p;
	float r;
	fragmentdecal_t dec;

	VectorCopy(center, dec.center);
	VectorCopy(normal, dec.normal);
	dec.radius = 0;
	dec.callback = callback;
	dec.ctx = ctx;

	VectorCopy(tangent1,	dec.planenorm[0]);
	VectorNegate(tangent1,	dec.planenorm[1]);
	VectorCopy(tangent2,	dec.planenorm[2]);
	VectorNegate(tangent2,	dec.planenorm[3]);
	VectorCopy(dec.normal,		dec.planenorm[4]);
	VectorNegate(dec.normal,	dec.planenorm[5]);
	for (p = 0; p < 6; p++)
	{
		r = sqrt(DotProduct(dec.planenorm[p], dec.planenorm[p]));
		VectorScale(dec.planenorm[p], 1/r, dec.planenorm[p]);
		r*= size/2;
		if (r > dec.radius)
			dec.radius = r;
		dec.planedist[p] = -(r - DotProduct(dec.center, dec.planenorm[p]));
	}
	dec.numplanes = 6;

	sh_shadowframe++;

	if (!mod || mod->type != mod_brush)
	{
	}
	else if (mod->fromgame == fg_quake)
		Q1BSP_ClipDecalToNodes(mod, &dec, mod->nodes);
#ifdef Q3BSPS
	else if (cl.worldmodel->fromgame == fg_quake3)
		Q3BSP_ClipDecalToNodes(&dec, mod->nodes);
#endif

#ifdef TERRAIN
	if (cl.worldmodel && cl.worldmodel->terrain)
		Terrain_ClipDecal(&dec, center, dec.radius, mod);
#endif
}

#endif
/*
Rendering functions (Client only)

==============================================================================

Server only functions
*/
#ifndef CLIENTONLY

//does the recursive work of Q1BSP_FatPVS
static void SV_Q1BSP_AddToFatPVS (model_t *mod, vec3_t org, mnode_t *node, qbyte *buffer, unsigned int buffersize)
{
	int		i;
	qbyte	*pvs;
	mplane_t	*plane;
	float	d;

	while (1)
	{
	// if this is a leaf, accumulate the pvs bits
		if (node->contents < 0)
		{
			if (node->contents != Q1CONTENTS_SOLID)
			{
				pvs = Q1BSP_LeafPVS (mod, (mleaf_t *)node, NULL, 0);
				for (i=0; i<buffersize; i++)
					buffer[i] |= pvs[i];
			}
			return;
		}

		plane = node->plane;
		d = DotProduct (org, plane->normal) - plane->dist;
		if (d > 8)
			node = node->children[0];
		else if (d < -8)
			node = node->children[1];
		else
		{	// go down both
			SV_Q1BSP_AddToFatPVS (mod, org, node->children[0], buffer, buffersize);
			node = node->children[1];
		}
	}
}

/*
=============
Q1BSP_FatPVS

Calculates a PVS that is the inclusive or of all leafs within 8 pixels of the
given point.
=============
*/
static unsigned int Q1BSP_FatPVS (model_t *mod, vec3_t org, qbyte *pvsbuffer, unsigned int buffersize, qboolean add)
{
	unsigned int fatbytes = (mod->numleafs+31)>>3;
	if (fatbytes > buffersize)
		Sys_Error("map had too much pvs data (too many leaves)\n");;
	if (!add)
		Q_memset (pvsbuffer, 0, fatbytes);
	SV_Q1BSP_AddToFatPVS (mod, org, mod->nodes, pvsbuffer, fatbytes);
	return fatbytes;
}

#endif
static qboolean Q1BSP_EdictInFatPVS(model_t *mod, struct pvscache_s *ent, qbyte *pvs)
{
	int i;

	if (ent->num_leafs == MAX_ENT_LEAFS+1)
		return true;	//it's in too many leafs for us to cope with. Just trivially accept it.

	for (i=0 ; i < ent->num_leafs ; i++)
		if (pvs[ent->leafnums[i] >> 3] & (1 << (ent->leafnums[i]&7) ))
			return true;	//we might be able to see this one.

	return false;	//none of this ents leafs were visible, so neither is the ent.
}

/*
===============
SV_FindTouchedLeafs

Links the edict to the right leafs so we can get it's potential visability.
===============
*/
static void Q1BSP_RFindTouchedLeafs (model_t *wm, struct pvscache_s *ent, mnode_t *node, float *mins, float *maxs)
{
	mplane_t	*splitplane;
	mleaf_t		*leaf;
	int			sides;
	int			leafnum;

	if (node->contents == Q1CONTENTS_SOLID)
		return;

// add an efrag if the node is a leaf

	if ( node->contents < 0)
	{
		if (ent->num_leafs >= MAX_ENT_LEAFS)
		{
			ent->num_leafs = MAX_ENT_LEAFS+1;	//too many. mark it as such so we can trivially accept huge mega-big brush models.
			return;
		}

		leaf = (mleaf_t *)node;
		leafnum = leaf - wm->leafs - 1;

		ent->leafnums[ent->num_leafs] = leafnum;
		ent->num_leafs++;
		return;
	}

// NODE_MIXED

	splitplane = node->plane;
	sides = BOX_ON_PLANE_SIDE(mins, maxs, splitplane);

// recurse down the contacted sides
	if (sides & 1)
		Q1BSP_RFindTouchedLeafs (wm, ent, node->children[0], mins, maxs);

	if (sides & 2)
		Q1BSP_RFindTouchedLeafs (wm, ent, node->children[1], mins, maxs);
}
static void Q1BSP_FindTouchedLeafs(model_t *mod, struct pvscache_s *ent, float *mins, float *maxs)
{
	ent->num_leafs = 0;
	if (mins && maxs)
		Q1BSP_RFindTouchedLeafs (mod, ent, mod->nodes, mins, maxs);
}


/*
Server only functions

==============================================================================

PVS type stuff
*/

/*
===================
Mod_DecompressVis
===================
*/
static qbyte *Q1BSP_DecompressVis (qbyte *in, model_t *model, qbyte *decompressed, unsigned int buffersize)
{
	int		c;
	qbyte	*out;
	int		row;

	row = (model->numclusters+7)>>3;
	out = decompressed;

	if (buffersize < row)
		row = buffersize;

#if 0
	memcpy (out, in, row);
#else
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
#endif

	return decompressed;
}

static qbyte	mod_novis[MAX_MAP_LEAFS/8];

qbyte *Q1BSP_LeafPVS (model_t *model, mleaf_t *leaf, qbyte *buffer, unsigned int buffersize)
{
	static qbyte	decompressed[MAX_MAP_LEAFS/8];

	if (leaf == model->leafs)
		return mod_novis;

	if (!buffer)
	{
		buffer = decompressed;
		buffersize = sizeof(decompressed);
	}

	return Q1BSP_DecompressVis (leaf->compressed_vis, model, buffer, buffersize);
}

//pvs is 1-based. clusters are 0-based. otherwise, q1bsp has a 1:1 mapping.
static qbyte *Q1BSP_ClusterPVS (model_t *model, int cluster, qbyte *buffer, unsigned int buffersize)
{
	static qbyte	decompressed[MAX_MAP_LEAFS/8];

	if (cluster == -1)
		return mod_novis;
	cluster++;

	if (!buffer)
	{
		buffer = decompressed;
		buffersize = sizeof(decompressed);
	}

	return Q1BSP_DecompressVis (model->leafs[cluster].compressed_vis, model, buffer, buffersize);
}

//returns the leaf number, which is used as a bit index into the pvs.
static int Q1BSP_LeafnumForPoint (model_t *model, vec3_t p)
{
	mnode_t		*node;
	float		d;
	mplane_t	*plane;

	if (!model)
	{
		Sys_Error ("Mod_PointInLeaf: bad model");
	}
	if (!model->nodes)
		return 0;

	node = model->nodes;
	while (1)
	{
		if (node->contents < 0)
			return (mleaf_t *)node - model->leafs;
		plane = node->plane;
		d = DotProduct (p,plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return 0;	// never reached
}

mleaf_t *Q1BSP_LeafForPoint (model_t *model, vec3_t p)
{
	return model->leafs + Q1BSP_LeafnumForPoint(model, p);
}

//returns the leaf number, which is used as a direct bit index into the pvs.
//-1 for invalid
static int Q1BSP_ClusterForPoint (model_t *model, vec3_t p)
{
	mnode_t		*node;
	float		d;
	mplane_t	*plane;

	if (!model)
	{
		Sys_Error ("Mod_PointInLeaf: bad model");
	}
	if (!model->nodes)
		return -1;

	node = model->nodes;
	while (1)
	{
		if (node->contents < 0)
			return ((mleaf_t *)node - model->leafs) - 1;
		plane = node->plane;
		d = DotProduct (p,plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}

	return -1;	// never reached
}


/*
PVS type stuff

==============================================================================

Init stuff
*/


void Q1BSP_Init(void)
{
	memset (mod_novis, 0xff, sizeof(mod_novis));
}

typedef struct {
    char lumpname[24]; // up to 23 chars, zero-padded
    int fileofs;  // from file start
    int filelen;
} bspx_lump_t;
typedef struct {
    char id[4];  // 'BSPX'
    int numlumps;
	bspx_lump_t lumps[1];
} bspx_header_t;
static char *bspxbase;
static bspx_header_t *bspxheader;
//supported lumps:
//RGBLIGHTING (.lit)
//LIGHTINGDIR (.lux)
void *Q1BSPX_FindLump(char *lumpname, int *lumpsize)
{
	int i;
	*lumpsize = 0;
	if (!bspxheader)
		return NULL;

	for (i = 0; i < bspxheader->numlumps; i++)
	{
		if (!strncmp(bspxheader->lumps[i].lumpname, lumpname, 24))
		{
			*lumpsize = bspxheader->lumps[i].filelen;
			return bspxbase + bspxheader->lumps[i].fileofs;
		}
	}
	return NULL;
}
void Q1BSPX_Setup(model_t *mod, char *filebase, unsigned int filelen, lump_t *lumps, int numlumps)
{
	int i;
	int offs = 0;
	bspx_header_t *h;

	bspxbase = filebase;
	bspxheader = NULL;

	for (i = 0; i < numlumps; i++, lumps++)
	{
		if (offs < lumps->fileofs + lumps->filelen)
			offs = lumps->fileofs + lumps->filelen;
	}
	offs = (offs + 3) & ~3;
	if (offs + sizeof(*bspxheader) > filelen)
		return; /*no space for it*/
	h = (bspx_header_t*)(filebase + offs);

	i = LittleLong(h->numlumps);
	/*verify the header*/
	if (*(int*)h->id != *(int*)"BSPX" ||
		i < 0 ||
		offs + sizeof(*h) + sizeof(h->lumps[0])*(i-1) > filelen)
		return;
	h->numlumps = i;
	while(i-->0)
	{
		h->lumps[i].fileofs = LittleLong(h->lumps[i].fileofs);
		h->lumps[i].filelen = LittleLong(h->lumps[i].filelen);
		if (h->lumps[i].fileofs + h->lumps[i].filelen > filelen)
			return;
	}

	bspxheader = h;
}

//sets up the functions a server needs.
//fills in bspfuncs_t
void Q1BSP_SetModelFuncs(model_t *mod)
{
#ifndef CLIENTONLY
	mod->funcs.FatPVS				= Q1BSP_FatPVS;
#endif
	mod->funcs.EdictInFatPVS		= Q1BSP_EdictInFatPVS;
	mod->funcs.FindTouchedLeafs		= Q1BSP_FindTouchedLeafs;
	mod->funcs.LightPointValues		= NULL;
	mod->funcs.StainNode			= NULL;
	mod->funcs.MarkLights			= NULL;

	mod->funcs.ClusterForPoint		= Q1BSP_ClusterForPoint;
	mod->funcs.ClusterPVS			= Q1BSP_ClusterPVS;
	mod->funcs.NativeTrace			= Q1BSP_Trace;
	mod->funcs.PointContents		= Q1BSP_PointContents;
}
