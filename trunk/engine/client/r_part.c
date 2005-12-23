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

/*
The aim of this particle system is to have as much as possible configurable.
Some parts still fail here, and are marked FIXME
Effects are flushed on new maps.
The engine has a few builtins.
*/

#include "quakedef.h"
#ifdef SWQUAKE
#include "r_local.h"
#endif
#ifdef RGLQUAKE
#include "glquake.h"//hack
#endif

#include "renderque.h"

#include "r_partset.h"

extern qbyte *host_basepal;

int pt_explosion,
	pt_pointfile,
	pt_entityparticles,
	pt_blob,
	pt_blood,
	pt_lightningblood,
	pt_gunshot,
	pt_wizspike,
	pt_knightspike,
	pt_spike,
	pt_superspike,
	pt_lavasplash,
	pt_teleportsplash,
	pt_blasterparticles,
	pt_superbullet,
	pt_bullet,
	pt_spark,
	pt_plasma,
	pt_smoke;

int pe_default,
	pe_size2,
	pe_size3,
	pe_defaulttrail;

int rt_blastertrail,
	rt_railtrail,
	rt_bubbletrail,
	rt_rocket,
	rt_grenade,
	rt_gib,
	rt_lightning1,
	rt_lightning2,
	rt_lightning3,
	pt_lightning1_end,
	pt_lightning2_end,
	pt_lightning3_end;

//triangle fan sparks use these.
static double sint[7] = {0.000000, 0.781832,  0.974928,  0.433884, -0.433884, -0.974928, -0.781832};
static double cost[7] = {1.000000, 0.623490, -0.222521, -0.900969, -0.900969, -0.222521,  0.623490};

#define crand() (rand()%32767/16383.5f-1)

void D_DrawParticleTrans (particle_t *pparticle, int blendmode);
void D_DrawSparkTrans (particle_t *pparticle, vec3_t src, vec3_t dest, int blendmode);

void P_ReadPointFile_f (void);
void P_ExportBuiltinSet_f(void);

#define MAX_BEAMSEGS             2048   // default max # of beam segments
#define MAX_PARTICLES			32768	// default max # of particles at one
										//  time
#define MAX_DECALS				 4096	// this is going to be expensive
#define MAX_TRAILSTATES           512   // default max # of trailstates

//int		ramp1[8] = {0x6f, 0x6d, 0x6b, 0x69, 0x67, 0x65, 0x63, 0x61};
//int		ramp2[8] = {0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x68, 0x66};
//int		ramp3[8] = {0x6d, 0x6b, 6,	  5,    4,    3,    2,    1};

particle_t	*free_particles;
particle_t	*particles;	//contains the initial list of alloced particles.
int			r_numparticles;

beamseg_t   *free_beams;
beamseg_t   *beams;
int			r_numbeams;

clippeddecal_t	*free_decals;
clippeddecal_t	*decals;
int			r_numdecals;

trailstate_t *trailstates;
int			ts_cycle; // current cyclic index of trailstates
int			r_numtrailstates;

vec3_t			r_pright, r_pup, r_ppn;

extern cvar_t r_bouncysparks;
extern cvar_t r_part_rain;
extern cvar_t r_bloodstains;
extern cvar_t gl_part_flame;

cvar_t r_particlesdesc = {"r_particlesdesc", "spikeset", NULL, CVAR_LATCH|CVAR_SEMICHEAT};

cvar_t r_part_rain_quantity = {"r_part_rain_quantity", "1"};

cvar_t r_rockettrail = {"r_rockettrail", "1", NULL, CVAR_SEMICHEAT};
cvar_t r_grenadetrail = {"r_grenadetrail", "1", NULL, CVAR_SEMICHEAT};

cvar_t r_particle_tracelimit = {"r_particle_tracelimit", "250"};
cvar_t r_part_sparks = {"r_part_sparks", "1"};
cvar_t r_part_sparks_trifan = {"r_part_sparks_trifan", "1"};
cvar_t r_part_sparks_textured = {"r_part_sparks_textured", "1"};
cvar_t r_part_beams = {"r_part_beams", "1"};
cvar_t r_part_beams_textured = {"r_part_beams_textured", "1"};
cvar_t r_part_contentswitch = {"r_part_contentswitch", "1"};

static float particletime;

typedef struct skytris_s {
	struct skytris_s *next;
	vec3_t org;
	vec3_t x;
	vec3_t y;
	float area;
	float nexttime;
	msurface_t *face;
} skytris_t;

//these could be deltas or absolutes depending on ramping mode.
typedef struct {
	vec3_t rgb;
	float alpha;
	float scale;
	float rotation;
} ramp_t;

#define APPLYBLEND(bm)	\
		switch (bm)												\
		{														\
		case BM_ADD:											\
			qglBlendFunc(GL_SRC_ALPHA, GL_ONE);					\
			break;												\
		case BM_SUBTRACT:										\
			qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);	\
			break;												\
		case BM_BLENDCOLOUR:										\
			qglBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);	\
			break;												\
		case BM_BLEND:											\
		default:												\
			qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	\
			break;												\
		}


typedef struct part_type_s {
	char name[MAX_QPATH];
	char texname[MAX_QPATH];
	vec3_t rgb;
	vec3_t rgbchange;
	vec3_t rgbrand;
	int colorindex;
	int colorrand;
	float rgbchangetime;
	vec3_t rgbrandsync;
	float scale, alpha;
	float alphachange;
	float die, randdie;
	float randomvel, veladd;
	float orgadd;
	float offsetspread;
	float offsetspreadvert;
	float randomvelvert;
	float randscale;

	float spawntime;
	float spawnchance;

	enum {PT_NORMAL, PT_SPARK, PT_SPARKFAN, PT_TEXTUREDSPARK, PT_BEAM, PT_DECAL} type;
	blendmode_t blendmode;

	float rotationstartmin, rotationstartrand;
	float rotationmin, rotationrand;

	float scaledelta;
	float count;
	float countrand;
	int texturenum;
	int assoc;
	int cliptype;
	int inwater;
	float clipcount;
	int emit;
	float emittime;
	float emitrand;
	float emitstart;

	float areaspread;
	float areaspreadvert;
	float scalefactor;
	float invscalefactor;

	float spawnparam1;
	float spawnparam2;
/*	float spawnparam3; */

	float offsetup; // make this into a vec3_t later with dir, possibly for mdls

	enum {
		SM_BOX, //box = even spread within the area
		SM_CIRCLE, //circle = around edge of a circle
		SM_BALL, //ball = filled sphere
		SM_SPIRAL, //spiral = spiral trail
		SM_TRACER, //tracer = tracer trail
		SM_TELEBOX, //telebox = q1-style telebox
		SM_LAVASPLASH, //lavasplash = q1-style lavasplash
		SM_UNICIRCLE, //unicircle = uniform circle
		SM_FIELD, //field = synced field (brightfield, etc)
		SM_DISTBALL // uneven distributed ball
	} spawnmode;

	float gravity;
	vec3_t friction;
	float clipbounce;
	int stains;

	enum {RAMP_NONE, RAMP_DELTA, RAMP_ABSOLUTE} rampmode;
	int rampindexes;
	ramp_t *ramp;

	int loaded;
	particle_t	*particles;
	clippeddecal_t *clippeddecals;
	beamseg_t *beams;
	skytris_t *skytris;

	unsigned int flags;
#define PT_VELOCITY	     0x001
#define PT_FRICTION	     0x002
#define PT_CHANGESCOLOUR 0x004
#define PT_CITRACER      0x008 // Q1-style tracer behavior for colorindex
#define PT_INVFRAMETIME  0x010 // apply inverse frametime to count (causes emits to be per frame)
#define PT_AVERAGETRAIL  0x020 // average trail points from start to end, useful with t_lightning, etc
#define PT_NOSTATE       0x040 // don't use trailstate for this emitter (careful with assoc...)
#define PT_NOSPREADFIRST 0x080 // don't randomize org/vel for first generated particle
#define PT_NOSPREADLAST  0x100 // don't randomize org/vel for last generated particle
} part_type_t;
int numparticletypes;
part_type_t *part_type;

static part_type_t *P_GetParticleType(char *name)
{
	int i;
	part_type_t *ptype;
	for (i = 0; i < numparticletypes; i++)
	{
		ptype = &part_type[i];
		if (!strcmp(ptype->name, name))
			return ptype;
	}
	part_type = BZ_Realloc(part_type, sizeof(part_type_t)*(numparticletypes+1));
	ptype = &part_type[numparticletypes++];
	strcpy(ptype->name, name);
	ptype->assoc=-1;
	ptype->cliptype = -1;
	ptype->emit = -1;
/*
	Due to BZ_Realloc we can assume all of this anyway
	ptype->loaded = 0;
	ptype->ramp = NULL;
	ptype->particles = NULL;
	ptype->beams = NULL;
*/
	return ptype;
}

int P_AllocateParticleType(char *name)	//guarentees that the particle type exists, returning it's index.
{
	part_type_t *pt = P_GetParticleType(name);
	return pt - part_type;
}

int P_ParticleTypeForName(char *name)
{
	int to;

	to = P_GetParticleType(name) - part_type;
	if (to < 0 || to >= numparticletypes)
	{
		return -1;
	}

	return to;
}

int P_FindParticleType(char *name)	//checks if particle description 'name' exists, returns -1 if not.
{
	int i;
	for (i = 0; i < numparticletypes; i++)
	{
		if (!strcmp(part_type[i].name, name))
			return i;
	}

	return -1;
}

int P_DescriptionIsLoaded(char *name)
{
	int i = P_FindParticleType(name);
	part_type_t *ptype;
	if (i < 0)
		return false;
	ptype = &part_type[i];
	if (!ptype->loaded)
		return false;
	return i+1;
}

qboolean P_TypeIsLoaded(int effect)
{
	int i = effect;
	part_type_t *ptype;
	if (i < 0)
		return false;
	ptype = &part_type[i];
	if (!ptype->loaded)
		return false;
	return true;
}

static void P_SetModified(void)	//called when the particle system changes (from console).
{
	if (Cmd_FromGamecode())
		return;	//server stuffed particle descriptions don't count.

	f_modified_particles = true;

	if (care_f_modified)
	{
		care_f_modified = false;
		Cbuf_AddText("say particles description has changed\n", RESTRICT_LOCAL);
	}
}
static int CheckAssosiation(char *name, int from)
{
	int to, orig;

	orig = to = P_AllocateParticleType(name);

	while(to != -1)
	{
		if (to == from)
		{
			Con_Printf("Assosiation of %s would cause infinate loop\n", name);
			return -1;
		}
		to = part_type[to].assoc;
	}
	return orig;
}

#ifdef RGLQUAKE
void P_LoadTexture(part_type_t *ptype, qboolean warn)
{
	if (*ptype->texname && strcmp(ptype->texname, "default"))
	{
		ptype->texturenum = Mod_LoadHiResTexture(ptype->texname, "particles", true, true, true);

		if (!ptype->texturenum)
		{
			if (warn)
				Con_DPrintf("Couldn't load texture %s for particle effect %s\n", ptype->texname, ptype->name);

			if (strstr(ptype->texname, "glow") || strstr(ptype->texname, "ball"))
				ptype->texturenum = balltexture;
			else
				ptype->texturenum = explosiontexture;
		}
	}
	else
		ptype->texturenum = explosiontexture;
}
#endif

