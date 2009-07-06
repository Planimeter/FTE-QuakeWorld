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
#include "qwsvdef.h"

#ifdef VM_Q1

#define	GAME_API_VERSION	12
#define MAX_Q1QVM_EDICTS	768 //according to ktx at api version 12 (fte's protocols go to 2048)

#define VMFSID_Q1QVM 57235	//a cookie

#define WASTED_EDICT_T_SIZE 112	//qclib has split edict_t and entvars_t.
								//mvdsv and the api we're implementing has them in one lump
								//so we need to bias our entvars_t and fake the offsets a little.

//===============================================================

//
// system traps provided by the main engine
//
typedef enum
{
	//============== general Quake services ==================

	G_GETAPIVERSION,		// ( void);	//0

	G_DPRINT,		// ( const char *string );	//1
	// print message on the local console

	G_ERROR,		// ( const char *string );	//2
	// abort the game
	G_GetEntityToken,		//3

	G_SPAWN_ENT,		//4
	G_REMOVE_ENT,		//5
	G_PRECACHE_SOUND,
	G_PRECACHE_MODEL,
	G_LIGHTSTYLE,
	G_SETORIGIN,
	G_SETSIZE,			//10
	G_SETMODEL,
	G_BPRINT,
	G_SPRINT,
	G_CENTERPRINT,
	G_AMBIENTSOUND,		//15
	G_SOUND,
	G_TRACELINE,
	G_CHECKCLIENT,
	G_STUFFCMD,
	G_LOCALCMD,			//20
	G_CVAR,
	G_CVAR_SET,
	G_FINDRADIUS,
	G_WALKMOVE,
	G_DROPTOFLOOR,		//25
	G_CHECKBOTTOM,
	G_POINTCONTENTS,
	G_NEXTENT,
	G_AIM,
	G_MAKESTATIC,		//30
	G_SETSPAWNPARAMS,
	G_CHANGELEVEL,
	G_LOGFRAG,
	G_GETINFOKEY,
	G_MULTICAST,		//35
	G_DISABLEUPDATES,
	G_WRITEBYTE,     
	G_WRITECHAR,     
	G_WRITESHORT,    
	G_WRITELONG,		//40
	G_WRITEANGLE,    
	G_WRITECOORD,    
	G_WRITESTRING,   
	G_WRITEENTITY,
	G_FLUSHSIGNON,		//45
	g_memset,
	g_memcpy,		
	g_strncpy,		
	g_sin,	
	g_cos,				//50
	g_atan2,	
	g_sqrt,	
	g_floor,	
	g_ceil,	
	g_acos,				//55
	G_CMD_ARGC,
	G_CMD_ARGV,
	G_TraceCapsule,
	G_FS_OpenFile,
	G_FS_CloseFile,		//60
	G_FS_ReadFile,
	G_FS_WriteFile,
	G_FS_SeekFile,
	G_FS_TellFile,
	G_FS_GetFileList,	//65
	G_CVAR_SET_FLOAT,
	G_CVAR_STRING,
	G_Map_Extension,
	G_strcmp,
	G_strncmp,			//70
	G_stricmp,
	G_strnicmp,
	G_Find,
	G_executecmd,
	G_conprint,			//75
	G_readcmd,
	G_redirectcmd,
	G_Add_Bot,
	G_Remove_Bot,
	G_SetBotUserInfo,	//80
	G_SetBotCMD,

	G_strftime,
	G_CMD_ARGS,
	G_CMD_TOKENIZE,
	G_strlcpy,		//85
	G_strlcat,
	G_MAKEVECTORS,
	G_NEXTCLIENT,

	G_PRECACHE_VWEP_MODEL,
	G_SETPAUSE,
	G_SETUSERINFO,
	G_MOVETOGOAL,


	G_MAX
} gameImport_t;


//
// functions exported by the game subsystem
//
typedef enum
{
	GAME_INIT,	// ( int levelTime, int randomSeed, int restart );
	// init and shutdown will be called every single level
	// The game should call G_GET_ENTITY_TOKEN to parse through all the
	// entity configuration text and spawn gentities.
	GAME_LOADENTS,
	GAME_SHUTDOWN,	// (void);

	GAME_CLIENT_CONNECT,	 	// ( int clientNum ,int isSpectator);
	GAME_PUT_CLIENT_IN_SERVER,

	GAME_CLIENT_USERINFO_CHANGED,	// ( int clientNum,int isSpectator );

	GAME_CLIENT_DISCONNECT,			// ( int clientNum,int isSpectator );

	GAME_CLIENT_COMMAND,			// ( int clientNum,int isSpectator );

	GAME_CLIENT_PRETHINK,
	GAME_CLIENT_THINK,				// ( int clientNum,int isSpectator );
	GAME_CLIENT_POSTTHINK,

	GAME_START_FRAME,					// ( int levelTime );
	GAME_SETCHANGEPARMS, //self
	GAME_SETNEWPARMS,
	GAME_CONSOLE_COMMAND,			// ( void );
	GAME_EDICT_TOUCH,                      //(self,other)
	GAME_EDICT_THINK,                      //(self,other=world,time)
	GAME_EDICT_BLOCKED,                     //(self,other)
	GAME_CLIENT_SAY, 		//(int isteam)
} gameExport_t;


typedef enum
{
	F_INT, 
	F_FLOAT,
	F_LSTRING,			// string on disk, pointer in memory, TAG_LEVEL
//	F_GSTRING,			// string on disk, pointer in memory, TAG_GAME
	F_VECTOR,
	F_ANGLEHACK,
//	F_ENTITY,			// index on disk, pointer in memory
//	F_ITEM,				// index on disk, pointer in memory
//	F_CLIENT,			// index on disk, pointer in memory
	F_IGNORE
} fieldtype_t;

typedef struct
{
	string_t	name;
	int			ofs;
	fieldtype_t	type;
//	int			flags;
} field_t;



