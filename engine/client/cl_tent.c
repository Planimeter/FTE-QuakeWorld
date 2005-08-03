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
// cl_tent.c -- client side temporary entities

#include "quakedef.h"
#include "particles.h"
entity_state_t *CL_FindPacketEntity(int num);

#define R_AddDecals(a)	//disabled for now

#ifdef Q2CLIENT
typedef enum
{
	Q2TE_GUNSHOT,	//0
	Q2TE_BLOOD,
	Q2TE_BLASTER,
	Q2TE_RAILTRAIL,
	Q2TE_SHOTGUN,
	Q2TE_EXPLOSION1,
	Q2TE_EXPLOSION2,
	Q2TE_ROCKET_EXPLOSION,
	Q2TE_GRENADE_EXPLOSION,
	Q2TE_SPARKS,
	Q2TE_SPLASH,	//10
	Q2TE_BUBBLETRAIL,
	Q2TE_SCREEN_SPARKS,
	Q2TE_SHIELD_SPARKS,
	Q2TE_BULLET_SPARKS,
	Q2TE_LASER_SPARKS,
	Q2TE_PARASITE_ATTACK,
	Q2TE_ROCKET_EXPLOSION_WATER,
	Q2TE_GRENADE_EXPLOSION_WATER,
	Q2TE_MEDIC_CABLE_ATTACK,
	Q2TE_BFG_EXPLOSION,	//20
	Q2TE_BFG_BIGEXPLOSION,
	Q2TE_BOSSTPORT,			// used as '22' in a map, so DON'T RENUMBER!!!
	Q2TE_BFG_LASER,
	Q2TE_GRAPPLE_CABLE,
	Q2TE_WELDING_SPARKS,
	Q2TE_GREENBLOOD,
	Q2TE_BLUEHYPERBLASTER,
	Q2TE_PLASMA_EXPLOSION,
	Q2TE_TUNNEL_SPARKS,
//ROGUE
	Q2TE_BLASTER2,	//30
	Q2TE_RAILTRAIL2,
	Q2TE_FLAME,
	Q2TE_LIGHTNING,
	Q2TE_DEBUGTRAIL,
	Q2TE_PLAIN_EXPLOSION,
	Q2TE_FLASHLIGHT,
	Q2TE_FORCEWALL,
	Q2TE_HEATBEAM,
	Q2TE_MONSTER_HEATBEAM,
	Q2TE_STEAM,		//40
	Q2TE_BUBBLETRAIL2,
	Q2TE_MOREBLOOD,
	Q2TE_HEATBEAM_SPARKS,
	Q2TE_HEATBEAM_STEAM,
	Q2TE_CHAINFIST_SMOKE,
	Q2TE_ELECTRIC_SPARKS,
	Q2TE_TRACKER_EXPLOSION,
	Q2TE_TELEPORT_EFFECT,
	Q2TE_DBALL_GOAL,
	Q2TE_WIDOWBEAMOUT,	//50
	Q2TE_NUKEBLAST,
	Q2TE_WIDOWSPLASH,
	Q2TE_EXPLOSION1_BIG,
	Q2TE_EXPLOSION1_NP,
	Q2TE_FLECHETTE
//ROGUE
} temp_event_t;

#define Q2SPLASH_UNKNOWN		0
#define Q2SPLASH_SPARKS		1
#define Q2SPLASH_BLUE_WATER	2
#define Q2SPLASH_BROWN_WATER	3
#define Q2SPLASH_SLIME		4
#define	Q2SPLASH_LAVA			5
#define Q2SPLASH_BLOOD		6
#endif



	// hexen 2
#define TE_STREAM_CHAIN			25
#define TE_STREAM_SUNSTAFF1		26
#define TE_STREAM_SUNSTAFF2		27
#define TE_STREAM_LIGHTNING		28
#define TE_STREAM_COLORBEAM		29
#define TE_STREAM_ICECHUNKS		30
#define TE_STREAM_GAZE			31
#define TE_STREAM_FAMINE		32


#define	MAX_BEAMS	64
typedef struct
{
	int		entity;
	short	tag;
	qbyte	flags;
	qbyte	type;
	qbyte	skin;
	struct model_s	*model;
	float	endtime;
	float	alpha;
	vec3_t	start, end;
	int		particleeffect;
	trailstate_t *trailstate;
} beam_t;

beam_t		cl_beams[MAX_BEAMS];

#define	MAX_EXPLOSIONS	32
typedef struct
{
	vec3_t	origin;
	vec3_t	oldorigin;

	int firstframe;
	int numframes;

	int		type;
		vec3_t	angles;
		int		flags;
		float	alpha;
		float light;
		float lightcolor[3];
	float	start;
	float	framerate;
	model_t	*model;
	int skinnum;
} explosion_t;

explosion_t	cl_explosions[MAX_EXPLOSIONS];

#define MAX_SEEFS 32
typedef struct {
	int type;
	int entnum;

	vec3_t efsize;
	qbyte colour;
	int offset;

	float die; 
} seef_t;
seef_t cl_seef[MAX_SEEFS];


sfx_t			*cl_sfx_wizhit;
sfx_t			*cl_sfx_knighthit;
sfx_t			*cl_sfx_tink1;
sfx_t			*cl_sfx_ric1;
sfx_t			*cl_sfx_ric2;
sfx_t			*cl_sfx_ric3;
sfx_t			*cl_sfx_r_exp3;

cvar_t	cl_expsprite = {"cl_expsprite", "0"};
cvar_t  r_explosionlight = {"r_explosionlight", "1"};
cvar_t	cl_truelightning = {"cl_truelightning", "0",	NULL, CVAR_SEMICHEAT};

typedef struct {
	sfx_t **sfx;
	char *efname;
} tentsfx_t;
tentsfx_t tentsfx[] =
{
	{&cl_sfx_wizhit, "wizard/hit.wav"},
	{&cl_sfx_knighthit, "hknight/hit.wav"},
	{&cl_sfx_tink1, "weapons/tink1.wav"},
	{&cl_sfx_ric1, "weapons/ric1.wav"},
	{&cl_sfx_ric2, "weapons/ric2.wav"},
	{&cl_sfx_ric3, "weapons/ric3.wav"},
	{&cl_sfx_r_exp3, "weapons/r_exp3.wav"}
};
/*
=================
CL_ParseTEnts
=================
*/
void CL_InitTEnts (void)
{
	int i;
	for (i = 0; i < sizeof(tentsfx)/sizeof(tentsfx[0]); i++)
	{
		if (COM_FCheckExists(va("sound/%s", tentsfx[i].efname)))
			*tentsfx[i].sfx = S_PrecacheSound (tentsfx[i].efname);
		else
			*tentsfx[i].sfx = NULL;
	}

	Cvar_Register (&cl_expsprite, "Temporary entity control");
	Cvar_Register (&cl_truelightning, "Temporary entity control");
	Cvar_Register (&r_explosionlight, "Temporary entity control");
}

#ifdef Q2CLIENT
enum {
	q2cl_mod_explode,
	q2cl_mod_smoke,
	q2cl_mod_flash,
	q2cl_mod_parasite_segment,
	q2cl_mod_grapple_cable,
	q2cl_mod_parasite_tip,
	q2cl_mod_explo4,
	q2cl_mod_bfg_explo,
	q2cl_mod_powerscreen,
	q2cl_mod_max
};
typedef struct {
	char *modelname;

} tentmodels_t;
tentmodels_t q2tentmodels[q2cl_mod_max] = {
	{"models/objects/explode/tris.md2"},
	{"models/objects/smoke/tris.md2"},
	{"models/objects/flash/tris.md2"},
	{"models/monsters/parasite/segment/tris.md2"},
	{"models/ctf/segment/tris.md2"},
	{"models/monsters/parasite/tip/tris.md2"},
	{"models/objects/r_explode/tris.md2"},
	{"sprites/s_bfg2.sp2"},
	{"models/items/armor/effect/tris.md2"}
};

int CLQ2_RegisterTEntModels (void)
{
//	int i;
//	for (i = 0; i < q2cl_mod_max; i++)
//		if (!CL_CheckOrDownloadFile(q2tentmodels[i].modelname, false))
//			return false;

	return true;
}	
#endif
/*
=================
CL_ClearTEnts
=================
*/
void CL_ClearTEnts (void)
{
	memset (&cl_beams, 0, sizeof(cl_beams));
	memset (&cl_explosions, 0, sizeof(cl_explosions));
	memset (&cl_seef, 0, sizeof(cl_seef));
}

/*
=================
CL_AllocExplosion
=================
*/
explosion_t *CL_AllocExplosion (void)
{
	int		i;
	float	time;
	int		index;
	
	for (i=0 ; i<MAX_EXPLOSIONS ; i++)
		if (!cl_explosions[i].model)
		{
			cl_explosions[i].firstframe = -1;
			cl_explosions[i].framerate = 10;
			return &cl_explosions[i];
		}
// find the oldest explosion
	time = cl.time;
	index = 0;

	for (i=0 ; i<MAX_EXPLOSIONS ; i++)
		if (cl_explosions[i].start < time)
		{
			time = cl_explosions[i].start;
			index = i;
		}
	cl_explosions[index].firstframe = -1;
	cl_explosions[index].framerate = 10;
	return &cl_explosions[index];
}

