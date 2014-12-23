/*
serverside master heartbeat code
clientside master queries and server ping/polls
*/

#include "quakedef.h"

#include "cl_master.h"

qboolean	sb_enablequake2;
qboolean	sb_enablequake3;
qboolean	sb_enablenetquake;
qboolean	sb_enabledarkplaces;
qboolean	sb_enablequakeworld;

void Master_DetermineMasterTypes(void)
{
	if (com_protocolname.modified)
	{
		char *prot = com_protocolname.string;
		com_protocolname.modified = 0;

		sb_enabledarkplaces = true;	//dpmaster is not specific to any single game/mod, so can be left enabled even when running q2 etc, for extra redundancy.
		sb_enablequake2 = false;
		sb_enablequake3 = false;
		sb_enablenetquake = false;
		sb_enablequakeworld = false;

		//this is stupid
		if (!Q_strncasecmp(prot, "FTE-", 4))
			prot += 4;
		else if (!Q_strncasecmp(prot, "DarkPlaces-", 11))
			prot += 11;

		if (!strcmp(prot, "Quake2"))
			sb_enablequake2 = true;
		if (!strcmp(prot, "Quake3"))
			sb_enablequake3 = true;
		//for DP compatibility, we consider these separate(ish) games.
		if (!strcmp(prot, "Quake") || !strcmp(com_protocolname.string, "Hipnotic") || !strcmp(com_protocolname.string, "Rogue"))
			sb_enablenetquake = sb_enablequakeworld = true;
	}
}

#define MAX_MASTER_ADDRESSES 4	//each master might have multiple dns addresses, typically both ipv4+ipv6. we want to report to both address families so we work with remote single-stack hosts.

#ifndef CLIENTONLY
void Net_Masterlist_Callback(struct cvar_s *var, char *oldvalue);
static void SV_SetMaster_f (void);
#else
#define Net_Masterlist_Callback NULL
#endif

extern cvar_t sv_public;
extern cvar_t sv_reportheartbeats;

extern cvar_t sv_listen_qw;
extern cvar_t sv_listen_nq;
extern cvar_t sv_listen_dp;
extern cvar_t sv_listen_q3;

typedef struct {
	enum masterprotocol_e protocol;
	cvar_t		cv;
	char		*comment;

#ifndef CLIENTONLY
	qboolean	needsresolve;	//set any time the cvar is modified
	qboolean	resolving;	//set any time the cvar is modified
	netadr_t	adr[MAX_MASTER_ADDRESSES];
#endif
} net_masterlist_t;
net_masterlist_t net_masterlist[] = {
	//user-specified master lists.
	{MP_QUAKEWORLD, CVARC("net_qwmaster1", "", Net_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("net_qwmaster2", "", Net_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("net_qwmaster3", "", Net_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("net_qwmaster4", "", Net_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("net_qwmaster5", "", Net_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("net_qwmaster6", "", Net_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("net_qwmaster7", "", Net_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("net_qwmaster8", "", Net_Masterlist_Callback)},

	//dpmaster is the generic non-quake-specific master protocol that we use for custom stand-alone mods.
	{MP_DPMASTER,	CVARAFC("net_master1", "", "sv_master1", 0, Net_Masterlist_Callback)},
	{MP_DPMASTER,	CVARAFC("net_master2", "", "sv_master2", 0, Net_Masterlist_Callback)},
	{MP_DPMASTER,	CVARAFC("net_master3", "", "sv_master3", 0, Net_Masterlist_Callback)},
	{MP_DPMASTER,	CVARAFC("net_master4", "", "sv_master4", 0, Net_Masterlist_Callback)},
	{MP_DPMASTER,	CVARAFC("net_master5", "", "sv_master5", 0, Net_Masterlist_Callback)},
	{MP_DPMASTER,	CVARAFC("net_master6", "", "sv_master6", 0, Net_Masterlist_Callback)},
	{MP_DPMASTER,	CVARAFC("net_master7", "", "sv_master7", 0, Net_Masterlist_Callback)},
	{MP_DPMASTER,	CVARAFC("net_master8", "", "sv_master8", 0, Net_Masterlist_Callback)},

	{MP_QUAKE2,		CVARC("net_q2master1", "", Net_Masterlist_Callback)},
	{MP_QUAKE2,		CVARC("net_q2master2", "", Net_Masterlist_Callback)},
	{MP_QUAKE2,		CVARC("net_q2master3", "", Net_Masterlist_Callback)},
	{MP_QUAKE2,		CVARC("net_q2master4", "", Net_Masterlist_Callback)},

	{MP_QUAKE3,		CVARC("net_q3master1", "", Net_Masterlist_Callback)},
	{MP_QUAKE3,		CVARC("net_q3master2", "", Net_Masterlist_Callback)},
	{MP_QUAKE3,		CVARC("net_q3master3", "", Net_Masterlist_Callback)},
	{MP_QUAKE3,		CVARC("net_q3master4", "", Net_Masterlist_Callback)},

	//engine-specified/maintained master lists (so users can be lazy and update the engine without having to rewrite all their configs).
	{MP_QUAKEWORLD, CVARFC("net_qwmasterextra1", "qwmaster.ocrana.de:27000",						CVAR_NOSAVE, Net_Masterlist_Callback),	"Ocrana(2nd)"},	//german. admin unknown
	{MP_QUAKEWORLD, CVARFC("net_qwmasterextra2", ""/*"masterserver.exhale.de:27000" seems dead*/,	CVAR_NOSAVE, Net_Masterlist_Callback)},	//german. admin unknown
	{MP_QUAKEWORLD, CVARFC("net_qwmasterextra3", "asgaard.morphos-team.net:27000",					CVAR_NOSAVE, Net_Masterlist_Callback),	"Germany, admin: bigfoot"},
	{MP_QUAKEWORLD, CVARFC("net_qwmasterextra4", "master.quakeservers.net:27000",					CVAR_NOSAVE, Net_Masterlist_Callback),	"Germany, admin: raz0?"},
	{MP_QUAKEWORLD, CVARFC("net_qwmasterextra5", "qwmaster.fodquake.net:27000",						CVAR_NOSAVE, Net_Masterlist_Callback),	"admin: bigfoot"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"satan.idsoftware.com:27000",				CVAR_NOSAVE, Net_Masterlist_Callback),	"Official id Master"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"satan.idsoftware.com:27002",				CVAR_NOSAVE, Net_Masterlist_Callback),	"Official id Master For CTF Servers"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"satan.idsoftware.com:27003",				CVAR_NOSAVE, Net_Masterlist_Callback),	"Official id Master For TeamFortress Servers"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"satan.idsoftware.com:27004",				CVAR_NOSAVE, Net_Masterlist_Callback),	"Official id Master For Miscilaneous Servers"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"satan.idsoftware.com:27006",				CVAR_NOSAVE, Net_Masterlist_Callback),	"Official id Master For Deathmatch Servers"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"150.254.66.120:27000",						CVAR_NOSAVE, Net_Masterlist_Callback),	"Poland"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"62.112.145.129:27000",						CVAR_NOSAVE, Net_Masterlist_Callback),	"Ocrana (original)"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"master.edome.net",							CVAR_NOSAVE, Net_Masterlist_Callback),	"edome"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"qwmaster.barrysworld.com",					CVAR_NOSAVE, Net_Masterlist_Callback),	"barrysworld"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"213.221.174.165:27000",					CVAR_NOSAVE, Net_Masterlist_Callback),	"unknown1"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"195.74.0.8",								CVAR_NOSAVE, Net_Masterlist_Callback),	"unknown2"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"204.182.161.2",							CVAR_NOSAVE, Net_Masterlist_Callback),	"unknown5"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"kubus.rulez.pl:27000",						CVAR_NOSAVE, Net_Masterlist_Callback),	"kubus.rulez.pl"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"telefrag.me:27000",						CVAR_NOSAVE, Net_Masterlist_Callback),	"telefrag.me"},
//	{MP_QUAKEWORLD, CVARFC("net_qwmasterextraHistoric",	"master.teamdamage.com:27000",				CVAR_NOSAVE, Net_Masterlist_Callback),	"master.teamdamage.com"},

	{MP_DPMASTER,	CVARFC("net_masterextra1",		"ghdigital.com:27950 69.59.212.88:27950",										CVAR_NOSAVE, Net_Masterlist_Callback)}, //69.59.212.88 (admin: LordHavoc)
	{MP_DPMASTER,	CVARFC("net_masterextra2",		"dpmaster.deathmask.net:27950 107.161.23.68:27950 [2604:180::4ac:98c1]:27950",	CVAR_NOSAVE, Net_Masterlist_Callback)}, //107.161.23.68 (admin: Willis)
	{MP_DPMASTER,	CVARFC("net_masterextra3",		"dpmaster.tchr.no:27950 92.62.40.73:27950",										CVAR_NOSAVE, Net_Masterlist_Callback)}, //92.62.40.73 (admin: tChr)

//	{MP_QUAKE2,		CVARFC("net_q2masterextra1",	"satan.idsoftware.com:27900",					CVAR_NOSAVE, Net_Masterlist_Callback),	"Official Quake2 master server"},
//	{MP_QUAKE2,		CVARFC("net_q2masterextra1",	"master.planetgloom.com:27900",					CVAR_NOSAVE, Net_Masterlist_Callback)},	//?
//	{MP_QUAKE2,		CVARFC("net_q2masterextra1",	"master.q2servers.com:27900",					CVAR_NOSAVE, Net_Masterlist_Callback)},	//?
	{MP_QUAKE2,		CVARFC("net_q2masterextra1",	"netdome.biz:27900",							CVAR_NOSAVE, Net_Masterlist_Callback)},	//?

//	{MP_QUAKE3,		CVARFC("net_q3masterextra1",	"masterserver.exhale.de:27950",					CVAR_NOSAVE, Net_Masterlist_Callback),	"Official Quake3 master server"},
	{MP_QUAKE3,		CVARFC("net_q3masterextra1",	"master.quake3arena.com:27950",					CVAR_NOSAVE, Net_Masterlist_Callback),	"Official Quake3 master server"},

	{MP_UNSPECIFIED, CVAR(NULL, NULL)}
};
void Net_Master_Init(void)
{
	int i;
	for (i = 0; net_masterlist[i].cv.name; i++)
		Cvar_Register(&net_masterlist[i].cv, "master servers");
#ifndef CLIENTONLY
	Cmd_AddCommand ("setmaster", SV_SetMaster_f);
#endif
}

#ifndef CLIENTONLY

void Net_Masterlist_Callback(struct cvar_s *var, char *oldvalue)
{
	int i;

	for (i = 0; net_masterlist[i].cv.name; i++)
	{
		if (var == &net_masterlist[i].cv)
			break;
	}

	if (!net_masterlist[i].cv.name)
		return;

	net_masterlist[i].needsresolve = true;
}

