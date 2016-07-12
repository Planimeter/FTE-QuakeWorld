#ifdef _WIN32
	#ifndef _CRT_SECURE_NO_WARNINGS
		#define _CRT_SECURE_NO_WARNINGS
	#endif
	#define _CRT_NONSTDC_NO_WARNINGS
	#ifndef _CRT_SECURE_NO_DEPRECATE
		#define _CRT_SECURE_NO_DEPRECATE
	#endif
	#ifndef _CRT_NONSTDC_NO_DEPRECATE
		#define _CRT_NONSTDC_NO_DEPRECATE
	#endif
	#ifndef AVAIL_ZLIB
		#ifdef _MSC_VER
			//#define AVAIL_ZLIB
		#endif
	#endif

	#include <windows.h>
#else
	#include <stdarg.h>
	#include <math.h>

	#include <stdlib.h>
	#include <setjmp.h>
	#include <string.h>
	#include <ctype.h>

	#ifndef __declspec
		#define __declspec(mode)
	#endif
//#define _inline inline
#endif
typedef unsigned char qbyte;
#include <stdio.h>

#define DLL_PROG
#ifndef PROGSUSED
#define PROGSUSED
#endif

#define false 0
#define true 1

#include "progtype.h"
#include "progslib.h"

#include "pr_comp.h"


#ifdef _MSC_VER
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#endif

//extern progfuncs_t *progfuncs;
typedef struct sharedvar_s
{
	int varofs;
	int size;
} sharedvar_t;
typedef struct
{
	mfunction_t		*f;
	unsigned char	stepping;
	unsigned char	progsnum;
	int				s;
	int				pushed;
	unsigned long long	timestamp;
} prstack_t;

typedef struct
{
	unsigned int size;
	char value[4];
} tempstr_t;

//FIXME: the defines hidden inside this structure are evil.
typedef struct prinst_s
 {
	tempstr_t **tempstrings;
	unsigned int maxtempstrings;
	unsigned int numtempstrings;
#ifdef QCGC
	unsigned int nexttempstring;
#else
	unsigned int numtempstringsstack;
#endif

	char **allocedstrings;
	int maxallocedstrings;
	int numallocedstrings;

	struct progstate_s * progstate;
#define pr_progstate prinst.progstate

	progsnum_t pr_typecurrent;	//active index into progstate array. fixme: remove in favour of only using current_progstate
	unsigned int maxprogs;

	struct progstate_s *current_progstate;
#define current_progstate prinst.current_progstate

	char * watch_name;
	eval_t * watch_ptr;
	eval_t watch_old;
	etype_t watch_type;

	unsigned int numshares;
	sharedvar_t *shares;	//shared globals, not including parms
	unsigned int maxshares;

	struct prmemb_s     *memblocks;

	unsigned int maxfields;
	unsigned int numfields;
	fdef_t *field;	//biggest size

	int reorganisefields;


//pr_exec.c
#define	MAX_STACK_DEPTH		1024	//insanely high value requried for xonotic.
	prstack_t pr_stack[MAX_STACK_DEPTH];
#define pr_stack prinst.pr_stack
	int pr_depth;
#define pr_depth prinst.pr_depth
	int spushed;

#define	LOCALSTACK_SIZE		16384
	int localstack[LOCALSTACK_SIZE];
	int localstack_used;

	int debugstatement;
	int continuestatement;
	int exitdepth;

	pbool profiling;
	unsigned long long profilingalert;
	mfunction_t	*pr_xfunction;
#define pr_xfunction prinst.pr_xfunction
	int pr_xstatement;
#define pr_xstatement prinst.pr_xstatement

//pr_edict.c

	unsigned int maxedicts;

	evalc_t spawnflagscache;
	unsigned int fields_size;	// in bytes
	unsigned int max_fields_size;


//initlib.c
	int mfreelist;
	char * addressablehunk;
	size_t addressableused;
	size_t addressablesize;

	struct edictrun_s **edicttable;
} prinst_t;

typedef struct progfuncs_s
{
	struct pubprogfuncs_s funcs;
	struct prinst_s	inst;	//private fields. Leave alone.
} progfuncs_t;

