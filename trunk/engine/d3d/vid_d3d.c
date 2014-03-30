#include "quakedef.h"
#include "gl_draw.h"
#include "shader.h"
#include "renderque.h"

#include "glquake.h"

#ifdef D3D9QUAKE
#include "winquake.h"

#if !defined(HMONITOR_DECLARED) && (WINVER < 0x0500)
    #define HMONITOR_DECLARED
    DECLARE_HANDLE(HMONITOR);
#endif

#include    <d3d9.h>

//#pragma comment(lib, "../libs/dxsdk9/lib/d3d9.lib")


/*Fixup outdated windows headers*/
#ifndef WM_XBUTTONDOWN
   #define WM_XBUTTONDOWN      0x020B
   #define WM_XBUTTONUP      0x020C
#endif
#ifndef MK_XBUTTON1
   #define MK_XBUTTON1         0x0020
#endif
#ifndef MK_XBUTTON2
   #define MK_XBUTTON2         0x0040
#endif
// copied from DarkPlaces in an attempt to grab more buttons
#ifndef MK_XBUTTON3
   #define MK_XBUTTON3         0x0080
#endif
#ifndef MK_XBUTTON4
   #define MK_XBUTTON4         0x0100
#endif
#ifndef MK_XBUTTON5
   #define MK_XBUTTON5         0x0200
#endif
#ifndef MK_XBUTTON6
   #define MK_XBUTTON6         0x0400
#endif
#ifndef MK_XBUTTON7
   #define MK_XBUTTON7         0x0800
#endif

#ifndef WM_INPUT
	#define WM_INPUT 255
#endif

//static void D3D9_GetBufferSize(int *width, int *height); //not defined
static void resetD3D9(void);
static LPDIRECT3D9 pD3D;
LPDIRECT3DDEVICE9 pD3DDev9;
static D3DPRESENT_PARAMETERS d3dpp;
float d3d_trueprojection[16];

qboolean vid_initializing;

extern qboolean		scr_initialized;                // ready to draw
extern qboolean		scr_drawloading;
extern qboolean		scr_con_forcedraw;
static qboolean d3d_resized;

cvar_t vid_hardwaregamma;


//sound/error code needs this
HWND mainwindow;

//input code needs these
int		window_center_x, window_center_y;
RECT		window_rect;
int window_x, window_y;


/*void BuildGammaTable (float g, float c);
static void	D3D9_VID_GenPaletteTables (unsigned char *palette)
{
	extern unsigned short		ramps[3][256];
	qbyte	*pal;
	unsigned r,g,b;
	unsigned v;
	unsigned short i;
	unsigned	*table;
	extern qbyte gammatable[256];

	if (palette)
	{
		extern cvar_t v_contrast;
		BuildGammaTable(v_gamma.value, v_contrast.value);

		//
		// 8 8 8 encoding
		//
		if (1)//vid_hardwaregamma.value)
		{
		//	don't built in the gamma table

			pal = palette;
			table = d_8to24rgbtable;
			for (i=0 ; i<256 ; i++)
			{
				r = pal[0];
				g = pal[1];
				b = pal[2];
				pal += 3;

		//		v = (255<<24) + (r<<16) + (g<<8) + (b<<0);
		//		v = (255<<0) + (r<<8) + (g<<16) + (b<<24);
				v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
				*table++ = v;
			}
			d_8to24rgbtable[255] &= 0xffffff;	// 255 is transparent
		}
		else
		{
	//computer has no hardware gamma (poor suckers) increase table accordingly

			pal = palette;
			table = d_8to24rgbtable;
			for (i=0 ; i<256 ; i++)
			{
				r = gammatable[pal[0]];
				g = gammatable[pal[1]];
				b = gammatable[pal[2]];
				pal += 3;

		//		v = (255<<24) + (r<<16) + (g<<8) + (b<<0);
		//		v = (255<<0) + (r<<8) + (g<<16) + (b<<24);
				v = (255<<24) + (r<<0) + (g<<8) + (b<<16);
				*table++ = v;
			}
			d_8to24rgbtable[255] &= 0xffffff;	// 255 is transparent
		}

		if (LittleLong(1) != 1)
		{
			for (i=0 ; i<256 ; i++)
				d_8to24rgbtable[i] = LittleLong(d_8to24rgbtable[i]);
		}
	}

	if (pD3DDev9)
		IDirect3DDevice9_SetGammaRamp(pD3DDev9, 0, D3DSGR_NO_CALIBRATION, (D3DGAMMARAMP *)ramps);
}
*/
typedef enum {MS_WINDOWED, MS_FULLSCREEN, MS_FULLDIB, MS_UNINIT} modestate_t;
static modestate_t modestate;


