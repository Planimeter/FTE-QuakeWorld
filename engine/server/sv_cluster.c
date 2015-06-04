#include "quakedef.h"
#include "netinc.h"

#ifdef SUBSERVERS

#ifdef SQL
#include "sv_sql.h"
#endif

extern cvar_t sv_serverip;

void VARGS SV_RejectMessage(int protocol, char *format, ...);


void MSV_UpdatePlayerStats(unsigned int playerid, unsigned int serverid, int numstats, float *stats);

typedef struct {
	//fixme: hash tables
	unsigned int	playerid;
	char			name[64];
	char			guid[64];
	char			address[64];

	link_t allplayers;
//	link_t sameserver;

	pubsubserver_t *server;	//should never be null
} clusterplayer_t;

static pubsubserver_t *subservers;
static link_t clusterplayers;
qboolean	isClusterSlave;
unsigned int nextserverid;

static clusterplayer_t *MSV_FindPlayerId(unsigned int playerid)
{
	link_t *l;
	clusterplayer_t *pl;

	FOR_EACH_LINK(l, clusterplayers)
	{
		pl = STRUCT_FROM_LINK(l, clusterplayer_t, allplayers);
		if (pl->playerid == playerid)
			return pl;
	}
	return NULL;
}
static clusterplayer_t *MSV_FindPlayerName(char *playername)
{
	link_t *l;
	clusterplayer_t *pl;

	FOR_EACH_LINK(l, clusterplayers)
	{
		pl = STRUCT_FROM_LINK(l, clusterplayer_t, allplayers);
		if (!strcmp(pl->name, playername))
			return pl;
	}
	return NULL;
}
static void MSV_ServerCrashed(pubsubserver_t *server)
{
	link_t *l, *next;
	clusterplayer_t *pl;

	//forget any players that are meant to be on this server.
	for (l = clusterplayers.next ; l != &clusterplayers ; l = next)
	{
		next = l->next;
		pl = STRUCT_FROM_LINK(l, clusterplayer_t, allplayers);
		if (pl->server == server)
		{
			Con_Printf("%s(%s) crashed out\n", pl->name, server->name);
			RemoveLink(&pl->allplayers);
			Z_Free(pl);
		}
	}

	Z_Free(server);
}

pubsubserver_t *MSV_StartSubServer(unsigned int id, const char *mapname)
{
	sizebuf_t send;
	char send_buf[64];
	pubsubserver_t *s = Sys_ForkServer();
	if (s)
	{
		if (!id)
			id = ++nextserverid;
		s->id = id;
		s->next = subservers;
		subservers = s;

		Q_strncpyz(s->name, mapname, sizeof(s->name));

		memset(&send, 0, sizeof(send));
		send.data = send_buf;
		send.maxsize = sizeof(send_buf);
		send.cursize = 2;
		MSG_WriteByte(&send, ccmd_acceptserver);
		MSG_WriteLong(&send, s->id);
		MSG_WriteString(&send, s->name);
		Sys_InstructSlave(s, &send);
	}
	return s;
}

pubsubserver_t *MSV_FindSubServer(unsigned int id)
{
	pubsubserver_t *s;
	for (s = subservers; s; s = s->next)
	{
		if (id == s->id)
			return s;
	}

	return NULL;
}

//"5" finds server 5 only
//"5:" is equivelent to the above
//"5:dm4" finds server 5, and will start it if not known (even if a different node is running the same map)
//"0:dm4" starts a new server running dm4, even if its already running
//":dm4" finds any server running dm4. starts a new one if needed.
//"dm4" is ambiguous, in the case of a map beginning with a number (bah). don't use.
pubsubserver_t *MSV_FindSubServerName(const char *servername)
{
	pubsubserver_t *s;
	unsigned int id;
	qboolean forcenew = false;
	char *mapname;

	id = strtoul(servername, &mapname, 0);
	if (*mapname == ':')
	{
		if (!id && servername != mapname)
			forcenew = true;
		mapname++;
	}
	else if (*mapname)
	{
		Con_Printf("Invalid node name (lacks colon): %s\n", servername);
		mapname = "";
	}

	if (id)
	{
		s = MSV_FindSubServer(id);
		if (s)
			return s;
	}

	if (*mapname)
	{
		if (!forcenew)
		{
			for (s = subservers; s; s = s->next)
			{
				if (!strcmp(s->name, mapname))
					return s;
			}
		}

		return MSV_StartSubServer(id, mapname);
	}
	return NULL;
}
qboolean MSV_AddressForServer(netadr_t *ret, int natype, pubsubserver_t *s)
{
	if (s)
	{
		if (natype == s->addrv6.type)
			*ret = s->addrv6;
		else
			*ret = s->addrv4;
		return true;
	}
	return false;
}

