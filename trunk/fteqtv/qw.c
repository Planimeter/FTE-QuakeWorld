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


#include "qtv.h"

static const filename_t ConnectionlessModelList[] = {{""}, {"maps/start.bsp"}, {"progs/player.mdl"}, {""}};
static const filename_t ConnectionlessSoundList[] = {{""}, {""}};

void QTV_Say(cluster_t *cluster, sv_t *qtv, viewer_t *v, char *message, qboolean noupwards);

void QTV_DefaultMovevars(movevars_t *vars)
{
	vars->gravity = 800;
	vars->maxspeed = 320;
	vars->spectatormaxspeed = 500;
	vars->accelerate = 10;
	vars->airaccelerate = 0.7f;
	vars->waterfriction = 4;
	vars->entgrav = 1;
	vars->stopspeed = 10;
	vars->wateraccelerate = 10;
	vars->friction = 4;
}


void Menu_Enter(cluster_t *cluster, viewer_t *viewer, int buttonnum);

#if defined(_WIN32) && !defined(__MINGW32_VERSION)
int snprintf(char *buffer, int buffersize, char *format, ...)
{
	va_list		argptr;
	int ret;
	va_start (argptr, format);
	ret = _vsnprintf (buffer, buffersize, format, argptr);
	buffer[buffersize - 1] = '\0';
	va_end (argptr);

	return ret;
}
#endif
#if (defined(_WIN32) && !defined(_VC80_UPGRADE) && !defined(__MINGW32_VERSION))
int vsnprintf(char *buffer, int buffersize, char *format, va_list argptr)
{
	int ret;
	ret = _vsnprintf (buffer, buffersize, format, argptr);
	buffer[buffersize - 1] = '\0';

	return ret;
}
#endif

const usercmd_t nullcmd;

#define	CM_ANGLE1 	(1<<0)
#define	CM_ANGLE3 	(1<<1)
#define	CM_FORWARD	(1<<2)
#define	CM_SIDE		(1<<3)
#define	CM_UP		(1<<4)
#define	CM_BUTTONS	(1<<5)
#define	CM_IMPULSE	(1<<6)
#define	CM_ANGLE2 	(1<<7)
void ReadDeltaUsercmd (netmsg_t *m, const usercmd_t *from, usercmd_t *move)
{
	int bits;

	memcpy (move, from, sizeof(*move));

	bits = ReadByte (m);

// read current angles
	if (bits & CM_ANGLE1)
		move->angles[0] = ReadShort (m);
	if (bits & CM_ANGLE2)
		move->angles[1] = ReadShort (m);
	if (bits & CM_ANGLE3)
		move->angles[2] = ReadShort (m);

// read movement
	if (bits & CM_FORWARD)
		move->forwardmove = ReadShort(m);
	if (bits & CM_SIDE)
		move->sidemove = ReadShort(m);
	if (bits & CM_UP)
		move->upmove = ReadShort(m);

// read buttons
	if (bits & CM_BUTTONS)
		move->buttons = ReadByte (m);

	if (bits & CM_IMPULSE)
		move->impulse = ReadByte (m);

// read time to run command
	move->msec = ReadByte (m);		// always sent
}

void WriteDeltaUsercmd (netmsg_t *m, const usercmd_t *from, usercmd_t *move)
{
	int bits = 0;

	if (move->angles[0] != from->angles[0])
		bits |= CM_ANGLE1;
	if (move->angles[1] != from->angles[1])
		bits |= CM_ANGLE2;
	if (move->angles[2] != from->angles[2])
		bits |= CM_ANGLE3;

	if (move->forwardmove != from->forwardmove)
		bits |= CM_FORWARD;
	if (move->sidemove != from->sidemove)
		bits |= CM_SIDE;
	if (move->upmove != from->upmove)
		bits |= CM_UP;

	if (move->buttons != from->buttons)
		bits |= CM_BUTTONS;
	if (move->impulse != from->impulse)
		bits |= CM_IMPULSE;


	WriteByte (m, bits);

// read current angles
	if (bits & CM_ANGLE1)
		WriteShort (m, move->angles[0]);
	if (bits & CM_ANGLE2)
		WriteShort (m, move->angles[1]);
	if (bits & CM_ANGLE3)
		WriteShort (m, move->angles[2]);

// read movement
	if (bits & CM_FORWARD)
		WriteShort(m, move->forwardmove);
	if (bits & CM_SIDE)
		WriteShort(m, move->sidemove);
	if (bits & CM_UP)
		WriteShort(m, move->upmove);

// read buttons
	if (bits & CM_BUTTONS)
		WriteByte (m, move->buttons);

	if (bits & CM_IMPULSE)
		WriteByte (m, move->impulse);

// read time to run command
	WriteByte (m, move->msec);		// always sent
}










