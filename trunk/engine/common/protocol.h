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
// protocol.h -- communications protocols
#define PEXT_SETVIEW			0x00000001

#define PEXT_SCALE				0x00000002
#define PEXT_LIGHTSTYLECOL		0x00000004
#define PEXT_TRANS				0x00000008
#ifdef SIDEVIEWS
	#define PEXT_VIEW2			0x00000010
#endif
#define PEXT_BULLETENS			0x00000020
#ifdef AVAIL_ZLIB
//	#define PEXT_ZLIBDL			0x00000040
#endif
//#define PEXT_LIGHTUPDATES		0x00000080	//send progs/zap.mdl in the same mannor as a nail.
#define PEXT_FATNESS			0x00000100	//GL only (or servers)
#define PEXT_HLBSP				0x00000200
#define PEXT_TE_BULLET			0x00000400
#define PEXT_HULLSIZE			0x00000800
#define PEXT_MODELDBL			0x00001000
#define PEXT_ENTITYDBL			0x00002000	//max of 1024 ents instead of 512
#define PEXT_ENTITYDBL2			0x00004000	//max of 1024 ents instead of 512
#define PEXT_FLOATCOORDS		0x00008000	//supports floating point origins.
#define PEXT_VWEAP				0x00010000	//cause an extra qbyte to be sent, and an extra list of models for vweaps.
#ifdef Q2BSPS
#define PEXT_Q2BSP				0x00020000
#endif
#ifdef Q3BSPS
#define PEXT_Q3BSP				0x00040000
#endif
//#define PEXT_SEEF1				0x00080000
#define PEXT_SPLITSCREEN		0x00100000
#define PEXT_HEXEN2				0x00200000	//more stats and working particle builtin.
#define PEXT_SPAWNSTATIC2		0x00400000	//Sends an entity delta instead of a baseline.
#define PEXT_CUSTOMTEMPEFFECTS	0x00800000	//supports custom temp ents.
#define PEXT_256PACKETENTITIES	0x01000000	//Client can recieve 256 packet entities.
//#define PEXT_64PLAYERS			0x02000000	//Client is able to cope with 64 players. Wow.
#define PEXT_SHOWPIC			0x04000000
#define PEXT_SETATTACHMENT		0x08000000	//md3 tags (needs networking, they need to lerp).
//#define PEXT_PK3DOWNLOADS		0x10000000	//retrieve a list of pk3s/pk3s/paks for downloading (with optional URL and crcs)
#define PEXT_CHUNKEDDOWNLOADS	0x20000000	//alternate file download method. Hopefully it'll give quadroupled download speed, especially on higher pings.

#ifdef CSQC_DAT
#define PEXT_CSQC				0x40000000	//csqc additions
#endif



//ZQuake transparent protocol extensions.
#define Z_EXT_PM_TYPE		(1<<0)	// basic PM_TYPE functionality (reliable jump_held)
#define Z_EXT_PM_TYPE_NEW	(1<<1)	// adds PM_FLY, PM_SPECTATOR
#define Z_EXT_VIEWHEIGHT	(1<<2)	// STAT_VIEWHEIGHT
#define Z_EXT_SERVERTIME	(1<<3)	// STAT_TIME
#define Z_EXT_PITCHLIMITS	(1<<4)	// serverinfo maxpitch & minpitch
#define Z_EXT_JOIN_OBSERVE	(1<<5)	// server: "join" and "observe" commands are supported
									// client: on-the-fly spectator <-> player switching supported
#define SUPPORTED_Z_EXTENSIONS (Z_EXT_PM_TYPE|Z_EXT_PM_TYPE_NEW|Z_EXT_VIEWHEIGHT|Z_EXT_SERVERTIME|Z_EXT_PITCHLIMITS|Z_EXT_JOIN_OBSERVE)


#define PROTOCOL_VERSION_FTE			(('F'<<0) + ('T'<<8) + ('E'<<16) + ('X' << 24))	//fte extensions.
#define PROTOCOL_VERSION_HUFFMAN		(('H'<<0) + ('U'<<8) + ('F'<<16) + ('F' << 24))	//packet compression

#define	PROTOCOL_VERSION	28
#define	PROTOCOL_VERSION_Q2_MIN	31
#define	PROTOCOL_VERSION_Q2	34

//=========================================

#define	PORT_CLIENT	27001
#define	PORT_MASTER	27000
#define	PORT_SERVER	27500
#define Q2PORT_CLIENT 27901
#define Q2PORT_SERVER 27910

//=========================================

// out of band message id bytes

// M = master, S = server, C = client, A = any
// the second character will allways be \n if the message isn't a single
// qbyte long (?? not true anymore?)

#define	S2C_CHALLENGE		'c'
#define	S2C_CONNECTION		'j'
#define	A2A_PING			'k'	// respond with an A2A_ACK
#define	A2A_ACK				'l'	// general acknowledgement without info
#define	A2A_NACK			'm'	// [+ comment] general failure
#define A2A_ECHO			'e' // for echoing
#define	A2C_PRINT			'n'	// print a message on client

#define	S2M_HEARTBEAT		'a'	// + serverinfo + userlist + fraglist
#define	A2C_CLIENT_COMMAND	'B'	// + command line
#define	S2M_SHUTDOWN		'C'

#define M2C_MASTER_REPLY	'd'	// + \n + qw server port list
//==================
// note that there are some defs.qc that mirror to these numbers
// also related to svc_strings[] in cl_parse
//==================

//
// server to client
//
#define	svc_bad				0
#define	svc_nop				1
#define	svc_disconnect		2
#define	svc_updatestat		3	// [qbyte] [qbyte]
#define	svc_version			4	// [long] server version
#define	svc_setview			5	// [short] entity number
#define	svc_sound			6	// <see code>
#define	svc_time			7	// [float] server time
#define	svc_print			8	// [qbyte] id [string] null terminated string
#define	svc_stufftext		9	// [string] stuffed into client's console buffer
								// the string should be \n terminated
#define	svc_setangle		10	// [angle3] set the view angle to this absolute value

#define	svc_serverdata		11	// [long] protocol ...
#define	svc_lightstyle		12	// [qbyte] [string]
#define	svc_updatename		13	// [qbyte] [string]
#define	svc_updatefrags		14	// [qbyte] [short]
#define	svc_clientdata		15	// <shortbits + data>
#define	svc_stopsound		16	// <see code>
#define	svc_updatecolors	17	// [qbyte] [qbyte] [qbyte]
#define	svc_particle		18	// [vec3] <variable>
#define	svc_damage			19
	
