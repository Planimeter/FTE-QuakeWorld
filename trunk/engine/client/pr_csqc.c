#include "quakedef.h"

#ifdef CSQC_DAT

#ifdef RGLQUAKE
#include "glquake.h"	//evil to include this
#endif

static progfuncs_t *csqcprogs;

static unsigned int csqcchecksum;
static qboolean csqcwantskeys;

cvar_t	pr_csmaxedicts = {"pr_csmaxedicts", "3072"};
cvar_t	cl_csqcdebug = {"cl_csqcdebug", "0"};	//prints entity numbers which arrive (so I can tell people not to apply it to players...)

//If I do it like this, I'll never forget to register something...
#define csqcglobals	\
	globalfunction(init_function,		"CSQC_Init");	\
	globalfunction(shutdown_function,	"CSQC_Shutdown");	\
	globalfunction(draw_function,		"CSQC_UpdateView");	\
	globalfunction(parse_stuffcmd,		"CSQC_Parse_StuffCmd");	\
	globalfunction(parse_centerprint,	"CSQC_Parse_CenterPrint");	\
	globalfunction(input_event,			"CSQC_InputEvent");	\
	globalfunction(console_command,		"CSQC_ConsoleCommand");	\
	\
	globalfunction(ent_update,			"CSQC_Ent_Update");	\
	globalfunction(ent_remove,			"CSQC_Ent_Remove");	\
	\
	/*These are pointers to the csqc's globals.*/	\
	globalfloat(time,					"time");				/*float		Written before entering most qc functions*/	\
	globalentity(self,					"self");				/*entity	Written before entering most qc functions*/	\
	\
	globalfloat(maxclients,				"maxclients");			/*float		*/	\
	\
	globalvector(forward,				"v_forward");			/*vector	written by anglevectors*/	\
	globalvector(right,					"v_right");				/*vector	written by anglevectors*/	\
	globalvector(up,					"v_up");				/*vector	written by anglevectors*/	\
	\
	globalfloat(trace_allsolid,			"trace_allsolid");		/*bool		written by traceline*/	\
	globalfloat(trace_startsolid,		"trace_startsolid");	/*bool		written by traceline*/	\
	globalfloat(trace_fraction,			"trace_fraction");		/*float		written by traceline*/	\
	globalfloat(trace_inwater,			"trace_inwater");		/*bool		written by traceline*/	\
	globalfloat(trace_inopen,			"trace_inopen");		/*bool		written by traceline*/	\
	globalvector(trace_endpos,			"trace_endpos");		/*vector	written by traceline*/	\
	globalvector(trace_plane_normal,	"trace_plane_normal");	/*vector	written by traceline*/	\
	globalfloat(trace_plane_dist,		"trace_plane_dist");	/*float		written by traceline*/	\
	globalentity(trace_ent,				"trace_ent");			/*entity	written by traceline*/	\
	\
	globalfloat(clientcommandframe,		"clientcommandframe");	\
	globalfloat(servercommandframe,		"servercommandframe");	\
	\
	globalfloat(player_localentnum,		"player_localentnum");	/*float		the entity number of the local player*/	\
	\
	globalvector(pmove_org,				"pmove_org");			\
	globalvector(pmove_vel,				"pmove_vel");			\
	globalvector(pmove_mins,			"pmove_mins");			\
	globalvector(pmove_maxs,			"pmove_maxs");			\
	globalfloat(input_timelength,		"input_timelength");	\
	globalvector(input_angles,			"input_angles");		\
	globalvector(input_movevalues,		"input_movevalues");	\
	globalfloat(input_buttons,			"input_buttons");		\
	\
	globalfloat(movevar_gravity,		"movevar_gravity");		\
	globalfloat(movevar_stopspeed,		"movevar_stopspeed");	\
	globalfloat(movevar_maxspeed,		"movevar_maxspeed");	\
	globalfloat(movevar_spectatormaxspeed,"movevar_spectatormaxspeed");	\
	globalfloat(movevar_accelerate,		"movevar_accelerate");		\
	globalfloat(movevar_airaccelerate,	"movevar_airaccelerate");	\
	globalfloat(movevar_wateraccelerate,"movevar_wateraccelerate");	\
	globalfloat(movevar_friction,		"movevar_friction");		\
	globalfloat(movevar_waterfriction,	"movevar_waterfriction");	\
	globalfloat(movevar_entgravity,		"movevar_entgravity");		\


typedef struct {
#define globalfloat(name,qcname) float *name
#define globalvector(name,qcname) float *name
#define globalentity(name,qcname) int *name
#define globalstring(name,qcname) string_t *name
#define globalfunction(name,qcname) func_t name
//These are the functions the engine will call to, found by name.

	csqcglobals

#undef globalfloat
#undef globalvector
#undef globalentity
#undef globalstring
#undef globalfunction
} csqcglobals_t;
static csqcglobals_t csqcg;

#define plnum 0


static void CSQC_FindGlobals(void)
{
#define globalfloat(name,qcname) csqcg.name = (float*)PR_FindGlobal(csqcprogs, qcname, 0);
#define globalvector(name,qcname) csqcg.name = (float*)PR_FindGlobal(csqcprogs, qcname, 0);
#define globalentity(name,qcname) csqcg.name = (int*)PR_FindGlobal(csqcprogs, qcname, 0);
#define globalstring(name,qcname) csqcg.name = (string_t*)PR_FindGlobal(csqcprogs, qcname, 0);
#define globalfunction(name,qcname) csqcg.name = PR_FindFunction(csqcprogs,qcname,PR_ANY);

	csqcglobals

#undef globalfloat
#undef globalvector
#undef globalentity
#undef globalstring
#undef globalfunction

	if (csqcg.time)
		*csqcg.time = Sys_DoubleTime();

	if (csqcg.player_localentnum)
		*csqcg.player_localentnum = cl.playernum[plnum]+1;

	if (csqcg.maxclients)
		*csqcg.maxclients = MAX_CLIENTS;
}



//this is the list for all the csqc fields.
//(the #define is so the list always matches the ones pulled out)
#define csqcfields	\
	fieldfloat(entnum);		\
	fieldfloat(modelindex);	\
	fieldvector(origin);	\
	fieldvector(angles);	\
	fieldfloat(alpha);		/*transparency*/	\
	fieldfloat(scale);		/*model scale*/		\
	fieldfloat(fatness);	/*expand models X units along thier normals.*/	\
	fieldfloat(skin);		\
	fieldfloat(colormap);	\
	fieldfloat(frame);		\
	fieldfloat(oldframe);	\
	fieldfloat(lerpfrac);	\
	fieldfloat(renderflags);\
							\
	fieldfloat(drawmask);	/*So that the qc can specify all rockets at once or all bannanas at once*/	\
	fieldfunction(predraw);	/*If present, is called just before it's drawn.*/	\
							\
	fieldstring(model);		\
	fieldfloat(ideal_yaw);	\
	fieldfloat(ideal_pitch);\
	fieldfloat(yaw_speed);	\
	fieldfloat(pitch_speed);\
							\
	fieldentity(chain);		\
							\
	fieldvector(solid);		\
	fieldvector(mins);		\
	fieldvector(maxs);		\
	fieldvector(absmins);	\
	fieldvector(absmaxs);	\
	fieldfloat(hull);		/*(FTE_PEXT_HEXEN2)*/


//note: doesn't even have to match the clprogs.dat :)
typedef struct {
#define fieldfloat(name) float name
#define fieldvector(name) vec3_t name
#define fieldentity(name) int name
#define fieldstring(name) string_t name
#define fieldfunction(name) func_t name
csqcfields
#undef fieldfloat
#undef fieldvector
#undef fieldentity
#undef fieldstring
#undef fieldfunction
} csqcentvars_t;

typedef struct csqcedict_s
{
	qboolean	isfree;
	float		freetime; // sv.time when the object was freed
	int			entnum;
	qboolean	readonly;	//world
	csqcentvars_t	*v;

	//add whatever you wish here
} csqcedict_t;

static csqcedict_t *csqc_edicts;	//consider this 'world'


static void CSQC_InitFields(void)
{	//CHANGING THIS FUNCTION REQUIRES CHANGES TO csqcentvars_t
#define fieldfloat(name) PR_RegisterFieldVar(csqcprogs, ev_float, #name, (int)&((csqcentvars_t*)0)->name, -1)
#define fieldvector(name) PR_RegisterFieldVar(csqcprogs, ev_vector, #name, (int)&((csqcentvars_t*)0)->name, -1)
#define fieldentity(name) PR_RegisterFieldVar(csqcprogs, ev_entity, #name, (int)&((csqcentvars_t*)0)->name, -1)
#define fieldstring(name) PR_RegisterFieldVar(csqcprogs, ev_string, #name, (int)&((csqcentvars_t*)0)->name, -1)
#define fieldfunction(name) PR_RegisterFieldVar(csqcprogs, ev_function, #name, (int)&((csqcentvars_t*)0)->name, -1)
csqcfields	//any *64->int32 casts are erroneous, it's biased off NULL.
#undef fieldfloat
#undef fieldvector
#undef fieldentity
#undef fieldstring
#undef fieldfunction
}

static csqcedict_t *csqcent[MAX_EDICTS];

#define	RETURN_SSTRING(s) (((string_t *)pr_globals)[OFS_RETURN] = PR_SetString(prinst, s))	//static - exe will not change it.
char *PF_TempStr(progfuncs_t *prinst);

static int csqcentsize;

