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
// cl_main.c  -- client main loop

#include <ctype.h>
#include "quakedef.h"
#include "winquake.h"
#include <sys/types.h>
#include "netinc.h"
#include "cl_master.h"
#include "cl_ignore.h"

#if defined(_WIN32) && !defined(MINGW) && defined(RGLQUAKE)
#define WINAVI
#endif

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

qboolean	noclip_anglehack;		// remnant from old quake


cvar_t	rcon_password = SCVAR("rcon_password", "");

cvar_t	rcon_address = SCVAR("rcon_address", "");

cvar_t	cl_timeout = SCVAR("cl_timeout", "60");

cvar_t	cl_shownet = SCVAR("cl_shownet","0");	// can be 0, 1, or 2

cvar_t	cl_sbar		= SCVARF("cl_sbar", "0", CVAR_ARCHIVE);
cvar_t	cl_hudswap	= SCVARF("cl_hudswap", "0", CVAR_ARCHIVE);
cvar_t	cl_maxfps	= SCVARF("cl_maxfps", "1000", CVAR_ARCHIVE);
cvar_t	cl_nopext	= SCVARF("cl_nopext", "0", CVAR_ARCHIVE);
cvar_t	cl_nolerp	= SCVAR("cl_nolerp", "1");
cvar_t	hud_tracking_show = SCVAR("hud_tracking_show", "1");

cvar_t	cfg_save_name = SCVARF("cfg_save_name", "fte", CVAR_ARCHIVE);

cvar_t	cl_splitscreen = SCVAR("cl_splitscreen", "0");

cvar_t	lookspring = SCVARF("lookspring","0", CVAR_ARCHIVE);
cvar_t	lookstrafe = SCVARF("lookstrafe","0", CVAR_ARCHIVE);
cvar_t	sensitivity = SCVARF("sensitivity","10", CVAR_ARCHIVE);

cvar_t cl_staticsounds = SCVAR("cl_staticsounds", "1");

cvar_t	m_pitch = SCVARF("m_pitch","0.022", CVAR_ARCHIVE);
cvar_t	m_yaw = SCVAR("m_yaw","0.022");
cvar_t	m_forward = SCVAR("m_forward","1");
cvar_t	m_side = SCVAR("m_side","0.8");

cvar_t	entlatency = SCVAR("entlatency", "20");
cvar_t	cl_predict_players = SCVAR("cl_predict_players", "1");
cvar_t	cl_predict_players2 = SCVAR("cl_predict_players2", "1");
cvar_t	cl_solid_players = SCVAR("cl_solid_players", "1");
cvar_t	cl_noblink = SCVAR("cl_noblink", "0");

cvar_t cl_demospeed = SCVAR("cl_demospeed", "1");


cvar_t	cl_indepphysics = SCVAR("cl_indepphysics", "0");

cvar_t  localid = SCVAR("localid", "");

cvar_t	cl_antibunch = SCVAR("cl_antibunch", "0");

cvar_t	r_drawflame = SCVAR("r_drawflame", "1");

static qboolean allowremotecmd = true;

//
// info mirrors
//
cvar_t	password = SCVARF("password",		"",			CVAR_USERINFO);	//this is parhaps slightly dodgy...
cvar_t	spectator = SCVARF("spectator",		"",			CVAR_USERINFO);
cvar_t	name = SCVARF("name",				"unnamed",	CVAR_ARCHIVE | CVAR_USERINFO);
cvar_t	team = SCVARF("team",				"",			CVAR_ARCHIVE | CVAR_USERINFO);
cvar_t	skin = SCVARF("skin",				"",			CVAR_ARCHIVE | CVAR_USERINFO);
cvar_t	model = SCVARF("model",				"",			CVAR_ARCHIVE | CVAR_USERINFO);
cvar_t	topcolor = SCVARF("topcolor",		"",			CVAR_ARCHIVE | CVAR_USERINFO);
cvar_t	bottomcolor = SCVARF("bottomcolor",	"",			CVAR_ARCHIVE | CVAR_USERINFO);
cvar_t	rate = SCVARF("rate",				"2500",		CVAR_ARCHIVE | CVAR_USERINFO);
cvar_t	drate = SCVARF("drate",				"100000",	CVAR_ARCHIVE | CVAR_USERINFO);		// :)
cvar_t	noaim = SCVARF("noaim",				"",			CVAR_ARCHIVE | CVAR_USERINFO);
cvar_t	msg = SCVARF("msg",					"1",		CVAR_ARCHIVE | CVAR_USERINFO);
cvar_t	cl_nofake = SCVAR("cl_nofake",		"2");
cvar_t	cl_chatsound = SCVAR("cl_chatsound","1");
cvar_t cl_enemychatsound = SCVAR("cl_enemychatsound", "misc/talk.wav");
cvar_t cl_teamchatsound = SCVAR("cl_teamchatsound", "misc/talk.wav");

cvar_t	r_rocketlight	= SCVAR("r_rocketlight",	"1");
cvar_t	r_lightflicker	= SCVAR("r_lightflicker",	"1");
cvar_t	cl_r2g			= SCVAR("cl_r2g",	"0");
cvar_t	r_powerupglow	= SCVAR("r_powerupglow", "1");
cvar_t	v_powerupshell	= SCVAR("v_powerupshell", "0");
cvar_t	cl_gibfilter	= SCVAR("cl_gibfilter", "0");
cvar_t	cl_deadbodyfilter	= SCVAR("cl_deadbodyfilter", "0");

cvar_t	allow_download_csprogs = SCVAR("allow_download_csprogs", "0");

cvar_t	cl_muzzleflash = SCVAR("cl_muzzleflash", "1");

cvar_t	cl_item_bobbing = SCVAR("cl_model_bobbing", "0");

cvar_t	requiredownloads = SCVARF("requiredownloads","1", CVAR_ARCHIVE);
cvar_t	cl_standardchat = SCVARF("cl_standardchat", "0", CVAR_ARCHIVE);
cvar_t	msg_filter = SCVAR("msg_filter", "0");	//0 for neither, 1 for mm1, 2 for mm2, 3 for both
cvar_t  cl_standardmsg = SCVARF("cl_standardmsg", "0", CVAR_ARCHIVE);
cvar_t  cl_parsewhitetext = SCVAR("cl_parsewhitetext", "0");

cvar_t	host_mapname = SCVAR("host_mapname", "");

extern cvar_t cl_hightrack;

char cl_screengroup[] = "Screen options";
char cl_controlgroup[] = "client operation options";
char cl_inputgroup[] = "client input controls";
char cl_predictiongroup[] = "Client side prediction";


client_static_t	cls;
client_state_t	cl;

// alot of this should probably be dynamically allocated
entity_state_t	cl_baselines[MAX_EDICTS];
efrag_t			cl_efrags[MAX_EFRAGS];
entity_t		cl_static_entities[MAX_STATIC_ENTITIES];
trailstate_t   *cl_static_emit[MAX_STATIC_ENTITIES];
lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
//lightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
dlight_t		cl_dlights[MAX_DLIGHTS];

// refresh list
// this is double buffered so the last frame
// can be scanned for oldorigins of trailing objects
int				cl_numvisedicts, cl_oldnumvisedicts;
entity_t		*cl_visedicts, *cl_oldvisedicts;
entity_t		cl_visedicts_list[2][MAX_VISEDICTS];

double			connect_time = -1;		// for connection retransmits
int				connect_type = 0;
int				connect_tries = 0;	//increased each try, every fourth trys nq connect packets.

quakeparms_t host_parms;

qboolean	host_initialized;		// true if into command execution
qboolean	nomaster;

double		host_frametime;
double		realtime;				// without any filtering or bounding
double		oldrealtime;			// last frame run
int			host_framecount;

int			host_hunklevel;

qbyte		*host_basepal;
qbyte		*host_colormap;

cvar_t	host_speeds = SCVAR("host_speeds","0");		// set for running times
#ifdef CRAZYDEBUGGING
cvar_t	developer = SCVAR("developer","1");
#else
cvar_t	developer = SCVAR("developer","0");
#endif

int			fps_count;

jmp_buf 	host_abort;

void Master_Connect_f (void);

float	server_version = 0;	// version of server we connected to

char emodel_name[] =
	{ 'e' ^ 0xff, 'm' ^ 0xff, 'o' ^ 0xff, 'd' ^ 0xff, 'e' ^ 0xff, 'l' ^ 0xff, 0 };
char pmodel_name[] =
	{ 'p' ^ 0xff, 'm' ^ 0xff, 'o' ^ 0xff, 'd' ^ 0xff, 'e' ^ 0xff, 'l' ^ 0xff, 0 };
char prespawn_name[] =
	{ 'p'^0xff, 'r'^0xff, 'e'^0xff, 's'^0xff, 'p'^0xff, 'a'^0xff, 'w'^0xff, 'n'^0xff,
		' '^0xff, '%'^0xff, 'i'^0xff, ' '^0xff, '0'^0xff, ' '^0xff, '%'^0xff, 'i'^0xff, 0 };
char modellist_name[] =
	{ 'm'^0xff, 'o'^0xff, 'd'^0xff, 'e'^0xff, 'l'^0xff, 'l'^0xff, 'i'^0xff, 's'^0xff, 't'^0xff,
		' '^0xff, '%'^0xff, 'i'^0xff, ' '^0xff, '%'^0xff, 'i'^0xff, 0 };
char soundlist_name[] =
	{ 's'^0xff, 'o'^0xff, 'u'^0xff, 'n'^0xff, 'd'^0xff, 'l'^0xff, 'i'^0xff, 's'^0xff, 't'^0xff,
		' '^0xff, '%'^0xff, 'i'^0xff, ' '^0xff, '%'^0xff, 'i'^0xff, 0 };


void CL_MakeActive(char *gamename)
{
	cls.state = ca_active;
	if (VID_SetWindowCaption)
		VID_SetWindowCaption(va("FTE %s: %s", gamename, cls.servername));

	SCR_EndLoadingPlaque();

	TP_ExecTrigger("f_spawn");
}
/*
==================
CL_Quit_f
==================
*/
void CL_Quit_f (void)
{
	TP_ExecTrigger("f_quit");
	Cbuf_Execute();

#ifndef CLIENTONLY
	if (!isDedicated)
#endif
	{
		M_Menu_Quit_f ();
		return;
	}
	CL_Disconnect ();
	Sys_Quit ();
}

/*
=======================
CL_Version_f
======================
*/
void CL_Version_f (void)
{
	Con_TPrintf (TLC_VERSIONST, DISTRIBUTION, build_number());

	Con_TPrintf (TL_EXEDATETIME, __DATE__, __TIME__);
}

void CL_ConnectToDarkPlaces(char *challenge, netadr_t adr)
{
	char	data[2048];
	cls.fteprotocolextensions = 0;

	cls.resendinfo = false;

	connect_time = realtime;	// for retransmit requests

	sprintf(data, "%c%c%c%cconnect\\protocol\\darkplaces 3\\challenge\\%s", 255, 255, 255, 255, challenge);

	NET_SendPacket (NS_CLIENT, strlen(data), data, adr);

	cl.splitclients = 0;
}