#define	svc_spawnstatic		20
#define	svc_spawnstatic2	21
#define	svc_spawnbaseline	22
	
#define	svc_temp_entity		23	// variable
#define	svc_setpause		24	// [qbyte] on / off
#define	svc_signonnum		25	// [qbyte]  used for the signon sequence

#define	svc_centerprint		26	// [string] to put in center of the screen

#define	svc_killedmonster	27
#define	svc_foundsecret		28

#define	svc_spawnstaticsound	29	// [coord3] [qbyte] samp [qbyte] vol [qbyte] aten

#define	svc_intermission	30		// [vec3_t] origin [vec3_t] angle
#define	svc_finale			31		// [string] text

#define	svc_cdtrack			32		// [qbyte] track
#define svc_sellscreen		33

#define svc_cutscene		34	//hmm... nq only... added after qw tree splitt?

#define	svc_smallkick		34		// set client punchangle to 2
#define	svc_bigkick			35		// set client punchangle to 4

#define	svc_updateping		36		// [qbyte] [short]
#define	svc_updateentertime	37		// [qbyte] [float]

#define	svc_updatestatlong	38		// [qbyte] [long]

#define	svc_muzzleflash		39		// [short] entity

#define	svc_updateuserinfo	40		// [qbyte] slot [long] uid
									// [string] userinfo

#define	svc_download		41		// [short] size [size bytes]
#define	svc_playerinfo		42		// variable
#define	svc_nails			43		// [qbyte] num [48 bits] xyzpy 12 12 12 4 8 
#define	svc_chokecount		44		// [qbyte] packets choked
#define	svc_modellist		45		// [strings]
#define	svc_soundlist		46		// [strings]
#define	svc_packetentities	47		// [...]
#define	svc_deltapacketentities	48		// [...]
#define svc_maxspeed		49		// maxspeed change, for prediction
#define svc_entgravity		50		// gravity change, for prediction
#define svc_setinfo			51		// setinfo on a client
#define svc_serverinfo		52		// serverinfo
#define svcnq_effect		52		// [vector] org [byte] modelindex [byte] startframe [byte] framecount [byte] framerate
#define svcnq_effect2		53		// [vector] org [short] modelindex [short] startframe [byte] framecount [byte] framerate
#define svc_updatepl		53		// [qbyte] [qbyte]

#define svc_nails2			54		//qwe - [qbyte] num [52 bits] nxyzpy 8 12 12 12 4 8 


#ifdef PEXT_VIEW2
#define svc_view2			56
#endif
#ifdef PEXT_LIGHTSTYLECOL
#define svc_lightstylecol	57
#endif

#ifdef PEXT_BULLETENS
#define svc_bulletentext	58
#endif

#ifdef PEXT_LIGHTUPDATES
#define	svc_lightnings		59
#endif

#ifdef PEXT_MODELDBL
#define	svc_modellistshort	60		// [strings]
#endif

#define svc_ftesetclientpersist	61	//ushort DATA

#define svc_setportalstate 62

#define	svc_particle2		63
#define	svc_particle3		64
#define	svc_particle4		65
#define svc_spawnbaseline2	66

#define	svc_customtempent	67

#define svc_choosesplitclient 68
#define svc_showpic			69
#define svc_hidepic			70
#define svc_movepic			71
#define svc_updatepic		72

#define svc_setattachment	73

#define svcqw_effect			74		// [vector] org [byte] modelindex [byte] startframe [byte] framecount [byte] framerate
#define svcqw_effect2			75		// [vector] org [short] modelindex [short] startframe [byte] framecount [byte] framerate

#ifdef PEXT_CSQC
#define svc_csqcentities	76	//entity lump for csqc
#endif

#define svc_invalid			256


enum svcq2_ops_e
{
	svcq2_bad,	//0

	// these ops are known to the game dll
	svcq2_muzzleflash,	//1
	svcq2_muzzleflash2,	//2
	svcq2_temp_entity,	//3
	svcq2_layout,		//4
	svcq2_inventory,	//5

	// the rest are private to the client and server
	svcq2_nop,			//6
	svcq2_disconnect,	//7
	svcq2_reconnect,	//8
	svcq2_sound,		//9			// <see code>
	svcq2_print,		//10			// [qbyte] id [string] null terminated string
	svcq2_stufftext,	//11			// [string] stuffed into client's console buffer, should be \n terminated
	svcq2_serverdata,	//12			// [long] protocol ...
	svcq2_configstring,	//13		// [short] [string]
	svcq2_spawnbaseline,//14		
	svcq2_centerprint,	//15		// [string] to put in center of the screen
	svcq2_download,		//16		// [short] size [size bytes]
	svcq2_playerinfo,	//17			// variable
	svcq2_packetentities,//18			// [...]
	svcq2_deltapacketentities,//19	// [...]
	svcq2_frame			//20 (the bastard to implement.)
};

enum clcq2_ops_e
{
	clcq2_bad,
	clcq2_nop, 		
	clcq2_move,				// [[usercmd_t]
	clcq2_userinfo,			// [[userinfo string]
	clcq2_stringcmd			// [string] message
};


//==============================================

//
// client to server
//
#define	clc_bad			0
#define	clc_nop 		1
#define	clc_disconnect	2	//nq only
#define	clc_move		3		// [[usercmd_t]
#define	clc_stringcmd	4		// [string] message
#define	clc_delta		5		// [qbyte] sequence number, requests delta compression of message
#define clc_tmove		6		// teleport request, spectator only
#define clc_upload		7		// teleport request, spectator only


//==============================================

// playerinfo flags from server
// playerinfo allways sends: playernum, flags, origin[] and framenumber

#define	PF_MSEC			(1<<0)
#define	PF_COMMAND		(1<<1)
#define	PF_VELOCITY1	(1<<2)
#define	PF_VELOCITY2	(1<<3)
#define	PF_VELOCITY3	(1<<4)
#define	PF_MODEL		(1<<5)
#define	PF_SKINNUM		(1<<6)
#define	PF_EFFECTS		(1<<7)
#define	PF_WEAPONFRAME	(1<<8)		// only sent for view player
#define	PF_DEAD			(1<<9)		// don't block movement any more
#define	PF_GIB			(1<<10)		// offset the view height differently