void SV_Master_SingleHeartbeat(net_masterlist_t *master)
{
	char		string[2048];
	qboolean	madeqwstring = false;
	char		adr[MAX_ADR_SIZE];
	netadr_t	*na;
	int i;

	for (i = 0; i < MAX_MASTER_ADDRESSES; i++)
	{
		na = &master->adr[i];
		if (na->port)
		{
			switch(master->protocol)
			{
			case MP_QUAKEWORLD:
				if (sv_listen_qw.value)
				{
					if (!madeqwstring)
					{
						int active, j;

						// count active users
						active = 0;
						for (j=0 ; j<svs.allocated_client_slots ; j++)
							if (svs.clients[j].state == cs_connected ||
							svs.clients[j].state == cs_spawned )
								active++;

						sprintf (string, "%c\n%i\n%i\n", S2M_HEARTBEAT,
							svs.heartbeat_sequence, active);

						madeqwstring = true;
					}

					if (sv_reportheartbeats.value)
						Con_TPrintf ("Sending heartbeat to %s (%s)\n", NET_AdrToString (adr, sizeof(adr), na), master->cv.string);

					NET_SendPacket (NS_SERVER, strlen(string), string, na);
				}
				break;
			case MP_QUAKE2:
				if (svs.gametype == GT_QUAKE2 && sv_listen_qw.value)	//set listen to 1 to allow qw connections, 2 to allow nq connections too.
				{
					if (sv_reportheartbeats.value)
						Con_TPrintf ("Sending heartbeat to %s (%s)\n", NET_AdrToString (adr, sizeof(adr), na), master->cv.string);

					{
						char *str = "\377\377\377\377heartbeat\n%s";
						char *q2statusresp = "";
						NET_SendPacket (NS_SERVER, strlen(str), va(str, q2statusresp), na);
					}
				}
				break;
			case MP_DPMASTER:
				if (sv_listen_dp.value || sv_listen_nq.value)	//set listen to 1 to allow qw connections, 2 to allow nq connections too.
				{
					if (sv_reportheartbeats.value)
						Con_TPrintf ("Sending heartbeat to %s (%s)\n", NET_AdrToString (adr, sizeof(adr), na), master->cv.string);

					{
						//darkplaces here refers to the master server protocol, rather than the game protocol
						//(specifies that the server responds to infoRequest packets from the master)
						char *str = "\377\377\377\377heartbeat DarkPlaces\x0A";
						NET_SendPacket (NS_SERVER, strlen(str), str, na);
					}
				}
				break;
			default:
				break;
			}
		}
	}
}

//main thread
struct thr_res
{
	qboolean success;
	netadr_t na[8];
	char str[1];	//trailing
};
void SV_Master_Worker_Resolved(void *ctx, void *data, size_t a, size_t b)
{
	int i;
	struct thr_res *work = data;
	netadr_t *na;
	net_masterlist_t *master = &net_masterlist[a];

	master->resolving = false;

	//only accept the result if the master wasn't changed while we were still resolving it. no race conditions please.
	if (!strcmp(master->cv.string, work->str))
	{
		master->needsresolve = false;
		for (i = 0; i < MAX_MASTER_ADDRESSES; i++)
		{
			na = &master->adr[i];
			*na = work->na[i];
			master->needsresolve = false;

			if (na->type == NA_INVALID)
				memset(na, 0, sizeof(*na));
			else
			{
				//fix up default ports if not specified
				if (!na->port)
				{
					switch (master->protocol)
					{
					case MP_DPMASTER:	na->port = BigShort (27950);	break;
					case MP_QUAKE2:		na->port = BigShort (27000);	break;	//FIXME: verify
					case MP_QUAKE3:		na->port = BigShort (27950);	break;
					case MP_QUAKEWORLD:	na->port = BigShort (27000);	break;
					}
				}

				//some master servers require a ping to get them going or so
				if (sv.state)
				{
					//tcp masters require a route
					if (na->type == NA_TCP || na->type == NA_TCPV6 || na->type == NA_TLSV4 || na->type == NA_TLSV6)
						NET_EnsureRoute(svs.sockets, master->cv.name, master->cv.string, false);

					//q2+qw masters are given a ping to verify that they're still up
					switch (master->protocol)
					{
					case MP_QUAKE2:
						NET_SendPacket (NS_SERVER, 8, "\xff\xff\xff\xffping", na);
						break;
					case MP_QUAKEWORLD:
						//qw does this for some reason, keep the behaviour even though its unreliable thus pointless
						NET_SendPacket (NS_SERVER, 2, "k\0", na);
						break;
					default:
						break;
					}
				}
			}
		}
		if (!work->success)
			Con_TPrintf ("Couldn't resolve master \"%s\"\n", master->cv.string);
		else
			SV_Master_SingleHeartbeat(master);
	}
	Z_Free(work);
}
//worker thread
void SV_Master_Worker_Resolve(void *ctx, void *data, size_t a, size_t b)
{
	char token[1024];
	int found = 0;
	qboolean first = true;
	char *str;
	struct thr_res *work = data;
	str = work->str;
	while (str && *str)
	{
		str = COM_ParseOut(str, token, sizeof(token));
		if (*token)
			found += NET_StringToAdr2(token, 0, &work->na[found], MAX_MASTER_ADDRESSES-found);
		if (first && found)
			break;	//if we found one by name, don't try any fallback ip addresses.
		first = false;
	}
	work->success = !!found;
	COM_AddWork(0, SV_Master_Worker_Resolved, NULL, work, a, b);
}

/*
================
Master_Heartbeat

Send a message to the master every few minutes to
let it know we are alive, and log information
================
*/
#define	HEARTBEAT_SECONDS	300
void SV_Master_Heartbeat (void)
{
	int			i;
	qboolean	enabled;

	if (!sv_public.ival || SSV_IsSubServer())
		return;

	if (realtime-HEARTBEAT_SECONDS - svs.last_heartbeat < HEARTBEAT_SECONDS)
		return;		// not time to send yet

	svs.last_heartbeat = realtime-HEARTBEAT_SECONDS;

	svs.heartbeat_sequence++;

	Master_DetermineMasterTypes();

	// send to group master
	for (i = 0; net_masterlist[i].cv.name; i++)
	{
		switch (net_masterlist[i].protocol)
		{
		case MP_DPMASTER:	enabled = sb_enabledarkplaces;	break;
		case MP_QUAKE2:		enabled = sb_enablequake2;		break;
		case MP_QUAKE3:		enabled = sb_enablequake3;		break;
		case MP_QUAKEWORLD:	enabled = sb_enablequakeworld;	break;
		default:			enabled = false;				break;
		}
		if (!enabled)
			continue;

		if (net_masterlist[i].resolving)
			continue;

		if (net_masterlist[i].needsresolve)
		{
			if (!*net_masterlist[i].cv.string || *net_masterlist[i].cv.string == '*')
				memset(net_masterlist[i].adr, 0, sizeof(net_masterlist[i].adr));
			else
			{
				struct thr_res *work = Z_Malloc(sizeof(*work) + strlen(net_masterlist[i].cv.string));
				strcpy(work->str, net_masterlist[i].cv.string);
				net_masterlist[i].resolving = true;	//don't spam work
				COM_AddWork(0, SV_Master_Worker_Resolve, NULL, work, i, 0);
			}
		}
		else
			SV_Master_SingleHeartbeat(&net_masterlist[i]);
	}
}

static void SV_Master_Add(int type, char *stringadr)
{
	int i;

	for (i = 0; net_masterlist[i].cv.name; i++)
	{
		if (net_masterlist[i].protocol != type)
			continue;
		if (!*net_masterlist[i].cv.string)
			break;
	}

	if (!net_masterlist[i].cv.name)
	{
		Con_Printf ("Too many masters\n");
		return;
	}

	Cvar_Set(&net_masterlist[i].cv, stringadr);

	svs.last_heartbeat = -99999;
}

void SV_Master_ClearAll(void)
{
	int i;
	for (i = 0; net_masterlist[i].cv.name; i++)
	{
		Cvar_Set(&net_masterlist[i].cv, "");
	}
}

/*
====================
SV_SetMaster_f

Make a master server current. deprecated in favour of setting numbered masters via configs/engine source code.
only supports qw masters.
====================
*/
static void SV_SetMaster_f (void)
{
	int		i;

	SV_Master_ClearAll();

	if (!strcmp(Cmd_Argv(1), "none"))
	{
		if (cl_warncmd.ival)
			Con_Printf ("Entering no-master mode\n");
		return;
	}
	if (!strcmp(Cmd_Argv(1), "clear"))
		return;

	if (!strcmp(Cmd_Argv(1), "default"))
	{
		for (i = 0; net_masterlist[i].cv.name; i++)
			Cvar_Set(&net_masterlist[i].cv, net_masterlist[i].cv.enginevalue);
		return;
	}

	Cvar_Set(&sv_public, "1");	//go public.

	for (i=1 ; i<Cmd_Argc() ; i++)
	{
		SV_Master_Add(MP_QUAKEWORLD, Cmd_Argv(i));
	}

	svs.last_heartbeat = -99999;
}

void SV_Master_ReResolve(void)
{
	int i;
	for (i = 0; net_masterlist[i].cv.name; i++)
	{
		net_masterlist[i].needsresolve = true;
	}
	//trigger a heartbeat at the next available opportunity.
	svs.last_heartbeat = -9999;
}

/*
=================
Master_Shutdown

Informs all masters that this server is going down
=================
*/
void SV_Master_Shutdown (void)
{
	char		string[2048];
	char		adr[MAX_ADR_SIZE];
	int			i, j;
	netadr_t	*na;

	//note that if a master server actually blindly listens to this then its exploitable.
	//we send it out anyway as for us its all good.
	//master servers ought to try and check up on the status of the server first, if they listen to this.

	sprintf (string, "%c\n", S2M_SHUTDOWN);

	// send to group master
	for (i = 0; net_masterlist[i].cv.name; i++)
	{
		for (j = 0; j < MAX_MASTER_ADDRESSES; j++)
		{
			na = &net_masterlist[i].adr[j];
			if (na->port)
			{
				switch(net_masterlist[i].protocol)
				{
				case MP_QUAKEWORLD:
					if (sv_reportheartbeats.value)
						Con_TPrintf ("Sending shutdown to %s\n", NET_AdrToString (adr, sizeof(adr), na));

					NET_SendPacket (NS_SERVER, strlen(string), string, na);
					break;
				//dp has no shutdown
				default:
					break;
				}
			}
		}
	}
}
#endif




#if defined(CL_MASTER) && !defined(SERVERONLY)

#define NET_GAMENAME_NQ		"QUAKE"

//rename to cl_master.c sometime

//the networking operates seperatly from the main code. This is so we can have full control over all parts of the server sending prints.
//when we send status to the server, it replys with a print command. The text to print contains the serverinfo.
//Q2's print command is a compleate 'print', while qw is just a 'p', thus we can distinguish the two easily.

//save favorites and allow addition of new ones from game?
//add filters some time

//remove dead servers.
//master was polled a minute ago and server was not on list - server on multiple masters would be awkward.

#ifdef _WIN32
#include "winquake.h"
#define USEIPX
#else
typedef int SOCKET;
#endif

#include "netinc.h"

#ifdef AF_IPX
#define USEIPX
#endif


//the number of servers should be limited only by memory.

cvar_t slist_cacheinfo = SCVAR("slist_cacheinfo", "0");	//this proves dangerous, memory wise.
cvar_t slist_writeserverstxt = SCVAR("slist_writeservers", "0");

void CL_MasterListParse(netadrtype_t adrtype, int type, qboolean slashpad);
void CL_QueryServers(void);
int CL_ReadServerInfo(char *msg, enum masterprotocol_e prototype, qboolean favorite);
void MasterInfo_RemoveAllPlayers(void);

master_t *master;
player_t *mplayers;
serverinfo_t *firstserver;
struct selectedserver_s selectedserver;

static serverinfo_t **visibleservers;
static int numvisibleservers;
static int maxvisibleservers;

static double nextsort;

static hostcachekey_t sortfield;
static qboolean decreasingorder;




typedef struct {
	hostcachekey_t fieldindex;

	float operandi;
	const char *operands;

	qboolean or;
	int compareop;
} visrules_t;
#define MAX_VISRULES 8
visrules_t visrules[MAX_VISRULES];
int numvisrules;




#define SLIST_MAXKEYS 64
char slist_keyname[SLIST_MAXKEYS][MAX_INFO_KEY];
int slist_customkeys;


#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif


#define POLLUDP4SOCKETS 64	//it's big so we can have lots of messages when behind a firewall. Basically if a firewall only allows replys, and only remembers 3 servers per socket, we need this big cos it can take a while for a packet to find a fast optimised route and we might be waiting for a few secs for a reply the first time around.
int lastpollsockUDP4;

#ifdef IPPROTO_IPV6
#define POLLUDP6SOCKETS 4	//it's non-zero so we can have lots of messages when behind a firewall. Basically if a firewall only allows replys, and only remembers 3 servers per socket, we need this big cos it can take a while for a packet to find a fast optimised route and we might be waiting for a few secs for a reply the first time around.
int lastpollsockUDP6;
#else
#define POLLUDP6SOCKETS 0
#endif

