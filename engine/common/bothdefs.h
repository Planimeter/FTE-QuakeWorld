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

#ifndef __BOTHDEFS_H
#define __BOTHDEFS_H

// release version
#define FTE_VER_MAJOR 1
#define FTE_VER_MINOR 3

#if defined(__APPLE__) && defined(__MACH__)
	#define MACOSX
#endif

#if defined(__MINGW32_VERSION) || defined(__MINGW__) || defined(__MINGW32__) || defined(__MINGW64__)
	#define MINGW
#endif
#if !defined(MINGW) && defined(__GNUC__) && defined(_WIN32)
	#define MINGW	//Erm, why is this happening?
#endif

#ifdef ANDROID
	#define NO_PNG
	#define NO_JPEG
	#define NO_OGG
#endif

#ifdef NACL
	#define NO_PNG
	#define NO_JPEG
	#define NO_OGG
	#define NO_ZLIB
#endif

#ifdef FTE_TARGET_WEB
	//no Sys_LoadLibrary support, so we might as well kill this stuff off.
	#define NO_PNG
	#define NO_JPEG
	#define NO_OGG
	#define NO_ZLIB
	#define NO_FREETYPE
#endif

#ifdef HAVE_CONFIG_H	//if it was configured properly, then we have a more correct list of features we want to use.
	#include "config.h"
#else
	#ifndef MSVCLIBSPATH
	#if _MSC_VER == 1200
		#define MSVCLIBSPATH "../libs/vc6-libs/"
	#else
		#define MSVCLIBSPATH "../libs/"
	#endif
	#endif

	#ifdef NO_LIBRARIES
		#define NO_DIRECTX
		#define NO_PNG
		#define NO_JPEG
		#define NO_ZLIB
		#define NO_OGG
	#else
		#define AVAIL_OPENAL
		#define AVAIL_FREETYPE
	#endif

	#define AVAIL_OGGVORBIS
	#if defined(__CYGWIN__)
		#define AVAIL_ZLIB
	#else
		#define AVAIL_PNGLIB
		#define AVAIL_JPEGLIB
		#define AVAIL_ZLIB
		#define AVAIL_OGGVORBIS
	#endif

#if !defined(NO_DIRECTX) && !defined(NODIRECTX) && defined(_WIN32)
	#define AVAIL_DINPUT
	#define AVAIL_DDRAW
	#define AVAIL_DSOUND
	#define AVAIL_D3D
#endif

#if !defined(MINIMAL) && !defined(NPFTE) && !defined(NPQTV)
#if defined(_WIN32) && !defined(FTE_SDL) && !defined(WINRT)
	#define HAVE_WINSSPI	//built in component, checks against windows' root ca database and revocations etc.
#elif defined(__linux__) || defined(__CYGWIN__)
	#define HAVE_GNUTLS		//currently disabled as it does not validate the server's certificate, beware the mitm attack.
#endif
#endif
#if defined(HAVE_WINSSPI) || defined(HAVE_GNUTLS)
	#define HAVE_SSL
#endif

//#define DYNAMIC_ZLIB
//#define DYNAMIC_LIBPNG
//#define DYNAMIC_LIBJPEG
//#define LIBVORBISFILE_STATIC
//#define SPEEX_STATIC

#ifdef D3DQUAKE
#define D3D9QUAKE
#endif

#if (defined(D3D9QUAKE) || defined(D3D11QUAKE)) && !defined(D3DQUAKE)
#define D3DQUAKE
#endif

#if defined(_MSC_VER) //too lazy to fix up the makefile
//#define BOTLIB_STATIC
#endif

#define ODE_DYNAMIC

#ifdef NO_OPENAL
	#undef AVAIL_OPENAL
#endif

#ifdef NO_PNG
	#undef AVAIL_PNGLIB
#endif
#ifdef NO_JPEG
	#undef AVAIL_JPEGLIB
#endif
#ifdef NO_ZLIB
	#undef AVAIL_ZLIB
#endif
#ifdef NO_OGG
	#undef AVAIL_OGGVORBIS
