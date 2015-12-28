#define COMPILER
#define PROGSUSED

//#define COMMONINLINES
//#define inline _inline

#include "cmdlib.h"
#include <setjmp.h>
/*
#include <stdio.h>
#include <conio.h>


#include "pr_comp.h"
*/

//this is for testing
#define WRITEASM

#ifdef __MINGW32_VERSION
#define MINGW
#endif

#define progfuncs qccprogfuncs
extern progfuncs_t *qccprogfuncs;

#ifndef _WIN32
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

void *qccHunkAlloc(size_t mem);
void qccClearHunk(void);

extern short   (*PRBigShort) (short l);
extern short   (*PRLittleShort) (short l);
extern int     (*PRBigLong) (int l);
extern int     (*PRLittleLong) (int l);
extern float   (*PRBigFloat) (float l);
extern float   (*PRLittleFloat) (float l);


#define	MAX_ERRORS		10

#define	MAX_NAME		256		// chars long

extern unsigned int MAX_REGS;
extern unsigned int MAX_LOCALS;
extern unsigned int MAX_TEMPS;

extern int	MAX_STRINGS;
extern int	MAX_GLOBALS;
extern int	MAX_FIELDS;
extern int	MAX_STATEMENTS;
extern int	MAX_FUNCTIONS;

#define	QCC_MAX_SOUNDS		1024	//convert to int?
#define QCC_MAX_TEXTURES	1024	//convert to int?
#define	QCC_MAX_MODELS		1024	//convert to int?
#define	QCC_MAX_FILES		1024	//convert to int?
#define	MAX_DATA_PATH	64

extern int MAX_CONSTANTS;
#define MAXCONSTANTNAMELENGTH 64
#define MAXCONSTANTPARAMLENGTH 32
#define MAXCONSTANTPARAMS 32

typedef enum {QCF_STANDARD, QCF_HEXEN2, QCF_DARKPLACES, QCF_FTE, QCF_FTEDEBUG, QCF_FTEH2, QCF_KK7, QCF_QTEST} qcc_targetformat_t;
extern qcc_targetformat_t qcc_targetformat;


/*

TODO:

"stopped at 10 errors"

other pointer types for models and clients?

compact string heap?

always initialize all variables to something safe

the def->type->type arrangement is really silly.

return type checking

parm count type checking

immediate overflow checking

pass the first two parms in call->b and call->c

*/

