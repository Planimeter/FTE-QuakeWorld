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
// snd_mem.c: sound caching

#include "quakedef.h"

#include "winquake.h"
#include "fs.h"

int			cache_full_cycle;

qbyte *S_Alloc (int size);

#define LINEARUPSCALE(in, inrate, insamps, out, outrate, outlshift, outrshift) \
	{ \
		scale = inrate / (double)outrate; \
		infrac = floor(scale * 65536); \
		outsamps = insamps / scale; \
		inaccum = 0; \
		outnlsamps = floor(1.0 / scale); \
		outsamps -= outnlsamps; \
	\
		while (outsamps) \
		{ \
			*out = ((0xFFFF - inaccum)*in[0] + inaccum*in[1]) >> (16 - outlshift + outrshift); \
			inaccum += infrac; \
			in += (inaccum >> 16); \
			inaccum &= 0xFFFF; \
			out++; \
			outsamps--; \
		} \
		while (outnlsamps) \
		{ \
			*out = (*in >> outrshift) << outlshift; \
			out++; \
			outnlsamps--; \
		} \
	}

#define LINEARUPSCALESTEREO(in, inrate, insamps, out, outrate, outlshift, outrshift) \
	{ \
		scale = inrate / (double)outrate; \
		infrac = floor(scale * 65536); \
		outsamps = insamps / scale; \
		inaccum = 0; \
		outnlsamps = floor(1.0 / scale); \
		outsamps -= outnlsamps; \
	\
		while (outsamps) \
		{ \
			out[0] = ((0xFFFF - inaccum)*in[0] + inaccum*in[2]) >> (16 - outlshift + outrshift); \
			out[1] = ((0xFFFF - inaccum)*in[1] + inaccum*in[3]) >> (16 - outlshift + outrshift); \
			inaccum += infrac; \
			in += (inaccum >> 16) * 2; \
			inaccum &= 0xFFFF; \
			out += 2; \
			outsamps--; \
		} \
		while (outnlsamps) \
		{ \
			out[0] = (in[0] >> outrshift) << outlshift; \
			out[1] = (in[1] >> outrshift) << outlshift; \
			out += 2; \
			outnlsamps--; \
		} \
	}

#define LINEARUPSCALESTEREOTOMONO(in, inrate, insamps, out, outrate, outlshift, outrshift) \
	{ \
		scale = inrate / (double)outrate; \
		infrac = floor(scale * 65536); \
		outsamps = insamps / scale; \
		inaccum = 0; \
		outnlsamps = floor(1.0 / scale); \
		outsamps -= outnlsamps; \
	\
		while (outsamps) \
		{ \
			*out = ((((0xFFFF - inaccum)*in[0] + inaccum*in[2]) >> (16 - outlshift + outrshift)) + \
				(((0xFFFF - inaccum)*in[1] + inaccum*in[3]) >> (16 - outlshift + outrshift))) >> 1; \
			inaccum += infrac; \
			in += (inaccum >> 16) * 2; \
			inaccum &= 0xFFFF; \
			out++; \
			outsamps--; \
		} \
		while (outnlsamps) \
		{ \
			out[0] = (((in[0] >> outrshift) << outlshift) + ((in[1] >> outrshift) << outlshift)) >> 1; \
			out++; \
			outnlsamps--; \
		} \
	}

#define LINEARDOWNSCALE(in, inrate, insamps, out, outrate, outlshift, outrshift) \
	{ \
		scale = outrate / (double)inrate; \
		infrac = floor(scale * 65536); \
		inaccum = 0; \
		insamps--; \
		outsampleft = 0; \
	\
		while (insamps) \
		{ \
			inaccum += infrac; \
			if (inaccum >> 16) \
			{ \
				inaccum &= 0xFFFF; \
				outsampleft += (infrac - inaccum) * (*in); \
				*out = outsampleft >> (16 - outlshift + outrshift); \
				out++; \
				outsampleft = inaccum * (*in); \
			} \
			else \
				outsampleft += infrac * (*in); \
			in++; \
			insamps--; \
		} \
		outsampleft += (0xFFFF - inaccum) * (*in);\
		*out = outsampleft >> (16 - outlshift + outrshift); \
	}