#ifdef USEIPX
#define POLLIPXSOCKETS	2	//ipx isn't used as much. In fact, we only expect local servers to be using it. I'm not sure why I implemented it anyway. You might see a q2 server using it. Rarely.
int lastpollsockIPX;
#else
#define POLLIPXSOCKETS 0
#endif

#define FIRSTIPXSOCKET (0)
#define FIRSTUDP4SOCKET (FIRSTIPXSOCKET+POLLIPXSOCKETS)
#define FIRSTUDP6SOCKET (FIRSTUDP4SOCKET+POLLUDP4SOCKETS)
#define POLLTOTALSOCKETS (FIRSTUDP6SOCKET+POLLUDP6SOCKETS)
SOCKET pollsocketsList[POLLTOTALSOCKETS];

void Master_SetupSockets(void)
{
	int i;
	for (i = 0; i < POLLTOTALSOCKETS; i++)
		pollsocketsList[i] = INVALID_SOCKET;
}

void Master_HideServer(serverinfo_t *server)
{
	int i, j;
	for (i = 0; i < numvisibleservers;)
	{
		if (visibleservers[i] == server)
		{
			for (j = i; j < numvisibleservers-1; j++)
				visibleservers[j] = visibleservers[j+1];
			visibleservers--;
		}
		else
			 i++;
	}
	server->insortedlist = false;
}

void Master_InsertAt(serverinfo_t *server, int pos)
{
	int i;
	if (numvisibleservers >= maxvisibleservers)
	{
		maxvisibleservers = maxvisibleservers+10;
		visibleservers = BZ_Realloc(visibleservers, maxvisibleservers*sizeof(serverinfo_t*));
	}
	for (i = numvisibleservers; i > pos; i--)
	{
		visibleservers[i] = visibleservers[i-1];
	}
	visibleservers[pos] = server;
	numvisibleservers++;

	server->insortedlist = true;
}

qboolean Master_CompareInteger(int a, int b, slist_test_t rule)
{
	switch(rule)
	{
	case SLIST_TEST_CONTAINS:
		return !!(a&b);
	case SLIST_TEST_NOTCONTAIN:
		return !(a&b);
	case SLIST_TEST_LESSEQUAL:
		return a<=b;
	case SLIST_TEST_LESS:
		return a<b;
	case SLIST_TEST_STARTSWITH:
	case SLIST_TEST_EQUAL:
		return a==b;
	case SLIST_TEST_GREATER:
		return a>b;
	case SLIST_TEST_GREATEREQUAL:
		return a>=b;
	case SLIST_TEST_NOTSTARTSWITH:
	case SLIST_TEST_NOTEQUAL:
		return a!=b;
	}
	return false;
}
qboolean Master_CompareString(const char *a, const char *b, slist_test_t rule)
{
	switch(rule)
	{
	case SLIST_TEST_STARTSWITH:
		return strnicmp(a, b, strlen(b))==0;
	case SLIST_TEST_NOTSTARTSWITH:
		return strnicmp(a, b, strlen(b))!=0;
	case SLIST_TEST_CONTAINS:
		return !!strstr(a, b);
	case SLIST_TEST_NOTCONTAIN:
		return !strstr(a, b);
	case SLIST_TEST_LESSEQUAL:
		return stricmp(a, b)<=0;
	case SLIST_TEST_LESS:
		return stricmp(a, b)<0;
	case SLIST_TEST_EQUAL:
		return stricmp(a, b)==0;
	case SLIST_TEST_GREATER:
		return stricmp(a, b)>0;
	case SLIST_TEST_GREATEREQUAL:
		return stricmp(a, b)>=0;
	case SLIST_TEST_NOTEQUAL:
		return stricmp(a, b)!=0;
	}
	return false;
}

qboolean Master_ServerIsGreater(serverinfo_t *a, serverinfo_t *b)
{
	switch(sortfield)
	{
	case SLKEY_ADDRESS:
		break;
	case SLKEY_BASEGAME:
		return Master_CompareInteger(a->special&SS_PROTOCOLMASK, b->special&SS_PROTOCOLMASK, SLIST_TEST_LESS);
	case SLKEY_FLAGS:
		return Master_CompareInteger(a->special&~SS_PROTOCOLMASK, b->special&~SS_PROTOCOLMASK, SLIST_TEST_LESS);
	case SLKEY_CUSTOM:
		break;
	case SLKEY_FRAGLIMIT:
		return Master_CompareInteger(a->fl, b->fl, SLIST_TEST_LESS);
	case SLKEY_FREEPLAYERS:
		return Master_CompareInteger(a->maxplayers - a->players, b->maxplayers - b->players, SLIST_TEST_LESS);
	case SLKEY_GAMEDIR:
		return Master_CompareString(a->gamedir, b->gamedir, SLIST_TEST_LESS);
	case SLKEY_MAP:
		return Master_CompareString(a->map, b->map, SLIST_TEST_LESS);
	case SLKEY_MAXPLAYERS:
		return Master_CompareInteger(a->maxplayers, b->maxplayers, SLIST_TEST_LESS);
	case SLKEY_NAME:
		return Master_CompareString(a->name, b->name, SLIST_TEST_LESS);
	case SLKEY_NUMPLAYERS:
		return Master_CompareInteger(a->players, b->players, SLIST_TEST_LESS);
	case SLKEY_PING:
		return Master_CompareInteger(a->ping, b->ping, SLIST_TEST_LESS);
	case SLKEY_TIMELIMIT:
		return Master_CompareInteger(a->tl, b->tl, SLIST_TEST_LESS);
	case SLKEY_TOOMANY:
		break;

	// warning: enumeration value �SLKEY_*� not handled in switch
	case SLKEY_MOD:
	case SLKEY_PROTOCOL:
	case SLKEY_NUMBOTS:
	case SLKEY_NUMHUMANS:
	case SLKEY_QCSTATUS:
	case SLKEY_ISFAVORITE:
		break;

	}
	return false;
}

