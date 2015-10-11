#include "quakedef.h"

//frankly, xaudio2 gives nothing over directsound, unless we're getting it to do all the mixing instead. which gets really messy and far too involved.
//I suppose it has a use with WINRT... although that doesn't make it useful.
//we're lazy and don't do any special threading, this makes it inferior to the directsound implementation - potentially, the callback feature could allow for slightly lower latencies.

#if defined(AVAIL_XAUDIO2) && !defined(SERVERONLY)
#include "winquake.h"
#include <xaudio2.h>

#define SDRVNAME "XAudio2"

typedef struct
{
	IXAudio2VoiceCallback cb;	//must be first. yay for fake single inheritance.

	IXAudio2 *ixa;
	IXAudio2MasteringVoice *master;
	IXAudio2SourceVoice *source;

	//contiguous block of memory, because its easier.
	qbyte *bufferstart;
	unsigned int subbuffersize;	//in samplepairs
	unsigned int buffercount;
	unsigned int bufferidx;
	unsigned int bufferavail;
} xaud_t;

static void *XAUDIO_Lock(soundcardinfo_t *sc, unsigned int *startoffset)
{
	qbyte *ret;
	xaud_t *xa = sc->handle;
	ret = xa->bufferstart;

//	*startoffset = 0;
//	ret += xa->subbuffersize * xa->bufferidx;

	return ret;
}
static void XAUDIO_Unlock(soundcardinfo_t *sc, void *buffer)
{
}
static unsigned int XAUDIO_GetDMAPos(soundcardinfo_t *sc)
{
	xaud_t *xa = sc->handle;
	unsigned int s = (xa->bufferidx+xa->bufferavail) * xa->subbuffersize * sc->sn.numchannels;
	return s;
}
static void XAUDIO_Submit(soundcardinfo_t *sc, int start, int end)
{
	xaud_t *xa = sc->handle;
	XAUDIO2_BUFFER buf;

	//determine total buffer size
	int buffersize = sc->sn.samples*sc->sn.samplebits/8;

	//determine time offsets in bytes
	start *= sc->sn.numchannels*sc->sn.samplebits/8;
	end *= sc->sn.numchannels*sc->sn.samplebits/8;

	while (start < end)
	{
		if (!xa->bufferavail)
			break;	//o.O that's not meant to happen
		memset(&buf, 0, sizeof(buf));
		buf.AudioBytes = end - start;
		if (buf.AudioBytes > xa->subbuffersize * sc->sn.numchannels*sc->sn.samplebits/8)
		{
			if (buf.AudioBytes < xa->subbuffersize * sc->sn.numchannels*sc->sn.samplebits/8)
			{	//dma code should ensure that only multiples of 'samplequeue' are processed.
				Con_Printf("XAudio2 underrun\n");
				break;	
			}
			buf.AudioBytes = xa->subbuffersize * sc->sn.numchannels*sc->sn.samplebits/8;
		}
		buf.pAudioData = xa->bufferstart + (start%buffersize);
		if ((qbyte*)buf.pAudioData + buf.AudioBytes > xa->bufferstart + buffersize)
		{	//this shouldn't ever happen either
			Con_Printf("XAudio2 overrun\n");
			break;	
		}
		IXAudio2SourceVoice_SubmitSourceBuffer(xa->source, &buf, NULL);
		xa->bufferidx += 1;
		xa->bufferavail -= 1;
		start += buf.AudioBytes;
	}
}

static void XAUDIO_Shutdown(soundcardinfo_t *sc)
{
	xaud_t *xa = sc->handle;
	//releases are allowed to block, supposedly.
	IXAudio2SourceVoice_DestroyVoice(xa->source);
	IXAudio2MasteringVoice_DestroyVoice(xa->master);
	IXAudio2_Release(xa->ixa);
	BZ_Free(xa->bufferstart);
	Z_Free(xa);
	sc->handle = NULL;
}

void WINAPI XAUDIO_CB_OnVoiceProcessingPassStart (IXAudio2VoiceCallback *ths, UINT32 BytesRequired) {}
void WINAPI XAUDIO_CB_OnVoiceProcessingPassEnd (IXAudio2VoiceCallback *ths) {}
void WINAPI XAUDIO_CB_OnStreamEnd (IXAudio2VoiceCallback *ths) {}
void WINAPI XAUDIO_CB_OnBufferStart (IXAudio2VoiceCallback *ths, void* pBufferContext) {}
void WINAPI XAUDIO_CB_OnBufferEnd (IXAudio2VoiceCallback *ths, void* pBufferContext) {xaud_t *xa = (xaud_t*)ths; S_LockMixer(); xa->bufferavail+=1; S_UnlockMixer();}
void WINAPI XAUDIO_CB_OnLoopEnd (IXAudio2VoiceCallback *ths, void* pBufferContext) {}
void WINAPI XAUDIO_CB_OnVoiceError (IXAudio2VoiceCallback *ths, void* pBufferContext, HRESULT Error) {}
static IXAudio2VoiceCallbackVtbl cbvtbl =
{
	XAUDIO_CB_OnVoiceProcessingPassStart,
    XAUDIO_CB_OnVoiceProcessingPassEnd,
    XAUDIO_CB_OnStreamEnd,
    XAUDIO_CB_OnBufferStart,
    XAUDIO_CB_OnBufferEnd,
    XAUDIO_CB_OnLoopEnd,
    XAUDIO_CB_OnVoiceError
};

