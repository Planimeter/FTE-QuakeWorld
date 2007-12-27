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

#include "qtv.h"

#include "bsd_string.h"

#define MAX_INFO_KEY 64

//I apologise for this if it breaks your formatting or anything
#define HELPSTRING "\
FTEQTV proxy commands: (build "__DATE__")\n\
----------------------\n\
connect, qtv, addserver\n\
  connect to a MVD stream (TCP)\n\
qtvlist\n\
  lists available streams on a proxy\n\
qw\n\
  connect to a server as a player (UDP)\n\
adddemo\n\
  play a demo from a MVD file\n\
port\n\
  UDP port for QuakeWorld client connections\n\
mvdport\n\
  specify TCP port for MVD broadcasting\n\
maxviewers, maxproxies\n\
  limit number of connections\n\
status, choke, late, talking, nobsp, reconnect, exec, password, master, hostname, record, stop, quit\n\
  other random commands\n\
\n"





char *Info_ValueForKey (char *s, const char *key, char *buffer, int buffersize)
{
	char	pkey[1024];
	char	*o;

	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
			{
				*buffer='\0';
				return buffer;
			}
			*o++ = *s++;
			if (o+2 >= pkey+sizeof(pkey))	//hrm. hackers at work..
			{
				*buffer='\0';
				return buffer;
			}
		}
		*o = 0;
		s++;

		o = buffer;

		while (*s != '\\' && *s)
		{
			if (!*s)
			{
				*buffer='\0';
				return buffer;
			}
			*o++ = *s++;

			if (o+2 >= buffer+buffersize)	//hrm. hackers at work..
			{
				*buffer='\0';
				return buffer;
			}
		}
		*o = 0;

		if (!strcmp (key, pkey) )
			return buffer;

		if (!*s)
		{
			*buffer='\0';
			return buffer;
		}
		s++;
	}
}

void Info_RemoveKey (char *s, const char *key)
{
	char	*start;
	char	pkey[1024];
	char	value[1024];
	char	*o;

	if (strstr (key, "\\"))
	{
//		printf ("Key has a slash\n");
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey) )
		{
			strcpy (start, s);	// remove this part
			return;
		}

		if (!*s)
			return;
	}

}

void Info_SetValueForStarKey (char *s, const char *key, const char *value, int maxsize)
{
	char	newv[1024], *v;
	int		c;
#ifdef SERVERONLY
	extern cvar_t sv_highchars;
#endif

	if (strstr (key, "\\") || strstr (value, "\\") )
	{
//		printf ("Key has a slash\n");
		return;
	}

	if (strstr (key, "\"") || strstr (value, "\"") )
	{
//		printf ("Key has a quote\n");
		return;
	}

	if (strlen(key) >= MAX_INFO_KEY || strlen(value) >= MAX_INFO_KEY)
	{
//		printf ("Key or value is too long\n");
		return;
	}

	// this next line is kinda trippy
	if (*(v = Info_ValueForKey(s, key, newv, sizeof(newv))))
	{
		// key exists, make sure we have enough room for new value, if we don't,
		// don't change it!
		if (strlen(value) - strlen(v) + strlen(s) + 1 > maxsize)
		{
	//		Con_TPrintf (TL_INFOSTRINGTOOLONG);
			return;
		}
	}


	Info_RemoveKey (s, key);
	if (!value || !strlen(value))
		return;

	snprintf (newv, sizeof(newv)-1, "\\%s\\%s", key, value);

	if ((int)(strlen(newv) + strlen(s) + 1) > maxsize)
	{
//		printf ("info buffer is too small\n");
		return;
	}

	// only copy ascii values
	s += strlen(s);
	v = newv;
	while (*v)
	{
		c = (unsigned char)*v++;

//		c &= 127;		// strip high bits
		if (c > 13) // && c < 127)
			*s++ = c;
	}
	*s = 0;
}




#define DEFAULT_PUNCTUATION "(,{})(\':;=!><&|+"

char *COM_ParseToken (char *data, char *out, int outsize, const char *punctuation)
{
	int		c;
	int		len;

	if (!punctuation)
		punctuation = DEFAULT_PUNCTUATION;

	len = 0;
	out[0] = 0;

	if (!data)
		return NULL;

// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;			// end of file;
		data++;
	}

// skip // comments
	if (c=='/')
	{
		if (data[1] == '/')
		{
			while (*data && *data != '\n')
				data++;
			goto skipwhite;
		}
		else if (data[1] == '*')
		{
			data+=2;
			while (*data && (*data != '*' || data[1] != '/'))
				data++;
			data+=2;
			goto skipwhite;
		}
	}


// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			if (len >= outsize-1)
			{
				out[len] = '\0';
				return data;
			}
			c = *data++;
			if (c=='\"' || !c)
			{
				out[len] = 0;
				return data;
			}
			out[len] = c;
			len++;
		}
	}

