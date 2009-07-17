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
// sbar.c -- status bar code

#include "quakedef.h"

extern cvar_t hud_tracking_show;

cvar_t scr_scoreboard_drawtitle = SCVAR("scr_scoreboard_drawtitle", "1");
cvar_t scr_scoreboard_forcecolors = SCVAR("scr_scoreboard_forcecolors", "0");	//damn americans
cvar_t scr_scoreboard_newstyle = SCVAR("scr_scoreboard_newstyle", "1");	// New scoreboard style ported from Electro, by Molgrum
cvar_t scr_scoreboard_showfrags = SCVAR("scr_scoreboard_showfrags", "0");
cvar_t scr_scoreboard_teamscores = SCVAR("scr_scoreboard_teamscores", "1");
cvar_t scr_scoreboard_titleseperator = SCVAR("scr_scoreboard_titleseperator", "1");
cvar_t sbar_teamstatus = SCVAR("sbar_teamstatus", "1");

//===========================================
//rogue changed and added defines

#define RIT_SHELLS              128
#define RIT_NAILS               256
#define RIT_ROCKETS             512
#define RIT_CELLS               1024
#define RIT_AXE                 2048
#define RIT_LAVA_NAILGUN        4096
#define RIT_LAVA_SUPER_NAILGUN  8192
#define RIT_MULTI_GRENADE       16384
#define RIT_MULTI_ROCKET        32768
#define RIT_PLASMA_GUN          65536
#define RIT_ARMOR1              8388608
#define RIT_ARMOR2              16777216
#define RIT_ARMOR3              33554432
#define RIT_LAVA_NAILS          67108864
#define RIT_PLASMA_AMMO         134217728
#define RIT_MULTI_ROCKETS       268435456
#define RIT_SHIELD              536870912
#define RIT_ANTIGRAV            1073741824
#define RIT_SUPERHEALTH         2147483648

//===========================================
//hipnotic added defines

#define HIT_PROXIMITY_GUN_BIT 16
#define HIT_MJOLNIR_BIT       7
#define HIT_LASER_CANNON_BIT  23
#define HIT_PROXIMITY_GUN   (1<<HIT_PROXIMITY_GUN_BIT)
#define HIT_MJOLNIR         (1<<HIT_MJOLNIR_BIT)
#define HIT_LASER_CANNON    (1<<HIT_LASER_CANNON_BIT)
#define HIT_WETSUIT         (1<<(23+2))
#define HIT_EMPATHY_SHIELDS (1<<(23+3))





int			sb_updates;		// if >= vid.numpages, no update needed
int			sb_hexen2_cur_item;//hexen2 hud
qboolean	sb_hexen2_extra_info;//show the extra stuff
float		sb_hexen2_item_time;

qboolean	sbar_parsingteamstatuses;	//so we don't eat it if its not displayed

#define STAT_MINUS		10	// num frame for '-' stats digit
mpic_t		*sb_nums[2][11];
mpic_t		*sb_colon, *sb_slash;
mpic_t		*sb_ibar;
mpic_t		*sb_sbar;
mpic_t		*sb_scorebar;

mpic_t		*sb_weapons[7][8];	// 0 is active, 1 is owned, 2-5 are flashes
mpic_t		*sb_ammo[4];
mpic_t		*sb_sigil[4];
mpic_t		*sb_armor[3];
mpic_t		*sb_items[32];

mpic_t	*sb_faces[7][2];		// 0 is gibbed, 1 is dead, 2-6 are alive
							// 0 is static, 1 is temporary animation
mpic_t	*sb_face_invis;
mpic_t	*sb_face_quad;
mpic_t	*sb_face_invuln;
mpic_t	*sb_face_invis_invuln;

//rogue pictures.
mpic_t      *rsb_invbar[2];
mpic_t      *rsb_weapons[5];
mpic_t      *rsb_items[2];
mpic_t      *rsb_ammo[3];
mpic_t      *rsb_teambord;
//all must be found for any to be used.

qboolean	sb_showscores;
qboolean	sb_showteamscores;

qboolean	sbarfailed;
qboolean	sbar_rogue;
qboolean	sbar_hexen2;

vrect_t		sbar_rect;	//screen area that the sbar must fit.

int			sb_lines;			// scan lines to draw

void Sbar_DeathmatchOverlay (int start);
void Sbar_TeamOverlay (void);
void Sbar_MiniDeathmatchOverlay (void);
void Sbar_ChatModeOverlay(void);

int Sbar_PlayerNum(void)
{
	int num;
	num = cl.spectator?Cam_TrackNum(0):-1;
	if (num < 0)
		num = cl.playernum[0];
	return num;
}

int Sbar_TopColour(player_info_t *p)
{
	if (scr_scoreboard_forcecolors.value)
		return p->ttopcolor;
	else
		return p->rtopcolor;
}

int Sbar_BottomColour(player_info_t *p)
{
	if (scr_scoreboard_forcecolors.value)
		return p->tbottomcolor;
	else
		return p->rbottomcolor;
}

void Draw_ExpandedString(int x, int y, conchar_t *str)
{
	while(*str)
	{
		Draw_ColouredCharacter (x, y, *str++);
		x += 8;
	}
}

void Draw_FunString(int x, int y, unsigned char *str)
{
	conchar_t buffer[2048];
	COM_ParseFunString(CON_WHITEMASK, str, buffer, sizeof(buffer), false);

	Draw_ExpandedString(x, y, buffer);
}

void Draw_FunStringLen(int x, int y, unsigned char *str, int numchars)
{
	conchar_t buffer[2048];

	numchars *= sizeof(conchar_t);	//numchars should now be the size of the chars.

	if (numchars > sizeof(buffer))
		numchars = sizeof(buffer);
	COM_ParseFunString(CON_WHITEMASK, str, buffer, numchars, false);

	Draw_ExpandedString(x, y, buffer);
}

static qboolean largegame = false;









#ifdef Q2CLIENT
void DrawHUDString (char *string, int x, int y, int centerwidth, int xor)
{
	int		margin;
	char	line[1024];
	int		width;
	int		i;

	margin = x;

	while (*string)
	{
		// scan out one line of text from the string
		width = 0;
		while (*string && *string != '\n')
			line[width++] = *string++;
		line[width] = 0;

		if (centerwidth)
			x = margin + (centerwidth - width*8)/2;
		else
			x = margin;
		for (i=0 ; i<width ; i++)
		{
			Draw_Character (x, y, line[i]^xor);
			x += 8;
		}
		if (*string)
		{
			string++;	// skip the \n
			x = margin;
			y += 8;
		}
	}
}
#define STAT_MINUS		10	// num frame for '-' stats digit
char		*q2sb_nums[2][11] =
{
	{"num_0", "num_1", "num_2", "num_3", "num_4", "num_5",
	"num_6", "num_7", "num_8", "num_9", "num_minus"},
	{"anum_0", "anum_1", "anum_2", "anum_3", "anum_4", "anum_5",
	"anum_6", "anum_7", "anum_8", "anum_9", "anum_minus"}
};

#define	ICON_WIDTH	24
#define	ICON_HEIGHT	24
#define	CHAR_WIDTH	16
#define	ICON_SPACE	8
void SCR_DrawField (int x, int y, int color, int width, int value)
{
	char	num[16], *ptr;
	int		l;
	int		frame;

	if (width < 1)
		return;

	// draw number string
	if (width > 5)
		width = 5;

	snprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;
	x += 2 + CHAR_WIDTH*(width - l);

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		Draw_TransPic (x,y,Draw_SafeCachePic(q2sb_nums[color][frame]));
		x += CHAR_WIDTH;
		ptr++;
		l--;
	}
}