#ifdef PROTOCOL_VERSION_FTE
unsigned int CL_SupportedFTEExtensions(void)
{
	unsigned int fteprotextsupported = 0;

#ifdef PEXT_SCALE	//dmw - protocol extensions
	fteprotextsupported |= PEXT_SCALE;
#endif
#ifdef PEXT_LIGHTSTYLECOL
	fteprotextsupported |= PEXT_LIGHTSTYLECOL;
#endif
#ifdef PEXT_TRANS
	fteprotextsupported |= PEXT_TRANS;
#endif
#ifdef PEXT_VIEW2
	fteprotextsupported |= PEXT_VIEW2;
#endif
#ifdef PEXT_BULLETENS
	fteprotextsupported |= PEXT_BULLETENS;
#endif
#ifdef PEXT_ACCURATETIMINGS
	fteprotextsupported |= PEXT_ACCURATETIMINGS;
#endif
#ifdef PEXT_ZLIBDL
	fteprotextsupported |= PEXT_ZLIBDL;
#endif
#ifdef PEXT_LIGHTUPDATES
	fteprotextsupported |= PEXT_LIGHTUPDATES;
#endif
#ifdef PEXT_FATNESS
	fteprotextsupported |= PEXT_FATNESS;
#endif
#ifdef PEXT_HLBSP
	fteprotextsupported |= PEXT_HLBSP;
#endif

#ifdef PEXT_Q2BSP
	fteprotextsupported |= PEXT_Q2BSP;
#endif
#ifdef PEXT_Q3BSP
	fteprotextsupported |= PEXT_Q3BSP;
#endif

#ifdef PEXT_TE_BULLET
	fteprotextsupported |= PEXT_TE_BULLET;
#endif
#ifdef PEXT_HULLSIZE
	fteprotextsupported |= PEXT_HULLSIZE;
#endif
#ifdef PEXT_SETVIEW
	fteprotextsupported |= PEXT_SETVIEW;
#endif
#ifdef PEXT_MODELDBL
	fteprotextsupported |= PEXT_MODELDBL;
#endif
#ifdef PEXT_VWEAP
	fteprotextsupported |= PEXT_VWEAP;
#endif
#ifdef PEXT_FLOATCOORDS
	fteprotextsupported |= PEXT_FLOATCOORDS;
#endif
	fteprotextsupported |= PEXT_SPAWNSTATIC2;
	fteprotextsupported |= PEXT_COLOURMOD;
	fteprotextsupported |= PEXT_SPLITSCREEN;
	fteprotextsupported |= PEXT_HEXEN2;
	fteprotextsupported |= PEXT_CUSTOMTEMPEFFECTS;
	fteprotextsupported |= PEXT_256PACKETENTITIES;
	fteprotextsupported |= PEXT_ENTITYDBL;
	fteprotextsupported |= PEXT_ENTITYDBL2;
//	fteprotextsupported |= PEXT_64PLAYERS;
	fteprotextsupported |= PEXT_SHOWPIC;
	fteprotextsupported |= PEXT_SETATTACHMENT;
#ifdef PEXT_CHUNKEDDOWNLOADS
	fteprotextsupported |= PEXT_CHUNKEDDOWNLOADS;
#endif
#ifdef PEXT_CSQC
	fteprotextsupported |= PEXT_CSQC;
#endif
#ifdef PEXT_DPFLAGS
	fteprotextsupported |= PEXT_DPFLAGS;
#endif

	return fteprotextsupported;
}
#endif

