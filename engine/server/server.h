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
// server.h

#define	QW_SERVER

#include "../http/iweb.h"


#define	MAX_MASTERS	8				// max recipients for heartbeat packets

#define	MAX_SIGNON_BUFFERS	16

typedef enum {
	ss_dead,			// no map loaded
	ss_loading,			// spawning level edicts
	ss_active,			// actively running
	ss_cinematic
} server_state_t;
// some qc commands are only valid before the server has finished
// initializing (precache commands, static sounds / objects, etc)

#ifdef SVCHAT
typedef struct chatvar_s {
	char varname[64];
	float value;

	struct chatvar_s *next;
} chatvar_t;
typedef struct {
	qboolean active;
	char filename[64];
	edict_t *edict;

	char maintext[1024];
	struct
	{
		float tag;
		char text[256];
	} option[6];
	int options;

	chatvar_t *vars;

	float time;
} svchat_t;
#endif

typedef struct {
	int netstyle;
	char particleeffecttype[64];
	char stain[3];
	qbyte radius;
	qbyte dlightrgb[3];
	qbyte dlightradius;
	qbyte dlighttime;
	qbyte dlightcfade[3];
} svcustomtents_t;
#define CTE_CUSTOMCOUNT		1
#define CTE_CUSTOMDIRECTION	2
#define CTE_STAINS			4
#define CTE_GLOWS			8
#define CTE_CHANNELFADE     16
#define CTE_ISBEAM			128



typedef struct
{
	vec3_t	origin;
	char	angles[3];
	qbyte	modelindex;
	qbyte	frame;
	qbyte	colormap;
	qbyte	skinnum;
	qbyte	effects;

	qbyte	scale;
	qbyte	trans;
	char	fatness;
} mvdentity_state_t;