SOCKET QW_InitUDPSocket(int port)
{
	int sock;

	struct sockaddr_in	address;
//	int fromlen;

	unsigned long nonblocking = true;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons((u_short)port);



	if ((sock = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET)
	{
		return INVALID_SOCKET;
	}

	if (ioctlsocket (sock, FIONBIO, &nonblocking) == -1)
	{
		closesocket(sock);
		return INVALID_SOCKET;
	}

	if( bind (sock, (void *)&address, sizeof(address)) == -1)
	{
		closesocket(sock);
		return INVALID_SOCKET;
	}
	return sock;
}

void BuildServerData(sv_t *tv, netmsg_t *msg, int servercount, viewer_t *viewer)
{
	movevars_t movevars;
	WriteByte(msg, svc_serverdata);
	WriteLong(msg, PROTOCOL_VERSION);
	WriteLong(msg, servercount);

	if (!tv)
	{
		//dummy connection, for choosing a game to watch.
		WriteString(msg, "qw");

		if (!viewer)
			WriteFloat(msg, 0);
		else
			WriteByte(msg, (MAX_CLIENTS-1) | (128));
		WriteString(msg, "FTEQTV Proxy");


		// get the movevars
		QTV_DefaultMovevars(&movevars);
		WriteFloat(msg, movevars.gravity);
		WriteFloat(msg, movevars.stopspeed);
		WriteFloat(msg, movevars.maxspeed);
		WriteFloat(msg, movevars.spectatormaxspeed);
		WriteFloat(msg, movevars.accelerate);
		WriteFloat(msg, movevars.airaccelerate);
		WriteFloat(msg, movevars.wateraccelerate);
		WriteFloat(msg, movevars.friction);
		WriteFloat(msg, movevars.waterfriction);
		WriteFloat(msg, movevars.entgrav);



		WriteByte(msg, svc_stufftext);
		WriteString2(msg, "fullserverinfo \"");
		WriteString2(msg, "\\*QTV\\"VERSION);
		WriteString(msg, "\"\n");

	}
	else
	{
		WriteString(msg, tv->gamedir);

		if (!viewer)
			WriteFloat(msg, 0);
		else
		{
			if (tv->controller == viewer)
				WriteByte(msg, viewer->thisplayer);
			else
				WriteByte(msg, viewer->thisplayer | 128);
		}
		WriteString(msg, tv->mapname);


		// get the movevars
		WriteFloat(msg, tv->movevars.gravity);
		WriteFloat(msg, tv->movevars.stopspeed);
		WriteFloat(msg, tv->movevars.maxspeed);
		WriteFloat(msg, tv->movevars.spectatormaxspeed);
		WriteFloat(msg, tv->movevars.accelerate);
		WriteFloat(msg, tv->movevars.airaccelerate);
		WriteFloat(msg, tv->movevars.wateraccelerate);
		WriteFloat(msg, tv->movevars.friction);
		WriteFloat(msg, tv->movevars.waterfriction);
		WriteFloat(msg, tv->movevars.entgrav);



		WriteByte(msg, svc_stufftext);
		WriteString2(msg, "fullserverinfo \"");
		WriteString2(msg, tv->serverinfo);
		WriteString(msg, "\"\n");
	}
}
void BuildNQServerData(sv_t *tv, netmsg_t *msg, qboolean mvd, int playernum)
{
	int i;
	WriteByte(msg, svc_serverdata);
	WriteLong(msg, PROTOCOL_VERSION_NQ);
	WriteByte(msg, 16);	//MAX_CLIENTS
	WriteByte(msg, 1);	//game type


	if (!tv || tv->parsingconnectiondata )
	{
		//dummy connection, for choosing a game to watch.
		WriteString(msg, "FTEQTV Proxy");


		//modellist
		for (i = 1; *ConnectionlessModelList[i].name; i++)
		{
			WriteString(msg, ConnectionlessModelList[i].name);
		}
		WriteString(msg, "");

		//soundlist
		for (i = 1; *ConnectionlessSoundList[i].name; i++)
		{
			WriteString(msg, ConnectionlessSoundList[i].name);
		}
		WriteString(msg, "");

		WriteByte(msg, svc_cdtrack);
		WriteByte(msg, 0);	//two of them, yeah... weird, eh?
		WriteByte(msg, 0);

		WriteByte(msg, svc_nqsetview);
		WriteShort(msg, playernum);

		WriteByte(msg, svc_nqsignonnum);
		WriteByte(msg, 1);
	}
	else
	{
		//dummy connection, for choosing a game to watch.
		WriteString(msg, "FTEQTV Proxy");


		//modellist
		for (i = 1; *tv->modellist[i].name; i++)
		{
			WriteString(msg, tv->modellist[i].name);
		}
		WriteString(msg, "");

		//soundlist
		for (i = 1; *tv->soundlist[i].name; i++)
		{
			WriteString(msg, tv->soundlist[i].name);
		}
		WriteString(msg, "");

		WriteByte(msg, svc_cdtrack);
		WriteByte(msg, tv->cdtrack);	//two of them, yeah... weird, eh?
		WriteByte(msg, tv->cdtrack);

		WriteByte(msg, svc_nqsetview);
		WriteShort(msg, 15);

		WriteByte(msg, svc_nqsignonnum);
		WriteByte(msg, 1);
	}
}

void SendServerData(sv_t *tv, viewer_t *viewer)
{
	netmsg_t msg;
	char buffer[MAX_NQMSGLEN];

	if (viewer->netchan.isnqprotocol)
		InitNetMsg(&msg, buffer, sizeof(buffer));
	else
		InitNetMsg(&msg, buffer,1024);

	if (tv && (tv->controller == viewer || !tv->controller))
		viewer->thisplayer = tv->thisplayer;
	else
		viewer->thisplayer = viewer->netchan.isnqprotocol?15:MAX_CLIENTS-1;
	if (viewer->netchan.isnqprotocol)
		BuildNQServerData(tv, &msg, false, viewer->thisplayer);
	else
		BuildServerData(tv, &msg, viewer->servercount, viewer);

	SendBufferToViewer(viewer, msg.data, msg.cursize, true);

	viewer->thinksitsconnected = false;

	memset(viewer->currentstats, 0, sizeof(viewer->currentstats));
}

void SendNQSpawnInfoToViewer(cluster_t *cluster, viewer_t *viewer, netmsg_t *msg)
{
	char buffer[64];
	int i;
	int colours;
	sv_t *tv = viewer->server;
	WriteByte(msg, svc_nqtime);
	WriteFloat(msg, cluster->curtime/1000.0f);

	if (tv)
	{
		for (i=0; i<MAX_CLIENTS && i < 16; i++)
		{
			WriteByte (msg, svc_nqupdatename);
			WriteByte (msg, i);
			Info_ValueForKey(tv->players[i].userinfo, "name", buffer, sizeof(buffer));
			WriteString (msg, buffer);	//fixme

			WriteByte (msg, svc_updatefrags);
			WriteByte (msg, i);
			WriteShort (msg, tv->players[i].frags);

			Info_ValueForKey(tv->players[i].userinfo, "bottomcolor", buffer, sizeof(buffer));
			colours = atoi(buffer);
			Info_ValueForKey(tv->players[i].userinfo, "topcolor", buffer, sizeof(buffer));
			colours |= atoi(buffer)*16;
			WriteByte (msg, svc_nqupdatecolors);
			WriteByte (msg, i);
			WriteByte (msg, colours);
		}
	}
	else
	{
		for (i=0; i < 16; i++)
		{
			WriteByte (msg, svc_nqupdatename);
			WriteByte (msg, i);
			WriteString (msg, "");
			WriteByte (msg, svc_updatefrags);
			WriteByte (msg, i);
			WriteShort (msg, 0);
			WriteByte (msg, svc_nqupdatecolors);
			WriteByte (msg, i);
			WriteByte (msg, 0);
		}
	}

	WriteByte(msg, svc_nqsignonnum);
	WriteByte(msg, 3);
}

int SendCurrentUserinfos(sv_t *tv, int cursize, netmsg_t *msg, int i, int thisplayer)
{
	if (i < 0)
		return i;
	if (i >= MAX_CLIENTS)
		return i;

	for (; i < MAX_CLIENTS; i++)
	{
		if (i == thisplayer && (!tv || !(tv->controller || tv->proxyplayer)))
		{
			WriteByte(msg, svc_updateuserinfo);
			WriteByte(msg, i);
			WriteLong(msg, i);
			WriteString2(msg, "\\*spectator\\1\\name\\");

			if (tv && tv->hostname[0])
				WriteString(msg, tv->hostname);
			else
				WriteString(msg, "FTEQTV");


			WriteByte(msg, svc_updatefrags);
			WriteByte(msg, i);
			WriteShort(msg, 9999);

			WriteByte(msg, svc_updateping);
			WriteByte(msg, i);
			WriteShort(msg, 0);

			WriteByte(msg, svc_updatepl);
			WriteByte(msg, i);
			WriteByte(msg, 0);

			continue;
		}
		if (!tv)
			continue;
		if (msg->cursize+cursize+strlen(tv->players[i].userinfo) > 768)
		{
			return i;
		}
		WriteByte(msg, svc_updateuserinfo);
		WriteByte(msg, i);
		WriteLong(msg, i);
		WriteString(msg, tv->players[i].userinfo);

		WriteByte(msg, svc_updatefrags);
		WriteByte(msg, i);
		WriteShort(msg, tv->players[i].frags);

		WriteByte(msg, svc_updateping);
		WriteByte(msg, i);
		WriteShort(msg, tv->players[i].ping);

		WriteByte(msg, svc_updatepl);
		WriteByte(msg, i);
		WriteByte(msg, tv->players[i].packetloss);
	}

	i++;

	return i;
}
void WriteEntityState(netmsg_t *msg, entity_state_t *es)
{
	int i;
	WriteByte(msg, es->modelindex);
	WriteByte(msg, es->frame);
	WriteByte(msg, es->colormap);
	WriteByte(msg, es->skinnum);
	for (i = 0; i < 3; i++)
	{
		WriteShort(msg, es->origin[i]);
		WriteByte(msg, es->angles[i]);
	}
}
int SendCurrentBaselines(sv_t *tv, int cursize, netmsg_t *msg, int maxbuffersize, int i)
{

	if (i < 0 || i >= MAX_ENTITIES)
		return i;

	for (; i < MAX_ENTITIES; i++)
	{
		if (msg->cursize+cursize+16 > maxbuffersize)
		{
			return i;
		}

		if (tv->entity[i].baseline.modelindex)
		{
			WriteByte(msg, svc_spawnbaseline);
			WriteShort(msg, i);
			WriteEntityState(msg, &tv->entity[i].baseline);
		}
	}

	return i;
}
int SendCurrentLightmaps(sv_t *tv, int cursize, netmsg_t *msg, int maxbuffersize, int i)
{
	if (i < 0 || i >= MAX_LIGHTSTYLES)
		return i;

	for (; i < MAX_LIGHTSTYLES; i++)
	{
		if (msg->cursize+cursize+strlen(tv->lightstyle[i].name) > maxbuffersize)
		{
			return i;
		}
		WriteByte(msg, svc_lightstyle);
		WriteByte(msg, i);
		WriteString(msg, tv->lightstyle[i].name);
	}
	return i;
}
int SendStaticSounds(sv_t *tv, int cursize, netmsg_t *msg, int maxbuffersize, int i)
{
	if (i < 0 || i >= MAX_STATICSOUNDS)
		return i;

	for (; i < MAX_STATICSOUNDS; i++)
	{
		if (msg->cursize+cursize+16 > maxbuffersize)
		{
			return i;
		}
		if (!tv->staticsound[i].soundindex)
			continue;

		WriteByte(msg, svc_spawnstaticsound);
		WriteShort(msg, tv->staticsound[i].origin[0]);
		WriteShort(msg, tv->staticsound[i].origin[1]);
		WriteShort(msg, tv->staticsound[i].origin[2]);
		WriteByte(msg, tv->staticsound[i].soundindex);
		WriteByte(msg, tv->staticsound[i].volume);
		WriteByte(msg, tv->staticsound[i].attenuation);
	}

	return i;
}
int SendStaticEntities(sv_t *tv, int cursize, netmsg_t *msg, int maxbuffersize, int i)
{
	if (i < 0 || i >= MAX_STATICENTITIES)
		return i;

	for (; i < MAX_STATICENTITIES; i++)
	{
		if (msg->cursize+cursize+16 > maxbuffersize)
		{
			return i;
		}
		if (!tv->spawnstatic[i].modelindex)
			continue;

		WriteByte(msg, svc_spawnstatic);
		WriteEntityState(msg, &tv->spawnstatic[i]);
	}

	return i;
}

int SendList(sv_t *qtv, int first, const filename_t *list, int svc, netmsg_t *msg)
{
	int i;

	WriteByte(msg, svc);
	WriteByte(msg, first);
	for (i = first+1; i < 256; i++)
	{
//		printf("write %i: %s\n", i, list[i].name);
		WriteString(msg, list[i].name);
		if (!*list[i].name)	//fixme: this probably needs testing for where we are close to the limit
		{	//no more
			WriteByte(msg, 0);
			return -1;
		}

		if (msg->cursize > 768)
		{	//truncate
			i--;
			break;
		}
	}
	WriteByte(msg, 0);
	WriteByte(msg, i);

	return i;
}

void QW_StreamPrint(cluster_t *cluster, sv_t *server, viewer_t *allbut, char *message)
{
	viewer_t *v;

	for (v = cluster->viewers; v; v = v->next)
	{
		if (v->server == server)
		{
			if (v == allbut)
				continue;
			QW_PrintfToViewer(v, "%s", message);
		}
	}
}


void QW_StreamStuffcmd(cluster_t *cluster, sv_t *server, char *fmt, ...)
{
	viewer_t *v;
	va_list		argptr;
	char buf[1024];
	char cmd[512];

	netmsg_t msg;

	va_start (argptr, fmt);
	vsnprintf (cmd, sizeof(cmd), fmt, argptr);
	va_end (argptr);

	InitNetMsg(&msg, buf, sizeof(buf));
	WriteByte(&msg, svc_stufftext);
	WriteString(&msg, cmd);
	

	for (v = cluster->viewers; v; v = v->next)
	{
		if (v->server == server)
		{
			SendBufferToViewer(v, msg.data, msg.cursize, true);
		}
	}
}



void QW_SetViewersServer(cluster_t *cluster, viewer_t *viewer, sv_t *sv)
{
	char buffer[1024];
	sv_t *oldserver;
	oldserver = viewer->server;
	if (viewer->server)
		viewer->server->numviewers--;
	viewer->server = sv;
	if (viewer->server)
		viewer->server->numviewers++;
	if (!sv || !sv->parsingconnectiondata)
	{
		QW_StuffcmdToViewer(viewer, "cmd new\n");
		viewer->thinksitsconnected = false;
	}
	viewer->servercount++;
	viewer->origin[0] = 0;
	viewer->origin[1] = 0;
	viewer->origin[2] = 0;

	if (sv != oldserver)
	{
		if (sv)
		{
			snprintf(buffer, sizeof(buffer), "%cQTV%c%s leaves to watch %s (%i)\n", 91+128, 93+128, viewer->name, *sv->hostname?sv->hostname:sv->server, sv->streamid);
			QW_StreamPrint(cluster, oldserver, viewer, buffer);
		}
		snprintf(buffer, sizeof(buffer), "%cQTV%c%s joins the stream\n", 91+128, 93+128, viewer->name);
		QW_StreamPrint(cluster, sv, viewer, buffer);
	}
}

//fixme: will these want to have state?..
int NewChallenge(netadr_t *addr)
{
	return 4;
}
qboolean ChallengePasses(netadr_t *addr, int challenge)
{
	if (challenge == 4)
		return true;
	return false;
}

void NewClient(cluster_t *cluster, viewer_t *viewer)
{
	viewer->userid = ++cluster->nextuserid;
	viewer->timeout = cluster->curtime + 15*1000;
	viewer->trackplayer = -1;

	viewer->menunum = -1;
	QW_SetMenu(viewer, MENU_NONE);


	QW_PrintfToViewer(viewer, "Welcome to FTEQTV build %i\n", cluster->buildnumber);
	QW_StuffcmdToViewer(viewer, "alias admin \"cmd admin\"\n");

		QW_StuffcmdToViewer(viewer, "alias \"proxy:up\" \"say proxy:menu up\"\n");
		QW_StuffcmdToViewer(viewer, "alias \"proxy:down\" \"say proxy:menu down\"\n");
		QW_StuffcmdToViewer(viewer, "alias \"proxy:right\" \"say proxy:menu right\"\n");
		QW_StuffcmdToViewer(viewer, "alias \"proxy:left\" \"say proxy:menu left\"\n");

		QW_StuffcmdToViewer(viewer, "alias \"proxy:select\" \"say proxy:menu select\"\n");

		QW_StuffcmdToViewer(viewer, "alias \"proxy:home\" \"say proxy:menu home\"\n");
		QW_StuffcmdToViewer(viewer, "alias \"proxy:end\" \"say proxy:menu end\"\n");
		QW_StuffcmdToViewer(viewer, "alias \"proxy:menu\" \"say proxy:menu\"\n");
		QW_StuffcmdToViewer(viewer, "alias \"proxy:backspace\" \"say proxy:menu backspace\"\n");

		QW_StuffcmdToViewer(viewer, "alias \".help\" \"say .help\"\n");
		QW_StuffcmdToViewer(viewer, "alias \".disconnect\" \"say .disconnect\"\n");
		QW_StuffcmdToViewer(viewer, "alias \".menu\" \"say .menu\"\n");
		QW_StuffcmdToViewer(viewer, "alias \".admin\" \"say .admin\"\n");
		QW_StuffcmdToViewer(viewer, "alias \".reset\" \"say .reset\"\n");
		QW_StuffcmdToViewer(viewer, "alias \".clients\" \"say .clients\"\n");
//		QW_StuffcmdToViewer(viewer, "alias \".qtv\" \"say .qtv\"\n");
//		QW_StuffcmdToViewer(viewer, "alias \".join\" \"say .join\"\n");
//		QW_StuffcmdToViewer(viewer, "alias \".observe\" \"say .observe\"\n");

	QW_PrintfToViewer(viewer, "Type admin for the admin menu\n");
}

void ParseUserInfo(cluster_t *cluster, viewer_t *viewer)
{
	char buf[1024];
	float rate;
	char temp[64];
	Info_ValueForKey(viewer->userinfo, "name", temp, sizeof(temp));

	if (!*temp)
		strcpy(temp, "unnamed");
	if (!*viewer->name)
		Sys_Printf(cluster, "Viewer %s connected\n", temp);

	if (strcmp(viewer->name, temp))
	{
		if (*viewer->name)
		{
			snprintf(buf, sizeof(buf), "%cQTV%c%s changed name to %cQTV%c%s\n", 
					91+128, 93+128, viewer->name,
					91+128, 93+128, temp
					);
		}
		else
		{
			snprintf(buf, sizeof(buf), "%cQTV%c%s joins the stream\n", 
					91+128, 93+128, temp
					);

		}

		QW_StreamPrint(cluster, viewer->server, NULL, buf);
	}

	Q_strncpyz(viewer->name, temp, sizeof(viewer->name));

	Info_ValueForKey(viewer->userinfo, "rate", temp, sizeof(temp));
	rate = atof(temp);
	if (!rate)
		rate = 2500;
	if (rate < 250)
		rate = 250;
	if (rate > 10000)
		rate = 10000;
	viewer->netchan.rate = 1000.0f / rate;
}

void NewNQClient(cluster_t *cluster, netadr_t *addr)
{
	sv_t *initialserver;
	int header;
	int len;
	unsigned char buffer[64];
	viewer_t *viewer = NULL;;


	if (cluster->numviewers >= cluster->maxviewers && cluster->maxviewers)
	{
		buffer[4] = CCREP_REJECT;
		strcpy(buffer+5, "Sorry, proxy is full.\n");
		len = strlen(buffer+5)+5;
	}
/*	else
	{
		buffer[4] = CCREP_REJECT;
		strcpy(buffer+5, "NQ not supported yet\n");
		len = strlen(buffer+5)+5;
	}*/
	else if (!(viewer = malloc(sizeof(viewer_t))))
	{
		buffer[4] = CCREP_REJECT;
		strcpy(buffer+5, "Out of memory\n");
		len = strlen(buffer+5)+5;
	}
	else
	{
		buffer[4] = CCREP_ACCEPT;
		buffer[5] = (cluster->qwlistenportnum&0x00ff)>>0;
		buffer[6] = (cluster->qwlistenportnum&0xff00)>>8;
		buffer[7] = 0;
		buffer[8] = 0;
		len = 4+1+4;
	}

	*(int*)buffer = NETFLAG_CTL | len;
	header = (buffer[0]<<24) + (buffer[1]<<16) + (buffer[2]<<8) + buffer[3];
	*(int*)buffer = header;

	NET_SendPacket (cluster, cluster->qwdsocket, len, buffer, *addr);

	if (!viewer)
		return;


	memset(viewer, 0, sizeof(*viewer));


	Netchan_Setup (cluster->qwdsocket, &viewer->netchan, *addr, 0, false);
	viewer->netchan.isnqprotocol = true;

	viewer->next = cluster->viewers;
	cluster->viewers = viewer;
	viewer->delta_frame = -1;

	initialserver = NULL;
	if (cluster->numservers == 1)
	{
		initialserver = cluster->servers;
		if (!initialserver->modellist[1].name[0])
			initialserver = NULL;	//damn, that server isn't ready
	}

	viewer->server = initialserver;
	if (viewer->server)
		viewer->server->numviewers++;

	cluster->numviewers++;

	sprintf(viewer->userinfo, "\\name\\%s", "unnamed");

	ParseUserInfo(cluster, viewer);

	NewClient(cluster, viewer);

	QW_StuffcmdToViewer(viewer, "cmd new\n");

	Sys_Printf(cluster, "New NQ client connected\n");
}

void NewQWClient(cluster_t *cluster, netadr_t *addr, char *connectmessage)
{
	sv_t *initialserver;
	viewer_t *viewer;

	char qport[32];
	char challenge[32];
	char infostring[256];

	connectmessage+=11;

	connectmessage = COM_ParseToken(connectmessage, qport, sizeof(qport), "");
	connectmessage = COM_ParseToken(connectmessage, challenge, sizeof(challenge), "");
	connectmessage = COM_ParseToken(connectmessage, infostring, sizeof(infostring), "");

	if (!ChallengePasses(addr, atoi(challenge)))
	{
		Netchan_OutOfBandPrint(cluster, cluster->qwdsocket, *addr, "n" "Bad challenge");
		return;
	}


	viewer = malloc(sizeof(viewer_t));
	if (!viewer)
	{
		Netchan_OutOfBandPrint(cluster, cluster->qwdsocket, *addr, "n" "Out of memory");
		return;
	}
	memset(viewer, 0, sizeof(viewer_t));

	Netchan_Setup (cluster->qwdsocket, &viewer->netchan, *addr, atoi(qport), false);
	viewer->netchan.message.maxsize = 1400;

	viewer->next = cluster->viewers;
	cluster->viewers = viewer;
	viewer->delta_frame = -1;

	initialserver = NULL;
	if (cluster->numservers == 1)
	{
		initialserver = cluster->servers;
		if (!initialserver->modellist[1].name[0])
			initialserver = NULL;	//damn, that server isn't ready
	}

	viewer->server = initialserver;
	if (viewer->server)
		viewer->server->numviewers++;

	cluster->numviewers++;

	strncpy(viewer->userinfo, infostring, sizeof(viewer->userinfo)-1);
	ParseUserInfo(cluster, viewer);

	Netchan_OutOfBandPrint(cluster, cluster->qwdsocket, *addr, "j");

	NewClient(cluster, viewer);
}

void QW_SetMenu(viewer_t *v, int menunum)
{
	if ((v->menunum==MENU_NONE) != (menunum==MENU_NONE))
	{
		if (menunum != MENU_NONE)
		{
			QW_StuffcmdToViewer(v, "//set prox_inmenu 1\n");

			QW_StuffcmdToViewer(v, "alias \"+proxfwd\" \"proxy:up\"\n");
			QW_StuffcmdToViewer(v, "alias \"+proxback\" \"proxy:down\"\n");
			QW_StuffcmdToViewer(v, "alias \"+proxleft\" \"proxy:left\"\n");
			QW_StuffcmdToViewer(v, "alias \"+proxright\" \"proxy:right\"\n");

			QW_StuffcmdToViewer(v, "alias \"-proxfwd\" \" \"\n");
			QW_StuffcmdToViewer(v, "alias \"-proxback\" \" \"\n");
			QW_StuffcmdToViewer(v, "alias \"-proxleft\" \" \"\n");
			QW_StuffcmdToViewer(v, "alias \"-proxright\" \" \"\n");
		}
		else
		{
			QW_StuffcmdToViewer(v, "//set prox_inmenu 0\n");

			QW_StuffcmdToViewer(v, "alias \"+proxfwd\" \"+forward\"\n");
			QW_StuffcmdToViewer(v, "alias \"+proxback\" \"+back\"\n");
			QW_StuffcmdToViewer(v, "alias \"+proxleft\" \"+moveleft\"\n");
			QW_StuffcmdToViewer(v, "alias \"+proxright\" \"+moveright\"\n");

			QW_StuffcmdToViewer(v, "alias \"-proxfwd\" \"-forward\"\n");
			QW_StuffcmdToViewer(v, "alias \"-proxback\" \"-back\"\n");
			QW_StuffcmdToViewer(v, "alias \"-proxleft\" \"-moveleft\"\n");
			QW_StuffcmdToViewer(v, "alias \"-proxright\" \"-moveright\"\n");
		}
		QW_StuffcmdToViewer(v, "-forward\n");
		QW_StuffcmdToViewer(v, "-back\n");
		QW_StuffcmdToViewer(v, "-moveleft\n");
		QW_StuffcmdToViewer(v, "-moveright\n");
	}

	v->menunum = menunum;
	v->menuop = 0;
}

void QTV_Rcon(cluster_t *cluster, char *message, netadr_t *from)
{
	char buffer[8192];

	char *command;
	int passlen;

	if (!*cluster->adminpassword)
	{
		Netchan_OutOfBandPrint(cluster, cluster->qwdsocket, *from, "n" "Bad rcon_password.\n");
		return;
	}

	while(*message > '\0' && *message <= ' ')
		message++;

	command = strchr(message, ' ');
	passlen = command-message;
	if (passlen != strlen(cluster->adminpassword) || strncmp(message, cluster->adminpassword, passlen))
	{
		Netchan_OutOfBandPrint(cluster, cluster->qwdsocket, *from, "n" "Bad rcon_password.\n");
		return;
	}

	Netchan_OutOfBandPrint(cluster, cluster->qwdsocket, *from, "n%s", Rcon_Command(cluster, NULL, command, buffer, sizeof(buffer), false));
}

void QTV_Status(cluster_t *cluster, netadr_t *from)
{
	int i;
	char buffer[8192];
	sv_t *sv;

	netmsg_t msg;
	char elem[256];
	InitNetMsg(&msg, buffer, sizeof(buffer));
	WriteLong(&msg, -1);
	WriteByte(&msg, 'n');

	WriteString2(&msg, "\\*QTV\\");
	sprintf(elem, "%i", cluster->buildnumber);
	WriteString2(&msg, elem);

	if (cluster->numservers==1)
	{	//show this server's info
		sv = cluster->servers;

		WriteString2(&msg, sv->serverinfo);
		WriteString2(&msg, "\n");

		for (i = 0;i < MAX_CLIENTS; i++)
		{
			if (i == sv->thisplayer)
				continue;
			if (!sv->players[i].active)
				continue;
			//userid
			sprintf(elem, "%i", i);
			WriteString2(&msg, elem);
			WriteString2(&msg, " ");

			//frags
			sprintf(elem, "%i", sv->players[i].frags);
			WriteString2(&msg, elem);
			WriteString2(&msg, " ");

			//time (minuites)
			sprintf(elem, "%i", 0);
			WriteString2(&msg, elem);
			WriteString2(&msg, " ");

			//ping
			sprintf(elem, "%i", sv->players[i].ping);
			WriteString2(&msg, elem);
			WriteString2(&msg, " ");

			//name
			Info_ValueForKey(sv->players[i].userinfo, "name", elem, sizeof(elem));
			WriteString2(&msg, "\"");
			WriteString2(&msg, elem);
			WriteString2(&msg, "\" ");

			//skin
			Info_ValueForKey(sv->players[i].userinfo, "skin", elem, sizeof(elem));
			WriteString2(&msg, "\"");
			WriteString2(&msg, elem);
			WriteString2(&msg, "\" ");
			WriteString2(&msg, " ");

			//tc
			Info_ValueForKey(sv->players[i].userinfo, "topcolor", elem, sizeof(elem));
			WriteString2(&msg, elem);
			WriteString2(&msg, " ");

			//bc
			Info_ValueForKey(sv->players[i].userinfo, "bottomcolor", elem, sizeof(elem));
			WriteString2(&msg, elem);
			WriteString2(&msg, " ");

			WriteString2(&msg, "\n");
		}
	}
	else
	{
		WriteString2(&msg, "\\hostname\\");
		WriteString2(&msg, cluster->hostname);

		for (sv = cluster->servers, i = 0; sv; sv = sv->next, i++)
		{
			sprintf(elem, "\\%i\\", sv->streamid);
			WriteString2(&msg, elem);
			WriteString2(&msg, sv->serveraddress);
			sprintf(elem, " (%s)", sv->serveraddress);
			WriteString2(&msg, elem);
		}
	}

	WriteByte(&msg, 0);
	NET_SendPacket(cluster, cluster->qwdsocket, msg.cursize, msg.data, *from);
}


void ConnectionlessPacket(cluster_t *cluster, netadr_t *from, netmsg_t *m)
{
	char buffer[MAX_MSGLEN];
	int i;

	ReadLong(m);
	ReadString(m, buffer, sizeof(buffer));

	if (!strncmp(buffer, "rcon ", 5))
	{
		QTV_Rcon(cluster, buffer+5, from);
		return;
	}
	if (!strncmp(buffer, "ping", 4))
	{	//ack
		NET_SendPacket (cluster, cluster->qwdsocket, 1, "l", *from);
		return;
	}
	if (!strncmp(buffer, "status", 6))
	{
		QTV_Status(cluster, from);
		return;
	}
	if (!strncmp(buffer, "getchallenge", 12))
	{
		i = NewChallenge(from);
		Netchan_OutOfBandPrint(cluster, cluster->qwdsocket, *from, "c%i", i);
		return;
	}
	if (!strncmp(buffer, "connect 28 ", 11))
	{
		if (cluster->numviewers >= cluster->maxviewers && cluster->maxviewers)
			Netchan_OutOfBandPrint(cluster, cluster->qwdsocket, *from, "n" "Sorry, proxy is full.\n");
		else
			NewQWClient(cluster, from, buffer);
		return;
	}
	if (!strncmp(buffer, "l\n", 2))
	{
		Sys_Printf(cluster, "Ack\n");
	}
}


void SV_WriteDelta(int entnum, const entity_state_t *from, const entity_state_t *to, netmsg_t *msg, qboolean force)
{
	unsigned int i;
	unsigned int bits;

	bits = 0;
	if (from->angles[0] != to->angles[0])
		bits |= U_ANGLE1;
	if (from->angles[1] != to->angles[1])
		bits |= U_ANGLE2;
	if (from->angles[2] != to->angles[2])
		bits |= U_ANGLE3;

	if (from->origin[0] != to->origin[0])
		bits |= U_ORIGIN1;
	if (from->origin[1] != to->origin[1])
		bits |= U_ORIGIN2;
	if (from->origin[2] != to->origin[2])
		bits |= U_ORIGIN3;

	if (from->colormap != to->colormap)
		bits |= U_COLORMAP;
	if (from->skinnum != to->skinnum)
		bits |= U_SKIN;
	if (from->modelindex != to->modelindex)
		bits |= U_MODEL;
	if (from->frame != to->frame)
		bits |= U_FRAME;
	if (from->effects != to->effects)
		bits |= U_EFFECTS;

	if (bits & 255)
		bits |= U_MOREBITS;



	if (!bits && !force)
		return;

	i = (entnum&511) | (bits&~511);
	WriteShort (msg, i);

	if (bits & U_MOREBITS)
		WriteByte (msg, bits&255);
/*
#ifdef PROTOCOLEXTENSIONS
	if (bits & U_EVENMORE)
		WriteByte (msg, evenmorebits&255);
	if (evenmorebits & U_YETMORE)
		WriteByte (msg, (evenmorebits>>8)&255);
#endif
*/
	if (bits & U_MODEL)
		WriteByte (msg,	to->modelindex&255);
	if (bits & U_FRAME)
		WriteByte (msg, to->frame);
	if (bits & U_COLORMAP)
		WriteByte (msg, to->colormap);
	if (bits & U_SKIN)
		WriteByte (msg, to->skinnum);
	if (bits & U_EFFECTS)
		WriteByte (msg, to->effects&0x00ff);
	if (bits & U_ORIGIN1)
		WriteShort (msg, to->origin[0]);
	if (bits & U_ANGLE1)
		WriteByte(msg, to->angles[0]);
	if (bits & U_ORIGIN2)
		WriteShort (msg, to->origin[1]);
	if (bits & U_ANGLE2)
		WriteByte(msg, to->angles[1]);
	if (bits & U_ORIGIN3)
		WriteShort (msg, to->origin[2]);
	if (bits & U_ANGLE3)
		WriteByte(msg, to->angles[2]);
}

const entity_state_t nullentstate;
void SV_EmitPacketEntities (const sv_t *qtv, const viewer_t *v, const packet_entities_t *to, netmsg_t *msg)
{
	const entity_state_t *baseline;
	const packet_entities_t *from;
	int		oldindex, newindex;
	int		oldnum, newnum;
	int		oldmax;

	// this is the frame that we are going to delta update from
	if (v->delta_frame != -1)
	{
		from = &v->frame[v->delta_frame & (ENTITY_FRAMES-1)];
		oldmax = from->numents;

		WriteByte (msg, svc_deltapacketentities);
		WriteByte (msg, v->delta_frame);
	}
	else
	{
		oldmax = 0;	// no delta update
		from = NULL;

		WriteByte (msg, svc_packetentities);
	}

	newindex = 0;
	oldindex = 0;
//Con_Printf ("---%i to %i ----\n", client->delta_sequence & UPDATE_MASK
//			, client->netchan.outgoing_sequence & UPDATE_MASK);
	while (newindex < to->numents || oldindex < oldmax)
	{
		newnum = newindex >= to->numents ? 9999 : to->entnum[newindex];
		oldnum = oldindex >= oldmax ? 9999 : from->entnum[oldindex];

		if (newnum == oldnum)
		{	// delta update from old position
//Con_Printf ("delta %i\n", newnum);
			SV_WriteDelta (newnum, &from->ents[oldindex], &to->ents[newindex], msg, false);

			oldindex++;
			newindex++;
			continue;
		}

		if (newnum < oldnum)
		{	// this is a new entity, send it from the baseline
			baseline = &qtv->entity[newnum].baseline;
//Con_Printf ("baseline %i\n", newnum);
			SV_WriteDelta (newnum, baseline, &to->ents[newindex], msg, true);

			newindex++;
			continue;
		}

		if (newnum > oldnum)
		{	// the old entity isn't present in the new message
//Con_Printf ("remove %i\n", oldnum);
			WriteShort (msg, oldnum | U_REMOVE);
			oldindex++;
			continue;
		}
	}

	WriteShort (msg, 0);	// end of packetentities
}

void Prox_SendInitialEnts(sv_t *qtv, oproxy_t *prox, netmsg_t *msg)
{
	frame_t *frame;
	int i, entnum;
	WriteByte(msg, svc_packetentities);
	frame = &qtv->frame[qtv->netchan.incoming_sequence & (ENTITY_FRAMES-1)];
	for (i = 0; i < frame->numents; i++)
	{
		entnum = frame->entnums[i];
		SV_WriteDelta(i, &qtv->entity[entnum].baseline, &frame->ents[i], msg, true);
	}
	WriteShort(msg, 0);
}

static float InterpolateAngle(float current, float ideal, float fraction)
{
	float move;

	move = ideal - current;
	if (move >= 32767)
		move -= 65535;
	else if (move <= -32767)
		move += 65535;

	return current + fraction * move;
}

void SendLocalPlayerState(sv_t *tv, viewer_t *v, int playernum, netmsg_t *msg)
{
	int flags;
	int j;

	WriteByte(msg, svc_playerinfo);
	WriteByte(msg, playernum);

	if (tv && tv->controller == v)
	{
		v->trackplayer = tv->thisplayer;
		flags = 0;
		if (tv->players[tv->thisplayer].current.weaponframe)
			flags |= PF_WEAPONFRAME;
		if (tv->players[tv->thisplayer].current.effects)
			flags |= PF_EFFECTS;
		for (j=0 ; j<3 ; j++)
			if (tv->players[tv->thisplayer].current.velocity[j])
				flags |= (PF_VELOCITY1<<j);

		WriteShort(msg, flags);
		WriteShort(msg, tv->players[tv->thisplayer].current.origin[0]);
		WriteShort(msg, tv->players[tv->thisplayer].current.origin[1]);
		WriteShort(msg, tv->players[tv->thisplayer].current.origin[2]);
		WriteByte(msg, tv->players[tv->thisplayer].current.frame);

		for (j=0 ; j<3 ; j++)
			if (flags & (PF_VELOCITY1<<j) )
				WriteShort (msg, tv->players[tv->thisplayer].current.velocity[j]);

		if (flags & PF_MODEL)
			WriteByte(msg, tv->players[tv->thisplayer].current.modelindex);
		if (flags & PF_SKINNUM)
			WriteByte(msg, tv->players[tv->thisplayer].current.skinnum);
		if (flags & PF_EFFECTS)
			WriteByte(msg, tv->players[tv->thisplayer].current.effects);
		if (flags & PF_WEAPONFRAME)
			WriteByte(msg, tv->players[tv->thisplayer].current.weaponframe);
	}
	else
	{
		flags = 0;

		for (j=0 ; j<3 ; j++)
			if ((int)v->velocity[j])
				flags |= (PF_VELOCITY1<<j);

		WriteShort(msg, flags);
		WriteShort(msg, v->origin[0]*8);
		WriteShort(msg, v->origin[1]*8);
		WriteShort(msg, v->origin[2]*8);
		WriteByte(msg, 0);

		for (j=0 ; j<3 ; j++)
			if (flags & (PF_VELOCITY1<<j) )
				WriteShort (msg, v->velocity[j]);
	}
}

#define	UNQ_MOREBITS	(1<<0)
#define	UNQ_ORIGIN1	(1<<1)
#define	UNQ_ORIGIN2	(1<<2)
#define	UNQ_ORIGIN3	(1<<3)
#define	UNQ_ANGLE2	(1<<4)
#define	UNQ_NOLERP	(1<<5)		// don't interpolate movement
#define	UNQ_FRAME		(1<<6)
#define UNQ_SIGNAL	(1<<7)		// just differentiates from other updates

// svc_update can pass all of the fast update bits, plus more
#define	UNQ_ANGLE1	(1<<8)
#define	UNQ_ANGLE3	(1<<9)
#define	UNQ_MODEL		(1<<10)
#define	UNQ_COLORMAP	(1<<11)
#define	UNQ_SKIN		(1<<12)
#define	UNQ_EFFECTS	(1<<13)
#define	UNQ_LONGENTITY	(1<<14)
#define UNQ_UNUSED	(1<<15)


#define	SU_VIEWHEIGHT	(1<<0)
#define	SU_IDEALPITCH	(1<<1)
#define	SU_PUNCH1		(1<<2)
#define	SU_PUNCH2		(1<<3)
#define	SU_PUNCH3		(1<<4)
#define	SU_VELOCITY1	(1<<5)
#define	SU_VELOCITY2	(1<<6)
#define	SU_VELOCITY3	(1<<7)
//define	SU_AIMENT		(1<<8)  AVAILABLE BIT
#define	SU_ITEMS		(1<<9)
#define	SU_ONGROUND		(1<<10)		// no data follows, the bit is it
#define	SU_INWATER		(1<<11)		// no data follows, the bit is it
#define	SU_WEAPONFRAME	(1<<12)
#define	SU_ARMOR		(1<<13)
#define	SU_WEAPON		(1<<14)

void SendNQClientData(sv_t *tv, viewer_t *v, netmsg_t *msg)
{
	playerinfo_t *pl;
	int bits;
	int i;

	if (!tv)
		return;

	if (v->trackplayer < 0)
	{
		WriteByte (msg, svc_nqclientdata);
		WriteShort (msg, SU_VIEWHEIGHT|SU_ITEMS);
		WriteByte (msg, 22);
		WriteLong (msg, 0);
		WriteShort (msg, 1000);
		WriteByte (msg, 0);
		WriteByte (msg, 0);
		WriteByte (msg, 0);
		WriteByte (msg, 0);
		WriteByte (msg, 0);
		WriteByte (msg, 0);
		return;
	}
	else
		pl = &tv->players[v->trackplayer];

	bits = 0;
	
	if (!pl->dead)
		bits |= SU_VIEWHEIGHT;
		
	if (0)
		bits |= SU_IDEALPITCH;

	bits |= SU_ITEMS;
	
	if ( 0)
		bits |= SU_ONGROUND;
	
	if ( 0 )
		bits |= SU_INWATER;
	
	for (i=0 ; i<3 ; i++)
	{
		if (0)
			bits |= (SU_PUNCH1<<i);
		if (0)
			bits |= (SU_VELOCITY1<<i);
	}
	
	if (pl->current.weaponframe)
		bits |= SU_WEAPONFRAME;

	if (pl->stats[STAT_ARMOR])
		bits |= SU_ARMOR;

//	if (pl->stats[STAT_WEAPON])
		bits |= SU_WEAPON;

// send the data

	WriteByte (msg, svc_nqclientdata);
	WriteShort (msg, bits);

	if (bits & SU_VIEWHEIGHT)
		WriteByte (msg, 22);

	if (bits & SU_IDEALPITCH)
		WriteByte (msg, 0);

	for (i=0 ; i<3 ; i++)
	{
		if (bits & (SU_PUNCH1<<i))
			WriteByte (msg, 0);
		if (bits & (SU_VELOCITY1<<i))
			WriteByte (msg, 0);
	}

// [always sent]	if (bits & SU_ITEMS)
	WriteLong (msg, pl->stats[STAT_ITEMS]);

	if (bits & SU_WEAPONFRAME)
		WriteByte (msg, pl->current.weaponframe);
	if (bits & SU_ARMOR)
		WriteByte (msg, pl->stats[STAT_ARMOR]);
	if (bits & SU_WEAPON)
		WriteByte (msg, pl->stats[STAT_WEAPON]);
	
	WriteShort (msg, pl->stats[STAT_HEALTH]);
	WriteByte (msg, pl->stats[STAT_AMMO]);
	WriteByte (msg, pl->stats[STAT_SHELLS]);
	WriteByte (msg, pl->stats[STAT_NAILS]);
	WriteByte (msg, pl->stats[STAT_ROCKETS]);
	WriteByte (msg, pl->stats[STAT_CELLS]);

	WriteByte (msg, pl->stats[STAT_ACTIVEWEAPON]);
}

void SendNQPlayerStates(cluster_t *cluster, sv_t *tv, viewer_t *v, netmsg_t *msg)
{
	int e;
	int i;
	usercmd_t to;
	float lerp;
	int bits;
	unsigned short org[3];
	entity_t *ent;
	playerinfo_t *pl;


	memset(&to, 0, sizeof(to));

	if (tv)
	{
		WriteByte(msg, svc_nqtime);
		WriteFloat(msg, tv->physicstime/1000.0f);

		BSP_SetupForPosition(tv->bsp, v->origin[0], v->origin[1], v->origin[2]);

		lerp = ((tv->simtime - tv->oldpackettime)/1000.0f) / ((tv->nextpackettime - tv->oldpackettime)/1000.0f);
		if (lerp < 0)
			lerp = 0;
		if (lerp > 1)
			lerp = 1;

		if (tv->controller == v)
			lerp = 1;
	}
	else
	{
		WriteByte(msg, svc_nqtime);
		WriteFloat(msg, cluster->curtime/1000.0f);

		lerp = 1;
	}

	SendNQClientData(tv, v, msg);

	if (tv)
	{
		
		if (v->trackplayer >= 0)
		{
			WriteByte(msg, svc_nqsetview);
			WriteShort(msg, v->trackplayer+1);

			WriteByte(msg, svc_setangle);
			WriteByte(msg, (int)InterpolateAngle(tv->players[v->trackplayer].old.angles[0], tv->players[v->trackplayer].current.angles[0], lerp)>>8);
			WriteByte(msg, (int)InterpolateAngle(tv->players[v->trackplayer].old.angles[1], tv->players[v->trackplayer].current.angles[1], lerp)>>8);
			WriteByte(msg, (int)InterpolateAngle(tv->players[v->trackplayer].old.angles[2], tv->players[v->trackplayer].current.angles[2], lerp)>>8);
		}
		else
		{
			WriteByte(msg, svc_nqsetview);
			WriteShort(msg, v->thisplayer+1);
		}


		for (e = 0; e < MAX_CLIENTS; e++)
		{
			pl = &tv->players[e];
			ent = &tv->entity[e+1];

			if (e == v->thisplayer && v->trackplayer < 0)
			{
				bits = UNQ_ORIGIN1 | UNQ_ORIGIN2 | UNQ_ORIGIN3 | UNQ_COLORMAP;


  				if (e+1 >= 256)
					bits |= UNQ_LONGENTITY;
					
				if (bits >= 256)
					bits |= UNQ_MOREBITS;
				WriteByte (msg,bits | UNQ_SIGNAL);
				if (bits & UNQ_MOREBITS)
					WriteByte (msg, bits>>8);
				if (bits & UNQ_LONGENTITY)
					WriteShort (msg,e+1);
				else
					WriteByte (msg,e+1);

				if (bits & UNQ_MODEL)
					WriteByte (msg,	0);
				if (bits & UNQ_FRAME)
					WriteByte (msg, 0);
				if (bits & UNQ_COLORMAP)
					WriteByte (msg, 0);
				if (bits & UNQ_SKIN)
					WriteByte (msg, 0);
				if (bits & UNQ_EFFECTS)
					WriteByte (msg, 0);
				if (bits & UNQ_ORIGIN1)
					WriteShort (msg, v->origin[0]);		
				if (bits & UNQ_ANGLE1)
					WriteByte(msg, -(v->ucmds[2].angles[0]>>8));
				if (bits & UNQ_ORIGIN2)
					WriteShort (msg, v->origin[1]);
				if (bits & UNQ_ANGLE2)
					WriteByte(msg, v->ucmds[2].angles[1]>>8);
				if (bits & UNQ_ORIGIN3)
					WriteShort (msg, v->origin[2]);
				if (bits & UNQ_ANGLE3)
					WriteByte(msg, v->ucmds[2].angles[2]>>8);
				continue;
			}

			if (!pl->active)
				continue;

			if (pl->current.modelindex >= tv->numinlines && !BSP_Visible(tv->bsp, pl->leafcount, pl->leafs))
				continue;

			pl->current.modelindex = 8;
			
// send an update
			bits = 0;
			
			for (i=0 ; i<3 ; i++)
			{
				org[i] = (lerp)*pl->current.origin[i] + (1-lerp)*pl->old.origin[i];
				bits |= UNQ_ORIGIN1<<i;
			}

			if ( pl->current.angles[0]>>8 != ent->baseline.angles[0] )
				bits |= UNQ_ANGLE1;
				
			if ( pl->current.angles[1]>>8 != ent->baseline.angles[1] )
				bits |= UNQ_ANGLE2;
				
			if ( pl->current.angles[2]>>8 != ent->baseline.angles[2] )
				bits |= UNQ_ANGLE3;
				
//			if (pl->v.movetype == MOVETYPE_STEP)
//				bits |= UNQ_NOLERP;	// don't mess up the step animation
		
			if (ent->baseline.colormap != e+1 || ent->baseline.colormap > 15)
				bits |= UNQ_COLORMAP;
				
			if (ent->baseline.skinnum != pl->current.skinnum)
				bits |= UNQ_SKIN;
				
			if (ent->baseline.frame != pl->current.frame)
				bits |= UNQ_FRAME;
			
			if (ent->baseline.effects != pl->current.effects)
				bits |= UNQ_EFFECTS;
			
			if (ent->baseline.modelindex != pl->current.modelindex)
				bits |= UNQ_MODEL;

			if (e+1 >= 256)
				bits |= UNQ_LONGENTITY;
				
			if (bits >= 256)
				bits |= UNQ_MOREBITS;

		//
		// write the message
		//
			WriteByte (msg,bits | UNQ_SIGNAL);
			
			if (bits & UNQ_MOREBITS)
				WriteByte (msg, bits>>8);
			if (bits & UNQ_LONGENTITY)
				WriteShort (msg,e+1);
			else
				WriteByte (msg,e+1);

			if (bits & UNQ_MODEL)
				WriteByte (msg,	pl->current.modelindex);
			if (bits & UNQ_FRAME)
				WriteByte (msg, pl->current.frame);
			if (bits & UNQ_COLORMAP)
				WriteByte (msg, (e>=15)?0:(e+1));
			if (bits & UNQ_SKIN)
				WriteByte (msg, pl->current.skinnum);
			if (bits & UNQ_EFFECTS)
				WriteByte (msg, pl->current.effects);
			if (bits & UNQ_ORIGIN1)
				WriteShort (msg, org[0]);		
			if (bits & UNQ_ANGLE1)
				WriteByte(msg, -(pl->current.angles[0]>>8));
			if (bits & UNQ_ORIGIN2)
				WriteShort (msg, org[1]);
			if (bits & UNQ_ANGLE2)
				WriteByte(msg, pl->current.angles[1]>>8);
			if (bits & UNQ_ORIGIN3)
				WriteShort (msg, org[2]);
			if (bits & UNQ_ANGLE3)
				WriteByte(msg, pl->current.angles[2]>>8);
		}
/*		for (e = 0; e < tv->maxents; e++)
		{
			ent = &tv->entity[e];
			if (!ent->current.modelindex)
				continue;

			if (ent->current.modelindex >= tv->numinlines && !BSP_Visible(tv->bsp, ent->leafcount, ent->leafs))
				continue;

			if (msg->cursize + 128 > msg->maxsize)
				break;
			
// send an update
			bits = 0;

			if (ent->updatetime == tv->oldpackettime)
			{
				for (i=0 ; i<3 ; i++)
				{
					org[i] = (lerp)*ent->current.origin[i] + (1-lerp)*ent->old.origin[i];
					miss = org[i] - ent->baseline.origin[i];
				//	if ( miss < -1 || miss > 1 )
						bits |= UNQ_ORIGIN1<<i;
				}
			}
			else
			{
				for (i=0 ; i<3 ; i++)
				{
					org[i] = ent->current.origin[i];
					miss = org[i] - ent->baseline.origin[i];
				//	if ( miss < -1 || miss > 1 )
						bits |= UNQ_ORIGIN1<<i;
				}
			}

			if ( ent->current.angles[0] != ent->baseline.angles[0] )
				bits |= UNQ_ANGLE1;
				
			if ( ent->current.angles[1] != ent->baseline.angles[1] )
				bits |= UNQ_ANGLE2;
				
			if ( ent->current.angles[2] != ent->baseline.angles[2] )
				bits |= UNQ_ANGLE3;
				
//			if (ent->v.movetype == MOVETYPE_STEP)
//				bits |= UNQ_NOLERP;	// don't mess up the step animation
		
			if (ent->baseline.colormap != ent->current.colormap || ent->baseline.colormap > 15)
				bits |= UNQ_COLORMAP;
				
			if (ent->baseline.skinnum != ent->current.skinnum)
				bits |= UNQ_SKIN;
				
			if (ent->baseline.frame != ent->current.frame)
				bits |= UNQ_FRAME;
			
			if (ent->baseline.effects != ent->current.effects)
				bits |= UNQ_EFFECTS;
			
			if (ent->baseline.modelindex != ent->current.modelindex)
				bits |= UNQ_MODEL;

			if (e >= 256)
				bits |= UNQ_LONGENTITY;
				
			if (bits >= 256)
				bits |= UNQ_MOREBITS;

		//
		// write the message
		//
			WriteByte (msg,bits | UNQ_SIGNAL);
			
			if (bits & UNQ_MOREBITS)
				WriteByte (msg, bits>>8);
			if (bits & UNQ_LONGENTITY)
				WriteShort (msg,e);
			else
				WriteByte (msg,e);

			if (bits & UNQ_MODEL)
				WriteByte (msg,	ent->current.modelindex);
			if (bits & UNQ_FRAME)
				WriteByte (msg, ent->current.frame);
			if (bits & UNQ_COLORMAP)
				WriteByte (msg, (ent->current.colormap>15)?0:(ent->current.colormap));
			if (bits & UNQ_SKIN)
				WriteByte (msg, ent->current.skinnum);
			if (bits & UNQ_EFFECTS)
				WriteByte (msg, ent->current.effects);
			if (bits & UNQ_ORIGIN1)
				WriteShort (msg, org[0]);		
			if (bits & UNQ_ANGLE1)
				WriteByte(msg, ent->current.angles[0]);
			if (bits & UNQ_ORIGIN2)
				WriteShort (msg, org[1]);
			if (bits & UNQ_ANGLE2)
				WriteByte(msg, ent->current.angles[1]);
			if (bits & UNQ_ORIGIN3)
				WriteShort (msg, org[2]);
			if (bits & UNQ_ANGLE3)
				WriteByte(msg, ent->current.angles[2]);
		}
*/
	}
	else
	{
		WriteShort (msg,UNQ_MOREBITS|UNQ_MODEL|UNQ_ORIGIN1 | UNQ_ORIGIN2 | UNQ_ORIGIN3 | UNQ_SIGNAL);
		WriteByte (msg, 1);
		WriteByte (msg, 2);	//model
		WriteShort (msg, v->origin[0]);
		WriteShort (msg, v->origin[1]);
		WriteShort (msg, v->origin[2]);
	}
}

void SendPlayerStates(sv_t *tv, viewer_t *v, netmsg_t *msg)
{
	viewer_t *cv;
	packet_entities_t *e;
	int i;
	usercmd_t to;
	unsigned short flags;
	short interp;
	float lerp;
	int track;
	int runaway = 10;

	int snapdist = 128;	//in quake units

	snapdist = snapdist*8;
	snapdist = snapdist*snapdist;


	memset(&to, 0, sizeof(to));

	if (tv)
	{
		if (v->trackplayer >= 0 && !v->backbuffered)
		{
			if (v->trackplayer != tv->trackplayer && tv->usequkeworldprotocols)
				if (!tv->players[v->trackplayer].active && tv->players[tv->trackplayer].active)
				{
					QW_StuffcmdToViewer (v, "track %i\n", tv->trackplayer);
				}
		}
		if (tv->physicstime != v->settime && tv->cluster->chokeonnotupdated)
		{
			WriteByte(msg, svc_updatestatlong);
			WriteByte(msg, STAT_TIME);
			WriteLong(msg, v->settime);

			v->settime = tv->physicstime;
		}

		BSP_SetupForPosition(tv->bsp, v->origin[0], v->origin[1], v->origin[2]);

		lerp = ((tv->simtime - tv->oldpackettime)/1000.0f) / ((tv->nextpackettime - tv->oldpackettime)/1000.0f);
		if (lerp < 0)
			lerp = 0;
		if (lerp > 1)
			lerp = 1;

		if (tv->controller == v)
			lerp = 1;

		track = v->trackplayer;
		for (cv = v; cv && runaway-->0; cv = cv->commentator)
		{
			track = cv->trackplayer;
			if (track != MAX_CLIENTS-2)
				break;
		}
		/*
		if (v->commentator && track == MAX_CLIENTS-2)
		{
			track = v->commentator->trackplayer;
			if (track < 0)
				track = MAX_CLIENTS-2;
		}*/

		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (i == v->thisplayer)
			{
				SendLocalPlayerState(tv, v, i, msg);
				continue;
			}

			if (v->commentator && v->thinksitsconnected)// && track == i)
			{
				if (i == MAX_CLIENTS-2)
				{
					flags = PF_COMMAND;
					WriteByte(msg, svc_playerinfo);
					WriteByte(msg, i);
					WriteShort(msg, flags);


					interp = v->commentator->origin[0]*8;
					WriteShort(msg, interp);
					interp = v->commentator->origin[1]*8;
					WriteShort(msg, interp);
					interp = v->commentator->origin[2]*8;
					WriteShort(msg, interp);

					WriteByte(msg, 0);

					if (flags & PF_MSEC)
					{
						WriteByte(msg, 0);
					}
					if (flags & PF_COMMAND)
					{
						to.angles[0] = v->commentator->ucmds[2].angles[0];
						to.angles[1] = v->commentator->ucmds[2].angles[1];
						to.angles[2] = v->commentator->ucmds[2].angles[2];
						WriteDeltaUsercmd(msg, &nullcmd, &to);
					}
					if (flags & PF_MODEL)
						WriteByte(msg, tv->players[i].current.modelindex);
					if (flags & PF_WEAPONFRAME)
						WriteByte(msg, tv->players[i].current.weaponframe);
					continue;
				}
				if (track == i)
					continue;
			}

			if (!tv->players[i].active)
				continue;

			//bsp cull. currently tracked player is always visible
			if (track != i && !BSP_Visible(tv->bsp, tv->players[i].leafcount, tv->players[i].leafs))
				continue;

			flags = PF_COMMAND;
			if (track == i && tv->players[i].current.weaponframe)
				flags |= PF_WEAPONFRAME;

			WriteByte(msg, svc_playerinfo);
			WriteByte(msg, i);
			WriteShort(msg, flags);

			if ((tv->players[i].current.origin[0] - tv->players[i].old.origin[0])*(tv->players[i].current.origin[0] - tv->players[i].old.origin[0]) > snapdist ||
				(tv->players[i].current.origin[1] - tv->players[i].old.origin[1])*(tv->players[i].current.origin[1] - tv->players[i].old.origin[1]) > snapdist ||
				(tv->players[i].current.origin[2] - tv->players[i].old.origin[2])*(tv->players[i].current.origin[2] - tv->players[i].old.origin[2]) > snapdist)
			{	//teleported (or respawned), so don't interpolate
				WriteShort(msg, tv->players[i].current.origin[0]);
				WriteShort(msg, tv->players[i].current.origin[1]);
				WriteShort(msg, tv->players[i].current.origin[2]);
			}
			else
			{	//send interpolated angles
				interp = (lerp)*tv->players[i].current.origin[0] + (1-lerp)*tv->players[i].old.origin[0];
				WriteShort(msg, interp);
				interp = (lerp)*tv->players[i].current.origin[1] + (1-lerp)*tv->players[i].old.origin[1];
				WriteShort(msg, interp);
				interp = (lerp)*tv->players[i].current.origin[2] + (1-lerp)*tv->players[i].old.origin[2];
				WriteShort(msg, interp);
			}

			WriteByte(msg, tv->players[i].current.frame);

			if (flags & PF_MSEC)
			{
				WriteByte(msg, 0);
			}
			if (flags & PF_COMMAND)
			{
	//			to.angles[0] = tv->players[i].current.angles[0];
	//			to.angles[1] = tv->players[i].current.angles[1];
	//			to.angles[2] = tv->players[i].current.angles[2];

				to.angles[0] = InterpolateAngle(tv->players[i].old.angles[0], tv->players[i].current.angles[0], lerp);
				to.angles[1] = InterpolateAngle(tv->players[i].old.angles[1], tv->players[i].current.angles[1], lerp);
				to.angles[2] = InterpolateAngle(tv->players[i].old.angles[2], tv->players[i].current.angles[2], lerp);
				WriteDeltaUsercmd(msg, &nullcmd, &to);
			}
			//vel
			//model
			if (flags & PF_MODEL)
				WriteByte(msg, tv->players[i].current.modelindex);
			//skin
			//effects
			//weaponframe
			if (flags & PF_WEAPONFRAME)
				WriteByte(msg, tv->players[i].current.weaponframe);
		}
	}
	else
	{
		lerp = 1;

		SendLocalPlayerState(tv, v, v->thisplayer, msg);
	}


	e = &v->frame[v->netchan.outgoing_sequence&(ENTITY_FRAMES-1)];
	e->numents = 0;
	if (tv)
	{
		int oldindex = 0, newindex = 0;
		entity_state_t *newstate;
		int newnum, oldnum;
		frame_t *frompacket, *topacket;
		topacket = &tv->frame[tv->netchan.incoming_sequence&(ENTITY_FRAMES-1)];
		if (tv->usequkeworldprotocols)
		{
			//qw protocols don't interpolate... yet
			frompacket = topacket;
		}
		else
		{
			frompacket = &tv->frame[(tv->netchan.incoming_sequence-1)&(ENTITY_FRAMES-1)];
		}

		for (newindex = 0; newindex < topacket->numents; newindex++)
		{
			//don't pvs cull bsp models
			//pvs cull everything else
			newstate = &topacket->ents[newindex];
			newnum = topacket->entnums[newindex];
			if (newstate->modelindex >= tv->numinlines && !BSP_Visible(tv->bsp, tv->entity[newnum].leafcount, tv->entity[newnum].leafs))
				continue;

			e->entnum[e->numents] = newnum;
			memcpy(&e->ents[e->numents], newstate, sizeof(entity_state_t));

			if (frompacket != topacket)	//optimisation for qw protocols
			{
				entity_state_t *oldstate;

				if (oldindex < frompacket->numents)
				{
					oldnum = frompacket->entnums[oldindex];
				
					while(oldnum < newnum)
					{
						oldindex++;
						if (oldindex >= frompacket->numents)
							break;	//no more
						oldnum = frompacket->entnums[oldindex];
					}
					if (oldnum == newnum)
					{
						//ent exists in old packet
						oldstate = &frompacket->ents[oldindex];
					}
					else
					{
						oldstate = newstate;
					}
				}
				else
				{	//reached end, definatly not in packet
					oldstate = newstate;
				}


				if ((newstate->origin[0] - oldstate->origin[0])*(newstate->origin[0] - oldstate->origin[0]) > snapdist ||
					(newstate->origin[1] - oldstate->origin[1])*(newstate->origin[1] - oldstate->origin[1]) > snapdist ||
					(newstate->origin[2] - oldstate->origin[2])*(newstate->origin[2] - oldstate->origin[2]) > snapdist)
				{	//teleported (or respawned), so don't interpolate
					e->ents[e->numents].origin[0] = newstate->origin[0];
					e->ents[e->numents].origin[1] = newstate->origin[1];
					e->ents[e->numents].origin[2] = newstate->origin[2];
				}
				else
				{
					e->ents[e->numents].origin[0] = (lerp)*newstate->origin[0] + (1-lerp)*oldstate->origin[0];
					e->ents[e->numents].origin[1] = (lerp)*newstate->origin[1] + (1-lerp)*oldstate->origin[1];
					e->ents[e->numents].origin[2] = (lerp)*newstate->origin[2] + (1-lerp)*oldstate->origin[2];
				}
			}

			e->numents++;

			if (e->numents == ENTS_PER_FRAME)
				break;
		}
	}

	SV_EmitPacketEntities(tv, v, e, msg);

	if (tv && tv->nailcount)
	{
		WriteByte(msg, svc_nails);
		WriteByte(msg, tv->nailcount);
		for (i = 0; i < tv->nailcount; i++)
		{
			WriteByte(msg, tv->nails[i].bits[0]);
			WriteByte(msg, tv->nails[i].bits[1]);
			WriteByte(msg, tv->nails[i].bits[2]);
			WriteByte(msg, tv->nails[i].bits[3]);
			WriteByte(msg, tv->nails[i].bits[4]);
			WriteByte(msg, tv->nails[i].bits[5]);
		}
	}
}

