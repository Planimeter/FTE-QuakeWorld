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
// sound.h -- client sound i/o functions

#ifndef __SOUND__
#define __SOUND__

// !!! if this is changed, it much be changed in asm_i386.h too !!!
#define MAXSOUNDCHANNELS 6	//on a per device basis

// !!! if this is changed, it much be changed in asm_i386.h too !!!
struct sfx_s;
/*typedef struct
{
	int left;
	int right;
} portable_samplepair_t;
*/
typedef struct
{
	int s[MAXSOUNDCHANNELS];
} portable_samplegroup_t;

typedef struct {
	int decodedlen;
	int (*decodemore) (struct sfx_s *sfx, int length);	//retrurn true when done.
	void (*abort) (struct sfx_s *sfx);	//it's not playing elsewhere. free entirly
	void *buf;
} sfxdecode_t;

typedef struct sfx_s
{
	char 	name[MAX_OSPATH];
	cache_user_t	cache;
	sfxdecode_t *decoder;
} sfx_t;

// !!! if this is changed, it much be changed in asm_i386.h too !!!
typedef struct sfxcache_s
{
	int 	length;
	int 	loopstart;
	int 	speed;
	int 	width;
	int 	numchannels;
	qbyte	data[1];		// variable sized
} sfxcache_t;

typedef struct
{
	qboolean		gamealive;
	qboolean		soundalive;
	qboolean		splitbuffer;
	int				numchannels;
	int				samples;				// mono samples in buffer
	int				submission_chunk;		// don't mix less than this #
	int				samplepos;				// in mono samples
	int				samplebits;
	int				speed;
	unsigned char	*buffer;
} dma_t;

// !!! if this is changed, it much be changed in asm_i386.h too !!!
typedef struct
{
	sfx_t	*sfx;			// sfx number
	int		vol[MAXSOUNDCHANNELS];		// 0-255 volume
	int		delay[MAXSOUNDCHANNELS];
	int		end;			// end time in global paintsamples
	int 	pos;			// sample position in sfx
	int		looping;		// where to loop, -1 = no looping
	int		entnum;			// to allow overriding a specific sound
	int		entchannel;		//int audio_fd
	vec3_t	origin;			// origin of sound effect
	vec_t	dist_mult;		// distance multiplier (attenuation/clipK)
	int		master_vol;		// 0-255 master volume
} channel_t;

typedef struct
{
	int		rate;
	int		width;
	int		numchannels;
	int		loopstart;
	int		samples;
	int		dataofs;		// chunk starts this many bytes from file start
} wavinfo_t;

struct soundcardinfo_s;
typedef struct soundcardinfo_s soundcardinfo_t;

void S_Init (void);
void S_Startup (void);
void S_Shutdown (void);
void S_StartSound (int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol,  float attenuation);
void S_StaticSound (sfx_t *sfx, vec3_t origin, float vol, float attenuation);
void S_StopSound (int entnum, int entchannel);
void S_StopAllSounds(qboolean clear);
void S_Update (vec3_t origin, vec3_t v_forward, vec3_t v_right, vec3_t v_up);
void S_ExtraUpdate (void);

sfx_t *S_PrecacheSound (char *sample);
void S_TouchSound (char *sample);
void S_ClearPrecache (void);
void S_BeginPrecaching (void);
void S_EndPrecaching (void);

void S_ClearBuffer (soundcardinfo_t *sc);

void S_PaintChannels(soundcardinfo_t *sc, int endtime);
void S_InitPaintChannels (soundcardinfo_t *sc);

void S_ShutdownCard (soundcardinfo_t *sc);
void S_StopSoundCard (soundcardinfo_t *sc, int entnum, int entchannel);

qboolean S_IsPlayingSomewhere(sfx_t *s);
void ResampleSfx (sfx_t *sfx, int inrate, int inwidth, qbyte *data);