#endif
#if defined(NO_FREETYPE)
	#undef AVAIL_FREETYPE
#endif

//set any additional defines or libs in win32
	#define SVRANKING
	#define LOADERTHREAD

	#ifdef MINIMAL
		#define CL_MASTER		//this is useful

		#undef AVAIL_JPEGLIB	//no jpeg support
		#undef AVAIL_PNGLIB		//no png support
		#define NOMEDIA			//NO playing of avis/cins/roqs

		#define SPRMODELS		//quake1 sprite models
		#define MD3MODELS		//we DO want to use quake3 alias models. This might be a minimal build, but we still want this.
		#define PLUGINS

		#define PSET_CLASSIC

		//#define CSQC_DAT	//support for csqc

		#ifndef SERVERONLY	//don't be stupid, stupid.
			#ifndef CLIENTONLY
				#define CLIENTONLY
			#endif
		#endif
	#else
		#define USE_SQLITE
		#ifdef SERVERONLY
			#define USE_MYSQL	//allow mysql in dedicated servers.
		#endif
		#if defined(_WIN32) && !defined(FTE_SDL) && !defined(WINRT) 
			//#define SUBSERVERS	//use subserver code.
		#endif

		#define SIDEVIEWS	4	//enable secondary/reverse views.

//		#define DSPMODELS		//doom sprites (only needs DOOMWADS to generate the right wad file names)
		#define SPRMODELS		//quake1 sprite models
		#define SP2MODELS		//quake2 sprite models
		#define MD2MODELS		//quake2 alias models
		#define MD3MODELS		//quake3 alias models
		#define MD5MODELS		//doom3 models
		#define ZYMOTICMODELS	//zymotic skeletal models.
		#define DPMMODELS		//darkplaces model format (which I've never seen anyone use)
		#define PSKMODELS		//PSK model format (ActorX stuff from UT, though not the format the game itself uses)
//		#define HALFLIFEMODELS	//halflife model support (experimental)
		#define INTERQUAKEMODELS
		#define RAGDOLL

		#define HUFFNETWORK		//huffman network compression
		#define DOOMWADS		//doom wad/sprite support
//		#define MAP_DOOM		//doom map support
//		#define MAP_PROC		//doom3/quake4 map support
		//#define WOLF3DSUPPORT	//wolfenstein3d map support (not started yet)
		#define Q2BSPS			//quake 2 bsp support
		#define Q3BSPS			//quake 3 bsp support
		#define TERRAIN			//heightmap support
//		#define SV_MASTER		//starts up a master server
		#define SVCHAT			//serverside npc chatting. see sv_chat.c
		#define Q2SERVER		//server can run a q2 game dll and switches to q2 network and everything else.
		#define Q2CLIENT		//client can connect to q2 servers
		#define Q3CLIENT
		#define Q3SERVER
		#define HEXEN2			//technically server only
//		#define HLCLIENT 7		//we can run HL gamecode (not protocol compatible, set to 6 or 7)
//		#define HLSERVER 140	//we can run HL gamecode (not protocol compatible, set to 138 or 140)
		#define NQPROT			//server and client are capable of using quake1/netquake protocols. (qw is still prefered. uses the command 'nqconnect')
		#define ZLIB			//zip/pk3 support
		#define WEBSERVER		//http/ftp servers
		#define WEBCLIENT		//http/ftp clients.
		#define RUNTIMELIGHTING	//calculate lit/lux files the first time the map is loaded and doesn't have a loadable lit.
//		#define QTERM			//qterm... adds a console command that allows running programs from within quake - bit like xterm.
		#define CL_MASTER		//query master servers and stuff for a dynamic server listing.
		#define R_XFLIP			//allow view to be flipped horizontally
		#define TEXTEDITOR
		#define IMAGEFMT_DDS	//a sort of image file format.
		#define IMAGEFMT_BLP	//a sort of image file format.
#ifndef RTLIGHTS
		#define RTLIGHTS		//realtime lighting
