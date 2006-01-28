//read menu.h

#include "quakedef.h"

int omousex;
int omousey;
qboolean mousemoved;
qboolean bindingactive;
extern cvar_t cl_cursor;
extern cvar_t cl_cursorsize;
extern cvar_t cl_cursorbias;

void Draw_TextBox (int x, int y, int width, int lines)
{
	mpic_t	*p;
	int		cx, cy;
	int		n;

	// draw left side
	cx = x;
	cy = y;
	p = Draw_SafeCachePic ("gfx/box_tl.lmp");

	if (!p)	//assume none exist
		return;

	if (p)
		Draw_TransPic (cx, cy, p);
	p = Draw_SafeCachePic ("gfx/box_ml.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		if (p)
			Draw_TransPic (cx, cy, p);
	}
	p = Draw_SafeCachePic ("gfx/box_bl.lmp");
	if (p)
		Draw_TransPic (cx, cy+8, p);

	// draw middle
	cx += 8;
	while (width > 0)
	{
		cy = y;
		p = Draw_SafeCachePic ("gfx/box_tm.lmp");
		if (p)
			Draw_TransPic (cx, cy, p);
		p = Draw_SafeCachePic ("gfx/box_mm.lmp");
		for (n = 0; n < lines; n++)
		{
			cy += 8;
			if (n == 1)
				p = Draw_SafeCachePic ("gfx/box_mm2.lmp");
			if (p)
				Draw_TransPic (cx, cy, p);
		}
		p = Draw_SafeCachePic ("gfx/box_bm.lmp");
		if (p)
			Draw_TransPic (cx, cy+8, p);
		width -= 2;
		cx += 16;
	}

	// draw right side
	cy = y;
	p = Draw_SafeCachePic ("gfx/box_tr.lmp");
	if (p)
		Draw_TransPic (cx, cy, p);
	p = Draw_SafeCachePic ("gfx/box_mr.lmp");
	for (n = 0; n < lines; n++)
	{
		cy += 8;
		if (p)
			Draw_TransPic (cx, cy, p);
	}
	p = Draw_SafeCachePic ("gfx/box_br.lmp");
	if (p)
		Draw_TransPic (cx, cy+8, p);
}

void Draw_BigFontString(int x, int y, const char *text)
{
	int sx, sy;
	mpic_t *p;
	p = Draw_SafeCachePic ("gfx/menu/bigfont.lmp");

	while(*text)
	{
		if (*text >= 'a' && *text <= 'z')
		{
			sx = ((*text-'a')%8)*20;
			sy = ((*text-'a')/8)*20;
		}
		else if (*text >= 'A' && *text <= 'Z')
		{
			sx = ((*text-'A')%8)*20;
			sy = ((*text-'A')/8)*20;
		}
		else// if (*text <= ' ')
		{
			sx=-1;
			sy=-1;
		}
		if(sx>=0)
			Draw_SubPic(x, y, p, sx, sy, 20, 20);
		x+=20;
		text++;
	}
}

char *menudotstyle;
int maxdots;
int mindot;
int dotofs;

