#define PROGSUSED
#include "progsint.h"

#define HunkAlloc BADGDFG sdfhhsf FHS

void PR_SetBuiltins(int type);
/*
progstate_t *pr_progstate;
progsnum_t pr_typecurrent;
int maxprogs;

progstate_t *current_progstate;
int numshares;

sharedvar_t *shares;	//shared globals, not including parms
int maxshares;
*/

//switches progs without preserving parms/ret/shared
pbool PR_SwitchProgs(progfuncs_t *progfuncs, progsnum_t type)
{	
	if ((unsigned)type >= maxprogs)
	{
		if (type == -1)
		{
			pr_typecurrent = -1;
			current_progstate = NULL;
			return true;
		}
		PR_RunError(&progfuncs->funcs, "QCLIB: Bad prog type - %i", type);
//		Sys_Error("Bad prog type - %i", type);
	}

	if (pr_progstate[(unsigned)type].progs == NULL)	//we havn't loaded it yet, for some reason
		return false;	

	current_progstate = &pr_progstate[(unsigned)type];

	pr_typecurrent = type;

	return true;
}

//switch to new progs, preserving all arguments. oldpr should be 'pr_typecurrent'
pbool PR_SwitchProgsParms(progfuncs_t *progfuncs, progsnum_t newpr)	//from 2 to 1
{
	unsigned int a;
	progstate_t *np;
	progstate_t *op;
	int oldpr = pr_typecurrent;

	if (newpr == oldpr)
	{
		//don't bother coping variables to themselves...
		return true;
	}

	np = &pr_progstate[(int)newpr];
	op = &pr_progstate[(int)oldpr];

	if ((unsigned)newpr >= maxprogs || !np->globals)
	{
		printf("QCLIB: Bad prog type - %i", newpr);
		return false;
	}
	if ((unsigned)oldpr >= maxprogs || !op->globals)	//startup?
		return PR_SwitchProgs(progfuncs, newpr);

	//copy parms.
	for (a = 0; a < MAX_PARMS;a++)
	{
		*(int *)&np->globals[OFS_PARM0+3*a  ] = *(int *)&op->globals[OFS_PARM0+3*a  ];
		*(int *)&np->globals[OFS_PARM0+3*a+1] = *(int *)&op->globals[OFS_PARM0+3*a+1];
		*(int *)&np->globals[OFS_PARM0+3*a+2] = *(int *)&op->globals[OFS_PARM0+3*a+2];
	}
	np->globals[OFS_RETURN] = op->globals[OFS_RETURN];
	np->globals[OFS_RETURN+1] = op->globals[OFS_RETURN+1];
	np->globals[OFS_RETURN+2] = op->globals[OFS_RETURN+2];

	//move the vars defined as shared.
	for (a = 0; a < numshares; a++)//fixme: make offset per progs
	{
		memmove(&((int *)np->globals)[shares[a].varofs], &((int *)op->globals)[shares[a].varofs], shares[a].size*4);
/*		((int *)p1->globals)[shares[a].varofs] = ((int *)p2->globals)[shares[a].varofs];
		if (shares[a].size > 1)
		{
			((int *)p1->globals)[shares[a].varofs+1] = ((int *)p2->globals)[shares[a].varofs+1];
			if (shares[a].size > 2)
				((int *)p1->globals)[shares[a].varofs+2] = ((int *)p2->globals)[shares[a].varofs+2];
		}
*/
	}
	return PR_SwitchProgs(progfuncs, newpr);
}

progsnum_t PDECL PR_LoadProgs(pubprogfuncs_t *ppf, char *s, int headercrc, builtin_t *builtins, int numbuiltins)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	unsigned int a;
	progsnum_t oldtype;
	oldtype = pr_typecurrent;	
	for (a = 0; a < maxprogs; a++)
	{
		if (pr_progstate[a].progs == NULL)
		{
			pr_typecurrent = a;
			current_progstate = &pr_progstate[a];
			if (PR_ReallyLoadProgs(progfuncs, s, headercrc, &pr_progstate[a], false))	//try and load it			
			{
				current_progstate->builtins = builtins;
				current_progstate->numbuiltins = numbuiltins;
				if (a <= progfuncs->funcs.numprogs)
					progfuncs->funcs.numprogs = a+1;

#ifdef QCJIT
				current_progstate->jit = PR_GenerateJit(progfuncs);
#endif
				if (oldtype != -1)
					PR_SwitchProgs(progfuncs, oldtype);
				return a;	//we could load it. Yay!
			}
			PR_SwitchProgs(progfuncs, oldtype);
			return -1; // loading failed.
		}
	}
	PR_SwitchProgs(progfuncs, oldtype);
	return -1;
}