//Uses FTE's multiline console stuff.
//This is the function that loads the effect descriptions (via console).
void P_ParticleEffect_f(void)
{
	char *var, *value;
	char *buf;
	particle_t *parts;
	beamseg_t *beamsegs;
	skytris_t *st;
	qboolean settype = false;

	part_type_t *ptype;
	int pnum, assoc;

	if (Cmd_Argc()!=2)
	{
		Con_Printf("No name for particle effect\n");
		return;
	}

	buf = Cbuf_GetNext(Cmd_ExecLevel);
	while (*buf && *buf <= ' ')
		buf++;	//no whitespace please.
	if (*buf != '{')
	{
		Cbuf_InsertText(buf, Cmd_ExecLevel);
		Con_Printf("This is a multiline command and should be used within config files\n");
		return;
	}

	ptype = P_GetParticleType(Cmd_Argv(1));
	if (!ptype)
	{
		Con_Printf("Bad name\n");
		return;
	}

	P_SetModified();

	pnum = ptype-part_type;

	st = ptype->skytris;
	if (ptype->ramp)
		BZ_Free(ptype->ramp);

	while (ptype->particles) // empty particle list
	{
		parts = ptype->particles->next;
		ptype->particles->next = free_particles;
		free_particles = ptype->particles;
		ptype->particles = parts;
	}

	// go with a lazy clear of list.. mark everything as DEAD and let
	// the beam rendering handle removing nodes
	beamsegs = ptype->beams;
	while (beamsegs)
	{
		beamsegs->flags |= BS_DEAD;
		beamsegs = beamsegs->next;
	}

	beamsegs = ptype->beams;
	memset(ptype, 0, sizeof(*ptype));
//	ptype->particles = parts;
	ptype->beams = beamsegs;
	ptype->skytris = st;
	strcpy(ptype->name, Cmd_Argv(1));
	ptype->assoc=-1;
	ptype->inwater = -1;
	ptype->cliptype = -1;
	ptype->emit = -1;
	ptype->alpha = 1;
	ptype->alphachange = 1;
	ptype->clipbounce = 0.8;
	ptype->colorindex = -1;
	ptype->rotationstartmin = -M_PI;	//start with a random angle
	ptype->rotationstartrand = M_PI-ptype->rotationstartmin;
	ptype->spawnchance = 1;

	while(1)
	{
		buf = Cbuf_GetNext(Cmd_ExecLevel);
		if (!*buf)
		{
			Con_Printf("Unexpected end of buffer with effect %s\n", ptype->name);
			return;
		}

		while (*buf && *buf <= ' ')
			buf++;	//no whitespace please.
		if (*buf == '}')
			break;

		Cmd_TokenizeString(buf, true, true);
		var = Cmd_Argv(0);
		value = Cmd_Argv(1);

		// TODO: switch this mess to some sort of binary tree to increase
		// parse speed
		if (!strcmp(var, "texture"))
			Q_strncpyz(ptype->texname, value, sizeof(ptype->texname));
		else if (!strcmp(var, "rotationstart"))
		{
			ptype->rotationstartmin = atof(value)*M_PI/180;
			if (Cmd_Argc()>2)
				ptype->rotationstartrand = atof(Cmd_Argv(2))*M_PI/180-ptype->rotationstartmin;
			else
				ptype->rotationstartrand = 0;
		}
		else if (!strcmp(var, "rotationspeed"))
		{
			ptype->rotationmin = atof(value)*M_PI/180;
			if (Cmd_Argc()>2)
				ptype->rotationrand = atof(Cmd_Argv(2))*M_PI/180-ptype->rotationmin;
			else
				ptype->rotationrand = 0;
		}

		else if (!strcmp(var, "scale"))
		{
			ptype->scale = atof(value);
			if (Cmd_Argc()>2)
				ptype->randscale = atof(Cmd_Argv(2)) - ptype->scale;
		}
		else if (!strcmp(var, "scalerand"))
			ptype->randscale = atof(value);

		else if (!strcmp(var, "scalefactor"))
			ptype->scalefactor = atof(value);
		else if (!strcmp(var, "scaledelta"))
			ptype->scaledelta = atof(value);


		else if (!strcmp(var, "step"))
		{
			ptype->count = 1/atof(value);
			if (Cmd_Argc()>2)
				ptype->countrand = 1/atof(Cmd_Argv(2));
		}
		else if (!strcmp(var, "count"))
		{
			ptype->count = atof(value);
			if (Cmd_Argc()>2)
				ptype->countrand = atof(Cmd_Argv(2));
		}

		else if (!strcmp(var, "alpha"))
			ptype->alpha = atof(value);
		else if (!strcmp(var, "alphachange"))
			ptype->alphachange = atof(value);
		else if (!strcmp(var, "die"))
			ptype->die = atof(value);
		else if (!strcmp(var, "diesubrand"))
			ptype->randdie = atof(value);

		else if (!strcmp(var, "randomvel"))
		{
			ptype->randomvel = atof(value);
			if (Cmd_Argc()>2)
				ptype->randomvelvert = atof(Cmd_Argv(2));
			else
				ptype->randomvelvert = ptype->randomvel;
		}
		else if (!strcmp(var, "veladd"))
			ptype->veladd = atof(value);
		else if (!strcmp(var, "orgadd"))
			ptype->orgadd = atof(value);
		else if (!strcmp(var, "friction"))
		{
			ptype->friction[2] = ptype->friction[1] = ptype->friction[0] = atof(value);

			if (Cmd_Argc()>3)
			{
				ptype->friction[2] = atof(Cmd_Argv(3));
				ptype->friction[1] = atof(Cmd_Argv(2));
			}
			else if (Cmd_Argc()>2)
			{
				ptype->friction[2] = atof(Cmd_Argv(2));
			}
		}
		else if (!strcmp(var, "gravity"))
			ptype->gravity = atof(value);
		else if (!strcmp(var, "clipbounce"))
			ptype->clipbounce = atof(value);

		else if (!strcmp(var, "assoc"))
		{
			assoc = CheckAssosiation(value, pnum);	//careful - this can realloc all the particle types
			ptype = &part_type[pnum];
			ptype->assoc = assoc;
		}
		else if (!strcmp(var, "inwater"))
		{
			// the underwater effect switch should only occur for
			// 1 level so the standard assoc check works
			assoc = CheckAssosiation(value, pnum);
			ptype = &part_type[pnum];
			ptype->inwater = assoc;
		}
		else if (!strcmp(var, "colorindex"))
		{
			if (Cmd_Argc()>2)
				ptype->colorrand = atof(Cmd_Argv(2));
			ptype->colorindex = atoi(value);
		}
		else if (!strcmp(var, "colorrand"))
			ptype->colorrand = atoi(value); // now obsolete
		else if (!strcmp(var, "citracer"))
			ptype->flags |= PT_CITRACER;

		else if (!strcmp(var, "red"))
			ptype->rgb[0] = atof(value)/255;
		else if (!strcmp(var, "green"))
			ptype->rgb[1] = atof(value)/255;
		else if (!strcmp(var, "blue"))
			ptype->rgb[2] = atof(value)/255;
		else if (!strcmp(var, "rgb"))
		{
			ptype->rgb[0] = ptype->rgb[1] = ptype->rgb[2] = atof(value)/255;
			if (Cmd_Argc()>3)
			{
				ptype->rgb[1] = atof(Cmd_Argv(2))/255;
				ptype->rgb[2] = atof(Cmd_Argv(3))/255;
			}
		}

		else if (!strcmp(var, "reddelta"))
		{
			ptype->rgbchange[0] = atof(value)/255;
			if (!ptype->rgbchangetime)
				ptype->rgbchangetime = ptype->die;
		}
		else if (!strcmp(var, "greendelta"))
		{
			ptype->rgbchange[1] = atof(value)/255;
			if (!ptype->rgbchangetime)
				ptype->rgbchangetime = ptype->die;
		}
		else if (!strcmp(var, "bluedelta"))
		{
			ptype->rgbchange[2] = atof(value)/255;
			if (!ptype->rgbchangetime)
				ptype->rgbchangetime = ptype->die;
		}
		else if (!strcmp(var, "rgbdelta"))
		{
			ptype->rgbchange[0] = ptype->rgbchange[1] = ptype->rgbchange[2] = atof(value)/255;
			if (Cmd_Argc()>3)
			{
				ptype->rgbchange[1] = atof(Cmd_Argv(2))/255;
				ptype->rgbchange[2] = atof(Cmd_Argv(3))/255;
			}
			if (!ptype->rgbchangetime)
				ptype->rgbchangetime = ptype->die;
		}
		else if (!strcmp(var, "rgbdeltatime"))
			ptype->rgbchangetime = atof(value);

		else if (!strcmp(var, "redrand"))
			ptype->rgbrand[0] = atof(value)/255;
		else if (!strcmp(var, "greenrand"))
			ptype->rgbrand[1] = atof(value)/255;
		else if (!strcmp(var, "bluerand"))
			ptype->rgbrand[2] = atof(value)/255;
		else if (!strcmp(var, "rgbrand"))
		{
			ptype->rgbrand[0] = ptype->rgbrand[1] = ptype->rgbrand[2] = atof(value)/255;
			if (Cmd_Argc()>3)
			{
				ptype->rgbrand[1] = atof(Cmd_Argv(2))/255;
				ptype->rgbrand[2] = atof(Cmd_Argv(3))/255;
			}
		}

		else if (!strcmp(var, "rgbrandsync"))
		{
			ptype->rgbrandsync[0] = ptype->rgbrandsync[1] = ptype->rgbrandsync[2] = atof(value);
			if (Cmd_Argc()>3)
			{
				ptype->rgbrandsync[1] = atof(Cmd_Argv(2));
				ptype->rgbrandsync[2] = atof(Cmd_Argv(3));
			}
		}
		else if (!strcmp(var, "redrandsync"))
			ptype->rgbrandsync[0] = atof(value);
		else if (!strcmp(var, "greenrandsync"))
			ptype->rgbrandsync[1] = atof(value);
		else if (!strcmp(var, "bluerandsync"))
			ptype->rgbrandsync[2] = atof(value);

		else if (!strcmp(var, "stains"))
			ptype->stains = atoi(value);
		else if (!strcmp(var, "blend"))
		{
			if (!strcmp(value, "add"))
				ptype->blendmode = BM_ADD;
			else if (!strcmp(value, "subtract"))
				ptype->blendmode = BM_SUBTRACT;
			else if (!strcmp(value, "blendcolour") || !strcmp(value, "blendcolor"))
				ptype->blendmode = BM_BLENDCOLOUR;
			else
				ptype->blendmode = BM_BLEND;
		}
		else if (!strcmp(var, "spawnmode"))
		{
			if (!strcmp(value, "circle"))
				ptype->spawnmode = SM_CIRCLE;
			else if (!strcmp(value, "ball"))
				ptype->spawnmode = SM_BALL;
			else if (!strcmp(value, "spiral"))
				ptype->spawnmode = SM_SPIRAL;
			else if (!strcmp(value, "tracer"))
				ptype->spawnmode = SM_TRACER;
			else if (!strcmp(value, "telebox"))
				ptype->spawnmode = SM_TELEBOX;
			else if (!strcmp(value, "lavasplash"))
				ptype->spawnmode = SM_LAVASPLASH;
			else if (!strcmp(value, "uniformcircle"))
				ptype->spawnmode = SM_UNICIRCLE;
			else if (!strcmp(value, "syncfield"))
				ptype->spawnmode = SM_FIELD;
			else if (!strcmp(value, "distball"))
				ptype->spawnmode = SM_DISTBALL;
			else
				ptype->spawnmode = SM_BOX;

		}
		else if (!strcmp(var, "type"))
		{
			if (!strcmp(value, "beam"))
				ptype->type = PT_BEAM;
			else if (!strcmp(value, "spark"))
				ptype->type = PT_SPARK;
			else if (!strcmp(value, "sparkfan") || !strcmp(value, "trianglefan"))
				ptype->type = PT_SPARKFAN;
			else if (!strcmp(value, "texturedspark"))
				ptype->type = PT_TEXTUREDSPARK;
			else if (!strcmp(value, "decal"))
				ptype->type = PT_DECAL;
			else
				ptype->type = PT_NORMAL;
			settype = true;
		}
		else if (!strcmp(var, "isbeam"))
		{
			Con_DPrintf("isbeam is deprechiated, use type beam\n");
			ptype->type = PT_BEAM;
		}
		else if (!strcmp(var, "spawntime"))
			ptype->spawntime = atof(value);
		else if (!strcmp(var, "spawnchance"))
			ptype->spawnchance = atof(value);
		else if (!strcmp(var, "cliptype"))
		{
			assoc = P_ParticleTypeForName(value);//careful - this can realloc all the particle types
			ptype = &part_type[pnum];
			ptype->cliptype = assoc;
		}
		else if (!strcmp(var, "clipcount"))
			ptype->clipcount = atof(value);

		else if (!strcmp(var, "emit"))
		{
			assoc = P_ParticleTypeForName(value);//careful - this can realloc all the particle types
			ptype = &part_type[pnum];
			ptype->emit = assoc;
		}
		else if (!strcmp(var, "emitinterval"))
			ptype->emittime = atof(value);
		else if (!strcmp(var, "emitintervalrand"))
			ptype->emitrand = atof(value);
		else if (!strcmp(var, "emitstart"))
			ptype->emitstart = atof(value);

		// old names
		else if (!strcmp(var, "areaspread"))
		{
			Con_DPrintf("areaspread is deprechiated, use spawnorg\n");
			ptype->areaspread = atof(value);
		}
		else if (!strcmp(var, "areaspreadvert"))
		{
			Con_DPrintf("areaspreadvert is deprechiated, use spawnorg\n");
			ptype->areaspreadvert = atof(value);
		}
		else if (!strcmp(var, "offsetspread"))
		{
			Con_DPrintf("offsetspread is deprechiated, use spawnvel\n");
			ptype->offsetspread = atof(value);
		}
		else if (!strcmp(var, "offsetspreadvert"))
		{
			Con_DPrintf("offsetspreadvert is deprechiated, use spawnvel\n");
			ptype->offsetspreadvert  = atof(value);
		}

		// new names
		else if (!strcmp(var, "spawnorg"))
		{
			ptype->areaspreadvert = ptype->areaspread = atof(value);

			if (Cmd_Argc()>2)
				ptype->areaspreadvert = atof(Cmd_Argv(2));
		}
		else if (!strcmp(var, "spawnvel"))
		{
			ptype->offsetspreadvert = ptype->offsetspread = atof(value);

			if (Cmd_Argc()>2)
				ptype->offsetspreadvert = atof(Cmd_Argv(2));
		}

		// spawn mode param fields
		else if (!strcmp(var, "spawnparam1"))
			ptype->spawnparam1 = atof(value);
		else if (!strcmp(var, "spawnparam2"))
			ptype->spawnparam2 = atof(value);
/*		else if (!strcmp(var, "spawnparam3"))
			ptype->spawnparam3 = atof(value); */

		else if (!strcmp(var, "up"))
			ptype->offsetup = atof(value);
		else if (!strcmp(var, "rampmode"))
		{
			if (!strcmp(value, "none"))
				ptype->rampmode = RAMP_NONE;
			else if (!strcmp(value, "absolute"))
				ptype->rampmode = RAMP_ABSOLUTE;
			else //if (!strcmp(value, "delta"))
				ptype->rampmode = RAMP_DELTA;
		}
		else if (!strcmp(var, "rampindexlist"))
		{ // better not use this with delta ramps...
			int cidx, i;

			i = 1;
			while (i <= Cmd_Argc())
			{
				ptype->ramp = BZ_Realloc(ptype->ramp, sizeof(ramp_t)*(ptype->rampindexes+1));

				cidx = atoi(Cmd_Argv(i));
				ptype->ramp[ptype->rampindexes].alpha = cidx > 255 ? 0.5 : 1;

				cidx = (cidx & 0xff) * 3;
				ptype->ramp[ptype->rampindexes].rgb[0] = host_basepal[cidx] * (1/255.0);
				ptype->ramp[ptype->rampindexes].rgb[1] = host_basepal[cidx+1] * (1/255.0);
				ptype->ramp[ptype->rampindexes].rgb[2] = host_basepal[cidx+2] * (1/255.0);

				ptype->ramp[ptype->rampindexes].scale = ptype->scale;

				ptype->rampindexes++;
				i++;
			}
		}
		else if (!strcmp(var, "rampindex"))
		{
			int cidx;
			ptype->ramp = BZ_Realloc(ptype->ramp, sizeof(ramp_t)*(ptype->rampindexes+1));

			cidx = atoi(value);
			ptype->ramp[ptype->rampindexes].alpha = cidx > 255 ? 0.5 : 1;

			if (Cmd_Argc() > 2) // they gave alpha
				ptype->ramp[ptype->rampindexes].alpha *= atof(Cmd_Argv(2));

			cidx = (cidx & 0xff) * 3;
			ptype->ramp[ptype->rampindexes].rgb[0] = host_basepal[cidx] * (1/255.0);
			ptype->ramp[ptype->rampindexes].rgb[1] = host_basepal[cidx+1] * (1/255.0);
			ptype->ramp[ptype->rampindexes].rgb[2] = host_basepal[cidx+2] * (1/255.0);

			if (Cmd_Argc() > 3) // they gave scale
				ptype->ramp[ptype->rampindexes].scale = atof(Cmd_Argv(3));
			else
				ptype->ramp[ptype->rampindexes].scale = ptype->scale;


			ptype->rampindexes++;
		}
		else if (!strcmp(var, "ramp"))
		{
			ptype->ramp = BZ_Realloc(ptype->ramp, sizeof(ramp_t)*(ptype->rampindexes+1));

			ptype->ramp[ptype->rampindexes].rgb[0] = atof(value)/255;
			if (Cmd_Argc()>3)	//seperate rgb
			{
				ptype->ramp[ptype->rampindexes].rgb[1] = atof(Cmd_Argv(2))/255;
				ptype->ramp[ptype->rampindexes].rgb[2] = atof(Cmd_Argv(3))/255;

				if (Cmd_Argc()>4)	//have we alpha and scale changes?
				{
					ptype->ramp[ptype->rampindexes].alpha = atof(Cmd_Argv(4));
					if (Cmd_Argc()>5)	//have we scale changes?
						ptype->ramp[ptype->rampindexes].scale = atof(Cmd_Argv(5));
					else
						ptype->ramp[ptype->rampindexes].scale = ptype->scaledelta;
				}
				else
				{
					ptype->ramp[ptype->rampindexes].alpha = ptype->alpha;
					ptype->ramp[ptype->rampindexes].scale = ptype->scaledelta;
				}
			}
			else	//they only gave one value
			{
				ptype->ramp[ptype->rampindexes].rgb[1] = ptype->ramp[ptype->rampindexes].rgb[0];
				ptype->ramp[ptype->rampindexes].rgb[2] = ptype->ramp[ptype->rampindexes].rgb[0];

				ptype->ramp[ptype->rampindexes].alpha = ptype->alpha;
				ptype->ramp[ptype->rampindexes].scale = ptype->scaledelta;
			}

			ptype->rampindexes++;
		}
		else if (!strcmp(var, "perframe"))
			ptype->flags |= PT_INVFRAMETIME;
		else if (!strcmp(var, "averageout"))
			ptype->flags |= PT_AVERAGETRAIL;
		else if (!strcmp(var, "nostate"))
			ptype->flags |= PT_NOSTATE;
		else if (!strcmp(var, "nospreadfirst"))
			ptype->flags |= PT_NOSPREADFIRST;
		else if (!strcmp(var, "nospreadlast"))
			ptype->flags |= PT_NOSPREADLAST;
		else
			Con_DPrintf("%s is not a recognised particle type field (in %s)\n", var, ptype->name);
	}
	ptype->invscalefactor = 1-ptype->scalefactor;
	ptype->loaded = 1;
	if (ptype->clipcount < 1)
		ptype->clipcount = 1;

	//if there is a chance that it moves
	if (ptype->randomvel || ptype->gravity || ptype->veladd || ptype->offsetspread || ptype->offsetspreadvert)
		ptype->flags |= PT_VELOCITY;
	//if it has friction
	if (ptype->friction)
		ptype->flags |= PT_FRICTION;

	if (!settype)
	{
		if (ptype->type == PT_NORMAL && !*ptype->texname)
			ptype->type = PT_SPARK;
		if (ptype->type == PT_SPARK)
		{
			if (*ptype->texname)
				ptype->type = PT_TEXTUREDSPARK;
			if (ptype->scale)
				ptype->type = PT_SPARKFAN;
		}
	}

	if (ptype->rampmode && !ptype->ramp)
	{
		ptype->rampmode = RAMP_NONE;
		Con_Printf("Particle type %s has a ramp mode but no ramp\n", ptype->name);
	}
	else if (ptype->ramp && !ptype->rampmode)
	{
		Con_Printf("Particle type %s has a ramp but no ramp mode\n", ptype->name);
	}

#ifdef RGLQUAKE
	if (qrenderer == QR_OPENGL)
	{
		P_LoadTexture(ptype, true);
	}
#endif
}

//assosiate a point effect with a model.
//the effect will be spawned every frame with count*frametime
//has the capability to hide models.
void P_AssosiateEffect_f (void)
{
	char *modelname = Cmd_Argv(1);
	char *effectname = Cmd_Argv(2);
	int effectnum;
	model_t *model;

	if (!cls.demoplayback && (
		strstr(modelname, "player") ||
		strstr(modelname, "eyes") ||
		strstr(modelname, "flag") ||
		strstr(modelname, "tf_stan") ||
		strstr(modelname, ".bsp") ||
		strstr(modelname, "turr")))
	{
		Con_Printf("Sorry: Not allowed to attach effects to model \"%s\"\n", modelname);
		return;
	}

	model = Mod_FindName(modelname);
	if (!cls.demoplayback && (model->flags & EF_ROTATE))
	{
		Con_Printf("Sorry: You may not assosiate effects with item model \"%s\"\n", modelname);
		return;
	}
	effectnum = P_AllocateParticleType(effectname);
	model->particleeffect = effectnum;
	if (atoi(Cmd_Argv(3)))
		model->engineflags |= MDLF_ENGULPHS;

	P_SetModified();	//make it appear in f_modified.
}