#define prinst progfuncs->inst
#define externs progfuncs->funcs.parms

#include "qcd.h"

#define STRING_SPECMASK	0xc0000000	//
#define STRING_TEMP		0x80000000	//temp string, will be collected.
#define STRING_STATIC	0xc0000000	//pointer to non-qcvm string.
#define STRING_NORMAL_	0x00000000	//stringtable/mutable. should always be a fallthrough
#define STRING_NORMAL2_	0x40000000	//stringtable/mutable. should always be a fallthrough

typedef struct
{
	int			targetflags;	//weather we need to mark the progs as a newer version
	char		*name;
	char		*opname;
	int		priority_;	//FIXME: priority should be done differently...
	enum {ASSOC_LEFT, ASSOC_RIGHT, ASSOC_RIGHT_RESULT}			associative;
	struct QCC_type_s		**type_a, **type_b, **type_c;

	unsigned int flags;
	//ASSIGNS_B
	//ASSIGNS_IB
	//ASSIGNS_C
	//ASSIGNS_IC
} QCC_opcode_t;
extern	QCC_opcode_t	pr_opcodes[];		// sized by initialization




#ifdef _MSC_VER
#define Q_vsnprintf _vsnprintf
#else
#define Q_vsnprintf vsnprintf
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#define sv_num_edicts (*externs->sv_num_edicts)
#define sv_edicts (*externs->sv_edicts)

#define printf externs->Printf
#define Sys_Error externs->Sys_Error

int PRHunkMark(progfuncs_t *progfuncs);
void PRHunkFree(progfuncs_t *progfuncs, int mark);
void *PRHunkAlloc(progfuncs_t *progfuncs, int size, char *name);
void *PRAddressableExtend(progfuncs_t *progfuncs, void *src, size_t srcsize, int pad);

#ifdef printf
#undef LIKEPRINTF
#define LIKEPRINTF(x)
#endif

//void *HunkAlloc (int size);
char *VARGS qcva (char *text, ...) LIKEPRINTF(1);
void QC_InitShares(progfuncs_t *progfuncs);
void QC_StartShares(progfuncs_t *progfuncs);
void PDECL QC_AddSharedVar(pubprogfuncs_t *progfuncs, int num, int type);
void PDECL QC_AddSharedFieldVar(pubprogfuncs_t *progfuncs, int num, char *stringtable);
int PDECL QC_RegisterFieldVar(pubprogfuncs_t *progfuncs, unsigned int type, char *name, signed long requestedpos, signed long originalofs);
pbool PDECL QC_Decompile(pubprogfuncs_t *progfuncs, char *fname);
int PDECL PR_ToggleBreakpoint(pubprogfuncs_t *progfuncs, char *filename, int linenum, int flag);
void    StripExtension (char *path);


#define edvars(ed) (((edictrun_t*)ed)->fields)	//pointer to the field vars, given an edict


void SetEndian(void);
extern short   (*PRBigShort) (short l);
extern short   (*PRLittleShort) (short l);
extern int     (*PRBigLong) (int l);
extern int     (*PRLittleLong) (int l);
extern float   (*PRBigFloat) (float l);
extern float   (*PRLittleFloat) (float l);



/*
#ifndef COMPILER
typedef union eval_s
{
	string_t		string;
	float			_float;
	float			vector[3];
	func_t			function;
	int				_int;
	int				edict;
	progsnum_t		prog;	//so it can easily be changed
} eval_t;
#endif
*/
typedef struct edictrun_s
{
	enum ereftype_e	ereftype;
	float			freetime;			// realtime when the object was freed
	unsigned int	entnum;
	unsigned int	fieldsize;
	pbool			readonly;	//causes error when QC tries writing to it. (quake's world entity)
	void			*fields;

// other fields from progs come immediately after
} edictrun_t;


int PDECL Comp_Begin(pubprogfuncs_t *progfuncs, int nump, char **parms);
int PDECL Comp_Continue(pubprogfuncs_t *progfuncs);

