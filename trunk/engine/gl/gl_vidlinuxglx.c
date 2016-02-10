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

/*
Small note: anything concerning EGL in here is specific to egl-with-x11.
if you want egl-with-framebuffer, look elsewhere.
*/

/*
X11 is a huge pile of shit. I don't mean just the old x11 protocol, but all the _current_ standards that don't even try to fix the issues too.

Its fucking retarded the crap that you have to do to get something to work.
timeouts to ensure alt+tab between two apps doesn't fuck up gamma ramps is NOT a nice thing to have to do.
_MOUSE_ grabs cause alt+tab to fuck up
if I use xinput2 to get raw mouse events (so that I don't have to use some random hack to disable acceleration and risk failing to reset it on crashes), then the mouse still moves outside of our window, and trying to fire then looses focus...
xf86vm extension results in scrolling around a larger viewport. dependant upon the mouse position. even if we constrain the mouse to our window, it'll still scroll.
warping the pointer still triggers 'raw' mouse move events. in what world does that make any sense?!?
alt-tab task lists are a window in their own right. that's okay, but what's not okay is that they destroy that window without giving focus to the new window first, so the old one gets focus and that fucks everything up too. yay for timeouts.
to allow alt-tabbing with window managers that do not respect requests to not shove stuff on us, we have to hide ourselves completely and create a separate window that can still accept focus from the window manager. its fecking vile.
window managers reparent us too, in much the same way. which is a bad thing because we keep getting reparented and that makes a mess of focus events. its a nightmare.

the whole thing is bloody retarded.

none of these issues will be fixed by a compositing window manager, because there's still a window manager there.
*/


#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#ifdef __linux__
#include <sys/vt.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <signal.h>

#include <dlfcn.h>

#include "quakedef.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <GL/glx.h>
#ifdef USE_EGL
#include "gl_videgl.h"
#endif
#include "glquake.h"

#include <X11/keysym.h>
#include <X11/cursorfont.h>

static Display *vid_dpy = NULL;
static Cursor vid_nullcursor;
static Window vid_window;
static Window vid_decoywindow;	//for legacy mode, this is a boring window that we can reparent into as needed
static Window vid_root;
static GLXContext ctx = NULL;
static int scrnum;
static long vid_x_eventmask;
static enum
{
	PSL_NONE,
#ifdef USE_EGL
	PSL_EGL,
#endif
	PSL_GLX
} currentpsl;

extern cvar_t vid_conautoscale;

extern int sys_parentleft;
extern int sys_parenttop;
extern int sys_parentwidth;
extern int sys_parentheight;
extern long    sys_parentwindow;

#define KEY_MASK (KeyPressMask | KeyReleaseMask)
#define MOUSE_MASK (ButtonPressMask | ButtonReleaseMask | \
		    PointerMotionMask)

#define X_MASK (KEY_MASK | MOUSE_MASK | ResizeRequest | StructureNotifyMask | FocusChangeMask | VisibilityChangeMask)

struct _XrmHashBucketRec;

static struct
{
	void *lib;
	int	 (*pXChangeProperty)(Display *display, Window w, Atom property, Atom type, int format, int mode, unsigned char *data, int nelements);
	int	 (*pXCloseDisplay)(Display *display);
	int 	 (*pXConvertSelection)(Display *display, Atom selection, Atom target, Atom property, Window requestor, Time time);
	Colormap (*pXCreateColormap)(Display *display, Window w, Visual *visual, int alloc);
	GC	 (*pXCreateGC)(Display *display, Drawable d, unsigned long valuemask, XGCValues *values);
	Pixmap	 (*pXCreatePixmap)(Display *display, Drawable d, unsigned int width, unsigned int height, unsigned int depth);
	Cursor	 (*pXCreatePixmapCursor)(Display *display, Pixmap source, Pixmap mask, XColor *foreground_color, XColor *background_color, unsigned int x, unsigned int y);
	Window	 (*pXCreateWindow)(Display *display, Window parent, int x, int y, unsigned int width, unsigned int height, unsigned int border_width, int depth, unsigned int class, Visual *visual, unsigned long valuemask, XSetWindowAttributes *attributes);
	int	 (*pXDefineCursor)(Display *display, Window w, Cursor cursor);
	int	 (*pXDestroyWindow)(Display *display, Window w);
	int	 (*pXFillRectangle)(Display *display, Drawable d, GC gc, int x, int y, unsigned int width, unsigned int height);
	int 	 (*pXFlush)(Display *display);
	int 	 (*pXFree)(void *data);
	int	 (*pXFreeCursor)(Display *display, Cursor cursor);
	void	 (*pXFreeEventData)(Display *display, XGenericEventCookie *cookie);
	int	 (*pXFreeGC)(Display *display, GC gc);
	int	 (*pXFreePixmap)(Display *display, Pixmap pixmap);
	char	*(*pXGetAtomName)(Display *display, Atom atom);
	Bool	 (*pXGetEventData)(Display *display, XGenericEventCookie *cookie);
	Window 	 (*pXGetSelectionOwner)(Display *display, Atom selection);
	Status	 (*pXGetWindowAttributes)(Display *display, Window w, XWindowAttributes *window_attributes_return);
	int	 (*pXGetWindowProperty)(Display *display, Window w, Atom property, long long_offset, long long_length, Bool delete, Atom req_type, Atom *actual_type_return, int *actual_format_return, unsigned long *nitems_return, unsigned long *bytes_after_return, unsigned char **prop_return);
	int	 (*pXGrabKeyboard)(Display *display, Window grab_window, Bool owner_events, int pointer_mode, int keyboard_mode, Time time);
	int	 (*pXGrabPointer)(Display *display, Window grab_window, Bool owner_events, unsigned int event_mask, int pointer_mode, int keyboard_mode, Window confine_to, Cursor cursor, Time time);
	Atom 	 (*pXInternAtom)(Display *display, char *atom_name, Bool only_if_exists);
	KeySym	 (*pXLookupKeysym)(XKeyEvent *key_event, int index);
	int	 (*pXLookupString)(XKeyEvent *event_struct, char *buffer_return, int bytes_buffer, KeySym *keysym_return, XComposeStatus *status_in_out);
	int	 (*pXMapWindow)(Display *display, Window w);
	int	 (*pXMoveResizeWindow)(Display *display, Window w, int x, int y, unsigned width, unsigned height);
	int	 (*pXMoveWindow)(Display *display, Window w, int x, int y);
	int	 (*pXNextEvent)(Display *display, XEvent *event_return);
	Display *(*pXOpenDisplay)(char *display_name);
	int 	 (*pXPending)(Display *display);
	Bool 	 (*pXQueryExtension)(Display *display, const char *name, int *major_opcode_return, int *first_event_return, int *first_error_return);
	int 	 (*pXRaiseWindow)(Display *display, Window w);
	int	 (*pXReparentWindow)(Display *display, Window w, Window parent, int x, int y);
	int	 (*pXResizeWindow)(Display *display, Window w, unsigned width, unsigned height);
	int	 (*pXSelectInput)(Display *display, Window w, long event_mask);
	Status 	 (*pXSendEvent)(Display *display, Window w, Bool propagate, long event_mask, XEvent *event_send);
	int	 (*pXSetIconName)(Display *display, Window w, char *icon_name);
	int	 (*pXSetInputFocus)(Display *display, Window focus, int revert_to, Time time);
	int 	 (*pXSetSelectionOwner)(Display *display, Atom selection, Window owner, Time time);
	void	 (*pXSetWMNormalHints)(Display *display, Window w, XSizeHints *hints);
	Status	 (*pXSetWMProtocols)(Display *display, Window w, Atom *protocols, int count);
	int	 (*pXStoreName)(Display *display, Window w, char *window_name);
	int 	 (*pXSync)(Display *display, Bool discard);
	int	 (*pXUndefineCursor)(Display *display, Window w);
	int	 (*pXUngrabKeyboard)(Display *display, Time time);
	int	 (*pXUngrabPointer)(Display *display, Time time);
	int 	 (*pXWarpPointer)(Display *display, Window src_w, Window dest_w, int src_x, int src_y, unsigned int src_width, unsigned int src_height, int dest_x, int dest_y);
	Status (*pXMatchVisualInfo)(Display *display, int screen, int depth, int class, XVisualInfo *vinfo_return);