/*

comments
--------
// comments discard text until the end of line
/ *  * / comments discard all enclosed text (spaced out on this line because this documentation is in a regular C comment block, and typing them in normally causes a parse error)

code structure
--------------
A definition is:
	<type> <name> [ = <immediate>] {, <name> [ = <immediate>] };


types
-----
simple types: void, float, vector, string, or entity
	float		width, height;
	string		name;
	entity		self, other;

vector types:
	vector		org;	// also creates org_x, org_y, and org_z float defs
	
	
A function type is specified as: 	simpletype ( type name {,type name} )
The names are ignored except when the function is initialized.	
	void()		think;
	entity()	FindTarget;
	void(vector destination, float speed, void() callback)	SUB_CalcMove;
	void(...)	dprint;		// variable argument builtin

A field type is specified as:  .type
	.vector		origin;
	.string		netname;
	.void()		think, touch, use;
	

names
-----
Names are a maximum of 64 characters, must begin with A-Z,a-z, or _, and can continue with those characters or 0-9.

There are two levels of scoping: global, and function.  The parameter list of a function and any vars declared inside a function with the "local" statement are only visible within that function, 


immediates
----------
Float immediates must begin with 0-9 or minus sign.  .5 is illegal.
	
A parsing ambiguity is present with negative constants. "a-5" will be parsed as "a", then "-5", causing an error.  Seperate the - from the digits with a space "a - 5" to get the proper behavior.
	12
	1.6
	0.5
	-100

Vector immediates are three float immediates enclosed in single quotes.
	'0 0 0'
	'20.5 -10 0.00001'
	
String immediates are characters enclosed in double quotes.  The string cannot contain explicit newlines, but the escape character \n can embed one.  The \" escape can be used to include a quote in the string.
	"maps/jrwiz1.bsp"
	"sound/nin/pain.wav"
	"ouch!\n"

Code immediates are statements enclosed in {} braces.
statement:
	{ <multiple statements> }
	<expression>;
	local <type> <name> [ = <immediate>] {, <name> [ = <immediate>] };
	return <expression>;
	if ( <expression> ) <statement> [ else <statement> ];
	while ( <expression> ) <statement>;
	do <statement> while ( <expression> );
	<function name> ( <function parms> );
	
expression:
	combiations of names and these operators with standard C precedence:
	"&&", "||", "<=", ">=","==", "!=", "!", "*", "/", "-", "+", "=", ".", "<", ">", "&", "|"
	Parenthesis can be used to alter order of operation.
	The & and | operations perform integral bit ops on floats
	
A built in function immediate is a number sign followed by an integer.
	#1
	#12


compilation
-----------
Source files are processed sequentially without dumping any state, so if a defs file is the first one processed, the definitions will be available to all other files.

The language is strongly typed and there are no casts.

Anything that is initialized is assumed to be constant, and will have immediates folded into it.  If you change the value, your program will malfunction.  All uninitialized globals will be saved to savegame files.

Functions cannot have more than eight parameters.

Error recovery during compilation is minimal.  It will skip to the next global definition, so you will never see more than one error at a time in a given function.  All compilation aborts after ten error messages.

Names can be defined multiple times until they are defined with an initialization, allowing functions to be prototyped before their definition.

void()	MyFunction;			// the prototype

void()	MyFunction =		// the initialization
{
	dprint ("we're here\n");
};


entities and fields
-------------------


execution
---------
Code execution is initiated by C code in quake from two main places:  the timed think routines for periodic control, and the touch function when two objects impact each other.

There are three global variables that are set before beginning code execution:
	entity	world;		// the server's world object, which holds all global
						// state for the server, like the deathmatch flags
						// and the body ques.
	entity	self;		// the entity the function is executing for
	entity	other;		// the other object in an impact, not used for thinks
	float	time;		// the current game time.  Note that because the
						// entities in the world are simulated sequentially,
						// time is NOT strictly increasing.  An impact late
						// in one entity's time slice may set time higher
						// than the think function of the next entity. 
						// The difference is limited to 0.1 seconds.
Execution is also caused by a few uncommon events, like the addition of a new client to an existing server.
	
There is a runnaway counter that stops a program if 100000 statements are executed, assuming it is in an infinite loop.

It is acceptable to change the system set global variables.  This is usually done to pose as another entity by changing self and calling a function.

The interpretation is fairly efficient, but it is still over an order of magnitude slower than compiled C code.  All time consuming operations should be made into built in functions.

A profile counter is kept for each function, and incremented for each interpreted instruction inside that function.  The "profile" console command in Quake will dump out the top 10 functions, then clear all the counters.  The "profile all" command will dump sorted stats for every function that has been executed.


afunc ( 4, bfunc(1,2,3));
will fail because there is a shared parameter marshaling area, which will cause the 1 from bfunc to overwrite the 4 already placed in parm0.  When a function is called, it copies the parms from the globals into it's privately scoped variables, so there is no collision when calling another function.

total = factorial(3) + factorial(4);
Will fail because the return value from functions is held in a single global area.  If this really gets on your nerves, tell me and I can work around it at a slight performance and space penalty by allocating a new register for the function call and copying it out.


built in functions
------------------
void(string text)	dprint;
Prints the string to the server console.

void(entity client, string text)	cprint;
Prints a message to a specific client.

void(string text)	bprint;
Broadcast prints a message to all clients on the current server.

entity()	spawn;
Returns a totally empty entity.  You can manually set everything up, or just set the origin and call one of the existing entity setup functions.

entity(entity start, .string field, string match) find;
Searches the server entity list beginning at start, looking for an entity that has entity.field = match.  To start at the beginning of the list, pass world.  World is returned when the end of the list is reached.

<FIXME: define all the other functions...>


gotchas
-------

The && and || operators DO NOT EARLY OUT like C!

Don't confuse single quoted vectors with double quoted strings

The function declaration syntax takes a little getting used to.

Don't forget the ; after the trailing brace of a function initialization.

Don't forget the "local" before defining local variables.

There are no ++ / -- operators, or operate/assign operators.

*/


#if 1
#include "hash.h"
extern hashtable_t compconstantstable;
extern hashtable_t globalstable, localstable;
#endif

#ifdef WRITEASM
extern FILE *asmfile;
extern pbool asmfilebegun;
#endif
//=============================================================================

// offsets are always multiplied by 4 before using
typedef unsigned int	gofs_t;				// offset in global data block
typedef struct QCC_function_s QCC_function_t;

#define	MAX_PARMS	8

//keep this sizeof(float)
typedef union QCC_eval_s
{
	QCC_string_t			string;
	float				_float;
	float				vector[1];
	func_t				function;
	int					_int;
//	union QCC_eval_s		*ptr;
} QCC_eval_t;

//must be the maximum size possible for a single basic type.
typedef union QCC_evalstorage_s
{
	QCC_string_t			string;
	float				_float;
	float				vector[3];
	func_t				function;
	int					_int;
//	union QCC_eval_s		*ptr;
} QCC_evalstorage_t;

struct QCC_typeparam_s
{
	struct QCC_type_s *type;
	pbool optional;
	pbool out;
	unsigned int ofs;
	unsigned int arraysize;
	char *paramname;
};
struct accessor_s
{
	struct accessor_s *next;
	struct QCC_type_s *type;
	struct QCC_type_s *indexertype;	//null if not indexer
	QCC_sref_t getset_func[2];
	pbool getset_isref[2];
	char *fieldname;
};

