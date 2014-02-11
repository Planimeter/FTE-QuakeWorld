//file for builtin implementations relevent to only clientside VMs (menu+csqc).
#include "quakedef.h"

#include "pr_common.h"
#include "shader.h"

#if defined(CSQC_DAT) || defined(MENU_DAT)

//these two global qcinput variables are the current scan code being passed to qc, if valid. this protects against protected apis where the qc just passes stuff through.
int qcinput_scan;
int qcinput_unicode;

//QC key codes are based upon DP's keycode constants. This is on account of menu.dat coming first.
int MP_TranslateFTEtoQCCodes(int code)
{
	switch(code)
	{
	case K_TAB:				return 9;
	case K_ENTER:			return 13;
	case K_ESCAPE:			return 27;
	case K_SPACE:			return 32;
	case K_BACKSPACE:		return 127;
	case K_UPARROW:			return 128;
	case K_DOWNARROW:		return 129;
	case K_LEFTARROW:		return 130;
	case K_RIGHTARROW:		return 131;
	case K_LALT:			return 132;
	case K_RALT:			return -K_RALT;
	case K_LCTRL:			return 133;
	case K_RCTRL:			return -K_RCTRL;
	case K_LSHIFT:			return 134;
	case K_RSHIFT:			return -K_RSHIFT;
	case K_F1:				return 135;
	case K_F2:				return 136;
	case K_F3:				return 137;
	case K_F4:				return 138;
	case K_F5:				return 139;
	case K_F6:				return 140;
	case K_F7:				return 141;
	case K_F8:				return 142;
	case K_F9:				return 143;
	case K_F10:				return 144;
	case K_F11:				return 145;
	case K_F12:				return 146;
	case K_INS:				return 147;
	case K_DEL:				return 148;
	case K_PGDN:			return 149;
	case K_PGUP:			return 150;
	case K_HOME:			return 151;
	case K_END:				return 152;
	case K_PAUSE:			return 153;
	case K_KP_NUMLOCK:		return 154;
	case K_CAPSLOCK:		return 155;
	case K_SCRLCK:			return 156;
	case K_KP_INS:			return 157;
	case K_KP_END:			return 158;
	case K_KP_DOWNARROW:	return 159;
	case K_KP_PGDN:			return 160;
	case K_KP_LEFTARROW:	return 161;
	case K_KP_5:			return 162;
	case K_KP_RIGHTARROW:	return 163;
	case K_KP_HOME:			return 164;
	case K_KP_UPARROW:		return 165;
	case K_KP_PGUP:			return 166;
	case K_KP_DEL:			return 167;
	case K_KP_SLASH:		return 168;
	case K_KP_STAR:			return 169;
	case K_KP_MINUS:		return 170;
	case K_KP_PLUS:			return 171;
	case K_KP_ENTER:		return 172;
	case K_KP_EQUALS:		return 173;
	case K_PRINTSCREEN:		return 174;

	case K_MOUSE1:			return 512;
	case K_MOUSE2:			return 513;
	case K_MOUSE3:			return 514;
	case K_MWHEELUP:		return 515;
	case K_MWHEELDOWN:		return 516;
	case K_MOUSE4:			return 517;
	case K_MOUSE5:			return 518;
	case K_MOUSE6:			return 519;
	case K_MOUSE7:			return 520;
	case K_MOUSE8:			return 521;
	case K_MOUSE9:			return 522;
	case K_MOUSE10:			return 523;
//	case K_MOUSE11:			return 524;
//	case K_MOUSE12:			return 525;
//	case K_MOUSE13:			return 526;
//	case K_MOUSE14:			return 527;
//	case K_MOUSE15:			return 528;
//	case K_MOUSE16:			return 529;

	case K_JOY1:			return 768;
	case K_JOY2:			return 769;
	case K_JOY3:			return 770;
	case K_JOY4:			return 771;
//	case K_JOY5:			return 772;
//	case K_JOY6:			return 773;
//	case K_JOY7:			return 774;
//	case K_JOY8:			return 775;
//	case K_JOY9:			return 776;
//	case K_JOY10:			return 777;
//	case K_JOY11:			return 778;
//	case K_JOY12:			return 779;
//	case K_JOY13:			return 780;
//	case K_JOY14:			return 781;
//	case K_JOY15:			return 782;
//	case K_JOY16:			return 783;

	case K_AUX1:			return 784;
	case K_AUX2:			return 785;
	case K_AUX3:			return 786;
	case K_AUX4:			return 787;
	case K_AUX5:			return 788;
	case K_AUX6:			return 789;
	case K_AUX7:			return 790;
	case K_AUX8:			return 791;
	case K_AUX9:			return 792;
	case K_AUX10:			return 793;
	case K_AUX11:			return 794;
	case K_AUX12:			return 795;
	case K_AUX13:			return 796;
	case K_AUX14:			return 797;
	case K_AUX15:			return 798;
	case K_AUX16:			return 799;
	case K_AUX17:			return 800;
	case K_AUX18:			return 801;
	case K_AUX19:			return 802;
	case K_AUX20:			return 803;
	case K_AUX21:			return 804;
	case K_AUX22:			return 805;
	case K_AUX23:			return 806;
	case K_AUX24:			return 807;
	case K_AUX25:			return 808;
	case K_AUX26:			return 809;
	case K_AUX27:			return 810;
	case K_AUX28:			return 811;
	case K_AUX29:			return 812;
	case K_AUX30:			return 813;
	case K_AUX31:			return 814;
	case K_AUX32:			return 815;

	case K_VOLUP:			return -code;
	case K_VOLDOWN:			return -code;
	case K_APP:				return -code;
	case K_SEARCH:			return -code;

	default:				return code;
	}
}

