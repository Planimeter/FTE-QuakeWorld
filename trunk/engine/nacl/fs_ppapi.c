#include "quakedef.h"
#include "fs.h"

#include <ppapi/c/pp_errors.h>
#include <ppapi/c/ppb_core.h>
#include <ppapi/c/pp_file_info.h>
#include <ppapi/c/ppb_file_system.h>
#include <ppapi/c/ppb_file_ref.h>
#include <ppapi/c/ppb_url_request_info.h>
#include <ppapi/c/ppb_url_response_info.h>
#include <ppapi/c/pp_var.h>
#include <ppapi/c/ppb_var.h>
#include <ppapi/c/ppb_file_io.h>
#include <ppapi/c/ppb_url_loader.h>

extern PPB_FileIO *ppb_fileio;
extern PPB_FileRef *ppb_fileref;
extern PPB_FileSystem *ppb_filesystem;
extern PPB_Core *ppb_core;
extern PPB_URLLoader  *urlloader;
extern PPB_URLRequestInfo *urlrequestinfo;
extern PPB_URLResponseInfo *urlresponseinfo;
extern PPB_Var *ppb_var_interface;
extern PP_Instance pp_instance;

#define GOOGLE_DONT_KNOW_HOW_TO_CREATE_USABLE_APIS
#ifdef GOOGLE_DONT_KNOW_HOW_TO_CREATE_USABLE_APIS
//the pepper api is flawed. deeply flawed.
//1: api calls (including various gl calls) may only be made on the main thread).
//2: the main thread may not use non-asyncronous calls.
//the recommendation to get around this is to run everything on a worker thread, but to make calls to the main thread to do the actual call, then signal the worker thread to wake up again in the main thread's callback.
//which is impractical when you have 80+ different sorts of performance-dependant gl calls.
//I can't easily put things requiring file access on another thread, if only because it would make alias/exec console commands non-synchronous
//to get around this, I instead make my own memory-only 'filesystem', populating it at startup with downloads. I *really* hope your browser/server are set to enable file caching.
//at some point I'll periodically write the data back to a persistant store/reload it at startup so saved games/configs can work

#define FSPPAPI_OpenTemp FS_OpenTemp
#define VFSPPAPI_Open VFSOS_Open

typedef struct mfchunk_s
{
	struct mfchunk_s *prev;
	struct mfchunk_s *next;
	unsigned long startpos;
	unsigned int len;
	char data[64];
} mfchunk_t;
typedef struct mfile_s
{
	/*chunks can be trimmed only when there's no refs*/
	char name[MAX_QPATH];
	int refs;
	int unlinked:1;
	unsigned long length;

	mfchunk_t *chunkhead;
	mfchunk_t *chunktail;

	struct mfile_s *prev;
	struct mfile_s *next;
} mfile_t;
mfile_t *mfiles;

typedef struct
{
	vfsfile_t funcs;

	mfile_t *file;
	unsigned long offset;
	mfchunk_t *cchunk;
} vfsmfile_t;

qboolean FSPPAPI_Init(int *fileid)
{
	return true;	/*engine has all the content it needs*/
}

static int preparechunk(vfsmfile_t *f, int bytes, void **data)
{
	int sz;
	mfchunk_t *cnk;
	if (!bytes)
	{
		*data = 0;
		return 0;
	}

	if (!f->cchunk)
		cnk = f->file->chunkhead;
	else
	{
		cnk = f->cchunk;
		//rewind through the chunks
		while (cnk->startpos > f->offset)
			cnk = cnk->prev;
	}
	//find the chunk that contains our start offset
	while (!cnk || cnk->startpos+cnk->len <= f->offset)
	{
		if (!cnk)
		{
			sz = (bytes + sizeof(*cnk) - sizeof(cnk->data) + 4095) & ~4095;
			if (sz < 65536)
				sz = 65536;
			cnk = malloc(sz);
			memset(cnk, 0xcc, sz);
			if (!cnk)
			{
				*data = 0;
				return 0;
			}
			cnk->len = (sz + sizeof(cnk->data) - sizeof(*cnk));
			cnk->next = NULL;
			if (f->file->chunktail)
			{
				cnk->prev = f->file->chunktail;
				cnk->prev->next = cnk;
				cnk->startpos = cnk->prev->startpos + cnk->prev->len;
			}
			else
			{
				f->file->chunkhead = cnk;
				cnk->prev = NULL;
				cnk->startpos = 0;
			}
//			Sys_Printf("Allocated chunk %p: %u-%u\n", cnk, cnk->startpos, cnk->startpos + cnk->len);
			f->file->chunktail = cnk;
		}
		else
			cnk = cnk->next;
	}

//	Sys_Printf("Returning offset %p, %i\n", cnk, (f->offset - cnk->startpos));
//	Sys_Printf("%u %u\n", f->offset, cnk->startpos);

	*data = cnk->data + (f->offset - cnk->startpos);
	f->cchunk = cnk;

	sz = cnk->startpos + cnk->len - f->offset;
	if (sz > bytes)
	{
//		Sys_Printf("Returning len %u\n", bytes);
		return bytes;
	}
//	Sys_Printf("Returning len %u\n", sz);
	return sz;

}

