#if !defined(MINIMAL) && !defined(OMIT_QCC)

#include "qcc.h"

//FIXME: #define IAMNOTLAZY

#define SUPPORTINLINE

void QCC_PR_ParseAsm(void);

#define MEMBERFIELDNAME "__m%s"

#define STRCMP(s1,s2) (((*s1)!=(*s2)) || strcmp(s1+1,s2+1))	//saves about 2-6 out of 120 - expansion of idea from fastqcc
#define STRNCMP(s1,s2,l) (((*s1)!=(*s2)) || strncmp(s1+1,s2+1,l))	//pathetic saving here.

extern char *compilingfile;

int conditional;

//standard qc keywords
#define keyword_do		1
#define keyword_return	1
#define keyword_if		1
#define keyword_else	1
#define keyword_local	1
#define keyword_while	1

//extended keywords.
pbool keyword_switch;	//hexen2/c
pbool keyword_case;		//hexen2/c
pbool keyword_default;	//hexen2/c
pbool keyword_break;	//hexen2/c
pbool keyword_continue;	//hexen2/c
pbool keyword_loop;		//hexen2
pbool keyword_until;	//hexen2
pbool keyword_thinktime;//hexen2
pbool keyword_asm;
pbool keyword_class;
pbool keyword_optional;
pbool keyword_const;	//fixme
pbool keyword_entity;	//for skipping the local
pbool keyword_float;	//for skipping the local
pbool keyword_for;
pbool keyword_goto;
pbool keyword_int;		//for skipping the local
pbool keyword_integer;	//for skipping the local
pbool keyword_state;
pbool keyword_string;	//for skipping the local
pbool keyword_struct;
pbool keyword_var;		//allow it to be initialised and set around the place.
pbool keyword_vector;	//for skipping the local

pbool keyword_static;
pbool keyword_nonstatic;
pbool keyword_used;
pbool keyword_unused;


pbool keyword_enum;	//kinda like in c, but typedef not supported.
pbool keyword_enumflags;	//like enum, but doubles instead of adds 1.
pbool keyword_typedef;	//fixme
#define keyword_codesys		flag_acc	//reacc needs this (forces the resultant crc)
#define keyword_function	flag_acc	//reacc needs this (reacc has this on all functions, wierd eh?)
#define keyword_objdata		flag_acc	//reacc needs this (following defs are fields rather than globals, use var to disable)
#define keyword_object		flag_acc	//reacc needs this (an entity)
#define keyword_pfunc		flag_acc	//reacc needs this (pointer to function)
#define keyword_system		flag_acc	//reacc needs this (potatos)
#define keyword_real		flag_acc	//reacc needs this (a float)
#define keyword_exit		flag_acc	//emits an OP_DONE opcode.
#define keyword_external	flag_acc	//reacc needs this (a builtin)
pbool keyword_extern;	//function is external, don't error or warn if the body was not found
pbool keyword_shared;	//mark global to be copied over when progs changes (part of FTE_MULTIPROGS)
pbool keyword_noref;	//nowhere else references this, don't strip it.
pbool keyword_nosave;	//don't write the def to the output.
pbool keyword_inline;
pbool keyword_strip;
pbool keyword_ignore;
pbool keyword_union;	//you surly know what a union is!

#define keyword_not			1	//hexenc support needs this, and fteqcc can optimise without it, but it adds an extra token after the if, so it can cause no namespace conflicts

pbool keywords_coexist;		//don't disable a keyword simply because a var was made with the same name.
pbool output_parms;			//emit some PARMX fields. confuses decompilers.
pbool autoprototype;		//take two passes over the source code. First time round doesn't enter and functions or initialise variables.
pbool autoprototyped;		//previously autoprototyped. no longer allowed to enable autoproto, but don't warn about it.
pbool parseonly;			//parse defs and stuff, but don't bother compiling any actual code.
pbool pr_subscopedlocals;	//causes locals to be valid ONLY within their statement block. (they simply can't be referenced by name outside of it)
pbool flag_nullemptystr;	//null immediates are 0, not 1.
pbool flag_ifstring;		//makes if (blah) equivelent to if (blah != "") which resolves some issues in multiprogs situations.
pbool flag_iffloat;			//use an op_if_f instruction instead of op_if so if(-0) evaluates to false.
pbool flag_ifvector;		//use an op_not_v instruction instead of testing only _x.
pbool flag_vectorlogic;		//flag_ifvector but for && and ||
pbool flag_acc;				//reacc like behaviour of src files (finds *.qc in start dir and compiles all in alphabetical order)
pbool flag_caseinsensitive;	//symbols will be matched to an insensitive case if the specified case doesn't exist. This should b usable for any mod
pbool flag_laxcasts;		//Allow lax casting. This'll produce loadsa warnings of course. But allows compilation of certain dodgy code.
pbool flag_hashonly;		//Allows use of only #constant for precompiler constants, allows certain preqcc using mods to compile
pbool flag_fasttrackarrays;	//Faster arrays, dynamically detected, activated only in supporting engines.
pbool flag_msvcstyle;		//MSVC style warnings, so msvc's ide works properly
pbool flag_debugmacros;		//Print out #defines as they are expanded, for debugging.
pbool flag_assume_integer;	//5 - is that an integer or a float? qcc says float. but we support int too, so maybe we want that instead?
pbool flag_filetimes;
pbool flag_typeexplicit;	//no implicit type conversions, you must do the casts yourself.
pbool flag_noboundchecks;	//Disable generation of bound check instructions.
pbool flag_guiannotate;

pbool opt_overlaptemps;		//reduce numpr_globals by reuse of temps. When they are not needed they are freed for reuse. The way this is implemented is better than frikqcc's. (This is the single most important optimisation)
pbool opt_assignments;		//STORE_F isn't used if an operation wrote to a temp.
pbool opt_shortenifnots;		//if(!var) is made an IF rather than NOT IFNOT
pbool opt_noduplicatestrings;	//brute force string check. time consuming but more effective than the equivelent in frikqcc.
pbool opt_constantarithmatic;	//3*5 appears as 15 instead of the extra statement.
pbool opt_nonvec_parms;			//store_f instead of store_v on function calls, where possible.
pbool opt_constant_names;		//take out the defs and name strings of constants.
pbool opt_constant_names_strings;//removes the defs of strings too. plays havok with multiprogs.
pbool opt_precache_file;			//remove the call, the parameters, everything.
pbool opt_filenames;				//strip filenames. hinders older decompilers.
pbool opt_unreferenced;			//strip defs that are not referenced.
pbool opt_function_names;		//strip out the names of builtin functions.
pbool opt_locals;				//strip out the names of locals and immediates.
pbool opt_dupconstdefs;			//float X = 5; and float Y = 5; occupy the same global with this.
pbool opt_return_only;			//RETURN; DONE; at the end of a function strips out the done statement if there is no way to get to it.
pbool opt_compound_jumps;		//jumps to jump statements jump to the final point.
pbool opt_stripfunctions;		//if a functions is only ever called directly or by exe, don't emit the def.
pbool opt_locals_overlapping;	//make the local vars of all functions occupy the same globals.
pbool opt_logicops;				//don't make conditions enter functions if the return value will be discarded due to a previous value. (C style if statements)
pbool opt_vectorcalls;			//vectors can be packed into 3 floats, which can yield lower numpr_globals, but cost two more statements per call (only works for q1 calling conventions).
pbool opt_classfields;
pbool opt_simplifiedifs;		//if (f != 0) -> if_f (f). if (f == 0) -> ifnot_f (f)
//bool opt_comexprremoval;

//these are the results of the opt_. The values are printed out when compilation is compleate, showing effectivness.
int optres_shortenifnots;
int optres_assignments;
int optres_overlaptemps;
int optres_noduplicatestrings;
int optres_constantarithmatic;
int optres_nonvec_parms;
int optres_constant_names;
int optres_constant_names_strings;
int optres_precache_file;
int optres_filenames;
int optres_unreferenced;
int optres_function_names;
int optres_locals;
int optres_dupconstdefs;
int optres_return_only;
int optres_compound_jumps;
//int optres_comexprremoval;
int optres_stripfunctions;
int optres_locals_overlapping;
int optres_logicops;
int optres_inlines;

int optres_test1;
int optres_test2;

void *(*pHash_Get)(hashtable_t *table, const char *name);
void *(*pHash_GetNext)(hashtable_t *table, const char *name, void *old);
void *(*pHash_Add)(hashtable_t *table, const char *name, void *data, bucket_t *);
void (*pHash_RemoveData)(hashtable_t *table, const char *name, void *data);

QCC_def_t *QCC_PR_DummyDef(QCC_type_t *type, char *name, struct QCC_function_s *scope, int arraysize, QCC_def_t *rootsymbol, unsigned int ofs, int referable, unsigned int flags);
QCC_type_t *QCC_PR_FindType (QCC_type_t *type);
QCC_type_t *QCC_PR_PointerType (QCC_type_t *pointsto);
QCC_type_t *QCC_PR_FieldType (QCC_type_t *pointsto);
QCC_sref_t	QCC_PR_Term (unsigned int exprflags);
QCC_sref_t	QCC_PR_ParseValue (QCC_type_t *assumeclass, pbool allowarrayassign, pbool expandmemberfields, pbool makearraypointers);
QCC_sref_t	QCC_PR_GenerateFunctionCall (QCC_sref_t newself, QCC_sref_t func, QCC_sref_t *arglist, QCC_type_t **argtypelist, int argcount);
void		QCC_Marshal_Locals(int firststatement, int laststatement);
QCC_sref_t	QCC_PR_ParseArrayPointer (QCC_sref_t d, pbool allowarrayassign, pbool makestructpointers);
QCC_sref_t	QCC_LoadFromArray(QCC_sref_t base, QCC_sref_t index, QCC_type_t *t, pbool preserve);

QCC_ref_t *QCC_DefToRef(QCC_ref_t *ref, QCC_sref_t def);	//ref is a buffer to write into, to avoid excessive allocs
QCC_sref_t	QCC_RefToDef(QCC_ref_t *ref, pbool freetemps);
QCC_ref_t *QCC_PR_RefExpression (QCC_ref_t *retbuf, int priority, int exprflags);
QCC_ref_t *QCC_PR_ParseRefValue (QCC_ref_t *refbuf, QCC_type_t *assumeclass, pbool allowarrayassign, pbool expandmemberfields, pbool makearraypointers);
QCC_ref_t *QCC_PR_ParseRefArrayPointer (QCC_ref_t *refbuf, QCC_ref_t *d, pbool allowarrayassign, pbool makestructpointers);
QCC_ref_t *QCC_PR_BuildRef(QCC_ref_t *retbuf, unsigned int reftype, QCC_sref_t base, QCC_sref_t index, QCC_type_t *cast, pbool	readonly);
QCC_ref_t *QCC_PR_BuildAccessorRef(QCC_ref_t *retbuf, QCC_sref_t base, QCC_sref_t index, struct accessor_s *accessor, pbool readonly);
QCC_sref_t	QCC_StoreToRef(QCC_ref_t *dest, QCC_sref_t source, pbool readable, pbool preservedest);
void QCC_PR_DiscardRef(QCC_ref_t *ref);
QCC_function_t *QCC_PR_ParseImmediateStatements (QCC_def_t *def, QCC_type_t *type);
const char *QCC_VarAtOffset(QCC_sref_t ref, unsigned int size);

QCC_statement_t *QCC_Generate_OP_IFNOT(QCC_sref_t e, pbool preserve);
QCC_statement_t *QCC_Generate_OP_IF(QCC_sref_t e, pbool preserve);
QCC_statement_t *QCC_Generate_OP_GOTO(void);
QCC_sref_t QCC_PR_GenerateLogicalNot(QCC_sref_t e, const char *errormessage);
QCC_function_t *QCC_PR_GenerateQCFunction (QCC_def_t *def, QCC_type_t *type);

enum
{
	STFL_PRESERVEA=1u<<0,	//if a temp is released as part of the statement, it can be reused for the result. Which is bad if the temp is needed for something else, like e.e.f += 4;
	STFL_CONVERTA=1u<<1,		//convert to/from ints/floats to match the operand types required by the opcode
	STFL_PRESERVEB=1u<<2,
	STFL_CONVERTB=1u<<3,
	STFL_DISCARDRESULT=1u<<4
};
#define QCC_PR_Statement(op,a,b,st) QCC_PR_StatementFlags(op,a,b,st,STFL_CONVERTA|STFL_CONVERTB)
QCC_sref_t QCC_PR_StatementFlags ( QCC_opcode_t *op, QCC_sref_t var_a, QCC_sref_t var_b, QCC_statement_t **outstatement, unsigned int flags);

void QCC_PR_ParseState (void);
pbool expandedemptymacro;	//just inhibits warnings about hanging semicolons

QCC_pr_info_t	pr;
//QCC_def_t		**pr_global_defs/*[MAX_REGS]*/;	// to find def for a global variable

//keeps track of how many funcs are called while parsing a statement
//int qcc_functioncalled;

//========================================

const QCC_sref_t nullsref = {0};

struct QCC_function_s		*pr_scope;		// the function being parsed, or NULL
QCC_type_t		*pr_classtype;	// the class that the current function is part of.
QCC_type_t		*pr_assumetermtype;	//undefined things get this time, with no warning about being undeclared (used for the state function, so prototypes are not needed)
pbool	pr_dumpasm;
QCC_string_t	s_file, s_file2;			// filename for function definition

unsigned int			locals_marshalled;	// largest local block size that needs to be allocated for locals overlapping.

jmp_buf		pr_parse_abort;		// longjump with this on parse error

void QCC_PR_ParseDefs (char *classname);

pbool qcc_usefulstatement;

pbool debug_armour_defined;

int max_breaks;
int max_continues;
int max_cases;
int num_continues;
int num_breaks;
int num_cases;
int *pr_breaks;
int *pr_continues;
int *pr_cases;
QCC_sref_t *pr_casesdef;
QCC_sref_t *pr_casesdef2;

typedef struct {
	int statementno;
	int lineno;
	char name[256];
} gotooperator_t;

int max_labels;
int max_gotos;
gotooperator_t *pr_labels;
gotooperator_t *pr_gotos;
int num_gotos;
int num_labels;

QCC_sref_t extra_parms[MAX_EXTRA_PARMS];

//#define ASSOC_RIGHT_RESULT ASSOC_RIGHT

//========================================

//FIXME: modifiy list so most common GROUPS are first
//use look up table for value of first char and sort by first char and most common...?

//if true, effectivly {b=a; return a;}
QCC_opcode_t pr_opcodes[] =
{
 {6, "<DONE>", "DONE", -1, ASSOC_LEFT,			&type_void, &type_void, &type_void},

 {6, "*", "MUL_F",			3, ASSOC_LEFT,				&type_float, &type_float, &type_float},
 {6, "*", "MUL_V",			3, ASSOC_LEFT,				&type_vector, &type_vector, &type_float},
 {6, "*", "MUL_FV",			3, ASSOC_LEFT,				&type_float, &type_vector, &type_vector},
 {6, "*", "MUL_VF",			3, ASSOC_LEFT,				&type_vector, &type_float, &type_vector},

 {6, "/", "DIV_F",			3, ASSOC_LEFT,				&type_float, &type_float, &type_float},

 {6, "+", "ADD_F",			4, ASSOC_LEFT,				&type_float, &type_float, &type_float},
 {6, "+", "ADD_V",			4, ASSOC_LEFT,				&type_vector, &type_vector, &type_vector},

 {6, "-", "SUB_F",			4, ASSOC_LEFT,				&type_float, &type_float, &type_float},
 {6, "-", "SUB_V",			4, ASSOC_LEFT,				&type_vector, &type_vector, &type_vector},

 {6, "==", "EQ_F",			5, ASSOC_LEFT,				&type_float, &type_float, &type_float},
 {6, "==", "EQ_V",			5, ASSOC_LEFT,				&type_vector, &type_vector, &type_float},
 {6, "==", "EQ_S",			5, ASSOC_LEFT,				&type_string, &type_string, &type_float},
 {6, "==", "EQ_E",			5, ASSOC_LEFT,				&type_entity, &type_entity, &type_float},
 {6, "==", "EQ_FNC",		5, ASSOC_LEFT,				&type_function, &type_function, &type_float},

 {6, "!=", "NE_F",			5, ASSOC_LEFT,				&type_float, &type_float, &type_float},
 {6, "!=", "NE_V",			5, ASSOC_LEFT,				&type_vector, &type_vector, &type_float},
 {6, "!=", "NE_S",			5, ASSOC_LEFT,				&type_string, &type_string, &type_float},
 {6, "!=", "NE_E",			5, ASSOC_LEFT,				&type_entity, &type_entity, &type_float},
 {6, "!=", "NE_FNC",		5, ASSOC_LEFT,				&type_function, &type_function, &type_float},

 {6, "<=", "LE_F",			5, ASSOC_LEFT,					&type_float, &type_float, &type_float},
 {6, ">=", "GE_F",			5, ASSOC_LEFT,					&type_float, &type_float, &type_float},
 {6, "<", "LT_F",			5, ASSOC_LEFT,					&type_float, &type_float, &type_float},
 {6, ">", "GT_F",			5, ASSOC_LEFT,					&type_float, &type_float, &type_float},

 {6, ".", "LOADF_F",		1, ASSOC_LEFT,			&type_entity, &type_field, &type_float},
 {6, ".", "LOADF_V",		1, ASSOC_LEFT,			&type_entity, &type_field, &type_vector},
 {6, ".", "LOADF_S",		1, ASSOC_LEFT,			&type_entity, &type_field, &type_string},
 {6, ".", "LOADF_E",		1, ASSOC_LEFT,			&type_entity, &type_field, &type_entity},
 {6, ".", "LOADF_FI",	1, ASSOC_LEFT,			&type_entity, &type_field, &type_field},
 {6, ".", "LOADF_FU",	1, ASSOC_LEFT,			&type_entity, &type_field, &type_function},

 {6, ".", "FLDADDRESS",		1, ASSOC_LEFT,				&type_entity, &type_field, &type_pointer},

 {6, "=", "STORE_F",		6, ASSOC_RIGHT,				&type_float, &type_float, &type_float},
 {6, "=", "STORE_V",		6, ASSOC_RIGHT,				&type_vector, &type_vector, &type_vector},
 {6, "=", "STORE_S",		6, ASSOC_RIGHT,				&type_string, &type_string, &type_string},
 {6, "=", "STORE_ENT",		6, ASSOC_RIGHT,				&type_entity, &type_entity, &type_entity},
 {6, "=", "STORE_FLD",		6, ASSOC_RIGHT,				&type_field, &type_field, &type_field},
 {6, "=", "STORE_FNC",		6, ASSOC_RIGHT,				&type_function, &type_function, &type_function},

 {6, "=", "STOREP_F",		6, ASSOC_RIGHT,				&type_pointer, &type_float, &type_float},
 {6, "=", "STOREP_V",		6, ASSOC_RIGHT,				&type_pointer, &type_vector, &type_vector},
 {6, "=", "STOREP_S",		6, ASSOC_RIGHT,				&type_pointer, &type_string, &type_string},
 {6, "=", "STOREP_ENT",		6, ASSOC_RIGHT,			&type_pointer, &type_entity, &type_entity},
 {6, "=", "STOREP_FLD",		6, ASSOC_RIGHT,			&type_pointer, &type_field, &type_field},
 {6, "=", "STOREP_FNC",		6, ASSOC_RIGHT,			&type_pointer, &type_function, &type_function},

 {6, "<RETURN>", "RETURN",	-1, ASSOC_LEFT,		&type_vector, &type_void, &type_void},

 {6, "!", "NOT_F",			-1, ASSOC_LEFT,				&type_float, &type_void, &type_float},
 {6, "!", "NOT_V",			-1, ASSOC_LEFT,				&type_vector, &type_void, &type_float},
 {6, "!", "NOT_S",			-1, ASSOC_LEFT,				&type_vector, &type_void, &type_float},
 {6, "!", "NOT_ENT",		-1, ASSOC_LEFT,				&type_entity, &type_void, &type_float},
 {6, "!", "NOT_FNC",		-1, ASSOC_LEFT,				&type_function, &type_void, &type_float},

  {6, "<IF>", "IF",			-1, ASSOC_RIGHT,				&type_float, NULL, &type_void},
  {6, "<IFNOT>", "IFNOT",	-1, ASSOC_RIGHT,			&type_float, NULL, &type_void},

// calls returns REG_RETURN
 {6, "<CALL0>", "CALL0",	-1, ASSOC_LEFT,			&type_function, &type_void, &type_void},
 {6, "<CALL1>", "CALL1",	-1, ASSOC_LEFT,			&type_function, &type_void, &type_void},
 {6, "<CALL2>", "CALL2",	-1, ASSOC_LEFT,			&type_function, &type_void, &type_void},
 {6, "<CALL3>", "CALL3",	-1, ASSOC_LEFT,			&type_function, &type_void, &type_void},
 {6, "<CALL4>", "CALL4",	-1, ASSOC_LEFT,			&type_function, &type_void, &type_void},
 {6, "<CALL5>", "CALL5",	-1, ASSOC_LEFT,			&type_function, &type_void, &type_void},
 {6, "<CALL6>", "CALL6",	-1, ASSOC_LEFT,			&type_function, &type_void, &type_void},
 {6, "<CALL7>", "CALL7",	-1, ASSOC_LEFT,			&type_function, &type_void, &type_void},
 {6, "<CALL8>", "CALL8",	-1, ASSOC_LEFT,			&type_function, &type_void, &type_void},

 {6, "<STATE>", "STATE",	-1, ASSOC_LEFT,			&type_float, &type_float, &type_void},

 {6, "<GOTO>", "GOTO",		-1, ASSOC_RIGHT,			NULL, &type_void, &type_void},

 {6, "&&", "AND_F",			7, ASSOC_LEFT,					&type_float,	&type_float, &type_float},
 {6, "||", "OR_F",			7, ASSOC_LEFT,					&type_float,	&type_float, &type_float},

 {6, "&", "BITAND",			3, ASSOC_LEFT,				&type_float, &type_float, &type_float},
 {6, "|", "BITOR",			3, ASSOC_LEFT,				&type_float, &type_float, &type_float},

 //version 6 are in normal progs.



//these are hexen2
 {7, "*=", "MULSTORE_F",	6, ASSOC_RIGHT_RESULT,				&type_float, &type_float, &type_float},
 {7, "*=", "MULSTORE_VF",	6, ASSOC_RIGHT_RESULT,				&type_vector, &type_float, &type_vector},
 {7, "*=", "MULSTOREP_F",	6, ASSOC_RIGHT_RESULT,				&type_pointer, &type_float, &type_float},
 {7, "*=", "MULSTOREP_VF",	6, ASSOC_RIGHT_RESULT,				&type_pointer, &type_float, &type_vector},

 {7, "/=", "DIVSTORE_F",	6, ASSOC_RIGHT_RESULT,				&type_float, &type_float, &type_float},
 {7, "/=", "DIVSTOREP_F",	6, ASSOC_RIGHT_RESULT,				&type_pointer, &type_float, &type_float},

 {7, "+=", "ADDSTORE_F",	6, ASSOC_RIGHT_RESULT,				&type_float, &type_float, &type_float},
 {7, "+=", "ADDSTORE_V",	6, ASSOC_RIGHT_RESULT,				&type_vector, &type_vector, &type_vector},
 {7, "+=", "ADDSTOREP_F",	6, ASSOC_RIGHT_RESULT,				&type_pointer, &type_float, &type_float},
 {7, "+=", "ADDSTOREP_V",	6, ASSOC_RIGHT_RESULT,				&type_pointer, &type_vector, &type_vector},

 {7, "-=", "SUBSTORE_F",	6, ASSOC_RIGHT_RESULT,				&type_float, &type_float, &type_float},
 {7, "-=", "SUBSTORE_V",	6, ASSOC_RIGHT_RESULT,				&type_vector, &type_vector, &type_vector},
 {7, "-=", "SUBSTOREP_F",	6, ASSOC_RIGHT_RESULT,				&type_pointer, &type_float, &type_float},
 {7, "-=", "SUBSTOREP_V",	6, ASSOC_RIGHT_RESULT,				&type_pointer, &type_vector, &type_vector},

 {7, "<FETCH_GBL_F>", "FETCH_GBL_F",		-1, ASSOC_LEFT,	&type_float, &type_float, &type_float},
 {7, "<FETCH_GBL_V>", "FETCH_GBL_V",		-1, ASSOC_LEFT,	&type_vector, &type_float, &type_vector},
 {7, "<FETCH_GBL_S>", "FETCH_GBL_S",		-1, ASSOC_LEFT,	&type_string, &type_float, &type_string},
 {7, "<FETCH_GBL_E>", "FETCH_GBL_E",		-1, ASSOC_LEFT,	&type_entity, &type_float, &type_entity},
 {7, "<FETCH_GBL_FNC>", "FETCH_GBL_FNC",	-1, ASSOC_LEFT,	&type_function, &type_float, &type_function},

 {7, "<CSTATE>", "CSTATE",					-1, ASSOC_LEFT,	&type_float, &type_float, &type_void},

 {7, "<CWSTATE>", "CWSTATE",				-1, ASSOC_LEFT,	&type_float, &type_float, &type_void},

 {7, "<THINKTIME>", "THINKTIME",			-1, ASSOC_LEFT,	&type_entity, &type_float, &type_void},

 {7, "|=", "BITSET_F",						6,	ASSOC_RIGHT,	&type_float, &type_float, &type_float},
 {7, "|=", "BITSETP_F",						6,	ASSOC_RIGHT,	&type_pointer, &type_float, &type_float},
 {7, "&~=", "BITCLR_F",						6,	ASSOC_RIGHT,	&type_float, &type_float, &type_float},
 {7, "&~=", "BITCLRP_F",					6,	ASSOC_RIGHT,	&type_pointer, &type_float, &type_float},

 {7, "<RAND0>", "RAND0",					-1, ASSOC_LEFT,	&type_void, &type_void, &type_float},
 {7, "<RAND1>", "RAND1",					-1, ASSOC_LEFT,	&type_float, &type_void, &type_float},
 {7, "<RAND2>", "RAND2",					-1, ASSOC_LEFT,	&type_float, &type_float, &type_float},
 {7, "<RANDV0>", "RANDV0",					-1, ASSOC_LEFT,	&type_void, &type_void, &type_vector},
 {7, "<RANDV1>", "RANDV1",					-1, ASSOC_LEFT,	&type_vector, &type_void, &type_vector},
 {7, "<RANDV2>", "RANDV2",					-1, ASSOC_LEFT,	&type_vector, &type_vector, &type_vector},

 {7, "<SWITCH_F>", "SWITCH_F",				-1, ASSOC_RIGHT,	&type_float, NULL, &type_void},
 {7, "<SWITCH_V>", "SWITCH_V",				-1, ASSOC_RIGHT,	&type_vector, NULL, &type_void},
 {7, "<SWITCH_S>", "SWITCH_S",				-1, ASSOC_RIGHT,	&type_string, NULL, &type_void},
 {7, "<SWITCH_E>", "SWITCH_E",				-1, ASSOC_RIGHT,	&type_entity, NULL, &type_void},
 {7, "<SWITCH_FNC>", "SWITCH_FNC",			-1, ASSOC_RIGHT,	&type_function, NULL, &type_void},

 {7, "<CASE>", "CASE",						-1, ASSOC_RIGHT,	&type_variant, NULL, &type_void},
 {7, "<CASERANGE>", "CASERANGE",			-1, ASSOC_RIGHT,	&type_float, &type_float, NULL},


//Later are additions by DMW.

 {7, "<CALL1H>", "CALL1H",	-1, ASSOC_RIGHT,			&type_function, &type_variant, &type_void},
 {7, "<CALL2H>", "CALL2H",	-1, ASSOC_RIGHT,			&type_function, &type_variant, &type_variant},
 {7, "<CALL3H>", "CALL3H",	-1, ASSOC_RIGHT,			&type_function, &type_variant, &type_variant},
 {7, "<CALL4H>", "CALL4H",	-1, ASSOC_RIGHT,			&type_function, &type_variant, &type_variant},
 {7, "<CALL5H>", "CALL5H",	-1, ASSOC_RIGHT,			&type_function, &type_variant, &type_variant},
 {7, "<CALL6H>", "CALL6H",	-1, ASSOC_RIGHT,			&type_function, &type_variant, &type_variant},
 {7, "<CALL7H>", "CALL7H",	-1, ASSOC_RIGHT,			&type_function, &type_variant, &type_variant},
 {7, "<CALL8H>", "CALL8H",	-1, ASSOC_RIGHT,			&type_function, &type_variant, &type_variant},

 {7, "=",	"STORE_I", 6, ASSOC_RIGHT,				&type_integer, &type_integer, &type_integer},
 {7, "=",	"STORE_IF", 6, ASSOC_RIGHT,			&type_float, &type_integer, &type_integer},
 {7, "=",	"STORE_FI", 6, ASSOC_RIGHT,			&type_integer, &type_float, &type_float},

 {7, "+", "ADD_I", 4, ASSOC_LEFT,				&type_integer, &type_integer, &type_integer},
 {7, "+", "ADD_FI", 4, ASSOC_LEFT,				&type_float, &type_integer, &type_float},
 {7, "+", "ADD_IF", 4, ASSOC_LEFT,				&type_integer, &type_float, &type_float},

 {7, "-", "SUB_I", 4, ASSOC_LEFT,				&type_integer, &type_integer, &type_integer},
 {7, "-", "SUB_FI", 4, ASSOC_LEFT,				&type_float, &type_integer, &type_float},
 {7, "-", "SUB_IF", 4, ASSOC_LEFT,				&type_integer, &type_float, &type_float},

 {7, "<CIF>", "C_ITOF", -1, ASSOC_LEFT,				&type_integer, &type_void, &type_float},
 {7, "<CFI>", "C_FTOI", -1, ASSOC_LEFT,				&type_float, &type_void, &type_integer},
 {7, "<CPIF>", "CP_ITOF", -1, ASSOC_LEFT,			&type_pointer, &type_integer, &type_float},
 {7, "<CPFI>", "CP_FTOI", -1, ASSOC_LEFT,			&type_pointer, &type_float, &type_integer},

 {7, ".", "LOADF_I", 1, ASSOC_LEFT,				&type_entity,	&type_field, &type_integer},
 {7, "=", "STOREP_I", 6, ASSOC_RIGHT,				&type_pointer,	&type_integer, &type_integer},
 {7, "=", "STOREP_IF", 6, ASSOC_RIGHT,				&type_pointer,	&type_float, &type_integer},
 {7, "=", "STOREP_FI", 6, ASSOC_RIGHT,				&type_pointer,	&type_integer, &type_float},

 {7, "&", "BITAND_I", 3, ASSOC_LEFT,				&type_integer,	&type_integer, &type_integer},
 {7, "|", "BITOR_I", 3, ASSOC_LEFT,				&type_integer,	&type_integer, &type_integer},

 {7, "*", "MUL_I", 3, ASSOC_LEFT,				&type_integer,	&type_integer, &type_integer},
 {7, "/", "DIV_I", 3, ASSOC_LEFT,				&type_integer,	&type_integer, &type_integer},
 {7, "==", "EQ_I", 5, ASSOC_LEFT,				&type_integer,	&type_integer, &type_integer},
 {7, "!=", "NE_I", 5, ASSOC_LEFT,				&type_integer,	&type_integer, &type_integer},

 {7, "<IFNOTS>", "IFNOTS", -1, ASSOC_RIGHT,		&type_string,	NULL, &type_void},
 {7, "<IFS>", "IFS", -1, ASSOC_RIGHT,				&type_string,	NULL, &type_void},

 {7, "!", "NOT_I", -1, ASSOC_LEFT,				&type_integer,	&type_void, &type_integer},

 {7, "/", "DIV_VF", 3, ASSOC_LEFT,				&type_vector,	&type_float, &type_float},

 {7, "^", "BITXOR_I", 3, ASSOC_LEFT,				&type_integer,	&type_integer, &type_integer},
 {7, ">>", "RSHIFT_I", 3, ASSOC_LEFT,			&type_integer,	&type_integer, &type_integer},
 {7, "<<", "LSHIFT_I", 3, ASSOC_LEFT,			&type_integer,	&type_integer, &type_integer},

										//var,		offset			return
 {7, "<ARRAY>", "GLOBALADDRESS", -1, ASSOC_LEFT,	&type_float,		&type_integer, &type_pointer},
 {7, "<ARRAY>", "ADD_PIW", -1, ASSOC_LEFT,		&type_pointer,	&type_integer, &type_pointer},

 {7, "=", "LOADA_F", 6, ASSOC_LEFT,			&type_float,	&type_integer, &type_float},
 {7, "=", "LOADA_V", 6, ASSOC_LEFT,			&type_vector,	&type_integer, &type_vector},
 {7, "=", "LOADA_S", 6, ASSOC_LEFT,			&type_string,	&type_integer, &type_string},
 {7, "=", "LOADA_ENT", 6, ASSOC_LEFT,		&type_entity,	&type_integer, &type_entity},
 {7, "=", "LOADA_FLD", 6, ASSOC_LEFT,		&type_field,	&type_integer, &type_field},
 {7, "=", "LOADA_FNC", 6, ASSOC_LEFT,		&type_function,	&type_integer, &type_function},
 {7, "=", "LOADA_I", 6, ASSOC_LEFT,			&type_integer,	&type_integer, &type_integer},

 {7, "=", "STORE_P", 6, ASSOC_RIGHT,			&type_pointer,	&type_pointer, &type_void},
 {7, ".", "LOADF_P", 1, ASSOC_LEFT,			&type_entity,	&type_field, &type_pointer},

 {7, "=", "LOADP_F", 6, ASSOC_LEFT,			&type_pointer,	&type_integer, &type_float},
 {7, "=", "LOADP_V", 6, ASSOC_LEFT,			&type_pointer,	&type_integer, &type_vector},
 {7, "=", "LOADP_S", 6, ASSOC_LEFT,			&type_pointer,	&type_integer, &type_string},
 {7, "=", "LOADP_ENT", 6, ASSOC_LEFT,		&type_pointer,	&type_integer, &type_entity},
 {7, "=", "LOADP_FLD", 6, ASSOC_LEFT,		&type_pointer,	&type_integer, &type_field},
 {7, "=", "LOADP_FNC", 6, ASSOC_LEFT,		&type_pointer,	&type_integer, &type_function},
 {7, "=", "LOADP_I", 6, ASSOC_LEFT,			&type_pointer,	&type_integer, &type_integer},


 {7, "<=", "LE_I", 5, ASSOC_LEFT,					&type_integer, &type_integer, &type_integer},
 {7, ">=", "GE_I", 5, ASSOC_LEFT,					&type_integer, &type_integer, &type_integer},
 {7, "<", "LT_I", 5, ASSOC_LEFT,					&type_integer, &type_integer, &type_integer},
 {7, ">", "GT_I", 5, ASSOC_LEFT,					&type_integer, &type_integer, &type_integer},

 {7, "<=", "LE_IF", 5, ASSOC_LEFT,					&type_integer, &type_float, &type_integer},
 {7, ">=", "GE_IF", 5, ASSOC_LEFT,					&type_integer, &type_float, &type_integer},
 {7, "<", "LT_IF", 5, ASSOC_LEFT,					&type_integer, &type_float, &type_integer},
 {7, ">", "GT_IF", 5, ASSOC_LEFT,					&type_integer, &type_float, &type_integer},

 {7, "<=", "LE_FI", 5, ASSOC_LEFT,					&type_float, &type_integer, &type_integer},
 {7, ">=", "GE_FI", 5, ASSOC_LEFT,					&type_float, &type_integer, &type_integer},
 {7, "<", "LT_FI", 5, ASSOC_LEFT,					&type_float, &type_integer, &type_integer},
 {7, ">", "GT_FI", 5, ASSOC_LEFT,					&type_float, &type_integer, &type_integer},

 {7, "==", "EQ_IF", 5, ASSOC_LEFT,				&type_integer,	&type_float, &type_integer},
 {7, "==", "EQ_FI", 5, ASSOC_LEFT,				&type_float,	&type_integer, &type_float},

 	//-------------------------------------
	//string manipulation.
 {7, "+", "ADD_SF",	4, ASSOC_LEFT,				&type_string,	&type_float, &type_string},
 {7, "-", "SUB_S",	4, ASSOC_LEFT,				&type_string,	&type_string, &type_float},
 {7, "<STOREP_C>", "STOREP_C",	1, ASSOC_RIGHT,	&type_string,	&type_float, &type_float},
 {7, "<LOADP_C>", "LOADP_C",	1, ASSOC_LEFT,	&type_string,	&type_float, &type_float},
	//-------------------------------------



{7, "*", "MUL_IF", 5, ASSOC_LEFT,				&type_integer,	&type_float,	&type_float},
{7, "*", "MUL_FI", 5, ASSOC_LEFT,				&type_float,	&type_integer,	&type_float},
{7, "*", "MUL_VI", 5, ASSOC_LEFT,				&type_vector,	&type_integer,	&type_vector},
{7, "*", "MUL_IV", 5, ASSOC_LEFT,				&type_integer,	&type_vector,	&type_vector},

{7, "/", "DIV_IF", 5, ASSOC_LEFT,				&type_integer,	&type_float,	&type_float},
{7, "/", "DIV_FI", 5, ASSOC_LEFT,				&type_float,	&type_integer,	&type_float},

{7, "&", "BITAND_IF", 5, ASSOC_LEFT,			&type_integer,	&type_float,	&type_integer},
{7, "|", "BITOR_IF", 5, ASSOC_LEFT,				&type_integer,	&type_float,	&type_integer},
{7, "&", "BITAND_FI", 5, ASSOC_LEFT,			&type_float,	&type_integer,	&type_integer},
{7, "|", "BITOR_FI", 5, ASSOC_LEFT,				&type_float,	&type_integer,	&type_integer},

{7, "&&", "AND_I", 7, ASSOC_LEFT,				&type_integer,	&type_integer,	&type_integer},
{7, "||", "OR_I", 7, ASSOC_LEFT,				&type_integer,	&type_integer,	&type_integer},
{7, "&&", "AND_IF", 7, ASSOC_LEFT,				&type_integer,	&type_float,	&type_integer},
{7, "||", "OR_IF", 7, ASSOC_LEFT,				&type_integer,	&type_float,	&type_integer},
{7, "&&", "AND_FI", 7, ASSOC_LEFT,				&type_float,	&type_integer,	&type_integer},
{7, "||", "OR_FI", 7, ASSOC_LEFT,				&type_float,	&type_integer,	&type_integer},
{7, "!=", "NE_IF", 5, ASSOC_LEFT,				&type_integer,	&type_float,	&type_integer},
{7, "!=", "NE_FI", 5, ASSOC_LEFT,				&type_float,	&type_integer,	&type_integer},






{7, "<>",	"GSTOREP_I", -1, ASSOC_LEFT,			&type_float,	&type_float,	&type_float},
{7, "<>",	"GSTOREP_F", -1, ASSOC_LEFT,			&type_float,	&type_float,	&type_float},
{7, "<>",	"GSTOREP_ENT", -1, ASSOC_LEFT,			&type_float,	&type_float,	&type_float},
{7, "<>",	"GSTOREP_FLD", -1, ASSOC_LEFT,			&type_float,	&type_float,	&type_float},
{7, "<>",	"GSTOREP_S", -1, ASSOC_LEFT,			&type_float,	&type_float,	&type_float},
{7, "<>",	"GSTOREP_FNC", -1, ASSOC_LEFT,			&type_float,	&type_float,	&type_float},
{7, "<>",	"GSTOREP_V", -1, ASSOC_LEFT,			&type_float,	&type_float,	&type_float},

{7, "<>",	"GADDRESS", -1, ASSOC_LEFT,				&type_float,	&type_float,	&type_float},

{7, "<>",	"GLOAD_I",		-1, ASSOC_LEFT,				&type_float,	&type_float,	&type_float},
{7, "<>",	"GLOAD_F",		-1, ASSOC_LEFT,				&type_float,	&type_float,	&type_float},
{7, "<>",	"GLOAD_FLD",	-1, ASSOC_LEFT,				&type_float,	&type_float,	&type_float},
{7, "<>",	"GLOAD_ENT",	-1, ASSOC_LEFT,				&type_float,	&type_float,	&type_float},
{7, "<>",	"GLOAD_S",		-1, ASSOC_LEFT,				&type_float,	&type_float,	&type_float},
{7, "<>",	"GLOAD_FNC",	-1, ASSOC_LEFT,				&type_float,	&type_float,	&type_float},

{7, "<>",	"BOUNDCHECK",	-1, ASSOC_LEFT,				&type_integer,	NULL,	NULL},

{7, "<UNUSED>",	"UNUSED",		6,	ASSOC_RIGHT,				&type_void,	&type_void,	&type_void},
{7, "<PUSH>",	"PUSH",		-1, ASSOC_RIGHT,			&type_float,	&type_void,		&type_pointer},
{7, "<POP>",	"POP",		-1, ASSOC_RIGHT,			&type_float,	&type_void,		&type_void},

{7, "<SWITCH_I>", "SWITCH_I",				-1, ASSOC_LEFT,	&type_void, NULL, &type_void},
{7, "<>",	"GLOAD_S",		-1, ASSOC_LEFT,				&type_float,	&type_float,	&type_float},

{7, "<IF_F>",	"IF_F",		-1, ASSOC_RIGHT,				&type_float, NULL, &type_void},
{7, "<IFNOT_F>","IFNOT_F",	-1, ASSOC_RIGHT,				&type_float, NULL, &type_void},
/*
{7, "<=>",	"STOREF_F",		-1, ASSOC_RIGHT,				&type_entity, &type_field, &type_float},
{7, "<=>",	"STOREF_V",		-1, ASSOC_RIGHT,				&type_entity, &type_field, &type_vector},
{7, "<=>",	"STOREF_IF",	-1, ASSOC_RIGHT,				&type_entity, &type_field, &type_float},
{7, "<=>",	"STOREF_FI",	-1, ASSOC_RIGHT,				&type_entity, &type_field, &type_float},
*/
/* emulated ops begin here */
 {7, "<>",	"OP_EMULATED",		-1, ASSOC_LEFT,				&type_float,	&type_float,	&type_float},


 {7, "|=", "BITSET_I",		6,	ASSOC_RIGHT_RESULT,		&type_integer, &type_integer, &type_integer},
 {7, "|=", "BITSETP_I",		6,	ASSOC_RIGHT_RESULT,		&type_pointer, &type_integer, &type_integer},
 {7, "&~=", "BITCLR_I",		6,	ASSOC_RIGHT_RESULT,		&type_integer, &type_integer, &type_integer},


 {7, "*=", "MULSTORE_I",	6,	ASSOC_RIGHT_RESULT,		&type_integer, &type_integer, &type_integer},
 {7, "/=", "DIVSTORE_I",	6,	ASSOC_RIGHT_RESULT,		&type_integer, &type_integer, &type_integer},
 {7, "+=", "ADDSTORE_I",	6,	ASSOC_RIGHT_RESULT,		&type_integer, &type_integer, &type_integer},
 {7, "-=", "SUBSTORE_I",	6,	ASSOC_RIGHT_RESULT,		&type_integer, &type_integer, &type_integer},

 {7, "*=", "MULSTOREP_I",	6,	ASSOC_RIGHT_RESULT,		&type_pointer, &type_integer, &type_integer},
 {7, "/=", "DIVSTOREP_I",	6,	ASSOC_RIGHT_RESULT,		&type_pointer, &type_integer, &type_integer},
 {7, "+=", "ADDSTOREP_I",	6,	ASSOC_RIGHT_RESULT,		&type_pointer, &type_integer, &type_integer},
 {7, "-=", "SUBSTOREP_I",	6,	ASSOC_RIGHT_RESULT,		&type_pointer, &type_integer, &type_integer},

 {7, "*=", "MULSTORE_IF",	6,	ASSOC_RIGHT_RESULT,		&type_integer, &type_float, &type_float},
 {7, "*=", "MULSTOREP_IF",	6,	ASSOC_RIGHT_RESULT,		&type_intpointer, &type_float, &type_float},
 {7, "/=", "DIVSTORE_IF",	6,	ASSOC_RIGHT_RESULT,		&type_integer, &type_float, &type_float},
 {7, "/=", "DIVSTOREP_IF",	6,	ASSOC_RIGHT_RESULT,		&type_intpointer, &type_float, &type_float},
 {7, "+=", "ADDSTORE_IF",	6,	ASSOC_RIGHT_RESULT,		&type_integer, &type_float, &type_float},
 {7, "+=", "ADDSTOREP_IF",	6,	ASSOC_RIGHT_RESULT,		&type_intpointer, &type_float, &type_float},
 {7, "-=", "SUBSTORE_IF",	6,	ASSOC_RIGHT_RESULT,		&type_integer, &type_float, &type_float},
 {7, "-=", "SUBSTOREP_IF",	6,	ASSOC_RIGHT_RESULT,		&type_intpointer, &type_float, &type_float},

 {7, "*=", "MULSTORE_FI",	6,	ASSOC_RIGHT_RESULT,		&type_float, &type_integer, &type_float},
 {7, "*=", "MULSTOREP_FI",	6,	ASSOC_RIGHT_RESULT,		&type_floatpointer, &type_integer, &type_float},
 {7, "/=", "DIVSTORE_FI",	6,	ASSOC_RIGHT_RESULT,		&type_float, &type_integer, &type_float},
 {7, "/=", "DIVSTOREP_FI",	6,	ASSOC_RIGHT_RESULT,		&type_floatpointer, &type_integer, &type_float},
 {7, "+=", "ADDSTORE_FI",	6,	ASSOC_RIGHT_RESULT,		&type_float, &type_integer, &type_float},
 {7, "+=", "ADDSTOREP_FI",	6,	ASSOC_RIGHT_RESULT,		&type_floatpointer, &type_integer, &type_float},
 {7, "-=", "SUBSTORE_FI",	6,	ASSOC_RIGHT_RESULT,		&type_float, &type_integer, &type_float},
 {7, "-=", "SUBSTOREP_FI",	6,	ASSOC_RIGHT_RESULT,		&type_floatpointer, &type_integer, &type_float},

 {7, "*=", "MULSTORE_VI",	6, ASSOC_RIGHT_RESULT,		&type_vector, &type_integer, &type_vector},
 {7, "*=", "MULSTOREP_VI",	6, ASSOC_RIGHT_RESULT,		&type_pointer, &type_integer, &type_vector},

 {7, "=", "LOADA_STRUCT",	6, ASSOC_LEFT,				&type_float,	&type_integer, &type_float},

 {7, "=",	"LOADP_P",		6, ASSOC_LEFT,				&type_pointer,	&type_integer, &type_pointer},
 {7, "=",	"STOREP_P",		6,	ASSOC_RIGHT,			&type_pointer,	&type_pointer,	&type_pointer},
 {7, "~",	"BITNOT_F",		-1, ASSOC_LEFT,				&type_float,	&type_void, &type_float},
 {7, "~",	"BITNOT_I",		-1, ASSOC_LEFT,				&type_integer,	&type_void, &type_integer},

 {7, "==", "EQ_P", 5, ASSOC_LEFT,						&type_pointer,	&type_pointer, &type_float},
 {7, "!=", "NE_P", 5, ASSOC_LEFT,						&type_pointer,	&type_pointer, &type_float},
 {7, "<=", "LE_P", 5, ASSOC_LEFT,						&type_pointer,	&type_pointer, &type_float},
 {7, ">=", "GE_P", 5, ASSOC_LEFT,						&type_pointer,	&type_pointer, &type_float},
 {7, "<", "LT_P", 5, ASSOC_LEFT,						&type_pointer,	&type_pointer, &type_float},
 {7, ">", "GT_P", 5, ASSOC_LEFT,						&type_pointer,	&type_pointer, &type_float},

 {7, "&=", "ANDSTORE_F",	6, ASSOC_RIGHT_RESULT,		&type_float, &type_float, &type_float},
 {7, "&~", "BITCLR_F",	6, ASSOC_LEFT,					&type_float, &type_float, &type_float},
 {7, "&~", "BITCLR_I",	6, ASSOC_LEFT,					&type_integer, &type_integer, &type_integer},

 {7, "+", "ADD_SI",	4, ASSOC_LEFT,						&type_string,	&type_integer,	&type_string},
 {7, "+", "ADD_PF",	6, ASSOC_LEFT,						&type_pointer,	&type_float,	&type_pointer},
 {7, "+", "ADD_FP",	6, ASSOC_LEFT,						&type_float,	&type_pointer,	&type_pointer},
 {7, "+", "ADD_PI",	6, ASSOC_LEFT,						&type_pointer,	&type_integer,	&type_pointer},
 {7, "+", "ADD_IP",	6, ASSOC_LEFT,						&type_integer,	&type_pointer,	&type_pointer},
 {7, "-", "SUB_PF",	6, ASSOC_LEFT,						&type_pointer,	&type_float,	&type_pointer},
 {7, "-", "SUB_PI",	6, ASSOC_LEFT,						&type_pointer,	&type_integer,	&type_pointer},
 {7, "-", "SUB_PP",	6, ASSOC_LEFT,						&type_pointer,	&type_pointer,	&type_integer},

 {7, "%", "MOD_F",	6, ASSOC_LEFT,						&type_float,	&type_float,	&type_float},
 {7, "%", "MOD_I",	6, ASSOC_LEFT,						&type_integer,	&type_integer,	&type_integer},
 {7, "%", "MOD_V",	6, ASSOC_LEFT,						&type_vector,	&type_vector,	&type_vector},

 {7, "^", "BITXOR_F", 3, ASSOC_LEFT,					&type_float,	&type_float, &type_float},
 {7, ">>", "RSHIFT_F", 3, ASSOC_LEFT,					&type_float,	&type_float, &type_float},
 {7, "<<", "LSHIFT_F", 3, ASSOC_LEFT,					&type_float,	&type_float, &type_float},

 {7, "&&", "AND_ANY",	7, ASSOC_LEFT,						&type_variant,	&type_variant, &type_float},
 {7, "||", "OR_ANY",	7, ASSOC_LEFT,						&type_variant,	&type_variant, &type_float},

 {0, NULL}
};


pbool OpAssignsToC(unsigned int op)
{
	// calls, switches and cases DON'T
	if(pr_opcodes[op].type_c == &type_void)
		return false;
	if(op >= OP_SWITCH_F && op <= OP_CALL8H)
		return false;
	if(op >= OP_RAND0 && op <= OP_RANDV2)
		return false;
	// they use a and b, but have 3 types
	// safety
	if(op >= OP_BITSETSTORE_F && op <= OP_BITCLRSTOREP_F)
		return false;
	/*if(op >= OP_STORE_I && op <= OP_STORE_FI)
	  return false; <- add STOREP_*?*/
	if(op == OP_STOREP_C || op == OP_LOADP_C)
		return false;
	if(op >= OP_MULSTORE_F && op <= OP_SUBSTOREP_V)
		return false;	//actually they do.
	return true;
}
pbool OpAssignsToB(unsigned int op)
{
	if(op >= OP_BITSETSTORE_F && op <= OP_BITCLRSTOREP_F)
		return true;
	if(op >= OP_STORE_I && op <= OP_STORE_FI)
		return true;
	if(op == OP_STOREP_C || op == OP_LOADP_C)
		return true;
	if(op >= OP_MULSTORE_F && op <= OP_SUBSTOREP_V)
		return true;
	if((op >= OP_STORE_F && op <= OP_STOREP_FNC) || op == OP_STOREP_P || op == OP_STORE_P)
		return true;
	return false;
}
/*pbool OpAssignedTo(QCC_def_t *v, unsigned int op)
{
	if(OpAssignsToC(op))
	{
	}
	else if(OpAssignsToB(op))
	{
	}
	return false;
}
*/
#undef ASSOC_RIGHT_RESULT

#define	TOP_PRIORITY	7
#define FUNC_PRIORITY	1
#define UNARY_PRIORITY	1
#define	NOT_PRIORITY	5
//conditional and/or
#define CONDITION_PRIORITY 7

QCC_opcode_t *opcodes_store[] =
{
	NULL
};
QCC_opcode_t *opcodes_addstore[] =
{
/*	&pr_opcodes[OP_ADDSTORE_F],
	&pr_opcodes[OP_ADDSTORE_V],
	&pr_opcodes[OP_ADDSTORE_I],
	&pr_opcodes[OP_ADDSTORE_IF],
	&pr_opcodes[OP_ADDSTORE_FI],
	&pr_opcodes[OP_ADDSTOREP_F],
	&pr_opcodes[OP_ADDSTOREP_V],
	&pr_opcodes[OP_ADDSTOREP_I],
	&pr_opcodes[OP_ADDSTOREP_IF],
	&pr_opcodes[OP_ADDSTOREP_FI],*/
	&pr_opcodes[OP_ADD_F],
	&pr_opcodes[OP_ADD_V],
	&pr_opcodes[OP_ADD_I],
	&pr_opcodes[OP_ADD_FI],
	&pr_opcodes[OP_ADD_IF],
	&pr_opcodes[OP_ADD_SF],
	NULL
};
QCC_opcode_t *opcodes_substore[] =
{
/*	&pr_opcodes[OP_SUBSTORE_F],
	&pr_opcodes[OP_SUBSTORE_V],
	&pr_opcodes[OP_SUBSTORE_I],
	&pr_opcodes[OP_SUBSTORE_IF],
	&pr_opcodes[OP_SUBSTORE_FI],
	&pr_opcodes[OP_SUBSTOREP_F],
	&pr_opcodes[OP_SUBSTOREP_V],
	&pr_opcodes[OP_SUBSTOREP_I],
	&pr_opcodes[OP_SUBSTOREP_IF],
	&pr_opcodes[OP_SUBSTOREP_FI],*/
	&pr_opcodes[OP_SUB_F],
	&pr_opcodes[OP_SUB_V],
	&pr_opcodes[OP_SUB_I],
	&pr_opcodes[OP_SUB_FI],
	&pr_opcodes[OP_SUB_IF],
	&pr_opcodes[OP_SUB_S],
	NULL
};
QCC_opcode_t *opcodes_mulstore[] =
{
/*	&pr_opcodes[OP_MULSTORE_F],
	&pr_opcodes[OP_MULSTORE_VF],
	&pr_opcodes[OP_MULSTORE_VI],
	&pr_opcodes[OP_MULSTORE_I],
	&pr_opcodes[OP_MULSTORE_IF],
	&pr_opcodes[OP_MULSTORE_FI],
	&pr_opcodes[OP_MULSTOREP_F],
	&pr_opcodes[OP_MULSTOREP_VF],
	&pr_opcodes[OP_MULSTOREP_VI],
	&pr_opcodes[OP_MULSTOREP_I],
	&pr_opcodes[OP_MULSTOREP_IF],
	&pr_opcodes[OP_MULSTOREP_FI],*/
	&pr_opcodes[OP_MUL_F],
	&pr_opcodes[OP_MUL_V],
	&pr_opcodes[OP_MUL_FV],
	&pr_opcodes[OP_MUL_IV],
	&pr_opcodes[OP_MUL_VF],
	&pr_opcodes[OP_MUL_VI],
	&pr_opcodes[OP_MUL_I],
	&pr_opcodes[OP_MUL_FI],
	&pr_opcodes[OP_MUL_IF],
	NULL
};
QCC_opcode_t *opcodes_divstore[] =
{
	&pr_opcodes[OP_DIV_F],
	&pr_opcodes[OP_DIV_I],
	&pr_opcodes[OP_DIV_FI],
	&pr_opcodes[OP_DIV_IF],
	&pr_opcodes[OP_DIV_VF],
	NULL
};
QCC_opcode_t *opcodes_orstore[] =
{
	&pr_opcodes[OP_BITOR_F],
	&pr_opcodes[OP_BITOR_I],
	&pr_opcodes[OP_BITOR_IF],
	&pr_opcodes[OP_BITOR_FI],
	NULL
};
QCC_opcode_t *opcodes_xorstore[] =
{
	&pr_opcodes[OP_BITXOR_I],
	&pr_opcodes[OP_BITXOR_F],
	NULL
};
QCC_opcode_t *opcodes_andstore[] =
{
	&pr_opcodes[OP_BITAND_F],
	&pr_opcodes[OP_BITAND_I],
	&pr_opcodes[OP_BITAND_IF],
	&pr_opcodes[OP_BITAND_FI],
	NULL
};
QCC_opcode_t *opcodes_clearstore[] =
{
//	&pr_opcodes[OP_BITCLRSTORE_F],
//	&pr_opcodes[OP_BITCLRSTORE_I],
//	&pr_opcodes[OP_BITCLRSTOREP_F],
	&pr_opcodes[OP_BITCLR_F],
	&pr_opcodes[OP_BITCLR_I],
	NULL
};

//this system cuts out 10/120
//these evaluate as top first.
QCC_opcode_t *opcodeprioritized[TOP_PRIORITY+1][128] =
{
	{	//don't use
/*		&pr_opcodes[OP_DONE],
		&pr_opcodes[OP_RETURN],

		&pr_opcodes[OP_NOT_F],
		&pr_opcodes[OP_NOT_V],
		&pr_opcodes[OP_NOT_S],
		&pr_opcodes[OP_NOT_ENT],
		&pr_opcodes[OP_NOT_FNC],

		&pr_opcodes[OP_IF],
		&pr_opcodes[OP_IFNOT],
		&pr_opcodes[OP_CALL0],
		&pr_opcodes[OP_CALL1],
		&pr_opcodes[OP_CALL2],
		&pr_opcodes[OP_CALL3],
		&pr_opcodes[OP_CALL4],
		&pr_opcodes[OP_CALL5],
		&pr_opcodes[OP_CALL6],
		&pr_opcodes[OP_CALL7],
		&pr_opcodes[OP_CALL8],
		&pr_opcodes[OP_STATE],
		&pr_opcodes[OP_GOTO],

		&pr_opcodes[OP_IFNOTS],
		&pr_opcodes[OP_IFS],

		&pr_opcodes[OP_NOT_I],
*/		NULL
	}, {	//1

//		&pr_opcodes[OP_LOAD_F],
//		&pr_opcodes[OP_LOAD_V],
//		&pr_opcodes[OP_LOAD_S],
//		&pr_opcodes[OP_LOAD_ENT],
//		&pr_opcodes[OP_LOAD_FLD],
//		&pr_opcodes[OP_LOAD_FNC],
//		&pr_opcodes[OP_LOAD_I],
//		&pr_opcodes[OP_LOAD_P],
//		&pr_opcodes[OP_ADDRESS],
		NULL
	}, {	//2
/*	//conversion. don't use
		&pr_opcodes[OP_C_ITOF],
		&pr_opcodes[OP_C_FTOI],
		&pr_opcodes[OP_CP_ITOF],
		&pr_opcodes[OP_CP_FTOI],
*/		NULL
	}, {	//3
		&pr_opcodes[OP_MUL_F],
		&pr_opcodes[OP_MUL_V],
		&pr_opcodes[OP_MUL_FV],
		&pr_opcodes[OP_MUL_IV],
		&pr_opcodes[OP_MUL_VF],
		&pr_opcodes[OP_MUL_VI],
		&pr_opcodes[OP_MUL_I],
		&pr_opcodes[OP_MUL_FI],
		&pr_opcodes[OP_MUL_IF],

		&pr_opcodes[OP_DIV_F],
		&pr_opcodes[OP_DIV_I],
		&pr_opcodes[OP_DIV_FI],
		&pr_opcodes[OP_DIV_IF],
		&pr_opcodes[OP_DIV_VF],

		&pr_opcodes[OP_BITAND_F],
		&pr_opcodes[OP_BITAND_I],
		&pr_opcodes[OP_BITAND_IF],
		&pr_opcodes[OP_BITAND_FI],

		&pr_opcodes[OP_BITOR_F],
		&pr_opcodes[OP_BITOR_I],
		&pr_opcodes[OP_BITOR_IF],
		&pr_opcodes[OP_BITOR_FI],

		&pr_opcodes[OP_BITXOR_I],
		&pr_opcodes[OP_RSHIFT_I],
		&pr_opcodes[OP_LSHIFT_I],
		
		&pr_opcodes[OP_BITXOR_F],
		&pr_opcodes[OP_RSHIFT_F],
		&pr_opcodes[OP_LSHIFT_F],

		&pr_opcodes[OP_MOD_F],
		&pr_opcodes[OP_MOD_I],
		&pr_opcodes[OP_MOD_V],

		NULL
	}, {	//4

		&pr_opcodes[OP_ADD_F],
		&pr_opcodes[OP_ADD_V],
		&pr_opcodes[OP_ADD_I],
		&pr_opcodes[OP_ADD_FI],
		&pr_opcodes[OP_ADD_IF],
		&pr_opcodes[OP_ADD_SF],
		&pr_opcodes[OP_ADD_PF],
		&pr_opcodes[OP_ADD_FP],
		&pr_opcodes[OP_ADD_PI],
		&pr_opcodes[OP_ADD_IP],

		&pr_opcodes[OP_SUB_F],
		&pr_opcodes[OP_SUB_V],
		&pr_opcodes[OP_SUB_I],
		&pr_opcodes[OP_SUB_FI],
		&pr_opcodes[OP_SUB_IF],
		&pr_opcodes[OP_SUB_S],
		&pr_opcodes[OP_SUB_PF],
		&pr_opcodes[OP_SUB_PI],
		&pr_opcodes[OP_SUB_PP],
		NULL
	}, {	//5

		&pr_opcodes[OP_EQ_F],
		&pr_opcodes[OP_EQ_V],
		&pr_opcodes[OP_EQ_S],
		&pr_opcodes[OP_EQ_E],
		&pr_opcodes[OP_EQ_FNC],
		&pr_opcodes[OP_EQ_I],
		&pr_opcodes[OP_EQ_IF],
		&pr_opcodes[OP_EQ_FI],
		&pr_opcodes[OP_EQ_P],

		&pr_opcodes[OP_NE_F],
		&pr_opcodes[OP_NE_V],
		&pr_opcodes[OP_NE_S],
		&pr_opcodes[OP_NE_E],
		&pr_opcodes[OP_NE_FNC],
		&pr_opcodes[OP_NE_I],
		&pr_opcodes[OP_NE_IF],
		&pr_opcodes[OP_NE_FI],
		&pr_opcodes[OP_NE_P],

		&pr_opcodes[OP_LE_F],
		&pr_opcodes[OP_LE_I],
		&pr_opcodes[OP_LE_IF],
		&pr_opcodes[OP_LE_FI],
		&pr_opcodes[OP_LE_P],
		&pr_opcodes[OP_GE_F],
		&pr_opcodes[OP_GE_I],
		&pr_opcodes[OP_GE_IF],
		&pr_opcodes[OP_GE_FI],
		&pr_opcodes[OP_GE_P],
		&pr_opcodes[OP_LT_F],
		&pr_opcodes[OP_LT_I],
		&pr_opcodes[OP_LT_IF],
		&pr_opcodes[OP_LT_FI],
		&pr_opcodes[OP_LT_P],
		&pr_opcodes[OP_GT_F],
		&pr_opcodes[OP_GT_I],
		&pr_opcodes[OP_GT_IF],
		&pr_opcodes[OP_GT_FI],
		&pr_opcodes[OP_GT_P],

		NULL
	}, {	//6
		&pr_opcodes[OP_STOREP_P],

		&pr_opcodes[OP_STORE_F],
		&pr_opcodes[OP_STORE_V],
		&pr_opcodes[OP_STORE_S],
		&pr_opcodes[OP_STORE_ENT],
		&pr_opcodes[OP_STORE_FLD],
		&pr_opcodes[OP_STORE_FNC],
		&pr_opcodes[OP_STORE_I],
		&pr_opcodes[OP_STORE_IF],
		&pr_opcodes[OP_STORE_FI],
		&pr_opcodes[OP_STORE_P],

		&pr_opcodes[OP_STOREP_F],
		&pr_opcodes[OP_STOREP_V],
		&pr_opcodes[OP_STOREP_S],
		&pr_opcodes[OP_STOREP_ENT],
		&pr_opcodes[OP_STOREP_FLD],
		&pr_opcodes[OP_STOREP_FNC],
		&pr_opcodes[OP_STOREP_I],
		&pr_opcodes[OP_STOREP_IF],
		&pr_opcodes[OP_STOREP_FI],

		&pr_opcodes[OP_DIVSTORE_F],
		&pr_opcodes[OP_DIVSTORE_I],
		&pr_opcodes[OP_DIVSTORE_FI],
		&pr_opcodes[OP_DIVSTORE_IF],
		&pr_opcodes[OP_DIVSTOREP_F],
		&pr_opcodes[OP_DIVSTOREP_I],
		&pr_opcodes[OP_DIVSTOREP_IF],
		&pr_opcodes[OP_DIVSTOREP_FI],
		&pr_opcodes[OP_MULSTORE_F],
		&pr_opcodes[OP_MULSTORE_VF],
		&pr_opcodes[OP_MULSTORE_VI],
		&pr_opcodes[OP_MULSTORE_I],
		&pr_opcodes[OP_MULSTORE_IF],
		&pr_opcodes[OP_MULSTORE_FI],
		&pr_opcodes[OP_MULSTOREP_F],
		&pr_opcodes[OP_MULSTOREP_VF],
		&pr_opcodes[OP_MULSTOREP_VI],
		&pr_opcodes[OP_MULSTOREP_I],
		&pr_opcodes[OP_MULSTOREP_IF],
		&pr_opcodes[OP_MULSTOREP_FI],
		&pr_opcodes[OP_ADDSTORE_F],
		&pr_opcodes[OP_ADDSTORE_V],
		&pr_opcodes[OP_ADDSTORE_I],
		&pr_opcodes[OP_ADDSTORE_IF],
		&pr_opcodes[OP_ADDSTORE_FI],
		&pr_opcodes[OP_ADDSTOREP_F],
		&pr_opcodes[OP_ADDSTOREP_V],
		&pr_opcodes[OP_ADDSTOREP_I],
		&pr_opcodes[OP_ADDSTOREP_IF],
		&pr_opcodes[OP_ADDSTOREP_FI],
		&pr_opcodes[OP_SUBSTORE_F],
		&pr_opcodes[OP_SUBSTORE_V],
		&pr_opcodes[OP_SUBSTORE_I],
		&pr_opcodes[OP_SUBSTORE_IF],
		&pr_opcodes[OP_SUBSTORE_FI],
		&pr_opcodes[OP_SUBSTOREP_F],
		&pr_opcodes[OP_SUBSTOREP_V],
		&pr_opcodes[OP_SUBSTOREP_I],
		&pr_opcodes[OP_SUBSTOREP_IF],
		&pr_opcodes[OP_SUBSTOREP_FI],

		&pr_opcodes[OP_ANDSTORE_F],

		&pr_opcodes[OP_BITSETSTORE_F],
		&pr_opcodes[OP_BITSETSTORE_I],
//		&pr_opcodes[OP_BITSETSTORE_IF],
//		&pr_opcodes[OP_BITSETSTORE_FI],
		&pr_opcodes[OP_BITSETSTOREP_F],
		&pr_opcodes[OP_BITSETSTOREP_I],
//		&pr_opcodes[OP_BITSETSTOREP_IF],
//		&pr_opcodes[OP_BITSETSTOREP_FI],
		&pr_opcodes[OP_BITCLRSTORE_F],
		&pr_opcodes[OP_BITCLRSTOREP_F],

		NULL
	}, {	//7
		&pr_opcodes[OP_AND_F],
		&pr_opcodes[OP_AND_I],
		&pr_opcodes[OP_AND_IF],
		&pr_opcodes[OP_AND_FI],
		&pr_opcodes[OP_AND_ANY],
		&pr_opcodes[OP_OR_F],
		&pr_opcodes[OP_OR_I],
		&pr_opcodes[OP_OR_IF],
		&pr_opcodes[OP_OR_FI],
		&pr_opcodes[OP_OR_ANY],
		NULL
	}
};

pbool QCC_OPCodeValid(QCC_opcode_t *op)
{
	int num;
	num = op - pr_opcodes;

	//never any emulated opcodes
	if (num >= OP_NUMREALOPS)
		return false;

	switch(qcc_targetformat)
	{
	case QCF_STANDARD:
	case QCF_KK7:
	case QCF_QTEST:
		if (num < OP_MULSTORE_F)
			return true;
		return false;
	case QCF_HEXEN2:
		if (num >= OP_SWITCH_V && num <= OP_SWITCH_FNC)	//these were assigned numbers but were never actually implemtented in standard h2.
			return false;
//		if (num >= OP_MULSTORE_F && num <= OP_SUBSTOREP_V)
//			return false;
		if (num <= OP_CALL8H)	//CALLXH are fixed up. This is to provide more dynamic switching...
			return true;
		return false;
	case QCF_FTEH2:
	case QCF_FTE:
	case QCF_FTEDEBUG:
		return true;
	case QCF_DARKPLACES:
		//all id opcodes.
		if (num < OP_MULSTORE_F)
			return true;

		//extended opcodes.
		//DPFIXME: this is a list of the extended opcodes. I was conservative regarding supported ones.
		//         at the time of writing, these are the ones that look like they'll work just fine in Blub\0's patch.
		//         the ones that looked too permissive with bounds checks, or would give false positives are disabled.
		//         if the DP guys want I can change them as desired.
		switch(num)
		{
		//maths and conditionals (simple opcodes that read from specific globals and write to a global)
		case OP_ADD_I:
		case OP_ADD_IF:
		case OP_ADD_FI:
		case OP_SUB_I:
		case OP_SUB_IF:
		case OP_SUB_FI:
		case OP_MUL_I:
		case OP_MUL_IF:
		case OP_MUL_FI:
		case OP_MUL_VI:
		case OP_DIV_VF:
		case OP_DIV_I:
		case OP_DIV_IF:
		case OP_DIV_FI:
		case OP_BITAND_I:
		case OP_BITOR_I:
		case OP_BITAND_IF:
		case OP_BITOR_IF:
		case OP_BITAND_FI:
		case OP_BITOR_FI:
		case OP_GE_I:
		case OP_LE_I:
		case OP_GT_I:
		case OP_LT_I:
		case OP_AND_I:
		case OP_OR_I:
		case OP_GE_IF:
		case OP_LE_IF:
		case OP_GT_IF:
		case OP_LT_IF:
		case OP_AND_IF:
		case OP_OR_IF:
		case OP_GE_FI:
		case OP_LE_FI:
		case OP_GT_FI:
		case OP_LT_FI:
		case OP_AND_FI:
		case OP_OR_FI:
		case OP_NOT_I:
		case OP_EQ_I:
		case OP_EQ_IF:
		case OP_EQ_FI:
		case OP_NE_I:
		case OP_NE_IF:
		case OP_NE_FI:
			return true;

		//stores into a pointer (generated from 'ent.field=XXX')
		case OP_STOREP_I:	//no worse than the other OP_STOREP_X functions
		//reads from an entity field
		case OP_LOAD_I:		//no worse than the other OP_LOAD_X functions.
		case OP_LOAD_P:
			return true;

		//stores into the globals array.
		//they can change any global dynamically, but thats no security risk.
		//fteqcc will not automatically generate these.
		//fteqw does not support them either.
		case OP_GSTOREP_I:
		case OP_GSTOREP_F:
		case OP_GSTOREP_ENT:
		case OP_GSTOREP_FLD:
		case OP_GSTOREP_S:
		case OP_GSTOREP_FNC:
		case OP_GSTOREP_V:
			return true;

		//this opcode looks weird
		case OP_GADDRESS://floatc = globals[inta + floatb] (fte does not support)
			return true;

		//fteqcc will not automatically generate these
		//fteqw does not support them either, for that matter.
		case OP_GLOAD_I://c = globals[inta]
		case OP_GLOAD_F://note: fte does not support these
		case OP_GLOAD_FLD:
		case OP_GLOAD_ENT:
		case OP_GLOAD_S:
		case OP_GLOAD_FNC:
			return true;
		case OP_GLOAD_V:
			return false;	//DPFIXME: this is commented out in the patch I was given a link to... because the opcode wasn't defined.

		//these are reportedly functional.
		case OP_CALL8H:
		case OP_CALL7H:
		case OP_CALL6H:
		case OP_CALL5H:
		case OP_CALL4H:
		case OP_CALL3H:
		case OP_CALL2H:
		case OP_CALL1H:
			return true;

		case OP_RAND0:
		case OP_RAND1:
		case OP_RAND2:
		case OP_RANDV0:
		case OP_RANDV1:
		case OP_RANDV2:
			return true;

		case OP_BITSETSTORE_F: // b |= a
		case OP_BITCLRSTORE_F: // b &= ~a
		case OP_BITSETSTOREP_F: // *b |= a
		case OP_BITCLRSTOREP_F: // *b &= ~a
			return false;	//FIXME: I do not fully follow the controversy over these.

		case OP_SWITCH_F:
		case OP_SWITCH_V:
		case OP_SWITCH_S:
		case OP_SWITCH_E:
		case OP_SWITCH_FNC:
		case OP_CASE:
		case OP_CASERANGE:
			return true;

		//assuming the pointers here are fine, the return values are a little strange.
		//but its fine
		case OP_ADDSTORE_F:
		case OP_ADDSTORE_V:
		case OP_ADDSTOREP_F: // e.f += f
		case OP_ADDSTOREP_V: // e.v += v
		case OP_SUBSTORE_F:
		case OP_SUBSTORE_V:
		case OP_SUBSTOREP_F: // e.f += f
		case OP_SUBSTOREP_V: // e.v += v
			return true;

		case OP_LOADA_I:
		case OP_LOADA_F:
		case OP_LOADA_FLD:
		case OP_LOADA_ENT:
		case OP_LOADA_S:
		case OP_LOADA_FNC:
		case OP_LOADA_V:
			return false;	//DPFIXME: DP does not bounds check these properly. I won't generate them.

		case OP_CONV_ITOF:
		case OP_CONV_FTOI:
			return true;	//these look fine.

		case OP_STOREP_C: // store a char in a string
			return false; //DPFIXME: dp's bounds check may give false positives with expected uses.

		case OP_MULSTORE_F:
		case OP_MULSTORE_VF:
		case OP_MULSTOREP_F:
		case OP_MULSTOREP_VF: // e.v *= f
		case OP_DIVSTORE_F:
		case OP_DIVSTOREP_F:
		case OP_STORE_IF:
		case OP_STORE_FI:
		case OP_STORE_P:
		case OP_STOREP_IF: // store a value to a pointer
		case OP_STOREP_FI:
		case OP_IFNOT_S:
		case OP_IF_S:
			return true;

		case OP_IFNOT_F:	//added, but not in dp yet
		case OP_IF_F:
			return false;

		case OP_CP_ITOF:
		case OP_CP_FTOI:
			return false;	//DPFIXME: These are not bounds checked at all.
		case OP_GLOBALADDRESS:
			return true;	//DPFIXME: DP will reject these pointers if they are ever used.
		case OP_ADD_PIW:
			return true;	//just maths.

		case OP_ADD_SF: //(char*)c = (char*)a + (float)b
		case OP_SUB_S:  //(float)c = (char*)a - (char*)b
			return true;
		case OP_LOADP_C:        //load character from a string
			return false;	//DPFIXME: DP looks like it'll reject these or wrongly allow.

		case OP_LOADP_I:
		case OP_LOADP_F:
		case OP_LOADP_FLD:
		case OP_LOADP_ENT:
		case OP_LOADP_S:
		case OP_LOADP_FNC:
		case OP_LOADP_V:
			return true;

		case OP_BITXOR_I:
		case OP_RSHIFT_I:
		case OP_LSHIFT_I:
			return true;

		case OP_FETCH_GBL_F:
		case OP_FETCH_GBL_S:
		case OP_FETCH_GBL_E:
		case OP_FETCH_GBL_FNC:
		case OP_FETCH_GBL_V:
			return false;	//DPFIXME: DP will not bounds check this properly, it is too permissive.
		case OP_CSTATE:
		case OP_CWSTATE:
			return false;	//DP does not support this hexenc opcode.
		case OP_THINKTIME:
			return true;	//but it does support this one.

		default:			//anything I forgot to mention is new.
			return false;
		}
	}
	return false;
}

#define EXPR_WARN_ABOVE_1 2
#define EXPR_DISALLOW_COMMA 4
#define EXPR_DISALLOW_ARRAYASSIGN 8
QCC_sref_t QCC_PR_Expression (int priority, int exprflags);
int QCC_AStatementJumpsTo(int targ, int first, int last);
pbool QCC_StatementIsAJump(int stnum, int notifdest);

//===========================================================================

//typically used for debugging. Also used to determine function names for intrinsics.
const char *QCC_GetSRefName(QCC_sref_t ref)
{
	if (ref.sym && ref.sym->name && !ref.ofs)
		return ref.sym->name;
	return "TEMP";
}

//NULL is defined as the immediate 0 or 0i (aka __NULL__).
//named constants that merely have the value 0 are NOT meant to count.
pbool QCC_SRef_IsNull(QCC_sref_t ref)
{
	return (ref.cast->type == ev_integer || ref.cast->type == ev_float) && ref.sym && ref.sym->initialized && ref.sym->constant && !ref.sym->symboldata[ref.ofs]._int;
}

//retrieves the data associated with the reference if its constant and thus readable at compile time.
const QCC_eval_t *QCC_SRef_EvalConst(QCC_sref_t ref)
{
	if (ref.sym && ref.sym->initialized && ref.sym->constant)
	{
		ref.sym->referenced = true;
		return &ref.sym->symboldata[/*ref.sym->ofs +*/ ref.ofs];
	}
	return NULL;
}

/*
============
PR_Statement

Emits a primitive statement, returning the var it places it's value in
============
*/
static int QCC_ShouldConvert(QCC_type_t *from, etype_t wanted)
{
	/*no conversion needed*/
	if (from->type == wanted)
		return 0;
	if (from->type == ev_integer && wanted == ev_function)
		return 0;
	if (from->type == ev_integer && wanted == ev_pointer)
		return 0;
	/*stuff needs converting*/
	if (from->type == ev_pointer && from->aux_type)
	{
		if (from->aux_type->type == ev_float && wanted == ev_integer)
			return OP_CP_FTOI;

		if (from->aux_type->type == ev_integer && wanted == ev_float)
			return OP_CP_ITOF;
	}
	else
	{
		if (from->type == ev_float && wanted == ev_integer)
			return OP_CONV_FTOI;

		if (from->type == ev_integer && wanted == ev_float)
			return OP_CONV_ITOF;
	}

	/*impossible*/
	return -1;
}
QCC_sref_t QCC_SupplyConversionForAssignment(QCC_sref_t to, QCC_sref_t from, QCC_type_t *wanted, pbool fatal)
{
	int o;

	if (wanted->type == ev_accessor && wanted->parentclass && from.cast->type != ev_accessor)
		wanted = wanted->parentclass;
	if (from.cast->type == ev_accessor && from.cast->parentclass && wanted->type != ev_accessor)
		from.cast = from.cast->parentclass;

	o = QCC_ShouldConvert(from.cast, wanted->type);

	if (o == 0) //type already matches
		return from;
	if (flag_typeexplicit)
	{
		char totypename[256], fromtypename[256];
		TypeName(wanted, totypename, sizeof(totypename));
		TypeName(from.cast, fromtypename, sizeof(fromtypename));
		QCC_PR_ParseErrorPrintSRef(ERR_TYPEMISMATCH, from, "Implicit type mismatch on assignment to %s. Needed %s, got %s.", QCC_GetSRefName(to), totypename, fromtypename);
	}
	if (o < 0)
	{
		if (fatal && wanted->type != ev_variant && from.cast->type != ev_variant)
		{
			char totypename[256], fromtypename[256];
			TypeName(wanted, totypename, sizeof(totypename));
			TypeName(from.cast, fromtypename, sizeof(fromtypename));
			if (flag_laxcasts)
			{
				QCC_PR_ParseWarning(WARN_LAXCAST, "Implicit type mismatch on assignment to %s. Needed %s, got %s.", QCC_GetSRefName(to), totypename, fromtypename);
				QCC_PR_ParsePrintSRef(WARN_LAXCAST, from);
			}
			else
				QCC_PR_ParseErrorPrintSRef(ERR_TYPEMISMATCH, from, "Implicit type mismatch on assignment to %s. Needed %s, got %s.", QCC_GetSRefName(to), totypename, fromtypename);
		}
		return from;
	}

	return QCC_PR_Statement(&pr_opcodes[o], from, nullsref, NULL);	//conversion return value
}
QCC_sref_t QCC_SupplyConversion(QCC_sref_t  var, etype_t wanted, pbool fatal)
{
	extern char *basictypenames[];
	int o;

	o = QCC_ShouldConvert(var.cast, wanted);

	if (o == 0) //type already matches
		return var;
	if (flag_typeexplicit)
		QCC_PR_ParseErrorPrintSRef(ERR_TYPEMISMATCH, var, "Implicit type mismatch. Needed %s, got %s.", basictypenames[wanted], basictypenames[var.cast->type]);
	if (o < 0)
	{
		if (fatal && wanted != ev_variant && var.cast->type != ev_variant)
		{
			if (flag_laxcasts)
			{
				QCC_PR_ParseWarning(WARN_LAXCAST, "Implicit type mismatch. Needed %s, got %s.", basictypenames[wanted], basictypenames[var.cast->type]);
				QCC_PR_ParsePrintSRef(WARN_LAXCAST, var);
			}
			else
				QCC_PR_ParseErrorPrintSRef(ERR_TYPEMISMATCH, var, "Implicit type mismatch. Needed %s, got %s.", basictypenames[wanted], basictypenames[var.cast->type]);
		}
		return var;
	}

	return QCC_PR_Statement(&pr_opcodes[o], var, nullsref, NULL);	//conversion return value
}
QCC_sref_t QCC_MakeTranslateStringConst(char *value);
QCC_sref_t QCC_MakeStringConst(char *value);
QCC_sref_t QCC_MakeFloatConst(float value);
QCC_sref_t QCC_MakeIntConst(int value);
QCC_sref_t QCC_MakeVectorConst(float a, float b, float c);

size_t tempslocked;	//stats
size_t tempsused;
size_t tempsmax;
temp_t *tempsinfo;

QCC_def_t *aliases;
QCC_def_t *allaliases;

static QCC_sref_t QCC_GetTemp(QCC_type_t *type);
void QCC_FreeTemp(QCC_sref_t t);
void QCC_FreeDef(QCC_def_t *def);
QCC_sref_t QCC_MakeSRefForce(QCC_def_t *def, unsigned int ofs, QCC_type_t *type);

//we're about to overwrite the given def, so if there's any aliases to it, we need to clear them out.
static void QCC_ClobberDef(QCC_def_t *def)
{
	QCC_def_t *a, **link;
	for (link = &aliases; *link;)
	{
		a = *link;
		if (!def || a->generatedfor == def)
		{
			//okay, we have a live alias. bum.
			//create a new temp for it. update previous statements to refer to the original location instead of the alias.
			//copy the source into the temp, and then update the alias's def to be a sub-symbol of the temp's def instead of a sub-symbol of the original location.
			//yes. all this just to make mods like xonotic not an insane sea of copies.
			QCC_sref_t tmp, from;
			int st;
			*link = a->nextlocal;
			a->nextlocal = NULL;
			if (a->refcount)
			{
				tmp = QCC_GetTemp(a->type);
				for (st = a->fromstatement; st < numstatements; st++)
				{
					if (statements[st].a.sym == a)
						statements[st].a.sym = a->generatedfor;
					if (statements[st].b.sym == a)
						statements[st].b.sym = a->generatedfor;
					if (statements[st].c.sym == a)
						statements[st].c.sym = a->generatedfor;
				}
				a->symbolheader = tmp.sym;
				from = QCC_MakeSRefForce(a->generatedfor, 0, a->type);
				a->generatedfor = tmp.sym;
				a->name = tmp.sym->name;
				a->temp = tmp.sym->temp;
				a->ofs = tmp.sym->ofs;
				tmp.sym = a;
				tmp.sym->refcount = a->refcount;
				if (a->type->type == ev_vector)
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_V], from, tmp, NULL, STFL_PRESERVEB));
				else
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], from, tmp, NULL, STFL_PRESERVEB));
			}
		}
		else
			link = &(*link)->nextlocal;
	}
}
//return an alias to a reference. typically the return value.
static QCC_sref_t QCC_GetAliasTemp(QCC_sref_t ref)
{
	QCC_def_t *def;
	def = qccHunkAlloc(sizeof(QCC_def_t));
	def->type = ref.cast;
	def->generatedfor = ref.sym;
	def->symbolheader = def;
	def->name = ref.sym->name;
	def->referenced = true;
	def->fromstatement = numstatements;

	//allaliases allows them to be finalized correctly
	def->strip = true;
	def->next = allaliases;
	allaliases = def;

	//and active aliases are the ones that are currently live in their original location
	def->nextlocal = aliases;
	aliases = def;

	QCC_FreeTemp(ref);

	return QCC_MakeSRefForce(def, 0, ref.cast);
}

static QCC_sref_t QCC_GetTemp(QCC_type_t *type)
{
	QCC_sref_t var_c = nullsref;
	size_t u;

	var_c.cast = type;

	if (opt_overlaptemps)	//don't exceed. This lets us allocate a huge block, and still be able to compile smegging big funcs.
	{
		for (u = 0; u < tempsused; u += tempsinfo[u].size)
		{
			if (!tempsinfo[u].def->refcount && tempsinfo[u].size == type->size)
				break;
		}
	}
	else
		u = tempsused;

	if (u == tempsused)
	{
		gofs_t ofs = tempsused;
		unsigned int i;
		unsigned int size = type->size;
		tempsused += size;

		if (tempsused > tempsmax)
		{
			size_t newmax = (tempsused + 64) & ~63;
			tempsinfo = realloc(tempsinfo, newmax*sizeof(*tempsinfo));
			memset(tempsinfo+ofs, 0, (newmax-ofs)*sizeof(*tempsinfo));
			tempsmax = newmax;
		}
		for(i = u; size > 0; i++, size--)
		{
//			tempsinfo[i].used = (i==ofs)?0:2;	//2 is an 'padded' temp. if encountered, scan down for one that doesn't have a 2 there.
			tempsinfo[i].size = (i==ofs)?size:0;
			tempsinfo[i].def = qccHunkAlloc(sizeof(QCC_def_t));
			tempsinfo[i].def->symbolheader = tempsinfo[i].def;
		}
		for (i = 0; i < tempsused; i++)
			tempsinfo[i].def->temp = &tempsinfo[i];
		tempsinfo[u].def->ofs = u;
		tempsinfo[u].def->type = type;
		tempsinfo[u].def->name = "temp";
	}
	else
		optres_overlaptemps+=type->size;

	var_c.sym = tempsinfo[u].def;
	var_c.ofs = 0;//u;
	tempsinfo[u].def->refcount+=1;

	tempsinfo[u].lastfunc = pr_scope;
	tempsinfo[u].laststatement = numstatements;

	var_c.sym->referenced = true;
	return var_c;
}

void QCC_FinaliseTemps(void)
{
	unsigned int i;
	for (i = 0; i < tempsused; )
	{
		tempsinfo[i].def->ofs = numpr_globals;
		numpr_globals += tempsinfo[i].size;
		i += tempsinfo[i].size;
	}

	if (numpr_globals >= MAX_REGS)
	{
		if (!opt_overlaptemps || !opt_locals_overlapping)
			QCC_Error(ERR_TOOMANYGLOBALS, "numpr_globals exceeded MAX_REGS - you'll need to use more optimisations");
		else
			QCC_Error(ERR_TOOMANYGLOBALS, "numpr_globals exceeded MAX_REGS");
	}

	//finalize alises so they map correctly.
	while(allaliases)
	{
		allaliases->ofs = allaliases->generatedfor->ofs;
		allaliases = allaliases->next;
	}
}

void QCC_FreeDef(QCC_def_t *def)
{
	if (def && def->symbolheader)
	{
		if (--def->symbolheader->refcount < 0)
			QCC_PR_ParseWarning(WARN_DEBUGGING, "INTERNAL: over-freed refcount to %s", def->name);
	}
}
//nothing else references this temp.
void QCC_FreeTemp(QCC_sref_t t)
{
	if (t.sym && t.sym->symbolheader)
	{
		if (--t.sym->symbolheader->refcount < 0)
			QCC_PR_ParseWarning(WARN_DEBUGGING, "INTERNAL: over-freed refcount to %s", t.sym->name);
	}
}

static void QCC_ForceUnFreeDef(QCC_def_t *def)
{
	if (def && def->symbolheader)
		def->symbolheader->refcount++;
}
/*
static void QCC_UnFreeDef(QCC_def_t *def)
{
	if (def && def->symbolheader)
	{
		if (!def->symbolheader->refcount++)
			QCC_PR_ParseWarning(WARN_DEBUGGING, "INTERNAL: %s was already fully freed.", def->name);
	}
}
*/
static void QCC_UnFreeTemp(QCC_sref_t t)
{
	if (t.sym && t.sym->symbolheader)
	{
		if (!t.sym->symbolheader->refcount++)
			QCC_PR_ParseWarning(WARN_DEBUGGING, "INTERNAL: %s was already fully freed.", t.sym->name);
	}
}

//We've just parsed a statement.
//We can gaurentee that any used temps are now not used.
#ifdef _DEBUG
static void QCC_FreeTemps(void)
{
}
#else
#define QCC_FreeTemps()
#endif
void QCC_PurgeTemps(void)
{
	free(tempsinfo);
	tempsinfo = NULL;
	tempsmax = 0;
	tempsused = 0;
	aliases = NULL;
	allaliases = NULL;
}

//temps that are still in use over a function call can be considered dodgy.
//we need to remap these to locally defined temps, on return from the function so we know we got them all.
static void QCC_LockActiveTemps(QCC_sref_t exclude)
{
	size_t u;
	size_t excludeofs = ~0;

	QCC_ClobberDef(NULL);

	if (exclude.sym && exclude.sym->temp)
		excludeofs = exclude.sym->temp - tempsinfo;

	for (u = 0; u < tempsused; u += tempsinfo[u].size)
	{
		if (tempsinfo[u].def->refcount && u != excludeofs)	//don't print this after an error jump out.
			tempsinfo[u].locked = true;
	}
}

/*
static void QCC_ForceLockTempForOffset(int ofs)
{
	tempsinfo[ofs].locked = true;
}
*/

static QCC_def_t *QCC_MakeLocked(gofs_t tofs, gofs_t tsize, QCC_def_t *tmp)
{
#ifdef WRITEASM
	char buffer[128];
#endif

	QCC_def_t *def = NULL;
	QCC_def_t *a, **link;
	tempslocked+=tsize;

	def = QCC_PR_DummyDef(type_float, NULL, pr_scope, tsize==1?0:tsize, NULL, 0, false, GDF_STRIP);
#ifdef WRITEASM
	sprintf(buffer, "locked_%i", tofs);
	def->name = qccHunkAlloc(strlen(buffer)+1);
	strcpy(def->name, buffer);
#endif
	def->referenced = true;


	//aliases might refer to this temp.
	//make sure they point to the local instead.
	for (link = &allaliases; *link;)
	{
		a = *link;
		if (a->generatedfor == tmp)
		{
//			*link = a->next;
//			a->next = NULL;
			
			a->generatedfor = def;
			a->name = def->name;
		}
		else
			link = &(*link)->next;
	}
	return def;
}
static void QCC_RemapLockedTemp(gofs_t tofs, gofs_t tsize, int firststatement, int laststatement)
{
	QCC_def_t *def = NULL;
	QCC_statement_t *st;
	int i;

	for (i = firststatement, st = &statements[i]; i < laststatement; i++, st++)
	{
		if (pr_opcodes[st->op].type_a && st->a.sym && st->a.sym->temp && st->a.sym->ofs >= tofs && st->a.sym->ofs < tofs + tsize)
		{
			if (!def)
				def = QCC_MakeLocked(tofs, tsize, st->a.sym);
			st->a.sym = def;
//			st->a.ofs = st->a.ofs - tofs;
		}
		if (pr_opcodes[st->op].type_b && st->b.sym && st->b.sym->temp && st->b.sym->ofs >= tofs && st->b.sym->ofs < tofs + tsize)
		{
			if (!def)
				def = QCC_MakeLocked(tofs, tsize, st->b.sym);
			st->b.sym = def;
//			st->b.ofs = st->b.ofs - tofs;
		}
		if (pr_opcodes[st->op].type_c && st->c.sym && st->c.sym->temp && st->c.sym->ofs >= tofs && st->c.sym->ofs < tofs + tsize)
		{
			if (!def)
				def = QCC_MakeLocked(tofs, tsize, st->c.sym);
			st->c.sym = def;
//			st->c.ofs = st->c.ofs - tofs;
		}
	}
}

static void QCC_RemapLockedTemps(int firststatement, int laststatement)
{
	size_t u;
	for (u = 0; u < tempsused; u += tempsinfo[u].size)
	{
		if (tempsinfo[u].locked)
		{
			QCC_RemapLockedTemp(u, tempsinfo[u].size, firststatement, laststatement);
			tempsinfo[u].locked = false;
		}
	}
}

static void QCC_fprintfLocals(FILE *f, QCC_def_t *locals)
{
	QCC_def_t	*var;
	char typebuf[1024];
	size_t u;

	for (var = locals; var; var = var->nextlocal)
	{
		if (var->arraysize)
			fprintf(f, "local %s %s[%i];\n", TypeName(var->type, typebuf, sizeof(typebuf)), var->name, var->arraysize);
		else
			fprintf(f, "local %s %s;\n", TypeName(var->type, typebuf, sizeof(typebuf)), var->name);
	}

	for (u = 0; u < tempsused; u += tempsinfo[u].size)
	{
		if (!tempsinfo[u].locked)
		{
			fprintf(f, "local %s temp_%i;\n", (tempsinfo[u].size == 1)?"float":"vector", u);
		}
	}
}

#ifdef WRITEASM
void QCC_WriteAsmFunction(QCC_function_t	*sc, unsigned int firststatement, QCC_def_t *firstlocal);
const char *QCC_VarAtOffset(QCC_sref_t ref, unsigned int size)
{	//for debugging, we don't need to preserve the cast.
	static char message[1024];
	//check the temps

	if (ref.sym)
	{
		if (ref.sym && ref.sym->temp)
		{
			QC_snprintfz(message, sizeof(message), "temp_%i+%i", ref.sym->ofs-tempsinfo[0].def->ofs, ref.ofs);
			return message;
		}
		else if (ref.sym->name && !STRCMP(ref.sym->name, "IMMEDIATE"))
		{
			const QCC_eval_t *val = QCC_SRef_EvalConst(ref);
			switch(!val?-1:ref.cast->type)
			{
			case ev_string:
				{
					char *in = &strings[val->string], *out=message;
					char *end = out+sizeof(message)-3;
					*out++ = '\"';
					for(; out < end && *in; in++)
					{
						if (*in == '\n')
						{
							*out++ = '\\';
							*out++ = 'n';
						}
						else if (*in == '\t')
						{
							*out++ = '\\';
							*out++ = 't';
						}
						else if (*in == '\r')
						{
							*out++ = '\\';
							*out++ = 'r';
						}
						else if (*in == '\"')
						{
							*out++ = '\\';
							*out++ = '"';
						}
						else if (*in == '\'')
						{
							*out++ = '\\';
							*out++ = '\'';
						}
						else
							*out++ = *in;
					}
					*out++ = '\"';
					*out++ = 0;
				}
				return message;
			case ev_function:
				if (val->_int>0 && val->_int < numfunctions && *functions[val->_int].name)
					QC_snprintfz(message, sizeof(message), "%s", functions[val->_int].name);
				else
					QC_snprintfz(message, sizeof(message), "%ii", val->_int);
				return message;
			case ev_integer:
				QC_snprintfz(message, sizeof(message), "%ii", val->_int);
				return message;
			case ev_float:
				QC_snprintfz(message, sizeof(message), "%gf", val->_float);
				return message;
			case ev_vector:
				QC_snprintfz(message, sizeof(message), "'%g %g %g'", val->vector[0], val->vector[1], val->vector[2]);
				return message;
			default:
				QC_snprintfz(message, sizeof(message), "IMMEDIATE");
				return message;
			}
		}
		else if (ref.ofs || ref.cast != ref.sym->type)
		{
			QC_snprintfz(message, sizeof(message), "%s+%i", ref.sym->name, ref.ofs);
			return message;
		}

		return ref.sym->name;
	}

	QC_snprintfz(message, sizeof(message), "offset_%i", ref.ofs);
	return message;
}
#endif

#if IAMNOTLAZY
//need_lock is set if it crossed a function call.
static int QCC_PR_FindSourceForTemp(QCC_def_t *tempdef, int op, pbool *need_lock)
{
	int st = -1;
	*need_lock = false;
	if (tempdef->temp)
	{
		for (st = numstatements-1; st>=0; st--)
		{
			if (statements[st].c == tempdef->ofs)
			{
				if (statements[st].op == op)
					return st;
				return -1;
			}

			if ((statements[st].op >= OP_CALL0 && statements[st].op <= OP_CALL8) || (statements[st].op >= OP_CALL1H && statements[st].op <= OP_CALL8H))
				*need_lock = true;
		}
	}
	return st;
}
#endif

static int QCC_PR_FindSourceForAssignedOffset(QCC_def_t *sym, int firstst)
{
	int st = -1;
	for (st = numstatements-1; st>=firstst; st--)
	{
		if (statements[st].c.sym == sym && OpAssignsToC(statements[st].op))
			return st;
		if (statements[st].b.sym == sym && OpAssignsToB(statements[st].op))
			return st;
	}
	return -1;
}

pbool QCC_Temp_Describe(QCC_def_t *def, char *buffer, int buffersize)
{
	QCC_statement_t	*s;
	int st;
	temp_t *t = def->temp;
	if (!t)
		return false;
	if (t->lastfunc != pr_scope)
		return false;
	st = QCC_PR_FindSourceForAssignedOffset(t->def, t->laststatement);
	if (st == -1)
		return false;
	s = &statements[st];
	switch(s->op)
	{
	default:
		QC_snprintfz(buffer, buffersize, "%s %s %s", QCC_VarAtOffset(s->a, 1), pr_opcodes[s->op].name, QCC_VarAtOffset(s->b, 1));
		break;
	}
	return true;
}

static int QCC_PR_RoundFloatConst(const QCC_eval_t *eval)
{
	float val = eval->_float;
	int ival = val;
	if (val != (float)ival)
		QCC_PR_ParseWarning(WARN_CONSTANTCOMPARISON, "Constant float operand not an integer value");
	return ival;
}

QCC_statement_t *QCC_PR_SimpleStatement ( QCC_opcode_t *op, QCC_sref_t var_a, QCC_sref_t var_b, QCC_sref_t var_c, int force);

QCC_sref_t QCC_PR_StatementFlags ( QCC_opcode_t *op, QCC_sref_t var_a, QCC_sref_t var_b, QCC_statement_t **outstatement, unsigned int flags)
{
	char typea[256], typeb[256];
	QCC_statement_t	*statement;
	QCC_sref_t			var_c=nullsref;


	if (var_a.sym)
	{
 		var_a.sym->referenced = true;
		if (flags&STFL_PRESERVEA)
			QCC_UnFreeTemp(var_a);
	}
	if (var_b.sym)
	{
		var_b.sym->referenced = true;
		if (flags&STFL_PRESERVEB)
			QCC_UnFreeTemp(var_b);
	}

	if (op->priority != -1 && op->priority != CONDITION_PRIORITY)
	{
		if (op->associative!=ASSOC_LEFT)
		{
			if (op->type_a != &type_pointer && (flags&STFL_CONVERTB))
				var_b = QCC_SupplyConversion(var_b, (*op->type_a)->type, false);
		}
		else
		{
			if (var_a.cast && (flags&STFL_CONVERTA))
				var_a = QCC_SupplyConversion(var_a, (*op->type_a)->type, false);
			if (var_b.cast && (flags&STFL_CONVERTB))
				var_b = QCC_SupplyConversion(var_b, (*op->type_b)->type, false);
		}
	}

	//maths operators
	if (opt_constantarithmatic || !pr_scope)
	{
		const QCC_eval_t *eval_a = QCC_SRef_EvalConst(var_a);
		const QCC_eval_t *eval_b = QCC_SRef_EvalConst(var_b);
		if (eval_a)
		{
			if (eval_b)
			{
				//both are constants
				switch (op - pr_opcodes)	//improve some of the maths.
				{
				case OP_LOADA_F:
				case OP_LOADA_V:
				case OP_LOADA_S:
				case OP_LOADA_ENT:
				case OP_LOADA_FLD:
				case OP_LOADA_FNC:
				case OP_LOADA_I:
					{
						QCC_sref_t nd = var_a;
						QCC_FreeTemp(var_b);
						nd.ofs += eval_b->_int;
						//FIXME: case away the array...
						return nd;
					}
					break;
				case OP_BITXOR_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int ^ eval_b->_int);
				case OP_RSHIFT_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int >> eval_b->_int);
				case OP_LSHIFT_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int << eval_b->_int);
				case OP_BITXOR_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(QCC_PR_RoundFloatConst(eval_a) ^ QCC_PR_RoundFloatConst(eval_b));
				case OP_RSHIFT_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(QCC_PR_RoundFloatConst(eval_a) >> QCC_PR_RoundFloatConst(eval_b));
				case OP_LSHIFT_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(QCC_PR_RoundFloatConst(eval_a) << QCC_PR_RoundFloatConst(eval_b));
				case OP_BITOR_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(QCC_PR_RoundFloatConst(eval_a) | QCC_PR_RoundFloatConst(eval_b));
				case OP_BITAND_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(QCC_PR_RoundFloatConst(eval_a) & QCC_PR_RoundFloatConst(eval_b));
				case OP_MUL_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(eval_a->_float * eval_b->_float);
				case OP_DIV_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(eval_a->_float / eval_b->_float);
				case OP_ADD_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(eval_a->_float + eval_b->_float);
				case OP_SUB_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(eval_a->_float - eval_b->_float);

				case OP_BITOR_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int | eval_b->_int);
				case OP_BITAND_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int & eval_b->_int);
				case OP_MUL_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int * eval_b->_int);
				case OP_MUL_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(eval_a->_int * eval_b->_float);
				case OP_MUL_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_float * eval_b->_int);
				case OP_DIV_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					if (eval_b->_int == 0)
					{
						QCC_PR_ParseWarning(WARN_CONSTANTCOMPARISON, "Division by constant 0");
						return QCC_MakeIntConst(0);
					}
					else
						return QCC_MakeIntConst(eval_a->_int / eval_b->_int);
				case OP_ADD_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int + eval_b->_int);
				case OP_SUB_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int - eval_b->_int);

				case OP_MOD_I:
					if (!eval_b->_int)
						break;
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int % eval_b->_int);
				case OP_MOD_F:
					{
						float a = eval_a->_float,n=eval_b->_float;
						if (!n)
							break;
						QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
						optres_constantarithmatic++;
						return QCC_MakeFloatConst(a - (n * (int)(a/n)));
					}

				case OP_ADD_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(eval_a->_int + eval_b->_float);
				case OP_ADD_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(eval_a->_float + eval_b->_int);

				case OP_SUB_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(eval_a->_int - eval_b->_float);
				case OP_SUB_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(eval_a->_float - eval_b->_int);

				case OP_AND_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int && eval_b->_int);
				case OP_OR_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_int || eval_b->_int);
				case OP_AND_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_float && eval_b->_float);
				case OP_OR_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_float || eval_b->_float);
				case OP_MUL_V:	//mul_v is actually a dot-product
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(	eval_a->vector[0] * eval_b->vector[0] +
												eval_a->vector[1] * eval_b->vector[1] +
												eval_a->vector[2] * eval_b->vector[2]);
				case OP_MUL_FV:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeVectorConst(	eval_a->_float * eval_b->vector[0],
												eval_a->_float * eval_b->vector[1],
												eval_a->_float * eval_b->vector[2]);
				case OP_MUL_VF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeVectorConst(	eval_a->vector[0] * eval_b->_float,
												eval_a->vector[1] * eval_b->_float,
												eval_a->vector[2] * eval_b->_float);
				case OP_ADD_V:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeVectorConst(	eval_a->vector[0] + eval_b->vector[0],
												eval_a->vector[1] + eval_b->vector[1],
												eval_a->vector[2] + eval_b->vector[2]);
				case OP_SUB_V:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					optres_constantarithmatic++;
					return QCC_MakeVectorConst(	eval_a->vector[0] - eval_b->vector[0],
												eval_a->vector[1] - eval_b->vector[1],
												eval_a->vector[2] - eval_b->vector[2]);


					
				case OP_LE_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int <= eval_b->_int);
				case OP_GE_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int >= eval_b->_int);
				case OP_LT_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int < eval_b->_int);
				case OP_GT_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int > eval_b->_int);

				case OP_LE_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int <= eval_b->_float);
				case OP_GE_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int >= eval_b->_float);
				case OP_LT_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int < eval_b->_float);
				case OP_GT_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int > eval_b->_float);

				case OP_LE_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_float <= eval_b->_int);
				case OP_GE_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_float >= eval_b->_int);
				case OP_LT_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_float < eval_b->_int);
				case OP_GT_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_float > eval_b->_int);

				case OP_EQ_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeFloatConst(eval_a->_float == eval_b->_float);
				case OP_EQ_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int == eval_b->_int);
				case OP_EQ_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int == eval_b->_float);
				case OP_EQ_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_float == eval_b->_int);

				case OP_MUL_VI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeVectorConst(	eval_a->vector[0] * eval_b->_int,
												eval_a->vector[1] * eval_b->_int,
												eval_a->vector[2] * eval_b->_int);
				case OP_MUL_IV:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeVectorConst(	eval_a->_int * eval_b->vector[0],
												eval_a->_int * eval_b->vector[1],
												eval_a->_int * eval_b->vector[2]);
				case OP_DIV_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeFloatConst(eval_a->_int / eval_b->_float);
				case OP_DIV_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeFloatConst(eval_a->_float / eval_b->_int);
				case OP_BITAND_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int & QCC_PR_RoundFloatConst(eval_b));
				case OP_BITOR_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int | QCC_PR_RoundFloatConst(eval_b));
				case OP_BITAND_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(QCC_PR_RoundFloatConst(eval_a) & eval_b->_int);
				case OP_BITOR_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(QCC_PR_RoundFloatConst(eval_a) | eval_b->_int);
				case OP_AND_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int && eval_b->_float);
				case OP_OR_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int || eval_b->_float);
				case OP_AND_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_float && eval_b->_int);
				case OP_OR_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_float || eval_b->_int);
				case OP_NE_F:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeFloatConst(eval_a->_float != eval_b->_float);
				case OP_NE_I:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int != eval_b->_int);
				case OP_NE_IF:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_int != eval_b->_float);
				case OP_NE_FI:
					QCC_FreeTemp(var_a); QCC_FreeTemp(var_b);
					return QCC_MakeIntConst(eval_a->_float != eval_b->_int);
				}
			}
			else
			{
				//a is const, b is not
				switch (op - pr_opcodes)
				{
					//OP_NOT_S needs to do a string comparison
				case OP_NOT_F:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(!eval_a->_float);
				case OP_NOT_V:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(!eval_a->vector[0] && !eval_a->vector[1] && !eval_a->vector[2]);
				case OP_NOT_ENT: // o.O
				case OP_NOT_FNC: // o.O
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(!eval_a->_int);
				case OP_NOT_I:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(!eval_a->_int);
				case OP_BITNOT_F:
					QCC_FreeTemp(var_a);
					return QCC_MakeFloatConst(~QCC_PR_RoundFloatConst(eval_a));
				case OP_BITNOT_I:
					QCC_FreeTemp(var_a);
					return QCC_MakeIntConst(~eval_a->_int);
				case OP_CONV_FTOI:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					return QCC_MakeIntConst(eval_a->_float);
				case OP_CONV_ITOF:
					QCC_FreeTemp(var_a);
					optres_constantarithmatic++;
					return QCC_MakeFloatConst(eval_a->_int);

				case OP_BITOR_F:
				case OP_OR_F:
				case OP_ADD_F:
					if (eval_a->_float == 0)
					{
						optres_constantarithmatic++;
						QCC_FreeTemp(var_a);
						return var_b;
					}
					break;
				case OP_MUL_F:
					if (eval_a->_float == 1)
					{
						optres_constantarithmatic++;
						QCC_FreeTemp(var_a);
						return var_b;
					}
					break;
				case OP_BITAND_F:
				case OP_AND_F:
					if (QCC_PR_RoundFloatConst(eval_a) != 0)
					{
						optres_constantarithmatic++;
						QCC_FreeTemp(var_a);
						return var_b;
					}
					break;

				case OP_BITOR_I:
				case OP_OR_I:
				case OP_ADD_I:
					if (eval_a->_int == 0)
					{
						optres_constantarithmatic++;
						QCC_FreeTemp(var_a);
						return var_b;
					}
					break;
				case OP_MUL_I:
					if (eval_a->_int == 1)
					{
						optres_constantarithmatic++;
						QCC_FreeTemp(var_a);
						return var_b;
					}
					break;
				case OP_BITAND_I:
				case OP_AND_I:
					if (eval_a->_int != 0)
					{
						optres_constantarithmatic++;
						QCC_FreeTemp(var_a);
						return var_b;
					}
					break;
				}
			}
		}
		else if (eval_b)
		{
			//b is const, a is not
			switch (op - pr_opcodes)
			{
#if IAMNOTLAZY
			case OP_LOADA_F:
			case OP_LOADA_V:
			case OP_LOADA_S:
			case OP_LOADA_ENT:
			case OP_LOADA_FLD:
			case OP_LOADA_FNC:
			case OP_LOADA_I:
				{
					QCC_def_t *nd;
					nd = (void *)qccHunkAlloc (sizeof(QCC_def_t));
					memset (nd, 0, sizeof(QCC_def_t));
					nd->type = var_a->type;
					nd->ofs = var_a->ofs + G_INT(var_b->ofs);
					nd->temp = var_a->temp;
					nd->constant = false;
					nd->name = var_a->name;
					return nd;
				}
				break;

			case OP_BITOR_F:
			case OP_OR_F:
			case OP_SUB_F:
			case OP_ADD_F:
				if (eval_b->_float == 0)
				{
					optres_constantarithmatic++;
					QCC_FreeTemp(var_b);
					return var_a;
				}
				break;
			case OP_DIV_F:
			case OP_MUL_F:
				if (eval_b->_float == 1)
				{
					optres_constantarithmatic++;
					QCC_FreeTemp(var_b);
					return var_a;
				}
				break;
			//no bitand_f, I don't trust the casts
			case OP_AND_F:
				if (eval_b->_float)
				{
					optres_constantarithmatic++;
					QCC_FreeTemp(var_b);
					return var_a;
				}
				else
				{	//can never be true, return the false
					optres_constantarithmatic++;
					QCC_FreeTemp(var_a);
					return var_b;
				}
				break;

			case OP_BITOR_I:
			case OP_OR_I:
			case OP_SUB_I:
			case OP_ADD_I:
				if (eval_b->_int == 0)
				{
					optres_constantarithmatic++;
					QCC_FreeTemp(var_b);
					return var_a;
				}
				break;
			case OP_DIV_I:
			case OP_MUL_I:
				if (eval_b->_int == 1)
				{
					optres_constantarithmatic++;
					QCC_FreeTemp(var_b);
					return var_a;
				}
				break;
			case OP_BITAND_I:
				if (eval_b->_int == 0xffffffff)
				{
					optres_constantarithmatic++;
					QCC_FreeTemp(var_b);
					return var_a;
				}
			case OP_AND_I:
				if (eval_b->_int)
				{
					optres_constantarithmatic++;
					QCC_FreeTemp(var_b);
					return var_a;
				}
				else
				{				
					optres_constantarithmatic++;
					QCC_FreeTemp(var_a);
					return var_b;
				}
				break;
#endif
			}
		}
	}

	switch (op - pr_opcodes)
	{
	case OP_STORE_F:
	case OP_STORE_V:
	case OP_STORE_FLD:
	case OP_STORE_P:
	case OP_STORE_I:
	case OP_STORE_ENT:
	case OP_STORE_FNC:
		{
			const QCC_eval_t *idxeval = QCC_SRef_EvalConst(var_a);
			if (idxeval && (var_a.cast->type == ev_integer || var_a.cast->type == ev_float) && !idxeval->_int)
			{
				//you're allowed to assign 0i to anything
				if (op - pr_opcodes == OP_STORE_V)	//make sure vectors get set properly.
				{
					QCC_FreeTemp(var_a);
					var_a = QCC_MakeVectorConst(0, 0, 0);
				}
			}
			/*else
			{
				QCC_type_t *t = var_a->type;
				while(t)
				{
					if (!typecmp_lax(t, var_b->type))
						break;
					t = t->parentclass;
				}
				if (!t)
				{
					TypeName(var_a->type, typea, sizeof(typea));
					TypeName(var_b->type, typeb, sizeof(typeb));
					QCC_PR_ParseWarning(WARN_STRICTTYPEMISMATCH, "Implicit assignment from %s to %s %s", typea, typeb, var_b->name);
				}
			}*/
		}
		break;

	case OP_STOREP_F:
	case OP_STOREP_V:
	case OP_STOREP_FLD:
	case OP_STOREP_P:
	case OP_STOREP_I:
	case OP_STOREP_ENT:
	case OP_STOREP_FNC:
		{
			const QCC_eval_t *idxeval = QCC_SRef_EvalConst(var_a);
			if (idxeval && (var_a.cast->type == ev_integer || var_a.cast->type == ev_float) && !idxeval->_int)
			{
				//you're allowed to assign 0i to anything
				if (op - pr_opcodes == OP_STOREP_V)	//make sure vectors get set properly.
				{
					QCC_FreeTemp(var_a);
					var_a = QCC_MakeVectorConst(0, 0, 0);
				}
			}
			/*else
			{
				QCC_type_t *t = var_a->type;
				while(t)
				{
					if (!typecmp_lax(t, var_b->type->aux_type))
						break;
					t = t->parentclass;
				}
				if (!t)
				{
					TypeName(var_a->type, typea, sizeof(typea));
					TypeName(var_b->type->aux_type, typeb, sizeof(typeb));
					QCC_PR_ParseWarning(WARN_STRICTTYPEMISMATCH, "Implicit field assignment from %s to %s", typea, typeb);
				}
			}*/ 
		}
		break;

	case OP_LOADA_F:
	case OP_LOADA_V:
	case OP_LOADA_S:
	case OP_LOADA_ENT:
	case OP_LOADA_FLD:
	case OP_LOADA_FNC:
	case OP_LOADA_I:
		break;
	case OP_AND_F:
		if (var_a.sym == var_b.sym && var_a.ofs == var_b.ofs)
			QCC_PR_ParseWarning(WARN_CONSTANTCOMPARISON, "Parameter offsets for && are the same");
		if (var_a.sym && var_b.sym && (var_a.sym->constant && var_b.sym->constant))
			QCC_PR_ParseWarning(WARN_CONSTANTCOMPARISON, "Result of comparison is constant");
		break;
	case OP_OR_F:
		if (var_a.sym == var_b.sym && var_a.ofs == var_b.ofs)
			QCC_PR_ParseWarning(WARN_CONSTANTCOMPARISON, "Parameters for || are the same");
		if (var_a.sym && var_b.sym && (var_a.sym->constant || var_b.sym->constant))
			QCC_PR_ParseWarning(WARN_CONSTANTCOMPARISON, "Result of comparison is constant");
		break;
	case OP_EQ_F:
	case OP_EQ_S:
	case OP_EQ_E:
	case OP_EQ_FNC:
//		if (opt_shortenifnots)
//			if (var_b->constant && ((int*)qcc_pr_globals)[var_b->ofs]==0)	// (a == 0) becomes (!a)
//				op = &pr_opcodes[(op - pr_opcodes) - OP_EQ_F + OP_NOT_F];
	case OP_EQ_V:

	case OP_NE_F:
	case OP_NE_V:
	case OP_NE_S:
	case OP_NE_E:
	case OP_NE_FNC:

	case OP_LE_F:
	case OP_GE_F:
	case OP_LT_F:
	case OP_GT_F:
		if (typecmp_lax(var_a.cast, var_b.cast))
		{
			QCC_type_t *t;
			//simplify a, see if we can get an inherited comparison
			for (t = var_a.cast; t; t = t->parentclass)
			{
				if (typecmp_lax(t, var_b.cast))
					break;
			}
			if (t)
				break;
			//now try with b simplified
			for (t = var_b.cast; t; t = t->parentclass)
			{
				if (typecmp_lax(var_a.cast, t))
					break;
			}
			if (t)
				break;
			//if both need to simplify then the classes are too diverse
			TypeName(var_a.cast, typea, sizeof(typea));
			TypeName(var_b.cast, typeb, sizeof(typeb));
			QCC_PR_ParseWarning(WARN_STRICTTYPEMISMATCH, "'%s' type mismatch: %s with %s", op->name, typea, typeb);
		}
		if ((var_a.sym->constant && var_b.sym->constant && !var_a.sym->temp && !var_b.sym->temp) || (var_a.sym == var_b.sym && var_a.ofs == var_b.ofs))
			QCC_PR_ParseWarning(WARN_CONSTANTCOMPARISON, "Result of comparison is constant");
		break;
	case OP_IF_S:
	case OP_IFNOT_S:
	case OP_IF_F:
	case OP_IFNOT_F:
	case OP_IF_I:
	case OP_IFNOT_I:
//		if (var_a.cast->type == ev_function && !var_a.sym->temp)
//			QCC_PR_ParseWarning(WARN_CONSTANTCOMPARISON, "Result of comparison is constant");
//		if (!var_a.sym || var_a.sym->constant && !var_a.sym->temp)
//			QCC_PR_ParseWarning(WARN_CONSTANTCOMPARISON, "Result of comparison is constant");
		break;
	default:
		break;
	}

	if (numstatements)
	{	//optimise based on last statement.
		if (op - pr_opcodes == OP_IFNOT_I)
		{
			if (opt_shortenifnots && var_a.cast && var_a.sym->refcount == 1 && (statements[numstatements-1].op == OP_NOT_F || statements[numstatements-1].op == OP_NOT_FNC || statements[numstatements-1].op == OP_NOT_ENT))
			{
				if (statements[numstatements-1].c.sym == var_a.sym && statements[numstatements-1].c.ofs == var_a.ofs)
				{
					if (statements[numstatements-1].op == OP_NOT_F && QCC_OPCodeValid(&pr_opcodes[OP_IF_F]))
						op = &pr_opcodes[OP_IF_F];
					else
						op = &pr_opcodes[OP_IF_I];
					numstatements--;
					QCC_FreeTemp(var_a);
					var_a = statements[numstatements].a;
					QCC_ForceUnFreeDef(var_a.sym);

					optres_shortenifnots++;
				}
			}
		}
		else if (op - pr_opcodes == OP_IFNOT_F)
		{
			if (opt_shortenifnots && var_a.cast && var_a.sym->refcount == 1 && statements[numstatements-1].op == OP_NOT_F)
			{
				if (statements[numstatements-1].c.sym == var_a.sym && statements[numstatements-1].c.ofs == var_a.ofs)
				{
					op = &pr_opcodes[OP_IF_F];
					numstatements--;
					QCC_FreeTemp(var_a);
					var_a = statements[numstatements].a;
					QCC_ForceUnFreeDef(var_a.sym);

					optres_shortenifnots++;
				}
			}
		}
		else if (op - pr_opcodes == OP_IFNOT_S)
		{
			if (opt_shortenifnots && var_a.cast && statements[numstatements-1].op == OP_NOT_S)
			{
				if (statements[numstatements-1].c.sym == var_a.sym && statements[numstatements-1].c.ofs == var_a.ofs)
				{
					op = &pr_opcodes[OP_IF_S];
					numstatements--;
					QCC_FreeTemp(var_a);
					var_a = statements[numstatements].a;
					QCC_ForceUnFreeDef(var_a.sym);

					optres_shortenifnots++;
				}
			}
		}
		else if (((unsigned) ((op - pr_opcodes) - OP_STORE_F) < 6) || (op-pr_opcodes) == OP_STORE_P)
		{
			// remove assignments if what should be assigned is the 3rd operand of the previous statement?
			// don't if it's a call, callH, switch or case
			// && var_a->ofs >RESERVED_OFS)
			if (OpAssignsToC(statements[numstatements-1].op) &&
			    opt_assignments && var_a.cast && var_a.sym == statements[numstatements-1].c.sym && var_a.ofs == statements[numstatements-1].c.ofs)
			{
				if (var_a.cast->type == var_b.cast->type)
				{
					if (var_a.sym && var_b.sym && var_a.sym->temp && var_a.sym->refcount==1)
					{
						statement = &statements[numstatements-1];
						statement->c = var_b;

						if (var_a.cast->type != var_b.cast->type)
							QCC_PR_ParseWarning(0, "store type mismatch");
						var_b.sym->referenced=true;
						var_a.sym->referenced=true;
						QCC_FreeTemp(var_a);
						optres_assignments++;

						return var_b;
					}
				}
			}
		}
	}

	if (!QCC_OPCodeValid(op))
	{
		QCC_sref_t tmp;
//FIXME: add support for flags so we don't corrupt temps
		switch(op - pr_opcodes)
		{
		case OP_LOADA_STRUCT:
			/*emit this anyway. if it reaches runtime then you messed up.
			this is valid only if you do &foo[0]*/
//			QCC_PR_ParseWarning(0, "OP_LOADA_STRUCT: cannot emulate");
			break;

		case OP_ADD_SF:
			var_c = QCC_PR_GetSRef(NULL, "AddStringFloat", NULL, false, 0, 0);
			if (var_c.cast)
			{
				QCC_type_t *types[2] = {type_string, type_float};
				QCC_sref_t evals[2];
				evals[0] = var_a;	//stupid msvc bug
				evals[1] = var_b;
				var_a = QCC_PR_GenerateFunctionCall(nullsref, var_c, evals, types, 2);
			}
			else
			{
				QCC_PR_ParseWarning(0, "string(string,float) AddStringFloat: emulation depends upon denormals");
				var_b = QCC_SupplyConversion(var_b, ev_integer, true);	//FIXME: this should be an unconditional float->int conversion
				var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_F], var_a, var_b, NULL, 0);
			}
			var_c.cast = type_string;
			return var_c;
		case OP_ADD_SI:
			QCC_PR_ParseWarning(0, "OP_ADD_SI: denormals may be unsafe");
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_F], var_a, var_b, NULL, 0);
			var_c.cast = type_string;
			return var_c;
		case OP_ADD_PF:
		case OP_ADD_FP:
		case OP_ADD_PI:
		case OP_ADD_IP:
			var_c = (op == &pr_opcodes[OP_ADD_PF] || op == &pr_opcodes[OP_ADD_PI])?var_a:var_b;
			var_b = (op == &pr_opcodes[OP_ADD_PF] || op == &pr_opcodes[OP_ADD_PI])?var_b:var_a;
			if (op == &pr_opcodes[OP_ADD_FP] || op == &pr_opcodes[OP_ADD_PF])
				var_b = QCC_SupplyConversion(var_b, ev_integer, true);	//FIXME: this should be an unconditional float->int conversion
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_I], var_b, QCC_MakeIntConst(var_c.cast->size), NULL, 0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_PIW], var_c, var_b, NULL, 0);
		case OP_SUB_PF:
		case OP_SUB_PI:
			var_c = var_a;
			var_b = var_b;
			if (op == &pr_opcodes[OP_SUB_PF])
				var_b = QCC_SupplyConversion(var_b, ev_integer, true);	//FIXME: this should be an unconditional float->int conversion
			//fixme: word size
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_I], var_b, QCC_MakeIntConst(var_c.cast->size*4), NULL, 0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_I], var_c, var_b, NULL, 0);
		case OP_SUB_PP:
			if (typecmp(var_a.cast, var_b.cast))
				QCC_PR_ParseError(0, "incompatible pointer types");
			//determine byte offset
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_I], var_a, var_b, NULL, 0);
			//determine divisor (fixme: word size)
			var_b = QCC_MakeIntConst(var_c.cast->size*4);
			//divide the result
			return QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_I], var_c, var_b, NULL, 0);

		case OP_BITAND_I:
			{
				QCC_type_t *argt[2] = {type_integer, type_integer};
				QCC_sref_t fnc = QCC_PR_GetSRef(NULL, "BitandInt", NULL, false, 0, 0);
				QCC_sref_t arg[2];arg[0] = var_a;arg[1]=var_b; //MSVC sucks when arg is a struct array. its fine when its a def_t ptr array
				if (!fnc.cast)
					QCC_PR_ParseError(0, "BitandInt function not defined: cannot emulate int+int");
				var_c = QCC_PR_GenerateFunctionCall(nullsref, fnc, arg, argt, 2);
				var_c.cast = type_integer;
				return var_c;
			}
			break;
		case OP_BITOR_I:
			{
				QCC_type_t *argt[2] = {type_integer, type_integer};
				QCC_sref_t fnc = QCC_PR_GetSRef(NULL, "BitorInt", NULL, false, 0, 0);
				QCC_sref_t arg[2];arg[0] = var_a;arg[1]=var_b; //MSVC sucks when arg is a struct array. its fine when its a def_t ptr array
				if (!fnc.cast)
					QCC_PR_ParseError(0, "BitorInt function not defined: cannot emulate int+int");
				var_c = QCC_PR_GenerateFunctionCall(nullsref, fnc, arg, argt, 2);
				var_c.cast = type_integer;
				return var_c;
			}
			break;
		case OP_ADD_I:
			{
				QCC_type_t *argt[2] = {type_integer, type_integer};
				QCC_sref_t fnc = QCC_PR_GetSRef(NULL, "AddInt", NULL, false, 0, 0);
				QCC_sref_t arg[2];arg[0] = var_a;arg[1]=var_b; //MSVC sucks when arg is a struct array. its fine when its a def_t ptr array
				if (!fnc.cast)
					QCC_PR_ParseError(0, "AddInt function not defined: cannot emulate int+int");
				var_c = QCC_PR_GenerateFunctionCall(nullsref, fnc, arg, argt, 2);
				var_c.cast = type_integer;
				return var_c;
			}
			break;
		case OP_MOD_I:
			{
				QCC_type_t *argt[2] = {type_integer, type_integer};
				QCC_sref_t fnc = QCC_PR_GetSRef(NULL, "ModInt", NULL, false, 0, 0);
				QCC_sref_t arg[2];arg[0] = var_a;arg[1]=var_b; //MSVC sucks when arg is a struct array. its fine when its a def_t ptr array
				if (!fnc.cast)
					QCC_PR_ParseError(0, "ModInt function not defined: cannot emulate int%%int");
				var_c = QCC_PR_GenerateFunctionCall(nullsref, fnc, arg, argt, 2);
				var_c.cast = type_integer;
				return var_c;
			}
			break;
		case OP_MOD_F:
			{
				QCC_type_t *argt[2] = {type_float, type_float};
				QCC_sref_t fnc = QCC_PR_GetSRef(NULL, "mod", NULL, false, 0, 0);
				QCC_sref_t arg[2];arg[0] = var_a;arg[1]=var_b; //MSVC sucks when arg is a struct array. its fine when its a def_t ptr array
				if (!fnc.cast)
				{
					//a - (n * floor(a/n));
					//(except using v|v instead of floor)
					var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_F], var_a, var_b, NULL, STFL_CONVERTA|STFL_CONVERTB|STFL_PRESERVEA|STFL_PRESERVEB);
					var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITOR_F], var_c, var_c, NULL, STFL_PRESERVEA);
					var_c = QCC_PR_Statement(&pr_opcodes[OP_MUL_F], var_b, var_c, NULL);
					return QCC_PR_Statement(&pr_opcodes[OP_SUB_F], var_a, var_c, NULL);

//					QCC_PR_ParseError(0, "mod function not defined: cannot emulate float%%float");
				}
				var_c = QCC_PR_GenerateFunctionCall(nullsref, fnc, arg, argt, 2);
				var_c.cast = type_float;
				return var_c;
			}
			break;
		case OP_MOD_V:
			{
				QCC_type_t *argt[2] = {type_vector, type_vector};
				QCC_sref_t fnc = QCC_PR_GetSRef(NULL, "ModVec", NULL, false, 0, 0);
				QCC_sref_t arg[2];arg[0] = var_a;arg[1]=var_b; //MSVC sucks when arg is a struct array. its fine when its a def_t ptr array
				if (!fnc.cast)
					QCC_PR_ParseError(0, "ModVec function not defined: cannot emulate vector%%vector");
				var_c = QCC_PR_GenerateFunctionCall(nullsref, fnc, arg, argt, 2);
				var_c.cast = type_vector;
				return var_c;
			}
			break;
		case OP_SUB_I:
			{
				QCC_type_t *argt[2] = {type_integer, type_integer};
				QCC_sref_t fnc = QCC_PR_GetSRef(NULL, "SubInt", NULL, false, 0, 0);
				QCC_sref_t arg[2];arg[0] = var_a;arg[1]=var_b; //MSVC sucks when arg is a struct array. its fine when its a def_t ptr array
				if (!fnc.cast)
					QCC_PR_ParseError(0, "SubInt function not defined: cannot emulate int-int");
				var_c = QCC_PR_GenerateFunctionCall(nullsref, fnc, arg, argt, 2);
				var_c.cast = type_integer;
				return var_c;
			}
			break;
		case OP_MUL_I:
			{
				QCC_type_t *argt[2] = {type_integer, type_integer};
				QCC_sref_t fnc = QCC_PR_GetSRef(NULL, "MulInt", NULL, false, 0, 0);
				QCC_sref_t arg[2];arg[0] = var_a;arg[1]=var_b; //MSVC sucks when arg is a struct array. its fine when its a def_t ptr array
				if (!fnc.cast)
					QCC_PR_ParseError(0, "MulInt function not defined: cannot emulate int*int");
				var_c = QCC_PR_GenerateFunctionCall(nullsref, fnc, arg, argt, 2);
				var_c.cast = type_integer;
				return var_c;
			}
			break;
		case OP_DIV_I:
			{
				QCC_type_t *argt[2] = {type_integer, type_integer};
				QCC_sref_t fnc = QCC_PR_GetSRef(NULL, "DivInt", NULL, false, 0, 0);
				QCC_sref_t arg[2];arg[0] = var_a;arg[1]=var_b; //MSVC sucks when arg is a struct array. its fine when its a def_t ptr array
				if (!fnc.cast)
					QCC_PR_ParseError(0, "DivInt function not defined: cannot emulate int/int");
				var_c = QCC_PR_GenerateFunctionCall(nullsref, fnc, arg, argt, 2);
				var_c.cast = type_integer;
				return var_c;
			}
			break;

		case OP_DIV_VF:
			//v/f === v*(1/f)
			op = &pr_opcodes[OP_MUL_VF];
			var_b = QCC_PR_Statement(&pr_opcodes[OP_DIV_F], QCC_MakeFloatConst(1), var_b, NULL);
			break;

		case OP_CONV_ITOF:
		case OP_STORE_IF:
			{
				const QCC_eval_t *eval_a = QCC_SRef_EvalConst(var_a);
				if (eval_a)
				{
					QCC_FreeTemp(var_a);
					var_a = QCC_MakeFloatConst(eval_a->_int);
				}
				else
				{
					var_c = QCC_PR_GetSRef(NULL, "itof", NULL, false, 0, 0);
					if (!var_c.cast)
					{
						QCC_PR_ParseError(0, "itof function not defined: cannot emulate int -> float conversions");
					}
					var_a = QCC_PR_GenerateFunctionCall(nullsref, var_c, &var_a, &type_integer, 1);
					var_a.cast = type_float;
				}
				if (var_b.cast)
				{
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], var_a, var_b, NULL, 0));
					return var_b;
				}
			}
			return var_a;
		case OP_CONV_FTOI:
		case OP_STORE_FI:
			{
				const QCC_eval_t *eval_a = QCC_SRef_EvalConst(var_a);
				if (eval_a)
				{
					QCC_FreeTemp(var_a);
					var_a = QCC_MakeIntConst(eval_a->_float);
				}
				else
				{
					var_c = QCC_PR_GetSRef(NULL, "ftoi", NULL, false, 0, 0);
					if (!var_c.cast)
					{
						//with denormals, 5 * 1i -> 5i
						QCC_PR_ParseWarning(0, "ftoi emulation: denormals have limited precision");
						var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_F], var_a, QCC_MakeIntConst(1), NULL, 0);
					}
					else
						var_a = QCC_PR_GenerateFunctionCall(nullsref, var_c, &var_a, &type_float, 1);
					var_a.cast = type_integer;
				}
				if (var_b.cast)
				{
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_I], var_a, var_b, NULL, 0));
					return var_b;
				}
			}
			return var_a;
		case OP_STORE_I:
			op = pr_opcodes+OP_STORE_F;
			break;

		case OP_BITXOR_F:
//			a = (a & ~b) | (b & ~a);
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITNOT_F], var_b, nullsref, NULL, STFL_PRESERVEA);
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_F], var_a, var_c, NULL, STFL_PRESERVEA);
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_BITNOT_F], var_a, nullsref, NULL, 0);
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_F], var_b, var_a, NULL, 0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_BITOR_F], var_c, var_a, NULL, STFL_PRESERVEA);

		case OP_IF_S:
			tmp = QCC_MakeFloatConst(0);
			tmp.cast = type_string;
			var_a = QCC_PR_Statement(&pr_opcodes[OP_NE_S], var_a, tmp, NULL);
			op = &pr_opcodes[OP_IF_I];
			break;

		case OP_IFNOT_S:
			tmp = QCC_MakeFloatConst(0);
			tmp.cast = type_string;
			var_a = QCC_PR_Statement(&pr_opcodes[OP_NE_S], var_a, tmp, NULL);
			op = &pr_opcodes[OP_IFNOT_I];
			break;

		case OP_IF_F:
			tmp = QCC_MakeFloatConst(0);
			var_a = QCC_PR_Statement(&pr_opcodes[OP_NE_F], var_a, tmp, NULL);
			op = &pr_opcodes[OP_IF_I];
			break;

		case OP_IFNOT_F:
			tmp = QCC_MakeFloatConst(0);
			var_a = QCC_PR_Statement(&pr_opcodes[OP_NE_F], var_a, tmp, NULL);
			op = &pr_opcodes[OP_IFNOT_I];
			break;

		case OP_ADDSTORE_F:
			op = &pr_opcodes[OP_ADD_F];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;
		case OP_ADDSTORE_I:
			op = &pr_opcodes[OP_ADD_I];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;
		case OP_ADDSTORE_FI:
			op = &pr_opcodes[OP_ADD_FI];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;
//		case OP_ADDSTORE_IF:
//			fixme: result is a float but needs to be an int
//			op = &pr_opcodes[OP_ADD_IF];
//			tmp = var_b;
//			var_b = var_a;
//			var_a = tmp;
//			break;

		case OP_SUBSTORE_F:
			op = &pr_opcodes[OP_SUB_F];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;
		case OP_SUBSTORE_FI:
			op = &pr_opcodes[OP_SUB_FI];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;
//		case OP_SUBSTORE_IF:
//			fixme: result is a float but needs to be an int
//			op = &pr_opcodes[OP_SUB_IF];
//			tmp = var_b;
//			var_b = var_a;
//			var_a = tmp;
//			break;
		case OP_SUBSTORE_I:
			op = &pr_opcodes[OP_SUB_I];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;

		case OP_BITNOT_I:
			op = &pr_opcodes[OP_SUB_I];
			var_b = var_a;
			var_a = QCC_MakeIntConst(~0);
			break;
		case OP_BITNOT_F:
			op = &pr_opcodes[OP_SUB_F];
			var_b = var_a;
			var_a = QCC_MakeFloatConst(-1);	//divVerent says -1 is safe, even with floats. I guess I'm just too paranoid.
			break;

		case OP_DIVSTORE_F:
			op = &pr_opcodes[OP_DIV_F];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;
		case OP_DIVSTORE_FI:
			op = &pr_opcodes[OP_DIV_FI];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;
//		case OP_DIVSTORE_IF:
//			fixme: result is a float, but needs to be an int
//			op = &pr_opcodes[OP_DIV_IF];
//			tmp = var_b;
//			var_b = var_a;
//			var_a = tmp;
//			break;
		case OP_DIVSTORE_I:
			op = &pr_opcodes[OP_DIV_I];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;

		case OP_MULSTORE_F:
			op = &pr_opcodes[OP_MUL_F];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;
//		case OP_MULSTORE_IF:
//			fixme: result is a float, but needs to be an int
//			op = &pr_opcodes[OP_MUL_IF];
//			var_c = var_b;
//			var_b = var_a;
//			var_a = var_c;
//			break;
		case OP_MULSTORE_FI:
			op = &pr_opcodes[OP_MUL_FI];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;

		case OP_ADDSTORE_V:
			op = &pr_opcodes[OP_ADD_V];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;

		case OP_SUBSTORE_V:
			op = &pr_opcodes[OP_SUB_V];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;

		case OP_MULSTORE_VF:
			op = &pr_opcodes[OP_MUL_VF];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;
		case OP_MULSTORE_VI:
			op = &pr_opcodes[OP_MUL_VI];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;

		case OP_BITSETSTORE_I:
			op = &pr_opcodes[OP_BITOR_I];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;
		case OP_BITSETSTORE_F:
			op = &pr_opcodes[OP_BITOR_F];
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			break;

		case OP_STOREP_P:
			op = &pr_opcodes[OP_STOREP_I];
			break;

		case OP_EQ_P:
			op = &pr_opcodes[OP_EQ_I];
			break;
		case OP_NE_P:
			op = &pr_opcodes[OP_NE_I];
			break;

		case OP_GT_P:
			op = &pr_opcodes[OP_GT_I];
			break;
		case OP_GE_P:
			op = &pr_opcodes[OP_GE_I];
			break;
		case OP_LE_P:
			op = &pr_opcodes[OP_LE_I];
			break;
		case OP_LT_P:
			op = &pr_opcodes[OP_LT_I];
			break;

		case OP_BITCLR_I:
			tmp = var_b;
			var_b = var_a;
			var_a = tmp;
			if (QCC_OPCodeValid(&pr_opcodes[OP_BITCLRSTORE_I]))
			{
				op = &pr_opcodes[OP_BITCLRSTORE_I];
				break;
			}
			//fallthrough
		case OP_BITCLRSTORE_I:
			//b = var, a = bit field.
			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_I], var_b, var_a, NULL, STFL_PRESERVEA);

			op = &pr_opcodes[OP_SUB_I];
			var_a = var_b;
			var_b = var_c;
			var_c = ((op - pr_opcodes)==OP_BITCLRSTORE_I)?var_a:nullsref;
			break;
		case OP_BITCLR_F:
			var_c = var_b;
			var_b = var_a;
			var_a = var_c;
			if (QCC_OPCodeValid(&pr_opcodes[OP_BITCLRSTORE_F]))
			{
				op = &pr_opcodes[OP_BITCLRSTORE_F];
				break;
			}
			//fallthrough
		case OP_BITCLRSTORE_F:
			//b = var, a = bit field.

			//b - (b&a)

			var_c = QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_F], var_b, var_a, NULL, STFL_PRESERVEA);

			op = &pr_opcodes[OP_SUB_F];
			var_a = var_b;
			var_b = var_c;
			var_c = ((op - pr_opcodes)==OP_BITCLRSTORE_F)?var_a:nullsref;
			break;
#if IAMNOTLAZY
		case OP_SUBSTOREP_FI:
		case OP_SUBSTOREP_IF:
		case OP_ADDSTOREP_FI:
		case OP_ADDSTOREP_IF:
		case OP_MULSTOREP_FI:
		case OP_MULSTOREP_IF:
		case OP_DIVSTOREP_FI:
		case OP_DIVSTOREP_IF:

		case OP_MULSTOREP_VF:
		case OP_MULSTOREP_VI:
		case OP_SUBSTOREP_V:
		case OP_ADDSTOREP_V:

		case OP_SUBSTOREP_F:
		case OP_SUBSTOREP_I:
		case OP_ADDSTOREP_I:
		case OP_ADDSTOREP_F:
		case OP_MULSTOREP_F:
		case OP_DIVSTOREP_F:
		case OP_BITSETSTOREP_F:
		case OP_BITSETSTOREP_I:
		case OP_BITCLRSTOREP_F:
//			QCC_PR_ParseWarning(0, "XSTOREP_F emulation is still experimental");
			QCC_UnFreeTemp(var_a);
			QCC_UnFreeTemp(var_b);

			statement = &statements[numstatements++];

			//don't chain these... this expansion is not the same.
			{
				int st;
				pbool need_lock;
				st = QCC_PR_FindSourceForTemp(var_b, OP_ADDRESS, &need_lock);

				var_c = QCC_GetTemp(*op->type_c);
				if (st < 0)
				{
					/*generate new OP_LOADP instruction*/
					statement->op = ((*op->type_c)->type==ev_vector)?OP_LOADP_V:OP_LOADP_F;
					statement->a = var_b->ofs;
					statement->b = var_c->ofs;
					statement->c = 0;
				}
				else
				{
					/*it came from an OP_ADDRESS - st says the instruction*/
					if (need_lock)
					{
						QCC_ForceLockTempForOffset(statements[st].a);
						QCC_ForceLockTempForOffset(statements[st].b);
//						QCC_LockTemp(var_c); /*that temp needs to be preserved over calls*/
					}

					/*generate new OP_ADDRESS instruction - FIXME: the arguments may have changed since the original instruction*/
					statement->op = OP_ADDRESS;
					statement->a = statements[st].a;
					statement->b = statements[st].b;
					statement->c = var_c->ofs;
					statement->linenum = statements[st].linenum;

					/*convert old one to an OP_LOAD*/
					statements[st].op = ((*op->type_c)->type==ev_vector)?OP_LOAD_V:OP_LOAD_F;
//					statements[st].a = statements[st].a;
//					statements[st].b = statements[st].b;
//					statements[st].c = statements[st].c;
					statements[st].linenum = pr_token_line_last;
				}
			}

			statement = &statements[numstatements++];

			statement->linenum = pr_token_line_last;
			switch(op - pr_opcodes)
			{
			case OP_SUBSTOREP_V:
				statement->op = OP_SUB_V;
				break;
			case OP_ADDSTOREP_V:
				statement->op = OP_ADD_V;
				break;
			case OP_MULSTOREP_VF:
				statement->op = OP_MUL_VF;
				break;
			case OP_MULSTOREP_VI:
				statement->op = OP_MUL_VI;
				break;
			case OP_SUBSTOREP_F:
				statement->op = OP_SUB_F;
				break;
			case OP_SUBSTOREP_I:
				statement->op = OP_SUB_I;
				break;
			case OP_SUBSTOREP_IF:
				statement->op = OP_SUB_IF;
				break;
			case OP_SUBSTOREP_FI:
				statement->op = OP_SUB_FI;
				break;
			case OP_ADDSTOREP_IF:
				statement->op = OP_ADD_IF;
				break;
			case OP_ADDSTOREP_FI:
				statement->op = OP_ADD_FI;
				break;
			case OP_MULSTOREP_IF:
				statement->op = OP_MUL_IF;
				break;
			case OP_MULSTOREP_FI:
				statement->op = OP_MUL_FI;
				break;
			case OP_DIVSTOREP_IF:
				statement->op = OP_DIV_IF;
				break;
			case OP_DIVSTOREP_FI:
				statement->op = OP_DIV_FI;
				break;
			case OP_ADDSTOREP_F:
				statement->op = OP_ADD_F;
				break;
			case OP_ADDSTOREP_I:
				statement->op = OP_ADD_I;
				break;
			case OP_MULSTOREP_F:
				statement->op = OP_MUL_F;
				break;
			case OP_DIVSTOREP_F:
				statement->op = OP_DIV_F;
				break;
			case OP_BITSETSTOREP_F:
				statement->op = OP_BITOR_F;
				break;
			case OP_BITSETSTOREP_I:
				statement->op = OP_BITOR_I;
				break;
			case OP_BITCLRSTOREP_F:
				//float pointer float
				temp = QCC_GetTemp(type_float);
				statement->op = OP_BITAND_F;
				statement->a = var_c ? var_c->ofs : 0;
				statement->b = var_a ? var_a->ofs : 0;
				statement->c = temp->ofs;

				statement = &statements[numstatements];
				numstatements++;

				statement->linenum = pr_token_line_last;
				statement->op = OP_SUB_F;

				//t = c & i
				//c = c - t
				break;
			default:	//no way will this be hit...
				QCC_PR_ParseError(ERR_INTERNAL, "opcode invalid 3 times %i", op - pr_opcodes);
			}
			if (op - pr_opcodes == OP_BITCLRSTOREP_F)
			{
				statement->a = var_b ? var_b->ofs : 0;
				statement->b = temp ? temp->ofs : 0;
				statement->c = var_b->ofs;
				QCC_FreeTemp(temp);
				QCC_FreeTemp(var_a);
				var_a = var_b;	//this is the value.
				var_b = var_c;	//this is the ptr.
			}
			else
			{
				statement->a = var_b ? var_b->ofs : 0;
				statement->b = var_a ? var_a->ofs : 0;
				statement->c = var_b->ofs;
				QCC_FreeTemp(var_a);
				var_a = var_b;	//this is the value.
				var_b = var_c;	//this is the ptr.
			}

			op = &pr_opcodes[((*op->type_c)->type==ev_vector)?OP_STOREP_V:OP_STOREP_F];
			QCC_FreeTemp(var_c);
			var_c = NULL;
			QCC_FreeTemp(var_b);
			break;
#endif
		//statements where the rhs is an input int and can be swapped with a float
		case OP_ADD_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_SUB_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_BITAND_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_BITOR_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_BITOR_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_LT_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_LT_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_LE_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_LE_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_GT_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_GT_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_GE_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_GE_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_EQ_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_EQ_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_NE_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_NE_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_DIV_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_MUL_FI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_F], var_a, var_b, NULL, flags&STFL_PRESERVEA);
		case OP_MUL_VI:
			var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_b, nullsref, NULL, (flags&STFL_PRESERVEB)?STFL_PRESERVEA:0);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_VF], var_a, var_b, NULL, flags&STFL_PRESERVEA);

		//statements where the lhs is an input int and can be swapped with a float
		case OP_ADD_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_SUB_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_BITAND_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_BITAND_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_BITOR_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_BITOR_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_LT_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_LT_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_LE_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_LE_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_GT_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_GT_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_GE_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_GE_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_EQ_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_EQ_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_NE_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_NE_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_DIV_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_MUL_IF:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_F], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_MUL_IV:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_FV], var_a, var_b, NULL, flags&STFL_PRESERVEB);

		//statements where both sides will need to be converted to floats to work

		case OP_LE_I:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_LE_FI], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_GE_I:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_GE_FI], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_LT_I:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_LT_FI], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_GT_I:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_GT_FI], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_EQ_I:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_EQ_FI], var_a, var_b, NULL, flags&STFL_PRESERVEB);
		case OP_NE_I:
			var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], var_a, nullsref, NULL, flags&STFL_PRESERVEA);
			return QCC_PR_StatementFlags(&pr_opcodes[OP_NE_FI], var_a, var_b, NULL, flags&STFL_PRESERVEB);

		case OP_AND_I:
		case OP_AND_FI:
		case OP_AND_IF:
		case OP_AND_ANY:
			if (var_a.cast->type == ev_vector && flag_vectorlogic)	//we can do a dot-product to test if a vector has a value, instead of a double-not
				var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_V], var_a, var_a, NULL, flags&STFL_PRESERVEA);
			if (var_b.cast->type == ev_vector && flag_vectorlogic)
				var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_V], var_b, var_b, NULL, flags&STFL_PRESERVEB);

			if (((var_a.cast->size != 1 && flag_vectorlogic) || (var_a.cast->type == ev_string && flag_ifstring)) && ((var_b.cast->size != 1 && flag_vectorlogic) || (var_b.cast->type == ev_string && flag_ifstring)))
			{	//just 3 extra instructions instead of 4.
				var_a = QCC_PR_GenerateLogicalNot(var_a, "%s used as truth value");
				var_b = QCC_PR_GenerateLogicalNot(var_b, "%s used as truth value");
				var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_OR_ANY], var_a, var_b, NULL, 0);
				return QCC_PR_GenerateLogicalNot(var_a, "%s isn't a float...");
			}
			if ((var_a.cast->size != 1 && flag_vectorlogic) || (var_a.cast->type == ev_string && flag_ifstring))
				var_a = QCC_PR_GenerateLogicalNot(QCC_PR_GenerateLogicalNot(var_a, "%s used as truth value"), "%s isn't a float...");
			if ((var_b.cast->size != 1 && flag_vectorlogic) || (var_b.cast->type == ev_string && flag_ifstring))
				var_b = QCC_PR_GenerateLogicalNot(QCC_PR_GenerateLogicalNot(var_b, "%s used as truth value"), "%s isn't a float...");
			if (var_a.cast->type != ev_float && var_b.cast->type != ev_float && QCC_OPCodeValid(&pr_opcodes[OP_AND_I]))
				op = &pr_opcodes[OP_AND_I];	//negative 0 as a float is considered zero by the fpu, which makes 0x80000000 tricky. avoid that if we can.
			else if (var_a.cast->type != ev_float && var_b.cast->type == ev_float && QCC_OPCodeValid(&pr_opcodes[OP_AND_IF]))
				op = &pr_opcodes[OP_AND_IF];
			else if (var_a.cast->type == ev_float && var_b.cast->type != ev_float && QCC_OPCodeValid(&pr_opcodes[OP_AND_FI]))
				op = &pr_opcodes[OP_AND_FI];
			else
				op = &pr_opcodes[OP_AND_F];	//generally works. if there's no other choice then meh.
			break;
		case OP_OR_I:
		case OP_OR_FI:
		case OP_OR_IF:
		case OP_OR_ANY:
			if (var_a.cast->type == ev_vector && flag_vectorlogic)	//we can do a dot-product to test if a vector has a value, instead of a double-not
				var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_V], var_a, var_a, NULL, flags&STFL_PRESERVEA);
			if (var_b.cast->type == ev_vector && flag_vectorlogic)
				var_b = QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_V], var_b, var_b, NULL, flags&STFL_PRESERVEB);

			if (((var_a.cast->size != 1 && flag_vectorlogic) || (var_a.cast->type == ev_string && flag_ifstring)) && ((var_b.cast->size != 1 && flag_vectorlogic) || (var_b.cast->type == ev_string && flag_ifstring)))
			{	//just 3 extra instructions instead of 4.
				var_a = QCC_PR_GenerateLogicalNot(var_a, "%s used as truth value");
				var_b = QCC_PR_GenerateLogicalNot(var_b, "%s used as truth value");
				var_a = QCC_PR_StatementFlags(&pr_opcodes[OP_AND_ANY], var_a, var_b, NULL, 0);
				return QCC_PR_GenerateLogicalNot(var_a, "%s isn't a float...");
			}
			if ((var_a.cast->size != 1 && flag_vectorlogic) || (var_a.cast->type == ev_string && flag_ifstring))
				var_a = QCC_PR_GenerateLogicalNot(QCC_PR_GenerateLogicalNot(var_a, "%s used as truth value"), "%s isn't a float...");
			if ((var_b.cast->size != 1 && flag_vectorlogic) || (var_b.cast->type == ev_string && flag_ifstring))
				var_b = QCC_PR_GenerateLogicalNot(QCC_PR_GenerateLogicalNot(var_b, "%s used as truth value"), "%s isn't a float...");
			if (var_a.cast->type != ev_float && var_b.cast->type != ev_float && QCC_OPCodeValid(&pr_opcodes[OP_OR_I]))
				op = &pr_opcodes[OP_OR_I];	//negative 0 as a float is considered zero by the fpu, which makes 0x80000000 tricky. avoid that if we can.
			else if (var_a.cast->type != ev_float && var_b.cast->type == ev_float && QCC_OPCodeValid(&pr_opcodes[OP_OR_IF]))
				op = &pr_opcodes[OP_OR_IF];
			else if (var_a.cast->type == ev_float && var_b.cast->type != ev_float && QCC_OPCodeValid(&pr_opcodes[OP_OR_FI]))
				op = &pr_opcodes[OP_OR_FI];
			else
				op = &pr_opcodes[OP_OR_F];	//generally works. if there's no other choice then meh.
			break;
		default:
			{
				int oldtarg = qcc_targetformat;
				pbool shamelessadvertising;
				qcc_targetformat = QCF_FTE;
				shamelessadvertising = QCC_OPCodeValid(op);
				qcc_targetformat = oldtarg;
				if (shamelessadvertising)
					QCC_PR_ParseError(ERR_BADEXTENSION, "Opcode \"%s|%s\" not valid for target. Consider the use of: #pragma target fte", op->name, op->opname);
				else
					QCC_PR_ParseError(ERR_BADEXTENSION, "Opcode \"%s|%s\" is not supported.", op->name, op->opname);
			}
			break;
		}
	}

	if (!pr_scope)
		QCC_PR_ParseError(ERR_BADEXTENSION, "Unable to generate statements at global scope.\n");


	if (op->type_c == &type_void || op->associative==ASSOC_RIGHT || op->type_c == NULL)
	{
		QCC_FreeTemp(var_b);	//returns a instead of some result/temp
		if (flags&STFL_DISCARDRESULT)
			QCC_FreeTemp(var_a);
	}
	else
	{
		QCC_FreeTemp(var_a);
		QCC_FreeTemp(var_b);
	}

	if (outstatement)
		QCC_ClobberDef(NULL);

	statement = &statements[numstatements++];

	if (outstatement)
		*outstatement = statement;

	statement->linenum = pr_token_line_last;
	statement->op = op - pr_opcodes;
	statement->a = var_a;
	statement->b = var_b;

	if (var_c.cast && var_c.sym && !var_c.sym->referenced)
	{
		QCC_PR_ParseWarning(WARN_DEBUGGING, "INTERNAL: var_c was not referenced");
		QCC_PR_ParsePrintSRef(WARN_DEBUGGING, var_c);
	}
	if (var_b.cast && var_b.sym && !var_b.sym->referenced)
	{
		QCC_PR_ParseWarning(WARN_DEBUGGING, "INTERNAL: var_b was not referenced");
		QCC_PR_ParsePrintSRef(WARN_DEBUGGING, var_b);
	}
	if (var_a.cast && var_a.sym && !var_a.sym->referenced)
	{
		QCC_PR_ParseWarning(WARN_DEBUGGING, "INTERNAL: var_a was not referenced");
		QCC_PR_ParsePrintSRef(WARN_DEBUGGING, var_a);
	}

	if (var_c.cast)
		statement->c = var_c;
	else if (op->type_c == &type_void || op->associative==ASSOC_RIGHT || op->type_c == NULL)
	{
		var_c = nullsref;
		statement->c = nullsref;			// ifs, gotos, and assignments
										// don't need vars allocated
		if (flags&STFL_DISCARDRESULT)
			return nullsref;
		return var_a;
	}
	else if (op-pr_opcodes == OP_ADD_PIW)
	{
		var_c = QCC_GetTemp(var_a.cast);
		statement->c = var_c;
	}
	else
	{	// allocate result space
		var_c = QCC_GetTemp(*op->type_c);
		statement->c = var_c;
		if (op->type_b == &type_field)
		{
			//&(a.b) returns a pointer to b, so that pointer's auxtype should have the same type as b's auxtype
			if (var_b.cast->type == ev_variant)
				var_c.cast = type_variant;
			else if (var_c.cast->type == ev_pointer)
				var_c.cast = QCC_PR_PointerType(var_b.cast->aux_type);
			else if (var_c.cast->type == ev_field)
				var_c.cast = QCC_PR_FieldType(var_b.cast->aux_type);
			else
				var_c.cast = var_b.cast->aux_type;
		}
	}

	if (flags&STFL_DISCARDRESULT)
	{
		QCC_FreeTemp(var_c);
		var_c = nullsref;
	}
	return var_c;
}

/*
============
QCC_PR_SimpleStatement

Emits a primitive statement
============
*/
QCC_statement_t *QCC_PR_SimpleStatement ( QCC_opcode_t *op, QCC_sref_t var_a, QCC_sref_t var_b, QCC_sref_t var_c, int force)
{
	QCC_statement_t	*statement;

	if (!force && !QCC_OPCodeValid(op))
	{
//		outputversion = op->extension;
//		if (noextensions)
		QCC_PR_ParseError(ERR_BADEXTENSION, "Opcode \"%s|%s\" not valid for target. Consider the use of: #pragma target fte\n", op->name, op->opname);
	}

	statement = &statements[numstatements];
	numstatements++;

	statement->op = op - pr_opcodes;
	statement->a = var_a;
	statement->b = var_b;
	statement->c = var_c;
	statement->linenum = pr_token_line_last;

	return statement;
}

/*
============
PR_ParseImmediate

Looks for a preexisting constant
============
*/
QCC_sref_t	QCC_PR_ParseImmediate (void)
{
	QCC_sref_t	cn;

	switch(pr_immediate_type->type)
	{
	case ev_float:
		cn = QCC_MakeFloatConst(pr_immediate._float);
		QCC_PR_Lex ();
		return cn;

	case ev_integer:
		cn = QCC_MakeIntConst(pr_immediate._int);
		QCC_PR_Lex ();
		return cn;

	case ev_vector:
		cn = QCC_MakeVectorConst(pr_immediate.vector[0], pr_immediate.vector[1], pr_immediate.vector[2]);
		QCC_PR_Lex ();
		return cn;

	case ev_string:
		{
			int t,l;
			char tmp[8192];
			t=l = strlen(pr_immediate_string);
			if (l+1 > sizeof(tmp))
				QCC_PR_ParseError (ERR_NAMETOOLONG, "string immediate is too long");
			memcpy(tmp, pr_immediate_string, l);
			tmp[l] = 0;
			
			//extra logic to amalgamate any "additional " "string" " immediates", like in C
			for(;;)
			{
				QCC_PR_Lex ();
				if (pr_token_type == tt_immediate && pr_immediate_type == type_string)
				{
					l = strlen(pr_immediate_string);
					if (t+l+1 > sizeof(tmp))
						QCC_PR_ParseError (ERR_NAMETOOLONG, "string immediate is too long");
					memcpy(tmp+t, pr_immediate_string, l);
					tmp[t+l] = 0;
					t+=l;
				}
				else
					break;
			} 

			cn = QCC_MakeStringConst(tmp);
		}
		return cn;
	default:
		QCC_PR_ParseError (ERR_BADIMMEDIATETYPE, "weird immediate type");
		return nullsref;
	}
}

QCC_ref_t *QCC_PR_GenerateAddressOf(QCC_ref_t *retbuf, QCC_ref_t *operand)
{
//	QCC_def_t *e2;
	if (operand->type == REF_FIELD)
	{
		//&e.f should generate a pointer def
		//as opposed to a ref
		return QCC_PR_BuildRef(retbuf,
				REF_GLOBAL,
				QCC_PR_Statement(&pr_opcodes[OP_ADDRESS], operand->base, operand->index, NULL), 
				nullsref,
				QCC_PR_PointerType((operand->index.cast->type == ev_field)?operand->index.cast->aux_type:type_variant),
				true);
	}
	if (operand->type == REF_ARRAYHEAD || operand->type == REF_GLOBAL || operand->type == REF_ARRAY)
	{
		if (!QCC_OPCodeValid(&pr_opcodes[OP_GLOBALADDRESS]))
			QCC_PR_ParseError (ERR_BADEXTENSION, "Address-of operator is not supported in this form without extensions. Consider the use of: #pragma target fte");

		//&foo (or &((&foo)[5]), which is basically an array). the result is a temp and thus cannot be assigned to (but should be possible to dereference further).
		return QCC_PR_BuildRef(retbuf,
				REF_GLOBAL,
				QCC_PR_Statement(&pr_opcodes[OP_GLOBALADDRESS], operand->base, operand->index.cast?QCC_SupplyConversion(operand->index, ev_integer, true):nullsref, NULL), 
				nullsref,
				QCC_PR_PointerType(operand->cast),
				true);
	}
	if (operand->type == REF_POINTER)
	{
		//&(p[5]) just reverts back to p+5. it cannot be assigned to.
		QCC_sref_t addr;
		if (operand->index.cast)
		{
			if (!QCC_OPCodeValid(&pr_opcodes[OP_ADD_PIW]))
				QCC_PR_ParseError (ERR_BADEXTENSION, "Address-of operator is not supported in this form without extensions. Consider the use of: #pragma target fte");
			addr = QCC_PR_Statement(&pr_opcodes[OP_ADD_PIW], operand->base, QCC_SupplyConversion(operand->index, ev_integer, true), NULL);
		}
		else
			addr = operand->base;
		return QCC_PR_BuildRef(retbuf,
				REF_GLOBAL,
				addr, 
				nullsref,
				QCC_PR_PointerType(operand->cast),
				true);
	}
	QCC_PR_ParseError (ERR_BADEXTENSION, "Cannot use addressof operator ('&') on a global. Please use the FTE target.");
	return operand;
}


void QCC_PrecacheSound (const char *n, int ch)
{
	int		i;

	if (!*n)
		return;
	for (i=0 ; i<numsounds ; i++)
		if (!STRCMP(n, precache_sound[i].name))
			return;
	if (numsounds == QCC_MAX_SOUNDS)
		return;
//		QCC_Error ("PrecacheSound: numsounds == MAX_SOUNDS");
	strcpy (precache_sound[i].name, n);
	if (ch >= '1'  && ch <= '9')
		precache_sound[i].block = ch - '0';
	else
		precache_sound[i].block = 1;
	numsounds++;
}

void QCC_PrecacheModel (const char *n, int ch)
{
	int		i;

	if (!*n)
		return;
	for (i=0 ; i<nummodels ; i++)
		if (!STRCMP(n, precache_model[i].name))
		{
			if (!precache_model[i].block)
			{
				if (ch >= '1'  && ch <= '9')
					precache_model[i].block = ch - '0';
				else
					precache_model[i].block = 1;
			}
			return;
		}
	if (nummodels == QCC_MAX_MODELS)
		return;
//		QCC_Error ("PrecacheModels: nummodels == MAX_MODELS");
	strcpy (precache_model[i].name, n);
	if (ch >= '1'  && ch <= '9')
		precache_model[i].block = ch - '0';
	else
		precache_model[i].block = 1;
	precache_model[i].filename = strings+s_file;
	precache_model[i].fileline = pr_source_line;
	nummodels++;
}

void QCC_SetModel (const char *n)
{
	int		i;

	if (!*n)
		return;
	for (i=0 ; i<nummodels ; i++)
		if (!STRCMP(n, precache_model[i].name))
		{
			precache_model[i].used++;
			return;
		}
	if (nummodels == QCC_MAX_MODELS)
		return;
	strcpy (precache_model[i].name, n);
	precache_model[i].block = 0;
	precache_model[i].used=1;

	precache_model[i].filename = strings+s_file;
	precache_model[i].fileline = pr_source_line;
	nummodels++;
}

void QCC_PrecacheTexture (const char *n, int ch)
{
	int		i;

	if (!*n)
		return;
	for (i=0 ; i<numtextures ; i++)
		if (!STRCMP(n, precache_texture[i].name))
			return;
	if (nummodels == QCC_MAX_MODELS)
		return;
//		QCC_Error ("PrecacheTextures: numtextures == MAX_TEXTURES");
	strcpy (precache_texture[i].name, n);
	if (ch >= '1'  && ch <= '9')
		precache_texture[i].block = ch - '0';
	else
		precache_texture[i].block = 1;
	numtextures++;
}

void QCC_PrecacheFile (const char *n, int ch)
{
	int		i;

	if (!*n)
		return;
	for (i=0 ; i<numfiles ; i++)
		if (!STRCMP(n, precache_file[i].name))
			return;
	if (numfiles == QCC_MAX_FILES)
		return;
//		QCC_Error ("PrecacheFile: numfiles == MAX_FILES");
	strcpy (precache_file[i].name, n);
	if (ch >= '1'  && ch <= '9')
		precache_file[i].block = ch - '0';
	else
		precache_file[i].block = 1;
	numfiles++;
}

#ifdef SUPPORTINLINE
struct inlinectx_s
{
	QCC_def_t		*fdef;
	QCC_function_t	*func;
	QCC_sref_t *arglist;

	QCC_sref_t		result;

	struct
	{
		QCC_def_t *def;
		QCC_def_t *srcsym;
		int bias;
	} locals[64];
	int numlocals;
};
static pbool QCC_PR_InlinePushResult(struct inlinectx_s *ctx, QCC_sref_t src, QCC_sref_t mappedto)
{
	int i;
	for (i = 0; i < ctx->numlocals; i++)
	{
		if (ctx->locals[i].srcsym == src.sym)
			break;
	}
	if (i == ctx->numlocals)
	{
		if (ctx->numlocals >= sizeof(ctx->locals)/sizeof(ctx->locals[0]))
			return false;
		ctx->locals[i].srcsym = src.sym;
		ctx->numlocals++;
	}
	else if (ctx->locals[i].def)
		QCC_FreeDef(ctx->locals[i].def);
	ctx->locals[i].def = mappedto.sym;
	ctx->locals[i].bias = mappedto.ofs - src.ofs;	//FIXME: this feels unsafe (needed for array[immediate] fixups)
	return true;
}

static QCC_sref_t QCC_PR_InlineFindDef(struct inlinectx_s *ctx, QCC_sref_t src, pbool assign)
{
	QCC_def_t *d;
	int p;
//	int pstart = ctx->func->parm_start;

	//aliases are weird and annoying
	if (src.sym && src.sym->generatedfor)
		src.sym = src.sym->generatedfor;

	for (p = 0; p < ctx->numlocals; p++)
	{
		if (ctx->locals[p].srcsym == src.sym && ctx->locals[p].def)
		{
			d = ctx->locals[p].def;
			if (assign)
			{
				QCC_FreeDef(ctx->locals[p].def);
				ctx->locals[p].def = NULL;
				ctx->locals[p].bias = 0;
				d = NULL;
				return QCC_MakeSRefForce(NULL, 0, NULL);
			}
			return QCC_MakeSRefForce(d, src.ofs + ctx->locals[p].bias, src.cast);
		}
	}

	if (src.sym && (src.sym->localscope || src.sym->temp))
	{
		//if its a parm, use that
		QCC_def_t *local;
		for (local = ctx->func->firstlocal, p = 0; local && p < MAX_PARMS && (unsigned int)p < ctx->func->type->num_parms; local = local->deftail->nextlocal, p++)
		{
			if (src.sym->symbolheader == local)
			{
				return QCC_MakeSRefForce(ctx->arglist[p].sym->symbolheader, src.ofs, src.cast);
			}
		}

		//otherwise its a local or a temp.
		if (ctx->numlocals >= sizeof(ctx->locals)/sizeof(ctx->locals[0]))
			return nullsref;
		ctx->locals[ctx->numlocals].srcsym = src.sym;
		ctx->locals[ctx->numlocals].def = QCC_GetTemp(src.sym->type).sym;
		return QCC_MakeSRefForce(ctx->locals[ctx->numlocals++].def, src.ofs, src.cast);
	}
	return QCC_MakeSRefForce(src.sym, src.ofs, src.cast);
/*
	if (ofs < RESERVED_OFS)
	{
		if (ofs == OFS_RETURN)
		{
			def_ret.type = type_void;
			return &def_ret;
		}
		ofs -= OFS_PARM0;
		ofs /= 3;
		def_parms[ofs].type = type_void;
		return &def_parms[ofs];
	}
	if (ofs < pstart || ofs >= pstart+ctx->func->locals)
	{
		for (d = &pr.def_head; d; d = d->next)
		{
			if (d->ofs == ofs)
				return d;
		}
		return NULL;	//not found?
	}

	for (p = 0; p < ctx->func->numparms; pstart += ctx->func->parm_size[p++])
	{
		if (ofs < pstart+ctx->func->parm_size[p])
			return ctx->arglist[p];
	}

	if (ctx->numlocals >= sizeof(ctx->locals)/sizeof(ctx->locals[0]))
		return NULL;
	ctx->locals[ctx->numlocals].srcofs = ofs;
	ctx->locals[ctx->numlocals].def = QCC_GetTemp(type_float);
	return ctx->locals[ctx->numlocals++].def;
*/
}

//returns a string saying why inlining failed.
static char *QCC_PR_InlineStatements(struct inlinectx_s *ctx)
{
	/*FIXME: what happens with:
	t = foo;
	foo = 5;
	return t;
	*/
	QCC_sref_t a, b, c;
	QCC_statement_t *st;
	const QCC_eval_t *eval;
//	float af,bf;
//	int i;

	st = &statements[ctx->func->code];
	while(1)
	{
		switch(st->op)
		{
		case OP_IF_F:
		case OP_IFNOT_F:
		case OP_IF_I:
		case OP_IFNOT_I:
		case OP_IF_S:
		case OP_IFNOT_S:
			if (st->b.ofs > 0 && st[st->b.ofs].op == OP_DONE)
			{
				//logically, an if statement around the entire function is safe because the locals are safe
			}
		case OP_GOTO:
		case OP_SWITCH_F:
		case OP_SWITCH_I:
		case OP_SWITCH_E:
		case OP_SWITCH_FNC:
		case OP_SWITCH_S:
		case OP_SWITCH_V:
			return "function contains branches";	//conditionals are not supported in any way. this keeps the code linear. each input can only come from a single place.
/*		case OP_CALL0:
		case OP_CALL1:
		case OP_CALL2:
		case OP_CALL3:
		case OP_CALL4:
		case OP_CALL5:
		case OP_CALL6:
		case OP_CALL7:
		case OP_CALL8:
		case OP_CALL1H:
		case OP_CALL2H:
		case OP_CALL3H:
		case OP_CALL4H:
		case OP_CALL5H:
		case OP_CALL6H:
		case OP_CALL7H:
		case OP_CALL8H:
			return "function contains function calls";	//conditionals are not supported in any way. this keeps the code linear. each input can only come from a single place.
*/
		case OP_RETURN:
		case OP_DONE:
			a = QCC_PR_InlineFindDef(ctx, st->a, false);
			ctx->result = a;
			if (!a.cast)
			{
				if (ctx->func->type->aux_type->type == ev_void)
					ctx->result.cast = type_void;
				else
					return "OP_RETURN no a";
			}
			return NULL;
		case OP_BOUNDCHECK:
			a = QCC_PR_InlineFindDef(ctx, st->a, false);
			QCC_PR_InlinePushResult(ctx, st->a, a);
			eval = QCC_SRef_EvalConst(a);
			if (eval)
			{
				if (eval->_int < (int)st->c.ofs || eval->_int >= (int)st->b.ofs)
					QCC_PR_ParseWarning(0, "constant value exceeds bounds failed bounds check while inlining\n");
			}
			else
				QCC_PR_SimpleStatement(&pr_opcodes[OP_BOUNDCHECK], a, st->b, st->c, false);
			break;
		default:
			if ((st->op >= OP_CALL0 && st->op <= OP_CALL8) || (st->op >= OP_CALL1H && st->op <= OP_CALL8H))
			{
				//function calls are a little weird in that they have no outputs
				if (st->a.cast)
				{
					a = QCC_PR_InlineFindDef(ctx, st->a, false);
					if (!a.cast)
						return "unable to determine what a was";
				}
				else
					a = nullsref;

				if (st->b.cast)
				{
					b = QCC_PR_InlineFindDef(ctx, st->b, false);
					if (!b.cast)
						return "unable to determine what a was";
				}
				else
					b = nullsref;
				if (st->c.cast)
				{
					c = QCC_PR_InlineFindDef(ctx, st->c, false);
					if (!c.cast)
						return "unable to determine what a was";
				}
				else
					c = nullsref;

				{
					QCC_sref_t r;
					r.sym = &def_ret;
					r.ofs = 0;
					r.cast = a.cast;
					QCC_PR_InlinePushResult(ctx, r, nullsref);
				}

				QCC_ClobberDef(&def_ret);
				QCC_FreeTemp(a);
				QCC_FreeTemp(b);
				QCC_FreeTemp(c);
				QCC_LockActiveTemps(a);
				QCC_PR_SimpleStatement(&pr_opcodes[st->op], a, b, c, false);

				{
					QCC_sref_t r;
					r.sym = &def_ret;
					r.ofs = 0;
					r.cast = a.cast;
					QCC_PR_InlinePushResult(ctx, r, QCC_GetAliasTemp(QCC_MakeSRefForce(&def_ret, 0, a.cast->aux_type)));
				}
			}
			else
			if (pr_opcodes[st->op].associative == ASSOC_RIGHT)
			{
				//a->b
				if (st->a.cast)
				{
					a = QCC_PR_InlineFindDef(ctx, st->a, false);
					if (!a.cast)
						return "unable to determine what a was";
				}
				else
					a = nullsref;
				b = QCC_PR_InlineFindDef(ctx, st->b, true);
				c = QCC_PR_StatementFlags(&pr_opcodes[st->op], a, b, NULL, 0);

				if (!QCC_PR_InlinePushResult(ctx, st->b, c))
					return "too many temps";
			}
			else
			{
				//a+b->c
				if (st->a.cast)
				{
					a = QCC_PR_InlineFindDef(ctx, st->a, false);
					if (!a.cast)
						return "unable to determine what a was";
				}
				else
					a = nullsref;
				if (st->b.cast)
				{
					b = QCC_PR_InlineFindDef(ctx, st->b, false);
					if (!b.cast)
						return "unable to determine what b was";
				}
				else
					b = nullsref;

				if (pr_opcodes[st->op].associative == ASSOC_LEFT && pr_opcodes[st->op].type_c != &type_void)
				{
					QCC_sref_t r;
					c = QCC_PR_InlineFindDef(ctx, st->c, true);
					r = QCC_PR_StatementFlags(&pr_opcodes[st->op], a, b, NULL, 0);
					if (c.cast && !QCC_SRef_EvalConst(r))
						c = QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_ENT], r, c, NULL, 0);
					else
					{
						QCC_FreeTemp(c);
						c = r;
					}

					if (!QCC_PR_InlinePushResult(ctx, st->c, c))
						return "too many temps";
				}
				else
				{
					if (st->c.cast)
					{
						c = QCC_PR_InlineFindDef(ctx, st->c, false);
						if (!c.cast)
							return "unable to determine what c was";
					}
					else
						c = nullsref;
					QCC_PR_SimpleStatement(&pr_opcodes[st->op], a, b, c, false);
				}
			}
			break;
		}
		st++;
	}
}
#endif
static QCC_sref_t QCC_PR_Inline(QCC_sref_t fdef, QCC_sref_t *arglist, unsigned int argcount)
{
#ifndef SUPPORTINLINE
	return nullsref;
#else
//	QCC_def_t *dd = NULL;
	struct inlinectx_s ctx;
	char *error;
	int statements, i;
	const QCC_eval_t *eval = QCC_SRef_EvalConst(fdef);
	//make sure that its a function type and that there's no special weirdness
	if (!eval || fdef.cast != fdef.sym->type || eval->function < 0 || argcount > 8 || eval->function >= numfunctions || fdef.sym->arraysize != 0 || fdef.cast->type != ev_function || argcount != fdef.cast->num_parms || fdef.cast->vargs || fdef.cast->vargcount)
		return nullsref;

	ctx.numlocals = 0;
	ctx.arglist = arglist;
	ctx.fdef = fdef.sym;
	ctx.result = nullsref;
	ctx.func = &functions[eval->function];
	if ((int)ctx.func->code <= 0)
		return nullsref;	//don't try to inline builtins. that simply cannot work.

	//FIXME: inefficient: we can't revert this on failure, so make sure its done early, just in case.
	if (argcount && arglist[0].sym->generatedfor == &def_ret)
		QCC_ClobberDef(&def_ret);
	QCC_ClobberDef(NULL);

	statements = numstatements;
	error = QCC_PR_InlineStatements(&ctx);
	if (error)
	{
		QCC_PR_ParseWarning(0, "Couldn't inline \"%s\": %s", ctx.func->name, error);
		QCC_PR_ParsePrintDef(0, fdef.sym);
	}
	

	for(i = 0; i < ctx.numlocals; i++)
	{
		if (ctx.locals[i].def)
			QCC_FreeDef(ctx.locals[i].def);
	}

	if (!ctx.result.cast)
		numstatements = statements;	//on failure, remove the extra statements
	else
	{	//on success, make sure the args were freed
		while (argcount-->0)
			QCC_FreeTemp(arglist[argcount]);
	}

	return ctx.result;
#endif
}
QCC_sref_t QCC_PR_GenerateFunctionCall (QCC_sref_t newself, QCC_sref_t func, QCC_sref_t *arglist, QCC_type_t **argtypelist, int argcount)	//warning, the func could have no name set if it's a field call.
{
	QCC_sref_t		d, oself, self, retval;
	int			i;
	QCC_type_t		*t;
//	int np;

	int callconvention;
	QCC_statement_t *st;

	func.sym->timescalled++;

	if (!newself.cast && func.sym->constant && func.sym->allowinline)
	{
		d = QCC_PR_Inline(func, arglist, argcount);
		if (d.cast)
		{
			optres_inlines++;
			func.sym->referenced = true;	//not really, but hey, the warning is stupid.
			QCC_FreeTemp(func);
			return d;
		}
	}

	if (QCC_OPCodeValid(&pr_opcodes[OP_CALL1H]))
		callconvention = OP_CALL1H;	//FTE extended
	else
		callconvention = OP_CALL1;	//standard

	t = func.cast;
	if (t->type != ev_function && t->type != ev_variant)
	{
		QCC_PR_ParseErrorPrintSRef (ERR_NOTAFUNCTION, func, "not a function");
	}

	self = nullsref;
	oself = nullsref;
	d = nullsref;

	if (newself.cast)
	{
		//we're entering OO code with a different self. make sure self is preserved.
		//eg: other.touch(self)

		self = QCC_PR_GetSRef(type_entity, "self", NULL, true, 0, false);
		if (newself.ofs != self.ofs || newself.sym != self.sym)
		{
			oself = QCC_GetTemp(pr_classtype?pr_classtype:type_entity);
			//oself = self
			QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_ENT], self, oself, nullsref, false);
			//self = other
			QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_ENT], newself, self, nullsref, false);

			//if the args refered to self, update them to refer to oself instead
			//(as self is now set to 'other')
			for (i = 0; i < argcount; i++)
			{
				if (arglist[i].ofs == self.ofs && arglist[i].sym == self.sym)
				{
					QCC_FreeTemp(arglist[i]);
					arglist[i] = oself;
					QCC_UnFreeTemp(arglist[i]);
				}
			}
		}
		else
		{
			QCC_FreeTemp(self);
			self = nullsref;
		}
		QCC_FreeTemp(newself);
	}

//	write the arguments (except for first two if hexenc)
	for (i = 0; i < argcount; i++)
	{
		if (i>=MAX_PARMS)
		{
			d = extra_parms[i - MAX_PARMS];
			if (!d.cast)
			{
				char name[128];
				QC_snprintfz(name, sizeof(name), "$parm%u", i);
				d = extra_parms[i - MAX_PARMS] = QCC_PR_GetSRef(type_vector, name, NULL, true, 0, GDF_STRIP);
			}
			else
				QCC_ForceUnFreeDef(d.sym);
		}
		else
		{
			d.sym = &def_parms[i];
			d.ofs = 0;
			d.cast = type_vector;
		}
		if (argtypelist && argtypelist[i])
			d.cast = argtypelist[i];
		else
			d.cast = arglist[i].cast;

		if (callconvention == OP_CALL1H && i < 2)
		{
			//first two args are passed in the call opcode, so don't need to be copied
			arglist[i].sym->referenced = true;
			d.sym->referenced = true;
			/*don't free these temps yet, free them after the return check*/
		}
		//FIXME: if the def is a temp with only one reference, we can update the statement that generated the temp to directly store to the parm
		else if (d.cast->size == 3 || !opt_nonvec_parms)
			QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[OP_STORE_V], arglist[i], d, NULL, 0));
		else
		{
			switch(d.cast->type)
			{
			case ev_entity:
				QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[OP_STORE_ENT], arglist[i], d, NULL, 0));
				break;
			case ev_string:
				QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[OP_STORE_S], arglist[i], d, NULL, 0));
				break;
			case ev_field:
				QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[OP_STORE_FLD], arglist[i], d, NULL, 0));
				break;
			case ev_function:
				QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[OP_STORE_FNC], arglist[i], d, NULL, 0));
				break;
			default:
				QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[OP_STORE_F], arglist[i], d, NULL, 0));
				break;
			}
			optres_nonvec_parms++;
		}
	}

	if (func.cast->vargcount)
	{
		QCC_sref_t va_passcount = QCC_PR_GetSRef(type_float, "__va_count", NULL, true, 0, 0);
		QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[OP_STORE_F], QCC_MakeFloatConst(argcount), va_passcount, NULL, 0));
	}

	QCC_ClobberDef(&def_ret);

	/*can free temps used for arguments now*/
	if (callconvention == OP_CALL1H)
	{
		for (i = 0; i < argcount && i < 2; i++)
			QCC_FreeTemp(arglist[i]);
	}

	//we dont need to lock the local containing the function index because its thrown away after the call anyway
	//(if a function is called in the argument list then it'll be locked as part of that call)
	QCC_LockActiveTemps(func);	//any temps before are likly to be used with the return value.

	//generate the call
	if (argcount>MAX_PARMS)
		QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[callconvention-1+MAX_PARMS], func, nullsref, (QCC_statement_t **)&st));
	else if (argcount)
		QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[callconvention-1+argcount], func, nullsref, (QCC_statement_t **)&st));
	else
		QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_CALL0], func, nullsref, (QCC_statement_t **)&st));

	if (callconvention == OP_CALL1H)
	{
		if (argcount)
		{
			st->b = arglist[0];
//			QCC_FreeTemp(param[0]);
			if (argcount>1)
			{
				st->c = arglist[1];
//				QCC_FreeTemp(param[1]);
			}
		}
	}

	if (t->type == ev_variant)
		retval = QCC_GetAliasTemp(QCC_MakeSRefForce(&def_ret, 0, type_variant));
	else
		retval = QCC_GetAliasTemp(QCC_MakeSRefForce(&def_ret, 0, t->aux_type));

	//restore the class owner
	if (oself.cast)
		QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_ENT], oself, self, nullsref, false);
	QCC_FreeTemp(oself);
	QCC_FreeTemp(self);

	return retval;
}

/*
============
PR_ParseFunctionCall
============
*/
QCC_sref_t QCC_PR_ParseFunctionCall (QCC_ref_t *funcref)	//warning, the func could have no name set if it's a field call.
{
	QCC_sref_t	newself, func;
	QCC_sref_t	e, d, out;
	unsigned int			arg;
	QCC_type_t		*t, *p;
	int extraparms=false;
	unsigned int np;
	const char *funcname;

	QCC_sref_t param[MAX_PARMS+MAX_EXTRA_PARMS];
	QCC_type_t *paramtypes[MAX_PARMS+MAX_EXTRA_PARMS];

	if (funcref->type == REF_FIELD && strstr(QCC_GetSRefName(funcref->index), "::"))
	{
		newself = funcref->base;
		QCC_UnFreeTemp(newself);
		func = QCC_RefToDef(funcref, true);
	}
	else if (funcref->type == REF_NONVIRTUAL)
	{
		newself = funcref->index;
		QCC_UnFreeTemp(newself);
		func = QCC_RefToDef(funcref, true);
	}
	else
	{
		newself = nullsref;
		func = QCC_RefToDef(funcref, true);
	}

	func.sym->timescalled++;

	t = func.cast;

	if (t->type == ev_variant)
	{
		t->aux_type = type_variant;
	}

	if (t->type != ev_function && t->type != ev_variant)
	{
		QCC_PR_ParseErrorPrintSRef (ERR_NOTAFUNCTION, func, "not a function");
	}

	funcname = QCC_GetSRefName(func);
	if (!newself.cast && !t->num_parms&&t->type != ev_variant)	//intrinsics. These base functions have variable arguments. I would check for (...) args too, but that might be used for extended builtin functionality. (this code wouldn't compile otherwise)
	{
		if (!strcmp(funcname, "sizeof"))
		{
			QCC_type_t *t;
			if (!func.sym->initialized)
				func.sym->initialized = 3;
			func.sym->referenced = true;
			QCC_FreeTemp(func);
			t = QCC_PR_ParseType(false, true);
			if (t)
			{
				QCC_PR_Expect(")");
				return QCC_MakeIntConst(t->size * 4);
			}
			else
			{
				int sz;
				int oldstcount = numstatements;
#if 1
				QCC_ref_t refbuf, *r;
				r = QCC_PR_ParseRefValue(&refbuf, pr_classtype, false, false, false);
				if (r->type == REF_ARRAYHEAD && !r->index.cast)
				{
					e = r->base;
					sz = e.sym->arraysize;
				}
				else
					sz = 1;
				sz *= r->cast->size;
				QCC_FreeTemp(r->base);
				if (r->index.cast)
					QCC_FreeTemp(r->index);
#else
				e = QCC_PR_ParseValue(pr_classtype, false, true, false);
				if (!e)
					QCC_PR_ParseErrorPrintSRef (ERR_NOTAFUNCTION, func, "sizeof term not supported");
				if (!e->arraysize)
					sz = 1;
				else
					sz = e->arraysize;
				sz *= e->type->size;
				QCC_FreeTemp(e);
#endif
				//the term should not have side effects, or generate any actual statements.
				numstatements = oldstcount;
				QCC_PR_Expect(")");
				sz *= 4;	//4 bytes per word
				return QCC_MakeIntConst(sz);
			}
		}
		if (!strcmp(funcname, "_"))
		{
			if (!func.sym->initialized)
				func.sym->initialized = 3;
			func.sym->referenced = true;
			QCC_FreeTemp(func);
			if (pr_token_type == tt_immediate && pr_immediate_type->type == ev_string)
			{
				d = QCC_MakeTranslateStringConst(pr_immediate_string);
				QCC_PR_Lex();
			}
			else
			{
				QCC_PR_ParseErrorPrintSRef (ERR_TYPEMISMATCHPARM, func, "_() intrinsic accepts only a string immediate", 1);
				d = nullsref;
			}
			QCC_PR_Expect(")");
			return d;
		}

		if (!strcmp(funcname, "va_arg") || !strcmp(funcname, "..."))	//second for compat with gmqcc
		{
			QCC_sref_t va_list;
			QCC_sref_t idx;
			QCC_type_t *type;
			va_list = QCC_PR_GetSRef(type_vector, "__va_list", pr_scope, false, 0, 0);
			idx = QCC_PR_Expression (TOP_PRIORITY, EXPR_DISALLOW_COMMA);
			idx = QCC_PR_Statement(&pr_opcodes[OP_MUL_F], idx, QCC_MakeFloatConst(3), NULL);
			QCC_PR_Expect(",");
			type = QCC_PR_ParseType(false, false);
			QCC_PR_Expect(")");
			if (!va_list.cast || !va_list.sym || !va_list.sym->arraysize)
				QCC_PR_ParseError (ERR_TYPEMISMATCHPARM, "va_arg() intrinsic only works inside varadic functions");

			if (!func.sym->initialized)
				func.sym->initialized = 3;
			func.sym->referenced = true;
			QCC_FreeTemp(func);
			return QCC_LoadFromArray(va_list, idx, type, false);
		}
		if (!strcmp(funcname, "random"))
		{
			if (!func.sym->initialized)
				func.sym->initialized = 3;
			func.sym->referenced = true;
			QCC_FreeTemp(func);
			if (!QCC_PR_CheckToken(")"))
			{
				e = QCC_PR_Expression (TOP_PRIORITY, EXPR_DISALLOW_COMMA);
				e = QCC_SupplyConversion(e, ev_float, true);
				if (e.cast->type != ev_float)
					QCC_PR_ParseErrorPrintSRef (ERR_TYPEMISMATCHPARM, func, "type mismatch on parm %i", 1);
				if (!QCC_PR_CheckToken(")"))
				{
					QCC_PR_Expect(",");
					d = QCC_PR_Expression (TOP_PRIORITY, EXPR_DISALLOW_COMMA);
					d = QCC_SupplyConversion(d, ev_float, true);
					if (d.cast->type != ev_float)
						QCC_PR_ParseErrorPrintSRef (ERR_TYPEMISMATCHPARM, func, "type mismatch on parm %i", 2);
					QCC_PR_Expect(")");
				}
				else
					d = nullsref;
			}
			else
			{
				e = nullsref;
				d = nullsref;
			}

			if (QCC_OPCodeValid(&pr_opcodes[OP_RAND0]))
			{
				if(qcc_targetformat != QCF_HEXEN2)
					out = QCC_GetTemp(type_float);
				else
				{	//hexen2 requires the output be def_ret
					QCC_ClobberDef(&def_ret);
					out = nullsref;
				}
				if (e.cast)
				{
					if (d.cast)
					{
						QCC_PR_SimpleStatement(&pr_opcodes[OP_RAND2], e, d, out, false);
						QCC_FreeTemp(d);
					}
					else
						QCC_PR_SimpleStatement(&pr_opcodes[OP_RAND1], e, nullsref, out, false);
					QCC_FreeTemp(e);
				}
				else
					QCC_PR_SimpleStatement(&pr_opcodes[OP_RAND0], nullsref, nullsref, out, false);
				if (!out.cast)
					out = QCC_GetAliasTemp(QCC_MakeSRefForce(&def_ret, 0, type_float));
			}
			else
			{
				QCC_ClobberDef(&def_ret);
				//this is normally a builtin, so don't bother locking temps.
				QCC_PR_SimpleStatement(&pr_opcodes[OP_CALL0], func, nullsref, nullsref, false);
				out = QCC_GetAliasTemp(QCC_MakeSRefForce(&def_ret, 0, type_float));
				if (d.cast)
				{
					QCC_sref_t t;
					//min + (max-min)*random()
					t = QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_F], d, e, NULL, STFL_PRESERVEB);
					out = QCC_PR_Statement(&pr_opcodes[OP_MUL_F], out, t, NULL);
					out = QCC_PR_Statement(&pr_opcodes[OP_ADD_F], out, e, NULL);
				}
				else if (e.cast)
					out = QCC_PR_Statement(&pr_opcodes[OP_MUL_F], out, e, NULL);
			}
			return out;
		}
		if (!strcmp(funcname, "randomv"))
		{
			if (!func.sym->initialized)
				func.sym->initialized = 3;
			func.sym->referenced=true;
			QCC_FreeTemp(func);
			if (!QCC_PR_CheckToken(")"))
			{
				e = QCC_PR_Expression (TOP_PRIORITY, EXPR_DISALLOW_COMMA);
				if (e.cast->type != ev_vector)
					QCC_PR_ParseErrorPrintSRef (ERR_TYPEMISMATCHPARM, func, "type mismatch on parm %i", 1);
				if (!QCC_PR_CheckToken(")"))
				{
					QCC_PR_Expect(",");
					d = QCC_PR_Expression (TOP_PRIORITY, EXPR_DISALLOW_COMMA);
					if (d.cast->type != ev_vector)
						QCC_PR_ParseErrorPrintSRef (ERR_TYPEMISMATCHPARM, func, "type mismatch on parm %i", 2);
					QCC_PR_Expect(")");
				}
				else
					d = nullsref;
			}
			else
			{
				e = nullsref;
				d = nullsref;
			}

			if (QCC_OPCodeValid(&pr_opcodes[OP_RANDV0]))
			{
				if(qcc_targetformat != QCF_HEXEN2)
					out = QCC_GetTemp(type_vector);
				else
				{	//hexen2 requires the output be def_ret
					QCC_ClobberDef(&def_ret);
					out = nullsref;
				}
				if (e.cast)
				{
					if (d.cast)
					{
						QCC_PR_SimpleStatement(&pr_opcodes[OP_RANDV2], e, d, out, false);
						QCC_FreeTemp(d);
					}
					else
						QCC_PR_SimpleStatement(&pr_opcodes[OP_RANDV1], e, nullsref, out, false);
					QCC_FreeTemp(e);
				}
				else
					QCC_PR_SimpleStatement(&pr_opcodes[OP_RANDV0], nullsref, nullsref, out, false);
				if (!out.cast)
					out = QCC_GetAliasTemp(QCC_MakeSRefForce(&def_ret, 0, type_vector));
			}
			else
			{
				QCC_sref_t x,y,z;
				QCC_sref_t min = nullsref;
				QCC_sref_t scale = nullsref;
				if (d.cast)
				{
					min = e;
					scale = QCC_PR_StatementFlags(&pr_opcodes[OP_SUB_V], d, min, NULL, STFL_PRESERVEB);
				}
				else if (e.cast)
					scale = e;
				QCC_ClobberDef(&def_ret);
				out = QCC_GetAliasTemp(QCC_MakeSRefForce(&def_ret, 0, type_vector));
				x = out;
				x.cast = type_float;
				y = x;
				y.ofs += 1;
				z = y;
				z.ofs += 1;
				QCC_PR_SimpleStatement(&pr_opcodes[OP_CALL0], func, nullsref, nullsref, false);
				if (scale.cast)
				{
					scale.cast = type_float;
					scale.ofs += 2;
					QCC_PR_SimpleStatement(&pr_opcodes[OP_MUL_F], x, scale, z, false);
					scale.ofs--;
				}
				else
					QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_F], x, z, nullsref, false);
				QCC_PR_SimpleStatement(&pr_opcodes[OP_CALL0], func, nullsref, nullsref, false);
				if (scale.cast)
				{
					QCC_PR_SimpleStatement(&pr_opcodes[OP_MUL_F], x, scale, y, false);
					scale.ofs--;
				}
				else
					QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_F], x, y, nullsref, false);
				QCC_PR_SimpleStatement(&pr_opcodes[OP_CALL0], func, nullsref, nullsref, false);
				if (scale.cast)
				{
					QCC_PR_SimpleStatement(&pr_opcodes[OP_MUL_F], x, scale, x, false);
					scale.ofs--;
					scale.cast = type_vector;
					QCC_FreeTemp(scale);
				}
				if (min.cast)
					out = QCC_PR_Statement(&pr_opcodes[OP_ADD_V], out, min, NULL);
			}

			return out;
		}
		else if (!strcmp(funcname, "spawn"))
		{
			QCC_sref_t result;
			QCC_type_t *rettype;
			
			/*
			ret = spawn();
			ret.FOO* = FOO*;
			result.(classcall)spawnfunc_foo();
			return result;
			this mechanism means entities can be spawned easily via maps.
			*/

			if (!QCC_PR_CheckToken(")"))
			{
				char *nam = QCC_PR_ParseName();
				rettype = QCC_TypeForName(nam);
				if (!rettype || rettype->type != ev_entity)
					QCC_PR_ParseError(ERR_NOTANAME, "Spawn operator with undefined class: %s", nam);
			}
			else
				rettype = NULL;	//default, corrected to entity later

			//ret = spawn()
			result = QCC_PR_GenerateFunctionCall(nullsref, func, NULL, NULL, 0);

			if (rettype)
			{
				char genfunc[256];
				//do field assignments.
				while(QCC_PR_CheckToken(","))
				{
					QCC_sref_t f, p, v;
					f = QCC_PR_ParseValue(rettype, false, false, true);
					if (f.cast->type != ev_field)
						QCC_PR_ParseError(0, "Named field is not a field.");
					if (QCC_PR_CheckToken("="))							//allow : or = as a separator, but throw a warning for =
						QCC_PR_ParseWarning(0, "That = should be a :");	//rejecting = helps avoid qcc bugs. :P
					else
						QCC_PR_Expect(":");
					v = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);

					p = QCC_PR_StatementFlags(&pr_opcodes[OP_ADDRESS], result, f, NULL, STFL_PRESERVEA);
					if (v.cast->size == 3)
						QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_STOREP_V], v, p, NULL));
					else
						QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_STOREP_F], v, p, NULL));
				}
				QCC_PR_Expect(")");

				QC_snprintfz(genfunc, sizeof(genfunc), "spawnfunc_%s", rettype->name);
				func = QCC_PR_GetSRef(type_function, genfunc, NULL, true, 0, GDF_CONST);
				func.sym->referenced = true;

				QCC_UnFreeTemp(result);
				QCC_FreeTemp(QCC_PR_GenerateFunctionCall(result, func, NULL, NULL, 0));
				result.cast = rettype;
			}

			return result;
		}
		else if (!strcmp(funcname, "entnum") && !QCC_PR_CheckToken(")"))
		{
			//t = (a/%1) / (nextent(world)/%1)
			//a/%1 does a (int)entity to float conversion type thing
			if (!func.sym->initialized)
				func.sym->initialized = 3;
			func.sym->referenced = true;
			QCC_FreeTemp(func);

			e = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
			QCC_PR_Expect(")");
			e = QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_F], e, QCC_MakeIntConst(1), NULL, 0);
			d = QCC_PR_GetSRef(NULL, "nextent", NULL, false, 0, false);
			if (!d.cast)
				QCC_PR_ParseError(0, "the nextent builtin is not defined");
			QCC_UnFreeTemp(e);
			d = QCC_PR_GenerateFunctionCall (nullsref, d, &e, &type_float, 1);
			d = QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_F], d, QCC_MakeIntConst(1), NULL, 0);
			e = QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_F], e, d, NULL, 0);

			return e;
		}
	}	//so it's not an intrinsic.

	if (opt_precache_file)	//should we strip out all precache_file calls?
	{
		if (!newself.cast && !strncmp(funcname,"precache_file", 13))
		{
			if (pr_token_type == tt_immediate && pr_immediate_type->type == ev_string && pr_scope && !strcmp(pr_scope->name, "main"))
			{
				optres_precache_file += strlen(pr_immediate_string);
				QCC_PR_Lex();
				QCC_PR_Expect(")");
				QCC_PrecacheFile (pr_immediate_string, funcname[13]);
				QCC_FreeTemp(func);
				QCC_FreeTemp(newself);
				return QCC_MakeFloatConst(0);
			}
		}
	}

// copy the arguments to the global parameter variables
	arg = 0;
	if (t->type == ev_variant)
	{
		extraparms = true;
		np = 0;
	}
	else if (t->vargs)
	{
		extraparms = true;
		np = t->num_parms;
	}
	else
		np = t->num_parms;

	//any temps referenced to build the parameters don't need to be locked.
	if (!QCC_PR_CheckToken(")"))
	{
		do
		{
			if (arg >= t->num_parms)
				p = NULL;
			else
				p = t->params[arg].type;

			if (arg >= MAX_PARMS+MAX_EXTRA_PARMS)
				QCC_PR_ParseErrorPrintSRef (ERR_TOOMANYTOTALPARAMETERS, func, "More than %i parameters", MAX_PARMS+MAX_EXTRA_PARMS);
			else if (extraparms && arg >= MAX_PARMS && !t->vargcount)
			{
				//vararg builtins cannot accept more than 8 args. they can't tell if they got more, and wouldn't know where to read them.
				QCC_PR_ParseWarning (WARN_TOOMANYPARAMETERSVARARGS, "More than %i parameters on varargs function", MAX_PARMS);
				QCC_PR_ParsePrintSRef(WARN_TOOMANYPARAMETERSVARARGS, func);
			}
			else if (!extraparms && arg >= t->num_parms && !p)
			{
				QCC_PR_ParseWarning (WARN_TOOMANYPARAMETERSFORFUNC, "too many parameters on call to %s", funcname);
				QCC_PR_ParsePrintSRef(WARN_TOOMANYPARAMETERSFORFUNC, func);
			}


			//with vectorcalls, we store the vector into the args as individual floats
			//this allows better reuse of vector constants.
			//copy it into the offset now, because we can.
			if (opt_vectorcalls && pr_token_type == tt_immediate && pr_immediate_type == type_vector && arg < MAX_PARMS)
			{
				e.sym = &def_parms[arg];
				e.cast = type_float;
				e.ofs = 0;
				QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[OP_STORE_F], QCC_MakeFloatConst(pr_immediate.vector[0]), e, NULL, 0));
				e.ofs = 1;
				QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[OP_STORE_F], QCC_MakeFloatConst(pr_immediate.vector[1]), e, NULL, 0));
				e.ofs = 2;
				QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[OP_STORE_F], QCC_MakeFloatConst(pr_immediate.vector[2]), e, NULL, 0));

				e.ofs = 0;
				e.cast = type_vector;

				QCC_PR_Lex();
			}
			else
				e = QCC_PR_Expression (TOP_PRIORITY, EXPR_DISALLOW_COMMA);

			if (arg == 0 && func.sym->name)
			{
				const char *fname = QCC_GetSRefName(func);
			// save information for model and sound caching
				if (!strncmp(fname,"precache_", 9) && e.cast->type == ev_string && e.sym->constant && !e.sym->temp)
				{
					const char *value = &strings[e.sym->symboldata[e.ofs].string];
					if (!strncmp(fname+9,"sound", 5))
						QCC_PrecacheSound (value, fname[14]);
					else if (!strncmp(fname+9,"model", 5))
						QCC_PrecacheModel (value, fname[14]);
					else if (!strncmp(fname+9,"texture", 7))
						QCC_PrecacheTexture (value, fname[16]);
					else if (!strncmp(fname+9,"file", 4))
						QCC_PrecacheFile (value, fname[13]);
				}
			}

			if (p)
			{
				if (typecmp(e.cast, p))
				/*if (e->type->type != ev_integer && p->type != ev_function)
				if (e->type->type != ev_function && p->type != ev_integer)
				if ( e->type->type != p->type )*/
				{
					if (p->type == ev_integer && e.cast->type == ev_float)	//convert float -> int... is this a constant?
						e = QCC_PR_Statement(pr_opcodes+OP_CONV_FTOI, e, nullsref, NULL);
					else if (p->type == ev_float && e.cast->type == ev_integer)	//convert float -> int... is this a constant?
						e = QCC_PR_Statement(pr_opcodes+OP_CONV_ITOF, e, nullsref, NULL);
					else if ((p->type == ev_function || p->type == ev_field || p->type == ev_string || p->type == ev_pointer || p->type == ev_entity) && QCC_SRef_IsNull(e))
					{	//you're allowed to use int 0 to pass a null function/field/string/pointer/entity
						//this is basically because __NULL__ is defined as 0i (int 0)
						//WARNING: field 0 is actually a valid field, and is commonly modelindex.
					}
					else if ((p->type == ev_field || p->type == ev_pointer) && e.cast->type == p->type && (p->aux_type->type == ev_variant || e.cast->aux_type->type == ev_variant || p->aux_type->type == ev_void || e.cast->aux_type->type == ev_void))
					{	//allow passing variant fields etc (also allow .void or *void as universal/variant field/pointer types)
					}
					else if (	(p->type == ev_accessor && p->parentclass->type == e.cast->type)
							||  (e.cast->type == ev_accessor && e.cast->parentclass->type == p->type))
					{
					}
					else if ((p->type == ev_vector) && QCC_SRef_IsNull(e))
					{
						//also allow it for vector types too, but make sure the entire vector is valid.
						e = QCC_MakeVectorConst(0, 0, 0);
					}
					else if (p->type != ev_variant && e.cast->type != ev_variant)	//can cast to variant whatever happens
					{
						QCC_type_t *inh;
						for (inh = e.cast->parentclass; inh; inh = inh->parentclass)
						{
							if (!typecmp(inh, p))
								break;
						}
						if (!inh)
						{
							char typebuf1[1024];
							char typebuf2[1024];
							if (flag_laxcasts || (p->type == ev_function && e.cast->type == ev_function))
							{

								QCC_PR_ParseWarning(WARN_LAXCAST, "type mismatch on parm %i: %s should be %s", arg+1, TypeName(e.cast, typebuf1, sizeof(typebuf1)), TypeName(p, typebuf2, sizeof(typebuf2)));
								QCC_PR_ParsePrintSRef(WARN_LAXCAST, func);
							}
							else
							{
								QCC_PR_ParseWarning (ERR_TYPEMISMATCHPARM, "type mismatch on parm %i: %s should be %s", arg+1, TypeName(e.cast, typebuf1, sizeof(typebuf1)), TypeName(p, typebuf2, sizeof(typebuf2)));
								QCC_PR_ParsePrintSRef(ERR_TYPEMISMATCHPARM, func);
							}
						}
					}
					p = e.cast;
				}
			}

			if (arg == 1 && !STRCMP(QCC_GetSRefName(func), "setmodel") && e.cast->type == ev_string && e.sym->constant && !e.sym->temp)
			{
				const char *value = &strings[e.sym->symboldata[e.ofs].string];
				QCC_SetModel(value);
			}

			param[arg] = e;
			paramtypes[arg] = p;
			arg++;
		} while (QCC_PR_CheckToken (","));
		QCC_PR_Expect (")");
	}

	//don't warn if we omited optional arguments
	while (np > arg && func.cast->params[np-1].optional)
		np--;
	if (arg < np)
	{
		if (arg+1==np && !strcmp(QCC_GetSRefName(func), "makestatic"))
		{
			//vanilla QC sucks. I want fteextensions.qc to compile with vanilla, yet also result in errors for when the mod fucks up.
			QCC_PR_ParseWarning (WARN_BADPARAMS, "too few parameters on call to %s. Passing 'self'.", QCC_GetSRefName(func));
			QCC_PR_ParsePrintSRef (WARN_BADPARAMS, func);

			param[arg] = QCC_PR_GetSRef(NULL, "self", NULL, 0, 0, false);
			paramtypes[arg] = param[arg].cast;
		}
		else if (arg+1==np && !strcmp(QCC_GetSRefName(func), "ai_charge"))
		{
			//vanilla QC sucks. I want fteextensions.qc to compile with vanilla, yet also result in errors for when the mod fucks up.
			QCC_PR_ParseWarning (WARN_BADPARAMS, "too few parameters on call to %s. Passing 0.", QCC_GetSRefName(func));
			QCC_PR_ParsePrintSRef (WARN_BADPARAMS, func);

			param[arg] = QCC_MakeFloatConst(0);
			paramtypes[arg] = param[arg].cast;
		}
		else
		{
			QCC_PR_ParseWarning (WARN_TOOFEWPARAMS, "too few parameters on call to %s", QCC_GetSRefName(func));
			QCC_PR_ParsePrintSRef (WARN_TOOFEWPARAMS, func);
		}
	}

	return QCC_PR_GenerateFunctionCall(newself, func, param, paramtypes, arg);
}

//returns a usable sref_t, always increases the def's refcount even if the def is not live yet. should only be used when a term is created/named.
//this special distinction allows temp reuse to be caught/debugged more reliably.
QCC_sref_t QCC_MakeSRefForce(QCC_def_t *def, unsigned int ofs, QCC_type_t *type)
{
	QCC_sref_t sr;
	sr.sym = def;
	sr.ofs = ofs;
	sr.cast = type;
	if (def)
		QCC_ForceUnFreeDef(def);
	return sr;
}
//makes a sref from a def+ofs+type. also increases refcount. considered an error if the specified def is not currently live.
QCC_sref_t QCC_MakeSRef(QCC_def_t *def, unsigned int ofs, QCC_type_t *type)
{
	QCC_sref_t sr;
	sr.sym = def;
	sr.ofs = ofs;
	sr.cast = type;
	if (def)
		QCC_UnFreeTemp(sr);
	return sr;
}
int constchecks;
int varchecks;
int typechecks;
extern hashtable_t floatconstdefstable;
QCC_sref_t QCC_MakeIntConst(int value)
{
	QCC_def_t	*cn;

	cn = Hash_GetKey(&floatconstdefstable, value);
	if (cn)
		return QCC_MakeSRefForce(cn, 0, type_integer);

//FIXME: union with float consts?
// check for a constant with the same value
	/*for (cn=pr.def_head.next ; cn ; cn=cn->next)
	{
		varchecks++;
		if (!cn->initialized)
			continue;
		if (!cn->constant)
			continue;
		constchecks++;
		if (cn->type != type_integer)
			continue;
		typechecks++;

		if ( cn->symboldata[cn->ofs]._int == value )
		{
			return QCC_MakeSRefForce(cn, 0, type_integer);
		}
	}*/

// allocate a new one
	cn = (void *)qccHunkAlloc (sizeof(QCC_def_t) + sizeof(int));
	cn->next = NULL;
	pr.def_tail->next = cn;
	pr.def_tail = cn;

	cn->type = type_integer;
	cn->name = "IMMEDIATE";
	cn->constant = true;
	cn->initialized = 1;
	cn->scope = NULL;		// always share immediates
	cn->arraysize = 0;

	cn->ofs = 0;
	cn->symbolheader = cn;
	cn->symbolsize = cn->type->size;
	cn->symboldata = (QCC_eval_t*)(cn+1);

	cn->symboldata[0]._int = value;

	Hash_AddKey(&floatconstdefstable, value, cn, qccHunkAlloc(sizeof(bucket_t)));

	return QCC_MakeSRefForce(cn, 0, type_integer);
}

QCC_sref_t QCC_MakeVectorConst(float a, float b, float c)
{
	QCC_def_t	*cn;

// check for a constant with the same value
	for (cn=pr.def_head.next ; cn ; cn=cn->next)
	{
		varchecks++;
		if (!cn->initialized)
			continue;
		if (!cn->constant)
			continue;
		constchecks++;
		if (cn->type != type_vector)
			continue;
		typechecks++;

		if (cn->symboldata[0].vector[0] == a &&
			cn->symboldata[0].vector[1] == b &&
			cn->symboldata[0].vector[2] == c)
		{
			return QCC_MakeSRefForce(cn, 0, type_vector);
		}
	}

// allocate a new one
	cn = (void *)qccHunkAlloc (sizeof(QCC_def_t)+sizeof(float)*3);
	cn->next = NULL;
	pr.def_tail->next = cn;
	pr.def_tail = cn;

	cn->type = type_vector;
	cn->name = "IMMEDIATE";
	cn->constant = true;
	cn->initialized = 1;
	cn->scope = NULL;		// always share immediates
	cn->arraysize = 0;

// copy the immediate to the global area
	cn->ofs = 0;
	cn->symbolheader = cn;
	cn->symbolsize = cn->type->size;
	cn->symboldata = (QCC_eval_t*)(cn+1);

	cn->symboldata[0].vector[0] = a;
	cn->symboldata[0].vector[1] = b;
	cn->symboldata[0].vector[2] = c;

	return QCC_MakeSRefForce(cn, 0, type_vector);
}

extern hashtable_t floatconstdefstable;
QCC_sref_t QCC_MakeFloatConst(float value)
{
	QCC_def_t	*cn;

	union {
		float f;
		int i;
	} fi;

	fi.f = value;

	cn = Hash_GetKey(&floatconstdefstable, fi.i);
	if (cn)
		return QCC_MakeSRefForce(cn, 0, type_float);

// allocate a new one
	cn = (void *)qccHunkAlloc (sizeof(QCC_def_t) + sizeof(float));
	cn->next = NULL;
	pr.def_tail->next = cn;
	pr.def_tail = cn;

//	cn->s_file = s_file;
//	cn->s_line = pr_source_line;
	cn->type = type_float;
	cn->name = "IMMEDIATE";
	cn->constant = true;
	cn->initialized = 1;
	cn->scope = NULL;		// always share immediates
	cn->arraysize = 0;

	cn->ofs = 0;
	cn->symbolheader = cn;
	cn->symbolsize = cn->type->size;
	cn->symboldata = (QCC_eval_t*)(cn+1);

	Hash_AddKey(&floatconstdefstable, fi.i, cn, qccHunkAlloc(sizeof(bucket_t)));

	cn->symboldata[0]._float = value;

	return QCC_MakeSRefForce(cn, 0, type_float);
}

extern hashtable_t stringconstdefstable, stringconstdefstable_trans;
int dotranslate_count;
static QCC_sref_t QCC_MakeStringConstInternal(char *value, pbool translate)
{
	QCC_def_t	*cn;
	int string;

	cn = pHash_Get(translate?&stringconstdefstable_trans:&stringconstdefstable, value);
	if (cn)
		return QCC_MakeSRefForce(cn, 0, type_string);

// allocate a new one
	if(translate)
	{
		char buf[64];
		QC_snprintfz(buf, sizeof(buf), "dotranslate_%i", ++dotranslate_count);
		cn = (void *)qccHunkAlloc (sizeof(QCC_def_t)+sizeof(string_t) + strlen(buf)+1);
		cn->name = (char*)((QCC_eval_t*)(cn+1)+1);
		strcpy(cn->name, buf);
		cn->used = true;	//
		cn->referenced = true;
	}
	else
	{
		cn = (void *)qccHunkAlloc (sizeof(QCC_def_t)+sizeof(string_t));
		cn->name = "IMMEDIATE";
	}
	cn->next = NULL;
	pr.def_tail->next = cn;
	pr.def_tail = cn;

	cn->type = type_string;
	cn->constant = !translate;
	cn->initialized = 1;
	cn->scope = NULL;		// always share immediates
	cn->arraysize = 0;
	cn->localscope = false;

// copy the immediate to the global area
	cn->ofs = 0;
	cn->symbolheader = cn;
	cn->symbolsize = cn->type->size;
	cn->symboldata = (QCC_eval_t*)(cn+1);

	string = QCC_CopyString (value);

	pHash_Add(translate?&stringconstdefstable_trans:&stringconstdefstable, strings+string, cn, qccHunkAlloc(sizeof(bucket_t)));

	cn->symboldata[0].string = string;

	return QCC_MakeSRefForce(cn, 0, type_string);
}

QCC_sref_t QCC_MakeStringConst(char *value)
{
	return QCC_MakeStringConstInternal(value, false);
}
QCC_sref_t QCC_MakeTranslateStringConst(char *value)
{
	return QCC_MakeStringConstInternal(value, true);
}

QCC_type_t *QCC_PointerTypeTo(QCC_type_t *type)
{
	QCC_type_t *newtype;
	newtype = QCC_PR_NewType("ptr", ev_pointer, false);
	newtype->aux_type = type;
	return newtype;
}

char *basictypenames[] = {
	"void",
	"string",
	"float",
	"vector",
	"entity",
	"field",
	"function",
	"pointer",
	"integer",
	"variant",
	"struct",
	"union",
	"accessor"
};

QCC_type_t **basictypes[] =
{
	&type_void,
	&type_string,
	&type_float,
	&type_vector,
	&type_entity,
	&type_field,
	&type_function,
	&type_pointer,
	&type_integer,
	&type_variant,
	NULL,	//type_struct
	NULL,	//type_union
};

QCC_def_t *QCC_MemberInParentClass(char *name, QCC_type_t *clas)
{	//if a member exists, return the member field (rather than mapped-to field)
	QCC_def_t *def;
	unsigned int p;
	char membername[2048];

	if (!clas)
	{
		def = QCC_PR_GetDef(NULL, name, NULL, 0, 0, false);
		if (def && def->type->type == ev_field)	//the member existed as a normal entity field.
			return def;
		return NULL;
	}

	for (p = 0; p < clas->num_parms; p++)
	{
		if (strcmp(clas->params[p].paramname, name))
			continue;

		//the parent has it.

		QC_snprintfz(membername, sizeof(membername), "%s::"MEMBERFIELDNAME, clas->name, clas->params[p].paramname);
		def = QCC_PR_GetDef(NULL, membername, NULL, false, 0, false);
		if (def)
			return def;
		break;
	}

	return QCC_MemberInParentClass(name, clas->parentclass);
}

#if 0
//create fields for the types, instanciate the members to the fields.
//we retouch the parents each time to guarentee polymorphism works.
//FIXME: virtual methods will not work properly. Need to trace down to see if a parent already defined it
void QCC_PR_EmitFieldsForMembers(QCC_type_t *clas, int *basictypefield)
{
//we created fields for each class when we defined the actual classes.
//we need to go through each member and match it to the offset of it's parent class, if overloaded, or create a new field if not..

//basictypefield is cleared before we do this
//we emit the parent's fields first (every time), thus ensuring that we don't reuse parent fields on a child class.
	char membername[2048];
	unsigned int p;
	int a;
	unsigned int o;
	QCC_type_t *mt, *ft;
	QCC_def_t *f, *m;
	extern pbool verbose;
	if (clas->parentclass != type_entity)	//parents MUST have all their fields set or inheritance would go crazy.
		QCC_PR_EmitFieldsForMembers(clas->parentclass, basictypefield);

	for (p = 0; p < clas->num_parms; p++)
	{
		mt = clas->params[p].type;
		QC_snprintfz(membername, sizeof(membername), "%s::"MEMBERFIELDNAME, clas->name, clas->params[p].paramname);
		m = QCC_PR_GetDef(NULL, membername, NULL, false, 0, false);

		f = QCC_MemberInParentClass(clas->params[p].paramname, clas->parentclass);
		if (f)
		{
			if (f->type->type != ev_field || typecmp(f->type->aux_type, mt))
			{
				char ct[256];
				char pt[256];
				TypeName(f->type->aux_type, pt, sizeof(pt));
				TypeName(mt, ct, sizeof(ct));
				QCC_PR_Warning(0, NULL, 0, "type mismatch on inheritance of %s::%s. %s vs %s", clas->name, clas->params[p].paramname, ct, pt);
			}
			if (!m)
			{
				basictypefield[mt->type] += 1;
				continue;
			}
			if (m->arraysize)
				QCC_Error(ERR_INTERNAL, "FTEQCC does not support overloaded arrays of members");
			a=0;
			for (o = 0; o < m->type->size; o++)
				((int *)qcc_pr_globals)[o+a*mt->size+m->ofs] = ((int *)qcc_pr_globals)[o+a*mt->size+f->ofs];
			continue;
		}

		//came from parent class instead?
		if (!m)
			QCC_Error(ERR_INTERNAL, "field def missing for class member (%s::%s)", clas->name, clas->params[p].paramname);

		for (a = 0; a < (m->arraysize?m->arraysize:1); a++)
		{
			/*if it was already set, don't go recursive and generate 500 fields for a one-member class that was inheritted from 500 times*/
			if (((int *)qcc_pr_globals)[0+a*mt->size+m->ofs])
			{
				++basictypefield[mt->type];
				continue;
			}

			//we need the type in here so saved games can still work without saving ints as floats. (would be evil)
			ft = QCC_PR_FieldType(*basictypes[mt->type]);
			QC_snprintfz(membername, sizeof(membername), "::%s%i", basictypenames[mt->type], ++basictypefield[mt->type]);
			f = QCC_PR_GetDef(ft, membername, NULL, false, 0, GDF_CONST);
			if (!f)
			{
				//give it a location if this is the first class that uses this fieldspace
				f = QCC_PR_GetDef(ft, membername, NULL, true, 0, GDF_CONST);
				for (o = 0; o < m->type->size; o++)
					((int *)qcc_pr_globals)[o+f->ofs] = pr.size_fields + o;
				pr.size_fields += o;
			}

			for (o = 0; o < m->type->size; o++)
				((int *)qcc_pr_globals)[o+a*mt->size+m->ofs] = ((int *)qcc_pr_globals)[o+f->ofs];

			if (verbose)
				QCC_PR_Note(0, NULL, 0, "%s maps to %s", m->name, f->name);

			f->references++;
		}
	}
}
#endif
void QCC_PR_EmitClassFunctionTable(QCC_type_t *clas, QCC_type_t *childclas, QCC_sref_t ed)
{	//go through clas, do the virtual thing only if the child class does not override.

	char membername[2048];
	QCC_type_t *type;
	QCC_type_t *oc;
	unsigned int p;

	QCC_sref_t point, member;
	QCC_sref_t virt;

	if (clas->parentclass)
		QCC_PR_EmitClassFunctionTable(clas->parentclass, childclas, ed);

	for (p = 0; p < clas->num_parms; p++)
	{
		type = clas->params[p].type;
		for (oc = childclas; oc != clas; oc = oc->parentclass)
		{
			QC_snprintfz(membername, sizeof(membername), "%s::"MEMBERFIELDNAME, oc->name, clas->params[p].paramname);
			if (QCC_PR_GetSRef(NULL, membername, NULL, false, 0, false).cast)
				break;	//a child class overrides.
		}
		if (oc != clas)
			continue;

		if (type->type == ev_function)	//FIXME: inheritance will not install all the member functions.
		{
			member = nullsref;
			for (oc = childclas; oc && !member.cast; oc = oc->parentclass)
			{
				QC_snprintfz(membername, sizeof(membername), "%s::"MEMBERFIELDNAME, oc->name, clas->params[p].paramname);
				member = QCC_PR_GetSRef(NULL, membername, NULL, false, 0, false);
			}
			if (!member.cast)
			{
				QC_snprintfz(membername, sizeof(membername), "%s::"MEMBERFIELDNAME, clas->name, clas->params[p].paramname);
				QCC_PR_Warning(ERR_INTERNAL, NULL, 0, "Member function %s was not defined", membername);
				continue;
			}
			QC_snprintfz(membername, sizeof(membername), "%s::%s", clas->name, clas->params[p].paramname);
			virt = QCC_PR_GetSRef(type, membername, NULL, false, 0, false);
			if (!virt.cast)
			{
				QCC_PR_Warning(0, NULL, 0, "Member function %s was not defined", membername);
				continue;
			}
			point = QCC_PR_StatementFlags(&pr_opcodes[OP_ADDRESS], ed, member, NULL, STFL_PRESERVEA);
			type_pointer->aux_type = virt.cast;
			QCC_PR_Statement(&pr_opcodes[OP_STOREP_FNC], virt, point, NULL);
		}
	}
}

//take all functions in the type, and parent types, and make sure the links all work properly.
void QCC_PR_EmitClassFromFunction(QCC_def_t *scope, QCC_type_t *basetype)
{
	QCC_type_t *parenttype;

	QCC_sref_t ed;
	QCC_sref_t constructor = nullsref;
	int basictypefield[ev_union+1];

//	int func;

	if (numfunctions >= MAX_FUNCTIONS)
		QCC_Error(ERR_INTERNAL, "Too many function defs");

	pr_scope = NULL;
	memset(basictypefield, 0, sizeof(basictypefield));
//	QCC_PR_EmitFieldsForMembers(basetype, basictypefield);


	pr_source_line = pr_token_line_last = scope->s_line;

	pr_scope = QCC_PR_GenerateQCFunction(scope, scope->type);
	//reset the locals chain
	pr.local_head.nextlocal = NULL;
	pr.local_tail = &pr.local_head;

	scope->initialized = true;
	scope->symboldata[scope->ofs].function = pr_scope - functions;

	ed = QCC_PR_GetSRef(type_entity, "self", NULL, true, 0, false);

	{
		QCC_sref_t fclassname = QCC_PR_GetSRef(NULL, "classname", NULL, false, 0, false);
		if (fclassname.cast)
		{
			QCC_sref_t point = QCC_PR_StatementFlags(&pr_opcodes[OP_ADDRESS], ed, fclassname, NULL, STFL_PRESERVEA);
			type_pointer->aux_type = type_string;
			QCC_PR_Statement(&pr_opcodes[OP_STOREP_FNC], QCC_MakeStringConst(basetype->name), point, NULL);
		}
	}

	QCC_PR_EmitClassFunctionTable(basetype, basetype, ed);

	//FIXME: these constructors are called in the wrong order
	constructor = nullsref;
	for (parenttype = basetype; parenttype; parenttype = parenttype->parentclass)
	{
		char membername[2048];
		QC_snprintfz(membername, sizeof(membername), "%s::%s", parenttype->name, parenttype->name);
		constructor = QCC_PR_GetSRef(NULL, membername, NULL, false, 0, false);

		if (constructor.cast)
		{	//self = ent;
//			self = QCC_PR_GetDef(type_entity, "self", NULL, false, 0, false);
//			oself = QCC_PR_GetDef(type_entity, "oself", scope, !constructed, 0, false);
//			if (!constructed)
//			{
//				QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_STORE_ENT], self, oself, NULL));
//				constructed = true;
//			}
			constructor.sym->referenced = true;
//			QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_STORE_ENT], ed, self, NULL));	//return to our old self.
			QCC_PR_SimpleStatement(&pr_opcodes[OP_CALL0], constructor, nullsref, nullsref, false);
			QCC_FreeTemp(constructor);
		}
	}
//	if (constructed)
//		QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_STORE_ENT], oself, self, NULL));

	QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_DONE], nullsref, nullsref, NULL));



	QCC_WriteAsmFunction(pr_scope, pr_scope->code, pr_scope->firstlocal);

	QCC_Marshal_Locals(pr_scope->code, numstatements);
}

static QCC_sref_t QCC_PR_ExpandField(QCC_sref_t ent, QCC_sref_t field, QCC_type_t *fieldtype, unsigned int preserveflags)
{
	QCC_sref_t r;
	if (!fieldtype)
	{
		if (field.cast->type == ev_field)
			fieldtype = field.cast->aux_type;
		else
		{
			if (field.cast->type != ev_variant)
				QCC_PR_ParseErrorPrintSRef(ERR_INTERNAL, field, "QCC_PR_ExpandField: invalid field type");
			fieldtype = type_variant;
		}
	}
	//FIXME: class.staticmember should directly read staticmember instead of trying to dereference
	switch(fieldtype->type)
	{
	default:
		QCC_PR_ParseErrorPrintSRef(ERR_INTERNAL, field, "QCC_PR_ExpandField: invalid field type");
		r = field;
		break;
	case ev_integer:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_I], ent, field, NULL, preserveflags);
		break;
	case ev_pointer:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_P], ent, field, NULL, preserveflags);
		r.cast = fieldtype;
		break;
	case ev_field:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_FLD], ent, field, NULL, preserveflags);
		r.cast = fieldtype;
		break;
	case ev_variant:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_FLD], ent, field, NULL, preserveflags);
		r.cast = fieldtype;
		break;
	case ev_float:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_F], ent, field, NULL, preserveflags);
		break;
	case ev_string:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_S], ent, field, NULL, preserveflags);
		break;
	case ev_vector:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_V], ent, field, NULL, preserveflags);
		break;
	case ev_function:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_FNC], ent, field, NULL, preserveflags);
		r.cast = fieldtype;
		break;
	case ev_entity:
		r = QCC_PR_StatementFlags(&pr_opcodes[OP_LOAD_ENT], ent, field, NULL, preserveflags);
		break;
	}
	return r;
}

/*checks for <DEF>.foo and expands in a class-aware fashion
normally invoked via QCC_PR_ParseArrayPointer
*/
static QCC_ref_t *QCC_PR_ParseField(QCC_ref_t *refbuf, QCC_ref_t *lhs)
{
	QCC_type_t *t;
	t = lhs->cast;
	if (t->type == ev_entity && (QCC_PR_CheckToken(".") || QCC_PR_CheckToken("->")))
	{
		QCC_ref_t *field;
		QCC_ref_t fieldbuf;
		if (QCC_PR_CheckToken("("))
		{
			field = QCC_PR_RefExpression(&fieldbuf, TOP_PRIORITY, 0);
			QCC_PR_Expect(")");
		}
		else
			field = QCC_PR_ParseRefValue(&fieldbuf, t, false, false, true);
		if (field->cast->type == ev_field || field->cast->type == ev_variant)
		{
			//fields are generally always readonly. that refers to the field def itself, rather than products of said field.
			//entities, like 'world' might also be consts. just ignore that fact. the def itself is not assigned, but the fields of said def.
			//the engine may have a problem with this, but the qcc has no way to referenced locations as readonly separately from the def itself.
			lhs = QCC_PR_BuildRef(refbuf, REF_FIELD, QCC_RefToDef(lhs, true), QCC_RefToDef(field, true), (field->cast->type == ev_field)?field->cast->aux_type:type_variant, false);
		}
		else
		{
			if (field->type == REF_GLOBAL && strstr(QCC_GetSRefName(field->base), "::"))
			{
				QCC_sref_t theent = QCC_RefToDef(lhs, true);
				*refbuf = *field;
				refbuf->type = REF_NONVIRTUAL;
				refbuf->index = theent;
				return refbuf;
			}
			if (t->parentclass)
				QCC_PR_ParseError(ERR_INTERNAL, "%s is not a field of class %s", QCC_GetSRefName(QCC_RefToDef(field, false)), t->name);
			else
				QCC_PR_ParseError(ERR_INTERNAL, "%s is not a field", QCC_GetSRefName(QCC_RefToDef(field, false)));
		}

		lhs = QCC_PR_ParseField(refbuf, lhs);

		lhs = QCC_PR_ParseRefArrayPointer (refbuf, lhs, false, false);
	}
	else if (t->accessors && (QCC_PR_CheckToken(".") || QCC_PR_CheckToken("->")))
	{
		QCC_sref_t index = nullsref;
		char *fieldname = QCC_PR_ParseName();
		struct accessor_s *acc;

		for (acc = t->accessors; acc; acc = acc->next)
			if (!strcmp(acc->fieldname, fieldname))
			{
				if (acc->indexertype)
				{
					if (QCC_PR_CheckToken(".") || QCC_PR_CheckToken("->"))
						index = QCC_MakeStringConst(QCC_PR_ParseName());
					else
					{
						QCC_PR_Expect("[");
						index = QCC_PR_Expression (TOP_PRIORITY, 0);
						QCC_PR_Expect("]");
					}
				}
				break;
			}
		if (!acc)
			for (acc = t->accessors; acc; acc = acc->next)
				if (!*acc->fieldname && acc->indexertype)
				{
					index = QCC_MakeStringConst(fieldname);
					break;
				}
		if (!acc)
			QCC_PR_ParseError(ERR_INTERNAL, "%s is not a member of %s", fieldname, t->name);

		lhs = QCC_PR_BuildAccessorRef(refbuf, QCC_RefToDef(lhs, true), index, acc, lhs->readonly);
		lhs = QCC_PR_ParseField(refbuf, lhs);
	}
	return lhs;
}

/*checks for:
<d>[X]
<d>[X].foo
<d>.foo
within types which are a contiguous block, expanding to an array index.

Also calls QCC_PR_ParseField, which does fields too.
*/
QCC_ref_t *QCC_PR_ParseRefArrayPointer (QCC_ref_t *retbuf, QCC_ref_t *r, pbool allowarrayassign, pbool makearraypointers)
{
	QCC_type_t *t;
	QCC_sref_t idx;
	QCC_sref_t tmp;
	pbool allowarray;
	unsigned int arraysize;
	unsigned int rewindpoint = numstatements;
	pbool dereference = false;

	t = r->cast;
	if (r->type == REF_ARRAYHEAD && r->cast == r->base.cast && r->base.sym)
		arraysize = r->base.sym->arraysize;
	else
		arraysize = 0;
	idx = nullsref;

	while(1)
	{
		allowarray = false;
		if (idx.cast)
			allowarray = arraysize>0 ||
						(t->type == ev_vector) ||
						(t->type == ev_field && t->aux_type->type == ev_vector);
		else if (!idx.cast)
		{
			allowarray = arraysize>0 ||
						(t->type == ev_pointer) ||	//we can dereference pointers
						(t->type == ev_string) ||	//strings are effectively pointers
						(t->type == ev_vector) ||	//vectors are mini arrays
						(t->type == ev_field && t->aux_type->type == ev_vector) ||	//as are field vectors
						(!arraysize&&t->accessors);	//custom accessors
		}

		if (allowarray && QCC_PR_CheckToken("["))
		{
			tmp = QCC_PR_Expression (TOP_PRIORITY, 0);
			QCC_PR_Expect("]");

			if (!arraysize && t->accessors)
			{
				struct accessor_s *acc;
				for (acc = t->accessors; acc; acc = acc->next)
					if (!*acc->fieldname)
						break;
				if(acc)
				{
					r = QCC_PR_BuildAccessorRef(retbuf, QCC_RefToDef(r, true), tmp, acc, r->readonly);
					return QCC_PR_ParseRefArrayPointer(retbuf, r, allowarrayassign, makearraypointers);
				}
			}

			/*if its a pointer that got dereferenced, follow the type*/
			if (!idx.cast && t->type == ev_pointer && !arraysize)
				t = t->aux_type;

			if (!idx.cast && r->cast->type == ev_pointer && !arraysize)
			{
				/*no bounds checks on pointer dereferences*/
				dereference = true;
			}
			else if (!idx.cast && r->cast->type == ev_string && !arraysize)
			{
				/*automatic runtime bounds checks on strings, I'm not going to check this too much...*/
				r = QCC_PR_BuildRef(retbuf, REF_STRING, QCC_RefToDef(r, true), tmp, type_float, r->readonly);
				return QCC_PR_ParseRefArrayPointer(retbuf, r, allowarrayassign, makearraypointers);
			}
			else if ((!idx.cast && r->cast->type == ev_vector && !arraysize) || (idx.cast && t->type == ev_vector && !arraysize))
			{
				/*array notation on vector*/
				if (tmp.sym->constant)
				{
					unsigned int i;
					if (tmp.cast->type == ev_integer)
						i = tmp.sym->symboldata[tmp.ofs]._int;
					else if (tmp.cast->type == ev_float)
						i = tmp.sym->symboldata[tmp.ofs]._float;
					else
						i = -1;
					if (i < 0 || i >= 3)
						QCC_PR_ParseErrorPrintSRef(0, r->base, "(vector) array index out of bounds");
				}
				else if (QCC_OPCodeValid(&pr_opcodes[OP_BOUNDCHECK]) && !flag_noboundchecks)
				{
					tmp = QCC_SupplyConversion(tmp, ev_integer, true);
					QCC_PR_SimpleStatement (&pr_opcodes[OP_BOUNDCHECK], tmp, QCC_MakeSRef(NULL, 3, NULL), nullsref, false);
				}
				t = type_float;
			}
			else if ((!idx.cast && r->cast->type == ev_field && r->cast->aux_type->type == ev_vector && !arraysize) || (idx.cast && t->type == ev_field && t->aux_type->type && !arraysize))
			{
				/*array notation on vector field*/
				if (tmp.sym->constant)
				{
					unsigned int i;
					if (tmp.cast->type == ev_integer)
						i = tmp.sym->symboldata[tmp.ofs]._int;
					else if (tmp.cast->type == ev_float)
						i = tmp.sym->symboldata[tmp.ofs]._float;
					else
						i = -1;
					if (i < 0 || i >= 3)
						QCC_PR_ParseErrorPrintSRef(0, r->base, "(vector) array index out of bounds");
				}
				else if (QCC_OPCodeValid(&pr_opcodes[OP_BOUNDCHECK]) && !flag_noboundchecks)
				{
					tmp = QCC_SupplyConversion(tmp, ev_integer, true);
					QCC_PR_SimpleStatement (&pr_opcodes[OP_BOUNDCHECK], tmp, QCC_MakeSRef(NULL, 3, NULL), nullsref, false);
				}
				t = type_floatfield;
			}
			else if (!arraysize)
			{
				QCC_PR_ParseErrorPrintSRef(0, r->base, "array index on non-array");
			}
			else if (tmp.sym->constant)
			{
				unsigned int i;
				if (tmp.cast->type == ev_integer)
					i = tmp.sym->symboldata[tmp.ofs]._int;
				else if (tmp.cast->type == ev_float)
					i = tmp.sym->symboldata[tmp.ofs]._float;
				else
					i = -1;
				if (i < 0 || i >= arraysize)
					QCC_PR_ParseErrorPrintSRef(0, r->base, "(constant) array index out of bounds");
			}
			else
			{
				if (QCC_OPCodeValid(&pr_opcodes[OP_BOUNDCHECK]) && !flag_noboundchecks)
				{
					tmp = QCC_SupplyConversion(tmp, ev_integer, true);
					QCC_PR_SimpleStatement (&pr_opcodes[OP_BOUNDCHECK], tmp, QCC_MakeSRef(NULL, arraysize, NULL), nullsref, false);
				}
			}
			arraysize = 0;

			if (t->size != 1) /*don't multiply by type size if the instruction/emulation will do that instead*/
			{
				if (tmp.cast->type == ev_float)
					tmp = QCC_PR_Statement(&pr_opcodes[OP_MUL_F], tmp, QCC_MakeFloatConst(t->size), NULL);
				else
					tmp = QCC_PR_Statement(&pr_opcodes[OP_MUL_I], tmp, QCC_MakeIntConst(t->size), NULL);
			}

			//legacy opcodes needs to stay using floats even if an int was specified
			if (!QCC_OPCodeValid(&pr_opcodes[OP_ADD_I]))
			{
				if (idx.cast)
					idx = QCC_SupplyConversion(idx, ev_float, true);
				tmp = QCC_SupplyConversion(tmp, ev_float, true);
			}

			/*calc the new index*/
			if (idx.cast && idx.cast->type == ev_float && tmp.cast->type == ev_float)
				idx = QCC_PR_Statement(&pr_opcodes[OP_ADD_F], idx, QCC_SupplyConversion(tmp, ev_float, true), NULL);
			else if (idx.cast)
				idx = QCC_PR_Statement(&pr_opcodes[OP_ADD_I], idx, QCC_SupplyConversion(tmp, ev_integer, true), NULL);
			else
				idx = tmp;
		}
		else if (arraysize && (QCC_PR_CheckToken(".") || QCC_PR_CheckToken("->")))
		{
			//the only field of an array type is the 'length' property.
			//if we calculated offsets etc, discard those statements.
			numstatements = rewindpoint;
			QCC_PR_Expect("length");
			QCC_FreeTemp(r->base);
			QCC_FreeTemp(r->index);
			QCC_FreeTemp(idx);
			return QCC_PR_BuildRef(retbuf, REF_GLOBAL, QCC_MakeIntConst(arraysize), nullsref, type_integer, true);
		}
		else if (((t->type == ev_pointer && !arraysize) || t->type == ev_struct || t->type == ev_union) && (QCC_PR_CheckToken(".") || QCC_PR_CheckToken("->")))
		{
			char *tname;
			unsigned int i;
			if (!idx.cast && t->type == ev_pointer && !arraysize)
			{
				t = t->aux_type;
				dereference = true;
			}
			tname = t->name;

			if (t->type == ev_struct || t->type == ev_union)
			{
				if (!t->size)
					QCC_PR_ParseError(0, "%s was not defined yet", tname);
			}
			else
				QCC_PR_ParseError(0, "indirection in something that is not a struct or union", tname);

			for (i = 0; i < t->num_parms; i++)
			{
				if (QCC_PR_CheckName(t->params[i].paramname))
					break;
			}
			if (i == t->num_parms)
				QCC_PR_ParseError(0, "%s is not a member of %s", pr_token, tname);
			if (!t->params[i].ofs && idx.cast)
				;
			else if (QCC_OPCodeValid(&pr_opcodes[OP_ADD_I]))
			{
				tmp = QCC_MakeIntConst(t->params[i].ofs);
				if (idx.cast)
					idx = QCC_PR_Statement(&pr_opcodes[OP_ADD_I], idx, tmp, NULL);
				else
					idx = tmp;
			}
			else
			{
				tmp = QCC_MakeFloatConst(t->params[i].ofs);
				if (idx.cast)
					idx = QCC_PR_Statement(&pr_opcodes[OP_ADD_F], idx, tmp, NULL);
				else
					idx = tmp;
			}
			arraysize = t->params[i].arraysize;
			t = t->params[i].type;
		}
		else
			break;
	}

	if (idx.cast)
	{
		QCC_sref_t base;
		if (r->type == REF_ARRAYHEAD)
		{
			r->type = REF_ARRAY;
			base = QCC_RefToDef(r, true);
			r->type = REF_ARRAYHEAD;
		}
		else
			base = QCC_RefToDef(r, true);

		//okay, not a pointer, we'll have to read it in somehow
		if (dereference)
		{
			r = QCC_PR_BuildRef(retbuf, REF_POINTER, base, idx, t, r->readonly);
			r = QCC_PR_ParseRefArrayPointer(retbuf, r, allowarrayassign, makearraypointers);
			r = QCC_PR_ParseField(retbuf, r);
			return r;
		}
/*		else if (d->type->type == ev_vector && d->arraysize == 0)
		{	//array notation on vectors (non-field)
			d = QCC_PR_Statement(&pr_opcodes[OP_LOADA_F], d, QCC_SupplyConversion(idx, ev_integer, true), (QCC_statement_t **)0xffffffff);
			d->type = type_float;
		}
*//*	else if (d->type->type == ev_field && d->type->aux_type->type == ev_vector && d->arraysize == 0)
		{	//array notation on vectors (fields)
			d = QCC_PR_Statement(&pr_opcodes[OP_LOADA_FLD], d, QCC_SupplyConversion(idx, ev_integer, true), (QCC_statement_t **)0xffffffff);
			d->type = type_floatfield;
		}
*/		else
		{
			QCC_PR_BuildRef(retbuf, REF_ARRAY, base, idx, t, r->readonly);
		}
		r = retbuf;

		//parse recursively
		r = QCC_PR_ParseRefArrayPointer(retbuf, r, allowarrayassign, makearraypointers);

		if (arraysize && makearraypointers)
		{
			QCC_PR_ParseWarning(0, "Is this still needed?");
			r = QCC_PR_GenerateAddressOf(retbuf, r);
		}
	}

	r = QCC_PR_ParseField(retbuf, r);
	return r;
}

/*
============
PR_ParseValue

Returns the global ofs for the current token
============
*/
QCC_ref_t	*QCC_PR_ParseRefValue (QCC_ref_t *refbuf, QCC_type_t *assumeclass, pbool allowarrayassign, pbool expandmemberfields, pbool makearraypointers)
{
	QCC_sref_t		d;
	QCC_type_t		*t;
	char		*name;

	char membername[2048];

// if the token is an immediate, allocate a constant for it
	if (pr_token_type == tt_immediate)
		return QCC_DefToRef(refbuf, QCC_PR_ParseImmediate ());

	if (QCC_PR_CheckToken("["))
	{
		//originally used for reacc - taking the form of [5 84 2]
		//we redefine it to include statements - [a+b, c, 3+(d*2)]
		//and to not need the 2nd/3rd parts if you're lazy - [5] or [5,6] - FIXME: should we accept 1-d vector? or is that too risky with arrays and weird error messages?
		//note the addition of commas.
		//if we're parsing reacc code, we will still accept [(a+b) c (3+(d*2))], as QCC_PR_Term contains the () handling. We do also allow optional commas.
		QCC_sref_t x,y,z;
		if (flag_acc)
		{
			x = QCC_PR_Term(EXPR_DISALLOW_COMMA);
			QCC_PR_CheckToken(",");
			y = QCC_PR_Term(EXPR_DISALLOW_COMMA);
			QCC_PR_CheckToken(",");
			z = QCC_PR_Term(EXPR_DISALLOW_COMMA);
		}
		else
		{
			x = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);

			if (QCC_PR_CheckToken(","))
				y = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
			else
				y = QCC_MakeFloatConst(0);

			if (QCC_PR_CheckToken(","))
				z = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
			else
				z = QCC_MakeFloatConst(0);
		}

		QCC_PR_Expect("]");

		if ((x.cast->type != ev_float && x.cast->type != ev_integer) ||
			(y.cast->type != ev_float && y.cast->type != ev_integer) ||
			(z.cast->type != ev_float && z.cast->type != ev_integer))
		{
			QCC_PR_ParseError(ERR_TYPEMISMATCH, "Argument not a single numeric value in vector constructor");
			return QCC_DefToRef(refbuf, QCC_MakeVectorConst(0, 0, 0));
		}

		//return a constant if we can.
		if (x.sym->constant && y.sym->constant && z.sym->constant)
		{
			d = QCC_MakeVectorConst(
				(x.cast->type==ev_float)?x.sym->symboldata[x.ofs]._float:x.sym->symboldata[x.ofs]._int,
				(y.cast->type==ev_float)?y.sym->symboldata[y.ofs]._float:y.sym->symboldata[y.ofs]._int,
				(z.cast->type==ev_float)?z.sym->symboldata[z.ofs]._float:z.sym->symboldata[z.ofs]._int);
			QCC_FreeTemp(x);
			QCC_FreeTemp(y);
			QCC_FreeTemp(z);
			return QCC_DefToRef(refbuf, d);
		}

		//pack the variables into a vector
		d = QCC_GetTemp(type_vector);
		d.cast = type_float;
		if (x.cast->type == ev_float)
			x=QCC_PR_StatementFlags(pr_opcodes + OP_STORE_F, x, d, NULL, STFL_PRESERVEB);
		else
			x=QCC_PR_StatementFlags(pr_opcodes+OP_STORE_IF, x, d, NULL, STFL_PRESERVEB);
		QCC_FreeTemp(x);
		d.ofs++;
		if (y.cast->type == ev_float)
			y=QCC_PR_StatementFlags(pr_opcodes + OP_STORE_F, y, d, NULL, STFL_PRESERVEB);
		else
			y=QCC_PR_StatementFlags(pr_opcodes+OP_STORE_IF, y, d, NULL, STFL_PRESERVEB);
		QCC_FreeTemp(y);
		d.ofs++;
		if (z.cast->type == ev_float)
			z=QCC_PR_StatementFlags(pr_opcodes + OP_STORE_F, z, d, NULL, STFL_PRESERVEB);
		else
			z=QCC_PR_StatementFlags(pr_opcodes+OP_STORE_IF, z, d, NULL, STFL_PRESERVEB);
		QCC_FreeTemp(z);
		d.ofs++;
		d.ofs -= 3;
		d.cast = type_vector;

		return QCC_DefToRef(refbuf, d);
	}

	if (QCC_PR_CheckToken("::"))
	{
		assumeclass = NULL;
		expandmemberfields = false;	//::classname is always usable for eg: the find builtin.
	}
	name = QCC_PR_ParseName ();

	//fixme: namespaces should be relative
	if (QCC_PR_CheckToken("::"))
	{
		expandmemberfields = false;	//this::classname should also be available to the find builtin, etc. this won't affect self.classname::member nor classname::staticfunc

		if (assumeclass && !strcmp(name, "super"))
			t = assumeclass->parentclass;
		else if (assumeclass && !strcmp(name, "this"))
			t = assumeclass;
		else
			t = QCC_TypeForName(name);
		if (!t || t->type != ev_entity)
		{
			QCC_PR_ParseError (ERR_NOTATYPE, "Not a class \"%s\"", name);
			d = nullsref;
		}
		else
		{
			QCC_type_t *p;
			char membername[1024];
			name = QCC_PR_ParseName ();
			//walk up the parents if needed, to find one that has that field
			for(d = nullsref, p = t; !d.cast && p; p = p->parentclass)
			{
				//use static functions in preference to virtual functions. kinda needed so you can use super::func...
				QC_snprintfz(membername, sizeof(membername), "%s::%s", p->name, name);
				d = QCC_PR_GetSRef (NULL, membername, pr_scope, false, 0, false);
				if (!d.cast)
				{
					QC_snprintfz(membername, sizeof(membername), "%s::"MEMBERFIELDNAME, p->name, name);
					d = QCC_PR_GetSRef (NULL, membername, pr_scope, false, 0, false);
				}
			}
			if (!d.cast)
			{
				QCC_PR_ParseError (ERR_UNKNOWNVALUE, "Unknown value \"%s::%s\"", t->name, name);
			}
		}
	}
	else
	{
		d = nullsref;
		// 'testvar' becomes 'this::testvar'
		if (assumeclass && assumeclass->parentclass)
		{	//try getting a member.
			QCC_type_t *type;
			for(type = assumeclass; type && !d.cast; type = type->parentclass)
			{
				//look for virtual things
				QC_snprintfz(membername, sizeof(membername), "%s::"MEMBERFIELDNAME, type->name, name);
				d = QCC_PR_GetSRef (NULL, membername, pr_scope, false, 0, false);
			}
			for(type = assumeclass; type && !d.cast; type = type->parentclass)
			{
				//look for non-virtual things (functions: after virtual stuff, because this will find the actual function def too)
				QC_snprintfz(membername, sizeof(membername), "%s::%s", type->name, name);
				d = QCC_PR_GetSRef (NULL, membername, pr_scope, false, 0, false);
			}
		}
		if (!d.cast)
		{
			// look through the defs
			d = QCC_PR_GetSRef (NULL, name, pr_scope, false, 0, false);
		}
	}

	if (!d.cast)
	{
		if (!strcmp(name, "nil"))
			d = QCC_MakeIntConst(0);
		else if (	(!strcmp(name, "randomv"))	||
				(!strcmp(name, "sizeof"))	||
				(!strcmp(name, "entnum"))	||
				(!strcmp(name, "va_arg"))	||
				(!strcmp(name, "..."))		||	//for compat. otherwise wtf?
				(!strcmp(name, "_"))		)	//intrinsics, any old function with no args will do.
		{
			d = QCC_PR_GetSRef (type_function, name, NULL, true, 0, false);
//			d->initialized = 0;
		}
		else if (	(!strcmp(name, "random" ))	)	//intrinsics, any old function with no args will do. returning a float just in case people declare things in the wrong order
		{
			d = QCC_PR_GetSRef (type_floatfunction, name, NULL, true, 0, false);
//			d.sym->initialized = 0;
		}
		else if (keyword_class && !strcmp(name, "this"))
		{
			if (!pr_classtype)
				QCC_PR_ParseError(ERR_NOTANAME, "Cannot use 'this' outside of an OO function\n");
			d = QCC_PR_GetSRef(type_entity, "self", NULL, true, 0, false);
			d.cast = pr_classtype;
		}
		else if (keyword_class && !strcmp(name, "super"))
		{
			if (!assumeclass)
				QCC_PR_ParseError(ERR_NOTANAME, "Cannot use 'super' outside of an OO function\n");
			if (!assumeclass->parentclass)
				QCC_PR_ParseError(ERR_NOTANAME, "class %s has no super\n", pr_classtype->name);
			d = QCC_PR_GetSRef(NULL, "self", NULL, true, 0, false);
			d.cast = assumeclass->parentclass;
		}
		else if (pr_assumetermtype)
		{
			d = QCC_PR_GetSRef (pr_assumetermtype, name, NULL, true, 0, false);
			if (!d.cast)
				QCC_PR_ParseError (ERR_UNKNOWNVALUE, "Unknown value \"%s\"", name);
		}
		else
		{
			d = QCC_PR_GetSRef (type_variant, name, pr_scope, true, 0, false);
			if (!expandmemberfields && assumeclass)
			{
				if (!d.cast)
					QCC_PR_ParseError (ERR_UNKNOWNVALUE, "Unknown field \"%s\" in class \"%s\"", name, assumeclass->name);
				else if (!assumeclass->parentclass && assumeclass != type_entity)
				{
					QCC_PR_ParseWarning (ERR_UNKNOWNVALUE, "Class \"%s\" is not defined, cannot access memeber \"%s\"", assumeclass->name, name);
					if (!autoprototype && !autoprototyped)
						QCC_PR_Note(ERR_UNKNOWNVALUE, strings+s_file, pr_source_line, "Consider using #pragma autoproto");
				}
				else
				{
					QCC_PR_ParseWarning (ERR_UNKNOWNVALUE, "Unknown field \"%s\" in class \"%s\"", name, assumeclass->name);
				}
			}
			else
			{
				if (!d.cast)
					QCC_PR_ParseError (ERR_UNKNOWNVALUE, "Unknown value \"%s\"", name);
				else
				{
					QCC_PR_ParseWarning (ERR_UNKNOWNVALUE, "Unknown value \"%s\".", name);
				}
			}
		}
	}

	d.sym->referenced = true;

	//class code uses self as though it was 'this'. its a hack, but this is QC.
	if (assumeclass && pr_classtype && !strcmp(name, "self"))
	{
		//use 'this' instead.
		QCC_sref_t t = QCC_PR_GetSRef(NULL, "this", pr_scope, false, 0, false);
		if (!t.cast)	//shouldn't happen.
		{
			t.sym = QCC_PR_DummyDef(pr_classtype, "this", pr_scope, 0, d.sym, 0, true, GDF_CONST);
			t.cast = pr_classtype;
			t.ofs = 0;
		}
		else
			QCC_FreeTemp(d);
		d = t;
		QCC_PR_ParseWarning (WARN_SELFNOTTHIS, "'self' used inside OO function, use 'this'.", pr_scope->name);
	}

	if (!d.cast)
		QCC_PR_ParseError (ERR_INTERNAL, "d.cast == NULL");

	//within class functions, refering to fields should use them directly.
	if (pr_classtype && expandmemberfields && d.cast->type == ev_field)
	{
		QCC_sref_t t;
		if (assumeclass)
		{
			t = QCC_PR_GetSRef(NULL, "this", pr_scope, false, 0, false);
			if (!t.cast)
			{
				t.sym = QCC_PR_DummyDef(pr_classtype, "this", pr_scope, 0, QCC_PR_GetDef(NULL, "self", NULL, true, 0, false), 0, true, GDF_CONST);	//create a union into it
				t.cast = pr_classtype;
				t.ofs = 0;
			}
		}
		else
			t = QCC_PR_GetSRef(NULL, "self", NULL, true, 0, false);

		d = QCC_PR_ParseArrayPointer(d, allowarrayassign, makearraypointers); //opportunistic vecmember[0] handling

		//then return a reference to this.field
		QCC_PR_BuildRef(refbuf, REF_FIELD, t, d, d.cast->aux_type, false);

		//(a.(foo[4]))[2] should still function, and may be common with field vectors
		return QCC_PR_ParseRefArrayPointer(refbuf, refbuf, allowarrayassign, makearraypointers); //opportunistic vecmember[0] handling
	}

	if (d.sym->arraysize)
	{
		QCC_ref_t *r;
		QCC_DefToRef(refbuf, d);
		refbuf->type = REF_ARRAYHEAD;
		r = QCC_PR_ParseRefArrayPointer(refbuf, refbuf, allowarrayassign, makearraypointers);
		/*if (r->type == REF_ARRAYHEAD)
		{
			r->type = REF_GLOBAL;
			return QCC_PR_GenerateAddressOf(refbuf, r);
		}*/
		return r;
	}

	return QCC_PR_ParseRefArrayPointer(refbuf, QCC_DefToRef(refbuf, d), allowarrayassign, makearraypointers);
}

QCC_sref_t QCC_PR_GenerateLogicalNot(QCC_sref_t e, const char *errormessage)
{
	etype_t	t;
	QCC_type_t *type = e.cast;
	while(type->type == ev_accessor)
		type = type->parentclass;
	t = type->type;
	if (t == ev_float)
		return QCC_PR_Statement (&pr_opcodes[OP_NOT_F], e, nullsref, NULL);
	else if (t == ev_string)
		return QCC_PR_Statement (&pr_opcodes[OP_NOT_S], e, nullsref, NULL);
	else if (t == ev_entity)
		return QCC_PR_Statement (&pr_opcodes[OP_NOT_ENT], e, nullsref, NULL);
	else if (t == ev_vector)
		return QCC_PR_Statement (&pr_opcodes[OP_NOT_V], e, nullsref, NULL);
	else if (t == ev_function)
		return QCC_PR_Statement (&pr_opcodes[OP_NOT_FNC], e, nullsref, NULL);
	else if (t == ev_integer)
		return QCC_PR_Statement (&pr_opcodes[OP_NOT_FNC], e, nullsref, NULL);	//functions are integer values too.
	else if (t == ev_pointer)
		return QCC_PR_Statement (&pr_opcodes[OP_NOT_FNC], e, nullsref, NULL);	//Pointers are too.
	else if (t == ev_void && flag_laxcasts)
	{
		QCC_PR_ParseWarning(WARN_LAXCAST, errormessage, "void");
		return QCC_PR_Statement (&pr_opcodes[OP_NOT_F], e, nullsref, NULL);
	}
	else
	{
		char etype[256];
		TypeName(e.cast, etype, sizeof(etype));

		QCC_PR_ParseError (ERR_BADNOTTYPE, errormessage, etype);
		return nullsref;
	}
}

QCC_sref_t QCC_EvaluateCast(QCC_sref_t src, QCC_type_t *cast, pbool implicit)
{
//casting from an accessor uses the base type of that accessor (this allows us to properly read void* accessors)
	if (src.cast->type == ev_accessor)
		src.cast = src.cast->parentclass;

	/*you may cast from a type to itself*/
	if (!typecmp(src.cast, cast))
	{
		//no-op
	}
	/*you may cast from const 0 to any other basic type for free (from either int or float for simplicity). things get messy when its a struct*/
	else if (QCC_SRef_IsNull(src) && cast->type != ev_struct && cast->type != ev_union)
	{
		QCC_FreeTemp(src);
		if (cast->size == 3)// || cast->type == ev_variant)
			src = QCC_MakeVectorConst(0,0,0);
		else
			src = QCC_MakeIntConst(0);
		src.cast = cast;
	}
	/*cast from int->float will convert*/
	else if (cast->type == ev_float && src.cast->type == ev_integer)
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_ITOF], src, nullsref, NULL);
	/*cast from float->int will convert*/
	else if (cast->type == ev_integer && src.cast->type == ev_float)
		src = QCC_PR_Statement (&pr_opcodes[OP_CONV_FTOI], src, nullsref, NULL);
	else if (cast->type == ev_entity && src.cast->type == ev_entity)
	{
		if (implicit)
		{	//this is safe if the source inherits from the dest type
			//but we should warn if the other way around
			QCC_type_t *t = src.cast;
			while(t)
			{
				if (!typecmp_lax(t, cast))
					break;
				t = t->parentclass;
			}
			if (!t)
			{
				char typea[256];
				char typeb[256];
				TypeName(src.cast, typea, sizeof(typea));
				TypeName(cast, typeb, sizeof(typeb));
				QCC_PR_ParseWarning(0, "Implicit cast from %s to %s\n", typea, typeb);
			}
		}
		src.cast = cast;
	}
	/*variants can be cast from/to anything without warning, even implicitly. FIXME: size issues*/
	else if (cast->type == ev_variant || src.cast->type == ev_variant)
		src.cast = cast;
	/*these casts are fine when explicit*/
	else if (
		/*you may explicitly cast between pointers and ints (strings count as pointers - WARNING: some strings may not be expressable as pointers)*/
		((cast->type == ev_pointer || cast->type == ev_string || cast->type == ev_integer) && (src.cast->type == ev_pointer || src.cast->type == ev_string || src.cast->type == ev_integer))
		//functions can be explicitly cast from one to another
		|| (cast->type == ev_function && src.cast->type == ev_function)
		//ents->ints || ints->ents. WARNING: the integer value of ent types is engine specific.
		|| (cast->type == ev_entity && src.cast->type == ev_integer)
		|| (cast->type == ev_integer && src.cast->type == ev_entity)
		)
	{
		//direct cast
		if (implicit && typecmp_lax(src.cast, cast))
		{
			char typea[256];
			char typeb[256];
			TypeName(src.cast, typea, sizeof(typea));
			TypeName(cast, typeb, sizeof(typeb));
			QCC_PR_ParseWarning(0, "Implicit cast from %s to %s\n", typea, typeb);
		}
		src.cast = cast;
	}
	else
	{
		char typea[256];
		char typeb[256];
		TypeName(src.cast, typea, sizeof(typea));
		TypeName(cast, typeb, sizeof(typeb));
		QCC_PR_ParseError(0, "Cannot cast from %s to %s\n", typea, typeb);
	}
	return src;
}

/*
============
PR_Term
============
*/
QCC_ref_t *QCC_PR_RefTerm (QCC_ref_t *retbuf, unsigned int exprflags)
{
	QCC_ref_t	*r;
	QCC_sref_t	e, e2;
	etype_t	t;
	if (pr_token_type == tt_punct)	//a little extra speed...
	{
		int preinc;
		if (QCC_PR_CheckToken("++"))
			preinc = 1;
		else if (QCC_PR_CheckToken("--"))
			preinc = -1;
		else
			preinc = 0;

		if (preinc)
		{
			QCC_ref_t tmp;
			qcc_usefulstatement=true;
			r = QCC_PR_RefTerm (&tmp, 0);	//parse the term, as if we were going to return it
			if (r->readonly)
				QCC_PR_ParseError(ERR_BADPLUSPLUSOPERATOR, "++ operator on read-only value");
			e = QCC_RefToDef(r, false);	//read it as needed
			if (e.sym->constant)
			{
				QCC_PR_ParseWarning(WARN_ASSIGNMENTTOCONSTANT, "Assignment to constant %s", QCC_GetSRefName(e));
				QCC_PR_ParsePrintSRef(WARN_ASSIGNMENTTOCONSTANT, e);
			}
			if (e.sym->temp)
				QCC_PR_ParseWarning(WARN_ASSIGNMENTTOCONSTANT, "Hey! That's a temp! ++ operators cannot work on temps!");
			switch (r->cast->type)
			{
			case ev_integer:
				e = QCC_PR_Statement(&pr_opcodes[OP_ADD_I], e, QCC_MakeIntConst(preinc), NULL);
				break;
			case ev_pointer:
				e = QCC_PR_Statement(&pr_opcodes[OP_ADD_PIW], e, QCC_MakeIntConst(preinc * e.cast->aux_type->size), NULL);
				break;
			case ev_float:
				e = QCC_PR_Statement(&pr_opcodes[OP_ADD_F], e, QCC_MakeFloatConst(preinc), NULL);
				break;
			default:
				QCC_PR_ParseError(ERR_BADPLUSPLUSOPERATOR, "++ operator on unsupported type");
				break;
			}
			//return the 'result' of the store. its read-only now.
			return QCC_DefToRef(retbuf, QCC_StoreToRef(r, e, true, false));
		}
		if (QCC_PR_CheckToken ("!"))
		{
			e = QCC_PR_Expression (NOT_PRIORITY, EXPR_DISALLOW_COMMA|EXPR_WARN_ABOVE_1);
			e = QCC_PR_GenerateLogicalNot(e, "Type mismatch: !%s");
			return QCC_DefToRef(retbuf, e);
		}

		if (QCC_PR_CheckToken ("~"))
		{
			e = QCC_PR_Expression (NOT_PRIORITY, EXPR_DISALLOW_COMMA|EXPR_WARN_ABOVE_1);
			t = e.cast->type;
			if (t == ev_float)
				e2 = QCC_PR_Statement (&pr_opcodes[OP_BITNOT_F], e, nullsref, NULL);
			else if (t == ev_integer)
				e2 = QCC_PR_Statement (&pr_opcodes[OP_BITNOT_I], e, nullsref, NULL);	//functions are integer values too.
			else
			{
				e2 = nullsref;		// shut up compiler warning;
				QCC_PR_ParseError (ERR_BADNOTTYPE, "type mismatch for binary not");
			}
			return QCC_DefToRef(retbuf, e2);
		}
		if (QCC_PR_CheckToken ("&"))
		{
			r = QCC_PR_RefExpression (retbuf, UNARY_PRIORITY, EXPR_DISALLOW_COMMA);

			return QCC_PR_GenerateAddressOf(retbuf, r);
		}
		if (QCC_PR_CheckToken ("*"))
		{
			e = QCC_PR_Expression (UNARY_PRIORITY, EXPR_DISALLOW_COMMA);
			if (e.cast->type == ev_pointer)	//FIXME: arrays
				return QCC_PR_BuildRef(retbuf, REF_POINTER, e, nullsref, e.cast->aux_type, false);
			else if (e.cast->accessors)
			{
				struct accessor_s *acc;
				for (acc = e.cast->accessors; acc; acc = acc->next)
					if (!strcmp(acc->fieldname, ""))
						return QCC_PR_BuildAccessorRef(retbuf, e, nullsref, acc, e.sym->constant);
			}
			QCC_PR_ParseErrorPrintSRef (ERR_TYPEMISMATCH, e, "Unable to dereference non-pointer type.");
		}
		if (QCC_PR_CheckToken ("-"))
		{
			e = QCC_PR_Expression (UNARY_PRIORITY, EXPR_DISALLOW_COMMA);

			switch(e.cast->type)
			{
			case ev_float:
				e2 = QCC_PR_Statement (&pr_opcodes[OP_SUB_F], QCC_MakeFloatConst(0), e, NULL);
				break;
			case ev_vector:
				e2 = QCC_PR_Statement (&pr_opcodes[OP_SUB_V], QCC_MakeVectorConst(0, 0, 0), e, NULL);
				break;
			case ev_integer:
				e2 = QCC_PR_Statement (&pr_opcodes[OP_SUB_I], QCC_MakeIntConst(0), e, NULL);
				break;
			default:
				QCC_PR_ParseError (ERR_BADNOTTYPE, "type mismatch for -");
				e2 = nullsref;
				break;
			}
			return QCC_DefToRef(retbuf, e2);
		}
		if (QCC_PR_CheckToken ("+"))
		{
			e = QCC_PR_Expression (UNARY_PRIORITY, EXPR_DISALLOW_COMMA);

			switch(e.cast->type)
			{
			case ev_float:
			case ev_vector:
			case ev_integer:
				e2 = e;
				break;
			default:
				QCC_PR_ParseError (ERR_BADNOTTYPE, "type mismatch for +");
				e2 = nullsref;
				break;
			}
			return QCC_DefToRef(retbuf, e2);
		}
		if (QCC_PR_CheckToken ("("))
		{
			QCC_type_t *newtype;
			newtype = QCC_PR_ParseType(false, true);
			if (newtype)
			{
				QCC_PR_Expect (")");
				if (newtype->type == ev_function && pr_token_type == tt_punct && !strcmp(pr_token, "{"))
				{
					//save some state of the parent
					QCC_def_t *firstlocal = pr.local_head.nextlocal;
					QCC_def_t *lastlocal = pr.local_tail;
					QCC_function_t *parent = pr_scope;
					QCC_statement_t *patch;

					//FIXME: make sure gotos/labels/cases/continues/breaks are not broken by this.

					//generate a goto statement around the nested function, so that nothing is hurt.
					e = nullsref;
					e.cast = type_float;
					patch = QCC_Generate_OP_GOTO();
					e = QCC_MakeIntConst(QCC_PR_ParseImmediateStatements (NULL, newtype) - functions);
					e.cast = newtype;
					patch->a.ofs = &statements[numstatements] - patch;

					//make sure parent state is restored properly.
					pr.local_head.nextlocal = firstlocal;
					pr.local_tail = lastlocal;
					pr_scope = parent;
				}
				else if ((newtype->type == ev_struct || newtype->type == ev_union) && pr_token_type == tt_punct && !strcmp(pr_token, "{"))
				{
					//FIXME
					QCC_PR_ParseError(0, "struct immediates are not supported at this time\n");
				}
				else
				{
					//not a single term, so we can cast the result of function calls. just make sure its not too high a priority
					//and yeah, okay, use defs not refs. whatever.
					e = QCC_PR_Expression (UNARY_PRIORITY, EXPR_DISALLOW_COMMA);
					e = QCC_EvaluateCast(e, newtype, false);
				}
				return QCC_DefToRef(retbuf, e);
			}
			else
			{
				pbool oldcond = conditional;
				conditional = conditional?2:0;
				r =	QCC_PR_RefExpression(retbuf, TOP_PRIORITY, 0);
				QCC_PR_Expect (")");
				conditional = oldcond;

//				QCC_PR_ParseArrayPointer(r, true, true);

				r = QCC_PR_ParseRefArrayPointer(retbuf, r, true, true);
			}
			return r;
		}
	}
	return QCC_PR_ParseRefValue (retbuf, pr_classtype, !(exprflags&EXPR_DISALLOW_ARRAYASSIGN), true, true);
}


int QCC_canConv(QCC_sref_t from, etype_t to)
{
	if (from.cast->type == to)
		return 0;

	//triggers a warning. conversion works by using _x
	if (from.cast->type == ev_vector && to == ev_float)
		return 8;

	if (pr_classtype)
	{
		if (from.cast->type == ev_field)
		{
			if (from.cast->aux_type->type == to)
				return 1;
		}
	}

	//somewhat high penalty, ensures the other side is correct
	if (from.cast->type == ev_variant)
		return 3;
	if (to == ev_variant)
		return 3;

/*	if (from->type->type == ev_pointer && from->type->aux_type->type == to)
		return 1;

	if (QCC_ShouldConvert(from.cast, to)>=0)
		return 1;
*/
//	if (from->type->type == ev_integer && to == ev_function)
//		return 1;

	//silently convert 0 to whatever as required.
	if (QCC_SRef_IsNull(from))
		return 2;

	return -100;
}
/*
==============
QCC_PR_RefExpression
==============
*/

QCC_ref_t *QCC_PR_BuildAccessorRef(QCC_ref_t *retbuf, QCC_sref_t base, QCC_sref_t index, struct accessor_s *accessor, pbool readonly)
{
	retbuf->postinc = 0;
	retbuf->type = REF_ACCESSOR;
	retbuf->base = base;
	retbuf->index = index;
	retbuf->accessor = accessor;
	retbuf->cast = accessor->type;
	retbuf->readonly = readonly;
	return retbuf;
}
QCC_ref_t *QCC_PR_BuildRef(QCC_ref_t *retbuf, unsigned int reftype, QCC_sref_t base, QCC_sref_t index, QCC_type_t *cast, pbool	readonly)
{
	retbuf->postinc = 0;
	retbuf->type = reftype;
	retbuf->base = base;
	retbuf->index = index;
	retbuf->cast = cast?cast:base.cast;
	retbuf->readonly = readonly;
	retbuf->accessor = NULL;
	return retbuf;
}
QCC_ref_t *QCC_DefToRef(QCC_ref_t *retbuf, QCC_sref_t def)
{
	return QCC_PR_BuildRef(retbuf,
				REF_GLOBAL,
				def, 
				nullsref,
				def.cast,
				!def.sym || !!def.sym->constant);
}
/*
void QCC_StoreToOffset(int dest, int source, QCC_type_t *type)
{
	//fixme: we should probably handle entire structs or something
	switch(type->type)
	{
	default:
	case ev_float:
		QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F, source, dest, 0, false);
		break;
	case ev_vector:
		QCC_PR_SimpleStatement(OP_STORE_V, source, dest, 0, false);
		break;
	case ev_entity:
		QCC_PR_SimpleStatement(OP_STORE_ENT, source, dest, 0, false);
		break;
	case ev_string:
		QCC_PR_SimpleStatement(OP_STORE_S, source, dest, 0, false);
		break;
	case ev_function:
		QCC_PR_SimpleStatement(OP_STORE_FNC, source, dest, 0, false);
		break;
	case ev_field:
		QCC_PR_SimpleStatement(OP_STORE_FLD, source, dest, 0, false);
		break;
	case ev_integer:
		QCC_PR_SimpleStatement(OP_STORE_I, source, dest, 0, false);
		break;
	case ev_pointer:
		QCC_PR_SimpleStatement(OP_STORE_P, source, dest, 0, false);
		break;
	}
}*/
void QCC_StoreToSRef(QCC_sref_t dest, QCC_sref_t source, QCC_type_t *type, pbool preservesource, pbool preservedest)
{
	unsigned int i;
	int flags = 0;
	if (preservesource)
		flags |= STFL_PRESERVEA;
	if (preservedest)
		flags |= STFL_PRESERVEB;
	//fixme: we should probably handle entire structs or something
	switch(type->type)
	{
	case ev_struct:
	case ev_union:
		//don't bother trying to optimise any temps here, its not likely to happen anyway.
		for (i = 0; i+2 < type->size; i+=3, dest.ofs += 3, source.ofs += 3)
		{
			QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_V], source, dest, nullsref, false);
		}
		for (; i < type->size; i++, dest.ofs++, source.ofs++)
		{
			QCC_PR_SimpleStatement(&pr_opcodes[OP_STORE_F], source, dest, nullsref, false);
		}
		source.ofs -= type->size;
		dest.ofs -= type->size;
		if (!preservesource)
			QCC_FreeTemp(source);
		if (!preservedest)
			QCC_FreeTemp(dest);
		break;
	default:
	case ev_float:
		QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], source, dest, NULL, flags));
		break;
	case ev_vector:
		QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_V], source, dest, NULL, flags));
		break;
	case ev_entity:
		QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_ENT], source, dest, NULL, flags));
		break;
	case ev_string:
		QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_S], source, dest, NULL, flags));
		break;
	case ev_function:
		QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_FNC], source, dest, NULL, flags));
		break;
	case ev_field:
		QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_FLD], source, dest, NULL, flags));
		break;
	case ev_integer:
		QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_I], source, dest, NULL, flags));
		break;
	case ev_pointer:
		QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_P], source, dest, NULL, flags));
		break;
	}
}
//if readable, returns source (or dest if the store was folded), otherwise returns NULL
QCC_sref_t QCC_CollapseStore(QCC_sref_t dest, QCC_sref_t source, QCC_type_t *type, pbool readable, pbool preservedest)
{
	if (opt_assignments && OpAssignsToC(statements[numstatements-1].op) && source.sym == statements[numstatements-1].c.sym && source.ofs == statements[numstatements-1].c.ofs)
	{
		if (source.sym->temp)
		{
			QCC_statement_t *statement = &statements[numstatements-1];
			statement->c.sym = dest.sym;
			statement->c.ofs = dest.ofs;
			dest.sym->referenced = true;

			optres_assignments++;
			QCC_FreeTemp(source);
			if (readable)
				return dest;
			if (!preservedest)
				QCC_FreeTemp(dest);
			return nullsref;
		}
	}
	QCC_StoreToSRef(dest, source, type, readable, preservedest);
	if (readable)
		return source;
	return nullsref;
}
void QCC_StoreToPointer(QCC_sref_t dest, QCC_sref_t source, QCC_type_t *type)
{
	while (type->type == ev_accessor)
		type = type->parentclass;

	//fixme: we should probably handle entire structs or something
	switch(type->type)
	{
	default:
		QCC_PR_ParseErrorPrintSRef(ERR_INTERNAL, dest, "QCC_StoreToPointer doesn't know how to store to that type");
	case ev_float:
		QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_F], source, dest, nullsref, false);
		break;
	case ev_vector:
		QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_V], source, dest, nullsref, false);
		break;
	case ev_entity:
		QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_ENT], source, dest, nullsref, false);
		break;
	case ev_string:
		QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_S], source, dest, nullsref, false);
		break;
	case ev_function:
		QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_FNC], source, dest, nullsref, false);
		break;
	case ev_field:
		QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_FLD], source, dest, nullsref, false);
		break;
	case ev_integer:
		QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_I], source, dest, nullsref, false);
		break;
	case ev_pointer:
		if (!QCC_OPCodeValid(&pr_opcodes[OP_STOREP_P]))
			QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_FLD], source, dest, nullsref, false);
		else
			QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_P], source, dest, nullsref, false);
		break;
	}
}
void QCC_LoadFromPointer(QCC_sref_t dest, QCC_sref_t source, QCC_sref_t idx, QCC_type_t *type)
{
	while (type->type == ev_accessor)
		type = type->parentclass;

	//fixme: we should probably handle entire structs or something
	switch(type->type)
	{
	case ev_float:
		QCC_PR_SimpleStatement (&pr_opcodes[OP_LOADP_F], source, idx, dest, false);
		break;
	case ev_string:
		QCC_PR_SimpleStatement (&pr_opcodes[OP_LOADP_S], source, idx, dest, false);
		break;
	case ev_vector:
		QCC_PR_SimpleStatement (&pr_opcodes[OP_LOADP_V], source, idx, dest, false);
		break;
	case ev_entity:
		QCC_PR_SimpleStatement (&pr_opcodes[OP_LOADP_ENT], source, idx, dest, false);
		break;
	case ev_field:
		QCC_PR_SimpleStatement (&pr_opcodes[OP_LOADP_FLD], source, idx, dest, false);
		break;
	case ev_function:
		QCC_PR_SimpleStatement (&pr_opcodes[OP_LOADP_FNC], source, idx, dest, false);
		break;
	case ev_integer:
		QCC_PR_SimpleStatement (&pr_opcodes[OP_LOADP_I], source, idx, dest, false);
		break;
	default:
		QCC_PR_ParseErrorPrintSRef(ERR_INTERNAL, dest, "QCC_LoadFromPointer doesn't know how to load from that type");
	case ev_pointer:
		if (!QCC_OPCodeValid(&pr_opcodes[OP_LOADP_P]))
			QCC_PR_SimpleStatement (&pr_opcodes[OP_LOADP_I], source, idx, dest, false);
		else
			QCC_PR_SimpleStatement (&pr_opcodes[OP_LOADP_P], source, idx, dest, false);
		break;
	}
}
void QCC_StoreToArray(QCC_sref_t base, QCC_sref_t index, QCC_sref_t source, QCC_type_t *t)
{
	/*if its assigned to, generate a functioncall to do the store*/
	QCC_sref_t args[2], funcretr;

	if (QCC_OPCodeValid(&pr_opcodes[OP_GLOBALADDRESS]))
	{
		QCC_sref_t addr;
		//ptr = &base[index];
		addr = QCC_PR_Statement(&pr_opcodes[OP_GLOBALADDRESS], base, QCC_SupplyConversion(index, ev_integer, true), NULL);
		//*ptr = source
		QCC_StoreToPointer(addr, source, t);
		source.sym->referenced = true;
		QCC_FreeTemp(addr);
		QCC_FreeTemp(source);
	}
	else
	{
		const char *basename = QCC_GetSRefName(base);
		base.sym->referenced = true;
		QCC_FreeTemp(base);

		funcretr = QCC_PR_GetSRef(NULL, qcva("ArraySet*%s", basename), base.sym->scope, false, 0, GDF_CONST|(base.sym->scope?GDF_STATIC:0));
		if (!funcretr.cast)
		{
			QCC_type_t *arraysetfunc = qccHunkAlloc(sizeof(*arraysetfunc));
			struct QCC_typeparam_s *fparms = qccHunkAlloc(sizeof(*fparms)*2);
			arraysetfunc->size = 1;
			arraysetfunc->type = ev_function;
			arraysetfunc->aux_type = type_void;
			arraysetfunc->params = fparms;
			arraysetfunc->num_parms = 2;
			arraysetfunc->name = "ArraySet";
			fparms[0].type = type_float;
			fparms[1].type = base.sym->type;
			funcretr = QCC_PR_GetSRef(arraysetfunc, qcva("ArraySet*%s", basename), base.sym->scope, true, 0, GDF_CONST|(base.sym->scope?GDF_STATIC:0));
			funcretr.sym->generatedfor = base.sym;
		}

		if (source.cast->type != t->type)
			QCC_PR_ParseErrorPrintSRef(ERR_TYPEMISMATCH, base, "Type Mismatch on array assignment");

		if (base.cast->type == ev_vector)
		{
			//FIXME: we may very well have a *3 already, dividing by 3 again is crazy.
			index = QCC_PR_Statement(&pr_opcodes[OP_DIV_F], index, QCC_MakeFloatConst(3), NULL);
		}

		args[0] = QCC_SupplyConversion(index, ev_float, true);
		args[1] = source;
		qcc_usefulstatement=true;
		QCC_FreeTemp(QCC_PR_GenerateFunctionCall(nullsref, funcretr, args, NULL, 2));
	}
}
QCC_sref_t QCC_LoadFromArray(QCC_sref_t base, QCC_sref_t index, QCC_type_t *t, pbool preserve)
{
	int flags;
	int accel;

	//dp-style opcodes take integer indicies, and thus often need type conversions
	//h2-style opcodes take float indicies, but have a built in boundscheck that wrecks havoc with vectors and structs (and thus sucks when the types don't match)

	if (index.cast->type != ev_float || t->type != base.cast->type)
		accel = 2;
	else
		accel = 1;

	if (accel == 2 && !QCC_OPCodeValid(&pr_opcodes[OP_LOADA_F]))
		accel = 1;
	if (accel == 1 && !QCC_OPCodeValid(&pr_opcodes[OP_FETCH_GBL_F]))
		accel = QCC_OPCodeValid(&pr_opcodes[OP_LOADA_F])?2:0;

	if (accel == 2)
	{
		if (index.cast->type == ev_float)
		{
			flags = preserve?STFL_PRESERVEA:0;
			index = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_FTOI], index, nullsref, NULL, preserve?STFL_PRESERVEA:0);
		}
		else
		{
			flags = preserve?STFL_PRESERVEA|STFL_PRESERVEB:0;
			if (index.cast->type != ev_integer)
				QCC_PR_ParseErrorPrintSRef(ERR_TYPEMISMATCH, base, "array index is not a single numeric value");
		}
		switch(t->type)
		{
		case ev_string:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADA_S], base, index, NULL, flags);	//get pointer to precise def.
			break;
		case ev_float:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADA_F], base, index, NULL, flags);	//get pointer to precise def.
			break;
		case ev_vector:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADA_V], base, index, NULL, flags);	//get pointer to precise def.
			break;
		case ev_entity:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADA_ENT], base, index, NULL, flags);	//get pointer to precise def.
			break;
		case ev_field:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADA_FLD], base, index, NULL, flags);	//get pointer to precise def.
			break;
		case ev_function:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADA_FNC], base, index, NULL, flags);	//get pointer to precise def.
			break;
		case ev_pointer:	//no OP_LOADA_P
		case ev_integer:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_LOADA_I], base, index, NULL, flags);	//get pointer to precise def.
			break;
//		case ev_variant:
		case ev_struct:
		case ev_union:
			QCC_PR_ParseErrorPrintSRef(ERR_TYPEMISMATCH, base, "variable is a struct");
			return nullsref;
		default:
			QCC_PR_ParseError(ERR_NOVALIDOPCODES, "Unable to load type... oops.");
			return nullsref;
		}

		base.cast = t;
		return base;
	}
	else if (accel == 1)
	{
		if (!base.sym->arraysize)
			QCC_PR_ParseErrorPrintSRef(ERR_TYPEMISMATCH, base, "array lookup on non-array");

		if (base.sym->temp)
			QCC_PR_ParseErrorPrintSRef(ERR_TYPEMISMATCH, base, "array lookup on a temp");

		if (index.cast->type == ev_integer)
		{
			flags = preserve?STFL_PRESERVEA:0;
			index = QCC_PR_StatementFlags(&pr_opcodes[OP_CONV_ITOF], index, nullsref, NULL, preserve?STFL_PRESERVEA:0);
		}
		else
		{
			flags = preserve?STFL_PRESERVEA|STFL_PRESERVEB:0;
			if (index.cast->type != ev_float)
				QCC_PR_ParseErrorPrintSRef(ERR_TYPEMISMATCH, base, "array index is not a single numeric value");
		}

		/*hexen2 format has opcodes to read arrays (but has no way to write)*/
		switch(t->type)
		{
		case ev_field:
		case ev_pointer:
		case ev_integer:
		case ev_float:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_FETCH_GBL_F], base, index, NULL, flags);	//get pointer to precise def.
			base.cast = t;
			break;
		case ev_vector:
			//hexen2 uses element indicies. we internally use words.
			//words means you can pack vectors into structs without the offset needing to be a multiple of 3.
			//as its floats, I'm going to try using 0/0.33/0.66 just for the luls
			//FIXME: we may very well have a *3 already, dividing by 3 again is crazy.
			index = QCC_PR_StatementFlags(&pr_opcodes[OP_DIV_F], index, QCC_MakeFloatConst(3), NULL, flags&STFL_PRESERVEA);
			flags &= ~STFL_PRESERVEB;
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_FETCH_GBL_V], base, index, NULL, flags);	//get pointer to precise def.
			break;
		case ev_string:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_FETCH_GBL_S], base, index, NULL, flags);	//get pointer to precise def.
			break;
		case ev_entity:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_FETCH_GBL_E], base, index, NULL, flags);	//get pointer to precise def.
			break;
		case ev_function:
			base = QCC_PR_StatementFlags(&pr_opcodes[OP_FETCH_GBL_FNC], base, index, NULL, flags);	//get pointer to precise def.
			break;
		default:
			QCC_PR_ParseError(ERR_NOVALIDOPCODES, "No op available to read array");
			return nullsref;
		}
		base.cast = t;
		return base;
	}
	else
	{
		/*emulate the array access using a function call to do the read for us*/
		QCC_sref_t args[1], funcretr;

		base.sym->referenced = true;

		if (base.cast->type == ev_field && base.sym->constant && !base.sym->initialized && flag_noboundchecks && flag_fasttrackarrays)
		{
			int i;
			//denormalised floats means we could do:
			//return (add_f: base + (mul_f: index*1i))
			//make sure the array has no gaps
			//the initialised thing is to ensure that it doesn't contain random consecutive system fields that might get remapped weirdly by an engine.
			for (i = 1; i < base.sym->arraysize; i++)
			{
				if (base.sym->symboldata[base.ofs+i]._int != base.sym->symboldata[base.ofs+i-1]._int+1)
					break;
			}
			//its contiguous. we'll do this in two instructions.
			if (i == base.sym->arraysize)
			{
				//denormalised floats means we could do:
				//return (add_f: base + (mul_f: index*1i))
				return QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_F], base, QCC_PR_StatementFlags(&pr_opcodes[OP_MUL_F], index, QCC_MakeIntConst(1), NULL, 0), NULL, 0);
			}
		}

		funcretr = QCC_PR_GetSRef(NULL, qcva("ArrayGet*%s", QCC_GetSRefName(base)), base.sym->scope, false, 0, GDF_CONST|(base.sym->scope?GDF_STATIC:0));
		if (!funcretr.cast)
		{
			QCC_type_t *ftype = qccHunkAlloc(sizeof(*ftype));
			struct QCC_typeparam_s *fparms = qccHunkAlloc(sizeof(*fparms)*1);
			ftype->size = 1;
			ftype->type = ev_function;
			ftype->aux_type = base.cast;
			ftype->params = fparms;
			ftype->num_parms = 1;
			ftype->name = "ArrayGet";
			fparms[0].type = type_float;
			funcretr = QCC_PR_GetSRef(ftype, qcva("ArrayGet*%s", QCC_GetSRefName(base)), base.sym->scope, true, 0, GDF_CONST|(base.sym->scope?GDF_STATIC:0));
			funcretr.sym->generatedfor = base.sym;
			if (!funcretr.sym->constant)
				printf("not constant?\n");
		}

		if (preserve)
			QCC_UnFreeTemp(index);
		else
			QCC_FreeTemp(base);

		/*make sure the function type that we're calling exists*/

		if (base.cast->type == ev_vector)
		{
			//FIXME: we may very well have a *3 already, dividing by 3 again is crazy.
			args[0] = QCC_PR_Statement(&pr_opcodes[OP_DIV_F], QCC_SupplyConversion(index, ev_float, true), QCC_MakeFloatConst(3), NULL);
			base = QCC_PR_GenerateFunctionCall(nullsref, funcretr, args, &type_float, 1);
			base.cast = t;
		}
		else
		{
			if (t->size > 1)
			{
				QCC_sref_t r;
				unsigned int i;
				int old_op = opt_assignments;
				base = QCC_GetTemp(t);
				index = QCC_SupplyConversion(index, ev_float, true);

				for (i = 0; i < t->size; i++)
				{
					if (i)
						args[0] = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_F], index, QCC_MakeFloatConst(i), NULL, STFL_PRESERVEA);
					else
					{
						args[0] = index;
						QCC_UnFreeTemp(index);
						opt_assignments = false;
					}
					QCC_UnFreeTemp(funcretr);
					r = QCC_PR_GenerateFunctionCall(nullsref, funcretr, args, &type_float, 1);
					opt_assignments = old_op;
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], r, base, NULL, STFL_PRESERVEB));
					base.ofs++;
				}
				QCC_FreeTemp(funcretr);
				QCC_FreeTemp(index);
				base.ofs -= i;
			}
			else
			{
				args[0] = QCC_SupplyConversion(index, ev_float, true);
				base = QCC_PR_GenerateFunctionCall(nullsref, funcretr, args, &type_float, 1);
			}
			base.cast = t;
		}
	}
	return base;
}

//reads a ref as required
//the result should ALWAYS be freed, even if freetemps is set.
QCC_sref_t QCC_RefToDef(QCC_ref_t *ref, pbool freetemps)
{
	QCC_sref_t tmp = nullsref, idx;
	QCC_sref_t ret = ref->base;
	if (ref->postinc)
	{
		int inc = ref->postinc;
		ref->postinc = 0;
		//read the value, without preventing the store later
		ret = QCC_RefToDef(ref, false);
		//archive off the old value
		tmp = QCC_GetTemp(ret.cast);
		QCC_StoreToSRef(tmp, ret, ret.cast, false, true);
		ret = tmp;
		//update the value
		switch(ref->cast->type)
		{
		case ev_float:
			QCC_StoreToRef(ref, QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_F], ret, QCC_MakeFloatConst(inc), NULL, STFL_PRESERVEA), false, !freetemps);
			break;
		case ev_integer:
			QCC_StoreToRef(ref, QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], ret, QCC_MakeIntConst(inc), NULL, STFL_PRESERVEA), false, !freetemps);
			break;
		case ev_pointer:
			inc *= ref->cast->aux_type->size;
			QCC_StoreToRef(ref, QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_PIW], ret, QCC_MakeIntConst(inc), NULL, STFL_PRESERVEA), false, !freetemps);
			break;
		default:
			QCC_PR_ParseErrorPrintSRef(ERR_INTERNAL, ret, "post increment operator not supported with this type");
			break;
		}
		//hack any following uses of the ref to refer to the temp
		ref->type = REF_GLOBAL;
		ref->base = ret;
		ref->index = nullsref;
		ref->readonly = true;

		return ret;
	}

	switch(ref->type)
	{
	case REF_NONVIRTUAL:
		if (freetemps)
			QCC_FreeTemp(ref->index);
		else
			QCC_UnFreeTemp(ret);
		break;
	case REF_ARRAYHEAD:
		{
			QCC_ref_t buf;
			ref = QCC_PR_GenerateAddressOf(&buf, ref);
			return QCC_RefToDef(ref, freetemps);
		}
		break;
	case REF_GLOBAL:
	case REF_ARRAY:
		if (ref->index.cast)
		{
			//FIXME: this needs to be deprecated
			const QCC_eval_t *idxeval = QCC_SRef_EvalConst(ref->index);
			if (idxeval && (ref->index.cast->type == ev_float || ref->index.cast->type == ev_integer))
			{
				ref->base.sym->referenced = true;
				if (ref->index.sym)
					ref->index.sym->referenced = true;
				if (ref->index.cast->type == ev_float)
				{
					if (idxeval->_float != (float)(int)idxeval->_float)
						QCC_PR_ParseWarning(0, "Array index rounding on %s[%g]", QCC_GetSRefName(ref->base), idxeval->_float);
					ret.ofs += idxeval->_float;
				}
				else
					ret.ofs += idxeval->_int;

				if (freetemps)
					QCC_FreeTemp(ref->index);
				else
					QCC_UnFreeTemp(ret);
			}
			else
			{
				ret = QCC_LoadFromArray(ref->base, ref->index, ref->cast, !freetemps);
			}
		}
		else if (freetemps)
			QCC_FreeTemp(ref->index);
		else
			QCC_UnFreeTemp(ret);
		break;
	case REF_POINTER:
		tmp = QCC_GetTemp(ref->cast);
		if (ref->index.cast)
		{
//			if (!freetemps)
				QCC_UnFreeTemp(ref->index);
			idx = QCC_SupplyConversion(ref->index, ev_integer, true);
		}
		else
			idx = nullsref;
		QCC_LoadFromPointer(tmp, ref->base, idx, ref->cast);
		QCC_FreeTemp(idx);
		if (freetemps)
			QCC_PR_DiscardRef(ref);
		return tmp;
	case REF_FIELD:
		return QCC_PR_ExpandField(ref->base, ref->index, ref->cast, freetemps?0:(STFL_PRESERVEA|STFL_PRESERVEB));
	case REF_STRING:
		return QCC_PR_StatementFlags(&pr_opcodes[OP_LOADP_C], ref->base, ref->index, NULL, freetemps?0:(STFL_PRESERVEA|STFL_PRESERVEB));
	case REF_ACCESSOR:
		if (ref->accessor && ref->accessor->getset_func[0].cast)
		{
			int args = 0;
			QCC_sref_t arg[2];
			if (ref->accessor->getset_isref[0])
			{
				if (ref->base.sym->temp)
					QCC_PR_ParseErrorPrintSRef(ERR_NOFUNC, ref->base, "Accessor %s(get) cannot be used on a temporary accessor reference", ref->accessor?ref->accessor->fieldname:"");
				//there shouldn't really be any need for this, but its problematic if the accessor is a field.
				arg[args++] = QCC_PR_Statement(&pr_opcodes[OP_GLOBALADDRESS], ref->base, nullsref, NULL);
			}
			else
				arg[args++] = ref->base;
			if (ref->accessor->indexertype)
				arg[args++] = ref->index.cast?QCC_SupplyConversion(ref->index, ref->accessor->indexertype->type, true):QCC_MakeIntConst(0);
			QCC_ForceUnFreeDef(ref->accessor->getset_func[0].sym);
			return QCC_PR_GenerateFunctionCall(nullsref, ref->accessor->getset_func[0], arg, NULL, args);
		}
		else
			QCC_PR_ParseErrorPrintSRef(ERR_NOFUNC, ref->base, "Accessor %s has no get function", ref->accessor?ref->accessor->fieldname:"");
		break;
	}
	ret.cast = ref->cast;
	return ret;
}

//return value is the 'source', unless we folded the store and stripped a temp, in which case it'll be the new value at the given location, either way should have the same value as source.
QCC_sref_t QCC_StoreToRef(QCC_ref_t *dest, QCC_sref_t source, pbool readable, pbool preservedest)
{
	QCC_ref_t ptrref;
	if (dest->readonly)
	{
		QCC_PR_ParseWarning(WARN_ASSIGNMENTTOCONSTANT, "Assignment to constant %s", QCC_GetSRefName(dest->base));
		QCC_PR_ParsePrintSRef(WARN_ASSIGNMENTTOCONSTANT, dest->base);
		if (dest->index.cast)
			QCC_PR_ParsePrintSRef(WARN_ASSIGNMENTTOCONSTANT, dest->index);
	}

	if (QCC_SRef_IsNull(source))
	{
		if (dest->cast->type == ev_vector)
		{
			QCC_FreeTemp(source);
			source = QCC_MakeVectorConst(0, 0, 0);
		}
		source.cast = dest->cast;
	}
	else
	{
		QCC_type_t *t = source.cast;
		while(t)
		{
			if (!typecmp_lax(t, dest->cast))
				break;
			t = t->parentclass;
		}
		if (!t && !(source.cast->type == ev_pointer && dest->cast->type == ev_pointer && (source.cast->aux_type->type == ev_void || source.cast->aux_type->type == ev_variant)))
		{	//extra check to allow void*->any*
			char typea[256];
			char typeb[256];
			if ((dest->cast->type == ev_float || dest->cast->type == ev_integer) && (source.cast->type == ev_float || source.cast->type == ev_integer))
				source = QCC_SupplyConversion(source, dest->cast->type, true);
			else
			{
				TypeName(source.cast, typea, sizeof(typea));
				TypeName(dest->cast, typeb, sizeof(typeb));
				if (dest->type == REF_FIELD)
					QCC_PR_ParseWarning(WARN_STRICTTYPEMISMATCH, "type mismatch: %s %s to %s %s.%s", typea, QCC_GetSRefName(source), typeb, QCC_GetSRefName(dest->base), QCC_GetSRefName(dest->index));
				else if (dest->index.cast)
					QCC_PR_ParseWarning(WARN_STRICTTYPEMISMATCH, "type mismatch: %s %s to %s[%s]", typea, QCC_GetSRefName(source), typeb, QCC_GetSRefName(dest->base), QCC_GetSRefName(dest->index));
				else
					QCC_PR_ParseWarning(WARN_STRICTTYPEMISMATCH, "type mismatch: %s %s to %s %s", typea, QCC_GetSRefName(source), typeb, QCC_GetSRefName(dest->base));
			}
		}
	}

	for(;;)
	{
		switch(dest->type)
		{
		case REF_ARRAYHEAD:
			QCC_PR_ParseWarning(ERR_PARSEERRORS, "left operand must be an l-value (add you mean %s[0]?)", QCC_GetSRefName(dest->base));
			if (!preservedest)
				QCC_PR_DiscardRef(dest);
			break;
		default:
			QCC_PR_ParseWarning(ERR_PARSEERRORS, "left operand must be an l-value (unsupported reference type)", QCC_GetSRefName(dest->base));
			if (!preservedest)
				QCC_PR_DiscardRef(dest);
			break;
		case REF_GLOBAL:
		case REF_ARRAY:
			if (!dest->index.cast || dest->index.sym->constant)
			{
				QCC_sref_t dd;
	//			QCC_PR_ParseWarning(0, "FIXME: trying to do references: assignments to arrays with const offset not supported.\n");
		case REF_NONVIRTUAL:
				dd.cast = dest->cast;
				dd.ofs = dest->base.ofs;
				dd.sym = dest->base.sym;

				if (dest->index.cast)
				{
					if (!preservedest)
						QCC_FreeTemp(dest->index);
					if (dest->index.cast->type == ev_float)
						dd.ofs += dest->index.sym->symboldata[dest->index.ofs]._float;
					else if (dest->index.cast->type == ev_integer)
						dd.ofs += dest->index.sym->symboldata[dest->index.ofs]._int;
					else
						QCC_PR_ParseErrorPrintSRef(ERR_INTERNAL, dest->base, "static array index is not float nor int");
				}

				//FIXME: can dest even be a temp?
//				if (readable)
//					QCC_UnFreeTemp(source);
				source = QCC_CollapseStore(dd, source, dest->cast, readable, preservedest);
			}
			else
			{
				if (readable)
					QCC_UnFreeTemp(source);
				QCC_StoreToArray(dest->base, dest->index, source, dest->cast);
			}
			break;
		case REF_POINTER:
			source.sym->referenced = true;
			if (dest->index.cast)
			{
				QCC_sref_t addr;
				addr = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_PIW], dest->base, QCC_SupplyConversion(dest->index, ev_integer, true), NULL, preservedest?STFL_PRESERVEA:0);
				QCC_StoreToPointer(addr, source, dest->cast);
				QCC_FreeTemp(addr);
			}
			else
			{
				QCC_StoreToPointer(dest->base, source, dest->cast);
				if (dest->base.sym)
					dest->base.sym->referenced = true;
				if (!preservedest)
					QCC_FreeTemp(dest->base);
			}
			if (!readable)
			{
				QCC_FreeTemp(source);
				source = nullsref;
			}
			break;
		case REF_STRING:
			{
				QCC_sref_t addr;
				if (dest->index.cast)
				{
					addr = QCC_PR_Statement(&pr_opcodes[OP_ADD_I], dest->base, QCC_SupplyConversion(dest->index, ev_integer, true), NULL);
				}
				else
				{
					addr = dest->base;
				}
				QCC_PR_Statement(&pr_opcodes[OP_STOREP_C], addr, source, NULL);
			}
			break;
		case REF_ACCESSOR:
			if (dest->accessor && dest->accessor->getset_func[1].cast)
			{
				int args = 0;
				QCC_sref_t arg[3];
				if (dest->accessor->getset_isref[1])
				{
					if (dest->base.sym->temp)
						QCC_PR_ParseErrorPrintDef(ERR_NOFUNC, dest->base.sym, "Accessor %s(set) cannot be used on a temporary accessor reference", dest->accessor?dest->accessor->fieldname:"");
					//there shouldn't really be any need for this, but its problematic if the accessor is a field.
					arg[args++] = QCC_PR_Statement(&pr_opcodes[OP_GLOBALADDRESS], dest->base, nullsref, NULL);
				}
				else
					arg[args++] = dest->base;
				if (dest->accessor->indexertype)
					arg[args++] = dest->index.cast?QCC_SupplyConversion(dest->index, dest->accessor->indexertype->type, true):QCC_MakeIntConst(0);
				arg[args++] = source;

				if (readable)	//if we're returning source, make sure it can't get freed
					QCC_UnFreeTemp(source);
				QCC_ForceUnFreeDef(dest->accessor->getset_func[1].sym);
				QCC_FreeTemp(QCC_PR_GenerateFunctionCall(nullsref, dest->accessor->getset_func[1], arg, NULL/*argt*/, args));
			}
			else
				QCC_PR_ParseErrorPrintSRef(ERR_NOFUNC, dest->base, "Accessor has no set function");
			break;
		case REF_FIELD:
//			{
				//fixme: we should do this earlier, to preserve original instruction ordering.
				//such that self.enemy = (self = world); still has the same result (more common with function calls)
				
				dest = QCC_PR_BuildRef(&ptrref, REF_POINTER, 
								QCC_PR_StatementFlags(&pr_opcodes[OP_ADDRESS], dest->base, dest->index, NULL, preservedest?STFL_PRESERVEA:0),	//pointer address
								nullsref, (dest->index.cast->type == ev_field)?dest->index.cast->aux_type:type_variant, dest->readonly);
				preservedest = false;
				continue;

//				source = QCC_StoreToRef(
//						QCC_PR_BuildRef(&tmp, REF_POINTER, 
//								QCC_PR_StatementFlags(&pr_opcodes[OP_ADDRESS], dest->base, dest->index, NULL, preservedest?STFL_PRESERVEA:0),	//pointer address
//								NULL, (dest->index->type->type == ev_field)?dest->index->type->aux_type:type_variant, dest->readonly),
//						source, readable, false);
	//		QCC_PR_ParseWarning(ERR_INTERNAL, "FIXME: trying to do references: assignments to ent.field not supported.\n");
//			}
//			break;
		}
		break;
	}
	return source;
}

/*QCC_ref_t *QCC_PR_RefTerm (QCC_ref_t *ref, unsigned int exprflags)
{
	return QCC_DefToRef(ref, QCC_PR_Term(exprflags));
}*/
QCC_sref_t QCC_PR_Term (unsigned int exprflags)
{
	QCC_ref_t refbuf;
	return QCC_RefToDef(QCC_PR_RefTerm(&refbuf, exprflags), true);
}
QCC_sref_t	QCC_PR_ParseValue (QCC_type_t *assumeclass, pbool allowarrayassign, pbool expandmemberfields, pbool makearraypointers)
{
	QCC_ref_t refbuf;
	return QCC_RefToDef(QCC_PR_ParseRefValue(&refbuf, assumeclass, allowarrayassign, expandmemberfields, makearraypointers), true);
}
QCC_sref_t QCC_PR_ParseArrayPointer (QCC_sref_t d, pbool allowarrayassign, pbool makestructpointers)
{
	QCC_ref_t refbuf;
	QCC_ref_t inr;
	QCC_DefToRef(&inr, d);
	return QCC_RefToDef(QCC_PR_ParseRefArrayPointer(&refbuf, &inr, allowarrayassign, makestructpointers), true);
}

void QCC_PR_DiscardRef(QCC_ref_t *ref)
{
	if (ref->postinc)
	{
		QCC_sref_t oval;
		int inc = ref->postinc;
		ref->postinc = 0;
		//read the value
		oval = QCC_RefToDef(ref, false);
		//and update it
		switch(ref->cast->type)
		{
		case ev_float:
			oval = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_F], oval, QCC_MakeFloatConst(inc), NULL, 0);
			break;
		case ev_integer:
			oval = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_I], oval, QCC_MakeIntConst(inc), NULL, 0);
			break;
		case ev_pointer:
			inc *= ref->cast->aux_type->size;
			oval = QCC_PR_StatementFlags(&pr_opcodes[OP_ADD_PIW], oval, QCC_MakeIntConst(inc), NULL, 0);
			break;
		default:
			QCC_PR_ParseErrorPrintSRef(ERR_INTERNAL, oval, "post increment operator not supported with this type");
			break;
		}
		QCC_StoreToRef(ref, oval, false, false);
		qcc_usefulstatement = true;
	}
	else
	{
		QCC_FreeTemp(ref->base);
		if (ref->index.cast)
			QCC_FreeTemp(ref->index);
	}
}

QCC_opcode_t *QCC_PR_ChooseOpcode(QCC_sref_t lhs, QCC_sref_t rhs, QCC_opcode_t **priority)
{
	QCC_opcode_t	*op, *oldop;

	QCC_opcode_t *bestop;
	int numconversions, c;

	etype_t type_a;
	etype_t type_c;

	op = oldop = *priority++;

	// type check
	type_a = lhs.cast->type;
//	type_b = rhs.cast->type;

	if (op->name[0] == '.')// field access gets type from field
	{
		if (rhs.cast->aux_type)
			type_c = rhs.cast->aux_type->type;
		else
			type_c = -1;	// not a field
	}
	else
		type_c = ev_void;

	bestop = NULL;
	numconversions = 32767;
	while (op)
	{
		if (!(type_c != ev_void && type_c != (*op->type_c)->type))
		{
			if (!STRCMP (op->name , oldop->name))	//matches
			{
				//return values are never converted - what to?
//				if (type_c != ev_void && type_c != op->type_c->type->type)
//				{
//					op++;
//					continue;
//				}

				if (op->associative!=ASSOC_LEFT)
				{//assignment
#if 0
					if (op->type_a == &type_pointer)	//ent var
					{
						/*FIXME: I don't like this code*/
						if (lhs->type->type != ev_pointer)
							c = -200;	//don't cast to a pointer.
						else if ((*op->type_c)->type == ev_void && op->type_b == &type_pointer && rhs.cast->type == ev_pointer)
							c = 0;	//generic pointer... fixme: is this safe? make sure both sides are equivelent
						else if (lhs->type->aux_type->type != (*op->type_b)->type)	//if e isn't a pointer to a type_b
							c = -200;	//don't let the conversion work
						else
							c = QCC_canConv(rhs, (*op->type_c)->type);
					}
					else
#endif
					{
						c=QCC_canConv(rhs, (*op->type_b)->type);
						if (type_a != (*op->type_a)->type)	//in this case, a is the final assigned value
							c = -300;	//don't use this op, as we must not change var b's type
						else if ((*op->type_a)->type == ev_pointer && lhs.cast->aux_type->type != (*op->type_a)->aux_type->type)
							c = -300;	//don't use this op if its a pointer to a different type
					}
				}
				else
				{
					/*if (op->type_a == &type_pointer)	//ent var
					{
						if (e2->type->type != ev_pointer || e2->type->aux_type->type != (*op->type_b)->type)	//if e isn't a pointer to a type_b
							c = -200;	//don't let the conversion work
						else
							c = 0;
					}
					else*/
					{
						c=QCC_canConv(lhs, (*op->type_a)->type);
						c+=QCC_canConv(rhs, (*op->type_b)->type);
					}
				}

				if (c>=0 && c < numconversions)
				{
					bestop = op;
					numconversions=c;
					if (c == 0)//can't get less conversions than 0...
						break;
				}
			}
			else
				break;
		}
		op = *priority++;
	}
	if (bestop == NULL)
	{
		if (oldop->priority == CONDITION_PRIORITY)
			op = oldop;
		else
		{
			op = oldop;
			QCC_PR_ParseWarning(flag_laxcasts?WARN_LAXCAST:ERR_TYPEMISMATCH, "type mismatch for %s (%s and %s)", oldop->name, lhs.cast->name, rhs.cast->name);
			QCC_PR_ParsePrintSRef(flag_laxcasts?WARN_LAXCAST:ERR_TYPEMISMATCH, lhs);
			QCC_PR_ParsePrintSRef(flag_laxcasts?WARN_LAXCAST:ERR_TYPEMISMATCH, rhs);
		}
	}
	else
	{
		op = bestop;
		if (numconversions>3)
		{
			c=QCC_canConv(lhs, (*op->type_a)->type);
			if (c>3)
				QCC_PR_ParseWarning(WARN_IMPLICITCONVERSION, "Implicit conversion from %s to %s", lhs.cast->name, (*op->type_a)->name);
			c=QCC_canConv(rhs, (*op->type_b)->type);
			if (c>3)
				QCC_PR_ParseWarning(WARN_IMPLICITCONVERSION, "Implicit conversion from %s to %s", rhs.cast->name, (*op->type_a)->name);
		}
	}
	return op;
}

QCC_ref_t *QCC_PR_RefExpression (QCC_ref_t *retbuf, int priority, int exprflags)
{
	QCC_ref_t rhsbuf;
//	QCC_dstatement32_t	*st;
	QCC_opcode_t	*op;

	int opnum;

	QCC_ref_t		*lhsr, *rhsr;
	QCC_sref_t		lhsd, rhsd;

	if (priority == 0)
	{
		lhsr = QCC_PR_RefTerm (retbuf, exprflags);
		if (!STRCMP(pr_token, "++"))
		{
			if (lhsr->readonly)
				QCC_PR_ParseError(ERR_PARSEERRORS, "postincrement: lhs is readonly");
			lhsr->postinc += 1;
			QCC_PR_Lex();
		}
		else if (!STRCMP(pr_token, "--"))
		{
			if (lhsr->readonly)
				QCC_PR_ParseError(ERR_PARSEERRORS, "postdecrement: lhs is readonly");
			lhsr->postinc += -1;
			QCC_PR_Lex();
		}
		return lhsr;
	}

	lhsr = QCC_PR_RefExpression (retbuf, priority-1, exprflags);

	while (1)
	{
		if (priority == FUNC_PRIORITY && QCC_PR_CheckToken ("(") )
		{
			qcc_usefulstatement=true;
			lhsd = QCC_PR_ParseFunctionCall (lhsr);
			lhsd = QCC_PR_ParseArrayPointer(lhsd, true, true);
			lhsr = QCC_DefToRef(retbuf, lhsd);
		}
		if (priority == FUNC_PRIORITY && QCC_PR_CheckToken ("?"))
		{
			//if we have no int types, force all ints to floats here, just to ensure that we don't end up with non-constant ints that we then can't cope with.

			QCC_sref_t val, r;
			QCC_statement_t *fromj, *elsej;
			if (QCC_PR_CheckToken(":"))
			{
				//r=a?:b -> if (a) r=a else r=b;
				val = QCC_RefToDef(lhsr, true);
				fromj = QCC_Generate_OP_IFNOT(val, true);
				r = QCC_GetTemp(val.cast);
				QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[(r.cast->size>=3)?OP_STORE_V:OP_STORE_F], val, r, NULL, STFL_PRESERVEB));
			}
			else
			{
				fromj = QCC_Generate_OP_IFNOT(QCC_RefToDef(lhsr, true), false);

				val = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
				if (val.cast->type == ev_integer && !QCC_OPCodeValid(&pr_opcodes[OP_STORE_I]))
					val = QCC_SupplyConversion(val, ev_float, true);
				r = QCC_GetTemp(val.cast);
				QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[(r.cast->size>=3)?OP_STORE_V:OP_STORE_F], val, r, NULL, STFL_PRESERVEB));
				//r can be stomped upon until its reused anyway

				QCC_PR_Expect(":");
			}
			QCC_PR_Statement(&pr_opcodes[OP_GOTO], nullsref, nullsref, &elsej);
			fromj->b.ofs = &statements[numstatements] - fromj;
			val = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
			if (val.cast->type == ev_integer && !QCC_OPCodeValid(&pr_opcodes[OP_STORE_I]))
				val = QCC_SupplyConversion(val, ev_float, true);

/*			//cond?5:5.1  should be accepted
			if ((val->type->type != val->type->type) &&
				(val->type->type == ev_float || val->type->type == ev_integer) &&
				(r->type->type == ev_float || r->type->type == ev_integer))
			{
				val = QCC_SupplyConversion(val, ev_float, true);
				r = QCC_SupplyConversion(r, ev_float, true);
			}
*/
			if (typecmp(val.cast, r.cast) != 0)
			{
				//if they're mixed int/float, cast to floats.
				QCC_PR_ParseError(0, "Ternary operator with mismatching types\n");
			}
			QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[(r.cast->size>=3)?OP_STORE_V:OP_STORE_F], val, r, NULL, STFL_PRESERVEB));

			elsej->a.ofs = &statements[numstatements] - elsej;
			return QCC_DefToRef(retbuf, r);
		}

		opnum=0;

		if (pr_token_type == tt_immediate)
		{
			if ((pr_immediate_type->type == ev_float && pr_immediate._float < 0) ||
				(pr_immediate_type->type == ev_integer && pr_immediate._int < 0))	//hehehe... was a minus all along...
				{
					QCC_PR_IncludeChunk(pr_token, true, NULL);
					strcpy(pr_token, "+");//two negatives would make a positive.
					pr_token_type = tt_punct;
				}
		}

		if (pr_token_type != tt_punct)
		{
			QCC_PR_ParseWarning(WARN_UNEXPECTEDPUNCT, "Expected punctuation");
		}

		if (priority == 6)
		{	//assignments
			QCC_opcode_t **ops = NULL;
			char *opname = NULL;
			int i;
			if (QCC_PR_CheckToken ("="))
			{
				ops = opcodes_store;
				opname = "=";
			}
			else if (QCC_PR_CheckToken ("+="))
			{
				ops = opcodes_addstore;
				opname = "+=";
			}
			else if (QCC_PR_CheckToken ("-="))
			{
				ops = opcodes_substore;
				opname = "-=";
			}
			else if (QCC_PR_CheckToken ("|="))
			{
				ops = opcodes_orstore;
				opname = "|=";
			}
			else if (QCC_PR_CheckToken ("&="))
			{
				ops = opcodes_andstore;
				opname = "&=";
			}
			else if (QCC_PR_CheckToken ("&~="))
			{
				ops = opcodes_clearstore;
				opname = "&~=";
			}
			else if (QCC_PR_CheckToken ("^="))
			{
				ops = opcodes_xorstore;
				opname = "^=";
			}
			else if (QCC_PR_CheckToken ("*="))
			{
				ops = opcodes_mulstore;
				opname = "*=";
			}
			else if (QCC_PR_CheckToken ("/="))
			{
				ops = opcodes_divstore;
				opname = "/=";
			}

			if (ops)
			{
				if (lhsr->postinc)
					QCC_PR_ParseError(ERR_INTERNAL, "Assignment to post-inc result");
				if (lhsr->readonly)
				{
					QCC_PR_ParseWarning(WARN_ASSIGNMENTTOCONSTANT, "Assignment to lvalue");
					QCC_PR_ParsePrintSRef(WARN_ASSIGNMENTTOCONSTANT, lhsr->base);
					if (lhsr->index.cast)
						QCC_PR_ParsePrintSRef(WARN_ASSIGNMENTTOCONSTANT, lhsr->index);
				}

				rhsr = QCC_PR_RefExpression (&rhsbuf, priority, exprflags | EXPR_DISALLOW_ARRAYASSIGN);

				if (conditional&1)
					QCC_PR_ParseWarning(WARN_ASSIGNMENTINCONDITIONAL, "suggest parenthesis for assignment used as truth value .");

				rhsd = QCC_RefToDef(rhsr, true);

				if (ops != opcodes_store)
				{
					lhsd = QCC_RefToDef(lhsr, false);
					for (i = 0; (op=ops[i]); i++)
					{
//						if (QCC_OPCodeValid(op))
						{
							if ((*op->type_b)->type == rhsd.cast->type && (*op->type_a)->type == lhsd.cast->type)
								break;
						}
					}
					if (!ops[i])
					{
						rhsd = QCC_SupplyConversion(rhsd, lhsr->cast->type, true);
						for (i = 0; ops[i]; i++)
						{
							op = ops[i];
	//						if (QCC_OPCodeValid(op))
							{
								if ((*op->type_b)->type == rhsd.cast->type && (*op->type_a)->type == lhsd.cast->type)
									break;
							}
						}
						if (!ops[i])
							QCC_PR_ParseError(0, "Type mismatch on assignment. %s %s %s is not supported\n", lhsd.cast->name, opname, rhsd.cast->name);
					}

					if (op->associative != ASSOC_LEFT)
						rhsd = QCC_PR_Statement(op, lhsd, rhsd, NULL);
					else
						rhsd = QCC_PR_Statement(op, lhsd, rhsd, NULL);

					//convert so we don't have issues with: i = (int)(float)(i+f)
					//this will also catch things like vec *= vec; which would be trying to store a float into a vector.
					rhsd = QCC_SupplyConversionForAssignment(lhsr->base, rhsd, lhsr->cast, true);
				}
				else
				{
					if (QCC_SRef_IsNull(rhsd))
					{
						QCC_FreeTemp(rhsd);
						if (lhsr->cast->type == ev_vector)
							rhsd = QCC_MakeVectorConst(0,0,0);
						else if (lhsr->cast->type == ev_struct || lhsr->cast->type == ev_union)
						{
							QCC_PR_ParseError(0, "Type mismatch on assignment. %s %s %s is not supported\n", lhsr->cast->name, opname, rhsd.cast->name);
						}
						else if(lhsr->cast->type == ev_float)
							rhsd = QCC_MakeFloatConst(0);
						else if(lhsr->cast->type == ev_integer)
							rhsd = QCC_MakeIntConst(0);
						else
							rhsd = QCC_MakeIntConst(0);
						rhsd.cast = lhsr->cast;
					}
					else
						rhsd = QCC_SupplyConversionForAssignment(lhsr->base, rhsd, lhsr->cast, true);
				}
				rhsd = QCC_StoreToRef(lhsr, rhsd, true, false);	//FIXME: this should not always be true, but we don't know if the caller actually needs it
				qcc_usefulstatement = true;
				lhsr = QCC_DefToRef(retbuf, rhsd);	//we read the rhs, we can just return that as the result
				lhsr->readonly = true;	//(a=b)=c is an error
			}
			else
				break;
		}
		else
		{
			QCC_statement_t *logicjump;
			//go straight for the correct priority.
			for (op = opcodeprioritized[priority][opnum]; op; op = opcodeprioritized[priority][++opnum])
	//		for (op=pr_opcodes ; op->name ; op++)
			{
	//			if (op->priority != priority)
	//				continue;
				if (!QCC_PR_CheckToken (op->name))
					continue;

				logicjump = NULL;
				if (opt_logicops && lhsr->type == REF_GLOBAL)
				{
					lhsd = QCC_RefToDef(lhsr, true);
					if (op == &pr_opcodes[OP_AND_F])		//guarenteed to be false if the lhs is false
						logicjump = QCC_Generate_OP_IFNOT(lhsd, true);
					else if (op == &pr_opcodes[OP_OR_F])	//guarenteed to be true if the lhs is true
						logicjump = QCC_Generate_OP_IF(lhsd, true);
				}
				else
					lhsd = QCC_RefToDef(lhsr, true);
				
				rhsr = QCC_PR_RefExpression (&rhsbuf, priority-1, exprflags | EXPR_DISALLOW_ARRAYASSIGN);

				if (op->associative!=ASSOC_LEFT)
				{
					QCC_PR_ParseError(ERR_INTERNAL, "internal error: should be unreachable\n");
				}
				else
				{
					rhsd = QCC_RefToDef(rhsr, true);
					op = QCC_PR_ChooseOpcode(lhsd, rhsd, &opcodeprioritized[priority][opnum]);
				
					if (logicjump)	//logic shortcut jumps to just before the if. the rhs is uninitialised if the jump was taken, but the lhs makes it deterministic.
					{
						logicjump->b.ofs = &statements[numstatements] - logicjump;
						if (logicjump->b.ofs == 1)
							numstatements--;	//err, that was pointless.
						else if (logicjump->b.ofs == 2 && !(logicjump[1].op >= OP_CALL0 && logicjump[1].op <= OP_CALL8) && !(logicjump[1].op >= OP_CALL1H && logicjump[1].op <= OP_CALL8H))
						{
							logicjump[0] = logicjump[1];
							numstatements--;	//don't bother if the jump is the same cost as the thing we're trying to skip (calls are expensive despite being a single opcode).
						}
						else
							optres_logicops++;
					}

					lhsd = QCC_PR_Statement (op, lhsd, rhsd, NULL);
					lhsr = QCC_DefToRef(retbuf, lhsd);
				}

				if (priority > 1 && exprflags & EXPR_WARN_ABOVE_1)
					QCC_PR_ParseWarning(WARN_UNARYNOTSCOPE, "unary-not applies to non-unary expression");

				break;
			}
			if (!op)
				break;
		}
	}
	if (lhsr == NULL)
		QCC_PR_ParseError(ERR_INTERNAL, "e == null");

	if (!(exprflags&EXPR_DISALLOW_COMMA) && priority == TOP_PRIORITY && QCC_PR_CheckToken (","))
	{
		QCC_PR_DiscardRef(lhsr);
		if (!qcc_usefulstatement)
			QCC_PR_ParseWarning(WARN_POINTLESSSTATEMENT, "Statement does not do anything");
		qcc_usefulstatement = false;
		lhsr = QCC_PR_RefExpression(retbuf, TOP_PRIORITY, exprflags);
	}
	return lhsr;
}

QCC_sref_t QCC_PR_Expression (int priority, int exprflags)
{
	QCC_ref_t refbuf, *ret;
	ret = QCC_PR_RefExpression(&refbuf, priority, exprflags);
	return QCC_RefToDef(ret, true);
}
//parse the expression and discard the result. generate a warning if there were no assignments
//this avoids generating getter statements from RefToDef in QCC_PR_Expression.
void QCC_PR_DiscardExpression (int priority, int exprflags)
{
	QCC_ref_t refbuf, *ref;
	pbool olduseful = qcc_usefulstatement;
	qcc_usefulstatement = false;

	ref = QCC_PR_RefExpression(&refbuf, priority, exprflags);
	QCC_PR_DiscardRef(ref);

	if (ref->cast->type != ev_void && !qcc_usefulstatement)
	{
//		int osl = pr_source_line;
//		pr_source_line = statementstart;
		QCC_PR_ParseWarning(WARN_POINTLESSSTATEMENT, "Statement does not do anything");
//		pr_source_line = osl;
	}
	qcc_usefulstatement = olduseful;
}

int QCC_PR_IntConstExpr(void)
{
	//fixme: should make sure that no actual statements are generated
	QCC_sref_t def = QCC_PR_Expression(TOP_PRIORITY, 0);
	if (def.sym->constant)
	{
		QCC_FreeTemp(def);
		def.sym->referenced = true;
		if (def.cast->type == ev_integer)
			return def.sym->symboldata[def.ofs]._int;
		if (def.cast->type == ev_float)
		{
			int i = def.sym->symboldata[def.ofs]._float;
			if ((float)i == def.sym->symboldata[def.ofs]._float)
				return i;
		}
	}
	QCC_PR_ParseError(ERR_NOTACONSTANT, "Value is not an integer constant");
	return true;
}

void QCC_PR_GotoStatement (QCC_statement_t *patch2, char *labelname)
{
	if (num_gotos >= max_gotos)
	{
		max_gotos += 8;
		pr_gotos = realloc(pr_gotos, sizeof(*pr_gotos)*max_gotos);
	}

	strncpy(pr_gotos[num_gotos].name, labelname, sizeof(pr_gotos[num_gotos].name) -1);
	pr_gotos[num_gotos].lineno = pr_source_line;
	pr_gotos[num_gotos].statementno = patch2 - statements;

	num_gotos++;
}

pbool QCC_PR_StatementBlocksMatch(QCC_statement_t *p1, int p1count, QCC_statement_t *p2, int p2count)
{
	if (p1count != p2count)
		return false;

	while(p1count>0)
	{
		if (p1->op != p2->op)
			return false;
		if (memcmp(&p1->a, &p2->a, sizeof(p1->a)))
			return false;
		if (memcmp(&p1->b, &p2->b, sizeof(p1->b)))
			return false;
		if (memcmp(&p1->c, &p2->c, sizeof(p1->c)))
			return false;
		p1++;
		p2++;
		p1count--;
	}

	return true;
}

//vanilla qc only has an OP_IFNOT_I, others will be emulated as required, so we tend to need to emulate other opcodes.
QCC_statement_t *QCC_Generate_OP_IF(QCC_sref_t e, pbool preserve)
{
	unsigned int flags = STFL_CONVERTA | (preserve?STFL_PRESERVEA:0);
	QCC_statement_t *st;
	int op = 0;

	switch(e.cast->type)
	{
	//int/pointer types
	case ev_entity:
	case ev_field:
	case ev_function:
	case ev_pointer:
	case ev_integer:
		op = OP_IF_I;
		break;

	//emulated types
	case ev_string:
		QCC_PR_ParseWarning(WARN_IFSTRING_USED, "if (string) tests for null, not empty.");
		if (flag_ifstring)
			op = OP_IF_S;
		else
			op = OP_IF_I;
		break;
	case ev_float:
		if (flag_iffloat || QCC_OPCodeValid(&pr_opcodes[OP_IF_F]))
			op = OP_IF_F;
		else
			op = OP_IF_I;
		break;
	case ev_vector:
		if (flag_ifvector)
		{
			e = QCC_PR_StatementFlags (&pr_opcodes[OP_NOT_V], e, nullsref, NULL, flags);
			op = OP_IFNOT_I;
		}
		else
			op = OP_IF_I;
		break;

	case ev_variant:
	case ev_struct:
	case ev_union:
	case ev_void:
	default:
		QCC_PR_ParseWarning(WARN_CONDITIONALTYPEMISMATCH, "conditional type mismatch: %s", basictypenames[e.cast->type]);
		op = OP_IF_I;
		break;
	}

	QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[op], e, nullsref, &st, flags|STFL_DISCARDRESULT));
	return st;
}
QCC_statement_t *QCC_Generate_OP_IFNOT(QCC_sref_t e, pbool preserve)
{
	unsigned int flags = STFL_CONVERTA | (preserve?STFL_PRESERVEA:0);
	QCC_statement_t *st;
	int op = 0;

	if (!e.cast)
		e.cast = type_void;
	switch(e.cast->type)
	{
	//int/pointer types
	case ev_entity:
	case ev_field:
	case ev_function:
	case ev_pointer:
	case ev_integer:
		op = OP_IFNOT_I;
		break;

	//emulated types
	case ev_string:
		QCC_PR_ParseWarning(WARN_IFSTRING_USED, "if (string) tests for null, not empty");
		if (flag_ifstring)
			op = OP_IFNOT_S;
		else
			op = OP_IFNOT_I;
		break;
	case ev_float:
		if (flag_iffloat || QCC_OPCodeValid(&pr_opcodes[OP_IFNOT_F]))
			op = OP_IFNOT_F;
		else
			op = OP_IFNOT_I;
		break;
	case ev_vector:
		if (flag_ifvector)
		{
			e = QCC_PR_StatementFlags (&pr_opcodes[OP_NOT_V], e, nullsref, NULL, flags);
			op = OP_IF_I;
		}
		else
			op = OP_IFNOT_I;
		break;

	case ev_variant:
	case ev_struct:
	case ev_union:
	case ev_void:
	default:
		QCC_PR_ParseWarning(WARN_CONDITIONALTYPEMISMATCH, "conditional type mismatch: %s", basictypenames[e.cast->type]);
		op = OP_IFNOT_I;
		break;
	}

	QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[op], e, nullsref, &st, flags|STFL_DISCARDRESULT));
	return st;
}

//for consistancy with if+ifnot
QCC_statement_t *QCC_Generate_OP_GOTO(void)
{
	QCC_statement_t *st;
	QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[OP_GOTO], nullsref, nullsref, &st, STFL_DISCARDRESULT));
	return st;
}

/*
============
PR_ParseStatement

============
*/
void QCC_PR_ParseStatement (void)
{
	int continues;
	int breaks;
	int cases;
	int i;
	QCC_sref_t				e, e2;
	QCC_def_t	*d;
	QCC_statement_t		*patch1, *patch2, *patch3, *patch4;
	int statementstart = pr_source_line;
	pbool wasuntil;

	QCC_ClobberDef(NULL);	//make sure any conditionals don't weird out.

	if (QCC_PR_CheckToken ("{"))
	{
		d = pr.local_tail;
		while (!QCC_PR_CheckToken("}"))
			QCC_PR_ParseStatement ();

		if (pr_subscopedlocals)
		{	//remove any new locals from the hashtable.
			for (d = d->nextlocal; d; d = d->nextlocal)
			{
				if (!d->subscoped_away)
				{
					pHash_RemoveData(&localstable, d->name, d);
					d->subscoped_away = true;
				}
			}
		}
		return;
	}

	if (QCC_PR_CheckKeyword(keyword_return, "return"))
	{
		/*if (pr_classtype)
		{
			e = QCC_PR_GetDef(NULL, "__oself", pr_scope, false, 0);
			e2 = QCC_PR_GetDef(NULL, "self", NULL, false, 0);
			QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_STORE_ENT], e, QCC_PR_DummyDef(pr_classtype, "self", pr_scope, 0, e2->ofs, false), NULL));
		}*/

		if (QCC_PR_CheckToken (";"))
		{
			if (pr_scope->type->aux_type->type != ev_void)
				QCC_PR_ParseWarning(WARN_MISSINGRETURNVALUE, "\'%s\' returned nothing, expected %s", pr_scope->name, pr_scope->type->aux_type->name);
			if (opt_return_only)
				QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_DONE], nullsref, nullsref, NULL));
			else
				QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_RETURN], nullsref, nullsref, NULL));
			return;
		}
		e = QCC_PR_Expression (TOP_PRIORITY, 0);
		if (QCC_SRef_IsNull(e))
		{
			QCC_FreeTemp(e);
			//return __NULL__; is allowed regardless of actual return type.
			switch (pr_scope->type->aux_type->type)
			{
			case ev_vector:
				e = QCC_MakeVectorConst(0, 0, 0);
				break;
			default:
			case ev_float:
				e = QCC_MakeFloatConst(0);
				break;
			}
			e.cast = pr_scope->type->aux_type;
		}
		else if (pr_scope->type->aux_type->type != e.cast->type)
		{
			e = QCC_SupplyConversion(e, pr_scope->type->aux_type->type, true);
//			QCC_PR_ParseWarning(WARN_WRONGRETURNTYPE, "\'%s\' returned %s, expected %s", pr_scope->name, e->type->name, pr_scope->type->aux_type->name);
		}
		QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_RETURN], e, nullsref, NULL));
		QCC_PR_Expect (";");
		return;
	}
	if (QCC_PR_CheckKeyword(keyword_exit, "exit"))
	{
		QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_DONE], nullsref, nullsref, NULL));
		QCC_PR_Expect (";");
		return;
	}

	if (QCC_PR_CheckKeyword(keyword_loop, "loop"))
	{
		continues = num_continues;
		breaks = num_breaks;

		patch2 = &statements[numstatements];
		QCC_PR_ParseStatement ();
		QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_GOTO], nullsref, nullsref, &patch3));
		patch3->a.ofs = patch2 - patch3;

		if (breaks != num_breaks)
		{
			for(i = breaks; i < num_breaks; i++)
			{
				patch1 = &statements[pr_breaks[i]];
				statements[pr_breaks[i]].a.ofs = &statements[numstatements] - patch1;	//jump to after the return-to-top goto
			}
			num_breaks = breaks;
		}
		if (continues != num_continues)
		{
			for(i = continues; i < num_continues; i++)
			{
				patch1 = &statements[pr_continues[i]];
				statements[pr_continues[i]].a.ofs = patch2 - patch1;	//jump back to top
			}
			num_continues = continues;
		}
		return;
	}

	wasuntil = QCC_PR_CheckKeyword(keyword_until, "until");
	if (wasuntil || QCC_PR_CheckKeyword(keyword_while, "while"))
	{
		const QCC_eval_t *eval;
		continues = num_continues;
		breaks = num_breaks;

		QCC_PR_Expect ("(");
		patch2 = &statements[numstatements];
		conditional = 1;
		e = QCC_PR_Expression (TOP_PRIORITY, 0);
		conditional = 0;

		eval = QCC_SRef_EvalConst(e);
		if (eval && opt_compound_jumps && e.cast->type == ev_float)
		{
			optres_compound_jumps++;
			QCC_FreeTemp(e);
			if (!eval->_float != wasuntil)
				QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_GOTO], nullsref, nullsref, &patch1));
			else
				patch1 = NULL;
		}
		else if (wasuntil)
			patch1 = QCC_Generate_OP_IF(e, false);
		else
			patch1 = QCC_Generate_OP_IFNOT(e, false);
		QCC_PR_Expect (")");	//after the line number is noted..
		QCC_PR_ParseStatement ();
		patch3 = QCC_Generate_OP_GOTO();
		patch3->a.ofs = patch2 - patch3;
		if (patch1)
		{
			if (patch1->op == OP_GOTO)
				patch1->a.ofs = &statements[numstatements] - patch1;
			else
				patch1->b.ofs = &statements[numstatements] - patch1;
		}

		if (breaks != num_breaks)
		{
			for(i = breaks; i < num_breaks; i++)
			{
				patch1 = &statements[pr_breaks[i]];
				statements[pr_breaks[i]].a.ofs = &statements[numstatements] - patch1;	//jump to after the return-to-top goto
			}
			num_breaks = breaks;
		}
		if (continues != num_continues)
		{
			for(i = continues; i < num_continues; i++)
			{
				patch1 = &statements[pr_continues[i]];
				statements[pr_continues[i]].a.ofs = patch2 - patch1;	//jump back to top
			}
			num_continues = continues;
		}
		return;
	}
	if (QCC_PR_CheckKeyword(keyword_for, "for"))
	{
		int old_numstatements;
		int numtemp, i;

		QCC_statement_t		temp[256];

		continues = num_continues;
		breaks = num_breaks;

		QCC_PR_Expect("(");
		if (!QCC_PR_CheckToken(";"))
		{
			QCC_PR_DiscardExpression(TOP_PRIORITY, 0);
			QCC_PR_Expect(";");
		}

		QCC_ClobberDef(NULL);

		patch2 = &statements[numstatements];	//restart of the loop
		if (!QCC_PR_CheckToken(";"))
		{
			conditional = 1;
			e = QCC_PR_Expression(TOP_PRIORITY, 0);
			conditional = 0;
			QCC_PR_Expect(";");
		}
		else
			e = nullsref;

		if (e.cast)	//final condition+jump
			QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_IFNOT_I], e, nullsref, &patch1, STFL_DISCARDRESULT));
		else
			patch1 = NULL;

		if (!QCC_PR_CheckToken(")"))
		{
			old_numstatements = numstatements;
			QCC_PR_DiscardExpression(TOP_PRIORITY, 0);

			numtemp = numstatements - old_numstatements;
			if (numtemp > sizeof(temp)/sizeof(temp[0]))
				QCC_PR_ParseError(ERR_TOOCOMPLEX, "Update expression too large");
			numstatements = old_numstatements;
			for (i = 0 ; i < numtemp ; i++)
			{
				temp[i] = statements[numstatements + i];
			}

			QCC_PR_Expect(")");
		}
		else
			numtemp = 0;

		//parse the statement block
		if (!QCC_PR_CheckToken(";"))
			QCC_PR_ParseStatement();	//don't give the hanging ';' warning.
		patch3 = &statements[numstatements];	//location for continues
		//reinsert the 'increment' statements. lets hope they didn't have any gotos...
		for (i = 0 ; i < numtemp ; i++)
		{
			statements[numstatements] = temp[i];
			statements[numstatements].linenum = pr_token_line_last;
			numstatements++;
		}
		patch4 = QCC_Generate_OP_GOTO();
		patch4->a.ofs = patch2 - patch4;
		if (patch1)
			patch1->b.ofs = &statements[numstatements] - patch1;	//condition failure jumps here

		//fix up breaks+continues
		if (breaks != num_breaks)
		{
			for(i = breaks; i < num_breaks; i++)
			{
				patch1 = &statements[pr_breaks[i]];
				statements[pr_breaks[i]].a.ofs = &statements[numstatements] - patch1;
			}
			num_breaks = breaks;
		}
		if (continues != num_continues)
		{
			for(i = continues; i < num_continues; i++)
			{
				patch1 = &statements[pr_continues[i]];
				statements[pr_continues[i]].a.ofs = patch3 - patch1;
			}
			num_continues = continues;
		}

		return;
	}
	if (QCC_PR_CheckKeyword(keyword_do, "do"))
	{
		const QCC_eval_t *eval;
		pbool until;
		continues = num_continues;
		breaks = num_breaks;

		patch1 = &statements[numstatements];
		QCC_PR_ParseStatement ();
		until = QCC_PR_CheckKeyword(keyword_until, "until");
		if (!until)
			QCC_PR_Expect ("while");
		QCC_PR_Expect ("(");
		conditional = 1;
		e = QCC_PR_Expression (TOP_PRIORITY, 0);
		conditional = 0;

		eval = QCC_SRef_EvalConst(e);
		if (eval)
		{
			if (until?!eval->_int:eval->_int)
			{
				QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_GOTO], nullsref, nullsref, &patch2));
				patch2->a.ofs = patch1 - patch2;
			}
			QCC_FreeTemp(e);
		}
		else
		{
			if (until)
				patch2 = QCC_Generate_OP_IFNOT(e, false);
			else
				patch2 = QCC_Generate_OP_IF(e, false);
			patch2->b.ofs = patch1 - patch2;
		}

		QCC_PR_Expect (")");
		QCC_PR_Expect (";");

		if (breaks != num_breaks)
		{
			for(i = breaks; i < num_breaks; i++)
			{
				patch2 = &statements[pr_breaks[i]];
				statements[pr_breaks[i]].a.ofs = &statements[numstatements] - patch2;
			}
			num_breaks = breaks;
		}
		if (continues != num_continues)
		{
			for(i = continues; i < num_continues; i++)
			{
				patch2 = &statements[pr_continues[i]];
				statements[pr_continues[i]].a.ofs = patch1 - patch2;
			}
			num_continues = continues;
		}

		return;
	}

	if (QCC_PR_CheckKeyword(keyword_local, "local"))
	{
//		if (locals_end != numpr_globals)	//is this breaking because of locals?
//			QCC_PR_ParseWarning("local vars after temp vars\n");
		QCC_PR_ParseDefs (NULL);
		return;
	}

	if (pr_token_type == tt_name)
	{
		QCC_type_t *type = QCC_TypeForName(pr_token);
		if (type && type->typedefed)
		{
			if (strncmp(pr_file_p, "::", 2))
			{
				QCC_PR_ParseDefs (NULL);
				return;
			}
		}

		if ((keyword_var && !STRCMP ("var", pr_token)) ||
			(keyword_string && !STRCMP ("string", pr_token)) ||
			(keyword_float && !STRCMP ("float", pr_token)) ||
			(keyword_entity && !STRCMP ("entity", pr_token)) ||
			(keyword_vector && !STRCMP ("vector", pr_token)) ||
			(keyword_integer && !STRCMP ("integer", pr_token)) ||
			(keyword_int && !STRCMP ("int", pr_token)) ||
			(keyword_class && !STRCMP ("class", pr_token)) ||
			(keyword_class && !STRCMP ("static", pr_token)) ||
			(keyword_const && !STRCMP ("const", pr_token)))
		{
			QCC_PR_ParseDefs (NULL);
			return;
		}
	}

	if (QCC_PR_CheckKeyword(keyword_state, "state"))
	{
		QCC_PR_Expect("[");
		QCC_PR_ParseState();
		QCC_PR_Expect(";");
		return;
	}
	if (QCC_PR_CheckToken("#"))
	{
		char *name;
		float frame = pr_immediate._float;
		QCC_PR_Lex();
		name = QCC_PR_ParseName();
		QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_STATE], QCC_MakeFloatConst(frame), QCC_PR_GetSRef(type_function, name, NULL, false, 0, false), NULL));
		QCC_PR_Expect(";");
		return;
	}

	if (QCC_PR_CheckKeyword(keyword_if, "if"))
	{
		unsigned int oldnumst, oldlab;
		pbool striptruth = false;
		pbool stripfalse = false;
		const QCC_eval_t *eval;
		pbool negate = QCC_PR_CheckKeyword(keyword_not, "not");

		QCC_PR_Expect ("(");
		conditional = 1;
		e = QCC_PR_Expression (TOP_PRIORITY, 0);
		conditional = 0;

		eval = QCC_SRef_EvalConst(e);

//		negate = negate != 0;

		oldnumst = numstatements;
		if (eval)
		{
			if (e.cast->type == ev_float)
				striptruth = eval->_float == 0;
			else
				striptruth = eval->_int == 0;
			if (negate)
				striptruth = !striptruth;
			stripfalse = !striptruth;
			patch1 = NULL;
			QCC_FreeTemp(e);

			if (striptruth)
				patch1 = QCC_Generate_OP_GOTO();
		}
		else if (negate)
		{
			patch1 = QCC_Generate_OP_IF(e, false);
		}
		else
		{
			patch1 = QCC_Generate_OP_IFNOT(e, false);
		}

		QCC_PR_Expect (")");	//close bracket is after we save the statement to mem (so debugger does not show the if statement as being on the line after

		oldlab = num_labels;
		QCC_PR_ParseStatement ();
		if (striptruth && oldlab == num_labels)
		{
			numstatements = oldnumst;
			patch1 = NULL;
		}
		else
			striptruth = false;

		if (QCC_PR_CheckKeyword (keyword_else, "else"))
		{
			int lastwasreturn;
			lastwasreturn = statements[numstatements-1].op == OP_RETURN || statements[numstatements-1].op == OP_DONE ||
				statements[numstatements-1].op == OP_GOTO;

			//the last statement of the if was a return, so we don't need the goto at the end
			if (lastwasreturn && opt_compound_jumps && !QCC_AStatementJumpsTo(numstatements, patch1-statements, numstatements))
			{
//				QCC_PR_ParseWarning(0, "optimised the else");
				optres_compound_jumps++;
				if (patch1)
					patch1->b.ofs = &statements[numstatements] - patch1;
				oldnumst = numstatements;
				oldlab = num_labels;
				QCC_PR_ParseStatement ();
				if (stripfalse && oldlab == num_labels)
				{
					patch2 = NULL;
					numstatements = oldnumst;

					if (patch1)
						patch1->b.ofs = &statements[numstatements] - patch1;
				}
			}
			else
			{
//				QCC_PR_ParseWarning(0, "using the else");
				oldnumst = numstatements;
				if (striptruth)
					patch2 = NULL;
				else
					patch2 = QCC_Generate_OP_GOTO();
				if (patch1)
					patch1->b.ofs = &statements[numstatements] - patch1;

				oldlab = num_labels;
				QCC_PR_ParseStatement ();
				if (stripfalse && oldlab == num_labels)
				{
					patch2 = NULL;
					numstatements = oldnumst;

					if (patch1)
						patch1->b.ofs = &statements[numstatements] - patch1;
				}

				if (patch2)
					patch2->a.ofs = &statements[numstatements] - patch2;

				if (QCC_PR_StatementBlocksMatch(patch1+1, patch2-patch1, patch2+1, &statements[numstatements] - patch2))
					QCC_PR_ParseWarning(0, "Two identical blocks each side of an else");
			}
		}
		else if (patch1)
		{
			if (patch1->op == OP_GOTO)
				patch1->a.ofs = &statements[numstatements] - patch1;
			else
				patch1->b.ofs = &statements[numstatements] - patch1;
		}

		return;
	}
	if (QCC_PR_CheckKeyword(keyword_switch, "switch"))
	{
		int op;
		int hcstyle;
		int defaultcase = -1;
		int oldst;
		QCC_type_t *switchtype;

		breaks = num_breaks;
		cases = num_cases;


		QCC_PR_Expect ("(");

		conditional = 1;
		e = QCC_PR_Expression (TOP_PRIORITY, 0);
		conditional = 0;

		//expands

		//switch (CONDITION)
		//{
		//case 1:
		//	break;
		//case 2:
		//default:
		//	break;
		//}

		//to

		// x = CONDITION, goto start
		// l1:
		//	goto end
		// l2:
		// def:
		//	goto end
		//	goto end			P1
		// start:
		//	if (x == 1) goto l1;
		//	if (x == 2) goto l2;
		//	goto def
		// end:

		//x is emitted in an opcode, stored as a register that we cannot access later.
		//it should be possible to nest these.

		switchtype = e.cast;
		switch(switchtype->type)
		{
		case ev_float:
			op = OP_SWITCH_F;
			break;
		case ev_entity:	//whu???
			op = OP_SWITCH_E;
			break;
		case ev_vector:
			op = OP_SWITCH_V;
			break;
		case ev_string:
			op = OP_SWITCH_S;
			break;
		case ev_function:
			op = OP_SWITCH_FNC;
			break;
		default:	//err hmm.
			op = 0;
			break;
		}

		if (op)
			hcstyle = QCC_OPCodeValid(&pr_opcodes[op]);
		else
			hcstyle = false;

		QCC_ClobberDef(NULL);

		if (hcstyle)
			QCC_FreeTemp(QCC_PR_StatementFlags (&pr_opcodes[op], e, nullsref, &patch1, STFL_DISCARDRESULT));
		else
		{
			patch1 = QCC_Generate_OP_GOTO();	//fixme: rearrange this, to avoid the goto
			QCC_FreeTemp(e);
		}

		QCC_PR_Expect (")");	//close bracket is after we save the statement to mem (so debugger does not show the if statement as being on the line after

		oldst = numstatements;
		QCC_PR_ParseStatement ();

		//this is so that a missing goto at the end of your switch doesn't end up in the jumptable again
		if (oldst == numstatements || !QCC_StatementIsAJump(numstatements-1, numstatements-1) || QCC_AStatementJumpsTo(numstatements, pr_scope->code, numstatements))
		{
			patch2 = QCC_Generate_OP_GOTO();	//the P1 statement/the theyforgotthebreak statement.
//			QCC_PR_ParseWarning(0, "emitted goto");
		}
		else
		{
			patch2 = NULL;
//			QCC_PR_ParseWarning(0, "No goto");
		}

		if (hcstyle)
			patch1->b.ofs = &statements[numstatements] - patch1;	//the goto start part
		else
			patch1->a.ofs = &statements[numstatements] - patch1;	//the goto start part

		QCC_ForceUnFreeDef(e.sym);	//in the following code, e should still be live
		for (i = cases; i < num_cases; i++)
		{
			if (!pr_casesdef[i].cast)
			{
				if (defaultcase >= 0)
					QCC_PR_ParseError(ERR_MULTIPLEDEFAULTS, "Duplicated default case");
				defaultcase = i;
			}
			else
			{
				if (pr_casesdef[i].cast->type != e.cast->type)
					pr_casesdef[i] = QCC_SupplyConversion(pr_casesdef[i], e.cast->type, true);
				if (pr_casesdef2[i].cast)
				{
					if (pr_casesdef2[i].cast->type != e.cast->type)
						pr_casesdef2[i] = QCC_SupplyConversion(pr_casesdef2[i], e.cast->type, true);

					if (hcstyle)
					{
						QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_CASERANGE], pr_casesdef[i], pr_casesdef2[i], &patch3));
						patch3->c.ofs = &statements[pr_cases[i]] - patch3;
					}
					else
					{
						QCC_sref_t e3;

						if (e.cast->type == ev_float)
						{
							e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_GE_F], e, pr_casesdef[i], NULL, STFL_PRESERVEA);
							e3 = QCC_PR_StatementFlags (&pr_opcodes[OP_LE_F], e, pr_casesdef2[i], NULL, STFL_PRESERVEA);
							e2 = QCC_PR_Statement (&pr_opcodes[OP_AND_F], e2, e3, NULL);
							patch3 = QCC_Generate_OP_IF(e2, false);
							patch3->b.ofs = &statements[pr_cases[i]] - patch3;
						}
						else if (e.cast->type == ev_integer)
						{
							e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_GE_I], e, pr_casesdef[i], NULL, STFL_PRESERVEA);
							e3 = QCC_PR_StatementFlags (&pr_opcodes[OP_LE_I], e, pr_casesdef2[i], NULL, STFL_PRESERVEA);
							e2 = QCC_PR_Statement (&pr_opcodes[OP_AND_I], e2, e3, NULL);
							patch3 = QCC_Generate_OP_IF(e2, false);
							patch3->b.ofs = &statements[pr_cases[i]] - patch3;
						}
						else
							QCC_PR_ParseWarning(WARN_SWITCHTYPEMISMATCH, "switch caserange MUST be a float or integer");
					}
				}
				else
				{
					if (hcstyle)
					{
						QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_CASE], pr_casesdef[i], nullsref, &patch3));
						patch3->b.ofs = &statements[pr_cases[i]] - patch3;
					}
					else
					{
						const QCC_eval_t *eval = QCC_SRef_EvalConst(pr_casesdef[i]);
						if (!eval || eval->_int)
						{
							switch(e.cast->type)
							{
							case ev_float:
								e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_EQ_F], e, pr_casesdef[i], NULL, STFL_PRESERVEA);
								break;
							case ev_entity:	//whu???
								e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_EQ_E], e, pr_casesdef[i], NULL, STFL_PRESERVEA);
								break;
							case ev_vector:
								e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_EQ_V], e, pr_casesdef[i], NULL, STFL_PRESERVEA);
								break;
							case ev_string:
								e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_EQ_S], e, pr_casesdef[i], NULL, STFL_PRESERVEA);
								break;
							case ev_function:
								e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_EQ_FNC], e, pr_casesdef[i], NULL, STFL_PRESERVEA);
								break;
							case ev_field:
								e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_EQ_FNC], e, pr_casesdef[i], NULL, STFL_PRESERVEA);
								break;
							case ev_pointer:
								e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_EQ_P], e, pr_casesdef[i], NULL, STFL_PRESERVEA);
								break;
							case ev_integer:
								e2 = QCC_PR_StatementFlags (&pr_opcodes[OP_EQ_I], e, pr_casesdef[i], NULL, STFL_PRESERVEA);
								break;
							default:
								QCC_PR_ParseError(ERR_BADSWITCHTYPE, "Bad switch type");
								e2 = nullsref;
								break;
							}
							QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_IF_I], e2, nullsref, &patch3));
						}
						else
						{
							QCC_FreeTemp(pr_casesdef[i]);
							QCC_UnFreeTemp(e);
							patch3 = QCC_Generate_OP_IFNOT(e, false);
						}
						patch3->b.ofs = &statements[pr_cases[i]] - patch3;
					}
				}
			}
		}
		QCC_FreeTemp(e);
		if (defaultcase>=0)
		{
			patch3 = QCC_Generate_OP_GOTO();
			patch3->a.ofs = &statements[pr_cases[defaultcase]] - patch3;
		}

		num_cases = cases;


		patch3 = &statements[numstatements];
		if (patch2)
			patch2->a.ofs = patch3 - patch2;	//set P1 jump

		if (breaks != num_breaks)
		{
			for(i = breaks; i < num_breaks; i++)
			{
				patch2 = &statements[pr_breaks[i]];
				patch2->a.ofs = patch3 - patch2;
			}
			num_breaks = breaks;
		}
		return;
	}

	if (QCC_PR_CheckKeyword(keyword_asm, "asm"))
	{
		if (QCC_PR_CheckToken("{"))
		{
			while (!QCC_PR_CheckToken("}"))
				QCC_PR_ParseAsm ();
		}
		else
			QCC_PR_ParseAsm ();
		return;
	}

	//frikqcc-style labels
	if (QCC_PR_CheckToken(":"))
	{
		if (pr_token_type != tt_name)
		{
			QCC_PR_ParseError(ERR_BADLABELNAME, "invalid label name \"%s\"", pr_token);
			return;
		}

		for (i = 0; i < num_labels; i++)
			if (!STRNCMP(pr_labels[i].name, pr_token, sizeof(pr_labels[num_labels].name) -1))
			{
				QCC_PR_ParseWarning(WARN_DUPLICATELABEL, "Duplicate label %s", pr_token);
				QCC_PR_Lex();
				return;
			}

		if (num_labels >= max_labels)
		{
			max_labels += 8;
			pr_labels = realloc(pr_labels, sizeof(*pr_labels)*max_labels);
		}

		strncpy(pr_labels[num_labels].name, pr_token, sizeof(pr_labels[num_labels].name) -1);
		pr_labels[num_labels].lineno = pr_source_line;
		pr_labels[num_labels].statementno = numstatements;

		num_labels++;

//		QCC_PR_ParseWarning("Gotos are evil");
		QCC_PR_Lex();
		return;
	}
	if (QCC_PR_CheckKeyword(keyword_goto, "goto"))
	{
		if (pr_token_type != tt_name)
		{
			QCC_PR_ParseError(ERR_NOLABEL, "invalid label name \"%s\"", pr_token);
			return;
		}

		patch2 = QCC_Generate_OP_GOTO();

		QCC_PR_GotoStatement (patch2, pr_token);

//		QCC_PR_ParseWarning("Gotos are evil");
		QCC_PR_Lex();
		QCC_PR_Expect(";");
		return;
	}

	if (QCC_PR_CheckKeyword(keyword_break, "break"))
	{
		if (!STRCMP ("(", pr_token))
		{	//make sure it wasn't a call to the break function.
			QCC_PR_IncludeChunk("break(", true, NULL);
			QCC_PR_Lex();	//so it sees the break.
		}
		else
		{
			if (num_breaks >= max_breaks)
			{
				max_breaks += 8;
				pr_breaks = realloc(pr_breaks, sizeof(*pr_breaks)*max_breaks);
			}
			pr_breaks[num_breaks] = numstatements;
			QCC_PR_Statement (&pr_opcodes[OP_GOTO], nullsref, nullsref, NULL);
			num_breaks++;
			QCC_PR_Expect(";");
			return;
		}
	}
	if (QCC_PR_CheckKeyword(keyword_continue, "continue"))
	{
		if (num_continues >= max_continues)
		{
			max_continues += 8;
			pr_continues = realloc(pr_continues, sizeof(*pr_continues)*max_continues);
		}
		pr_continues[num_continues] = numstatements;
		QCC_PR_Statement (&pr_opcodes[OP_GOTO], nullsref, nullsref, NULL);
		num_continues++;
		QCC_PR_Expect(";");
		return;
	}
	if (QCC_PR_CheckKeyword(keyword_case, "case"))
	{
		if (num_cases >= max_cases)
		{
			max_cases += 8;
			pr_cases = realloc(pr_cases, sizeof(*pr_cases)*max_cases);
			pr_casesdef = realloc(pr_casesdef, sizeof(*pr_casesdef)*max_cases);
			pr_casesdef2 = realloc(pr_casesdef2, sizeof(*pr_casesdef2)*max_cases);
		}
		pr_cases[num_cases] = numstatements;
		pr_casesdef[num_cases] = QCC_PR_Expression (TOP_PRIORITY, EXPR_DISALLOW_COMMA);
		if (QCC_PR_CheckToken(".."))
		{
			const QCC_eval_t *evalmin, *evalmax;
			pr_casesdef2[num_cases] = QCC_PR_Expression (TOP_PRIORITY, EXPR_DISALLOW_COMMA);
			pr_casesdef2[num_cases] = QCC_SupplyConversion(pr_casesdef2[num_cases], pr_casesdef[num_cases].cast->type, true);

			evalmin = QCC_SRef_EvalConst(pr_casesdef[num_cases]);
			evalmax = QCC_SRef_EvalConst(pr_casesdef2[num_cases]);
			if (evalmin && evalmax)
			{
				if ((pr_casesdef[num_cases].cast->type == ev_integer && evalmin->_int >= evalmax->_int) ||
					(pr_casesdef[num_cases].cast->type == ev_float && evalmin->_float >= evalmax->_float))
					QCC_PR_ParseError(ERR_CASENOTIMMEDIATE, "Caserange statement uses backwards range\n");
			}
		}
		else
			pr_casesdef2[num_cases] = nullsref;

		if (numstatements != pr_cases[num_cases])
			QCC_PR_ParseError(ERR_CASENOTIMMEDIATE, "Case statements may not use formulas\n");
		num_cases++;
		QCC_PR_Expect(":");
		return;
	}
	if (QCC_PR_CheckKeyword(keyword_default, "default"))
	{
		if (num_cases >= max_cases)
		{
			max_cases += 8;
			pr_cases = realloc(pr_cases, sizeof(*pr_cases)*max_cases);
			pr_casesdef = realloc(pr_casesdef, sizeof(*pr_casesdef)*max_cases);
			pr_casesdef2 = realloc(pr_casesdef2, sizeof(*pr_casesdef2)*max_cases);
		}
		pr_cases[num_cases] = numstatements;
		pr_casesdef[num_cases] = nullsref;
		pr_casesdef2[num_cases] = nullsref;
		num_cases++;
		QCC_PR_Expect(":");
		return;
	}

	if (QCC_PR_CheckKeyword(keyword_thinktime, "thinktime"))
	{
		QCC_sref_t nextthink;
		QCC_sref_t time;
		e = QCC_PR_Expression (TOP_PRIORITY, 0);
		QCC_PR_Expect(":");
		e2 = QCC_PR_Expression (TOP_PRIORITY, 0);
		if (e.cast->type != ev_entity || e2.cast->type != ev_float)
			QCC_PR_ParseError(ERR_THINKTIMETYPEMISMATCH, "thinktime type mismatch");

		QCC_SupplyConversion(e2, ev_float, true);

		if (QCC_OPCodeValid(&pr_opcodes[OP_THINKTIME]))
			QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_THINKTIME], e, e2, NULL));
		else
		{
			nextthink = QCC_PR_GetSRef(NULL, "nextthink", NULL, false, 0, false);
			if (!nextthink.cast)
				QCC_PR_ParseError (ERR_UNKNOWNVALUE, "Unknown value \"%s\"", "nextthink");
			time = QCC_PR_GetSRef(type_float, "time", NULL, false, 0, false);
			if (!time.cast)
				QCC_PR_ParseError (ERR_UNKNOWNVALUE, "Unknown value \"%s\"", "time");
			nextthink = QCC_PR_Statement(&pr_opcodes[OP_ADDRESS], e, nextthink, NULL);
			time = QCC_PR_Statement(&pr_opcodes[OP_ADD_F], time, e2, NULL);
			QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_STOREP_F], time, nextthink, NULL));
		}
		QCC_PR_Expect(";");
		return;
	}
	if (QCC_PR_CheckToken(";"))
	{
		int osl = pr_source_line;
		pr_source_line = statementstart;
		if (!expandedemptymacro)
		{
			QCC_PR_ParseWarning(WARN_POINTLESSSTATEMENT, "Hanging ';'");
			while (QCC_PR_CheckToken(";"))
				;
		}
		pr_source_line = osl;
		return;
	}

	//C-style labels.
	if (pr_token_type == tt_name && pr_file_p[0] == ':' && pr_file_p[1] != ':')
	{
		if (pr_token_type != tt_name)
		{
			QCC_PR_ParseError(ERR_BADLABELNAME, "invalid label name \"%s\"", pr_token);
			return;
		}

		for (i = 0; i < num_labels; i++)
			if (!STRNCMP(pr_labels[i].name, pr_token, sizeof(pr_labels[num_labels].name) -1))
			{
				QCC_PR_ParseWarning(WARN_DUPLICATELABEL, "Duplicate label %s", pr_token);
				QCC_PR_Lex();
				return;
			}

		if (num_labels >= max_labels)
		{
			max_labels += 8;
			pr_labels = realloc(pr_labels, sizeof(*pr_labels)*max_labels);
		}

		strncpy(pr_labels[num_labels].name, pr_token, sizeof(pr_labels[num_labels].name) -1);
		pr_labels[num_labels].lineno = pr_source_line;
		pr_labels[num_labels].statementno = numstatements;

		num_labels++;

//		QCC_PR_ParseWarning("Gotos are evil");
		QCC_PR_Lex();
		QCC_PR_Expect(":");
		return;
	}

//	qcc_functioncalled=0;

	QCC_PR_DiscardExpression (TOP_PRIORITY, 0);
	expandedemptymacro = false;
	QCC_PR_Expect (";");

//	qcc_functioncalled=false;
}


/*
==============
PR_ParseState

States are special functions made for convenience.  They automatically
set frame, nextthink (implicitly), and think (allowing forward definitions).

// void() name = [framenum, nextthink] {code}
// expands to:
// function void name ()
// {
//		self.frame=framenum;
//		self.nextthink = time + 0.1;
//		self.think = nextthink
//		<code>
// };
==============
*/
void QCC_PR_ParseState (void)
{
	QCC_sref_t	s1, def;

	if (QCC_PR_CheckToken("++") || QCC_PR_CheckToken("--"))
	{
		s1 = QCC_PR_ParseImmediate ();
		QCC_PR_Expect("..");
		def = QCC_PR_ParseImmediate ();
		QCC_PR_Expect ("]");

		if (s1.cast->type != ev_float || def.cast->type != ev_float)
			QCC_PR_ParseError(ERR_STATETYPEMISMATCH, "state type mismatch");


		if (QCC_OPCodeValid(&pr_opcodes[OP_CSTATE]))
			QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_CSTATE], s1, def, NULL));
		else
		{
			QCC_statement_t *patch1, *entercyc, *fwd, *back;
			QCC_sref_t t1, t2;
			QCC_sref_t framef, frame;
			QCC_sref_t self;
			QCC_sref_t cycle_wrapped;

			self = QCC_PR_GetSRef(type_entity, "self", NULL, false, 0, false);
			framef = QCC_PR_GetSRef(NULL, "frame", NULL, false, 0, false);
			cycle_wrapped = QCC_PR_GetSRef(type_float, "cycle_wrapped", NULL, false, 0, false);

			frame = QCC_PR_Statement(&pr_opcodes[OP_LOAD_F], self, framef, NULL);
			if (cycle_wrapped.cast)
				QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_STORE_F], QCC_MakeFloatConst(0), cycle_wrapped, NULL));

			//make sure the frame is within the bounds given.
			t1 = QCC_PR_StatementFlags(&pr_opcodes[OP_LT_F], frame, s1, NULL, STFL_PRESERVEA);
			t2 = QCC_PR_StatementFlags(&pr_opcodes[OP_GT_F], frame, def, NULL, STFL_PRESERVEA);
			t1 = QCC_PR_Statement(&pr_opcodes[OP_OR_F], t1, t2, NULL);
			patch1 = QCC_Generate_OP_IFNOT(t1, false);
				QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], s1, frame, NULL, STFL_PRESERVEB));
				entercyc = QCC_Generate_OP_GOTO();
			patch1->b.ofs = &statements[numstatements] - patch1;

			t1 = QCC_PR_Statement(&pr_opcodes[OP_GE_F], def, s1, NULL);
			fwd = QCC_Generate_OP_IFNOT(t1, false);	//this block is the 'it's in a forwards direction'
			{
				QCC_PR_SimpleStatement(&pr_opcodes[OP_ADD_F], frame, QCC_MakeFloatConst(1), frame, false);
				t1 = QCC_PR_Statement(&pr_opcodes[OP_GT_F], frame, def, NULL);
				patch1 = QCC_Generate_OP_IFNOT(t1, false);
				{
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], s1, frame, NULL, STFL_PRESERVEB));
					if (cycle_wrapped.cast)
						QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_STORE_F], QCC_MakeFloatConst(1), cycle_wrapped, NULL));
				}
				patch1->b.ofs = &statements[numstatements] - patch1;
			}
			back = QCC_Generate_OP_GOTO();
			fwd->b.ofs = &statements[numstatements] - fwd;
			{
				//reverse animation.
				QCC_PR_SimpleStatement(&pr_opcodes[OP_SUB_F], frame, QCC_MakeFloatConst(1), frame, false);
				t1 = QCC_PR_StatementFlags(&pr_opcodes[OP_LT_F], frame, s1, NULL, STFL_PRESERVEA);
				patch1 = QCC_Generate_OP_IFNOT(t1, false);
				{
					QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], def, frame, NULL, STFL_PRESERVEB));
					if (cycle_wrapped.cast)
						QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_STORE_F], QCC_MakeFloatConst(1), cycle_wrapped, NULL));
				}
				patch1->b.ofs = &statements[numstatements] - patch1;
			}
			back->b.ofs = &statements[numstatements] - back;

			/*out of range*/entercyc->b.ofs = &statements[numstatements] - entercyc;
			//self.frame = frame happens with the normal state opcode.
			QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_STATE], frame, QCC_MakeSRef(pr_scope->def, 0, pr_scope->type), NULL));
		}
		return;
	}

	s1 = QCC_PR_Expression (TOP_PRIORITY, EXPR_DISALLOW_COMMA);
	s1 = QCC_SupplyConversion(s1, ev_float, true);

	if (!QCC_PR_CheckToken (","))
		QCC_PR_ParseWarning(WARN_UNEXPECTEDPUNCT, "missing comma in state definition");

	pr_assumetermtype = type_function;
	def = QCC_PR_Expression (TOP_PRIORITY, EXPR_DISALLOW_COMMA);
	if (typecmp(def.cast, type_function))
	{
		if (!QCC_SRef_IsNull(def))
		{
			char typebuf1[256];
			char typebuf2[256];
			QCC_PR_ParseErrorPrintSRef (ERR_TYPEMISMATCH, def, "Type mismatch: %s, should be %s", TypeName(def.cast, typebuf1, sizeof(typebuf1)), TypeName(type_function, typebuf2, sizeof(typebuf2)));
		}
	}
	pr_assumetermtype = NULL;

	QCC_PR_Expect ("]");

	QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_STATE], s1, def, NULL));
}

void QCC_PR_ParseAsm(void)
{
	QCC_statement_t *patch1;
	int op, p;
	QCC_sref_t a, b, c;

	if (QCC_PR_CheckKeyword(keyword_local, "local"))
	{
		QCC_PR_ParseDefs (NULL);
		return;
	}

	for (op = 0; op < OP_NUMOPS; op++)
	{
		if (!STRCMP(pr_token, pr_opcodes[op].opname))
		{
			QCC_PR_Lex();
			if (pr_opcodes[op].priority==-1 && pr_opcodes[op].associative!=ASSOC_LEFT)
			{
				if (pr_opcodes[op].type_a==NULL)
				{
					patch1 = QCC_PR_SimpleStatement(&pr_opcodes[op], nullsref, nullsref, nullsref, true);

					if (pr_token_type == tt_name)
					{
						QCC_PR_GotoStatement(patch1, QCC_PR_ParseName());
					}
					else
					{
						p = (int)pr_immediate._float;
						patch1->a.ofs = p;
					}

					QCC_PR_Lex();
				}
				else if (pr_opcodes[op].type_b==NULL)
				{
					a = QCC_PR_ParseValue(pr_classtype, false, false, true);
					patch1 = QCC_PR_SimpleStatement(&pr_opcodes[op], a, nullsref, nullsref, true);

					if (pr_token_type == tt_name)
					{
						QCC_PR_GotoStatement(patch1, QCC_PR_ParseName());
					}
					else
					{
						p = (int)pr_immediate._float;
						patch1->b.ofs = (int)p;
					}

					QCC_PR_Lex();
				}
				else
				{
					a = QCC_PR_ParseValue(pr_classtype, false, false, true);
					b = QCC_PR_ParseValue(pr_classtype, false, false, true);
					patch1 = QCC_PR_SimpleStatement(&pr_opcodes[op], a, b, nullsref, true);

					if (pr_token_type == tt_name)
					{
						QCC_PR_GotoStatement(patch1, QCC_PR_ParseName());
					}
					else
					{
						p = (int)pr_immediate._float;
						patch1->c.ofs = p;
					}

					QCC_PR_Lex();
				}
			}
			else
			{
				if (pr_opcodes[op].type_a != &type_void)
					a = QCC_PR_ParseValue(pr_classtype, false, false, true);
				else
					a=nullsref;
				if (pr_opcodes[op].type_b != &type_void)
					b = QCC_PR_ParseValue(pr_classtype, false, false, true);
				else
					b=nullsref;
				if (pr_opcodes[op].associative==ASSOC_LEFT && pr_opcodes[op].type_c != &type_void)
					c = QCC_PR_ParseValue(pr_classtype, false, false, true);
				else
					c=nullsref;

				QCC_PR_SimpleStatement(&pr_opcodes[op], a, b, c, true);
			}

			QCC_PR_Expect(";");
			return;
		}
	}
	QCC_PR_ParseError(ERR_BADOPCODE, "Bad op code name %s", pr_token);
}

static pbool QCC_FuncJumpsTo(int first, int last, int statement)
{
	int st;
	for (st = first; st < last; st++)
	{
		if (pr_opcodes[statements[st].op].type_a == NULL)
		{
			if (st + (signed)statements[st].a.ofs == statement)
			{
				if (st != first)
				{
					if (statements[st-1].op == OP_RETURN)
						continue;
					if (statements[st-1].op == OP_DONE)
						continue;
					return true;
				}
			}
		}
		if (pr_opcodes[statements[st].op].type_b == NULL)
		{
			if (st + (signed)statements[st].b.ofs == statement)
			{
				if (st != first)
				{
					if (statements[st-1].op == OP_RETURN)
						continue;
					if (statements[st-1].op == OP_DONE)
						continue;
					return true;
				}
			}
		}
		if (pr_opcodes[statements[st].op].type_c == NULL)
		{
			if (st + (signed)statements[st].c.ofs == statement)
			{
				if (st != first)
				{
					if (statements[st-1].op == OP_RETURN)
						continue;
					if (statements[st-1].op == OP_DONE)
						continue;
					return true;
				}
			}
		}
	}
	return false;
}
/*
static pbool QCC_FuncJumpsToRange(int first, int last, int firstr, int lastr)
{
	int st;
	for (st = first; st < last; st++)
	{
		if (pr_opcodes[statements[st].op].type_a == NULL)
		{
			if (st + (signed)statements[st].a >= firstr && st + (signed)statements[st].a <= lastr)
			{
				if (st != first)
				{
					if (statements[st-1].op == OP_RETURN)
						continue;
					if (statements[st-1].op == OP_DONE)
						continue;
					return true;
				}
			}
		}
		if (pr_opcodes[statements[st].op].type_b == NULL)
		{
			if (st + (signed)statements[st].b >= firstr && st + (signed)statements[st].b <= lastr)
			{
				if (st != first)
				{
					if (statements[st-1].op == OP_RETURN)
						continue;
					if (statements[st-1].op == OP_DONE)
						continue;
					return true;
				}
			}
		}
		if (pr_opcodes[statements[st].op].type_c == NULL)
		{
			if (st + (signed)statements[st].c >= firstr && st + (signed)statements[st].c <= lastr)
			{
				if (st != first)
				{
					if (statements[st-1].op == OP_RETURN)
						continue;
					if (statements[st-1].op == OP_DONE)
						continue;
					return true;
				}
			}
		}
	}
	return false;
}
*/
#if 0
void QCC_CompoundJumps(int first, int last)
{
	//jumps to jumps are reordered so they become jumps to the final target.
	int statement;
	int st;
	for (st = first; st < last; st++)
	{
		if (pr_opcodes[statements[st].op].type_a == NULL)
		{
			statement = st + (signed)statements[st].a;
			if (statements[statement].op == OP_RETURN || statements[statement].op == OP_DONE)
			{	//goto leads to return. Copy the command out to remove the goto.
				statements[st].op = statements[statement].op;
				statements[st].a = statements[statement].a;
				statements[st].b = statements[statement].b;
				statements[st].c = statements[statement].c;
				optres_compound_jumps++;
			}
			while (statements[statement].op == OP_GOTO)
			{
				statements[st].a = statement+statements[statement].a - st;
				statement = st + (signed)statements[st].a;
				optres_compound_jumps++;
			}
		}
		if (pr_opcodes[statements[st].op].type_b == NULL)
		{
			statement = st + (signed)statements[st].b;
			while (statements[statement].op == OP_GOTO)
			{
				statements[st].b = statement+statements[statement].a - st;
				statement = st + (signed)statements[st].b;
				optres_compound_jumps++;
			}
		}
		if (pr_opcodes[statements[st].op].type_c == NULL)
		{
			statement = st + (signed)statements[st].c;
			while (statements[statement].op == OP_GOTO)
			{
				statements[st].c = statement+statements[statement].a - st;
				statement = st + (signed)statements[st].c;
				optres_compound_jumps++;
			}
		}
	}
}
#else
void QCC_CompoundJumps(int first, int last)
{
	//jumps to jumps are reordered so they become jumps to the final target.
	int statement;
	int st;
	int infloop;
	for (st = first; st < last; st++)
	{
		if (pr_opcodes[statements[st].op].type_a == NULL)
		{
			statement = st + (signed)statements[st].a.ofs;
			if (statements[statement].op == OP_RETURN || statements[statement].op == OP_DONE)
			{	//goto leads to return. Copy the command out to remove the goto.
				statements[st] = statements[statement];
				optres_compound_jumps++;
			}
			infloop = 1000;
			while (statements[statement].op == OP_GOTO)
			{
				if (!infloop--)
				{
					QCC_PR_ParseWarning(0, "Infinate loop detected");
					break;
				}
				statements[st].a.ofs = (statement+statements[statement].a.ofs - st);
				statement = st + (signed)statements[st].a.ofs;
				optres_compound_jumps++;
			}
		}
		if (pr_opcodes[statements[st].op].type_b == NULL)
		{
			statement = st + (signed)statements[st].b.ofs;
			infloop = 1000;
			while (statements[statement].op == OP_GOTO)
			{
				if (!infloop--)
				{
					QCC_PR_ParseWarning(0, "Infinate loop detected");
					break;
				}
				statements[st].b.ofs = (statement+statements[statement].a.ofs - st);
				statement = st + (signed)statements[st].b.ofs;
				optres_compound_jumps++;
			}
		}
		if (pr_opcodes[statements[st].op].type_c == NULL)
		{
			statement = st + (signed)statements[st].c.ofs;
			infloop = 1000;
			while (statements[statement].op == OP_GOTO)
			{
				if (!infloop--)
				{
					QCC_PR_ParseWarning(0, "Infinate loop detected");
					break;
				}
				statements[st].c.ofs = (statement+statements[statement].a.ofs - st);
				statement = st + (signed)statements[st].c.ofs;
				optres_compound_jumps++;
			}
		}
	}
}
#endif

void QCC_CheckForDeadAndMissingReturns(int first, int last, int rettype)
{
	QCC_function_t *fnc;
	int st, st2;

	if (statements[last-1].op == OP_DONE)
		last--;	//don't want the done

	if (rettype != ev_void)
		if (statements[last-1].op != OP_RETURN)
		{
			if (statements[last-1].op != OP_GOTO || (signed)statements[last-1].a.ofs > 0)
			{
				QCC_PR_ParseWarning(WARN_MISSINGRETURN, "%s: not all control paths return a value", pr_scope->name );
				return;
			}
		}

	for (st = first; st < last; st++)
	{
		if (statements[st].op == OP_RETURN || statements[st].op == OP_GOTO)
		{
			st++;
			if (st == last)
				continue;	//erm... end of function doesn't count as unreachable.

			if (!opt_compound_jumps)
			{	//we can ignore single statements like these without compound jumps (compound jumps correctly removes all).
				if (statements[st].op == OP_GOTO)	//inefficient compiler, we can ignore this.
					continue;
				if (statements[st].op == OP_DONE)	//inefficient compiler, we can ignore this.
					continue;
			}
			//always allow random return statements, because people like putting returns after switches even when all the switches have a return.
			if (statements[st].op == OP_RETURN)	//inefficient compiler, we can ignore this.
				continue;

			//check for embedded functions. FIXME: should generate outside of the parent function.
			for (fnc = pr_scope+1; fnc < &functions[numfunctions]; fnc++)
			{
				if (fnc->code == st)
					break;
			}
			if (fnc < &functions[numfunctions])
				continue;

			//make sure something goes to just after this return.
			for (st2 = first; st2 < last; st2++)
			{
				if (pr_opcodes[statements[st2].op].associative == ASSOC_RIGHT)
				{
					if (pr_opcodes[statements[st2].op].type_a == NULL)
					{
						if (st2 + (signed)statements[st2].a.ofs == st)
							break;
					}
					if (pr_opcodes[statements[st2].op].type_b == NULL)
					{
						if (st2 + (signed)statements[st2].b.ofs == st)
							break;
					}
					if (pr_opcodes[statements[st2].op].type_c == NULL)
					{
						if (st2 + (signed)statements[st2].c.ofs == st)
							break;
					}
				}
			}
			if (st2 == last)
			{
				QCC_PR_Warning(WARN_UNREACHABLECODE, pr_scope->file, statements[st].linenum, "%s: contains unreachable code (line %i)", pr_scope->name, statements[st].linenum);
			}
			continue;
		}
		if (rettype != ev_void && pr_opcodes[statements[st].op].associative == ASSOC_RIGHT)
		{
			if (pr_opcodes[statements[st].op].type_a == NULL)
			{
				if (st + (signed)statements[st].a.ofs == last)
				{
					QCC_PR_ParseWarning(WARN_MISSINGRETURN, "%s: not all control paths return a value", pr_scope->name );
					return;
				}
			}
			if (pr_opcodes[statements[st].op].type_b == NULL)
			{
				if (st + (signed)statements[st].b.ofs == last)
				{
					QCC_PR_ParseWarning(WARN_MISSINGRETURN, "%s: not all control paths return a value", pr_scope->name );
					return;
				}
			}
			if (pr_opcodes[statements[st].op].type_c == NULL)
			{
				if (st + (signed)statements[st].c.ofs == last)
				{
					QCC_PR_ParseWarning(WARN_MISSINGRETURN, "%s: not all control paths return a value", pr_scope->name );
					return;
				}
			}
		}
	}
}

pbool QCC_StatementIsAJump(int stnum, int notifdest)	//only the unconditionals.
{
	if (statements[stnum].op == OP_RETURN)
		return true;
	if (statements[stnum].op == OP_DONE)
		return true;
	if (statements[stnum].op == OP_GOTO)
		if ((int)statements[stnum].a.ofs != notifdest)
			return true;
	return false;
}

int QCC_AStatementJumpsTo(int targ, int first, int last)
{
	int st;
	for (st = first; st < last; st++)
	{
		if (pr_opcodes[statements[st].op].type_a == NULL)
		{
			if (st + (signed)statements[st].a.ofs == targ && statements[st].a.ofs)
			{
				return true;
			}
		}
		if (pr_opcodes[statements[st].op].type_b == NULL)
		{
			if (st + (signed)statements[st].b.ofs == targ)
			{
				return true;
			}
		}
		if (pr_opcodes[statements[st].op].type_c == NULL)
		{
			if (st + (signed)statements[st].c.ofs == targ)
			{
				return true;
			}
		}
	}

	for (st = 0; st < num_labels; st++)	//assume it's used.
	{
		if (pr_labels[st].statementno == targ)
			return true;
	}

	for (st = 0; st < num_cases; st++)	//assume it's used.
	{
		if (pr_cases[st] == targ)
			return true;
	}

	return false;
}
/*
//goes through statements, if it sees a matching statement earlier, it'll strim out the current.
void QCC_CommonSubExpressionRemoval(int first, int last)
{
	int cur;	//the current
	int prev;	//the earlier statement
	for (cur = last-1; cur >= first; cur--)
	{
		if (pr_opcodes[statements[cur].op].priority == -1)
			continue;
		for (prev = cur-1; prev >= first; prev--)
		{
			if (statements[prev].op >= OP_CALL0 && statements[prev].op <= OP_CALL8)
			{
				optres_test1++;
				break;
			}
			if (statements[prev].op >= OP_CALL1H && statements[prev].op <= OP_CALL8H)
			{
				optres_test1++;
				break;
			}
			if (pr_opcodes[statements[prev].op].right_associative)
			{	//make sure no changes to var_a occur.
				if (statements[prev].b == statements[cur].a)
				{
					optres_test2++;
					break;
				}
				if (statements[prev].b == statements[cur].b && !pr_opcodes[statements[cur].op].right_associative)
				{
					optres_test2++;
					break;
				}
			}
			else
			{
				if (statements[prev].c == statements[cur].a)
				{
					optres_test2++;
					break;
				}
				if (statements[prev].c == statements[cur].b && !pr_opcodes[statements[cur].op].right_associative)
				{
					optres_test2++;
					break;
				}
			}

			if (statements[prev].op == statements[cur].op)
				if (statements[prev].a == statements[cur].a)
					if (statements[prev].b == statements[cur].b)
						if (statements[prev].c == statements[cur].c)
						{
							if (!QCC_FuncJumpsToRange(first, last, prev, cur))
							{
								statements[cur].op = OP_STORE_F;
								statements[cur].a = 28;
								statements[cur].b = 28;
								optres_comexprremoval++;
							}
							else
								optres_test1++;
							break;
						}
		}
	}
}
*/

#define OpAssignsToA(op) false

//follow branches (by recursing).
//stop on first read(error, return statement) or write(no error, return -1)
//end-of-block returns 0, done/return/goto returns -2
int QCC_CheckOneUninitialised(int firststatement, int laststatement, QCC_def_t *def, unsigned int min, unsigned int max)
{
	int ret;
	int i;
	QCC_statement_t *st;

	for (i = firststatement; i < laststatement; i++)
	{
		st = &statements[i];

		if (st->op == OP_DONE || st->op == OP_RETURN)
		{
			if (st->a.sym == def && st->a.ofs >= min && st->a.ofs < max)
				return i;
			return -2;
		}

		if (st->op == OP_GLOBALADDRESS && (st->a.sym == def || (st->a.sym == def->symbolheader)))
			return -1;	//assume taking a pointer to it is an initialisation.

//		this code catches gotos, but can cause issues with while statements.
//		if (st->op == OP_GOTO && (int)st->a < 1)
//			return -2;

		if (pr_opcodes[st->op].type_a)
		{
			if (st->a.sym == def && st->a.ofs >= min && st->a.ofs < max)
			{
				if (OpAssignsToA(st->op))
					return -1;

				return i;
			}
		}
		else if (pr_opcodes[st->op].associative == ASSOC_RIGHT && (int)st->a.ofs > 0)
		{
			int jump = i + (int)st->a.ofs;
			ret = QCC_CheckOneUninitialised(i + 1, jump, def, min, max);
			if (ret > 0)
				return ret;
			i = jump-1;
		}

		if (pr_opcodes[st->op].type_b)
		{
			if (st->b.sym == def && st->b.ofs >= min && st->b.ofs < max)
			{
				if (OpAssignsToB(st->op))
					return -1;

				return i;
			}
		}
		else if (pr_opcodes[st->op].associative == ASSOC_RIGHT && (int)st->b.ofs > 0)
		{
			int jump = i + (int)st->b.ofs;
			//check if there's an else.
			st = &statements[jump-1];
			if (st->op == OP_GOTO && (int)st->a.ofs > 0)
			{
				int jump2 = jump-1 + st->a.ofs;
				int rett = QCC_CheckOneUninitialised(i + 1, jump - 1, def, min, max);
				if (rett > 0)
					return rett;
				ret = QCC_CheckOneUninitialised(jump, jump2, def, min, max);
				if (ret > 0)
					return ret;
				if (rett < 0 && ret < 0)
					return (rett == ret)?ret:-1;	//inited or aborted in both, don't need to continue along this branch
				i = jump2-1;
			}
			else
			{
				ret = QCC_CheckOneUninitialised(i + 1, jump, def, min, max);
				if (ret > 0)
					return ret;
				i = jump-1;
			}
			continue;
		}

		if (pr_opcodes[st->op].type_c && st->c.sym == def && st->c.ofs >= min && st->c.ofs < max)
		{
			if (OpAssignsToC(st->op))
				return -1;

			return i;
		}
		else if (pr_opcodes[st->op].associative == ASSOC_RIGHT && (int)st->c.ofs > 0)
		{
			int jump = i + (int)st->c.ofs;
			ret = QCC_CheckOneUninitialised(i + 1, jump, def, min, max);
			if (ret > 0)
				return ret;
			i = jump-1;

			continue;
		}
	}

	return 0;
}

pbool QCC_CheckUninitialised(int firststatement, int laststatement)
{
	QCC_def_t *local;
	unsigned int i;
	pbool result = false;
	unsigned int paramend = FIRST_LOCAL;
	QCC_type_t *type = pr_scope->type;
	int err;

	//assume all, because we don't care for optimisations once we know we're not going to compile anything (removes warning about uninitialised unknown variables/typos).
	if (pr_error_count)
		return true;

	for (i = 0; i < type->num_parms; i++)
	{
		paramend += type->params[i].type->size;
	}

	for (local = pr.local_head.nextlocal; local; local = local->nextlocal)
	{
		if (local->constant)
			continue;	//will get some other warning, so we don't care.
		if (local->symbolheader != local)
			continue;	//ignore slave symbols, cos they're not interesting and should have been checked as part of the parent.
		if (local->ofs < paramend)
			continue;
		err = QCC_CheckOneUninitialised(firststatement, laststatement, local, local->ofs, local->ofs + local->type->size * (local->arraysize?local->arraysize:1));
		if (err > 0)
		{
			QCC_PR_Warning(WARN_UNINITIALIZED, strings+s_file, statements[err].linenum, "Potentially uninitialised variable %s", local->name);
			result = true;
//			break;
		}
	}

	return result;
}

void QCC_Marshal_Locals(int firststatement, int laststatement)
{
	QCC_def_t *local;
	pbool error = false;



	if (!pr.local_head.nextlocal)	//nothing to marshal
	{
		return;
	}

	if (!opt_locals_overlapping)
	{
		if (qccwarningaction[WARN_UNINITIALIZED])
			QCC_CheckUninitialised(firststatement, laststatement);	//still need to call it for warnings, but if those warnings are off we can skip the cost
		error = true;	//always use the legacy behaviour
	}
	else if (QCC_CheckUninitialised(firststatement, laststatement))
	{
		error = true;
//		QCC_PR_Note(ERR_INTERNAL, strings+s_file, pr_source_line, "Not overlapping locals from %s due to uninitialised locals", pr_scope->name);
	}
	else
	{
		//make sure we're allowed to marshall this function's locals
		for (local = pr.local_head.nextlocal; local; local = local->nextlocal)
		{
			//FIXME: check for uninitialised locals.
			//these matter when the function goes recursive (and locals marshalling counts as recursive every time).
			if (local->symboldata[local->ofs]._int)
			{
				QCC_PR_Note(ERR_INTERNAL, strings+local->s_file, local->s_line, "Marshaling non-const initialised %s", local->name);
				error = true;
			}

			if (local->constant)
			{
				QCC_PR_Note(ERR_INTERNAL, strings+local->s_file, local->s_line, "Marshaling const %s", local->name);
				error = true;
			}
		}
	}

	if (error)
		pr_scope->privatelocals = true;
	else
		pr_scope->privatelocals = false;
	pr_scope->firstlocal = pr.local_head.nextlocal;
	pr.local_head.nextlocal = NULL;
	pr.local_tail = &pr.local_head;
}

#ifdef WRITEASM
void QCC_WriteGUIAsmFunction(QCC_function_t	*sc, unsigned int firststatement)
{
	unsigned int			i;
//	QCC_type_t *type;
	char typebuf[512];

	char line[2048];
	extern int currentsourcefile;

//	type = sc->type;

	for (i = firststatement; i < (unsigned int)numstatements; i++)
	{
		line[0] = 0;
		QC_strlcat(line, pr_opcodes[statements[i].op].opname, sizeof(line));
		if (pr_opcodes[statements[i].op].type_a != &type_void)
		{
//			if (strlen(pr_opcodes[statements[i].op].opname)<6)
//				QC_strlcat(line, "  ", sizeof(line));
			if (pr_opcodes[statements[i].op].type_a)
				QC_snprintfz(typebuf, sizeof(typebuf), "  %s", QCC_VarAtOffset(statements[i].a, (*pr_opcodes[statements[i].op].type_a)->size));
			else
				QC_snprintfz(typebuf, sizeof(typebuf), "  %i", statements[i].a.ofs);
			QC_strlcat(line, typebuf, sizeof(line));
			if (pr_opcodes[statements[i].op].type_b != &type_void)
			{
				if (pr_opcodes[statements[i].op].type_b)
					QC_snprintfz(typebuf, sizeof(typebuf), ",  %s", QCC_VarAtOffset(statements[i].b, (*pr_opcodes[statements[i].op].type_b)->size));
				else
					QC_snprintfz(typebuf, sizeof(typebuf), ",  %i", statements[i].b.ofs);
				QC_strlcat(line, typebuf, sizeof(line));
				if (pr_opcodes[statements[i].op].type_c != &type_void && (pr_opcodes[statements[i].op].associative==ASSOC_LEFT || statements[i].c.cast))
				{
					if (pr_opcodes[statements[i].op].type_c)
						QC_snprintfz(typebuf, sizeof(typebuf), ",  %s", QCC_VarAtOffset(statements[i].c, (*pr_opcodes[statements[i].op].type_c)->size));
					else
						QC_snprintfz(typebuf, sizeof(typebuf), ",  %i", statements[i].c.ofs);
					QC_strlcat(line, typebuf, sizeof(line));
				}
			}
			else
			{
				if (pr_opcodes[statements[i].op].type_c != &type_void)
				{
					if (pr_opcodes[statements[i].op].type_c)
						QC_snprintfz(typebuf, sizeof(typebuf), ",  %s", QCC_VarAtOffset(statements[i].c, (*pr_opcodes[statements[i].op].type_c)->size));
					else
						QC_snprintfz(typebuf, sizeof(typebuf), ",  %i", statements[i].c.ofs);
					QC_strlcat(line, typebuf, sizeof(line));
				}
			}
		}	
		else
		{
			if (pr_opcodes[statements[i].op].type_c != &type_void)
			{
				if (pr_opcodes[statements[i].op].type_c)
					QC_snprintfz(typebuf, sizeof(typebuf), "  %s", QCC_VarAtOffset(statements[i].c, (*pr_opcodes[statements[i].op].type_c)->size));
				else
					QC_snprintfz(typebuf, sizeof(typebuf), "  %i", statements[i].c.ofs);
				QC_strlcat(line, typebuf, sizeof(line));
			}
		}
		if (currentsourcefile)
			printf("code: %s:%i: %i:%s;\n", sc->file, statements[i].linenum, currentsourcefile, line);
		else
			printf("code: %s:%i: %s;\n", sc->file, statements[i].linenum, line);
	}
}

void QCC_WriteAsmFunction(QCC_function_t	*sc, unsigned int firststatement, QCC_def_t *firstparm)
{
	unsigned int			i;
	QCC_def_t *o = firstparm;
	QCC_type_t *type;
	char typebuf[512];

	if (flag_guiannotate)
		QCC_WriteGUIAsmFunction(sc, firststatement);

	if (!asmfile)
		return;

	type = sc->type;
	fprintf(asmfile, "%s(", TypeName(type->aux_type, typebuf, sizeof(typebuf)));
	for (o = pr.local_head.nextlocal, i = 0; i < type->num_parms; i++)
	{
		if (i)
			fprintf(asmfile, ", ");

		if (o)
		{
			fprintf(asmfile, "%s %s", TypeName(o->type, typebuf, sizeof(typebuf)), o->name);
			o = o->nextlocal;
		}
		else
			fprintf(asmfile, "%s", TypeName(type->params[i].type, typebuf, sizeof(typebuf)));
	}

	fprintf(asmfile, ") %s = asm\n{\n", sc->name);

	QCC_fprintfLocals(asmfile, o);

	for (i = firststatement; i < (unsigned int)numstatements; i++)
	{
		fprintf(asmfile, "\t%s", pr_opcodes[statements[i].op].opname);
		if (pr_opcodes[statements[i].op].type_a != &type_void)
		{
			if (strlen(pr_opcodes[statements[i].op].opname)<6)
				fprintf(asmfile, "\t");
			if (pr_opcodes[statements[i].op].type_a)
				fprintf(asmfile, "\t%s", QCC_VarAtOffset(statements[i].a, (*pr_opcodes[statements[i].op].type_a)->size));
			else
				fprintf(asmfile, "\t%i", statements[i].a.ofs);
			if (pr_opcodes[statements[i].op].type_b != &type_void)
			{
				if (pr_opcodes[statements[i].op].type_b)
					fprintf(asmfile, ",\t%s", QCC_VarAtOffset(statements[i].b, (*pr_opcodes[statements[i].op].type_b)->size));
				else
					fprintf(asmfile, ",\t%i", statements[i].b.ofs);
				if (pr_opcodes[statements[i].op].type_c != &type_void && (pr_opcodes[statements[i].op].associative==ASSOC_LEFT || statements[i].c.sym))
				{
					if (pr_opcodes[statements[i].op].type_c)
						fprintf(asmfile, ",\t%s", QCC_VarAtOffset(statements[i].c, (*pr_opcodes[statements[i].op].type_c)->size));
					else
						fprintf(asmfile, ",\t%i", statements[i].c.ofs);
				}
			}
			else
			{
				if (pr_opcodes[statements[i].op].type_c != &type_void)
				{
					if (pr_opcodes[statements[i].op].type_c)
						fprintf(asmfile, ",\t%s", QCC_VarAtOffset(statements[i].c, (*pr_opcodes[statements[i].op].type_c)->size));
					else
						fprintf(asmfile, ",\t%i", statements[i].c.ofs);
				}
			}
		}	
		else
		{
			if (pr_opcodes[statements[i].op].type_c != &type_void)
			{
				if (pr_opcodes[statements[i].op].type_c)
					fprintf(asmfile, "\t%s", QCC_VarAtOffset(statements[i].c, (*pr_opcodes[statements[i].op].type_c)->size));
				else
					fprintf(asmfile, "\t%i", statements[i].c.ofs);
			}
		}
		fprintf(asmfile, "; /*%i*/\n", statements[i].linenum);
	}

	fprintf(asmfile, "}\n\n");
}
#endif

QCC_function_t *QCC_PR_GenerateBuiltinFunction (QCC_def_t *def, int builtinnum)
{
	QCC_function_t *func;
	if (numfunctions >= MAX_FUNCTIONS)
		QCC_PR_ParseError(ERR_INTERNAL, "Too many functions - %i\nAdd \"MAX_FUNCTIONS\" \"%i\" to qcc.cfg", numfunctions, (numfunctions+4096)&~4095);
	func = &functions[numfunctions++];
	func->s_file = s_file2;
	func->file = strings+s_file;
	func->line = def->s_line;	//FIXME
	func->name = def->name;
	func->builtin = builtinnum;
	func->code = -1;
	func->type = def->type;
	func->firstlocal = NULL;
	func->def = def;
	return func;
}
QCC_function_t *QCC_PR_GenerateQCFunction (QCC_def_t *def, QCC_type_t *type)
{
	QCC_function_t *func;
	if (numfunctions >= MAX_FUNCTIONS)
		QCC_PR_ParseError(ERR_INTERNAL, "Too many functions - %i\nAdd \"MAX_FUNCTIONS\" \"%i\" to qcc.cfg", numfunctions, (numfunctions+4096)&~4095);
	func = &functions[numfunctions++];
	func->s_file = s_file2;
	func->file = strings+s_file;
	func->line = def?def->s_line:0;	//FIXME
	func->name = def?def->name:"";
	func->builtin = 0;
	func->code = numstatements;
	func->firstlocal = NULL;
	func->def = def;
	func->type = type;
	return func;
}

/*
============
PR_ParseImmediateStatements

Parse a function body

If def is set, allows stuff to refer back to a def for the function.
============
*/
QCC_function_t *QCC_PR_ParseImmediateStatements (QCC_def_t *def, QCC_type_t *type)
{
	unsigned int u;
	QCC_function_t	*f;
	QCC_sref_t		parm;
	pbool needsdone=false;

	conditional = 0;

	expandedemptymacro = false;

//
// check for builtin function definition #1, #2, etc
//
// hexenC has void name() : 2;
	if (QCC_PR_CheckToken ("#") || QCC_PR_CheckToken (":"))
	{
		int binum = 0;
		if (pr_token_type == tt_immediate
		&& pr_immediate_type == type_float
		&& pr_immediate._float == (int)pr_immediate._float)
			binum = (int)pr_immediate._float;
		else if (pr_token_type == tt_immediate && pr_immediate_type == type_integer)
			binum = pr_immediate._int;
		else
			QCC_PR_ParseError (ERR_BADBUILTINIMMEDIATE, "Bad builtin immediate");
		f = QCC_PR_GenerateBuiltinFunction(def, binum);
		QCC_PR_Lex ();

		return f;
	}
	if (QCC_PR_CheckKeyword(keyword_external, "external"))
	{	//reacc style builtin
		if (pr_token_type != tt_immediate
		|| pr_immediate_type != type_float
		|| pr_immediate._float != (int)pr_immediate._float)
			QCC_PR_ParseError (ERR_BADBUILTINIMMEDIATE, "Bad builtin immediate");
		f = QCC_PR_GenerateBuiltinFunction(def, (int)-pr_immediate._float);
		QCC_PR_Lex ();
		QCC_PR_Expect(";");

		return f;
	}

//	if (type->vargs)
//		QCC_PR_ParseError (ERR_FUNCTIONWITHVARGS, "QC function with variable arguments and function body");

	pr_scope = f = QCC_PR_GenerateQCFunction(def, type);

	//reset the locals chain
	pr.local_head.nextlocal = NULL;
	pr.local_tail = &pr.local_head;

//
// define the basic parms
//

	for (u=0 ; u<type->num_parms && u < MAX_PARMS; u++)
	{
		if (!*pr_parm_names[u])
			QCC_PR_ParseError(ERR_PARAMWITHNONAME, "Parameter is not named");
		parm = QCC_PR_GetSRef (type->params[u].type, pr_parm_names[u], pr_scope, true, 0, false);
		parm.sym->used = true;	//make sure system parameters get seen by the engine, even if the names are stripped..
		parm.sym->referenced = true;
		QCC_FreeTemp(parm);
	}
	//qcvm sees the function start here.
	f->code = numstatements;
	//define any extra parts
	for (; u < type->num_parms; u++)
	{
		if (!*pr_parm_names[u])
			QCC_PR_ParseError(ERR_PARAMWITHNONAME, "Parameter is not named");
		parm = QCC_PR_GetSRef (type->params[u].type, pr_parm_names[u], pr_scope, true, 0, false);
		parm.sym->referenced = true;

		if (!extra_parms[u - MAX_PARMS].sym)
		{
			char name[128];
			QC_snprintfz(name, sizeof(name), "$parm%u", u);
			extra_parms[u - MAX_PARMS] = QCC_PR_GetSRef(type_vector, name, NULL, true, 0, GDF_STRIP);
		}
		else
			QCC_ForceUnFreeDef(extra_parms[u - MAX_PARMS].sym);
		extra_parms[u - MAX_PARMS].cast = parm.cast;
		if (parm.cast->type != ev_vector)
			QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_STORE_F], extra_parms[u - MAX_PARMS], parm, NULL));
		else
			QCC_FreeTemp(QCC_PR_Statement (&pr_opcodes[OP_STORE_V], extra_parms[u - MAX_PARMS], parm, NULL));
	}

	if (type->vargcount)
	{
		if (!pr_parm_argcount_name)
			QCC_Error(ERR_INTERNAL, "I forgot what the va_count argument is meant to be called");
		else
		{
			QCC_sref_t va_passcount = QCC_PR_GetSRef(type_float, "__va_count", NULL, true, 0, 0);
			QCC_sref_t va_count = QCC_PR_GetSRef(type_float, pr_parm_argcount_name, pr_scope, true, 0, 0);
			QCC_sref_t numparms = QCC_MakeFloatConst(type->num_parms);
			QCC_PR_SimpleStatement(&pr_opcodes[OP_SUB_F], va_passcount, numparms, va_count, false);
			QCC_FreeTemp(numparms);
			QCC_FreeTemp(va_passcount);
			QCC_FreeTemp(va_count);
		}
	}

	if (type->vargs)
	{
		int i;
		int maxvacount = 24;
		QCC_sref_t a;
		pbool opcodeextensions = QCC_OPCodeValid(&pr_opcodes[OP_FETCH_GBL_F]) || QCC_OPCodeValid(&pr_opcodes[OP_LOADA_F]);	//if we have opcode extensions, we can use those instead of via a function. this allows to use proper locals for the vargs.
		QCC_sref_t va_list;
		va_list = QCC_PR_GetSRef(type_vector, "__va_list", pr_scope, true, maxvacount, opcodeextensions?0:GDF_STATIC);

		for (i = 0; i < maxvacount; i++)
		{
			QCC_ref_t varef;
			u = i + type->num_parms;
			if (u >= MAX_PARMS)
			{
				if (!extra_parms[u - MAX_PARMS].sym)
				{
					char name[128];
					QC_snprintfz(name, sizeof(name), "$parm%u", u);
					extra_parms[u - MAX_PARMS] = QCC_PR_GetSRef(type_vector, name, NULL, true, 0, GDF_STRIP);
				}
				else
					QCC_ForceUnFreeDef(extra_parms[u - MAX_PARMS].sym);
				a = extra_parms[u - MAX_PARMS];
			}
			else
			{
				a.sym = &def_parms[u];
				a.ofs = 0;
			}
			a.cast = type_vector;
			QCC_UnFreeTemp(va_list);
			QCC_StoreToRef(QCC_PR_BuildRef(&varef, REF_ARRAY, va_list, QCC_MakeIntConst(i*3), type_vector, false), a, false, false);
		}
		QCC_FreeTemp(va_list);
	}

	QCC_RemapLockedTemps(-1, -1);

	/*if (pr_classtype)
	{
		QCC_def_t *e, *e2;
		e = QCC_PR_GetDef(pr_classtype, "__oself", pr_scope, true, 0);
		e2 = QCC_PR_GetDef(type_entity, "self", NULL, true, 0);
		QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_STORE_ENT], QCC_PR_DummyDef(pr_classtype, "self", pr_scope, 0, e2->ofs, false), e, NULL));
	}*/

//
// check for a state opcode
//
	if (QCC_PR_CheckToken ("["))
		QCC_PR_ParseState ();

	if (QCC_PR_CheckKeyword (keyword_asm, "asm"))
	{
		QCC_PR_Expect ("{");
		while (!QCC_PR_CheckToken("}"))
			QCC_PR_ParseAsm ();
	}
	else
	{
		if (QCC_PR_CheckKeyword (keyword_var, "var"))	//reacc support
		{	//parse lots of locals
			char *name;
			do {
				name = QCC_PR_ParseName();
				QCC_PR_Expect(":");
				QCC_FreeDef(QCC_PR_GetDef(QCC_PR_ParseType(false, false), name, pr_scope, true, 0, false));
				QCC_PR_Expect(";");
			} while(!QCC_PR_CheckToken("{"));
		}
		else
			QCC_PR_Expect ("{");
//
// parse regular statements
//
		while (STRCMP ("}", pr_token))	//not check token to avoid the lex consuming following pragmas
		{
			QCC_PR_ParseStatement ();
			QCC_FreeTemps();
		}
	}
	QCC_FreeTemps();

	// this is cheap
//	if (type->aux_type->type)
//		if (statements[numstatements - 1].op != OP_RETURN)
//			QCC_PR_ParseWarning(WARN_MISSINGRETURN, "%s: not all control paths return a value", pr_scope->name );

	if (f->code == numstatements)
		needsdone = true;
	else if (statements[numstatements - 1].op != OP_RETURN && statements[numstatements - 1].op != OP_DONE)
		needsdone = true;

	if (num_gotos)
	{
		int i, j;
		for (i = 0; i < num_gotos; i++)
		{
			for (j = 0; j < num_labels; j++)
			{
				if (!strcmp(pr_gotos[i].name, pr_labels[j].name))
				{
					if (!pr_opcodes[statements[pr_gotos[i].statementno].op].type_a)
						statements[pr_gotos[i].statementno].a.ofs += pr_labels[j].statementno - pr_gotos[i].statementno;
					else if (!pr_opcodes[statements[pr_gotos[i].statementno].op].type_b)
						statements[pr_gotos[i].statementno].b.ofs += pr_labels[j].statementno - pr_gotos[i].statementno;
					else
						statements[pr_gotos[i].statementno].c.ofs += pr_labels[j].statementno - pr_gotos[i].statementno;
					break;
				}
			}
			if (j == num_labels)
			{
				num_gotos = 0;
				QCC_PR_ParseError(ERR_NOLABEL, "Goto statement with no matching label \"%s\"", pr_gotos[i].name);
			}
		}
		num_gotos = 0;
	}

	if (opt_return_only && !needsdone)
		needsdone = QCC_FuncJumpsTo(f->code, numstatements, numstatements);

	// emit an end of statements opcode
	if (!opt_return_only || needsdone)
	{
		/*if (pr_classtype)
		{
			QCC_def_t *e, *e2;
			e = QCC_PR_GetDef(NULL, "__oself", pr_scope, false, 0);
			e2 = QCC_PR_GetDef(NULL, "self", NULL, false, 0);
			QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_STORE_ENT], e, QCC_PR_DummyDef(pr_classtype, "self", pr_scope, 0, e2->ofs, false), NULL));
		}*/

		QCC_PR_Statement (&pr_opcodes[OP_DONE], nullsref, nullsref, NULL);
	}
	else
		optres_return_only++;

	QCC_CheckForDeadAndMissingReturns(f->code, numstatements, type->aux_type->type);

//	if (opt_comexprremoval)
//		QCC_CommonSubExpressionRemoval(f->code, numstatements);


	QCC_RemapLockedTemps(f->code, numstatements);
	QCC_WriteAsmFunction(pr_scope, f->code, f->firstlocal);

	QCC_Marshal_Locals(f->code, numstatements);

	if (opt_compound_jumps)
		QCC_CompoundJumps(f->code, numstatements);

	if (num_labels)
		num_labels = 0;


	if (num_continues)
	{
		num_continues=0;
		QCC_PR_ParseError(ERR_ILLEGALCONTINUES, "%s: function contains illegal continues", pr_scope->name);
	}
	if (num_breaks)
	{
		num_breaks=0;
		QCC_PR_ParseError(ERR_ILLEGALBREAKS, "%s: function contains illegal breaks", pr_scope->name);
	}
	if (num_cases)
	{
		num_cases = 0;
		QCC_PR_ParseError(ERR_ILLEGALCASES, "%s: function contains illegal cases", pr_scope->name);
	}

	QCC_PR_Lex();

	return f;
}

void QCC_PR_ArrayRecurseDivideRegular(QCC_sref_t array, QCC_sref_t index, int min, int max)
{
	QCC_statement_t *st;
	QCC_sref_t eq;
	int stride;

	if (array.cast->type == ev_vector)
		stride = 3;
	else
		stride = 1;	//struct arrays should be 1, so that every element can be accessed...

	if (min == max || min+1 == max)
	{
		eq = QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst(min+0.5f), NULL, STFL_PRESERVEA);
		QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, eq, nullsref, &st));
		st->b.ofs = 2;
		QCC_PR_Statement(pr_opcodes+OP_RETURN, array, nullsref, &st);
		st->a.ofs += min*stride;
	}
	else
	{
		int mid = min + (max-min)/2;

		if (max-min>4)
		{
			eq = QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst(mid+0.5f), NULL, STFL_PRESERVEA);
			QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, eq, nullsref, &st));
		}
		else
			st = NULL;
		QCC_PR_ArrayRecurseDivideRegular(array, index, min, mid);
		if (st)
			st->b.ofs = numstatements - (st-statements);
		QCC_PR_ArrayRecurseDivideRegular(array, index, mid, max);
	}
}

//the idea here is that we return a vector, the caller then figures out the extra 3rd.
//This is useful when we have a load of indexes.
void QCC_PR_ArrayRecurseDivideUsingVectors(QCC_sref_t array, QCC_sref_t index, int min, int max)
{
	QCC_statement_t *st;
	QCC_sref_t eq;
	if (min == max || min+1 == max)
	{
		eq = QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst(min+0.5f), NULL, STFL_PRESERVEA);
		QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, eq, nullsref, &st));
		st->b.ofs = 2;
		QCC_PR_Statement(pr_opcodes+OP_RETURN, array, nullsref, &st);
		st->a.ofs += min*3;
	}
	else
	{
		int mid = min + (max-min)/2;

		if (max-min>4)
		{
			eq = QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst(mid+0.5f), NULL, STFL_PRESERVEA);
			QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, eq, nullsref, &st));
		}
		else
			st = NULL;
		QCC_PR_ArrayRecurseDivideUsingVectors(array, index, min, mid);
		if (st)
			st->b.ofs = numstatements - (st-statements);
		QCC_PR_ArrayRecurseDivideUsingVectors(array, index, mid, max);
	}
}

//returns a vector overlapping the result needed.
QCC_def_t *QCC_PR_EmitArrayGetVector(QCC_sref_t array)
{
	QCC_sref_t temp, index;
	QCC_def_t *func;

	int numslots;

	QCC_type_t *ftype = qccHunkAlloc(sizeof(*ftype));
	struct QCC_typeparam_s *fparms = qccHunkAlloc(sizeof(*fparms)*1);
	ftype->size = 1;
	ftype->type = ev_function;
	ftype->aux_type = type_vector;
	ftype->params = fparms;
	ftype->num_parms = 1;
	ftype->name = "ArrayGet";
	fparms[0].type = type_float;

	//array shouldn't ever be a vector array
	numslots = array.sym->arraysize*array.cast->size;
	numslots = (numslots+2)/3;

	s_file = array.sym->s_file;
	func = QCC_PR_GetDef(ftype, qcva("ArrayGetVec*%s", array.sym->name), NULL, true, 0, false);

	pr_source_line = pr_token_line_last = array.sym->s_line;	//thankfully these functions are emitted after compilation.

	if (numfunctions >= MAX_FUNCTIONS)
		QCC_Error(ERR_INTERNAL, "Too many function defs");

	pr_scope = QCC_PR_GenerateQCFunction(func, ftype);
	pr_source_line = pr_token_line_last = pr_scope->line = array.sym->s_line;	//thankfully these functions are emitted after compilation.
	pr_scope->s_file = array.sym->s_file;
	func->symboldata[func->ofs]._int = pr_scope - functions;

	index = QCC_PR_GetSRef(type_float, "index___", pr_scope, true, 0, false);
	index.sym->referenced = true;
	temp = QCC_PR_GetSRef(type_float, "div3___", pr_scope, true, 0, false);
	QCC_PR_SimpleStatement(pr_opcodes+OP_DIV_F, index, QCC_MakeFloatConst(3), temp, false);
	QCC_PR_SimpleStatement(pr_opcodes+OP_BITAND_F, temp, temp, temp, false);//round down to int

	QCC_PR_ArrayRecurseDivideUsingVectors(array, temp, 0, numslots);

	QCC_PR_Statement(pr_opcodes+OP_RETURN, QCC_MakeVectorConst(0,0,0), nullsref, NULL);	//err... we didn't find it, give up.
	QCC_PR_Statement(pr_opcodes+OP_DONE, nullsref, nullsref, NULL);	//err... we didn't find it, give up.

	func->initialized = 1;


	QCC_WriteAsmFunction(pr_scope, pr_scope->code, pr_scope->firstlocal);
	QCC_Marshal_Locals(pr_scope->code, numstatements);
	QCC_FreeTemps();

	return func;
}

void QCC_PR_EmitArrayGetFunction(QCC_def_t *scope, QCC_def_t *arraydef, char *arrayname)
{
	QCC_sref_t vectortrick;
	QCC_sref_t index, thearray = QCC_MakeSRefForce(arraydef, 0, arraydef->type);

	QCC_statement_t *st;
	QCC_sref_t eq;
	QCC_statement_t *bc1=NULL, *bc2=NULL;

//	QCC_sref_t fasttrackpossible = nullsref;
	int numslots;

	if (thearray.cast->type == ev_vector)
		numslots = thearray.sym->arraysize;
	else
		numslots = thearray.sym->arraysize*thearray.cast->size;

//	if (flag_fasttrackarrays && numslots > 6)
//		fasttrackpossible = QCC_PR_GetSRef(type_float, "__ext__fasttrackarrays", NULL, true, 0, false);

	s_file = scope->s_file;

	vectortrick = nullsref;
	if (numslots >= 15 && thearray.cast->type != ev_vector)
	{
		vectortrick.sym = QCC_PR_EmitArrayGetVector(thearray);
		vectortrick.cast = vectortrick.sym->type;
	}

	pr_scope = QCC_PR_GenerateQCFunction(scope, scope->type);
	pr_source_line = pr_token_line_last = pr_scope->line = thearray.sym->s_line;	//thankfully these functions are emitted after compilation.
	pr_scope->s_file = thearray.sym->s_file;

	index = QCC_PR_GetSRef(type_float, "__indexg", pr_scope, true, 0, false);

	scope->initialized = true;
	scope->symboldata[scope->ofs]._int = pr_scope - functions;

/*	if (fasttrackpossible)
	{
		QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, fasttrackpossible, nullsref, &st);
		//fetch_gbl takes: (float size, variant array[]), float index, variant pos
		//note that the array size is coded into the globals, one index before the array.

		if (thearray.cast->type == ev_vector)
			QCC_PR_SimpleStatement(&pr_opcodes[OP_FETCH_GBL_V], thearray, index, sref_ret, true);
		else
			QCC_PR_SimpleStatement(&pr_opcodes[OP_FETCH_GBL_F], thearray, index, sref_ret, true);

		QCC_FreeTemp(QCC_PR_Statement(&pr_opcodes[OP_RETURN], sref_ret, nullsref, NULL));

		//finish the jump
		st->b.ofs = &statements[numstatements] - st;
	}
*/

	if (!flag_noboundchecks)
		QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IF_I, QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst(0), NULL, STFL_PRESERVEA), nullsref, &bc1));

	if (vectortrick.cast)
	{
		QCC_sref_t div3, intdiv3, ret;

		//okay, we've got a function to retrieve the var as part of a vector.
		//we need to work out which part, x/y/z that it's stored in.
		//0,1,2 = i - ((int)i/3 *) 3;

		if (!flag_noboundchecks)
			QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IF_I, QCC_PR_StatementFlags(pr_opcodes+OP_GE_F, index, QCC_MakeFloatConst(numslots), NULL, STFL_PRESERVEA), nullsref, &bc2));

		div3 = QCC_PR_GetSRef(type_float, "div3___", pr_scope, true, 0, false);
		intdiv3 = QCC_PR_GetSRef(type_float, "intdiv3___", pr_scope, true, 0, false);

		div3.sym->referenced = true;
		QCC_PR_SimpleStatement(pr_opcodes+OP_BITAND_F, index, index, index, false);
		QCC_PR_SimpleStatement(pr_opcodes+OP_DIV_F, index, QCC_MakeFloatConst(3), div3, false);
		QCC_PR_SimpleStatement(pr_opcodes+OP_BITAND_F, div3, div3, intdiv3, false);

		QCC_UnFreeTemp(index);
		ret = QCC_PR_GenerateFunctionCall (nullsref, vectortrick, &index, &type_float, 1);

		div3 = QCC_PR_Statement(pr_opcodes+OP_MUL_F, intdiv3, QCC_MakeFloatConst(3), NULL);
		QCC_PR_SimpleStatement(pr_opcodes+OP_SUB_F, index, div3, index, false);
		QCC_FreeTemp(div3);

		eq = QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst(0+0.5f), NULL, STFL_PRESERVEA);
		QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, eq, nullsref, &st));
		st->b.ofs = 2;
		ret.cast = type_float;
		QCC_PR_Statement(pr_opcodes+OP_RETURN, ret, nullsref, NULL);
		ret.ofs++;

		eq = QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst(1+0.5f), NULL, STFL_PRESERVEA);
		QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, eq, nullsref, &st));
		st->b.ofs = 2;
		QCC_PR_Statement(pr_opcodes+OP_RETURN, ret, nullsref, NULL);
		ret.ofs++;

		eq = QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst(2+0.5), NULL, STFL_PRESERVEA);
		QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, eq, nullsref, &st));
		st->b.ofs = 2;
		QCC_PR_Statement(pr_opcodes+OP_RETURN, ret, nullsref, NULL);
		ret.ofs++;
		ret.ofs-=3;
		QCC_FreeTemp(ret);
	}
	else
	{
		QCC_PR_SimpleStatement(pr_opcodes+OP_BITAND_F, index, index, index, false);
		QCC_PR_ArrayRecurseDivideRegular(thearray, index, 0, numslots);
	}

	if (bc1)
		bc1->b.ofs = &statements[numstatements] - bc1;
	if (bc2)
		bc2->b.ofs = &statements[numstatements] - bc2;
	if (bc1 || bc2)
	{
		QCC_sref_t errfnc = QCC_PR_GetSRef(NULL, "error", NULL, false, 0, false);
		QCC_sref_t sprintffnc = QCC_PR_GetSRef(NULL, "sprintf", NULL, false, 0, false);
		QCC_sref_t errmsg;
		if (sprintffnc.cast)
		{	//if sprintf is defined, we generate a more verbose error message. this message should appear in at least one of the engines the mod was made for, and in others it should be obivious enough.
			QCC_sref_t args[3];
			QCC_type_t *argtypes[3] = {type_string, type_float, type_float};
			args[0] = QCC_MakeStringConst("bounds check failed (0 <= %g < %g is false)\n");
			QCC_UnFreeTemp(index);
			args[1] = index;
			args[2] = QCC_MakeFloatConst(numslots);
			errmsg = QCC_PR_GenerateFunctionCall(nullsref, sprintffnc, args, argtypes, 3);
		}
		else
			errmsg = QCC_MakeStringConst("bounds check failed\n");
		QCC_FreeTemp(QCC_PR_GenerateFunctionCall(nullsref, errfnc, &errmsg, &type_string, 1));
	}
	QCC_FreeTemp(index);
	//we get here if they tried reading beyond the end of the array with bounds checks disabled. just return the last valid element.
	if (thearray.cast->type == ev_vector)
		thearray.ofs += (numslots-1)*3;
	else
		thearray.ofs += (numslots-1);
	QCC_PR_Statement(pr_opcodes+OP_RETURN, thearray, nullsref, &st);
	QCC_PR_Statement(pr_opcodes+OP_DONE, nullsref, nullsref, NULL);

	QCC_WriteAsmFunction(pr_scope, pr_scope->code, pr_scope->firstlocal);
	QCC_Marshal_Locals(pr_scope->code, numstatements);
	QCC_FreeTemps();
}

void QCC_PR_ArraySetRecurseDivide(QCC_sref_t array, QCC_sref_t index, QCC_sref_t value, int min, int max)
{
	QCC_statement_t *st;
	QCC_sref_t eq;
	int stride;

	if (array.cast->type == ev_vector)
		stride = 3;
	else
		stride = 1;	//struct arrays should be 1, so that every element can be accessed...

	if (min == max || min+1 == max)
	{
		eq = QCC_PR_StatementFlags(pr_opcodes+OP_EQ_F, index, QCC_MakeFloatConst((float)min), NULL, STFL_PRESERVEA);
		QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, eq, nullsref, &st));
		st->b.ofs = 3;
		if (stride == 3)
			QCC_PR_StatementFlags(pr_opcodes+OP_STORE_V, value, array, &st, STFL_PRESERVEB);
		else
			QCC_PR_StatementFlags(pr_opcodes+OP_STORE_F, value, array, &st, STFL_PRESERVEB);
		st->b.ofs += min*stride;
		QCC_PR_Statement(pr_opcodes+OP_RETURN, nullsref, nullsref, NULL);
	}
	else
	{
		int mid = min + (max-min)/2;

		if (max-min>4)
		{
			eq = QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst((float)mid), NULL, STFL_PRESERVEA);
			QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, eq, nullsref, &st));
		}
		else
			st = NULL;
		QCC_PR_ArraySetRecurseDivide(array, index, value, min, mid);
		if (st)
			st->b.ofs = numstatements - (st-statements);
		QCC_PR_ArraySetRecurseDivide(array, index, value, mid, max);
	}
}

void QCC_PR_EmitArraySetFunction(QCC_def_t *scope, QCC_def_t *arraydef, char *arrayname)
{
	QCC_sref_t index, value, thearray = QCC_MakeSRefForce(arraydef, 0, arraydef->type);

	QCC_sref_t fasttrackpossible;
	int numslots;
	QCC_statement_t *bc1=NULL, *bc2=NULL;

	if (thearray.cast->type == ev_vector)
		numslots = thearray.sym->arraysize;
	else
		numslots = thearray.sym->arraysize*thearray.cast->size;

	fasttrackpossible = nullsref;
	if (flag_fasttrackarrays && numslots > 6)
		fasttrackpossible = QCC_PR_GetSRef(type_float, "__ext__fasttrackarrays", NULL, true, 0, false);

	if (numfunctions >= MAX_FUNCTIONS)
		QCC_Error(ERR_INTERNAL, "Too many function defs");

	s_file = arraydef->s_file;
	pr_scope = QCC_PR_GenerateQCFunction(scope, scope->type);
	pr_source_line = pr_token_line_last = pr_scope->line = thearray.sym->s_line;	//thankfully these functions are emitted after compilation.
	pr_scope->s_file = thearray.sym->s_file;

	index = QCC_PR_GetSRef(type_float, "indexs___", pr_scope, true, 0, false);
	value = QCC_PR_GetSRef(thearray.cast, "value___", pr_scope, true, 0, false);

	scope->initialized = true;
	scope->symboldata[scope->ofs]._int = pr_scope - functions;

	if (fasttrackpossible.cast)
	{
		QCC_statement_t *st;

		QCC_PR_Statement(pr_opcodes+OP_IFNOT_I, fasttrackpossible, nullsref, &st);
		//note that the array size is coded into the globals, one index before the array.

		QCC_PR_SimpleStatement(&pr_opcodes[OP_CONV_FTOI], index, nullsref, index, true);	//address stuff is integer based, but standard qc (which this accelerates in supported engines) only supports floats
		if (!flag_noboundchecks)
			QCC_PR_SimpleStatement (&pr_opcodes[OP_BOUNDCHECK], index, QCC_MakeSRef(NULL, numslots, NULL), nullsref, true);//annoy the programmer. :p
		if (thearray.cast->type == ev_vector)//shift it upwards for larger types
			QCC_PR_SimpleStatement(&pr_opcodes[OP_MUL_I], index, QCC_MakeIntConst(thearray.cast->size), index, true);
		QCC_PR_SimpleStatement(&pr_opcodes[OP_GLOBALADDRESS], thearray, index, index, true);	//comes with built in add
		if (thearray.cast->type == ev_vector)
			QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_V], value, index, nullsref, true);	//*b = a
		else
			QCC_PR_SimpleStatement(&pr_opcodes[OP_STOREP_F], value, index, nullsref, true);
		QCC_PR_Statement(&pr_opcodes[OP_RETURN], value, nullsref, NULL);

		//finish the jump
		st->b.ofs = &statements[numstatements] - st;
	}

	QCC_PR_SimpleStatement(pr_opcodes+OP_BITAND_F, index, index, index, false);
	if (!flag_noboundchecks)
		QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IF_I, QCC_PR_StatementFlags(pr_opcodes+OP_LT_F, index, QCC_MakeFloatConst(0), NULL, STFL_PRESERVEA), nullsref, &bc1));
	if (!flag_noboundchecks)
		QCC_FreeTemp(QCC_PR_Statement(pr_opcodes+OP_IF_I, QCC_PR_StatementFlags(pr_opcodes+OP_GT_F, index, QCC_MakeFloatConst(numslots-1), NULL, STFL_PRESERVEA), nullsref, &bc2));

	QCC_PR_ArraySetRecurseDivide(thearray, index, value, 0, numslots);

	if (bc1)
		bc1->b.ofs = &statements[numstatements] - bc1;
	if (bc2)
		bc2->b.ofs = &statements[numstatements] - bc2;
	if (bc1 || bc2)
	{
		QCC_sref_t errfnc = QCC_PR_GetSRef(NULL, "error", NULL, false, 0, false);
		QCC_sref_t errmsg = QCC_MakeStringConst("bounds check failed\n");
		QCC_FreeTemp(QCC_PR_GenerateFunctionCall(nullsref, errfnc, &errmsg, &type_string, 1));
	}

	QCC_PR_Statement(pr_opcodes+OP_DONE, nullsref, nullsref, NULL);



	QCC_WriteAsmFunction(pr_scope, pr_scope->code, pr_scope->firstlocal);
	QCC_Marshal_Locals(pr_scope->code, numstatements);
	QCC_FreeTemps();
}

//register a def, and all of it's sub parts.
//only the main def is of use to the compiler.
//the subparts are emitted to the compiler and allow correct saving/loading
//be careful with fields, this doesn't allocated space, so will it allocate fields. It only creates defs at specified offsets.
QCC_def_t *QCC_PR_DummyDef(QCC_type_t *type, char *name, QCC_function_t *scope, int arraysize, QCC_def_t *rootsymbol, unsigned int ofs, int referable, unsigned int flags)
{
	char array[64];
	char newname[256];
	int a;
	QCC_def_t *def, *first=NULL;
	char typebuf[1024];

	while (rootsymbol && rootsymbol->symbolheader != rootsymbol)
	{
		ofs += rootsymbol->ofs;
		rootsymbol = rootsymbol->symbolheader;
	}

#define KEYWORD(x) if (!STRCMP(name, #x) && keyword_##x) {if (keyword_##x)QCC_PR_ParseWarning(WARN_KEYWORDDISABLED, "\""#x"\" keyword used as variable name%s", keywords_coexist?" - coexisting":" - disabling");keyword_##x=keywords_coexist;}
	if (name)
	{
		KEYWORD(var);
		KEYWORD(thinktime);
		KEYWORD(for);
		KEYWORD(switch);
		KEYWORD(case);
		KEYWORD(default);
		KEYWORD(goto);
		if (type->type != ev_function)
			KEYWORD(break);
		KEYWORD(continue);
		KEYWORD(state);
		KEYWORD(string);
		if (qcc_targetformat != QCF_HEXEN2)
			KEYWORD(float);	//hmm... hexen2 requires this...
		KEYWORD(entity);
		KEYWORD(vector);
		KEYWORD(const);
		KEYWORD(asm);
	}

	if (!type)
		return NULL;

	for (a = -1; a < arraysize; a++)
	{
		if (a == -1)
			*array = '\0';
		else
			QC_snprintfz(array, sizeof(array), "[%i]", a);

		if (name)
			QC_snprintfz(newname, sizeof(newname), "%s%s", name, array);
		else
			*newname = *"";

		// allocate a new def
		def = (void *)qccHunkAlloc (sizeof(QCC_def_t));
		memset (def, 0, sizeof(*def));
		def->next = NULL;
		def->arraysize = a>=0?0:arraysize;
		pr.def_tail->next = def;
		pr.def_tail = def;

		if (scope && !(flags & (GDF_CONST|GDF_STATIC)))
		{	//constants are never considered locals. static variables are should also not be counted as locals despite being visible from only one place.
			pr.local_tail->nextlocal = def;
			pr.local_tail = def;
			def->localscope = true;
		}

		if (a >= 0)
			def->referenced = true;

		def->s_line = pr_source_line;
		def->s_file = s_file;
		if (a>=0)
			def->initialized = 1;

		def->name = (void *)qccHunkAlloc (strlen(newname)+1);
		strcpy (def->name, newname);
		def->type = type;

		def->scope = scope;
		def->constant = !!(flags & GDF_CONST);
		def->isstatic = !!(flags & GDF_STATIC);
		if (arraysize && a < 0)
		{	//array headers can be stripped safely.
			def->saved = false;
			def->strip = true;
		}
		else
		{
			def->saved = (!arraysize || a>=0) && !!(flags & GDF_SAVED);	//saved is never set on the head def in an array.
			def->strip = !!(flags & GDF_STRIP);
		}
		def->allowinline = !!(flags & GDF_INLINE);

		if (flags & GDF_USED)
			def->used = true;

		def->ofs = ofs + ((a>0)?type->size*a:0);
		if (!first)
			first = def;
		if (!rootsymbol)
		{
			rootsymbol = first;
			rootsymbol->symboldata = qccHunkAlloc ((def->arraysize?def->arraysize:1) * type->size * sizeof(float));
		}

		def->symbolheader = rootsymbol;
		def->symboldata = rootsymbol->symboldata;
		def->symbolsize = (def->arraysize?def->arraysize:1) * type->size;

		if (type->type == ev_struct && (!arraysize || a>=0))
		{
			unsigned int partnum;
			QCC_type_t *parttype;
			def->saved = false;	//struct headers don't get saved.
			for (partnum = 0; partnum < type->num_parms; partnum++)
			{
				parttype = type->params[partnum].type;
				while (parttype->type == ev_accessor)
					parttype = parttype->parentclass;
				switch (parttype->type)
				{
				case ev_vector:
					QC_snprintfz(newname, sizeof(newname), "%s%s.%s", name, array, type->params[partnum].paramname);
					QCC_PR_DummyDef(parttype, newname, scope, type->params[partnum].arraysize, rootsymbol, def->ofs+type->params[partnum].ofs, false, flags | GDF_CONST);

//					QC_snprintfz(newname, sizeof(newname), "%s%s.%s_x", name, array, type->params[partnum].paramname);
//					QCC_PR_DummyDef(type_float, newname, scope, 0, rootsymbol, type->params[partnum].ofs - rootsymbol->ofs, false, flags | GDF_CONST);
//					QC_snprintfz(newname, sizeof(newname), "%s%s.%s_y", name, array, type->params[partnum].paramname);
//					QCC_PR_DummyDef(type_float, newname, scope, 0, rootsymbol, type->params[partnum].ofs+1 - rootsymbol->ofs, false, flags | GDF_CONST);
//					QC_snprintfz(newname, sizeof(newname), "%s%s.%s_z", name, array, type->params[partnum].paramname);
//					QCC_PR_DummyDef(type_float, newname, scope, 0, rootsymbol, type->params[partnum].ofs+2 - rootsymbol->ofs, false, flags | GDF_CONST);
					break;

				case ev_accessor:	//shouldn't happen.
				case ev_float:
				case ev_string:
				case ev_entity:
				case ev_field:
				case ev_pointer:
				case ev_integer:
				case ev_struct:
				case ev_union:
				case ev_variant:	//for lack of any better alternative
					QC_snprintfz(newname, sizeof(newname), "%s%s.%s", name, array, type->params[partnum].paramname);
					QCC_PR_DummyDef(parttype, newname, scope, type->params[partnum].arraysize, rootsymbol, def->ofs+type->params[partnum].ofs, false, flags);
					break;

				case ev_function:
					QC_snprintfz(newname, sizeof(newname), "%s%s.%s", name, array, parttype->name);
					QCC_PR_DummyDef(parttype, newname, scope, type->params[partnum].arraysize, rootsymbol, def->ofs+type->params[partnum].ofs, false, flags)->initialized = true;
					break;
				case ev_void:
					break;
				}
			}
		}
		else if (type->type == ev_vector)
		{	//do the vector thing.
			QC_snprintfz(newname, sizeof(newname), "%s%s_x", name, array);
			QCC_PR_DummyDef(type_float, newname, scope, 0, rootsymbol, def->ofs+0, referable, (flags&~GDF_SAVED) | GDF_STRIP);
			QC_snprintfz(newname, sizeof(newname), "%s%s_y", name, array);
			QCC_PR_DummyDef(type_float, newname, scope, 0, rootsymbol, def->ofs+1, referable, (flags&~GDF_SAVED) | GDF_STRIP);
			QC_snprintfz(newname, sizeof(newname), "%s%s_z", name, array);
			QCC_PR_DummyDef(type_float, newname, scope, 0, rootsymbol, def->ofs+2, referable, (flags&~GDF_SAVED) | GDF_STRIP);
		}
		else if (type->type == ev_field)
		{
			if (type->aux_type->type == ev_vector)
			{
				//do the vector thing.
				QC_snprintfz(newname, sizeof(newname), "%s%s_x", name, array);
				QCC_PR_DummyDef(type_floatfield, newname, scope, 0, rootsymbol, def->ofs+0, referable, flags);
				QC_snprintfz(newname, sizeof(newname), "%s%s_y", name, array);
				QCC_PR_DummyDef(type_floatfield, newname, scope, 0, rootsymbol, def->ofs+1, referable, flags);
				QC_snprintfz(newname, sizeof(newname), "%s%s_z", name, array);
				QCC_PR_DummyDef(type_floatfield, newname, scope, 0, rootsymbol, def->ofs+2, referable, flags);
			}
		}
		first->deftail = pr.def_tail;
	}

	if (referable)
	{
//		if (!arraysize && first->type->type != ev_field)
//			first->constant = false;
		if (scope)
			pHash_Add(&localstable, first->name, first, qccHunkAlloc(sizeof(bucket_t)));
		else
			pHash_Add(&globalstable, first->name, first, qccHunkAlloc(sizeof(bucket_t)));

		if (!scope && asmfile)
			fprintf(asmfile, "%s %s;\n", TypeName(first->type, typebuf, sizeof(typebuf)), first->name);
	}

	return first;
}

/*
============
PR_GetDef

If type is NULL, it will match any type
If allocate is true, a new def will be allocated if it can't be found
If arraysize=0, its not an array and has 1 element.
If arraysize>0, its an array and requires array notation
If arraysize<0, its an array with undefined size - GetDef will fail if its not already allocated.
============
*/

QCC_def_t *QCC_PR_GetDef (QCC_type_t *type, char *name, struct QCC_function_s *scope, pbool allocate, int arraysize, unsigned int flags)
{
	int ofs;
	QCC_def_t		*def;
//	char element[MAX_NAME];
	QCC_def_t *foundstatic = NULL;
	char typebuf1[1024], typebuf2[1024];
	int ins, insmax;

	if (!allocate)
		arraysize = -1;

	if (pHash_Get != &Hash_Get)
	{
		ins = 0;
		insmax = allocate?1:2;
	}
	else
	{
		ins = 1;
		insmax = 2;
	}
	for (; ins < insmax; ins++)
	{
		if (scope)
		{
			def = pHash_Get(&localstable, name);

			while(def)
			{
				//ignore differing case the first time around.
				if (ins == 0 && strcmp(def->name, name))
				{
					def = pHash_GetNext(&localstable, name, def);
					continue;		// in a different function
				}

				if ( def->scope && def->scope != scope)
				{
					def = pHash_GetNext(&localstable, name, def);
					continue;		// in a different function
				}

				if (type && typecmp(def->type, type))
					QCC_PR_ParseErrorPrintDef (ERR_TYPEMISMATCHREDEC, def, "Type mismatch on redeclaration of %s. %s, should be %s",name, TypeName(type, typebuf1, sizeof(typebuf1)), TypeName(def->type, typebuf2, sizeof(typebuf2)));
				if (def->arraysize != arraysize && arraysize>=0)
					QCC_PR_ParseErrorPrintDef (ERR_TYPEMISMATCHARRAYSIZE, def, "Array sizes for redecleration of %s do not match",name);
				if (allocate && scope)
				{
					QCC_PR_ParseWarning (WARN_DUPLICATEDEFINITION, "%s duplicate definition ignored", name);
					QCC_PR_ParsePrintDef(WARN_DUPLICATEDEFINITION, def);
	//				if (!scope)
	//					QCC_PR_ParsePrintDef(def);
				}

				QCC_ForceUnFreeDef(def);
				return def;
			}
		}

		def = pHash_Get(&globalstable, name);

		while(def)
		{
			//ignore differing case the first time around.
			if (ins == 0 && strcmp(def->name, name))
			{
				def = pHash_GetNext(&globalstable, name, def);
				continue;		// in a different function
			}

			if ( (def->scope || (scope && allocate)) && def->scope != scope)
			{
				def = pHash_GetNext(&globalstable, name, def);
				continue;		// in a different function
			}

			//ignore it if its static in some other file.
			if (def->isstatic && strcmp(strings+def->s_file, strings+s_file))
			{
				if (!foundstatic)
					foundstatic = def;	//save it off purely as a warning.
				def = pHash_GetNext(&globalstable, name, def);
				continue;		// in a different function
			}

			if (type && typecmp(def->type, type))
			{
				if (pr_scope || typecmp_lax(def->type, type))
				{
					if (!strcmp("droptofloor", def->name))
					{
						//this is a hack. droptofloor was wrongly declared in vanilla qc, which causes problems with replacement extensions.qc.
						//yes, this is a selfish lazy hack for this, there's probably a better way, but at least we spit out a warning still.
						QCC_PR_ParseWarning (WARN_LAXCAST, "%s builtin was wrongly defined as %s. ignoring invalid dupe definition",name, TypeName(type, typebuf1, sizeof(typebuf1)));
						QCC_PR_ParsePrintDef(WARN_LAXCAST, def);
					}
					else
					{
						//unequal even when we're lax
						QCC_PR_ParseErrorPrintDef (ERR_TYPEMISMATCHREDEC, def, "Type mismatch on redeclaration of %s. %s, should be %s",name, TypeName(type, typebuf1, sizeof(typebuf1)), TypeName(def->type, typebuf2, sizeof(typebuf2)));
					}
				}
				else
				{
					if (type->type != ev_function || type->num_parms != def->type->num_parms || !(def->type->vargs && !type->vargs))
					{
						//if the second def simply has no ..., don't bother warning about it.

						QCC_PR_ParseWarning (WARN_TYPEMISMATCHREDECOPTIONAL, "Optional arguments differ on redeclaration of %s. %s, should be %s",name, TypeName(type, typebuf1, sizeof(typebuf1)), TypeName(def->type, typebuf2, sizeof(typebuf2)));
						QCC_PR_ParsePrintDef(WARN_TYPEMISMATCHREDECOPTIONAL, def);

						if (type->type == ev_function)
						{
							//update the def's type to the new one if the mandatory argument count is longer
							//FIXME: don't change the param names!
							if (type->num_parms > def->type->num_parms)
								def->type = type;
						}
					}
				}
			}
			if (def->arraysize != arraysize && arraysize>=0)
				QCC_PR_ParseErrorPrintDef(ERR_TYPEMISMATCHARRAYSIZE, def, "Array sizes for redecleration of %s do not match",name);
			if (allocate && scope)
			{
				if (pr_scope)
				{	//warn? or would that be pointless?
					def = pHash_GetNext(&globalstable, name, def);
					continue;		// in a different function
				}

				QCC_PR_ParseWarning (WARN_DUPLICATEDEFINITION, "%s duplicate definition ignored", name);
				QCC_PR_ParsePrintDef(WARN_DUPLICATEDEFINITION, def);
	//			if (!scope)
	//				QCC_PR_ParsePrintDef(def);
			}
			QCC_ForceUnFreeDef(def);
			return def;
		}
	}

	if (foundstatic && !allocate && !(flags & GDF_SILENT))
	{
		QCC_PR_ParseWarning (WARN_DUPLICATEDEFINITION, "%s defined static", name);
		QCC_PR_ParsePrintDef(WARN_DUPLICATEDEFINITION, foundstatic);
	}

	if (!allocate)
		return NULL;

	if (arraysize < 0)
	{
		QCC_PR_ParseError (ERR_ARRAYNEEDSSIZE, "First declaration of array %s with no size",name);
	}

	if (scope && qccwarningaction[WARN_SAMENAMEASGLOBAL])
	{
		def = QCC_PR_GetDef(NULL, name, NULL, false, arraysize, false);
		if (def && def->type->type == type->type)
		{	//allow type differences. this means that arguments called 'min' or 'mins' are accepted with the 'min' builtin or the 'mins' field in existance.
			QCC_PR_ParseWarning(WARN_SAMENAMEASGLOBAL, "Local \"%s\" hides global with same name and type", name);
			QCC_PR_ParsePrintDef(WARN_SAMENAMEASGLOBAL, def);
		}
		QCC_FreeDef(def);
	}

	ofs = 0;

	if (!strncmp(name, "autocvar_", 9))
	{
		if (scope)
			QCC_PR_ParseWarning(WARN_MISUSEDAUTOCVAR, "Autocvar \"%s\" defined with local scope", name);
		else if (flags & GDF_CONST)
			QCC_PR_ParseWarning(WARN_MISUSEDAUTOCVAR, "Autocvar \"%s\" defined as constant", name);
		else if (flags & GDF_STATIC)
			QCC_PR_ParseWarning(WARN_MISUSEDAUTOCVAR, "Autocvar \"%s\" defined as static", name);
		if (!(flags & GDF_STRIP))
			flags |= GDF_USED;
	}

	def = QCC_PR_DummyDef(type, name, scope, arraysize, NULL, ofs, true, flags);

	QCC_ForceUnFreeDef(def);
	return def;
}
QCC_sref_t QCC_PR_GetSRef (QCC_type_t *type, char *name, QCC_function_t *scope, pbool allocate, int arraysize, unsigned int flags)
{
	QCC_sref_t sr;
	QCC_def_t *def = QCC_PR_GetDef(type, name, scope, allocate, arraysize, flags);
	if (def)
	{
		sr.sym = def;
		sr.cast = def->type;
		sr.ofs = 0;
		return sr;
	}
	return nullsref;
}

QCC_def_t *QCC_PR_DummyFieldDef(QCC_type_t *type, char *name, QCC_function_t *scope, int arraysize, unsigned int *fieldofs, unsigned int saved)
{
	char array[64];
	char newname[256];
	int a, parms;
	QCC_def_t *def, *first=NULL;
	unsigned int maxfield, startfield;
	QCC_type_t *ftype;
	pbool isunion;
	startfield = *fieldofs;
	maxfield = startfield;

	for (a = 0; a < (arraysize?arraysize:1); a++)
	{
		if (a == 0)
			*array = '\0';
		else
			QC_snprintfz(array, sizeof(array), "[%i]", a);

		if (*name)
		{
			QC_snprintfz(newname, sizeof(newname), "%s%s", name, array);

			// allocate a new def
			def = (void *)qccHunkAlloc (sizeof(QCC_def_t));
			memset (def, 0, sizeof(*def));
			def->next = NULL;
			def->arraysize = arraysize;

			pr.def_tail->next = def;
			pr.def_tail = def;

			def->s_line = pr_source_line;
			def->s_file = s_file;

			def->name = (void *)qccHunkAlloc (strlen(newname)+1);
			strcpy (def->name, newname);
			def->type = type;

			def->scope = scope;

			if (scope)
				def->ofs = 0;
			else
				def->symboldata[def->ofs]._int = *fieldofs++;
			if (!first)
				first = def;
		}
		else
		{
			def=NULL;
		}

//	printf("Emited %s\n", newname);

		if ((type)->type == ev_struct||(type)->type == ev_union)
		{
			int partnum;
			QCC_type_t *parttype;
			if (def)
				def->referenced = true;
			isunion = ((type)->type == ev_union);
			for (partnum = 0, parms = (type)->num_parms; partnum < parms; partnum++)
			{
				parttype = type->params[partnum].type;
				while(parttype->type == ev_accessor)
					parttype = parttype->parentclass;
				switch (parttype->type)
				{
				case ev_union:
				case ev_struct:
					if (*name && *name != '<')
						QC_snprintfz(newname, sizeof(newname), "%s%s.%s", name, array, type->params[partnum].paramname);
					else
						QC_snprintfz(newname, sizeof(newname), "%s%s", type->params[partnum].paramname, array);
					def = QCC_PR_DummyFieldDef(parttype, newname, scope, 1, fieldofs, saved);
					break;
				case ev_accessor:
				case ev_float:
				case ev_string:
				case ev_vector:
				case ev_entity:
				case ev_field:
				case ev_pointer:
				case ev_integer:
				case ev_variant:
				case ev_function:
					if (*name && *name != '<')
						QC_snprintfz(newname, sizeof(newname), "%s%s.%s", name, array, type->params[partnum].paramname);
					else
						QC_snprintfz(newname, sizeof(newname), "%s%s", type->params[partnum].paramname, array);
					ftype = QCC_PR_NewType("FIELD_TYPE", ev_field, false);
					ftype->aux_type = parttype;
					if (parttype->type == ev_vector)
						ftype->size = parttype->size;	//vector fields create a _y and _z too, so we need this still.
					def = QCC_PR_GetDef(NULL, newname, scope, false, 0, saved);
					if (!def)
					{
						def = QCC_PR_GetDef(ftype, newname, scope, true, 0, saved);
						if (parttype->type == ev_function)
							def->initialized = true;
						def->symboldata->_int = *fieldofs;
						*fieldofs += parttype->size;
					}
					else
					{
						QCC_PR_ParseWarning(WARN_CONFLICTINGUNIONMEMBER, "conflicting offsets for union/struct expansion of %s. Ignoring new def.", newname);
						QCC_PR_ParsePrintDef(WARN_CONFLICTINGUNIONMEMBER, def);
					}
					QCC_FreeDef(def);
					break;
				case ev_void:
					break;
				}
				if (*fieldofs > maxfield)
					maxfield = *fieldofs;
				if (isunion)
					*fieldofs = startfield;
			}
		}
	}

	*fieldofs = maxfield;	//final size of the union.
	return first;
}



void QCC_PR_ExpandUnionToFields(QCC_type_t *type, unsigned int *fields)
{
	QCC_type_t *pass = type->aux_type;
	QCC_PR_DummyFieldDef(pass, "", pr_scope, 1, fields, GDF_SAVED|GDF_CONST);
}

void QCC_PR_ParseInitializerType(int arraysize, QCC_def_t *basedef, QCC_sref_t def)
{
	QCC_sref_t tmp;
	int i;

	if (arraysize)
	{
		//arrays go recursive
		QCC_PR_Expect("{");
		if (!QCC_PR_CheckToken("}"))
		{
			for (i = 0; i < arraysize; i++)
			{
				QCC_PR_ParseInitializerType(0, basedef, def);
				def.ofs += def.cast->size;
				if (!QCC_PR_CheckToken(","))
				{
					QCC_PR_Expect("}");
					break;
				}
				if (QCC_PR_CheckToken("}"))
					break;
			}
		}
	}
	else
	{
		QCC_type_t *type = def.cast;
		if (type->type == ev_function && pr_token_type == tt_punct)
		{
			/*begin function special case*/
			QCC_function_t *parentfunc = pr_scope;
			QCC_function_t *f;
			char fname[256];
			const char *defname = QCC_GetSRefName(def);

			tmp = nullsref;
			*fname = 0;

			if (QCC_PR_CheckToken ("#") || QCC_PR_CheckToken (":"))
			{
				int binum = 0;
				if (pr_token_type == tt_immediate
				&& pr_immediate_type == type_float
				&& pr_immediate._float == (int)pr_immediate._float)
					binum = (int)pr_immediate._float;
				else if (pr_token_type == tt_immediate && pr_immediate_type == type_integer)
					binum = pr_immediate._int;
				else if (pr_token_type == tt_immediate && pr_immediate_type == type_string)
					strncpy(fname, pr_immediate_string, sizeof(fname));
				else if (pr_token_type == tt_name)
					strncpy(fname, pr_token, sizeof(fname));
				else 
					QCC_PR_ParseError (ERR_BADBUILTINIMMEDIATE, "Bad builtin immediate");
				QCC_PR_Lex();

				if (!*fname && QCC_PR_CheckToken (":"))
					strncpy(fname, QCC_PR_ParseName(), sizeof(fname));

				//if the builtin already exists, just use that dfunction instead
				if (basedef->initialized)
				{
					if (*fname)
					{
						for (i = 1; i < numfunctions; i++)
						{
							if (!strcmp(functions[i].name, fname) && functions[i].code == -1 && functions[i].builtin == binum)
							{
								tmp = QCC_MakeIntConst(i);
								break;
							}
						}
					}
					else
					{
						for (i = 1; i < numfunctions; i++)
						{
							if (!strcmp(functions[i].name, defname) && functions[i].code == -1 && functions[i].builtin == binum)
							{
								tmp = QCC_MakeIntConst(i);
								break;
							}
						}
					}
				}

				if (!tmp.cast)
					f = QCC_PR_GenerateBuiltinFunction(def.sym, binum);
				else
					f = NULL;
			}
			else
			{
				if (basedef->initialized == 1)
				{
					//normally this is an error, but to aid supporting new stuff with old, we convert it into a warning if a vanilla(ish) qc function replaces extension builtins.
					//the qc function is the one that is used, but there is a warning so you know how to gain efficiency.
					int bi = -1;
					if (def.cast->type == ev_function && !arraysize)
					{
						if (!strcmp(defname, "anglemod"))
							bi = def.sym->symboldata[def.ofs]._int;
					}
					if (bi <= 0 || bi >= numfunctions)
						bi = 0;
					else
						bi = functions[bi].code;
					if (bi < 0)
					{
						QCC_PR_ParseWarning(WARN_NOTSTANDARDBEHAVIOUR, "%s already declared as builtin", defname);
						QCC_PR_ParsePrintSRef(WARN_NOTSTANDARDBEHAVIOUR, def);
						basedef->initialized = 3;
					}
					else
						QCC_PR_ParseErrorPrintSRef (ERR_REDECLARATION, def, "redeclaration of function body");
				}

				if (pr_scope)
				{
//					QCC_PR_ParseErrorPrintSRef (ERR_INITIALISEDLOCALFUNCTION, def, "initialisation of function body within function body");
					//save some state of the parent
					QCC_def_t *firstlocal = pr.local_head.nextlocal;
					QCC_def_t *lastlocal = pr.local_tail;
					QCC_function_t *parent = pr_scope;
					QCC_statement_t *patch;

					//FIXME: make sure gotos/labels/cases/continues/breaks are not broken by this.

					//generate a goto statement around the nested function, so that nothing is hurt.
					patch = QCC_Generate_OP_GOTO();
					f = QCC_PR_ParseImmediateStatements (NULL, type);
					patch->a.ofs = &statements[numstatements] - patch;

					//make sure parent state is restored properly.
					pr.local_head.nextlocal = firstlocal;
					pr.local_tail = lastlocal;
					pr_scope = parent;
				}
				else
					f = QCC_PR_ParseImmediateStatements (def.sym, type);

				//allow dupes if its a builtin
				if (!f->code && basedef->initialized)
				{
					for (i = 1; i < numfunctions; i++)
					{
						if (functions[i].code == -f->builtin)
						{
							tmp = QCC_MakeIntConst(i);
							break;
						}
					}
				}
			}
			if (!tmp.cast)
			{
				pr_scope = parentfunc;
				tmp = QCC_MakeIntConst(f - functions);
			}
		}
		else if (type->type == ev_string && QCC_PR_CheckName("_"))
		{
			char trname[128];
			QCC_PR_Expect("(");
			if (pr_token_type != tt_immediate || pr_immediate_type->type != ev_string)
				QCC_PR_ParseError(0, "_() intrinsic accepts only a string immediate");
			if (!pr_scope || basedef->constant || basedef->isstatic)
			{
				//if this is a static initialiser, embed a dotranslate in there
				def.sym->symboldata[def.ofs]._int = QCC_CopyString (pr_immediate_string);

				if (!pr_scope || def.sym->constant)
				{
					QCC_def_t *dt;
					QC_snprintfz(trname, sizeof(trname), "dotranslate_%i", ++dotranslate_count);
					dt = QCC_PR_DummyDef(type_string, trname, pr_scope, 0, def.sym, def.ofs, true, GDF_CONST);
					dt->referenced = true;
					dt->constant = 1;
					dt->initialized = 1;
				}
				QCC_PR_Lex();
				QCC_PR_Expect(")");
				return;
			}
			tmp = QCC_MakeTranslateStringConst(pr_immediate_string);
			QCC_PR_Lex();
			QCC_PR_Expect(")");
		}
		else if ((type->type == ev_struct || type->type == ev_union) && QCC_PR_CheckToken("{"))
		{
			//structs go recursive
			unsigned int partnum;
			pbool isunion;
			gofs_t offset = def.ofs;

			isunion = ((type)->type == ev_union);
			for (partnum = 0; partnum < (type)->num_parms; partnum++)
			{
				if (QCC_PR_CheckToken("}"))
					break;

				def.cast = (type)->params[partnum].type;
				def.ofs = offset + (type)->params[partnum].ofs;

				QCC_PR_ParseInitializerType((type)->params[partnum].arraysize, basedef, def);
				if (isunion || !QCC_PR_CheckToken(","))
				{
					QCC_PR_Expect("}");
					break;
				}
			}
			return;
		}
		else
		{
			tmp = QCC_PR_Expression(TOP_PRIORITY, EXPR_DISALLOW_COMMA);
			tmp = QCC_EvaluateCast(tmp, type, true);
		}

		if (!pr_scope || basedef->constant || basedef->isstatic)
		{
			tmp.sym->referenced = true;
			if (!tmp.sym->constant)
			{
				QCC_PR_ParseWarning(ERR_BADIMMEDIATETYPE, "initializer is not constant");
				QCC_PR_ParsePrintSRef(ERR_BADIMMEDIATETYPE, tmp);
			}

			if (!tmp.sym->initialized)
			{
				//FIXME: we NEED to support relocs somehow
				QCC_PR_ParseWarning(WARN_UNINITIALIZED, "initializer is not initialised, %s will be treated as 0", QCC_GetSRefName(tmp));
				QCC_PR_ParsePrintSRef(WARN_UNINITIALIZED, tmp);
			}

			if (basedef->initialized && basedef->initialized != 3)
			{
				for (i = 0; (unsigned)i < type->size; i++)
					if (def.sym->symboldata[def.ofs+i]._int != tmp.sym->symboldata[tmp.ofs+i]._int)
						QCC_PR_ParseErrorPrintSRef (ERR_REDECLARATION, def, "incompatible redeclaration");
			}
			else
			{
				for (i = 0; (unsigned)i < type->size; i++)
					def.sym->symboldata[def.ofs+i]._int = tmp.sym->symboldata[tmp.ofs+i]._int;
			}
		}
		else
		{
			QCC_sref_t rhs = tmp;
			if (def.sym->initialized)
				QCC_PR_ParseErrorPrintSRef (ERR_REDECLARATION, def, "%s initialised twice", basedef->name);

			for (i = 0; (unsigned)i < type->size; )
			{
				if (type->size - i >= 3)
				{
					rhs.cast = def.cast = type_vector;
					if (type->size - i == 3)
					{
						QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_V], rhs, def, NULL, STFL_PRESERVEB));
						return;
					}
					else
						QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_V], rhs, def, NULL, STFL_PRESERVEA|STFL_PRESERVEB));
					i+=3;
					def.ofs += 3;
					rhs.ofs += 3;
				}
				else
				{
					if (def.cast->type == ev_function)
					{
						rhs.cast = def.cast = type_function;
						if (type->size - i == 1)
						{
							QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_FNC], rhs, def, NULL, STFL_PRESERVEB));
							return;
						}
						else
							QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_FNC], rhs, def, NULL, STFL_PRESERVEA|STFL_PRESERVEB));
					}
					else if (def.cast->type == ev_entity)
					{
						rhs.cast = def.cast = type_entity;
						if (type->size - i == 1)
						{
							QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_ENT], rhs, def, NULL, STFL_PRESERVEB));
							return;
						}
						else
							QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_ENT], rhs, def, NULL, STFL_PRESERVEA|STFL_PRESERVEB));
					}
					else if (def.cast->type == ev_field)
					{
						rhs.cast = def.cast = type_field;
						if (type->size - i == 1)
						{
							QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_FLD], rhs, def, NULL, STFL_PRESERVEB));
							return;
						}
						else
							QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_FLD], rhs, def, NULL, STFL_PRESERVEA|STFL_PRESERVEB));
					}
					else if (def.cast->type == ev_integer)
					{
						rhs.cast = def.cast = type_integer;
						if (type->size - i == 1)
						{
							QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_I], rhs, def, NULL, STFL_PRESERVEB));
							return;
						}
						else
							QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_I], rhs, def, NULL, STFL_PRESERVEA|STFL_PRESERVEB));
					}
					else
					{
						rhs.cast = def.cast = type_float;
						if (type->size - i == 1)
						{
							QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], rhs, def, NULL, STFL_PRESERVEB));
							return;
						}
						else
							QCC_FreeTemp(QCC_PR_StatementFlags(&pr_opcodes[OP_STORE_F], rhs, def, NULL, STFL_PRESERVEA|STFL_PRESERVEB));
					}
					i++;
					def.ofs++;
					rhs.ofs++;
				}
			}
		}
		QCC_FreeTemp(tmp);
	}
}

void QCC_PR_ParseInitializerDef(QCC_def_t *def)
{
	QCC_PR_ParseInitializerType(def->arraysize, def, QCC_MakeSRef(def, def->ofs, def->type));
	if (!def->initialized || def->initialized == 3)
		def->initialized = 1;
}

int accglobalsblock;	//0 = error, 1 = var, 2 = function, 3 = objdata


void QCC_PR_ParseEnum(pbool flags)
{
	const char *name;
	QCC_sref_t		sref;
	pbool wantint = false;
	int iv = 0;
	float fv = 0;

	if (QCC_PR_CheckKeyword(keyword_integer, "integer") || QCC_PR_CheckKeyword(keyword_int, "int"))
		wantint = true;
	else if (QCC_PR_CheckKeyword(keyword_float, "float"))
		wantint = false;
	else
		wantint = flag_assume_integer;

	QCC_PR_Expect("{");
	while(1)
	{
		name = QCC_PR_ParseName();
		if (QCC_PR_CheckToken("="))
		{
			if (pr_token_type == tt_immediate && pr_immediate_type->type == ev_float)
			{
				iv = fv = pr_immediate._float;
				QCC_PR_Lex();
			}
			else if (pr_token_type == tt_immediate && pr_immediate_type->type == ev_integer)
			{
				fv = iv = pr_immediate._int;
				QCC_PR_Lex();
			}
			else
			{
				const QCC_eval_t *eval;
				sref = QCC_PR_GetSRef(NULL, QCC_PR_ParseName(), NULL, false, 0, GDF_STRIP);
				eval = QCC_SRef_EvalConst(sref);
				if (eval)
				{
					if (sref.cast->type == ev_float)
						iv = fv = eval->_float;
					else
						fv = iv = eval->_int;
				}
				else if (sref.sym)
					QCC_PR_ParseError(ERR_NOTANUMBER, "enum - %s is not a constant", sref.sym->name);
				else
					QCC_PR_ParseError(ERR_NOTANUMBER, "enum - not a number");

				QCC_FreeTemp(sref);
			}
		}
		if (flags)
		{
			int bits = 0;
			int i = wantint?iv:(int)fv;
			if (!wantint && i != fv)
				QCC_PR_ParseWarning(WARN_ENUMFLAGS_NOTINTEGER, "enumflags - %f not an integer value", fv);
			else
			{
				while(i)
				{
					if (((i>>1)<<1) != i)
						bits++;
					i>>=1;
				}
				if (bits > 1)
					QCC_PR_ParseWarning(WARN_ENUMFLAGS_NOTINTEGER, "enumflags - %f has multiple bits set", fv);
			}
		}
		if (wantint)
			sref = QCC_MakeIntConst(iv);
		else
			sref = QCC_MakeFloatConst(fv);
		pHash_Add(&globalstable, name, sref.sym, qccHunkAlloc(sizeof(bucket_t)));
		QCC_FreeTemp(sref);

		if (flags)
		{
			fv *= 2;
			iv *= 2;
		}
		else
		{
			fv++;
			iv++;
		}

		if (QCC_PR_CheckToken("}"))
			break;
		QCC_PR_Expect(",");
		if (QCC_PR_CheckToken("}"))
			break; // accept trailing comma
	}
}
/*
================
PR_ParseDefs

Called at the outer layer and when a local statement is hit
================
*/
void QCC_PR_ParseDefs (char *classname)
{
	char		*name;
	QCC_type_t		*type, *defclass;
	QCC_def_t		*def, *d;
	QCC_sref_t		dynlength;
	QCC_function_t	*f;
	int			i = 0; // warning: �i� may be used uninitialized in this function
	pbool shared=false;
	pbool isstatic=defaultstatic;
	pbool externfnc=false;
	pbool isconstant = false;
	pbool isvar = false;
	pbool noref = defaultnoref;
	pbool nosave = false;
	pbool allocatenew = true;
	pbool inlinefunction = false;
	pbool allowinline = false;
	pbool dostrip = false;
	pbool forceused = false;
	int arraysize;
	unsigned int gd_flags;

	pr_assumetermtype = NULL;

	while (QCC_PR_CheckToken(";"))
		;

	//FIXME: these should be moved into parsetype
	if (QCC_PR_CheckKeyword(keyword_enum, "enum"))
	{
		QCC_PR_ParseEnum(false);
		QCC_PR_Expect(";");
		return;
	}
	if (QCC_PR_CheckKeyword(keyword_enumflags, "enumflags"))
	{
		QCC_PR_ParseEnum(true);
		QCC_PR_Expect(";");
		return;
	}

	if (QCC_PR_CheckKeyword (keyword_typedef, "typedef"))
	{
		type = QCC_PR_ParseType(true, false);
		if (!type)
		{
			QCC_PR_ParseError(ERR_NOTANAME, "typedef found unexpected tokens");
		}
		if (QCC_PR_CheckToken("*"))
		{
			QCC_type_t *ptr;
			ptr = QCC_PR_NewType(QCC_CopyString(pr_token)+strings, ev_pointer, false);
			ptr->aux_type = type;
			type = ptr;
		}
		else
		{
			type->name = QCC_CopyString(pr_token)+strings;
		}
		type->typedefed = true;
		QCC_PR_Lex();
		QCC_PR_Expect(";");
		return;
	}

	if (flag_acc)
	{
		char *oldp;
		if (QCC_PR_CheckKeyword (keyword_codesys, "CodeSys"))	//reacc support.
		{
			if (ForcedCRC)
				QCC_PR_ParseError(ERR_BADEXTENSION, "progs crc was already specified - only one is allowed");
			ForcedCRC = (int)pr_immediate._float;
			QCC_PR_Lex();
			QCC_PR_Expect(";");
			return;
		}

		oldp = pr_file_p;
		if (QCC_PR_CheckKeyword (keyword_var, "var"))	//reacc support.
		{
			if (accglobalsblock == 3)
			{
				if (!QCC_PR_GetDef(type_void, "end_sys_fields", NULL, false, 0, false))
					QCC_PR_GetDef(type_void, "end_sys_fields", NULL, true, 0, false);
			}

			QCC_PR_ParseName();
			if (QCC_PR_CheckToken(":"))
				accglobalsblock = 1;
			pr_file_p = oldp;
			QCC_PR_Lex();
		}

		if (QCC_PR_CheckKeyword (keyword_function, "function"))	//reacc support.
		{
			accglobalsblock = 2;
		}
		if (QCC_PR_CheckKeyword (keyword_objdata, "objdata"))	//reacc support.
		{
			if (accglobalsblock == 3)
			{
				if (!QCC_PR_GetDef(type_void, "end_sys_fields", NULL, false, 0, false))
					QCC_PR_GetDef(type_void, "end_sys_fields", NULL, true, 0, false);
			}
			else
				if (!QCC_PR_GetDef(type_void, "end_sys_globals", NULL, false, 0, false))
					QCC_PR_GetDef(type_void, "end_sys_globals", NULL, true, 0, false);
			accglobalsblock = 3;
		}
	}

	if (!pr_scope)
	switch(accglobalsblock)//reacc support.
	{
	case 1:
		{
		char *oldp = pr_file_p;
		name = QCC_PR_ParseName();
		if (!QCC_PR_CheckToken(":"))	//nope, it wasn't!
		{
			QCC_PR_IncludeChunk(name, true, NULL);
			QCC_PR_Lex();
			QCC_PR_UnInclude();
			pr_file_p = oldp;
			break;
		}
		if (QCC_PR_CheckKeyword(keyword_object, "object"))
			QCC_PR_GetDef(type_entity, name, NULL, true, 0, true);
		else if (QCC_PR_CheckKeyword(keyword_string, "string"))
			QCC_PR_GetDef(type_string, name, NULL, true, 0, true);
		else if (QCC_PR_CheckKeyword(keyword_real, "real"))
		{
			def = QCC_PR_GetDef(type_float, name, NULL, true, 0, true);
			if (QCC_PR_CheckToken("="))
			{
				def->symboldata[def->ofs]._float = pr_immediate._float;
				QCC_PR_Lex();
			}
		}
		else if (QCC_PR_CheckKeyword(keyword_vector, "vector"))
		{
			def = QCC_PR_GetDef(type_vector, name, NULL, true, 0, true);
			if (QCC_PR_CheckToken("="))
			{
				QCC_PR_Expect("[");
				def->symboldata[def->ofs].vector[0] = pr_immediate._float;
				QCC_PR_Lex();
				def->symboldata[def->ofs].vector[1] = pr_immediate._float;
				QCC_PR_Lex();
				def->symboldata[def->ofs].vector[2] = pr_immediate._float;
				QCC_PR_Lex();
				QCC_PR_Expect("]");
			}
		}
		else if (QCC_PR_CheckKeyword(keyword_pfunc, "pfunc"))
			QCC_PR_GetDef(type_function, name, NULL, true, 0, true);
		else
			QCC_PR_ParseError(ERR_BADNOTTYPE, "Bad type\n");
		QCC_PR_Expect (";");

		if (QCC_PR_CheckKeyword (keyword_system, "system"))
			QCC_PR_Expect (";");
		return;
		}
	case 2:
		name = QCC_PR_ParseName();
		QCC_PR_GetDef(type_function, name, NULL, true, 0, true);
		QCC_PR_CheckToken (";");
		return;
	case 3:
		{
			char *oldp = pr_file_p;
			name = QCC_PR_ParseName();
			if (!QCC_PR_CheckToken(":"))	//nope, it wasn't!
			{
				QCC_PR_IncludeChunk(name, true, NULL);
				QCC_PR_Lex();
				QCC_PR_UnInclude();
				pr_file_p = oldp;
				break;
			}
			if (QCC_PR_CheckKeyword(keyword_object, "object"))
				def = QCC_PR_GetDef(QCC_PR_FieldType(type_entity), name, NULL, true, 0, GDF_CONST|GDF_SAVED);
			else if (QCC_PR_CheckKeyword(keyword_string, "string"))
				def = QCC_PR_GetDef(QCC_PR_FieldType(type_string), name, NULL, true, 0, GDF_CONST|GDF_SAVED);
			else if (QCC_PR_CheckKeyword(keyword_real, "real"))
				def = QCC_PR_GetDef(QCC_PR_FieldType(type_float), name, NULL, true, 0, GDF_CONST|GDF_SAVED);
			else if (QCC_PR_CheckKeyword(keyword_vector, "vector"))
				def = QCC_PR_GetDef(QCC_PR_FieldType(type_vector), name, NULL, true, 0, GDF_CONST|GDF_SAVED);
			else if (QCC_PR_CheckKeyword(keyword_pfunc, "pfunc"))
				def = QCC_PR_GetDef(QCC_PR_FieldType(type_function), name, NULL, true, 0, GDF_CONST|GDF_SAVED);
			else
			{
				QCC_PR_ParseError(ERR_BADNOTTYPE, "Bad type\n");
				QCC_PR_Expect (";");
				return;
			}

			if (!def->initialized)
			{
				unsigned int u;
				def->initialized = 1;
				for (u = 0; u < def->type->size*(def->arraysize?def->arraysize:1); u++)	//make arrays of fields work.
				{
					if (*(int *)&def->symboldata[def->ofs+u])
					{
						QCC_PR_ParseWarning(0, "Field def already has a value:");
						QCC_PR_ParsePrintDef(0, def);
					}
					*(int *)&def->symboldata[def->ofs+u] = pr.size_fields+u;
				}

				pr.size_fields += u;
			}

			QCC_PR_Expect (";");
			return;
		}
	}

	while(1)
	{
		if (QCC_PR_CheckKeyword(keyword_extern, "extern"))
			externfnc=true;
		else if (QCC_PR_CheckKeyword(keyword_shared, "shared"))
		{
			shared=true;
			if (pr_scope)
				QCC_PR_ParseError (ERR_NOSHAREDLOCALS, "Cannot have shared locals");
		}
		else if (QCC_PR_CheckKeyword(keyword_const, "const"))
			isconstant = true;
		else if (QCC_PR_CheckKeyword(keyword_var, "var"))
			isvar = true;
		else if (QCC_PR_CheckKeyword(keyword_static, "static"))
			isstatic = true;
		else if (!pr_scope && QCC_PR_CheckKeyword(keyword_nonstatic, "nonstatic"))
			isstatic = false;
		else if (QCC_PR_CheckKeyword(keyword_noref, "noref"))
			noref=true;
		else if (QCC_PR_CheckKeyword(keyword_unused, "unused"))
			noref=true;
		else if (QCC_PR_CheckKeyword(keyword_used, "used"))
			forceused=true;
		else if (QCC_PR_CheckKeyword(keyword_nosave, "nosave"))
			nosave = true;
		else if (QCC_PR_CheckKeyword(keyword_strip, "strip"))
			dostrip = true;
		else if (QCC_PR_CheckKeyword(keyword_inline, "inline"))
			allowinline = true;
		else if (QCC_PR_CheckKeyword(keyword_ignore, "ignore"))
			dostrip = true;
		else
			break;
	}

	type = QCC_PR_ParseType (false, false);
	if (type == NULL)	//ignore
		return;

	inlinefunction = type_inlinefunction;

	if (externfnc && type->type != ev_function)
	{
		printf ("Only functions may be defined as external (yet)\n");
		externfnc=false;
	}

	if (!pr_scope && QCC_PR_CheckKeyword(keyword_function, "function"))	//reacc support.
	{
		name = QCC_PR_ParseName ();
		QCC_PR_Expect("(");
		type = QCC_PR_ParseFunctionTypeReacc(false, type);
		QCC_PR_Expect(";");

		def = QCC_PR_GetDef (type, name, NULL, true, 0, false);

		if (autoprototype || dostrip)
		{	//ignore the code and stuff

			if (QCC_PR_CheckKeyword(keyword_external, "external"))
			{	//builtin
				QCC_PR_Lex();
				QCC_PR_Expect(";");
			}
			else
			{
				int blev = 1;

				while (!QCC_PR_CheckToken("{"))	//skip over the locals.
				{
					if (pr_token_type == tt_eof)
					{
						QCC_PR_ParseError(0, "Unexpected EOF");
						break;
					}
					QCC_PR_Lex();
				}

				//balance out the { and }
				while(blev)
				{
					if (pr_token_type == tt_eof)
						break;
					if (QCC_PR_CheckToken("{"))
						blev++;
					else if (QCC_PR_CheckToken("}"))
						blev--;
					else
						QCC_PR_Lex();	//ignore it.
				}
			}
			return;
		}
		else
		{
			def->referenced = true;

			f = QCC_PR_ParseImmediateStatements (def, type);

			def->initialized = 1;
			def->isstatic = isstatic;
			def->symboldata[def->ofs].function = numfunctions;
			f->def = def;
	//				if (pr_dumpasm)
	//					PR_PrintFunction (def);

			if (numfunctions >= MAX_FUNCTIONS)
				QCC_Error(ERR_INTERNAL, "Too many function defs");
		}
		return;
	}

//	if (pr_scope && (type->type == ev_field) )
//		QCC_PR_ParseError ("Fields must be global");

	do
	{		
		if (QCC_PR_CheckToken (";"))
		{
			if (type->type == ev_field && (type->aux_type->type == ev_union || type->aux_type->type == ev_struct))
			{
				QCC_PR_ExpandUnionToFields(type, &pr.size_fields);
				return;
			}
			if (type->type == ev_struct && strcmp(type->name, "struct"))
				return;	//allow named structs
			if (type->type == ev_entity && type != type_entity)
				return;	//allow forward class definititions with or without a variable. 
			if (type->type == ev_accessor)	//accessors shouldn't trigger problems if they're just a type.
				return;
//			if (type->type == ev_union)
//			{
//				return;
//			}
			QCC_PR_ParseError (ERR_TYPEWITHNONAME, "type (%s) with no name", type->name);
			name = NULL;
		}
		else
		{
			name = QCC_PR_ParseName ();
		}

		if (QCC_PR_CheckToken("::") && !classname)
		{
			classname = name;
			name = QCC_PR_ParseName();
		}

//check for an array

		dynlength = nullsref;
		if ( QCC_PR_CheckToken ("[") )
		{
			char *oldprfile = pr_file_p;
			int oldline = pr_source_line;
			int depth;
			arraysize = 0;
			if (QCC_PR_CheckToken("]"))
			{
				//FIXME: preprocessor will hate this with a passion.
				QCC_PR_Expect("=");
				QCC_PR_Expect("{");
				arraysize++;
				depth = 1;
				while(1)
				{
					if(pr_token_type == tt_eof)
					{
						QCC_PR_ParseError (ERR_EOF, "EOF inside definition of %s", name);
						break;
					}
					else if (depth == 1 && QCC_PR_CheckToken(","))
					{
						if (QCC_PR_CheckToken("}"))
							break;
						arraysize++;
					}
					else if (QCC_PR_CheckToken("{") || QCC_PR_CheckToken("["))
						depth++;
					else if (QCC_PR_CheckToken("}") || QCC_PR_CheckToken("]"))
					{
						depth--;
						if (depth == 0)
							break;
					}
					else
						QCC_PR_Lex();
				}
				pr_file_p = oldprfile;
				pr_source_line = oldline;
				QCC_PR_Lex();
			}
			else
			{
				const QCC_eval_t *eval;
				arraysize = 0;
				dynlength = QCC_PR_Expression(TOP_PRIORITY, 0);
				eval = QCC_SRef_EvalConst(dynlength);
				if (eval)
				{
					if (dynlength.cast->type == ev_integer)
						arraysize = eval->_int;
					else if (dynlength.cast->type == ev_float)
					{
						int i = eval->_float;
						if ((float)i == eval->_float)
							arraysize = i;
					}
					else
						QCC_PR_ParseError (ERR_BADARRAYSIZE, "Definition of array (%s) size is not of a numerical value", name);
					QCC_FreeTemp(dynlength);
					dynlength = nullsref;
				}
				else if (!pr_scope)
					dynlength = nullsref;
				QCC_PR_Expect("]");
			}

			if (arraysize < 1 && !dynlength.cast)
			{
				QCC_PR_ParseError (ERR_BADARRAYSIZE, "Definition of array (%s) size is not of a numerical value", name);
				arraysize=1;	//grrr...
			}
		}
		else
			arraysize = 0;

		if (QCC_PR_CheckToken("("))
		{
			if (inlinefunction)
				QCC_PR_ParseWarning(WARN_UNSAFEFUNCTIONRETURNTYPE, "Function returning function. Is this what you meant? (suggestion: use typedefs)");
			inlinefunction = false;
			type = QCC_PR_ParseFunctionType(false, type);
		}

		if (classname)
		{
			char *membername = name;
			name = qccHunkAlloc(strlen(classname) + strlen(name) + 3);
			sprintf(name, "%s::%s", classname, membername);
			defclass = QCC_TypeForName(classname);
			if (!defclass || !defclass->parentclass)
				QCC_PR_ParseError(ERR_NOTANAME, "%s is not a class\n", classname);
		}
		else
			defclass = NULL;

		gd_flags = 0;
		if (isstatic)
			gd_flags |= GDF_STATIC;
		if (isconstant || (type->type == ev_function && !isvar))
			gd_flags |= GDF_CONST;
		if (!nosave)
			gd_flags |= GDF_SAVED;
		if (allowinline)
			gd_flags |= GDF_INLINE;
		if (dostrip)
			gd_flags |= GDF_STRIP;
		else if (forceused)	//FIXME: make proper pragma(used) thingie
			gd_flags |= GDF_USED;

#if IAMNOTLAZY
		if (dynlength.cast)
		{
			def = QCC_PR_GetDef (QCC_PR_PointerType(type), name, pr_scope, allocatenew, 0, gd_flags);
			dynlength = QCC_SupplyConversion(dynlength, ev_integer, true);
			if (type->size != 1)
				dynlength = QCC_PR_Statement(pr_opcodes+OP_MUL_I, dynlength, QCC_MakeIntConst(type->size), NULL);
			QCC_PR_SimpleStatement(&pr_opcodes[OP_PUSH], dynlength, nullsref, def, false);	//push *(int*)&a elements
		}
		else
#endif
			def = QCC_PR_GetDef (type, name, pr_scope, allocatenew, arraysize, gd_flags);

		if (!def)
			QCC_PR_ParseError(ERR_NOTANAME, "%s is not part of class %s", name, classname);

		if (noref)
		{
			if (type->type == ev_function && !def->initialized)
				def->initialized = 3;
			def->referenced = true;
		}

		if (!def->initialized && shared)	//shared count as initiialised
		{
			def->shared = shared;
			def->initialized = true;
		}
		if (externfnc)
			def->initialized = 2;

		if (isstatic)
		{
			if (!strcmp(strings+def->s_file, strings+s_file))
				def->isstatic = isstatic;
			else //if (type->type != ev_function && defaultstatic)	//functions don't quite consitiute a definition
				QCC_PR_ParseErrorPrintDef (ERR_REDECLARATION, def, "can't redefine non-static as static");
		}

// check for an initialization
		/*if (type->type == ev_function && (pr_scope))
		{
			if ( QCC_PR_CheckToken ("=") )
			{
				QCC_PR_ParseError (ERR_INITIALISEDLOCALFUNCTION, "local functions may not be initialised");
			}

			d = def;
			while (d != def->deftail)
			{
				d = d->next;
				d->initialized = 1;	//fake function
				d->symboldata[d->ofs].function = 0;
			}

			continue;
		}*/

		if (type->type == ev_field && QCC_PR_CheckName ("alias"))
		{
			QCC_PR_ParseError(ERR_INTERNAL, "FTEQCC does not support this variant of decompiled hexenc\nPlease obtain the original version released by Raven Software instead.");
			name = QCC_PR_ParseName();
		}
		else if ( QCC_PR_CheckToken ("=") || ((type->type == ev_function) && (pr_token[0] == '{' || pr_token[0] == '[' || pr_token[0] == ':')))	//this is an initialisation (or a function)
		{
			QCC_type_t *parentclass;
			if (def->shared)
				QCC_PR_ParseError (ERR_SHAREDINITIALISED, "shared values may not be assigned an initial value", name);
			if (def->initialized == 1)
			{
//				if (def->type->type == ev_function)
//				{
//					i = G_FUNCTION(def->ofs);
//					df = &functions[i];
//					QCC_PR_ParseErrorPrintDef (ERR_REDECLARATION, def, "%s redeclared, prev instance is in %s", name, strings+df->s_file);
//				}
//				else
//					QCC_PR_ParseErrorPrintDef(ERR_REDECLARATION, def, "%s redeclared", name);
			}

			if (autoprototype || dostrip)
			{	//ignore the code and stuff
				if (dostrip && !def->initialized)
					def->initialized = 3;
				if (dostrip)
					def->referenced = true;
				if (QCC_PR_CheckToken("["))
				{
					while (!QCC_PR_CheckToken("]"))
					{
						if (pr_token_type == tt_eof)
							break;
						QCC_PR_Lex();
					}
				}
				if (QCC_PR_CheckToken("{"))
				{
					int blev = 1;
					//balance out the { and }
					while(blev)
					{
						if (pr_token_type == tt_eof)
							break;
						if (QCC_PR_CheckToken("{"))
							blev++;
						else if (QCC_PR_CheckToken("}"))
							blev--;
						else
							QCC_PR_Lex();	//ignore it.
					}
				}
				else
				{
					if (type->type == ev_string && QCC_PR_CheckName("_"))
					{
						QCC_PR_Expect("(");
						QCC_PR_Lex();
						QCC_PR_Expect(")");
					}
					else
					{
						QCC_PR_CheckToken("#");
						do
						{
							QCC_PR_Lex();
						} while (*pr_token && strcmp(pr_token, ",") && strcmp(pr_token, ";"));
					}
				}
				QCC_FreeDef(def);
				continue;
			}

			parentclass = pr_classtype;
			pr_classtype = defclass?defclass:pr_classtype;
			def->constant = (isconstant || (!isvar && !pr_scope));
			QCC_PR_ParseInitializerDef(def);
			QCC_FreeDef(def);
			pr_classtype = parentclass;
		}
		else
		{
			if (type->type == ev_function)
				isconstant = !isvar;

			if (dostrip)
			{
				def->constant = isconstant;
				def->referenced = true;
			}
			else if (type->type == ev_field)
			{
				//fields are const by default, even when not initialised (as they are initialised behind the scenes)
				if (isconstant)
					def->constant = 2;	//special flag on fields, 2, makes the pointer obtained from them also constant.
				else if (isvar || (pr_scope && !isstatic))
					def->constant = 0;
				else
					def->constant = 1;

				if (!def->initialized && def->constant)
				{
					unsigned int i;
					def->initialized = true;
					//if the field already has a value, don't allocate new field space for it as that would confuse things.
					//otherwise allocate new space.
					if (def->symboldata[def->ofs]._int)
					{
						for (i = 0; i < type->size*(arraysize?arraysize:1); i++)	//make arrays of fields work.
						{
							if (def->symboldata[def->ofs+i]._int != i + def->symboldata[def->ofs]._int)
							{
								QCC_PR_ParseWarning(0, "Inconsistant field def:");
								QCC_PR_ParsePrintDef(0, def);
								break;
							}
						}
					}
					else
					{
						for (i = 0; i < type->size*(arraysize?arraysize:1); i++)	//make arrays of fields work.
						{
							if (def->symboldata[def->ofs+i]._int)
							{
								QCC_PR_ParseWarning(0, "Field def already has a value:");
								QCC_PR_ParsePrintDef(0, def);
							}
							def->symboldata[def->ofs+i]._int = pr.size_fields+i;
						}

						pr.size_fields += i;
					}
				}
			}
			else
				def->constant = isconstant;
		}

		d = def;
		QCC_FreeDef(d);
		while (d != def->deftail)
		{
			d = d->next;
			d->constant = def->constant;
			d->initialized = def->initialized;
		}
	} while (QCC_PR_CheckToken (","));

	if (type->type == ev_function)
		QCC_PR_CheckTokenComment (";", &def->comment);
	else
	{
		if (!QCC_PR_CheckTokenComment (";", &def->comment))
			QCC_PR_ParseWarning(WARN_UNDESIRABLECONVENTION, "Missing semicolon at end of definition");
	}
}

/*
============
PR_CompileFile

compiles the 0 terminated text, adding defintions to the pr structure
============
*/
pbool	QCC_PR_CompileFile (char *string, char *filename)
{
	jmp_buf oldjb;
	if (!pr.memory)
		QCC_Error (ERR_INTERNAL, "PR_CompileFile: Didn't clear");

	QCC_PR_ClearGrabMacros (true);	// clear the frame macros

	compilingfile = filename;

	if (opt_filenames)
	{
		optres_filenames += strlen(filename);
		pr_file_p = qccHunkAlloc(strlen(filename)+1);
		strcpy(pr_file_p, filename);
		s_file = pr_file_p - strings;
		s_file2 = 0;
	}
	else
	{
		s_file = s_file2 = QCC_CopyString (filename);
	}
	pr_file_p = string;
	pr_assumetermtype = NULL;

	pr_source_line = 0;

	memcpy(&oldjb, &pr_parse_abort, sizeof(oldjb));

	if( setjmp( pr_parse_abort ) ) {
		// dont count it as error
	} else {
		//clock up the first line
		QCC_PR_NewLine (false);

		QCC_PR_Lex ();	// read first token
	}

	while (pr_token_type != tt_eof)
	{
		if (setjmp(pr_parse_abort))
		{
			num_continues = 0;
			num_breaks = 0;
			num_cases = 0;
			if (++pr_error_count > MAX_ERRORS)
			{
				memcpy(&pr_parse_abort, &oldjb, sizeof(oldjb));
				return false;
			}
			QCC_PR_SkipToSemicolon ();
			if (pr_token_type == tt_eof)
			{
				memcpy(&pr_parse_abort, &oldjb, sizeof(oldjb));
				return false;
			}
		}

		pr_scope = NULL;	// outside all functions

		QCC_PR_ParseDefs (NULL);

#ifdef _DEBUG
		if (!pr_error_count)
		{
			QCC_def_t *d;
			unsigned int i;
			for (d = pr.def_head.next; d; d = d->next)
			{
				if (d->refcount)
				{
					QCC_sref_t sr;
					sr.sym = d;
					sr.cast = d->type;
					sr.ofs = 0;
					QCC_PR_ParseWarning(WARN_DEBUGGING, "INTERNAL: %i references still held on %s (%s)", d->refcount, d->name, QCC_VarAtOffset(sr, d->type->size));
					d->refcount = 0;
				}
			}
			for (i = 0; i < tempsused; i++)
			{
				d = tempsinfo[i].def;
				if (d->refcount)
				{
					QCC_sref_t sr;
					sr.sym = d;
					sr.cast = d->type;
					sr.ofs = 0;
					QCC_PR_ParseWarning(WARN_DEBUGGING, "INTERNAL: %i references still held on %s (%s)", d->refcount, d->name, QCC_VarAtOffset(sr, d->type->size));
					d->refcount = 0;
				}
			}
		}
#endif
	}
	memcpy(&pr_parse_abort, &oldjb, sizeof(oldjb));

	return (pr_error_count == 0);
}

pbool QCC_Include(char *filename)
{
	char *newfile;
	char fname[512];
	char *opr_file_p;
	QCC_string_t os_file, os_file2;
	int opr_source_line;
	char *ocompilingfile;
	struct qcc_includechunk_s *oldcurrentchunk;

	ocompilingfile = compilingfile;
	os_file = s_file;
	os_file2 = s_file2;
	opr_source_line = pr_source_line;
	opr_file_p = pr_file_p;
	oldcurrentchunk = currentchunk;

	strcpy(fname, filename);
	QCC_LoadFile(fname, (void*)&newfile);
	currentchunk = NULL;
	pr_file_p = newfile;
	QCC_PR_CompileFile(newfile, fname);
	currentchunk = oldcurrentchunk;

	compilingfile = ocompilingfile;
	s_file = os_file;
	s_file2 = os_file2;
	pr_source_line = opr_source_line;
	pr_file_p = opr_file_p;

	if (pr_error_count > MAX_ERRORS)
		longjmp (pr_parse_abort, 1);

//	QCC_PR_IncludeChunk(newfile, false, fname);

	return true;
}
void QCC_Cleanup(void)
{
	free(pr_breaks);
	free(pr_continues);
	free(pr_cases);
	free(pr_casesdef);
	free(pr_casesdef2);
	max_breaks = max_continues = max_cases = num_continues = num_breaks = num_cases = 0;
	pr_breaks = NULL;
	pr_continues = NULL;
	pr_cases = NULL;
	pr_casesdef = NULL;
	pr_casesdef2 = NULL;
}
#endif