int MP_TranslateQCtoFTECodes(int code)
{
	switch(code)
	{
	case 9:			return K_TAB;
	case 13:		return K_ENTER;
	case 27:		return K_ESCAPE;
	case 32:		return K_SPACE;
	case 127:		return K_BACKSPACE;
	case 128:		return K_UPARROW;
	case 129:		return K_DOWNARROW;
	case 130:		return K_LEFTARROW;
	case 131:		return K_RIGHTARROW;
	case 132:		return K_LALT;
	case 133:		return K_LCTRL;
	case 134:		return K_LSHIFT;
	case 135:		return K_F1;
	case 136:		return K_F2;
	case 137:		return K_F3;
	case 138:		return K_F4;
	case 139:		return K_F5;
	case 140:		return K_F6;
	case 141:		return K_F7;
	case 142:		return K_F8;
	case 143:		return K_F9;
	case 144:		return K_F10;
	case 145:		return K_F11;
	case 146:		return K_F12;
	case 147:		return K_INS;
	case 148:		return K_DEL;
	case 149:		return K_PGDN;
	case 150:		return K_PGUP;
	case 151:		return K_HOME;
	case 152:		return K_END;
	case 153:		return K_PAUSE;
	case 154:		return K_KP_NUMLOCK;
	case 155:		return K_CAPSLOCK;
	case 156:		return K_SCRLCK;
	case 157:		return K_KP_INS;
	case 158:		return K_KP_END;
	case 159:		return K_KP_DOWNARROW;
	case 160:		return K_KP_PGDN;
	case 161:		return K_KP_LEFTARROW;
	case 162:		return K_KP_5;
	case 163:		return K_KP_RIGHTARROW;
	case 164:		return K_KP_HOME;
	case 165:		return K_KP_UPARROW;
	case 166:		return K_KP_PGUP;
	case 167:		return K_KP_DEL;
	case 168:		return K_KP_SLASH;
	case 169:		return K_KP_STAR;
	case 170:		return K_KP_MINUS;
	case 171:		return K_KP_PLUS;
	case 172:		return K_KP_ENTER;
	case 173:		return K_KP_EQUALS;
	case 174:		return K_PRINTSCREEN;

	case 512:		return K_MOUSE1;
	case 513:		return K_MOUSE2;
	case 514:		return K_MOUSE3;
	case 515:		return K_MWHEELUP;
	case 516:		return K_MWHEELDOWN;
	case 517:		return K_MOUSE4;
	case 518:		return K_MOUSE5;
	case 519:		return K_MOUSE6;
	case 520:		return K_MOUSE7;
	case 521:		return K_MOUSE8;
	case 522:		return K_MOUSE9;
	case 523:		return K_MOUSE10;
//	case 524:		return K_MOUSE11;
//	case 525:		return K_MOUSE12;
//	case 526:		return K_MOUSE13;
//	case 527:		return K_MOUSE14;
//	case 528:		return K_MOUSE15;
//	case 529:		return K_MOUSE16;

	case 768:		return K_JOY1;
	case 769:		return K_JOY2;
	case 770:		return K_JOY3;
	case 771:		return K_JOY4;
//	case 772:		return K_JOY5;
//	case 773:		return K_JOY6;
//	case 774:		return K_JOY7;
//	case 775:		return K_JOY8;
//	case 776:		return K_JOY9;
//	case 777:		return K_JOY10;
//	case 778:		return K_JOY11;
//	case 779:		return K_JOY12;
//	case 780:		return K_JOY13;
//	case 781:		return K_JOY14;
//	case 782:		return K_JOY15;
//	case 783:		return K_JOY16;

	case 784:		return K_AUX1;
	case 785:		return K_AUX2;
	case 786:		return K_AUX3;
	case 787:		return K_AUX4;
	case 788:		return K_AUX5;
	case 789:		return K_AUX6;
	case 790:		return K_AUX7;
	case 791:		return K_AUX8;
	case 792:		return K_AUX9;
	case 793:		return K_AUX10;
	case 794:		return K_AUX11;
	case 795:		return K_AUX12;
	case 796:		return K_AUX13;
	case 797:		return K_AUX14;
	case 798:		return K_AUX15;
	case 799:		return K_AUX16;
	case 800:		return K_AUX17;
	case 801:		return K_AUX18;
	case 802:		return K_AUX19;
	case 803:		return K_AUX20;
	case 804:		return K_AUX21;
	case 805:		return K_AUX22;
	case 806:		return K_AUX23;
	case 807:		return K_AUX24;
	case 808:		return K_AUX25;
	case 809:		return K_AUX26;
	case 810:		return K_AUX27;
	case 811:		return K_AUX28;
	case 812:		return K_AUX29;
	case 813:		return K_AUX30;
	case 814:		return K_AUX31;
	case 815:		return K_AUX32;
	default:		
		if (code < 0)	//negative values are 'fte-native' keys, for stuff that the api lacks.
			return -code;
		return code;
	}
}

