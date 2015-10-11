//mp3 menu and track selector.
//was origonally an mp3 track selector, now handles lots of media specific stuff - like q3 films!
//should rename to m_media.c
#include "quakedef.h"

#ifdef GLQUAKE
#include "glquake.h"//fixme
#endif
#include "shader.h"

#if !defined(NOMEDIA)
#if defined(_WIN32) && !defined(WINRT) && !defined(NOMEDIAMENU)
//#define WINAMP
#endif
#if defined(_WIN32) && !defined(WINRT)
#define WINAVI
#endif


typedef struct mediatrack_s{
	char filename[MAX_QPATH];
	char nicename[MAX_QPATH];
	int length;
	struct mediatrack_s *next;
} mediatrack_t;

qboolean media_fadeout;
float media_fadeouttime;

//info about the current stuff that is playing.
static char media_currenttrack[MAX_QPATH];
static char media_friendlyname[MAX_QPATH];
#endif

//higher bits have priority (if they have something to play).
#define MEDIA_GAMEMUSIC (1u<<0)	//cd music. also music command etc.
#define MEDIA_CVARLIST	(1u<<1)	//cvar abuse. handy for preserving times when switching tracks.
#define MEDIA_PLAYLIST	(1u<<2)	//
static unsigned int media_playlisttypes;
static unsigned int media_playlistcurrent;

static int cdplayingtrack;	//currently playing cd track (becomes 0 when paused)
static int cdpausedtrack;	//currently paused cd track

//info about (fake)cd tracks that we want to play
int cdplaytracknum;
static char cdplaytrack[MAX_QPATH];
static char cdloopingtrack[MAX_QPATH];
//info about (fake)cd tracks that we could play if asked.
#define REMAPPED_TRACKS 256
static struct
{
	char fname[MAX_QPATH];
} cdremap[REMAPPED_TRACKS];
static qboolean cdenabled;
static int cdnumtracks;		//maximum cd track we can play.


//cvar abuse
static int music_playlist_last;
static cvar_t music_playlist_index = CVAR("music_playlist_index", "-1");
//	created dynamically: CVAR("music_playlist_list0+", ""),
//	created dynamically: CVAR("music_playlist_sampleposition0+", "-1"),


static qboolean Media_Changed (unsigned int mediatype)
{
	//something changed, but it has a lower priority so we don't care
	if (mediatype < media_playlistcurrent)
		return false;

	//make sure we're not playing any cd music.
	if (cdplayingtrack || cdpausedtrack)
	{
		cdplayingtrack = 0;
		cdpausedtrack = 0;
		CDAudio_Stop();
	}

#if !defined(NOMEDIA)
	media_fadeout = true;
	media_fadeouttime = realtime;
#endif
	return true;
}

//fake cd tracks.
qboolean Media_NamedTrack(const char *track, const char *looptrack)
{
	unsigned int tracknum;
#if !defined(NOMEDIA)
	static char *path[] =
	{
		"music/",
		"sound/cdtracks/",
		"",
		NULL
	};
	static char *ext[] =
	{
		"",
		".ogg",
#ifdef WINAVI
		".mp3",
#endif
		".wav",
		NULL
	};
	char trackname[MAX_QPATH];
	int ie, ip;
#endif
	char *trackend;
	qboolean found = false;

	if (!track || !*track)			//ignore calls if the primary track is invalid. whatever is already playing will continue to play.
		return false;
	if (!looptrack || !*looptrack)	//null or empty looptrack loops using the primary track, for compat with q3.
		looptrack = track;

	//check if its a proper number (0123456789 without any other weird stuff. if so, we can use fake track paths or actual cd tracks)
	tracknum = strtoul(track, &trackend, 0);
	if (*trackend)
		tracknum = 0;
	if (tracknum > 0 && tracknum < REMAPPED_TRACKS && *cdremap[tracknum].fname)
	{	//remap the track if its remapped.
		track = cdremap[tracknum].fname;
		tracknum = strtoul(track, &trackend, 0);
		if (*trackend)
			tracknum = 0;
	}

	if (!strcmp(looptrack, "-"))	//- for the looptrack argument can be used to prevent looping.
		looptrack = "";
#if defined(NOMEDIA)
	found = false;
#else
	for(ip = 0; path[ip] && !found; ip++)
	{
		if (tracknum)
		{
			if (tracknum <= 999)
			{
				for(ie = 0; ext[ie] && !found; ie++)
				{
					Q_snprintfz(trackname, sizeof(trackname), "%strack%03i%s", path[ip], tracknum, ext[ie]);
					found = COM_FCheckExists(trackname);
				}
			}
			if (tracknum <= 99)
			{
				for(ie = 0; ext[ie] && !found; ie++)
				{
					Q_snprintfz(trackname, sizeof(trackname), "%strack%02i%s", path[ip], tracknum, ext[ie]);
					found = COM_FCheckExists(trackname);
				}
			}
		}

		if (!found)
		{
			for(ie = 0; ext[ie] && !found; ie++)
			{
				Q_snprintfz(trackname, sizeof(trackname), "%s%s%s", path[ip], track, ext[ie]);
				found = COM_FCheckExists(trackname);
			}
		}
	}

	if (found)
	{
		cdplaytracknum = 0;
		Q_strncpyz(cdplaytrack, trackname, sizeof(cdplaytrack));
		Q_strncpyz(cdloopingtrack, looptrack, sizeof(cdloopingtrack));
		Media_Changed(MEDIA_GAMEMUSIC);
		return true;
	}
#endif
	if (tracknum && cdenabled)
	{
		Q_strncpyz(cdloopingtrack, looptrack, sizeof(cdloopingtrack));

		//couldn't do a faketrack, resort to actual cd tracks, if we're allowed
		if (!CDAudio_Startup())
			return false;
		if (cdnumtracks <= 0)
			cdnumtracks = CDAudio_GetAudioDiskInfo();

		if (tracknum > cdnumtracks)
		{
			Con_DPrintf("CDAudio: Bad track number %u.\n", tracknum);
			return false;	//can't play that, sorry. its not an available track
		}

		if (cdplayingtrack == tracknum)
			return true;	//already playing, don't need to do anything

		cdplaytracknum = tracknum;
		Q_strncpyz(cdplaytrack, "", sizeof(cdplaytrack));
		Q_strncpyz(cdloopingtrack, "", sizeof(cdloopingtrack));
		Media_Changed(MEDIA_GAMEMUSIC);
		return true;
	}
	return false;
}

//for q3
void Media_NamedTrack_f(void)
{
	if (Cmd_Argc() == 3)
		Media_NamedTrack(Cmd_Argv(1), Cmd_Argv(2));
	else
		Media_NamedTrack(Cmd_Argv(1), Cmd_Argv(1));
}

void Media_NumberedTrack(unsigned int initialtrack, unsigned int looptrack)
{
	char *init = initialtrack?va("%u", initialtrack):NULL;
	char *loop = looptrack?va("%u", looptrack):NULL;

	Media_NamedTrack(init, loop);
}

void Media_EndedTrack(void)
{
	cdplayingtrack = 0;
	cdpausedtrack = 0;

	if (*cdloopingtrack)
		Media_NamedTrack(cdloopingtrack, cdloopingtrack);
}



#if !defined(NOMEDIA)


#include "winquake.h"
#if defined(_WIN32) && !defined(WINRT)
//#define WINAMP
#endif
#if defined(_WIN32) && !defined(WINRT)
#define WINAVI
#endif

#ifdef WINAMP

#include "winamp.h"
HWND hwnd_winamp;

#endif

qboolean Media_EvaluateNextTrack(void);

int lasttrackplayed;

cvar_t media_shuffle = SCVAR("media_shuffle", "1");
cvar_t media_repeat = SCVAR("media_repeat", "1");
#ifdef WINAMP
cvar_t media_hijackwinamp = SCVAR("media_hijackwinamp", "0");
#endif

int selectedoption=-1;
int numtracks;
int nexttrack=-1;
mediatrack_t *tracks;

char media_iofilename[MAX_OSPATH]="";

int loadedtracknames;

#ifdef WINAMP
qboolean WinAmp_GetHandle (void)
{
	if ((hwnd_winamp = FindWindowW(L"Winamp", NULL)))
		return true;
	if ((hwnd_winamp = FindWindowW(L"Winamp v1.x", NULL)))
		return true;

	*currenttrack.nicename = '\0';

	return false;
}

qboolean WinAmp_StartTune(char *name)
{
	int trys;
	int pos;
	COPYDATASTRUCT cds;
	if (!WinAmp_GetHandle())
		return false;

	//FIXME: extract from fs if it's in a pack.
	//FIXME: always give absolute path
	cds.dwData = IPC_PLAYFILE;
	cds.lpData = (void *) name;
	cds.cbData = strlen((char *) cds.lpData)+1; // include space for null char
	SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_DELETE);
	SendMessage(hwnd_winamp,WM_COPYDATA,(WPARAM)NULL,(LPARAM)&cds);
	SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)0,IPC_STARTPLAY );

	for (trys = 1000; trys; trys--)
	{
		pos = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETOUTPUTTIME);
		if (pos>100 && SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETOUTPUTTIME)>=0)	//tune has started
			break;

		Sleep(10);	//give it a chance.
		if (!WinAmp_GetHandle())
			break;
	}

	return true;
}

void WinAmp_Think(void)
{
	int pos;
	int len;

	if (!WinAmp_GetHandle())
		return;

	pos = bgmvolume.value*255;
	if (pos > 255) pos = 255;
	if (pos < 0) pos = 0;
	PostMessage(hwnd_winamp, WM_WA_IPC,pos,IPC_SETVOLUME);

//optimise this to reduce calls?
	pos = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETOUTPUTTIME);
	len = SendMessage(hwnd_winamp,WM_WA_IPC,1,IPC_GETOUTPUTTIME)*1000;

	if ((pos > len || pos <= 100) && len != -1)
	if (Media_EvaluateNextTrack())
		WinAmp_StartTune(currenttrack.filename);
}
#endif
void Media_Seek (float time)
{
#ifdef WINAMP
	if (media_hijackwinamp.value)
	{
		int pos;
		if (WinAmp_GetHandle())
		{
			pos = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETOUTPUTTIME);
			pos += time*1000;
			PostMessage(hwnd_winamp,WM_WA_IPC,pos,IPC_JUMPTOTIME);

			WinAmp_Think();
		}
	}
#endif
	S_Music_Seek(time);
}

void Media_FForward_f(void)
{
	float time = atoi(Cmd_Argv(1));
	if (!time)
		time = 15;
	Media_Seek(time);
}
void Media_Rewind_f (void)
{
	float time = atoi(Cmd_Argv(1));
	if (!time)
		time = 15;
	Media_Seek(-time);
}

qboolean Media_EvaluateNextTrack(void)
{
	mediatrack_t *track;
	int trnum;
	if (!tracks)
		return false;
	if (nexttrack>=0)
	{
		trnum = nexttrack;
		for (track = tracks; track; track=track->next)
		{
			if (!trnum)
			{
				Q_strncpyz(media_currenttrack, track->filename, sizeof(media_currenttrack));
				Q_strncpyz(media_friendlyname, track->nicename, sizeof(media_friendlyname));
				lasttrackplayed = nexttrack;
				break;
			}
			trnum--;
		}
		nexttrack = -1;
	}
	else
	{
		if (media_shuffle.value)
			nexttrack=((float)(rand()&0x7fff)/0x7fff)*numtracks;
		else
		{
			nexttrack = lasttrackplayed+1;
			if (nexttrack >= numtracks)
			{
				if (media_repeat.value)
					nexttrack = 0;
				else
				{
					*media_currenttrack='\0';
					*media_friendlyname='\0';
					return false;
				}
			}
		}
		trnum = nexttrack;
		for (track = tracks; track; track=track->next)
		{
			if (!trnum)
			{
				Q_strncpyz(media_currenttrack, track->filename, sizeof(media_currenttrack));
				Q_strncpyz(media_friendlyname, track->nicename, sizeof(media_friendlyname));
				lasttrackplayed = nexttrack;
				break;
			}
			trnum--;
		}
		nexttrack = -1;
	}

	return true;
}

void Media_SetPauseTrack(qboolean paused)
{
	if (paused)
	{
		if (!cdplayingtrack)
			return;
		cdpausedtrack = cdplayingtrack;
		cdplayingtrack = 0;
		CDAudio_Pause();
	}
	else
	{
		if (!cdpausedtrack)
			return;
		cdplayingtrack = cdpausedtrack;
		cdpausedtrack = 0;
		CDAudio_Resume();
	}
}

void CD_f (void)
{
	char	*command;
	int		ret;
	int		n;

	if (Cmd_Argc() < 2)
		return;

	command = Cmd_Argv (1);

	if (Q_strcasecmp(command, "play") == 0)
	{
		Media_NamedTrack(Cmd_Argv(2), "-");
		return;
	}

	if (Q_strcasecmp(command, "loop") == 0)
	{
		if (Cmd_Argc() < 4)
			Media_NamedTrack(Cmd_Argv(2), NULL);
		else
			Media_NamedTrack(Cmd_Argv(2), Cmd_Argv(3));
		return;
	}

	if (Q_strcasecmp(command, "remap") == 0)
	{
		ret = Cmd_Argc() - 2;
		if (ret <= 0)
		{
			for (n = 1; n < REMAPPED_TRACKS; n++)
				if (*cdremap[n].fname)
					Con_Printf("  %u -> %s\n", n, cdremap[n].fname);
			return;
		}
		for (n = 1; n <= ret; n++)
			Q_strncpyz(cdremap[n].fname, Cmd_Argv (n+1), sizeof(cdremap[n].fname));
		return;
	}

	if (Q_strcasecmp(command, "stop") == 0)
	{
		*cdplaytrack = *cdloopingtrack = 0;
		cdplaytracknum = 0;
		Media_Changed(MEDIA_GAMEMUSIC);
		return;
	}

	if (!bgmvolume.value)
	{
		Con_Printf("Background music is disabled: %s is 0\n", bgmvolume.name);
		return;
	}

	if (!CDAudio_Startup())
	{
		Con_Printf("No cd drive detected\n");
		return;
	}

	if (Q_strcasecmp(command, "on") == 0)
	{
		cdenabled = true;
		return;
	}

	if (Q_strcasecmp(command, "off") == 0)
	{
		if (cdplayingtrack || cdpausedtrack)
			CDAudio_Stop();
		cdenabled = false;

		*cdplaytrack = *cdloopingtrack = 0;
		cdplaytracknum = 0;
		Media_Changed(MEDIA_GAMEMUSIC);
		return;
	}

	if (Q_strcasecmp(command, "reset") == 0)
	{
		cdenabled = true;
		if (cdplayingtrack || cdpausedtrack)
			CDAudio_Stop();
		for (n = 0; n < REMAPPED_TRACKS; n++)
			strcpy(cdremap[n].fname, "");
		cdnumtracks = CDAudio_GetAudioDiskInfo();
		return;
	}

	if (Q_strcasecmp(command, "close") == 0)
	{
		CDAudio_CloseDoor();
		return;
	}

	if (cdnumtracks <= 0)
	{
		cdnumtracks = CDAudio_GetAudioDiskInfo();
		if (cdnumtracks < 0)
		{
			Con_Printf("No CD in player.\n");
			return;
		}
	}

	if (Q_strcasecmp(command, "pause") == 0)
	{
		Media_SetPauseTrack(true);
		return;
	}

	if (Q_strcasecmp(command, "resume") == 0)
	{
		Media_SetPauseTrack(false);
		return;
	}

	if (Q_strcasecmp(command, "eject") == 0)
	{
		if (cdplayingtrack || cdpausedtrack)
			CDAudio_Stop();
		CDAudio_Eject();
		cdnumtracks = -1;
		return;
	}

	if (Q_strcasecmp(command, "info") == 0)
	{
		Con_Printf("%u tracks\n", cdnumtracks);
		if (cdplayingtrack > 0)
			Con_Printf("Currently %s track %u\n", *cdloopingtrack ? "looping" : "playing", cdplayingtrack);
		else if (cdpausedtrack > 0)
			Con_Printf("Paused %s track %u\n", *cdloopingtrack ? "looping" : "playing", cdpausedtrack);
		return;
	}
}

//actually, this func just flushes and states that it should be playing. the ambientsound func actually changes the track.
void Media_Next_f (void)
{
	Media_Changed(MEDIA_PLAYLIST);

#ifdef WINAMP
	if (media_hijackwinamp.value)
	{
		if (WinAmp_GetHandle())
		if (Media_EvaluateNextTrack())
			WinAmp_StartTune(currenttrack.filename);
	}
#endif
}





void Media_AddTrack(const char *fname)
{
	mediatrack_t *newtrack;
	if (!*fname)
		return;
	for (newtrack = tracks; newtrack; newtrack = newtrack->next)
	{
		if (!strcmp(newtrack->filename, fname))
			return;	//already added. ho hum
	}
	newtrack = Z_Malloc(sizeof(mediatrack_t));
	Q_strncpyz(newtrack->filename, fname, sizeof(newtrack->filename));
	Q_strncpyz(newtrack->nicename, COM_SkipPath(fname), sizeof(newtrack->nicename));
	newtrack->length = 0;
	newtrack->next = tracks;
	tracks = newtrack;
	numtracks++;

	if (numtracks == 1)
		Media_Changed(MEDIA_PLAYLIST);
}
void Media_RemoveTrack(const char *fname)
{
	mediatrack_t **link, *newtrack;
	if (!*fname)
		return;
	for (link = &tracks; *link; link = &(*link)->next)
	{
		newtrack = *link;
		if (!strcmp(newtrack->filename, fname))
		{
			*link = newtrack->next;
			numtracks--;

			if (!strcmp(media_currenttrack, newtrack->filename))
				Media_Changed(MEDIA_PLAYLIST);
			Z_Free(newtrack);
			return;
		}
	}
}
void M_Media_Add_f (void)
{
	char *fname = Cmd_Argv(1);

	if (Cmd_Argc() == 1)
		Con_Printf("%s <track>\n", Cmd_Argv(0));
	else
		Media_AddTrack(fname);
}
void M_Media_Remove_f (void)
{
	char *fname = Cmd_Argv(1);

	if (Cmd_Argc() == 1)
		Con_Printf("%s <track>\n", Cmd_Argv(0));
	else
		Media_RemoveTrack(fname);
}


#if !defined(NOMEDIAMENU) && !defined(NOBUILTINMENUS)

void Media_LoadTrackNames (char *listname);

#define MEDIA_MIN	-7
#define MEDIA_VOLUME -7
#define MEDIA_FASTFORWARD -6
#define MEDIA_CLEARLIST -5
#define MEDIA_ADDTRACK -4
#define MEDIA_ADDLIST -3
#define MEDIA_SHUFFLE -2
#define MEDIA_REPEAT -1

