#include "quakedef.h"
#include <ctype.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#endif

typedef struct f_modified_s {
	char name[MAX_QPATH];
	qboolean ismodified;
	struct f_modified_s *next;
} f_modified_t;

static f_modified_t *f_modified_list;
qboolean care_f_modified;
qboolean f_modified_particles;


cvar_t allow_f_version		= SCVAR("allow_f_version", "1");
cvar_t allow_f_server		= SCVAR("allow_f_server", "1");
cvar_t allow_f_modified		= SCVAR("allow_f_modified", "1");
cvar_t allow_f_skins		= SCVAR("allow_f_skins", "1");
cvar_t allow_f_ruleset		= SCVAR("allow_f_ruleset", "1");
cvar_t allow_f_scripts		= SCVAR("allow_f_scripts", "1");
cvar_t allow_f_fakeshaft	= SCVAR("allow_f_fakeshaft", "1");
cvar_t allow_f_system		= SCVAR("allow_f_system", "0");
cvar_t allow_f_cmdline		= SCVAR("allow_f_cmdline", "0");
cvar_t auth_validateclients	= SCVAR("auth_validateclients", "1");
cvar_t ruleset			= SCVAR("ruleset", "none");


#define SECURITY_INIT_BAD_CHECKSUM	1
#define SECURITY_INIT_BAD_VERSION	2
#define SECURITY_INIT_ERROR			3
#define SECURITY_INIT_NOPROC		4

typedef struct signed_buffer_s {
	qbyte *buf;
	unsigned long size;
} signed_buffer_t;

typedef signed_buffer_t *(*Security_Verify_Response_t) (int playernum, unsigned char *, char *userinfo, char *serverinfo);
typedef int (*Security_Init_t) (char *);
typedef signed_buffer_t *(*Security_Generate_Crc_t) (int playernum, char *userinfo, char *serverinfo);
typedef signed_buffer_t *(*Security_IsModelModified_t) (char *, int, qbyte *, int);
typedef void (*Security_Supported_Binaries_t) (void *);
typedef void (*Security_Shutdown_t) (void);


static Security_Verify_Response_t Security_Verify_Response;
static Security_Init_t Security_Init;
static Security_Generate_Crc_t Security_Generate_Crc;
static Security_IsModelModified_t Security_IsModelModified;
static Security_Supported_Binaries_t Security_Supported_Binaries;
static Security_Shutdown_t Security_Shutdown;


static void *secmodule;