char *Get_Q2ConfigString(int i)
{
	if (i >= Q2CS_IMAGES && i < Q2CS_IMAGES	+ Q2MAX_IMAGES)
		return cl.image_name [i-Q2CS_IMAGES];
	if (i == Q2CS_STATUSBAR)
		return cl.q2statusbar;

	if (i >= Q2CS_MODELS && i < Q2CS_MODELS	+ Q2MAX_MODELS)
		return cl.model_name [i-Q2CS_MODELS];
	if (i >= Q2CS_SOUNDS && i < Q2CS_SOUNDS	+ Q2MAX_SOUNDS)
		return cl.model_name [i-Q2CS_SOUNDS];
//#define	Q2CS_LIGHTS				(Q2CS_IMAGES	+Q2MAX_IMAGES)
//#define	Q2CS_ITEMS				(Q2CS_LIGHTS	+Q2MAX_LIGHTSTYLES)
//#define	Q2CS_PLAYERSKINS		(Q2CS_ITEMS		+Q2MAX_ITEMS)
//#define Q2CS_GENERAL			(Q2CS_PLAYERSKINS	+Q2MAX_CLIENTS)
	return "";
}
void Sbar_ExecuteLayoutString (char *s)
{
	int		x, y;
	int		value;
	int		width;
	int		index;
//	q2clientinfo_t	*ci;

	if (cls.state != ca_active)
		return;

	if (!s[0])
		return;

	x = sbar_rect.x;
	y = sbar_rect.y;
	width = 3;

	while (s)
	{
		s = COM_Parse (s);
		if (!strcmp(com_token, "xl"))
		{
			s = COM_Parse (s);
			x = sbar_rect.x + atoi(com_token);
			continue;
		}
		if (!strcmp(com_token, "xr"))
		{
			s = COM_Parse (s);
			x = sbar_rect.x + sbar_rect.width + atoi(com_token);
			continue;
		}
		if (!strcmp(com_token, "xv"))
		{
			s = COM_Parse (s);
			x = sbar_rect.x + sbar_rect.width/2 - 160 + atoi(com_token);
			continue;
		}

		if (!strcmp(com_token, "yt"))
		{
			s = COM_Parse (s);
			y = sbar_rect.y + atoi(com_token);
			continue;
		}
		if (!strcmp(com_token, "yb"))
		{
			s = COM_Parse (s);
			y = sbar_rect.y + sbar_rect.height + atoi(com_token);
			continue;
		}
		if (!strcmp(com_token, "yv"))
		{
			s = COM_Parse (s);
			y = sbar_rect.y + sbar_rect.height/2 - 120 + atoi(com_token);
			continue;
		}

		if (!strcmp(com_token, "pic"))
		{	// draw a pic from a stat number
			s = COM_Parse (s);
			value = cl.q2frame.playerstate.stats[atoi(com_token)];
			if (value >= Q2MAX_IMAGES || value < 0)
				Host_EndGame ("Pic >= Q2MAX_IMAGES");
			if (Get_Q2ConfigString(Q2CS_IMAGES+value))
			{
//				SCR_AddDirtyPoint (x, y);
//				SCR_AddDirtyPoint (x+23, y+23);
				Draw_Pic (x, y, Draw_SafeCachePic(Get_Q2ConfigString(Q2CS_IMAGES+value)));
			}
			continue;
		}

		if (!strcmp(com_token, "client"))
		{	// draw a deathmatch client block
			int		score, ping, time;

			s = COM_Parse (s);
			x = sbar_rect.width/2 - 160 + atoi(com_token);
			s = COM_Parse (s);
			y = sbar_rect.height/2 - 120 + atoi(com_token);
//			SCR_AddDirtyPoint (x, y);
//			SCR_AddDirtyPoint (x+159, y+31);

			s = COM_Parse (s);
			value = atoi(com_token);
			if (value >= MAX_CLIENTS || value < 0)
				Host_EndGame ("client >= MAX_CLIENTS");
//			ci = &cl.clientinfo[value];

			s = COM_Parse (s);
			score = atoi(com_token);

			s = COM_Parse (s);
			ping = atoi(com_token);

			s = COM_Parse (s);
			time = atoi(com_token);

//			DrawAltString (x+32, y, ci->name);
			Draw_String (x+32, y+8,  "Score: ");
			Draw_Alt_String (x+32+7*8, y+8,  va("%i", score));
			Draw_String (x+32, y+16, va("Ping:  %i", ping));
			Draw_String (x+32, y+24, va("Time:  %i", time));

//			if (!ci->icon)
//				ci = &cl.baseclientinfo;
//			Draw_Pic (x, y, Draw_CachePic(ci->iconname));
			continue;
		}

		if (!strcmp(com_token, "ctf"))
		{	// draw a ctf client block
			int		score, ping;
			char	block[80];

			s = COM_Parse (s);
			x = sbar_rect.width/2 - 160 + atoi(com_token);
			s = COM_Parse (s);
			y = sbar_rect.height/2 - 120 + atoi(com_token);
//			SCR_AddDirtyPoint (x, y);
//			SCR_AddDirtyPoint (x+159, y+31);

			s = COM_Parse (s);
			value = atoi(com_token);
			if (value >= MAX_CLIENTS || value < 0)
				Host_EndGame ("client >= MAX_CLIENTS");
//			ci = &cl.clientinfo[value];

			s = COM_Parse (s);
			score = atoi(com_token);

			s = COM_Parse (s);
			ping = atoi(com_token);
			if (ping > 999)
				ping = 999;

			sprintf(block, "%3d %3d %-12.12s", score, ping, "Player"/*ci->name*/);

//			if (value == cl.playernum)
//				Draw_Alt_String (x, y, block);
//			else
				Draw_String (x, y, block);
			continue;
		}

		if (!strcmp(com_token, "picn"))
		{	// draw a pic from a name
			s = COM_Parse (s);
//			SCR_AddDirtyPoint (x, y);
//			SCR_AddDirtyPoint (x+23, y+23);
			Draw_Pic (x, y, Draw_SafeCachePic(com_token));
			continue;
		}

		if (!strcmp(com_token, "num"))
		{	// draw a number
			s = COM_Parse (s);
			width = atoi(com_token);
			s = COM_Parse (s);
			value = cl.q2frame.playerstate.stats[atoi(com_token)];
			SCR_DrawField (x, y, 0, width, value);
			continue;
		}

		if (!strcmp(com_token, "hnum"))
		{	// health number
			int		color;

			width = 3;
			value = cl.q2frame.playerstate.stats[Q2STAT_HEALTH];
			if (value > 25)
				color = 0;	// green
			else if (value > 0)
				color = (cl.q2frame.serverframe>>2) & 1;		// flash
			else
				color = 1;

			if (cl.q2frame.playerstate.stats[Q2STAT_FLASHES] & 1)
				Draw_Pic (x, y, Draw_SafeCachePic("field_3"));

			SCR_DrawField (x, y, color, width, value);
			continue;
		}

		if (!strcmp(com_token, "anum"))
		{	// ammo number
			int		color;

			width = 3;
			value = cl.q2frame.playerstate.stats[Q2STAT_AMMO];
			if (value > 5)
				color = 0;	// green
			else if (value >= 0)
				color = (cl.q2frame.serverframe>>2) & 1;		// flash
			else
				continue;	// negative number = don't show

			if (cl.q2frame.playerstate.stats[Q2STAT_FLASHES] & 4)
				Draw_Pic (x, y, Draw_SafeCachePic("field_3"));

			SCR_DrawField (x, y, color, width, value);
			continue;
		}

		if (!strcmp(com_token, "rnum"))
		{	// armor number
			int		color;

			width = 3;
			value = cl.q2frame.playerstate.stats[Q2STAT_ARMOR];
			if (value < 1)
				continue;

			color = 0;	// green

			if (cl.q2frame.playerstate.stats[Q2STAT_FLASHES] & 2)
				Draw_Pic (x, y, Draw_SafeCachePic("field_3"));

			SCR_DrawField (x, y, color, width, value);
			continue;
		}


		if (!strcmp(com_token, "stat_string"))
		{
			s = COM_Parse (s);
			index = atoi(com_token);
			if (index < 0 || index >= Q2MAX_CONFIGSTRINGS)
				Host_EndGame ("Bad stat_string index");
			index = cl.q2frame.playerstate.stats[index];
			if (index < 0 || index >= Q2MAX_CONFIGSTRINGS)
				Host_EndGame ("Bad stat_string index");
			Draw_String (x, y, Get_Q2ConfigString(index));
			continue;
		}

		if (!strcmp(com_token, "cstring"))
		{
			s = COM_Parse (s);
			DrawHUDString (com_token, x, y, 320, 0);
			continue;
		}

		if (!strcmp(com_token, "string"))
		{
			s = COM_Parse (s);
			Draw_String (x, y, com_token);
			continue;
		}

		if (!strcmp(com_token, "cstring2"))
		{
			s = COM_Parse (s);
			DrawHUDString (com_token, x, y, 320,0x80);
			continue;
		}

		if (!strcmp(com_token, "string2"))
		{
			s = COM_Parse (s);
			Draw_Alt_String (x, y, com_token);
			continue;
		}

		if (!strcmp(com_token, "if"))
		{	// draw a number
			s = COM_Parse (s);
			value = cl.q2frame.playerstate.stats[atoi(com_token)];
			if (!value)
			{	// skip to endif
				while (s && strcmp(com_token, "endif") )
				{
					s = COM_Parse (s);
				}
			}

			continue;
		}


	}
}
#endif

/*
===============
Sbar_ShowTeamScores

Tab key down
===============
*/
void Sbar_ShowTeamScores (void)
{
	if (sb_showteamscores)
		return;

#ifdef CSQC_DAT
	if (CSQC_ConsoleCommand(Cmd_Argv(0)))
		return;
#endif

	sb_showteamscores = true;
	sb_updates = 0;
}

/*
===============
Sbar_DontShowTeamScores

Tab key up
===============
*/
void Sbar_DontShowTeamScores (void)
{
	sb_showteamscores = false;
	sb_updates = 0;

#ifdef CSQC_DAT
	if (CSQC_ConsoleCommand(Cmd_Argv(0)))
		return;
#endif
}

/*
===============
Sbar_ShowScores

Tab key down
===============
*/
void Sbar_ShowScores (void)
{
	if (scr_scoreboard_teamscores.value)
	{
		Sbar_ShowTeamScores();
		return;
	}

	if (sb_showscores)
		return;

#ifdef CSQC_DAT
	if (CSQC_ConsoleCommand(Cmd_Argv(0)))
		return;
#endif

	sb_showscores = true;
	sb_updates = 0;
}

void Sbar_Hexen2InvLeft_f(void)
{
	sb_hexen2_item_time = realtime;
	sb_hexen2_cur_item--;
	if (sb_hexen2_cur_item < 0)
		sb_hexen2_cur_item = 14;
}
void Sbar_Hexen2InvRight_f(void)
{
	sb_hexen2_item_time = realtime;
	sb_hexen2_cur_item++;
	if (sb_hexen2_cur_item > 14)
		sb_hexen2_cur_item = 0;
}
void Sbar_Hexen2InvUse_f(void)
{
	Cbuf_AddText(va("impulse %d\n", 100+sb_hexen2_cur_item), Cmd_ExecLevel);
}

void Sbar_Hexen2ShowInfo_f(void)
{
	sb_hexen2_extra_info = true;
}

void Sbar_Hexen2DontShowInfo_f(void)
{
	sb_hexen2_extra_info = false;
}

/*
===============
Sbar_DontShowScores

Tab key up
===============
*/
void Sbar_DontShowScores (void)
{
	if (scr_scoreboard_teamscores.value)
	{
		Sbar_DontShowTeamScores();
		return;
	}

	sb_showscores = false;
	sb_updates = 0;

#ifdef CSQC_DAT
	if (CSQC_ConsoleCommand(Cmd_Argv(0)))
		return;
#endif
}

/*
===============
Sbar_Changed
===============
*/
void Sbar_Changed (void)
{
	sb_updates = 0;	// update next frame
}

/*
===============
Sbar_Init
===============
*/

qboolean sbar_loaded;

char *failedpic;
mpic_t *Sbar_PicFromWad(char *name)
{
	mpic_t *ret;
	ret = Draw_SafePicFromWad(name);

	if (ret)
		return ret;

	failedpic = name;
	return NULL;
}
void Sbar_Flush (void)
{
	sbar_loaded = false;
}
void Sbar_Start (void)	//if one of these fails, skip the entire status bar.
{
	int		i;
	if (sbar_loaded)
		return;

	sbar_loaded = true;

	if (!wad_base)	//the wad isn't loaded. This is an indication that it doesn't exist.
	{
		sbarfailed = true;
		return;
	}
	failedpic = NULL;

	sbarfailed = false;

	for (i=0 ; i<10 ; i++)
	{
		sb_nums[0][i] = Sbar_PicFromWad (va("num_%i",i));
		sb_nums[1][i] = Sbar_PicFromWad (va("anum_%i",i));
	}

	if (sb_nums[0][0] && sb_nums[0][0]->width < 13)
		sbar_hexen2 = true;

	sb_nums[0][10] = Sbar_PicFromWad ("num_minus");
	sb_nums[1][10] = Sbar_PicFromWad ("anum_minus");

	sb_colon = Sbar_PicFromWad ("num_colon");
	sb_slash = Sbar_PicFromWad ("num_slash");

	sb_weapons[0][0] = Sbar_PicFromWad ("inv_shotgun");
	sb_weapons[0][1] = Sbar_PicFromWad ("inv_sshotgun");
	sb_weapons[0][2] = Sbar_PicFromWad ("inv_nailgun");
	sb_weapons[0][3] = Sbar_PicFromWad ("inv_snailgun");
	sb_weapons[0][4] = Sbar_PicFromWad ("inv_rlaunch");
	sb_weapons[0][5] = Sbar_PicFromWad ("inv_srlaunch");
	sb_weapons[0][6] = Sbar_PicFromWad ("inv_lightng");

	sb_weapons[1][0] = Sbar_PicFromWad ("inv2_shotgun");
	sb_weapons[1][1] = Sbar_PicFromWad ("inv2_sshotgun");
	sb_weapons[1][2] = Sbar_PicFromWad ("inv2_nailgun");
	sb_weapons[1][3] = Sbar_PicFromWad ("inv2_snailgun");
	sb_weapons[1][4] = Sbar_PicFromWad ("inv2_rlaunch");
	sb_weapons[1][5] = Sbar_PicFromWad ("inv2_srlaunch");
	sb_weapons[1][6] = Sbar_PicFromWad ("inv2_lightng");

	for (i=0 ; i<5 ; i++)
	{
		sb_weapons[2+i][0] = Sbar_PicFromWad (va("inva%i_shotgun",i+1));
		sb_weapons[2+i][1] = Sbar_PicFromWad (va("inva%i_sshotgun",i+1));
		sb_weapons[2+i][2] = Sbar_PicFromWad (va("inva%i_nailgun",i+1));
		sb_weapons[2+i][3] = Sbar_PicFromWad (va("inva%i_snailgun",i+1));
		sb_weapons[2+i][4] = Sbar_PicFromWad (va("inva%i_rlaunch",i+1));
		sb_weapons[2+i][5] = Sbar_PicFromWad (va("inva%i_srlaunch",i+1));
		sb_weapons[2+i][6] = Sbar_PicFromWad (va("inva%i_lightng",i+1));
	}

	sb_ammo[0] = Sbar_PicFromWad ("sb_shells");
	sb_ammo[1] = Sbar_PicFromWad ("sb_nails");
	sb_ammo[2] = Sbar_PicFromWad ("sb_rocket");
	sb_ammo[3] = Sbar_PicFromWad ("sb_cells");

	sb_armor[0] = Sbar_PicFromWad ("sb_armor1");
	sb_armor[1] = Sbar_PicFromWad ("sb_armor2");
	sb_armor[2] = Sbar_PicFromWad ("sb_armor3");

	sb_items[0] = Sbar_PicFromWad ("sb_key1");
	sb_items[1] = Sbar_PicFromWad ("sb_key2");
	sb_items[2] = Sbar_PicFromWad ("sb_invis");
	sb_items[3] = Sbar_PicFromWad ("sb_invuln");
	sb_items[4] = Sbar_PicFromWad ("sb_suit");
	sb_items[5] = Sbar_PicFromWad ("sb_quad");

	sb_sigil[0] = Sbar_PicFromWad ("sb_sigil1");
	sb_sigil[1] = Sbar_PicFromWad ("sb_sigil2");
	sb_sigil[2] = Sbar_PicFromWad ("sb_sigil3");
	sb_sigil[3] = Sbar_PicFromWad ("sb_sigil4");

	sb_faces[4][0] = Sbar_PicFromWad ("face1");
	sb_faces[4][1] = Sbar_PicFromWad ("face_p1");
	sb_faces[3][0] = Sbar_PicFromWad ("face2");
	sb_faces[3][1] = Sbar_PicFromWad ("face_p2");
	sb_faces[2][0] = Sbar_PicFromWad ("face3");
	sb_faces[2][1] = Sbar_PicFromWad ("face_p3");
	sb_faces[1][0] = Sbar_PicFromWad ("face4");
	sb_faces[1][1] = Sbar_PicFromWad ("face_p4");
	sb_faces[0][0] = Sbar_PicFromWad ("face5");
	sb_faces[0][1] = Sbar_PicFromWad ("face_p5");

	sb_face_invis = Sbar_PicFromWad ("face_invis");
	sb_face_invuln = Sbar_PicFromWad ("face_invul2");
	sb_face_invis_invuln = Sbar_PicFromWad ("face_inv2");
	sb_face_quad = Sbar_PicFromWad ("face_quad");

	sb_sbar = Sbar_PicFromWad ("sbar");
	sb_ibar = Sbar_PicFromWad ("ibar");
	sb_scorebar = Sbar_PicFromWad ("scorebar");

	if (failedpic)
		sbarfailed = true;
	failedpic = NULL;



	//try to detect rogue wads, and thus the stats we will be getting from the server.
	failedpic = NULL;

	rsb_invbar[0] = Sbar_PicFromWad ("r_invbar1");
	rsb_invbar[1] = Sbar_PicFromWad ("r_invbar2");

	rsb_weapons[0] = Sbar_PicFromWad ("r_lava");
	rsb_weapons[1] = Sbar_PicFromWad ("r_superlava");
	rsb_weapons[2] = Sbar_PicFromWad ("r_gren");
	rsb_weapons[3] = Sbar_PicFromWad ("r_multirock");
	rsb_weapons[4] = Sbar_PicFromWad ("r_plasma");

	rsb_items[0] = Sbar_PicFromWad ("r_shield1");
	rsb_items[1] = Sbar_PicFromWad ("r_agrav1");

	rsb_teambord = Sbar_PicFromWad ("r_teambord");

	rsb_ammo[0] = Sbar_PicFromWad ("r_ammolava");
	rsb_ammo[1] = Sbar_PicFromWad ("r_ammomulti");
	rsb_ammo[2] = Sbar_PicFromWad ("r_ammoplasma");

	if (!failedpic || COM_CheckParm("-rogue"))
		sbar_rogue = true;
	else
		sbar_rogue = false;
}

