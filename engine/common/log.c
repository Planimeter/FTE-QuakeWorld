// log.c: handles console logging functions and cvars

#include "quakedef.h"

// cvar callbacks
void Log_Dir_Callback (struct cvar_s *var, char *oldvalue);
void Log_Name_Callback (struct cvar_s *var, char *oldvalue);

// cvars
#define CONLOGGROUP "Console logging"
cvar_t		log_name = SCVARFC("log_name", "", CVAR_NOTFROMSERVER, Log_Name_Callback);
cvar_t		log_dir = SCVARFC("log_dir", "", CVAR_NOTFROMSERVER, Log_Dir_Callback);
cvar_t		log_readable = SCVARF("log_readable", "0", CVAR_NOTFROMSERVER);
cvar_t		log_enable = SCVARF("log_enable", "0", CVAR_NOTFROMSERVER);
cvar_t		log_developer = SCVARF("log_developer", "0", CVAR_NOTFROMSERVER);
cvar_t		log_rotate_files = SCVARF("log_rotate_files", "0", CVAR_NOTFROMSERVER);
cvar_t		log_rotate_size = SCVARF("log_rotate_size", "131072", CVAR_NOTFROMSERVER);
cvar_t		log_dosformat = SCVARF("log_dosformat", "0", CVAR_NOTFROMSERVER);

// externals
int COM_FileSize(char *path);
extern char gamedirfile[];

// table of readable characters, same as ezquake
char readable[256] =
{
	'.', '_', '_', '_', '_', '.', '_', '_',
	'_', '_', '\n', '_', '\n', '>', '.', '.',
	'[', ']', '0', '1', '2', '3', '4', '5',
	'6', '7', '8', '9', '.', '_', '_', '_',
	' ', '!', '\"', '#', '$', '%', '&', '\'',
	'(', ')', '*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', ':', ';', '<', '=', '>', '?',
	'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
	'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
	'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
	'x', 'y', 'z', '{', '|', '}', '~', '_',
	'_', '_', '_', '_', '_', '.', '_', '_',
	'_', '_', '_', '_', '_', '>', '.', '.',
	'[', ']', '0', '1', '2', '3', '4', '5',
	'6', '7', '8', '9', '.', '_', '_', '_',
	' ', '!', '\"', '#', '$', '%', '&', '\'',
	'(', ')', '*', '+', ',', '-', '.', '/',
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', ':', ';', '<', '=', '>', '?',
	'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
	'X', 'Y', 'Z', '[', '\\', ']', '^', '_',
	'`', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
	'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
	'x', 'y', 'z', '{', '|', '}', '~', '_'
};

// Log_Dir_Callback: called when a log_dir is changed
void Log_Dir_Callback (struct cvar_s *var, char *oldvalue)
{
	char *t = var->string;

	// sanity check for directory
	if (strstr(t, "..") || strstr(t, ":") || *t == '/' || *t == '\\')
	{
		Con_Printf(S_NOTICE "%s forced to default due to invalid characters.\n", var->name);
		// recursion is avoided by assuming the default value is sane
		Cvar_ForceSet(var, var->defaultstr);
	}
}

// Log_Name_Callback: called when a log_dir is changed
void Log_Name_Callback (struct cvar_s *var, char *oldvalue)
{
	char *t = var->string;

	// sanity check for directory
	if (strstr(t, "..") || strstr(t, ":") || strstr(t, "/") || strstr(t, "\\"))
	{
		Con_Printf(S_NOTICE "%s forced to default due to invalid characters.\n", var->name);
		// recursion is avoided by assuming the default value is sane
		Cvar_ForceSet(var, var->defaultstr);
	}
}