typedef struct QCC_type_s
{
	etype_t			type;

	struct QCC_type_s	*parentclass;	//type_entity...
// function types are more complex
	struct QCC_type_s	*aux_type;	// return type or field type
	
	struct QCC_typeparam_s *params; //[num_parms]
	unsigned int		num_parms;

	unsigned int size;
	pbool typedefed:1;
	pbool vargs:1;
	pbool vargcount:1;
	char *name;
	char *aname;

	struct accessor_s *accessors;

	struct QCC_type_s *ptrto;	//this points to a type that is a pointer back to this type. yeah, weird.
} QCC_type_t;
int typecmp(QCC_type_t *a, QCC_type_t *b);
int typecmp_lax(QCC_type_t *a, QCC_type_t *b);

typedef struct temp_s temp_t;
void QCC_PurgeTemps(void);
void QCC_FinaliseTemps(void);

//not written
typedef struct QCC_def_s
{
	QCC_type_t		*type;
	char		*name;
	char		*comment;		//ui info
	struct QCC_def_s	*next;
	struct QCC_def_s	*nextlocal;	//provides a chain of local variables for the opt_locals_marshalling optimisation.
	gofs_t		ofs;
	struct QCC_function_s	*scope;		// function the var was defined in, or NULL
	struct QCC_def_s	*deftail;	// arrays and structs create multiple globaldef objects providing different types at the different parts of the single object (struct), or alternative names (vectors). this allows us to correctly set the const type based upon how its initialised.
	struct QCC_def_s	*generatedfor;
	int			initialized;	// 1 when a declaration included "= immediate". 2 = extern. 3 = don't warn (unless actually called)
	int			constant;		// 1 says we can use the value over and over again

	struct QCC_def_s	*symbolheader;	//this is the original symbol within which the def is stored.
	union QCC_eval_s	*symboldata;	//null if uninitialised.
	unsigned int		symbolsize;		//total byte size of symbol

	int refcount;			//if 0, temp can be reused. tracked on globals too in order to catch bugs that would otherwise be a little too obscure.
	int timescalled;	//part of the opt_stripfunctions optimisation.

	int s_file;
	int s_line;

	int arraysize;
	//should really use proper flags
	pbool funccalled:1;			//was called somewhere.
	pbool read:1;			//variable was read
	pbool written:1;		//variable was written
	pbool referenced:1;		//was used somewhere in the code (even if it can still be stripped). this controls warnings only.
	pbool shared:1;			//weird multiprogs flag thing.
	pbool saved:1;			//def may be saved to saved games.
	pbool isstatic:1;		//global, even if scoped. also specific to the file it was seen in.
	pbool subscoped_away:1;	//this local is no longer linked into the locals hash table. don't do remove it twice.
//	pbool followptr:1;
	pbool strip:1;			//info about this def should be stripped. it may still consume globals space however, and its storage can still be used, its just not visible.
	pbool allowinline:1;	//calls to this function will attempt to inline the specified function. requires const, supposedly.
	pbool used:1;			//if it remains 0, it may be stripped. this is forced for functions and fields. commonly 0 on fields.
	pbool localscope:1;		//is a local, as opposed to a static (which is only visible within its scope)

	int	fromstatement;		//statement that it is valid from.
	temp_t *temp;
} QCC_def_t;

struct temp_s {
	QCC_def_t *def;
	unsigned char locked;
	unsigned int size;

	struct QCC_function_s *lastfunc;
	unsigned int laststatement;
};
extern size_t tempsused;

typedef struct
{
	enum{
		REF_GLOBAL,	//(global.ofs)				- use vector[2] is an array ref or vector_z
		REF_ARRAY,	//(global.ofs+wordoffset)	- constant offsets should be direct references, variable offsets will generally result in function calls
		REF_ARRAYHEAD,//(global)				- like REF_ARRAY, but otherwise convert to a pointer.
		REF_POINTER,//*(pointerdef+wordindex)	- maths...
		REF_FIELD,	//(entity.field)			- reading is a single load, writing requires address+storep
		REF_STRING,	//"hello"[1]=='e'			- special opcodes, or str2chr builtin, or something
		REF_NONVIRTUAL,	//(global.ofs)			- identical to global except for function calls, where index can be used to provide the 'newself' for the call.
		REF_ACCESSOR //buf_create()[5]
	} type;

	QCC_sref_t base;
	QCC_sref_t index;
	QCC_type_t *cast;	//entity.float is float, not pointer.
	struct accessor_s *accessor;	//the accessor field of base that we're trying to use
	int		postinc;	//+1 or -1
	pbool	readonly;	//for whatever reason, like base being a const
} QCC_ref_t;

//============================================================================

// pr_loc.h -- program local defs


//=============================================================================
extern char QCC_copyright[1024];
extern char QCC_Packname[5][128];
extern int QCC_packid;