static int VFSMEM_ReadBytes (struct vfsfile_s *file, void *buffer, int bytestoread)
{
	int total = 0;
	int chunklen;
	void *chunkdat;
	vfsmfile_t *f = (vfsmfile_t*)file;

	if (bytestoread > f->file->length - f->offset)
		bytestoread = f->file->length - f->offset;

	while ((chunklen = preparechunk(f, bytestoread, &chunkdat)) > 0)
	{
//		Sys_Printf("Read %i at %u\n", chunklen, f->offset);
		memcpy(buffer, chunkdat, chunklen);
		buffer = (char*)buffer + chunklen;
		bytestoread -= chunklen;
		total += chunklen;
		f->offset += chunklen;
	}

//	Sys_Printf("%s", (char*)buffer-total);
	return total;
}
static int VFSMEM_WriteBytes (struct vfsfile_s *file, const void *buffer, int bytestoread)
{
	int total = 0;
	int chunklen;
	void *chunkdat;
	vfsmfile_t *f = (vfsmfile_t*)file;

	while ((chunklen = preparechunk(f, bytestoread, &chunkdat)) > 0)
	{
//		Sys_Printf("Write %i at %u\n", chunklen, f->offset);
		memcpy(chunkdat, buffer, chunklen);
		buffer = (char*)buffer + chunklen;
		bytestoread -= chunklen;
		total += chunklen;
		f->offset += chunklen;
	}

	if (f->file->length < f->offset)
		f->file->length = f->offset;

//	Sys_Printf("written: %i, file is now at %i\n", total, f->offset);
	return total;
}
static qboolean VFSMEM_Seek (struct vfsfile_s *file, unsigned long pos)
{
	vfsmfile_t *f = (vfsmfile_t*)file;
	f->offset = pos;
	return true;
}
static unsigned long VFSMEM_Tell (struct vfsfile_s *file)
{
	vfsmfile_t *f = (vfsmfile_t*)file;
	return f->offset;
}
static unsigned long VFSMEM_GetSize (struct vfsfile_s *file)
{
	vfsmfile_t *f = (vfsmfile_t*)file;
	return f->file->length;
}
static void VFSMEM_Close(vfsfile_t *file)
{
	vfsmfile_t *f = (vfsmfile_t*)file;
	f->file->refs -= 1;
	if (!f->file->refs)
	{
		if (f->file->unlinked)
		{
			mfchunk_t *cnk;
			while (f->file->chunkhead)
			{
				cnk = f->file->chunkhead->next;
				free(f->file->chunkhead);
				f->file->chunkhead = cnk;
			}
			free(f->file);
		}
	}
	free(f);
}
static void VFSMEM_Flush(struct vfsfile_s *file)
{
//	vfsmfile_t *f = (vfsmfile_t*)file;
}


vfsfile_t *FSPPAPI_OpenTemp(void)
{
	/*create a file which is already unlinked*/
	mfile_t *f;
	vfsmfile_t *r;
	f = malloc(sizeof(*f));
	if (!f)
		return NULL;
	
	strcpy(f->name, "some temp file");
	f->refs = 0;
	f->unlinked = true;
	f->length = 0;
	f->next = NULL;
	f->prev = NULL;
	f->chunkhead = NULL;
	f->chunktail = NULL;
	
	r = malloc(sizeof(*r));
	if (!r)
		return NULL;
	r->file = f;
	r->offset = 0;
	r->cchunk = NULL;
	f->refs++;

	r->funcs.ReadBytes = VFSMEM_ReadBytes;
	r->funcs.WriteBytes = VFSMEM_WriteBytes;
	r->funcs.Seek = VFSMEM_Seek;
	r->funcs.Tell = VFSMEM_Tell;
	r->funcs.GetLen = VFSMEM_GetSize;
	r->funcs.Close = VFSMEM_Close;
	r->funcs.Flush = VFSMEM_Flush;

	return &r->funcs;
}