//string	findkeysforcommand(string command) = #610;
void QCBUILTIN PF_cl_findkeysforcommand (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *cmdname = PR_GetStringOfs(prinst, OFS_PARM0);
	int keynums[2];
	char keyname[512];

	M_FindKeysForCommand(0, cmdname, keynums);

	keyname[0] = '\0';

	Q_strncatz (keyname, va(" \'%i\'", MP_TranslateFTEtoQCCodes(keynums[0])), sizeof(keyname));
	Q_strncatz (keyname, va(" \'%i\'", MP_TranslateFTEtoQCCodes(keynums[1])), sizeof(keyname));

	RETURN_TSTRING(keyname);
}

void QCBUILTIN PF_cl_getkeybind (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *binding = Key_GetBinding(MP_TranslateQCtoFTECodes(G_FLOAT(OFS_PARM0)));
	RETURN_TSTRING(binding);
}

void QCBUILTIN PF_cl_stringtokeynum(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int i;
	int modifier;
	char *s;

	s = PR_GetStringOfs(prinst, OFS_PARM0);
	i = Key_StringToKeynum(s, &modifier);
	if (i < 0 || modifier != ~0)
	{
		G_FLOAT(OFS_RETURN) = -1;
		return;
	}
	i = MP_TranslateFTEtoQCCodes(i);
	G_FLOAT(OFS_RETURN) = i;
}

