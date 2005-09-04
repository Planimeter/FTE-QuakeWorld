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
// in_win.c -- windows 95 mouse and joystick code
// 02/21/97 JCB Added extended DirectInput code to support external controllers.

#include "quakedef.h"
#include "winquake.h"
//#include "dosisms.h"

#define USINGRAWINPUT

#ifdef USINGRAWINPUT
#include "in_raw.h"
#endif

#ifndef NODIRECTX
#include <dinput.h>

#ifdef _MSC_VER
#pragma comment (lib, "dxguid.lib")
#endif

#define DINPUT_BUFFERSIZE           16
#define iDirectInputCreate(a,b,c,d)	pDirectInputCreate(a,b,c,d)

HRESULT (WINAPI *pDirectInputCreate)(HINSTANCE hinst, DWORD dwVersion,
	LPDIRECTINPUT * lplpDirectInput, LPUNKNOWN punkOuter);

#endif

#define DINPUT_VERSION_DX3 0x0300
#define DINPUT_VERSION_DX7 0x0700

// mouse variables
cvar_t	m_filter = {"m_filter","0"};
cvar_t  m_accel = {"m_accel", "0"};
cvar_t	m_forcewheel = {"m_forcewheel", "1"};
cvar_t	in_mwhook = {"in_mwhook","0", NULL, CVAR_ARCHIVE};
cvar_t	in_dinput = {"in_dinput","0", NULL, CVAR_ARCHIVE};

cvar_t	m_accel_noforce = {"m_accel_noforce", "0"};
cvar_t  m_threshold_noforce = {"m_threshold_noforce", "0"};

cvar_t	cl_keypad = {"cl_keypad", "0"};

typedef struct {
	union {
		struct { // serial mouse
			HANDLE comhandle;
			HANDLE threadhandle;
			DWORD threadid;
		};
		HANDLE rawinputhandle; // raw input
	};

	int numbuttons;

	volatile int buttons;
	volatile int oldbuttons;
	volatile int wheeldelta;

	volatile int delta[2];
	int old_delta[2];
	int accum[2];

	int pos[2];
} mouse_t;

mouse_t sysmouse;
#ifdef SERIALMOUSE
mouse_t serialmouse;
#endif

//int			mouse_buttons;
//int			mouse_oldbuttonstate;
//int			mouse_x, mouse_y, old_mouse_x, old_mouse_y, mx_accum, my_accum;

static qboolean	restore_spi;
static int		originalmouseparms[3], newmouseparms[3] = {0, 0, 0};
qboolean		mouseinitialized;
static qboolean	mouseparmsvalid, mouseactivatetoggle;
static qboolean	mouseshowtoggle = 1;
static qboolean	dinput_acquired;
unsigned int uiWheelMessage;

qboolean	mouseactive;

// joystick defines and variables
// where should defines be moved?
#define JOY_ABSOLUTE_AXIS	0x00000000		// control like a joystick
#define JOY_RELATIVE_AXIS	0x00000010		// control like a mouse, spinner, trackball
#define	JOY_MAX_AXES		6				// X, Y, Z, R, U, V
#define JOY_AXIS_X			0
#define JOY_AXIS_Y			1
#define JOY_AXIS_Z			2
#define JOY_AXIS_R			3
#define JOY_AXIS_U			4
#define JOY_AXIS_V			5

enum _ControlList
{
	AxisNada = 0, AxisForward, AxisLook, AxisSide, AxisTurn
};

DWORD	dwAxisFlags[JOY_MAX_AXES] =
{
	JOY_RETURNX, JOY_RETURNY, JOY_RETURNZ, JOY_RETURNR, JOY_RETURNU, JOY_RETURNV
};

DWORD	dwAxisMap[JOY_MAX_AXES];
DWORD	dwControlMap[JOY_MAX_AXES];
PDWORD	pdwRawValue[JOY_MAX_AXES];

#ifdef IN_XFLIP
cvar_t	in_xflip = {"in_xflip", "0"};
#endif

// none of these cvars are saved over a session
// this means that advanced controller configuration needs to be executed
// each time.  this avoids any problems with getting back to a default usage
// or when changing from one controller to another.  this way at least something
// works.
cvar_t	in_joystick = {"joystick","0", NULL, CVAR_ARCHIVE};
cvar_t	joy_name = {"joyname", "joystick"};
cvar_t	joy_advanced = {"joyadvanced", "0"};
cvar_t	joy_advaxisx = {"joyadvaxisx", "0"};
cvar_t	joy_advaxisy = {"joyadvaxisy", "0"};
cvar_t	joy_advaxisz = {"joyadvaxisz", "0"};
cvar_t	joy_advaxisr = {"joyadvaxisr", "0"};
cvar_t	joy_advaxisu = {"joyadvaxisu", "0"};
cvar_t	joy_advaxisv = {"joyadvaxisv", "0"};
cvar_t	joy_forwardthreshold = {"joyforwardthreshold", "0.15"};
cvar_t	joy_sidethreshold = {"joysidethreshold", "0.15"};
cvar_t	joy_pitchthreshold = {"joypitchthreshold", "0.15"};
cvar_t	joy_yawthreshold = {"joyyawthreshold", "0.15"};
cvar_t	joy_forwardsensitivity = {"joyforwardsensitivity", "-1.0"};
cvar_t	joy_sidesensitivity = {"joysidesensitivity", "-1.0"};
cvar_t	joy_pitchsensitivity = {"joypitchsensitivity", "1.0"};
cvar_t	joy_yawsensitivity = {"joyyawsensitivity", "-1.0"};
cvar_t	joy_wwhack1 = {"joywwhack1", "0.0"};
cvar_t	joy_wwhack2 = {"joywwhack2", "0.0"};

qboolean	joy_avail, joy_advancedinit, joy_haspov;
DWORD		joy_oldbuttonstate, joy_oldpovstate;

int			joy_id;
DWORD		joy_flags;
DWORD		joy_numbuttons;

#ifndef NODIRECTX
// devices
LPDIRECTINPUT		g_pdi;
LPDIRECTINPUTDEVICE	g_pMouse;

static HINSTANCE hInstDI;

// current DirectInput version in use, 0 means using no DirectInput
static int dinput; 

typedef struct MYDATA {
	LONG  lX;                   // X axis goes here
	LONG  lY;                   // Y axis goes here
	LONG  lZ;                   // Z axis goes here
	BYTE  bButtonA;             // One button goes here
	BYTE  bButtonB;             // Another button goes here
	BYTE  bButtonC;             // Another button goes here
	BYTE  bButtonD;             // Another button goes here
#if (DIRECTINPUT_VERSION >= DINPUT_VERSION_DX7)
	BYTE  bButtonE;             // DX7 buttons
	BYTE  bButtonF;             
	BYTE  bButtonG;             
	BYTE  bButtonH;
#endif
} MYDATA;

static DIOBJECTDATAFORMAT rgodf[] = {
  { &GUID_XAxis,    FIELD_OFFSET(MYDATA, lX),       DIDFT_AXIS | DIDFT_ANYINSTANCE,   0,},
  { &GUID_YAxis,    FIELD_OFFSET(MYDATA, lY),       DIDFT_AXIS | DIDFT_ANYINSTANCE,   0,},
  { &GUID_ZAxis,    FIELD_OFFSET(MYDATA, lZ),       0x80000000 | DIDFT_AXIS | DIDFT_ANYINSTANCE,   0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonA), DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonB), DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonC), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonD), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
#if (DIRECTINPUT_VERSION >= DINPUT_VERSION_DX7)
  { 0,              FIELD_OFFSET(MYDATA, bButtonE), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonF), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonG), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
  { 0,              FIELD_OFFSET(MYDATA, bButtonH), 0x80000000 | DIDFT_BUTTON | DIDFT_ANYINSTANCE, 0,},
#endif
};

#define NUM_OBJECTS (sizeof(rgodf) / sizeof(rgodf[0]))

static DIDATAFORMAT	df = {
	sizeof(DIDATAFORMAT),       // this structure
	sizeof(DIOBJECTDATAFORMAT), // size of object data format
	DIDF_RELAXIS,               // absolute axis coordinates
	sizeof(MYDATA),             // device data size
	NUM_OBJECTS,                // number of objects
	rgodf,                      // and here they are
};

#if (DIRECTINPUT_VERSION >= DINPUT_VERSION_DX7)
// DX7 devices
LPDIRECTINPUT7		g_pdi7;
LPDIRECTINPUTDEVICE7	g_pMouse7;

// DX7 specific calls
#define iDirectInputCreateEx(a,b,c,d,e)	pDirectInputCreateEx(a,b,c,d,e)

static HRESULT (WINAPI *pDirectInputCreateEx)(HINSTANCE hinst,
		DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter);
#endif

#else
#define dinput 0
#endif

static JOYINFOEX	ji;

// raw input specific defines
#ifdef USINGRAWINPUT
// defines
#define MAX_RI_DEVICE_SIZE 128
#define INIT_RIBUFFER_SIZE (sizeof(RAWINPUTHEADER)+sizeof(RAWMOUSE))

#define RI_RAWBUTTON_MASK 0x000003E0
#define RI_INVALID_POS    0x80000000