void MenuDrawItems(int xpos, int ypos, menuoption_t *option, menu_t *menu)
{
	int i;
	mpic_t *p; 
	while (option)
	{
		if (mousemoved && !bindingactive)
		{
			if (omousex > xpos+option->common.posx && omousex < xpos+option->common.posx+option->common.width)
				if (omousey > ypos+option->common.posy && omousey < ypos+option->common.posy+option->common.height)
				{
					if (menu->selecteditem != option)
					{
						S_LocalSound ("misc/menu1.wav");
						menu->selecteditem = option;
					}
					if (menu->cursoritem)
						menu->cursoritem->common.posy = menu->selecteditem->common.posy;
				}
		}
		if (!option->common.ishidden)
		switch(option->common.type)
		{
		case mt_text:
			if (!option->text.text)
				Draw_Character (xpos+option->common.posx, ypos+option->common.posy, 12+((int)(realtime*4)&1));
			else if (option->text.isred)
				Draw_Alt_String(xpos+option->common.posx, ypos+option->common.posy, option->text.text);
			else
				Draw_String(xpos+option->common.posx, ypos+option->common.posy, option->text.text);
			break;
		case mt_button:
			if (!menu->cursoritem && menu->selecteditem == option)
				Draw_Alt_String(xpos+option->common.posx, ypos+option->common.posy, option->button.text);
			else
				Draw_String(xpos+option->common.posx, ypos+option->common.posy, option->button.text);
			break;
		case mt_buttonbigfont:
			Draw_BigFontString(xpos+option->common.posx, ypos+option->common.posy, option->button.text);
			break;
		case mt_menudot:
			i = (int)(realtime * 10)%maxdots;
			p = Draw_SafeCachePic(va(menudotstyle, i+mindot ));
			Draw_TransPic(xpos+option->common.posx, ypos+option->common.posy+dotofs, p);
			break;
		case mt_picture:
			p = NULL;
			if (menu->selecteditem && menu->selecteditem->common.posx == option->common.posx && menu->selecteditem->common.posy == option->common.posy)
			{
				char selname[MAX_QPATH];
				Q_strncpyz(selname, option->picture.picturename, sizeof(selname));
				COM_StripExtension(selname, selname);
				p = Draw_SafeCachePic(va("%s_sel", selname));
			}

			if (!p)
				p = Draw_SafeCachePic(option->picture.picturename);

			Draw_TransPic (xpos+option->common.posx, ypos+option->common.posy, p);
			break;
		case mt_childwindow:
			MenuDrawItems(xpos+option->common.posx, ypos+option->common.posy, ((menu_t *)option->custom.data)->options, (menu_t *)option->custom.data);
			break;
		case mt_box:
			Draw_TextBox(xpos+option->common.posx, ypos+option->common.posy, option->box.width, option->box.height);
			break;
		case mt_slider:
			if (option->slider.var)
			{
#define SLIDER_RANGE 10
				float range;
				int	i;
				int x = xpos+option->common.posx;
				int y = ypos+option->common.posy;

				range = (option->slider.current - option->slider.min)/(option->slider.max-option->slider.min);

				if (option->slider.text)
				{
					if (!menu->cursoritem && menu->selecteditem == option)
						Draw_Alt_String(x, y, option->slider.text);
					else
						Draw_String(x, y, option->slider.text);
					x += strlen(option->slider.text)*8+28;
				}

				if (range < 0)
					range = 0;
				if (range > 1)
					range = 1;
				Draw_Character (x-8, y, 128);
				for (i=0 ; i<SLIDER_RANGE ; i++)
					Draw_Character (x + i*8, y, 129);
				Draw_Character (x+i*8, y, 130);
				Draw_Character (x + (SLIDER_RANGE-1)*8 * range, y, 131);
			}
			break;
		case mt_checkbox:
			{
				int x = xpos+option->common.posx;
				int y = ypos+option->common.posy;
				qboolean on;
				if (option->check.func)
					on = option->check.func(option, CHK_CHECKED);
				else if (!option->check.var)
						on = option->check.value;
				else if (option->check.bits)	//bits is a bitmask for use with cvars (users can be clumsy, so bittage of 0 uses non-zero as true, but sets only bit 1)
				{
					if (option->check.var->latched_string)
						on = atoi(option->check.var->latched_string)&option->check.bits;
					else
						on = (int)(option->check.var->value)&option->check.bits;
				}
				else
				{
					if (option->check.var->latched_string)
						on = !!atof(option->check.var->latched_string);
					else
						on = !!option->check.var->value;
				}

				if (option->check.text)
				{
					if (!menu->cursoritem && menu->selecteditem == option)
						Draw_Alt_String(x, y, option->check.text);
					else
						Draw_String(x, y, option->check.text);
					x += strlen(option->check.text)*8+28;
				}
#if 0
				if (on)
					Draw_Character (x, y, 131);
				else
					Draw_Character (x, y, 129);
#endif
				if (on)
					Draw_String (x, y, "on");
				else
					Draw_String (x, y, "off");
			}
			break;
		case mt_edit:
			{
				int x = xpos+option->common.posx;
				int y = ypos+option->common.posy;

				if (!menu->cursoritem && menu->selecteditem == option)
					Draw_Alt_String(x, y, option->edit.caption);
				else
					Draw_String(x, y, option->edit.caption);
				x+=strlen(option->edit.caption)*8+8;
				Draw_TextBox(x-8, y-8, 16, 1);
				Draw_String(x, y, option->edit.text);

				if (menu->selecteditem == option && (int)(realtime*4) & 1)
				{
					x += strlen(option->edit.text)*8;
					Draw_Character(x, y, 11);
				}
			}
			break;
		case mt_bind:
			{
				int x = xpos+option->common.posx;
				int y = ypos+option->common.posy;
				int l;
				int		keys[2];
				char *keyname;

				if (!menu->cursoritem && menu->selecteditem == option)
					Draw_Alt_String(x, y, option->bind.caption);
				else
					Draw_String(x, y, option->bind.caption);
				x += strlen(option->bind.caption)*8+28;
				{
					l = strlen (option->bind.command);
					
					M_FindKeysForCommand (option->bind.command, keys);
					
					if (bindingactive && menu->selecteditem == option)
					{
						Draw_String (x, y, "Press key");
					}
					else if (keys[0] == -1)
					{
						Draw_String (x, y, "???");
					}
					else
					{
						keyname = Key_KeynumToString (keys[0]);
						Draw_String (x, y, keyname);
						x += strlen(keyname) * 8;
						if (keys[1] != -1)
						{
							Draw_String (x + 8, y, "or");
							Draw_String (x + 32, y, Key_KeynumToString (keys[1]));
						}
					}
				}
			}
			break;

		case mt_combo:
			{
				int x = xpos+option->common.posx;
				int y = ypos+option->common.posy;

				Draw_String(x, y, option->combo.caption);
				x += strlen(option->combo.caption)*8+24;
				Draw_String(x, y, option->combo.options[option->combo.selectedoption]);
			}
			break;
		case mt_custom:
			option->custom.draw(xpos+option->common.posx, ypos+option->common.posy, &option->custom, menu);
			break;
		default:
			Sys_Error("Bad item type\n");
			break;
		}
		option = option->common.next;
	}
}

void MenuDraw(menu_t *menu)
{
	if (menu->event)
		menu->event(menu);
	menu->xpos = ((vid.width - 320)>>1);
	MenuDrawItems(menu->xpos, menu->ypos, menu->options, menu);
}


menutext_t *MC_AddWhiteText(menu_t *menu, int x, int y, const char *text, qboolean rightalign)
{
	menutext_t *n = Z_Malloc(sizeof(menutext_t));
	n->common.type = mt_text;
	n->common.iszone = true;	
	n->common.posx = x;
	n->common.posy = y;
	n->text = text;

	if (rightalign && text)
		n->common.posx -= strlen(text)*8;

	n->common.next = menu->options;
	menu->options = (menuoption_t *)n;
	return n;
}