void M_Media_Draw (menu_t *menu)
{
	mediatrack_t *track;
	int y;
	int op, i;

#define MP_Hightlight(x,y,text,hl) (hl?M_PrintWhite(x, y, text):M_Print(x, y, text))

	if (!bgmvolume.value)
		M_Print (12, 32, "Not playing - no volume");
	else if (!*media_currenttrack)
	{
		if (!tracks)
			M_Print (12, 32, "Not playing - no track to play");
		else
		{
#ifdef WINAMP
			if (!WinAmp_GetHandle())
				M_Print (12, 32, "Please start WinAmp 2");
			else
#endif
				M_Print (12, 32, "Not playing - switched off");
		}
	}
	else
	{
		M_Print (12, 32, "Currently playing:");
		M_Print (12, 40, *media_friendlyname?media_friendlyname:media_currenttrack);
	}

	y=52;
	op = selectedoption - (vid.height-y)/16;
	if (op + (vid.height-y)/8>numtracks)
		op = numtracks - (vid.height-y)/8;
	if (op < MEDIA_MIN)
		op = MEDIA_MIN;
	while(op < 0)
	{
		switch(op)
		{
		case MEDIA_VOLUME:
			MP_Hightlight (12, y, va("<< Volume %2i%%    >>", (int)(100*bgmvolume.value)), op == selectedoption);
			y+=8;
			break;
		case MEDIA_CLEARLIST:
			MP_Hightlight (12, y, "Clear all", op == selectedoption);
			y+=8;
			break;
		case MEDIA_FASTFORWARD:
			{
				float time, duration;
				if (S_GetMusicInfo(0, &time, &duration))
				{
					int itime = time;
					int iduration = duration;
					if (iduration)
					{
						int pct = (int)((100*time)/duration);
						MP_Hightlight (12, y, va("<< %i:%02i / %i:%02i - %i%% >>", itime/60, itime%60, iduration/60, iduration%60, pct), op == selectedoption);
					}
					else
						MP_Hightlight (12, y, va("<< %i:%02i >>", itime/60, itime%60), op == selectedoption);
				}
				else
					MP_Hightlight (12, y, "<<    skip		 >>", op == selectedoption);
			}
			y+=8;
			break;
		case MEDIA_ADDTRACK:
			MP_Hightlight (12, y, "Add Track", op == selectedoption);
			if (op == selectedoption)
				M_PrintWhite (12+9*8, y, media_iofilename);
			y+=8;
			break;
		case MEDIA_ADDLIST:
			MP_Hightlight (12, y, "Add List", op == selectedoption);
			if (op == selectedoption)
				M_PrintWhite (12+9*8, y, media_iofilename);
			y+=8;
			break;
		case MEDIA_SHUFFLE:
			if (media_shuffle.value)
				MP_Hightlight (12, y, "Shuffle on", op == selectedoption);
			else
				MP_Hightlight (12, y, "Shuffle off", op == selectedoption);
			y+=8;
			break;
		case MEDIA_REPEAT:
			if (media_shuffle.value)
			{
				if (media_repeat.value)
					MP_Hightlight (12, y, "Repeat on", op == selectedoption);
				else
					MP_Hightlight (12, y, "Repeat off", op == selectedoption);
			}
			else
			{
				if (media_repeat.value)
					MP_Hightlight (12, y, "(Repeat on)", op == selectedoption);
				else
					MP_Hightlight (12, y, "(Repeat off)", op == selectedoption);
			}
			y+=8;
			break;
		}
		op++;
	}

	for (track = tracks, i=0; track && i<op; track=track->next, i++);
	for (; track; track=track->next, y+=8, op++)
	{
		if (op == selectedoption)
			M_PrintWhite (12, y, track->nicename);
		else
			M_Print (12, y, track->nicename);
	}
}

char compleatenamepath[MAX_OSPATH];
char compleatenamename[MAX_OSPATH];
qboolean compleatenamemultiple;
int QDECL Com_CompleatenameCallback(const char *name, qofs_t size, time_t mtime, void *data, searchpathfuncs_t *spath)
{
	if (*compleatenamename)
		compleatenamemultiple = true;
	Q_strncpyz(compleatenamename, name, sizeof(compleatenamename));

	return true;
}
void Com_CompleateOSFileName(char *name)
{
	char *ending;
	compleatenamemultiple = false;

	strcpy(compleatenamepath, name);
	ending = COM_SkipPath(compleatenamepath);
	if (compleatenamepath!=ending)
		ending[-1] = '\0';	//strip a slash
	*compleatenamename='\0';

	Sys_EnumerateFiles(NULL, va("%s*", name), Com_CompleatenameCallback, NULL, NULL);
	Sys_EnumerateFiles(NULL, va("%s*.*", name), Com_CompleatenameCallback, NULL, NULL);

	if (*compleatenamename)
		strcpy(name, compleatenamename);
}

qboolean M_Media_Key (int key, menu_t *menu)
{
	int dir;
	if (key == K_ESCAPE)
	{
		return false;
	}
	else if (key == K_RIGHTARROW || key == K_LEFTARROW)
	{
		if (key == K_RIGHTARROW)
			dir = 1;
		else dir = -1;
		switch(selectedoption)
		{
		case MEDIA_VOLUME:
			bgmvolume.value += dir * 0.1;
			if (bgmvolume.value < 0)
				bgmvolume.value = 0;
			if (bgmvolume.value > 1)
				bgmvolume.value = 1;
			Cvar_SetValue (&bgmvolume, bgmvolume.value);
			break;
		case MEDIA_FASTFORWARD:
			Media_Seek(15*dir);
			break;
		default:
			if (selectedoption >= 0)
				Media_Next_f();
			break;
		}
	}
	else if (key == K_DOWNARROW)
	{
		selectedoption++;
		if (selectedoption>=numtracks)
			selectedoption = numtracks-1;
	}
	else if (key == K_PGDN)
	{
		selectedoption+=10;
		if (selectedoption>=numtracks)
			selectedoption = numtracks-1;
	}
	else if (key == K_UPARROW)
	{
		selectedoption--;
		if (selectedoption < MEDIA_MIN)
			selectedoption = MEDIA_MIN;
	}
	else if (key == K_PGUP)
	{
		selectedoption-=10;
		if (selectedoption < MEDIA_MIN)
			selectedoption = MEDIA_MIN;
	}
	else if (key == K_DEL)
	{
		if (selectedoption>=0)
		{
			mediatrack_t *prevtrack=NULL, *tr;
			int num=0;
			tr=tracks;
			while(tr)
			{
				if (num == selectedoption)
				{
					if (prevtrack)
						prevtrack->next = tr->next;
					else
						tracks = tr->next;
					numtracks--;
					if (!strcmp(media_currenttrack, tr->filename))
						Media_Changed(MEDIA_PLAYLIST);
					Z_Free(tr);
					break;
				}

				prevtrack = tr;
				tr=tr->next;
				num++;
			}
		}
	}
	else if (key == K_ENTER || key == K_KP_ENTER)
	{
		switch(selectedoption)
		{
		case MEDIA_FASTFORWARD:
			Media_Seek(15);
			break;
		case MEDIA_CLEARLIST:
			{
				mediatrack_t *prevtrack;
				while(tracks)
				{
					prevtrack = tracks;
					tracks=tracks->next;
					Z_Free(prevtrack);
					numtracks--;
				}
				if (numtracks!=0)
				{
					numtracks=0;
					Con_SafePrintf("numtracks should be 0\n");
				}
				Media_Changed(MEDIA_PLAYLIST);
			}
			break;
		case MEDIA_ADDTRACK:
			if (*media_iofilename)
				Media_AddTrack(media_iofilename);
			else
				Cmd_ExecuteString("menu_mediafiles", RESTRICT_LOCAL);
			break;
		case MEDIA_ADDLIST:
			if (*media_iofilename)
				Media_LoadTrackNames(media_iofilename);
			break;
		case MEDIA_SHUFFLE:
			Cvar_Set(&media_shuffle, media_shuffle.value?"0":"1");
			break;
		case MEDIA_REPEAT:
			Cvar_Set(&media_repeat, media_repeat.value?"0":"1");
			break;
		default:
			if (selectedoption>=0)
			{
				nexttrack = selectedoption;
				Media_Next_f();
				return true;
			}
			return false;
		}
		return true;
	}
	else
	{
		if (selectedoption == MEDIA_ADDLIST || selectedoption == MEDIA_ADDTRACK)
		{
			if (key == K_TAB)
			{
				Com_CompleateOSFileName(media_iofilename);
				return true;
			}
			else if (key == K_BACKSPACE)
			{
				dir = strlen(media_iofilename);
				if (dir)
					media_iofilename[dir-1] = '\0';
				return true;
			}
			else if ((key >= 'a' && key <= 'z') || (key >= '0' && key <= '9') || key == '/' || key == '_' || key == '.' || key == ':')
			{
				dir = strlen(media_iofilename);
				media_iofilename[dir] = key;
				media_iofilename[dir+1] = '\0';
				return true;
			}
		}
		else if (selectedoption>=0)
		{
			mediatrack_t *tr;
			int num=0;
			tr=tracks;
			while(tr)
			{
				if (num == selectedoption)
					break;

				tr=tr->next;
				num++;
			}
			if (!tr)
				return false;

			if (key == K_BACKSPACE)
			{
				dir = strlen(tr->nicename);
				if (dir)
					tr->nicename[dir-1] = '\0';
				return true;
			}
			else if ((key >= 'a' && key <= 'z') || (key >= '0' && key <= '9') || key == '/' || key == '_' || key == '.' || key == ':'  || key == '&' || key == '|' || key == '#' || key == '\'' || key == '\"' || key == '\\' || key == '*' || key == '@' || key == '!' || key == '(' || key == ')' || key == '%' || key == '^' || key == '?' || key == '[' || key == ']' || key == ';' || key == ':' || key == '+' || key == '-' || key == '=')
			{
				dir = strlen(tr->nicename);
				tr->nicename[dir] = key;
				tr->nicename[dir+1] = '\0';
				return true;
			}
		}
	}
	return false;
}

void M_Menu_Media_f (void)
{
	menu_t *menu;
	m_state = m_complex;
	Key_Dest_Add(kdm_emenu);
	menu = M_CreateMenu(0);

//	MC_AddPicture(menu, 16, 4, 32, 144, "gfx/qplaque.lmp");
	MC_AddCenterPicture(menu, 4, 24, "gfx/p_option.lmp");

	menu->key = M_Media_Key;
	menu->postdraw = M_Media_Draw;
}



//safeprints only.
void Media_LoadTrackNames (char *listname)
{
	char *lineend;
	char *len;
	char *filename;
	char *trackname;
	mediatrack_t *newtrack;
	size_t fsize;
	char *data = COM_LoadTempFile(listname, &fsize);

	loadedtracknames=true;

	if (!data)
		return;

	if (!Q_strncasecmp(data, "#extm3u", 7))
	{
		data = strchr(data, '\n')+1;
		for(;;)
		{
			lineend = strchr(data, '\n');

			if (Q_strncasecmp(data, "#extinf:", 8))
			{
				if (!lineend)
					return;
				Con_SafePrintf("Bad m3u file\n");
				return;
			}
			len = data+8;
			trackname = strchr(data, ',')+1;
			if (!trackname)
				return;

			lineend[-1]='\0';

			filename = data = lineend+1;

			lineend = strchr(data, '\n');

			if (lineend)
			{
				lineend[-1]='\0';
				data = lineend+1;
			}

			newtrack = Z_Malloc(sizeof(mediatrack_t));
#ifndef _WIN32	//crossplatform - lcean up any dos names
			if (filename[1] == ':')
			{
				snprintf(newtrack->filename, sizeof(newtrack->filename)-1, "/mnt/%c/%s", filename[0]-'A'+'a', filename+3);
				while((filename = strchr(newtrack->filename, '\\')))
					*filename = '/';

			}
			else
#endif
				Q_strncpyz(newtrack->filename, filename, sizeof(newtrack->filename));
			Q_strncpyz(newtrack->nicename, trackname, sizeof(newtrack->nicename));
			newtrack->length = atoi(len);
			newtrack->next = tracks;
			tracks = newtrack;
			numtracks++;

			if (!lineend)
				return;
		}
	}
	else
	{
		for(;;)
		{
			trackname = filename = data;
			lineend = strchr(data, '\n');

			if (!lineend && !*data)
				break;
			lineend[-1]='\0';
			data = lineend+1;

			newtrack = Z_Malloc(sizeof(mediatrack_t));
			Q_strncpyz(newtrack->filename, filename, sizeof(newtrack->filename));
			Q_strncpyz(newtrack->nicename, COM_SkipPath(trackname), sizeof(newtrack->nicename));
			newtrack->length = 0;
			newtrack->next = tracks;
			tracks = newtrack;
			numtracks++;

			if (!lineend)
				break;
		}
	}
}
#endif

//mixer is locked, its safe to do stuff, but try not to block
float Media_CrossFade(int musicchanel, float vol, float time)
{
	if (media_fadeout)
	{
		float fadetime = 1;
		float frac = (fadetime + media_fadeouttime - realtime)/fadetime;
		vol *= frac;
	}
	else if (music_playlist_index.modified)
	{
		if (Media_Changed(MEDIA_CVARLIST))
		{
			if (music_playlist_last >= 0)
			{
				cvar_t *sampleposition = Cvar_Get(va("music_playlist_sampleposition%i", music_playlist_last), "-1", 0, "compat");
				if (sampleposition && sampleposition->value != -1)
					Cvar_SetValue(sampleposition, time);
			}
			vol = -1;	//kill it NOW
		}
	}
	return vol;
}

//mixer is locked, its safe to do stuff, but try not to block
char *Media_NextTrack(int musicchannelnum, float *starttime)
{
	if (bgmvolume.value <= 0)
		return NULL;

	if (media_fadeout)
	{
		if (S_Music_Playing(musicchannelnum))
			return NULL;	//can't pick a new track until they've all stopped.

		//okay, it has actually stopped everywhere.
	}
	media_fadeout = false;	//it has actually ended now

	music_playlist_index.modified = false;
	music_playlist_last = -1;
	media_playlistcurrent = 0;
	Q_strncpyz(media_currenttrack, "", sizeof(media_currenttrack));
	Q_strncpyz(media_friendlyname, "", sizeof(media_friendlyname));
	if (!media_playlistcurrent && (media_playlisttypes & MEDIA_PLAYLIST))
	{
#if !defined(NOMEDIAMENU) && !defined(NOBUILTINMENUS)
		if (!loadedtracknames)
			Media_LoadTrackNames("sound/media.m3u");
#endif
		if (Media_EvaluateNextTrack())
		{
			media_playlistcurrent = MEDIA_PLAYLIST;
			return media_currenttrack;
		}
	}
	if (!media_playlistcurrent && (media_playlisttypes & MEDIA_CVARLIST))
	{
		if (music_playlist_index.ival >= 0)
		{
			cvar_t *list = Cvar_Get(va("music_playlist_list%i", music_playlist_index.ival), "", 0, "compat");
			if (list)
			{
				cvar_t *sampleposition = Cvar_Get(va("music_playlist_sampleposition%i", music_playlist_index.ival), "-1", 0, "compat");
				Q_snprintfz(media_currenttrack, sizeof(media_currenttrack), "sound/cdtracks/%s", list->string);
				Q_strncpyz(media_friendlyname, "", sizeof(media_friendlyname));
				media_playlistcurrent = MEDIA_CVARLIST;
				music_playlist_last = music_playlist_index.ival;
				if (sampleposition)
				{
					*starttime = sampleposition->value;
					if (*starttime == -1)
						*starttime = 0;
				}
				else
					*starttime = 0;
			}
		}
	}
	if (!media_playlistcurrent && (media_playlisttypes & MEDIA_GAMEMUSIC))
	{
		if (cdplaytracknum)
		{
			if (cdplayingtrack != cdplaytracknum && cdpausedtrack != cdplaytracknum)
			{
				CDAudio_Play(cdplaytracknum);
				cdplayingtrack = cdplaytracknum;
			}
			media_playlistcurrent = MEDIA_GAMEMUSIC;
		}
		else if (*cdplaytrack)
		{
			Q_strncpyz(media_currenttrack, cdplaytrack, sizeof(media_currenttrack));
			Q_strncpyz(media_friendlyname, "", sizeof(media_friendlyname));
			media_playlistcurrent = MEDIA_GAMEMUSIC;
		}
	}

	return media_currenttrack;
}















#undef	dwFlags
#undef	lpFormat
#undef	lpData
#undef	cbData
#undef	lTime


///temporary residence for media handling
#include "roq.h"


#ifdef WINAVI
#undef CDECL	//windows is stupid at times.
#define CDECL __cdecl

#if defined(_MSC_VER) && (_MSC_VER < 1300)
#define DWORD_PTR DWORD
#endif

#if 0
#include <msacm.h>
#else
DECLARE_HANDLE(HACMSTREAM);
typedef HACMSTREAM *LPHACMSTREAM;
DECLARE_HANDLE(HACMDRIVER);
typedef struct {
	DWORD     cbStruct;
	DWORD     fdwStatus;
	DWORD_PTR dwUser;
	LPBYTE    pbSrc;
	DWORD     cbSrcLength;
	DWORD     cbSrcLengthUsed;
	DWORD_PTR dwSrcUser;
	LPBYTE    pbDst;
	DWORD     cbDstLength;
	DWORD     cbDstLengthUsed;
	DWORD_PTR dwDstUser;
	DWORD     dwReservedDriver[10];
} ACMSTREAMHEADER, *LPACMSTREAMHEADER;
#define ACM_STREAMCONVERTF_BLOCKALIGN   0x00000004
#endif

//mingw workarounds
#define LPWAVEFILTER void *
#include <objbase.h>

MMRESULT (WINAPI *qacmStreamUnprepareHeader) (HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwUnprepare);
MMRESULT (WINAPI *qacmStreamConvert) (HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwConvert);
MMRESULT (WINAPI *qacmStreamPrepareHeader) (HACMSTREAM has, LPACMSTREAMHEADER pash, DWORD fdwPrepare);
MMRESULT (WINAPI *qacmStreamOpen) (LPHACMSTREAM phas, HACMDRIVER had, LPWAVEFORMATEX pwfxSrc, LPWAVEFORMATEX pwfxDst, LPWAVEFILTER pwfltr, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen);
MMRESULT (WINAPI *qacmStreamClose) (HACMSTREAM has, DWORD fdwClose);

static qboolean qacmStartup(void)
{
	static int inited;
	static dllhandle_t *module;
	if (!inited)
	{
		dllfunction_t funcs[] =
		{
			{(void*)&qacmStreamUnprepareHeader,	"acmStreamUnprepareHeader"},
			{(void*)&qacmStreamConvert,			"acmStreamConvert"},
			{(void*)&qacmStreamPrepareHeader,	"acmStreamPrepareHeader"},
			{(void*)&qacmStreamOpen,			"acmStreamOpen"},
			{(void*)&qacmStreamClose,			"acmStreamClose"},
			{NULL,NULL}
		};
		inited = true;
		module = Sys_LoadLibrary("msacm32.dll", funcs);
	}

	return module?true:false;
}

#if 0
#include <vfw.h>
#else
typedef struct 
{
	DWORD fccType;
	DWORD fccHandler;
	DWORD dwFlags;
	DWORD dwCaps;
	WORD  wPriority;
	WORD  wLanguage;
	DWORD dwScale;
	DWORD dwRate;
	DWORD dwStart;
	DWORD dwLength;
	DWORD dwInitialFrames;
	DWORD dwSuggestedBufferSize;
	DWORD dwQuality;
	DWORD dwSampleSize;
	RECT  rcFrame;
	DWORD dwEditCount;
	DWORD dwFormatChangeCount;
	TCHAR szName[64];
} AVISTREAMINFOA, *LPAVISTREAMINFOA;
typedef struct AVISTREAM *PAVISTREAM;
typedef struct AVIFILE *PAVIFILE;
typedef struct GETFRAME *PGETFRAME;
typedef struct	 
{
	DWORD  fccType;
	DWORD  fccHandler;
	DWORD  dwKeyFrameEvery;
	DWORD  dwQuality;
	DWORD  dwBytesPerSecond;
	DWORD  dwFlags;
	LPVOID lpFormat;
	DWORD  cbFormat;
	LPVOID lpParms;
	DWORD  cbParms;
	DWORD  dwInterleaveEvery;
} AVICOMPRESSOPTIONS;
#define streamtypeVIDEO         mmioFOURCC('v', 'i', 'd', 's')
#define streamtypeAUDIO         mmioFOURCC('a', 'u', 'd', 's')
#define AVISTREAMREAD_CONVENIENT	(-1L)
#define AVIIF_KEYFRAME	0x00000010L
#endif