typedef struct
{
	qboolean	active;				// false when server is going down
	server_state_t	state;			// precache commands are only valid during load

	double		time;
	int framenum;
	
	int			lastcheck;			// used by PF_checkclient
	double		lastchecktime;		// for monster ai 

	qboolean	paused;				// are we paused?	

	//check player/eyes models for hacks
	unsigned	model_player_checksum;
	unsigned	eyes_player_checksum;
	
	char		name[64];			// file map name
	char		mapname[256];
	char		modelname[MAX_QPATH];		// maps/<name>.bsp, for model_precache[0]
	struct model_s 	*worldmodel;
	char		model_precache[MAX_MODELS][MAX_QPATH];	// NULL terminated
	char		sound_precache[MAX_SOUNDS][MAX_QPATH];	// NULL terminated
	char		image_precache[Q2MAX_IMAGES][MAX_QPATH];
	char		*lightstyles[MAX_LIGHTSTYLES];
	char		lightstylecolours[MAX_LIGHTSTYLES];
	struct model_s		*models[MAX_MODELS];

	char *statusbar;	//part of the config strings - q2 has a progs specified sbar.

	int			allocated_client_slots;	//number of slots available. (used mostly to stop single player saved games cacking up)
	int			max_edicts;	//limiting factor... 1024 fields*4*MAX_EDICTS == a heck of a lot.
	int			num_edicts;			// increases towards MAX_EDICTS
	edict_t		*edicts;			// can NOT be array indexed, because
									// edict_t is variable sized, but can
									// be used to reference the world ent

	qbyte		*pvs, *phs;			// fully expanded and decompressed

	// added to every client's unreliable buffer each frame, then cleared
	sizebuf_t	datagram;
	qbyte		datagram_buf[MAX_DATAGRAM];

	// added to every client's reliable buffer each frame, then cleared
	sizebuf_t	reliable_datagram;
	qbyte		reliable_datagram_buf[MAX_QWMSGLEN];

	// the multicast buffer is used to send a message to a set of clients	
	sizebuf_t	multicast;
	qbyte		multicast_buf[MAX_NQMSGLEN];

#ifdef NQPROT
	sizebuf_t	nqdatagram;
	qbyte		nqdatagram_buf[MAX_NQDATAGRAM];

	sizebuf_t	nqreliable_datagram;
	qbyte		nqreliable_datagram_buf[MAX_NQMSGLEN];

	sizebuf_t	nqmulticast;
	qbyte		nqmulticast_buf[MAX_NQMSGLEN];
#endif


	sizebuf_t	q2datagram;
	qbyte		q2datagram_buf[MAX_Q2DATAGRAM];

	sizebuf_t	q2reliable_datagram;
	qbyte		q2reliable_datagram_buf[MAX_Q2MSGLEN];

	sizebuf_t	q2multicast;
	qbyte		q2multicast_buf[MAX_Q2MSGLEN];

	// the master buffer is used for building log packets
	sizebuf_t	master;
	qbyte		master_buf[MAX_DATAGRAM];

	// the signon buffer will be sent to each client as they connect
	// includes the entity baselines, the static entities, etc
	// large levels will have >MAX_DATAGRAM sized signons, so 
	// multiple signon messages are kept
	sizebuf_t	signon;
	int			num_signon_buffers;
	int			signon_buffer_size[MAX_SIGNON_BUFFERS];
	qbyte		signon_buffers[MAX_SIGNON_BUFFERS][MAX_DATAGRAM];

	qboolean msgfromdemo;

	qboolean gamedirchanged;





	qboolean mvdrecording;

//====================================================
//this lot is for playback of demos

	qboolean mvdplayback;
	float realtime;
	FILE *demofile;	//also signifies playing the thing.

	int lasttype;
	int lastto;

//playback spikes (svc_nails/nails2)
	int numdemospikes;
	struct {
		vec3_t org;
		qbyte id;
		qbyte pitch;
		qbyte yaw;
		qbyte modelindex;
	} demospikes[255];

//playback of entities (svc_nails/nails2)
	mvdentity_state_t	*demostate;
	mvdentity_state_t	*demobaselines;
	int demomaxents;
	qboolean demostatevalid;

//players
	struct {
		int stats[MAX_CL_STATS];
		int pl;
		int ping;
		int frags;
		int userid;
		int weaponframe;
		char			userinfo[MAX_INFO_STRING];
		vec3_t velocity;
		vec3_t avelocity;
		float updatetime;
	} recordedplayer[MAX_CLIENTS];

//gamestate
	char		demoinfo[MAX_SERVERINFO_STRING];
	char		demmodel_precache[MAX_MODELS][MAX_QPATH];	// NULL terminated
	char		demsound_precache[MAX_SOUNDS][MAX_QPATH];	// NULL terminated
	char		demgamedir[64];
	char		demname[64];			// map name

	qboolean	democausesreconnect;	//this makes current clients go through the connection process (and when the demo ends too)
	sizebuf_t	demosignon;
	int			num_demosignon_buffers;
	int			demosignon_buffer_size[MAX_SIGNON_BUFFERS];
	qbyte		demosignon_buffers[MAX_SIGNON_BUFFERS][MAX_DATAGRAM];
	char		demfullmapname[64];

	char		*demolightstyles[MAX_LIGHTSTYLES];
//====================================================

	entity_state_t extendedstatics[MAX_STATIC_ENTITIES];
	int numextrastatics;
//	movevars_t	demomovevars;	//FIXME:!
//end this lot... (demo playback)

	svcustomtents_t customtents[255];
} server_t;


#define	NUM_SPAWN_PARMS			16

typedef enum
{
	cs_free,		// can be reused for a new connection
	cs_zombie,		// client has been disconnected, but don't reuse
					// connection for a couple seconds
	cs_connected,	// has been assigned to a client_t, but not in game yet	
	cs_spawned		// client is fully in game
} client_conn_state_t;

typedef struct
{
	// received from client

	// reply
	double				senttime;
	float				ping_time;
	packet_entities_t	entities;	//must come last (mvd states are bigger)
} client_frame_t;

