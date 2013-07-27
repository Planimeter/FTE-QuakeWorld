#include <jni.h>
#include <errno.h>

#include <android/log.h>

#include "quakedef.h"
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <dirent.h>

#ifndef isDedicated
#ifdef SERVERONLY
qboolean isDedicated = true;
#else
qboolean isDedicated = false;
#endif
#endif
void *sys_window; /*public so the renderer can attach to the correct place*/
static int sys_running = false;
int sys_glesversion;
int sys_soundflags;	/*1 means active. 2 means reset (so claim that its not active for one frame to force a reset)*/
static void *sys_memheap;
static unsigned int sys_lastframe;
static unsigned int vibrateduration;
static char errormessage[256];
static char sys_basedir[MAX_OSPATH];
static char sys_basepak[MAX_OSPATH];
extern  jmp_buf 	host_abort;

cvar_t sys_vibrate = CVARD("sys_vibrate", "1", "Enables the system vibrator for damage events and such things. The value provided is a duration scaler.");
cvar_t sys_osk = CVAR("sys_osk", "0");	//to be toggled
cvar_t sys_keepscreenon = CVARD("sys_keepscreenon", "1", "If set, the screen will never darken. This might cost some extra battery power, but then so will running a 3d engine.");	//to be toggled
cvar_t sys_orientation = CVARD("sys_orientation", "landscape", "Specifies what angle to render quake at.\nValid values are: sensor (autodetect), landscape, portrait, reverselandscape, reverseportrait");
cvar_t sys_glesversion_cvar = CVARD("sys_glesversion", "1", "Specifies which version of gles to use. 1 or 2 are valid values.");
extern cvar_t vid_conautoscale;


#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, DISTRIBUTION"Droid", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, DISTRIBUTION"Droid", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, DISTRIBUTION"Droid", __VA_ARGS__))

void Sys_Vibrate(int count)
{
	vibrateduration = count*10*sys_vibrate.value;
}
JNIEXPORT jint JNICALL Java_com_fteqw_FTEDroidEngine_getvibrateduration(JNIEnv *env, jobject obj)
{
	unsigned int dur = vibrateduration;
	vibrateduration = 0;
	return dur;
}

JNIEXPORT jstring JNICALL Java_com_fteqw_FTEDroidEngine_geterrormessage(JNIEnv *env, jobject obj)
{
	return (*env)->NewStringUTF(env, errormessage);
}
JNIEXPORT jstring JNICALL Java_com_fteqw_FTEDroidEngine_getpreferedorientation(JNIEnv *env, jobject obj)
{
	sys_orientation.modified = false;
	sys_glesversion_cvar.modified = false;
	return (*env)->NewStringUTF(env, sys_orientation.string);
}

JNIEXPORT jint JNICALL Java_com_fteqw_FTEDroidEngine_getpreferedglesversion(JNIEnv *env, jobject obj)
{
	return sys_glesversion_cvar.ival;
}

/*the java passes in all input directly via a 'UI' thread. we don't need to poll it at all*/
void INS_Move(float *movements, int pnum)
{
}
void INS_Commands(void)
{
}
void INS_Init(void)
{
}
void INS_ReInit(void)
{
}
void INS_Shutdown(void)
{
}
JNIEXPORT void JNICALL Java_com_fteqw_FTEDroidEngine_keypress(JNIEnv *env, jobject obj,
                 jint down, jint keycode, jint unicode)
{
	IN_KeyEvent(0, down, keycode, unicode);
}
JNIEXPORT void JNICALL Java_com_fteqw_FTEDroidEngine_motion(JNIEnv *env, jobject obj,
                 jint act, jint ptrid, jfloat x, jfloat y, jfloat size)
{
	if (act)
		IN_KeyEvent(ptrid, act==1, K_MOUSE1, 0);
	else
		IN_MouseMove(ptrid, true, x, y, 0, size);
}