/*
=======================
CL_SendConnectPacket

called by CL_Connect_f and CL_CheckResend
======================
*/
void CL_SendConnectPacket (
#ifdef PROTOCOL_VERSION_FTE
						   int ftepext,
#endif
						   int compressioncrc
						  /*, ...*/)	//dmw new parms
{
	netadr_t	adr;
	char	data[2048];
	char playerinfo2[MAX_INFO_STRING];
	double t1, t2;
#ifdef PROTOCOL_VERSION_FTE
	int fteprotextsupported=0;
#endif
	int clients;
	int c;

// JACK: Fixed bug where DNS lookups would cause two connects real fast
//       Now, adds lookup time to the connect time.
//		 Should I add it to realtime instead?!?!

	if (cls.state != ca_disconnected)
		return;

	if (cl_nopext.value)	//imagine it's an unenhanced server
	{
		ftepext = 0;
		compressioncrc = 0;
	}

#ifdef PROTOCOL_VERSION_FTE
	fteprotextsupported = CL_SupportedFTEExtensions();

	fteprotextsupported &= ftepext;

#ifdef Q2CLIENT
	if (cls.protocol != CP_QUAKEWORLD)
		fteprotextsupported = 0;
#endif

	cls.fteprotocolextensions = fteprotextsupported;
#endif

	t1 = Sys_DoubleTime ();

	if (!NET_StringToAdr (cls.servername, &adr))
	{
		Con_TPrintf (TLC_BADSERVERADDRESS);
		connect_time = -1;
		return;
	}

	if (!NET_IsClientLegal(&adr))
	{
		Con_TPrintf (TLC_ILLEGALSERVERADDRESS);
		connect_time = -1;
		return;
	}

	if (adr.port == 0)
		adr.port = BigShort (27500);
	t2 = Sys_DoubleTime ();

	cls.resendinfo = false;

	connect_time = realtime+t2-t1;	// for retransmit requests

	cls.qport = Cvar_VariableValue("qport");

//	Info_SetValueForStarKey (cls.userinfo, "*ip", NET_AdrToString(adr), MAX_INFO_STRING);

	Q_strncpyz(playerinfo2, cls.userinfo, sizeof(playerinfo2)-1);
	Info_SetValueForStarKey (playerinfo2, "name", "Second player", MAX_INFO_STRING);

	clients = 1;
	if (cl_splitscreen.value && (fteprotextsupported & PEXT_SPLITSCREEN))
	{
//		if (adr.type == NA_LOOPBACK)
			clients = cl_splitscreen.value+1;
//		else
//			Con_Printf("Split screens are still under development\n");
	}

	if (clients < 1)
		clients = 1;
	if (clients > MAX_SPLITS)
		clients = MAX_SPLITS;

#ifdef Q2CLIENT
	if (cls.protocol == CP_QUAKE2)	//sorry - too lazy.
		clients = 1;
#endif

#ifdef Q3CLIENT
	if (cls.protocol == CP_QUAKE3)
	{	//q3 requires some very strange things.
		CLQ3_SendConnectPacket(adr);
		return;
	}
#endif

	sprintf(data, "%c%c%c%cconnect", 255, 255, 255, 255);

	if (clients>1)	//splitscreen 'connect' command specifies the number of userinfos sent.
		strcat(data, va("%i", clients));

#ifdef Q2CLIENT
	if (cls.protocol == CP_QUAKE2)
		strcat(data, va(" %i", PROTOCOL_VERSION_Q2));
	else
#endif
		strcat(data, va(" %i", PROTOCOL_VERSION));


	strcat(data, va(" %i %i", cls.qport, cls.challenge));

	//userinfo 0 + zquake extension info.
	strcat(data, va(" \"%s\\*z_ext\\%i\"", cls.userinfo, SUPPORTED_Z_EXTENSIONS));
	for (c = 1; c < clients; c++)
	{
		Info_SetValueForStarKey (playerinfo2, "name", va("%s%i", name.string, c+1), MAX_INFO_STRING);
		strcat(data, va(" \"%s\"", playerinfo2, SUPPORTED_Z_EXTENSIONS));
	}

	strcat(data, "\n");

#ifdef PROTOCOL_VERSION_FTE
	if (ftepext)
		strcat(data, va("0x%x 0x%x\n", PROTOCOL_VERSION_FTE, fteprotextsupported));
#endif

#ifdef HUFFNETWORK
	if (compressioncrc && Huff_CompressionCRC(compressioncrc))
	{
		strcat(data, va("0x%x 0x%x\n", (('H'<<0) + ('U'<<8) + ('F'<<16) + ('F' << 24)), compressioncrc));
		cls.netchan.compress = true;
	}
	else
#endif
		cls.netchan.compress = false;

	NET_SendPacket (NS_CLIENT, strlen(data), data, adr);

	cl.splitclients = 0;
	CL_RegisterSplitCommands();
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out

=================
*/
void CL_CheckForResend (void)
{
	netadr_t	adr;
	char	data[2048];
	double t1, t2;

#ifndef CLIENTONLY
	if (!cls.state && sv.state)
	{
		Q_strncpyz (cls.servername, "internalserver", sizeof(cls.servername));
		cls.state = ca_disconnected;
		switch (svs.gametype)
		{
#ifdef Q3CLIENT
		case GT_QUAKE3:
			cls.protocol = CP_QUAKE3;
			break;
#endif
#ifdef Q2CLIENT
		case GT_QUAKE2:
			cls.protocol = CP_QUAKE2;
			break;
#endif
		default:
			cls.protocol = CP_QUAKEWORLD;
			break;
		}

		CL_FlushClientCommands();	//clear away all client->server clientcommands.

		CL_SendConnectPacket (svs.fteprotocolextensions, false);
		return;
	}
#endif

	if (connect_time == -1)
		return;
	if (cls.state != ca_disconnected)
		return;
	/*
#ifdef NQPROT
	if (connect_type)
	{
		if (!connect_time || !(realtime - connect_time < 5.0))
		{
			connect_time = realtime;
			NQ_BeginConnect(cls.servername);
			NQ_ContinueConnect(cls.servername);
		}
		else
			NQ_ContinueConnect(cls.servername);
		return;
	}
#endif
	*/
	if (connect_time && realtime - connect_time < 5.0)
		return;

	t1 = Sys_DoubleTime ();
	if (!NET_StringToAdr (cls.servername, &adr))
	{
		Con_TPrintf (TLC_BADSERVERADDRESS);
		connect_time = -1;
		return;
	}
	if (!NET_IsClientLegal(&adr))
	{
		Con_TPrintf (TLC_ILLEGALSERVERADDRESS);
		connect_time = -1;
		return;
	}

	if (adr.port == 0)
	{
		if (connect_type)
			adr.port = BigShort (26000);	//assume a different port for nq
		else
			adr.port = BigShort (27500);
	}
	t2 = Sys_DoubleTime ();

	connect_time = realtime+t2-t1;	// for retransmit requests

#ifdef Q3CLIENT
	//Q3 clients send thier cdkey to the q3 authorize server.
	//they send this packet with the challenge.
	//and the server will refuse the client if it hasn't sent it.
	CLQ3_SendAuthPacket(adr);
#endif

#ifdef NQPROT
	if (connect_type || ((connect_tries&3)==3))
	{
		sizebuf_t sb;
		memset(&sb, 0, sizeof(sb));
		sb.data = data;
		sb.maxsize = sizeof(data);

		Con_TPrintf (TLC_CONNECTINGTO, cls.servername);

		MSG_WriteLong(&sb, LongSwap(NETFLAG_CTL | (strlen(NET_GAMENAME_NQ)+7)));
		MSG_WriteByte(&sb, CCREQ_CONNECT);
		MSG_WriteString(&sb, NET_GAMENAME_NQ);
		MSG_WriteByte(&sb, NET_PROTOCOL_VERSION);
		NET_SendPacket (NS_CLIENT, sb.cursize, sb.data, adr);
	}
	else
#endif
	{
		Con_TPrintf (TLC_CONNECTINGTO, cls.servername);
		sprintf (data, "%c%c%c%cgetchallenge\n", 255, 255, 255, 255);
		NET_SendPacket (NS_CLIENT, strlen(data), data, adr);
	}

	connect_tries++;
}

void CL_BeginServerConnect(void)
{
	connect_time = 0;
	connect_type = 0;
	connect_tries = 0;
	CL_CheckForResend();
}
#ifdef NQPROT
void CLNQ_BeginServerConnect(void)
{
	connect_time = 0;
	connect_type = 1;
	connect_tries = 0;
	CL_CheckForResend();
}
#endif
void CL_BeginServerReconnect(void)
{
#ifndef CLIENTONLY
	if (isDedicated)
	{
		Con_TPrintf (TLC_DEDICATEDCANNOTCONNECT);
		return;
	}
#endif
	connect_time = 0;
	CL_CheckForResend();
}
/*
================
CL_Connect_f

================
*/
void CL_Connect_f (void)
{
	char	*server;

	if (Cmd_Argc() != 2)
	{
		Con_TPrintf (TLC_SYNTAX_CONNECT);
		return;
	}

	server = Cmd_Argv (1);

	CL_Disconnect_f ();

	Q_strncpyz (cls.servername, server, sizeof(cls.servername));
	CL_BeginServerConnect();
}

void CL_Join_f (void)
{
	char	*server;

	if (Cmd_Argc() != 2)
	{
		if (cls.state)
		{	//Hmm. This server sucks.
			if (cls.z_ext & Z_EXT_JOIN_OBSERVE)
				Cmd_ForwardToServer();
			else
				Cbuf_AddText("\nspectator 0;reconnect\n", RESTRICT_LOCAL);
			return;
		}
		Con_Printf ("join requires a connection or servername/ip\n");
		return;
	}

	server = Cmd_Argv (1);

	CL_Disconnect_f ();

	Cvar_Set(&spectator, "0");

	Q_strncpyz (cls.servername, server, sizeof(cls.servername));
	CL_BeginServerConnect();
}

void CL_Observe_f (void)
{
	char	*server;

	if (Cmd_Argc() != 2)
	{
		if (cls.state)
		{	//Hmm. This server sucks.
			if (cls.z_ext & Z_EXT_JOIN_OBSERVE)
				Cmd_ForwardToServer();
			else
				Cbuf_AddText("\nspectator 1;reconnect\n", RESTRICT_LOCAL);
			return;
		}
		Con_Printf ("observe requires a connection or servername/ip\n");
		return;
	}

	server = Cmd_Argv (1);

	CL_Disconnect_f ();

	Cvar_Set(&spectator, "1");

	Q_strncpyz (cls.servername, server, sizeof(cls.servername));
	CL_BeginServerConnect();
}

#ifdef NQPROT
void CLNQ_Connect_f (void)
{
	char	*server;

	if (Cmd_Argc() != 2)
	{
		Con_TPrintf (TLC_SYNTAX_CONNECT);
		return;
	}

	server = Cmd_Argv (1);

	CL_Disconnect_f ();

	Q_strncpyz (cls.servername, server, sizeof(cls.servername));
	CLNQ_BeginServerConnect();
}
#endif

#ifdef TCPCONNECT
void CL_TCPConnect_f (void)
{
	char buffer[6];
	int newsocket;
	int len;
	int _true = true;

	float giveuptime;

	char	*server;

	if (Cmd_Argc() != 2)
	{
		Con_TPrintf (TLC_SYNTAX_CONNECT);
		return;
	}

	server = Cmd_Argv (1);

	CL_Disconnect_f ();

	Q_strncpyz (cls.servername, server, sizeof(cls.servername));

	NET_StringToAdr(cls.servername, &cls.sockettcpdest);

	if (cls.sockettcp != INVALID_SOCKET)
		closesocket (cls.sockettcp);
	cls.sockettcp = INVALID_SOCKET;
	cls.tcpinlen = 0;

	newsocket = TCP_OpenStream(cls.sockettcpdest);
	if (newsocket == INVALID_SOCKET)
	{
		//failed
		Con_Printf("Failed to connect, server is either down, firewalled, or on a different port\n");
		return;
	}

	Con_Printf("Waiting for confirmation of server (10 secs)\n");

	giveuptime = Sys_DoubleTime() + 10;

	while(giveuptime > Sys_DoubleTime())
	{
		len = recv(newsocket, buffer, sizeof(buffer), 0);
		if (!strncmp(buffer, "qizmo\n", 6))
		{
			cls.sockettcp = newsocket;
			break;
		}
		SCR_UpdateScreen();
	}

	if (cls.sockettcp == INVALID_SOCKET)
	{
		Con_Printf("Timeout - wrong server type\n");
		closesocket(newsocket);
		return;
	}
	Con_Printf("Confirmed\n");

	send(cls.sockettcp, buffer, sizeof(buffer), 0);
	setsockopt(cls.sockettcp, IPPROTO_TCP, TCP_NODELAY, (char *)&_true, sizeof(_true));

	CL_BeginServerConnect();
}
#endif

/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void CL_Rcon_f (void)
{
	char	message[1024];
	int		i;
	netadr_t	to;

	if (!*rcon_password.string)	//FIXME: this is strange...
	{
		Con_TPrintf (TLC_NORCONPASSWORD);
		return;
	}

	message[0] = 255;
	message[1] = 255;
	message[2] = 255;
	message[3] = 255;
	message[4] = 0;

	strcat (message, "rcon ");

	strcat (message, rcon_password.string);
	strcat (message, " ");

	for (i=1 ; i<Cmd_Argc() ; i++)
	{
		strcat (message, Cmd_Argv(i));
		strcat (message, " ");
	}

	if (cls.state >= ca_connected)
		to = cls.netchan.remote_address;
	else
	{
		if (!strlen(rcon_address.string))
		{
			Con_TPrintf (TLC_NORCONDEST);

			return;
		}
		NET_StringToAdr (rcon_address.string, &to);
	}

	NET_SendPacket (NS_CLIENT, strlen(message)+1, message
		, to);
}


/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState (void)
{
	int			i;
#ifndef CLIENTONLY
#define serverrunning (sv.state != ss_dead)
#define tolocalserver NET_IsLoopBackAddress(cls.netchan.remote_address)
#else
#define serverrunning false
#define tolocalserver false
#define SV_UnspawnServer()
#endif

	CL_AllowIndependantSendCmd(false);	//model stuff could be a problem.

	S_StopAllSounds (true);

	Cvar_ApplyLatches(CVAR_SERVEROVERRIDE);

	Con_DPrintf ("Clearing memory\n");
#ifdef PEXT_BULLETENS
	WipeBulletenTextures ();
#endif
	if (!serverrunning || !tolocalserver)
	{
		if (serverrunning)
			SV_UnspawnServer();
		D_FlushCaches ();
		Mod_ClearAll ();

		if (host_hunklevel)	// FIXME: check this...
			Hunk_FreeToLowMark (host_hunklevel);

		Cvar_ApplyLatches(CVAR_LATCH);
	}

	CL_ClearTEnts ();
	CL_ClearCustomTEnts();
	SCR_ShowPic_Clear();

	if (cl.playernum[0] == -1)
	{	//left over from q2 connect.
		Media_PlayFilm("");
	}

	for (i = 0; i < UPDATE_BACKUP; i++)
	{
		if (cl.frames[i].packet_entities.entities)
			Z_Free(cl.frames[i].packet_entities.entities);
	}

	if (cl.lerpents)
		BZ_Free(cl.lerpents);

	{
		downloadlist_t *next;
		while(cl.downloadlist)
		{
			next = cl.downloadlist->next;
			Z_Free(cl.downloadlist);
			cl.downloadlist = next;
		}
		while(cl.faileddownloads)
		{
			next = cl.faileddownloads->next;
			Z_Free(cl.faileddownloads);
			cl.faileddownloads = next;
		}
	}

// wipe the entire cl structure
	memset (&cl, 0, sizeof(cl));

	SZ_Clear (&cls.netchan.message);

	r_worldentity.model = NULL;

// clear other arrays
	memset (cl_efrags, 0, sizeof(cl_efrags));
	memset (cl_dlights, 0, sizeof(cl_dlights));
	memset (cl_lightstyle, 0, sizeof(cl_lightstyle));

	for (i = 0; i < MAX_EDICTS; i++)
	{
		memcpy(&cl_baselines[i], &nullentitystate, sizeof(cl_baselines[i]));
	}



//
// allocate the efrags and chain together into a free list
//
	cl.free_efrags = cl_efrags;
	for (i=0 ; i<MAX_EFRAGS-1 ; i++)
		cl.free_efrags[i].entnext = &cl.free_efrags[i+1];
	cl.free_efrags[i].entnext = NULL;

	for (i = 0; i < MAX_SPLITS; i++)
		cl.viewheight[i] = DEFAULT_VIEWHEIGHT;
	cl.minpitch = -70;
	cl.maxpitch = 80;

	cl.oldgametime = 0;
	cl.gametime = 0;
	cl.gametimemark = 0;
}

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect (void)
{
	qbyte	final[12];

	connect_time = -1;

	Cvar_ApplyLatches(CVAR_SERVEROVERRIDE);

	if (VID_SetWindowCaption)
		VID_SetWindowCaption(FULLENGINENAME": disconnected");

// stop sounds (especially looping!)
	S_StopAllSounds (true);
#ifdef VM_CG
	CG_Stop();
#endif
#ifdef CSQC_DAT
	CSQC_Shutdown();
#endif
	// if running a local server, shut it down
	if (cls.demoplayback != DPB_NONE)
		CL_StopPlayback ();
	else if (cls.state != ca_disconnected)
	{
		if (cls.demorecording)
			CL_Stop_f ();

		switch(cls.protocol)
		{
#ifdef NQPROT
		case CP_NETQUAKE:
			final[0] = clc_disconnect;
			final[1] = clc_stringcmd;
			strcpy (final+2, "drop");
			Netchan_Transmit (&cls.netchan, strlen(final)+1, final, 250000);
			Netchan_Transmit (&cls.netchan, strlen(final)+1, final, 250000);
			Netchan_Transmit (&cls.netchan, strlen(final)+1, final, 250000);
			break;
#endif
#ifdef Q2CLIENT
		case CP_QUAKE2:
			final[0] = clcq2_stringcmd;
			strcpy (final+1, "disconnect");
			Netchan_Transmit (&cls.netchan, strlen(final)+1, final, 2500);
			Netchan_Transmit (&cls.netchan, strlen(final)+1, final, 2500);
			Netchan_Transmit (&cls.netchan, strlen(final)+1, final, 2500);
			break;

#endif
		case CP_QUAKEWORLD:
			final[0] = clc_stringcmd;
			strcpy (final+1, "drop");
			Netchan_Transmit (&cls.netchan, strlen(final)+1, final, 2500);
			Netchan_Transmit (&cls.netchan, strlen(final)+1, final, 2500);
			Netchan_Transmit (&cls.netchan, strlen(final)+1, final, 2500);
			break;
		}

		cls.state = ca_disconnected;

		cls.demoplayback = DPB_NONE;
		cls.demorecording = cls.timedemo = false;

#ifndef CLIENTONLY
	//running a server, and it's our own
		if (serverrunning && !tolocalserver)
			SV_UnspawnServer();
#endif
	}
	Cam_Reset();

	if (cl.worldmodel)
	{
#if defined(RUNTIMELIGHTING) && defined(RGLQUAKE)
		extern model_t *lightmodel;
		lightmodel = NULL;
#endif

		cl.worldmodel->needload=true;
		cl.worldmodel=NULL;
	}

	if (cls.downloadmethod <= DL_QWPENDING)
		cls.downloadmethod = DL_NONE;
	if (cls.downloadqw)
	{
		VFS_CLOSE(cls.downloadqw);
		cls.downloadqw = NULL;
	}
	if (!cls.downloadmethod)
		*cls.downloadname = '\0';

	{
		downloadlist_t *next;
		while(cl.downloadlist)
		{
			next = cl.downloadlist->next;
			Z_Free(cl.downloadlist);
			cl.downloadlist = next;
		}
		while(cl.faileddownloads)
		{
			next = cl.faileddownloads->next;
			Z_Free(cl.faileddownloads);
			cl.faileddownloads = next;
		}
	}

	COM_FlushTempoaryPacks();

	r_worldentity.model = NULL;
	cl.spectator = 0;
	cl.sendprespawn = false;

#ifdef NQPROT
	cls.signon=0;
#endif
	CL_StopUpload();

#ifndef CLIENTONLY
	if (!isDedicated)
#endif
		SCR_EndLoadingPlaque();

	cls.protocol = CP_UNKNOWN;
	cl.servercount = 0;
	cls.findtrack = false;

#ifdef TCPCONNECT
	if (cls.sockettcp != INVALID_SOCKET)
	{
		closesocket(cls.sockettcp);
		cls.sockettcp = INVALID_SOCKET;
	}
#endif

	cls.qport++;	//a hack I picked up from qizmo
}

#undef serverrunning
#undef tolocalserver

void CL_Disconnect_f (void)
{
#ifndef CLIENTONLY
	if (sv.state)
		SV_UnspawnServer();
#endif

	CL_Disconnect ();

	Alias_WipeStuffedAliaes();
}

/*
====================
CL_User_f

user <name or userid>

Dump userdata / masterdata for a user
====================
*/
void CL_User_f (void)
{
	int		uid;
	int		i;

#ifndef CLIENTONLY
	if (sv.state)
	{
		SV_User_f();
		return;
	}
#endif

	if (Cmd_Argc() != 2)
	{
		Con_TPrintf (TLC_SYNTAX_USER);
		return;
	}

	uid = atoi(Cmd_Argv(1));

	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (!cl.players[i].name[0])
			continue;
		if (cl.players[i].userid == uid
		|| !strcmp(cl.players[i].name, Cmd_Argv(1)) )
		{
			Info_Print (cl.players[i].userinfo);
			return;
		}
	}
	Con_TPrintf (TLC_USER_NOUSER);
}

/*
====================
CL_Users_f

Dump userids for all current players
====================
*/
void CL_Users_f (void)
{
	int		i;
	int		c;

	c = 0;
	Con_TPrintf (TLC_USERBANNER);
	Con_TPrintf (TLC_USERBANNER2);
	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (cl.players[i].name[0])
		{
			Con_TPrintf (TLC_USERLINE, cl.players[i].userid, cl.players[i].frags, cl.players[i].name);
			c++;
		}
	}

	Con_TPrintf (TLC_USERTOTAL, c);
}

void CL_Color_f (void)
{
	// just for quake compatability...
	int		top, bottom;
	char	num[16];

	qboolean server_owns_colour;

	if (Cmd_Argc() == 1)
	{
		Con_TPrintf (TLC_COLOURCURRENT,
			Info_ValueForKey (cls.userinfo, "topcolor"),
			Info_ValueForKey (cls.userinfo, "bottomcolor") );
		Con_TPrintf (TLC_SYNTAX_COLOUR);
		return;
	}

	if (Cmd_FromGamecode())
		server_owns_colour = true;
	else
		server_owns_colour = false;


	if (Cmd_Argc() == 2)
		top = bottom = atoi(Cmd_Argv(1));
	else
	{
		top = atoi(Cmd_Argv(1));
		bottom = atoi(Cmd_Argv(2));
	}

	top &= 15;
	if (top > 13)
		top = 13;
	bottom &= 15;
	if (bottom > 13)
		bottom = 13;

	sprintf (num, "%i", top);
	if (top == 0)
		*num = '\0';
	if (Cmd_ExecLevel>RESTRICT_SERVER) //colour command came from server for a split client
		Cbuf_AddText(va("cmd %i setinfo topcolor %i\n", Cmd_ExecLevel-RESTRICT_SERVER-1, top), Cmd_ExecLevel);
//	else if (server_owns_colour)
//		Cvar_LockFromServer(&topcolor, num);
	else
		Cvar_Set (&topcolor, num);
	sprintf (num, "%i", bottom);
	if (bottom == 0)
		*num = '\0';
	if (Cmd_ExecLevel>RESTRICT_SERVER) //colour command came from server for a split client
		Cbuf_AddText(va("cmd %i setinfo bottomcolor %i\n", Cmd_ExecLevel-RESTRICT_SERVER-1, bottom), Cmd_ExecLevel);
	else if (server_owns_colour)
		Cvar_LockFromServer(&bottomcolor, num);
	else
		Cvar_Set (&bottomcolor, num);
#ifdef NQPROT
	if (cls.protocol == CP_NETQUAKE)
		Cmd_ForwardToServer();
#endif
}