#endif

		#define VM_Q1			//q1 qvm gamecode interface
		//#define	VM_LUA			//q1 lua gamecode interface

		#define TCPCONNECT		//a tcpconnect command, that allows the player to connect to tcp-encapsulated qw protocols.
		#define IRCCONNECT		//an ircconnect command, that allows the player to connect to irc-encapsulated qw protocols... yeah, really.

		#define PLUGINS			//qvm/dll plugins.
		#define SUPPORT_ICE		//Interactive Connectivity Establishment protocol, for peer-to-peer connections

		#define CSQC_DAT	//support for csqc
		#define MENU_DAT	//support for menu.dat

		#define PSET_SCRIPT
		#define PSET_CLASSIC
		//#define PSET_DARKPLACES

		#define VOICECHAT

#if defined(_WIN32) && !defined(MULTITHREAD) //always thread on win32 non-minimal builds
		#define MULTITHREAD
#endif
	#endif

#endif


//software rendering is just too glitchy, don't use it.
#if defined(SWQUAKE) && !defined(_DEBUG)
	#undef SWQUAKE
#endif


//include a file to update the various configurations for game-specific configs (hopefully just names)
#ifdef BRANDING_INC
	#define STRINGIFY2(s) #s
	#define STRINGIFY(s) STRINGIFY2(s)
	#include STRINGIFY(BRANDING_INC)
#endif
#ifndef DISTRIBUTION
	#define DISTRIBUTION "FTE"	//short name used to identify this engine. must be a single word
#endif
#ifndef DISTRIBUTIONLONG
	#define DISTRIBUTIONLONG "Forethought Entertainment"	//effectively the 'company' name
#endif
#ifndef FULLENGINENAME
	#define FULLENGINENAME "FTE QuakeWorld"	//the posh name for the engine
#endif
#ifndef ENGINEWEBSITE
	#define ENGINEWEBSITE "http://www.fteqw.com"	//url for program
#endif

#ifdef QUAKETC
	#define NOBUILTINMENUS	//kill engine menus (should be replaced with ewither csqc or menuqc)
	#undef Q2CLIENT	//not useful
	#undef Q2SERVER	//not useful
	#undef Q3CLIENT	//not useful
	#undef Q3SERVER	//not useful
	#undef HLCLIENT	//not useful
	#undef HLSERVER	//not useful
	#undef VM_Q1	//not useful
	#undef VM_LUA	//not useful
	#undef HALFLIFEMODELS	//yuck
	#undef RUNTIMELIGHTING	//presumably not useful
	#undef HEXEN2
#endif

//#define QUAKESPYAPI //define this if you want the engine to be usable via gamespy/quakespy, which has been dead for a long time now.

#ifdef FTE_TARGET_WEB
	//try to trim the fat
	#undef VOICECHAT	//too lazy to compile speex
	#undef HLCLIENT		//dlls...
	#undef HLSERVER		//dlls...
	#undef CL_MASTER	//bah. use the site to specify the servers.
	#undef SV_MASTER	//yeah, because that makes sense in a browser
	#undef ODE_STATIC	//blurgh, too lazy
	#undef ODE_DYNAMIC	//dlls...
	#undef RAGDOLL		//no ode
	#undef TCPCONNECT	//err...
	#undef IRCCONNECT	//not happening
	#undef PLUGINS		//pointless
	#undef VM_Q1		//no dlls
	#undef MAP_PROC		//meh
	#undef HALFLIFEMODELS	//blurgh
	#undef WEBSERVER	//hah, yeah, right
	#undef SUPPORT_ICE	//kinda requires udp, so not usable

	//extra features stripped to try to reduce memory footprints
	#undef RUNTIMELIGHTING	//too slow anyway
	#undef Q2CLIENT
	#undef Q2SERVER	//requires a dll anyway.
	#undef Q3CLIENT
	#undef Q3SERVER //trying to trim memory use