void UpdateStats(sv_t *qtv, viewer_t *v)
{
	viewer_t *cv;
	netmsg_t msg;
	char buf[6];
	int i;
	const static unsigned int nullstats[MAX_STATS] = {1000};

	const unsigned int *stats;

	InitNetMsg(&msg, buf, sizeof(buf));

	if (v->commentator && v->thinksitsconnected)
		cv = v->commentator;
	else
		cv = v;

	if (qtv && qtv->controller == cv)
		stats = qtv->players[qtv->thisplayer].stats;
	else if (cv->trackplayer == -1 || !qtv)
		stats = nullstats;
	else
		stats = qtv->players[cv->trackplayer].stats;

	for (i = 0; i < MAX_STATS; i++)
	{
		if (v->currentstats[i] != stats[i])
		{
			if (v->netchan.isnqprotocol)
			{	//nq only supports 32bit stats
				WriteByte(&msg, svc_updatestat);
				WriteByte(&msg, i);
				WriteLong(&msg, stats[i]);
			}
			else if (stats[i] < 256)
			{
				WriteByte(&msg, svc_updatestat);
				WriteByte(&msg, i);
				WriteByte(&msg, stats[i]);
			}
			else
			{
				WriteByte(&msg, svc_updatestatlong);
				WriteByte(&msg, i);
				WriteLong(&msg, stats[i]);
			}
			SendBufferToViewer(v, msg.data, msg.cursize, true);
			msg.cursize = 0;
			v->currentstats[i] = stats[i];
		}
	}
}