typedef struct {
	int	pad[28];
	int	self;
	int	other;
	int	world;
	float	time;
	float	frametime;
	int	newmis;
	float	force_retouch;
	string_t	mapname;
	float	serverflags;
	float	total_secrets;
	float	total_monsters;
	float	found_secrets;
	float	killed_monsters;
	float	parm1;
	float	parm2;
	float	parm3;
	float	parm4;
	float	parm5;
	float	parm6;
	float	parm7;
	float	parm8;
	float	parm9;
	float	parm10;
	float	parm11;
	float	parm12;
	float	parm13;
	float	parm14;
	float	parm15;
	float	parm16;
	vec3_t	v_forward;
	vec3_t	v_up;
	vec3_t	v_right;
	float	trace_allsolid;
	float	trace_startsolid;
	float	trace_fraction;
	vec3_t	trace_endpos;
	vec3_t	trace_plane_normal;
	float	trace_plane_dist;
	int	trace_ent;
	float	trace_inopen;
	float	trace_inwater;
	int	msg_entity;
	func_t	main;
	func_t	StartFrame;
	func_t	PlayerPreThink;
	func_t	PlayerPostThink;
	func_t	ClientKill;
	func_t	ClientConnect;
	func_t	PutClientInServer;
	func_t	ClientDisconnect;
	func_t	SetNewParms;
	func_t	SetChangeParms;
} q1qvmglobalvars_t;


//this is not usable in 64bit to refer to a 32bit qvm (hence why we have two versions).
typedef struct
{
	edict_t		*ents;
	int		sizeofent;
	q1qvmglobalvars_t	*global;
	field_t		*fields;
	int 		APIversion;
} gameDataN_t;

typedef struct
{
	unsigned int	ents;
	int		sizeofent;
	unsigned int	global;
	unsigned int	fields;
	int		APIversion;
} gameData32_t;

typedef int		fileHandle_t;

typedef enum {
	FS_READ_BIN,
	FS_READ_TXT,
	FS_WRITE_BIN,
	FS_WRITE_TXT,
	FS_APPEND_BIN,
	FS_APPEND_TXT
} fsMode_t;

typedef enum {
	FS_SEEK_CUR,
	FS_SEEK_END,
	FS_SEEK_SET
} fsOrigin_t;











static field_t *qvmfields;
static char *q1qvmentstring;
static vm_t *q1qvm;
static progfuncs_t q1qvmprogfuncs;
static edict_t *q1qvmedicts[MAX_Q1QVM_EDICTS];


static void *evars;	//pointer to the gamecodes idea of an edict_t
static int vevars;	//offset into the vm base of evars

/*
static char *Q1QVMPF_AddString(progfuncs_t *pf, char *base, int minlength)
{
	char *n;
	int l = strlen(base);
	Con_Printf("warning: string %s will not be readable from the qvm\n", base);
	l = l<minlength?minlength:l;
	n = Z_TagMalloc(l+1, VMFSID_Q1QVM);
	strcpy(n, base);
	return n;
}
*/

static edict_t *Q1QVMPF_EdictNum(progfuncs_t *pf, unsigned int num)
{
	edict_t *e;

	if (/*num < 0 ||*/ num >= sv.max_edicts)
		return NULL;

	e = q1qvmedicts[num];
	if (!e)
	{
		e = q1qvmedicts[num] = Z_TagMalloc(sizeof(edict_t)+sizeof(extentvars_t), VMFSID_Q1QVM);
		e->v = (stdentvars_t*)((char*)evars + (num * pr_edict_size) + WASTED_EDICT_T_SIZE);
		e->xv = (extentvars_t*)(e+1);
		e->entnum = num;
	}
	return e;
}

static unsigned int Q1QVMPF_NumForEdict(progfuncs_t *pf, edict_t *e)
{
	return e->entnum;
}

static int Q1QVMPF_EdictToProgs(progfuncs_t *pf, edict_t *e)
{
	return e->entnum*pr_edict_size;
}
static edict_t *Q1QVMPF_ProgsToEdict(progfuncs_t *pf, int num)
{
	if (num % pr_edict_size)
		Con_Printf("Edict To Progs with remainder\n");
	num /= pr_edict_size;

	return Q1QVMPF_EdictNum(pf, num);
}

void Q1QVMED_ClearEdict (edict_t *e, qboolean wipe)
{
	int num = e->entnum;
	if (wipe)
		memset (e->v, 0, pr_edict_size - WASTED_EDICT_T_SIZE);
	e->isfree = false;
	e->entnum = num;
}

static void Q1QVMPF_EntRemove(progfuncs_t *pf, edict_t *e)
{
	if (!ED_CanFree(e))
		return;
	e->isfree = true;
	e->freetime = sv.time;
}

static edict_t *Q1QVMPF_EntAlloc(progfuncs_t *pf)
{
	int i;
	edict_t *e;
	for ( i=0 ; i<sv.num_edicts ; i++)
	{
		e = (edict_t*)EDICT_NUM(pf, i);
		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if (!e || (e->isfree && ( e->freetime < 2 || sv.time - e->freetime > 0.5 ) ))
		{
			Q1QVMED_ClearEdict (e, true);

			ED_Spawned((struct edict_s *) e, false);
			return (struct edict_s *)e;
		}
	}

	if (i >= sv.max_edicts-1)	//try again, but use timed out ents.
	{
		for ( i=0 ; i<sv.num_edicts ; i++)
		{
			e = (edict_t*)EDICT_NUM(pf, i);
			// the first couple seconds of server time can involve a lot of
			// freeing and allocating, so relax the replacement policy
			if (!e || (e->isfree))
			{
				Q1QVMED_ClearEdict (e, true);

				ED_Spawned((struct edict_s *) e, false);
				return (struct edict_s *)e;
			}
		}

		if (i >= sv.max_edicts-1)
		{
			Sys_Error ("ED_Alloc: no free edicts");
		}
	}

	sv.num_edicts++;
	e = (edict_t*)EDICT_NUM(pf, i);

// new ents come ready wiped
//	Q1QVMED_ClearEdict (e, false);

	ED_Spawned((struct edict_s *) e, false);

	return (struct edict_s *)e;
}

static int Q1QVMPF_LoadEnts(progfuncs_t *pf, char *mapstring, float spawnflags)
{
	q1qvmentstring = mapstring;
	VM_Call(q1qvm, GAME_LOADENTS);
	q1qvmentstring = NULL;
	return pr_edict_size;
}

