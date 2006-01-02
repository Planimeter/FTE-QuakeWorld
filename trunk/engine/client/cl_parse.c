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
// cl_parse.c  -- parse a message received from the server

#include "quakedef.h"
#include "cl_ignore.h"

void CL_GetNumberedEntityInfo (int num, float *org, float *ang);
void CLNQ_ParseDarkPlaces5Entities(void);
void CL_SetStat (int pnum, int stat, int value);

int nq_dp_protocol;
int msgflags;


char *svc_strings[] =
{
	"svc_bad",
	"svc_nop",
	"svc_disconnect",
	"svc_updatestat",
	"svc_version",		// [long] server version
	"svc_setview",		// [short] entity number
	"svc_sound",			// <see code>
	"svc_time",			// [float] server time
	"svc_print",			// [string] null terminated string
	"svc_stufftext",		// [string] stuffed into client's console buffer
						// the string should be \n terminated
	"svc_setangle",		// [vec3] set the view angle to this absolute value

	"svc_serverdata",		// [long] version ...
	"svc_lightstyle",		// [qbyte] [string]
	"svc_updatename",		// [qbyte] [string]
	"svc_updatefrags",	// [qbyte] [short]
	"svc_clientdata",		// <shortbits + data>
	"svc_stopsound",		// <see code>
	"svc_updatecolors",	// [qbyte] [qbyte]
	"svc_particle",		// [vec3] <variable>
	"svc_damage",			// [qbyte] impact [qbyte] blood [vec3] from

	"svc_spawnstatic",
	"svc_spawnstatic2",
	"svc_spawnbaseline",

	"svc_temp_entity",		// <variable>
	"svc_setpause",
	"svc_signonnum",
	"svc_centerprint",
	"svc_killedmonster",
	"svc_foundsecret",
	"svc_spawnstaticsound",
	"svc_intermission",
	"svc_finale",

	"svc_cdtrack",
	"svc_sellscreen",

	"svc_smallkick",
	"svc_bigkick",

	"svc_updateping",
	"svc_updateentertime",

	"svc_updatestatlong",
	"svc_muzzleflash",
	"svc_updateuserinfo",
	"svc_download",
	"svc_playerinfo",
	"svc_nails",
	"svc_choke",
	"svc_modellist",
	"svc_soundlist",
	"svc_packetentities",
 	"svc_deltapacketentities",
	"svc_maxspeed",
	"svc_entgravity",

	"svc_setinfo",
	"svc_serverinfo",
	"svc_updatepl",
	"NEW svc_nails2",
	"OBSOLETE",
	"NEW svc_view2",
	"NEW svc_lightstylecol",
	"NEW svc_bulletentext",
	"NEW svc_lightnings",
	"NEW svc_modellistshort",
	"NEW svc_ftesetclientpersist",
	"NEW svc_setportalstate",
	"NEW svc_particle2",
	"NEW svc_particle3",
	"NEW svc_particle4",
	"NEW svc_spawnbaseline2",
	"NEW svc_customtempent",
	"NEW svc_choosesplitclient",
	"NEW PROTOCOL"
};

char *svc_nqstrings[] =
{
	"nqsvc_bad",
	"nqsvc_nop",
	"nqsvc_disconnect",
	"nqsvc_updatestat",
	"nqsvc_version",		// [long] server version
	"nqsvc_setview",		// [short] entity number
	"nqsvc_sound",			// <see code>
	"nqsvc_time",			// [float] server time
	"nqsvc_print",			// [string] null terminated string
	"nqsvc_stufftext",		// [string] stuffed into client's console buffer
						// the string should be \n terminated
	"nqsvc_setangle",		// [vec3] set the view angle to this absolute value

	"nqsvc_serverinfo",		// [long] version
						// [string] signon string
						// [string]..[0]model cache [string]...[0]sounds cache
						// [string]..[0]item cache
	"nqsvc_lightstyle",		// [qbyte] [string]
	"nqsvc_updatename",		// [qbyte] [string]
	"nqsvc_updatefrags",	// [qbyte] [short]
	"nqsvc_clientdata",		// <shortbits + data>
	"nqsvc_stopsound",		// <see code>
	"nqsvc_updatecolors",	// [qbyte] [qbyte]
	"nqsvc_particle",		// [vec3] <variable>
	"nqsvc_damage",			// [qbyte] impact [qbyte] blood [vec3] from

	"nqsvc_spawnstatic",
	"nqOBSOLETE svc_spawnbinary",
	"nqsvc_spawnbaseline",

	"nqsvc_temp_entity",		// <variable>
	"nqsvc_setpause",
	"nqsvc_signonnum",
	"nqsvc_centerprint",
	"nqsvc_killedmonster",
	"nqsvc_foundsecret",
	"nqsvc_spawnstaticsound",
	"nqsvc_intermission",
	"nqsvc_finale",			// [string] music [string] text
	"nqsvc_cdtrack",			// [qbyte] track [qbyte] looptrack
	"nqsvc_sellscreen",
	"nqsvc_cutscene",	//34

	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",		//40
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"NEW PROTOCOL",
	"dpsvc_cgame",		//50
	"dpsvc_updatestatubyte",
	"dpsvc_effect",
	"dpsvc_effect2",
	"dp6svc_precache/dp5svc_sound2",
	"dpsvc_spawnbaseline2",
	"dpsvc_spawnstatic2",
	"dpsvc_entities",
	"NEW PROTOCOL",
	"dpsvc_spawnstaticsound2"
};

extern cvar_t requiredownloads, cl_standardchat, msg_filter;
int	oldparsecountmod;
int	parsecountmod;
double	parsecounttime;

int		cl_spikeindex, cl_playerindex, cl_h_playerindex, cl_flagindex, cl_rocketindex, cl_grenadeindex, cl_gib1index, cl_gib2index, cl_gib3index;

#ifdef PEXT_LIGHTUPDATES
int		cl_lightningindex;
#endif

//=============================================================================

int packet_latency[NET_TIMINGS];

int CL_CalcNet (void)
{
	int		a, i;
	frame_t	*frame;
	int lost;
//	char st[80];

	for (i=cls.netchan.outgoing_sequence-UPDATE_BACKUP+1
		; i <= cls.netchan.outgoing_sequence
		; i++)
	{
		frame = &cl.frames[i&UPDATE_MASK];
		if (frame->receivedtime == -1)
			packet_latency[i&NET_TIMINGSMASK] = 9999;	// dropped
		else if (frame->receivedtime == -2)
			packet_latency[i&NET_TIMINGSMASK] = 10000;	// choked
		else if (frame->invalid)
			packet_latency[i&NET_TIMINGSMASK] = 9998;	// invalid delta
		else
			packet_latency[i&NET_TIMINGSMASK] = (frame->receivedtime - frame->senttime)*20;
	}

	lost = 0;
	for (a=0 ; a<NET_TIMINGS ; a++)
	{
		i = (cls.netchan.outgoing_sequence-a) & NET_TIMINGSMASK;
		if (packet_latency[i] == 9999)
			lost++;
	}
	return lost * 100 / NET_TIMINGS;
}

//=============================================================================

//note: this will overwrite existing files.
//returns true if the download is going to be downloaded after the call.
qboolean CL_EnqueDownload(char *filename, char *localname, qboolean verbose, qboolean ignorefailedlist)
{
	downloadlist_t *dl;
	if (!localname)
		localname = filename;

	if (strchr(localname, '\\') || strchr(localname, ':') || strstr(localname, ".."))
	{
		Con_Printf("Denying download of \"%s\"\n", filename);
		return false;
	}

	if (cls.demoplayback)
		return false;

	if (!ignorefailedlist)
	{
		for (dl = cl.faileddownloads; dl; dl = dl->next)	//yeah, so it failed... Ignore it.
		{
			if (!strcmp(dl->name, filename))
			{
				if (verbose)
					Con_Printf("We've failed to download \"%s\" already\n", filename);
				return false;
			}
		}
	}

	for (dl = cl.downloadlist; dl; dl = dl->next)	//It's already on our list. Ignore it.
	{
		if (!strcmp(dl->name, filename))
		{
			if (verbose)
				Con_Printf("Already waiting for \"%s\"\n", filename);
			return true;
		}
	}

	if (!strcmp(cls.downloadname, filename))
	{
		if (verbose)
			Con_Printf("Already downloading \"%s\"\n", filename);
		return true;
	}

	dl = Z_Malloc(sizeof(downloadlist_t));
	Q_strncpyz(dl->name, filename, sizeof(dl->name));
	Q_strncpyz(dl->localname, localname, sizeof(dl->localname));
	dl->next = cl.downloadlist;
	cl.downloadlist = dl;

	if (verbose)
		Con_Printf("Enqued download of \"%s\"\n", filename);

	return true;
}

void CL_DisenqueDownload(char *filename)
{
	downloadlist_t *dl, *nxt;
	if(cl.downloadlist)	//remove from enqued download list
	{
		if (!strcmp(cl.downloadlist->name, filename))
		{
			dl = cl.downloadlist;
			cl.downloadlist = cl.downloadlist->next;
			Z_Free(dl);
		}
		else
		{
			for (dl = cl.downloadlist->next; dl->next; dl = dl->next)
			{
				if (!strcmp(dl->next->name, filename))
				{
					nxt = dl->next->next;
					Z_Free(dl->next);
					dl->next = nxt;
					break;
				}
			}
		}
	}
}

void CL_SendDownloadRequest(char *filename, char *localname)
{
	strcpy (cls.downloadname, localname);
	Con_TPrintf (TL_DOWNLOADINGFILE, cls.downloadname);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	COM_StripExtension (localname, cls.downloadtempname);
	strcat (cls.downloadtempname, ".tmp");

	CL_SendClientCommand(true, "download %s", filename);

	//prevent ftp/http from changing stuff
	cls.downloadmethod = DL_QWPENDING;
	cls.downloadpercent = 0;

	CL_DisenqueDownload(filename);
}

//Do any reloading for the file that just reloaded.
void CL_FinishDownload(char *filename, char *tempname)
{
	int i;
	extern int mod_numknown;
	extern model_t	mod_known[];

	COM_RefreshFSCache_f();

	cls.downloadmethod = DL_NONE;

	// rename the temp file to it's final name
	if (tempname)
	{
		char oldn[MAX_OSPATH], newn[MAX_OSPATH];
		if (strcmp(tempname, filename))
		{
			if (strncmp(tempname,"skins/",6))
			{
				sprintf (oldn, "%s/%s", com_gamedir, tempname);
				sprintf (newn, "%s/%s", com_gamedir, filename);
			}
			else
			{
				sprintf (oldn, "qw/%s", tempname);
				sprintf (newn, "qw/%s", filename);
			}
			if (rename (oldn, newn))
				Con_TPrintf (TL_RENAMEFAILED);
		}
	}

	if (!strcmp(filename, "gfx/palette.lmp"))
	{
		Cbuf_AddText("vid_restart\n", RESTRICT_LOCAL);
	}
	else
	{
		for (i = 0; i < mod_numknown; i++)	//go and load this model now.
		{
			if (!strcmp(mod_known[i].name, filename))
			{
				Mod_ForName(mod_known[i].name, false);	//throw away result.
				break;
			}
		}
		Skin_FlushSkin(filename);
	}
}
/*
void MapDownload(char *name, qboolean gotornot)
{
	if (gotornot)	//yay
		return;


	CL_EnqueDownload(filename, false, false);
}
*/
/*
===============
CL_CheckOrDownloadFile

Returns true if the file exists, otherwise it attempts
to start a download from the server.
===============
*/
qboolean	CL_CheckOrDownloadFile (char *filename, char *localname, int nodelay)
{
	if (!localname)
		localname = filename;
	if (strstr (localname, ".."))
	{
		Con_TPrintf (TL_NORELATIVEPATHS);
		return true;
	}

	if (COM_FCheckExists (localname))
	{	// it exists, no need to download
		return true;
	}

	//ZOID - can't download when recording
	if (cls.demorecording)
	{
		Con_TPrintf (TL_NODOWNLOADINDEMO, filename);
		return true;
	}
	//ZOID - can't download when playback
	if (cls.demoplayback)
		return true;

	SCR_EndLoadingPlaque();	//release console.

/*	if (1)
	if (strncmp(filename, "maps/", 5))
	if (strcmp(filename + strlen(filename)-4, ".bsp"))
	{
		char base[MAX_QPATH];
		COM_FileBase(filename, base);
		HTTP_CL_Get(va("http://maps.quakeworld.nu/%s/download/", base), filename, MapDownload);
	}
*/
	if (CL_EnqueDownload(filename, localname, false, false))
		return !nodelay;
	else
		return true;
}

qboolean CL_CheckMD2Skins (char *name)
{
	md2_t *pheader;
	qbyte *precache_model;
	int precache_model_skin = 1;
	char *str;

	// checking for skins in the model
	precache_model = COM_LoadMallocFile (name);
	if (!precache_model) {
		precache_model_skin = 0;
		return false; // couldn't load it
	}
	if (LittleLong(*(unsigned *)precache_model) != MD2IDALIASHEADER) {
		// not an alias model
		BZ_Free(precache_model);
		precache_model = 0;
		precache_model_skin = 0;
		return false;
	}
	pheader = (md2_t *)precache_model;
	if (LittleLong (pheader->version) != MD2ALIAS_VERSION) {
		BZ_Free(precache_model);
		precache_model = 0;
		precache_model_skin = 0;
		return false;
	}

	pheader = (md2_t *)precache_model;

	while (precache_model_skin - 1 < LittleLong(pheader->num_skins))
	{
		str = (char *)precache_model +
			LittleLong(pheader->ofs_skins) +
			(precache_model_skin - 1)*MD2MAX_SKINNAME;
		COM_CleanUpPath(str);
		if (!CL_CheckOrDownloadFile(str, str, false))
		{
			precache_model_skin++;

			BZ_Free(precache_model);
			precache_model=NULL;
			return true; // started a download
		}
		precache_model_skin++;
	}
	if (precache_model) {
		BZ_Free(precache_model);
		precache_model=NULL;
	}
	precache_model_skin = 0;

	return false;
}
/*
=================
Model_NextDownload
=================
*/
void Sound_NextDownload (void);
void Model_NextDownload (void)
{
//	char *twf;
	char	*s;
	int		i;
	extern	char gamedirfile[];

	if (cls.downloadnumber == 0)
	{
		Con_TPrintf (TLC_CHECKINGMODELS);
		cls.downloadnumber = 1;

		cl.worldmodel = NULL;
	}
#ifdef Q2CLIENT
	if (cls.protocol == CP_QUAKE2)
	{
		R_SetSky(cl.skyname, cl.skyrotate, cl.skyaxis);
		for (i = 0; i < Q2MAX_IMAGES; i++)
		{
			char picname[256];
			if (!*cl.image_name[i])
				continue;
			sprintf(picname, "pics/%s.pcx", cl.image_name[i]);
			if (!CL_CheckOrDownloadFile(picname, picname, false))
				return;
		}
		if (!CLQ2_RegisterTEntModels())
			return;
	}
#endif

	cls.downloadtype = dl_model;

	for (
		; cl.model_name[cls.downloadnumber][0]
		; cls.downloadnumber++)
	{
		s = cl.model_name[cls.downloadnumber];
		if (s[0] == '*')
			continue;	// inline brush model

		if (!stricmp(COM_FileExtension(s), "dsp"))	//doom sprites are weird, and not really downloadable via this system
			continue;

#ifdef Q2CLIENT
		if (cls.protocol == CP_QUAKE2 && s[0] == '#')	//this is a vweap
			continue;
#endif

		if (!CL_CheckOrDownloadFile(s, s, cls.downloadnumber==1))	//world is required to be loaded.
			return;		// started a download

		if (strstr(s, ".md2"))
			if (CL_CheckMD2Skins(s))
				return;
	}

	CL_AllowIndependantSendCmd(false);	//stop it now, the indep stuff *could* require model tracing.

	if (cl.playernum[0] == -1)
	{	//q2 cinematic - don't load the models.
		cl.worldmodel = cl.model_precache[1] = Mod_ForName ("", false);
	}
	else if (!cl.worldmodel)
	{
		for (i=1 ; i<MAX_MODELS ; i++)
		{
			if (!cl.model_name[i][0])
				break;

			Hunk_Check();

			cl.model_precache[i] = NULL;
			cl.model_precache[i] = Mod_ForName (cl.model_name[i], false);

			Hunk_Check();

			if (!cl.model_precache[i] || (i == 1 && (cl.model_precache[i]->type == mod_dummy || cl.model_precache[i]->needload)))
			{
				Con_TPrintf (TL_FILE_X_MISSING, cl.model_name[i]);
				Con_TPrintf (TL_GETACLIENTPACK, gamedirfile);
				CL_Disconnect ();
				return;
			}

			S_ExtraUpdate();
		}

		cl.worldmodel = cl.model_precache[1];

		if (!cl.worldmodel)
			Host_EndGame("Worldmodel wasn't sent\n");
	}

	if (cl.worldmodel->type != mod_brush && cl.worldmodel->type != mod_heightmap)
		Host_EndGame("Worldmodel must be a bsp of some sort\n");

	if (!R_CheckSky())
		return;
	if (!Wad_NextDownload())	//world is required to be loaded.
		return;		// started a download

	{
		extern model_t *loadmodel;
		loadmodel = cl.worldmodel;
		if (!cl.worldmodel)
			Host_EndGame("No worldmodel was loaded\n");

		if (R_PreNewMap)
			R_PreNewMap();

		Mod_NowLoadExternal();
	}


	// all done
	R_NewMap ();

	Hunk_Check ();		// make sure nothing is hurt
#ifdef Q2CLIENT
	if (cls.protocol == CP_QUAKE2)
	{
		cls.downloadnumber = 0;
		Skin_NextDownload();
	}
	else
#endif
	{
	// done with modellist, request first of static signon messages
//		CL_SendClientCommand("prespawn %i 0 %i", cl.servercount, cl.worldmodel->checksum2);
		CL_SendClientCommand(true, prespawn_name, cl.servercount, LittleLong(cl.worldmodel->checksum2));
	}
}

