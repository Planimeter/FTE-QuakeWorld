//read menu.h

#include "quakedef.h"
#include "shader.h"

#ifndef NOBUILTINMENUS

int selectitem;
menu_t *menu_script;

void M_Script_Option (menu_t *menu, char *optionvalue)
{
	menuoption_t *mo;

	char buf[8192];
	//update the option
	Cbuf_AddText(va("set option %s\n", COM_QuotedString(optionvalue, buf, sizeof(buf), false)), RESTRICT_LOCAL);

	//expand private arguments
	for (mo = menu->options, *buf = 0; mo; mo = mo->common.next)
	{
		if (mo->common.type == mt_edit)
		{
			if (strlen(buf) + strlen(mo->edit.text) + 2 >= sizeof(buf))
				break;
			memmove(buf+strlen(mo->edit.text)+1, buf, strlen(buf)+1);
			memcpy(buf, mo->edit.text, strlen(mo->edit.text));
			buf[strlen(mo->edit.text)] = ' ';
		}
	}
	Cmd_TokenizeString(buf, false, false);
	Cmd_ExpandString(menu->data, buf, sizeof(buf), RESTRICT_SERVER, true, true);

	//and execute it as-is
	Cbuf_AddText(buf, RESTRICT_LOCAL);
	Cbuf_AddText("\n", RESTRICT_LOCAL);
}

void M_Script_Remove (menu_t *menu)
{
	if (menu == menu_script)
		menu_script = NULL;

	M_Script_Option(menu, "cancel");
}
qboolean M_Script_Key (int key, menu_t *menu)
{
	if (menu->selecteditem && menu->selecteditem->common.type == mt_edit)
		return false;
	if (key >= '0' && key <= '9' && menu->data)
	{
		if (key == '0')	//specal case so that "hello" < "0"... (plus matches common impulses)
			M_Script_Option(menu, "10");
		else
			M_Script_Option(menu, va("%i", key-'0'));
		return true;
	}
	return false;
}

void M_MenuS_Callback_f (void)
{
	if (menu_script)
	{
		M_Script_Option(menu_script, Cmd_Argv(1));
	}
}
void M_MenuS_Clear_f (void)
{
	if (menu_script)
	{
		M_RemoveMenu(menu_script);
	}
}

void M_MenuS_Script_f (void)	//create a menu.
{
	int items;
	extern menu_t *currentmenu;
	menu_t *oldmenu;
	char *alias = Cmd_Argv(1);
	Key_Dest_Add(kdm_emenu);
	m_state = m_complex;

	selectitem = 0;
	items=0;

	if (menu_script)
	{
		menuoption_t *option;
		for (option = menu_script->options; option; option = option->common.next)
		{
			if (option->common.type == mt_button)
			{
				if (menu_script->selecteditem == option)
					selectitem = items;
				items++;
			}
		}
		selectitem = items - selectitem-1;
		M_MenuS_Clear_f();
	}

	oldmenu = currentmenu;

	menu_script = M_CreateMenu(0);
	if (oldmenu)
	{
		M_HideMenu(oldmenu);	//bring to front
		M_AddMenu(oldmenu);
	}	
	menu_script->remove = M_Script_Remove;
	menu_script->key = M_Script_Key;
	
	Key_Dest_Remove(kdm_console);

	if (Cmd_Argc() == 1)
		menu_script->data = Cmd_ParseMultiline(true);
	else
		menu_script->data = Z_StrDup(alias);
}

void M_MenuS_Box_f (void)
{
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	int width = atoi(Cmd_Argv(3));
	int height = atoi(Cmd_Argv(4));

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}

	MC_AddBox(menu_script, x, y, width/8, height/8);
}

void M_MenuS_CheckBox_f (void)
{
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	char *text = Cmd_Argv(3);
	char *cvarname = Cmd_Argv(4);
	int bitmask = atoi(Cmd_Argv(5));
	cvar_t *cvar;

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}
	cvar = Cvar_Get(cvarname, text, 0, "User variables");
	if (!cvar)
		return;
	MC_AddCheckBox(menu_script, x, x+160, y, text, cvar, bitmask);
}

void M_MenuS_Slider_f (void)
{
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	char *text = Cmd_Argv(3);
	char *cvarname = Cmd_Argv(4);
	float min = atof(Cmd_Argv(5));
	float max = atof(Cmd_Argv(6));
	cvar_t *cvar;

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}
	cvar = Cvar_Get(cvarname, text, 0, "User variables");
	if (!cvar)
		return;
	MC_AddSlider(menu_script, x, x+160, y, text, cvar, min, max, 0);
}

void M_MenuS_Picture_f (void)
{
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	char *picname = Cmd_Argv(3);
	mpic_t *p;

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}

	p = R2D_SafeCachePic(picname);
	if (!p)
		return;

	if (!strcmp(Cmd_Argv(1), "-"))
		MC_AddCenterPicture(menu_script, y, p->height, picname);
	else
		MC_AddPicture(menu_script, x, y, p->width, p->height, picname);
}

void M_MenuS_Edit_f (void)
{
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	char *text = Cmd_Argv(3);
	char *def = Cmd_Argv(4);

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}

	MC_AddEditCvar(menu_script, x, x+160, y, text, def, false);
}
void M_MenuS_EditPriv_f (void)
{
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	char *text = Cmd_Argv(3);
	char *def = Cmd_Argv(4);

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}

	MC_AddEdit(menu_script, x, x+160, y, text, def);
}

void M_MenuS_Text_f (void)
{
	menuoption_t *option;
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	char *text = Cmd_Argv(3);
	char *command = Cmd_Argv(4);

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}
	if (Cmd_Argc() == 4)
		MC_AddBufferedText(menu_script, x, 0, y, text, false, false);
	else
	{
		option = (menuoption_t *)MC_AddConsoleCommand(menu_script, x, 0, y, text, va("menucallback %s\n", command));
		if (selectitem-- == 0)
			menu_script->selecteditem = option;
	}
}