pbool PDECL PR_SetWatchPoint(pubprogfuncs_t *progfuncs, char *key);
char *PDECL PR_EvaluateDebugString(pubprogfuncs_t *progfuncs, char *key);
char *PDECL PR_SaveEnts(pubprogfuncs_t *progfuncs, char *mem, size_t *size, size_t maxsize, int mode);
int PDECL PR_LoadEnts(pubprogfuncs_t *progfuncs, const char *file, float killonspawnflags);
char *PDECL PR_SaveEnt (pubprogfuncs_t *progfuncs, char *buf, size_t *size, size_t maxsize, struct edict_s *ed);
struct edict_s *PDECL PR_RestoreEnt (pubprogfuncs_t *progfuncs, const char *buf, size_t *size, struct edict_s *ed);
void PDECL PR_StackTrace (pubprogfuncs_t *progfuncs, int showlocals);

eval_t *PR_GetReadTempStringPtr(progfuncs_t *progfuncs, string_t str, size_t offset, size_t datasize);
eval_t *PR_GetWriteTempStringPtr(progfuncs_t *progfuncs, string_t str, size_t offset, size_t datasize);

extern int noextensions;

typedef enum
{
	PST_DEFAULT,//everything 16bit
	PST_FTE32,	//everything 32bit
	PST_KKQWSV, //32bit statements, 16bit globaldefs. NO SAVED GAMES.
	PST_QTEST,	//16bit statements, 32bit globaldefs(other differences converted on load)
} progstructtype_t;

#ifndef COMPILER
typedef struct progstate_s
{
	dprograms_t		*progs;
	mfunction_t		*functions;
	char			*strings;
	union {
		ddefXX_t		*globaldefs;
		ddef16_t		*globaldefs16;
		ddef32_t		*globaldefs32;
	};
	union {
		ddefXX_t		*fielddefs;
		ddef16_t		*fielddefs16;
		ddef32_t		*fielddefs32;
	};
	void	*statements;
//	void			*global_struct;
	float			*globals;			// same as pr_global_struct
	int				globals_size;	// in bytes

	typeinfo_t	*types;

	int				edict_size;	// in bytes

	char			filename[128];

	int *linenums;	//debug versions only

	progstructtype_t structtype;

#ifdef QCJIT
	struct jitstate *jit;
#endif
} progstate_t;

typedef struct extensionbuiltin_s {
	char *name;
	builtin_t func;
	struct extensionbuiltin_s *prev;
} extensionbuiltin_t;

//============================================================================


#define pr_progs			current_progstate->progs
#define	pr_cp_functions		current_progstate->functions
#define	pr_strings			current_progstate->strings
#define	pr_globaldefs16		((ddef16_t*)current_progstate->globaldefs16)
#define	pr_globaldefs32		((ddef32_t*)current_progstate->globaldefs32)
#define	pr_fielddefs16		((ddef16_t*)current_progstate->fielddefs16)
#define	pr_fielddefs32		((ddef32_t*)current_progstate->fielddefs32)
#define	pr_statements16		((dstatement16_t*)current_progstate->statements)
#define	pr_statements32		((dstatement32_t*)current_progstate->statements)
//#define	pr_global_struct	current_progstate->global_struct
#define pr_globals			current_progstate->globals
#define pr_linenums			current_progstate->linenums
#define pr_types			current_progstate->types



//============================================================================

void PR_Init (void);

pbool PR_RunWarning (pubprogfuncs_t *progfuncs, char *error, ...);

void PDECL PR_ExecuteProgram (pubprogfuncs_t *progfuncs, func_t fnum);
int PDECL PR_LoadProgs(pubprogfuncs_t *progfncs, const char *s);
int PR_ReallyLoadProgs (progfuncs_t *progfuncs, const char *filename, progstate_t *progstate, pbool complain);

void *PRHunkAlloc(progfuncs_t *progfuncs, int ammount, char *name);

void PR_Profile_f (void);

struct edict_s *PDECL ED_Alloc (pubprogfuncs_t *progfuncs, pbool object, size_t extrasize);
void PDECL ED_Free (pubprogfuncs_t *progfuncs, struct edict_s *ed);

