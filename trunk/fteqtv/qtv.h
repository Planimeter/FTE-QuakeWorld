/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the included (GNU.txt) GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifdef __GNUC__
#define LittleLong(x) ({ typeof(x) _x = (x); _x = (((unsigned char *)&_x)[0]|(((unsigned char *)&_x)[1]<<8)|(((unsigned char *)&_x)[2]<<16)|(((unsigned char *)&_x)[3]<<24)); _x; })
#define LittleShort(x) ({ typeof(x) _x = (x); _x = (((unsigned char *)&_x)[0]|(((unsigned char *)&_x)[1]<<8)); _x; })
#else
#define LittleLong(x) (x)
#define LittleShort(x) (x)
#endif

//this is for a future version
//#define COMMENTARY

//each server that we are connected to has it's own state.
//it should be easy enough to use one thread per server.

//mvd info is forwarded to other proxies instantly
//qwd stuff is buffered and delayed. :(

//this means that when a new proxy connects, we have to send initial state as well as a chunk of pending state, expect to need to send new data before the proxy even has all the init stuff. We may need to raise MAX_PROXY_BUFFER to be larger than on the server



//how does multiple servers work
//each proxy acts as a cluster of connections to servers
//when a viewer connects, they are given a list of active server connections
//if there's only one server connection, they are given that one automatically.

#ifdef _WIN32
	#include <conio.h>
	#include <winsock.h>	//this includes windows.h and is the reason for much compiling slowness with windows builds.
	#ifdef _MSC_VER
		#pragma comment (lib, "wsock32.lib")
	#endif
	#define qerrno WSAGetLastError()
	#define EWOULDBLOCK WSAEWOULDBLOCK
	#define EINPROGRESS WSAEINPROGRESS
	#define ECONNREFUSED WSAECONNREFUSED
	#define ENOTCONN WSAENOTCONN

	//we have special functions to properly terminate sprintf buffers in windows.
	//we assume other systems are designed with even a minor thought to security.
	#if !defined(__MINGW32_VERSION)
		#define unlink _unlink	//why do MS have to be so awkward?
		int snprintf(char *buffer, int buffersize, char *format, ...);
		#if !defined(_VC80_UPGRADE)
			int vsnprintf(char *buffer, int buffersize, char *format, va_list argptr);
		#endif
	#else
		#define unlink remove	//seems mingw misses something
	#endif

	#ifdef _MSC_VER
		//okay, so warnings are here to help... they're ugly though.
		#pragma warning(disable: 4761)	//integral size mismatch in argument
		#pragma warning(disable: 4244)	//conversion from float to short
		#pragma warning(disable: 4018)	//signed/unsigned mismatch
	#endif

#elif defined(__CYGWIN__)

	#include <sys/time.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/errno.h>
	#include <arpa/inet.h>
	#include <stdarg.h>
	#include <netdb.h>
	#include <stdlib.h>
	#include <sys/ioctl.h>
	#include <unistd.h>

	#define ioctlsocket ioctl
	#define closesocket close

#elif defined(linux) || defined(ixemul)
	#include <sys/time.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <stdarg.h>
	#include <stdlib.h>
	#include <string.h>
	#include <errno.h>
	#include <sys/ioctl.h>
	#include <unistd.h>

	#define ioctlsocket ioctl
	#define closesocket close
#else
#error "Please insert required headers here"
//try the cygwin ones
#endif

#ifndef pgetaddrinfo
	#ifndef _WIN32
		#define pgetaddrinfo getaddrinfo
		#define pfreeaddrinfo freeaddrinfo
	#endif
#endif
#ifndef SOCKET
	#define SOCKET int
#endif
#ifndef INVALID_SOCKET
	#define INVALID_SOCKET -1
#endif
#ifndef qerrno
	#define qerrno errno
#endif


#include <stdio.h>
#include <string.h>

#ifndef _WIN32
//stricmp is ansi, strcasecmp is unix.
	#define stricmp strcasecmp
	#define strnicmp strncasecmp
