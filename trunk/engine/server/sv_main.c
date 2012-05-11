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
#include "qwsvdef.h"
#include "netinc.h"
#include <sys/types.h>
#ifndef CLIENTONLY
#define Q2EDICT_NUM(i) (q2edict_t*)((char *)ge->edicts+i*ge->edict_size)

void SV_Savegame_f (void);
void SV_Loadgame_f (void);
#define INVIS_CHAR1 12
#define INVIS_CHAR2 (char)138
#define INVIS_CHAR3 (char)160

quakeparms_t host_parms;

qboolean	host_initialized;		// true if into command execution (compatability)

double		host_frametime;
double		realtime;				// without any filtering or bounding

int			host_hunklevel;

// callbacks
void SV_Masterlist_Callback(struct cvar_s *var, char *oldvalue);
void SV_Tcpport_Callback(struct cvar_s *var, char *oldvalue);
void SV_Tcpport6_Callback(struct cvar_s *var, char *oldvalue);
void SV_Port_Callback(struct cvar_s *var, char *oldvalue);
void SV_PortIPv6_Callback(struct cvar_s *var, char *oldvalue);
void SV_PortIPX_Callback(struct cvar_s *var, char *oldvalue);

typedef struct {
	enum {
		MP_NONE,
		MP_QUAKEWORLD,
		MP_DARKPLACES,
	} protocol;
	cvar_t		cv;

	qboolean	needsresolve;	//set any time the cvar is modified
	netadr_t	adr;
} sv_masterlist_t;
sv_masterlist_t sv_masterlist[] = {
	{MP_QUAKEWORLD, CVARC("sv_master1", "", SV_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("sv_master2", "", SV_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("sv_master3", "", SV_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("sv_master4", "", SV_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("sv_master5", "", SV_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("sv_master6", "", SV_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("sv_master7", "", SV_Masterlist_Callback)},
	{MP_QUAKEWORLD, CVARC("sv_master8", "", SV_Masterlist_Callback)},

	{MP_QUAKEWORLD, CVARC("sv_qwmasterextra1", "qwmaster.ocrana.de:27000", SV_Masterlist_Callback)},	//german. admin unknown
	{MP_QUAKEWORLD, CVARC("sv_qwmasterextra2", "masterserver.exhale.de:27000", SV_Masterlist_Callback)},	//german. admin unknown
	{MP_QUAKEWORLD, CVARC("sv_qwmasterextra3", "asgaard.morphos-team.net:27000", SV_Masterlist_Callback)},	//german. admin bigfoot
	{MP_QUAKEWORLD, CVARC("sv_qwmasterextra4", "master.quakeservers.net:27000", SV_Masterlist_Callback)},	//european. admin: raz0?
	{MP_QUAKEWORLD, CVARC("sv_qwmasterextra5", "qwmaster.fodquake.net:27000", SV_Masterlist_Callback)},

	{MP_DARKPLACES, CVARC("sv_masterextra1", "ghdigital.com:27950", SV_Masterlist_Callback)}, //69.59.212.88 (admin: LordHavoc)
	{MP_DARKPLACES, CVARC("sv_masterextra2", "dpmaster.deathmask.net:27950", SV_Masterlist_Callback)}, //209.164.24.243 (admin: Willis)
	{MP_DARKPLACES, CVARC("sv_masterextra3", "dpmaster.tchr.no:27950", SV_Masterlist_Callback)}, // (admin: tChr)
	{MP_NONE, CVAR(NULL, NULL)}
};

client_t	*host_client;			// current client

// bound the size of the physics time tic
#ifdef SERVERONLY
cvar_t	sv_mintic = SCVAR("sv_mintic","0.03");
#else
cvar_t	sv_mintic = SCVAR("sv_mintic","0");	//client builds can think as often as they want.
#endif
cvar_t	sv_maxtic = SCVAR("sv_maxtic","0.1");//never run a tick slower than this
cvar_t	sv_limittics = SCVAR("sv_limittics","3");//

cvar_t	sv_nailhack = SCVAR("sv_nailhack","0");


cvar_t	timeout = SCVAR("timeout","65");		// seconds without any message
cvar_t	zombietime = SCVAR("zombietime", "2");	// seconds to sink messages
											// after disconnect
#ifdef SERVERONLY
cvar_t	developer = SCVAR("developer","0");		// show extra messages

cvar_t	rcon_password = SCVARF("rcon_password", "", CVAR_NOUNSAFEEXPAND);	// password for remote server commands
cvar_t	password = SCVARF("password", "", CVAR_NOUNSAFEEXPAND);	// password for entering the game
#else
extern cvar_t	developer;
extern cvar_t	rcon_password;
extern cvar_t	password;
#endif
cvar_t	spectator_password = CVARF("spectator_password", "", CVAR_NOUNSAFEEXPAND);	// password for entering as a sepctator

cvar_t	allow_download = CVAR("allow_download", "1");
cvar_t	allow_download_skins = CVAR("allow_download_skins", "1");
cvar_t	allow_download_models = CVAR("allow_download_models", "1");
cvar_t	allow_download_sounds = CVAR("allow_download_sounds", "1");
cvar_t	allow_download_demos = CVAR("allow_download_demos", "1");
cvar_t	allow_download_maps = CVAR("allow_download_maps", "1");
cvar_t	allow_download_anymap = CVAR("allow_download_pakmaps", "0");
cvar_t	allow_download_pakcontents = CVAR("allow_download_pakcontents", "1");
cvar_t	allow_download_root = CVAR("allow_download_root", "0");
cvar_t	allow_download_textures = CVAR("allow_download_textures", "1");
cvar_t	allow_download_packages = CVAR("allow_download_packages", "1");
cvar_t	allow_download_wads = CVAR("allow_download_wads", "1");
cvar_t	allow_download_configs = CVAR("allow_download_configs", "0");
cvar_t	allow_download_copyrighted = CVAR("allow_download_copyrighted", "0");

cvar_t sv_public = CVAR("sv_public", "0");
cvar_t sv_listen_qw = CVARAF("sv_listen_qw", "1", "sv_listen", 0);
cvar_t sv_listen_nq = CVARD("sv_listen_nq", "2", "Allow new (net)quake clients to connect to the server. 0 = don't let them in. 1 = allow them in (WARNING: this allows 'qsmurf' DOS attacks). 2 = accept (net)quake clients by emulating a challenge (as secure as QW/Q2 but does not fully conform to the NQ protocol).");
cvar_t sv_listen_dp = CVAR("sv_listen_dp", "0"); /*kinda fucked right now*/
cvar_t sv_listen_q3 = CVAR("sv_listen_q3", "0");
cvar_t sv_reportheartbeats = CVAR("sv_reportheartbeats", "1");
cvar_t sv_highchars = CVAR("sv_highchars", "1");
cvar_t sv_loadentfiles = CVAR("sv_loadentfiles", "1");
cvar_t sv_maxrate = CVAR("sv_maxrate", "30000");
cvar_t sv_maxdrate = CVARAF("sv_maxdrate", "100000",
							"sv_maxdownloadrate", 0);
cvar_t sv_minping = CVARF("sv_minping", "0", CVAR_SERVERINFO);

cvar_t sv_bigcoords = CVARF("sv_bigcoords", "", CVAR_SERVERINFO);
cvar_t sv_calcphs = CVAR("sv_calcphs", "2");

cvar_t sv_cullplayers_trace = CVARF("sv_cullplayers_trace", "", CVAR_SERVERINFO);
cvar_t sv_cullentities_trace = CVARF("sv_cullentities_trace", "", CVAR_SERVERINFO);
cvar_t sv_phs = CVAR("sv_phs", "1");
cvar_t sv_resetparms = CVAR("sv_resetparms", "0");
cvar_t sv_pupglow = CVARF("sv_pupglow", "", CVAR_SERVERINFO);

cvar_t sv_master = CVAR("sv_master", "0");
cvar_t sv_masterport = CVAR("sv_masterport", "0");

cvar_t	sv_gamespeed = CVAR("sv_gamespeed", "1");
cvar_t	sv_csqcdebug = CVAR("sv_csqcdebug", "0");
cvar_t	sv_csqc_progname = CVAR("sv_csqc_progname", "csprogs.dat");
#ifdef TCPCONNECT
cvar_t	sv_port_tcp = CVARC("sv_port_tcp", "", SV_Tcpport_Callback);
#ifdef IPPROTO_IPV6
cvar_t	sv_port_tcp6 = CVARC("sv_port_tcp6", "", SV_Tcpport6_Callback);
#endif
#endif
#ifdef HAVE_IPV4
cvar_t  sv_port_ipv4 = CVARC("sv_port", "27500", SV_Port_Callback);
#endif
#ifdef IPPROTO_IPV6
cvar_t  sv_port_ipv6 = CVARC("sv_port_ipv6", "", SV_PortIPv6_Callback);
#endif
#ifdef USEIPX
cvar_t  sv_port_ipx = CVARC("sv_port_ipx", "", SV_PortIPX_Callback);
#endif

cvar_t pausable	= CVAR("pausable", "1");


//
// game rules mirrored in svs.info
//
cvar_t	fraglimit		= CVARF("fraglimit",		"" ,	CVAR_SERVERINFO);
cvar_t	timelimit		= CVARF("timelimit",		"" ,	CVAR_SERVERINFO);
cvar_t	teamplay		= CVARF("teamplay",		"" ,	CVAR_SERVERINFO);
cvar_t	samelevel		= CVARF("samelevel",		"" ,	CVAR_SERVERINFO);
cvar_t	maxclients		= CVARAF("maxclients",		"8",
								 "sv_maxclients",			CVAR_SERVERINFO);
cvar_t	maxspectators	= CVARF("maxspectators",	"8",	CVAR_SERVERINFO);
#ifdef SERVERONLY
cvar_t	deathmatch		= CVARF("deathmatch",		"1",	CVAR_SERVERINFO);			// 0, 1, or 2
#else
cvar_t	deathmatch		= CVARF("deathmatch",		"0",	CVAR_SERVERINFO);			// 0, 1, or 2
#endif
cvar_t	coop			= CVARF("coop",			"" ,	CVAR_SERVERINFO);
cvar_t	skill			= CVARF("skill",			"" ,	CVAR_SERVERINFO);			// 0, 1, 2 or 3
cvar_t	spawn			= CVARF("spawn",			"" ,	CVAR_SERVERINFO);
cvar_t	watervis		= CVARF("watervis",		"" ,	CVAR_SERVERINFO);
cvar_t	rearview		= CVARF("rearview",		"" ,	CVAR_SERVERINFO);
cvar_t	allow_luma		= CVARF("allow_luma",		"1",	CVAR_SERVERINFO);
#pragma warningmsg("Remove this some time")
cvar_t	allow_bump		= CVARF("allow_bump",		"1",	CVAR_SERVERINFO);
cvar_t	allow_skybox	= CVARF("allow_skybox",	"",		CVAR_SERVERINFO);
cvar_t	sv_allow_splitscreen = CVARF("allow_splitscreen","",CVAR_SERVERINFO);
cvar_t	fbskins			= CVARF("fbskins",			"1",	CVAR_SERVERINFO);	//to get rid of lame fuhquake fbskins
cvar_t	mirrors			= CVARF("mirrors",			"" ,	CVAR_SERVERINFO);

cvar_t	sv_motd[]		={	CVAR("sv_motd1",		""),
							CVAR("sv_motd2",		""),
							CVAR("sv_motd3",		""),
							CVAR("sv_motd4",		"")	};

cvar_t sv_compatiblehulls = CVAR("sv_compatiblehulls", "1");

cvar_t	hostname = CVARF("hostname","unnamed", CVAR_SERVERINFO);

cvar_t	secure = CVARF("secure", "", CVAR_SERVERINFO);

extern cvar_t sv_nomsec;

char cvargroup_serverpermissions[] = "server permission variables";
char cvargroup_serverinfo[] = "serverinfo variables";
char cvargroup_serverphysics[] = "server physics variables";
char cvargroup_servercontrol[] = "server control variables";

vfsfile_t	*sv_fraglogfile;

void SV_FixupName(char *in, char *out, unsigned int outlen);
void SV_AcceptClient (netadr_t adr, int userid, char *userinfo);
void Master_Shutdown (void);
void PRH2_SetPlayerClass(client_t *cl, int classnum, qboolean fromqc);
char *SV_BannedReason (netadr_t *a);

#ifdef SQL
void PR_SQLCycle();
#endif

//============================================================================

qboolean ServerPaused(void)
{
	return sv.paused;
}

/*
================
SV_Shutdown

Quake calls this before calling Sys_Quit or Sys_Error
================
*/
void SV_Shutdown (void)
{
	Master_Shutdown ();
	if (sv_fraglogfile)
	{
		VFS_CLOSE (sv_fraglogfile);
		sv_fraglogfile = NULL;
	}

	PR_Deinit();
#ifdef USEODE
	World_ODE_Shutdown();
#endif

	if (sv.mvdrecording)
		SV_MVDStop (0, false);

	NET_Shutdown ();
#ifdef WEBSERVER
	IWebShutdown();
#endif

	Memory_DeInit();
}

/*
================
SV_Error

Sends a datagram to all the clients informing them of the server crash,
then exits
================
*/
void VARGS SV_Error (char *error, ...)
{
	va_list		argptr;
	static	char		string[1024];
	static	qboolean inerror = false;

	if (inerror)
	{
		Sys_Error ("SV_Error: recursively entered (%s)", string);
	}

	inerror = true;

	va_start (argptr,error);
	vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);

	{
		extern cvar_t pr_ssqc_coreonerror;

		if (svprogfuncs && pr_ssqc_coreonerror.value && svprogfuncs->save_ents)
		{
			int size = 1024*1024*8;
			char *buffer = BZ_Malloc(size);
			svprogfuncs->save_ents(svprogfuncs, buffer, &size, 3);
			COM_WriteFile("ssqccore.txt", buffer, size);
			BZ_Free(buffer);
		}
	}


	SV_EndRedirect();

	Con_Printf ("SV_Error: %s\n",string);

	if (sv.state)
		SV_FinalMessage (va("server crashed: %s\n", string));

	SV_UnspawnServer();
#ifndef SERVERONLY
	if (cls.state)
	{
		inerror = false;
		Host_EndGame("SV_Error: %s\n",string);
	}
	SCR_EndLoadingPlaque();

	if (!isDedicated)	//dedicated servers crash...
	{
		extern jmp_buf 	host_abort;
		SCR_EndLoadingPlaque();
		inerror=false;
		longjmp (host_abort, 1);
	}
#endif

	SV_Shutdown ();

	Sys_Error ("SV_Error: %s\n",string);
}

#ifdef SERVERONLY
void VARGS Host_Error (char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,error);
	vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);

	SV_Error("%s", string);
}
#endif

/*
==================
SV_FinalMessage

Used by SV_Error and SV_Quit_f to send a final message to all connected
clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void SV_FinalMessage (char *message)
{
	int			i;
	client_t	*cl;

	SZ_Clear (&sv.datagram);
	MSG_WriteByte (&sv.datagram, svc_print);
	MSG_WriteByte (&sv.datagram, PRINT_HIGH);
	MSG_WriteString (&sv.datagram, message);
	MSG_WriteByte (&sv.datagram, svc_disconnect);

	for (i=0, cl = svs.clients ; i<MAX_CLIENTS ; i++, cl++)
		if (cl->state >= cs_spawned)
			if (ISNQCLIENT(cl) || ISQWCLIENT(cl))
				Netchan_Transmit (&cl->netchan, sv.datagram.cursize
						, sv.datagram.data, 10000);
}



/*
=====================
SV_DropClient

Called when the player is totally leaving the server, either willingly
or unwillingly.  This is NOT called if the entire server is quiting
or crashing.
=====================
*/
void SV_DropClient (client_t *drop)
{
	laggedpacket_t *lp;

	/*drop the first in the chain first*/
	if (drop->controller && drop->controller != drop)
	{
		SV_DropClient(drop->controller);
		return;
	}

	if (!drop->controller)
	{
		// add the disconnect
		if (drop->state != cs_zombie)
		{
			switch (drop->protocol)
			{
			case SCP_QUAKE2:
				MSG_WriteByte (&drop->netchan.message, svcq2_disconnect);
				break;
			case SCP_QUAKEWORLD:
			case SCP_NETQUAKE:
			case SCP_FITZ666:
			case SCP_DARKPLACES6:
			case SCP_DARKPLACES7:
				MSG_WriteByte (&drop->netchan.message, svc_disconnect);
				break;
			case SCP_BAD:
				break;
			case SCP_QUAKE3:
				break;
			}
		}
	}

#ifdef SVRANKING
	if (drop->state == cs_spawned)
	{
		int j;
		rankstats_t rs;
		if (drop->rankid)
		{
			if (Rank_GetPlayerStats(drop->rankid, &rs))
			{
				rs.timeonserver += realtime - drop->stats_started;
				drop->stats_started = realtime;
				rs.kills += drop->kills;
				rs.deaths += drop->deaths;

				rs.flags1 &= ~(RANK_CUFFED|RANK_MUTED|RANK_CRIPPLED);
				if (drop->iscuffed==2)
					rs.flags1 |= RANK_CUFFED;
				if (drop->ismuted==2)
					rs.flags1 |= RANK_MUTED;
				if (drop->iscrippled==2)
					rs.flags1 |= RANK_CRIPPLED;
				drop->kills=0;
				drop->deaths=0;
				pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, drop->edict);
				if (pr_global_ptrs->SetChangeParms)
					PR_ExecuteProgram (svprogfuncs, pr_global_struct->SetChangeParms);
				for (j=0 ; j<NUM_RANK_SPAWN_PARMS ; j++)
					if (pr_global_ptrs->spawnparamglobals[j])
						rs.parm[j] = *pr_global_ptrs->spawnparamglobals[j];
				Rank_SetPlayerStats(drop->rankid, &rs);
			}
		}
	}
#endif
#ifdef SVCHAT
	SV_WipeChat(drop);
#endif
	switch(svs.gametype)
	{
	case GT_MAX:
		break;
	case GT_Q1QVM:
	case GT_PROGS:
		if (svprogfuncs)
		{
			if (drop->state == cs_spawned)
			{
#ifdef VM_Q1
				if (svs.gametype == GT_Q1QVM)
				{
					Q1QVM_DropClient(drop);
				}
				else
#endif
				{
					if (!drop->spectator)
					{
					// call the prog function for removing a client
					// this will set the body to a dead frame, among other things
						pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, drop->edict);
						if (pr_global_ptrs->ClientDisconnect)
							PR_ExecuteProgram (svprogfuncs, pr_global_struct->ClientDisconnect);
						sv.spawned_client_slots--;
					}
					else if (SpectatorDisconnect)
					{
					// call the prog function for removing a client
					// this will set the body to a dead frame, among other things
						pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, drop->edict);
						PR_ExecuteProgram (svprogfuncs, SpectatorDisconnect);
					}
				}

				if (progstype == PROG_NQ)
					ED_Clear(svprogfuncs, drop->edict);
			}

			if (drop->spawninfo)
				Z_Free(drop->spawninfo);
			drop->spawninfo = NULL;
		}
		break;
	case GT_QUAKE2:
#ifdef Q2SERVER
		if (ge)
			ge->ClientDisconnect(drop->q2edict);
#endif
		break;
	case GT_QUAKE3:
#ifdef Q3SERVER
		SVQ3_DropClient(drop);
#endif
		break;
	case GT_HALFLIFE:
#ifdef HLSERVER
		SVHL_DropClient(drop);
#endif
		break;
	}

	if (drop->centerprintstring)
		Z_Free(drop->centerprintstring);
	drop->centerprintstring = NULL;

	if (!drop->redirect)
	{
		if (drop->spectator)
			Con_Printf ("Spectator %s removed\n",drop->name);
		else
			Con_Printf ("Client %s removed\n",drop->name);
	}

	if (drop->download)
	{
		VFS_CLOSE (drop->download);
		drop->download = NULL;
	}
	if (drop->upload)
	{
		VFS_CLOSE (drop->upload);
		drop->upload = NULL;
	}
	*drop->uploadfn = 0;

#ifndef SERVERONLY
	if (drop->netchan.remote_address.type == NA_LOOPBACK)
	{
		Netchan_Transmit(&drop->netchan, 0, "", SV_RateForClient(drop));
#ifdef warningmsg
#pragma warningmsg("This mans that we may not see the reason we kicked ourselves.")
#endif
		CL_Disconnect();
		drop->state = cs_free;	//don't do zombie stuff
	}
	else
#endif
	{
		drop->state = cs_zombie;		// become free in a few seconds
		drop->connection_started = realtime;	// for zombie timeout
	}
	drop->istobeloaded = false;

	drop->old_frags = 0;
#ifdef SVRANKING
	drop->kills = 0;
	drop->deaths = 0;
#endif
	if (svprogfuncs && drop->edict)
		drop->edict->v->frags = 0;
	drop->namebuf[0] = 0;
	drop->name = drop->namebuf;
	memset (drop->userinfo, 0, sizeof(drop->userinfo));
	memset (drop->userinfobasic, 0, sizeof(drop->userinfobasic));

	while ((lp = drop->laggedpacket))
	{
		drop->laggedpacket = lp->next;
		lp->next = svs.free_lagged_packet;
		svs.free_lagged_packet = lp;
	}
	drop->laggedpacket_last = NULL;

	if (drop->frameunion.frames)	//union of the same sort of structure
	{
		Z_Free(drop->frameunion.frames);
		drop->frameunion.frames = NULL;
	}
	if (drop->sentents.entities)
	{
		Z_Free(drop->sentents.entities);
		memset(&drop->sentents.entities, 0, sizeof(drop->sentents.entities));
	}

	if (svs.gametype == GT_PROGS || svs.gametype == GT_Q1QVM)	//gamecode should do it all for us.
	{
// send notification to all remaining clients
		SV_FullClientUpdate (drop, &sv.reliable_datagram, 0);
#ifdef NQPROT
		SVNQ_FullClientUpdate (drop, &sv.nqreliable_datagram);
#endif
		SV_MVD_FullClientUpdate(NULL, drop);
	}

	if (drop->controlled)
	{
		drop->controlled->controller = NULL;
		SV_DropClient(drop->controlled);
		drop->controlled = NULL;
	}
}


