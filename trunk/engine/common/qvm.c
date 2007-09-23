/*************************************************************************
** QVM
** Copyright (C) 2003 by DarkOne
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**************************************************************************
** Quake3 compatible virtual machine
*************************************************************************/

/*
spike's changes.
masks are now done by modulus rather than and
VM_POINTER contains a mask check.
ds_mask is set to the mem allocated for stack and data.
builtins range check written to buffers.

QVM_Step placed in QVM_Exec for efficiency.

an invalid statement was added at the end of the statements to prevent the qvm walking off.
stack pops/pushes are all tested. An extra stack entry was faked to prevent stack checks on double-stack operators from overwriting.


Fixme: there is always the possibility that I missed a potential virus loophole..
Also, can efficiency be improved much?
*/


#include "quakedef.h"

#ifdef VM_ANY

#ifdef _MSC_VER	//fix this please
#define inline _inline
#endif



#define RETURNOFFSETMARKER NULL

typedef enum vm_type_e
{
	VM_NONE,
	VM_NATIVE,
	VM_BYTECODE
} vm_type_t;

struct vm_s {
// common
	vm_type_t type;
	char name[MAX_QPATH];
	sys_calldll_t syscalldll;
	sys_callqvm_t syscallqvm;

// shared
	void *hInst;

// native
	int (VARGS *vmMain)(int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6);
};


#ifdef _WIN32
#include "winquake.h"
void *Sys_LoadDLL(const char *name, void **vmMain, sys_calldll_t syscall)
{
	void (VARGS *dllEntry)(sys_calldll_t syscall);
	char dllname[MAX_OSPATH];
	HINSTANCE hVM;

	sprintf(dllname, "%sx86.dll", name);

	hVM=NULL;
	{
		char name[MAX_OSPATH];
		char *gpath;
		// run through the search paths
		gpath = NULL;
		while (1)
		{
			gpath = COM_NextPath (gpath);
			if (!gpath)
				return NULL;		// couldn't find one anywhere
			snprintf (name, sizeof(name), "%s/%s", gpath, dllname);
			hVM = LoadLibrary (name);
			if (hVM)
			{
				Con_DPrintf ("LoadLibrary (%s)\n",name);
				break;
			}
		}
	}

	if(!hVM) return NULL;

	dllEntry=(void *)GetProcAddress(hVM, "dllEntry");
	if(!dllEntry)
	{
		Con_Printf ("Sys_LoadDLL: %s lacks a dllEntry function\n",name);
		FreeLibrary(hVM);
		return NULL;
	}

	dllEntry(syscall);

	*vmMain=(void *)GetProcAddress(hVM, "vmMain");
	if(!*vmMain)
	{
		Con_Printf ("Sys_LoadDLL: %s lacks a vmMain function\n",name);
		FreeLibrary(hVM);
		return NULL;
	}
	return hVM;
}

/*
** Sys_UnloadDLL
*/
void Sys_UnloadDLL(void *handle)
{
	if(handle)
	{
		if(!FreeLibrary((HMODULE)handle))
			Sys_Error("Sys_UnloadDLL FreeLibrary failed");
	}
}
#else
#if defined(__MORPHOS__) && I_AM_BIGFOOT
#include <proto/dynload.h>
#else
#include <dlfcn.h>
#endif