ULONG	(WINAPI *qAVIStreamRelease)			(PAVISTREAM pavi);
HRESULT	(WINAPI *qAVIStreamEndStreaming)	(PAVISTREAM pavi);
HRESULT	(WINAPI *qAVIStreamGetFrameClose)	(PGETFRAME pg);
HRESULT	(WINAPI *qAVIStreamRead)			(PAVISTREAM pavi, LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, LONG FAR * plBytes, LONG FAR * plSamples);
LPVOID	(WINAPI *qAVIStreamGetFrame)		(PGETFRAME pg, LONG lPos);
HRESULT	(WINAPI *qAVIStreamReadFormat)		(PAVISTREAM pavi, LONG lPos,LPVOID lpFormat,LONG FAR *lpcbFormat);
LONG	(WINAPI *qAVIStreamStart)			(PAVISTREAM pavi);
PGETFRAME(WINAPI*qAVIStreamGetFrameOpen)	(PAVISTREAM pavi, LPBITMAPINFOHEADER lpbiWanted);
HRESULT	(WINAPI *qAVIStreamBeginStreaming)	(PAVISTREAM pavi, LONG lStart, LONG lEnd, LONG lRate);
LONG	(WINAPI *qAVIStreamSampleToTime)	(PAVISTREAM pavi, LONG lSample);
LONG	(WINAPI *qAVIStreamLength)			(PAVISTREAM pavi);
HRESULT	(WINAPI *qAVIStreamInfoA)			(PAVISTREAM pavi, LPAVISTREAMINFOA psi, LONG lSize);
ULONG	(WINAPI *qAVIFileRelease)			(PAVIFILE pfile);
HRESULT	(WINAPI *qAVIFileGetStream)			(PAVIFILE pfile, PAVISTREAM FAR * ppavi, DWORD fccType, LONG lParam);
HRESULT	(WINAPI *qAVIFileOpenA)				(PAVIFILE FAR *ppfile, LPCSTR szFile, UINT uMode, LPCLSID lpHandler);
void	(WINAPI *qAVIFileInit)				(void);
HRESULT	(WINAPI *qAVIStreamWrite)			(PAVISTREAM pavi, LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, DWORD dwFlags, LONG FAR *plSampWritten, LONG FAR *plBytesWritten);
HRESULT	(WINAPI *qAVIStreamSetFormat)		(PAVISTREAM pavi, LONG lPos,LPVOID lpFormat,LONG cbFormat);
HRESULT	(WINAPI *qAVIMakeCompressedStream)	(PAVISTREAM FAR * ppsCompressed, PAVISTREAM ppsSource, AVICOMPRESSOPTIONS FAR * lpOptions, CLSID FAR *pclsidHandler);
HRESULT	(WINAPI *qAVIFileCreateStreamA)		(PAVIFILE pfile, PAVISTREAM FAR *ppavi, AVISTREAMINFOA FAR * psi);

static qboolean qAVIStartup(void)
{
	static int aviinited;
	static dllhandle_t *avimodule;
	if (!aviinited)
	{
		dllfunction_t funcs[] =
		{
			{(void*)&qAVIFileInit,				"AVIFileInit"},
			{(void*)&qAVIStreamRelease,			"AVIStreamRelease"},
			{(void*)&qAVIStreamEndStreaming,	"AVIStreamEndStreaming"},
			{(void*)&qAVIStreamGetFrameClose,	"AVIStreamGetFrameClose"},
			{(void*)&qAVIStreamRead,			"AVIStreamRead"},
			{(void*)&qAVIStreamGetFrame,		"AVIStreamGetFrame"},
			{(void*)&qAVIStreamReadFormat,		"AVIStreamReadFormat"},
			{(void*)&qAVIStreamStart,			"AVIStreamStart"},
			{(void*)&qAVIStreamGetFrameOpen,	"AVIStreamGetFrameOpen"},
			{(void*)&qAVIStreamBeginStreaming,	"AVIStreamBeginStreaming"},
			{(void*)&qAVIStreamSampleToTime,	"AVIStreamSampleToTime"},
			{(void*)&qAVIStreamLength,			"AVIStreamLength"},
			{(void*)&qAVIStreamInfoA,			"AVIStreamInfoA"},
			{(void*)&qAVIFileRelease,			"AVIFileRelease"},
			{(void*)&qAVIFileGetStream,			"AVIFileGetStream"},
			{(void*)&qAVIFileOpenA,				"AVIFileOpenA"},
			{(void*)&qAVIStreamWrite,			"AVIStreamWrite"},
			{(void*)&qAVIStreamSetFormat,		"AVIStreamSetFormat"},
			{(void*)&qAVIMakeCompressedStream,	"AVIMakeCompressedStream"},
			{(void*)&qAVIFileCreateStreamA,		"AVIFileCreateStreamA"},
			{NULL,NULL}
		};
		aviinited = true;
		avimodule = Sys_LoadLibrary("avifil32.dll", funcs);

		if (avimodule)
			qAVIFileInit();
	}

	return avimodule?true:false;
}
#endif

struct cin_s
{
	qboolean (*decodeframe)(cin_t *cin, qboolean nosound);
	void (*doneframe)(cin_t *cin);
	void (*shutdown)(cin_t *cin);	//warning: doesn't free cin_t
	void (*rewind)(cin_t *cin);
	//these are any interactivity functions you might want...
	void (*cursormove) (struct cin_s *cin, float posx, float posy);	//pos is 0-1
	void (*key) (struct cin_s *cin, int code, int unicode, int event);
	qboolean (*setsize) (struct cin_s *cin, int width, int height);
	void (*getsize) (struct cin_s *cin, int *width, int *height, float *aspect);
	void (*changestream) (struct cin_s *cin, const char *streamname);



	//these are the outputs (not always power of two!)
	enum uploadfmt outtype;
	int outwidth;
	int outheight;
	qbyte *outdata;
	qbyte *outpalette;
	int outunchanged;
	qboolean ended;
	float filmpercentage;

	texid_t texture;


#ifdef WINAVI
	struct {
		qboolean resettimer;
		AVISTREAMINFOA		vidinfo;
		PAVISTREAM			pavivideo;
		AVISTREAMINFOA		audinfo;
		PAVISTREAM			pavisound;
		PAVIFILE			pavi;
		PGETFRAME			pgf;

		HACMSTREAM			audiodecoder;

		LPWAVEFORMATEX pWaveFormat;

		//sound stuff
		int soundpos;

		//source info
		float filmfps;
		int num_frames;
	} avi;
#endif

#ifdef PLUGINS
	struct {
		void *ctx;
		struct plugin_s *plug;
		media_decoder_funcs_t *funcs;	/*fixme*/
		struct cin_s *next;
		struct cin_s *prev;
	} plugin;
#endif

	struct {
		qbyte *filmimage;	//rgba
		int imagewidth;
		int imageheight;
	} image;

	struct {
		roq_info *roqfilm;
	} roq;

	struct {
		struct cinematics_s *cin;
	} q2cin;

	float filmstarttime;
	float nextframetime;
	float filmlasttime;

	int currentframe;	//last frame in buffer
	qbyte *framedata;	//Z_Malloced buffer
};

shader_t *videoshader;

//////////////////////////////////////////////////////////////////////////////////
//AVI Support (windows)
#ifdef WINAVI
void Media_WINAVI_Shutdown(struct cin_s *cin)
{
	qAVIStreamGetFrameClose(cin->avi.pgf);
	qAVIStreamEndStreaming(cin->avi.pavivideo);
	qAVIStreamRelease(cin->avi.pavivideo);
	//we don't need to free the file (we freed it immediatly after getting the stream handles)
}
qboolean Media_WinAvi_DecodeFrame(cin_t *cin, qboolean nosound)
{
	LPBITMAPINFOHEADER lpbi;									// Holds The Bitmap Header Information
	float newframe;
	int newframei;
	int wantsoundtime;
	extern cvar_t _snd_mixahead;

	float curtime = Sys_DoubleTime();

	if (cin->avi.resettimer)
	{
		cin->filmstarttime = curtime;
		cin->avi.resettimer = 0;
		newframe = 0;
		newframei = newframe;
	}
	else
	{
		newframe = (((curtime - cin->filmstarttime) * cin->avi.vidinfo.dwRate) / cin->avi.vidinfo.dwScale) + cin->avi.vidinfo.dwInitialFrames;
		newframei = newframe;

		if (newframei>=cin->avi.num_frames)
			cin->ended = true;

		if (newframei == cin->currentframe)
		{
			cin->outunchanged = true;
			return true;
		}
	}
	cin->outunchanged = false;

	if (cin->currentframe < newframei-1)
		Con_DPrintf("Dropped %i frame(s)\n", (newframei - cin->currentframe)-1);

	cin->currentframe = newframei;
	cin->filmpercentage = (float)cin->currentframe / cin->avi.num_frames;

	if (newframei>=cin->avi.num_frames)
	{
		cin->filmstarttime = curtime;
		cin->currentframe = newframei = 0;
		cin->avi.soundpos = 0;
	}

	lpbi = (LPBITMAPINFOHEADER)qAVIStreamGetFrame(cin->avi.pgf, cin->currentframe);	// Grab Data From The AVI Stream
	if (!lpbi || lpbi->biBitCount != 24)//oops
	{
		cin->avi.resettimer = true;
		cin->ended = true;
		return false;
	}
	else
	{
		cin->outtype = TF_BGR24_FLIP;
		cin->outwidth = lpbi->biWidth;
		cin->outheight = lpbi->biHeight;
		cin->outdata = (char*)lpbi+lpbi->biSize;
	}

	if(nosound)
		wantsoundtime = 0;
	else
		wantsoundtime = ((((curtime - cin->filmstarttime) + _snd_mixahead.value + 0.02) * cin->avi.audinfo.dwRate) / cin->avi.audinfo.dwScale) + cin->avi.audinfo.dwInitialFrames;

	while (cin->avi.pavisound && cin->avi.soundpos < wantsoundtime)
	{
		LONG lSize;
		LPBYTE pBuffer;
		LONG samples;

		/*if the audio skipped more than a second, drop it all and start at a sane time, so our raw audio playing code doesn't buffer too much*/
		if (cin->avi.soundpos + (1*cin->avi.audinfo.dwRate / cin->avi.audinfo.dwScale) < wantsoundtime)
		{
			cin->avi.soundpos = wantsoundtime;
			break;
		}

		qAVIStreamRead(cin->avi.pavisound, cin->avi.soundpos, AVISTREAMREAD_CONVENIENT, NULL, 0, &lSize, &samples);
		pBuffer = cin->framedata;
		qAVIStreamRead(cin->avi.pavisound, cin->avi.soundpos, AVISTREAMREAD_CONVENIENT, pBuffer, lSize, NULL, &samples);

		cin->avi.soundpos+=samples;

		/*if no progress, stop!*/
		if (!samples)
			break;

		if (cin->avi.audiodecoder)
		{
			ACMSTREAMHEADER strhdr;
			char buffer[1024*256];

			memset(&strhdr, 0, sizeof(strhdr));
			strhdr.cbStruct = sizeof(strhdr);
			strhdr.pbSrc = pBuffer;
			strhdr.cbSrcLength = lSize;
			strhdr.pbDst = buffer;
			strhdr.cbDstLength = sizeof(buffer);

			qacmStreamPrepareHeader(cin->avi.audiodecoder, &strhdr, 0);
			qacmStreamConvert(cin->avi.audiodecoder, &strhdr, ACM_STREAMCONVERTF_BLOCKALIGN);
			qacmStreamUnprepareHeader(cin->avi.audiodecoder, &strhdr, 0);

			S_RawAudio(-1, strhdr.pbDst, cin->avi.pWaveFormat->nSamplesPerSec, strhdr.cbDstLengthUsed/4, cin->avi.pWaveFormat->nChannels, 2, 1);
		}
		else
			S_RawAudio(-1, pBuffer, cin->avi.pWaveFormat->nSamplesPerSec, samples, cin->avi.pWaveFormat->nChannels, 2, 1);
	}
	return true;
}

cin_t *Media_WinAvi_TryLoad(char *name)
{
	cin_t *cin;
	PAVIFILE			pavi;
	flocation_t loc;

	if (strchr(name, ':'))
		return NULL;

	if (!qAVIStartup())
		return NULL;


	FS_FLocateFile(name, FSLF_IFFOUND, &loc);

	if (!loc.offset && *loc.rawname && !qAVIFileOpenA(&pavi, loc.rawname, OF_READ, NULL))//!AVIStreamOpenFromFile(&pavi, name, streamtypeVIDEO, 0, OF_READ, NULL))
	{
		int filmwidth;
		int filmheight;

		cin = Z_Malloc(sizeof(cin_t));
		cin->avi.pavi = pavi;

		if (qAVIFileGetStream(cin->avi.pavi, &cin->avi.pavivideo, streamtypeVIDEO, 0))	//retrieve video stream
		{
			qAVIFileRelease(pavi);
			Con_Printf("%s contains no video stream\n", name);
			return NULL;
		}
		if (qAVIFileGetStream(cin->avi.pavi, &cin->avi.pavisound, streamtypeAUDIO, 0))	//retrieve audio stream
		{
			Con_DPrintf("%s contains no audio stream\n", name);
			cin->avi.pavisound=NULL;
		}
		qAVIFileRelease(cin->avi.pavi);

//play with video
		qAVIStreamInfoA(cin->avi.pavivideo, &cin->avi.vidinfo, sizeof(cin->avi.vidinfo));
		filmwidth=cin->avi.vidinfo.rcFrame.right-cin->avi.vidinfo.rcFrame.left;					// Width Is Right Side Of Frame Minus Left
		filmheight=cin->avi.vidinfo.rcFrame.bottom-cin->avi.vidinfo.rcFrame.top;					// Height Is Bottom Of Frame Minus Top
		cin->framedata = BZ_Malloc(filmwidth*filmheight*4);

		cin->avi.num_frames=qAVIStreamLength(cin->avi.pavivideo);							// The Last Frame Of The Stream
		cin->avi.filmfps=1000.0f*(float)cin->avi.num_frames/(float)qAVIStreamSampleToTime(cin->avi.pavivideo,cin->avi.num_frames);		// Calculate Rough Milliseconds Per Frame

		qAVIStreamBeginStreaming(cin->avi.pavivideo, 0, cin->avi.num_frames, 100);

		cin->avi.pgf=qAVIStreamGetFrameOpen(cin->avi.pavivideo, NULL);

		if (!cin->avi.pgf)
		{
			Con_Printf("AVIStreamGetFrameOpen failed. Please install a vfw codec for '%c%c%c%c'. Try ffdshow.\n", 
				((unsigned char*)&cin->avi.vidinfo.fccHandler)[0],
				((unsigned char*)&cin->avi.vidinfo.fccHandler)[1],
				((unsigned char*)&cin->avi.vidinfo.fccHandler)[2],
				((unsigned char*)&cin->avi.vidinfo.fccHandler)[3]
				);
		}

		cin->currentframe=0;
		cin->filmstarttime = Sys_DoubleTime();

		cin->avi.soundpos=0;
		cin->avi.resettimer = true;


//play with sound
		if (cin->avi.pavisound)
		{
			LONG lSize;
			LPBYTE pChunk;
			qAVIStreamInfoA(cin->avi.pavisound, &cin->avi.audinfo, sizeof(cin->avi.audinfo));

			qAVIStreamRead(cin->avi.pavisound, 0, AVISTREAMREAD_CONVENIENT, NULL, 0, &lSize, NULL);

			if (!lSize)
				cin->avi.pWaveFormat = NULL;
			else
			{
				pChunk = BZ_Malloc(sizeof(qbyte)*lSize);

				if(qAVIStreamReadFormat(cin->avi.pavisound, qAVIStreamStart(cin->avi.pavisound), pChunk, &lSize))
				{
				   // error
					Con_Printf("Failiure reading sound info\n");
				}
				cin->avi.pWaveFormat = (LPWAVEFORMATEX)pChunk;
			}

			if (!cin->avi.pWaveFormat)
			{
				Con_Printf("VFW is broken\n");
				qAVIStreamRelease(cin->avi.pavisound);
				cin->avi.pavisound=NULL;
			}
			else if (cin->avi.pWaveFormat->wFormatTag != 1)
			{
				WAVEFORMATEX pcm_format;
				HACMDRIVER drv = NULL;

				memset (&pcm_format, 0, sizeof(pcm_format));
				pcm_format.wFormatTag = WAVE_FORMAT_PCM;
				pcm_format.nChannels = cin->avi.pWaveFormat->nChannels;
				pcm_format.nSamplesPerSec = cin->avi.pWaveFormat->nSamplesPerSec;
				pcm_format.nBlockAlign = 4;
				pcm_format.nAvgBytesPerSec = pcm_format.nSamplesPerSec*4;
				pcm_format.wBitsPerSample = 16;
				pcm_format.cbSize = 0;

				if (!qacmStartup() || 0!=qacmStreamOpen(&cin->avi.audiodecoder, drv, cin->avi.pWaveFormat, &pcm_format, NULL, 0, 0, 0))
				{
					Con_Printf("Failed to open audio decoder\n");	//FIXME: so that it no longer is...
					qAVIStreamRelease(cin->avi.pavisound);
					cin->avi.pavisound=NULL;
				}
			}
		}

		cin->decodeframe = Media_WinAvi_DecodeFrame;
		cin->shutdown = Media_WINAVI_Shutdown;
		return cin;
	}
	return NULL;
}
#endif

//AVI Support (windows)
//////////////////////////////////////////////////////////////////////////////////
//Plugin Support
#ifdef PLUGINS
static media_decoder_funcs_t *plugindecodersfunc[8];
static struct plugin_s *plugindecodersplugin[8];
static cin_t *active_cin_plugins;

qboolean Media_RegisterDecoder(struct plugin_s *plug, media_decoder_funcs_t *funcs)
{
	int i;
	for (i = 0; i < sizeof(plugindecodersfunc)/sizeof(plugindecodersfunc[0]); i++)
	{
		if (plugindecodersfunc[i] == NULL)
		{
			plugindecodersfunc[i] = funcs;
			plugindecodersplugin[i] = plug;
			return true;
		}
	}
	return false;
}
/*funcs==null closes ALL decoders from this plugin*/
qboolean Media_UnregisterDecoder(struct plugin_s *plug, media_decoder_funcs_t *funcs)
{
	qboolean success = true;
	int i;
	static media_decoder_funcs_t deadfuncs;
	struct plugin_s *oldplug = currentplug;
	cin_t *cin, *next;

	for (i = 0; i < sizeof(plugindecodersfunc)/sizeof(plugindecodersfunc[0]); i++)
	{
		if (plugindecodersfunc[i] == funcs || (!funcs && plugindecodersplugin[i] == plug))
		{
			//kill any cinematics currently using that decoder
			for (cin = active_cin_plugins; cin; cin = next)
			{
				next = cin->plugin.next;

				if (cin->plugin.plug == plug && cin->plugin.funcs == plugindecodersfunc[i])
				{
					//we don't kill the engine's side of it, not just yet anyway.
					currentplug = cin->plugin.plug;
					if (cin->plugin.funcs->shutdown)
						cin->plugin.funcs->shutdown(cin->plugin.ctx);
					cin->plugin.funcs = &deadfuncs;
					cin->plugin.plug = NULL;
					cin->plugin.ctx = NULL;
				}
			}
			currentplug = oldplug;


			plugindecodersfunc[i] = NULL;
			plugindecodersplugin[i] = NULL;
			if (funcs)
				return success;
		}
	}

	if (!funcs)
	{
		static media_decoder_funcs_t deadfuncs;
		struct plugin_s *oldplug = currentplug;
		cin_t *cin, *next;

		for (cin = active_cin_plugins; cin; cin = next)
		{
			next = cin->plugin.next;

			if (cin->plugin.plug == plug)
			{
				//we don't kill the engine's side of it, not just yet anyway.
				currentplug = cin->plugin.plug;
				if (cin->plugin.funcs->shutdown)
					cin->plugin.funcs->shutdown(cin->plugin.ctx);
				cin->plugin.funcs = &deadfuncs;
				cin->plugin.plug = NULL;
				cin->plugin.ctx = NULL;
			}
		}
		currentplug = oldplug;
	}
	return success;
}

