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
// vid.h -- video driver defs

#define VID_CBITS	6
#define VID_GRADES	(1 << VID_CBITS)

// a pixel can be one, two, or four bytes
typedef qbyte pixel_t;

typedef enum {QR_NONE, QR_SOFTWARE, QR_OPENGL} r_qrenderer_t;

typedef struct {
	//you are not allowed to make anything not work if it's not based on these vars...
	int width;
	int height;
	qboolean fullscreen;
	int bpp;
	int rate;
	int multisample;	//for opengl antialiasing (which requires context stuff)
	float streach;
	char glrenderer[MAX_QPATH];
	r_qrenderer_t renderer;
	qboolean allow_modex;
} rendererstate_t;

typedef struct vrect_s
{
	int				x,y,width,height;
	struct vrect_s	*pnext;
} vrect_t;

typedef struct
{
	pixel_t			*buffer;		// invisible buffer
	pixel_t			*colormap;		// 256 * VID_GRADES size
	unsigned short	*colormap16;	// 256 * VID_GRADES size
	int				fullbright;		// index of first fullbright color
	unsigned		rowbytes;	// may be > width if displayed in a window
	unsigned		width;		
	unsigned		height;
	float			aspect;		// width / height -- < 0 is taller than wide
	int				numpages;
	int				recalc_refdef;	// if true, recalc vid-based stuff
	pixel_t			*conbuffer;
	int				conrowbytes;
	unsigned		conwidth;
	unsigned		conheight;
	int				maxwarpwidth;
	int				maxwarpheight;
	pixel_t			*direct;		// direct drawing to framebuffer, if not
									//  NULL
} viddef_t;

extern	viddef_t	vid;				// global video state
extern unsigned short	d_8to16table[256];

extern unsigned int	d_8to24bgrtable[256];
extern unsigned int	d_8to24rgbtable[256];
extern unsigned int	*d_8to32table;

#ifdef RGLQUAKE
void	GLVID_SetPalette (unsigned char *palette);
// called at startup and after any gamma correction

void	GLVID_ShiftPalette (unsigned char *palette);
// called for bonus and pain flashes, and for underwater color changes

qboolean GLVID_Init (rendererstate_t *info, unsigned char *palette);
// Called at startup to set up translation tables, takes 256 8 bit RGB values
// the palette data will go away after the call, so it must be copied off if
// the video driver will need it again

void	GLVID_Shutdown (void);
// Called at shutdown

void	GLVID_Update (vrect_t *rects);
// flushes the given rectangles from the view buffer to the screen

int GLVID_SetMode (rendererstate_t *info, unsigned char *palette);
// sets the mode; only used by the Quake engine for resetting to mode 0 (the
// base mode) on memory allocation failures

void GLVID_HandlePause (qboolean pause);
// called only on Win32, when pause happens, so the mouse can be released

void GLVID_LockBuffer (void);
void GLVID_UnlockBuffer (void);

int GLVID_ForceUnlockedAndReturnState (void);
void GLVID_ForceLockState (int lk);

qboolean GLVID_Is8bit();

void GLD_BeginDirectRect (int x, int y, qbyte *pbitmap, int width, int height);
void GLD_EndDirectRect (int x, int y, int width, int height);
char *GLVID_GetRGBInfo(int prepadbytes, int *truewidth, int *trueheight);
void GLVID_SetCaption(char *caption);
#endif

#ifdef SWQUAKE
void	SWVID_SetPalette (unsigned char *palette);
// called at startup and after any gamma correction

void	SWVID_ShiftPalette (unsigned char *palette);
// called for bonus and pain flashes, and for underwater color changes

qboolean SWVID_Init (rendererstate_t *info, unsigned char *palette);
// Called at startup to set up translation tables, takes 256 8 bit RGB values
// the palette data will go away after the call, so it must be copied off if
// the video driver will need it again

void	SWVID_Shutdown (void);
// Called at shutdown

void	SWVID_Update (vrect_t *rects);
// flushes the given rectangles from the view buffer to the screen

void SWVID_HandlePause (qboolean pause);
// called only on Win32, when pause happens, so the mouse can be released

void SWVID_LockBuffer (void);
void SWVID_UnlockBuffer (void);

int SWVID_ForceUnlockedAndReturnState (void);
void SWVID_ForceLockState (int lk);

char *SWVID_GetRGBInfo(int prepadbytes, int *truewidth, int *trueheight);
#endif