void Sbar_Init (void)
{
	Cvar_Register(&scr_scoreboard_drawtitle, "Scoreboard settings");
	Cvar_Register(&scr_scoreboard_forcecolors, "Scoreboard settings");
	Cvar_Register(&scr_scoreboard_newstyle, "Scoreboard settings");
	Cvar_Register(&scr_scoreboard_showfrags, "Scoreboard settings");
	Cvar_Register(&scr_scoreboard_teamscores, "Scoreboard settings");
	Cvar_Register(&scr_scoreboard_titleseperator, "Scoreboard settings");

	Cvar_Register(&sbar_teamstatus, "Status bar settings");

	Cmd_AddCommand ("+showscores", Sbar_ShowScores);
	Cmd_AddCommand ("-showscores", Sbar_DontShowScores);

	Cmd_AddCommand ("+showteamscores", Sbar_ShowTeamScores);
	Cmd_AddCommand ("-showteamscores", Sbar_DontShowTeamScores);

	//stuff to get hexen2 working out-of-the-box
	Cmd_AddCommand ("invleft", Sbar_Hexen2InvLeft_f);
	Cmd_AddCommand ("invright", Sbar_Hexen2InvRight_f);
	Cmd_AddCommand ("invuse", Sbar_Hexen2InvUse_f);
	Cmd_AddCommand ("+showinfo", Sbar_Hexen2ShowInfo_f);
	Cmd_AddCommand ("-showinfo", Sbar_Hexen2DontShowInfo_f);
	Cbuf_AddText("alias +crouch \"impulse 22\"\n", RESTRICT_LOCAL);
	Cbuf_AddText("alias -crouch \"impulse 22\"\n", RESTRICT_LOCAL);
}


//=============================================================================

// drawing routines are reletive to the status bar location
/*
=============
Sbar_DrawPic
=============
*/
void Sbar_DrawPic (int x, int y, mpic_t *pic)
{
	Draw_Pic (sbar_rect.x + x /* + ((sbar_rect.width - 320)>>1) */, sbar_rect.y + y + (sbar_rect.height-SBAR_HEIGHT), pic);
}

/*
=============
Sbar_DrawSubPic
=============
JACK: Draws a portion of the picture in the status bar.
*/

void Sbar_DrawSubPic(int x, int y, mpic_t *pic, int srcx, int srcy, int width, int height)
{
	Draw_SubPic (sbar_rect.x + x, sbar_rect.y + y+(sbar_rect.height-SBAR_HEIGHT), pic, srcx, srcy, width, height);
}


/*
=============
Sbar_DrawTransPic
=============
*/
void Sbar_DrawTransPic (int x, int y, mpic_t *pic)
{
	Draw_TransPic (sbar_rect.x + x /*+ ((sbar_rect.width - 320)>>1) */, sbar_rect.y + y + (sbar_rect.height-SBAR_HEIGHT), pic);
}

/*
================
Sbar_DrawCharacter

Draws one solid graphics character
================
*/
void Sbar_DrawCharacter (int x, int y, int num)
{
	Draw_Character (sbar_rect.x +  x /*+ ((sbar_rect.width - 320)>>1) */ + 4, sbar_rect.y + y + sbar_rect.height-SBAR_HEIGHT, num);
}

/*
================
Sbar_DrawString
================
*/
void Sbar_DrawString (int x, int y, char *str)
{
	Draw_String (sbar_rect.x + x /*+ ((sbar_rect.width - 320)>>1) */, sbar_rect.y + y+ sbar_rect.height-SBAR_HEIGHT, str);
}

void Sbar_DrawFunString (int x, int y, char *str)
{
	Draw_FunString (sbar_rect.x + x /*+ ((sbar_rect.width - 320)>>1) */, sbar_rect.y + y+ sbar_rect.height-SBAR_HEIGHT, str);
}

void Draw_TinyString (int x, int y, const qbyte *str)
{
	float xstart = x;
	while (*str)
	{
		if (*str == '\n')
		{
			x = xstart;
			y += 6;
			str++;
			continue;
		}
		if (Draw_TinyCharacter)
		Draw_TinyCharacter (x, y, *str);
//		Draw_Character (x, y, *str);
		str++;
		x += 6;
	}
}
void Sbar_DrawTinyString (int x, int y, char *str)
{
	Draw_TinyString (sbar_rect.x + x /*+ ((sbar_rect.width - 320)>>1) */, sbar_rect.y + y+ sbar_rect.height-SBAR_HEIGHT, str);
}

void Sbar_FillPC (int x, int y, int w, int h, unsigned int pcolour)
{
	if (pcolour >= 16)
	{
		Draw_FillRGB (x, y, w, h, ((pcolour&0xff0000)>>16)/255.0f, ((pcolour&0xff00)>>8)/255.0f, (pcolour&0xff)/255.0f);
	}
	else
	{
		pcolour = Sbar_ColorForMap(pcolour);
		Draw_Fill (x, y, w, h, pcolour);
	}
}
static void Sbar_FillPCDark (int x, int y, int w, int h, unsigned int pcolour)
{
	if (pcolour >= 16)
	{
		Draw_FillRGB (x, y, w, h, (pcolour&0xff)/1024.0f, ((pcolour&0xff00)>>8)/1024.0f, ((pcolour&0xff0000)>>17)/1024.0f);
	}
	else
	{
		pcolour = Sbar_ColorForMap(pcolour)-1;
		Draw_Fill (x, y, w, h, pcolour);
	}
}


/*
=============
Sbar_itoa
=============
*/
int Sbar_itoa (int num, char *buf)
{
	char	*str;
	int		pow10;
	int		dig;

	str = buf;

	if (num < 0)
	{
		*str++ = '-';
		num = -num;
	}

	for (pow10 = 10 ; num >= pow10 ; pow10 *= 10)
	;

	do
	{
		pow10 /= 10;
		dig = num/pow10;
		*str++ = '0'+dig;
		num -= dig*pow10;
	} while (pow10 != 1);

	*str = 0;

	return str-buf;
}


/*
=============
Sbar_DrawNum
=============
*/
void Sbar_DrawNum (int x, int y, int num, int digits, int color)
{
	char			str[12];
	char			*ptr;
	int				l, frame;
#undef small
	int small=false;

	if (digits < 0)
	{
		small = true;
		digits*=-1;
	}

	l = Sbar_itoa (num, str);
	ptr = str;
	if (l > digits)
		ptr += (l-digits);

	if (small)
	{
		if (l < digits)
			x += (digits-l)*8;
		while (*ptr)
		{
			Sbar_DrawCharacter(x, y, *ptr+18 - '0');
			ptr++;
			x+=8;
		}
		return;
	}
	if (l < digits)
		x += (digits-l)*24;

	while (*ptr)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		Sbar_DrawTransPic (x,y,sb_nums[color][frame]);
		x += 24;
		ptr++;
	}
}

void Sbar_Hexen2DrawNum (int x, int y, int num, int digits)
{
	char			str[12];
	char			*ptr;
	int				l, frame;

	l = Sbar_itoa (num, str);
	ptr = str;
	if (l > digits)
		ptr += (l-digits);

	//hexen2 hud has it centered
	if (l < digits)
		x += ((digits-l)*13)/2;

	while (*ptr)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		Sbar_DrawTransPic (x,y,sb_nums[0][frame]);
		x += 13;
		ptr++;
	}
}

//=============================================================================

int		fragsort[MAX_CLIENTS];
int		scoreboardlines;
typedef struct {
	char team[16+1];
	int frags;
	int players;
	int plow, phigh, ptotal;
	int topcolour, bottomcolour;
	qboolean ownteam;
} team_t;
team_t teams[MAX_CLIENTS];
int teamsort[MAX_CLIENTS];
int scoreboardteams;

/*
===============
Sbar_SortFrags
===============
*/
void Sbar_SortFrags (qboolean includespec)
{
	int		i, j, k;

// sort by frags
	scoreboardlines = 0;
	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (cl.players[i].name[0] &&
			(!cl.players[i].spectator || includespec))
		{
			fragsort[scoreboardlines] = i;
			scoreboardlines++;
			if (cl.players[i].spectator)
				cl.players[i].frags = -999;
		}
	}

	for (i=0 ; i<scoreboardlines ; i++)
		for (j=0 ; j<scoreboardlines-1-i ; j++)
			if (cl.players[fragsort[j]].frags < cl.players[fragsort[j+1]].frags)
			{
				k = fragsort[j];
				fragsort[j] = fragsort[j+1];
				fragsort[j+1] = k;
			}
}

void Sbar_SortTeams (void)
{
	int				i, j, k;
	player_info_t	*s;
	char t[16+1];
	int ownnum;

// request new ping times every two second
	scoreboardteams = 0;

	if (!cl.teamplay)
		return;

// sort the teams
	memset(teams, 0, sizeof(teams));
	for (i = 0; i < MAX_CLIENTS; i++)
		teams[i].plow = 999;

	ownnum = Sbar_PlayerNum();

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		s = &cl.players[i];
		if (!s->name[0] || s->spectator)
			continue;

		// find his team in the list
		Q_strncpyz(t, s->team, sizeof(t));
		if (!t[0])
			continue; // not on team
		for (j = 0; j < scoreboardteams; j++)
			if (!strcmp(teams[j].team, t))
			{
				break;
			}

		/*if (cl.teamfortress)
		{
			teams[j].topcolour = teams[j].bottomcolour = TF_TeamToColour(t);
		}
		else*/ if (j == scoreboardteams || i == ownnum)
		{
			teams[j].topcolour = Sbar_TopColour(s);
			teams[j].bottomcolour = Sbar_BottomColour(s);
		}

		if (j == scoreboardteams)
		{ // create a team for this player
			scoreboardteams++;
			strcpy(teams[j].team, t);
		}

		teams[j].frags += s->frags;
		teams[j].players++;

		if (teams[j].plow > s->ping)
			teams[j].plow = s->ping;
		if (teams[j].phigh < s->ping)
			teams[j].phigh = s->ping;
		teams[j].ptotal += s->ping;
		
	}

	// sort
	for (i = 0; i < scoreboardteams; i++)
		teamsort[i] = i;

	// good 'ol bubble sort
	for (i = 0; i < scoreboardteams - 1; i++)
		for (j = i + 1; j < scoreboardteams; j++)
			if (teams[teamsort[i]].frags < teams[teamsort[j]].frags)
			{
				k = teamsort[i];
				teamsort[i] = teamsort[j];
				teamsort[j] = k;
			}
}