#define LINEARDOWNSCALESTEREO(in, inrate, insamps, out, outrate, outlshift, outrshift) \
	{ \
		scale = outrate / (double)inrate; \
		infrac = floor(scale * 65536); \
		inaccum = 0; \
		insamps--; \
		outsampleft = 0; \
		outsampright = 0; \
	\
		while (insamps) \
		{ \
			inaccum += infrac; \
			if (inaccum >> 16) \
			{ \
				inaccum &= 0xFFFF; \
				outsampleft += (infrac - inaccum) * in[0]; \
				outsampright += (infrac - inaccum) * in[1]; \
				out[0] = outsampleft >> (16 - outlshift + outrshift); \
				out[1] = outsampright >> (16 - outlshift + outrshift); \
				out += 2; \
				outsampleft = inaccum * in[0]; \
				outsampright = inaccum * in[1]; \
			} \
			else \
			{ \
				outsampleft += infrac * in[0]; \
				outsampright += infrac * in[1]; \
			} \
			in += 2; \
			insamps--; \
		} \
		outsampleft += (0xFFFF - inaccum) * in[0];\
		outsampright += (0xFFFF - inaccum) * in[1];\
		out[0] = outsampleft >> (16 - outlshift + outrshift); \
		out[1] = outsampright >> (16 - outlshift + outrshift); \
	}

#define LINEARDOWNSCALESTEREOTOMONO(in, inrate, insamps, out, outrate, outlshift, outrshift) \
	{ \
		scale = outrate / (double)inrate; \
		infrac = floor(scale * 65536); \
		inaccum = 0; \
		insamps--; \
		outsampleft = 0; \
	\
		while (insamps) \
		{ \
			inaccum += infrac; \
			if (inaccum >> 16) \
			{ \
				inaccum &= 0xFFFF; \
				outsampleft += (infrac - inaccum) * ((in[0] + in[1]) >> 1); \
				*out = outsampleft >> (16 - outlshift + outrshift); \
				out++; \
				outsampleft = inaccum * ((in[0] + in[1]) >> 1); \
			} \
			else \
				outsampleft += infrac * ((in[0] + in[1]) >> 1); \
			in += 2; \
			insamps--; \
		} \
		outsampleft += (0xFFFF - inaccum) * ((in[0] + in[1]) >> 1);\
		*out = outsampleft >> (16 - outlshift + outrshift); \
	}

#define STANDARDRESCALE(in, inrate, insamps, out, outrate, outlshift, outrshift) \
	{ \
		scale = inrate / (double)outrate; \
		infrac = floor(scale * 65536); \
		outsamps = insamps / scale; \
		inaccum = 0; \
	\
		while (outsamps) \
		{ \
			*out = (*in >> outrshift) << outlshift; \
			inaccum += infrac; \
			in += (inaccum >> 16); \
			inaccum &= 0xFFFF; \
			out++; \
			outsamps--; \
		} \
	}

#define STANDARDRESCALESTEREO(in, inrate, insamps, out, outrate, outlshift, outrshift) \
	{ \
		scale = inrate / (double)outrate; \
		infrac = floor(scale * 65536); \
		outsamps = insamps / scale; \
		inaccum = 0; \
	\
		while (outsamps) \
		{ \
			out[0] = (in[0] >> outrshift) << outlshift; \
			out[1] = (in[1] >> outrshift) << outlshift; \
			inaccum += infrac; \
			in += (inaccum >> 16) * 2; \
			inaccum &= 0xFFFF; \
			out += 2; \
			outsamps--; \
		} \
	}

#define STANDARDRESCALESTEREOTOMONO(in, inrate, insamps, out, outrate, outlshift, outrshift) \
	{ \
		scale = inrate / (double)outrate; \
		infrac = floor(scale * 65536); \
		outsamps = insamps / scale; \
		inaccum = 0; \
	\
		while (outsamps) \
		{ \
			out[0] = (((in[0] >> outrshift) << outlshift) + ((in[1] >> outrshift) << outlshift)) >> 1; \
			inaccum += infrac; \
			in += (inaccum >> 16) * 2; \
			inaccum &= 0xFFFF; \
			out++; \
			outsamps--; \
		} \
	}

