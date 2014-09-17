#include "progtype.h"
#include "progslib.h"

#ifdef CLIENTONLY
typedef struct edict_s {
	pbool	isfree;

	float		freetime;			// realtime when the object was freed
	unsigned int entnum;
	pbool	readonly;	//causes error when QC tries writing to it. (quake's world entity)
	void	*v;
} edict_t;
#endif

struct wedict_s
{
	qboolean	isfree;
	float		freetime; // sv.time when the object was freed
	int			entnum;
	qboolean	readonly;	//world
#ifdef VM_Q1
	comentvars_t	*v;
	comextentvars_t	*xv;
#else
	union {
		comentvars_t	*v;
		comentvars_t	*xv;
	};
#endif
	/*the above is shared with qclib*/
	link_t	area;
	pvscache_t pvsinfo;

#ifdef USEODE
	entityode_t ode;
#endif
	/*the above is shared with ssqc*/
};

#define PF_cin_open PF_Fixme
#define PF_cin_close PF_Fixme
#define PF_cin_setstate PF_Fixme
#define PF_cin_getstate PF_Fixme
#define PF_cin_restart PF_Fixme
#define PF_drawline PF_Fixme
#define PF_gecko_create PF_Fixme
#define PF_gecko_destroy PF_Fixme
#define PF_gecko_navigate PF_Fixme
#define PF_gecko_keyevent PF_Fixme
#define PF_gecko_movemouse PF_Fixme
#define PF_gecko_resize PF_Fixme
#define PF_gecko_get_texture_extent PF_Fixme

#define PF_gecko_mousemove PF_Fixme
#define PF_WritePicture PF_Fixme
#define PF_ReadPicture PF_Fixme

#define G_PROG G_FLOAT

//the lh extension system asks for a name for the extension.
//the ebfs version is a function that returns a builtin number.
//thus lh's system requires various builtins to exist at specific numbers.
typedef struct lh_extension_s {
	char *name;
	int numbuiltins;
	qboolean *queried;
	char *builtinnames[21];	//extend freely
} lh_extension_t;

extern lh_extension_t QSG_Extensions[];
extern unsigned int QSG_Extensions_count;

pbool QDECL QC_WriteFile(const char *name, void *data, int len);
void *VARGS PR_CB_Malloc(int size);	//these functions should be tracked by the library reliably, so there should be no need to track them ourselves.
void VARGS PR_CB_Free(void *mem);

int PR_Printf (const char *fmt, ...);
void PF_InitTempStrings(pubprogfuncs_t *prinst);
string_t PR_TempString(pubprogfuncs_t *prinst, const char *str);	//returns a tempstring containing str
char *PF_TempStr(pubprogfuncs_t *prinst);	//returns a tempstring which can be filled in with whatever junk you want.

#define	RETURN_SSTRING(s) (((int *)pr_globals)[OFS_RETURN] = PR_SetString(prinst, s))	//static - exe will not change it.
#define	RETURN_TSTRING(s) (((int *)pr_globals)[OFS_RETURN] = PR_TempString(prinst, s))	//temp (static but cycle buffers)
extern cvar_t pr_tempstringsize;
extern cvar_t pr_tempstringcount;
extern cvar_t pr_enable_profiling;

extern int qcinput_scan;
extern int qcinput_unicode;
int MP_TranslateFTEtoQCCodes(int code);
int MP_TranslateQCtoFTECodes(int code);