//ZQuake.
#define	PF_PMC_MASK		((1<<11)	+\
						 (1<<12)	+\
						 (1<<13))

#define PF_EXTRA_PFS	(1<<15)

//FIXME: Resolve this.

// bits 11..13 are player move type bits

#ifdef PEXT_SCALE
#define	PF_SCALE_NOZ		(1<<12)
#define	PF_SCALE_Z			(1<<16)
#endif
#ifdef PEXT_TRANS
#define	PF_TRANS_NOZ		(1<<13)
#define	PF_TRANS_Z			(1<<17)
#endif
#ifdef PEXT_FATNESS
#define	PF_FATNESS_NOZ		(1<<14)
#define	PF_FATNESS_Z		(1<<18)
#endif
#ifdef PEXT_HULLSIZE
#define PF_HULLSIZE_NOZ		(1<<15)
#define	PF_HULLSIZE_Z		(1<<14)
#endif

//#define	PF_ORIGINDBL		(1<<19)



#define PF_PMC_SHIFT	11



// player move types
#define PMC_NORMAL			0		// normal ground movement
#define PMC_NORMAL_JUMP_HELD	1	// normal ground novement + jump_held
#define PMC_OLD_SPECTATOR	2		// fly through walls (QW compatibility mode)
#define PMC_SPECTATOR		3		// fly through walls
#define PMC_FLY				4		// fly, bump into walls
#define PMC_NONE			5		// can't move (client had better lerp the origin...)
#define PMC_FREEZE			6		// TODO: lerp movement and viewangles
#define PMC_EXTRA3			7		// future extension

//any more will require a different protocol message.

//==============================================

// if the high bit of the client to server qbyte is set, the low bits are
// client move cmd bits
// ms and angle2 are allways sent, the others are optional
#define	CM_ANGLE1 	(1<<0)
#define	CM_ANGLE3 	(1<<1)
#define	CM_FORWARD	(1<<2)
#define	CM_SIDE		(1<<3)
#define	CM_UP		(1<<4)
#define	CM_BUTTONS	(1<<5)
#define	CM_IMPULSE	(1<<6)
#define	CM_ANGLE2 	(1<<7)

//sigh...
#define	Q2CM_ANGLE1 	(1<<0)
#define	Q2CM_ANGLE2 	(1<<1)
#define	Q2CM_ANGLE3 	(1<<2)
#define	Q2CM_FORWARD	(1<<3)
#define	Q2CM_SIDE		(1<<4)
#define	Q2CM_UP			(1<<5)
#define	Q2CM_BUTTONS	(1<<6)
#define	Q2CM_IMPULSE	(1<<7)

//==============================================

// the first 16 bits of a packetentities update holds 9 bits
// of entity number and 7 bits of flags
#define	U_ORIGIN1	(1<<9)
#define	U_ORIGIN2	(1<<10)
#define	U_ORIGIN3	(1<<11)
#define	U_ANGLE2	(1<<12)
#define	U_FRAME		(1<<13)
#define	U_REMOVE	(1<<14)		// REMOVE this entity, don't add it
#define	U_MOREBITS	(1<<15)

// if MOREBITS is set, these additional flags are read in next
#define	U_ANGLE1	(1<<0)
#define	U_ANGLE3	(1<<1)
#define	U_MODEL		(1<<2)
#define	U_COLORMAP	(1<<3)
#define	U_SKIN		(1<<4)
#define	U_EFFECTS	(1<<5)
#define	U_SOLID		(1<<6)		// the entity should be solid for prediction
#ifdef PROTOCOLEXTENSIONS
#define U_EVENMORE	(1<<7)	//extension info follows

//fte extensions
//EVENMORE flags
#ifdef PEXT_SCALE
#define U_SCALE		(1<<0)	//scaler of alias models
#endif
#ifdef PEXT_TRANS
#define U_TRANS		(1<<1)	//transparency value
#endif
#ifdef PEXT_FATNESS
#define U_FATNESS	(1<<2)	//qbyte describing how fat an alias model should be. (moves verticies along normals). Useful for vacuum chambers...
#endif
#ifdef PEXT_MODELDBL
#define U_MODELDBL	(1<<3)	//extra bit for modelindexes
#endif
//FIXME: IMPLEMENT
#ifdef PEXT_ENTITYDBL
#define U_ENTITYDBL	(1<<5)	//use an extra qbyte for origin parts, cos one of them is off
#endif
#ifdef PEXT_ENTITYDBL2
#define U_ENTITYDBL2 (1<<6)	//use an extra qbyte for origin parts, cos one of them is off
#endif
#define U_YETMORE	(1<<7)	//even more extension info stuff.

#define U_DRAWFLAGS	(1<<8)	//use an extra qbyte for origin parts, cos one of them is off
#define U_ABSLIGHT	(1<<9)	//Force a lightlevel

#ifdef PEXT_BIGORIGINS
#define U_ORIGINDBL	(1<<10)	//use an extra qbyte for origin parts, cos one of them is off
#endif


#endif





#define	Q2U_ORIGIN1	(1<<0)
#define	Q2U_ORIGIN2	(1<<1)
#define	Q2U_ANGLE2	(1<<2)
#define	Q2U_ANGLE3	(1<<3)
#define	Q2U_FRAME8	(1<<4)		// frame is a qbyte
#define	Q2U_EVENT		(1<<5)
#define	Q2U_REMOVE	(1<<6)		// REMOVE this entity, don't add it
#define	Q2U_MOREBITS1	(1<<7)		// read one additional qbyte

// second qbyte
#define	Q2U_NUMBER16	(1<<8)		// NUMBER8 is implicit if not set
#define	Q2U_ORIGIN3	(1<<9)
#define	Q2U_ANGLE1	(1<<10)
#define	Q2U_MODEL		(1<<11)
#define Q2U_RENDERFX8	(1<<12)		// fullbright, etc
#define	Q2U_EFFECTS8	(1<<14)		// autorotate, trails, etc
#define	Q2U_MOREBITS2	(1<<15)		// read one additional qbyte

// third qbyte
#define	Q2U_SKIN8		(1<<16)
#define	Q2U_FRAME16	(1<<17)		// frame is a short
#define	Q2U_RENDERFX16 (1<<18)	// 8 + 16 = 32
#define	Q2U_EFFECTS16	(1<<19)		// 8 + 16 = 32
#define	Q2U_MODEL2	(1<<20)		// weapons, flags, etc
#define	Q2U_MODEL3	(1<<21)
#define	Q2U_MODEL4	(1<<22)
#define	Q2U_MOREBITS3	(1<<23)		// read one additional qbyte

