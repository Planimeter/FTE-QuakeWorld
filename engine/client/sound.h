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

#define MAXSOUNDCHANNELS 8	//on a per device basis

//pitch shifting can 
#define ssamplepos_t qintptr_t
#define usamplepos_t quintptr_t
#define PITCHSHIFT 6	/*max audio file length = (1<<32>>PITCHSHIFT)/KHZ*/

struct sfx_s;
typedef struct
{
	int s[MAXSOUNDCHANNELS];
} portable_samplegroup_t;

typedef struct {
	struct sfxcache_s *(*decodedata) (struct sfx_s *sfx, struct sfxcache_s *buf, ssamplepos_t start, int length);	//return true when done.
	float (*querydata) (struct sfx_s *sfx, struct sfxcache_s *buf);	//reports length + original format info without actually decoding anything.
	void (*ended) (struct sfx_s *sfx);	//sound stopped playing and is now silent (allow rewinding or something).
	void (*purge) (struct sfx_s *sfx);	//sound is being purged from memory. destroy everything.
	void *buf;
} sfxdecode_t;

enum
{
	SLS_NOTLOADED,	//not tried to load it
	SLS_LOADING,	//loading it on a worker thread.
	SLS_LOADED,		//currently in memory and usable.
	SLS_FAILED		//already tried to load it. it won't work. not found, invalid format, etc
};
typedef struct sfx_s
{
	char 	name[MAX_OSPATH];
	sfxdecode_t		decoder;

	int loadstate; //no more super-spammy
	qboolean touched:1; //if the sound is still relevent

#ifdef AVAIL_OPENAL
	unsigned int	openal_buffer;
#endif
} sfx_t;

// !!! if this is changed, it much be changed in asm_i386.h too !!!
typedef struct sfxcache_s
{
	usamplepos_t length;	//sample count
	int loopstart;	//-1 or sample index to begin looping at once the sample ends
	unsigned int speed;
	unsigned int width;
	unsigned int numchannels;
	usamplepos_t soundoffset;	//byte index into the sound
	qbyte	*data;		// variable sized
} sfxcache_t;

typedef struct
{
//	qboolean		gamealive;
//	qboolean		soundalive;
//	qboolean		splitbuffer;
	int				numchannels;			// this many samples per frame
	int				samples;				// mono samples in buffer (individual, non grouped)
//	int				submission_chunk;		// don't mix less than this #
	int				samplepos;				// in mono samples
	int				samplebits;
	int				speed;					// this many frames per second
	unsigned char	*buffer;				// pointer to mixed pcm buffer (not directly used by mixer)
} dma_t;

#define CF_RELIABLE		1	// serverside only. yeah, evil. screw you.
#define CF_FORCELOOP	2	// forces looping. set on static sounds.
#define CF_NOSPACIALISE 4	// these sounds are played at a fixed volume in both speakers, but still gets quieter with distance.
//#define CF_PAUSED		8	// rate = 0. or something.
#define CF_ABSVOLUME	16	// ignores volume cvar.

#define CF_UNICAST		256 // serverside only. the sound is sent to msg_entity only.
#define CF_AUTOSOUND	512	// generated from q2 entities, which avoids breaking regular sounds, using it outside the sound system will probably break things.