vfsfile_t *VFSPPAPI_Open(const char *osname, const char *mode)
{
	mfile_t *f;
	vfsmfile_t *r;

	if (strlen(osname) >= sizeof(f->name))
		return NULL;

	for (f = mfiles; f; f = f->next)
	{
		if (!strcmp(f->name, osname))
			break;
	}
	if (!f && (*mode == 'w' || *mode == 'a'))
	{
		f = malloc(sizeof(*f));
		if (f)
		{
			strcpy(f->name, osname);
			f->refs = 0;
			f->unlinked = false;
			f->length = 0;
			f->next = mfiles;
			f->prev = NULL;
			if (mfiles)
				mfiles->prev = f;
			mfiles = f;
			f->chunkhead = NULL;
			f->chunktail = NULL;
		}
	}
	if (!f)
		return NULL;

	r = malloc(sizeof(*r));
	if (!r)
		return NULL;
	r->file = f;
	r->offset = 0;
	r->cchunk = NULL;
	f->refs++;

	r->funcs.ReadBytes = VFSMEM_ReadBytes;
	r->funcs.WriteBytes = VFSMEM_WriteBytes;
	r->funcs.Seek = VFSMEM_Seek;
	r->funcs.Tell = VFSMEM_Tell;
	r->funcs.GetLen = VFSMEM_GetSize;
	r->funcs.Close = VFSMEM_Close;
	r->funcs.Flush = VFSMEM_Flush;

	return &r->funcs;
}

static vfsfile_t *FSPPAPI_OpenVFS(void *handle, flocation_t *loc, const char *mode)
{
	char diskname[MAX_OSPATH];

	//path is already cleaned, as anything that gets a valid loc needs cleaning up first.

	snprintf(diskname, sizeof(diskname), "%s/%s", (char*)handle, loc->rawname);

	return VFSPPAPI_Open(diskname, mode);
}

static void FSPPAPI_GetDisplayPath(void *handle, char *outpath, unsigned int pathsize)
{
	Q_strncpyz(outpath, (char*)handle, pathsize);
}
static void FSPPAPI_ClosePath(void *handle)
{
	Z_Free(handle);
}

int Sys_EnumerateFiles (const char *rootpath, const char *match, int (*func)(const char *, int, void *, void *), void *parm, void *spath)
{
	int rootlen = strlen(rootpath);
	char *sub;
	mfile_t *f;
	if (*match == '/')
		match++;
	for (f = mfiles; f; f = f->next)
	{
		sub = f->name;
		if (strncmp(sub, rootpath, rootlen))
			continue;
		sub += rootlen;
		if (*sub == '/')
			sub++;
		if (wildcmp(match, sub))
		{
			if (!func(sub, f->length, parm, spath))
				return false;
		}
	}
	return true;
}
static int FSPPAPI_EnumerateFiles (void *handle, const char *match, int (*func)(const char *, int, void *, void *), void *parm, void *spath)
{
	return Sys_EnumerateFiles((char*)handle, match, func, parm, spath);
}

static int FSPPAPI_RebuildFSHash(const char *filename, int filesize, void *data, void *handle)
{
	if (filename[strlen(filename)-1] == '/')
	{	//this is actually a directory

		char childpath[256];
		Q_snprintfz(childpath, sizeof(childpath), "%s*", filename);
		Sys_EnumerateFiles((char*)data, childpath, FSPPAPI_RebuildFSHash, data, handle);
		return true;
	}
	FS_AddFileHash(0, filename, NULL, handle);
	return true;
}
static void FSPPAPI_BuildHash(void *handle, int depth)
{
	Sys_EnumerateFiles(handle, "*", FSPPAPI_RebuildFSHash, handle, handle);
}

