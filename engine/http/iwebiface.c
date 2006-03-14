#include "bothdefs.h"

#ifdef WEBSVONLY
	#define WEBSERVER
#else
	#include "bothdefs.h"
#endif

#ifdef WEBSERVER

#include "iweb.h"

#ifdef WEBSVONLY	//we need some functions from quake

/*char	*va(char *format, ...)
{
#define VA_BUFFERS 2 //power of two
	va_list		argptr;
	static char		string[VA_BUFFERS][1024];
	static int bufnum;

	bufnum++;
	bufnum &= (VA_BUFFERS-1);
	
	va_start (argptr, format);
	_vsnprintf (string[bufnum],sizeof(string[bufnum])-1, format,argptr);
	va_end (argptr);

	return string[bufnum];	
}*/

void Sys_Error(char *format, ...)
{
	va_list		argptr;
	char		string[1024];
	
	va_start (argptr, format);
	_vsnprintf (string,sizeof(string)-1, format,argptr);
	va_end (argptr);

	printf("%s", string);
	getchar();
	exit(1000);
}

int COM_CheckParm(char *parm)
{
	return 0;
}

char *authedusername;
char *autheduserpassword;
int main(int argc, char **argv)
{
	WSADATA pointlesscrap;
	WSAStartup(2, &pointlesscrap);

	if (argc == 3)
	{
		authedusername = argv[1];
		autheduserpassword = argv[2];
		printf("Username = \"%s\"\nPassword = \"%s\"\n", authedusername, autheduserpassword);
	}
	else
		printf("Server is read only\n");

	while(1)
	{
		FTP_ServerRun(1);
		HTTP_ServerPoll(1);
		if (ftpserverfailed || httpserverfailed)
			Sys_Error("FTP/HTTP server failed");
		Sleep(1);
	}
}


void COM_EnumerateFiles (char *match, int (*func)(char *, int, void *), void *parm)
{
	HANDLE r;
	WIN32_FIND_DATA fd;	
	char apath[MAX_OSPATH];
	char file[MAX_OSPATH];
	char *s;
	int go;
	strcpy(apath, match);
//	sprintf(apath, "%s%s", gpath, match);
	for (s = apath+strlen(apath)-1; s>= apath; s--)
	{
		if (*s == '/')			
			break;
	}
	s++;
	*s = '\0';	
	
	strcpy(file, match);
	r = FindFirstFile(file, &fd);
	if (r==(HANDLE)-1)
		return;
    go = true;
	do
	{
		if (*fd.cFileName == '.');
		else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)	//is a directory
		{
			sprintf(file, "%s%s/", apath, fd.cFileName);
			go = func(file, fd.nFileSizeLow, parm);
		}
		else
		{
			sprintf(file, "%s%s", apath, fd.cFileName);
			go = func(file, fd.nFileSizeLow, parm);
		}
	}
	while(FindNextFile(r, &fd) && go);
	FindClose(r);
}



enum {TTP_UNKNOWN, TTP_STRING} com_tokentype;
char *COM_ParseOut (char *data, char *out, int outlen)
{
	int		c;
	int		len;
	
	len = 0;
	out[0] = 0;
	
	if (!data)
		return NULL;
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;			// end of file;
		data++;
	}
	
// skip // comments
	if (c=='/')
	{
		if (data[1] == '/')
		{
			while (*data && *data != '\n')
				data++;
			goto skipwhite;
		}
	}
	

// handle quoted strings specially
	if (c == '\"')
	{
		com_tokentype = TTP_STRING;
		data++;
		while (1)
		{
			if (len >= outlen-1)
				return data;

			c = *data++;
			if (c=='\"' || !c)
			{
				out[len] = 0;
				return data;
			}
			out[len] = c;
			len++;
		}
	}

	com_tokentype = TTP_UNKNOWN;

// parse a regular word
	do
	{
		if (len >= outlen-1)
			return data;

		out[len] = c;
		data++;
		len++;
		c = *data;
	} while (c>32);
	
	out[len] = 0;
	return data;
}

char com_token[2048];
char *COM_ParseToken (char *data)
{
	int		c;
	int		len;
	
	len = 0;
	com_token[0] = 0;
	
	if (!data)
		return NULL;
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
			return NULL;			// end of file;
		data++;
	}
	
