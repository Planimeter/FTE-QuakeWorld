#include "quakedef.h"

#include <SDL.h>

#if SDL_MAJOR_VERSION >=2
SDL_Window *sdlwindow;
#else
extern SDL_Surface *sdlsurf;
#endif

qboolean ActiveApp;
qboolean mouseactive;
extern qboolean mouseusedforgui;
extern qboolean vid_isfullscreen;

#if SDL_MAJOR_VERSION > 1 || (SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION >= 3)
#define HAVE_SDL_TEXTINPUT
#endif

void IN_ActivateMouse(void)
{
	if (mouseactive)
		return;

	mouseactive = true;
	SDL_ShowCursor(0);

#if SDL_MAJOR_VERSION >= 2
	SDL_SetRelativeMouseMode(true);
	SDL_SetWindowGrab(sdlwindow, true);
#else
	SDL_WM_GrabInput(SDL_GRAB_ON);
#endif
}

void IN_DeactivateMouse(void)
{
	if (!mouseactive)
		return;

	mouseactive = false;
	SDL_ShowCursor(1);
#if SDL_MAJOR_VERSION >= 2
	SDL_SetRelativeMouseMode(false);
	SDL_SetWindowGrab(sdlwindow, false);
#else
	SDL_WM_GrabInput(SDL_GRAB_OFF);
#endif
}