	char *(*pXSetLocaleModifiers)(char *modifier_list);
	Bool (*pXSupportsLocale)(void); 
	XIM		(*pXOpenIM)(Display *display, struct _XrmHashBucketRec *db, char *res_name, char *res_class);
	char *	(*pXGetIMValues)(XIM im, ...); 
	XIC		(*pXCreateIC)(XIM im, ...);
	void	(*pXSetICFocus)(XIC ic); 
	char *  (*pXGetICValues)(XIC ic, ...); 
	Bool	(*pXFilterEvent)(XEvent *event, Window w);
	int		(*pXutf8LookupString)(XIC ic, XKeyPressedEvent *event, char *buffer_return, int bytes_buffer, KeySym *keysym_return, Status *status_return);
	int		(*pXwcLookupString)(XIC ic, XKeyPressedEvent *event, wchar_t *buffer_return, int bytes_buffer, KeySym *keysym_return, Status *status_return);
	void	(*pXDestroyIC)(XIC ic);
	Status	(*pXCloseIM)(XIM im);
	qboolean	dounicode;
	XIC			unicodecontext;
	XIM			inputmethod;
} x11;
static qboolean x11_initlib(void)
{
	dllfunction_t x11_functable[] =
	{
		{(void**)&x11.pXChangeProperty,		"XChangeProperty"},
		{(void**)&x11.pXCloseDisplay,		"XCloseDisplay"},
		{(void**)&x11.pXConvertSelection,	"XConvertSelection"},
		{(void**)&x11.pXCreateColormap,		"XCreateColormap"},
		{(void**)&x11.pXCreateGC,		"XCreateGC"},
		{(void**)&x11.pXCreatePixmap,		"XCreatePixmap"},
		{(void**)&x11.pXCreatePixmapCursor,	"XCreatePixmapCursor"},
		{(void**)&x11.pXCreateWindow,		"XCreateWindow"},
		{(void**)&x11.pXDefineCursor,		"XDefineCursor"},
		{(void**)&x11.pXDestroyWindow,		"XDestroyWindow"},
		{(void**)&x11.pXFillRectangle,		"XFillRectangle"},
		{(void**)&x11.pXFlush,			"XFlush"},
		{(void**)&x11.pXFree,			"XFree"},
		{(void**)&x11.pXFreeCursor,		"XFreeCursor"},
		{(void**)&x11.pXFreeGC,			"XFreeGC"},
		{(void**)&x11.pXFreePixmap,		"XFreePixmap"},
		{(void**)&x11.pXGetAtomName,		"XGetAtomName"},
		{(void**)&x11.pXGetSelectionOwner,	"XGetSelectionOwner"},
		{(void**)&x11.pXGetWindowAttributes,	"XGetWindowAttributes"},
		{(void**)&x11.pXGetWindowProperty,	"XGetWindowProperty"},
		{(void**)&x11.pXGrabKeyboard,		"XGrabKeyboard"},
		{(void**)&x11.pXGrabPointer,		"XGrabPointer"},
		{(void**)&x11.pXInternAtom,		"XInternAtom"},
		{(void**)&x11.pXLookupKeysym,		"XLookupKeysym"},
		{(void**)&x11.pXLookupString,		"XLookupString"},
		{(void**)&x11.pXMapWindow,		"XMapWindow"},
		{(void**)&x11.pXMoveResizeWindow,	"XMoveResizeWindow"},
		{(void**)&x11.pXMoveWindow,		"XMoveWindow"},
		{(void**)&x11.pXNextEvent,		"XNextEvent"},
		{(void**)&x11.pXOpenDisplay,		"XOpenDisplay"},
		{(void**)&x11.pXPending,		"XPending"},
		{(void**)&x11.pXQueryExtension,		"XQueryExtension"},
		{(void**)&x11.pXRaiseWindow,		"XRaiseWindow"},
		{(void**)&x11.pXReparentWindow,		"XReparentWindow"},
		{(void**)&x11.pXResizeWindow,		"XResizeWindow"},
		{(void**)&x11.pXSelectInput,		"XSelectInput"},
		{(void**)&x11.pXSendEvent,		"XSendEvent"},
		{(void**)&x11.pXSetIconName,		"XSetIconName"},
		{(void**)&x11.pXSetInputFocus,		"XSetInputFocus"},
		{(void**)&x11.pXSetSelectionOwner,	"XSetSelectionOwner"},
		{(void**)&x11.pXSetWMNormalHints,	"XSetWMNormalHints"},
		{(void**)&x11.pXSetWMProtocols,		"XSetWMProtocols"},
		{(void**)&x11.pXStoreName,		"XStoreName"},
		{(void**)&x11.pXSync,			"XSync"},
		{(void**)&x11.pXUndefineCursor,		"XUndefineCursor"},
		{(void**)&x11.pXUngrabKeyboard,		"XUngrabKeyboard"},
		{(void**)&x11.pXUngrabPointer,		"XUngrabPointer"},
		{(void**)&x11.pXWarpPointer,		"XWarpPointer"},
		{(void**)&x11.pXMatchVisualInfo,		"XMatchVisualInfo"},
		{NULL, NULL}
	};

	if (!x11.lib)
	{
#ifdef __CYGWIN__
		x11.lib = Sys_LoadLibrary("cygX11-6.dll", x11_functable);
#else
		x11.lib = Sys_LoadLibrary("libX11.so.6", x11_functable);
#endif
		if (!x11.lib)
			x11.lib = Sys_LoadLibrary("libX11", x11_functable);

		//these ones are extensions, and the reason we're doing this.
		if (x11.lib)
		{
			//raw input (yay mouse deltas)
			x11.pXGetEventData		= Sys_GetAddressForName(x11.lib, "XGetEventData");
			x11.pXFreeEventData		= Sys_GetAddressForName(x11.lib, "XFreeEventData");

			//internationalisation
			x11.pXSetLocaleModifiers = Sys_GetAddressForName(x11.lib, "XSetLocaleModifiers");
			x11.pXSupportsLocale	= Sys_GetAddressForName(x11.lib, "XSupportsLocale");
			x11.pXOpenIM			= Sys_GetAddressForName(x11.lib, "XOpenIM");
			x11.pXGetIMValues		= Sys_GetAddressForName(x11.lib, "XGetIMValues");
			x11.pXCreateIC			= Sys_GetAddressForName(x11.lib, "XCreateIC");
			x11.pXSetICFocus		= Sys_GetAddressForName(x11.lib, "XSetICFocus");
			x11.pXGetICValues		= Sys_GetAddressForName(x11.lib, "XGetICValues");
			x11.pXFilterEvent		= Sys_GetAddressForName(x11.lib, "XFilterEvent");
			x11.pXutf8LookupString	= Sys_GetAddressForName(x11.lib, "Xutf8LookupString");
			x11.pXwcLookupString	= Sys_GetAddressForName(x11.lib, "XwcLookupString");
			x11.pXDestroyIC			= Sys_GetAddressForName(x11.lib, "XDestroyIC");
			x11.pXCloseIM			= Sys_GetAddressForName(x11.lib, "XCloseIM");
		}
		else
		{
			Con_Printf("Unable to load libX11\n");
		}
	}
	

	return !!x11.lib;
}

#define FULLSCREEN_VMODE	1	//using xf86 vidmode (we can actually change modes)
#define FULLSCREEN_VMODEACTIVE	2	//xf86 vidmode currently forced
#define FULLSCREEN_LEGACY	4	//override redirect used
#define FULLSCREEN_WM		8	//fullscreen hint used
#define FULLSCREEN_ACTIVE	16	//currently fullscreen
static int fullscreenflags;
static int fullscreenwidth;
static int fullscreenheight;

void X_GoFullscreen(void);
void X_GoWindowed(void);
/*when alt-tabbing or whatever, the window manager creates a window, then destroys it again, resulting in weird focus events that trigger mode switches and grabs. using a timer reduces the weirdness and allows alt-tab to work properly. or at least better than it was working. that's the theory anyway*/
static unsigned int modeswitchtime;
static int modeswitchpending;

typedef struct
{
	unsigned int        dotclock;
	unsigned short      hdisplay;
	unsigned short      hsyncstart;
	unsigned short      hsyncend;
	unsigned short      htotal;
	unsigned short      hskew;
	unsigned short      vdisplay;
	unsigned short      vsyncstart;
	unsigned short      vsyncend;
	unsigned short      vtotal;
	unsigned int        flags;
} XF86VidModeModeInfo;	//we don't touch this struct

static struct
{
	int opcode, event, error;
	int vmajor, vminor;
	void *lib;
	Bool (*pXF86VidModeQueryVersion)(Display *dpy, int *majorVersion, int *minorVersion);
	Bool (*pXF86VidModeGetGammaRampSize)(Display *dpy, int screen, int *size);
	Bool (*pXF86VidModeGetGammaRamp)(Display *dpy, int screen, int size, unsigned short *red, unsigned short *green, unsigned short *blue);
	Bool (*pXF86VidModeSetGammaRamp)(Display *dpy, int screen, int size, unsigned short *red, unsigned short *green, unsigned short *blue);
	Bool (*pXF86VidModeSetViewPort)(Display *dpy, int screen, int x, int y);
	Bool (*pXF86VidModeSwitchToMode)(Display *dpy, int screen, XF86VidModeModeInfo *modeline);
	Bool (*pXF86VidModeGetAllModeLines)(Display *dpy, int screen, int *modecount, XF86VidModeModeInfo ***modelinesPtr);

	XF86VidModeModeInfo **modes;
	int num_modes;
	int usemode;
	unsigned short originalramps[3][256];
	qboolean originalapplied;	//states that the origionalramps arrays are valid, and contain stuff that we should revert to on close
} vm;
static qboolean VMODE_Init(void)
{
	dllfunction_t vm_functable[] =
	{
		{(void**)&vm.pXF86VidModeQueryVersion, "XF86VidModeQueryVersion"},
		{(void**)&vm.pXF86VidModeGetGammaRampSize, "XF86VidModeGetGammaRampSize"},
		{(void**)&vm.pXF86VidModeGetGammaRamp, "XF86VidModeGetGammaRamp"},
		{(void**)&vm.pXF86VidModeSetGammaRamp, "XF86VidModeSetGammaRamp"},
		{(void**)&vm.pXF86VidModeSetViewPort, "XF86VidModeSetViewPort"},
		{(void**)&vm.pXF86VidModeSwitchToMode, "XF86VidModeSwitchToMode"},
		{(void**)&vm.pXF86VidModeGetAllModeLines, "XF86VidModeGetAllModeLines"},
		{NULL, NULL}
	};
	vm.vmajor = 0;
	vm.vminor = 0;
	vm.usemode = -1;
	vm.originalapplied = false;

	if (COM_CheckParm("-novmode"))
		return false;

	if (!x11.pXQueryExtension(vid_dpy, "XFree86-VidModeExtension", &vm.opcode, &vm.event, &vm.error))
	{
		Con_Printf("VidModeExtension extension not available.\n");
		return false;
	}
	
	if (!vm.lib)
		vm.lib = Sys_LoadLibrary("libXxf86vm", vm_functable);

	if (vm.lib)
	{
		if (vm.pXF86VidModeQueryVersion(vid_dpy, &vm.vmajor, &vm.vminor))
			Con_Printf("Using XF86-VidModeExtension Ver. %d.%d\n", vm.vmajor, vm.vminor);
		else
		{
			Con_Printf("No XF86-VidModeExtension support\n");
			vm.vmajor = 0;
			vm.vminor = 0;
		}
	}

	return vm.vmajor;
}





extern cvar_t	_windowed_mouse;


static float old_windowed_mouse = 0;

static enum
{
	XIM_ORIG,
	XIM_DGA,
	XIM_XI2,
} x11_input_method;

#define XF86DGADirectMouse		0x0004
static struct
{
	int opcode, event, error;
	void *lib;
	Status (*pXF86DGADirectVideo) (Display *dpy, int screen, int enable);
} dgam;
static qboolean DGAM_Init(void)
{
	dllfunction_t dgam_functable[] =
	{
		{(void**)&dgam.pXF86DGADirectVideo, "XF86DGADirectVideo"},
		{NULL, NULL}
	};

	if (!x11.pXQueryExtension(vid_dpy, "XFree86-DGA", &dgam.opcode, &dgam.event, &dgam.error))
	{
		Con_Printf("DGA extension not available.\n");
		return false;
	}
	
	if (!dgam.lib)
		dgam.lib = Sys_LoadLibrary("libXxf86dga", dgam_functable);
	return !!dgam.lib;
}