#ifdef Q2SERVER
typedef struct	//merge?
{
	int					areabytes;
	qbyte				areabits[MAX_Q2MAP_AREAS/8];		// portalarea visibility bits
	q2player_state_t	ps;
	int					num_entities;
	int					first_entity;		// into the circular sv_packet_entities[]
	int					senttime;			// for ping calculations
} q2client_frame_t;
#endif

#define MAXCACHEDSOUNDBUFFERS 8
typedef struct {
	int socket;
	qboolean floodingbuffers;	//not enough sound causes this. Sound is then only mixed when full.
	qbyte *buffer[MAXCACHEDSOUNDBUFFERS];
} svvoicechat_t;

#define MAX_BACK_BUFFERS 16

enum seef_e{
	SEEF_BRIGHTFIELD,
	SEEF_DARKLIGHT,
	SEEF_DARKFIELD,
	SEEF_LIGHT
};

typedef struct specialenteffects_s {
	enum seef_e efnum;
	int entnum;

	vec3_t size;
	char offset;
	qbyte colour;	//matches q1 palette

	float die;

	struct specialenteffects_s *next;
} specialenteffects_t;

typedef struct client_s
{
	client_conn_state_t	state;

	int				spectator;			// non-interactive

	qboolean		sendinfo;			// at end of frame, send info to all
										// this prevents malicious multiple broadcasts
	float			lastnametime;		// time of last name change
	int				lastnamecount;		// time of last name change
	unsigned		checksum;			// checksum for calcs
	qboolean		drop;				// lose this guy next opportunity
	int				lossage;			// loss percentage

	int				userid;							// identifying number
	char			userinfo[MAX_INFO_STRING];		// infostring

	usercmd_t		lastcmd;			// for filling in big drops and partial predictions
	double			localtime;			// of last message
	qboolean jump_held;

	float			maxspeed;			// localized maxspeed
	float			entgravity;			// localized ent gravity

	int viewent;	//fake the entity positioning.
	
	edict_t			*edict;				// EDICT_NUM(clientnum+1)
#ifdef Q2SERVER
	q2edict_t		*q2edict;				// EDICT_NUM(clientnum+1)
#endif

	int				playerclass;
	char			team[32];
	char			name[32];			// for printing to other people
										// extracted from userinfo
	int				messagelevel;		// for filtering printed messages

	// the datagram is written to after every frame, but only cleared
	// when it is sent out to the client.  overflow is tolerated.
	sizebuf_t		datagram;
	qbyte			datagram_buf[MAX_DATAGRAM];

	// back buffers for client reliable data
	sizebuf_t	backbuf;
	int			num_backbuf;
	int			backbuf_size[MAX_BACK_BUFFERS];
	qbyte		backbuf_data[MAX_BACK_BUFFERS][MAX_BACKBUFLEN];

#ifdef NQPROT	
	qboolean nqprot;
#endif

	double			connection_started;	// or time of disconnect for zombies
	qboolean		send_message;		// set on frames a datagram arived on

// spawn parms are carried from level to level
	float			spawn_parms[NUM_SPAWN_PARMS];
	char			*spawninfo;
	float			spawninfotime;
	float			nextservertimeupdate;

// client known data for deltas	
	int				old_frags;
	
	int				stats[MAX_CL_STATS];

	union{	//save space
	client_frame_t	*frames;	// updates can be deltad from here
#ifdef Q2SERVER
	q2client_frame_t	*q2frames;
#endif
	};
	FILE			*download;			// file being downloaded
	int				downloadsize;		// total bytes
	int				downloadcount;		// bytes sent

	int				spec_track;			// entnum of player tracking

	//true/false/persist
	qbyte		ismuted;
	qbyte		iscuffed;
	qbyte		iscrippled;

	qbyte		istobeloaded;	//loadgame creates place holders for clients to connect to. Effectivly loading a game reconnects all clients, but has precreated ents.

	double			whensaid[10];       // JACK: For floodprots
 	int			whensaidhead;       // Head value for floodprots
 	double			lockedtill;

	qboolean		upgradewarn;		// did we warn him?

	FILE			*upload;
	char			uploadfn[MAX_QPATH];
	netadr_t		snap_from;
	qboolean		remote_snap;
 
//===== NETWORK ============
	int				chokecount;
	int				delta_sequence;		// -1 = no compression
	netchan_t		netchan;

	svvoicechat_t voicechat;

#ifdef SVCHAT
	svchat_t chat;
#endif
#ifdef SVRANKING
	int rankid;

	int	kills;
	int	deaths;

	double			stats_started;
#endif

#ifdef PROTOCOL_VERSION_FTE
	unsigned long fteprotocolextensions;
#endif
	unsigned long zquake_extensions;
	int isq2client;	//contains protocol version too.

//speed cheat testing
	int msecs;
	int msec_cheating;
	float last_check;

	qboolean gibfilter;

	int trustlevel;

	qboolean wasrecorded;	//this client shouldn't get any net messages sent to them

	int language;	//the clients language

	struct {
		qbyte vweap;
	} otherclientsknown[MAX_CLIENTS];	//updated as needed. Flag at a time, or all flags.

	specialenteffects_t *enteffects;

	struct client_s *controller;
	struct client_s *controlled;

	int rate;
	int drate;
} client_t;