// raw input dynamic functions
typedef INT (WINAPI *pGetRawInputDeviceList)(OUT PRAWINPUTDEVICELIST pRawInputDeviceList, IN OUT PINT puiNumDevices, IN UINT cbSize);
typedef INT(WINAPI *pGetRawInputData)(IN HRAWINPUT hRawInput, IN UINT uiCommand, OUT LPVOID pData, IN OUT PINT pcbSize, IN UINT cbSizeHeader);
typedef INT(WINAPI *pGetRawInputDeviceInfoA)(IN HANDLE hDevice, IN UINT uiCommand, OUT LPVOID pData, IN OUT PINT pcbSize);
typedef BOOL (WINAPI *pRegisterRawInputDevices)(IN PCRAWINPUTDEVICE pRawInputDevices, IN UINT uiNumDevices, IN UINT cbSize);

pGetRawInputDeviceList _GRIDL;
pGetRawInputData _GRID;
pGetRawInputDeviceInfoA _GRIDIA;
pRegisterRawInputDevices _RRID;

mouse_t *rawmice;
int rawmicecount;
RAWINPUT *raw;
int ribuffersize;

cvar_t in_rawinput = {"in_rawinput", "0"};
cvar_t in_rawinput_combine = {"in_rawinput_combine", "0"};
cvar_t in_rawinput_rdp = {"in_rawinput_rdp", "0"};
#endif

// forward-referenced functions
void IN_StartupJoystick (void);
void Joy_AdvancedUpdate_f (void);
void IN_JoyMove (usercmd_t *cmd, int pnum);

/*
===========
Force_CenterView_f
===========
*/
void Force_CenterView_f (void)
{
	cl.viewangles[0][PITCH] = 0;
}

typedef void (*MW_DllFunc1)(void);
typedef int (*MW_DllFunc2)(HWND);

MW_DllFunc1 DLL_MW_RemoveHook = NULL;
MW_DllFunc2 DLL_MW_SetHook = NULL;
qboolean MW_Hook_enabled = false;
HINSTANCE mw_hDLL;
qboolean MW_Hook_allowed;

static void MW_Set_Hook (void)
{
	if (!mainwindow)
		return;
	if (!MW_Hook_allowed && !in_mwhook.value)
		return;

	if (MW_Hook_enabled)
	{
		Con_Printf("MouseWare hook already loaded\n");
		return;
	}

	if (!(mw_hDLL = LoadLibrary("mw_hook.dll")))
	{
		Con_Printf("Couldn't find mw_hook.dll\n");
		in_mwhook.value = 0;
		MW_Hook_allowed=0;
		return;
	}
	DLL_MW_RemoveHook = (MW_DllFunc1) GetProcAddress(mw_hDLL, "MW_RemoveHook");
	DLL_MW_SetHook = (MW_DllFunc2) GetProcAddress(mw_hDLL, "MW_SetHook");
	if (!DLL_MW_SetHook || !DLL_MW_RemoveHook)
	{
		Con_Printf("Error initializing MouseWare hook\n");
		FreeLibrary(mw_hDLL);
		in_mwhook.value = 0;
		MW_Hook_allowed=0;
		return;
	}
	if (!DLL_MW_SetHook(mainwindow))
	{
		Con_Printf("Couldn't initialize MouseWare hook\n");
		FreeLibrary(mw_hDLL);
		in_mwhook.value = 0;
		MW_Hook_allowed=0;
		return;
	}
	MW_Hook_enabled = true;
//	Con_Printf("MouseWare hook initialized\n");
}

static void MW_Remove_Hook (void)
{
	if (MW_Hook_enabled)
	{
		DLL_MW_RemoveHook();
		FreeLibrary(mw_hDLL);
		MW_Hook_enabled = false;
//		Con_Printf("MouseWare hook removed\n");
		return;
	}
	Con_Printf("MouseWare hook not loaded\n");
}

static void MW_Shutdown(void)
{
	if (!MW_Hook_enabled)
		return;
	MW_Remove_Hook();
}

void MW_Hook_Message (long buttons)
{
	static long old_buttons = 0;

	buttons &= 0xFFFF;
	switch (buttons ^ old_buttons)
	{
		case 8:		Key_Event(K_MOUSE4, buttons > old_buttons ? true : false); break;
		case 16:	Key_Event(K_MOUSE5, buttons > old_buttons ? true : false); break;
		case 32:	Key_Event(K_MOUSE6, buttons > old_buttons ? true : false); break;
		case 64:	Key_Event(K_MOUSE7, buttons > old_buttons ? true : false); break;
		case 128:	Key_Event(K_MOUSE8, buttons > old_buttons ? true : false); break;
		default:	break;
	}

	old_buttons = buttons;
}

/*
===========
IN_UpdateClipCursor
===========
*/
void IN_UpdateClipCursor (void)
{

	if (mouseinitialized && mouseactive && !dinput)
	{
		ClipCursor (&window_rect);
	}
}


/*
===========
IN_ShowMouse
===========
*/
void IN_ShowMouse (void)
{

	if (!mouseshowtoggle)
	{
		ShowCursor (TRUE);
		mouseshowtoggle = 1;
	}
}


/*
===========
IN_HideMouse
===========
*/
void IN_HideMouse (void)
{

	if (mouseshowtoggle)
	{
		ShowCursor (FALSE);
		mouseshowtoggle = 0;
	}
}


/*
===========
IN_ActivateMouse
===========
*/
void IN_ActivateMouse (void)
{

	mouseactivatetoggle = true;

	if (mouseinitialized)
	{
#ifndef NODIRECTX
#if (DIRECTINPUT_VERSION >= DINPUT_VERSION_DX7)
		if (dinput >= DINPUT_VERSION_DX7)
		{
			if (g_pMouse7)
			{
				if (!dinput_acquired)
				{
					IDirectInputDevice7_Acquire(g_pMouse7);
					dinput_acquired = true;
				}
			}
			else
			{
				return;
			}
		}
		else
#endif
		if (dinput)
		{
			if (g_pMouse)
			{
				if (!dinput_acquired)
				{
					IDirectInputDevice_Acquire(g_pMouse);
					dinput_acquired = true;
				}
			}
			else
			{
				return;
			}
		}
		else
#endif
		{
			if (mouseparmsvalid)
				restore_spi = SystemParametersInfo (SPI_SETMOUSE, 0, newmouseparms, 0);

			SetCursorPos (window_center_x, window_center_y);
			SetCapture (mainwindow);
			ClipCursor (&window_rect);
		}

		MW_Set_Hook();

		mouseactive = true;
	}
}


/*
===========
IN_SetQuakeMouseState
===========
*/
void IN_SetQuakeMouseState (void)
{
	if (mouseactivatetoggle)
		IN_ActivateMouse ();
	else
		IN_DeactivateMouse();
}


/*
===========
IN_DeactivateMouse
===========
*/
void IN_DeactivateMouse (void)
{

	mouseactivatetoggle = false;

	if (mouseinitialized)
	{
#ifndef NODIRECTX
#if (DIRECTINPUT_VERSION >= DINPUT_VERSION_DX7)
		if (dinput >= DINPUT_VERSION_DX7)
		{
			if (g_pMouse7)
			{
				if (dinput_acquired)
				{
					IDirectInputDevice_Unacquire(g_pMouse7);
					dinput_acquired = false;
				}
			}
			else
			{
				return;
			}
		}
		else
#endif
		if (dinput)
		{
			if (g_pMouse)
			{
				if (dinput_acquired)
				{
					IDirectInputDevice_Unacquire(g_pMouse);
					dinput_acquired = false;
				}
			}
		}
		else
#endif
		{
			if (restore_spi)
				SystemParametersInfo (SPI_SETMOUSE, 0, originalmouseparms, 0);

			ClipCursor (NULL);
			ReleaseCapture ();
		}

		MW_Shutdown();

		mouseactive = false;
	}
}


/*
===========
IN_RestoreOriginalMouseState
===========
*/
void IN_RestoreOriginalMouseState (void)
{
	if (mouseactivatetoggle)
	{
		IN_DeactivateMouse ();
		mouseactivatetoggle = true;
	}

// try to redraw the cursor so it gets reinitialized, because sometimes it
// has garbage after the mode switch
	ShowCursor (TRUE);
	ShowCursor (FALSE);
}