#if SDL_MAJOR_VERSION >= 2
#define MAX_JOYSTICKS 4
static struct sdljoy_s
{
	//fte doesn't distinguish between joysticks and controllers.
	//in sdl, controllers are some glorified version of joysticks apparently.
	char *devname;
	SDL_Joystick *joystick;
	SDL_GameController *controller;
	SDL_JoystickID id;
} sdljoy[MAX_JOYSTICKS]; 
//the enumid is the value for the open function rather than the working id.
static void J_ControllerAdded(int enumid)
{
	const char *cname;
	int i;
	for (i = 0; i < MAX_JOYSTICKS; i++)
		if (sdljoy[i].controller == NULL)
			break;
	if (i == MAX_JOYSTICKS)
		return;

	sdljoy[i].controller = SDL_GameControllerOpen(enumid);
	if (!sdljoy[i].controller)
		return;
	sdljoy[i].joystick = SDL_GameControllerGetJoystick(sdljoy[i].controller);
	sdljoy[i].id = SDL_JoystickInstanceID(sdljoy[i].joystick);

	cname = SDL_GameControllerName(sdljoy[i].controller);
	if (!cname)
		cname = "Unknown Controller";
	Con_Printf("Found new controller (%i): %s\n", i, cname);
	sdljoy[i].devname = Z_StrDup(cname);
}
static void J_JoystickAdded(int enumid)
{
	const char *cname;
	int i;
	for (i = 0; i < MAX_JOYSTICKS; i++)
		if (sdljoy[i].joystick == NULL)
			break;
	if (i == MAX_JOYSTICKS)
		return;

	sdljoy[i].joystick = SDL_JoystickOpen(enumid);
	if (!sdljoy[i].joystick)
		return;
	sdljoy[i].id = SDL_JoystickInstanceID(sdljoy[i].joystick);

	cname = SDL_JoystickName(sdljoy[i].joystick);
	if (!cname)
		cname = "Unknown Joystick";
	Con_Printf("Found new joystick (%i): %s\n", i, cname);
}
static struct sdljoy_s *J_DevId(int jid)
{
	int i;
	for (i = 0; i < MAX_JOYSTICKS; i++)
		if (sdljoy[i].joystick && sdljoy[i].id == jid)
			return &sdljoy[i];
	return NULL;
}
static void J_ControllerAxis(int jid, int axis, int value)
{
	int axismap[] = {0,1,3,4,2,5};

	struct sdljoy_s *joy = J_DevId(jid);
	if (joy && axis < sizeof(axismap)/sizeof(axismap[0]))
		IN_JoystickAxisEvent(joy - sdljoy, axismap[axis], value / 32767.0);
}
static void J_JoystickAxis(int jid, int axis, int value)
{
	int axismap[] = {0,1,3,4,2,5};

	struct sdljoy_s *joy = J_DevId(jid);
	if (joy && axis < sizeof(axismap)/sizeof(axismap[0]))
		IN_JoystickAxisEvent(joy - sdljoy, axismap[axis], value / 32767.0);
}
//we don't do hats and balls and stuff.
static void J_ControllerButton(int jid, int button, qboolean pressed)
{
	//controllers have reliable button maps.
	//but that doesn't meant that fte has specific k_ names for those buttons, but the mapping should be reliable, at least until they get mapped to proper k_ values.
	int buttonmap[] = {
#if 0
		//NOTE: DP has specific 'X360' buttons for many of these. of course, its not an exact mapping...
		K_X360_A,				/*SDL_CONTROLLER_BUTTON_A*/
		K_X360_B,				/*SDL_CONTROLLER_BUTTON_B*/
		K_X360_X,				/*SDL_CONTROLLER_BUTTON_X*/
		K_X360_Y,				/*SDL_CONTROLLER_BUTTON_Y*/
		K_X360_BACK,			/*SDL_CONTROLLER_BUTTON_BACK*/
		K_AUX2,					/*SDL_CONTROLLER_BUTTON_GUIDE*/
		K_X360_START,			/*SDL_CONTROLLER_BUTTON_START*/
		K_X360_LEFT_THUMB,		/*SDL_CONTROLLER_BUTTON_LEFTSTICK*/
		K_X360_RIGHT_THUMB,		/*SDL_CONTROLLER_BUTTON_RIGHTSTICK*/
		K_X360_LEFT_SHOULDER,	/*SDL_CONTROLLER_BUTTON_LEFTSHOULDER*/
		K_X360_RIGHT_SHOULDER,	/*SDL_CONTROLLER_BUTTON_RIGHTSHOULDER*/
		K_X360_DPAD_UP,			/*SDL_CONTROLLER_BUTTON_DPAD_UP*/
		K_X360_DPAD_DOWN,		/*SDL_CONTROLLER_BUTTON_DPAD_DOWN*/
		K_X360_DPAD_LEFT,		/*SDL_CONTROLLER_BUTTON_DPAD_LEFT*/
		K_X360_DPAD_RIGHT		/*SDL_CONTROLLER_BUTTON_DPAD_RIGHT*/
#else
		K_JOY1,		/*SDL_CONTROLLER_BUTTON_A*/
		K_JOY2,		/*SDL_CONTROLLER_BUTTON_B*/
		K_JOY3,		/*SDL_CONTROLLER_BUTTON_X*/
		K_JOY4,		/*SDL_CONTROLLER_BUTTON_Y*/
		K_AUX1,		/*SDL_CONTROLLER_BUTTON_BACK*/
		K_AUX2,		/*SDL_CONTROLLER_BUTTON_GUIDE*/
		K_AUX3,		/*SDL_CONTROLLER_BUTTON_START*/
		K_AUX4,		/*SDL_CONTROLLER_BUTTON_LEFTSTICK*/
		K_AUX5,		/*SDL_CONTROLLER_BUTTON_RIGHTSTICK*/
		K_AUX6,		/*SDL_CONTROLLER_BUTTON_LEFTSHOULDER*/
		K_AUX7,		/*SDL_CONTROLLER_BUTTON_RIGHTSHOULDER*/
		K_AUX8,		/*SDL_CONTROLLER_BUTTON_DPAD_UP*/
		K_AUX9,		/*SDL_CONTROLLER_BUTTON_DPAD_DOWN*/
		K_AUX10,	/*SDL_CONTROLLER_BUTTON_DPAD_LEFT*/
		K_AUX11		/*SDL_CONTROLLER_BUTTON_DPAD_RIGHT*/
#endif
	};

	struct sdljoy_s *joy = J_DevId(jid);
	if (joy && button < sizeof(buttonmap)/sizeof(buttonmap[0]))
		IN_KeyEvent(joy - sdljoy, pressed, buttonmap[button], 0);
}
static void J_JoystickButton(int jid, int button, qboolean pressed)
{
	//generic joysticks have no specific mappings. they're really random like that.
	int buttonmap[] = {
		K_JOY1,
		K_JOY2,
		K_JOY3,
		K_JOY4,
		K_AUX1,
		K_AUX2,
		K_AUX3,
		K_AUX4,
		K_AUX5,
		K_AUX6,
		K_AUX7,
		K_AUX8,
		K_AUX9,
		K_AUX10,
		K_AUX11,
		K_AUX12,
		K_AUX13,
		K_AUX14,
		K_AUX15,
		K_AUX16,
		K_AUX17,
		K_AUX18,
		K_AUX19,
		K_AUX20,
		K_AUX21,
		K_AUX22,
		K_AUX23,
		K_AUX24,
		K_AUX25,
		K_AUX26,
		K_AUX27,
		K_AUX28,
		K_AUX29,
		K_AUX30,
		K_AUX31,
		K_AUX32
	};

	struct sdljoy_s *joy = J_DevId(jid);
	if (joy && button < sizeof(buttonmap)/sizeof(buttonmap[0]))
		IN_KeyEvent(joy - sdljoy, pressed, buttonmap[button], 0);
}
static void J_Kill(int jid, qboolean verbose)
{
	int i;
	struct sdljoy_s *joy = J_DevId(jid);

	if (!joy)
		return;

	//make sure all the axis are nulled out, to avoid surprises.
	for (i = 0; i < 6; i++)
		IN_JoystickAxisEvent(joy - sdljoy, i, 0);

	if (joy->controller)
	{
		for (i = 0; i < 32; i++)
			J_ControllerButton(jid, i, false);
		Con_Printf("Controller unplugged(%i): %s\n", (int)(joy - sdljoy), joy->devname);
		SDL_GameControllerClose(joy->controller);
	}
	else
	{
		for (i = 0; i < 32; i++)
			J_JoystickButton(jid, i, false);
		Con_Printf("Joystick unplugged(%i): %s\n", (int)(joy - sdljoy), joy->devname);
		SDL_JoystickClose(joy->joystick);
	}
	joy->controller = NULL;
	joy->joystick = NULL;
	Z_Free(joy->devname);
	joy->devname = NULL;
}
static void J_KillAll(void)
{
	int i;
	for (i = 0; i < MAX_JOYSTICKS; i++)
		J_Kill(sdljoy[i].id, false);
}
#endif