const extern	unsigned int		type_size[];
//extern	QCC_def_t	*def_for_type[9];

extern	QCC_type_t	*type_void, *type_string, *type_float, *type_vector, *type_entity, *type_field, *type_function, *type_floatfunction, *type_pointer, *type_floatpointer, *type_intpointer, *type_integer, *type_variant, *type_floatfield;
extern char *basictypenames[];

struct QCC_function_s
{
	int					builtin;	// the builtin number. >= 0
	int					code;		// first statement. if -1, is a builtin.
	string_t			s_file;		// source file with definition
	const char			*file;
	int					line;
	char				*name;		//internal name of function
	struct QCC_function_s *parentscope;	//for nested functions
	struct QCC_type_s	*type;		//same as the def's type
	struct QCC_def_s	*def;
	struct QCC_def_s	*firstlocal;
	pbool				privatelocals;	//false means locals may overlap with other functions, true is needed for compat if stuff is uninitialised.
//	unsigned int		parm_ofs[MAX_PARMS];	// always contiguous, right?
};


//
// output generated by prog parsing
//
typedef struct
{
	char		*memory;
	int			max_memory;
	int			current_memory;
	QCC_type_t		*types;
	
	QCC_def_t		def_head;		// unused head of linked list
	QCC_def_t		*def_tail;		// add new defs after this and move it
	QCC_def_t		local_head;		// chain of variables which need to be pushed and stuff (head unused).
	QCC_def_t		*local_tail;	// add new defs after this and move it
	
	unsigned int	size_fields;
} QCC_pr_info_t;

extern	QCC_pr_info_t	pr;


typedef struct
{
	char name[MAXCONSTANTNAMELENGTH];
	char *value;
	char params[MAXCONSTANTPARAMS][MAXCONSTANTPARAMLENGTH];
	int numparams;
	pbool used:1;
	pbool inside:1;
	pbool evil:1;
	pbool varg:1;

	int namelen;
} CompilerConstant_t;
extern CompilerConstant_t *CompilerConstant;

//============================================================================

extern	pbool	pr_dumpasm;

//extern	QCC_def_t		**pr_global_defs;	// to find def for a global variable

typedef enum {
tt_eof,			// end of file reached
tt_name, 		// an alphanumeric name token
tt_punct, 		// code punctuation
tt_immediate,	// string, float, vector
} token_type_t;

extern	char		pr_token[8192];
extern	token_type_t	pr_token_type;
extern	int				pr_token_line;
extern	int				pr_token_line_last;
extern	QCC_type_t		*pr_immediate_type;
extern	QCC_evalstorage_t		pr_immediate;

extern pbool keyword_asm;
extern pbool keyword_break;
extern pbool keyword_case;
extern pbool keyword_class;
extern pbool keyword_const;
extern pbool keyword_inout;
extern pbool keyword_optional;
extern pbool keyword_continue;
extern pbool keyword_default;
extern pbool keyword_do;
extern pbool keyword_entity;
extern pbool keyword_float;
extern pbool keyword_for;
extern pbool keyword_goto;
extern pbool keyword_int;
extern pbool keyword_integer;
extern pbool keyword_state;
extern pbool keyword_string;
extern pbool keyword_struct;
extern pbool keyword_switch;
extern pbool keyword_thinktime;
extern pbool keyword_loop;
extern pbool keyword_until;
extern pbool keyword_var;
extern pbool keyword_vector;
extern pbool keyword_union;
extern pbool keyword_enum;	//kinda like in c, but typedef not supported.
extern pbool keyword_enumflags;	//like enum, but doubles instead of adds 1.
extern pbool keyword_typedef;	//fixme
extern pbool keyword_extern;	//function is external, don't error or warn if the body was not found
extern pbool keyword_shared;	//mark global to be copied over when progs changes (part of FTE_MULTIPROGS)
extern pbool keyword_noref;	//nowhere else references this, don't strip it.
extern pbool keyword_nosave;	//don't write the def to the output.
extern pbool keyword_inline;	//don't write the def to the output.
extern pbool keyword_strip;	//don't write the def to the output.
extern pbool keyword_union;	//you surly know what a union is!

extern pbool keyword_unused;
extern pbool keyword_used;
extern pbool keyword_static;
extern pbool keyword_nonstatic;
extern pbool keyword_ignore;

extern pbool keywords_coexist;
extern pbool output_parms;
extern pbool autoprototype, autoprototyped, parseonly;
extern pbool pr_subscopedlocals;
extern pbool flag_nullemptystr, flag_ifstring, flag_brokenifstring, flag_iffloat, flag_ifvector, flag_vectorlogic;
extern pbool flag_acc;
extern pbool flag_caseinsensitive;
extern pbool flag_laxcasts;
extern pbool flag_hashonly;
extern pbool flag_fasttrackarrays;
extern pbool flag_assume_integer;
extern pbool flag_msvcstyle;
extern pbool flag_debugmacros;
extern pbool flag_filetimes;
extern pbool flag_typeexplicit;
extern pbool flag_noboundchecks;
extern pbool flag_brokenarrays;
extern pbool flag_rootconstructor;
extern pbool flag_guiannotate;