#ifndef NODIRECTX
BOOL CALLBACK IN_EnumerateDevices(LPCDIDEVICEINSTANCE inst, LPVOID parm)
{
	Con_SafePrintf("Found: %s\n", inst->tszProductName);

	return DIENUM_CONTINUE;
}
/*
===========
IN_InitDInput
===========
*/
int IN_InitDInput (void)
{
    HRESULT		hr;
	DIPROPDWORD	dipdw = {
		{
			sizeof(DIPROPDWORD),        // diph.dwSize
			sizeof(DIPROPHEADER),       // diph.dwHeaderSize
			0,                          // diph.dwObj
			DIPH_DEVICE,                // diph.dwHow
		},
		DINPUT_BUFFERSIZE,              // dwData
	};

	if (!hInstDI)
	{
		hInstDI = LoadLibrary("dinput.dll");
		
		if (hInstDI == NULL)
		{
			Con_SafePrintf ("Couldn't load dinput.dll\n");
			return 0;
		}
	}

#if (DIRECTINPUT_VERSION >= DINPUT_VERSION_DX7)
	if (!pDirectInputCreateEx)
		pDirectInputCreateEx = (void *)GetProcAddress(hInstDI,"DirectInputCreateEx");

	if (pDirectInputCreateEx) // use DirectInput 7
	{
		// register with DirectInput and get an IDirectInput to play with.
		hr = iDirectInputCreateEx(global_hInstance, DINPUT_VERSION_DX7, &IID_IDirectInput7, &g_pdi7, NULL);

		if (FAILED(hr))
			return 0;

		IDirectInput7_EnumDevices(g_pdi7, 0, &IN_EnumerateDevices, NULL, DIEDFL_ATTACHEDONLY);

		// obtain an interface to the system mouse device.
		hr = IDirectInput7_CreateDeviceEx(g_pdi7, &GUID_SysMouse, &IID_IDirectInputDevice7, &g_pMouse7, NULL);

		if (FAILED(hr)) {
			Con_SafePrintf ("Couldn't open DI7 mouse device\n");
			return 0;
		}

		// set the data format to "mouse format".
		hr = IDirectInputDevice7_SetDataFormat(g_pMouse7, &df);

		if (FAILED(hr)) {
			Con_SafePrintf ("Couldn't set DI7 mouse format\n");
			return 0;
		}

		// set the cooperativity level.
		hr = IDirectInputDevice7_SetCooperativeLevel(g_pMouse7, mainwindow,
			DISCL_EXCLUSIVE | DISCL_FOREGROUND);

		if (FAILED(hr)) {
			Con_SafePrintf ("Couldn't set DI7 coop level\n");
			return 0;
		}

		// set the buffer size to DINPUT_BUFFERSIZE elements.
		// the buffer size is a DWORD property associated with the device
		hr = IDirectInputDevice7_SetProperty(g_pMouse7, DIPROP_BUFFERSIZE, &dipdw.diph);

		if (FAILED(hr)) {
			Con_SafePrintf ("Couldn't set DI7 buffersize\n");
			return 0;
		}

		return DINPUT_VERSION_DX7;
	}
#endif

	if (!pDirectInputCreate)
	{
		pDirectInputCreate = (void *)GetProcAddress(hInstDI,"DirectInputCreateA");

		if (!pDirectInputCreate)
		{
			Con_SafePrintf ("Couldn't get DI3 proc addr\n");
			return 0;
		}
	}

// register with DirectInput and get an IDirectInput to play with.
	hr = iDirectInputCreate(global_hInstance, DINPUT_VERSION_DX3, &g_pdi, NULL);

	if (FAILED(hr))
	{
		return 0;
	}
	IDirectInput_EnumDevices(g_pdi, 0, &IN_EnumerateDevices, NULL, DIEDFL_ATTACHEDONLY);

// obtain an interface to the system mouse device.
	hr = IDirectInput_CreateDevice(g_pdi, &GUID_SysMouse, &g_pMouse, NULL);

	if (FAILED(hr))
	{
		Con_SafePrintf ("Couldn't open DI3 mouse device\n");
		return 0;
	}

// set the data format to "mouse format".
	hr = IDirectInputDevice_SetDataFormat(g_pMouse, &df);

	if (FAILED(hr))
	{
		Con_SafePrintf ("Couldn't set DI3 mouse format\n");
		return 0;
	}

// set the cooperativity level.
	hr = IDirectInputDevice_SetCooperativeLevel(g_pMouse, mainwindow,
			DISCL_EXCLUSIVE | DISCL_FOREGROUND);

	if (FAILED(hr))
	{
		Con_SafePrintf ("Couldn't set DI3 coop level\n");
		return 0;
	}


// set the buffer size to DINPUT_BUFFERSIZE elements.
// the buffer size is a DWORD property associated with the device
	hr = IDirectInputDevice_SetProperty(g_pMouse, DIPROP_BUFFERSIZE, &dipdw.diph);

	if (FAILED(hr))
	{
		Con_SafePrintf ("Couldn't set DI3 buffersize\n");
		return 0;
	}

	return DINPUT_VERSION_DX3;
}

void IN_CloseDInput (void)
{
#if (DIRECTINPUT_VERSION >= DINPUT_VERSION_DX7)
	if (g_pMouse7)
	{
		IDirectInputDevice7_Release(g_pMouse7);
		g_pMouse7 = NULL;
	}
	if (g_pdi7)
	{
		IDirectInput7_Release(g_pdi7);
		g_pdi7 = NULL;
	}
#endif
	if (g_pMouse)
	{
		IDirectInputDevice_Release(g_pMouse);
		g_pMouse = NULL;
	}
	if (g_pdi)
	{
		IDirectInput_Release(g_pdi);
		g_pdi = NULL;
	}
	if (hInstDI)
	{
		FreeLibrary(hInstDI);
		hInstDI = NULL;
		pDirectInputCreate = NULL;
	}

}
#endif

#ifdef USINGRAWINPUT
void IN_RawInput_DeInit(void)
{
	RAWINPUTDEVICE Rid;

	if (rawmicecount < 1)
		return;

	// deregister raw input
	Rid.usUsagePage = 0x01; 
	Rid.usUsage = 0x02; 
	Rid.dwFlags = RIDEV_REMOVE;
	Rid.hwndTarget = NULL;

	(*_RRID)(&Rid, 1, sizeof(Rid));

	Z_Free(rawmice);

	// dealloc mouse structure
	rawmicecount = 0;
}
#endif

void IN_SetSerialBoad(HANDLE port, int boadrate)
{
	DCB dcb;
	memset(&dcb, 0, sizeof(dcb));

	dcb.DCBlength = sizeof(dcb);
	GetCommState(port, &dcb);

	dcb.fBinary = TRUE;
	dcb.fParity = FALSE;
	dcb.fOutxCtsFlow = 0;
	dcb.fOutxDsrFlow = 0;
	dcb.fDtrControl = DTR_CONTROL_DISABLE;
	dcb.fDsrSensitivity = 0;
	dcb.fTXContinueOnXoff = FALSE;
	dcb.fOutX = 0; 
	dcb.fInX = 0;
	dcb.fErrorChar = 0;
	dcb.fNull = 0;
	dcb.fRtsControl = RTS_CONTROL_DISABLE;
	dcb.fAbortOnError = 0;
	dcb.ByteSize = 7;
	dcb.StopBits = ONESTOPBIT;
	dcb.Parity = NOPARITY;
	dcb.ErrorChar = 0;
	dcb.EvtChar = 0;
	dcb.EofChar = 0;

	dcb.BaudRate = boadrate;     // set the baud rate
	SetCommState(port, &dcb);

	//now get the com port to electricute the mouse... powering up...
	EscapeCommFunction(port, SETDTR);
	EscapeCommFunction(port, SETRTS);
}

//microsoft's 2 button mouse protocol
unsigned int _stdcall IN_SerialMSRun(void *param)
{
	mouse_t *mouse = param;
	char code[3];
	DWORD read;
	int total=0;
	IN_SetSerialBoad(mouse->comhandle, 1200);
	total=0;
	while(1)
	{
		ReadFile(mouse->comhandle, code, sizeof(code)-total, &read, NULL);
		total+=read;
		if (total == 3)
		{
			mouse->buttons = 0; /* No button - should only happen on an error */
			if ((code[0] & 0x20) != 0)
				mouse->buttons |= 1;
			else if ((code[0] & 0x10) != 0)
				mouse->buttons |= 2;
			mouse->delta[0] = (code[0] & 0x03) * 64 + (code[1] & 0x3F);
			if (mouse->delta[0] > 127)
				mouse->delta[0] = mouse->delta[0] - 256;
			mouse->delta[1] = (code[0] & 0x0C) * 16 + (code[2] & 0x3F);
			if (mouse->delta[1] > 127)
				mouse->delta[1] = mouse->delta[1] - 256;

//			Con_Printf("%i %i %i\n", serialmousexmove, serialmouseymove, serialmousebuttons);
			total=0;
		}
	}
	return true;
}

//microsofts's intellimouse protocol
//used by most wheel mice.
unsigned long __stdcall IN_SerialMSIntelliRun(void *param)
{
	mouse_t *mouse = param;
	unsigned char code[80];
	DWORD read, total=0;
	IN_SetSerialBoad(mouse->comhandle, 1200);

	ReadFile(mouse->comhandle, code, 11*4+2, &read, NULL);	//header info which we choose to ignore

	mouse->numbuttons = 3;

	while(1)
	{
		ReadFile(mouse->comhandle, code+total, 4-total, &read, NULL);
		total+=read;
		if (total >= 4)
		{
//			if (mouse->oldbuttons == mouse->buttons)
//				mouse->buttons=0;	//don't clear prematurly.
			mouse->buttons =	((code[0] & 0x20) >> 5)	/* left */
								| ((code[3] & 0x10) >> 2)	/* middle */
								| ((code[0] & 0x10) >> 3);	/* right */
			mouse->delta[0] +=	(signed char)(((code[0] & 0x03) << 6) | (code[1]/* & 0x3F*/));
			mouse->delta[1] +=	(signed char)(((code[0] & 0x0C) << 4) | (code[2]/* & 0x3F*/));

			if (m_forcewheel.value)
				mouse->wheeldelta += (signed char)((code[3] & 0x0f)<<4)/16;

			total=0;
		}
		else		//an else shouldn't happen...
		{
			Sleep(4);
	//		return false;
		}
	}
	return true;
}