//returns the next prespawn 'buffer' number to use, or -1 if no more
int Prespawn(sv_t *qtv, int curmsgsize, netmsg_t *msg, int bufnum, int thisplayer)
{
	int r, ni;
	r = bufnum;

	ni = SendCurrentUserinfos(qtv, curmsgsize, msg, bufnum, thisplayer);
	r += ni - bufnum;
	bufnum = ni;
	bufnum -= MAX_CLIENTS;

	ni = SendCurrentBaselines(qtv, curmsgsize, msg, 768, bufnum);
	r += ni - bufnum;
	bufnum = ni;
	bufnum -= MAX_ENTITIES;

	ni = SendCurrentLightmaps(qtv, curmsgsize, msg, 768, bufnum);
	r += ni - bufnum;
	bufnum = ni;
	bufnum -= MAX_LIGHTSTYLES;

	ni = SendStaticSounds(qtv, curmsgsize, msg, 768, bufnum);
	r += ni - bufnum;
	bufnum = ni;
	bufnum -= MAX_STATICSOUNDS;

	ni = SendStaticEntities(qtv, curmsgsize, msg, 768, bufnum);
	r += ni - bufnum;
	bufnum = ni;
	bufnum -= MAX_STATICENTITIES;

	if (bufnum == 0)
		return -1;

	return r;
}