menutext_t *MC_AddBufferedText(menu_t *menu, int x, int y, const char *text, qboolean rightalign, qboolean red)
{
	menutext_t *n = Z_Malloc(sizeof(menutext_t) + strlen(text)+1);
	n->common.type = mt_text;
	n->common.iszone = true;	
	n->common.posx = x;
	n->common.posy = y;
	n->text = (char *)(n+1);
	strcpy((char *)(n+1), text);
	n->isred = red;

	if (rightalign && text)
		n->common.posx -= strlen(text)*8;

	n->common.next = menu->options;
	menu->options = (menuoption_t *)n;
	return n;
}

menutext_t *MC_AddRedText(menu_t *menu, int x, int y, const char *text, qboolean rightalign)
{
	menutext_t *n;
	n = MC_AddWhiteText(menu, x, y, text, false);
	n->isred = true;
	return n;
}

menubind_t *MC_AddBind(menu_t *menu, int x, int y, const char *caption, char *command)
{
	menubind_t *n = Z_Malloc(sizeof(menutext_t) + strlen(caption)+1 + strlen(command)+1);
	n->common.type = mt_bind;
	n->common.iszone = true;	
	n->common.posx = x;
	n->common.posy = y;
	n->caption = (char *)(n+1);
	strcpy(n->caption, caption);
	n->command = n->caption+strlen(n->caption)+1;
	strcpy(n->command, command);
	n->common.width = strlen(caption)*8 + 64;
	n->common.height = 8;

	n->common.next = menu->options;
	menu->options = (menuoption_t *)n;
	return n;
}

menupicture_t *MC_AddPicture(menu_t *menu, int x, int y, char *picname)
{
	menupicture_t *n;
	if (!qrenderer)
		return NULL;

	Draw_SafeCachePic(picname);

	n = Z_Malloc(sizeof(menupicture_t) + strlen(picname)+1);
	n->common.type = mt_picture;
	n->common.iszone = true;
	n->common.posx = x;
	n->common.posy = y;
	n->picturename = (char *)(n+1);
	strcpy(n->picturename, picname);

	n->common.next = menu->options;
	menu->options = (menuoption_t *)n;
	return n;
}

menupicture_t *MC_AddCenterPicture(menu_t *menu, int y, char *picname)
{
	int x;
	mpic_t *p;

	if (!qrenderer)
		return NULL;
	p = Draw_SafeCachePic(picname);
	if (!p)
		x = 320/2;
	else
		x = (320-p->width)/2;

	return MC_AddPicture(menu, x, y, picname);
}

menupicture_t *MC_AddCursor(menu_t *menu, int x, int y)
{
	int mgt;
	menupicture_t *n = Z_Malloc(sizeof(menupicture_t));
	n->common.type = mt_menudot;
	n->common.iszone = true;
	n->common.posx = x;
	n->common.posy = y;

	n->common.next = menu->options;
	menu->options = (menuoption_t *)n;


	mgt = M_GameType();
	if (mgt == MGT_QUAKE2)
	{	//AND QUAKE 2 WINS!!!
		menudotstyle = "m_cursor%i";
		mindot = 0;
		maxdots = 15;
		dotofs=0;
	}
	else if (mgt == MGT_HEXEN2)
	{	//AND THE WINNER IS HEXEN 2!!!
		menudotstyle = "gfx/menu/menudot%i.lmp";
		mindot = 1;
		maxdots = 8;
		dotofs=-5;
	}
	else
	{	//QUAKE 1 WINS BY DEFAULT!
		menudotstyle = "gfx/menudot%i.lmp";
		mindot = 1;
		maxdots = 6;
		dotofs=0;
	}
	return n;
}

menuedit_t *MC_AddEdit(menu_t *menu, int x, int y, char *text, char *def)
{
	menuedit_t *n = Z_Malloc(sizeof(menuedit_t));
	n->common.type = mt_edit;
	n->common.iszone = true;	
	n->common.posx = x;
	n->common.posy = y;
	n->modified = true;
	n->caption = text;
	Q_strncpyz(n->text, def, sizeof(n->text));

	n->common.next = menu->options;
	menu->options = (menuoption_t *)n;
	return n;
}

menuedit_t *MC_AddEditCvar(menu_t *menu, int x, int y, char *text, char *name)
{
	menuedit_t *n = Z_Malloc(sizeof(menuedit_t)+strlen(text)+1);
	cvar_t *cvar;
	cvar = Cvar_Get(name, "", CVAR_USERCREATED|CVAR_ARCHIVE, NULL);	//well, this is a menu/
	n->common.type = mt_edit;
	n->common.iszone = true;	
	n->common.posx = x;
	n->common.posy = y;
	n->common.width = (strlen(text)+17)*8;
	n->common.height = 8;
	n->modified = true;
	n->caption = (char *)(n+1);
	strcpy((char *)(n+1), text);
	n->cvar = cvar;
	Q_strncpyz(n->text, cvar->string, sizeof(n->text));

	n->common.next = menu->options;
	menu->options = (menuoption_t *)n;
	return n;
}

menubox_t *MC_AddBox(menu_t *menu, int x, int y, int width, int height)
{
	menubox_t *n = Z_Malloc(sizeof(menubox_t));
	n->common.type = mt_box;
	n->common.iszone = true;
	n->common.posx = x;
	n->common.posy = y;
	n->width = width;
	n->height = height;

	n->common.next = menu->options;
	menu->options = (menuoption_t *)n;
	return n;
}

menucustom_t *MC_AddCustom(menu_t *menu, int x, int y, void *data)
{
	menucustom_t *n = Z_Malloc(sizeof(menucustom_t));
	n->common.type = mt_custom;
	n->common.iszone = true;
	n->common.posx = x;
	n->common.posy = y;
	n->data = data;

	n->common.next = menu->options;
	menu->options = (menuoption_t *)n;
	return n;
}

