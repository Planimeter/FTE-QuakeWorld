//ezquake likes this
#include <assert.h>
#include <ctype.h>

//ezquake types.
#define byte qbyte
#define qbool qboolean
#define Com_Printf Con_Printf
#define Com_DPrintf Con_DPrintf
#define Cvar_Find(n) pCvar_GetNVFDG(n,NULL,0,NULL,NULL)
#define Cvar_SetValue(var,val) pCvar_SetFloat(var->name,val)
#define Cvar_Set(var,val) pCvar_SetString(var->name,val)
#define Cmd_Argc pCmd_Argc
#define Cmd_Argv(x) ""
#define Cbuf_AddText(x) pCmd_AddText(x,false)
#define Sys_Error(x) pSys_Error(x)
#define Q_calloc calloc
#define Q_malloc malloc
#define Q_strdup strdup
#define Q_free free
#define Q_rint(x) ((int)(x+0.5))
#define Q_atoi atoi
#define strlcpy Q_strlcpy
#define strlcat Q_strlcat
#undef snprintf
#define snprintf Q_snprintf

#undef mpic_t
#define mpic_t void


#define MV_VIEWS 4


extern float cursor_x;
extern float cursor_y;
extern int host_screenupdatecount;
extern cvar_t *scr_newHud;
extern cvar_t *cl_multiview;

#define Cam_TrackNum() cl.tracknum
#define spec_track cl.tracknum
#define autocam ((spec_track==-1)?CAM_NONE:CAM_TRACK)
//#define HAXX

#define vid plugvid
#define cls plugcls
#define cl plugcl
#define player_info_t plugclientinfo_t

struct {
	int intermission;
	int teamplay;
	int deathmatch;
	int stats[MAX_CL_STATS];
	int item_gettime[32];
	char serverinfo[4096];
	player_info_t players[MAX_CLIENTS];
	int playernum;
	int tracknum;
	vec3_t simvel;
	float time;
	float faceanimtime;
	qboolean spectator;
	qboolean standby;
	qboolean countdown;

	int splitscreenview;
} cl;
struct {
	int state;
	float min_fps;
	float fps;
	float realtime;
	float frametime;
	qbool mvdplayback;
	int demoplayback;
} cls;
struct {
	int width;
	int height;
//	float displayFrequency;
} vid;


//reimplementations of ezquake functions
void Draw_SetOverallAlpha(float a);
void Draw_AlphaFillRGB(float x, float y, float w, float h, qbyte r, qbyte g, qbyte b, qbyte a);
void Draw_Fill(float x, float y, float w, float h, qbyte pal);
char *ColorNameToRGBString (const char *newval);
byte *StringToRGB(const char *str);

#define Draw_String					pDraw_String
#define Draw_Alt_String				pDraw_String //FIXME
#define Draw_ColoredString(x,y,str,alt)			pDraw_String(x,y,str) //FIXME
#define Draw_SString(x,y,str,sc)	pDraw_String(x,y,str) //FIXME
#define Draw_SAlt_String(x,y,str,sc)	pDraw_String(x,y,str) //FIXME

void Draw_SPic(float x, float y, mpic_t *pic, float scale);
void Draw_SSubPic(float x, float y, mpic_t *pic, float s1, float t1, float s2, float t2, float scale);
#define Draw_STransPic Draw_SPic
void Draw_Character(float x, float y, unsigned int ch);
void Draw_SCharacter(float x, float y, unsigned int ch, float scale);

void SCR_DrawWadString(float x, float y, float scale, char *str);

void Draw_SAlphaSubPic2(float x, float y, mpic_t *pic, float s1, float t1, float s2, float t2, float w, float h, float alpha);

void Draw_AlphaFill(float x, float y, float w, float h, qbyte pal, float alpha);
void Draw_AlphaPic(float x, float y, mpic_t *pic, float alpha);
void Draw_AlphaSubPic(float x, float y, mpic_t *pic, float s1, float t1, float s2, float t2, float alpha);
void SCR_HUD_DrawBar(int direction, int value, float max_value, float *rgba, int x, int y, int width, int height);

mpic_t *Draw_CachePicSafe(const char *name, qbool crash, qbool ignorewad);
mpic_t *Draw_CacheWadPic(const char *name);

int Sbar_TopColor(player_info_t *pi);
int Sbar_BottomColor(player_info_t *pi);
char *TP_ParseFunChars(char*, qbool chat);
char *TP_ItemName(unsigned int itbit);

#define Util_SkipChars(src,strip,dst,dstlen) strlcpy(dst,src,dstlen)
#define Util_SkipEZColors(src,dst,dstlen) strlcpy(dst,src,dstlen)

void Replace_In_String(char *string, size_t strsize, char leadchar, int patterns, ...);
static qbool Utils_RegExpMatch(char *regexp, char *term) {return true;}
#define strlen_color strlen

#define clamp(v,min,max) v=bound(min,v,max)


#define TIMETYPE_CLOCK 0
#define TIMETYPE_GAMECLOCK 1
#define TIMETYPE_GAMECLOCKINV 2
#define TIMETYPE_DEMOCLOCK 3
int SCR_GetClockStringWidth(const char *s, qbool big, float scale);
int SCR_GetClockStringHeight(qbool big, float scale);
const char* SCR_GetTimeString(int timetype, const char *format);
void SCR_DrawBigClock(int x, int y, int style, int blink, float scale, const char *t);
void SCR_DrawSmallClock(int x, int y, int style, int blink, float scale, const char *t);

typedef struct
{
	qbyte c[4];
} clrinfo_t;
void Draw_ColoredString3(float x, float y, const char *str, clrinfo_t *clr, int huh, int wut);
void UI_PrintTextBlock();
void Draw_AlphaRectangleRGB(int x, int y, int w, int h, int foo, int bar, byte r, byte g, byte b, byte a);
void Draw_AlphaLineRGB(float x1, float y1, float x2, float y2, float width, byte r, byte g, byte b, byte a);
void Draw_Polygon(int x, int y, vec3_t *vertices, int num_vertices, qbool fill, byte r, byte g, byte b, byte a);

//glue
EBUILTIN(cvar_t*, Cvar_GetNVFDG, (const char *name, const char *defaultval, unsigned int flags, const char *description, const char *groupname));