#define QUICKCONVERT(in, insamps, out, outlshift, outrshift) \
	{ \
		while (insamps) \
		{ \
			*out = (*in >> outrshift) << outlshift; \
			out++; \
			in++; \
			insamps--; \
		} \
	}

#define QUICKCONVERTSTEREOTOMONO(in, insamps, out, outlshift, outrshift) \
	{ \
		while (insamps) \
		{ \
			*out = (((in[0] >> outrshift) << outlshift) + ((in[1] >> outrshift) << outlshift)) >> 1; \
			out++; \
			in += 2; \
			insamps--; \
		} \
	}

// SND_ResampleStream: takes a sound stream and converts with given parameters. Limited to
// 8-16-bit signed conversions and mono-to-mono/stereo-to-stereo conversions.
// Not an in-place algorithm.
void SND_ResampleStream (void *in, int inrate, int inwidth, int inchannels, int insamps, void *out, int outrate, int outwidth, int outchannels, int resampstyle)
{
	double scale;
	signed char *in8 = (signed char *)in;
	short *in16 = (short *)in;
	signed char *out8 = (signed char *)out;
	short *out16 = (short *)out;
	int outsamps, outnlsamps, outsampleft, outsampright;
	int infrac, inaccum;

	if (insamps <= 0)
		return;

	if (inchannels == outchannels && inwidth == outwidth && inrate == outrate)
	{
		memcpy(out, in, inwidth*insamps*inchannels);
		return;
	}

	if (inchannels == 1 && outchannels == 1)
	{
		if (inwidth == 1)
		{
			if (outwidth == 1)
			{
				if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALE(in8, inrate, insamps, out8, outrate, 0, 0)
					else
						STANDARDRESCALE(in8, inrate, insamps, out8, outrate, 0, 0)
				}
				else // downsample
				{
					if (resampstyle > 1)
						LINEARDOWNSCALE(in8, inrate, insamps, out8, outrate, 0, 0)
					else
						STANDARDRESCALE(in8, inrate, insamps, out8, outrate, 0, 0)
				}
				return;
			}
			else
			{
				if (inrate == outrate) // quick convert
					QUICKCONVERT(in8, insamps, out16, 8, 0)
				else if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALE(in8, inrate, insamps, out16, outrate, 8, 0)
					else
						STANDARDRESCALE(in8, inrate, insamps, out16, outrate, 8, 0)
				}
				else // downsample
				{
					if (resampstyle > 1)
						LINEARDOWNSCALE(in8, inrate, insamps, out16, outrate, 8, 0)
					else
						STANDARDRESCALE(in8, inrate, insamps, out16, outrate, 8, 0)
				}
				return;
			}
		}
		else // 16-bit
		{
			if (outwidth == 2)
			{
				if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALE(in16, inrate, insamps, out16, outrate, 0, 0)
					else
						STANDARDRESCALE(in16, inrate, insamps, out16, outrate, 0, 0)
				}
				else // downsample
				{
					if (resampstyle > 1)
						LINEARDOWNSCALE(in16, inrate, insamps, out16, outrate, 0, 0)
					else
						STANDARDRESCALE(in16, inrate, insamps, out16, outrate, 0, 0)
				}
				return;
			}
			else
			{
				if (inrate == outrate) // quick convert
					QUICKCONVERT(in16, insamps, out8, 0, 8)
				else if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALE(in16, inrate, insamps, out8, outrate, 0, 8)
					else
						STANDARDRESCALE(in16, inrate, insamps, out8, outrate, 0, 8)
				}
				else // downsample
				{
					if (resampstyle > 1)
						LINEARDOWNSCALE(in16, inrate, insamps, out8, outrate, 0, 8)
					else
						STANDARDRESCALE(in16, inrate, insamps, out8, outrate, 0, 8)
				}
				return;
			}
		}
	}
	else if (outchannels == 2 && inchannels == 2)
	{
		if (inwidth == 1)
		{
			if (outwidth == 1)
			{
				if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALESTEREO(in8, inrate, insamps, out8, outrate, 0, 0)
					else
						STANDARDRESCALESTEREO(in8, inrate, insamps, out8, outrate, 0, 0)
				}
				else // downsample
				{
					if (resampstyle > 1)
						LINEARDOWNSCALESTEREO(in8, inrate, insamps, out8, outrate, 0, 0)
					else
						STANDARDRESCALESTEREO(in8, inrate, insamps, out8, outrate, 0, 0)
				}
			}
			else
			{
				if (inrate == outrate) // quick convert
				{
					insamps *= 2;
					QUICKCONVERT(in8, insamps, out16, 8, 0)
				}
				else if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALESTEREO(in8, inrate, insamps, out16, outrate, 8, 0)
					else
						STANDARDRESCALESTEREO(in8, inrate, insamps, out16, outrate, 8, 0)
				}
				else // downsample
				{
					if (resampstyle > 1)
						LINEARDOWNSCALESTEREO(in8, inrate, insamps, out16, outrate, 8, 0)
					else
						STANDARDRESCALESTEREO(in8, inrate, insamps, out16, outrate, 8, 0)
				}
			}
		}
		else // 16-bit
		{
			if (outwidth == 2)
			{
				if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALESTEREO(in16, inrate, insamps, out16, outrate, 0, 0)
					else
						STANDARDRESCALESTEREO(in16, inrate, insamps, out16, outrate, 0, 0)
				}
				else // downsample
				{
					if (resampstyle > 1)
						LINEARDOWNSCALESTEREO(in16, inrate, insamps, out16, outrate, 0, 0)
					else
						STANDARDRESCALESTEREO(in16, inrate, insamps, out16, outrate, 0, 0)
				}
			}
			else
			{
				if (inrate == outrate) // quick convert
				{
					insamps *= 2;
					QUICKCONVERT(in16, insamps, out8, 0, 8)
				}
				else if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALESTEREO(in16, inrate, insamps, out8, outrate, 0, 8)
					else
						STANDARDRESCALESTEREO(in16, inrate, insamps, out8, outrate, 0, 8)
				}
				else // downsample
				{
					if (resampstyle > 1)
						LINEARDOWNSCALESTEREO(in16, inrate, insamps, out8, outrate, 0, 8)
					else
						STANDARDRESCALESTEREO(in16, inrate, insamps, out8, outrate, 0, 8)
				}
			}
		}
	}