static void Validation_Version(void)
{
	char sr[256];
	char *s = sr;
	char *auth;
	char authbuf[256];

	extern cvar_t r_shadow_realtime_world, r_drawflat;

	switch(qrenderer)
	{
#ifdef RGLQUAKE
	case QR_OPENGL:
		s = sr;
		//print certain allowed 'cheat' options.
		//realtime lighting (shadows can show around corners)
		//drawflat is just lame
		//24bits can be considered eeeevil, by some.
		if (r_shadows.value)
		{
			if (r_shadow_realtime_world.value)
				*s++ = 'W';
			else
				*s++ = 'S';
		}
		if (r_drawflat.value)
			*s++ = 'F';
		if (gl_load24bit.value)
			*s++ = 'H';

		*s = *"";
		break;
#endif
#ifdef SWQUAKE
	case QR_SOFTWARE:
		if (r_pixbytes == 4)
		{
			*s++ = '3';
			*s++ = '2';
		}
		else if (r_pixbytes == 2)
		{
			*s++ = '1';
			*s++ = '6';
		}
		else
			*s++ = '8';
		break;
#endif
	default:
		*sr = *"";
		break;
	}

	*s = '\0';

	if (!allow_f_version.value)
		return;	//suppress it

	if (Security_Generate_Crc)
	{
		signed_buffer_t *resp;

		resp = Security_Generate_Crc(cl.playernum[0], cl.players[cl.playernum[0]].userinfo, cl.serverinfo);
		if (!resp || !resp->buf)
			auth = "";
		else
			Q_snprintfz(auth, sizeof(authbuf), " crc: %s", resp->buf);
	}
	else
		auth = "";

	if (*sr)
		Cbuf_AddText (va("say "DISTRIBUTION"Quake v%i "PLATFORM"/%s/%s%s\n", build_number(), q_renderername, sr, auth), RESTRICT_RCON);
	else
		Cbuf_AddText (va("say "DISTRIBUTION"Quake v%i "PLATFORM"/%s%s\n", build_number(), q_renderername, auth), RESTRICT_RCON);
}
void Validation_CheckIfResponse(char *text)
{
	//client name, version type(os-renderer where it matters, os/renderer where renderer doesn't), 12 char hex crc
	int f_query_client;
	int i;
	char *crc;
	char *versionstring;

	if (!Security_Verify_Response)
		return;	//valid or not, we can't check it.

	if (!auth_validateclients.value)
		return;

	//do the parsing.
	{
		char *comp;
		int namelen;

		for (crc = text + strlen(text) - 1; crc > text; crc--)
			if ((unsigned)*crc > ' ')
				break;

		//find the crc.
		for (i = 0; i < 29; i++)
		{
			if (crc <= text)
				return;	//not enough chars.
			if ((unsigned)crc[-1] <= ' ')
				break;
			crc--;
		}

		//we now want 3 string seperated tokens, so the first starts at the fourth found ' ' + 1
		i = 7;
		for (comp = crc-1; ; comp--)
		{
			if (comp < text)
				return;
			if (*comp == ' ')
			{
				i--;
				if (!i)
					break;
			}

		}

		versionstring = comp+1;
		if (comp <= text)
			return;	//not enough space for the 'name:'
		if (*(comp-1) != ':')
			return;	//whoops. not a say.

		namelen = comp - text-1;

		for (f_query_client = 0; f_query_client < MAX_CLIENTS; f_query_client++)
		{
			if (strlen(cl.players[f_query_client].name) == namelen)
				if (!strncmp(cl.players[f_query_client].name, text, namelen))
					break;
		}
		if (f_query_client == MAX_CLIENTS)
			return; //looks like a validation, but it's not from a known client.
	}

	{
		char *match = DISTRIBUTION"Quake v";
		if (strncmp(versionstring, match, strlen(match)))
			return;	//this is not us
	}

	//now do the validation
	{
		signed_buffer_t *resp;

		resp = Security_Verify_Response(f_query_client, crc, cl.players[f_query_client].userinfo, cl.serverinfo);

		if (resp && resp->size && *resp->buf)
			Con_Printf(S_NOTICE "Authentication Successful.\n");
		else// if (!resp)
			Con_Printf(S_ERROR "AUTHENTICATION FAILED.\n");
	}
}

void InitValidation(void)
{
	Cvar_Register(&allow_f_version,	"Authentication");
	Cvar_Register(&allow_f_server,	"Authentication");
	Cvar_Register(&allow_f_modified,	"Authentication");
	Cvar_Register(&allow_f_skins,	"Authentication");
	Cvar_Register(&allow_f_ruleset,	"Authentication");
	Cvar_Register(&allow_f_fakeshaft,	"Authentication");
	Cvar_Register(&allow_f_scripts,	"Authentication");
	Cvar_Register(&allow_f_system,	"Authentication");
	Cvar_Register(&allow_f_cmdline,	"Authentication");
	Cvar_Register(&ruleset,		"Authentication");

#ifdef _WIN32
	secmodule = LoadLibrary("fteqw-security.dll");
	if (secmodule)
	{
		Security_Verify_Response	= (void*)GetProcAddress(secmodule, "Security_Verify_Response");
		Security_Init				= (void*)GetProcAddress(secmodule, "Security_Init");
		Security_Generate_Crc		= (void*)GetProcAddress(secmodule, "Security_Generate_Crc");
		Security_IsModelModified	= (void*)GetProcAddress(secmodule, "Security_IsModelModified");
		Security_Supported_Binaries	= (void*)GetProcAddress(secmodule, "Security_Supported_Binaries");
		Security_Shutdown			= (void*)GetProcAddress(secmodule, "Security_Shutdown");
	}
#endif

	if (Security_Init)
	{
		switch(Security_Init(va("%s %.2f %i", DISTRIBUTION, 2.57, build_number())))
		{
		case SECURITY_INIT_BAD_CHECKSUM:
			Con_Printf("Checksum failed. Security module does not support this build. Go upgrade it.\n");
			break;
		case SECURITY_INIT_BAD_VERSION:
			Con_Printf("Version failed. Security module does not support this version. Go upgrade.\n");
			break;
		case SECURITY_INIT_ERROR:
			Con_Printf("'Generic' security error. Stop hacking.\n");
			break;
		case SECURITY_INIT_NOPROC:
			Con_Printf("/proc/* does not exist. You will need to upgrade/reconfigure your kernel.\n");
			break;
		case 0:
			Cvar_Register(&auth_validateclients,	"Authentication");
			return;
		}
#ifdef _WIN32
		FreeLibrary(secmodule);
#endif
	}
	Security_Verify_Response	= NULL;
	Security_Init				= NULL;
	Security_Generate_Crc		= NULL;
	Security_IsModelModified	= NULL;
	Security_Supported_Binaries	= NULL;
	Security_Shutdown			= NULL;
}