// a client can leave the server in one of four ways:
// dropping properly by quiting or disconnecting
// timing out if no valid messages are received for timeout.value seconds
// getting kicked off by the server operator
// a program error, like an overflowed reliable buffer




//=============================================================================

//mvd stuff

#define	MSG_BUF_SIZE 8192

typedef struct
{
	vec3_t	origin;
	vec3_t	angles;
	int		weaponframe;
	int		skinnum;
	int		model;
	int		effects;
}	demoinfo_t;

typedef struct
{
	demoinfo_t	info;
	float		sec;
	int			parsecount;
	qboolean	fixangle;
	vec3_t		angle;
	float		cmdtime;
	int			flags;
	int			frame;
} demo_client_t;

typedef struct {
	qbyte type;
	qbyte full;
	int to;
	int size;
	qbyte data[1]; //gcc doesn't allow [] (?)
} header_t;

typedef struct
{
	qboolean	allowoverflow;	// if false, do a Sys_Error
	qboolean	overflowed;		// set to true if the buffer size failed
	qbyte	*data;
	int		maxsize;
	int		cursize;
	int		bufsize;
	header_t *h;
} demobuf_t;

typedef struct
{
	demo_client_t clients[MAX_CLIENTS];
	double		time;
	demobuf_t	buf;

} demo_frame_t;

typedef struct {
	qbyte	*data;
	int		start, end, last;
	int		maxsize;
} dbuffer_t;

#define DEMO_FRAMES 64
#define DEMO_FRAMES_MASK (DEMO_FRAMES - 1)

typedef struct
{
	demobuf_t	*dbuf;
	dbuffer_t	dbuffer;
	sizebuf_t	datagram;
	qbyte		datagram_data[MSG_BUF_SIZE];
	int			lastto;
	int			lasttype;
	double		time, pingtime;
	int			stats[MAX_CLIENTS][MAX_CL_STATS]; // ouch!
	client_t	recorder;
	qboolean	fixangle[MAX_CLIENTS];
	float		fixangletime[MAX_CLIENTS];
	vec3_t		angles[MAX_CLIENTS];
	char		name[MAX_OSPATH], path[MAX_OSPATH];
	int			parsecount;
	int			lastwritten;
	demo_frame_t	frames[DEMO_FRAMES];
	demoinfo_t	info[MAX_CLIENTS];
	qbyte		buffer[20*MAX_QWMSGLEN];
	int			bufsize;
	int			forceFrame;

	struct mvddest_s *dest;
} demo_t;


//=============================================================================


#define	STATFRAMES	100
typedef struct
{
	double	active;
	double	idle;
	int		count;
	int		packets;

	double	latched_active;
	double	latched_idle;
	int		latched_packets;
} svstats_t;

// MAX_CHALLENGES is made large to prevent a denial
// of service attack that could cycle all of them
// out before legitimate users connected
#define	MAX_CHALLENGES	1024

