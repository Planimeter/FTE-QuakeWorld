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
// snd_dma.c -- main control for any streaming sound output devices

#include "quakedef.h"

void S_Play(void);
void S_PlayVol(void);
void S_SoundList(void);
void S_Update_(soundcardinfo_t *sc);
void S_StopAllSounds(qboolean clear);
void S_StopAllSoundsC(void);

void S_UpdateCard(soundcardinfo_t *sc);

// =======================================================================
// Internal sound data & structures
// =======================================================================

soundcardinfo_t *sndcardinfo;	//the master card.

int				snd_blocked = 0;
static qboolean	snd_ambient = 1;
qboolean		snd_initialized = false;
int				snd_speed;

vec3_t		listener_origin;
vec3_t		listener_forward;
vec3_t		listener_right;
vec3_t		listener_up;
vec_t		sound_nominal_clip_dist=1000.0;

int			soundtime;		// sample PAIRS


#define	MAX_SFX		512
sfx_t		*known_sfx;		// hunk allocated [MAX_SFX]
int			num_sfx;

sfx_t		*ambient_sfx[NUM_AMBIENTS];

//int 		desired_speed = 44100;
int 		desired_bits = 16;

int sound_started=0;

cvar_t bgmvolume = {"musicvolume", "0", NULL, CVAR_ARCHIVE};
cvar_t volume = {"volume", "0.7", NULL, CVAR_ARCHIVE};

cvar_t nosound = {"nosound", "0"};
cvar_t precache = {"precache", "1"};
cvar_t loadas8bit = {"loadas8bit", "0"};
cvar_t bgmbuffer = {"bgmbuffer", "4096"};
cvar_t ambient_level = {"ambient_level", "0.3"};
cvar_t ambient_fade = {"ambient_fade", "100"};
cvar_t snd_noextraupdate = {"snd_noextraupdate", "0"};
cvar_t snd_show = {"snd_show", "0"};
cvar_t snd_khz = {"snd_khz", "11"};
cvar_t	snd_inactive = {"snd_inactive", "0"};	//set if you want sound even when tabbed out.
cvar_t _snd_mixahead = {"_snd_mixahead", "0.2", NULL, CVAR_ARCHIVE};
cvar_t snd_leftisright = {"snd_leftisright", "0", NULL, CVAR_ARCHIVE};
cvar_t snd_eax = {"snd_eax", "0"};
cvar_t snd_speakers = {"snd_numspeakers", "2"};
cvar_t	snd_playersoundvolume = {"snd_localvolume", "1"};	//sugested by crunch

cvar_t	snd_capture = {"snd_capture", "0"};

cvar_t snd_usemultipledevices = {"snd_multipledevices", "0"};


// ====================================================================
// User-setable variables
// ====================================================================


//
// Fake dma is a synchronous faking of the DMA progress used for
// isolating performance in the renderer.  The fakedma_updates is
// number of times S_Update() is called per second.
//

qboolean fakedma = false;
int fakedma_updates = 15;


void S_AmbientOff (void)
{
	snd_ambient = false;
}


void S_AmbientOn (void)
{
	snd_ambient = true;
}


void S_SoundInfo_f(void)
{
	soundcardinfo_t *sc;
	if (!sound_started)
	{
		Con_Printf ("sound system not started\n");
		return;
	}

	if (!sndcardinfo)
	{
		Con_Printf ("No sound cards\n");
		return;
	}
	for (sc = sndcardinfo; sc; sc = sc->next)
	{
		Con_Printf("%5d stereo\n", sc->sn.numchannels - 1);
		Con_Printf("%5d samples\n", sc->sn.samples);
		Con_Printf("%5d samplepos\n", sc->sn.samplepos);
		Con_Printf("%5d samplebits\n", sc->sn.samplebits);
		Con_Printf("%5d submission_chunk\n", sc->sn.submission_chunk);
		Con_Printf("%5d speed\n", sc->sn.speed);
		Con_Printf("0x%x dma buffer\n", sc->sn.buffer);
		Con_Printf("%5d total_channels\n", sc->total_chans);
	}
}

void (*pDSOUND_UpdateCapture) (void);
void S_RunCapture(void)
{
	//add new drivers in order or desirability.
	if (pDSOUND_UpdateCapture)
	{
		pDSOUND_UpdateCapture();
		return;
	}
}


sounddriver pDSOUND_InitCard;
sounddriver pALSA_InitCard;
sounddriver pOSS_InitCard;
sounddriver pSDL_InitCard;
sounddriver pWAV_InitCard;
sounddriver pAHI_InitCard;

typedef struct {
	char *name;
	sounddriver *ptr;
} sdriver_t;
sdriver_t drivers[] = {
//in order of preference
	{"DSound", &pDSOUND_InitCard},
	{"ALSA", &pALSA_InitCard},
	{"OSS", &pOSS_InitCard},
	{"SDL", &pSDL_InitCard},
	{"WaveOut", &pWAV_InitCard},
	{"AHI", &pAHI_InitCard},
	{NULL, NULL}
};

static int SNDDMA_Init(soundcardinfo_t *sc, int *cardnum, int *drivernum)
{
	sdriver_t *sd;
	int st = 0;

	memset(sc, 0, sizeof(*sc));
	if (snd_khz.value >= 45)
		sc->sn.speed = 48000;
	else if (snd_khz.value >= 30)	//set by a slider
		sc->sn.speed = 44100;
	else if (snd_khz.value >= 20)
		sc->sn.speed = 22050;
	else
		sc->sn.speed = 11025;

	sd = &drivers[*drivernum];
	if (!sd->ptr)
		return 2;	//no more cards.
	if (!*sd->ptr)	//driver not loaded
	{
		Con_DPrintf("Sound driver %s is not loaded\n", sd->name);
		st = 2;
	}
	else
	{
		Con_DPrintf("Trying to load a %s sound device\n", sd->name);
		st = (**sd->ptr)(sc, *cardnum);
	}

	if (st == 1)	//worked
	{
		*cardnum += 1;	//use the next card next time
		return st;
	}
	else if (st == 0)	//failed, try the next card with this driver.
	{
		*cardnum += 1;
		return SNDDMA_Init(sc, cardnum, drivernum);
	}
	else	//card number wasn't valid, try the first card of the next driver
	{
		*cardnum = 0;
		*drivernum += 1;
		return SNDDMA_Init(sc, cardnum, drivernum);
	}
}