//====================================================================

typedef struct pinnedmessages_s {
	struct pinnedmessages_s *next;
	char setby[64];
	char message[1024];
} pinnedmessages_t;
pinnedmessages_t *pinned;
qboolean dopinnedload = true;
void PIN_DeleteOldestMessage(void);
void PIN_MakeMessage(char *from, char *msg);

void PIN_LoadMessages(void)
{
	char setby[64];
	char message[1024];

	int i;
	char *file, *end;
	char *lstart;
	int len;

	dopinnedload = false;

	while(pinned)
		PIN_DeleteOldestMessage();

	len = FS_LoadFile("pinned.txt", (void**)&file);
	if (!file)
		return;
	end = file+len;

	lstart = file;
	for(;;)
	{
		while (lstart<end && *lstart <= ' ')
			lstart++;

		for (i = 0; lstart<end && i < sizeof(message)-1; i++)
		{
			if (*lstart == '\n' || *lstart == '\r')
				break;
			message[i] = *lstart++;
		}
		message[i] = '\0';

		while (lstart<end && *lstart <= ' ')
			lstart++;

		for (i = 0; lstart<end && i < sizeof(setby)-1; i++)
		{
			if (*lstart == '\n' || *lstart == '\r')
				break;
			setby[i] = *lstart++;
		}
		setby[i] = '\0';

		if (!*setby)
			break;

		PIN_MakeMessage(setby, message);
	}

	FS_FreeFile(file);
}
void PIN_SaveMessages(void)
{
	pinnedmessages_t *p;
	FILE *f;

	f = COM_WriteFileOpen("pinned.txt");
	if (!f)
	{
		Con_Printf("couldn't write anything\n");
		return;
	}

	for (p = pinned; p; p = p->next)
		fprintf(f, "%s\r\n\t%s\r\n\n", p->message, p->setby);

	fclose(f);
}
void PIN_DeleteOldestMessage(void)
{
	pinnedmessages_t *old = pinned;
	if (old)
	{
		pinned = pinned->next;
		Z_Free(old);
	}
}
void PIN_MakeMessage(char *from, char *msg)
{
	pinnedmessages_t *p;
	pinnedmessages_t *newp;

	newp = BZ_Malloc(sizeof(pinnedmessages_t));
	Q_strncpyz(newp->setby, from, sizeof(newp->setby));
	Q_strncpyz(newp->message, msg, sizeof(newp->message));
	newp->next = NULL;

	if (!pinned)
		pinned = newp;
	else
	{
		for (p = pinned; ; p = p->next)
		{
			if (!p->next)
			{
				p->next = newp;
				break;
			}
		}
	}
}
void PIN_ShowMessages(client_t *cl)
{
	pinnedmessages_t *p;
	if (dopinnedload)
		PIN_LoadMessages();

	if (!pinned)
		return;

	SV_ClientPrintf(cl, PRINT_HIGH, "\n\n\n");
	for (p = pinned; p; p = p->next)
	{
		SV_ClientPrintf(cl, PRINT_HIGH, "%s\n\n        %s\n", p->message, p->setby);
		SV_ClientPrintf(cl, PRINT_HIGH, "\n\n\n");
	}

}

//====================================================================

/*
===================
SV_CalcPing

===================
*/
int SV_CalcPing (client_t *cl, qboolean forcecalc)
{
	float		ping;
	int			i;
	int			count;
	register	client_frame_t *frame;

	if (!cl->frameunion.frames)
		return 0;

	switch (cl->protocol)
	{
	case SCP_BAD:
		break;
	case SCP_QUAKE2:
		break;
	case SCP_QUAKE3:
		break;
	case SCP_DARKPLACES6:
	case SCP_DARKPLACES7:
	case SCP_NETQUAKE:
	case SCP_FITZ666:
	case SCP_QUAKEWORLD:
		ping = 0;
		count = 0;
		for (frame = cl->frameunion.frames, i=0 ; i<UPDATE_BACKUP ; i++, frame++)
		{
			if (frame->ping_time > 0)
			{
				ping += frame->ping_time;
				count++;
			}
		}
		if (!count)
			return 9999;
		ping /= count;

		return ping*1000;
	}
	return 0;
}

void SV_GenerateBasicUserInfo(client_t *cl)
{
	char *key, *s;
	int i;
	for (i= 1; (key = Info_KeyForNumber(cl->userinfo, i)); i++)
	{
		if (!*key)
			break;
		if (!SV_UserInfoIsBasic(key))
			continue;

		s = Info_ValueForKey(cl->userinfo, key);
		Info_SetValueForStarKey (cl->userinfobasic, key, s, sizeof(cl->userinfobasic));
	}
}

/*
===================
SV_FullClientUpdate

Writes all update values to a sizebuf
===================
*/
void SV_FullClientUpdate (client_t *client, sizebuf_t *buf, unsigned int ftepext)
{
	int		i;
	char	info[MAX_INFO_STRING];

	i = client - svs.clients;

#ifdef SERVER_DEMO_PLAYBACK
	if (sv.demofile)
	{
		MSG_WriteByte (buf, svc_updatefrags);
		MSG_WriteByte (buf, i);
		MSG_WriteShort (buf, sv.recordedplayer[i].frags);

		MSG_WriteByte (buf, svc_updateping);
		MSG_WriteByte (buf, i);
		MSG_WriteShort (buf, sv.recordedplayer[i].ping);

		MSG_WriteByte (buf, svc_updatepl);
		MSG_WriteByte (buf, i);
		MSG_WriteByte (buf, sv.recordedplayer[i].pl);

		MSG_WriteByte (buf, svc_updateentertime);
		MSG_WriteByte (buf, i);
		MSG_WriteFloat (buf, 0);

		Q_strncpyz (info, sv.recordedplayer[i].userinfo, sizeof(info));
		Info_RemoveKey(info, "password");		//main password key
		Info_RemoveKey(info, "*ip");		//don't broadcast this in playback
		Info_RemovePrefixedKeys (info, '_');	// server passwords, etc

		MSG_WriteByte (buf, svc_updateuserinfo);
		MSG_WriteByte (buf, i);
		MSG_WriteLong (buf, sv.recordedplayer[i].userid);
		MSG_WriteString (buf, info);
		return;
	}
#endif

//Sys_Printf("SV_FullClientUpdate:  Updated frags for client %d\n", i);

	MSG_WriteByte (buf, svc_updatefrags);
	MSG_WriteByte (buf, i);
	MSG_WriteShort (buf, client->old_frags);

	MSG_WriteByte (buf, svc_updateping);
	MSG_WriteByte (buf, i);
	MSG_WriteShort (buf, SV_CalcPing (client, false));

	MSG_WriteByte (buf, svc_updatepl);
	MSG_WriteByte (buf, i);
	MSG_WriteByte (buf, client->lossage);

	MSG_WriteByte (buf, svc_updateentertime);
	MSG_WriteByte (buf, i);
	MSG_WriteFloat (buf, realtime - client->connection_started);

#ifdef warningmsg
#pragma warningmsg("this is a bug: it can be broadcast to all qw clients")
#endif
	if (ftepext & PEXT_BIGUSERINFOS)
		Q_strncpyz (info, client->userinfo, sizeof(info));
	else
		Q_strncpyz (info, client->userinfobasic, sizeof(info));
	Info_RemoveKey(info, "password");		//main password key
	Info_RemovePrefixedKeys (info, '_');	// server passwords, etc

	MSG_WriteByte (buf, svc_updateuserinfo);
	MSG_WriteByte (buf, i);
	MSG_WriteLong (buf, client->userid);
	MSG_WriteString (buf, info);
}
#ifdef NQPROT
void SVNQ_FullClientUpdate (client_t *client, sizebuf_t *buf)
{
	int playercolor, top, bottom;
	int		i;

	i = client - svs.clients;

	if (i >= 16)
		return;

//Sys_Printf("SV_FullClientUpdate:  Updated frags for client %d\n", i);

	MSG_WriteByte (buf, svc_updatefrags);
	MSG_WriteByte (buf, i);
	MSG_WriteShort (buf, client->old_frags);

	MSG_WriteByte (buf, svc_updatename);
	MSG_WriteByte (buf, i);
	MSG_WriteString (buf, Info_ValueForKey(client->userinfo, "name"));

	top = atoi(Info_ValueForKey(client->userinfo, "topcolor"));
	bottom = atoi(Info_ValueForKey(client->userinfo, "bottomcolor"));
	top &= 15;
	if (top > 13)
		top = 13;
	bottom &= 15;
	if (bottom > 13)
		bottom = 13;
	playercolor = top*16 + bottom;
	MSG_WriteByte (buf, svc_updatecolors);
	MSG_WriteByte (buf, i);
	MSG_WriteByte (buf, playercolor);
}
#endif
/*
===================
SV_FullClientUpdateToClient

Writes all update values to a client's reliable stream
===================
*/
void SV_FullClientUpdateToClient (client_t *client, client_t *cl)
{
#ifdef NQPROT
	if (!ISQWCLIENT(cl))
	{
		ClientReliableCheckBlock(cl, 24 + strlen(client->userinfo));
		if (cl->num_backbuf) {
			SVNQ_FullClientUpdate (client, &cl->backbuf);
			ClientReliable_FinishWrite(cl);
		} else
			SVNQ_FullClientUpdate (client, &cl->netchan.message);
	}
	else
#endif
	{
#ifdef SERVER_DEMO_PLAYBACK
		if (sv.demofile)
		{
			int i = client - svs.clients;
			ClientReliableCheckBlock(cl, 24 + strlen(sv.recordedplayer[i].userinfo));
		}
		else
#endif
			ClientReliableCheckBlock(cl, 24 + strlen(client->userinfo));
		if (cl->num_backbuf) {
			SV_FullClientUpdate (client, &cl->backbuf, cl->fteprotocolextensions);
			ClientReliable_FinishWrite(cl);
		} else
			SV_FullClientUpdate (client, &cl->netchan.message, cl->fteprotocolextensions);
	}
}


/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

#define STATUS_OLDSTYLE					0
#define	STATUS_SERVERINFO				1
#define	STATUS_PLAYERS					2
#define	STATUS_SPECTATORS				4
#define	STATUS_SPECTATORS_AS_PLAYERS	8 //for ASE - change only frags: show as "S"

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see
This message can be up to around 5k with worst case string lengths.
================
*/
void SVC_Status (void)
{
	int displayflags;
	int		i;
	client_t	*cl;
	char *name;
	int		ping;
	int		top, bottom;
	char frags[64];

	int slots=0;

	displayflags = atoi(Cmd_Argv(1));
	if (displayflags == STATUS_OLDSTYLE)
		displayflags = STATUS_SERVERINFO|STATUS_PLAYERS;

	Cmd_TokenizeString ("status", false, false);
	SV_BeginRedirect (RD_PACKET, LANGDEFAULT);
	if (displayflags&STATUS_SERVERINFO)
		Con_Printf ("%s\n", svs.info);
	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		cl = &svs.clients[i];
		if ((cl->state == cs_connected || cl->state == cs_spawned || cl->name[0]) && ((cl->spectator && displayflags&STATUS_SPECTATORS) || (!cl->spectator && displayflags&STATUS_PLAYERS)))
		{
			top = atoi(Info_ValueForKey (cl->userinfo, "topcolor"));
			bottom = atoi(Info_ValueForKey (cl->userinfo, "bottomcolor"));
			top = (top < 0) ? 0 : ((top > 13) ? 13 : top);
			bottom = (bottom < 0) ? 0 : ((bottom > 13) ? 13 : bottom);
			ping = SV_CalcPing (cl, false);
			name = cl->name;

			if (!cl->state)	//show bots differently. Just to be courteous.
				Con_Printf ("%i %i %i %i \"BOT:%s\" \"%s\" %i %i\n", cl->userid,
					cl->old_frags, (int)(realtime - cl->connection_started)/60,
					ping, cl->name, Info_ValueForKey (cl->userinfo, "skin"), top, bottom);
			else
			{
				if (cl->spectator)
				{	//silly mvdsv stuff
					if (displayflags & STATUS_SPECTATORS_AS_PLAYERS)
					{
						frags[0] = 'S';
						frags[1] = '\0';
					}
					else
					{
						ping = -ping;
						sprintf(frags, "%i", -9999);
						name  = va("\\s\\%s", name);
					}
				}
				else
					sprintf(frags, "%i", cl->old_frags);

				Con_Printf ("%i %s %i %i \"%s\" \"%s\" %i %i\n", cl->userid,
					frags, (int)(realtime - cl->connection_started)/60,
					ping, name, Info_ValueForKey (cl->userinfo, "skin"), top, bottom);
			}
		}
		else
			slots++;
	}

	SV_EndRedirect ();
}

#ifdef NQPROT
void SVC_GetInfo (char *challenge, int fullstatus)
{
	//dpmaster support
	char response[MAX_UDP_PACKET];

	client_t	*cl;
	int numclients = 0;
	int i;
	char *resp;
	char *gamestatus;
	eval_t *v;

	if (!sv_listen_nq.ival && !sv_listen_dp.ival)
		return;

	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		cl = &svs.clients[i];
		if ((cl->state == cs_connected || cl->state == cs_spawned || cl->name[0]) && !cl->spectator)
			numclients++;
	}

	if (svprogfuncs)
	{
		v = PR_FindGlobal(svprogfuncs, "worldstatus", PR_ANY, NULL);
		if (v)
			gamestatus = PR_GetString(svprogfuncs, v->string);
		else
			gamestatus = "";
	}
	else
		gamestatus = "";


	resp = response;

	//response packet header
	*resp++ = 0xff;
	*resp++ = 0xff;
	*resp++ = 0xff;
	*resp++ = 0xff;
	if (fullstatus)
		Q_strncpyz(resp, "statusResponse", sizeof(response) - (resp-response) - 1);
	else
		Q_strncpyz(resp, "infoResponse", sizeof(response) - (resp-response) - 1);
	resp += strlen(resp);
	*resp++ = '\n';

	//first line is the serverinfo
	Q_strncpyz(resp, svs.info, sizeof(response) - (resp-response));
	//this is a DP protocol query, so some QW fields are not needed
	Info_RemoveKey(resp, "maxclients");
	Info_RemoveKey(resp, "map");
	Info_SetValueForKey(resp, "gamename", com_protocolname.string, sizeof(response) - (resp-response));
	Info_SetValueForKey(resp, "modname", com_modname.string, sizeof(response) - (resp-response));
	Info_SetValueForKey(resp, "protocol", va("%d", NET_PROTOCOL_VERSION), sizeof(response) - (resp-response));
	Info_SetValueForKey(resp, "clients", va("%d", numclients), sizeof(response) - (resp-response));
	Info_SetValueForKey(resp, "sv_maxclients", maxclients.string, sizeof(response) - (resp-response));
	Info_SetValueForKey(resp, "mapname", Info_ValueForKey(svs.info, "map"), sizeof(response) - (resp-response));
	Info_SetValueForKey(resp, "qcstatus", gamestatus, sizeof(response) - (resp-response));
	Info_SetValueForKey(resp, "challenge", challenge, sizeof(response) - (resp-response));
	resp += strlen(resp);

	*resp++ = 0;	//there's already a null, but hey

	if (fullstatus)
	{
		client_t *cl;
		char *start = resp;

		if (resp != response+sizeof(response))
		{
			resp[-1] = '\n';	//replace the null terminator that we already wrote

			//on the following lines we have an entry for each client
			for (i=0 ; i<MAX_CLIENTS ; i++)
			{
				cl = &svs.clients[i];
				if ((cl->state == cs_connected || cl->state == cs_spawned || cl->name[0]) && !cl->spectator)
				{
					Q_strncpyz(resp, va(
									"%d %d \"%s\" \"%s\"\n"
									,
									cl->old_frags,
									SV_CalcPing(cl, false),
									cl->team,
									cl->name
									), sizeof(response) - (resp-response));
					resp += strlen(resp);
				}
			}

			*resp++ = 0;	//this might not be a null
			if (resp == response+sizeof(response))
			{
				//we're at the end of the buffer, it's full. bummer
				//replace 12 bytes with infoResponse
				memcpy(response+4, "infoResponse", 12);
				//move down by len(statusResponse)-len(infoResponse) bytes
				memmove(response+4+12, response+4+14, resp-response-(4+14));
				start -= 14-12; //fix this pointer

				resp = start;
				resp[-1] = 0;	//reset the \n
			}
		}
	}

	NET_SendPacket (NS_SERVER, resp-response, response, net_from);
}
#endif