typedef struct
{
	netadr_t	adr;
	int			challenge;
	int			time;
} challenge_t;

typedef struct bannedips_s {
	struct bannedips_s *next;
	netadr_t	adr;
} bannedips_t;

typedef struct levelcache_s {
	struct levelcache_s *next;
	char *mapname;
} levelcache_t;

typedef struct
{
	int			spawncount;			// number of servers spawned since start,
									// used to check late spawns

	int socketip;
	int socketip6;
	int socketipx;

	client_t	clients[MAX_CLIENTS];
	int			serverflags;		// episode completion information
	
	double		last_heartbeat;
	int			heartbeat_sequence;
	svstats_t	stats;

	char		info[MAX_SERVERINFO_STRING];

	// log messages are used so that fraglog processes can get stats
	int			logsequence;	// the message currently being filled
	double		logtime;		// time of last swap
	sizebuf_t	log[2];
	qbyte		log_buf[2][MAX_DATAGRAM];

	challenge_t	challenges[MAX_CHALLENGES];	// to prevent invalid IPs from connecting

	bannedips_t *bannedips;

	char progsnames[MAX_PROGS][32];
	progsnum_t progsnum[MAX_PROGS];
	int numprogs;

#ifdef PROTOCOLEXTENSIONS
	unsigned long fteprotocolextensions;
#endif

	qboolean demoplayback;
	qboolean demorecording;
	qboolean msgfromdemo;

	int language;	//the server operators language

	levelcache_t *levcache;
} server_static_t;

//=============================================================================

// edict->movetype values
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
#define MOVETYPE_PUSHPULL		13		// pushable/pullable object
#define MOVETYPE_SWIM			14		// should keep the object in water

// edict->solid values
#define	SOLID_NOT				0		// no interaction with other objects
#define	SOLID_TRIGGER			1		// touch on edge, but not blocking
#define	SOLID_BBOX				2		// touch on edge, block
#define	SOLID_SLIDEBOX			3		// touch on edge, but not an onground
#define	SOLID_BSP				4		// bsp clip, touch on edge, block
#define	SOLID_PHASEH2			5
#define	SOLID_CORPSE			5
#define SOLID_LADDER			20		//dmw. touch on edge, not blocking. Touching players have different physics. Otherwise a SOLID_TRIGGER

// edict->deadflag values
#define	DEAD_NO					0
#define	DEAD_DYING				1
#define	DEAD_DEAD				2

#define	DAMAGE_NO				0
#define	DAMAGE_YES				1
#define	DAMAGE_AIM				2

// edict->flags
#define	FL_FLY					1
#define	FL_SWIM					2
#define	FL_GLIMPSE				4
#define	FL_CLIENT				8
#define	FL_INWATER				16
#define	FL_MONSTER				32
#define	FL_GODMODE				64
#define	FL_NOTARGET				128
#define	FL_ITEM					256
#define	FL_ONGROUND				512
#define	FL_PARTIALGROUND		1024	// not all corners are valid
#define	FL_WATERJUMP			2048	// player jumping out of water

#define FL_MOVECHAIN_ANGLE		32768    // when in a move chain, will update the angle
#define FL_CLASS_DEPENDENT		2097152

#define FF_CROUCHING			1	//fte flags. seperate from flags
#define FF_LADDER				2	//fte flags. seperate from flags

// entity effects

//define	EF_BRIGHTFIELD			1
//define	EF_MUZZLEFLASH 			2
#define	EF_BRIGHTLIGHT 			4
#define	EF_DIMLIGHT 			8

#define	EF_FULLBRIGHT			512


#define	SPAWNFLAG_NOT_EASY			256
#define	SPAWNFLAG_NOT_MEDIUM		512
#define	SPAWNFLAG_NOT_HARD			1024
#define	SPAWNFLAG_NOT_DEATHMATCH	2048

