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

#include "quakedef.h"
#ifndef CLIENTONLY

#include "winquake.h"

#include "netinc.h"


void SV_MVDStop_f (void);

#define demo_size_padding 0x1000

static void SV_DemoDir_Callback(struct cvar_s *var, char *oldvalue);

cvar_t	sv_demoUseCache = CVARD("sv_demoUseCache", "0", "If set, demo data will be flushed only periodically");
cvar_t	sv_demoCacheSize = CVAR("sv_demoCacheSize", "0x80000"); //half a meg
cvar_t	sv_demoMaxDirSize = CVAR("sv_demoMaxDirSize", "102400");	//so ktpro autorecords.
cvar_t	sv_demoDir = CVARC("sv_demoDir", "demos", SV_DemoDir_Callback);
cvar_t	sv_demofps = CVAR("sv_demofps", "30");
cvar_t	sv_demoPings = CVARD("sv_demoPings", "10", "Interval between ping updates in mvds");
cvar_t	sv_demoMaxSize = CVARD("sv_demoMaxSize", "", "Demos will be truncated to be no larger than this size.");
cvar_t	sv_demoExtraNames = CVAR("sv_demoExtraNames", "");
cvar_t	sv_demoExtensions = CVARD("sv_demoExtensions", "0", "Enables protocol extensions within MVDs. This will cause older/non-fte clients to error upon playback");

cvar_t qtv_password		= CVAR(		"qtv_password", "");
cvar_t qtv_streamport	= CVARAF(	"qtv_streamport", "0",
									"mvd_streamport", 0);
cvar_t qtv_maxstreams	= CVARAF(	"qtv_maxstreams", "1",
									"mvd_maxstreams",  0);

cvar_t			sv_demoPrefix = CVAR("sv_demoPrefix", "");
cvar_t			sv_demoSuffix = CVAR("sv_demoSuffix", "");
cvar_t			sv_demotxt = CVAR("sv_demotxt", "1");

void SV_WriteMVDMessage (sizebuf_t *msg, int type, int to, float time);
void SV_WriteRecordMVDMessage (sizebuf_t *msg);

demo_t			demo;
static float			demo_prevtime;
//static dbuffer_t	*demobuffer;
//static int	header = (char *)&((header_t*)0)->data - (char *)NULL;
static sizebuf_t demomsg;
int demomsgtype;
int demomsgto;
static char demomsgbuf[MAX_OVERALLMSGLEN];

mvddest_t *singledest;

mvddest_t *SV_InitStream(int socket);
qboolean SV_MVD_Record (mvddest_t *dest);
char *SV_MVDName2Txt(char *name);
extern cvar_t qtv_password;

//does not unlink.
void DestClose(mvddest_t *d, qboolean destroyfiles)
{
	char path[MAX_OSPATH];

	if (d->cache)
		BZ_Free(d->cache);
	if (d->file)
		VFS_CLOSE(d->file);
#ifdef HAVE_TCP
	if (d->socket)
		closesocket(d->socket);
#endif

	if (destroyfiles)
	{
		snprintf(path, MAX_OSPATH, "%s/%s", d->path, d->name);
		FS_Remove(path, FS_GAMEONLY);

		FS_Remove(SV_MVDName2Txt(path), FS_GAMEONLY);
	}

	Z_Free(d);
}

void DestFlush(qboolean compleate)
{
	int len;
	mvddest_t *d, *t;

	if (!demo.dest)
		return;
	while (demo.dest->error)
	{
		d = demo.dest;
		demo.dest = d->nextdest;

		DestClose(d, false);

		if (!demo.dest)
		{
			SV_MVDStop(3, false);
			return;
		}
	}
	for (d = demo.dest; d; d = d->nextdest)
	{
		switch(d->desttype)
		{
		case DEST_FILE:
			VFS_FLUSH (d->file);
			break;
		case DEST_BUFFEREDFILE:
			if (d->cacheused+demo_size_padding > d->maxcachesize || compleate)
			{
				len = VFS_WRITE(d->file, d->cache, d->cacheused);
				if (len < d->cacheused)
					d->error = true;
				VFS_FLUSH(d->file);

				d->cacheused = 0;
			}
			break;

		case DEST_STREAM:
#ifndef HAVE_TCP
			d->error = true;
#else
			if (d->cacheused && !d->error)
			{
				len = send(d->socket, d->cache, d->cacheused, 0);
				if (len == 0) //client died
					d->error = true;
				else if (len > 0)	//we put some data through
				{	//move up the buffer
					d->cacheused -= len;
					memmove(d->cache, d->cache+len, d->cacheused);
				}
				else
				{	//error of some kind. would block or something
					int e;
					e = neterrno();
					if (e != NET_EWOULDBLOCK)
						d->error = true;
				}
			}
#endif
			break;

		case DEST_NONE:
			Sys_Error("DestFlush encoundered bad dest.");
		}

		if (sv_demoMaxSize.value && d->totalsize > sv_demoMaxSize.value*1024)
			d->error = 2;	//abort, but don't kill it.

		while (d->nextdest && d->nextdest->error)
		{
			t = d->nextdest;
			d->nextdest = t->nextdest;

			DestClose(t, false);
		}
	}
}

void SV_MVD_RunPendingConnections(void)
{
#ifndef HAVE_TCP
	if (demo.pendingdest)
		Sys_Error("demo.pendingdest not null");
#else
	unsigned short ushort_result;
	char *e;
	int len;
	mvdpendingdest_t *p;
	mvdpendingdest_t *np;

	if (!demo.pendingdest)
		return;

	while (demo.pendingdest && demo.pendingdest->error)
	{
		np = demo.pendingdest->nextdest;

		if (demo.pendingdest->socket != INVALID_SOCKET)
			closesocket(demo.pendingdest->socket);
		Z_Free(demo.pendingdest);
		demo.pendingdest = np;
	}

	for (p = demo.pendingdest; p && p->nextdest; p = p->nextdest)
	{
		if (p->nextdest->error)
		{
			np = p->nextdest->nextdest;
			if (p->nextdest->socket != INVALID_SOCKET)
				closesocket(p->nextdest->socket);
			Z_Free(p->nextdest);
			p->nextdest = np;
		}
	}

	for (p = demo.pendingdest; p; p = p->nextdest)
	{
		if (p->outsize && !p->error)
		{
			len = send(p->socket, p->outbuffer, p->outsize, 0);
			if (len == 0) //client died
				p->error = true;
			else if (len > 0)	//we put some data through
			{	//move up the buffer
				p->outsize -= len;
				memmove(p->outbuffer, p->outbuffer+len, p->outsize );
			}
			else
			{	//error of some kind. would block or something
				int e;
				e = neterrno();
				if (e != NET_EWOULDBLOCK)
					p->error = true;
			}
		}
		if (!p->error)
		{
			len = recv(p->socket, p->inbuffer + p->insize, sizeof(p->inbuffer) - p->insize - 1, 0);
			if (len > 0)
			{//fixme: cope with extra \rs
				char *end;
				p->insize += len;
				p->inbuffer[p->insize] = 0;

				for (end = p->inbuffer; ; end++)
				{
					if (*end == '\0')
					{
						end = NULL;
						break;	//not enough data
					}

					if (end[0] == '\n')
					{
						if (end[1] == '\n')
						{
							end[1] = '\0';
							break;
						}
					}
				}
				if (end)
				{	//we found the end of the header
					qboolean server = false;
					char *start, *lineend;
					int versiontouse = 0;
					int raw = 0;
					char password[256] = "";
					enum {
						QTVAM_NONE,
						QTVAM_PLAIN,
						QTVAM_CCITT,
						QTVAM_MD4,
						QTVAM_MD5,
					} authmethod = QTVAM_NONE;

					start = p->inbuffer;

					lineend = strchr(start, '\n');
					if (!lineend)
					{
//						char *e;
//						e =	"This is a QTV server.";
//						send(p->socket, e, strlen(e), 0);

						p->error = true;
						continue;
					}
					*lineend = '\0';
					COM_ParseToken(start, NULL);
					start = lineend+1;
					if (strcmp(com_token, "QTV"))
					{	//it's an error if it's not qtv.
						if (!strcmp(com_token, "QTVSV"))
							server = true;
						else
						{
							p->error = true;
							lineend = strchr(start, '\n');
							continue;
						}
					}

					if (server != p->isreverse)
					{	//just a small check
						p->error = true;
						return;
					}

					for(;;)
					{
						lineend = strchr(start, '\n');
						if (!lineend)
							break;
						*lineend = '\0';
						start = COM_ParseToken(start, NULL);
						if (*start == ':')
						{
//VERSION: a list of the different qtv protocols supported. Multiple versions can be specified. The first is assumed to be the prefered version.
//RAW: if non-zero, send only a raw mvd with no additional markup anywhere (for telnet use). Doesn't work with challenge-based auth, so will only be accepted when proxy passwords are not required.
//AUTH: specifies an auth method, the exact specs varies based on the method
//		PLAIN: the password is sent as a PASSWORD line
//		MD4: the server responds with an "AUTH: MD4\n" line as well as a "CHALLENGE: somerandomchallengestring\n" line, the client sends a new 'initial' request with CHALLENGE: MD4\nRESPONSE: hexbasedmd4checksumhere\n"
//		MD5: same as md4
//		CCITT: same as md4, but using the CRC stuff common to all quake engines.
//		if the supported/allowed auth methods don't match, the connection is silently dropped.
//SOURCE: which stream to play from, DEFAULT is special. Without qualifiers, it's assumed to be a tcp address.
//COMPRESSION: Suggests a compression method (multiple are allowed). You'll get a COMPRESSION response, and compression will begin with the binary data.

							start = start+1;
							while(*start == ' ' || *start == '\t')
								start++;
							Con_Printf("qtv, got (%s) (%s)\n", com_token, start);
							if (!strcmp(com_token, "VERSION"))
							{
								start = COM_ParseToken(start, NULL);
								if (atoi(com_token) == 1)
									versiontouse = 1;
							}
							else if (!strcmp(com_token, "RAW"))
							{
								start = COM_ParseToken(start, NULL);
								raw = atoi(com_token);
							}
							else if (!strcmp(com_token, "PASSWORD"))
							{
								start = COM_ParseToken(start, NULL);
								Q_strncpyz(password, com_token, sizeof(password));
							}
							else if (!strcmp(com_token, "AUTH"))
							{
								int thisauth;
								start = COM_ParseToken(start, NULL);
								if (!strcmp(com_token, "NONE"))
									thisauth = QTVAM_PLAIN;
								else if (!strcmp(com_token, "PLAIN"))
									thisauth = QTVAM_PLAIN;
								else if (!strcmp(com_token, "CCIT"))
									thisauth = QTVAM_CCITT;
								else if (!strcmp(com_token, "MD4"))
									thisauth = QTVAM_MD4;
//								else if (!strcmp(com_token, "MD5"))
//									thisauth = QTVAM_MD5;
								else
								{
									thisauth = QTVAM_NONE;
									Con_DPrintf("qtv: received unrecognised auth method (%s)\n", com_token);
								}

								if (authmethod < thisauth)
									authmethod = thisauth;
							}
							else if (!strcmp(com_token, "SOURCE"))
							{
								//servers don't support source, and ignore it.
								//source is only useful for qtv proxy servers.
							}
							else if (!strcmp(com_token, "COMPRESSION"))
							{
								//compression not supported yet
							}
							else
							{
								//not recognised.
							}
						}
						start = lineend+1;
					}

					len = (end - p->inbuffer)+2;
					p->insize -= len;
					memmove(p->inbuffer, p->inbuffer + len, p->insize);
					p->inbuffer[p->insize] = 0;

					e = NULL;
					if (p->hasauthed)
					{
					}
					else if (p->isreverse)
						p->hasauthed = true;	//reverse connections do not need to auth.
					else if (!*qtv_password.string)
						p->hasauthed = true;	//no password, no need to auth.
					else if (*password)
					{
						switch (authmethod)
						{
						case QTVAM_NONE:
							e = ("QTVSV 1\n"
								 "PERROR: You need to provide a password.\n\n");
							break;
						case QTVAM_PLAIN:
							p->hasauthed = !strcmp(qtv_password.string, password);
							break;
						case QTVAM_CCITT:
							QCRC_Init(&ushort_result);
							QCRC_AddBlock(&ushort_result, p->challenge, strlen(p->challenge));
							QCRC_AddBlock(&ushort_result, qtv_password.string, strlen(qtv_password.string));
							p->hasauthed = (ushort_result == strtoul(password, NULL, 0));
							break;
						case QTVAM_MD4:
							{
								char hash[512];
								int md4sum[4];

								snprintf(hash, sizeof(hash), "%s%s", p->challenge, qtv_password.string);
								Com_BlockFullChecksum (hash, strlen(hash), (unsigned char*)md4sum);
								sprintf(hash, "%X%X%X%X", md4sum[0], md4sum[1], md4sum[2], md4sum[3]);
								p->hasauthed = !strcmp(password, hash);
							}
							break;
						case QTVAM_MD5:
						default:
							e = ("QTVSV 1\n"
								 "PERROR: FTEQWSV bug detected.\n\n");
							break;
						}
						if (!p->hasauthed && !e)
						{
							if (raw)
								e = "";
							else
								e =	("QTVSV 1\n"
									 "PERROR: Bad password.\n\n");
						}
					}
					else
					{
						//no password, and not automagically authed
						switch (authmethod)
						{
						case QTVAM_NONE:
							if (raw)
								e = "";
							else
								e = ("QTVSV 1\n"
									 "PERROR: You need to provide a common auth method.\n\n");
							break;
						case QTVAM_PLAIN:
							p->hasauthed = !strcmp(qtv_password.string, password);
							break;

							if (0)
							{
						case QTVAM_CCITT:
									e =	("QTVSV 1\n"
										"AUTH: CCITT\n"
										"CHALLENGE: ");
							}
							else if (0)
							{
						case QTVAM_MD4:
									e =	("QTVSV 1\n"
										"AUTH: MD4\n"
										"CHALLENGE: ");
							}
							else
							{
						case QTVAM_MD5:
									e =	("QTVSV 1\n"
										"AUTH: MD5\n"
										"CHALLENGE: ");
							}

							send(p->socket, e, strlen(e), 0);
							send(p->socket, p->challenge, strlen(p->challenge), 0);
							e = "\n\n";
							send(p->socket, e, strlen(e), 0);
							continue;

						default:
							e = ("QTVSV 1\n"
								 "PERROR: FTEQWSV bug detected.\n\n");
							break;
						}
					}

					if (e)
					{
					}
					else if (!versiontouse)
					{
						e =	("QTVSV 1\n"
							 "PERROR: Incompatible version (valid version is v1)\n\n");
					}
					else if (raw)
					{
						if (p->hasauthed == false)
						{
							e =	"";
						}
						else
						{
							SV_MVD_Record(SV_InitStream(p->socket));
							p->socket = INVALID_SOCKET;	//so it's not cleared wrongly.
						}
						p->error = true;
					}
					else
					{
						if (p->hasauthed == true)
						{
							mvddest_t *dst;
							e =	("QTVSV 1\n"
								 "BEGIN\n"
								 "\n");
							send(p->socket, e, strlen(e), 0);
							e = NULL;
							dst = SV_InitStream(p->socket);
							dst->droponmapchange = p->isreverse;
							SV_MVD_Record(dst);
							p->socket = INVALID_SOCKET;	//so it's not cleared wrongly.
						}
						else
						{
							e =	("QTVSV 1\n"
								"PERROR: You need to provide a password.\n\n");
						}
						p->error = true;
					}

					if (e)
					{
						send(p->socket, e, strlen(e), 0);
						p->error = true;
					}
				}
			}
			else if (len == 0)
				p->error = true;
			else
			{	//error of some kind. would block or something
				int e = neterrno();
				if (e != NET_EWOULDBLOCK)
					p->error = true;
			}
		}
	}
#endif
}