JNIEXPORT jint JNICALL Java_com_fteqw_FTEDroidEngine_frame(JNIEnv *env, jobject obj,
				jfloat ax, jfloat ay, jfloat az)
{
	int ret;
	static vec3_t oac;

	//if we had an error, don't even run a frame any more.
	if (*errormessage || !sys_running)
	{
		Sys_Printf("Crashed or quit\n");
		return 8;
	}

	#ifdef SERVERONLY
	SV_Frame();
	#else
	unsigned int now = Sys_Milliseconds();
	double tdelta = (now - sys_lastframe) * 0.001;
	if (oac[0] != ax || oac[1] != ay || oac[2] != az)
	{
		//down: x= +9.8
		//left: y= -9.8
		//up:   z= +9.8
		CSQC_Accelerometer(ax, ay, az);
		oac[0] = ax;
		oac[1] = ay;
		oac[2] = az;
	}
	Host_Frame(tdelta);
	sys_lastframe = now;
	#endif

	ret = 0;
	if (key_dest == key_console || key_dest == key_message || (key_dest == key_game && cls.state == ca_disconnected) || sys_osk.ival)
		ret |= 1;
	if (vibrateduration)
		ret |= 2;
	if (sys_keepscreenon.ival)
		ret |= 4;
	if (*errormessage)
		ret |= 8;
	if (sys_orientation.modified || sys_glesversion_cvar.modified)
		ret |= 16;
	if (sys_soundflags)
	{
		if (sys_soundflags & 2)
			sys_soundflags &= ~2;
		else
			ret |= 32;
	}
	return ret;
}

//tells us that our old gl context got completely obliterated
JNIEXPORT void JNICALL Java_com_fteqw_FTEDroidEngine_newglcontext(JNIEnv *env, jobject obj)
{
	if (sys_running)
		sys_running = 2;

	//fixme: wipe image handles
}

//called when the user tries to use us to open one of our file types
JNIEXPORT void JNICALL Java_com_fteqw_FTEDroidEngine_openfile(JNIEnv *env, jobject obj,
				jstring openfile)
{
	const char *fname = (*env)->GetStringUTFChars(env, openfile, NULL);
	Host_RunFile(fname, strlen(fname), NULL);
	(*env)->ReleaseStringUTFChars(env, openfile, fname);
}

//called for init or resizes
JNIEXPORT void JNICALL Java_com_fteqw_FTEDroidEngine_init(JNIEnv *env, jobject obj,
				jint width, jint height, jint glesversion, jstring japkpath, jstring jusrpath)
{
	const char *tmp;

	if (*errormessage)
		return;

	vid.pixelwidth = width;
	vid.pixelheight = height;
	sys_glesversion = glesversion;
	if (sys_running)
	{
		Sys_Printf("vid size changed\n");
		if (1)//FFS sys_running == 2)
		{
			//if our textures got destroyed, we need to reload them all
			Cmd_ExecuteString("vid_restart\n", RESTRICT_LOCAL);
		}
		else
		{
			//otherwise we just need to set the size properly again.
			Cvar_ForceCallback(&vid_conautoscale);
		}
	}
	else
	{
		const char *args [] =
		{
			"ftedroid",
			"-basepack",
			sys_basepak,	/*filled in later*/
			"",
			""
		};
		char *basepack;
		int align;
		quakeparms_t parms;
		Sys_Printf("reinit\n");
		if (sys_memheap)
			free(sys_memheap);
		memset(&parms, 0, sizeof(parms));
		parms.basedir = sys_basedir;	/*filled in later*/
		parms.argc = 3;
		parms.argv = args;

		tmp = (*env)->GetStringUTFChars(env, japkpath, NULL);
		Q_strncpyz(sys_basepak, tmp, sizeof(sys_basedir));
		(*env)->ReleaseStringUTFChars(env, japkpath, tmp);

		tmp = (*env)->GetStringUTFChars(env, jusrpath, NULL);
		Q_strncpyz(sys_basedir, tmp, sizeof(sys_basedir));
		(*env)->ReleaseStringUTFChars(env, jusrpath, tmp);


		Sys_Printf("Starting up (apk=%s, usr=%s)\n", args[2], parms.basedir);

		COM_InitArgv(parms.argc, parms.argv);
		TL_InitLanguages();
		#ifdef SERVERONLY
			SV_Init(&parms);
		#else
			Host_Init(&parms);
		#endif
		sys_running = true;
		sys_lastframe = Sys_Milliseconds();
		sys_orientation.modified = true;
	}
}

static int secbase;