// skip // comments
	if (c=='/')
	{
		if (data[1] == '/')
		{
			while (*data && *data != '\n')
				data++;
			goto skipwhite;
		}
		else if (data[1] == '*')
		{
			data+=2;
			while (*data && (*data != '*' || data[1] != '/'))
				data++;
			data+=2;
			goto skipwhite;
		}
	}
	

// handle quoted strings specially
	if (c == '\"')
	{
		com_tokentype = TTP_STRING;
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				return data;
			}
			com_token[len] = c;
			len++;
		}
	}

	com_tokentype = TTP_UNKNOWN;

// parse single characters
	if (c==',' || c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':' || c==';' || c == '=' || c == '!' || c == '>' || c == '<' || c == '&' || c == '|' || c == '+')
	{
		com_token[len] = c;
		len++;
		com_token[len] = 0;
		return data+1;
	}

// parse a regular word
	do
	{
		com_token[len] = c;
		data++;
		len++;
		c = *data;
		if (c==',' || c=='{' || c=='}'|| c==')'|| c=='(' || c=='\'' || c==':' || c==';' || c == '=' || c == '!' || c == '>' || c == '<' || c == '&' || c == '|' || c == '+')
			break;
	} while (c>32);
	
	com_token[len] = 0;
	return data;
}


IWEBFILE *IWebFOpenRead(char *name)					//fread(name, "rb");
{
	FILE *f;
	char name2[512];
	if (strstr(name, ".."))
		return NULL;
	sprintf(name2, "%s/%s", com_gamedir, name);
	f = fopen(name2, "rb");
	if (f)
	{
		IWEBFILE *ret = IWebMalloc(sizeof(IWEBFILE));
		if (!ret)
		{
			fclose(f);
			return NULL;
		}
		ret->f = f;
		ret->start = 0;

		fseek(f, 0, SEEK_END);
		ret->end = ftell(f);//ret->start+ret->length;
		fseek(f, 0, SEEK_SET);

		ret->length = ret->end - ret->start;
		return ret;
	}
	return NULL;
}



#else

#ifndef CLIENTONLY
cvar_t ftpserver = SCVAR("sv_ftp", "0");
cvar_t httpserver = SCVAR("sv_http", "0");
cvar_t sv_readlevel = SCVAR("sv_readlevel", "0");	//default to allow anyone
cvar_t sv_writelevel = SCVAR("sv_writelevel", "35");	//allowed to write to uploads/uname
cvar_t sv_fulllevel = SCVAR("sv_fulllevel", "51");	//allowed to write anywhere, replace any file...
#endif

//this file contains functions called from each side.

void VARGS IWebDPrintf(char *fmt, ...)
{
	va_list		argptr;
	char		msg[4096];

	if (!developer.value)
		return;
	
	va_start (argptr,fmt);
	vsnprintf (msg,sizeof(msg)-10, fmt,argptr);	//catch any nasty bugs... (this is hopefully impossible)
	va_end (argptr);

	Con_Printf("%s", msg);
}
void VARGS IWebPrintf(char *fmt, ...)
{
	va_list		argptr;
	char		msg[4096];

	va_start (argptr,fmt);
	vsnprintf (msg,sizeof(msg)-10, fmt,argptr);	//catch any nasty bugs... (this is hopefully impossible)
	va_end (argptr);

	Con_Printf("%s", msg);
}
void VARGS IWebWarnPrintf(char *fmt, ...)
{
	va_list		argptr;
	char		msg[4096];

	va_start (argptr,fmt);
	vsnprintf (msg,sizeof(msg)-10, fmt,argptr);	//catch any nasty bugs... (this is hopefully impossible)
	va_end (argptr);

	Con_Printf(S_WARNING "%s", msg);
}