/*
=================
CL_ParseBeam
=================
*/
beam_t	*CL_NewBeam (int entity, int tag)
{
	beam_t	*b;
	int i;

// override any beam with the same entity (unless they used world)
	if (entity)
	{
		for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
			if (b->entity == entity && b->tag == tag)
			{
				return b;
			}
	}

// find a free beam
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
	{
		if (!b->model)
		{
			return b;
		}
	}
	return NULL;
}
#define STREAM_ATTACHED			16
#define STREAM_TRANSLUCENT		32
void CL_AddBeam (int tent, int ent, vec3_t start, vec3_t end)	//fixme: use TE_ numbers instead of 0 - 5
{
	beam_t	*b;

	model_t *m;
	int btype, etype;

	switch(tent)
	{
	case 0:
		if (ent < 0 && ent >= -MAX_CLIENTS)	//a zquake concept. ent between -1 and -maxplayers is to be taken to be a railtrail from a particular player instead of a beam.
		{
			CLQ2_RailTrail(start, end);
			return;
		}
	default:
		m = Mod_ForName("progs/bolt.mdl", false);
		btype = rt_lightning1;
		etype = pt_lightning1_end;
		break;
	case 1:
		if (ent < 0 && ent >= -MAX_CLIENTS)	//based on the railgun concept - this adds a rogue style TE_BEAM effect.
		{
	case 5:
			m = Mod_ForName("progs/beam.mdl", false);	//remember to precache!
			btype = P_FindParticleType("te_beam");
			etype = P_FindParticleType("te_beam_end");
		}
		else
		{
			m = Mod_ForName("progs/bolt2.mdl", false);
			btype = rt_lightning2;
			etype = pt_lightning2_end;
		}
		break;
	case 2:
		m = Mod_ForName("progs/bolt3.mdl", false);
		btype = rt_lightning3;
		etype = pt_lightning3_end;
		break;
#ifdef Q2CLIENT
	case 3:
		m = Mod_ForName(q2tentmodels[q2cl_mod_parasite_segment].modelname, false);
		btype = P_FindParticleType("te_parasite_attack");
		etype = P_FindParticleType("te_parasite_attack_end");
		break;
	case 4:
		m = Mod_ForName(q2tentmodels[q2cl_mod_grapple_cable].modelname, false);
		btype = P_FindParticleType("te_grapple_cable");
		etype = P_FindParticleType("te_grapple_cable_end");
		break;
#endif
	}

	if (cls.state == ca_active && etype >= 0)
	{
		vec3_t impact, normal;
		vec3_t extra;
		VectorSubtract(end, start, normal);
		VectorNormalize(normal);
		VectorMA(end, 4, normal, extra);	//extend the end-point by four
		if (TraceLineN(start, extra, impact, normal))
		{
			P_RunParticleEffectType(impact, normal, 1, etype); 
			R_AddDecals(end);
			R_AddStain(end, -10, -10, -10, 20);
		}
	}

	b = CL_NewBeam(ent, -1);
	if (!b)
	{
		Con_Printf ("beam list overflow!\n");	
		return;
	}

	b->entity = ent;
	b->model = m;
	b->tag = -1;
	b->flags |= /*STREAM_ATTACHED|*/1;
	b->endtime = cl.time + 0.2;
	b->alpha = 1;
	b->particleeffect = btype;
	VectorCopy (start, b->start);
	VectorCopy (end, b->end);
}
void CL_ParseBeam (int tent)
{
	int		ent;
	vec3_t	start, end;

	ent = MSG_ReadShort ();
	
	start[0] = MSG_ReadCoord ();
	start[1] = MSG_ReadCoord ();
	start[2] = MSG_ReadCoord ();
	
	end[0] = MSG_ReadCoord ();
	end[1] = MSG_ReadCoord ();
	end[2] = MSG_ReadCoord ();

	CL_AddBeam(tent, ent, start, end);
}
void CL_ParseStream (int type)
{
	int		ent;
	vec3_t	start, end;
	beam_t	*b, *b2;
	int flags;
	int tag;
	float duration;
	int skin;
	
	ent = MSG_ReadShort();
	flags = MSG_ReadByte();
	tag = flags&15;
	flags-=tag;
	duration = (float)MSG_ReadByte()*0.05;
	skin = 0;
	if(type == TE_STREAM_COLORBEAM)
	{
		skin = MSG_ReadByte();
	}
	start[0] = MSG_ReadCoord();
	start[1] = MSG_ReadCoord();
	start[2] = MSG_ReadCoord();
	end[0] = MSG_ReadCoord();
	end[1] = MSG_ReadCoord();
	end[2] = MSG_ReadCoord();

	b = CL_NewBeam(ent, tag);
	if (!b)
	{
		Con_Printf ("beam list overflow!\n");	
		return;
	}

	b->entity = ent;
	b->tag = tag;
	b->flags = flags;
	b->model = NULL;
	b->endtime = cl.time + duration;
	b->alpha = 1;
	VectorCopy (start, b->start);
	VectorCopy (end, b->end);

	switch(type)
	{
	case TE_STREAM_ICECHUNKS:
		b->model = 	Mod_ForName("models/stice.mdl", true);
		b->flags |= 2;
		b->particleeffect = P_AllocateParticleType("te_stream_icechunks");
		R_AddStain(end, -10, -10, 0, 20);
		break;
	case TE_STREAM_SUNSTAFF1:
		b->model = Mod_ForName("models/stsunsf1.mdl", true);
		b->particleeffect = P_AllocateParticleType("te_stream_sunstaff1");
		if (b->particleeffect < 0)
		{
			b2 = CL_NewBeam(ent, tag+128);
			if (b2)
			{
				memcpy(b2, b, sizeof(*b2));
				b2->model = Mod_ForName("models/stsunsf2.mdl", true);
				b2->alpha = 0.5;
			}
		}
		break;
	case TE_STREAM_SUNSTAFF2:
		b->model = 	Mod_ForName("models/stsunsf1.mdl", true);
		b->particleeffect = P_AllocateParticleType("te_stream_sunstaff2");
		R_AddStain(end, -10, -10, -10, 20);
		break;
	}
}


void CL_ParseSEEF(int type)
{
	int i;
	short entnum;
	qboolean remove = false;
	seef_t *seef;

	entnum = MSG_ReadShort();
	if (entnum & 0x8000)
	{
		remove = true;
		entnum &= ~0x8000;
	}

	for (i = 0, seef = cl_seef; i < MAX_SEEFS; i++, seef++)	//try and find an old onw
	{
		if (seef->entnum == entnum && seef->type == type)
			break;
	}
	if (remove)
	{
		if (seef)
			seef->die = 0;	//mark it as free
		return;
	}
	if (i == MAX_SEEFS)
	{
		for (i = 0, seef = cl_seef; i < MAX_SEEFS; i++, seef++)	//try and find an old onw
		{
			if (seef->die < cl.time)
				break;
		}
		if (i == MAX_SEEFS)
			seef = &cl_seef[rand()%MAX_SEEFS];	//use a random one (more likly to not be dead)
	}

	seef->type = type;
	seef->die = cl.time + 20;	//as removed ents won't be spotted.
	seef->entnum = entnum;

	switch(type)
	{
	case TE_SEEF_BRIGHTFIELD:
		seef->efsize[0] = MSG_ReadCoord ();
		seef->efsize[1] = MSG_ReadCoord ();
		seef->efsize[2] = MSG_ReadCoord ();
		seef->offset = MSG_ReadChar();
		seef->colour = MSG_ReadByte();
		break;
	case TE_SEEF_DARKFIELD:
		seef->colour = MSG_ReadByte();
		break;
	case TE_SEEF_DARKLIGHT:
	case TE_SEEF_LIGHT:
		seef->efsize[0] = MSG_ReadCoord ();
		seef->efsize[1] = MSG_ReadCoord ();
		break;
	default:
		Host_EndGame("Bad SEEF type\n");
	}
}

/*
=================
CL_ParseTEnt
=================
*/

