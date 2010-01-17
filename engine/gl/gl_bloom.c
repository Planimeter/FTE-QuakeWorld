/*
Copyright (C) 1997-2001 Id Software, Inc.

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
// gl_bloom.c: 2D lighting post process effect


//http://www.quakesrc.org/forums/viewtopic.php?t=4340&start=0

#include "quakedef.h"

#ifdef RGLQUAKE
#include "glquake.h"

extern vrect_t gl_truescreenrect;

/*
==============================================================================

						LIGHT BLOOMS

==============================================================================
*/

static float Diamond8x[8][8] = {
	{0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, 0.0f},
	{0.0f, 0.0f, 0.2f, 0.3f, 0.3f, 0.2f, 0.0f, 0.0f},
	{0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f},
	{0.1f, 0.3f, 0.6f, 0.9f, 0.9f, 0.6f, 0.3f, 0.1f},
	{0.1f, 0.3f, 0.6f, 0.9f, 0.9f, 0.6f, 0.3f, 0.1f},
	{0.0f, 0.2f, 0.4f, 0.6f, 0.6f, 0.4f, 0.2f, 0.0f},
	{0.0f, 0.0f, 0.2f, 0.3f, 0.3f, 0.2f, 0.0f, 0.0f},
	{0.0f, 0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f, 0.0f} };

static float Diamond6x[6][6] = {
	{0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f},
	{0.0f, 0.3f, 0.5f, 0.5f, 0.3f, 0.0f},
	{0.1f, 0.5f, 0.9f, 0.9f, 0.5f, 0.1f},
	{0.1f, 0.5f, 0.9f, 0.9f, 0.5f, 0.1f},
	{0.0f, 0.3f, 0.5f, 0.5f, 0.3f, 0.0f},
	{0.0f, 0.0f, 0.1f, 0.1f, 0.0f, 0.0f} };

static float Diamond4x[4][4] = {
	{0.3f, 0.4f, 0.4f, 0.3f},
	{0.4f, 0.9f, 0.9f, 0.4f},
	{0.4f, 0.9f, 0.9f, 0.4f},
	{0.3f, 0.4f, 0.4f, 0.3f} };

       cvar_t		r_bloom = FCVAR("r_bloom", "gl_bloom", "0", CVAR_ARCHIVE);
       cvar_t		r_bloom_alpha = SCVAR("r_bloom_alpha", "0.5");
	   cvar_t		r_bloom_diamond_size = SCVAR("r_bloom_diamond_size", "8");
       cvar_t		r_bloom_intensity = SCVAR("r_bloom_intensity", "1");
	   cvar_t		r_bloom_darken = SCVAR("r_bloom_darken", "3");
	   cvar_t		r_bloom_sample_size = SCVARF("r_bloom_sample_size", "256", CVAR_RENDERERLATCH);
       cvar_t		r_bloom_fast_sample = SCVARF("r_bloom_fast_sample", "0", CVAR_RENDERERLATCH);

typedef struct {
	//texture numbers
	int	tx_screen;
	int tx_effect;
	int tx_backup;
	int tx_downsample;

	//the viewport dimensions
	int vp_x;
	int vp_y;
	int vp_w;
	int vp_h;

	//texture coordinates of screen data inside screentexture
	float scr_s;
	float scr_t;

	//dimensions of the screen texture (power of two)
	int scr_w;
	int scr_h;

	//downsampled dimensions (will always be smaller than viewport)
	int smp_w;
	int smp_h;
	//tex coords to be used for the sample
	float smp_s;
	float smp_t;

	int size_downsample;
	int size_backup;
	int size_sample;
} bloomstate_t;

static bloomstate_t bs;

//this macro is in sample size workspace coordinates
#define R_Bloom_SamplePass( xpos, ypos )				\
	qglBegin(GL_QUADS);									\
	qglTexCoord2f(	0,					bs.smp_t);		\
	qglVertex2f(	xpos,				ypos);			\
	qglTexCoord2f(	0,					0);				\
	qglVertex2f(	xpos,				ypos+bs.smp_h);	\
	qglTexCoord2f(	bs.smp_s,			0);				\
	qglVertex2f(	xpos+bs.smp_w,		ypos+bs.smp_h);	\
	qglTexCoord2f(	bs.smp_s,			bs.smp_t);		\
	qglVertex2f(	xpos+bs.smp_w,		ypos);			\
	qglEnd();