typedef struct
{
	sfx_t	*sfx;			// sfx number
	int		vol[MAXSOUNDCHANNELS];		// volume, .8 fixed point.
	ssamplepos_t pos;		// sample position in sfx, <0 means delay sound start (shifted up by 8)
	int     rate;			// 24.8 fixed point rate scaling
	int		flags;			// cf_ flags
	int		entnum;			// to allow overriding a specific sound
	int		entchannel;		// to avoid overriding a specific sound too easily
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
void S_Shutdown (qboolean final);
float S_GetSoundTime(int entnum, int entchannel);
void S_StartSound (int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol, float attenuation, float timeofs, float pitchadj, unsigned int flags);
float S_UpdateSound(int entnum, int entchannel, sfx_t *sfx, vec3_t origin, float fvol, float attenuation, float timeofs, float pitchadj, unsigned int flags);
void S_StaticSound (sfx_t *sfx, vec3_t origin, float vol, float attenuation);
void S_StopSound (int entnum, int entchannel);
void S_StopAllSounds(qboolean clear);
void S_UpdateListener(int seat, vec3_t origin, vec3_t forward, vec3_t right, vec3_t up, qboolean underwater);
void S_GetListenerInfo(int seat, float *origin, float *forward, float *right, float *up);
void S_Update (void);
void S_ExtraUpdate (void);
void S_MixerThread(soundcardinfo_t *sc);
void S_Purge(qboolean retaintouched);

void S_LockMixer(void);
void S_UnlockMixer(void);

qboolean S_HaveOutput(void);

void S_Music_Clear(sfx_t *onlyifsample);
void S_Music_Seek(float time);
qboolean S_GetMusicInfo(int musicchannel, float *time, float *duration);
qboolean S_Music_Playing(int musicchannel);
float Media_CrossFade(int musicchanel, float vol, float time);	//queries the volume we're meant to be playing (checks for fade out). -1 for no more, otherwise returns vol.
char *Media_NextTrack(int musicchanel, float *time);	//queries the track we're meant to be playing now.

sfx_t *S_FindName (const char *name, qboolean create);
sfx_t *S_PrecacheSound (const char *sample);
void S_TouchSound (char *sample);
void S_UntouchAll(void);
void S_ClearPrecache (void);
void S_BeginPrecaching (void);
void S_EndPrecaching (void);

void S_PaintChannels(soundcardinfo_t *sc, int endtime);
void S_InitPaintChannels (soundcardinfo_t *sc);

soundcardinfo_t *S_SetupDeviceSeat(char *driver, char *device, int seat);
void S_ShutdownCard (soundcardinfo_t *sc);

void S_DefaultSpeakerConfiguration(soundcardinfo_t *sc);
void S_ResetFailedLoad(void);

#ifdef PEXT2_VOICECHAT
void S_Voip_Parse(void);
#endif
#ifdef VOICECHAT
extern cvar_t snd_voip_showmeter;
void S_Voip_Transmit(unsigned char clc, sizebuf_t *buf);
void S_Voip_MapChange(void);
int S_Voip_Loudness(qboolean ignorevad);	//-1 for not capturing, otherwise between 0 and 100
qboolean S_Voip_Speaking(unsigned int plno);
void S_Voip_Ignore(unsigned int plno, qboolean ignore);
#else
#define S_Voip_Loudness() -1
#define S_Voip_Speaking(p) false
#define S_Voip_Ignore(p,s)
#endif

qboolean S_IsPlayingSomewhere(sfx_t *s);
qboolean ResampleSfx (sfx_t *sfx, int inrate, int inchannels, int inwidth, int insamps, int inloopstart, qbyte *data);

// picks a channel based on priorities, empty slots, number of channels
channel_t *SND_PickChannel(soundcardinfo_t *sc, int entnum, int entchannel);

void SND_ResampleStream (void *in, int inrate, int inwidth, int inchannels, int insamps, void *out, int outrate, int outwidth, int outchannels, int resampstyle);

// restart entire sound subsystem (doesn't flush old sounds, so make sure that happens)
void S_DoRestart (qboolean onlyifneeded);

void S_Restart_f (void);

//plays streaming audio
void S_RawAudio(int sourceid, qbyte *data, int speed, int samples, int channels, int width, float volume);

void CLVC_Poll (void);

void SNDVC_MicInput(qbyte *buffer, int samples, int freq, int width);



#ifdef AVAIL_OPENAL
void OpenAL_CvarInit(void);
#endif


// ====================================================================
// User-setable variables
// ====================================================================

#define	MAX_CHANNELS			1024/*tracked sounds (including statics)*/
#define	MAX_DYNAMIC_CHANNELS	64	/*playing sounds (identical ones merge)*/


#define NUM_MUSICS				1

#define AMBIENT_FIRST 0
#define AMBIENT_STOP NUM_AMBIENTS
#define MUSIC_FIRST AMBIENT_STOP
#define MUSIC_STOP (MUSIC_FIRST + NUM_MUSICS)
#define DYNAMIC_FIRST MUSIC_STOP
#define DYNAMIC_STOP (DYNAMIC_FIRST + MAX_DYNAMIC_CHANNELS)

//
// Fake dma is a synchronous faking of the DMA progress used for
// isolating performance in the renderer.  The fakedma_updates is
// number of times S_Update() is called per second.
//

extern int				snd_speed;

extern vec_t sound_nominal_clip_dist;

extern	cvar_t loadas8bit;
extern	cvar_t bgmvolume;
extern	cvar_t volume;
extern	cvar_t snd_capture;

extern float voicevolumemod;

extern qboolean	snd_initialized;
extern cvar_t snd_mixerthread;

extern int		snd_blocked;

void S_LocalSound (const char *s);
qboolean S_LoadSound (sfx_t *s);

typedef qboolean (*S_LoadSound_t) (sfx_t *s, qbyte *data, int datalen, int sndspeed);
qboolean S_RegisterSoundInputPlugin(S_LoadSound_t loadfnc);	//called to register additional sound input plugins

wavinfo_t GetWavinfo (char *name, qbyte *wav, int wavlength);

void S_AmbientOff (void);
void S_AmbientOn (void);


//inititalisation functions.
typedef struct
{
	const char *name;	//must be a single token, with no :
	qboolean (QDECL *InitCard) (soundcardinfo_t *sc, const char *cardname);	//NULL for default device.
	qboolean (QDECL *Enumerate) (void (QDECL *callback) (const char *drivername, const char *devicecode, const char *readablename));
} sounddriver_t;
typedef int (*sounddriver) (soundcardinfo_t *sc, int cardnum);
extern sounddriver pOPENAL_InitCard;
extern sounddriver pDSOUND_InitCard;
extern sounddriver pALSA_InitCard;
extern sounddriver pSNDIO_InitCard;
extern sounddriver pOSS_InitCard;
extern sounddriver pSDL_InitCard;
extern sounddriver pWAV_InitCard;
extern sounddriver pAHI_InitCard;

struct soundcardinfo_s { //windows has one defined AFTER directsound
	char name[256];	//a description of the card.
	char guid[256];	//device name as detected (so input code can create sound devices without bugging out too much)
	struct soundcardinfo_s *next;
	int seat;

//speaker orientations for spacialisation.
	float dist[MAXSOUNDCHANNELS];

	vec3_t speakerdir[MAXSOUNDCHANNELS];

//info on which sound effects are playing
	//FIXME: use a linked list
	channel_t   channel[MAX_CHANNELS];
	int			total_chans;

	float	ambientlevels[NUM_AMBIENTS];	//we use a float instead of the channel's int volume value to avoid framerate dependancies with slow transitions.

//mixer
	volatile dma_t sn;	//why is this volatile?
	qboolean inactive_sound;	//continue mixing for this card even when the window isn't active.
	qboolean selfpainting;	//allow the sound code to call the right functions when it feels the need (not properly supported).

	int	paintedtime;	//used in the mixer as last-written pos (in frames)
	int	oldsamplepos;	//this is used to track buffer wraps
	int	buffers;	//used to keep track of how many buffer wraps for consistant sound
	int	samplequeue;	//this is the number of samples the device can enqueue. if set, DMAPos returns the write point (rather than hardware read point) (in samplepairs).

//callbacks
	void *(*Lock) (soundcardinfo_t *sc, unsigned int *startoffset);	//grab a pointer to the hardware ringbuffer or whatever. startoffset is the starting offset. you can set it to 0 and bump the start offset if you need.
	void (*Unlock) (soundcardinfo_t *sc, void *buffer);				//release the hardware ringbuffer memory
	void (*Submit) (soundcardinfo_t *sc, int start, int end);		//if the ringbuffer is emulated, this is where you should push it to the device.
	void (*Shutdown) (soundcardinfo_t *sc);							//kill the device
	unsigned int (*GetDMAPos) (soundcardinfo_t *sc);				//get the current point that the hardware is reading from (the return value should not wrap, at least not very often)
	void (*SetWaterDistortion) (soundcardinfo_t *sc, qboolean underwater);	//if you have eax enabled, change the environment. fixme. generally this is a stub. optional.
	void (*Restore) (soundcardinfo_t *sc);							//called before lock/unlock/lock/unlock/submit. optional
	void (*ChannelUpdate) (soundcardinfo_t *sc, channel_t *channel, unsigned int schanged);	//properties of a sound effect changed. this is to notify hardware mixers. optional.
	void (*ListenerUpdate) (soundcardinfo_t *sc, vec3_t origin, vec3_t forward, vec3_t right, vec3_t up, vec3_t velocity);	//player moved or something. this is to notify hardware mixers. optional.

//driver-specific - if you need more stuff, you should just shove it in the handle pointer
	void *thread;
	void *handle;
	int snd_sent;
	int snd_completed;
	int audio_fd;
};

extern soundcardinfo_t *sndcardinfo;

typedef struct
{
	int apiver;
	char *drivername;
	qboolean (QDECL *Enumerate) (void (QDECL *callback) (const char *drivername, const char *devicecode, const char *readablename));
	void *(QDECL *Init) (int samplerate, const char *device);			/*create a new context*/
	void (QDECL *Start) (void *ctx);		/*begin grabbing new data, old data is potentially flushed*/
	unsigned int (QDECL *Update) (void *ctx, unsigned char *buffer, unsigned int minbytes, unsigned int maxbytes);	/*grab the data into a different buffer*/
	void (QDECL *Stop) (void *ctx);		/*stop grabbing new data, old data may remain*/
	void (QDECL *Shutdown) (void *ctx);	/*destroy everything*/
} snd_capture_driver_t;

#endif