static void D3DVID_UpdateWindowStatus (HWND hWnd)
{
	POINT p;
	RECT nr;
	int window_width, window_height;
	GetClientRect(hWnd, &nr);

	//if its bad then we're probably minimised
	if (nr.right <= nr.left)
		return;
	if (nr.bottom <= nr.top)
		return;

	p.x = 0;
	p.y = 0;
	ClientToScreen(hWnd, &p);
	window_x = p.x;
	window_y = p.y;
	window_width = nr.right - nr.left;
	window_height = nr.bottom - nr.top;
//	vid.pixelwidth = window_width;
//	vid.pixelheight = window_height;

	window_rect.left = window_x;
	window_rect.top = window_y;
	window_rect.right = window_x + window_width;
	window_rect.bottom = window_y + window_height;
	window_center_x = (window_rect.left + window_rect.right) / 2;
	window_center_y = (window_rect.top + window_rect.bottom) / 2;

	INS_UpdateClipCursor ();
}

static qboolean D3D9AppActivate(BOOL fActive, BOOL minimize)
/****************************************************************************
*
* Function:     AppActivate
* Parameters:   fActive - True if app is activating
*
* Description:  If the application is activating, then swap the system
*               into SYSPAL_NOSTATIC mode so that our palettes will display
*               correctly.
*
****************************************************************************/
{
	static BOOL	sound_active;

	if (ActiveApp == fActive && Minimized == minimize)
		return false;	//so windows doesn't crash us over and over again.

	ActiveApp = fActive;
	Minimized = minimize;

// enable/disable sound on focus gain/loss
	if (!ActiveApp && sound_active)
	{
		S_BlockSound ();
		sound_active = false;
	}
	else if (ActiveApp && !sound_active)
	{
		S_UnblockSound ();
		sound_active = true;
	}

	INS_UpdateGrabs(modestate != MS_WINDOWED, ActiveApp);

	if (fActive)
	{
		Cvar_ForceCallback(&v_gamma);
	}
	if (!fActive)
	{
		Cvar_ForceCallback(&v_gamma);	//wham bam thanks.
	}

	return true;
}





static LRESULT WINAPI D3D9_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LONG    lRet = 1;
	int		fActive, fMinimized, temp;
	extern unsigned int uiWheelMessage;

	if ( uMsg == uiWheelMessage )
		uMsg = WM_MOUSEWHEEL;

	switch (uMsg)
	{
		case WM_KILLFOCUS:
			if (modestate == MS_FULLDIB)
				ShowWindow(mainwindow, SW_SHOWMINNOACTIVE);
			break;

		case WM_CREATE:
			break;

		case WM_MOVE:
			D3DVID_UpdateWindowStatus (hWnd);
			break;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if (!vid_initializing)
				INS_TranslateKeyEvent (wParam, lParam, true, 0);
			break;

		case WM_KEYUP:
		case WM_SYSKEYUP:
			if (!vid_initializing)
				INS_TranslateKeyEvent (wParam, lParam, false, 0);
			break;

		case WM_SYSCHAR:
		// keep Alt-Space from happening
			break;

	// this is complicated because Win32 seems to pack multiple mouse events into
	// one update sometimes, so we always check all states and look for events
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_MOUSEMOVE:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
			temp = 0;

			if (wParam & MK_LBUTTON)
				temp |= 1;

			if (wParam & MK_RBUTTON)
				temp |= 2;

			if (wParam & MK_MBUTTON)
				temp |= 4;

			if (wParam & MK_XBUTTON1)
				temp |= 8;

			if (wParam & MK_XBUTTON2)
				temp |= 16;

			if (wParam & MK_XBUTTON3)
				temp |= 32;

			if (wParam & MK_XBUTTON4)
				temp |= 64;

			if (wParam & MK_XBUTTON5)
				temp |= 128;

			if (wParam & MK_XBUTTON6)
				temp |= 256;

			if (wParam & MK_XBUTTON7)
				temp |= 512;

			if (!vid_initializing)
				INS_MouseEvent (temp);

			break;

		// JACK: This is the mouse wheel with the Intellimouse
		// Its delta is either positive or neg, and we generate the proper
		// Event.
		case WM_MOUSEWHEEL:
			if (!vid_initializing)
			{
				if ((short) HIWORD(wParam) > 0)
				{
					Key_Event(0, K_MWHEELUP, 0, true);
					Key_Event(0, K_MWHEELUP, 0, false);
				}
				else
				{
					Key_Event(0, K_MWHEELDOWN, 0, true);
					Key_Event(0, K_MWHEELDOWN, 0, false);
				}
			}
			break;

		case WM_INPUT:
			// raw input handling
			if (!vid_initializing)
				INS_RawInput_Read((HANDLE)lParam);
			break;

		case WM_GETMINMAXINFO:
			{
				RECT windowrect;
				RECT clientrect;
				MINMAXINFO *mmi = (MINMAXINFO *) lParam;

				GetWindowRect (hWnd, &windowrect);
				GetClientRect (hWnd, &clientrect);

				mmi->ptMinTrackSize.x = 320 + ((windowrect.right - windowrect.left) - (clientrect.right - clientrect.left));
				mmi->ptMinTrackSize.y = 200 + ((windowrect.bottom - windowrect.top) - (clientrect.bottom - clientrect.top));
			}
			return 0;
    	case WM_SIZE:
			d3d_resized = true;
			break;

		case WM_CLOSE:
			if (!vid_initializing)
				if (MessageBox (mainwindow, "Are you sure you want to quit?", "Confirm Exit",
							MB_YESNO | MB_SETFOREGROUND | MB_ICONQUESTION) == IDYES)
				{
					Cbuf_AddText("\nquit\n", RESTRICT_LOCAL);
				}

	        break;

		case WM_ACTIVATE:
			fActive = LOWORD(wParam);
			fMinimized = (BOOL) HIWORD(wParam);
			if (!D3D9AppActivate(!(fActive == WA_INACTIVE), fMinimized))
				break;//so, urm, tell me microsoft, what changed?
			if (modestate == MS_FULLDIB)
				ShowWindow(mainwindow, SW_SHOWNORMAL);

		// fix the leftover Alt from any Alt-Tab or the like that switched us away
//			ClearAllStates ();

			break;

   	    case WM_DESTROY:
        {
//			if (dibwindow)
//				DestroyWindow (dibwindow);
        }
        break;

		case MM_MCINOTIFY:
            lRet = CDAudio_MessageHandler (hWnd, uMsg, wParam, lParam);
			break;

    	default:
            /* pass all unhandled messages to DefWindowProc */
            lRet = DefWindowProc (hWnd, uMsg, wParam, lParam);
        break;
    }

    /* return 1 if handled message, 0 if not */
    return lRet;
}