#define R_Bloom_Quad( x, y, width, height, textwidth, textheight )	\
	qglBegin(GL_QUADS);												\
	qglTexCoord2f(	0,			textheight);						\
	qglVertex2f(	x,			y);									\
	qglTexCoord2f(	0,			0);									\
	qglVertex2f(	x,			y+height);							\
	qglTexCoord2f(	textwidth,	0);									\
	qglVertex2f(	x+width,	y+height);							\
	qglTexCoord2f(	textwidth,	textheight);						\
	qglVertex2f(	x+width,	y);									\
	qglEnd();



/*
=================
R_Bloom_InitBackUpTexture
=================
*/
void R_Bloom_InitBackUpTexture(int widthheight)
{
	qbyte	*data;

	data = Z_Malloc(widthheight * widthheight * 4);

	bs.size_backup = widthheight;
	bs.tx_backup = GL_LoadTexture32("***bs.tx_backup***", bs.size_backup, bs.size_backup, (unsigned int*)data, false, false);

	Z_Free (data);
}

/*
=================
R_Bloom_InitEffectTexture
=================
*/
void R_Bloom_InitEffectTexture(void)
{
	qbyte	*data;
	float	bloomsizecheck;

	if (r_bloom_sample_size.value < 32)
		Cvar_SetValue (&r_bloom_sample_size, 32);

	//make sure bloom size is a power of 2
	bs.size_sample = r_bloom_sample_size.value;
	bloomsizecheck = (float)bs.size_sample;
	while (bloomsizecheck > 1.0f) bloomsizecheck /= 2.0f;
	if (bloomsizecheck != 1.0f)
	{
		bs.size_sample = 32;
		while (bs.size_sample < r_bloom_sample_size.value)
			bs.size_sample *= 2;
	}

	//make sure bloom size doesn't have stupid values
	if (bs.size_sample > bs.scr_w ||
		bs.size_sample > bs.scr_h)
		bs.size_sample = min(bs.scr_w, bs.scr_h);

	if (bs.size_sample != r_bloom_sample_size.value)
		Cvar_SetValue (&r_bloom_sample_size, bs.size_sample);

	data = Z_Malloc(bs.size_sample * bs.size_sample * 4);

	bs.tx_effect = GL_LoadTexture32("***bs.tx_effect***", bs.size_sample, bs.size_sample, (unsigned int*)data, false, false);

	Z_Free (data);
}

/*
=================
R_Bloom_InitTextures
=================
*/
void R_Bloom_InitTextures(void)
{
	qbyte	*data;
	int		size;
	int maxtexsize;

	//find closer power of 2 to screen size
	for (bs.scr_w = 1;bs.scr_w < glwidth;bs.scr_w *= 2);
	for (bs.scr_h = 1;bs.scr_h < glheight;bs.scr_h *= 2);

	//disable blooms if we can't handle a texture of that size
	qglGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxtexsize);
	if (bs.scr_w > maxtexsize ||
		bs.scr_h > maxtexsize)
	{
		bs.scr_w = bs.scr_h = 0;
		Cvar_SetValue (&r_bloom, 0);
		Con_Printf("WARNING: 'R_InitBloomScreenTexture' too high resolution for Light Bloom. Effect disabled\n");
		return;
	}

	//init the screen texture
	size = bs.scr_w * bs.scr_h * 4;
	data = Z_Malloc(size);
	memset(data, 255, size);
	if (!bs.tx_screen)
		bs.tx_screen = GL_AllocNewTexture();
	GL_Bind(bs.tx_screen);
	qglTexImage2D (GL_TEXTURE_2D, 0, gl_solid_format, bs.scr_w, bs.scr_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	Z_Free (data);


	//validate bloom size and init the bloom effect texture
	R_Bloom_InitEffectTexture ();

	//if screensize is more than 2x the bloom effect texture, set up for stepped downsampling
	bs.tx_downsample = 0;
	bs.size_downsample = 0;
	if (glwidth > (bs.size_sample * 2) && !r_bloom_fast_sample.value)
	{
		bs.size_downsample = (int)(bs.size_sample * 2);
		data = Z_Malloc(bs.size_downsample * bs.size_downsample * 4);
		bs.tx_downsample = GL_LoadTexture32("***bs.tx_downsample***", bs.size_downsample, bs.size_downsample, (unsigned int*)data, false, false);
		Z_Free (data);
	}

	//Init the screen backup texture
	if (bs.size_downsample)
		R_Bloom_InitBackUpTexture(bs.size_downsample);
	else
		R_Bloom_InitBackUpTexture(bs.size_sample);
}