/*
================
S_Startup
================
*/

void S_ClearRaw(void);
void S_Startup (void)
{
	int cardnum, drivernum;
	int warningmessage=0;
	int		rc;
	soundcardinfo_t *sc;

	if (!snd_initialized)
		return;

	if (sound_started)
		S_Shutdown();

	snd_blocked = 0;

	if (!fakedma)
	{
		for(cardnum = 0, drivernum = 0;;)
		{
			sc = Z_Malloc(sizeof(soundcardinfo_t));
			rc = SNDDMA_Init(sc, &cardnum, &drivernum);

			if (!rc)	//error stop
			{
				Con_Printf("S_Startup: SNDDMA_Init failed.\n");
				Z_Free(sc);
				break;
			}
			if (rc == 2)	//silently stop (no more cards)
			{
				Z_Free(sc);
				break;
			}

			if (sc->sn.numchannels < 3)
			{
				sc->pitch[0] = 0;
				sc->pitch[1] = 0;
				sc->dist[0] = 1;
				sc->dist[1] = 1;
				sc->yaw[0] = 270;
				sc->yaw[1] = 90;
			}
			else if (sc->sn.numchannels < 5)
			{
				sc->pitch[0] = 0;
				sc->pitch[1] = 0;
				sc->pitch[2] = 0;
				sc->pitch[3] = 0;
				sc->dist[0] = 1;
				sc->dist[1] = 1;
				sc->dist[2] = 1;
				sc->dist[3] = 1;
				sc->yaw[0] = 315;
				sc->yaw[1] = 45;
				sc->yaw[2] = 225;
				sc->yaw[3] = 135;
			}
			else
			{
				sc->pitch[0] = 0;
				sc->pitch[1] = 0;
				sc->pitch[2] = 0;
				sc->pitch[3] = 0;
				sc->pitch[4] = 0;
				sc->pitch[5] = 0;
				sc->dist[0] = 1;
				sc->dist[1] = 1;
				sc->dist[2] = 1;
				sc->dist[3] = 1;
				sc->dist[4] = 1;
				sc->dist[5] = 1;
				sc->yaw[0] = 315;
				sc->yaw[1] = 45;
				sc->yaw[2] = 0;
				sc->yaw[3] = 0;
				sc->yaw[4] = 225;
				sc->yaw[5] = 135;
			}

			if (sndcardinfo)
			{	//if the sample speeds of multiple soundcards do not match, it'll fail.
				if (sc->sn.speed != sc->sn.speed)
				{
					if (!warningmessage)
					{
						Con_Printf("S_Startup: Ignoring soundcard %s due to mismatched sample speeds.\nTry running Quake with -singlesound to use just the primary soundcard\n", sc->name);
						S_ShutdownCard(sc);
						warningmessage = true;
					}

					Z_Free(sc);
					continue;
				}
			}
			else
				snd_speed = sc->sn.speed;

			sc->next = sndcardinfo;
			sndcardinfo = sc;

			if (!snd_usemultipledevices.value)
				break;
		}
	}

	sound_started = 1;

	S_ClearRaw();
}

void SNDDMA_SetUnderWater(qboolean underwater)
{
	soundcardinfo_t *sc;

	for (sc = sndcardinfo; sc; sc=sc->next)
		sc->SetWaterDistortion(sc, underwater);
}

//why isn't this part of S_Restart_f anymore?
//so that the video code can call it directly without flushing the models it's just loaded.
void S_DoRestart (void)
{
	if (COM_CheckParm("-nosound"))
		return;

	S_StopAllSounds (true);

	S_Shutdown();
	sound_started = 0;
	S_Startup();

	ambient_sfx[AMBIENT_WATER] = S_PrecacheSound ("ambience/water1.wav");
	ambient_sfx[AMBIENT_SKY] = S_PrecacheSound ("ambience/wind2.wav");

	S_StopAllSounds (true);
}

void S_Restart_f (void)
{
	Cache_Flush();//forget the old sounds.

	S_DoRestart();
}

void S_Control_f (void)
{
	int i;
	char *command;
	if (Cmd_Argc() < 2)
		return;

	command = Cmd_Argv (1);

	if (!Q_strcasecmp(command, "off"))
	{
		Cache_Flush();//forget the old sounds.

		S_StopAllSounds (true);

		S_Shutdown();
		sound_started = 0;
	}
	else if (!Q_strcasecmp(command, "multi") || !Q_strcasecmp(command, "multiple"))
	{
		if (!Q_strcasecmp(Cmd_Argv (2), "off"))
		{
			if (snd_usemultipledevices.value)
			{
				Cvar_SetValue(&snd_usemultipledevices, 0);
				S_Restart_f();
			}
		}
		else if (!snd_usemultipledevices.value)
		{
			Cvar_SetValue(&snd_usemultipledevices, 1);
			S_Restart_f();
		}
		return;
	}
	if (!Q_strcasecmp(command, "single"))
	{
		Cvar_SetValue(&snd_usemultipledevices, 0);
		S_Restart_f();
		return;
	}

	if (!Q_strcasecmp(command, "rate") || !Q_strcasecmp(command, "speed"))
	{
		Cvar_SetValue(&snd_khz, atof(Cmd_Argv (2))/1000);
		S_Restart_f();
		return;
	}

	//individual device control
	if (!Q_strncasecmp(command, "card", 4))
	{
		int card;
		soundcardinfo_t *sc;
		card = atoi(command+4);

		for (i = 0, sc = sndcardinfo; i < card && sc; i++,sc=sc->next)
			;

		if (!sc)
		{
			Con_Printf("Sound card %i is invalid (try resetting first)\n", card);
			return;
		}
		if (Cmd_Argc() < 3)
		{
			Con_Printf("Scard %i is %s\n", card, sc->name);
			return;
		}
		command = Cmd_Argv (2);
		if (!Q_strcasecmp(command, "mono"))
		{
			for (i = 0; i < MAXSOUNDCHANNELS; i++)
			{
				sc->yaw[i] = 0;
				sc->pitch[i] = 0;	//point forwards
				sc->dist[i] = 1;
			}
		}
		else if (!Q_strcasecmp(command, "standard") || !Q_strcasecmp(command, "stereo"))
		{
			for (i = 0; i < MAXSOUNDCHANNELS; i++)
			{
				if (i & 1)
					sc->yaw[i] = 90;	//right
				else
					sc->yaw[i] = 270;	//left
				sc->pitch[i] = 0;
				sc->dist[i] = 1;
			}
		}
		else if (!Q_strcasecmp(command, "swap"))
		{
			for (i = 0; i < MAXSOUNDCHANNELS; i++)
			{
				if (i & 1)
					sc->yaw[i] = 270;	//left
				else
					sc->yaw[i] = 90;	//right
				sc->pitch[i] = 0;
				sc->dist[i] = 1;
			}
		}
		else if (!Q_strcasecmp(command, "front"))
		{
			for (i = 0; i < MAXSOUNDCHANNELS; i++)
			{
				if (i & 1)
					sc->yaw[i] = 45;	//front right
				else
					sc->yaw[i] = 315;	//front left
				sc->pitch[i] = 0;
				sc->dist[i] = 1;
			}
		}
		else if (!Q_strcasecmp(command, "back"))
		{
			for (i = 0; i < MAXSOUNDCHANNELS; i++)
			{
				if (i & 1)
					sc->yaw[i] = 180-45;	//behind right
				else
					sc->yaw[i] = 180+45;	//behind left
				sc->pitch[i] = 0;
				sc->dist[i] = 1;
			}
		}
		return;
	}
}