void CL_CheckServerInfo(void)
{
	char *s;
	unsigned int allowed;
	int oldstate;
	qboolean oldallowshaders;

	oldallowshaders = cls.allow_shaders;

	cl.teamplay = atoi(Info_ValueForKey(cl.serverinfo, "teamplay"));
	cl.deathmatch = atoi(Info_ValueForKey(cl.serverinfo, "deathmatch"));

	cls.allow_cheats = false;
	cls.allow_semicheats=true;
	cls.allow_rearview=false;
	cls.allow_watervis=false;
	cls.allow_skyboxes=false;
	cls.allow_mirrors=false;
	cls.allow_shaders=false;
	cls.allow_luma=true;
	cls.allow_bump=false;
#ifdef FISH
	cls.allow_fish=false;
#endif
//	cls.allow_fbskins = 0;
//	cls.allow_overbrightlight;
	if (cls.demoplayback || atoi(Info_ValueForKey(cl.serverinfo, "rearview")))
		cls.allow_rearview=true;

	if (cls.demoplayback || atoi(Info_ValueForKey(cl.serverinfo, "watervis")))
		cls.allow_watervis=true;

	if (cls.demoplayback || atoi(Info_ValueForKey(cl.serverinfo, "allow_skybox")) || atoi(Info_ValueForKey(cl.serverinfo, "allow_skyboxes")))
		cls.allow_skyboxes=true;

	if (cls.demoplayback || atoi(Info_ValueForKey(cl.serverinfo, "mirrors")))
		cls.allow_mirrors=true;

	if (cls.demoplayback || atoi(Info_ValueForKey(cl.serverinfo, "allow_shaders")))
		cls.allow_shaders=true;

	if (cls.demoplayback || !atoi(Info_ValueForKey(cl.serverinfo, "allow_luma")))
		cls.allow_luma=false;

	if (cls.demoplayback || atoi(Info_ValueForKey(cl.serverinfo, "allow_lmgamma")))
		cls.allow_lightmapgamma=true;

	s = Info_ValueForKey(cl.serverinfo, "allow_bump");
	if (cls.demoplayback || atoi(s) || !*s)	//admin doesn't care.
		cls.allow_bump=true;
#ifdef FISH
	if (cls.demoplayback || atoi(Info_ValueForKey(cl.serverinfo, "allow_fish")))
		cls.allow_fish=true;
#endif

	s = Info_ValueForKey(cl.serverinfo, "fbskins");
	if (cls.demoplayback || *s)
		cls.allow_fbskins = atof(s);
	else
		cls.allow_fbskins = 1;

	s = Info_ValueForKey(cl.serverinfo, "*cheats");
	if (cls.demoplayback || !stricmp(s, "on"))
		cls.allow_cheats = true;

	s = Info_ValueForKey(cl.serverinfo, "strict");
	if (!cls.demoplayback && *s && strcmp(s, "0"))
	{
		cls.allow_semicheats = false;
		cls.allow_cheats	= false;
	}

	cls.allow_shaders = cls.allow_cheats;

	if (cls.demoplayback || atoi(Info_ValueForKey(cl.serverinfo, "allow_shaders")))
		cls.allow_shaders=true;


	cls.maxfps = atof(Info_ValueForKey(cl.serverinfo, "maxfps"));
	if (cls.maxfps < 20)
		cls.maxfps = 72;

	if (!atoi(Info_ValueForKey(cl.serverinfo, "deathmatch")))
		cls.gamemode = GAME_COOP;
	else
		cls.gamemode = GAME_DEATHMATCH;

	cls.z_ext = atoi(Info_ValueForKey(cl.serverinfo, "*z_ext"));

	// movement vars for prediction
	cl.bunnyspeedcap = Q_atof(Info_ValueForKey(cl.serverinfo, "pm_bunnyspeedcap"));
	movevars.slidefix = (Q_atof(Info_ValueForKey(cl.serverinfo, "pm_slidefix")) != 0);
	movevars.airstep = (Q_atof(Info_ValueForKey(cl.serverinfo, "pm_airstep")) != 0);
	movevars.walljump = (Q_atof(Info_ValueForKey(cl.serverinfo, "pm_walljump")));
	movevars.ktjump = Q_atof(Info_ValueForKey(cl.serverinfo, "pm_ktjump"));

	// Initialize cl.maxpitch & cl.minpitch
	s = (cls.z_ext & Z_EXT_PITCHLIMITS) ? Info_ValueForKey (cl.serverinfo, "maxpitch") : "";
	cl.maxpitch = *s ? Q_atof(s) : 80.0f;
	s = (cls.z_ext & Z_EXT_PITCHLIMITS) ? Info_ValueForKey (cl.serverinfo, "minpitch") : "";
	cl.minpitch = *s ? Q_atof(s) : -70.0f;

	allowed = atoi(Info_ValueForKey(cl.serverinfo, "allow"));
	if (allowed & 1)
		cls.allow_watervis = true;
	if (allowed & 2)
		cls.allow_rearview = true;
	if (allowed & 4)
		cls.allow_skyboxes = true;
	if (allowed & 8)
		cls.allow_mirrors = true;
	if (allowed & 16)
		cls.allow_shaders = true;
	if (allowed & 32)
		cls.allow_luma = true;
	if (allowed & 64)
		cls.allow_bump = true;
#ifdef FISH
	if (allowed & 128)
		cls.allow_fish = true;
#endif
	if (allowed & 256)
		cls.allow_lightmapgamma = true;
	if (allowed & 512)
		cls.allow_cheats = true;

	if (cls.allow_semicheats)
		cls.allow_anyparticles = true;
	else
		cls.allow_anyparticles = false;


	s = Info_ValueForKey(cl.serverinfo, "status");
	oldstate = cl.ktprostate;
	if (!stricmp(s, "standby"))
		cl.ktprostate = KTPRO_STANDBY;
	else if (!stricmp(s, "countdown"))
		cl.ktprostate = KTPRO_COUNTDOWN;
	else
		cl.ktprostate = KTPRO_DONTKNOW;
	if (oldstate != cl.ktprostate)
		cl.ktprogametime = 0;

	Cvar_ForceCheatVars(cls.allow_semicheats, cls.allow_cheats);


	if (oldallowshaders != cls.allow_shaders)
		Cache_Flush();	//this will cause all models to be reloaded.
}
/*
==================
CL_FullServerinfo_f

Sent by server just after the svc_serverdata
==================
*/
void CL_FullServerinfo_f (void)
{
	char *p;
	float v;

	if (!Cmd_FromGamecode())
	{
		Con_Printf("Hey! fullserverinfo is meant to come from the server!\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_TPrintf (TLC_SYNTAX_FULLSERVERINFO);
		return;
	}

	Q_strncpyz (cl.serverinfo, Cmd_Argv(1), sizeof(cl.serverinfo));

	if ((p = Info_ValueForKey(cl.serverinfo, "*version")) && *p) {
		v = Q_atof(p);
		if (v) {
			if (!server_version)
				Con_TPrintf (TLC_SERVER_VERSION, v);
			server_version = v;
		}
	}
	CL_CheckServerInfo();

	cl.gamespeed = atof(Info_ValueForKey(cl.serverinfo, "*gamespeed"))/100.f;
	if (cl.gamespeed < 0.1)
		cl.gamespeed = 1;

	cl.csqcdebug = atoi(Info_ValueForKey(cl.serverinfo, "*csqcdebug"));
}

/*
==================
CL_FullInfo_f

Allow clients to change userinfo
==================
*/
void CL_FullInfo_f (void)
{
	char	key[512];
	char	value[512];
	char	*o;
	char	*s;

	if (Cmd_Argc() != 2)
	{
		Con_TPrintf (TLC_SYNTAX_FULLINFO);
		return;
	}

	s = Cmd_Argv(1);
	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (!*s)
		{
			Con_TPrintf (TL_KEYHASNOVALUE);
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;

		if (!stricmp(key, pmodel_name) || !stricmp(key, emodel_name))
			continue;

		Info_SetValueForKey (cls.userinfo, key, value, MAX_INFO_STRING);
	}
}

/*
==================
CL_SetInfo_f

Allow clients to change userinfo
==================
*/
void CL_SetInfo_f (void)
{
	cvar_t *var;
	if (Cmd_Argc() == 1)
	{
		Info_Print (cls.userinfo);
		return;
	}
	if (Cmd_Argc() != 3)
	{
		Con_TPrintf (TLC_SYNTAX_SETINFO);
		return;
	}
	if (!stricmp(Cmd_Argv(1), pmodel_name) || !strcmp(Cmd_Argv(1), emodel_name))
		return;

	if (Cmd_Argv(1)[0] == '*')
	{
		int i;
		if (!strcmp(Cmd_Argv(1), "*"))
			if (!strcmp(Cmd_Argv(2), ""))
			{	//clear it out
				char *k;
				for(i=0;;)
				{
					k = Info_KeyForNumber(cls.userinfo, i);
					if (!*k)
						break;	//no more.
					else if (*k == '*')
						i++;	//can't remove * keys
					else if ((var = Cvar_FindVar(k)) && var->flags&CVAR_SERVERINFO)
						i++;	//this one is a cvar.
					else
						Info_RemoveKey(cls.userinfo, k);	//we can remove this one though, so yay.
				}

				return;
			}
		Con_TPrintf (TL_STARKEYPROTECTED);
		return;
	}

	var = Cvar_FindVar(Cmd_Argv(1));
	if (var && (var->flags & CVAR_USERINFO))
	{	//get the cvar code to set it. the server might have locked it.
		Cvar_Set(var, Cmd_Argv(2));
		return;
	}

	Info_SetValueForKey (cls.userinfo, Cmd_Argv(1), Cmd_Argv(2), MAX_INFO_STRING);
	if (cls.state >= ca_connected)
	{
#ifdef Q2CLIENT
		if (cls.protocol == CP_QUAKE2 || cls.protocol == CP_QUAKE3)
			cls.resendinfo = true;
		else
#endif
			Cmd_ForwardToServer ();
	}
}

void CL_SaveInfo(vfsfile_t *f)
{
	VFS_WRITE(f, "\n", 1);
	VFS_WRITE(f, "setinfo * \"\"\n", 13);
	Info_WriteToFile(f, cls.userinfo, "setinfo", CVAR_USERINFO);
}

/*
====================
CL_Packet_f

packet <destination> <contents>

Contents allows \n escape character
====================
*/
void CL_Packet_f (void)
{
	char	send[2048];
	int		i, l;
	char	*in, *out;
	netadr_t	adr;

	if (Cmd_Argc() != 3)
	{
		Con_TPrintf (TLC_PACKET_SYNTAX);
		return;
	}

	if (!NET_StringToAdr (Cmd_Argv(1), &adr))
	{
		Con_TPrintf (TLC_BADADDRESS);
		return;
	}

	if (Cmd_FromGamecode())	//some mvd servers stuffcmd a packet command which lets them know which ip the client is from.
	{						//unfortunatly, 50% of servers are badly configured.
		if (adr.type == NA_IP)
			if (adr.address.ip[0] == 127)
			if (adr.address.ip[1] == 0)
			if (adr.address.ip[2] == 0)
			if (adr.address.ip[3] == 1)
			{
				adr.address.ip[0] = cls.netchan.remote_address.address.ip[0];
				adr.address.ip[1] = cls.netchan.remote_address.address.ip[1];
				adr.address.ip[2] = cls.netchan.remote_address.address.ip[2];
				adr.address.ip[3] = cls.netchan.remote_address.address.ip[3];
				adr.port = cls.netchan.remote_address.port;
				Con_Printf (S_WARNING "Server is broken. Trying to send to server instead.\n");

			}

		Con_DPrintf ("Sending realip packet\n");
	}
	cls.lastarbiatarypackettime = Sys_DoubleTime();	//prevent the packet command from causing a reconnect on badly configured mvdsv servers.

	in = Cmd_Argv(2);
	out = send+4;
	send[0] = send[1] = send[2] = send[3] = 0xff;

	l = strlen (in);
	for (i=0 ; i<l ; i++)
	{
		if (in[i] == '\\' && in[i+1] == 'n')
		{
			*out++ = '\n';
			i++;
		}
		else
			*out++ = in[i];
	}
	*out = 0;

#ifdef TCPCONNECT
	{
		int tcpsock;	//extra code to stop the packet command from sending to the server via tcp
		tcpsock = cls.sockettcp;
		cls.sockettcp = -1;
		NET_SendPacket (NS_CLIENT, out-send, send, adr);
		cls.sockettcp = tcpsock;
	}
#else
	NET_SendPacket (NS_CLIENT, out-send, send, adr);
#endif
}


/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo (void)
{
	char	str[1024];

	if (cls.demonum == -1)
		return;		// don't play demos

	if (!cls.demos[cls.demonum][0] || cls.demonum == MAX_DEMOS)
	{
		cls.demonum = 0;
		if (!cls.demos[cls.demonum][0])
		{
//			Con_Printf ("No demos listed with startdemos\n");
			cls.demonum = -1;
			return;
		}
	}

	sprintf (str,"playdemo %s\n", cls.demos[cls.demonum]);
	Cbuf_InsertText (str, RESTRICT_LOCAL, false);
	cls.demonum++;
}

/*
===============================================================================

DEMO LOOP CONTROL

===============================================================================
*/


/*
==================
CL_Startdemos_f
==================
*/
void CL_Startdemos_f (void)
{
	int		i, c;

	c = Cmd_Argc() - 1;
	if (c > MAX_DEMOS)
	{
		Con_Printf ("Max %i demos in demoloop\n", MAX_DEMOS);
		c = MAX_DEMOS;
	}
	Con_Printf ("%i demo(s) in loop\n", c);

	for (i=1 ; i<c+1 ; i++)
		Q_strncpyz (cls.demos[i-1], Cmd_Argv(i), sizeof(cls.demos[0]));

	if (
#ifndef CLIENTONLY
		!sv.state &&
#endif
		cls.demonum != -1 && cls.demoplayback==DPB_NONE && !Media_PlayingFullScreen() && COM_CheckParm("-demos"))
	{
		cls.demonum = 0;
		CL_NextDemo ();
	}
	else
		cls.demonum = -1;
}


/*
==================
CL_Demos_f

Return to looping demos
==================
*/
void CL_Demos_f (void)
{
	if (cls.demonum == -1)
		cls.demonum = 1;
	CL_Disconnect_f ();
	CL_NextDemo ();
}

/*
==================
CL_Stopdemo_f

stop demo
==================
*/
void CL_Stopdemo_f (void)
{
	if (cls.demoplayback == DPB_NONE)
		return;
	CL_StopPlayback ();
	CL_Disconnect ();
}



/*
=================
CL_Changing_f

Just sent as a hint to the client that they should
drop to full console
=================
*/
void CL_Changing_f (void)
{
	if (cls.downloadqw)  // don't change when downloading
		return;

	SCR_BeginLoadingPlaque();

	S_StopAllSounds (true);
	cl.intermission = 0;
	cls.state = ca_connected;	// not active anymore, but not disconnected
	Con_TPrintf (TLC_CHANGINGMAP);

#ifdef NQPROT
	cls.signon=0;
#endif
}


/*
=================
CL_Reconnect_f

The server is changing levels
=================
*/
void CL_Reconnect_f (void)
{
	if (cls.downloadqw)  // don't change when downloading
		return;
#ifdef NQPROT
	if (cls.protocol == CP_NETQUAKE)
	{
		CL_Changing_f();
		return;
	}
#endif
	S_StopAllSounds (true);

	if (cls.state == ca_connected)
	{
		Con_TPrintf (TLC_RECONNECTING);
		CL_SendClientCommand(true, "new");
		return;
	}

	if (!*cls.servername)
	{
		Con_TPrintf (TLC_RECONNECT_NOSERVER);
		return;
	}

	CL_Disconnect();
	CL_BeginServerReconnect();
}

/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void CL_ConnectionlessPacket (void)
{
	char	*s;
	int		c;
    MSG_BeginReading ();
    MSG_ReadLong ();        // skip the -1

	Cmd_TokenizeString(net_message.data+4, false, false);

	if (net_message.cursize == sizeof(net_message_buffer))
		net_message.data[sizeof(net_message_buffer)-1] = '\0';
	else
		net_message.data[net_message.cursize] = '\0';

#ifdef PLUGINS
	if (Plug_ConnectionlessClientPacket(net_message.data+4, net_message.cursize-4))
		return;
#endif

	c = MSG_ReadByte ();

	if (cls.demoplayback == DPB_NONE)
		Con_TPrintf (TL_ST_COLON, NET_AdrToString (net_from));
//	Con_DPrintf ("%s", net_message.data + 4);

	if (c == S2C_CHALLENGE)
	{
		unsigned long pext = 0, huffcrc=0;
		Con_TPrintf (TLC_S2C_CHALLENGE);

		s = MSG_ReadString ();
		COM_Parse(s);
		if (!strcmp(com_token, "hallengeResponse"))
		{
#ifdef Q3CLIENT
			cls.protocol = CP_QUAKE3;
			cls.challenge = atoi(s+17);
			CL_SendConnectPacket (0, 0/*, ...*/);
			return;
#else
			Con_Printf("\nUnable to connect to Quake3\n");
			return;
#endif
		}
		else if (!strcmp(com_token, "hallenge"))
		{
			char *s2;
			for (s2 = s+9; *s2; s2++)
			{
				if ((*s2 < '0' || *s2 > '9') && *s2 != '-')
					break;
			}
			if (*s2)
			{//and if it's not, we're unlikly to be compatable with whatever it is that's talking at us.
#ifdef NQPROT
				cls.protocol = CP_NETQUAKE;
				CL_ConnectToDarkPlaces(s+9, net_from);
#else
				Con_Printf("\nUnable connect to DarkPlaces\n");
#endif
				return;
			}

#ifdef Q2CLIENT
			cls.protocol = CP_QUAKE2;
#else
			Con_Printf("\nUnable to connect to Quake2\n");
#endif
			s+=9;
		}
#ifdef Q3CLIENT
		else if (!strcmp(com_token, "onnectResponse"))
		{
			goto client_connect;
		}
#endif
#ifdef Q2CLIENT
		else if (!strcmp(com_token, "lient_connect"))
		{
			goto client_connect;
		}
#endif
		else
			cls.protocol = CP_QUAKEWORLD;
		cls.challenge = atoi(s);

		for(;;)
		{
			c = MSG_ReadLong ();
			if (msg_badread)
				break;
			if (c == PROTOCOL_VERSION_FTE)
				pext = MSG_ReadLong ();
#ifdef HUFFNETWORK
			else if (c == (('H'<<0) + ('U'<<8) + ('F'<<16) + ('F' << 24)))
				huffcrc = MSG_ReadLong ();
#endif
			//else if (c == PROTOCOL_VERSION_...)
			else
				MSG_ReadLong ();
		}
		CL_SendConnectPacket (pext, huffcrc/*, ...*/);
		return;
	}
#ifdef Q2CLIENT
	if (cls.protocol == CP_QUAKE2)
	{
		char *nl;
		msg_readcount--;
		c = msg_readcount;
		s = MSG_ReadString ();
		nl = strchr(s, '\n');
		if (nl)
		{
			msg_readcount = c + nl-s + 1;
			*nl = '\0';
		}

		if (!strcmp(s, "print"))
		{
			Con_TPrintf (TLC_A2C_PRINT);

			s = MSG_ReadString ();
			Con_Printf ("%s", s);
			return;
		}
		else if (!strcmp(s, "client_connect"))
		{
			goto client_connect;
		}
		else if (!strcmp(s, "disconnect"))
		{
			if (NET_CompareAdr(net_from, cls.netchan.remote_address))
			{
				Con_Printf ("disconnect\n");
				CL_Disconnect_f();
				return;
			}
			else
			{
				Con_Printf("Ignoring random disconnect command\n");
				return;
			}
		}
		else
		{
			Con_TPrintf (TLC_Q2CONLESSPACKET_UNKNOWN, s);
			msg_readcount = c;
			c = MSG_ReadByte();
		}
	}
#endif

#ifdef NQPROT
	if (c == 'a')
	{
		s = MSG_ReadString ();
		COM_Parse(s);
		if (!strcmp(com_token, "ccept"))
		{
			Con_Printf ("accept\n");
			Netchan_Setup(NS_CLIENT, &cls.netchan, net_from, cls.qport);
			Con_DPrintf ("CL_EstablishConnection: connected to %s\n", cls.servername);


			cls.netchan.isnqprotocol = true;
			cls.protocol = CP_NETQUAKE;

			cls.demonum = -1;			// not in the demo loop now
			cls.state = ca_connected;


			SCR_BeginLoadingPlaque();
			return;
		}
	}
#endif

	if (c == 'd')	//note - this conflicts with qw masters, our browser uses a different socket.
	{
		Con_Printf ("d\n");
		if (cls.demoplayback != DPB_NONE)
		{
			Con_Printf("Disconnect\n");
			CL_Disconnect_f();
		}
		return;
	}

	if (c == S2C_CONNECTION)
	{
		int compress;
#ifdef Q2CLIENT
client_connect:	//fixme: make function
#endif
		Con_TPrintf (TLC_GOTCONNECTION);
		if (cls.state >= ca_connected)
		{
			if (cls.demoplayback == DPB_NONE)
				Con_TPrintf (TLC_DUPCONNECTION);
			return;
		}
		compress = cls.netchan.compress;
		Netchan_Setup (NS_CLIENT, &cls.netchan, net_from, cls.qport);
		cls.netchan.compress = compress;
#ifdef Q3CLIENT
		if (cls.protocol != CP_QUAKE3)
#endif
			CL_SendClientCommand(true, "new");
		cls.state = ca_connected;
		Con_TPrintf (TLC_CONNECTED);
		allowremotecmd = false; // localid required now for remote cmds
		return;
	}
	// remote command from gui front end
	if (c == A2C_CLIENT_COMMAND)	//man I hate this.
	{
		char	cmdtext[2048];

		Con_TPrintf (TLC_CONLESS_CONCMD);
		if (net_from.type != net_local_cl_ipadr.type
			|| ((*(unsigned *)net_from.address.ip != *(unsigned *)net_local_cl_ipadr.address.ip) && (*(unsigned *)net_from.address.ip != htonl(INADDR_LOOPBACK))))
		{
			Con_TPrintf (TLC_CMDFROMREMOTE);
			return;
		}
#ifdef _WIN32
		ShowWindow (mainwindow, SW_RESTORE);
		SetForegroundWindow (mainwindow);
#endif
		s = MSG_ReadString ();

		Q_strncpyz(cmdtext, s, sizeof(cmdtext));

		s = MSG_ReadString ();

		while (*s && isspace(*s))
			s++;
		while (*s && isspace(s[strlen(s) - 1]))
			s[strlen(s) - 1] = 0;

		if (!allowremotecmd && (!*localid.string || strcmp(localid.string, s))) {
			if (!*localid.string) {
				Con_TPrintf (TL_LINEBREAK_EQUALS);
				Con_TPrintf (TLC_LOCALID_NOTSET);
				Con_TPrintf (TL_LINEBREAK_EQUALS);
				return;
			}
			Con_TPrintf (TL_LINEBREAK_EQUALS);
			Con_TPrintf (TLC_LOCALID_BAD,
				s, localid.string);
			Con_TPrintf (TL_LINEBREAK_EQUALS);
			Cvar_Set(&localid, "");
			return;
		}

		Cbuf_AddText (cmdtext, RESTRICT_SERVER);
		allowremotecmd = false;
		return;
	}
	// print command from somewhere
	if (c == 'p')
	{
		if (!strncmp(net_message.data+4, "print\n", 6))
		{
			Con_TPrintf (TLC_A2C_PRINT);
			Con_Printf ("%s", net_message.data+10);
			return;
		}
	}
	if (c == A2C_PRINT)
	{
		Con_TPrintf (TLC_A2C_PRINT);

		s = MSG_ReadString ();
		Con_Printf ("%s", s);
		return;
	}
	if (c == 'r')//dp's reject
	{
		s = MSG_ReadString ();
		Con_Printf("r%s\n", s);
		return;
	}


	// ping from somewhere
	if (c == A2A_PING)
	{
		char	data[6];

		Con_TPrintf (TLC_A2A_PING);

		data[0] = 0xff;
		data[1] = 0xff;
		data[2] = 0xff;
		data[3] = 0xff;
		data[4] = A2A_ACK;
		data[5] = 0;

		NET_SendPacket (NS_CLIENT, 6, &data, net_from);
		return;
	}

//happens in demos
	if (c == svc_disconnect && cls.demoplayback != DPB_NONE)
	{
		Host_EndGame ("End of Demo");
		return;
	}

	Con_TPrintf (TLC_CONLESSPACKET_UNKNOWN, c);
}

#ifdef NQPROT
void CLNQ_ConnectionlessPacket(void)
{
	char *s;
	int length;

	MSG_BeginReading ();
	length = LongSwap(MSG_ReadLong ());
	if (!(length & NETFLAG_CTL))
		return;	//not an nq control packet.
	length &= NETFLAG_LENGTH_MASK;
	if (length != net_message.cursize)
		return;	//not an nq packet.

	switch(MSG_ReadByte())
	{
	case CCREP_ACCEPT:
		if (cls.state >= ca_connected)
		{
			if (cls.demoplayback == DPB_NONE)
				Con_TPrintf (TLC_DUPCONNECTION);
			return;
		}
		net_from.port = htons((short)MSG_ReadLong());

		if (MSG_ReadByte() == 1)	//a proquake server adds a little extra info
		{
			int ver = MSG_ReadByte();
			Con_Printf("ProQuake server %i.%i\n", ver/10, ver%10);

			if (MSG_ReadByte() == 1)
			{
				Con_Printf("ProQuake sucks\nGo play on a decent server.\n");
				return;
			}
		}

		Netchan_Setup (NS_CLIENT, &cls.netchan, net_from, cls.qport);
		cls.netchan.isnqprotocol = true;
		cls.netchan.compress = 0;
		cls.protocol = CP_NETQUAKE;
		cls.state = ca_connected;
		Con_TPrintf (TLC_CONNECTED);
		allowremotecmd = false; // localid required now for remote cmds

		//send a dummy packet.
		//this makes our local nat think we initialised the conversation.
		NET_SendPacket(NS_CLIENT, 0, "", net_from);
		return;

	case CCREP_REJECT:
		s = MSG_ReadString();
		Con_Printf("Connect failed\n%s\n", s);
		return;
	}
}
#endif

/*
=================
CL_ReadPackets
=================
*/
void CL_ReadPackets (void)
{
//	while (NET_GetPacket ())
	for(;;)
	{
		if (cl.oldgametime && cl_antibunch.value)
		{
			float want;
			static float clamp;

			want = cl.oldgametime + realtime - cl.gametimemark - clamp;
			if (want>cl.time)	//don't decrease
			{
				clamp = 0;
				cl.time = want;
			}

			if (cl.time > cl.gametime)
			{
				clamp += cl.time - cl.gametime;
				cl.time = cl.gametime;
			}
			if (cl.time < cl.oldgametime)
			{
				clamp -= cl.time - cl.gametime;
				cl.time = cl.oldgametime;
			}

			if (cl.time < cl.gametime-(1/cl_antibunch.value))
			{
//				if (cl.gametime - 2 > cl.time)
//					cl.gametime = 0;
				break;
			}

		}

		if (!CL_GetMessage())
			break;

#ifdef NQPROT
		if (cls.demoplayback == DPB_NETQUAKE)
		{
			MSG_BeginReading ();
			cls.netchan.last_received = realtime;
			CLNQ_ParseServerMessage ();

			if (!cls.demoplayback)
				CL_NextDemo();
			continue;
		}
#endif
#ifdef Q2CLIENT
		if (cls.demoplayback == DPB_QUAKE2)
		{
			MSG_BeginReading ();
			cls.netchan.last_received = realtime;
			CLQ2_ParseServerMessage ();
			continue;
		}
#endif
		//
		// remote command packet
		//
		if (*(int *)net_message.data == -1)
		{
			CL_ConnectionlessPacket ();
			continue;
		}

		if (net_message.cursize < 6 && cls.demoplayback != DPB_MVD) //MVDs don't have the whole sequence header thing going on
		{
			Con_TPrintf (TL_RUNTPACKET,NET_AdrToString(net_from));
			continue;
		}

		if (cls.state == ca_disconnected)
		{	//connect to nq servers, but don't get confused with sequenced packets.
#ifdef NQPROT
			CLNQ_ConnectionlessPacket ();
#endif
			continue;	//ignore it. We arn't connected.
		}

		//
		// packet from server
		//
		if (!cls.demoplayback &&
			!NET_CompareAdr (net_from, cls.netchan.remote_address))
		{
			Con_DPrintf ("%s:sequenced packet from wrong server\n"
				,NET_AdrToString(net_from));
			continue;
		}

		switch(cls.protocol)
		{
#ifdef Q3CLIENT
		case CP_QUAKE3:
			CLQ3_ParseServerMessage();
			break;
#endif
#ifdef NQPROT
		case CP_NETQUAKE:
			switch(NQNetChan_Process(&cls.netchan))
			{
			case 0:
				break;
			case 1://datagram
//				if (cls.n
				cls.netchan.incoming_sequence = cls.netchan.outgoing_sequence - 3;
			case 2://reliable
				CLNQ_ParseServerMessage ();
				break;
			}
			break;
#endif
#ifdef Q2CLIENT
		case CP_QUAKE2:
			if (!Netchan_Process(&cls.netchan))
				continue;		// wasn't accepted for some reason
			CLQ2_ParseServerMessage ();
			break;
#endif
		case CP_QUAKEWORLD:
			if (cls.demoplayback == DPB_MVD)
			{
				MSG_BeginReading();
				cls.netchan.last_received = realtime;
			}
			else if (!Netchan_Process(&cls.netchan))
				continue;		// wasn't accepted for some reason
			CL_ParseServerMessage ();
			break;
		}

//		if (cls.demoplayback && cls.state >= ca_active && !CL_DemoBehind())
//			return;
	}

	//
	// check timeout
	//
	if (cls.state >= ca_connected
	 && realtime - cls.netchan.last_received > cl_timeout.value)
	{
		Con_TPrintf (TLC_SERVERTIMEOUT);
		CL_Disconnect ();
		return;
	}

	if (cls.demoplayback == DPB_MVD)
		MVD_Interpolate();
}

//=============================================================================

/*
=====================
CL_Download_f
=====================
*/
void CL_Download_f (void)
{
//	char *p, *q;
	char *url;

	url = Cmd_Argv(1);

#ifdef WEBCLIENT
	if (!strnicmp(url, "http://", 7) || !strnicmp(url, "ftp://", 6))
	{
		if (Cmd_IsInsecure())
			return;
		HTTP_CL_Get(url, Cmd_Argv(2), NULL);//"test.txt");
		return;
	}
#endif

	if (!strnicmp(url, "qw://", 5) || !strnicmp(url, "q2://", 5))
	{
		url += 5;
	}

	if (cls.state == ca_disconnected || cls.demoplayback)
	{
		Con_TPrintf (TLC_CONNECTFIRST);
		return;
	}

	if (cls.netchan.remote_address.type == NA_LOOPBACK)
	{
		Con_TPrintf (TLC_CONNECTFIRST);
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Con_TPrintf (TLC_SYNTAX_DOWNLOAD);
		return;
	}

	if (Cmd_IsInsecure())	//mark server specified downloads.
	{
		if (!strnicmp(url, "game", 4) || !stricmp(url, "progs.dat") || !stricmp(url, "menu.dat") || !stricmp(url, "csprogs.dat") || !stricmp(url, "qwprogs.dat") || strstr(url, "..") || strstr(url, ".dll") || strstr(url, ".so"))
		{	//yes, I know the user can use a different progs from the one that is specified. If you leave it blank there will be no problem. (server isn't allowed to stuff progs cvar)
			Con_Printf("Ignoring stuffed download of \"%s\" due to possible security risk\n", url);
			return;
		}

		CL_CheckOrEnqueDownloadFile(url, url);
		return;
	}

	CL_EnqueDownload(url, url, true, true);

	/*
	strcpy(cls.downloadname, url);

	_snprintf (cls.downloadname, sizeof(cls.downloadname), "%s/%s", com_gamedir, url);

	p = cls.downloadname;
	for (;;)
	{
		if ((q = strchr(p, '/')) != NULL)
		{
			*q = 0;
			Sys_mkdir(cls.downloadname);
			*q = '/';
			p = q + 1;
		} else
			break;
	}

	COM_StripExtension(cls.downloadname, cls.downloadtempname);
	COM_DefaultExtension(cls.downloadtempname, ".tmp");
	if (cls.down
//	cls.downloadqw = fopen (cls.downloadname, "wb");
	cls.downloadmethod = DL_QWPENDING;


	CL_SendClientCommand("download %s\n",url);*/
}

#ifdef _WINDOWS
#include <windows.h>
/*
=================
CL_Minimize_f
=================
*/
void CL_Windows_f (void)
{
	if (!mainwindow)
	{
		Con_Printf("Cannot comply\n");
		return;
	}
//	if (modestate == MS_WINDOWED)
//		ShowWindow(mainwindow, SW_MINIMIZE);
//	else
		SendMessage(mainwindow, WM_SYSKEYUP, VK_TAB, 1 | (0x0F << 16) | (1<<29));
}
#endif

#ifndef CLIENTONLY
void CL_ServerInfo_f(void)
{
	if (!sv.state && cls.state)
	{
		if (cls.demoplayback)
		{
			Info_Print (cl.serverinfo);
		}
		else
			Cmd_ForwardToServer ();
	}
	else
	{
		SV_Serverinfo_f();	//allow it to be set... (whoops)
	}
}
#endif


#ifdef WEBCLIENT
void CL_FTP_f(void)
{
	FTP_Client_Command(Cmd_Args(), NULL);
}
#endif

void SCR_ShowPic_Script_f(void);
/*
=================
CL_Init
=================
*/
void CL_Init (void)
{
	extern void CL_Say_f (void);
	extern void CL_SayMe_f (void);
	extern void CL_SayTeam_f (void);
	extern	cvar_t		baseskin;
	extern	cvar_t		noskins;
	char st[80];

	cls.state = ca_disconnected;

	sprintf (st, "%s %i", DISTRIBUTION, build_number());
	Info_SetValueForStarKey (cls.userinfo, "*ver", st, MAX_INFO_STRING);

	InitValidation();

	CL_InitInput ();
	CL_InitTEnts ();
	CL_InitPrediction ();
	CL_InitCam ();
	PM_Init ();
	TP_Init();

//
// register our commands
//
	CLSCR_Init();
#ifdef MENU_DAT
	MP_RegisterCvarsAndCmds();
#endif
#ifdef CSQC_DAT
	CSQC_RegisterCvarsAndThings();
#endif
	Cvar_Register (&host_speeds, cl_controlgroup);
	Cvar_Register (&developer, cl_controlgroup);

	Cvar_Register (&cfg_save_name, cl_controlgroup);

	cl_demospeed.name2 = "demo_setspeed";
	Cvar_Register (&cl_demospeed, "Demo playback");
	Cvar_Register (&cl_warncmd, "Warnings");
	Cvar_Register (&cl_upspeed, cl_inputgroup);
	Cvar_Register (&cl_forwardspeed, cl_inputgroup);
	Cvar_Register (&cl_backspeed, cl_inputgroup);
	Cvar_Register (&cl_sidespeed, cl_inputgroup);
	Cvar_Register (&cl_movespeedkey, cl_inputgroup);
	Cvar_Register (&cl_yawspeed, cl_inputgroup);
	Cvar_Register (&cl_pitchspeed, cl_inputgroup);
	Cvar_Register (&cl_anglespeedkey, cl_inputgroup);
	Cvar_Register (&cl_shownet,	cl_screengroup);
	Cvar_Register (&cl_sbar,	cl_screengroup);
	Cvar_Register (&cl_hudswap,	cl_screengroup);
	Cvar_Register (&cl_maxfps,	cl_screengroup);
	Cvar_Register (&cl_timeout, cl_controlgroup);
	Cvar_Register (&lookspring, cl_inputgroup);
	Cvar_Register (&lookstrafe, cl_inputgroup);
	Cvar_Register (&sensitivity, cl_inputgroup);

	Cvar_Register (&m_pitch, cl_inputgroup);
	Cvar_Register (&m_yaw, cl_inputgroup);
	Cvar_Register (&m_forward, cl_inputgroup);
	Cvar_Register (&m_side, cl_inputgroup);

	Cvar_Register (&rcon_password,	cl_controlgroup);
	Cvar_Register (&rcon_address,	cl_controlgroup);

	Cvar_Register (&entlatency,	cl_predictiongroup);
	Cvar_Register (&cl_predict_players2,	cl_predictiongroup);
	Cvar_Register (&cl_predict_players,	cl_predictiongroup);
	Cvar_Register (&cl_solid_players,	cl_predictiongroup);

	Cvar_Register (&localid,	cl_controlgroup);

	Cvar_Register (&cl_muzzleflash, cl_controlgroup);

	Cvar_Register (&baseskin,	"Teamplay");
	Cvar_Register (&noskins,	"Teamplay");
	Cvar_Register (&cl_noblink,	"Console controls");	//for lack of a better group

	Cvar_Register (&cl_item_bobbing, "Item effects");

	Cvar_Register (&cl_staticsounds, "Item effects");

	Cvar_Register (&r_rocketlight, "Item effects");
	Cvar_Register (&r_lightflicker, "Item effects");
	Cvar_Register (&cl_r2g, "Item effects");
	Cvar_Register (&r_powerupglow, "Item effects");
	Cvar_Register (&v_powerupshell, "Item effects");

	Cvar_Register (&cl_gibfilter, "Item effects");
	Cvar_Register (&cl_deadbodyfilter, "Item effects");

	Cvar_Register (&cl_nolerp, "Item effects");

	Cvar_Register (&r_drawflame, "Item effects");

	Cvar_Register (&allow_download_csprogs, cl_controlgroup);

	//
	// info mirrors
	//
	Cvar_Register (&name,	cl_controlgroup);
	Cvar_Register (&password,	cl_controlgroup);
	Cvar_Register (&spectator,	cl_controlgroup);
	Cvar_Register (&skin,	cl_controlgroup);
	Cvar_Register (&model,	cl_controlgroup);
	Cvar_Register (&team,	cl_controlgroup);
	Cvar_Register (&topcolor,	cl_controlgroup);
	Cvar_Register (&bottomcolor,	cl_controlgroup);
	Cvar_Register (&rate,	cl_controlgroup);
	Cvar_Register (&drate,	cl_controlgroup);
	Cvar_Register (&msg,	cl_controlgroup);
	Cvar_Register (&noaim,	cl_controlgroup);

	Cvar_Register (&cl_nofake,	cl_controlgroup);
	Cvar_Register (&cl_chatsound,	cl_controlgroup);
	Cvar_Register (&cl_enemychatsound,	cl_controlgroup);
	Cvar_Register (&cl_teamchatsound,	cl_controlgroup);

	Cvar_Register (&requiredownloads,	cl_controlgroup);
	Cvar_Register (&cl_standardchat,	cl_controlgroup);
	Cvar_Register (&msg_filter,	cl_controlgroup);
	Cvar_Register (&cl_standardmsg,		cl_controlgroup);
	Cvar_Register (&cl_parsewhitetext,	cl_controlgroup);
	Cvar_Register (&cl_nopext, cl_controlgroup);
	Cvar_Register (&cl_splitscreen, cl_controlgroup);

	host_mapname.name2 = "mapname";
	Cvar_Register (&host_mapname,		"Scripting");

	Cvar_Register (&cl_indepphysics, cl_controlgroup);
	Cvar_Register (&cl_antibunch, "evil hacks");
	Cvar_Register (&hud_tracking_show, "statusbar");

#ifdef WEBCLIENT
	Cmd_AddCommand ("ftp", CL_FTP_f);
#endif

	Cmd_AddCommand ("version", CL_Version_f);

	Cmd_AddCommand ("changing", CL_Changing_f);
	Cmd_AddCommand ("disconnect", CL_Disconnect_f);
	Cmd_AddCommand ("record", CL_Record_f);
	Cmd_AddCommand ("rerecord", CL_ReRecord_f);
	Cmd_AddCommand ("stop", CL_Stop_f);
	Cmd_AddCommand ("playdemo", CL_PlayDemo_f);
	Cmd_AddCommand ("playqtv", CL_QTVPlay_f);
	Cmd_AddCommand ("demo_jump", CL_DemoJump_f);
	Cmd_AddCommand ("timedemo", CL_TimeDemo_f);

	Cmd_AddCommand ("showpic", SCR_ShowPic_Script_f);

	Cmd_AddCommand ("startdemos", CL_Startdemos_f);
	Cmd_AddCommand ("demos", CL_Demos_f);
	Cmd_AddCommand ("stopdemo", CL_Stopdemo_f);

	Cmd_AddCommand ("skins", Skin_Skins_f);
	Cmd_AddCommand ("allskins", Skin_AllSkins_f);

	Cmd_AddCommand ("quit", CL_Quit_f);

	Cmd_AddCommand ("connect", CL_Connect_f);
#ifdef TCPCONNECT
	Cmd_AddCommand ("tcpconnect", CL_TCPConnect_f);
#endif
#ifdef NQPROT
	Cmd_AddCommand ("nqconnect", CLNQ_Connect_f);
#endif
	Cmd_AddCommand ("reconnect", CL_Reconnect_f);
	Cmd_AddCommand ("join", CL_Join_f);
	Cmd_AddCommand ("observe", CL_Observe_f);

	Cmd_AddCommand ("rcon", CL_Rcon_f);
	Cmd_AddCommand ("packet", CL_Packet_f);
	Cmd_AddCommand ("user", CL_User_f);
	Cmd_AddCommand ("users", CL_Users_f);

	Cmd_AddCommand ("setinfo", CL_SetInfo_f);
	Cmd_AddCommand ("fullinfo", CL_FullInfo_f);
	Cmd_AddCommand ("fullserverinfo", CL_FullServerinfo_f);

	Cmd_AddCommand ("color", CL_Color_f);
	Cmd_AddCommand ("download", CL_Download_f);

	Cmd_AddCommand ("nextul", CL_NextUpload);
	Cmd_AddCommand ("stopul", CL_StopUpload);

//
// forward to server commands
//
	Cmd_AddCommand ("god", NULL);	//cheats
	Cmd_AddCommand ("give", NULL);
	Cmd_AddCommand ("noclip", NULL);
	Cmd_AddCommand ("fly", NULL);
	Cmd_AddCommand ("setpos", NULL);

	Cmd_AddCommand ("topten", NULL);

	Cmd_AddCommand ("kill", NULL);
	Cmd_AddCommand ("pause", NULL);
	Cmd_AddCommand ("say", CL_Say_f);
	Cmd_AddCommand ("me", CL_SayMe_f);
	Cmd_AddCommand ("sayone", CL_Say_f);
	Cmd_AddCommand ("say_team", CL_SayTeam_f);
#ifdef CLIENTONLY
	Cmd_AddCommand ("serverinfo", NULL);
#else
	Cmd_AddCommand ("serverinfo", CL_ServerInfo_f);
#endif

//
//  Windows commands
//
#ifdef _WINDOWS
	Cmd_AddCommand ("windows", CL_Windows_f);
#endif

	Ignore_Init();
}


/*
================
Host_EndGame

Call this to drop to a console without exiting the qwcl
================
*/
void VARGS Host_EndGame (char *message, ...)
{
	va_list		argptr;
	char		string[1024];

	SCR_EndLoadingPlaque();

	va_start (argptr,message);
	vsnprintf (string,sizeof(string)-1, message,argptr);
	va_end (argptr);
	Con_TPrintf (TL_NL);
	Con_TPrintf (TL_LINEBREAK_EQUALS);
	Con_TPrintf (TLC_CLIENTCON_ERROR_ENDGAME, string);
	Con_TPrintf (TL_LINEBREAK_EQUALS);
	Con_TPrintf (TL_NL);

	SCR_EndLoadingPlaque();

	CL_Disconnect ();

	SV_UnspawnServer();

	Cvar_Set(&cl_shownet, "0");

	longjmp (host_abort, 1);
}

/*
================
Host_Error

This shuts down the client and exits qwcl
================
*/
void VARGS Host_Error (char *error, ...)
{
	va_list		argptr;
	char		string[1024];
	static	qboolean inerror = false;

	if (inerror)
		Sys_Error ("Host_Error: recursively entered");
	inerror = true;

	va_start (argptr,error);
	vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);
	Con_TPrintf (TLC_HOSTFATALERROR, string);

	CL_Disconnect ();
	cls.demonum = -1;

	inerror = false;

// FIXME
	Sys_Error ("Host_Error: %s\n",string);
}