void PR_ShiftParms(progfuncs_t *progfuncs, int amount)
{
	int a;
	for (a = 0; a < MAX_PARMS - amount;a++)
		*(int *)&pr_globals[OFS_PARM0+3*a] = *(int *)&pr_globals[OFS_PARM0+3*(amount+a)];
}

//forget a progs
void PR_Clear(progfuncs_t *progfuncs)
{
	unsigned int a;
	for (a = 0; a < maxprogs; a++)
	{
#ifdef QCJIT
		if (pr_progstate[a].jit)
			PR_CloseJit(pr_progstate[a].jit);
#endif
		pr_progstate[a].progs = NULL;
	}
}



void QC_StartShares(progfuncs_t *progfuncs)
{
	numshares = 0;
	maxshares = 32;
	if (shares)
		externs->memfree(shares);
	shares = externs->memalloc(sizeof(sharedvar_t)*maxshares);
}
void PDECL QC_AddSharedVar(pubprogfuncs_t *ppf, int start, int size)	//fixme: make offset per progs and optional
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
	int ofs;
	unsigned int a;

	if (numshares >= maxshares)
	{
		void *buf;
		buf = shares;
		maxshares += 16;		
		shares = externs->memalloc(sizeof(sharedvar_t)*maxshares);

		memcpy(shares, buf, sizeof(sharedvar_t)*numshares);

		externs->memfree(buf);
	}
	ofs = start;
	for (a = 0; a < numshares; a++)
	{
		if (shares[a].varofs+shares[a].size == ofs)
		{
			shares[a].size += size;	//expand size.
			return;
		}
		if (shares[a].varofs == start)
			return;
	}


	shares[numshares].varofs = start;
	shares[numshares].size = size;
	numshares++;
}


//void ShowWatch(void);

void QC_InitShares(progfuncs_t *progfuncs)
{
//	ShowWatch();
	if (!prinst.field)	//don't make it so we will just need to remalloc everything
	{
		prinst.maxfields = 64;
		prinst.field = externs->memalloc(sizeof(fdef_t) * prinst.maxfields);
	}

	prinst.numfields = 0;
	progfuncs->funcs.fieldadjust = 0;
}

void QC_FlushProgsOffsets(progfuncs_t *progfuncs)
{	//sets the fields up for loading a new progs.
	//fields are matched by name to other progs
	//not by offset
	unsigned int i;
	for (i = 0; i < prinst.numfields; i++)
		prinst.field[i].progsofs = -1;
}


//called if a global is defined as a field
//returns offset.

//vectors must be added before any of their corresponding _x/y/z vars
//in this way, even screwed up progs work.

//requestedpos is the offset the engine WILL put it at.
//origionaloffs is used to track matching field offsets. fields with the same progs offset overlap