//assosiate a particle trail with a model.
//the effect will be spawned between two points when an entity with the model moves.
void P_AssosiateTrail_f (void)
{
	char *modelname = Cmd_Argv(1);
	char *effectname = Cmd_Argv(2);
	int effectnum;
	model_t *model;

	if (!cls.demoplayback && (
		strstr(modelname, "player") ||
		strstr(modelname, "eyes") ||
		strstr(modelname, "flag") ||
		strstr(modelname, "tf_stan")))
	{
		Con_Printf("Sorry, you can't assosiate trails with model \"%s\"\n", modelname);
		return;
	}

	model = Mod_FindName(modelname);
	effectnum = P_AllocateParticleType(effectname);
	model->particletrail = effectnum;
	model->engineflags |= MDLF_NODEFAULTTRAIL;	//we could have assigned the trail to a model that wasn't loaded.

	P_SetModified();	//make it appear in f_modified.
}

// P_SelectableTrail: given default/opposite effects, model pointer, and a user selection cvar
// changes model to the appropriate trail effect and default trail index
void P_SelectableTrail(model_t *model, cvar_t *selection, int mdleffect, int mdlcidx, int oppeffect, int oppcidx)
{
	int select = (int)(selection->value);

	switch (select)
	{
	case 0: // check for string, otherwise no trail
		{
			int effect = P_FindParticleType(selection->string);

			if (effect >= 0)
			{
				model->particletrail = effect;
				model->traildefaultindex = mdlcidx;
				break;
			}
		}

		model->particletrail = -1;
		break;
	case 1: // default model effect
	default:
		model->particletrail = mdleffect;
		model->traildefaultindex = mdlcidx;
		break;
	case 2: // opposite effect
		model->particletrail = oppeffect;
		model->traildefaultindex= oppcidx;
		break;
	case 3: // alt rocket effect
		model->particletrail = P_AllocateParticleType("t_altrocket");
		model->traildefaultindex = 107;
		break;
	case 4: // gib
		model->particletrail = rt_gib;
		model->traildefaultindex = 70;
		break;
	case 5: // zombie gib
		model->particletrail = P_AllocateParticleType("t_zomgib");
		model->traildefaultindex = 70;
		break;
	case 6: // Scrag tracer
		model->particletrail = P_AllocateParticleType("t_tracer");
		model->traildefaultindex = 60;
		break;
	case 7: // Knight tracer
		model->particletrail = P_AllocateParticleType("t_tracer2");
		model->traildefaultindex = 238;
		break;
	case 8: // Vore tracer
		model->particletrail = P_AllocateParticleType("t_tracer3");
		model->traildefaultindex = 154;
		break;
	case 9: // rail trail
		model->particletrail = rt_railtrail;
		model->traildefaultindex = 15;
		break;
	}
}

void P_DefaultTrail (model_t *model)
{
	// TODO: EF_BRIGHTFIELD should probably be handled in here somewhere
	// TODO: make trail default color into RGB values instead of indexes
	if (model->engineflags & MDLF_NODEFAULTTRAIL)
		return;

	if (model->flags & EF_ROCKET)
		P_SelectableTrail(model, &r_rockettrail, rt_rocket, 109, rt_grenade, 6);
	else if (model->flags & EF_GRENADE)
		P_SelectableTrail(model, &r_grenadetrail, rt_grenade, 6, rt_rocket, 109);
	else if (model->flags & EF_GIB)
	{
		model->particletrail = rt_gib;
		model->traildefaultindex = 70;
	}
	else if (model->flags & EF_TRACER)
	{
		model->particletrail = P_AllocateParticleType("t_tracer");
		model->traildefaultindex = 60;
	}
	else if (model->flags & EF_ZOMGIB)
	{
		model->particletrail = P_AllocateParticleType("t_zomgib");
		model->traildefaultindex = 70;
	}
	else if (model->flags & EF_TRACER2)
	{
		model->particletrail = P_AllocateParticleType("t_tracer2");
		model->traildefaultindex = 238;
	}
	else if (model->flags & EF_TRACER3)
	{
		model->particletrail = P_AllocateParticleType("t_tracer3");
		model->traildefaultindex = 154;
	}
	else if (model->flags & EF_BLOODSHOT)	//these are the hexen2 ones.
	{
		model->particletrail = P_AllocateParticleType("t_bloodshot");
		model->traildefaultindex = 136;
	}
	else if (model->flags & EF_FIREBALL)
	{
		model->particletrail = P_AllocateParticleType("t_fireball");
		model->traildefaultindex = 424;
	}
	else if (model->flags & EF_ACIDBALL)
	{
		model->particletrail = P_AllocateParticleType("t_acidball");
		model->traildefaultindex = 440;
	}
	else if (model->flags & EF_ICE)
	{
		model->particletrail = P_AllocateParticleType("t_ice");
		model->traildefaultindex = 408;
	}
	else if (model->flags & EF_SPIT)
	{
		model->particletrail = P_AllocateParticleType("t_spit");
		model->traildefaultindex = 260;
	}
	else if (model->flags & EF_SPELL)
	{
		model->particletrail = P_AllocateParticleType("t_spell");
		model->traildefaultindex = 260;
	}
	else if (model->flags & EF_VORP_MISSILE)
	{
		model->particletrail = P_AllocateParticleType("t_vorpmissile");
		model->traildefaultindex = 302;
	}
	else if (model->flags & EF_SET_STAFF)
	{
		model->particletrail = P_AllocateParticleType("t_setstaff");
		model->traildefaultindex = 424;
	}
	else if (model->flags & EF_MAGICMISSILE)
	{
		model->particletrail = P_AllocateParticleType("t_magicmissile");
		model->traildefaultindex = 149;
	}
	else if (model->flags & EF_BONESHARD)
	{
		model->particletrail = P_AllocateParticleType("t_boneshard");
		model->traildefaultindex = 384;
	}
	else if (model->flags & EF_SCARAB)
	{
		model->particletrail = P_AllocateParticleType("t_scarab");
		model->traildefaultindex = 254;
	}
	else
		model->particletrail = -1;
}

#if _DEBUG
// R_BeamInfo_f - debug junk
void P_BeamInfo_f (void)
{
	beamseg_t *bs;
	int i, j, k, l, m;

	i = 0;

	for (bs = free_beams; bs; bs = bs->next)
		i++;

	Con_Printf("%i free beams\n", i);

	for (i = 0; i < numparticletypes; i++)
	{
		m = l = k = j = 0;
		for (bs = part_type[i].beams; bs; bs = bs->next)
		{
			if (!bs->p)
				k++;

			if (bs->flags & BS_DEAD)
				l++;

			if (bs->flags & BS_LASTSEG)
				m++;

			j++;
		}

		if (j)
			Con_Printf("Type %i = %i NULL p, %i DEAD, %i LASTSEG, %i total\n", i, k, l, m, j);
	}
}

void P_PartInfo_f (void)
{
	particle_t *p;

	int i, j;

	i = 0;

	for (p = free_particles; p; p = p->next)
		i++;

	Con_Printf("%i free particles\n", i);

	for (i = 0; i < numparticletypes; i++)
	{
		j = 0;
		for (p = part_type[i].particles; p; p = p->next)
			j++;

		if (j)
			Con_Printf("Type %s = %i total\n", part_type[i].name, j);
	}
}
#endif

/*
===============
R_InitParticles
===============
*/
void P_InitParticles (void)
{
	char *particlecvargroupname = "Particle effects";
	int		i;

	if (r_numparticles)	//already inited
		return;

	i = COM_CheckParm ("-particles");

	if (i)
	{
		r_numparticles = (int)(Q_atoi(com_argv[i+1]));
	}
	else
	{
		r_numparticles = MAX_PARTICLES;
	}

	r_numbeams = MAX_BEAMSEGS;

	r_numdecals = MAX_DECALS;

	r_numtrailstates = MAX_TRAILSTATES;

	particles = (particle_t *)
			Hunk_AllocName (r_numparticles * sizeof(particle_t), "particles");

	beams = (beamseg_t *)
			Hunk_AllocName (r_numbeams * sizeof(beamseg_t), "beamsegs");

	decals = (clippeddecal_t *)
			Hunk_AllocName (r_numdecals * sizeof(clippeddecal_t), "decals");

	trailstates = (trailstate_t *)
			Hunk_AllocName (r_numtrailstates * sizeof(trailstate_t), "trailstates");
	ts_cycle = 0;

	Cmd_AddCommand("pointfile", P_ReadPointFile_f);	//load the leak info produced from qbsp into the particle system to show a line. :)

	Cmd_AddCommand("r_part", P_ParticleEffect_f);
	Cmd_AddCommand("r_effect", P_AssosiateEffect_f);
	Cmd_AddCommand("r_trail", P_AssosiateTrail_f);

	Cmd_AddCommand("r_exportbuiltinparticles", P_ExportBuiltinSet_f);

#if _DEBUG
	Cmd_AddCommand("r_partinfo", P_PartInfo_f);
	Cmd_AddCommand("r_beaminfo", P_BeamInfo_f);
#endif

	//particles
	Cvar_Register(&r_particlesdesc, particlecvargroupname);
	Cvar_Register(&r_bouncysparks, particlecvargroupname);
	Cvar_Register(&r_part_rain, particlecvargroupname);

	Cvar_Register(&r_part_rain_quantity, particlecvargroupname);

	Cvar_Register(&r_particle_tracelimit, particlecvargroupname);

	Cvar_Register(&r_rockettrail, particlecvargroupname);
	Cvar_Register(&r_grenadetrail, particlecvargroupname);

	Cvar_Register(&r_part_sparks, particlecvargroupname);
	Cvar_Register(&r_part_sparks_trifan, particlecvargroupname);
	Cvar_Register(&r_part_sparks_textured, particlecvargroupname);
	Cvar_Register(&r_part_beams, particlecvargroupname);
	Cvar_Register(&r_part_beams_textured, particlecvargroupname);
	Cvar_Register(&r_part_contentswitch, particlecvargroupname);

	Cvar_Register (&gl_part_flame, particlecvargroupname);

	pt_explosion		= P_AllocateParticleType("te_explosion");
	pt_pointfile		= P_AllocateParticleType("pe_pointfile");
	pt_entityparticles	= P_AllocateParticleType("ef_entityparticles");
	pt_blob				= P_AllocateParticleType("te_blob");

	pt_blood			= P_AllocateParticleType("te_blood");
	pt_lightningblood	= P_AllocateParticleType("te_lightningblood");
	pt_gunshot			= P_AllocateParticleType("te_gunshot");
	pt_lavasplash		= P_AllocateParticleType("te_lavasplash");
	pt_teleportsplash	= P_AllocateParticleType("te_teleportsplash");
	pt_superbullet		= P_AllocateParticleType("te_superbullet");
	pt_bullet			= P_AllocateParticleType("te_bullet");
	pt_blasterparticles = P_AllocateParticleType("te_blasterparticles");
	pt_wizspike			= P_AllocateParticleType("te_wizspike");
	pt_knightspike		= P_AllocateParticleType("te_knightspike");
	pt_spike			= P_AllocateParticleType("te_spike");
	pt_superspike		= P_AllocateParticleType("te_superspike");

	rt_railtrail		= P_AllocateParticleType("te_railtrail");
	rt_bubbletrail		= P_AllocateParticleType("te_bubbletrail");
	rt_blastertrail		= P_AllocateParticleType("t_blastertrail");
	rt_rocket			= P_AllocateParticleType("t_rocket");
	rt_grenade			= P_AllocateParticleType("t_grenade");
	rt_gib				= P_AllocateParticleType("t_gib");

	rt_lightning1		= P_AllocateParticleType("te_lightning1");
	rt_lightning2		= P_AllocateParticleType("te_lightning2");
	rt_lightning3		= P_AllocateParticleType("te_lightning3");

	pt_lightning1_end	= P_AllocateParticleType("te_lightning1_end");
	pt_lightning2_end	= P_AllocateParticleType("te_lightning2_end");
	pt_lightning3_end	= P_AllocateParticleType("te_lightning3_end");

	pt_spark			= P_AllocateParticleType("te_spark");
	pt_plasma			= P_AllocateParticleType("te_plasma");
	pt_smoke			= P_AllocateParticleType("te_smoke");

	pe_default			= P_AllocateParticleType("pe_default");
	pe_size2			= P_AllocateParticleType("pe_size2");
	pe_size3			= P_AllocateParticleType("pe_size3");
	pe_defaulttrail		= P_AllocateParticleType("pe_defaulttrail");
}


/*
===============
P_ClearParticles
===============
*/
void P_ClearParticles (void)
{
	int		i;

	free_particles = &particles[0];
	for (i=0 ;i<r_numparticles ; i++)
		particles[i].next = &particles[i+1];
	particles[r_numparticles-1].next = NULL;

	free_decals = &decals[0];
	for (i=0 ;i<r_numdecals ; i++)
		decals[i].next = &decals[i+1];
	decals[r_numdecals-1].next = NULL;

	free_beams = &beams[0];
	for (i=0 ;i<r_numbeams ; i++)
	{
		beams[i].p = NULL;
		beams[i].flags = BS_DEAD;
		beams[i].next = &beams[i+1];
	}
	beams[r_numbeams-1].next = NULL;

	particletime = cl.time;

#ifdef RGLQUAKE
	if (qrenderer == QR_OPENGL)
	{
		for (i = 0; i < numparticletypes; i++)
		{
			P_LoadTexture(&part_type[i], false);
		}
	}
#endif

	for (i = 0; i < numparticletypes; i++)
	{
		part_type[i].clippeddecals = NULL;
		part_type[i].particles = NULL;
		part_type[i].beams = NULL;
		part_type[i].skytris = NULL;
	}
}

void P_ExportBuiltinSet_f(void)
{
	char *efname = Cmd_Argv(1);
	char *file;

	if (!*efname)
	{
		Con_Printf("Please name the built in effect (faithful, spikeset or highfps)\n");
		return;
	}
	else if (!stricmp(efname, "faithful"))
		file = particle_set_faithful;
	else if (!stricmp(efname, "spikeset"))
		file = particle_set_spikeset;
	else if (!stricmp(efname, "highfps"))
		file = particle_set_highfps;
	else
	{
		if (!stricmp(efname, "none"))
		{
			Con_Printf("nothing to export\n");
			return;
		}
		Con_Printf("'%s' is not a built in particle set\n", efname);
		return;
	}

	COM_WriteFile(va("particles/%s.cfg", efname), file, strlen(file));
	Con_Printf("Written particles/%s.cfg\n", efname);
}

void P_NewServer(void)
{
	extern model_t	mod_known[];
	extern int		mod_numknown;

	model_t *mod;
	int i;

	for (i = 0; i < numparticletypes; i++)
	{
		*part_type[i].texname = '\0';
		part_type[i].scale = 0;
		part_type[i].loaded = 0;
		if (part_type->ramp)
			BZ_Free(part_type->ramp);
		part_type->ramp = NULL;
	}



	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		mod->particleeffect = -1;
		mod->particletrail = -1;
		mod->engineflags &= ~MDLF_NODEFAULTTRAIL;

		P_DefaultTrail(mod);
	}

	f_modified_particles = false;

	{
		char *semi;
		semi = strchr(r_particlesdesc.string, ';');
		if (semi)	//make sure nothing uses this for other means.
			*semi = '\0';
	}

	//particle descriptions submitted by the server are deemed to not be cheats but game configs.
	if (!stricmp(r_particlesdesc.string, "none"))
		return;
	else if (!stricmp(r_particlesdesc.string, "faithful") || !*r_particlesdesc.string)
		Cbuf_AddText(particle_set_faithful, RESTRICT_SERVER);
	else if (!stricmp(r_particlesdesc.string, "spikeset"))
		Cbuf_AddText(particle_set_spikeset, RESTRICT_SERVER);
	else if (!stricmp(r_particlesdesc.string, "highfps"))
		Cbuf_AddText(particle_set_highfps, RESTRICT_SERVER);
	else if (!stricmp(r_particlesdesc.string, "minimal"))
		Cbuf_AddText(particle_set_minimal, RESTRICT_SERVER);
	else
	{
		char *file = COM_LoadMallocFile(va("particles/%s.cfg", r_particlesdesc.string));
		if (!file)
			file = COM_LoadMallocFile(va("%s.cfg", r_particlesdesc.string));
		if (file)
		{
			Cbuf_AddText(file, RESTRICT_LOCAL);
			Cbuf_AddText("\n", RESTRICT_LOCAL);	//I'm paranoid.
			BZ_Free(file);
		}
		else
		{
			Con_Printf("Couldn't find particle description, using spikeset\n");
			Cbuf_AddText(particle_set_spikeset, RESTRICT_SERVER);
		}
/*
#if defined(_DEBUG) && defined(_WIN32)	//expand the particles cfg into a C style quoted string, and copy to clipboard so I can paste it in.
		{
			char *TL_ExpandToCString(char *in);
			extern HWND mainwindow;
			char *file = COM_LoadTempFile(va("%s.cfg", r_particlesdesc.string));
			char *lptstrCopy, *buf, temp;
			int len;
			HANDLE hglbCopy = GlobalAlloc(GMEM_MOVEABLE,
				com_filesize*2);
			lptstrCopy = GlobalLock(hglbCopy);
			while(file && *file)
			{
				len = strlen(file)+1;
				if (len > 1024)
					len = 1024;
				temp = file[len-1];
				file[len-1] = '\0';
				buf = TL_ExpandToCString(file);
				file[len-1] = temp;
				len-=1;
				com_filesize -= len;
				file+=len;

				len = strlen(buf);
				memcpy(lptstrCopy, buf, len);
				lptstrCopy+=len;
			}
			*lptstrCopy = '\0';
			GlobalUnlock(hglbCopy);

			if (!OpenClipboard(mainwindow))
				return;
			EmptyClipboard();

			SetClipboardData(CF_TEXT, hglbCopy);
			CloseClipboard();
		}
#endif
*/
	}
}