#define SPAWNFLAG_NOT_H2PALADIN			256
#define SPAWNFLAG_NOT_H2CLERIC			512
#define SPAWNFLAG_NOT_H2NECROMANCER		1024
#define SPAWNFLAG_NOT_H2THEIF			2048
#define	SPAWNFLAG_NOT_H2EASY			4096
#define	SPAWNFLAG_NOT_H2MEDIUM			8192
#define	SPAWNFLAG_NOT_H2HARD		    16384
#define	SPAWNFLAG_NOT_H2DEATHMATCH		32768
#define SPAWNFLAG_NOT_H2COOP			65536
#define SPAWNFLAG_NOT_H2SINGLE			131072

#if 0//ndef Q2SERVER
typedef enum multicast_e
{
	MULTICAST_ALL,
	MULTICAST_PHS,
	MULTICAST_PVS,
	MULTICAST_ALL_R,
	MULTICAST_PHS_R,
	MULTICAST_PVS_R
} multicast_t;
#endif

//============================================================================

extern	cvar_t	sv_mintic, sv_maxtic;
extern	cvar_t	sv_maxspeed;

extern	netadr_t	master_adr[MAX_MASTERS];	// address of the master server

extern	cvar_t	spawn;
extern	cvar_t	teamplay;
extern	cvar_t	deathmatch;
extern	cvar_t	fraglimit;
extern	cvar_t	timelimit;

extern	server_static_t	svs;				// persistant server info
extern	server_t		sv;					// local server

extern	client_t	*host_client;

extern	edict_t		*sv_player;

extern	char		localmodels[MAX_MODELS][5];	// inline model names for precache

extern	char		localinfo[MAX_LOCALINFO_STRING+1];

extern	int			host_hunklevel;
extern	FILE		*sv_logfile;
extern	FILE		*sv_fraglogfile;

//===========================================================

//
// sv_main.c
//
void VARGS SV_Error (char *error, ...);
void SV_Shutdown (void);
void SV_Frame (float time);
void SV_FinalMessage (char *message);
void SV_DropClient (client_t *drop);
struct quakeparms_s;
void SV_Init (struct quakeparms_s *parms);

int SV_CalcPing (client_t *cl);
void SV_FullClientUpdate (client_t *client, sizebuf_t *buf);
void SV_FullClientUpdateToClient (client_t *client, client_t *cl);
void SVNQ_FullClientUpdate (client_t *client, sizebuf_t *buf);

int SV_ModelIndex (char *name);

qboolean SV_CheckBottom (edict_t *ent);
qboolean SV_movestep (edict_t *ent, vec3_t move, qboolean relink, qboolean noenemy, struct globalvars_s *set_trace);

void SV_WriteClientdataToMessage (client_t *client, sizebuf_t *msg);
void SV_WriteDelta (entity_state_t *from, entity_state_t *to, sizebuf_t *msg, qboolean force, unsigned int protext);

void SV_MoveToGoal (progfuncs_t *prinst, struct globalvars_s *pr_globals);

void SV_SaveSpawnparms (qboolean);
void SV_SaveLevelCache(qboolean dontharmgame);
qboolean SV_LoadLevelCache(char *level, char *startspot, qboolean ignoreplayers);

void SV_Physics_Client (edict_t	*ent, int num);

void SV_ExecuteUserCommand (char *s, qboolean fromQC);
void SV_InitOperatorCommands (void);

void SV_SendServerinfo (client_t *client);
void SV_ExtractFromUserinfo (client_t *cl);

void SV_SaveInfos(FILE *f);


void Master_Heartbeat (void);
void Master_Packet (void);

void SV_FixupName(char *in, char *out);

//
// sv_init.c
//
void SV_SpawnServer (char *server, char *startspot, qboolean noents, qboolean usecinematic);
void SV_UnspawnServer (void);
void SV_FlushSignon (void);

void SV_FilterImpulseInit(void);

//svq2_game.c
qboolean SVQ2_InitGameProgs(void);
void SVQ2_ShutdownGameProgs (void);