// parse single characters
	if (strchr(punctuation, c))
	{
		out[len] = c;
		len++;
		out[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		if (len >= outsize-1)
			break;
		out[len] = c;
		data++;
		len++;
		c = *data;
		if (strchr(punctuation, c))
			break;
	} while (c>32);

	out[len] = 0;
	return data;
}




void Cmd_Printf(cmdctxt_t *ctx, char *fmt, ...)
{
	va_list		argptr;
	char		string[2048];
	
	va_start (argptr, fmt);
	vsnprintf (string, sizeof(string)-1, fmt,argptr);
	string[sizeof(string)-1] = 0;
	va_end (argptr);

	if (ctx->printfunc)
		ctx->printfunc(ctx, string);
	else if (ctx->qtv)
		QTV_Printf(ctx->qtv, "%s", string);
	else
		Sys_Printf(ctx->cluster, "%s", string);
}

void Cmd_Hostname(cmdctxt_t *ctx)
{
	if (Cmd_Argc(ctx) < 2)
	{
		if (*ctx->cluster->hostname)
			Cmd_Printf(ctx, "Current hostname is \"%s\"\n", ctx->cluster->hostname);
		else
			Cmd_Printf(ctx, "No master server is currently set.\n");
	}
	else
	{
		strlcpy(ctx->cluster->hostname, Cmd_Argv(ctx, 1), sizeof(ctx->cluster->hostname));

		Cmd_Printf(ctx, "hostname set to \"%s\"\n", ctx->cluster->hostname);
	}
}

void Cmd_Master(cmdctxt_t *ctx)
{
	char *newval = Cmd_Argv(ctx, 1);
	netadr_t addr;

	if (Cmd_Argc(ctx) < 2)
	{
		if (*ctx->cluster->master)
			Cmd_Printf(ctx, "Subscribed to a master server (use '-' to clear)\n");
		else
			Cmd_Printf(ctx, "No master server is currently set.\n");
		return;
	}

	if (!strcmp(newval, "-"))
	{
		strlcpy(ctx->cluster->master, "", sizeof(ctx->cluster->master));
		Cmd_Printf(ctx, "Master server cleared\n");
		return;
	}

	if (!NET_StringToAddr(newval, &addr, 27000))	//send a ping like a qw server does. this is kinda pointless of course.
	{
		Cmd_Printf(ctx, "Couldn't resolve address\n");
		return;
	}

	strlcpy(ctx->cluster->master, newval, sizeof(ctx->cluster->master));
	ctx->cluster->mastersendtime = ctx->cluster->curtime;

	if (ctx->cluster->qwdsocket != INVALID_SOCKET)
		NET_SendPacket (ctx->cluster, ctx->cluster->qwdsocket, 1, "k", addr);
	Cmd_Printf(ctx, "Master server set.\n");
}

void Cmd_UDPPort(cmdctxt_t *ctx)
{
	int news;
	int newp = atoi(Cmd_Argv(ctx, 1));
	news = QW_InitUDPSocket(newp);

	if (news != INVALID_SOCKET)
	{
		ctx->cluster->mastersendtime = ctx->cluster->curtime;
		closesocket(ctx->cluster->qwdsocket);
		ctx->cluster->qwdsocket = news;
		ctx->cluster->qwlistenportnum = newp;

		Cmd_Printf(ctx, "Opened udp port %i (all connected qw clients will time out)\n", newp);
	}
	else
		Cmd_Printf(ctx, "Failed to open udp port %i\n", newp);
}
void Cmd_AdminPassword(cmdctxt_t *ctx)
{
	if (!Cmd_IsLocal(ctx))
	{
		Cmd_Printf(ctx, "Rejecting remote password change.\n");
		return;
	}

	if (Cmd_Argc(ctx) < 2)
	{
		if (*ctx->cluster->adminpassword)
			Cmd_Printf(ctx, "An admin password is currently set\n");
		else
			Cmd_Printf(ctx, "No admin password is currently set\n");
	}
	else
	{
		strlcpy(ctx->cluster->adminpassword, Cmd_Argv(ctx, 1), sizeof(ctx->cluster->adminpassword));
		Cmd_Printf(ctx, "Password changed.\n");
	}
}

void Cmd_GenericQuery(cmdctxt_t *ctx, int dataset)
{
	char *method = "tcp:";
	char *address, *password;
	if (Cmd_Argc(ctx) < 2)
	{
		Cmd_Printf(ctx, "%s requires an ip:port parameter\n", Cmd_Argv(ctx, 0));
		return;
	}

	address = Cmd_Argv(ctx, 1);
	password = Cmd_Argv(ctx, 2);
	//this is evil
	/* Holy crap, Spike... Holy crap. */
	memmove(address+strlen(method), address, ARG_LEN-(1+strlen(method)));
	strncpy(address, method, strlen(method));

	if (!QTV_NewServerConnection(ctx->cluster, address, password, false, false, false, dataset))
		Cmd_Printf(ctx, "Failed to connect to \"%s\", connection aborted\n", address);

	Cmd_Printf(ctx, "Querying \"%s\"\n", address);
}