#ifdef NQPROT
void CL_ParseTEnt (qboolean nqprot)
#else
void CL_ParseTEnt (void)
#endif
{
#ifndef NQPROT
#define nqprot false	//it's easier
#endif
	int		type;
	vec3_t	pos, pos2;
	dlight_t	*dl;
	int		rnd;
//	explosion_t	*ex;
	int		cnt, colour;

	type = MSG_ReadByte ();
	switch (type)
	{
	case TE_WIZSPIKE:			// spike hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_AddStain(pos, -10, 0, -10, 20);

		if (P_RunParticleEffectType(pos, NULL, 1, pt_wizspike))
			P_RunParticleEffect (pos, vec3_origin, 20, 30);

		S_StartSound (-2, 0, cl_sfx_wizhit, pos, 1, 1);
		break;
		
	case TE_KNIGHTSPIKE:			// spike hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_AddStain(pos, -10, -10, -10, 20);

		if (P_RunParticleEffectType(pos, NULL, 1, pt_knightspike))
			P_RunParticleEffect (pos, vec3_origin, 226, 20);

		S_StartSound (-2, 0, cl_sfx_knighthit, pos, 1, 1);
		break;
		
	case DPTE_SPIKEQUAD:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_AddStain(pos, -10, -10, -10, 20);
		R_AddDecals(pos);

		if (P_RunParticleEffectTypeString(pos, NULL, 1, "te_spikequad"))
			if (P_RunParticleEffectType(pos, NULL, 1, pt_spike))
				if (P_RunParticleEffectType(pos, NULL, 10, pt_gunshot))
					P_RunParticleEffect (pos, vec3_origin, 0, 10);

		if ( rand() % 5 )
			S_StartSound (-2, 0, cl_sfx_tink1, pos, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound (-2, 0, cl_sfx_ric1, pos, 1, 1);
			else if (rnd == 2)
				S_StartSound (-2, 0, cl_sfx_ric2, pos, 1, 1);
			else
				S_StartSound (-2, 0, cl_sfx_ric3, pos, 1, 1);
		}
		break;
	case TE_SPIKE:			// spike hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_AddStain(pos, -10, -10, -10, 20);
		R_AddDecals(pos);

		if (P_RunParticleEffectType(pos, NULL, 1, pt_spike))
			if (P_RunParticleEffectType(pos, NULL, 10, pt_gunshot))
				P_RunParticleEffect (pos, vec3_origin, 0, 10);

		if ( rand() % 5 )
			S_StartSound (-2, 0, cl_sfx_tink1, pos, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound (-2, 0, cl_sfx_ric1, pos, 1, 1);
			else if (rnd == 2)
				S_StartSound (-2, 0, cl_sfx_ric2, pos, 1, 1);
			else
				S_StartSound (-2, 0, cl_sfx_ric3, pos, 1, 1);
		}
		break;
	case DPTE_SUPERSPIKEQUAD:			// super spike hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_AddStain(pos, -10, -10, -10, 20);
		R_AddDecals(pos);

		if (P_RunParticleEffectTypeString(pos, NULL, 1, "te_superspikequad"))
			if (P_RunParticleEffectType(pos, NULL, 1, pt_superspike))
				if (P_RunParticleEffectType(pos, NULL, 2, pt_spike))
					if (P_RunParticleEffectType(pos, NULL, 20, pt_gunshot))
						P_RunParticleEffect (pos, vec3_origin, 0, 20);

		if ( rand() % 5 )
			S_StartSound (-2, 0, cl_sfx_tink1, pos, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound (-2, 0, cl_sfx_ric1, pos, 1, 1);
			else if (rnd == 2)
				S_StartSound (-2, 0, cl_sfx_ric2, pos, 1, 1);
			else
				S_StartSound (-2, 0, cl_sfx_ric3, pos, 1, 1);
		}
		break;
	case TE_SUPERSPIKE:			// super spike hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_AddStain(pos, -10, -10, -10, 20);
		R_AddDecals(pos);

		if (P_RunParticleEffectType(pos, NULL, 1, pt_superspike))
			if (P_RunParticleEffectType(pos, NULL, 2, pt_spike))
				if (P_RunParticleEffectType(pos, NULL, 20, pt_gunshot))
					P_RunParticleEffect (pos, vec3_origin, 0, 20);

		if ( rand() % 5 )
			S_StartSound (-2, 0, cl_sfx_tink1, pos, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound (-2, 0, cl_sfx_ric1, pos, 1, 1);
			else if (rnd == 2)
				S_StartSound (-2, 0, cl_sfx_ric2, pos, 1, 1);
			else
				S_StartSound (-2, 0, cl_sfx_ric3, pos, 1, 1);
		}
		break;
	
#ifdef PEXT_TE_BULLET
	case TE_BULLET:
		if (!(cls.fteprotocolextensions & PEXT_TE_BULLET))
			Sys_Error("Thought PEXT_TE_BULLET was disabled");
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_AddStain(pos, -10, -10, -10, 20);
		R_AddDecals(pos);

		if (P_RunParticleEffectType(pos, NULL, 1, pt_bullet))
			if (P_RunParticleEffectType(pos, NULL, 10, pt_gunshot))
				P_RunParticleEffect (pos, vec3_origin, 0, 10);

		if ( rand() % 5 )
			S_StartSound (-2, 0, cl_sfx_tink1, pos, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound (-2, 0, cl_sfx_ric1, pos, 1, 1);
			else if (rnd == 2)
				S_StartSound (-2, 0, cl_sfx_ric2, pos, 1, 1);
			else
				S_StartSound (-2, 0, cl_sfx_ric3, pos, 1, 1);
		}
		break;
	case TE_SUPERBULLET:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_AddStain(pos, -10, -10, -10, 20);
		R_AddDecals(pos);

		if (P_RunParticleEffectType(pos, NULL, 1, pt_superbullet))
			if (P_RunParticleEffectType(pos, NULL, 2, pt_bullet))
				if (P_RunParticleEffectType(pos, NULL, 20, pt_gunshot))
					P_RunParticleEffect (pos, vec3_origin, 0, 20);

		if ( rand() % 5 )
			S_StartSound (-2, 0, cl_sfx_tink1, pos, 1, 1);
		else
		{
			rnd = rand() & 3;
			if (rnd == 1)
				S_StartSound (-2, 0, cl_sfx_ric1, pos, 1, 1);
			else if (rnd == 2)
				S_StartSound (-2, 0, cl_sfx_ric2, pos, 1, 1);
			else
				S_StartSound (-2, 0, cl_sfx_ric3, pos, 1, 1);
		}		
		break;
#endif

	case DPTE_EXPLOSIONQUAD:			// rocket explosion
	// particles
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		if (P_RunParticleEffectTypeString(pos, NULL, 1, "te_explosionquad"))
			P_ParticleExplosion (pos);
		
	// light
		if (r_explosionlight.value) {
			dl = CL_AllocDlight (0);
			VectorCopy (pos, dl->origin);
			dl->radius = 150 + bound(0, r_explosionlight.value, 1)*200;
			dl->die = cl.time + 1;
			dl->decay = 300;
		
			dl->color[0] = 0.2;
			dl->color[1] = 0.155;
			dl->color[2] = 0.05;
			dl->channelfade[0] = 0.196;
			dl->channelfade[1] = 0.23;
			dl->channelfade[2] = 0.12;
		}


	// sound
		S_StartSound (-2, 0, cl_sfx_r_exp3, pos, 1, 1);
	
	// sprite		
		if (cl_expsprite.value) // temp hopefully
		{
			explosion_t *ex = CL_AllocExplosion ();
			VectorCopy (pos, ex->origin);
			ex->start = cl.time;
			ex->model = Mod_ForName ("progs/s_explod.spr", true);
		}
		break;
	case TE_EXPLOSION:			// rocket explosion
	// particles
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		P_ParticleExplosion (pos);
		
	// light
		dl = CL_AllocDlight (0);
		VectorCopy (pos, dl->origin);
		dl->radius = 350;
		dl->die = cl.time + 1;
		dl->decay = 300;
		
		dl->color[0] = 0.2;
		dl->color[1] = 0.155;
		dl->color[2] = 0.05;
		dl->channelfade[0] = 0.196;
		dl->channelfade[1] = 0.23;
		dl->channelfade[2] = 0.12;



	// sound
		S_StartSound (-2, 0, cl_sfx_r_exp3, pos, 1, 1);
	
	// sprite		
		if (cl_expsprite.value && !nqprot) // temp hopefully
		{
			explosion_t *ex = CL_AllocExplosion ();
			VectorCopy (pos, ex->origin);
			ex->start = cl.time;
			ex->model = Mod_ForName ("progs/s_explod.spr", true);
		}
		break;

	case DPTE_EXPLOSIONRGB:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		P_ParticleExplosion (pos);
		
	// light
		if (r_explosionlight.value) {
			dl = CL_AllocDlight (0);
			VectorCopy (pos, dl->origin);
			dl->radius = 150 + bound(0, r_explosionlight.value, 1)*200;
			dl->die = cl.time + 0.5;
			dl->decay = 300;
		
			dl->color[0] = 0.4f*MSG_ReadByte()/255.0f;
			dl->color[1] = 0.4f*MSG_ReadByte()/255.0f;
			dl->color[2] = 0.4f*MSG_ReadByte()/255.0f;
			dl->channelfade[0] = 0;
			dl->channelfade[1] = 0;
			dl->channelfade[2] = 0;
		}

		S_StartSound (-2, 0, cl_sfx_r_exp3, pos, 1, 1);
		break;

	case DPTE_TEI_BIGEXPLOSION:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		P_ParticleExplosion (pos);
		
	// light
		dl = CL_AllocDlight (0);
		VectorCopy (pos, dl->origin);
		dl->radius = 500;
		dl->die = cl.time + 1;
		dl->decay = 500;
		
		dl->color[0] = 0.4f;
		dl->color[1] = 0.3f;
		dl->color[2] = 0.15f;
		dl->channelfade[0] = 0;
		dl->channelfade[1] = 0;
		dl->channelfade[2] = 0;

		S_StartSound (-2, 0, cl_sfx_r_exp3, pos, 1, 1);
		break;
		
	case TE_TAREXPLOSION:			// tarbaby explosion
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		P_BlobExplosion (pos);

		S_StartSound (-2, 0, cl_sfx_r_exp3, pos, 1, 1);
		break;

	case TE_LIGHTNING1:				// lightning bolts
	case TE_LIGHTNING2:				// lightning bolts
		CL_ParseBeam (type - TE_LIGHTNING1);
		break;
	case TE_LIGHTNING3:				// lightning bolts
		CL_ParseBeam (2);
		break;
	
	case TE_LAVASPLASH:	
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		P_LavaSplash (pos);
		break;
	
	case TE_TELEPORT:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();
		P_RunParticleEffectType(pos, NULL, 1, pt_teleportsplash);
		break;

	case DPTE_GUNSHOTQUAD:			// bullet hitting wall
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_AddStain(pos, -10, -10, -10, 20);

		if (P_RunParticleEffectTypeString(pos, NULL, 1, "te_gunshotquad"))
			if (P_RunParticleEffectType(pos, NULL, 1, pt_gunshot))
				P_RunParticleEffect (pos, vec3_origin, 0, 20);

		break;
	case TE_GUNSHOT:			// bullet hitting wall
		if (nqprot)
			cnt = 1;
		else
			cnt = MSG_ReadByte ();
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_AddStain(pos, -10, -10, -10, 20);

		if (P_RunParticleEffectType(pos, NULL, cnt, pt_gunshot))
			P_RunParticleEffect (pos, vec3_origin, 0, 20*cnt);

		break;
		
	case TE_BLOOD:				// bullets hitting body
		cnt = MSG_ReadByte ();
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_AddStain(pos, 0, -10, -10, 40);

		if (P_RunParticleEffectType(pos, NULL, cnt, pt_blood))
			P_RunParticleEffect (pos, vec3_origin, 73, 20*cnt);

		break;

	case TE_LIGHTNINGBLOOD:		// lightning hitting body
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		R_AddStain(pos, 1, -10, -10, 20);

		if (P_RunParticleEffectType(pos, NULL, 1, pt_lightningblood))
			P_RunParticleEffect (pos, vec3_origin, 225, 50);

		break;

	case TE_RAILTRAIL:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		pos2[0] = MSG_ReadCoord ();
		pos2[1] = MSG_ReadCoord ();
		pos2[2] = MSG_ReadCoord ();
		CLQ2_RailTrail (pos, pos2);
		break;

	case TE_SEEF_DARKLIGHT:
	case TE_SEEF_LIGHT:
	case TE_SEEF_BRIGHTFIELD:
	case TE_SEEF_DARKFIELD:
		CL_ParseSEEF(type);
		break;

	case TE_STREAM_CHAIN:
	case TE_STREAM_SUNSTAFF1:
	case TE_STREAM_SUNSTAFF2:
	case TE_STREAM_LIGHTNING:
	case TE_STREAM_COLORBEAM:
	case TE_STREAM_ICECHUNKS:
	case TE_STREAM_GAZE:
	case TE_STREAM_FAMINE:
		CL_ParseStream (type);
		break;

	case DPTE_BLOOD:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		pos2[0] = MSG_ReadChar ();
		pos2[1] = MSG_ReadChar ();
		pos2[2] = MSG_ReadChar ();

		cnt = MSG_ReadByte ();

		P_RunParticleEffectType(pos, pos2, cnt, pt_blood);
		break;

	case DPTE_SPARK:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		pos2[0] = MSG_ReadChar ();
		pos2[1] = MSG_ReadChar ();
		pos2[2] = MSG_ReadChar ();

		cnt = MSG_ReadByte ();
		{
			extern int pt_spark;
			P_RunParticleEffectType(pos, pos2, cnt, pt_spark);
		}
		break;

	case DPTE_BLOODSHOWER:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		pos2[0] = MSG_ReadCoord ();
		pos2[1] = MSG_ReadCoord ();
		pos2[2] = MSG_ReadCoord ();

		cnt = MSG_ReadCoord ();	//speed

		cnt = MSG_ReadShort ();

		{
			VectorAdd(pos, pos2, pos);
			VectorScale(pos, 0.5, pos);
			P_RunParticleEffectTypeString(pos, NULL, cnt, "te_bloodshower");
		}
		break;

	case DPTE_SMALLFLASH:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		// light
		dl = CL_AllocDlight (0);
		VectorCopy (pos, dl->origin);
		dl->radius = 200;
		dl->decay = 1000;
		dl->die = cl.time + 0.2;
		dl->color[0] = 0.4;
		dl->color[1] = 0.4;
		dl->color[2] = 0.4;
		break;

	case DPTE_CUSTOMFLASH:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

			// light
		dl = CL_AllocDlight (0);
		VectorCopy (pos, dl->origin);
		dl->radius = MSG_ReadByte()*8;
		pos2[0] = (MSG_ReadByte() + 1) * (1.0 / 256.0);
		dl->die = cl.time + pos2[0];
		dl->decay = dl->radius / pos2[0];
	
		// DP's range is 0-2 for lights, FTE is 0-0.4.. 255/637.5 = 0.4
		dl->color[0] = MSG_ReadByte()*(1.0f/637.5f);
		dl->color[1] = MSG_ReadByte()*(1.0f/637.5f);
		dl->color[2] = MSG_ReadByte()*(1.0f/637.5f);
		
		break;

	case DPTE_FLAMEJET:
		// origin
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		// velocity
		pos2[0] = MSG_ReadCoord ();
		pos2[1] = MSG_ReadCoord ();
		pos2[2] = MSG_ReadCoord ();

		// count
		cnt = MSG_ReadByte ();

		if (P_RunParticleEffectTypeString(pos, pos2, cnt, "te_flamejet"))
			P_RunParticleEffect (pos, pos2, 232, cnt);
		break;

	case DPTE_PLASMABURN:
		// origin
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		// light
		dl = CL_AllocDlight (0);
		VectorCopy (pos, dl->origin);
		dl->radius = 200;
		dl->decay = 1000;
		dl->die = cl.time + 0.2;
		dl->color[0] = 0.2;
		dl->color[1] = 0.2;
		dl->color[2] = 0.2;

		// stain (Hopefully this is close to how DP does it)
		R_AddStain(pos, -10, -10, -10, 30);

		P_ParticleTrail(pos, pos2, P_FindParticleType("te_plasmaburn"), NULL);
		break;

	case DPTE_TEI_G3:	//nexuiz's nex beam
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		pos2[0] = MSG_ReadCoord ();
		pos2[1] = MSG_ReadCoord ();
		pos2[2] = MSG_ReadCoord ();

		//sigh...
		MSG_ReadCoord ();
		MSG_ReadCoord ();
		MSG_ReadCoord ();

		P_ParticleTrail(pos, pos2, P_FindParticleType("te_nexbeam"), NULL);
		break;

	case DPTE_SMOKE:
		//org
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		//dir
		pos2[0] = MSG_ReadCoord ();
		pos2[1] = MSG_ReadCoord ();
		pos2[2] = MSG_ReadCoord ();

		//count
		cnt = MSG_ReadByte ();
		{
			extern int pt_smoke;
			P_RunParticleEffectType(pos, pos2, cnt, pt_smoke);
		}
		break;

	case DPTE_TEI_PLASMAHIT:
		pos[0] = MSG_ReadCoord ();
		pos[1] = MSG_ReadCoord ();
		pos[2] = MSG_ReadCoord ();

		//dir
		pos2[0] = MSG_ReadCoord ();
		pos2[1] = MSG_ReadCoord ();
		pos2[2] = MSG_ReadCoord ();
		cnt = MSG_ReadByte ();

		{
			extern int pt_plasma;
			P_RunParticleEffectType(pos, pos2, cnt, pt_plasma);
		}
		break;

	case DPTE_PARTICLECUBE:
		{
			vec3_t dir;
			int jitter;
			int gravity;

			//min
			pos[0] = MSG_ReadCoord();
			pos[1] = MSG_ReadCoord();
			pos[2] = MSG_ReadCoord();

			//max
			pos2[0] = MSG_ReadCoord();
			pos2[1] = MSG_ReadCoord();
			pos2[2] = MSG_ReadCoord();

			//dir
			dir[0] = MSG_ReadCoord();
			dir[1] = MSG_ReadCoord();
			dir[2] = MSG_ReadCoord();

			cnt = MSG_ReadShort();	//count
			colour = MSG_ReadByte ();	//colour
			gravity = MSG_ReadByte ();	//gravity flag
			jitter = MSG_ReadCoord();	//jitter

			P_RunParticleCube(pos, pos2, dir, cnt, colour, gravity, jitter);
		}
		break;
	case DPTE_PARTICLERAIN:
		{
			vec3_t dir;

			//min
			pos[0] = MSG_ReadCoord();
			pos[1] = MSG_ReadCoord();
			pos[2] = MSG_ReadCoord();

			//max
			pos2[0] = MSG_ReadCoord();
			pos2[1] = MSG_ReadCoord();
			pos2[2] = MSG_ReadCoord();

			//dir
			dir[0] = MSG_ReadCoord();
			dir[1] = MSG_ReadCoord();
			dir[2] = MSG_ReadCoord();

			cnt = MSG_ReadShort();	//count
			colour = MSG_ReadByte ();	//colour

			P_RunParticleWeather(pos, pos2, dir, cnt, colour, "rain");
		}
		break;
	case DPTE_PARTICLESNOW:
		{
			vec3_t dir;

			//min
			pos[0] = MSG_ReadCoord();
			pos[1] = MSG_ReadCoord();
			pos[2] = MSG_ReadCoord();

			//max
			pos2[0] = MSG_ReadCoord();
			pos2[1] = MSG_ReadCoord();
			pos2[2] = MSG_ReadCoord();

			//dir
			dir[0] = MSG_ReadCoord();
			dir[1] = MSG_ReadCoord();
			dir[2] = MSG_ReadCoord();

			cnt = MSG_ReadShort();	//count
			colour = MSG_ReadByte ();	//colour

			P_RunParticleWeather(pos, pos2, dir, cnt, colour, "snow");
		}
		break;

	default:
		Host_EndGame ("CL_ParseTEnt: bad type - %i", type);
	}
}