#endif


//linux and other systems have strlcat / strlcpy
//we support windows and can't use those
#define Q_strncatz(dest, src, sizeofdest)	\
	do {	\
		strncat(dest, src, sizeofdest - strlen(dest) - 1);	\
		dest[sizeofdest - 1] = 0;	\
	} while (0)
#define Q_strncpyz(dest, src, sizeofdest)	\
	do {	\
		strncpy(dest, src, sizeofdest - 1);	\
		dest[sizeofdest - 1] = 0;	\
	} while (0)





#define VERSION "0.01"	//this will be added to the serverinfo

#define PROX_DEFAULTLISTENPORT 27501
#define PROX_DEFAULTSERVER "localhost:27500"

#define	MAX_SERVERINFO_STRING	1024	//standard quake has 512 here.
#define MAX_USERINFO 192
#define MAX_CLIENTS 32
#define MAX_LIST 256
#define MAX_MODELS MAX_LIST
#define MAX_SOUNDS MAX_LIST
#define MAX_ENTITIES 512
#define MAX_STATICSOUNDS 64
#define MAX_STATICENTITIES 128
#define MAX_LIGHTSTYLES 64
#define DEFAULT_HOSTNAME "FTEQTV"

#define MAX_PROXY_INBUFFER 4096
#define MAX_PROXY_BUFFER (1<<14)	//must be power-of-two
#define PREFERED_PROXY_BUFFER	4096 //the ammount of data we try to leave in our input buffer (must be large enough to contain any single mvd frame)

#define MAX_ENTITY_LEAFS 32
#define ENTS_PER_FRAME 64 //max number of entities per frame (OUCH!).
#define ENTITY_FRAMES 64 //number of frames to remember for deltaing


#define Z_EXT_SERVERTIME	(1<<3)	// STAT_TIME
#define Z_EXT_STRING "8"


#define MAX_STATS 32
#define	STAT_HEALTH			0
#define	STAT_FRAGS			1
#define	STAT_WEAPON			2
#define	STAT_AMMO			3
#define	STAT_ARMOR			4
#define	STAT_WEAPONFRAME	5
#define	STAT_SHELLS			6
#define	STAT_NAILS			7
#define	STAT_ROCKETS		8
#define	STAT_CELLS			9
#define	STAT_ACTIVEWEAPON	10
#define	STAT_TOTALSECRETS	11
#define	STAT_TOTALMONSTERS	12
#define	STAT_SECRETS		13		// bumped on client side by svc_foundsecret
#define	STAT_MONSTERS		14		// bumped by svc_killedmonster
#define STAT_ITEMS			15

#define STAT_TIME 17	//A ZQ hack, sending time via a stat.
						//this allows l33t engines to interpolate properly without spamming at a silly high fps.