int DestCloseAllFlush(qboolean destroyfiles, qboolean mvdonly)
{
	int numclosed = 0;
	mvddest_t *d, **prev, *next;
	DestFlush(true);	//make sure it's all written.

	prev = &demo.dest;
	d = demo.dest;
	while(d)
	{
		next = d->nextdest;
		if (!mvdonly || d->droponmapchange)
		{
			*prev = d->nextdest;
			DestClose(d, destroyfiles);
			numclosed++;
		}
		else
			prev = &d->nextdest;

		d = next;
	}

	return numclosed;
}


int DemoWriteDest(void *data, int len, mvddest_t *d)
{
	if (d->error)
		return 0;
	d->totalsize += len;
	switch(d->desttype)
	{
	case DEST_FILE:
		VFS_WRITE(d->file, data, len);
		break;
	case DEST_BUFFEREDFILE:	//these write to a cache, which is flushed later
	case DEST_STREAM:
		if (d->cacheused+len > d->maxcachesize)
		{
			d->error = true;
			return 0;
		}
		memcpy(d->cache+d->cacheused, data, len);
		d->cacheused += len;
		break;
	case DEST_NONE:
		Sys_Error("DemoWriteDest encoundered bad dest.");
	}
	return len;
}

int DemoWrite(void *data, int len)	//broadcast to all proxies/mvds
{
	mvddest_t *d;
	for (d = demo.dest; d; d = d->nextdest)
	{
		if (singledest && singledest != d)
			continue;
		DemoWriteDest(data, len, d);
	}
	return len;
}

void DemoWriteQTVTimePad(int msecs)	//broadcast to all proxies
{
	mvddest_t *d;
	unsigned char buffer[6];
	while (msecs > 0)
	{
		//duration
		if (msecs > 255)
			buffer[0] = 255;
		else
			buffer[0] = msecs;
		msecs -= buffer[0];
		//message type
		buffer[1] = dem_read;
		//length
		buffer[2] = 0;
		buffer[3] = 0;
		buffer[4] = 0;
		buffer[5] = 0;

		for (d = demo.dest; d; d = d->nextdest)
		{
			if (d->desttype == DEST_STREAM)
			{
				DemoWriteDest(buffer, sizeof(buffer), d);
			}
		}
	}
}




// returns the file size
// return -1 if file is not present
// the file should be in BINARY mode for stupid OSs that care
#define MAX_DIRFILES 1000
#define MAX_MVD_NAME 64

typedef struct
{
	char	name[MAX_MVD_NAME];
	int		size;
} file_t;

typedef struct
{
	file_t *files;
	int		size;
	int		numfiles;
	int		numdirs;

	int		maxfiles;
} dir_t;

#define SORT_NO 0
#define SORT_BY_DATE 1

int QDECL Sys_listdirFound(const char *fname, qofs_t fsize, time_t mtime, void *uptr, searchpathfuncs_t *spath)
{
	file_t *f;
	dir_t *dir = uptr;
	fname = COM_SkipPath(fname);
	if (!*fname)
	{
		dir->numdirs++;
		return true;
	}
	if (dir->numfiles == dir->maxfiles)
		return true;
	f = &dir->files[dir->numfiles++];
	Q_strncpyz(f->name, fname, sizeof(f->name));
	f->size = fsize;
	dir->size += fsize;

	return true;
}

dir_t *Sys_listdir (char *path, char *ext, qboolean usesorting)
{
	char searchterm[MAX_QPATH];

	unsigned int maxfiles = MAX_DIRFILES;
	dir_t *dir = malloc(sizeof(*dir) + sizeof(*dir->files)*maxfiles);
	memset(dir, 0, sizeof(*dir));
	dir->files = (file_t*)(dir+1);
	dir->maxfiles = maxfiles;

	Q_strncpyz(searchterm, va("%s/*%s", path, ext), sizeof(searchterm));
	COM_EnumerateFiles(searchterm, Sys_listdirFound, dir);

	return dir;
}
void Sys_freedir(dir_t *dir)
{
	free(dir);
}









// only one .. is allowed (so we can get to the same dir as the quake exe)
static void SV_DemoDir_Callback(struct cvar_s *var, char *oldvalue)
{
	char *value;

	value = var->string;
	if (!value[0] || value[0] == '/' || (value[0] == '\\' && value[1] == '\\'))
	{
		Cvar_ForceSet(&sv_demoDir, "demos");
		return;
	}
	if (value[0] == '.' && value[1] == '.')
		value += 2;
	if (strstr(value,".."))
	{
		Cvar_ForceSet(&sv_demoDir, "demos");
		return;
	}
}

void SV_MVDPings (void)
{
	sizebuf_t *msg;
	client_t *client;
	int		j;

	for (j = 0, client = svs.clients; j < demo.recorder.max_net_clients && j < svs.allocated_client_slots; j++, client++)
	{
		if (client->state != cs_spawned)
			continue;

		msg = MVDWrite_Begin (dem_all, 0, 7);
		MSG_WriteByte(msg, svc_updateping);
		MSG_WriteByte(msg,  j);
		MSG_WriteShort(msg,  SV_CalcPing(client, false));
		MSG_WriteByte(msg, svc_updatepl);
		MSG_WriteByte (msg, j);
		MSG_WriteByte (msg, client->lossage);
	}
}
void SV_MVD_FullClientUpdate(sizebuf_t *msg, client_t *player)
{
	char info[EXTENDED_INFO_STRING];
	qboolean dosizes;

	if (!sv.mvdrecording)
		return;

	dosizes = !msg;

	if (dosizes)
		msg = MVDWrite_Begin (dem_all, 0, 4);
	MSG_WriteByte (msg, svc_updatefrags);
	MSG_WriteByte (msg, player - svs.clients);
	MSG_WriteShort (msg, player->old_frags);

	if (dosizes)
		msg = MVDWrite_Begin (dem_all, 0, 4);
	MSG_WriteByte (msg, svc_updateping);
	MSG_WriteByte (msg, player - svs.clients);
	MSG_WriteShort (msg, SV_CalcPing(player, false));

	if (dosizes)
		msg = MVDWrite_Begin (dem_all, 0, 3);
	MSG_WriteByte (msg, svc_updatepl);
	MSG_WriteByte (msg, player - svs.clients);
	MSG_WriteByte (msg, player->lossage);

	if (dosizes)
		msg = MVDWrite_Begin (dem_all, 0, 6);
	MSG_WriteByte (msg, svc_updateentertime);
	MSG_WriteByte (msg, player - svs.clients);
	MSG_WriteFloat (msg, realtime - player->connection_started);

	SV_GeneratePublicUserInfo(demo.recorder.fteprotocolextensions, player, info, sizeof(info));

	if (dosizes)
		msg = MVDWrite_Begin (dem_all, 0, 7 + strlen(info));
	MSG_WriteByte (msg, svc_updateuserinfo);
	MSG_WriteByte (msg, player - svs.clients);
	MSG_WriteLong (msg, player->userid);
	MSG_WriteString (msg, info);
}