void R_BloomRegister(void)
{
	Cvar_Register (&r_bloom, "bloom");
	Cvar_Register (&r_bloom_alpha, "bloom");
	Cvar_Register (&r_bloom_diamond_size, "bloom");
	Cvar_Register (&r_bloom_intensity, "bloom");
	Cvar_Register (&r_bloom_darken, "bloom");
	Cvar_Register (&r_bloom_sample_size, "bloom");
	Cvar_Register (&r_bloom_fast_sample, "bloom");
}

/*
=================
R_InitBloomTextures
=================
*/
void R_InitBloomTextures(void)
{
	bs.size_sample = 0;
	if (!r_bloom.value)
		return;

	bs.tx_screen = 0;	//this came from a vid_restart, where none of the textures are valid any more.
	R_Bloom_InitTextures ();
}


/*
=================
R_Bloom_DrawEffect
=================
*/
void R_Bloom_DrawEffect(void)
{
	GL_Bind(bs.tx_effect);
	qglEnable(GL_BLEND);
	qglBlendFunc(GL_ONE, GL_ONE);
	qglColor4f(r_bloom_alpha.value, r_bloom_alpha.value, r_bloom_alpha.value, 1.0f);
	GL_TexEnv(GL_MODULATE);
	qglBegin(GL_QUADS);
	qglTexCoord2f	(0,					bs.smp_t);
	qglVertex2f		(bs.vp_x,			bs.vp_y);
	qglTexCoord2f	(0,					0);
	qglVertex2f		(bs.vp_x,			bs.vp_y + bs.vp_h);
	qglTexCoord2f	(bs.smp_s,			0);
	qglVertex2f		(bs.vp_x + bs.vp_w,	bs.vp_y + bs.vp_h);
	qglTexCoord2f	(bs.smp_s,			bs.smp_t);
	qglVertex2f		(bs.vp_x + bs.vp_w,	bs.vp_y);
	qglEnd();

	qglDisable(GL_BLEND);
}


#if 0
/*
=================
R_Bloom_GeneratexCross - alternative bluring method
=================
*/
void R_Bloom_GeneratexCross(void)
{
	int			i;
	static int		BLOOM_BLUR_RADIUS = 8;
	//static float	BLOOM_BLUR_INTENSITY = 2.5f;
	float	BLOOM_BLUR_INTENSITY;
	static float intensity;
	static float range;

	//set up sample size workspace
	qglViewport( 0, 0, bs.smp_w, bs.smp_h );
	qglMatrixMode( GL_PROJECTION );
    qglLoadIdentity ();
	qglOrtho(0, bs.smp_w, bs.smp_h, 0, -10, 100);
	qglMatrixMode( GL_MODELVIEW );
    qglLoadIdentity ();

	//copy small scene into bs.tx_effect
	GL_Bind(0, bs.tx_effect);
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, bs.smp_w, bs.smp_h);

	//start modifying the small scene corner
	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	qglEnable(GL_BLEND);

	//darkening passes
	if( r_bloom_darken.value )
	{
		qglBlendFunc(GL_DST_COLOR, GL_ZERO);
		GL_TexEnv(GL_MODULATE);

		for(i=0; i<r_bloom_darken->integer ;i++) {
			R_Bloom_SamplePass( 0, 0 );
		}
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, bs.smp_w, bs.smp_h);
	}

	//bluring passes
	if( BLOOM_BLUR_RADIUS ) {

		qglBlendFunc(GL_ONE, GL_ONE);

		range = (float)BLOOM_BLUR_RADIUS;

		BLOOM_BLUR_INTENSITY = r_bloom_intensity.value;
		//diagonal-cross draw 4 passes to add initial smooth
		qglColor4f( 0.5f, 0.5f, 0.5f, 1.0);
		R_Bloom_SamplePass( 1, 1 );
		R_Bloom_SamplePass( -1, 1 );
		R_Bloom_SamplePass( -1, -1 );
		R_Bloom_SamplePass( 1, -1 );
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, bs.smp_w, bs.smp_h);

		for(i=-(BLOOM_BLUR_RADIUS+1);i<BLOOM_BLUR_RADIUS;i++) {
			intensity = BLOOM_BLUR_INTENSITY/(range*2+1)*(1 - fabs(i*i)/(float)(range*range));
			if( intensity < 0.05f ) continue;
			qglColor4f( intensity, intensity, intensity, 1.0f);
			R_Bloom_SamplePass( i, 0 );
			//R_Bloom_SamplePass( -i, 0 );
		}

		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, bs.smp_w, bs.smp_h);

		//for(i=0;i<BLOOM_BLUR_RADIUS;i++) {
		for(i=-(BLOOM_BLUR_RADIUS+1);i<BLOOM_BLUR_RADIUS;i++) {
			intensity = BLOOM_BLUR_INTENSITY/(range*2+1)*(1 - fabs(i*i)/(float)(range*range));
			if( intensity < 0.05f ) continue;
			qglColor4f( intensity, intensity, intensity, 1.0f);
			R_Bloom_SamplePass( 0, i );
			//R_Bloom_SamplePass( 0, -i );
		}

		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, bs.smp_w, bs.smp_h);
	}

	//restore full screen workspace
	qglViewport( 0, 0, glState.width, glState.height );
	qglMatrixMode( GL_PROJECTION );
    qglLoadIdentity ();
	qglOrtho(0, glState.width, glState.height, 0, -10, 100);
	qglMatrixMode( GL_MODELVIEW );
    qglLoadIdentity ();
}
#endif