void P_ReadPointFile_f (void)
{
	vfsfile_t	*f;
	vec3_t	org;
	int		r;
	int		c;
	char	name[MAX_OSPATH];
	char line[1024];
	char *s;

	COM_StripExtension(cl.worldmodel->name, name);
	strcat(name, ".pts");

	f = FS_OpenVFS(name, "rb", FS_GAME);
	if (!f)
	{
		Con_Printf ("couldn't open %s\n", name);
		return;
	}

	P_ClearParticles();	//so overflows arn't as bad.

	Con_Printf ("Reading %s...\n", name);
	c = 0;
	for ( ;; )
	{
		VFS_GETS(f, line, sizeof(line));

		s = COM_Parse(line);
		org[0] = atof(com_token);

		s = COM_Parse(s);
		if (!s)
			continue;
		org[1] = atof(com_token);

		s = COM_Parse(s);
		if (!s)
			continue;
		org[2] = atof(com_token);
		if (COM_Parse(s))
			continue;

		c++;

		if (c%8)
			continue;

		if (!free_particles)
		{
			Con_Printf ("Not enough free particles\n");
			break;
		}
		P_RunParticleEffectType(org, NULL, 1, pt_pointfile);
	}

	VFS_CLOSE (f);
	Con_Printf ("%i points read\n", c);
}

void P_AddRainParticles(void)
{
	float x;
	float y;
	static float skipped;
	static float lastrendered;
	int ptype;

	vec3_t org, vdist;

	skytris_t *st;

	if (!r_part_rain.value || !r_part_rain_quantity.value)
	{
		skipped = true;
		return;
	}

	if (lastrendered < particletime - 0.5)
		skipped = true;	//we've gone for half a sec without any new rain. This would cause some strange effects, so reset times.

	if (skipped)
	{
		for (ptype = 0; ptype<numparticletypes; ptype++)
		{
			for (st = part_type[ptype].skytris; st; st = st->next)
			{
				st->nexttime = particletime;
			}
		}
	}
	skipped = false;

	lastrendered = particletime;
/*
{
	int i;

glDisable(GL_TEXTURE_2D);
glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
glDisable(GL_DEPTH_TEST);
glBegin(GL_TRIANGLES);

	st = skytris;
	for (i = 0; i < r_part_rain_quantity.value; i++)
		st = st->next;
		glVertex3f(st->org[0], st->org[1], st->org[2]);
		glVertex3f(st->org[0]+st->x[0], st->org[1]+st->x[1], st->org[2]+st->x[2]);
		glVertex3f(st->org[0]+st->y[0], st->org[1]+st->y[1], st->org[2]+st->y[2]);
glEnd();
glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
glBegin(GL_POINTS);
		for (i = 0; i < 1000; i++)
		{
			x = frandom()*frandom();
			y = frandom() * (1-x);
			VectorMA(st->org, x, st->x, org);
			VectorMA(org, y, st->y, org);

			glVertex3f(org[0], org[1], org[2]);
		}
glEnd();
glEnable(GL_DEPTH_TEST);
}
*/
	for (ptype = 0; ptype<numparticletypes; ptype++)
	{
		if (!part_type[ptype].loaded)	//woo, batch skipping.
			continue;

		for (st = part_type[ptype].skytris; st; st = st->next)
		{
	//		if (st->face->visframe != r_framecount)
	//			continue;

			if (st->face->visframe != r_framecount)
			{
				st->nexttime = particletime;
				continue;
			}

			while (st->nexttime < particletime)
			{
				if (!free_particles)
					return;

				st->nexttime += 10000/(st->area*r_part_rain_quantity.value);

				x = frandom()*frandom();
				y = frandom() * (1-x);
				VectorMA(st->org, x, st->x, org);
				VectorMA(org, y, st->y, org);


				VectorSubtract(org, r_refdef.vieworg, vdist);

				if (Length(vdist) > (1024+512)*frandom())
					continue;

				if (st->face->flags & SURF_PLANEBACK)
					VectorMA(org, -0.5, st->face->plane->normal, org);
				else
					VectorMA(org, 0.5, st->face->plane->normal, org);

				if (!(cl.worldmodel->funcs.PointContents(cl.worldmodel, org) & FTECONTENTS_SOLID))
				{
					if (st->face->flags & SURF_PLANEBACK)
					{
						vdist[0] = -st->face->plane->normal[0];
						vdist[1] = -st->face->plane->normal[1];
						vdist[2] = -st->face->plane->normal[2];
						P_RunParticleEffectType(org, vdist, 1, ptype);
					}
					else
						P_RunParticleEffectType(org, st->face->plane->normal, 1, ptype);
				}
			}
		}
	}
}


void R_Part_SkyTri(float *v1, float *v2, float *v3, msurface_t *surf)
{
	float dot;
	float xm;
	float ym;
	float theta;
	vec3_t xd;
	vec3_t yd;

	skytris_t *st;

	st = Hunk_Alloc(sizeof(skytris_t));
	st->next = part_type[surf->texinfo->texture->parttype].skytris;
	VectorCopy(v1, st->org);
	VectorSubtract(v2, st->org, st->x);
	VectorSubtract(v3, st->org, st->y);

	VectorCopy(st->x, xd);
	VectorCopy(st->y, yd);
/*
	xd[2] = 0;	//prevent area from being valid on vertical surfaces
	yd[2] = 0;
*/
	xm = Length(xd);
	ym = Length(yd);

	dot = DotProduct(xd, yd);
	theta = acos(dot/(xm*ym));
	st->area = sin(theta)*xm*ym;
	st->nexttime = particletime;
	st->face = surf;

	if (st->area<=0)
		return;//bummer.

	part_type[surf->texinfo->texture->parttype].skytris = st;
}



void P_EmitSkyEffectTris(model_t *mod, msurface_t 	*fa)
{
	vec3_t		verts[64];
	int v1;
	int v2;
	int v3;
	int numverts;
	int i, lindex;
	float *vec;

	//
	// convert edges back to a normal polygon
	//
	numverts = 0;
	for (i=0 ; i<fa->numedges ; i++)
	{
		lindex = mod->surfedges[fa->firstedge + i];

		if (lindex > 0)
			vec = mod->vertexes[mod->edges[lindex].v[0]].position;
		else
			vec = mod->vertexes[mod->edges[-lindex].v[1]].position;
		VectorCopy (vec, verts[numverts]);
		numverts++;

		if (numverts>=64)
		{
			Con_Printf("Too many verts on sky surface\n");
			return;
		}
	}

	v1 = 0;
	v2 = 1;
	for (v3 = 2; v3 < numverts; v3++)
	{
		R_Part_SkyTri(verts[v1], verts[v2], verts[v3], fa);

		v2 = v3;
	}
}

// Trailstate functions
static void P_CleanTrailstate(trailstate_t *ts)
{
	// clear LASTSEG flag from lastbeam so it can be reused
	if (ts->lastbeam)
	{
		ts->lastbeam->flags &= ~BS_LASTSEG;
		ts->lastbeam->flags |= BS_NODRAW;
	}

	// clean structure
	memset(ts, 0, sizeof(trailstate_t));
}

void P_DelinkTrailstate(trailstate_t **tsk)
{
	trailstate_t *ts;
	trailstate_t *assoc;

	if (*tsk == NULL)
		return; // not linked to a trailstate

	ts = *tsk; // store old pointer
	*tsk = NULL; // clear pointer

	if (ts->key != tsk)
		return; // prevent overwrite

	assoc = ts->assoc; // store assoc
	P_CleanTrailstate(ts); // clean directly linked trailstate

	// clean trailstates assoc linked
	while (assoc)
	{
		ts = assoc->assoc;
		P_CleanTrailstate(assoc);
		assoc = ts;
	}
}

static trailstate_t *P_NewTrailstate(trailstate_t **key)
{
	trailstate_t *ts;

	// bounds check here in case r_numtrailstates changed
	if (ts_cycle >= r_numtrailstates)
		ts_cycle = 0;

	// get trailstate
	ts = trailstates + ts_cycle;

	// clear trailstate
	P_CleanTrailstate(ts);

	// set key
	ts->key = key;

	// advance index cycle
	ts_cycle++;

	// return clean trailstate
	return ts;
}

#define NUMVERTEXNORMALS	162
float	r_avertexnormals[NUMVERTEXNORMALS][3] = {
#include "anorms.h"
};
vec2_t	avelocities[NUMVERTEXNORMALS];
#define BEAMLENGTH 16
// vec3_t	avelocity = {23, 7, 3};
// float	partstep = 0.01;
// float	timescale = 0.01;