/*
void debugfs(void)
{
	static qboolean firstrun = true;
	int len;
	FILE *f;
	vfsfile_t *v;
	char *buf;
	if (!firstrun)
		return;
	firstrun = false;
	f = fopen("C:/Games/Quake/fte_trunk/pub/id1/pak0.pak", "rb");
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);
	buf = malloc(len);
	fread(buf, 1, len, f);
	fclose(f);
	v = VFSPPAPI_Open("C:\\Games\\Quake/id1/pak0.pak", "wb");
	VFS_WRITE(v, buf, len);
	free(buf);
	VFS_CLOSE(v);
}
*/

static qboolean FSPPAPI_FLocate(void *handle, flocation_t *loc, const char *filename, void *hashedresult)
{
	int len;
	char netpath[MAX_OSPATH];

	if (hashedresult && (void *)hashedresult != handle)
		return false;

/*
	if (!static_registered)
	{	// if not a registered version, don't ever go beyond base
		if ( strchr (filename, '/') || strchr (filename,'\\'))
			continue;
	}
*/

// check a file in the directory tree
	snprintf (netpath, sizeof(netpath)-1, "%s/%s",(char*)handle, filename);

	{
		vfsfile_t *f = VFSPPAPI_Open(netpath, "rb");
		if (!f)
			return false;
		len = VFS_GETLEN(f);
		VFS_CLOSE(f);
	}

	if (loc)
	{
		loc->len = len;
		loc->offset = 0;
		loc->index = 0;
		Q_strncpyz(loc->rawname, filename, sizeof(loc->rawname));
	}

	return true;
}
static void FSPPAPI_ReadFile(void *handle, flocation_t *loc, char *buffer)
{
	vfsfile_t *f;
	size_t result;

	f = VFSPPAPI_Open(loc->rawname, "rb");
	if (!f)	//err...
		return;
	VFS_SEEK(f, loc->offset);
	result = VFS_READ(f, buffer, loc->len);

	if (result != loc->len)
		Con_Printf("FSPPAPI_ReadFile() fread: Filename: %s, expected %i, result was %u\n",loc->rawname,loc->len,(unsigned int)result);

	VFS_CLOSE(f);
}

static void *FSPPAPI_OpenPath(vfsfile_t *mustbenull, const char *desc)
{
	char *np;
	int dlen = strlen(desc);
	if (mustbenull)
		return NULL;
	np = Z_Malloc(dlen+1);
	if (np)
	{
		memcpy(np, desc, dlen+1);
	}
	return np;
}

searchpathfuncs_t osfilefuncs = {
	FSPPAPI_GetDisplayPath,
	FSPPAPI_ClosePath,
	FSPPAPI_BuildHash,
	FSPPAPI_FLocate,
	FSPPAPI_ReadFile,
	FSPPAPI_EnumerateFiles,
	FSPPAPI_OpenPath,
	NULL,
	FSPPAPI_OpenVFS
};

#else
#define FSPPAPI_OpenTemp FS_OpenTemp
#define VFSPPAPI_Open VFSOS_Open

extern PPB_FileIO *ppb_fileio;
extern PPB_FileRef *ppb_fileref;
extern PPB_FileSystem *ppb_filesystem;
extern PPB_Core *ppb_core;
extern PP_Instance pp_instance;
static PP_Resource mainfilesystem;


struct PP_CompletionCallback nullccb;

void FSPPAPI_Init(void)
{
	mainfilesystem = ppb_filesystem->Create(pp_instance, PP_FILESYSTEMTYPE_LOCALPERSISTENT);
	ppb_filesystem->Open(mainfilesystem, 100*1024*1024, nullccb);
}