/*
=================
Sound_NextDownload
=================
*/
void Sound_NextDownload (void)
{
	char	*s;
	int		i;

	cls.downloadtype = dl_sound;

	if (cls.downloadnumber == 0)
	{
		Con_TPrintf (TLC_CHECKINGSOUNDS);
#ifdef CSQC_DAT
		if (cls.fteprotocolextensions & PEXT_CSQC)
		{
			s = Info_ValueForKey(cl.serverinfo, "*csprogs");
			if (*s || cls.demoplayback)	//only allow csqc if the server says so, and the 'checksum' matches.
			{
				extern cvar_t allow_download_csprogs;
				unsigned int chksum = strtoul(s, NULL, 0);
				if (allow_download_csprogs.value)
				{
					char *str = va("csprogsvers/%x.dat", chksum);
					if (!CL_CheckOrDownloadFile("csprogs.dat", str, true))
					{
						cls.downloadnumber = 1;
						return;
					}
				}
				else
				{
					Con_Printf("Not downloading csprogs.dat\n");
				}
			}
		}
		cls.downloadnumber = 1;
#endif
	}

#ifdef CSQC_DAT
	if (cls.downloadnumber == 1 && cls.fteprotocolextensions & PEXT_CSQC)
	{
		s = Info_ValueForKey(cl.serverinfo, "*csprogs");
		if (*s || cls.demoplayback)	//only allow csqc if the server says so, and the 'checksum' matches.
		{
			unsigned int chksum = strtoul(s, NULL, 0);
			if (CSQC_Init(chksum))
				CL_SendClientCommand(true, "enablecsqc");
			else
				Sbar_Start();	//try and start this before we're actually on the server,
								//this'll stop the mod from sending so much stuffed data at us, whilst we're frozen while trying to load.
								//hopefully this'll make it more robust.
								//csqc is expected to use it's own huds, or to run on decent servers. :p
		}
	}
#endif

	for (
		; cl.sound_name[cls.downloadnumber][0]
		; cls.downloadnumber++)
	{
		s = cl.sound_name[cls.downloadnumber];
		if (*s == '*')
			continue;
		if (!CL_CheckOrDownloadFile(va("sound/%s",s), NULL, false))
			return;		// started a download
	}

	for (i=1 ; i<MAX_SOUNDS ; i++)
	{
		if (!cl.sound_name[i][0])
			break;
		cl.sound_precache[i] = S_PrecacheSound (cl.sound_name[i]);

		S_ExtraUpdate();
	}

	// done with sounds, request models now
	memset (cl.model_precache, 0, sizeof(cl.model_precache));
	cl_playerindex = -1;
	cl_h_playerindex = -1;
	cl_spikeindex = -1;
	cl_flagindex = -1;
	cl_rocketindex = -1;
	cl_grenadeindex = -1;
	cl_gib1index = -1;
	cl_gib2index = -1;
	cl_gib3index = -1;
#ifdef PEXT_LIGHTUPDATES
	cl_lightningindex = -1;
#endif
#ifdef Q2CLIENT
	if (cls.protocol == CP_QUAKE2)
	{
		cls.downloadnumber = 0;
		Model_NextDownload();
	}
	else
#endif
	{
		if (CL_RemoveClientCommands("modellist"))
			Con_Printf("Multiple modellists\n");
//		CL_SendClientCommand ("modellist %i 0", cl.servercount);
		CL_SendClientCommand (true, modellist_name, cl.servercount, 0);
	}
}


/*
======================
CL_RequestNextDownload
======================
*/
void CL_RequestNextDownload (void)
{
	if (cl.downloadlist)
	{
		if (!COM_FCheckExists (cl.downloadlist->localname))
			CL_SendDownloadRequest(cl.downloadlist->name, cl.downloadlist->localname);
		else
		{
			Con_Printf("Already have %s\n", cl.downloadlist->localname);
			CL_DisenqueDownload(cl.downloadlist->name);
		}
		return;
	}
	switch (cls.downloadtype)
	{
	case dl_single:
	case dl_singlestuffed:
		break;
	case dl_skin:
		Skin_NextDownload ();
		break;
	case dl_model:
		Model_NextDownload ();
		break;
	case dl_sound:
		Sound_NextDownload ();
		break;
	case dl_none:
	default:
		Con_DPrintf("Unknown download type.\n");
	}
}

int CL_RequestADownloadChunk(void);
void CL_SendDownloadReq(sizebuf_t *msg)
{
	if (cl.downloadlist && !cls.downloadmethod)
	{
		CL_RequestNextDownload();
		return;
	}

#ifdef PEXT_CHUNKEDDOWNLOADS
	if (cls.downloadmethod == DL_QWCHUNKS)
	{
		int i = CL_RequestADownloadChunk();
		if (i < 0)
		{
			//we can stop downloading now.
		}
		else
		{
			CL_SendClientCommand(false, "nextdl %i\n", i);
		}
		return;
	}
#endif
}

#ifdef PEXT_ZLIBDL
#ifdef _WIN32
#define ZEXPORT VARGS
#include "../../zip/zlib.h"

//# pragma comment (lib, "zip/zlib.lib")
#else
#include <zlib.h>
#endif

char *ZLibDownloadDecode(int *messagesize, char *input, int finalsize)
{
	char *outbuf = Hunk_TempAlloc(finalsize);
	z_stream zs;

	*messagesize = (*(short*)input);
	input+=2;

	if (!*messagesize)
	{
		*messagesize = finalsize+2;
		return input;
	}

	memset(&zs, 0, sizeof(zs));


	zs.next_in = input;
    zs.avail_in = *messagesize;	//tell it that it has a lot. Possibly a bad idea.
    zs.total_in = 0;

    zs.next_out = outbuf;
    zs.avail_out = finalsize;	//this is the limiter.
    zs.total_out = 0;

    zs.data_type = Z_BINARY;

	inflateInit(&zs);
	inflate(&zs, Z_FINISH);	//decompress it in one go.
	inflateEnd(&zs);

	*messagesize = zs.total_in+2;
	return outbuf;
}
#endif

void CL_DownloadFailed(char *name)
{
	//add this to our failed list. (so we don't try downloading it again...)
	downloadlist_t *failed;
	failed = Z_Malloc(sizeof(downloadlist_t));
	failed->next = cl.faileddownloads;
	cl.faileddownloads = failed;
	Q_strncpyz(failed->name, name, sizeof(failed->name));

	cls.downloadmethod = DL_NONE;
}

float downloadstarttime;
#ifdef PEXT_CHUNKEDDOWNLOADS
#define MAXBLOCKS 64	//must be power of 2
#define DLBLOCKSIZE 1024
int downloadsize;
int receivedbytes;
int recievedblock[MAXBLOCKS];
int firstblock;
int blockcycle;
void CL_ParseChunkedDownload(void)
{
	qbyte	*svname;
	qbyte	osname[MAX_OSPATH];
	int totalsize;
	int chunknum;
	char data[DLBLOCKSIZE];

	chunknum = MSG_ReadLong();
	if (chunknum < 0)
	{
		totalsize = MSG_ReadLong();
		svname = MSG_ReadString();
		if (cls.demoplayback)
			return;

		if (totalsize < 0)
		{
			if (totalsize == -2)
				Con_Printf("Server permissions deny downloading file %s\n", svname);
			else
				Con_Printf("Couldn't find file %s on the server\n", svname);

			CL_DownloadFailed(svname);

			CL_RequestNextDownload();
			return;
		}

		if (cls.downloadmethod == DL_QWCHUNKS)
			Host_EndGame("Received second download - \"%s\"\n", svname);

		if (!strcmp(cls.downloadname, svname))
			Host_EndGame("Server sent the wrong download - \"%s\" instead of \"%s\"\n", svname, cls.downloadname);

		//start the new download
		cls.downloadmethod = DL_QWCHUNKS;
		cls.downloadpercent = 0;
		downloadsize = totalsize;

		downloadstarttime = Sys_DoubleTime();

		strcpy(cls.downloadname, svname);
		COM_StripExtension(svname, cls.downloadtempname);
		COM_DefaultExtension(cls.downloadtempname, ".tmp");

		if (!strncmp(cls.downloadtempname,"skins/",6))
			sprintf (osname, "qw/%s", cls.downloadtempname);	//skins go to quake/qw/skins. never quake/gamedir/skins (blame id)
		else
			sprintf (osname, "%s/%s", com_gamedir, cls.downloadtempname);
		COM_CreatePath (osname);
		cls.downloadqw = fopen (osname, "wb");

		if (!cls.downloadqw)
		{
			CL_DownloadFailed(svname);
			return;
		}

		firstblock = 0;
		receivedbytes = 0;
		blockcycle = -1;	//so it requests 0 first. :)
		memset(recievedblock, 0, sizeof(recievedblock));
		return;
	}

//	Con_Printf("Received dl block %i: ", chunknum);

	MSG_ReadData(data, DLBLOCKSIZE);

	if (cls.demoplayback)
	{	//err, yeah, when playing demos we don't actually pay any attention to this.
		return;
	}
	if (chunknum < firstblock)
	{
//		Con_Printf("too old\n", chunknum);
		return;
	}
	if (chunknum-firstblock >= MAXBLOCKS)
	{
//		Con_Printf("^1too new!\n", chunknum);
		return;
	}

	if (recievedblock[chunknum&(MAXBLOCKS-1)])
	{
//		Con_Printf("duplicated\n", chunknum);
		return;
	}
//	Con_Printf("usable\n", chunknum);
	receivedbytes+=DLBLOCKSIZE;
	recievedblock[chunknum&(MAXBLOCKS-1)] = true;

	while(recievedblock[firstblock&(MAXBLOCKS-1)])
	{
		recievedblock[firstblock&(MAXBLOCKS-1)] = false;
		firstblock++;
	}

	fseek(cls.downloadqw, chunknum*DLBLOCKSIZE, SEEK_SET);
	if (downloadsize - chunknum*DLBLOCKSIZE < DLBLOCKSIZE)	//final block is actually meant to be smaller than we recieve.
		fwrite(data, 1, downloadsize - chunknum*DLBLOCKSIZE, cls.downloadqw);
	else
		fwrite(data, 1, DLBLOCKSIZE, cls.downloadqw);

	cls.downloadpercent = receivedbytes/(float)downloadsize*100;
}

int CL_RequestADownloadChunk(void)
{
	int i;
	int b;

	if (cls.downloadmethod != DL_QWCHUNKS)
	{
		Con_Printf("download not initiated\n");
		return 0;
	}

	blockcycle++;
	for (i = 0; i < MAXBLOCKS; i++)
	{
		b = ((i+blockcycle)&(MAXBLOCKS-1))
			+ firstblock;
		if (!recievedblock[b&(MAXBLOCKS-1)])	//don't ask for ones we've already got.
		{
			if (b >= (downloadsize+DLBLOCKSIZE-1)/DLBLOCKSIZE)	//don't ask for blocks that are over the size of the file.
				continue;
//			Con_Printf("Requesting block %i\n", b);
			return b;
		}
	}

//	Con_Printf("^1 EOF?\n");

	fclose(cls.downloadqw);
	CL_FinishDownload(cls.downloadname, cls.downloadtempname);

	Con_Printf("Download took %i seconds\n", (int)(Sys_DoubleTime() - downloadstarttime));

	*cls.downloadname = '\0';
	cls.downloadqw = NULL;
	cls.downloadpercent = 0;

	return -1;
}

#endif

/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
void CL_ParseDownload (void)
{
	int		size, percent;
	qbyte	name[1024];

#ifdef PEXT_CHUNKEDDOWNLOADS
	if (cls.fteprotocolextensions & PEXT_CHUNKEDDOWNLOADS)
	{
		CL_ParseChunkedDownload();
		return;
	}
#endif

	// read the data
	size = MSG_ReadShort ();
	percent = MSG_ReadByte ();

	if (cls.demoplayback)
	{
		if (size > 0)
			msg_readcount += size;
		return; // not in demo playback
	}

	if (!*cls.downloadname)	//huh... that's not right...
	{
		Con_Printf("^1Warning^7: Server sending unknown file.\n");
		strcpy(cls.downloadname, "unknown.txt");
		strcpy(cls.downloadtempname, "unknown.tmp");
	}

	if (size < 0)
	{
		Con_TPrintf (TL_FILENOTFOUND);
		if (cls.downloadqw)
		{
			Con_TPrintf (TL_CLS_DOWNLOAD_ISSET);
			fclose (cls.downloadqw);
			cls.downloadqw = NULL;
		}
		if (cl.downloadlist && !strcmp(cl.downloadlist->name, cls.downloadname))
		{
			downloadlist_t *next;
			next = cl.downloadlist->next;
			Z_Free(cl.downloadlist);
			cl.downloadlist = next;
		}

		CL_DownloadFailed(cls.downloadname);

		CL_RequestNextDownload ();
		return;
	}

	// open the file if not opened yet
	if (!cls.downloadqw)
	{
		if (strncmp(cls.downloadtempname,"skins/",6))
			sprintf (name, "%s/%s", com_gamedir, cls.downloadtempname);
		else
			sprintf (name, "qw/%s", cls.downloadtempname);

		COM_CreatePath (name);

		cls.downloadqw = fopen (name, "wb");
		if (!cls.downloadqw)
		{
			msg_readcount += size;
			Con_TPrintf (TL_FAILEDTOOPEN, cls.downloadtempname);
			CL_DownloadFailed(cls.downloadname);
			CL_RequestNextDownload ();
			return;
		}

		downloadstarttime = Sys_DoubleTime();
		SCR_EndLoadingPlaque();
	}
#ifdef PEXT_ZLIBDL
	if (percent >= 101 && percent <= 201)// && cls.fteprotocolextensions & PEXT_ZLIBDL)
	{
		int compsize;

		percent = percent - 101;

		fwrite (ZLibDownloadDecode(&compsize, net_message.data + msg_readcount, size), 1, size, cls.download);

		msg_readcount += compsize;
	}
	else
#endif
	{
		fwrite (net_message.data + msg_readcount, 1, size, cls.downloadqw);
		msg_readcount += size;
	}

	if (cls.downloadmethod == DL_QWPENDING)
		cls.downloadmethod = DL_QW;

	if (percent != 100)
	{
// change display routines by zoid
		// request next block
		cls.downloadpercent = percent;

		CL_SendClientCommand(true, "nextdl");
	}
	else
	{
		fclose (cls.downloadqw);

		CL_FinishDownload(cls.downloadname, cls.downloadtempname);
		*cls.downloadname = '\0';
		cls.downloadqw = NULL;
		cls.downloadpercent = 0;

		Con_Printf("Download took %i seconds\n", (int)(Sys_DoubleTime() - downloadstarttime));

		// get another file if needed

		CL_RequestNextDownload ();
	}
}

static qbyte *upload_data;
static int upload_pos;
static int upload_size;