unsigned int	Sbar_ColorForMap (unsigned int m)
{
	if (m >= 16)
		return m;

	m = (m < 0) ? 0 : ((m > 13) ? 13 : m);

	m *= 16;
	return m < 128 ? m + 8 : m + 8;
}


/*
===============
Sbar_SoloScoreboard
===============
*/
void Sbar_SoloScoreboard (void)
{
	int l;
	float time;
	char	str[80];
	int		minutes, seconds, tens, units;

	Sbar_DrawPic (0, 0, sb_scorebar);

	// time
	time = cl.servertime;
	minutes = time / 60;
	seconds = time - 60*minutes;
	tens = seconds / 10;
	units = seconds - 10*tens;
	sprintf (str,"Time :%3i:%i%i", minutes, tens, units);
	Sbar_DrawString (184, 4, str);

	// draw level name
	l = strlen (cl.levelname);
	Sbar_DrawString (232 - l*4, 12, cl.levelname);
}

void Sbar_CoopScoreboard (void)
{
	float time;
	char	str[80];
	int		minutes, seconds, tens, units;
	int		l;

	sprintf (str,"Monsters:%3i /%3i", cl.stats[0][STAT_MONSTERS], cl.stats[0][STAT_TOTALMONSTERS]);
	Sbar_DrawString (8, 4, str);

	sprintf (str,"Secrets :%3i /%3i", cl.stats[0][STAT_SECRETS], cl.stats[0][STAT_TOTALSECRETS]);
	Sbar_DrawString (8, 12, str);

// time
	time = cl.servertime;
	minutes = time / 60;
	seconds = time - 60*minutes;
	tens = seconds / 10;
	units = seconds - 10*tens;
	sprintf (str,"Time :%3i:%i%i", minutes, tens, units);
	Sbar_DrawString (184, 4, str);

// draw level name
	l = strlen (cl.levelname);
	Sbar_DrawString (232 - l*4, 12, cl.levelname);
}

//=============================================================================