#if 0
	else if (outchannels == 1 && inchannels == 2)
	{
		if (inwidth == 1)
		{
			if (outwidth == 1)
			{
				if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALESTEREOTOMONO(in8, inrate, insamps, out8, outrate, 0, 0)
					else
						STANDARDRESCALESTEREOTOMONO(in8, inrate, insamps, out8, outrate, 0, 0)
				}
				else // downsample
					STANDARDRESCALESTEREOTOMONO(in8, inrate, insamps, out8, outrate, 0, 0)
			}
			else
			{
				if (inrate == outrate) // quick convert
					QUICKCONVERTSTEREOTOMONO(in8, insamps, out16, 8, 0)
				else if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALESTEREOTOMONO(in8, inrate, insamps, out16, outrate, 8, 0)
					else
						STANDARDRESCALESTEREOTOMONO(in8, inrate, insamps, out16, outrate, 8, 0)
				}
				else // downsample
					STANDARDRESCALESTEREOTOMONO(in8, inrate, insamps, out16, outrate, 8, 0)
			}
		}
		else // 16-bit
		{
			if (outwidth == 2)
			{
				if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALESTEREOTOMONO(in16, inrate, insamps, out16, outrate, 0, 0)
					else
						STANDARDRESCALESTEREOTOMONO(in16, inrate, insamps, out16, outrate, 0, 0)
				}
				else // downsample
					STANDARDRESCALESTEREOTOMONO(in16, inrate, insamps, out16, outrate, 0, 0)
			}
			else
			{
				if (inrate == outrate) // quick convert
					QUICKCONVERTSTEREOTOMONO(in16, insamps, out8, 0, 8)
				else if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALESTEREOTOMONO(in16, inrate, insamps, out8, outrate, 0, 8)
					else
						STANDARDRESCALESTEREOTOMONO(in16, inrate, insamps, out8, outrate, 0, 8)
				}
				else // downsample
					STANDARDRESCALESTEREOTOMONO(in16, inrate, insamps, out8, outrate, 0, 8)
			}
		}
	}