#ifdef Q2SERVER
void SVC_InfoQ2 (void)
{
	char	string[64];
	int		i, count;
	int		version;

	if (maxclients.value == 1)
		return;		// ignore in single player

	version = atoi (Cmd_Argv(1));

	if (version != PROTOCOL_VERSION_Q2)
		snprintf (string, sizeof(string), "%s: wrong version\n", hostname.string);
	else
	{
		count = 0;
		for (i=0 ; i<maxclients.value ; i++)
			if (svs.clients[i].state >= cs_connected)
				count++;

		snprintf (string, sizeof(string), "%16s %8s %2i/%2i\n", hostname.string, sv.name, count, (int)maxclients.value);
	}

	Netchan_OutOfBandPrint (NS_SERVER, net_from, "info\n%s", string);
}
#endif

/*
===================
SV_CheckLog

===================
*/
#define	LOG_HIGHWATER	4096
#define	LOG_FLUSH		10*60
void SV_CheckLog (void)
{
	sizebuf_t	*sz;

	sz = &svs.log[svs.logsequence&1];

	// bump sequence if allmost full, or ten minutes have passed and
	// there is something still sitting there
	if (sz->cursize > LOG_HIGHWATER
	|| (realtime - svs.logtime > LOG_FLUSH && sz->cursize) )
	{
		// swap buffers and bump sequence
		svs.logtime = realtime;
		svs.logsequence++;
		sz = &svs.log[svs.logsequence&1];
		sz->cursize = 0;
		Con_Printf ("beginning fraglog sequence %i\n", svs.logsequence);
	}

}

/*
================
SVC_Log

Responds with all the logged frags for ranking programs.
If a sequence number is passed as a parameter and it is
the same as the current sequence, an A2A_NACK will be returned
instead of the data.
================
*/
void SVC_Log (void)
{
	int		seq;
	char	data[MAX_DATAGRAM+64];
	char	adr[MAX_ADR_SIZE];

	if (Cmd_Argc() == 2)
		seq = atoi(Cmd_Argv(1));
	else
		seq = -1;

	if (seq == svs.logsequence-1 || !sv_fraglogfile)
	{	// they already have this data, or we aren't logging frags
		data[0] = A2A_NACK;
		NET_SendPacket (NS_SERVER, 1, data, net_from);
		return;
	}

	Con_DPrintf ("sending log %i to %s\n", svs.logsequence-1, NET_AdrToString(adr, sizeof(adr), net_from));

	sprintf (data, "stdlog %i\n", svs.logsequence-1);
	strcat (data, (char *)svs.log_buf[((svs.logsequence-1)&1)]);

	NET_SendPacket (NS_SERVER, strlen(data)+1, data, net_from);
}

/*
================
SVC_Ping

Just responds with an acknowledgement
================
*/
void SVC_Ping (void)
{
	char	data;

	data = A2A_ACK;

	NET_SendPacket (NS_SERVER, 1, &data, net_from);
}

//from net_from
int SV_NewChallenge (void)
{
	int		i;
	int		oldest;
	int		oldestTime;

	oldest = 0;
	oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	for (i = 0 ; i < MAX_CHALLENGES ; i++)
	{
		if (NET_CompareBaseAdr (net_from, svs.challenges[i].adr))
			return svs.challenges[i].challenge;
		if (svs.challenges[i].time < oldestTime)
		{
			oldestTime = svs.challenges[i].time;
			oldest = i;
		}
	}

	// overwrite the oldest
	svs.challenges[oldest].challenge = (rand() << 16) ^ rand();
	svs.challenges[oldest].adr = net_from;
	svs.challenges[oldest].time = realtime;

	return svs.challenges[oldest].challenge;
}

/*
=================
SVC_GetChallenge

Returns a challenge number that can be used
in a subsequent client_connect command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.
=================
*/
void SVC_GetChallenge (void)
{
#ifdef HUFFNETWORK
	int compressioncrc;
#endif
	int		i;
	int		oldest;
	int		oldestTime;

	if (!sv_listen_qw.value && !sv_listen_dp.value && !sv_listen_q3.value)
		return;

	oldest = 0;
	oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	for (i = 0 ; i < MAX_CHALLENGES ; i++)
	{
		if (NET_CompareBaseAdr (net_from, svs.challenges[i].adr))
			break;
		if (svs.challenges[i].time < oldestTime)
		{
			oldestTime = svs.challenges[i].time;
			oldest = i;
		}
	}

	if (i == MAX_CHALLENGES)
	{
		// overwrite the oldest
		svs.challenges[oldest].challenge = (rand() << 16) ^ rand();
		svs.challenges[oldest].adr = net_from;
		svs.challenges[oldest].time = realtime;
		i = oldest;
	}

	// send it back
	{
		char *buf;
		int lng;
		char *over;

#ifdef Q3SERVER
		if (svs.gametype == GT_QUAKE3)	//q3 servers
			buf = va("challengeResponse %i", svs.challenges[i].challenge);
		else
#endif
#ifdef Q2SERVER
			if (svs.gametype == GT_QUAKE2)	//quake 2 servers give a different challenge responce
			buf = va("challenge %i", svs.challenges[i].challenge);
		else
#endif
			buf = va("%c%i", S2C_CHALLENGE, svs.challenges[i].challenge);

		over = buf + strlen(buf) + 1;

		if (svs.gametype == GT_PROGS || svs.gametype == GT_Q1QVM)
		{
#ifdef PROTOCOL_VERSION_FTE
			unsigned int mask;
			//tell the client what fte extensions we support
			mask = Net_PextMask(1);
			if (mask)
			{
				lng = LittleLong(PROTOCOL_VERSION_FTE);
				memcpy(over, &lng, sizeof(lng));
				over+=sizeof(lng);

				lng = LittleLong(mask);
				memcpy(over, &lng, sizeof(lng));
				over+=sizeof(lng);
			}
			//tell the client what fte extensions we support
			mask = Net_PextMask(2);
			if (mask)
			{
				lng = LittleLong(PROTOCOL_VERSION_FTE2);
				memcpy(over, &lng, sizeof(lng));
				over+=sizeof(lng);

				lng = LittleLong(mask);
				memcpy(over, &lng, sizeof(lng));
				over+=sizeof(lng);
			}
#endif

			mask = net_mtu.ival&~7;
			if (mask > 64)
			{
				lng = LittleLong(PROTOCOL_VERSION_FRAGMENT);
				memcpy(over, &lng, sizeof(lng));
				over+=sizeof(lng);

				lng = LittleLong(mask);
				memcpy(over, &lng, sizeof(lng));
				over+=sizeof(lng);
			}

#ifdef HUFFNETWORK
			compressioncrc = Huff_PreferedCompressionCRC();
			if (compressioncrc)
			{
				lng = LittleLong((('H'<<0) + ('U'<<8) + ('F'<<16) + ('F' << 24)));
				memcpy(over, &lng, sizeof(lng));
				over+=sizeof(lng);

				lng = LittleLong(compressioncrc);
				memcpy(over, &lng, sizeof(lng));
				over+=sizeof(lng);
			}
#endif
		}
		if (sv_listen_qw.value || (svs.gametype != GT_PROGS && svs.gametype != GT_Q1QVM))
			Netchan_OutOfBand(NS_SERVER, net_from, over-buf, buf);

		if (sv_listen_dp.value && (sv_listen_nq.value || sv_bigcoords.value || !sv_listen_qw.value))
		{
		//dp (protocol6 upwards) can respond to this (and fte won't get confused because the challenge will be wrong)
			buf = va("challenge "DISTRIBUTION"%i", svs.challenges[i].challenge);
			Netchan_OutOfBand(NS_SERVER, net_from, strlen(buf)+1, buf);
		}
#ifdef Q3SERVER
		if (svs.gametype == GT_PROGS || svs.gametype == GT_Q1QVM)
		{
			if (sv_listen_q3.value)
			{
				buf = va("challengeResponse %i", svs.challenges[i].challenge);
				Netchan_OutOfBand(NS_SERVER, net_from, strlen(buf), buf);
	}
		}
#endif
	}

//	Netchan_OutOfBandPrint (net_from, "%c%i", S2C_CHALLENGE,
//				svs.challenges[i].challenge);
}

void SV_GetNewSpawnParms(client_t *cl)
{
	int i;
	if (svprogfuncs)	//q2 dlls don't use parms in this mannor. It's all internal to the dll.
	{
		// call the progs to get default spawn parms for the new client
#ifdef VM_Q1
		if (svs.gametype == GT_Q1QVM)
			Q1QVM_SetNewParms();
		else
#endif
		{
			if (pr_global_ptrs->SetNewParms)
				PR_ExecuteProgram (svprogfuncs, pr_global_struct->SetNewParms);
		}
		for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
		{
			if (pr_global_ptrs->spawnparamglobals[i])
				cl->spawn_parms[i] = *pr_global_ptrs->spawnparamglobals[i];
			else
				cl->spawn_parms[i] = 0;
		}
	}
}

void VARGS SV_OutOfBandPrintf (int q2, netadr_t adr, char *format, ...)
{
	va_list		argptr;
	char		string[8192];

	va_start (argptr, format);
	if (q2)
	{
		strcpy(string, "print\n");
		vsnprintf (string+6,sizeof(string)-1-6, format+1,argptr);
	}
	else
	{
		string[0] = A2C_PRINT;
		string[1] = '\n';
		vsnprintf (string+2,sizeof(string)-1-2, format,argptr);
	}
	va_end (argptr);


	Netchan_OutOfBand (NS_SERVER, adr, strlen(string), (qbyte *)string);
}
void VARGS SV_OutOfBandTPrintf (int q2, netadr_t adr, int language, translation_t text, ...)
{
	va_list		argptr;
	char		string[8192];
	char *format = langtext(text, language);

	va_start (argptr, text);
	if (q2)
	{
		strcpy(string, "print\n");
		vsnprintf (string+6,sizeof(string)-1-6, format+1,argptr);
	}
	else
	{
		string[0] = A2C_PRINT;
		vsnprintf (string+1,sizeof(string)-1-1, format,argptr);
	}
	va_end (argptr);


	Netchan_OutOfBand (NS_SERVER, adr, strlen(string), (qbyte *)string);
}

qboolean SV_ChallengePasses(int challenge)
{
	int i;
	for (i=0 ; i<MAX_CHALLENGES ; i++)
	{	//one per ip.
		if (NET_CompareBaseAdr (net_from, svs.challenges[i].adr))
		{
			if (challenge == svs.challenges[i].challenge)
				return true;
			return false;
		}
	}
	return false;
}

void VARGS SV_RejectMessage(int protocol, char *format, ...)
{
	va_list		argptr;
	char		string[8192];
	int len;

	va_start (argptr, format);
	switch(protocol)
	{
#ifdef NQPROT
	case SCP_NETQUAKE:
	case SCP_FITZ666:
		string[4] = CCREP_REJECT;
		vsnprintf (string+5,sizeof(string)-1-5, format,argptr);
		len = strlen(string+4)+1+4;
		*(int*)string = BigLong(NETFLAG_CTL|len);
		NET_SendPacket(NS_SERVER, len, string, net_from);
		return;
	case SCP_DARKPLACES6:
	case SCP_DARKPLACES7:
		strcpy(string, "reject ");
		vsnprintf (string+7,sizeof(string)-1-7, format,argptr);
		len = strlen(string);
		break;
#endif

	case SCP_QUAKE2:
	case SCP_QUAKE3:
	default:
		strcpy(string, "print\n");
		vsnprintf (string+6,sizeof(string)-1-6, format,argptr);
		len = strlen(string);
		break;

	case SCP_QUAKEWORLD:
		string[0] = A2C_PRINT;
		string[1] = '\n';
		vsnprintf (string+2,sizeof(string)-1-2, format,argptr);
		len = strlen(string);
		break;
	}
	va_end (argptr);

	Netchan_OutOfBand (NS_SERVER, net_from, len, (qbyte *)string);
}

void SV_AcceptMessage(int protocol)
{
	char		string[8192];
	sizebuf_t	sb;
	int len;
#ifdef NQPROT
	netadr_t localaddr;
#endif

	memset(&sb, 0, sizeof(sb));
	sb.maxsize = sizeof(string);
	sb.data = string;

	switch(protocol)
	{
#ifdef NQPROT
	case SCP_NETQUAKE:
	case SCP_FITZ666:
		if (net_from.type != NA_LOOPBACK)
		{
			SZ_Clear(&sb);
			MSG_WriteLong(&sb, 0);
			MSG_WriteByte(&sb, CCREP_ACCEPT);
			NET_LocalAddressForRemote(svs.sockets, &net_from, &localaddr, 0);
			MSG_WriteLong(&sb, ShortSwap(localaddr.port));
			*(int*)sb.data = BigLong(NETFLAG_CTL|sb.cursize);
			NET_SendPacket(NS_SERVER, sb.cursize, sb.data, net_from);
			return;
		}
	case SCP_DARKPLACES6:
	case SCP_DARKPLACES7:
		strcpy(string, "accept");
		len = strlen(string);
		break;
#endif

	case SCP_QUAKE3:
		strcpy(string, "connectResponse");
		len = strlen(string);
		break;
	case SCP_QUAKE2:
	default:
		strcpy(string, "client_connect\n");
		len = strlen(string);
		break;

	case SCP_QUAKEWORLD:
		string[0] = S2C_CONNECTION;
		string[1] = '\n';
		string[2] = '\0';
		len = strlen(string);
		break;
	}

	Netchan_OutOfBand (NS_SERVER, net_from, len, (qbyte *)string);
}

#ifndef _WIN32
#include <sys/stat.h>
static void SV_CheckRecentCrashes(client_t *tellclient)
{
	struct stat sb;
	if (-1 != stat("crash.log", &sb))
	{
		SV_ClientPrintf(tellclient, PRINT_HIGH, "\1WARNING: crash.log exists, dated %s\n", ctime(&sb.st_mtime));
	}
}
#else
static void SV_CheckRecentCrashes(client_t *tellclient)
{
}
#endif