typedef struct {
	vfsfile_t funcs;
	PP_Resource handle;
	int64_t offset;
} vfsppapifile_t;
static int VFSPPAPI_ReadBytes (struct vfsfile_s *file, void *buffer, int bytestoread)
{
	int res;
	vfsppapifile_t *intfile = (vfsppapifile_t*)file;
	
	res = ppb_fileio->Read(intfile->handle, intfile->offset, buffer, bytestoread, nullccb);
	if (res > 0)
		intfile->offset += res;
	return res;
}
static int VFSPPAPI_WriteBytes (struct vfsfile_s *file, const void *buffer, int bytestoread)
{
	int res;
	vfsppapifile_t *intfile = (vfsppapifile_t*)file;
	res = ppb_fileio->Write(intfile->handle, intfile->offset, buffer, bytestoread, nullccb);
	if (res > 0)
		intfile->offset += res;
	return res;
}
static qboolean VFSPPAPI_Seek (struct vfsfile_s *file, unsigned long pos)
{
	vfsppapifile_t *intfile = (vfsppapifile_t*)file;
	intfile->offset = pos;
	return true;
}
static unsigned long VFSPPAPI_Tell (struct vfsfile_s *file)
{
	vfsppapifile_t *intfile = (vfsppapifile_t*)file;
	return intfile->offset;
}
static void VFSPPAPI_Flush(struct vfsfile_s *file)
{
	vfsppapifile_t *intfile = (vfsppapifile_t*)file;
	ppb_fileio->Flush(intfile->handle, nullccb);
}
static unsigned long VFSPPAPI_GetSize (struct vfsfile_s *file)
{
	vfsppapifile_t *intfile = (vfsppapifile_t*)file;
	struct PP_FileInfo fileinfo;

	fileinfo.size = 0;
	ppb_fileio->Query(intfile->handle, &fileinfo, nullccb);
	return fileinfo.size;
}
static void VFSPPAPI_Close(vfsfile_t *file)
{
	vfsppapifile_t *intfile = (vfsppapifile_t*)file;
	ppb_fileio->Close(intfile->handle);
	ppb_core->ReleaseResource(intfile->handle);
	Z_Free(file);
}

#ifdef _WIN32
static void VFSPPAPI_CloseTemp(vfsfile_t *file)
{
	vfsppapifile_t *intfile = (vfsppapifile_t*)file;
	char *fname = (char*)(intfile+1); 
	ppb_fileio->Close(intfile->handle);
	ppb_core->ReleaseResource(intfile->handle);
	/*FIXME: add the remove somewhere*/
//	_unlink(fname);
	Z_Free(file);
}
#endif

vfsfile_t *FSPPAPI_OpenTemp(void)
{
	return NULL;
#if 0
	FILE *f;
	vfsppapifile_t *file;

	f = tmpfile();
	if (!f)
		return NULL;

	file = Z_Malloc(sizeof(vfsppapifile_t));
	file->funcs.Close = VFSPPAPI_Close;
	file->funcs.ReadBytes = VFSPPAPI_ReadBytes;
	file->funcs.WriteBytes = VFSPPAPI_WriteBytes;
	file->funcs.Seek = VFSPPAPI_Seek;
	file->funcs.Tell = VFSPPAPI_Tell;
	file->funcs.GetLen = VFSPPAPI_GetSize;
	file->funcs.Flush = VFSPPAPI_Flush;
	file->handle = f;

	return (vfsfile_t*)file;
#endif
}

vfsfile_t *VFSPPAPI_Open(const char *osname, const char *mode)
{
	int e;
	PP_Resource f;
	PP_Resource fsf;
	vfsppapifile_t *file;
	qboolean read = !!strchr(mode, 'r');
	qboolean write = !!strchr(mode, 'w');
	qboolean append = !!strchr(mode, 'a');
	int newmode = 0;

	if (read)
		newmode |= PP_FILEOPENFLAG_READ;
	if (write)
		newmode |= PP_FILEOPENFLAG_WRITE|PP_FILEOPENFLAG_TRUNCATE|PP_FILEOPENFLAG_CREATE;
	if (append)
		newmode |= PP_FILEOPENFLAG_WRITE|PP_FILEOPENFLAG_CREATE;

	/*should we support w+ or r+ */

	fsf = ppb_fileref->Create(mainfilesystem, osname);
	f = ppb_fileio->Create(pp_instance);
	e = ppb_fileio->Open(f, fsf, newmode, nullccb);
	ppb_core->ReleaseResource(fsf);

	if (e != PP_OK)
	{
		Con_Printf("unable to open %s. error %i\n", osname, e);
		return NULL;
	}

	file = Z_Malloc(sizeof(vfsppapifile_t));
	file->funcs.ReadBytes = strchr(mode, 'r')?VFSPPAPI_ReadBytes:NULL;
	file->funcs.WriteBytes = (strchr(mode, 'w')||strchr(mode, 'a'))?VFSPPAPI_WriteBytes:NULL;
	file->funcs.Seek = VFSPPAPI_Seek;
	file->funcs.Tell = VFSPPAPI_Tell;
	file->funcs.GetLen = VFSPPAPI_GetSize;
	file->funcs.Close = VFSPPAPI_Close;
	file->funcs.Flush = VFSPPAPI_Flush;
	file->handle = f;

	if (append)
		file->offset = VFSPPAPI_GetSize((vfsfile_t*)file);
	else
		file->offset = 0;

	return (vfsfile_t*)file;
}

