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
void ResampleSfx (sfx_t *sfx, int inrate, int inwidth, qbyte *data)
{
	extern cvar_t snd_linearresample;
	double scale;
	sfxcache_t	*sc;
	int insamps, outsamps;

	sc = Cache_Check (&sfx->cache);
	if (!sc)
		return;

	insamps = sc->length;
	scale = snd_speed / (double)inrate;
	outsamps = insamps * scale;
	sc->length = outsamps;
	if (sc->loopstart != -1)
		sc->loopstart = sc->loopstart * scale;

	sc->speed = snd_speed;
	if (loadas8bit.value)
		sc->width = 1;
	else
		sc->width = inwidth;

	SND_ResampleStream (data, 
		inrate, 
		inwidth, 
		sc->numchannels, 
		insamps, 
		sc->data, 
		sc->speed, 
		sc->width, 
		sc->numchannels, 
		(int)snd_linearresample.value);
}

//=============================================================================
#ifdef DOOMWADS
// needs fine tuning.. educated guesses
#define DSPK_RATE 128
#define DSPK_FREQ 31

sfxcache_t *S_LoadDoomSpeakerSound (sfx_t *s, qbyte *data, int datalen, int sndspeed)
{
	sfxcache_t	*sc;

	// format data from Unofficial Doom Specs v1.6
	unsigned short *dataus;
	int samples, len, timeraccum, inrate, inaccum;
	qbyte *outdata;
	qbyte towrite;

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

	while (len > 0)
	{
		timeraccum += *data * DSPK_FREQ;
		if (timeraccum > snd_speed)
		{
			towrite ^= 0xFF; // swap speaker component
			timeraccum -= snd_speed;
		}

		inaccum--;
		if (!inaccum)
		{
			data++;
			inaccum = inrate;
		}
		*outdata = towrite;
		outdata++;
		len--;
	}

	return sc;
}

sfxcache_t *S_LoadDoomSound (sfx_t *s, qbyte *data, int datalen, int sndspeed)
{
	sfxcache_t	*sc;

	// format data from Unofficial Doom Specs v1.6
	unsigned short *dataus;
	int samples, rate, len;

	if (datalen < 8)
		return NULL;

	dataus = (unsigned short*)data;

	if (LittleShort(dataus[0]) != 3)
		return NULL;

	rate = LittleShort(dataus[1]);
	samples = LittleShort(dataus[2]);

	data += 8;
	datalen -= 8;

	if (datalen != samples)
		return NULL;

	len = (int)((double)samples * (double)snd_speed / (double)rate);

	sc = Cache_Alloc (&s->cache, len + sizeof(sfxcache_t), s->name);
	if (!sc)
	{
		return NULL;
	}

	sc->length = samples;
	sc->loopstart = -1;
	sc->numchannels = 1;
	sc->width = 1;
	sc->speed = rate;

	if (sc->width == 1)
		COM_CharBias(data, sc->length);
	else if (sc->width == 2)
		COM_SwapLittleShortBlock((short *)data, sc->length);

	ResampleSfx (s, sc->speed, sc->width, data);

	return sc;
}
#endif

sfxcache_t *S_LoadWavSound (sfx_t *s, qbyte *data, int datalen, int sndspeed)
{
	wavinfo_t	info;
	int		len;
	sfxcache_t	*sc;

	if (datalen < 4 || strncmp(data, "RIFF", 4))
		return NULL;

	info = GetWavinfo (s->name, data, datalen);
	if (info.numchannels < 1 || info.numchannels > 2)
	{
		s->failedload = true;
		Con_Printf ("%s has an unsupported quantity of channels.\n",s->name);
		return NULL;
	}

	len = (int) ((double) info.samples * (double) snd_speed / (double) info.rate);
	len = len * info.width * info.numchannels;

	sc = Cache_Alloc ( &s->cache, len + sizeof(sfxcache_t), s->name);
	if (!sc)
	{
		return NULL;
	}
	
	sc->length = info.samples;
	sc->loopstart = info.loopstart;
	sc->speed = info.rate;
	sc->width = info.width;
	sc->numchannels = info.numchannels;

	if (sc->width == 1)
		COM_CharBias(data + info.dataofs, sc->length*sc->numchannels);
	else if (sc->width == 2)
		COM_SwapLittleShortBlock((short *)(data + info.dataofs), sc->length*sc->numchannels);

	ResampleSfx (s, sc->speed, sc->width, data + info.dataofs);

	return sc;
}