static void D3D9_VID_SwapBuffers(void)
{
	IDirect3DDevice9_Present(pD3DDev9, NULL, NULL, NULL, NULL);
}

static void resetD3D9(void)
{
	HRESULT res;
	res = IDirect3DDevice9_Reset(pD3DDev9, &d3dpp);
	if (FAILED(res))
	{
		Con_Printf("IDirect3DDevice9_Reset failed (%u)\n", res&0xffff);
		return;
	}


	/*clear the screen to black as soon as we start up, so there's no lingering framebuffer state*/
	IDirect3DDevice9_BeginScene(pD3DDev9);
	IDirect3DDevice9_Clear(pD3DDev9, 0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	IDirect3DDevice9_EndScene(pD3DDev9);
	IDirect3DDevice9_Present(pD3DDev9, NULL, NULL, NULL, NULL);







	//IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRENDERSTATE_DITHERENABLE, FALSE);
	//IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRENDERSTATE_SPECULARENABLE, FALSE);
	//IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE);
	IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_LIGHTING, FALSE);
}

#if (WINVER < 0x500) && !defined(__GNUC__)
typedef struct tagMONITORINFO
{
    DWORD   cbSize;
    RECT    rcMonitor;
    RECT    rcWork;
    DWORD   dwFlags;
} MONITORINFO, *LPMONITORINFO;
#endif

static qboolean initD3D9Device(HWND hWnd, rendererstate_t *info, unsigned int devno, unsigned int devtype)
{
	int err;
	RECT rect;
	D3DADAPTER_IDENTIFIER9 inf;
	D3DCAPS9 caps;
	unsigned int cflags;

	memset(&inf, 0, sizeof(inf));
	if (FAILED(IDirect3D9_GetAdapterIdentifier(pD3D, devno, 0, &inf)))
		return false;

	if (FAILED(IDirect3D9_GetDeviceCaps(pD3D, devno, devtype, &caps)))
		return false;

	memset(&d3dpp, 0, sizeof(d3dpp));    // clear out the struct for use
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;    // discard old frames
	d3dpp.hDeviceWindow = hWnd;    // set the window to be used by Direct3D
	d3dpp.BackBufferWidth = info->width;
	d3dpp.BackBufferHeight = info->height;
	d3dpp.MultiSampleType = info->multisample;
	d3dpp.BackBufferCount = 1 + info->triplebuffer;
	d3dpp.FullScreen_RefreshRateInHz = info->fullscreen?info->rate:0;	//don't pass a rate if not fullscreen, d3d doesn't like it.
	d3dpp.Windowed = !info->fullscreen;

	d3dpp.EnableAutoDepthStencil = true;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;//D3DFMT_D16;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	if (info->fullscreen)
	{
		if (info->bpp == 16)
			d3dpp.BackBufferFormat = D3DFMT_R5G6B5;
		else
			d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	}

	switch(info->wait)
	{
	default:
		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
		break;
	case 0:
		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
		break;
	case 1:
		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
		break;
	case 2:
		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_TWO;
		break;
	case 3:
		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_THREE;
		break;
	case 4:
		d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_FOUR;
		break;
	}

	cflags = D3DCREATE_FPU_PRESERVE;
	if ((caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) && (caps.DevCaps & D3DDEVCAPS_PUREDEVICE))
		cflags |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		cflags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	//cflags |= D3DCREATE_DISABLE_DRIVER_MANAGEMENT;

	pD3DDev9 = NULL;
	// create a device class using this information and information from the d3dpp stuct
	err = IDirect3D9_CreateDevice(pD3D,
			devno,
			devtype,
			hWnd,
			cflags,
			&d3dpp,
			&pD3DDev9);

	if (pD3DDev9)
	{
		HMONITOR hm;
		MONITORINFO mi;
		char *s;
		for (s = inf.Description + strlen(inf.Description)-1; s >= inf.Description && *s <= ' '; s--)
			*s = 0;
		Con_Printf("D3D9 Driver: %s\n", inf.Description);

		vid.numpages = d3dpp.BackBufferCount;

		if (d3dpp.Windowed)	//fullscreen we get positioned automagically.
		{					//windowed, we get positioned at 0,0... which is often going to be on the wrong screen
							//the user can figure it out from here
			static HANDLE huser32;
			BOOL (WINAPI *pGetMonitorInfoA)(HMONITOR hMonitor, LPMONITORINFO lpmi);
			if (!huser32)
				huser32 = LoadLibrary("user32.dll");
			if (!huser32)
				return false;
			pGetMonitorInfoA = (void*)GetProcAddress(huser32, "GetMonitorInfoA");
			if (!pGetMonitorInfoA)
				return false;

			hm = IDirect3D9_GetAdapterMonitor(pD3D, devno);
			memset(&mi, 0, sizeof(mi));
			mi.cbSize = sizeof(mi);
			pGetMonitorInfoA(hm, &mi);
			rect.left = mi.rcWork.left + ((mi.rcWork.right - mi.rcWork.left) - info->width) / 2;
			rect.top = mi.rcWork.top + ((mi.rcWork.bottom - mi.rcWork.top) - info->height) / 2;
			rect.right = rect.left+d3dpp.BackBufferWidth;
			rect.bottom = rect.top+d3dpp.BackBufferHeight;
			AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
			MoveWindow(d3dpp.hDeviceWindow, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, false);

			D3D9Shader_Init();
		}
		return true;	//successful
	}
	else
	{
		char *s;
		switch(err)
		{
		default: s = "Unkown error"; break;
		case D3DERR_DEVICELOST: s = "Device lost"; break;
		case D3DERR_INVALIDCALL: s = "Invalid call"; break;
		case D3DERR_NOTAVAILABLE: s = "Not available"; break;
		case D3DERR_OUTOFVIDEOMEMORY: s = "Out of video memory"; break;
		}
		Con_Printf("IDirect3D9_CreateDevice failed: %s.\n", s);
	}
	return false;
}