#endif
}

/*
================
ResampleSfx
================
*/
qboolean ResampleSfx (sfx_t *sfx, int inrate, int inchannels, int inwidth, int insamps, int inloopstart, qbyte *data)
{
	extern cvar_t snd_linearresample;
	double scale;
	sfxcache_t	*sc;
	int outsamps;
	int len;
	int outwidth;

	scale = snd_speed / (double)inrate;
	outsamps = insamps * scale;
	if (loadas8bit.ival < 0)
		outwidth = 2;
	else if (loadas8bit.ival)
		outwidth = 1;
	else
		outwidth = inwidth;
	len = outsamps * outwidth * inchannels;

	sfx->decoder.buf = sc = BZ_Malloc(len + sizeof(sfxcache_t));
	if (!sc)
	{
		return false;
	}

	sc->numchannels = inchannels;
	sc->width = outwidth;
	sc->speed = snd_speed;
	sc->length = outsamps;
	sc->soundoffset = 0;
	sc->data = (qbyte*)(sc+1);
	if (inloopstart == -1)
		sc->loopstart = inloopstart;
	else
		sc->loopstart = inloopstart * scale;

	SND_ResampleStream (data,
		inrate,
		inwidth,
		inchannels,
		insamps,
		sc->data,
		sc->speed,
		sc->width,
		sc->numchannels,
		snd_linearresample.ival);

	return true;
}

//=============================================================================
#ifdef DOOMWADS
#define DSPK_RATE 140
#define DSPK_BASE 170.0
#define DSPK_EXP 0.0433

/*
sfxcache_t *S_LoadDoomSpeakerSound (sfx_t *s, qbyte *data, int datalen, int sndspeed)
{
	sfxcache_t	*sc;

	// format data from Unofficial Doom Specs v1.6
	unsigned short *dataus;
	int samples, len, inrate, inaccum;
	qbyte *outdata;
	qbyte towrite;
	double timeraccum, timerfreq;

	if (datalen < 4)
		return NULL;

	dataus = (unsigned short*)data;

	if (LittleShort(dataus[0]) != 0)
		return NULL;

	samples = LittleShort(dataus[1]);

	data += 4;
	datalen -= 4;

	if (datalen != samples)
		return NULL;

	len = (int)((double)samples * (double)snd_speed / DSPK_RATE);

	sc = Cache_Alloc (&s->cache, len + sizeof(sfxcache_t), s->name);
	if (!sc)
	{
		return NULL;
	}

	sc->length = len;
	sc->loopstart = -1;
	sc->numchannels = 1;
	sc->width = 1;
	sc->speed = snd_speed;

	timeraccum = 0;
	outdata = sc->data;
	towrite = 0x40;
	inrate = (int)((double)snd_speed / DSPK_RATE);
	inaccum = inrate;
	if (*data)
		timerfreq = DSPK_BASE * pow((double)2.0, DSPK_EXP * (*data));
	else
		timerfreq = 0;

	while (len > 0)
	{
		timeraccum += timerfreq;
		if (timeraccum > (float)snd_speed)
		{
			towrite ^= 0xFF; // swap speaker component
			timeraccum -= (float)snd_speed;
		}

		inaccum--;
		if (!inaccum)
		{
			data++;
			if (*data)
				timerfreq = DSPK_BASE * pow((double)2.0, DSPK_EXP * (*data));
			inaccum = inrate;
		}
		*outdata = towrite;
		outdata++;
		len--;
	}

	return sc;
}
*/
qboolean S_LoadDoomSound (sfx_t *s, qbyte *data, int datalen, int sndspeed)
{
	// format data from Unofficial Doom Specs v1.6
	unsigned short *dataus;
	int samples, rate;

	if (datalen < 8)
		return false;

	dataus = (unsigned short*)data;

	if (LittleShort(dataus[0]) != 3)
		return false;

	rate = LittleShort(dataus[1]);
	samples = LittleShort(dataus[2]);

	data += 8;
	datalen -= 8;

	if (datalen != samples)
		return false;

	COM_CharBias(data, datalen);

	return ResampleSfx (s, rate, 1, 1, samples, -1, data);
}
#endif