void IWebInit(void)
{
#ifdef WEBSERVER
	Cvar_Register(&sv_fulllevel, "Internet Server Access");
	Cvar_Register(&sv_writelevel, "Internet Server Access");
	Cvar_Register(&sv_readlevel, "Internet Server Access");

	Cvar_Register(&ftpserver, "Internet Server Access");
	Cvar_Register(&httpserver, "Internet Server Access");
#endif
}
void IWebRun(void)
{
#ifdef WEBSERVER
	extern qboolean httpserverfailed, ftpserverfailed;

	FTP_ServerRun(ftpserver.value!= 0);
	HTTP_ServerPoll(httpserver.value!=0);
	if (ftpserverfailed)
	{
		Con_Printf("FTP Server failed to load, setting %s to 0\n", ftpserver.name);
		Cvar_SetValue(&ftpserver, 0);
		ftpserverfailed = false;
	}
	if (httpserverfailed)
	{
		Con_Printf("HTTP Server failed to load, setting %s to 0\n", httpserver.name);
		Cvar_SetValue(&httpserver, 0);
		httpserverfailed = false;
	}
#endif
}
void IWebShutdown(void)
{
}
#endif

#ifndef WEBSVONLY
//replacement for Z_Malloc. It simply allocates up to a reserve ammount.
void *IWebMalloc(int size)
{
	char *mem = Z_TagMalloc(size+32768, 15);
	if (!mem)
		return NULL;	//bother

	Z_Free(mem);
	return Z_Malloc(size);	//allocate the real ammount
}

void *IWebRealloc(void *old, int size)
{
	char *mem = Z_TagMalloc(size+32768, 15);
	if (!mem)	//make sure there will be padding left
		return NULL;	//bother

	Z_Free(mem);
	return BZ_Realloc(old, size);
}
#endif




int IWebAuthorize(char *name, char *password)
{
#ifdef WEBSVONLY
	if (authedusername)
		if (!strcmp(name, authedusername))
			if (!strcmp(password, autheduserpassword))
				return IWEBACC_FULL;
	return IWEBACC_READ;
#else
#ifndef CLIENTONLY
	int id = Rank_GetPlayerID(name, atoi(password), false, true);
	rankinfo_t info;
	if (!id)
	{
		if (!sv_readlevel.value)
			return IWEBACC_READ;	//read only anywhere
		return 0;
	}

	Rank_GetPlayerInfo(id, &info);

	if (info.s.trustlevel >= sv_fulllevel.value)
		return IWEBACC_READ	| IWEBACC_WRITE | IWEBACC_FULL;	//allowed to read and write anywhere to the quake filesystem
	if (info.s.trustlevel >= sv_writelevel.value)
		return IWEBACC_READ	| IWEBACC_WRITE;	//allowed to read anywhere write to specific places
	if (info.s.trustlevel >= sv_readlevel.value)
		return IWEBACC_READ;	//read only anywhere
#endif
	return 0;
#endif
}

iwboolean IWebAllowUpLoad(char *fname, char *uname)	//called for partial write access
{
	if (strstr(fname, ".."))
		return false;
	if (!strncmp(fname, "uploads/", 8))
	{
		if (!strncmp(fname+8, uname, strlen(uname)))
			if (fname[8+strlen(uname)] == '/')
				return true;
	}
	return false;
}

iwboolean	FTP_StringToAdr (const char *s, qbyte ip[4], qbyte port[2])
{
	s = COM_ParseToken(s, NULL);
	ip[0] = atoi(com_token);

	s = COM_ParseToken(s, NULL);
	if (*com_token != ',')
		return false;

	s = COM_ParseToken(s, NULL);
	ip[1] = atoi(com_token);

	s = COM_ParseToken(s, NULL);
	if (*com_token != ',')
		return false;

	s = COM_ParseToken(s, NULL);
	ip[2] = atoi(com_token);

	s = COM_ParseToken(s, NULL);
	if (*com_token != ',')
		return false;
	
	s = COM_ParseToken(s, NULL);
	ip[3] = atoi(com_token);

	s = COM_ParseToken(s, NULL);
	if (*com_token != ',')
		return false;
	
	s = COM_ParseToken(s, NULL);
	port[0] = atoi(com_token);

	s = COM_ParseToken(s, NULL);
	if (*com_token != ',')
		return false;
	
	s = COM_ParseToken(s, NULL);
	port[1] = atoi(com_token);


	return true;
}

char *Q_strcpyline(char *out, char *in, int maxlen)
{
	char *w = out;
	while (*in && maxlen > 0)
	{
		if (*in == '\r' || *in == '\n')
			break;
		*w = *in;
		in++;
		w++;
		maxlen--;
	}
	*w = '\0';
	return out;
}

#endif