menucheck_t *MC_AddCheckBox(menu_t *menu, int x, int y, const char *text, cvar_t *var, int bits)
{
	menucheck_t *n = Z_Malloc(sizeof(menucheck_t)+strlen(text)+1);
	n->common.type = mt_checkbox;
	n->common.iszone = true;	
	n->common.posx = x;
	n->common.posy = y;
	n->common.height = 8;
	n->common.width = (strlen(text)+7)*8;
	n->text = (char *)(n+1);
	strcpy((char *)(n+1), text);
	n->var = var;
	n->bits = bits;

	n->common.next = menu->options;
	menu->options = (menuoption_t *)n;
	return n;
}
menuslider_t *MC_AddSlider(menu_t *menu, int x, int y, const char *text, cvar_t *var, float min, float max)
{	
	menuslider_t *n = Z_Malloc(sizeof(menuslider_t)+strlen(text)+1);
	n->common.type = mt_slider;
	n->common.iszone = true;	
	n->common.posx = x;
	n->common.posy = y;
	n->common.height = 8;
	n->common.width = (strlen(text)+SLIDER_RANGE+5)*8;
	n->var = var;
	n->text = (char *)(n+1);
	strcpy((char *)(n+1), text);

	if (var)
		n->current = var->value;

	n->min = min;
	n->max = max;

	n->common.next = menu->options;
	menu->options = (menuoption_t *)n;

	return n;
}

menucombo_t *MC_AddCombo(menu_t *menu, int x, int y, const char *caption, const char **ops, int initialvalue)
{
	menucombo_t *n = Z_Malloc(sizeof(menucombo_t));
	n->common.type = mt_combo;
	n->common.iszone = true;	
	n->common.posx = x;
	n->common.posy = y;	
	n->common.height = 8;
	n->common.width = strlen(caption)*8;
	n->caption = caption;
	n->options = ops;

	n->common.next = menu->options;
	menu->options = (menuoption_t *)n;

	n->numoptions = 0;
	while(ops[n->numoptions])
	{
		n->common.width = strlen(caption)*8+strlen(ops[n->numoptions])*8;
		n->numoptions++;
	}

	if (initialvalue >= n->numoptions) 
	{
		Con_Printf("WARNING: Fixed initialvalue for %s\n", caption);
		initialvalue = n->numoptions-1;
	}
	n->selectedoption = initialvalue;

	return n;
}
menucombo_t *MC_AddCvarCombo(menu_t *menu, int x, int y, const char *caption, cvar_t *cvar, const char **ops, const char **values)
{
	menucombo_t *n = Z_Malloc(sizeof(menucombo_t));
	n->common.type = mt_combo;
	n->common.iszone = true;	
	n->common.posx = x;
	n->common.posy = y;	
	n->common.height = 8;
	n->common.width = strlen(caption)*8;
	n->caption = caption;
	n->options = ops;
	n->values = values;
	n->cvar = cvar;

	n->selectedoption = 0;

	n->common.next = menu->options;
	menu->options = (menuoption_t *)n;

	n->numoptions = 0;
	while(ops[n->numoptions])
	{
		if (!strcmp(values[n->numoptions], cvar->string))
			n->selectedoption = n->numoptions;
		n->common.width = strlen(caption)*8+strlen(ops[n->numoptions])*8;
		n->numoptions++;
	}

	return n;
}

menubutton_t *MC_AddConsoleCommand(menu_t *menu, int x, int y, const char *text, const char *command)
{
	menubutton_t *n = Z_Malloc(sizeof(menubutton_t)+strlen(text)+1+strlen(command)+1);
	n->common.type = mt_button;
	n->common.iszone = true;	
	n->common.posx = x;
	n->common.posy = y;
	n->common.height = 8;
	n->common.width = strlen(text)*8;
	n->text = (char *)(n+1);
	strcpy((char *)(n+1), text);
	n->command = n->text + strlen(n->text)+1;
	strcpy((char *)n->command, command);

	n->common.next = menu->options;
	menu->options = (menuoption_t *)n;
	return n;
}
menubutton_t *MC_AddConsoleCommandBigFont(menu_t *menu, int x, int y, const char *text, const char *command)
{
	menubutton_t *n = Z_Malloc(sizeof(menubutton_t)+strlen(text)+1+strlen(command)+1);
	n->common.type = mt_buttonbigfont;
	n->common.iszone = true;	
	n->common.posx = x;
	n->common.posy = y;
	n->common.height = 8;
	n->common.width = strlen(text)*8;
	n->text = (char *)(n+1);
	strcpy((char *)(n+1), text);
	n->command = n->text + strlen(n->text)+1;
	strcpy((char *)n->command, command);

	n->common.next = menu->options;
	menu->options = (menuoption_t *)n;
	return n;
}

menubutton_t *MC_AddCommand(menu_t *menu, int x, int y, char *text, qboolean (*command) (union menuoption_s *,struct menu_s *,int))
{
	menubutton_t *n = Z_Malloc(sizeof(menubutton_t));
	n->common.type = mt_button;
	n->common.iszone = true;	
	n->common.posx = x;
	n->common.posy = y;
	n->text = text;
	n->command = NULL;
	n->key = command;
	n->common.height = 8;
	n->common.width = strlen(text)*8;

	n->common.next = menu->options;
	menu->options = (menuoption_t *)n;
	return n;
}