void MSV_InstructSlave(unsigned int id, sizebuf_t *cmd)
{
	pubsubserver_t *s;
	if (!id)
	{
		for (s = subservers; s; s = s->next)
			Sys_InstructSlave(s, cmd);
	}
	else
	{
		s = MSV_FindSubServer(id);
		if (s)
			Sys_InstructSlave(s, cmd);
	}
}

void SV_SetupNetworkBuffers(qboolean bigcoords);

void MSV_MapCluster_f(void)
{
	char *sqlparams[] =
	{
		"",
		"",
		"",
		"login",
	};
	
	//this command will likely be used in configs. don't ever allow subservers to act as entire new clusters
	if (SSV_IsSubServer())
		return;

#ifndef SERVERONLY
	CL_Disconnect();
#endif

	if (sv.state)
		SV_UnspawnServer();
	NET_InitServer();

	//child processes return 0 and fall through
	memset(&sv, 0, sizeof(sv));
	if (atoi(Cmd_Argv(1)))
	{
		Con_Printf("Opening database \"%s\"\n", sqlparams[3]);
		sv.logindatabase = SQL_NewServer("sqlite", sqlparams);
		if (sv.logindatabase == -1)
		{
			SV_UnspawnServer();
			Con_Printf("Unable to open account database\n");
			return;
		}
	}
	else
	{
		sv.logindatabase = -1;
		Con_Printf("Operating in databaseless mode\n");
	}
	sv.state = ss_clustermode;
	ClearLink(&clusterplayers);

	//and for legacy clients, we need some server stuff inited.
	SV_SetupNetworkBuffers(false);
	SV_UpdateMaxPlayers(32);
}

void SSV_PrintToMaster(char *s)
{
	sizebuf_t send;
	char send_buf[8192];
	memset(&send, 0, sizeof(send));
	send.data = send_buf;
	send.maxsize = sizeof(send_buf);
	send.cursize = 2;

	MSG_WriteByte(&send, ccmd_print);
	MSG_WriteString(&send, s);
	SSV_InstructMaster(&send);
}

void MSV_Status(void)
{
	link_t *l;
	char bufmem[1024];
	pubsubserver_t *s;
	clusterplayer_t *pl;
	for (s = subservers; s; s = s->next)
	{
		Con_Printf("%i: %s", s->id, s->name);
		if (s->addrv4.type != NA_INVALID)
			Con_Printf(" %s", NET_AdrToString(bufmem, sizeof(bufmem), &s->addrv4));
		if (s->addrv6.type != NA_INVALID)
			Con_Printf(" %s", NET_AdrToString(bufmem, sizeof(bufmem), &s->addrv6));
		Con_Printf("\n");
	}

	FOR_EACH_LINK(l, clusterplayers)
	{
		pl = STRUCT_FROM_LINK(l, clusterplayer_t, allplayers);
		Con_Printf("%i(%s): (%s) %s (%s)\n", pl->playerid, pl->server->name, pl->guid, pl->name, pl->address);
	}
}
void MSV_SubServerCommand_f(void)
{
	sizebuf_t buf;
	char bufmem[1024];
	pubsubserver_t *s;
	int id;
	char *c;
	if (Cmd_Argc() == 1)
	{
		Con_Printf("Active servers on this cluster:\n");
		for (s = subservers; s; s = s->next)
		{
			Con_Printf("%i: %s", s->id, s->name);
			if (s->addrv4.type != NA_INVALID)
				Con_Printf(" %s", NET_AdrToString(bufmem, sizeof(bufmem), &s->addrv4));
			if (s->addrv6.type != NA_INVALID)
				Con_Printf(" %s", NET_AdrToString(bufmem, sizeof(bufmem), &s->addrv6));
			Con_Printf("\n");
		}
		return;
	}
	if (!strcmp(Cmd_Argv(0), "ssv_all"))
		id = 0;
	else
	{
		id = atoi(Cmd_Argv(1));
		Cmd_ShiftArgs(1, false);
	}

	buf.data = bufmem;
	buf.maxsize = sizeof(bufmem);
	buf.cursize = 2;
	buf.packing = SZ_RAWBYTES;
	c = Cmd_Args();
	MSG_WriteByte(&buf, ccmd_stuffcmd);
	MSG_WriteString(&buf, c);
	buf.data[0] = buf.cursize & 0xff;
	buf.data[1] = (buf.cursize>>8) & 0xff;
	MSV_InstructSlave(id, &buf);
}