#ifdef USINGRAWINPUT
// raw input registration functions
int IN_RawInput_Register(void)
{
	// This function registers to receive the WM_INPUT messages
	RAWINPUTDEVICE Rid; // Register only for mouse messages from wm_input.  

	//register to get wm_input messages
	Rid.usUsagePage = 0x01; 
	Rid.usUsage = 0x02; 
	Rid.dwFlags = RIDEV_NOLEGACY; // adds HID mouse and also ignores legacy mouse messages
	Rid.hwndTarget = NULL;

	// Register to receive the WM_INPUT message for any change in mouse (buttons, wheel, and movement will all generate the same message)
	if (!(*_RRID)(&Rid, 1, sizeof(Rid)))
		return 0;

	return 1;
}

int IN_RawInput_IsRDPMouse(char *cDeviceString)
{
	char cRDPString[] = "\\??\\Root#RDP_MOU#";
	int i;

	if (strlen(cDeviceString) < strlen(cRDPString)) {
		return 0;
	}

	for (i = strlen(cRDPString) - 1; i >= 0; i--)
	{
		if (cDeviceString[i] != cRDPString[i])
			return 0;
	}

	return 1; // is RDP mouse
}

void IN_RawInput_Init(void)
{
	  // "0" to exclude, "1" to include
	PRAWINPUTDEVICELIST pRawInputDeviceList;
	int inputdevices, i, j, mtemp;
	char dname[MAX_RI_DEVICE_SIZE];

	// Return 0 if rawinput is not available
	HMODULE user32 = LoadLibrary("user32.dll");
	if (!user32)
	{
		Con_SafePrintf("Raw input: unable to load user32.dll\n");
		return;
	}
	_RRID = (pRegisterRawInputDevices)GetProcAddress(user32,"RegisterRawInputDevices");
	if (!_RRID)
	{
		Con_SafePrintf("Raw input: function RegisterRawInputDevices could not be registered\n");
		return;
	}
	_GRIDL = (pGetRawInputDeviceList)GetProcAddress(user32,"GetRawInputDeviceList");
	if (!_GRIDL)
	{
		Con_SafePrintf("Raw input: function GetRawInputDeviceList could not be registered\n");
		return;
	}
	_GRIDIA = (pGetRawInputDeviceInfoA)GetProcAddress(user32,"GetRawInputDeviceInfoA");
	if (!_GRIDIA)
	{
		Con_SafePrintf("Raw input: function GetRawInputDeviceInfoA could not be registered\n");
		return;
	}
	_GRID = (pGetRawInputData)GetProcAddress(user32,"GetRawInputData");
	if (!_GRID)
	{
		Con_SafePrintf("Raw input: function GetRawInputData could not be registered\n");
		return;
	}

	rawmicecount = 0;
	rawmice = NULL;
	raw = NULL;
	ribuffersize = 0;

	// 1st call to GetRawInputDeviceList: Pass NULL to get the number of devices.
	if ((*_GRIDL)(NULL, &inputdevices, sizeof(RAWINPUTDEVICELIST)) != 0)
	{
		Con_SafePrintf("Raw input: unable to count raw input devices\n");
		return;
	}

	// Allocate the array to hold the DeviceList
	pRawInputDeviceList = Z_Malloc(sizeof(RAWINPUTDEVICELIST) * inputdevices);

	// 2nd call to GetRawInputDeviceList: Pass the pointer to our DeviceList and GetRawInputDeviceList() will fill the array
	if ((*_GRIDL)(pRawInputDeviceList, &inputdevices, sizeof(RAWINPUTDEVICELIST)) == -1)
	{
		Con_SafePrintf("Raw input: unable to get raw input device list\n");
		return;
	}

	// Loop through all devices and count the mice
	for (i = 0, mtemp = 0; i < inputdevices; i++)
	{
		if (pRawInputDeviceList[i].dwType == RIM_TYPEMOUSE) 
		{
			j = MAX_RI_DEVICE_SIZE;

			// Get the device name and use it to determine if it's the RDP Terminal Services virtual device.
			if ((*_GRIDIA)(pRawInputDeviceList[i].hDevice, RIDI_DEVICENAME, dname, &j) < 0)
				dname[0] = 0;

			if (!(in_rawinput_rdp.value) && IN_RawInput_IsRDPMouse(dname)) // use rdp mouse (cvar)
				continue;

			// advance temp device count
			mtemp++;
		}
	}

	// exit out if no devices found
	if (!mtemp)
	{
		Con_SafePrintf("Raw input: no usable device found\n");
		return;
	}

	// Loop again and bind devices
	rawmice = Z_Malloc(sizeof(mouse_t) * mtemp);
	for (i = 0; i < inputdevices; i++)
	{
		if (pRawInputDeviceList[i].dwType == RIM_TYPEMOUSE)
		{
			j = MAX_RI_DEVICE_SIZE;

			// Get the device name and use it to determine if it's the RDP Terminal Services virtual device.
			if ((*_GRIDIA)(pRawInputDeviceList[i].hDevice, RIDI_DEVICENAME, dname, &j) < 0)
				dname[0] = 0;

			if (!(in_rawinput_rdp.value) && IN_RawInput_IsRDPMouse(dname)) // use rdp mouse (cvar)
				continue;

			// print pretty message about the mouse
			dname[MAX_RI_DEVICE_SIZE - 1] = 0;
			for (mtemp = strlen(dname); mtemp >= 0; mtemp--)
			{
				if (dname[mtemp] == '#')
				{
					dname[mtemp + 1] = 0;
					break;
				}
			}
			Con_SafePrintf("Raw input: [%i] %s\n", i, dname);

			// set handle
			rawmice[rawmicecount].rawinputhandle = pRawInputDeviceList[i].hDevice;
			rawmice[rawmicecount].numbuttons = 10;
			rawmice[rawmicecount].pos[0] = RI_INVALID_POS;
			rawmicecount++;
		}
	}

   
	// free the RAWINPUTDEVICELIST
	Z_Free(pRawInputDeviceList);

	// finally, register to recieve raw input WM_INPUT messages
	if (!IN_RawInput_Register()) {
		Con_SafePrintf("Raw input: unable to register raw input\n");

		// quick deinit
		rawmicecount = 0;
		Z_Free(rawmice);
		return;
	}

	// alloc raw input buffer
	raw = BZ_Malloc(INIT_RIBUFFER_SIZE);
	ribuffersize = INIT_RIBUFFER_SIZE;

	Con_SafePrintf("Raw input: initialized with %i mice\n", rawmicecount);

	return; // success
}
#endif

/*
===========
IN_StartupMouse
===========
*/
void IN_StartupMouse (void)
{
	if ( COM_CheckParm ("-nomouse") ) 
		return; 

	mouseinitialized = true;

#ifndef NODIRECTX
	if (in_dinput.value)
	{
		dinput = IN_InitDInput ();

		if (dinput)
		{
			Con_SafePrintf ("DirectInput initialized, version %i\n", (dinput >> 8 & 0xFF));
		}
		else
		{
			Con_SafePrintf ("DirectInput not initialized\n");
		}
	}
	else
		dinput = 0;

	if (!dinput)
#endif
	{
		if (!mouseparmsvalid)
			mouseparmsvalid = SystemParametersInfo (SPI_GETMOUSE, 0, originalmouseparms, 0);

		if (mouseparmsvalid)
		{
			if ( m_accel_noforce.value ) 
				newmouseparms[2] = originalmouseparms[2];

			if ( m_threshold_noforce.value ) 
			{
				newmouseparms[0] = originalmouseparms[0];
				newmouseparms[1] = originalmouseparms[1];
			}
		}

#ifdef USINGRAWINPUT
		if (in_rawinput.value)
		{
			IN_RawInput_Init();
		}
#endif
	}

	if (COM_CheckParm("-m_mwhook"))
		MW_Hook_allowed = true;

	sysmouse.numbuttons = 10;

// if a fullscreen video mode was set before the mouse was initialized,
// set the mouse state appropriately
	if (mouseactivatetoggle)
		IN_ActivateMouse ();

#ifdef SERIALMOUSE
	if (serialmouse.comhandle)
	{
		TerminateThread(serialmouse.threadhandle, 0);
		CloseHandle(serialmouse.threadhandle);
		CloseHandle(serialmouse.comhandle);
	}
	serialmouse.numbuttons = 0;

	if (COM_CheckParm("-mouse2"))
	{
		serialmouse.comhandle = CreateFile("\\\\.\\COM2",
				GENERIC_READ,
				0,           // share for reading 
                NULL,                      // default security 
                OPEN_EXISTING,             // existing file only 
                FILE_ATTRIBUTE_NORMAL,     // normal file 
                NULL);                     // no attr. template 

		if (serialmouse.comhandle == INVALID_HANDLE_VALUE)
		{
			serialmouse.comhandle = NULL;
			return;
		}
		serialmouse.threadhandle = CreateThread(NULL, 1024, IN_SerialMSIntelliRun, (void *)&serialmouse, CREATE_SUSPENDED, &serialmouse.threadid);
		SetThreadPriority(serialmouse.threadhandle, THREAD_PRIORITY_HIGHEST);
		ResumeThread(serialmouse.threadhandle);
	}
	else
		serialmouse.comhandle = NULL;
#endif
}