/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_WriteConfiguration (void)
{
	vfsfile_t	*f;

	if (host_initialized && cfg_save_name.string && *cfg_save_name.string)
	{
		if (strchr(cfg_save_name.string, '.'))
		{
			Con_TPrintf (TLC_CONFIGCFG_WRITEFAILED);
			return;
		}

		f = FS_OpenVFS(va("%s.cfg",cfg_save_name.string), "wb", FS_GAMEONLY);
		if (!f)
		{
			Con_TPrintf (TLC_CONFIGCFG_WRITEFAILED);
			return;
		}

		Key_WriteBindings (f);
		Cvar_WriteVariables (f, false);

		VFS_CLOSE (f);
	}
}


//============================================================================

#if 0
/*
==================
Host_SimulationTime

This determines if enough time has passed to run a simulation frame
==================
*/
qboolean Host_SimulationTime(float time)
{
	float fps;

	if (oldrealtime > realtime)
		oldrealtime = 0;

	if (cl_maxfps.value)
		fps = max(30.0, min(cl_maxfps.value, 72.0));
	else
		fps = max(30.0, min(rate.value/80.0, 72.0));

	if (!cls.timedemo && (realtime + time) - oldrealtime < 1.0/fps)
		return false;			// framerate is too high
	return true;
}
#endif