menubutton_t *VARGS MC_AddConsoleCommandf(menu_t *menu, int x, int y, const char *text, char *command, ...)
{
	va_list		argptr;
	static char		string[1024];
	menubutton_t *n;
	
	va_start (argptr, command);
	_vsnprintf (string,sizeof(string)-1, command,argptr);
	va_end (argptr);	

	n = Z_Malloc(sizeof(menubutton_t) + strlen(string)+1);
	n->common.type = mt_button;
	n->common.iszone = true;	
	n->common.posx = x;
	n->common.posy = y;
	n->text = text;	
	n->command = (char *)(n+1);
	strcpy((char *)(n+1), string);

	n->common.next = menu->options;
	menu->options = (menuoption_t *)n;
	return n;
}

void MC_Slider_Key(menuslider_t *option, int key)
{
	float range = (option->current - option->min)/(option->max-option->min);

	if (key == K_LEFTARROW)
	{
		range -= 0.1;
		if (range < 0)
			range = 0;
		option->current = range = (range * (option->max-option->min)) + option->min;
		if (option->var)
			Cvar_SetValue(option->var, range);
	}
	else if (key == K_RIGHTARROW)
	{
		range += 0.1;
		if (range > 1)
			range = 1;
		option->current = range = (range * (option->max-option->min)) + option->min;
		if (option->var)
			Cvar_SetValue(option->var, range);
	}
	else if (key == K_ENTER || key == K_MOUSE1)
	{
		range += 0.1;

		if (range >= 1.05)
			range = 0;
		if (range > 1)
			range = 1;
		option->current = range = (range * (option->max-option->min)) + option->min;
		if (option->var)
			Cvar_SetValue(option->var, range);
	}
	else
		return;

	S_LocalSound ("misc/menu2.wav");
}

void MC_CheckBox_Key(menucheck_t *option, int key)
{
	if (key != K_ENTER && key != K_LEFTARROW && key != K_RIGHTARROW && key != K_MOUSE1)
		return;
	if (option->func)
		option->func((union menuoption_s *)option, CHK_TOGGLE);
	else if (!option->var)
		option->value = !option->value;
	else
	{
		if (option->bits)
		{
			int old;
			if (option->var->latched_string)
				old = atoi(option->var->latched_string);
			else
				old = option->var->value;

			if (old & option->bits)
				Cvar_SetValue(option->var, old&~option->bits);
			else
				Cvar_SetValue(option->var, old|option->bits);
		}
		else
		{
			if (option->var->latched_string)
				Cvar_SetValue(option->var, !atof(option->var->latched_string));
			else
				Cvar_SetValue(option->var, !option->var->value);
		}
		S_LocalSound ("misc/menu2.wav");
	}
}

void MC_EditBox_Key(menuedit_t *edit, int key)
{
	int len = strlen(edit->text);
	if (key == K_DEL || key == K_BACKSPACE)
	{
		if (!len)
			return;
		edit->text[len-1] = '\0';
	}
	else if (key < 32 || key > 127)
		return;
	else
	{
		edit->text[len] = key;
		edit->text[len+1] = '\0';
	}

	edit->modified = true;

	if (edit->cvar)
	{
		Cvar_Set(edit->cvar, edit->text);
		S_LocalSound ("misc/menu2.wav");
	}
}

void MC_Combo_Key(menucombo_t *combo, int key)
{
	if (key == K_ENTER || key == K_RIGHTARROW || key == K_MOUSE1)
	{
		combo->selectedoption++;
		if (combo->selectedoption >= combo->numoptions)
			combo->selectedoption = 0;

changed:
		if (combo->cvar)
			Cvar_Set(combo->cvar, (char *)combo->values[combo->selectedoption]);
	}
	else if (key == K_LEFTARROW)
	{
		combo->selectedoption--;
		if (combo->selectedoption < 0)
			combo->selectedoption = combo->numoptions-1;
		goto changed;
	}
}

menu_t *currentmenu;
menu_t *firstmenu;

void M_AddMenuFront (menu_t *menu)
{
	menu_t *pmenu;
	m_state = m_complex;
	if (!firstmenu)
	{
		M_AddMenu(menu);
		return;
	}
	pmenu = firstmenu;
	while(pmenu->parent)
		pmenu = pmenu->parent;
	pmenu->parent = menu;
	menu->child = pmenu;
	menu->parent = NULL;

	menu->exclusive = true;

	menu->xpos = ((vid.width - 320)>>1);

	currentmenu = menu;
}

void M_AddMenu (menu_t *menu)
{
	m_state = m_complex;
	menu->parent = firstmenu;
	if (firstmenu)
		firstmenu->child = menu;
	menu->child = NULL;
	firstmenu = menu;

	menu->exclusive = true;

	currentmenu = menu;
}
menu_t *M_CreateMenu (int extrasize)
{
	menu_t *menu;
	menu = Z_Malloc(sizeof(menu_t)+extrasize);
	menu->iszone=true;
	menu->data = menu+1;

	M_AddMenu(menu);

	return menu;
}
void M_HideMenu (menu_t *menu)
{
	if (menu == firstmenu)
		firstmenu = menu->parent;
	else
	{
		menu_t *prev;
		prev = menu->child;
		if (prev)
			prev->parent = menu->parent;
		if (menu->parent)
			menu->parent->child = menu;
	}
}
void M_RemoveMenu (menu_t *menu)
{	
	menuoption_t *op, *oop;
	if (menu->remove)
		menu->remove(menu);
	if (menu == firstmenu)
		firstmenu = menu->parent;
	else
	{
		menu_t *prev;
		prev = menu->child;
		if (prev)
			prev->parent = menu->parent;
		if (menu->parent)
			menu->parent->child = menu;
	}

	op = menu->options;
	while(op)
	{
		oop = op;
		op = op->common.next;
		if (oop->common.iszone)
			Z_Free(oop);
	}
	menu->options=NULL;

	if (menu->iszone)
	{
		menu->iszone=false;
		Z_Free(menu);
	}

	if (menu == currentmenu)
		currentmenu = firstmenu;
}