void CL_NextUpload(void)
{
	qbyte	buffer[1024];
	int		r;
	int		percent;
	int		size;

	if (!upload_data)
		return;

	r = upload_size - upload_pos;
	if (r > 768)
		r = 768;
	memcpy(buffer, upload_data + upload_pos, r);
	MSG_WriteByte (&cls.netchan.message, clc_upload);
	MSG_WriteShort (&cls.netchan.message, r);

	upload_pos += r;
	size = upload_size;
	if (!size)
		size = 1;
	percent = upload_pos*100/size;
	MSG_WriteByte (&cls.netchan.message, percent);
	SZ_Write (&cls.netchan.message, buffer, r);

Con_DPrintf ("UPLOAD: %6d: %d written\n", upload_pos - r, r);

	if (upload_pos != upload_size)
		return;

	Con_TPrintf (TL_UPLOADCOMPLEATE);

	BZ_Free(upload_data);
	upload_data = 0;
	upload_pos = upload_size = 0;
}

void CL_StartUpload (qbyte *data, int size)
{
	if (cls.state < ca_onserver)
		return; // gotta be connected

	// override
	if (upload_data)
		BZ_Free(upload_data);

Con_DPrintf("Upload starting of %d...\n", size);

	upload_data = BZ_Malloc(size);
	memcpy(upload_data, data, size);
	upload_size = size;
	upload_pos = 0;

	CL_NextUpload();
}

qboolean CL_IsUploading(void)
{
	if (upload_data)
		return true;
	return false;
}

void CL_StopUpload(void)
{
	if (upload_data)
		BZ_Free(upload_data);
	upload_data = NULL;
}

/*
=====================================================================

  SERVER CONNECTING MESSAGES

=====================================================================
*/
#ifdef CLIENTONLY
float nextdemotime;
#endif

/*
==================
CL_ParseServerData
==================
*/
void CL_ParseServerData (void)
{
	int pnum;
	int clnum;
	char	*str;
	int protover, svcnt;

	float maxspeed, entgrav;

	Con_DPrintf ("Serverdata packet received.\n");
//
// wipe the client_state_t struct
//

	SCR_BeginLoadingPlaque();

// parse protocol version number
// allow 2.2 and 2.29 demos to play
#ifdef PROTOCOL_VERSION_FTE
	cls.fteprotocolextensions=0;
	for(;;)
	{
		protover = MSG_ReadLong ();
		if (protover == PROTOCOL_VERSION_FTE)
		{
			cls.fteprotocolextensions =  MSG_ReadLong();
			Con_TPrintf (TL_FTEEXTENSIONS, cls.fteprotocolextensions);
			continue;
		}
		if (protover == PROTOCOL_VERSION)	//this ends the version info
			break;
		if (cls.demoplayback && (protover == 26 || protover == 27 || protover == 28))	//older versions, maintain demo compatability.
			break;
		Host_EndGame ("Server returned version %i, not %i\nYou probably need to upgrade.\nCheck http://www.quakeworld.net/", protover, PROTOCOL_VERSION);
	}
#else
	protover = MSG_ReadLong ();
	if (protover != PROTOCOL_VERSION &&
		!(cls.demoplayback && (protover == 26 || protover == 27 || protover == 28)))
		Host_EndGame ("Server returned version %i, not %i\nYou probably need to upgrade.\nCheck http://www.quakeworld.net/", protover, PROTOCOL_VERSION);
#endif

	if (cls.fteprotocolextensions & PEXT_FLOATCOORDS)
	{
		sizeofcoord = 4;
		sizeofangle = 2;
	}
	else
	{
		sizeofcoord = 2;
		sizeofangle = 1;
	}

	svcnt = MSG_ReadLong ();

	// game directory
	str = MSG_ReadString ();

#ifndef CLIENTONLY
	if (!sv.state)
#endif
	{
		COM_FlushTempoaryPacks();
		COM_Gamedir(str);
#ifndef CLIENTONLY
		Info_SetValueForStarKey (svs.info, "*gamedir", str, MAX_SERVERINFO_STRING);
#endif
		COM_FlushFSCache();
	}

	CL_ClearState ();
	P_NewServer();
	Stats_NewMap();
	cl.servercount = svcnt;

	cl.teamfortress = !Q_strcasecmp(str, "fortress");

	if (cl.gamedirchanged)
	{
		cl.gamedirchanged = false;
#ifndef CLIENTONLY
		if (!sv.state)
#endif
			Wads_Flush();

		T_FreeStrings();
	}

	if (cls.demoplayback == DPB_MVD)
	{
		int i;
		extern float nextdemotime;
		cls.netchan.last_received = nextdemotime = /*olddemotime =*/ MSG_ReadFloat();
		cl.playernum[0] = MAX_CLIENTS - 1;
		cl.spectator = true;
		for (i = 0; i < UPDATE_BACKUP; i++)
			cl.frames[i].playerstate[cl.playernum[0]].pm_type = PM_SPECTATOR;

		cl.splitclients = 1;
		CL_RegisterSplitCommands();
	}
	else
	{
		// parse player slot, high bit means spectator
		pnum = MSG_ReadByte ();
		for (clnum = 0; ; clnum++)
		{
			cl.playernum[clnum] = pnum;
			if (cl.playernum[clnum] & 128)
			{
				cl.spectator = true;
				cl.playernum[clnum] &= ~128;
			}

			if (!(cls.fteprotocolextensions & PEXT_SPLITSCREEN))
				break;

			pnum = MSG_ReadByte ();

			if (pnum == 128)
				break;

			if (clnum == MAX_SPLITS)
				Host_EndGame("Server sent us too many alternate clients\n");
		}
		cl.splitclients = clnum+1;
		CL_RegisterSplitCommands();
	}

	// get the full level name
	str = MSG_ReadString ();
	Q_strncpyz (cl.levelname, str, sizeof(cl.levelname));

	// get the movevars
	movevars.gravity			= MSG_ReadFloat();
	movevars.stopspeed			= MSG_ReadFloat();
	maxspeed					= MSG_ReadFloat();
	movevars.spectatormaxspeed	= MSG_ReadFloat();
	movevars.accelerate			= MSG_ReadFloat();
	movevars.airaccelerate		= MSG_ReadFloat();
	movevars.wateraccelerate	= MSG_ReadFloat();
	movevars.friction			= MSG_ReadFloat();
	movevars.waterfriction		= MSG_ReadFloat();
	entgrav						= MSG_ReadFloat();

	for (clnum = 0; clnum < cl.splitclients; clnum++)
	{
		cl.maxspeed[clnum] = maxspeed;
		cl.entgravity[clnum] = entgrav;
	}

	// seperate the printfs so the server message can have a color
	Con_TPrintf (TLC_LINEBREAK_NEWLEVEL);
	Con_TPrintf (TLC_PC_PS_NL, 2, str);

	if (CL_RemoveClientCommands("new"))	//mvdsv is really appaling some times.
	{
	//	Con_Printf("Multiple 'new' commands?!?!? This server needs reinstalling!\n");
	}

	memset(cl.sound_name, 0, sizeof(cl.sound_name));
#ifdef PEXT_PK3DOWNLOADS
	if (cls.fteprotocolextensions & PEXT_PK3DOWNLOADS)	//instead of going for a soundlist, go for the pk3 list instead. The server will make us go for the soundlist after.
	{
		CL_SendClientCommand ("pk3list %i 0", cl.servercount, 0);
	}
	else
#endif
	{
		// ask for the sound list next
//		CL_SendClientCommand ("soundlist %i 0", cl.servercount);
		CL_SendClientCommand (true, soundlist_name, cl.servercount, 0);
	}

	// now waiting for downloads, etc
	cls.state = ca_onserver;

#ifdef VM_CG
	CG_Stop();
#endif
#ifdef CSQC_DAT
	CSQC_Shutdown();	//revive it when we get the serverinfo saying the checksum.
#endif
}

void CLQ2_ParseServerData (void)
{
	char	*str;
	int		i;
	int svcnt;
//	int cflag;

	sizeofcoord = 2;
	sizeofangle = 1;

	Con_DPrintf ("Serverdata packet received.\n");
//
// wipe the client_state_t struct
//
	SCR_BeginLoadingPlaque();
//	CL_ClearState ();
	cls.state = ca_onserver;

// parse protocol version number
	i = MSG_ReadLong ();
//	cls.serverProtocol = i;

	if (i > PROTOCOL_VERSION_Q2 || i < PROTOCOL_VERSION_Q2_MIN)
		Host_EndGame ("Server returned version %i, not %i", i, PROTOCOL_VERSION_Q2);

	svcnt = MSG_ReadLong ();
	/*cl.attractloop =*/ MSG_ReadByte ();

	// game directory
	str = MSG_ReadString ();
//	strncpy (cl.gamedir, str, sizeof(cl.gamedir)-1);

	// set gamedir
	if (!*str)
		COM_Gamedir("baseq2");
	else
		COM_Gamedir(str);
	COM_FlushFSCache();
//	if ((*str && (!fs_gamedirvar->string || !*fs_gamedirvar->string || strcmp(fs_gamedirvar->string, str))) || (!*str && (fs_gamedirvar->string || *fs_gamedirvar->string)))
//		Cvar_Set("game", str);

	CL_ClearState ();
	cl.minpitch = -89;
	cl.maxpitch = 89;
	cl.servercount = svcnt;

	Stats_NewMap();


	// parse player entity number
	cl.playernum[0] = MSG_ReadShort ();
	cl.splitclients = 1;
	CL_RegisterSplitCommands();
	cl.spectator = false;

	// get the full level name
	str = MSG_ReadString ();
	Q_strncpyz (cl.levelname, str, sizeof(cl.levelname));

	if (cl.playernum[0] == -1)
	{	// playing a cinematic or showing a pic, not a level
		SCR_EndLoadingPlaque();
		if (!Media_PlayFilm(str))
			Con_TPrintf (TLC_NOQ2CINEMATICSSUPPORT, cl.servercount);
		else
			CL_MakeActive("Quake2");
	}
	else
	{
		// seperate the printfs so the server message can have a color
		Con_TPrintf (TLC_LINEBREAK_NEWLEVEL);
		Con_TPrintf (TLC_PC_PS_NL, 2, str);

		Media_PlayFilm("");

		// need to prep refresh at next oportunity
		//cl.refresh_prepped = false;
	}

	P_NewServer();

	if (R_PreNewMap)
		R_PreNewMap();
}



#ifdef NQPROT
//FIXME: move to header
void CL_KeepaliveMessage(void){}
void CLNQ_ParseServerData(void)		//Doesn't change gamedir - use with caution.
{
	int	nummodels, numsounds, i;
	char	*str;
	int protover;
	if (developer.value)
		Con_TPrintf (TLC_GOTSVDATAPACKET);
	CL_ClearState ();
	Stats_NewMap();
	P_NewServer();

	protover = MSG_ReadLong ();

	sizeofcoord = 2;
	sizeofangle = 1;

	nq_dp_protocol = 0;
	cls.z_ext = 0;

	if (protover == 250)
		Host_EndGame ("Nehahra demo net protocol is not supported\n");
	else if (protover == 3502)
	{
		//darkplaces5
		nq_dp_protocol = 5;
		sizeofcoord = 4;
		sizeofangle = 2;

		Con_DPrintf("DP5 protocols\n");
	}
	else if (protover == DP6_PROTOCOL_VERSION)
	{
		//darkplaces6 (it's a small difference from dp5)
		nq_dp_protocol = 6;
		sizeofcoord = 4;
		sizeofangle = 2;

		cls.z_ext = Z_EXT_VIEWHEIGHT;

		Con_DPrintf("DP6 protocols\n");
	}
	else if (protover == DP7_PROTOCOL_VERSION)
	{
		//darkplaces7 (it's a small difference from dp5)
		nq_dp_protocol = 7;
		sizeofcoord = 4;
		sizeofangle = 2;

		cls.z_ext = Z_EXT_VIEWHEIGHT;

		Con_DPrintf("DP7 protocols\n");
	}
	else if (protover != NQ_PROTOCOL_VERSION)
	{
		Host_EndGame ("Server returned version %i, not %i\nYou will need to use a different client.", protover, NQ_PROTOCOL_VERSION);
	}
	else
	{
		Con_DPrintf("Standard NQ protocols\n");
	}

	if (MSG_ReadByte() > MAX_CLIENTS)
	{
		Con_Printf ("Warning, this server supports more than 32 clients, additional clients will do bad things\n");
	}

	cl.splitclients = 1;
	CL_RegisterSplitCommands();

	/*cl.gametype =*/ MSG_ReadByte ();

	str = MSG_ReadString ();
	Q_strncpyz (cl.levelname, str, sizeof(cl.levelname));

	// seperate the printfs so the server message can have a color
	Con_TPrintf (TLC_LINEBREAK_NEWLEVEL);
	Con_TPrintf (TLC_PC_PS_NL, 2, str);

	SCR_BeginLoadingPlaque();

	if (R_PreNewMap)
		R_PreNewMap();

	memset (cl.model_name, 0, sizeof(cl.model_name));
	for (nummodels=1 ; ; nummodels++)
	{
		str = MSG_ReadString ();
		if (!str[0])
			break;
		if (nummodels==MAX_MODELS)
		{
			Con_TPrintf (TLC_TOOMANYMODELPRECACHES);
			return;
		}
		strcpy (cl.model_name[nummodels], str);
		Mod_TouchModel (str);

//		cl.model_precache[nummodels] = Mod_ForName (cl.model_name[nummodels], false);
	}

	memset (cl.sound_name, 0, sizeof(cl.sound_name));
	for (numsounds=1 ; ; numsounds++)
	{
		str = MSG_ReadString ();
		if (!str[0])
			break;
		if (numsounds==MAX_SOUNDS)
		{
			Con_TPrintf (TLC_TOOMANYSOUNDPRECACHES);
			return;
		}
		strcpy (cl.sound_name[numsounds], str);
		S_TouchSound (str);

//		cl.sound_precache[numsounds] = S_PrecacheSound (cl.sound_name[numsounds]);
	}

//
// now we try to load everything else until a cache allocation fails
//

	for (i=1 ; i<nummodels ; i++)
	{
		cl.model_precache[i] = Mod_ForName (cl.model_name[i], i==1);
//		if (!ignorenonprecached.value || i == 1)	//need world
		{
			if (cl.model_precache[i] == NULL)
			{
				Host_EndGame("Model %s not found\n", cl.model_name[i]);
			}
		}
		CL_KeepaliveMessage ();
	}

	S_BeginPrecaching ();
	for (i=1 ; i<numsounds ; i++)
	{
		cl.sound_precache[i] = S_PrecacheSound (cl.sound_name[i]);
		CL_KeepaliveMessage ();
	}
	S_EndPrecaching ();

	cl.worldmodel = cl.model_precache[1];

	R_NewMap ();

	if (cls.demoplayback)
		CSQC_Init(0);

	SCR_EndLoadingPlaque();

	Hunk_Check ();		// make sure nothing is hurt

	cls.state = ca_onserver;
}
void CLNQ_SignonReply (void)
{
	extern cvar_t	topcolor;
	extern cvar_t	bottomcolor;
	extern cvar_t	rate;
	extern cvar_t	model;
	extern cvar_t	skin;

Con_DPrintf ("CL_SignonReply: %i\n", cls.signon);

	switch (cls.signon)
	{
	case 1:
		CL_SendClientCommand(true, "prespawn");
		break;

	case 2:
		CL_SendClientCommand(true, "name \"%s\"\n", name.string);
		name.modified = false;

		CL_SendClientCommand(true, "color %i %i\n", (int)topcolor.value, (int)bottomcolor.value);

		CL_SendClientCommand(true, "spawn %s", "");

		if (nq_dp_protocol)	//dp needs a couple of extras to work properly.
		{
			CL_SendClientCommand(true, "rate %s", rate.string);

			CL_SendClientCommand(true, "playermodel %s", model.string);
			CL_SendClientCommand(true, "playerskin %s", skin.string);
		}
		break;

	case 3:
		CL_SendClientCommand(true, "begin");
		Cache_Report ();		// print remaining memory
#ifdef VM_CG
		CG_Start();
#endif
		break;

	case 4:
		SCR_EndLoadingPlaque ();		// allow normal screen updates
		break;
	}
}

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

#define DPSU_EXTEND1		(1<<15)
// first extend byte
#define DPSU_PUNCHVEC1	(1<<16)
#define DPSU_PUNCHVEC2	(1<<17)
#define DPSU_PUNCHVEC3	(1<<18)
#define DPSU_VIEWZOOM		(1<<19) // byte factor (0 = 0.0 (not valid), 255 = 1.0)
#define DPSU_UNUSED20		(1<<20)
#define DPSU_UNUSED21		(1<<21)
#define DPSU_UNUSED22		(1<<22)
#define DPSU_EXTEND2		(1<<23) // another byte to follow, future expansion
// second extend byte
#define DPSU_UNUSED24		(1<<24)
#define DPSU_UNUSED25		(1<<25)
#define DPSU_UNUSED26		(1<<26)
#define DPSU_UNUSED27		(1<<27)
#define DPSU_UNUSED28		(1<<28)
#define DPSU_UNUSED29		(1<<29)
#define DPSU_UNUSED30		(1<<30)
#define DPSU_EXTEND3		(1<<31) // another byte to follow, future expansion