void *Sys_LoadDLL(const char *name, void **vmMain, int (EXPORT_FN *syscall)(int arg, ... ))
{
	void (*dllEntry)(int (EXPORT_FN *syscall)(int arg, ... ));
	char dllname[MAX_OSPATH];
	void *hVM;

#if defined(__MORPHOS__) && I_AM_BIGFOOT
	if (DynLoadBase == 0)
		return 0;
#endif

#if defined(__amd64__)
	return 0;	//give up early, don't even try going there
	sprintf(dllname, "%samd.so", name);
#elif defined(_M_IX86) || defined(__i386__)
	sprintf(dllname, "%sx86.so", name);
#elif defined(__powerpc__)
	sprintf(dllname, "%sppc.so", name);
#elif defined(__ppc__)
	sprintf(dllname, "%sppc.so", name);
#else
	sprintf(dllname, "%sunk.so", name);
#endif

	hVM=NULL;
	{
		char name[MAX_OSPATH];
		char *gpath;
		// run through the search paths
		gpath = NULL;
		while (1)
		{
			gpath = COM_NextPath (gpath);
			if (!gpath)
				return NULL;		// couldn't find one anywhere
			_snprintf (name, sizeof(name), "%s/%s", gpath, dllname);

			Con_Printf("Loading native: %s\n", name);
			hVM = dlopen (name, RTLD_NOW);
			if (hVM)
			{
				Con_DPrintf ("Sys_LoadDLL: dlopen (%s)\n",name);
				break;
			}
			else
			{
				Con_DPrintf("Sys_LoadDLL: dlerror()=\"%s\"", dlerror());
			}
		}
	}

	if(!hVM) return NULL;

	dllEntry=(void *)dlsym(hVM, "dllEntry");
	if(!dllEntry)
	{
		Con_Printf("Sys_LoadDLL: %s does not have a dllEntry function\n");
		dlclose(hVM);
		return NULL;
	}

	(*dllEntry)(syscall);

	*vmMain=(void *)dlsym(hVM, "vmMain");
	if(!*vmMain)
	{
		Con_Printf("Sys_LoadDLL: %s does not have a vmMain function\n");
		dlclose(hVM);
		return NULL;
	}

	return hVM;
}

/*
** Sys_UnloadDLL
*/
void Sys_UnloadDLL(void *handle)
{
	if(handle)
	{
		if(dlclose(handle))
			Sys_Error("Sys_UnloadDLL FreeLibrary failed");
	}
}
#endif










// ------------------------- * QVM files * -------------------------
#define	VM_MAGIC	0x12721444
#define LL(x)			x = LittleLong(x)

#pragma pack(push,1)
typedef struct vmHeader_s
{
	int vmMagic;

	int instructionCount;

	int codeOffset;
	int codeLength;

	int dataOffset;
	int dataLength;	// should be byteswapped on load
	int litLength;	// copy as is
	int bssLength;	// zero filled memory appended to datalength
} vmHeader_t;
#pragma pack(pop)

// ------------------------- * in memory representation * -------------------------

typedef struct qvm_s
{
// segments
	unsigned int *cs;	// code  segment, each instruction is 2 ints
	qbyte *ds;	// data  segment, partially filled on load (data, lit, bss)
	qbyte *ss;	// stack segment, (follows immediatly after ds, corrupting data before vm)

// pointer registers
	unsigned int *pc;	// program counter, points to cs, goes up
	unsigned int *sp;	// stack pointer, initially points to end of ss, goes down
	unsigned int bp;	// base pointer, initially len_ds+len_ss/2

	unsigned int *min_sp;
	unsigned int *max_sp;
	unsigned int min_bp;
	unsigned int max_bp;

// status
	unsigned int len_cs;	// size of cs
	unsigned int len_ds;	// size of ds
	unsigned int len_ss;	// size of ss
	unsigned int ds_mask; // ds mask (ds+ss)

// memory
	unsigned int mem_size;
	qbyte *mem_ptr;

//	unsigned int cycles;	// command cicles executed
	sys_callqvm_t syscall;
} qvm_t;

qvm_t *QVM_Load(const char *name, sys_callqvm_t syscall);
void QVM_UnLoad(qvm_t *qvm);
int QVM_Exec(qvm_t *qvm, int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7);


// ------------------------- * OP.CODES * -------------------------