void MSV_ReadFromSubServer(pubsubserver_t *s)
{
	sizebuf_t	send;
	qbyte		send_buf[MAX_QWMSGLEN];
	netadr_t adr;
	char *str;
	int c;

	c = MSG_ReadByte();
	switch(c)
	{
	default:
	case ccmd_bad:
		Sys_Error("Corrupt message (%i) from SubServer %i:%s", c, s->id, s->name);
		break;
	case ccmd_print:
		Con_Printf("^6%i(%s)^7: %s", s->id, s->name, MSG_ReadString());
		break;
	case ccmd_saveplayer:
		{
			clusterplayer_t *pl;
			float stats[NUM_SPAWN_PARMS];
			int i;
			unsigned char reason = MSG_ReadByte();
			unsigned int plid = MSG_ReadLong();
			int numstats = MSG_ReadByte();
			numstats = min(numstats, NUM_SPAWN_PARMS);
			for (i = 0; i < numstats; i++)
				stats[i] = MSG_ReadFloat();

			pl = MSV_FindPlayerId(plid);
			if (!pl)
			{
				Con_Printf("player %u(%s) does not exist!\n", plid, s->name);
				return;
			}
			//player already got taken by a different server, don't save stale data.
			if (reason && pl->server != s)
				return;

			MSV_UpdatePlayerStats(plid, s->id, numstats, stats);

			switch (reason)
			{
			case 0: //server reports that it accepted the player
				if (pl->server && pl->server != s)
				{	//let the previous server know
					sizebuf_t	send;
					qbyte		send_buf[64];
					memset(&send, 0, sizeof(send));
					send.data = send_buf;
					send.maxsize = sizeof(send_buf);
					send.cursize = 2;
					MSG_WriteByte(&send, ccmd_transferedplayer);
					MSG_WriteLong(&send, s->id);
					MSG_WriteLong(&send, plid);
					Sys_InstructSlave(pl->server, &send);
				}
				pl->server = s;
				break;
			case 2:	//drop
			case 3: //transfer abort
				if (pl->server == s)
				{
					Con_Printf("%s(%s) dropped\n", pl->name, s->name);
					RemoveLink(&pl->allplayers);
					Z_Free(pl);
				}
				break;
			}
		}
		break;
	case ccmd_transferplayer:
		{	//server is offering a player to another server
			char guid[64];
			char mapname[64];
			char plnamebuf[64];
			int plid = MSG_ReadLong();
			char *plname = MSG_ReadStringBuffer(plnamebuf, sizeof(plnamebuf));
			char *newmap = MSG_ReadStringBuffer(mapname, sizeof(mapname));
			char *claddr = MSG_ReadString();
			char *clguid = MSG_ReadStringBuffer(guid, sizeof(guid));
			pubsubserver_t *toptr;

			memset(&send, 0, sizeof(send));
			send.data = send_buf;
			send.maxsize = sizeof(send_buf);
			send.cursize = 2;

			if (NULL!=(toptr=MSV_FindSubServerName(newmap)) && s != toptr)
			{
//				Con_Printf("Transfer to %i:%s\n", toptr->id, toptr->name);

				MSG_WriteByte(&send, ccmd_takeplayer);
				MSG_WriteLong(&send, plid);
				MSG_WriteString(&send, plname);
				MSG_WriteLong(&send, s->id);
				MSG_WriteString(&send, claddr);
				MSG_WriteString(&send, clguid);

				c = MSG_ReadByte();
				MSG_WriteByte(&send, c);
//				Con_Printf("Transfer %i stats\n", c);
				while(c--)
					MSG_WriteFloat(&send, MSG_ReadFloat());

				Sys_InstructSlave(toptr, &send);
			}
			else
			{
				//suck up the stats
				c = MSG_ReadByte();
				while(c--)
					MSG_ReadFloat();

//				Con_Printf("Transfer abort\n");

				MSG_WriteByte(&send, ccmd_tookplayer);
				MSG_WriteLong(&send, s->id);
				MSG_WriteLong(&send, plid);
				MSG_WriteString(&send, "");

				Sys_InstructSlave(s, &send);
			}
		}
		break;
	case ccmd_tookplayer:
		{	//server has space, and wants the client.
			int to = MSG_ReadLong();
			int plid = MSG_ReadLong();
			char *claddr = MSG_ReadString();
			char *rmsg;
			netadr_t cladr;
			netadr_t svadr;
			char adrbuf[256];

//			Con_Printf("Took player\n");

			memset(&send, 0, sizeof(send));
			send.data = send_buf;
			send.maxsize = sizeof(send_buf);
			send.cursize = 2;

			NET_StringToAdr(claddr, 0, &cladr);
			MSV_AddressForServer(&svadr, cladr.type, s);
			if (!to)
			{
				if (svadr.type != NA_INVALID)
				{
					rmsg = va("fredir\n%s", NET_AdrToString(adrbuf, sizeof(adrbuf), &svadr));
					Netchan_OutOfBand (NS_SERVER, &cladr, strlen(rmsg), (qbyte *)rmsg);
				}
			}
			else
			{
				MSG_WriteByte(&send, ccmd_tookplayer);
				MSG_WriteLong(&send, s->id);
				MSG_WriteLong(&send, plid);
				MSG_WriteString(&send, NET_AdrToString(adrbuf, sizeof(adrbuf), &svadr));

				MSV_InstructSlave(to, &send);
			}
		}
		break;
	case ccmd_serveraddress:
		s->addrv4.type = NA_INVALID;
		s->addrv6.type = NA_INVALID;
		str = MSG_ReadString();
		Q_strncpyz(s->name, str, sizeof(s->name));
		for (;;)
		{
			str = MSG_ReadString();
			if (!*str)
				break;
			if (NET_StringToAdr(str, 0, &adr))
			{
				if (adr.type == NA_IP && s->addrv4.type == NA_INVALID)
					s->addrv4 = adr;
				if (adr.type == NA_IPV6 && s->addrv6.type == NA_INVALID)
					s->addrv6 = adr;
			}
		}
		Con_Printf("%i:%s: restarted\n", s->id, s->name);
		break;
	case ccmd_stringcmd:
		{
			char dest[1024];
			char from[1024];
			char cmd[1024];
			char info[1024];
			MSG_ReadStringBuffer(dest, sizeof(dest));
			MSG_ReadStringBuffer(from, sizeof(from));
			MSG_ReadStringBuffer(cmd, sizeof(cmd));
			MSG_ReadStringBuffer(info, sizeof(info));

			memset(&send, 0, sizeof(send));
			send.data = send_buf;
			send.maxsize = sizeof(send_buf);
			send.cursize = 2;

			MSG_WriteByte(&send, ccmd_stringcmd);
			MSG_WriteString(&send, dest);
			MSG_WriteString(&send, from);
			MSG_WriteString(&send, cmd);
			MSG_WriteString(&send, info);

			if (!*dest)	//broadcast if no dest
			{
				for (s = subservers; s; s = s->next)
					Sys_InstructSlave(s, &send);
			}
			else if (*dest == '\\')
			{
				//send to a specific server (backslashes should not be valid in infostrings, and thus not in names.
				//FIXME: broadcasting for now.
				for (s = subservers; s; s = s->next)
					Sys_InstructSlave(s, &send);
			}
			else
			{
				//send it to the server that the player is currently on.
				clusterplayer_t *pl = MSV_FindPlayerName(dest);
				if (pl)
					Sys_InstructSlave(pl->server, &send);
				else if (!pl && strncmp(cmd, "error:", 6))
				{
					//player not found. send it back to the sender, but add an error prefix.
					send.cursize = 2;
					MSG_WriteByte(&send, ccmd_stringcmd);
					MSG_WriteString(&send, from);
					MSG_WriteString(&send, dest);
					SZ_Write(&send, "error:", 6);
					MSG_WriteString(&send, cmd);
					MSG_WriteString(&send, info);
					Sys_InstructSlave(s, &send);
				}
			}
		}
		break;
	}
	if (msg_readcount != net_message.cursize || msg_badread)
		Sys_Error("Master: Readcount isn't right (%i)\n", net_message.data[0]);
}