//	#undef Q2BSPS	//emscripten can't cope with bss, leading to increased download time. too lazy to fix.
//	#undef Q3BSPS	//emscripten can't cope with bss, leading to increased download time. too lazy to fix.
//	#undef PSET_SCRIPT	//bss+size
	#define GLSLONLY	//pointless having the junk
	#define GLESONLY	//should reduce the conditions a little
	#define R_MAX_RECURSE 2 //less bss
//	#undef RTLIGHTS
#endif
#ifdef WINRT
	#undef TCPCONNECT	//err...
	#undef IRCCONNECT	//not happening
	#undef AVAIL_DSOUND	//yeah, good luck there
	#undef AVAIL_DINPUT	//nope, not supported.
	#undef SV_MASTER	//no socket interface
	#undef CL_MASTER	//no socket interface
	#undef WEBSERVER		//http/ftp servers
	#undef WEBCLIENT		//http/ftp clients.
	#undef MULTITHREAD
#endif
#ifdef ANDROID
	#undef RTLIGHTS
	#ifndef SPEEX_STATIC
		#undef VOICECHAT
	#endif
	#undef TEXTEDITOR
	#define GLESONLY	//should reduce the conditions a little
#endif
#if defined(NACL)
	#undef SUPPORT_ICE
	#undef CL_MASTER	//no sockets support
	#undef SV_MASTER	//noone uses this anyway
	#undef WEBSERVER	//no sockets support (certainly no servers)
	#undef TCPCONNECT
	#undef IRCCONNECT
	#define GLSLONLY	//pointless having the junk
	#define GLESONLY	//should reduce the conditions a little
#endif

#ifndef MULTITHREAD
	//database code requires threads to do stuff async.
	#undef USE_SQLITE
	#undef USE_MYSQL
#endif

#if defined(USE_SQLITE) || defined(USE_MYSQL)
	#define SQL
#endif

//fix things a little...
#ifdef NPQTV
	#define NPFTE
	#undef NPQTV
#endif
#ifdef NPFTE
	/*plugins require threads and stuff now, and http download support*/
	#ifndef MULTITHREAD
		#define MULTITHREAD
		#define WEBCLIENT
	#endif
#endif

#if (defined(NOLOADERTHREAD) || !defined(MULTITHREAD)) && defined(LOADERTHREAD)
	#undef LOADERTHREAD
#endif

#ifndef _WIN32
	#undef QTERM	//not supported - FIXME: move to native plugin
#endif

#if (defined(Q2CLIENT) || defined(Q2SERVER))
	#ifndef Q2BSPS
		#error "Q2 game support without Q2BSP support. doesn't make sense"
	#endif
	#if !defined(MD2MODELS) || !defined(SP2MODELS)
		#error "Q2 game support without full Q2 model support. doesn't make sense"
	#endif
#endif

#ifdef NPFTE
	#undef TEXTEDITOR
	#undef WEBSERVER
#endif

#ifndef AVAIL_ZLIB
	#undef SUPPORT_ICE	//depends upon zlib's crc32 for fingerprinting. I cba writing my own.
#endif

#ifdef SERVERONLY	//remove options that don't make sense on only a server
	#undef Q2CLIENT
	#undef Q3CLIENT
	#undef HLCLIENT
	#undef HALFLIFEMODELS
	#undef VM_UI
	#undef VM_CG
	#undef WEBCLIENT
	#undef TEXTEDITOR
	#undef RUNTIMELIGHTING
	#undef DSPMODELS
	#undef SPRMODELS
	#undef SP2MODELS

	#undef PSET_SCRIPT
	#undef PSET_CLASSIC
	#undef PSET_DARKPLACES
#endif
#ifdef CLIENTONLY	//remove optional server components that make no sence on a client only build.
	#undef Q2SERVER
	#undef Q3SERVER
	#undef HLSERVER
	#undef WEBSERVER
	#undef VM_Q1
	#undef SQL
#endif


#if defined(CSQC_DAT) || !defined(CLIENTONLY)	//use ode only if we have a constant world state, and the library is enbled in some form.
	#define USERBE
#endif