/*
================
S_Init
================
*/
void S_Init (void)
{
	int p;

	if (snd_initialized)	//whoops
	{
		Con_Printf("Sound is already initialized\n");
		return;
	}
	Con_DPrintf("\nSound Initialization\n");

//	if (COM_CheckParm("-simsound"))
//		fakedma = true;

	Cmd_AddCommand("play", S_Play);
	Cmd_AddCommand("play2", S_Play);
	Cmd_AddCommand("playvol", S_PlayVol);
	Cmd_AddCommand("stopsound", S_StopAllSoundsC);
	Cmd_AddCommand("soundlist", S_SoundList);
	Cmd_AddCommand("soundinfo", S_SoundInfo_f);

	Cmd_AddCommand("snd_restart", S_Restart_f);

	Cmd_AddCommand("soundcontrol", S_Control_f);

	Cvar_Register(&nosound,				"Sound controls");
	Cvar_Register(&volume,				"Sound controls");
	Cvar_Register(&precache,			"Sound controls");
	Cvar_Register(&loadas8bit,			"Sound controls");
	Cvar_Register(&bgmvolume,			"Sound controls");
	Cvar_Register(&bgmbuffer,			"Sound controls");
	Cvar_Register(&ambient_level,		"Sound controls");
	Cvar_Register(&ambient_fade,		"Sound controls");
	Cvar_Register(&snd_noextraupdate,	"Sound controls");
	Cvar_Register(&snd_show,			"Sound controls");
	Cvar_Register(&_snd_mixahead,		"Sound controls");
	Cvar_Register(&snd_khz,				"Sound controls");
	Cvar_Register(&snd_leftisright,		"Sound controls");
	Cvar_Register(&snd_eax,				"Sound controls");
	Cvar_Register(&snd_speakers,		"Sound controls");

	Cvar_Register(&snd_capture,			"Sound controls");

	Cvar_Register(&snd_inactive,		"Sound controls");

	Cvar_Register(&snd_playersoundvolume,		"Sound controls");
	Cvar_Register(&snd_usemultipledevices,		"Sound controls");

	if (COM_CheckParm("-nosound"))
	{
		Cvar_ForceSet(&nosound, "1");
		nosound.flags |= CVAR_NOSET;
		return;
	}

	p = COM_CheckParm ("-soundspeed");
	if (p)
	{
		if (p < com_argc-1)
			Cvar_SetValue(&snd_khz, atof(com_argv[p+1])/1000);
		else
			Sys_Error ("S_Init: you must specify a speed in KB after -soundspeed");
	}

	if (COM_CheckParm ("-nomultipledevices") || COM_CheckParm ("-singlesound"))
		Cvar_SetValue(&snd_usemultipledevices, 0);

	if (COM_CheckParm ("-multisound"))
		Cvar_SetValue(&snd_usemultipledevices, 1);


	if (host_parms.memsize < 0x800000)
	{
		Cvar_Set (&loadas8bit, "1");
		Con_Printf ("loading all sounds as 8bit\n");
	}

	snd_initialized = true;

	SND_InitScaletable ();

	known_sfx = Hunk_AllocName (MAX_SFX*sizeof(sfx_t), "sfx_t");
	num_sfx = 0;

// create a piece of DMA memory

/*	if (fakedma)
	{
		cursndcard = Z_Malloc(sizeof(*sndcardinfo));
		cursndcard->next = sndcardinfo;
		sndcardinfo = cursndcard;

		shm = (void *) Hunk_AllocName(sizeof(*shm), "shm");
		shm->splitbuffer = 0;
		shm->samplebits = 16;
		shm->speed = 22050;
		shm->numchannels = 2;
		shm->samples = 44100;
		shm->samplepos = 0;
		shm->soundalive = true;
		shm->gamealive = true;
		shm->submission_chunk = 1;
		shm->buffer = Hunk_AllocName(1<<16, "shmbuf");
	}
*/
	if (sndcardinfo)
		Con_SafePrintf ("Sound sampling rate: %i\n", sndcardinfo->sn.speed);

	// provides a tick sound until washed clean

//	if (shm->buffer)
//		shm->buffer[4] = shm->buffer[5] = 0x7f;	// force a pop for debugging

	ambient_sfx[AMBIENT_WATER] = S_PrecacheSound ("ambience/water1.wav");
	ambient_sfx[AMBIENT_SKY] = S_PrecacheSound ("ambience/wind2.wav");

	S_StopAllSounds (true);
}


// =======================================================================
// Shutdown sound engine
// =======================================================================

void S_ShutdownCard(soundcardinfo_t *sc)
{
#if defined(_WIN32) && !defined(NODIRECTX)
	extern int aimedforguid;
	aimedforguid = 0;
#endif
	if (!fakedma)
	{
		sc->Shutdown(sc);
	}
}
void S_Shutdown(void)
{
	soundcardinfo_t *sc, *next;
	for (sc = sndcardinfo; sc; sc=next)
	{
		next = sc->next;
		S_ShutdownCard(sc);
		Z_Free(sc);
		sndcardinfo = next;
	}

	sound_started = 0;
}