int Q1BSP_ClipDecal(vec3_t center, vec3_t normal, vec3_t tangent, vec3_t tangent2, float size, float **out);
int P_RunParticleEffectState (vec3_t org, vec3_t dir, float count, int typenum, trailstate_t **tsk)
{
	part_type_t *ptype = &part_type[typenum];
	int i, j, k, l, spawnspc;
	float m, pcount;
	particle_t	*p;
	beamseg_t *b, *bfirst;
	vec3_t ofsvec, arsvec; // offsetspread vec, areaspread vec
	trailstate_t *ts;

	if (typenum < 0 || typenum >= numparticletypes)
		return 1;

	if (!ptype->loaded)
		return 1;

	// inwater check, switch only once
	if (r_part_contentswitch.value && ptype->inwater >= 0)
	{
		int cont;
		cont = cl.worldmodel->funcs.PointContents(cl.worldmodel, org);

		if (cont & FTECONTENTS_WATER)
			ptype = &part_type[ptype->inwater];
	}

	// eliminate trailstate if flag set
	if (ptype->flags & PT_NOSTATE)
		tsk = NULL;

	// trailstate allocation/deallocation
	if (tsk)
	{
		// if *tsk = NULL get a new one
		if (*tsk == NULL)
		{
			ts = P_NewTrailstate(tsk);
			*tsk = ts;
		}
		else
		{
			ts = *tsk;

			if (ts->key != tsk) // trailstate was overwritten
			{
				ts = P_NewTrailstate(tsk); // so get a new one
				*tsk = ts;
			}
		}
	}
	else
		ts = NULL;

	if (ptype->type == PT_DECAL)
	{
		clippeddecal_t *d;
		int decalcount;
		float dist;
		vec3_t tangent, t2;
		vec3_t vec={0.5, 0.5, 0.5};
		float *decverts;
		int i;
		trace_t tr;

		vec3_t bestdir;

		if (!free_decals)
			return 0;

		if (!dir)
		{
			bestdir[0] = 0;
			bestdir[1] = 0.73;
			bestdir[2] = 0.73;
			dist = 1;
			for (i = 0; i < 6; i++)
			{
				if (i >= 3)
				{
					t2[0] = ((i&3)==0)*8;
					t2[1] = ((i&3)==1)*8;
					t2[2] = ((i&3)==2)*8;
				}
				else
				{
					t2[0] = -((i&3)==0)*8;
					t2[1] = -((i&3)==1)*8;
					t2[2] = -((i&3)==2)*8;
				}
				VectorSubtract(org, t2, tangent);
				VectorAdd(org, t2, t2);

				if (cl.worldmodel->funcs.Trace (cl.worldmodel, 0, 0,tangent, t2, vec3_origin, vec3_origin, &tr))
				{
					if (tr.fraction < dist)
					{
						dist = tr.fraction;
						VectorCopy(tr.plane.normal, bestdir);
					}
				}
			}
			dir = bestdir;
		}
		VectorInverse(dir);

		VectorNormalize(vec);
		CrossProduct(dir, vec, tangent);
		CrossProduct(dir, tangent, t2);

		decalcount = Q1BSP_ClipDecal(org, dir, tangent, t2, ptype->scale, &decverts);
		while(decalcount)
		{
			if (!free_decals)
				break;

			d = free_decals;
			free_decals = d->next;
			d->next = ptype->clippeddecals;
			ptype->clippeddecals = d;

			VectorCopy((decverts+0), d->vertex[0]);
			VectorCopy((decverts+3), d->vertex[1]);
			VectorCopy((decverts+6), d->vertex[2]);

			for (i = 0; i < 3; i++)
			{
				VectorSubtract(d->vertex[i], org, vec);
				d->texcoords[i][0] = (DotProduct(vec, t2)/ptype->scale)+0.5;
				d->texcoords[i][1] = (DotProduct(vec, tangent)/ptype->scale)+0.5;
			}

			d->die = ptype->randdie*frandom();

			if (ptype->die)
				d->alpha = ptype->alpha-d->die*(ptype->alpha/ptype->die)*ptype->alphachange;
			else
				d->alpha = ptype->alpha;

			if (ptype->colorindex >= 0)
			{
				int cidx;
				cidx = ptype->colorrand > 0 ? rand() % ptype->colorrand : 0;
				cidx = ptype->colorindex + cidx;
				if (cidx > 255)
					d->alpha = d->alpha / 2; // Hexen 2 style transparency
				cidx = (cidx & 0xff) * 3;
				d->rgb[0] = host_basepal[cidx] * (1/255.0);
				d->rgb[1] = host_basepal[cidx+1] * (1/255.0);
				d->rgb[2] = host_basepal[cidx+2] * (1/255.0);
			}
			else
				VectorCopy(ptype->rgb, d->rgb);

			vec[2] = frandom();
			vec[0] = vec[2]*ptype->rgbrandsync[0] + frandom()*(1-ptype->rgbrandsync[0]);
			vec[1] = vec[2]*ptype->rgbrandsync[1] + frandom()*(1-ptype->rgbrandsync[1]);
			vec[2] = vec[2]*ptype->rgbrandsync[2] + frandom()*(1-ptype->rgbrandsync[2]);
			d->rgb[0] += vec[0]*ptype->rgbrand[0] + ptype->rgbchange[0]*d->die;
			d->rgb[1] += vec[1]*ptype->rgbrand[1] + ptype->rgbchange[1]*d->die;
			d->rgb[2] += vec[2]*ptype->rgbrand[2] + ptype->rgbchange[2]*d->die;

			d->die = particletime + ptype->die - d->die;

			decverts += 3*3;
			decalcount--;
		}

		return 0;
	}

	// get msvc to shut up
	j = k = l = 0;
	m = 0;

	while(ptype)
	{
		// init spawn specific variables
		b = bfirst = NULL;
		spawnspc = 8;
		pcount = count*(ptype->count+ptype->countrand*frandom());
		if (ptype->flags & PT_INVFRAMETIME)
			pcount /= host_frametime;
		if (ts)
			pcount += ts->emittime;

		switch (ptype->spawnmode)
		{
		case SM_UNICIRCLE:
			m = pcount;
			if (ptype->type == PT_BEAM)
				m--;

			if (m < 1)
				m = 0;
			else
				m = (M_PI*2)/m;

			if (ptype->spawnparam1) /* use for weird shape hacks */
				m *= ptype->spawnparam1;
			break;
		case SM_TELEBOX:
			spawnspc = 4;
			l = -ptype->areaspreadvert;
		case SM_LAVASPLASH:
			j = k = -ptype->areaspread;
			if (ptype->spawnparam1)
				m = ptype->spawnparam1;
			else
				m = 0.55752; /* default weird number for tele/lavasplash used in vanilla Q1 */

			if (ptype->spawnparam2)
				spawnspc = (int)ptype->spawnparam2;
			break;
		case SM_FIELD:
			if (!avelocities[0][0])
			{
				for (j=0 ; j<NUMVERTEXNORMALS*2 ; j++)
					avelocities[0][j] = (rand()&255) * 0.01;
			}

			j = 0;
			m = 0;
			break;
		default:	//others don't need intitialisation
			break;
		}

		// time limit (for completeness)
		if (ptype->spawntime && ts)
		{
			if (ts->statetime > particletime)
				return 0; // timelimit still in effect

			ts->statetime = particletime + ptype->spawntime; // record old time
		}

		// random chance for point effects
		if (ptype->spawnchance < frandom())
		{
			i = ceil(pcount);
			break;
		}

		// particle spawning loop
		for (i = 0; i < pcount; i++)
		{
			if (!free_particles)
				break;
			p = free_particles;
			if (ptype->type == PT_BEAM)
			{
				if (!free_beams)
					break;
				if (b)
				{
					b = b->next = free_beams;
					free_beams = free_beams->next;
				}
				else
				{
					b = bfirst = free_beams;
					free_beams = free_beams->next;
				}
				b->texture_s = i; // TODO: FIX THIS NUMBER
				b->flags = 0;
				b->p = p;
				VectorClear(b->dir);
			}
			free_particles = p->next;
			p->next = ptype->particles;
			ptype->particles = p;

			p->die = ptype->randdie*frandom();
			p->scale = ptype->scale+ptype->randscale*frandom();
			if (ptype->die)
				p->alpha = ptype->alpha-p->die*(ptype->alpha/ptype->die)*ptype->alphachange;
			else
				p->alpha = ptype->alpha;
			// p->color = 0;
			p->nextemit = particletime + ptype->emitstart - p->die;
			if (ptype->emittime < 0)
				p->trailstate = NULL;

			p->rotationspeed = ptype->rotationmin + frandom()*ptype->rotationrand;
			p->angle = ptype->rotationstartmin + frandom()*ptype->rotationstartrand;

			if (ptype->colorindex >= 0)
			{
				int cidx;
				cidx = ptype->colorrand > 0 ? rand() % ptype->colorrand : 0;
				cidx = ptype->colorindex + cidx;
				if (cidx > 255)
					p->alpha = p->alpha / 2; // Hexen 2 style transparency
				cidx = (cidx & 0xff) * 3;
				p->rgb[0] = host_basepal[cidx] * (1/255.0);
				p->rgb[1] = host_basepal[cidx+1] * (1/255.0);
				p->rgb[2] = host_basepal[cidx+2] * (1/255.0);
			}
			else
				VectorCopy(ptype->rgb, p->rgb);

			// use org temporarily for rgbsync
			p->org[2] = frandom();
			p->org[0] = p->org[2]*ptype->rgbrandsync[0] + frandom()*(1-ptype->rgbrandsync[0]);
			p->org[1] = p->org[2]*ptype->rgbrandsync[1] + frandom()*(1-ptype->rgbrandsync[1]);
			p->org[2] = p->org[2]*ptype->rgbrandsync[2] + frandom()*(1-ptype->rgbrandsync[2]);

			p->rgb[0] += p->org[0]*ptype->rgbrand[0] + ptype->rgbchange[0]*p->die;
			p->rgb[1] += p->org[1]*ptype->rgbrand[1] + ptype->rgbchange[1]*p->die;
			p->rgb[2] += p->org[2]*ptype->rgbrand[2] + ptype->rgbchange[2]*p->die;

			// randomvel
			p->vel[0] = crandom()*ptype->randomvel;
			p->vel[1] = crandom()*ptype->randomvel;
			p->vel[2] = crandom()*ptype->randomvelvert;

			// handle spawn modes (org/vel)
			switch (ptype->spawnmode)
			{
			case SM_BOX:
				ofsvec[0] = crandom();
				ofsvec[1] = crandom();
				ofsvec[2] = crandom();

				arsvec[0] = ofsvec[0]*ptype->areaspread;
				arsvec[1] = ofsvec[1]*ptype->areaspread;
				arsvec[2] = ofsvec[2]*ptype->areaspreadvert;
				break;
			case SM_TELEBOX:
				ofsvec[0] = k;
				ofsvec[1] = j;
				ofsvec[2] = l+4;
				VectorNormalize(ofsvec);
				VectorScale(ofsvec, 1.0-(frandom())*m, ofsvec);

				// org is just like the original
				arsvec[0] = j + (rand()%spawnspc);
				arsvec[1] = k + (rand()%spawnspc);
				arsvec[2] = l + (rand()%spawnspc);

				// advance telebox loop
				j += spawnspc;
				if (j >= ptype->areaspread)
				{
					j = -ptype->areaspread;
					k += spawnspc;
					if (k >= ptype->areaspread)
					{
						k = -ptype->areaspread;
						l += spawnspc;
						if (l >= ptype->areaspreadvert)
							l = -ptype->areaspreadvert;
					}
				}
				break;
			case SM_LAVASPLASH:
				// calc directions, org with temp vector
				ofsvec[0] = k + (rand()%spawnspc);
				ofsvec[1] = j + (rand()%spawnspc);
				ofsvec[2] = 256;

				arsvec[0] = ofsvec[0];
				arsvec[1] = ofsvec[1];
				arsvec[2] = frandom()*ptype->areaspreadvert;

				VectorNormalize(ofsvec);
				VectorScale(ofsvec, 1.0-(frandom())*m, ofsvec);

				// advance splash loop
				j += spawnspc;
				if (j >= ptype->areaspread)
				{
					j = -ptype->areaspread;
					k += spawnspc;
					if (k >= ptype->areaspread)
						k = -ptype->areaspread;
				}
				break;
			case SM_UNICIRCLE:
				ofsvec[0] = cos(m*i);
				ofsvec[1] = sin(m*i);
				ofsvec[2] = 0;
				VectorScale(ofsvec, ptype->areaspread, arsvec);
				break;
			case SM_FIELD:
				arsvec[0] = cl.time * (avelocities[i][0] + m);
				arsvec[1] = cl.time * (avelocities[i][1] + m);
				arsvec[2] = cos(arsvec[1]);

				ofsvec[0] = arsvec[2]*cos(arsvec[0]);
				ofsvec[1] = arsvec[2]*sin(arsvec[0]);
				ofsvec[2] = -sin(arsvec[1]);

				arsvec[0] = r_avertexnormals[j][0]*ptype->areaspread + ofsvec[0]*BEAMLENGTH;
				arsvec[1] = r_avertexnormals[j][1]*ptype->areaspread + ofsvec[1]*BEAMLENGTH;
				arsvec[2] = r_avertexnormals[j][2]*ptype->areaspreadvert + ofsvec[2]*BEAMLENGTH;

				VectorNormalize(ofsvec);

				j++;
				if (j >= NUMVERTEXNORMALS)
				{
					j = 0;
					m += 0.1762891; // some BS number to try to "randomize" things
				}
				break;
			case SM_DISTBALL:
				{
					float rdist;

					rdist = ptype->spawnparam2 - crandom()*(1-(crandom() * ptype->spawnparam1));

					// this is a strange spawntype, which is based on the fact that
					// crandom()*crandom() provides something similar to an exponential
					// probability curve
					ofsvec[0] = hrandom();
					ofsvec[1] = hrandom();
					if (ptype->areaspreadvert)
						ofsvec[2] = hrandom();
					else
						ofsvec[2] = 0;

					VectorNormalize(ofsvec);
					VectorScale(ofsvec, rdist, ofsvec);

					arsvec[0] = ofsvec[0]*ptype->areaspread;
					arsvec[1] = ofsvec[1]*ptype->areaspread;
					arsvec[2] = ofsvec[2]*ptype->areaspreadvert;
				}
				break;
			default: // SM_BALL, SM_CIRCLE
				ofsvec[0] = hrandom();
				ofsvec[1] = hrandom();
				if (ptype->areaspreadvert)
					ofsvec[2] = hrandom();
				else
					ofsvec[2] = 0;

				VectorNormalize(ofsvec);
				if (ptype->spawnmode != SM_CIRCLE)
					VectorScale(ofsvec, frandom(), ofsvec);

				arsvec[0] = ofsvec[0]*ptype->areaspread;
				arsvec[1] = ofsvec[1]*ptype->areaspread;
				arsvec[2] = ofsvec[2]*ptype->areaspreadvert;
				break;
			}

			p->org[0] = org[0] + arsvec[0];
			p->org[1] = org[1] + arsvec[1];
			p->org[2] = org[2] + arsvec[2] + ptype->offsetup;

			// apply arsvec+ofsvec
			if (dir)
			{
				p->vel[0] += dir[0]*ptype->veladd+ofsvec[0]*ptype->offsetspread;
				p->vel[1] += dir[1]*ptype->veladd+ofsvec[1]*ptype->offsetspread;
				p->vel[2] += dir[2]*ptype->veladd+ofsvec[2]*ptype->offsetspreadvert;

				p->org[0] += dir[0]*ptype->orgadd;
				p->org[1] += dir[1]*ptype->orgadd;
				p->org[2] += dir[2]*ptype->orgadd;
			}
			else
			{
				p->vel[0] += ofsvec[0]*ptype->offsetspread;
				p->vel[1] += ofsvec[1]*ptype->offsetspread;
				p->vel[2] += ofsvec[2]*ptype->offsetspreadvert - ptype->veladd;

				p->org[2] -= ptype->orgadd;
			}

			p->die = particletime + ptype->die - p->die;
		}

		// update beam list
		if (ptype->type == PT_BEAM)
		{
			if (b)
			{
				// update dir for bfirst for certain modes since it will never get updated
				switch (ptype->spawnmode)
				{
				case SM_UNICIRCLE:
					// kinda hackish here, assuming ofsvec contains the point at i-1
					arsvec[0] = cos(m*(i-2));
					arsvec[1] = sin(m*(i-2));
					arsvec[2] = 0;
					VectorSubtract(ofsvec, arsvec, bfirst->dir);
					VectorNormalize(bfirst->dir);
					break;
				default:
					break;
				}

				b->flags |= BS_NODRAW;
				b->next = ptype->beams;
				ptype->beams = bfirst;
			}
		}

		// save off emit times in trailstate
		if (ts)
			ts->emittime = pcount - i;

		// go to next associated effect
		if (ptype->assoc < 0)
			break;

		// new trailstate
		if (ts)
		{
			tsk = &(ts->assoc);
			// if *tsk = NULL get a new one
			if (*tsk == NULL)
			{
				ts = P_NewTrailstate(tsk);
				*tsk = ts;
			}
			else
			{
				ts = *tsk;

				if (ts->key != tsk) // trailstate was overwritten
				{
					ts = P_NewTrailstate(tsk); // so get a new one
					*tsk = ts;
				}
			}
		}

		ptype = &part_type[ptype->assoc];
	}

	return 0;
}

void P_BlobExplosion (vec3_t org)
{
	P_RunParticleEffectType(org, NULL, 1, pt_blob);
}

int P_RunParticleEffectTypeString (vec3_t org, vec3_t dir, float count, char *name)
{
	int type = P_FindParticleType(name);
	if (type < 0)
		return 1;

	return P_RunParticleEffectType(org, dir, count, type);
}

void P_EmitEffect (vec3_t pos, int type, trailstate_t **tsk)
{
#ifdef SIDEVIEWS
// is this even needed?
//	if (r_secondaryview==1)
//		return;
#endif
	if (cl.paused)
		return;

 	P_RunParticleEffectState(pos, NULL, host_frametime, type, tsk);
}

/*
===============
P_RunParticleEffect

===============
*/
void P_RunParticleEffect (vec3_t org, vec3_t dir, int color, int count)
{
	int ptype;

	ptype = P_FindParticleType(va("pe_%i", color));
	if (P_RunParticleEffectType(org, dir, count, ptype))
	{
		color &= ~0x7;
		if (count > 130 && part_type[pe_size3].loaded)
		{
			part_type[pe_size3].colorindex = color;
			part_type[pe_size3].colorrand = 8;
			P_RunParticleEffectType(org, dir, count, pe_size3);
			return;
		}
		if (count > 20 && part_type[pe_size2].loaded)
		{
			part_type[pe_size2].colorindex = color;
			part_type[pe_size2].colorrand = 8;
			P_RunParticleEffectType(org, dir, count, pe_size2);
			return;
		}
		part_type[pe_default].colorindex = color;
		part_type[pe_default].colorrand = 8;
		P_RunParticleEffectType(org, dir, count, pe_default);
		return;
	}
}

//h2 stylie
void P_RunParticleEffect2 (vec3_t org, vec3_t dmin, vec3_t dmax, int color, int effect, int count)
{
	int			i, j;
	float		num;
	float invcount;
	vec3_t	nvel;

	int ptype = P_FindParticleType(va("pe2_%i_%i", effect, color));
	if (ptype < 0)
	{
		ptype = P_FindParticleType(va("pe2_%i", effect));
		if (ptype < 0)
			ptype = pe_default;

		part_type[ptype].colorindex = color;
	}

	invcount = 1/part_type[ptype].count; // using this to get R_RPET to always spawn 1
	count = count * part_type[ptype].count;

	for (i=0 ; i<count ; i++)
	{
		if (!free_particles)
			return;

		for (j=0 ; j<3 ; j++)
		{
			num = rand() / (float)RAND_MAX;
			nvel[j] = dmin[j] + ((dmax[j] - dmin[j]) * num);
		}
		P_RunParticleEffectType(org, nvel, invcount, ptype);

	}
}

/*
===============
P_RunParticleEffect3

===============
*/
//h2 stylie
void P_RunParticleEffect3 (vec3_t org, vec3_t box, int color, int effect, int count)
{
	int			i, j;
	vec3_t	nvel;
	float		num;
	float invcount;

	int ptype = P_FindParticleType(va("pe3_%i_%i", effect, color));
	if (ptype < 0)
	{
		ptype = P_FindParticleType(va("pe3_%i", effect));
		if (ptype < 0)
			ptype = pe_default;

		part_type[ptype].colorindex = color;
	}

	invcount = 1/part_type[ptype].count; // using this to get R_RPET to always spawn 1
	count = count * part_type[ptype].count;

	for (i=0 ; i<count ; i++)
	{
		if (!free_particles)
			return;

		for (j=0 ; j<3 ; j++)
		{
			num = rand() / (float)RAND_MAX;
			nvel[j] = (box[j] * num * 2) - box[j];
		}

		P_RunParticleEffectType(org, nvel, invcount, ptype);
	}
}

/*
===============
P_RunParticleEffect4

===============
*/
//h2 stylie
void P_RunParticleEffect4 (vec3_t org, float radius, int color, int effect, int count)
{
	int			i, j;
	vec3_t	nvel;
	float		num;
	float invcount;

	int ptype = P_FindParticleType(va("pe4_%i_%i", effect, color));
	if (ptype < 0)
	{
		ptype = P_FindParticleType(va("pe4_%i", effect));
		if (ptype < 0)
			ptype = pe_default;

		part_type[ptype].colorindex = color;
	}

	invcount = 1/part_type[ptype].count; // using this to get R_RPET to always spawn 1
	count = count * part_type[ptype].count;

	for (i=0 ; i<count ; i++)
	{
		if (!free_particles)
			return;

		for (j=0 ; j<3 ; j++)
		{
			num = rand() / (float)RAND_MAX;
			nvel[j] = (radius * num * 2) - radius;
		}
		P_RunParticleEffectType(org, nvel, invcount, ptype);
	}
}

void P_RunParticleCube(vec3_t minb, vec3_t maxb, vec3_t dir, float count, int colour, qboolean gravity, float jitter)
{
	vec3_t org;
	int			i, j;
	float		num;
	float invcount;

	int ptype = P_FindParticleType(va("te_cube%s_%i", gravity?"_g":"", colour));
	if (ptype < 0)
	{
		ptype = P_FindParticleType(va("te_cube%s", gravity?"_g":""));
		if (ptype < 0)
			ptype = pe_default;

		part_type[ptype].colorindex = colour;
	}

	invcount = 1/part_type[ptype].count; // using this to get R_RPET to always spawn 1
	count = count * part_type[ptype].count;

	for (i=0 ; i<count ; i++)
	{
		if (!free_particles)
			return;

		for (j=0 ; j<3 ; j++)
		{
			num = rand() / (float)RAND_MAX;
			org[j] = minb[j] + num*(maxb[j]-minb[j]);
		}
		P_RunParticleEffectType(org, dir, invcount, ptype);
	}
}

void P_RunParticleWeather(vec3_t minb, vec3_t maxb, vec3_t dir, float count, int colour, char *efname)
{
	vec3_t org;
	int			i, j;
	float		num;
	float invcount;

	int ptype = P_FindParticleType(va("te_%s_%i", efname, colour));
	if (ptype < 0)
	{
		ptype = P_FindParticleType(va("te_%s", efname));
		if (ptype < 0)
			ptype = pe_default;

		part_type[ptype].colorindex = colour;
	}

	invcount = 1/part_type[ptype].count; // using this to get R_RPET to always spawn 1
	count = count * part_type[ptype].count;

	for (i=0 ; i<count ; i++)
	{
		if (!free_particles)
			return;

		for (j=0 ; j<3 ; j++)
		{
			num = rand() / (float)RAND_MAX;
			org[j] = minb[j] + num*(maxb[j]-minb[j]);
		}
		P_RunParticleEffectType(org, dir, invcount, ptype);
	}
}

/*
===============
R_LavaSplash

===============
*/
void P_LavaSplash (vec3_t org)
{
	P_RunParticleEffectType(org, NULL, 1, pt_lavasplash);
}