void Cmd_QTVList(cmdctxt_t *ctx)
{
	Cmd_GenericQuery(ctx, 1);
}
void Cmd_QTVDemoList(cmdctxt_t *ctx)
{
	Cmd_GenericQuery(ctx, 2);
}

void Cmd_GenericConnect(cmdctxt_t *ctx, char *method)
{
	sv_t *sv;
	char *address, *password;
	if (Cmd_Argc(ctx) < 2)
	{
		if (!strncmp(method, "file", 4))
			Cmd_Printf(ctx, "%s requires a demo name parameter\n", Cmd_Argv(ctx, 0));
		else
			Cmd_Printf(ctx, "%s requires an ip:port parameter\n", Cmd_Argv(ctx, 0));
		return;
	}

	address = Cmd_Argv(ctx, 1);
	password = Cmd_Argv(ctx, 2);
	//this is evil
	/* Holy crap, Spike... Holy crap. */
	memmove(address+strlen(method), address, ARG_LEN-(1+strlen(method)));
	strncpy(address, method, strlen(method));

	sv = QTV_NewServerConnection(ctx->cluster, address, password, false, false, false, false);
	if (!sv)
		Cmd_Printf(ctx, "Failed to connect to \"%s\", connection aborted\n", address);
	else
		Cmd_Printf(ctx, "Source registered \"%s\" as stream %i\n", address, sv->streamid);
}

void Cmd_QTVConnect(cmdctxt_t *ctx)
{
	Cmd_GenericConnect(ctx, "tcp:");
}
void Cmd_QWConnect(cmdctxt_t *ctx)
{
	Cmd_GenericConnect(ctx, "udp:");
}
void Cmd_MVDConnect(cmdctxt_t *ctx)
{
	Cmd_GenericConnect(ctx, "file:");
}

void Cmd_Exec(cmdctxt_t *ctx)
{
	FILE *f;
	char line[512], *l;
	char *fname = Cmd_Argv(ctx, 1);

	if (!Cmd_IsLocal(ctx))
	{
		if (*fname == '\\' || *fname == '/' || strstr(fname, "..") || fname[1] == ':')
		{
			Cmd_Printf(ctx, "Absolute paths are prohibited.\n");
			return;
		}
		if (!strncmp(fname, "usercfg/", 8))	//this is how we stop users from execing a 50gb pk3..
		{
			Cmd_Printf(ctx, "Remote-execed configs must be in the usercfg directory\n");
			return;
		}
	}

	f = fopen(fname, "rt");
	if (!f)
	{
		Cmd_Printf(ctx, "Couldn't exec \"%s\"\n", fname);
		return;
	}
	else
	{
		Cmd_Printf(ctx, "Execing \"%s\"\n", fname);
		while(fgets(line, sizeof(line)-1, f))
		{
			l = line;
			while(*(unsigned char*)l <= ' ' && *l)
				l++;
			if (*l && l[0] != '/' && l[1] != '/')
			{
				Cmd_ExecuteNow(ctx, l);
			}
		}
		fclose(f);
	}
}

void catbuffer(char *buffer, int bufsize, char *format, ...)
{
	va_list argptr;
	unsigned int buflen;

	buflen = strlen(buffer);

	va_start(argptr, format);
	vsnprintf(buffer + buflen, bufsize - buflen, format, argptr);
	va_end(argptr);
}

void Cmd_Say(cmdctxt_t *ctx)
{
	int i;
	viewer_t *v;
	char message[8192];
	message[0] = '\0';

	for (i = 1; i < Cmd_Argc(ctx); i++)
		catbuffer(message, sizeof(message)-1, "%s%s", i==1?"":" ", Cmd_Argv(ctx, i));

	if (ctx->qtv)
	{
		if (!SV_SayToUpstream(ctx->qtv, message))
			SV_SayToViewers(ctx->qtv, message);
	}
	else
	{
		//we don't have to remember the client proxies here... no streams = no active client proxies
		for (v = ctx->cluster->viewers; v; v = v->next)
		{
			QW_PrintfToViewer(v, "proxy: %s\n", message);
		}
	}

	Cmd_Printf(ctx, "proxy: %s\n", message);
}