#define	DEFAULT_VIEWHEIGHT	22
void CLNQ_ParseClientdata (void)
{
	int		i;

	unsigned int bits;

	bits = MSG_ReadShort();

	if (bits & DPSU_EXTEND1)
		bits |= (MSG_ReadByte() << 16);
	if (bits & DPSU_EXTEND2)
		bits |= (MSG_ReadByte() << 24);

	if (bits & SU_VIEWHEIGHT)
		CL_SetStat(0, STAT_VIEWHEIGHT, MSG_ReadChar ());
	else if (nq_dp_protocol < 6)
		CL_SetStat(0, STAT_VIEWHEIGHT, DEFAULT_VIEWHEIGHT);

	if (bits & SU_IDEALPITCH)
		/*cl.idealpitch =*/ MSG_ReadChar ();
	/*else
		cl.idealpitch = 0;*/

//	VectorCopy (cl.mvelocity[0], cl.mvelocity[1]);
	for (i=0 ; i<3 ; i++)
	{
		if (bits & (SU_PUNCH1<<i) )
			/*cl.punchangle[i] =*/ nq_dp_protocol?MSG_ReadAngle16():MSG_ReadChar();
//		else
//			cl.punchangle[i] = 0;

		if (bits & (DPSU_PUNCHVEC1<<i))
		{
			/*cl.punchvector[i] =*/ MSG_ReadCoord();
		}
//		else
//			cl.punchvector[i] = 0;

		if (bits & (SU_VELOCITY1<<i) )
		{
			if (nq_dp_protocol >= 5)
				/*cl.simvel[0][i] =*/ MSG_ReadFloat();
			else
			/*cl.mvelocity[0][i] =*/ MSG_ReadChar()/**16*/;
		}
//		else
//			cl.mvelocity[0][i] = 0;
	}

	if (bits & SU_ITEMS)
		CL_SetStat(0, STAT_ITEMS, MSG_ReadLong());

//	cl.onground = (bits & SU_ONGROUND) != 0;
//	cl.inwater = (bits & SU_INWATER) != 0;

	if (nq_dp_protocol >= 6)
	{
	}
	else if (nq_dp_protocol == 5)
	{
		CL_SetStat(0, STAT_WEAPONFRAME, (bits & SU_WEAPONFRAME)?(unsigned short)MSG_ReadShort():0);
		CL_SetStat(0, STAT_ARMOR, (bits & SU_ARMOR)?MSG_ReadShort():0);
		CL_SetStat(0, STAT_WEAPON, (bits & SU_WEAPON)?MSG_ReadShort():0);

		CL_SetStat(0, STAT_HEALTH, MSG_ReadShort());

		CL_SetStat(0, STAT_AMMO, MSG_ReadShort());

		CL_SetStat(0, STAT_SHELLS, MSG_ReadShort());
		CL_SetStat(0, STAT_NAILS, MSG_ReadShort());
		CL_SetStat(0, STAT_ROCKETS, MSG_ReadShort());
		CL_SetStat(0, STAT_CELLS, MSG_ReadShort());

		CL_SetStat(0, STAT_ACTIVEWEAPON, (unsigned short)MSG_ReadShort());
	}
	else
	{
		CL_SetStat(0, STAT_WEAPONFRAME, (bits & SU_WEAPONFRAME)?(unsigned char)MSG_ReadByte():0);
		CL_SetStat(0, STAT_ARMOR, (bits & SU_ARMOR)?MSG_ReadByte():0);
		CL_SetStat(0, STAT_WEAPON, (bits & SU_WEAPON)?MSG_ReadByte():0);

		CL_SetStat(0, STAT_HEALTH, MSG_ReadShort());

		CL_SetStat(0, STAT_AMMO, MSG_ReadByte());

		CL_SetStat(0, STAT_SHELLS, MSG_ReadByte());
		CL_SetStat(0, STAT_NAILS, MSG_ReadByte());
		CL_SetStat(0, STAT_ROCKETS, MSG_ReadByte());
		CL_SetStat(0, STAT_CELLS, MSG_ReadByte());

		CL_SetStat(0, STAT_ACTIVEWEAPON, MSG_ReadByte());
	}

	if (bits & DPSU_VIEWZOOM)
	{
		if (nq_dp_protocol >= 6)
			i = (unsigned short) MSG_ReadShort();
		else
			i = MSG_ReadByte();
		if (i < 2)
			i = 2;
		CL_SetStat(0, STAT_VIEWZOOM, i);
	}
	else if (nq_dp_protocol < 6)
		CL_SetStat(0, STAT_VIEWZOOM, 255);
}
#endif
/*
==================
CL_ParseSoundlist
==================
*/
void CL_ParseSoundlist (void)
{
	int	numsounds;
	char	*str;
	int n;

// precache sounds
//	memset (cl.sound_precache, 0, sizeof(cl.sound_precache));

	numsounds = MSG_ReadByte();

	for (;;)
	{
		str = MSG_ReadString ();
		if (!str[0])
			break;
		numsounds++;
		if (numsounds >= MAX_SOUNDS)
			Host_EndGame ("Server sent too many sound_precache");

//		if (strlen(str)>4)
//		if (!strcmp(str+strlen(str)-4, ".mp3"))	//don't let the server send us a specific mp3. convert it to wav and this way we know not to look outside the quake path for it.
//			strcpy(str+strlen(str)-4, ".wav");

		strcpy (cl.sound_name[numsounds], str);
	}

	n = MSG_ReadByte();

	if (n)
	{
		if (CL_RemoveClientCommands("soundlist"))
			Con_Printf("Multiple soundlists\n");
//		CL_SendClientCommand("soundlist %i %i", cl.servercount, n);
		CL_SendClientCommand(true, soundlist_name, cl.servercount, n);
		return;
	}

	cls.downloadnumber = 0;
	cls.downloadtype = dl_sound;
	Sound_NextDownload ();
}

/*
==================
CL_ParseModellist
==================
*/
void CL_ParseModellist (qboolean lots)
{
	int	nummodels;
	char	*str;
	int n;

// precache models and note certain default indexes
	if (lots)
		nummodels = MSG_ReadShort();
	else
		nummodels = MSG_ReadByte();

	for (;;)
	{
		str = MSG_ReadString ();
		if (!str[0])
			break;
		nummodels++;
		if (nummodels>=MAX_MODELS)
			Host_EndGame ("Server sent too many model_precache");
		strcpy (cl.model_name[nummodels], str);

		if (!strcmp(cl.model_name[nummodels],"progs/spike.mdl"))
			cl_spikeindex = nummodels;
#ifdef PEXT_LIGHTUPDATES
		if (!strcmp(cl.model_name[nummodels], "progs/zap.mdl"))
			cl_lightningindex = nummodels;
#endif
		if (!strcmp(cl.model_name[nummodels],"progs/player.mdl"))
			cl_playerindex = nummodels;
		if (!strcmp(cl.model_name[nummodels],"progs/h_player.mdl"))
			cl_h_playerindex = nummodels;
		if (!strcmp(cl.model_name[nummodels],"progs/flag.mdl"))
			cl_flagindex = nummodels;

		if (!strcmp(cl.model_name[nummodels],"progs/missile.mdl"))
			cl_rocketindex = nummodels;
		if (!strcmp(cl.model_name[nummodels],"progs/grenade.mdl"))
			cl_grenadeindex = nummodels;


		if (!strcmp(cl.model_name[nummodels],"progs/gib1.mdl"))
			cl_gib1index = nummodels;
		if (!strcmp(cl.model_name[nummodels],"progs/gib2.mdl"))
			cl_gib2index = nummodels;
		if (!strcmp(cl.model_name[nummodels],"progs/gib3.mdl"))
			cl_gib3index = nummodels;
	}

	if (nummodels)
		SCR_ImageName(cl.model_name[1]);

	n = MSG_ReadByte();

	if (n)
	{
//		CL_SendClientCommand("modellist %i %i", cl.servercount, n);
		CL_SendClientCommand(true, modellist_name, cl.servercount, (nummodels&0xff00) + n);
		return;
	}

	cls.downloadnumber = 0;
	cls.downloadtype = dl_model;
	Model_NextDownload ();
}

void CL_ProcessUserInfo (int slot, player_info_t *player);
void CLQ2_ParseClientinfo(int i, char *s)
{
	char *model, *name;
	player_info_t *player;
	//s contains "name\model/skin"

	player = &cl.players[i];

	*player->userinfo = '\0';

	model = strchr(s, '\\');
	if (model)
	{
		*model = '\0';
		model++;
		name = s;
	}
	else
	{
		name = "Unnammed";
		model = "male";
	}
#if 0
	skin = strchr(model, '/');
	if (skin)
	{
		*skin = '\0';
		skin++;
	}
	else
		skin = "";
	Info_SetValueForKey(player->userinfo, "model", model, MAX_INFO_STRING);
	Info_SetValueForKey(player->userinfo, "skin", skin, MAX_INFO_STRING);
#else
	Info_SetValueForKey(player->userinfo, "skin", model, MAX_INFO_STRING);
#endif
	Info_SetValueForKey(player->userinfo, "name", name, MAX_INFO_STRING);

	cl.players[i].userid = i;
	cl.players[i].bottomcolor = 1;
	cl.players[i].topcolor = 1;
	CL_ProcessUserInfo (i, player);
}

void CLQ2_ParseConfigString (void)
{
	int		i;
	char	*s;
//	char	olds[MAX_QPATH];

	i = MSG_ReadShort ();
	if (i < 0 || i >= Q2MAX_CONFIGSTRINGS)
		Host_EndGame ("configstring > Q2MAX_CONFIGSTRINGS");
	s = MSG_ReadString();

//	strncpy (olds, cl.configstrings[i], sizeof(olds));
//	olds[sizeof(olds) - 1] = 0;

//	strcpy (cl.configstrings[i], s);

	// do something apropriate

	if (i == Q2CS_SKY)
	{
		Q_strncpyz (cl.skyname, s, sizeof(cl.skyname));
	}
	else if (i == Q2CS_SKYAXIS)
	{
		s = COM_Parse(s);
		cl.skyaxis[0] = atof(com_token);
		s = COM_Parse(s);
		cl.skyaxis[1] = atof(com_token);
		s = COM_Parse(s);
		cl.skyaxis[2] = atof(com_token);
	}
	else if (i == Q2CS_SKYROTATE)
		cl.skyrotate = atof(s);
	else if (i == Q2CS_STATUSBAR)
	{
		Q_strncpyz(cl.q2statusbar, s, sizeof(cl.q2statusbar));
	}
	else if (i >= Q2CS_LIGHTS && i < Q2CS_LIGHTS+Q2MAX_LIGHTSTYLES)
	{
#ifdef PEXT_LIGHTSTYLECOL
		cl_lightstyle[i - Q2CS_LIGHTS].colour = 7;	//white
#endif
		Q_strncpyz (cl_lightstyle[i - Q2CS_LIGHTS].map,  s, sizeof(cl_lightstyle[i-Q2CS_LIGHTS].map));
		cl_lightstyle[i - Q2CS_LIGHTS].length = Q_strlen(cl_lightstyle[i - Q2CS_LIGHTS].map);

	}
	else if (i == Q2CS_CDTRACK)
	{
//		if (cl.refresh_prepped)
			CDAudio_Play (atoi(s), true);
	}
	else if (i >= Q2CS_MODELS && i < Q2CS_MODELS+Q2MAX_MODELS)
	{
//		if (cl.refresh_prepped)
		{
			Q_strncpyz(cl.model_name[i-Q2CS_MODELS], s, MAX_QPATH);
			cl.model_precache[i-Q2CS_MODELS] = Mod_ForName (cl.model_name[i-Q2CS_MODELS], false);
		}
	}
	else if (i >= Q2CS_SOUNDS && i < Q2CS_SOUNDS+Q2MAX_MODELS)
	{
//		if (cl.refresh_prepped)
		Q_strncpyz(cl.sound_name[i-Q2CS_SOUNDS], s, MAX_QPATH);
			cl.sound_precache[i-Q2CS_SOUNDS] = S_PrecacheSound (s);
	}
	else if (i >= Q2CS_IMAGES && i < Q2CS_IMAGES+Q2MAX_MODELS)
	{	//ignore
		Q_strncpyz(cl.image_name[i-Q2CS_IMAGES], s, MAX_QPATH);
	}
	else if (i >= Q2CS_PLAYERSKINS && i < Q2CS_PLAYERSKINS+Q2MAX_CLIENTS)
	{
//		if (cl.refresh_prepped && strcmp(olds, s))
			CLQ2_ParseClientinfo (i-Q2CS_PLAYERSKINS, s);
	}

#ifdef VM_UI
	UI_StringChanged(i);
#endif
}



/*
==================
CL_ParseBaseline
==================
*/
void CL_ParseBaseline (entity_state_t *es)
{
	int			i;

	memset(es, 0, sizeof(entity_state_t));

	es->modelindex = MSG_ReadByte ();
	es->frame = MSG_ReadByte ();
	es->colormap = MSG_ReadByte();
	es->skinnum = MSG_ReadByte();

	for (i=0 ; i<3 ; i++)
	{
		es->origin[i] = MSG_ReadCoord ();
		es->angles[i] = MSG_ReadAngle ();
	}
#ifdef PEXT_SCALE
	es->scale = 1*16;
#endif
#ifdef PEXT_TRANS
	es->trans = 255;
#endif
}
void CL_ParseBaseline2 (void)
{
	entity_state_t nullst, es;

	memset(&nullst, 0, sizeof(entity_state_t));
	memset(&es, 0, sizeof(entity_state_t));

	CL_ParseDelta(&nullst, &es, MSG_ReadShort(), true);
	memcpy(&cl_baselines[es.number], &es, sizeof(es));
}

void CLQ2_Precache_f (void)
{
#ifdef VM_CG
	CG_Start();
#endif

	cls.downloadnumber = 0;
	cls.downloadtype = dl_sound;

	CL_RequestNextDownload();
}



/*
=====================
CL_ParseStatic

Static entities are non-interactive world objects
like torches
=====================
*/
void CL_ParseStatic (int version)
{
	entity_t *ent;
	int		i;
	entity_state_t	es, nullstate;

	if (version == 1)
	{
		CL_ParseBaseline (&es);
		i = cl.num_statics;
		cl.num_statics++;
	}
	else
	{
		memset(&nullstate, 0, sizeof(nullstate));
		CL_ParseDelta(&nullstate, &es, MSG_ReadShort(), true);
		es.number+=MAX_EDICTS;

		for (i = 0; i < cl.num_statics; i++)
			if (cl_static_entities[i].keynum == es.number)
			{
				R_RemoveEfrags (&cl_static_entities[i]);
				P_DelinkTrailstate (&cl_static_emit[i]);
				break;
			}

		if (i == cl.num_statics)
			cl.num_statics++;
	}

	if (i >= MAX_STATIC_ENTITIES)
	{
		cl.num_statics--;
		Con_Printf ("Too many static entities");
		return;
	}
	ent = &cl_static_entities[i];
	memset(ent, 0, sizeof(*ent));
	cl_static_emit[i] = NULL;

	ent->keynum = es.number;

// copy it to the current state
	ent->model = cl.model_precache[es.modelindex];
	ent->oldframe = ent->frame = es.frame;
#ifdef SWQUAKE
	ent->palremap = D_IdentityRemap();
#endif
	ent->skinnum = es.skinnum;
	ent->drawflags = es.hexen2flags;

#ifdef PEXT_SCALE
	ent->scale = es.scale/16.0;
#endif
#ifdef PEXT_TRANS
	ent->alpha = es.trans/255.0;
#endif
	ent->fatness = es.fatness/2.0;
	ent->abslight = es.abslight;

	VectorCopy (es.origin, ent->origin);
	VectorCopy (es.angles, ent->angles);
	es.angles[0]*=-1;
	AngleVectors(es.angles, ent->axis[0], ent->axis[1], ent->axis[2]);
	VectorInverse(ent->axis[1]);

	if (!cl.worldmodel)
	{
		Con_TPrintf (TLC_PARSESTATICWITHNOMAP);
		return;
	}

	R_AddEfrags (ent);
}