/*
==================
SVC_DirectConnect

A connection request that did not come from the master
==================
*/
int	nextuserid;
client_t *SVC_DirectConnect(void)
{
	char		userinfo[MAX_CLIENTS][2048];
	netadr_t	adr;
	int			i;
	client_t	*cl, *newcl;
	client_t	temp;
	edict_t		*ent;
#ifdef Q2SERVER
	q2edict_t	*q2ent;
#endif
	int			edictnum;
	char		*s;
	int			clients, spectators;
	qboolean	spectator;
	int			qport;
	int			version;
	int			challenge;
	int			huffcrc = 0;
	int			mtu = 0;
	char guid[128] = "";
	char basic[80];
	qboolean	redirect = false;

	int maxpacketentities;

	int numssclients = 1;

	int protocol;

	unsigned int protextsupported=0;
	unsigned int protextsupported2=0;


	char *name;
	char adrbuf[MAX_ADR_SIZE];

	if (*Cmd_Argv(1) == '\\')
	{	//connect "\key\val"

		//this is used by q3 (note, we already decrypted the huffman connection packet in a hack)
		if (!sv_listen_q3.value)
			return NULL;
		numssclients = 1;
		protocol = SCP_QUAKE3;

		Q_strncpyz (userinfo[0], Cmd_Argv(1), sizeof(userinfo[0])-1);

		switch (atoi(Info_ValueForKey(userinfo[0], "protocol")))
		{
		case 68:	//regular q3 1.32
			break;
//		case 43:	//q3 1.11 (most 'recent' demo)
//			break;
		default:
			SV_RejectMessage (SCP_BAD, "Server is %s.\n", version_string());
			Con_Printf ("* rejected connect from incompatable client\n");
			return NULL;
		}

		s = Info_ValueForKey(userinfo[0], "challenge");
		challenge = atoi(s);

		s = Info_ValueForKey(userinfo[0], "qport");
		qport = atoi(s);

		s = Info_ValueForKey(userinfo[0], "name");
		if (!*s)
			Info_SetValueForKey(userinfo[0], "name", "UnnamedQ3", sizeof(userinfo[0]));
	}
	else if (*(Cmd_Argv(0)+7) == '\\')
	{	//DP has the userinfo attached directly to the end of the connect command
		if (!sv_listen_dp.value)
			return NULL;
		Q_strncpyz (userinfo[0], net_message.data + 11, sizeof(userinfo[0])-1);

		if (strcmp(Info_ValueForKey(userinfo[0], "protocol"), "darkplaces 3"))
		{
			SV_RejectMessage (SCP_BAD, "Server is %s.\n", version_string());
			Con_Printf ("* rejected connect from incompatible client\n");
			return NULL;
		}
		//it's a darkplaces client.

		s = Info_ValueForKey(userinfo[0], "protocols");
		if (svs.netprim.coordsize != 4)
		{	//we allow nq with sv_listen_nq 0...
			//reason: dp is too similar for concerns about unsupported code, while the main reason why we disable nq is because of the lack of challenges
			//(and no, this isn't a way to bypass invalid challenges)
			protocol = SCP_NETQUAKE;
			Con_Printf ("* DP without sv_bigcoords 1\n");
		}
		else if (strstr(s, "DP7"))
			protocol = SCP_DARKPLACES7;
		else
			protocol = SCP_DARKPLACES6;

		s = Info_ValueForKey(userinfo[0], "challenge");
		if (!strncmp(s, DISTRIBUTION, strlen(DISTRIBUTION)))
			challenge = atoi(s+3);
		else
			challenge = atoi(s);

		Info_RemoveKey(userinfo[0], "protocol");
		Info_RemoveKey(userinfo[0], "protocols");
		Info_RemoveKey(userinfo[0], "challenge");

		s = Info_ValueForKey(userinfo[0], "name");
		if (!*s)
			Info_SetValueForKey(userinfo[0], "name", "CONNECTING", sizeof(userinfo[0]));

		qport = 0;
	}
	else
	{
		if (atoi(Cmd_Argv(0)+7))
		{
			numssclients = atoi(Cmd_Argv(0)+7);
			if (numssclients<1 || numssclients > 4)
			{
				SV_RejectMessage (SCP_BAD, "Server is %s.\n", version_string());
				Con_Printf ("* rejected connect from broken client\n");
				return NULL;
			}
		}

		version = atoi(Cmd_Argv(1));
		if (version >= 31 && version <= 34)
		{
			numssclients = 1;
			protocol = SCP_QUAKE2;
		}
		else if (version == 3)
		{
			numssclients = 1;
			protocol = SCP_NETQUAKE; //because we can
		}
		else if (version != PROTOCOL_VERSION_QW)
		{
			SV_RejectMessage (SCP_BAD, "Server is protocol version %i, received %i\n", PROTOCOL_VERSION_QW, version);
			Con_Printf ("* rejected connect from version %i\n", version);
			return NULL;
		}
		else
			protocol = SCP_QUAKEWORLD;

		qport = atoi(Cmd_Argv(2));

		challenge = atoi(Cmd_Argv(3));

		// note an extra qbyte is needed to replace spectator key
		for (i = 0; i < numssclients; i++)
			Q_strncpyz (userinfo[i], Cmd_Argv(4+i), sizeof(userinfo[i])-1);
	}

	for (i = 0; i < numssclients; i++)
	{
		//don't let users exploit mods made for mvdsv instead of FTE
		Info_RemoveKey (userinfo[i], "*VIP");
	}

	if (protocol == SCP_QUAKEWORLD)	//readd?
	{
		if (!sv_listen_qw.value && net_from.type != NA_LOOPBACK)
		{
			SV_RejectMessage (SCP_BAD, "QuakeWorld protocols are not permitted on this server.\n");
			Con_Printf ("* rejected connect from quakeworld\n");
			return NULL;
		}
	}



	while(!msg_badread)
	{
		Cmd_TokenizeString(MSG_ReadStringLine(), false, false);
		switch(Q_atoi(Cmd_Argv(0)))
		{
		case PROTOCOL_VERSION_FTE:
			if (protocol == SCP_QUAKEWORLD)
			{
				protextsupported = Q_atoi(Cmd_Argv(1));
				Con_DPrintf("Client supports 0x%x fte extensions\n", protextsupported);
			}
			break;
		case PROTOCOL_VERSION_FTE2:
			if (protocol == SCP_QUAKEWORLD)
			{
				protextsupported2 = Q_atoi(Cmd_Argv(1));
				Con_DPrintf("Client supports 0x%x fte2 extensions\n", protextsupported2);
			}
			break;
		case PROTOCOL_VERSION_HUFFMAN:
			huffcrc = Q_atoi(Cmd_Argv(1));
			Con_DPrintf("Client supports huffman compression. crc 0x%x\n", huffcrc);
			break;
		case PROTOCOL_VERSION_FRAGMENT:
			mtu = Q_atoi(Cmd_Argv(1)) & ~7;
			if (mtu < 64)
				mtu = 0;
			Con_DPrintf("Client supports fragmentation. mtu %i.\n", mtu);
			break;
		case PROTOCOL_INFO_GUID:
			Q_strncpyz(guid, Cmd_Argv(1), sizeof(guid));
			Con_DPrintf("GUID %s\n", Cmd_Argv(1));
			break;
		}
	}
	msg_badread=false;
	


	if (protextsupported & PEXT_256PACKETENTITIES)
		maxpacketentities = MAX_EXTENDED_PACKET_ENTITIES;
	else
		maxpacketentities = MAX_STANDARD_PACKET_ENTITIES;

	/*allow_splitscreen applies only to non-local clients, so that clients have only one enabler*/
	if (!sv_allow_splitscreen.ival && net_from.type != NA_LOOPBACK)
		numssclients = 1;

	if (!(protextsupported & PEXT_SPLITSCREEN))
		numssclients = 1;

	if (sv.msgfromdemo || net_from.type == NA_LOOPBACK)	//normal rules don't apply
		i=0;
	else
	{
	// see if the challenge is valid
		if (!SV_ChallengePasses(challenge))
		{
			SV_RejectMessage (protocol, "Bad challenge.\n");
			return NULL;
		}
	}

	// check for password or spectator_password
	if (svprogfuncs)
	{
		s = Info_ValueForKey (userinfo[0], "spectator");
		if (s[0] && strcmp(s, "0"))
		{
			if (spectator_password.string[0] &&
				stricmp(spectator_password.string, "none") &&
				strcmp(spectator_password.string, s) )
			{	// failed
				Con_Printf ("%s:spectator password failed\n", NET_AdrToString (adrbuf, sizeof(adrbuf), net_from));
				SV_RejectMessage (protocol, "requires a spectator password\n\n");
				return NULL;
			}
			Info_RemoveKey (userinfo[0], "spectator"); // remove key
			Info_SetValueForStarKey (userinfo[0], "*spectator", "1", sizeof(userinfo[0]));
			spectator = true;
		}
		else
		{
			s = Info_ValueForKey (userinfo[0], "password");
			if (password.string[0] &&
				stricmp(password.string, "none") &&
				strcmp(password.string, s) )
			{
				Con_Printf ("%s:password failed\n", NET_AdrToString (adrbuf, sizeof(adrbuf), net_from));
				SV_RejectMessage (protocol, "server requires a password\n\n");
				return NULL;
			}
			spectator = false;
			Info_RemoveKey (userinfo[0], "password"); // remove passwd
			Info_RemoveKey (userinfo[0], "*spectator"); // remove key
		}
	}
	else
		spectator = false;//q2 does all of it's checks internally, and deals with spectator ship too

	adr = net_from;
	nextuserid++;	// so every client gets a unique id

	newcl = &temp;
	memset (newcl, 0, sizeof(client_t));

	newcl->userid = nextuserid;
	newcl->fteprotocolextensions = protextsupported;
	newcl->fteprotocolextensions2 = protextsupported2;
	newcl->protocol = protocol;
	Q_strncpyz(newcl->guid, guid, sizeof(newcl->guid));

	newcl->maxmodels = 256;
	if (protocol == SCP_QUAKEWORLD)	//readd?
	{
		newcl->max_net_ents = 512;
		if (newcl->fteprotocolextensions & PEXT_ENTITYDBL)
			newcl->max_net_ents += 512;
		if (newcl->fteprotocolextensions & PEXT_ENTITYDBL2)
			newcl->max_net_ents += 1024;

		if (newcl->fteprotocolextensions & PEXT_MODELDBL)
			newcl->maxmodels = MAX_MODELS;
	}
	else if (ISDPCLIENT(newcl))
	{
		newcl->max_net_ents = 32767;
		newcl->maxmodels = 1024;
	}
	else
		newcl->max_net_ents = 600;

	if (sv.msgfromdemo)
		newcl->wasrecorded = true;

	// works properly
	if (!sv_highchars.value)
	{
		qbyte *p, *q;

		for (p = (qbyte *)newcl->userinfo, q = (qbyte *)userinfo;
			*q && p < (qbyte *)newcl->userinfo + sizeof(newcl->userinfo)-1; q++)
			if (*q > 31 && *q <= 127)
				*p++ = *q;
	}
	else
		Q_strncpyS (newcl->userinfo, userinfo[0], sizeof(newcl->userinfo)-1);
	newcl->userinfo[sizeof(newcl->userinfo)-1] = '\0';

	// if there is already a slot for this ip, drop it
	for (i=0,cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
	{
		if (cl->state == cs_free)
			continue;
		if (NET_CompareBaseAdr (adr, cl->netchan.remote_address)
			&& ( cl->netchan.qport == qport
			|| adr.port == cl->netchan.remote_address.port ))
		{
			if (cl->state == cs_connected)
			{
				if (cl->protocol != protocol)
					Con_Printf("%s: diff prot connect\n", NET_AdrToString (adrbuf, sizeof(adrbuf), adr));
				else
					Con_Printf("%s:dup connect\n", NET_AdrToString (adrbuf, sizeof(adrbuf), adr));

				SV_DropClient(cl);
				/*
				nextuserid--;
				return NULL;
				*/
			}
			else if (cl->state == cs_zombie)
			{
				Con_Printf ("%s:reconnect\n", NET_AdrToString (adrbuf, sizeof(adrbuf), adr));
				SV_DropClient (cl);
			}
			else
			{
				Con_Printf ("%s:reconnect\n", NET_AdrToString (adrbuf, sizeof(adrbuf), adr));
//				SV_DropClient (cl);
			}
			break;
		}
	}

	name = Info_ValueForKey (temp.userinfo, "name");

	if (protocol == SCP_QUAKEWORLD &&!atoi(Info_ValueForKey (temp.userinfo, "iknow")))
	{
		if (sv.world.worldmodel->fromgame == fg_halflife && !(newcl->fteprotocolextensions & PEXT_HLBSP))
		{
			if (atof(Info_ValueForKey (temp.userinfo, "*FuhQuake")) < 0.3)
			{
				SV_RejectMessage (protocol, "The server is using a halflife level and we don't think your client supports this\nuse 'setinfo iknow 1' to ignore this check\nYou can go to "ENGINEWEBSITE" to get a compatible client\n\nYou may need to enable an option\n\n");
//				Con_Printf("player %s was dropped due to incompatible client\n", name);
//				return;
			}
		}
#ifdef PEXT_Q2BSP
		else if (sv.world.worldmodel->fromgame == fg_quake2 && !(newcl->fteprotocolextensions & PEXT_Q2BSP))
		{
			SV_RejectMessage (protocol, "The server is using a quake 2 level and we don't think your client supports this\nuse 'setinfo iknow 1' to ignore this check\nYou can go to "ENGINEWEBSITE" to get a compatible client\n\nYou may need to enable an option\n\n");
//			Con_Printf("player %s was dropped due to incompatible client\n", name);
//			return;
		}
#endif
#ifdef PEXT_Q3BSP
		else if (sv.world.worldmodel->fromgame == fg_quake3 && !(newcl->fteprotocolextensions & PEXT_Q3BSP))
		{
			SV_RejectMessage (protocol, "The server is using a quake 3 level and we don't think your client supports this\nuse 'setinfo iknow 1' to ignore this check\nYou can go to "ENGINEWEBSITE" to get a compatible client\n\nYou may need to enable an option\n\n");
//			Con_Printf("player %s was dropped due to incompatible client\n", name);
//			return;
		}
#endif
	}

	SV_FixupName(name, temp.namebuf, sizeof(temp.namebuf));
	name = temp.namebuf;

	deleetstring(basic, name);
	if (!*basic || strstr(basic, "console"))
		name = "unnamed";	//have fun dudes.

	// count up the clients and spectators
	clients = 0;
	spectators = 0;
	newcl = NULL;
	if (!sv.allocated_client_slots)
	{
		Con_Printf("Apparently, there are no client slots allocated. This shouldn't be happening\n");
		return NULL;
	}
	for (i=0,cl=svs.clients ; i<sv.allocated_client_slots ; i++,cl++)
	{
		if (cl->state == cs_free)
			continue;
		if (cl->spectator)
			spectators++;
		else
			clients++;

		if (cl->istobeloaded && cl->state == cs_zombie)
		{
			if (!newcl)
			{
				if (!strcmp(cl->name, name) || !*cl->name)	//named, or first come first serve.
				{
					newcl = cl;
					temp.istobeloaded = cl->istobeloaded;
					break;
				}
			}
		}
	}

	if (!newcl)	//client has no slot. It's possible to bipass this if server is loading a game. (or a duplicated qsocket)
	{
		/*single player logic*/
		if (sv.allocated_client_slots == 1 && net_from.type == NA_LOOPBACK)
			if (svs.clients[0].state >= cs_connected)
				SV_DropClient(svs.clients);

		// if at server limits, refuse connection
		if ( maxclients.ival > MAX_CLIENTS )
			Cvar_SetValue (&maxclients, MAX_CLIENTS);
		if (maxspectators.ival > MAX_CLIENTS)
			Cvar_SetValue (&maxspectators, MAX_CLIENTS);

		// find a free client slot
		for (i=0,cl=svs.clients ; i<sv.allocated_client_slots ; i++,cl++)
		{
			if (cl->state == cs_free)
			{
				newcl = cl;
				break;
			}
		}

		/*only q1/h2 has maxclients/maxspectators limits. q2 or q3 the gamecode does this*/
		if (svprogfuncs)
		{
			if (spectator && spectators >= maxspectators.ival)
				redirect = true;
			if (!spectator && clients >= maxclients.ival)
				redirect = true;
		}
		else
		{
			if (clients >= maxclients.ival)
				redirect = true;
		}

		if (redirect)
		{
			extern cvar_t sv_fullredirect;
			if (!*sv_fullredirect.string)
				newcl = NULL;
		}

		if (!newcl)
		{
			if (!svprogfuncs)
			{
				SV_RejectMessage (protocol, "\nserver is full\n\n");
				Con_Printf ("%s:full connect\n", NET_AdrToString (adrbuf, sizeof(adrbuf), adr));
			}
			else
			{
				if (spectator && spectators >= maxspectators.ival)
				{
					SV_RejectMessage (protocol, "\nserver is full (%i of %i spectators)\n\n", spectators, maxspectators.ival);
					Con_Printf ("%s:full connect\n", NET_AdrToString (adrbuf, sizeof(adrbuf), adr));
				}
				else if (!spectator && clients >= maxclients.ival)
					SV_RejectMessage (protocol, "\nserver is full (%i of %i players)\n\n", clients, maxclients.ival);
				else
					SV_RejectMessage (protocol, "\nserver is full (%i of %i connections)\n\n", clients+spectators, sv.allocated_client_slots);
			}
			return NULL;
		}
	}

	{
		char *banreason = SV_BannedReason(&adr);
		if (banreason)
		{
			if (*banreason)
				SV_RejectMessage (protocol, "You were banned.\nReason: %s\n", banreason);
			else
				SV_RejectMessage (protocol, "You were banned.\n");
			return NULL;
		}
	}

	//set up the gamecode for this player, optionally drop them here
	edictnum = (newcl-svs.clients)+1;
	switch (svs.gametype)
	{
#ifdef HLSERVER
		//fallthrough
	case GT_HALFLIFE:
		{
			char reject[128];
			if (!SVHL_ClientConnect(newcl, adr, reject))
			{
				SV_RejectMessage(protocol, "%s", reject);
				return NULL;
			}
		}

		break;
#endif


#ifdef VM_Q1
	case GT_Q1QVM:
#endif
	case GT_PROGS:
		ent = EDICT_NUM(svprogfuncs, edictnum);
#ifdef Q2SERVER
		temp.q2edict = NULL;
#endif
		temp.edict = ent;

		break;

#ifdef Q2SERVER
	case GT_QUAKE2:
		q2ent = Q2EDICT_NUM(edictnum);
		temp.edict = NULL;
		temp.q2edict = q2ent;

		if (!ge->ClientConnect(q2ent, temp.userinfo))
			return NULL;

		ge->ClientUserinfoChanged(q2ent, temp.userinfo);


		break;
#endif
	default:
		Sys_Error("Bad svs.gametype in SVC_DirectConnect");
		break;
	}

	//initialise the client's frames, based on that client's protocol
	switch(temp.protocol)
	{
#ifdef Q3SERVER
	case SCP_QUAKE3:
		Huff_PreferedCompressionCRC();
		if (temp.frameunion.q3frames)
			Z_Free(temp.frameunion.q3frames);
		temp.frameunion.q3frames = Z_Malloc(Q3UPDATE_BACKUP*sizeof(*temp.frameunion.q3frames));
		break;
#endif

#ifdef Q2SERVER
	case SCP_QUAKE2:
		// build a new connection
		// accept the new client
		// this is the only place a client_t is ever initialized
		temp.frameunion.q2frames = newcl->frameunion.q2frames;	//don't touch these.
		if (temp.frameunion.q2frames)
			Z_Free(temp.frameunion.q2frames);

		temp.frameunion.q2frames = Z_Malloc(sizeof(q2client_frame_t)*Q2UPDATE_BACKUP);
		break;
#endif

	default:
		temp.frameunion.frames = newcl->frameunion.frames;	//don't touch these.
		if (temp.frameunion.frames)
		{
			Con_Printf("Debug: frames were set?\n");
			Z_Free(temp.frameunion.frames);
		}

		if ((temp.fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS))// || ISDPCLIENT(&temp))
		{
			char *ptr;
			int maxents = maxpacketentities;	/*this is the max number of ents updated per frame. we can't track more, so...*/
			ptr = Z_Malloc(	sizeof(client_frame_t)*UPDATE_BACKUP+
							sizeof(*temp.pendingentbits)*temp.max_net_ents+
							sizeof(unsigned int)*maxents*UPDATE_BACKUP+
							sizeof(unsigned short)*maxents*UPDATE_BACKUP);
			temp.frameunion.frames = (void*)ptr;
			ptr += sizeof(*temp.frameunion.frames)*UPDATE_BACKUP;
			temp.pendingentbits = (void*)ptr;
			ptr += sizeof(*temp.pendingentbits)*temp.max_net_ents;
			for (i = 0; i < UPDATE_BACKUP; i++)
			{
				temp.frameunion.frames[i].entities.max_entities = maxents;
				temp.frameunion.frames[i].resendentnum = (void*)ptr;
				ptr += sizeof(*temp.frameunion.frames[i].resendentnum)*maxents;
				temp.frameunion.frames[i].resendentbits = (void*)ptr;
				ptr += sizeof(*temp.frameunion.frames[i].resendentbits)*maxents;
			}
		}
		else
		{
			temp.frameunion.frames = Z_Malloc((sizeof(client_frame_t)+sizeof(entity_state_t)*maxpacketentities)*UPDATE_BACKUP);
			for (i = 0; i < UPDATE_BACKUP; i++)
			{
				temp.frameunion.frames[i].entities.max_entities = maxpacketentities;
				temp.frameunion.frames[i].entities.entities = (entity_state_t*)(temp.frameunion.frames+UPDATE_BACKUP) + i*temp.frameunion.frames[i].entities.max_entities;
			}
		}
		break;
	}


	{
		char *n, *t;
		n = newcl->name;
		t = newcl->team;
		*newcl = temp;
		newcl->name = n;
		newcl->team = t;
	}

	newcl->challenge = challenge;
	newcl->zquake_extensions = atoi(Info_ValueForKey(newcl->userinfo, "*z_ext"));
	if (*Info_ValueForKey(newcl->userinfo, "*fuhquake"))	//fuhquake doesn't claim to support z_ext but does look at our z_ext serverinfo key.
	{														//so switch on the bits that it should be sending.
		newcl->zquake_extensions |= Z_EXT_PM_TYPE|Z_EXT_PM_TYPE_NEW;
	}
	newcl->zquake_extensions &= SUPPORTED_Z_EXTENSIONS;

	Netchan_Setup (NS_SERVER, &newcl->netchan, adr, qport);

	if (huffcrc)
		newcl->netchan.compress = true;
	else
		newcl->netchan.compress = false;
	if (mtu >= 64)
	{
		newcl->netchan.fragmentsize = mtu;
		newcl->netchan.message.maxsize = sizeof(newcl->netchan.message_buf);
	}

	newcl->protocol = protocol;
#ifdef NQPROT
	newcl->netchan.isnqprotocol = ISNQCLIENT(newcl);
#endif

	newcl->state = cs_connected;

#ifdef Q3SERVER
	newcl->gamestatesequence = -1;
#endif
	newcl->datagram.allowoverflow = true;
	newcl->datagram.data = newcl->datagram_buf;
	newcl->datagram.maxsize = sizeof(newcl->datagram_buf);

	newcl->netchan.netprim = svs.netprim;
	newcl->datagram.prim = svs.netprim;
	newcl->netchan.message.prim = svs.netprim;

	// spectator mode can ONLY be set at join time
	newcl->spectator = spectator;

	newcl->realip_ping = (((rand()^(rand()<<8) ^ *(int*)&realtime)&0xffffff)<<8) | (newcl-svs.clients);

	if (newcl->istobeloaded)
		newcl->playerclass = newcl->edict->xv->playerclass;

	// parse some info from the info strings
	SV_ExtractFromUserinfo (newcl);
	SV_GenerateBasicUserInfo (newcl);

	// JACK: Init the floodprot stuff.
	newcl->floodprotmessage = 0.0;
	newcl->lastspoke = 0.0;
	newcl->lockedtill = 0;

#ifdef SVRANKING
	if (svs.demorecording || (svs.demoplayback && newcl->wasrecorded))	//disable rankings. Could cock things up.
	{
		SV_GetNewSpawnParms(newcl);
	}
	else
	{
//rankid is figured out in extract from user info
		if (!newcl->rankid)	//failed to get a userid
		{
			if (rank_needlogin.value)
			{
				SV_RejectMessage (protocol, "Bad password/username\nThis server requires logins. Please see the serverinfo for website and info on how to register.\n");
				newcl->state = cs_free;
				return NULL;
			}

//			SV_OutOfBandPrintf (isquake2client, adr, "\nWARNING: You have not got a place on the ranking system, probably because a user with the same name has already connected and your pwds differ.\n\n");

			SV_GetNewSpawnParms(newcl);
		}
		else
		{
			rankstats_t rs;
			if (!Rank_GetPlayerStats(newcl->rankid, &rs))
			{
				SV_RejectMessage (protocol, "Rankings/Account system failed\n");
				Con_Printf("banned player %s is trying to connect\n", newcl->name);
				newcl->name[0] = 0;
				memset (newcl->userinfo, 0, sizeof(newcl->userinfo));
				newcl->state = cs_free;
				return NULL;
			}

			if (rs.flags1 & RANK_MUTED)
			{
				SV_BroadcastTPrintf(PRINT_MEDIUM, STL_CLIENTISSTILLMUTED, newcl->name);
			}
			if (rs.flags1 & RANK_CUFFED)
			{
				SV_BroadcastTPrintf(PRINT_LOW, STL_CLIENTISSTILLCUFFED, newcl->name);
			}
			if (rs.flags1 & RANK_CRIPPLED)
			{
				SV_BroadcastTPrintf(PRINT_HIGH, STL_CLIENTISSTILLCRIPPLED, newcl->name);
			}

			if (rs.timeonserver)
			{
				if (cl->istobeloaded)
				{	//do nothing.
				}
				else if (sv_resetparms.value)
				{
					SV_GetNewSpawnParms(newcl);
				}
				else
				{
					extern cvar_t rank_parms_first, rank_parms_last;
					for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
					{
						if (i < NUM_RANK_SPAWN_PARMS && i >= rank_parms_first.ival && i <= rank_parms_last.ival)
							newcl->spawn_parms[i] = rs.parm[i];
						else
							newcl->spawn_parms[i] = 0;
					}
				}

				if (rs.timeonserver > 3*60)	//woo. Ages.
					s = va(langtext(STL_BIGGREETING, newcl->language), newcl->name, (int)(rs.timeonserver/(60*60)), (int)((int)(rs.timeonserver/60)%(60)));
				else	//measure this guy in minuites.
					s = va(langtext(STL_SHORTGREETING, newcl->language), newcl->name, (int)(rs.timeonserver/60));

				SV_OutOfBandPrintf (protocol == SCP_QUAKE2, adr, s);
			}
			else if (!cl->istobeloaded)
			{
				SV_GetNewSpawnParms(newcl);

				SV_OutOfBandTPrintf (protocol == SCP_QUAKE2, adr, newcl->language, STL_FIRSTGREETING, newcl->name, (int)rs.timeonserver);
			}
			//else loaded players already have their initial parms set
		}
	}
#else
	// call the progs to get default spawn parms for the new client
	if (!cl->istobeloaded)
	{
		SV_GetNewSpawnParms(newcl);
	}
#endif


	if (!newcl->wasrecorded)
	{
		SV_AcceptMessage (protocol);

		if (redirect)
		{
		}
		else if (newcl->spectator)
		{
			SV_BroadcastTPrintf(PRINT_LOW, STL_SPECTATORCONNECTED, newcl->name);
//			Con_Printf ("Spectator %s connected\n", newcl->name);
		}
		else
		{
			SV_BroadcastTPrintf(PRINT_LOW, STL_CLIENTCONNECTED, newcl->name);
//			Con_DPrintf ("Client %s connected\n", newcl->name);
		}
	}
	else
	{
		if (newcl->spectator)
		{
			SV_BroadcastTPrintf(PRINT_LOW, STL_RECORDEDSPECTATORCONNECTED, newcl->name);
//			Con_Printf ("Recorded spectator %s connected\n", newcl->name);
		}
		else
		{
			SV_BroadcastTPrintf(PRINT_LOW, STL_RECORDEDCLIENTCONNECTED, newcl->name);
//			Con_DPrintf ("Recorded client %s connected\n", newcl->name);
		}
	}
	newcl->sendinfo = true;

	if (redirect)
		numssclients = 1;
	else
	{
		for (i = 0; i < sizeof(sv_motd)/sizeof(sv_motd[0]); i++)
		{
			if (*sv_motd[i].string)
				SV_ClientPrintf(newcl, PRINT_CHAT, "%s\n", sv_motd[i].string);
		}
	}

	SV_CheckRecentCrashes(newcl);

#ifdef VOICECHAT
	SV_VoiceInitClient(newcl);
#endif

	newcl->fteprotocolextensions &= ~PEXT_SPLITSCREEN;
	for (clients = 1; clients < numssclients; clients++)
	{
		for (i=0,cl=svs.clients ; i<sv.allocated_client_slots ; i++,cl++)
		{
			if (cl->state == cs_free)
			{
				break;
			}
		}
		if (i == sv.allocated_client_slots)
			break;

		newcl->fteprotocolextensions |= PEXT_SPLITSCREEN;

		temp.frameunion.frames = cl->frameunion.frames;	//don't touch these.
		temp.edict = cl->edict;
		memcpy(cl, newcl, sizeof(client_t));
		Q_strncatz(cl->guid, va("%i", clients), sizeof(cl->guid));
		cl->name = cl->namebuf;
		cl->team = cl->teambuf;

		nextuserid++;	// so every client gets a unique id
		cl->userid = nextuserid;

		cl->playerclass = 0;
		cl->frameunion.frames = temp.frameunion.frames;
		cl->edict = temp.edict;

		cl->fteprotocolextensions |= PEXT_SPLITSCREEN;

		if (newcl->controller)
		{
			newcl->controller->controlled = cl;
			newcl->controller = cl;
		}
		else
		{
			newcl->controlled = cl;
			newcl->controller = cl;
		}
		cl->controller = newcl;
		cl->controlled = NULL;

		Q_strncpyS (cl->userinfo, userinfo[clients], sizeof(cl->userinfo)-1);
		cl->userinfo[sizeof(cl->userinfo)-1] = '\0';

		if (spectator)
		{
			Info_RemoveKey (cl->userinfo, "spectator");
			Info_SetValueForStarKey (cl->userinfo, "*spectator", "1", sizeof(cl->userinfo));
		}
		else
			Info_RemoveKey (cl->userinfo, "*spectator");

		SV_ExtractFromUserinfo (cl);

		SV_GetNewSpawnParms(cl);
	}
	newcl->controller = NULL;

	if (!redirect)
	{
		Sys_ServerActivity();

		PIN_ShowMessages(newcl);
	}

	if (ISNQCLIENT(newcl))
	{
		newcl->netchan.message.maxsize = sizeof(newcl->netchan.message_buf);
		host_client = newcl;
		SVNQ_New_f();
	}

	newcl->redirect = redirect;


	return newcl;
}


int Rcon_Validate (void)
{
	if (!strlen (rcon_password.string))
		return 0;

	if (strcmp (Cmd_Argv(1), rcon_password.string) )
		return 0;

	return 1;
}

/*
===============
SVC_RemoteCommand

A client issued an rcon command.
Shift down the remaining args
Redirect all printfs
===============
*/
void SVC_RemoteCommand (void)
{
	int		i;
	char	remaining[1024];
	char	adr[MAX_ADR_SIZE];

	{
		char *br = SV_BannedReason(&net_from);
		if (br)
		{
			Con_Printf ("Rcon from banned ip %s\n", NET_AdrToString (adr, sizeof(adr), net_from));
			return;
		}
	}

	if (!Rcon_Validate ())
	{
#ifdef SVRANKING
		if (cmd_allowaccess.value)	//try and find a username, match the numeric password
		{
			int rid;
			char *s = Cmd_Argv(1);
			char *colon=NULL, *c2;
			rankstats_t stats;
			c2=s;
			for(;;)
			{
				c2 = strchr(c2, ':');
				if (!c2)
					break;
				colon = c2;
				c2++;
			}
			if (colon)	//oh, could this be a specific username?
			{
				*colon = '\0';
				colon++;
				rid = Rank_GetPlayerID(NULL, s, atoi(colon), false, true);
				if (rid)
				{
					if (!Rank_GetPlayerStats(rid, &stats))
						return;


					Con_Printf ("Rcon from %s:\n%s\n"
						, NET_AdrToString (adr, sizeof(adr), net_from), net_message.data+4);

					SV_BeginRedirect (RD_PACKET, LANGDEFAULT);

					remaining[0] = 0;

					for (i=2 ; i<Cmd_Argc() ; i++)
					{
						if (strlen(remaining)+strlen(Cmd_Argv(i))>=sizeof(remaining)-2)
						{
							Con_Printf("Rcon was too long\n");
							SV_EndRedirect ();
							Con_Printf ("Rcon from %s:\n%s\n"
								, NET_AdrToString (adr, sizeof(adr), net_from), "Was too long - possible buffer overflow attempt");
							return;
						}
						strcat (remaining, Cmd_Argv(i) );
						strcat (remaining, " ");
					}

					Cmd_ExecuteString (remaining, stats.trustlevel);

					SV_EndRedirect ();
					return;
				}
			}
		}
#endif

		Con_Printf ("Bad rcon from %s:\n%s\n"
			, NET_AdrToString (adr, sizeof(adr), net_from), net_message.data+4);

		SV_BeginRedirect (RD_PACKET, LANGDEFAULT);

		Con_Printf ("Bad rcon_password.\n");

	}
	else
	{

		Con_Printf ("Rcon from %s:\n%s\n"
			, NET_AdrToString (adr, sizeof(adr), net_from), net_message.data+4);

		SV_BeginRedirect (RD_PACKET, LANGDEFAULT);

		remaining[0] = 0;

		for (i=2 ; i<Cmd_Argc() ; i++)
		{
			if (strlen(remaining)+strlen(Cmd_Argv(i))>=sizeof(remaining)-2)
			{
				Con_Printf("Rcon was too long\n");
				SV_EndRedirect ();
				Con_Printf ("Rcon from %s:\n%s\n"
					, NET_AdrToString (adr, sizeof(adr), net_from), "Was too long - possible buffer overflow attempt");
				return;
			}
			strcat (remaining, Cmd_Argv(i) );
			strcat (remaining, " ");
		}

		Cmd_ExecuteString (remaining, rcon_level.ival);

	}

	SV_EndRedirect ();
}

void SVC_RealIP (void)
{
	unsigned int slotnum;
	int cookie;
	char *banreason;
	char adr[MAX_ADR_SIZE];

	slotnum = atoi(Cmd_Argv(1));
	cookie = atoi(Cmd_Argv(2));

	if (slotnum >= MAX_CLIENTS)
	{
		//a malitious user
		return;
	}

	if (cookie != svs.clients[slotnum].realip_num)
	{
		//could be someone trying to kick someone else
		//so we can't kick, as much as we might like to.
		return;
	}

	if (svs.clients[slotnum].realip_status)
		return;

	if (NET_AddressSmellsFunny(net_from))
	{
		Con_Printf("funny realip address: %s, ", NET_AdrToString(adr, sizeof(adr), net_from));
		Con_Printf("proxy address: %s\n", NET_AdrToString(adr, sizeof(adr), svs.clients[slotnum].netchan.remote_address));
		return;
	}

	banreason = SV_BannedReason(&net_from);
	if (banreason)
	{
		Con_Printf("%s has a banned realip\n", svs.clients[slotnum].name);
		if (*banreason)
			SV_ClientPrintf(&svs.clients[slotnum], PRINT_CHAT, "You were banned.\nReason: %s\n", banreason);
		else
			SV_ClientPrintf(&svs.clients[slotnum], PRINT_CHAT, "You were banned.\n");
		SV_DropClient(&svs.clients[slotnum]);
		return;
	}

	svs.clients[slotnum].realip_status = 1;
	svs.clients[slotnum].realip = net_from;
}

void SVC_ACK (void)
{
	int slotnum;
	char adr[MAX_ADR_SIZE];

	for (slotnum = 0; slotnum < MAX_CLIENTS; slotnum++)
	{
		if (svs.clients[slotnum].state)
		{
			if (svs.clients[slotnum].realip_status == 1 && NET_CompareAdr(svs.clients[slotnum].realip, net_from))
			{
				if (!*Cmd_Argv(1))
					svs.clients[slotnum].realip_status = 2;
				else if (atoi(Cmd_Argv(1)) == svs.clients[slotnum].realip_ping &&
						atoi(Cmd_Argv(2)) == svs.clients[slotnum].realip_num)
				{
					svs.clients[slotnum].realip_status = 3;
				}
				else
				{
					Netchan_OutOfBandPrint(NS_SERVER, net_from, "realip not accepted. Please stop hacking.\n");
				}
				return;
			}
		}
	}
	Con_Printf ("A2A_ACK from %s\n", NET_AdrToString (adr, sizeof(adr), net_from));
}

//returns false to block replies
//this is to mitigate wasted bandwidth if we're used as a udp escilation
qboolean SVC_ThrottleInfo (void)
{
#define THROTTLE_PPS 20
	static unsigned int blockuntil;
	unsigned int curtime = Sys_Milliseconds(); 
	unsigned int inc = 1000/THROTTLE_PPS;

	/*don't go too far back*/
	//if (blockuntil < curtime - 1000)
	if (1000 < curtime - blockuntil)
		blockuntil = curtime - 1000;

	/*don't allow it to go beyond curtime or we get issues with the logic above*/
	if (inc > curtime-blockuntil)
		return true;

	blockuntil += inc;
	return false;
}
/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
qboolean SV_ConnectionlessPacket (void)
{
	char	*s;
	char	*c;
	char	adr[MAX_ADR_SIZE];

	MSG_BeginReading (svs.netprim);

	if (net_message.cursize >= MAX_QWMSGLEN)	//add a null term in message space
	{
		Con_Printf("Oversized packet from %s\n", NET_AdrToString (adr, sizeof(adr), net_from));
		net_message.cursize=MAX_QWMSGLEN-1;
	}
	net_message.data[net_message.cursize] = '\0';	//terminate it properly. Just in case.

	MSG_ReadLong ();		// skip the -1 marker

	s = MSG_ReadStringLine ();

	Cmd_TokenizeString (s, false, false);

	c = Cmd_Argv(0);

	if (!strcmp(c, "ping") || ( c[0] == A2A_PING && (c[1] == 0 || c[1] == '\n')) )
		SVC_Ping ();
	else if (c[0] == A2A_ACK && (c[1] == 0 || c[1] == '\n') )
		SVC_ACK ();
	else if (!strcmp(c,"status"))
	{
		if (SVC_ThrottleInfo())
			SVC_Status ();
	}
	else if (!strcmp(c,"log"))
	{
		if (SVC_ThrottleInfo())
			SVC_Log ();
	}
#ifdef Q2SERVER
	else if (!strcmp(c, "info"))
	{
		if (SVC_ThrottleInfo())
			SVC_InfoQ2 ();
	}
#endif
	else if (!strncmp(c,"connect", 7))
	{
#ifdef Q3SERVER
		if (svs.gametype == GT_QUAKE3)
		{
			SVQ3_DirectConnect();
			return true;
		}

		if (sv_listen_q3.value)
		{
			if (!strstr(s, "\\name\\"))
			{	//if name isn't in the string, assume they're q3
				//this isn't quite true though, hence the listen check. but users shouldn't be connecting with an empty name anyway. more fool them.
				Huff_DecryptPacket(&net_message, 12);
				MSG_BeginReading(svs.netprim);
				MSG_ReadLong();
				s = MSG_ReadStringLine();
				Cmd_TokenizeString(s, false, false);
			}
		}
#endif
			if (secure.value)	//FIXME: possible problem for nq clients when enabled
		{
			Netchan_OutOfBandPrint (NS_SERVER, net_from, "%c\nThis server requires client validation.\nPlease use the "DISTRIBUTION" validation program\n", A2C_PRINT);
		}
		else
		{
			SVC_DirectConnect ();
			return true;
		}
	}
	else if (!strcmp(c,"\xad\xad\xad\xad""getchallenge"))
	{
		SVC_GetChallenge ();
	}
	else if (!strcmp(c,"getchallenge"))
	{
		SVC_GetChallenge ();
	}
#ifdef NQPROT
	/*for DP*/
	else if (!strcmp(c, "getstatus"))
	{
		if (SVC_ThrottleInfo())
			SVC_GetInfo(Cmd_Args(), true);
	}
	else if (!strcmp(c, "getinfo"))
	{
		if (SVC_ThrottleInfo())
			SVC_GetInfo(Cmd_Args(), false);
	}
#endif
	else if (!strcmp(c, "rcon"))
		SVC_RemoteCommand ();
	else if (!strcmp(c, "realip"))
		SVC_RealIP ();
	else if (!PR_GameCodePacket(net_message.data+4))
	{
		static unsigned int lt;
		unsigned int ct = Sys_Milliseconds();
		if (ct - lt > 5*1000)
		{
			Con_Printf ("bad connectionless packet from %s: \"%s\"\n"
					, NET_AdrToString (adr, sizeof(adr), net_from), c);
			lt = ct;
		}
	}

	return false;
}

#ifdef NQPROT
void SVNQ_ConnectionlessPacket(void)
{
	sizebuf_t sb;
	int header;
	int length;
	int active, i;
	int mod, modver, flags;
	unsigned int passwd;
	char *str;
	char buffer[256], buffer2[256];
	netadr_t localaddr;
	if (net_from.type == NA_LOOPBACK)
		return;

	if (!sv_listen_nq.value)
		return;

	MSG_BeginReading(svs.netprim);
	header = LongSwap(MSG_ReadLong());
	if (!(header & NETFLAG_CTL))
	{
		if (sv_listen_nq.ival == 2)
		if ((header & (NETFLAG_DATA|NETFLAG_EOM)) == (NETFLAG_DATA|NETFLAG_EOM))
		{
			int sequence;
			sequence = LongSwap(MSG_ReadLong());
			if (sequence <= 1)
			{
				int numnops = 0;
				int numnonnops = 0;
				/*make it at least robust enough to ignore any other stringcmds*/
				while(1)
				{
					switch(MSG_ReadByte())
					{
					case clc_nop:
						numnops++;
						continue;
					case clc_stringcmd:
						numnonnops++;
						Cmd_TokenizeString(MSG_ReadString(), false, false);
						if (!strcmp("challengeconnect", Cmd_Argv(0)))
						{
							client_t *newcl;
							/*okay, so this is a reliable packet from a client, containing a 'cmd challengeconnect $challenge' response*/
							str = va("connect %i %i %s \"\\name\\unconnected\"", NET_PROTOCOL_VERSION, 0, Cmd_Argv(1));
							Cmd_TokenizeString (str, false, false);

							newcl = SVC_DirectConnect();
							if (newcl)
								newcl->netchan.incoming_reliable_sequence = sequence;

							/*if there is anything else in the packet, we don't actually care. its reliable, so they'll resend*/
							return;
						}
						continue;
					case -1:
						break;
					default:
						numnonnops++;
						break;
					}
					break;
				}

				if (numnops && !numnonnops)
				{
					sb.maxsize = sizeof(buffer);
					sb.data = buffer;

					/*ack it, so dumb proquake clones can actually send the proper packet*/
					SZ_Clear(&sb);
					MSG_WriteLong(&sb, BigLong(NETFLAG_ACK | 8));
					MSG_WriteLong(&sb, sequence);
					NET_SendPacket(NS_SERVER, sb.cursize, sb.data, net_from);


					/*resend the cmd request, cos if they didn't send it then it must have gotten dropped*/
					SZ_Clear(&sb);
					MSG_WriteLong(&sb, 0);
					MSG_WriteLong(&sb, 0);

					MSG_WriteByte(&sb, svc_stufftext);
					MSG_WriteString(&sb, va("cmd challengeconnect %i\n", SV_NewChallenge()));

					*(int*)sb.data = BigLong(NETFLAG_UNRELIABLE|sb.cursize);
					NET_SendPacket(NS_SERVER, sb.cursize, sb.data, net_from);
				}
			}
		}
		return;	//no idea what it is.
	}

	length = header & NETFLAG_LENGTH_MASK;
	if (length != net_message.cursize)
		return;	//corrupt or not ours

	switch(MSG_ReadByte())
	{
	case CCREQ_CONNECT:
		sb.maxsize = sizeof(buffer);
		sb.data = buffer;
		if (strcmp(MSG_ReadString(), NET_GAMENAME_NQ))
		{
			SZ_Clear(&sb);
			MSG_WriteLong(&sb, 0);
			MSG_WriteByte(&sb, CCREP_REJECT);
			MSG_WriteString(&sb, "Incorrect game\n");
			*(int*)sb.data = BigLong(NETFLAG_CTL+sb.cursize);
			NET_SendPacket(NS_SERVER, sb.cursize, sb.data, net_from);
			return;	//not our game.
		}
		if (MSG_ReadByte() != NET_PROTOCOL_VERSION)
		{
			SZ_Clear(&sb);
			MSG_WriteLong(&sb, 0);
			MSG_WriteByte(&sb, CCREP_REJECT);
			MSG_WriteString(&sb, "Incorrect version\n");
			*(int*)sb.data = BigLong(NETFLAG_CTL+sb.cursize);
			NET_SendPacket(NS_SERVER, sb.cursize, sb.data, net_from);
			return;	//not our version...
		}
		mod = MSG_ReadByte();
		modver = MSG_ReadByte();
		flags = MSG_ReadByte();
		passwd = MSG_ReadLong();

		if (!strncmp(MSG_ReadString(), "getchallenge", 12))
		{
			/*dual-stack client, supporting either DP or QW protocols*/
			SVC_GetChallenge ();
		}
		else
		{
			if (sv_listen_nq.ival == 2)
			{
				SZ_Clear(&sb);
				MSG_WriteLong(&sb, 0);
				MSG_WriteByte(&sb, CCREP_ACCEPT);
				NET_LocalAddressForRemote(svs.sockets, &net_from, &localaddr, 0);
				MSG_WriteLong(&sb, ShortSwap(localaddr.port));
				*(int*)sb.data = BigLong(NETFLAG_CTL|sb.cursize);
				NET_SendPacket(NS_SERVER, sb.cursize, sb.data, net_from);


				SZ_Clear(&sb);
				MSG_WriteLong(&sb, 0);
				MSG_WriteLong(&sb, 0);

				MSG_WriteByte(&sb, svc_stufftext);
				MSG_WriteString(&sb, va("cmd challengeconnect %i\n", SV_NewChallenge()));

				*(int*)sb.data = BigLong(NETFLAG_UNRELIABLE|sb.cursize);
				NET_SendPacket(NS_SERVER, sb.cursize, sb.data, net_from);
				/*don't worry about repeating, the nop case above will recover it*/
			}
			else
			{
				str = va("connect %i %i %i \"\\name\\unconnected\"", NET_PROTOCOL_VERSION, 0, SV_NewChallenge());
				Cmd_TokenizeString (str, false, false);

				SVC_DirectConnect();
			}
		}
		break;
	case CCREQ_SERVER_INFO:
		if (Q_strcmp (MSG_ReadString(), NET_GAMENAME_NQ) != 0)
			break;

		sb.maxsize = sizeof(buffer);
		sb.data = buffer;
		SZ_Clear (&sb);
		// save space for the header, filled in later
		MSG_WriteLong (&sb, 0);
		MSG_WriteByte (&sb, CCREP_SERVER_INFO);
		if (NET_LocalAddressForRemote(svs.sockets, &net_from, &localaddr, 0))
			MSG_WriteString (&sb, NET_AdrToString (buffer2, sizeof(buffer2), localaddr));
		else
			MSG_WriteString (&sb, "unknown");
		active = 0;
		for (i = 0; i < sv.allocated_client_slots; i++)
			if (svs.clients[i].state)
				active++;
		MSG_WriteString (&sb, hostname.string);
		MSG_WriteString (&sb, sv.name);
		MSG_WriteByte (&sb, active);
		MSG_WriteByte (&sb, maxclients.value);
		MSG_WriteByte (&sb, NET_PROTOCOL_VERSION);
		*(int*)sb.data = BigLong(NETFLAG_CTL+sb.cursize);
		NET_SendPacket(NS_SERVER, sb.cursize, sb.data, net_from);
		break;
	case CCREQ_PLAYER_INFO:
		/*one request per player, ouch ouch ouch, what will it make of 32 players, I wonder*/
		sb.maxsize = sizeof(buffer);
		sb.data = buffer;
		// save space for the header, filled in later
		SZ_Clear (&sb);
		MSG_WriteLong (&sb, 0);
		{
			unsigned int pno;
			client_t *cl;
			pno = MSG_ReadByte();
			if (pno >= sv.allocated_client_slots)
				break;
			cl = &svs.clients[pno];
			if (cl->state <= cs_zombie)
				break;

			MSG_WriteByte (&sb, CCREP_PLAYER_INFO);
			MSG_WriteByte (&sb, pno);
			MSG_WriteString (&sb, cl->name);
			MSG_WriteLong (&sb, cl->playercolor);
			MSG_WriteLong (&sb, cl->old_frags);
			MSG_WriteLong (&sb, realtime - cl->connection_started);
			MSG_WriteString (&sb, "");	/*player's address, leave blank, don't spam that info as it can result in personal attacks exploits*/
		}
		*(int*)sb.data = BigLong(NETFLAG_CTL+sb.cursize);
		NET_SendPacket(NS_SERVER, sb.cursize, sb.data, net_from);
		break;
	case CCREQ_RULE_INFO:
		/*lol, nq is evil*/
		sb.maxsize = sizeof(buffer);
		sb.data = buffer;
		// save space for the header, filled in later
		SZ_Clear (&sb);
		MSG_WriteLong (&sb, 0);
		{
			char *rname, *rval, *kname;
			rname = MSG_ReadString();

			if (!*rname)
				rname = Info_KeyForNumber(svs.info, 0);
			else
			{
				int i = 0;
				for(;;)
				{
					kname = Info_KeyForNumber(svs.info, i);
					if (!*kname)
					{
						rname = NULL;
						break;
					}
					i++;
					if (!strcmp(kname, rname))
					{
						rname = Info_KeyForNumber(svs.info, i);
						break;
					}
				}
			}
			rval = Info_ValueForKey(svs.info, rname);
			MSG_WriteByte (&sb, CCREP_RULE_INFO);
			MSG_WriteString (&sb, rname);
			MSG_WriteString (&sb, rval);
		}
		*(int*)sb.data = BigLong(NETFLAG_CTL+sb.cursize);
		NET_SendPacket(NS_SERVER, sb.cursize, sb.data, net_from);
		break;
	}
}
#endif

/*
==============================================================================

PACKET FILTERING


You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

writeip
Dumps "addip <ip>" commands to listip.cfg so it can be execed at a later date.  The filter lists are not saved and restored by default, because I beleive it would cause too much confusion.

filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.


==============================================================================
*/

cvar_t	filterban = SCVAR("filterban", "1");


// SV_BannedAdress, run through ban address list and return corresponding bannedips_t
// pointer, otherwise return NULL if not in the list
bannedips_t *SV_GetBannedAddressEntry (netadr_t *a)
{
	bannedips_t *banip;
	for (banip = svs.bannedips; banip; banip=banip->next)
	{
		if (NET_CompareAdrMasked(*a, banip->adr, banip->adrmask))
			return banip;
	}
	return NULL;
}

/*
=================
SV_FilterPacket
=================
*/
char *SV_BannedReason (netadr_t *a)
{
	char *reason = filterban.value?NULL:"";	//"" = banned with no explicit reason
	bannedips_t *banip;

	if (NET_IsLoopBackAddress(*a))
		return NULL; // never filter loopback

	for (banip = svs.bannedips; banip; banip=banip->next)
	{
		if (NET_CompareAdrMasked(*a, banip->adr, banip->adrmask))
		{
			switch(banip->type)
			{
			case BAN_BAN:
				return banip->reason;
			case BAN_PERMIT:
				return 0;
			default:
				reason = filterban.value?banip->reason:NULL;
				break;
			}
		}
	}
	return reason;
}

//send a network packet to a new non-connected client.
//this is to combat tunneling
void SV_OpenRoute_f(void)
{
	netadr_t to;
	char data[64];

	NET_StringToAdr(Cmd_Argv(1), &to);
	if (!to.port)
		to.port = PORT_QWCLIENT;

	sprintf(data, "\xff\xff\xff\xff%c", S2C_CONNECTION);

	Netchan_OutOfBandPrint(NS_SERVER, to, "hello");
//	NET_SendPacket (strlen(data)+1, data, to);
}

//============================================================================

/*
=================
SV_ReadPackets
=================
*/
#ifdef SERVER_DEMO_PLAYBACK
//FIMXE: move to header
qboolean SV_GetPacket (void);
#endif

qboolean SV_ReadPackets (float *delay)
{
	int			i;
	client_t	*cl;
	int			qport;
	laggedpacket_t *lp;
	char		*banreason;
	qboolean	received = false;
	int			giveup = 5000; /*we're fucked if we need this to be this high, but at least we can retain some clients if we're really running that slow*/
	int			cookie = 0;

	for (i = 0; i < MAX_CLIENTS; i++)	//fixme: shouldn't we be using svs.allocated_client_slots ?
	{
		cl = &svs.clients[i];
		while (cl->laggedpacket)
		{
			//schedule a wakeup so minping is more consistant
			if (cl->laggedpacket->time > realtime)
			{
				if (*delay > cl->laggedpacket->time - realtime)
					*delay = cl->laggedpacket->time - realtime;
				break;
			}
			else
			{
				lp = cl->laggedpacket;
				cl->laggedpacket = lp->next;
				if (cl->laggedpacket_last == lp)
					cl->laggedpacket_last = lp->next;

				lp->next = svs.free_lagged_packet;
				svs.free_lagged_packet = lp;

				SZ_Clear(&net_message);
				memcpy(net_message.data, lp->data, lp->length);
				net_message.cursize = lp->length;

				net_from = cl->netchan.remote_address;	//not sure if anything depends on this, but lets not screw them up willynilly

				if (ISNQCLIENT(cl))
				{
					if (cl->state != cs_zombie)
					{
						if (NQNetChan_Process(&cl->netchan))
						{
							received++;
							svs.stats.packets++;
							SVNQ_ExecuteClientMessage(cl);
						}
					}
					break;
				}
				else
				{
					/*QW*/
					if (Netchan_Process(&cl->netchan))
					{	// this is a valid, sequenced packet, so process it
						received++;
						svs.stats.packets++;
						if (cl->state > cs_zombie)
						{	//make sure they didn't already disconnect
							if (cl->send_message)
								cl->chokecount++;
							else
								cl->send_message = true;	// reply at end of frame

		#ifdef Q2SERVER
							if (cl->protocol == SCP_QUAKE2)
								SVQ2_ExecuteClientMessage(cl);
							else
		#endif
								SV_ExecuteClientMessage (cl);
						}
					}
				}
			}
		}
	}

#ifdef SERVER_DEMO_PLAYBACK
	while (giveup-- > 0 && SV_GetPacket())
#else
	while (giveup-- > 0 && (cookie=NET_GetPacket (NS_SERVER, cookie)) >= 0)
#endif
	{
		banreason = SV_BannedReason (&net_from);
		if (banreason)
		{
			if (*banreason)
				Netchan_OutOfBandPrint(NS_SERVER, net_from, "%cYou are banned: %s\n", A2C_PRINT, banreason);
			else
				Netchan_OutOfBandPrint(NS_SERVER, net_from, "%cYou are banned\n", A2C_PRINT);
			continue;
		}

		// check for connectionless packet (0xffffffff) first
		if (*(int *)net_message.data == -1)
		{
			SV_ConnectionlessPacket();
			continue;
		}

#ifdef Q3SERVER
		if (svs.gametype == GT_QUAKE3)
		{
			received++;
			SVQ3_HandleClient();
			continue;
		}
#endif

		// read the qport out of the message so we can fix up
		// stupid address translating routers
		MSG_BeginReading (svs.netprim);
		MSG_ReadLong ();		// sequence number
		MSG_ReadLong ();		// sequence number
		qport = MSG_ReadShort () & 0xffff;

		// check for packets from connected clients
		for (i=0, cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
		{
			if (cl->state == cs_free)
				continue;
			if (!NET_CompareBaseAdr (net_from, cl->netchan.remote_address))
				continue;
#ifdef NQPROT
			if (ISNQCLIENT(cl) && cl->netchan.remote_address.port == net_from.port)
			{
				if (cl->state != cs_zombie)
				{
					if (cl->delay > 0)
						goto dominping;

					if (NQNetChan_Process(&cl->netchan))
					{
						received++;
						svs.stats.packets++;
						SVNQ_ExecuteClientMessage(cl);
					}
				}
				break;
			}
#endif

#ifdef Q3SERVER
			if (ISQ3CLIENT(cl))
				continue;
#endif

			if (cl->netchan.qport != qport)
				continue;
			if (cl->netchan.remote_address.port != net_from.port)
			{
				Con_DPrintf ("SV_ReadPackets: fixing up a translated port\n");
				cl->netchan.remote_address.port = net_from.port;
			}

			if (cl->delay > 0)
			{
dominping:
				if (cl->state == cs_zombie)
					break;
				if (net_message.cursize > sizeof(svs.free_lagged_packet->data))
				{
					Con_Printf("minping code is screwy\n");
					break;	//drop this packet
				}

				if (!svs.free_lagged_packet)	//kinda nasty
					svs.free_lagged_packet = Z_Malloc(sizeof(*svs.free_lagged_packet));

				if (!cl->laggedpacket)
					cl->laggedpacket_last = cl->laggedpacket = svs.free_lagged_packet;
				else
					cl->laggedpacket_last = cl->laggedpacket_last->next = svs.free_lagged_packet;
				cl->laggedpacket_last->next = NULL;
				svs.free_lagged_packet = svs.free_lagged_packet->next;

				cl->laggedpacket_last->time = realtime + cl->delay;
				memcpy(cl->laggedpacket_last->data, net_message.data, net_message.cursize);
				cl->laggedpacket_last->length = net_message.cursize;
				break;
			}


			if (Netchan_Process(&cl->netchan))
			{	// this is a valid, sequenced packet, so process it
				received++;
				svs.stats.packets++;
				if (cl->state != cs_zombie)
				{
					if (cl->send_message)
						cl->chokecount++;
					else
						cl->send_message = true;	// reply at end of frame

#ifdef Q2SERVER
					if (cl->protocol == SCP_QUAKE2)
						SVQ2_ExecuteClientMessage(cl);
					else
#endif
						SV_ExecuteClientMessage (cl);
				}
			}
			break;
		}

		if (i != MAX_CLIENTS)
			continue;

#ifdef Q3SERVER
		if (sv_listen_q3.ival && SVQ3_HandleClient())
		{
			received++;
			continue;
		}
#endif

#ifdef NQPROT
		SVNQ_ConnectionlessPacket();
#endif

		// packet is not from a known client
		//	Con_Printf ("%s:sequenced packet without connection\n"
		// ,NET_AdrToString(net_from));
	}

	return received;
}

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client in timeout.value
seconds, drop the conneciton.

When a client is normally dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void SV_CheckTimeouts (void)
{
	int		i;
	client_t	*cl;
	float	droptime;
	int	nclients;

	droptime = realtime - timeout.value;
	nclients = 0;

	for (i=0,cl=svs.clients ; i<MAX_CLIENTS ; i++,cl++)
	{
		if (cl->state == cs_connected || cl->state == cs_spawned) {
			if (!cl->spectator)
				nclients++;
			if (cl->netchan.last_received < droptime && cl->netchan.remote_address.type != NA_LOOPBACK && cl->protocol != SCP_BAD) {
				SV_BroadcastTPrintf (PRINT_HIGH, STL_CLIENTTIMEDOUT, cl->name);
				SV_DropClient (cl);
				cl->state = cs_free;	// don't bother with zombie state for local player.
			}
		}
		if (cl->state == cs_zombie &&
			realtime - cl->connection_started > zombietime.value)
		{
			if (cl->connection_started == -1)
			{
				continue;
			}
			cl->state = cs_free;	// can now be reused
			if (cl->istobeloaded)
			{
				pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, cl->edict);
				PR_ExecuteProgram (svprogfuncs, pr_global_struct->ClientDisconnect);
				sv.spawned_client_slots--;

				cl->istobeloaded=false;

				SV_BroadcastTPrintf (PRINT_HIGH, STL_LOADZOMIBETIMEDOUT, cl->name);
//				cl->state = cs_zombie;	// the real zombieness starts now
//				cl->connection_started = realtime;
			}
		}
	}
	if ((sv.paused&1) && !nclients)
	{
		// nobody left, unpause the server
		if (SV_TogglePause(NULL))
			SV_BroadcastTPrintf(PRINT_HIGH, STL_CLIENTLESSUNPAUSE);
	}
}

/*
===================
SV_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
void SV_GetConsoleCommands (void)
{
	char	*cmd;

	while (1)
	{
		cmd = Sys_ConsoleInput ();
		if (!cmd)
			break;
		Cbuf_AddText (cmd, RESTRICT_LOCAL);
	}
}

#define MINDRATE 500
#define MINRATE 500
int SV_RateForClient(client_t *cl)
{
	int rate;
	if (cl->download)
	{
		rate = cl->drate;
		if (sv_maxdrate.ival)
		{
			if (!rate || rate > sv_maxdrate.value)
				rate = sv_maxdrate.value;
			else if (rate < MINDRATE)
				rate = MINDRATE;
		}
		else if (rate >= 1 && rate < MINDRATE)
			rate = MINDRATE;
	}
	else
	{
		rate = cl->rate;
		if (sv_maxrate.ival)
		{
			if (rate > sv_maxrate.value)
				rate = sv_maxrate.value;
			else if (rate < MINRATE)
				rate = MINRATE;
		}
		else if (rate >= 1 && rate < MINRATE)
			rate = MINRATE;
	}

	return rate;
}
/*
===================
SV_CheckVars

===================
*/
void SV_CheckVars (void)
{
	static char *pw, *spw;
	int			v;

	if (password.string == pw && spectator_password.string == spw)
		return;
	pw = password.string;
	spw = spectator_password.string;

	v = 0;
	if (pw && pw[0] && strcmp(pw, "none"))
		v |= 1;
	if (spw && spw[0] && strcmp(spw, "none"))
		v |= 2;

	Con_DPrintf ("Updated needpass.\n");
	if (!v)
		Info_SetValueForKey (svs.info, "needpass", "", MAX_SERVERINFO_STRING);
	else
		Info_SetValueForKey (svs.info, "needpass", va("%i",v), MAX_SERVERINFO_STRING);
}

#ifdef Q2SERVER
void SVQ2_ClearEvents(void)
{
	q2edict_t	*ent;
	int		i;

	for (i=0 ; i<ge->num_edicts ; i++, ent++)
	{
		ent = Q2EDICT_NUM(i);
		// events only last for a single message
		ent->s.event = 0;
	}
}
#endif


/*
==================
SV_Impulse_f

Spawns a client, uses an impulse, uses that clients think then removes the client.
==================
*/
void SV_Impulse_f (void)
{
	int i;
	if (svs.gametype != GT_PROGS)
	{
		Con_Printf("Not supported in this game type\n");
		return;
	}

	for (i = 0; i < sv.allocated_client_slots; i++)
	{
		if (svs.clients[i].state == cs_free)
		{
			break;
		}
	}

	if (i == sv.allocated_client_slots)
	{
		Con_Printf("No empty player slots\n");
		return;
	}
	if (!svprogfuncs)
		return;

	pr_global_struct->time = sv.world.physicstime;

	svs.clients[i].state = cs_connected;

	SV_SetUpClientEdict(&svs.clients[i], svs.clients[i].edict);

	svs.clients[i].edict->v->netname = PR_SetString(svprogfuncs, "Console");

	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, svs.clients[i].edict);
	PR_ExecuteProgram (svprogfuncs, pr_global_struct->ClientConnect);

	pr_global_struct->time = sv.world.physicstime;
	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, svs.clients[i].edict);
	PR_ExecuteProgram (svprogfuncs, pr_global_struct->PutClientInServer);
	sv.spawned_client_slots++;

	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, svs.clients[i].edict);
	PR_ExecuteProgram (svprogfuncs, pr_global_struct->PlayerPreThink);
	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, svs.clients[i].edict);
	PR_ExecuteProgram (svprogfuncs, svs.clients[i].edict->v->think);
	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, svs.clients[i].edict);
	PR_ExecuteProgram (svprogfuncs, pr_global_struct->PlayerPostThink);

	svs.clients[i].edict->v->impulse = atoi(Cmd_Argv(1));

	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, svs.clients[i].edict);
	PR_ExecuteProgram (svprogfuncs, pr_global_struct->PlayerPreThink);
	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, svs.clients[i].edict);
	PR_ExecuteProgram (svprogfuncs, svs.clients[i].edict->v->think);
	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, svs.clients[i].edict);
	PR_ExecuteProgram (svprogfuncs, pr_global_struct->PlayerPostThink);

	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, svs.clients[i].edict);
	PR_ExecuteProgram (svprogfuncs, pr_global_struct->ClientDisconnect);
	sv.spawned_client_slots--;

	svs.clients[i].state = cs_free;
}