#if 0
/*
==============
DemoWriteToDisk

Writes to disk a message meant for specifc client
or all messages if type == 0
Message is cleared from demobuf after that
==============
*/

static void SV_MVDWriteToDisk(int type, int to, float time)
{
	int pos = 0, oldm, oldd;
	header_t *p;
	int	size;
	sizebuf_t msg;

	p = (header_t *)demo.dbuf->sb.data;
	demo.dbuf->h = NULL;

	oldm = demo.dbuf->bufsize;
	oldd = demobuffer->start;
	while (pos < demo.dbuf->bufsize)
	{
		size = p->size;
		pos += header + size;

		// no type means we are writing to disk everything
		if (!type || (p->type == type && p->to == to))
		{
			if (size)
			{
				msg.data = p->data;
				msg.cursize = size;

				SV_WriteMVDMessage(&msg, p->type, p->to, time);
			}

			// data is written so it need to be cleard from demobuf
			if (demo.dbuf->sb.data != (qbyte*)p)
				memmove(demo.dbuf->sb.data + size + header, demo.dbuf->sb.data, (qbyte*)p - demo.dbuf->sb.data);

			demo.dbuf->bufsize -= size + header;
			demo.dbuf->sb.data += size + header;
			pos -= size + header;
			demo.dbuf->sb.maxsize -= size + header;
			demobuffer->start += size + header;
		}
		// move along
		p = (header_t *)(p->data + size);
	}

	if (demobuffer->start == demobuffer->last)
	{
		if (demobuffer->start == demobuffer->end)
		{
			demobuffer->end = 0; // demobuffer is empty
			demo.dbuf->sb.data = demobuffer->data;
		}

		// go back to begining of the buffer
		demobuffer->last = demobuffer->end;
		demobuffer->start = 0;
	}
}
#endif

sizebuf_t *MVDWrite_Begin(qbyte type, int to, int size)
{
	if (demomsg.cursize)
		SV_WriteMVDMessage(&demomsg, demomsgtype, demomsgto, demo_prevtime);

	demomsgtype = type;
	demomsgto = to;

	demomsg.maxsize = size;
	demomsg.cursize = 0;
	demomsg.data = demomsgbuf;
	return &demomsg;
#if 0
	qbyte *p;
	qboolean move = false;

	// will it fit?
	while (demo.dbuf->bufsize + size + header > demo.dbuf->sb.maxsize)
	{
		// if we reached the end of buffer move msgbuf to the begining
		if (!move && demobuffer->end > demobuffer->start)
			move = true;

		if (!SV_MVDWritePackets(1))
			return false;

		if (move && demobuffer->start > demo.dbuf->bufsize + header + size)
			MVDMoveBuf();
	}

	if (demo.dbuf->h == NULL || demo.dbuf->h->type != type || demo.dbuf->h->to != to || demo.dbuf->h->full) {
		MVDSetBuf(type, to);
	}

	if (demo.dbuf->h->size + size > MAX_QWMSGLEN)
	{
		demo.dbuf->h->full = 1;
		MVDSetBuf(type, to);
	}

	// we have to make room for new data
	if (demo.dbuf->sb.cursize != demo.dbuf->bufsize) {
		p = demo.dbuf->sb.data + demo.dbuf->sb.cursize;
		memmove(p+size, p, demo.dbuf->bufsize - demo.dbuf->sb.cursize);
	}

	demo.dbuf->bufsize += size;
	demo.dbuf->h->size += size;
	if ((demobuffer->end += size) > demobuffer->last)
		demobuffer->last = demobuffer->end;

	return true;
#endif
}

/*
====================
SV_WriteMVDMessage

Dumps the current net message, along with framing
====================
*/
void SV_WriteMVDMessage (sizebuf_t *msg, int type, int to, float time)
{
	int		len, i, msec;
	qbyte	c;

	if (!sv.mvdrecording)
		return;

	msec = (time - demo_prevtime)*1000;
	if (abs(msec) > 1000)
	{
		//catastoptic slip. debugging? reset any sync
		msec = 0;
		demo_prevtime = time;
	}
	else
	{
		if (msec > 255) msec = 255;
		if (msec < 2) msec = 0;
		demo_prevtime += msec*0.001;
	}

	c = msec;
	DemoWrite(&c, sizeof(c));

	if (demo.lasttype != type || demo.lastto != to)
	{
		demo.lasttype = type;
		demo.lastto = to;
		switch (demo.lasttype)
		{
		case dem_all:
			c = dem_all;
			DemoWrite (&c, sizeof(c));
			break;
		case dem_multiple:
			c = dem_multiple;
			DemoWrite (&c, sizeof(c));

			i = LittleLong(demo.lastto);
			DemoWrite (&i, sizeof(i));
			break;
		case dem_single:
		case dem_stats:
			c = demo.lasttype + (demo.lastto << 3);
			DemoWrite (&c, sizeof(c));
			break;
		default:
			SV_MVDStop_f ();
			Con_Printf("bad demo message type:%d", type);
			return;
		}
	} else {
		c = dem_read;
		DemoWrite (&c, sizeof(c));
	}


	len = LittleLong (msg->cursize);
	DemoWrite (&len, 4);
	DemoWrite (msg->data, msg->cursize);

	DestFlush(false);
}

//if you use ClientReliable to write to demo.recorder's message buffer (for code reuse) call this function to ensure its flushed.
void SV_MVD_WriteReliables(void)
{
	int i;
	if (demo.recorder.netchan.message.cursize)
	{
		SV_WriteMVDMessage(&demo.recorder.netchan.message, dem_all, 0, sv.time);
		demo.recorder.netchan.message.cursize = 0;
	}
	for (i = 0; i < demo.recorder.num_backbuf; i++)
	{
		demo.recorder.backbuf.data = demo.recorder.backbuf_data[i];
		demo.recorder.backbuf.cursize = demo.recorder.backbuf_size[i];
		SV_WriteMVDMessage(&demo.recorder.backbuf, dem_all, 0, sv.time);
		demo.recorder.backbuf_size[i] = 0;
	}
	demo.recorder.num_backbuf = 0;
	demo.recorder.backbuf.cursize = 0;
}

/*
====================
SV_MVDWritePackets

Interpolates to get exact players position for current frame
and writes packets to the disk/memory
====================
*/

float adjustangle(float current, float ideal, float fraction)
{
	float move;

	move = ideal - current;
	if (ideal > current)
	{

		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}

	move *= fraction;

	return (current + move);
}

qboolean SV_MVDWritePackets (int num)
{
	demo_frame_t	*frame, *nextframe;
	demo_client_t	*cl, *nextcl = NULL;
	int				i, j, flags;
	qboolean		valid;
	double			time, playertime, nexttime;
	float			f;
	vec3_t			origin, angles;
	sizebuf_t		msg;
	qbyte			msg_buf[MAX_QWMSGLEN];
	demoinfo_t		*demoinfo;

	if (!sv.mvdrecording)
		return false;

	msg.prim = svs.netprim;
	msg.data = msg_buf;
	msg.maxsize = sizeof(msg_buf);

	if (num > demo.parsecount - demo.lastwritten + 1)
		num = demo.parsecount - demo.lastwritten + 1;

	// 'num' frames to write
	for ( ; num; num--, demo.lastwritten++)
	{
		frame = &demo.frames[demo.lastwritten&DEMO_FRAMES_MASK];
		time = frame->time;
		nextframe = frame;
		msg.cursize = 0;

		// find two frames
		// one before the exact time (time - msec) and one after,
		// then we can interpolte exact position for current frame
		for (i = 0, cl = frame->clients, demoinfo = demo.info; i < demo.recorder.max_net_clients ; i++, cl++, demoinfo++)
		{
			if (cl->parsecount != demo.lastwritten)
				continue; // not valid

			nexttime = playertime = time - cl->sec;

			for (j = demo.lastwritten+1, valid = false; nexttime < time && j < demo.parsecount; j++)
			{
				nextframe = &demo.frames[j&DEMO_FRAMES_MASK];
				nextcl = &nextframe->clients[i];

				if (nextcl->parsecount != j)
					break; // disconnected?
				if (nextcl->fixangle)
					break; // respawned, or walked into teleport, do not interpolate!
				if (!(nextcl->flags & DF_DEAD) && (cl->flags & DF_DEAD))
					break; // respawned, do not interpolate

				nexttime = nextframe->time - nextcl->sec;

				if (nexttime >= time)
				{
					// good, found what we were looking for
					valid = true;
					break;
				}
			}

			if (valid)
			{
				f = (time - nexttime)/(nexttime - playertime);
				for (j=0;j<3;j++) {
					angles[j] = adjustangle(cl->info.angles[j], nextcl->info.angles[j],1.0+f);
					origin[j] = nextcl->info.origin[j] + f*(nextcl->info.origin[j]-cl->info.origin[j]);
				}
			} else {
				VectorCopy(cl->info.origin, origin);
				VectorCopy(cl->info.angles, angles);
			}

			// now write it to buf
			flags = cl->flags;	//df_dead/df_gib

			if (demo.playerreset[i])
			{
				demo.playerreset[i] = false;
				flags |= DF_RESET;
			}

			if (cl->fixangle)
			{
				demo.fixangletime[i] = cl->cmdtime;
			}

			for (j=0; j < 3; j++)
				if (origin[j] != demoinfo->origin[i])
					flags |= DF_ORIGINX << j;

			if (cl->fixangle || demo.fixangletime[i] != cl->cmdtime)
			{
				for (j=0; j < 3; j++)
					if (angles[j] != demoinfo->angles[j])
						flags |= DF_ANGLEX << j;
			}

			if (cl->info.model != demoinfo->model)
				flags |= DF_MODEL;
			if (cl->info.effects != demoinfo->effects)
				flags |= DF_EFFECTS;
			if (cl->info.skinnum != demoinfo->skinnum)
				flags |= DF_SKINNUM;
			if (cl->info.weaponframe != demoinfo->weaponframe)
				flags |= DF_WEAPONFRAME;

			MSG_WriteByte (&msg, svc_playerinfo);
			MSG_WriteByte (&msg, i);
			MSG_WriteShort (&msg, flags);

			MSG_WriteByte (&msg, cl->frame);

			for (j=0 ; j<3 ; j++)
				if (flags & (DF_ORIGINX << j))
					MSG_WriteCoord (&msg, origin[j]);

			for (j=0 ; j<3 ; j++)
				if (flags & (DF_ANGLEX << j))
					MSG_WriteAngle16 (&msg, angles[j]);


			if (flags & DF_MODEL)
				MSG_WriteByte (&msg, cl->info.model);

			if (flags & DF_SKINNUM)
				MSG_WriteByte (&msg, cl->info.skinnum);

			if (flags & DF_EFFECTS)
				MSG_WriteByte (&msg, cl->info.effects);

			if (flags & DF_WEAPONFRAME)
				MSG_WriteByte (&msg, cl->info.weaponframe);

			VectorCopy(cl->info.origin, demoinfo->origin);
			VectorCopy(cl->info.angles, demoinfo->angles);
			demoinfo->skinnum = cl->info.skinnum;
			demoinfo->effects = cl->info.effects;
			demoinfo->weaponframe = cl->info.weaponframe;
			demoinfo->model = cl->info.model;
		}

		//flush any intermediate data
		MVDWrite_Begin(255, -1, 0);
		if (msg.cursize)
			SV_WriteMVDMessage(&msg, dem_all, 0, (float)time);

		/* The above functions can set this variable to false, but that's a really bad thing. Let's try to fix it. */
		if (!sv.mvdrecording)
			return false;
	}

	if (demo.lastwritten > demo.parsecount)
		demo.lastwritten = demo.parsecount;

	return true;
}