void MSG_ReadPos (vec3_t pos);
void MSG_ReadDir (vec3_t dir);
typedef struct {
	int netstyle;
	int particleeffecttype;
	char stain[3];
	qbyte radius;
	vec3_t dlightrgb;
	float dlightradius;
	float dlighttime;
	vec3_t dlightcfade;
} clcustomtents_t;

#define CTE_CUSTOMCOUNT		1
#define CTE_CUSTOMDIRECTION	2
#define CTE_STAINS			4
#define CTE_GLOWS			8
#define CTE_CHANNELFADE		16
#define CTE_ISBEAM			128

clcustomtents_t customtenttype[255];	//network based.
void CL_ParseCustomTEnt(void)
{
	int count;
	vec3_t pos;
	vec3_t pos2;
	vec3_t dir;
	char *str;
	clcustomtents_t *t;
	int type = MSG_ReadByte();

	if (type == 255)	//255 is register
	{
		type = MSG_ReadByte();
		if (type == 255)
			Host_EndGame("Custom temp type 255 isn't valid\n");
		t = &customtenttype[type];

		t->netstyle = MSG_ReadByte();
		str = MSG_ReadString();
		t->particleeffecttype = P_AllocateParticleType(str);

		if (t->netstyle & CTE_STAINS)
		{
			t->stain[0] = MSG_ReadChar();
			t->stain[1] = MSG_ReadChar();
			t->stain[2] = MSG_ReadChar();
			t->radius = MSG_ReadByte();
		}
		else
			t->radius = 0;
		if (t->netstyle & CTE_GLOWS)
		{
			t->dlightrgb[0] = MSG_ReadByte()/255.0f;
			t->dlightrgb[1] = MSG_ReadByte()/255.0f;
			t->dlightrgb[2] = MSG_ReadByte()/255.0f;
			t->dlightradius = MSG_ReadByte();
			t->dlighttime = MSG_ReadByte()/16.0f;
			if (t->netstyle & CTE_CHANNELFADE)
			{
				t->dlightcfade[0] = MSG_ReadByte()/64.0f;
				t->dlightcfade[1] = MSG_ReadByte()/64.0f;
				t->dlightcfade[2] = MSG_ReadByte()/64.0f;
			}
		}
		else
			t->dlighttime = 0;
		return;
	}

	t = &customtenttype[type];
	if (t->particleeffecttype < 0)
		Host_EndGame("Custom Temporary entity %i was not registered\n", type);

	if (t->netstyle & CTE_ISBEAM)
	{
		MSG_ReadPos (pos);
		MSG_ReadPos (pos2);
		P_ParticleTrail(pos, pos2, t->particleeffecttype, NULL);
	}
	else
	{
		if (t->netstyle & CTE_CUSTOMCOUNT)
			count = MSG_ReadByte();
		else
			count = 1;

		MSG_ReadPos (pos);
		VectorCopy(pos, pos2);

		if (t->netstyle & CTE_CUSTOMDIRECTION)
		{
			MSG_ReadDir (dir);
			P_RunParticleEffectType(pos, dir, 1, t->particleeffecttype);
		}
		else P_RunParticleEffectType(pos, NULL, 1, t->particleeffecttype);
	}

	if (t->netstyle & CTE_STAINS)
	{	//added at pos2 - end of trail
		R_AddStain(pos2, t->stain[0], t->stain[1], t->stain[2], 40);
	}
	if (t->netstyle & CTE_GLOWS)
	{	//added at pos1 firer's end.
		dlight_t	*dl;
		dl = CL_AllocDlight (0);
		VectorCopy (pos, dl->origin);
		dl->radius = t->dlightradius*4;
		dl->die = cl.time + t->dlighttime;
		dl->decay = t->radius/t->dlighttime;
		
		dl->color[0] = t->dlightrgb[0];
		dl->color[1] = t->dlightrgb[1];
		dl->color[2] = t->dlightrgb[2];

		if (t->netstyle & CTE_CHANNELFADE)
		{
			dl->channelfade[0] = t->dlightcfade[0];
			dl->channelfade[1] = t->dlightcfade[1];
			dl->channelfade[2] = t->dlightcfade[2];
		}

		/*
		if (dl->color[0] < 0)
			dl->channelfade[0] = 0;
		else
			dl->channelfade[0] = dl->color[0]/t->dlighttime;

		if (dl->color[1] < 0)
			dl->channelfade[1] = 0;
		else
			dl->channelfade[1] = dl->color[0]/t->dlighttime;

		if (dl->color[2] < 0)
			dl->channelfade[2] = 0;
		else
			dl->channelfade[2] = dl->color[0]/t->dlighttime;
		*/
	}
}
void CL_ClearCustomTEnts(void)
{
	int i;
	for (i = 0; i < sizeof(customtenttype)/sizeof(customtenttype[0]); i++)
		customtenttype[i].particleeffecttype = -1;
}

