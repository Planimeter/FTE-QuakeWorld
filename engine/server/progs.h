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

#define QCLIB	//as opposed to standard qc stuff. One or other. All references+changes were by DMW unless specified otherwise. Starting 1/10/02

#define MAX_PROGS 64
#define MAXADDONS 16

#define NewGetEdictFieldValue GetEdictFieldValue
void Q_SetProgsParms(qboolean forcompiler);
void PR_Deinit(void);
void PR_LoadGlabalStruct(void);
void Q_InitProgs(void);
void PR_RegisterSVBuiltins(void);
void PR_RegisterFields(void);
void PR_Init(void);
void ED_Spawned (struct edict_s *ent);
qboolean PR_KrimzonParseCommand(char *s);
qboolean PR_UserCmd(char *cmd);
qboolean PR_ConsoleCmd(void);

void PR_RunThreads(void);


#define PR_MAINPROGS 0	//this is a constant that should really be phased out. But seeing as QCLIB requires some sort of master progs due to extern funcs...
	//maybe go through looking for extern funcs, and remember which were not allocated. It would then be a first come gets priority. Not too bad I supppose.

#include "progtype.h"
#include "progdefs.h"

extern int compileactive;

typedef enum {PROG_NONE, PROG_QW, PROG_NQ, PROG_H2, PROG_UNKNOWN} progstype_t;
extern progstype_t progstype;
                                 

//extern globalvars_t *glob0;

extern int pr_edict_size;


//extern progparms_t progparms;

//extern progsnum_t mainprogs;

#define	MAX_ENT_LEAFS	16
typedef struct edict_s
{
	qboolean	isfree;
	float		freetime; // sv.time when the object was freed
	int			entnum;
	qboolean	readonly;	//world
	link_t		area;				// linked to a division node or leaf

	int solidtype;
	
	int			num_leafs;
	short		leafnums[MAX_ENT_LEAFS];
	int areanum;	//q2bsp
	int areanum2;	//q2bsp
	int headnode;	//q2bsp

	entity_state_t	baseline;

	unsigned short tagent;
	unsigned short tagindex;

	entvars_t	v;					// C exported fields from progs
// other fields from progs come immediately after
} edict_t;
  


#include "progslib.h"

#undef pr_global_struct
//#define pr_nqglobal_struct *((nqglobalvars_t*)pr_globals)
#define pr_global_struct *pr_nqglobal_struct

extern nqglobalvars_t *pr_nqglobal_struct;

extern progfuncs_t *svprogfuncs;	//instance
extern progparms_t svprogparms;
extern progsnum_t svmainprogs;
extern progsnum_t clmainprogs;
#define	EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,edict_t,area)
#define	Q2EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,q2edict_t,area)

extern func_t SpectatorConnect;
extern func_t SpectatorThink;
extern func_t SpectatorDisconnect;

extern func_t SV_PlayerPhysicsQC;
extern func_t EndFrameQC;

qboolean PR_QCChat(char *text, int say_type);

void PR_ClientUserInfoChanged(char *name, char *oldivalue, char *newvalue);
void PR_LocalInfoChanged(char *name, char *oldivalue, char *newvalue);