// =======================================================================
// Load a sound
// =======================================================================

/*
==================
S_FindName

==================
*/
sfx_t *S_FindName (char *name)
{
	int		i;
	sfx_t	*sfx;

	if (!name)
		Sys_Error ("S_FindName: NULL\n");

	if (Q_strlen(name) >= MAX_OSPATH)
		Sys_Error ("Sound name too long: %s", name);

// see if already loaded
	for (i=0 ; i < num_sfx ; i++)
		if (!Q_strcmp(known_sfx[i].name, name))
		{
			return &known_sfx[i];
		}

	if (num_sfx == MAX_SFX)
		Sys_Error ("S_FindName: out of sfx_t");

	sfx = &known_sfx[i];
	strcpy (sfx->name, name);

	num_sfx++;

	return sfx;
}


/*
==================
S_TouchSound

==================
*/
void S_TouchSound (char *name)
{
	sfx_t	*sfx;

	if (!sound_started)
		return;

	sfx = S_FindName (name);
	Cache_Check (&sfx->cache);
}

/*
==================
S_PrecacheSound

==================
*/
sfx_t *S_PrecacheSound (char *name)
{
	sfx_t	*sfx;

	if (nosound.value)
		return NULL;

	sfx = S_FindName (name);

// cache it in
	if (precache.value &&sndcardinfo)
		S_LoadSound (sfx);

	return sfx;
}


//=============================================================================

/*
=================
SND_PickChannel
=================
*/
channel_t *SND_PickChannel(soundcardinfo_t *sc, int entnum, int entchannel)
{
    int ch_idx;
    int first_to_die;
    int life_left;

// Check for replacement sound, or find the best one to replace
    first_to_die = -1;
    life_left = 0x7fffffff;
    for (ch_idx=NUM_AMBIENTS + NUM_MUSICS; ch_idx < NUM_AMBIENTS + NUM_MUSICS + MAX_DYNAMIC_CHANNELS ; ch_idx++)
    {
		if (entchannel != 0		// channel 0 never overrides
		&& sc->channel[ch_idx].entnum == entnum
		&& (sc->channel[ch_idx].entchannel == entchannel || entchannel == -1) && sc->channel[ch_idx].end > snd_speed/10)
		{	// always override sound from same entity
			first_to_die = ch_idx;
			break;
		}

		// don't let monster sounds override player sounds
		if (sc->channel[ch_idx].entnum == cl.playernum[0]+1 && entnum != cl.playernum[0]+1 && sc->channel[ch_idx].sfx)
			continue;

		if (sc->channel[ch_idx].end < life_left)
		{
			life_left = sc->channel[ch_idx].end;
			first_to_die = ch_idx;
		}
   }

	if (first_to_die == -1)
		return NULL;

	if (sc->channel[first_to_die].sfx)
		sc->channel[first_to_die].sfx = NULL;

    return &sc->channel[first_to_die];
}

/*
=================
SND_Spatialize
=================
*/
void SND_Spatialize(soundcardinfo_t *sc, channel_t *ch)
{
    vec_t dotright, dotforward, dotup;
    vec_t dist;
    vec_t scale;
    vec3_t source_vec;
	sfx_t *snd;
	int i;

// anything coming from the view entity will always be full volume
	if (ch->entnum == -1 || ch->entnum == cl.playernum[0]+1)
	{
		for (i = 0; i < sc->sn.numchannels; i++)
		{
			ch->vol[i] = ch->master_vol * snd_playersoundvolume.value;
			ch->delay[i] = 0;
		}
		return;
	}

// calculate stereo seperation and distance attenuation
	snd = ch->sfx;
	VectorSubtract(ch->origin, listener_origin, source_vec);

	dist = VectorNormalize(source_vec) * ch->dist_mult;

	dotright = DotProduct(listener_right, source_vec);
	dotforward = DotProduct(listener_forward, source_vec);
	dotup = DotProduct(listener_up, source_vec);

	for (i = 0; i < sc->sn.numchannels; i++)
	{
		scale = 1 + (dotright*sin(sc->yaw[i]*M_PI/180) + dotforward*cos(sc->yaw[i]*M_PI/180));// - dotup*cos(sc->pitch[0])*2;
		scale = (1.0 - dist) * scale * sc->dist[i];
		ch->vol[i] = (int) (ch->master_vol * scale);
		if (ch->vol[i] < 0)
			ch->vol[i] = 0;

		ch->delay[i] = 0;//(scale[0]-1)*1024;
	}
}

// =======================================================================
// Start a sound effect
// =======================================================================

void S_StartSoundCard(soundcardinfo_t *sc, int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol, float attenuation, int startpos)
{
	channel_t *target_chan, *check;
	sfxcache_t	*scache;
	int		vol;
	int		ch_idx;
	int		skip;

	if (!sound_started)
		return;

	if (!sfx)
		return;

	if (nosound.value)
		return;

	vol = fvol*255;

// pick a channel to play on
	target_chan = SND_PickChannel(sc, entnum, entchannel);
	if (!target_chan)
		return;

// spatialize
	memset (target_chan, 0, sizeof(*target_chan));
	if (!origin)
	{
		VectorCopy(listener_origin, target_chan->origin);
	}
	else
	{
		VectorCopy(origin, target_chan->origin);
	}
	target_chan->dist_mult = attenuation / sound_nominal_clip_dist;
	target_chan->master_vol = vol;
	target_chan->entnum = entnum;
	target_chan->entchannel = entchannel;
	SND_Spatialize(sc, target_chan);

	if (!target_chan->vol[0] && !target_chan->vol[1] && !target_chan->vol[2] && !target_chan->vol[3] && !target_chan->vol[4] && !target_chan->vol[5])
		return;		// not audible at all

// new channel
	scache = S_LoadSound (sfx);
	if (!scache)
	{
		target_chan->sfx = NULL;
		return;		// couldn't load the sound's data
	}
	target_chan->sfx = sfx;
	target_chan->pos = startpos;
	target_chan->end = sc->paintedtime + scache->length;
	target_chan->looping = false;

// if an identical sound has also been started this frame, offset the pos
// a bit to keep it from just making the first one louder
	check = &sc->channel[NUM_AMBIENTS];
	for (ch_idx=NUM_AMBIENTS + NUM_MUSICS; ch_idx < NUM_AMBIENTS + NUM_MUSICS + MAX_DYNAMIC_CHANNELS; ch_idx++, check++)
	{
		if (check == target_chan)
			continue;
		if (check->sfx == sfx && !check->pos)
		{
			skip = rand () % (int)(0.1*sc->sn.speed);
			if (skip >= target_chan->end)
				skip = target_chan->end - 1;
			target_chan->pos += skip;
			target_chan->end -= skip;
			break;
		}
	}
}