void M_RemoveAllMenus (void)
{
	if (!firstmenu)
		return;

	while(firstmenu)
		M_RemoveMenu(firstmenu);

}

void M_Complex_Draw(void)
{
	extern int mousecursor_x, mousecursor_y;
	menu_t *menu, *cmenu;
	qboolean foundexclusive = false;
	mpic_t *p;

	if (omousex != mousecursor_x || omousey != mousecursor_y)
		mousemoved = true;
	else
		mousemoved = false;
	omousex = mousecursor_x;
	omousey = mousecursor_y;

	if (!firstmenu)
	{
		key_dest = key_game;
		m_state = m_none;
		return;
	}

	for (menu = firstmenu; menu; )
	{
		cmenu = menu;
		menu = menu->parent;	//this way we can remove the currently drawn menu easily (if needed)

		if (cmenu->exclusive)
		{
			if (foundexclusive)
				continue;
			foundexclusive=true;
		}
		MenuDraw(cmenu);
	}

	if (!*cl_cursor.string)
		p = NULL;
	else
		p = Draw_SafeCachePic(cl_cursor.string);
	if (p)
	{
		Draw_ImageColours(1, 1, 1, 1);
		Draw_Image(mousecursor_x-cl_cursorbias.value, mousecursor_y-cl_cursorbias.value, cl_cursorsize.value, cl_cursorsize.value, 0, 0, 1, 1, p);
//		Draw_TransPic(mousecursor_x-4, mousecursor_y-4, p);
	}
	else
		Draw_Character(mousecursor_x-4, mousecursor_y-4, '+');
}

menuoption_t *M_NextItem(menu_t *m, menuoption_t *old)
{
	menuoption_t *op = m->options;
	while(op->common.next)
	{
		if (op->common.next == old)
			return op;

		op = op->common.next;
	}
	return op;
}
menuoption_t *M_NextSelectableItem(menu_t *m, menuoption_t *old)
{
	menuoption_t *op;

	if (!old)
		old = M_NextItem(m, old);
	
	op = old;

	while (1)
	{
		if (!op)
			op = currentmenu->options;

		op = M_NextItem(m, op);
		if (!op)
			op = currentmenu->options;

		if (op == old)
		{
			if (op->common.type == mt_slider || op->common.type == mt_checkbox || op->common.type == mt_button || op->common.type == mt_buttonbigfont || op->common.type == mt_edit || op->common.type == mt_combo || op->common.type == mt_bind || op->common.type == mt_custom)
				return op;
			return NULL;	//whoops.
		}

		if (op->common.type == mt_slider || op->common.type == mt_checkbox || op->common.type == mt_button || op->common.type == mt_buttonbigfont || op->common.type == mt_edit || op->common.type == mt_combo || op->common.type == mt_bind || op->common.type == mt_custom)
			if (!op->common.ishidden)
				return op;
	}
}

menuoption_t *M_PrevSelectableItem(menu_t *m, menuoption_t *old)
{
	menuoption_t *op;	

	if (!old)
		old = currentmenu->options;
	
	op = old;

	while (1)
	{
		if (!op)
			op = currentmenu->options;

		op = op->common.next;
		if (!op)
			op = currentmenu->options;

		if (op == old)
			return old;	//whoops.

		if (op->common.type == mt_slider || op->common.type == mt_checkbox || op->common.type == mt_button || op->common.type == mt_buttonbigfont || op->common.type == mt_edit || op->common.type == mt_combo || op->common.type == mt_bind || op->common.type == mt_custom)
			if (!op->common.ishidden)
				return op;
	}
}

void M_Complex_Key(int key)
{
	if (!currentmenu)
		return;	//erm...
	
	if (currentmenu->key)
		if (currentmenu->key(key, currentmenu))
			return;

	if (currentmenu->selecteditem && currentmenu->selecteditem->common.type == mt_custom && (key == K_DOWNARROW || key == K_UPARROW || key == K_TAB))
		if (currentmenu->selecteditem->custom.key(&currentmenu->selecteditem->custom, currentmenu, key))
			return;

	if (currentmenu->selecteditem && currentmenu->selecteditem->common.type == mt_bind)
	{
		if (bindingactive)
		{
			S_LocalSound ("misc/menu1.wav");

			if (key != K_ESCAPE && key != '`')
			{
				Cbuf_InsertText (va("bind %s \"%s\"\n", Key_KeynumToString (key), currentmenu->selecteditem->bind.command), RESTRICT_LOCAL);
			}
			bindingactive = false;
			return;
		}
	}
	
	switch(key)
	{
	case K_MOUSE2:
	case K_ESCAPE:
		//remove
		M_RemoveMenu(currentmenu);
		S_LocalSound ("misc/menu3.wav");
		break;
	case K_TAB:
	case K_DOWNARROW:
		currentmenu->selecteditem = M_NextSelectableItem(currentmenu, currentmenu->selecteditem);

		if (currentmenu->selecteditem)				
		{
			S_LocalSound ("misc/menu1.wav");
			if (currentmenu->cursoritem)
				currentmenu->cursoritem->common.posy = currentmenu->selecteditem->common.posy;
		}
		break;
	case K_UPARROW:
		currentmenu->selecteditem = M_PrevSelectableItem(currentmenu, currentmenu->selecteditem);

		if (currentmenu->selecteditem)				
		{
			S_LocalSound ("misc/menu1.wav");
			if (currentmenu->cursoritem)
				currentmenu->cursoritem->common.posy = currentmenu->selecteditem->common.posy;
		}
		break;
	default:
		if (!currentmenu->selecteditem)
		{
			if (!currentmenu->options)
				return;
			currentmenu->selecteditem = currentmenu->options;
		}
		switch(currentmenu->selecteditem->common.type)
		{
		case mt_slider:
			MC_Slider_Key(&currentmenu->selecteditem->slider, key);
			break;
		case mt_checkbox:
			MC_CheckBox_Key(&currentmenu->selecteditem->check, key);
			break;
		case mt_button:
		case mt_buttonbigfont:
			if (!currentmenu->selecteditem->button.command)
				currentmenu->selecteditem->button.key(currentmenu->selecteditem, currentmenu, key);
			else if (key == K_ENTER || key == K_MOUSE1)
			{
				Cbuf_AddText(currentmenu->selecteditem->button.command, RESTRICT_LOCAL);
				S_LocalSound ("misc/menu2.wav");
			}
			break;
		case mt_custom:
			currentmenu->selecteditem->custom.key(&currentmenu->selecteditem->custom, currentmenu, key);
			break;
		case mt_edit:
			MC_EditBox_Key(&currentmenu->selecteditem->edit, key);
			break;
		case mt_combo:
			MC_Combo_Key(&currentmenu->selecteditem->combo, key);
			break;
		case mt_bind:
			if (key == K_ENTER || key == K_MOUSE1)
				bindingactive = true;
			else if (key == K_BACKSPACE || key == K_DEL)
				M_UnbindCommand (currentmenu->selecteditem->bind.command);
		default:
			break;
		}
		break;
	}
}