static void P_ParticleTrailDraw (vec3_t startpos, vec3_t end, part_type_t *ptype, trailstate_t **tsk)
{
	vec3_t	vec, vstep, right, up, start;
	float	len;
	int			tcount;
	particle_t	*p;
	beamseg_t   *b;
	beamseg_t   *bfirst;
	trailstate_t *ts;

	float veladd = -ptype->veladd;
	float randvel = ptype->randomvel;
	float randvelvert = ptype->randomvelvert;
	float step;
	float stop;
	float tdegree = 2*M_PI/256; /* MSVC whine */
	float nrfirst, nrlast;

	VectorCopy(startpos, start);

	// eliminate trailstate if flag set
	if (ptype->flags & PT_NOSTATE)
		tsk = NULL;

	// trailstate allocation/deallocation
	if (tsk)
	{
		// if *tsk = NULL get a new one
		if (*tsk == NULL)
		{
			ts = P_NewTrailstate(tsk);
			*tsk = ts;
		}
		else
		{
			ts = *tsk;

			if (ts->key != tsk) // trailstate was overwritten
			{
				ts = P_NewTrailstate(tsk); // so get a new one
				*tsk = ts;
			}
		}
	}
	else
		ts = NULL;

	if (ptype->assoc>=0)
	{
		if (ts)
			P_ParticleTrail(start, end, ptype->assoc, &(ts->assoc));
		else
			P_ParticleTrail(start, end, ptype->assoc, NULL);
	}

	// time limit for trails
	if (ptype->spawntime && ts)
	{
		if (ts->statetime > particletime)
			return; // timelimit still in effect

		ts->statetime = particletime + ptype->spawntime; // record old time
		ts = NULL; // clear trailstate so we don't save length/lastseg
	}

	// random chance for trails
	if (ptype->spawnchance < frandom())
		return; // don't spawn but return success

	if (!ptype->die)
		ts = NULL;

	// use ptype step to calc step vector and step size
	step = 1/ptype->count;

	if (step < 0.01)
		step = 0.01;

	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	if (ptype->flags & PT_AVERAGETRAIL)
	{
		float tavg;
		// mangle len/step to get last point to be at end
		tavg = len / step;
		tavg = tavg / ceil(tavg);
		step *= tavg;
		len += step;
	}

	VectorScale(vec, step, vstep);

	// add offset
	start[2] += ptype->offsetup;

	if (ptype->spawnmode == SM_SPIRAL)
	{
		VectorVectors(vec, right, up);

		// precalculate degree of rotation
		if (ptype->spawnparam1)
			tdegree = 2*M_PI/ptype->spawnparam1; /* distance per rotation inversed */
	}

	// store last stop here for lack of a better solution besides vectors
	if (ts)
	{
		ts->laststop = stop = ts->laststop + len;	//when to stop
		len = ts->lastdist;
	}
	else
	{
		stop = len;
		len = 0;
	}

//	len = ts->lastdist/step;
//	len = (len - (int)len)*step;
//	VectorMA (start, -len, vec, start);

	if (ptype->flags & PT_NOSPREADFIRST)
		nrfirst = len + step*1.5;
	else
		nrfirst = len;

	if (ptype->flags & PT_NOSPREADLAST)
		nrlast = stop;
	else
		nrlast = stop + step;

	b = bfirst = NULL;

	while (len < stop)
	{
		len += step;

		if (!free_particles)
		{
			len = stop;
			break;
		}

		p = free_particles;
		if (ptype->type == PT_BEAM)
		{
			if (!free_beams)
			{
				len = stop;
				break;
			}
			if (b)
			{
				b = b->next = free_beams;
				free_beams = free_beams->next;
			}
			else
			{
				b = bfirst = free_beams;
				free_beams = free_beams->next;
			}
			b->texture_s = len; // not sure how to calc this
			b->flags = 0;
			b->p = p;
			VectorCopy(vec, b->dir);
		}

		free_particles = p->next;
		p->next = ptype->particles;
		ptype->particles = p;

		p->die = ptype->randdie*frandom();
		p->scale = ptype->scale+ptype->randscale*frandom();
		if (ptype->die)
			p->alpha = ptype->alpha-p->die*(ptype->alpha/ptype->die)*ptype->alphachange;
		else
			p->alpha = ptype->alpha;
//		p->color = 0;

//		if (ptype->spawnmode == SM_TRACER)
		if (ptype->spawnparam1)
			tcount = (int)(len * ptype->count / ptype->spawnparam1);
		else
			tcount = (int)(len * ptype->count);

		if (ptype->colorindex >= 0)
		{
			int cidx;
			cidx = ptype->colorrand > 0 ? rand() % ptype->colorrand : 0;
			if (ptype->flags & PT_CITRACER) // colorindex behavior as per tracers in std Q1
				cidx += ((tcount & 4) << 1);

			cidx = ptype->colorindex + cidx;
			if (cidx > 255)
				p->alpha = p->alpha / 2;
			cidx = (cidx & 0xff) * 3;
			p->rgb[0] = host_basepal[cidx] * (1/255.0);
			p->rgb[1] = host_basepal[cidx+1] * (1/255.0);
			p->rgb[2] = host_basepal[cidx+2] * (1/255.0);
		}
		else
			VectorCopy(ptype->rgb, p->rgb);

		// use org temporarily for rgbsync
		p->org[2] = frandom();
		p->org[0] = p->org[2]*ptype->rgbrandsync[0] + frandom()*(1-ptype->rgbrandsync[0]);
		p->org[1] = p->org[2]*ptype->rgbrandsync[1] + frandom()*(1-ptype->rgbrandsync[1]);
		p->org[2] = p->org[2]*ptype->rgbrandsync[2] + frandom()*(1-ptype->rgbrandsync[2]);
		if (ptype->orgadd)
		{
			p->org[0] += vec[0]*ptype->orgadd;
			p->org[1] += vec[1]*ptype->orgadd;
			p->org[2] += vec[2]*ptype->orgadd;
		}

		p->rgb[0] += p->org[0]*ptype->rgbrand[0] + ptype->rgbchange[0]*p->die;
		p->rgb[1] += p->org[1]*ptype->rgbrand[1] + ptype->rgbchange[1]*p->die;
		p->rgb[2] += p->org[2]*ptype->rgbrand[2] + ptype->rgbchange[2]*p->die;

		VectorCopy (vec3_origin, p->vel);
		p->nextemit = particletime + ptype->emitstart - p->die;
		if (ptype->emittime < 0)
			p->trailstate = NULL; // init trailstate

		p->rotationspeed = ptype->rotationmin + frandom()*ptype->rotationrand;
		p->angle = ptype->rotationstartmin + frandom()*ptype->rotationstartrand;

		if (len < nrfirst || len >= nrlast)
		{
			// no offset or areaspread for these particles...
			p->vel[0] = vec[0]*veladd+crandom()*randvel;
			p->vel[1] = vec[1]*veladd+crandom()*randvel;
			p->vel[2] = vec[2]*veladd+crandom()*randvelvert;

			VectorCopy(start, p->org);
		}
		else
		{
			switch(ptype->spawnmode)
			{
			case SM_TRACER:
				if (tcount & 1)
				{
					p->vel[0] = vec[1]*ptype->offsetspread;
					p->vel[1] = -vec[0]*ptype->offsetspread;
					p->org[0] = vec[1]*ptype->areaspread;
					p->org[1] = -vec[0]*ptype->areaspread;
				}
				else
				{
					p->vel[0] = -vec[1]*ptype->offsetspread;
					p->vel[1] = vec[0]*ptype->offsetspread;
					p->org[0] = -vec[1]*ptype->areaspread;
					p->org[1] = vec[0]*ptype->areaspread;
				}

				p->vel[0] += vec[0]*veladd+crandom()*randvel;
				p->vel[1] += vec[1]*veladd+crandom()*randvel;
				p->vel[2] = vec[2]*veladd+crandom()*randvelvert;

				p->org[0] += start[0];
				p->org[1] += start[1];
				p->org[2] = start[2];
				break;
			case SM_SPIRAL:
				{
					float tsin, tcos;

					tcos = cos(len*tdegree)*ptype->areaspread;
					tsin = sin(len*tdegree)*ptype->areaspread;

					p->org[0] = start[0] + right[0]*tcos + up[0]*tsin;
					p->org[1] = start[1] + right[1]*tcos + up[1]*tsin;
					p->org[2] = start[2] + right[2]*tcos + up[2]*tsin;

					tcos = cos(len*tdegree)*ptype->offsetspread;
					tsin = sin(len*tdegree)*ptype->offsetspread;

					p->vel[0] = vec[0]*veladd+crandom()*randvel + right[0]*tcos + up[0]*tsin;
					p->vel[1] = vec[1]*veladd+crandom()*randvel + right[1]*tcos + up[1]*tsin;
					p->vel[2] = vec[2]*veladd+crandom()*randvelvert + right[2]*tcos + up[2]*tsin;
				}
				break;
			// TODO: directionalize SM_BALL/SM_CIRCLE/SM_DISTBALL
			case SM_BALL:
			case SM_CIRCLE:
				p->org[0] = crandom();
				p->org[1] = crandom();
				p->org[2] = crandom();
				VectorNormalize(p->org);
				if (ptype->spawnmode != SM_CIRCLE)
					VectorScale(p->org, frandom(), p->org);

				p->vel[0] = vec[0]*veladd+crandom()*randvel + p->org[0]*ptype->offsetspread;
				p->vel[1] = vec[1]*veladd+crandom()*randvel + p->org[1]*ptype->offsetspread;
				p->vel[2] = vec[2]*veladd+crandom()*randvelvert + p->org[2]*ptype->offsetspreadvert;

				p->org[0] = p->org[0]*ptype->areaspread + start[0];
				p->org[1] = p->org[1]*ptype->areaspread + start[1];
				p->org[2] = p->org[2]*ptype->areaspreadvert + start[2];
				break;
			case SM_DISTBALL:
				{
					float rdist;

					rdist = ptype->spawnparam2 - crandom()*(1-(crandom() * ptype->spawnparam1));

					// this is a strange spawntype, which is based on the fact that
					// crandom()*crandom() provides something similar to an exponential
					// probability curve
					p->org[0] = crandom();
					p->org[1] = crandom();
					p->org[2] = crandom();

					VectorNormalize(p->org);
					VectorScale(p->org, rdist, p->org);

					p->vel[0] = vec[0]*veladd+crandom()*randvel + p->org[0]*ptype->offsetspread;
					p->vel[1] = vec[1]*veladd+crandom()*randvel + p->org[1]*ptype->offsetspread;
					p->vel[2] = vec[2]*veladd+crandom()*randvelvert + p->org[2]*ptype->offsetspreadvert;

					p->org[0] = p->org[0]*ptype->areaspread + start[0];
					p->org[1] = p->org[1]*ptype->areaspread + start[1];
					p->org[2] = p->org[2]*ptype->areaspreadvert + start[2];
				}
				break;
			default:
				p->org[0] = crandom();
				p->org[1] = crandom();
				p->org[2] = crandom();

				p->vel[0] = vec[0]*veladd+crandom()*randvel + p->org[0]*ptype->offsetspread;
				p->vel[1] = vec[1]*veladd+crandom()*randvel + p->org[1]*ptype->offsetspread;
				p->vel[2] = vec[2]*veladd+crandom()*randvelvert + p->org[2]*ptype->offsetspreadvert;

				p->org[0] = p->org[0]*ptype->areaspread + start[0];
				p->org[1] = p->org[1]*ptype->areaspread + start[1];
				p->org[2] = p->org[2]*ptype->areaspreadvert + start[2];
				break;
			}
		}

		VectorAdd (start, vstep, start);

		if (ptype->countrand)
		{
			float rstep = frandom() / ptype->countrand;
			VectorMA(start, rstep, vec, start);
			step += rstep;
		}

		p->die = particletime + ptype->die - p->die;
	}

	if (ts)
	{
		ts->lastdist = len;

		// update beamseg list
		if (ptype->type == PT_BEAM)
		{
			if (b)
			{
				if (ptype->beams)
				{
					if (ts->lastbeam)
					{
						b->next = ts->lastbeam->next;
						ts->lastbeam->next = bfirst;
						ts->lastbeam->flags &= ~BS_LASTSEG;
					}
					else
					{
						b->next = ptype->beams;
						ptype->beams = bfirst;
					}
				}
				else
				{
					ptype->beams = bfirst;
					b->next = NULL;
				}

				b->flags |= BS_LASTSEG;
				ts->lastbeam = b;
			}

			if ((!free_particles || !free_beams) && ts->lastbeam)
			{
				ts->lastbeam->flags &= ~BS_LASTSEG;
				ts->lastbeam->flags |= BS_NODRAW;
				ts->lastbeam = NULL;
			}
		}
	}
	else if (ptype->type == PT_BEAM)
	{
		if (b)
		{
			b->flags |= BS_NODRAW;
			b->next = ptype->beams;
			ptype->beams = bfirst;
		}
	}

	return;
}

int P_ParticleTrail (vec3_t startpos, vec3_t end, int type, trailstate_t **tsk)
{
	part_type_t *ptype = &part_type[type];

	if (type < 0 || type >= numparticletypes)
		return 1;	//bad value

	if (!ptype->loaded)
		return 1;

	// inwater check, switch only once
	if (r_part_contentswitch.value && ptype->inwater >= 0)
	{
		int cont;
		cont = cl.worldmodel->funcs.PointContents(cl.worldmodel, startpos);

		if (cont & FTECONTENTS_WATER)
			ptype = &part_type[ptype->inwater];
	}

	P_ParticleTrailDraw (startpos, end, ptype, tsk);
	return 0;
}

void P_ParticleTrailIndex (vec3_t start, vec3_t end, int color, int crnd, trailstate_t **tsk)
{
	part_type[pe_defaulttrail].colorindex = color;
	part_type[pe_defaulttrail].colorrand = crnd;
	P_ParticleTrail(start, end, pe_defaulttrail, tsk);
}

#ifdef Q2BSPS
qboolean Q2TraceLineN (vec3_t start, vec3_t end, vec3_t impact, vec3_t normal)
{
	vec3_t nul = {0,0,0};
	trace_t trace = CM_BoxTrace(pmove.physents[0].model, start, end, nul, nul, MASK_SOLID);

	if (trace.fraction < 1)
	{
		VectorCopy (trace.plane.normal, normal);
		VectorCopy (trace.endpos, impact);
		return true;
	}
	return false;
}
#endif

qboolean DoomTraceLineN (vec3_t start, vec3_t end, vec3_t impact, vec3_t normal)
{
	return false;
}

#if 1	//extra code to make em bounce of doors.
qboolean TraceLineN (vec3_t start, vec3_t end, vec3_t impact, vec3_t normal)
{
	trace_t		trace;
	float len, bestlen;
	int i;
	vec3_t delta, ts, te;
	physent_t *pe;
	hull_t *hull;
	qboolean clipped=false;

	memset (&trace, 0, sizeof(trace));

	VectorSubtract(end, start, delta);
	bestlen = Length(delta);

	VectorCopy (end, impact);

	for (i=0 ; i< pmove.numphysent ; i++)
	{
		pe = &pmove.physents[i];
		if (pe->model)
		{
			hull = &pe->model->hulls[0];
			VectorSubtract(start, pe->origin, ts);
			VectorSubtract(end, pe->origin, te);
			pe->model->funcs.Trace(pe->model, 0, 0, ts, te, vec3_origin, vec3_origin, &trace);
			if (trace.fraction<1)
			{
				VectorSubtract(trace.endpos, ts, delta);
				len = Length(delta);
				if (len < bestlen)
				{
					bestlen = len;
					VectorCopy (trace.plane.normal, normal);
					VectorAdd (pe->origin, trace.endpos, impact);
				}

				clipped=true;
			}
			if (trace.startsolid)
			{
				VectorNormalize(delta);
				normal[0] = -delta[0];
				normal[1] = -delta[1];
				normal[2] = -delta[2];
				VectorCopy (start, impact);
				return true;
			}

		}
	}

	if (clipped)
	{
		return true;
	}
	else
	{
		return false;
	}
}

#else	//basic (faster)
qboolean TraceLineN (vec3_t start, vec3_t end, vec3_t impact, vec3_t normal)
{
	pmtrace_t	trace;

	memset (&trace, 0, sizeof(trace));
	trace.fraction = 1;
	if (cl.worldmodel->hulls->funcs.RecursiveHullCheck (cl.worldmodel->hulls, 0, 0, 1, start, end, &trace))
		return false;

	if (trace.startsolid)
		return true;

	VectorCopy (trace.endpos, impact);
	VectorCopy (trace.plane.normal, normal);

	return true;
}
#endif