void Cmd_Status(cmdctxt_t *ctx)
{
	Cmd_Printf(ctx, "QTV Status:\n");
	Cmd_Printf(ctx, " %i sources\n", ctx->cluster->numservers);
	Cmd_Printf(ctx, " %i viewers\n", ctx->cluster->numviewers);
	Cmd_Printf(ctx, " %i proxies\n", ctx->cluster->numproxies);

	Cmd_Printf(ctx, "Common Options:\n");
	Cmd_Printf(ctx, " Hostname %s\n", ctx->cluster->hostname);
	
	if (ctx->cluster->chokeonnotupdated)
		Cmd_Printf(ctx, " Choke\n");
	if (ctx->cluster->lateforward)
		Cmd_Printf(ctx, " Late forwarding\n");
	if (!ctx->cluster->notalking)
		Cmd_Printf(ctx, " Talking allowed\n");
	if (ctx->cluster->nobsp)
		Cmd_Printf(ctx, " No BSP loading\n");
	if (ctx->cluster->tcpsocket != INVALID_SOCKET)
	{
		Cmd_Printf(ctx, " tcp port %i\n", ctx->cluster->tcplistenportnum);
	}
	if (ctx->cluster->tcpsocket != INVALID_SOCKET)
	{
		Cmd_Printf(ctx, " udp port %i\n", ctx->cluster->qwlistenportnum);
	}
	Cmd_Printf(ctx, " user connections are %sallowed\n", ctx->cluster->nouserconnects?"NOT ":"");
	Cmd_Printf(ctx, "\n");


	if (ctx->qtv)
	{
		Cmd_Printf(ctx, "Selected server: %s\n", ctx->qtv->server);
		if (ctx->qtv->sourcefile)
			Cmd_Printf(ctx, " Playing from file\n");
		if (ctx->qtv->sourcesock != INVALID_SOCKET)
			Cmd_Printf(ctx, " Connected\n");
		if (ctx->qtv->parsingqtvheader || ctx->qtv->parsingconnectiondata)
			Cmd_Printf(ctx, " Waiting for gamestate\n");
		if (ctx->qtv->usequakeworldprotocols)
		{
			Cmd_Printf(ctx, " QuakeWorld protocols\n");
			if (ctx->qtv->controller)
			{
				Cmd_Printf(ctx, " Controlled by %s\n", ctx->qtv->controller->name);
			}
		}
		else if (ctx->qtv->sourcesock == INVALID_SOCKET && !ctx->qtv->sourcefile)
			Cmd_Printf(ctx, " Connection not established\n");

		if (*ctx->qtv->modellist[1].name)
		{
			Cmd_Printf(ctx, " Map name %s\n", ctx->qtv->modellist[1].name);
		}
		if (*ctx->qtv->connectpassword)
			Cmd_Printf(ctx, " Using a password\n");

		if (ctx->qtv->errored == ERR_DISABLED)
			Cmd_Printf(ctx, " Stream is disabled\n");

		if (ctx->qtv->disconnectwhennooneiswatching)
			Cmd_Printf(ctx, " Stream is temporary\n");

/*		if (ctx->qtv->tcpsocket != INVALID_SOCKET)
		{
			Cmd_Printf(ctx, " Listening for proxies (%i)\n", ctx->qtv->tcplistenportnum);
		}
*/

		if (ctx->qtv->bsp)
		{
			Cmd_Printf(ctx, " BSP (%s) is loaded\n", ctx->qtv->mapname);
		}
	}

}

void Cmd_UserConnects(cmdctxt_t *ctx)
{
	if (Cmd_Argc(ctx) < 2)
	{
		Cmd_Printf(ctx, "userconnects is set to %i\n", !ctx->cluster->nouserconnects);
	}
	else
	{
		ctx->cluster->nouserconnects = !atoi(Cmd_Argv(ctx, 1));
		Cmd_Printf(ctx, "userconnects is now %i\n", !ctx->cluster->nouserconnects);
	}
}
void Cmd_Choke(cmdctxt_t *ctx)
{
	if (Cmd_Argc(ctx) < 2)
	{
		if (ctx->cluster->chokeonnotupdated)
			Cmd_Printf(ctx, "proxy will not interpolate packets\n");
		else
			Cmd_Printf(ctx, "proxy will smooth action at the expense of extra packets\n");
		return;
	}
	ctx->cluster->chokeonnotupdated = !!atoi(Cmd_Argv(ctx, 1));
	Cmd_Printf(ctx, "choke-until-update set to %i\n", ctx->cluster->chokeonnotupdated);
}
void Cmd_Late(cmdctxt_t *ctx)
{
	if (Cmd_Argc(ctx) < 2)
	{
		if (ctx->cluster->lateforward)
			Cmd_Printf(ctx, "forwarded streams will be artificially delayed\n");
		else
			Cmd_Printf(ctx, "forwarded streams are forwarded immediatly\n");
		return;
	}
	ctx->cluster->lateforward = !!atoi(Cmd_Argv(ctx, 1));
	Cmd_Printf(ctx, "late forwarding set\n");
}
void Cmd_Talking(cmdctxt_t *ctx)
{
	if (Cmd_Argc(ctx) < 2)
	{
		if (ctx->cluster->notalking)
			Cmd_Printf(ctx, "viewers may not talk\n");
		else
			Cmd_Printf(ctx, "viewers may talk freely\n");
		return;
	}
	ctx->cluster->notalking = !atoi(Cmd_Argv(ctx, 1));
	Cmd_Printf(ctx, "talking permissions set\n");
}
void Cmd_NoBSP(cmdctxt_t *ctx)
{
	char *val = Cmd_Argv(ctx, 1);
	if (!*val)
	{
		if (ctx->cluster->nobsp)
			Cmd_Printf(ctx, "no bsps will be loaded\n");
		else
			Cmd_Printf(ctx, "attempting to load bsp files\n");
	}
	else
	{
		ctx->cluster->nobsp = !!atoi(val);
		Cmd_Printf(ctx, "nobsp will change at start of next map\n");
	}
}