pbool PR_RunGC			(progfuncs_t *progfuncs);
string_t PDECL PR_AllocTempString			(pubprogfuncs_t *ppf, const char *str);
char *PDECL ED_NewString (pubprogfuncs_t *ppf, const char *string, int minlength, pbool demarkup);
// returns a copy of the string allocated from the server's string heap

void PDECL ED_Print (pubprogfuncs_t *progfuncs, struct edict_s *ed);
//void ED_Write (FILE *f, edictrun_t *ed);
const char *ED_ParseEdict (progfuncs_t *progfuncs, const char *data, edictrun_t *ent);

//void ED_WriteGlobals (FILE *f);
void ED_ParseGlobals (char *data);

//void ED_LoadFromFile (char *data);

//define EDICT_NUM(n) ((edict_t *)(sv.edicts+ (n)*pr_edict_size))
//define NUM_FOR_EDICT(e) (((byte *)(e) - sv.edicts)/pr_edict_size)

struct edict_s *PDECL QC_EDICT_NUM(pubprogfuncs_t *progfuncs, unsigned int n);
unsigned int PDECL QC_NUM_FOR_EDICT(pubprogfuncs_t *progfuncs, struct edict_s *e);

#define EDICT_NUM(pf, num)	QC_EDICT_NUM(&pf->funcs,num)
#define NUM_FOR_EDICT(pf, e) QC_NUM_FOR_EDICT(&pf->funcs,e)

//#define	NEXT_EDICT(e) ((edictrun_t *)( (byte *)e + pr_edict_size))

#define	EDICT_TO_PROG(pf, e) (((edictrun_t*)e)->entnum)
#define PROG_TO_EDICT(pf, e) ((struct edictrun_s *)prinst.edicttable[e])

//============================================================================

#define	G_FLOAT(o) (pr_globals[o])
#define	G_FLOAT2(o) (pr_globals[OFS_PARM0 + o*3])
#define	G_INT(o) (*(int *)&pr_globals[o])
#define	G_EDICT(o) ((edict_t *)((qbyte *)sv_edicts+ *(int *)&pr_globals[o]))
#define G_EDICTNUM(o) NUM_FOR_EDICT(G_EDICT(o))
#define	G_VECTOR(o) (&pr_globals[o])
#define	G_STRING(o) (*(string_t *)&pr_globals[o])
#define G_STRING2(o) ((char*)*(string_t *)&pr_globals[o])
#define	GQ_STRING(o) (*(QCC_string_t *)&pr_globals[o])
#define GQ_STRING2(o) ((char*)*(QCC_string_t *)&pr_globals[o])
#define	G_FUNCTION(o) (*(func_t *)&pr_globals[o])
#define G_PROG(o) G_FLOAT(o)	//simply so it's nice and easy to change...

#define	RETURN_EDICT(e) (((int *)pr_globals)[OFS_RETURN] = EDICT_TO_PROG(e))

#define	E_FLOAT(e,o) (((float*)&e->v)[o])
#define	E_INT(e,o) (*(int *)&((float*)&e->v)[o])
#define	E_VECTOR(e,o) (&((float*)&e->v)[o])
#define	E_STRING(e,o) (*(string_t *)&((float*)(e+1))[o])

const extern	unsigned int		type_size[];


extern	unsigned short		pr_crc;

void VARGS PR_RunError (pubprogfuncs_t *progfuncs, char *error, ...) LIKEPRINTF(2);

void ED_PrintEdicts (progfuncs_t *progfuncs);
void ED_PrintNum (progfuncs_t *progfuncs, int ent);


pbool PR_SwitchProgs(progfuncs_t *progfuncs, progsnum_t type);
pbool PR_SwitchProgsParms(progfuncs_t *progfuncs, progsnum_t newprogs);




eval_t *PDECL QC_GetEdictFieldValue(pubprogfuncs_t *progfuncs, struct edict_s *ed, char *name, etype_t type, evalc_t *cache);
void PDECL PR_GenerateStatementString (pubprogfuncs_t *progfuncs, int statementnum, char *out, int outlen);
fdef_t *PDECL ED_FieldInfo (pubprogfuncs_t *progfuncs, unsigned int *count);
char *PDECL PR_UglyValueString (pubprogfuncs_t *progfuncs, etype_t type, eval_t *val);
pbool	PDECL ED_ParseEval (pubprogfuncs_t *progfuncs, eval_t *eval, int type, const char *s);