void MSV_PollSlaves(void)
{
	pubsubserver_t **link, *s;
	for (link = &subservers; (s=*link); )
	{
		switch(Sys_SubServerRead(s))
		{
		case -1:
			//error - server is dead and needs to be freed.
			*link = s->next;
			MSV_ServerCrashed(s);
			break;
		case 0:
			//no messages
			link = &s->next;
			break;
		case 1:
			//got a message. read it and see if there's more.
			MSV_ReadFromSubServer(s);
			break;
		}
	}
}

void SSV_ReadFromControlServer(void)
{
	int c;
	char *s;

	c = MSG_ReadByte();
	switch(c)
	{
	case ccmd_bad:
	default:
		SV_Error("Invalid message from cluster (%i)\n", c);
		break;
	case ccmd_stuffcmd:
		s = MSG_ReadString();
		SV_BeginRedirect(RD_MASTER, 0);
		Cmd_ExecuteString(s, RESTRICT_LOCAL);
		SV_EndRedirect();
		break;

	case ccmd_acceptserver:
		svs.clusterserverid	= MSG_ReadLong();
		s = MSG_ReadString();
		if (*s && !strchr(s, ';') && !strchr(s, '\n') && !strchr(s, '\"'))	//sanity check the argument
			Cmd_ExecuteString(va("map \"%s\"", s), RESTRICT_LOCAL);
		if (svprogfuncs && pr_global_ptrs->serverid)
			*pr_global_ptrs->serverid = svs.clusterserverid;
		break;

	case ccmd_tookplayer:
		{
			client_t *cl = NULL;
			int to = MSG_ReadLong();
			int plid = MSG_ReadLong();
			char *addr = MSG_ReadString();
			int i;

			Con_Printf("%s: got tookplayer\n", sv.name);

			for (i = 0; i < svs.allocated_client_slots; i++)
			{
				if (svs.clients[i].state && svs.clients[i].userid == plid)
				{
					cl = &svs.clients[i];
					break;
				}
			}
			if (cl)
			{
				if (!*addr)
				{
					Con_Printf("%s: tookplayer: failed\n", sv.name);
					Info_SetValueForStarKey(cl->userinfo, "*transfer", "", sizeof(cl->userinfo));
				}
				else
				{
					Con_Printf("%s: tookplayer: do transfer\n", sv.name);
//					SV_StuffcmdToClient(cl, va("connect \"%s\"\n", addr));
					SV_StuffcmdToClient(cl, va("cl_transfer \"%s\"\n", addr));
					cl->redirect = 2;
				}
			}
			else
				Con_Printf("%s: tookplayer: invalid player.\n", sv.name);
		}
		break;

	case ccmd_transferedplayer:
		{
			client_t *cl;
			char *to;
			int toserver = MSG_ReadLong();
			int playerid = MSG_ReadLong();
			int i;

			for (i = 0; i < svs.allocated_client_slots; i++)
			{
				if (svs.clients[i].userid == playerid && svs.clients[i].state >= cs_loadzombie)
				{
					cl = &svs.clients[i];
					cl->drop = true;
					to = Info_ValueForKey(cl->userinfo, "*transfer");
					Con_Printf("%s transfered to %s\n", cl->name, to);
					break;
				}
			}
		}
		break;

	case ccmd_takeplayer:
		{
			client_t *cl = NULL;
			int i, j;
			float stat;
			char guid[64], name[64];
			int plid = MSG_ReadLong();
			char *plname = MSG_ReadStringBuffer(name, sizeof(name));
			int fromsv = MSG_ReadLong();
			char *claddr = MSG_ReadString();
			char *clguid = MSG_ReadStringBuffer(guid, sizeof(guid));

			if (sv.state >= ss_active)
			{
				for (i = 0; i < svs.allocated_client_slots; i++)
				{
					if (!svs.clients[i].state || (svs.clients[i].userid == plid && svs.clients[i].state >= cs_loadzombie))
					{
						cl = &svs.clients[i];
						break;
					}
				}
			}

//			Con_Printf("%s: takeplayer\n", sv.name);
			if (cl)
			{
				cl->userid = plid;
				if (cl->state == cs_loadzombie && cl->istobeloaded)
					cl->connection_started = realtime+20;	//renew the slot
				else if (!cl->state)
				{	//allocate a new pending player.
					cl->state = cs_loadzombie;
					cl->connection_started = realtime+20;
					Q_strncpyz(cl->guid, clguid, sizeof(cl->guid));
					Q_strncpyz(cl->namebuf, plname, sizeof(cl->namebuf));
					cl->name = cl->namebuf;
					sv.spawned_client_slots++;
					memset(&cl->netchan, 0, sizeof(cl->netchan));
					SV_GetNewSpawnParms(cl);
				}
				//else: already on the server somehow. laggy/dupe request? must be.
			}
			else
			{
				Con_Printf("%s: server full!\n", sv.name);
			}

			j = MSG_ReadByte();
//			Con_Printf("%s: %i stats\n", sv.name, j);
			for (i = 0; i < j; i++)
			{
				stat = MSG_ReadFloat();
				if (cl && cl->state == cs_loadzombie && i < NUM_SPAWN_PARMS)
					cl->spawn_parms[i] = stat;
			}

			{
				sizebuf_t	send;
				qbyte		send_buf[MAX_QWMSGLEN];

				memset(&send, 0, sizeof(send));
				send.data = send_buf;
				send.maxsize = sizeof(send_buf);
				send.cursize = 2;

				if (cl)
				{
					MSG_WriteByte(&send, ccmd_tookplayer);
					MSG_WriteLong(&send, fromsv);
					MSG_WriteLong(&send, plid);
					MSG_WriteString(&send, claddr);
					SSV_InstructMaster(&send);
				}
			}
		}
		break;
	case ccmd_stringcmd:
		{
			char dest[1024];
			char from[1024];
			char cmd[1024];
			char info[1024];
			int i;
			client_t *cl;
			MSG_ReadStringBuffer(dest, sizeof(dest));
			MSG_ReadStringBuffer(from, sizeof(from));
			MSG_ReadStringBuffer(cmd, sizeof(cmd));
			MSG_ReadStringBuffer(info, sizeof(info));

			if (!PR_ParseClusterEvent(dest, from, cmd, info))
			{
				//meh, lets make some lame fallback thing
				for (i = 0; i < sv.allocated_client_slots; i++)
				{
					cl = &svs.clients[i];
					if (cl->state >= ca_connected)
					if (!*dest || !strcmp(dest, cl->name))
					{
						if (!strcmp(cmd, "say"))
							SV_PrintToClient(cl, PRINT_HIGH, va("^[%s^]: %s\n", from, info));
						else if (!strcmp(cmd, "join"))
						{
							SV_PrintToClient(cl, PRINT_HIGH, va("^[%s^] is joining you\n", from));
							SSV_Send(from, cl->name, "joinnode", va("%i", svs.clusterserverid));
						}
						else if (!strcmp(cmd, "joinnode"))
						{
							SSV_InitiatePlayerTransfer(cl, info);
						}
						else
							SV_PrintToClient(cl, PRINT_HIGH, va("%s from [%s]: %s\n", cmd, from, info));
						if (*dest)
							break;
					}
				}
			}
		}
		break;
	}

	if (msg_readcount != net_message.cursize || msg_badread)
		Sys_Error("Subserver: Readcount isn't right (%i)\n", net_message.data[0]);
}