//svq2_ents.c
void SV_BuildClientFrame (client_t *client);
void SV_WriteFrameToClient (client_t *client, sizebuf_t *msg);
#ifdef Q2SERVER
void MSGQ2_WriteDeltaEntity (q2entity_state_t *from, q2entity_state_t *to, sizebuf_t *msg, qboolean force, qboolean newentity);
#endif


//
// sv_phys.c
//
void SV_TouchLinks ( edict_t *ent, areanode_t *node );
void SV_ProgStartFrame (void);
qboolean SV_Physics (void);
void SV_CheckVelocity (edict_t *ent);
void SV_AddGravity (edict_t *ent, float scale);
qboolean SV_RunThink (edict_t *ent);
void SV_Physics_Toss (edict_t *ent);
void SV_RunNewmis (void);
void SV_Impact (edict_t *e1, edict_t *e2);
void SV_SetMoveVars(void);

//
// sv_send.c
//
void SV_QCStat(int type, char *name, int statnum);
void SV_ClearQCStats(void);

void SV_SendClientMessages (void);

void SV_Multicast (vec3_t origin, multicast_t to);
#define FULLDIMENSIONMASK 0xffffffff
void SV_MulticastProtExt(vec3_t origin, multicast_t to, int dimension_mask, int with, int without);

void SV_StartSound (edict_t *entity, int channel, char *sample, int volume,
    float attenuation);
void VARGS SV_ClientPrintf (client_t *cl, int level, char *fmt, ...);
void VARGS SV_ClientTPrintf (client_t *cl, int level, translation_t text, ...);
void VARGS SV_BroadcastPrintf (int level, char *fmt, ...);
void VARGS SV_BroadcastTPrintf (int level, translation_t fmt, ...);
void VARGS SV_BroadcastCommand (char *fmt, ...);
void SV_SendServerInfoChange(char *key, char *value);
void SV_SendMessagesToAll (void);
void SV_FindModelNumbers (void);

//
// sv_user.c
//
void SV_ExecuteClientMessage (client_t *cl);
void SVQ2_ExecuteClientMessage (client_t *cl);
int SV_PMTypeForClient (client_t *cl);
void SV_UserInit (void);
void SV_TogglePause (void);

void SV_ClientThink (void);

void VoteFlushAll(void);

//sv_master.c
void SVM_Think(int port);


//
// svonly.c
//
typedef enum {RD_NONE, RD_CLIENT, RD_PACKET, RD_OBLIVION} redirect_t;	//oblivion is provided so people can read the output before the buffer is wiped.
void SV_BeginRedirect (redirect_t rd, int lang);
void SV_EndRedirect (void);

//
// sv_ccmds.c
//
void SV_Status_f (void);

//
// sv_ents.c
//
void SV_WriteEntitiesToClient (client_t *client, sizebuf_t *msg, qboolean ignorepvs);
int SV_HullNumForPlayer(int h2hull, float *mins, float *maxs);
void SV_GibFilterInit(void);
void SV_CleanupEnts(void);

//
// sv_nchan.c
//

void ClientReliableCheckBlock(client_t *cl, int maxsize);
void ClientReliable_FinishWrite(client_t *cl);
void ClientReliableWrite_Begin(client_t *cl, int c, int maxsize);
void ClientReliableWrite_Angle(client_t *cl, float f);
void ClientReliableWrite_Angle16(client_t *cl, float f);
void ClientReliableWrite_Byte(client_t *cl, int c);
void ClientReliableWrite_Char(client_t *cl, int c);
void ClientReliableWrite_Float(client_t *cl, float f);
void ClientReliableWrite_Coord(client_t *cl, float f);
void ClientReliableWrite_Long(client_t *cl, int c);
void ClientReliableWrite_Short(client_t *cl, int c);
void ClientReliableWrite_String(client_t *cl, char *s);
void ClientReliableWrite_SZ(client_t *cl, void *data, int len);


#ifdef  SVRANKING
//flags
#define RANK_BANNED		1
#define RANK_MUTED		2
#define RANK_CUFFED		4
#define RANK_CRIPPLED	8	//ha ha... get speed cheaters with this!... :o)