/*
==================
Host_Frame

Runs all active servers
==================
*/
#if defined(WINAVI) && !defined(NOMEDIA)
extern float recordavi_frametime;
extern qboolean recordingdemo;
#endif

extern cvar_t cl_netfps;
int		nopacketcount;
void SNDDMA_SetUnderWater(qboolean underwater);
float CL_FilterTime (double time, float wantfps);
void Host_Frame (double time)
{
	static double		time1 = 0;
	static double		time2 = 0;
	static double		time3 = 0;
	int			pass1, pass2, pass3;
//	float fps;
	float realframetime;
	static double spare;

	RSpeedLocals();

	if (setjmp (host_abort) )
		return;			// something bad happened, or the server disconnected

	realframetime = time;

#if defined(WINAVI) && !defined(NOMEDIA)
	if (cls.demoplayback && recordingdemo && recordavi_frametime>0.01)
	{
		realframetime = time = recordavi_frametime;
	}
#endif

//	if (cls.demoplayback && cl_demospeed.value>0)
//		realframetime *= cl_demospeed.value; // this probably screws up other timings

#ifndef CLIENTONLY
	RSpeedRemark();
	SV_Frame();
	RSpeedEnd(RSPEED_SERVER);
#endif
	if (cl.gamespeed<0.1)
		cl.gamespeed = 1;
	time *= cl.gamespeed;

#ifdef WEBCLIENT
	FTP_ClientThink();
	HTTP_CL_Think();
#endif

#ifdef PLUGINS
	Plug_Tick();
#endif

	// decide the simulation time
	realtime += realframetime;
	if (oldrealtime > realtime)
		oldrealtime = 0;

	if (cl.paused)
		cl.gametimemark += time;


#ifdef VOICECHAT
	CLVC_Poll();
#endif

/*
	if (cl_maxfps.value)
		fps = cl_maxfps.value;//max(30.0, min(cl_maxfps.value, 72.0));
	else
		fps = max(30.0, min(rate.value/80.0, 72.0));

	if (!cls.timedemo && realtime - oldrealtime < 1.0/fps)
		return;			// framerate is too high

	*/
	Mod_Think();	//think even on idle (which means small walls and a fast cpu can get more surfaces done.
	if ((cl_maxfps.value>0 && cl_netfps.value>0) || cls.demoplayback)
	{	//limit the fps freely, and expect the netfps to cope.
		if ((realtime - oldrealtime) < 1/cl_maxfps.value)
			return;
	}
	else
	{
		realtime += spare/1000;	//don't use it all!
		spare = CL_FilterTime((realtime - oldrealtime)*1000, (cl_maxfps.value>0||cls.protocol!=CP_QUAKEWORLD)?cl_maxfps.value:cl_netfps.value);
		if (!spare)
			return;
		if (spare < 0 || cls.state < ca_onserver)
			spare = 0;	//uncapped.
		if (spare > 10)
			spare = 10;

		realtime -= spare/1000;	//don't use it all!
	}

	host_frametime = (realtime - oldrealtime)*cl.gamespeed;
	if (!cl.paused)
	{
		cl.ktprogametime += host_frametime;
	}
	oldrealtime = realtime;

	CL_ProgressDemoTime();


#if defined(Q2CLIENT)
	if (cls.protocol == CP_QUAKE2)
		cl.time += host_frametime;
#endif


//	if (host_frametime > 0.2)
//		host_frametime = 0.2;

	// get new key events
	Sys_SendKeyEvents ();

	// allow mice or other external controllers to add commands
	IN_Commands ();

	// process console commands
	Cbuf_Execute ();

#ifndef CLIENTONLY
	if (isDedicated)	//someone changed it.
		return;
#endif

	cls.framecount++;

	RSpeedRemark();

	CL_UseIndepPhysics(!!cl_indepphysics.value);

	CL_AllowIndependantSendCmd(false);

	// fetch results from server
	CL_ReadPackets ();

	CL_AllowIndependantSendCmd(true);

	// send intentions now
	// resend a connection request if necessary
	if (cls.state == ca_disconnected)
	{
		IN_Move(NULL, 0);
		CL_CheckForResend ();
	}
	else
	{
		extern qboolean runningindepphys;
		if (!runningindepphys)
			CL_SendCmd (host_frametime/cl.gamespeed);

		if (cls.state == ca_onserver && cl.validsequence && cl.worldmodel)
		{	// first update is the final signon stage
			CL_MakeActive("QuakeWorld");
		}
	}

	TP_CheckVars();
	RSpeedEnd(RSPEED_PROTOCOL);

	// update video
	if (host_speeds.value)
		time1 = Sys_DoubleTime ();

	if (SCR_UpdateScreen)
	{
		extern mleaf_t	*r_viewleaf;
		extern cvar_t scr_chatmodecvar;

		if (scr_chatmodecvar.value && !cl.intermission)
			scr_chatmode = (cl.spectator&&cl.splitclients<2&&cls.state == ca_active)?2:1;
		else
			scr_chatmode = 0;

		SCR_UpdateScreen ();
		if (cls.state >= ca_active && r_viewleaf)
			SNDDMA_SetUnderWater(r_viewleaf->contents <= Q1CONTENTS_WATER);
		else
			SNDDMA_SetUnderWater(false);
	}

	if (host_speeds.value)
		time2 = Sys_DoubleTime ();

	// update audio
#ifdef CSQC_DAT
	if (CSQC_SettingListener())
		S_ExtraUpdate();
	else
#endif
	if (cls.state == ca_active)
		S_Update (r_origin, vpn, vright, vup);
	else
		S_Update (vec3_origin, vec3_origin, vec3_origin, vec3_origin);

	CDAudio_Update();

	if (host_speeds.value)
	{
		pass1 = (time1 - time3)*1000;
		time3 = Sys_DoubleTime ();
		pass2 = (time2 - time1)*1000;
		pass3 = (time3 - time2)*1000;
		Con_TPrintf (TLC_HOSTSPEEDSOUTPUT,
					pass1+pass2+pass3, pass1, pass2, pass3);
	}

	host_framecount++;
	fps_count++;

	IN_Commands ();

	// process console commands
	Cbuf_Execute ();

	CL_RequestNextDownload();
}