typedef enum qvm_op_e
{
	OP_UNDEF,
	OP_NOP,
	OP_BREAK,

	OP_ENTER, // b32
	OP_LEAVE,	// b32
	OP_CALL,
	OP_PUSH,
	OP_POP,

	OP_CONST,	// b32
	OP_LOCAL,	// b32

	OP_JUMP,

// -------------------

	OP_EQ,	// b32
	OP_NE,	// b32

	OP_LTI,	// b32
	OP_LEI,	// b32
	OP_GTI,	// b32
	OP_GEI,	// b32

	OP_LTU,	// b32
	OP_LEU,	// b32
	OP_GTU,	// b32
	OP_GEU,	// b32

	OP_EQF,	// b32
	OP_NEF,	// b32

	OP_LTF,	// b32
	OP_LEF,	// b32
	OP_GTF,	// b32
	OP_GEF,	// b32

// -------------------

	OP_LOAD1,
	OP_LOAD2,
	OP_LOAD4,
	OP_STORE1,
	OP_STORE2,
	OP_STORE4,
	OP_ARG,	// b8
	OP_BLOCK_COPY,	// b32

//-------------------

	OP_SEX8,
	OP_SEX16,

	OP_NEGI,
	OP_ADD,
	OP_SUB,
	OP_DIVI,
	OP_DIVU,
	OP_MODI,
	OP_MODU,
	OP_MULI,
	OP_MULU,

	OP_BAND,
	OP_BOR,
	OP_BXOR,
	OP_BCOM,

	OP_LSH,
	OP_RSHI,
	OP_RSHU,

	OP_NEGF,
	OP_ADDF,
	OP_SUBF,
	OP_DIVF,
	OP_MULF,

	OP_CVIF,
	OP_CVFI
} qvm_op_t;









// ------------------------- * Init & ShutDown * -------------------------