// fourth qbyte
#define	Q2U_OLDORIGIN	(1<<24)		// FIXME: get rid of this
#define	Q2U_SKIN16	(1<<25)
#define	Q2U_SOUND		(1<<26)
#define	Q2U_SOLID		(1<<27)

//==============================================

// a sound with no channel is a local only sound
// the sound field has bits 0-2: channel, 3-12: entity
#define	SND_VOLUME		(1<<15)		// a qbyte
#define	SND_ATTENUATION	(1<<14)		// a qbyte

#define DEFAULT_SOUND_PACKET_VOLUME 255
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0


#define	DEFAULT_VIEWHEIGHT	22


// svc_print messages have an id, so messages can be filtered
#define	PRINT_LOW			0
#define	PRINT_MEDIUM		1
#define	PRINT_HIGH			2
#define	PRINT_CHAT			3	// also go to chat buffer

//
// temp entity events
//
enum {
	TE_SPIKE			= 0,
	TE_SUPERSPIKE		= 1,
	TE_GUNSHOT			= 2,
	TE_EXPLOSION		= 3,
	TE_TAREXPLOSION		= 4,
	TE_LIGHTNING1		= 5,
	TE_LIGHTNING2		= 6,
	TE_WIZSPIKE			= 7,
	TE_KNIGHTSPIKE		= 8,
	TE_LIGHTNING3		= 9,
	TE_LAVASPLASH		= 10,
	TE_TELEPORT			= 11,

	TE_BLOOD			= 12,
	TE_LIGHTNINGBLOOD	= 13,

#ifdef PEXT_TE_BULLET
	TE_BULLET			= 14,
	TE_SUPERBULLET		= 15,
#endif

	TE_RAILTRAIL		= 17,

		// hexen 2
	TE_STREAM_CHAIN			= 25,
	TE_STREAM_SUNSTAFF1		= 26,
	TE_STREAM_SUNSTAFF2		= 27,
	TE_STREAM_LIGHTNING		= 28,
	TE_STREAM_COLORBEAM		= 29,
	TE_STREAM_ICECHUNKS		= 30,
	TE_STREAM_GAZE			= 31,
	TE_STREAM_FAMINE		= 32,

	TE_BIGGRENADE			= 33,
	TE_CHUNK				= 34,
	TE_HWBONEPOWER			= 35,
	TE_HWBONEPOWER2			= 36,
	TE_METEORHIT			= 37,
	TE_HWRAVENDIE			= 38,
	TE_HWRAVENEXPLODE		= 39,
	TE_XBOWHIT				= 40,

	TE_CHUNK2				= 41,
	TE_ICEHIT				= 42,
	TE_ICESTORM				= 43,
	TE_HWMISSILEFLASH		= 44,
	TE_SUNSTAFF_CHEAP		= 45,
	TE_LIGHTNING_HAMMER		= 46,
	TE_DRILLA_EXPLODE		= 47,
	TE_DRILLA_DRILL			= 48,

	TE_HWTELEPORT			= 49,
	TE_SWORD_EXPLOSION		= 50,

	TE_AXE_BOUNCE			= 51,
	TE_AXE_EXPLODE			= 52,
	TE_TIME_BOMB			= 53,
	TE_FIREBALL				= 54,
	TE_SUNSTAFF_POWER		= 55,
	TE_PURIFY2_EXPLODE		= 56,
	TE_PLAYER_DEATH			= 57,
	TE_PURIFY1_EFFECT		= 58,
	TE_TELEPORT_LINGER		= 59,
	TE_LINE_EXPLOSION		= 60,
	TE_METEOR_CRUSH			= 61,
//MISSION PACK
	TE_STREAM_LIGHTNING_SMALL	= 62,

	TE_ACIDBALL				= 63,
	TE_ACIDBLOB				= 64,
	TE_FIREWALL				= 65,
	TE_FIREWALL_IMPACT		= 66,
	TE_HWBONERIC			= 67,
	TE_POWERFLAME			= 68,
	TE_BLOODRAIN			= 69,
	TE_AXE					= 70,
	TE_PURIFY2_MISSILE		= 71,
	TE_SWORD_SHOT			= 72,
	TE_ICESHOT				= 73,
	TE_METEOR				= 74,
	TE_LIGHTNINGBALL		= 75,
	TE_MEGAMETEOR			= 76,
	TE_CUBEBEAM				= 77,
	TE_LIGHTNINGEXPLODE		= 78,
	TE_ACID_BALL_FLY		= 79,
	TE_ACID_BLOB_FLY		= 80,
	TE_CHAINLIGHTNING		= 81
};

#define NQTE_EXPLOSION2	12
#define NQTE_BEAM		13


#define TE_SEEF_BRIGHTFIELD	200
#define TE_SEEF_DARKLIGHT	201
#define TE_SEEF_DARKFIELD	202
#define	TE_SEEF_LIGHT		203

//FTE's version of TEI_SHOWLMP2
#define SL_ORG_NW	0
#define SL_ORG_NE	1
#define SL_ORG_SW	2
#define SL_ORG_SE	3
#define SL_ORG_CC	4
#define SL_ORG_CN	5
#define SL_ORG_CS	6
#define SL_ORG_CW	7
#define SL_ORG_CE	8

/*
==========================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

==========================================================
*/

#define	MAX_CLIENTS		32

#define	UPDATE_BACKUP	64	// copies of entity_state_t to keep buffered
							// must be power of two
#define	UPDATE_MASK		(UPDATE_BACKUP-1)

#define	Q2UPDATE_BACKUP	16	// copies of entity_state_t to keep buffered
							// must be power of two
#define	Q2UPDATE_MASK		(Q2UPDATE_BACKUP-1)

#define	Q3UPDATE_BACKUP	32	// copies of entity_state_t to keep buffered
							// must be power of two
#define	Q3UPDATE_MASK		(Q3UPDATE_BACKUP-1)


// entity_state_t is the information conveyed from the server
// in an update message