#ifndef __cplusplus
typedef enum {false, true} qboolean;
#else
typedef int qboolean;
extern "C" {
#endif

typedef unsigned char netadr_t[64];

#define NQ_PACKETS_PER_SECOND 20
#define MAX_NQMSGLEN 8000
#define MAX_MSGLEN 1400
#define MAX_NQDATAGRAM 1024
#define MAX_BACKBUF_SIZE 1000	//this is smaller so we don't loose entities when lagging


//NQ transport layer defines
#define NETFLAG_LENGTH_MASK	0x0000ffff
#define NETFLAG_DATA		0x00010000
#define NETFLAG_ACK			0x00020000
#define NETFLAG_NAK			0x00040000
#define NETFLAG_EOM			0x00080000
#define NETFLAG_UNRELIABLE	0x00100000
#define NETFLAG_CTL			0x80000000

#define CCREQ_CONNECT		0x01
#define CCREQ_SERVER_INFO	0x02

#define CCREP_ACCEPT		0x81
#define CCREP_REJECT		0x82
#define CCREP_SERVER_INFO	0x83

#define NET_GAMENAME_NQ		"QUAKE"
#define NET_PROTOCOL_VERSION	3

#ifdef COMMENTARY
typedef struct soundcapt_s {
	int (*update)(struct soundcapt_s *ghnd, int samplechunks, char *buffer);
	void (*close)(struct soundcapt_s *ptr);
} soundcapt_t;
#endif

typedef struct {
	unsigned int readpos;
	unsigned int cursize;
	unsigned int maxsize;
	char *data;
	unsigned int startpos;
	qboolean overflowed;
	qboolean allowoverflow;
} netmsg_t;

typedef struct {
	SOCKET sock;
	netadr_t remote_address;
	unsigned short qport;
	unsigned int last_received;
	unsigned int cleartime;

	int reliable_length;
	qboolean drop;
	qboolean isclient;
	qboolean isnqprotocol;

	netmsg_t message;
	char message_buf[MAX_NQMSGLEN];	//reliable message being built
	char reliable_buf[MAX_NQMSGLEN];	//reliable message that we're making sure arrives.
	float rate;


	unsigned int incoming_acknowledged;
	unsigned int last_reliable_sequence;
	unsigned int incoming_reliable_acknowledged;
	unsigned int incoming_reliable_sequence;
	unsigned int reliable_sequence;

	unsigned int incoming_sequence;
	unsigned int outgoing_sequence;



	unsigned int reliable_start;
	unsigned int outgoing_unreliable;
	unsigned int incoming_unreliable;
	unsigned int in_fragment_length;

	char in_fragment_buf[MAX_NQMSGLEN];
} netchan_t;

typedef struct {
#define MAX_QPATH 64
	char name[MAX_QPATH];
} filename_t;

typedef struct {
	unsigned char frame;
	unsigned char modelindex;
	unsigned char colormap;
	unsigned char skinnum;
	short origin[3];
	char angles[3];
	unsigned char effects;
} entity_state_t;
typedef struct {
	unsigned char frame;
	unsigned char modelindex;
	unsigned char skinnum;
	short origin[3];
	short velocity[3];
	short angles[3];
	unsigned char effects;
	unsigned char weaponframe;
} player_state_t;
typedef struct {
	unsigned int stats[MAX_STATS];
	char userinfo[MAX_USERINFO];

	int ping;
	int packetloss;
	int frags;
	float entertime;

	int leafcount;
	unsigned short leafs[MAX_ENTITY_LEAFS];

	qboolean active:1;
	qboolean gibbed:1;
	qboolean dead:1;
	player_state_t current;
	player_state_t old;
} playerinfo_t;

typedef struct {
	entity_state_t ents[ENTS_PER_FRAME];	//ouchie ouchie!
	unsigned short entnum[ENTS_PER_FRAME];
	int numents;
} packet_entities_t;

typedef struct {
	unsigned char msec;
	unsigned short angles[3];
	short forwardmove, sidemove, upmove;
	unsigned char buttons;
	unsigned char impulse;
} usercmd_t;
extern const usercmd_t nullcmd;


typedef float vec3_t[3];
typedef struct {
	float gravity;
	float maxspeed;
	float spectatormaxspeed;
	float accelerate;
	float airaccelerate;
	float waterfriction;
	float entgrav;
	float stopspeed;
	float wateraccelerate;
	float friction;
} movevars_t;
typedef struct {
	//in / out
	vec3_t origin;
	vec3_t velocity;

	//in
	usercmd_t cmd;
	movevars_t movevars;

	//internal
	vec3_t angles;
	float frametime;
	vec3_t forward, right, up;
} pmove_t;


#define MAX_BACK_BUFFERS	16
typedef struct sv_s sv_t;
typedef struct cluster_s cluster_t;
typedef struct viewer_s {
	qboolean drop;
	unsigned int timeout;
	unsigned int nextpacket;	//for nq clients
	netchan_t netchan;
	qboolean maysend;
	qboolean chokeme;
	qboolean thinksitsconnected;
	qboolean conmenussupported;
	int delta_frame;

	int servercount;

	netmsg_t backbuf[MAX_BACK_BUFFERS];	//note data is malloced!
	int backbuffered;

	unsigned int currentstats[MAX_STATS];
	int trackplayer;
	int thisplayer;

	packet_entities_t frame[ENTITY_FRAMES];

	struct viewer_s *next;

	char name[32];
	char userinfo[1024];

	int lost;	//packets
	usercmd_t ucmds[3];


	int settime;	//the time that we last told the client.

	vec3_t velocity;
	vec3_t origin;

	int isadmin;
	char expectcommand[16];

	sv_t *server;

	int menunum;
	int menuop;
	int fwdval;	//for scrolling up/down the menu using +forward/+back :)
} viewer_t;

//'other proxy', these are mvd stream clients.
typedef struct oproxy_s {
	int authkey;
	unsigned int droptime;

	qboolean flushing;
	qboolean drop;

	sv_t *defaultstream;

	FILE *file;		//recording a demo
	SOCKET sock;	//playing to a proxy

	unsigned char inbuffer[MAX_PROXY_INBUFFER];
	unsigned int inbuffersize;	//amount of data available.

	unsigned char buffer[MAX_PROXY_BUFFER];
	unsigned int buffersize;	//use cyclic buffering.
	unsigned int bufferpos;
	struct oproxy_s *next;
} oproxy_t;

typedef struct {
	short origin[3];
	unsigned char soundindex;
	unsigned char volume;
	unsigned char attenuation;
} staticsound_t;

typedef struct bsp_s bsp_t;

typedef struct {
	entity_state_t baseline;
	entity_state_t current;
	entity_state_t old;
	unsigned int updatetime;	//to stop lerping when it's an old entity (bodies, stationary grenades, ...)

	int leafcount;
	unsigned short leafs[MAX_ENTITY_LEAFS];
} entity_t;

#define MAX_ENTITY_FRAMES 64
typedef struct {
	int numents;
	int maxents;
	entity_state_t *ents;	//dynamically allocated
} frame_t;

typedef struct {
	unsigned char number;
	char bits[6];
} nail_t;

struct sv_s {
	char connectpassword[64];	//password given to server
	netadr_t serveraddress;
	netchan_t netchan;

	unsigned char buffer[MAX_PROXY_BUFFER];	//this doesn't cycle.
	int buffersize;	//it memmoves down
	int forwardpoint;	//the point in the stream that we're forwarded up to.
	qboolean parsingqtvheader;

	unsigned char upstreambuffer[2048];
	int upstreambuffersize;

	unsigned int parsetime;

	char gamedir[MAX_QPATH];
	char mapname[256];
	movevars_t movevars;
	int cdtrack;
	entity_t entity[MAX_ENTITIES];
	frame_t frame[MAX_ENTITY_FRAMES];
	int maxents;
	staticsound_t staticsound[MAX_STATICSOUNDS];
	int staticsound_count;
	entity_state_t spawnstatic[MAX_STATICENTITIES];
	int spawnstatic_count;
	filename_t lightstyle[MAX_LIGHTSTYLES];

	char serverinfo[MAX_SERVERINFO_STRING];
	char hostname[MAX_QPATH];
	playerinfo_t players[MAX_CLIENTS];

	filename_t modellist[MAX_MODELS];
	filename_t soundlist[MAX_SOUNDS];
	int modelindex_spike;	// qw is wierd.
	int modelindex_player;	// qw is wierd.

	FILE *downloadfile;
	char downloadname[256];

	char status[64];

	nail_t nails[32];
	int nailcount;

	qboolean usequkeworldprotocols;
	int challenge;
	unsigned short qport;
	int isconnected;
	int clservercount;
	unsigned int nextsendpings;
	int trackplayer;
	int thisplayer;
	unsigned int timeout;
	qboolean ispaused;
	unsigned int packetratelimiter;
	viewer_t *controller;

	qboolean proxyplayer;	//a player is actually playing on the proxy.
	usercmd_t proxyplayerucmds[3];
	int proxyplayerucmdnum;
	int proxyplayerbuttons;
	float proxyplayerangles[3];
	float proxyplayerimpulse;

	qboolean maysend;

	FILE *file;
	unsigned int filelength;
	SOCKET sourcesock;

	SOCKET tcpsocket;	//tcp + mvd protocol
	int tcplistenportnum;

	oproxy_t *proxies;

	qboolean parsingconnectiondata;	//so reject any new connects for now

	unsigned int physicstime;	//the last time all the ents moved.
	unsigned int simtime;
	unsigned int curtime;
	unsigned int oldpackettime;
	unsigned int nextpackettime;
	unsigned int nextconnectattempt;



	qboolean drop;
	qboolean disconnectwhennooneiswatching;
	unsigned int numviewers;

	cluster_t *cluster;
	sv_t *next;	//next proxy->server connection

	bsp_t *bsp;
	int numinlines;

#ifdef COMMENTARY
	//audio stuff
	soundcapt_t *comentrycapture;
#endif

	//options:
	char server[MAX_QPATH];
	int streamid;
};

struct cluster_s {
	SOCKET qwdsocket;	//udp + quakeworld protocols
	SOCKET tcpsocket;	//tcp listening socket (for mvd and listings and stuff)

	char commandinput[512];
	int inputlength;

	unsigned int mastersendtime;
	unsigned int mastersequence;
	unsigned int curtime;

	viewer_t *viewers;
	int numviewers;
	sv_t *servers;
	int numservers;
	int nextstreamid;

	//options
	int qwlistenportnum;
	int tcplistenportnum;
	char adminpassword[256];//password required for rcon etc
	char qtvpassword[256];	//password required to connect a proxy
	char hostname[256];
	char master[MAX_QPATH];
	qboolean chokeonnotupdated;
	qboolean lateforward;
	qboolean notalking;
	qboolean nobsp;
	qboolean allownqclients;	//nq clients require no challenge
	qboolean nouserconnects;	//prohibit users from connecting to new streams.

	int maxviewers;

	int buildnumber;

	int numproxies;
	int maxproxies;

	qboolean wanttoexit;

	oproxy_t *pendingproxies;
};

#define MENU_NONE 0
#define MENU_SERVERS 1
#define MENU_ADMIN 2
#define MENU_ADMINSERVER 3





unsigned char ReadByte(netmsg_t *b);
unsigned short ReadShort(netmsg_t *b);
unsigned int ReadLong(netmsg_t *b);
float ReadFloat(netmsg_t *b);
void ReadString(netmsg_t *b, char *string, int maxlen);

unsigned int SwapLong(unsigned int val);
unsigned int BigLong(unsigned int val);





#define	clc_bad			0
#define	clc_nop 		1
#define	clc_disconnect	2		//NQ only
#define	clc_move		3		// [[usercmd_t]
#define	clc_stringcmd	4		// [string] message
#define	clc_delta		5		// [byte] sequence number, requests delta compression of message
#define clc_tmove		6		// teleport request, spectator only
#define clc_upload		7		// teleport request, spectator only






#define	svc_bad				0
#define	svc_nop				1
#define	svc_disconnect		2
#define	svc_updatestat		3	// [qbyte] [qbyte]
//#define	svc_version			4	// [long] server version
#define	svc_nqsetview			5	// [short] entity number
#define	svc_sound			6	// <see code>
#define	svc_nqtime			7	// [float] server time
#define	svc_print			8	// [qbyte] id [string] null terminated string
#define	svc_stufftext		9	// [string] stuffed into client's console buffer
								// the string should be \n terminated
#define	svc_setangle		10	// [angle3] set the view angle to this absolute value

#define	svc_serverdata		11	// [long] protocol ...
#define	svc_lightstyle		12	// [qbyte] [string]
#define	svc_nqupdatename		13	// [qbyte] [string]
#define	svc_updatefrags		14	// [qbyte] [short]
#define	svc_nqclientdata		15	// <shortbits + data>
//#define	svc_stopsound		16	// <see code>
#define	svc_nqupdatecolors	17	// [qbyte] [qbyte] [qbyte]
#define	svc_particle		18	// [vec3] <variable>
#define	svc_damage			19

#define	svc_spawnstatic		20
//#define	svc_spawnstatic2	21
#define	svc_spawnbaseline	22

#define	svc_temp_entity		23	// variable
#define	svc_setpause		24	// [qbyte] on / off
#define	svc_nqsignonnum		25	// [qbyte]  used for the signon sequence

#define	svc_centerprint		26	// [string] to put in center of the screen

//#define	svc_killedmonster	27
//#define	svc_foundsecret		28

#define	svc_spawnstaticsound	29	// [coord3] [qbyte] samp [qbyte] vol [qbyte] aten

#define	svc_intermission	30		// [vec3_t] origin [vec3_t] angle
//#define	svc_finale			31		// [string] text

#define	svc_cdtrack			32		// [qbyte] track
//#define svc_sellscreen		33

//#define svc_cutscene		34	//hmm... nq only... added after qw tree splitt?

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
#define svc_updatepl		53		// [qbyte] [qbyte]
#define svc_nails2			54		//qwe - [qbyte] num [52 bits] nxyzpy 8 12 12 12 4 8





#define dem_audio		0

#define dem_cmd			0
#define dem_read		1
#define dem_set			2
#define dem_multiple	3
#define dem_single		4
#define dem_stats		5
#define dem_all			6

#define dem_mask		7


#define PROTOCOL_VERSION_NQ	15
#define	PROTOCOL_VERSION	28


//flags on entities
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





//flags on players
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



//flags for where a message can be sent, for easy broadcasting
#define Q1 (NQ|QW)
#define QW 1
#define NQ 2
#define CONNECTING 4



void InitNetMsg(netmsg_t *b, char *buffer, int bufferlength);
unsigned char ReadByte(netmsg_t *b);
unsigned short ReadShort(netmsg_t *b);
unsigned int ReadLong(netmsg_t *b);
float ReadFloat(netmsg_t *b);
void ReadString(netmsg_t *b, char *string, int maxlen);
void WriteByte(netmsg_t *b, unsigned char c);
void WriteShort(netmsg_t *b, unsigned short l);
void WriteLong(netmsg_t *b, unsigned int l);
void WriteFloat(netmsg_t *b, float f);
void WriteString2(netmsg_t *b, const char *str);
void WriteString(netmsg_t *b, const char *str);
void WriteData(netmsg_t *b, const char *data, int length);

void Multicast(sv_t *tv, char *buffer, int length, int to, unsigned int playermask,int suitablefor);
void Broadcast(cluster_t *cluster, char *buffer, int length, int suitablefor);
void ParseMessage(sv_t *tv, char *buffer, int length, int to, int mask);
void BuildServerData(sv_t *tv, netmsg_t *msg, qboolean mvd, int servercount, qboolean spectatorflag);
void BuildNQServerData(sv_t *tv, netmsg_t *msg, qboolean mvd, int servercount);
SOCKET QW_InitUDPSocket(int port);
void QW_UpdateUDPStuff(cluster_t *qtv);
unsigned int Sys_Milliseconds(void);
void Prox_SendInitialEnts(sv_t *qtv, oproxy_t *prox, netmsg_t *msg);
qboolean QTV_Connect(sv_t *qtv, char *serverurl);
void QTV_Shutdown(sv_t *qtv);
qboolean	NET_StringToAddr (char *s, netadr_t *sadr, int defaultport);

void SendBufferToViewer(viewer_t *v, const char *buffer, int length, qboolean reliable);
void QW_PrintfToViewer(viewer_t *v, char *format, ...);
void QW_StuffcmdToViewer(viewer_t *v, char *format, ...);

void PM_PlayerMove (pmove_t *pmove);

void Netchan_Setup (SOCKET sock, netchan_t *chan, netadr_t adr, int qport, qboolean isclient);
void Netchan_OutOfBandPrint (cluster_t *cluster, SOCKET sock, netadr_t adr, char *format, ...);
int Netchan_IsLocal (netadr_t adr);
void NET_SendPacket(cluster_t *cluster, SOCKET sock, int length, char *data, netadr_t adr);
qboolean Net_CompareAddress(netadr_t *s1, netadr_t *s2, int qp1, int qp2);
qboolean Netchan_Process (netchan_t *chan, netmsg_t *msg);
qboolean NQNetchan_Process(cluster_t *cluster, netchan_t *chan, netmsg_t *msg);
void Netchan_Transmit (cluster_t *cluster, netchan_t *chan, int length, const unsigned char *data);
int SendList(sv_t *qtv, int first, const filename_t *list, int svc, netmsg_t *msg);
int Prespawn(sv_t *qtv, int curmsgsize, netmsg_t *msg, int bufnum, int thisplayer);

bsp_t *BSP_LoadModel(cluster_t *cluster, char *gamedir, char *bspname);
void BSP_Free(bsp_t *bsp);
int BSP_LeafNum(bsp_t *bsp, float x, float y, float z);
unsigned int BSP_Checksum(bsp_t *bsp);
int BSP_SphereLeafNums(bsp_t *bsp, int maxleafs, unsigned short *list, float x, float y, float z, float radius);
qboolean BSP_Visible(bsp_t *bsp, int leafcount, unsigned short *list);
void BSP_SetupForPosition(bsp_t *bsp, float x, float y, float z);
void QW_SetViewersServer(viewer_t *viewer, sv_t *sv);
unsigned short QCRC_Block (unsigned char *start, int count);
unsigned short QCRC_Value(unsigned short crcvalue);
void Netchan_OutOfBand (cluster_t *cluster, SOCKET sock, netadr_t adr, int length, unsigned char *data);
void WriteDeltaUsercmd (netmsg_t *m, const usercmd_t *from, usercmd_t *move);
void SendClientCommand(sv_t *qtv, char *fmt, ...);
void QTV_Run(sv_t *qtv);
void QW_FreeViewer(cluster_t *cluster, viewer_t *viewer);
void QW_SetMenu(viewer_t *v, int menunum);

char *Rcon_Command(cluster_t *cluster, sv_t *qtv, char *command, char *buffer, int sizeofbuffer, qboolean localcommand);
char *COM_ParseToken (char *data, char *out, int outsize, const char *punctuation);
char *Info_ValueForKey (char *s, const char *key, char *buffer, int buffersize);
void Info_SetValueForStarKey (char *s, const char *key, const char *value, int maxsize);
void ReadDeltaUsercmd (netmsg_t *m, const usercmd_t *from, usercmd_t *move);
unsigned Com_BlockChecksum (void *buffer, int length);
void Com_BlockFullChecksum (void *buffer, int len, unsigned char *outbuf);

void Sys_Printf(cluster_t *cluster, char *fmt, ...);

oproxy_t *Net_FileProxy(sv_t *qtv, char *filename);
sv_t *QTV_NewServerConnection(cluster_t *cluster, char *server, char *password, qboolean force, qboolean autoclose, qboolean noduplicates);
SOCKET Net_MVDListen(int port);
qboolean Net_StopFileProxy(sv_t *qtv);


void SV_FindProxies(SOCKET sock, cluster_t *cluster, sv_t *defaultqtv);
qboolean SV_ReadPendingProxy(cluster_t *cluster, oproxy_t *pend);
void SV_ForwardStream(sv_t *qtv, char *buffer, int length);

unsigned char *FS_ReadFile(char *gamedir, char *filename, unsigned int *size);

void ChooseFavoriteTrack(sv_t *tv);

void DemoViewer_Init(void);
void DemoViewer_Update(sv_t *svtest);
void DemoViewer_Shutdown(void);


#ifdef __cplusplus
}
#endif