void S_StartSound(int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol, float attenuation)
{
	soundcardinfo_t *sc;

	if (!sfx || !*sfx->name)	//no named sounds would need specific starting.
		return;

	for (sc = sndcardinfo; sc; sc = sc->next)
		S_StartSoundCard(sc, entnum, entchannel, sfx, origin, fvol, attenuation, 0);
}

qboolean S_IsPlayingSomewhere(sfx_t *s)
{
	soundcardinfo_t *si;
	int i;
	for (si = sndcardinfo; si; si=si->next)
	{
		for (i = 0; i < MAX_CHANNELS; i++)
		if (si->channel[i].sfx == s)
			return true;
	}
	return false;
}

void S_StopSoundCard(soundcardinfo_t *sc, int entnum, int entchannel)
{
	int i;

	for (i=0 ; i<sc->total_chans ; i++)
	{
		if (sc->channel[i].entnum == entnum
			&& sc->channel[i].entchannel == entchannel)
		{
			sc->channel[i].end = 0;
			sc->channel[i].sfx = NULL;
			return;
		}
	}
}

void S_StopSound(int entnum, int entchannel)
{
	soundcardinfo_t *sc;
	for (sc = sndcardinfo; sc; sc = sc->next)
		S_StopSoundCard(sc, entnum, entchannel);
}

void S_StopAllSounds(qboolean clear)
{
	int		i;
	sfx_t *s;

	soundcardinfo_t *sc;

	for (sc = sndcardinfo; sc; sc = sc->next)
	{
		if (!sound_started)
			return;


		sc->total_chans = MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS + NUM_MUSICS;	// no statics

		for (i=0 ; i<MAX_CHANNELS ; i++)
			if (sc->channel[i].sfx)
			{
				s = sc->channel[i].sfx;
				sc->channel[i].sfx = NULL;
				if (s->decoder)
				if (!S_IsPlayingSomewhere(s))	//if we aint playing it elsewhere, free it compleatly.
				{
					s->decoder->abort(s);
				}
			}

		Q_memset(sc->channel, 0, MAX_CHANNELS * sizeof(channel_t));

		if (clear)
			S_ClearBuffer (sc);
	}
}

void S_StopAllSoundsC (void)
{
	S_StopAllSounds (true);
}

void S_ClearBuffer (soundcardinfo_t *sc)
{
	void *buffer;

	int		clear;
//#if defined(_WIN32) && !defined(NODIRECTX)
//	if (!sound_started || (!sc->sn.buffer && !sc->pDSBuf))
//#else
	if (!sound_started || !sc->sn.buffer)
//#endif
		return;

	if (sc->sn.samplebits == 8)
		clear = 0x80;
	else
		clear = 0;

	buffer = sc->Lock(sc);
	if (buffer)
	{
		Q_memset(sc->sn.buffer, clear, sc->sn.samples * sc->sn.samplebits/8);
		sc->Unlock(sc, buffer);
	}
/*
#if defined(_WIN32) && !defined(NODIRECTX)
	if (sc->pDSBuf)
	{
		DWORD	dwSize;
		DWORD	*pData;
		int		reps;
		HRESULT	hresult;

		reps = 0;

		while ((hresult = sc->pDSBuf->lpVtbl->Lock(sc->pDSBuf, 0, sc->gSndBufSize, (void**)&pData, &dwSize, NULL, NULL, 0)) != DS_OK)
		{
			if (hresult != DSERR_BUFFERLOST)
			{
				Con_Printf ("S_ClearBuffer: DS::Lock Sound Buffer Failed\n");
				S_ShutdownCard (sc);
				return;
			}

			if (++reps > 10000)
			{
				Con_Printf ("S_ClearBuffer: DS: couldn't restore buffer\n");
				S_ShutdownCard (sc);
				return;
			}
		}

		Q_memset(pData, clear, sc->sn.samples * sc->sn.samplebits/8);

		sc->pDSBuf->lpVtbl->Unlock(sc->pDSBuf, pData, dwSize, NULL, 0);

	}
	else
#endif
	{
		Q_memset(sc->sn.buffer, clear, sc->sn.samples * sc->sn.samplebits/8);
	}
*/
}

/*
=================
S_StaticSound
=================
*/
void S_StaticSound (sfx_t *sfx, vec3_t origin, float vol, float attenuation)
{
	channel_t	*ss;
	sfxcache_t		*scache;
	soundcardinfo_t *scard;

	if (!sfx)
		return;

	for (scard = sndcardinfo; scard; scard = scard->next)
	{
		if (scard->total_chans == MAX_CHANNELS)
		{
			Con_Printf ("total_channels == MAX_CHANNELS\n");
			continue;
		}

		ss = &scard->channel[scard->total_chans];
		scard->total_chans++;

		scache = S_LoadSound (sfx);
		if (!scache)
			return;

		if (scache->loopstart == -1)
		{
			Con_Printf ("Ambient sound %s not looped\n", sfx->name);
			return;
		}

		ss->sfx = sfx;
		VectorCopy (origin, ss->origin);
		ss->master_vol = vol;
		ss->dist_mult = (attenuation/64) / sound_nominal_clip_dist;
		ss->end = scard->paintedtime + scache->length;
		ss->looping = true;

		SND_Spatialize (scard, ss);
	}
}


//=============================================================================