//pr_cmds.c builtins that need to be moved to a common.
void VARGS PR_BIError(progfuncs_t *progfuncs, char *format, ...);
void PF_cvar_string (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_cvar_set (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_print (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_dprint (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_error (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_rint (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_floor (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_ceil (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_Tokenize  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_ArgV  (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_FindString (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_FindFloat (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_nextent (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_randomvec (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_Sin (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_Cos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_Sqrt (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_bound (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_strlen(progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_strcat (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_ftos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_fabs (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_vtos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_etos (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_stof (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_mod (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_substring (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_stov (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_dupstring(progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_forgetstring(progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_Spawn (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_min (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_max (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_registercvar (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_pow (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_chr2str (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_localcmd (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_random (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_fopen (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_fclose (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_fputs (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_fgets (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_normalize (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_vlen (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_vectoyaw (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_vectoangles (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_FindFlags (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_findchain (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_findchainfloat (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_findchainflags (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_coredump (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_traceon (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_traceoff (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_eprint (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PF_registercvar (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PF_strstrofs (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_str2chr (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_chr2str (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_strconv (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_infoadd (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_infoget (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_strncmp (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_strcasecmp (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_strncasecmp (progfuncs_t *prinst, struct globalvars_s *pr_globals);

//these functions are from pr_menu.dat
void PF_CL_is_cached_pic (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_precache_pic (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_free_pic (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_drawcharacter (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_drawstring (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_drawpic (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_drawline (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_drawfill (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_drawsetcliparea (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_drawresetcliparea (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_CL_drawgetimagesize (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void PF_cl_keynumtostring (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_cl_stringtokeynum(progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_cl_getkeybind (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void search_close_progs(progfuncs_t *prinst, qboolean complain);
void PF_search_begin (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_search_end (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_search_getsize (progfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_search_getfilename (progfuncs_t *prinst, struct globalvars_s *pr_globals);


#define MAXTEMPBUFFERLEN	1024

void PF_fclose_progs (progfuncs_t *prinst);
char *PF_VarString (progfuncs_t *prinst, int	first, struct globalvars_s *pr_globals);







void CL_CS_LinkEdict(csqcedict_t *ent)
{
	//FIXME: use some sort of area grid ?
	VectorAdd(ent->v->origin, ent->v->mins, ent->v->absmins);
	VectorAdd(ent->v->origin, ent->v->maxs, ent->v->absmaxs);
}

//FIXME: Not fully functional
trace_t CL_Move(vec3_t v1, vec3_t mins, vec3_t maxs, vec3_t v2, float nomonsters, csqcedict_t *passedict)
{
	int e;
	csqcedict_t *ed;
	vec3_t minb, maxb;
	hull_t *hull;

	trace_t	trace, trace2;

	memset(&trace, 0, sizeof(trace));
	trace.fraction = 1;
	cl.worldmodel->hulls->funcs.RecursiveHullCheck (cl.worldmodel->hulls, 0, 0, 1, v1, v2, &trace);

//why use trace.endpos instead?
//so that if we hit a wall early, we don't have a box covering the whole world because of a shotgun trace.
	minb[0] = ((v1[0] < trace.endpos[0])?v1[0]:trace.endpos[0]) - mins[0]-1;
	minb[1] = ((v1[1] < trace.endpos[1])?v1[1]:trace.endpos[1]) - mins[1]-1;
	minb[2] = ((v1[2] < trace.endpos[2])?v1[2]:trace.endpos[2]) - mins[2]-1;
	maxb[0] = ((v1[0] > trace.endpos[0])?v1[0]:trace.endpos[0]) + maxs[0]+1;
	maxb[1] = ((v1[1] > trace.endpos[1])?v1[1]:trace.endpos[1]) + maxs[1]+1;
	maxb[2] = ((v1[2] > trace.endpos[2])?v1[2]:trace.endpos[2]) + maxs[2]+1;
/*
	for (e=1; e < *csqcprogs->parms->sv_num_edicts; e++)
	{
		ed = (void*)EDICT_NUM(csqcprogs, e);
		if (ed->isfree)
			continue;	//can't collide
		if (!ed->v->solid)
			continue;
		if (ed->v->absmaxs[0] < minb[0] ||
			ed->v->absmaxs[1] < minb[1] ||
			ed->v->absmaxs[2] < minb[2] ||
			ed->v->absmins[0] > maxb[0] ||
			ed->v->absmins[1] > maxb[1] ||
			ed->v->absmins[2] > maxb[2])
			continue;

		hull = CL_HullForEntity(ed);
		memset(&trace, 0, sizeof(trace));
		trace.fraction = 1;
		TransformedHullCheck(hull, v1, v2, &trace2, ed->v->angles);
		trace2.ent = (void*)ed;
		if (trace2.fraction < trace.fraction)
			trace = trace2;
	}
*/
	return trace;
}




static void PF_cs_remove (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t *ed;
	
	ed = (csqcedict_t*)G_EDICT(prinst, OFS_PARM0);

	if (ed->isfree)
	{
		Con_DPrintf("CSQC Tried removing free entity\n");
		return;
	}

	ED_Free (prinst, (void*)ed);
}

static void PF_cvar (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	cvar_t	*var;
	char	*str;

	str = PR_GetStringOfs(prinst, OFS_PARM0);
	{
		var = Cvar_Get(str, "", 0, "csqc cvars");
		if (var)
			G_FLOAT(OFS_RETURN) = var->value;
		else
			G_FLOAT(OFS_RETURN) = 0;
	}
}

//too specific to the prinst's builtins.
static void PF_Fixme (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	Con_Printf("\n");

	prinst->RunError(prinst, "\nBuiltin %i not implemented.\nCSQC is not compatable.", prinst->lastcalledbuiltinnumber);
	PR_BIError (prinst, "bulitin not implemented");
}
static void PF_NoCSQC (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	Con_Printf("\n");

	prinst->RunError(prinst, "\nBuiltin %i does not make sense in csqc.\nCSQC is not compatable.", prinst->lastcalledbuiltinnumber);
	PR_BIError (prinst, "bulitin not implemented");
}

static void PF_cl_cprint (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *str = PF_VarString(prinst, 0, pr_globals);
	SCR_CenterPrint(0, str);
}

static void PF_cs_makevectors (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	if (!csqcg.forward || !csqcg.right || !csqcg.up)
		Host_EndGame("PF_makevectors: one of v_forward, v_right or v_up was not defined\n");
	AngleVectors (G_VECTOR(OFS_PARM0), csqcg.forward, csqcg.right, csqcg.up);
}
/*
void QuaternainToAngleMatrix(float *quat, vec3_t *mat)
{
	float xx      = quat[0] * quat[0];
    float xy      = quat[0] * quat[1];
    float xz      = quat[0] * quat[2];
    float xw      = quat[0] * quat[3];
    float yy      = quat[1] * quat[1];
    float yz      = quat[1] * quat[2];
    float yw      = quat[1] * quat[3];
    float zz      = quat[2] * quat[2];
    float zw      = quat[2] * quat[3];
    mat[0][0]  = 1 - 2 * ( yy + zz );
    mat[0][1]  =     2 * ( xy - zw );
    mat[0][2]  =     2 * ( xz + yw );
    mat[1][0]  =     2 * ( xy + zw );
    mat[1][1]  = 1 - 2 * ( xx + zz );
    mat[1][2]  =     2 * ( yz - xw );
    mat[2][0]  =     2 * ( xz - yw );
    mat[2][1]  =     2 * ( yz + xw );
    mat[2][2] = 1 - 2 * ( xx + yy );
}

void quaternion_multiply(float *a, float *b, float *c)
{
#define x1 a[0]
#define y1 a[1]
#define z1 a[2]
#define w1 a[3]
#define x2 b[0]
#define y2 b[1]
#define z2 b[2]
#define w2 b[3]
	c[0] = w1*x2 + x1*w2 + y1*z2 - z1*y2;
	c[1] = w1*y2 + y1*w2 + z1*x2 - x1*z2;
	c[2] = w1*z2 + z1*w2 + x1*y2 - y1*x2;
	c[3] = w1*w2 - x1*x2 - y1*y2 - z1*z2;
}

void quaternion_rotation(float pitch, float roll, float yaw, float angle, float *quat)
{
	float sin_a, cos_a;

	sin_a = sin( angle / 360 );
    cos_a = cos( angle / 360 );
    quat[0]    = pitch	* sin_a;
    quat[1]    = yaw	* sin_a;
    quat[2]    = roll	* sin_a;
    quat[3]    = cos_a;
}

void EularToQuaternian(vec3_t angles, float *quat)
{
  float x[4] = {sin(angles[2]/360), 0, 0, cos(angles[2]/360)};
  float y[4] = {0, sin(angles[1]/360), 0, cos(angles[1]/360)};
  float z[4] = {0, 0, sin(angles[0]/360), cos(angles[0]/360)};
  float t[4];
  quaternion_multiply(x, y, t);
  quaternion_multiply(t, z, quat);
}
*/
#define CSQCRF_VIEWMODEL		1 //Not drawn in mirrors
#define CSQCRF_EXTERNALMODEL	2 //drawn ONLY in mirrors
#define CSQCRF_DEPTHHACK		4 //fun depthhack
#define CSQCRF_ADDATIVE			8 //add instead of blend
#define CSQCRF_USEAXIS			16 //use v_forward/v_right/v_up as an axis/matrix - predraw is needed to use this properly

void PF_R_AddEntity(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t *in = (void*)G_EDICT(prinst, OFS_PARM0);
	entity_t ent;
	int i;
	model_t *model;

	if (in->v->predraw)
	{
		int oldself = *csqcg.self;
		*csqcg.self = EDICT_TO_PROG(prinst, (void*)in);
		PR_ExecuteProgram(prinst, in->v->predraw);
		*csqcg.self = oldself;
	}

	i = in->v->modelindex;
	if (i == 0)
		return;
	else if (i > 0 && i < MAX_MODELS)
		model = cl.model_precache[i];
	else if (i < 0 && i > -MAX_CSQCMODELS)
		model = cl.model_csqcprecache[-i];
	else
		return; //there might be other ent types later as an extension that stop this.

	memset(&ent, 0, sizeof(ent));
	ent.model = model;

	if (!ent.model)
	{
		Con_Printf("PF_R_AddEntity: model wasn't precached!\n");
		return;
	}

	if (in->v->renderflags)
	{
		i = in->v->renderflags;
		if (i & CSQCRF_VIEWMODEL)
			ent.flags |= Q2RF_WEAPONMODEL;
		if (i & CSQCRF_EXTERNALMODEL)
			ent.flags |= Q2RF_EXTERNALMODEL;
		if (i & CSQCRF_DEPTHHACK)
			ent.flags |= Q2RF_DEPTHHACK;
		if (i & CSQCRF_ADDATIVE)
			ent.flags |= Q2RF_ADDATIVE;
		//CSQCRF_USEAXIS is below
	}
	
	ent.frame = in->v->frame;
	ent.oldframe = in->v->oldframe;
	ent.lerpfrac = in->v->lerpfrac;

	ent.angles[0] = in->v->angles[0];
	ent.angles[1] = in->v->angles[1];
	ent.angles[2] = in->v->angles[2];

	VectorCopy(in->v->origin, ent.origin);
	if ((int)in->v->renderflags & CSQCRF_USEAXIS)
	{
		VectorCopy(csqcg.forward, ent.axis[0]);
		VectorNegate(csqcg.right, ent.axis[1]);
		VectorCopy(csqcg.up, ent.axis[2]);
	}
	else
	{
		AngleVectors(ent.angles, ent.axis[0], ent.axis[1], ent.axis[2]);
		VectorInverse(ent.axis[1]);
	}

	ent.alpha = in->v->alpha;
	ent.scale = in->v->scale;
	ent.skinnum = in->v->skin;
	ent.fatness = in->v->fatness;

	V_AddEntity(&ent);

/*
	{
		float a[4];
		float q[4];
		float r[4];
		EularToQuaternian(ent.angles, a);

		QuaternainToAngleMatrix(a, ent.axis);
		ent.origin[0] += 16;
		V_AddEntity(&ent);

		quaternion_rotation(0, 0, 1, cl.time*360, r);
		quaternion_multiply(a, r, q);
		QuaternainToAngleMatrix(q, ent.axis);
		ent.origin[0] -= 32;
		ent.angles[1] = cl.time;
		V_AddEntity(&ent);
	}
*/
}

static void PF_R_AddDynamicLight(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *org = G_VECTOR(OFS_PARM0);
	float radius = G_FLOAT(OFS_PARM1);
	float *rgb = G_VECTOR(OFS_PARM2);
	V_AddLight(org, radius, rgb[0]/5, rgb[1]/5, rgb[2]/5);
}

#define MASK_ENGINE 1
static void PF_R_AddEntityMask(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int mask = G_FLOAT(OFS_PARM0);
	csqcedict_t *ent;
	int e;

	for (e=1; e < *prinst->parms->sv_num_edicts; e++)
	{
		ent = (void*)EDICT_NUM(prinst, e);
		if (ent->isfree)
			continue;

		if ((int)ent->v->drawmask & mask)
		{
			G_INT(OFS_PARM0) = EDICT_TO_PROG(prinst, (void*)ent);
			PF_R_AddEntity(prinst, pr_globals);
		}
	}

	if (mask & MASK_ENGINE && cl.worldmodel)
	{
		CL_LinkViewModel ();
		CL_LinkPlayers ();
		CL_LinkPacketEntities ();
		CL_LinkProjectiles ();
		CL_UpdateTEnts ();
	}
}

//float CalcFov (float fov_x, float width, float height);
//clear scene, and set up the default stuff.
static void PF_R_ClearScene (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	extern frame_t		*view_frame;
	extern player_state_t		*view_message;

	CL_DecayLights ();

	if (cl.worldmodel)
	{
		//work out which packet entities are solid
		CL_SetSolidEntities ();

		// Set up prediction for other players
		CL_SetUpPlayerPrediction(false);

		// do client side motion prediction
		CL_PredictMove ();

		// Set up prediction for other players
		CL_SetUpPlayerPrediction(true);
	}

	CL_SwapEntityLists();

	view_frame = NULL;//&cl.frames[cls.netchan.incoming_sequence & UPDATE_MASK];
	view_message = NULL;//&view_frame->playerstate[cl.playernum[plnum]];
	V_CalcRefdef(0);	//set up the defaults (for player 0)
	/*
	VectorCopy(cl.simangles[0], r_refdef.viewangles);
	VectorCopy(cl.simorg[0], r_refdef.vieworg);
	r_refdef.flags = 0;

	r_refdef.vrect.x = 0;
	r_refdef.vrect.y = 0;
	r_refdef.vrect.width = vid.width;
	r_refdef.vrect.height = vid.height;

	r_refdef.fov_x = scr_fov.value;
	r_refdef.fov_y = CalcFov (r_refdef.fov_x, r_refdef.vrect.width, r_refdef.vrect.height);
	*/
}

static void PF_R_SetViewFlag(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *s = PR_GetStringOfs(prinst, OFS_PARM0);
	float *p = G_VECTOR(OFS_PARM1);

	G_FLOAT(OFS_RETURN) = 1;
	switch(*s)
	{
	case 'F':
		if (!strcmp(s, "FOV"))	//set both fov numbers
		{
			r_refdef.fov_x = p[0];
			r_refdef.fov_y = p[1];
			return;
		}
		if (!strcmp(s, "FOV_X"))
		{
			r_refdef.fov_x = *p;
			return;
		}
		if (!strcmp(s, "FOV_Y"))
		{
			r_refdef.fov_y = *p;
			return;
		}
		break;
	case 'O':
		if (!strcmp(s, "ORIGIN"))
		{
			VectorCopy(p, r_refdef.vieworg);
			return;
		}
		if (!strcmp(s, "ORIGIN_X"))
		{
			r_refdef.vieworg[0] = *p;
			return;
		}
		if (!strcmp(s, "ORIGIN_Y"))
		{
			r_refdef.vieworg[1] = *p;
			return;
		}
		if (!strcmp(s, "ORIGIN_Z"))
		{
			r_refdef.vieworg[2] = *p;
			return;
		}
		break;
		
	case 'A':
		if (!strcmp(s, "ANGLES"))
		{
			VectorCopy(p, r_refdef.viewangles);
			return;
		}
		if (!strcmp(s, "ANGLES_X"))
		{
			r_refdef.viewangles[0] = *p;
			return;
		}
		if (!strcmp(s, "ANGLES_Y"))
		{
			r_refdef.viewangles[1] = *p;
			return;
		}
		if (!strcmp(s, "ANGLES_Z"))
		{
			r_refdef.viewangles[2] = *p;
			return;
		}
		break;
		
	case 'W':
		if (!strcmp(s, "WIDTH"))
		{
			r_refdef.vrect.width = *p;
			return;
		}
		break;
	case 'H':
		if (!strcmp(s, "HEIGHT"))
		{
			r_refdef.vrect.height = *p;
			return;
		}
		break;
	case 'S':
		if (!strcmp(s, "SIZE"))
		{
			r_refdef.vrect.width = p[0];
			r_refdef.vrect.height = p[1];
			return;
		}
		break;
	case 'M':
		if (!strcmp(s, "MIN_X"))
		{
			r_refdef.vrect.x = *p;
			return;
		}
		if (!strcmp(s, "MIN_Y"))
		{
			r_refdef.vrect.y = *p;
			return;
		}
		if (!strcmp(s, "MIN"))
		{
			r_refdef.vrect.x = p[0];
			r_refdef.vrect.y = p[1];
			return;
		}
		break;
	default:
		break;
	}
	Con_DPrintf("SetViewFlag: %s not recognised\n", s);

	G_FLOAT(OFS_RETURN) = 0;
}

static void PF_R_RenderScene(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	if (cl.worldmodel)
		R_PushDlights ();
/*
	if (cl_csqcdebug.value)
	Con_Printf("%f %f %f\n",	r_refdef.vieworg[0],
								r_refdef.vieworg[1],
								r_refdef.vieworg[2]);
	*/

#ifdef RGLQUAKE
	if (qrenderer == QR_OPENGL)
	{
		gl_ztrickdisabled|=16;
		qglDisable(GL_ALPHA_TEST);
		qglDisable(GL_BLEND);
	}
#endif

	VectorCopy (r_refdef.vieworg, cl.viewent[0].origin);
	CalcGunAngle(0);

	R_RenderView();

#ifdef RGLQUAKE
	if (qrenderer == QR_OPENGL)
	{
		gl_ztrickdisabled&=~16;
		GL_Set2D ();
		qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		GL_TexEnv(GL_MODULATE);
	}
#endif

	#ifdef RGLQUAKE
	if (qrenderer == QR_OPENGL)
	{
		qglDisable(GL_ALPHA_TEST);
		qglEnable(GL_BLEND);
	}
#endif

	vid.recalc_refdef = 1;
}

static void PF_cs_getstatf(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int stnum = G_FLOAT(OFS_PARM0);
	float val = *(float*)&cl.stats[plnum][stnum];	//copy float into the stat
	G_FLOAT(OFS_RETURN) = val;
}
static void PF_cs_getstati(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{	//convert an int stat into a qc float.

	int stnum = G_FLOAT(OFS_PARM0);
	int val = cl.stats[plnum][stnum];
	if (*prinst->callargc > 1)
	{
		int first, count;
		first = G_FLOAT(OFS_PARM1);
		count = G_FLOAT(OFS_PARM2);
		G_FLOAT(OFS_RETURN) = (((unsigned int)val)&(((1<<count)-1)<<first))>>first;
	}
	else
		G_FLOAT(OFS_RETURN) = val;
}
static void PF_cs_getstats(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int stnum = G_FLOAT(OFS_PARM0);
	char *out;

	out = PF_TempStr(prinst);

	//the network protocol byteswaps

	((unsigned int*)out)[0] = LittleLong(cl.stats[0][stnum+0]);
	((unsigned int*)out)[1] = LittleLong(cl.stats[0][stnum+1]);
	((unsigned int*)out)[2] = LittleLong(cl.stats[0][stnum+2]);
	((unsigned int*)out)[3] = LittleLong(cl.stats[0][stnum+3]);
	((unsigned int*)out)[4] = 0;	//make sure it's null terminated

	RETURN_SSTRING(out);
}

static void PF_cs_SetOrigin(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t *ent = (void*)G_EDICT(prinst, OFS_PARM0);
	float *org = G_VECTOR(OFS_PARM1);

	VectorCopy(org, ent->v->origin);

	CL_CS_LinkEdict(ent);
}

static void PF_cs_SetSize(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t *ent = (void*)G_EDICT(prinst, OFS_PARM0);
	float *mins = G_VECTOR(OFS_PARM1);
	float *maxs = G_VECTOR(OFS_PARM1);

	VectorCopy(mins, ent->v->mins);
	VectorCopy(maxs, ent->v->maxs);

	CL_CS_LinkEdict(ent);
}

static void PF_cs_traceline(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	*v1, *v2, *mins, *maxs;
	trace_t	trace;
	int		nomonsters;
	csqcedict_t	*ent;
	int savedhull;

	v1 = G_VECTOR(OFS_PARM0);
	v2 = G_VECTOR(OFS_PARM1);
	nomonsters = G_FLOAT(OFS_PARM2);
	ent = (csqcedict_t*)G_EDICT(prinst, OFS_PARM3);

//	if (*prinst->callargc == 6)
//	{
//		mins = G_VECTOR(OFS_PARM4);
//		maxs = G_VECTOR(OFS_PARM5);
//	}
//	else
	{
		mins = vec3_origin;
		maxs = vec3_origin;
	}

	savedhull = ent->v->hull;
	ent->v->hull = 0;
	trace = CL_Move (v1, mins, maxs, v2, nomonsters, ent);
	ent->v->hull = savedhull;
	
	*csqcg.trace_allsolid = trace.allsolid;
	*csqcg.trace_startsolid = trace.startsolid;
	*csqcg.trace_fraction = trace.fraction;
	*csqcg.trace_inwater = trace.inwater;
	*csqcg.trace_inopen = trace.inopen;
	VectorCopy (trace.endpos, csqcg.trace_endpos);
	VectorCopy (trace.plane.normal, csqcg.trace_plane_normal);
	*csqcg.trace_plane_dist =  trace.plane.dist;	
	if (trace.ent)
		*csqcg.trace_ent = EDICT_TO_PROG(prinst, (void*)trace.ent);
	else
		*csqcg.trace_ent = EDICT_TO_PROG(prinst, (void*)csqc_edicts);
}
static void PF_cs_tracebox(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	*v1, *v2, *mins, *maxs;
	trace_t	trace;
	int		nomonsters;
	csqcedict_t	*ent;
	int savedhull;

	v1 = G_VECTOR(OFS_PARM0);
	mins = G_VECTOR(OFS_PARM1);
	maxs = G_VECTOR(OFS_PARM2);
	v2 = G_VECTOR(OFS_PARM3);
	nomonsters = G_FLOAT(OFS_PARM4);
	ent = (csqcedict_t*)G_EDICT(prinst, OFS_PARM5);

	savedhull = ent->v->hull;
	ent->v->hull = 0;
	trace = CL_Move (v1, mins, maxs, v2, nomonsters, ent);
	ent->v->hull = savedhull;
	
	*csqcg.trace_allsolid = trace.allsolid;
	*csqcg.trace_startsolid = trace.startsolid;
	*csqcg.trace_fraction = trace.fraction;
	*csqcg.trace_inwater = trace.inwater;
	*csqcg.trace_inopen = trace.inopen;
	VectorCopy (trace.endpos, csqcg.trace_endpos);
	VectorCopy (trace.plane.normal, csqcg.trace_plane_normal);
	*csqcg.trace_plane_dist =  trace.plane.dist;	
	if (trace.ent)
		*csqcg.trace_ent = EDICT_TO_PROG(prinst, (void*)trace.ent);
	else
		*csqcg.trace_ent = EDICT_TO_PROG(prinst, (void*)csqc_edicts);
}

static void PF_cs_pointcontents(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float	*v;
	int cont;
	
	v = G_VECTOR(OFS_PARM0);

	cont = cl.worldmodel->hulls[0].funcs.HullPointContents(&cl.worldmodel->hulls[0], v);
	if (cont & FTECONTENTS_SOLID)
		G_FLOAT(OFS_RETURN) = Q1CONTENTS_SOLID;
	else if (cont & FTECONTENTS_SKY)
		G_FLOAT(OFS_RETURN) = Q1CONTENTS_SKY;
	else if (cont & FTECONTENTS_LAVA)
		G_FLOAT(OFS_RETURN) = Q1CONTENTS_LAVA;
	else if (cont & FTECONTENTS_SLIME)
		G_FLOAT(OFS_RETURN) = Q1CONTENTS_SLIME;
	else if (cont & FTECONTENTS_WATER)
		G_FLOAT(OFS_RETURN) = Q1CONTENTS_WATER;
	else
		G_FLOAT(OFS_RETURN) = Q1CONTENTS_EMPTY;
}

static int FindModel(char *name, int *free)
{
	int i;

	*free = 0;

	for (i = 1; i < MAX_CSQCMODELS; i++)
	{
		if (!*cl.model_csqcname[i])
		{
			*free = -i;
			break;
		}
		if (!strcmp(cl.model_csqcname[i], name))
			return -i;
	}
	for (i = 1; i < MAX_MODELS; i++)
	{
		if (!strcmp(cl.model_name[i], name))
			return i;
	}
	return 0;
}
static void PF_cs_SetModel(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t *ent = (void*)G_EDICT(prinst, OFS_PARM0);
	char *modelname = PR_GetStringOfs(prinst, OFS_PARM1);
	int freei;
	int modelindex = FindModel(modelname, &freei);

	if (!modelindex)
	{
		if (!freei)
			Host_EndGame("CSQC ran out of model slots\n");
		Con_DPrintf("Late caching model \"%s\"\n", modelname);
		Q_strncpyz(cl.model_csqcname[-freei], modelname, sizeof(cl.model_csqcname[-freei]));	//allocate a slot now
		modelindex = freei;

		cl.model_csqcprecache[-freei] = Mod_ForName(cl.model_csqcname[-freei], false);
	}

	ent->v->modelindex = modelindex;
	if (modelindex < 0)
		ent->v->model = PR_SetString(prinst, cl.model_csqcname[-modelindex]);
	else
		ent->v->model = PR_SetString(prinst, cl.model_name[modelindex]);
}
static void PF_cs_SetModelIndex(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t *ent = (void*)G_EDICT(prinst, OFS_PARM0);
	int modelindex = G_FLOAT(OFS_PARM1);

	ent->v->modelindex = modelindex;
	if (modelindex < 0)
		ent->v->model = PR_SetString(prinst, cl.model_csqcname[-modelindex]);
	else
		ent->v->model = PR_SetString(prinst, cl.model_name[modelindex]);
}
static void PF_cs_PrecacheModel(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int modelindex, freei;
	char *modelname = PR_GetStringOfs(prinst, OFS_PARM0);
	int i;

	for (i = 1; i < MAX_MODELS; i++)	//Make sure that the server specified model is loaded..
	{
		if (!*cl.model_name[i])
			break;
		if (!strcmp(cl.model_name[i], modelname))
		{
			cl.model_precache[i] = Mod_ForName(cl.model_name[i], false);
			break;
		}
	}

	modelindex = FindModel(modelname, &freei);	//now load it

	if (!modelindex)
	{
		if (!freei)
			Host_EndGame("CSQC ran out of model slots\n");
		Q_strncpyz(cl.model_csqcname[-freei], modelname, sizeof(cl.model_csqcname[-freei]));	//allocate a slot now
		modelindex = freei;

		cl.model_csqcprecache[-freei] = Mod_ForName(cl.model_csqcname[-freei], false);
	}
}
static void PF_cs_PrecacheSound(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *soundname = PR_GetStringOfs(prinst, OFS_PARM0);
	S_PrecacheSound(soundname);
}

static void PF_cs_ModelnameForIndex(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int modelindex = G_FLOAT(OFS_PARM0);

	if (modelindex < 0)
		G_INT(OFS_RETURN) = (int)PR_SetString(prinst, cl.model_csqcname[-modelindex]);
	else
		G_INT(OFS_RETURN) = (int)PR_SetString(prinst, cl.model_name[modelindex]);
}

static void PF_ReadByte(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = MSG_ReadByte();
}

static void PF_ReadChar(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = MSG_ReadChar();
}

static void PF_ReadShort(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = MSG_ReadShort();
}

static void PF_ReadLong(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = MSG_ReadLong();
}

static void PF_ReadCoord(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = MSG_ReadCoord();
}

static void PF_ReadString(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *str = PF_TempStr(prinst);
	char *read = MSG_ReadString();

	Q_strncpyz(str, read, MAXTEMPBUFFERLEN);
}

static void PF_ReadAngle(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = MSG_ReadAngle();
}


static void PF_objerror (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char	*s;
	edict_t	*ed;
	
	s = PF_VarString(prinst, 0, pr_globals);
/*	Con_Printf ("======OBJECT ERROR in %s:\n%s\n", PR_GetString(pr_xfunction->s_name),s);
*/	ed = PROG_TO_EDICT(prinst, pr_global_struct->self);
/*	ED_Print (ed);
*/
	ED_Print(prinst, ed);
	Con_Printf("%s", s);

	if (developer.value)
		(*prinst->pr_trace) = 2;
	else
	{
		ED_Free (prinst, ed);

		prinst->AbortStack(prinst);
	
		PR_BIError (prinst, "Program error: %s", s);

		if (sv.time > 10)
			Cbuf_AddText("restart\n", RESTRICT_LOCAL);
	}
}

static void PF_cs_setsensativityscaler (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	in_sensitivityscale = G_FLOAT(OFS_PARM0);
}

static void PF_cs_pointparticles (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int effectnum = G_FLOAT(OFS_PARM0)-1;
	float *org = G_VECTOR(OFS_PARM1);
	float *vel = G_VECTOR(OFS_PARM2);
	float count = G_FLOAT(OFS_PARM3);

	if (*prinst->callargc < 3)
		vel = vec3_origin;
	if (*prinst->callargc < 4)
		count = 1;

	P_RunParticleEffectType(org, vel, count, effectnum);
}

static void PF_cs_particlesloaded (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *effectname = PR_GetStringOfs(prinst, OFS_PARM0);
	G_FLOAT(OFS_RETURN) = P_DescriptionIsLoaded(effectname);
}

//get the input commands, and stuff them into some globals.
static void PF_cs_getinputstate (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int f;
	usercmd_t *cmd;

	f = G_FLOAT(OFS_PARM0);
	if (f > cls.netchan.outgoing_sequence)
	{
		G_FLOAT(OFS_RETURN) = false;
		return;
	}
	if (f < cls.netchan.outgoing_sequence - UPDATE_MASK || f < 0)
	{
		G_FLOAT(OFS_RETURN) = false;
		return;
	}
	
	// save this command off for prediction
	cmd = &cl.frames[f&UPDATE_MASK].cmd[plnum];

	if (csqcg.input_timelength)
		*csqcg.input_timelength = cmd->msec/1000.0f;
	if (csqcg.input_angles)
	{
		csqcg.input_angles[0] = SHORT2ANGLE(cmd->angles[0]);
		csqcg.input_angles[1] = SHORT2ANGLE(cmd->angles[1]);
		csqcg.input_angles[2] = SHORT2ANGLE(cmd->angles[2]);
	}
	if (csqcg.input_movevalues)
	{
		csqcg.input_movevalues[0] = cmd->forwardmove;
		csqcg.input_movevalues[1] = cmd->sidemove;
		csqcg.input_movevalues[2] = cmd->upmove;
	}
	if (csqcg.input_buttons)
		*csqcg.input_buttons = cmd->buttons;

	G_FLOAT(OFS_RETURN) = true;
}
#define ANGLE2SHORT(x) ((x/360.0)*65535)
//read lots of globals, run the default player physics, write lots of globals.
static void PF_cs_runplayerphysics (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int msecs;
	extern vec3_t	player_mins;
	extern vec3_t	player_maxs;
/*
	int			sequence;	// just for debugging prints

	// player state
	vec3_t		origin;
	vec3_t		angles;
	vec3_t		velocity;
	qboolean		jump_held;
	int			jump_msec;	// msec since last jump
	float		waterjumptime;
	int			pm_type;
	int			hullnum;

	// world state
	int			numphysent;
	physent_t	physents[MAX_PHYSENTS];	// 0 should be the world

	// input
	usercmd_t	cmd;

	qboolean onladder;

	// results
	int			numtouch;
	int			touchindex[MAX_PHYSENTS];
	qboolean		onground;
	int			groundent;		// index in physents array, only valid
								// when onground is true
	int			waterlevel;
	int			watertype;
} playermove_t;

typedef struct {
	float	gravity;
	float	stopspeed;
	float	maxspeed;
	float	spectatormaxspeed;
	float	accelerate;
	float	airaccelerate;
	float	wateraccelerate;
	float	friction;
	float	waterfriction;
	float	entgravity;
	float	bunnyspeedcap;
	float	ktjump;
	qboolean	slidefix;
	qboolean	airstep;
	qboolean	walljump;


	*/

	pmove.sequence = *csqcg.clientcommandframe;
	pmove.pm_type = PM_NORMAL;

//set up the movement command
	msecs = *csqcg.input_timelength*1000 + 0.5f;
	//precision inaccuracies. :(
	pmove.angles[0] = ANGLE2SHORT(csqcg.input_angles[0]);
	pmove.angles[1] = ANGLE2SHORT(csqcg.input_angles[1]);
	pmove.angles[2] = ANGLE2SHORT(csqcg.input_angles[2]);
	pmove.cmd.forwardmove = csqcg.input_movevalues[0];
	pmove.cmd.sidemove = csqcg.input_movevalues[1];
	pmove.cmd.upmove = csqcg.input_movevalues[2];

	VectorCopy(csqcg.pmove_org, pmove.origin);
	VectorCopy(csqcg.pmove_vel, pmove.velocity);
	VectorCopy(csqcg.pmove_maxs, player_maxs);
	VectorCopy(csqcg.pmove_mins, player_mins);
	pmove.hullnum = 1;


	while(msecs)
	{
		pmove.cmd.msec = msecs;
		if (pmove.cmd.msec > 50)
			pmove.cmd.msec = 50;
		msecs -= pmove.cmd.msec;
		PM_PlayerMove(1);
	}


	VectorCopy(pmove.origin, csqcg.pmove_org);
	VectorCopy(pmove.velocity, csqcg.pmove_vel);
}

static void CheckSendPings(void)
{	//quakeworld sends a 'pings' client command to retrieve the frequently updating stuff
	if (realtime - cl.last_ping_request > 2)
	{
		cl.last_ping_request = realtime;
		CL_SendClientCommand(false, "pings");
	}
}

//string(float pnum, string keyname)
static void PF_cs_getplayerkey (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *ret;
	int pnum = G_FLOAT(OFS_PARM0);
	char *keyname = PR_GetStringOfs(prinst, OFS_PARM1);
	if (pnum < 0)
	{
		Sbar_SortFrags(false);
		if (pnum >= -scoreboardlines)
		{//sort by 
			pnum = fragsort[-(pnum+1)];
		}
	}

	if (pnum < 0 || pnum >= MAX_CLIENTS)
		ret = "";
	else if (!*cl.players[pnum].userinfo)
		ret = "";
	else if (!strcmp(keyname, "ping"))
	{
		CheckSendPings();

		ret = PF_TempStr(prinst);
		sprintf(ret, "%i", cl.players[pnum].ping);
	}
	else if (!strcmp(keyname, "frags"))
	{
		ret = PF_TempStr(prinst);
		sprintf(ret, "%i", cl.players[pnum].frags);
	}
	else if (!strcmp(keyname, "pl"))	//packet loss
	{
		CheckSendPings();

		ret = PF_TempStr(prinst);
		sprintf(ret, "%i", cl.players[pnum].pl);
	}
	else if (!strcmp(keyname, "entertime"))	//packet loss
	{
		ret = PF_TempStr(prinst);
		sprintf(ret, "%i", cl.players[pnum].entertime);
	}
	else
	{
		ret = Info_ValueForKey(cl.players[pnum].userinfo, keyname);
	}
	if (*ret)
		RETURN_SSTRING(ret);
	else
		G_INT(OFS_RETURN) = 0;
}

extern int mouseusedforgui, mousecursor_x, mousecursor_y;
static void PF_cs_setwantskeys (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	qboolean wants = G_FLOAT(OFS_PARM0);
	csqcwantskeys = wants;
	mouseusedforgui = wants;
}

static void PF_cs_getmousepos (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN+0) = mousecursor_x;
	G_FLOAT(OFS_RETURN+1) = mousecursor_y;
	G_FLOAT(OFS_RETURN+2) = 0;
}

#define lh_extension_t void
lh_extension_t *checkfteextensionsv(char *name);
lh_extension_t *checkextension(char *name);

static void PF_checkextension (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *extname = PR_GetStringOfs(prinst, OFS_PARM0);
	G_FLOAT(OFS_RETURN) = checkextension(extname) || checkfteextensionsv(extname);
}

void PF_cs_sound(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char		*sample;
	int			channel;
	csqcedict_t		*entity;
	int 		volume;
	float attenuation;

	sfx_t *sfx;
		
	entity = (csqcedict_t*)G_EDICT(prinst, OFS_PARM0);
	channel = G_FLOAT(OFS_PARM1);
	sample = PR_GetStringOfs(prinst, OFS_PARM2);
	volume = G_FLOAT(OFS_PARM3) * 255;
	attenuation = G_FLOAT(OFS_PARM4);

	sfx = S_PrecacheSound(sample);
	if (sfx)
		S_StartSound(-entity->entnum, channel, sfx, entity->v->origin, volume, attenuation);
};

static void PF_cs_particle(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *org = G_VECTOR(OFS_PARM0);
	float *dir = G_VECTOR(OFS_PARM1);
	float colour = G_FLOAT(OFS_PARM2);
	float count = G_FLOAT(OFS_PARM2);

	P_RunParticleEffect(org, dir, colour, count);
}

void CL_SpawnSpriteEffect(vec3_t org, model_t *model, int startframe, int framecount, int framerate);
void PF_cl_effect(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float *org = G_VECTOR(OFS_PARM0);
	char *name = PR_GetStringOfs(prinst, OFS_PARM1);
	float startframe = G_FLOAT(OFS_PARM2);
	float endframe = G_FLOAT(OFS_PARM3);
	float framerate = G_FLOAT(OFS_PARM4);
	model_t *mdl;

	mdl = Mod_ForName(name, false);
	if (mdl)
		CL_SpawnSpriteEffect(org, mdl, startframe, endframe, framerate);
	else
		Con_Printf("PF_cl_effect: Couldn't load model %s\n", name);
}

void PF_cl_ambientsound(progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char		*samp;
	float		*pos;
	float 		vol, attenuation;

	pos = G_VECTOR (OFS_PARM0);			
	samp = PR_GetStringOfs(prinst, OFS_PARM1);
	vol = G_FLOAT(OFS_PARM2);
	attenuation = G_FLOAT(OFS_PARM3);

	S_StaticSound (S_PrecacheSound (samp), pos, vol, attenuation);
}

static void PF_cs_vectorvectors (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	VectorCopy(G_VECTOR(OFS_PARM0), csqcg.forward);
	VectorNormalize(csqcg.forward);
	VectorVectors(csqcg.forward, csqcg.right, csqcg.up);
}

static void PF_cs_lightstyle (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int stnum = G_FLOAT(OFS_PARM0);
	char *str = PR_GetStringOfs(prinst, OFS_PARM1);
	int colourflags = 7;

	if ((unsigned)stnum >= MAX_LIGHTSTYLES)
	{
		Con_Printf ("PF_cs_lightstyle: stnum > MAX_LIGHTSTYLES");
		return;
	}
	cl_lightstyle[stnum].colour = colourflags;
	Q_strncpyz (cl_lightstyle[stnum].map,  str, sizeof(cl_lightstyle[stnum].map));
	cl_lightstyle[stnum].length = Q_strlen(cl_lightstyle[stnum].map);
}

void PF_cs_changeyaw (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t		*ent;
	float		ideal, current, move, speed;

	ent = (void*)PROG_TO_EDICT(prinst, *csqcg.self);
	current = anglemod( ent->v->angles[1] );
	ideal = ent->v->ideal_yaw;
	speed = ent->v->yaw_speed;
	
	if (current == ideal)
		return;
	move = ideal - current;
	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}
	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}
	
	ent->v->angles[1] = anglemod (current + move);
}
void PF_cs_changepitch (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t		*ent;
	float		ideal, current, move, speed;

	ent = (void*)PROG_TO_EDICT(prinst, *csqcg.self);
	current = anglemod( ent->v->angles[0] );
	ideal = ent->v->ideal_pitch;
	speed = ent->v->pitch_speed;
	
	if (current == ideal)
		return;
	move = ideal - current;
	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}
	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}
	
	ent->v->angles[0] = anglemod (current + move);
}

static void PF_cs_findradius (progfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	csqcedict_t	*ent, *chain;
	float	rad;
	float	*org;
	vec3_t	eorg;
	int		i, j;

	chain = (csqcedict_t *)sv.edicts;

	org = G_VECTOR(OFS_PARM0);
	rad = G_FLOAT(OFS_PARM1);

	for (i=1 ; i<sv.num_edicts ; i++)
	{
		ent = (void*)EDICT_NUM(svprogfuncs, i);
		if (ent->isfree)
			continue;
		if (ent->v->solid == SOLID_NOT)
			continue;
		for (j=0 ; j<3 ; j++)
			eorg[j] = org[j] - (ent->v->origin[j] + (ent->v->mins[j] + ent->v->maxs[j])*0.5);			
		if (Length(eorg) > rad)
			continue;
			
		ent->v->chain = EDICT_TO_PROG(prinst, (void*)chain);
		chain = ent;
	}

	RETURN_EDICT(prinst, (void*)chain);
}

//these are the builtins that still need to be added.
#define PF_cs_droptofloor		PF_Fixme
#define PF_cs_tracetoss			PF_Fixme
#define PF_cs_makestatic		PF_Fixme
#define PF_cs_copyentity		PF_Fixme
#define PF_cl_te_blood			PF_Fixme
#define PF_cl_te_bloodshower	PF_Fixme
#define PF_cl_te_particlecube	PF_Fixme
#define PF_cl_te_spark			PF_Fixme
#define PF_cl_te_smallflash		PF_Fixme
#define PF_cl_te_customflash	PF_Fixme
#define PF_cl_te_gunshot		PF_Fixme
#define PF_cl_te_spike			PF_Fixme
#define PF_cl_te_superspike		PF_Fixme
#define PF_cl_te_explosion		PF_Fixme
#define PF_cl_te_tarexplosion	PF_Fixme
#define PF_cl_te_wizspike		PF_Fixme
#define PF_cl_te_knightspike	PF_Fixme
#define PF_cl_te_lavasplash		PF_Fixme
#define PF_cl_te_teleport		PF_Fixme
#define PF_cl_te_explosion2		PF_Fixme
#define PF_cl_te_lightning1		PF_Fixme
#define PF_cl_te_lightning2		PF_Fixme
#define PF_cl_te_lightning3		PF_Fixme
#define PF_cl_te_beam			PF_Fixme
#define PF_cl_te_plasmaburn		PF_Fixme
#define PS_cs_setattachment		PF_Fixme
#define PF_cs_break				PF_Fixme
#define PF_cs_walkmove			PF_Fixme
#define PF_cs_checkbottom		PF_Fixme
#define PF_cl_playingdemo		PF_Fixme
#define PF_cl_runningserver		PF_Fixme

#define PF_FixTen PF_Fixme,PF_Fixme,PF_Fixme,PF_Fixme,PF_Fixme,PF_Fixme,PF_Fixme,PF_Fixme,PF_Fixme,PF_Fixme

//prefixes:
//PF_ - common, works on any vm
//PF_cs_ - works in csqc only (dependant upon globals or fields)
//PF_cl_ - works in csqc and menu (if needed...)

//warning: functions that depend on globals are bad, mkay?
static builtin_t csqc_builtins[] = {
//0
PF_Fixme,				// #0
PF_cs_makevectors,		// #1 void() makevectors (QUAKE)
PF_cs_SetOrigin,		// #2 void(entity e, vector org) setorigin (QUAKE)
PF_cs_SetModel,			// #3 void(entity e, string modl) setmodel (QUAKE)
PF_cs_SetSize,			// #4 void(entity e, vector mins, vector maxs) setsize (QUAKE)
PF_Fixme,				// #5
PF_cs_break,			// #6 void() debugbreak (QUAKE)
PF_random,				// #7 float() random (QUAKE)
PF_cs_sound,			// #8 void(entity e, float chan, string samp, float vol, float atten) sound (QUAKE)
PF_normalize,			// #9 vector(vector in) normalize (QUAKE)
//10
PF_error,				// #10 void(string errortext) error (QUAKE)
PF_objerror,			// #11 void(string errortext) onjerror (QUAKE)
PF_vlen,				// #12 float(vector v) vlen (QUAKE)
PF_vectoyaw,			// #13 float(vector v) vectoyaw (QUAKE)
PF_Spawn,				// #14 entity() spawn (QUAKE)
PF_cs_remove,			// #15 void(entity e) remove (QUAKE)
PF_cs_traceline,		// #16 void(vector v1, vector v2, float nomonst, entity forent) traceline (QUAKE)
PF_NoCSQC,				// #17 entity() checkclient (QUAKE) (don't support)
PF_FindString,			// #18 entity(entity start, .string fld, string match) findstring (QUAKE)
PF_cs_PrecacheSound,	// #19 void(string str) precache_sound (QUAKE)
//20
PF_cs_PrecacheModel,	// #20 void(string str) precache_model (QUAKE)
PF_NoCSQC,				// #21 void(entity client, string s) stuffcmd (QUAKE) (don't support)
PF_cs_findradius,		// #22 entity(vector org, float rad) findradius (QUAKE)
PF_NoCSQC,				// #23 void(string s, ...) bprint (QUAKE) (don't support)
PF_NoCSQC,				// #24 void(entity e, string s, ...) sprint (QUAKE) (don't support)
PF_dprint,				// #25 void(string s, ...) dprint (QUAKE)
PF_ftos,				// #26 string(float f) ftos (QUAKE)
PF_vtos,				// #27 string(vector f) vtos (QUAKE)
PF_coredump,			// #28 void(void) coredump (QUAKE)
PF_traceon,				// #29 void() traceon (QUAKE)
//30
PF_traceoff,			// #30 void() traceoff (QUAKE)
PF_eprint,				// #31 void(entity e) eprint (QUAKE)
PF_cs_walkmove,			// #32 float(float yaw, float dist) walkmove (QUAKE)
PF_Fixme,				// #33
PF_cs_droptofloor,		// #34
PF_cs_lightstyle,		// #35 void(float lightstyle, string stylestring) lightstyle (QUAKE)
PF_rint,				// #36 float(float f) rint (QUAKE)
PF_floor,				// #37 float(float f) floor (QUAKE)
PF_ceil,				// #38 float(float f) ceil (QUAKE)
PF_Fixme,				// #39
//40
PF_cs_checkbottom,		// #40 float(entity e) checkbottom (QUAKE)
PF_cs_pointcontents,	// #41 float(vector org) pointcontents (QUAKE)
PF_Fixme,				// #42
PF_fabs,				// #43 float(float f) fabs (QUAKE)
PF_NoCSQC,				// #44 vector(entity e, float speed) aim (QUAKE) (don't support)
PF_cvar,				// #45 float(string cvarname) cvar (QUAKE)
PF_localcmd,			// #46 void(string str) localcmd (QUAKE)
PF_nextent,				// #47 entity(entity e) nextent (QUAKE)
PF_cs_particle,			// #48 void(vector org, vector dir, float colour, float count) particle (QUAKE)
PF_cs_changeyaw,		// #49 void() changeyaw (QUAKE)
//50
PF_Fixme,				// #50
PF_vectoangles,			// #51 vector(vector v) vectoangles (QUAKE)
PF_Fixme,				// #52 void(float to, float f) WriteByte (QUAKE)
PF_Fixme,				// #53 void(float to, float f) WriteChar (QUAKE)
PF_Fixme,				// #54 void(float to, float f) WriteShort (QUAKE)

PF_Fixme,				// #55 void(float to, float f) WriteLong (QUAKE)
PF_Fixme,				// #56 void(float to, float f) WriteCoord (QUAKE)
PF_Fixme,				// #57 void(float to, float f) WriteAngle (QUAKE)
PF_Fixme,				// #58 void(float to, float f) WriteString (QUAKE)
PF_Fixme,				// #59 void(float to, float f) WriteEntity (QUAKE)

//60
PF_Sin,					// #60 float(float angle) sin (DP_QC_SINCOSSQRTPOW)
PF_Cos,					// #61 float(float angle) cos (DP_QC_SINCOSSQRTPOW)
PF_Sqrt,				// #62 float(float value) sqrt (DP_QC_SINCOSSQRTPOW)
PF_cs_changepitch,		// #63 void(entity ent) changepitch (DP_QC_CHANGEPITCH)
PF_cs_tracetoss,		// #64 void(entity ent, entity ignore) tracetoss (DP_QC_TRACETOSS)

PF_etos,				// #65 string(entity ent) etos (DP_QC_ETOS)
PF_Fixme,				// #66
PF_Fixme,				// #67 void(float step) movetogoal (QUAKE)
PF_NoCSQC,				// #68 void(string s) precache_file (QUAKE) (don't support)
PF_cs_makestatic,		// #69 void(entity e) makestatic (QUAKE)
//70
PF_NoCSQC,				// #70 void(string mapname) changelevel (QUAKE) (don't support)
PF_Fixme,				// #71
PF_cvar_set,			// #72 void(string cvarname, string valuetoset) cvar_set (QUAKE)
PF_NoCSQC,				// #73 void(entity ent, string text) centerprint (QUAKE) (don't support - cprint is supported instead)
PF_cl_ambientsound,		// #74 void (vector pos, string samp, float vol, float atten) ambientsound (QUAKE)

PF_cs_PrecacheModel,	// #75 void(string str) precache_model2 (QUAKE)
PF_cs_PrecacheSound,	// #76 void(string str) precache_sound2 (QUAKE)
PF_NoCSQC,				// #77 void(string str) precache_file2 (QUAKE)
PF_NoCSQC,				// #78 void() setspawnparms (QUAKE) (don't support)
PF_NoCSQC,				// #79 void(entity killer, entity killee) logfrag (QW_ENGINE) (don't support)
//80
PF_NoCSQC,				// #80 string(entity e, string keyname) infokey (QW_ENGINE) (don't support)
PF_stof,				// #81 float(string s) stof (FRIK_FILE or QW_ENGINE)
PF_NoCSQC,				// #82 void(vector where, float set) multicast (QW_ENGINE) (don't support)
PF_Fixme,
PF_Fixme,

PF_Fixme,
PF_Fixme,
PF_Fixme,
PF_Fixme,
PF_Fixme,
//90
PF_cs_tracebox,
PF_Fixme,
PF_Fixme,
PF_registercvar,		// #93 void(string cvarname, string defaultvalue) registercvar (DP_QC_REGISTERCVAR)
PF_min,					// #94 float(float a, floats) min (DP_QC_MINMAXBOUND)

PF_max,					// #95 float(float a, floats) max (DP_QC_MINMAXBOUND)
PF_bound,				// #96 float(float minimum, float val, float maximum) bound (DP_QC_MINMAXBOUND)
PF_pow,					// #97 float(float value) pow (DP_QC_SINCOSSQRTPOW)
PF_Fixme,				// #98
PF_checkextension,		// #99 float(string extname) checkextension (EXT_CSQC)
//100
PF_Fixme,
PF_Fixme,
PF_Fixme,
PF_Fixme,
PF_Fixme,

PF_Fixme,
PF_Fixme,
PF_Fixme,
PF_Fixme,
PF_Fixme,
//110
PF_fopen,				// #110 float(string strname, float accessmode) fopen (FRIK_FILE)
PF_fclose,				// #111 void(float fnum) fclose (FRIK_FILE)
PF_fgets,				// #112 string(float fnum) fgets (FRIK_FILE)
PF_fputs,				// #113 void(float fnum, string str) fputs (FRIK_FILE)
PF_strlen,				// #114 float(string str) strlen (FRIK_FILE)

PF_strcat,				// #115 string(string str1, string str2, ...) strcat (FRIK_FILE)
PF_substring,			// #116 string(string str, float start, float length) substring (FRIK_FILE)
PF_stov,				// #117 vector(string str) stov (FRIK_FILE)
PF_dupstring,			// #118 string(string str) dupstring (FRIK_FILE)
PF_forgetstring,		// #119 void(string str) freestring (FRIK_FILE)


//120
PF_Fixme,
PF_Fixme,
PF_Fixme,
PF_Fixme,
PF_Fixme,

PF_Fixme,
PF_Fixme,
PF_Fixme,
PF_Fixme,
PF_Fixme,

//130
PF_R_ClearScene,			// #??? 
PF_R_AddEntityMask,			// #??? 
PF_R_AddEntity,				// #??? 
PF_R_SetViewFlag,			// #??? 
PF_R_RenderScene,			// #??? 

PF_R_AddDynamicLight,		// #??? 
PF_Fixme,
PF_Fixme,
PF_Fixme,
PF_CL_drawline,			// #???

//140
PF_CL_is_cached_pic,		// #??? 
PF_CL_precache_pic,			// #??? 
PF_CL_free_pic,				// #??? 
PF_CL_drawcharacter,		// #??? 
PF_CL_drawstring,			// #??? 
PF_CL_drawpic,				// #??? 
PF_CL_drawfill,				// #??? 
PF_CL_drawsetcliparea,		// #??? 
PF_CL_drawresetcliparea,	// #??? 
PF_CL_drawgetimagesize,		// #??? vector(string picname) draw_getimagesize (EXT_CSQC)

//150
PF_cs_getstatf,				// #??? float(float stnum) getstatf (EXT_CSQC)
PF_cs_getstati,				// #??? float(float stnum) getstati (EXT_CSQC)
PF_cs_getstats,				// #??? string(float firststnum) getstats (EXT_CSQC)
PF_cs_SetModelIndex,		// #??? void(entity e, float mdlindex) setmodelindex (EXT_CSQC)
PF_cs_ModelnameForIndex,	// #??? string(float mdlindex) modelnameforindex (EXT_CSQC)

PF_cs_setsensativityscaler, // #??? void(float sens) setsensitivityscaler (EXT_CSQC)
PF_cl_cprint,				// #??? void(string s) cprint (EXT_CSQC)
PF_print,					// #??? void(string s) print (EXT_CSQC)
PF_cs_pointparticles,		// #??? void(float effectnum, vector origin [, vector dir, float count]) pointparticles (EXT_CSQC)
PF_cs_particlesloaded,		// #??? float(string effectname) particleeffectnum (EXT_CSQC)

//160
PF_cs_getinputstate,		// #??? float(float framenum) getinputstate (EXT_CSQC)
PF_cs_runplayerphysics,		// #??? 
PF_cs_getplayerkey,			// #??? string(float playernum, string keyname) getplayerkeyvalue (EXT_CSQC)
PF_cs_setwantskeys,			// #??? void(float wants) setwantskeys (EXT_CSQC)
PF_cs_getmousepos,			// #??? vector() getmousepos (EXT_CSQC)

PF_cl_playingdemo,			// #??? float() isdemo
PF_cl_runningserver,			// #??? float() isserver
PF_cl_keynumtostring,			// #??? string(float keynum) keynumtostring (EXT_CSQC)
PF_cl_stringtokeynum,			// #??? float(string keyname) stringtokeynum (EXT_CSQC)
PF_cl_getkeybind,			// #??? string(float keynum) getkeybind (EXT_CSQC)

//170
//note that 'ReadEntity' is pretty hard to implement reliably. Modders should use a combination of ReadShort, and findfloat, and remember that it might not be known clientside (pvs culled or other reason)
PF_ReadByte,				// #??? float() readbyte (EXT_CSQC)
PF_ReadChar,				// #??? float() readchar (EXT_CSQC)
PF_ReadShort,				// #??? float() readshort (EXT_CSQC)
PF_ReadLong,				// #??? float() readlong (EXT_CSQC)
PF_ReadCoord,				// #??? float() readcoord (EXT_CSQC)

PF_ReadAngle,				// #??? float() readangle (EXT_CSQC)
PF_ReadString,				// #??? string() readstring (EXT_CSQC)
PF_Fixme,
PF_Fixme,
PF_Fixme,

//180
PF_FixTen,

//190
PF_FixTen,

//200
PF_FixTen,

//210
PF_FixTen,

//220
PF_Fixme,		// #220
PF_strstrofs,	// #221 float(string s1, string sub) strstrofs (FTE_STRINGS)
PF_str2chr,		// #222 float(string str, float index) str2chr (FTE_STRINGS)
PF_chr2str,		// #223 string(float chr, ...) chr2str (FTE_STRINGS)
PF_strconv,		// #224 string(float ccase, float redalpha, float redchars, string str, ...) strconv (FTE_STRINGS)

PF_infoadd,		// #225 string(string old, string key, string value) infoadd
PF_infoget,		// #226 string(string info, string key) infoget
PF_strncmp,		// #227 float(string s1, string s2, float len) strncmp (FTE_STRINGS)
PF_strcasecmp,	// #228 float(string s1, string s2) strcasecmp (FTE_STRINGS)
PF_strncasecmp,	// #229 float(string s1, string s2, float len) strncasecmp (FTE_STRINGS)

//230
PF_FixTen,

//240
PF_FixTen,

//250
PF_FixTen,

//260
PF_FixTen,

//270
PF_FixTen,

//280
PF_FixTen,

//290
PF_FixTen,

//300
PF_FixTen,

//310
PF_FixTen,

//320
PF_FixTen,

//330
PF_FixTen,

//340
PF_FixTen,

//350
PF_FixTen,

//360
PF_FixTen,

//370
PF_FixTen,

//380
PF_FixTen,

//390
PF_FixTen,

//400
PF_cs_copyentity,		// #400 void(entity from, entity to) copyentity (DP_QC_COPYENTITY)
PF_NoCSQC,				// #401 void(entity cl, float colours) setcolors (DP_SV_SETCOLOR) (don't implement)
PF_findchain,			// #402 entity(string field, string match) findchain (DP_QC_FINDCHAIN)
PF_findchainfloat,		// #403 entity(float fld, float match) findchainfloat (DP_QC_FINDCHAINFLOAT)
PF_cl_effect,			// #404 void(vector org, string modelname, float startframe, float endframe, float framerate) effect (DP_SV_EFFECT)

PF_cl_te_blood,			// #405 te_blood (DP_TE_BLOOD)
PF_cl_te_bloodshower,	// #406 void(vector mincorner, vector maxcorner, float explosionspeed, float howmany) te_bloodshower (DP_TE_BLOODSHOWER)
PF_Fixme,				// #407
PF_cl_te_particlecube,	// #408 void(vector mincorner, vector maxcorner, vector vel, float howmany, float color, float gravityflag, float randomveljitter) te_particlecube (DP_TE_PARTICLECUBE)
PF_Fixme,				// #409

PF_Fixme,				// #410
PF_cl_te_spark,			// #411 void(vector org, vector vel, float howmany) te_spark (DP_TE_SPARK)
PF_Fixme,				// #412
PF_Fixme,				// #413
PF_Fixme,				// #414

PF_Fixme,				// #415
PF_cl_te_smallflash,	// #416 void(vector org) te_smallflash (DP_TE_SMALLFLASH)
PF_cl_te_customflash,	// #417 void(vector org, float radius, float lifetime, vector color) te_customflash (DP_TE_CUSTOMFLASH)
PF_cl_te_gunshot,		// #418 te_gunshot (DP_TE_STANDARDEFFECTBUILTINS)
PF_cl_te_spike,			// #419 te_spike (DP_TE_STANDARDEFFECTBUILTINS)

PF_cl_te_superspike,	// #420 te_superspike (DP_TE_STANDARDEFFECTBUILTINS)
PF_cl_te_explosion,		// #421 te_explosion (DP_TE_STANDARDEFFECTBUILTINS)
PF_cl_te_tarexplosion,	// #422 te_tarexplosion (DP_TE_STANDARDEFFECTBUILTINS)
PF_cl_te_wizspike,		// #423 te_wizspike (DP_TE_STANDARDEFFECTBUILTINS)
PF_cl_te_knightspike,	// #424 te_knightspike (DP_TE_STANDARDEFFECTBUILTINS)

PF_cl_te_lavasplash,	// #425 te_lavasplash (DP_TE_STANDARDEFFECTBUILTINS)
PF_cl_te_teleport,		// #426 te_teleport (DP_TE_STANDARDEFFECTBUILTINS)
PF_cl_te_explosion2,	// #427 te_explosion2 (DP_TE_STANDARDEFFECTBUILTINS)
PF_cl_te_lightning1,	// #428 te_lightning1 (DP_TE_STANDARDEFFECTBUILTINS)
PF_cl_te_lightning2,	// #429 te_lightning2 (DP_TE_STANDARDEFFECTBUILTINS)

PF_cl_te_lightning3,	// #430 te_lightning3 (DP_TE_STANDARDEFFECTBUILTINS)
PF_cl_te_beam,			// #431 te_beam (DP_TE_STANDARDEFFECTBUILTINS)
PF_cs_vectorvectors,	// #432 void(vector dir) vectorvectors (DP_QC_VECTORVECTORS)
PF_cl_te_plasmaburn,	// #433 void(vector org) te_plasmaburn (DP_TE_PLASMABURN)
PF_Fixme,				// #434

PF_Fixme,				// #435
PF_Fixme,				// #436
PF_Fixme,				// #437
PF_Fixme,				// #438
PF_Fixme,				// #439

PF_NoCSQC,				// #440 void(entity e, string s) clientcommand (KRIMZON_SV_PARSECLIENTCOMMAND) (don't implement)
PF_Tokenize,			// #441 float(string s) tokenize (KRIMZON_SV_PARSECLIENTCOMMAND)
PF_ArgV,				// #442 string(float n) argv (KRIMZON_SV_PARSECLIENTCOMMAND)
PS_cs_setattachment,	// #443 void(entity e, entity tagentity, string tagname) setattachment (DP_GFX_QUAKE3MODELTAGS)
PF_search_begin,		// #444 float	search_begin(string pattern, float caseinsensitive, float quiet) (DP_QC_FS_SEARCH)

PF_search_end,			// #445 void	search_end(float handle) (DP_QC_FS_SEARCH)
PF_search_getsize,		// #446 float	search_getsize(float handle) (DP_QC_FS_SEARCH)
PF_search_getfilename,	// #447 string	search_getfilename(float handle, float num) (DP_QC_FS_SEARCH)
PF_cvar_string,			// #448 string(float n) cvar_string (DP_QC_CVAR_STRING)
PF_FindFlags,			// #449 entity(entity start, .entity fld, float match) findflags (DP_QC_FINDFLAGS)

PF_findchainflags,		// #450 entity(.float fld, float match) findchainflags (DP_QC_FINDCHAINFLAGS)
PF_Fixme,				// #451
PF_Fixme,				// #452
PF_NoCSQC,				// #453 void(entity player) dropclient (DP_QC_BOTCLIENT) (don't implement)
PF_NoCSQC,				// #454	entity() spawnclient (DP_QC_BOTCLIENT) (don't implement)

PF_NoCSQC,				// #455 float(entity client) clienttype (DP_QC_BOTCLIENT) (don't implement)
PF_Fixme,				// #456
PF_Fixme,				// #457
PF_Fixme,				// #458
PF_Fixme,				// #459

//460
PF_FixTen,
};
static int csqc_numbuiltins = sizeof(csqc_builtins)/sizeof(csqc_builtins[0]);





static jmp_buf csqc_abort;
static progparms_t csqcprogparms;
static int num_csqc_edicts;



int COM_FileSize(char *path);
pbool QC_WriteFile(char *name, void *data, int len);
void *VARGS PR_CB_Malloc(int size);	//these functions should be tracked by the library reliably, so there should be no need to track them ourselves.
void VARGS PR_CB_Free(void *mem);

//Any menu builtin error or anything like that will come here.
void VARGS CSQC_Abort (char *format, ...)	//an error occured.
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr, format);
	_vsnprintf (string,sizeof(string)-1, format,argptr);
	va_end (argptr);

	Con_Printf("CSQC_Abort: %s\nShutting down csqc\n", string);


{
	int size = 1024*1024*8;
	char *buffer = BZ_Malloc(size);
	csqcprogs->save_ents(csqcprogs, buffer, &size, 3);
	COM_WriteFile("csqccore.txt", buffer, size);
	BZ_Free(buffer);
}

	Host_EndGame("csqc error");
}

void CSQC_Shutdown(void)
{
	search_close_progs(csqcprogs, false);
	if (csqcprogs)
	{
		CloseProgs(csqcprogs);
		Con_Printf("Closed csqc\n");
	}
	csqcprogs = NULL;

	in_sensitivityscale = 1;
}

//when the qclib needs a file, it calls out to this function.
qbyte *CSQC_PRLoadFile (char *path, void *buffer, int bufsize)
{
	qbyte *file;
	//pretend it doesn't 
	file = COM_LoadStackFile(path, buffer, bufsize);

	if (!cls.demoplayback)	//allow any csqc when playing a demo
		if (!strcmp(path, "csprogs.dat"))	//Fail to load any csprogs who's checksum doesn't match.
			if (Com_BlockChecksum(buffer, com_filesize) != csqcchecksum)
				return NULL;

	return file;
}

double  csqctime;
qboolean CSQC_Init (unsigned int checksum)
{
	csqcchecksum = checksum;

	CSQC_Shutdown();

	if (!qrenderer)
	{
		return false;
	}

	memset(cl.model_csqcname, 0, sizeof(cl.model_csqcname));
	memset(cl.model_csqcprecache, 0, sizeof(cl.model_csqcprecache));

	csqcprogparms.progsversion = PROGSTRUCT_VERSION;
	csqcprogparms.ReadFile = CSQC_PRLoadFile;//char *(*ReadFile) (char *fname, void *buffer, int *len);
	csqcprogparms.FileSize = COM_FileSize;//int (*FileSize) (char *fname);	//-1 if file does not exist
	csqcprogparms.WriteFile = QC_WriteFile;//bool (*WriteFile) (char *name, void *data, int len);
	csqcprogparms.printf = (void *)Con_Printf;//Con_Printf;//void (*printf) (char *, ...);
	csqcprogparms.Sys_Error = Sys_Error;
	csqcprogparms.Abort = CSQC_Abort;
	csqcprogparms.edictsize = sizeof(csqcedict_t);

	csqcprogparms.entspawn = NULL;//void (*entspawn) (struct edict_s *ent);	//ent has been spawned, but may not have all the extra variables (that may need to be set) set
	csqcprogparms.entcanfree = NULL;//bool (*entcanfree) (struct edict_s *ent);	//return true to stop ent from being freed
	csqcprogparms.stateop = NULL;//StateOp;//void (*stateop) (float var, func_t func);
	csqcprogparms.cstateop = NULL;//CStateOp;
	csqcprogparms.cwstateop = NULL;//CWStateOp;
	csqcprogparms.thinktimeop = NULL;//ThinkTimeOp;

	//used when loading a game
	csqcprogparms.builtinsfor = NULL;//builtin_t *(*builtinsfor) (int num);	//must return a pointer to the builtins that were used before the state was saved.
	csqcprogparms.loadcompleate = NULL;//void (*loadcompleate) (int edictsize);	//notification to reset any pointers.

	csqcprogparms.memalloc = PR_CB_Malloc;//void *(*memalloc) (int size);	//small string allocation	malloced and freed randomly
	csqcprogparms.memfree = PR_CB_Free;//void (*memfree) (void * mem);


	csqcprogparms.globalbuiltins = csqc_builtins;//builtin_t *globalbuiltins;	//these are available to all progs
	csqcprogparms.numglobalbuiltins = csqc_numbuiltins;

	csqcprogparms.autocompile = PR_NOCOMPILE;//enum {PR_NOCOMPILE, PR_COMPILENEXIST, PR_COMPILECHANGED, PR_COMPILEALWAYS} autocompile;

	csqcprogparms.gametime = &csqctime;

	csqcprogparms.sv_edicts = (edict_t **)&csqc_edicts;
	csqcprogparms.sv_num_edicts = &num_csqc_edicts;

	csqcprogparms.useeditor = NULL;//sorry... QCEditor;//void (*useeditor) (char *filename, int line, int nump, char **parms);

	csqctime = Sys_DoubleTime();
	if (!csqcprogs)
	{
		in_sensitivityscale = 1;
		csqcprogs = InitProgs(&csqcprogparms);
		PR_Configure(csqcprogs, -1, 1);
		
		CSQC_InitFields();	//let the qclib know the field order that the engine needs.
		
		if (PR_LoadProgs(csqcprogs, "csprogs.dat", 0, NULL, 0) < 0) //no per-progs builtins.
		{
			CSQC_Shutdown();
			//failed to load or something
			return false;
		}
		if (setjmp(csqc_abort))
		{
			CSQC_Shutdown();
			return false;
		}

		PF_InitTempStrings(csqcprogs);

		memset(csqcent, 0, sizeof(csqcent));
		
		csqcentsize = PR_InitEnts(csqcprogs, pr_csmaxedicts.value);
		
		CSQC_FindGlobals();

		ED_Alloc(csqcprogs);	//we need a word entity.
		//world edict becomes readonly
		EDICT_NUM(csqcprogs, 0)->readonly = true;
		EDICT_NUM(csqcprogs, 0)->isfree = false;

		if (csqcg.init_function)
			PR_ExecuteProgram(csqcprogs, csqcg.init_function);

		Con_Printf("Loaded csqc\n");
	}

	return true; //success!
}

void CSQC_RegisterCvarsAndThings(void)
{
	Cvar_Register(&pr_csmaxedicts, "csqc");
	Cvar_Register(&cl_csqcdebug, "csqc");
}

qboolean CSQC_DrawView(void)
{
	if (!csqcg.draw_function || !csqcprogs || !cl.worldmodel)
		return false;

	r_secondaryview = 0;

	DropPunchAngle (0);
	if (cl.worldmodel)
		R_LessenStains();

	if (csqcg.clientcommandframe)
		*csqcg.clientcommandframe = cls.netchan.outgoing_sequence;
	if (csqcg.servercommandframe)
		*csqcg.servercommandframe = cls.netchan.incoming_sequence;

	if (csqcg.time)
		*csqcg.time = Sys_DoubleTime();

	PR_ExecuteProgram(csqcprogs, csqcg.draw_function);

	return true;
}

qboolean CSQC_KeyPress(int key, qboolean down)
{
	void *pr_globals;

	if (!csqcprogs || !csqcwantskeys)
		return false;

	pr_globals = PR_globals(csqcprogs, PR_CURRENT);
	G_FLOAT(OFS_PARM0) = !down;
	G_FLOAT(OFS_PARM1) = MP_TranslateFTEtoDPCodes(key);
	G_FLOAT(OFS_PARM2) = 0;

	PR_ExecuteProgram (csqcprogs, csqcg.input_event);

	return true;
}

qboolean CSQC_ConsoleCommand(char *cmd)
{
	void *pr_globals;
	char *str;
	if (!csqcprogs || !csqcg.console_command)
		return false;

	str = PF_TempStr(csqcprogs);
	Q_strncpyz(str, cmd, MAXTEMPBUFFERLEN);

	pr_globals = PR_globals(csqcprogs, PR_CURRENT);
	(((string_t *)pr_globals)[OFS_PARM0] = PR_SetString(csqcprogs, str));

	PR_ExecuteProgram (csqcprogs, csqcg.console_command);
	return true;
}

qboolean CSQC_StuffCmd(char *cmd)
{
	void *pr_globals;
	char *str;
	if (!csqcprogs || !csqcg.parse_stuffcmd)
		return false;

	str = PF_TempStr(csqcprogs);
	Q_strncpyz(str, cmd, MAXTEMPBUFFERLEN);

	pr_globals = PR_globals(csqcprogs, PR_CURRENT);
	(((string_t *)pr_globals)[OFS_PARM0] = PR_SetString(csqcprogs, str));

	PR_ExecuteProgram (csqcprogs, csqcg.parse_stuffcmd);
	return true;
}
qboolean CSQC_CenterPrint(char *cmd)
{
	void *pr_globals;
	char *str;
	if (!csqcprogs || !csqcg.parse_centerprint)
		return false;

	str = PF_TempStr(csqcprogs);
	Q_strncpyz(str, cmd, MAXTEMPBUFFERLEN);

	pr_globals = PR_globals(csqcprogs, PR_CURRENT);
	(((string_t *)pr_globals)[OFS_PARM0] = PR_SetString(csqcprogs, str));

	PR_ExecuteProgram (csqcprogs, csqcg.parse_centerprint);
	return G_FLOAT(OFS_RETURN);
}

//this protocol allows up to 32767 edicts.
#ifdef PEXT_CSQC
void CSQC_ParseEntities(void)
{
	csqcedict_t *ent;
	unsigned short entnum;
	void *pr_globals;
	int packetsize;
	int packetstart;

	if (!csqcprogs)
		Host_EndGame("CSQC needs to be initialized for this server.\n");

	if (!csqcg.ent_update || !csqcg.self)
		Host_EndGame("CSQC is unable to parse entities\n");

	pr_globals = PR_globals(csqcprogs, PR_CURRENT);

	if (csqcg.time)
		*csqcg.time = Sys_DoubleTime();
	if (csqcg.servercommandframe)
		*csqcg.servercommandframe = cls.netchan.incoming_sequence;

	for(;;)
	{
		entnum = MSG_ReadShort();
		if (!entnum)
			break;
		if (entnum & 0x8000)
		{	//remove
			entnum &= ~0x8000;

			if (!entnum)
				Host_EndGame("CSQC cannot remove world!\n");

			if (entnum >= MAX_EDICTS)
				Host_EndGame("CSQC recieved too many edicts!\n");
			if (cl_csqcdebug.value)
				Con_Printf("Remove %i\n", entnum);

			ent = csqcent[entnum];
			csqcent[entnum] = NULL;

			if (!ent)	//hrm.
				continue;

			*csqcg.self = EDICT_TO_PROG(csqcprogs, (void*)ent);
			PR_ExecuteProgram(csqcprogs, csqcg.ent_remove);
			//the csqc is expected to call the remove builtin.
		}
		else
		{
			if (entnum >= MAX_EDICTS)
				Host_EndGame("CSQC recieved too many edicts!\n");

			if (cl.csqcdebug)
			{
				packetsize = MSG_ReadShort();
				packetstart = msg_readcount;
			}
			else
			{
				packetsize = 0;
				packetstart = 0;
			}

			ent = csqcent[entnum];
			if (!ent)
			{
				ent = (csqcedict_t*)ED_Alloc(csqcprogs);
				csqcent[entnum] = ent;
				ent->v->entnum = entnum;
				G_FLOAT(OFS_PARM0) = true;

				if (cl_csqcdebug.value)
					Con_Printf("Add %i\n", entnum);
			}
			else
			{
				G_FLOAT(OFS_PARM0) = false;
				if (cl_csqcdebug.value)
					Con_Printf("Update %i\n", entnum);
			}

			*csqcg.self = EDICT_TO_PROG(csqcprogs, (void*)ent);
			PR_ExecuteProgram(csqcprogs, csqcg.ent_update);

			if (cl.csqcdebug)
			{
				if (msg_readcount != packetstart+packetsize)
				{
					if (msg_readcount > packetstart+packetsize)
						Con_Printf("CSQC overread entity %i. Size %i, read %i\n", entnum, packetsize, msg_readcount - packetsize);
					else
						Con_Printf("CSQC underread entity %i. Size %i, read %i\n", entnum, packetsize, msg_readcount - packetsize);
					Con_Printf("First byte is %i\n", net_message.data[msg_readcount]);
#ifndef CLIENTONLY
					if (sv.state)
					{
						Con_Printf("Server classname: \"%s\"\n", svprogfuncs->stringtable+EDICT_NUM(svprogfuncs, entnum)->v->classname);
					}
#endif
				}
				msg_readcount = packetstart+packetsize;	//leetism.
			}
		}
	}
}
#endif

#endif
