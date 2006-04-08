#include "quakedef.h"
#include "particles.h"

#ifdef Q2CLIENT
#include "shader.h"

extern cvar_t r_drawviewmodel;

extern cvar_t cl_nopred;
typedef enum
{
	Q2EV_NONE,
	Q2EV_ITEM_RESPAWN,
	Q2EV_FOOTSTEP,
	Q2EV_FALLSHORT,
	Q2EV_FALL,
	Q2EV_FALLFAR,
	Q2EV_PLAYER_TELEPORT,
	Q2EV_OTHER_TELEPORT
} q2entity_event_t;

#define Q2PMF_NO_PREDICTION	64	// temporarily disables prediction (used for grappling hook)

float LerpAngle (float a2, float a1, float frac)
{
	if (a1 - a2 > 180)
		a1 -= 360;
	if (a1 - a2 < -180)
		a1 += 360;
	return a2 + frac * (a1 - a2);
}

// entity_state_t->effects
// Effects are things handled on the client side (lights, particles, frame animations)
// that happen constantly on the given entity.
// An entity that has effects will be sent to the client
// even if it has a zero index model.
#define	Q2EF_ROTATE				0x00000001		// rotate (bonus items)
#define	Q2EF_GIB				0x00000002		// leave a trail
#define	Q2EF_BLASTER			0x00000008		// redlight + trail
#define	Q2EF_ROCKET				0x00000010		// redlight + trail
#define	Q2EF_GRENADE			0x00000020
#define	Q2EF_HYPERBLASTER		0x00000040
#define	Q2EF_BFG				0x00000080
#define Q2EF_COLOR_SHELL		0x00000100
#define Q2EF_POWERSCREEN		0x00000200
#define	Q2EF_ANIM01				0x00000400		// automatically cycle between frames 0 and 1 at 2 hz
#define	Q2EF_ANIM23				0x00000800		// automatically cycle between frames 2 and 3 at 2 hz
#define Q2EF_ANIM_ALL			0x00001000		// automatically cycle through all frames at 2hz
#define Q2EF_ANIM_ALLFAST		0x00002000		// automatically cycle through all frames at 10hz
#define	Q2EF_FLIES				0x00004000
#define	Q2EF_QUAD				0x00008000
#define	Q2EF_PENT				0x00010000
#define	Q2EF_TELEPORTER			0x00020000		// particle fountain
#define Q2EF_FLAG1				0x00040000
#define Q2EF_FLAG2				0x00080000
// RAFAEL
#define Q2EF_IONRIPPER			0x00100000
#define Q2EF_GREENGIB			0x00200000
#define	Q2EF_BLUEHYPERBLASTER	0x00400000
#define Q2EF_SPINNINGLIGHTS		0x00800000
#define Q2EF_PLASMA				0x01000000
#define Q2EF_TRAP				0x02000000

//ROGUE
#define Q2EF_TRACKER			0x04000000
#define	Q2EF_DOUBLE				0x08000000
#define	Q2EF_SPHERETRANS		0x10000000
#define Q2EF_TAGTRAIL			0x20000000
#define Q2EF_HALF_DAMAGE		0x40000000
#define Q2EF_TRACKERTRAIL		0x80000000
//ROGUE




#define Q2MAX_STATS	32



typedef struct q2centity_s
{
	entity_state_t	baseline;		// delta from this if not from a previous frame
	entity_state_t	current;
	entity_state_t	prev;			// will always be valid, but might just be a copy of current

	int			serverframe;		// if not current, this ent isn't in the frame

	trailstate_t *trailstate;
//	float		trailcount;			// for diminishing grenade trails
	vec3_t		lerp_origin;		// for trails (variable hz)

//	int			fly_stoptime;
} q2centity_t;


void CLQ2_EntityEvent(entity_state_t *es){};
void CLQ2_TeleporterParticles(entity_state_t *es){};
void CLQ2_Tracker_Shell(vec3_t org){};
void CLQ2_TrapParticles(entity_t *ent){};
void CLQ2_BfgParticles(entity_t *ent){};
void CLQ2_FlyEffect(q2centity_t *ent, vec3_t org){};
void CLQ2_BlasterTrail2(vec3_t oldorg, vec3_t neworg){};


#define MAX_Q2EDICTS 1024
#define	MAX_PARSE_ENTITIES	1024


q2centity_t cl_entities[MAX_Q2EDICTS];
entity_state_t	cl_parse_entities[MAX_PARSE_ENTITIES];

void Q2S_StartSound(vec3_t origin, int entnum, int entchannel, sfx_t *sfx, float fvol, float attenuation, float timeofs);
void CL_SmokeAndFlash(vec3_t origin);

//extern	struct model_s	*cl_mod_powerscreen;