/*
===========
IN_Init
===========
*/
void IN_Init (void)
{
	static qboolean firstinit = true;
	if (firstinit)
	{
		//keyboard variables
		Cvar_Register (&cl_keypad, "Input stuff");

		// mouse variables
		Cvar_Register (&m_filter, "Input stuff");
		Cvar_Register (&m_accel, "Input stuff");
		Cvar_Register (&m_forcewheel, "Input stuff");
		Cvar_Register (&in_mwhook, "Input stuff");

		Cvar_Register (&in_dinput, "Input stuff");

		Cvar_Register (&m_accel_noforce, "Input stuff");
		Cvar_Register (&m_threshold_noforce, "Input stuff");

		// this looks strange but quake cmdline definitions
		// and MS documentation don't agree with each other
		if (COM_CheckParm ("-noforcemspd"))
			Cvar_Set(&m_accel_noforce, "1");

		if (COM_CheckParm ("-noforcemaccel"))
			Cvar_Set(&m_threshold_noforce, "1");

		if (COM_CheckParm ("-noforcemparms"))
		{
			Cvar_Set(&m_accel_noforce, "1");
			Cvar_Set(&m_threshold_noforce, "1");
		}

		if (COM_CheckParm ("-dinput"))
			Cvar_Set(&in_dinput, "1");

#ifdef IN_XFLIP
		Cvar_Register (&in_xflip, "Input stuff");
#endif

		// joystick variables		
		Cvar_Register (&in_joystick, "Joystick variables");

		Cvar_Register (&joy_name, "Joystick variables");
		Cvar_Register (&joy_advanced, "Joystick variables");
		Cvar_Register (&joy_advaxisx, "Joystick variables");
		Cvar_Register (&joy_advaxisy, "Joystick variables");
		Cvar_Register (&joy_advaxisz, "Joystick variables");
		Cvar_Register (&joy_advaxisr, "Joystick variables");
		Cvar_Register (&joy_advaxisu, "Joystick variables");
		Cvar_Register (&joy_advaxisv, "Joystick variables");
		Cvar_Register (&joy_forwardthreshold, "Joystick variables");
		Cvar_Register (&joy_sidethreshold, "Joystick variables");
		Cvar_Register (&joy_pitchthreshold, "Joystick variables");
		Cvar_Register (&joy_yawthreshold, "Joystick variables");
		Cvar_Register (&joy_forwardsensitivity, "Joystick variables");
		Cvar_Register (&joy_sidesensitivity, "Joystick variables");
		Cvar_Register (&joy_pitchsensitivity, "Joystick variables");
		Cvar_Register (&joy_yawsensitivity, "Joystick variables");
		Cvar_Register (&joy_wwhack1, "Joystick variables");
		Cvar_Register (&joy_wwhack2, "Joystick variables");

		Cmd_AddCommand ("force_centerview", Force_CenterView_f);
		Cmd_AddCommand ("joyadvancedupdate", Joy_AdvancedUpdate_f);

		uiWheelMessage = RegisterWindowMessage ( "MSWHEEL_ROLLMSG" );

#ifdef USINGRAWINPUT
		Cvar_Register (&in_rawinput, "Input stuff");
		Cvar_Register (&in_rawinput_combine, "Input stuff");
		Cvar_Register (&in_rawinput_rdp, "Input stuff");
#endif
	}
	else
	{
		IN_StartupMouse ();
		IN_StartupJoystick ();
	}

	firstinit = false;
}

/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown (void)
{
	IN_DeactivateMouse ();
	IN_ShowMouse ();

	mouseparmsvalid = false;

#ifndef NODIRECTX
	IN_CloseDInput();
#endif
#ifdef USINGRAWINPUT
	IN_RawInput_DeInit();
#endif
}


/*
===========
IN_MouseEvent
===========
*/
void IN_MouseEvent (int mstate)
{
	int		i;

	if ((mouseactive || (key_dest != key_console && key_dest != key_game)) && !dinput)
	{
	// perform button actions
		for (i=0 ; i<sysmouse.numbuttons ; i++)
		{
			if ( (mstate & (1<<i)) &&
				!(sysmouse.oldbuttons & (1<<i)) )
			{
				Key_Event (K_MOUSE1 + i, true);
			}

			if ( !(mstate & (1<<i)) &&
				(sysmouse.oldbuttons & (1<<i)) )
			{
					Key_Event (K_MOUSE1 + i, false);
			}
		}	
			
		sysmouse.oldbuttons = mstate;
	}
}

static void ProcessMouse(mouse_t *mouse, usercmd_t *cmd, int pnum)
{
	extern int mouseusedforgui, mousecursor_x, mousecursor_y;
	extern int mousemove_x, mousemove_y;

	int mx, my;
	double mouse_x, mouse_y, mouse_deltadist;

	int i;

	// perform button actions
	for (i=0 ; i<mouse->numbuttons ; i++)
	{
		if ( (mouse->buttons & (1<<i)) &&
			!(mouse->oldbuttons & (1<<i)) )
		{
			Key_Event (K_MOUSE1 + i, true);
		}

		if ( !(mouse->buttons & (1<<i)) &&
			(mouse->oldbuttons & (1<<i)) )
		{
				Key_Event (K_MOUSE1 + i, false);
		}
	}
	mouse->oldbuttons = mouse->buttons;
	while(mouse->wheeldelta<0)
	{
		Key_Event (K_MWHEELUP, true);
		Key_Event (K_MWHEELUP, false);
		mouse->wheeldelta++;
	}

	while(mouse->wheeldelta>0)
	{
		Key_Event (K_MWHEELDOWN, true);
		Key_Event (K_MWHEELDOWN, false);
		mouse->wheeldelta--;
	}


	mx = mouse->delta[0];
	mouse->delta[0]=0;
	my = mouse->delta[1];
	mouse->delta[1]=0;


#ifdef IN_XFLIP
	if(in_xflip.value) mx *= -1;
#endif

	if (mouseusedforgui || (key_dest == key_menu && m_state == m_complex) || UI_MenuState())
	{
		mousemove_x += mx;
		mousemove_y += my;
		mousecursor_x += mx;
		mousecursor_y += my;

		if (mousecursor_y<0)
			mousecursor_y=0;
		if (mousecursor_x<0)
			mousecursor_x=0;

		if (mousecursor_x >= vid.width)
			mousecursor_x = vid.width - 1;

		if (mousecursor_y >= vid.height)
			mousecursor_y = vid.height - 1;
		mx=my=0;

		UI_MousePosition(mousecursor_x, mousecursor_y);
	}

	if (m_filter.value)
	{
		double fraction = bound(0, m_filter.value, 2) * 0.5;
		mouse_x = (mx*(1-fraction) + mouse->old_delta[0]*fraction);
		mouse_y = (my*(1-fraction) + mouse->old_delta[1]*fraction);
	}
	else
	{
		mouse_x = mx;
		mouse_y = my;
	}

	mouse->old_delta[0] = mx;
	mouse->old_delta[1] = my;

	if (m_accel.value) {
		mouse_deltadist = sqrt(mx*mx + my*my);
		mouse_x *= (mouse_deltadist*m_accel.value + sensitivity.value*in_sensitivityscale);
		mouse_y *= (mouse_deltadist*m_accel.value + sensitivity.value*in_sensitivityscale);
	} else {
		mouse_x *= sensitivity.value*in_sensitivityscale;
		mouse_y *= sensitivity.value*in_sensitivityscale;
	}

	if (cl.stats[pnum][STAT_VIEWZOOM])
	{
		mouse_x *= cl.stats[pnum][STAT_VIEWZOOM]/255.0f;
		mouse_y *= cl.stats[pnum][STAT_VIEWZOOM]/255.0f;
	}


	if (!cmd)
	{
		if (mx || my)
		{
			SetCursorPos (window_center_x, window_center_y);
		}
		return;
	}

	if (cl.paused)
		return;

// add mouse X/Y movement to cmd
	if ( (in_strafe.state[pnum] & 1) || (lookstrafe.value && (in_mlook.state[pnum] & 1) ))
		cmd->sidemove += m_side.value * mouse_x;
	else
	{
//		if ((int)((cl.viewangles[pnum][PITCH]+89.99)/180) & 1)
//			mouse_x *= -1;
		cl.viewangles[pnum][YAW] -= m_yaw.value * mouse_x;
	}

	if (in_mlook.state[pnum] & 1)
		V_StopPitchDrift (pnum);
		
	if ( (in_mlook.state[pnum] & 1) && !(in_strafe.state[pnum] & 1))
	{
		cl.viewangles[pnum][PITCH] += m_pitch.value * mouse_y;

		CL_ClampPitch(pnum);
	}
	else
	{
		if ((in_strafe.state[pnum] & 1) && noclip_anglehack)
			cmd->upmove -= m_forward.value * mouse_y;
		else
			cmd->forwardmove -= m_forward.value * mouse_y;
	}

}