typedef struct {	//stats info	
	int kills;
	int deaths;
	float parm[NUM_SPAWN_PARMS];
	float timeonserver;
	qbyte flags1;
	qbyte trustlevel;
	char pad2;
	char pad3;
} rankstats_t;

typedef struct {	//name, identity and order.
	int prev;		//score is held for convineance.
	int next;
	char name[32];
	int pwd;
	float score;
} rankheader_t;

typedef struct {
	rankheader_t h;
	rankstats_t s;	
} rankinfo_t;

int Rank_GetPlayerID(char *name, int pwd, qboolean allowcreate);
void Rank_SetPlayerStats(int id, rankstats_t *stats);
rankstats_t *Rank_GetPlayerStats(int id, rankstats_t *buffer);
rankinfo_t *Rank_GetPlayerInfo(int id, rankinfo_t *buffer);
qboolean Rank_OpenRankings(void);
void Rank_Flush (void);

void Rank_ListTop10_f (void);
void Rank_RegisterCommands(void);
int Rank_GetPass (char *name);

extern cvar_t rank_needlogin;


client_t *SV_GetClientForString(char *name, int *id);


qboolean ReloadRanking(client_t *cl, char *newname);
#endif






void NPP_Flush(void);
void NPP_NQWriteByte(int dest, qbyte data);
void NPP_NQWriteChar(int dest, char data);
void NPP_NQWriteShort(int dest, short data);
void NPP_NQWriteLong(int dest, long data);
void NPP_NQWriteAngle(int dest, float data);
void NPP_NQWriteCoord(int dest, float data);
void NPP_NQWriteString(int dest, char *data);
void NPP_NQWriteEntity(int dest, short data);

void NPP_QWWriteByte(int dest, qbyte data);
void NPP_QWWriteChar(int dest, char data);
void NPP_QWWriteShort(int dest, short data);
void NPP_QWWriteLong(int dest, long data);
void NPP_QWWriteAngle(int dest, float data);
void NPP_QWWriteCoord(int dest, float data);
void NPP_QWWriteString(int dest, char *data);
void NPP_QWWriteEntity(int dest, short data);



void NPP_MVDForceFlush(void);


//replacement rand function (useful cos it's fully portable, with seed grabbing)
void predictablesrand(unsigned int x);
int predictablerandgetseed(void);
int predictablerand(void);







#ifdef SVCHAT
void SV_WipeChat(client_t *client);
int SV_ChatMove(int impulse);
void SV_ChatThink(client_t *client);
#endif


void SV_ConSay_f(void);





//
// sv_mvd.c
//
void SV_MVDPings (void);
void SV_MVDWriteToDisk(int type, int to, float time);
void MVDWrite_Begin(qbyte type, int to, int size);
void MVDSetMsgBuf(demobuf_t *prev,demobuf_t *cur);
void SV_MVDStop (int reason);
void SV_MVDStop_f (void);
void SV_MVDWritePackets (int num);
void MVD_Init (void);

extern demo_t			demo;				// server demo struct

extern cvar_t	sv_demofps;
extern cvar_t	sv_demoPings;
extern cvar_t	sv_demoNoVis;
extern cvar_t	sv_demoUseCache;
extern cvar_t	sv_demoMaxSize;
extern cvar_t	sv_demoMaxDirSize;

void SV_MVDInit(void);
void SV_SendMVDMessage(void);
qboolean SV_ReadMVD (void);
void SV_FlushDemoSignon (void);

// savegame.c
void SV_FlushLevelCache(void);

int SV_RateForClient(client_t *cl);


void SVVC_Frame (qboolean enabled);
void SV_CalcPHS (void);
#ifdef Q2SERVER
void SVQ2_LinkEdict(q2edict_t *ent);
#endif

void SV_GetConsoleCommands (void);
void SV_CheckTimer(void);

typedef struct
{
	int sec;
	int min;
	int hour;
	int day;
	int mon;
	int year;
	char str[128];
} date_t;
void SV_TimeOfDay(date_t *date);