part_type_t *lasttype;
static vec3_t pright, pup;
float pframetime;
#ifdef RGLQUAKE
void GL_DrawTexturedParticle(particle_t *p, part_type_t *type)
{
	float x,y;
	float scale;

	if (lasttype != type)
	{
		lasttype = type;
		qglEnd();
		qglEnable(GL_TEXTURE_2D);
		GL_Bind(type->texturenum);
		APPLYBLEND(type->blendmode);
		qglShadeModel(GL_FLAT);
		qglBegin(GL_QUADS);
	}

	scale = (p->org[0] - r_origin[0])*vpn[0] + (p->org[1] - r_origin[1])*vpn[1]
		+ (p->org[2] - r_origin[2])*vpn[2];
	scale = (scale*p->scale)*(type->invscalefactor) + p->scale * (type->scalefactor*250);
	if (scale < 20)
		scale = 0.25;
	else
		scale = 0.25 + scale * 0.001;

	qglColor4f (p->rgb[0],
				p->rgb[1],
				p->rgb[2],
				p->alpha);

	if (p->angle)
	{
		x = sin(p->angle)*scale;
		y = cos(p->angle)*scale;
	}
	else
	{
		x = 0;
		y = scale;
	}
	qglTexCoord2f(0,0);
	qglVertex3f (p->org[0] - x*pright[0] - y*pup[0], p->org[1] - x*pright[1] - y*pup[1], p->org[2] - x*pright[2] - y*pup[2]);
	qglTexCoord2f(0,1);
	qglVertex3f (p->org[0] - y*pright[0] + x*pup[0], p->org[1] - y*pright[1] + x*pup[1], p->org[2] - y*pright[2] + x*pup[2]);
	qglTexCoord2f(1,1);
	qglVertex3f (p->org[0] + x*pright[0] + y*pup[0], p->org[1] + x*pright[1] + y*pup[1], p->org[2] + x*pright[2] + y*pup[2]);
	qglTexCoord2f(1,0);
	qglVertex3f (p->org[0] + y*pright[0] - x*pup[0], p->org[1] + y*pright[1] - x*pup[1], p->org[2] + y*pright[2] - x*pup[2]);
}


void GL_DrawSketchParticle(particle_t *p, part_type_t *type)
{
	float x,y;
	float scale;

	int quant;

	if (lasttype != type)
	{
		lasttype = type;
		qglEnd();
		qglDisable(GL_TEXTURE_2D);
		GL_Bind(type->texturenum);
//		if (type->blendmode == BM_ADD)		//addative
//			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
//		else if (type->blendmode == BM_SUBTRACT)	//subtractive
//			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//		else
			qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		qglShadeModel(GL_SMOOTH);
		qglBegin(GL_LINES);
	}

	scale = (p->org[0] - r_origin[0])*vpn[0] + (p->org[1] - r_origin[1])*vpn[1]
		+ (p->org[2] - r_origin[2])*vpn[2];
	scale = (scale*p->scale)*(type->invscalefactor) + p->scale * (type->scalefactor*250);
	if (scale < 20)
		scale = 0.25;
	else
		scale = 0.25 + scale * 0.001;

	qglColor4f (p->rgb[0]/2,
				p->rgb[1]/2,
				p->rgb[2]/2,
				p->alpha*2);

	quant = scale;

	if (p->angle)
	{
		x = sin(p->angle)*scale;
		y = cos(p->angle)*scale;
	}
	else
	{
		x = 0;
		y = scale;
	}
	qglVertex3f (p->org[0] - x*pright[0] - y*pup[0], p->org[1] - x*pright[1] - y*pup[1], p->org[2] - x*pright[2] - y*pup[2]);
	qglVertex3f (p->org[0] + x*pright[0] + y*pup[0], p->org[1] + x*pright[1] + y*pup[1], p->org[2] + x*pright[2] + y*pup[2]);
	qglVertex3f (p->org[0] + y*pright[0] - x*pup[0], p->org[1] + y*pright[1] - x*pup[1], p->org[2] + y*pright[2] - x*pup[2]);
	qglVertex3f (p->org[0] - y*pright[0] + x*pup[0], p->org[1] - y*pright[1] + x*pup[1], p->org[2] - y*pright[2] + x*pup[2]);
}

void GL_DrawTrifanParticle(particle_t *p, part_type_t *type)
{
	int i;
	vec3_t v;
	float scale;

	qglEnd();

	if (lasttype != type)
	{
		lasttype = type;
		qglDisable(GL_TEXTURE_2D);
		APPLYBLEND(type->blendmode);
		qglShadeModel(GL_SMOOTH);
	}

	scale = (p->org[0] - r_origin[0])*vpn[0] + (p->org[1] - r_origin[1])*vpn[1]
		+ (p->org[2] - r_origin[2])*vpn[2];
	scale = (scale*p->scale)*(type->invscalefactor) + p->scale * (type->scalefactor*250);
	if (scale < 20)
		scale = 0.05;
	else
		scale = 0.05 + scale * 0.0001;
/*
	if ((p->vel[0]*p->vel[0]+p->vel[1]*p->vel[1]+p->vel[2]*p->vel[2])*2*scale > 30*30)
		scale = 1+1/30/Length(p->vel)*2;*/

	qglBegin (GL_TRIANGLE_FAN);
	qglColor4f (p->rgb[0],
				p->rgb[1],
				p->rgb[2],
				p->alpha);
	qglVertex3fv (p->org);
	qglColor4f (p->rgb[0]/2,
				p->rgb[1]/2,
				p->rgb[2]/2,
				0);
	for (i=7 ; i>=0 ; i--)
	{
		v[0] = p->org[0] - p->vel[0]*scale + vright[0]*cost[i%7]*p->scale + vup[0]*sint[i%7]*p->scale;
		v[1] = p->org[1] - p->vel[1]*scale + vright[1]*cost[i%7]*p->scale + vup[1]*sint[i%7]*p->scale;
		v[2] = p->org[2] - p->vel[2]*scale + vright[2]*cost[i%7]*p->scale + vup[2]*sint[i%7]*p->scale;
		qglVertex3fv (v);
	}
	qglEnd ();
	qglBegin (GL_TRIANGLES);
}

void GL_DrawLineSparkParticle(particle_t *p, part_type_t *type)
{
	if (lasttype != type)
	{
		lasttype = type;
		qglEnd();
		qglDisable(GL_TEXTURE_2D);
		GL_Bind(type->texturenum);
		APPLYBLEND(type->blendmode);
		qglShadeModel(GL_SMOOTH);
		qglBegin(GL_LINES);
	}

	qglColor4f (p->rgb[0],
				p->rgb[1],
				p->rgb[2],
				p->alpha);
	qglVertex3f (p->org[0], p->org[1], p->org[2]);

	qglColor4f (p->rgb[0],
				p->rgb[1],
				p->rgb[2],
				0);
	qglVertex3f (p->org[0]-p->vel[0]/10, p->org[1]-p->vel[1]/10, p->org[2]-p->vel[2]/10);
}

void GL_DrawTexturedSparkParticle(particle_t *p, part_type_t *type)
{
	vec3_t v, cr, o2, point;
	if (lasttype != type)
	{
		lasttype = type;
		qglEnd();
		qglEnable(GL_TEXTURE_2D);
		GL_Bind(type->texturenum);
		APPLYBLEND(type->blendmode);
		qglShadeModel(GL_SMOOTH);
		qglBegin(GL_QUADS);
	}

	qglColor4f (p->rgb[0],
				p->rgb[1],
				p->rgb[2],
				p->alpha);

	VectorSubtract(r_refdef.vieworg, p->org, v);
	CrossProduct(v, p->vel, cr);
	VectorNormalize(cr);

	VectorMA(p->org, -p->scale/2, cr, point);
	qglTexCoord2f(0, 0);
	qglVertex3fv(point);
	VectorMA(p->org, p->scale/2, cr, point);
	qglTexCoord2f(0, 1);
	qglVertex3fv(point);


	VectorMA(p->org, 0.1, p->vel, o2);

	VectorSubtract(r_refdef.vieworg, o2, v);
	CrossProduct(v, p->vel, cr);
	VectorNormalize(cr);

	VectorMA(o2, p->scale/2, cr, point);
	qglTexCoord2f(1, 1);
	qglVertex3fv(point);
	VectorMA(o2, -p->scale/2, cr, point);
	qglTexCoord2f(1, 0);
	qglVertex3fv(point);
}

void GL_DrawSketchSparkParticle(particle_t *p, part_type_t *type)
{
	if (lasttype != type)
	{
		lasttype = type;
		qglEnd();
		qglDisable(GL_TEXTURE_2D);
		GL_Bind(type->texturenum);
		APPLYBLEND(type->blendmode);
		qglShadeModel(GL_SMOOTH);
		qglBegin(GL_LINES);
	}

	qglColor4f (p->rgb[0],
				p->rgb[1],
				p->rgb[2],
				p->alpha);
	qglVertex3f (p->org[0], p->org[1], p->org[2]);

	qglColor4f (p->rgb[0],
				p->rgb[1],
				p->rgb[2],
				0);
	qglVertex3f (p->org[0]-p->vel[0]/10, p->org[1]-p->vel[1]/10, p->org[2]-p->vel[2]/10);
}

void GL_DrawParticleBeam_Textured(beamseg_t *b, part_type_t *type)
{
	vec3_t v, point;
	vec3_t cr;
	beamseg_t *c;
	particle_t *p;
	particle_t *q;
	float ts;

//	if (!b->next)
//		return;

	c = b->next;

	q = c->p;
//	if (!q)
//		return;

	p = b->p;
//	if (!p)
//		return;

	if (lasttype != type)
	{
		lasttype = type;
		qglEnd();
		qglEnable(GL_TEXTURE_2D);
		GL_Bind(type->texturenum);
		APPLYBLEND(type->blendmode);
		qglShadeModel(GL_SMOOTH);
		qglBegin(GL_QUADS);
	}
	qglColor4f(q->rgb[0],
			  q->rgb[1],
			  q->rgb[2],
			  q->alpha);
//	qglBegin(GL_LINE_LOOP);
	VectorSubtract(r_refdef.vieworg, q->org, v);
	VectorNormalize(v);
	CrossProduct(c->dir, v, cr);
	ts = (c->texture_s*type->rotationstartmin + particletime*type->rotationmin)/754;

	VectorMA(q->org, -q->scale, cr, point);
	qglTexCoord2f(ts, 0);
	qglVertex3fv(point);
	VectorMA(q->org, q->scale, cr, point);
	qglTexCoord2f(ts, 1);
	qglVertex3fv(point);

	qglColor4f(p->rgb[0],
			  p->rgb[1],
			  p->rgb[2],
			  p->alpha);

	VectorSubtract(r_refdef.vieworg, p->org, v);
	VectorNormalize(v);
	CrossProduct(b->dir, v, cr); // replace with old p->dir?
	ts = (b->texture_s*type->rotationstartmin + particletime*type->rotationmin)/754;

	VectorMA(p->org, p->scale, cr, point);
	qglTexCoord2f(ts, 1);
	qglVertex3fv(point);
	VectorMA(p->org, -p->scale, cr, point);
	qglTexCoord2f(ts, 0);
	qglVertex3fv(point);
//	qglEnd();
}

void GL_DrawParticleBeam_Untextured(beamseg_t *b, part_type_t *type)
{
	vec3_t v;
	vec3_t cr;
	beamseg_t *c;
	particle_t *p;
	particle_t *q;

	vec3_t point[4];

//	if (!b->next)
//		return;

	c = b->next;

	q = c->p;
//	if (!q)
//		return;

	p = b->p;
//	if (!p)
//		return;

	if (lasttype != type)
	{
		lasttype = type;
		qglEnd();
		qglDisable(GL_TEXTURE_2D);
		GL_Bind(type->texturenum);
		APPLYBLEND(type->blendmode);
		qglShadeModel(GL_SMOOTH);
		qglBegin(GL_QUADS);
	}

//	qglBegin(GL_LINE_LOOP);
	VectorSubtract(r_refdef.vieworg, q->org, v);
	VectorNormalize(v);
	CrossProduct(c->dir, v, cr);

	VectorMA(q->org, -q->scale, cr, point[0]);
	VectorMA(q->org, q->scale, cr, point[1]);


	VectorSubtract(r_refdef.vieworg, p->org, v);
	VectorNormalize(v);
	CrossProduct(b->dir, v, cr); // replace with old p->dir?

	VectorMA(p->org, p->scale, cr, point[2]);
	VectorMA(p->org, -p->scale, cr, point[3]);


	//one half
	//back out
	//back in
	//front in
	//front out
	qglColor4f(q->rgb[0],
		  q->rgb[1],
		  q->rgb[2],
		  0);
	qglVertex3fv(point[0]);
	qglColor4f(q->rgb[0],
		  q->rgb[1],
		  q->rgb[2],
		  q->alpha);
	qglVertex3fv(q->org);

	qglColor4f(p->rgb[0],
		  p->rgb[1],
		  p->rgb[2],
		  p->alpha);
	qglVertex3fv(p->org);
	qglColor4f(p->rgb[0],
		  p->rgb[1],
		  p->rgb[2],
		  0);
	qglVertex3fv(point[3]);

	//front out
	//front in
	//back in
	//back out
	qglColor4f(p->rgb[0],
		  p->rgb[1],
		  p->rgb[2],
		  0);
	qglVertex3fv(point[2]);
	qglColor4f(p->rgb[0],
		  p->rgb[1],
		  p->rgb[2],
		  p->alpha);
	qglVertex3fv(p->org);

	qglColor4f(q->rgb[0],
		  q->rgb[1],
		  q->rgb[2],
		  q->alpha);
	qglVertex3fv(q->org);
	qglColor4f(q->rgb[0],
		  q->rgb[1],
		  q->rgb[2],
		  0);
	qglVertex3fv(point[1]);

//	qglEnd();
}

void GL_DrawClippedDecal(clippeddecal_t *d, part_type_t *type)
{
	if (lasttype != type)
	{
		lasttype = type;
		qglEnd();
		qglEnable(GL_TEXTURE_2D);
		GL_Bind(type->texturenum);
		APPLYBLEND(type->blendmode);
		qglShadeModel(GL_SMOOTH);

//		qglDisable(GL_TEXTURE_2D);
//		qglBegin(GL_LINE_LOOP);

		qglBegin(GL_TRIANGLES);
	}

	qglColor4f(d->rgb[0],
		  d->rgb[1],
		  d->rgb[2],
		  d->alpha);

	qglTexCoord2fv(d->texcoords[0]);
	qglVertex3fv(d->vertex[0]);
	qglTexCoord2fv(d->texcoords[1]);
	qglVertex3fv(d->vertex[1]);
	qglTexCoord2fv(d->texcoords[2]);
	qglVertex3fv(d->vertex[2]);
}

#endif
#ifdef SWQUAKE
void SWD_DrawParticleSpark(particle_t *p, part_type_t *type)
{
	float speed;
	vec3_t src, dest;

	int r,g,b;	//if you have a cpu with mmx, good for you...
	r = p->rgb[0]*255;
	if (r < 0)
		r = 0;
	else if (r > 255)
		r = 255;
	g = p->rgb[1]*255;
	if (g < 0)
		g = 0;
	else if (g > 255)
		g = 255;
	b = p->rgb[2]*255;
	if (b < 0)
		b = 0;
	else if (b > 255)
		b = 255;
	p->color = GetPalette(r, g, b);

	speed = Length(p->vel);
	if ((speed) < 1)
	{
		VectorCopy(p->org, src);
		VectorCopy(p->org, dest);
	}
	else
	{	//causes flickers with lower vels (due to bouncing in physics)
		if (speed < 50)
			speed *= 50/speed;
		VectorMA(p->org, 2.5/(speed), p->vel, src);
		VectorMA(p->org, -2.5/(speed), p->vel, dest);
	}

	D_DrawSparkTrans(p, src, dest, type->blendmode);
}
void SWD_DrawParticleBlob(particle_t *p, part_type_t *type)
{
	int r,g,b;	//This really shouldn't be like this. Pitty the 32 bit renderer...
	r = p->rgb[0]*255;
	if (r < 0)
		r = 0;
	else if (r > 255)
		r = 255;
	g = p->rgb[1]*255;
	if (g < 0)
		g = 0;
	else if (g > 255)
		g = 255;
	b = p->rgb[2]*255;
	if (b < 0)
		b = 0;
	else if (b > 255)
		b = 255;
	p->color = GetPalette(r, g, b);
	D_DrawParticleTrans(p, type->blendmode);
}
void SWD_DrawParticleBeam(beamseg_t *beam, part_type_t *type)
{
	int r,g,b;	//if you have a cpu with mmx, good for you...
	beamseg_t *c;
	particle_t *p;
	particle_t *q;

//	if (!b->next)
//		return;

	c = beam->next;

	q = c->p;
//	if (!q)
//		return;

	p = beam->p;

	r = p->rgb[0]*255;
	if (r < 0)
		r = 0;
	else if (r > 255)
		r = 255;
	g = p->rgb[1]*255;
	if (g < 0)
		g = 0;
	else if (g > 255)
		g = 255;
	b = p->rgb[2]*255;
	if (b < 0)
		b = 0;
	else if (b > 255)
		b = 255;
	p->color = GetPalette(r, g, b);
	D_DrawSparkTrans(p, p->org, q->org, type->blendmode);
}
#endif