//PGM
int	vidref_val;
//PGM
#include "q2m_flash.c"
void CLQ2_RunMuzzleFlash2 (int ent, int flash_number)
{
	vec3_t		origin;
	dlight_t	*dl;
	vec3_t		forward, right, up;
	char		soundname[64];

	// locate the origin
	AngleVectors (cl_entities[ent].current.angles, forward, right, up);
	origin[0] = cl_entities[ent].current.origin[0] + forward[0] * monster_flash_offset[flash_number][0] + right[0] * monster_flash_offset[flash_number][1];
	origin[1] = cl_entities[ent].current.origin[1] + forward[1] * monster_flash_offset[flash_number][0] + right[1] * monster_flash_offset[flash_number][1];
	origin[2] = cl_entities[ent].current.origin[2] + forward[2] * monster_flash_offset[flash_number][0] + right[2] * monster_flash_offset[flash_number][1] + monster_flash_offset[flash_number][2];

	dl = CL_AllocDlight (ent);
	VectorCopy (origin,  dl->origin);
	dl->radius = 200 + (rand()&31);
//	dl->minlight = 32;
	dl->die = cl.time + 0.1;

	switch (flash_number)
	{
	case Q2MZ2_INFANTRY_MACHINEGUN_1:
	case Q2MZ2_INFANTRY_MACHINEGUN_2:
	case Q2MZ2_INFANTRY_MACHINEGUN_3:
	case Q2MZ2_INFANTRY_MACHINEGUN_4:
	case Q2MZ2_INFANTRY_MACHINEGUN_5:
	case Q2MZ2_INFANTRY_MACHINEGUN_6:
	case Q2MZ2_INFANTRY_MACHINEGUN_7:
	case Q2MZ2_INFANTRY_MACHINEGUN_8:
	case Q2MZ2_INFANTRY_MACHINEGUN_9:
	case Q2MZ2_INFANTRY_MACHINEGUN_10:
	case Q2MZ2_INFANTRY_MACHINEGUN_11:
	case Q2MZ2_INFANTRY_MACHINEGUN_12:
	case Q2MZ2_INFANTRY_MACHINEGUN_13:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		P_RunParticleEffect (origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("infantry/infatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_SOLDIER_MACHINEGUN_1:
	case Q2MZ2_SOLDIER_MACHINEGUN_2:
	case Q2MZ2_SOLDIER_MACHINEGUN_3:
	case Q2MZ2_SOLDIER_MACHINEGUN_4:
	case Q2MZ2_SOLDIER_MACHINEGUN_5:
	case Q2MZ2_SOLDIER_MACHINEGUN_6:
	case Q2MZ2_SOLDIER_MACHINEGUN_7:
	case Q2MZ2_SOLDIER_MACHINEGUN_8:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		P_RunParticleEffect (origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("soldier/solatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_GUNNER_MACHINEGUN_1:
	case Q2MZ2_GUNNER_MACHINEGUN_2:
	case Q2MZ2_GUNNER_MACHINEGUN_3:
	case Q2MZ2_GUNNER_MACHINEGUN_4:
	case Q2MZ2_GUNNER_MACHINEGUN_5:
	case Q2MZ2_GUNNER_MACHINEGUN_6:
	case Q2MZ2_GUNNER_MACHINEGUN_7:
	case Q2MZ2_GUNNER_MACHINEGUN_8:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		P_RunParticleEffect (origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("gunner/gunatck2.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_ACTOR_MACHINEGUN_1:
	case Q2MZ2_SUPERTANK_MACHINEGUN_1:
	case Q2MZ2_SUPERTANK_MACHINEGUN_2:
	case Q2MZ2_SUPERTANK_MACHINEGUN_3:
	case Q2MZ2_SUPERTANK_MACHINEGUN_4:
	case Q2MZ2_SUPERTANK_MACHINEGUN_5:
	case Q2MZ2_SUPERTANK_MACHINEGUN_6:
	case Q2MZ2_TURRET_MACHINEGUN:			// PGM
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;

		P_RunParticleEffect (origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("infantry/infatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_BOSS2_MACHINEGUN_L1:
	case Q2MZ2_BOSS2_MACHINEGUN_L2:
	case Q2MZ2_BOSS2_MACHINEGUN_L3:
	case Q2MZ2_BOSS2_MACHINEGUN_L4:
	case Q2MZ2_BOSS2_MACHINEGUN_L5:
	case Q2MZ2_CARRIER_MACHINEGUN_L1:		// PMM
	case Q2MZ2_CARRIER_MACHINEGUN_L2:		// PMM
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;

		P_RunParticleEffect (origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("infantry/infatck1.wav"), 1, ATTN_NONE, 0);
		break;

	case Q2MZ2_SOLDIER_BLASTER_1:
	case Q2MZ2_SOLDIER_BLASTER_2:
	case Q2MZ2_SOLDIER_BLASTER_3:
	case Q2MZ2_SOLDIER_BLASTER_4:
	case Q2MZ2_SOLDIER_BLASTER_5:
	case Q2MZ2_SOLDIER_BLASTER_6:
	case Q2MZ2_SOLDIER_BLASTER_7:
	case Q2MZ2_SOLDIER_BLASTER_8:
	case Q2MZ2_TURRET_BLASTER:			// PGM
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("soldier/solatck2.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_FLYER_BLASTER_1:
	case Q2MZ2_FLYER_BLASTER_2:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("flyer/flyatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_MEDIC_BLASTER_1:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("medic/medatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_HOVER_BLASTER_1:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("hover/hovatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_FLOAT_BLASTER_1:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("floater/fltatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_SOLDIER_SHOTGUN_1:
	case Q2MZ2_SOLDIER_SHOTGUN_2:
	case Q2MZ2_SOLDIER_SHOTGUN_3:
	case Q2MZ2_SOLDIER_SHOTGUN_4:
	case Q2MZ2_SOLDIER_SHOTGUN_5:
	case Q2MZ2_SOLDIER_SHOTGUN_6:
	case Q2MZ2_SOLDIER_SHOTGUN_7:
	case Q2MZ2_SOLDIER_SHOTGUN_8:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		CL_SmokeAndFlash(origin);
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("soldier/solatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_TANK_BLASTER_1:
	case Q2MZ2_TANK_BLASTER_2:
	case Q2MZ2_TANK_BLASTER_3:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("tank/tnkatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_TANK_MACHINEGUN_1:
	case Q2MZ2_TANK_MACHINEGUN_2:
	case Q2MZ2_TANK_MACHINEGUN_3:
	case Q2MZ2_TANK_MACHINEGUN_4:
	case Q2MZ2_TANK_MACHINEGUN_5:
	case Q2MZ2_TANK_MACHINEGUN_6:
	case Q2MZ2_TANK_MACHINEGUN_7:
	case Q2MZ2_TANK_MACHINEGUN_8:
	case Q2MZ2_TANK_MACHINEGUN_9:
	case Q2MZ2_TANK_MACHINEGUN_10:
	case Q2MZ2_TANK_MACHINEGUN_11:
	case Q2MZ2_TANK_MACHINEGUN_12:
	case Q2MZ2_TANK_MACHINEGUN_13:
	case Q2MZ2_TANK_MACHINEGUN_14:
	case Q2MZ2_TANK_MACHINEGUN_15:
	case Q2MZ2_TANK_MACHINEGUN_16:
	case Q2MZ2_TANK_MACHINEGUN_17:
	case Q2MZ2_TANK_MACHINEGUN_18:
	case Q2MZ2_TANK_MACHINEGUN_19:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		P_RunParticleEffect (origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
		snprintf(soundname, sizeof(soundname), "tank/tnkatk2%c.wav", 'a' + rand() % 5);
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound(soundname), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_CHICK_ROCKET_1:
	case Q2MZ2_TURRET_ROCKET:			// PGM
		dl->color[0] = 1;dl->color[1] = 0.5;dl->color[2] = 0.2;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("chick/chkatck2.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_TANK_ROCKET_1:
	case Q2MZ2_TANK_ROCKET_2:
	case Q2MZ2_TANK_ROCKET_3:
		dl->color[0] = 1;dl->color[1] = 0.5;dl->color[2] = 0.2;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("tank/tnkatck1.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_SUPERTANK_ROCKET_1:
	case Q2MZ2_SUPERTANK_ROCKET_2:
	case Q2MZ2_SUPERTANK_ROCKET_3:
	case Q2MZ2_BOSS2_ROCKET_1:
	case Q2MZ2_BOSS2_ROCKET_2:
	case Q2MZ2_BOSS2_ROCKET_3:
	case Q2MZ2_BOSS2_ROCKET_4:
	case Q2MZ2_CARRIER_ROCKET_1:
//	case Q2MZ2_CARRIER_ROCKET_2:
//	case Q2MZ2_CARRIER_ROCKET_3:
//	case Q2MZ2_CARRIER_ROCKET_4:
		dl->color[0] = 1;dl->color[1] = 0.5;dl->color[2] = 0.2;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("tank/rocket.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_GUNNER_GRENADE_1:
	case Q2MZ2_GUNNER_GRENADE_2:
	case Q2MZ2_GUNNER_GRENADE_3:
	case Q2MZ2_GUNNER_GRENADE_4:
		dl->color[0] = 1;dl->color[1] = 0.5;dl->color[2] = 0;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("gunner/gunatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_GLADIATOR_RAILGUN_1:
	// PMM
	case Q2MZ2_CARRIER_RAILGUN:
	case Q2MZ2_WIDOW_RAIL:
	// pmm
		dl->color[0] = 0.5;dl->color[1] = 0.5;dl->color[2] = 1.0;
		break;

// --- Xian's shit starts ---
	case Q2MZ2_MAKRON_BFG:
		dl->color[0] = 0.5;dl->color[1] = 1 ;dl->color[2] = 0.5;
		//Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_RegisterSound("makron/bfg_fire.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_MAKRON_BLASTER_1:
	case Q2MZ2_MAKRON_BLASTER_2:
	case Q2MZ2_MAKRON_BLASTER_3:
	case Q2MZ2_MAKRON_BLASTER_4:
	case Q2MZ2_MAKRON_BLASTER_5:
	case Q2MZ2_MAKRON_BLASTER_6:
	case Q2MZ2_MAKRON_BLASTER_7:
	case Q2MZ2_MAKRON_BLASTER_8:
	case Q2MZ2_MAKRON_BLASTER_9:
	case Q2MZ2_MAKRON_BLASTER_10:
	case Q2MZ2_MAKRON_BLASTER_11:
	case Q2MZ2_MAKRON_BLASTER_12:
	case Q2MZ2_MAKRON_BLASTER_13:
	case Q2MZ2_MAKRON_BLASTER_14:
	case Q2MZ2_MAKRON_BLASTER_15:
	case Q2MZ2_MAKRON_BLASTER_16:
	case Q2MZ2_MAKRON_BLASTER_17:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("makron/blaster.wav"), 1, ATTN_NORM, 0);
		break;
	
	case Q2MZ2_JORG_MACHINEGUN_L1:
	case Q2MZ2_JORG_MACHINEGUN_L2:
	case Q2MZ2_JORG_MACHINEGUN_L3:
	case Q2MZ2_JORG_MACHINEGUN_L4:
	case Q2MZ2_JORG_MACHINEGUN_L5:
	case Q2MZ2_JORG_MACHINEGUN_L6:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		P_RunParticleEffect (origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("boss3/xfire.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_JORG_MACHINEGUN_R1:
	case Q2MZ2_JORG_MACHINEGUN_R2:
	case Q2MZ2_JORG_MACHINEGUN_R3:
	case Q2MZ2_JORG_MACHINEGUN_R4:
	case Q2MZ2_JORG_MACHINEGUN_R5:
	case Q2MZ2_JORG_MACHINEGUN_R6:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		P_RunParticleEffect (origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
		break;

	case Q2MZ2_JORG_BFG_1:
		dl->color[0] = 0.5;dl->color[1] = 1 ;dl->color[2] = 0.5;
		break;

	case Q2MZ2_BOSS2_MACHINEGUN_R1:
	case Q2MZ2_BOSS2_MACHINEGUN_R2:
	case Q2MZ2_BOSS2_MACHINEGUN_R3:
	case Q2MZ2_BOSS2_MACHINEGUN_R4:
	case Q2MZ2_BOSS2_MACHINEGUN_R5:
	case Q2MZ2_CARRIER_MACHINEGUN_R1:			// PMM
	case Q2MZ2_CARRIER_MACHINEGUN_R2:			// PMM

		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;

		P_RunParticleEffect (origin, vec3_origin, 0, 40);
		CL_SmokeAndFlash(origin);
		break;

// ======
// ROGUE
	case Q2MZ2_STALKER_BLASTER:
	case Q2MZ2_DAEDALUS_BLASTER:
	case Q2MZ2_MEDIC_BLASTER_2:
	case Q2MZ2_WIDOW_BLASTER:
	case Q2MZ2_WIDOW_BLASTER_SWEEP1:
	case Q2MZ2_WIDOW_BLASTER_SWEEP2:
	case Q2MZ2_WIDOW_BLASTER_SWEEP3:
	case Q2MZ2_WIDOW_BLASTER_SWEEP4:
	case Q2MZ2_WIDOW_BLASTER_SWEEP5:
	case Q2MZ2_WIDOW_BLASTER_SWEEP6:
	case Q2MZ2_WIDOW_BLASTER_SWEEP7:
	case Q2MZ2_WIDOW_BLASTER_SWEEP8:
	case Q2MZ2_WIDOW_BLASTER_SWEEP9:
	case Q2MZ2_WIDOW_BLASTER_100:
	case Q2MZ2_WIDOW_BLASTER_90:
	case Q2MZ2_WIDOW_BLASTER_80:
	case Q2MZ2_WIDOW_BLASTER_70:
	case Q2MZ2_WIDOW_BLASTER_60:
	case Q2MZ2_WIDOW_BLASTER_50:
	case Q2MZ2_WIDOW_BLASTER_40:
	case Q2MZ2_WIDOW_BLASTER_30:
	case Q2MZ2_WIDOW_BLASTER_20:
	case Q2MZ2_WIDOW_BLASTER_10:
	case Q2MZ2_WIDOW_BLASTER_0:
	case Q2MZ2_WIDOW_BLASTER_10L:
	case Q2MZ2_WIDOW_BLASTER_20L:
	case Q2MZ2_WIDOW_BLASTER_30L:
	case Q2MZ2_WIDOW_BLASTER_40L:
	case Q2MZ2_WIDOW_BLASTER_50L:
	case Q2MZ2_WIDOW_BLASTER_60L:
	case Q2MZ2_WIDOW_BLASTER_70L:
	case Q2MZ2_WIDOW_RUN_1:
	case Q2MZ2_WIDOW_RUN_2:
	case Q2MZ2_WIDOW_RUN_3:
	case Q2MZ2_WIDOW_RUN_4:
	case Q2MZ2_WIDOW_RUN_5:
	case Q2MZ2_WIDOW_RUN_6:
	case Q2MZ2_WIDOW_RUN_7:
	case Q2MZ2_WIDOW_RUN_8:
		dl->color[0] = 0;dl->color[1] = 1;dl->color[2] = 0;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("tank/tnkatck3.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_WIDOW_DISRUPTOR:
		dl->color[0] = -1;dl->color[1] = -1;dl->color[2] = -1;
		Q2S_StartSound (NULL, ent, CHAN_WEAPON, S_PrecacheSound("weapons/disint2.wav"), 1, ATTN_NORM, 0);
		break;

	case Q2MZ2_WIDOW_PLASMABEAM:
	case Q2MZ2_WIDOW2_BEAMER_1:
	case Q2MZ2_WIDOW2_BEAMER_2:
	case Q2MZ2_WIDOW2_BEAMER_3:
	case Q2MZ2_WIDOW2_BEAMER_4:
	case Q2MZ2_WIDOW2_BEAMER_5:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_1:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_2:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_3:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_4:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_5:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_6:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_7:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_8:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_9:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_10:
	case Q2MZ2_WIDOW2_BEAM_SWEEP_11:
		dl->radius = 300 + (rand()&100);
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		dl->die = cl.time + 200;
		break;
// ROGUE
// ======

// --- Xian's shit ends ---

  //hmm... he must take AGES on the loo.... :p
	}
	dl->color[0] /= 5;
	dl->color[1] /= 5;
	dl->color[2] /= 5;
}

/*
=========================================================================

FRAME PARSING

=========================================================================
*/

#if 0

typedef struct
{
	int		modelindex;
	int		num; // entity number
	int		effects;
	vec3_t	origin;
	vec3_t	oldorigin;
	vec3_t	angles;
	qboolean present;
} projectile_t;

#define	MAX_PROJECTILES	64
projectile_t	cl_projectiles[MAX_PROJECTILES];

void CLQ2_ClearProjectiles (void)
{
	int i;

	for (i = 0; i < MAX_PROJECTILES; i++) {
//		if (cl_projectiles[i].present)
//			Com_DPrintf("PROJ: %d CLEARED\n", cl_projectiles[i].num);
		cl_projectiles[i].present = false;
	}
}

/*
=====================
CL_ParseProjectiles

Flechettes are passed as efficient temporary entities
=====================
*/
void CLQ2_ParseProjectiles (void)
{
	int		i, c, j;
	byte	bits[8];
	byte	b;
	projectile_t	pr;
	int lastempty = -1;
	qboolean old = false;

	c = MSG_ReadByte (&net_message);
	for (i=0 ; i<c ; i++)
	{
		bits[0] = MSG_ReadByte (&net_message);
		bits[1] = MSG_ReadByte (&net_message);
		bits[2] = MSG_ReadByte (&net_message);
		bits[3] = MSG_ReadByte (&net_message);
		bits[4] = MSG_ReadByte (&net_message);
		pr.origin[0] = ( ( bits[0] + ((bits[1]&15)<<8) ) <<1) - 4096;
		pr.origin[1] = ( ( (bits[1]>>4) + (bits[2]<<4) ) <<1) - 4096;
		pr.origin[2] = ( ( bits[3] + ((bits[4]&15)<<8) ) <<1) - 4096;
		VectorCopy(pr.origin, pr.oldorigin);

		if (bits[4] & 64)
			pr.effects = EF_BLASTER;
		else
			pr.effects = 0;

		if (bits[4] & 128) {
			old = true;
			bits[0] = MSG_ReadByte (&net_message);
			bits[1] = MSG_ReadByte (&net_message);
			bits[2] = MSG_ReadByte (&net_message);
			bits[3] = MSG_ReadByte (&net_message);
			bits[4] = MSG_ReadByte (&net_message);
			pr.oldorigin[0] = ( ( bits[0] + ((bits[1]&15)<<8) ) <<1) - 4096;
			pr.oldorigin[1] = ( ( (bits[1]>>4) + (bits[2]<<4) ) <<1) - 4096;
			pr.oldorigin[2] = ( ( bits[3] + ((bits[4]&15)<<8) ) <<1) - 4096;
		}

		bits[0] = MSG_ReadByte (&net_message);
		bits[1] = MSG_ReadByte (&net_message);
		bits[2] = MSG_ReadByte (&net_message);

		pr.angles[0] = 360*bits[0]/256;
		pr.angles[1] = 360*bits[1]/256;
		pr.modelindex = bits[2];

		b = MSG_ReadByte (&net_message);
		pr.num = (b & 0x7f);
		if (b & 128) // extra entity number byte
			pr.num |= (MSG_ReadByte (&net_message) << 7);

		pr.present = true;

		// find if this projectile already exists from previous frame 
		for (j = 0; j < MAX_PROJECTILES; j++) {
			if (cl_projectiles[j].modelindex) {
				if (cl_projectiles[j].num == pr.num) {
					// already present, set up oldorigin for interpolation
					if (!old)
						VectorCopy(cl_projectiles[j].origin, pr.oldorigin);
					cl_projectiles[j] = pr;
					break;
				}
			} else
				lastempty = j;
		}

		// not present previous frame, add it
		if (j == MAX_PROJECTILES) {
			if (lastempty != -1) {
				cl_projectiles[lastempty] = pr;
			}
		}
	}
}

/*
=============
CL_LinkProjectiles

=============
*/
void CLQ2_AddProjectiles (void)
{
	int		i, j;
	projectile_t	*pr;
	entity_t		ent;

	memset (&ent, 0, sizeof(ent));

	for (i=0, pr=cl_projectiles ; i < MAX_PROJECTILES ; i++, pr++)
	{
		// grab an entity to fill in
		if (pr->modelindex < 1)
			continue;
		if (!pr->present) {
			pr->modelindex = 0;
			continue; // not present this frame (it was in the previous frame)
		}

		ent.model = cl.model_draw[pr->modelindex];

		// interpolate origin
		for (j=0 ; j<3 ; j++)
		{
			ent.origin[j] = ent.oldorigin[j] = pr->oldorigin[j] + cl.lerpfrac * 
				(pr->origin[j] - pr->oldorigin[j]);

		}

		if (pr->effects & EF_BLASTER)
		{
			if (P_ParticleTrail(pr->oldorigin, ent.origin, rt_blastertrail, NULL))
				P_ParticleTrailIndex(pr->oldorigin, ent.origin, 0xe0, 1, NULL);
		}
		V_AddLight (pr->origin, 200, 0.2, 0.2, 0);

		VectorCopy (pr->angles, ent.angles);
		V_AddLerpEntity (&ent);
	}
}
#endif

/*
=================
CL_ParseEntityBits

Returns the entity number and the header bits
=================
*/
int	bitcounts[32];	/// just for protocol profiling
int CLQ2_ParseEntityBits (unsigned *bits)
{
	unsigned	b, total;
	int			i;
	int			number;

	total = MSG_ReadByte ();
	if (total & Q2U_MOREBITS1)
	{
		b = MSG_ReadByte ();
		total |= b<<8;
	}
	if (total & Q2U_MOREBITS2)
	{
		b = MSG_ReadByte ();
		total |= b<<16;
	}
	if (total & Q2U_MOREBITS3)
	{
		b = MSG_ReadByte ();
		total |= b<<24;
	}

	// count the bits for net profiling
	for (i=0 ; i<32 ; i++)
		if (total&(1<<i))
			bitcounts[i]++;

	if (total & Q2U_NUMBER16)
		number = MSG_ReadShort ();
	else
		number = MSG_ReadByte ();

	*bits = total;

	return number;
}

/*
==================
CL_ParseDelta

Can go from either a baseline or a previous packet_entity
==================
*/
void CLQ2_ParseDelta (entity_state_t *from, entity_state_t *to, int number, int bits)
{
	// set everything to the state we are delta'ing from
	*to = *from;

	VectorCopy (from->origin, to->old_origin);
	to->number = number;

	if (bits & Q2U_MODEL)
		to->modelindex = MSG_ReadByte ();
	if (bits & Q2U_MODEL2)
		to->modelindex2 = MSG_ReadByte ();
	if (bits & Q2U_MODEL3)
		to->modelindex3 = MSG_ReadByte ();
	if (bits & Q2U_MODEL4)
		to->modelindex4 = MSG_ReadByte ();
		
	if (bits & Q2U_FRAME8)
		to->frame = MSG_ReadByte ();
	if (bits & Q2U_FRAME16)
		to->frame = MSG_ReadShort ();

	if ((bits & Q2U_SKIN8) && (bits & Q2U_SKIN16))		//used for laser colors
		to->skinnum = MSG_ReadLong();
	else if (bits & Q2U_SKIN8)
		to->skinnum = MSG_ReadByte();
	else if (bits & Q2U_SKIN16)
		to->skinnum = MSG_ReadShort();

	if ( (bits & (Q2U_EFFECTS8|Q2U_EFFECTS16)) == (Q2U_EFFECTS8|Q2U_EFFECTS16) )
		to->effects = MSG_ReadLong();
	else if (bits & Q2U_EFFECTS8)
		to->effects = MSG_ReadByte();
	else if (bits & Q2U_EFFECTS16)
		to->effects = MSG_ReadShort();

	if ( (bits & (Q2U_RENDERFX8|Q2U_RENDERFX16)) == (Q2U_RENDERFX8|Q2U_RENDERFX16) )
		to->renderfx = MSG_ReadLong();
	else if (bits & Q2U_RENDERFX8)
		to->renderfx = MSG_ReadByte();
	else if (bits & Q2U_RENDERFX16)
		to->renderfx = MSG_ReadShort();

	if (bits & Q2U_ORIGIN1)
		to->origin[0] = MSG_ReadCoord ();
	if (bits & Q2U_ORIGIN2)
		to->origin[1] = MSG_ReadCoord ();
	if (bits & Q2U_ORIGIN3)
		to->origin[2] = MSG_ReadCoord ();
		
	if (bits & Q2U_ANGLE1)
		to->angles[0] = MSG_ReadAngle();
	if (bits & Q2U_ANGLE2)
		to->angles[1] = MSG_ReadAngle();
	if (bits & Q2U_ANGLE3)
		to->angles[2] = MSG_ReadAngle();

	if (bits & Q2U_OLDORIGIN)
		MSG_ReadPos (to->old_origin);

	if (bits & Q2U_SOUND)
		to->sound = MSG_ReadByte ();

	if (bits & Q2U_EVENT)
		to->event = MSG_ReadByte ();
	else
		to->event = 0;

	if (bits & Q2U_SOLID)
		to->solid = MSG_ReadShort ();
}

/*
==================
CL_DeltaEntity

Parses deltas from the given base and adds the resulting entity
to the current frame
==================
*/
void CLQ2_DeltaEntity (q2frame_t *frame, int newnum, entity_state_t *old, int bits)
{
	q2centity_t	*ent;
	entity_state_t	*state;

	ent = &cl_entities[newnum];

	state = &cl_parse_entities[cl.parse_entities & (MAX_PARSE_ENTITIES-1)];
	cl.parse_entities++;
	frame->num_entities++;

	CLQ2_ParseDelta (old, state, newnum, bits);

	// some data changes will force no lerping
	if (state->modelindex != ent->current.modelindex
		|| state->modelindex2 != ent->current.modelindex2
		|| state->modelindex3 != ent->current.modelindex3
		|| state->modelindex4 != ent->current.modelindex4
		|| abs(state->origin[0] - ent->current.origin[0]) > 512
		|| abs(state->origin[1] - ent->current.origin[1]) > 512
		|| abs(state->origin[2] - ent->current.origin[2]) > 512
		|| state->event == Q2EV_PLAYER_TELEPORT
		|| state->event == Q2EV_OTHER_TELEPORT
		)
	{
		ent->serverframe = -99;
	}

	if (ent->serverframe != cl.q2frame.serverframe - 1)
	{	// wasn't in last update, so initialize some things
		// clear trailstate
		P_DelinkTrailstate(&ent->trailstate);

		// duplicate the current state so lerping doesn't hurt anything
		ent->prev = *state;
		if (state->event == Q2EV_OTHER_TELEPORT)
		{
			VectorCopy (state->origin, ent->prev.origin);
			VectorCopy (state->origin, ent->lerp_origin);
		}
		else
		{
			VectorCopy (state->old_origin, ent->prev.origin);
			VectorCopy (state->old_origin, ent->lerp_origin);
		}
	}
	else
	{	// shuffle the last state to previous
		ent->prev = ent->current;
	}

	ent->serverframe = cl.q2frame.serverframe;
	ent->current = *state;
}

/*
==================
CL_ParsePacketEntities

An svc_packetentities has just been parsed, deal with the
rest of the data stream.
==================
*/
void CLQ2_ParsePacketEntities (q2frame_t *oldframe, q2frame_t *newframe)
{
	int			newnum;
	int			bits;
	entity_state_t	*oldstate=NULL;
	int			oldindex, oldnum;

	cl.validsequence = cls.netchan.incoming_sequence;

	newframe->parse_entities = cl.parse_entities;
	newframe->num_entities = 0;

	// delta from the entities present in oldframe
	oldindex = 0;
	if (!oldframe)
		oldnum = 99999;
	else
	{
		if (oldindex >= oldframe->num_entities)
			oldnum = 99999;
		else
		{
			oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}

	while (1)
	{
		newnum = CLQ2_ParseEntityBits (&bits);
		if (newnum >= MAX_Q2EDICTS)
			Host_EndGame ("CL_ParsePacketEntities: bad number:%i", newnum);

		if (msg_readcount > net_message.cursize)
			Host_EndGame ("CL_ParsePacketEntities: end of message");

		if (!newnum)
			break;

		while (oldnum < newnum)
		{	// one or more entities from the old packet are unchanged
			if (cl_shownet.value == 3)
				Con_Printf ("   unchanged: %i\n", oldnum);
			CLQ2_DeltaEntity (newframe, oldnum, oldstate, 0);
			
			oldindex++;

			if (oldindex >= oldframe->num_entities)
				oldnum = 99999;
			else
			{
				oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
		}

		if (bits & Q2U_REMOVE)
		{	// the entity present in oldframe is not in the current frame
			if (cl_shownet.value == 3)
				Con_Printf ("   remove: %i\n", newnum);
			if (oldnum != newnum)
				Con_Printf ("U_REMOVE: oldnum != newnum\n");

			oldindex++;

			if (oldindex >= oldframe->num_entities)
				oldnum = 99999;
			else
			{
				oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum == newnum)
		{	// delta from previous state
			if (cl_shownet.value == 3)
				Con_Printf ("   delta: %i\n", newnum);
			CLQ2_DeltaEntity (newframe, newnum, oldstate, bits);

			oldindex++;

			if (oldindex >= oldframe->num_entities)
				oldnum = 99999;
			else
			{
				oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum > newnum)
		{	// delta from baseline
			if (cl_shownet.value == 3)
				Con_Printf ("   baseline: %i\n", newnum);
			CLQ2_DeltaEntity (newframe, newnum, &cl_entities[newnum].baseline, bits);
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while (oldnum != 99999)
	{	// one or more entities from the old packet are unchanged
		if (cl_shownet.value == 3)
			Con_Printf ("   unchanged: %i\n", oldnum);
		CLQ2_DeltaEntity (newframe, oldnum, oldstate, 0);
		
		oldindex++;

		if (oldindex >= oldframe->num_entities)
			oldnum = 99999;
		else
		{
			oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}
}

void CLQ2_ParseBaseline (void)
{
	entity_state_t	*es;
	int				bits;
	int				newnum;
	entity_state_t	nullstate;

	memset (&nullstate, 0, sizeof(nullstate));

	newnum = CLQ2_ParseEntityBits (&bits);
	es = &cl_entities[newnum].baseline;
	CLQ2_ParseDelta (&nullstate, es, newnum, bits);
}


/*
===================
CL_ParsePlayerstate
===================
*/
void CLQ2_ParsePlayerstate (q2frame_t *oldframe, q2frame_t *newframe)
{
	int			flags;
	q2player_state_t	*state;
	int			i;
	int			statbits;

	state = &newframe->playerstate;

	// clear to old value before delta parsing
	if (oldframe)
		*state = oldframe->playerstate;
	else
		memset (state, 0, sizeof(*state));

	flags = MSG_ReadShort ();

	//
	// parse the pmove_state_t
	//
	if (flags & Q2PS_M_TYPE)
		state->pmove.pm_type = MSG_ReadByte ();

	if (flags & Q2PS_M_ORIGIN)
	{
		state->pmove.origin[0] = MSG_ReadShort ();
		state->pmove.origin[1] = MSG_ReadShort ();
		state->pmove.origin[2] = MSG_ReadShort ();
	}

	if (flags & Q2PS_M_VELOCITY)
	{
		state->pmove.velocity[0] = MSG_ReadShort ();
		state->pmove.velocity[1] = MSG_ReadShort ();
		state->pmove.velocity[2] = MSG_ReadShort ();
	}

	if (flags & Q2PS_M_TIME)
		state->pmove.pm_time = MSG_ReadByte ();

	if (flags & Q2PS_M_FLAGS)
		state->pmove.pm_flags = MSG_ReadByte ();

	if (flags & Q2PS_M_GRAVITY)
		state->pmove.gravity = MSG_ReadShort ();

	if (flags & Q2PS_M_DELTA_ANGLES)
	{
		state->pmove.delta_angles[0] = MSG_ReadShort ();
		state->pmove.delta_angles[1] = MSG_ReadShort ();
		state->pmove.delta_angles[2] = MSG_ReadShort ();
	}

//	if (cl.attractloop)
//		state->pmove.pm_type = Q2PM_FREEZE;		// demo playback

	//
	// parse the rest of the player_state_t
	//
	if (flags & Q2PS_VIEWOFFSET)
	{
		state->viewoffset[0] = MSG_ReadChar () * 0.25;
		state->viewoffset[1] = MSG_ReadChar () * 0.25;
		state->viewoffset[2] = MSG_ReadChar () * 0.25;
	}

	if (flags & Q2PS_VIEWANGLES)
	{
		state->viewangles[0] = MSG_ReadAngle16 ();
		state->viewangles[1] = MSG_ReadAngle16 ();
		state->viewangles[2] = MSG_ReadAngle16 ();
	}

	if (flags & Q2PS_KICKANGLES)
	{
		state->kick_angles[0] = MSG_ReadChar () * 0.25;
		state->kick_angles[1] = MSG_ReadChar () * 0.25;
		state->kick_angles[2] = MSG_ReadChar () * 0.25;
	}

	if (flags & Q2PS_WEAPONINDEX)
	{
		state->gunindex = MSG_ReadByte ();
	}

	if (flags & Q2PS_WEAPONFRAME)
	{
		state->gunframe = MSG_ReadByte ();
		state->gunoffset[0] = MSG_ReadChar ()*0.25;
		state->gunoffset[1] = MSG_ReadChar ()*0.25;
		state->gunoffset[2] = MSG_ReadChar ()*0.25;
		state->gunangles[0] = MSG_ReadChar ()*0.25;
		state->gunangles[1] = MSG_ReadChar ()*0.25;
		state->gunangles[2] = MSG_ReadChar ()*0.25;
	}

	if (flags & Q2PS_BLEND)
	{
		state->blend[0] = MSG_ReadByte ()/255.0;
		state->blend[1] = MSG_ReadByte ()/255.0;
		state->blend[2] = MSG_ReadByte ()/255.0;
		state->blend[3] = MSG_ReadByte ()/255.0;
	}

	if (flags & Q2PS_FOV)
		state->fov = MSG_ReadByte ();

	if (flags & Q2PS_RDFLAGS)
		state->rdflags = MSG_ReadByte ();

	// parse stats
	statbits = MSG_ReadLong ();
	for (i=0 ; i<Q2MAX_STATS ; i++)
		if (statbits & (1<<i) )
			state->stats[i] = MSG_ReadShort();
}


/*
==================
CL_FireEntityEvents

==================
*/
void CLQ2_FireEntityEvents (q2frame_t *frame)
{
	entity_state_t		*s1;
	int					pnum, num;

	for (pnum = 0 ; pnum<frame->num_entities ; pnum++)
	{
		num = (frame->parse_entities + pnum)&(MAX_PARSE_ENTITIES-1);
		s1 = &cl_parse_entities[num];
		if (s1->event)
			CLQ2_EntityEvent (s1);

		// EF_TELEPORTER acts like an event, but is not cleared each frame
		if (s1->effects & Q2EF_TELEPORTER)
			CLQ2_TeleporterParticles (s1);
	}
}


/*
================
CL_ParseFrame
================
*/
void CLQ2_ParseFrame (void)
{
	int			cmd;
	int			len;
	q2frame_t		*old;

	memset (&cl.q2frame, 0, sizeof(cl.q2frame));

#if 0
	CLQ2_ClearProjectiles(); // clear projectiles for new frame
#endif

	cl.q2frame.serverframe = MSG_ReadLong ();
	cl.q2frame.deltaframe = MSG_ReadLong ();
	cl.q2frame.servertime = cl.q2frame.serverframe*100;

	cl.surpressCount = MSG_ReadByte ();

	if (cl_shownet.value == 3)
		Con_Printf ("   frame:%i  delta:%i\n", cl.q2frame.serverframe,
		cl.q2frame.deltaframe);

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message 
	if (cl.q2frame.deltaframe <= 0)
	{
		cl.q2frame.valid = true;		// uncompressed frame
		old = NULL;
//		cls.demowaiting = false;	// we can start recording now
	}
	else
	{
		old = &cl.q2frames[cl.q2frame.deltaframe & Q2UPDATE_MASK];
		if (!old->valid)
		{	// should never happen
			Con_Printf ("Delta from invalid frame (not supposed to happen!).\n");
		}
		if (old->serverframe != cl.q2frame.deltaframe)
		{	// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			Con_Printf ("Delta frame too old.\n");
		}
		else if (cl.parse_entities - old->parse_entities > MAX_PARSE_ENTITIES-128)
		{
			Con_Printf ("Delta parse_entities too old.\n");
		}
		else
			cl.q2frame.valid = true;	// valid delta parse
	}

	// clamp time 
	if (cl.time > cl.q2frame.servertime/1000.0)
		cl.time = cl.q2frame.servertime/1000.0;
	else if (cl.time < (cl.q2frame.servertime - 100)/1000.0)
		cl.time = (cl.q2frame.servertime - 100)/1000.0;

	// read areabits
	len = MSG_ReadByte ();
	MSG_ReadData (&cl.q2frame.areabits, len);

	// read playerinfo
	cmd = MSG_ReadByte ();
//	SHOWNET(svc_strings[cmd]);
	if (cmd != svcq2_playerinfo)
		Host_EndGame ("CL_ParseFrame: not playerinfo");
	CLQ2_ParsePlayerstate (old, &cl.q2frame);

	// read packet entities
	cmd = MSG_ReadByte ();
//	SHOWNET(svc_strings[cmd]);
	if (cmd != svcq2_packetentities)
		Host_EndGame ("CL_ParseFrame: not packetentities");
	CLQ2_ParsePacketEntities (old, &cl.q2frame);

	// save the frame off in the backup array for later delta comparisons
	cl.q2frames[cl.q2frame.serverframe & Q2UPDATE_MASK] = cl.q2frame;

	if (cl.q2frame.valid)
	{
		// getting a valid frame message ends the connection process
		if (cls.state != ca_active)
		{
			CL_MakeActive("Quake2");

//			cl.force_refdef = true;
			cl.predicted_origin[0] = cl.q2frame.playerstate.pmove.origin[0]*0.125;
			cl.predicted_origin[1] = cl.q2frame.playerstate.pmove.origin[1]*0.125;
			cl.predicted_origin[2] = cl.q2frame.playerstate.pmove.origin[2]*0.125;
			VectorCopy (cl.q2frame.playerstate.viewangles, cl.predicted_angles);
//			if (cls.disable_servercount != cl.servercount
//				&& cl.refresh_prepped)
				SCR_EndLoadingPlaque ();	// get rid of loading plaque
		}
//		cl.sound_prepped = true;	// can start mixing ambient sounds
	
		// fire entity events
		CLQ2_FireEntityEvents (&cl.q2frame);
#ifdef Q2BSPS
		CLQ2_CheckPredictionError ();
#endif
	}
}

/*
==========================================================================

INTERPOLATE BETWEEN FRAMES TO GET RENDERING PARMS

==========================================================================
*/
/*
struct model_s *S_RegisterSexedModel (entity_state_t *ent, char *base)
{
	int				n;
	char			*p;
	struct model_s	*mdl;
	char			model[MAX_QPATH];
	char			buffer[MAX_QPATH];

	// determine what model the client is using
	model[0] = 0;
	n = CS_PLAYERSKINS + ent->number - 1;
	if (cl.configstrings[n][0])
	{
		p = strchr(cl.configstrings[n], '\\');
		if (p)
		{
			p += 1;
			strcpy(model, p);
			p = strchr(model, '/');
			if (p)
				*p = 0;
		}
	}
	// if we can't figure it out, they're male
	if (!model[0])
		strcpy(model, "male");

	Com_sprintf (buffer, sizeof(buffer), "players/%s/%s", model, base+1);
	mdl = re.RegisterModel(buffer);
	if (!mdl) {
		// not found, try default weapon model
		Com_sprintf (buffer, sizeof(buffer), "players/%s/weapon.md2", model);
		mdl = re.RegisterModel(buffer);
		if (!mdl) {
			// no, revert to the male model
			Com_sprintf (buffer, sizeof(buffer), "players/%s/%s", "male", base+1);
			mdl = re.RegisterModel(buffer);
			if (!mdl) {
				// last try, default male weapon.md2
				Com_sprintf (buffer, sizeof(buffer), "players/male/weapon.md2");
				mdl = re.RegisterModel(buffer);
			}
		} 
	}

	return mdl;
}

*/

void V_AddLight (vec3_t org, float quant, float r, float g, float b)
{
	CL_NewDlightRGB (0, org[0], org[1], org[2], quant, -0.1, r, g, b);
}
/*
===============
CL_AddPacketEntities

===============
*/
void CLQ2_AddPacketEntities (q2frame_t *frame)
{
	entity_t			ent;
	entity_state_t		*s1;
	float				autorotate;
	int					i;
	int					pnum;
	q2centity_t			*cent;
	int					autoanim;
//	q2clientinfo_t		*ci;
	player_info_t		*player;
	unsigned int		effects, renderfx;

	// bonus items rotate at a fixed rate
	autorotate = anglemod(cl.time*100);

	// brush models can auto animate their frames
	autoanim = 2*cl.time;

	memset (&ent, 0, sizeof(ent));

	for (pnum = 0 ; pnum<frame->num_entities ; pnum++)
	{
		s1 = &cl_parse_entities[(frame->parse_entities+pnum)&(MAX_PARSE_ENTITIES-1)];

		cent = &cl_entities[s1->number];

		effects = s1->effects;
		renderfx = s1->renderfx;

		ent.keynum = s1->number;

		ent.scale = 1;
		ent.shaderRGBAf[0] = 1;
		ent.shaderRGBAf[1] = 1;
		ent.shaderRGBAf[2] = 1;
		ent.shaderRGBAf[3] = 1;
		ent.fatness = 0;
		ent.scoreboard = NULL;

			// set frame
		if (effects & Q2EF_ANIM01)
			ent.frame = autoanim & 1;
		else if (effects & Q2EF_ANIM23)
			ent.frame = 2 + (autoanim & 1);
		else if (effects & Q2EF_ANIM_ALL)
			ent.frame = autoanim;
		else if (effects & Q2EF_ANIM_ALLFAST)
			ent.frame = cl.time / 100;
		else
			ent.frame = s1->frame;

		// quad and pent can do different things on client
		if (effects & Q2EF_PENT)
		{
			effects &= ~Q2EF_PENT;
			effects |= Q2EF_COLOR_SHELL;
			renderfx |= Q2RF_SHELL_RED;
		}

		if (effects & Q2EF_QUAD)
		{
			effects &= ~Q2EF_QUAD;
			effects |= Q2EF_COLOR_SHELL;
			renderfx |= Q2RF_SHELL_BLUE;
		}
//======
// PMM
		if (effects & Q2EF_DOUBLE)
		{
			effects &= ~Q2EF_DOUBLE;
			effects |= Q2EF_COLOR_SHELL;
			renderfx |= Q2RF_SHELL_DOUBLE;
		}

		if (effects & Q2EF_HALF_DAMAGE)
		{
			effects &= ~Q2EF_HALF_DAMAGE;
			effects |= Q2EF_COLOR_SHELL;
			renderfx |= Q2RF_SHELL_HALF_DAM;
		}
// pmm
//======
		ent.oldframe = cent->prev.frame;
		ent.lerpfrac = cl.lerpfrac;

		if (renderfx & (Q2RF_FRAMELERP|Q2RF_BEAM))
		{	// step origin discretely, because the frames
			// do the animation properly
			VectorCopy (cent->current.origin, ent.origin);
			VectorCopy (cent->current.old_origin, ent.oldorigin);
		}
		else
		{	// interpolate origin
			for (i=0 ; i<3 ; i++)
			{
				ent.origin[i] = ent.oldorigin[i] = cent->prev.origin[i] + cl.lerpfrac * 
					(cent->current.origin[i] - cent->prev.origin[i]);
			}
		}

		// create a new entity
	
		// tweak the color of beams
		if ( renderfx & Q2RF_BEAM )
		{	// the four beam colors are encoded in 32 bits of skinnum (hack)
			ent.shaderRGBAf[3] = 0.30;
			ent.skinnum = (s1->skinnum >> ((rand() % 4)*8)) & 0xff;
			ent.model = NULL;
			ent.lerpfrac = 1;
		}
		else
		{
			// set skin
			if (s1->modelindex == 255)
			{	// use custom player skin
				ent.skinnum = 0;
				ent.model = NULL;


				player = &cl.players[s1->skinnum%MAX_CLIENTS];
				ent.model = player->model;
				if (!ent.model || ent.model->needload)	//we need to do better than this
				{
					ent.model = Mod_ForName(va("players/%s/tris.md2", Info_ValueForKey(player->userinfo, "model")), false);
					if (!ent.model || ent.model->needload)
						ent.model = Mod_ForName("players/male/tris.md2", false);
				}
				ent.scoreboard = player;
/*				ci = &cl.clientinfo[s1->skinnum & 0xff];
//				ent.skin = ci->skin;
				ent.model = ci->model;
				if (!ent.skin || !ent.model)
				{
					ent.skin = cl.baseclientinfo.skin;
					ent.model = cl.baseclientinfo.model;
				}

//============
//PGM
				if (renderfx & Q2RF_USE_DISGUISE)
				{
					if(!strncmp((char *)ent.skin, "players/male", 12))
					{
						ent.skin = re.RegisterSkin ("players/male/disguise.pcx");
						ent.model = re.RegisterModel ("players/male/tris.md2");
					}
					else if(!strncmp((char *)ent.skin, "players/female", 14))
					{
						ent.skin = re.RegisterSkin ("players/female/disguise.pcx");
						ent.model = re.RegisterModel ("players/female/tris.md2");
					}
					else if(!strncmp((char *)ent.skin, "players/cyborg", 14))
					{
						ent.skin = re.RegisterSkin ("players/cyborg/disguise.pcx");
						ent.model = re.RegisterModel ("players/cyborg/tris.md2");
					}
				}*/
//PGM
//============
			}
			else
			{
				ent.skinnum = s1->skinnum;
//				ent.skin = NULL;
				ent.model = cl.model_precache[s1->modelindex];
			}
		}

		// only used for black hole model right now, FIXME: do better
		if (renderfx == Q2RF_TRANSLUCENT)
			ent.shaderRGBAf[3] = 0.70;

		// render effects (fullbright, translucent, etc)
		if ((effects & Q2EF_COLOR_SHELL))
			ent.flags = 0;	// renderfx go on color shell entity
		else
			ent.flags = renderfx;

		// calculate angles
		if (effects & Q2EF_ROTATE)
		{	// some bonus items auto-rotate
			ent.angles[0] = 0;
			ent.angles[1] = autorotate;
			ent.angles[2] = 0;
		}
		// RAFAEL
		else if (effects & Q2EF_SPINNINGLIGHTS)
		{
			ent.angles[0] = 0;
			ent.angles[1] = anglemod(cl.time/2) + s1->angles[1];
			ent.angles[2] = 180;
			{
				vec3_t forward;
				vec3_t start;

				AngleVectors (ent.angles, forward, NULL, NULL);
				VectorMA (ent.origin, 64, forward, start);
				V_AddLight (start, 100, 0.2, 0, 0);
			}
		}
		else
		{	// interpolate angles
			float	a1, a2;

			for (i=0 ; i<3 ; i++)
			{
				a1 = cent->current.angles[i];
				a2 = cent->prev.angles[i];
				ent.angles[i] = LerpAngle (a2, a1, cl.lerpfrac);
			}
		}

 	ent.angles[0]*=-1;	//q2 has it fixed.

		if (s1->number == cl.playernum[0]+1)	//woo! this is us!
		{
			ent.flags |= Q2RF_EXTERNALMODEL;	// only draw from mirrors

			if (effects & Q2EF_FLAG1)
				V_AddLight (ent.origin, 225, 0.2, 0.05, 0.05);
			else if (effects & Q2EF_FLAG2)
				V_AddLight (ent.origin, 225, 0.05, 0.05, 0.2);
			else if (effects & Q2EF_TAGTRAIL)						//PGM
				V_AddLight (ent.origin, 225, 0.2, 0.2, 0.0);	//PGM
			else if (effects & Q2EF_TRACKERTRAIL)					//PGM
				V_AddLight (ent.origin, 225, -0.2, -0.2, -0.2);	//PGM
		}

		// if set to invisible, skip
		if (!s1->modelindex)
			continue;

		if (effects & Q2EF_BFG)
		{
			ent.flags |= Q2RF_TRANSLUCENT;
			ent.shaderRGBAf[3] = 0.30;
		}

		// RAFAEL
		if (effects & Q2EF_PLASMA)
		{
			ent.flags |= Q2RF_TRANSLUCENT;
			ent.shaderRGBAf[3] = 0.6;
		}

		if (effects & Q2EF_SPHERETRANS)
		{
			ent.flags |= Q2RF_TRANSLUCENT;
			// PMM - *sigh*  yet more EF overloading
			if (effects & Q2EF_TRACKERTRAIL)
				ent.shaderRGBAf[3] = 0.6;
			else
				ent.shaderRGBAf[3] = 0.3;
		}
//pmm

		// add to refresh list
		V_AddLerpEntity (&ent);


		// color shells generate a seperate entity for the main model
		if (effects & Q2EF_COLOR_SHELL)
		{
			// PMM - at this point, all of the shells have been handled
			// if we're in the rogue pack, set up the custom mixing, otherwise just
			// keep going

			// all of the solo colors are fine.  we need to catch any of the combinations that look bad
			// (double & half) and turn them into the appropriate color, and make double/quad something special
			if (renderfx & Q2RF_SHELL_HALF_DAM)
			{
				
				{
					// ditch the half damage shell if any of red, blue, or double are on
					if (renderfx & (Q2RF_SHELL_RED|Q2RF_SHELL_BLUE|Q2RF_SHELL_DOUBLE))
						renderfx &= ~Q2RF_SHELL_HALF_DAM;
				}
			}

			if (renderfx & Q2RF_SHELL_DOUBLE)
			{

				{
					// lose the yellow shell if we have a red, blue, or green shell
					if (renderfx & (Q2RF_SHELL_RED|Q2RF_SHELL_BLUE|Q2RF_SHELL_GREEN))
						renderfx &= ~Q2RF_SHELL_DOUBLE;
					// if we have a red shell, turn it to purple by adding blue
					if (renderfx & Q2RF_SHELL_RED)
						renderfx |= Q2RF_SHELL_BLUE;
					// if we have a blue shell (and not a red shell), turn it to cyan by adding green
					else if (renderfx & Q2RF_SHELL_BLUE)
						// go to green if it's on already, otherwise do cyan (flash green)
						if (renderfx & Q2RF_SHELL_GREEN)
							renderfx &= ~Q2RF_SHELL_BLUE;
						else
							renderfx |= Q2RF_SHELL_GREEN;
				}
			}
			// pmm
			ent.flags = renderfx | Q2RF_TRANSLUCENT;
			ent.shaderRGBAf[3] = 0.30;
			ent.fatness = 1;
			ent.shaderRGBAf[0] = (!!(renderfx & Q2RF_SHELL_RED));
			ent.shaderRGBAf[1] = (!!(renderfx & Q2RF_SHELL_GREEN));
			ent.shaderRGBAf[2] = (!!(renderfx & Q2RF_SHELL_BLUE));
#ifdef Q3SHADERS	//fixme: do better.
			//fixme: this is woefully gl specific. :(
			if (qrenderer == QR_OPENGL)
			{
				ent.forcedshader = R_RegisterCustom("q2/shell", Shader_DefaultSkinShell);
			}
#endif
			V_AddLerpEntity (&ent);
		}
#ifdef Q3SHADERS
		ent.forcedshader = NULL;
#endif

//		ent.skin = NULL;		// never use a custom skin on others
		ent.skinnum = 0;
		ent.flags = 0;
		ent.shaderRGBAf[3] = 1;

		// duplicate for linked models
		if (s1->modelindex2)
		{
			if (s1->modelindex2 == 255)
			{	// custom weapon
				ent.model=NULL;
				/*
				ci = &cl.clientinfo[s1->skinnum & 0xff];
				i = (s1->skinnum >> 8); // 0 is default weapon model
				if (!cl_vwep->value || i > MAX_CLIENTWEAPONMODELS - 1)
					i = 0;
				ent.model = ci->weaponmodel[i];
				if (!ent.model) {
					if (i != 0)
						ent.model = ci->weaponmodel[0];
					if (!ent.model)
						ent.model = cl.baseclientinfo.weaponmodel[0];
				}
				*/
			}
			else
				ent.model = cl.model_precache[s1->modelindex2];

			// PMM - check for the defender sphere shell .. make it translucent
			// replaces the previous version which used the high bit on modelindex2 to determine transparency
/*			if (!Q_strcasecmp (cl.model_name[(s1->modelindex2)], "models/items/shell/tris.md2"))
			{
				ent.alpha = 0.32;
				ent.flags = Q2RF_TRANSLUCENT;
			}
*/			// pmm

			V_AddLerpEntity (&ent);

			//PGM - make sure these get reset.
			ent.flags = 0;
			ent.shaderRGBAf[3] = 1;
			//PGM
		}
		if (s1->modelindex3)
		{
			ent.model = cl.model_precache[s1->modelindex3];
			V_AddLerpEntity (&ent);
		}
		if (s1->modelindex4)
		{
			ent.model = cl.model_precache[s1->modelindex4];
			V_AddLerpEntity (&ent);
		}

		if ( effects & Q2EF_POWERSCREEN )
		{
/*			ent.model = cl_mod_powerscreen;
			ent.oldframe = 0;
			ent.frame = 0;
			ent.flags |= (Q2RF_TRANSLUCENT | Q2RF_SHELL_GREEN);
			ent.alpha = 0.30;
			V_AddLerpEntity (&ent);
*/		}

		// add automatic particle trails
		if ( (effects&~Q2EF_ROTATE) )
		{
			if (effects & Q2EF_ROCKET)
			{
				if (P_ParticleTrail(cent->lerp_origin, ent.origin, rt_rocket, &cent->trailstate))
						P_ParticleTrailIndex(cent->lerp_origin, ent.origin, 0xdc, 4, &cent->trailstate);

				V_AddLight (ent.origin, 200, 0.2, 0.2, 0);
			}
			// PGM - Do not reorder EF_BLASTER and EF_HYPERBLASTER. 
			// EF_BLASTER | EF_TRACKER is a special case for EF_BLASTER2... Cheese!
			else if (effects & Q2EF_BLASTER)
			{
//PGM
				if (effects & Q2EF_TRACKER)	// lame... problematic?
				{
					CLQ2_BlasterTrail2 (cent->lerp_origin, ent.origin);
					V_AddLight (ent.origin, 200, 0, 0.2, 0);		
				}
				else
				{
					if (P_ParticleTrail(cent->lerp_origin, ent.origin, rt_blastertrail, &cent->trailstate))
						P_ParticleTrailIndex(cent->lerp_origin, ent.origin, 0xe0, 1, &cent->trailstate);
					V_AddLight (ent.origin, 200, 0.2, 0.2, 0);
				}
//PGM
			}
			else if (effects & Q2EF_HYPERBLASTER)
			{
				if (effects & Q2EF_TRACKER)						// PGM	overloaded for blaster2.
					V_AddLight (ent.origin, 200, 0, 0.2, 0);		// PGM
				else											// PGM
					V_AddLight (ent.origin, 200, 0.2, 0.2, 0);
			}
			else if (effects & Q2EF_GIB)
			{
				if (P_ParticleTrail(cent->lerp_origin, ent.origin, rt_gib, &cent->trailstate))
					P_ParticleTrailIndex(cent->lerp_origin, ent.origin, 0xe8, 8, &cent->trailstate);
			}
			else if (effects & Q2EF_GRENADE)
			{
				if (P_ParticleTrail(cent->lerp_origin, ent.origin, rt_grenade, &cent->trailstate))
					P_ParticleTrailIndex(cent->lerp_origin, ent.origin, 4, 8, &cent->trailstate);
			}
			else if (effects & Q2EF_FLIES)
			{
				CLQ2_FlyEffect (cent, ent.origin);
			}
			else if (effects & Q2EF_BFG)
			{
				static int bfg_lightramp[6] = {300, 400, 600, 300, 150, 75};

				if (effects & Q2EF_ANIM_ALLFAST)
				{
					CLQ2_BfgParticles (&ent);
					i = 200;
				}
				else
				{
					i = bfg_lightramp[s1->frame];
				}
				V_AddLight (ent.origin, i, 0, 0.2, 0);
			}
			// RAFAEL
			else if (effects & Q2EF_TRAP)
			{
				ent.origin[2] += 32;
				CLQ2_TrapParticles (&ent);
				i = (rand()%100) + 100;
				V_AddLight (ent.origin, i, 0.2, 0.16, 0.05);
			}
			else if (effects & Q2EF_FLAG1)
			{
				if (P_ParticleTrail(cent->lerp_origin, ent.origin, P_FindParticleType("ef_flag1"), &cent->trailstate))
					P_ParticleTrailIndex(cent->lerp_origin, ent.origin, 242, 1, &cent->trailstate);
				V_AddLight (ent.origin, 225, 0.2, 0.05, 0.05);
			}
			else if (effects & Q2EF_FLAG2)
			{
				if (P_ParticleTrail(cent->lerp_origin, ent.origin, P_FindParticleType("ef_flag2"), &cent->trailstate))
					P_ParticleTrailIndex(cent->lerp_origin, ent.origin, 115, 1, &cent->trailstate);
				V_AddLight (ent.origin, 225, 0.05, 0.05, 0.2);
			}
//======
//ROGUE
			else if (effects & Q2EF_TAGTRAIL)
			{
				if (P_ParticleTrail(cent->lerp_origin, ent.origin, P_FindParticleType("ef_tagtrail"), &cent->trailstate))
					P_ParticleTrailIndex(cent->lerp_origin, ent.origin, 220, 1, &cent->trailstate);
				V_AddLight (ent.origin, 225, 0.2, 0.2, 0.0);
			}
			else if (effects & Q2EF_TRACKERTRAIL)
			{
				if (effects & Q2EF_TRACKER)
				{
					float intensity;

					intensity = 50 + (500 * (sin(cl.time/500.0) + 1.0));

					// FIXME - check out this effect in rendition
					V_AddLight (ent.origin, intensity, -0.2, -0.2, -0.2);
				}
				else
				{
					CLQ2_Tracker_Shell (cent->lerp_origin);
					V_AddLight (ent.origin, 155, -0.2, -0.2, -0.2);
				}
			}
			else if (effects & Q2EF_TRACKER)
			{
				if (P_ParticleTrail(cent->lerp_origin, ent.origin, P_FindParticleType("ef_tracker"), &cent->trailstate))
					P_ParticleTrailIndex(cent->lerp_origin, ent.origin, 0, 1, &cent->trailstate);
				V_AddLight (ent.origin, 200, -0.2, -0.2, -0.2);
			}
//ROGUE
//======
			// RAFAEL
			else if (effects & Q2EF_GREENGIB)
			{
				if (P_ParticleTrail(cent->lerp_origin, ent.origin, P_FindParticleType("ef_greengib"), &cent->trailstate))
					P_ParticleTrailIndex(cent->lerp_origin, ent.origin, 219, 8, &cent->trailstate);
			}
			// RAFAEL
			else if (effects & Q2EF_IONRIPPER)
			{
				if (P_ParticleTrail(cent->lerp_origin, ent.origin, P_FindParticleType("ef_ionripper"), &cent->trailstate))
					P_ParticleTrailIndex(cent->lerp_origin, ent.origin, 228, 4, &cent->trailstate);
				V_AddLight (ent.origin, 100, 0.2, 0.1, 0.1);
			}
			// RAFAEL
			else if (effects & Q2EF_BLUEHYPERBLASTER)
			{
				V_AddLight (ent.origin, 200, 0, 0, 0.2);
			}
			// RAFAEL
			else if (effects & Q2EF_PLASMA)
			{
				if (effects & Q2EF_ANIM_ALLFAST)
				{
					P_ParticleTrail(cent->lerp_origin, ent.origin, rt_blastertrail, &cent->trailstate);
				}
				V_AddLight (ent.origin, 130, 0.2, 0.1, 0.1);
			}
		}

		VectorCopy (ent.origin, cent->lerp_origin);
	}
}



/*
==============
CL_AddViewWeapon
==============
*/
void CLQ2_AddViewWeapon (q2player_state_t *ps, q2player_state_t *ops)
{
	entity_t	gun;		// view model
	int			i;
	entity_t	*view;
	extern cvar_t r_viewmodelsize;

	// allow the gun to be completely removed
	if (!r_drawviewmodel.value)
		return;

	// don't draw gun if in wide angle view
	if (ps->fov > 90)
		return;

	//generate root matrix..
	view = &cl.viewent[0];
	VectorCopy(cl.simorg[0], view->origin);
	AngleVectors(cl.simangles[0], view->axis[0], view->axis[1], view->axis[2]);
	VectorInverse(view->axis[1]);

	memset (&gun, 0, sizeof(gun));

//	if (gun_model)
//		gun.model = gun_model;	// development tool
//	else
		gun.model = cl.model_precache[ps->gunindex];
	if (!gun.model)
		return;

	gun.scale = r_viewmodelsize.value;
	gun.shaderRGBAf[0] = 1;
	gun.shaderRGBAf[1] = 1;
	gun.shaderRGBAf[2] = 1;
	if (r_drawviewmodel.value < 1 || r_drawviewmodel.value > 0)
		gun.shaderRGBAf[3] = r_drawviewmodel.value;
	else
		gun.shaderRGBAf[3] = 1;

	// set up gun position
	for (i=0 ; i<3 ; i++)
	{
		gun.origin[i] = 0;//ops->gunoffset[i]
		//	+ cl.lerpfrac * (ps->gunoffset[i] - ops->gunoffset[i]);
		gun.angles[i] = 0;//LerpAngle (ops->gunangles[i],
		//	ps->gunangles[i], cl.lerpfrac);
	}
	gun.angles[0]*=-1;

//	if (gun_frame)
//	{
//		gun.frame = gun_frame;	// development tool
//		gun.oldframe = gun_frame;	// development tool
//	}
//	else
	{
		gun.frame = ps->gunframe;
		if (gun.frame == 0)
			gun.oldframe = 0;	// just changed weapons, don't lerp from old
		else
			gun.oldframe = ops->gunframe;
	}

	gun.flags = Q2RF_MINLIGHT | Q2RF_DEPTHHACK | Q2RF_WEAPONMODEL;
	gun.lerpfrac = 1-cl.lerpfrac;
	VectorCopy (gun.origin, gun.oldorigin);	// don't lerp at all
	V_AddEntity (&gun);
}


/*
===============
CL_CalcViewValues

Sets r_refdef view values
===============
*/
void CLQ2_CalcViewValues (void)
{
	extern cvar_t v_gunkick;
	int			i;
	float		lerp, backlerp;
	q2centity_t	*ent;
	q2frame_t		*oldframe;
	q2player_state_t	*ps, *ops;

	r_refdef.useperspective = true;

	r_refdef.currentplayernum = 0;

	// find the previous frame to interpolate from
	ps = &cl.q2frame.playerstate;
	i = (cl.q2frame.serverframe - 1) & Q2UPDATE_MASK;
	oldframe = &cl.q2frames[i];
	if (oldframe->serverframe != cl.q2frame.serverframe-1 || !oldframe->valid)
		oldframe = &cl.q2frame;		// previous frame was dropped or involid
	ops = &oldframe->playerstate;

	// see if the player entity was teleported this frame
	if ( fabs(ops->pmove.origin[0] - ps->pmove.origin[0]) > 256*8
		|| abs(ops->pmove.origin[1] - ps->pmove.origin[1]) > 256*8
		|| abs(ops->pmove.origin[2] - ps->pmove.origin[2]) > 256*8)
		ops = ps;		// don't interpolate

	ent = &cl_entities[cl.playernum[0]+1];
	lerp = cl.lerpfrac;

	// calculate the origin
	if (cl.worldmodel && (!cl_nopred.value) && !(cl.q2frame.playerstate.pmove.pm_flags & Q2PMF_NO_PREDICTION))
	{	// use predicted values
		float	delta;

		backlerp = 1.0 - lerp;
		for (i=0 ; i<3 ; i++)
		{
			r_refdef.vieworg[i] = cl.predicted_origin[i] + ops->viewoffset[i] 
				+ cl.lerpfrac * (ps->viewoffset[i] - ops->viewoffset[i])
				- backlerp * cl.prediction_error[i];
		}

		// smooth out stair climbing
		delta = realtime - cl.predicted_step_time;
		if (delta < 0.1)
			r_refdef.vieworg[2] -= cl.predicted_step * (0.1 - delta)*10;
	}
	else
	{	// just use interpolated values
		for (i=0 ; i<3 ; i++)
			r_refdef.vieworg
			[i] = ops->pmove.origin[i]*0.125 + ops->viewoffset[i] 
				+ lerp * (ps->pmove.origin[i]*0.125 + ps->viewoffset[i] 
				- (ops->pmove.origin[i]*0.125 + ops->viewoffset[i]) );
	}

	// if not running a demo or on a locked frame, add the local angle movement
	if (cl.worldmodel && cl.q2frame.playerstate.pmove.pm_type < Q2PM_DEAD )
	{	// use predicted values
		for (i=0 ; i<3 ; i++)
			r_refdef.viewangles[i] = cl.predicted_angles[i];
	}
	else
	{	// just use interpolated values
		for (i=0 ; i<3 ; i++)
			r_refdef.viewangles[i] = LerpAngle (ops->viewangles[i], ps->viewangles[i], lerp);
	}

	for (i=0 ; i<3 ; i++)
		r_refdef.viewangles[i] += v_gunkick.value * LerpAngle (ops->kick_angles[i], ps->kick_angles[i], lerp);

	VectorCopy(r_refdef.vieworg, cl.simorg[0]);
	VectorCopy(r_refdef.viewangles, cl.simangles[0]);
//	VectorCopy(r_refdef.viewangles, cl.viewangles);

//	AngleVectors (r_refdef.viewangles, v_forward, v_right, v_up);

	// interpolate field of view
	r_refdef.fov_x = ops->fov + lerp * (ps->fov - ops->fov);

	// don't interpolate blend color
	for (i=0 ; i<4 ; i++)
		v_blend[i] = ps->blend[i];

	// add the weapon
	CLQ2_AddViewWeapon (ps, ops);
}

/*
===============
CL_AddEntities

Emits all entities, particles, and lights to the refresh
===============
*/
void CLQ2_AddEntities (void)
{
	if (cls.state != ca_active)
		return;


	r_refdef.currentplayernum = 0;


	cl_visedicts = cl_visedicts_list[cls.netchan.incoming_sequence&1];

	cl_numvisedicts = 0;

	if (cl.time*1000 > cl.q2frame.servertime)
	{
//		if (cl_showclamp.value)
//			Con_Printf ("high clamp %f\n", cl.time - cl.q2frame.servertime);
		cl.time = (cl.q2frame.servertime)/1000.0;
		cl.lerpfrac = 1.0;
	}
	else if (cl.time*1000 < cl.q2frame.servertime - 100)
	{
//		if (cl_showclamp.value)
//			Con_Printf ("low clamp %f\n", cl.q2frame.servertime-100 - cl.time);
		cl.time = (cl.q2frame.servertime - 100)/1000.0;
		cl.lerpfrac = 0;
	}
	else
		cl.lerpfrac = 1.0 - (cl.q2frame.servertime - cl.time*1000) * 0.01;

//	if (cl_timedemo.value)
//		cl.lerpfrac = 1.0;

//	CLQ2_AddPacketEntities (&cl.qwframe);
//	CLQ2_AddTEnts ();
//	CLQ2_AddParticles ();
//	CLQ2_AddDLights ();
//	CLQ2_AddLightStyles ();

	CLQ2_CalcViewValues ();
	// PMM - moved this here so the heat beam has the right values for the vieworg, and can lock the beam to the gun
	CLQ2_AddPacketEntities (&cl.q2frame);
#if 0
	CLQ2_AddProjectiles ();
#endif
	CL_UpdateTEnts ();
//	CLQ2_AddParticles ();
//	CLQ2_AddDLights ();
//	CLQ2_AddLightStyles ();
}

void CL_GetNumberedEntityInfo (int num, float *org, float *ang)
{
	q2centity_t	*ent;

	if (num < 0 || num >= MAX_EDICTS)
		Host_EndGame ("CL_GetNumberedEntityInfo: bad ent");
	ent = &cl_entities[num];

	if (org)
		VectorCopy (ent->current.origin, org);
	if (ang)
		VectorCopy (ent->current.angles, ang);


	// FIXME: bmodel issues...
}
#endif