void SSV_UpdateAddresses(void)
{
	char		buf[256];
	netadr_t	addr[64];
	struct ftenet_generic_connection_s			*con[sizeof(addr)/sizeof(addr[0])];
	int			flags[sizeof(addr)/sizeof(addr[0])];
	int			count;
	sizebuf_t	send;
	qbyte		send_buf[MAX_QWMSGLEN];
	int i;

	if (!SSV_IsSubServer())
		return;

	count = NET_EnumerateAddresses(svs.sockets, con, flags, addr, sizeof(addr)/sizeof(addr[0]));

	if (*sv_serverip.string)
	{
		for (i = 0; i < count; i++)
		{
			if (addr[i].type == NA_IP)
			{
				NET_StringToAdr2(sv_serverip.string, BigShort(addr[i].port), &addr[0], sizeof(addr)/sizeof(addr[0]));
				count = 1;
				break;
			}
		}
	}

	memset(&send, 0, sizeof(send));
	send.data = send_buf;
	send.maxsize = sizeof(send_buf);
	send.cursize = 2;
	MSG_WriteByte(&send, ccmd_serveraddress);

	MSG_WriteString(&send, sv.name);
	for (i = 0; i < count; i++)
		MSG_WriteString(&send, NET_AdrToString(buf, sizeof(buf), &addr[i]));
	MSG_WriteByte(&send, 0);
	SSV_InstructMaster(&send);
}