sfxcache_t *S_LoadOVSound (sfx_t *s, qbyte *data, int datalen, int sndspeed);

S_LoadSound_t AudioInputPlugins[10] =
{
#ifdef AVAIL_OGGVORBIS
	S_LoadOVSound,
#endif
	S_LoadWavSound,
#ifdef DOOMWADS
	S_LoadDoomSound,
	S_LoadDoomSpeakerSound,
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

/*
==============
S_LoadSound
==============
*/

sfxcache_t *S_LoadSound (sfx_t *s)
{
	char stackbuf[65536];
    char	namebuffer[256];
	qbyte	*data;
	sfxcache_t	*sc;
	int i;

	char *name = s->name;

	if (s->failedload)
		return NULL;	//it failed to load once before, don't bother trying again.

// see if still in memory
	sc = Cache_Check (&s->cache);
	if (sc)
		return sc;

	s->decoder = NULL;




	if (name[1] == ':' && name[2] == '\\')
	{
		FILE *f;
#ifndef _WIN32	//convert from windows to a suitable alternative.
		char unixname[128];
		sprintf(unixname, "/mnt/%c/%s", name[0]-'A'+'a', name+3);
		name = unixname;
		while (*name)
		{
			if (*name == '\\')
				*name = '/';
			name++;
		}			
		name = unixname;
#endif

		if ((f = fopen(name, "rb")))
		{
			com_filesize = COM_filelength(f);
			data = Hunk_TempAlloc (com_filesize);
			fread(data, 1, com_filesize, f);
			fclose(f);
		}
		else
		{
			Con_SafePrintf ("Couldn't load %s\n", namebuffer);
			return NULL;
		}
	}
	else
	{
	//Con_Printf ("S_LoadSound: %x\n", (int)stackbuf);
	// load it in

		data = NULL;
		if (*name == '*')
		{
			Q_strcpy(namebuffer, "players/male/");	//q2
			Q_strcat(namebuffer, name+1);	//q2
		}
		else if (name[0] == '.' && name[1] == '.' && name[2] == '/')
			Q_strcpy(namebuffer, name+3);
		else
		{
			Q_strcpy(namebuffer, "sound/");
			Q_strcat(namebuffer, name);
			data = COM_LoadStackFile(name, stackbuf, sizeof(stackbuf));
		}

	//	Con_Printf ("loading %s\n",namebuffer);

		if (!data)
			data = COM_LoadStackFile(namebuffer, stackbuf, sizeof(stackbuf));
		if (!data)
		{
			char altname[sizeof(namebuffer)];
			COM_StripExtension(namebuffer, altname, sizeof(altname));
			COM_DefaultExtension(altname, ".ogg", sizeof(altname));
			data = COM_LoadStackFile(altname, stackbuf, sizeof(stackbuf));
			if (data)
				Con_DPrintf("found a mangled name\n");
		}
	}

	if (!data)
	{
		//FIXME: check to see if qued for download.
		Con_DPrintf ("Couldn't load %s\n", namebuffer);
		s->failedload = true;
		return NULL;
	}

	s->failedload = false;

	for (i = sizeof(AudioInputPlugins)/sizeof(AudioInputPlugins[0])-1; i >= 0; i--)
	{
		if (AudioInputPlugins[i])
		{
			sc = AudioInputPlugins[i](s, data, com_filesize, snd_speed);
			if (sc)
				return sc;
		}
	}

	if (!s->failedload)
		Con_Printf ("Format not recognised: %s\n", namebuffer);

	s->failedload = true;
	return NULL;
}



/*
===============================================================================

WAV loading

===============================================================================
*/


qbyte	*data_p;
qbyte 	*iff_end;
qbyte 	*last_chunk;
qbyte 	*iff_data;
int 	iff_chunk_len;


short GetLittleShort(void)
{
	short val = 0;
	val = *data_p;
	val = val + (*(data_p+1)<<8);
	data_p += 2;
	return val;
}

int GetLittleLong(void)
{
	int val = 0;
	val = *data_p;
	val = val + (*(data_p+1)<<8);
	val = val + (*(data_p+2)<<16);
	val = val + (*(data_p+3)<<24);
	data_p += 4;
	return val;
}

void FindNextChunk(char *name)
{
	unsigned int dataleft;

	while (1)
	{
		dataleft = iff_end - last_chunk;
		if (dataleft < 8)
		{	// didn't find the chunk
			data_p = NULL;
			return;
		}

		data_p=last_chunk;
		data_p += 4;
		dataleft-= 8;
		iff_chunk_len = GetLittleLong();
		if (iff_chunk_len < 0 || iff_chunk_len > dataleft)
		{
			data_p = NULL;
			return;
		}
//		if (iff_chunk_len > 1024*1024)
//			Sys_Error ("FindNextChunk: %i length is past the 1 meg sanity limit", iff_chunk_len);
		data_p -= 8;
		last_chunk = data_p + 8 + ( (iff_chunk_len + 1) & ~1 );
		if (!Q_strncmp(data_p, name, 4))
			return;
	}
}

void FindChunk(char *name)
{
	last_chunk = iff_data;
	FindNextChunk (name);
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
	int     i;
	int     format;
	int		samples;

	memset (&info, 0, sizeof(info));

	if (!wav)
		return info;
		
	iff_data = wav;
	iff_end = wav + wavlength;

// find "RIFF" chunk
	FindChunk("RIFF");
	if (!(data_p && !Q_strncmp(data_p+8, "WAVE", 4)))
	{
		Con_Printf("Missing RIFF/WAVE chunks\n");
		return info;
	}

// get "fmt " chunk
	iff_data = data_p + 12;
// DumpChunks ();

	FindChunk("fmt ");
	if (!data_p)
	{
		Con_Printf("Missing fmt chunk\n");
		return info;
	}
	data_p += 8;
	format = GetLittleShort();
	if (format != 1)
	{
		Con_Printf("Microsoft PCM format only\n");
		return info;
	}

	info.numchannels = GetLittleShort();
	info.rate = GetLittleLong();
	data_p += 4+2;
	info.width = GetLittleShort() / 8;

// get cue chunk
	FindChunk("cue ");
	if (data_p)
	{
		data_p += 32;
		info.loopstart = GetLittleLong();
//		Con_Printf("loopstart=%d\n", sfx->loopstart);

	// if the next chunk is a LIST chunk, look for a cue length marker
		FindNextChunk ("LIST");
		if (data_p)
		{
			if (!strncmp (data_p + 28, "mark", 4))
			{	// this is not a proper parse, but it works with cooledit...
				data_p += 24;
				i = GetLittleLong ();	// samples in loop
				info.samples = info.loopstart + i;
//				Con_Printf("looped length: %i\n", i);
			}
		}
	}
	else
		info.loopstart = -1;

// find data chunk
	FindChunk("data");
	if (!data_p)
	{
		Con_Printf("Missing data chunk\n");
		return info;
	}

	data_p += 4;
	samples = GetLittleLong () / info.width /info.numchannels;

	if (info.samples)
	{
		if (samples < info.samples)
			Sys_Error ("Sound %s has a bad loop length", name);
	}
	else
		info.samples = samples;

	info.dataofs = data_p - wav;
	
	return info;
}