#if 0
#include <X11/extensions/XInput2.h>
#else
#define XISetMask(ptr, event)   (((unsigned char*)(ptr))[(event)>>3] |=  (1 << ((event) & 7)))
#define XIMaskIsSet(ptr, event) (((unsigned char*)(ptr))[(event)>>3] &   (1 << ((event) & 7)))
#define XIMaskLen(event)        (((event + 7) >> 3))
typedef struct {
	int				mask_len;
	unsigned char	*mask;
	double			*values;
} XIValuatorState;
typedef struct
{
	int					deviceid;
	int					mask_len;
	unsigned char*		mask;
} XIEventMask;
#define XIAllMasterDevices 1
#define XI_RawButtonPress 15
#define XI_RawButtonRelease 16
#define XI_RawMotion 17
#define XI_LASTEVENT XI_RawMotion
typedef struct {
	int				type;			/* GenericEvent */
	unsigned long	serial;			/* # of last request processed by server */
	Bool			send_event;		/* true if this came from a SendEvent request */
	Display			*display;		/* Display the event was read from */
	int				extension;		/* XI extension offset */
	int				evtype;			/* XI_RawKeyPress, XI_RawKeyRelease, etc. */
	Time			time;
	int				deviceid;
	int				sourceid;		/* Bug: Always 0. https://bugs.freedesktop.org//show_bug.cgi?id=34240 */
	int				detail;
	int				flags;
	XIValuatorState	valuators;
	double			*raw_values;
} XIRawEvent;
#endif
static struct
{
	int opcode, event, error;
	int vmajor, vminor;
	void *libxi;

	Status (*pXIQueryVersion)( Display *display, int *major_version_inout, int *minor_version_inout);
	int (*pXISelectEvents)(Display *dpy, Window win, XIEventMask *masks, int num_masks);
} xi2;
static qboolean XI2_Init(void)
{
	dllfunction_t xi2_functable[] =
	{
		{(void**)&xi2.pXIQueryVersion, "XIQueryVersion"},
		{(void**)&xi2.pXISelectEvents, "XISelectEvents"},
		{NULL, NULL}
	};
	XIEventMask evm;
	unsigned char maskbuf[XIMaskLen(XI_LASTEVENT)];

	if (!x11.pXQueryExtension(vid_dpy, "XInputExtension", &xi2.opcode, &xi2.event, &xi2.error))
	{
		Con_Printf("XInput extension not available.\n");
		return false;
	}

	if (!xi2.libxi)
	{
#ifdef __CYGWIN__
		if (!xi2.libxi)
			xi2.libxi = Sys_LoadLibrary("cygXi-6.dll", xi2_functable);
#endif
		if (!xi2.libxi)
			xi2.libxi = Sys_LoadLibrary("libXi.so.6", xi2_functable);
		if (!xi2.libxi)
			xi2.libxi = Sys_LoadLibrary("libXi", xi2_functable);
		if (!xi2.libxi)
			Con_Printf("XInput library not available or too old.\n");
	}
	if (xi2.libxi)
	{
		xi2.vmajor = 2;
		xi2.vminor = 0;
		if (xi2.pXIQueryVersion(vid_dpy, &xi2.vmajor, &xi2.vminor))
		{
			Con_Printf("XInput library or server is too old\n");
			return false;
		}
		evm.deviceid = XIAllMasterDevices;
		evm.mask_len = sizeof(maskbuf);
		evm.mask = maskbuf;
		memset(maskbuf, 0, sizeof(maskbuf));
		XISetMask(maskbuf, XI_RawMotion);
		XISetMask(maskbuf, XI_RawButtonPress);
		XISetMask(maskbuf, XI_RawButtonRelease);
/*		if (xi2.vmajor >= 2 && xi2.vminor >= 2)
		{
			XISetMask(maskbuf, XI_RawTouchBegin);
			XISetMask(maskbuf, XI_RawTouchUpdate);
			XISetMask(maskbuf, XI_RawTouchEnd);
		}
*/		xi2.pXISelectEvents(vid_dpy, DefaultRootWindow(vid_dpy), &evm, 1);
		return true;
	}
	return false;
}

/*-----------------------------------------------------------------------*/

//qboolean is8bit = false;
//qboolean isPermedia = false;
extern qboolean sys_gracefulexit;

#define SYS_CLIPBOARD_SIZE 512
char clipboard_buffer[SYS_CLIPBOARD_SIZE];


/*-----------------------------------------------------------------------*/

static dllhandle_t *gllibrary;

XVisualInfo* (*qglXChooseVisual) (Display *dpy, int screen, int *attribList);
void (*qglXSwapBuffers) (Display *dpy, GLXDrawable drawable);
Bool (*qglXMakeCurrent) (Display *dpy, GLXDrawable drawable, GLXContext ctx);
GLXContext (*qglXCreateContext) (Display *dpy, XVisualInfo *vis, GLXContext shareList, Bool direct);
void (*qglXDestroyContext) (Display *dpy, GLXContext ctx);
void *(*qglXGetProcAddress) (char *name);

void GLX_CloseLibrary(void)
{
	Sys_CloseLibrary(gllibrary);
	gllibrary = NULL;
}

qboolean GLX_InitLibrary(char *driver)
{
	dllfunction_t funcs[] =
	{
		{(void*)&qglXChooseVisual,		"glXChooseVisual"},
		{(void*)&qglXSwapBuffers,		"glXSwapBuffers"},
		{(void*)&qglXMakeCurrent,		"glXMakeCurrent"},
		{(void*)&qglXCreateContext,		"glXCreateContext"},
		{(void*)&qglXDestroyContext,	"glXDestroyContext"},
		{NULL,							NULL}
	};

	if (driver && *driver)
		gllibrary = Sys_LoadLibrary(driver, funcs);
	else
		gllibrary = NULL;
#ifdef __CYGWIN__
	if (!gllibrary)
		gllibrary = Sys_LoadLibrary("cygGL-1.dll", funcs);
#endif
	if (!gllibrary)	//I hate this.
		gllibrary = Sys_LoadLibrary("libGL.so.1", funcs);
	if (!gllibrary)
		gllibrary = Sys_LoadLibrary("libGL", funcs);
	if (!gllibrary)
		return false;

	qglXGetProcAddress = Sys_GetAddressForName(gllibrary, "glXGetProcAddress");
	if (!qglXGetProcAddress)
		qglXGetProcAddress = Sys_GetAddressForName(gllibrary, "glXGetProcAddressARB");

	return true;
}

void *GLX_GetSymbol(char *name)
{
	void *symb;

	if (qglXGetProcAddress)
		symb = qglXGetProcAddress(name);
	else
		symb = NULL;

	if (!symb)
		symb = Sys_GetAddressForName(gllibrary, name);
	return symb;
}

static void X_ShutdownUnicode(void)
{
	if (x11.unicodecontext)
		x11.pXDestroyIC(x11.unicodecontext);
	x11.unicodecontext = NULL;
	if (x11.inputmethod)
		x11.pXCloseIM(x11.inputmethod);
	x11.inputmethod = NULL;
	x11.dounicode = false;
}
#include <locale.h>
static long X_InitUnicode(void)
{
	long requiredevents = 0;
	X_ShutdownUnicode();

	if (!COM_CheckParm("-noxim"))
	{
		if (x11.pXSetLocaleModifiers && x11.pXSupportsLocale && x11.pXOpenIM && x11.pXGetIMValues && x11.pXCreateIC && x11.pXSetICFocus && x11.pXGetICValues && x11.pXFilterEvent && (x11.pXutf8LookupString || x11.pXwcLookupString) && x11.pXDestroyIC && x11.pXCloseIM)
		{
			setlocale(LC_CTYPE, "");	//just in case.
			x11.pXSetLocaleModifiers("");
			if (x11.pXSupportsLocale())
			{
				x11.inputmethod = x11.pXOpenIM(vid_dpy, NULL, NULL, NULL);
				if (x11.inputmethod)
				{
					XIMStyles *sup = NULL;
					XIMStyle st = 0;
					int i;
					x11.pXGetIMValues(x11.inputmethod, XNQueryInputStyle, &sup, NULL);
					for (i = 0; sup && i < sup->count_styles; i++)
					{	//each style will have one of each bis set.
#define prestyles (XIMPreeditNothing|XIMPreeditNone)
#define statusstyles (XIMStatusNothing|XIMStatusNone)
#define supstyles (prestyles|statusstyles)
						if ((sup->supported_styles[i] & supstyles) != sup->supported_styles[i])
							continue;
						if ((st & prestyles) != (sup->supported_styles[i] & prestyles))
						{
							if ((sup->supported_styles[i] & XIMPreeditNothing) && !(st & XIMPreeditNothing))
								st = sup->supported_styles[i];
							else if ((sup->supported_styles[i] & XIMPreeditNone) && !(st & (XIMPreeditNone|XIMPreeditNothing)))
								st = sup->supported_styles[i];
						}
						else
						{
							if ((sup->supported_styles[i] & XIMStatusNothing) && !(st & XIMStatusNothing))
								st = sup->supported_styles[i];
							else if ((sup->supported_styles[i] & XIMStatusNone) && !(st & (XIMStatusNone|XIMStatusNothing)))
								st = sup->supported_styles[i];
						}
					}
					x11.pXFree(sup);
					if (st != 0)
					{
						x11.unicodecontext = x11.pXCreateIC(x11.inputmethod,
							XNInputStyle, st,
							XNClientWindow, vid_window,
							XNFocusWindow, vid_window,
							NULL);
						if (x11.unicodecontext)
						{
							x11.pXSetICFocus(x11.unicodecontext);
							x11.dounicode = true;

							x11.pXGetICValues(x11.unicodecontext, XNFilterEvents, &requiredevents, NULL);
						}
					}
				}
			}
			setlocale(LC_CTYPE, "C");
		}
	}

	Con_DPrintf("Unicode support: %s\n", x11.dounicode?"available":"unavailable");

	return requiredevents;
}