//pr_cmds.c builtins that need to be moved to a common.
void VARGS PR_BIError(pubprogfuncs_t *progfuncs, char *format, ...) LIKEPRINTF(2);
void QCBUILTIN PF_print (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_dprint (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_error (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_rint (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_floor (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_ceil (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_anglemod (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_Tokenize  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_tokenizebyseparator  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_tokenize_console  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_ArgV  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_argv_start_index  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_argv_end_index  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_FindString (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_nextent (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_Sin (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_Cos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_Sqrt (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_bound (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strlen(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strcat (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_ftos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_fabs (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_vtos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_etos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_stof (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_mod (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_substring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_stov (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_dupstring(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_forgetstring(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_Spawn (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_droptofloor (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_checkbottom (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_min (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_max (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_registercvar (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_pow (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_asin (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_acos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_atan (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_atan2 (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_tan (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_localcmd (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_sprintf_internal (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals, const char *s, int firstarg, char *outbuf, int outbuflen);
void QCBUILTIN PF_sprintf (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_random (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_fclose (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_fputs (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_fgets (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_normalize (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_vlen (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_vhlen (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_changeyaw (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_changepitch (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_vectoyaw (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_vectoangles (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_rotatevectorsbyangles (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_rotatevectorsbymatrix (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_findchain (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_findchainfloat (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_coredump (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_traceon (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_traceoff (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_eprint (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_search_begin (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_search_end (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_search_getsize (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_search_getfilename (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_isfunction (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_callfunction (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_writetofile(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_loadfromfile (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_loadfromdata (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_parseentitydata(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_WasFreed (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_break (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_crc16 (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cvar_type (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_uri_escape  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_uri_unescape  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_uri_get  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_itos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_stoi (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_stoh (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_htos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void PR_fclose_progs (pubprogfuncs_t *prinst);
char *PF_VarString (pubprogfuncs_t *prinst, int	first, struct globalvars_s *pr_globals);
void PR_ProgsAdded(pubprogfuncs_t *prinst, int newprogs, const char *modulename);
void PR_AutoCvar(pubprogfuncs_t *prinst, cvar_t *var);
void QCBUILTIN PF_numentityfields (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_entityfieldname (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_entityfieldtype (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_getentityfieldstring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_putentityfieldstring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_checkcommand (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_argescape(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);


void QCBUILTIN PF_getsurfacenumpoints(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_getsurfacepoint(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_getsurfacenormal(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_getsurfacetexture(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_getsurfacenearpoint(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_getsurfaceclippedpoint(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_getsurfacenumtriangles(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_getsurfacetriangle(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_getsurfacepointattribute(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_checkpvs(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_setattachment(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

#ifndef SKELETALOBJECTS
	#define PF_gettaginfo			PF_Fixme
	#define PF_gettagindex			PF_Fixme
	#define PF_skintoname			PF_Fixme
	#define PF_frametoname			PF_Fixme
	#define PF_skel_set_bone_world	PF_Fixme
	#define PF_skel_mmap			PF_Fixme
	#define PF_skel_ragedit			PF_Fixme
	#define PF_frameduration		PF_Fixme
	#define PF_frameforname			PF_Fixme
	#define PF_skel_delete			PF_Fixme
	#define PF_skel_copybones		PF_Fixme
	#define PF_skel_mul_bones		PF_Fixme
	#define PF_skel_mul_bone		PF_Fixme
	#define PF_skel_set_bone		PF_Fixme
	#define PF_skel_get_boneabs		PF_Fixme
	#define PF_skel_get_bonerel		PF_Fixme
	#define PF_skel_find_bone		PF_Fixme
	#define PF_skel_get_boneparent	PF_Fixme
	#define PF_skel_get_bonename	PF_Fixme
	#define PF_skel_get_numbones	PF_Fixme
	#define PF_skel_build			PF_Fixme
	#define PF_skel_create			PF_Fixme
	#define PF_skinforname			PF_Fixme
#else
	void QCBUILTIN PF_skel_set_bone_world (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_mmap(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_ragedit(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_create (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_build (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_get_numbones (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_get_bonename (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_get_boneparent (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_find_bone (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_get_bonerel (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_get_boneabs (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_set_bone (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_mul_bone (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_mul_bones (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_copybones (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skel_delete (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_frametoname (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skintoname (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_frameforname (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_frameduration (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_skinforname (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_gettaginfo (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
	void QCBUILTIN PF_gettagindex (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
#endif
#if defined(SKELETALOBJECTS) || defined(RAGDOLL)
	void skel_lookup(pubprogfuncs_t *prinst, int skelidx, framestate_t *out);
	void skel_dodelete(pubprogfuncs_t *prinst);
	void skel_reset(pubprogfuncs_t *prinst);
#endif
void QCBUILTIN PF_physics_enable(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_physics_addforce(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_physics_addtorque(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_terrain_edit(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_touchtriggers(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

//pr_cmds.c builtins that need to be moved to a common.
void VARGS PR_BIError(pubprogfuncs_t *progfuncs, char *format, ...) LIKEPRINTF(2);
void QCBUILTIN PF_cvar_string (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cvar_set (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cvar_setf (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_ArgC (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_randomvec (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strreplace (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strireplace (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_randomvector (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_fopen (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_FindString (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_FindFloat (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_FindFlags (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_findchain (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_findchainfloat (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void PF_findchainflags (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_bitshift(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_Abort(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_externcall (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_externrefcall (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_externvalue (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_externset (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_instr (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_strlennocol (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strdecolorize (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strtolower (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strtoupper (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strftime (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_strstrofs (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_str2chr (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_chr2str (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strconv (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_infoadd (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_infoget (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strncmp (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strncasecmp (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_strpad (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_digest_hex (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_findradius (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_edict_for_num (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_num_for_edict (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cvar_defstring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cvar_description (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

//these functions are from pr_menu.c
void QCBUILTIN PF_SubConGetSet (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_SubConPrintf (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_SubConDraw (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_SubConInput (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_is_cached_pic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_precache_pic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_free_pic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawcharacter (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawrawstring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawcolouredstring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawpic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawline (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawfill (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawsetcliparea (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawresetcliparea (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawgetimagesize (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_stringwidth (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_drawsubpic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_findfont (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_CL_loadfont (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
#if defined(CSQC_DAT) && !defined(SERVERONLY)
void QCBUILTIN PF_R_PolygonBegin(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_R_PolygonVertex(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_R_PolygonEnd(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
#else
#define PF_R_PolygonBegin PF_Fixme
#define PF_R_PolygonVertex PF_Fixme
#define PF_R_PolygonEnd PF_Fixme
#endif

void QCBUILTIN PF_cl_getresolution (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_gethostcachevalue (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_gethostcachestring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_resethostcachemasks(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_sethostcachemaskstring(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_sethostcachemasknumber(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_resorthostcache(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_sethostcachesort(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_refreshhostcache(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_gethostcachenumber(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_gethostcacheindexforkey(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_addwantedhostcachekey(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_getextresponse(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_netaddress_resolve(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_getmousepos (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_GetBindMap (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_SetBindMap (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_keynumtostring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_findkeysforcommand (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_stringtokeynum(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_getkeybind (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_setmousetarget (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_getmousetarget (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_playingdemo (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cl_runningserver (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cs_gecko_create (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cs_gecko_destroy (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cs_gecko_navigate (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cs_gecko_keyevent (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cs_gecko_mousemove (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cs_gecko_resize (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_cs_gecko_get_texture_extent (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
typedef enum{
	SLIST_HOSTCACHEVIEWCOUNT,
	SLIST_HOSTCACHETOTALCOUNT,
	SLIST_MASTERQUERYCOUNT,
	SLIST_MASTERREPLYCOUNT,
	SLIST_SERVERQUERYCOUNT,
	SLIST_SERVERREPLYCOUNT,
	SLIST_SORTFIELD,
	SLIST_SORTDESCENDING
} hostcacheglobal_t;
void QCBUILTIN PF_shaderforname (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void search_close_progs(pubprogfuncs_t *prinst, qboolean complain);

void QCBUILTIN PF_buf_create  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_buf_del  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_buf_getsize  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_buf_copy  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_buf_sort  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_buf_implode  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_bufstr_get  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_bufstr_set  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_bufstr_add  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_bufstr_free  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_buf_cvarlist  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_buf_loadfile  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_buf_writefile  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_hash_createtab	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_hash_destroytab	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_hash_add	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_hash_get	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_hash_getcb	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_hash_delete	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_hash_getkey	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_memalloc	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_memfree	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_memcpy	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_memfill8	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_memgetval	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_memsetval	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_memptradd	(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_soundlength (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_calltimeofday (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_gettime (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

void QCBUILTIN PF_whichpack (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);

int QDECL QCEditor (pubprogfuncs_t *prinst, char *filename, int line, int statement, int nump, char **parms);
void PR_Common_Shutdown(pubprogfuncs_t *progs, qboolean errored);





/*these are server ones, provided by pr_cmds.c, as required by pr_q1qvm.c*/
#ifdef VM_Q1
void PR_SV_FillWorldGlobals(world_t *w);
model_t *SVPR_GetCModel(world_t *w, int modelindex);
void QCBUILTIN PF_WriteByte (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_WriteChar (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_WriteShort (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_WriteLong (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_WriteAngle (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_WriteCoord (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_WriteFloat (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_WriteEntity (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_multicast (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_svtraceline (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_changelevel (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_applylightstyle(int style, const char *val, vec3_t rgb);
void PF_ambientsound_Internal (float *pos, const char *samp, float vol, float attenuation);
void QCBUILTIN PF_makestatic (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_logfrag (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_ExecuteCommand  (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_setspawnparms (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
void QCBUILTIN PF_precache_vwep_model(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals);
int PF_ForceInfoKey_Internal(unsigned int entnum, const char *key, const char *value);
int PF_checkclient_Internal (pubprogfuncs_t *prinst);
void PF_precache_sound_Internal (pubprogfuncs_t *prinst, const char *s);
int PF_precache_model_Internal (pubprogfuncs_t *prinst, const char *s, qboolean queryonly);
void PF_setmodel_Internal (pubprogfuncs_t *prinst, edict_t *e, const char *m);
char *PF_infokey_Internal (int entnum, const char *value);
void PF_stuffcmd_Internal(int entnum, const char *str);
void PF_centerprint_Internal (int entnum, qboolean plaque, const char *s);
void PF_WriteString_Internal (int target, const char *str);
pbool QDECL ED_CanFree (edict_t *ed);
#endif

#define	MOVETYPE_NONE			0		// never moves
#define	MOVETYPE_ANGLENOCLIP	1
#define	MOVETYPE_ANGLECLIP		2
#define	MOVETYPE_WALK			3		// gravity
#define	MOVETYPE_STEP			4		// gravity, special edge handling
#define	MOVETYPE_FLY			5
#define	MOVETYPE_TOSS			6		// gravity
#define	MOVETYPE_PUSH			7		// no clip to world, push and crush
#define	MOVETYPE_NOCLIP			8
#define	MOVETYPE_FLYMISSILE		9		// extra size to monsters
#define	MOVETYPE_BOUNCE			10
#define MOVETYPE_BOUNCEMISSILE	11		// bounce w/o gravity
#define MOVETYPE_FOLLOW			12		// track movement of aiment
#define MOVETYPE_H2PUSHPULL		13		// pushable/pullable object
#define MOVETYPE_H2SWIM			14		// should keep the object in water
#define MOVETYPE_6DOF			30		// flightsim mode
#define MOVETYPE_WALLWALK		31		// walks up walls and along ceilings
#define MOVETYPE_PHYSICS		32

// edict->solid values
#define	SOLID_NOT				0		// no interaction with other objects
#define	SOLID_TRIGGER			1		// touch on edge, but not blocking
#define	SOLID_BBOX				2		// touch on edge, block
#define	SOLID_SLIDEBOX			3		// touch on edge, but not an onground
#define	SOLID_BSP				4		// bsp clip, touch on edge, block
#define	SOLID_PHASEH2			5		// hexen2 flag - these ents can be freely walked through or something
#define	SOLID_CORPSE			5		// non-solid to solid_slidebox entities and itself.
#define SOLID_LADDER			20		//dmw. touch on edge, not blocking. Touching players have different physics. Otherwise a SOLID_TRIGGER
#define SOLID_PORTAL			21		//1: traces always use point-size. 2: various movetypes automatically transform entities. 3: traces that impact portal bbox use a union. 4. traces ignore part of the world within the portal's box
#define	SOLID_PHYSICS_BOX		32		// deprecated. physics object (mins, maxs, mass, origin, axis_forward, axis_left, axis_up, velocity, spinvelocity)
#define	SOLID_PHYSICS_SPHERE	33		// deprecated. physics object (mins, maxs, mass, origin, axis_forward, axis_left, axis_up, velocity, spinvelocity)
#define	SOLID_PHYSICS_CAPSULE	34		// deprecated. physics object (mins, maxs, mass, origin, axis_forward, axis_left, axis_up, velocity, spinvelocity)
#define SOLID_PHYSICS_TRIMESH	35
#define SOLID_PHYSICS_CYLINDER  36

#define	GEOMTYPE_NONE      -1
#define	GEOMTYPE_SOLID      0
#define	GEOMTYPE_BOX		1
#define	GEOMTYPE_SPHERE		2
#define	GEOMTYPE_CAPSULE	3
#define	GEOMTYPE_TRIMESH	4
#define	GEOMTYPE_CYLINDER	5
#define	GEOMTYPE_CAPSULE_X	6
#define	GEOMTYPE_CAPSULE_Y	7
#define	GEOMTYPE_CAPSULE_Z	8
#define	GEOMTYPE_CYLINDER_X	9
#define	GEOMTYPE_CYLINDER_Y	10
#define	GEOMTYPE_CYLINDER_Z	11


#define JOINTTYPE_POINT 1
#define JOINTTYPE_HINGE 2
#define JOINTTYPE_SLIDER 3
#define JOINTTYPE_UNIVERSAL 4
#define JOINTTYPE_HINGE2 5
#define JOINTTYPE_FIXED -1

#define	DAMAGE_NO				0
#define	DAMAGE_YES				1
#define	DAMAGE_AIM				2

#define CLIENTTYPE_DISCONNECTED	0
#define CLIENTTYPE_REAL			1
#define CLIENTTYPE_BOT			2
#define CLIENTTYPE_NOTACLIENT	3

//shared constants
typedef enum
{
	VF_MIN = 1,
	VF_MIN_X = 2,
	VF_MIN_Y = 3,
	VF_SIZE = 4,
	VF_SIZE_X = 5,
	VF_SIZE_Y = 6,
	VF_VIEWPORT = 7,
	VF_FOV = 8,
	VF_FOVX = 9,
	VF_FOVY = 10,
	VF_ORIGIN = 11,
	VF_ORIGIN_X = 12,
	VF_ORIGIN_Y = 13,
	VF_ORIGIN_Z = 14,
	VF_ANGLES = 15,
	VF_ANGLES_X = 16,
	VF_ANGLES_Y = 17,
	VF_ANGLES_Z = 18,
	VF_DRAWWORLD = 19,
	VF_ENGINESBAR = 20,
	VF_DRAWCROSSHAIR = 21,
	VF_CARTESIAN_ANGLES = 22,

	//this is a DP-compatibility hack.
	VF_CL_VIEWANGLES_V = 33,
	VF_CL_VIEWANGLES_X = 34,
	VF_CL_VIEWANGLES_Y = 35,
	VF_CL_VIEWANGLES_Z = 36,


	//33-36 used by DP...
	VF_PERSPECTIVE = 200,
	//201 used by DP... WTF? CLEARSCREEN
	VF_LPLAYER = 202,
	VF_AFOV = 203,	//aproximate fov (match what the engine would normally use for the fov cvar). p0=fov, p1=zoom
	VF_SCREENVSIZE = 204,
	VF_SCREENPSIZE = 205,
	VF_VIEWENTITY = 206,
	VF_STATSENTITIY = 207,	//the player number for the stats.
	VF_SCREENVOFFSET = 208,

	VF_RT_SOURCECOLOUR	= 209,
	VF_RT_DEPTH			= 210,
	VF_RT_RIPPLE		= 211,	/**/
	VF_RT_DESTCOLOUR0	= 212,
	VF_RT_DESTCOLOUR1	= 213,
	VF_RT_DESTCOLOUR2	= 214,
	VF_RT_DESTCOLOUR3	= 215,
	VF_RT_DESTCOLOUR4	= 216,
	VF_RT_DESTCOLOUR5	= 217,
	VF_RT_DESTCOLOUR6	= 218,
	VF_RT_DESTCOLOUR7	= 219,
} viewflags;

/*FIXME: this should be changed*/
#define CSQC_API_VERSION 1.0f

#define CSQCRF_VIEWMODEL		1 //Not drawn in mirrors
#define CSQCRF_EXTERNALMODEL	2 //drawn ONLY in mirrors
#define CSQCRF_DEPTHHACK		4 //fun depthhack
#define CSQCRF_ADDITIVE			8 //add instead of blend
#define CSQCRF_USEAXIS			16 //use v_forward/v_right/v_up as an axis/matrix - predraw is needed to use this properly
#define CSQCRF_NOSHADOW			32 //don't cast shadows upon other entities (can still be self shadowing, if the engine wishes, and not additive)
#define CSQCRF_FRAMETIMESARESTARTTIMES 64 //EXT_CSQC_1: frame times should be read as (time-frametime).
#define CSQCRF_REMOVED			128 //was stupid

/*only read+append+write are standard frik_file*/
#define FRIK_FILE_READ		0 /*read-only*/
#define FRIK_FILE_APPEND	1 /*append (write-only, but offset begins at end of previous file)*/
#define FRIK_FILE_WRITE		2 /*write-only*/
#define FRIK_FILE_INVALID	3 /*no idea what this is for, presume placeholder*/
#define FRIK_FILE_READNL	4 /*fgets ignores newline chars, returning the entire thing in one lump*/
#define FRIK_FILE_MMAP_READ	5 /*fgets returns a pointer. memory is not guarenteed to be released.*/
#define FRIK_FILE_MMAP_RW	6 /*fgets returns a pointer. file is written upon close. memory is not guarenteed to be released.*/

#define MASK_DELTA 1
#define MASK_STDVIEWMODEL 2

enum lightfield_e
{
	lfield_origin=0,
	lfield_colour=1,
	lfield_radius=2,
	lfield_flags=3,
	lfield_style=4,
	lfield_angles=5,
	lfield_fov=6,
	lfield_corona=7,
	lfield_coronascale=8,
	lfield_cubemapname=9,
	lfield_ambientscale=10,
	lfield_diffusescale=11,
	lfield_specularscale=12
};
enum csqc_input_event
{
	/*devid is the player id (on android, its the multitouch id and is always present even in single player)*/
	CSIE_KEYDOWN = 0,		/*syscode, unicode, devid	the two codes are not both guarenteed to be set at the same time, and may happen as separate events*/
	CSIE_KEYUP = 1,			/*syscode, unicode, devid	as keydown, unicode up events are not guarenteed*/
	CSIE_MOUSEDELTA = 2,	/*x, y, devid				mouse motion. x+y are relative*/
	CSIE_MOUSEABS = 3,		/*x, y, devid				*/
	CSIE_ACCELEROMETER = 4,	/*x, y, z*/
	CSIE_FOCUS = 5,			/*mouse, key, devid.		if has, the game window has focus. (true/false/-1)*/
	CSIE_JOYAXIS = 6,		/*axis, value, devid*/
};

enum terrainedit_e
{
	ter_reload,			//
	ter_save,			//
	ter_sethole,		//vector pos, float radius, floatbool hole
	ter_height_set,		//vector pos, float radius, float newheight
	ter_height_smooth,	//vector pos, float radius, float percent
	ter_height_spread,	//vector pos, float radius, float percent
	ter_raise,			//vector pos, float radius, float heightchange
	ter_lower,			//vector pos, float radius, float heightchange
	ter_tex_kill,		//vector pos, void junk, void junk, string texname
	ter_tex_get,		//vector pos, void junk, float imagenum
	ter_tex_blend,		//vector pos, float radius, float percent, string texname
	ter_tex_concentrate,	//vector pos, float radius, float percent
	ter_tex_noise,		//vector pos, float radius, float percent
	ter_tex_blur,		//vector pos, float radius, float percent
	ter_water_set,		//vector pos, float radius, float newwaterheight
	ter_mesh_add,		//entity ent
	ter_mesh_kill,		//vector pos, float radius
	ter_tint,			//vector pos, float radius, float percent, vector newcol, float newalph
	ter_height_flatten,	//vector pos, float radius, float percent
	ter_tex_replace,	//vector pos, float radius, string texname
	ter_reset,			//vector pos, float radius
	ter_reloadsect,		//vector pos, float radius
//	ter_poly_add,		//add a poly, woo
//	ter_poly_remove,	//remove polys

//	ter_autopaint_h,	//vector pos, float radius, float percent, string tex1, string tex2				(paint tex1/tex2
//	ter_autopaint_n	//vector pos, float radius, float percent, string tex1, string tex2
};

enum
{
	GE_MAXENTS			= -1,
	GE_ACTIVE			= 0,
	GE_ORIGIN			= 1,
	GE_FORWARD			= 2,
	GE_RIGHT			= 3,
	GE_UP				= 4,
	GE_SCALE			= 5,
	GE_ORIGINANDVECTORS	= 6,
	GE_ALPHA			= 7,
	GE_COLORMOD			= 8,
	GE_PANTSCOLOR		= 9,
	GE_SHIRTCOLOR		= 10,
	GE_SKIN				= 11,
	GE_MINS				= 12,
	GE_MAXS				= 13,
	GE_ABSMIN			= 14,
	GE_ABSMAX			= 15,
	GE_LIGHT			= 16
};