/*
===========
IN_MouseMove
===========
*/
void IN_MouseMove (usercmd_t *cmd, int pnum)
{
#ifdef RGLQUAKE
	extern int glwidth, glheight;
#endif
	POINT		current_pos;

	extern int mousecursor_x, mousecursor_y;
	extern int window_x, window_y;

	if (!mouseactive)
	{
		GetCursorPos (&current_pos);
		mousecursor_x = current_pos.x-window_x;
		mousecursor_y = current_pos.y-window_y;

#ifdef RGLQUAKE
		if (qrenderer == QR_OPENGL)	//2d res scaling.
		{
			mousecursor_x *= vid.width/(float)glwidth;
			mousecursor_y *= vid.height/(float)glheight;
		}
#endif

		UI_MousePosition(mousecursor_x, mousecursor_y);

		return;
	}

#ifndef NODIRECTX
	if (dinput)
	{
		DIDEVICEOBJECTDATA	od;
		DWORD				dwElements;
		HRESULT				hr;

		for (;;)
		{
			dwElements = 1;

#if (DIRECTINPUT_VERSION >= DINPUT_VERSION_DX7)
			if (dinput >= DINPUT_VERSION_DX7)
			{
				hr = IDirectInputDevice7_GetDeviceData(g_pMouse7,
						sizeof(DIDEVICEOBJECTDATA), &od, &dwElements, 0);

				if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
				{
					dinput_acquired = true;
					IDirectInputDevice7_Acquire(g_pMouse7);
					break;
				}
			}
			else
#endif
			{
				hr = IDirectInputDevice_GetDeviceData(g_pMouse,
						sizeof(DIDEVICEOBJECTDATA), &od, &dwElements, 0);

				if ((hr == DIERR_INPUTLOST) || (hr == DIERR_NOTACQUIRED))
				{
					dinput_acquired = true;
					IDirectInputDevice_Acquire(g_pMouse);
					break;
				}
			}

			/* Unable to read data or no data available */
			if (FAILED(hr) || dwElements == 0)
			{
				break;
			}

			/* Look at the element to see what happened */

			switch (od.dwOfs)
			{
				case DIMOFS_X:
					sysmouse.delta[0] += od.dwData;
					break;

				case DIMOFS_Y:
					sysmouse.delta[1] += od.dwData;
					break;

				case DIMOFS_Z:
					if (m_forcewheel.value)
					{
						if (od.dwData &	0x80)
							sysmouse.wheeldelta++;
						else
							sysmouse.wheeldelta--;
					}
					break;

				case DIMOFS_BUTTON0:
					if (od.dwData & 0x80)
						sysmouse.buttons |= 1; 
					else 
						sysmouse.buttons &= ~1; 
					break;

				case DIMOFS_BUTTON1:
					if (od.dwData & 0x80)
						sysmouse.buttons |= (1 << 1); 
					else 
						sysmouse.buttons &= ~(1 << 1); 
					break;

				case DIMOFS_BUTTON2:
					if (od.dwData & 0x80)
						sysmouse.buttons |= (1 << 2); 
					else 
						sysmouse.buttons &= ~(1 << 2); 
					break;

				case DIMOFS_BUTTON3:
					if (od.dwData & 0x80)
						sysmouse.buttons |= (1 << 3); 
					else 
						sysmouse.buttons &= ~(1 << 3); 
					break;

#if (DIRECTINPUT_VERSION >= DINPUT_VERSION_DX7)
				case DIMOFS_BUTTON4:
					if (od.dwData & 0x80)
						sysmouse.buttons |= (1 << 4); 
					else 
						sysmouse.buttons &= ~(1 << 4); 
					break;

				case DIMOFS_BUTTON5:
					if (od.dwData & 0x80)
						sysmouse.buttons |= (1 << 5); 
					else 
						sysmouse.buttons &= ~(1 << 5); 
					break;

				case DIMOFS_BUTTON6:
					if (od.dwData & 0x80)
						sysmouse.buttons |= (1 << 6); 
					else 
						sysmouse.buttons &= ~(1 << 6); 
					break;

				case DIMOFS_BUTTON7:
					if (od.dwData & 0x80)
						sysmouse.buttons |= (1 << 7); 
					else 
						sysmouse.buttons &= ~(1 << 7); 
					break;
#endif
			}
		}
	}
	else
#endif
	{
		IN_Accumulate();

		sysmouse.buttons = sysmouse.oldbuttons;	//don't do it!!! Our buttons are event driven. We don't want to merge em and forget do we now?
	}

#ifdef USINGRAWINPUT
	if (rawmicecount)
	{
		if (in_rawinput_combine.value && pnum == 0)
		{
			// not the right way to do this but it'll work for now
			int x;

			for (x = 0; x < rawmicecount; x++)
			{
				ProcessMouse(rawmice + x, cmd, 0);
			}
		}
		else if (pnum < rawmicecount)
		{
			ProcessMouse(rawmice + pnum, cmd, pnum);
		}
	}
#endif

	if (pnum == 0)
		ProcessMouse(&sysmouse,		cmd, pnum);

#ifdef SERIALMOUSE
	if (pnum == 1 || cl.splitclients<2)
		ProcessMouse(&serialmouse,	cmd, pnum);
#endif
}


/*
===========
IN_Move
===========
*/
void IN_Move (usercmd_t *cmd, int pnum)
{

	if (ActiveApp && !Minimized)
	{
		IN_MouseMove (cmd, pnum);
		if (pnum == 1 || cl.splitclients<2)
			IN_JoyMove (cmd, pnum);
	}
}


/*
===========
IN_Accumulate
===========
*/
void IN_Accumulate (void)
{
	if (mouseactive && !dinput)
	{
#ifdef USINGRAWINPUT
		if (rawmicecount)
		{
		}
		else
#endif
		{
			POINT		current_pos;

			GetCursorPos (&current_pos);

			sysmouse.delta[0] += current_pos.x - window_center_x;
			sysmouse.delta[1] += current_pos.y - window_center_y;
		}

	// force the mouse to the center, so there's room to move
		SetCursorPos (window_center_x, window_center_y);
	}
}

#ifdef USINGRAWINPUT
void IN_RawInput_MouseRead(HANDLE in_device_handle)
{
	int i = 0, tbuttons, j;
	int dwSize;

	// get raw input
	if ((*_GRID)((HRAWINPUT)in_device_handle, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER)) == -1) 
	{
		Con_Printf("Raw input: unable to add to get size of raw input header.\n");
		return;
	}

	if (dwSize > ribuffersize)
	{
		ribuffersize = dwSize;
		raw = BZ_Realloc(raw, dwSize);
	}
		
	if ((*_GRID)((HRAWINPUT)in_device_handle, RID_INPUT, raw, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize ) {
		Con_Printf("Raw input: unable to add to get raw input header.\n");
		return;
	} 

	// find mouse in our mouse list
	for (; i < rawmicecount; i++)
	{
		if (rawmice[i].rawinputhandle == raw->header.hDevice)
			break;
	}

	if (i == rawmicecount) // we're not tracking this mouse
		return;

	// movement
	if (raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)
	{
		if (rawmice[i].pos[0] != RI_INVALID_POS)
		{
			rawmice[i].delta[0] += raw->data.mouse.lLastX - rawmice[i].pos[0];
			rawmice[i].delta[1] += raw->data.mouse.lLastY - rawmice[i].pos[1];
		}
		rawmice[i].pos[0] = raw->data.mouse.lLastX;
		rawmice[i].pos[1] = raw->data.mouse.lLastY;
	}
	else // RELATIVE
	{
		rawmice[i].delta[0] += raw->data.mouse.lLastX;
		rawmice[i].delta[1] += raw->data.mouse.lLastY;
		rawmice[i].pos[0] = RI_INVALID_POS;
	}

	// buttons
	if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) 
		Key_Event(K_MOUSE1, true);
	if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP)   
		Key_Event(K_MOUSE1, false);
	if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN) 
		Key_Event(K_MOUSE2, true);
	if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP)   
		Key_Event(K_MOUSE2, false);
	if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN) 
		Key_Event(K_MOUSE3, true);
	if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_3_UP)   
		Key_Event(K_MOUSE3, false);
	if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) 
		Key_Event(K_MOUSE4, true);
	if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP)   
		Key_Event(K_MOUSE4, false);
	if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) 
		Key_Event(K_MOUSE5, true);
	if (raw->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP)   
		Key_Event(K_MOUSE5, false);

	// mouse wheel
	if (raw->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) {      // If the current message has a mouse_wheel message
		if ((SHORT)raw->data.mouse.usButtonData > 0) {
			Key_Event(K_MWHEELUP, true);
			Key_Event(K_MWHEELUP, false);
		}
		if ((SHORT)raw->data.mouse.usButtonData < 0) {
			Key_Event(K_MWHEELDOWN, true);
			Key_Event(K_MWHEELDOWN, false);
		}
	}

	// extra buttons
	tbuttons = raw->data.mouse.ulRawButtons & RI_RAWBUTTON_MASK;
	for (j=6 ; j<rawmice[i].numbuttons ; j++)
	{
		if ( (tbuttons & (1<<j)) && !(rawmice[i].buttons & (1<<j)) )
		{
			Key_Event (K_MOUSE1 + j, true);
		}

		if ( !(tbuttons & (1<<j)) && (rawmice[i].buttons & (1<<j)) )
		{
			Key_Event (K_MOUSE1 + j, false);
		}

	}

	rawmice[i].buttons &= ~RI_RAWBUTTON_MASK;
	rawmice[i].buttons |= tbuttons;
}
#else
void IN_RawInput_MouseRead(HANDLE in_device_handle)
{
}
#endif