typedef struct {
	int itemselected;
	menu_t *dropout;
	menutext_t *op[64];
	char *text[64];

	menu_t *parent;
} guiinfo_t;

qboolean MC_GuiKey(int key, menu_t *menu)
{
	guiinfo_t *info = (guiinfo_t *)menu->data;
	switch(key)
	{
	case K_ESCAPE:		
		if (info->dropout)
			MC_GuiKey(key, info->dropout);
		else
		{
			guiinfo_t *gui;
			M_RemoveMenu(menu);
			if (menu->parent)
			{
				gui = (guiinfo_t *)menu->parent->data;
				gui->dropout = NULL;
			}
		}
		break;

	case K_ENTER:
	case K_RIGHTARROW:
		if (info->dropout)
			MC_GuiKey(key, info->dropout);
		else
		{
			int y, i;
			guiinfo_t *gui;
			info->dropout = M_CreateMenu(sizeof(guiinfo_t));
			currentmenu = info->dropout;
			info->dropout->key = MC_GuiKey;
			info->dropout->exclusive = false;
			info->dropout->parent = menu;
			info->dropout->xpos = 0;
			info->dropout->ypos = menu->ypos+info->itemselected*8;
			for (i = 0; info->text[i]; i++)
				if (info->dropout->xpos < strlen(info->text[i]))
					info->dropout->xpos = strlen(info->text[i]);
			info->dropout->xpos*=8;
			info->dropout->xpos+=menu->xpos;
			gui = (guiinfo_t *)info->dropout->data;
			gui->text[0] = "Hello";
			gui->text[1] = "Hello again";
			gui->text[2] = "Hello yet again";
			for (y = 0, i = 0; gui->text[i]; i++, y+=1*8)
			{				
				info->op[i] = MC_AddRedText(info->dropout, 0, y, gui->text[i], false);
			}
		}
		break;
	case K_LEFTARROW:
		if (info->dropout)
			MC_GuiKey(key, info->dropout);
		else
		{
			guiinfo_t *gui;
			M_RemoveMenu(menu);
			if (menu->parent)
			{
				gui = (guiinfo_t *)menu->parent->data;
				gui->dropout = NULL;
			}
		}
		break;
	case K_UPARROW:
		info->op[info->itemselected]->isred = true;
		if (info->itemselected)
			info->itemselected--;
		info->op[info->itemselected]->isred = false;
		break;
	case K_DOWNARROW:
		if (!info->op[info->itemselected])
			break;
		info->op[info->itemselected]->isred = true;
		if (info->text[info->itemselected+1])
			info->itemselected++;
		info->op[info->itemselected]->isred = false;
		break;
	}

	return true;
}


qboolean MC_Main_Key (int key, menu_t *menu)	//here purly to restart demos.
{
	if (key == K_ESCAPE)
	{
		extern int m_save_demonum;
		key_dest = key_game;
		m_state = m_none;
		cls.demonum = m_save_demonum;
		if (cls.demonum != -1 && !cls.demoplayback && cls.state == ca_disconnected && COM_CheckParm("-demos"))
			CL_NextDemo ();
		return true;
	}
	return false;
}