/*
===================
S_UpdateAmbientSounds
===================
*/
char *Media_NextTrack(void);
mleaf_t *Q1BSP_LeafForPoint (model_t *model, vec3_t p);
void S_UpdateAmbientSounds (soundcardinfo_t *sc)
{
	mleaf_t		*l;
	float		vol;
	int			ambient_channel;
	channel_t	*chan;
	int i;

	if (!snd_ambient)
		return;

	for (i = NUM_AMBIENTS; i < NUM_AMBIENTS + NUM_MUSICS; i++)
	{
		chan = &sc->channel[i];
		if (!chan->sfx)
		{
			char *nexttrack = Media_NextTrack();
			sfxcache_t *scache;
			sfx_t *newmusic;

			if (nexttrack && *nexttrack)
			{
				newmusic = S_PrecacheSound(nexttrack);


				scache = S_LoadSound (newmusic);
				if (scache)
				{
					chan->sfx = newmusic;
					chan->pos = 0;
					chan->end = sc->paintedtime + scache->length+1000;
					chan->vol[0] = chan->vol[1] = chan->vol[2] = chan->vol[3] = chan->vol[4] = chan->vol[5] = chan->master_vol = 100;
				}
			}
		}
		if (chan->sfx)
			chan->vol[0] = chan->vol[1] = chan->vol[2] = chan->vol[3] = chan->vol[4] = chan->vol[5] = chan->master_vol = (255/volume.value)*bgmvolume.value;
	}


// calc ambient sound levels
	if (!cl.worldmodel || cl.worldmodel->type != mod_brush || cl.worldmodel->fromgame != fg_quake)
		return;

	l = Q1BSP_LeafForPoint(cl.worldmodel, listener_origin);
	if (!l || !ambient_level.value)
	{
		for (ambient_channel = 0 ; ambient_channel< NUM_AMBIENTS ; ambient_channel++)
			sc->channel[ambient_channel].sfx = NULL;
		return;
	}

	for (ambient_channel = 0 ; ambient_channel< NUM_AMBIENTS ; ambient_channel++)
	{
		chan = &sc->channel[ambient_channel];
		chan->sfx = ambient_sfx[ambient_channel];

		VectorCopy(listener_origin, chan->origin);

		vol = ambient_level.value * l->ambient_sound_level[ambient_channel];
		if (vol < 8)
			vol = 0;

	// don't adjust volume too fast
		if (chan->master_vol < vol)
		{
			chan->master_vol += host_frametime * ambient_fade.value;
			if (chan->master_vol > vol)
				chan->master_vol = vol;
		}
		else if (chan->master_vol > vol)
		{
			chan->master_vol -= host_frametime * ambient_fade.value;
			if (chan->master_vol < vol)
				chan->master_vol = vol;
		}

		chan->vol[0] = chan->vol[1] = chan->vol[2] = chan->vol[3] = chan->vol[4] = chan->vol[5] = chan->master_vol;
	}
}

/*
============
S_Update

Called once each time through the main loop
============
*/
void S_Update(vec3_t origin, vec3_t forward, vec3_t right, vec3_t up)
{
	soundcardinfo_t *sc;

	VectorCopy(origin, listener_origin);
	VectorCopy(forward, listener_forward);
	VectorCopy(right, listener_right);
	VectorCopy(up, listener_up);

	S_RunCapture();

	for (sc = sndcardinfo; sc; sc = sc->next)
		S_UpdateCard(sc);

}
void S_UpdateCard(soundcardinfo_t *sc)
{
	int			i, j;
	int			total;
	channel_t	*ch;
	channel_t	*combine;

	if (!sound_started)
		return;
	if ((snd_blocked > 0))
	{
		if (!sc->inactive_sound)
			return;
	}

// update general area ambient sound sources
	S_UpdateAmbientSounds (sc);

	combine = NULL;

// update spatialization for static and dynamic sounds
	ch = sc->channel+NUM_AMBIENTS+NUM_MUSICS;
	for (i=NUM_AMBIENTS+ NUM_MUSICS ; i<sc->total_chans; i++, ch++)
	{
		if (!ch->sfx)
			continue;

		SND_Spatialize(sc, ch);         // respatialize channel
		if (!ch->vol[0] && !ch->vol[1] && !ch->vol[2] && !ch->vol[3] && !ch->vol[4] && !ch->vol[5])
			continue;

	// try to combine static sounds with a previous channel of the same
	// sound effect so we don't mix five torches every frame

		if (i > MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS + NUM_MUSICS)
		{
		// see if it can just use the last one
			if (combine && combine->sfx == ch->sfx)
			{
				combine->vol[0] += ch->vol[0];
				combine->vol[1] += ch->vol[1];
				combine->vol[2] += ch->vol[2];
				combine->vol[3] += ch->vol[3];
				combine->vol[4] += ch->vol[4];
				combine->vol[5] += ch->vol[5];
				ch->vol[0] = ch->vol[1] = ch->vol[2] = ch->vol[3] = ch->vol[4] = ch->vol[5] = 0;
				continue;
			}
		// search for one
			combine = sc->channel+MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS + NUM_MUSICS;
			for (j=MAX_DYNAMIC_CHANNELS + NUM_AMBIENTS+ NUM_MUSICS ; j<i; j++, combine++)
				if (combine->sfx == ch->sfx)
					break;

			if (j == sc->total_chans)
			{
				combine = NULL;
			}
			else
			{
				if (combine != ch)
				{
					combine->vol[0] += ch->vol[0];
					combine->vol[1] += ch->vol[1];
					combine->vol[2] += ch->vol[2];
					combine->vol[3] += ch->vol[3];
					combine->vol[4] += ch->vol[4];
					combine->vol[5] += ch->vol[5];
					ch->vol[0] = ch->vol[1] = ch->vol[2] = ch->vol[3] = ch->vol[4] = ch->vol[5] = 0;
				}
				continue;
			}
		}


	}

//
// debugging output
//
	if (snd_show.value)
	{
		total = 0;
		ch = sc->channel;
		for (i=0 ; i<sc->total_chans; i++, ch++)
			if (ch->sfx && (ch->vol[0] || ch->vol[1]) )
			{
//					Con_Printf ("%i, %i %i %i %i %i %i %s\n", i, ch->vol[0], ch->vol[1], ch->vol[2], ch->vol[3], ch->vol[4], ch->vol[5], ch->sfx->name);
				total++;
			}

		Con_Printf ("----(%i)----\n", total);
	}

// mix some sound
	S_Update_(sc);
}

void GetSoundtime(soundcardinfo_t *sc)
{
	int		samplepos;
	int		fullsamples;

	fullsamples = sc->sn.samples / sc->sn.numchannels;

// it is possible to miscount buffers if it has wrapped twice between
// calls to S_Update.  Oh well.
	samplepos = sc->GetDMAPos(sc);

	if (samplepos < sc->oldsamplepos)
	{
		sc->buffers++;					// buffer wrapped

		if (sc->paintedtime > 0x40000000)
		{	// time to chop things off to avoid 32 bit limits
			sc->buffers = 0;
			sc->paintedtime = fullsamples;
			S_StopAllSounds (true);
		}

		sc->rawstart =0;
	}
	else
		sc->rawstart += samplepos - sc->oldsamplepos;
	sc->oldsamplepos = samplepos;

	soundtime = sc->buffers*fullsamples + samplepos/sc->sn.numchannels;
}