void PMove(viewer_t *v, usercmd_t *cmd)
{
	sv_t *qtv;
	pmove_t pmove;
	if (v->server && v->server->controller == v)
	{
		v->origin[0] = v->server->players[v->server->thisplayer].current.origin[0]/8.0f;
		v->origin[1] = v->server->players[v->server->thisplayer].current.origin[1]/8.0f;
		v->origin[2] = v->server->players[v->server->thisplayer].current.origin[2]/8.0f;

		v->velocity[0] = v->server->players[v->server->thisplayer].current.velocity[2]/8.0f;
		v->velocity[1] = v->server->players[v->server->thisplayer].current.velocity[2]/8.0f;
		v->velocity[2] = v->server->players[v->server->thisplayer].current.velocity[2]/8.0f;
		return;
	}
	pmove.origin[0] = v->origin[0];
	pmove.origin[1] = v->origin[1];
	pmove.origin[2] = v->origin[2];

	pmove.velocity[0] = v->velocity[0];
	pmove.velocity[1] = v->velocity[1];
	pmove.velocity[2] = v->velocity[2];

	pmove.cmd = *cmd;
	qtv = v->server;
	if (qtv)
	{
		pmove.movevars = qtv->movevars;
	}
	else
	{
		QTV_DefaultMovevars(&pmove.movevars);
	}
	PM_PlayerMove(&pmove);

	v->origin[0] = pmove.origin[0];
	v->origin[1] = pmove.origin[1];
	v->origin[2] = pmove.origin[2];

	v->velocity[0] = pmove.velocity[0];
	v->velocity[1] = pmove.velocity[1];
	v->velocity[2] = pmove.velocity[2];
}

void QW_SetCommentator(cluster_t *cluster, viewer_t *v, viewer_t *commentator)
{
//	if (v->commentator == commentator)
//		return;

	WriteByte(&v->netchan.message, svc_setinfo);
	WriteByte(&v->netchan.message, MAX_CLIENTS-2);
	WriteString(&v->netchan.message, "name");
	if (commentator)
	{
		WriteString(&v->netchan.message, commentator->name);
		QW_StuffcmdToViewer(v, "track %i\n", MAX_CLIENTS-2);
		QW_PrintfToViewer(v, "Following commentator %s\n", commentator->name);

		if (v->server != commentator->server)
			QW_SetViewersServer(cluster, v, commentator->server);
	}
	else
	{
		WriteString(&v->netchan.message, "");
		if (v->commentator )
			QW_PrintfToViewer(v, "Commentator disabled\n");
	}
	v->commentator = commentator;
}