extern pbool opt_overlaptemps;
extern pbool opt_shortenifnots;
extern pbool opt_noduplicatestrings;
extern pbool opt_constantarithmatic;
extern pbool opt_nonvec_parms;
extern pbool opt_constant_names;
extern pbool opt_precache_file;
extern pbool opt_filenames;
extern pbool opt_assignments;
extern pbool opt_unreferenced;
extern pbool opt_function_names;
extern pbool opt_locals;
extern pbool opt_dupconstdefs;
extern pbool opt_constant_names_strings;
extern pbool opt_return_only;
extern pbool opt_compound_jumps;
//extern pbool opt_comexprremoval;
extern pbool opt_stripfunctions;
extern pbool opt_locals_overlapping;
extern pbool opt_logicops;
extern pbool opt_vectorcalls;
extern pbool opt_classfields;

extern int optres_shortenifnots;
extern int optres_overlaptemps;
extern int optres_noduplicatestrings;
extern int optres_constantarithmatic;
extern int optres_nonvec_parms;
extern int optres_constant_names;
extern int optres_precache_file;
extern int optres_filenames;
extern int optres_assignments;
extern int optres_unreferenced;
extern int optres_function_names;
extern int optres_locals;
extern int optres_dupconstdefs;
extern int optres_constant_names_strings;
extern int optres_return_only;
extern int optres_compound_jumps;
//extern int optres_comexprremoval;
extern int optres_stripfunctions;
extern int optres_locals_overlapping;
extern int optres_logicops;
extern int optres_inlines;

pbool CompileParams(progfuncs_t *progfuncs, void(*cb)(void), int nump, char **parms);

void QCC_PR_PrintStatement (QCC_statement_t *s);

void QCC_PR_Lex (void);
// reads the next token into pr_token and classifies its type

QCC_type_t *QCC_PR_NewType (char *name, int basictype, pbool typedefed);
QCC_type_t *QCC_PointerTypeTo(QCC_type_t *type);
QCC_type_t *QCC_PR_ParseType (int newtype, pbool silentfail);
extern pbool type_inlinefunction;
QCC_type_t *QCC_TypeForName(char *name);
QCC_type_t *QCC_PR_ParseFunctionType (int newtype, QCC_type_t *returntype);
QCC_type_t *QCC_PR_ParseFunctionTypeReacc (int newtype, QCC_type_t *returntype);
char *QCC_PR_ParseName (void);
CompilerConstant_t *QCC_PR_DefineName(char *name);

int QCC_PR_IntConstExpr(void);

#ifndef COMMONINLINES
pbool QCC_PR_CheckImmediate (const char *string);
pbool QCC_PR_CheckToken (const char *string);
pbool QCC_PR_CheckName (const char *string);
void QCC_PR_Expect (const char *string);
pbool QCC_PR_CheckKeyword(int keywordenabled, const char *string);
#endif
pbool QCC_PR_CheckTokenComment(const char *string, char **comment);
void VARGS QCC_PR_ParseError (int errortype, const char *error, ...);
pbool VARGS QCC_PR_ParseWarning (int warningtype, const char *error, ...);
pbool VARGS QCC_PR_Warning (int type, const char *file, int line, const char *error, ...);
void VARGS QCC_PR_Note (int type, const char *file, int line, const char *error, ...);
void QCC_PR_ParsePrintDef (int warningtype, QCC_def_t *def);
void QCC_PR_ParsePrintSRef (int warningtype, QCC_sref_t sref);
void VARGS QCC_PR_ParseErrorPrintDef (int errortype, QCC_def_t *def, const char *error, ...);
void VARGS QCC_PR_ParseErrorPrintSRef (int errortype, QCC_sref_t sref, const char *error, ...);

int QCC_WarningForName(const char *name);
char *QCC_NameForWarning(int idx);