void Cmd_MaxViewers(cmdctxt_t *ctx)
{
	char *val = Cmd_Argv(ctx, 1);
	if (!*val)
	{
		if (ctx->cluster->maxviewers)
			Cmd_Printf(ctx, "maxviewers is currently %i\n", ctx->cluster->maxviewers);
		else
			Cmd_Printf(ctx, "maxviewers is currently unlimited\n");
	}
	else
	{
		ctx->cluster->maxviewers = atoi(val);
		Cmd_Printf(ctx, "maxviewers set\n");
	}
}
void Cmd_AllowNQ(cmdctxt_t *ctx)
{
	char *val = Cmd_Argv(ctx, 1);
	if (!*val)
	{
		Cmd_Printf(ctx, "allownq is currently %i\n", ctx->cluster->allownqclients);
	}
	else
	{
		ctx->cluster->allownqclients = !!atoi(val);
		Cmd_Printf(ctx, "allownq set\n");
	}
}
void Cmd_MaxProxies(cmdctxt_t *ctx)
{
	char *val = Cmd_Argv(ctx, 1);
	if (!*val)
	{
		if (ctx->cluster->maxproxies)
			Cmd_Printf(ctx, "maxproxies is currently %i\n", ctx->cluster->maxproxies);
		else
			Cmd_Printf(ctx, "maxproxies is currently unlimited\n");
	}
	else
	{
		ctx->cluster->maxproxies = atoi(val);
		Cmd_Printf(ctx, "maxproxies set\n");
	}
}


void Cmd_Ping(cmdctxt_t *ctx)
{
	netadr_t addr;
	char *val = Cmd_Argv(ctx, 1);
	if (NET_StringToAddr(val, &addr, 27500))
	{
		NET_SendPacket (ctx->cluster, ctx->cluster->qwdsocket, 1, "k", addr);
		Cmd_Printf(ctx, "pinged\n");
	}
	Cmd_Printf(ctx, "couldn't resolve\n");
}

void Cmd_Help(cmdctxt_t *ctx)
{
	Cmd_Printf(ctx, HELPSTRING);
}

void Cmd_Echo(cmdctxt_t *ctx)
{
	Cmd_Printf(ctx, "%s", Cmd_Argv(ctx, 1));
}
	
void Cmd_Quit(cmdctxt_t *ctx)
{
	if (!Cmd_IsLocal(ctx))
		Cmd_Printf(ctx, "Remote shutdown refused.\n");
	ctx->cluster->wanttoexit = true;
	Cmd_Printf(ctx, "Shutting down.\n");
}

















void Cmd_Streams(cmdctxt_t *ctx)
{
	sv_t *qtv;
	char *status;
	Cmd_Printf(ctx, "Streams:\n");

	for (qtv = ctx->cluster->servers; qtv; qtv = qtv->next)
	{
		switch (qtv->errored)
		{
		case ERR_NONE:
			if (qtv->controller)
				status = " (player controlled)";
			else if (qtv->parsingconnectiondata)
				status = " (connecting)";
			else
				status = "";
			break;
		case ERR_DISABLED:
			status = " (disabled)";
			break;
		case ERR_DROP:	//a user should never normally see this, but there is a chance
			status = " (dropping)";
			break;
		case ERR_RECONNECT:	//again, rare
			status = " (reconnecting)";
			break;
		default:	//some other kind of error, transitioning
			status = " (errored)";
			break;
		}
		Cmd_Printf(ctx, "%i: %s%s\n", qtv->streamid, qtv->server, status);

		if (qtv->upstreamacceptschat)
			Cmd_Printf(ctx, "  (dbg) can chat!\n");
	}
}




void Cmd_DemoSpeed(cmdctxt_t *ctx)
{
	char *val = Cmd_Argv(ctx, 1);
	if (*val)
	{
		ctx->qtv->parsespeed = atof(val)*1000;
		Cmd_Printf(ctx, "Setting demo speed to %f\n", ctx->qtv->parsespeed/1000.0f);
	}
	else
		Cmd_Printf(ctx, "Playing demo at %f speed\n", ctx->qtv->parsespeed/1000.0f);
}

void Cmd_Disconnect(cmdctxt_t *ctx)
{
	QTV_Shutdown(ctx->qtv);
	Cmd_Printf(ctx, "Disconnected\n");
}