// table of readable characters, same as ezquake
char readable[256] =
{
	'.', '_', '_', '_', '_', '.', '_', '_',
	'_', '_', '\n', '_', '\n', '>', '.', '.',
	'[', ']', '0', '1', '2', '3', '4', '5',
	'6', '7', '8', '9', '.', '_', '_', '_',
	' ', '!', '\"', '#', '$', '%', '&', '\'',
	'(', ')', '*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', ':', ';', '<', '=', '>', '?',
	'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
	'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
	'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
	'x', 'y', 'z', '{', '|', '}', '~', '_',
	'_', '_', '_', '_', '_', '.', '_', '_',
	'_', '_', '_', '_', '_', '>', '.', '.',
	'[', ']', '0', '1', '2', '3', '4', '5',
	'6', '7', '8', '9', '.', '_', '_', '_',
	' ', '!', '\"', '#', '$', '%', '&', '\'',
	'(', ')', '*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', ':', ';', '<', '=', '>', '?',
	'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
	'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
	'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
	'x', 'y', 'z', '{', '|', '}', '~', '_'
};
#define chartbl readable

void MVD_Init (void)
{
#define MVDVARGROUP "Server MVD cvars"

	Cvar_Register (&sv_demofps,		MVDVARGROUP);
	Cvar_Register (&sv_demoPings,		MVDVARGROUP);
	Cvar_Register (&sv_demoUseCache,	MVDVARGROUP);
	Cvar_Register (&sv_demoCacheSize,	MVDVARGROUP);
	Cvar_Register (&sv_demoMaxSize,		MVDVARGROUP);
	Cvar_Register (&sv_demoMaxDirSize,	MVDVARGROUP);
	Cvar_Register (&sv_demoDir,		MVDVARGROUP);
	Cvar_Register (&sv_demoPrefix,		MVDVARGROUP);
	Cvar_Register (&sv_demoSuffix,		MVDVARGROUP);
	Cvar_Register (&sv_demotxt,		MVDVARGROUP);
	Cvar_Register (&sv_demoExtraNames,	MVDVARGROUP);
	Cvar_Register (&sv_demoExtensions,	MVDVARGROUP);
}

static char *SV_PrintTeams(void)
{
	char *teams[MAX_CLIENTS];
//	char *p;
	int	i, j, numcl = 0, numt = 0;
	client_t *clients[MAX_CLIENTS];
	char buf[2048] = {0};
	extern cvar_t teamplay;
//	extern char chartbl2[];

	// count teams and players
	for (i=0; i < sv.allocated_client_slots; i++)
	{
		if (svs.clients[i].state != cs_spawned)
			continue;
		if (svs.clients[i].spectator)
			continue;

		clients[numcl++] = &svs.clients[i];
		for (j = 0; j < numt; j++)
			if (!strcmp(Info_ValueForKey(svs.clients[i].userinfo, "team"), teams[j]))
				break;
		if (j != numt)
			continue;

		teams[numt++] = Info_ValueForKey(svs.clients[i].userinfo, "team");
	}

	// create output

	if (numcl == 2) // duel
	{
		snprintf(buf, sizeof(buf), "team1 %s\nteam2 %s\n", clients[0]->name, clients[1]->name);
	}
	else if (!teamplay.value) // ffa
	{
		snprintf(buf, sizeof(buf), "players:\n");
		for (i = 0; i < numcl; i++)
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "  %s\n", clients[i]->name);
	}
	else
	{ // teamplay
		for (j = 0; j < numt; j++)
		{
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "team %s:\n", teams[j]);
			for (i = 0; i < numcl; i++)
				if (!strcmp(Info_ValueForKey(clients[i]->userinfo, "team"), teams[j]))
					snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "  %s\n", clients[i]->name);
		}
	}

	if (!numcl)
		return "\n";
//	for (p = buf; *p; p++) *p = chartbl2[(qbyte)*p];
	return va("%s",buf);
}


mvddest_t *SV_FindRecordFile(char *match, mvddest_t ***link_out)
{
	mvddest_t **link, *f;
	for (link = &demo.dest; *link; link = &(*link)->nextdest)
	{
		f = *link;
		if (f->desttype == DEST_FILE || f->desttype == DEST_BUFFEREDFILE)
		{
			if (!match || !strcmp(match, f->name))
			{
				if (link_out)
					*link_out = link;
				return f;
			}
		}
	}
	return NULL;
}
/*
====================
SV_InitRecord
====================
*/

mvddest_t *SV_InitRecordFile (char *name)
{
	char *s;
	mvddest_t *dst;
	vfsfile_t *file;

	char path[MAX_OSPATH];

	file = FS_OpenVFS (name, "wb", FS_GAMEONLY);
	if (!file)
	{
		Con_Printf ("ERROR: couldn't open \"%s\"\n", name);
		return NULL;
	}

	dst = Z_Malloc(sizeof(mvddest_t));

	if (!sv_demoUseCache.value)
	{
		dst->desttype = DEST_FILE;
		dst->file = file;
		dst->maxcachesize = 0;
	}
	else
	{
		dst->desttype = DEST_BUFFEREDFILE;
		dst->file = file;
		if (sv_demoCacheSize.ival < 0x8000)
			dst->maxcachesize = 0x8000;
		else
			dst->maxcachesize = sv_demoCacheSize.ival;
		dst->cache = BZ_Malloc(dst->maxcachesize);
	}
	dst->droponmapchange = true;

	s = name + strlen(name);
	while (*s != '/') s--;
	Q_strncpyz(dst->name, s+1, sizeof(dst->name));
	Q_strncpyz(dst->path, sv_demoDir.string, sizeof(dst->path));

	if (!*dst->path)
		Q_strncpyz(dst->path, ".", MAX_OSPATH);

	SV_BroadcastPrintf (PRINT_CHAT, "Server starts recording (%s):\n%s\n", (dst->desttype == DEST_BUFFEREDFILE) ? "memory" : "disk", name);
	Cvar_ForceSet(Cvar_Get("serverdemo", "", CVAR_NOSET, ""), SV_Demo_CurrentOutput());

	Q_strncpyz(path, name, MAX_OSPATH);
	Q_strncpyz(path + strlen(path) - 3, "txt", MAX_OSPATH - strlen(path) + 3);

	if (sv_demotxt.value)
	{
		vfsfile_t *f;

		f = FS_OpenVFS (path, "wt", FS_GAMEONLY);
		if (f != NULL)
		{
			char buf[2000];
			date_t date;

			COM_TimeOfDay(&date);

			snprintf(buf, sizeof(buf), "date %s\nmap %s\nteamplay %d\ndeathmatch %d\ntimelimit %d\n%s",date.str, sv.name, (int)teamplay.value, (int)deathmatch.value, (int)timelimit.value, SV_PrintTeams());
			VFS_WRITE(f, buf, strlen(buf));
			VFS_FLUSH(f);
			VFS_CLOSE(f);
		}
	}
	else
	{
		FS_Remove(path, FS_GAMEONLY);
		FS_FlushFSHashRemoved();
	}

	return dst;
}

char *SV_Demo_CurrentOutput(void)
{
	mvddest_t *d;
	for (d = demo.dest; d; d = d->nextdest)
	{
		if (d->desttype == DEST_FILE || d->desttype == DEST_BUFFEREDFILE)
			return d->name;
	}
	return "QTV";
}

mvddest_t *SV_InitStream(int socket)
{
	mvddest_t *dst;

	dst = Z_Malloc(sizeof(mvddest_t));

	dst->desttype = DEST_STREAM;
	dst->socket = socket;
	dst->maxcachesize = 0x8000;	//is this too small?
	dst->cache = BZ_Malloc(dst->maxcachesize);
	dst->droponmapchange = false;

	SV_BroadcastPrintf (PRINT_CHAT, "Smile, you're on QTV!\n");

	return dst;
}

mvdpendingdest_t *SV_MVD_InitPendingStream(int socket, char *ip)
{
	mvdpendingdest_t *dst;
	int i;
	dst = Z_Malloc(sizeof(mvdpendingdest_t));
	dst->socket = socket;

	Q_strncpyz(dst->challenge, ip, sizeof(dst->challenge));
	for (i = strlen(dst->challenge); i < sizeof(dst->challenge)-1; i++)
		dst->challenge[i] = rand()%(127-33) + 33;	//generate a random challenge

	dst->nextdest = demo.pendingdest;
	demo.pendingdest = dst;

	return dst;
}

/*
====================
SV_Stop

stop recording a demo
====================
*/
void SV_MVDStop (int reason, qboolean mvdonly)
{
	sizebuf_t *msg;
	int numclosed;
	if (!sv.mvdrecording)
	{
		Con_Printf ("Not recording a demo.\n");
		return;
	}

	if (reason == 2 || reason == 3)
	{
		DestCloseAllFlush(true, mvdonly);
		// stop and remove

		if (!demo.dest)
			sv.mvdrecording = false;

		if (reason == 3)
			SV_BroadcastPrintf (PRINT_CHAT, "QTV disconnected\n");
		else
			SV_BroadcastPrintf (PRINT_CHAT, "Server recording canceled, demo removed\n");

		Cvar_ForceSet(Cvar_Get("serverdemo", "", CVAR_NOSET, ""), "");

		return;
	}

// write a disconnect message to the demo file
	msg = MVDWrite_Begin(dem_all, 0, 2+strlen("EndOfDemo"));
	MSG_WriteByte (msg, svc_disconnect);
	MSG_WriteString (msg, "EndOfDemo");

	SV_MVDWritePackets(demo.parsecount - demo.lastwritten + 1);
// finish up

	numclosed = DestCloseAllFlush(false, mvdonly);

	if (!demo.dest)
		sv.mvdrecording = false;
	if (numclosed)
	{
		if (!reason)
			SV_BroadcastPrintf (PRINT_CHAT, "Server recording completed\n");
		else
			SV_BroadcastPrintf (PRINT_CHAT, "Server recording stoped\nMax demo size exceeded\n");
	}

	Cvar_ForceSet(Cvar_Get("serverdemo", "", CVAR_NOSET, ""), "");
}

/*
====================
SV_Stop_f
====================
*/
void SV_MVDStop_f (void)
{
	SV_MVDStop(0, true);
}

/*
====================
SV_Cancel_f

Stops recording, and removes the demo
====================
*/
void SV_MVD_Cancel_f (void)
{
	SV_MVDStop(2, true);
}