//note: we probably suffer from progs with renamed system globals.
int PDECL QC_RegisterFieldVar(pubprogfuncs_t *ppf, unsigned int type, char *name, signed long engineofs, signed long progsofs)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
//	progstate_t *p;
//	int pnum;
	unsigned int i;
	int namelen;
	int ofs;

	int fnum;

	if (!name)	//engine can use this to offset all progs fields
	{			//which fixes constant field offsets (some ktpro arrays)
		progfuncs->funcs.fieldadjust = fields_size/4;
//		printf("FIELD ADJUST: %i %i %i\n", progfuncs->funcs.fieldadjust, fields_size, (int)fields_size/4);
		return 0;
	}


	prinst.reorganisefields = true;

	//look for an existing match
	for (i = 0; i < prinst.numfields; i++)
	{		
		if (!strcmp(name, prinst.field[i].name))
		{
			if (prinst.field[i].type != type)
			{
				/*Hexen2/DP compat hack: if the new type is a float and the original type is a vector, make the new def alias to the engine's _x field
				this 'works around' the unused .vector color field used for rtlight colours vs the .float color used for particle colours (the float initialisers in map files will expand into the x slot safely).
				qc/hc can work around this by just using .vector color/color_x instead, which is the same as this hack, but would resolve defs to allow rtlight colours.
				*/
				if (prinst.field[i].type != ev_vector || type != ev_float)
				{
					printf("Field type mismatch on \"%s\". %i != %i\n", name, prinst.field[i].type, type);
					continue;
				}
			}
			if (!progfuncs->funcs.fieldadjust && engineofs>=0)
				if ((unsigned)engineofs/4 != prinst.field[i].ofs)
					Sys_Error("Field %s at wrong offset", name);

			if (prinst.field[i].progsofs == -1)
				prinst.field[i].progsofs = progsofs;
//			printf("Dupfield %s %i -> %i\n", name, prinst.field[i].progsofs,prinst.field[i].ofs);
			return prinst.field[i].ofs-progfuncs->funcs.fieldadjust;	//got a match
		}
	}

	if (prinst.numfields+1>prinst.maxfields)
	{
		fdef_t *nf;
		i = prinst.maxfields;
		prinst.maxfields += 32;
		nf = externs->memalloc(sizeof(fdef_t) * prinst.maxfields);
		memcpy(nf, prinst.field, sizeof(fdef_t) * i);
		externs->memfree(prinst.field);
		prinst.field = nf;
	}

	//try to add a new one
	fnum = prinst.numfields;
	prinst.numfields++;
	prinst.field[fnum].name = name;	
	if (type == ev_vector)
	{
		char *n;		
		namelen = strlen(name)+5;	

		n=PRHunkAlloc(progfuncs, namelen, "str");
		sprintf(n, "%s_x", name);
		ofs = QC_RegisterFieldVar(&progfuncs->funcs, ev_float, n, engineofs, progsofs);
		prinst.field[fnum].ofs = ofs+progfuncs->funcs.fieldadjust;

		n=PRHunkAlloc(progfuncs, namelen, "str");
		sprintf(n, "%s_y", name);
		QC_RegisterFieldVar(&progfuncs->funcs, ev_float, n, (engineofs==-1)?-1:(engineofs+4), (progsofs==-1)?-1:progsofs+1);

		n=PRHunkAlloc(progfuncs, namelen, "str");
		sprintf(n, "%s_z", name);
		QC_RegisterFieldVar(&progfuncs->funcs, ev_float, n, (engineofs==-1)?-1:(engineofs+8), (progsofs==-1)?-1:progsofs+2);
	}
	else if (engineofs >= 0)
	{	//the engine is setting up a list of required field indexes.

		//paranoid checking of the offset.
	/*	for (i = 0; i < numfields-1; i++)
		{
			if (field[i].ofs == ((unsigned)engineofs)/4)
			{
				if (type == ev_float && field[i].type == ev_vector)	//check names
				{
					if (strncmp(field[i].name, name, strlen(field[i].name)))
						Sys_Error("Duplicated offset");
				}
				else
					Sys_Error("Duplicated offset");
			}
		}*/
		if (engineofs&3)
			Sys_Error("field %s is %i&3", name, (int)engineofs);
		prinst.field[fnum].ofs = ofs = engineofs/4;
	}
	else
	{	//we just found a new fieldname inside a progs
		prinst.field[fnum].ofs = ofs = fields_size/4;	//add on the end

		//if the progs field offset matches annother offset in the same progs, make it match up with the earlier one.
		if (progsofs>=0)
		{
			for (i = 0; i < prinst.numfields-1; i++)
			{
				if (prinst.field[i].progsofs == (unsigned)progsofs)
				{
//					printf("found union field %s %i -> %i\n", prinst.field[i].name, prinst.field[i].progsofs, prinst.field[i].ofs);
					prinst.field[fnum].ofs = ofs = prinst.field[i].ofs;
					break;
				}
				if (prinst.field[i].type == ev_vector && prinst.field[i].progsofs+1 == (unsigned)progsofs)
				{
//					printf("found union field %s %i -> %i\n", prinst.field[i].name, prinst.field[i].progsofs+1, prinst.field[i].ofs+1);
					prinst.field[fnum].ofs = ofs = prinst.field[i].ofs+1;
					break;
				}
				if (prinst.field[i].type == ev_vector && prinst.field[i].progsofs+2 == (unsigned)progsofs)
				{
//					printf("found union field %s %i -> %i\n", prinst.field[i].name, prinst.field[i].progsofs+2, prinst.field[i].ofs+2);
					prinst.field[fnum].ofs = ofs = prinst.field[i].ofs+2;
					break;
				}
			}
		}
	}
//	if (type != ev_vector)
		if (fields_size < (ofs+type_size[type])*4)
			fields_size = (ofs+type_size[type])*4;

	if (max_fields_size && fields_size > max_fields_size)
		Sys_Error("Allocated too many additional fields after ents were inited.");
	prinst.field[fnum].type = type;

	prinst.field[fnum].progsofs = progsofs;