static qboolean QDECL XAUDIO_InitCard(soundcardinfo_t *sc, const char *cardname)
{
#ifdef WINRT
	char *dev = NULL;
#else
	int dev = 0;
#endif
	xaud_t *xa = Z_Malloc(sizeof(*xa));
	WAVEFORMATEX wfmt;

	xa->cb.lpVtbl = &cbvtbl;

	sc->sn.numchannels = 2;

	wfmt.wFormatTag = WAVE_FORMAT_PCM;
    wfmt.nChannels = sc->sn.numchannels;
    wfmt.nSamplesPerSec = sc->sn.speed;
	wfmt.wBitsPerSample = sc->sn.samplebits;
    wfmt.nBlockAlign = wfmt.nChannels * (wfmt.wBitsPerSample / 8);
	wfmt.nAvgBytesPerSec = wfmt.nSamplesPerSec * wfmt.nBlockAlign;
    wfmt.cbSize = 0;

	sc->inactive_sound = true;
	xa->buffercount = xa->bufferavail = 3;	//submit this many straight up
	xa->subbuffersize = 256;	//number of sampleblocks per submission
	sc->samplequeue = -1;	//-1 means we're streaming, XAUDIO_GetDMAPos returns exactly as much as we want to paint to.

	sc->sn.samples = xa->buffercount * xa->subbuffersize * sc->sn.numchannels;

	xa->bufferstart = BZ_Malloc(sc->sn.samples * (sc->sn.samplebits/8));

	if (xa->bufferstart)
	{
		if (SUCCEEDED(XAudio2Create(&xa->ixa, 0, XAUDIO2_DEFAULT_PROCESSOR)))
		{
#ifdef WINRT
			if (SUCCEEDED(IXAudio2_CreateMasteringVoice(xa->ixa, &xa->master, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, dev, NULL, AudioCategory_GameEffects)))
#else
			if (cardname && *cardname)
			{
				UINT32 devs = 0;
				XAUDIO2_DEVICE_DETAILS details;
				char id[MAX_QPATH];
				if (FAILED(IXAudio2_GetDeviceCount(xa->ixa, &devs)))
					devs = 0;

				for (dev = 0; dev < devs; dev++)
				{
					if (SUCCEEDED(IXAudio2_GetDeviceDetails(xa->ixa, dev, &details)))
					{
						narrowen(id, sizeof(id), details.DeviceID);

						if (!strcmp(id, cardname))
							break;
					}
				}
				if (dev == devs)
					dev = ~0;	//something invalid.
			}

			/*
			//FIXME: correct the details to match the hardware
			*/


			if (dev != ~0 && SUCCEEDED(IXAudio2_CreateMasteringVoice(xa->ixa, &xa->master, XAUDIO2_DEFAULT_CHANNELS, XAUDIO2_DEFAULT_SAMPLERATE, 0, dev, NULL)))
#endif
			{
				//egads
				XAUDIO2_VOICE_SENDS vs;
				XAUDIO2_SEND_DESCRIPTOR sd[1];
				vs.SendCount = 1;
				vs.pSends = sd;
				sd[0].Flags = 0;
				sd[0].pOutputVoice = (IXAudio2Voice*)xa->master;

				if (SUCCEEDED(IXAudio2_CreateSourceVoice(xa->ixa, &xa->source, &wfmt, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &xa->cb, &vs, NULL)))
				{
					sc->handle = xa;
					sc->GetDMAPos = XAUDIO_GetDMAPos;
					sc->Lock = XAUDIO_Lock;
					sc->Unlock = XAUDIO_Unlock;
					sc->Submit = XAUDIO_Submit;
					sc->Shutdown = XAUDIO_Shutdown;

					IXAudio2SourceVoice_Start(xa->source, 0, XAUDIO2_COMMIT_NOW);
					return true;
				}
				IXAudio2MasteringVoice_DestroyVoice(xa->master);
			}
			IXAudio2_Release(xa->ixa);
		}
		BZ_Free(xa->bufferstart);
	}
	Z_Free(xa);
	return false;
}

qboolean QDECL XAUDIO_Enumerate(void (QDECL *callback) (const char *drivername, const char *devicecode, const char *readablename))
{
	IXAudio2 *ixa;
#ifndef WINRT
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	//winrt provides no enumeration mechanism.
	if (SUCCEEDED(XAudio2Create(&ixa, 0, XAUDIO2_DEFAULT_PROCESSOR)))
	{
		UINT32 devs = 0, i;
		XAUDIO2_DEVICE_DETAILS details;
		char id[MAX_QPATH], name[MAX_QPATH];
		if (FAILED(IXAudio2_GetDeviceCount(ixa, &devs)))
			devs = 0;

		strcpy(name, "XA2:");

		for (i = 0; i < devs; i++)
		{
			if (SUCCEEDED(IXAudio2_GetDeviceDetails(ixa, i, &details)))
			{
				narrowen(id, sizeof(id), details.DeviceID);
				narrowen(name+4, sizeof(name)-4, details.DisplayName);

				callback(SDRVNAME, id, name);
			}
		}
		IXAudio2_Release(ixa);
		return true;
	}
#endif
	return false;
}

sounddriver_t XAUDIO2_Output =
{
	SDRVNAME,
	XAUDIO_InitCard,
	XAUDIO_Enumerate
};
#endif