qboolean Master_PassesMasks(serverinfo_t *a)
{
	int i;
	qboolean val, res;
//	qboolean enabled;

	//always filter out dead unresponsive servers.
	if (!a->ping)
		return false;

/*	switch(a->special & SS_PROTOCOLMASK)
	{
	case SS_QUAKE3: enabled = sb_enablequake3; break;
	case SS_QUAKE2: enabled = sb_enablequake2; break;
	case SS_NETQUAKE: enabled = sb_enablenetquake; break;
	case SS_QUAKEWORLD: enabled = sb_enablequakeworld; break;
	case SS_DARKPLACES: enabled = sb_enabledarkplaces; break;
	default: enabled = true; break;
	}
	if (!enabled)
		return false;
*/
	val = 1;

	for (i = 0; i < numvisrules; i++)
	{
		switch(visrules[i].fieldindex)
		{
		case SLKEY_PING:
			res = Master_CompareInteger(a->ping, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_NUMPLAYERS:
			res = Master_CompareInteger(a->players, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_MAXPLAYERS:
			res = Master_CompareInteger(a->maxplayers, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_FREEPLAYERS:
			res = Master_CompareInteger(a->freeslots, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_NUMBOTS:
			res = Master_CompareInteger(a->numbots, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_NUMHUMANS:
			res = Master_CompareInteger(a->numhumans, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_TIMELIMIT:
			res = Master_CompareInteger(a->tl, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_FRAGLIMIT:
			res = Master_CompareInteger(a->fl, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_PROTOCOL:
			res = Master_CompareInteger(a->fl, visrules[i].operandi, visrules[i].compareop);
			break;

		case SLKEY_MAP:
			res = Master_CompareString(a->map, visrules[i].operands, visrules[i].compareop);
			break;
		case SLKEY_NAME:
			res = Master_CompareString(a->name, visrules[i].operands, visrules[i].compareop);
			break;
		case SLKEY_GAMEDIR:
			res = Master_CompareString(a->gamedir, visrules[i].operands, visrules[i].compareop);
			break;

		case SLKEY_BASEGAME:
			res = Master_CompareInteger(a->special&SS_PROTOCOLMASK, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_FLAGS:
			res = Master_CompareInteger(a->special&~SS_PROTOCOLMASK, visrules[i].operandi, visrules[i].compareop);
			break;
		case SLKEY_MOD:
			res = Master_CompareString(a->modname, visrules[i].operands, visrules[i].compareop);
			break;
		case SLKEY_QCSTATUS:
			res = Master_CompareString(a->qcstatus, visrules[i].operands, visrules[i].compareop);
			break;
		default:
			continue;
		}
		if (visrules[i].or)
			val |= res;
		else
			val &= res;
	}

	return val;
}

void Master_ClearMasks(void)
{
	numvisrules = 0;
}

void Master_SetMaskString(qboolean or, hostcachekey_t field, const char *param, slist_test_t testop)
{
	if (numvisrules == MAX_VISRULES)
		return;	//just don't add it.

	nextsort = 0;
	visrules[numvisrules].fieldindex = field;
	visrules[numvisrules].compareop = testop;
	visrules[numvisrules].operands = param;
	visrules[numvisrules].or = or;
	numvisrules++;
}
void Master_SetMaskInteger(qboolean or, hostcachekey_t field, int param, slist_test_t testop)
{
	if (numvisrules == MAX_VISRULES)
		return;	//just don't add it.

	nextsort = 0;
	visrules[numvisrules].fieldindex = field;
	visrules[numvisrules].compareop = testop;
	visrules[numvisrules].operandi = param;
	visrules[numvisrules].or = or;
	numvisrules++;
}
void Master_SetSortField(hostcachekey_t field, qboolean descending)
{
	nextsort = 0;
	sortfield = field;
	decreasingorder = descending;
}
hostcachekey_t Master_GetSortField(void)
{
	return sortfield;
}
qboolean Master_GetSortDescending(void)
{
	return decreasingorder;
}

void Master_ShowServer(serverinfo_t *server)
{
	int i;
	if (!numvisibleservers)
	{
		Master_InsertAt(server, 0);
		return;
	}

	if (decreasingorder)
	{
		for (i = 0; i < numvisibleservers; i++)
		{
			if (!Master_ServerIsGreater(server, visibleservers[i]))
			{
				Master_InsertAt(server, i);
				return;
			}
		}

	}
	else
	{
		for (i = 0; i < numvisibleservers; i++)
		{
			if (Master_ServerIsGreater(server, visibleservers[i]))
			{
				Master_InsertAt(server, i);
				return;
			}
		}
	}

	Master_InsertAt(server, numvisibleservers);
}

void Master_ResortServer(serverinfo_t *server)
{
	if (server->insortedlist)
	{
		if (!Master_PassesMasks(server))
			Master_HideServer(server);
	}
	else
	{
		if (Master_PassesMasks(server))
			Master_ShowServer(server);
	}
}

void Master_SortServers(void)
{
	serverinfo_t *server;

	int total = Master_TotalCount();
	if (maxvisibleservers < total)
	{
		maxvisibleservers = total;
		visibleservers = BZ_Realloc(visibleservers, maxvisibleservers*sizeof(serverinfo_t*));
	}

	{
		numvisibleservers = 0;
		for (server = firstserver; server; server = server->next)
			server->insortedlist = false;
	}

	for (server = firstserver; server; server = server->next)
	{
		Master_ResortServer(server);
	}

	if (nextsort < Sys_DoubleTime())
		nextsort = Sys_DoubleTime() + 8;
}

serverinfo_t *Master_SortedServer(int idx)
{
//	if (nextsort < Sys_DoubleTime())
//		Master_SortServers();

	if (idx < 0 || idx >= numvisibleservers)
		return NULL;

	return visibleservers[idx];
}

int Master_NumSorted(void)
{
	if (nextsort < Sys_DoubleTime())
		Master_SortServers();

	return numvisibleservers;
}


float Master_ReadKeyFloat(serverinfo_t *server, int keynum)
{
	if (!server)
		return -1;
	else if (keynum < SLKEY_CUSTOM)
	{
		switch(keynum)
		{
		case SLKEY_PING:
			return server->ping;
		case SLKEY_NUMPLAYERS:
			return server->players;
		case SLKEY_MAXPLAYERS:
			return server->maxplayers;
		case SLKEY_FREEPLAYERS:
			return server->maxplayers - server->players;
		case SLKEY_BASEGAME:
			return server->special&SS_PROTOCOLMASK;
		case SLKEY_FLAGS:
			return server->special&~SS_PROTOCOLMASK;
		case SLKEY_TIMELIMIT:
			return server->tl;
		case SLKEY_FRAGLIMIT:
			return server->fl;
		case SLKEY_PROTOCOL:
			return server->protocol;
		case SLKEY_NUMBOTS:
			return server->numbots;
		case SLKEY_NUMHUMANS:
			return server->numhumans;
		case SLKEY_ISFAVORITE:
			return !!(server->special & SS_FAVORITE);
		case SLKEY_ISLOCAL:
			return !!(server->special & SS_LOCAL);
		case SLKEY_ISPROXY:
			return !!(server->special & SS_PROXY);

		default:
			return atof(Master_ReadKeyString(server, keynum));
		}
	}
	else if (server->moreinfo)
		return atof(Info_ValueForKey(server->moreinfo->info, slist_keyname[keynum-SLKEY_CUSTOM]));

	return 0;
}

char *Master_ReadKeyString(serverinfo_t *server, int keynum)
{
	static char adr[MAX_ADR_SIZE];

	if (!server)
		return "";

	if (keynum < SLKEY_CUSTOM)
	{
		switch(keynum)
		{
		case SLKEY_MAP:
			return server->map;
		case SLKEY_NAME:
			return server->name;
		case SLKEY_ADDRESS:
			return NET_AdrToString(adr, sizeof(adr), &server->adr);
		case SLKEY_GAMEDIR:
			return server->gamedir;

		case SLKEY_MOD:
			return server->modname;
		case SLKEY_QCSTATUS:
			return server->qcstatus;

		default:
			{
				static char s[64];
				sprintf(s, "%f", Master_ReadKeyFloat(server, keynum));
				return s;
			}
		}
	}
	else if (server->moreinfo)
		return Info_ValueForKey(server->moreinfo->info, slist_keyname[keynum-SLKEY_CUSTOM]);

	return "";
}

int Master_KeyForName(const char *keyname)
{
	int i;
	if (!strcmp(keyname, "map"))
		return SLKEY_MAP;
	else if (!strcmp(keyname, "ping"))
		return SLKEY_PING;
	else if (!strcmp(keyname, "name") || !strcmp(keyname, "hostname"))
		return SLKEY_NAME;
	else if (!strcmp(keyname, "address") || !strcmp(keyname, "cname"))
		return SLKEY_ADDRESS;
	else if (!strcmp(keyname, "maxplayers"))
		return SLKEY_MAXPLAYERS;
	else if (!strcmp(keyname, "numplayers"))
		return SLKEY_NUMPLAYERS;
	else if (!strcmp(keyname, "freeplayers") || !strcmp(keyname, "freeslots"))
		return SLKEY_FREEPLAYERS;
	else if (!strcmp(keyname, "gamedir") || !strcmp(keyname, "game") || !strcmp(keyname, "*gamedir"))
		return SLKEY_GAMEDIR;
	else if (!strcmp(keyname, "basegame"))
		return SLKEY_BASEGAME;
	else if (!strcmp(keyname, "flags"))
		return SLKEY_FLAGS;
	else if (!strcmp(keyname, "mod"))
		return SLKEY_MOD;
	else if (!strcmp(keyname, "protocol"))
		return SLKEY_PROTOCOL;
	else if (!strcmp(keyname, "numbots"))
		return SLKEY_NUMBOTS;
	else if (!strcmp(keyname, "numhumans"))
		return SLKEY_NUMHUMANS;
	else if (!strcmp(keyname, "qcstatus"))
		return SLKEY_QCSTATUS;
	else if (!strcmp(keyname, "isfavorite"))
		return SLKEY_ISFAVORITE;
	else if (!strcmp(keyname, "islocal"))
		return SLKEY_ISLOCAL;
	else if (!strcmp(keyname, "isproxy"))
		return SLKEY_ISPROXY;

	else if (slist_customkeys == SLIST_MAXKEYS)
		return SLKEY_TOOMANY;
	else
	{
		for (i = 0; i < slist_customkeys; i++)
		{
			if (!strcmp(slist_keyname[i], keyname))
			{
				return i + SLKEY_CUSTOM;
			}
		}
		Q_strncpyz(slist_keyname[slist_customkeys], keyname, MAX_INFO_KEY);

		slist_customkeys++;

		return slist_customkeys-1 + SLKEY_CUSTOM;
	}
}

//main thread
void CLMaster_AddMaster_Worker_Resolved(void *ctx, void *data, size_t a, size_t b)
{
	master_t *mast = data;
	master_t *oldmast;

	if (mast->adr.type == NA_INVALID)
	{
		Con_Printf("Failed to resolve master address \"%s\"\n", mast->address);
	}
	else if (mast->adr.type != NA_IP && mast->adr.type != NA_IPV6 && mast->adr.type != NA_IPX)
	{
		Con_Printf("Fixme: unable to poll address family for \"%s\"\n", mast->address);
	}
	else
	{
		if (mast->mastertype == MT_BCAST)	//broadcasts
		{
			if (mast->adr.type == NA_IP)
				mast->adr.type = NA_BROADCAST_IP;
			if (mast->adr.type == NA_IPX)
				mast->adr.type = NA_BROADCAST_IPX;
			if (mast->adr.type == NA_IPV6)
				mast->adr.type = NA_BROADCAST_IP6;
		}

		//fix up default ports if not specified
		if (!mast->adr.port)
		{
			switch (mast->protocoltype)
			{
			case MP_DPMASTER:	mast->adr.port = BigShort (27950);	break;
			case MP_QUAKE2:		mast->adr.port = BigShort (27000);	break;	//FIXME: verify
			case MP_QUAKE3:		mast->adr.port = BigShort (27950);	break;
			case MP_QUAKEWORLD:	mast->adr.port = BigShort (27000);	break;
			}
		}

		for (oldmast = master; oldmast; oldmast = oldmast->next)
		{
			if (NET_CompareAdr(&oldmast->adr, &mast->adr) && oldmast->mastertype == mast->mastertype && oldmast->protocoltype == mast->protocoltype)	//already exists.
				break;
		}
		if (!oldmast)
		{
			mast->next = master;
			master = mast;
			return;
		}
		else if (oldmast->nosave && !mast->nosave)
			oldmast->nosave = false;
	}
	Z_Free(mast);
}
//worker thread
void CLMaster_AddMaster_Worker_Resolve(void *ctx, void *data, size_t a, size_t b)
{
	netadr_t adrs[MAX_MASTER_ADDRESSES];
	char token[1024];
	int found = 0;
	qboolean first = true;
	char *str;
	master_t *work = data;
	//resolve all the addresses
	str = work->address;
	while (str && *str)
	{
		str = COM_ParseOut(str, token, sizeof(token));
		if (*token)
			found += NET_StringToAdr2(token, 0, &adrs[found], MAX_MASTER_ADDRESSES-found);
		if (first && found)
			break;	//if we found one by name, don't try any fallback ip addresses.
		first = false;
	}

	//add the main ip address
	work->adr = adrs[0];
	COM_AddWork(0, CLMaster_AddMaster_Worker_Resolved, NULL, work, a, b);

	//add dupes too (eg: ipv4+ipv6)
	while(found --> 1)
	{
		master_t *alt = Z_Malloc(sizeof(master_t)+strlen(work->name)+1+strlen(work->address)+1);
		alt->address = work->name + strlen(work->name)+1;
		alt->mastertype = work->mastertype;
		alt->protocoltype = work->protocoltype;
		strcpy(alt->name, work->name);
		strcpy(alt->address, work->address);
		alt->sends = 1;
		alt->nosave = true;
		alt->adr = adrs[found];
		COM_AddWork(0, CLMaster_AddMaster_Worker_Resolved, NULL, alt, a, b);
	}
}

void Master_AddMaster (char *address, enum mastertype_e mastertype, enum masterprotocol_e protocol, char *description)
{
	master_t *mast;

	if (!address || !*address)
		return;

	if (!description)
		description = address;

	mast = Z_Malloc(sizeof(master_t)+strlen(description)+1+strlen(address)+1);
	mast->address = mast->name + strlen(description)+1;
	mast->mastertype = mastertype;
	mast->protocoltype = protocol;
	strcpy(mast->name, description);
	strcpy(mast->address, address);
	mast->sends = 1;

	COM_AddWork(1, CLMaster_AddMaster_Worker_Resolve, NULL, mast, 0, 0);
}

void MasterInfo_Shutdown(void)
{
	master_t *mast;
	serverinfo_t *sv;
	MasterInfo_RemoveAllPlayers();
	while(firstserver)
	{
		sv = firstserver;
		firstserver = sv->next;
		Z_Free(sv);
	}
	while(master)
	{
		mast = master;
		master = mast->next;
#ifdef WEBCLIENT
		if (mast->dl)
			DL_Close(mast->dl);
#endif
		Z_Free(mast);
	}

	maxvisibleservers = 0;
	numvisibleservers = 0;
	Z_Free(visibleservers);
}

void Master_AddMasterHTTP (char *address, int mastertype, int protocoltype, char *description)
{
	master_t *mast;
/*	int servertype;

	if (protocoltype == MP_DP)
		servertype = SS_DARKPLACES;
	else if (protocoltype == MP_Q2)
		servertype = SS_QUAKE2;
	else if (protocoltype == MP_Q3)
		servertype = SS_QUAKE3;
	else if (protocoltype == MP_NQ)
		servertype = SS_NETQUAKE;
	else
		servertype = 0;
*/
	for (mast = master; mast; mast = mast->next)
	{
		if (!strcmp(mast->address, address) && mast->mastertype == mastertype && mast->protocoltype == protocoltype)	//already exists.
			return;
	}
	mast = Z_Malloc(sizeof(master_t)+strlen(description)+1+strlen(address)+1);
	mast->address = mast->name + strlen(description)+1;
	mast->mastertype = mastertype;
	mast->protocoltype = protocoltype;
//	mast->servertype = servertype;
	strcpy(mast->name, description);
	strcpy(mast->address, address);

	mast->next = master;
	master = mast;
}

//build a linked list of masters.	Doesn't duplicate addresses.
qboolean Master_LoadMasterList (char *filename, qboolean withcomment, int defaulttype, int defaultprotocol, int depth)
{
	vfsfile_t *f;
	char line[1024];
	char name[1024];
	char entry[1024];
	char *next, *sep;
	int servertype;
	int protocoltype;
	qboolean favourite;

	if (depth <= 0)
		return false;
	depth--;

	f = FS_OpenVFS(filename, "rb", FS_ROOT);
	if (!f)
		return false;

	while(VFS_GETS(f, line, sizeof(line)-1))
	{
		if (*line == '#')	//comment
			continue;

		*name = 0;
		favourite = false;
		servertype = defaulttype;
		protocoltype = defaultprotocol;

		next = COM_ParseOut(line, entry, sizeof(entry));
		if (!*com_token)
			continue;

		//special cases. Add a port if you have a server named 'file'... (unlikly)
		if (!strcmp(entry, "file"))
		{
			if (withcomment)
				next = COM_ParseOut(next, name, sizeof(name));
			next = COM_ParseOut(next, entry, sizeof(entry));
			if (!next)
				continue;
			servertype = MT_BAD;
		}
		else if (!strcmp(entry, "master"))
		{
			if (withcomment)
				next = COM_ParseOut(next, name, sizeof(name));
			next = COM_ParseOut(next, entry, sizeof(entry));
			if (!next)
				continue;
			servertype = MT_MASTERUDP;
		}
		else if (!strcmp(entry, "url"))
		{
			if (withcomment)
				next = COM_ParseOut(next, name, sizeof(name));
			next = COM_ParseOut(next, entry, sizeof(entry));
			servertype = MT_MASTERHTTP;
		}

		next = COM_Parse(next);

		for(sep = com_token; sep; sep = next)
		{
			next = strchr(sep, ':');
			if (next)
				*next = 0;

			if (!strcmp(sep, "single"))
				servertype = MT_SINGLE;
			else if (!strcmp(sep, "master"))
				servertype = MT_MASTERUDP;
			else if (!strcmp(sep, "masterhttp"))
				servertype = MT_MASTERHTTP;
			else if (!strcmp(sep, "masterhttpjson"))
				servertype = MT_MASTERHTTPJSON;
			else if (!strcmp(sep, "bcast"))
				servertype = MT_BCAST;

			else if (!strcmp(com_token, "qw"))
				protocoltype = MP_QUAKEWORLD;
			else if (!strcmp(com_token, "q2"))
				protocoltype = MP_QUAKE2;
			else if (!strcmp(com_token, "q3"))
				protocoltype = MP_QUAKE3;
			else if (!strcmp(com_token, "nq"))
				protocoltype = MP_NETQUAKE;
			else if (!strcmp(com_token, "dp"))
				protocoltype = MP_DPMASTER;

			//legacy compat
			else if (!strcmp(com_token, "httpjson"))
			{
				servertype = MT_MASTERHTTPJSON;
				protocoltype = MP_NETQUAKE;
			}
			else if (!strcmp(com_token, "httpnq"))
			{
				servertype = MT_MASTERHTTP;
				protocoltype = MP_NETQUAKE;
			}
			else if (!strcmp(com_token, "httpqw"))
			{
				servertype = MT_MASTERHTTP;
				protocoltype = MP_QUAKEWORLD;
			}

			else if (!strcmp(com_token, "favourite") || !strcmp(com_token, "favorite"))
				favourite = true;
		}

		if (!*name && next)
		{
			sep = name;
			while(*next == ' ' || *next == '\t')
				next++;
			while (*next && sep < name+sizeof(name)-1)
				*sep++ = *next++;
			*sep = 0;
		}

		if (servertype == MT_BAD)
			Master_LoadMasterList(entry, false, servertype, protocoltype, depth);
		else
		{
			//favourites are added explicitly, with their name and stuff
			if (favourite && servertype == MT_SINGLE)
			{
				if (NET_StringToAdr(entry, 0, &net_from))
					CL_ReadServerInfo(va("\\hostname\\%s", name), -servertype, true);
				else
					Con_Printf("Failed to resolve address - \"%s\"\n", entry);
			}

			switch (servertype)
			{
			case MT_MASTERHTTPJSON:
			case MT_MASTERHTTP:
				Master_AddMasterHTTP(entry, servertype, protocoltype, name);
				break;
			default:
				Master_AddMaster(entry, servertype, protocoltype, name);
				break;
			}
		}
	}
	VFS_CLOSE(f);

	return true;
}

void NET_SendPollPacket(int len, void *data, netadr_t to)
{
	int ret;
	struct sockaddr_qstorage	addr;

	NetadrToSockadr (&to, &addr);
#ifdef USEIPX
	if (((struct sockaddr*)&addr)->sa_family == AF_IPX)
	{
		lastpollsockIPX++;
		if (lastpollsockIPX>=POLLIPXSOCKETS)
			lastpollsockIPX=0;
		if (pollsocketsList[FIRSTIPXSOCKET+lastpollsockIPX]==INVALID_SOCKET)
			pollsocketsList[FIRSTIPXSOCKET+lastpollsockIPX] = IPX_OpenSocket(PORT_ANY, true);
		if (pollsocketsList[FIRSTIPXSOCKET+lastpollsockIPX]==INVALID_SOCKET)
			return;	//bother
		ret = sendto (pollsocketsList[FIRSTIPXSOCKET+lastpollsockIPX], data, len, 0, (struct sockaddr *)&addr, sizeof(addr) );
	}
	else
#endif
#ifdef IPPROTO_IPV6
	if (((struct sockaddr*)&addr)->sa_family == AF_INET6)
	{
		lastpollsockUDP6++;
		if (lastpollsockUDP6>=POLLUDP6SOCKETS)
			lastpollsockUDP6=0;
		if (pollsocketsList[FIRSTUDP6SOCKET+lastpollsockUDP6]==INVALID_SOCKET)
			pollsocketsList[FIRSTUDP6SOCKET+lastpollsockUDP6] = UDP6_OpenSocket(PORT_ANY, true);
		if (pollsocketsList[FIRSTUDP6SOCKET+lastpollsockUDP6]==INVALID_SOCKET)
			return;	//bother
		ret = sendto (pollsocketsList[FIRSTUDP6SOCKET+lastpollsockUDP6], data, len, 0, (struct sockaddr *)&addr, sizeof(addr) );
	}
	else
#endif
		if (((struct sockaddr*)&addr)->sa_family == AF_INET)
	{
		lastpollsockUDP4++;
		if (lastpollsockUDP4>=POLLUDP4SOCKETS)
			lastpollsockUDP4=0;
		if (pollsocketsList[FIRSTUDP4SOCKET+lastpollsockUDP4]==INVALID_SOCKET)
			pollsocketsList[FIRSTUDP4SOCKET+lastpollsockUDP4] = UDP_OpenSocket(PORT_ANY, true);
		if (pollsocketsList[FIRSTUDP4SOCKET+lastpollsockUDP4]==INVALID_SOCKET)
			return;	//bother
		ret = sendto (pollsocketsList[FIRSTUDP4SOCKET+lastpollsockUDP4], data, len, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in) );
	}
	else
		return;

	if (ret == -1)
	{
		int er = neterrno();
// wouldblock is silent
		if (er == NET_EWOULDBLOCK)
			return;

		if (er == NET_ECONNREFUSED)
			return;

		if (er == NET_EADDRNOTAVAIL)
			Con_DPrintf("NET_SendPollPacket Warning: %i\n", er);
		else
			Con_Printf ("NET_SendPollPacket ERROR: %i\n", er);
	}
}

int Master_CheckPollSockets(void)
{
	int sock;
	SOCKET usesocket;
	char adr[MAX_ADR_SIZE];

	for (sock = 0; sock < POLLTOTALSOCKETS; sock++)
	{
		int 	ret;
		struct sockaddr_qstorage	from;
		int		fromlen;

		usesocket = pollsocketsList[sock];

		if (usesocket == INVALID_SOCKET)
			continue;
		fromlen = sizeof(from);
		ret = recvfrom (usesocket, (char *)net_message_buffer, sizeof(net_message_buffer), 0, (struct sockaddr *)&from, &fromlen);

		if (ret == -1)
		{
			int e = neterrno();
			if (e == NET_EWOULDBLOCK)
				continue;
			if (e == NET_EMSGSIZE)
			{
				SockadrToNetadr (&from, &net_from);
				Con_Printf ("Warning:  Oversize packet from %s\n",
					NET_AdrToString (adr, sizeof(adr), &net_from));
				continue;
			}
			if (e == NET_ECONNABORTED || e == NET_ECONNRESET)
			{
//				Con_Printf ("Connection lost or aborted\n");
				continue;
			}


			Con_Printf ("NET_CheckPollSockets: %i, %s\n", e, strerror(e));
			continue;
		}
		SockadrToNetadr (&from, &net_from);

		net_message.cursize = ret;
		if (ret == sizeof(net_message_buffer) )
		{
			Con_Printf ("Oversize packet from %s\n", NET_AdrToString (adr, sizeof(adr), &net_from));
			continue;
		}

		if (*(int *)net_message.data == -1)
		{
			int c;
			char *s;

			MSG_BeginReading (msg_nullnetprim);
			MSG_ReadLong ();        // skip the -1

			c = msg_readcount;
			s = MSG_ReadStringLine();	//peek for q2 messages.
#ifdef Q2CLIENT
			if (!strcmp(s, "print"))
			{
				CL_ReadServerInfo(MSG_ReadString(), MP_QUAKE2, false);
				continue;
			}
			if (!strcmp(s, "info"))	//parse a bit more...
			{
				CL_ReadServerInfo(MSG_ReadString(), MP_QUAKE2, false);
				continue;
			}
#ifdef IPPROTO_IPV6
			if (!strncmp(s, "server6", 7))	//parse a bit more...
			{
				msg_readcount = c+7;
				CL_MasterListParse(NA_IPV6, SS_QUAKE2, false);
				continue;
			}
#endif
			if (!strncmp(s, "servers", 7))	//parse a bit more...
			{
				msg_readcount = c+7;
				CL_MasterListParse(NA_IP, SS_QUAKE2, false);
				continue;
			}
#endif
#ifdef Q3CLIENT
			if (!strcmp(s, "statusResponse"))
			{
				CL_ReadServerInfo(MSG_ReadString(), MP_QUAKE3, false);
				continue;
			}
#endif

#ifdef IPPROTO_IPV6
			if (!strncmp(s, "getserversResponse6", 19) && (s[19] == '\\' || s[19] == '/'))	//parse a bit more...
			{
				msg_readcount = c+19-1;
				CL_MasterListParse(NA_IPV6, SS_DARKPLACES, true);
				continue;
			}
#endif
			if (!strncmp(s, "getserversExtResponse", 21) && (s[21] == '\\' || s[21] == '/'))	//parse a bit more...
			{
				msg_readcount = c+21-1;
				CL_MasterListParse(NA_IP, SS_DARKPLACES, true);
				continue;
			}
			if (!strncmp(s, "getserversResponse", 18) && (s[18] == '\\' || s[18] == '/'))	//parse a bit more...
			{
				msg_readcount = c+18-1;
				CL_MasterListParse(NA_IP, SS_DARKPLACES, true);
				continue;
			}
			if (!strcmp(s, "infoResponse"))	//parse a bit more...
			{
				CL_ReadServerInfo(MSG_ReadString(), MP_DPMASTER, false);
				continue;
			}

#ifdef IPPROTO_IPV6
			if (!strncmp(s, "qw_slist6\\", 10))	//parse a bit more...
			{
				msg_readcount = c+9-1;
				CL_MasterListParse(NA_IPV6, SS_QUAKEWORLD, false);
				continue;
			}
#endif

			msg_readcount = c;

			c = MSG_ReadByte ();

			if (c == A2C_PRINT)	//qw server reply.
			{
				CL_ReadServerInfo(MSG_ReadString(), MP_QUAKEWORLD, false);
				continue;
			}

			if (c == M2C_MASTER_REPLY)	//qw master reply.
			{
				CL_MasterListParse(NA_IP, SS_QUAKEWORLD, false);
				continue;
			}
		}
#ifdef NQPROT
		else
		{	//connected packet? Must be a NQ packet.
			char name[32];
			char map[16];
			int users, maxusers;

			int control;

			MSG_BeginReading (msg_nullnetprim);
			control = BigLong(*((int *)net_message.data));
			MSG_ReadLong();
			if (control == -1)
				continue;
			if ((control & (~NETFLAG_LENGTH_MASK)) !=  NETFLAG_CTL)
				continue;
			if ((control & NETFLAG_LENGTH_MASK) != ret)
				continue;

			if (MSG_ReadByte() != CCREP_SERVER_INFO)
				continue;

			/*this is an address string sent from the server. its not usable. if its replying to serverinfos, its possible to send it connect requests, while the address that it claims is 50% bugged*/
			MSG_ReadString();

			Q_strncpyz(name, MSG_ReadString(), sizeof(name));
			Q_strncpyz(map, MSG_ReadString(), sizeof(map));
			users = MSG_ReadByte();
			maxusers = MSG_ReadByte();
			if (MSG_ReadByte() != NQ_NETCHAN_VERSION)
			{
//				Q_strcpy(name, "*");
//				Q_strcat(name, name);
			}

			CL_ReadServerInfo(va("\\hostname\\%s\\map\\%s\\maxclients\\%i\\clients\\%i", name, map, maxusers, users), MP_NETQUAKE, false);
		}
#endif
		continue;
	}
	return 0;
}

void Master_RemoveKeepInfo(serverinfo_t *sv)
{
	sv->special &= ~SS_KEEPINFO;
	if (sv->moreinfo)
	{
		Z_Free(sv->moreinfo);
		sv->moreinfo = NULL;
	}
}

void SListOptionChanged(serverinfo_t *newserver)
{
	if (selectedserver.inuse)
	{
		serverinfo_t *oldserver;

		selectedserver.detail = NULL;

		if (!slist_cacheinfo.value)	//we have to flush it. That's the rules.
		{
			for (oldserver = firstserver; oldserver; oldserver=oldserver->next)
			{
				if (NET_CompareAdr(&selectedserver.adr, &oldserver->adr))//*(int*)selectedserver.ipaddress == *(int*)server->ipaddress && selectedserver.port == server->port)
				{
					if (oldserver->moreinfo)
					{
						Z_Free(oldserver->moreinfo);
						oldserver->moreinfo = NULL;
					}
					break;
				}
			}
		}

		if (!newserver)
			return;

		selectedserver.adr = newserver->adr;

		if (newserver->moreinfo)	//we cached it.
		{
			selectedserver.detail = newserver->moreinfo;
			return;
		}
//we don't know all the info, so send a request for it.
		selectedserver.detail = newserver->moreinfo = Z_Malloc(sizeof(serverdetailedinfo_t));

		newserver->moreinfo->numplayers = newserver->players;
		strcpy(newserver->moreinfo->info, "");
		Info_SetValueForKey(newserver->moreinfo->info, "hostname", newserver->name, sizeof(newserver->moreinfo->info));


		newserver->sends++;
		Master_QueryServer(newserver);
	}
}

#ifdef WEBCLIENT
void MasterInfo_ProcessHTTP(struct dl_download *dl)
{
	master_t *mast = dl->user_ctx;
	vfsfile_t *file = dl->file;
	int protocoltype = mast->protocoltype;
	netadr_t adr;
	char *s;
	char *el;
	serverinfo_t *info;
	char adrbuf[MAX_ADR_SIZE];
	char linebuffer[2048];
	mast->dl = NULL;

	if (!file)
		return;

	while(VFS_GETS(file, linebuffer, sizeof(linebuffer)))
	{
		s = linebuffer;
		while (*s == '\t' || *s == ' ')
			s++;

		el = s + strlen(s);
		if (el>s && el[-1] == '\r')
			el[-1] = '\0';

		if (*s == '#')	//hash is a comment, apparently.
			continue;

		if (!NET_StringToAdr(s, 80, &adr))
			continue;

		if ((info = Master_InfoForServer(&adr)))	//remove if the server already exists.
		{
			info->sends = 1;	//reset.
		}
		else
		{
			info = Z_Malloc(sizeof(serverinfo_t));
			info->adr = adr;
			info->sends = 1;

			info->special = 0;
			if (protocoltype == MP_DPMASTER)
				info->special |= SS_DARKPLACES;
			else if (protocoltype == MP_QUAKE2)
				info->special |= SS_QUAKE2;
			else if (protocoltype == MP_QUAKE3)
				info->special |= SS_QUAKE3;
			else if (protocoltype == MP_NETQUAKE)
				info->special |= SS_NETQUAKE;

			info->refreshtime = 0;
			info->ping = 0xffff;

			snprintf(info->name, sizeof(info->name), "%s", NET_AdrToString(adrbuf, sizeof(adrbuf), &info->adr));

			info->next = firstserver;
			firstserver = info;

			Master_ResortServer(info);
		}
	}
}

char *jsonnode(int level, char *node)
{
	netadr_t adr = {NA_INVALID};
	char servername[256] = {0};
	char key[256];
	int flags = SS_NETQUAKE;	//assumption
	int port = 0;
	int cp = 0, mp = 0;
	if (*node != '{')
		return node;
	do
	{
		node++;
		node = COM_ParseToken(node, ",:{}[]");
		if (*node != ':')
			continue;
		node++;
		if (*node == '[')
		{
			do
			{
				node++;
				node = jsonnode(level+1, node);
				if (!node)
					return NULL;
				if (*node == ']')
				{
					break;
				}
			} while(*node == ',');
			if (*node != ']')
				return NULL;
			node++;
		}
		else
		{
			Q_strncpyz(key, com_token, sizeof(key));
			node = COM_ParseToken(node, ",:{}[]");

			if (level == 1)
			{
				if (!strcmp(key, "IPAddress"))
					NET_StringToAdr(com_token, 0, &adr);
				if (!strcmp(key, "Port"))
					port = atoi(com_token);
				if (!strcmp(key, "DNS"))
					Q_strncpyz(servername, com_token, sizeof(servername));
				if (!strcmp(key, "CurrentPlayerCount"))
					cp = atoi(com_token);
				if (!strcmp(key, "MaxPlayers"))
					mp = atoi(com_token);
				if (!strcmp(key, "Game"))
				{
					flags &= ~SS_PROTOCOLMASK;
					if (!strcmp(com_token, "NetQuake"))
						flags |= SS_NETQUAKE;
					if (!strcmp(com_token, "QuakeWorld"))
						flags |= SS_QUAKEWORLD;
					if (!strcmp(com_token, "Quake2"))
						flags |= SS_QUAKE2;
					if (!strcmp(com_token, "Quake3"))
						flags |= SS_QUAKE3;
				}
			}
		}
	} while(*node == ',');

	if (*node == '}')
		node++;

	if (adr.type != NA_INVALID)
	{
		serverinfo_t *info;

		if (port)
			adr.port = htons(port);

		if ((info = Master_InfoForServer(&adr)))	//remove if the server already exists.
		{
			if (!info->special)
				info->special = flags;
			info->sends = 1;	//reset.
		}
		else
		{
			info = Z_Malloc(sizeof(serverinfo_t));
			info->adr = adr;
			info->sends = 1;
			info->special = flags;
			info->refreshtime = 0;
			info->players = cp;
			info->maxplayers = mp;

			snprintf(info->name, sizeof(info->name), "%s", *servername?servername:NET_AdrToString(servername, sizeof(servername), &info->adr));

			info->next = firstserver;
			firstserver = info;

			Master_ResortServer(info);
		}
	}

	return node;
}

void MasterInfo_ProcessHTTPJSON(struct dl_download *dl)
{
	int len;
	char *buf;
	master_t *mast = dl->user_ctx;
	mast->dl = NULL;
	if (dl->file)
	{
		len = VFS_GETLEN(dl->file);
		buf = malloc(len + 1);
		VFS_READ(dl->file, buf, len);
		buf[len] = 0;
		jsonnode(0, buf);
		free(buf);
	}
	else
	{
		Con_Printf("Unable to query master at \"%s\"\n", dl->url);
	}
}
#endif

//don't try sending to servers we don't support
void MasterInfo_Request(master_t *mast)
{
	//static int mastersequence; // warning: unused variable �mastersequence�
	if (!mast)
		return;

	if (mast->sends)
		mast->sends--;

	//these are generic requests
	switch(mast->mastertype)
	{
#ifdef WEBCLIENT
	case MT_MASTERHTTPJSON:
		if (!mast->dl)
		{
			mast->dl = HTTP_CL_Get(mast->address, NULL, MasterInfo_ProcessHTTPJSON);
			if (mast->dl)
				mast->dl->user_ctx = mast;
		}
		break;
	case MT_MASTERHTTP:
		if (!mast->dl)
		{
			mast->dl = HTTP_CL_Get(mast->address, NULL, MasterInfo_ProcessHTTP);
			if (mast->dl)
				mast->dl->user_ctx = mast;
		}
		break;
#endif
	case MT_MASTERUDP:
		switch(mast->protocoltype)
		{
#ifdef Q3CLIENT
		case MP_QUAKE3:
			{
				char *str;
				str = va("%c%c%c%cgetservers %u empty full\x0A\n", 255, 255, 255, 255, 68);
				NET_SendPollPacket (strlen(str), str, mast->adr);
			}
			break;
#endif
#ifdef Q2CLIENT
		case MP_QUAKE2:
			NET_SendPollPacket (6, "query", mast->adr);
			break;
#endif
		case MP_QUAKEWORLD:
			NET_SendPollPacket (3, "c\n", mast->adr);
			break;
#ifdef NQPROT
		case MP_NETQUAKE:
			//there is no nq udp master protocol
			break;
		case MP_DPMASTER:
			{
				char *str;
				//for compat with dp, we use the nq netchan version. which is stupid, but whatever
				//we ask for ipv6 addresses from ipv6 masters (assuming it resolved okay)
				if (mast->adr.type == NA_IPV6)
					str = va("%c%c%c%cgetserversExt %s %u empty full ipv6"/*\x0A\n"*/, 255, 255, 255, 255, com_protocolname.string, NQ_NETCHAN_VERSION);
				else
					str = va("%c%c%c%cgetservers %s %u empty full"/*\x0A\n"*/, 255, 255, 255, 255, com_protocolname.string, NQ_NETCHAN_VERSION);
				NET_SendPollPacket (strlen(str), str, mast->adr);
			}
			break;
#endif
		}
		break;
	case MT_BCAST:
	case MT_SINGLE:	//FIXME: properly add the server and flag it for resending instead of directly pinging it
		switch(mast->protocoltype)
		{
#ifdef Q3CLIENT
		case MP_QUAKE3:
			NET_SendPollPacket (14, va("%c%c%c%cgetstatus\n", 255, 255, 255, 255), mast->adr);
			break;
#endif
#ifdef Q2CLIENT
		case MP_QUAKE2:
#endif
		case MP_QUAKEWORLD:
			NET_SendPollPacket (11, va("%c%c%c%cstatus\n", 255, 255, 255, 255), mast->adr);
			break;
#ifdef NQPROT
		case MP_NETQUAKE:
			SZ_Clear(&net_message);
			net_message.packing = SZ_RAWBYTES;
			net_message.currentbit = 0;
			MSG_WriteLong(&net_message, 0);// save space for the header, filled in later
			MSG_WriteByte(&net_message, CCREQ_SERVER_INFO);
			MSG_WriteString(&net_message, NET_GAMENAME_NQ);	//look for either sort of server
			MSG_WriteByte(&net_message, NQ_NETCHAN_VERSION);
			*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
			NET_SendPollPacket(net_message.cursize, net_message.data, mast->adr);
			SZ_Clear(&net_message);
			break;
		case MP_DPMASTER:	//fixme
			{
				char *str;
				str = va("%c%c%c%cgetinfo", 255, 255, 255, 255);
				NET_SendPollPacket (strlen(str), str, mast->adr);
			}
			break;
#endif
		}
		break;
	}
}


void MasterInfo_WriteServers(void)
{
	char *typename, *protoname;
	master_t *mast;
	serverinfo_t *server;
	vfsfile_t *mf, *qws;
	char adr[MAX_ADR_SIZE];

	mf = FS_OpenVFS("masters.txt", "wt", FS_ROOT);
	if (!mf)
	{
		Con_Printf("Couldn't write masters.txt");
		return;
	}

	for (mast = master; mast; mast=mast->next)
	{
		if (mast->nosave)
			continue;

		switch(mast->mastertype)
		{
		case MT_MASTERUDP:
			typename = "master";
			break;
		case MT_MASTERHTTP:
			typename = "masterhttp";
			break;
		case MT_MASTERHTTPJSON:
			typename = "masterjson";
			break;
		case MT_BCAST:
			typename = "bcast";
			break;
		case MT_SINGLE:
			typename = "single";
			break;
		default:
			typename = "??";
			break;
		}
		switch(mast->protocoltype)
		{
		case MP_QUAKEWORLD:
			protoname = ":qw";
			break;
		case MP_QUAKE2:
			protoname = ":q2";
			break;
		case MP_QUAKE3:
			protoname = ":q3";
			break;
		case MP_NETQUAKE:
			protoname = ":nq";
			break;
		case MP_DPMASTER:
			protoname = ":dp";
			break;
		default:
		case MP_UNSPECIFIED:
			protoname = "";
			break;
		}
		if (mast->address)
			VFS_PUTS(mf, va("%s\t%s%s\t%s\n", mast->address, typename, protoname, mast->name));
		else
			VFS_PUTS(mf, va("%s\t%s\t%s\n", NET_AdrToString(adr, sizeof(adr), &mast->adr), typename, mast->name));
	}

	if (slist_writeserverstxt.value)
		qws = FS_OpenVFS("servers.txt", "wt", FS_ROOT);
	else
		qws = NULL;
	if (qws)
		VFS_PUTS(mf, va("\n%s\t%s\t%s\n\n", "file servers.txt", "favorite:qw", "personal server list"));

	for (server = firstserver; server; server = server->next)
	{
		if (server->special & SS_FAVORITE)
		{
			switch(server->special & SS_PROTOCOLMASK)
			{
			case SS_QUAKE3:
				VFS_PUTS(mf, va("%s\t%s\t%s\n", NET_AdrToString(adr, sizeof(adr), &server->adr), "favorite:q3", server->name));
				break;
			case SS_QUAKE2:
				VFS_PUTS(mf, va("%s\t%s\t%s\n", NET_AdrToString(adr, sizeof(adr), &server->adr), "favorite:q2", server->name));
				break;
			case SS_NETQUAKE:
				VFS_PUTS(mf, va("%s\t%s\t%s\n", NET_AdrToString(adr, sizeof(adr), &server->adr), "favorite:nq", server->name));
				break;
			case SS_QUAKEWORLD:
				if (qws)	//servers.txt doesn't support the extra info, so don't write it if its not needed
					VFS_PUTS(qws, va("%s\t%s\n", NET_AdrToString(adr, sizeof(adr), &server->adr), server->name));
				else			
					VFS_PUTS(mf, va("%s\t%s\t%s\n", NET_AdrToString(adr, sizeof(adr), &server->adr), "favorite:qw", server->name));
				break;
			}
		}
	}

	if (qws)
		VFS_CLOSE(qws);


	VFS_CLOSE(mf);
}

//poll master servers for server lists.
void MasterInfo_Refresh(void)
{
	master_t *mast;
	qboolean loadedone;

	loadedone = false;
	loadedone |= Master_LoadMasterList("masters.txt", false, MT_MASTERUDP, MP_QUAKEWORLD, 5);	//fte listing
	loadedone |= Master_LoadMasterList("sources.txt", true, MT_MASTERUDP, MP_QUAKEWORLD, 5);	//merge with ezquake compat listing

	if (!loadedone)
	{
		int i;
		Master_LoadMasterList("servers.txt", false, MT_MASTERUDP, MP_QUAKEWORLD, 1);

		Master_AddMasterHTTP("http://www.gameaholic.com/servers/qspy-quakeworld",	MT_MASTERHTTP,		MP_QUAKEWORLD, "gameaholic's QW master");
		Master_AddMasterHTTP("http://www.quakeservers.net/lists/servers/global.txt",MT_MASTERHTTP,		MP_QUAKEWORLD, "QuakeServers.net (http)");
		Master_AddMaster("255.255.255.255:"STRINGIFY(PORT_QWSERVER),				MT_BCAST,			MP_QUAKEWORLD, "Nearby QuakeWorld UDP servers.");
		Master_AddMasterHTTP("http://www.gameaholic.com/servers/qspy-quake",		MT_MASTERHTTP,		MP_NETQUAKE, "gameaholic's NQ master");
		Master_AddMasterHTTP("http://servers.quakeone.com/index.php?format=json",	MT_MASTERHTTPJSON,	MP_NETQUAKE, "quakeone's server listing");
		Master_AddMaster("255.255.255.255:"STRINGIFY(PORT_NQSERVER),				MT_BCAST,			MP_NETQUAKE, "Nearby Quake1 servers");
		Master_AddMaster("255.255.255.255:"STRINGIFY(PORT_NQSERVER),				MT_BCAST,			MP_DPMASTER, "Nearby DarkPlaces servers");
		Master_AddMasterHTTP("http://www.gameaholic.com/servers/qspy-quake2",		MT_MASTERHTTP,		MP_QUAKE2, "gameaholic's Q2 master");
		Master_AddMaster("255.255.255.255:27910",									MT_BCAST,			MP_QUAKE2, "Nearby Quake2 UDP servers.");
		Master_AddMasterHTTP("http://www.gameaholic.com/servers/qspy-quake3",		MT_MASTERHTTP,		MP_QUAKE3, "gameaholic's Q3 master");
		Master_AddMaster("255.255.255.255:"STRINGIFY(PORT_Q3SERVER),				MT_BCAST,			MP_QUAKE3, "Nearby Quake3 UDP servers.");

		for (i = 0; net_masterlist[i].cv.name; i++)
		{
			Master_AddMaster(net_masterlist[i].cv.string, MT_MASTERUDP, net_masterlist[i].protocol, net_masterlist[i].comment);
		}
	}


	for (mast = master; mast; mast=mast->next)
	{
		mast->sends = 1;
	}

	Master_SortServers();
	nextsort = Sys_DoubleTime() + 2;
}

void Master_QueryServer(serverinfo_t *server)
{
	char	data[2048];
	server->sends--;
	server->refreshtime = Sys_DoubleTime();

	switch(server->special & SS_PROTOCOLMASK)
	{
	case SS_QUAKE3:
		Q_snprintfz(data, sizeof(data), "%c%c%c%cgetstatus", 255, 255, 255, 255);
		break;
	case SS_DARKPLACES:
		Q_snprintfz(data, sizeof(data), "%c%c%c%cgetinfo", 255, 255, 255, 255);
		break;
#ifdef NQPROT
	case SS_NETQUAKE:
		SZ_Clear(&net_message);
		net_message.packing = SZ_RAWBYTES;
		net_message.currentbit = 0;
		MSG_WriteLong(&net_message, 0);// save space for the header, filled in later
		MSG_WriteByte(&net_message, CCREQ_SERVER_INFO);
		MSG_WriteString(&net_message, NET_GAMENAME_NQ);	//look for either sort of server
		MSG_WriteByte(&net_message, NQ_NETCHAN_VERSION);
		*((int *)net_message.data) = BigLong(NETFLAG_CTL | (net_message.cursize & NETFLAG_LENGTH_MASK));
		NET_SendPollPacket(net_message.cursize, net_message.data, server->adr);
		SZ_Clear(&net_message);
		return;
#endif
	case SS_QUAKEWORLD:
	case SS_QUAKE2:
		Q_snprintfz(data, sizeof(data), "%c%c%c%cstatus", 255, 255, 255, 255);
		break;
	default:
		return;
	}
	NET_SendPollPacket (strlen(data), data, server->adr);
}
//send a packet to each server in sequence.
void CL_QueryServers(void)
{
	static int poll;
	int op;
	serverinfo_t *server;
	master_t *mast;

	Master_DetermineMasterTypes();

	op = poll;

	for (mast = master; mast; mast=mast->next)
	{
		switch (mast->protocoltype)
		{
		case MP_UNSPECIFIED:
			continue;
		case MP_DPMASTER:	//dpmaster allows the client to specify the protocol to query. this means it always matches the current game type, so don't bother allowing the user to disable it.
			if (!sb_enabledarkplaces)
				continue;
			break;
		case MP_NETQUAKE:
			if (!sb_enablenetquake)
				continue;
			break;
		case MP_QUAKEWORLD:
			if (!sb_enablequakeworld)
				continue;
			break;
		case MP_QUAKE2:
			if (!sb_enablequake2)
				continue;
			break;
		case MP_QUAKE3:
			if (!sb_enablequake3)
				continue;
			break;
		}

		if (mast->sends > 0)
			MasterInfo_Request(mast);
	}


	for (server = firstserver; op>0 && server; server=server->next, op--);

	if (!server)
	{
		poll = 0;
		return;
	}

	if (op == 0)
	{

		//we only want to send poll packets to servers which will not be filtered (otherwise it's pointless)
		while(server)
		{
			qboolean enabled;
			switch(server->special & SS_PROTOCOLMASK)
			{
			case SS_UNKNOWN: enabled = true; break;
			case SS_QUAKE3: enabled = sb_enablequake3; break;
			case SS_QUAKE2: enabled = sb_enablequake2; break;
			case SS_NETQUAKE: enabled = sb_enablenetquake; break;
			case SS_QUAKEWORLD: enabled = sb_enablequakeworld; break;
			case SS_DARKPLACES: enabled = sb_enabledarkplaces; break;
			default: enabled = false; break;
			}
			if (enabled)
				break;
			server = server->next;
			poll++;
		}
		if (!server)
		{
			server = firstserver;
			while (server)
			{
				qboolean enabled;
				switch(server->special & SS_PROTOCOLMASK)
				{
				case SS_UNKNOWN: enabled = true; break;
				case SS_QUAKE3: enabled = sb_enablequake3; break;
				case SS_QUAKE2: enabled = sb_enablequake2; break;
				case SS_NETQUAKE: enabled = sb_enablenetquake; break;
				case SS_QUAKEWORLD: enabled = sb_enablequakeworld; break;
				case SS_DARKPLACES: enabled = sb_enabledarkplaces; break;
				default: enabled = false; break;
				}
				if (enabled)
					break;
				server = server->next;
				poll++;
			}

		}
		if (server && server->sends > 0)
		{
			Master_QueryServer(server);
		}
		poll++;
		return;
	}


	poll = 0;
}

unsigned int Master_TotalCount(void)
{
	unsigned int count=0;
	serverinfo_t *info;

	for (info = firstserver; info; info = info->next)
	{
		count++;
	}
	return count;
}

unsigned int Master_NumPolled(void)
{
	unsigned int count=0;
	serverinfo_t *info;

	for (info = firstserver; info; info = info->next)
	{
		if (info->maxplayers)
			count++;
	}
	return count;
}

//true if server is on a different master's list.
serverinfo_t *Master_InfoForServer (netadr_t *addr)
{
	serverinfo_t *info;

	for (info = firstserver; info; info = info->next)
	{
		if (NET_CompareAdr(&info->adr, addr))
			return info;
	}
	return NULL;
}
serverinfo_t *Master_InfoForNum (int num)
{
	serverinfo_t *info;

	for (info = firstserver; info; info = info->next)
	{
		if (num-- <=0)
			return info;
	}
	return NULL;
}

void MasterInfo_RemoveAllPlayers(void)
{
	player_t *p;
	while(mplayers)
	{
		p = mplayers;
		mplayers = p->next;
		Z_Free(p);
	}
}
void MasterInfo_RemovePlayers(netadr_t *adr)
{
	player_t *p, *prev;
	prev = NULL;
	for (p = mplayers; p; )
	{
		if (NET_CompareAdr(&p->adr, adr))
		{
			if (prev)
				prev->next = p->next;
			else
				mplayers = p->next;
			Z_Free(p);
			p=prev;

			continue;
		}
		else
			prev = p;

		p = p->next;
	}
}

void MasterInfo_AddPlayer(netadr_t *serveradr, char *name, int ping, int frags, int colours, char *skin)
{
	player_t *p;
	p = Z_Malloc(sizeof(player_t));
	p->next = mplayers;
	p->adr = *serveradr;
	p->colour = colours;
	p->frags = frags;
	Q_strncpyz(p->name, name, sizeof(p->name));
	Q_strncpyz(p->skin, skin, sizeof(p->skin));
	mplayers = p;
}

//we got told about a server, parse it's info
int CL_ReadServerInfo(char *msg, enum masterprotocol_e prototype, qboolean favorite)
{
	serverdetailedinfo_t details;

	char *token;
	char *nl;
	char *name;
	int ping;
	int len;
	serverinfo_t *info;
	char adr[MAX_ADR_SIZE];

	info = Master_InfoForServer(&net_from);

	if (!info)	//not found...
	{
		if (atoi(Info_ValueForKey(msg, "sv_punkbuster")))
			return false;	//never add servers that require punkbuster. :(
//		if (atoi(Info_ValueForKey(msg, "sv_pure")))
//			return false;	//we don't support the filesystem hashing. :(

		info = Z_Malloc(sizeof(serverinfo_t));

		info->adr = net_from;

		snprintf(info->name, sizeof(info->name), "%s", NET_AdrToString(adr, sizeof(adr), &info->adr));

		info->next = firstserver;
		firstserver = info;

		//server replied from a broadcast message, make sure we ping it to retrieve its actual ping
		info->sends = 1;
		info->ping = 0xffff;	//not known
		info->special |= SS_LOCAL;
	}
	else
	{
		MasterInfo_RemovePlayers(&info->adr);

		//determine the ping
		if (info->refreshtime)
		{
			ping = (Sys_DoubleTime() - info->refreshtime)*1000;
			if (ping > 0xfffe)
				info->ping = 0xfffe;	//highest (that is known)
			else
				info->ping = ping;
		}
		info->refreshtime = 0;
	}

	nl = strchr(msg, '\n');
	if (nl)
	{
		*nl = '\0';
		nl++;
	}
	name = Info_ValueForKey(msg, "hostname");
	if (!*name)
		name = Info_ValueForKey(msg, "sv_hostname");
	Q_strncpyz(info->name, name, sizeof(info->name));
	info->special = info->special & (SS_FAVORITE | SS_KEEPINFO | SS_LOCAL);	//favorite+local is never cleared
	if (!strcmp(DISTRIBUTION, Info_ValueForKey(msg, "*distrib")))
		info->special |= SS_FTESERVER;
	else if (!strncmp(DISTRIBUTION, Info_ValueForKey(msg, "*version"), strlen(DISTRIBUTION)))
		info->special |= SS_FTESERVER;

	info->protocol = atoi(Info_ValueForKey(msg, "protocol"));
	info->special &= ~SS_PROTOCOLMASK;
	if (info->protocol)
	{
		switch(info->protocol)
		{
		case PROTOCOL_VERSION_QW:	info->special = SS_QUAKEWORLD;	break;
#ifdef NQPROT
		case NQ_PROTOCOL_VERSION:	info->special = SS_NETQUAKE;	break;
		case H2_PROTOCOL_VERSION:	info->special = SS_NETQUAKE;	break;	//erk
		case NEHD_PROTOCOL_VERSION:	info->special = SS_NETQUAKE;	break;
		case FITZ_PROTOCOL_VERSION:	info->special = SS_NETQUAKE;	break;
		case RMQ_PROTOCOL_VERSION:	info->special = SS_NETQUAKE;	break;
		case DP5_PROTOCOL_VERSION:	info->special = SS_DARKPLACES;	break;	//dp actually says 3... but hey, that's dp being WEIRD.
		case DP6_PROTOCOL_VERSION:	info->special = SS_DARKPLACES;	break;
		case DP7_PROTOCOL_VERSION:	info->special = SS_DARKPLACES;	break;
#endif
		default:
			if (PROTOCOL_VERSION_Q2 >= info->protocol && info->protocol >= PROTOCOL_VERSION_Q2_MIN)
				info->special |= SS_QUAKE2;	//q2 has a range!
			else if (info->protocol > 60)
				info->special |= SS_QUAKE3;
			else
				info->special |= SS_DARKPLACES;
			break;
		}
	}
	else if (prototype == MP_QUAKE2)
		info->special |= SS_QUAKE2;
	else if (prototype == MP_QUAKE3)
		info->special |= SS_QUAKE3;
	else if (prototype == MP_NETQUAKE)
		info->special |= SS_NETQUAKE;
	else
		info->special |= SS_QUAKEWORLD;
	if (favorite)	//was specifically named, not retrieved from a master.
		info->special |= SS_FAVORITE;

	info->players = 0;
	ping = atoi(Info_ValueForKey(msg, "maxclients"));
	if (!ping)
		ping = atoi(Info_ValueForKey(msg, "sv_maxclients"));
	info->maxplayers = bound(0, ping, 255);

	ping = atoi(Info_ValueForKey(msg, "timelimit"));
	info->tl = bound(-327678, ping, 32767);
	ping = atoi(Info_ValueForKey(msg, "fraglimit"));
	info->fl = bound(-32768, ping, 32767);

	if (*Info_ValueForKey(msg, "*qtv") || *Info_ValueForKey(msg, "*QTV"))
		info->special |= SS_PROXY|SS_FTESERVER;	//qtv
	if (!strcmp(Info_ValueForKey(msg, "*progs"), "666") && !strcmp(Info_ValueForKey(msg, "*version"), "2.91"))
		info->special |= SS_PROXY;	//qizmo
	if (!Q_strncmp(Info_ValueForKey(msg, "*version"), "qwfwd", 5))
		info->special |= SS_PROXY;	//qwfwd
	if (!Q_strncasecmp(Info_ValueForKey(msg, "*version"), "qtv ", 4))
		info->special |= SS_PROXY;	//eztv

	token = Info_ValueForKey(msg, "map");
	if (!*token)
		token = Info_ValueForKey(msg, "mapname");
	Q_strncpyz(info->map,		token,	sizeof(info->map));

	token = Info_ValueForKey(msg, "*gamedir");
	if (!*token)
		token = Info_ValueForKey(msg, "gamedir");
	if (!*token)
		token = Info_ValueForKey(msg, "modname");
	Q_strncpyz(info->gamedir,	token,	sizeof(info->gamedir));
	Q_strncpyz(info->qcstatus,		Info_ValueForKey(msg, "qcstatus"),	sizeof(info->qcstatus));
	Q_strncpyz(info->modname,		Info_ValueForKey(msg, "modname"),	sizeof(info->modname));

	info->gameversion = atoi(Info_ValueForKey(msg, "gameversion"));

	info->numbots = atoi(Info_ValueForKey(msg, "bots"));
	info->numhumans = info->players - info->numbots;
	info->freeslots = info->maxplayers - info->players;

	strcpy(details.info, msg);
	msg = msg+strlen(msg)+1;

	info->players=details.numplayers = 0;
	if (!strchr(msg, '\n'))
		info->players = atoi(Info_ValueForKey(details.info, "clients"));
	else
	{
		int clnum;

		for (clnum=0; clnum < MAX_CLIENTS; clnum++)
		{
			nl = strchr(msg, '\n');
			if (!nl)
				break;
			*nl = '\0';

			token = msg;
			if (!token)
				break;
			details.players[clnum].userid = atoi(token);
			token = strchr(token+1, ' ');
			if (!token)
				break;
			details.players[clnum].frags = atoi(token);
			token = strchr(token+1, ' ');
			if (!token)
				break;
			details.players[clnum].time = atoi(token);
			msg = token;
			token = strchr(msg+1, ' ');
			if (!token)	//probably q2 response
			{
				//see if this is actually a Quake2 server.
				token = strchr(msg+1, '\"');
				if (!token)	//it wasn't.
					break;

				details.players[clnum].ping = details.players[clnum].frags;
				details.players[clnum].frags = details.players[clnum].userid;

				msg = strchr(token+1, '\"');
				if (!msg)
					break;
				len = msg - token;
				if (len >= sizeof(details.players[clnum].name))
					len = sizeof(details.players[clnum].name);
				Q_strncpyz(details.players[clnum].name, token+1, len);

				details.players[clnum].skin[0] = '\0';

				details.players[clnum].topc = 0;
				details.players[clnum].botc = 0;
				details.players[clnum].time = 0;
			}
			else	//qw responce
			{
				details.players[clnum].time = atoi(token);
				msg = token;
				token = strchr(msg+1, ' ');
				if (!token)
					break;

				details.players[clnum].ping = atoi(token);

				token = strchr(token+1, '\"');
				if (!token)
					break;
				msg = strchr(token+1, '\"');
				if (!msg)
					break;
				len = msg - token;
				if (len >= sizeof(details.players[clnum].name))
					len = sizeof(details.players[clnum].name);
				Q_strncpyz(details.players[clnum].name, token+1, len);
				details.players[clnum].name[len] = '\0';

				token = strchr(msg+1, '\"');
				if (!token)
					break;
				msg = strchr(token+1, '\"');
				if (!msg)
					break;
				len = msg - token;
				if (len >= sizeof(details.players[clnum].skin))
					len = sizeof(details.players[clnum].skin);
				Q_strncpyz(details.players[clnum].skin, token+1, len);
				details.players[clnum].skin[len] = '\0';

				token = strchr(msg+1, ' ');
				if (!token)
					break;
				details.players[clnum].topc = atoi(token);
				token = strchr(token+1, ' ');
				if (!token)
					break;
				details.players[clnum].botc = atoi(token);
			}

			MasterInfo_AddPlayer(&info->adr, details.players[clnum].name, details.players[clnum].ping, details.players[clnum].frags, details.players[clnum].topc*4 | details.players[clnum].botc, details.players[clnum].skin);

			info->players = ++details.numplayers;

			msg = nl;
			if (!msg)
				break;	//erm...
			msg++;
		}
	}
	if (!info->moreinfo && ((slist_cacheinfo.value == 2 || NET_CompareAdr(&info->adr, &selectedserver.adr)) || (info->special & SS_KEEPINFO)))
		info->moreinfo = Z_Malloc(sizeof(serverdetailedinfo_t));
	if (NET_CompareAdr(&info->adr, &selectedserver.adr))
		selectedserver.detail = info->moreinfo;

	if (info->moreinfo)
		memcpy(info->moreinfo, &details, sizeof(serverdetailedinfo_t));

	return true;
}

//rewrite to scan for existing server instead of wiping all.
void CL_MasterListParse(netadrtype_t adrtype, int type, qboolean slashpad)
{
	serverinfo_t *info;
	serverinfo_t *last, *old;
	int adrlen;

	int p1, p2;
	char adr[MAX_ADR_SIZE];
	int i;

	switch(adrtype)
	{
	case NA_IP:
		adrlen = 4;
		break;
	case NA_IPV6:
		adrlen = 16;
		break;
	case NA_IPX:
		adrlen = 10;
		break;
	default:
		return;
	}

	MSG_ReadByte ();

	last = firstserver;

	while(msg_readcount+1+2 < net_message.cursize)
	{
		if (slashpad)
		{
			switch(MSG_ReadByte())
			{
			case '\\':
				adrtype = NA_IP;
				adrlen = 4;
				break;
			case '/':
				adrtype = NA_IPV6;
				adrlen = 16;
				break;
			default:
				firstserver = last;
				return;
			}
		}

		info = Z_Malloc(sizeof(serverinfo_t));
		info->adr.type = adrtype;
		switch(adrtype)
		{
		case NA_IP:
		case NA_IPV6:
		case NA_IPX:
			//generic fixed-length addresses
			for (i = 0; i < adrlen; i++)
				((qbyte *)&info->adr.address)[i] = MSG_ReadByte();
			break;
		default:
			break;
		}

		p1 = MSG_ReadByte();
		p2 = MSG_ReadByte();
		info->adr.port = htons((unsigned short)((p1<<8)|p2));
		if (!info->adr.port)
		{
			Z_Free(info);
			break;
		}
		if ((old = Master_InfoForServer(&info->adr)))	//remove if the server already exists.
		{
			if ((old->special & (SS_PROTOCOLMASK)) != (type & (SS_PROTOCOLMASK)))
				old->special = type | (old->special & (SS_FAVORITE|SS_LOCAL));
			old->sends = 1;	//reset.
			Z_Free(info);
		}
		else
		{
			info->sends = 1;
			info->special = type;
			info->refreshtime = 0;

			snprintf(info->name, sizeof(info->name), "%s", NET_AdrToString(adr, sizeof(adr), &info->adr));

			info->next = last;
			last = info;

			Master_ResortServer(info);
		}
	}

	firstserver = last;
}

#endif