void M_Menu_Main_f (void)
{
	extern cvar_t m_helpismedia;
	menubutton_t *b;
	menu_t *mainm;
	mpic_t *p;

	int mgt;

	SCR_EndLoadingPlaque();	//just in case...

	if (!Draw_SafeCachePic)
	{
		Con_ToggleConsole_f();
		return;
	}

/*
	if (0)
	{
		int x, i;
		guiinfo_t *gui;
		m_state = m_complex;
		key_dest = key_menu;
		m_entersound = true;

		mainm = M_CreateMenu(sizeof(guiinfo_t));
		mainm->key = MC_GuiKey;
		mainm->xpos=0;
		gui = (guiinfo_t *)mainm->data;		
		gui->text[0] = "Single";
		gui->text[1] = "Multiplayer";
		gui->text[2] = "Quit";
		for (x = 0, i = 0; gui->text[i]; i++)
		{
			gui->op[i] = MC_AddRedText(mainm, x, 0, gui->text[i], false);
			x+=(strlen(gui->text[i])+1)*8;
		}		
		return;
	}
*/

	S_LocalSound ("misc/menu2.wav");

	mgt = M_GameType();
	if (mgt == MGT_QUAKE2)	//quake2 main menu.
	{
		if (Draw_SafeCachePic("pics/m_main_game"))
		{			
			m_state = m_complex;
			key_dest = key_menu;

			mainm = M_CreateMenu(0);	
			mainm->key = MC_Main_Key;	

			MC_AddPicture(mainm, 0, 4, "pics/m_main_plaque");
			p = Draw_SafeCachePic("pics/m_main_logo");
			if (!p)
				return;
			MC_AddPicture(mainm, 0, 173, "pics/m_main_logo");
			MC_AddPicture(mainm, 68, 13, "pics/m_main_game");
			MC_AddPicture(mainm, 68, 53, "pics/m_main_multiplayer");
			MC_AddPicture(mainm, 68, 93, "pics/m_main_options");
			MC_AddPicture(mainm, 68, 133, "pics/m_main_video");
			MC_AddPicture(mainm, 68, 173, "pics/m_main_quit");

			mainm->selecteditem = (menuoption_t *)
			MC_AddConsoleCommand	(mainm, 68, 13,	"", "menu_single\n");
			MC_AddConsoleCommand	(mainm, 68, 53,	"", "menu_multi\n");
			MC_AddConsoleCommand	(mainm, 68, 93,	"", "menu_options\n");
			MC_AddConsoleCommand	(mainm, 68, 133,	"", "menu_video\n");
			MC_AddConsoleCommand	(mainm, 68, 173,	"", "menu_quit\n");

			mainm->cursoritem = (menuoption_t *)MC_AddCursor(mainm, 42, 13);
		}
		return;
	}
	else if (mgt == MGT_HEXEN2)
	{
		m_state = m_complex;
		key_dest = key_menu;
		mainm = M_CreateMenu(0);	
		mainm->key = MC_Main_Key;	

		MC_AddPicture(mainm, 16, 0, "gfx/menu/hplaque.lmp");
		p = Draw_SafeCachePic("gfx/menu/title0.lmp");
		if (!p)
			return;
		MC_AddPicture(mainm, (320-p->width)/2, 0, "gfx/menu/title0.lmp");

		b=MC_AddConsoleCommandBigFont	(mainm, 80, 64,	"Single Player", "menu_single\n");
		b->common.width = 12*20;
		b->common.height = 20;
		b=MC_AddConsoleCommandBigFont	(mainm, 80, 64+20,	"MultiPlayer", "menu_multi\n");
		b->common.width = 12*20;
		b->common.height = 20;
		b=MC_AddConsoleCommandBigFont	(mainm, 80, 64+40,	"Options", "menu_options\n");
		b->common.width = 12*20;
		b->common.height = 20;
		if (m_helpismedia.value)
			b=MC_AddConsoleCommandBigFont	(mainm, 80, 64+60,	"Media", "menu_media\n");
		else
			b=MC_AddConsoleCommandBigFont	(mainm, 80, 64+60,	"Help", "help\n");
		b->common.width = 12*20;
		b->common.height = 20;
		b=MC_AddConsoleCommandBigFont	(mainm, 80, 64+80,	"Quit", "menu_quit\n");
		b->common.width = 12*20;
		b->common.height = 20;

		mainm->cursoritem = (menuoption_t *)MC_AddCursor(mainm, 48, 64);
	}
	else
	{
		m_state = m_complex;
		key_dest = key_menu;
		mainm = M_CreateMenu(0);		

		p = Draw_SafeCachePic("gfx/ttl_main.lmp");
		if (!p)
		{
			MC_AddRedText(mainm, 16, 0,				"MAIN MENU", false);

			mainm->selecteditem = (menuoption_t *)
			MC_AddConsoleCommand	(mainm, 64, 32,	"Join server", "menu_servers\n");
			MC_AddConsoleCommand	(mainm, 64, 40,	"Options", "menu_options\n");
			MC_AddConsoleCommand	(mainm, 64, 48,	"Quit", "menu_quit\n");
			return;
		}
		mainm->key = MC_Main_Key;
		MC_AddPicture(mainm, 16, 4, "gfx/qplaque.lmp");

		MC_AddPicture(mainm, (320-p->width)/2, 4, "gfx/ttl_main.lmp");
		MC_AddPicture(mainm, 72, 32, "gfx/mainmenu.lmp");


		p = Draw_SafeCachePic("gfx/mainmenu.lmp");

		b=MC_AddConsoleCommand	(mainm, 72, 32,	"", "menu_single\n");
		mainm->selecteditem = (menuoption_t *)b;
		b->common.width = p->width;
		b->common.height = 20;
		b=MC_AddConsoleCommand	(mainm, 72, 52,	"", "menu_multi\n");
		b->common.width = p->width;
		b->common.height = 20;
		b=MC_AddConsoleCommand	(mainm, 72, 72,	"", "menu_options\n");
		b->common.width = p->width;
		b->common.height = 20;
		if (m_helpismedia.value)
			b=MC_AddConsoleCommand(mainm, 72, 92,	"", "menu_media\n");
		else
			b=MC_AddConsoleCommand(mainm, 72, 92,	"", "help\n");
		b->common.width = p->width;
		b->common.height = 20;
		b=MC_AddConsoleCommand	(mainm, 72, 112,	"", "menu_quit\n");
		b->common.width = p->width;
		b->common.height = 20;

		mainm->cursoritem = (menuoption_t *)MC_AddCursor(mainm, 54, 32);
	}
}