/*
===================
IN_ClearStates
===================
*/
void IN_ClearStates (void)
{

	if (mouseactive)
	{
		memset(&sysmouse, 0, sizeof(sysmouse));
		sysmouse.numbuttons = 10;
	}
}


/* 
=============== 
IN_StartupJoystick 
=============== 
*/  
void IN_StartupJoystick (void) 
{ 
	int			numdevs;
	JOYCAPS		jc;
	MMRESULT	mmr;
 
 	// assume no joystick
	joy_avail = false; 

	// abort startup if user requests no joystick
	if ( COM_CheckParm ("-nojoy") ) 
		return; 
 
	// verify joystick driver is present
	if ((numdevs = joyGetNumDevs ()) == 0)
	{
		Con_Printf ("\njoystick not found -- driver not present\n\n");
		return;
	}

	mmr = JOYERR_UNPLUGGED;

	// cycle through the joystick ids for the first valid one
	for (joy_id=0 ; joy_id<numdevs ; joy_id++)
	{
		memset (&ji, 0, sizeof(ji));
		ji.dwSize = sizeof(ji);
		ji.dwFlags = JOY_RETURNCENTERED;

		if ((mmr = joyGetPosEx (joy_id, &ji)) == JOYERR_NOERROR)
			break;
	} 

	// abort startup if we didn't find a valid joystick
	if (mmr != JOYERR_NOERROR)
	{
		Con_Printf ("\njoystick not found -- no valid joysticks (%x)\n\n", mmr);
		return;
	}

	// get the capabilities of the selected joystick
	// abort startup if command fails
	memset (&jc, 0, sizeof(jc));
	if ((mmr = joyGetDevCaps (joy_id, &jc, sizeof(jc))) != JOYERR_NOERROR)
	{
		Con_Printf ("\njoystick not found -- invalid joystick capabilities (%x)\n\n", mmr); 
		return;
	}

	// save the joystick's number of buttons and POV status
	joy_numbuttons = jc.wNumButtons;
	joy_haspov = jc.wCaps & JOYCAPS_HASPOV;

	// old button and POV states default to no buttons pressed
	joy_oldbuttonstate = joy_oldpovstate = 0;

	// mark the joystick as available and advanced initialization not completed
	// this is needed as cvars are not available during initialization

	joy_avail = true; 
	joy_advancedinit = false;

	Con_Printf ("\njoystick detected\n\n"); 
}


/*
===========
RawValuePointer
===========
*/
PDWORD RawValuePointer (int axis)
{
	switch (axis)
	{
	case JOY_AXIS_X:
		return &ji.dwXpos;
	case JOY_AXIS_Y:
		return &ji.dwYpos;
	case JOY_AXIS_Z:
		return &ji.dwZpos;
	case JOY_AXIS_R:
		return &ji.dwRpos;
	case JOY_AXIS_U:
		return &ji.dwUpos;
	case JOY_AXIS_V:
		return &ji.dwVpos;
	}
	return 0;	//compiler shut up.
}


/*
===========
Joy_AdvancedUpdate_f
===========
*/
void Joy_AdvancedUpdate_f (void)
{

	// called once by IN_ReadJoystick and by user whenever an update is needed
	// cvars are now available
	int	i;
	DWORD dwTemp;

	// initialize all the maps
	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		dwAxisMap[i] = AxisNada;
		dwControlMap[i] = JOY_ABSOLUTE_AXIS;
		pdwRawValue[i] = RawValuePointer(i);
	}

	if( joy_advanced.value == 0.0)
	{
		// default joystick initialization
		// 2 axes only with joystick control
		dwAxisMap[JOY_AXIS_X] = AxisTurn;
		// dwControlMap[JOY_AXIS_X] = JOY_ABSOLUTE_AXIS;
		dwAxisMap[JOY_AXIS_Y] = AxisForward;
		// dwControlMap[JOY_AXIS_Y] = JOY_ABSOLUTE_AXIS;
	}
	else
	{
		if (Q_strcmp (joy_name.string, "joystick") != 0)
		{
			// notify user of advanced controller
			Con_Printf ("\n%s configured\n\n", joy_name.string);
		}

		// advanced initialization here
		// data supplied by user via joy_axisn cvars
		dwTemp = (DWORD) joy_advaxisx.value;
		dwAxisMap[JOY_AXIS_X] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_X] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisy.value;
		dwAxisMap[JOY_AXIS_Y] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Y] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisz.value;
		dwAxisMap[JOY_AXIS_Z] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Z] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisr.value;
		dwAxisMap[JOY_AXIS_R] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_R] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisu.value;
		dwAxisMap[JOY_AXIS_U] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_U] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisv.value;
		dwAxisMap[JOY_AXIS_V] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_V] = dwTemp & JOY_RELATIVE_AXIS;
	}

	// compute the axes to collect from DirectInput
	joy_flags = JOY_RETURNCENTERED | JOY_RETURNBUTTONS | JOY_RETURNPOV;
	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		if (dwAxisMap[i] != AxisNada)
		{
			joy_flags |= dwAxisFlags[i];
		}
	}
}


/*
===========
IN_Commands
===========
*/
void IN_Commands (void)
{
	int		i, key_index;
	DWORD	buttonstate, povstate;

	if (!joy_avail)
	{
		return;
	}

	
	// loop through the joystick buttons
	// key a joystick event or auxillary event for higher number buttons for each state change
	buttonstate = ji.dwButtons;
	for (i=0 ; i < joy_numbuttons ; i++)
	{
		if ( (buttonstate & (1<<i)) && !(joy_oldbuttonstate & (1<<i)) )
		{
			key_index = (i < 4) ? K_JOY1 : K_AUX1;
			Key_Event (key_index + i, true);
		}

		if ( !(buttonstate & (1<<i)) && (joy_oldbuttonstate & (1<<i)) )
		{
			key_index = (i < 4) ? K_JOY1 : K_AUX1;
			Key_Event (key_index + i, false);
		}
	}
	joy_oldbuttonstate = buttonstate;

	if (joy_haspov)
	{
		// convert POV information into 4 bits of state information
		// this avoids any potential problems related to moving from one
		// direction to another without going through the center position
		povstate = 0;
		if(ji.dwPOV != JOY_POVCENTERED)
		{
			if (ji.dwPOV == JOY_POVFORWARD)
				povstate |= 0x01;
			if (ji.dwPOV == JOY_POVRIGHT)
				povstate |= 0x02;
			if (ji.dwPOV == JOY_POVBACKWARD)
				povstate |= 0x04;
			if (ji.dwPOV == JOY_POVLEFT)
				povstate |= 0x08;
		}
		// determine which bits have changed and key an auxillary event for each change
		for (i=0 ; i < 4 ; i++)
		{
			if ( (povstate & (1<<i)) && !(joy_oldpovstate & (1<<i)) )
			{
				Key_Event (K_AUX29 + i, true);
			}

			if ( !(povstate & (1<<i)) && (joy_oldpovstate & (1<<i)) )
			{
				Key_Event (K_AUX29 + i, false);
			}
		}
		joy_oldpovstate = povstate;
	}
}


/* 
=============== 
IN_ReadJoystick
=============== 
*/  
qboolean IN_ReadJoystick (void)
{

	memset (&ji, 0, sizeof(ji));
	ji.dwSize = sizeof(ji);
	ji.dwFlags = joy_flags;

	if (joyGetPosEx (joy_id, &ji) == JOYERR_NOERROR)
	{
		// this is a hack -- there is a bug in the Logitech WingMan Warrior DirectInput Driver
		// rather than having 32768 be the zero point, they have the zero point at 32668
		// go figure -- anyway, now we get the full resolution out of the device
		if (joy_wwhack1.value != 0.0)
		{
			ji.dwUpos += 100;
		}
		return true;
	}
	else
	{
		// read error occurred
		// turning off the joystick seems too harsh for 1 read error,
		// but what should be done?
		// Con_Printf ("IN_ReadJoystick: no response\n");
		// joy_avail = false;
		return false;
	}
}