#ifdef _POSIX_TIMERS
double Sys_DoubleTime(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

	if (!secbase)
	{
		secbase = ts.tv_sec;
		return ts.tv_nsec/1000000000.0;
	}
	return (ts.tv_sec - secbase) + ts.tv_nsec/1000000000.0;
}
unsigned int Sys_Milliseconds(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);

	if (!secbase)
	{
		secbase = ts.tv_sec;
		return ts.tv_nsec/1000000;
	}
	return (ts.tv_sec - secbase)*1000 + ts.tv_nsec/1000000;
}
#else
double Sys_DoubleTime(void)
{
	struct timeval tp;
	struct timezone tzp;

	gettimeofday(&tp, &tzp);

	if (!secbase)
	{
			secbase = tp.tv_sec;
			return tp.tv_usec/1000000.0;
	}

	return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;
}
unsigned int Sys_Milliseconds(void)
{
	struct timeval tp;
	struct timezone tzp;

	gettimeofday(&tp, &tzp);

	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec/1000;
	}

	return (tp.tv_sec - secbase)*1000 + tp.tv_usec/1000;
}
#endif

void Sys_Shutdown(void)
{
	free(sys_memheap);
}
void Sys_Quit(void)
{
#ifndef SERVERONLY
	Host_Shutdown ();
#else
	SV_Shutdown();
#endif

	LOGI("%s", "quitting");

	longjmp(host_abort, 1);
	exit(0);
}
void Sys_Error (const char *error, ...)
{
	va_list         argptr;
	char             string[1024];

	va_start (argptr, error);
	vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);
	if (!*string)
		strcpy(string, "no error");

	Q_strncpyz(errormessage, string, sizeof(errormessage));

	LOGE("%s", string);

	longjmp(host_abort, 1);
	exit(1);
}
void Sys_Printf (char *fmt, ...)
{
	va_list         argptr;
	char             string[1024];

	va_start (argptr, fmt);
	vsnprintf (string,sizeof(string)-1, fmt,argptr);
	va_end (argptr);

	LOGI("%s", string);
}
void Sys_Warn (char *fmt, ...)
{
	va_list         argptr;
	char             string[1024];

	va_start (argptr, fmt);
	vsnprintf (string,sizeof(string)-1, fmt,argptr);
	va_end (argptr);

	LOGW("%s", string);
}

void Sys_CloseLibrary(dllhandle_t *lib)
{
	dlclose(lib);
}
dllhandle_t *Sys_LoadLibrary(const char *name, dllfunction_t *funcs)
{
	dllhandle_t *h;
	h = dlopen(name, RTLD_LAZY);
	return h;
}
void *Sys_GetAddressForName(dllhandle_t *module, const char *exportname)
{
	return dlsym(module, exportname);
}
char *Sys_ConsoleInput (void)
{
	return NULL;
}
void Sys_mkdir (char *path)    //not all pre-unix systems have directories (including dos 1)
{
	mkdir(path, 0777);
}
qboolean Sys_remove (char *path)
{
	return !unlink(path);
}
qboolean Sys_Rename (char *oldfname, char *newfname)
{
	return !rename(oldfname, newfname);
}
void Sys_SendKeyEvents(void)
{
}
void Sys_Init(void)
{
	Cvar_Register(&sys_vibrate, "android stuff");
	Cvar_Register(&sys_osk, "android stuff");
	Cvar_Register(&sys_keepscreenon, "android stuff");
	Cvar_Register(&sys_orientation, "android stuff");
	Cvar_Register(&sys_glesversion_cvar, "android stuff");
}

qboolean Sys_GetDesktopParameters(int *width, int *height, int *bpp, int *refreshrate)
{
	*width = 320;
	*height = 240;
	*bpp = 16;
	*refreshrate = 60;
	return false;
}
qboolean Sys_RandomBytes(qbyte *string, int len)
{
	qboolean res = false;
	int fd = open("/dev/urandom", 0);
	if (fd >= 0)
	{
		res = (read(fd, string, len) == len);
		close(fd);
	}

	return res;
}

void Sys_ServerActivity(void)
{
	/*FIXME: flash window*/
}

qboolean Sys_InitTerminal(void)
{
	/*switching to dedicated mode, show text window*/
	return false;
}
void Sys_CloseTerminal(void)
{
}

char *Sys_GetClipboard(void)
{
	return NULL;
}
void Sys_CloseClipboard(char *buf)
{
}
void Sys_SaveClipboard(char *text)
{
}