//FIXME: split the q2 vars.
#ifdef SERVERONLY
typedef struct entity_state_s
{
	int		number;			// edict index

	int		flags;			// nolerp, etc
	vec3_t	origin;
	vec3_t	angles;
	int		modelindex;
	int		frame;
	int		colormap;
	int		skinnum;
	int		effects;

#ifdef PEXT_SCALE
	float	scale;
#endif
#ifdef PEXT_TRANS
	float	trans;
#endif
#ifdef PEXT_FATNESS
	float fatness;
#endif

	int		drawflags;
	int		abslight;
} entity_state_t;
#else
typedef struct entity_state_s
{
	int		number;			// edict index

	int		flags;			// nolerp, etc
	vec3_t	origin;
	vec3_t	old_origin;		//q2
	vec3_t	angles;
	int		modelindex;
	int		modelindex2;	//q2
	int		modelindex3;	//q2
	int		modelindex4;	//q2
	int		frame;
	int		colormap;
	int		skinnum;
	int		effects;
	int		renderfx;		//q2
	int		sound;			//q2
	int		event;			//q2

	int		solid;

#ifdef PEXT_SCALE
	float	scale;
#endif
#ifdef PEXT_TRANS
	float	trans;
#endif
#ifdef PEXT_FATNESS
	float	fatness;
#endif
	int		drawflags;
	int		abslight;
} entity_state_t;
#endif

#define MAX_EXTENDED_PACKET_ENTITIES	256	//sanity limit.
#define	MAX_STANDARD_PACKET_ENTITIES	64	// doesn't count nails
#define	MAX_MVDPACKET_ENTITIES	196	// doesn't count nails
typedef struct
{
	int		num_entities;
	int		max_entities;
	entity_state_t	*entities;
} packet_entities_t;

typedef struct usercmd_s
{
	//the first members of this structure MUST match the q2 version
	qbyte	msec;
	qbyte	buttons;
	short	angles[3];
	short	forwardmove, sidemove, upmove;
	qbyte	impulse;
	qbyte lightlevel;
	qbyte weapon;
	int servertime;
} usercmd_t;

typedef struct q2usercmd_s
{
	qbyte	msec;
	qbyte	buttons;
	short	angles[3];
	short	forwardmove, sidemove, upmove;
	qbyte	impulse;
	qbyte lightlevel;
} q2usercmd_t;

typedef struct q1usercmd_s
{
	qbyte	msec;
	vec3_t	angles;
	short	forwardmove, sidemove, upmove;
	qbyte	buttons;
	qbyte	impulse;
} q1usercmd_t;
#define SHORT2ANGLE(x) (x) * (360.0/65536)




//
// per-level limits
//
#define	Q2MAX_CLIENTS			256		// absolute limit
#define	Q2MAX_EDICTS			1024	// must change protocol to increase more
#define	Q2MAX_LIGHTSTYLES		256
#define	Q2MAX_MODELS			256		// these are sent over the net as bytes
#define	Q2MAX_SOUNDS			256		// so they cannot be blindly increased
#define	Q2MAX_IMAGES			256
#define	Q2MAX_ITEMS				256
#define Q2MAX_GENERAL			(Q2MAX_CLIENTS*2)	// general config strings


#define	Q2CS_NAME				0
#define	Q2CS_CDTRACK			1
#define	Q2CS_SKY				2
#define	Q2CS_SKYAXIS			3		// %f %f %f format
#define	Q2CS_SKYROTATE			4
#define	Q2CS_STATUSBAR			5		// display program string

#define Q2CS_AIRACCEL			29		// air acceleration control
#define	Q2CS_MAXCLIENTS			30
#define	Q2CS_MAPCHECKSUM		31		// for catching cheater maps

#define	Q2CS_MODELS				32
#define	Q2CS_SOUNDS				(Q2CS_MODELS	+Q2MAX_MODELS)
#define	Q2CS_IMAGES				(Q2CS_SOUNDS	+Q2MAX_SOUNDS)
#define	Q2CS_LIGHTS				(Q2CS_IMAGES	+Q2MAX_IMAGES)
#define	Q2CS_ITEMS				(Q2CS_LIGHTS	+Q2MAX_LIGHTSTYLES)
#define	Q2CS_PLAYERSKINS		(Q2CS_ITEMS		+Q2MAX_ITEMS)
#define Q2CS_GENERAL			(Q2CS_PLAYERSKINS	+Q2MAX_CLIENTS)
#define	Q2MAX_CONFIGSTRINGS		(Q2CS_GENERAL	+Q2MAX_GENERAL)


// player_state->stats[] indexes
#define Q2STAT_HEALTH_ICON		0
#define	Q2STAT_HEALTH				1
#define	Q2STAT_AMMO_ICON			2
#define	Q2STAT_AMMO				3
#define	Q2STAT_ARMOR_ICON			4
#define	Q2STAT_ARMOR				5
#define	Q2STAT_SELECTED_ICON		6
#define	Q2STAT_PICKUP_ICON		7
#define	Q2STAT_PICKUP_STRING		8
#define	Q2STAT_TIMER_ICON			9
#define	Q2STAT_TIMER				10
#define	Q2STAT_HELPICON			11
#define	Q2STAT_SELECTED_ITEM		12
#define	Q2STAT_LAYOUTS			13
#define	Q2STAT_FRAGS				14
#define	Q2STAT_FLASHES			15		// cleared each frame, 1 = health, 2 = armor
#define Q2STAT_CHASE				16
#define Q2STAT_SPECTATOR			17

#define	Q2MAX_STATS				32


//for the local player
#define	Q2PS_M_TYPE			(1<<0)
#define	Q2PS_M_ORIGIN			(1<<1)
#define	Q2PS_M_VELOCITY		(1<<2)
#define	Q2PS_M_TIME			(1<<3)
#define	Q2PS_M_FLAGS			(1<<4)
#define	Q2PS_M_GRAVITY		(1<<5)
#define	Q2PS_M_DELTA_ANGLES	(1<<6)

#define	Q2PS_VIEWOFFSET		(1<<7)
#define	Q2PS_VIEWANGLES		(1<<8)
#define	Q2PS_KICKANGLES		(1<<9)
#define	Q2PS_BLEND			(1<<10)
#define	Q2PS_FOV				(1<<11)
#define	Q2PS_WEAPONINDEX		(1<<12)
#define	Q2PS_WEAPONFRAME		(1<<13)
#define	Q2PS_RDFLAGS			(1<<14)