/*
===================
CL_ParseStaticSound
===================
*/
void CL_ParseStaticSound (void)
{
	extern cvar_t cl_staticsounds;
	vec3_t		org;
	int			sound_num, vol, atten;
	int			i;

	for (i=0 ; i<3 ; i++)
		org[i] = MSG_ReadCoord ();
	sound_num = MSG_ReadByte ();
	vol = MSG_ReadByte ();
	atten = MSG_ReadByte ();

	if (!cl_staticsounds.value)
		return;

	S_StaticSound (cl.sound_precache[sound_num], org, vol, atten);
}



/*
=====================================================================

ACTION MESSAGES

=====================================================================
*/

/*
==================
CL_ParseStartSoundPacket
==================
*/
void CL_ParseStartSoundPacket(void)
{
    vec3_t  pos;
    int 	channel, ent;
    int 	sound_num;
    int 	volume;
    float 	attenuation;
 	int		i;

    channel = MSG_ReadShort();

    if (channel & SND_VOLUME)
		volume = MSG_ReadByte ();
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;

    if (channel & SND_ATTENUATION)
		attenuation = MSG_ReadByte () / 64.0;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;

	sound_num = MSG_ReadByte ();

	for (i=0 ; i<3 ; i++)
		pos[i] = MSG_ReadCoord ();

	ent = (channel>>3)&1023;
	channel &= 7;

	if (ent > MAX_EDICTS)
		Host_EndGame ("CL_ParseStartSoundPacket: ent = %i", ent);

#ifdef PEXT_CSQC
	if (!CSQC_StartSound(ent, channel, cl.sound_name[sound_num], pos, volume/255.0, attenuation))
#endif
		S_StartSound (ent, channel, cl.sound_precache[sound_num], pos, volume/255.0, attenuation);


	if (ent == cl.playernum[0]+1)
		TP_CheckPickupSound(cl.sound_name[sound_num], pos);
}

#ifdef Q2CLIENT
void CLQ2_ParseStartSoundPacket(void)
{
    vec3_t  pos_v;
	float	*pos;
    int 	channel, ent;
    int 	sound_num;
    float 	volume;
    float 	attenuation;
	int		flags;
	float	ofs;

	flags = MSG_ReadByte ();
	sound_num = MSG_ReadByte ();

    if (flags & Q2SND_VOLUME)
		volume = MSG_ReadByte () / 255.0;
	else
		volume = Q2DEFAULT_SOUND_PACKET_VOLUME;

    if (flags & Q2SND_ATTENUATION)
		attenuation = MSG_ReadByte () / 64.0;
	else
		attenuation = Q2DEFAULT_SOUND_PACKET_ATTENUATION;

    if (flags & Q2SND_OFFSET)
		ofs = MSG_ReadByte () / 1000.0;
	else
		ofs = 0;

	if (flags & Q2SND_ENT)
	{	// entity reletive
		channel = MSG_ReadShort();
		ent = channel>>3;
		if (ent > MAX_EDICTS)
			Host_EndGame ("CL_ParseStartSoundPacket: ent = %i", ent);

		channel &= 7;
	}
	else
	{
		ent = 0;
		channel = 0;
	}

	if (flags & Q2SND_POS)
	{	// positioned in space
		MSG_ReadPos (pos_v);

		pos = pos_v;
	}
	else	// use entity number
	{
		CL_GetNumberedEntityInfo(ent, pos_v, NULL);
		pos = pos_v;
//		pos = NULL;
	}

	if (!cl.sound_precache[sound_num])
		return;

	S_StartSound (ent, channel, cl.sound_precache[sound_num], pos, volume, attenuation);
}
#endif

#ifdef NQPROT
#define	NQSND_VOLUME		(1<<0)		// a qbyte
#define	NQSND_ATTENUATION	(1<<1)		// a qbyte
void CLNQ_ParseStartSoundPacket(void)
{
    vec3_t  pos;
    int 	channel, ent;
    int 	sound_num;
    int 	volume;
    int 	field_mask;
    float 	attenuation;
 	int		i;

    field_mask = MSG_ReadByte();

    if (field_mask & NQSND_VOLUME)
		volume = MSG_ReadByte ();
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;

    if (field_mask & NQSND_ATTENUATION)
		attenuation = MSG_ReadByte () / 64.0;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;

	channel = MSG_ReadShort ();
	sound_num = MSG_ReadByte ();

	ent = channel >> 3;
	channel &= 7;

	if (ent > MAX_EDICTS)
		Host_EndGame ("CL_ParseStartSoundPacket: ent = %i", ent);

	for (i=0 ; i<3 ; i++)
		pos[i] = MSG_ReadCoord ();

    S_StartSound (ent, channel, cl.sound_precache[sound_num], pos, volume/255.0, attenuation);

	if (ent == cl.playernum[0]+1)
		TP_CheckPickupSound(cl.sound_name[sound_num], pos);
}
#endif


/*
==================
CL_ParseClientdata

Server information pertaining to this client only, sent every frame
==================
*/
void CL_ParseClientdata (void)
{
	int				i;
	float		latency;
	frame_t		*frame;

// calculate simulated time of message
	oldparsecountmod = parsecountmod;

	i = cls.netchan.incoming_acknowledged;
	if (cls.demoplayback == DPB_MVD)
		cl.oldparsecount = i - 1;
	cl.parsecount = i;
	i &= UPDATE_MASK;
	parsecountmod = i;
	frame = &cl.frames[i];
	if (cls.demoplayback == DPB_MVD)
		frame->senttime = realtime - host_frametime;
	parsecounttime = cl.frames[i].senttime;

	frame->receivedtime = (cl.gametimemark - cl.oldgametimemark)*20;

// calculate latency
	latency = frame->receivedtime - frame->senttime;

	if (latency < 0 || latency > 1.0)
	{
//		Con_Printf ("Odd latency: %5.2f\n", latency);
	}
	else
	{
	// drift the average latency towards the observed latency
		if (latency < cls.latency)
			cls.latency = latency;
		else
			cls.latency += 0.001;	// drift up, so correction are needed
	}
}

/*
=====================
CL_NewTranslation
=====================
*/
void CL_NewTranslation (int slot)
{
#ifdef SWQUAKE
	int		i, j;
	int		top, bottom;
	qbyte	*dest, *source;
		int local;
#endif

	char *s;
	player_info_t	*player;

	if (slot >= MAX_CLIENTS)
		Sys_Error ("CL_NewTranslation: slot > MAX_CLIENTS");

	player = &cl.players[slot];

	s = Skin_FindName (player);
	COM_StripExtension(s, s);
	if (player->skin && !stricmp(s, player->skin->name))
		player->skin = NULL;


#ifdef RGLQUAKE
	if (qrenderer == QR_OPENGL)
	{	//gl doesn't need to do anything except prevent the sys_error below.
	}
	else
#endif
#ifdef SWQUAKE
		if (qrenderer == QR_SOFTWARE)
	{
		top = player->topcolor;
		bottom = player->bottomcolor;
		if (!cl.splitclients && !(cl.fpd & FPD_NO_FORCE_COLOR))	//no colour/skin forcing in splitscreen.
		{
			if (cl.teamplay && cl.spectator)
			{
				local = Cam_TrackNum(0);
				if (local < 0)
					local = cl.playernum[0];
			}
			else
				local = cl.playernum[0];
			if (cl.teamplay && !strcmp(player->team, cl.players[local].team))
			{
				if (cl_teamtopcolor>=0)
					top = cl_teamtopcolor;
				if (cl_teambottomcolor>=0)
					bottom = cl_teambottomcolor;
			}
			else
			{
				if (cl_enemytopcolor>=0)
					top = cl_enemytopcolor;
				if (cl_enemybottomcolor>=0)
					bottom = cl_enemybottomcolor;
			}
		}

		if (top > 13 || top < 0)
			top = 13;
		if (bottom > 13 || bottom < 0)
			bottom = 13;

		if (player->_topcolor != top ||
			player->_bottomcolor != bottom || !player->skin) {
			player->_topcolor = top;
			player->_bottomcolor = bottom;
			D_DereferenceRemap(player->palremap);
			player->palremap = D_GetPaletteRemap(255, 255, 255, false, true, top, bottom);
		}
	}
	else
#endif
		Sys_Error("Bad rendering method in CL_NewTranslation");
}

/*
==============
CL_UpdateUserinfo
==============
*/
void CL_ProcessUserInfo (int slot, player_info_t *player)
{
	Q_strncpyz (player->name, Info_ValueForKey (player->userinfo, "name"), sizeof(player->name));
	Q_strncpyz (player->team, Info_ValueForKey (player->userinfo, "team"), sizeof(player->team));
	player->topcolor = atoi(Info_ValueForKey (player->userinfo, "topcolor"));
	player->bottomcolor = atoi(Info_ValueForKey (player->userinfo, "bottomcolor"));
	if (atoi(Info_ValueForKey (player->userinfo, "*spectator")))
		player->spectator = true;
	else
		player->spectator = false;

	if (slot == cl.playernum[0] && player->name[0])
		cl.spectator = player->spectator;

	player->model = NULL;

	if (cls.state == ca_active)
		Skin_Find (player);

	Sbar_Changed ();
	CL_NewTranslation (slot);
}

/*
==============
CL_UpdateUserinfo
==============
*/
void CL_UpdateUserinfo (void)
{
	int		slot;
	player_info_t	*player;

	slot = MSG_ReadByte ();
	if (slot >= MAX_CLIENTS)
		Host_EndGame ("CL_ParseServerMessage: svc_updateuserinfo > MAX_SCOREBOARD");

	player = &cl.players[slot];
	player->userid = MSG_ReadLong ();
	Q_strncpyz (player->userinfo, MSG_ReadString(), sizeof(player->userinfo));

	CL_ProcessUserInfo (slot, player);
}

/*
==============
CL_SetInfo
==============
*/
void CL_SetInfo (void)
{
	int		slot;
	player_info_t	*player;
	char key[MAX_QWMSGLEN];
	char value[MAX_QWMSGLEN];

	slot = MSG_ReadByte ();
	if (slot >= MAX_CLIENTS)
		Host_EndGame ("CL_ParseServerMessage: svc_setinfo > MAX_SCOREBOARD");

	player = &cl.players[slot];

	Q_strncpyz (key, MSG_ReadString(), sizeof(key));
	Q_strncpyz (value, MSG_ReadString(), sizeof(value));

	Con_DPrintf("SETINFO %s: %s=%s\n", player->name, key, value);

	Info_SetValueForStarKey (player->userinfo, key, value, sizeof(player->userinfo));

	CL_ProcessUserInfo (slot, player);
}

/*
==============
CL_ServerInfo
==============
*/
void CL_ServerInfo (void)
{
//	int		slot;
//	player_info_t	*player;
	char key[MAX_QWMSGLEN];
	char value[MAX_QWMSGLEN];

	Q_strncpyz (key, MSG_ReadString(), sizeof(key));
	Q_strncpyz (value, MSG_ReadString(), sizeof(value));

	Con_DPrintf("SERVERINFO: %s=%s\n", key, value);

	Info_SetValueForKey (cl.serverinfo, key, value, MAX_SERVERINFO_STRING);

	CL_CheckServerInfo();
}

/*
=====================
CL_SetStat
=====================
*/
void CL_SetStat (int pnum, int stat, int value)
{
	int	j;
	if (stat < 0 || stat >= MAX_CL_STATS)
		return;
//		Host_EndGame ("CL_SetStat: %i is invalid", stat);

	if (cls.demoplayback == DPB_MVD)
	{
		extern int cls_lastto;
		cl.players[cls_lastto].stats[stat]=value;
		if ( spec_track[pnum] != cls_lastto )
			return;
	}

	if (cl.stats[pnum][stat] != value)
		Sbar_Changed ();

	if (stat == STAT_ITEMS)
	{	// set flash times
		for (j=0 ; j<32 ; j++)
			if ( (value & (1<<j)) && !(cl.stats[pnum][stat] & (1<<j)))
				cl.item_gettime[pnum][j] = cl.time;
	}

	if (stat == STAT_VIEWHEIGHT && cls.z_ext & Z_EXT_VIEWHEIGHT)
		cl.viewheight[pnum] = value;

	if (stat == STAT_TIME && (cls.fteprotocolextensions & PEXT_ACCURATETIMINGS))
	{
		cl.oldgametime = cl.gametime;
		cl.oldgametimemark = cl.gametimemark;

		cl.gametime = value * 0.001;
		cl.gametimemark = realtime;
	}

	if (stat == STAT_WEAPON)
	{
		if (cl.stats[pnum][stat] != value)
		{
			if (value == 0)
				TP_ExecTrigger ("f_reloadstart");
			else if (cl.stats[pnum][stat] == 0)
				TP_ExecTrigger ("f_reloadend");
		}
	}

	cl.stats[pnum][stat] = value;

	if (pnum == 0)
		TP_StatChanged(stat, value);
}

/*
==============
CL_MuzzleFlash
==============
*/
void CL_MuzzleFlash (int destsplit)
{
	vec3_t		fv, rv, uv;
	dlight_t	*dl=NULL;
	int			i;
	player_state_t	*pl;

	packet_entities_t *pack;
	entity_state_t *s1;
	int pnum;

	extern cvar_t cl_muzzleflash;

	i = MSG_ReadShort ();

	//was it us?
	if (!cl_muzzleflash.value) // remove all muzzleflashes
		return;

	if (i-1 == cl.playernum[destsplit] && cl_muzzleflash.value == 2)
		return;

	pack = &cl.frames[cls.netchan.incoming_sequence&UPDATE_MASK].packet_entities;

	for (pnum=0 ; pnum<pack->num_entities ; pnum++)	//try looking for an entity with that id first
	{
		s1 = &pack->entities[pnum];

		if (s1->number == i)
		{
			dl = CL_AllocDlight (i);
			VectorCopy (s1->origin,  dl->origin);
			break;
		}
	}
	if (pnum==pack->num_entities)
	{	//that ent number doesn't exist, go for a player with that number
		if ((unsigned)(i) <= MAX_CLIENTS && i > 0)
		{
			// don't draw our own muzzle flash in gl if flashblending
			if (i-1 == cl.playernum[destsplit] && r_flashblend.value && qrenderer == QR_OPENGL)
				return;

			pl = &cl.frames[parsecountmod].playerstate[i-1];

			dl = CL_AllocDlight (i);
			VectorCopy (pl->origin,  dl->origin);	//set it's origin

			AngleVectors (pl->viewangles, fv, rv, uv);	//shift it up a little
			VectorMA (dl->origin, 18, fv, dl->origin);
		}
		else
			return;
	}

	dl->radius = 200 + (rand()&31);
	dl->minlight = 32;
	dl->die = cl.time + 0.1334;
	dl->color[0] = 0.2;
	dl->color[1] = 0.1;
	dl->color[2] = 0.05;

	dl->channelfade[0] = 1.5;
	dl->channelfade[1] = 0.75;
	dl->channelfade[2] = 0.375;
}