void SSV_SavePlayerStats(client_t *cl, int reason)
{
	//called when the *transfer userinfo gets set to the new map
	sizebuf_t	send;
	qbyte		send_buf[MAX_QWMSGLEN];
	int i;
	if (!SSV_IsSubServer())
		return;

	if ((reason == 1 || reason == 2) && cl->edict)
		SV_SaveSpawnparmsClient(cl, NULL);

	memset(&send, 0, sizeof(send));
	send.data = send_buf;
	send.maxsize = sizeof(send_buf);
	send.cursize = 2;

	MSG_WriteByte(&send, ccmd_saveplayer);
	MSG_WriteByte(&send, reason);
	MSG_WriteLong(&send, cl->userid);
	MSG_WriteByte(&send, NUM_SPAWN_PARMS);
	for (i = 0; i < NUM_SPAWN_PARMS; i++)
	{
		MSG_WriteFloat(&send, cl->spawn_parms[i]);
	}

	SSV_InstructMaster(&send);
}
void SSV_Send(const char *dest, const char *src, const char *cmd, const char *msg)
{
	//called when the *transfer userinfo gets set to the new map
	sizebuf_t	send;
	qbyte		send_buf[MAX_QWMSGLEN];
	if (!SSV_IsSubServer())
		return;

	memset(&send, 0, sizeof(send));
	send.data = send_buf;
	send.maxsize = sizeof(send_buf);
	send.cursize = 2;

	MSG_WriteByte(&send, ccmd_stringcmd);
	MSG_WriteString(&send, dest?dest:"");
	MSG_WriteString(&send, src?src:"");
	MSG_WriteString(&send, cmd?cmd:"");
	MSG_WriteString(&send, msg?msg:"");

	SSV_InstructMaster(&send);
}
void SSV_InitiatePlayerTransfer(client_t *cl, const char *newserver)
{
	//called when the *transfer userinfo gets set to the new map
	sizebuf_t	send;
	qbyte		send_buf[MAX_QWMSGLEN];
	int i;
	char tmpbuf[256];
	float parms[NUM_SPAWN_PARMS];

	SV_SaveSpawnparmsClient(cl, parms);

	memset(&send, 0, sizeof(send));
	send.data = send_buf;
	send.maxsize = sizeof(send_buf);
	send.cursize = 2;
	MSG_WriteByte(&send, ccmd_transferplayer);
	MSG_WriteLong(&send, cl->userid);
	MSG_WriteString(&send, cl->name);
	MSG_WriteString(&send, newserver);
	MSG_WriteString(&send, NET_AdrToString(tmpbuf, sizeof(tmpbuf), &cl->netchan.remote_address));
	MSG_WriteString(&send, cl->guid);

	//stats
	MSG_WriteByte(&send, NUM_SPAWN_PARMS);
	for (i = 0; i < NUM_SPAWN_PARMS; i++)
	{
		MSG_WriteFloat(&send, parms[i]);
	}

	SSV_InstructMaster(&send);
}