void Cmd_Halt(cmdctxt_t *ctx)
{
	if (ctx->qtv->errored == ERR_DISABLED || ctx->qtv->errored == ERR_PERMANENT)
	{
		Cmd_Printf(ctx, "Stream is already halted\n");
	}
	else
	{
		ctx->qtv->errored = ERR_PERMANENT;
		Cmd_Printf(ctx, "Stream will disconnect\n");
	}
}
void Cmd_Resume(cmdctxt_t *ctx)
{
	if (ctx->qtv->errored == ERR_NONE)
		Cmd_Printf(ctx, "Stream is already functional\n");

	ctx->qtv->errored = ERR_RECONNECT;
	Cmd_Printf(ctx, "Stream will attempt to reconnect\n");
}

void Cmd_Record(cmdctxt_t *ctx)
{
	char *fname = Cmd_Argv(ctx, 1);
	if (!*fname)
		Cmd_Printf(ctx, "record requires a filename on the proxy's machine\n");

	if (!Cmd_IsLocal(ctx))
	{
		if (*fname == '\\' || *fname == '/' || strstr(fname, "..") || fname[1] == ':')
		{
			Cmd_Printf(ctx, "Absolute paths are prohibited.\n");
			return;
		}
	}

	if (Net_FileProxy(ctx->qtv, fname))
		Cmd_Printf(ctx, "Recording to disk\n");
	else
		Cmd_Printf(ctx, "Failed to open file\n");
}
void Cmd_Stop(cmdctxt_t *ctx)
{
	if (Net_StopFileProxy(ctx->qtv))
		Cmd_Printf(ctx, "stopped\n");
	else
		Cmd_Printf(ctx, "not recording to disk\n");
}

void Cmd_Reconnect(cmdctxt_t *ctx)
{
	if (ctx->qtv->disconnectwhennooneiswatching == 2)
		Cmd_Printf(ctx, "Stream is a reverse connection (command rejected)\n");
	else if (QTV_Connect(ctx->qtv, ctx->qtv->server))
		Cmd_Printf(ctx, "Reconnected\n");
	else
		Cmd_Printf(ctx, "Failed to reconnect (will keep trying)\n");
}

void Cmd_MVDPort(cmdctxt_t *ctx)
{
	char *val = Cmd_Argv(ctx, 1);
	int news;
	int newp = atoi(val);

	if (!newp)
	{
		if (ctx->cluster->tcpsocket != INVALID_SOCKET)
		{
			closesocket(ctx->cluster->tcpsocket);
			ctx->cluster->tcpsocket = INVALID_SOCKET;
			ctx->cluster->tcplistenportnum = 0;

			Cmd_Printf(ctx, "mvd port is now closed\n");
		}
		else
			Cmd_Printf(ctx, "Already closed\n");
	}
	else
	{
		news = Net_MVDListen(newp);

		if (news != INVALID_SOCKET)
		{
			if (ctx->cluster->tcpsocket != INVALID_SOCKET)
				closesocket(ctx->cluster->tcpsocket);
			ctx->cluster->tcpsocket = news;
			ctx->cluster->tcplistenportnum = newp;

			Cmd_Printf(ctx, "Opened tcp port %i\n", newp);
		}
		else
			Cmd_Printf(ctx, "Failed to open tcp port %i\n", newp);
	}
}

void Cmd_DemoList(cmdctxt_t *ctx)
{
	int i;
	int count;
	Cluster_BuildAvailableDemoList(ctx->cluster);

	count = ctx->cluster->availdemoscount;
	Cmd_Printf(ctx, "%i demos\n", count);
	for (i = 0; i < count; i++)
	{
		Cmd_Printf(ctx, " %7i %s\n", ctx->cluster->availdemos[i].size, ctx->cluster->availdemos[i].name);
	}
}

void Cmd_BaseDir(cmdctxt_t *ctx)
{
	char *val;
	val = Cmd_Argv(ctx, 1);
	if (!Cmd_IsLocal(ctx))
		Cmd_Printf(ctx, "Sorry, you may not use this command remotly\n");

	if (*val)
		chdir(val);
	else
	{
		char buffer[256];
		val = getcwd(buffer, sizeof(buffer));
		if (val)
			Cmd_Printf(ctx, "basedir is: %s\n", val);
		else
			Cmd_Printf(ctx, "system error getting basedir\n");
	}
}

void Cmd_DemoDir(cmdctxt_t *ctx)
{
	char *val;
	val = Cmd_Argv(ctx, 1);

	if (*val)
	{
		if (!Cmd_IsLocal(ctx))
		{
			Cmd_Printf(ctx, "Sorry, but I don't trust this code that well!\n");
			return;
		}
		while (*val > 0 &&*val <= ' ')
			val++;

		if (strchr(val, '.') || strchr(val, ':') || *val == '/')
			Cmd_Printf(ctx, "Rejecting path\n");
		else
		{
			strlcpy(ctx->cluster->demodir, val, sizeof(ctx->cluster->demodir));
			Cmd_Printf(ctx, "Changed demo dir to \"%s\"\n", ctx->cluster->demodir);
		}
	}
	else
	{
		Cmd_Printf(ctx, "Current demo directory is \"%s\"\n", ctx->cluster->demodir);
	}
}