//////////////////////
//f_modified

void Validation_IncludeFile(char *filename, char *file, int filelen)
{
}

static void Validation_FilesModified (void)
{
	Con_Printf ("Not implemented\n", RESTRICT_RCON);
}

void Validation_FlushFileList(void)
{
	f_modified_t *fm;
	while(f_modified_list)
	{
		fm = f_modified_list->next;

		Z_Free(f_modified_list);
		f_modified_list = fm;
	}
}

/////////////////////////
//minor (codewise) responses

static void Validation_Server(void)
{
#ifndef _MSC_VER
	#warning is allowing the user to turn this off practical?..
#endif
	if (!allow_f_server.value)
		return;
	Cbuf_AddText(va("say server is %s\n", NET_AdrToString(cls.netchan.remote_address)), RESTRICT_LOCAL);
}

static void Validation_Skins(void)
{
	extern cvar_t r_fullbrightSkins, r_fb_models;
	int percent = r_fullbrightSkins.value*100;

	if (!allow_f_skins.value)
		return;

	RulesetLatch(&r_fb_models);
	RulesetLatch(&r_fullbrightSkins);

	if (percent < 0)
		percent = 0;
	if (percent > cls.allow_fbskins*100)
		percent = cls.allow_fbskins*100;
	if (percent)
		Cbuf_AddText(va("say all player skins %i%% fullbright%s\n", percent, r_fb_models.value?" (plus luma)":""), RESTRICT_LOCAL);
	else if (r_fb_models.value)
		Cbuf_AddText("say luma textures only\n", RESTRICT_LOCAL);
	else
		Cbuf_AddText("say Only cheaters use full bright skins\n", RESTRICT_LOCAL);
}

static void Validation_Scripts(void)
{	//subset of ruleset
	if (!allow_f_scripts.value)
		return;
	if (ruleset_allow_frj.value)
		Cbuf_AddText("say scripts are allowed\n", RESTRICT_LOCAL);
	else
		Cbuf_AddText("say scripts are capped\n", RESTRICT_LOCAL);
}

static void Validation_FakeShaft(void)
{
	extern cvar_t cl_truelightning;
	if (!allow_f_fakeshaft.value)
		return;
	if (cl_truelightning.value > 0.999)
		Cbuf_AddText("say fakeshaft on\n", RESTRICT_LOCAL);
	else if (cl_truelightning.value > 0)
		Cbuf_AddText(va("say fakeshaft %.1f%%\n", cl_truelightning.value), RESTRICT_LOCAL);
	else
		Cbuf_AddText("say fakeshaft off\n", RESTRICT_LOCAL);
}

static void Validation_System(void)
{	//subset of ruleset
	if (!allow_f_system.value)
		return;
	Cbuf_AddText("say f_system not supported\n", RESTRICT_LOCAL);
}