static void simple_crypt(char *buf, int len)
{
	while (len--)
		*buf++ ^= 0xff;
}

void Host_FixupModelNames(void)
{
	simple_crypt(emodel_name, sizeof(emodel_name) - 1);
	simple_crypt(pmodel_name, sizeof(pmodel_name) - 1);
	simple_crypt(prespawn_name,  sizeof(prespawn_name)  - 1);
	simple_crypt(modellist_name, sizeof(modellist_name) - 1);
	simple_crypt(soundlist_name, sizeof(soundlist_name) - 1);
}



#ifdef Q3CLIENT
void CL_ReadCDKey(void)
{	//q3 cdkey
	//you don't need one, just use a server without sv_strictauth set to 0.
	char *buffer;
	buffer = COM_LoadTempFile("q3key");
	if (buffer)	//a cdkey is meant to be 16 chars
	{
		cvar_t *var;
		char *chr;
		for (chr = buffer; *chr; chr++)
		{
			if (*(unsigned char*)chr < ' ')
			{
				*chr = '\0';	//don't get more than one line.
				break;
			}
		}
		var = Cvar_Get("cl_cdkey", buffer, CVAR_LATCH, "Q3 compatability");
	}
}
#endif

//============================================================================


/*
====================
Host_Init
====================
*/
void Host_Init (quakeparms_t *parms)
{
	int i;
	int qrc, hrc, def;

	extern cvar_t	vid_renderer;
	COM_InitArgv (parms->argc, parms->argv);

	if (setjmp (host_abort) )
		Sys_Error("Host_Init: An error occured. Try the -condebug commandline parameter\n");

	if (COM_CheckParm ("-minmemory"))
		parms->memsize = MINIMUM_MEMORY;

	host_parms = *parms;

	if (parms->memsize < MINIMUM_MEMORY)
		Sys_Error ("Only %4.1f megs of memory reported, can't execute game", parms->memsize / (float)0x100000);

	Memory_Init (parms->membase, parms->memsize);

	COM_ParsePlusSets();
	Cbuf_Init ();
	Cmd_Init ();
	V_Init ();
	COM_Init ();
#ifdef Q2BSPS
	CM_Init();
#endif
	Host_FixupModelNames();

	NET_Init ();
	NET_InitClient ();
	Netchan_Init ();
	Renderer_Init();

//	W_LoadWadFile ("gfx.wad");
	Key_Init ();
	Con_Init ();
	M_Init ();

#ifndef _WIN32
	IN_Init ();
	CDAudio_Init ();
//	VID_Init (host_basepal);
//	Draw_Init ();
//	SCR_Init ();
//	R_Init ();

	S_Init ();

	cls.state = ca_disconnected;
	Sbar_Init ();
	CL_Init ();
#else
	S_Init ();

	cls.state = ca_disconnected;
	CDAudio_Init ();
	Sbar_Init ();
	CL_Init ();
	IN_Init ();
#endif
	TranslateInit();
#ifndef CLIENTONLY
	SV_Init(parms);
#endif
#ifdef TEXTEDITOR
	Editor_Init();
#endif

#ifdef PLUGINS
	Plug_Init();
#endif

#ifdef CL_MASTER
	Master_SetupSockets();
#endif

#ifdef Q3CLIENT
	CL_ReadCDKey();
#endif

	//	Con_Printf ("Exe: "__TIME__" "__DATE__"\n");
	Con_TPrintf (TL_HEAPSIZE, parms->memsize/ (1024*1024.0));

	Cbuf_AddText ("cl_warncmd 0\n", RESTRICT_LOCAL);

	Cbuf_AddText ("+mlook\n", RESTRICT_LOCAL);		//fixme: this is bulky, only exec one of these.

	//who should we imitate?
	qrc = COM_FDepthFile("quake.rc", true);	//q1
	hrc = COM_FDepthFile("hexen.rc", true);	//h2
	def = COM_FDepthFile("default.cfg", true);	//q2/q3

	if (qrc <= def && qrc <= hrc && qrc!=0x7fffffff)
		Cbuf_AddText ("exec quake.rc\n", RESTRICT_LOCAL);
	else if (hrc <= def && hrc!=0x7fffffff)
		Cbuf_AddText ("exec hexen.rc\n", RESTRICT_LOCAL);
	else
	{	//they didn't give us an rc file!
		Cbuf_AddText ("bind ~ toggleconsole\n", RESTRICT_LOCAL);	//we expect default.cfg to not exist. :(
		Cbuf_AddText ("exec default.cfg\n", RESTRICT_LOCAL);
		Cbuf_AddText ("exec config.cfg\n", RESTRICT_LOCAL);
		if (COM_FCheckExists ("q3config.cfg"))
			Cbuf_AddText ("exec q3config.cfg\n", RESTRICT_LOCAL);
		Cbuf_AddText ("exec autoexec.cfg\n", RESTRICT_LOCAL);
	}
	Cbuf_AddText ("exec fte.cfg\n", RESTRICT_LOCAL);
	Cbuf_AddText ("cl_warncmd 1\n", RESTRICT_LOCAL);	//and then it's allowed to start moaning.

	Hunk_AllocName (0, "-HOST_HUNKLEVEL-");
	host_hunklevel = Hunk_LowMark ();

	R_SetRenderer(-1);//set the renderer stuff to unset...

	host_initialized = true;

	Cbuf_Execute ();	//if the server initialisation causes a problem, give it a place to abort to

	Cmd_StuffCmds();

	Cbuf_Execute ();	//if the server initialisation causes a problem, give it a place to abort to


	//assuming they didn't use any waits in thier config (fools)
	//the configs should be fully loaded.
	//so convert the backwards compable commandline parameters in cvar sets.

	if (COM_CheckParm ("-window") || COM_CheckParm ("-startwindowed"))
		Cvar_Set(Cvar_FindVar("vid_fullscreen"), "0");
	if (COM_CheckParm ("-fullscreen"))
		Cvar_Set(Cvar_FindVar("vid_fullscreen"), "1");

	if ((i = COM_CheckParm ("-width")))	//width on it's own also sets height
	{
		Cvar_Set(Cvar_FindVar("vid_width"), com_argv[i+1]);
		Cvar_SetValue(Cvar_FindVar("vid_height"), (atoi(com_argv[i+1])/4)*3);
	}
	if ((i = COM_CheckParm ("-height")))
		Cvar_Set(Cvar_FindVar("vid_height"), com_argv[i+1]);

	if ((i = COM_CheckParm ("-conwidth")))	//width on it's own also sets height
	{
		Cvar_Set(Cvar_FindVar("vid_conwidth"), com_argv[i+1]);
		Cvar_SetValue(Cvar_FindVar("vid_conheight"), (atoi(com_argv[i+1])/4)*3);
	}
	if ((i = COM_CheckParm ("-conheight")))
		Cvar_Set(Cvar_FindVar("vid_conheight"), com_argv[i+1]);

	if ((i = COM_CheckParm ("-bpp")))
		Cvar_Set(Cvar_FindVar("vid_bpp"), com_argv[i+1]);

	Cvar_ApplyLatches(CVAR_RENDERERLATCH);

//-1 means 'never set'
	if (qrenderer == -1 && *vid_renderer.string)
	{
		Cmd_ExecuteString("vid_restart\n", RESTRICT_LOCAL);
	}
	if (qrenderer == -1)
	{	//we still failed. Try again, but use the default renderer.
		Cvar_Set(&vid_renderer, "");
		Cmd_ExecuteString("vid_restart\n", RESTRICT_LOCAL);
	}
	if (qrenderer == -1)
		Sys_Error("No renderer was set!\n");

	if (qrenderer == QR_NONE)
		Con_Printf("Use the setrenderer command to use a gui\n");

#ifdef VM_UI
	UI_Init();
#endif

#ifndef NOMEDIA
	if (!cls.demofile && !cls.state && !Media_PlayingFullScreen())
	{
		int ol_depth;
		int idcin_depth;
		int idroq_depth;

		idcin_depth = COM_FDepthFile("video/idlog.cin", true);	//q2
		idroq_depth = COM_FDepthFile("video/idlogo.roq", true);	//q2
		ol_depth = COM_FDepthFile("video/openinglogos.roq", true);	//jk2

		if (ol_depth != 0x7fffffff && (ol_depth <= idroq_depth || ol_depth <= idcin_depth))
			Media_PlayFilm("video/openinglogos.roq");
		else if (idroq_depth != 0x7fffffff && idroq_depth <= idcin_depth)
			Media_PlayFilm("video/idlogo.roq");
		else if (idcin_depth != 0x7fffffff)
			Media_PlayFilm("video/idlog.cin");
	}
#endif

Con_TPrintf (TL_NL);
	Con_TPrintf (TL_VERSION, DISTRIBUTION, build_number());
Con_TPrintf (TL_NL);

	Con_TPrintf (TLC_QUAKEWORLD_INITED);

	Con_DPrintf("This program is free software; you can redistribute it and/or "
				"modify it under the terms of the GNU General Public License "
				"as published by the Free Software Foundation; either version 2 "
				"of the License, or (at your option) any later version."
				"\n"
				"This program is distributed in the hope that it will be useful, "
				"but WITHOUT ANY WARRANTY; without even the implied warranty of "
				"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. "
				"\n"
				"See the GNU General Public License for more details.\n");
}


/*
===============
Host_Shutdown

FIXME: this is a callback from Sys_Quit and Sys_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void Host_Shutdown(void)
{
	static qboolean isdown = false;

	if (isdown)
	{
		Sys_Printf ("recursive shutdown\n");
		return;
	}
	isdown = true;

#ifdef VM_UI
	UI_Stop();
#endif

	Host_WriteConfiguration ();

	CDAudio_Shutdown ();
	S_Shutdown();
	IN_Shutdown ();
	if (VID_DeInit)
		VID_DeInit();
#ifndef CLIENTONLY
	SV_Shutdown();
#else
	NET_Shutdown ();
#endif

	Cvar_Shutdown();
	Validation_FlushFileList();
}

#ifdef CLIENTONLY
void SV_EndRedirect (void)
{
}
#endif