void Cmd_MuteStream(cmdctxt_t *ctx)
{
	char *val;
	val = Cmd_Argv(ctx, 1);
	if (*val)
	{
		ctx->qtv->silentstream = atoi(val);
		Cmd_Printf(ctx, "Stream is now %smuted\n", ctx->qtv->silentstream?"":"un");
	}
	else
		Cmd_Printf(ctx, "Stream is currently %smuted\n", ctx->qtv->silentstream?"":"un");
}

#ifdef VIEWER
void Cmd_Watch(cmdctxt_t *ctx)
{
	if (!localcommand)
	{
		Cmd_Printf(ctx, "watch is not permitted remotly\n");
	}

	if (cluster->viewserver == qtv)
	{
		cluster->viewserver = NULL;
		Cmd_Printf(ctx, "Stopped watching\n");
	}
	else
	{
		cluster->viewserver = qtv;
		Cmd_Printf(ctx, "Watching\n");
	}
}
#endif



typedef struct rconcommands_s {
	char *name;
	qboolean serverspecific;	//works within a qtv context
	qboolean clusterspecific;	//works without a qtv context (ignores context)
	consolecommand_t func;
	char *description;
} rconcommands_t;

extern const rconcommands_t rconcommands[];

void Cmd_Commands(cmdctxt_t *ctx)
{
	rconcommands_t *cmd;
	consolecommand_t lastfunc = NULL;

	Cmd_Printf(ctx, "Commands:\n");
	for (cmd = rconcommands; cmd->name; cmd = cmd++)
	{
		if (cmd->func == lastfunc)
			continue;	//no spamming alternative command names

		Cmd_Printf(ctx, "%s: %s\n", cmd->name, cmd->description?cmd->description:"no description available");
		lastfunc = cmd->func;
	}
}

const rconcommands_t rconcommands[] =
{
	{"exec",		1, 1, Cmd_Exec,		"executes a config file"},
	{"status",		1, 1, Cmd_Status,	"prints proxy/stream status" },
	{"say",			1, 1, Cmd_Say,		"says to a stream"},

	{"help",		0, 1, Cmd_Help,		"shows the brief intro help text"},
	{"commands",		0, 1, Cmd_Commands,	"prints the list of commands"},
	{"hostname",		0, 1, Cmd_Hostname,	"changes the hostname seen in server browsers"},
	{"master",		0, 1, Cmd_Master,	"specifies which master server to use"},
	{"udpport",		0, 1, Cmd_UDPPort,	"specifies to listen on a provided udp port for regular qw clients"},
	 {"port",		0, 1, Cmd_UDPPort},
	{"adminpassword",	0, 1, Cmd_AdminPassword,"specifies the password for qtv administrators"},
	 {"rconpassword",	0, 1, Cmd_AdminPassword},
	{"qtvlist",		0, 1, Cmd_QTVList,	"queries a seperate proxy for a list of available streams"},
	{"qtvdemolist",		0, 1, Cmd_QTVDemoList,	"queries a seperate proxy for a list of available demos"},
	{"qtv",			0, 1, Cmd_QTVConnect,	"adds a new tcp/qtv stream"},
	 {"addserver",		0, 1, Cmd_QTVConnect},
	 {"connect",		0, 1, Cmd_QTVConnect},
	{"qw",			0, 1, Cmd_QWConnect,	"adds a new udp/qw stream"},
	 {"observe",		0, 1, Cmd_QWConnect},
	{"demos",		0, 1, Cmd_DemoList,	"shows the list of demos available on this proxy"},
	{"demo",		0, 1, Cmd_MVDConnect,	"adds a demo as a new stream"},
	 {"playdemo",		0, 1, Cmd_MVDConnect},
	{"choke",		0, 1, Cmd_Choke,	"chokes packets to the data rate in the stream, disables proxy-side interpolation"},
	{"late",		0, 1, Cmd_Late,		"enforces a time delay on packets sent through this proxy"},
	{"talking",		0, 1, Cmd_Talking,	"permits viewers to talk to each other"},
	{"nobsp",		0, 1, Cmd_NoBSP,	"disables loading of bsp files"},
	{"userconnects",	0, 1, Cmd_UserConnects,	"prevents users from creating thier own streams"},
	{"maxviewers",		0, 1, Cmd_MaxViewers,	"sets a limit on udp/qw client connections"},
	{"maxproxies",		0, 1, Cmd_MaxProxies,	"sets a limit on tcp/qtv client connections"},
	{"demodir",		0, 1, Cmd_DemoDir,	"specifies where to get the demo list from"},
	{"basedir",		0, 1, Cmd_BaseDir,	"specifies where to get any files required by the game. this is prefixed to the server-specified game dir."},
	{"ping",		0, 1, Cmd_Ping,		"sends a udp ping to a qtv proxy or server"},
	{"reconnect",		0, 1, Cmd_Reconnect,	"forces a stream to reconnect to its server (restarts demos)"},
	{"echo",		0, 1, Cmd_Echo,		"a useless command that echos a string"},
	{"quit",		0, 1, Cmd_Quit,		"closes the qtv"},
	{"exit",		0, 1, Cmd_Quit},
	{"streams",		0, 1, Cmd_Streams,	"shows a list of active streams"},
	{"allownq",		0, 1, Cmd_AllowNQ,	"permits nq clients to connect. This can be disabled as this code is less tested than the rest"},



	{"halt",		1, 0, Cmd_Halt,		"disables a stream, preventing it from reconnecting until someone tries watching it anew"},
	{"disable",		1, 0, Cmd_Halt},
	{"resume",		1, 0, Cmd_Resume,	"reactivates a stream, allowing it to reconnect"},
	{"enable",		1, 0, Cmd_Resume},
	{"mute",		1, 0, Cmd_MuteStream,	"hides prints that come from the game server"},
	{"mutestream",		1, 0, Cmd_MuteStream},
	{"disconnect",		1, 0, Cmd_Disconnect,	"fully closes a stream"},
	{"record",		1, 0, Cmd_Record,	"records a stream to a demo"},
	{"stop",		1, 0, Cmd_Stop,		"stops recording of a demo"},
	{"demospeed",		1, 0, Cmd_DemoSpeed,	"changes the rate the demo is played at"},
	{"tcpport",		0, 1, Cmd_MVDPort,	"specifies which port to listen on for tcp/qtv connections"},
	 {"mvdport",		0, 1, Cmd_MVDPort},

#ifdef VIEWER
	{"watch",		1, 0, Cmd_Watch,	"specifies to watch that stream in the built-in viewer"},
#endif
	 
	{NULL}
};