static eval_t *Q1QVMPF_GetEdictFieldValue(progfuncs_t *pf, edict_t *e, char *fieldname, evalc_t *cache)
{
	if (!strcmp(fieldname, "message"))
	{
		return (eval_t*)&e->v->message;
	}
	return NULL;
}

static eval_t	*Q1QVMPF_FindGlobal		(progfuncs_t *prinst, char *name, progsnum_t num)
{
	return NULL;
}

static globalvars_t *Q1QVMPF_Globals(progfuncs_t *prinst, int prnum)
{
	return NULL;
}

static string_t Q1QVMPF_StringToProgs(progfuncs_t *prinst, char *str)
{
	return (string_t)(str - (char*)VM_MemoryBase(q1qvm));
}

static char *Q1QVMPF_StringToNative(progfuncs_t *prinst, string_t str)
{
	return (char*)VM_MemoryBase(q1qvm) + str;
}

void PF_WriteByte (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_WriteChar (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_WriteShort (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_WriteLong (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_WriteAngle (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_WriteCoord (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_WriteFloat (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_WriteString (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_WriteEntity (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_multicast (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PF_svtraceline (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_changelevel (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PF_cvar_set (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_cvar_setf (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PF_lightstyle (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_ambientsound (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PF_makestatic (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_logfrag (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_centerprint (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_localcmd (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_ExecuteCommand  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_setspawnparms (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_walkmove (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_ForceInfoKey(progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_precache_vwep_model(progfuncs_t *prinst, struct globalvars_s *pr_globals);


int PF_checkclient_Internal (progfuncs_t *prinst);
void PF_precache_sound_Internal (progfuncs_t *prinst, char *s);
void PF_precache_model_Internal (progfuncs_t *prinst, char *s);
void PF_setmodel_Internal (progfuncs_t *prinst, edict_t *e, char *m);
char *PF_infokey_Internal (int entnum, char *value);;

static int WrapQCBuiltin(builtin_t func, void *offset, unsigned int mask, const int *arg, char *argtypes)
{
	globalvars_t gv;
	int argnum=0;
	while(*argtypes)
	{
		switch(*argtypes++)
		{
		case 'f':
			gv.param[argnum++].f = VM_FLOAT(*arg++);
			break;
		case 'i':
			gv.param[argnum++].f = VM_LONG(*arg++);
			break;
		case 'n':	//ent num
			gv.param[argnum++].i = EDICT_TO_PROG(svprogfuncs, Q1QVMPF_EdictNum(svprogfuncs, VM_LONG(*arg++)));
			break;
		case 'v':	//three seperate args -> 1 vector
			gv.param[argnum].vec[0] = VM_FLOAT(*arg++);
			gv.param[argnum].vec[1] = VM_FLOAT(*arg++);
			gv.param[argnum].vec[2] = VM_FLOAT(*arg++);
			argnum++;
			break;
		case 's':	//three seperate args -> 1 vector
			gv.param[argnum].i = VM_LONG(*arg++);
			argnum++;
			break;
		}
	}
	svprogfuncs->callargc = &argnum;
	gv.ret.i = 0;
	func(svprogfuncs, &gv);
	return gv.ret.i;
}

#define VALIDATEPOINTER(o,l) if ((int)o + l >= mask || VM_POINTER(o) < offset) SV_Error("Call to game trap %i passes invalid pointer\n", fn);	//out of bounds.
static int syscallqvm (void *offset, unsigned int mask, int fn, const int *arg)
{
	switch (fn)
	{
	case G_GETAPIVERSION:
		return GAME_API_VERSION;

	case G_DPRINT:
		Con_Printf("%s", VM_POINTER(arg[0]));
		break;

	case G_ERROR:
		SV_Error("Q1QVM: %s", VM_POINTER(arg[0]));
		break;

	case G_GetEntityToken:
		{
			if (VM_OOB(arg[0], arg[1]))
				return false;
			if (q1qvmentstring)
			{
				char *ret = VM_POINTER(arg[0]);
				q1qvmentstring = COM_Parse(q1qvmentstring);
				Q_strncpyz(ret, com_token, VM_LONG(arg[1]));
				return *com_token != 0;
			}
			else
			{
				char *ret = VM_POINTER(arg[0]);
				strcpy(ret, "");
				return false;
			}
		}
		break;

	case G_SPAWN_ENT:
		return Q1QVMPF_EntAlloc(svprogfuncs)->entnum;

	case G_REMOVE_ENT:
		if (arg[0] >= sv.max_edicts)
			return false;
		Q1QVMPF_EntRemove(svprogfuncs, q1qvmedicts[arg[0]]);
		return true;

	case G_PRECACHE_SOUND:
		PF_precache_sound_Internal(svprogfuncs, VM_POINTER(arg[0]));
		break;

	case G_PRECACHE_MODEL:
		PF_precache_model_Internal(svprogfuncs, VM_POINTER(arg[0]));
		break;

	case G_LIGHTSTYLE:
		WrapQCBuiltin(PF_lightstyle, offset, mask, arg, "is");
		break;

	case G_SETORIGIN:
		{
			edict_t *e = Q1QVMPF_EdictNum(svprogfuncs, arg[0]);
			if (!e || e->isfree)
				return false;

			e->v->origin[0] = VM_FLOAT(arg[1]);
			e->v->origin[1] = VM_FLOAT(arg[2]);
			e->v->origin[2] = VM_FLOAT(arg[3]);
			SV_LinkEdict (e, false);
			return true;
		}
		break;
		
	case G_SETSIZE:
		{
			edict_t *e = Q1QVMPF_EdictNum(svprogfuncs, arg[0]);
			if (!e || e->isfree)
				return false;

			e->v->mins[0] = VM_FLOAT(arg[1]);
			e->v->mins[1] = VM_FLOAT(arg[2]);
			e->v->mins[2] = VM_FLOAT(arg[3]);

			e->v->maxs[0] = VM_FLOAT(arg[4]);
			e->v->maxs[1] = VM_FLOAT(arg[5]);
			e->v->maxs[2] = VM_FLOAT(arg[6]);

			VectorSubtract (e->v->maxs, e->v->mins, e->v->size);
			SV_LinkEdict (e, false);
			return true;
		}
	case G_SETMODEL:
		{
			edict_t *e = Q1QVMPF_EdictNum(svprogfuncs, arg[0]);
			PF_setmodel_Internal(svprogfuncs, e, VM_POINTER(arg[1]));
		}
		break;

	case G_BPRINT:
		SV_BroadcastPrintf(arg[0], "%s", VM_POINTER(arg[1]));
		break;

	case G_SPRINT:
		if ((unsigned)VM_LONG(arg[0]) > sv.allocated_client_slots)
			return 0;
		SV_ClientPrintf(&svs.clients[VM_LONG(arg[0])-1], VM_LONG(arg[1]), "%s", VM_POINTER(arg[2]));
		break;

	case G_CENTERPRINT:
		WrapQCBuiltin(PF_centerprint, offset, mask, arg, "ns");
		break;

	case G_AMBIENTSOUND:
		WrapQCBuiltin(PF_ambientsound, offset, mask, arg, "vsff");
		break;

	case G_SOUND:
//		( int edn, int channel, char *samp, float vol, float att )
		SVQ1_StartSound (Q1QVMPF_EdictNum(svprogfuncs, VM_LONG(arg[0])), VM_LONG(arg[1]), VM_POINTER(arg[2]), VM_FLOAT(arg[3])*255, VM_FLOAT(arg[4]));
		break;

	case G_TRACELINE:
		WrapQCBuiltin(PF_svtraceline, offset, mask, arg, "vvin");
		break;

	case G_CHECKCLIENT:
		return PF_checkclient_Internal(svprogfuncs);

	case G_STUFFCMD:
		{
			char *s;
			client_t *cl;
			if ((unsigned)VM_LONG(arg[0]) > sv.allocated_client_slots)
				return -1;
			cl = &svs.clients[VM_LONG(arg[0])-1];
			if (cl->state != cs_spawned)
				return -1;
			s = VM_POINTER(arg[1]);

			ClientReliableWrite_Begin (cl, svc_stufftext, 3+strlen(s));
			ClientReliableWrite_String(cl, s);
		}
		break;

	case G_LOCALCMD:
		WrapQCBuiltin(PF_localcmd, offset, mask, arg, "s");
		break;

	case G_CVAR:
		{
			int i;
			cvar_t *c;
			c = Cvar_Get(VM_POINTER(arg[0]), "", 0, "Gamecode");
			i = VM_LONG(c->value);
			return i;
		}

	case G_CVAR_SET:
		WrapQCBuiltin(PF_cvar_set, offset, mask, arg, "ss");
		break;

	case G_FINDRADIUS:
		{
			int start = ((char*)VM_POINTER(arg[0]) - (char*)evars) / pr_edict_size;
			edict_t *ed;
			vec3_t diff;
			float *org = VM_POINTER(arg[1]);
			float rad = VM_FLOAT(arg[2]);
			rad *= rad;
			for(start++; start < sv.num_edicts; start++)
			{
				ed = EDICT_NUM(svprogfuncs, start);
				if (ed->isfree)
					continue;
				VectorSubtract(ed->v->origin, org, diff);
				if (rad > DotProduct(diff, diff))
					return (int)(vevars + start*pr_edict_size);
			}
			return 0;
		}

	case G_WALKMOVE:
		return 0;//FIXME WrapQCBuiltin(PF_cvar_set, offset, mask, arg, "ff");
#ifdef _MSC_VER
#pragma message("G_WALKMOVE not implemented")
#elif defined(GCC)
#endif

	case G_DROPTOFLOOR:
		{
			edict_t		*ent;
			vec3_t		end;
			vec3_t		start;
			trace_t		trace;
			extern cvar_t pr_droptofloorunits;

			ent = EDICT_NUM(svprogfuncs, arg[0]);

			VectorCopy (ent->v->origin, end);
			if (pr_droptofloorunits.value > 0)
				end[2] -= pr_droptofloorunits.value;
			else
				end[2] -= 256;

			VectorCopy (ent->v->origin, start);
			trace = SV_Move (start, ent->v->mins, ent->v->maxs, end, MOVE_NORMAL, ent);

			if (trace.fraction == 1 || trace.allsolid)
				return false;
			else
			{
				VectorCopy (trace.endpos, ent->v->origin);
				SV_LinkEdict (ent, false);
				ent->v->flags = (int)ent->v->flags | FL_ONGROUND;
				ent->v->groundentity = EDICT_TO_PROG(svprogfuncs, trace.ent);
				return true;
			}
		}
		break;

	case G_CHECKBOTTOM:
		return SV_CheckBottom(EDICT_NUM(svprogfuncs, VM_LONG(arg[0])));

	case G_POINTCONTENTS:
		{
			vec3_t v;
			v[0] = VM_FLOAT(arg[0]);
			v[1] = VM_FLOAT(arg[1]);
			v[2] = VM_FLOAT(arg[2]);
			return sv.worldmodel->funcs.PointContents(sv.worldmodel, v);
		}
		break;
	
	case G_NEXTENT:
		{	//input output are entity numbers
			unsigned int i;
			edict_t	*ent;

			i = VM_LONG(arg[0]);
			while (1)
			{
				i++;
				if (i >= sv.num_edicts)
				{
					return 0;
				}
				ent = EDICT_NUM(svprogfuncs, i);
				if (!ent->isfree)
				{
					return i;
				}
			}
			break;
		}
		/*
	case G_AIM:	//not in mvdsv anyway
		break;
*/
	case G_MAKESTATIC:
		WrapQCBuiltin(PF_makestatic, offset, mask, arg, "n");
		break;

	case G_SETSPAWNPARAMS:
		WrapQCBuiltin(PF_setspawnparms, offset, mask, arg, "n");
		break;

	case G_CHANGELEVEL:
		WrapQCBuiltin(PF_changelevel, offset, mask, arg, "s");
		break;

	case G_LOGFRAG:
		WrapQCBuiltin(PF_logfrag, offset, mask, arg, "nn");
		break;
	case G_PRECACHE_VWEP_MODEL:
		{
		int i = WrapQCBuiltin(PF_precache_vwep_model, offset, mask, arg, "s");
		float f = *(float*)&i;
		return f;
		}
		break;

	case G_GETINFOKEY:
		{
			char *v;
			if (VM_OOB(arg[2], arg[3]))
				return -1;
			v = PF_infokey_Internal(VM_LONG(arg[0]), VM_POINTER(arg[1]));
			Q_strncpyz(VM_POINTER(arg[2]), v, VM_LONG(arg[3]));
		}
		break;

	case G_MULTICAST:
		WrapQCBuiltin(PF_multicast, offset, mask, arg, "vi");
		break;

	case G_DISABLEUPDATES:
		//FIXME: remember to ask mvdsv people why this is useful
		Con_Printf("G_DISABLEUPDATES: not supported\n");
		break;

	case G_WRITEBYTE:
		WrapQCBuiltin(PF_WriteByte, offset, mask, arg, "ii");
		break;
	case G_WRITECHAR:
		WrapQCBuiltin(PF_WriteChar, offset, mask, arg, "ii");
		break;
	case G_WRITESHORT:
		WrapQCBuiltin(PF_WriteShort, offset, mask, arg, "ii");
		break;
	case G_WRITELONG:
		WrapQCBuiltin(PF_WriteLong, offset, mask, arg, "ii");
		break;
	case G_WRITEANGLE:
		WrapQCBuiltin(PF_WriteAngle, offset, mask, arg, "if");
		break;
	case G_WRITECOORD:
		WrapQCBuiltin(PF_WriteCoord, offset, mask, arg, "if");
		break;
	case G_WRITESTRING:
		WrapQCBuiltin(PF_WriteString, offset, mask, arg, "is");
		break;
	case G_WRITEENTITY:
		WrapQCBuiltin(PF_WriteEntity, offset, mask, arg, "in");
		break;

	case G_FLUSHSIGNON:
		SV_FlushSignon ();
		break;

	case g_memset:
		{
			void *dst = VM_POINTER(arg[0]);
			VALIDATEPOINTER(arg[0], arg[2]);
			memset(dst, arg[1], arg[2]);
			return arg[0];
		}
	case g_memcpy:
		{
			void *dst = VM_POINTER(arg[0]);
			void *src = VM_POINTER(arg[1]);
			VALIDATEPOINTER(arg[0], arg[2]);
			memmove(dst, src, arg[2]);
			return arg[0];
		}
		break;
	case g_strncpy:
		VALIDATEPOINTER(arg[0], arg[2]);
		Q_strncpyS(VM_POINTER(arg[0]), VM_POINTER(arg[1]), arg[2]);
		return arg[0];
	case g_sin:
		VM_FLOAT(fn)=(float)sin(VM_FLOAT(arg[0]));
		return fn;
	case g_cos:
		VM_FLOAT(fn)=(float)cos(VM_FLOAT(arg[0]));
		return fn;
	case g_atan2:
		VM_FLOAT(fn)=(float)atan2(VM_FLOAT(arg[0]), VM_FLOAT(arg[1]));
		return fn;
	case g_sqrt:
		VM_FLOAT(fn)=(float)sqrt(VM_FLOAT(arg[0]));
		return fn;
	case g_floor:
		VM_FLOAT(fn)=(float)floor(VM_FLOAT(arg[0]));
		return fn;
	case g_ceil:
		VM_FLOAT(fn)=(float)ceil(VM_FLOAT(arg[0]));
		return fn;
	case g_acos:
		VM_FLOAT(fn)=(float)acos(VM_FLOAT(arg[0]));
		return fn;

	case G_CMD_ARGC:
		return Cmd_Argc();
	case G_CMD_ARGV:
		{
			char *c;
			c = Cmd_Argv(VM_LONG(arg[0]));
			if (VM_OOB(arg[1], arg[2]))
				return -1;
			Q_strncpyz(VM_POINTER(arg[1]), c, VM_LONG(arg[2]));
		}
		break;

	case G_TraceCapsule:
		WrapQCBuiltin(PF_svtraceline, offset, mask, arg, "vvinvv");
		break;

	case G_FS_OpenFile:
		//0 = name
		//1 = &handle
		//2 = mode
		//ret = filesize or -1
		
	//	Con_Printf("G_FSOpenFile: %s (mode %i)\n", VM_POINTER(arg[0]), arg[2]);

		return VM_fopen(VM_POINTER(arg[0]), VM_POINTER(arg[1]), arg[2]/2, VMFSID_Q1QVM);

	case G_FS_CloseFile:
		VM_fclose(arg[0], VMFSID_Q1QVM);
		break;

	case G_FS_ReadFile:
		if (VM_OOB(arg[0], arg[1]))
			return 0;
		return VM_FRead(VM_POINTER(arg[0]), VM_LONG(arg[1]), VM_LONG(arg[2]), VMFSID_Q1QVM);
/*
	//not supported, open will fail anyway
	case G_FS_WriteFile:
		return VM_FWrite(VM_POINTER(arg[0]), VM_LONG(arg[1]), VM_LONG(arg[2]), VMFSID_Q1QVM);
		break;
*/
/*
	case G_FS_SeekFile:
//	int trap_FS_SeekFile( fileHandle_t handle, int offset, int type )
		return VM_FSeek(VM_LONG(arg[0]), VM_LONG(arg[1]), VM_LONG(arg[2]));
		break;
*/
/*
	case G_FS_TellFile:
		break;
*/

	case G_FS_GetFileList:
		if (VM_OOB(arg[2], arg[3]))
			return 0;
		return VM_GetFileList(VM_POINTER(arg[0]), VM_POINTER(arg[1]), VM_POINTER(arg[2]), VM_LONG(arg[3]));
		break;

	case G_CVAR_SET_FLOAT:
		WrapQCBuiltin(PF_cvar_setf, offset, mask, arg, "sf");
		break;

	case G_CVAR_STRING:
		{
			cvar_t *cv;
			if (VM_OOB(arg[1], arg[2]))
				return -1;
			cv = Cvar_Get(VM_POINTER(arg[0]), "", 0, "QC variables");
			Q_strncpyz(VM_POINTER(arg[1]), cv->string, VM_LONG(arg[2]));
		}
		break;
	
	case G_Map_Extension:
		//yes, this does exactly match mvdsv...
		if (VM_LONG(arg[1]) < G_MAX)
			return -2;	//can't map that there
		else
			return -1;	//extension not known
	
	case G_strcmp:
		{
			char *a = VM_POINTER(arg[0]);
			char *b = VM_POINTER(arg[1]);
			return strcmp(a, b);
		}
	case G_strncmp:
		{
			char *a = VM_POINTER(arg[0]);
			char *b = VM_POINTER(arg[1]);
			return strncmp(a, b, VM_LONG(arg[2]));
		}
	case G_stricmp:
		{
			char *a = VM_POINTER(arg[0]);
			char *b = VM_POINTER(arg[1]);
			return stricmp(a, b);
		}
	case G_strnicmp:
		{
			char *a = VM_POINTER(arg[0]);
			char *b = VM_POINTER(arg[1]);
			return strnicmp(a, b, VM_LONG(arg[2]));
		}

	case G_Find:
		{
			edict_t *e = VM_POINTER(arg[0]);
			int ofs = VM_LONG(arg[1]) - WASTED_EDICT_T_SIZE;
			char *match = VM_POINTER(arg[2]);
			char *field;
			int first = e?((char*)e - (char*)evars)/pr_edict_size:0;
			int i;
			if (!match)
				match = "";
			for (i = first+1; i < sv.num_edicts; i++)
			{
				e = q1qvmedicts[i];
				field = VM_POINTER(*((string_t*)e->v + ofs/4));
				if (field == NULL)
				{
					if (*match == '\0')
						return ((char*)e->v - (char*)offset)-WASTED_EDICT_T_SIZE;
				}
				else
				{
					if (!strcmp(field, match))
						return ((char*)e->v - (char*)offset)-WASTED_EDICT_T_SIZE;
				}
			}
		}
		return 0;

	case G_executecmd:
		WrapQCBuiltin(PF_ExecuteCommand, offset, mask, arg, "");
		break;

	case G_conprint:
		Con_Printf("%s", VM_POINTER(arg[0]));
		break;

	case G_readcmd:
		{
			extern char outputbuf[];
			extern redirect_t sv_redirected;
			extern int sv_redirectedlang;
			redirect_t old;
			int oldl;

			char *s = VM_POINTER(arg[0]);
			char *output = VM_POINTER(arg[1]);
			int outputlen = VM_LONG(arg[2]);

			if (VM_OOB(arg[1], arg[2]))
				return -1;

			Cbuf_Execute();	//FIXME: this code is flawed
			Cbuf_AddText (s, RESTRICT_LOCAL);

			old = sv_redirected;
			oldl = sv_redirectedlang;
			if (old != RD_NONE)
				SV_EndRedirect();

			SV_BeginRedirect(RD_OBLIVION, LANGDEFAULT);
			Cbuf_Execute();
			Q_strncpyz(output, outputbuf, outputlen);
			SV_EndRedirect();

			if (old != RD_NONE)
				SV_BeginRedirect(old, oldl);

Con_DPrintf("PF_readcmd: %s\n%s", s, output);

		}
		break;

	case G_redirectcmd:
		//FIXME: KTX uses this, along with a big fat warning.
		//it shouldn't be vital to the normal functionality
		//just restricts admin a little (did these guys never hear of rcon?)
		//I'm too lazy to implement it though.
		return 0;

	case G_Add_Bot:
		//FIXME: not implemented, always returns failure.
		//the other bot functions only ever work on bots anyway, so don't need to be implemented until this one is
		return 0;
/*
	case G_Remove_Bot:
		break;
	case G_SetBotCMD:
		break;
*/
	case G_SETUSERINFO:
		{
			char *key = VM_POINTER(arg[1]);
			if (*key == '*' && (VM_LONG(arg[3])&1))
				return -1;	//denied!
		}
		//fallthrough

	case G_MOVETOGOAL:
		return !!WrapQCBuiltin(SV_MoveToGoal, offset, mask, arg, "f");

	case G_SetBotUserInfo:
		WrapQCBuiltin(PF_ForceInfoKey, offset, mask, arg, "ess");
		return 0;

	case G_strftime:
		{
			char *out = VM_POINTER(arg[0]);
			char *fmt = VM_POINTER(arg[2]);
			time_t curtime;
			struct tm *local;
			if (VM_OOB(arg[0], arg[1]) || !out)
				return -1;	//please don't corrupt me
			time(&curtime);
			curtime += VM_LONG(arg[3]);
			local = localtime(&curtime);
			strftime(out, VM_LONG(arg[1]), fmt, local);
		}
		break;
	case G_CMD_ARGS:
		{
			char *c;
			c = Cmd_Args();
			if (VM_OOB(arg[0], arg[1]))
				return -1;
			Q_strncpyz(VM_POINTER(arg[0]), c, VM_LONG(arg[1]));
		}
		break;
	case G_CMD_TOKENIZE:
		{
			char *str = VM_POINTER(arg[0]);
			Cmd_TokenizeString(str, false, false);
			return Cmd_Argc();
		}
		break;
	case G_strlcpy:
		{
			char *dst = VM_POINTER(arg[0]);
			char *src = VM_POINTER(arg[1]);
			if (VM_OOB(arg[0], arg[2]))
				return -1;
			Q_strncpyz(dst, src, VM_LONG(arg[2]));
			//WARNING: no return value
		}
		break;
	case G_strlcat:
		{
			char *dst = VM_POINTER(arg[0]);
			char *src = VM_POINTER(arg[1]);
			if (VM_OOB(arg[0], arg[2]))
				return -1;
			Q_strncatz(dst, src, VM_LONG(arg[2]));
			//WARNING: no return value
		}
		break;

	case G_MAKEVECTORS:
		AngleVectors(VM_POINTER(arg[0]), P_VEC(v_forward), P_VEC(v_right), P_VEC(v_up));
		break;

	case G_NEXTCLIENT:
		{
			unsigned int start = ((char*)VM_POINTER(arg[0]) - (char*)evars) / pr_edict_size;
			while (start < sv.allocated_client_slots)
			{
				if (svs.clients[start].state == cs_spawned)
					return (int)(vevars + (start+1) * pr_edict_size);
				start++;
			}
			return 0;
		}
		break;

	default:
		SV_Error("Q1QVM: Trap %i not implemented\n", fn);
		break;
	}
	return 0;
}

static int EXPORT_FN syscallnative (int arg, ...)
{
	int args[13];
	va_list argptr;

	va_start(argptr, arg);
	args[0]=va_arg(argptr, int);
	args[1]=va_arg(argptr, int);
	args[2]=va_arg(argptr, int);
	args[3]=va_arg(argptr, int);
	args[4]=va_arg(argptr, int);
	args[5]=va_arg(argptr, int);
	args[6]=va_arg(argptr, int);
	args[7]=va_arg(argptr, int);
	args[8]=va_arg(argptr, int);
	args[9]=va_arg(argptr, int);
	args[10]=va_arg(argptr, int);
	args[11]=va_arg(argptr, int);
	args[12]=va_arg(argptr, int);
	va_end(argptr);

	return syscallqvm(NULL, ~0, arg, args);
}

void Q1QVM_Shutdown(void)
{
	int i;
	if (q1qvm)
	{
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (svs.clients[i].name)
				Q_strncpyz(svs.clients[i].namebuf, svs.clients[i].name, sizeof(svs.clients[i].namebuf));
			svs.clients[i].name = svs.clients[i].namebuf;
		}
		VM_Destroy(q1qvm);
	}
	q1qvm = NULL;
	VM_fcloseall(VMFSID_Q1QVM);
	if (svprogfuncs == &q1qvmprogfuncs)
		svprogfuncs = NULL;
	Z_FreeTags(VMFSID_Q1QVM);
}

qboolean PR_LoadQ1QVM(void)
{
	static float writable;
	int i;
	gameDataN_t *gd, gdm;
	gameData32_t *gd32;
	int ret;

	if (q1qvm)
		VM_Destroy(q1qvm);

	q1qvm = VM_Create(NULL, "qwprogs", syscallnative, syscallqvm);
	if (!q1qvm)
		return false;


	progstype = PROG_QW;


	svprogfuncs = &q1qvmprogfuncs;


//	q1qvmprogfuncs.AddString = Q1QVMPF_AddString;	//using this breaks 64bit support, and is a 'bad plan' elsewhere too,
	q1qvmprogfuncs.EDICT_NUM = Q1QVMPF_EdictNum;
	q1qvmprogfuncs.NUM_FOR_EDICT = Q1QVMPF_NumForEdict;
	q1qvmprogfuncs.EdictToProgs = Q1QVMPF_EdictToProgs;
	q1qvmprogfuncs.ProgsToEdict = Q1QVMPF_ProgsToEdict;
	q1qvmprogfuncs.EntAlloc = Q1QVMPF_EntAlloc;
	q1qvmprogfuncs.EntFree = Q1QVMPF_EntRemove;
	q1qvmprogfuncs.FindGlobal = Q1QVMPF_FindGlobal;
	q1qvmprogfuncs.load_ents = Q1QVMPF_LoadEnts;
	q1qvmprogfuncs.globals = Q1QVMPF_Globals;
	q1qvmprogfuncs.GetEdictFieldValue = Q1QVMPF_GetEdictFieldValue;
	q1qvmprogfuncs.StringToProgs = Q1QVMPF_StringToProgs;
	q1qvmprogfuncs.StringToNative = Q1QVMPF_StringToNative;

	sv.num_edicts = 0;	//we're not ready for most of the builtins yet
	sv.max_edicts = 0;	//so clear these out, just in case
	pr_edict_size = 0;	//if we get a division by zero, then at least its a safe crash

	memset(q1qvmedicts, 0, sizeof(q1qvmedicts));

	q1qvmprogfuncs.stringtable = VM_MemoryBase(q1qvm);

	ret = VM_Call(q1qvm, GAME_INIT, (int)(sv.time*1000), rand());
	if (!ret)
	{
		Q1QVM_Shutdown();
		return false;
	}
	gd32 = (gameData32_t*)((char*)VM_MemoryBase(q1qvm) + ret);	//qvm is 32bit

	//when running native64, we need to convert these to real types, so we can use em below
	gd = &gdm;
	gd->ents = (struct edict_s *)gd32->ents;
	gd->sizeofent = gd32->sizeofent;
	gd->global = (q1qvmglobalvars_t *)gd32->global;
	gd->fields = (field_t *)gd32->fields;
	gd->APIversion = gd32->APIversion;

	pr_edict_size = gd->sizeofent;

	vevars = (long)gd->ents;
	evars = ((char*)VM_MemoryBase(q1qvm) + vevars);
	//FIXME: range check this pointer
	//FIXME: range check the globals pointer

	sv.num_edicts = 1;
	sv.max_edicts = sizeof(q1qvmedicts)/sizeof(q1qvmedicts[0]);

//WARNING: global is not remapped yet...
//This code is written evilly, but works well enough
#define globalint(required, name) pr_nqglobal_struct->name = (int*)((char*)VM_MemoryBase(q1qvm)+(long)&gd->global->name)	//the logic of this is somewhat crazy
#define globalfloat(required, name) pr_nqglobal_struct->name = (float*)((char*)VM_MemoryBase(q1qvm)+(long)&gd->global->name)
#define globalstring(required, name) pr_nqglobal_struct->name = (string_t*)((char*)VM_MemoryBase(q1qvm)+(long)&gd->global->name)
#define globalvec(required, name) pr_nqglobal_struct->V_##name = (vec3_t*)((char*)VM_MemoryBase(q1qvm)+(long)&gd->global->name)
#define globalfunc(required, name) pr_nqglobal_struct->name = (int*)((char*)VM_MemoryBase(q1qvm)+(long)&gd->global->name)
	globalint		(true, self);	//we need the qw ones, but any in standard quake and not quakeworld, we don't really care about.
	globalint		(true, other);
	globalint		(true, world);
	globalfloat		(true, time);
	globalfloat		(true, frametime);
	globalint		(false, newmis);	//not always in nq.
	globalfloat		(false, force_retouch);
	globalstring	(true, mapname);
//	globalfloat		(false, deathmatch);
//	globalfloat		(false, coop);
//	globalfloat		(false, teamplay);
	globalfloat		(true, serverflags);
	globalfloat		(true, total_secrets);
	globalfloat		(true, total_monsters);
	globalfloat		(true, found_secrets);
	globalfloat		(true, killed_monsters);
	globalvec		(true, v_forward);
	globalvec		(true, v_up);
	globalvec		(true, v_right);
	globalfloat		(true, trace_allsolid);
	globalfloat		(true, trace_startsolid);
	globalfloat		(true, trace_fraction);
	globalvec		(true, trace_endpos);
	globalvec		(true, trace_plane_normal);
	globalfloat		(true, trace_plane_dist);
	globalint		(true, trace_ent);
	globalfloat		(true, trace_inopen);
	globalfloat		(true, trace_inwater);
//	globalfloat		(false, trace_endcontents);
//	globalfloat		(false, trace_surfaceflags);
//	globalfloat		(false, cycle_wrapped);
	globalint		(false, msg_entity);
	globalfunc		(false, main);
	globalfunc		(true, StartFrame);
	globalfunc		(true, PlayerPreThink);
	globalfunc		(true, PlayerPostThink);
	globalfunc		(true, ClientKill);
	globalfunc		(true, ClientConnect);
	globalfunc		(true, PutClientInServer);
	globalfunc		(true, ClientDisconnect);
	globalfunc		(false, SetNewParms);
	globalfunc		(false, SetChangeParms);

	pr_nqglobal_struct->trace_surfaceflags = &writable;
	pr_nqglobal_struct->trace_endcontents = &writable;

	for (i = 0; i < 16; i++)
		spawnparamglobals[i] = (float*)((char*)VM_MemoryBase(q1qvm)+(long)(&gd->global->parm1 + i));
	for (; i < NUM_SPAWN_PARMS; i++)
		spawnparamglobals[i] = NULL;


	sv.edicts = EDICT_NUM(svprogfuncs, 0);

	return true;
}




void Q1QVM_ClientConnect(client_t *cl)
{
	if (cl->edict->v->netname)
	{
		strcpy(cl->namebuf, cl->name);
		cl->name = Q1QVMPF_StringToNative(svprogfuncs, cl->edict->v->netname);
		//FIXME: check this pointer
		strcpy(cl->name, cl->namebuf);
	}
	// call the spawn function
	pr_global_struct->time = sv.physicstime;
	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, cl->edict);
	VM_Call(q1qvm, GAME_CLIENT_CONNECT, cl->spectator);

	// actually spawn the player
	pr_global_struct->time = sv.physicstime;
	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, cl->edict);
	VM_Call(q1qvm, GAME_PUT_CLIENT_IN_SERVER, cl->spectator);
}

qboolean Q1QVM_GameConsoleCommand(void)
{
	int oldself, oldother;
	if (!q1qvm)
		return false;

	//FIXME: if an rcon command from someone on the server, mvdsv sets self to match the ip of that player
	//this is not required (broken by proxies anyway) but is a nice handy feature
	
	pr_global_struct->time = sv.physicstime;
	oldself = pr_global_struct->self;	//these are usually useless
	oldother = pr_global_struct->other;	//but its possible that someone makes a mod that depends on the 'mod' command working via redirectcmd+co
						//this at least matches mvdsv
	pr_global_struct->self = 0;
	pr_global_struct->other = 0;

	VM_Call(q1qvm, GAME_CONSOLE_COMMAND);	//mod uses Cmd_Argv+co to get args

	pr_global_struct->self = oldself;
	pr_global_struct->other = oldother;
	return true;
}

qboolean Q1QVM_ClientSay(edict_t *player, qboolean team)
{
	qboolean washandled;
	if (!q1qvm)
		return false;

	SV_EndRedirect();

	pr_global_struct->time = sv.physicstime;
	pr_global_struct->self = Q1QVMPF_EdictToProgs(svprogfuncs, player);
	washandled = VM_Call(q1qvm, GAME_CLIENT_SAY, team);

	SV_BeginRedirect(RD_CLIENT, host_client->language);	//put it back to how we expect it was. *shudder*

	return washandled;
}

qboolean Q1QVM_UserInfoChanged(edict_t *player)
{
	if (!q1qvm)
		return false;

	pr_global_struct->time = sv.physicstime;
	pr_global_struct->self = Q1QVMPF_EdictToProgs(svprogfuncs, player);
	return VM_Call(q1qvm, GAME_CLIENT_USERINFO_CHANGED);
}

void Q1QVM_PlayerPreThink(void)
{
	VM_Call(q1qvm, GAME_CLIENT_PRETHINK, host_client->spectator);
}

void Q1QVM_RunPlayerThink(void)
{
	VM_Call(q1qvm, GAME_EDICT_THINK);
	VM_Call(q1qvm, GAME_CLIENT_THINK, host_client->spectator);
}

void Q1QVM_PostThink(void)
{
	VM_Call(q1qvm, GAME_CLIENT_POSTTHINK, host_client->spectator);
}

void Q1QVM_StartFrame(void)
{
	VM_Call(q1qvm, GAME_START_FRAME, (int)(sv.time*1000));
}

void Q1QVM_Touch(void)
{
	VM_Call(q1qvm, GAME_EDICT_TOUCH);
}

void Q1QVM_Think(void)
{
	VM_Call(q1qvm, GAME_EDICT_THINK);
}

void Q1QVM_Blocked(void)
{
	VM_Call(q1qvm, GAME_EDICT_BLOCKED);
}

void Q1QVM_SetNewParms(void)
{
	VM_Call(q1qvm, GAME_SETNEWPARMS);
}

void Q1QVM_SetChangeParms(void)
{
	VM_Call(q1qvm, GAME_SETCHANGEPARMS);
}

void Q1QVM_ClientCommand(void)
{
	VM_Call(q1qvm, GAME_CLIENT_COMMAND);
}

void Q1QVM_DropClient(client_t *cl)
{
	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, cl->edict);
	VM_Call(q1qvm, GAME_CLIENT_DISCONNECT);
}

void Q1QVM_ChainMoved(void)
{
}
void Q1QVM_EndFrame(void)
{
}

#endif