void CLNQ_ParseParticleEffect (void)
{
	vec3_t		org, dir;
	int			i, count, msgcount, color;
	
	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	for (i=0 ; i<3 ; i++)
		dir[i] = MSG_ReadChar () * (1.0/16);
	msgcount = MSG_ReadByte ();
	color = MSG_ReadByte ();

	if (msgcount == 255)
		count = 1024;
	else
		count = msgcount;
	
	P_RunParticleEffect (org, dir, color, count);
}
void CL_ParseParticleEffect2 (void)
{
	vec3_t		org, dmin, dmax;
	int			i, msgcount, color, effect;
	
	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	for (i=0 ; i<3 ; i++)
		dmin[i] = MSG_ReadFloat ();
	for (i=0 ; i<3 ; i++)
		dmax[i] = MSG_ReadFloat ();
	color = MSG_ReadShort ();
	msgcount = MSG_ReadByte ();
	effect = MSG_ReadByte ();

	P_RunParticleEffect2 (org, dmin, dmax, color, effect, msgcount);
}
void CL_ParseParticleEffect3 (void)
{
	vec3_t		org, box;
	int			i, msgcount, color, effect;
	
	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	for (i=0 ; i<3 ; i++)
		box[i] = MSG_ReadByte ();
	color = MSG_ReadShort ();
	msgcount = MSG_ReadByte ();
	effect = MSG_ReadByte ();

	P_RunParticleEffect3 (org, box, color, effect, msgcount);
}
void CL_ParseParticleEffect4 (void)
{
	vec3_t		org;
	int			i, msgcount, color, effect;
	float		radius;
	
	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	radius = MSG_ReadByte();
	color = MSG_ReadShort ();
	msgcount = MSG_ReadByte ();
	effect = MSG_ReadByte ();

	P_RunParticleEffect4 (org, radius, color, effect, msgcount);
}

void CL_SpawnSpriteEffect(vec3_t org, model_t *model, int startframe, int framecount, int framerate)
{
	explosion_t	*ex;

	ex = CL_AllocExplosion ();
	VectorCopy (org, ex->origin);
	ex->start = cl.time;
	ex->model = model;
	ex->firstframe = startframe;
	ex->numframes = framecount;
	ex->framerate = framerate;

	ex->angles[0] = 0;
	ex->angles[1] = 0;
	ex->angles[2] = 0;
}

// [vector] org [byte] modelindex [byte] startframe [byte] framecount [byte] framerate
// [vector] org [short] modelindex [short] startframe [byte] framecount [byte] framerate
void CL_ParseEffect (qboolean effect2)
{
	vec3_t org;
	int modelindex;
	int startframe;
	int framecount;
	int framerate;

	org[0] = MSG_ReadCoord();
	org[1] = MSG_ReadCoord();
	org[2] = MSG_ReadCoord();

	if (effect2)
		modelindex = MSG_ReadShort();
	else
		modelindex = MSG_ReadByte();

	if (effect2)
		startframe = MSG_ReadShort();
	else
		startframe = MSG_ReadByte();

	framecount = MSG_ReadByte();
	framerate = MSG_ReadByte();


	CL_SpawnSpriteEffect(org, cl.model_precache[modelindex], startframe, framecount, framerate);
}

#ifdef Q2CLIENT
void CL_SmokeAndFlash(vec3_t origin)
{
	explosion_t	*ex;

	ex = CL_AllocExplosion ();
	VectorCopy (origin, ex->origin);
	VectorClear(ex->angles);
//	ex->type = ex_misc;
	ex->numframes = 4;
	ex->flags = Q2RF_TRANSLUCENT;
	ex->start = cl.time;
	ex->model = Mod_ForName (q2tentmodels[q2cl_mod_smoke].modelname, false);

	ex = CL_AllocExplosion ();
	VectorCopy (origin, ex->origin);
	VectorClear(ex->angles);
//	ex->type = ex_flash;
	ex->flags = Q2RF_FULLBRIGHT;
	ex->numframes = 2;
	ex->start = cl.time;
	ex->model = Mod_ForName (q2tentmodels[q2cl_mod_flash].modelname, false);
}

void CL_Laser (vec3_t start, vec3_t end, int colors)
{
	explosion_t	*ex = CL_AllocExplosion();
	ex->firstframe = 0;
	ex->numframes = 10;
	ex->alpha = 0.33f;
	ex->model = (void*)0xDEAFF00D;	//something not null
	ex->skinnum = (colors >> ((rand() % 4)*8)) & 0xff;
	VectorCopy (start, ex->origin);
	VectorCopy (end, ex->oldorigin);
	ex->flags = Q2RF_TRANSLUCENT | Q2RF_BEAM;
	ex->start = cl.time;
}

static qbyte splash_color[] = {0x00, 0xe0, 0xb0, 0x50, 0xd0, 0xe0, 0xe8};