static void X_KeyEvent(XKeyEvent *ev, qboolean pressed, qboolean filtered)
{
	int i;
	int key;
	KeySym keysym, shifted;
	unsigned int unichar[64];
	int unichars = 0;
	key = 0;

	keysym = x11.pXLookupKeysym(ev, 0);
	if (pressed && !filtered)
	{
		if (x11.dounicode)
		{
			Status status = XLookupNone;
			if (x11.pXutf8LookupString)
			{
				char buf1[4] = {0};
				char *buf = buf1, *c;
				int count = x11.pXutf8LookupString(x11.unicodecontext, (XKeyPressedEvent*)ev, buf1, sizeof(buf1), NULL, &status);
				if (status == XBufferOverflow)
				{
					buf = alloca(count+1);
					count = x11.pXutf8LookupString(x11.unicodecontext, (XKeyPressedEvent*)ev, buf, count, NULL, &status);
				}
				for (c = buf; c < &buf[count]; )
				{
					int error;
					unsigned int uc = utf8_decode(&error, c, &c);
					if (uc)
						unichar[unichars++] = uc;
				}
			}
			else
			{
				//is allowed some weird encodings...
				wchar_t buf1[4] = {0};
				wchar_t *buf = buf1;
				int count = x11.pXwcLookupString(x11.unicodecontext, (XKeyPressedEvent*)ev, buf, sizeof(buf1), &shifted, &status);
				if (status == XBufferOverflow)
				{
					buf = alloca(sizeof(wchar_t)*(count+1));
					count = x11.pXwcLookupString(x11.unicodecontext, (XKeyPressedEvent*)ev, buf, count, NULL, &status);
				}
				//if wchar_t is 16bit, then expect problems when we completely ignore surrogates. this is why we favour the utf8 route as it doesn't care whether wchar_t is defined as 16bit or 32bit.
				for (i = 0; i < count; i++)
					if (buf[i])
						unichar[unichars++] = buf[i];
			}
		}
		else
		{
			char buf[64];
			if ((keysym & 0xff000000) == 0x01000000)
				unichar[unichars++] = keysym & 0x00ffffff;
			else
			{
				int count = x11.pXLookupString(ev, buf, sizeof(buf), &shifted, 0);
				for (i = 0; i < count; i++)
					if (buf[i])
						unichar[unichars++] = (unsigned char)buf[i];
			}
		}
	}

	switch(keysym)
	{
		case XK_KP_Page_Up:		key = K_KP_PGUP; break;
		case XK_Page_Up:		key = K_PGUP; break;

		case XK_KP_Page_Down:	key = K_KP_PGDN; break;
		case XK_Page_Down:		key = K_PGDN; break;

		case XK_KP_Home:		key = K_KP_HOME; break;
		case XK_Home:			key = K_HOME; break;

		case XK_KP_End:			key = K_KP_END; break;
		case XK_End:			key = K_END; break;

		case XK_KP_Left:		key = K_KP_LEFTARROW; break;
		case XK_Left:			key = K_LEFTARROW; break;

		case XK_KP_Right:		key = K_KP_RIGHTARROW; break;
		case XK_Right:			key = K_RIGHTARROW;		break;

		case XK_KP_Down:		key = K_KP_DOWNARROW; break;
		case XK_Down:			key = K_DOWNARROW; break;

		case XK_KP_Up:			key = K_KP_UPARROW; break;
		case XK_Up:				key = K_UPARROW;	 break;

		case XK_Escape:			key = K_ESCAPE;		break;

		case XK_KP_Enter:		key = K_KP_ENTER;	break;
		case XK_Return:			key = K_ENTER;		 break;

		case XK_Tab:			key = K_TAB;			 break;

		case XK_F1:				key = K_F1;				break;

		case XK_F2:				key = K_F2;				break;

		case XK_F3:				key = K_F3;				break;

		case XK_F4:				key = K_F4;				break;

		case XK_F5:				key = K_F5;				break;

		case XK_F6:				key = K_F6;				break;

		case XK_F7:				key = K_F7;				break;

		case XK_F8:				key = K_F8;				break;

		case XK_F9:				key = K_F9;				break;

		case XK_F10:			key = K_F10;			 break;

		case XK_F11:			key = K_F11;			 break;

		case XK_F12:			key = K_F12;			 break;

		case XK_BackSpace:		key = K_BACKSPACE; break;

		case XK_KP_Delete:		key = K_KP_DEL; break;
		case XK_Delete:			key = K_DEL; break;

		case XK_Pause:			key = K_PAUSE;		 break;

		case XK_Shift_L:		key = K_LSHIFT;		break;
		case XK_Shift_R:		key = K_RSHIFT;		break;

		case XK_Execute:		key = K_LCTRL;		break;
		case XK_Control_L:		key = K_LCTRL;		break;
		case XK_Control_R:		key = K_RCTRL;		 break;

		case XK_Alt_L:			key = K_LALT;			break;
		case XK_Meta_L:			key = K_LALT;			break;
		case XK_Alt_R:			key = K_RALT;			break;
		case XK_Meta_R:			key = K_RALT;			break;

		case XK_KP_Begin:		key = K_KP_5;	break;

		case XK_KP_Insert:		key = K_KP_INS; break;
		case XK_Insert:			key = K_INS; break;

		case XK_KP_Multiply:	key = K_KP_STAR; break;
		case XK_KP_Add:			key = K_KP_PLUS; break;
		case XK_KP_Subtract:	key = K_KP_MINUS; break;
		case XK_KP_Divide:		key = K_KP_SLASH; break;

#if 0
		case 0x021: key = '1';break;/* [!] */
		case 0x040: key = '2';break;/* [@] */
		case 0x023: key = '3';break;/* [#] */
		case 0x024: key = '4';break;/* [$] */
		case 0x025: key = '5';break;/* [%] */
		case 0x05e: key = '6';break;/* [^] */
		case 0x026: key = '7';break;/* [&] */
		case 0x02a: key = '8';break;/* [*] */
		case 0x028: key = '9';;break;/* [(] */
		case 0x029: key = '0';break;/* [)] */
		case 0x05f: key = '-';break;/* [_] */
		case 0x02b: key = '=';break;/* [+] */
		case 0x07c: key = '\'';break;/* [|] */
		case 0x07d: key = '[';break;/* [}] */
		case 0x07b: key = ']';break;/* [{] */
		case 0x022: key = '\'';break;/* ["] */
		case 0x03a: key = ';';break;/* [:] */
		case 0x03f: key = '/';break;/* [?] */
		case 0x03e: key = '.';break;/* [>] */
		case 0x03c: key = ',';break;/* [<] */
#endif

		default:
			key = keysym;
			if (key < 32)
				key = 0;
			else if (key > 127)
				key = 0;
			else if (key >= 'A' && key <= 'Z')
				key = key - 'A' + 'a';
			break;
	}

	if (unichars)
	{
		//we got some text, this is fun isn't it?
		//the key value itself is sent with the last text char. this avoids multiple presses, and dead keys were already sent.
		for (i = 0; i < unichars-1; i++)
		{
			IN_KeyEvent(0, pressed, 0, unichar[i]);
		}
		IN_KeyEvent(0, pressed, key, unichar[i]);
	}
	else
	{
		//no text available, just do the keypress
		IN_KeyEvent(0, pressed, key, 0);
	}
}

static void install_grabs(void)
{
	//XGrabPointer can cause alt+tab type shortcuts to be skipped by the window manager. This means we don't want to use it unless we have no choice.
	//the grab is purely to constrain the pointer to the window
	if (GrabSuccess != x11.pXGrabPointer(vid_dpy, DefaultRootWindow(vid_dpy),
				True,
				0,
				GrabModeAsync, GrabModeAsync,
				vid_window,
				None,
				CurrentTime))
		Con_Printf("Pointer grab failed\n");

	if (x11_input_method == XIM_DGA)
	{
		dgam.pXF86DGADirectVideo(vid_dpy, DefaultScreen(vid_dpy), XF86DGADirectMouse);
	}
	else
	{
		x11.pXWarpPointer(vid_dpy, None, vid_window,
					 0, 0, 0, 0,
					 vid.width / 2, vid.height / 2);
	}

//	x11.pXSync(vid_dpy, True);
}

static void uninstall_grabs(void)
{
	if (x11_input_method == XIM_DGA)
	{
		dgam.pXF86DGADirectVideo(vid_dpy, DefaultScreen(vid_dpy), 0);
	}

	if (vid_dpy)
		x11.pXUngrabPointer(vid_dpy, CurrentTime);

//	x11.pXSync(vid_dpy, True);
}

static void ClearAllStates (void)
{
	int		i;

// send an up event for each key, to make sure the server clears them all
	for (i=0 ; i<256 ; i++)
	{
		Key_Event (0, i, 0, false);
	}

	Key_ClearStates ();
//	IN_ClearStates ();
}