static void initD3D9(HWND hWnd, rendererstate_t *info)
{
	int i;
	int numadaptors;
	int err;
	D3DADAPTER_IDENTIFIER9 inf;

	static HMODULE d3d9dll;
	LPDIRECT3D9 (WINAPI *pDirect3DCreate9) (int version);

	if (!d3d9dll)
		d3d9dll = LoadLibrary("d3d9.dll");
	if (!d3d9dll)
	{
		Con_Printf("Direct3d 9 does not appear to be installed\n");
		return;
	}
	pDirect3DCreate9 = (void*)GetProcAddress(d3d9dll, "Direct3DCreate9");
	if (!pDirect3DCreate9)
	{
		Con_Printf("Direct3d 9 does not appear to be installed properly\n");
		return;
	}

	pD3D = pDirect3DCreate9(D3D_SDK_VERSION);    // create the Direct3D interface
	if (!pD3D)
		return;

	numadaptors = IDirect3D9_GetAdapterCount(pD3D);
	for (i = 0; i < numadaptors; i++)
	{	//NVIDIA's debug app requires that we use a specific device
		memset(&inf, 0, sizeof(inf));
		err = IDirect3D9_GetAdapterIdentifier(pD3D, i, 0, &inf);
		if (strstr(inf.Description, "PerfHUD"))
			if (initD3D9Device(hWnd, info, i, D3DDEVTYPE_REF))
				return;
	}
	for (i = 0; i < numadaptors; i++)
	{	//try each adaptor in turn until we get one that actually works
		if (initD3D9Device(hWnd, info, i, D3DDEVTYPE_HAL))
			return;
	}
	for (i = 0; i < numadaptors; i++)
	{	//try each adaptor in turn until we get one that actually works
		if (initD3D9Device(hWnd, info, i, D3DDEVTYPE_SW))
			return;
	}
	for (i = 0; i < numadaptors; i++)
	{	//try each adaptor in turn until we get one that actually works
		if (initD3D9Device(hWnd, info, i, D3DDEVTYPE_REF))
			return;
	}
}