static vfsfile_t *FSPPAPI_OpenVFS(void *handle, flocation_t *loc, const char *mode)
{
	char diskname[MAX_OSPATH];

	//path is already cleaned, as anything that gets a valid loc needs cleaning up first.

	snprintf(diskname, sizeof(diskname), "%s/%s", (char*)handle, loc->rawname);

	return VFSPPAPI_Open(diskname, mode);
}

static void FSPPAPI_PrintPath(void *handle)
{
	Con_Printf("%s\n", (char*)handle);
}
static void FSPPAPI_ClosePath(void *handle)
{
	Z_Free(handle);
}
static int FSPPAPI_RebuildFSHash(const char *filename, int filesize, void *data)
{
	if (filename[strlen(filename)-1] == '/')
	{	//this is actually a directory

		char childpath[256];
		Q_snprintfz(childpath, sizeof(childpath), "%s*", filename);
		Sys_EnumerateFiles((char*)data, childpath, FSPPAPI_RebuildFSHash, data);
		return true;
	}
	if (!Hash_GetInsensative(&filesystemhash, filename))
	{
		bucket_t *bucket = (bucket_t*)BZ_Malloc(sizeof(bucket_t) + strlen(filename)+1);
		strcpy((char *)(bucket+1), filename);
//#ifdef _WIN32
//		Q_strlwr((char *)(bucket+1));
//#endif
		Hash_AddInsensative(&filesystemhash, (char *)(bucket+1), data, bucket);

		fs_hash_files++;
	}
	else
		fs_hash_dups++;
	return true;
}
static void FSPPAPI_BuildHash(void *handle)
{
	Sys_EnumerateFiles(handle, "*", FSPPAPI_RebuildFSHash, handle);
}
static qboolean FSPPAPI_FLocate(void *handle, flocation_t *loc, const char *filename, void *hashedresult)
{
	int len;
	char netpath[MAX_OSPATH];

	Con_Printf("Locate %s\n", filename);


	if (hashedresult && (void *)hashedresult != handle)
		return false;

/*
	if (!static_registered)
	{	// if not a registered version, don't ever go beyond base
		if ( strchr (filename, '/') || strchr (filename,'\\'))
			continue;
	}
*/

// check a file in the directory tree
	snprintf (netpath, sizeof(netpath)-1, "%s/%s",(char*)handle, filename);

	{
		vfsfile_t *f = VFSPPAPI_Open(netpath, "rb");
		if (!f)
			return false;
		len = VFS_GETLEN(f);
		VFS_CLOSE(f);
	}

	if (loc)
	{
		loc->len = len;
		loc->offset = 0;
		loc->index = 0;
		Q_strncpyz(loc->rawname, filename, sizeof(loc->rawname));
	}

	return true;
}
static void FSPPAPI_ReadFile(void *handle, flocation_t *loc, char *buffer)
{
	vfsfile_t *f;
	size_t result;

	f = VFSPPAPI_Open(loc->rawname, "rb");
	if (!f)	//err...
		return;
	VFS_SEEK(f, loc->offset);
	result = VFS_READ(f, buffer, loc->len);

	if (result != loc->len)
		Con_Printf("FSPPAPI_ReadFile() fread: Filename: %s, expected %i, result was %u\n",loc->rawname,loc->len,(unsigned int)result);

	VFS_CLOSE(f);
}
static int FSPPAPI_EnumerateFiles (void *handle, const char *match, int (*func)(const char *, int, void *), void *parm)
{
	return Sys_EnumerateFiles(handle, match, func, parm);
}

searchpathfuncs_t osfilefuncs = {
	FSPPAPI_PrintPath,
	FSPPAPI_ClosePath,
	FSPPAPI_BuildHash,
	FSPPAPI_FLocate,
	FSPPAPI_ReadFile,
	FSPPAPI_EnumerateFiles,
	NULL,
	NULL,
	FSPPAPI_OpenVFS
};
#endif