//	printf("Field %s %i -> %i\n", name, prinst.field[fnum].progsofs,prinst.field[fnum].ofs);
	
	//we've finished setting the structure	
	return ofs - progfuncs->funcs.fieldadjust;
}


//called if a global is defined as a field
void PDECL QC_AddSharedFieldVar(pubprogfuncs_t *ppf, int num, char *stringtable)
{
	progfuncs_t *progfuncs = (progfuncs_t*)ppf;
//	progstate_t *p;
//	int pnum;
	unsigned int i, o;

	//look for an existing match not needed, cos we look a little later too.
	/*
	for (i = 0; i < numfields; i++)
	{		
		if (!strcmp(pr_globaldefs[num].s_name, field[i].s_name))
		{
			//really we should look for a field def

			*(int *)&pr_globals[pr_globaldefs[num].ofs] = field[i].ofs;	//got a match

			return;
		}
	}
	*/
	
	switch(current_progstate->structtype)
	{
	case PST_KKQWSV:
	case PST_DEFAULT:
		for (i=1 ; i<pr_progs->numfielddefs; i++)
		{
			if (!strcmp(pr_fielddefs16[i].s_name+stringtable, pr_globaldefs16[num].s_name+stringtable))
			{
//				int old = *(int *)&pr_globals[pr_globaldefs16[num].ofs];
				*(int *)&pr_globals[pr_globaldefs16[num].ofs] = QC_RegisterFieldVar(&progfuncs->funcs, pr_fielddefs16[i].type, pr_globaldefs16[num].s_name+stringtable, -1, *(int *)&pr_globals[pr_globaldefs16[num].ofs]);
//				printf("Field=%s global %i -> %i\n", pr_globaldefs16[num].s_name+stringtable, old, *(volatile int *)&pr_globals[pr_globaldefs16[num].ofs]);
				return;
			}
		}

		for (i = 0; i < prinst.numfields; i++)
		{
			o = prinst.field[i].progsofs;
			if (o == *(unsigned int *)&pr_globals[pr_globaldefs16[num].ofs])
			{
//				int old = *(int *)&pr_globals[pr_globaldefs16[num].ofs];
				*(int *)&pr_globals[pr_globaldefs16[num].ofs] = prinst.field[i].ofs-progfuncs->funcs.fieldadjust;
//				printf("Field global=%s %i -> %i\n", pr_globaldefs16[num].s_name+stringtable, old, *(volatile int *)&pr_globals[pr_globaldefs16[num].ofs]);
				return;
			}
		}

		//oh well, must be a parameter.
//		if (*(int *)&pr_globals[pr_globaldefs16[num].ofs])
//			Sys_Error("QCLIB: Global field var with no matching field \"%s\", from offset %i", pr_globaldefs16[num].s_name+stringtable, *(int *)&pr_globals[pr_globaldefs16[num].ofs]);
		return;
	case PST_FTE32:
	case PST_QTEST:
		for (i=1 ; i<pr_progs->numfielddefs; i++)
		{
			if (!strcmp(pr_fielddefs32[i].s_name+stringtable, pr_globaldefs32[num].s_name+stringtable))
			{
				*(int *)&pr_globals[pr_globaldefs32[num].ofs] = QC_RegisterFieldVar(&progfuncs->funcs, pr_fielddefs32[i].type, pr_globaldefs32[num].s_name+stringtable, -1, *(int *)&pr_globals[pr_globaldefs32[num].ofs]);
				return;
			}
		}

		for (i = 0; i < prinst.numfields; i++)
		{
			o = prinst.field[i].progsofs;
			if (o == *(unsigned int *)&pr_globals[pr_globaldefs32[num].ofs])
			{
				*(int *)&pr_globals[pr_globaldefs32[num].ofs] = prinst.field[i].ofs-progfuncs->funcs.fieldadjust;
				return;
			}
		}

		//oh well, must be a parameter.
		if (*(int *)&pr_globals[pr_globaldefs32[num].ofs])
			Sys_Error("QCLIB: Global field var with no matching field \"%s\", from offset %i", pr_globaldefs32[num].s_name+stringtable, *(int *)&pr_globals[pr_globaldefs32[num].ofs]);
		return;
	default:
		Sys_Error("Bad bits");
		break;
	}
	Sys_Error("Should be unreachable");	
}