#if  defined(ZYMOTICMODELS) || defined(MD5MODELS) || defined(DPMMODELS) || defined(PSKMODELS) || defined(INTERQUAKEMODELS) 
	#define SKELETALMODELS	//defined if we have a skeletal model.
#endif
#if (defined(CSQC_DAT) || !defined(CLIENTONLY)) && defined(SKELETALMODELS)
	#define SKELETALOBJECTS	//the skeletal objects API is only used if we actually have skeletal models, and gamecode that uses the builtins.
#endif
#if !defined(USERBE) || !defined(SKELETALMODELS)
	#undef RAGDOLL	//not possible to ragdoll if we don't have certain other features.
#endif

//remove any options that depend upon GL.
#if !defined(GLQUAKE)
	#undef IMAGEFMT_DDS // this is dumb
	#undef IMAGEFMT_BLP // this is dumb
#endif

#if !defined(Q3BSPS)
	#undef Q3CLIENT //reconsider this (later)
	#undef Q3SERVER //reconsider this (later)
#endif

#ifndef Q3CLIENT
	#undef VM_CG	// :(
	#undef VM_UI
#else
	#define VM_CG
	#define VM_UI
#endif

#if defined(VM_Q1) || defined(VM_UI) || defined(VM_CG) || defined(Q3SERVER) || defined(PLUGINS)
	#define VM_ANY
#endif

#define PROTOCOLEXTENSIONS

#ifdef MINIMAL
	#define IFMINIMAL(x,y) x
#else
	#define IFMINIMAL(x,y) y
#endif

//#define PRE_SAYONE	2.487	//FIXME: remove.

// defs common to client and server

#ifndef PLATFORM
	#if defined(FTE_TARGET_WEB)
		#define PLATFORM		"Web"
	#elif defined(NACL)
		#define PLATFORM		"Nacl"
	#elif defined(_WIN32)
		#if defined(__amd64__)
			#define PLATFORM	"Win64"
		#else
			#define PLATFORM	"Win32"
		#endif
		#define ARCH_DL_POSTFIX ".dll"
	#elif defined(__CYGWIN__)
		#define PLATFORM		"Cygwin"	/*technically also windows*/
		#define ARCH_DL_POSTFIX ".dll"
	#elif defined(ANDROID)
		#define PLATFORM		"Android"	/*technically also linux*/
	#elif defined(__linux__)
		#if defined(__amd64__)
			#define PLATFORM	"Linux64"
		#else
			#define PLATFORM	"Linux"
		#endif
	#elif defined(__FreeBSD__)
		#define PLATFORM	"FreeBSD"
	#elif defined(__OpenBSD__)
		#define PLATFORM	"OpenBSD"
	#elif defined(__NetBSD__)
		#define PLATFORM	"NetBSD"
	#elif defined(__MORPHOS__)
		#define PLATFORM	"MorphOS"
	#elif defined(MACOSX)
		#define PLATFORM	"MacOS X"
	#else
		#define PLATFORM	"Unknown"
	#endif
#endif

#ifndef ARCH_DL_POSTFIX
	#define ARCH_DL_POSTFIX ".so"
#endif

#if defined(__amd64__)
	#define ARCH_CPU_POSTFIX "amd"
#elif defined(_M_IX86) || defined(__i386__)
	#define ARCH_CPU_POSTFIX "x86"
#elif defined(__powerpc__) || defined(__ppc__)
	#define ARCH_CPU_POSTFIX "ppc"
#elif defined(__arm__)
	#define ARCH_CPU_POSTFIX "arm"
#else
	#define ARCH_CPU_POSTFIX "unk"
#endif

#ifdef _MSC_VER
	#define VARGS __cdecl
	#define MSVCDISABLEWARNINGS
	#if _MSC_VER >= 1300
		#define FTE_DEPRECATED __declspec(deprecated)
		#ifndef _CRT_SECURE_NO_WARNINGS
			#define _CRT_SECURE_NO_WARNINGS
		#endif
		#ifndef _CRT_NONSTDC_NO_WARNINGS
			#define _CRT_NONSTDC_NO_WARNINGS
		#endif
	#endif
	#define NORETURN __declspec(noreturn)
