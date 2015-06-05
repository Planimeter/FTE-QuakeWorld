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
#include "quakedef.h"
#include "winquake.h"

cvar_t	cl_predict_extrapolate = CVARD("cl_predict_extrapolate", "", "If 1, enables prediction based upon partial input frames which can change over time resulting in a swimmy feel but does not need to interpolate. If 0, prediction will stay in the past and thus use only completed frames. Interpolation will then be used to smooth movement.\nThis cvar only applies when video and input frames are independant (ie: cl_netfps is set).");
cvar_t	cl_predict_timenudge = CVARD("cl_predict_timenudge", "0", "A debug feature. You should normally leave this as 0. Nudges local player prediction into the future if positive (resulting in extrapolation), or into the past if negative (resulting in laggy interpolation). Value is in seconds, so small decimals are required. This cvar applies even if input frames are tied to video frames.");
cvar_t	cl_predict_smooth = CVARD("cl_lerp_smooth", "2", "If 2, will act as 1 when playing demos and otherwise act as if set to 0.\nIf 1, interpolation will run in the past, resulting in really smooth movement at the cost of latency (even on bunchy german ISDNs).\nIf 0, interpolation will be based upon packet arrival times and may judder due to packet loss.");
cvar_t	cl_nopred = SCVAR("cl_nopred","0");
cvar_t	cl_pushlatency = SCVAR("pushlatency","-999");

extern float	pm_airaccelerate;

extern usercmd_t independantphysics[MAX_SPLITS];

#ifdef Q2CLIENT
#define	MAX_PARSE_ENTITIES	1024
extern entity_state_t	clq2_parse_entities[MAX_PARSE_ENTITIES];

char *Get_Q2ConfigString(int i);

#ifdef Q2BSPS
void VARGS Q2_Pmove (q2pmove_t *pmove);
#define	Q2PMF_DUCKED			1
#define	Q2PMF_JUMP_HELD		2
#define	Q2PMF_ON_GROUND		4
#define	Q2PMF_TIME_WATERJUMP	8	// pm_time is waterjump
#define	Q2PMF_TIME_LAND		16	// pm_time is time before rejump
#define	Q2PMF_TIME_TELEPORT	32	// pm_time is non-moving time
#define Q2PMF_NO_PREDICTION	64	// temporarily disables prediction (used for grappling hook)
#endif

vec3_t cl_predicted_origins[UPDATE_BACKUP];


/*
===================
CL_CheckPredictionError
===================
*/
#ifdef Q2BSPS
void CLQ2_CheckPredictionError (void)
{
	int		frame;
	int		delta[3];
	int		i;
	int		len;

	if (cl_nopred.value || (cl.q2frame.playerstate.pmove.pm_flags & Q2PMF_NO_PREDICTION))
		return;

	// calculate the last usercmd_t we sent that the server has processed
	frame = cls.netchan.incoming_acknowledged;
	frame &= (UPDATE_MASK);

	// compare what the server returned with what we had predicted it to be
	VectorSubtract (cl.q2frame.playerstate.pmove.origin, cl_predicted_origins[frame], delta);

	// save the prediction error for interpolation
	len = abs(delta[0]) + abs(delta[1]) + abs(delta[2]);
	if (len > 640)	// 80 world units
	{	// a teleport or something
		VectorClear (cl.prediction_error);
	}
	else
	{
//		if (/*cl_showmiss->value && */(delta[0] || delta[1] || delta[2]) )
//			Con_Printf ("prediction miss on %i: %i\n", cl.q2frame.serverframe,
//			delta[0] + delta[1] + delta[2]);

		VectorCopy (cl.q2frame.playerstate.pmove.origin, cl_predicted_origins[frame]);

		// save for error itnerpolation
		for (i=0 ; i<3 ; i++)
			cl.prediction_error[i] = delta[i]*0.125;
	}
}


/*
====================
CL_ClipMoveToEntities

====================
*/
void CLQ2_ClipMoveToEntities ( vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, trace_t *tr )
{
	int			i, x, zd, zu;
	trace_t		trace;
	float		*angles;
	entity_state_t	*ent;
	int			num;
	model_t		*cmodel;
	vec3_t		bmins, bmaxs;

	for (i=0 ; i<cl.q2frame.num_entities ; i++)
	{
		num = (cl.q2frame.parse_entities + i)&(MAX_PARSE_ENTITIES-1);
		ent = &clq2_parse_entities[num];

		if (!ent->solid)
			continue;

		if (ent->number == cl.playerview[0].playernum+1)
			continue;

		if (ent->solid == ES_SOLID_BSP)
		{	// special value for bmodel
			cmodel = cl.model_precache[ent->modelindex];
			if (!cmodel)
				continue;
			angles = ent->angles;
		}
		else
		{	// encoded bbox
			x = 8*(ent->solid & 31);
			zd = 8*((ent->solid>>5) & 31);
			zu = 8*((ent->solid>>10) & 63) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			cmodel = CM_TempBoxModel (bmins, bmaxs);
			angles = vec3_origin;	// boxes don't rotate
		}

		if (tr->allsolid)
			return;

		World_TransformedTrace (cmodel, 0, 0, start, end, mins, maxs, false, &trace, ent->origin, angles, MASK_PLAYERSOLID);

		if (trace.allsolid || trace.startsolid || trace.fraction < tr->fraction)
		{
			trace.ent = (struct edict_s *)ent;
			*tr = trace;
		}
	}
}