static void SV_PauseChanged(void)
{
	int i;
	client_t *cl;
	// send notification to all clients
	for (i=0, cl = svs.clients ; i<MAX_CLIENTS ; i++, cl++)
	{
		if (!cl->state)
			continue;
		if ((ISQWCLIENT(cl) || ISNQCLIENT(cl)) && !cl->controller)
		{
			ClientReliableWrite_Begin (cl, svc_setpause, 2);
			ClientReliableWrite_Byte (cl, sv.paused!=0);
		}
	}
}

/*
==================
SV_Frame

==================
*/
float SV_Frame (void)
{
	extern cvar_t pr_imitatemvdsv;
	static double	start, end, idletime;
	float oldtime;
	qboolean isidle;
	static int oldpaused;
	float timedelta;
	float delay;

	start = Sys_DoubleTime ();
	svs.stats.idle += start - end;
	end = start;

	//qw qc uses this for newmis handling
	svs.framenum++;
	if (svs.framenum > 0x10000)
		svs.framenum = 0;

	delay = sv_maxtic.value;

// keep the random time dependent
	rand ();

	if (!sv.gamespeed)
		sv.gamespeed = 1;

#ifndef SERVERONLY
	isidle = !isDedicated && sv.allocated_client_slots == 1 && key_dest != key_game && cls.state == ca_active;
	/*server is effectively paused if there are no clients*/
	if (sv.spawned_client_slots == 0)
		isidle = true;
	if ((sv.paused & 4) != (isidle?4:0))
		sv.paused ^= 4;
#endif

	if (oldpaused != sv.paused)
	{
		SV_PauseChanged();
		oldpaused = sv.paused;
	}


	//work out the gamespeed
	if (sv.gamespeed != sv_gamespeed.value)
	{
		char *newspeed;
		sv.gamespeed = sv_gamespeed.value;
		if (sv.gamespeed < 0.1 || sv.gamespeed == 1)
			sv_gamespeed.value = sv.gamespeed = 1;

		if (sv.gamespeed == 1)
			newspeed = "";
		else
			newspeed = va("%g", sv.gamespeed*100);
		Info_SetValueForStarKey(svs.info, "*gamespeed", newspeed, MAX_SERVERINFO_STRING);
		SV_SendServerInfoChange("*gamespeed", newspeed);

		//correct sv.starttime
		sv.starttime = Sys_DoubleTime() - (sv.time/sv.gamespeed);
	}


// decide the simulation time
	{
		oldtime = sv.time;
		sv.time = (Sys_DoubleTime() - sv.starttime)*sv.gamespeed;
		timedelta = sv.time - oldtime;
		if (sv.time < oldtime)
		{
			sv.time = oldtime;	//urm
			timedelta = 0;
		}

		if (isDedicated)
			realtime += sv.time - oldtime;

		if (sv.paused && sv.time > 1.5)
		{
			sv.starttime += (sv.time - oldtime)/sv.gamespeed;	//move the offset
			sv.time = oldtime;	//and keep time as it was.
		}
	}


#ifdef WEBSERVER
	IWebRun();
#endif

	if (sv_master.ival)
	{
		if (sv_masterport.ival)
			SVM_Think(sv_masterport.ival);
		else
			SVM_Think(PORT_QWMASTER);
	}

	{
void SV_MVDStream_Poll(void);
	SV_MVDStream_Poll();
	}

	if (sv.state < ss_active || !sv.world.worldmodel)
	{
#ifndef SERVERONLY
	// check for commands typed to the host
		if (isDedicated)
#endif
		{
			SV_GetConsoleCommands ();
			Cbuf_Execute ();
		}
		return delay;
	}

// check timeouts
	SV_CheckTimeouts ();

	SV_CheckTimer ();

// toggle the log buffer if full
	SV_CheckLog ();

// get packets
	isidle = !SV_ReadPackets (&delay);

	if (pr_imitatemvdsv.ival)
	{
		Cbuf_Execute ();
		if (sv.state < ss_active)	//whoops...
			return delay;
	}

	if (sv.multicast.cursize)
	{
		Con_Printf("Unterminated multicast\n");
		sv.multicast.cursize=0;
	}

	// move autonomous things around if enough time has passed
	if (!sv.paused || (sv.world.physicstime < 1 && sv.spawned_client_slots))
	{
#ifdef Q2SERVER
		//q2 is idle even if clients sent packets.
		if (svs.gametype == GT_QUAKE2)
			isidle = true;
#endif
		if (SV_Physics ())
			isidle = false;
	}
	else
	{
		isidle = idletime < 0.1;
#ifdef VM_Q1
		if (svs.gametype == GT_Q1QVM)
		{
			Q1QVM_GameCodePausedTic(Sys_DoubleTime() - sv.pausedstart);
		}
		else
#endif
		{
			PR_GameCodePausedTic(Sys_DoubleTime() - sv.pausedstart);
		}
	}

	if (!isidle || idletime > 0.15)
	{
		//this is the q2 frame number found in the q2 protocol. each packet should contain a new frame or interpolation gets confused
		sv.framenum++;

#ifdef SQL
		PR_SQLCycle();
#endif

#ifdef SERVER_DEMO_PLAYBACK
		while(SV_ReadMVD());
#endif

		if (sv.multicast.cursize)
		{
			Con_Printf("Unterminated multicast\n");
			sv.multicast.cursize=0;
		}

#ifndef SERVERONLY
// check for commands typed to the host
		if (isDedicated)
#endif
		{
#ifdef PLUGINS
			Plug_Tick();
#endif

			SV_GetConsoleCommands ();

// process console commands
			if (!pr_imitatemvdsv.value)
				Cbuf_Execute ();
		}

		if (sv.state < ss_active)	//whoops...
			return delay;

		SV_CheckVars ();

// send messages back to the clients that had packets read this frame
		SV_SendClientMessages ();

//	demo_start = Sys_DoubleTime ();
		SV_SendMVDMessage();
//	demo_end = Sys_DoubleTime ();
//	svs.stats.demo += demo_end - demo_start;

// send a heartbeat to the master if needed
		Master_Heartbeat ();


#ifdef Q2SERVER
		if (ge && ge->edicts)
			SVQ2_ClearEvents();
#endif
		idletime = 0;
	}
	idletime += timedelta;

// collect timing statistics
	end = Sys_DoubleTime ();
	svs.stats.active += end-start;
	if (++svs.stats.count == STATFRAMES)
	{
		svs.stats.latched_active = svs.stats.active;
		svs.stats.latched_idle = svs.stats.idle;
		svs.stats.latched_packets = svs.stats.packets;
		svs.stats.active = 0;
		svs.stats.idle = 0;
		svs.stats.packets = 0;
		svs.stats.count = 0;
	}
	return delay;
}

