// cmdlib.h

#ifndef __CMDLIB__
#define __CMDLIB__

#include "progsint.h"

/*#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <setjmp.h>
#include <io.h>

#ifdef NeXT
#include <libc.h>
#endif
*/

// the dec offsetof macro doesn't work very well...
#define myoffsetof(type,identifier) ((size_t)&((type *)NULL)->identifier)


// set these before calling CheckParm
extern int myargc;
extern char **myargv;

//char *strupr (char *in);
//char *strlower (char *in);
int QCC_filelength (int handle);
int QCC_tell (int handle);

int QC_strcasecmp (const char *s1, const char *s2);

#ifdef _MSC_VER
#define QC_vsnprintf _vsnprintf
static void VARGS QC_snprintfz (char *dest, size_t size, const char *fmt, ...)
{
	va_list args;
	va_start (args, fmt);
	vsnprintf (dest, size-1, fmt, args);
	va_end (args);
	//make sure its terminated.
	dest[size-1] = 0;
}
#else
#define QC_vsnprintf vsnprintf
#define QC_snprintfz snprintf
#endif

#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
	#ifndef LIKEPRINTF
		#define LIKEPRINTF(x) __attribute__((format(printf,x,x+1)))
	#endif
#endif
#ifndef LIKEPRINTF
#define LIKEPRINTF(x)
#endif

double I_FloatTime (void);

void	VARGS QCC_Error (int errortype, const char *error, ...) LIKEPRINTF(2);
int		CheckParm (char *check);


int 	SafeOpenWrite (char *filename, int maxsize);
int 	SafeOpenRead (char *filename);
void 	SafeRead (int handle, void *buffer, long count);
void 	SafeWrite (int handle, void *buffer, long count);
pbool	SafeClose(int hand);
int SafeSeek(int hand, int ofs, int mode);
void 	*SafeMalloc (long size);


long	QCC_LoadFile (char *filename, void **bufferptr);
void	QCC_SaveFile (char *filename, void *buffer, long count);

void 	DefaultExtension (char *path, char *extension);
void 	DefaultPath (char *path, char *basepath);
void 	StripFilename (char *path);
void 	StripExtension (char *path);

void 	ExtractFilePath (char *path, char *dest);
void 	ExtractFileBase (char *path, char *dest);
void	ExtractFileExtension (char *path, char *dest);

long 	ParseNum (char *str);


char *QCC_COM_Parse (char *data);
char *QCC_COM_Parse2 (char *data);

extern	char	qcc_token[1024];
extern	int		qcc_eof;


#define qcc_iswhite(c) ((c) == ' ' || (c) == '\r' || (c) == '\n' || (c) == '\t' || (c) == '\v')
#define qcc_iswhitesameline(c) ((c) == ' ' || (c) == '\t')


#endif