static qboolean Media_Plugin_DecodeFrame(cin_t *cin, qboolean nosound)
{
	struct plugin_s *oldplug = currentplug;
	if (!cin->plugin.funcs->decodeframe)
		return false;	//plugin closed or something

	currentplug = cin->plugin.plug;
	cin->outdata = cin->plugin.funcs->decodeframe(cin->plugin.ctx, nosound, &cin->outtype, &cin->outwidth, &cin->outheight);
	currentplug = oldplug;

	cin->outunchanged = (cin->outdata==NULL);

	cin->filmpercentage = 0;
	cin->ended = (cin->outtype == TF_INVALID);
	return !cin->ended;
}
static void Media_Plugin_DoneFrame(cin_t *cin)
{
	struct plugin_s *oldplug = currentplug;
	currentplug = cin->plugin.plug;
	if (cin->plugin.funcs->doneframe)
		cin->plugin.funcs->doneframe(cin->plugin.ctx, cin->outdata);
	currentplug = oldplug;
}
static void Media_Plugin_Shutdown(cin_t *cin)
{
	struct plugin_s *oldplug = currentplug;
	currentplug = cin->plugin.plug;
	if (cin->plugin.funcs->shutdown)
		cin->plugin.funcs->shutdown(cin->plugin.ctx);
	currentplug = oldplug;

	if (cin->plugin.prev)
		cin->plugin.prev->plugin.next = cin->plugin.next;
	else
		active_cin_plugins = cin->plugin.next;
	if (cin->plugin.next)
		cin->plugin.next->plugin.prev = cin->plugin.prev;
}
static void Media_Plugin_Rewind(cin_t *cin)
{
	struct plugin_s *oldplug = currentplug;
	currentplug = cin->plugin.plug;
	if (cin->plugin.funcs->rewind)
		cin->plugin.funcs->rewind(cin->plugin.ctx);
	currentplug = oldplug;
}

void Media_Plugin_MoveCursor(cin_t *cin, float posx, float posy)
{
	struct plugin_s *oldplug = currentplug;
	currentplug = cin->plugin.plug;
	if (cin->plugin.funcs->cursormove)
		cin->plugin.funcs->cursormove(cin->plugin.ctx, posx, posy);
	currentplug = oldplug;
}
void Media_Plugin_KeyPress(cin_t *cin, int code, int unicode, int event)
{
	struct plugin_s *oldplug = currentplug;
	currentplug = cin->plugin.plug;
	if (cin->plugin.funcs->key)
		cin->plugin.funcs->key(cin->plugin.ctx, code, unicode, event);
	currentplug = oldplug;
}
qboolean Media_Plugin_SetSize(cin_t *cin, int width, int height)
{
	qboolean result = false;
	struct plugin_s *oldplug = currentplug;
	currentplug = cin->plugin.plug;
	if (cin->plugin.funcs->setsize)
		result = cin->plugin.funcs->setsize(cin->plugin.ctx, width, height);
	currentplug = oldplug;
	return result;
}
void Media_Plugin_GetSize(cin_t *cin, int *width, int *height, float *aspect)
{
	struct plugin_s *oldplug = currentplug;
	currentplug = cin->plugin.plug;
	if (cin->plugin.funcs->getsize)
		cin->plugin.funcs->getsize(cin->plugin.ctx, width, height);
	currentplug = oldplug;
}
void Media_Plugin_ChangeStream(cin_t *cin, const char *streamname)
{
	struct plugin_s *oldplug = currentplug;
	currentplug = cin->plugin.plug;
	if (cin->plugin.funcs->changestream)
		cin->plugin.funcs->changestream(cin->plugin.ctx, streamname);
	currentplug = oldplug;
}

cin_t *Media_Plugin_TryLoad(char *name)
{
	cin_t *cin;
	int i;
	media_decoder_funcs_t *funcs = NULL;
	struct plugin_s *plug = NULL;
	void *ctx = NULL;
	struct plugin_s *oldplug = currentplug;
	for (i = 0; i < sizeof(plugindecodersfunc)/sizeof(plugindecodersfunc[0]); i++)
	{
		funcs = plugindecodersfunc[i];
		if (funcs)
		{
			plug = plugindecodersplugin[i];
			currentplug = plug;
			ctx = funcs->createdecoder(name);
			if (ctx)
				break;
		}
	}
	currentplug = oldplug;

	if (ctx)
	{
		cin = Z_Malloc(sizeof(cin_t));
		cin->plugin.funcs = funcs;
		cin->plugin.plug = plug;
		cin->plugin.ctx = ctx;
		cin->plugin.next = active_cin_plugins;
		cin->plugin.prev = NULL;
		if (cin->plugin.next)
			cin->plugin.next->plugin.prev = cin;
		active_cin_plugins = cin;
		cin->decodeframe = Media_Plugin_DecodeFrame;
		cin->doneframe = Media_Plugin_DoneFrame;
		cin->shutdown = Media_Plugin_Shutdown;
		cin->rewind = Media_Plugin_Rewind;

		cin->cursormove = Media_Plugin_MoveCursor;
		cin->key = Media_Plugin_KeyPress;
		cin->setsize = Media_Plugin_SetSize;
		cin->getsize = Media_Plugin_GetSize;
		cin->changestream = Media_Plugin_ChangeStream;

		return cin;
	}
	return NULL;
}
#endif
//Plugin Support
//////////////////////////////////////////////////////////////////////////////////
//Quake3 RoQ Support

#ifdef Q3CLIENT
void Media_Roq_Shutdown(struct cin_s *cin)
{
	roq_close(cin->roq.roqfilm);
	cin->roq.roqfilm=NULL;
}

qboolean Media_Roq_DecodeFrame (cin_t *cin, qboolean nosound)
{
	float curtime = Sys_DoubleTime();

	if ((int)(cin->filmlasttime*30) == (int)((float)realtime*30) && cin->outtype != TF_INVALID)
	{
		cin->outunchanged = !!cin->outtype;
		return true;
	}
	else if (curtime<cin->nextframetime || roq_read_frame(cin->roq.roqfilm)==1)	 //0 if end, -1 if error, 1 if success
	{
	//#define LIMIT(x) ((x)<0xFFFF)?(x)>>16:0xFF;
#define LIMIT(x) ((((x) > 0xffffff) ? 0xff0000 : (((x) <= 0xffff) ? 0 : (x) & 0xff0000)) >> 16)
		unsigned char *pa=cin->roq.roqfilm->y[0];
		unsigned char *pb=cin->roq.roqfilm->u[0];
		unsigned char *pc=cin->roq.roqfilm->v[0];
		int pix=0;
		int num_columns=(cin->roq.roqfilm->width)>>1;
		int num_rows=cin->roq.roqfilm->height;
		int y;
		int x;

		qbyte *framedata;

		if (cin->roq.roqfilm->num_frames)
			cin->filmpercentage = cin->roq.roqfilm->frame_num / cin->roq.roqfilm->num_frames;
		else
			cin->filmpercentage = 0;

		cin->filmlasttime = (float)realtime;

		if (!(curtime<cin->nextframetime))	//roq file was read properly
		{
			cin->nextframetime += 1/30.0;	//add a little bit of extra speed so we cover up a little bit of glitchy sound... :o)

			if (cin->nextframetime < curtime)
				cin->nextframetime = curtime;

			framedata = cin->framedata;

			for(y = 0; y < num_rows; ++y)	//roq playing doesn't give nice data. It's still fairly raw.
			{										//convert it properly.
				for(x = 0; x < num_columns; ++x)
				{

					int r, g, b, y1, y2, u, v, t;
					y1 = *(pa++); y2 = *(pa++);
					u = pb[x] - 128;
					v = pc[x] - 128;

					y1 <<= 16;
					y2 <<= 16;
					r = 91881 * v;
					g = -22554 * u + -46802 * v;
					b = 116130 * u;

					t=r+y1;
					framedata[pix] =(unsigned char) LIMIT(t);
					t=g+y1;
					framedata[pix+1] =(unsigned char) LIMIT(t);
					t=b+y1;
					framedata[pix+2] =(unsigned char) LIMIT(t);

					t=r+y2;
					framedata[pix+4] =(unsigned char) LIMIT(t);
					t=g+y2;
					framedata[pix+5] =(unsigned char) LIMIT(t);
					t=b+y2;
					framedata[pix+6] =(unsigned char) LIMIT(t);
					pix+=8;

				}
				if(y & 0x01) { pb += num_columns; pc += num_columns; }
			}
		}

		cin->outunchanged = false;
		cin->outtype = TF_RGBA32;
		cin->outwidth = cin->roq.roqfilm->width;
		cin->outheight = cin->roq.roqfilm->height;
		cin->outdata = cin->framedata;

		if (!nosound)
		if (cin->roq.roqfilm->audio_channels && S_HaveOutput() && cin->roq.roqfilm->aud_pos < cin->roq.roqfilm->vid_pos)
		if (roq_read_audio(cin->roq.roqfilm)>0)
		{
/*				FILE *f;
			char wav[] = "\x52\x49\x46\x46\xea\x5f\x04\x00\x57\x41\x56\x45\x66\x6d\x74\x20\x12\x00\x00\x00\x01\x00\x02\x00\x22\x56\x00\x00\x88\x58\x01\x00\x04\x00\x10\x00\x00\x00\x66\x61\x63\x74\x04\x00\x00\x00\xee\x17\x01\x00\x64\x61\x74\x61\xb8\x5f\x04\x00";
			int size;

			f = fopen("d:/quake/id1/sound/raw.wav", "r+b");
			if (!f)
				f = fopen("d:/quake/id1/sound/raw.wav", "w+b");
			fseek(f, 0, SEEK_SET);
			fwrite(&wav, sizeof(wav), 1, f);
			fseek(f, 0, SEEK_END);
			fwrite(roqfilm->audio, roqfilm->audio_size, 2, f);
			size = ftell(f) - sizeof(wav);
			fseek(f, 54, SEEK_SET);
			fwrite(&size, sizeof(size), 1, f);
			fclose(f);
*/
			S_RawAudio(-1, cin->roq.roqfilm->audio, 22050, cin->roq.roqfilm->audio_size/cin->roq.roqfilm->audio_channels, cin->roq.roqfilm->audio_channels, 2, 1);
		}

		return true;
	}
	else
	{
		cin->ended = true;
		cin->roq.roqfilm->frame_num = 0;
		cin->roq.roqfilm->aud_pos = cin->roq.roqfilm->roq_start;
		cin->roq.roqfilm->vid_pos = cin->roq.roqfilm->roq_start;
	}

	return false;
}

cin_t *Media_RoQ_TryLoad(char *name)
{
	cin_t *cin;
	roq_info *roqfilm;
	if (strchr(name, ':'))
		return NULL;

	if ((roqfilm = roq_open(name)))
	{
		cin = Z_Malloc(sizeof(cin_t));
		cin->decodeframe = Media_Roq_DecodeFrame;
		cin->shutdown = Media_Roq_Shutdown;

		cin->roq.roqfilm = roqfilm;
		cin->nextframetime = Sys_DoubleTime();

		cin->framedata = BZ_Malloc(roqfilm->width*roqfilm->height*4);
		return cin;
	}
	return NULL;
}
#endif

//Quake3 RoQ Support
//////////////////////////////////////////////////////////////////////////////////
//Static Image Support

#ifndef MINIMAL
void Media_Static_Shutdown(struct cin_s *cin)
{
	BZ_Free(cin->image.filmimage);
	cin->image.filmimage = NULL;
}

qboolean Media_Static_DecodeFrame(cin_t *cin, qboolean nosound)
{
	cin->outunchanged = cin->outtype==TF_RGBA32?true:false;//handy
	cin->outtype = TF_RGBA32;
	cin->outwidth = cin->image.imagewidth;
	cin->outheight = cin->image.imageheight;
	cin->outdata = cin->image.filmimage;
	return true;
}

cin_t *Media_Static_TryLoad(char *name)
{
	cin_t *cin;
	char *dot = strrchr(name, '.');

	if (dot && (!strcmp(dot, ".pcx") || !strcmp(dot, ".tga") || !strcmp(dot, ".png") || !strcmp(dot, ".jpg")))
	{
		qbyte *staticfilmimage;
		int imagewidth;
		int imageheight;
		qboolean hasalpha;

		int fsize;
		char fullname[MAX_QPATH];
		qbyte *file;

		Q_snprintfz(fullname, sizeof(fullname), "%s", name);
		fsize = FS_LoadFile(fullname, (void **)&file);
		if (!file)
		{
			Q_snprintfz(fullname, sizeof(fullname), "pics/%s", name);
			fsize = FS_LoadFile(fullname, (void **)&file);
			if (!file)
				return NULL;
		}

		if ((staticfilmimage = ReadPCXFile(file, fsize, &imagewidth, &imageheight)) ||	//convert to 32 rgba if not corrupt
			(staticfilmimage = ReadTargaFile(file, fsize, &imagewidth, &imageheight, &hasalpha, false)) ||
#ifdef AVAIL_JPEGLIB
			(staticfilmimage = ReadJPEGFile(file, fsize, &imagewidth, &imageheight)) ||
#endif
#ifdef AVAIL_PNGLIB
			(staticfilmimage = ReadPNGFile(file, fsize, &imagewidth, &imageheight, fullname)) ||
#endif
			0)
		{
			FS_FreeFile(file);	//got image data
		}
		else
		{
			FS_FreeFile(file);	//got image data
			Con_Printf("Static cinematic format not supported.\n");	//not supported format
			return NULL;
		}

		cin = Z_Malloc(sizeof(cin_t));
		cin->decodeframe = Media_Static_DecodeFrame;
		cin->shutdown = Media_Static_Shutdown;

		cin->image.filmimage = staticfilmimage;
		cin->image.imagewidth = imagewidth;
		cin->image.imageheight = imageheight;

		return cin;
	}
	return NULL;
}
#endif

//Static Image Support
//////////////////////////////////////////////////////////////////////////////////
//Quake2 CIN Support

#ifdef Q2CLIENT
void Media_Cin_Shutdown(struct cin_s *cin)
{
	CIN_StopCinematic(cin->q2cin.cin);
}

qboolean Media_Cin_DecodeFrame(cin_t *cin, qboolean nosound)
{
	cin->outunchanged = cin->outdata!=NULL;
	switch (CIN_RunCinematic(cin->q2cin.cin, &cin->outdata, &cin->outwidth, &cin->outheight, &cin->outpalette))
	{
	default:
	case 0:
		cin->ended = true;
		return cin->outdata!=NULL;
	case 1:
		cin->outunchanged = false;
		return cin->outdata!=NULL;
	case 2:
		return cin->outdata!=NULL;
	}
}

cin_t *Media_Cin_TryLoad(char *name)
{
	struct cinematics_s *q2cin;
	cin_t *cin;
	char *dot = strrchr(name, '.');

	if (dot && (!strcmp(dot, ".cin")))
	{
		q2cin = CIN_PlayCinematic(name);
		if (q2cin)
		{
			cin = Z_Malloc(sizeof(cin_t));
			cin->q2cin.cin = q2cin;
			cin->decodeframe = Media_Cin_DecodeFrame;
			cin->shutdown = Media_Cin_Shutdown;

			cin->outtype = TF_8PAL24;
			return cin;
		}
	}

	return NULL;
}
#endif

//Quake2 CIN Support
//////////////////////////////////////////////////////////////////////////////////

qboolean Media_PlayingFullScreen(void)
{
	return videoshader!=NULL;
}

void Media_ShutdownCin(cin_t *cin)
{
	if (!cin)
		return;

	if (cin->shutdown)
		cin->shutdown(cin);

	if (TEXVALID(cin->texture))
		Image_UnloadTexture(cin->texture);

	if (cin->framedata)
	{
		BZ_Free(cin->framedata);
		cin->framedata = NULL;
	}

	Z_Free(cin);
}

cin_t *Media_StartCin(char *name)
{
	cin_t *cin = NULL;

	if (!name || !*name)	//clear only.
		return NULL;

	if (!cin)
		cin = Media_Static_TryLoad(name);

#ifdef Q2CLIENT
	if (!cin)
		cin = Media_Cin_TryLoad(name);
#endif
#ifdef Q3CLIENT
	if (!cin)
		cin = Media_RoQ_TryLoad(name);
#endif
#ifdef WINAVI
	if (!cin)
		cin = Media_WinAvi_TryLoad(name);
#endif
#ifdef PLUGINS
	if (!cin)
		cin = Media_Plugin_TryLoad(name);
#endif
	if (!cin)
		Con_Printf("Unable to decode \"%s\"\n", name);

	return cin;
}

struct pendingfilms_s
{
	struct pendingfilms_s *next;
	char name[1];
} *pendingfilms;
qboolean Media_BeginNextFilm(void)
{
	cin_t *cin;
	char sname[MAX_QPATH];
	struct pendingfilms_s *p;

	if (!pendingfilms)
		return false;
	p = pendingfilms;
	pendingfilms = p->next;
	snprintf(sname, sizeof(sname), "cinematic/%s", p->name);

	if (!qrenderer)
	{
		Z_Free(p);
		return false;
	}

	videoshader = R_RegisterCustom(sname, SUF_NONE, Shader_DefaultCinematic, p->name);
	Z_Free(p);

	cin = R_ShaderGetCinematic(videoshader);
	if (cin)
	{
		cin->ended = false;
		if (cin->rewind)
			cin->rewind(cin);
		if (cin->changestream)
			cin->changestream(cin, "cmd:focus");

		return true;
	}
	else
	{
		R_UnloadShader(videoshader);
		videoshader = NULL;

		return false;
	}
}
qboolean Media_StopFilm(qboolean all)
{
	if (all)
	{
		struct pendingfilms_s *p;
		while(pendingfilms)
		{
			p = pendingfilms;
			pendingfilms = p->next;
			Z_Free(p);
		}
	}
	if (videoshader)
	{
		R_UnloadShader(videoshader);
		videoshader = NULL;

		S_RawAudio(-1, NULL, 0, 0, 0, 0, 0);
	}

	while (pendingfilms && !videoshader)
	{
		if (Media_BeginNextFilm())
			break;
	}

	//for q2 cinematic-maps.
	if (!videoshader && cls.state == ca_active)
	{
		CL_SendClientCommand(true, "nextserver %i", cl.servercount);
	}
	return true;
}
qboolean Media_PlayFilm(char *name, qboolean enqueue)
{
	if (!enqueue || !*name)
		Media_StopFilm(true);

	if (*name)
	{
		struct pendingfilms_s **p;
		for (p = &pendingfilms; *p; p = &(*p)->next)
			;
		(*p) = Z_Malloc(sizeof(**p) + strlen(name));
		strcpy((*p)->name, name);

		while (pendingfilms && !videoshader)
		{
			if (Media_BeginNextFilm())
				break;
		}
	}

	if (videoshader)
	{
		CDAudio_Stop();
		SCR_EndLoadingPlaque();

		if (Key_Dest_Has(kdm_emenu))
		{
			Key_Dest_Remove(kdm_emenu);
			m_state = m_none;
		}
		if (Key_Dest_Has(kdm_gmenu))
			Key_Dest_Remove(kdm_gmenu);	//FIXME
		if (!Key_Dest_Has(kdm_console))
			scr_con_current=0;
		return true;
	}
	else
		return false;
}
qboolean Media_ShowFilm(void)
{
	if (videoshader)
	{
		cin_t *cin = R_ShaderGetCinematic(videoshader);
		if (cin && cin->ended)
		{
			if (videoshader)
			{
				R_UnloadShader(videoshader);
				videoshader = NULL;
			}
			Media_StopFilm(false);
		}
		else if (cin)
		{
			int cw = cin->outwidth, ch = cin->outheight;
			float ratiox, ratioy, aspect;
			if (cin->cursormove)
				cin->cursormove(cin, mousecursor_x/(float)vid.width, mousecursor_y/(float)vid.height);
			if (cin->setsize)
				cin->setsize(cin, vid.pixelwidth, vid.pixelheight);

			//FIXME: should have a proper aspect ratio setting. RoQ files are always power of two, which makes things ugly.
			if (cin->getsize)
				cin->getsize(cin, &cw, &ch, &aspect);
			else
			{
				cw = 4;
				ch = 3;
			}

			ratiox = (float)cw / vid.pixelwidth;
			ratioy = (float)ch / vid.pixelheight;

			if (!ch || !cw)
			{
				R2D_ImageColours(0, 0, 0, 1);
				R2D_FillBlock(0, 0, vid.width, vid.height);
				R2D_ScalePic(0, 0, 0, 0, videoshader);
			}
			else if (ratiox > ratioy)
			{
				int h = (vid.width * ch) / cw;
				int p = vid.height - h;

				//letterbox
				R2D_ImageColours(0, 0, 0, 1);
				R2D_FillBlock(0, 0, vid.width, p/2);
				R2D_FillBlock(0, h + (p/2), vid.width, p/2);

				R2D_ImageColours(1, 1, 1, 1);
				R2D_ScalePic(0, p/2, vid.width, h, videoshader);
			}
			else
			{
				int w = (vid.height * cw) / ch;
				int p = vid.width - w;
				//sidethingies
				R2D_ImageColours(0, 0, 0, 1);
				R2D_FillBlock(0, 0, (p/2), vid.height);
				R2D_FillBlock(w + (p/2), 0, p/2, vid.height);

				R2D_ImageColours(1, 1, 1, 1);
				R2D_ScalePic(p/2, 0, w, vid.height, videoshader);
			}

			SCR_SetUpToDrawConsole();
			if  (scr_con_current)
				SCR_DrawConsole (false);
			return true;
		}
	}
	return false;
}