/*
===============
SV_InitLocal
===============
*/
extern void Log_Init (void);

void SV_InitLocal (void)
{
	int		i, p;
	extern	cvar_t	pr_allowbutton1;
	extern	cvar_t	sv_aim;

	extern	cvar_t	pm_bunnyspeedcap;
	extern	cvar_t	pm_ktjump;
	extern	cvar_t	pm_slidefix;
	extern	cvar_t	pm_airstep;
	extern	cvar_t	pm_walljump;
	extern	cvar_t	pm_slidyslopes;

	SV_InitOperatorCommands	();
	SV_UserInit ();

#ifndef SERVERONLY
	if (isDedicated)
#endif
	{
		Cvar_Register (&developer,	cvargroup_servercontrol);

		Cvar_Register (&password,	cvargroup_servercontrol);
		Cvar_Register (&rcon_password,	cvargroup_servercontrol);

		Log_Init();
	}
	rcon_password.restriction = RESTRICT_MAX;	//no cheatie rconers changing rcon passwords...
	Cvar_Register (&spectator_password,	cvargroup_servercontrol);

	Cvar_Register (&sv_mintic,	cvargroup_servercontrol);
	Cvar_Register (&sv_maxtic,	cvargroup_servercontrol);
	Cvar_Register (&sv_limittics,	cvargroup_servercontrol);

	Cvar_Register (&fraglimit,	cvargroup_serverinfo);
	Cvar_Register (&timelimit,	cvargroup_serverinfo);
	Cvar_Register (&teamplay,	cvargroup_serverinfo);
	Cvar_Register (&coop,	cvargroup_serverinfo);
	Cvar_Register (&skill,	cvargroup_serverinfo);
	Cvar_Register (&samelevel,	cvargroup_serverinfo);
	Cvar_Register (&maxclients,	cvargroup_serverinfo);
	Cvar_Register (&maxspectators,	cvargroup_serverinfo);
	Cvar_Register (&hostname,	cvargroup_serverinfo);
	Cvar_Register (&deathmatch,	cvargroup_serverinfo);
	Cvar_Register (&spawn,	cvargroup_servercontrol);

	//arguably cheats. Must be switched on to use.
	Cvar_Register (&watervis,	cvargroup_serverinfo);
	Cvar_Register (&rearview,	cvargroup_serverinfo);
	Cvar_Register (&mirrors,	cvargroup_serverinfo);
	Cvar_Register (&allow_luma,	cvargroup_serverinfo);
	Cvar_Register (&allow_bump,	cvargroup_serverinfo);
	Cvar_Register (&allow_skybox,	cvargroup_serverinfo);
	Cvar_Register (&sv_allow_splitscreen,	cvargroup_serverinfo);
	Cvar_Register (&fbskins,	cvargroup_serverinfo);

	Cvar_Register (&timeout,	cvargroup_servercontrol);
	Cvar_Register (&zombietime,	cvargroup_servercontrol);

	Cvar_Register (&sv_pupglow,	cvargroup_serverinfo);
	Cvar_Register (&sv_loadentfiles,	cvargroup_servercontrol);

	Cvar_Register (&sv_bigcoords,			cvargroup_serverphysics);

	Cvar_Register (&pm_bunnyspeedcap,		cvargroup_serverphysics);
	Cvar_Register (&pm_ktjump,				cvargroup_serverphysics);
	Cvar_Register (&pm_slidefix,			cvargroup_serverphysics);
	Cvar_Register (&pm_slidyslopes,			cvargroup_serverphysics);
	Cvar_Register (&pm_airstep,				cvargroup_serverphysics);
	Cvar_Register (&pm_walljump,			cvargroup_serverphysics);

	Cvar_Register (&sv_compatiblehulls,		cvargroup_serverphysics);

	for (i = 0; i < sizeof(sv_motd)/sizeof(sv_motd[0]); i++)
		Cvar_Register(&sv_motd[i],	cvargroup_serverinfo);

	Cvar_Register (&sv_aim,	cvargroup_servercontrol);

	Cvar_Register (&sv_resetparms,	cvargroup_servercontrol);

	Cvar_Register (&sv_public,	cvargroup_servercontrol);
	Cvar_Register (&sv_listen_qw,	cvargroup_servercontrol);
	Cvar_Register (&sv_listen_nq,	cvargroup_servercontrol);
	Cvar_Register (&sv_listen_dp,	cvargroup_servercontrol);
	Cvar_Register (&sv_listen_q3,	cvargroup_servercontrol);
	sv_listen_qw.restriction = RESTRICT_MAX;

#ifdef TCPCONNECT
	Cvar_Register (&sv_port_tcp,	cvargroup_servercontrol);
	sv_port_tcp.restriction = RESTRICT_MAX;
#ifdef IPPROTO_IPV6
	Cvar_Register (&sv_port_tcp6,	cvargroup_servercontrol);
	sv_port_tcp6.restriction = RESTRICT_MAX;
#endif
#endif
#ifdef IPPROTO_IPV6
	Cvar_Register (&sv_port_ipv6,	cvargroup_servercontrol);
	sv_port_ipv6.restriction = RESTRICT_MAX;
#endif
#ifdef USEIPX
	Cvar_Register (&sv_port_ipx,	cvargroup_servercontrol);
	sv_port_ipx.restriction = RESTRICT_MAX;
#endif
#ifdef HAVE_IPV4
	Cvar_Register (&sv_port_ipv4,	cvargroup_servercontrol);
	sv_port_ipv4.restriction = RESTRICT_MAX;
#endif

	Cvar_Register (&sv_reportheartbeats, cvargroup_servercontrol);

#ifndef SERVERONLY
	if (isDedicated)
#endif
		Cvar_Set(&sv_public, "1");

	Cvar_Register (&sv_master,	cvargroup_servercontrol);
	Cvar_Register (&sv_masterport,	cvargroup_servercontrol);

	Cvar_Register (&filterban,	cvargroup_servercontrol);

	Cvar_Register (&allow_download,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_skins,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_models,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_sounds,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_maps,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_demos,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_anymap,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_pakcontents,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_textures,cvargroup_serverpermissions);
	Cvar_Register (&allow_download_configs,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_packages,cvargroup_serverpermissions);
	Cvar_Register (&allow_download_wads,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_root,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_copyrighted,	cvargroup_serverpermissions);
	Cvar_Register (&secure,	cvargroup_serverpermissions);

	Cvar_Register (&sv_highchars,	cvargroup_servercontrol);
	Cvar_Register (&sv_calcphs,	cvargroup_servercontrol);
	Cvar_Register (&sv_phs,	cvargroup_servercontrol);
	Cvar_Register (&sv_cullplayers_trace, cvargroup_servercontrol);
	Cvar_Register (&sv_cullentities_trace, cvargroup_servercontrol);

	Cvar_Register (&sv_csqc_progname,	cvargroup_servercontrol);
	Cvar_Register (&sv_csqcdebug, cvargroup_servercontrol);

	Cvar_Register (&sv_gamespeed, cvargroup_serverphysics);
	Cvar_Register (&sv_nomsec,	cvargroup_serverphysics);
	Cvar_Register (&pr_allowbutton1, cvargroup_servercontrol);

	Cvar_Register (&pausable,	cvargroup_servercontrol);

	Cvar_Register (&sv_maxrate, cvargroup_servercontrol);
	Cvar_Register (&sv_maxdrate, cvargroup_servercontrol);
	Cvar_Register (&sv_minping, cvargroup_servercontrol);

	Cvar_Register (&sv_nailhack, cvargroup_servercontrol);

	Cmd_AddCommand ("sv_impulse", SV_Impulse_f);

	Cmd_AddCommand ("openroute", SV_OpenRoute_f);

	Cmd_AddCommand ("savegame", SV_Savegame_f);
	Cmd_AddCommand ("loadgame", SV_Loadgame_f);
	Cmd_AddCommand ("save", SV_Savegame_f);
	Cmd_AddCommand ("load", SV_Loadgame_f);

	SV_MVDInit();

	for (i=0 ; i<MAX_MODELS ; i++)
		sprintf (localmodels[i], "*%i", i);

	Info_SetValueForStarKey (svs.info, "*version", version_string(), MAX_SERVERINFO_STRING);

	Info_SetValueForStarKey (svs.info, "*z_ext", va("%i", SUPPORTED_Z_EXTENSIONS), MAX_SERVERINFO_STRING);

	// init fraglog stuff
	svs.logsequence = 1;
	svs.logtime = realtime;
	svs.log[0].data = svs.log_buf[0];
	svs.log[0].maxsize = sizeof(svs.log_buf[0]);
	svs.log[0].cursize = 0;
	svs.log[0].allowoverflow = true;
	svs.log[1].data = svs.log_buf[1];
	svs.log[1].maxsize = sizeof(svs.log_buf[1]);
	svs.log[1].cursize = 0;
	svs.log[1].allowoverflow = true;


	svs.free_lagged_packet = NULL;

	// parse params for cvars
	p = COM_CheckParm ("-port");
	if (!p)
		p = COM_CheckParm ("-svport");
	if (p && p < com_argc)
	{
		int port = atoi(com_argv[p+1]);
		if (!port)
			port = PORT_QWSERVER;
#ifdef HAVE_IPV4
		if (*sv_port_ipv4.string)
			Cvar_SetValue(&sv_port_ipv4, port);
#endif
#ifdef IPPROTO_IPV6
		if (*sv_port_ipv6.string)
			Cvar_SetValue(&sv_port_ipv6, port);
#endif
#ifdef USEIPX
		if (*sv_port_ipx.string)
			Cvar_SetValue(&sv_port_ipx, port);
#endif
	}

}