#define ATTN_NONE	0
#define ATTN_NORM 1
#define ATTN_STATIC 1
void Q2S_StartSound(vec3_t origin, int entnum, int entchannel, sfx_t *sfx, float fvol, float attenuation, float timeofs)
{
	S_StartSound(entnum, entchannel, sfx, origin, fvol, attenuation);
}
void CLQ2_ParseTEnt (void)
{
	int		type;
	vec3_t	pos, pos2, dir;
	explosion_t	*ex;
	int		cnt;
	int		color;
	int		r;
//	int		ent;
//	int		magnitude;

	type = MSG_ReadByte ();

	switch (type)
	{
	case Q2TE_BLOOD:			// bullet hitting flesh
		MSG_ReadPos (pos);
		MSG_ReadDir (dir);
		P_RunParticleEffectType(pos, dir, 1, pt_blood);
		R_AddStain(pos, 0, -10, -10, 40);
		break;

	case Q2TE_GUNSHOT:			// bullet hitting wall
	case Q2TE_SPARKS:
	case Q2TE_BULLET_SPARKS:
		MSG_ReadPos (pos);
		MSG_ReadDir (dir);
		if (type == Q2TE_GUNSHOT)
			P_RunParticleEffect (pos, dir, 0, 40);
		else
			P_RunParticleEffect (pos, dir, 0xe0, 6);

		R_AddStain(pos, -10, -10, -10, 20);

		if (type != Q2TE_SPARKS)
		{
			CL_SmokeAndFlash(pos);
			
			// impact sound (nope, not the same as Q1...)
			cnt = rand()&15;
			if (cnt == 1)
				Q2S_StartSound (pos, 0, 0, S_PrecacheSound ("world/ric1.wav"), 1, ATTN_NORM, 0);
			else if (cnt == 2)
				Q2S_StartSound (pos, 0, 0, S_PrecacheSound ("world/ric2.wav"), 1, ATTN_NORM, 0);
			else if (cnt == 3)
				Q2S_StartSound (pos, 0, 0, S_PrecacheSound ("world/ric3.wav"), 1, ATTN_NORM, 0);
		}

		break;

	case Q2TE_SCREEN_SPARKS:
	case Q2TE_SHIELD_SPARKS:
		MSG_ReadPos (pos);
		MSG_ReadDir (dir);
		if (type == Q2TE_SCREEN_SPARKS)
			P_RunParticleEffect (pos, dir, 0xd0, 40);
		else
			P_RunParticleEffect (pos, dir, 0xb0, 40);
		//FIXME : replace or remove this sound
		S_StartSound (-2, 0, S_PrecacheSound ("weapons/lashit.wav"), pos, 1, 1);
		break;

	case Q2TE_SHOTGUN:			// bullet hitting wall
		MSG_ReadPos (pos);
		MSG_ReadDir (dir);
		P_RunParticleEffect (pos, dir, 0, 20);
		CL_SmokeAndFlash(pos);
		R_AddStain(pos, -10, -10, -10, 20);
		break;

	case Q2TE_SPLASH:			// bullet hitting water
		cnt = MSG_ReadByte ();
		MSG_ReadPos (pos);
		MSG_ReadDir (dir);
		r = MSG_ReadByte ();
		if (r > 6)
			color = 0x00;
		else
			color = splash_color[r];
		P_RunParticleEffect (pos, dir, color, cnt);

		if (r == Q2SPLASH_SPARKS)
		{
			r = rand() & 3;
			if (r == 1)
				Q2S_StartSound (pos, 0, 0, S_PrecacheSound ("world/spark5.wav"), 1, ATTN_NORM, 0);
			else if (r == 2)
				Q2S_StartSound (pos, 0, 0, S_PrecacheSound ("world/spark6.wav"), 1, ATTN_NORM, 0);
			else
				Q2S_StartSound (pos, 0, 0, S_PrecacheSound ("world/spark7.wav"), 1, ATTN_NORM, 0);

//			if (r == 0)
//				Q2S_StartSound (pos, 0, 0, cl_sfx_spark5, 1, ATTN_STATIC, 0);
//			else if (r == 1)
//				Q2S_StartSound (pos, 0, 0, cl_sfx_spark6, 1, ATTN_STATIC, 0);
//			else
//				Q2S_StartSound (pos, 0, 0, cl_sfx_spark7, 1, ATTN_STATIC, 0);
		}
		break;

	case Q2TE_LASER_SPARKS:
		cnt = MSG_ReadByte ();
		MSG_ReadPos (pos);
		MSG_ReadDir (dir);
		color = MSG_ReadByte ();
		P_RunParticleEffect (pos, dir, color, cnt);
		break;

	// RAFAEL
	case Q2TE_BLUEHYPERBLASTER:
		MSG_ReadPos (pos);
		MSG_ReadPos (dir);
		if (P_RunParticleEffectType(pos, dir, 1, pt_blasterparticles))
			P_RunParticleEffect (pos, dir, 0xe0, 40);
		break;

	case Q2TE_BLASTER:			// blaster hitting wall
		MSG_ReadPos (pos);
		MSG_ReadDir (dir);

		if (P_RunParticleEffectType(pos, dir, 1, pt_blasterparticles))
			P_RunParticleEffect (pos, dir, 0xe0, 40);

		R_AddStain(pos, 0, -5, -10, 20);

		ex = CL_AllocExplosion ();
		VectorCopy (pos, ex->origin);
		ex->start = cl.time;
		ex->model = Mod_ForName (q2tentmodels[q2cl_mod_explode].modelname, false);
		ex->firstframe = 0;
		ex->numframes = 4;

		ex->angles[0] = acos(dir[2])/M_PI*180;
	// PMM - fixed to correct for pitch of 0
		if (dir[0])
			ex->angles[1] = atan2(dir[1], dir[0])/M_PI*180;
		else if (dir[1] > 0)
			ex->angles[1] = 90;
		else if (dir[1] < 0)
			ex->angles[1] = 270;
		else
			ex->angles[1] = 0;
		ex->angles[0]*=-1;

		S_StartSound (-2, 0, S_PrecacheSound ("weapons/lashit.wav"), pos, 1, 1);
		break;

	case Q2TE_RAILTRAIL:			// railgun effect
		MSG_ReadPos (pos);
		MSG_ReadPos (pos2);
		CLQ2_RailTrail (pos, pos2);
		Q2S_StartSound (pos, 0, 0, S_PrecacheSound ("weapons/railgf1a.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2TE_EXPLOSION2:
	case Q2TE_GRENADE_EXPLOSION:
	case Q2TE_GRENADE_EXPLOSION_WATER:
		MSG_ReadPos (pos);

		P_ParticleExplosion (pos);
				
	// light
		if (r_explosionlight.value) {
			dlight_t *dl;
			dl = CL_AllocDlight (0);
			VectorCopy (pos, dl->origin);
			dl->radius = 150 + bound(0, r_explosionlight.value, 1)*200;
			dl->die = cl.time + 0.5;
			dl->decay = 300;
			dl->color[0] = 0.2;
			dl->color[1] = 0.1;
			dl->color[2] = 0.1;
		}
	
	// sound
		if (type == Q2TE_GRENADE_EXPLOSION_WATER)
			S_StartSound (-2, 0, S_PrecacheSound ("weapons/xpld_wat.wav"), pos, 1, 1);
		else
			S_StartSound (-2, 0, S_PrecacheSound ("weapons/grenlx1a.wav"), pos, 1, 1);
	
	// sprite
		/*
		if (!R_ParticleExplosionHeart(pos))
		{
			ex = CL_AllocExplosion ();
			VectorCopy (pos, ex->origin);
			VectorClear(ex->angles);
			ex->start = cl.time;
			ex->model = Mod_ForName (q2tentmodels[q2cl_mod_explo4].modelname, true);
			ex->firstframe = 30;
			ex->numframes = 19;
		}
		*/
		break;
/*
		ex = CL_AllocExplosion ();
		VectorCopy (pos, ex->ent.origin);
		ex->type = ex_poly;
		ex->ent.flags = RF_FULLBRIGHT;
		ex->start = cl.frame.servertime - 100;
		ex->light = 350;
		ex->lightcolor[0] = 1.0;
		ex->lightcolor[1] = 0.5;
		ex->lightcolor[2] = 0.5;
		ex->ent.model = cl_mod_explo4;
		ex->frames = 19;
		ex->baseframe = 30;
		ex->ent.angles[1] = rand() % 360;
		CL_ExplosionParticles (pos);
		if (type == TE_GRENADE_EXPLOSION_WATER)
			Q2S_StartSound (pos, 0, 0, cl_sfx_watrexp, 1, ATTN_NORM, 0);
		else
			Q2S_StartSound (pos, 0, 0, cl_sfx_grenexp, 1, ATTN_NORM, 0);
		break;
*/
	// RAFAEL
	case Q2TE_PLASMA_EXPLOSION:
		MSG_ReadPos (pos);
/*		ex = CL_AllocExplosion ();
		VectorCopy (pos, ex->ent.origin);
		ex->type = ex_poly;
		ex->ent.flags = RF_FULLBRIGHT;
		ex->start = cl.frame.servertime - 100;
		ex->light = 350;
		ex->lightcolor[0] = 1.0; 
		ex->lightcolor[1] = 0.5;
		ex->lightcolor[2] = 0.5;
		ex->ent.angles[1] = rand() % 360;
		ex->ent.model = cl_mod_explo4;
		if (frand() < 0.5)
			ex->baseframe = 15;
		ex->frames = 15;
		CL_ExplosionParticles (pos);
		Q2S_StartSound (pos, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
*/		break;

	case Q2TE_EXPLOSION1:
	case Q2TE_EXPLOSION1_BIG:						// PMM
	case Q2TE_ROCKET_EXPLOSION:
	case Q2TE_ROCKET_EXPLOSION_WATER:
	case Q2TE_EXPLOSION1_NP:						// PMM
		MSG_ReadPos (pos);

	// particle effect
		if (type != Q2TE_EXPLOSION1_BIG && type != Q2TE_EXPLOSION1_NP)
			P_ParticleExplosion (pos);

	// light
		if (r_explosionlight.value) {
			dlight_t *dl;
			dl = CL_AllocDlight (0);
			VectorCopy (pos, dl->origin);
			dl->radius = 150 + bound(0, r_explosionlight.value, 1)*200;
			dl->die = cl.time + 0.5;
			dl->decay = 300;
			dl->color[0] = 0.2;
			dl->color[1] = 0.1;
			dl->color[2] = 0.08;
		}

	// sound
		if (type == Q2TE_ROCKET_EXPLOSION_WATER)
			S_StartSound (-2, 0, S_PrecacheSound ("weapons/xpld_wat.wav"), pos, 1, 1);
		else
			S_StartSound (-2, 0, S_PrecacheSound ("weapons/rocklx1a.wav"), pos, 1, 1);

	// sprite		
/*		if (!R_ParticleExplosionHeart(pos))
		{
			ex = CL_AllocExplosion ();
			VectorCopy (pos, ex->origin);
			VectorClear(ex->angles);
			ex->start = cl.time;
			ex->model = Mod_ForName (q2tentmodels[q2cl_mod_explo4].modelname, false);
			if (rand()&1)
				ex->firstframe = 15;
			else
				ex->firstframe = 0;
			ex->numframes = 15;
		}*/
		break;
/*
		ex = CL_AllocExplosion ();
		VectorCopy (pos, ex->ent.origin);
		ex->type = ex_poly;
		ex->ent.flags = RF_FULLBRIGHT;
		ex->start = cl.frame.servertime - 100;
		ex->light = 350;
		ex->lightcolor[0] = 1.0;
		ex->lightcolor[1] = 0.5;
		ex->lightcolor[2] = 0.5;
		ex->ent.angles[1] = rand() % 360;
		if (type != TE_EXPLOSION1_BIG)				// PMM
			ex->ent.model = cl_mod_explo4;			// PMM
		else
			ex->ent.model = cl_mod_explo4_big;
		if (frand() < 0.5)
			ex->baseframe = 15;
		ex->frames = 15;
		if ((type != TE_EXPLOSION1_BIG) && (type != TE_EXPLOSION1_NP))		// PMM
			CL_ExplosionParticles (pos);									// PMM
		if (type == TE_ROCKET_EXPLOSION_WATER)
			Q2S_StartSound (pos, 0, 0, cl_sfx_watrexp, 1, ATTN_NORM, 0);
		else
			Q2S_StartSound (pos, 0, 0, cl_sfx_rockexp, 1, ATTN_NORM, 0);
		break;

*/	case Q2TE_BFG_EXPLOSION:
		MSG_ReadPos (pos);
/*		ex = CL_AllocExplosion ();
		VectorCopy (pos, ex->ent.origin);
		ex->type = ex_poly;
		ex->flags = RF_FULLBRIGHT;
		ex->start = cl.q2frame.servertime - 100;
		ex->light = 350;
		ex->lightcolor[0] = 0.0;
		ex->lightcolor[1] = 1.0;
		ex->lightcolor[2] = 0.0;
		ex->model = cl_mod_bfg_explo;
		ex->flags |= RF_TRANSLUCENT;
		ex->alpha = 0.30;
		ex->frames = 4;
*/		break;

	case Q2TE_BFG_BIGEXPLOSION:
		MSG_ReadPos (pos);
//		CL_BFGExplosionParticles (pos);
		break;

	case Q2TE_BFG_LASER:
		MSG_ReadPos (pos);
		MSG_ReadPos (pos2);
		CL_Laser(pos, pos2, 0xd0d1d2d3);
		break;

	case Q2TE_BUBBLETRAIL:
		MSG_ReadPos (pos);
		MSG_ReadPos (pos2);
		CLQ2_BubbleTrail (pos, pos2);
		break;

	case Q2TE_PARASITE_ATTACK:
	case Q2TE_MEDIC_CABLE_ATTACK:
		CL_ParseBeam (3);
		break;

	case Q2TE_BOSSTPORT:			// boss teleporting to station
		MSG_ReadPos (pos);
/*		CL_BigTeleportParticles (pos);
*/		Q2S_StartSound (pos, 0, 0, S_PrecacheSound ("misc/bigtele.wav"), 1, ATTN_NONE, 0);
		break;

	case Q2TE_GRAPPLE_CABLE:
		CL_ParseBeam (4);
		MSG_ReadPos (pos);
		break;

	// RAFAEL
	case Q2TE_WELDING_SPARKS:
		cnt = MSG_ReadByte ();
		MSG_ReadPos (pos);
		MSG_ReadDir (dir);
		color = MSG_ReadByte ();
/*		CL_ParticleEffect2 (pos, dir, color, cnt);

		ex = CL_AllocExplosion ();
		VectorCopy (pos, ex->ent.origin);
		ex->type = ex_flash;
		// note to self
		// we need a better no draw flag
		ex->ent.flags = RF_BEAM;
		ex->start = cl.frame.servertime - 0.1;
		ex->light = 100 + (rand()%75);
		ex->lightcolor[0] = 1.0;
		ex->lightcolor[1] = 1.0;
		ex->lightcolor[2] = 0.3;
		ex->ent.model = cl_mod_flash;
		ex->frames = 2;
*/		break;

	case Q2TE_GREENBLOOD:
		MSG_ReadPos (pos);
		MSG_ReadDir (dir);
//		CL_ParticleEffect2 (pos, dir, 0xdf, 30);
		break;

	// RAFAEL
	case Q2TE_TUNNEL_SPARKS:
		cnt = MSG_ReadByte ();
		MSG_ReadPos (pos);
		MSG_ReadDir (dir);
		color = MSG_ReadByte ();
//		CL_ParticleEffect3 (pos, dir, color, cnt);
		break;

//=============
//PGM
		// PMM -following code integrated for flechette (different color)
	case Q2TE_BLASTER2:			// green blaster hitting wall
	case Q2TE_FLECHETTE:			// flechette
		MSG_ReadPos (pos);
		MSG_ReadDir (dir);
		
		// PMM
/*		if (type == Q2TE_BLASTER2)
			CL_BlasterParticles2 (pos, dir, 0xd0);
		else
			CL_BlasterParticles2 (pos, dir, 0x6f); // 75

		ex = CL_AllocExplosion ();
		VectorCopy (pos, ex->ent.origin);
		ex->ent.angles[0] = acos(dir[2])/M_PI*180;
	// PMM - fixed to correct for pitch of 0
		if (dir[0])
			ex->ent.angles[1] = atan2(dir[1], dir[0])/M_PI*180;
		else if (dir[1] > 0)
			ex->ent.angles[1] = 90;
		else if (dir[1] < 0)
			ex->ent.angles[1] = 270;
		else
			ex->ent.angles[1] = 0;

		ex->type = ex_misc;
		ex->ent.flags = Q2RF_FULLBRIGHT|Q2RF_TRANSLUCENT;

		// PMM
		if (type == Q2TE_BLASTER2)
			ex->ent.skinnum = 1;
		else // flechette
			ex->ent.skinnum = 2;

		ex->start = cl.frame.servertime - 100;
		ex->light = 150;
		// PMM
		if (type == Q2TE_BLASTER2)
			ex->lightcolor[1] = 1;
		else // flechette
		{
			ex->lightcolor[0] = 0.19;
			ex->lightcolor[1] = 0.41;
			ex->lightcolor[2] = 0.75;
		}
		ex->ent.model = cl_mod_explode;
		ex->frames = 4;
		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
*//*		break;


	case Q2TE_LIGHTNING:
		ent = CL_ParseLightning (cl_mod_lightning);
		S_StartSound (NULL, ent, CHAN_WEAPON, cl_sfx_lightning, 1, ATTN_NORM, 0);
		break;
*/
	case Q2TE_DEBUGTRAIL:
		MSG_ReadPos (pos);
		MSG_ReadPos (pos2);
		P_ParticleTrail(pos, pos2, P_AllocateParticleType("te_debugtrail"), NULL);
		break;

	case Q2TE_PLAIN_EXPLOSION:
		MSG_ReadPos (pos);

		ex = CL_AllocExplosion ();
		VectorCopy (pos, ex->origin);
//		ex->type = ex_poly;
		ex->flags = Q2RF_FULLBRIGHT;
		ex->light = 350;
		ex->lightcolor[0] = 1.0;
		ex->lightcolor[1] = 0.5;
		ex->lightcolor[2] = 0.5;
		ex->angles[1] = rand() % 360;
		ex->model = Mod_ForName (q2tentmodels[q2cl_mod_explo4].modelname, false);
		if (rand() < RAND_MAX/2)
			ex->firstframe = 15;
		ex->numframes = 15;
		Q2S_StartSound (pos, 0, 0, S_PrecacheSound("weapons/rocklx1a.wav"), 1, ATTN_NORM, 0);
		break;
/*
	case Q2TE_FLASHLIGHT:
		MSG_ReadPos(&net_message, pos);
		ent = MSG_ReadShort(&net_message);
		CL_Flashlight(ent, pos);
		break;

	case Q2TE_FORCEWALL:
		MSG_ReadPos(&net_message, pos);
		MSG_ReadPos(&net_message, pos2);
		color = MSG_ReadByte (&net_message);
		CL_ForceWall(pos, pos2, color);
		break;

	case Q2TE_HEATBEAM:
		ent = CL_ParsePlayerBeam (cl_mod_heatbeam);
		break;

	case Q2TE_MONSTER_HEATBEAM:
		ent = CL_ParsePlayerBeam (cl_mod_monster_heatbeam);
		break;

	case Q2TE_HEATBEAM_SPARKS:
//		cnt = MSG_ReadByte (&net_message);
		cnt = 50;
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
//		r = MSG_ReadByte (&net_message);
//		magnitude = MSG_ReadShort (&net_message);
		r = 8;
		magnitude = 60;
		color = r & 0xff;
		CL_ParticleSteamEffect (pos, dir, color, cnt, magnitude);
		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;
	
	case Q2TE_HEATBEAM_STEAM:
//		cnt = MSG_ReadByte (&net_message);
		cnt = 20;
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
//		r = MSG_ReadByte (&net_message);
//		magnitude = MSG_ReadShort (&net_message);
//		color = r & 0xff;
		color = 0xe0;
		magnitude = 60;
		CL_ParticleSteamEffect (pos, dir, color, cnt, magnitude);
		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case Q2TE_STEAM:
		CL_ParseSteam();
		break;

	case Q2TE_BUBBLETRAIL2:
//		cnt = MSG_ReadByte (&net_message);
		cnt = 8;
		MSG_ReadPos (&net_message, pos);
		MSG_ReadPos (&net_message, pos2);
		CL_BubbleTrail2 (pos, pos2, cnt);
		S_StartSound (pos,  0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case Q2TE_MOREBLOOD:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
		CL_ParticleEffect (pos, dir, 0xe8, 250);
		break;

	case Q2TE_CHAINFIST_SMOKE:
		dir[0]=0; dir[1]=0; dir[2]=1;
		MSG_ReadPos(&net_message, pos);
		CL_ParticleSmokeEffect (pos, dir, 0, 20, 20);
		break;

	case Q2TE_ELECTRIC_SPARKS:
		MSG_ReadPos (&net_message, pos);
		MSG_ReadDir (&net_message, dir);
//		CL_ParticleEffect (pos, dir, 109, 40);
		CL_ParticleEffect (pos, dir, 0x75, 40);
		//FIXME : replace or remove this sound
		S_StartSound (pos, 0, 0, cl_sfx_lashit, 1, ATTN_NORM, 0);
		break;

	case Q2TE_TRACKER_EXPLOSION:
		MSG_ReadPos (&net_message, pos);
		CL_ColorFlash (pos, 0, 150, -1, -1, -1);
		CL_ColorExplosionParticles (pos, 0, 1);
//		CL_Tracker_Explode (pos);
		S_StartSound (pos, 0, 0, cl_sfx_disrexp, 1, ATTN_NORM, 0);
		break;
*/
	case Q2TE_TELEPORT_EFFECT:
	case Q2TE_DBALL_GOAL:
		MSG_ReadPos (pos);
		if (P_RunParticleEffectType(pos, NULL, 1, pt_teleportsplash))
			P_RunParticleEffect(pos, NULL, 8, 768);
		// This effect won't match ---
		// Color should be 7+(rand()%8)
		// not 8&~7+(rand()%8)
		break;
/*
	case Q2TE_WIDOWBEAMOUT:
		CL_ParseWidow ();
		break;

	case Q2TE_NUKEBLAST:
		CL_ParseNuke ();
		break;

	case Q2TE_WIDOWSPLASH:
		MSG_ReadPos (&net_message, pos);
		CL_WidowSplash (pos);
		break;
//PGM
//==============
*/
	default:
		Host_EndGame ("CLQ2_ParseTEnt: bad/non-implemented type %i", type);
	}
}
#endif


/*
=================
CL_NewTempEntity
=================
*/
entity_t *CL_NewTempEntity (void)
{
	entity_t	*ent;

	if (cl_numvisedicts == MAX_VISEDICTS)
		return NULL;
	ent = &cl_visedicts[cl_numvisedicts];
	cl_numvisedicts++;
	ent->keynum = 0;
	
	memset (ent, 0, sizeof(*ent));

	ent->colormap = vid.colormap;
#ifdef PEXT_SCALE
	ent->scale = 1;
#endif
#ifdef PEXT_TRANS
	ent->alpha = 1;
#endif
	return ent;
}


/*
=================
CL_UpdateBeams
=================
*/
void CL_UpdateBeams (void)
{
	int			i;
	beam_t		*b;
	vec3_t		dist, org;
	float		d;
	entity_t	*ent;
	entity_state_t *st;
	float		yaw, pitch;
	float		forward, offset;

	extern cvar_t cl_truelightning, v_viewheight;

// update lightning
	for (i=0, b=cl_beams ; i< MAX_BEAMS ; i++, b++)
	{
		if (!b->model)
			continue;
		
		if (b->endtime < cl.time)
		{
			P_DelinkTrailstate(&b->trailstate);
			b->model = NULL;
			continue;
		}

	// if coming from the player, update the start position
		if (b->flags & 1 && b->entity == (autocam[0]?spec_track[0]:(cl.playernum[0]+1)) && b->entity>0 && b->entity<= MAX_CLIENTS)	// entity 0 is the world
		{
			player_state_t	*pl;
//			VectorSubtract(cl.simorg, b->start, org);
//			VectorAdd(b->end, org, b->end);		//move the end point by simorg-start

			pl = &cl.frames[cl.parsecount&UPDATE_MASK].playerstate[b->entity-1];
			if (pl->messagenum == cl.parsecount)
			{
				VectorCopy (cl.simorg[0], b->start);	//move the start point to view origin
				b->start[2] += cl.crouch[0];
				if (v_viewheight.value)
				{
					if (v_viewheight.value <= -7)
						b->start[2] += -7;
					else if (v_viewheight.value >= 4)
						b->start[2] += 4;
					else
						b->start[2] += v_viewheight.value;
				}


				//rotate the end point to face in the view direction. This gives a smoother shafting. turning looks great.
				if (cl_truelightning.value)
				{
					VectorSubtract (b->end, b->start, dist);
					d = VectorNormalize(dist);
					AngleVectors (cl.simangles[0], b->end, dist, org);
					VectorMA(b->start, d, b->end, b->end);
					b->end[2] += cl.viewheight[0];
				}
			}
		}
		else if (b->flags & STREAM_ATTACHED)
		{
			player_state_t	*pl;
			st = CL_FindPacketEntity(b->entity);
			if (st)
			{
				VectorCopy(st->origin, b->start);
			}
			else if (b->entity <= MAX_CLIENTS && b->entity > 0)
			{
				pl = &cl.frames[cl.parsecount&UPDATE_MASK].playerstate[b->entity-1];
				VectorCopy(pl->origin, b->start);
				b->start[2]+=16;
			}
		}

	// calculate pitch and yaw
		VectorSubtract (b->end, b->start, dist);

		if (dist[1] == 0 && dist[0] == 0)
		{
			yaw = 0;
			if (dist[2] > 0)
				pitch = 90;
			else
				pitch = 270;
		}
		else
		{
			yaw = (int) (atan2(dist[1], dist[0]) * 180 / M_PI);
			if (yaw < 0)
				yaw += 360;
	
			forward = sqrt (dist[0]*dist[0] + dist[1]*dist[1]);
			pitch = (int) (atan2(dist[2], forward) * 180 / M_PI);
			if (pitch < 0)
				pitch += 360;
		}


/*		if (1)	//cool funky particle mode.
		{
			CL_LightningParticleBeam(b->start, b->end);
			continue;
		}
*/
//		if (part_type[rt_lightning1].loaded)
//		if (!P_ParticleTrail(b->start, b->end, rt_lightning1, NULL))
//			continue;
		if (b->particleeffect >= 0 && !P_ParticleTrail(b->start, b->end, b->particleeffect, &b->trailstate))
			continue;

	// add new entities for the lightning
		VectorCopy (b->start, org);
		d = VectorNormalize(dist);

		if(b->flags & 2)
		{
			offset = (int)(cl.time*40)%30;
			for(i = 0; i < 3; i++)
			{
				org[i] += dist[i]*offset;
			}
		}

		while (d > 0)
		{
			ent = CL_NewTempEntity ();
			if (!ent)
				return;
			VectorCopy (org, ent->origin);
			ent->model = b->model;
			ent->drawflags |= MLS_ABSLIGHT;
			ent->abslight = 192;
			ent->alpha = b->alpha;

			ent->angles[0] = -pitch;
			ent->angles[1] = yaw;
			ent->angles[2] = (int)((cl.time*d*1000))%360;	//paused lightning too.
			AngleVectors(ent->angles, ent->axis[0], ent->axis[1], ent->axis[2]);
			ent->angles[0] = pitch;

			for (i=0 ; i<3 ; i++)
				org[i] += dist[i]*30;
			d -= 30;
		}
	}
	
}

/*
=================
CL_UpdateExplosions
=================
*/
void CL_UpdateExplosions (void)
{
	int			i;
	float		f;
	int			of;
	int numframes;
	int firstframe;
	explosion_t	*ex;
	entity_t	*ent;

	for (i=0, ex=cl_explosions ; i< MAX_EXPLOSIONS ; i++, ex++)
	{
		if (!ex->model)
			continue;
		f = ex->framerate*(cl.time - ex->start);
		if (ex->firstframe >= 0)
		{
			firstframe = ex->firstframe;
			numframes = ex->numframes;
		}
		else
		{
			firstframe = 0;
			numframes = ex->model->numframes;
		}

		of = (int)f-1;
		if ((int)f >= numframes || (int)f < 0)
		{
			ex->model = NULL;
			continue;
		}
		if (of < 0)
			of = 0;

		ent = CL_NewTempEntity ();
		if (!ent)
			return;
		VectorCopy (ex->origin, ent->origin);
		VectorCopy (ex->oldorigin, ent->oldorigin);
		VectorCopy (ex->angles, ent->angles);
		ent->skinnum = ex->skinnum;
		ent->angles[0]*=-1;
		AngleVectors(ent->angles, ent->axis[0], ent->axis[1], ent->axis[2]);
		VectorInverse(ent->axis[1]);
		ent->model = ex->model;
		ent->frame = (int)f+firstframe;
		ent->oldframe = of+firstframe;
		ent->lerpfrac = 1-(f - (int)f);
		ent->alpha = 1.0 - f/(numframes);
		ent->flags = ex->flags;
	}
}

entity_state_t *CL_FindPacketEntity(int num);
void CL_UpdateSEEFs(void)
{
	float *eorg;
	int i;
	dlight_t *dl;
	entity_state_t *ent;
	for(i = 0; i < MAX_SEEFS; i++)
	{
		if (!cl_seef[i].type)
			continue;

		if (cl_seef[i].die < cl.time)
			continue;
		ent = CL_FindPacketEntity(cl_seef[i].entnum);
		if (!ent)
		{
			extern int parsecountmod;
			if ((unsigned)(cl_seef[i].entnum) <= MAX_CLIENTS && cl_seef[i].entnum > 0)
			{
				if (cl_seef[i].entnum-1 == cl.playernum[0])
					eorg = cl.simorg[0];
				else
					eorg = cl.frames[parsecountmod].playerstate[cl_seef[i].entnum-1].origin;
			}
			else
				continue;
		}
		else
			eorg = ent->origin;
		ent = NULL;

		switch (cl_seef[i].type)
		{
		case TE_SEEF_BRIGHTFIELD:
			if (!cl.paused)
			{
				vec3_t org;
				org[0] = eorg[0];
				org[1] = eorg[1];
				org[2] = eorg[2] + cl_seef[i].offset;
				P_EntityParticles(org, cl_seef[i].colour, cl_seef[i].efsize);
			}
			break;
		case TE_SEEF_DARKFIELD:
			if (!cl.paused)
				P_DarkFieldParticles(eorg, cl_seef[i].colour);
			break;
		case TE_SEEF_DARKLIGHT:
			dl = CL_AllocDlight (cl_seef[i].entnum);
			VectorCopy (eorg, dl->origin);
			dl->radius = cl_seef[i].efsize[0] + rand()/(float)RAND_MAX*cl_seef[i].efsize[1];
			dl->die = cl.time+0.1;
			dl->decay = 0;
			dl->color[0] = -0.1;
			dl->color[1] = -0.1;
			dl->color[2] = -0.1;
			break;
		case TE_SEEF_LIGHT:
			dl = CL_AllocDlight (cl_seef[i].entnum);
			VectorCopy (eorg, dl->origin);
			dl->radius = cl_seef[i].efsize[0] + rand()/(float)RAND_MAX*cl_seef[i].efsize[1];
			dl->die = cl.time+0.1;
			dl->decay = 0;
			dl->color[0] = 0.1;
			dl->color[1] = 0.1;
			dl->color[2] = 0.1;
			break;
		default:
			Sys_Error("Bad seef type\n");
		}
	}
}

/*
=================
CL_UpdateTEnts
=================
*/
void CL_UpdateTEnts (void)
{
	CL_UpdateBeams ();
	CL_UpdateExplosions ();
	CL_UpdateSEEFs ();
}