qboolean S_LoadWavSound (sfx_t *s, qbyte *data, int datalen, int sndspeed)
{
	wavinfo_t	info;

	if (datalen < 4 || strncmp(data, "RIFF", 4))
		return false;

	info = GetWavinfo (s->name, data, datalen);
	if (info.numchannels < 1 || info.numchannels > 2)
	{
		s->loadstate = SLS_FAILED;
		Con_Printf ("%s has an unsupported quantity of channels.\n",s->name);
		return false;
	}

	if (info.width == 1)
		COM_CharBias(data + info.dataofs, info.samples*info.numchannels);
	else if (info.width == 2)
		COM_SwapLittleShortBlock((short *)(data + info.dataofs), info.samples*info.numchannels);

	return ResampleSfx (s, info.rate, info.numchannels, info.width, info.samples, info.loopstart, data + info.dataofs);
}

qboolean S_LoadOVSound (sfx_t *s, qbyte *data, int datalen, int sndspeed);

#ifdef FTE_TARGET_WEB
//web browsers contain their own decoding libraries that our openal stuff can use.
qboolean S_LoadBrowserFile (sfx_t *s, qbyte *data, int datalen, int sndspeed)
{
	sfxcache_t *sc;
	s->decoder.buf = sc = BZ_Malloc(sizeof(sfxcache_t) + datalen);
	sc->data = (qbyte*)(sc+1);
	sc->length = datalen;
	sc->width = 0;	//ie: not pcm
	sc->loopstart = -1;
	sc->speed = sndspeed;
	sc->numchannels = 2;
	sc->soundoffset = 0;
	memcpy(sc->data, data, datalen);

	return true;
}
#endif

//highest priority is last.
S_LoadSound_t AudioInputPlugins[10] =
{
#ifdef FTE_TARGET_WEB
	S_LoadBrowserFile,
#endif
#ifdef AVAIL_OGGVORBIS
	S_LoadOVSound,
#endif
	S_LoadWavSound,
#ifdef DOOMWADS
	S_LoadDoomSound,
//	S_LoadDoomSpeakerSound,
#endif
};

qboolean S_RegisterSoundInputPlugin(S_LoadSound_t loadfnc)
{
	int i;
	for (i = 0; i < sizeof(AudioInputPlugins)/sizeof(AudioInputPlugins[0]); i++)
	{
		if (!AudioInputPlugins[i])
		{
			AudioInputPlugins[i] = loadfnc;
			return true;
		}
	}
	return false;
}

void S_Wakeup (void *ctx, void *ctxdata, size_t a, size_t b)
{
}
/*
==============
S_LoadSound
==============
*/