#ifdef Q2CLIENT
void Q2S_StartSound(vec3_t origin, int entnum, int entchannel, sfx_t *sfx, float fvol, float attenuation, float timeofs);
void CLQ2_ParseMuzzleFlash (void)
{
	vec3_t		fv, rv, dummy;
	dlight_t	*dl;
	int			i, weapon;
	vec3_t		org, ang;
	int			silenced;
	float		volume;
	char		soundname[64];

	i = MSG_ReadShort ();
	if (i < 1 || i >= Q2MAX_EDICTS)
		Host_Error ("CL_ParseMuzzleFlash: bad entity");

	weapon = MSG_ReadByte ();
	silenced = weapon & Q2MZ_SILENCED;
	weapon &= ~Q2MZ_SILENCED;

	CL_GetNumberedEntityInfo(i, org, ang);

	dl = CL_AllocDlight (i);
	VectorCopy (org,  dl->origin);
	AngleVectors (ang, fv, rv, dummy);
	VectorMA (dl->origin, 18, fv, dl->origin);
	VectorMA (dl->origin, 16, rv, dl->origin);
	if (silenced)
		dl->radius = 100 + (rand()&31);
	else
		dl->radius = 200 + (rand()&31);
	dl->minlight = 32;
	dl->die = cl.time+0.05; //+ 0.1;
	dl->decay = 1;

	dl->channelfade[0] = 2;
	dl->channelfade[1] = 2;
	dl->channelfade[2] = 2;

	if (silenced)
		volume = 0.2;
	else
		volume = 1;


	switch (weapon)
	{
	case Q2MZ_BLASTER:
		dl->color[0] = 0.2;dl->color[1] = 0.2;dl->color[2] = 0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/blastf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_BLUEHYPERBLASTER:
		dl->color[0] = 0;dl->color[1] = 0;dl->color[2] = 0.2;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/hyprbf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_HYPERBLASTER:
		dl->color[0] = 0.2;dl->color[1] = 0.2;dl->color[2] = 0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/hyprbf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_MACHINEGUN:
		dl->color[0] = 0.2;dl->color[1] = 0.2;dl->color[2] = 0;
		_snprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound(soundname), volume, ATTN_NORM, 0);
		break;

	case Q2MZ_SHOTGUN:
		dl->color[0] = 0.2;dl->color[1] = 0.2;dl->color[2] = 0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/shotgf1b.wav"), volume, ATTN_NORM, 0);
		Q2S_StartSound (NULL, i, CHAN_AUTO,   S_PrecacheSound("weapons/shotgr1b.wav"), volume, ATTN_NORM, 0.1);
		break;
	case Q2MZ_SSHOTGUN:
		dl->color[0] = 0.2;dl->color[1] = 0.2;dl->color[2] = 0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/sshotf1b.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_CHAINGUN1:
		dl->radius = 200 + (rand()&31);
		dl->color[0] = 0.2;dl->color[1] = 0.05;dl->color[2] = 0;
		_snprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound(soundname), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_CHAINGUN2:
		dl->radius = 225 + (rand()&31);
		dl->color[0] = 0.2;dl->color[1] = 0.1;dl->color[2] = 0;
		dl->die = cl.time  + 0.1;	// long delay
		_snprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound(soundname), volume, ATTN_NORM, 0);
		_snprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound(soundname), volume, ATTN_NORM, 0.05);
		break;
	case Q2MZ_CHAINGUN3:
		dl->radius = 250 + (rand()&31);
		dl->color[0] = 0.2;dl->color[1] = 0.2;dl->color[2] = 0;
		dl->die = cl.time  + 0.1;	// long delay
		_snprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound(soundname), volume, ATTN_NORM, 0);
		_snprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound(soundname), volume, ATTN_NORM, 0.033);
		_snprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound(soundname), volume, ATTN_NORM, 0.066);
		break;

	case Q2MZ_RAILGUN:
		dl->color[0] = 0.1;dl->color[1] = 0.1;dl->color[2] = 0.2;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/railgf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_ROCKET:
		dl->color[0] = 0.2;dl->color[1] = 0.1;dl->color[2] = 0.04;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/rocklf1a.wav"), volume, ATTN_NORM, 0);
		Q2S_StartSound (NULL, i, CHAN_AUTO,   S_PrecacheSound("weapons/rocklr1b.wav"), volume, ATTN_NORM, 0.1);
		break;
	case Q2MZ_GRENADE:
		dl->color[0] = 0.2;dl->color[1] = 0.1;dl->color[2] = 0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/grenlf1a.wav"), volume, ATTN_NORM, 0);
		Q2S_StartSound (NULL, i, CHAN_AUTO,   S_PrecacheSound("weapons/grenlr1b.wav"), volume, ATTN_NORM, 0.1);
		break;
	case Q2MZ_BFG:
		dl->color[0] = 0;dl->color[1] = 0.2;dl->color[2] = 0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/bfg__f1y.wav"), volume, ATTN_NORM, 0);
		break;

	case Q2MZ_LOGIN:
		dl->color[0] = 0;dl->color[1] = 0.2; dl->color[2] = 0;
		dl->die = cl.time + 1.0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
//		CL_LogoutEffect (pl->current.origin, weapon);
		break;
	case Q2MZ_LOGOUT:
		dl->color[0] = 0.2;dl->color[1] = 0; dl->color[2] = 0;
		dl->die = cl.time + 1.0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
//		CL_LogoutEffect (pl->current.origin, weapon);
		break;
	case Q2MZ_RESPAWN:
		dl->color[0] = 0.2;dl->color[1] = 0.2; dl->color[2] = 0;
		dl->die = cl.time + 1.0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
//		CL_LogoutEffect (pl->current.origin, weapon);
		break;
	// RAFAEL
	case Q2MZ_PHALANX:
		dl->color[0] = 0.2;dl->color[1] = 0.1; dl->color[2] = 0.1;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/plasshot.wav"), volume, ATTN_NORM, 0);
		break;
	// RAFAEL
	case Q2MZ_IONRIPPER:
		dl->color[0] = 0.2;dl->color[1] = 0.1; dl->color[2] = 0.1;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/rippfire.wav"), volume, ATTN_NORM, 0);
		break;

// ======================
// PGM
	case Q2MZ_ETF_RIFLE:
		dl->color[0] = 0.18;dl->color[1] = 0.14;dl->color[2] = 0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/nail1.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_SHOTGUN2:
		dl->color[0] = 0.2;dl->color[1] = 0.2;dl->color[2] = 0;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/shotg2.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_HEATBEAM:
		dl->color[0] = 0.2;dl->color[1] = 0.2;dl->color[2] = 0;
		dl->die = cl.time + 100;
	//	Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/bfg__l1a.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_BLASTER2:
		dl->color[0] = 0;dl->color[1] = 0.2;dl->color[2] = 0;
		// FIXME - different sound for blaster2 ??
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/blastf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_TRACKER:
		// negative flashes handled the same in gl/soft until CL_AddDLights
		dl->color[0] = -0.2;dl->color[1] = -0.2;dl->color[2] = -0.2;
		Q2S_StartSound (NULL, i, CHAN_WEAPON, S_PrecacheSound("weapons/disint2.wav"), volume, ATTN_NORM, 0);
		break;
	case Q2MZ_NUKE1:
		dl->color[0] = 0.2;dl->color[1] = 0;dl->color[2] = 0;
		dl->die = cl.time + 100;
		break;
	case Q2MZ_NUKE2:
		dl->color[0] = 0.2;dl->color[1] = 0.2;dl->color[2] = 0;
		dl->die = cl.time + 100;
		break;
	case Q2MZ_NUKE4:
		dl->color[0] = 0;dl->color[1] = 0;dl->color[2] = 0.2;
		dl->die = cl.time + 100;
		break;
	case Q2MZ_NUKE8:
		dl->color[0] = 0;dl->color[1] = 0.2;dl->color[2] = 0.2;
		dl->die = cl.time + 100;
		break;
// PGM
// ======================
	}
}

void CLQ2_ParseMuzzleFlash2 (void)
{
	int			ent;
	int			flash_number;

	ent = MSG_ReadShort ();
	if (ent < 1 || ent >= Q2MAX_EDICTS)
		Host_EndGame ("CL_ParseMuzzleFlash2: bad entity");

	flash_number = MSG_ReadByte ();

	CLQ2_RunMuzzleFlash2(ent, flash_number);
}

void CLQ2_ParseInventory (void)
{
	int		i;

	// TODO: finish this properly
	for (i=0 ; i<Q2MAX_ITEMS ; i++)
//		cl.inventory[i] = MSG_ReadShort (&net_message);
		MSG_ReadShort (); // just ignore everything for now
}
#endif

//return if we want to print the message.
char *CL_ParseChat(char *text, player_info_t **player)
{
	extern cvar_t cl_chatsound, cl_nofake, cl_teamchatsound, cl_enemychatsound;
	int flags;
	int offset=0;
	qboolean	suppress_talksound;
	char *p;
	extern cvar_t cl_parsewhitetext;
	char *s;
	int check_flood;

	flags = TP_CategorizeMessage (text, &offset, player);

	s = text + offset;

	if (flags)
	{
		if (!cls.demoplayback)
			Sys_ServerActivity();	//chat always flashes the screen..

		//check f_ stuff
		if (!strncmp(s, "f_", 2))
		{
			static float versionresponsetime;
			static float modifiedresponsetime;
			static float skinsresponsetime;
			static float serverresponsetime;

			if (!strncmp(s, "f_version", 9) && versionresponsetime < Sys_DoubleTime())	//respond to it.
			{
				ValidationPrintVersion(text);
				versionresponsetime = Sys_DoubleTime() + 5;
			}
			else if (!strncmp(s, "f_server", 8) && serverresponsetime < Sys_DoubleTime())	//respond to it.
			{
				Validation_Server();
				serverresponsetime = Sys_DoubleTime() + 5;
			}
			else if (!strncmp(s, "f_modified", 10) && modifiedresponsetime < Sys_DoubleTime())	//respond to it.
			{
				Validation_FilesModified();
				modifiedresponsetime = Sys_DoubleTime() + 5;
			}
			else if (!strncmp(s, "f_skins", 7) && skinsresponsetime < Sys_DoubleTime())	//respond to it.
			{
				Validation_Skins();
				skinsresponsetime = Sys_DoubleTime() + 5;
			}
			return s;
		}

		Validation_CheckIfResponse(text);

		if (!Plug_ChatMessage(text + offset, *player ? (int)(*player - cl.players) : -1, flags))
			return NULL;

		if (flags == 2 && !TP_FilterMessage(text + offset))
			return NULL;

		if ((int)msg_filter.value & flags)
			return NULL;	//filter chat

		check_flood = Ignore_Check_Flood(s, flags, offset);
		if (check_flood == IGNORE_NO_ADD)
			return NULL;
		else if (check_flood == NO_IGNORE_ADD)
			Ignore_Flood_Add(s);
	}
	else
	{
		if (!Plug_ServerMessage(text + offset, PRINT_CHAT))
			return NULL;
	}

	suppress_talksound = false;

	if (flags == 2 || (!cl.teamplay && flags))
		suppress_talksound = TP_CheckSoundTrigger (text + offset);

	if (!cl_chatsound.value ||		// no sound at all
		(cl_chatsound.value == 2 && flags != 2))	// only play sound in mm2
		suppress_talksound = true;

	if (!suppress_talksound)
	{
		if (flags == 2 && cl.teamplay)
			S_LocalSound (cl_teamchatsound.string);
		else
			S_LocalSound (cl_enemychatsound.string);
	}

	if (cl_nofake.value == 1 || (cl_nofake.value == 2 && flags != 2)) {
		for (p = s; *p; p++)
			if (*p == 13 || (*p == 10 && p[1]))
				*p = ' ';
	}

	msgflags = flags;

	return s;
}

char printtext[4096];
void CL_ParsePrint(char *msg, int level)
{
	if (strlen(printtext) + strlen(msg) >= sizeof(printtext))
	{
		Con_Printf("%s", printtext);
		Q_strncpyz(printtext, msg, sizeof(printtext));
	}
	else
		strcat(printtext, msg);	//safe due to size on if.
	while((msg = strchr(printtext, '\n')))
	{
		*msg = '\0';
		if (level != PRINT_CHAT)
			Stats_ParsePrintLine(printtext);

		TP_SearchForMsgTriggers(printtext, level);
		msg++;

		memmove(printtext, msg, strlen(msg)+1);
	}
}

// CL_PlayerColor: returns color and mask for player_info_t
int CL_PlayerColor(player_info_t *plr, int *name_ormask)
{
	char *t;
	int c;

	*name_ormask = 0;

	if (cl.teamfortress) //override based on team
	{
		// TODO: needs some work
		switch (plr->bottomcolor)
		{	//translate q1 skin colours to console colours
		case 10:
		case 1:
			*name_ormask = CON_HIGHCHARSMASK;
		case 4:	//red
			c = 1;
			break;
		case 11:
			*name_ormask = CON_HIGHCHARSMASK;
		case 3: // green
			c = 2;
			break;
		case 5:
			*name_ormask = CON_HIGHCHARSMASK;
		case 12:
			c = 3;
			break;
		case 6:
		case 7:
			*name_ormask = CON_HIGHCHARSMASK;
		case 8:
		case 9:
			c = 6;
			break;
		case 2: // light blue
			*name_ormask = CON_HIGHCHARSMASK;
		case 13: //blue
		case 14: //blue
			c = 5;
			break;
		default:
			*name_ormask = CON_HIGHCHARSMASK;
		case 0: // white
			c = 7;
			break;
		}
	}
	else if (cl.teamplay)
	{
		// team name hacks
		if (!strcmp(plr->team, "red"))
			c = 1;
		else if (!strcmp(plr->team, "blue"))
			c = 5;
		else
		{
			char *t;

			t = plr->team;
			c = 0;

			for (t = plr->team; *t; t++)
			{
				c >>= 1;
				c ^= *t; // TODO: very weak hash, replace
			}

			if ((c / 7) & 1)
				*name_ormask = CON_HIGHCHARSMASK;

			c = 1 + (c % 7);
		}
	}
	else
	{
		// override chat color with tc infokey
		// 0-6 is standard colors (red to white)
		// 7-13 is using secondard charactermask
		// 14 and afterwards repeats
		t = Info_ValueForKey(plr->userinfo, "tc");
		if (*t)
			c = atoi(t);
		else
			c = plr->userid; // Quake2 can start from 0

		if ((c / 7) & 1)
			*name_ormask = CON_HIGHCHARSMASK;

		c = 1 + (c % 7);
	}

	return c;
}

// CL_PrintChat: takes chat strings and performs name coloring and cl_parsewhitetext parsing
// NOTE: text in rawmsg/msg is assumed destroyable and should not be used afterwards
void CL_PrintChat(player_info_t *plr, char *rawmsg, char *msg, int plrflags)
{
	char *name = NULL;
	int c;
	int name_ormask = 0;
	extern cvar_t cl_parsewhitetext;
	qboolean memessage = false;

	if (plrflags & TPM_FAKED)
	{
		name = rawmsg; // use rawmsg pointer and msg modification to generate null-terminated string
		if (msg)
			*(msg - 2) = 0; // it's assumed that msg has 2 chars before it due to strstr
	}

	if (msg[0] == '/' && msg[1] == 'm' && msg[2] == 'e' && msg[3] == ' ')
	{
		msg += 4;
		memessage = true; // special /me formatting
	}

	if (plr) // use special formatting with a real chat message
		name = plr->name; // use player's name

	if (cl_standardchat.value)
	{
		name_ormask = CON_HIGHCHARSMASK;
		c = 7;
	}
	else
	{
		if (plrflags & TPM_SPECTATOR) // is an observer
		{
			// TODO: we don't even check for this yet...
			if (plrflags & TPM_TEAM) // is on team
				c = 0; // blacken () on observers
			else
			{
				name_ormask = CON_HIGHCHARSMASK;
				c = 7;
			}
		}
		else if (plr)
			c = CL_PlayerColor(plr, &name_ormask);
		else
		{
			// defaults for fake clients
			name_ormask = CON_HIGHCHARSMASK;
			c = 7;
		}
	}

	c = '0' + c;

	if (name)
	{
		if (memessage)
		{
			con_ormask = CON_HIGHCHARSMASK;
			if (!cl_standardchat.value && (plrflags & TPM_SPECTATOR))
				Con_Printf ("^0*^7 ");
			else
				Con_Printf ("* ");
		}

		if (plrflags & TPM_TEAM) // for team chat don't highlight the name, just the brackets
		{
			// color is reset every printf so we're safe here
			con_ormask = name_ormask;
			Con_Printf("^%c(", c);
			con_ormask = CON_HIGHCHARSMASK;
			Con_Printf("%s", name);
			con_ormask = name_ormask;
			Con_Printf("^%c)", c);
		}
		else
		{
			con_ormask = name_ormask;
			Con_Printf("^%c%s", c, name);
		}

		if (!memessage)
		{
			// only print seperator with an actual player name
			con_ormask = CON_HIGHCHARSMASK;
			if (!cl_standardchat.value && (plrflags & TPM_SPECTATOR))
				Con_Printf ("^0:^7 ");
			else
				Con_Printf (": ");
		}
		else
			Con_Printf (" ");
	}

	// print message
	con_ormask = CON_HIGHCHARSMASK;
	if (cl_parsewhitetext.value && (cl_parsewhitetext.value == 1 || (plrflags & TPM_TEAM)))
	{
		char *t, *u;

		while (t = strchr(msg, '{'))
		{
			u = strchr(msg, '}');
			if (u)
			{
				*t = 0;
				*u = 0;
				Con_Printf("%s", msg);
				con_ormask = 0;
				Con_Printf("%s", t+1);
				con_ormask = CON_HIGHCHARSMASK;
				msg = u+1;
			}
			else
				break;
		}
		Con_Printf("%s", msg);
		con_ormask = 0;
	}
	else
	{
		Con_Printf ("%s", msg);
	}
	con_ormask = 0;

}