static void GetEvent(void)
{
	XEvent event, rep;
	int b;
	qboolean x11violations = true;
	Window mw;
	qboolean filtered = false;

	x11.pXNextEvent(vid_dpy, &event);

	if (x11.dounicode)
		if (x11.pXFilterEvent(&event, vid_window))
			filtered = true;

	switch (event.type)
	{
	case GenericEvent:
		if (x11.pXGetEventData(vid_dpy, &event.xcookie))
		{
			if (event.xcookie.extension == xi2.opcode)
			{
				switch(event.xcookie.evtype)
				{
				case XI_RawButtonPress:
				case XI_RawButtonRelease:
					if (old_windowed_mouse)
					{
						XIRawEvent *raw = event.xcookie.data;
						int button = raw->detail;	//1-based
						switch(button)
						{
						case 1: button = K_MOUSE1; break;
						case 2: button = K_MOUSE3; break;
						case 3: button = K_MOUSE2; break;
						case 4: button = K_MWHEELUP; break;	//so much for 'raw'.
						case 5: button = K_MWHEELDOWN; break;
						case 6: button = K_MOUSE4; break;
						case 7: button = K_MOUSE5; break;
						case 8: button = K_MOUSE6; break;
						case 9: button = K_MOUSE7; break;
						case 10: button = K_MOUSE8; break;
						case 11: button = K_MOUSE9; break;
						case 12: button = K_MOUSE10; break;
						default:button = 0; break;
						}
						if (button)
				                        IN_KeyEvent(raw->deviceid, (event.xcookie.evtype==XI_RawButtonPress), button, 0);
					}
					break;
				case XI_RawMotion:
					if (old_windowed_mouse)
					{
						XIRawEvent *raw = event.xcookie.data;
						double *val, *raw_val;
						double rawx = 0, rawy = 0;
						int i;
						val = raw->valuators.values;
						raw_val = raw->raw_values;
						for (i = 0; i < raw->valuators.mask_len * 8; i++)
						{
							if (XIMaskIsSet(raw->valuators.mask, i))
							{
								if (i == 0) rawx = *raw_val;
								if (i == 1) rawy = *raw_val;
								val++;
								raw_val++;
							}
						}
						IN_MouseMove(raw->deviceid, false, rawx, rawy, 0, 0);
					}
					break;
				default:
					Con_Printf("Unknown xinput event %u!\n", event.xcookie.evtype);
					break;
				}
			}
			else
				Con_Printf("Unknown generic event!\n");
		}
		x11.pXFreeEventData(vid_dpy, &event.xcookie);
		break;
	case ResizeRequest:
		vid.pixelwidth = event.xresizerequest.width;
		vid.pixelheight = event.xresizerequest.height;
		Cvar_ForceCallback(&vid_conautoscale);
//		if (fullscreenflags & FULLSCREEN_ACTIVE)
//			x11.pXMoveWindow(vid_dpy, vid_window, 0, 0);
		break;
	case ConfigureNotify:
		if (event.xconfigurerequest.window == vid_window)
		{
			vid.pixelwidth = event.xconfigurerequest.width;
			vid.pixelheight = event.xconfigurerequest.height;
			Cvar_ForceCallback(&vid_conautoscale);
		}
		else if (event.xconfigurerequest.window == vid_decoywindow)
		{
			if (!(fullscreenflags & FULLSCREEN_ACTIVE))
				x11.pXResizeWindow(vid_dpy, vid_window, event.xconfigurerequest.width, event.xconfigurerequest.height);
		}
//		if (fullscreenflags & FULLSCREEN_ACTIVE)
//			x11.pXMoveWindow(vid_dpy, vid_window, 0, 0);
		break;
	case KeyPress:
		X_KeyEvent(&event.xkey, true, filtered);
		break;
	case KeyRelease:
		X_KeyEvent(&event.xkey, false, filtered);
		break;

	case MotionNotify:
		if (x11_input_method == XIM_DGA && old_windowed_mouse)
		{
			IN_MouseMove(0, false, event.xmotion.x_root, event.xmotion.y_root, 0, 0);
		}
		else
		{
			if (old_windowed_mouse)
			{
				if (x11_input_method != XIM_XI2)
				{
					int cx = vid.pixelwidth/2, cy=vid.pixelheight/2;

					IN_MouseMove(0, false, event.xmotion.x - cx, event.xmotion.y - cy, 0, 0);

					/* move the mouse to the window center again (disabling warp first so we don't see it*/
					x11.pXSelectInput(vid_dpy, vid_window, vid_x_eventmask & ~PointerMotionMask);
					x11.pXWarpPointer(vid_dpy, None, vid_window, 0, 0, 0, 0,
						cx, cy);
					x11.pXSelectInput(vid_dpy, vid_window, vid_x_eventmask);
				}
			}
			else
			{
				IN_MouseMove(0, true, event.xmotion.x, event.xmotion.y, 0, 0);
			}
		}
		break;

	case ButtonPress:
		if (x11_input_method == XIM_XI2 && old_windowed_mouse)
			break;	//no dupes!
		b=-1;
		if (event.xbutton.button == 1)
			b = K_MOUSE1;
		else if (event.xbutton.button == 2)
			b = K_MOUSE3;
		else if (event.xbutton.button == 3)
			b = K_MOUSE2;
		//note, the x11 protocol does not support a mousewheel
		//we only support it because we follow convention. the actual protocol specifies 4+5 as regular buttons
		else if (event.xbutton.button == 4)
			b = x11violations?K_MWHEELUP:K_MOUSE4;
		else if (event.xbutton.button == 5)
			b = x11violations?K_MWHEELDOWN:K_MOUSE5;
		//note, the x11 protocol does not support more than 5 mouse buttons
		//which is a bit of a shame, but hey.
		else if (event.xbutton.button == 6)
			b = x11violations?K_MOUSE4:-1;
		else if (event.xbutton.button == 7)
			b = x11violations?K_MOUSE5:-1;
		else if (event.xbutton.button == 8)
			b = x11violations?K_MOUSE6:-1;
		else if (event.xbutton.button == 9)
			b = x11violations?K_MOUSE7:-1;
		else if (event.xbutton.button == 10)
			b = x11violations?K_MOUSE8:-1;
		else if (event.xbutton.button == 11)
			b = x11violations?K_MOUSE9:-1;
		else if (event.xbutton.button == 12)
			b = x11violations?K_MOUSE10:-1;

		if (b>=0)
			IN_KeyEvent(0, true, b, 0);

/*
		if (fullscreenflags & FULLSCREEN_LEGACY)
		if (fullscreenflags & FULLSCREEN_VMODE)
		if (!vid.activeapp)
		{	//KDE doesn't seem to like us, in that you can't alt-tab back or click to activate.
			//This allows us to steal input focus back from the window manager
			x11.pXSetInputFocus(vid_dpy, vid_window, RevertToParent, CurrentTime);
		}
*/
		break;

	case ButtonRelease:
		b=-1;
		if (event.xbutton.button == 1)
			b = K_MOUSE1;
		else if (event.xbutton.button == 2)
			b = K_MOUSE3;
		else if (event.xbutton.button == 3)
			b = K_MOUSE2;
		//note, the x11 protocol does not support a mousewheel
		//we only support it because we follow convention. the actual protocol specifies 4+5 as regular buttons
		else if (event.xbutton.button == 4)
			b = x11violations?K_MWHEELUP:K_MOUSE4;
		else if (event.xbutton.button == 5)
			b = x11violations?K_MWHEELDOWN:K_MOUSE5;
		//note, the x11 protocol does not support more than 5 mouse buttons
		//which is a bit of a shame, but hey.
		else if (event.xbutton.button == 6)
			b = x11violations?K_MOUSE4:-1;
		else if (event.xbutton.button == 7)
			b = x11violations?K_MOUSE5:-1;
		else if (event.xbutton.button == 8)
			b = x11violations?K_MOUSE6:-1;
		else if (event.xbutton.button == 9)
			b = x11violations?K_MOUSE7:-1;
		else if (event.xbutton.button == 10)
			b = x11violations?K_MOUSE8:-1;
		else if (event.xbutton.button == 11)
			b = x11violations?K_MOUSE9:-1;
		else if (event.xbutton.button == 12)
			b = x11violations?K_MOUSE10:-1;

		if (b>=0)
			IN_KeyEvent(0, false, b, 0);
		break;

	case FocusIn:
		//activeapp is if the game window is focused
		vid.activeapp = true;

		//but change modes to track the desktop window
//		if (!(fullscreenflags & FULLSCREEN_ACTIVE) || event.xfocus.window != vid_decoywindow)
		{
			modeswitchpending = 1;
			modeswitchtime = Sys_Milliseconds() + 1500;	/*fairly slow, to make sure*/
		}

		//we we're focusing onto the game window and we're currently fullscreen, hide the other one so alt-tab won't select that instead of a real alternate app.
//		if ((fullscreenflags & FULLSCREEN_ACTIVE) && (fullscreenflags & FULLSCREEN_LEGACY) && event.xfocus.window == vid_window)
//			x11.pXUnmapWindow(vid_dpy, vid_decoywindow);
		break;
	case FocusOut:
		//if we're already active, the decoy window shouldn't be focused anyway.
		if ((fullscreenflags & FULLSCREEN_ACTIVE) && event.xfocus.window == vid_decoywindow)
		{
			break;
		}

		if (vm.originalapplied)
			vm.pXF86VidModeSetGammaRamp(vid_dpy, scrnum, 256, vm.originalramps[0], vm.originalramps[1], vm.originalramps[2]);

		mw = vid_window;
		if ((fullscreenflags & FULLSCREEN_LEGACY) && (fullscreenflags & FULLSCREEN_ACTIVE))
			mw = vid_decoywindow;

		if (event.xfocus.window == mw || event.xfocus.window == vid_window)
		{
			vid.activeapp = false;
			if (old_windowed_mouse)
			{
				Con_DPrintf("uninstall grabs\n");
				uninstall_grabs();
				x11.pXUndefineCursor(vid_dpy, vid_window);
				old_windowed_mouse = false;
			}
			ClearAllStates();
		}
		modeswitchpending = -1;
		modeswitchtime = Sys_Milliseconds() + 100;	/*fairly fast, so we don't unapply stuff when switching to other progs with delays*/
		break;
	case ClientMessage:
		{
			char *name = x11.pXGetAtomName(vid_dpy, event.xclient.message_type);
			if (!strcmp(name, "WM_PROTOCOLS") && event.xclient.format == 32)
			{
				char *protname = x11.pXGetAtomName(vid_dpy, event.xclient.data.l[0]);
				if (!strcmp(protname, "WM_DELETE_WINDOW"))
				{
					Cmd_ExecuteString("menu_quit prompt", RESTRICT_LOCAL);
					x11.pXSetInputFocus(vid_dpy, vid_window, RevertToParent, CurrentTime);
				}
				else
					Con_Printf("Got message %s\n", protname);
				x11.pXFree(protname);
			}
			else
				Con_Printf("Got message %s\n", name);
			x11.pXFree(name);
		}
		break;

#if 1
	case SelectionRequest:	//needed for copy-to-clipboard
		{
			Atom xa_string = x11.pXInternAtom(vid_dpy, "UTF8_STRING", false);
			memset(&rep, 0, sizeof(rep));
			if (event.xselectionrequest.property == None)
				event.xselectionrequest.property = x11.pXInternAtom(vid_dpy, "foobar2000", false);
			if (event.xselectionrequest.property != None && event.xselectionrequest.target == xa_string)
			{
				x11.pXChangeProperty(vid_dpy, event.xselectionrequest.requestor, event.xselectionrequest.property, event.xselectionrequest.target, 8, PropModeReplace, (void*)clipboard_buffer, strlen(clipboard_buffer));
				rep.xselection.property = event.xselectionrequest.property;
			}
			else
			{
				rep.xselection.property = None;
			}
			rep.xselection.type = SelectionNotify;
			rep.xselection.serial = 0;
			rep.xselection.send_event = true;
			rep.xselection.display = rep.xselection.display;
			rep.xselection.requestor = event.xselectionrequest.requestor;
			rep.xselection.selection = event.xselectionrequest.selection;
			rep.xselection.target = event.xselectionrequest.target;
			rep.xselection.time = event.xselectionrequest.time;
			x11.pXSendEvent(vid_dpy, event.xselectionrequest.requestor, 0, 0, &rep);
		}
		break;
#endif

	default:
//		Con_Printf("%x\n", event.type);
		break;
	}
}