void S_LoadSoundWorker (void *ctx, void *ctxdata, size_t a, size_t b)
{
	sfx_t *s = ctx;
	char	namebuffer[256];
	qbyte	*data;
	int i;
	size_t result;
	char *name = s->name;
	size_t filesize;

	if (name[1] == ':' && name[2] == '\\')
	{
		vfsfile_t *f;
#ifndef _WIN32	//convert from windows to a suitable alternative.
		char unixname[128];
		Q_snprintfz(unixname, sizeof(unixname), "/mnt/%c/%s", name[0]-'A'+'a', name+3);
		name = unixname;
		while (*name)
		{
			if (*name == '\\')
				*name = '/';
			name++;
		}
		name = unixname;
#endif

		
		if ((f = VFSOS_Open(name, "rb")))
		{
			filesize = VFS_GETLEN(f);
			data = BZ_Malloc (filesize);
			result = VFS_READ(f, data, filesize);

			if (result != filesize)
				Con_SafePrintf("S_LoadSound() fread: Filename: %s, expected %i, result was %u\n", name, filesize, (unsigned int)result);

			VFS_CLOSE(f);
		}
		else
		{
			Con_SafePrintf ("Couldn't load %s\n", namebuffer);
			s->loadstate = SLS_FAILED;
			return;
		}
	}
	else
	{
	//Con_Printf ("S_LoadSound: %x\n", (int)stackbuf);
	// load it in

		data = NULL;
		filesize = 0;
		if (*name == '*')	//q2 sexed sounds
		{
			//clq2_parsestartsound detects this also, and should not try playing these sounds.
			s->loadstate = SLS_FAILED;
			return;
		}
		else if (name[0] == '.' && name[1] == '.' && name[2] == '/')
		{
			//not relative to sound/
			Q_strcpy(namebuffer, name+3);
		}
		else
		{
			//q1 behaviour, relative to sound/
			Q_strcpy(namebuffer, "sound/");
			Q_strcat(namebuffer, name);
			data = COM_LoadFile(namebuffer, 5, &filesize);
		}

	//	Con_Printf ("loading %s\n",namebuffer);

		if (!data)
			data = COM_LoadFile(name, 5, &filesize);
		if (!data)
		{
			char altname[sizeof(namebuffer)];
			COM_StripExtension(namebuffer, altname, sizeof(altname));
			COM_DefaultExtension(altname, ".ogg", sizeof(altname));
			data = COM_LoadFile(altname, 5, &filesize);
			if (data)
				Con_DPrintf("found a mangled name\n");
		}
	}

	if (!data)
	{
		//FIXME: check to see if queued for download.
		Con_DPrintf ("Couldn't load %s\n", namebuffer);
		s->loadstate = SLS_FAILED;
		return;
	}

	for (i = sizeof(AudioInputPlugins)/sizeof(AudioInputPlugins[0])-1; i >= 0; i--)
	{
		if (AudioInputPlugins[i])
		{
			if (AudioInputPlugins[i](s, data, filesize, snd_speed))
			{
				s->loadstate = SLS_LOADED;
				//wake up the main thread in case it decided to wait for us.
				COM_AddWork(0, S_Wakeup, s, NULL, 0, 0);
				BZ_Free(data);
				return;
			}
		}
	}

	if (s->loadstate != SLS_FAILED)
		Con_Printf ("Format not recognised: %s\n", namebuffer);

	s->loadstate = SLS_FAILED;
	BZ_Free(data);
	return;
}

qboolean S_LoadSound (sfx_t *s)
{
	if (s->loadstate == SLS_NOTLOADED && sndcardinfo)
	{
		s->loadstate = SLS_LOADING;
		COM_AddWork(1, S_LoadSoundWorker, s, NULL, 0, 0);
	}
	if (s->loadstate == SLS_FAILED)
		return false;	//it failed to load once before, don't bother trying again.

	return true;	//loaded okay, or still loading
}

/*
===============================================================================

WAV loading

===============================================================================
*/

typedef struct
{
	char	*wavname;
	qbyte	*data_p;
	qbyte 	*iff_end;
	qbyte 	*last_chunk;
	qbyte 	*iff_data;
	int 	iff_chunk_len;
} wavctx_t;

short GetLittleShort(wavctx_t *ctx)
{
	short val = 0;
	val = *ctx->data_p;
	val = val + (*(ctx->data_p+1)<<8);
	ctx->data_p += 2;
	return val;
}

int GetLittleLong(wavctx_t *ctx)
{
	int val = 0;
	val = *ctx->data_p;
	val = val + (*(ctx->data_p+1)<<8);
	val = val + (*(ctx->data_p+2)<<16);
	val = val + (*(ctx->data_p+3)<<24);
	ctx->data_p += 4;
	return val;
}

unsigned int FindNextChunk(wavctx_t *ctx, char *name)
{
	unsigned int dataleft;

	while (1)
	{
		dataleft = ctx->iff_end - ctx->last_chunk;
		if (dataleft < 8)
		{	// didn't find the chunk
			ctx->data_p = NULL;
			return 0;
		}

		ctx->data_p=ctx->last_chunk;
		ctx->data_p += 4;
		dataleft-= 8;
		ctx->iff_chunk_len = GetLittleLong(ctx);
		if (ctx->iff_chunk_len < 0)
		{
			ctx->data_p = NULL;
			return 0;
		}
		if (ctx->iff_chunk_len > dataleft)
		{
			Con_DPrintf ("\"%s\" seems truncated by %i bytes\n", ctx->wavname, ctx->iff_chunk_len-dataleft);
#if 1
			ctx->iff_chunk_len = dataleft;
#else
			ctx->data_p = NULL;
			return 0;
#endif
		}

		dataleft-= ctx->iff_chunk_len;
//		if (iff_chunk_len > 1024*1024)
//			Sys_Error ("FindNextChunk: %i length is past the 1 meg sanity limit", iff_chunk_len);
		ctx->data_p -= 8;
		ctx->last_chunk = ctx->data_p + 8 + ctx->iff_chunk_len;
		if ((ctx->iff_chunk_len&1) && dataleft)
			ctx->last_chunk++;
		if (!Q_strncmp(ctx->data_p, name, 4))
			return ctx->iff_chunk_len;
	}
}