static void Validation_CmdLine(void)
{
	if (!allow_f_cmdline.value)
		return;
	Cbuf_AddText("say f_cmdline not supported\n", RESTRICT_LOCAL);
}

//////////////////////
//rulesets

typedef struct {
	char *rulename;
	char *rulevalue;
} rulesetrule_t;
typedef struct {
	char *rulesetname;

	rulesetrule_t *rule;

	qboolean flagged;
} ruleset_t;

rulesetrule_t rulesetrules_strict[] = {
	{"gl_shadeq1", "0"},
	{"gl_shadeq3", "0"},	//FIXME: there needs to be some other way to block these
	{"ruleset_allow_playercount", "0"},
	{"ruleset_allow_frj", "0"},
	{"ruleset_allow_packet", "0"},
	{"ruleset_allow_particle_lightning", "0"},
	{"ruleset_allow_overlongsounds", "0"},
	{"ruleset_allow_larger_models", "0"},
	{"tp_disputablemacros", "0"},
	{"cl_instantrotate", "0"},
	{NULL}
};

rulesetrule_t rulesetrules_nnql[] = {
	{"ruleset_allow_larger_models", "0"},
	{"ruleset_allow_overlong_sounds", "0"},
	{"ruleset_allow_particle_lightning", "0"},
	{"ruleset_allow_packet", "0"},
	{"ruleset_allow_frj", "0"},
	{"ruleset_allow_playercount", "0"},
	{"gl_shadeq1", "0"},
	{"gl_shadeq3", "0"},
	{NULL}
};

static ruleset_t rulesets[] =
{
	{"strict", rulesetrules_strict},
	{"nnql", rulesetrules_nnql},
	{NULL}
};

void RulesetLatch(cvar_t *cvar)
{
	cvar->flags |= CVAR_RULESETLATCH;
}

void Validation_DelatchRulesets(void)
{	//game has come to an end, allow the ruleset to be changed
	Cvar_ApplyLatches(CVAR_RULESETLATCH);
	Con_DPrintf("Ruleset deactivated\n");
}

void Validation_Ruleset(void)
{	//this code is more complex than it needs to be
	//this allows for the ruleset code to print a ruleset name that is applied via the cvars, but not directly named by the user
	cvar_t *var;
	ruleset_t *rs;
	int i;
	char rsnames[1024];
	rs = rulesets;
	*rsnames = '\0';

#ifndef _MSC_VER
#warning here's a question... Should we latch the ruleset unconditionally, or only when someone actually cares?
#warning if we do it only when someone checks, we have a lot more checking, otherwise we have a freer tournament if the users choose to play that way
#warning I'm going to do it the old-fashioned way
#warning (yes, this is one for molgrum to resolve!)
#endif
	for (rs = rulesets; rs->rulesetname; rs++)
	{
		rs->flagged = false;

		for (i = 0; rs->rule[i].rulename; i++)
		{
			var = Cvar_FindVar(rs->rule[i].rulename);
			if (!var)	//sw rendering?
				continue;

			if (strcmp(var->string, rs->rule[i].rulevalue))
				break;	//current settings don't match
		}
		if (!rs->rule[i].rulename)
		{
			if (*rsnames)
			{
				Q_strncatz(rsnames, ", ", sizeof(rsnames));
			}
			Q_strncatz(rsnames, rs->rulesetname, sizeof(rsnames));
			rs->flagged = true;
		}
	}
	if (*rsnames)
	{
		Cbuf_AddText(va("say Ruleset: %s\n", rsnames), RESTRICT_LOCAL);

		//now we've told the other players what rules we're playing by, we'd best stick to them
		for (rs = rulesets; rs->rulesetname; rs++)
		{
			if (!rs->flagged)
				continue;
			for (i = 0; rs->rule[i].rulename; i++)
			{
				var = Cvar_FindVar(rs->rule[i].rulename);
				if (!var)
					continue;
				RulesetLatch(var);	//set the latched flag
			}
		}
	}
	else
		Cbuf_AddText("say No specific ruleset\n", RESTRICT_LOCAL);
}