unsigned long long Sys_GetClockRate(void);
#endif




#ifndef COMPILER

//this is windows - all files are written with this endian standard
//optimisation
//leave undefined if in doubt over os.
#ifdef _WIN32
#define NOENDIAN
#endif




//pr_multi.c

extern vec3_t vec3_origin;

struct qcthread_s *PDECL PR_ForkStack	(pubprogfuncs_t *progfuncs);
void PDECL PR_ResumeThread			(pubprogfuncs_t *progfuncs, struct qcthread_s *thread);
void	PDECL PR_AbortStack			(pubprogfuncs_t *progfuncs);
pbool	PDECL PR_GetBuiltinCallInfo	(pubprogfuncs_t *ppf, int *builtinnum, char *function, size_t sizeoffunction);

eval_t *PDECL PR_FindGlobal(pubprogfuncs_t *prfuncs, const char *globname, progsnum_t pnum, etype_t *type);
ddef16_t *ED_FindTypeGlobalFromProgs16 (progfuncs_t *progfuncs, const char *name, progsnum_t prnum, int type);
ddef32_t *ED_FindTypeGlobalFromProgs32 (progfuncs_t *progfuncs, const char *name, progsnum_t prnum, int type);
ddef16_t *ED_FindGlobalFromProgs16 (progfuncs_t *progfuncs, const char *name, progsnum_t prnum);
ddef32_t *ED_FindGlobalFromProgs32 (progfuncs_t *progfuncs, const char *name, progsnum_t prnum);
fdef_t *ED_FindField (progfuncs_t *progfuncs, const char *name);
fdef_t *ED_ClassFieldAtOfs (progfuncs_t *progfuncs, unsigned int ofs, const char *classname);
fdef_t *ED_FieldAtOfs (progfuncs_t *progfuncs, unsigned int ofs);
mfunction_t *ED_FindFunction (progfuncs_t *progfuncs, const char *name, progsnum_t *pnum, progsnum_t fromprogs);
func_t PDECL PR_FindFunc(pubprogfuncs_t *progfncs, const char *funcname, progsnum_t pnum);
//void PDECL PR_Configure (pubprogfuncs_t *progfncs, size_t addressable_size, int max_progs);
int PDECL PR_InitEnts(pubprogfuncs_t *progfncs, int maxents);
char *PR_ValueString (progfuncs_t *progfuncs, etype_t type, eval_t *val, pbool verbose);
void PDECL QC_ClearEdict (pubprogfuncs_t *progfuncs, struct edict_s *ed);
void PRAddressableFlush(progfuncs_t *progfuncs, size_t totalammount);
void QC_FlushProgsOffsets(progfuncs_t *progfuncs);

ddef16_t *ED_GlobalAtOfs16 (progfuncs_t *progfuncs, int ofs);
ddef16_t *ED_FindGlobal16 (progfuncs_t *progfuncs, char *name);
ddef32_t *ED_FindGlobal32 (progfuncs_t *progfuncs, char *name);
ddef32_t *ED_GlobalAtOfs32 (progfuncs_t *progfuncs, unsigned int ofs);

string_t PDECL PR_StringToProgs			(pubprogfuncs_t *inst, const char *str);
const char *ASMCALL PR_StringToNative				(pubprogfuncs_t *inst, string_t str);

void PR_FreeTemps			(progfuncs_t *progfuncs, int depth);

char *PR_GlobalString (progfuncs_t *progfuncs, int ofs);
char *PR_GlobalStringNoContents (progfuncs_t *progfuncs, int ofs);

pbool CompileFile(progfuncs_t *progfuncs, const char *filename);

struct jitstate;
struct jitstate *PR_GenerateJit(progfuncs_t *progfuncs);
void PR_EnterJIT(progfuncs_t *progfuncs, struct jitstate *jitstate, int statement);
void PR_CloseJit(struct jitstate *jit);

char *QCC_COM_Parse (const char *data);
extern char	qcc_token[1024];
extern char *basictypenames[];
#endif