/*
================
CL_PMTrace
================
*/
q2trace_t	VARGS CLQ2_PMTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	q2trace_t	q2t;
	trace_t		t;

	// check against world
	cl.worldmodel->funcs.NativeTrace(cl.worldmodel, 0, 0, NULL, start, end, mins, maxs, false, MASK_PLAYERSOLID, &t);
	if (t.fraction < 1.0)
		t.ent = (struct edict_s *)1;

	// check all other solid models
	CLQ2_ClipMoveToEntities (start, mins, maxs, end, &t);

	q2t.allsolid = t.allsolid;
	q2t.contents = t.contents;
	VectorCopy(t.endpos, q2t.endpos);
	q2t.ent = t.ent;
	q2t.fraction = t.fraction;
	q2t.plane = t.plane;
	q2t.startsolid = t.startsolid;
	q2t.surface = t.surface;

	return q2t;
}

int		VARGS CLQ2_PMpointcontents (vec3_t point)
{
	int			i;
	entity_state_t	*ent;
	int			num;
	model_t		*cmodel;
	int			contents;

	contents = CM_PointContents (cl.worldmodel, point);

	for (i=0 ; i<cl.q2frame.num_entities ; i++)
	{
		num = (cl.q2frame.parse_entities + i)&(MAX_PARSE_ENTITIES-1);
		ent = &clq2_parse_entities[num];

		if (ent->solid != 31) // special value for bmodel
			continue;

		cmodel = cl.model_precache[ent->modelindex];
		if (!cmodel)
			continue;

		contents |= CM_TransformedPointContents (cl.worldmodel, point, cmodel->hulls[0].firstclipnode, ent->origin, ent->angles);
	}

	return contents;
}

#endif
/*
=================
CL_PredictMovement

Sets cl.predicted_origin and cl.predicted_angles
=================
*/
void CLQ2_PredictMovement (void)	//q2 doesn't support split clients.
{
#ifdef Q2BSPS
	int			ack, current;
	int			frame;
	int			oldframe;
	q2usercmd_t	*cmd;
	q2pmove_t		pm;
	int			step;
	int			oldz;
#endif
	int			i;
	int pnum = 0;

	if (cls.state != ca_active)
		return;

//	if (cl_paused->value)
//		return;

#ifdef Q2BSPS
	if (cl_nopred.value || (cl.q2frame.playerstate.pmove.pm_flags & Q2PMF_NO_PREDICTION))
#endif
	{	// just set angles
		for (i=0 ; i<3 ; i++)
		{
			cl.predicted_angles[i] = cl.playerview[pnum].viewangles[i] + SHORT2ANGLE(cl.q2frame.playerstate.pmove.delta_angles[i]);
		}
		return;
	}
#ifdef Q2BSPS
	ack = cls.netchan.incoming_acknowledged;
	current = cls.netchan.outgoing_sequence;

	// if we are too far out of date, just freeze
	if (current - ack >= UPDATE_MASK)
	{
//		if (cl_showmiss->value)
//			Con_Printf ("exceeded CMD_BACKUP\n");
		return;
	}

	// copy current state to pmove
	memset (&pm, 0, sizeof(pm));
	pm.trace = CLQ2_PMTrace;
	pm.pointcontents = CLQ2_PMpointcontents;

	pm_airaccelerate = atof(Get_Q2ConfigString(Q2CS_AIRACCEL));

	pm.s = cl.q2frame.playerstate.pmove;

//	SCR_DebugGraph (current - ack - 1, 0);

	frame = 0;

	// run frames
	while (++ack < current)
	{
		frame = ack & (UPDATE_MASK);
		cmd = (q2usercmd_t*)&cl.outframes[frame].cmd[0];
		cmd->msec = cl.outframes[frame].cmd[0].msec;

		pm.cmd = *cmd;
		Q2_Pmove (&pm);

		// save for debug checking
		VectorCopy (pm.s.origin, cl_predicted_origins[frame]);
	}

	if (independantphysics[0].msec)
	{
		cmd = (q2usercmd_t*)&independantphysics[0];
		cmd->msec = independantphysics[0].msec;

		pm.cmd = *cmd;
		Q2_Pmove (&pm);
	}

	oldframe = (ack-2) & (UPDATE_MASK);
	oldz = cl_predicted_origins[oldframe][2];
	step = pm.s.origin[2] - oldz;
	if (step > 63 && step < 160 && (pm.s.pm_flags & Q2PMF_ON_GROUND) )
	{
		cl.predicted_step = step * 0.125;
		cl.predicted_step_time = realtime - host_frametime * 0.5;
	}

	cl.playerview[0].onground = !!(pm.s.pm_flags & Q2PMF_ON_GROUND);


	// copy results out for rendering
	cl.predicted_origin[0] = pm.s.origin[0]*0.125;
	cl.predicted_origin[1] = pm.s.origin[1]*0.125;
	cl.predicted_origin[2] = pm.s.origin[2]*0.125;

	VectorCopy (pm.viewangles, cl.predicted_angles);
#endif
}

/*
=================
CL_NudgePosition

If pmove.origin is in a solid position,
try nudging slightly on all axis to
allow for the cut precision of the net coordinates
=================
*/
void CL_NudgePosition (void)
{
	vec3_t	base;
	int		x, y;

	if (cl.worldmodel->funcs.PointContents (cl.worldmodel, NULL, pmove.origin) == FTECONTENTS_EMPTY)
		return;

	VectorCopy (pmove.origin, base);
	for (x=-1 ; x<=1 ; x++)
	{
		for (y=-1 ; y<=1 ; y++)
		{
			pmove.origin[0] = base[0] + x * 1.0/8;
			pmove.origin[1] = base[1] + y * 1.0/8;
			if (cl.worldmodel->funcs.PointContents (cl.worldmodel, NULL, pmove.origin) == FTECONTENTS_EMPTY)
				return;
		}
	}
	Con_DPrintf ("CL_NudgePosition: stuck\n");
}

#endif