/*
====================
SV_WriteMVDMessage

Dumps the current net message, prefixed by the length and view angles
====================
*/

void SV_WriteRecordMVDMessage (sizebuf_t *msg)
{
	int		len;
	qbyte	c;

	if (!sv.mvdrecording)
		return;

	if (!msg->cursize)
		return;

	c = 0;
	DemoWrite (&c, sizeof(c));

	c = dem_read;
	DemoWrite (&c, sizeof(c));

	len = LittleLong (msg->cursize);
	DemoWrite (&len, 4);

	DemoWrite (msg->data, msg->cursize);

	DestFlush(false);
}

void SV_WriteSetMVDMessage (void)
{
	int		len;
	qbyte	c;

//Con_Printf("write: %ld bytes, %4.4f\n", msg->cursize, realtime);

	if (!sv.mvdrecording)
		return;

	c = 0;
	DemoWrite (&c, sizeof(c));

	c = dem_set;
	DemoWrite (&c, sizeof(c));


	len = LittleLong(0);
	DemoWrite (&len, 4);
	len = LittleLong(0);
	DemoWrite (&len, 4);

	DestFlush(false);
}

void SV_MVD_SendInitialGamestate(mvddest_t *dest);
qboolean SV_MVD_Record (mvddest_t *dest)
{
	if (!dest)
		return false;

	DestFlush(true);

	if (!sv.mvdrecording)
	{
		memset(&demo, 0, sizeof(demo));
		demo.recorder.protocol = SCP_QUAKEWORLD;
		demo.recorder.netchan.netprim = sv.datagram.prim;

		demo.datagram.maxsize = sizeof(demo.datagram_data);
		demo.datagram.data = demo.datagram_data;
		demo.datagram.prim = demo.recorder.netchan.netprim;

		if (sv_demoExtensions.ival == 2)
		{	/*more limited subset supported by ezquake*/
			demo.recorder.fteprotocolextensions = PEXT_CHUNKEDDOWNLOADS|PEXT_256PACKETENTITIES|PEXT_FLOATCOORDS|PEXT_MODELDBL|PEXT_ENTITYDBL|PEXT_ENTITYDBL2|PEXT_SPAWNSTATIC2;
//			demo.recorder.fteprotocolextensions |= PEXT_HLBSP;	/*ezquake DOES have this, but it is pointless and should have been in some feature mask rather than protocol extensions*/
//			demo.recorder.fteprotocolextensions |= PEXT_ACCURATETIMINGS;	/*ezquake does not support this any more*/
//			demo.recorder.fteprotocolextensions |= PEXT_TRANS;	/*ezquake has no support for .alpha*/
			demo.recorder.fteprotocolextensions2 = PEXT2_VOICECHAT;
			demo.recorder.zquake_extensions = Z_EXT_PM_TYPE | Z_EXT_PM_TYPE_NEW | Z_EXT_VIEWHEIGHT | Z_EXT_SERVERTIME | Z_EXT_PITCHLIMITS | Z_EXT_JOIN_OBSERVE | Z_EXT_VWEP;
		}
		else if (sv_demoExtensions.ival)
		{	/*everything*/
			extern cvar_t pext_replacementdeltas;
			demo.recorder.fteprotocolextensions = PEXT_CSQC | PEXT_COLOURMOD | PEXT_DPFLAGS | PEXT_CUSTOMTEMPEFFECTS | PEXT_ENTITYDBL | PEXT_ENTITYDBL2 | PEXT_FATNESS | PEXT_HEXEN2 | PEXT_HULLSIZE | PEXT_LIGHTSTYLECOL | PEXT_MODELDBL | PEXT_SCALE | PEXT_SETATTACHMENT | PEXT_SETVIEW | PEXT_SOUNDDBL | PEXT_SPAWNSTATIC2 | PEXT_TRANS | PEXT_VIEW2;
			demo.recorder.fteprotocolextensions2 = PEXT2_VOICECHAT | PEXT2_SETANGLEDELTA | PEXT2_PRYDONCURSOR | (pext_replacementdeltas.ival?PEXT2_REPLACEMENTDELTAS:0);
			/*enable these, because we might as well (stat ones are always useful)*/
			demo.recorder.zquake_extensions = Z_EXT_PM_TYPE | Z_EXT_PM_TYPE_NEW | Z_EXT_VIEWHEIGHT | Z_EXT_SERVERTIME | Z_EXT_PITCHLIMITS | Z_EXT_JOIN_OBSERVE | Z_EXT_VWEP;
		}
		else
		{
			demo.recorder.fteprotocolextensions = 0;
			demo.recorder.fteprotocolextensions2 = 0;
			demo.recorder.zquake_extensions = Z_EXT_PM_TYPE | Z_EXT_PM_TYPE_NEW | Z_EXT_VIEWHEIGHT | Z_EXT_SERVERTIME | Z_EXT_PITCHLIMITS | Z_EXT_JOIN_OBSERVE | Z_EXT_VWEP;
		}

		//pointless extensions that are redundant with mvds
		demo.recorder.fteprotocolextensions &= ~PEXT_ACCURATETIMINGS | PEXT_HLBSP;
#ifdef PEXT_Q2BSP
		demo.recorder.fteprotocolextensions &= ~PEXT_Q2BSP;
#endif
#ifdef PEXT_Q3BSP
		demo.recorder.fteprotocolextensions &= ~PEXT_Q3BSP;
#endif
	}
//	else
//		SV_WriteRecordMVDMessage(&buf, dem_read);

	dest->nextdest = demo.dest;
	demo.dest = dest;

	SV_ClientProtocolExtensionsChanged(&demo.recorder);

	SV_MVD_SendInitialGamestate(dest);
	return true;
}
void SV_EnableClientsCSQC(void);
void SV_MVD_SendInitialGamestate(mvddest_t *dest)
{
	sizebuf_t	buf;
	char buf_data[MAX_QWMSGLEN];
	int i, j;
//	int n;
//	const char *s;

	client_t *player;
	char *gamedir;

	if (!demo.dest)
		return;

	sv.mvdrecording = true;

	host_client = &demo.recorder;
	if (host_client->fteprotocolextensions & PEXT_CSQC)
		SV_EnableClientsCSQC();


	demo.pingtime = demo.time = sv.time;


	singledest = dest;

/*-------------------------------------------------*/

// serverdata
	// send the info about the new client to all connected clients
	memset(&buf, 0, sizeof(buf));
	buf.data = buf_data;
	buf.maxsize = sizeof(buf_data);
	buf.prim = svs.netprim;

// send the serverdata

	gamedir = Info_ValueForKey (svs.info, "*gamedir");
	if (!gamedir[0])
		gamedir = FS_GetGamedir(true);

	MSG_WriteByte (&buf, svc_serverdata);

	//fix up extensions to match sv_bigcoords correctly. sorry for old clients not working.
	if (buf.prim.coordsize == 4)
		demo.recorder.fteprotocolextensions |= PEXT_FLOATCOORDS;
	else
		demo.recorder.fteprotocolextensions &= ~PEXT_FLOATCOORDS;

	if (demo.recorder.fteprotocolextensions)
	{
		MSG_WriteLong(&buf, PROTOCOL_VERSION_FTE);
		MSG_WriteLong(&buf, demo.recorder.fteprotocolextensions);
	}
	if (demo.recorder.fteprotocolextensions2)
	{
		MSG_WriteLong(&buf, PROTOCOL_VERSION_FTE2);
		MSG_WriteLong(&buf, demo.recorder.fteprotocolextensions2);
	}
	MSG_WriteLong (&buf, PROTOCOL_VERSION_QW);
	MSG_WriteLong (&buf, svs.spawncount);
	MSG_WriteString (&buf, gamedir);

	if (demo.recorder.fteprotocolextensions2 & PEXT2_MAXPLAYERS)
		MSG_WriteByte(&buf, demo.recorder.max_net_ents);

	MSG_WriteFloat (&buf, sv.time);

	// send full levelname
	MSG_WriteString (&buf, sv.mapname);

	// send the movevars
	MSG_WriteFloat(&buf, movevars.gravity);
	MSG_WriteFloat(&buf, movevars.stopspeed);
	MSG_WriteFloat(&buf, movevars.maxspeed);
	MSG_WriteFloat(&buf, movevars.spectatormaxspeed);
	MSG_WriteFloat(&buf, movevars.accelerate);
	MSG_WriteFloat(&buf, movevars.airaccelerate);
	MSG_WriteFloat(&buf, movevars.wateraccelerate);
	MSG_WriteFloat(&buf, movevars.friction);
	MSG_WriteFloat(&buf, movevars.waterfriction);
	MSG_WriteFloat(&buf, movevars.entgravity);

	SV_WriteRecordMVDMessage (&buf);
	SZ_Clear (&buf);

#if 1
	demo.recorder.prespawn_stage = PRESPAWN_SERVERINFO;
	demo.recorder.prespawn_idx = 0;
	demo.recorder.netchan.message = buf;
	while (demo.recorder.prespawn_stage != PRESPAWN_DONE)
	{
		if (demo.recorder.prespawn_stage == PRESPAWN_MAPCHECK)
		{
			demo.recorder.prespawn_stage++;//client won't reply, so don't wait.
			demo.recorder.prespawn_idx = 0;
		}
		if (demo.recorder.prespawn_stage == PRESPAWN_SOUNDLIST || demo.recorder.prespawn_stage == PRESPAWN_MODELLIST)
			demo.recorder.prespawn_idx &= ~0x80000000;	//normally set for the server to wait for ack. we don't want to wait.

		SV_SendClientPrespawnInfo(&demo.recorder);
		SV_WriteRecordMVDMessage (&demo.recorder.netchan.message);
		SZ_Clear (&demo.recorder.netchan.message);
	}
	memset(&demo.recorder.netchan.message, 0, sizeof(demo.recorder.netchan.message));
#else
	// send music
	MSG_WriteByte (&buf, svc_cdtrack);
	MSG_WriteByte (&buf, 0); // none in demos

	// send server info string
	MSG_WriteByte (&buf, svc_stufftext);
	MSG_WriteString (&buf, va("fullserverinfo \"%s\"\n", svs.info) );

	// flush packet
	SV_WriteRecordMVDMessage (&buf);
	SZ_Clear (&buf);

// soundlist
	MSG_WriteByte (&buf, svc_soundlist); /*FIXME: soundlist2*/
	MSG_WriteByte (&buf, 0);

	n = 0;
	s = sv.strings.sound_precache[n+1];
	while (*s)
	{
		MSG_WriteString (&buf, s);
		if (buf.cursize > MAX_QWMSGLEN/2)
		{
			MSG_WriteByte (&buf, 0);
			MSG_WriteByte (&buf, n);
			SV_WriteRecordMVDMessage (&buf);
			SZ_Clear (&buf);
			MSG_WriteByte (&buf, svc_soundlist);
			MSG_WriteByte (&buf, n + 1);
		}
		n++;
		s = sv.strings.sound_precache[n+1];
	}

	if (buf.cursize)
	{
		MSG_WriteByte (&buf, 0);
		MSG_WriteByte (&buf, 0);
		SV_WriteRecordMVDMessage (&buf);
		SZ_Clear (&buf);
	}

// modellist
	MSG_WriteByte (&buf, svc_modellist); /*FIXME: modellist2*/
	MSG_WriteByte (&buf, 0);

	n = 0;
	s = sv.strings.model_precache[n+1];
	while (s)
	{
		MSG_WriteString (&buf, s);
		if (buf.cursize > MAX_QWMSGLEN/2)
		{
			MSG_WriteByte (&buf, 0);
			MSG_WriteByte (&buf, n);
			SV_WriteRecordMVDMessage (&buf);
			SZ_Clear (&buf);
			MSG_WriteByte (&buf, svc_modellist);
			MSG_WriteByte (&buf, n + 1);
		}
		n++;
		s = sv.strings.model_precache[n+1];
	}
	if (buf.cursize)
	{
		MSG_WriteByte (&buf, 0);
		MSG_WriteByte (&buf, 0);
		SV_WriteRecordMVDMessage (&buf);
		SZ_Clear (&buf);
	}

// baselines
	{
		entity_state_t from;
		edict_t *ent;
		entity_state_t *state;

		memset(&from, 0, sizeof(from));

		for (n = 0; n < sv.world.num_edicts; n++)
		{
			ent = EDICT_NUM(svprogfuncs, n);
			state = &ent->baseline;

			if (!state->number || !state->modelindex)
			{	//ent doesn't have a baseline
				continue;
			}

			if (demo.recorder.fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)
			{
				MSG_WriteByte(&buf, svcfte_spawnbaseline2);
				SVFTE_EmitBaseline(state, true, &buf);
			}
			else if (!ent)
			{
				MSG_WriteByte(&buf, svc_spawnbaseline);

				MSG_WriteShort (&buf, n);

				MSG_WriteByte (&buf, 0);

				MSG_WriteByte (&buf, 0);
				MSG_WriteByte (&buf, 0);
				MSG_WriteByte (&buf, 0);
				for (i=0 ; i<3 ; i++)
				{
					MSG_WriteCoord(&buf, 0);
					MSG_WriteAngle(&buf, 0);
				}
			}
			else if (demo.recorder.fteprotocolextensions & PEXT_SPAWNSTATIC2)
			{
				MSG_WriteByte(&buf, svcfte_spawnbaseline2);
				SVQW_WriteDelta(&from, state, &buf, true, demo.recorder.fteprotocolextensions);
			}
			else
			{
				MSG_WriteByte(&buf, svc_spawnbaseline);

				MSG_WriteShort (&buf, n);

				MSG_WriteByte (&buf, state->modelindex&255);

				MSG_WriteByte (&buf, state->frame);
				MSG_WriteByte (&buf, (int)state->colormap);
				MSG_WriteByte (&buf, (int)state->skinnum);
				for (i=0 ; i<3 ; i++)
				{
					MSG_WriteCoord(&buf, state->origin[i]);
					MSG_WriteAngle(&buf, state->angles[i]);
				}
			}
			if (buf.cursize > MAX_QWMSGLEN/2)
			{
				SV_WriteRecordMVDMessage (&buf);
				SZ_Clear (&buf);
			}
		}
	}

	//prespawn

	for (n = 0; n < sv.num_signon_buffers; n++)
	{
		if (buf.cursize+sv.signon_buffer_size[n] > MAX_QWMSGLEN/2)
		{
			SV_WriteRecordMVDMessage (&buf);
			SZ_Clear (&buf);
		}
		SZ_Write (&buf,
			sv.signon_buffers[n],
			sv.signon_buffer_size[n]);
	}

	if (buf.cursize > MAX_QWMSGLEN/2)
	{
		SV_WriteRecordMVDMessage (&buf);
		SZ_Clear (&buf);
	}

	MSG_WriteByte (&buf, svc_stufftext);
	MSG_WriteString (&buf, va("cmd spawn %i\n",svs.spawncount) );

	if (buf.cursize)
	{
		SV_WriteRecordMVDMessage (&buf);
		SZ_Clear (&buf);
	}
#endif
// send current status of all other players

	for (i = 0; i < demo.recorder.max_net_clients && i < svs.allocated_client_slots; i++)
	{
		player = &svs.clients[i];

		SV_MVD_FullClientUpdate(&buf, player);

		if (buf.cursize > MAX_QWMSGLEN/2)
		{
			SV_WriteRecordMVDMessage (&buf);
			SZ_Clear (&buf);
		}
	}

// send all current light styles
	for (i=0 ; i<MAX_LIGHTSTYLES ; i++)
	{
		if (i >= MAX_STANDARDLIGHTSTYLES)
			if (!sv.strings.lightstyles[i])
				continue;
#ifdef PEXT_LIGHTSTYLECOL
		if ((demo.recorder.fteprotocolextensions & PEXT_LIGHTSTYLECOL) && (sv.strings.lightstylecolours[i][0]!=1||sv.strings.lightstylecolours[i][1]!=1||sv.strings.lightstylecolours[i][2]!=1) && sv.strings.lightstyles[i])
		{
			MSG_WriteByte (&buf, svcfte_lightstylecol);
			MSG_WriteByte (&buf, (unsigned char)i);
			MSG_WriteByte (&buf, 0x87);
			MSG_WriteShort(&buf, sv.strings.lightstylecolours[i][0]*1024);
			MSG_WriteShort(&buf, sv.strings.lightstylecolours[i][1]*1024);
			MSG_WriteShort(&buf, sv.strings.lightstylecolours[i][2]*1024);
			MSG_WriteString (&buf, sv.strings.lightstyles[i]);
		}
		else
#endif
		{
			MSG_WriteByte (&buf, svc_lightstyle);
			MSG_WriteByte (&buf, (unsigned char)i);
			MSG_WriteString (&buf, sv.strings.lightstyles[i]);
		}
	}

	//invalidate stats+players somehow
	for (i = 0; i < MAX_CLIENTS; i++)
	{
		for (j = 0; j < MAX_CL_STATS; j++)
		{
			demo.statsi[i][j] ^= -1;
			demo.statsf[i][j] *= -0.41426712;	//randomish value
		}
		demo.playerreset[i] = true;
	}

	// get the client to check and download skins
	// when that is completed, a begin command will be issued
	MSG_WriteByte (&buf, svc_stufftext);
	MSG_WriteString (&buf, "skins\n");

	SV_WriteRecordMVDMessage (&buf);

	SV_WriteSetMVDMessage();

	singledest = NULL;
}