/*
** QVM_Load
*/
qvm_t *QVM_Load(const char *name, sys_callqvm_t syscall)
{
	char path[MAX_QPATH];
	vmHeader_t *header;
	qvm_t *qvm;
	qbyte *raw;
	int n;
	int i;

	sprintf(path, "%s.qvm", name);
	raw = COM_LoadMallocFile(path);
//	FS_LoadFile(path, &raw, false);
// file not found
	if(!raw) return NULL;
	header=(vmHeader_t*)raw;

	LL(header->vmMagic);
	LL(header->instructionCount);
	LL(header->codeOffset);
	LL(header->codeLength);
	LL(header->dataOffset);
	LL(header->dataLength);
	LL(header->litLength);
	LL(header->bssLength);

// check file
	if(header->vmMagic!=VM_MAGIC || header->instructionCount<=0 || header->codeLength<=0)
	{
		BZ_Free(raw);
		return NULL;
	}

// create vitrual machine
	qvm=Z_Malloc(sizeof(qvm_t));
	qvm->len_cs=header->instructionCount+1;	//bad opcode padding.
	qvm->len_ds=header->dataLength+header->litLength+header->bssLength;
	qvm->len_ss=256*1024;									// 256KB stack space

// memory
	qvm->ds_mask = qvm->len_ds*sizeof(qbyte)+(qvm->len_ss+16*4)*sizeof(qbyte);//+4 for a stack check decrease
	for (i = 0; i < sizeof(qvm->ds_mask)*8-1; i++)
	{
		if ((1<<(unsigned int)i) >= qvm->ds_mask)	//is this bit greater than our minimum?
			break;
	}
	qvm->len_ss = (1<<i) - qvm->len_ds*sizeof(qbyte) - 4;	//expand the stack space to fill it.
	qvm->ds_mask = qvm->len_ds*sizeof(qbyte)+(qvm->len_ss+4)*sizeof(qbyte);
	qvm->len_ss -= qvm->len_ss&7;


	qvm->mem_size=qvm->len_cs*sizeof(int)*2 + qvm->ds_mask;
	qvm->mem_ptr=Z_Malloc(qvm->mem_size);
// set pointers
	qvm->cs=(unsigned int*)qvm->mem_ptr;
	qvm->ds=(qbyte*)(qvm->mem_ptr+qvm->len_cs*sizeof(int)*2);
	qvm->ss=(qbyte*)((qbyte*)qvm->ds+qvm->len_ds*sizeof(qbyte));
		//waste 32 bits here.
		//As the opcodes often check stack 0 and 1, with a backwards stack, 1 can leave the stack area. This is where we compensate for it.
// setup registers
	qvm->pc=qvm->cs;
	qvm->sp=(unsigned int*)(qvm->ss+qvm->len_ss);
	qvm->bp=qvm->len_ds+qvm->len_ss/2;
//	qvm->cycles=0;
	qvm->syscall=syscall;

	qvm->ds_mask--;

	qvm->min_sp = (unsigned int*)(qvm->ds+qvm->len_ds+qvm->len_ss/2);
	qvm->max_sp = (unsigned int*)(qvm->ds+qvm->len_ds+qvm->len_ss);
	qvm->min_bp = qvm->len_ds;
	qvm->max_bp = qvm->len_ds+qvm->len_ss/2;

	qvm->bp = qvm->max_bp;

// load instructions
{
	qbyte *src=raw+header->codeOffset;
	int *dst=(int*)qvm->cs;
	int total=header->instructionCount;
	qvm_op_t op;

	for(n=0; n<total; n++)
	{
		op=*src++;
		*dst++=(int)op;
		switch(op)
		{
		case OP_ENTER:
		case OP_LEAVE:
		case OP_CONST:
		case OP_LOCAL:
		case OP_EQ:
		case OP_NE:
		case OP_LTI:
		case OP_LEI:
		case OP_GTI:
		case OP_GEI:
		case OP_LTU:
		case OP_LEU:
		case OP_GTU:
		case OP_GEU:
		case OP_EQF:
		case OP_NEF:
		case OP_LTF:
		case OP_LEF:
		case OP_GTF:
		case OP_GEF:
		case OP_BLOCK_COPY:
			*dst++=LittleLong(*(int*)src);
			src+=4;
			break;
		case OP_ARG:
			*dst++=(int)*src++;
			break;
		default:
			*dst++=0;
			break;
		}
	}
	*dst++=OP_BREAK;	//in case someone 'forgot' the return on the last function.
	*dst++=0;
}

// load data segment
{
	int *src=(int*)(raw+header->dataOffset);
	int *dst=(int*)qvm->ds;
	int total=header->dataLength/4;

	for(n=0; n<total; n++)
		*dst++=LittleLong(*src++);

	memcpy(dst, src, header->litLength);
}

	BZ_Free(raw);
	return qvm;
}

/*
** QVM_UnLoad
*/
void QVM_UnLoad(qvm_t *qvm)
{
	Z_Free(qvm->mem_ptr);
	Z_Free(qvm);
}


// ------------------------- * private execution stuff * -------------------------

/*
** QVM_Goto
*/
static void inline QVM_Goto(qvm_t *vm, int addr)
{
	if(addr<0 || addr>vm->len_cs)
		Sys_Error("VM run time error: program jumped off to hyperspace\n");
	vm->pc=vm->cs+addr*2;
}

/*
** QVM_Call
**
** calls function
*/
static void inline QVM_Call(qvm_t *vm, int addr)
{
	vm->sp--;
	if (vm->sp < vm->min_sp) Sys_Error("QVM Stack underflow");

	if(addr<0)
	{
	// system trap function
		{
			int *fp;

			fp=(int*)(vm->ds+vm->bp)+2;
			vm->sp[0] = vm->syscall(vm->ds, vm->ds_mask, -addr-1, fp);
			return;
		}
	}

	if(addr>=vm->len_cs)
		Sys_Error("VM run time error: program jumped off to hyperspace\n");

	vm->sp[0]=(vm->pc-vm->cs); // push pc /return address/
	vm->pc=vm->cs+addr*2;
	if (!vm->pc)
		Sys_Error("VM run time error: program called the void\n");
}