// entity_state_t->renderfx flags
#define	Q2RF_MINLIGHT			1		// allways have some light (viewmodel)
#define	Q2RF_VIEWERMODEL		2		// don't draw through eyes, only mirrors
#define	Q2RF_WEAPONMODEL		4		// only draw through eyes
#define	Q2RF_FULLBRIGHT			8		// allways draw full intensity
#define	Q2RF_DEPTHHACK			16		// for view weapon Z crunching
#define	Q2RF_TRANSLUCENT		32
#define	Q2RF_FRAMELERP			64
#define Q2RF_BEAM				128
#define	Q2RF_CUSTOMSKIN			256		// skin is an index in image_precache
#define	Q2RF_GLOW				512		// pulse lighting for bonus items
#define Q2RF_SHELL_RED			1024
#define	Q2RF_SHELL_GREEN		2048
#define Q2RF_SHELL_BLUE			4096

//ROGUE
#define Q2RF_IR_VISIBLE			0x00008000		// 32768
#define	Q2RF_SHELL_DOUBLE		0x00010000		// 65536
#define	Q2RF_SHELL_HALF_DAM		0x00020000
#define Q2RF_USE_DISGUISE		0x00040000
//ROGUE

// player_state_t->refdef flags
#define	Q2RDF_UNDERWATER		1		// warp the screen as apropriate
#define Q2RDF_NOWORLDMODEL		2		// used for player configuration screen

//ROGUE
#define	Q2RDF_IRGOGGLES			4
#define Q2RDF_UVGOGGLES			8
//ROGUE




#define	Q2SND_VOLUME		(1<<0)		// a qbyte
#define	Q2SND_ATTENUATION	(1<<1)		// a qbyte
#define	Q2SND_POS			(1<<2)		// three coordinates
#define	Q2SND_ENT			(1<<3)		// a short 0-2: channel, 3-12: entity
#define	Q2SND_OFFSET		(1<<4)		// a qbyte, msec offset from frame start

#define Q2DEFAULT_SOUND_PACKET_VOLUME	1.0
#define Q2DEFAULT_SOUND_PACKET_ATTENUATION 1.0


#define ATTN_NONE	0
#define ATTN_NORM	1
#define CHAN_AUTO   0
#define CHAN_WEAPON 1
#define CHAN_VOICE  2
#define CHAN_ITEM   3
#define CHAN_BODY   4

#define	Q2MZ_BLASTER			0
#define Q2MZ_MACHINEGUN		1
#define	Q2MZ_SHOTGUN			2
#define	Q2MZ_CHAINGUN1		3
#define	Q2MZ_CHAINGUN2		4
#define	Q2MZ_CHAINGUN3		5
#define	Q2MZ_RAILGUN			6
#define	Q2MZ_ROCKET			7
#define	Q2MZ_GRENADE			8
#define	Q2MZ_LOGIN			9
#define	Q2MZ_LOGOUT			10
#define	Q2MZ_RESPAWN			11
#define	Q2MZ_BFG				12
#define	Q2MZ_SSHOTGUN			13
#define	Q2MZ_HYPERBLASTER		14
#define	Q2MZ_ITEMRESPAWN		15
// RAFAEL
#define Q2MZ_IONRIPPER		16
#define Q2MZ_BLUEHYPERBLASTER 17
#define Q2MZ_PHALANX			18
#define Q2MZ_SILENCED			128		// bit flag ORed with one of the above numbers

//ROGUE
#define Q2MZ_ETF_RIFLE		30
#define Q2MZ_UNUSED			31
#define Q2MZ_SHOTGUN2			32
#define Q2MZ_HEATBEAM			33
#define Q2MZ_BLASTER2			34
#define	Q2MZ_TRACKER			35
#define	Q2MZ_NUKE1			36
#define	Q2MZ_NUKE2			37
#define	Q2MZ_NUKE4			38
#define	Q2MZ_NUKE8			39
//ROGUE

//
// monster muzzle flashes
//
#define Q2MZ2_TANK_BLASTER_1				1
#define Q2MZ2_TANK_BLASTER_2				2
#define Q2MZ2_TANK_BLASTER_3				3
#define Q2MZ2_TANK_MACHINEGUN_1			4
#define Q2MZ2_TANK_MACHINEGUN_2			5
#define Q2MZ2_TANK_MACHINEGUN_3			6
#define Q2MZ2_TANK_MACHINEGUN_4			7
#define Q2MZ2_TANK_MACHINEGUN_5			8
#define Q2MZ2_TANK_MACHINEGUN_6			9
#define Q2MZ2_TANK_MACHINEGUN_7			10
#define Q2MZ2_TANK_MACHINEGUN_8			11
#define Q2MZ2_TANK_MACHINEGUN_9			12
#define Q2MZ2_TANK_MACHINEGUN_10			13
#define Q2MZ2_TANK_MACHINEGUN_11			14
#define Q2MZ2_TANK_MACHINEGUN_12			15
#define Q2MZ2_TANK_MACHINEGUN_13			16
#define Q2MZ2_TANK_MACHINEGUN_14			17
#define Q2MZ2_TANK_MACHINEGUN_15			18
#define Q2MZ2_TANK_MACHINEGUN_16			19
#define Q2MZ2_TANK_MACHINEGUN_17			20
#define Q2MZ2_TANK_MACHINEGUN_18			21
#define Q2MZ2_TANK_MACHINEGUN_19			22
#define Q2MZ2_TANK_ROCKET_1				23
#define Q2MZ2_TANK_ROCKET_2				24
#define Q2MZ2_TANK_ROCKET_3				25

#define Q2MZ2_INFANTRY_MACHINEGUN_1		26
#define Q2MZ2_INFANTRY_MACHINEGUN_2		27
#define Q2MZ2_INFANTRY_MACHINEGUN_3		28
#define Q2MZ2_INFANTRY_MACHINEGUN_4		29
#define Q2MZ2_INFANTRY_MACHINEGUN_5		30
#define Q2MZ2_INFANTRY_MACHINEGUN_6		31
#define Q2MZ2_INFANTRY_MACHINEGUN_7		32
#define Q2MZ2_INFANTRY_MACHINEGUN_8		33
#define Q2MZ2_INFANTRY_MACHINEGUN_9		34
#define Q2MZ2_INFANTRY_MACHINEGUN_10		35
#define Q2MZ2_INFANTRY_MACHINEGUN_11		36
#define Q2MZ2_INFANTRY_MACHINEGUN_12		37
#define Q2MZ2_INFANTRY_MACHINEGUN_13		38