void DrawParticleTypes (void texturedparticles(particle_t *,part_type_t*), void sparklineparticles(particle_t*,part_type_t*), void sparkfanparticles(particle_t*,part_type_t*), void sparktexturedparticles(particle_t*,part_type_t*), void beamparticlest(beamseg_t*,part_type_t*), void beamparticlesut(beamseg_t*,part_type_t*), void drawdecalparticles(clippeddecal_t*,part_type_t*))
{
	RSpeedMark();

	qboolean (*tr) (vec3_t start, vec3_t end, vec3_t impact, vec3_t normal);
	void *pdraw, *bdraw;

	int i;
	vec3_t oldorg;
	vec3_t stop, normal;
	part_type_t *type;
	particle_t		*p, *kill;
	clippeddecal_t *d, *dkill;
	ramp_t *ramp;
	float grav;
	vec3_t friction;
	float dist;
	particle_t *kill_list, *kill_first;	//the kill list is to stop particles from being freed and reused whilst still in this loop
										//which is bad because beams need to find out when particles died. Reuse can do wierd things.
										//remember that they're not drawn instantly either.
	beamseg_t *b, *bkill;

	int traces=r_particle_tracelimit.value;

	lasttype = NULL;

	pframetime = host_frametime;
	if (cl.paused || r_secondaryview)
		pframetime = 0;

	VectorScale (vup, 1.5, pup);
	VectorScale (vright, 1.5, pright);
#ifdef SWQUAKE
	VectorScale (vright, xscaleshrink, r_pright);
	VectorScale (vup, yscaleshrink, r_pup);
	VectorCopy (vpn, r_ppn);
#endif

#ifdef Q2BSPS
	if (cl.worldmodel->fromgame == fg_quake2 || cl.worldmodel->fromgame == fg_quake3)
		tr = Q2TraceLineN;
	else
#endif
		tr = TraceLineN;

	kill_list = kill_first = NULL;

	for (i = 0, type = &part_type[i]; i < numparticletypes; i++, type++)
	{
		if (type->clippeddecals)
		{
/*			for ( ;; )
			{
				dkill = type->clippeddecals;
				if (dkill && dkill->die < particletime)
				{
					type->clippeddecals = dkill->next;
					free_decals =


					dkill->next = (clippeddecal_t *)kill_list;
					kill_list = (particle_t*)dkill;
					if (!kill_first)
						kill_first = kill_list;
					continue;
				}
				break;
			}
*/			for (d=type->clippeddecals ; d ; d=d->next)
			{
	/*			for ( ;; )
				{
					dkill = d->next;
					if (dkill && dkill->die < particletime)
					{
						d->next = dkill->next;
						dkill->next = (clippeddecal_t *)kill_list;
						kill_list = (particle_t*)dkill;
						if (!kill_first)
							kill_first = kill_list;
						continue;
					}
					break;
				}*/



				switch (type->rampmode)
				{
				case RAMP_ABSOLUTE:
					ramp = type->ramp + (int)(type->rampindexes * (type->die - (d->die - particletime)) / type->die);
					VectorCopy(ramp->rgb, d->rgb);
					d->alpha = ramp->alpha;
					break;
				case RAMP_DELTA:	//particle ramps
					ramp = type->ramp + (int)(type->rampindexes * (type->die - (d->die - particletime)) / type->die);
					VectorMA(d->rgb, pframetime, ramp->rgb, d->rgb);
					d->alpha -= pframetime*ramp->alpha;
					break;
				case RAMP_NONE:	//particle changes acording to it's preset properties.
					if (particletime < (d->die-type->die+type->rgbchangetime))
					{
						d->rgb[0] += pframetime*type->rgbchange[0];
						d->rgb[1] += pframetime*type->rgbchange[1];
						d->rgb[2] += pframetime*type->rgbchange[2];
					}
					d->alpha -= pframetime*(type->alpha/type->die)*type->alphachange;
				}

				drawdecalparticles(d, type);
			}
		}

		if (!type->particles)
			continue;

		bdraw = NULL;
		pdraw = NULL;

		// set drawing methods by type and cvars and hope branch
		// prediction takes care of the rest
		switch(type->type)
		{
		case PT_NORMAL:
			pdraw = texturedparticles;
			break;
		case PT_BEAM:
			if (r_part_beams.value)
			{
				if (r_part_beams_textured.value && *type->texname)
				{
					if (r_part_beams_textured.value > 0)
						bdraw = beamparticlest;
				}
				else if (r_part_beams.value > 0)
					bdraw = beamparticlesut;
			}
			break;
		case PT_TEXTUREDSPARK:
			if (r_part_sparks.value)
			{
				if (r_part_sparks_textured.value)
				{
					if (r_part_sparks_textured.value > 0)
						pdraw = sparktexturedparticles;
				}
				else
					pdraw = sparklineparticles;
			}
			break;
		case PT_SPARKFAN:
			if (r_part_sparks.value)
			{
				if (r_part_sparks_trifan.value)
				{
					if (r_part_sparks_trifan.value > 0)
						pdraw = sparkfanparticles;
				}
				else
					pdraw = sparklineparticles;
			}
			break;
		case PT_SPARK:
			if (r_part_sparks.value)
			{
				if (r_part_sparks.value > 0)
					pdraw = sparklineparticles;
			}
			break;
		}

		if (!type->die)
		{
			while ((p=type->particles))
			{
				if (pdraw)
					RQ_AddDistReorder(pdraw, p, type, p->org);

				// make sure emitter runs at least once
				if (type->emit >= 0 && type->emitstart <= 0)
					P_RunParticleEffectType(p->org, p->vel, 1, type->emit);

				// make sure stain effect runs
				if (type->stains && r_bloodstains.value)
				{
					if (traces-->0&&tr(oldorg, p->org, stop, normal))
					{
						R_AddStain(stop,	(p->rgb[1]*-10+p->rgb[2]*-10),
											(p->rgb[0]*-10+p->rgb[2]*-10),
											(p->rgb[0]*-10+p->rgb[1]*-10),
											30*p->alpha*type->stains);
					}
				}

				type->particles = p->next;
//				p->next = free_particles;
//				free_particles = p;
				p->next = kill_list;
				kill_list = p;
				if (!kill_first) // branch here is probably faster than list traversal later
					kill_first = p;
			}

			if (type->beams)
			{
				b = type->beams;
			}

			while ((b=type->beams) && (b->flags & BS_DEAD))
			{
				type->beams = b->next;
				b->next = free_beams;
				free_beams = b;
			}

			while (b)
			{
				if (!(b->flags & BS_NODRAW))
				{
					// no BS_NODRAW implies b->next != NULL
					// BS_NODRAW should imply b->next == NULL or b->next->flags & BS_DEAD
					VectorCopy(b->next->p->org, stop);
					VectorCopy(b->p->org, oldorg);
					VectorSubtract(stop, oldorg, b->next->dir);
					VectorNormalize(b->next->dir);
					if (bdraw)
					{
						VectorAdd(stop, oldorg, stop);
						VectorScale(stop, 0.5, stop);

						RQ_AddDistReorder(bdraw, b, type, stop);
					}
				}

				// clean up dead entries ahead of current
				for ( ;; )
				{
					bkill = b->next;
					if (bkill && (bkill->flags & BS_DEAD))
					{
						b->next = bkill->next;
						bkill->next = free_beams;
						free_beams = bkill;
						continue;
					}
					break;
				}

				b->flags |= BS_DEAD;
				b = b->next;
			}

			continue;
		}

		//kill off early ones.
		if (type->emittime < 0)
		{
			for ( ;; )
			{
				kill = type->particles;
				if (kill && kill->die < particletime)
				{
					P_DelinkTrailstate(&kill->trailstate);
					type->particles = kill->next;
					kill->next = kill_list;
					kill_list = kill;
					if (!kill_first)
						kill_first = kill;
					continue;
				}
				break;
			}
		}
		else
		{
			for ( ;; )
			{
				kill = type->particles;
				if (kill && kill->die < particletime)
				{
					type->particles = kill->next;
					kill->next = kill_list;
					kill_list = kill;
					if (!kill_first)
						kill_first = kill;
					continue;
				}
				break;
			}
		}

		grav = type->gravity*pframetime;
		VectorScale(type->friction, pframetime, friction);

		for (p=type->particles ; p ; p=p->next)
		{
			if (type->emittime < 0)
			{
				for ( ;; )
				{
					kill = p->next;
					if (kill && kill->die < particletime)
					{
						P_DelinkTrailstate(&kill->trailstate);
						p->next = kill->next;
						kill->next = kill_list;
						kill_list = kill;
						if (!kill_first)
							kill_first = kill;
						continue;
					}
					break;
				}
			}
			else
			{
				for ( ;; )
				{
					kill = p->next;
					if (kill && kill->die < particletime)
					{
						p->next = kill->next;
						kill->next = kill_list;
						kill_list = kill;
						if (!kill_first)
							kill_first = kill;
						continue;
					}
					break;
				}
			}

			VectorCopy(p->org, oldorg);
			if (type->flags & PT_VELOCITY)
			{
				p->org[0] += p->vel[0]*pframetime;
				p->org[1] += p->vel[1]*pframetime;
				p->org[2] += p->vel[2]*pframetime;
				if (type->flags & PT_FRICTION)
				{
					p->vel[0] -= friction[0]*p->vel[0];
					p->vel[1] -= friction[1]*p->vel[1];
					p->vel[2] -= friction[2]*p->vel[2];
				}
				p->vel[2] -= grav;
			}

			p->angle += p->rotationspeed*pframetime;

			switch (type->rampmode)
			{
			case RAMP_ABSOLUTE:
				ramp = type->ramp + (int)(type->rampindexes * (type->die - (p->die - particletime)) / type->die);
				VectorCopy(ramp->rgb, p->rgb);
				p->alpha = ramp->alpha;
				p->scale = ramp->scale;
				break;
			case RAMP_DELTA:	//particle ramps
				ramp = type->ramp + (int)(type->rampindexes * (type->die - (p->die - particletime)) / type->die);
				VectorMA(p->rgb, pframetime, ramp->rgb, p->rgb);
				p->alpha -= pframetime*ramp->alpha;
				p->scale += pframetime*ramp->scale;
				break;
			case RAMP_NONE:	//particle changes acording to it's preset properties.
				if (particletime < (p->die-type->die+type->rgbchangetime))
				{
					p->rgb[0] += pframetime*type->rgbchange[0];
					p->rgb[1] += pframetime*type->rgbchange[1];
					p->rgb[2] += pframetime*type->rgbchange[2];
				}
				p->alpha -= pframetime*(type->alpha/type->die)*type->alphachange;
				p->scale += pframetime*type->scaledelta;
			}

			if (type->emit >= 0)
			{
				if (type->emittime < 0)
					P_ParticleTrail(oldorg, p->org, type->emit, &p->trailstate);
				else if (p->nextemit < particletime)
				{
					p->nextemit = particletime + type->emittime + frandom()*type->emitrand;
					P_RunParticleEffectType(p->org, p->vel, 1, type->emit);
				}
			}

			if (type->cliptype>=0 && r_bouncysparks.value)
			{
				if (traces-->0&&tr(oldorg, p->org, stop, normal))
				{
					if (type->stains && r_bloodstains.value)
						R_AddStain(stop,	p->rgb[1]*-10+p->rgb[2]*-10,
											p->rgb[0]*-10+p->rgb[2]*-10,
											p->rgb[0]*-10+p->rgb[1]*-10,
											30*p->alpha);

					if (type->cliptype == i)
					{	//bounce
						dist = DotProduct(p->vel, normal) * (-1-(rand()/(float)0x7fff)/2);

						VectorMA(p->vel, dist, normal, p->vel);
						VectorCopy(stop, p->org);
						p->vel[0] *= type->clipbounce;
						p->vel[1] *= type->clipbounce;
						p->vel[2] *= type->clipbounce;

						if (!*type->texname && Length(p->vel)<1000*pframetime && type->type == PT_NORMAL)
							p->die = -1;
					}
					else
					{
						p->die = -1;
						VectorNormalize(p->vel);
						P_RunParticleEffectType(stop, p->vel, type->clipcount/part_type[type->cliptype].count, type->cliptype);
					}

					continue;
				}
			}
			else if (type->stains && r_bloodstains.value)
			{
				if (traces-->0&&tr(oldorg, p->org, stop, normal))
				{
					R_AddStain(stop,	(p->rgb[1]*-10+p->rgb[2]*-10),
										(p->rgb[0]*-10+p->rgb[2]*-10),
										(p->rgb[0]*-10+p->rgb[1]*-10),
										30*p->alpha*type->stains);
					p->die = -1;
					continue;
				}
			}

			if (pdraw)
				RQ_AddDistReorder((void*)pdraw, p, type, p->org);
		}

		// beams are dealt with here

		// kill early entries
		for ( ;; )
		{
			bkill = type->beams;
			if (bkill && (bkill->flags & BS_DEAD || bkill->p->die < particletime) && !(bkill->flags & BS_LASTSEG))
			{
				type->beams = bkill->next;
				bkill->next = free_beams;
				free_beams = bkill;
				continue;
			}
			break;
		}


		b = type->beams;
		if (!b)
			continue;

		for ( ;; )
		{
			if (b->next)
			{
				// mark dead entries
				if (b->flags & (BS_LASTSEG|BS_DEAD|BS_NODRAW))
				{
					// kill some more dead entries
					for ( ;; )
					{
						bkill = b->next;
						if (bkill && (bkill->flags & BS_DEAD) && !(bkill->flags & BS_LASTSEG))
						{
							b->next = bkill->next;
							bkill->next = free_beams;
							free_beams = bkill;
							continue;
						}
						break;
					}

					if (!bkill) // have to check so we don't hit NULL->next
						continue;
				}
				else
				{
					if (!(b->next->flags & BS_DEAD))
					{
						VectorCopy(b->next->p->org, stop);
						VectorCopy(b->p->org, oldorg);
						VectorSubtract(stop, oldorg, b->next->dir);
						VectorNormalize(b->next->dir);
						if (bdraw)
						{
							VectorAdd(stop, oldorg, stop);
							VectorScale(stop, 0.5, stop);

							RQ_AddDistReorder(bdraw, b, type, stop);
						}
					}

//					if (b->p->die < particletime)
//						b->flags |= BS_DEAD;
				}
			}
			else
			{
				if (b->p->die < particletime) // end of the list check
					b->flags |= BS_DEAD;

				break;
			}

			if (b->p->die < particletime)
				b->flags |= BS_DEAD;

			b = b->next;
		}
	}

	RSpeedEnd(RSPEED_PARTICLES);

	// lazy delete for particles is done here
	if (kill_list)
	{
		kill_first->next = free_particles;
		free_particles = kill_list;
	}

	particletime += pframetime;
}

#ifdef RGLQUAKE
void P_FlushRenderer(void)
{
	qglDepthMask(0);	//primarily to stop close particles from obscuring each other
	qglDisable(GL_ALPHA_TEST);
	qglEnable (GL_BLEND);
	GL_TexEnv(GL_MODULATE);
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	lasttype = NULL;
}
#endif

/*
===============
R_DrawParticles
===============
*/
void P_DrawParticles (void)
{
	RSpeedMark();

	P_AddRainParticles();
#if defined(RGLQUAKE)
	if (qrenderer == QR_OPENGL)
	{
		extern cvar_t r_drawflat;

		P_FlushRenderer();

		if (qglPolygonOffset)
			qglPolygonOffset(-1, 0);
		qglEnable(GL_POLYGON_OFFSET_FILL);
		qglEnable(GL_BLEND);
		qglDisable(GL_ALPHA_TEST);
		qglBegin(GL_QUADS);
		if (r_drawflat.value == 2)
			DrawParticleTypes(GL_DrawSketchParticle, GL_DrawSketchSparkParticle, GL_DrawSketchSparkParticle, GL_DrawSketchSparkParticle, GL_DrawParticleBeam_Textured, GL_DrawParticleBeam_Untextured, GL_DrawClippedDecal);
		else
			DrawParticleTypes(GL_DrawTexturedParticle, GL_DrawLineSparkParticle, GL_DrawTrifanParticle, GL_DrawTexturedSparkParticle, GL_DrawParticleBeam_Textured, GL_DrawParticleBeam_Untextured, GL_DrawClippedDecal);
		qglEnd();
		qglDisable(GL_POLYGON_OFFSET_FILL);

		RSpeedRemark();
		qglBegin(GL_QUADS);
		RQ_RenderDistAndClear();
		qglEnd();
		RSpeedEnd(RSPEED_PARTICLESDRAW);

		qglEnable(GL_TEXTURE_2D);

		GL_TexEnv(GL_MODULATE);
		qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		qglDepthMask(1);
		return;
	}
#endif
#ifdef SWQUAKE
	if (qrenderer == QR_SOFTWARE)
	{
		lasttype = NULL;
		DrawParticleTypes(SWD_DrawParticleBlob, SWD_DrawParticleSpark, SWD_DrawParticleSpark, SWD_DrawParticleSpark, SWD_DrawParticleBeam, SWD_DrawParticleBeam, NULL);

		RSpeedRemark();
		D_StartParticles();
		RQ_RenderDistAndClear();
		D_EndParticles();
		RSpeedEnd(RSPEED_PARTICLESDRAW);
		return;
	}
#endif
}