void QTV_Say(cluster_t *cluster, sv_t *qtv, viewer_t *v, char *message, qboolean noupwards)
{
	char buf[1024];
	netmsg_t msg;

	if (message[strlen(message)-1] == '\"')
		message[strlen(message)-1] = '\0';

	if (*v->expectcommand)
	{
		buf[sizeof(buf)-1] = '\0';
		if (!strcmp(v->expectcommand, "hostname"))
		{
			strncpy(cluster->hostname, message, sizeof(cluster->hostname));
			cluster->hostname[sizeof(cluster->hostname)-1] = '\0';
		}
		else if (!strcmp(v->expectcommand, "master"))
		{
			strncpy(cluster->master, message, sizeof(cluster->master));
			cluster->master[sizeof(cluster->master)-1] = '\0';
			if (!strcmp(cluster->master, "."))
				*cluster->master = '\0';
			cluster->mastersendtime = cluster->curtime;
		}
		else if (!strcmp(v->expectcommand, "addserver"))
		{
			snprintf(buf, sizeof(buf), "tcp:%s", message);
			qtv = QTV_NewServerConnection(cluster, buf, "", false, false, false);
			if (qtv)
			{
				QW_SetViewersServer(cluster, v, qtv);
				QW_PrintfToViewer(v, "Connected\n", message);
			}
			else
				QW_PrintfToViewer(v, "Failed to connect to server \"%s\", connection aborted\n", message);
		}
		else if (!strcmp(v->expectcommand, "admin"))
		{
			if (!strcmp(message, cluster->adminpassword))
			{
				QW_SetMenu(v, MENU_ADMIN);
				v->isadmin = true;
				Sys_Printf(cluster, "Player %s logs in as admin\n", v->name);
			}
			else
			{
				QW_PrintfToViewer(v, "Admin password incorrect\n");
				Sys_Printf(cluster, "Player %s gets incorrect admin password\n", v->name);
			}
		}
		else if (!strcmp(v->expectcommand, "insecadddemo"))
		{
			snprintf(buf, sizeof(buf), "file:%s", message);
			qtv = QTV_NewServerConnection(cluster, buf, "", false, false, false);
			if (!qtv)
				QW_PrintfToViewer(v, "Failed to play demo \"%s\"\n", message);
			else
			{
				QW_SetViewersServer(cluster, v, qtv);
				QW_PrintfToViewer(v, "Opened demo file.\n", message);
			}
		}
		
		else if (!strcmp(v->expectcommand, "adddemo"))
		{
			snprintf(buf, sizeof(buf), "file:%s", message);
			qtv = QTV_NewServerConnection(cluster, buf, "", false, false, false);
			if (!qtv)
				QW_PrintfToViewer(v, "Failed to play demo \"%s\"\n", message);
			else
			{
				QW_SetViewersServer(cluster, v, qtv);
				QW_PrintfToViewer(v, "Opened demo file.\n", message);
			}
		}
		else if (!strcmp(v->expectcommand, "setmvdport"))
		{
			int newp;
			int news;
			if (qtv)
			{
				newp = atoi(message);

				if (newp)
				{
					news = Net_MVDListen(newp);

					if (news != INVALID_SOCKET)
					{
						if (qtv->tcpsocket != INVALID_SOCKET)
							closesocket(qtv->tcpsocket);
						qtv->tcpsocket = news;
						qtv->tcplistenportnum = newp;
						qtv->disconnectwhennooneiswatching = false;
					}
				}
				else if (qtv->tcpsocket != INVALID_SOCKET)
				{
					closesocket(qtv->tcpsocket);
					qtv->tcpsocket = INVALID_SOCKET;
				}
			}
			else
				QW_PrintfToViewer(v, "You were disconnected from that stream\n");
		}
		else
		{
			QW_PrintfToViewer(v, "Command %s was not recognised\n", v->expectcommand);
		}

		*v->expectcommand = '\0';
		return;
	}
	if (!strncmp(message, ".help", 5))
	{
		QW_PrintfToViewer(v,	"Website: http://www.fteqw.com/\n"
								"Commands:\n"
								".observe qwserver:port\n"
								".qtv tcpserver:port\n"
								".demo gamedir/demoname.mvd\n"
								".disconnect\n"
								".admin\n"
								".bind\n"
								);
	}

	else if (!strncmp(message, ".menu", 5))
	{
		message += 5;

		if (v->conmenussupported)
			goto guimenu;
		else
			goto tuimenu;
	}

	else if (!strncmp(message, ".tuimenu", 8))
	{
		message += 8;

tuimenu:
		if (v->menunum)
			QW_SetMenu(v, MENU_NONE);
		else
			QW_SetMenu(v, MENU_SERVERS);
	}
	else if (!strncmp(message, ".guimenu", 8))
	{
		sv_t *sv;
		int y;
		qboolean shownheader;

		message += 8;

guimenu:

		QW_SetMenu(v, MENU_NONE);

		shownheader = false;

		QW_StuffcmdToViewer(v, 

			"alias menucallback\n"
			"{\n"
				"menuclear\n"
				"if (option == \"OBSERVE\")\n"
					"{\necho Spectating server $_server\nsay .observe $_server\n}\n"
				"if (option == \"QTV\")\n"
					"{\necho Streaming from qtv at $_server\nsay .qtv $_server\n}\n"
				"if (option == \"JOIN\")\n"
					"{\necho Joining game at $_server\nsay .join $_server\n}\n"
				"if (option == \"ADMIN\")\n"
					"{\nsay .guiadmin\n}\n"
				"if (\"stream \" isin option)\n"
					"{\necho Changing stream\nsay .$option\n}\n"
			"}\n"
/*
			"conmenu menucallback\n"
			"menupic 16 4 gfx/qplaque.lmp\n"
			"menupic - 4 gfx/p_option.lmp\n"

			"menuedit 16 32 \"        Server\" \"_server\"\n"

			"menutext 72 48 \"Observe\" OBSERVE\n"
			"menutext 136 48 \"QTV\" QTV\n"
			"menutext 168 48 \"Cancel\" cancel\n"
			"menutext 224 48 \"Join\" JOIN\n"
			"menutext 264 48 \"Admin\" ADMIN\n"
*/
			"conmenu menucallback\n"
			"menupic 0 4 gfx/qplaque.lmp\n"
			"menupic 96 4 gfx/p_option.lmp\n"

			"menuedit 48 36 \"������\" \"_server\"\n"

			"menutext 104 52 \"Join\" JOIN\n"

			"menutext 152 52 \"Observe\" OBSERVE\n"

			"menutext 224 52 \"QTV\" QTV\n"



			"menutext 48 84 \"Admin\" ADMIN\n"

			"menutext 48 92 \"Close Menu\" cancel\n"



			"menutext 48 116 \"Type in a server address and\"\n"
			"menutext 48 124 \"click join to play in the game,\"\n"
			"menutext 48 132 \"observe(udp) to watch, or qtv(tcp)\"\n"
			"menutext 48 140 \"to connect to a stream or proxy.\"\n"
			);

		y = 140+16;
		for (sv = cluster->servers; sv; sv = sv->next)
		{
			if (!shownheader)
			{
				shownheader = true;
				QW_StuffcmdToViewer(v, "menutext 72 %i \"�����������\"\n", y);
				y+=8;
			}
			QW_StuffcmdToViewer(v, "menutext 32 %i \"%30s\" \"stream %i\"\n", y, *sv->hostname?sv->hostname:sv->server, sv->streamid);
			y+=8;
		}
		if (!shownheader)
			QW_StuffcmdToViewer(v, "menutext 72 %i \"There are no active games\"\n", y);
		
	}
	else if (!strncmp(message, ".guiadmin", 6))
	{
		if (!*cluster->adminpassword)
		{
			QW_StuffcmdToViewer(v, 

				"alias menucallback\n"
				"{\n"
					"menuclear\n"
				"}\n"

				"conmenu menucallback\n"
				"menupic 16 4 gfx/qplaque.lmp\n"
				"menupic - 4 gfx/p_option.lmp\n"

				"menutext 72 48 \"No admin password is set\"\n"
				"menutext 72 56 \"Admin access is prohibited\"\n"
				);
		}
		else if (v->isadmin)
			//already an admin, so don't show admin login screen
			QW_SetMenu(v, MENU_ADMIN);
		else
		{
			QW_StuffcmdToViewer(v, 

				"alias menucallback\n"
				"{\n"
					"menuclear\n"
					"if (option == \"log\")\n"
						"{\nsay $_password\n}\n"
					"set _password \"\"\n"
				"}\n"

				"conmenu menucallback\n"
				"menupic 16 4 gfx/qplaque.lmp\n"
				"menupic - 4 gfx/p_option.lmp\n"

				"menuedit 16 32 \"        Password\" \"_password\"\n"

				"menutext 72 48 \"Log in QW\" log\n"
				"menutext 192 48 \"Cancel\" cancel\n"
				);

			strcpy(v->expectcommand, "admin");
		}
	}
	else if (!strncmp(message, ".reset", 6))
	{
		QW_SetCommentator(cluster, v, NULL);
		QW_SetViewersServer(cluster, v, NULL);
		QW_SetMenu(v, MENU_SERVERS);
	}
	else if (!strncmp(message, ".admin", 6))
	{
		if (!*cluster->adminpassword)
		{
			if (Netchan_IsLocal(v->netchan.remote_address))
			{
				Sys_Printf(cluster, "Local player %s logs in as admin\n", v->name);
				QW_SetMenu(v, MENU_ADMIN);
				v->isadmin = true;
			}
			else
				QW_PrintfToViewer(v, "There is no admin password set\nYou may not log in.\n");
		}
		else if (v->isadmin)
			QW_SetMenu(v, MENU_ADMIN);
		else
		{
			strcpy(v->expectcommand, "admin");
			QW_StuffcmdToViewer(v, "echo Please enter the rcon password\nmessagemode\n");
		}
	}
	else if (!strncmp(message, ".connect ", 9) || !strncmp(message, ".qw ", 4) || !strncmp(message, ".observe ", 9))
	{
		if (!strncmp(message, ".qw ", 4))
			message += 4;
		else
			message += 9;
		snprintf(buf, sizeof(buf), "udp:%s", message);
		qtv = QTV_NewServerConnection(cluster, buf, "", false, true, true);
		if (qtv)
		{
			QW_SetMenu(v, MENU_NONE);
			QW_SetViewersServer(cluster, v, qtv);
			QW_PrintfToViewer(v, "Connected\n", message);
		}
		else
			QW_PrintfToViewer(v, "Failed to connect to server \"%s\", connection aborted\n", message);
	}
	else if (!strncmp(message, ".join ", 6))
	{
		message += 6;
		snprintf(buf, sizeof(buf), "udp:%s", message);
		qtv = QTV_NewServerConnection(cluster, buf, "", false, true, false);
		if (qtv)
		{
			QW_SetMenu(v, MENU_NONE);
			QW_SetViewersServer(cluster, v, qtv);
			qtv->controller = v;
			QW_PrintfToViewer(v, "Connected\n", message);
		}
		else
			QW_PrintfToViewer(v, "Failed to connect to server \"%s\", connection aborted\n", message);
	}
	else if (!strncmp(message, ".qtv ", 5))
	{
		message += 5;
		snprintf(buf, sizeof(buf), "tcp:%s", message);
		qtv = QTV_NewServerConnection(cluster, buf, "", false, true, true);
		if (qtv)
		{
			QW_SetMenu(v, MENU_NONE);
			QW_SetViewersServer(cluster, v, qtv);
			QW_PrintfToViewer(v, "Connected\n", message);
		}
		else
			QW_PrintfToViewer(v, "Failed to connect to server \"%s\", connection aborted\n", message);
	}
	else if (!strncmp(message, ".stream ", 7))
	{
		int id;
		message += 7;
		id = atoi(message);
		for (qtv = cluster->servers; qtv; qtv = qtv->next)
		{
			if (qtv->streamid == id)
			{
				break;
			}
		}
		if (qtv)
		{
			QW_SetMenu(v, MENU_NONE);
			QW_SetViewersServer(cluster, v, qtv);
			QW_PrintfToViewer(v, "Connected\n", message);
		}
		else
		{
			QW_PrintfToViewer(v, "Stream not recognised. Stream id is invalid or terminated.\n", message);
		}
	}
	else if (!strncmp(message, ".demo ", 6))
	{
		message += 6;
		snprintf(buf, sizeof(buf), "file:%s", message);
		qtv = QTV_NewServerConnection(cluster, buf, "", false, true, true);
		if (qtv)
		{
			QW_SetMenu(v, MENU_NONE);
			QW_SetViewersServer(cluster, v, qtv);
			QW_PrintfToViewer(v, "Connected\n", message);
		}
		else
			QW_PrintfToViewer(v, "Failed to connect to server \"%s\", connection aborted\n", message);
	}
	else if (!strncmp(message, ".disconnect", 11))
	{
		QW_SetMenu(v, MENU_SERVERS);
		QW_SetViewersServer(cluster, v, NULL);
		QW_PrintfToViewer(v, "Connected\n", message);
	}
	else if (!strncmp(message, "admin", 11))
	{
		QW_StuffcmdToViewer(v, "cmd say \".admin\"\n");
	}
	else if (!strncmp(message, ".clients", 8))
	{
		viewer_t *ov;
		int skipfirst = 0;
		int printable = 30;
		int remaining = 0;
		for (ov = cluster->viewers; ov; ov = ov->next)
		{
			if (skipfirst > 0)
			{
				skipfirst--;
			}
			else if (printable > 0)
			{
				printable--;
				if (ov->server)
				{
					if (ov->server->controller == ov)
						QW_PrintfToViewer(v, "%i: %s: %s\n", ov->userid, ov->name, ov->server->server);
					else
						QW_PrintfToViewer(v, "%i: %s: %s\n", ov->userid, ov->name, ov->server->server);
				}
				else
					QW_PrintfToViewer(v, "%i: %s: %s\n", ov->userid, ov->name, "None");
			}
			else
				remaining++;
		}
		if (remaining)
			QW_PrintfToViewer(v, "%i clients not shown\n", remaining);
	}
	else if (!strncmp(message, ".followid ", 10))
	{
		int id = atoi(message+10);
		viewer_t *cv;

		for (cv = cluster->viewers; cv; cv = cv->next)
		{
			if (cv->userid == id)
			{
				QW_SetCommentator(cluster, v, cv);
				return;
			}
		}
		QW_PrintfToViewer(v, "Couldn't find that player\n");
		QW_SetCommentator(cluster, v, NULL);
	}
	else if (!strncmp(message, ".follow ", 8))
	{
		char *id = message+8;
		viewer_t *cv;

		for (cv = cluster->viewers; cv; cv = cv->next)
		{
			if (!strcmp(cv->name, id))
			{
				QW_SetCommentator(cluster, v, cv);
				return;
			}
		}
		QW_PrintfToViewer(v, "Couldn't find that player\n");
		QW_SetCommentator(cluster, v, NULL);
	}
	else if (!strncmp(message, ".follow", 7))
	{
		QW_SetCommentator(cluster, v, NULL);
	}
	else if (!strncmp(message, "proxy:menu up", 13))
	{
		v->menuop -= 1;
	}
	else if (!strncmp(message, "proxy:menu down", 15))
	{
		v->menuop += 1;
	}
	else if (!strncmp(message, "proxy:menu enter", 16))
	{
		Menu_Enter(cluster, v, 1);
	}
	else if (!strncmp(message, "proxy:menu right", 16))
	{
		Menu_Enter(cluster, v, 1);
	}
	else if (!strncmp(message, "proxy:menu left", 15))
	{
		Menu_Enter(cluster, v, -1);
	}
	else if (!strncmp(message, "proxy:menu select", 17))
	{
		Menu_Enter(cluster, v, 0);
	}
	else if (!strncmp(message, "proxy:menu home", 15))
	{
		v->menuop -= 100000;
	}
	else if (!strncmp(message, "proxy:menu end", 14))
	{
		v->menuop += 100000;
	}
	else if (!strncmp(message, "proxy:menu back", 15))
	{
	}
	else if (!strncmp(message, "proxy:menu", 10))
	{
		if (v->menunum)
			Menu_Enter(cluster, v, 0);
		else
			QW_SetMenu(v, MENU_SERVERS);
	}
	else if (!strncmp(message, ".bind", 5))
	{
		QW_StuffcmdToViewer(v, "bind uparrow +proxfwd\n");
		QW_StuffcmdToViewer(v, "bind downarrow +proxback\n");
		QW_StuffcmdToViewer(v, "bind rightarrow +proxright\n");
		QW_StuffcmdToViewer(v, "bind leftarrow +proxleft\n");
		QW_PrintfToViewer(v, "Keys bound not recognised\n");
	}
	else if (!strncmp(message, ".menu bind", 10) || !strncmp(message, "proxy:menu bindstd", 18))
	{
		QW_StuffcmdToViewer(v, "bind uparrow \"proxy:menu up\"\n");
		QW_StuffcmdToViewer(v, "bind downarrow \"proxy:menu down\"\n");
		QW_StuffcmdToViewer(v, "bind rightarrow \"proxy:menu right\"\n");
		QW_StuffcmdToViewer(v, "bind leftarrow \"proxy:menu left\"\n");

		QW_StuffcmdToViewer(v, "bind enter \"proxy:menu select\"\n");

		QW_StuffcmdToViewer(v, "bind home \"proxy:menu home\"\n");
		QW_StuffcmdToViewer(v, "bind end \"proxy:menu end\"\n");
		QW_StuffcmdToViewer(v, "bind pause \"proxy:menu\"\n");
		QW_StuffcmdToViewer(v, "bind backspace \"proxy:menu back\"\n");

		QW_PrintfToViewer(v, "All keys bound not recognised\n");
	}
	else if (!strncmp(message, ".bsay ", 6))
	{
		viewer_t *ov;
		if (cluster->notalking)
			return;

		message += 6;

		for (ov = cluster->viewers; ov; ov = ov->next)
		{
			InitNetMsg(&msg, buf, sizeof(buf));

			WriteByte(&msg, svc_print);

			if (ov->netchan.isnqprotocol)
				WriteByte(&msg, 1);
			else
			{
				if (ov->conmenussupported)
				{
					WriteByte(&msg, 3);	//PRINT_CHAT
					WriteString2(&msg, "[^sBQTV^s]^s^5");
				}
				else
				{
					WriteByte(&msg, 2);	//PRINT_HIGH
					WriteByte(&msg, 91+128);
					WriteString2(&msg, "BQTV");
					WriteByte(&msg, 93+128);
					WriteByte(&msg, 0);

					WriteByte(&msg, svc_print);
					WriteByte(&msg, 3);	//PRINT_CHAT

				}
			}

			WriteString2(&msg, v->name);
			WriteString2(&msg, ": ");
//				WriteString2(&msg, "\x8d ");
			WriteString2(&msg, message);
			WriteString(&msg, "\n");

			SendBufferToViewer(ov, msg.data, msg.cursize, true);
		}
	}
	else if (!strncmp(message, ".", 1) && strncmp(message, "..", 2))
	{
		QW_PrintfToViewer(v, "Proxy command not recognised\n");
	}
	else
	{
		if (!strncmp(message, ".", 1))
			message++;
		*v->expectcommand = '\0';

		if (qtv && qtv->usequkeworldprotocols && !noupwards)
		{
			if (qtv->controller == v || !*v->name)
			{
				SendClientCommand(qtv, "say %s\n", message);

				if (cluster->notalking)
					return;
			}
			else
			{
				if (cluster->notalking)
					return;
				SendClientCommand(qtv, "say %s: %s\n", v->name, message);
			}

			//FIXME: we ought to broadcast this to everyone not watching that qtv.
		}
		else
		{
			viewer_t *ov;
			if (cluster->notalking)
				return;

			for (ov = cluster->viewers; ov; ov = ov->next)
			{
				if (ov->server != v->server)
					continue;

				InitNetMsg(&msg, buf, sizeof(buf));

				WriteByte(&msg, svc_print);

				if (ov->netchan.isnqprotocol)
					WriteByte(&msg, 1);
				else
				{
					if (ov->conmenussupported)
					{
						WriteByte(&msg, 3);	//PRINT_CHAT
						WriteString2(&msg, "[^sQTV^s]^s^5");
					}
					else
					{
						WriteByte(&msg, 2);	//PRINT_HIGH
						WriteByte(&msg, 91+128);
						WriteString2(&msg, "QTV");
						WriteByte(&msg, 93+128);
						WriteByte(&msg, 0);

						WriteByte(&msg, svc_print);
						WriteByte(&msg, 3);	//PRINT_CHAT

					}
				}

				WriteString2(&msg, v->name);
				WriteString2(&msg, ": ");
//				WriteString2(&msg, "\x8d ");
				WriteString2(&msg, message);
				WriteString(&msg, "\n");

				SendBufferToViewer(ov, msg.data, msg.cursize, true);
			}
		}
	}
}

viewer_t *QW_IsOn(cluster_t *cluster, char *name)
{
	viewer_t *v;
	for (v = cluster->viewers; v; v = v->next)
		if (!stricmp(v->name, name))		//this needs to allow dequakified names.
			return v;

	return NULL;
}

void QW_PrintfToViewer(viewer_t *v, char *format, ...)
{
	va_list		argptr;
	char buf[1024];

	va_start (argptr, format);
	vsnprintf (buf+2, sizeof(buf)-2, format, argptr);
	va_end (argptr);

	buf[0] = svc_print;
	buf[1] = 2;	//PRINT_HIGH

	SendBufferToViewer(v, buf, strlen(buf)+1, true);
}


void QW_StuffcmdToViewer(viewer_t *v, char *format, ...)
{
	va_list		argptr;
	char buf[1024];

	va_start (argptr, format);
	vsnprintf (buf+1, sizeof(buf)-1, format, argptr);
	va_end (argptr);

	buf[0] = svc_stufftext;
	SendBufferToViewer(v, buf, strlen(buf)+1, true);
}

void QW_PositionAtIntermission(sv_t *qtv, viewer_t *v)
{
	netmsg_t msg;
	char buf[4];
	const intermission_t *spot;


	if (qtv)
		spot = BSP_IntermissionSpot(qtv->bsp);
	else
		spot = BSP_IntermissionSpot(NULL);


	v->origin[0] = spot->pos[0];
	v->origin[1] = spot->pos[1];
	v->origin[2] = spot->pos[2];


	msg.data = buf;
	msg.maxsize = sizeof(buf);
	msg.cursize = 0;
	msg.overflowed = 0;

	WriteByte (&msg, svc_setangle);
	WriteByte (&msg, (spot->angle[0]/360) * 256);
	WriteByte (&msg, (spot->angle[1]/360) * 256);
	WriteByte (&msg, 0);//spot->angle[2]);

	SendBufferToViewer(v, msg.data, msg.cursize, true);
}