#if defined(GLQUAKE) || defined(D3DQUAKE)
texid_tf Media_UpdateForShader(cin_t *cin)
{
	if (!cin)
		return r_nulltex;
	if (!cin->decodeframe(cin, false))
	{
		return r_nulltex;
	}

	if (!cin->outunchanged)
	{
		if (!TEXVALID(cin->texture))
			TEXASSIGN(cin->texture, Image_CreateTexture("***cin***", NULL, IF_NOMIPMAP));
		Image_Upload(cin->texture, cin->outtype, cin->outdata, cin->outpalette, cin->outwidth, cin->outheight, IF_NOMIPMAP|IF_NOGAMMA);
	}

	if (cin->doneframe)
		cin->doneframe(cin);


	return cin->texture;
}
#endif

void Media_Send_KeyEvent(cin_t *cin, int button, int unicode, int event)
{
	if (!cin)
		cin = R_ShaderGetCinematic(videoshader);
	if (!cin || !cin->key)
		return;
	cin->key(cin, button, unicode, event);
}
void Media_Send_MouseMove(cin_t *cin, float x, float y)
{
	if (!cin)
		cin = R_ShaderGetCinematic(videoshader);
	if (!cin || !cin->cursormove)
		return;
	cin->cursormove(cin, x, y);
}
void Media_Send_Resize(cin_t *cin, int x, int y)
{
	if (!cin)
		cin = R_ShaderGetCinematic(videoshader);
	if (!cin || !cin->setsize)
		return;
	cin->setsize(cin, x, y);
}
void Media_Send_GetSize(cin_t *cin, int *x, int *y, float *aspect)
{
	*x = 0;
	*y = 0;
	*aspect = 0;
	if (!cin)
		cin = R_ShaderGetCinematic(videoshader);
	if (!cin || !cin->getsize)
		return;
	cin->getsize(cin, x, y, aspect);
}
void Media_Send_Reset(cin_t *cin)
{
	if (!cin || !cin->rewind)
		cin->rewind(cin);
}
void Media_Send_Command(cin_t *cin, const char *command)
{
	if (!cin)
		cin = R_ShaderGetCinematic(videoshader);
	if (cin && cin->changestream)
		cin->changestream(cin, command);
	else if (cin && cin->rewind && !strcmp(command, "cmd:rewind"))
		cin->rewind(cin);
}
void Media_Send_GetPositions(cin_t *cin, qboolean *active, float *curtime, float *duration)
{
	if (!cin)
		cin = R_ShaderGetCinematic(videoshader);
	if (cin)
	{
		*active = true && !cin->ended;
		*curtime = Sys_DoubleTime() - cin->filmstarttime;
		*duration = cin->filmpercentage;
	}
	else
	{
		*active = false;
		*curtime = 0;
		*duration = 0;
	}
}


void Media_PlayFilm_f (void)
{
	int i;
	if (Cmd_Argc() < 2)
	{
		Con_Printf("playfilm <filename>");
	}
	if (!strcmp(Cmd_Argv(0), "cinematic"))
		Media_PlayFilm(va("video/%s", Cmd_Argv(1)), false);
	else
	{
		Media_PlayFilm(Cmd_Argv(1), false);
		for (i = 2; i < Cmd_Argc(); i++)
			Media_PlayFilm(Cmd_Argv(i), true);
	}
}



















soundcardinfo_t *capture_fakesounddevice;

double captureframeinterval;	//interval between video frames
double capturelastvideotime;	//time of last video frame
int captureframe;
fbostate_t capturefbo;
int captureoldfbo;
qboolean capturingfbo;
texid_t	capturetexture;
qboolean captureframeforce;
#if defined(GLQUAKE) && !defined(GLESONLY)
//ring buffer
int pbo_handles[4];
enum uploadfmt pbo_format;
#define CAN_USE_PBOS
#endif
int pbo_oldest;

qboolean capturepaused;
extern cvar_t vid_conautoscale;
cvar_t capturerate = CVAR("capturerate", "30");
cvar_t capturewidth = CVARD("capturedemowidth", "0", "When using capturedemo, this specifies the width of the FBO image used.");
cvar_t captureheight = CVARD("capturedemoheight", "0", "When using capturedemo, this specifies the width of the FBO image used.");
#if 0//defined(WINAVI)
cvar_t capturedriver = CVARD("capturedriver", "avi", "The driver to use to capture the demo. avformat can be supported via a plugin.\navi: capture directly to avi (capturecodec should be a fourcc value).\nraw: capture to a series of screenshots.");
cvar_t capturecodec = CVAR("capturecodec", "divx");
#else
cvar_t capturedriver = CVARD("capturedriver", "raw", "The driver to use to capture the demo. avformat can be supported via a plugin.\nraw: capture to a series of screenshots.");
cvar_t capturecodec = CVARD("capturecodec", "tga", "the compression/encoding codec to use. With raw capturing, this should be one of tga,png,jpg,pcx (ie: screenshot extensions).\nWith (win)avi, this should be a fourcc code like divx or xvid.");
#endif
cvar_t capturesound = CVARD("capturesound", "1", "Enables the capturing of game voice. If not using capturedemo, this can be combined with cl_voip_test to capture your own voice.");
cvar_t capturesoundchannels = CVAR("capturesoundchannels", "1");
cvar_t capturesoundbits = CVAR("capturesoundbits", "8");
cvar_t capturemessage = CVAR("capturemessage", "");
qboolean recordingdemo;

media_encoder_funcs_t *currentcapture_funcs;
void *currentcapture_ctx;


#if 1
/*screenshot capture*/
struct capture_raw_ctx
{
	int frames;
	enum fs_relative fsroot;
	char videonameprefix[MAX_QPATH];
	char videonameextension[16];
	vfsfile_t *audio;
};

qboolean FS_FixPath(char *path, size_t pathsize);
static void *QDECL capture_raw_begin (char *streamname, int videorate, int width, int height, int *sndkhz, int *sndchannels, int *sndbits)
{
	struct capture_raw_ctx *ctx = Z_Malloc(sizeof(*ctx));

	if (!strcmp(capturecodec.string, "png") || !strcmp(capturecodec.string, "jpeg") || !strcmp(capturecodec.string, "jpg") || !strcmp(capturecodec.string, "bmp") || !strcmp(capturecodec.string, "pcx") || !strcmp(capturecodec.string, "tga"))
		Q_strncpyz(ctx->videonameextension, capturecodec.string, sizeof(ctx->videonameextension));
	else
		Q_strncpyz(ctx->videonameextension, "tga", sizeof(ctx->videonameextension));

	if (!FS_NativePath(va("%s", streamname), FS_GAMEONLY, ctx->videonameprefix, sizeof(ctx->videonameprefix)))
	{
		Z_Free(ctx);
		return NULL;
	}
	if (!FS_FixPath(ctx->videonameprefix, sizeof(ctx->videonameprefix)))
	{
		Z_Free(ctx);
		return NULL;
	}
	ctx->fsroot = FS_SYSTEM;
	ctx->audio = NULL;
	if (*sndkhz)
	{
		char filename[MAX_OSPATH];
		if (*sndbits < 8)
			*sndbits = 8;
		if (*sndbits != 8)
			*sndbits = 16;
		if (*sndchannels > 6)
			*sndchannels = 6;
		if (*sndchannels < 1)
			*sndchannels = 1;
		Q_snprintfz(filename, sizeof(filename), "%saudio_%ichan_%ikhz_%ib.raw", ctx->videonameprefix, *sndchannels, *sndkhz/1000, *sndbits);
		FS_CreatePath(filename, ctx->fsroot);
		ctx->audio = FS_OpenVFS(filename, "wb", ctx->fsroot);
	}
	if (!ctx->audio)
	{
		*sndkhz = 0;
		*sndchannels = 0;
		*sndbits = 0;
	}
	return ctx;
}
static void QDECL capture_raw_video (void *vctx, void *data, int frame, int width, int height, enum uploadfmt fmt)
{
	struct capture_raw_ctx *ctx = vctx;
	char filename[MAX_OSPATH];
	ctx->frames = frame+1;
	Q_snprintfz(filename, sizeof(filename), "%s%8.8i.%s", ctx->videonameprefix, frame, ctx->videonameextension);
	SCR_ScreenShot(filename, ctx->fsroot, data, width, height, fmt);
}
static void QDECL capture_raw_audio (void *vctx, void *data, int bytes)
{
	struct capture_raw_ctx *ctx = vctx;

	if (ctx->audio)
		VFS_WRITE(ctx->audio, data, bytes);
}
static void QDECL capture_raw_end (void *vctx)
{
	struct capture_raw_ctx *ctx = vctx;
	if (ctx->audio)
		VFS_CLOSE(ctx->audio);
	Con_Printf("%d video frames captured\n", ctx->frames);
	Z_Free(ctx);
}
static media_encoder_funcs_t capture_raw =
{
	"raw",
	capture_raw_begin,
	capture_raw_video,
	capture_raw_audio,
	capture_raw_end
};
#endif
#if defined(WINAVI)

/*screenshot capture*/
struct capture_avi_ctx
{
	PAVIFILE file;
	#define avi_video_stream(ctx) (ctx->codec_fourcc?ctx->compressed_video_stream:ctx->uncompressed_video_stream)
	PAVISTREAM uncompressed_video_stream;
	PAVISTREAM compressed_video_stream;
	PAVISTREAM uncompressed_audio_stream;
	WAVEFORMATEX wave_format;
	unsigned long codec_fourcc;

	int audio_frame_counter;
};

static void QDECL capture_avi_end(void *vctx)
{
	struct capture_avi_ctx *ctx = vctx;

    if (ctx->uncompressed_video_stream)	qAVIStreamRelease(ctx->uncompressed_video_stream);
    if (ctx->compressed_video_stream)	qAVIStreamRelease(ctx->compressed_video_stream);
    if (ctx->uncompressed_audio_stream)	qAVIStreamRelease(ctx->uncompressed_audio_stream);
    if (ctx->file)						qAVIFileRelease(ctx->file);
	Z_Free(ctx);
}

static void *QDECL capture_avi_begin (char *streamname, int videorate, int width, int height, int *sndkhz, int *sndchannels, int *sndbits)
{
	struct capture_avi_ctx *ctx = Z_Malloc(sizeof(*ctx));
	HRESULT hr;
	BITMAPINFOHEADER bitmap_info_header;
	AVISTREAMINFOA stream_header;
	FILE *f;
	char aviname[256];
	char nativepath[256];

	char *fourcc = capturecodec.string;

	if (strlen(fourcc) == 4)
		ctx->codec_fourcc = mmioFOURCC(*(fourcc+0), *(fourcc+1), *(fourcc+2), *(fourcc+3));
	else
		ctx->codec_fourcc = 0;

	if (!qAVIStartup())
	{
		Con_Printf("vfw support not available.\n");
		capture_avi_end(ctx);
		return NULL;
	}

	/*convert to foo.avi*/
	COM_StripExtension(streamname, aviname, sizeof(aviname));
	COM_DefaultExtension (aviname, ".avi", sizeof(aviname));
	/*find the system location of that*/
	FS_NativePath(aviname, FS_GAMEONLY, nativepath, sizeof(nativepath));

	//wipe it.
	f = fopen(nativepath, "rb");
	if (f)
	{
		fclose(f);
		unlink(nativepath);
	}

	hr = qAVIFileOpenA(&ctx->file, nativepath, OF_WRITE | OF_CREATE, NULL);
	if (FAILED(hr))
	{
		Con_Printf("Failed to open %s\n", nativepath);
		capture_avi_end(ctx);
		return NULL;
	}


	memset(&bitmap_info_header, 0, sizeof(BITMAPINFOHEADER));
	bitmap_info_header.biSize = 40;
	bitmap_info_header.biWidth = width;
	bitmap_info_header.biHeight = height;
	bitmap_info_header.biPlanes = 1;
	bitmap_info_header.biBitCount = 24;
	bitmap_info_header.biCompression = BI_RGB;
	bitmap_info_header.biSizeImage = width*height * 3;


	memset(&stream_header, 0, sizeof(stream_header));
	stream_header.fccType = streamtypeVIDEO;
	stream_header.fccHandler = ctx->codec_fourcc;
	stream_header.dwScale = 100;
	stream_header.dwRate = (unsigned long)(0.5 + 100.0/captureframeinterval);
	SetRect(&stream_header.rcFrame, 0, 0, width, height);

	hr = qAVIFileCreateStreamA(ctx->file, &ctx->uncompressed_video_stream, &stream_header);
	if (FAILED(hr))
	{
		Con_Printf("Couldn't initialise the stream, check codec\n");
		capture_avi_end(ctx);
		return NULL;
	}

	if (ctx->codec_fourcc)
	{
		AVICOMPRESSOPTIONS opts;
		memset(&opts, 0, sizeof(opts));
		opts.fccType = stream_header.fccType;
		opts.fccHandler = ctx->codec_fourcc;
		// Make the stream according to compression
		hr = qAVIMakeCompressedStream(&ctx->compressed_video_stream, ctx->uncompressed_video_stream, &opts, NULL);
		if (FAILED(hr))
		{
			Con_Printf("AVIMakeCompressedStream failed. check video codec.\n");
			capture_avi_end(ctx);
			return NULL;
		}
	}


	hr = qAVIStreamSetFormat(avi_video_stream(ctx), 0, &bitmap_info_header, sizeof(BITMAPINFOHEADER));
	if (FAILED(hr))
	{
		Con_Printf("AVIStreamSetFormat failed\n");
		capture_avi_end(ctx);
		return NULL;
	}

	if (*sndbits != 8 && *sndbits != 16)
		*sndbits = 8;
	if (*sndchannels < 1 && *sndchannels > 6)
		*sndchannels = 1;

	if (*sndkhz)
	{
		memset(&ctx->wave_format, 0, sizeof(WAVEFORMATEX));
		ctx->wave_format.wFormatTag = WAVE_FORMAT_PCM;
		ctx->wave_format.nChannels = *sndchannels;
		ctx->wave_format.nSamplesPerSec = *sndkhz;
		ctx->wave_format.wBitsPerSample = *sndbits;
		ctx->wave_format.nBlockAlign = ctx->wave_format.wBitsPerSample/8 * ctx->wave_format.nChannels;
		ctx->wave_format.nAvgBytesPerSec = ctx->wave_format.nSamplesPerSec * ctx->wave_format.nBlockAlign;
		ctx->wave_format.cbSize = 0;


		memset(&stream_header, 0, sizeof(stream_header));
		stream_header.fccType = streamtypeAUDIO;
		stream_header.dwScale = ctx->wave_format.nBlockAlign;
		stream_header.dwRate = stream_header.dwScale * (unsigned long)ctx->wave_format.nSamplesPerSec;
		stream_header.dwSampleSize = ctx->wave_format.nBlockAlign;

		//FIXME: be prepared to capture audio to mp3.

		hr = qAVIFileCreateStreamA(ctx->file, &ctx->uncompressed_audio_stream, &stream_header);
		if (FAILED(hr))
		{
			capture_avi_end(ctx);
			return NULL;
		}

		hr = qAVIStreamSetFormat(ctx->uncompressed_audio_stream, 0, &ctx->wave_format, sizeof(WAVEFORMATEX));
		if (FAILED(hr))
		{
			capture_avi_end(ctx);
			return NULL;
		}
	}
	return ctx;
}

static void QDECL capture_avi_video(void *vctx, void *vdata, int frame, int width, int height, enum uploadfmt fmt)
{
	struct capture_avi_ctx *ctx = vctx;
	qbyte *data = vdata;
	int c, i;
	qbyte temp;

	if (fmt == TF_BGRA32)
	{
		// truncate bgra to bgr
		c = width*height;
		for (i=0 ; i<c ; i++)
		{
			data[i*3+0] = data[i*4+0];
			data[i*3+1] = data[i*4+1];
			data[i*3+2] = data[i*4+2];
		}
	}
	else if (fmt == TF_RGB24)
	{
		// swap rgb to bgr
		c = width*height*3;
		for (i=0 ; i<c ; i+=3)
		{
			temp = data[i];
			data[i] = data[i+2];
			data[i+2] = temp;
		}
	}
	else if (fmt != TF_BGR24)
	{
		Con_Printf("Unsupported image format\n");
		return;
	}
	//write it
	if (FAILED(qAVIStreamWrite(avi_video_stream(ctx), frame, 1, data, width*height * 3, ((frame%15) == 0)?AVIIF_KEYFRAME:0, NULL, NULL)))
		Con_DPrintf("Recoring error\n");
}

static void QDECL capture_avi_audio(void *vctx, void *data, int bytes)
{
	struct capture_avi_ctx *ctx = vctx;
	if (ctx->uncompressed_audio_stream)
		qAVIStreamWrite(ctx->uncompressed_audio_stream, ctx->audio_frame_counter++, 1, data, bytes, AVIIF_KEYFRAME, NULL, NULL);
}

static media_encoder_funcs_t capture_avi =
{
	"avi",
	capture_avi_begin,
	capture_avi_video,
	capture_avi_audio,
	capture_avi_end
};
#endif

#ifdef _DEBUG
static void QDECL capture_null_end(void *vctx)
{
}
static void *QDECL capture_null_begin (char *streamname, int videorate, int width, int height, int *sndkhz, int *sndchannels, int *sndbits)
{
	return (void*)~0;
}
static void QDECL capture_null_video(void *vctx, void *vdata, int frame, int width, int height, enum uploadfmt fmt)
{
}
static void QDECL capture_null_audio(void *vctx, void *data, int bytes)
{
}
static media_encoder_funcs_t capture_null =
{
	"null",
	capture_null_begin,
	capture_null_video,
	capture_null_audio,
	capture_null_end
};
#endif