//QccMain.c must be changed if this is changed.
enum {
	WARN_DEBUGGING,
	WARN_ERROR,
	WARN_NOTREFERENCED,
	WARN_NOTREFERENCEDCONST,
	WARN_CONFLICTINGRETURNS,
	WARN_TOOFEWPARAMS,
	WARN_TOOMANYPARAMS,
	WARN_UNEXPECTEDPUNCT,
	WARN_UNINITIALIZED,
	WARN_ASSIGNMENTTOCONSTANT,
	WARN_ASSIGNMENTTOCONSTANTFUNC,
	WARN_MISSINGRETURNVALUE,
	WARN_WRONGRETURNTYPE,
	WARN_CORRECTEDRETURNTYPE,
	WARN_POINTLESSSTATEMENT,
	WARN_MISSINGRETURN,
	WARN_DUPLICATEDEFINITION,
	WARN_UNDEFNOTDEFINED,
	WARN_PRECOMPILERMESSAGE,
	WARN_TOOMANYPARAMETERSFORFUNC,
	WARN_TOOMANYPARAMETERSVARARGS,
	WARN_STRINGTOOLONG,
	WARN_BADTARGET,
	WARN_BADPRAGMA,
	WARN_NOTUTF8,
	WARN_HANGINGSLASHR,
	WARN_NOTDEFINED,
	WARN_NOTCONSTANT,
	WARN_SWITCHTYPEMISMATCH,
	WARN_CONFLICTINGUNIONMEMBER,
	WARN_KEYWORDDISABLED,
	WARN_ENUMFLAGS_NOTINTEGER,
	WARN_ENUMFLAGS_NOTBINARY,
	WARN_CASEINSENSITIVEFRAMEMACRO,
	WARN_DUPLICATELABEL,
	WARN_DUPLICATEMACRO,
	WARN_ASSIGNMENTINCONDITIONAL,
	WARN_MACROINSTRING,
	WARN_BADPARAMS,
	WARN_IMPLICITCONVERSION,
	WARN_EXTRAPRECACHE,
	WARN_NOTPRECACHED,
	WARN_DEADCODE,
	WARN_UNREACHABLECODE,
	WARN_NOTSTANDARDBEHAVIOUR,
	WARN_DUPLICATEPRECOMPILER,
	WARN_IDENTICALPRECOMPILER,
	WARN_FTE_SPECIFIC,	//extension that only FTEQCC will have a clue about.
	WARN_EXTENSION_USED,	//extension that frikqcc also understands
	WARN_IFSTRING_USED,
	WARN_LAXCAST,	//some errors become this with a compiler flag
	WARN_TYPEMISMATCHREDECOPTIONAL,
	WARN_UNDESIRABLECONVENTION,
	WARN_SAMENAMEASGLOBAL,
	WARN_CONSTANTCOMPARISON,
	WARN_UNSAFEFUNCTIONRETURNTYPE,
	WARN_MISSINGOPTIONAL,
	WARN_SYSTEMCRC,
	WARN_CONDITIONALTYPEMISMATCH,
	WARN_MISSINGMEMBERQUALIFIER,//virtual/static/nonvirtual qualifier is missing
	WARN_SELFNOTTHIS,			//warned for because 'self' does not have the right type. we convert such references to 'this' instead, which is more usable.
	WARN_EVILPREPROCESSOR,		//exploited by nexuiz, and generally unsafe.
	WARN_UNARYNOTSCOPE,			//!foo & bar  the ! applies to the result of &. This is unlike C.
	WARN_STRICTTYPEMISMATCH,	//self.think = T_Damage; both are functions, but the arguments/return types/etc differ.
	WARN_MISUSEDAUTOCVAR,		//various issues with autocvar definitions.
	WARN_IGNORECOMMANDLINE,
	WARN_COMPATIBILITYHACK,		//work around old defs.qc or invalid dpextensions.qc

	ERR_PARSEERRORS,	//caused by qcc_pr_parseerror being called.

	//these are definatly my fault...
	ERR_INTERNAL,
	ERR_TOOCOMPLEX,
	ERR_BADOPCODE,
	ERR_TOOMANYSTATEMENTS,
	ERR_TOOMANYSTRINGS,
	ERR_BADTARGETSWITCH,
	ERR_TOOMANYTYPES,
	ERR_TOOMANYPAKFILES,
	ERR_PRECOMPILERCONSTANTTOOLONG,
	ERR_MACROTOOMANYPARMS,
	ERR_TOOMANYFRAMEMACROS,

	//limitations, some are imposed by compiler, some arn't.
	ERR_TOOMANYGLOBALS,
	ERR_TOOMANYGOTOS,
	ERR_TOOMANYBREAKS,
	ERR_TOOMANYCONTINUES,
	ERR_TOOMANYCASES,
	ERR_TOOMANYLABELS,
	ERR_TOOMANYOPENFILES,
	ERR_TOOMANYTOTALPARAMETERS,