void S_ExtraUpdate (void)
{
	soundcardinfo_t *sc;

	if (!sound_started)
		return;

#ifdef _WIN32
	IN_Accumulate ();
#endif

	if (snd_noextraupdate.value)
		return;		// don't pollute timings

	S_RunCapture();

	for (sc = sndcardinfo; sc; sc = sc->next)
		S_Update_(sc);
}



void S_Update_(soundcardinfo_t *sc)
{
	unsigned        endtime;
	int				samps;


	if (sc->selfpainting)
		return;

	if ((snd_blocked > 0))
	{
		if (!sc->inactive_sound)
			return;
	}

// Updates DMA time
	GetSoundtime(sc);

// check to make sure that we haven't overshot
	if (sc->paintedtime < soundtime)
	{
		//Con_Printf ("S_Update_ : overflow\n");
		sc->paintedtime = soundtime;
	}

// mix ahead of current position
	endtime = soundtime + _snd_mixahead.value * sc->sn.speed;
//	samps = shm->samples >> (shm->numchannels-1);
	samps = sc->sn.samples / sc->sn.numchannels;
	if (endtime - soundtime > samps)
		endtime = soundtime + samps;

	if (sc->Restore)
		sc->Restore(sc);

	S_PaintChannels (sc, endtime);

	sc->Submit(sc);
}

/*
===============================================================================

console functions

===============================================================================
*/

void S_Play(void)
{
//	static int hash=345;
	int 	i;
	char name[256];
	sfx_t	*sfx;

	i = 1;
	while (i<Cmd_Argc())
	{
		if (!Q_strrchr(Cmd_Argv(i), '.'))
		{
			Q_strcpy(name, Cmd_Argv(i));
			Q_strcat(name, ".wav");
		}
		else
			Q_strcpy(name, Cmd_Argv(i));
		sfx = S_PrecacheSound(name);
		S_StartSound(cl.playernum[0]+1, -1, sfx, vec3_origin, 1.0, 0.0);
//		hash++;
		i++;
	}
}

void S_PlayVol(void)
{
//	static int hash=543;
	int i;
	float vol;
	char name[256];
	sfx_t	*sfx;

	i = 1;
	while (i<Cmd_Argc())
	{
		if (!Q_strrchr(Cmd_Argv(i), '.'))
		{
			Q_strcpy(name, Cmd_Argv(i));
			Q_strcat(name, ".wav");
		}
		else
			Q_strcpy(name, Cmd_Argv(i));
		sfx = S_PrecacheSound(name);
		vol = Q_atof(Cmd_Argv(i+1));
		S_StartSound(cl.playernum[0]+1, -1, sfx, vec3_origin, vol, 0.0);
//		hash;
		i+=2;
	}
}

void S_SoundList(void)
{
	int		i;
	sfx_t	*sfx;
	sfxcache_t	*sc;
	int		size, total;

	total = 0;
	for (sfx=known_sfx, i=0 ; i<num_sfx ; i++, sfx++)
	{
		sc = Cache_Check (&sfx->cache);
		if (!sc)
			continue;
		size = sc->length*sc->width*(sc->numchannels);
		total += size;
		if (sc->loopstart >= 0)
			Con_Printf ("L");
		else
			Con_Printf (" ");
		Con_Printf("(%2db) %6i : %s\n",sc->width*8,  size, sfx->name);
	}
	Con_Printf ("Total resident: %i\n", total);
}


void S_LocalSound (char *sound)
{
	sfx_t	*sfx;

	if (nosound.value)
		return;
	if (!sound_started)
		return;

	sfx = S_PrecacheSound (sound);
	if (!sfx)
	{
		Con_Printf ("S_LocalSound: can't cache %s\n", sound);
		return;
	}
	S_StartSound (-1, -1, sfx, vec3_origin, 1, 1);
}


void S_ClearPrecache (void)
{
}


void S_BeginPrecaching (void)
{
}


void S_EndPrecaching (void)
{
}













typedef struct {
	qboolean inuse;
	int id;
	sfx_t sfx;
	sfxcache_t *sfxcache;
} streaming_t;
#define MAX_RAW_SOURCES (MAX_CLIENTS+1)
streaming_t s_streamers[MAX_RAW_SOURCES];

/*
qboolean S_IsPlayingSomewhere(sfx_t *s)
{
	soundcardinfo_t *si;
	int i;
	for (si = sndcardinfo; si; si=si->next)
	{
		for (i = 0; i < MAX_CHANNELS; i++)
		if (si->channel[i].sfx == s)
			return true;
	}
	return false;
}*/
#undef free

void S_ClearRaw(void)
{
	memset(s_streamers, 0, sizeof(s_streamers));
}