void GLVID_Shutdown(void)
{
	printf("GLVID_Shutdown\n");
	if (!vid_dpy)
		return;

	x11.pXUngrabKeyboard(vid_dpy, CurrentTime);
	if (old_windowed_mouse)
		uninstall_grabs();

	if (vm.originalapplied)
		vm.pXF86VidModeSetGammaRamp(vid_dpy, scrnum, 256, vm.originalramps[0], vm.originalramps[1], vm.originalramps[2]);

	X_ShutdownUnicode();

	switch(currentpsl)
	{
#ifdef USE_EGL
	case PSL_EGL:
		EGL_Shutdown();
		break;
#endif
	case PSL_GLX:
		if (ctx)
		{
			qglXDestroyContext(vid_dpy, ctx);
			ctx = NULL;
		}
		break;
	case PSL_NONE:
		break;
	}

	if (vid_window)
		x11.pXDestroyWindow(vid_dpy, vid_window);
	if (vid_nullcursor)
		x11.pXFreeCursor(vid_dpy, vid_nullcursor);
	if (vid_dpy)
	{
		if (fullscreenflags & FULLSCREEN_VMODEACTIVE)
			vm.pXF86VidModeSwitchToMode(vid_dpy, scrnum, vm.modes[0]);
		fullscreenflags &= ~FULLSCREEN_VMODEACTIVE;

		if (vm.modes)
			x11.pXFree(vm.modes);
		vm.modes = NULL;
		vm.num_modes = 0;
	}
	x11.pXCloseDisplay(vid_dpy);
	vid_dpy = NULL;
	vid_window = (Window)NULL;
	currentpsl = PSL_NONE;
}

void GLVID_DeInit(void)	//FIXME:....
{
	GLVID_Shutdown();
}

static Cursor CreateNullCursor(Display *display, Window root)
{
	Pixmap cursormask;
	XGCValues xgc;
	GC gc;
	XColor dummycolour;
	Cursor cursor;

	cursormask = x11.pXCreatePixmap(display, root, 1, 1, 1/*depth*/);
	xgc.function = GXclear;
	gc =  x11.pXCreateGC(display, cursormask, GCFunction, &xgc);
	x11.pXFillRectangle(display, cursormask, gc, 0, 0, 1, 1);
	dummycolour.pixel = 0;
	dummycolour.red = 0;
	dummycolour.flags = 04;
	cursor = x11.pXCreatePixmapCursor(display, cursormask, cursormask,
		&dummycolour,&dummycolour, 0,0);
	x11.pXFreePixmap(display,cursormask);
	x11.pXFreeGC(display,gc);
	return cursor;
}

qboolean GLVID_ApplyGammaRamps(unsigned short *ramps)
{
	extern qboolean gammaworks;
	//extern cvar_t vid_hardwaregamma;

	//if we don't know the original ramps yet, don't allow changing them, because we're probably invalid anyway, and even if it worked, it'll break something later.
	if (!vm.originalapplied)
		return false;

	if (ramps)
	{
		//hardwaregamma==1 skips hardware gamma when we're not fullscreen, in favour of software glsl based gamma.
//		if (vid_hardwaregamma.value == 1 && !vid.activeapp && !(fullscreenflags & FULLSCREEN_ACTIVE))
//			return false;
//		if (!vid.activeapp)
//			return false;
//		if (!vid_hardwaregamma.value)
//			return false;
	
		//we have hardware gamma applied - if we're doing a BF, we don't want to reset to the default gamma if it randomly fails (yuck)
		if (gammaworks)
			vm.pXF86VidModeSetGammaRamp (vid_dpy, scrnum, 256, &ramps[0], &ramps[256], &ramps[512]);
		else
			gammaworks = !!vm.pXF86VidModeSetGammaRamp (vid_dpy, scrnum, 256, &ramps[0], &ramps[256], &ramps[512]);

		return gammaworks;
	}
	else
	{
		vm.pXF86VidModeSetGammaRamp(vid_dpy, scrnum, 256, vm.originalramps[0], vm.originalramps[1], vm.originalramps[2]);
		return true;
	}
}

void GLVID_SwapBuffers (void)
{
	switch(currentpsl)
	{
#ifdef USE_EGL
	case PSL_EGL:
		EGL_BeginRendering();
		break;
#endif
	case PSL_GLX:
		//we don't need to flush, XSawpBuffers does it for us.
		//chances are, it's version is more suitable anyway. At least there's the chance that it might be.
		qglXSwapBuffers(vid_dpy, vid_window);
		break;
	default:
	case PSL_NONE:
		break;
	}
}

#include "bymorphed.h"
void X_StoreIcon(Window wnd)
{
	int i;
	unsigned long data[64*64+2];
	unsigned int *indata = (unsigned int*)icon.pixel_data;
	unsigned int inwidth = icon.width;
	unsigned int inheight = icon.height;

	//FIXME: support loading an icon from the filesystem.

	Atom propname = x11.pXInternAtom(vid_dpy, "_NET_WM_ICON", false);
	Atom proptype = x11.pXInternAtom(vid_dpy, "CARDINAL", false);

	data[0] = inwidth;
	data[1] = inheight;
	for (i = 0; i < data[0]*data[1]; i++)
		data[i+2] = indata[i];

	x11.pXChangeProperty(vid_dpy, wnd, propname, proptype, 32, PropModeReplace, (void*)data, data[0]*data[1]+2);
}

void X_GoFullscreen(void)
{
	XEvent xev;
	
	//for NETWM window managers
	memset(&xev, 0, sizeof(xev));
	xev.type = ClientMessage;
	xev.xclient.window = vid_window;
	xev.xclient.message_type = x11.pXInternAtom(vid_dpy, "_NET_WM_STATE", False);
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 1;	//add
	xev.xclient.data.l[1] = x11.pXInternAtom(vid_dpy, "_NET_WM_STATE_FULLSCREEN", False);
	xev.xclient.data.l[2] = 0;

	//for any other window managers, and broken NETWM
	x11.pXMoveResizeWindow(vid_dpy, vid_window, 0, 0, fullscreenwidth, fullscreenheight);
	x11.pXSync(vid_dpy, False);
	x11.pXSendEvent(vid_dpy, DefaultRootWindow(vid_dpy), False, SubstructureNotifyMask, &xev);
	x11.pXSync(vid_dpy, False);
	x11.pXMoveResizeWindow(vid_dpy, vid_window, 0, 0, fullscreenwidth, fullscreenheight);
	x11.pXSync(vid_dpy, False);

Con_Printf("Gone fullscreen\n");
}
void X_GoWindowed(void)
{
	XEvent xev;
	x11.pXFlush(vid_dpy);
	x11.pXSync(vid_dpy, False);

	memset(&xev, 0, sizeof(xev));
	xev.type = ClientMessage;
	xev.xclient.window = vid_window;
	xev.xclient.message_type = x11.pXInternAtom(vid_dpy, "_NET_WM_STATE", False);
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 0;	//remove
	xev.xclient.data.l[1] = x11.pXInternAtom(vid_dpy, "_NET_WM_STATE_FULLSCREEN", False);
	xev.xclient.data.l[2] = 0;
	x11.pXSendEvent(vid_dpy, DefaultRootWindow(vid_dpy), False, SubstructureNotifyMask, &xev);
	x11.pXSync(vid_dpy, False);

	x11.pXMoveResizeWindow(vid_dpy, vid_window, 0, 0, 640, 480);
Con_Printf("Gone windowed\n");
}
qboolean X_CheckWMFullscreenAvailable(void)
{
	//root window must have _NET_SUPPORTING_WM_CHECK which is a Window created by the WM
	//the WM's window must have _NET_WM_NAME set, which is the name of the window manager
	//if we can find those, then the window manager has not crashed.
	//if we can then find _NET_WM_STATE_FULLSCREEN in the _NET_SUPPORTED atom list on the root, then we can get fullscreen mode from the WM
	//and we'll have no alt-tab issues whatsoever.

	Atom xa_net_supporting_wm_check = x11.pXInternAtom(vid_dpy, "_NET_SUPPORTING_WM_CHECK", False);
	Atom xa_net_wm_name = x11.pXInternAtom(vid_dpy, "_NET_WM_NAME", False);
	Atom xa_net_supported = x11.pXInternAtom(vid_dpy, "_NET_SUPPORTED", False);
	Atom xa_net_wm_state_fullscreen = x11.pXInternAtom(vid_dpy, "_NET_WM_STATE_FULLSCREEN", False);
	Window wmwindow;
	unsigned char *prop;
	unsigned long bytes_after, nitems;
	Atom type;
	int format;
	qboolean success = false;
	unsigned char *wmname;
	int i;

	if (COM_CheckParm("-nowmfullscreen"))
	{
		Con_Printf("Window manager fullscreen support disabled. Will attempt to hide from it instead.\n");
		return success;
	}
	

	if (x11.pXGetWindowProperty(vid_dpy, vid_root, xa_net_supporting_wm_check, 0, 16384, False, AnyPropertyType, &type, &format, &nitems, &bytes_after, &prop) != Success || prop == NULL)
	{
		Con_Printf("Window manager not identified\n");
		return success;
	}
	wmwindow = *(Window *)prop;
	x11.pXFree(prop);
	
	if (x11.pXGetWindowProperty(vid_dpy, wmwindow, xa_net_wm_name, 0, 16384, False, AnyPropertyType, &type, &format, &nitems, &bytes_after, &wmname) != Success || wmname == NULL)
	{
		Con_Printf("Window manager crashed or something\n");
		return success;
	}
	else
	{
		if (x11.pXGetWindowProperty(vid_dpy, vid_root, xa_net_supported, 0, 16384, False, AnyPropertyType, &type, &format, &nitems, &bytes_after, &prop) != Success || prop == NULL)
		{
			Con_Printf("Window manager \"%s\" support nothing\n", wmname);
		}
		else
		{
			for (i = 0; i < nitems; i++)
			{
//				Con_Printf("supported: %s\n", x11.pXGetAtomName(vid_dpy, ((Atom*)prop)[i]));
				if (((Atom*)prop)[i] == xa_net_wm_state_fullscreen)
				{
					success = true;
					break;
				}
			}
			if (!success)
				Con_Printf("Window manager \"%s\" does not appear to support fullscreen\n", wmname);
			else if (!strcmp(wmname, "Fluxbox"))
			{
				Con_Printf("Window manager \"%s\" claims to support fullscreen, but is known buggy\n", wmname);
				success = false;
			}
			else
				Con_Printf("Window manager \"%s\" supports fullscreen\n", wmname);
			x11.pXFree(prop);
		}
		x11.pXFree(wmname);
	}
	return success;
}