#endif
#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
	#define FTE_DEPRECATED  __attribute__((__deprecated__))	//no idea about the actual gcc version
	#define LIKEPRINTF(x) __attribute__((format(printf,x,x+1)))
#endif
#if (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 5))
	#define NORETURN __attribute__((noreturn))
#endif

//I'm making my own restrict, because msvc's headers can't cope if I #define restrict to __restrict, and quite possibly other platforms too
#if __STDC_VERSION__ >= 199901L
	#define fte_restrict restrict
#elif defined(_MSC_VER)
	#define fte_restrict __restrict
#else
	#define fte_restrict
#endif


#ifndef FTE_DEPRECATED
#define FTE_DEPRECATED
#endif
#ifndef LIKEPRINTF
#define LIKEPRINTF(x)
#endif
#ifndef VARGS
#define VARGS
#endif
#ifndef NORETURN
#define NORETURN
#endif

#ifdef _WIN32
#define ZEXPORT VARGS
#define ZEXPORTVA VARGS
#endif


// !!! if this is changed, it must be changed in d_ifacea.h too !!!
#define CACHE_SIZE	32		// used to align key data structures

#define UNUSED(x)	(x = x)	// for pesky compiler / lint warnings

#define	MINIMUM_MEMORY	0x550000

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2

#define	SOUND_CHANNELS		8


#define	MAX_QPATH		128			// max length of a quake game pathname
#define	MAX_OSPATH		256			// max length of a filesystem pathname

#define	ON_EPSILON		0.1			// point on plane side epsilon

#define	MAX_NQMSGLEN	65536		// max length of a reliable message
#define MAX_Q2MSGLEN	1400
#define MAX_QWMSGLEN	1450
#define MAX_OVERALLMSGLEN	65536	// mvdsv sends packets this big
#define	MAX_DATAGRAM	1450		// max length of unreliable message
#define MAX_Q2DATAGRAM	MAX_Q2MSGLEN
#define	MAX_NQDATAGRAM	1024		// max length of unreliable message
#define MAX_OVERALLDATAGRAM MAX_DATAGRAM

#define MAX_BACKBUFLEN	1200

//
// per-level limits
//
#ifdef FTE_TARGET_WEB
#define MAX_EDICTS		((1<<15)-1)
#else
//#define	MAX_EDICTS		((1<<22)-1)			// expandable up to 22 bits
#define	MAX_EDICTS		((1<<18)-1)			// expandable up to 22 bits
#endif
#define	MAX_LIGHTSTYLES	255
#define MAX_STANDARDLIGHTSTYLES 64
#define	MAX_PRECACHE_MODELS		2048			// these are sent over the net as bytes/shorts
#define	MAX_PRECACHE_SOUNDS		1024			// so they cannot be blindly increased
#define MAX_SSPARTICLESPRE 1024			// precached particle effect names, for server-side pointparticles/trailparticles.
#define MAX_VWEP_MODELS 32

#define	MAX_CSMODELS		1024			// these live entirly clientside
#define MAX_CSPARTICLESPRE	1024

#define	SAVEGAME_COMMENT_LENGTH	39

#define	MAX_STYLESTRING	64

#define MAX_Q2EDICTS 1024