//string	keynumtostring(float keynum) = #609;
void QCBUILTIN PF_cl_keynumtostring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int code = G_FLOAT(OFS_PARM0);

	code = MP_TranslateQCtoFTECodes (code);

	RETURN_TSTRING(Key_KeynumToString(code));
}


void QCBUILTIN PF_cl_playingdemo (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_FLOAT(OFS_RETURN) = !!cls.demoplayback;
}

void QCBUILTIN PF_cl_runningserver (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
#ifdef CLIENTONLY
	G_FLOAT(OFS_RETURN) = false;
#else
	G_FLOAT(OFS_RETURN) = sv.state != ss_dead;
#endif
}



#ifndef NOMEDIA

// #487 float(string name) gecko_create( string name )
void QCBUILTIN PF_cs_gecko_create (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *shadername = PR_GetStringOfs(prinst, OFS_PARM0);
	cin_t *cin;
	cin = R_ShaderGetCinematic(R_RegisterShader(shadername, SUF_2D,
				"{\n"
					"{\n"
						"videomap http:\n"
					"}\n"
				"}\n"
			));

	if (!cin)
		G_FLOAT(OFS_RETURN) = 0;
	else
		G_FLOAT(OFS_RETURN) = 1;
}
// #488 void(string name) gecko_destroy( string name )
void QCBUILTIN PF_cs_gecko_destroy (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
}
// #489 void(string name) gecko_navigate( string name, string URI )
void QCBUILTIN PF_cs_gecko_navigate (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *shader = PR_GetStringOfs(prinst, OFS_PARM0);
	char *command = PR_GetStringOfs(prinst, OFS_PARM1);
	cin_t *cin;
	cin = R_ShaderFindCinematic(shader);

	if (!cin)
		return;

	Media_Send_Command(cin, command);
}
// #490 float(string name) gecko_keyevent( string name, float key, float eventtype )
void QCBUILTIN PF_cs_gecko_keyevent (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *shader = PR_GetStringOfs(prinst, OFS_PARM0);
	int key = G_FLOAT(OFS_PARM1);
	int eventtype = G_FLOAT(OFS_PARM2);
	cin_t *cin;
	cin = R_ShaderFindCinematic(shader);

	if (!cin)
		return;
	Media_Send_KeyEvent(cin, MP_TranslateQCtoFTECodes(key), (key>127)?0:key, eventtype);
}
// #491 void gecko_mousemove( string name, float x, float y )
void QCBUILTIN PF_cs_gecko_mousemove (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *shader = PR_GetStringOfs(prinst, OFS_PARM0);
	float posx = G_FLOAT(OFS_PARM1);
	float posy = G_FLOAT(OFS_PARM2);
	cin_t *cin;
	cin = R_ShaderFindCinematic(shader);

	if (!cin)
		return;
	Media_Send_MouseMove(cin, posx, posy);
}
// #492 void gecko_resize( string name, float w, float h )
void QCBUILTIN PF_cs_gecko_resize (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *shader = PR_GetStringOfs(prinst, OFS_PARM0);
	float sizex = G_FLOAT(OFS_PARM1);
	float sizey = G_FLOAT(OFS_PARM2);
	cin_t *cin;
	cin = R_ShaderFindCinematic(shader);
	if (!cin)
		return;
	Media_Send_Resize(cin, sizex, sizey);
}
// #493 vector gecko_get_texture_extent( string name )
void QCBUILTIN PF_cs_gecko_get_texture_extent (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *shader = PR_GetStringOfs(prinst, OFS_PARM0);
	float *ret = G_VECTOR(OFS_RETURN);
	int sx, sy;
	cin_t *cin;
	cin = R_ShaderFindCinematic(shader);
	if (cin)
	{
		Media_Send_GetSize(cin, &sx, &sy);
	}
	else
	{
		sx = 0;
		sy = 0;
	}
	ret[0] = sx;
	ret[1] = sy;
	ret[2] = 0;
}
#endif