/*
=================
R_Bloom_GeneratexDiamonds
=================
*/
void R_Bloom_GeneratexDiamonds(void)
{
	int			i, j;
	float intensity;

	//set up sample size workspace
	qglViewport(0, 0, bs.smp_w, bs.smp_h);
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity();
	qglOrtho(0, bs.smp_w, bs.smp_h, 0, -10, 100);
	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity();

	//copy small scene into bs.tx_effect
	GL_Bind(bs.tx_effect);
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, bs.smp_w, bs.smp_h);

	//start modifying the small scene corner
	qglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	qglEnable(GL_BLEND);

	//darkening passes
	if (r_bloom_darken.value)
	{
		qglBlendFunc(GL_DST_COLOR, GL_ZERO);
		GL_TexEnv(GL_MODULATE);

		for (i=0; i<r_bloom_darken.value ;i++)
		{
			R_Bloom_SamplePass(0, 0);
		}
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, bs.smp_w, bs.smp_h);
	}

	//bluring passes
	//qglBlendFunc(GL_ONE, GL_ONE);
	qglBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_COLOR);

	if (r_bloom_diamond_size.value > 7 || r_bloom_diamond_size.value <= 3)
	{
		if (r_bloom_diamond_size.value != 8)
			Cvar_SetValue(&r_bloom_diamond_size, 8);

		for (i=0; i<r_bloom_diamond_size.value; i++)
		{
			for (j=0; j<r_bloom_diamond_size.value; j++)
			{
				intensity = r_bloom_intensity.value * 0.3 * Diamond8x[i][j];
				if (intensity < 0.01f)
					continue;
				qglColor4f(intensity, intensity, intensity, 1.0);
				R_Bloom_SamplePass(i-4, j-4);
			}
		}
	}
	else if (r_bloom_diamond_size.value > 5)
	{
		if (r_bloom_diamond_size.value != 6)
			Cvar_SetValue(&r_bloom_diamond_size, 6);

		for(i=0; i<r_bloom_diamond_size.value; i++)
		{
			for(j=0; j<r_bloom_diamond_size.value; j++)
			{
				intensity = r_bloom_intensity.value * 0.5 * Diamond6x[i][j];
				if (intensity < 0.01f)
					continue;
				qglColor4f(intensity, intensity, intensity, 1.0);
				R_Bloom_SamplePass(i-3, j-3);
			}
		}
	}
	else if (r_bloom_diamond_size.value > 3)
	{
		if (r_bloom_diamond_size.value != 4)
			Cvar_SetValue(&r_bloom_diamond_size, 4);

		for (i=0; i<r_bloom_diamond_size.value; i++)
		{
			for (j=0; j<r_bloom_diamond_size.value; j++)
			{
				intensity = r_bloom_intensity.value * 0.8f * Diamond4x[i][j];
				if (intensity < 0.01f)
					continue;
				qglColor4f(intensity, intensity, intensity, 1.0);
				R_Bloom_SamplePass( i-2, j-2 );
			}
		}
	}

	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, bs.smp_w, bs.smp_h);

	//restore full screen workspace
	qglViewport(0, 0, glwidth, glheight);
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
	qglOrtho(0, glwidth, glheight, 0, -10, 100);
	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();
}