unsigned int FindChunk(wavctx_t *ctx, char *name)
{
	ctx->last_chunk = ctx->iff_data;
	return FindNextChunk (ctx, name);
}


#if 0
void DumpChunks(void)
{
	char	str[5];

	str[4] = 0;
	data_p=iff_data;
	do
	{
		memcpy (str, data_p, 4);
		data_p += 4;
		iff_chunk_len = GetLittleLong();
		Con_Printf ("0x%x : %s (%d)\n", (int)(data_p - 4), str, iff_chunk_len);
		data_p += (iff_chunk_len + 1) & ~1;
	} while (data_p < iff_end);
}
#endif

/*
============
GetWavinfo
============
*/
wavinfo_t GetWavinfo (char *name, qbyte *wav, int wavlength)
{
	wavinfo_t	info;
	int		i;
	int		format;
	int		samples;
	int		chunklen;
	wavctx_t ctx;

	memset (&info, 0, sizeof(info));

	if (!wav)
		return info;

	ctx.data_p = NULL;
	ctx.last_chunk = NULL;
	ctx.iff_chunk_len = 0;

	ctx.iff_data = wav;
	ctx.iff_end = wav + wavlength;
	ctx.wavname = name;

// find "RIFF" chunk
	chunklen = FindChunk(&ctx, "RIFF");
	if (chunklen < 4 ||  Q_strncmp(ctx.data_p+8, "WAVE", 4))
	{
		Con_Printf("Missing RIFF/WAVE chunks in %s\n", name);
		return info;
	}

// get "fmt " chunk
	ctx.iff_data = ctx.data_p + 12;
// DumpChunks ();

	chunklen = FindChunk(&ctx, "fmt ");
	if (chunklen < 24-8)
	{
		Con_Printf("Missing/truncated fmt chunk\n");
		return info;
	}
	ctx.data_p += 8;
	format = GetLittleShort(&ctx);
	if (format != 1)
	{
		Con_Printf("Microsoft PCM format only\n");
		return info;
	}

	info.numchannels = GetLittleShort(&ctx);
	info.rate = GetLittleLong(&ctx);
	ctx.data_p += 4+2;
	info.width = GetLittleShort(&ctx) / 8;

// get cue chunk
	chunklen = FindChunk(&ctx, "cue ");
	if (chunklen >= 36-8)
	{
		ctx.data_p += 32;
		info.loopstart = GetLittleLong(&ctx);
//		Con_Printf("loopstart=%d\n", sfx->loopstart);

	// if the next chunk is a LIST chunk, look for a cue length marker
		chunklen = FindNextChunk (&ctx, "LIST");
		if (chunklen >= 32-8)
		{
			if (!strncmp (ctx.data_p + 28, "mark", 4))
			{	// this is not a proper parse, but it works with cooledit...
				ctx.data_p += 24;
				i = GetLittleLong (&ctx);	// samples in loop
				info.samples = info.loopstart + i;
//				Con_Printf("looped length: %i\n", i);
			}
		}
	}
	else
		info.loopstart = -1;

// find data chunk
	chunklen = FindChunk(&ctx, "data");
	if (!ctx.data_p)
	{
		Con_Printf("Missing data chunk in %s\n", name);
		return info;
	}

	ctx.data_p += 8;
	samples = chunklen / info.width /info.numchannels;

	if (info.samples)
	{
		if (samples < info.samples)
		{
			info.samples = samples;
			Con_Printf ("Sound %s has a bad loop length\n", name);
		}
	}
	else
		info.samples = samples;

	if (info.loopstart > info.samples)
	{
		Con_Printf ("Sound %s has a bad loop start\n", name);
		info.loopstart = info.samples;
	}

	info.dataofs = ctx.data_p - wav;

	return info;
}