#ifdef SQL
#include "sv_sql.h"
int pendinglookups = 0;
struct logininfo_s
{
	netadr_t clientaddr;
	char guid[64];
	char name[64];
};
#endif
qboolean SV_IgnoreSQLResult(queryrequest_t *req, int firstrow, int numrows, int numcols, qboolean eof)
{
	return false;
}
void MSV_UpdatePlayerStats(unsigned int playerid, unsigned int serverid, int numstats, float *stats)
{
	queryrequest_t *req;
	sqlserver_t *srv;
	static char hex[16] = "0123456789abcdef";
	char sql[2048], *sqle;
	union{float *f;qbyte *b;} blob;
	if (sv.logindatabase != -1)
	{
		Q_snprintfz(sql, sizeof(sql), "UPDATE accounts SET stats=x'");
		sqle = sql+strlen(sql);
		for (blob.f = stats, numstats*=4; numstats--; blob.b++)
		{
			*sqle++ = hex[*blob.b>>4];
			*sqle++ = hex[*blob.b&15];
		}
		Q_snprintfz(sqle, sizeof(sql)-(sqle-sql), "', serverid=%u WHERE playerid = %u;", serverid, playerid);

		srv = SQL_GetServer(sv.logindatabase, false);
		if (srv)
			SQL_NewQuery(srv, SV_IgnoreSQLResult, sql, &req);
	}
}

qboolean MSV_ClusterLoginReply(netadr_t *legacyclientredirect, unsigned int serverid, unsigned int playerid, char *playername, char *clientguid, netadr_t *clientaddr, void *statsblob, size_t statsblobsize)
{
	char tmpbuf[256];
	netadr_t serveraddr;
	pubsubserver_t *s = NULL;

	if (!s)
		s = MSV_FindSubServerName(":start");

	if (!s || !MSV_AddressForServer(&serveraddr, clientaddr->type, s))
		SV_RejectMessage(SCP_QUAKEWORLD, "Unable to find lobby.\n");
	else
	{
		sizebuf_t	send;
		qbyte		send_buf[MAX_QWMSGLEN];
		clusterplayer_t *pl;
		memset(&send, 0, sizeof(send));
		send.data = send_buf;
		send.maxsize = sizeof(send_buf);
		send.cursize = 2;

		pl = Z_Malloc(sizeof(*pl));
		Q_strncpyz(pl->name, playername, sizeof(pl->name));
		Q_strncpyz(pl->guid, clientguid, sizeof(pl->guid));
		NET_AdrToString(pl->address, sizeof(pl->address), clientaddr);
		pl->playerid = playerid;
		InsertLinkBefore(&pl->allplayers, &clusterplayers);
		pl->server = s;
		
		MSG_WriteByte(&send, ccmd_takeplayer);
		MSG_WriteLong(&send, playerid);
		MSG_WriteString(&send, pl->name);
		MSG_WriteLong(&send, 0);	//from server
		MSG_WriteString(&send, NET_AdrToString(tmpbuf, sizeof(tmpbuf), &net_from));
		MSG_WriteString(&send, clientguid);

		MSG_WriteByte(&send, statsblobsize/4);
		SZ_Write(&send, statsblob, statsblobsize&~3);
		Sys_InstructSlave(s, &send);

		if (serveraddr.type == NA_INVALID)
		{
			if (net_from.type != NA_LOOPBACK)
				SV_RejectMessage(SCP_QUAKEWORLD, "Starting instance.\n");
		}
		else if (legacyclientredirect)
		{
			*legacyclientredirect = serveraddr;
			return true;
		}
		else
		{
			char *s = va("fredir\n%s", NET_AdrToString(tmpbuf, sizeof(tmpbuf), &serveraddr));
			Netchan_OutOfBand (NS_SERVER, clientaddr, strlen(s), (qbyte *)s);
			return true;
		}
	}
	return false;
}