void Validation_Apply_Ruleset(void)
{	//rulesets are applied when the client first gets a connection to the server
	ruleset_t *rs;
	rulesetrule_t *rule;
	cvar_t *var;
	int i;

#ifndef _MSC_VER
	#warning fixme: the following line should not be needed. ensure this is the case
#endif
	Validation_DelatchRulesets();	//make sure there's no old one

	if (!*ruleset.string || !strcmp(ruleset.string, "none"))
		return;	//no ruleset is set

	for (rs = rulesets; rs->rulesetname; rs++)
	{
		if (!stricmp(rs->rulesetname, ruleset.string))
			break;
	}
	if (!rs->rulesetname)
	{
		Con_Printf("Cannot apply ruleset %s - not recognised\n", rs->rulesetname);
		return;
	}
	
	for (rule = rs->rule; rule->rulename; rule++)
	{
		for (i = 0; rs->rule[i].rulename; i++)
		{
			var = Cvar_FindVar(rs->rule[i].rulename);
			if (!var)
				continue;

			if (!Cvar_ApplyLatchFlag(var, rs->rule[i].rulevalue, CVAR_RULESETLATCH))
			{
				Con_Printf("Failed to apply ruleset %s due to cvar %s\n", rs->rulesetname, var->name);
				break;
			}
		}
	}

	Con_DPrintf("Ruleset set to %s\n", rs->rulesetname);
}

//////////////////////

void Validation_Auto_Response(int playernum, char *s)
{
	static float versionresponsetime;
	static float modifiedresponsetime;
	static float skinsresponsetime;
	static float serverresponsetime;
	static float rulesetresponsetime;
	static float systemresponsetime;
	static float fakeshaftresponsetime;
	static float cmdlineresponsetime;
	static float scriptsresponsetime;

	if (!strncmp(s, "f_version", 9) && versionresponsetime < Sys_DoubleTime())	//respond to it.
	{
		Validation_Version();
		versionresponsetime = Sys_DoubleTime() + 5;
	}
	else if (cl.spectator)
		return;
	else if (!strncmp(s, "f_server", 8) && serverresponsetime < Sys_DoubleTime())	//respond to it.
	{
		Validation_Server();
		serverresponsetime = Sys_DoubleTime() + 5;
	}
	else if (!strncmp(s, "f_system", 8) && systemresponsetime < Sys_DoubleTime())
	{
		Validation_System();
		systemresponsetime = Sys_DoubleTime() + 5;
	}
	else if (!strncmp(s, "f_cmdline", 9) && cmdlineresponsetime < Sys_DoubleTime())
	{
		Validation_CmdLine();
		cmdlineresponsetime = Sys_DoubleTime() + 5;
	}
	else if (!strncmp(s, "f_fakeshaft", 11) && fakeshaftresponsetime < Sys_DoubleTime())
	{
		Validation_FakeShaft();
		fakeshaftresponsetime = Sys_DoubleTime() + 5;
	}
	else if (!strncmp(s, "f_modified", 10) && modifiedresponsetime < Sys_DoubleTime())	//respond to it.
	{
		Validation_FilesModified();
		modifiedresponsetime = Sys_DoubleTime() + 5;
	}
	else if (!strncmp(s, "f_scripts", 9) && scriptsresponsetime < Sys_DoubleTime())
	{
		Validation_Scripts();
		scriptsresponsetime = Sys_DoubleTime() + 5;
	}
	else if (!strncmp(s, "f_skins", 7) && skinsresponsetime < Sys_DoubleTime())	//respond to it.
	{
		Validation_Skins();
		skinsresponsetime = Sys_DoubleTime() + 5;
	}
	else if (!strncmp(s, "f_ruleset", 9) && rulesetresponsetime < Sys_DoubleTime())
	{
		Validation_Ruleset();
		rulesetresponsetime = Sys_DoubleTime() + 5;
	}
}