static media_encoder_funcs_t *pluginencodersfunc[8];
static struct plugin_s *pluginencodersplugin[8];
qboolean Media_RegisterEncoder(struct plugin_s *plug, media_encoder_funcs_t *funcs)
{
	int i;
	for (i = 0; i < sizeof(pluginencodersfunc)/sizeof(pluginencodersfunc[0]); i++)
	{
		if (pluginencodersfunc[i] == NULL)
		{
			pluginencodersfunc[i] = funcs;
			pluginencodersplugin[i] = plug;
			return true;
		}
	}
	return false;
}
void Media_StopRecordFilm_f(void);
/*funcs==null closes ALL decoders from this plugin*/
qboolean Media_UnregisterEncoder(struct plugin_s *plug, media_encoder_funcs_t *funcs)
{
	qboolean success = false;
	int i;

	for (i = 0; i < sizeof(pluginencodersfunc)/sizeof(pluginencodersfunc[0]); i++)
	{
		if (pluginencodersplugin[i])
		if (pluginencodersfunc[i] == funcs || (!funcs && pluginencodersplugin[i] == plug))
		{
			if (currentcapture_funcs == pluginencodersfunc[i])
				Media_StopRecordFilm_f();
			success = true;
			pluginencodersfunc[i] = NULL;
			pluginencodersplugin[i] = NULL;
			if (funcs)
				return success;
		}
	}
	return success;
}
 
//returns 0 if not capturing. 1 if capturing live. 2 if capturing a demo (where frame timings are forced).
int Media_Capturing (void)
{
	if (!currentcapture_funcs)
		return 0;
	return captureframeforce?2:1;
}

void Media_CapturePause_f (void)
{
	capturepaused = !capturepaused;
}

qboolean Media_PausedDemo (qboolean fortiming)
{
	//if fortiming is set, then timing might need to advance if we still need to parse the demo to get the first valid data out of it.

	if (capturepaused)
		return true;

	//capturedemo doesn't record any frames when the console is visible
	//but that's okay, as we don't load any demo frames either.
	if ((cls.demoplayback && Media_Capturing()))
		if (Key_Dest_Has(~kdm_game) || scr_con_current > 0 || (!fortiming&&!cl.validsequence))
			return true;

	return false;
}

static qboolean Media_ForceTimeInterval(void)
{
	return (cls.demoplayback && Media_Capturing() && captureframeinterval>0);
}

double Media_TweekCaptureFrameTime(double oldtime, double time)
{
	if (Media_ForceTimeInterval())
	{
		captureframeforce = true;
		//if we're forcing time intervals, then we use fixed time increments and generate a new video frame for every single frame.
		return capturelastvideotime;
	}
	return oldtime + time;
}

void Media_RecordFrame (void)
{
	char *buffer;
	int truewidth, trueheight;
	enum uploadfmt fmt;

	if (!currentcapture_funcs)
		return;

/*	if (*capturecutoff.string && captureframe * captureframeinterval > capturecutoff.value*60)
	{
		currentcapture_funcs->capture_end(currentcapture_ctx);
		currentcapture_ctx = currentcapture_funcs->capture_begin(Cmd_Argv(1), capturerate.value, vid.pixelwidth, vid.pixelheight, &sndkhz, &sndchannels, &sndbits);
		if (!currentcapture_ctx)
		{
			currentcapture_funcs = NULL;
			return;
		}
		captureframe = 0;
	}
*/
	if (Media_PausedDemo(false))
	{
		int y = vid.height -32-16;
		if (y < scr_con_current) y = scr_con_current;
		if (y > vid.height-8)
			y = vid.height-8;
		Draw_FunString((strlen(capturemessage.string)+1)*8, y, S_COLOR_RED "PAUSED");

		if (captureframeforce)
			capturelastvideotime += captureframeinterval;
		return;
	}

	//don't capture frames while we're loading.
	if (cl.sendprespawn || (cls.state < ca_active && COM_HasWork()))
	{
		capturelastvideotime += captureframeinterval;
		return;
	}

//overlay this on the screen, so it appears in the film
	if (*capturemessage.string)
	{
		int y = vid.height -32-16;
		if (y < scr_con_current) y = scr_con_current;
		if (y > vid.height-8)
			y = vid.height-8;
		Draw_FunString(0, y, capturemessage.string);
	}

	//time for annother frame?
	if (!captureframeforce)
	{
		if (capturelastvideotime > realtime+1)
			capturelastvideotime = realtime;	//urm, wrapped?..
		if (capturelastvideotime > realtime)
			goto skipframe;
	}
	
	if (cls.findtrack)
	{
		capturelastvideotime += captureframeinterval;
		return;	//skip until we're tracking the right player.
	}

	if (R2D_Flush)
		R2D_Flush();

#ifdef CAN_USE_PBOS
	if (pbo_format != TF_INVALID)
	{
		int imagesize = vid.fbpwidth * vid.fbpheight * 4;
		while (pbo_oldest + countof(pbo_handles) <= captureframe)
		{
			qglBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pbo_handles[pbo_oldest%countof(pbo_handles)]);
			buffer = qglMapBufferARB(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB);
			if (buffer)
			{
				currentcapture_funcs->capture_video(currentcapture_ctx, buffer, pbo_oldest, vid.fbpwidth, vid.fbpheight, pbo_format);
				qglUnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);
			}
			pbo_oldest++;
		}

		if (!pbo_handles[captureframe%countof(pbo_handles)])
		{
			qglGenBuffersARB(1, &pbo_handles[captureframe%countof(pbo_handles)]);
			qglBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pbo_handles[captureframe%countof(pbo_handles)]);
			qglBufferDataARB(GL_PIXEL_PACK_BUFFER_ARB, imagesize, NULL, GL_STATIC_READ_ARB);
		}
		qglBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pbo_handles[captureframe%countof(pbo_handles)]);
		switch(pbo_format)
		{
		case TF_BGR24:
			qglReadPixels(0, 0, vid.fbpwidth, vid.fbpheight, GL_BGR_EXT, GL_UNSIGNED_BYTE, 0);
			break;
		case TF_BGRA32:
			qglReadPixels(0, 0, vid.fbpwidth, vid.fbpheight, GL_BGRA_EXT, GL_UNSIGNED_INT_8_8_8_8_REV, 0);
			break;
		case TF_RGB24:
			qglReadPixels(0, 0, vid.fbpwidth, vid.fbpheight, GL_RGB, GL_UNSIGNED_BYTE, 0);
			break;
		case TF_RGBA32:
			qglReadPixels(0, 0, vid.fbpwidth, vid.fbpheight, GL_RGBA, GL_UNSIGNED_BYTE, 0);
			break;
		}
		qglBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, 0);
	}
	else
#endif
	{
		pbo_oldest = captureframe+1;
		//submit the current video frame. audio will be mixed to match.
		buffer = VID_GetRGBInfo(&truewidth, &trueheight, &fmt);
		if (buffer)
		{
			currentcapture_funcs->capture_video(currentcapture_ctx, buffer, captureframe, truewidth, trueheight, fmt);
			BZ_Free (buffer);
		}
		else
		{
			Con_DPrintf("Unable to grab video image\n");
			currentcapture_funcs->capture_video(currentcapture_ctx, NULL, captureframe, 0, 0, TF_INVALID);
		}
	}
	captureframe++;
	capturelastvideotime += captureframeinterval;

	captureframeforce = false;

	//this is drawn to the screen and not the film
skipframe:
{
	int y = vid.height -32-16;
	if (y < scr_con_current) y = scr_con_current;
	if (y > vid.height-8)
		y = vid.height-8;

#ifdef GLQUAKE
	if (capturingfbo)
	{
		shader_t *pic;
		GLBE_FBO_Pop(captureoldfbo);
		vid.framebuffer = NULL;
		GL_Set2D(false);

		pic = R_RegisterShader("capturdemofeedback", SUF_NONE,
					"{\n"
						"program default2d\n"
						"{\n"
							"map $diffuse\n"
						"}\n"
					"}\n");
		pic->defaulttextures->base = capturetexture;
		//pulse green slightly, so its a bit more obvious
		R2D_ImageColours(1, 1+0.2*sin(realtime), 1, 1);
		R2D_Image(0, 0, vid.width, vid.height, 0, 1, 1, 0, pic);
		R2D_ImageColours(1, 1, 1, 1);
		Draw_FunString(0, 0, S_COLOR_RED"RECORDING");
		if (R2D_Flush)
			R2D_Flush();

		captureoldfbo = GLBE_FBO_Push(&capturefbo);
		vid.framebuffer = capturetexture;
		GL_Set2D(false);
	}
	else
#endif
		Draw_FunString((strlen(capturemessage.string)+1)*8, y, S_COLOR_RED"RECORDING");
}
}

static void MSD_SetUnderWater(soundcardinfo_t *sc, qboolean underwater)
{
}

static void *MSD_Lock (soundcardinfo_t *sc, unsigned int *sampidx)
{
	return sc->sn.buffer;
}
static void MSD_Unlock (soundcardinfo_t *sc, void *buffer)
{
}

static unsigned int MSD_GetDMAPos(soundcardinfo_t *sc)
{
	int		s;

	s = captureframe*(snd_speed*captureframeinterval);


//	s >>= (sc->sn.samplebits/8) - 1;
	s *= sc->sn.numchannels;
	return s;
}

static void MSD_Submit(soundcardinfo_t *sc, int start, int end)
{
	//Fixme: support outputting to wav
	//http://www.borg.com/~jglatt/tech/wave.htm


	int lastpos;
	int newpos;
	int samplestosubmit;
	int offset;
	int bytespersample;

	newpos = sc->paintedtime;

	while(1)
	{
		lastpos = sc->snd_completed;
		samplestosubmit = newpos - lastpos;
		if (samplestosubmit < (snd_speed*captureframeinterval))
			return;
		if (samplestosubmit < 1152)
			return;
		if (samplestosubmit > 1152)
			samplestosubmit = 1152;

		bytespersample = sc->sn.numchannels*sc->sn.samplebits/8;

		offset = (lastpos % (sc->sn.samples/sc->sn.numchannels));

		//we could just use a buffer size equal to the number of samples in each frame
		//but that isn't as robust when it comes to floating point imprecisions
		//namly: that it would loose a sample each frame with most framerates.

		if ((sc->snd_completed % (sc->sn.samples/sc->sn.numchannels)) < offset)
		{
			int partialsamplestosubmit;
			//wraped, two chunks to send
			partialsamplestosubmit = ((sc->sn.samples/sc->sn.numchannels)) - offset;
			currentcapture_funcs->capture_audio(currentcapture_ctx, sc->sn.buffer+offset*bytespersample, partialsamplestosubmit*bytespersample);
			samplestosubmit -= partialsamplestosubmit;
			sc->snd_completed += partialsamplestosubmit;
			offset = 0;
		}
		currentcapture_funcs->capture_audio(currentcapture_ctx, sc->sn.buffer+offset*bytespersample, samplestosubmit*bytespersample);
		sc->snd_completed += samplestosubmit;
	}
}

static void MSD_Shutdown (soundcardinfo_t *sc)
{
	Z_Free(sc->sn.buffer);
	capture_fakesounddevice = NULL;
}

void Media_InitFakeSoundDevice (int speed, int channels, int samplebits)
{
	soundcardinfo_t *sc;

	if (capture_fakesounddevice)
		return;

	//when we're recording a demo, we'll be timedemoing it as it were.
	//this means that the actual sound devices and the fake device will be going at different rates
	//which really confuses any stream decoding, like music.
	//so just kill all actual sound devices.
	if (recordingdemo)
	{
		soundcardinfo_t *next;
		for (sc = sndcardinfo; sc; sc=next)
		{
			next = sc->next;
			sc->Shutdown(sc);
			Z_Free(sc);
			sndcardinfo = next;
		}
	}

	if (!snd_speed)
		snd_speed = speed;

	sc = Z_Malloc(sizeof(soundcardinfo_t));

	sc->snd_sent = 0;
	sc->snd_completed = 0;

	sc->sn.samples = speed*0.5;
	sc->sn.speed = speed;
	sc->sn.samplebits = samplebits;
	sc->sn.samplepos = 0;
	sc->sn.numchannels = channels;
	sc->inactive_sound = true;

	sc->sn.samples -= sc->sn.samples%1152;

	sc->sn.buffer = (unsigned char *) BZ_Malloc(sc->sn.samples*sc->sn.numchannels*(sc->sn.samplebits/8));


	sc->Lock		= MSD_Lock;
	sc->Unlock		= MSD_Unlock;
	sc->SetWaterDistortion = MSD_SetUnderWater;
	sc->Submit		= MSD_Submit;
	sc->Shutdown	= MSD_Shutdown;
	sc->GetDMAPos	= MSD_GetDMAPos;

	sc->next = sndcardinfo;
	sndcardinfo = sc;

	capture_fakesounddevice = sc;

	S_DefaultSpeakerConfiguration(sc);
}



void Media_StopRecordFilm_f (void)
{
#ifdef CAN_USE_PBOS
	if (pbo_format)
	{
		int i;
		int imagesize = vid.fbpwidth * vid.fbpheight * 4;
		while (pbo_oldest < captureframe)
		{
			qbyte *buffer;
			qglBindBufferARB(GL_PIXEL_PACK_BUFFER_ARB, pbo_handles[pbo_oldest%countof(pbo_handles)]);
			buffer = qglMapBufferARB(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY_ARB);
			if (buffer)
			{
				currentcapture_funcs->capture_video(currentcapture_ctx, buffer, pbo_oldest, vid.fbpwidth, vid.fbpheight, TF_BGR24);
//				currentcapture_funcs->capture_video(currentcapture_ctx, buffer, pbo_oldest, vid.fbpwidth, vid.fbpheight, TF_BGRA32);
				qglUnmapBufferARB(GL_PIXEL_PACK_BUFFER_ARB);
			}
			pbo_oldest++;
		}

		for (i = 0; i < countof(pbo_handles); i++)
		{
			if (pbo_handles[i])
				qglDeleteBuffersARB(1, &pbo_handles[i]);
			pbo_handles[i] = 0;
		}
	}
#endif


	if (capture_fakesounddevice)
		S_ShutdownCard(capture_fakesounddevice);
	capture_fakesounddevice = NULL;

	if (recordingdemo)	//start up their regular audio devices again.
		Cmd_ExecuteString("snd_restart", RESTRICT_LOCAL);

	recordingdemo=false;

	if (currentcapture_funcs)
		currentcapture_funcs->capture_end(currentcapture_ctx);
	currentcapture_ctx = NULL;
	currentcapture_funcs = NULL;

#ifdef GLQUAKE
	if (capturingfbo)
		GLBE_FBO_Pop(captureoldfbo);
#endif
	vid.framebuffer = NULL;
	capturingfbo = false;

	Cvar_ForceCallback(&vid_conautoscale);
}
static void Media_RecordFilm (char *recordingname, qboolean demo)
{
	int sndkhz, sndchannels, sndbits;
	int i;

	Media_StopRecordFilm_f();

	if (capturerate.value<=0)
	{
		Con_Printf("Invalid capturerate\n");
		capturerate.value = 15;
	}

	captureframeinterval = 1/capturerate.value;
	if (captureframeinterval < 0.001)
		captureframeinterval = 0.001;	//no more than 1000 images per second.
	capturelastvideotime = realtime = 0;

	Con_ClearNotify();

	captureframe = pbo_oldest = 0;
	for (i = 0; i < sizeof(pluginencodersfunc)/sizeof(pluginencodersfunc[0]); i++)
	{
		if (pluginencodersfunc[i])
			if (!strcmp(pluginencodersfunc[i]->drivername, capturedriver.string))
				currentcapture_funcs = pluginencodersfunc[i];
	}
	//just use the first
	if (!currentcapture_funcs)
	{
		for (i = 0; i < sizeof(pluginencodersfunc)/sizeof(pluginencodersfunc[0]); i++)
		{
			if (pluginencodersfunc[i])
			{
				currentcapture_funcs = pluginencodersfunc[i];
				break;
			}
		}
	}
	if (capturesound.ival)
	{
		sndkhz = snd_speed?snd_speed:48000;
		sndchannels = capturesoundchannels.ival;
		sndbits = capturesoundbits.ival;
	}
	else
	{
		sndkhz = 0;
		sndchannels = 0;
		sndbits = 0;
	}

#ifdef GLQUAKE
	if (demo && capturewidth.ival && captureheight.ival && qrenderer == QR_OPENGL && gl_config.ext_framebuffer_objects)
	{
		capturingfbo = true;
		capturetexture = R2D_RT_Configure("$democapture", capturewidth.ival, captureheight.ival, TF_BGRA32);
		captureoldfbo = GLBE_FBO_Update(&capturefbo, FBO_RB_DEPTH|(Sh_StencilShadowsActive()?FBO_RB_STENCIL:0), &capturetexture, 1, r_nulltex, capturewidth.ival, captureheight.ival, 0);
		vid.fbpwidth = capturewidth.ival;
		vid.fbpheight = captureheight.ival;
		vid.framebuffer = capturetexture;
	}
#endif

#ifdef CAN_USE_PBOS
	pbo_format = TF_INVALID;
	if (qrenderer == QR_OPENGL && !gl_config.gles && gl_config.glversion >= 2.1)
	{	//both tgas and vfw favour bgr24, so lets get the gl drivers to suffer instead of us.
		if (vid.fbpwidth & 3)
			pbo_format = TF_BGRA32;	//don't bother changing pack alignment, just use something that is guarenteed to not need anything.
		else
			pbo_format = TF_BGR24;
	}
#endif

	recordingdemo = demo;
	
	if (!currentcapture_funcs->capture_begin)
		currentcapture_ctx = NULL;
	else
		currentcapture_ctx = currentcapture_funcs->capture_begin(recordingname, capturerate.value, vid.fbpwidth, vid.fbpheight, &sndkhz, &sndchannels, &sndbits);
	if (!currentcapture_ctx)
	{
		recordingdemo = false;
		currentcapture_funcs = NULL;
		Con_Printf("Unable to initialise capture driver\n");
	}
	else if (sndkhz)
		Media_InitFakeSoundDevice(sndkhz, sndchannels, sndbits);

	Cvar_ForceCallback(&vid_conautoscale);
}
static void Media_RecordFilm_f (void)
{
	if (Cmd_Argc() != 2)
	{
		Con_Printf("capture <filename>\nRecords video output in an avi file.\nUse capturerate and capturecodec to configure.\n");
		return;
	}
	if (Cmd_IsInsecure())	//err... don't think so sonny.
		return;

	Media_RecordFilm(Cmd_Argv(1), false);
}
void Media_CaptureDemoEnd(void)
{
	if (recordingdemo)
		Media_StopRecordFilm_f();
}
void CL_PlayDemo(char *demoname, qboolean usesystempath);
void Media_RecordDemo_f(void)
{
	if (Cmd_Argc() < 2)
		return;
	if (Cmd_FromGamecode())
		return;

	if (!Renderer_Started() && !isDedicated)
	{
		Cbuf_AddText(va("wait;%s %s\n", Cmd_Argv(0), Cmd_Args()), Cmd_ExecLevel);
		return;
	}

	CL_Stopdemo_f();	//capturing failed for some reason

	CL_PlayDemo(Cmd_Argv(1), false);
	if (!cls.demoplayback)
	{
		Con_Printf("unable to play demo, not capturing\n");
		return;
	}
	//FIXME: make sure it loaded okay
	Media_RecordFilm(Cmd_Argv(Cmd_Argc()>2?2:1), true);
	scr_con_current=0;
	Key_Dest_Remove(kdm_console|kdm_emenu|kdm_gmenu);

	if (!currentcapture_funcs)
		CL_Stopdemo_f();	//capturing failed for some reason
}

#if defined(_WIN32) && !defined(WINRT)
typedef struct ISpNotifySink ISpNotifySink;
typedef void *ISpNotifyCallback;
typedef void __stdcall SPNOTIFYCALLBACK(WPARAM wParam, LPARAM lParam);
typedef struct SPEVENT
{
    WORD        eEventId : 16;
    WORD  elParamType : 16;
    ULONG       ulStreamNum;
    ULONGLONG   ullAudioStreamOffset;
    WPARAM      wParam;
    LPARAM      lParam;
} SPEVENT;

#define SPEVENTSOURCEINFO void
#define ISpObjectToken void
#define ISpStreamFormat void
#define SPVOICESTATUS void
#define SPVPRIORITY int
#define SPEVENTENUM int