void ParseNQC(cluster_t *cluster, sv_t *qtv, viewer_t *v, netmsg_t *m)
{
	char buf[MAX_NQMSGLEN];
	netmsg_t msg;
	int mtype;

	while (m->readpos < m->cursize)
	{
		switch ((mtype=ReadByte(m)))
		{
		case clc_nop:
			break;
		case clc_stringcmd:
			ReadString (m, buf, sizeof(buf));
			printf("stringcmd: %s\n", buf);

			if (!strcmp(buf, "new"))
			{
				if (qtv && qtv->parsingconnectiondata)
					QW_StuffcmdToViewer(v, "cmd new\n");
				else
				{
					SendServerData(qtv, v);
				}
			}
			else if (!strncmp(buf, "prespawn", 8))
			{
				msg.data = buf;
				msg.maxsize = sizeof(buf);
				msg.cursize = 0;
				msg.overflowed = 0;

				if (qtv)
				{
					SendCurrentBaselines(qtv, 64, &msg, msg.maxsize, 0);
					SendCurrentLightmaps(qtv, 64, &msg, msg.maxsize, 0);

					SendStaticSounds(qtv, 64, &msg, msg.maxsize, 0);

					SendStaticEntities(qtv, 64, &msg, msg.maxsize, 0);
				}
				WriteByte (&msg, svc_nqsignonnum);
				WriteByte (&msg, 2);
				SendBufferToViewer(v, msg.data, msg.cursize, true);
			}

			else if (!strncmp(buf, "setinfo", 5))
			{
				#define TOKENIZE_PUNCTUATION ""
				#define MAX_ARGS 3
				#define ARG_LEN 256

				int i;
				char arg[MAX_ARGS][ARG_LEN];
				char *argptrs[MAX_ARGS];
				char *command = buf;

				for (i = 0; i < MAX_ARGS; i++)
				{
					command = COM_ParseToken(command, arg[i], ARG_LEN, TOKENIZE_PUNCTUATION);
					argptrs[i] = arg[i];
				}

				Info_SetValueForStarKey(v->userinfo, arg[1], arg[2], sizeof(v->userinfo));
				ParseUserInfo(cluster, v);
//				Info_ValueForKey(v->userinfo, "name", v->name, sizeof(v->name));

				if (v->server && v->server->controller == v)
					SendClientCommand(v->server, "%s", buf);
			}

			else if (!strncmp(buf, "name ", 5))
			{
				Info_SetValueForStarKey(v->userinfo, "name", buf+5, sizeof(v->userinfo));
				ParseUserInfo(cluster, v);

				if (v->server && v->server->controller == v)
					SendClientCommand(v->server, "setinfo name \"%s\"", v->name);
			}
			else if (!strncmp(buf, "color ", 6))
			{
				/*
				fixme
				*/
			}
			else if (!strncmp(buf, "spawn", 5))
			{
				msg.data = buf;
				msg.maxsize = sizeof(buf);
				msg.cursize = 0;
				msg.overflowed = 0;
				SendNQSpawnInfoToViewer(cluster, v, &msg);
				SendBufferToViewer(v, msg.data, msg.cursize, true);

				QW_PositionAtIntermission(qtv, v);

				v->thinksitsconnected = true;
			}
			else if (!strncmp(buf, "begin", 5))
			{
				int oldmenu;
				v->thinksitsconnected = true;

				oldmenu = v->menunum;
				QW_SetMenu(v, MENU_NONE);
				QW_SetMenu(v, oldmenu);

				if (!v->server)
					QTV_Say(cluster, v->server, v, ".menu", false);
			}

			else if (!strncmp(buf, "say \"", 5))
				QTV_Say(cluster, qtv, v, buf+5, false);
			else if (!strncmp(buf, "say ", 4))
				QTV_Say(cluster, qtv, v, buf+4, false);

			else if (!strncmp(buf, "say_team \"", 10))
				QTV_Say(cluster, qtv, v, buf+10, true);
			else if (!strncmp(buf, "say_team ", 9))
				QTV_Say(cluster, qtv, v, buf+9, true);


			else
			{
				QW_PrintfToViewer(v, "Command not recognised\n");
				Sys_Printf(cluster, "NQ client sent unrecognised stringcmd %s\n", buf);
			}
			break;
		case clc_disconnect:
			if (!v->drop)
				Sys_Printf(cluster, "NQ viewer %s disconnects\n", v->name);
			v->drop = true;
			return;
		case clc_move:
			ReadFloat(m);	//time, for pings
			//three angles
			v->ucmds[2].angles[0] = ReadByte(m)*256;
			v->ucmds[2].angles[1] = ReadByte(m)*256;
			v->ucmds[2].angles[2] = ReadByte(m)*256;
			//three direction values
			v->ucmds[2].forwardmove = ReadShort(m);
			v->ucmds[2].sidemove = ReadShort(m);
			v->ucmds[2].upmove = ReadShort(m);

			//one button
			v->ucmds[1].buttons = v->ucmds[2].buttons;
			v->ucmds[2].buttons = ReadByte(m);
			//one impulse
			v->ucmds[2].impulse = ReadByte(m);

			v->ucmds[2].msec = 5000/NQ_PACKETS_PER_SECOND;
			PMove(v, &v->ucmds[2]);

			if ((v->ucmds[1].buttons&1) != (v->ucmds[2].buttons&1) && (v->ucmds[2].buttons&1))
			{
				v->trackplayer++;
			}
			if ((v->ucmds[1].buttons&2) != (v->ucmds[2].buttons&2) && (v->ucmds[2].buttons&2))
			{
				v->trackplayer--;
			}
			break;
		default:
			Sys_Printf(cluster, "Bad message type %i\n", mtype);
			return;
		}
	}
}
void ParseQWC(cluster_t *cluster, sv_t *qtv, viewer_t *v, netmsg_t *m)
{
//	usercmd_t	oldest, oldcmd, newcmd;
	char buf[1024];
	netmsg_t msg;
	int i;

	v->delta_frame = -1;

	while (m->readpos < m->cursize)
	{
		i = ReadByte(m);
		switch (i)
		{
		case clc_nop:
			return;
		case clc_delta:
			v->delta_frame = ReadByte(m);
			break;
		case clc_stringcmd:
			ReadString (m, buf, sizeof(buf));
//			printf("stringcmd: %s\n", buf);

			if (!strcmp(buf, "new"))
			{
				if (qtv && qtv->parsingconnectiondata)
					QW_StuffcmdToViewer(v, "cmd new\n");
				else
				{
					QW_StuffcmdToViewer(v, "//querycmd conmenu\n");
					SendServerData(qtv, v);
				}
			}
			else if (!strncmp(buf, "modellist ", 10))
			{
				char *cmd = buf+10;
				int svcount = atoi(cmd);
				int first;

				while((*cmd >= '0' && *cmd <= '9') || *cmd == '-')
					cmd++;
				first = atoi(cmd);

				InitNetMsg(&msg, buf, sizeof(buf));

				if (svcount != v->servercount)
				{	//looks like we changed map without them.
					SendServerData(qtv, v);
					return;
				}

				if (!qtv)
					SendList(qtv, first, ConnectionlessModelList, svc_modellist, &msg);
				else
					SendList(qtv, first, qtv->modellist, svc_modellist, &msg);
				SendBufferToViewer(v, msg.data, msg.cursize, true);
			}
			else if (!strncmp(buf, "soundlist ", 10))
			{
				char *cmd = buf+10;
				int svcount = atoi(cmd);
				int first;

				while((*cmd >= '0' && *cmd <= '9') || *cmd == '-')
					cmd++;
				first = atoi(cmd);

				InitNetMsg(&msg, buf, sizeof(buf));

				if (svcount != v->servercount)
				{	//looks like we changed map without them.
					SendServerData(qtv, v);
					return;
				}

				if (!qtv)
					SendList(qtv, first, ConnectionlessSoundList, svc_soundlist, &msg);
				else
					SendList(qtv, first, qtv->soundlist, svc_soundlist, &msg);
				SendBufferToViewer(v, msg.data, msg.cursize, true);
			}
			else if (!strncmp(buf, "prespawn", 8))
			{
				char skin[128];

				if (atoi(buf + 9) != v->servercount)
					SendServerData(qtv, v);	//we're old.
				else
				{
					int crc;
					int r;
					char *s;
					s = buf+8;
					while(*s == ' ')
						s++;
					while((*s >= '0' && *s <= '9') || *s == '-')
						s++;
					while(*s == ' ')
						s++;
					r = atoi(s);

					if (r == 0)
					{
						while((*s >= '0' && *s <= '9') || *s == '-')
							s++;
						while(*s == ' ')
							s++;
						crc = atoi(s);

						if (qtv && qtv->controller == v)
						{
							if (!qtv->bsp)
							{
								QW_PrintfToViewer(v, "Proxy was unable to check your map version\n");
								qtv->drop = true;
							}
							else if (crc != BSP_Checksum(qtv->bsp))
							{
								QW_PrintfToViewer(v, "Your map (%s) does not match the servers\n", qtv->modellist[1].name);
								qtv->drop = true;
							}
						}
					}

					InitNetMsg(&msg, buf, sizeof(buf));

					if (qtv)
					{
						r = Prespawn(qtv, v->netchan.message.cursize, &msg, r, v->thisplayer);
						SendBufferToViewer(v, msg.data, msg.cursize, true);
					}
					else
					{
						r = SendCurrentUserinfos(qtv, v->netchan.message.cursize, &msg, r, v->thisplayer);
						if (r > MAX_CLIENTS)
							r = -1;
						SendBufferToViewer(v, msg.data, msg.cursize, true);
					}

					if (r < 0)
						sprintf(skin, "%ccmd spawn\n", svc_stufftext);
					else
						sprintf(skin, "%ccmd prespawn %i %i\n", svc_stufftext, v->servercount, r);

					SendBufferToViewer(v, skin, strlen(skin)+1, true);
				}
			}
			else if (!strncmp(buf, "spawn", 5))
			{
				char skin[64];
				sprintf(skin, "%cskins\n", svc_stufftext);
				SendBufferToViewer(v, skin, strlen(skin)+1, true);

				QW_PositionAtIntermission(qtv, v);
			}
			else if (!strncmp(buf, "begin", 5))
			{
				int oldmenu;
				viewer_t *com;
				if (atoi(buf+6) != v->servercount)
					SendServerData(qtv, v);	//this is unfortunate!
				else
				{
					v->thinksitsconnected = true;
					if (qtv && qtv->ispaused)
					{
						char msgb[] = {svc_setpause, 1};
						SendBufferToViewer(v, msgb, sizeof(msgb), true);
					}

					oldmenu = v->menunum;
					QW_SetMenu(v, MENU_NONE);
					QW_SetMenu(v, oldmenu);
				

					com = v->commentator;
					v->commentator = NULL;
					QW_SetCommentator(cluster, v, com);


					if (!v->server)
						QTV_Say(cluster, v->server, v, ".menu", false);
				}
			}
			else if (!strncmp(buf, "download", 8))
			{
				netmsg_t m;
				InitNetMsg(&m, buf, sizeof(buf));
				WriteByte(&m, svc_download);
				WriteShort(&m, -1);
				WriteByte(&m, 0);
				SendBufferToViewer(v, m.data, m.cursize, true);
			}
			else if (!strncmp(buf, "drop", 4))
			{
				if (!v->drop)
					Sys_Printf(cluster, "QW viewer %s disconnects\n", v->name);
				v->drop = true;
			}
			else if (!strncmp(buf, "ison", 4))
			{
				viewer_t *other;
				if ((other = QW_IsOn(cluster, buf+5)))
				{
					if (!other->server)
						QW_PrintfToViewer(v, "%s is on the proxy, but not yet watching a game\n", other->name);
					else
						QW_PrintfToViewer(v, "%s is watching %s\n", buf+5, other->server->server);
				}
				else
					QW_PrintfToViewer(v, "%s is not on the proxy, sorry\n", buf+5);	//the apology is to make the alternatives distinct.
			}
			else if (!strncmp(buf, "ptrack ", 7))
			{
				v->trackplayer = atoi(buf+7);
//				if (v->trackplayer != MAX_CLIENTS-2)
//					QW_SetCommentator(v, NULL);
			}
			else if (!strncmp(buf, "ptrack", 6))
			{
				v->trackplayer = -1;
				QW_SetCommentator(cluster, v, NULL);
			}
			else if (!strncmp(buf, "pings", 5))
			{
			}
			else if (!strncmp(buf, "say \"", 5))
				QTV_Say(cluster, qtv, v, buf+5, false);
			else if (!strncmp(buf, "say ", 4))
				QTV_Say(cluster, qtv, v, buf+4, false);

			else if (!strncmp(buf, "say_team \"", 10))
				QTV_Say(cluster, qtv, v, buf+10, true);
			else if (!strncmp(buf, "say_team ", 9))
				QTV_Say(cluster, qtv, v, buf+9, true);

			else if (!strncmp(buf, "servers", 7))
			{
				QW_SetMenu(v, MENU_SERVERS);
			}

			else if (!strncmp(buf, "setinfo", 5))
			{
				#define TOKENIZE_PUNCTUATION ""
				#define MAX_ARGS 3
				#define ARG_LEN 256

				int i;
				char arg[MAX_ARGS][ARG_LEN];
				char *argptrs[MAX_ARGS];
				char *command = buf;

				for (i = 0; i < MAX_ARGS; i++)
				{
					command = COM_ParseToken(command, arg[i], ARG_LEN, TOKENIZE_PUNCTUATION);
					argptrs[i] = arg[i];
				}

				Info_SetValueForStarKey(v->userinfo, arg[1], arg[2], sizeof(v->userinfo));
				ParseUserInfo(cluster, v);
//				Info_ValueForKey(v->userinfo, "name", v->name, sizeof(v->name));

				if (v->server && v->server->controller == v)
					SendClientCommand(v->server, "%s", buf);
			}
			else if (!strncmp(buf, "cmdsupported ", 13))
			{
				if (!strcmp(buf+13, "conmenu"))
					v->conmenussupported = true;
				else if (v->server && v->server->controller == v)
					SendClientCommand(v->server, "%s", buf);
			}
			else if (!qtv)
			{
				//all the other things need an active server.
				QW_PrintfToViewer(v, "Choose a server first\n");
			}
			else if (!strncmp(buf, "serverinfo", 5))
			{
				char *key, *value, *end;
				int len;
				netmsg_t m;
				InitNetMsg(&m, buf, sizeof(buf));
				WriteByte(&m, svc_print);
				WriteByte(&m, 2);
				end = qtv->serverinfo;
				for(;;)
				{
					if (!*end)
						break;
					key = end;
					value = strchr(key+1, '\\');
					if (!value)
						break;
					end = strchr(value+1, '\\');
					if (!end)
						end = value+strlen(value);

					len = value-key;

					key++;
					while(*key != '\\' && *key)
						WriteByte(&m, *key++);

					for (; len < 20; len++)
						WriteByte(&m, ' ');

					value++;
					while(*value != '\\' && *value)
						WriteByte(&m, *value++);
					WriteByte(&m, '\n');
				}
				WriteByte(&m, 0);

//				WriteString(&m, qtv->serverinfo);
				SendBufferToViewer(v, m.data, m.cursize, true);
			}
			else
			{
				if (v->server && v->server->controller == v)
					SendClientCommand(v->server, "%s", buf);
				else
					Sys_Printf(cluster, "Client sent unknown string command: %s\n", buf);
			}

			break;

		case clc_move:
			v->lost = ReadByte(m);
			ReadByte(m);
			ReadDeltaUsercmd(m, &nullcmd, &v->ucmds[0]);
			ReadDeltaUsercmd(m, &v->ucmds[0], &v->ucmds[1]);
			ReadDeltaUsercmd(m, &v->ucmds[1], &v->ucmds[2]);

			PMove(v, &v->ucmds[2]);
			break;
		case clc_tmove:
			v->origin[0] = ((signed short)ReadShort(m))/8.0f;
			v->origin[1] = ((signed short)ReadShort(m))/8.0f;
			v->origin[2] = ((signed short)ReadShort(m))/8.0f;
			break;

		case clc_upload:
			Sys_Printf(cluster, "Client uploads are not supported from %s\n", v->name);
			v->drop = true;
			return;

		default:
			Sys_Printf(cluster, "bad clc from %s\n", v->name);
			v->drop = true;
			return;
		}
	}
}