static qboolean D3D9_VID_Init(rendererstate_t *info, unsigned char *palette)
{
	DWORD width = info->width;
	DWORD height = info->height;
	//DWORD bpp = info->bpp;
	//DWORD zbpp = 16;
	//DWORD flags = 0;
	DWORD wstyle;
	RECT rect;
	MSG msg;

	extern cvar_t vid_conwidth;
	//extern cvar_t vid_conheight;

	//DDGAMMARAMP gammaramp;
	//int i;

	char *CLASSNAME = "FTED3D9QUAKE";
	WNDCLASS wc = {
		0,
		&D3D9_WindowProc,
		0,
		0,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		CLASSNAME
	};

	wc.hCursor       = LoadCursor (NULL,IDC_ARROW);

	vid_initializing = true;

	RegisterClass(&wc);

	if (info->fullscreen)
		wstyle = 0;
	else
		wstyle = WS_OVERLAPPEDWINDOW;

	rect.left = (GetSystemMetrics(SM_CXSCREEN) - info->width) / 2;
	rect.top = (GetSystemMetrics(SM_CYSCREEN) - info->height) / 2;
	rect.right = rect.left+info->width;
	rect.bottom = rect.top+info->height;
	AdjustWindowRectEx(&rect, wstyle, FALSE, 0);
	mainwindow = CreateWindow(CLASSNAME, "Direct3D9", wstyle, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, NULL, NULL, NULL, NULL);

	// Try as specified.

	initD3D9(mainwindow, info);
	if (!pD3DDev9)
	{
		Con_Printf("No suitable D3D9 device found\n");
		return false;
	}



	while (PeekMessage(&msg, NULL,  0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	ShowWindow(mainwindow, SW_NORMAL);

	IDirect3DDevice9_Clear(pD3DDev9, 0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	IDirect3DDevice9_BeginScene(pD3DDev9);
	IDirect3DDevice9_EndScene(pD3DDev9);
	IDirect3DDevice9_Present(pD3DDev9, NULL, NULL, NULL, NULL);



//	pD3DX->lpVtbl->GetBufferSize((void*)pD3DX, &width, &height);
	vid.pixelwidth = width;
	vid.pixelheight = height;

	vid.width = width;
	vid.height = height;

	vid_initializing = false;

	IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_LIGHTING, FALSE);

	GetWindowRect(mainwindow, &window_rect);


//	D3D9_VID_GenPaletteTables(palette);

	{
		extern qboolean	mouseactive;
		mouseactive = false;
	}

	return true;
}

/*a new model has been loaded*/
static void	(D3D9_R_NewMap)					(void)
{
	r_worldentity.model = cl.worldmodel;

#ifdef MAP_PROC
	if (cl.worldmodel && cl.worldmodel->fromgame == fg_doom3)
		D3_GenerateAreas(cl.worldmodel);
#endif

	/*wipe any lingering particles*/
	P_ClearParticles();
	CL_RegisterParticles();

	R_AnimateLight();
	Surf_DeInit();
	Surf_WipeStains();
	Surf_BuildLightmaps();

	TP_NewMap();
	R_SetSky(cl.skyname);

#ifdef RTLIGHTS
	Sh_PreGenerateLights();
#endif
}

extern mleaf_t		*r_viewleaf, *r_oldviewleaf;
extern mleaf_t		*r_viewleaf2, *r_oldviewleaf2;
static void	(D3D9_R_PreNewMap)				(void)
{
	r_viewleaf = NULL;
	r_oldviewleaf = NULL;
	r_viewleaf2 = NULL;
	r_oldviewleaf2 = NULL;
}

static void	 (D3D9_VID_DeInit)				(void)
{
	/*final shutdown, kill the video stuff*/
	if (pD3DDev9)
	{
		/*try and knock it back into windowed mode to avoid d3d bugs*/
		d3dpp.Windowed = true;
		IDirect3DDevice9_Reset(pD3DDev9, &d3dpp);

		IDirect3DDevice9_Release(pD3DDev9);
		pD3DDev9 = NULL;
	}
	if (pD3D)
	{
		IDirect3D9_Release(pD3D);
		pD3D = NULL;
	}
	if (mainwindow)
	{
		DestroyWindow(mainwindow);
		mainwindow = NULL;
	}

	Cvar_Unhook(&v_gamma);
	Cvar_Unhook(&v_contrast);
	Cvar_Unhook(&v_brightness);
}

qboolean D3D9_VID_ApplyGammaRamps		(unsigned short *ramps)
{
	if (d3dpp.Windowed)
		return false;
	if (pD3DDev9 && ramps)
		IDirect3DDevice9_SetGammaRamp(pD3DDev9, 0, D3DSGR_NO_CALIBRATION, (D3DGAMMARAMP *)ramps);
	return true;
}
static char	*(D3D9_VID_GetRGBInfo)			(int prepad, int *truevidwidth, int *truevidheight)
{
	IDirect3DSurface9 *backbuf, *surf;
	D3DLOCKED_RECT rect;
	D3DSURFACE_DESC desc;
	int i, j, c;
	qbyte *ret = NULL;
	qbyte *p;

	/*DON'T read the front buffer.
	this function can be used by the quakeworld remote screenshot 'snap' feature,
	so DO NOT read the frontbuffer because it can show other information than just quake to third parties*/

	if (!FAILED(IDirect3DDevice9_GetRenderTarget(pD3DDev9, 0, &backbuf)))
	{
		if (!FAILED(IDirect3DSurface9_GetDesc(backbuf, &desc)))
		if (desc.Format == D3DFMT_X8R8G8B8 || desc.Format == D3DFMT_A8R8G8B8)
		if (!FAILED(IDirect3DDevice9_CreateOffscreenPlainSurface(pD3DDev9,
					desc.Width, desc.Height, desc.Format,
					D3DPOOL_SYSTEMMEM, &surf, NULL))
			)
		{

			if (!FAILED(IDirect3DDevice9_GetRenderTargetData(pD3DDev9, backbuf, surf)))
			if (!FAILED(IDirect3DSurface9_LockRect(surf, &rect, NULL, D3DLOCK_NO_DIRTY_UPDATE|D3DLOCK_READONLY|D3DLOCK_NOSYSLOCK)))
			{
				ret = BZ_Malloc(prepad + desc.Width*desc.Height*3);
				if (ret)
				{
					// read surface rect and convert 32 bgra to 24 rgb and flip
					c = prepad+desc.Width*desc.Height*3;
					p = (qbyte *)rect.pBits;

					for (i=c-(3*desc.Width); i>=prepad; i-=(3*desc.Width))
					{
						for (j=0; j<desc.Width; j++)
						{
							ret[i+j*3+0] = p[j*4+2];
							ret[i+j*3+1] = p[j*4+1];
							ret[i+j*3+2] = p[j*4+0];
						}
						p += rect.Pitch;
					}

					*truevidwidth = desc.Width;
					*truevidheight = desc.Height;
				}

				IDirect3DSurface9_UnlockRect(surf);
			}
			IDirect3DSurface9_Release(surf);
		}
		IDirect3DSurface9_Release(backbuf);
	}

	return ret;
}
static void	(D3D9_VID_SetWindowCaption)		(char *msg)
{
	SetWindowText(mainwindow, msg);
}

void D3D9_Set2D (void)
{
	float m[16];
	D3DVIEWPORT9 vport;
//	IDirect3DDevice9_EndScene(pD3DDev9);

	Matrix4x4_CM_OrthographicD3D(m, 0 + (0.5*vid.width/vid.pixelwidth), vid.width + (0.5*vid.width/vid.pixelwidth), 0 + (0.5*vid.height/vid.pixelheight), vid.height + (0.5*vid.height/vid.pixelheight), 0, 100);
	IDirect3DDevice9_SetTransform(pD3DDev9, D3DTS_PROJECTION, (D3DMATRIX*)m);

	Matrix4x4_Identity(m);
	IDirect3DDevice9_SetTransform(pD3DDev9, D3DTS_WORLD, (D3DMATRIX*)m);

	Matrix4x4_Identity(m);
	IDirect3DDevice9_SetTransform(pD3DDev9, D3DTS_VIEW, (D3DMATRIX*)m);

	vport.X = 0;
	vport.Y = 0;
	vport.Width = vid.pixelwidth;
	vport.Height = vid.pixelheight;
	vport.MinZ = 0;
	vport.MaxZ = 1;
	IDirect3DDevice9_SetViewport(pD3DDev9, &vport);
}

static int d3d9error(int i)
{
	if (FAILED(i))// != D3D_OK)
		Con_Printf("D3D error: %i\n", i);
	return i;
}

static void	(D3D9_SCR_UpdateScreen)			(void)
{
	//extern int keydown[];
	//extern cvar_t vid_conheight;
	int uimenu;
#ifdef TEXTEDITOR
	//extern qboolean editormodal;
#endif
	qboolean nohud, noworld;
	RSpeedMark();

	if (d3d_resized && d3dpp.Windowed)
	{
		extern cvar_t vid_conautoscale, vid_conwidth;
		d3d_resized = false;

		// force width/height to be updated
		//vid.pixelwidth = window_rect.right - window_rect.left;
		//vid.pixelheight = window_rect.bottom - window_rect.top;
		D3DVID_UpdateWindowStatus(mainwindow);

		D3D9BE_Reset(true);
		vid.pixelwidth = d3dpp.BackBufferWidth = window_rect.right - window_rect.left;
		vid.pixelheight = d3dpp.BackBufferHeight = window_rect.bottom - window_rect.top;
		resetD3D9();
		D3D9BE_Reset(false);

		Cvar_ForceCallback(&vid_conautoscale);
		Cvar_ForceCallback(&vid_conwidth);
	}

	switch (IDirect3DDevice9_TestCooperativeLevel(pD3DDev9))
	{
	case D3DERR_DEVICELOST:
		//the user has task switched away from us or something, don't draw anything until they switch back to us
		return;
	case D3DERR_DEVICENOTRESET:
		D3D9BE_Reset(true);
		resetD3D9();
		if (FAILED(IDirect3DDevice9_TestCooperativeLevel(pD3DDev9)))
		{
			Con_Printf("Device lost, restarting video\n");
			Cmd_ExecuteString("vid_restart", RESTRICT_LOCAL);
			return;
		}
		D3D9BE_Reset(false);

		Cvar_ForceCallback(&v_gamma);
		break;
	default:
		break;
	}

	if (scr_disabled_for_loading)
	{
		extern float scr_disabled_time;
		if (Sys_DoubleTime() - scr_disabled_time > 60 || Key_Dest_Has(~kdm_game))
		{
			scr_disabled_for_loading = false;
		}
		else
		{
			IDirect3DDevice9_BeginScene(pD3DDev9);
			scr_drawloading = true;
			SCR_DrawLoading (true);
			scr_drawloading = false;
			IDirect3DDevice9_EndScene(pD3DDev9);
			IDirect3DDevice9_Present(pD3DDev9, NULL, NULL, NULL, NULL);
			RSpeedEnd(RSPEED_TOTALREFRESH);
			return;
		}
	}

	if (!scr_initialized || !con_initialized)
	{
		RSpeedEnd(RSPEED_TOTALREFRESH);
		return;                         // not initialized yet
	}

	Shader_DoReload();

#ifdef VM_UI
	uimenu = UI_MenuState();
#else
	uimenu = 0;
#endif

	d3d9error(IDirect3DDevice9_BeginScene(pD3DDev9));
/*
#ifdef TEXTEDITOR
	if (editormodal)
	{
		Editor_Draw();
		V_UpdatePalette (false);
#if defined(_WIN32) && defined(GLQUAKE)
		Media_RecordFrame();
#endif
		R2D_BrightenScreen();

		if (key_dest == key_console)
			Con_DrawConsole(vid_conheight.value/2, false);
		GL_EndRendering ();
		GL_DoSwap();
		RSpeedEnd(RSPEED_TOTALREFRESH);
		return;
	}
#endif
*/
	if (Media_ShowFilm())
	{
		M_Draw(0);
//		V_UpdatePalette (false);
#if defined(_WIN32)
		Media_RecordFrame();
#endif
//		R2D_BrightenScreen();
		IDirect3DDevice9_EndScene(pD3DDev9);
		IDirect3DDevice9_Present(pD3DDev9, NULL, NULL, NULL, NULL);
		RSpeedEnd(RSPEED_TOTALREFRESH);
		return;
	}

//
// do 3D refresh drawing, and then update the screen
//
	SCR_SetUpToDrawConsole ();

	noworld = false;
	nohud = false;

#ifdef VM_CG
	if (CG_Refresh())
		nohud = true;
	else
#endif
#ifdef CSQC_DAT
		if (CSQC_DrawView())
		nohud = true;
	else
#endif
		if (uimenu != 1)
		{
			if (r_worldentity.model && cls.state == ca_active)
 				V_RenderView ();
			else
			{
				noworld = true;
			}
		}

	D3D9_Set2D();

	R2D_BrightenScreen();

	scr_con_forcedraw = false;
	if (noworld)
	{
		if (scr_con_current != vid.height)
			R2D_ConsoleBackground(0, vid.height, true);
		else
			scr_con_forcedraw = true;

		nohud = true;
	}
	else if (!nohud)
		SCR_TileClear ();

	SCR_DrawTwoDimensional(uimenu, nohud);

	V_UpdatePalette (false);
#if defined(_WIN32) && defined(GLQUAKE)
	Media_RecordFrame();
#endif

	RSpeedEnd(RSPEED_TOTALREFRESH);
	RSpeedShow();


	d3d9error(IDirect3DDevice9_EndScene(pD3DDev9));
	d3d9error(IDirect3DDevice9_Present(pD3DDev9, NULL, NULL, NULL, NULL));

	window_center_x = (window_rect.left + window_rect.right)/2;
	window_center_y = (window_rect.top + window_rect.bottom)/2;


	INS_UpdateGrabs(modestate != MS_WINDOWED, ActiveApp);
}







static void	(D3D9_Draw_Init)				(void)
{
	R2D_Init();
}
static void	(D3D9_Draw_Shutdown)				(void)
{
	R2D_Shutdown();
}

static void	(D3D9_R_Init)					(void)
{
}
static void	(D3D9_R_DeInit)					(void)
{
	Surf_DeInit();
	Shader_Shutdown();
	D3D9_Image_Shutdown();
}



static void D3D9_SetupViewPortProjection(void)
{
	extern cvar_t gl_mindist;
	float	screenaspect;
	int		x, x2, y2, y, w, h;

	float fov_x, fov_y;

	D3DVIEWPORT9 vport;

	AngleVectors (r_refdef.viewangles, vpn, vright, vup);
	VectorCopy (r_refdef.vieworg, r_origin);

	//
	// set up viewpoint
	//
	x = r_refdef.vrect.x * vid.pixelwidth/(int)vid.width;
	x2 = (r_refdef.vrect.x + r_refdef.vrect.width) * vid.pixelwidth/(int)vid.width;
	y = (r_refdef.vrect.y) * vid.pixelheight/(int)vid.height;
	y2 = ((int)(r_refdef.vrect.y + r_refdef.vrect.height)) * vid.pixelheight/(int)vid.height;

	// fudge around because of frac screen scale
	if (x > 0)
		x--;
	if (x2 < vid.pixelwidth)
		x2++;
	if (y < 0)
		y--;
	if (y2 < vid.pixelheight)
		y2++;

	w = x2 - x;
	h = y2 - y;

	vport.X = x;
	vport.Y = y;
	vport.Width = w;
	vport.Height = h;
	vport.MinZ = 0;
	vport.MaxZ = 1;
	IDirect3DDevice9_SetViewport(pD3DDev9, &vport);

	fov_x = r_refdef.fov_x;//+sin(cl.time)*5;
	fov_y = r_refdef.fov_y;//-sin(cl.time+1)*5;

	if (r_waterwarp.value<0 && r_viewcontents & FTECONTENTS_FLUID)
	{
		fov_x *= 1 + (((sin(cl.time * 4.7) + 1) * 0.015) * r_waterwarp.value);
		fov_y *= 1 + (((sin(cl.time * 3.0) + 1) * 0.015) * r_waterwarp.value);
	}

	screenaspect = (float)r_refdef.vrect.width/r_refdef.vrect.height;

	/*view matrix*/
	Matrix4x4_CM_ModelViewMatrixFromAxis(r_refdef.m_view, vpn, vright, vup, r_refdef.vieworg);
	d3d9error(IDirect3DDevice9_SetTransform(pD3DDev9, D3DTS_VIEW, (D3DMATRIX*)r_refdef.m_view));

	/*d3d projection matricies scale depth to 0 to 1*/
	Matrix4x4_CM_Projection_Inf(d3d_trueprojection, fov_x, fov_y, gl_mindist.value/2);
	d3d9error(IDirect3DDevice9_SetTransform(pD3DDev9, D3DTS_PROJECTION, (D3DMATRIX*)d3d_trueprojection));
	/*ogl projection matricies scale depth to -1 to 1, and I would rather my code used consistant culling*/
	Matrix4x4_CM_Projection_Inf(r_refdef.m_projection, fov_x, fov_y, gl_mindist.value);
}

static void	(D3D9_R_RenderView)				(void)
{
	Surf_SetupFrame();

	D3D9_SetupViewPortProjection();

	if (r_clear.ival && !(r_refdef.flags & Q2RDF_NOWORLDMODEL))
		d3d9error(IDirect3DDevice9_Clear(pD3DDev9, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(255,0,0), 1, 0));
	else
		d3d9error(IDirect3DDevice9_Clear(pD3DDev9, 0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 1, 0));

	R_SetFrustum (r_refdef.m_projection, r_refdef.m_view);
	RQ_BeginFrame();
	if (!(r_refdef.flags & Q2RDF_NOWORLDMODEL))
	{
		if (cl.worldmodel)
			P_DrawParticles ();
	}
	Surf_DrawWorld();
	RQ_RenderBatchClear();
	
	D3D9_Set2D ();
}

void	(D3D9_R_NewMap)					(void);
void	(D3D9_R_PreNewMap)				(void);

void	(D3D9_R_PushDlights)			(void);
void	(D3D9_R_AddStain)				(vec3_t org, float red, float green, float blue, float radius);
void	(D3D9_R_LessenStains)			(void);

qboolean (D3D9_VID_Init)				(rendererstate_t *info, unsigned char *palette);
void	 (D3D9_VID_DeInit)				(void);
void	(D3D9_VID_SetPalette)			(unsigned char *palette);
void	(D3D9_VID_ShiftPalette)			(unsigned char *palette);
char	*(D3D9_VID_GetRGBInfo)			(int prepad, int *truevidwidth, int *truevidheight);
void	(D3D9_VID_SetWindowCaption)		(char *msg);

void	(D3D9_SCR_UpdateScreen)			(void);

void D3D9BE_RenderToTextureUpdate2d(qboolean destchanged)
{
}

rendererinfo_t d3d9rendererinfo =
{
	"Direct3D9",
	{
		"D3D9",
		"D3D",
		"Direct3d",
		"DirectX",
	},
	QR_DIRECT3D9,

	D3D9_Draw_Init,
	D3D9_Draw_Shutdown,

	D3D9_LoadTexture,
	D3D9_LoadTexture8Pal24,
	D3D9_LoadTexture8Pal32,
	D3D9_LoadCompressed,
	D3D9_FindTexture,
	D3D9_AllocNewTexture,
	D3D9_Upload,
	D3D9_DestroyTexture,

	D3D9_R_Init,
	D3D9_R_DeInit,
	D3D9_R_RenderView,

	D3D9_R_NewMap,
	D3D9_R_PreNewMap,

	D3D9_VID_Init,
	D3D9_VID_DeInit,
	D3D9_VID_SwapBuffers,
	D3D9_VID_ApplyGammaRamps,
	D3D9_VID_SetWindowCaption,
	D3D9_VID_GetRGBInfo,

	D3D9_SCR_UpdateScreen,

	D3D9BE_SelectMode,
	D3D9BE_DrawMesh_List,
	D3D9BE_DrawMesh_Single,
	D3D9BE_SubmitBatch,
	D3D9BE_GetTempBatch,
	D3D9BE_DrawWorld,
	D3D9BE_Init,
	D3D9BE_GenBrushModelVBO,
	D3D9BE_ClearVBO,
	D3D9BE_UploadAllLightmaps,
	D3D9BE_SelectEntity,
	D3D9BE_SelectDLight,
	D3D9BE_Scissor,
	D3D9BE_LightCullModel,

	D3D9BE_VBO_Begin,
	D3D9BE_VBO_Data,
	D3D9BE_VBO_Finish,
	D3D9BE_VBO_Destroy,

	D3D9BE_RenderToTextureUpdate2d,

	"no more"
};

#endif