void QCBUILTIN PF_soundlength (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *sample = PR_GetStringOfs(prinst, OFS_PARM0);

	sfx_t *sfx = S_PrecacheSound(sample);
	if (!sfx || sfx->failedload)
		G_FLOAT(OFS_RETURN) = 0;
	else
	{
		sfxcache_t cachebuf, *cache;
		if (sfx->decoder.decodedata)
			cache = sfx->decoder.decodedata(sfx, &cachebuf, 0x7ffffffe, 0);
		else
			cache = sfx->decoder.buf;
		G_FLOAT(OFS_RETURN) = (cache->soundoffset+cache->length) / (float)snd_speed;
	}
}

#ifdef CL_MASTER
#include "cl_master.h"

void QCBUILTIN PF_cl_gethostcachevalue (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	hostcacheglobal_t hcg = G_FLOAT(OFS_PARM0);
	G_FLOAT(OFS_RETURN) = 0;
	switch(hcg)
	{
	case SLIST_HOSTCACHEVIEWCOUNT:
		CL_QueryServers();
		Master_CheckPollSockets();
		G_FLOAT(OFS_RETURN) = Master_NumSorted();
		return;
	case SLIST_HOSTCACHETOTALCOUNT:
		CL_QueryServers();
		Master_CheckPollSockets();
		G_FLOAT(OFS_RETURN) = Master_TotalCount();
		return;

	case SLIST_MASTERQUERYCOUNT:
	case SLIST_MASTERREPLYCOUNT:
	case SLIST_SERVERQUERYCOUNT:
	case SLIST_SERVERREPLYCOUNT:
		G_FLOAT(OFS_RETURN) = 0;
		return;

	case SLIST_SORTFIELD:
		G_FLOAT(OFS_RETURN) = Master_GetSortField();
		return;
	case SLIST_SORTDESCENDING:
		G_FLOAT(OFS_RETURN) = Master_GetSortDescending();
		return;
	default:
		return;
	}
}

//void 	resethostcachemasks(void) = #615;
void QCBUILTIN PF_cl_resethostcachemasks(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	Master_ClearMasks();
}
//void 	sethostcachemaskstring(float mask, float fld, string str, float op) = #616;
void QCBUILTIN PF_cl_sethostcachemaskstring(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int mask = G_FLOAT(OFS_PARM0);
	int field = G_FLOAT(OFS_PARM1);
	char *str = PR_GetStringOfs(prinst, OFS_PARM2);
	int op = G_FLOAT(OFS_PARM3);

	Master_SetMaskString(mask, field, str, op);
}
//void	sethostcachemasknumber(float mask, float fld, float num, float op) = #617;
void QCBUILTIN PF_cl_sethostcachemasknumber(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	int mask = G_FLOAT(OFS_PARM0);
	int field = G_FLOAT(OFS_PARM1);
	int str = G_FLOAT(OFS_PARM2);
	int op = G_FLOAT(OFS_PARM3);

	Master_SetMaskInteger(mask, field, str, op);
}
//void 	resorthostcache(void) = #618;
void QCBUILTIN PF_cl_resorthostcache(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	Master_SortServers();
}
//void	sethostcachesort(float fld, float descending) = #619;
void QCBUILTIN PF_cl_sethostcachesort(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	Master_SetSortField(G_FLOAT(OFS_PARM0), G_FLOAT(OFS_PARM1));
}
//void	refreshhostcache(void) = #620;
void QCBUILTIN PF_cl_refreshhostcache(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	MasterInfo_Refresh();
}
//float	gethostcachenumber(float fld, float hostnr) = #621;
void QCBUILTIN PF_cl_gethostcachenumber(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	float ret = 0;
	int keynum = G_FLOAT(OFS_PARM0);
	int svnum = G_FLOAT(OFS_PARM1);
	serverinfo_t *sv;
	sv = Master_SortedServer(svnum);

	ret = Master_ReadKeyFloat(sv, keynum);

	G_FLOAT(OFS_RETURN) = ret;
}
void QCBUILTIN PF_cl_gethostcachestring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *ret;
	int keynum = G_FLOAT(OFS_PARM0);
	int svnum = G_FLOAT(OFS_PARM1);
	serverinfo_t *sv;

	sv = Master_SortedServer(svnum);
	ret = Master_ReadKeyString(sv, keynum);

	RETURN_TSTRING(ret);
}