#if SDL_MAJOR_VERSION >= 2
unsigned int MySDL_MapKey(unsigned int sdlkey)
{
	switch(sdlkey)
	{
	default:				return 0;
	//any ascii chars can be mapped directly to keys, even if they're only ever accessed with shift etc... oh well.
	case SDLK_RETURN:		return K_ENTER;
	case SDLK_ESCAPE:		return K_ESCAPE;
	case SDLK_BACKSPACE:	return K_BACKSPACE;
	case SDLK_TAB:			return K_TAB;
	case SDLK_SPACE:		return K_SPACE;
	case SDLK_EXCLAIM:
	case SDLK_QUOTEDBL:
	case SDLK_HASH:
	case SDLK_PERCENT:
	case SDLK_DOLLAR:
	case SDLK_AMPERSAND:
	case SDLK_QUOTE:
	case SDLK_LEFTPAREN:
	case SDLK_RIGHTPAREN:
	case SDLK_ASTERISK:
	case SDLK_PLUS:
	case SDLK_COMMA:
	case SDLK_MINUS:
	case SDLK_PERIOD:
	case SDLK_SLASH:
	case SDLK_0:
	case SDLK_1:
	case SDLK_2:
	case SDLK_3:
	case SDLK_4:
	case SDLK_5:
	case SDLK_6:
	case SDLK_7:
	case SDLK_8:
	case SDLK_9:
	case SDLK_COLON:
	case SDLK_SEMICOLON:
	case SDLK_LESS:
	case SDLK_EQUALS:
	case SDLK_GREATER:
	case SDLK_QUESTION:
	case SDLK_AT:
	case SDLK_LEFTBRACKET:
	case SDLK_BACKSLASH:
	case SDLK_RIGHTBRACKET:
	case SDLK_CARET:
	case SDLK_UNDERSCORE:
	case SDLK_BACKQUOTE:
	case SDLK_a:
	case SDLK_b:
	case SDLK_c:
	case SDLK_d:
	case SDLK_e:
	case SDLK_f:
	case SDLK_g:
	case SDLK_h:
	case SDLK_i:
	case SDLK_j:
	case SDLK_k:
	case SDLK_l:
	case SDLK_m:
	case SDLK_n:
	case SDLK_o:
	case SDLK_p:
	case SDLK_q:
	case SDLK_r:
	case SDLK_s:
	case SDLK_t:
	case SDLK_u:
	case SDLK_v:
	case SDLK_w:
	case SDLK_x:
	case SDLK_y:
	case SDLK_z:
							return sdlkey;
	case SDLK_CAPSLOCK:		return K_CAPSLOCK;
	case SDLK_F1:			return K_F1;
	case SDLK_F2:			return K_F2;
	case SDLK_F3:			return K_F3;
	case SDLK_F4:			return K_F4;
	case SDLK_F5:			return K_F5;
	case SDLK_F6:			return K_F6;
	case SDLK_F7:			return K_F7;
	case SDLK_F8:			return K_F8;
	case SDLK_F9: 			return K_F9;
	case SDLK_F10:			return K_F10;
	case SDLK_F11:			return K_F11;
	case SDLK_F12:			return K_F12;
	case SDLK_PRINTSCREEN:	return K_PRINTSCREEN;
	case SDLK_SCROLLLOCK:	return K_SCRLCK;
	case SDLK_PAUSE:		return K_PAUSE;
	case SDLK_INSERT:		return K_INS;
	case SDLK_HOME:			return K_HOME;
	case SDLK_PAGEUP:		return K_PGUP;
	case SDLK_DELETE:		return K_DEL;
	case SDLK_END:			return K_END;
	case SDLK_PAGEDOWN:		return K_PGDN;
	case SDLK_RIGHT:		return K_RIGHTARROW;
	case SDLK_LEFT:			return K_LEFTARROW;
	case SDLK_DOWN:			return K_DOWNARROW;
	case SDLK_UP:			return K_UPARROW;
	case SDLK_NUMLOCKCLEAR:	return K_KP_NUMLOCK;
	case SDLK_KP_DIVIDE:	return K_KP_SLASH;
	case SDLK_KP_MULTIPLY:	return K_KP_STAR;
	case SDLK_KP_MINUS:		return K_KP_MINUS;
	case SDLK_KP_PLUS:		return K_KP_PLUS;
	case SDLK_KP_ENTER:		return K_KP_ENTER;
	case SDLK_KP_1:			return K_KP_END;
	case SDLK_KP_2:			return K_KP_DOWNARROW;
	case SDLK_KP_3:			return K_KP_PGDN;
	case SDLK_KP_4:			return K_KP_LEFTARROW;
	case SDLK_KP_5:			return K_KP_5;
	case SDLK_KP_6:			return K_KP_RIGHTARROW;
	case SDLK_KP_7:			return K_KP_HOME;
	case SDLK_KP_8:			return K_KP_UPARROW;
	case SDLK_KP_9:			return K_KP_PGDN;
	case SDLK_KP_0:			return K_KP_INS;
	case SDLK_KP_PERIOD:	return K_KP_DEL;
	case SDLK_APPLICATION:	return K_APP;
	case SDLK_POWER:		return K_POWER;
	case SDLK_KP_EQUALS:	return K_KP_EQUALS;
	case SDLK_F13:			return K_F13;
	case SDLK_F14:			return K_F14;
	case SDLK_F15:			return K_F15;
/*
	case SDLK_F16:			return K_;
	case SDLK_F17:			return K_;
	case SDLK_F18:			return K_;
	case SDLK_F19:			return K_;
	case SDLK_F20:			return K_;
	case SDLK_F21:			return K_;
	case SDLK_F22:			return K_;
	case SDLK_F23:			return K_;
	case SDLK_F24:			return K_;
	case SDLK_EXECUTE:		return K_;
	case SDLK_HELP:			return K_;
	case SDLK_MENU:			return K_;
	case SDLK_SELECT:		return K_;
	case SDLK_STOP:			return K_;
	case SDLK_AGAIN:		return K_;
	case SDLK_UNDO:			return K_;
	case SDLK_CUT:			return K_;
	case SDLK_COPY:			return K_;
	case SDLK_PASTE:		return K_;
	case SDLK_FIND:			return K_;
	case SDLK_MUTE:			return K_;
*/
	case SDLK_VOLUMEUP:		return K_VOLUP;
	case SDLK_VOLUMEDOWN:	return K_VOLDOWN;
/*
	case SDLK_KP_COMMA:		return K_;
	case SDLK_KP_EQUALSAS400:	return K_;
	case SDLK_ALTERASE:		return K_;
	case SDLK_SYSREQ:		return K_;
	case SDLK_CANCEL:		return K_;
	case SDLK_CLEAR:		return K_;
	case SDLK_PRIOR:		return K_;
	case SDLK_RETURN2:		return K_;
	case SDLK_SEPARATOR:	return K_;
	case SDLK_OUT:			return K_;
	case SDLK_OPER:			return K_;
	case SDLK_CLEARAGAIN:	return K_;
	case SDLK_CRSEL:		return K_;
	case SDLK_EXSEL:		return K_;
	case SDLK_KP_00:		return K_;
	case SDLK_KP_000:		return K_;
	case SDLK_THOUSANDSSEPARATOR:	return K_;
	case SDLK_DECIMALSEPARATOR:		return K_;
	case SDLK_CURRENCYUNIT:			return K_;
	case SDLK_CURRENCYSUBUNIT:		return K_;
	case SDLK_KP_LEFTPAREN:			return K_;
	case SDLK_KP_RIGHTPAREN:		return K_;
	case SDLK_KP_LEFTBRACE:			return K_;
	case SDLK_KP_RIGHTBRACE:		return K_;
	case SDLK_KP_TAB:		return K_;
	case SDLK_KP_BACKSPACE:	return K_;
	case SDLK_KP_A:			return K_;
	case SDLK_KP_B:			return K_;
	case SDLK_KP_C:			return K_;
	case SDLK_KP_D:			return K_;
	case SDLK_KP_E:			return K_;
	case SDLK_KP_F:			return K_;
	case SDLK_KP_XOR:		return K_;
	case SDLK_KP_POWER:		return K_;
	case SDLK_KP_PERCENT:	return K_;
	case SDLK_KP_LESS:		return K_;
	case SDLK_KP_GREATER:	return K_;
	case SDLK_KP_AMPERSAND: return K_;
	case SDLK_KP_DBLAMPERSAND:		return K_;
	case SDLK_KP_VERTICALBAR:		return K_;
	case SDLK_KP_DBLVERTICALBAR:	return K_;
	case SDLK_KP_COLON:		return K_;
	case SDLK_KP_HASH:		return K_;
	case SDLK_KP_SPACE:		return K_;
	case SDLK_KP_AT:		return K_;
	case SDLK_KP_EXCLAM:	return K_;
	case SDLK_KP_MEMSTORE:	return K_;
	case SDLK_KP_MEMRECALL:	return K_;
	case SDLK_KP_MEMCLEAR:	return K_;
	case SDLK_KP_MEMADD:	return K_;
	case SDLK_KP_MEMSUBTRACT:	return K_;
	case SDLK_KP_MEMMULTIPLY:	return K_;
	case SDLK_KP_MEMDIVIDE:		return K_;
	case SDLK_KP_PLUSMINUS:		return K_;
	case SDLK_KP_CLEAR:			return K_;
	case SDLK_KP_CLEARENTRY:	return K_;
	case SDLK_KP_BINARY:		return K_;
	case SDLK_KP_OCTAL:			return K_;
	case SDLK_KP_DECIMAL:		return K_;
	case SDLK_KP_HEXADECIMAL:	return K_;
*/
	case SDLK_LCTRL:		return K_LCTRL;
	case SDLK_LSHIFT:		return K_LSHIFT;
	case SDLK_LALT:			return K_LALT;
	case SDLK_LGUI:			return K_APP;
	case SDLK_RCTRL:		return K_RCTRL;
	case SDLK_RSHIFT:		return K_RSHIFT;
	case SDLK_RALT:			return K_RALT;
/*
	case SDLK_RGUI:			return K_;
	case SDLK_MODE:			return K_;
	case SDLK_AUDIONEXT:	return K_;
	case SDLK_AUDIOPREV:	return K_;
	case SDLK_AUDIOSTOP:	return K_;
	case SDLK_AUDIOPLAY:	return K_;
	case SDLK_AUDIOMUTE:	return K_;
	case SDLK_MEDIASELECT:	return K_;
	case SDLK_WWW:			return K_;
	case SDLK_MAIL:			return K_;
	case SDLK_CALCULATOR:	return K_;
	case SDLK_COMPUTER:		return K_;
	case SDLK_AC_SEARCH:	return K_;
	case SDLK_AC_HOME:		return K_;
	case SDLK_AC_BACK:		return K_;
	case SDLK_AC_FORWARD:	return K_;
	case SDLK_AC_STOP:		return K_;
	case SDLK_AC_REFRESH:	return K_;
	case SDLK_AC_BOOKMARKS:	return K_;
	case SDLK_BRIGHTNESSDOWN:	return K_;
	case SDLK_BRIGHTNESSUP:		return K_;
	case SDLK_DISPLAYSWITCH:	return K_;
	case SDLK_KBDILLUMTOGGLE:	return K_;
	case SDLK_KBDILLUMDOWN:		return K_;
	case SDLK_KBDILLUMUP:		return K_;
	case SDLK_EJECT:			return K_;
	case SDLK_SLEEP:			return K_;
*/
	}
}
#else
#define tenoh	0,0,0,0,0, 0,0,0,0,0
#define fiftyoh tenoh, tenoh, tenoh, tenoh, tenoh
#define hundredoh fiftyoh, fiftyoh
static unsigned int tbl_sdltoquake[] =
{
	0,0,0,0,		//SDLK_UNKNOWN		= 0,
	0,0,0,0,		//SDLK_FIRST		= 0,
	K_BACKSPACE,	//SDLK_BACKSPACE	= 8,
	K_TAB,			//SDLK_TAB			= 9,
	0,0,
	0,				//SDLK_CLEAR		= 12,
	K_ENTER,		//SDLK_RETURN		= 13,
    0,0,0,0,0,
	K_PAUSE,		//SDLK_PAUSE		= 19,
	0,0,0,0,0,0,0,
	K_ESCAPE,		//SDLK_ESCAPE		= 27,
	0,0,0,0,
	K_SPACE,		//SDLK_SPACE		= 32,
	'!',			//SDLK_EXCLAIM		= 33,
	'"',			//SDLK_QUOTEDBL		= 34,
	'#',			//SDLK_HASH			= 35,
	'$',			//SDLK_DOLLAR		= 36,
	0,
	'&',			//SDLK_AMPERSAND	= 38,
	'\'',			//SDLK_QUOTE		= 39,
	'(',			//SDLK_LEFTPAREN	= 40,
	')',			//SDLK_RIGHTPAREN	= 41,
	'*',			//SDLK_ASTERISK		= 42,
	'+',			//SDLK_PLUS			= 43,
	',',			//SDLK_COMMA		= 44,
	'-',			//SDLK_MINUS		= 45,
	'.',			//SDLK_PERIOD		= 46,
	'/',			//SDLK_SLASH		= 47,
	'0',			//SDLK_0			= 48,
	'1',			//SDLK_1			= 49,
	'2',			//SDLK_2			= 50,
	'3',			//SDLK_3			= 51,
	'4',			//SDLK_4			= 52,
	'5',			//SDLK_5			= 53,
	'6',			//SDLK_6			= 54,
	'7',			//SDLK_7			= 55,
	'8',			//SDLK_8			= 56,
	'9',			//SDLK_9			= 57,
	':',			//SDLK_COLON		= 58,
	';',			//SDLK_SEMICOLON	= 59,
	'<',			//SDLK_LESS			= 60,
	'=',			//SDLK_EQUALS		= 61,
	'>',			//SDLK_GREATER		= 62,
	'?',			//SDLK_QUESTION		= 63,
	'@',			//SDLK_AT			= 64,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	'[',		//SDLK_LEFTBRACKET	= 91,
	'\\',		//SDLK_BACKSLASH	= 92,
	']',		//SDLK_RIGHTBRACKET	= 93,
	'^',		//SDLK_CARET		= 94,
	'_',		//SDLK_UNDERSCORE	= 95,
	'`',		//SDLK_BACKQUOTE	= 96,
	'a',		//SDLK_a			= 97,
	'b',		//SDLK_b			= 98,
	'c',		//SDLK_c			= 99,
	'd',		//SDLK_d			= 100,
	'e',		//SDLK_e			= 101,
	'f',		//SDLK_f			= 102,
	'g',		//SDLK_g			= 103,
	'h',		//SDLK_h			= 104,
	'i',		//SDLK_i			= 105,
	'j',		//SDLK_j			= 106,
	'k',		//SDLK_k			= 107,
	'l',		//SDLK_l			= 108,
	'm',		//SDLK_m			= 109,
	'n',		//SDLK_n			= 110,
	'o',		//SDLK_o			= 111,
	'p',		//SDLK_p			= 112,
	'q',		//SDLK_q			= 113,
	'r',		//SDLK_r			= 114,
	's',		//SDLK_s			= 115,
	't',		//SDLK_t			= 116,
	'u',		//SDLK_u			= 117,
	'v',		//SDLK_v			= 118,
	'w',		//SDLK_w			= 119,
	'x',		//SDLK_x			= 120,
	'y',		//SDLK_y			= 121,
	'z',		//SDLK_z			= 122,
	0,0,0,0,
	K_DEL, 		//SDLK_DELETE		= 127,
	hundredoh /*227*/, tenoh, tenoh, 0,0,0,0,0,0,0,0,
	K_KP_INS,		//SDLK_KP0		= 256,
	K_KP_END,		//SDLK_KP1		= 257,
	K_KP_DOWNARROW,		//SDLK_KP2		= 258,
	K_KP_PGDN,		//SDLK_KP3		= 259,
	K_KP_LEFTARROW,		//SDLK_KP4		= 260,
	K_KP_5,		//SDLK_KP5		= 261,
	K_KP_RIGHTARROW,		//SDLK_KP6		= 262,
	K_KP_HOME,		//SDLK_KP7		= 263,
	K_KP_UPARROW,		//SDLK_KP8		= 264,
	K_KP_PGUP,		//SDLK_KP9		= 265,
	K_KP_DEL,//SDLK_KP_PERIOD	= 266,
	K_KP_SLASH,//SDLK_KP_DIVIDE	= 267,
	K_KP_STAR,//SDLK_KP_MULTIPLY= 268,
	K_KP_MINUS,	//SDLK_KP_MINUS		= 269,
	K_KP_PLUS,	//SDLK_KP_PLUS		= 270,
	K_KP_ENTER,	//SDLK_KP_ENTER		= 271,
	K_KP_EQUALS,//SDLK_KP_EQUALS	= 272,
	K_UPARROW,	//SDLK_UP		= 273,
	K_DOWNARROW,//SDLK_DOWN		= 274,
	K_RIGHTARROW,//SDLK_RIGHT	= 275,
	K_LEFTARROW,//SDLK_LEFT		= 276,
	K_INS,		//SDLK_INSERT	= 277,
	K_HOME,		//SDLK_HOME		= 278,
	K_END,		//SDLK_END		= 279,
	K_PGUP, 	//SDLK_PAGEUP	= 280,
	K_PGDN,		//SDLK_PAGEDOWN	= 281,
	K_F1,		//SDLK_F1		= 282,
	K_F2,		//SDLK_F2		= 283,
	K_F3,		//SDLK_F3		= 284,
	K_F4,		//SDLK_F4		= 285,
	K_F5,		//SDLK_F5		= 286,
	K_F6,		//SDLK_F6		= 287,
	K_F7,		//SDLK_F7		= 288,
	K_F8,		//SDLK_F8		= 289,
	K_F9,		//SDLK_F9		= 290,
	K_F10,		//SDLK_F10		= 291,
	K_F11,		//SDLK_F11		= 292,
	K_F12,		//SDLK_F12		= 293,
	0,			//SDLK_F13		= 294,
	0,			//SDLK_F14		= 295,
	0,			//SDLK_F15		= 296,
	0,0,0,
	0,//K_NUMLOCK,	//SDLK_NUMLOCK	= 300,
	K_CAPSLOCK,	//SDLK_CAPSLOCK	= 301,
	0,//K_SCROLLOCK,//SDLK_SCROLLOCK= 302,
	K_SHIFT,	//SDLK_RSHIFT	= 303,
	K_SHIFT,	//SDLK_LSHIFT	= 304,
	K_CTRL,		//SDLK_RCTRL	= 305,
	K_CTRL,		//SDLK_LCTRL	= 306,
	K_RALT,		//SDLK_RALT		= 307,
	K_LALT,		//SDLK_LALT		= 308,
	0,			//SDLK_RMETA	= 309,
	0,			//SDLK_LMETA	= 310,
	0,			//SDLK_LSUPER	= 311,		/* Left "Windows" key */
	0,			//SDLK_RSUPER	= 312,		/* Right "Windows" key */
	0,			//SDLK_MODE		= 313,		/* "Alt Gr" key */
	0,			//SDLK_COMPOSE	= 314,		/* Multi-key compose key */
	0,			//SDLK_HELP		= 315,
	0,			//SDLK_PRINT	= 316,
	0,			//SDLK_SYSREQ	= 317,
	K_PAUSE,	//SDLK_BREAK	= 318,
	0,			//SDLK_MENU		= 319,
	0,			//SDLK_POWER	= 320,		/* Power Macintosh power key */
	'e',		//SDLK_EURO		= 321,		/* Some european keyboards */
	0			//SDLK_UNDO		= 322,		/* Atari keyboard has Undo */
};
#endif