	//these are probably yours, or qcc being fussy.
	ERR_BADEXTENSION,
	ERR_BADIMMEDIATETYPE,
	ERR_NOOUTPUT,
	ERR_NOTAFUNCTION,
	ERR_FUNCTIONWITHVARGS,
	ERR_BADHEX,
	ERR_UNKNOWNPUCTUATION,
	ERR_EXPECTED,
	ERR_NOTANAME,
	ERR_NAMETOOLONG,
	ERR_NOFUNC,
	ERR_COULDNTOPENFILE,
	ERR_NOTFUNCTIONTYPE,
	ERR_TOOFEWPARAMS,
	ERR_TOOMANYPARAMS,
	ERR_CONSTANTNOTDEFINED,
	ERR_BADFRAMEMACRO,
	ERR_TYPEMISMATCH,
	ERR_TYPEMISMATCHREDEC,
	ERR_TYPEMISMATCHPARM,
	ERR_TYPEMISMATCHARRAYSIZE,
	ERR_UNEXPECTEDPUNCTUATION,
	ERR_NOTACONSTANT,
	ERR_REDECLARATION,
	ERR_INITIALISEDLOCALFUNCTION,
	ERR_NOTDEFINED,
	ERR_ARRAYNEEDSSIZE,
	ERR_ARRAYNEEDSBRACES,
	ERR_TOOMANYINITIALISERS,
	ERR_TYPEINVALIDINSTRUCT,
	ERR_NOSHAREDLOCALS,
	ERR_TYPEWITHNONAME,
	ERR_BADARRAYSIZE,
	ERR_NONAME,
	ERR_SHAREDINITIALISED,
	ERR_UNKNOWNVALUE,
	ERR_BADARRAYINDEXTYPE,
	ERR_NOVALIDOPCODES,
	ERR_MEMBERNOTVALID,
	ERR_BADPLUSPLUSOPERATOR,
	ERR_BADNOTTYPE,
	ERR_BADTYPECAST,
	ERR_MULTIPLEDEFAULTS,
	ERR_CASENOTIMMEDIATE,
	ERR_BADSWITCHTYPE,
	ERR_BADLABELNAME,
	ERR_NOLABEL,
	ERR_THINKTIMETYPEMISMATCH,
	ERR_STATETYPEMISMATCH,
	ERR_BADBUILTINIMMEDIATE,
	ERR_PARAMWITHNONAME,
	ERR_BADPARAMORDER,
	ERR_ILLEGALCONTINUES,
	ERR_ILLEGALBREAKS,
	ERR_ILLEGALCASES,
	ERR_NOTANUMBER,
	ERR_WRONGSUBTYPE,
	ERR_EOF,
	ERR_NOPRECOMPILERIF,
	ERR_NOENDIF,
	ERR_HASHERROR,
	ERR_NOTATYPE,
	ERR_TOOMANYPACKFILES,
	ERR_INVALIDVECTORIMMEDIATE,
	ERR_INVALIDSTRINGIMMEDIATE,
	ERR_BADCHARACTERCODE,
	ERR_BADPARMS,
	ERR_WERROR,

	WARN_MAX
};

#define FLAG_KILLSDEBUGGERS	1
#define FLAG_ASDEFAULT		2
#define FLAG_SETINGUI		4
#define FLAG_HIDDENINGUI	8
#define FLAG_MIDCOMPILE		16	//option can be changed mid-compile with the special pragma
typedef struct {
	pbool *enabled;
	char *abbrev;
	int optimisationlevel;
	int flags;	//1: kills debuggers. 2: applied as default.
	char *fullname;
	char *description;
	void *guiinfo;
} optimisations_t;
extern optimisations_t optimisations[];

typedef struct {
	pbool *enabled;
	int flags;	//2 applied as default
	char *abbrev;
	char *fullname;
	char *description;
	void *guiinfo;
} compiler_flag_t;
extern compiler_flag_t compiler_flag[];

#define WA_IGNORE 0
#define WA_WARN 1
#define WA_ERROR 2
extern unsigned char qccwarningaction[WARN_MAX];

extern	jmp_buf		pr_parse_abort;		// longjump with this on parse error
extern	int			pr_source_line;
extern	char		*pr_file_p;

void *QCC_PR_Malloc (int size);


#define	OFS_NULL		0
#define	OFS_RETURN		1
#define	OFS_PARM0		4		// leave 3 ofs for each parm to hold vectors
#define	OFS_PARM1		7
#define	OFS_PARM2		10
#define	OFS_PARM3		13
#define	OFS_PARM4		16
#define	RESERVED_OFS	28


extern	struct QCC_function_s	*pr_scope;
extern	int		pr_error_count, pr_warning_count;

void QCC_PR_NewLine (pbool incomment);
#define GDF_NONE		0
#define GDF_SAVED	1
#define GDF_STATIC	2
#define GDF_CONST	4
#define GDF_STRIP	8	//always stripped, regardless of optimisations. used for class member fields
#define GDF_SILENT	16	//used by the gui, to suppress ALL warnings associated with querying the def.
#define GDF_INLINE	32	//attempt to inline calls to this function
#define GDF_USED	64	//don't strip this, ever.
QCC_def_t *QCC_PR_GetDef (QCC_type_t *type, char *name, struct QCC_function_s *scope, pbool allocate, int arraysize, unsigned int flags);
QCC_sref_t QCC_PR_GetSRef (QCC_type_t *type, char *name, struct QCC_function_s *scope, pbool allocate, int arraysize, unsigned int flags);
void QCC_FreeTemp(QCC_sref_t t);
char *QCC_PR_CheckCompConstTooltip(char *word, char *outstart, char *outend);