void M_MenuS_TextBig_f (void)
{
	menuoption_t *option;
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	char *text = Cmd_Argv(3);
	char *command = Cmd_Argv(4);

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}
	if (!*command)
		MC_AddConsoleCommandQBigFont(menu_script, x, y, text, command);
	else
	{
		option = (menuoption_t *)MC_AddConsoleCommandQBigFont(menu_script, x, y, text, va("menucallback %s\n", command));
		if (selectitem-- == 0)
			menu_script->selecteditem = option;
	}
}

void M_MenuS_Bind_f (void)
{
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	char *caption = Cmd_Argv(3);
	char *command = Cmd_Argv(4);

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}

	if (!*caption)
		caption = command;

	MC_AddBind(menu_script, x, x+160, y, command, caption, NULL);
}

void M_MenuS_Comboi_f (void)
{
	int opt;
	char *opts[64];
	char *values[64];
	char valuesb[64][8];
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	char *caption = Cmd_Argv(3);
	char *command = Cmd_Argv(4);
	char *line;

	cvar_t *var;

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}

	var = Cvar_Get(command, "0", 0, "custom cvars");
	if (!var)
		return;

	if (!*caption)
		caption = command;

	for (opt = 0; opt < sizeof(opts)/sizeof(opts[0])-2 && *(line=Cmd_Argv(5+opt)); opt++)
	{
		opts[opt] = line;
		Q_snprintfz(valuesb[opt], sizeof(valuesb[opt]), "%i", opt);
		values[opt] = valuesb[opt];
	}
	opts[opt] = NULL;

	MC_AddCvarCombo(menu_script, x, x+160, y, caption, var, (const char **)opts, (const char **)values);
}

char *Hunk_TempString(char *s)
{
	char *h;
	h = Hunk_TempAllocMore(strlen(s)+1);
	strcpy(h, s);
	return h;
}

void M_MenuS_Combos_f (void)
{
	int opt;
	char *opts[64];
	char *values[64];
	int x = atoi(Cmd_Argv(1));
	int y = atoi(Cmd_Argv(2));
	char *caption = Cmd_Argv(3);
	char *command = Cmd_Argv(4);
	char *line;

	cvar_t *var;

	if (!menu_script)
	{
		Con_Printf("%s with no active menu\n", Cmd_Argv(0));
		return;
	}

	var = Cvar_Get(command, "0", 0, "custom cvars");
	if (!var)
		return;

	if (!*caption)
		caption = command;

	line = Cmd_Argv(5);
	if (!*line)
	{
		line = Cbuf_GetNext(Cmd_ExecLevel, true);
		if (*line != '{')
			Cbuf_InsertText(line, Cmd_ExecLevel, true);	//whoops. Stick the trimmed string back in to the cbuf.
		else
			line = "{";
	}
	if (!strcmp(line, "{"))
	{
		char *line;
		Hunk_TempAlloc(4);
		for (opt = 0; opt < sizeof(opts)/sizeof(opts[0])-2; opt++)
		{
			line = Cbuf_GetNext(Cmd_ExecLevel, true);
			line = COM_Parse(line);
			if (!strcmp(com_token, "}"))
				break;
			opts[opt] = Hunk_TempString(com_token);
			line = COM_Parse(line);
			values[opt] = Hunk_TempString(com_token);
		}
	}
	else
	{
		for (opt = 0; opt < sizeof(opts)/sizeof(opts[0])-2; opt++)
		{
			line = Cmd_Argv(5+opt*2);
			if (!*line)
				break;
			opts[opt] = line;
			values[opt] = Cmd_Argv(5+opt*2 + 1);
		}
	}
	opts[opt] = NULL;

	MC_AddCvarCombo(menu_script, x, x+160, y, caption, var, (const char **)opts, (const char **)values);
}

/*
menuclear
conmenu menucallback

menubox 0 0 320 8
menutext 0 0 "GO GO GO!!!" 		"radio21"
menutext 0 8 "Fall back" 		"radio22"
menutext 0 8 "Stick together" 		"radio23"
menutext 0 16 "Get in position"		"radio24"
menutext 0 24 "Storm the front"	 	"radio25"
menutext 0 24 "Report in"	 	"radio26"
menutext 0 24 "Cancel"	
*/
void M_Script_Init(void)
{
	Cmd_AddCommandD("menuclear",	M_MenuS_Clear_f, "Pop the currently scripted menu.");
	Cmd_AddCommandD("menucallback",	M_MenuS_Callback_f, "Explicitly invoke the active script menu's callback function with the given option set.");
	Cmd_AddCommand("conmenu",	M_MenuS_Script_f);
	Cmd_AddCommand("menubox",	M_MenuS_Box_f);
	Cmd_AddCommand("menuedit",	M_MenuS_Edit_f);
	Cmd_AddCommand("menueditpriv",	M_MenuS_EditPriv_f);
	Cmd_AddCommand("menutext",	M_MenuS_Text_f);
	Cmd_AddCommand("menutextbig",	M_MenuS_TextBig_f);
	Cmd_AddCommand("menupic",	M_MenuS_Picture_f);
	Cmd_AddCommand("menucheck",	M_MenuS_CheckBox_f);
	Cmd_AddCommand("menuslider",	M_MenuS_Slider_f);
	Cmd_AddCommand("menubind",	M_MenuS_Bind_f);
	Cmd_AddCommand("menucomboi",	M_MenuS_Comboi_f);
	Cmd_AddCommand("menucombos",	M_MenuS_Combos_f);
}
#endif