/*
===============
Sbar_DrawInventory
===============
*/
void Sbar_DrawInventory (int pnum)
{
	int		i;
	char	num[6];
	float	time;
	int		flashon;
	qboolean	headsup;
	qboolean    hudswap;

	headsup = !(cl_sbar.value || (scr_viewsize.value<100&&cl.splitclients==1));
	hudswap = cl_hudswap.value; // Get that nasty float out :)

	if (!headsup)
	{
		if (sbar_rogue)
		{
			if ( cl.stats[pnum][STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN )
				Sbar_DrawPic (0, -24, rsb_invbar[0]);
			else
				Sbar_DrawPic (0, -24, rsb_invbar[1]);
		}
		else
			Sbar_DrawPic (0, -24, sb_ibar);
	}
// weapons
	for (i=0 ; i<7 ; i++)
	{
		if (cl.stats[pnum][STAT_ITEMS] & (IT_SHOTGUN<<i) )
		{
			time = cl.item_gettime[pnum][i];
			flashon = (int)((cl.time - time)*10);
			if (flashon < 0)
				flashon = 0;
			if (flashon >= 10)
			{
				if ( cl.stats[pnum][STAT_ACTIVEWEAPON] == (IT_SHOTGUN<<i)  )
					flashon = 1;
				else
					flashon = 0;
			}
			else
				flashon = (flashon%5) + 2;

			if (headsup) {
				if (i || sbar_rect.height>200)
					Sbar_DrawSubPic ((hudswap) ? 0 : (sbar_rect.width-24),-68-(7-i)*16 , sb_weapons[flashon][i],0,0,24,16);

			} else
			Sbar_DrawPic (i*24, -16, sb_weapons[flashon][i]);
//			Sbar_DrawSubPic (0,0,20,20,i*24, -16, sb_weapons[flashon][i]);

			if (flashon > 1)
				sb_updates = 0;		// force update to remove flash
		}
	}

	if (sbar_rogue)
	{
    // check for powered up weapon.
		if ( cl.stats[pnum][STAT_ACTIVEWEAPON] >= RIT_LAVA_NAILGUN )
		{
			for (i=0;i<5;i++)
			{
				if (cl.stats[pnum][STAT_ACTIVEWEAPON] == (RIT_LAVA_NAILGUN << i))
				{
					if (headsup)
					{
						if (sbar_rect.height>200)
							Sbar_DrawSubPic ((hudswap) ? 0 : (sbar_rect.width-24),-68-(5-i)*16 , rsb_weapons[i],0,0,24,16);

					}
					else
						Sbar_DrawPic ((i+2)*24, -16, rsb_weapons[i]);
				}
			}
		}
	}

// ammo counts
	for (i=0 ; i<4 ; i++)
	{
		sprintf (num, "%3i",cl.stats[pnum][STAT_SHELLS+i] );
		if (headsup)
		{
//			Sbar_DrawSubPic(3, -24, sb_ibar, 3, 0, 42,11);
			Sbar_DrawSubPic((hudswap) ? 0 : (sbar_rect.width-42), -24 - (4-i)*11, sb_ibar, 3+(i*48), 0, 42, 11);
			if (num[0] != ' ')
				Sbar_DrawCharacter ( (hudswap) ? 3 : (sbar_rect.width-39), -24 - (4-i)*11, 18 + num[0] - '0');
			if (num[1] != ' ')
				Sbar_DrawCharacter ( (hudswap) ? 11 : (sbar_rect.width-31), -24 - (4-i)*11, 18 + num[1] - '0');
			if (num[2] != ' ')
				Sbar_DrawCharacter ( (hudswap) ? 19 : (sbar_rect.width-23), -24 - (4-i)*11, 18 + num[2] - '0');
		}
		else
		{
			if (num[0] != ' ')
				Sbar_DrawCharacter ( (6*i+1)*8 - 2, -24, 18 + num[0] - '0');
			if (num[1] != ' ')
				Sbar_DrawCharacter ( (6*i+2)*8 - 2, -24, 18 + num[1] - '0');
			if (num[2] != ' ')
				Sbar_DrawCharacter ( (6*i+3)*8 - 2, -24, 18 + num[2] - '0');
		}
	}

	flashon = 0;
// items
	for (i=0 ; i<6 ; i++)
	{
		if (cl.stats[pnum][STAT_ITEMS] & (1<<(17+i)))
		{
			time = cl.item_gettime[pnum][17+i];
			if (time &&	time > cl.time - 2 && flashon )
			{	// flash frame
				sb_updates = 0;
			}
			else
				Sbar_DrawPic (192 + i*16, -16, sb_items[i]);
			if (time &&	time > cl.time - 2)
				sb_updates = 0;
		}
	}

	if (sbar_rogue)
	{
	// new rogue items
		for (i=0 ; i<2 ; i++)
		{
			if (cl.stats[pnum][STAT_ITEMS] & (1<<(29+i)))
			{
				time = cl.item_gettime[pnum][29+i];

				if (time &&	time > cl.time - 2 && flashon )
				{	// flash frame
					sb_updates = 0;
				}
				else
				{
					Sbar_DrawPic (288 + i*16, -16, rsb_items[i]);
				}

				if (time &&	time > cl.time - 2)
					sb_updates = 0;
			}
		}
	}
	else
	{
	// sigils
		for (i=0 ; i<4 ; i++)
		{
			if (cl.stats[pnum][STAT_ITEMS] & (1<<(28+i)))
			{
				time = cl.item_gettime[pnum][28+i];
				if (time &&	time > cl.time - 2 && flashon )
				{	// flash frame
					sb_updates = 0;
				}
				else
					Sbar_DrawPic (320-32 + i*8, -16, sb_sigil[i]);
				if (time &&	time > cl.time - 2)
					sb_updates = 0;
			}
		}
	}
}

//=============================================================================

/*
===============
Sbar_DrawFrags
===============
*/
void Sbar_DrawFrags (void)
{
	int				i, k, l;
	int				top, bottom;
	int				x, y, f;
	int				ownnum;
	char			num[12];
	player_info_t	*s;

	Sbar_SortFrags (false);

	ownnum = Sbar_PlayerNum();

// draw the text
	l = scoreboardlines <= 4 ? scoreboardlines : 4;

	x = 23;
//	xofs = (sbar_rect.width - 320)>>1;
	y = sbar_rect.height - SBAR_HEIGHT - 23;

	for (i=0 ; i<l ; i++)
	{
		k = fragsort[i];
		s = &cl.players[k];
		if (!s->name[0])
			continue;
		if (s->spectator)
			continue;

	// draw background
		top = Sbar_TopColour(s);
		bottom = Sbar_BottomColour(s);


//		Draw_Fill (xofs + x*8 + 10, y, 28, 4, top);
//		Draw_Fill (xofs + x*8 + 10, y+4, 28, 3, bottom);
		Sbar_FillPC (sbar_rect.x+x*8 + 10, sbar_rect.y+y, 28, 4, top);
		Sbar_FillPC (sbar_rect.x+x*8 + 10, sbar_rect.y+y+4, 28, 3, bottom);

	// draw number
		f = s->frags;
		sprintf (num, "%3i",f);

		Sbar_DrawCharacter ( (x+1)*8 , -24, num[0]);
		Sbar_DrawCharacter ( (x+2)*8 , -24, num[1]);
		Sbar_DrawCharacter ( (x+3)*8 , -24, num[2]);

		if (k == ownnum)
		{
			Sbar_DrawCharacter (x*8+2, -24, 16);
			Sbar_DrawCharacter ( (x+4)*8-4, -24, 17);
		}
		x+=4;
	}
}

//=============================================================================


/*
===============
Sbar_DrawFace
===============
*/
void Sbar_DrawFace (int pnum)
{
	int		f, anim;

	if ( (cl.stats[pnum][STAT_ITEMS] & (IT_INVISIBILITY | IT_INVULNERABILITY) )
	== (IT_INVISIBILITY | IT_INVULNERABILITY) )
	{
		Sbar_DrawPic (112, 0, sb_face_invis_invuln);
		return;
	}
	if (cl.stats[pnum][STAT_ITEMS] & IT_QUAD)
	{
		Sbar_DrawPic (112, 0, sb_face_quad );
		return;
	}
	if (cl.stats[pnum][STAT_ITEMS] & IT_INVISIBILITY)
	{
		Sbar_DrawPic (112, 0, sb_face_invis );
		return;
	}
	if (cl.stats[pnum][STAT_ITEMS] & IT_INVULNERABILITY)
	{
		Sbar_DrawPic (112, 0, sb_face_invuln);
		return;
	}

	if (cl.stats[pnum][STAT_HEALTH] >= 100)
		f = 4;
	else
		f = cl.stats[pnum][STAT_HEALTH] / 20;

	if (f < 0)
		f=0;

	if (cl.time <= cl.faceanimtime[pnum])
	{
		anim = 1;
		sb_updates = 0;		// make sure the anim gets drawn over
	}
	else
		anim = 0;
	Sbar_DrawPic (112, 0, sb_faces[f][anim]);
}

/*
=============
Sbar_DrawNormal
=============
*/
void Sbar_DrawNormal (int pnum)
{
	if (cl_sbar.value || (scr_viewsize.value<100&&cl.splitclients==1))
	Sbar_DrawPic (0, 0, sb_sbar);

// armor
	if (cl.stats[pnum][STAT_ITEMS] & IT_INVULNERABILITY)
	{
		Sbar_DrawNum (24, 0, 666, 3, 1);
		Sbar_DrawPic (0, 0, draw_disc);
	}
	else
	{
		if (sbar_rogue)
		{
			Sbar_DrawNum (24, 0, cl.stats[pnum][STAT_ARMOR], 3,
				cl.stats[pnum][STAT_ARMOR] <= 25);
			if (cl.stats[pnum][STAT_ITEMS] & RIT_ARMOR3)
				Sbar_DrawPic (0, 0, sb_armor[2]);
			else if (cl.stats[pnum][STAT_ITEMS] & RIT_ARMOR2)
				Sbar_DrawPic (0, 0, sb_armor[1]);
			else if (cl.stats[pnum][STAT_ITEMS] & RIT_ARMOR1)
				Sbar_DrawPic (0, 0, sb_armor[0]);
		}
		else
		{
			Sbar_DrawNum (24, 0, cl.stats[pnum][STAT_ARMOR], 3,
				cl.stats[pnum][STAT_ARMOR] <= 25);
			if (cl.stats[pnum][STAT_ITEMS] & IT_ARMOR3)
				Sbar_DrawPic (0, 0, sb_armor[2]);
			else if (cl.stats[pnum][STAT_ITEMS] & IT_ARMOR2)
				Sbar_DrawPic (0, 0, sb_armor[1]);
			else if (cl.stats[pnum][STAT_ITEMS] & IT_ARMOR1)
				Sbar_DrawPic (0, 0, sb_armor[0]);
		}
	}

// face
	Sbar_DrawFace (pnum);

// health
	Sbar_DrawNum (136, 0, cl.stats[pnum][STAT_HEALTH], 3
	, cl.stats[pnum][STAT_HEALTH] <= 25);

// ammo icon
	if (sbar_rogue)
	{
		if (cl.stats[pnum][STAT_ITEMS] & RIT_SHELLS)
			Sbar_DrawPic (224, 0, sb_ammo[0]);
		else if (cl.stats[pnum][STAT_ITEMS] & RIT_NAILS)
			Sbar_DrawPic (224, 0, sb_ammo[1]);
		else if (cl.stats[pnum][STAT_ITEMS] & RIT_ROCKETS)
			Sbar_DrawPic (224, 0, sb_ammo[2]);
		else if (cl.stats[pnum][STAT_ITEMS] & RIT_CELLS)
			Sbar_DrawPic (224, 0, sb_ammo[3]);
		else if (cl.stats[pnum][STAT_ITEMS] & RIT_LAVA_NAILS)
			Sbar_DrawPic (224, 0, rsb_ammo[0]);
		else if (cl.stats[pnum][STAT_ITEMS] & RIT_PLASMA_AMMO)
			Sbar_DrawPic (224, 0, rsb_ammo[1]);
		else if (cl.stats[pnum][STAT_ITEMS] & RIT_MULTI_ROCKETS)
			Sbar_DrawPic (224, 0, rsb_ammo[2]);
	}
	else
	{
		if (cl.stats[pnum][STAT_ITEMS] & IT_SHELLS)
			Sbar_DrawPic (224, 0, sb_ammo[0]);
		else if (cl.stats[pnum][STAT_ITEMS] & IT_NAILS)
			Sbar_DrawPic (224, 0, sb_ammo[1]);
		else if (cl.stats[pnum][STAT_ITEMS] & IT_ROCKETS)
			Sbar_DrawPic (224, 0, sb_ammo[2]);
		else if (cl.stats[pnum][STAT_ITEMS] & IT_CELLS)
			Sbar_DrawPic (224, 0, sb_ammo[3]);
	}

	Sbar_DrawNum (248, 0, cl.stats[pnum][STAT_AMMO], 3
	, cl.stats[pnum][STAT_AMMO] <= 10);
}

qboolean Sbar_ShouldDraw (void)
{
	#ifdef TEXTEDITOR
	extern qboolean editoractive;
#endif
	qboolean headsup;

	if (scr_con_current == vid.height)
		return false;		// console is full screen

#ifdef TEXTEDITOR
	if (editoractive)
		return false;
#endif

#ifdef VM_UI
	if (UI_DrawStatusBar((sb_showscores?1:0) + (sb_showteamscores?2:0))>0)
		return false;
	if (UI_MenuState())
		return false;
#endif

	headsup = !(cl_sbar.value || (scr_viewsize.value<100&&cl.splitclients==1));
	if ((sb_updates >= vid.numpages) && !headsup)
		return false;

	return true;
}

void Sbar_DrawScoreboard (void)
{
	int pnum;
	int deadcount=0;

	if (cls.protocol == CP_QUAKE2)
		return;

	for (pnum = 0; pnum < cl.splitclients; pnum++)
	{
		if (cl.stats[pnum][STAT_HEALTH] <= 0)
			deadcount++;
	}

	if (deadcount == cl.splitclients && !cl.spectator)
	{
#ifndef CLIENTONLY
		if (sv.state && cls.gamemode == GAME_COOP)
		{
			for (pnum = 0; pnum < sv.allocated_client_slots; pnum++)
			{
				if (svs.clients[pnum].state)
					if (svs.clients[pnum].netchan.remote_address.type != NA_LOOPBACK)
						break;
			}
			if (pnum == sv.allocated_client_slots)
				return;
		}
#endif
		if (cl.teamplay > 0 && !sb_showscores)
			Sbar_TeamOverlay();
		else
			Sbar_DeathmatchOverlay (0);
	}
	else if (sb_showscores)
		Sbar_DeathmatchOverlay (0);
	else if (sb_showteamscores)
		Sbar_TeamOverlay();
	else
		return;

	sb_updates = 0;
}


void Sbar_Hexen2DrawItem(int pnum, int x, int y, int itemnum)
{
	int num;
	Sbar_DrawPic(x, y, Draw_CachePic(va("gfx/arti%02d.lmp", itemnum)));

	num = cl.stats[pnum][STAT_H2_CNT_TORCH+itemnum];
	if(num > 0)
	{
		if (num >= 10)
			Sbar_DrawPic(x+20, y+21, Draw_CachePic(va("gfx/artinum%d.lmp", num/10)));
		Sbar_DrawPic(x+20+4, y+21, Draw_CachePic(va("gfx/artinum%d.lmp", num%10)));
	}
}

void Sbar_Hexen2DrawInventory(int pnum)
{
	int i;
	int x, y=-37;

	if (sb_hexen2_item_time+3 < realtime)
		return;

#if 1
	for (i = 0, x=320/2-114; i < 7; i++, x+=33)
	{
		if ((sb_hexen2_cur_item-3+i+30)%15 == sb_hexen2_cur_item)
			Sbar_DrawTransPic(x+9, y-12, Draw_CachePic("gfx/artisel.lmp"));
		Sbar_Hexen2DrawItem(pnum, x, y, (sb_hexen2_cur_item-3+i+30)%15);
	}
#else
	for (i = 0, x=320/2; i < 3; i++, x+=33)
	{
		Sbar_Hexen2DrawItem(pnum, x, y, (sb_hexen2_cur_item+1+i)%15);
	}
	for (i = 0, x=320/2-33; i < 3; i++, x-=33)
	{
		Sbar_Hexen2DrawItem(pnum, x, y, (sb_hexen2_cur_item-1-i+45)%15);
	}
#endif
}

void Sbar_Hexen2DrawExtra (int pnum)
{
	unsigned int i, slot;
	unsigned int pclass;
	int ringpos[] = {6, 44, 81, 119};
	char *ringimages[] = {"gfx/ring_f.lmp", "gfx/ring_w.lmp", "gfx/ring_t.lmp", "gfx/ring_r.lmp"};
	float val;
	char *pclassname[] = {
		"Unknown",
		"Paladin",
		"Crusader",	
		"Necromancer",
		"Assasin",
		"Demoness"
	};

	if (!sb_hexen2_extra_info)
	{
		sbar_rect.y -= 46-SBAR_HEIGHT;
		return;
	}

	pclass = cl.stats[pnum][STAT_H2_PLAYERCLASS];
	if (pclass >= sizeof(pclassname)/sizeof(pclassname[0]))
		pclass = sizeof(pclassname)/sizeof(pclassname[0]) - 1;


	//adjust it so there's space
	sbar_rect.y -= 46+98-SBAR_HEIGHT;

	Sbar_DrawPic(0, 46, Draw_SafeCachePic("gfx/btmbar1.lmp"));
	Sbar_DrawPic(160, 46, Draw_SafeCachePic("gfx/btmbar2.lmp"));

	Sbar_DrawTinyString (11, 48, pclassname[pclass]);

	Sbar_DrawTinyString (11, 58, va("int"));
	Sbar_DrawTinyString (33, 58, va("%02d", cl.stats[pnum][STAT_H2_INTELLIGENCE]));

	Sbar_DrawTinyString (11, 64, va("wis"));
	Sbar_DrawTinyString (33, 64, va("%02d", cl.stats[pnum][STAT_H2_WISDOM]));

	Sbar_DrawTinyString (11, 70, va("dex"));
	Sbar_DrawTinyString (33, 70, va("%02d", cl.stats[pnum][STAT_H2_DEXTERITY]));

	
	Sbar_DrawTinyString (58, 58, va("str"));
	Sbar_DrawTinyString (80, 58, va("%02d", cl.stats[pnum][STAT_H2_STRENGTH]));

	Sbar_DrawTinyString (58, 64, va("lvl"));
	Sbar_DrawTinyString (80, 64, va("%02d", cl.stats[pnum][STAT_H2_LEVEL]));

	Sbar_DrawTinyString (58, 70, va("exp"));
	Sbar_DrawTinyString (80, 70, va("%06d", cl.stats[pnum][STAT_H2_EXPERIENCE]));

	Sbar_DrawTinyString (11, 79, va("abilities"));
	if (cl.stats[pnum][STAT_H2_FLAGS] & (1<<22))
		Sbar_DrawTinyString (8, 89, va("ability 1"));
	if (cl.stats[pnum][STAT_H2_FLAGS] & (1<<23))
		Sbar_DrawTinyString (8, 96, va("ability 2"));

	for (i = 0; i < 4; i++)
	{
		if (cl.stats[pnum][STAT_H2_ARMOUR1+i] > 0)
		{
			Sbar_DrawPic (164+i*40, 115, Draw_SafeCachePic(va("gfx/armor%d.lmp", i+1)));
			Sbar_DrawTinyString (168+i*40, 136, va("+%d", cl.stats[pnum][STAT_H2_ARMOUR1+i]));
		}
	}
	for (i = 0; i < 4; i++)
	{
		if (cl.stats[pnum][STAT_H2_FLIGHT_T+i] > 0)
		{
			Sbar_DrawPic (ringpos[i], 119, Draw_SafeCachePic(va("gfx/ring_f.lmp", i+1)));
			val = cl.stats[pnum][STAT_H2_FLIGHT_T+i];
			if (val > 100)
				val = 100;
			if (val < 0)
				val = 0;
			Sbar_DrawPic(ringpos[i]+29 - (int)(26 * (val/(float)100)),142,Draw_CachePic("gfx/ringhlth.lmp"));
			Sbar_DrawPic(ringpos[i]+29, 142, Draw_CachePic("gfx/rhlthcvr.lmp"));
		}
	}

	slot = 0;
	for (i = 0; i < 8; i++)
	{
		if (cl.statsstr[pnum][STAT_H2_PUZZLE1+i])
		{
			Sbar_DrawPic (194+(slot%4)*31, slot<4?51:82, Draw_SafeCachePic(va("gfx/puzzle/%s.lmp", cl.statsstr[pnum][STAT_H2_PUZZLE1+i])));
			slot++;
		}
	}

	Sbar_DrawPic(134, 50, Draw_SafeCachePic(va("gfx/cport%d.lmp", pclass)));
}

void Sbar_Hexen2DrawBasic(int pnum)
{
	int chainpos;
	int val, maxval;
	Sbar_DrawPic(0, 0, Draw_CachePic("gfx/topbar1.lmp"));
	Sbar_DrawPic(160, 0, Draw_CachePic("gfx/topbar2.lmp"));
	Sbar_DrawTransPic(0, -23, Draw_CachePic("gfx/topbumpl.lmp"));
	Sbar_DrawTransPic(138, -8, Draw_CachePic("gfx/topbumpm.lmp"));
	Sbar_DrawTransPic(269, -23, Draw_CachePic("gfx/topbumpr.lmp"));

	//mana1
	maxval = cl.stats[pnum][STAT_H2_MAXMANA];
	val = cl.stats[pnum][STAT_H2_BLUEMANA];
	val = bound(0, val, maxval);
	Sbar_DrawTinyString(201, 22, va("%03d", val));
	if(val)
	{
		Sbar_DrawPic(190, 26-(int)((val*18.0)/(float)maxval+0.5), Draw_CachePic("gfx/bmana.lmp"));
		Sbar_DrawPic(190, 27, Draw_CachePic("gfx/bmanacov.lmp"));
	}

	//mana2
	maxval = cl.stats[pnum][STAT_H2_MAXMANA];
	val = cl.stats[pnum][STAT_H2_GREENMANA];
	val = bound(0, val, maxval);
	Sbar_DrawTinyString(243, 22, va("%03d", val));
	if(val)
	{
		Sbar_DrawPic(232, 26-(int)((val*18.0)/(float)maxval+0.5), Draw_CachePic("gfx/gmana.lmp"));
		Sbar_DrawPic(232, 27, Draw_CachePic("gfx/gmanacov.lmp"));
	}


	//health
	val = cl.stats[pnum][STAT_HEALTH];
	if (val < -99)
		val = -99;
	Sbar_Hexen2DrawNum(58, 14, val, 3);

	//armour
	val = 0;
	Sbar_Hexen2DrawNum(105, 14, val, 2);

//	SetChainPosition(cl.v.health, cl.v.max_health);
	chainpos = (195.0f*cl.stats[pnum][STAT_HEALTH]) / cl.stats[pnum][STAT_H2_MAXHEALTH];
	if (chainpos < 0)
		chainpos = 0;
	Sbar_DrawPic(45+((int)chainpos&7), 38, Draw_CachePic("gfx/hpchain.lmp"));
	Sbar_DrawPic(45+(int)chainpos, 36,	Draw_CachePic("gfx/hpgem.lmp"));
	Sbar_DrawPic(43, 36, Draw_CachePic("gfx/chnlcov.lmp"));
	Sbar_DrawPic(267, 36, Draw_CachePic("gfx/chnrcov.lmp"));


	Sbar_Hexen2DrawItem(pnum, 144, 3, sb_hexen2_cur_item);
}



void Sbar_DrawTeamStatus(void)
{
	int p;
	int y;
	int track;

	if (!sbar_teamstatus.value)
		return;
	y = -32;

	track = Cam_TrackNum(0);
	if (track == -1 || !cl.spectator)
		track = cl.playernum[0];

	for (p = 0; p < MAX_CLIENTS; p++)
	{
		if (cl.playernum[0] == p)	//self is not shown
			continue;
		if (track == p)	//nor is the person you are tracking
			continue;

		if (!*cl.players[p].teamstatus)	//only show them if they have something. no blank lines thanks
			continue;
		if (strcmp(cl.players[p].team, cl.players[track].team))
			continue;

		if (*cl.players[p].name)
		{
			Sbar_DrawFunString (0, y, cl.players[p].teamstatus);
			y-=8;
		}
	}
	sbar_parsingteamstatuses = true;
}

qboolean Sbar_UpdateTeamStatus(player_info_t *player, char *status)
{
	qboolean aswhite = false;
	char *outb;
	int outlen;
	char *msgstart;
	char *ledstatus;

	if (*status != '\r')// && !(strchr(status, 0x86) || strchr(status, 0x87) || strchr(status, 0x88) || strchr(status, 0x89)))
	{
		if (*status != 'x' || status[1] != '\r')
			return false;
		status++;
	}

	if (*status == '\r')
	{
		while (*status == ' ' || *status == '\r')
			status++;
		ledstatus = status;
		if (*(unsigned char*)ledstatus >= 0x86 && *(unsigned char*)ledstatus <= 0x89)
		{
			msgstart = strchr(status, ':');
			if (!status)
				return false;
			if (msgstart)
				status = msgstart+1;
			else
				ledstatus = NULL;
		}
		else
				ledstatus = NULL;
	}
	else
		ledstatus = NULL;

	while (*status == ' ' || *status == '\r')
		status++;

	//fixme: handle { and } stuff (assume red?)
	outb = player->teamstatus;
	outlen = sizeof(player->teamstatus)-1;
	if (ledstatus)
	{
		*outb++ = *ledstatus;
		outlen--;
	}

	while(outlen>0 && *status)
	{
		if (*status == '{')
		{
			aswhite=true;
			status++;
			continue;
		}
		if (aswhite)
		{
			if (*status == '}')
			{
				aswhite = false;
				status++;
				continue;
			}
			*outb++ = *status++;
		}
		else
			*outb++ = *status++|128;
		outlen--;
	}

	*outb = '\0';

	if (sbar_teamstatus.value == 2)
		return sbar_parsingteamstatuses;
	return false;
}

/*
===============
Sbar_Draw
===============
*/
void Sbar_Draw (void)
{
	qboolean headsup;
	char st[512];
	int pnum;



	headsup = !(cl_sbar.value || (scr_viewsize.value<100&&cl.splitclients==1));
	if ((sb_updates >= vid.numpages) && !headsup)
		return;

	sbar_parsingteamstatuses = false;

#ifdef HLCLIENT
	if (CLHL_DrawHud())
		return;
#endif

#ifdef Q2CLIENT
	if (cls.protocol == CP_QUAKE2)
	{
		SCR_VRectForPlayer(&sbar_rect, 0);

		if (*cl.q2statusbar)
			Sbar_ExecuteLayoutString(cl.q2statusbar);
		if (*cl.q2layout)
		{
			if (cl.q2frame.playerstate.stats[Q2STAT_LAYOUTS])
				Sbar_ExecuteLayoutString(cl.q2layout);
		}
		return;
	}
#endif

	Sbar_Start();

	for (pnum = 0; pnum < cl.splitclients; pnum++)
	{
		if (cl.splitclients>1 || scr_chatmode)
		{
			SCR_VRectForPlayer(&sbar_rect, pnum);
		}
		else
		{	//single player sbar takes full screen

			extern cvar_t scr_centersbar;

			sbar_rect.width = vid.width;
			sbar_rect.height = vid.height;
			sbar_rect.x = 0;
			sbar_rect.y = 0;

			if (scr_centersbar.value)
			{
				sbar_rect.x = (vid.width - 320)/2;
				sbar_rect.width -= sbar_rect.x;
			}
		}

		if (sbar_hexen2)
		{
			Sbar_Hexen2DrawExtra(pnum);
			Sbar_Hexen2DrawBasic(pnum);
			Sbar_Hexen2DrawInventory(pnum);

			if (cl.deathmatch)
				Sbar_MiniDeathmatchOverlay ();
			continue;
		}

		if (sbarfailed)	//files failed to load.
		{
			if (cl.stats[pnum][STAT_HEALTH] <= 0)	//when dead, show nothing
				continue;

			if (scr_viewsize.value != 120)
				Cvar_Set(&scr_viewsize, "120");

			Sbar_DrawString (0, -8, va("Health: %i", cl.stats[pnum][STAT_HEALTH]));
			Sbar_DrawString (0, -16, va(" Armor: %i", cl.stats[pnum][STAT_ARMOR]));

			if (cl.stats[pnum][STAT_H2_BLUEMANA])
				Sbar_DrawString (0, -24, va("  Blue: %i", cl.stats[pnum][STAT_H2_BLUEMANA]));
			if (cl.stats[pnum][STAT_H2_GREENMANA])
				Sbar_DrawString (0, -32, va(" Green: %i", cl.stats[pnum][STAT_H2_GREENMANA]));

			continue;
		}

		sb_updates++;

	// top line
		if (sb_lines > 24)
		{
			if (!cl.spectator || autocam[pnum] == CAM_TRACK)
				Sbar_DrawInventory (pnum);
			if ((!headsup || sbar_rect.width<512) && cl.deathmatch)
				Sbar_DrawFrags ();
		}

	// main area
		if (sb_lines > 0)
		{
			if (cl.spectator)
			{
				if (autocam[pnum] != CAM_TRACK)
				{
					Sbar_DrawPic (0, 0, sb_scorebar);
					Sbar_DrawString (160-7*8,4, "SPECTATOR MODE");
					Sbar_DrawString(160-14*8+4, 12, "Press [ATTACK] for AutoCamera");
				}
				else
				{
					if (sb_showscores || cl.stats[pnum][STAT_HEALTH] <= 0)
						Sbar_SoloScoreboard ();
					else if (cls.gamemode != GAME_DEATHMATCH)
						Sbar_CoopScoreboard ();
					else
						Sbar_DrawNormal (pnum);

					if (hud_tracking_show.value)
					{
						sprintf(st, "Tracking %-.64s",
							cl.players[spec_track[pnum]].name);
						Sbar_DrawFunString(0, -8, st);
					}
				}
			}
			else if (sb_showscores || (cl.stats[pnum][STAT_HEALTH] <= 0 && cl.splitclients == 1))
			{
				if (!pnum)
				{
					if (cls.gamemode != GAME_DEATHMATCH)
						Sbar_CoopScoreboard ();
					else
						Sbar_SoloScoreboard ();
				}
			}
			else
				Sbar_DrawNormal (pnum);
		}
	}

#ifdef RGLQUAKE
	if (cl_sbar.value == 1 || scr_viewsize.value<100)
	{
		if (cl.splitclients==1 && sbar_rect.x>0)
		{	// left
				Draw_TileClear (0, sbar_rect.height - sb_lines, sbar_rect.x, sb_lines);
		}
		if (sbar_rect.x + 320 <= sbar_rect.width && !headsup)
			Draw_TileClear (sbar_rect.x + 320, sbar_rect.height - sb_lines, sbar_rect.width - (320), sb_lines);
	}
#endif


	if (sb_lines > 0)
		Sbar_DrawTeamStatus();

	if (sb_lines > 0 && cl.deathmatch)
		Sbar_MiniDeathmatchOverlay ();

	{
		extern int scr_chatmode;
		if (scr_chatmode)
			Sbar_ChatModeOverlay();
	}
}

//=============================================================================

/*
==================
Sbar_IntermissionNumber

==================
*/
void Sbar_IntermissionNumber (int x, int y, int num, int digits, int color)
{
	char			str[12];
	char			*ptr;
	int				l, frame;

	l = Sbar_itoa (num, str);
	ptr = str;
	if (l > digits)
		ptr += (l-digits);
	if (l < digits)
		x += (digits-l)*24;

	while (*ptr)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		Draw_TransPic (x,y,sb_nums[color][frame]);
		x += 24;
		ptr++;
	}
}