int Sys_EnumerateFiles (const char *gpath, const char *match, int (*func)(const char *, int, void *, searchpathfuncs_t *), void *parm, searchpathfuncs_t *spath)
{
	DIR *dir;
	char apath[MAX_OSPATH];
	char file[MAX_OSPATH];
	char truepath[MAX_OSPATH];
	char *s;
	struct dirent *ent;
	struct stat st;

	//printf("path = %s\n", gpath);
	//printf("match = %s\n", match);

	if (!gpath)
		gpath = "";
	*apath = '\0';

	Q_strncpyz(apath, match, sizeof(apath));
	for (s = apath+strlen(apath)-1; s >= apath; s--)
	{
		if (*s == '/')
		{
			s[1] = '\0';
			match += s - apath+1;
			break;
		}
	}
	if (s < apath)  //didn't find a '/' 
		*apath = '\0'; 

	Q_snprintfz(truepath, sizeof(truepath), "%s/%s", gpath, apath); 


	//printf("truepath = %s\n", truepath); 
	//printf("gamepath = %s\n", gpath); 
	//printf("apppath = %s\n", apath); 
	//printf("match = %s\n", match); 
	dir = opendir(truepath); 
	if (!dir) 
	{ 
		Con_DPrintf("Failed to open dir %s\n", truepath); 
		return true; 
	} 
	do 
	{ 
		ent = readdir(dir); 
		if (!ent) 
			break; 
		if (*ent->d_name != '.') 
		{ 
			if (wildcmp(match, ent->d_name)) 
			{ 
				Q_snprintfz(file, sizeof(file), "%s/%s", truepath, ent->d_name); 

				if (stat(file, &st) == 0) 
				{ 
					Q_snprintfz(file, sizeof(file), "%s%s%s", apath, ent->d_name, S_ISDIR(st.st_mode)?"/":""); 

					if (!func(file, st.st_size, parm, spath)) 
					{ 
						closedir(dir); 
						return false; 
					} 
				} 
				else 
					printf("Stat failed for \"%s\"\n", file); 
			} 
		} 
	} while(1); 
	closedir(dir); 

	return true; 
}

#if 0
#include <android/asset_manager.h>
int Sys_EnumerateFiles (const char *gpath, const char *match, int (*func)(const char *, int, void *), void *parm)
{
	qboolean go = true;
	const char *f;

	struct AAssetDir *ad;
	ad = AAssetManager_openDir(assetmgr, gpath);

	while(go && (f = AAssetDir_getNextFileName(ad)))
	{
		if (wildcmp(match, f))
		{
Sys_Printf("Found %s\n", f);
			go = func(f, 0, parm);
		}
	}

	AAssetDir_close(ad);
	return 0;
}

typedef struct
{
	vfsfile_t funcs;
	AAsset *handle;
} assetfile_t;
static int AF_ReadBytes(vfsfile_t *h, void *buf, int len)
{
	assetfile_t *f = (assetfile_t*)h;
	return AAsset_read(f->handle, buf, len);
}
static qboolean AF_Seek(vfsfile_t *h, unsigned long offs)
{
	assetfile_t *f = (assetfile_t*)h;
	AAsset_seek(f->handle, offs, SEEK_SET);
	return true;
}
static unsigned long AF_Tell(vfsfile_t *h)
{
	assetfile_t *f = (assetfile_t*)h;
	return AAsset_seek(f->handle, 0, SEEK_CUR);
}
static unsigned long AF_GetSize(vfsfile_t *h)
{
	assetfile_t *f = (assetfile_t*)h;
	return AAsset_getLength(f->handle);
}

static void AF_Close(vfsfile_t *h)
{
	assetfile_t *f = (assetfile_t*)h;
	AAsset_close(f->handle);
	Z_Free(f);
}
static void AF_Flush(vfsfile_t *h)
{
}
vfsfile_t *Sys_OpenAsset(char *fname)
{
	assetfile_t *file;
	AAsset *a;
	a = AAssetManager_open(assetmgr, fname, AASSET_MODE_UNKNOWN);
	if (!a)
	{
		Sys_Printf("Unable to open asset %s\n", fname);
		return NULL;
	}
	Sys_Printf("opened asset %s\n", fname);

        file = Z_Malloc(sizeof(assetfile_t));
        file->funcs.ReadBytes = AF_ReadBytes;
        file->funcs.WriteBytes = NULL;
        file->funcs.Seek = AF_Seek;
        file->funcs.Tell = AF_Tell;
        file->funcs.GetLen = AF_GetSize;
        file->funcs.Close = AF_Close;
        file->funcs.Flush = AF_Flush;
        file->handle = a;

        return (vfsfile_t*)file;
}
#endif