/*
==============
CL_PredictUsercmd
==============
*/
void CL_PredictUsercmd (int pnum, int entnum, player_state_t *from, player_state_t *to, usercmd_t *u)
{
	// split up very long moves
	if (u->msec > 50)
	{
		player_state_t temp;
		usercmd_t split;

		split = *u;
		split.msec = u->msec / 2;	//special care to avoid forgetting an msec here and there
		CL_PredictUsercmd (pnum, entnum, from, &temp, &split);
		split.msec = u->msec - split.msec;
		CL_PredictUsercmd (pnum, entnum, &temp, to, &split);
		return;
	}
	if (!cl.worldmodel || cl.worldmodel->loadstate != MLS_LOADED)
		return;

	VectorCopy (from->origin, pmove.origin);
	VectorCopy (u->angles, pmove.angles);
	VectorCopy (from->velocity, pmove.velocity);
	VectorCopy (from->gravitydir, pmove.gravitydir);

	if (!(pmove.velocity[0] == 0) && !(pmove.velocity[0] != 0))
	{
		Con_Printf("nan velocity!\n");
		pmove.velocity[0] = 0;
		pmove.velocity[1] = 0;
		pmove.velocity[2] = 0;
	}

	pmove.jump_msec = (cls.z_ext & Z_EXT_PM_TYPE) ? 0 : from->jump_msec;
	pmove.jump_held = from->jump_held;
	pmove.waterjumptime = from->waterjumptime;
	pmove.pm_type = from->pm_type;

	pmove.cmd = *u;
	pmove.skipent = entnum;

	movevars.entgravity = cl.playerview[pnum].entgravity;
	movevars.maxspeed = cl.playerview[pnum].maxspeed;
	movevars.bunnyspeedcap = cl.bunnyspeedcap;
	pmove.onladder = false;
	pmove.safeorigin_known = false;
	pmove.capsule = false;	//FIXME

	VectorCopy(from->szmins, pmove.player_mins);
	VectorCopy(from->szmaxs, pmove.player_maxs);

	PM_PlayerMove (cl.gamespeed);

	to->waterjumptime = pmove.waterjumptime;
	to->jump_held = pmove.jump_held;
	to->jump_msec = pmove.jump_msec;
	pmove.jump_msec = 0;

	VectorCopy (pmove.origin, to->origin);
	VectorCopy (pmove.angles, to->viewangles);
	VectorCopy (pmove.velocity, to->velocity);
	VectorCopy (pmove.gravitydir, to->gravitydir);
	to->onground = pmove.onground;

	to->weaponframe = from->weaponframe;
	to->pm_type = from->pm_type;

	VectorCopy(pmove.player_mins, to->szmins);
	VectorCopy(pmove.player_maxs, to->szmaxs);
}


//Used when cl_nopred is 1 to determine whether we are on ground, otherwise stepup smoothing code produces ugly jump physics
void CL_CatagorizePosition (playerview_t *pv, float *org)
{
	//fixme: in nq, we are told by the server and should skip this, which avoids needing to know the player's size.
	if (cl.spectator)
	{
		pv->onground = false;	// in air
		return;
	}
	VectorClear (pmove.velocity);
	VectorCopy (org, pmove.origin);
	pmove.numtouch = 0;
	PM_CategorizePosition ();
	pv->onground = pmove.onground;
}
//Smooth out stair step ups.
//Called before CL_EmitEntities so that the player's lightning model origin is updated properly
void CL_CalcCrouch (playerview_t *pv)
{
	qboolean teleported;
	vec3_t delta;
	float orgz = -DotProduct(pv->simorg, pv->gravitydir);	//compensate for running on walls.

	VectorSubtract(pv->simorg, pv->oldorigin, delta);

	teleported = Length(delta)>48;

	if (teleported)
	{
		// possibly teleported or respawned
		pv->oldz = orgz;
		pv->extracrouch = 0;
		pv->crouchspeed = 100;
		pv->crouch = 0;
		VectorCopy (pv->simorg, pv->oldorigin);
		return;
	}

	//check if we moved in the x/y axis. if we didn't then we're on a moving platform and shouldn't be crouching.
/*	VectorMA(pv->oldorigin, pv->oldz-orgz, pv->gravitydir, pv->oldorigin);
	VectorSubtract(pv->simorg, pv->oldorigin, delta);
	if (Length(delta)<0.001)
		pv->oldz = orgz;
*/
	VectorCopy (pv->simorg, pv->oldorigin);


	if (pv->onground && orgz - pv->oldz > 0)
	{
		if (orgz - pv->oldz > movevars.stepheight+2)
		{
			// if on steep stairs, increase speed
			if (pv->crouchspeed < 160)
			{
				pv->extracrouch = orgz - pv->oldz - host_frametime * 200 - 15;
				pv->extracrouch = min(pv->extracrouch, 5);
			}
			pv->crouchspeed = 160;
		}

		pv->oldz += host_frametime * pv->crouchspeed;
		if (pv->oldz > orgz)
			pv->oldz = orgz;

		if (orgz - pv->oldz > 15 + pv->extracrouch)
			pv->oldz = orgz - 15 - pv->extracrouch;
		pv->extracrouch -= host_frametime * 200;
		pv->extracrouch = max(pv->extracrouch, 0);

		pv->crouch = pv->oldz - orgz;
	}
	else
	{
		// in air or moving down
		pv->oldz = orgz;
		pv->crouch += host_frametime * 150;
		if (orgz - pv->oldz < 0)
			pv->crouch -= orgz - pv->oldz;	//if the view moved down, remove that amount from our crouching to avoid unneeded bobbing
		if (pv->crouch > 0)
			pv->crouch = 0;
		pv->crouchspeed = 100;
		pv->extracrouch = 0;
	}
}

float LerpAngles360(float to, float from, float frac)
{
	float delta;
	delta = (from-to);

	if (delta > 180)
		delta -= 360;
	if (delta < -180)
		delta += 360;

	return to + frac*delta;
}

short LerpAngles16(short to, short from, float frac)
{
	int delta;
	delta = (from-to);

	if (delta > 32767)
		delta -= 65535;
	if (delta < -32767)
		delta += 65535;

	return to + frac*delta;
}