void QCC_PR_PrintDefs (void);

void QCC_PR_SkipToSemicolon (void);

extern char *pr_parm_argcount_name;
#define MAX_EXTRA_PARMS 128
#ifdef MAX_EXTRA_PARMS
extern	char		pr_parm_names[MAX_PARMS+MAX_EXTRA_PARMS][MAX_NAME];
extern	QCC_sref_t	extra_parms[MAX_EXTRA_PARMS];
#else
extern	char		pr_parm_names[MAX_PARMS][MAX_NAME];
#endif

char *QCC_PR_ValueString (etype_t type, void *val);

void QCC_PR_ClearGrabMacros (pbool newfile);

pbool	QCC_PR_CompileFile (char *string, char *filename);
void QCC_PR_ResetErrorScope(void);

extern	pbool	pr_dumpasm;

extern	QCC_string_t	s_file;			// filename for function definition

extern	QCC_def_t	def_ret, def_parms[MAX_PARMS];

void QCC_PR_EmitArrayGetFunction(QCC_def_t *defscope, QCC_def_t *thearray, char *arrayname);
void QCC_PR_EmitArraySetFunction(QCC_def_t *defscope, QCC_def_t *thearray, char *arrayname);
void QCC_PR_EmitClassFromFunction(QCC_def_t *defscope, QCC_type_t *basetype);

void PostCompile(void);
pbool PreCompile(void);
void QCC_Cleanup(void);


#define FIRST_LOCAL 0//(MAX_REGS)
//=============================================================================

extern char	pr_immediate_string[8192];

extern QCC_eval_t		*qcc_pr_globals;
extern unsigned int	numpr_globals;

extern char		*strings;
extern int			strofs;

extern QCC_statement_t	*statements;
extern int			numstatements;

extern QCC_function_t	*functions;
extern dfunction_t	*dfunctions;
extern int			numfunctions;

extern QCC_ddef_t		*qcc_globals;
extern int			numglobaldefs;

extern QCC_def_t		*activetemps;

extern QCC_ddef_t		*fields;
extern int			numfielddefs;

extern QCC_type_t *qcc_typeinfo;
extern int numtypeinfos;
extern int maxtypeinfos;

extern int ForcedCRC;
extern pbool defaultnoref;
extern pbool defaultstatic;

extern int *qcc_tempofs;
extern int max_temps;
//extern int qcc_functioncalled;	//unuse temps if this is true - don't want to reuse the same space.

extern int tempsstart;
extern int numtemps;

extern char compilingrootfile[];	//.src file currently being compiled

typedef char PATHSTRING[MAX_DATA_PATH];

typedef struct
{
	PATHSTRING name;
	int block;
	int used;
	int fileline;
	char *filename;
} precache_t;
extern precache_t	*precache_sound;
extern int			numsounds;
extern precache_t	*precache_texture;
extern int			numtextures;
extern precache_t	*precache_model;
extern int			nummodels;
extern precache_t	*precache_file;
extern int			numfiles;

typedef struct qcc_includechunk_s {
	struct qcc_includechunk_s *prev;
	char *filename;
	char *currentdatapoint;
	int currentlinenumber;
	CompilerConstant_t *cnst;
} qcc_includechunk_t;
extern qcc_includechunk_t *currentchunk;

int	QCC_CopyString (char *str);




typedef struct qcc_cachedsourcefile_s {
	char filename[128];
	int size;
	char *file;
	enum{FT_CODE, FT_DATA} type;	//quakec source file or not.
	struct qcc_cachedsourcefile_s *next;
} qcc_cachedsourcefile_t;
extern qcc_cachedsourcefile_t *qcc_sourcefile;





#ifdef COMMONINLINES
static bool inline QCC_PR_CheckToken (char *string)
{
	if (pr_token_type != tt_punct)
		return false;

	if (STRCMP (string, pr_token))
		return false;

	QCC_PR_Lex ();
	return true;
}

static void inline QCC_PR_Expect (char *string)
{
	if (strcmp (string, pr_token))
		QCC_PR_ParseError ("expected %s, found %s",string, pr_token);
	QCC_PR_Lex ();
}
#endif

void editbadfile(char *fname, int line);
char *TypeName(QCC_type_t *type, char *buffer, int buffersize);
void QCC_PR_IncludeChunk (char *data, pbool duplicate, char *filename);
void QCC_PR_IncludeChunkEx(char *data, pbool duplicate, char *filename, CompilerConstant_t *cnst);
void QCC_PR_CloseProcessor(void);
pbool QCC_PR_UnInclude(void);
extern void *(*pHash_Get)(hashtable_t *table, const char *name);
extern void *(*pHash_GetNext)(hashtable_t *table, const char *name, void *old);
extern void *(*pHash_Add)(hashtable_t *table, const char *name, void *data, bucket_t *);
extern void (*pHash_RemoveData)(hashtable_t *table, const char *name, void *data);