void Menu_Enter(cluster_t *cluster, viewer_t *viewer, int buttonnum)
{
	//build a possible message, even though it'll probably not be sent

	sv_t *sv;
	int i, min;

	switch(viewer->menunum)
	{
	default:
		break;

	case MENU_ADMINSERVER:
		if (viewer->server)
		{
			i = 0;
			sv = viewer->server;
			if (i++ == viewer->menuop)
			{	//mvd port
				QW_StuffcmdToViewer(viewer, "echo Please enter a new tcp port number\nmessagemode\n");
				strcpy(viewer->expectcommand, "setmvdport");
			}
			if (i++ == viewer->menuop)
			{	//disconnect
				QTV_Shutdown(viewer->server);
			}
			if (i++ == viewer->menuop)
			{	//back
				QW_SetMenu(viewer, MENU_ADMIN);
			}
			break;
		}
		//fallthrough
	case MENU_SERVERS:
		if (!cluster->servers)
		{
			QW_StuffcmdToViewer(viewer, "echo Please enter a server ip\nmessagemode\n");
			strcpy(viewer->expectcommand, "insecadddemo");
		}
		else
		{
			if (viewer->menuop < 0)
				viewer->menuop = 0;
			i = 0;
			min = viewer->menuop - 10;
			if (min < 0)
				min = 0;
			for (sv = cluster->servers; sv && i<min; sv = sv->next, i++)
			{//skip over the early connections.
			}
			min+=20;
			for (; sv && i < min; sv = sv->next, i++)
			{
				if (i == viewer->menuop)
				{
					/*if (sv->parsingconnectiondata || !sv->modellist[1].name[0])
					{
						QW_PrintfToViewer(viewer, "But that stream isn't connected\n");
					}
					else*/
					{
						QW_SetViewersServer(cluster, viewer, sv);
						QW_SetMenu(viewer, MENU_NONE);
						viewer->thinksitsconnected = false;
					}
					break;
				}
			}
		}
		break;
	case MENU_ADMIN:
		i = 0;
		if (i++ == viewer->menuop)
		{	//connection stuff
			QW_SetMenu(viewer, MENU_ADMINSERVER);
		}
		if (i++ == viewer->menuop)
		{	//qw port
			QW_StuffcmdToViewer(viewer, "echo You will need to reconnect\n");
			cluster->qwlistenportnum += (buttonnum<0)?-1:1;
		}
		if (i++ == viewer->menuop)
		{	//hostname
			strcpy(viewer->expectcommand, "hostname");
			QW_StuffcmdToViewer(viewer, "echo Please enter the new hostname\nmessagemode\n");
		}
		if (i++ == viewer->menuop)
		{	//master
			strcpy(viewer->expectcommand, "master");
			QW_StuffcmdToViewer(viewer, "echo Please enter the master dns or ip\necho Enter '.' for masterless mode\nmessagemode\n");
		}
		if (i++ == viewer->menuop)
		{	//password
			strcpy(viewer->expectcommand, "password");
			QW_StuffcmdToViewer(viewer, "echo Please enter the new rcon password\nmessagemode\n");
		}
		if (i++ == viewer->menuop)
		{	//add server
			strcpy(viewer->expectcommand, "messagemode");
			QW_StuffcmdToViewer(viewer, "echo Please enter the new qtv server dns or ip\naddserver\n");
		}
		if (i++ == viewer->menuop)
		{	//add demo
			strcpy(viewer->expectcommand, "adddemo");
			QW_StuffcmdToViewer(viewer, "echo Please enter the name of the demo to play\nmessagemode\n");
		}
		if (i++ == viewer->menuop)
		{	//choke
			cluster->chokeonnotupdated ^= 1;
		}
		if (i++ == viewer->menuop)
		{	//late forwarding
			cluster->lateforward ^= 1;
		}
		if (i++ == viewer->menuop)
		{	//no talking
			cluster->notalking ^= 1;
		}
		if (i++ == viewer->menuop)
		{	//nobsp
			cluster->nobsp ^= 1;
		}
		if (i++ == viewer->menuop)
		{	//back
			QW_SetMenu(viewer, MENU_NONE);
		}


		break;
	}
}

void Menu_Draw(cluster_t *cluster, viewer_t *viewer)
{
	char buffer[2048];
	char str[64];
	sv_t *sv;
	int i, min;
	unsigned char *s;

	netmsg_t m;

	if (viewer->backbuffered)
		return;

	InitNetMsg(&m, buffer, sizeof(buffer));

	WriteByte(&m, svc_centerprint);

	sprintf(str, "FTEQTV build %i\n", cluster->buildnumber);
	WriteString2(&m, str);
	if (strcmp(cluster->hostname, DEFAULT_HOSTNAME))
		WriteString2(&m, cluster->hostname);

	switch(viewer->menunum)
	{
	default:
		WriteString2(&m, "bad menu");
		break;

	case 3:	//per-connection options
		if (viewer->server)
		{
			sv = viewer->server;
			WriteString2(&m, "\n\nConnection Admin\n");
			WriteString2(&m, sv->hostname);
			if (sv->sourcefile)
				WriteString2(&m, " (demo)");
			WriteString2(&m, "\n\n");

			if (viewer->menuop < 0)
				viewer->menuop = 0;

			i = 0;
			WriteString2(&m, "            port");
			WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
			if (sv->tcpsocket == INVALID_SOCKET)
				sprintf(str, "!%-19i", sv->tcplistenportnum);
			else
				sprintf(str, "%-20i", sv->tcplistenportnum);
			WriteString2(&m, str);
			WriteString2(&m, "\n");

			WriteString2(&m, "      disconnect");
			WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
			sprintf(str, "%-20s", "...");
			WriteString2(&m, str);
			WriteString2(&m, "\n");

			WriteString2(&m, "            back");
			WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
			sprintf(str, "%-20s", "...");
			WriteString2(&m, str);
			WriteString2(&m, "\n");

			if (viewer->menuop >= i)
				viewer->menuop = i - 1;

			break;
		}
		//fallthrough
	case 1:	//connections list

		WriteString2(&m, "\n\nServers\n\n");

		if (!cluster->servers)
		{
			WriteString2(&m, "No active connections");
		}
		else
		{
			if (viewer->menuop < 0)
				viewer->menuop = 0;
			i = 0;
			min = viewer->menuop - 10;
			if (min < 0)
				min = 0;
			for (sv = cluster->servers; sv && i<min; sv = sv->next, i++)
			{//skip over the early connections.
			}
			min+=20;
			for (; sv && i < min; sv = sv->next, i++)
			{
				Info_ValueForKey(sv->serverinfo, "hostname", str, sizeof(str));
				if (sv->parsingconnectiondata || !sv->modellist[1].name[0])
					snprintf(str, sizeof(str), "%s", sv->server);

				if (i == viewer->menuop)
					for (s = (unsigned char *)str; *s; s++)
					{
						if ((unsigned)*s >= ' ')
							*s = 128 | (*s&~128);
					}
				WriteString2(&m, str);
				WriteString2(&m, "\n");
			}
		}
		break;

	case 2:	//admin menu

		WriteString2(&m, "\n\nCluster Admin\n\n");

		if (viewer->menuop < 0)
			viewer->menuop = 0;
		i = 0;

		WriteString2(&m, " this connection");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", "...");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "            port");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20i", cluster->qwlistenportnum);
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "        hostname");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", cluster->hostname);
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "          master");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", cluster->master);
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "        password");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", "...");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "      add server");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", "...");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "        add demo");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", "...");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "           choke");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", cluster->chokeonnotupdated?"yes":"no");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "delay forwarding");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", cluster->lateforward?"yes":"no");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "         talking");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", cluster->notalking?"no":"yes");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "           nobsp");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", cluster->nobsp?"yes":"no");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		WriteString2(&m, "            back");
		WriteString2(&m, (viewer->menuop==(i++))?" \r ":" : ");
		sprintf(str, "%-20s", "...");
		WriteString2(&m, str);
		WriteString2(&m, "\n");

		if (viewer->menuop >= i)
			viewer->menuop = i - 1;
		break;
	}


	WriteByte(&m, 0);
	SendBufferToViewer(viewer, m.data, m.cursize, true);
}

static const char dropcmd[] = {svc_stufftext, 'd', 'i', 's', 'c', 'o', 'n', 'n', 'e', 'c', 't', '\n', '\0'};

void QW_FreeViewer(cluster_t *cluster, viewer_t *viewer)
{
	char buf[1024];
	viewer_t *oview;
	int i;
	//note: unlink them yourself.

	snprintf(buf, sizeof(buf), "%cQTV%c%s leaves the proxy\n", 91+128, 93+128, viewer->name);
	QW_StreamPrint(cluster, viewer->server, NULL, buf);

	Sys_Printf(cluster, "Dropping viewer %s\n", viewer->name);

	//spam them thrice, then forget about them
	Netchan_Transmit(cluster, &viewer->netchan, strlen(dropcmd)+1, dropcmd);
	Netchan_Transmit(cluster, &viewer->netchan, strlen(dropcmd)+1, dropcmd);
	Netchan_Transmit(cluster, &viewer->netchan, strlen(dropcmd)+1, dropcmd);

	for (i = 0; i < MAX_BACK_BUFFERS; i++)
	{
		if (viewer->backbuf[i].data)
			free(viewer->backbuf[i].data);
	}

	if (viewer->server)
	{
		if (viewer->server->controller == viewer)
			viewer->server->drop = true;

		viewer->server->numviewers--;
	}

	for (oview = cluster->viewers; oview; oview = oview->next)
	{
		if (oview->commentator == viewer)
			QW_SetCommentator(cluster, oview, NULL);
	}

	free(viewer);

	cluster->numviewers--;
}

void QW_UpdateUDPStuff(cluster_t *cluster)
{
	char buffer[MAX_MSGLEN*2];
	char tempbuffer[256];
	netadr_t from;
	int fromsize = sizeof(from);
	int read;
	int qport;
	netmsg_t m;

	sv_t *useserver;
	viewer_t *v, *f;

	if (*cluster->master && (cluster->curtime > cluster->mastersendtime || cluster->mastersendtime > cluster->curtime + 4*1000*60))	//urm... time wrapped?
	{
		if (NET_StringToAddr(cluster->master, &from, 27000))
		{
			sprintf(buffer, "a\n%i\n0\n", cluster->mastersequence++);	//fill buffer with a heartbeat
//why is there no \xff\xff\xff\xff ?..
			NET_SendPacket(cluster, cluster->qwdsocket, strlen(buffer), buffer, from);
		}
		else
			Sys_Printf(cluster, "Cannot resolve master %s\n", cluster->master);

		cluster->mastersendtime = cluster->curtime + 3*1000*60;	//3 minuites.
	}

	m.data = buffer;
	m.cursize = 0;
	m.maxsize = MAX_MSGLEN;
	m.readpos = 0;

	for (;;)
	{
		read = recvfrom(cluster->qwdsocket, buffer, sizeof(buffer), 0, (struct sockaddr*)from, &fromsize);

		if (read <= 5)	//otherwise it's a runt or bad.
		{
			if (read < 0)	//it's bad.
				break;
			continue;
		}

		m.data = buffer;
		m.cursize = read;
		m.maxsize = MAX_MSGLEN;
		m.readpos = 0;

		if (*(int*)buffer == -1)
		{	//connectionless message
			ConnectionlessPacket(cluster, &from, &m);
			continue;
		}

		if (read < 10)	//otherwise it's a runt or bad.
		{
			if (read < 0)	//it's bad.
				break;

			qport = 0;
		}
		else
		{
			//read the qport
			ReadLong(&m);
			ReadLong(&m);
			qport = ReadShort(&m);
		}

		for (v = cluster->viewers; v; v = v->next)
		{
			if (v->netchan.isnqprotocol)
			{
				if (Net_CompareAddress(&v->netchan.remote_address, &from, 0, 0))
				{
					if (NQNetchan_Process(cluster, &v->netchan, &m))
					{
						useserver = v->server;
						if (useserver && useserver->parsingconnectiondata)
							useserver = NULL;

						v->timeout = cluster->curtime + 15*1000;

						ParseNQC(cluster, useserver, v, &m);

						if (v->server && v->server->controller == v)
						{
							QTV_Run(v->server);
						}
					}
				}
			}
			else
			{
				if (Net_CompareAddress(&v->netchan.remote_address, &from, v->netchan.qport, qport))
				{
					if (Netchan_Process(&v->netchan, &m))
					{
						useserver = v->server;
						if (useserver && useserver->parsingconnectiondata)
							useserver = NULL;

						v->timeout = cluster->curtime + 15*1000;

						v->netchan.outgoing_sequence = v->netchan.incoming_sequence;	//compensate for client->server packetloss.
						if (v->server && v->server->controller == v)
						{
	//						v->maysend = true;
							v->server->maysend =  true;
	//						v->server->netchan.outgoing_sequence = v->netchan.incoming_sequence;
						}
						else
						{
							if (!v->server)
								v->maysend = true;
							else if (!v->chokeme || !cluster->chokeonnotupdated)
							{
								v->maysend = true;
								v->chokeme = cluster->chokeonnotupdated;
							}
						}

						ParseQWC(cluster, useserver, v, &m);

						if (v->server && v->server->controller == v)
						{
							QTV_Run(v->server);
						}
					}
					break;
				}
			}
		}
		if (!v && cluster->allownqclients)
		{
			//NQ connectionless packet?
			m.readpos = 0;
			read = ReadLong(&m);
			read = SwapLong(read);
			if (read & NETFLAG_CTL)
			{	//looks hopeful
				switch(ReadByte(&m))
				{
				case CCREQ_SERVER_INFO:
					ReadString(&m, tempbuffer, sizeof(tempbuffer));
					if (!strcmp(tempbuffer, NET_GAMENAME_NQ))
					{
						m.cursize = 0;
						WriteLong(&m, 0);
						WriteByte(&m, CCREP_SERVER_INFO);
						WriteString(&m, "??");
						WriteString(&m, cluster->hostname);
						WriteString(&m, "Quake TV");
						WriteByte(&m, cluster->numviewers>255?255:cluster->numviewers);
						WriteByte(&m, cluster->maxviewers>255?255:cluster->maxviewers);
						WriteByte(&m, NET_PROTOCOL_VERSION);
						*(int*)m.data = BigLong(NETFLAG_CTL | m.cursize);
						NET_SendPacket(cluster, cluster->qwdsocket, m.cursize, m.data, from);
					}
					break;
				case CCREQ_CONNECT:
					ReadString(&m, tempbuffer, sizeof(tempbuffer));
					if (!strcmp(tempbuffer, NET_GAMENAME_NQ))
					{
						if (ReadByte(&m) == NET_PROTOCOL_VERSION)
						{
							//drop any old nq clients from this address
							for (v = cluster->viewers; v; v = v->next)
							{
								if (v->netchan.isnqprotocol)
								{
									if (Net_CompareAddress(&v->netchan.remote_address, &from, 0, 0))
									{
										Sys_Printf(cluster, "Dup connect from %s\n", v->name);
										v->drop = true;
									}
								}
							}
							NewNQClient(cluster, &from);
						}
					}
					break;
				default:
					break;
				}
			}
		}
	}


	if (cluster->viewers && cluster->viewers->drop)
	{
//		Sys_Printf(cluster, "Dropping viewer %s\n", v->name);
		f = cluster->viewers;
		cluster->viewers = f->next;

		QW_FreeViewer(cluster, f);
	}

	for (v = cluster->viewers; v; v = v->next)
	{
		if (v->next && v->next->drop)
		{	//free the next/
//			Sys_Printf(cluster, "Dropping viewer %s\n", v->name);
			f = v->next;
			v->next = f->next;

			QW_FreeViewer(cluster, f);
		}

		v->drop |= v->netchan.drop;

		if (v->timeout < cluster->curtime)
		{
			Sys_Printf(cluster, "Viewer %s timed out\n", v->name);
			v->drop = true;
		}

		if (v->netchan.isnqprotocol)
		{
			v->maysend = (v->nextpacket < cluster->curtime);
		}
		if (!Netchan_CanPacket(&v->netchan))
			continue;
		if (v->maysend)	//don't send incompleate connection data.
		{
			v->nextpacket = cluster->curtime + 1000/NQ_PACKETS_PER_SECOND;

			useserver = v->server;
			if (useserver && useserver->parsingconnectiondata)
				useserver = NULL;

			v->maysend = false;
			m.cursize = 0;
			if (v->thinksitsconnected)
			{
				if (v->netchan.isnqprotocol)
					SendNQPlayerStates(cluster, useserver, v, &m);
				else
					SendPlayerStates(useserver, v, &m);
				UpdateStats(useserver, v);

				if (v->menunum)
					Menu_Draw(cluster, v);
			}
			if (!v->menunum && v->server && v->server->parsingconnectiondata)
			{
				WriteByte(&m, svc_centerprint);
				WriteString(&m, v->server->status);
			}

			Netchan_Transmit(cluster, &v->netchan, m.cursize, m.data);

			if (!v->netchan.message.cursize && v->backbuffered)
			{//shift the backbuffers around
				memcpy(v->netchan.message.data, v->backbuf[0].data,  v->backbuf[0].cursize);
				v->netchan.message.cursize = v->backbuf[0].cursize;
				for (read = 0; read < v->backbuffered; read++)
				{
					if (read == v->backbuffered-1)
					{
						v->backbuf[read].cursize = 0;
					}
					else
					{
						memcpy(v->backbuf[read].data, v->backbuf[read+1].data,  v->backbuf[read+1].cursize);
						v->backbuf[read].cursize = v->backbuf[read+1].cursize;
					}
				}
			}
		}
	}
}