//
// stats are integers communicated to the client by the server
//
#define MAX_QW_STATS 32
enum {
STAT_HEALTH			= 0,
//STAT_FRAGS		= 1,
STAT_WEAPON			= 2,
STAT_AMMO			= 3,
STAT_ARMOR			= 4,
STAT_WEAPONFRAME	= 5,
STAT_SHELLS			= 6,
STAT_NAILS			= 7,
STAT_ROCKETS		= 8,
STAT_CELLS			= 9,
STAT_ACTIVEWEAPON	= 10,
STAT_TOTALSECRETS	= 11,
STAT_TOTALMONSTERS	= 12,
STAT_SECRETS		= 13,		// bumped on client side by svc_foundsecret
STAT_MONSTERS		= 14,		// bumped by svc_killedmonster
STAT_ITEMS			= 15,
STAT_VIEWHEIGHT		= 16,	//same as zquake
STAT_TIME			= 17,	//zquake
STAT_MATCHSTARTTIME = 18,
#ifdef SIDEVIEWS
STAT_VIEW2			= 20,
#endif
STAT_VIEWZOOM		= 21, // DP

//these stats are used only when running a hexen2 mod/hud, and will never be used for a quake mod/hud/generic code.
STAT_H2_LEVEL	= 32,				// changes stat bar
STAT_H2_INTELLIGENCE,				// changes stat bar
STAT_H2_WISDOM,						// changes stat bar
STAT_H2_STRENGTH,					// changes stat bar
STAT_H2_DEXTERITY,					// changes stat bar
STAT_H2_BLUEMANA,					// changes stat bar
STAT_H2_GREENMANA,					// changes stat bar
STAT_H2_EXPERIENCE,					// changes stat bar
STAT_H2_CNT_TORCH,					// changes stat bar
STAT_H2_CNT_H_BOOST,				// changes stat bar
STAT_H2_CNT_SH_BOOST,				// changes stat bar
STAT_H2_CNT_MANA_BOOST,				// changes stat bar
STAT_H2_CNT_TELEPORT,				// changes stat bar
STAT_H2_CNT_TOME,					// changes stat bar
STAT_H2_CNT_SUMMON,					// changes stat bar
STAT_H2_CNT_INVISIBILITY,			// changes stat bar
STAT_H2_CNT_GLYPH,					// changes stat bar
STAT_H2_CNT_HASTE,					// changes stat bar
STAT_H2_CNT_BLAST,					// changes stat bar
STAT_H2_CNT_POLYMORPH,				// changes stat bar
STAT_H2_CNT_FLIGHT,					// changes stat bar
STAT_H2_CNT_CUBEOFFORCE,			// changes stat bar
STAT_H2_CNT_INVINCIBILITY,			// changes stat bar
STAT_H2_ARTIFACT_ACTIVE,
STAT_H2_ARTIFACT_LOW,
STAT_H2_MOVETYPE,
STAT_H2_CAMERAMODE,
STAT_H2_HASTED,
STAT_H2_INVENTORY,
STAT_H2_RINGS_ACTIVE,

STAT_H2_RINGS_LOW,
STAT_H2_ARMOUR1,
STAT_H2_ARMOUR2,
STAT_H2_ARMOUR3,
STAT_H2_ARMOUR4,
STAT_H2_FLIGHT_T,
STAT_H2_WATER_T,
STAT_H2_TURNING_T,
STAT_H2_REGEN_T,
STAT_H2_PUZZLE1,
STAT_H2_PUZZLE2,
STAT_H2_PUZZLE3,
STAT_H2_PUZZLE4,
STAT_H2_PUZZLE5,
STAT_H2_PUZZLE6,
STAT_H2_PUZZLE7,
STAT_H2_PUZZLE8,
STAT_H2_MAXHEALTH,
STAT_H2_MAXMANA,
STAT_H2_FLAGS,
STAT_H2_PLAYERCLASS,

STAT_H2_OBJECTIVE1,
STAT_H2_OBJECTIVE2,


STAT_MOVEVARS_AIRACCEL_QW_STRETCHFACTOR		= 220, // DP
STAT_MOVEVARS_AIRCONTROL_PENALTY			= 221, // DP
STAT_MOVEVARS_AIRSPEEDLIMIT_NONQW 			= 222, // DP
STAT_MOVEVARS_AIRSTRAFEACCEL_QW 			= 223, // DP
STAT_MOVEVARS_AIRCONTROL_POWER				= 224, // DP
STAT_MOVEFLAGS                              = 225, // DP
STAT_MOVEVARS_WARSOWBUNNY_AIRFORWARDACCEL	= 226, // DP
STAT_MOVEVARS_WARSOWBUNNY_ACCEL				= 227, // DP
STAT_MOVEVARS_WARSOWBUNNY_TOPSPEED			= 228, // DP
STAT_MOVEVARS_WARSOWBUNNY_TURNACCEL			= 229, // DP
STAT_MOVEVARS_WARSOWBUNNY_BACKTOSIDERATIO	= 230, // DP
STAT_MOVEVARS_AIRSTOPACCELERATE				= 231, // DP
STAT_MOVEVARS_AIRSTRAFEACCELERATE			= 232, // DP
STAT_MOVEVARS_MAXAIRSTRAFESPEED				= 233, // DP
STAT_MOVEVARS_AIRCONTROL					= 234, // DP
STAT_FRAGLIMIT								= 235, // DP
STAT_TIMELIMIT								= 236, // DP
STAT_MOVEVARS_WALLFRICTION					= 237, // DP
STAT_MOVEVARS_FRICTION						= 238, // DP
STAT_MOVEVARS_WATERFRICTION					= 239, // DP
STAT_MOVEVARS_TICRATE						= 240, // DP
STAT_MOVEVARS_TIMESCALE						= 241, // DP
STAT_MOVEVARS_GRAVITY						= 242, // DP
STAT_MOVEVARS_STOPSPEED						= 243, // DP
STAT_MOVEVARS_MAXSPEED						= 244, // DP
STAT_MOVEVARS_SPECTATORMAXSPEED				= 245, // DP
STAT_MOVEVARS_ACCELERATE					= 246, // DP
STAT_MOVEVARS_AIRACCELERATE					= 247, // DP
STAT_MOVEVARS_WATERACCELERATE				= 248, // DP
STAT_MOVEVARS_ENTGRAVITY					= 249, // DP
STAT_MOVEVARS_JUMPVELOCITY					= 250, // DP
STAT_MOVEVARS_EDGEFRICTION					= 251, // DP
STAT_MOVEVARS_MAXAIRSPEED					= 252, // DP
STAT_MOVEVARS_STEPHEIGHT					= 253, // DP
STAT_MOVEVARS_AIRACCEL_QW					= 254, // DP
STAT_MOVEVARS_AIRACCEL_SIDEWAYS_FRICTION	= 255, // DP