/*
==================
Sbar_TeamOverlay

team frags
added by Zoid
==================
*/
void Sbar_TeamOverlay (void)
{
	mpic_t			*pic;
	int				i, k, l;
	int				x, y;
	char			num[12];
	char			team[5];
	team_t *tm;
	int plow, phigh, pavg;

// request new ping times every two second
	if (!cl.teamplay) {
		Sbar_DeathmatchOverlay(0);
		return;
	}

	y = 0;

	if (scr_scoreboard_drawtitle.value)
	{
		pic = Draw_SafeCachePic ("gfx/ranking.lmp");
		if (pic)
			Draw_Pic ((vid.width-pic->width)/2, 0, pic);
		y += 24;
	}

	x = (vid.width - 320)/2 + 36;
	Draw_String(x, y, "low/avg/high team total players");
	y += 8;
//	Draw_String(x, y, "------------ ---- ----- -------");
	Draw_String(x, y, "\x1d\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1f \x1d\x1e\x1e\x1f \x1d\x1e\x1e\x1e\x1f \x1d\x1e\x1e\x1e\x1e\x1e\x1f");
	y += 8;

// sort the teams
	Sbar_SortTeams();

// draw the text
	l = scoreboardlines;

	for (i=0 ; i < scoreboardteams && y <= vid.height-10 ; i++)
	{
		k = teamsort[i];
		tm = teams + k;

	// draw pings
		plow = tm->plow;
		if (plow < 0 || plow > 999)
			plow = 999;
		phigh = tm->phigh;
		if (phigh < 0 || phigh > 999)
			phigh = 999;
		if (!tm->players)
			pavg = 999;
		else
			pavg = tm->ptotal / tm->players;
		if (pavg < 0 || pavg > 999)
			pavg = 999;

		sprintf (num, "%3i/%3i/%3i", plow, pavg, phigh);
		Draw_String ( x, y, num);

	// draw team
		Q_strncpyz (team, tm->team, sizeof(team));
		Draw_String (x + 104, y, team);

	// draw total
		sprintf (num, "%5i", tm->frags);
		Draw_String (x + 104 + 40, y, num);

	// draw players
		sprintf (num, "%5i", tm->players);
		Draw_String (x + 104 + 88, y, num);

		if (!strncmp(cl.players[cl.playernum[0]].team, tm->team, 16)) {
			Draw_Character ( x + 104 - 8, y, 16);
			Draw_Character ( x + 104 + 32, y, 17);
		}

		y += 8;
	}
	y += 8;
	Sbar_DeathmatchOverlay(y);
}