//============================================================================

void SV_Masterlist_Callback(struct cvar_s *var, char *oldvalue)
{
	int i;

	for (i = 0; sv_masterlist[i].cv.name; i++)
	{
		if (var == &sv_masterlist[i].cv)
			break;
	}

	if (!sv_masterlist[i].cv.name)
		return;

	if (!*var->string)
	{
		sv_masterlist[i].adr.port = 0;
		return;
	}

	sv_masterlist[i].needsresolve = true;
}

/*
================
Master_Heartbeat

Send a message to the master every few minutes to
let it know we are alive, and log information
================
*/
#define	HEARTBEAT_SECONDS	300
void Master_Heartbeat (void)
{
	char		string[2048];
	int			active;
	int			i, j;
	qboolean	madeqwstring = false;
	char		adr[MAX_ADR_SIZE];

	if (!sv_public.ival)
		return;

	if (realtime-HEARTBEAT_SECONDS - svs.last_heartbeat < HEARTBEAT_SECONDS)
		return;		// not time to send yet

	svs.last_heartbeat = realtime-HEARTBEAT_SECONDS;

	svs.heartbeat_sequence++;

	// send to group master
	for (i = 0; sv_masterlist[i].cv.name; i++)
	{
		if (sv_masterlist[i].needsresolve)
		{
			sv_masterlist[i].needsresolve = false;

			if (!*sv_masterlist[i].cv.string)
				sv_masterlist[i].adr.port = 0;
			else if (!NET_StringToAdr(sv_masterlist[i].cv.string, &sv_masterlist[i].adr))
			{
				sv_masterlist[i].adr.port = 0;
				Con_Printf ("Couldn't resolve master \"%s\"\n", sv_masterlist[i].cv.string);
			}
			else
			{
				if (sv_masterlist[i].adr.type == NA_TCP || sv_masterlist[i].adr.type == NA_TCPV6)
					NET_EnsureRoute(svs.sockets, sv_masterlist[i].cv.name, sv_masterlist[i].cv.string, false);

				//choose default port
				switch (sv_masterlist[i].protocol)
				{
				case MP_DARKPLACES:
					if (sv_masterlist[i].adr.port == 0)
						sv_masterlist[i].adr.port = BigShort (27950);
					break;
				case MP_QUAKEWORLD:
					if (sv_masterlist[i].adr.port == 0)
						sv_masterlist[i].adr.port = BigShort (27000);

					//qw does this for some reason, keep the behaviour even though its unreliable thus pointless
					string[0] = A2A_PING;
					string[1] = 0;
					if (sv.state)
						NET_SendPacket (NS_SERVER, 2, string, sv_masterlist[i].adr);
					break;
				default:
					break;
				}
			}
		}

		if (sv_masterlist[i].adr.port)
		{
			switch(sv_masterlist[i].protocol)
			{
			case MP_QUAKEWORLD:
				if (sv_listen_qw.value)
				{
					if (!madeqwstring)
					{
						// count active users
						active = 0;
						for (j=0 ; j<MAX_CLIENTS ; j++)
							if (svs.clients[j].state == cs_connected ||
							svs.clients[j].state == cs_spawned )
								active++;

						sprintf (string, "%c\n%i\n%i\n", S2M_HEARTBEAT,
							svs.heartbeat_sequence, active);

						madeqwstring = true;
					}

					if (sv_reportheartbeats.value)
						Con_Printf ("Sending heartbeat to %s (%s)\n", NET_AdrToString (adr, sizeof(adr), sv_masterlist[i].adr), sv_masterlist[i].cv.string);

					NET_SendPacket (NS_SERVER, strlen(string), string, sv_masterlist[i].adr);
				}
				break;
			case MP_DARKPLACES:
				if (sv_listen_dp.value || sv_listen_nq.value)	//set listen to 1 to allow qw connections, 2 to allow nq connections too.
				{
					if (sv_reportheartbeats.value)
						Con_Printf ("Sending heartbeat to %s (%s)\n", NET_AdrToString (adr, sizeof(adr), sv_masterlist[i].adr), sv_masterlist[i].cv.string);

					{
						char *str = "\377\377\377\377heartbeat DarkPlaces\x0A";
						NET_SendPacket (NS_SERVER, strlen(str), str, sv_masterlist[i].adr);
					}
				}
				break;
			default:
				break;
			}
		}
	}
}