// CL_PrintStandardMessage: takes non-chat net messages and performs name coloring
// NOTE: msg is considered destroyable
char acceptedchars[] = {'.', '?', '!', '\'', ',', ':', ' ', '\0'};
void CL_PrintStandardMessage(char *msg)
{
	int i;
	player_info_t *p;
	extern cvar_t cl_standardmsg;
	char *begin = msg;

	// search for player names in message
	for (i = 0, p = cl.players; i < MAX_CLIENTS; p++, i++)
	{
		char *v;
		char *name;
		int len;
		int ormask;
		char c;

		name = p->name;
		if (!(*name))
			continue;
		len = strlen(name);
		v = strstr(msg, name);
		while (v)
		{
			// name parsing rules
			if (v != begin && *(v-1) != ' ') // must be space before name
			{
					v = strstr(v+len, name);
					continue;
			}

			{
				int i;
				char aftername = *(v + len);

				// search for accepted chars in char after name in msg
				for (i = 0; i < sizeof(acceptedchars); i++)
				{
					if (acceptedchars[i] == aftername)
						break;
				}

				if (sizeof(acceptedchars) == i)
				{
					v = strstr(v+len, name);
					continue; // no accepted char found
				}
			}

			*v = 0; // cut off message
			con_ormask = 0;
			// print msg chunk
			Con_Printf("%s", msg);
			msg = v + len; // update search point

			// get name color
			if (p->spectator || cl_standardmsg.value)
			{
				ormask = 0;
				c = '7';
			}
			else
				c = '0' + CL_PlayerColor(p, &ormask);

			// print name
			con_ormask = ormask;
			Con_Printf("^%c%s^7", c, name);
			break;
		}
	}

	// print final chunk
	con_ormask = 0;
	Con_Printf("%s", msg);
}

char stufftext[4096];
void CL_ParseStuffCmd(char *msg, int destsplit)	//this protects stuffcmds from network segregation.
{
	strncat(stufftext, msg, sizeof(stufftext)-1);
	while((msg = strchr(stufftext, '\n')))
	{
		*msg = '\0';
		Con_DPrintf("stufftext: %s\n", stufftext);
		if (!strncmp(stufftext, "fullserverinfo ", 15))
			Cmd_ExecuteString(stufftext, RESTRICT_SERVER+destsplit);	//do this NOW so that it's done before any models or anything are loaded
		else
		{
#ifdef CSQC_DAT
			if (!CSQC_StuffCmd(stufftext))
#endif
			{
				Cbuf_AddText (stufftext, RESTRICT_SERVER+destsplit);
				Cbuf_AddText ("\n", RESTRICT_SERVER+destsplit);
			}
		}
		msg++;

		memmove(stufftext, msg, strlen(msg)+1);
	}
}

void CL_ParsePrecache(void)
{
	int i = (unsigned short)MSG_ReadShort();
	char *s = MSG_ReadString();
	if (i < 32768)
	{
		if (i >= 1 && i < MAX_MODELS)
		{
			model_t *model;
			CL_CheckOrDownloadFile(s, s, true);
			model = Mod_ForName(s, i == 1);
			if (!model)
				Con_Printf("svc_precache: Mod_ForName(\"%s\") failed\n", s);
			cl.model_precache[i] = model;
			strcpy (cl.model_name[i], s);
		}
		else
			Con_Printf("svc_precache: model index %i outside range %i...%i\n", i, 1, MAX_MODELS);
	}
	else
	{
		i -= 32768;
		if (i >= 1 && i < MAX_SOUNDS)
		{
			sfx_t *sfx;
			CL_CheckOrDownloadFile(va("sounds/%s", s), NULL, true);
			sfx = S_PrecacheSound (s);
			if (!sfx)
				Con_Printf("svc_precache: S_PrecacheSound(\"%s\") failed\n", s);
			cl.sound_precache[i] = sfx;
			strcpy (cl.sound_name[i], s);
		}
		else
			Con_Printf("svc_precache: sound index %i outside range %i...%i\n", i, 1, MAX_SOUNDS);
	}
}


#define SHOWNET(x) if(cl_shownet.value==2)Con_Printf ("%3i:%s\n", msg_readcount-1, x);
#define SHOWNET2(x, y) if(cl_shownet.value==2)Con_Printf ("%3i:%3i:%s\n", msg_readcount-1, y, x);
/*
=====================
CL_ParseServerMessage
=====================
*/
int	received_framecount;
void CL_ParseServerMessage (void)
{
	int			cmd;
	char		*s;
	int			i, j;
	int			destsplit;

	received_framecount = host_framecount;
	cl.last_servermessage = realtime;
	CL_ClearProjectiles ();
	cl.fixangle = false;

//
// if recording demos, copy the message out


	//
	if (cl_shownet.value == 1)
		Con_TPrintf (TL_INT_SPACE,net_message.cursize);
	else if (cl_shownet.value == 2)
		Con_TPrintf (TLC_LINEBREAK_MINUS);


	CL_ParseClientdata ();

//
// parse the message
//
	while (1)
	{
		if (msg_badread)
		{
			Host_EndGame ("CL_ParseServerMessage: Bad server message");
			break;
		}

		cmd = MSG_ReadByte ();

		if (cmd == svc_choosesplitclient)
		{
			SHOWNET(svc_strings[cmd]);

			destsplit = MSG_ReadByte();
			cmd = MSG_ReadByte();
		}
		else
			destsplit = 0;

		if (cmd == -1)
		{
			msg_readcount++;	// so the EOM showner has the right value
			SHOWNET("END OF MESSAGE");
			break;
		}

		SHOWNET(svc_strings[cmd]);

	// other commands
		switch (cmd)
		{
		default:
			Host_EndGame ("CL_ParseServerMessage: Illegible server message");
			return;

		case svc_time:
			cl.oldgametime = cl.gametime;
			cl.gametime = MSG_ReadFloat();
			cl.gametimemark = realtime;
			break;

		case svc_nop:
//			Con_Printf ("svc_nop\n");
			break;

		case svc_disconnect:
			if (cls.state == ca_connected)
			{
				Host_EndGame ("Server disconnected\n"
					"Server version may not be compatible");
			}
			else
				Host_EndGame ("Server disconnected");
			break;

		case svc_print:
			i = MSG_ReadByte ();
			s = MSG_ReadString ();

			if (i == PRINT_CHAT)
			{
				char *msg;
				player_info_t *plr = NULL;

				if (TP_SuppressMessage(s))
					break;	//if this was unseen-sent from us, ignore it.

				if (msg = CL_ParseChat(s, &plr))
				{
					CL_ParsePrint(s, i);
					CL_PrintChat(plr, s, msg, msgflags);
				}
			}
			else
			{
				if (Plug_ServerMessage(s, i))
				{
					CL_ParsePrint(s, i);
					CL_PrintStandardMessage(s);
				}
			}
			break;

		case svc_centerprint:
			s = MSG_ReadString ();

			if (Plug_CenterPrintMessage(s, destsplit))
				SCR_CenterPrint (destsplit, s);
			break;

		case svc_stufftext:
			s = MSG_ReadString ();

			CL_ParseStuffCmd(s, destsplit);
			break;

		case svc_damage:
			V_ParseDamage (destsplit);
			break;

		case svc_serverdata:
			Cbuf_Execute ();		// make sure any stuffed commands are done
 			CL_ParseServerData ();
			vid.recalc_refdef = true;	// leave full screen intermission
			break;
#ifdef PEXT_SETVIEW
		case svc_setview:
			if (!(cls.fteprotocolextensions & PEXT_SETVIEW))
				Host_EndGame("PEXT_SETVIEW is meant to be disabled\n");
			cl.viewentity[destsplit]=MSG_ReadShort();
			break;
#endif
		case svc_setangle:
			if (cls.demoplayback == DPB_MVD)
			{
				i = MSG_ReadByte();
				if (i != spec_track[0] || !autocam[0])
				{	//this wasn't for us.
					for (i=0 ; i<3 ; i++)
						MSG_ReadAngle ();
					break;
				}
				cl.fixangle=true;
				for (i=0 ; i<3 ; i++)
					cl.simangles[destsplit][i] = cl.viewangles[destsplit][i] = MSG_ReadAngle ();
				break;
			}
			cl.fixangle=true;
			for (i=0 ; i<3 ; i++)
				cl.viewangles[destsplit][i] = MSG_ReadAngle ();
//			cl.viewangles[PITCH] = cl.viewangles[ROLL] = 0;
			break;

		case svc_lightstyle:
			i = MSG_ReadByte ();
			if (i >= MAX_LIGHTSTYLES)
				Host_EndGame ("svc_lightstyle > MAX_LIGHTSTYLES");
#ifdef PEXT_LIGHTSTYLECOL
			cl_lightstyle[i].colour = 7;	//white
#endif
			Q_strncpyz (cl_lightstyle[i].map,  MSG_ReadString(), sizeof(cl_lightstyle[i].map));
			cl_lightstyle[i].length = Q_strlen(cl_lightstyle[i].map);
			break;
#ifdef PEXT_LIGHTSTYLECOL
		case svc_lightstylecol:
			if (!(cls.fteprotocolextensions & PEXT_LIGHTSTYLECOL))
				Host_EndGame("PEXT_LIGHTSTYLECOL is meant to be disabled\n");
			i = MSG_ReadByte ();
			if (i >= MAX_LIGHTSTYLES)
				Host_EndGame ("svc_lightstyle > MAX_LIGHTSTYLES");
			cl_lightstyle[i].colour = MSG_ReadByte();
			Q_strncpyz (cl_lightstyle[i].map,  MSG_ReadString(), sizeof(cl_lightstyle[i].map));
			cl_lightstyle[i].length = Q_strlen(cl_lightstyle[i].map);
			break;
#endif

		case svc_sound:
			CL_ParseStartSoundPacket();
			break;

		case svc_stopsound:
			i = MSG_ReadShort();
			S_StopSound(i>>3, i&7);
			break;

		case svc_updatefrags:
			Sbar_Changed ();
			i = MSG_ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updatefrags > MAX_SCOREBOARD");
			cl.players[i].frags = MSG_ReadShort ();
			break;

		case svc_updateping:
			i = MSG_ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updateping > MAX_SCOREBOARD");
			cl.players[i].ping = MSG_ReadShort ();
			break;

		case svc_updatepl:
			i = MSG_ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updatepl > MAX_SCOREBOARD");
			cl.players[i].pl = MSG_ReadByte ();
			break;

		case svc_updateentertime:
		// time is sent over as seconds ago
			i = MSG_ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updateentertime > MAX_SCOREBOARD");
			cl.players[i].entertime = cl.servertime - MSG_ReadFloat ();
			break;

		case svc_spawnbaseline:
			i = MSG_ReadShort ();
			CL_ParseBaseline (&cl_baselines[i]);
			break;
		case svc_spawnbaseline2:
			CL_ParseBaseline2 ();
			break;
		case svc_spawnstatic:
			CL_ParseStatic (1);
			break;
		case svc_spawnstatic2:
			CL_ParseStatic (2);
			break;
		case svc_temp_entity:
#ifdef NQPROT
			CL_ParseTEnt (false);
#else
			CL_ParseTEnt ();
#endif
			break;
		case svc_customtempent:
			CL_ParseCustomTEnt();
			break;

		case svc_particle:
			CLNQ_ParseParticleEffect ();
			break;
		case svc_particle2:
			CL_ParseParticleEffect2 ();
			break;
		case svc_particle3:
			CL_ParseParticleEffect3 ();
			break;
		case svc_particle4:
			CL_ParseParticleEffect4 ();
			break;

		case svc_killedmonster:
			cl.stats[0][STAT_MONSTERS]++;
			break;

		case svc_foundsecret:
			cl.stats[0][STAT_SECRETS]++;
			break;

		case svc_updatestat:
			i = MSG_ReadByte ();
			j = MSG_ReadByte ();
			CL_SetStat (destsplit, i, j);
			break;
		case svc_updatestatlong:
			i = MSG_ReadByte ();
			j = MSG_ReadLong ();	//make qbyte if nq compatability?
			CL_SetStat (destsplit, i, j);
			break;

		case svc_spawnstaticsound:
			CL_ParseStaticSound ();
			break;

		case svc_cdtrack:
			cl.cdtrack = MSG_ReadByte ();
			CDAudio_Play ((qbyte)cl.cdtrack, true);
			break;

		case svc_intermission:
			if (!cl.intermission)
				TP_ExecTrigger ("f_mapend");
			cl.intermission = 1;
			cl.completed_time = cl.servertime;
			vid.recalc_refdef = true;	// go to full screen
			for (i=0 ; i<3 ; i++)
				cl.simorg[0][i] = MSG_ReadCoord ();
			for (i=0 ; i<3 ; i++)
				cl.simangles[0][i] = MSG_ReadAngle ();
			VectorCopy (vec3_origin, cl.simvel[0]);

			VectorCopy (cl.simvel[0], cl.simvel[1]);
			VectorCopy (cl.simangles[0], cl.simangles[1]);
			VectorCopy (cl.simorg[0], cl.simorg[1]);
			break;

		case svc_finale:
			cl.intermission = 2;
			cl.completed_time = cl.servertime;
			vid.recalc_refdef = true;	// go to full screen
			SCR_CenterPrint (destsplit, MSG_ReadString ());
			break;

		case svc_sellscreen:
			Cmd_ExecuteString ("help", RESTRICT_RCON);
			break;

		case svc_smallkick:
			cl.punchangle[destsplit] = -2;
			break;
		case svc_bigkick:
			cl.punchangle[destsplit] = -4;
			break;

		case svc_muzzleflash:
			CL_MuzzleFlash (destsplit);
			break;

		case svc_updateuserinfo:
			CL_UpdateUserinfo ();
			break;

		case svc_setinfo:
			CL_SetInfo ();
			break;

		case svc_serverinfo:
			CL_ServerInfo ();
			break;

		case svc_download:
			CL_ParseDownload ();
			break;

		case svc_playerinfo:
			CL_ParsePlayerinfo ();
			break;

		case svc_nails:
			CL_ParseProjectiles (cl_spikeindex, false);
			break;
		case svc_nails2:
			CL_ParseProjectiles (cl_spikeindex, true);
			break;

		case svc_chokecount:		// some preceding packets were choked
			i = MSG_ReadByte ();
			for (j=0 ; j<i ; j++)
				cl.frames[ (cls.netchan.incoming_acknowledged-1-j)&UPDATE_MASK ].receivedtime = -2;
			break;

		case svc_modellist:
			CL_ParseModellist (false);
			break;
		case svc_modellistshort:
			CL_ParseModellist (true);
			break;

		case svc_soundlist:
			CL_ParseSoundlist ();
			break;

		case svc_packetentities:
			CL_ParsePacketEntities (false);
			break;

		case svc_deltapacketentities:
			CL_ParsePacketEntities (true);
			break;

		case svc_maxspeed :
			cl.maxspeed[destsplit] = MSG_ReadFloat();
			break;

		case svc_entgravity :
			cl.entgravity[destsplit] = MSG_ReadFloat();
			break;

		case svc_setpause:
			cl.paused = MSG_ReadByte ();
			if (cl.paused)
				CDAudio_Pause ();
			else
				CDAudio_Resume ();
			break;

#ifdef PEXT_BULLETENS
		case svc_bulletentext:
			if (!(cls.fteprotocolextensions & PEXT_BULLETENS))
				Host_EndGame("PEXT_BULLETENS is meant to be disabled\n");
			Bul_ParseMessage();
			break;
#endif
#ifdef PEXT_LIGHTUPDATES
		case svc_lightnings:
			if (!(cls.fteprotocolextensions & PEXT_LIGHTUPDATES))
				Host_EndGame("PEXT_LIGHTUPDATES is meant to be disabled\n");
			CL_ParseProjectiles (cl_lightningindex);
			break;
#endif

		case svc_ftesetclientpersist:
			CL_ParseClientPersist();
			break;
#ifdef Q2BSPS
		case svc_setportalstate:
			i = MSG_ReadByte();
			j = MSG_ReadByte();
			i *= j & 127;
			j &= ~128;
			CMQ2_SetAreaPortalState(i, j!=0);
			break;
#endif

		case svc_showpic:
			SCR_ShowPic_Create();
			break;
		case svc_hidepic:
			SCR_ShowPic_Hide();
			break;
		case svc_movepic:
			SCR_ShowPic_Move();
			break;
		case svc_updatepic:
			SCR_ShowPic_Update();
			break;

		case svcqw_effect:
			CL_ParseEffect(false);
			break;
		case svcqw_effect2:
			CL_ParseEffect(true);
			break;

#ifdef PEXT_CSQC
		case svc_csqcentities:
			CSQC_ParseEntities();
			break;
#endif
		case svc_precache:
			CL_ParsePrecache();
			break;
		}
	}
}