//float	gethostcacheindexforkey(string key) = #622;
void QCBUILTIN PF_cl_gethostcacheindexforkey(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *keyname = PR_GetStringOfs(prinst, OFS_PARM0);

	G_FLOAT(OFS_RETURN) = Master_KeyForName(keyname);
}
//void	addwantedhostcachekey(string key) = #623;
void QCBUILTIN PF_cl_addwantedhostcachekey(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	PF_cl_gethostcacheindexforkey(prinst, pr_globals);
}

void QCBUILTIN PF_cl_getextresponse(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	//this does something weird
	G_INT(OFS_RETURN) = 0;
}
#else
void PF_cl_gethostcachevalue (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals){G_FLOAT(OFS_RETURN) = 0;}
void PF_cl_gethostcachestring (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals) {G_INT(OFS_RETURN) = 0;}
//void 	resethostcachemasks(void) = #615;
void PF_cl_resethostcachemasks(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals){}
//void 	sethostcachemaskstring(float mask, float fld, string str, float op) = #616;
void PF_cl_sethostcachemaskstring(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals){}
//void	sethostcachemasknumber(float mask, float fld, float num, float op) = #617;
void PF_cl_sethostcachemasknumber(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals){}
//void 	resorthostcache(void) = #618;
void PF_cl_resorthostcache(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals){}
//void	sethostcachesort(float fld, float descending) = #619;
void PF_cl_sethostcachesort(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals){}
//void	refreshhostcache(void) = #620;
void PF_cl_refreshhostcache(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals) {}
//float	gethostcachenumber(float fld, float hostnr) = #621;
void PF_cl_gethostcachenumber(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals){G_FLOAT(OFS_RETURN) = 0;}
//float	gethostcacheindexforkey(string key) = #622;
void PF_cl_gethostcacheindexforkey(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals){G_FLOAT(OFS_RETURN) = 0;}
//void	addwantedhostcachekey(string key) = #623;
void PF_cl_addwantedhostcachekey(pubprogfuncs_t *prinst, struct globalvars_s *pr_globals){}
#endif




void QCBUILTIN PF_shaderforname (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	char *str = PR_GetStringOfs(prinst, OFS_PARM0);
	char *defaultbody = PF_VarString(prinst, 1, pr_globals);

	shader_t *shad;

	if (*defaultbody)
		shad = R_RegisterShader(str, SUF_NONE, defaultbody);
	else
		shad = R_RegisterSkin(str, NULL);
	if (shad)
		G_FLOAT(OFS_RETURN) = shad->id+1;
	else
		G_FLOAT(OFS_RETURN) = 0;
}

void QCBUILTIN PF_cl_GetBindMap (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
	G_VECTOR(OFS_RETURN)[0] = 1;
	G_VECTOR(OFS_RETURN)[1] = 0;
	G_VECTOR(OFS_RETURN)[2] = 0;
}
void QCBUILTIN PF_cl_SetBindMap (pubprogfuncs_t *prinst, struct globalvars_s *pr_globals)
{
//	int primary = G_FLOAT(OFS_PARM0+0);
//	int secondary = G_FLOAT(OFS_PARM0+1);
//	if (IN_SetBindMap(primary, secondary))
//		G_FLOAT(OFS_RETURN) = 1;
	G_FLOAT(OFS_RETURN) = 0;
}

#endif