typedef struct ISpVoice ISpVoice;
typedef struct ISpVoiceVtbl
{
    HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
        ISpVoice * This,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void **ppvObject);

    ULONG ( STDMETHODCALLTYPE *AddRef )(
        ISpVoice * This);

    ULONG ( STDMETHODCALLTYPE *Release )(
        ISpVoice * This);

    HRESULT ( STDMETHODCALLTYPE *SetNotifySink )(
        ISpVoice * This,
        /* [in] */ ISpNotifySink *pNotifySink);

    /* [local] */ HRESULT ( STDMETHODCALLTYPE *SetNotifyWindowMessage )(
        ISpVoice * This,
        /* [in] */ HWND hWnd,
        /* [in] */ UINT Msg,
        /* [in] */ WPARAM wParam,
        /* [in] */ LPARAM lParam);

    /* [local] */ HRESULT ( STDMETHODCALLTYPE *SetNotifyCallbackFunction )(
        ISpVoice * This,
        /* [in] */ SPNOTIFYCALLBACK *pfnCallback,
        /* [in] */ WPARAM wParam,
        /* [in] */ LPARAM lParam);

    /* [local] */ HRESULT ( STDMETHODCALLTYPE *SetNotifyCallbackInterface )(
        ISpVoice * This,
        /* [in] */ ISpNotifyCallback *pSpCallback,
        /* [in] */ WPARAM wParam,
        /* [in] */ LPARAM lParam);

    /* [local] */ HRESULT ( STDMETHODCALLTYPE *SetNotifyWin32Event )(
        ISpVoice * This);

    /* [local] */ HRESULT ( STDMETHODCALLTYPE *WaitForNotifyEvent )(
        ISpVoice * This,
        /* [in] */ DWORD dwMilliseconds);

    /* [local] */ HANDLE ( STDMETHODCALLTYPE *GetNotifyEventHandle )(
        ISpVoice * This);

    HRESULT ( STDMETHODCALLTYPE *SetInterest )(
        ISpVoice * This,
        /* [in] */ ULONGLONG ullEventInterest,
        /* [in] */ ULONGLONG ullQueuedInterest);

    HRESULT ( STDMETHODCALLTYPE *GetEvents )(
        ISpVoice * This,
        /* [in] */ ULONG ulCount,
        /* [size_is][out] */ SPEVENT *pEventArray,
        /* [out] */ ULONG *pulFetched);

    HRESULT ( STDMETHODCALLTYPE *GetInfo )(
        ISpVoice * This,
        /* [out] */ SPEVENTSOURCEINFO *pInfo);

    HRESULT ( STDMETHODCALLTYPE *SetOutput )(
        ISpVoice * This,
        /* [in] */ IUnknown *pUnkOutput,
        /* [in] */ BOOL fAllowFormatChanges);

    HRESULT ( STDMETHODCALLTYPE *GetOutputObjectToken )(
        ISpVoice * This,
        /* [out] */ ISpObjectToken **ppObjectToken);

    HRESULT ( STDMETHODCALLTYPE *GetOutputStream )(
        ISpVoice * This,
        /* [out] */ ISpStreamFormat **ppStream);

    HRESULT ( STDMETHODCALLTYPE *Pause )(
        ISpVoice * This);

    HRESULT ( STDMETHODCALLTYPE *Resume )(
        ISpVoice * This);

    HRESULT ( STDMETHODCALLTYPE *SetVoice )(
        ISpVoice * This,
        /* [in] */ ISpObjectToken *pToken);

    HRESULT ( STDMETHODCALLTYPE *GetVoice )(
        ISpVoice * This,
        /* [out] */ ISpObjectToken **ppToken);

    HRESULT ( STDMETHODCALLTYPE *Speak )(
        ISpVoice * This,
        /* [string][in] */ const WCHAR *pwcs,
        /* [in] */ DWORD dwFlags,
        /* [out] */ ULONG *pulStreamNumber);

    HRESULT ( STDMETHODCALLTYPE *SpeakStream )(
        ISpVoice * This,
        /* [in] */ IStream *pStream,
        /* [in] */ DWORD dwFlags,
        /* [out] */ ULONG *pulStreamNumber);

    HRESULT ( STDMETHODCALLTYPE *GetStatus )(
        ISpVoice * This,
        /* [out] */ SPVOICESTATUS *pStatus,
        /* [string][out] */ WCHAR **ppszLastBookmark);

    HRESULT ( STDMETHODCALLTYPE *Skip )(
        ISpVoice * This,
        /* [string][in] */ WCHAR *pItemType,
        /* [in] */ long lNumItems,
        /* [out] */ ULONG *pulNumSkipped);

    HRESULT ( STDMETHODCALLTYPE *SetPriority )(
        ISpVoice * This,
        /* [in] */ SPVPRIORITY ePriority);

    HRESULT ( STDMETHODCALLTYPE *GetPriority )(
        ISpVoice * This,
        /* [out] */ SPVPRIORITY *pePriority);

    HRESULT ( STDMETHODCALLTYPE *SetAlertBoundary )(
        ISpVoice * This,
        /* [in] */ SPEVENTENUM eBoundary);

    HRESULT ( STDMETHODCALLTYPE *GetAlertBoundary )(
        ISpVoice * This,
        /* [out] */ SPEVENTENUM *peBoundary);

    HRESULT ( STDMETHODCALLTYPE *SetRate )(
        ISpVoice * This,
        /* [in] */ long RateAdjust);

    HRESULT ( STDMETHODCALLTYPE *GetRate )(
        ISpVoice * This,
        /* [out] */ long *pRateAdjust);

    HRESULT ( STDMETHODCALLTYPE *SetVolume )(
        ISpVoice * This,
        /* [in] */ USHORT usVolume);

    HRESULT ( STDMETHODCALLTYPE *GetVolume )(
        ISpVoice * This,
        /* [out] */ USHORT *pusVolume);

    HRESULT ( STDMETHODCALLTYPE *WaitUntilDone )(
        ISpVoice * This,
        /* [in] */ ULONG msTimeout);

    HRESULT ( STDMETHODCALLTYPE *SetSyncSpeakTimeout )(
        ISpVoice * This,
        /* [in] */ ULONG msTimeout);

    HRESULT ( STDMETHODCALLTYPE *GetSyncSpeakTimeout )(
        ISpVoice * This,
        /* [out] */ ULONG *pmsTimeout);

    /* [local] */ HANDLE ( STDMETHODCALLTYPE *SpeakCompleteEvent )(
        ISpVoice * This);

    /* [local] */ HRESULT ( STDMETHODCALLTYPE *IsUISupported )(
        ISpVoice * This,
        /* [in] */ const WCHAR *pszTypeOfUI,
        /* [in] */ void *pvExtraData,
        /* [in] */ ULONG cbExtraData,
        /* [out] */ BOOL *pfSupported);

    /* [local] */ HRESULT ( STDMETHODCALLTYPE *DisplayUI )(
        ISpVoice * This,
        /* [in] */ HWND hwndParent,
        /* [in] */ const WCHAR *pszTitle,
        /* [in] */ const WCHAR *pszTypeOfUI,
        /* [in] */ void *pvExtraData,
        /* [in] */ ULONG cbExtraData);

    END_INTERFACE
} ISpVoiceVtbl;

struct ISpVoice
{
    struct ISpVoiceVtbl *lpVtbl;
};
void TTS_SayUnicodeString(wchar_t *stringtosay)
{
	static CLSID CLSID_SpVoice = {0x96749377, 0x3391, 0x11D2,
								{0x9E,0xE3,0x00,0xC0,0x4F,0x79,0x73,0x96}};
	static GUID IID_ISpVoice = {0x6C44DF74,0x72B9,0x4992,
								{0xA1,0xEC,0xEF,0x99,0x6E,0x04,0x22,0xD4}};
	static ISpVoice *sp = NULL;

	if (!sp)
		CoCreateInstance(
				&CLSID_SpVoice,
				NULL,
				CLSCTX_SERVER,
				&IID_ISpVoice,
				(void*)&sp);

	if (sp)
	{
		sp->lpVtbl->Speak(sp, stringtosay, 1, NULL);
	}
}
void TTS_SayAsciiString(char *stringtosay)
{
	wchar_t bigbuffer[8192];
	mbstowcs(bigbuffer, stringtosay, sizeof(bigbuffer)/sizeof(bigbuffer[0]) - 1);
	bigbuffer[sizeof(bigbuffer)/sizeof(bigbuffer[0]) - 1] = 0;
	TTS_SayUnicodeString(bigbuffer);
}

cvar_t tts_mode = CVARD("tts_mode", "1", "Text to speech\n0: off\n1: Read only chat messages with a leading 'tts ' prefix.\n2: Read all chat messages\n3: Read every single console print.");
void TTS_SayChatString(char **stringtosay)
{
	if (!strncmp(*stringtosay, "tts ", 4))
	{
		*stringtosay += 4;
		if (tts_mode.ival != 1 && tts_mode.ival != 2)
			return;
	}
	else
	{
		if (tts_mode.ival != 2)
			return;
	}

	TTS_SayAsciiString(*stringtosay);
}
void TTS_SayConString(conchar_t *stringtosay)
{
	wchar_t bigbuffer[8192];
	int i;

	if (tts_mode.ival < 3)
		return;
	
	for (i = 0; i < 8192-1 && *stringtosay; i++, stringtosay++)
	{
		if ((*stringtosay & 0xff00) == 0xe000)
			bigbuffer[i] = *stringtosay & 0x7f;
		else
			bigbuffer[i] = *stringtosay & CON_CHARMASK;
	}
	bigbuffer[i] = 0;
	if (i)
		TTS_SayUnicodeString(bigbuffer);
}
void TTS_Say_f(void)
{
	TTS_SayAsciiString(Cmd_Args());
}

#define ISpRecognizer void
#define SPPHRASE void
#define SPSERIALIZEDPHRASE void
#define SPSTATEHANDLE void*
#define SPGRAMMARWORDTYPE int
#define SPPROPERTYINFO void
#define SPLOADOPTIONS void*
#define SPBINARYGRAMMAR void*
#define SPRULESTATE int
#define SPTEXTSELECTIONINFO void
#define SPWORDPRONOUNCEABLE void
#define SPGRAMMARSTATE int
typedef struct ISpRecoResult ISpRecoResult;
typedef struct ISpRecoContext ISpRecoContext;
typedef struct ISpRecoGrammar ISpRecoGrammar;

typedef struct ISpRecoContextVtbl
{
	HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
		ISpRecoContext * This,
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ void **ppvObject);

	ULONG ( STDMETHODCALLTYPE *AddRef )( 
		ISpRecoContext * This);

	ULONG ( STDMETHODCALLTYPE *Release )( 
		ISpRecoContext * This);

    HRESULT ( STDMETHODCALLTYPE *SetNotifySink )( 
        ISpRecoContext * This,
        /* [in] */ ISpNotifySink *pNotifySink);
    
    /* [local] */ HRESULT ( STDMETHODCALLTYPE *SetNotifyWindowMessage )( 
        ISpRecoContext * This,
        /* [in] */ HWND hWnd,
        /* [in] */ UINT Msg,
        /* [in] */ WPARAM wParam,
        /* [in] */ LPARAM lParam);
    
    /* [local] */ HRESULT ( STDMETHODCALLTYPE *SetNotifyCallbackFunction )( 
        ISpRecoContext * This,
        /* [in] */ SPNOTIFYCALLBACK *pfnCallback,
        /* [in] */ WPARAM wParam,
        /* [in] */ LPARAM lParam);
    
    /* [local] */ HRESULT ( STDMETHODCALLTYPE *SetNotifyCallbackInterface )( 
        ISpRecoContext * This,
        /* [in] */ ISpNotifyCallback *pSpCallback,
        /* [in] */ WPARAM wParam,
        /* [in] */ LPARAM lParam);
    
    /* [local] */ HRESULT ( STDMETHODCALLTYPE *SetNotifyWin32Event )( 
        ISpRecoContext * This);
    
    /* [local] */ HRESULT ( STDMETHODCALLTYPE *WaitForNotifyEvent )( 
        ISpRecoContext * This,
        /* [in] */ DWORD dwMilliseconds);
    
    /* [local] */ HANDLE ( STDMETHODCALLTYPE *GetNotifyEventHandle )( 
        ISpRecoContext * This);
    
    HRESULT ( STDMETHODCALLTYPE *SetInterest )( 
        ISpRecoContext * This,
        /* [in] */ ULONGLONG ullEventInterest,
        /* [in] */ ULONGLONG ullQueuedInterest);
    
    HRESULT ( STDMETHODCALLTYPE *GetEvents )( 
        ISpRecoContext * This,
        /* [in] */ ULONG ulCount,
        /* [size_is][out] */ SPEVENT *pEventArray,
        /* [out] */ ULONG *pulFetched);

    HRESULT ( STDMETHODCALLTYPE *GetInfo )( 
        ISpRecoContext * This,
        /* [out] */ SPEVENTSOURCEINFO *pInfo);
    
    HRESULT ( STDMETHODCALLTYPE *GetRecognizer )( 
        ISpRecoContext * This,
        /* [out] */ ISpRecognizer **ppRecognizer);
    
    HRESULT ( STDMETHODCALLTYPE *CreateGrammar )( 
        ISpRecoContext * This,
        /* [in] */ ULONGLONG ullGrammarId,
        /* [out] */ ISpRecoGrammar **ppGrammar);
} ISpRecoContextVtbl;
struct ISpRecoContext
{
    struct ISpRecoContextVtbl *lpVtbl;
};

typedef struct ISpRecoResultVtbl
{
    HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ISpRecoResult * This,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void **ppvObject);
    
    ULONG ( STDMETHODCALLTYPE *AddRef )( 
        ISpRecoResult * This);
    
    ULONG ( STDMETHODCALLTYPE *Release )( 
        ISpRecoResult * This);
    
    HRESULT ( STDMETHODCALLTYPE *GetPhrase )( 
        ISpRecoResult * This,
        /* [out] */ SPPHRASE **ppCoMemPhrase);
    
    HRESULT ( STDMETHODCALLTYPE *GetSerializedPhrase )( 
        ISpRecoResult * This,
        /* [out] */ SPSERIALIZEDPHRASE **ppCoMemPhrase);
    
    HRESULT ( STDMETHODCALLTYPE *GetText )( 
        ISpRecoResult * This,
        /* [in] */ ULONG ulStart,
        /* [in] */ ULONG ulCount,
        /* [in] */ BOOL fUseTextReplacements,
        /* [out] */ WCHAR **ppszCoMemText,
        /* [out] */ BYTE *pbDisplayAttributes);
    
    HRESULT ( STDMETHODCALLTYPE *Discard )( 
        ISpRecoResult * This,
        /* [in] */ DWORD dwValueTypes);
#if 0
    HRESULT ( STDMETHODCALLTYPE *GetResultTimes )( 
        ISpRecoResult * This,
        /* [out] */ SPRECORESULTTIMES *pTimes);
    
    HRESULT ( STDMETHODCALLTYPE *GetAlternates )( 
        ISpRecoResult * This,
        /* [in] */ ULONG ulStartElement,
        /* [in] */ ULONG cElements,
        /* [in] */ ULONG ulRequestCount,
        /* [out] */ ISpPhraseAlt **ppPhrases,
        /* [out] */ ULONG *pcPhrasesReturned);
    
    HRESULT ( STDMETHODCALLTYPE *GetAudio )( 
        ISpRecoResult * This,
        /* [in] */ ULONG ulStartElement,
        /* [in] */ ULONG cElements,
        /* [out] */ ISpStreamFormat **ppStream);
    
    HRESULT ( STDMETHODCALLTYPE *SpeakAudio )( 
        ISpRecoResult * This,
        /* [in] */ ULONG ulStartElement,
        /* [in] */ ULONG cElements,
        /* [in] */ DWORD dwFlags,
        /* [out] */ ULONG *pulStreamNumber);
    
    HRESULT ( STDMETHODCALLTYPE *Serialize )( 
        ISpRecoResult * This,
        /* [out] */ SPSERIALIZEDRESULT **ppCoMemSerializedResult);
    
    HRESULT ( STDMETHODCALLTYPE *ScaleAudio )( 
        ISpRecoResult * This,
        /* [in] */ const GUID *pAudioFormatId,
        /* [in] */ const WAVEFORMATEX *pWaveFormatEx);
    
    HRESULT ( STDMETHODCALLTYPE *GetRecoContext )( 
        ISpRecoResult * This,
        /* [out] */ ISpRecoContext **ppRecoContext);
    
#endif
} ISpRecoResultVtbl;
struct ISpRecoResult
{
    struct ISpRecoResultVtbl *lpVtbl;
};