// Con_Log: log string to console log
void Con_Log (char *s)
{
	char *d; // directory
	char *f; // filename
	char *t;
	char logbuf[1024];
	int i;

	if (!log_enable.value)
		return;

	// get directory/filename
	d = gamedirfile;
	if (log_dir.string[0])
		d = log_dir.string;

	f = "qconsole";
	if (log_name.string[0])
		f = log_name.string;

	// readable translation and Q3 code removal, use t for final string to write
	t = logbuf;
	// max debuglog buf is 1024
	for (i = 0; i < 1023; i++, s++)
	{
		if (*s == 0)
			break;
		else if (((int)(log_readable.value) & 2) && *s == '^')
		{
			// log_readable 2 removes Q3 codes as well
			char c = s[1];

			if ((c >= '0' && c <= '9') || c == 'a' || c == 'b' || c == 'h' || c == 's' || c == 'r')
			{
				i--;
				s++;
			}
			else if (c == '&')
			{
				if (isextendedcode(s[2]) && isextendedcode(s[3]))
				{
					i--;
					s += 3;
				}
			}
			else
			{
				*t = '^';
				t++;
			}
		}
		else if (log_dosformat.value && *s == '\n')
		{
			// convert \n to \r\n
			*t = '\r';
			t++;
			i++;
			if (i < 1023)
			{
				*t = '\n';
				t++;
			}
		}
		else
		{
			// use readable table to convert quake chars to reabable text
			if ((int)(log_readable.value) & 1)
				*t = readable[(unsigned char)(*s)]; // translate
			else
				*t = *s; // copy
			t++;
		}
	}

	*t = 0;

	f = va("%s/%s.log",d,f); // temp string in va()

	// file rotation
	if (log_rotate_size.value >= 4096 && log_rotate_files.value >= 1)
	{
		int x;
		vfsfile_t *fi;

		// check file size, use x as temp
		if ((fi = FS_OpenVFS(f, "rb", FS_BASE)))
		{
			x = VFS_GETLEN(fi);
			VFS_CLOSE(fi);
			x += i; // add string size to file size to never go over
		}
		else
			x = 0;

		if (x > (int)log_rotate_size.value)
		{
			char newf[MAX_OSPATH];
			char oldf[MAX_OSPATH];

			i = log_rotate_files.value;

			// unlink file at the top of the chain
			snprintf(oldf, sizeof(oldf)-1, "%s.%i", f, i);
			FS_Remove(oldf, FS_BASE);

			// rename files through chain
			for (x = i-1; x > 0; x--)
			{
				strcpy(newf, oldf);
				snprintf(oldf, sizeof(oldf)-1, "%s.%i", f, x);

				// check if file exists, otherwise skip
				if ((fi = FS_OpenVFS(oldf, "rb", FS_BASE)))
					VFS_CLOSE(fi);
				else
					continue; // skip nonexistant files

				if (FS_Rename(oldf, newf, FS_BASE))
				{
					// rename failed, disable log and bug out
					Cvar_ForceSet(&log_enable, "0");
					Con_Printf("Unable to rotate log files. Logging disabled.\n");
					return;
				}
			}

			// TODO: option to compress file somewhere in here?
			// rename our base file, which better exist...
			if (FS_Rename(f, oldf, FS_BASE))
			{
				// rename failed, disable log and bug out
				Cvar_ForceSet(&log_enable, "0");
				Con_Printf("Unable to rename base log file. Logging disabled.\n");
				return;
			}
		}
	}

	// write to log file
	if (Sys_DebugLog(f, "%s", logbuf))
	{
		// write failed, bug out
		Cvar_ForceSet(&log_enable, "0");
		Con_Printf("Unable to write to log file. Logging disabled.\n");
		return;
	}
}

void Log_Init(void)
{
	// register cvars
	Cvar_Register (&log_name, CONLOGGROUP);
	Cvar_Register (&log_dir, CONLOGGROUP);
	Cvar_Register (&log_readable, CONLOGGROUP);
	Cvar_Register (&log_enable, CONLOGGROUP);
	Cvar_Register (&log_developer, CONLOGGROUP);
	Cvar_Register (&log_rotate_size, CONLOGGROUP);
	Cvar_Register (&log_rotate_files, CONLOGGROUP);
	Cvar_Register (&log_dosformat, CONLOGGROUP);

	// cmd line options, debug options
#ifdef CRAZYDEBUGGING
	Cvar_ForceSet(&log_enable, "1");
	TRACE(("dbg: Con_Init: log_enable forced\n"));
#endif

	if (COM_CheckParm("-condebug"))
		Cvar_ForceSet(&log_enable, "1");
}