/*
====================
SV_CleanName

Cleans the demo name, removes restricted chars, makes name lowercase
====================
*/

char *SV_CleanName (unsigned char *name)
{
	static char text[1024];
	char *out = text;

	*out = chartbl[*name++];

	while (*name && out - text < sizeof(text))
		if (*out == '_' && chartbl[*name] == '_')
			name++;
		else *++out = chartbl[*name++];

	*++out = 0;
	return text;
}

/*
====================
SV_Record_f

record <demoname>
====================
*/
void SV_MVD_Record_f (void)
{
	int		c;
	char	name[MAX_OSPATH+MAX_MVD_NAME];
	char	newname[MAX_MVD_NAME];
	dir_t	*dir;

	c = Cmd_Argc();
	if (c != 2)
	{
		Con_Printf ("mvdrecord <demoname>\n");
		return;
	}

	if (sv.state != ss_active){
		Con_Printf ("Not active yet.\n");
		return;
	}

	dir = Sys_listdir(sv_demoDir.string, ".*", SORT_NO);
	if (sv_demoMaxDirSize.value && dir->size > sv_demoMaxDirSize.value*1024)
	{
		Con_Printf("insufficient directory space, increase sv_demoMaxDirSize\n");
		Sys_freedir(dir);
		return;
	}
	Sys_freedir(dir);
	dir = NULL;

	Q_strncpyz(newname, va("%s%s", sv_demoPrefix.string, SV_CleanName(Cmd_Argv(1))),
			sizeof(newname) - strlen(sv_demoSuffix.string) - 5);
	Q_strncatz(newname, sv_demoSuffix.string, MAX_MVD_NAME);

	snprintf (name, MAX_OSPATH+MAX_MVD_NAME, "%s/%s", sv_demoDir.string, newname);


	COM_StripExtension(name, name, sizeof(name));
	COM_DefaultExtension(name, ".mvd", sizeof(name));
	FS_CreatePath (name, FS_GAMEONLY);

	//
	// open the demo file and start recording
	//
	SV_MVD_Record (SV_InitRecordFile(name));
}