/*
=================
R_Bloom_DownsampleView
=================
*/
void R_Bloom_DownsampleView( void )
{
	qglDisable(GL_BLEND);
	qglColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	//stepped downsample
	if (bs.size_downsample)
	{
		int		midsample_width = bs.size_downsample * bs.smp_s;
		int		midsample_height = bs.size_downsample * bs.smp_t;

		//copy the screen and draw resized
		GL_Bind(bs.tx_screen);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, bs.vp_x, glheight - (bs.vp_y + bs.vp_h), bs.vp_w, bs.vp_h);

		R_Bloom_Quad(0, glheight-midsample_height, midsample_width, midsample_height, bs.scr_s, bs.scr_t);

		//now copy into Downsampling (mid-sized) texture
		GL_Bind(bs.tx_downsample);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, midsample_width, midsample_height);

		//now draw again in bloom size
		qglColor4f(0.5f, 0.5f, 0.5f, 1.0f);
		R_Bloom_Quad(0,  glheight-bs.smp_h, bs.smp_w, bs.smp_h, bs.smp_s, bs.smp_t);

		//now blend the big screen texture into the bloom generation space (hoping it adds some blur)
		qglEnable(GL_BLEND);
		qglBlendFunc(GL_ONE, GL_ONE);
		qglColor4f(0.5f, 0.5f, 0.5f, 1.0f);
		GL_Bind(bs.tx_screen);
		R_Bloom_Quad(0,  glheight-bs.smp_h, bs.smp_w, bs.smp_h, bs.scr_s, bs.scr_t);
		qglColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		qglDisable(GL_BLEND);
	}
	else
	{	//downsample simple

		GL_Bind(bs.tx_screen);
		qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, bs.vp_x, glheight - (bs.vp_y + bs.vp_h), bs.vp_w, bs.vp_h);
		R_Bloom_Quad(0, glheight-bs.smp_h, bs.smp_w, bs.smp_h, bs.scr_s, bs.scr_t);
	}
}

/*
=================
R_BloomBlend
=================
*/
void R_BloomBlend (void)//refdef_t *fd, meshlist_t *meshlist )
{
	int buw, buh;
	if (!r_bloom.value)
		return;

	if (!bs.size_sample || bs.scr_w < glwidth || bs.scr_h < glheight)
		R_Bloom_InitTextures();

	if (bs.scr_w < bs.size_sample ||
		bs.scr_h < bs.size_sample)
		return;

	//set up full screen workspace
	qglViewport(0, 0, glwidth, glheight);
	qglDisable(GL_DEPTH_TEST);
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity();
	qglOrtho(0, glwidth, glheight, 0, -10, 100);
	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity();
	qglDisable(GL_CULL_FACE);

	qglDisable(GL_BLEND);
	qglEnable(GL_TEXTURE_2D);

	qglColor4f(1, 1, 1, 1);

	//set up current sizes
	bs.vp_x = gl_truescreenrect.x;
	bs.vp_y = glheight - gl_truescreenrect.y;
	bs.vp_w = gl_truescreenrect.width;
	bs.vp_h = gl_truescreenrect.height;
	bs.scr_s = (float)bs.vp_w / (float)bs.scr_w;
	bs.scr_t = (float)bs.vp_h / (float)bs.scr_h;
	if (bs.vp_h > bs.vp_w)
	{
		bs.smp_s = (float)bs.vp_w / (float)bs.vp_h;
		bs.smp_t = 1.0f;
	}
	else
	{
		bs.smp_s = 1.0f;
		bs.smp_t = (float)bs.vp_h / (float)bs.vp_w;
	}
	bs.smp_w = bs.size_sample * bs.smp_s;
	bs.smp_h = bs.size_sample * bs.smp_t;

	bs.smp_s = (float)bs.smp_w/bs.size_sample;
	bs.smp_t = (float)bs.smp_h/bs.size_sample;

	buw = bs.size_downsample * bs.smp_s;
	buh = bs.size_downsample * bs.smp_t;

	//copy the screen space we'll use to work into the backup texture
	GL_Bind(bs.tx_backup);
	qglCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, buw, buh);

	//create the bloom image
	R_Bloom_DownsampleView();

	R_Bloom_GeneratexDiamonds();
	//R_Bloom_GeneratexCross();

	//restore the screen-backup to the screen
	qglDisable(GL_BLEND);
	GL_Bind(bs.tx_backup);
	qglColor4f(1, 1, 1, 1);
	R_Bloom_Quad(0,
		glheight - (buh),
		buw,
		buh,
		bs.smp_s,
		bs.smp_t);

	R_Bloom_DrawEffect();


	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (qglGetError())
		Con_Printf("GL Error whilst rendering bloom\n");
}

#endif