Window X_CreateWindow(qboolean override, XVisualInfo *visinfo, unsigned int width, unsigned int height, qboolean fullscreen)
{
	Window wnd, parent;
	XSetWindowAttributes attr;
	XSizeHints szhints;
	unsigned int mask;
	Atom prots[1];
	int x, y;

	/* window attributes */
	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attr.colormap = x11.pXCreateColormap(vid_dpy, vid_root, visinfo->visual, AllocNone);
	attr.event_mask = vid_x_eventmask = X_MASK;
	attr.backing_store = NotUseful;
	attr.save_under = False;
	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask | CWBackingStore |CWSaveUnder;

	// override redirect prevents the windowmanager from finding out about us, and thus will not apply borders to our window.
	if (override)
	{
		mask |= CWOverrideRedirect;
		attr.override_redirect = True;
	}

	memset(&szhints, 0, sizeof(szhints));
	szhints.flags = PMinSize;
	szhints.min_width = 320;
	szhints.min_height = 200;
	szhints.x = 0;
	szhints.y = 0;
	szhints.width = width;
	szhints.height = height;


	if (sys_parentwindow && !fullscreen)
	{
		x = (sys_parentwidth - width) / 2;
		y = (sys_parentheight - height) / 2;
		parent = sys_parentwindow;
	}
	else
	{
		parent = vid_root;
		x = 0;
		y = 0;
	}

	wnd = x11.pXCreateWindow(vid_dpy, parent, x, y, width, height,
						0, visinfo->depth, InputOutput,
						visinfo->visual, mask, &attr);
	/*ask the window manager to stop triggering bugs in Xlib*/
	prots[0] = x11.pXInternAtom(vid_dpy, "WM_DELETE_WINDOW", False);
	x11.pXSetWMProtocols(vid_dpy, wnd, prots, sizeof(prots)/sizeof(prots[0]));
	x11.pXSetWMNormalHints(vid_dpy, wnd, &szhints);
	/*set caption*/
	x11.pXStoreName(vid_dpy, wnd, "FTE QuakeWorld");
	x11.pXSetIconName(vid_dpy, wnd, "FTEQW");
	X_StoreIcon(wnd);
	/*make it visible*/
	x11.pXMapWindow(vid_dpy, wnd);

	return wnd;
}

qboolean X11VID_Init (rendererstate_t *info, unsigned char *palette, int psl)
{
	int width = info->width;	//can override these if vmode isn't available
	int height = info->height;
	int rate = info->rate;
	int i;
	int attrib[] = {
		GLX_RGBA,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		GLX_DOUBLEBUFFER,
		GLX_DEPTH_SIZE, 1,
		GLX_STENCIL_SIZE, 8,
		None
	};
#ifdef USE_EGL
	XVisualInfo vinfodef;
#endif
	XVisualInfo *visinfo;
	qboolean fullscreen = false;

	if (!x11_initlib())
		return false;

	if (info->fullscreen)
		fullscreen = true;

	currentpsl = psl;

	switch(currentpsl)
	{
#ifdef USE_EGL
	case PSL_EGL:
		if (!EGL_LoadLibrary(info->subrenderer))
		{
			Con_Printf("couldn't load EGL library\n");
			return false;
		}
		break;
#endif
	case PSL_GLX:
		if (!GLX_InitLibrary(info->subrenderer))
		{
			Con_Printf("Couldn't intialise GLX\nEither your drivers are not installed or you need to specify the library name with the gl_driver cvar\n");
			return false;
		}
		break;
	case PSL_NONE:
		return false;
	}

	if (!vid_dpy)
		vid_dpy = x11.pXOpenDisplay(NULL);
	if (!vid_dpy)
	{
		Con_Printf(CON_ERROR "Error: couldn't open the X display\n");
		return false;
	}

	scrnum = DefaultScreen(vid_dpy);
	vid_root = RootWindow(vid_dpy, scrnum);

	VMODE_Init();

	fullscreenflags = 0;

	vm.usemode = -1;
	if (vm.vmajor && !COM_CheckParm("-current"))
	{
		int best_fit, best_dist, dist, x, y, z, r;

		vm.pXF86VidModeGetAllModeLines(vid_dpy, scrnum, &vm.num_modes, &vm.modes);
		// Are we going fullscreen?  If so, let's change video mode
		if (fullscreen)
		{
			best_dist = 9999999;
			best_fit = -1;

			for (i = 0; i < vm.num_modes; i++)
			{
				//fixme: check this formula. should be the full refresh rate
				r = vm.modes[i]->dotclock * 1000 / (vm.modes[i]->htotal * vm.modes[i]->vtotal);
				if (width > vm.modes[i]->hdisplay ||
					height > vm.modes[i]->vdisplay ||
					rate > r)
					continue;

				x = width - vm.modes[i]->hdisplay;
				y = height - vm.modes[i]->vdisplay;
				z = rate - r;
				dist = (x * x) + (y * y) + (z * z);
				if (dist < best_dist)
				{
					best_dist = dist;
					best_fit = i;
				}
			}

			if (best_fit != -1)
			{
				// change to the mode
				if (vm.pXF86VidModeSwitchToMode(vid_dpy, scrnum, vm.modes[vm.usemode=best_fit]))
				{
					width = vm.modes[best_fit]->hdisplay;
					height = vm.modes[best_fit]->vdisplay;
					// Move the viewport to top left
					vm.pXF86VidModeSetViewPort(vid_dpy, scrnum, 0, 0);
					x11.pXSync(vid_dpy, False);

					fullscreenflags |= FULLSCREEN_VMODE | FULLSCREEN_VMODEACTIVE;
				}
				else
					Con_Printf("Failed to apply mode %i*%i\n", vm.modes[best_fit]->hdisplay, vm.modes[best_fit]->vdisplay);
			}
		}
	}

	if (fullscreen)
	{
		if (!(fullscreenflags & FULLSCREEN_VMODE))
		{
			//if we can't actually change the mode, our fullscreen is the size of the root window
			XWindowAttributes xwa;
			x11.pXGetWindowAttributes(vid_dpy, DefaultRootWindow(vid_dpy), &xwa);
			width = xwa.width;
			height = xwa.height;
		}

		//window managers fuck up too much if we change the video mode and request the windowmanager make us fullscreen.
		if ((!(fullscreenflags & FULLSCREEN_VMODE) || vm.usemode <= 0) && X_CheckWMFullscreenAvailable())
			fullscreenflags |= FULLSCREEN_WM;
		else
			fullscreenflags |= FULLSCREEN_LEGACY;
	}
	else if (sys_parentwindow)
	{
		if (width < 64 || width > sys_parentwidth)
			width = sys_parentwidth;
		if (height < 64 || height > sys_parentheight)
			height = sys_parentheight;
	}

	switch(currentpsl)
	{
#ifdef USE_EGL
	case PSL_EGL:
		visinfo = &vinfodef;
		if (!x11.pXMatchVisualInfo(vid_dpy, scrnum, info->bpp, TrueColor, visinfo))
	//	if (!x11.pXMatchVisualInfo(vid_dpy, scrnum, DefaultDepth(vid_dpy, scrnum), TrueColor, &visinfo))
		{
			Sys_Error("Couldn't choose visual for EGL\n");
		}
		break;
#endif
	case PSL_GLX:
		visinfo = qglXChooseVisual(vid_dpy, scrnum, attrib);
		if (!visinfo)
		{
			Sys_Error("qkHack: Error couldn't get an RGB, Double-buffered, Depth visual\n");
		}
		break;
	default:
	case PSL_NONE:
		visinfo = NULL;
		break;	//erm
	}

	vid.activeapp = false;
	if (fullscreenflags & FULLSCREEN_LEGACY)
	{
		vid_decoywindow = X_CreateWindow(false, visinfo, 640, 480, false);
		vid_window = X_CreateWindow(true, visinfo, width, height, fullscreen);
	}
	else
		vid_window = X_CreateWindow(false, visinfo, width, height, fullscreen);

	vid_x_eventmask |= X_InitUnicode();
	x11.pXSelectInput(vid_dpy, vid_window, vid_x_eventmask);

	CL_UpdateWindowTitle();
	/*make it visible*/

	if (fullscreenflags & FULLSCREEN_VMODE)
	{
		x11.pXRaiseWindow(vid_dpy, vid_window);
		x11.pXWarpPointer(vid_dpy, None, vid_window, 0, 0, 0, 0, 0, 0);
		x11.pXFlush(vid_dpy);
		// Move the viewport to top left
		vm.pXF86VidModeSetViewPort(vid_dpy, scrnum, 0, 0);
	}

	vid_nullcursor = CreateNullCursor(vid_dpy, vid_window);

	x11.pXFlush(vid_dpy);

	if (vm.vmajor >= 2)
	{
		int rampsize = 256;
		vm.pXF86VidModeGetGammaRampSize(vid_dpy, scrnum, &rampsize);
		if (rampsize != 256)
		{
			vm.originalapplied = false;
			Con_Printf("Gamma ramps are not of 256 components (but %i).\n", rampsize);
		}
		else
			vm.originalapplied = vm.pXF86VidModeGetGammaRamp(vid_dpy, scrnum, 256, vm.originalramps[0], vm.originalramps[1], vm.originalramps[2]);
	}
	else
		vm.originalapplied = false;

	switch(currentpsl)
	{
	case PSL_GLX:
		ctx = qglXCreateContext(vid_dpy, visinfo, NULL, True);
		if (!ctx)
		{
			Con_Printf("Failed to create GLX context.\n");
			GLVID_Shutdown();
			return false;
		}

		if (!qglXMakeCurrent(vid_dpy, vid_window, ctx))
		{
			Con_Printf("glXMakeCurrent failed\n");
			GLVID_Shutdown();
			return false;
		}

		GL_Init(&GLX_GetSymbol);
		break;
#ifdef USE_EGL
	case PSL_EGL:
		if (!EGL_Init(info, palette, (EGLNativeWindowType)vid_window, (EGLNativeDisplayType)vid_dpy))
		{
			Con_Printf("Failed to create EGL context.\n");
			GLVID_Shutdown();
			return false;
		}
		GL_Init(&EGL_Proc);
		break;
#endif
	case PSL_NONE:
		break;
	}

	//probably going to be resized in the event handler
	vid.pixelwidth = fullscreenwidth = width;
	vid.pixelheight = fullscreenheight = height;

	vid.numpages = 2;

	Con_SafePrintf ("Video mode %dx%d initialized.\n", width, height);
	if (fullscreenflags & FULLSCREEN_WM)
		X_GoFullscreen();
	if (fullscreenflags & FULLSCREEN_LEGACY)
		x11.pXMoveResizeWindow(vid_dpy, vid_window, 0, 0, fullscreenwidth, fullscreenheight);
	if (fullscreenflags)
		fullscreenflags |= FULLSCREEN_ACTIVE;

	// TODO: make this into a cvar, like "in_dgamouse", instead of parameters
	if (!COM_CheckParm("-noxi2") && XI2_Init())
	{
		x11_input_method = XIM_XI2;
		Con_DPrintf("Using XInput2\n");
	}
	else if (!COM_CheckParm("-nodga") && !COM_CheckParm("-nomdga") && DGAM_Init())
	{
		x11_input_method = XIM_DGA;
		Con_DPrintf("Using DGA mouse\n");
	}
	else
	{
		x11_input_method = XIM_ORIG;
		Con_DPrintf("Using X11 mouse\n");
	}

	x11.pXRaiseWindow(vid_dpy, vid_window);
	if (fullscreenflags & FULLSCREEN_LEGACY)
		x11.pXMoveResizeWindow(vid_dpy, vid_window, 0, 0, fullscreenwidth, fullscreenheight);
	if (Cvar_Get("vidx_grabkeyboard", "0", 0, "Additional video options")->value)
		x11.pXGrabKeyboard(vid_dpy, vid_window,
				  False,
				  GrabModeAsync, GrabModeAsync,
				  CurrentTime);
	else if (fullscreenflags & FULLSCREEN_LEGACY)
		x11.pXSetInputFocus(vid_dpy, vid_window, RevertToNone, CurrentTime);

	return true;
}
qboolean GLVID_Init (rendererstate_t *info, unsigned char *palette)
{
	return X11VID_Init(info, palette, PSL_GLX);
}
#ifdef USE_EGL
qboolean EGLVID_Init (rendererstate_t *info, unsigned char *palette)
{
	return X11VID_Init(info, palette, PSL_EGL);
}
#endif