#ifdef Q2CLIENT
void CLQ2_ParseServerMessage (void)
{
	int			cmd;
	char		*s;
	int			i;
//	int			j;

	received_framecount = host_framecount;
	cl.last_servermessage = realtime;
	CL_ClearProjectiles ();

//
// if recording demos, copy the message out
//
	if (cl_shownet.value == 1)
		Con_TPrintf (TL_INT_SPACE,net_message.cursize);
	else if (cl_shownet.value == 2)
		Con_TPrintf (TLC_LINEBREAK_MINUS);


	CL_ParseClientdata ();

//
// parse the message
//
	while (1)
	{
		if (msg_badread)
		{
			SV_UnspawnServer();
			Host_EndGame ("CLQ2_ParseServerMessage: Bad server message");
			break;
		}

		cmd = MSG_ReadByte ();

		if (cmd == -1)
		{
			msg_readcount++;	// so the EOM showner has the right value
			SHOWNET("END OF MESSAGE");
			break;
		}

		SHOWNET(va("%i", cmd));

	// other commands
		switch (cmd)
		{
		default:
			Host_EndGame ("CL_ParseServerMessage: Illegible server message");
			return;

	//known to game
		case svcq2_muzzleflash:
			CLQ2_ParseMuzzleFlash();
			break;
		case svcq2_muzzleflash2:
			CLQ2_ParseMuzzleFlash2();
			return;
		case svcq2_temp_entity:
			CLQ2_ParseTEnt();
			break;
		case svcq2_layout:
			s = MSG_ReadString ();
			Q_strncpyz (cl.q2layout, s, sizeof(cl.q2layout));
#ifdef VM_UI
			UI_Q2LayoutChanged();
#endif
			break;
		case svcq2_inventory:
			CLQ2_ParseInventory();
			break;

	// the rest are private to the client and server
		case svcq2_nop:			//6
			Host_EndGame ("CL_ParseServerMessage: svcq2_nop not implemented");
			return;
		case svcq2_disconnect:
			if (cls.state == ca_connected)
				Host_EndGame ("Server disconnected\n"
					"Server version may not be compatible");
			else
				Host_EndGame ("Server disconnected");
			return;
		case svcq2_reconnect:	//8
			Con_TPrintf (TLC_RECONNECTING);
			CL_SendClientCommand(true, "new");
			break;
		case svcq2_sound:		//9			// <see code>
			CLQ2_ParseStartSoundPacket();
			break;
		case svcq2_print:		//10			// [qbyte] id [string] null terminated string
			i = MSG_ReadByte ();
			s = MSG_ReadString ();

			if (i == PRINT_CHAT)
			{
				char *msg;
				player_info_t *plr = NULL;

				if (msg = CL_ParseChat(s, &plr))
				{
					CL_ParsePrint(s, i);
					CL_PrintChat(plr, s, msg, msgflags);
				}
			}
			else
			{
				if (Plug_ServerMessage(s, i))
				{
					CL_ParsePrint(s, i);
					CL_PrintStandardMessage(s);
				}
			}
			con_ormask = 0;
			break;
		case svcq2_stufftext:	//11			// [string] stuffed into client's console buffer, should be \n terminated
			s = MSG_ReadString ();
			Con_DPrintf ("stufftext: %s\n", s);
			if (!strncmp(s, "precache", 8))	//big major hack. Q2 uses a command that q1 has as a cvar.
			{	//call the q2 precache function.
				CLQ2_Precache_f();
			}
			else
				Cbuf_AddText (s, RESTRICT_SERVER);	//don't let the local user cheat
			break;
		case svcq2_serverdata:	//12			// [long] protocol ...
			Cbuf_Execute ();		// make sure any stuffed commands are done
			CLQ2_ParseServerData ();
			break;
		case svcq2_configstring:	//13		// [short] [string]
			CLQ2_ParseConfigString();
			break;
		case svcq2_spawnbaseline://14
			CLQ2_ParseBaseline();
			break;
		case svcq2_centerprint:	//15		// [string] to put in center of the screen
			s = MSG_ReadString();

			if (Plug_CenterPrintMessage(s, 0))
				SCR_CenterPrint (0, s);
			break;
		case svcq2_download:		//16		// [short] size [size bytes]
			CL_ParseDownload();
			break;
		case svcq2_playerinfo:	//17			// variable
			Host_EndGame ("CL_ParseServerMessage: svcq2_playerinfo not implemented");
			return;
		case svcq2_packetentities://18			// [...]
			Host_EndGame ("CL_ParseServerMessage: svcq2_packetentities not implemented");
			return;
		case svcq2_deltapacketentities://19	// [...]
			Host_EndGame ("CL_ParseServerMessage: svcq2_deltapacketentities not implemented");
			return;
		case svcq2_frame:			//20 (the bastard to implement.)
			CLQ2_ParseFrame();
			break;
		}
	}
	CL_SetSolidEntities ();
}
#endif

#ifdef NQPROT
//Proquake specific stuff
#define pqc_nop			1
#define pqc_new_team	2
#define pqc_erase_team	3
#define pqc_team_frags	4
#define	pqc_match_time	5
#define pqc_match_reset	6
#define pqc_ping_times	7
int MSG_ReadBytePQ (char **s)
{
	int ret = (*s)[0] * 16 + (*s)[1] - 272;
	*s+=2;
	return ret;
}
int MSG_ReadShortPQ (char **s)
{
	return MSG_ReadBytePQ(s) * 256 + MSG_ReadBytePQ(s);
}
void CLNQ_ParseProQuakeMessage (char *s)
{
	int cmd;
	int ping;
//	int team, shirt, frags, i, j;

	s++;
	cmd = *s++;

	switch (cmd)
	{
	default:
		Con_DPrintf("Unrecognised ProQuake Message %i\n", cmd);
		break;
/*	case pqc_new_team:
		Sbar_Changed ();
		team = MSG_ReadByte() - 16;
		if (team < 0 || team > 13)
			Host_Error ("CL_ParseProQuakeMessage: pqc_new_team invalid team");
		shirt = MSG_ReadByte() - 16;
		cl.teamgame = true;
		// cl.teamscores[team].frags = 0;	// JPG 3.20 - removed this
		cl.teamscores[team].colors = 16 * shirt + team;
		//Con_Printf("pqc_new_team %d %d\n", team, shirt);
		break;

	case pqc_erase_team:
		Sbar_Changed ();
		team = MSG_ReadByte() - 16;
		if (team < 0 || team > 13)
			Host_Error ("CL_ParseProQuakeMessage: pqc_erase_team invalid team");
		cl.teamscores[team].colors = 0;
		cl.teamscores[team].frags = 0;		// JPG 3.20 - added this
		//Con_Printf("pqc_erase_team %d\n", team);
		break;

	case pqc_team_frags:
		Sbar_Changed ();
		team = MSG_ReadByte() - 16;
		if (team < 0 || team > 13)
			Host_Error ("CL_ParseProQuakeMessage: pqc_team_frags invalid team");
		frags = MSG_ReadShortPQ();;
		if (frags & 32768)
			frags = frags - 65536;
		cl.teamscores[team].frags = frags;
		//Con_Printf("pqc_team_frags %d %d\n", team, frags);
		break;

	case pqc_match_time:
		Sbar_Changed ();
		cl.minutes = MSG_ReadBytePQ();
		cl.seconds = MSG_ReadBytePQ();
		cl.last_match_time = cl.time;
		//Con_Printf("pqc_match_time %d %d\n", cl.minutes, cl.seconds);
		break;

	case pqc_match_reset:
		Sbar_Changed ();
		for (i = 0 ; i < 14 ; i++)
		{
			cl.teamscores[i].colors = 0;
			cl.teamscores[i].frags = 0;		// JPG 3.20 - added this
		}
		//Con_Printf("pqc_match_reset\n");
		break;
*/
	case pqc_ping_times:
		while ((ping = MSG_ReadShortPQ(&s)))
		{
			if ((ping / 4096) >= MAX_CLIENTS)
				Host_Error ("CL_ParseProQuakeMessage: pqc_ping_times > MAX_CLIENTS");
			cl.players[ping / 4096].ping = ping & 4095;
		}
		break;
	}
}


void CLNQ_ParseServerMessage (void)
{
	int			cmd;
	char		*s;
	int			i, j;

//	received_framecount = host_framecount;
//	cl.last_servermessage = realtime;
	CL_ClearProjectiles ();
	cl.fixangle = false;

	cl.allowsendpacket = true;

//
// if recording demos, copy the message out
//
	if (cl_shownet.value == 1)
		Con_TPrintf (TL_INT_SPACE,net_message.cursize);
	else if (cl_shownet.value == 2)
		Con_TPrintf (TLC_LINEBREAK_MINUS);


	CL_ParseClientdata ();
//
// parse the message
//
	while (1)
	{
		if (msg_badread)
		{
			Host_EndGame ("CL_ParseServerMessage: Bad server message");
			break;
		}

		cmd = MSG_ReadByte ();

		if (cmd == -1)
		{
			msg_readcount++;	// so the EOM showner has the right value
			SHOWNET("END OF MESSAGE");
			break;
		}

		if (cmd & 128)
		{
			SHOWNET("fast update");
			CLNQ_ParseEntity(cmd&127);
			continue;
		}

		SHOWNET2(svc_nqstrings[cmd>(sizeof(svc_nqstrings)/sizeof(char*))?0:cmd], cmd);

	// other commands
		switch (cmd)
		{
		default:
			Host_EndGame ("CLNQ_ParseServerMessage: Illegible server message (%i)", cmd);
			return;

		case svc_nop:
//			Con_Printf ("svc_nop\n");
			break;

		case svc_print:
			s = MSG_ReadString ();

			if (*s == 1 || *s == 2)
			{
				char *msg;
				player_info_t *plr = NULL;

				if (msg = CL_ParseChat(s+1, &plr))
				{
					CL_ParsePrint(s+1, PRINT_CHAT);
					CL_PrintChat(plr, s+1, msg, msgflags);
				}
			}
			else
			{
				if (Plug_ServerMessage(s, PRINT_HIGH))
				{
					CL_ParsePrint(s, PRINT_HIGH);
					CL_PrintStandardMessage(s);
				}
			}
			con_ormask = 0;
			break;

		case svc_disconnect:
			CL_Disconnect();
			break;

		case svc_centerprint:
			s = MSG_ReadString ();

			if (Plug_CenterPrintMessage(s, 0))
				SCR_CenterPrint (0, s);
			break;

		case svc_stufftext:
			s = MSG_ReadString ();
			if (*s == 1)
			{
				Con_DPrintf("Proquake: %s\n", s);
				CLNQ_ParseProQuakeMessage(s);
			}
			else
			{
				Con_DPrintf ("stufftext: %s\n", s);
				Cbuf_AddText (s, RESTRICT_SERVER);	//no cheating here...
			}
			break;

		case svc_serverdata:
			Cbuf_Execute ();		// make sure any stuffed commands are done
			CLNQ_ParseServerData ();
			vid.recalc_refdef = true;	// leave full screen intermission
			break;

		case svcdp_precache:
			CL_ParsePrecache();
			break;

		case svc_cdtrack:
			cl.cdtrack = MSG_ReadByte ();
			MSG_ReadByte ();

			CDAudio_Play ((qbyte)cl.cdtrack, true);
			break;

		case svc_setview:
			if (!cl.viewentity[0])
				cl.playernum[0] = (cl.viewentity[0] = MSG_ReadShort())-1;
			else
				cl.viewentity[0]=MSG_ReadShort();
			break;

		case svc_signonnum:
			i = MSG_ReadByte ();

			if (i <= cls.signon)
				Host_EndGame ("Received signon %i when at %i", i, cls.signon);
			cls.signon = i;
			CLNQ_SignonReply ();
			break;
		case svc_setpause:
			cl.paused = MSG_ReadByte ();
			if (cl.paused)
				CDAudio_Pause ();
			else
				CDAudio_Resume ();
			break;

		case svc_spawnstaticsound:
			CL_ParseStaticSound ();
			break;

		case svc_spawnstatic:
			CL_ParseStatic (1);
			break;

		case svc_spawnbaseline:
			i = MSG_ReadShort ();
			CL_ParseBaseline (&cl_baselines[i]);
			break;

		case svc_time:

			cls.netchan.outgoing_sequence++;
			cls.netchan.incoming_sequence = cls.netchan.outgoing_sequence-1;
			cl.validsequence = cls.netchan.incoming_sequence-1;

			received_framecount = host_framecount;
			cl.last_servermessage = realtime;

			cl.oldgametime = cl.gametime;
			cl.oldgametimemark = cl.gametimemark;
			cl.gametime = MSG_ReadFloat();
			cl.gametimemark = realtime;

			if (nq_dp_protocol<5)
			{
//				cl.frames[(cls.netchan.incoming_sequence-1)&UPDATE_MASK].packet_entities = cl.frames[cls.netchan.incoming_sequence&UPDATE_MASK].packet_entities;
				cl.frames[cls.netchan.incoming_sequence&UPDATE_MASK].packet_entities.num_entities=0;
				cl.frames[cls.netchan.incoming_sequence&UPDATE_MASK].packet_entities.servertime = cl.gametime;
			}
			break;

		case svc_updatename:
			Sbar_Changed ();
			i = MSG_ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updatename > MAX_CLIENTS");
			strcpy(cl.players[i].name, MSG_ReadString());
			break;

		case svc_updatefrags:
			Sbar_Changed ();
			i = MSG_ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updatefrags > MAX_CLIENTS");
			cl.players[i].frags = MSG_ReadShort();
			break;
		case svc_updatecolors:
			{
			int a;
			Sbar_Changed ();
			i = MSG_ReadByte ();
			if (i >= MAX_CLIENTS)
				Host_EndGame ("CL_ParseServerMessage: svc_updatecolors > MAX_CLIENTS");
			a = MSG_ReadByte ();
			//FIXME:!!!!

			cl.players[i].topcolor = a&0x0f;
			cl.players[i].bottomcolor = (a&0xf0)>>4;

			if (cls.state == ca_active)
				Skin_Find (&cl.players[i]);

			Sbar_Changed ();
			CL_NewTranslation (i);
			}
			break;
		case svc_lightstyle:
			i = MSG_ReadByte ();
			if (i >= MAX_LIGHTSTYLES)
			{
				Con_Printf("svc_lightstyle: %i >= MAX_LIGHTSTYLES\n", i);
				MSG_ReadString();
				break;
			}
#ifdef PEXT_LIGHTSTYLECOL
			cl_lightstyle[i].colour = 7;	//white
#endif
			Q_strncpyz (cl_lightstyle[i].map,  MSG_ReadString(), sizeof(cl_lightstyle[i].map));
			cl_lightstyle[i].length = Q_strlen(cl_lightstyle[i].map);
			break;

		case svc_updatestat:
			i = MSG_ReadByte ();
			j = MSG_ReadLong ();
			CL_SetStat (0, i, j);
			break;
		case svcdp_updatestatbyte:
			i = MSG_ReadByte ();
			j = MSG_ReadByte ();
			CL_SetStat (0, i, j);
			break;
		case svc_setangle:
			for (i=0 ; i<3 ; i++)
				cl.viewangles[0][i] = MSG_ReadAngle ();
//			cl.viewangles[PITCH] = cl.viewangles[ROLL] = 0;
			break;

		case svc_clientdata:
			CLNQ_ParseClientdata ();
			break;

		case svc_sound:
			CLNQ_ParseStartSoundPacket();
			break;

		case svc_temp_entity:
			CL_ParseTEnt (true);
			break;

		case svc_particle:
			CLNQ_ParseParticleEffect ();
			break;

		case svc_killedmonster:
			cl.stats[0][STAT_MONSTERS]++;
			break;

		case svc_foundsecret:
			cl.stats[0][STAT_SECRETS]++;
			break;

		case svc_intermission:
			if (!cl.intermission)
				TP_ExecTrigger ("f_mapend");
			cl.intermission = 1;
			cl.completed_time = cl.servertime;
			vid.recalc_refdef = true;	// go to full screen
			break;

		case svc_finale:
			cl.intermission = 2;
			cl.completed_time = cl.servertime;
			vid.recalc_refdef = true;	// go to full screen
			SCR_CenterPrint (0, MSG_ReadString ());
			break;

		case svc_cutscene:
			cl.intermission = 3;
			cl.completed_time = cl.servertime;
			vid.recalc_refdef = true;	// go to full screen
			SCR_CenterPrint (0, MSG_ReadString ());
			break;

		case svc_sellscreen:	//pantsie
			Cmd_ExecuteString ("help 0", RESTRICT_RCON);
			break;

		case svc_damage:
			V_ParseDamage (0);
			break;

		case svcnq_effect:
			CL_ParseEffect(false);
			break;
		case svcnq_effect2:
			CL_ParseEffect(true);
			break;

		case 57://svc_entities
			if (cls.signon == 4 - 1)
			{	// first update is the final signon stage
				cls.signon = 4;
				CLNQ_SignonReply ();
			}
			//well, it's really any protocol, but we're only going to support version 5.
			CLNQ_ParseDarkPlaces5Entities();
			break;
		}
	}
}
#endif