	MAX_CL_STATS = 256
};

//
// item flags
//
#define	IT_SHOTGUN				1
#define	IT_SUPER_SHOTGUN		2
#define	IT_NAILGUN				4
#define	IT_SUPER_NAILGUN		8

#define	IT_GRENADE_LAUNCHER		16
#define	IT_ROCKET_LAUNCHER		32
#define	IT_LIGHTNING			64
#define	IT_SUPER_LIGHTNING		128

#define	IT_SHELLS				256
#define	IT_NAILS				512
#define	IT_ROCKETS				1024
#define	IT_CELLS				2048

#define	IT_AXE					4096

#define	IT_ARMOR1				8192
#define	IT_ARMOR2				16384
#define	IT_ARMOR3				32768

#define	IT_SUPERHEALTH			65536

#define	IT_KEY1					131072
#define	IT_KEY2					262144

#define	IT_INVISIBILITY			524288

#define	IT_INVULNERABILITY		1048576
#define	IT_SUIT					2097152
#define	IT_QUAD					4194304

#define	IT_SIGIL1				(1<<28)

#define	IT_SIGIL2				(1<<29)
#define	IT_SIGIL3				(1<<30)
#define	IT_SIGIL4				(1<<31)

//
// print flags
//
#define	PRINT_LOW			0		// pickup messages
#define	PRINT_MEDIUM		1		// death messages
#define	PRINT_HIGH			2		// critical messages
#define	PRINT_CHAT			3		// chat messages



//split screen stuff
#define MAX_SPLITS 4




//savegame vars
#define	SAVEGAME_COMMENT_LENGTH	39
#define	SAVEGAME_VERSION	667


#define PM_DEFAULTSTEPHEIGHT	18


#define dem_cmd			0
#define dem_read		1
#define dem_set			2
#define dem_multiple	3
#define	dem_single		4
#define dem_stats		5
#define dem_all			6


#endif	//ifndef __BOTHDEFS_H