void Sys_SendKeyEvents(void)
{
#ifndef CLIENTONLY
	//this is stupid
	SV_GetConsoleCommands();
#endif
	if (sys_gracefulexit)
	{
		Cbuf_AddText("\nquit\n", RESTRICT_LOCAL);
		sys_gracefulexit = false;
	}
	if (vid_dpy && vid_window)
	{
		qboolean wantwindowed;

		while (x11.pXPending(vid_dpy))
			GetEvent();

		if (modeswitchpending && modeswitchtime < Sys_Milliseconds())
		{
			if (old_windowed_mouse)
			{
				Con_DPrintf("uninstall grabs\n");
				uninstall_grabs();
				x11.pXUndefineCursor(vid_dpy, vid_window);
				old_windowed_mouse = false;
			}
			if (modeswitchpending > 0 && !(fullscreenflags & FULLSCREEN_ACTIVE))
			{
				//entering fullscreen mode
				if (fullscreenflags & FULLSCREEN_VMODE)
				{
					if (!(fullscreenflags & FULLSCREEN_VMODEACTIVE))
					{
						// change to the mode
						vm.pXF86VidModeSwitchToMode(vid_dpy, scrnum, vm.modes[vm.usemode]);
						fullscreenflags |= FULLSCREEN_VMODEACTIVE;
						// Move the viewport to top left
					}
					vm.pXF86VidModeSetViewPort(vid_dpy, scrnum, 0, 0);
				}
				Cvar_ForceCallback(&v_gamma);

				/*release the mouse now, because we're paranoid about clip regions*/
				if (fullscreenflags & FULLSCREEN_WM)
					X_GoFullscreen();
				if (fullscreenflags & FULLSCREEN_LEGACY)
				{
					x11.pXMoveWindow(vid_dpy, vid_window, 0, 0);
					x11.pXReparentWindow(vid_dpy, vid_window, vid_root, 0, 0);
					//x11.pXUnmapWindow(vid_dpy, vid_decoywindow);
					//make sure we have it
					x11.pXSetInputFocus(vid_dpy, vid_window, RevertToParent, CurrentTime);
					x11.pXRaiseWindow(vid_dpy, vid_window);
					x11.pXMoveResizeWindow(vid_dpy, vid_window, 0, 0, fullscreenwidth, fullscreenheight);
				}
				if (fullscreenflags)
					fullscreenflags |= FULLSCREEN_ACTIVE;
			}
			if (modeswitchpending < 0)
			{
				//leave fullscreen mode
		 		if (!COM_CheckParm("-stayactive"))
 				{	//a parameter that leaves the program fullscreen if you taskswitch.
 					//sounds pointless, works great with two moniters. :D
					if (fullscreenflags & FULLSCREEN_VMODE)
					{
	 					if (vm.originalapplied)
							vm.pXF86VidModeSetGammaRamp(vid_dpy, scrnum, 256, vm.originalramps[0], vm.originalramps[1], vm.originalramps[2]);
						if (fullscreenflags & FULLSCREEN_VMODEACTIVE)
						{
							vm.pXF86VidModeSwitchToMode(vid_dpy, scrnum, vm.modes[0]);
							fullscreenflags &= ~FULLSCREEN_VMODEACTIVE;
						}
					}
				}
				if (fullscreenflags & FULLSCREEN_WM)
					X_GoWindowed();
				if (fullscreenflags & FULLSCREEN_LEGACY)
				{
					x11.pXMapWindow(vid_dpy, vid_decoywindow);
					x11.pXReparentWindow(vid_dpy, vid_window, vid_decoywindow, 0, 0);
					x11.pXResizeWindow(vid_dpy, vid_decoywindow, 640, 480);
				}
				fullscreenflags &= ~FULLSCREEN_ACTIVE;
			}
			modeswitchpending = 0;
		}

		if (modeswitchpending)
			return;

		wantwindowed = !!_windowed_mouse.value;
		if (!vid.activeapp)
			wantwindowed = false;
		if (Key_MouseShouldBeFree() && !fullscreenflags)
			wantwindowed = false;

		if (old_windowed_mouse != wantwindowed)
		{
			old_windowed_mouse = wantwindowed;

			if (!wantwindowed)
			{
				Con_DPrintf("uninstall grabs\n");
				/* ungrab the pointer */
				uninstall_grabs();
				x11.pXUndefineCursor(vid_dpy, vid_window);
			}
			else
			{
				Con_DPrintf("install grabs\n");
				/* grab the pointer */
				install_grabs();
				/*hide the cursor*/
				x11.pXDefineCursor(vid_dpy, vid_window, vid_nullcursor);
			}
		}
	}
}

void Force_CenterView_f (void)
{
	cl.playerview[0].viewangles[PITCH] = 0;
}


//these are done from the x11 event handler. we don't support evdev.
void INS_Move(float *movements, int pnum)
{
}
void INS_Commands(void)
{
}
void INS_Init(void)
{
}
void INS_ReInit(void)
{
}
void INS_Shutdown(void)
{
}
void INS_EnumerateDevices(void *ctx, void(*callback)(void *ctx, char *type, char *devicename, int *qdevid))
{
}

void GLVID_SetCaption(char *text)
{
	x11.pXStoreName(vid_dpy, vid_window, text);
}

#ifdef USE_EGL
#include "shader.h"
#include "gl_draw.h"
rendererinfo_t eglrendererinfo =
{
	"EGL(X11)",
	{
		"egl"
	},
	QR_OPENGL,

	GLDraw_Init,
	GLDraw_DeInit,

	GL_UpdateFiltering,
	GL_LoadTextureMips,
	GL_DestroyTexture,

	GLR_Init,
	GLR_DeInit,
	GLR_RenderView,

	EGLVID_Init,
	GLVID_DeInit,
	GLVID_SwapBuffers,
	GLVID_ApplyGammaRamps,

	NULL,
	NULL,
	NULL,
	GLVID_SetCaption,       //setcaption
	GLVID_GetRGBInfo,


	GLSCR_UpdateScreen,

	GLBE_SelectMode,
	GLBE_DrawMesh_List,
	GLBE_DrawMesh_Single,
	GLBE_SubmitBatch,
	GLBE_GetTempBatch,
	GLBE_DrawWorld,
	GLBE_Init,
	GLBE_GenBrushModelVBO,
	GLBE_ClearVBO,
	GLBE_UploadAllLightmaps,
	GLBE_SelectEntity,
	GLBE_SelectDLight,
	GLBE_Scissor,
	GLBE_LightCullModel,

	GLBE_VBO_Begin,
	GLBE_VBO_Data,
	GLBE_VBO_Finish,
	GLBE_VBO_Destroy,

	GLBE_RenderToTextureUpdate2d,

	""
};
#endif

#if 1
char *Sys_GetClipboard(void)
{
	Atom xa_clipboard = x11.pXInternAtom(vid_dpy, "PRIMARY", false);
	Atom xa_string = x11.pXInternAtom(vid_dpy, "UTF8_STRING", false);
	Window clipboardowner = x11.pXGetSelectionOwner(vid_dpy, xa_clipboard);
	if (clipboardowner != None && clipboardowner != vid_window)
	{
		int fmt;
		Atom type;
		unsigned long nitems, bytesleft;
		unsigned char *data;
		x11.pXConvertSelection(vid_dpy, xa_clipboard, xa_string, None, vid_window, CurrentTime);
		x11.pXFlush(vid_dpy);
		x11.pXGetWindowProperty(vid_dpy, vid_window, xa_string, 0, 0, False, AnyPropertyType, &type, &fmt, &nitems, &bytesleft, &data);
		
		return data;
	}
	return clipboard_buffer;
}

void Sys_CloseClipboard(char *bf)
{
	if (bf == clipboard_buffer)
		return;

	x11.pXFree(bf);
}

void Sys_SaveClipboard(char *text)
{
	Atom xa_clipboard = x11.pXInternAtom(vid_dpy, "PRIMARY", false);
	Q_strncpyz(clipboard_buffer, text, SYS_CLIPBOARD_SIZE);
	x11.pXSetSelectionOwner(vid_dpy, xa_clipboard, vid_window, CurrentTime);
}
#endif

qboolean X11_GetDesktopParameters(int *width, int *height, int *bpp, int *refreshrate)
{
	Display *xtemp;
	int scr;

	if (!x11_initlib())
		return false;

	xtemp = x11.pXOpenDisplay(NULL);

	if (!xtemp)
		return false;

	scr = DefaultScreen(xtemp);

	*width = DisplayWidth(xtemp, scr);
	*height = DisplayHeight(xtemp, scr);
	*bpp = DefaultDepth(xtemp, scr);
	*refreshrate = 0;

	x11.pXCloseDisplay(xtemp);

	return true;
}