void CL_CalcClientTime(void)
{
	extern float demtime;
	if (!cls.state)
	{
		cl.servertime += host_frametime;
		cl.time = cl.servertime;
		return;
	}
	else// if (cls.protocol != CP_QUAKE3)
	{
		float oldst = realtime;

		if (cls.demoplayback && cls.timedemo)
		{	//more deterministic. one frame is drawn per demo packet parsed. so sync to it as closely as possible.
			/*NOTE: this also has the effect of speeding up particles etc*/
			extern float olddemotime;
			cl.servertime = olddemotime;
		}
		if (cls.protocol != CP_QUAKE3 && (!cl_predict_smooth.ival || (cl_predict_smooth.ival == 2 && !cls.demoplayback)) && cls.demoplayback != DPB_MVD)
		{
			float f;
			f = cl.gametime - cl.oldgametime;
			if (f > 0.1)
				f = 0.1;
			f = (realtime - cl.gametimemark) / (f);
			f = bound(0, f, 1);
			cl.servertime = cl.gametime*f + cl.oldgametime*(1-f);
		}
		else
		{
			float min, max;

			oldst = cl.servertime;

			max = cl.gametime;
			min = cl.oldgametime;
			if (max < min)
				max = min;

			if (max)
			{
				extern cvar_t cl_demospeed;
				if (cls.demoplayback && cl_demospeed.value >= 0 && cls.state == ca_active)
					cl.servertime += host_frametime*cl_demospeed.value;
				else
					cl.servertime += host_frametime;
			}
			else
				cl.servertime = 0;

			if (cl.servertime > max)
			{
				if (cl.servertime > max)
				{
					cl.servertime = max;
//					Con_Printf("clamped to new time\n");
				}
				else
				{
					cl.servertime -= 0.02*(max - cl.servertime);
				}
			}
			if (cl.servertime < min)
			{
				if (cl.servertime < min-0.5)
				{
					cl.servertime = min-0.5;
//					Con_Printf("clamped to old time\n");
				}
				else if (cl.servertime < min-0.3)
				{
					cl.servertime += 0.02*(min - cl.servertime);
//					Con_Printf("running really slow\n");
				}
				else
				{
					cl.servertime += 0.01*(min - cl.servertime);
//					Con_Printf("running slow\n");
				}
			}
		}
		cl.time = cl.servertime;
		if (oldst == 0)
		{
			int i;
			for (i = 0; i < cl.allocated_client_slots; i++)
			{
				cl.players[i].entertime += cl.servertime;
			}
		}
		return;
	}

	if (cls.protocol == CP_NETQUAKE || (cls.demoplayback && cls.demoplayback != DPB_MVD && cls.demoplayback != DPB_EZTV))
	{
		float want;
//		float off;

		want = cl.oldgametime + realtime - cl.gametimemark;
//		off = (want - cl.time);
		if (want>cl.time)	//don't decrease
			cl.time = want;

//		Con_Printf("Drifted to %f off by %f\n", cl.time, off);

//		Con_Printf("\n");
		if (cl.time > cl.gametime)
		{
			cl.time = cl.gametime;
//			Con_Printf("max TimeClamp\n");
		}
		if (cl.time < cl.oldgametime)
		{
			cl.time = cl.oldgametime;
//			Con_Printf("old TimeClamp\n");
		}

	}
	else
	{
		if (cl_pushlatency.value > 0)
			Cvar_Set (&cl_pushlatency, "0");

		cl.time = realtime - cls.latency - cl_pushlatency.value*0.001;
		if (cl.time > realtime)
			cl.time = realtime;
	}
}

static void CL_DecodeStateSize(unsigned short solid, int modelindex, vec3_t mins, vec3_t maxs)
{
	if (solid == ES_SOLID_BSP)
	{
		if (modelindex < MAX_PRECACHE_MODELS && cl.model_precache[modelindex] && cl.model_precache[modelindex]->loadstate == MLS_LOADED)
		{
			VectorCopy(cl.model_precache[modelindex]->mins, mins);
			VectorCopy(cl.model_precache[modelindex]->maxs, maxs);
		}
		else
		{
			VectorClear(mins);
			VectorClear(maxs);
		}
	}
	else if (solid)
	{
		mins[0] = -8*(solid&31);
		mins[1] = -8*(solid&31);
		mins[2] = -8*((solid>>5)&31);
		maxs[0] = 8*(solid&31);
		maxs[1] = 8*(solid&31);
		maxs[2] = 8*((solid>>10)&63) - 32;
	}
	else
	{
		VectorClear(mins);
		VectorClear(maxs);
	}
}