// picks a channel based on priorities, empty slots, number of channels
channel_t *SND_PickChannel(soundcardinfo_t *sc, int entnum, int entchannel);

// spatializes a channel
void SND_Spatialize(soundcardinfo_t *sc, channel_t *ch);

// restart entire sound subsystem (doesn't flush old sounds, so make sure that happens)
void S_DoRestart (void);

void S_Restart_f (void);

//plays streaming audio
void S_RawAudio(int sourceid, qbyte *data, int speed, int samples, int channels, int width);

void CLVC_Poll (void);

void SNDVC_MicInput(qbyte *buffer, int samples, int freq, int width);

// ====================================================================
// User-setable variables
// ====================================================================

#define	MAX_CHANNELS			256
#define	MAX_DYNAMIC_CHANNELS	8


//
// Fake dma is a synchronous faking of the DMA progress used for
// isolating performance in the renderer.  The fakedma_updates is
// number of times S_Update() is called per second.
//

extern int				snd_speed;

extern qboolean 		fakedma;
extern int 			fakedma_updates;
extern vec3_t listener_origin;
extern vec3_t listener_forward;
extern vec3_t listener_right;
extern vec3_t listener_up;
extern vec_t sound_nominal_clip_dist;

extern	cvar_t loadas8bit;
extern	cvar_t bgmvolume;
extern	cvar_t volume;
extern	cvar_t snd_capture;

extern qboolean	snd_initialized;
extern cvar_t snd_usemultipledevices;

extern int		snd_blocked;

void S_LocalSound (char *s);
sfxcache_t *S_LoadSound (sfx_t *s);

typedef sfxcache_t *(*S_LoadSound_t) (sfx_t *s, qbyte *data, int datalen, int sndspeed);
qboolean S_RegisterSoundInputPlugin(S_LoadSound_t loadfnc);	//called to register additional sound input plugins

wavinfo_t GetWavinfo (char *name, qbyte *wav, int wavlength);

void S_AmbientOff (void);
void S_AmbientOn (void);


//inititalisation functions.
typedef int (*sounddriver) (soundcardinfo_t *sc, int cardnum);
extern sounddriver pDSOUND_InitCard;
extern sounddriver pALSA_InitCard;
extern sounddriver pOSS_InitCard;
extern sounddriver pSDL_InitCard;
extern sounddriver pWAV_InitCard;
extern sounddriver pAHI_InitCard;

struct soundcardinfo_s { //windows has one defined AFTER directsound
	char name[256];	//a description of the card.
	struct soundcardinfo_s *next;

//speaker orientations for spacialisation.
	float dist[MAX_CHANNELS];
	float pitch[MAX_CHANNELS];
	float yaw[MAX_CHANNELS];

//info on which sound effects are playing
	channel_t   channel[MAX_CHANNELS];
	int			total_chans;

//mixer
	volatile dma_t sn;	//why is this volatile?
	qboolean inactive_sound;	//continue mixing for this card even when the window isn't active.
	qboolean selfpainting;	//allow the sound code to call the right functions when it feels the need (not properly supported).

	int	paintedtime;	//used in the mixer
	int	oldsamplepos;	//fixme: is this still needed?
	int	buffers;	//used to keep track of buffer wraps for consistant sound

//callbacks
	void *(*Lock) (soundcardinfo_t *sc);
	void (*Unlock) (soundcardinfo_t *sc, void *buffer);
	void (*Submit) (soundcardinfo_t *sc);
	void (*Shutdown) (soundcardinfo_t *sc);
	unsigned int (*GetDMAPos) (soundcardinfo_t *sc);
	void (*SetWaterDistortion) (soundcardinfo_t *sc, qboolean underwater);
	void (*Restore) (soundcardinfo_t *sc);

//driver -specific
	void *handle;
	int snd_sent;
	int snd_completed;
	int audio_fd;
};

extern soundcardinfo_t *sndcardinfo;



#endif