typedef struct ISpRecoGrammarVtbl
{
    HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
        ISpRecoGrammar * This,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void **ppvObject);
    
    ULONG ( STDMETHODCALLTYPE *AddRef )( 
        ISpRecoGrammar * This);
    
    ULONG ( STDMETHODCALLTYPE *Release )( 
        ISpRecoGrammar * This);
    
    HRESULT ( STDMETHODCALLTYPE *ResetGrammar )( 
        ISpRecoGrammar * This,
        /* [in] */ WORD NewLanguage);
    
    HRESULT ( STDMETHODCALLTYPE *GetRule )( 
        ISpRecoGrammar * This,
        /* [in] */ const WCHAR *pszRuleName,
        /* [in] */ DWORD dwRuleId,
        /* [in] */ DWORD dwAttributes,
        /* [in] */ BOOL fCreateIfNotExist,
        /* [out] */ SPSTATEHANDLE *phInitialState);
    
    HRESULT ( STDMETHODCALLTYPE *ClearRule )( 
        ISpRecoGrammar * This,
        SPSTATEHANDLE hState);
    
    HRESULT ( STDMETHODCALLTYPE *CreateNewState )( 
        ISpRecoGrammar * This,
        SPSTATEHANDLE hState,
        SPSTATEHANDLE *phState);
    
    HRESULT ( STDMETHODCALLTYPE *AddWordTransition )( 
        ISpRecoGrammar * This,
        SPSTATEHANDLE hFromState,
        SPSTATEHANDLE hToState,
        const WCHAR *psz,
        const WCHAR *pszSeparators,
        SPGRAMMARWORDTYPE eWordType,
        float Weight,
        const SPPROPERTYINFO *pPropInfo);
    
    HRESULT ( STDMETHODCALLTYPE *AddRuleTransition )( 
        ISpRecoGrammar * This,
        SPSTATEHANDLE hFromState,
        SPSTATEHANDLE hToState,
        SPSTATEHANDLE hRule,
        float Weight,
        const SPPROPERTYINFO *pPropInfo);
    
    HRESULT ( STDMETHODCALLTYPE *AddResource )( 
        ISpRecoGrammar * This,
        /* [in] */ SPSTATEHANDLE hRuleState,
        /* [in] */ const WCHAR *pszResourceName,
        /* [in] */ const WCHAR *pszResourceValue);
    
    HRESULT ( STDMETHODCALLTYPE *Commit )( 
        ISpRecoGrammar * This,
        DWORD dwReserved);
    
    HRESULT ( STDMETHODCALLTYPE *GetGrammarId )( 
        ISpRecoGrammar * This,
        /* [out] */ ULONGLONG *pullGrammarId);
    
    HRESULT ( STDMETHODCALLTYPE *GetRecoContext )( 
        ISpRecoGrammar * This,
        /* [out] */ ISpRecoContext **ppRecoCtxt);
    
    HRESULT ( STDMETHODCALLTYPE *LoadCmdFromFile )( 
        ISpRecoGrammar * This,
        /* [string][in] */ const WCHAR *pszFileName,
        /* [in] */ SPLOADOPTIONS Options);
    
    HRESULT ( STDMETHODCALLTYPE *LoadCmdFromObject )( 
        ISpRecoGrammar * This,
        /* [in] */ REFCLSID rcid,
        /* [string][in] */ const WCHAR *pszGrammarName,
        /* [in] */ SPLOADOPTIONS Options);
    
    HRESULT ( STDMETHODCALLTYPE *LoadCmdFromResource )( 
        ISpRecoGrammar * This,
        /* [in] */ HMODULE hModule,
        /* [string][in] */ const WCHAR *pszResourceName,
        /* [string][in] */ const WCHAR *pszResourceType,
        /* [in] */ WORD wLanguage,
        /* [in] */ SPLOADOPTIONS Options);
    
    HRESULT ( STDMETHODCALLTYPE *LoadCmdFromMemory )( 
        ISpRecoGrammar * This,
        /* [in] */ const SPBINARYGRAMMAR *pGrammar,
        /* [in] */ SPLOADOPTIONS Options);
    
    HRESULT ( STDMETHODCALLTYPE *LoadCmdFromProprietaryGrammar )( 
        ISpRecoGrammar * This,
        /* [in] */ REFGUID rguidParam,
        /* [string][in] */ const WCHAR *pszStringParam,
        /* [in] */ const void *pvDataPrarm,
        /* [in] */ ULONG cbDataSize,
        /* [in] */ SPLOADOPTIONS Options);
    
    HRESULT ( STDMETHODCALLTYPE *SetRuleState )( 
        ISpRecoGrammar * This,
        /* [string][in] */ const WCHAR *pszName,
        void *pReserved,
        /* [in] */ SPRULESTATE NewState);
    
    HRESULT ( STDMETHODCALLTYPE *SetRuleIdState )( 
        ISpRecoGrammar * This,
        /* [in] */ ULONG ulRuleId,
        /* [in] */ SPRULESTATE NewState);
    
    HRESULT ( STDMETHODCALLTYPE *LoadDictation )( 
        ISpRecoGrammar * This,
        /* [string][in] */ const WCHAR *pszTopicName,
        /* [in] */ SPLOADOPTIONS Options);
    
    HRESULT ( STDMETHODCALLTYPE *UnloadDictation )( 
        ISpRecoGrammar * This);
    
    HRESULT ( STDMETHODCALLTYPE *SetDictationState )( 
        ISpRecoGrammar * This,
        /* [in] */ SPRULESTATE NewState);
    
    HRESULT ( STDMETHODCALLTYPE *SetWordSequenceData )( 
        ISpRecoGrammar * This,
        /* [in] */ const WCHAR *pText,
        /* [in] */ ULONG cchText,
        /* [in] */ const SPTEXTSELECTIONINFO *pInfo);
    
    HRESULT ( STDMETHODCALLTYPE *SetTextSelection )( 
        ISpRecoGrammar * This,
        /* [in] */ const SPTEXTSELECTIONINFO *pInfo);
    
    HRESULT ( STDMETHODCALLTYPE *IsPronounceable )( 
        ISpRecoGrammar * This,
        /* [string][in] */ const WCHAR *pszWord,
        /* [out] */ SPWORDPRONOUNCEABLE *pWordPronounceable);
    
    HRESULT ( STDMETHODCALLTYPE *SetGrammarState )( 
        ISpRecoGrammar * This,
        /* [in] */ SPGRAMMARSTATE eGrammarState);
    
    HRESULT ( STDMETHODCALLTYPE *SaveCmd )( 
        ISpRecoGrammar * This,
        /* [in] */ IStream *pStream,
        /* [optional][out] */ WCHAR **ppszCoMemErrorText);
    
    HRESULT ( STDMETHODCALLTYPE *GetGrammarState )( 
        ISpRecoGrammar * This,
        /* [out] */ SPGRAMMARSTATE *peGrammarState);
} ISpRecoGrammarVtbl;
struct ISpRecoGrammar
{
	struct ISpRecoGrammarVtbl *lpVtbl;
};

static ISpRecoContext *stt_recctx = NULL;
static ISpRecoGrammar *stt_gram = NULL;
void STT_Event(void)
{
	WCHAR *wstring, *i;
	struct SPEVENT ev;
	ISpRecoResult *rr;
	HRESULT hr;
	char asc[2048], *o;
	int l;
	unsigned short c;
	char *nib = "0123456789abcdef";
	if (!stt_gram)
		return;

	while (SUCCEEDED(hr = stt_recctx->lpVtbl->GetEvents(stt_recctx, 1, &ev, NULL)) && hr != S_FALSE)
	{
		rr = (ISpRecoResult*)ev.lParam;
		rr->lpVtbl->GetText(rr, -1, -1, TRUE, &wstring, NULL);
		for (l = sizeof(asc)-1, o = asc, i = wstring; l > 0 && *i; )
		{
			c = *i++;
			if (c == '\n' || c == ';')
			{
			}
			else if (c < 128)
			{
				*o++ = c;
				l--;
			}
			else if (l > 6)
			{
				*o++ = '^';
				*o++ = 'U';
				*o++ = nib[(c>>12)&0xf];
				*o++ = nib[(c>>8)&0xf];
				*o++ = nib[(c>>4)&0xf];
				*o++ = nib[(c>>0)&0xf];
			}
			else
				break;
		}
		*o = 0;
		CoTaskMemFree(wstring);
		Cbuf_AddText("say tts ", RESTRICT_LOCAL);
		Cbuf_AddText(asc, RESTRICT_LOCAL);
		Cbuf_AddText("\n", RESTRICT_LOCAL);
		rr->lpVtbl->Release(rr);
	}
}
void STT_Init_f(void)
{
	static CLSID CLSID_SpSharedRecoContext	=	{0x47206204, 0x5ECA, 0x11D2, {0x96, 0x0F, 0x00, 0xC0, 0x4F, 0x8E, 0xE6, 0x28}};
	static CLSID IID_SpRecoContext			=	{0xF740A62F, 0x7C15, 0x489E, {0x82, 0x34, 0x94, 0x0A, 0x33, 0xD9, 0x27, 0x2D}};

	if (stt_gram)
	{
		stt_gram->lpVtbl->Release(stt_gram);
		stt_recctx->lpVtbl->Release(stt_recctx);
		stt_gram = NULL;
		stt_recctx = NULL;
		Con_Printf("Speech-to-text disabled\n");
		return;
	}

	if (SUCCEEDED(CoCreateInstance(&CLSID_SpSharedRecoContext, NULL, CLSCTX_SERVER, &IID_SpRecoContext, (void*)&stt_recctx)))
	{
		ULONGLONG ev = (((ULONGLONG)1) << 38) | (((ULONGLONG)1) << 30) | (((ULONGLONG)1) << 33);
		if (SUCCEEDED(stt_recctx->lpVtbl->SetNotifyWindowMessage(stt_recctx, mainwindow, WM_USER, 0, 0)))
		if (SUCCEEDED(stt_recctx->lpVtbl->SetInterest(stt_recctx, ev, ev)))
		if (SUCCEEDED(stt_recctx->lpVtbl->CreateGrammar(stt_recctx, 0, &stt_gram)))
		{
			if (SUCCEEDED(stt_gram->lpVtbl->LoadDictation(stt_gram, NULL, 0)))
			if (SUCCEEDED(stt_gram->lpVtbl->SetDictationState(stt_gram, 1)))
			{
				//success!
				Con_Printf("Speech-to-text active\n");
				return;
			}
			stt_gram->lpVtbl->Release(stt_gram);
		}
		stt_recctx->lpVtbl->Release(stt_recctx);
	}
	stt_gram = NULL;
	stt_recctx = NULL;

	Con_Printf("Speech-to-text unavailable\n");
}
#endif

qboolean S_LoadMP3Sound (sfx_t *s, qbyte *data, int datalen, int sndspeed);

void Media_Init(void)
{
	int i;
#if defined(_WIN32) && !defined(WINRT)
	Cmd_AddCommand("tts", TTS_Say_f);
	Cmd_AddCommand("stt", STT_Init_f);
	Cvar_Register(&tts_mode, "Gimmicks");
#endif

#if defined(WINAVI)
	Media_RegisterEncoder(NULL, &capture_avi);
#endif
	Media_RegisterEncoder(NULL, &capture_raw);
#ifdef _DEBUG
	Media_RegisterEncoder(NULL, &capture_null);
#endif

	Cmd_AddCommand("playvideo", Media_PlayFilm_f);
	Cmd_AddCommand("playfilm", Media_PlayFilm_f);
	Cmd_AddCommand("cinematic", Media_PlayFilm_f);
	Cmd_AddCommand("music_fforward", Media_FForward_f);
	Cmd_AddCommand("music_rewind", Media_Rewind_f);
	Cmd_AddCommand("music_next", Media_Next_f);
	Cmd_AddCommand("media_next", Media_Next_f);
	Cmd_AddCommand("music", Media_NamedTrack_f);

	Cvar_Register(&music_playlist_index, "compat");
	for (i = 0; i < 6; i++)
	{
		Cvar_Get(va("music_playlist_list%i", i), "", 0, "compat");
		Cvar_Get(va("music_playlist_sampleposition%i", i), "-1", 0, "compat");
	}
	music_playlist_last = -1;

	Cmd_AddCommand("cd", CD_f);
	cdenabled = false;
	if (COM_CheckParm("-nocdaudio"))
		cdenabled = false;
	if (COM_CheckParm("-cdaudio"))
		cdenabled = true;

	media_playlisttypes = MEDIA_PLAYLIST | MEDIA_GAMEMUSIC | MEDIA_CVARLIST;

	Cmd_AddCommand("capture", Media_RecordFilm_f);
	Cmd_AddCommand("capturedemo", Media_RecordDemo_f);
	Cmd_AddCommand("capturestop", Media_StopRecordFilm_f);
	Cmd_AddCommand("capturepause", Media_CapturePause_f);

	Cvar_Register(&capturemessage,	"AVI capture controls");
	Cvar_Register(&capturesound,	"AVI capture controls");
	Cvar_Register(&capturerate,	"AVI capture controls");
	Cvar_Register(&capturewidth,	"AVI capture controls");
	Cvar_Register(&captureheight,	"AVI capture controls");
	Cvar_Register(&capturedriver,	"AVI capture controls");
	Cvar_Register(&capturecodec,	"AVI capture controls");

#if defined(WINAVI)
	Cvar_Register(&capturesoundbits,	"AVI capture controls");
	Cvar_Register(&capturesoundchannels,	"AVI capture controls");

	S_RegisterSoundInputPlugin(S_LoadMP3Sound);
#endif

#ifdef WINAMP
	Cvar_Register(&media_hijackwinamp,	"Media player things");
#endif
	Cvar_Register(&media_shuffle,	"Media player things");
	Cvar_Register(&media_repeat,	"Media player things");
	Cmd_AddCommand ("media_add", M_Media_Add_f);
	Cmd_AddCommand ("media_remove", M_Media_Remove_f);
#if !defined(NOMEDIAMENU) && !defined(NOBUILTINMENUS)
	Cmd_AddCommand ("menu_media", M_Menu_Media_f);
#endif
}



#ifdef WINAVI
typedef struct
{
	HACMSTREAM acm;

	unsigned int dstbuffer; /*in frames*/
	unsigned int dstcount; /*in frames*/
	unsigned int dststart; /*in frames*/
	qbyte *dstdata;

	unsigned int srcspeed;
	unsigned int srcwidth;
	unsigned int srcchannels;
	unsigned int srcoffset; /*in bytes*/
	unsigned int srclen;	/*in bytes*/
	qbyte srcdata[1];
} mp3decoder_t;

static void S_MP3_Purge(sfx_t *sfx)
{
	mp3decoder_t *dec = sfx->decoder.buf;

	sfx->decoder.buf = NULL;
	sfx->decoder.ended = NULL;
	sfx->decoder.purge = NULL;
	sfx->decoder.decodedata = NULL;

	qacmStreamClose(dec->acm, 0);

	if (dec->dstdata)
		BZ_Free(dec->dstdata);
	BZ_Free(dec);

	sfx->loadstate = SLS_NOTLOADED;
}

float S_MP3_Query(sfx_t *sfx, sfxcache_t *buf)
{
	//we don't know unless we decode it all
	if (buf)
	{
	}
	return 0;
}

/*must be thread safe*/
sfxcache_t *S_MP3_Locate(sfx_t *sfx, sfxcache_t *buf, ssamplepos_t start, int length)
{
	int newlen;
	if (buf)
	{
		mp3decoder_t *dec = sfx->decoder.buf;
		ACMSTREAMHEADER strhdr;
		char buffer[8192];
		extern cvar_t snd_linearresample_stream;
		int framesz = (dec->srcwidth/8 * dec->srcchannels);

		if (dec->dststart > start)
		{
			/*I don't know where the compressed data is for each sample. acm doesn't have a seek. so reset to start, for music this should be the most common rewind anyway*/
			dec->dststart = 0;
			dec->dstcount = 0;
			dec->srcoffset = 0;
		}

		if (dec->dstcount > snd_speed*6)
		{
			int trim = dec->dstcount - snd_speed; //retain a second of buffer in case we have multiple sound devices
			if (dec->dststart + trim > start)
			{
				trim = start - dec->dststart;
				if (trim < 0)
					trim = 0;
			}
//			if (trim < 0)
//				trim = 0;
///			if (trim > dec->dstcount)
//				trim = dec->dstcount;
			memmove(dec->dstdata, dec->dstdata + trim*framesz, (dec->dstcount - trim)*framesz);
			dec->dststart += trim;
			dec->dstcount -= trim;
		}

		while(start+length >= dec->dststart+dec->dstcount)
		{
			memset(&strhdr, 0, sizeof(strhdr));
			strhdr.cbStruct = sizeof(strhdr);
			strhdr.pbSrc = dec->srcdata + dec->srcoffset;
			strhdr.cbSrcLength = dec->srclen - dec->srcoffset;
			strhdr.pbDst = buffer;
			strhdr.cbDstLength = sizeof(buffer);

			qacmStreamPrepareHeader(dec->acm, &strhdr, 0);
			qacmStreamConvert(dec->acm, &strhdr, ACM_STREAMCONVERTF_BLOCKALIGN);
			qacmStreamUnprepareHeader(dec->acm, &strhdr, 0);
			dec->srcoffset += strhdr.cbSrcLengthUsed;
			if (!strhdr.cbDstLengthUsed)
			{
				if (strhdr.cbSrcLengthUsed)
					continue;
				break;
			}

			newlen = dec->dstcount + (strhdr.cbDstLengthUsed * ((float)snd_speed / dec->srcspeed))/framesz;
			if (dec->dstbuffer < newlen+64)
			{
				dec->dstbuffer = newlen+64 + snd_speed;
				dec->dstdata = BZ_Realloc(dec->dstdata, dec->dstbuffer*framesz);
			}

			SND_ResampleStream(strhdr.pbDst, 
				dec->srcspeed, 
				dec->srcwidth/8, 
				dec->srcchannels, 
				strhdr.cbDstLengthUsed / framesz,
				dec->dstdata+dec->dstcount*framesz,
				snd_speed,
				dec->srcwidth/8,
				dec->srcchannels,
				snd_linearresample_stream.ival);
			dec->dstcount = newlen;
		}

		buf->data = dec->dstdata;
		buf->length = dec->dstcount;
		buf->loopstart = -1;
		buf->numchannels = dec->srcchannels;
		buf->soundoffset = dec->dststart;
		buf->speed = snd_speed;
		buf->width = dec->srcwidth/8;
	}
	return buf;
}

#ifndef WAVE_FORMAT_MPEGLAYER3
#define WAVE_FORMAT_MPEGLAYER3 0x0055
typedef struct
{
	WAVEFORMATEX  wfx;
	WORD          wID;
	DWORD         fdwFlags;
	WORD          nBlockSize;
	WORD          nFramesPerBlock;
	WORD          nCodecDelay;
} MPEGLAYER3WAVEFORMAT;
#endif
#ifndef MPEGLAYER3_ID_MPEG
#define MPEGLAYER3_WFX_EXTRA_BYTES 12
#define MPEGLAYER3_FLAG_PADDING_OFF 2
#define MPEGLAYER3_ID_MPEG 1
#endif

qboolean S_LoadMP3Sound (sfx_t *s, qbyte *data, int datalen, int sndspeed)
{
	WAVEFORMATEX pcm_format;
	MPEGLAYER3WAVEFORMAT mp3format;
	HACMDRIVER drv = NULL;
	mp3decoder_t *dec;

	char ext[8];
	COM_FileExtension(s->name, ext, sizeof(ext));
	if (stricmp(ext, "mp3"))
		return false;

	dec = BZF_Malloc(sizeof(*dec) + datalen);
	if (!dec)
		return false;
	memcpy(dec->srcdata, data, datalen);
	dec->srclen = datalen;
	s->decoder.buf = dec;
	s->decoder.ended = S_MP3_Purge;
	s->decoder.purge = S_MP3_Purge;
	s->decoder.decodedata = S_MP3_Locate;
	s->decoder.querydata = S_MP3_Query;
	
	dec->dstdata = NULL;
	dec->dstcount = 0;
	dec->dststart = 0;
	dec->dstbuffer = 0;
	dec->srcoffset = 0;

	dec->srcspeed = 44100;
	dec->srcchannels = 2;
	dec->srcwidth = 16;

	memset (&pcm_format, 0, sizeof(pcm_format));
	pcm_format.wFormatTag = WAVE_FORMAT_PCM;
	pcm_format.nChannels = dec->srcchannels;
	pcm_format.nSamplesPerSec = dec->srcspeed;
	pcm_format.nBlockAlign = dec->srcwidth/8*dec->srcchannels;
	pcm_format.nAvgBytesPerSec = pcm_format.nSamplesPerSec*dec->srcwidth/8*dec->srcchannels;
	pcm_format.wBitsPerSample = dec->srcwidth;
	pcm_format.cbSize = 0;

	mp3format.wfx.cbSize = MPEGLAYER3_WFX_EXTRA_BYTES;
	mp3format.wfx.wFormatTag = WAVE_FORMAT_MPEGLAYER3;
	mp3format.wfx.nChannels = dec->srcchannels;
	mp3format.wfx.nAvgBytesPerSec = 128 * (1024 / 8);  // not really used but must be one of 64, 96, 112, 128, 160kbps
	mp3format.wfx.wBitsPerSample = 0;                  // MUST BE ZERO
	mp3format.wfx.nBlockAlign = 1;                     // MUST BE ONE
	mp3format.wfx.nSamplesPerSec = dec->srcspeed;       // 44.1kHz
	mp3format.fdwFlags = MPEGLAYER3_FLAG_PADDING_OFF;
	mp3format.nBlockSize = 522;					       // voodoo value #1 - 144 x (bitrate / sample rate) + padding
	mp3format.nFramesPerBlock = 1;                     // MUST BE ONE
	mp3format.nCodecDelay = 0;//1393;                      // voodoo value #2
	mp3format.wID = MPEGLAYER3_ID_MPEG;

	if (!qacmStartup() || 0!=qacmStreamOpen(&dec->acm, drv, (WAVEFORMATEX*)&mp3format, &pcm_format, NULL, 0, 0, 0))
	{
		Con_Printf("Couldn't init decoder\n");
		return false;
	}

	S_MP3_Locate(s, NULL, 0, 100);
	return true;
}
#endif




#else
void M_Media_Draw (void){}
void M_Media_Key (int key) {}
qboolean Media_ShowFilm(void){return false;}

double Media_TweekCaptureFrameTime(double oldtime, double time) { return oldtime+time ; }
void Media_RecordFrame (void) {}
void Media_CaptureDemoEnd(void) {}
void Media_RecordDemo_f(void) {}
void Media_RecordAudioFrame (short *sample_buffer, int samples) {}
void Media_StopRecordFilm_f (void) {}
void Media_RecordFilm_f (void){}
void M_Menu_Media_f (void) {}
float Media_CrossFade(int ch, float vol, float time) {return vol;}

char *Media_NextTrack(int musicchannelnum, float *time) {return NULL;}
qboolean Media_PausedDemo(qboolean fortiming) {return false;}

int filmtexture;
#endif