/*
===========
IN_JoyMove
===========
*/
void IN_JoyMove (usercmd_t *cmd, int pnum)
{
	float	speed, aspeed;
	float	fAxisValue, fTemp;
	int		i;

	// complete initialization if first time in
	// this is needed as cvars are not available at initialization time
	if( joy_advancedinit != true )
	{
		Joy_AdvancedUpdate_f();
		joy_advancedinit = true;
	}

	// verify joystick is available and that the user wants to use it
	if (!joy_avail || !in_joystick.value)
	{
		return; 
	}
 
	// collect the joystick data, if possible
	if (IN_ReadJoystick () != true)
	{
		return;
	}

	if (in_speed.state[pnum] & 1)
		speed = cl_movespeedkey.value;
	else
		speed = 1;
	aspeed = speed * host_frametime;

	// loop through the axes
	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		// get the floating point zero-centered, potentially-inverted data for the current axis
		fAxisValue = (float) *pdwRawValue[i];
		// move centerpoint to zero
		fAxisValue -= 32768.0;

		if (joy_wwhack2.value != 0.0)
		{
			if (dwAxisMap[i] == AxisTurn)
			{
				// this is a special formula for the Logitech WingMan Warrior
				// y=ax^b; where a = 300 and b = 1.3
				// also x values are in increments of 800 (so this is factored out)
				// then bounds check result to level out excessively high spin rates
				fTemp = 300.0 * pow(abs(fAxisValue) / 800.0, 1.3);
				if (fTemp > 14000.0)
					fTemp = 14000.0;
				// restore direction information
				fAxisValue = (fAxisValue > 0.0) ? fTemp : -fTemp;
			}
		}

		// convert range from -32768..32767 to -1..1 
		fAxisValue /= 32768.0;

		switch (dwAxisMap[i])
		{
		case AxisForward:
			if ((joy_advanced.value == 0.0) && (in_mlook.state[pnum] & 1))
			{
				// user wants forward control to become look control
				if (fabs(fAxisValue) > joy_pitchthreshold.value)
				{		
					// if mouse invert is on, invert the joystick pitch value
					// only absolute control support here (joy_advanced is false)
					if (m_pitch.value < 0.0)
					{
						cl.viewangles[pnum][PITCH] -= (fAxisValue * joy_pitchsensitivity.value) * aspeed * cl_pitchspeed.value;
					}
					else
					{
						cl.viewangles[pnum][PITCH] += (fAxisValue * joy_pitchsensitivity.value) * aspeed * cl_pitchspeed.value;
					}
					V_StopPitchDrift(pnum);
				}
				else
				{
					// no pitch movement
					// disable pitch return-to-center unless requested by user
					// *** this code can be removed when the lookspring bug is fixed
					// *** the bug always has the lookspring feature on
					if(lookspring.value == 0.0)
						V_StopPitchDrift(pnum);
				}
			}
			else
			{
				// user wants forward control to be forward control
				if (fabs(fAxisValue) > joy_forwardthreshold.value)
				{
					cmd->forwardmove += (fAxisValue * joy_forwardsensitivity.value) * speed * cl_forwardspeed.value;
				}
			}
			break;

		case AxisSide:
			if (fabs(fAxisValue) > joy_sidethreshold.value)
			{
				cmd->sidemove += (fAxisValue * joy_sidesensitivity.value) * speed * cl_sidespeed.value;
			}
			break;

		case AxisTurn:
			if ((in_strafe.state[pnum] & 1) || (lookstrafe.value && (in_mlook.state[pnum] & 1)))
			{
				// user wants turn control to become side control
				if (fabs(fAxisValue) > joy_sidethreshold.value)
				{
					cmd->sidemove -= (fAxisValue * joy_sidesensitivity.value) * speed * cl_sidespeed.value;
				}
			}
			else
			{
				// user wants turn control to be turn control
				if (fabs(fAxisValue) > joy_yawthreshold.value)
				{
					if(dwControlMap[i] == JOY_ABSOLUTE_AXIS)
					{
						cl.viewangles[pnum][YAW] += (fAxisValue * joy_yawsensitivity.value) * aspeed * cl_yawspeed.value;
					}
					else
					{
						cl.viewangles[pnum][YAW] += (fAxisValue * joy_yawsensitivity.value) * speed * 180.0;
					}

				}
			}
			break;

		case AxisLook:
			if (in_mlook.state[pnum] & 1)
			{
				if (fabs(fAxisValue) > joy_pitchthreshold.value)
				{
					// pitch movement detected and pitch movement desired by user
					if(dwControlMap[i] == JOY_ABSOLUTE_AXIS)
					{
						cl.viewangles[pnum][PITCH] += (fAxisValue * joy_pitchsensitivity.value) * aspeed * cl_pitchspeed.value;
					}
					else
					{
						cl.viewangles[pnum][PITCH] += (fAxisValue * joy_pitchsensitivity.value) * speed * 180.0;
					}
					V_StopPitchDrift(pnum);
				}
				else
				{
					// no pitch movement
					// disable pitch return-to-center unless requested by user
					// *** this code can be removed when the lookspring bug is fixed
					// *** the bug always has the lookspring feature on
					if(lookspring.value == 0.0)
						V_StopPitchDrift(pnum);
				}
			}
			break;

		default:
			break;
		}
	}

	CL_ClampPitch(pnum);
}





qbyte        scantokey[128] = 
					{ 
//  0           1       2       3       4       5       6       7 
//  8           9       A       B       C       D       E       F 
	0  ,		27,		'1',		'2',		'3',	'4',		'5',			'6', 
	'7',		'8',	'9',		'0',		'-',	'=',		K_BACKSPACE,	9,			// 0 
	'q',		'w',	'e',		'r',		't',	'y',		'u',			'i', 
	'o',		'p',	'[',		']',		13 ,	K_CTRL,		'a',			's',		// 1 
	'd',		'f',	'g',		'h',		'j',	'k',		'l',			';', 
	'\'',		'`',	K_SHIFT,	'\\',		'z',	'x',		'c',			'v',		// 2 
	'b',		'n',	'm',		',',		'.',	'/',		K_SHIFT,		'*', 
	K_ALT,		' ',	K_CAPSLOCK,	K_F1,		K_F2,	K_F3,		K_F4,			K_F5,		// 3 
	K_F6,		K_F7,	K_F8,		K_F9,		K_F10,	K_PAUSE,	K_SCRLCK,		K_HOME, 
	K_UPARROW,	K_PGUP,	'-',		K_LEFTARROW,'5',	K_RIGHTARROW,'+',			K_END,		// 4 
	K_DOWNARROW,K_PGDN,	K_INS,		K_DEL,		0,      0,			0,				K_F11, 
	K_F12,		0,		0,			0,			0,		0,			0,				0,			// 5 
	0,			0,		0,			0,			0,		0,			0,				0, 
	0,			0,		0,			0,			0,		0,			0,				0,			// 6 
	0,			0,		0,			0,			0,		0,			0,				0, 
	0,			0,		0,			0,			0,		0,			0,				0			// 7 
					}; 

qbyte        shiftscantokey[128] = 
					{ 
//  0           1       2       3       4       5       6       7 
//  8           9       A       B       C       D       E       F 
	0  ,    27,     '!',    '@',    '#',    '$',    '%',    '^', 
	'&',    '*',    '(',    ')',    '_',    '+',    K_BACKSPACE, 9, // 0 
	'Q',    'W',    'E',    'R',    'T',    'Y',    'U',    'I', 
	'O',    'P',    '{',    '}',    13 ,    K_CTRL,'A',  'S',      // 1 
	'D',    'F',    'G',    'H',    'J',    'K',    'L',    ':', 
	'"' ,    '~',    K_SHIFT,'|',  'Z',    'X',    'C',    'V',      // 2 
	'B',    'N',    'M',    '<',    '>',    '?',    K_SHIFT,'*', 
	K_ALT,' ',   K_CAPSLOCK  ,    K_F1, K_F2, K_F3, K_F4, K_F5,   // 3 
	K_F6, K_F7, K_F8, K_F9, K_F10, K_PAUSE  ,    K_SCRLCK  , K_HOME, 
	K_UPARROW,K_PGUP,'_',K_LEFTARROW,'%',K_RIGHTARROW,'+',K_END, //4 
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             0,              K_F11, 
	K_F12,  0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 5 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0,        // 6 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0, 
	0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0  ,    0         // 7 
					}; 


/*
=======
MapKey

Map from windows to quake keynums
=======
*/
int MapKey (int vkey)
{
	int key;
	key = (vkey>>16)&255;

	if (cl_keypad.value)
	{
		switch (key)
		{
		case 0x1c:
			if ((vkey>>24)&1)	//not compleatly seperate
				return K_KP_ENTER;
			break;
		case 0x47:
			if (!((vkey>>24)&1))	//not compleatly seperate
				return K_KP_HOME;
			break;
		case 0x48:
			if (!((vkey>>24)&1))	//not compleatly seperate
				return K_KP_UPARROW;
			break;
		case 0x49:
			if (!((vkey>>24)&1))	//not compleatly seperate
				return K_KP_PGUP;
			break;
		case 0x4b:
			if (!((vkey>>24)&1))	//not compleatly seperate
				return K_KP_LEFTARROW;
			break;
		case 0x4c:
			return K_KP_5;
		case 0x4d:
			if (!((vkey>>24)&1))	//not compleatly seperate
				return K_KP_RIGHTARROW;
			break;
		case 0x4f:
			if (!((vkey>>24)&1))	//not compleatly seperate
				return K_KP_END;
			break;
		case 0x50:
			if (!((vkey>>24)&1))	//not compleatly seperate
				return K_KP_DOWNARROW;
			break;
		case 0x51:
			if (!((vkey>>24)&1))	//not compleatly seperate
				return K_KP_PGDN;
			break;
		case 0x52:
			if (!((vkey>>24)&1))	//not compleatly seperate
				return K_KP_INS;
			break;
		case 0x53:
			if (!((vkey>>24)&1))	//not compleatly seperate
				return K_KP_DEL;
			break;
		case 0x35:
			if ((vkey>>24)&1)	//not compleatly seperate
				return K_KP_SLASH;
			break;
		case 0x4a:
			return K_KP_MINUS;
		case 0x4e:
			return K_KP_PLUS;
		case 0x45:
			if ((vkey>>24)&1)	//not compleatly seperate
				return K_KP_NUMLOCK;
			break;
		case 0x37:
			return K_KP_STAR;
//		case 0x
//			return K_KP_EQUALS;
		}
	}
	if (key > 127)
		return 0;
	if (scantokey[key] == 0)
		Con_DPrintf("key 0x%02x has no translation\n", key);
	return scantokey[key];
}