/*called on packet reception*/
#include "pr_common.h"
static void CL_EntStateToPlayerState(player_state_t *plstate, entity_state_t *state)
{
	vec3_t a;
	int pmtype;
	qboolean onground = plstate->onground;
	vec3_t vel;
	VectorCopy(plstate->velocity, vel);
	memset(plstate, 0, sizeof(*plstate));
	switch(state->u.q1.pmovetype)
	{
	case MOVETYPE_NOCLIP:
		if (cls.z_ext & Z_EXT_PM_TYPE_NEW)
			pmtype = PM_SPECTATOR;
		else
			pmtype = PM_OLD_SPECTATOR;
		break;
	
	case MOVETYPE_FLY:
		pmtype = PM_FLY;
		break;
	case MOVETYPE_NONE:
		pmtype = PM_NONE;
		break;
	case MOVETYPE_BOUNCE:
	case MOVETYPE_TOSS:
		pmtype = PM_DEAD;
		break;
	case MOVETYPE_WALLWALK:
		pmtype = PM_WALLWALK;
		break;
	case MOVETYPE_6DOF:
		pmtype = PM_6DOF;
		break;
	default:
		pmtype = PM_NORMAL;
		break;
	}

	plstate->pm_type = pmtype;
	VectorCopy(state->origin, plstate->origin);
	if (cls.protocol == CP_NETQUAKE && !(cls.fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS))
	{	//nq is annoying, this stuff wasn't part of the entity state, so don't break it
		VectorCopy(vel, plstate->velocity);
		plstate->onground = onground;
	}
	else
		VectorScale(state->u.q1.velocity, 1/8.0, plstate->velocity);
	VectorCopy(state->angles, plstate->viewangles);
	if (state->u.q1.pmovetype)
		plstate->viewangles[0] *= -3;
	plstate->viewangles[2] = V_CalcRoll(plstate->viewangles, plstate->velocity);

	plstate->viewangles[0] = SHORT2ANGLE(state->u.q1.vangle[0]);
	plstate->viewangles[1] = SHORT2ANGLE(state->u.q1.vangle[1]);
	plstate->viewangles[2] = SHORT2ANGLE(state->u.q1.vangle[2]);

	a[0] = ((-192-state->u.q1.gravitydir[0])/256.0f) * 360;
	a[1] = (state->u.q1.gravitydir[1]/256.0f) * 360;
	a[2] = 0;
	AngleVectors(a, plstate->gravitydir, NULL, NULL);

	if (!state->solid)
	{
		VectorSet(plstate->szmins, -16, -16, -24);
		VectorSet(plstate->szmaxs, 16, 16, 32);
	}
	else
		CL_DecodeStateSize(state->solid, state->modelindex, plstate->szmins, plstate->szmaxs);
}
static void CL_EntStateToPlayerCommand(usercmd_t *cmd, entity_state_t *state, float age)
{
	int msec;
	float extra;
	memset(cmd, 0, sizeof(*cmd));

	extra = /*-cls.latency + */ 0.02;				//network latency
	extra += age;	//if the state is not exactly current
//	extra += realtime - cl.inframes[cl.validsequence&UPDATE_MASK].receivedtime;
//	extra += (cl.inframes[cl.validsequence&UPDATE_MASK].receivedtime - cl.inframes[cl.oldvalidsequence&UPDATE_MASK].receivedtime)*4;
	msec = 1000*extra;
//	Con_DPrintf("%i: age = %i, stale=%i\n", state->number, msec, state->u.q1.msec);
	msec += state->u.q1.msec;	//this is the age on the server
	cmd->msec = bound(0, msec, 250);

	cmd->forwardmove = state->u.q1.movement[0];
	cmd->sidemove = state->u.q1.movement[1];
	cmd->upmove = state->u.q1.movement[2];

	cmd->angles[0] = state->u.q1.vangle[0];// * -3 *65536/360.0;
	cmd->angles[1] = state->u.q1.vangle[1];// * 65536/360.0;
	cmd->angles[2] = state->u.q1.vangle[2];// * 65536/360.0;
}

void CL_PredictEntityMovement(entity_state_t *estate, float age)
{
	player_state_t startstate, resultstate;
	usercmd_t cmd;
	int oldphysent;
	extern cvar_t cl_predict_players;
	//build the entitystate state into a player state for prediction to use

	if (!estate->u.q1.pmovetype || !cl_predict_players.ival || age <= 0)
		VectorCopy(estate->origin, estate->u.q1.predorg);
	else
	{
		VectorClear(startstate.velocity);
		startstate.onground = false;
		CL_EntStateToPlayerState(&startstate, estate);
		CL_EntStateToPlayerCommand(&cmd, estate, age);

//		cmd.forwardmove = 5000;
//		cmd.msec = sin(realtime*6) * 128 + 128;
		oldphysent = pmove.numphysent;
		pmove.onground = true;
		CL_PredictUsercmd(0, estate->number, &startstate, &resultstate, &cmd);	//uses player 0's maxspeed/grav...
		pmove.numphysent = oldphysent;

		VectorCopy(resultstate.origin, estate->u.q1.predorg);
	}
}