static unsigned int tbl_sdltoquakemouse[] =
{
	K_MOUSE1,
	K_MOUSE3,
	K_MOUSE2,
	K_MWHEELUP,
	K_MWHEELDOWN,
	K_MOUSE4,
	K_MOUSE5,
	K_MOUSE6,
	K_MOUSE7,
	K_MOUSE8,
	K_MOUSE9,
	K_MOUSE10
};

void Sys_SendKeyEvents(void)
{
	SDL_Event event;
	while(SDL_PollEvent(&event))
	{
		switch(event.type)
		{
#if SDL_MAJOR_VERSION >= 2
		case SDL_WINDOWEVENT:
			switch(event.window.event)
			{
			default:
				break;
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				#if SDL_PATCHLEVEL >= 1
					SDL_GL_GetDrawableSize(sdlwindow, &vid.pixelwidth, &vid.pixelheight);	//get the proper physical size.
				#else
					SDL_GetWindowSize(sdlwindow, &vid.pixelwidth, &vid.pixelheight);
				#endif
				{
					extern cvar_t vid_conautoscale, vid_conwidth;	//make sure the screen is updated properly.
					Cvar_ForceCallback(&vid_conautoscale);
					Cvar_ForceCallback(&vid_conwidth);
				}
				break;
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				ActiveApp = true;
				break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
				ActiveApp = false;
				break;
			case SDL_WINDOWEVENT_CLOSE:
				Cbuf_AddText("quit prompt\n", RESTRICT_LOCAL);
				break;
			}
			break;
#else

		case SDL_ACTIVEEVENT:
			if (event.active.state & SDL_APPINPUTFOCUS)
			{	//follow keyboard status
				ActiveApp = !!event.active.gain;
				break;
			}
			break;

		case SDL_VIDEORESIZE:
#ifndef SERVERONLY
			vid.pixelwidth = event.resize.w;
			vid.pixelheight = event.resize.h;
			{
			extern cvar_t vid_conautoscale, vid_conwidth;	//make sure the screen is updated properly.
			Cvar_ForceCallback(&vid_conautoscale);
			Cvar_ForceCallback(&vid_conwidth);
			}
#endif
			break;
#endif

		case SDL_KEYUP:
		case SDL_KEYDOWN:
			{
				int s = event.key.keysym.sym;
				int qs;
#if SDL_MAJOR_VERSION >= 2
				qs = MySDL_MapKey(s);
#else
				if (s < sizeof(tbl_sdltoquake) / sizeof(tbl_sdltoquake[0]))
					qs = tbl_sdltoquake[s];
				else 
					qs = 0;
#endif

#ifdef FTE_TARGET_WEB
				if (s == 1249)
					qs = K_SHIFT;
#endif
#ifdef HAVE_SDL_TEXTINPUT
				IN_KeyEvent(0, event.key.state, qs, 0);
#else
				IN_KeyEvent(0, event.key.state, qs, event.key.keysym.unicode);
#endif
			}
			break;
#ifdef HAVE_SDL_TEXTINPUT
		case SDL_TEXTINPUT:
			{
				unsigned int uc;
				int err;
				char *text = event.text.text;
				while(*text)
				{
					uc = utf8_decode(&err, text, &text);
					if (uc && !err)
					{
						IN_KeyEvent(0, true, 0, uc);
						IN_KeyEvent(0, false, 0, uc);
					}
				}
			}
			break;
#endif

#if SDL_MAJOR_VERSION >= 2
		case SDL_FINGERDOWN:
		case SDL_FINGERUP:
			IN_MouseMove(event.tfinger.fingerId, true, event.tfinger.x * vid.pixelwidth, event.tfinger.y * vid.pixelheight, 0, event.tfinger.pressure);
			IN_KeyEvent(event.tfinger.fingerId, event.type==SDL_FINGERDOWN, K_MOUSE1, 0);
			break;
		case SDL_FINGERMOTION:
			IN_MouseMove(event.tfinger.fingerId, true, event.tfinger.x * vid.pixelwidth, event.tfinger.y * vid.pixelheight, 0, event.tfinger.pressure);
			break;

		case SDL_DROPFILE:
			Host_RunFile(event.drop.file, strlen(event.drop.file), NULL);
			SDL_free(event.drop.file);
			break;
#endif

		case SDL_MOUSEMOTION:
#if SDL_MAJOR_VERSION >= 2
			if (event.motion.which == SDL_TOUCH_MOUSEID)
				break;	//ignore legacy touch events.
#endif
			if (!mouseactive)
				IN_MouseMove(event.motion.which, true, event.motion.x, event.motion.y, 0, 0);
			else
				IN_MouseMove(event.motion.which, false, event.motion.xrel, event.motion.yrel, 0, 0);
			break;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
#if SDL_MAJOR_VERSION >= 2
			if (event.button.which == SDL_TOUCH_MOUSEID)
				break;	//ignore legacy touch events. 
#endif
			//Hmm. SDL allows for 255 buttons, but only defines 5...
			if (event.button.button > sizeof(tbl_sdltoquakemouse)/sizeof(tbl_sdltoquakemouse[0]))
				event.button.button = sizeof(tbl_sdltoquakemouse)/sizeof(tbl_sdltoquakemouse[0]);
			IN_KeyEvent(event.button.which, event.button.state, tbl_sdltoquakemouse[event.button.button-1], 0);
			break;

#if SDL_MAJOR_VERSION >= 2
		case SDL_APP_TERMINATING:
			Cbuf_AddText("quit force\n", RESTRICT_LOCAL);
			break;
#endif
		case SDL_QUIT:
			Cbuf_AddText("quit\n", RESTRICT_LOCAL);
			break;

#if SDL_MAJOR_VERSION >= 2
		//actually, joysticks *should* work with sdl1 as well, but there are some differences (like no hot plugging, I think).
		case SDL_JOYAXISMOTION:
			break;
//		case SDL_JOYBALLMOTION:
//		case SDL_JOYHATMOTION:
			break;
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			J_JoystickButton(event.jbutton.which, event.jbutton.button, event.type==SDL_CONTROLLERBUTTONDOWN);
			break;
		case SDL_JOYDEVICEADDED:
			J_JoystickAdded(event.jdevice.which);
			break;
		case SDL_JOYDEVICEREMOVED:
			J_Kill(event.jdevice.which, true);
			break;

		case SDL_CONTROLLERAXISMOTION:
			J_ControllerAxis(event.caxis.which, event.caxis.axis, event.caxis.value);
			break;
		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
			J_ControllerButton(event.cbutton.which, event.cbutton.button, event.type==SDL_CONTROLLERBUTTONDOWN);
			break;
		case SDL_CONTROLLERDEVICEADDED:
			J_ControllerAdded(event.cdevice.which);
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
			J_Kill(event.cdevice.which, true);
			break;
//		case SDL_CONTROLLERDEVICEREMAPPED:
//			break;
#endif
		}
	}
}






void INS_Shutdown (void)
{
	IN_DeactivateMouse();

#if SDL_MAJOR_VERSION >= 2
	J_KillAll();
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK|SDL_INIT_GAMECONTROLLER);
#endif
}

void INS_ReInit (void)
{
	IN_ActivateMouse();

#ifdef HAVE_SDL_TEXTINPUT
	SDL_StartTextInput();
#else
	SDL_EnableUNICODE(SDL_ENABLE);
#endif
#if SDL_MAJOR_VERSION >= 2
	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
#endif
}

//stubs, all the work is done in Sys_SendKeyEvents
void INS_Move(float *movements, int pnum)
{
}
void INS_Init (void)
{
#if SDL_MAJOR_VERSION >= 2
	SDL_InitSubSystem(SDL_INIT_JOYSTICK|SDL_INIT_GAMECONTROLLER);
#endif
}
void INS_Accumulate(void)	//input polling
{
}
void INS_Commands (void)	//used to Cbuf_AddText joystick button events in windows.
{
}
void INS_EnumerateDevices(void *ctx, void(*callback)(void *ctx, char *type, char *devicename, int *qdevid))
{
}