/*
==================
Sbar_DeathmatchOverlay

ping time frags name
==================
*/

//for reference:
//define COLUMN(title, width, code)

#define COLUMN_PING COLUMN(ping, 4*8,					\
{														\
	int p = s->ping;									\
	if (p < 0 || p > 999) p = 999;						\
	sprintf(num, "%4i", p);								\
	Draw_FunString(x, y, num);							\
})

#define COLUMN_PL COLUMN(pl, 2*8,						\
{														\
	int p = s->pl;										\
	sprintf(num, "%2i", p);								\
	Draw_FunString(x, y, num);							\
})
#define COLUMN_TIME COLUMN(time, 4*8,					\
{														\
	if (cl.intermission)								\
		total = cl.completed_time - s->entertime;		\
	else												\
		total = cl.servertime - s->entertime;			\
	minutes = (int)total/60;							\
	sprintf (num, "%4i", minutes);						\
	Draw_String ( x , y, num);							\
})
#define COLUMN_FRAGS COLUMN(frags, 5*8,					\
{														\
	if (s->spectator)									\
	{													\
		if (cl.teamplay)								\
			Draw_String( x, y, "spectator" );			\
		else											\
			Draw_String( x, y, "spec" );				\
	}													\
	else												\
	{													\
		if (largegame)									\
			Sbar_FillPC ( x, y+1, 40, 3, top);			\
		else											\
			Sbar_FillPC ( x, y, 40, 4, top);				\
		Sbar_FillPC ( x, y+4, 40, 4, bottom);				\
														\
		f = s->frags;									\
		sprintf (num, "%3i",f);							\
														\
		Draw_Character ( x+8 , y, num[0]);				\
		Draw_Character ( x+16 , y, num[1]);				\
		Draw_Character ( x+24 , y, num[2]);				\
														\
		if ((cl.spectator && k == spec_track[0]) ||		\
			(!cl.spectator && k == cl.playernum[0]))	\
		{												\
			Draw_Character ( x, y, 16);					\
			Draw_Character ( x + 32, y, 17);			\
		}												\
	}													\
														\
})
#define COLUMN_TEAMNAME COLUMN(team, 4*8,				\
{														\
	if (!s->spectator)									\
	{													\
		Draw_FunStringLen(x, y, s->team, 4);			\
	}													\
})
#define COLUMN_NAME COLUMN(name, (cl.teamplay ? 12*8 : 16*8),	{Draw_FunString(x, y, s->name);})
#define COLUMN_KILLS COLUMN(kils, 4*8, {Draw_FunString(x, y, va("%4i", Stats_GetKills(k)));})
#define COLUMN_TKILLS COLUMN(tkil, 4*8, {Draw_FunString(x, y, va("%4i", Stats_GetTKills(k)));})
#define COLUMN_DEATHS COLUMN(dths, 4*8, {Draw_FunString(x, y, va("%4i", Stats_GetDeaths(k)));})
#define COLUMN_TOUCHES COLUMN(tchs, 4*8, {Draw_FunString(x, y, va("%4i", Stats_GetTouches(k)));})
#define COLUMN_CAPS COLUMN(caps, 4*8, {Draw_FunString(x, y, va("%4i", Stats_GetCaptures(k)));})



//columns are listed here in display order
#define ALLCOLUMNS COLUMN_PING COLUMN_PL COLUMN_TIME COLUMN_FRAGS COLUMN_TEAMNAME COLUMN_NAME COLUMN_KILLS COLUMN_TKILLS COLUMN_DEATHS COLUMN_TOUCHES COLUMN_CAPS

enum
{
#define COLUMN(title, width, code) COLUMN##title,
	ALLCOLUMNS
#undef COLUMN
	COLUMN_MAX
};