qboolean MSV_ClusterLoginSQLResult(queryrequest_t *req, int firstrow, int numrows, int numcols, qboolean eof)
{
	sqlserver_t *sql = SQL_GetServer(req->srvid, true);
	queryresult_t *res = SQL_GetQueryResult(sql, req->num, 0);
	struct logininfo_s *info = req->user.thread;
	char *s;
	int playerid, serverid;
	char *statsblob;
	size_t blobsize;

	res = SQL_GetQueryResult(sql, req->num, 0);
	if (!res)
	{
		playerid = 0;
		statsblob = NULL;
		blobsize = 0;
		serverid = 0;
	}
	else
	{
		s = SQL_ReadField(sql, res, 0, 0, true, NULL);
		playerid = atoi(s);

		statsblob = SQL_ReadField(sql, res, 0, 2, true, &blobsize);

		s = SQL_ReadField(sql, res, 0, 1, true, NULL);
		serverid = s?atoi(s):0;
	}

	net_from = info->clientaddr;	//okay, that's a bit stupid, rewrite rejectmessage to accept an arg?
	if (!playerid)
		SV_RejectMessage(SCP_QUAKEWORLD, "Bad username or password.\n");
	else
		MSV_ClusterLoginReply(NULL, serverid, playerid, info->name, info->guid, &info->clientaddr, statsblob, blobsize);
	Z_Free(info);
	pendinglookups--;
	return false;
}

//returns true to block entry to this server.
extern int	nextuserid;
qboolean MSV_ClusterLogin(char *guid, char *userinfo, size_t userinfosize)
{
	char escname[64], escpasswd[64];
	sqlserver_t *sql;
	queryrequest_t *req;
	struct logininfo_s *info;

	if (sv.state != ss_clustermode)
		return false;

	/*if (!*guid)
	{
		SV_RejectMessage(SCP_QUAKEWORLD, "No guid info, please set cl_sendguid to 1.\n");
		return false;
	}*/

	if (sv.logindatabase != -1)
	{
		if (pendinglookups > 10)
			return true;
		sql = SQL_GetServer(sv.logindatabase, false);
		if (!sql)
			return true;
		SQL_Escape(sql, Info_ValueForKey(userinfo, "name"), escname, sizeof(escname)); 
		SQL_Escape(sql, Info_ValueForKey(userinfo, "password"), escpasswd, sizeof(escpasswd));
		if (SQL_NewQuery(sql, MSV_ClusterLoginSQLResult, va("SELECT playerid,serverid,stats FROM accounts WHERE name='%s' AND password='%s';", escname, escpasswd), &req) != -1)
		{
			pendinglookups++;
			req->user.thread = info = Z_Malloc(sizeof(*info));
			Q_strncpyz(info->guid, guid, sizeof(info->guid));
			info->clientaddr = net_from;
		}
	}
/*	else if (0)
	{
		char tmpbuf[256];
		netadr_t redir;
		if (MSV_ClusterLoginReply(&redir, 0, nextuserid++, guid, &net_from, NULL, 0))
		{
			Info_SetValueForStarKey(userinfo, "*redirect", NET_AdrToString(tmpbuf, sizeof(tmpbuf), &redir), userinfosize);
			return false;
		}
		return true;
	}*/
	else
		MSV_ClusterLoginReply(NULL, 0, ++nextuserid, Info_ValueForKey(userinfo, "name"), guid, &net_from, NULL, 0);
	return true;
}
#endif