void Master_Add(char *stringadr)
{
	int i;

	for (i = 0; sv_masterlist[i].cv.name; i++)
	{
		if (!*sv_masterlist[i].cv.string)
			break;
	}

	if (!sv_masterlist[i].cv.name)
	{
		Con_Printf ("Too many masters\n");
		return;
	}

	Cvar_Set(&sv_masterlist[i].cv, stringadr);

	svs.last_heartbeat = -99999;
}

void Master_ReResolve(void)
{
	int i;
	for (i = 0; sv_masterlist[i].cv.name; i++)
	{
		sv_masterlist[i].needsresolve = true;
	}
}

void Master_ClearAll(void)
{
	int i;
	for (i = 0; sv_masterlist[i].cv.name; i++)
	{
		Cvar_Set(&sv_masterlist[i].cv, "");
	}
}

/*
=================
Master_Shutdown

Informs all masters that this server is going down
=================
*/
void Master_Shutdown (void)
{
	char		string[2048];
	char		adr[MAX_ADR_SIZE];
	int			i;

	//note that if a master server actually blindly listens to this then its exploitable.
	//we send it out anyway as for us its all good.
	//master servers ought to try and check up on the status of the server first, if they listen to this.

	sprintf (string, "%c\n", S2M_SHUTDOWN);

	// send to group master
	for (i = 0; sv_masterlist[i].cv.name; i++)
	{
		if (sv_masterlist[i].adr.port)
		{
			switch(sv_masterlist[i].protocol)
			{
			case MP_QUAKEWORLD:
				if (sv_reportheartbeats.value)
					Con_Printf ("Sending shutdown to %s\n", NET_AdrToString (adr, sizeof(adr), sv_masterlist[i].adr));

				NET_SendPacket (NS_SERVER, strlen(string), string, sv_masterlist[i].adr);
				break;
			//dp has no shutdown
			default:
				break;
			}
		}
	}
}

#define iswhite(c) (c == ' ' || c == INVIS_CHAR1 || c == INVIS_CHAR2 || c == INVIS_CHAR3)
#define isinvalid(c) (c == ':' || c == '\r' || c == '\n' || (unsigned char)(c) == '\xff')
//colon is so clients can't get confused while parsing chats
//255 is so fuhquake/ezquake don't end up with nameless players

//is allowed to shorten, out must be as long as in and min of "unnamed"+1
void SV_FixupName(char *in, char *out, unsigned int outlen)
{
	char *s, *p;
	unsigned int len;

	if (outlen == 0)
		return;

	len = outlen;

	s = out;
	while(iswhite(*in) || isinvalid(*in) || *in == '\1' || *in == '\2')	//1 and 2 are to stop clients from printing the entire line as chat. only do that for the leading charater.
		in++;
	while(*in && len > 0)
	{
		if (isinvalid(*in))
		{
			in++;
			continue;
		}
		*s++ = *in++;
		len--;
	}
	*s = '\0';

	if (!*out)
	{	//reached end and it was all whitespace
		//white space only
		strncpy(out, "unnamed", outlen);
		out[outlen-1] = 0;
		p = out;
	}

	for (p = out + strlen(out) - 1; p != out && iswhite(*p) ; p--)
		;
	p[1] = 0;
}


qboolean ReloadRanking(client_t *cl, char *newname)
{
#ifdef SVRANKING
	int newid;
	int j;
	rankstats_t rs;
	newid = Rank_GetPlayerID(cl->guid, newname, atoi(Info_ValueForKey (cl->userinfo, "_pwd")), true, false);	//'_' keys are always stripped. On any server. So try and use that so persistant data won't give out the password when connecting to a different server
	if (!newid)
		newid = Rank_GetPlayerID(cl->guid, newname, atoi(Info_ValueForKey (cl->userinfo, "password")), true, false);
	if (newid)
	{
		if (cl->rankid && cl->state >= cs_spawned)//apply current stats
		{
			if (!Rank_GetPlayerStats(cl->rankid, &rs))
				return false;
			rs.timeonserver += realtime - cl->stats_started;
			cl->stats_started = realtime;
			rs.kills += cl->kills;
			rs.deaths += cl->deaths;

			rs.flags1 &= ~(RANK_CUFFED|RANK_MUTED|RANK_CRIPPLED);
			if (cl->iscuffed==2)
				rs.flags1 |= RANK_CUFFED;
			if (cl->ismuted==2)
				rs.flags1 |= RANK_MUTED;
			if (cl->iscrippled==2)
				rs.flags1 |= RANK_CRIPPLED;
			cl->kills=0;
			cl->deaths=0;
			pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, cl->edict);
			if (pr_global_ptrs->SetChangeParms)
				PR_ExecuteProgram (svprogfuncs, pr_global_struct->SetChangeParms);
			for (j=0 ; j<NUM_RANK_SPAWN_PARMS ; j++)
				if (pr_global_ptrs->spawnparamglobals[j])
					rs.parm[j] = *pr_global_ptrs->spawnparamglobals[j];
			Rank_SetPlayerStats(cl->rankid, &rs);
			cl->rankid = 0;
		}
		if (!Rank_GetPlayerStats(newid, &rs))
			return false;
		cl->rankid = newid;
		if (rs.flags1 & RANK_CUFFED)
			cl->iscuffed=2;
		else if (cl->iscuffed)	//continue being cuffed, but don't inflict the new user with persistant cuffing.
			cl->iscuffed=1;
		if (rs.flags1 & RANK_MUTED)
			cl->ismuted=2;
		else if (cl->ismuted)
			cl->ismuted=1;
		if (rs.flags1 & RANK_CRIPPLED)
			cl->iscrippled=2;
		else if (cl->iscrippled)
			cl->iscrippled=1;

		cl->trustlevel = rs.trustlevel;
		return true;
	}
#endif
	return false;
}
/*
=================
SV_ExtractFromUserinfo

Pull specific info from a newly changed userinfo string
into a more C freindly form.
=================
*/
void SV_ExtractFromUserinfo (client_t *cl)
{
	char	*val, *p;
	int		i;
	client_t	*client;
	int		dupc = 1;
	char	newname[80], basic[80];
	extern cvar_t rank_filename;

	val = Info_ValueForKey (cl->userinfo, "team");
	Q_strncpyz (cl->team, val, sizeof(cl->teambuf));

	// name for C code
	val = Info_ValueForKey (cl->userinfo, "name");

	if (cl->protocol != SCP_BAD || *val)
	{
		SV_FixupName(val, newname, sizeof(newname));
		if (strlen(newname) > 40)
			newname[40] = 0;
	}
	else
		newname[0] = 0;

	deleetstring(basic, newname);
	if ((!basic[0] && cl->protocol != SCP_BAD) || strstr(basic, "console"))
		strcpy(newname, "unnamed");

	// check to see if another user by the same name exists
	while (1) {
		for (i=0, client = svs.clients ; i<MAX_CLIENTS ; i++, client++) {
			if (client->state < cs_connected || client == cl)
				continue;
			if (!stricmp(client->name, newname))
				break;
		}
		if (i != MAX_CLIENTS) { // dup name
			if (strlen(newname) > sizeof(cl->namebuf) - 1)
				newname[sizeof(cl->namebuf) - 4] = 0;
			p = newname;

			if (newname[0] == '(')
			{
				if (newname[2] == ')')
					p = newname + 3;
				else if (val[3] == ')')
					p = newname + 4;
			}

			memmove(newname+10, p, strlen(p)+1);

			sprintf(newname, "(%d)%-.40s", dupc++, newname+10);
		} else
			break;
	}

	if (strncmp(newname, cl->name, sizeof(cl->namebuf)-1))
	{
		if (cl->ismuted && *cl->name)
			SV_ClientTPrintf (cl, PRINT_HIGH, STL_NONAMEASMUTE);
		else
		{

			Info_SetValueForKey (cl->userinfo, "name", newname, sizeof(cl->userinfo));
			if (!sv.paused && *cl->name)
			{
				if (!cl->lastnametime || realtime - cl->lastnametime > 5)
				{
					cl->lastnamecount = 0;
					cl->lastnametime = realtime;
				}
				else if (cl->lastnamecount++ > 4)
				{
					SV_BroadcastTPrintf (PRINT_HIGH, STL_CLIENTKICKEDNAMESPAM, cl->name);
					SV_ClientTPrintf (cl, PRINT_HIGH, STL_YOUWEREKICKEDNAMESPAM);
					SV_DropClient (cl);
					return;
				}
			}

			if (*cl->name && cl->state >= cs_spawned && !cl->spectator)
			{
				SV_BroadcastTPrintf (PRINT_HIGH, STL_CLIENTNAMECHANGE, cl->name, newname);
			}
			Q_strncpyz (cl->name, newname, sizeof(cl->namebuf));


#ifdef SVRANKING
			if (ReloadRanking(cl, newname))
			{
#endif
#ifdef SVRANKING
			}
			else if (cl->state >= cs_spawned && *rank_filename.string)
				SV_ClientPrintf(cl, PRINT_HIGH, "Your rankings name has not been changed\n");
#endif
		}
	}

	Info_SetValueForKey(cl->userinfo, "name", newname, sizeof(cl->userinfo));

	val = Info_ValueForKey (cl->userinfo, "lang");
	cl->language = atoi(val);
	if (!cl->language)
		cl->language = LANGDEFAULT;

	val = Info_ValueForKey (cl->userinfo, "nogib");
	if (atoi(val))
		cl->gibfilter = true;
	else
		cl->gibfilter = false;

	// rate command
	val = Info_ValueForKey (cl->userinfo, "rate");
	if (strlen(val))
		cl->rate = atoi(val);
	else
		cl->rate = ISNQCLIENT(cl)?10000:2500;

	val = Info_ValueForKey (cl->userinfo, "drate");
	if (strlen(val))
		cl->drate = atoi(val);
	else
		cl->drate = cl->rate;	//0 disables the downloading check

	val = Info_ValueForKey (cl->userinfo, "cl_playerclass");
	if (val)
	{
		PRH2_SetPlayerClass(cl, atoi(val), false);
	}

	// msg command
	val = Info_ValueForKey (cl->userinfo, "msg");
	if (strlen(val))
	{
		cl->messagelevel = atoi(val);
	}
#ifdef NQPROT
	{
		int top = atoi(Info_ValueForKey(cl->userinfo, "topcolor"));
		int bottom = atoi(Info_ValueForKey(cl->userinfo, "bottomcolor"));
		top &= 15;
		if (top > 13)
			top = 13;
		bottom &= 15;
		if (bottom > 13)
			bottom = 13;
		cl->playercolor = top*16 + bottom;
		if (svs.gametype == GT_PROGS || svs.gametype == GT_Q1QVM)
		{
			cl->edict->xv->clientcolors = cl->playercolor;
			MSG_WriteByte (&sv.nqreliable_datagram, svc_updatecolors);
			MSG_WriteByte (&sv.nqreliable_datagram, cl-svs.clients);
			MSG_WriteByte (&sv.nqreliable_datagram, cl->playercolor);
		}
	}
#endif
}

//============================================================================

/*
====================
SV_InitNet
====================
*/
void SV_InitNet (void)
{
	int i;

#ifndef SERVERONLY
	if (isDedicated)
#endif
	{
		NET_Init ();
		Netchan_Init ();
	}

	for (i = 0; sv_masterlist[i].cv.name; i++)
		Cvar_Register(&sv_masterlist[i].cv, "master servers");
	Master_ReResolve();

	// heartbeats will always be sent to the id master
	svs.last_heartbeat = -99999;		// send immediately
//	NET_StringToAdr ("192.246.40.70:27000", &idmaster_adr);
}

void SV_IgnoreCommand_f(void)
{
}


/*
====================
SV_Init
====================
*/
#ifdef SERVER_DEMO_PLAYBACK
//FIXME: move to header
void SV_Demo_Init(void);
#endif

void SV_Init (quakeparms_t *parms)
{
	int i;
#ifndef SERVERONLY
	if (isDedicated)
#endif
	{
		COM_InitArgv (parms->argc, parms->argv);

		if (COM_CheckParm ("-minmemory"))
			parms->memsize = MINIMUM_MEMORY;

		host_parms = *parms;

//		if (parms->memsize < MINIMUM_MEMORY)
//			SV_Error ("Only %4.1f megs of memory reported, can't execute game", parms->memsize / (float)0x100000);

		Cvar_Init();

		Memory_Init (parms->membase, parms->memsize);

		Sys_Init();

		COM_ParsePlusSets();

		Cbuf_Init ();
		Cmd_Init ();
#ifndef SERVERONLY
		R_SetRenderer(NULL);
#endif
		COM_Init ();
		Mod_Init ();
	}

#if defined(SERVERONLY) || !(defined(CSQC_DAT) || defined(MENU_DAT))
	PF_Common_RegisterCvars();
#endif

	PR_Init ();

	SV_InitNet ();

	SV_InitLocal ();

#ifdef WEBSERVER
	IWebInit();
#endif

#ifdef SERVER_DEMO_PLAYBACK
	SV_Demo_Init();
#endif

#ifdef USEODE
	World_ODE_Init();
#endif

#ifdef SVRANKING
	Rank_RegisterCommands();
#endif
	Cbuf_AddText("alias restart \"map .\"\nalias newgame \"map start\"\n", RESTRICT_LOCAL);

#ifndef SERVERONLY
	if (isDedicated)
#endif
	{
		PM_Init ();

#ifdef PLUGINS
		Plug_Init();
#endif

		Hunk_AllocName (0, "-HOST_HUNKLEVEL-");
		host_hunklevel = Hunk_LowMark ();

		host_initialized = true;

		//get rid of the worst of the spam
		Cmd_AddCommand("bind", SV_IgnoreCommand_f);

		Con_TPrintf (TL_EXEDATETIME, __DATE__, __TIME__);
		Con_TPrintf (TL_HEAPSIZE,parms->memsize/ (1024*1024.0));

		Con_Printf ("%s\n", version_string());

		Con_TPrintf (STL_INITED, fs_gamename.string);

		i = COM_CheckParm("+gamedir");
		if (i)
			COM_Gamedir(com_argv[i+1]);

		if (COM_FileSize("server.cfg") != -1)
			Cbuf_InsertText ("exec server.cfg\nexec ftesrv.cfg\n", RESTRICT_LOCAL, false);
		else
			Cbuf_InsertText ("exec quake.rc\nexec ftesrv.cfg\n", RESTRICT_LOCAL, false);

	// process command line arguments
		Cbuf_Execute ();

		Cmd_StuffCmds();

		Cbuf_Execute ();

		//and warn about any future times this is used.
		Cmd_RemoveCommand("bind");

	// if a map wasn't specified on the command line, spawn start.map
		if (sv.state == ss_dead && Cmd_AliasExist("startmap_dm", RESTRICT_LOCAL))
		{
			Cbuf_AddText("startmap_dm", RESTRICT_LOCAL);	//DP extension
			Cbuf_Execute();
		}
		if (sv.state == ss_dead && Cmd_AliasExist("startmap_sp", RESTRICT_LOCAL))
		{
			Cbuf_AddText("startmap_sp", RESTRICT_LOCAL);	//DP extension
			Cbuf_Execute();
		}
		if (sv.state == ss_dead && COM_FCheckExists("maps/start.bsp"))
			Cmd_ExecuteString ("map start", RESTRICT_LOCAL);	//regular q1
		if (sv.state == ss_dead && COM_FCheckExists("maps/demo1.bsp"))
			Cmd_ExecuteString ("map demo1", RESTRICT_LOCAL);	//regular h2 sp
#ifdef Q2SERVER
		if (sv.state == ss_dead && COM_FCheckExists("maps/base1.bsp"))
			Cmd_ExecuteString ("map base1", RESTRICT_LOCAL);	//regular q2 sp
#endif
#ifdef Q3SERVER
		if (sv.state == ss_dead && COM_FCheckExists("maps/q3dm1.bsp"))
			Cmd_ExecuteString ("map q3dm1", RESTRICT_LOCAL);	//regular q3 'sp'
#endif
#ifdef HLSERVER
		if (sv.state == ss_dead && COM_FCheckExists("maps/c0a0.bsp"))
			Cmd_ExecuteString ("map c0a0", RESTRICT_LOCAL);	//regular hl sp
#endif

		if (sv.state == ss_dead)
			SV_Error ("Couldn't spawn a server");
	}
}

#endif