#define Q2MZ2_SOLDIER_BLASTER_1			39
#define Q2MZ2_SOLDIER_BLASTER_2			40
#define Q2MZ2_SOLDIER_SHOTGUN_1			41
#define Q2MZ2_SOLDIER_SHOTGUN_2			42
#define Q2MZ2_SOLDIER_MACHINEGUN_1		43
#define Q2MZ2_SOLDIER_MACHINEGUN_2		44

#define Q2MZ2_GUNNER_MACHINEGUN_1			45
#define Q2MZ2_GUNNER_MACHINEGUN_2			46
#define Q2MZ2_GUNNER_MACHINEGUN_3			47
#define Q2MZ2_GUNNER_MACHINEGUN_4			48
#define Q2MZ2_GUNNER_MACHINEGUN_5			49
#define Q2MZ2_GUNNER_MACHINEGUN_6			50
#define Q2MZ2_GUNNER_MACHINEGUN_7			51
#define Q2MZ2_GUNNER_MACHINEGUN_8			52
#define Q2MZ2_GUNNER_GRENADE_1			53
#define Q2MZ2_GUNNER_GRENADE_2			54
#define Q2MZ2_GUNNER_GRENADE_3			55
#define Q2MZ2_GUNNER_GRENADE_4			56

#define Q2MZ2_CHICK_ROCKET_1				57

#define Q2MZ2_FLYER_BLASTER_1				58
#define Q2MZ2_FLYER_BLASTER_2				59

#define Q2MZ2_MEDIC_BLASTER_1				60

#define Q2MZ2_GLADIATOR_RAILGUN_1			61

#define Q2MZ2_HOVER_BLASTER_1				62

#define Q2MZ2_ACTOR_MACHINEGUN_1			63

#define Q2MZ2_SUPERTANK_MACHINEGUN_1		64
#define Q2MZ2_SUPERTANK_MACHINEGUN_2		65
#define Q2MZ2_SUPERTANK_MACHINEGUN_3		66
#define Q2MZ2_SUPERTANK_MACHINEGUN_4		67
#define Q2MZ2_SUPERTANK_MACHINEGUN_5		68
#define Q2MZ2_SUPERTANK_MACHINEGUN_6		69
#define Q2MZ2_SUPERTANK_ROCKET_1			70
#define Q2MZ2_SUPERTANK_ROCKET_2			71
#define Q2MZ2_SUPERTANK_ROCKET_3			72

#define Q2MZ2_BOSS2_MACHINEGUN_L1			73
#define Q2MZ2_BOSS2_MACHINEGUN_L2			74
#define Q2MZ2_BOSS2_MACHINEGUN_L3			75
#define Q2MZ2_BOSS2_MACHINEGUN_L4			76
#define Q2MZ2_BOSS2_MACHINEGUN_L5			77
#define Q2MZ2_BOSS2_ROCKET_1				78
#define Q2MZ2_BOSS2_ROCKET_2				79
#define Q2MZ2_BOSS2_ROCKET_3				80
#define Q2MZ2_BOSS2_ROCKET_4				81

#define Q2MZ2_FLOAT_BLASTER_1				82

#define Q2MZ2_SOLDIER_BLASTER_3			83
#define Q2MZ2_SOLDIER_SHOTGUN_3			84
#define Q2MZ2_SOLDIER_MACHINEGUN_3		85
#define Q2MZ2_SOLDIER_BLASTER_4			86
#define Q2MZ2_SOLDIER_SHOTGUN_4			87
#define Q2MZ2_SOLDIER_MACHINEGUN_4		88
#define Q2MZ2_SOLDIER_BLASTER_5			89
#define Q2MZ2_SOLDIER_SHOTGUN_5			90
#define Q2MZ2_SOLDIER_MACHINEGUN_5		91
#define Q2MZ2_SOLDIER_BLASTER_6			92
#define Q2MZ2_SOLDIER_SHOTGUN_6			93
#define Q2MZ2_SOLDIER_MACHINEGUN_6		94
#define Q2MZ2_SOLDIER_BLASTER_7			95
#define Q2MZ2_SOLDIER_SHOTGUN_7			96
#define Q2MZ2_SOLDIER_MACHINEGUN_7		97
#define Q2MZ2_SOLDIER_BLASTER_8			98
#define Q2MZ2_SOLDIER_SHOTGUN_8			99
#define Q2MZ2_SOLDIER_MACHINEGUN_8		100

// --- Xian shit below ---
#define	Q2MZ2_MAKRON_BFG					101
#define Q2MZ2_MAKRON_BLASTER_1			102
#define Q2MZ2_MAKRON_BLASTER_2			103
#define Q2MZ2_MAKRON_BLASTER_3			104
#define Q2MZ2_MAKRON_BLASTER_4			105
#define Q2MZ2_MAKRON_BLASTER_5			106
#define Q2MZ2_MAKRON_BLASTER_6			107
#define Q2MZ2_MAKRON_BLASTER_7			108
#define Q2MZ2_MAKRON_BLASTER_8			109
#define Q2MZ2_MAKRON_BLASTER_9			110
#define Q2MZ2_MAKRON_BLASTER_10			111
#define Q2MZ2_MAKRON_BLASTER_11			112
#define Q2MZ2_MAKRON_BLASTER_12			113
#define Q2MZ2_MAKRON_BLASTER_13			114
#define Q2MZ2_MAKRON_BLASTER_14			115
#define Q2MZ2_MAKRON_BLASTER_15			116
#define Q2MZ2_MAKRON_BLASTER_16			117
#define Q2MZ2_MAKRON_BLASTER_17			118
#define Q2MZ2_MAKRON_RAILGUN_1			119
#define	Q2MZ2_JORG_MACHINEGUN_L1			120
#define	Q2MZ2_JORG_MACHINEGUN_L2			121
#define	Q2MZ2_JORG_MACHINEGUN_L3			122
#define	Q2MZ2_JORG_MACHINEGUN_L4			123
#define	Q2MZ2_JORG_MACHINEGUN_L5			124
#define	Q2MZ2_JORG_MACHINEGUN_L6			125
#define	Q2MZ2_JORG_MACHINEGUN_R1			126
#define	Q2MZ2_JORG_MACHINEGUN_R2			127
#define	Q2MZ2_JORG_MACHINEGUN_R3			128
#define	Q2MZ2_JORG_MACHINEGUN_R4			129
#define Q2MZ2_JORG_MACHINEGUN_R5			130
#define	Q2MZ2_JORG_MACHINEGUN_R6			131
#define Q2MZ2_JORG_BFG_1					132
#define Q2MZ2_BOSS2_MACHINEGUN_R1			133
#define Q2MZ2_BOSS2_MACHINEGUN_R2			134
#define Q2MZ2_BOSS2_MACHINEGUN_R3			135
#define Q2MZ2_BOSS2_MACHINEGUN_R4			136
#define Q2MZ2_BOSS2_MACHINEGUN_R5			137