/*
==============
CL_PredictMove
==============
*/
void CL_PredictMovePNum (int seat)
{
	//when this is called, the entity states have been interpolated.
	//interpolation state should be updated to match prediction state, so entities move correctly in mirrors/portals.

	//this entire function is pure convolouted bollocks.
	playerview_t *pv = &cl.playerview[seat];
	int			i;
	float		f;
	int			fromframe, toframe;
	outframe_t	*backdate;
	player_state_t *fromstate, *tostate, framebuf[2];	//need two framebufs so we can interpolate between two states.
	usercmd_t	*cmdfrom = NULL, *cmdto = NULL;
	double		fromtime, totime;
	int			oldphysent;
	double		simtime;	//this is server time if nopred is set (lerp-only), and local time if we're predicting
	extern cvar_t cl_netfps;
	lerpents_t	*le;
	qboolean	nopred;
	qboolean	lerpangles = false;
	
	//these are to make svc_viewentity work better
	float netfps = cl_netfps.value;

	if (!netfps)
	{
		//every video frame has its own input frame.
		simtime = realtime;
	}
	else
	{
		qboolean extrap = cl_predict_extrapolate.ival;
//		float fps = 1/host_frametime;
//		fps = bound(6.7, fps, cls.maxfps);
		netfps = bound(6.7, netfps, cls.maxfps);
//		if (netfps > fps)
//			netfps = fps;
		if (!*cl_predict_extrapolate.string)
			extrap = netfps < 30;
		if (!extrap)
		{
			//interpolate. The input rate is completely smoothed out, at the cost of some latency.
			//You can still get juddering if the video rate doesn't match the monitor refresh rate (and isn't so high that it doesn't matter).
			//note that the code below will back-date input frames if the server acks too fast.
			simtime = realtime - (1/netfps);
		}
		else
		{
			//extrapolate if we've a low net rate. This should reduce apparent lag, but will be jerky if the net rate is not an (inverse) multiple of the monitor rate.
			//this is in addition to any monitor desync.
			simtime = realtime;
		}
	}

	simtime -= cls.latency;
	simtime += bound(-0.5, cl_predict_timenudge.value, 0.5);

	pv->nolocalplayer = !!(cls.fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS) || (cls.protocol != CP_QUAKEWORLD);

#ifdef Q2CLIENT
	if (cls.protocol == CP_QUAKE2)
	{
		if (!cl.worldmodel || cl.worldmodel->loadstate != MLS_LOADED)
			return;
		pv->crouch = 0;
		CLQ2_PredictMovement();
		return;
	}
#endif

	if (cl.paused && !(cls.demoplayback!=DPB_MVD && cls.demoplayback!=DPB_EZTV) && (!cl.spectator || !pv->cam_auto))
		return;

	if (!cl.validsequence)
	{
		return;
	}

	if (cl.intermission==1 && cls.protocol == CP_QUAKEWORLD)
	{
		//quakeworld locks view position once you hit intermission.
		VectorCopy (pv->intermissionangles, pv->simangles);
		return;
	}
	else
	{
		if (cl.currentpackentities && cl.currentpackentities->fixangles[seat])
		{
			if (cl.previouspackentities && cl.previouspackentities->fixangles[seat]==cl.currentpackentities->fixangles[seat])
			{
				for (i = 0; i < 3; i++)
					pv->simangles[i] = LerpAngles360(cl.currentpackentities->fixedangles[seat][i], cl.previouspackentities->fixedangles[seat][i], 1-(cl.previouspackentities->fixangles[seat]?cl.packfrac:1));
			}
			else
				VectorCopy(cl.currentpackentities->fixedangles[seat], pv->simangles);

			if (cl.currentpackentities->fixangles[seat] == 2)
				lerpangles = (cls.demoplayback == DPB_QUAKEWORLD);
		}
		else
		{
			lerpangles = (cls.demoplayback == DPB_QUAKEWORLD);
			VectorCopy (pv->viewangles, pv->simangles);
		}
	}

	if (pv->cam_locked && pv->cam_spec_track >= 0)
	{
		extern cvar_t cl_chasecam;
		if (!cl_chasecam.ival)
		{
			//FIXME: don't early out, so that we can smooth out angles too
			VectorCopy(pv->cam_desired_position, pv->simorg);
			VectorClear(pv->simvel);
			return;
		}
	}

	if (!pv->cam_locked && pv->cam_auto && cl.spectator && pv->cam_spec_track >= 0 && pv->cam_spec_track < cl.allocated_client_slots && pv->viewentity != pv->cam_spec_track+1)
	{
		if (cl.inframes[cl.validsequence & UPDATE_MASK].playerstate[pv->cam_spec_track].messagenum == cl.validsequence)
		{
			pv->cam_locked = true;
			pv->viewentity = pv->cam_spec_track+1;
		}
		else if (pv->cam_spec_track+1 < cl.maxlerpents && cl.lerpents[pv->cam_spec_track+1].sequence == cl.lerpentssequence)
		{
			pv->cam_locked = true;
			pv->viewentity = pv->cam_spec_track+1;
		}
	}

	nopred = cl_nopred.ival;

	//don't wrap
	if (!cl.ackedmovesequence)
		nopred = true;
	else if (cl.movesequence - cl.ackedmovesequence >= UPDATE_BACKUP-1)
		return;

	//these things also force-disable prediction
	if ((cls.demoplayback==DPB_MVD || cls.demoplayback == DPB_EZTV) ||
		cl.paused || pv->pmovetype == PM_NONE || pv->pmovetype == PM_FREEZE || pv->cam_locked)
	{
		nopred = true;
	}

	// figure out the first frame to lerp from.
	// we generate one new input frame every 1/72th of a second, with a refresh rate of 60hz that's blatently obvious
	// if we live in the present, we'll only have half a frame. in order to avoid extrapolation (which can give a swimmy feel), we live in the past by one frame time period
	// if we're running somewhere with a low latency, we can get a reply from the server before our next input frame is even generated, so we need to go backwards beyond the current state

	if (nopred)
	{
		//match interpolation info
		fromframe = ((char*)cl.previouspackentities - (char*)&cl.inframes[0].packet_entities) / sizeof(inframe_t);
		fromtime = cl.inframes[fromframe & UPDATE_MASK].packet_entities.servertime;
		toframe = ((char*)cl.currentpackentities - (char*)&cl.inframes[0].packet_entities) / sizeof(inframe_t);
		totime = cl.inframes[toframe & UPDATE_MASK].packet_entities.servertime;
		simtime = cl.currentpacktime;
	}
	else
	{
		fromframe = 0;
		toframe = 0;
		totime = fromtime = 0;

		//try to find the inbound frame that sandwiches the realtime that we're trying to simulate.
		//if we're predicting, this will be some time in the future, and thus we'll be forced to pick the most recent frame.
		//if we're interpolating, we'll need to grab the frame before that.
		//we're only interested in inbound frames, not outbound, but its outbound frames that contain the prediction timing, so we need to look that up
		//(note that in qw, inframe[i].ack==i holds true, but this code tries to be generic for unsyncronised protocols)
		//(note that in nq, using outbound times means we'll skip over dupe states without noticing, and input packets with dupes should also be handled gracefully)
//		Con_DPrintf("in:%i:%i out:%i:%i ack:%i\n", cls.netchan.incoming_sequence, cl.validsequence, cls.netchan.outgoing_sequence,cl.movesequence, cl.ackedmovesequence);
		for (i = cl.validsequence; i >= cls.netchan.incoming_sequence - UPDATE_MASK; i--)
		{
			int out;
			//skip frames which were not received, or are otherwise invalid. yay packetloss
			if (cl.inframes[i & UPDATE_MASK].frameid != i || cl.inframes[i & UPDATE_MASK].invalid)
			{
//				Con_DPrintf("stale incoming command %i\n", i);
				continue;
			}

			//each inbound frame tracks the outgoing frame that was last applied to it, and its outgoing frames that contain our timing info
			out = cl.inframes[i&UPDATE_MASK].ackframe;
			backdate = &cl.outframes[out & UPDATE_MASK];
			if (backdate->cmd_sequence != out)
			{
//				Con_DPrintf("stale outgoing command %i (%i:%i:%i)\n", i, out, backdate->cmd_sequence, backdate->server_message_num);
				continue;
			}
			//okay, looks valid

			//if this is the first one we found, make sure both from+to are set properly
			if (!fromframe)
			{
				fromframe = i;
				fromtime = backdate->senttime; 
			}
			toframe = fromframe;
			totime = fromtime;
			cmdto = cmdfrom;
			fromframe = i;
			fromtime = backdate->senttime;
			cmdfrom = &backdate->cmd[seat];
			if (fromtime < simtime && fromframe != toframe)
				break;	//okay, we found the first frame that is older, no need to continue looking
		}
	}

//	Con_DPrintf("sim%f, %i(%i-%i): old%f, cur%f\n", simtime, cl.ackedmovesequence, fromframe, toframe, fromtime, totime);

	if (pv->cam_locked && cl.spectator && pv->viewentity && pv->viewentity <= cl.allocated_client_slots)
	{
		fromstate = &cl.inframes[fromframe & UPDATE_MASK].playerstate[pv->viewentity-1];
		tostate = &cl.inframes[toframe & UPDATE_MASK].playerstate[pv->viewentity-1];
	}
	else
	{
		if (cls.demoplayback==DPB_MVD || cls.demoplayback == DPB_EZTV)
		{
			fromstate = &cl.inframes[cl.ackedmovesequence & UPDATE_MASK].playerstate[pv->playernum];
			tostate = &cl.inframes[cl.movesequence & UPDATE_MASK].playerstate[pv->playernum];
		}
		else
		{
			fromstate = &cl.inframes[fromframe & UPDATE_MASK].playerstate[pv->playernum];
			tostate = &cl.inframes[toframe & UPDATE_MASK].playerstate[pv->playernum];
		}
	}
	pv->pmovetype = tostate->pm_type;
	le = &cl.lerpplayers[pv->playernum];

	if (!cmdfrom)
		cmdfrom = &cl.outframes[fromframe & UPDATE_MASK].cmd[pv->playernum];
	if (!cmdto)
		cmdto = &cl.outframes[toframe & UPDATE_MASK].cmd[pv->playernum];

	//if our network protocol doesn't have a concept of separate players, make sure our player states are updated from those entities
	//fixme: use entity states instead of player states to avoid the extra work here
	if (pv->nolocalplayer || nopred)
	{
		packet_entities_t *pe;
		pe = &cl.inframes[fromframe & UPDATE_MASK].packet_entities;
		for (i = 0; i < pe->num_entities; i++)
		{
			if (pe->entities[i].number == pv->viewentity)
			{
				CL_EntStateToPlayerState(fromstate, &pe->entities[i]);
				if (nopred)
					fromtime -= (pe->entities[i].u.q1.msec / 1000.0f);	//correct the time to match stale players
				break;
			}
		}
		pe = &cl.inframes[toframe & UPDATE_MASK].packet_entities;
		for (i = 0; i < pe->num_entities; i++)
		{
			if (pe->entities[i].number == pv->viewentity)
			{
				CL_EntStateToPlayerState(tostate, &pe->entities[i]);
				if (nopred)
					totime -= (pe->entities[i].u.q1.msec / 1000.0f);	//correct the time to match stale players. FIXME: this can push the simtime into the 'future' resulting in stuttering
				if (cls.fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)
				{
					//putting weapon frames in there was probably a stupid idea.
					if (!(cls.fteprotocolextensions2 & PEXT2_PREDINFO))
					{
						pv->stats[STAT_WEAPONFRAME] = cl.players[pv->playernum].stats[STAT_WEAPONFRAME] = pe->entities[i].u.q1.weaponframe;
						pv->statsf[STAT_WEAPONFRAME] = cl.players[pv->playernum].statsf[STAT_WEAPONFRAME] = pe->entities[i].u.q1.weaponframe;
					}
					pv->pmovetype = tostate->pm_type;
				}
				break;
			}
		}
		if (pv->nolocalplayer && pv->viewentity < cl.maxlerpents)
			le = &cl.lerpents[pv->viewentity];
	}

	// predict forward until cl.time <= to->senttime
	oldphysent = pmove.numphysent;
	CL_SetSolidPlayers();
	pmove.skipent = pv->viewentity;

	//just in case we don't run any prediction
	VectorCopy(tostate->gravitydir, pmove.gravitydir);

	if (nopred)
	{	//still need the player's size for onground detection and bobbing.
		VectorCopy(tostate->szmins, pmove.player_mins);
		VectorCopy(tostate->szmaxs, pmove.player_maxs);
	}
	else
	{
		for (i=1 ; i<UPDATE_BACKUP-1 && cl.ackedmovesequence+i < cl.movesequence; i++)
		{
			outframe_t *of = &cl.outframes[(cl.ackedmovesequence+i) & UPDATE_MASK];
			if (totime >= simtime)
			{
				if (i == 1)
				{
					//we must always predict a frame, just to ensure that the playerstate's jump status etc is valid for the next frame, even if we're not going to use it for interpolation.
					//this assumes that we always have at least one video frame to each network frame, of course.
					//note that q2 updates its values via networking rather than propagation.
					player_state_t tmp, *next;
//					Con_DPrintf(" propagate %i: %f-%f\n", cl.ackedmovesequence+i, fromtime, totime);
					CL_PredictUsercmd (seat, pv->viewentity, tostate, &tmp, &of->cmd[seat]);
					next = &cl.inframes[(toframe+i) & UPDATE_MASK].playerstate[pv->playernum];
					next->jump_held = tmp.jump_held;
					next->jump_msec = tmp.jump_msec;
					VectorCopy(tmp.gravitydir, next->gravitydir);
				}
				break;
			}
			if (of->cmd_sequence != cl.ackedmovesequence+i)
			{
//				Con_DPrintf("trying to predict a frame which is no longer valid\n");
				break;
			}
			fromtime = totime;
			fromstate = tostate;
			fromframe = toframe;	//qw debug
			cmdfrom = cmdto;

			cmdto = &of->cmd[seat];
			totime = of->senttime;
			toframe = cl.ackedmovesequence+i;//qw debug

			if (i == 1)//I've no idea how else to propogate event state from one frame to the next
				tostate = &cl.inframes[(fromframe+i) & UPDATE_MASK].playerstate[pv->playernum];
			else
				tostate = &framebuf[i&1];

//			Con_DPrintf(" pred %i: %f-%f\n", cl.ackedmovesequence+i, fromtime, totime);
			CL_PredictUsercmd (seat, pv->viewentity, fromstate, tostate, cmdto);
		}

		if (simtime > totime)
		{
			//extrapolate X extra seconds
			float msec;
			usercmd_t indcmd;

			msec = ((simtime - totime) * 1000);
			if (msec >= 1)
			{
				fromstate = tostate;
				fromtime = totime;
				fromframe = toframe;
				cmdfrom = cmdto;

				tostate = &framebuf[i++&1];
				if (independantphysics[seat].msec && !cls.demoplayback)
					indcmd = independantphysics[seat];
				else
					indcmd = *cmdto;
				cmdto = &indcmd;
				totime = simtime;
				toframe+=1;

				if (cls.demoplayback)
				{
					extern cvar_t cl_demospeed;
					msec *= cl_demospeed.value;
				}

				cmdto->msec = bound(0, msec, 250);

//				Con_DPrintf(" extrap %i: %f-%f (%g)\n", toframe, fromtime, simtime, simtime-fromtime);
				CL_PredictUsercmd (seat, pv->viewentity, fromstate, tostate, cmdto);
			}
		}
		pv->onground = pmove.onground;
		pv->pmovetype = tostate->pm_type;
	}

	pmove.numphysent = oldphysent;

	if (totime == fromtime)
	{
		VectorCopy (tostate->velocity, pv->simvel);
		VectorCopy (tostate->origin, pv->simorg);

		if (pv->viewentity && pv->viewentity != pv->playernum+1 && pv->cam_locked)
			VectorCopy(tostate->viewangles, pv->simangles);
//Con_DPrintf("%f %f %f\n", fromtime, simtime, totime);
	}
	else
	{
		vec3_t move;
		// now interpolate some fraction of the final frame
		f = (simtime - fromtime) / (totime - fromtime);

		if (f < 0)
			f = 0;
		if (f > 1)
			f = 1;
//Con_DPrintf("%i:%f %f %i:%f (%f)\n", fromframe, fromtime, simtime, toframe, totime, f);
		VectorSubtract(tostate->origin, fromstate->origin, move);
		if (DotProduct(move, move) > 128*128)
		{
			// teleported, so don't lerp
			VectorCopy (tostate->velocity, pv->simvel);
			VectorCopy (tostate->origin, pv->simorg);
		}
		else
		{
			for (i=0 ; i<3 ; i++)
			{
				extern cvar_t temp1;
				pv->simorg[i] = (1-f)*fromstate->origin[i]   + f*tostate->origin[i];
				pv->simvel[i] = (1-f)*fromstate->velocity[i] + f*tostate->velocity[i];

				if (pv->viewentity && pv->viewentity != pv->playernum+1 && pv->cam_locked)
				{
					pv->simangles[i] = LerpAngles360(fromstate->viewangles[i], tostate->viewangles[i], f);// * (360.0/65535);
//					pv->viewangles[i] = LerpAngles16(fromstate->command.angles[i], tostate->command.angles[i], f) * (360.0/65535);
				}
				else if (lerpangles)
					pv->simangles[i] = LerpAngles16(cmdfrom->angles[i], cmdto->angles[i], f) * (360.0/65535);
			}
		}
	}
	if (cls.protocol == CP_NETQUAKE && nopred)
		pv->onground = tostate->onground;
	else
		CL_CatagorizePosition(pv, tostate->origin);

	CL_CalcCrouch (pv);
	pv->waterlevel = pmove.waterlevel;
	VectorCopy(pmove.gravitydir, pv->gravitydir);

	if (le)
	{
		//keep the entity tracking the prediction position, so mirrors don't go all weird
		VectorMA(pv->simorg, -pv->crouch, pv->gravitydir, le->origin);
		if (pv->stats[STAT_HEALTH] > 0)
		{
			VectorScale(pv->simangles, 1, le->angles);
			if (pv->pmovetype == PM_6DOF)
				le->angles[0] *= -1;
			else
				le->angles[0] *= -0.333;
		}
	}
}

void CL_PredictMove (void)
{
	int i;

	// Set up prediction for other players
	CL_SetUpPlayerPrediction(false);

	// do client side motion prediction
	for (i = 0; i < cl.splitclients; i++)
		CL_PredictMovePNum(i);

	// Set up prediction for other players
	CL_SetUpPlayerPrediction(true);
}


/*
==============
CL_InitPrediction
==============
*/
void CL_InitPrediction (void)
{
	extern char cl_predictiongroup[];
	Cvar_Register (&cl_pushlatency, cl_predictiongroup);
	Cvar_Register (&cl_nopred,	cl_predictiongroup);
	Cvar_Register (&cl_predict_extrapolate,	cl_predictiongroup);
	Cvar_Register (&cl_predict_timenudge,	cl_predictiongroup);
	Cvar_Register (&cl_predict_smooth,	cl_predictiongroup);
}