void Cmd_ExecuteNow(cmdctxt_t *ctx, char *command)
{
#define TOKENIZE_PUNCTUATION ""

	int i;
	char arg[MAX_ARGS][ARG_LEN];
	char *sid;
	char *cmdname;

	for (sid = command; *sid; sid++)
	{
		if (*sid == ':')
			break;
		if (*sid < '0' || *sid > '9')
			break;
	}
	if (*sid == ':')
	{
		i = atoi(command);
		command = sid+1;

		for (ctx->qtv = ctx->cluster->servers; ctx->qtv; ctx->qtv = ctx->qtv->next)
			if (ctx->qtv->streamid == i)
				break;
	}



	ctx->argc = 0;
	for (i = 0; i < MAX_ARGS; i++)
	{
		command = COM_ParseToken(command, arg[i], ARG_LEN, TOKENIZE_PUNCTUATION);
		ctx->arg[i] = arg[i];
		if (command)
			ctx->argc++;
	}

	cmdname = Cmd_Argv(ctx, 0);

	//if there's only one stream, set that as the selected stream
	if (!ctx->qtv && ctx->cluster->numservers==1)
		ctx->qtv = ctx->cluster->servers;

	if (ctx->qtv)
	{	//if there is a specific connection targetted

		for (i = 0; rconcommands[i].name; i++)
		{
			if (rconcommands[i].serverspecific)
				if (!strcmp(rconcommands[i].name, cmdname))
				{
					rconcommands[i].func(ctx);
					return;
				}
		}
	}

	for (i = 0; rconcommands[i].name; i++)
	{
		if (!strcmp(rconcommands[i].name, cmdname))
		{
			if (rconcommands[i].clusterspecific)
			{
				rconcommands[i].func(ctx);
				return;
			}
			else if (rconcommands[i].serverspecific)
			{
				Cmd_Printf(ctx, "Command \"%s\" requires a targeted server.\n", cmdname);
				return;
			}
		}
	}


	Cmd_Printf(ctx, "Command \"%s\" not recognised.\n", cmdname);
}

void Rcon_PrintToBuffer(cmdctxt_t *ctx, char *msg)
{
	if (ctx->printcookiesize < 1)
		return;
	while (ctx->printcookiesize>2 && *msg)
	{
		ctx->printcookiesize--;
		*(char*)ctx->printcookie = *msg++;
		ctx->printcookie = ((char*)ctx->printcookie)+1;
	}

	ctx->printcookiesize--;
	*(char*)ctx->printcookie = 0;
}

char *Rcon_Command(cluster_t *cluster, sv_t *source, char *command, char *resultbuffer, int resultbuffersize, int islocalcommand)
{
	cmdctxt_t ctx;
	ctx.cluster = cluster;
	ctx.qtv = source;
	ctx.argc = 0;
	ctx.printfunc = Rcon_PrintToBuffer;
	ctx.printcookie = resultbuffer;
	ctx.printcookiesize = resultbuffersize;
	ctx.localcommand = islocalcommand;
	*(char*)ctx.printcookie = 0;
	Cmd_ExecuteNow(&ctx, command);

	return resultbuffer;
}