/*
** QVM_Enter
**
** [oPC][0][.......]| <- oldBP
** ^BP
*/
static void inline QVM_Enter(qvm_t *vm, int size)
{
	int *fp;

	vm->bp-=size;
	if(vm->bp<vm->min_bp)
		Sys_Error("VM run time error: out of stack\n");

	fp=(int*)(vm->ds+vm->bp);
	fp[0]=vm->sp-vm->max_sp;					// unknown /maybe size/
	fp[1]=*vm->sp++;	// saved PC

	if (vm->sp > vm->max_sp) Sys_Error("QVM Stack overflow");
}

/*
** QVM_Return
*/
static void inline QVM_Return(qvm_t *vm, int size)
{
	int *fp;

	fp=(int*)(vm->ds+vm->bp);
	vm->bp+=size;

	if(vm->bp>vm->max_bp)
		Sys_Error("VM run time error: freed too much stack\n");

	if(fp[1]>=vm->len_cs*2)
		if ((int)(vm->cs+fp[1]) != (int)RETURNOFFSETMARKER)	//this being false causes the program to quit.
			Sys_Error("VM run time error: program returned to hyperspace (%p, %p)\n", (char*)vm->cs, (char*)fp[1]);
	if(fp[1]<0)
		if ((int)(vm->cs+fp[1]) != (int)RETURNOFFSETMARKER)
			Sys_Error("VM run time error: program returned to negative hyperspace\n");

	if (vm->sp-vm->max_sp != fp[0])
		Sys_Error("VM run time error: stack push/pop mismatch \n");
	vm->pc=vm->cs+fp[1]; // restore PC
}

// ------------------------- * execution * -------------------------