void SV_MVD_QTVReverse_f (void)
{
#ifndef HAVE_TCP
	Con_Printf ("%s is not supported in this build\n", Cmd_Argv(0));
#else
	char *ip;
	if (sv.state != ss_active)
	{
		Con_Printf ("Server is not running\n");
		return;
	}
	if (Cmd_Argc() != 2)
	{
		Con_Printf ("%s ip:port\n", Cmd_Argv(0));
		return;
	}

	ip = Cmd_Argv(1);



{
	char *data;
	int sock;

	struct sockaddr_qstorage	remote;
//	int fromlen;

	int adrfam;
	int adrsz;
	unsigned int nonblocking = true;


	if (!NET_StringToSockaddr(ip, 0, &remote, &adrfam, &adrsz))
	{
		Con_Printf ("qtvreverse: failed to resolve address\n");
		return;
	}

	if ((sock = socket (adrfam, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		Con_Printf ("qtvreverse: socket: %s\n", strerror(neterrno()));
		return;
	}
	if (connect(sock, (void*)&remote, adrsz) == INVALID_SOCKET)
	{
		closesocket(sock);
		Con_Printf ("qtvreverse: connect: %s\n", strerror(neterrno()));
		return;
	}

	if (ioctlsocket (sock, FIONBIO, (u_long *)&nonblocking) == INVALID_SOCKET)
	{
		closesocket(sock);
		Con_Printf ("qtvreverse: ioctl FIONBIO: %s\n", strerror(neterrno()));
		return;
	}

	data =	"QTV\n"
			"REVERSE\n"
			"\n";
	if (send(sock, data, strlen(data), 0) == INVALID_SOCKET)
	{
		closesocket(sock);
		Con_Printf ("qtvreverse: send: %s\n", strerror(neterrno()));
		return;
	}


	SV_MVD_InitPendingStream(sock, ip)->isreverse = true;
}

	//SV_MVD_Record (dest);
#endif
}

/*
====================
SV_EasyRecord_f

easyrecord [demoname]
====================
*/

int	Dem_CountPlayers ()
{
	int	i, count;

	count = 0;
	for (i = 0; i < sv.allocated_client_slots ; i++)
	{
		if (svs.clients[i].name[0] && !svs.clients[i].spectator)
			count++;
	}

	return count;
}

char *Dem_Team(int num)
{
	int i;
	static char *lastteam[2];
	qboolean first = true;
	client_t *client;
	static int index = 0;

	index = 1 - index;

	for (i = 0, client = svs.clients; num && i < sv.allocated_client_slots; i++, client++)
	{
		if (!client->name[0] || client->spectator)
			continue;

		if (first || strcmp(lastteam[index], Info_ValueForKey(client->userinfo, "team")))
		{
			first = false;
			num--;
			lastteam[index] = Info_ValueForKey(client->userinfo, "team");
		}
	}

	if (num)
		return "";

	return lastteam[index];
}

char *Dem_PlayerName(int num)
{
	int i;
	client_t *client;

	for (i = 0, client = svs.clients; i < sv.allocated_client_slots; i++, client++)
	{
		if (!client->name[0] || client->spectator)
			continue;

		if (!--num)
			return client->name;
	}

	return "";
}

// -> scream
char *Dem_PlayerNameTeam(char *t)
{
	int	i;
	client_t *client;
	static char	n[1024];
	int	sep;

	n[0] = 0;

	sep = 0;

	for (i = 0, client = svs.clients; i < sv.allocated_client_slots; i++, client++)
	{
		if (!client->name[0] || client->spectator)
			continue;

		if (strcmp(t, Info_ValueForKey(client->userinfo, "team"))==0)
		{
			if (sep >= 1)
				Q_strncatz (n, "_", sizeof(n));
//				snprintf (n, sizeof(n), "%s_", n);
			Q_strncatz (n, client->name, sizeof(n));
//			snprintf (n, sizeof(n),"%s%s", n, client->name);
			sep++;
		}
	}

	return n;
}

int	Dem_CountTeamPlayers (char *t)
{
	int	i, count;

	count = 0;
	for (i = 0; i < sv.allocated_client_slots ; i++)
	{
		if (svs.clients[i].name[0] && !svs.clients[i].spectator)
			if (strcmp(Info_ValueForKey(svs.clients[i].userinfo, "team"), t)==0)
				count++;
	}

	return count;
}

// <-

void SV_MVDEasyRecord_f (void)
{
	int		c;
	dir_t	*dir;
	char	name[1024];
	char	name2[MAX_OSPATH*7]; // scream
	//char	name2[MAX_OSPATH*2];
	int		i;
	vfsfile_t	*f;

	c = Cmd_Argc();
	if (c > 2)
	{
		Con_Printf ("easyrecord [demoname]\n");
		return;
	}

	if (sv.state < ss_active)
	{
		Con_Printf("Server isn't running or is still loading\n");
		return;
	}

	dir = Sys_listdir(sv_demoDir.string, ".*", SORT_NO);
	if (sv_demoMaxDirSize.value && dir->size > sv_demoMaxDirSize.value*1024)
	{
		Con_Printf("insufficient directory space, increase sv_demoMaxDirSize\n");
		Sys_freedir(dir);
		return;
	}
	Sys_freedir(dir);

	if (c == 2)
	{
		char *c;
		Q_strncpyz (name, Cmd_Argv(1), sizeof(name));
		while((c = strchr(name, ':')))
			*c = '-';
	}
	else
	{
		i = Dem_CountPlayers();
		/*if (!deathmatch.ival)
		{
			if (coop.ival || i>1)
				snprintf (name, sizeof(name), "coop_%s_%d(%d)", sv.name, skill.ival, i);
			else
				snprintf (name, sizeof(name), "sp_%s_%d_%s", sv.name, skill.ival, Dem_PlayerName(0));
		}
		else*/ if (teamplay.value >= 1 && i > 2)
		{
			// Teamplay
			snprintf (name, sizeof(name), "%don%d_", Dem_CountTeamPlayers(Dem_Team(1)), Dem_CountTeamPlayers(Dem_Team(2)));
			if (sv_demoExtraNames.value > 0)
			{
				Q_strncatz (name, va("[%s]_%s_vs_[%s]_%s_%s",
									Dem_Team(1), Dem_PlayerNameTeam(Dem_Team(1)),
									Dem_Team(2), Dem_PlayerNameTeam(Dem_Team(2)),
									sv.name), sizeof(name));
			} else
				Q_strncatz (name, va("%s_vs_%s_%s", Dem_Team(1), Dem_Team(2), sv.name), sizeof(name));
		} else {
			if (i == 2) {
				// Duel
				snprintf (name, sizeof(name), "duel_%s_vs_%s_%s",
					Dem_PlayerName(1),
					Dem_PlayerName(2),
					sv.name);
			} else {
				// FFA
				snprintf (name, sizeof(name), "ffa_%s(%d)", sv.name, i);
			}
		}
	}

	// <-

// Make sure the filename doesn't contain illegal characters
	Q_strncpyz(name, va("%s%s", sv_demoPrefix.string, SV_CleanName(name)),
			MAX_MVD_NAME - strlen(sv_demoSuffix.string) - 7);
	Q_strncatz(name, sv_demoSuffix.string, sizeof(name));
	Q_strncpyz(name, va("%s/%s", sv_demoDir.string, name), sizeof(name));
// find a filename that doesn't exist yet
	Q_strncpyz(name2, name, sizeof(name2));
//	COM_StripExtension(name2, name2);
	FS_CreatePath (name2, FS_GAMEONLY);
	strcat (name2, ".mvd");
	if ((f = FS_OpenVFS(name2, "rb", FS_GAMEONLY)) == 0)
		f = FS_OpenVFS(va("%s.gz", name2), "rb", FS_GAMEONLY);

	if (f)
	{
		i = 1;
		do {
			VFS_CLOSE (f);
			snprintf(name2, sizeof(name2), "%s_%02i", name, i);
//			COM_StripExtension(name2, name2);
			strcat (name2, ".mvd");
			if ((f = FS_OpenVFS (name2, "rb", FS_GAMEONLY)) == 0)
				f = FS_OpenVFS(va("%s.gz", name2), "rb", FS_GAMEONLY);
			i++;
		} while (f);
	}

	SV_MVD_Record (SV_InitRecordFile(name2));
}

#ifdef HAVE_TCP
static SOCKET MVD_StreamStartListening(int port)
{
	SOCKET sock;

	struct sockaddr_in	address;
//	int fromlen;

	unsigned int nonblocking = true;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons((u_short)port);



	if ((sock = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		Sys_Error ("MVD_StreamStartListening: socket: %s", strerror(neterrno()));
	}

	if (ioctlsocket (sock, FIONBIO, (u_long *)&nonblocking) == INVALID_SOCKET)
	{
		Sys_Error ("FTP_TCP_OpenSocket: ioctl FIONBIO: %s", strerror(neterrno()));
	}

	if( bind (sock, (void *)&address, sizeof(address)) == INVALID_SOCKET)
	{
		closesocket(sock);
		return INVALID_SOCKET;
	}

	listen(sock, 2);

	return sock;
}
#endif

void SV_MVDStream_Poll(void)
{
#ifdef HAVE_TCP
	static SOCKET listensocket=INVALID_SOCKET;
	static int listenport;
	int _true = true;

	int client;
	netadr_t na;
	struct sockaddr_qstorage addr;
	int addrlen;
	int count;
	qboolean wanted;
	mvddest_t *dest;
	char *ip;
	char adrbuf[MAX_ADR_SIZE];

	if (!sv.state || !qtv_streamport.ival)
		wanted = false;
	else if (listenport && qtv_streamport.ival != listenport)	//easy way to switch... disable for a frame. :)
	{
		listenport = qtv_streamport.ival;
		wanted = false;
	}
	else
	{
		listenport = qtv_streamport.ival;
		wanted = true;
	}

	if (wanted && listensocket==INVALID_SOCKET)
	{
		listensocket = MVD_StreamStartListening(listenport);
		if (listensocket==INVALID_SOCKET && qtv_streamport.modified)
		{
			Con_Printf("Cannot open TCP port %i for QTV\n", listenport);
			qtv_streamport.modified = false;
		}

	}
	else if (!wanted && listensocket!=INVALID_SOCKET)
	{
		closesocket(listensocket);
		listensocket = INVALID_SOCKET;
		return;
	}
	if (listensocket==INVALID_SOCKET)
		return;

	addrlen = sizeof(addr);
	client = accept(listensocket, (struct sockaddr *)&addr, &addrlen);

	if (client == INVALID_SOCKET)
		return;

	ioctlsocket(client, FIONBIO, (u_long *)&_true);

	if (qtv_maxstreams.value > 0)
	{
		count = 0;
		for (dest = demo.dest; dest; dest = dest->nextdest)
		{
			if (dest->desttype == DEST_STREAM)
			{
				count++;
			}
		}

		if (count > qtv_maxstreams.value)
		{	//sorry
			char *goawaymessage = "QTVSV 1\nTERROR: This server enforces a limit on the number of proxies connected at any one time. Please try again later\n\n";

			send(client, goawaymessage, strlen(goawaymessage), 0);
			closesocket(client);
			return;
		}
	}

	SockadrToNetadr(&addr, &na);
	ip = NET_AdrToString(adrbuf, sizeof(adrbuf), &na);
	Con_Printf("MVD streaming client attempting to connect from %s\n", ip);

	SV_MVD_InitPendingStream(client, ip);

//	SV_MVD_Record (SV_InitStream(client));
#endif
}

void SV_MVDList_f (void)
{
	mvddest_t *d;
	dir_t	*dir;
	file_t	*list;
	float	f;
	int		i,j,show;

	Con_Printf("content of %s/*.mvd\n", sv_demoDir.string);
	dir = Sys_listdir(sv_demoDir.string, ".mvd", SORT_BY_DATE);
	list = dir->files;
	if (!list->name[0])
	{
		Con_Printf("no demos\n");
	}

	for (i = 1; i <= dir->numfiles; i++, list++)
	{
		for (j = 1; j < Cmd_Argc(); j++)
			if (strstr(list->name, Cmd_Argv(j)) == NULL)
				break;
		show = Cmd_Argc() == j;

		if (show)
		{
			for (d = demo.dest; d; d = d->nextdest)
			{
				if (!strcmp(list->name, d->name))
					Con_Printf("*%d: ^[^7%s\\demo\\%s/%s^] %dk\n", i, list->name, sv_demoDir.string, list->name, d->totalsize/1024);
			}
			if (!d)
				Con_Printf("%d: ^[^7%s\\demo\\%s/%s^] %dk\n", i, list->name, sv_demoDir.string, list->name, list->size/1024);
		}
	}

	for (d = demo.dest; d; d = d->nextdest)
		dir->size += d->totalsize;

	Con_Printf("\ndirectory size: %.1fMB\n",(float)dir->size/(1024*1024));
	if (sv_demoMaxDirSize.value)
	{
		f = (sv_demoMaxDirSize.value*1024 - dir->size)/(1024*1024);
		if ( f < 0)
			f = 0;
		Con_Printf("space available: %.1fMB\n", f);
	}

	Sys_freedir(dir);
}

void SV_UserCmdMVDList_f (void)
{
	mvddest_t *d;
	dir_t	*dir;
	file_t	*list;
	float	f;
	int		i,j,show;

	SV_ClientPrintf(host_client, PRINT_HIGH, "available demos:\n");
	dir = Sys_listdir(sv_demoDir.string, ".mvd", SORT_BY_DATE);
	list = dir->files;
	if (!list->name[0])
	{
		SV_ClientPrintf(host_client, PRINT_HIGH, "no demos\n");
	}

	for (i = 1; i <= dir->numfiles; i++, list++)
	{
		for (j = 1; j < Cmd_Argc(); j++)
			if (strstr(list->name, Cmd_Argv(j)) == NULL)
				break;
		show = Cmd_Argc() == j;

		if (show)
		{
			for (d = demo.dest; d; d = d->nextdest)
			{
				if (!strcmp(list->name, d->name))
					SV_ClientPrintf(host_client, PRINT_HIGH, "*%d: %s %dk\n", i, list->name, d->totalsize/1024);
			}
			if (!d)
				SV_ClientPrintf(host_client, PRINT_HIGH, "%d: %s %dk\n", i, list->name, list->size/1024);
		}
	}

	for (d = demo.dest; d; d = d->nextdest)
		dir->size += d->totalsize;

	SV_ClientPrintf(host_client, PRINT_HIGH, "\ndirectory size: %.1fMB\n",(float)dir->size/(1024*1024));
	if (sv_demoMaxDirSize.value)
	{
		f = (sv_demoMaxDirSize.value*1024 - dir->size)/(1024*1024);
		if ( f < 0)
			f = 0;
		SV_ClientPrintf(host_client, PRINT_HIGH, "space available: %.1fMB\n", f);
	}

	Sys_freedir(dir);
}

char *SV_MVDNum(char *buffer, int bufferlen, int num)
{
	file_t	*list;
	dir_t	*dir;

	dir = Sys_listdir(sv_demoDir.string, ".mvd", SORT_BY_DATE);
	list = dir->files;

	if (num > dir->numfiles || num <= 0)
	{
		Sys_freedir(dir);
		return NULL;
	}
	num--;

	list += num;

	Q_strncpyz(buffer, list->name, bufferlen);
	Sys_freedir(dir);
	return buffer;
}

char *SV_MVDName2Txt(char *name)
{
	char s[MAX_OSPATH];

	if (!name)
		return NULL;

	Q_strncpyz(s, name, MAX_OSPATH);

	if (strstr(s, ".mvd.gz") != NULL)
		Q_strncpyz(s + strlen(s) - 6, "txt", MAX_OSPATH - strlen(s) + 6);
	else
		Q_strncpyz(s + strlen(s) - 3, "txt", MAX_OSPATH - strlen(s) + 3);

	return va("%s", s);
}

char *SV_MVDTxTNum(char *buffer, int bufferlen, int num)
{
	return SV_MVDName2Txt(SV_MVDNum(buffer, bufferlen, num));
}

void SV_MVDRemove_f (void)
{
	char name[MAX_MVD_NAME], *ptr;
	char path[MAX_OSPATH];
	int i;
	mvddest_t *active;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("rmdemo <demoname> - removes the demo\nrmdemo *<token>   - removes demo with <token> in the name\nrmdemo *          - removes all demos\n");
		return;
	}

	ptr = Cmd_Argv(1);
	if (*ptr == '*')
	{
		dir_t *dir;
		file_t *list;

		// remove all demos with specified token
		ptr++;

		dir = Sys_listdir(sv_demoDir.string, ".mvd", SORT_BY_DATE);
		list = dir->files;
		for (i = 0;i < dir->numfiles; list++)
		{
			if (strstr(list->name, ptr))
			{
				mvddest_t *active = SV_FindRecordFile(list->name, NULL);
				if (active)
					SV_MVDStop_f();

				// stop recording first;
				snprintf(path, MAX_OSPATH, "%s/%s", sv_demoDir.string, list->name);
				if (FS_Remove(path, FS_GAMEONLY))
				{
					Con_Printf("removing %s...\n", list->name);
					i++;
				}

				FS_Remove(SV_MVDName2Txt(path), FS_GAMEONLY);
			}
		}
		Sys_freedir(dir);

		if (i)
		{
			Con_Printf("%d demos removed\n", i);
		}
		else
		{
			Con_Printf("no matching found\n");
		}

		return;
	}

	Q_strncpyz(name, Cmd_Argv(1), MAX_MVD_NAME);
	COM_DefaultExtension(name, ".mvd", sizeof(name));

	snprintf(path, MAX_OSPATH, "%s/%s", sv_demoDir.string, name);

	active = SV_FindRecordFile(name, NULL);
	if (active)
		SV_MVDStop_f();

	if (FS_Remove(path, FS_GAMEONLY))
	{
		Con_Printf("demo %s successfully removed\n", name);
	}
	else
		Con_Printf("unable to remove demo %s\n", name);

	FS_Remove(SV_MVDName2Txt(path), FS_GAMEONLY);
}

void SV_MVDRemoveNum_f (void)
{
	int		num;
	char namebuf[MAX_QPATH];
	char	*val, *name;
	char path[MAX_OSPATH];

	if (Cmd_Argc() != 2)
	{
		Con_Printf("rmdemonum <#>\n");
		return;
	}

	val = Cmd_Argv(1);
	if ((num = atoi(val)) == 0 && val[0] != '0')
	{
		Con_Printf("rmdemonum <#>\n");
		return;
	}

	name = SV_MVDNum(namebuf, sizeof(namebuf), num);

	if (name != NULL)
	{
		mvddest_t *active = SV_FindRecordFile(name, NULL);
		if (active)
			SV_MVDStop_f();

		snprintf(path, MAX_OSPATH, "%s/%s", sv_demoDir.string, name);
		if (FS_Remove(path, FS_GAMEONLY))
		{
			Con_Printf("demo %s succesfully removed\n", name);
		}
		else
			Con_Printf("unable to remove demo %s\n", name);

		FS_Remove(SV_MVDName2Txt(path), FS_GAMEONLY);
	}
	else
		Con_Printf("invalid demo num\n");
}

void SV_MVDInfoAdd_f (void)
{
	char namebuf[MAX_QPATH];
	char *name, *args, path[MAX_OSPATH];
	vfsfile_t *f;

	if (Cmd_Argc() < 3) {
		Con_Printf("usage:MVDInfoAdd <demonum> <info string>\n<demonum> = * for currently recorded demo\n");
		return;
	}

	if (!strcmp(Cmd_Argv(1), "*"))
	{
		mvddest_t *active = SV_FindRecordFile(NULL, NULL);
		if (!active)
		{
			Con_Printf("Not recording demo!\n");
			return;
		}

		snprintf(path, MAX_OSPATH, "%s/%s", active->path, SV_MVDName2Txt(active->name));
	}
	else
	{
		name = SV_MVDTxTNum(namebuf, sizeof(namebuf), atoi(Cmd_Argv(1)));

		if (!name)
		{
			Con_Printf("invalid demo num\n");
			return;
		}

		snprintf(path, MAX_OSPATH, "%s/%s", sv_demoDir.string, name);
	}

	if ((f = FS_OpenVFS(path, "a+t", FS_GAMEONLY)) == NULL)
	{
		Con_Printf("failed to open the file\n");
		return;
	}

	// skip demonum
	args = Cmd_Args();
	while (*args > 32) args++;
	while (*args && *args <= 32) args++;

	VFS_WRITE(f, args, strlen(args));
	VFS_WRITE(f, "\n", 1);
	VFS_FLUSH(f);
	VFS_CLOSE(f);
}

void SV_MVDInfoRemove_f (void)
{
	char namebuf[MAX_QPATH];
	char *name, path[MAX_OSPATH];

	if (Cmd_Argc() < 2)
	{
		Con_Printf("usage:demoInfoRemove <demonum>\n<demonum> = * for currently recorded demo\n");
		return;
	}

	if (!strcmp(Cmd_Argv(1), "*"))
	{
		mvddest_t *active = SV_FindRecordFile(NULL, NULL);
		if (!active)
		{
			Con_Printf("Not recording demo!\n");
			return;
		}

		snprintf(path, MAX_OSPATH, "%s/%s", active->path, SV_MVDName2Txt(active->name));
	}
	else
	{
		name = SV_MVDTxTNum(namebuf, sizeof(namebuf), atoi(Cmd_Argv(1)));

		if (!name)
		{
			Con_Printf("invalid demo num\n");
			return;
		}

		snprintf(path, MAX_OSPATH, "%s/%s", sv_demoDir.string, name);
	}

	if (!FS_Remove(path, FS_GAMEONLY))
		Con_Printf("failed to remove the file\n");
	else Con_Printf("file removed\n");
}

void SV_MVDInfo_f (void)
{
	int len;
	char buf[64];
	vfsfile_t *f = NULL;
	char *name, path[MAX_OSPATH];

	if (Cmd_Argc() < 2)
	{
		Con_Printf("usage:demoinfo <demonum>\n<demonum> = * for currently recorded demo\n");
		return;
	}

	if (!strcmp(Cmd_Argv(1), "*"))
	{
		mvddest_t *active = SV_FindRecordFile(NULL, NULL);
		if (!active)
		{
			Con_Printf("Not recording demo!\n");
			return;
		}

		snprintf(path, MAX_OSPATH, "%s/%s", active->path, SV_MVDName2Txt(active->name));
	}
	else
	{
		name = SV_MVDTxTNum(buf, sizeof(buf), atoi(Cmd_Argv(1)));

		if (!name)
		{
			Con_Printf("invalid demo num\n");
			return;
		}

		snprintf(path, MAX_OSPATH, "%s/%s", sv_demoDir.string, name);
	}

	if ((f = FS_OpenVFS(path, "rt", FS_GAMEONLY)) == NULL)
	{
		Con_Printf("(empty)\n");
		return;
	}

	for(;;)
	{
		len = VFS_READ (f, buf, sizeof(buf)-1);
		if (len < 0)
			break;
		buf[len] = 0;
		Con_Printf("%s", buf);
	}

	VFS_CLOSE(f);
}








void SV_MVDPlayNum_f(void)
{
	char namebuf[MAX_QPATH];
	char *name;
	int		num;
	char	*val;

	if (Cmd_Argc() != 2)
	{
		Con_Printf("mvdplaynum <#>\n");
		return;
	}

	val = Cmd_Argv(1);
	if ((num = atoi(val)) == 0 && val[0] != '0')
	{
		Con_Printf("mvdplaynum <#>\n");
		return;
	}

	name = SV_MVDNum(namebuf, sizeof(namebuf), atoi(val));

	if (name)
		Cbuf_AddText(va("mvdplay %s\n", name), Cmd_ExecLevel);
	else
		Con_Printf("invalid demo num\n");
}



void SV_MVDInit(void)
{
	MVD_Init();

#ifdef SERVERONLY	//client command would conflict otherwise.
	Cmd_AddCommand ("record", SV_MVD_Record_f);
	Cmd_AddCommand ("stop", SV_MVDStop_f);
#endif
	Cmd_AddCommand ("cancel", SV_MVD_Cancel_f);
	Cmd_AddCommand ("qtvreverse", SV_MVD_QTVReverse_f);
	Cmd_AddCommand ("mvdrecord", SV_MVD_Record_f);
	Cmd_AddCommand ("easyrecord", SV_MVDEasyRecord_f);
	Cmd_AddCommand ("mvdstop", SV_MVDStop_f);
	Cmd_AddCommand ("mvdcancel", SV_MVD_Cancel_f);
	//Cmd_AddCommand ("mvdplaynum", SV_MVDPlayNum_f);
	Cmd_AddCommand ("mvdlist", SV_MVDList_f);
	Cmd_AddCommand ("demolist", SV_MVDList_f);
	Cmd_AddCommand ("rmdemo", SV_MVDRemove_f);
	Cmd_AddCommand ("rmdemonum", SV_MVDRemoveNum_f);

	Cvar_Register(&qtv_streamport, "MVD Streaming");
	Cvar_Register(&qtv_maxstreams, "MVD Streaming");
	Cvar_Register(&qtv_password, "MVD Streaming");
}

#endif