#define ADDCOLUMN(id) showcolumns |= (1<<id)
void Sbar_DeathmatchOverlay (int start)
{
	mpic_t			*pic;
	int				i, k, l;
	int				x, y, f;
	char			num[12];
	player_info_t	*s;
	int				total;
	int				minutes;
	int				skip = 10;
	int showcolumns;
	int startx, rank_width;

	if (largegame)
		skip = 8;

// request new ping times every two second
	if (realtime - cl.last_ping_request > 2	&& cls.protocol == CP_QUAKEWORLD && cls.demoplayback != DPB_EZTV)
	{
		cl.last_ping_request = realtime;
		CL_SendClientCommand(true, "pings");
	}

	if (start)
		y = start;
	else
	{
		y = 0;
		if (scr_scoreboard_drawtitle.value)
		{
			pic = Draw_SafeCachePic ("gfx/ranking.lmp");
			if (pic)
				Draw_Pic ((vid.width - 320)/2 + 160-pic->width/2, 0, pic);
			y += 24;
		}
	}

// scores
	Sbar_SortFrags(true);

// draw the text
	l = scoreboardlines;

	if (start)
		y = start;
	else
		y = 24;

	if (scr_scoreboard_newstyle.value)
	{
		// Electro's scoreboard eyecandy: Increase to fit the new scoreboard
		y += 8;
	}

	showcolumns = 0;

	rank_width = 0;

#define COLUMN(title, cwidth, code) if (rank_width+(cwidth)+8 <= vid.width) {showcolumns |= (1<<COLUMN##title); rank_width += cwidth+8;}
//columns are listed here in priority order (if the screen is too narrow, later ones will be hidden)
	COLUMN_NAME
	COLUMN_PING
	COLUMN_PL
	COLUMN_TIME
	COLUMN_FRAGS
	if (cl.teamplay)
	{
		COLUMN_TEAMNAME
	}
	if (cl.teamplay && Stats_HaveFlags())
	{
		COLUMN_CAPS
	}
	if (scr_scoreboard_showfrags.value && Stats_HaveKills())
	{
		COLUMN_KILLS
		COLUMN_DEATHS
		if (cl.teamplay)
		{
			COLUMN_TKILLS
		}
	}
	if (cl.teamplay && Stats_HaveFlags())
	{
		COLUMN_TOUCHES
	}
#undef COLUMN

	startx = (vid.width-rank_width)/2;

	if (scr_scoreboard_newstyle.value)
	{
		// Electro's scoreboard eyecandy: Draw top border
		Draw_Fill(startx - 3, y - 1, rank_width - 1, 1, 0);

		// Electro's scoreboard eyecandy: Draw the title row background
		Draw_Fill(startx - 2, y, rank_width - 3, 9, 1);
	}

	x = startx;
#define COLUMN(title, width, code) if (showcolumns & (1<<COLUMN##title)) {Draw_FunString(x, y, #title); x += width+8;}
	ALLCOLUMNS
#undef COLUMN


	y += 8;

	if (scr_scoreboard_titleseperator.value && !scr_scoreboard_newstyle.value)
	{
		x = startx;
#define COLUMN(title, width, code) \
if (showcolumns & (1<<COLUMN##title)) \
{ \
	Draw_String(x, y, "\x1d"); \
	for (i = 8; i < width-8; i+= 8) \
		Draw_String(x+i, y, "\x1e"); \
	Draw_String(x+i, y, "\x1f"); \
	x += width+8; \
}
		ALLCOLUMNS
#undef COLUMN
		y += 8;
	}

	if (scr_scoreboard_newstyle.value)
	{
		// Electro's scoreboard eyecandy: Draw top border (under header)
		Draw_Fill (startx - 3, y + 1, rank_width - 1, 1, 0);
		// Electro's scoreboard eyecandy: Don't go over the black border, move the rest down
		y += 2;
		// Electro's scoreboard eyecandy: Draw left border
		Draw_Fill (startx - 3, y - 10, 1, 9, 0);
		// Electro's scoreboard eyecandy: Draw right border
		Draw_Fill (startx - 3 + rank_width - 2, y - 10, 1, 9, 0);
	}

	y -= skip;

	for (i = 0; i < scoreboardlines; i++)
	{
		char	team[5];
		unsigned int		top, bottom;

		// TODO: Sort players so that the leading teams are drawn first
		k = fragsort[i];
		s = &cl.players[k];
		if (!s->name[0])
			continue;

		y += skip;
		if (y > vid.height-10)
			break;

		// Electro's scoreboard eyecandy: Moved this up here for usage with the row background color
		top = Sbar_TopColour(s);
		bottom = Sbar_BottomColour(s);

		if (scr_scoreboard_newstyle.value)
		{
			// Electro's scoreboard eyecandy: Render the main background transparencies behind players row
			// TODO: Alpha values on the background
			if ((cl.teamplay) && (!s->spectator))
			{
				int background_color;
				// Electro's scoreboard eyecandy: red vs blue are common teams, force the colours
				Q_strncpyz (team, Info_ValueForKey(s->userinfo, "team"), sizeof(team));

				if (!(strcmp("red", team)))
					background_color = 4; // forced red
				else if (!(strcmp("blue", team)))
					background_color = 13; // forced blue
				else
					background_color = bottom;

				Sbar_FillPCDark (startx - 2, y, rank_width - 3, skip, background_color);
			}
			else
				Draw_Fill (startx - 2, y, rank_width - 3, skip, 2);

			Draw_Fill (startx - 3, y, 1, skip, 0); // Electro - Border - Left
			Draw_Fill (startx - 3 + rank_width - 2, y, 1, skip, 0); // Electro - Border - Right
		}

		x = startx;
#define COLUMN(title, width, code) \
if (showcolumns & (1<<COLUMN##title)) \
{ \
	code \
	x += width+8; \
}
		ALLCOLUMNS
#undef COLUMN
	}

	if (scr_scoreboard_newstyle.value)
		Draw_Fill (startx - 3, y + skip, rank_width - 1, 1, 0); // Electro - Border - Bottom

/*
	if (cl.teamplay)
	{
		if (scr_chatmode)
			x = vid.width/2 + ((int)vid.width/2 - 320)/2 + 4;
		else
			x = ((int)vid.width - 320)/2 + 4;
//                            0    40 64   104   152  192
		Draw_String ( x , y, "ping pl time frags team name");
		y += 8;
//		Draw_String ( x , y, "---- -- ---- ----- ---- ----------------");
		Draw_String ( x , y, "\x1d\x1e\x1e\x1f \x1d\x1f \x1d\x1e\x1e\x1f \x1d\x1e\x1e\x1e\x1f \x1d\x1e\x1e\x1f \x1d\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1f");
		y += 8;
	}
	else
	{
		if (scr_chatmode)
			x = vid.width/2 + ((int)vid.width/2 - 320)/2 + 16;
		else
			x = ((int)vid.width - 320)/2 + 16;
//                            0    40 64   104   152
		Draw_String ( x , y, "ping pl time frags name");
		y += 8;
//		Draw_String ( x , y, "---- -- ---- ----- ----------------");
		Draw_String ( x , y, "\x1d\x1e\x1e\x1f \x1d\x1f \x1d\x1e\x1e\x1f \x1d\x1e\x1e\x1e\x1f \x1d\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1f");
		y += 8;
	}

	for (i=0 ; i<l && y <= vid.height-10 ; i++)
	{
		k = fragsort[i];
		s = &cl.players[k];
		if (!s->name[0])
			continue;

		// draw ping
		p = s->ping;
		if (p < 0 || p > 999)
			p = 999;
		sprintf (num, "%4i", p);
		Draw_FunString ( x, y, num);

		// draw pl
		p = s->pl;
		sprintf (num, "%3i", p);
		if (p > 25)
			Draw_Alt_String ( x+32, y, num);
		else
			Draw_String ( x+32, y, num);

		if (s->spectator)
		{
			Draw_String (x+56, y, "(spectator)");
			// draw name
			if (cl.teamplay)
				Draw_FunString (x+152+40, y, s->name);
			else
				Draw_FunString (x+152, y, s->name);
			y += skip;
			continue;
		}


		// draw time
		if (cl.intermission)
			total = cl.completed_time - s->entertime;
		else
			total = cl.servertime - s->entertime;
		minutes = (int)total/60;
		sprintf (num, "%4i", minutes);
		Draw_String ( x+64 , y, num);

		// draw background
		top = s->topcolor;
		bottom = s->bottomcolor;
		top = Sbar_ColorForMap (top);
		bottom = Sbar_ColorForMap (bottom);

		if (largegame)
			Draw_Fill ( x+104, y+1, 40, 3, top);
		else
			Draw_Fill ( x+104, y, 40, 4, top);
		Draw_Fill ( x+104, y+4, 40, 4, bottom);

	// draw number
		f = s->frags;
		sprintf (num, "%3i",f);

		Draw_Character ( x+112 , y, num[0]);
		Draw_Character ( x+120 , y, num[1]);
		Draw_Character ( x+128 , y, num[2]);

		if ((cl.spectator && k == spec_track[0]) ||
			(!cl.spectator && k == cl.playernum[0]))
		{
			Draw_Character ( x + 104, y, 16);
			Draw_Character ( x + 136, y, 17);
		}

		// team
		if (cl.teamplay)
			Draw_FunStringLen (x+152, y, s->team, 4);

		// draw name
		if (cl.teamplay)
			Draw_FunString (x+152+40, y, s->name);
		else
			Draw_FunString (x+152, y, s->name);

		y += skip;
	}
*/
	Draw_Character(0,0,' ' | CON_WHITEMASK);

	if (y >= vid.height-10) // we ran over the screen size, squish
		largegame = true;
}

void Sbar_ChatModeOverlay(void)
{
	int start =0;
	int				i, k, l;
	int				top, bottom;
	int				x, y;
	player_info_t	*s;
	char			team[5];
	int				skip = 10;

	if (largegame)
		skip = 8;

// request new ping times every two second
	if (realtime - cl.last_ping_request > 2 && cls.protocol == CP_QUAKEWORLD && cls.demoplayback != DPB_EZTV)
	{
		cl.last_ping_request = realtime;
		CL_SendClientCommand(true, "pings");
	}

// scores
	Sbar_SortFrags (true);

	if (Cam_TrackNum(0)>=0)
		Q_strncpyz (team, cl.players[Cam_TrackNum(0)].team, sizeof(team));
	else if (cl.playernum[0]>=0 && cl.playernum[0]<MAX_CLIENTS)
		Q_strncpyz (team, cl.players[cl.playernum[0]].team, sizeof(team));
	else
		*team = '\0';

// draw the text
	l = scoreboardlines;

	if (start)
		y = start;
	else
		y = 24;
	y = vid.height/2;

	x = 4;
	Draw_String ( x , y, "name");
	y += 8;
	Draw_String ( x , y, "\x1d\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1e\x1f");
	y += 8;

	for (i=0 ; i<l && y <= vid.height-10 ; i++)
	{
		k = fragsort[i];
		s = &cl.players[k];
		if (!s->name[0])
			continue;

		// draw background
		top = Sbar_TopColour(s);
		bottom = Sbar_BottomColour(s);

		if (largegame)
			Sbar_FillPC ( x, y+1, 8*4, 3, top);
		else
			Sbar_FillPC ( x, y, 8*4, 4, top);
		Sbar_FillPC ( x, y+4, 8*4, 4, bottom);

		if (cl.spectator && k == Cam_TrackNum(0))
		{
			Draw_Character ( x, y, 16);
			Draw_Character ( x+8*3, y, 17);
		}
		else if (!cl.spectator && k == cl.playernum[0])
		{
			Draw_Character ( x, y, 16);
			Draw_Character ( x+8*3, y, 17);
		}
		else if (cl.teamplay)
		{
			if (!stricmp(s->team, team))
			{
				Draw_Character ( x, y, '[');
				Draw_Character ( x+8*3, y, ']');
			}
		}

		// draw name
		if (cl.teamplay)
			Draw_FunString (x+8*4, y, s->name);
		else
			Draw_FunString (x+8*4, y, s->name);

		y += skip;
	}

	Draw_Character(0,0,' ' | CON_WHITEMASK);

	if (y >= vid.height-10) // we ran over the screen size, squish
		largegame = true;
}

/*
==================
Sbar_MiniDeathmatchOverlay

frags name
frags team name
displayed to right of status bar if there's room
==================
*/
void Sbar_MiniDeathmatchOverlay (void)
{
	int				i, k;
	int				top, bottom;
	int				x, y, f;
	char			num[12];
	player_info_t	*s;
	int				numlines;
	char			name[64+1];
	team_t			*tm;

	if (sbar_rect.width < 512 || !sb_lines)
		return; // not enuff room

// scores
	Sbar_SortFrags (false);
	if (sbar_rect.width >= 640)
		Sbar_SortTeams();

	if (!scoreboardlines)
		return; // no one there?

// draw the text
	y = sbar_rect.height - sb_lines - 1;
	numlines = sb_lines/8;
	if (numlines < 3)
		return; // not enough room

	// find us
	for (i=0 ; i < scoreboardlines; i++)
		if (fragsort[i] == cl.playernum[0])
			break;

	if (i == scoreboardlines) // we're not there, we are probably a spectator, just display top
		i = 0;
	else // figure out start
		i = i - numlines/2;

	if (i > scoreboardlines - numlines)
		i = scoreboardlines - numlines;
	if (i < 0)
		i = 0;

	x = 324;

	for (/* */ ; i < scoreboardlines && y < sbar_rect.height - 8 + 1; i++)
	{
		k = fragsort[i];
		s = &cl.players[k];
		if (!s->name[0])
			continue;

	// draw ping
		top = Sbar_TopColour(s);
		bottom = Sbar_BottomColour(s);

		Sbar_FillPC ( x, y+1, 40, 3, top);
		Sbar_FillPC ( x, y+4, 40, 4, bottom);

	// draw number
		f = s->frags;
		sprintf (num, "%3i",f);

		Draw_ColouredCharacter ( x+8 , y, CON_WHITEMASK|num[0]);
		Draw_Character ( x+16, y, num[1]);
		Draw_Character ( x+24, y, num[2]);

		if ((cl.spectator && k == spec_track[0]) ||
			(!cl.spectator && k == cl.playernum[0]))
		{
			Draw_Character ( x, y, 16);
			Draw_Character ( x + 32, y, 17);
		}

		Q_strncpyz(name, s->name, sizeof(name));
	// team and name
		if (cl.teamplay)
		{
			Draw_FunStringLen (x+48, y, s->team, 4);
			Draw_FunStringLen (x+48+40, y, name, MAX_DISPLAYEDNAME);
		}
		else
			Draw_FunStringLen (x+48, y, name, MAX_DISPLAYEDNAME);
		y += 8;
	}

	// draw teams if room
	if (sbar_rect.width < 640 || !cl.teamplay)
		return;

	// draw seperator
	x += 208;
	for (y = sbar_rect.height - sb_lines; y < sbar_rect.height - 6; y += 2)
		Draw_ColouredCharacter(x, y, CON_WHITEMASK|14);

	x += 16;

	y = sbar_rect.height - sb_lines;
	for (i=0 ; i < scoreboardteams && y <= sbar_rect.height; i++)
	{
		k = teamsort[i];
		tm = teams + k;

	// draw pings
		Draw_FunStringLen (x, y, tm->team, 4);

	// draw total
		sprintf (num, "%5i", tm->frags);
		Draw_FunString(x + 40, y, num);

		if (!strncmp(cl.players[cl.playernum[0]].team, tm->team, 16)) {
			Draw_Character ( x - 8, y, 16);
			Draw_Character ( x + 32, y, 17);
		}

		y += 8;
	}

}

void Sbar_CoopIntermission (void)
{
	mpic_t	*pic;
	int		dig;
	int		num;

	sbar_rect.width = vid.width;
	sbar_rect.height = vid.height;
	sbar_rect.x = 0;
	sbar_rect.y = 0;

	pic = Draw_SafeCachePic ("gfx/complete.lmp");
	if (!pic)
		return;
	Draw_Pic ((sbar_rect.width - 320)/2 + 64, (sbar_rect.height - 200)/2 + 24, pic);

	pic = Draw_SafeCachePic ("gfx/inter.lmp");
	if (pic)
		Draw_TransPic ((sbar_rect.width - 320)/2 + 0, (sbar_rect.height - 200)/2 + 56, pic);

// time
	dig = cl.completed_time/60;
	Sbar_IntermissionNumber ((sbar_rect.width - 320)/2 + 160, (sbar_rect.height - 200)/2 + 64, dig, 3, 0);
	num = cl.completed_time - dig*60;
	Draw_TransPic ((sbar_rect.width - 320)/2 + 234,(sbar_rect.height - 200)/2 + 64,sb_colon);
	Draw_TransPic ((sbar_rect.width - 320)/2 + 246,(sbar_rect.height - 200)/2 + 64,sb_nums[0][num/10]);
	Draw_TransPic ((sbar_rect.width - 320)/2 + 266,(sbar_rect.height - 200)/2 + 64,sb_nums[0][num%10]);

//it is assumed that secrits/monsters are going to be constant for any player...
	Sbar_IntermissionNumber ((sbar_rect.width - 320)/2 + 160, (sbar_rect.height - 200)/2 + 104, cl.stats[0][STAT_SECRETS], 3, 0);
	Draw_TransPic ((sbar_rect.width - 320)/2 + 232,(sbar_rect.height - 200)/2 + 104,sb_slash);
	Sbar_IntermissionNumber ((sbar_rect.width - 320)/2 + 240, (sbar_rect.height - 200)/2 + 104, cl.stats[0][STAT_TOTALSECRETS], 3, 0);

	Sbar_IntermissionNumber ((sbar_rect.width - 320)/2 + 160, (sbar_rect.height - 200)/2 + 144, cl.stats[0][STAT_MONSTERS], 3, 0);
	Draw_TransPic ((sbar_rect.width - 320)/2 + 232,(sbar_rect.height - 200)/2 + 144,sb_slash);
	Sbar_IntermissionNumber ((sbar_rect.width - 320)/2 + 240, (sbar_rect.height - 200)/2 + 144, cl.stats[0][STAT_TOTALMONSTERS], 3, 0);
}
/*
==================
Sbar_IntermissionOverlay

==================
*/
void Sbar_IntermissionOverlay (void)
{
#ifdef VM_UI
	if (UI_DrawIntermission()>0)
		return;
#endif

	Sbar_Start();

	if (cls.gamemode != GAME_DEATHMATCH)
		Sbar_CoopIntermission();
	else if (cl.teamplay > 0 && !sb_showscores)
		Sbar_TeamOverlay ();
	else
		Sbar_DeathmatchOverlay (0);
}


/*
==================
Sbar_FinaleOverlay

==================
*/
void Sbar_FinaleOverlay (void)
{
	mpic_t	*pic;

#ifdef VM_UI
	if (UI_DrawFinale()>0)
		return;
#endif
	pic = Draw_SafeCachePic ("gfx/finale.lmp");
	if (pic)
		Draw_TransPic ( (vid.width-pic->width)/2, 16, pic);
}