//streaming audio.	//this is useful when there is one source, and the sound is to be played with no attenuation
void S_RawAudio(int sourceid, qbyte *data, int speed, int samples, int channels, int width)
{
	soundcardinfo_t *si;
	int i;
	int prepadl;
	int oldlength;
	int spare;
	int outsamples;
	float speedfactor;
	sfxcache_t *newcache;
	streaming_t *s, *free=NULL;
	for (s = s_streamers, i = 0; i < MAX_RAW_SOURCES; i++, s++)
	{
		if (!s->inuse)
		{
			free = s;
			continue;
		}
		if (s->id == sourceid)
			break;
	}
	if (!data)
	{
		if (i == MAX_RAW_SOURCES)
			return;	//wierd, it wasn't even playing.
		s->sfxcache->loopstart = -1;	//stop mixing it
		s->inuse = false;

		for (si = sndcardinfo; si; si=si->next)
		for (i = 0; i < MAX_CHANNELS; i++)
			if (si->channel[i].sfx == &s->sfx)
			{
				si->channel[i].sfx = NULL;
				break;
			}
		BZ_Free(s->sfxcache);
		return;
	}
	if (i == MAX_RAW_SOURCES)	//whoops.
	{
		if (!free)
		{
			Con_Printf("No free audio streams\n");
			return;
		}
		s = free;
		free->id = sourceid;
		free->inuse = true;
		free->sfx.cache.fake = true;
		strcpy(free->sfx.name, "");
		free->sfxcache = BZ_Malloc(sizeof(sfxcache_t));
		free->sfx.cache.data = free->sfxcache;
		free->sfxcache->speed = snd_speed;
		free->sfxcache->numchannels = channels;
		free->sfxcache->width = width;
		free->sfxcache->loopstart = -1;
		free->sfxcache->length = 0;
//		Con_Printf("Added new raw stream\n");
	}
	if (s->sfxcache->width != width || s->sfxcache->numchannels != channels || s->sfxcache->speed != snd_speed)
	{
		s->sfxcache->width = width;
		s->sfxcache->numchannels = channels;
		s->sfxcache->speed = snd_speed;
		s->sfxcache->length = 0;
//		Con_Printf("Restarting raw stream\n");
	}

	speedfactor	= (float)speed/snd_speed;
	outsamples = samples/speedfactor;

	oldlength = s->sfxcache->length;

	prepadl = 0x7fffffff;
	for (si = sndcardinfo; si; si=si->next)	//make sure all cards are playing, and that we still get a prepad if just one is.
	{
		for (i = 0; i < MAX_CHANNELS; i++)
			if (si->channel[i].sfx == &s->sfx)
			{
				if (prepadl > si->channel[i].pos)
					prepadl = si->channel[i].pos;
				break;
			}
	}

	if (prepadl == 0x7fffffff)
	{
		if (snd_show.value)
			Con_Printf("Wasn't playing\n");
		prepadl = 0;
		spare = s->sfxcache->length;
		if (spare > snd_speed)
		{
			Con_Printf("Sacrificed raw sound stream\n");
			spare = 0;	//too far out. sacrifice it all
		}
	}
	else
	{
		spare = s->sfxcache->length - prepadl;
		if (spare < 0)	//remaining samples since last time
			spare = 0;

		if (s->sfxcache->length > snd_speed*2) // more than 2 seconds of sound
		{
			Con_Printf("Sacrificed raw sound stream\n");
			spare = 0;	//too far out. sacrifice it all
		}
	}

/*	else if (spare > snd_speed)
	{
		for (si = sndcardinfo; si; si=si->next)
		{
			for (i = 0; i < MAX_CHANNELS; i++)
				if (si->channel[i].sfx == &s->sfx)
				{
					break;
				}
			if (i == MAX_CHANNELS)	//this one wasn't playing.
				S_StartSoundCard(si, -1, 0, &s->sfx, r_origin, 1, 32767, prepadl);
		}
		return;	//let the slower sound cards catch up. (This shouldn't really happen, but it's possible two cards have slightly different timings but report the same speed)
	}*/

	newcache = BZ_Malloc(sizeof(sfxcache_t) + (spare+outsamples) * (s->sfxcache->numchannels) * s->sfxcache->width);
	memcpy(newcache, s->sfxcache, sizeof(sfxcache_t));
	memcpy(newcache->data, s->sfxcache->data + prepadl * (s->sfxcache->numchannels) * s->sfxcache->width, spare * (s->sfxcache->numchannels) * s->sfxcache->width);

	BZ_Free(s->sfxcache);
	s->sfxcache = s->sfx.cache.data = newcache;

	newcache->length = spare + outsamples;

	// move this whole operation to a seperate function?
	if (channels == 1)
	{
		if (width == 2)
		{
			short sample;
			short *indata = (short *)data;
			short *outpos = (short *)(newcache->data + spare * (s->sfxcache->numchannels) * s->sfxcache->width);
			if (speedfactor==1)	//fast
			{
				while (samples--)
				{
					sample = *indata++;
					*outpos++ = sample;
				}
			}
			else
			{
				int src=0;
				int pos=0;

				while (pos++ < outsamples)
				{
					src = pos*speedfactor;
					sample = indata[src];
					*outpos++ = sample;
				}
			}
		}
		else if (width == 1)
		{
			char sample;
			char *indata = (char *)data;
			char *outpos = (char *)(newcache->data + spare * (s->sfxcache->numchannels) * s->sfxcache->width);
			if (speedfactor==1)	//fast
			{
				while (samples--)
				{
					sample = *indata++;
					*outpos++ = (int)( (unsigned char)(sample) - 128);
				}
			}
			else
			{
				int src=0;
				int pos=0;

				while (pos++ < outsamples)
				{
					src = pos*speedfactor;
					sample = indata[src];
					*outpos++ = (int)( (unsigned char)(sample) - 128);
				}
			}
		}
		else
			Sys_Error("Width isn't 2\n");
	}
	else
	{
		if (width == 2)
		{
			short sample;
			short *indata = (short *)data;
			short *outpos = (short *)((qbyte *)newcache->data + spare * (s->sfxcache->numchannels) * s->sfxcache->width);
			if (speedfactor==1)	//fast
			{
				while (samples--)
				{
					sample = *indata++;
					*outpos++ = sample;

					sample = *indata++;
					*outpos++ = sample;
				}
			}
			else
			{
				int src=0;
				int pos=0;

				while (pos++ < outsamples)
				{
					src = pos*speedfactor;
					sample = indata[src*2];
					*outpos++ = sample;

					sample = indata[src*2+1];
					*outpos++ = sample;
				}
			}
		}
		else
			Sys_Error("Width isn't 2\n");
	}

	s->sfxcache->loopstart = s->sfxcache->length;

	for (si = sndcardinfo; si; si=si->next)
	{
		for (i = 0; i < MAX_CHANNELS; i++)
			if (si->channel[i].sfx == &s->sfx)
			{
				si->channel[i].pos -= prepadl;
//				si->channel[i].end -= prepadl;
				si->channel[i].end += outsamples;

				if (si->channel[i].end < si->paintedtime)
				{
					si->channel[i].pos = 0;
					si->channel[i].end = si->paintedtime + s->sfxcache->length;
				}
				break;
			}
		if (i == MAX_CHANNELS)	//this one wasn't playing.
		{
			S_StartSoundCard(si, -1, 0, &s->sfx, r_origin, 1, 32767, 500);
//			Con_Printf("Restarted\n");
		}
	}

//	Con_Printf("Stripped %i, added %i (length %i)\n", prepadl, samples, s->sfxcache->length);
}