/*
** VM_Exec
*/
int QVM_Exec(register qvm_t *qvm, int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7)
{
//remember that the stack is backwards. push takes 1.

//FIXME: does it matter that our stack pointer (qvm->sp) is backwards compared to q3?
//We are more consistant of course, but this simply isn't what q3 does.

//all stack shifts in this function are referenced through these 2 macros.
#define POP(t)	qvm->sp+=t;if (qvm->sp > qvm->max_sp) Sys_Error("QVM Stack underflow");
#define PUSH(v) qvm->sp--;if (qvm->sp < qvm->min_sp) Sys_Error("QVM Stack overflow");*qvm->sp=v
	qvm_op_t op=-1;
	unsigned int param;

	int *fp;
	unsigned int *oldpc;

	oldpc = qvm->pc;

// setup execution environment
	qvm->pc=RETURNOFFSETMARKER;
//	qvm->cycles=0;
// prepare local stack
	qvm->bp -= 15*4;	//we have to do this each call for the sake of (reliable) recursion.
	fp=(int*)(qvm->ds+qvm->bp);
// push all params
	fp[0]=0;
	fp[1]=0;
	fp[2]=command;
	fp[3]=arg0;
	fp[4]=arg1;
	fp[5]=arg2;
	fp[6]=arg3;
	fp[7]=arg4;
	fp[8]=arg5;
	fp[9]=arg6;
	fp[10]=arg7;	// arg7;
	fp[11]=0;	// arg8;
	fp[12]=0;	// arg9;
	fp[13]=0;	// arg10;
	fp[14]=0;	// arg11;

	QVM_Call(qvm, 0);

	for(;;)
	{
	// fetch next command
 		op=*qvm->pc++;
		param=*qvm->pc++;
//		qvm->cycles++;

		switch(op)
		{
	// aux
		case OP_UNDEF:
		case OP_NOP:
			break;
		default:
		case OP_BREAK: // break to debugger
			*(int*)NULL=-1;
			break;

	// subroutines
		case OP_ENTER:
			QVM_Enter(qvm, param);
			break;
		case OP_LEAVE:
			QVM_Return(qvm, param);

			if ((int)qvm->pc == (int)RETURNOFFSETMARKER)
			{
				// pick return value from stack
				qvm->pc = oldpc;

				qvm->bp += 15*4;
//				if(qvm->bp!=qvm->max_bp)
//					Sys_Error("VM run time error: freed too much stack\n");
				param = qvm->sp[0];
				POP(1);
				return param;
			}
			break;
		case OP_CALL:
			param = *qvm->sp;
			POP(1);
			QVM_Call(qvm, param);
			break;

	// stack
		case OP_PUSH:
			PUSH(*qvm->sp);
			break;
		case OP_POP:
			POP(1);
			break;
		case OP_CONST:
			PUSH(param);
			break;
		case OP_LOCAL:
			PUSH(param+qvm->bp);
			break;

	// branching
		case OP_JUMP:
			param = *qvm->sp;
			POP(1);
			QVM_Goto(qvm, param);
			break;
		case OP_EQ:
			if(qvm->sp[1]==qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_NE:
			if(qvm->sp[1]!=qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_LTI:
			if(*(signed int*)&qvm->sp[1]<*(signed int*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_LEI:
			if(*(signed int*)&qvm->sp[1]<=*(signed int*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_GTI:
			if(*(signed int*)&qvm->sp[1]>*(signed int*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_GEI:
			if(*(signed int*)&qvm->sp[1]>=*(signed int*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_LTU:
			if(*(unsigned int*)&qvm->sp[1]<*(unsigned int*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_LEU:
			if(*(unsigned int*)&qvm->sp[1]<=*(unsigned int*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_GTU:
			if(*(unsigned int*)&qvm->sp[1]>*(unsigned int*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_GEU:
			if(*(unsigned int*)&qvm->sp[1]>=*(unsigned int*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_EQF:
			if(*(float*)&qvm->sp[1]==*(float*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_NEF:
			if(*(float*)&qvm->sp[1]!=*(float*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_LTF:
			if(*(float*)&qvm->sp[1]<*(float*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_LEF:
			if(*(float*)&qvm->sp[1]<=*(float*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_GTF:
			if(*(float*)&qvm->sp[1]>*(float*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;
		case OP_GEF:
			if(*(float*)&qvm->sp[1]>=*(float*)&qvm->sp[0]) QVM_Goto(qvm, param);
			POP(2);
			break;

	// memory I/O: masks protect main memory
		case OP_LOAD1:
			*(unsigned int*)&qvm->sp[0]=*(unsigned char*)&qvm->ds[qvm->sp[0]&qvm->ds_mask];
			break;
		case OP_LOAD2:
			*(unsigned int*)&qvm->sp[0]=*(unsigned short*)&qvm->ds[qvm->sp[0]&qvm->ds_mask];
			break;
		case OP_LOAD4:
			*(unsigned int*)&qvm->sp[0]=*(unsigned int*)&qvm->ds[qvm->sp[0]&qvm->ds_mask];
			break;
		case OP_STORE1:
			*(qbyte*)&qvm->ds[qvm->sp[1]&qvm->ds_mask]=(qbyte)(qvm->sp[0]&0xFF);
			POP(2);
			break;
		case OP_STORE2:
			*(unsigned short*)&qvm->ds[qvm->sp[1]&qvm->ds_mask]=(unsigned short)(qvm->sp[0]&0xFFFF);
			POP(2);
			break;
		case OP_STORE4:
			*(unsigned int*)&qvm->ds[qvm->sp[1]&qvm->ds_mask]=*(unsigned int*)&qvm->sp[0];
			POP(2);
			break;
		case OP_ARG:
			*(unsigned int*)&qvm->ds[(param+qvm->bp)&qvm->ds_mask]=*(unsigned int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_BLOCK_COPY:
			if (qvm->sp[1]+param < qvm->ds_mask && qvm->sp[0] + param < qvm->ds_mask)
			{
				memmove(qvm->ds+(qvm->sp[1]&qvm->ds_mask), qvm->ds+(qvm->sp[0]&qvm->ds_mask), param);
			}
			POP(2);
			break;

	// integer arithmetic
		case OP_SEX8:
			if(*(signed int*)&qvm->sp[0]&0x80) *(signed int*)&qvm->sp[0]|=0xFFFFFF00;
			break;
		case OP_SEX16:
			if(*(signed int*)&qvm->sp[0]&0x8000) *(signed int*)&qvm->sp[0]|=0xFFFF0000;
			break;
		case OP_NEGI:
			*(signed int*)&qvm->sp[0]=-*(signed int*)&qvm->sp[0];
			break;
		case OP_ADD:
			*(signed int*)&qvm->sp[1]+=*(signed int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_SUB:
			*(signed int*)&qvm->sp[1]-=*(signed int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_DIVI:
			*(signed int*)&qvm->sp[1]/=*(signed int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_DIVU:
			*(unsigned int*)&qvm->sp[1]/=(*(unsigned int*)&qvm->sp[0]);
			POP(1);
			break;
		case OP_MODI:
			*(signed int*)&qvm->sp[1]%=*(signed int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_MODU:
			*(unsigned int*)&qvm->sp[1]%=(*(unsigned int*)&qvm->sp[0]);
			POP(1);
			break;
		case OP_MULI:
			*(signed int*)&qvm->sp[1]*=*(signed int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_MULU:
			*(unsigned int*)&qvm->sp[1]*=(*(unsigned int*)&qvm->sp[0]);
			POP(1);
			break;

	// logic
		case OP_BAND:
			*(unsigned int*)&qvm->sp[1]&=*(unsigned int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_BOR:
			*(unsigned int*)&qvm->sp[1]|=*(unsigned int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_BXOR:
			*(unsigned int*)&qvm->sp[1]^=*(unsigned int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_BCOM:
			*(unsigned int*)&qvm->sp[0]=~*(unsigned int*)&qvm->sp[0];
			break;
		case OP_LSH:
			*(unsigned int*)&qvm->sp[1]<<=*(unsigned int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_RSHI:
			*(signed int*)&qvm->sp[1]>>=*(signed int*)&qvm->sp[0];
			POP(1);
			break;
		case OP_RSHU:
			*(unsigned int*)&qvm->sp[1]>>=*(unsigned int*)&qvm->sp[0];
			POP(1);
			break;

	// floating point arithmetic
		case OP_NEGF:
			*(float*)&qvm->sp[0]=-*(float*)&qvm->sp[0];
			break;
		case OP_ADDF:
			*(float*)&qvm->sp[1]+=*(float*)&qvm->sp[0];
			POP(1);
			break;
		case OP_SUBF:
			*(float*)&qvm->sp[1]-=*(float*)&qvm->sp[0];
			POP(1);
			break;
		case OP_DIVF:
			*(float*)&qvm->sp[1]/=*(float*)&qvm->sp[0];
			POP(1);
			break;
		case OP_MULF:
			*(float*)&qvm->sp[1]*=*(float*)&qvm->sp[0];
			POP(1);
			break;

	// format conversion
		case OP_CVIF:
			*(float*)&qvm->sp[0]=(float)(signed int)qvm->sp[0];
			break;
		case OP_CVFI:
			*(signed int*)&qvm->sp[0]=(signed int)(*(float*)&qvm->sp[0]);
			break;
		}
	}
}











// ------------------------- * interface * -------------------------

/*
** VM_PrintInfo
*/
void VM_PrintInfo(vm_t *vm)
{
	qvm_t *qvm;

	if(!vm->name[0])
		return;
	Con_Printf("%s (%p): ", vm->name, vm->hInst);

	switch(vm->type)
	{
	case VM_NATIVE:
		Con_Printf("native\n");
		break;

	case VM_BYTECODE:
		Con_Printf("interpreted\n");
		if((qvm=vm->hInst))
		{
			Con_Printf("  code  length: %d\n", qvm->len_cs);
			Con_Printf("  data  length: %d\n", qvm->len_ds);
			Con_Printf("  stack length: %d\n", qvm->len_ss);
		}
		break;

	default:
		Con_Printf("unknown\n");
		break;
	}
}

/*
** VM_Create
*/
vm_t *VM_Create(vm_t *vm, const char *name, sys_calldll_t syscalldll, sys_callqvm_t syscallqvm)
{
	if(!name || !*name)
		Sys_Error("VM_Create: bad parms");

	if (!vm)
		vm = Z_Malloc(sizeof(vm_t));

// prepare vm struct
	memset(vm, 0, sizeof(vm_t));
	Q_strncpyz(vm->name, name, sizeof(vm->name));
	vm->syscalldll=syscalldll;
	vm->syscallqvm=syscallqvm;



	if (syscalldll)
	{
		if (!COM_CheckParm("-nodlls") && !COM_CheckParm("-nosos"))	//:)
		{
			if((vm->hInst=Sys_LoadDLL(name, (void**)&vm->vmMain, syscalldll)))
			{
				Con_DPrintf("Creating native machine \"%s\"\n", name);
				vm->type=VM_NATIVE;
				return vm;
			}
		}
	}


	if (syscallqvm)
	{
		if((vm->hInst=QVM_Load(name, syscallqvm)))
		{
			Con_DPrintf("Creating virtual machine \"%s\"\n", name);
			vm->type=VM_BYTECODE;
			return vm;
		}
	}

	Z_Free(vm);
	return NULL;
}

/*
** VM_Destroy
*/
void VM_Destroy(vm_t *vm)
{
	if(!vm) return;

	switch(vm->type)
	{
	case VM_NATIVE:
		if(vm->hInst) Sys_UnloadDLL(vm->hInst);
		break;

	case VM_BYTECODE:
		if(vm->hInst) QVM_UnLoad(vm->hInst);
		break;

	case VM_NONE:
		break;
	}

	Z_Free(vm);
}

/*
** VM_Restart
*/
qboolean VM_Restart(vm_t *vm)
{
	char name[MAX_QPATH];
	sys_calldll_t syscalldll;
	sys_callqvm_t syscallqvm;

	if(!vm) return false;

// save params
	Q_strncpyz(name, vm->name, sizeof(name));
	syscalldll=vm->syscalldll;
	syscallqvm=vm->syscallqvm;

// restart
	switch(vm->type)
	{
	case VM_NATIVE:
		if(vm->hInst) Sys_UnloadDLL(vm->hInst);
		break;

	case VM_BYTECODE:
		if(vm->hInst) QVM_UnLoad(vm->hInst);
		break;

	case VM_NONE:
		break;
	}

	return VM_Create(vm, name, syscalldll, syscallqvm)!=NULL;
}

void *VM_MemoryBase(vm_t *vm)
{
	switch(vm->type)
	{
	case VM_NATIVE:
		return NULL;
	case VM_BYTECODE:
		return ((qvm_t*)vm->hInst)->ds;
	default:
		return NULL;
	}
}

/*
** VM_Call
*/
int VARGS VM_Call(vm_t *vm, int instruction, ...)
{
	va_list argptr;
	int arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7;

	if(!vm) Sys_Error("VM_Call with NULL vm");

	va_start(argptr, instruction);
	arg0=va_arg(argptr, int);
	arg1=va_arg(argptr, int);
	arg2=va_arg(argptr, int);
	arg3=va_arg(argptr, int);
	arg4=va_arg(argptr, int);
	arg5=va_arg(argptr, int);
	arg6=va_arg(argptr, int);
	arg7=va_arg(argptr, int);
	va_end(argptr);

	switch(vm->type)
	{
	case VM_NATIVE:
		return vm->vmMain(instruction, arg0, arg1, arg2, arg3, arg4, arg5, arg6);

	case VM_BYTECODE:
		return QVM_Exec(vm->hInst, instruction, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7);

	case VM_NONE:
		return 0;
	}
	return 0;
}

#endif
