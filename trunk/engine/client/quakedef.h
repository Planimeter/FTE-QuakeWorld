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
// quakedef.h -- primary header for client

#include "bothdefs.h"	//first thing included by ALL files.

#if _MSC_VER
#define MSVCDISABLEWARNINGS
#endif

#ifdef MSVCDISABLEWARNINGS
//#pragma warning( disable : 4244 4127 4201 4214 4514 4305 4115 4018)
/*#pragma warning( disable : 4244)	//conversion from const double to float
#pragma warning( disable : 4305)	//truncation from const double to float
#pragma warning( disable : 4018)	//signed/unsigned mismatch... fix these?
#pragma warning( disable : 4706)	//assignment within conditional expression - watch for these in GCC where they can be fixed but still functional.
#pragma warning( disable : 4100)	//unreferenced formal parameter
#pragma warning( disable : 4201)	//nonstandard extension used : nameless struct/union
#pragma warning( disable : 4213)	//nonstandard extension used : cast on l-value
#pragma warning( disable : 4127)	//conditional expression is constant - fixme?
*/
#pragma warning( 4 : 4244)	//conversion from const double to float
#pragma warning( 4 : 4305)	//truncation from const double to float
#pragma warning( 4 : 4018)	//truncation from const double to float

#pragma warning( 2 : 4701)
#pragma warning(2:4132 4268)// const object not initialized

#pragma warning(2:4032)     // function arg has different type from declaration
#pragma warning(2:4092)     // 'sizeof' value too big
#pragma warning(2:4132 4268)// const object not initialized
//#pragma warning(2:4152)     // pointer conversion between function and data
#pragma warning(2:4239)     // standard doesn't allow this conversion
#pragma warning(2:4701)     // local variable used without being initialized
//#pragma warning(2:4706)     // if (a=b) instead of (if a==b)
#pragma warning(2:4709)     // comma in array subscript
#pragma warning(3:4061)     // not all enum values tested in switch statement
#pragma warning(3:4710)     // inline function was not inlined
#pragma warning(3:4121)     // space added for structure alignment
#pragma warning(3:4505)     // unreferenced local function removed
#pragma warning(3:4019)     // empty statement at global scope
//#pragma warning(3:4057)     // pointers refer to different base types
#pragma warning(3:4125)     // decimal digit terminates octal escape
//#pragma warning(2:4131)     // old-style function declarator
#pragma warning(3:4211)     // extern redefined as static
//#pragma warning(3:4213)     // cast on left side of = is non-standard
#pragma warning(3:4222)     // member function at file scope shouldn't be static
#pragma warning(3:4234 4235)// keyword not supported or reserved for future
#pragma warning(3:4504)     // type ambiguous; simplify code
#pragma warning(3:4507)     // explicit linkage specified after default linkage
#pragma warning(3:4515)     // namespace uses itself
#pragma warning(3:4516 4517)// access declarations are deprecated
#pragma warning(3:4670)     // base class of thrown object is inaccessible
#pragma warning(3:4671)     // copy ctor of thrown object is inaccessible
#pragma warning(3:4673)     // thrown object cannot be handled in catch block
#pragma warning(3:4674)     // dtor of thrown object is inaccessible
#pragma warning(3:4705)     // statement has no effect (example: a+1;)


#pragma warning( 4 : 4267)	//truncation from const double to float
#endif



#define QUAKEDEF_H__

#define WATERLAYERS

#ifdef GLQUAKE
#define RGLQUAKE
#undef GLQUAKE	//compiler option
#endif

#ifdef SERVERONLY
#define isDedicated true
#endif
#ifdef CLIENTONLY
#define isDedicated false
#endif

#ifdef __linux__
#define PNG_SUCKS_WITH_SETJMP	//cos it does.
#endif

#define	QUAKE_GAME			// as opposed to utilities
//define	PARANOID			// speed sapping error checking

#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(AVAIL_PNGLIB) && defined(PNG_SUCKS_WITH_SETJMP)
#include <png.h>
#else
#include <setjmp.h>
#endif
#include <time.h>
#ifdef __cplusplus
extern "C" {
#endif

#include "translate.h"

#include "common.h"
#include "bspfile.h"
#include "vid.h"
#include "sys.h"
#include "zone.h"
#include "mathlib.h"
#include "wad.h"
#include "cvar.h"
#include "screen.h"
#include "net.h"
#include "protocol.h"
#include "cmd.h"
#include "sbar.h"
#include "sound.h"
#include "merged.h"
#include "render.h"
#include "client.h"

#include "vm.h"


//#if defined(RGLQUAKE)
#include "gl_model.h"
//#else
//#include "model.h"
//#endif

#if defined(SWQUAKE)
#include "d_iface.h"
#endif

#ifdef PEXT_BULLETENS
#include "r_bulleten.h"
#endif

#include "input.h"
#include "keys.h"
#include "console.h"
#include "view.h"
#include "menu.h"
#include "crc.h"
#include "cdaudio.h"
#include "pmove.h"

#ifndef CLIENTONLY
#include "progs.h"
#endif
#include "world.h"
#ifndef CLIENTONLY
//#ifdef Q2SERVER
#include "q2game.h"
//#endif
#include "server.h"
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

//=============================================================================

// the host system specifies the base of the directory tree, the
// command line parms passed to the program, and the amount of memory
// available for the program to use

typedef struct quakeparms_s
{
	char	*basedir;
	int		argc;
	char	**argv;
	void	*membase;
	unsigned int		memsize;
} quakeparms_t;


//=============================================================================

#define MAX_NUM_ARGVS	128


extern qboolean noclip_anglehack;


//
// host
//
extern	quakeparms_t host_parms;

extern	cvar_t		com_gamename;
extern	cvar_t		sys_ticrate;
extern	cvar_t		sys_nostdout;
extern	cvar_t		developer;

extern	cvar_t	password;

extern	qboolean	host_initialized;		// true if into command execution
extern	double		host_frametime;
extern	qbyte		*host_basepal;
extern	qbyte		*host_colormap;
extern	int			host_framecount;	// incremented every frame, never reset
extern	double		realtime;			// not bounded in any way, changed at
										// start of every frame, never reset

void Host_ServerFrame (void);
void Host_InitCommands (void);
void Host_Init (quakeparms_t *parms);
void Host_Shutdown(void);
void VARGS Host_Error (char *error, ...);
void VARGS Host_EndGame (char *message, ...);
qboolean Host_SimulationTime(float time);
void Host_Frame (float time);
void Host_Quit_f (void);
void VARGS Host_ClientCommands (char *fmt, ...);
void Host_ShutdownServer (qboolean crash);

extern qboolean		msg_suppress_1;		// suppresses resolution and cache size console output
										//  an fullscreen DIB focus gain/loss


#if !defined(SERVERONLY) && !defined(CLIENTONLY)
extern qboolean isDedicated;
#endif



#ifdef __cplusplus
}
#endif