//ROGUE
#define	Q2MZ2_CARRIER_MACHINEGUN_L1		138
#define	Q2MZ2_CARRIER_MACHINEGUN_R1		139
#define	Q2MZ2_CARRIER_GRENADE				140
#define Q2MZ2_TURRET_MACHINEGUN			141
#define Q2MZ2_TURRET_ROCKET				142
#define Q2MZ2_TURRET_BLASTER				143
#define Q2MZ2_STALKER_BLASTER				144
#define Q2MZ2_DAEDALUS_BLASTER			145
#define Q2MZ2_MEDIC_BLASTER_2				146
#define	Q2MZ2_CARRIER_RAILGUN				147
#define	Q2MZ2_WIDOW_DISRUPTOR				148
#define	Q2MZ2_WIDOW_BLASTER				149
#define	Q2MZ2_WIDOW_RAIL					150
#define	Q2MZ2_WIDOW_PLASMABEAM			151		// PMM - not used
#define	Q2MZ2_CARRIER_MACHINEGUN_L2		152
#define	Q2MZ2_CARRIER_MACHINEGUN_R2		153
#define	Q2MZ2_WIDOW_RAIL_LEFT				154
#define	Q2MZ2_WIDOW_RAIL_RIGHT			155
#define	Q2MZ2_WIDOW_BLASTER_SWEEP1		156
#define	Q2MZ2_WIDOW_BLASTER_SWEEP2		157
#define	Q2MZ2_WIDOW_BLASTER_SWEEP3		158
#define	Q2MZ2_WIDOW_BLASTER_SWEEP4		159
#define	Q2MZ2_WIDOW_BLASTER_SWEEP5		160
#define	Q2MZ2_WIDOW_BLASTER_SWEEP6		161
#define	Q2MZ2_WIDOW_BLASTER_SWEEP7		162
#define	Q2MZ2_WIDOW_BLASTER_SWEEP8		163
#define	Q2MZ2_WIDOW_BLASTER_SWEEP9		164
#define	Q2MZ2_WIDOW_BLASTER_100			165
#define	Q2MZ2_WIDOW_BLASTER_90			166
#define	Q2MZ2_WIDOW_BLASTER_80			167
#define	Q2MZ2_WIDOW_BLASTER_70			168
#define	Q2MZ2_WIDOW_BLASTER_60			169
#define	Q2MZ2_WIDOW_BLASTER_50			170
#define	Q2MZ2_WIDOW_BLASTER_40			171
#define	Q2MZ2_WIDOW_BLASTER_30			172
#define	Q2MZ2_WIDOW_BLASTER_20			173
#define	Q2MZ2_WIDOW_BLASTER_10			174
#define	Q2MZ2_WIDOW_BLASTER_0				175
#define	Q2MZ2_WIDOW_BLASTER_10L			176
#define	Q2MZ2_WIDOW_BLASTER_20L			177
#define	Q2MZ2_WIDOW_BLASTER_30L			178
#define	Q2MZ2_WIDOW_BLASTER_40L			179
#define	Q2MZ2_WIDOW_BLASTER_50L			180
#define	Q2MZ2_WIDOW_BLASTER_60L			181
#define	Q2MZ2_WIDOW_BLASTER_70L			182
#define	Q2MZ2_WIDOW_RUN_1					183
#define	Q2MZ2_WIDOW_RUN_2					184
#define	Q2MZ2_WIDOW_RUN_3					185
#define	Q2MZ2_WIDOW_RUN_4					186
#define	Q2MZ2_WIDOW_RUN_5					187
#define	Q2MZ2_WIDOW_RUN_6					188
#define	Q2MZ2_WIDOW_RUN_7					189
#define	Q2MZ2_WIDOW_RUN_8					190
#define	Q2MZ2_CARRIER_ROCKET_1			191
#define	Q2MZ2_CARRIER_ROCKET_2			192
#define	Q2MZ2_CARRIER_ROCKET_3			193
#define	Q2MZ2_CARRIER_ROCKET_4			194
#define	Q2MZ2_WIDOW2_BEAMER_1				195
#define	Q2MZ2_WIDOW2_BEAMER_2				196
#define	Q2MZ2_WIDOW2_BEAMER_3				197
#define	Q2MZ2_WIDOW2_BEAMER_4				198
#define	Q2MZ2_WIDOW2_BEAMER_5				199
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_1			200
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_2			201
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_3			202
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_4			203
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_5			204
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_6			205
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_7			206
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_8			207
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_9			208
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_10		209
#define	Q2MZ2_WIDOW2_BEAM_SWEEP_11		210






#define MAX_MAP_AREA_BYTES		32

// edict->drawflags (hexen2 stuff)
#define MLS_MASKIN				7	// Model Light Style
#define MLS_MASKOUT				248
#define MLS_NONE				0
#define MLS_FULLBRIGHT			1
#define MLS_POWERMODE			2
#define MLS_TORCH				3
#define MLS_TOTALDARK			4
#define MLS_ABSLIGHT			7
#define SCALE_TYPE_MASKIN		24
#define SCALE_TYPE_MASKOUT		231
#define SCALE_TYPE_UNIFORM		0	// Scale X, Y, and Z
#define SCALE_TYPE_XYONLY		8	// Scale X and Y
#define SCALE_TYPE_ZONLY		16	// Scale Z
#define SCALE_ORIGIN_MASKIN		96
#define SCALE_ORIGIN_MASKOUT	159
#define SCALE_ORIGIN_CENTER		0	// Scaling origin at object center
#define SCALE_ORIGIN_BOTTOM		32	// Scaling origin at object bottom
#define SCALE_ORIGIN_TOP		64	// Scaling origin at object top
#define DRF_TRANSLUCENT			128




