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
// models.c -- model loading and caching

// models are the only shared resource between a client and server running
// on the same machine.

#include "quakedef.h"


#if 1//ndef SERVERONLY	//FIXME
#include "glquake.h"
#include "com_mesh.h"

extern cvar_t r_shadow_bumpscale_basetexture;
extern cvar_t r_replacemodels;
extern cvar_t gl_lightmap_average;
extern cvar_t r_softwarebanding;
cvar_t mod_loadentfiles						= CVAR("sv_loadentfiles", "1");
cvar_t mod_external_vis						= CVARD("mod_external_vis", "1", "Attempt to load .vis patches for quake maps, allowing transparent water to work properly.");
cvar_t mod_warnmodels						= CVARD("mod_warnmodels", "1", "Warn if any models failed to load. Set to 0 if your mod is likely to lack optional models (like its in development).");	//set to 0 for hexen2 and its otherwise-spammy-as-heck demo.
cvar_t mod_litsprites						= CVARD("mod_litsprites", "0", "If set to 1, sprites will be lit according to world lighting (including rtlights), like Tenebrae. Use EF_ADDITIVE or EF_FULLBRIGHT to make emissive sprites instead.");
#ifdef SERVERONLY
cvar_t gl_overbright, gl_specular, gl_load24bit, r_replacemodels, gl_miptexLevel, r_fb_bmodels;	//all of these can/should default to 0
cvar_t r_noframegrouplerp					= CVARF  ("r_noframegrouplerp", "0", CVAR_ARCHIVE);
cvar_t dpcompat_psa_ungroup					= CVAR  ("dpcompat_psa_ungroup", "0");
texture_t	r_notexture_mip_real;
texture_t	*r_notexture_mip = &r_notexture_mip_real;
#endif

qboolean isnotmap = true;	//used to not warp ammo models.

void CM_Init(void);
void CM_Shutdown(void);

void Mod_LoadSpriteShaders(model_t *spr);
qboolean QDECL Mod_LoadSpriteModel (model_t *mod, void *buffer, size_t fsize);
qboolean QDECL Mod_LoadSprite2Model (model_t *mod, void *buffer, size_t fsize);
qboolean QDECL Mod_LoadBrushModel (model_t *mod, void *buffer, size_t fsize);
#ifdef Q2BSPS
qboolean QDECL Mod_LoadQ2BrushModel (model_t *mod, void *buffer, size_t fsize);
#endif
model_t *Mod_LoadModel (model_t *mod, enum mlverbosity_e verbose);
static void Mod_PrintFormats_f(void);

#ifdef MAP_DOOM
qboolean Mod_LoadDoomLevel(model_t *mod);
#endif

#ifdef DSPMODELS
void Mod_LoadDoomSprite (model_t *mod);
#endif

#define	MAX_MOD_KNOWN	8192
model_t	*mod_known;
int		mod_numknown;

extern cvar_t r_loadlits;
#ifdef SPECULAR
extern cvar_t gl_specular;
#endif
extern cvar_t r_fb_bmodels;
mesh_t nullmesh;
void Mod_SortShaders(model_t *mod);
void Mod_LoadAliasShaders(model_t *mod);

#ifdef RUNTIMELIGHTING
model_t *lightmodel;
int numlightdata;
qboolean writelitfile;

long relitsurface;
void Mod_UpdateLightmap(int snum)
{
	msurface_t *s;	
	if (lightmodel)
	{
//		int i;
//		for (s = lightmodel->surfaces,i=0; i < lightmodel->numsurfaces; i++,s++)
//			s->cached_dlight = -1;

		if (snum < lightmodel->numsurfaces)
		{
			s = lightmodel->surfaces + snum;
			s->cached_dlight = -1;
		}
		else
			Con_Printf("lit non-existant surface\n");
	}
}
#endif

#ifndef SERVERONLY
static void Mod_MemList_f(void)
{
	int m;
	model_t *mod;
	int total = 0;
	for (m=0 , mod=mod_known ; m<mod_numknown ; m++, mod++)
	{
		if (mod->memgroup.bytes)
			Con_Printf("%s: %i bytes\n", mod->name, mod->memgroup.bytes);
		total += mod->memgroup.bytes;
	}
	Con_Printf("Total: %i bytes\n", total);
}

static void Mod_BatchList_f(void)
{
	int m, i;
	model_t *mod;
	batch_t *batch;
	unsigned int count;
	for (m=0 , mod=mod_known ; m<mod_numknown ; m++, mod++)
	{
		if (mod->type == mod_brush && mod->loadstate == MLS_LOADED)
		{
			Con_Printf("^1%s:\n", mod->name);
			count = 0;
			for (i = 0; i < SHADER_SORT_COUNT; i++)
			{
				for (batch = mod->batches[i]; batch; batch = batch->next)
				{
#if MAXRLIGHTMAPS > 1
					if (batch->lightmap[3] >= 0)
						Con_Printf("  %s lm=(%i:%i %i:%i %i:%i %i:%i) surfs=%u\n", batch->texture->shader->name, batch->lightmap[0], batch->lmlightstyle[0], batch->lightmap[1], batch->lmlightstyle[1], batch->lightmap[2], batch->lmlightstyle[2], batch->lightmap[3], batch->lmlightstyle[3], batch->maxmeshes);
					else if (batch->lightmap[2] >= 0)
						Con_Printf("  %s lm=(%i:%i %i:%i %i:%i) surfs=%u\n", batch->texture->shader->name, batch->lightmap[0], batch->lmlightstyle[0], batch->lightmap[1], batch->lmlightstyle[1], batch->lightmap[2], batch->lmlightstyle[2], batch->maxmeshes);
					else if (batch->lightmap[1] >= 0)
						Con_Printf("  %s lm=(%i:%i %i:%i) surfs=%u\n", batch->texture->shader->name, batch->lightmap[0], batch->lmlightstyle[0], batch->lightmap[1], batch->lmlightstyle[1], batch->maxmeshes);
					else
#endif
						if (batch->lmlightstyle[0] != 255)
						Con_Printf("  %s lm=(%i:%i) surfs=%u\n", batch->texture->shader->name, batch->lightmap[0], batch->lmlightstyle[0], batch->maxmeshes);
					else
						Con_Printf("  %s lm=%i surfs=%u\n", batch->texture->shader->name, batch->lightmap[0], batch->maxmeshes);
					count++;
				}
			}
			Con_Printf("^h(%u batches, lm %i*%i, lux %s)\n", count, mod->lightmaps.width, mod->lightmaps.height, mod->lightmaps.deluxemapping?"true":"false");
		}
	}
}

static void Mod_TextureList_f(void)
{
	int m, i;
	texture_t *tx;
	model_t *mod;
	qboolean shownmodelname = false;
	int count = 0;
	for (m=0 , mod=mod_known ; m<mod_numknown ; m++, mod++)
	{
		if (shownmodelname)
			Con_Printf("(%u textures)\n", count);
		shownmodelname = false;

		if (mod->type == mod_brush && mod->loadstate == MLS_LOADED)
		{
			if (*mod->name == '*')
				continue;//	inlines don't count
			count = 0;
			for (i = 0; i < mod->numtextures; i++)
			{
				tx = mod->textures[i];
				if (!tx)
					continue;	//happens on e1m2

				if (!shownmodelname)
				{
					shownmodelname = true;
					Con_Printf("%s\n", mod->name);
					count = 0;
				}

				Con_Printf("  %s\n", tx->name);
				count++;
			}
		}
	}
	if (shownmodelname)
		Con_Printf("(%u textures)\n", count);
}

static void Mod_BlockTextureColour_f (void)
{
	char texname[64];
	model_t *mod;
	texture_t *tx;
//	shader_t *s;
	char *match = Cmd_Argv(1);

	int i, m;
//	unsigned int colour[8*8];
	unsigned int rgba;

	((char *)&rgba)[0] = atoi(Cmd_Argv(2));
	((char *)&rgba)[1] = atoi(Cmd_Argv(3));
	((char *)&rgba)[2] = atoi(Cmd_Argv(4));
	((char *)&rgba)[3] = 255;

	sprintf(texname, "purergb_%i_%i_%i", (int)((char *)&rgba)[0], (int)((char *)&rgba)[1], (int)((char *)&rgba)[2]);
/*	s = R_RegisterCustom(Cmd_Argv(2), SUF_LIGHTMAP, NULL, NULL);
	if (!s)
	{
		s = R_RegisterCustom (texname, SUF_LIGHTMAP, Shader_DefaultBSPQ1, NULL);

		for (i = 0; i < sizeof(colour)/sizeof(colour[0]); i++)
			colour[i] = rgba;
		s->defaulttextures.base = GL_LoadTexture32(texname, 8, 8, colour, IF_NOMIPMAP);
	}
*/
	for (m=0 , mod=mod_known ; m<mod_numknown ; m++, mod++)
	{
		if (mod->type == mod_brush && mod->loadstate == MLS_LOADED)
		{
			for (i = 0; i < mod->numtextures; i++)
			{
				tx = mod->textures[i];
				if (!tx)
					continue;	//happens on e1m2

				if (!stricmp(tx->name, match))
					tx->shader->defaulttextures.base = Image_GetTexture(texname, NULL, IF_NOMIPMAP|IF_NEAREST, &rgba, NULL, 1, 1, TF_BGRA32);
			}
		}
	}
}
#endif


#if defined(RUNTIMELIGHTING) && defined(MULTITHREAD)
void *relightthread[8];
unsigned int relightthreads;
volatile qboolean wantrelight;

int RelightThread(void *arg)
{
	int surf;
	while (wantrelight)
	{
#ifdef _WIN32
		surf = InterlockedIncrement(&relitsurface);
#elif defined(__GNUC__)
		surf = __sync_add_and_fetch(&relitsurface, 1);
#else
		surf = relitsurface++;
#endif
		if (surf >= lightmodel->numsurfaces)
			break;
		LightFace(surf);
		lightmodel->surfaces[surf].cached_dlight = -1;
	}
	return 0;
}
#endif

void Mod_Think (void)
{
#ifdef RUNTIMELIGHTING
	if (lightmodel)
	{
#ifdef MULTITHREAD
		if (!relightthreads)
		{
			int i;
#if defined(_WIN32) && !defined(WINRT)
			HANDLE me = GetCurrentProcess();
			DWORD_PTR proc, sys;
			/*count cpus*/
			GetProcessAffinityMask(me, &proc, &sys);
			relightthreads = 0;
			for (i = 0; i < sizeof(proc)*8; i++)
				if (proc & ((size_t)1u<<i))
					relightthreads++;
			/*subtract 1*/
			if (relightthreads <= 1)
				relightthreads = 1;
			else
				relightthreads--;
#elif defined(__GNUC__)
			relightthreads = 2;	//erm, lets hope...
#else
			/*can't do atomics*/
			relightthreads = 1;
#endif
			if (relightthreads > sizeof(relightthread)/sizeof(relightthread[0]))
				relightthreads = sizeof(relightthread)/sizeof(relightthread[0]);
			wantrelight = true;
			for (i = 0; i < relightthreads; i++)
				relightthread[i] = Sys_CreateThread("relight", RelightThread, lightmodel, THREADP_NORMAL, 0);
		}
		if (relitsurface < lightmodel->numsurfaces)
		{
			return;
		}
#else
		LightFace(relitsurface);
		Mod_UpdateLightmap(relitsurface);

		relitsurface++;
#endif
		if (relitsurface >= lightmodel->numsurfaces)
		{
			vfsfile_t *f;
			char filename[MAX_QPATH];
			Con_Printf("Finished lighting %s\n", lightmodel->name);

#ifdef MULTITHREAD
			if (relightthreads)
			{
				int i;
				wantrelight = false;
				for (i = 0; i < relightthreads; i++)
				{
					Sys_WaitOnThread(relightthread[i]);
					relightthread[i] = NULL;
				}
				relightthreads = 0;
			}
#endif

			if (lightmodel->deluxdata)
			{
				COM_StripExtension(lightmodel->name, filename, sizeof(filename));
				COM_DefaultExtension(filename, ".lux", sizeof(filename));
				f = FS_OpenVFS(filename, "wb", FS_GAME);
				if (f)
				{
					VFS_WRITE(f, "QLIT\1\0\0\0", 8);
					VFS_WRITE(f, lightmodel->deluxdata, numlightdata*3);
					VFS_CLOSE(f);
				}
				else
					Con_Printf("Unable to write \"%s\"\n", filename);
			}

			if (writelitfile)	//the user might already have a lit file (don't overwrite it).
			{
				COM_StripExtension(lightmodel->name, filename, sizeof(filename));
				COM_DefaultExtension(filename, ".lit", sizeof(filename));

				f = FS_OpenVFS(filename, "wb", FS_GAME);
				if (f)
				{
					VFS_WRITE(f, "QLIT\1\0\0\0", 8);
					VFS_WRITE(f, lightmodel->lightdata, numlightdata*3);
					VFS_CLOSE(f);
				}
				else
					Con_Printf("Unable to write \"%s\"\n", filename);
			}
			lightmodel = NULL;
		}
	}
#endif
}

void Mod_RebuildLightmaps (void)
{
	int i, j;
	msurface_t *surf;
	model_t	*mod;

	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		if (mod->loadstate != MLS_LOADED)
			continue;

		if (mod->type == mod_brush)
		{
			for (j=0, surf = mod->surfaces; j<mod->numsurfaces ; j++, surf++)
				surf->cached_dlight=-1;//force it
		}
	}
}

void Mod_ResortShaders(void)
{
	//called when some shader changed its sort key.
	//this means we have to hunt down all models and update their batches.
	//really its only bsps that need this.
	batch_t *oldlists[SHADER_SORT_COUNT], *b;
	int i, j, bs;
	model_t	*mod;
	for (i=0, mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		if (mod->loadstate != MLS_LOADED)
			continue;

		memcpy(oldlists, mod->batches, sizeof(oldlists));
		memset(mod->batches, 0, sizeof(oldlists));
	
		for (j = 0; j < SHADER_SORT_COUNT; j++)
		{
			while((b=oldlists[j]))
			{
				oldlists[j] = b->next;
				bs = b->shader?b->shader->sort:j;

				b->next = mod->batches[bs];
				mod->batches[bs] = b;
			}
		}
	}
}
/*
===================
Mod_ClearAll
===================

called before new content is loaded.
*/
static int mod_datasequence;
void Mod_ClearAll (void)
{
#ifdef RUNTIMELIGHTING
#ifdef MULTITHREAD
	int		i;
	wantrelight = false;
	for (i = 0; i < relightthreads; i++)
	{
		Sys_WaitOnThread(relightthread[i]);
		relightthread[i] = NULL;
	}
	relightthreads = 0;
#endif
	lightmodel = NULL;
#endif

	mod_datasequence++;
}

//can be called in one of two ways.
//force=true: explicit flush. everything goes, even if its still in use.
//force=false: map change. lots of stuff is no longer in use and can be freely flushed.
//certain models cannot be safely flushed while still in use. such models will not be flushed even if forced (they may still be partially flushed).
void Mod_Purge(enum mod_purge_e ptype)
{
	int		i;
	model_t	*mod;
	qboolean unused;

	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
	{
		unused = mod->datasequence != mod_datasequence;

		//this model isn't active any more.
		if (unused || ptype != MP_MAPCHANGED)
		{
			if (mod->loadstate == MLS_LOADING)
				COM_WorkerPartialSync(mod, &mod->loadstate, MLS_LOADING);

			if (unused)
				Con_DPrintf("model \"%s\" no longer needed\n", mod->name);

#ifdef TERRAIN
			//we can safely flush all terrain sections at any time
			if (mod->terrain && ptype != MP_MAPCHANGED)
				Terr_PurgeTerrainModel(mod, false, true);
#endif

			//purge any vbos
			if (mod->type == mod_brush)
			{
				//brush models cannot be safely flushed.
				if (!unused && ptype != MP_RESET)
					continue;
#ifndef SERVERONLY
				Surf_Clear(mod);
#endif
			}

#ifdef TERRAIN
			if (mod->type == mod_brush || mod->type == mod_heightmap)
			{
				//heightmap/terrain models cannot be safely flushed (brush models might have terrain embedded).
				if (!unused && ptype != MP_RESET)
					continue;
				Terr_FreeModel(mod);
			}
#endif
			if (mod->type == mod_alias)
			{
				Mod_DestroyMesh(mod->meshinfo);
				mod->meshinfo = NULL;
			}

			//and obliterate anything else remaining in memory.
			ZG_FreeGroup(&mod->memgroup);
			mod->meshinfo = NULL;
			mod->loadstate = MLS_NOTLOADED;
		}
	}
}

/*
===============
Mod_Init
===============
*/
void Mod_Init (qboolean initial)
{
	mod_known = malloc(MAX_MOD_KNOWN * sizeof(*mod_known));
	if (!initial)
	{
		Mod_ClearAll();	//shouldn't be needed
		Mod_Purge(MP_RESET);//shouldn't be needed
		mod_numknown = 0;
		Q1BSP_Init();

#ifndef SERVERONLY
		Cmd_AddCommand("mod_memlist", Mod_MemList_f);
		Cmd_AddCommand("mod_batchlist", Mod_BatchList_f);
		Cmd_AddCommand("mod_texturelist", Mod_TextureList_f);
		Cmd_AddCommand("mod_usetexture", Mod_BlockTextureColour_f);
#endif
	}
	else
	{
		Cvar_Register(&mod_external_vis, "Graphical Nicaties");
		Cvar_Register(&mod_warnmodels, "Graphical Nicaties");
		Cvar_Register(&mod_litsprites, "Graphical Nicaties");
		Cvar_Register(&mod_loadentfiles, NULL);
		Cmd_AddCommand("version_modelformats", Mod_PrintFormats_f);
	}

	if (initial)
	{
		Alias_Register();

#ifdef SPRMODELS
		Mod_RegisterModelFormatMagic(NULL, "Quake1 Sprite (spr)",			IDSPRITEHEADER,							Mod_LoadSpriteModel);
#endif
#ifdef SP2MODELS
		Mod_RegisterModelFormatMagic(NULL, "Quake2 Sprite (sp2)",			IDSPRITE2HEADER,						Mod_LoadSprite2Model);
#endif

		//q2/q3bsps
#ifdef Q2BSPS
		Mod_RegisterModelFormatMagic(NULL, "Quake2/Quake2 Map (bsp)",		IDBSPHEADER,							Mod_LoadQ2BrushModel);
#endif
#ifdef Q3BSPS
		Mod_RegisterModelFormatMagic(NULL, "Raven Map (bsp)",				('R'<<0)+('B'<<8)+('S'<<16)+('P'<<24),	Mod_LoadQ2BrushModel);
		Mod_RegisterModelFormatMagic(NULL, "QFusion Map (bsp)",				('F'<<0)+('B'<<8)+('S'<<16)+('P'<<24),	Mod_LoadQ2BrushModel);
#endif

		//doom maps
#ifdef MAP_DOOM
		Mod_RegisterModelFormatMagic(NULL, "Doom IWad Map",					(('D'<<24)+('A'<<16)+('W'<<8)+'I'),		Mod_LoadDoomLevel);
		Mod_RegisterModelFormatMagic(NULL, "Doom PWad Map",					(('D'<<24)+('A'<<16)+('W'<<8)+'P'),		Mod_LoadDoomLevel);
#endif

#ifdef MAP_PROC
		Mod_RegisterModelFormatText(NULL, "Doom3 (cm)",						"CM",									D3_LoadMap_CollisionMap);
#endif

		//q1-based formats
		Mod_RegisterModelFormatMagic(NULL, "Quake1 2PSB Map(bsp)",			BSPVERSION_LONG1,						Mod_LoadBrushModel);
		Mod_RegisterModelFormatMagic(NULL, "Quake1 BSP2 Map(bsp)",			BSPVERSION_LONG2,						Mod_LoadBrushModel);
		Mod_RegisterModelFormatMagic(NULL, "Half-Life Map (bsp)",			30,										Mod_LoadBrushModel);
		Mod_RegisterModelFormatMagic(NULL, "Quake1 Map (bsp)",				29,										Mod_LoadBrushModel);
		Mod_RegisterModelFormatMagic(NULL, "Quake1 Prerelease Map (bsp)",	28,										Mod_LoadBrushModel);
	}
}

void Mod_Shutdown (qboolean final)
{
	if (final)
	{
		Mod_UnRegisterAllModelFormats(NULL);
#ifdef Q2BSPS
		CM_Shutdown();
#endif
	}
	else
	{
		Mod_ClearAll();
		Mod_Purge(MP_RESET);

		Cmd_RemoveCommand("mod_memlist");
		Cmd_RemoveCommand("mod_batchlist");
		Cmd_RemoveCommand("mod_texturelist");
		Cmd_RemoveCommand("mod_usetexture");
	}
	free(mod_known);
	mod_known = NULL;
	mod_numknown = 0;

#ifndef SERVERONLY
	r_worldentity.model = NULL;	//just in case.
	cl_numvisedicts = 0;	//make sure nothing gets cached.
#endif
}

/*
===============
Mod_Init

Caches the data if needed
===============
*/
void *Mod_Extradata (model_t *mod)
{
	void	*r;
	
	r = mod->meshinfo;
	if (r)
		return r;

	Mod_LoadModel (mod, MLV_ERROR);
	
	if (!mod->meshinfo)
		Sys_Error ("Mod_Extradata: caching failed");
	return mod->meshinfo;
}

/*
===============
Mod_PointInLeaf
===============
*/
mleaf_t *Mod_PointInLeaf (model_t *model, vec3_t p)
{
	mnode_t		*node;
	float		d;
	mplane_t	*plane;
	
	if (!model)
	{		
		Sys_Error ("Mod_PointInLeaf: bad model");
	}
	if (!model->nodes)
		return NULL;
#ifdef Q2BSPS
	if (model->fromgame == fg_quake2 || model->fromgame == fg_quake3)
	{
		return model->leafs + CM_PointLeafnum(model, p);
	}
#endif
	if (model->fromgame == fg_doom)
	{
		return NULL;
	}

	node = model->nodes;
	while (1)
	{
		if (node->contents < 0)
			return (mleaf_t *)node;
		plane = node->plane;
		d = DotProduct (p,plane->normal) - plane->dist;
		if (d > 0)
			node = node->children[0];
		else
			node = node->children[1];
	}
	
	return NULL;	// never reached
}

const char *Mod_FixName(const char *modname, const char *worldname)
{
	if (*modname == '*' && worldname && *worldname)
	{
		//make sure that the value is an inline value with no existing extra postfix or anything.
		char *e;
		if (strtoul(modname+1, &e, 10) != 0)
			if (!*e)
				return va("%s:%s", modname, worldname);
	}
	return modname;
}
/*
==================
Mod_FindName

==================
*/
model_t *Mod_FindName (const char *name)
{
	int		i;
	model_t	*mod;
	
//	if (!name[0])
//		Sys_Error ("Mod_ForName: NULL name");
		
//
// search the currently loaded models
//
	for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
		if (!strcmp (mod->name, name) )
			break;
			
	if (i == mod_numknown)
	{
#ifdef LOADERTHREAD
		Sys_LockMutex(com_resourcemutex);
		for (i=0 , mod=mod_known ; i<mod_numknown ; i++, mod++)
			if (!strcmp (mod->name, name) )
				break;
		if (i == mod_numknown)
		{
#endif
			if (mod_numknown == MAX_MOD_KNOWN)
				Sys_Error ("mod_numknown == MAX_MOD_KNOWN");
			if (strlen(name) >= sizeof(mod->name))
				Sys_Error ("model name is too long: %s", name);
			memset(mod, 0, sizeof(model_t));	//clear the old model as the renderers use the same globals
			Q_strncpyz (mod->name, name, sizeof(mod->name));
			mod->loadstate = MLS_NOTLOADED;
			mod_numknown++;
			mod->particleeffect = -1;
			mod->particletrail = -1;
#ifdef LOADERTHREAD
		}
		Sys_UnlockMutex(com_resourcemutex);
#endif
	}

//	if (mod->loadstate == MLS_FAILED)
//		mod->loadstate = MLS_NOTLOADED;

	//mark it as active, so it doesn't get flushed prematurely
	mod->datasequence = mod_datasequence;
	return mod;
}

/*
==================
Mod_TouchModel

==================
*/
void Mod_TouchModel (const char *name)
{
	//findname does this anyway.
	Mod_FindName (name);
}

static struct
{
	void *module;
	char *formatname;
	char *ident;
	unsigned int magic;
	qboolean (QDECL *load) (model_t *mod, void *buffer, size_t buffersize);
} modelloaders[64];

static void Mod_PrintFormats_f(void)
{
	int i;
	for (i = 0; i < sizeof(modelloaders)/sizeof(modelloaders[0]); i++)
	{
		if (modelloaders[i].load && modelloaders[i].formatname)
			Con_Printf("%s\n", modelloaders[i].formatname);
	}
}

int Mod_RegisterModelFormatText(void *module, const char *formatname, char *magictext, qboolean (QDECL *load) (model_t *mod, void *buffer, size_t fsize))
{
	int i, free = -1;
	for (i = 0; i < sizeof(modelloaders)/sizeof(modelloaders[0]); i++)
	{
		if (modelloaders[i].ident && !strcmp(modelloaders[i].ident, magictext))
		{
			free = i;
			break;	//extension match always replaces
		}
		else if (!modelloaders[i].load && free < 0)
			free = i;
	}
	if (free < 0)
		return 0;

	modelloaders[free].module = module;
	modelloaders[free].formatname = Z_StrDup(formatname);
	modelloaders[free].magic = 0;
	modelloaders[free].ident = Z_StrDup(magictext);
	modelloaders[free].load = load;

	return free+1;
}
int Mod_RegisterModelFormatMagic(void *module, const char *formatname, unsigned int magic, qboolean (QDECL *load) (model_t *mod, void *buffer, size_t fsize))
{
	int i, free = -1;
	for (i = 0; i < sizeof(modelloaders)/sizeof(modelloaders[0]); i++)
	{
		if (modelloaders[i].magic && modelloaders[i].magic == magic)
		{
			free = i;
			break;	//extension match always replaces
		}
		else if (!modelloaders[i].load && free < 0)
			free = i;
	}
	if (free < 0)
		return 0;

	modelloaders[free].module = module;
	if (modelloaders[free].formatname)
		Z_Free(modelloaders[free].formatname);
	modelloaders[free].formatname = Z_StrDup(formatname);
	modelloaders[free].magic = magic;
	modelloaders[free].ident = NULL;
	modelloaders[free].load = load;

	return free+1;
}

void Mod_UnRegisterModelFormat(void *module, int idx)
{
	
	idx--;
	if ((unsigned int)(idx) >= sizeof(modelloaders)/sizeof(modelloaders[0]))
		return;
	if (modelloaders[idx].module != module)
		return;

	COM_WorkerFullSync();
	Z_Free(modelloaders[idx].ident);
	modelloaders[idx].ident = NULL;
	Z_Free(modelloaders[idx].formatname);
	modelloaders[idx].formatname = NULL;
	modelloaders[idx].magic = 0;
	modelloaders[idx].load = NULL;
	modelloaders[idx].module = NULL;

	//FS_Restart will be needed
}

void Mod_UnRegisterAllModelFormats(void *module)
{
	int i;
	COM_WorkerFullSync();
	for (i = 0; i < sizeof(modelloaders)/sizeof(modelloaders[0]); i++)
	{
		if (modelloaders[i].module == module)
			Mod_UnRegisterModelFormat(module, i+1);
	}
}

void Mod_ModelLoaded(void *ctx, void *data, size_t a, size_t b)
{
	qboolean previouslyfailed;
	model_t *mod = ctx;
	enum mlverbosity_e verbose = b;
#ifndef SERVERONLY
	P_LoadedModel(mod);
#endif

	previouslyfailed = mod->loadstate == MLS_FAILED;
	mod->loadstate = a;

#ifdef TERRAIN
	if (mod->terrain)
		Terr_FinishTerrain(mod);
#endif
#ifndef SERVERONLY
	if (mod->type == mod_brush)
	{
		Surf_BuildModelLightmaps(mod);
	}
	if (mod->type == mod_sprite)
	{
		Mod_LoadSpriteShaders(mod);
	}
	if (mod->type == mod_alias)
	{
		if (qrenderer != QR_NONE)
			Mod_LoadAliasShaders(mod);


#ifdef RAGDOLL
		{
			int numbones = Mod_GetNumBones(mod, false);
			if (numbones)
			{
				size_t filesize;
				char *buf;
				char dollname[MAX_QPATH];
				Q_snprintfz(dollname, sizeof(dollname), "%s.doll", mod->name);
				buf = COM_LoadFile(dollname, 5, &filesize);
				if (buf)
				{
					mod->dollinfo = rag_createdollfromstring(mod, dollname, numbones, buf);
					BZ_Free(buf);
				}
			}
		}
#endif
	}
#endif

	switch(verbose)
	{
	default:
	case MLV_ERROR:
		Host_EndGame ("Mod_NumForName: %s not found or couldn't load", mod->name);
		break;
	case MLV_WARN:
		if (*mod->name != '*' && strcmp(mod->name, "null") && mod_warnmodels.ival && !previouslyfailed)
			Con_Printf(CON_ERROR "Unable to load %s\n", mod->name);
		break;
	case MLV_SILENT:
		break;
	}
}
/*
==================
Mod_LoadModel

Loads a model into the cache
==================
*/
void Mod_LoadModelWorker (void *ctx, void *data, size_t a, size_t b)
{
	model_t *mod = ctx;
	enum mlverbosity_e verbose = a;
	unsigned *buf = NULL;
	char mdlbase[MAX_QPATH];
	char *replstr;
	qboolean doomsprite = false;
	unsigned int magic, i;
	size_t filesize;
	char ext[8];

	if (!*mod->name)
	{
		mod->type = mod_dummy;
		mod->mins[0] = -16;
		mod->mins[1] = -16;
		mod->mins[2] = -16;
		mod->maxs[0] = 16;
		mod->maxs[1] = 16;
		mod->maxs[2] = 16;
		mod->engineflags = 0;
		COM_AddWork(0, Mod_ModelLoaded, mod, NULL, MLS_LOADED, 0);
		return;
	}
	
#ifdef RAGDOLL
	if (mod->dollinfo)
	{
		rag_freedoll(mod->dollinfo);
		mod->dollinfo = NULL;
	}
#endif

	if (mod->loadstate == MLS_FAILED)
		return;

//
// load the file
//
	// set necessary engine flags for loading purposes
	if (!strcmp(mod->name, "progs/player.mdl"))
		mod->engineflags |= MDLF_PLAYER | MDLF_DOCRC;
	else if (!strcmp(mod->name, "progs/flame.mdl") || 
		!strcmp(mod->name, "progs/flame2.mdl") ||
		!strcmp(mod->name, "models/flame1.mdl") ||	//hexen2 small standing flame
		!strcmp(mod->name, "models/flame2.mdl") ||	//hexen2 large standing flame
		!strcmp(mod->name, "models/cflmtrch.mdl"))	//hexen2 wall torch
		mod->engineflags |= MDLF_FLAME;
	else if (!strcmp(mod->name, "progs/bolt.mdl") ||
		!strcmp(mod->name, "progs/bolt2.mdl") ||
		!strcmp(mod->name, "progs/bolt3.mdl") ||
		!strcmp(mod->name, "progs/beam.mdl") || 
		!strcmp(mod->name, "models/stsunsf2.mdl") || 
		!strcmp(mod->name, "models/stsunsf1.mdl") ||
		!strcmp(mod->name, "models/stice.mdl"))
		mod->engineflags |= MDLF_BOLT;
	else if (!strcmp(mod->name, "progs/backpack.mdl"))
		mod->engineflags |= MDLF_NOTREPLACEMENTS;
	else if (!strcmp(mod->name, "progs/eyes.mdl"))
		mod->engineflags |= MDLF_NOTREPLACEMENTS|MDLF_DOCRC;

	/*handle ezquake-originated cheats that would feck over fte users if fte didn't support
	these are the conditions required for r_fb_models on non-players*/
	mod->engineflags |= MDLF_EZQUAKEFBCHEAT;
	if ((mod->engineflags & MDLF_DOCRC) ||
		!strcmp(mod->name, "progs/backpack.mdl") ||
		!strcmp(mod->name, "progs/gib1.mdl") ||
		!strcmp(mod->name, "progs/gib2.mdl") ||
		!strcmp(mod->name, "progs/gib3.mdl") ||
		!strcmp(mod->name, "progs/h_player.mdl") ||
		!strncmp(mod->name, "progs/v_", 8))
		mod->engineflags &= ~MDLF_EZQUAKEFBCHEAT;

	// get string used for replacement tokens
	COM_FileExtension(mod->name, ext, sizeof(ext));
	if (!Q_strcasecmp(ext, "spr") || !Q_strcasecmp(ext, "sp2"))
		replstr = ""; // sprite
	else if (!Q_strcasecmp(ext, "dsp")) // doom sprite
	{
		replstr = "";
		doomsprite = true;
	}
	else // assume models
		replstr = r_replacemodels.string;

	// gl_load24bit 0 disables all replacements
	if (!gl_load24bit.value)
		replstr = "";

	COM_StripExtension(mod->name, mdlbase, sizeof(mdlbase));

	while (replstr)
	{
		char token[256];
		replstr = COM_ParseStringSet(replstr, token, sizeof(token));

		if (replstr)
		{
			char altname[MAX_QPATH];
			Q_snprintfz(altname, sizeof(altname), "%s.%s", mdlbase, token);
			TRACE(("Mod_LoadModel: Trying to load (replacement) model \"%s\"\n", altname));
			buf = (unsigned *)COM_LoadFile (altname, 5, &filesize);
		}
		else
		{
			TRACE(("Mod_LoadModel: Trying to load model \"%s\"\n", mod->name));
			buf = (unsigned *)COM_LoadFile (mod->name, 5, &filesize);
			if (!buf)
			{
#ifdef DSPMODELS
				if (doomsprite) // special case needed for doom sprites
				{
					TRACE(("Mod_LoadModel: doomsprite: \"%s\"\n", mod->name));
					Mod_LoadDoomSprite(mod);
					BZ_Free(buf);
					COM_AddWork(0, Mod_ModelLoaded, mod, NULL, MLS_LOADED, 0);
					return;
				}
#endif
				break; // failed to load unreplaced file and nothing left
			}
		}
		if (!buf)
			continue;
	
//
// fill it in
//
		Mod_DoCRC(mod, (char*)buf, filesize);

		magic = LittleLong(*(unsigned *)buf);
		for(i = 0; i < sizeof(modelloaders) / sizeof(modelloaders[0]); i++)
		{
			if (modelloaders[i].load && modelloaders[i].magic == magic && !modelloaders[i].ident)
				break;
		}
		if (i < sizeof(modelloaders) / sizeof(modelloaders[0]))
		{
			if (!modelloaders[i].load(mod, buf, filesize))
			{
				BZ_Free(buf);
				continue;
			}
		}
		else
		{
			COM_ParseOut((char*)buf, token, sizeof(token));
			for(i = 0; i < sizeof(modelloaders) / sizeof(modelloaders[0]); i++)
			{
				if (modelloaders[i].load && modelloaders[i].ident && !strcmp(modelloaders[i].ident, token))
					break;
			}
			if (i < sizeof(modelloaders) / sizeof(modelloaders[0]))
			{
				if (!modelloaders[i].load(mod, buf, filesize))
				{
					BZ_Free(buf);
					continue;
				}
			}
			else
			{
				Con_Printf(CON_WARNING "Unrecognised model format 0x%x (%c%c%c%c)\n", LittleLong(*(unsigned *)buf), ((char*)buf)[0], ((char*)buf)[1], ((char*)buf)[2], ((char*)buf)[3]);
				BZ_Free(buf);
				continue;
			}
		}

/*
#ifdef MAP_PROC
			if (!strcmp(com_token, "CM"))	//doom3 map.
			{
				TRACE(("Mod_LoadModel: doom3 CM\n"));
				if (!D3_LoadMap_CollisionMap (mod, (char*)buf))
					continue;
				break;
			}
#endif
*/

		TRACE(("Mod_LoadModel: Loaded\n"));

		BZ_Free(buf);

		COM_AddWork(0, Mod_ModelLoaded, mod, NULL, MLS_LOADED, 0);
		return;
	}

	mod->type = mod_dummy;
	mod->mins[0] = -16;
	mod->mins[1] = -16;
	mod->mins[2] = -16;
	mod->maxs[0] = 16;
	mod->maxs[1] = 16;
	mod->maxs[2] = 16;
	mod->engineflags = 0;
	COM_AddWork(0, Mod_ModelLoaded, mod, NULL, MLS_FAILED, verbose);
}


model_t *Mod_LoadModel (model_t *mod, enum mlverbosity_e verbose)
{
	if (mod->loadstate == MLS_NOTLOADED && *mod->name != '*')
	{
		mod->loadstate = MLS_LOADING;
//		if (verbose == MLV_ERROR)	//if its fatal on failure (ie: world), do it on the main thread and block to wait for it.
//			Mod_LoadModelWorker(mod, MLV_WARN, 0);
//		else
			COM_AddWork(1, Mod_LoadModelWorker, mod, NULL, verbose, 0);
	}

	if (verbose == MLV_ERROR)
	{
		//someone already tried to load it without caring if it failed or not. make sure its loaded.
		//fixme: this is a spinloop.
		if (mod->loadstate == MLS_LOADING)
			COM_WorkerPartialSync(mod, &mod->loadstate, MLS_LOADING);

		if (mod->loadstate != MLS_LOADED)
			Host_EndGame ("Mod_NumForName: %s not found or couldn't load", mod->name);
	}
	return mod;
}

/*
==================
Mod_ForName

Loads in a model for the given name
==================
*/
model_t *Mod_ForName (const char *name, enum mlverbosity_e verbosity)
{
	model_t	*mod;
	
	mod = Mod_FindName (name);
	
	return Mod_LoadModel (mod, verbosity);
}


/*
===============================================================================

					BRUSHMODEL LOADING

===============================================================================
*/

void Mod_FinishTexture(texture_t *tx, const char *loadname)
{
#ifndef SERVERONLY
	extern cvar_t gl_shadeq1_name;
	char altname[MAX_QPATH];
	char *star;
	/*skies? just replace with the override sky*/
	if (!strncmp(tx->name, "sky", 3) && *cl.skyname)
		tx->shader = R_RegisterCustom (va("skybox_%s", cl.skyname), SUF_NONE, Shader_DefaultSkybox, NULL);	//just load the regular name.
	//find the *
	else if (!*gl_shadeq1_name.string || !strcmp(gl_shadeq1_name.string, "*"))
		tx->shader = R_RegisterCustom (tx->name, SUF_LIGHTMAP, Shader_DefaultBSPQ1, NULL);	//just load the regular name.
	else if (!(star = strchr(gl_shadeq1_name.string, '*')) || (strlen(gl_shadeq1_name.string)+strlen(tx->name)+1>=sizeof(altname)))	//it's got to fit.
		tx->shader = R_RegisterCustom (gl_shadeq1_name.string, SUF_LIGHTMAP, Shader_DefaultBSPQ1, NULL);
	else
	{
		strncpy(altname, gl_shadeq1_name.string, star-gl_shadeq1_name.string);	//copy the left
		altname[star-gl_shadeq1_name.string] = '\0';
		strcat(altname, tx->name);	//insert the *
		strcat(altname, star+1);	//add any final text.
		tx->shader = R_RegisterCustom (altname, SUF_LIGHTMAP, Shader_DefaultBSPQ1, NULL);
	}

	if (!strncmp(tx->name, "sky", 3))
		R_InitSky (&tx->shader->defaulttextures, tx, tx->mips[0]);
	else
	{
		unsigned int maps = 0;
		maps |= SHADER_HASPALETTED;
		maps |= SHADER_HASDIFFUSE;
		if (r_fb_bmodels.ival)
			maps |= SHADER_HASFULLBRIGHT;
		if (r_loadbumpmapping || (r_waterstyle.ival > 1 && *tx->name == '*'))
			maps |= SHADER_HASNORMALMAP;
		if (gl_specular.ival)
			maps |= SHADER_HASGLOSS;
		R_BuildLegacyTexnums(tx->shader, loadname, maps, ((*tx->name=='{')?TF_TRANS8:TF_SOLID8), tx->width, tx->height, tx->mips, host_basepal);
	}
	BZ_Free(tx->mips[0]);
#endif
}
static void Mod_LoadMiptex(model_t *loadmodel, texture_t *tx, miptex_t *mt)
{
#ifndef SERVERONLY
	qbyte *base;
	qboolean alphaed;
	int pixels = mt->width*mt->height/64*85;


	base = (qbyte *)(mt+1);

	if (loadmodel->fromgame == fg_halflife)
	{//external textures have already been filtered.

		//size is not directly known.
		//we might be able to infer based upon neighbours, but that seems like too much hassle
		base = W_ConvertWAD3Texture(mt, 0xffffffff, &mt->width, &mt->height, &alphaed);	//convert texture to 32 bit.
//		tx->texnums.base = R_LoadReplacementTexture(mt->name, loadname, alphaed?0:IF_NOALPHA, base, tx->width, tx->height, alphaed?TF_RGBA32:TF_RGBX32);
		BZ_Free(base);
	}
	else
	{
		qbyte *mipbase;
		unsigned int mipwidth, mipheight;
		extern cvar_t gl_miptexLevel;
		if ((unsigned int)gl_miptexLevel.ival < 4 && mt->offsets[gl_miptexLevel.ival])
		{
			mipbase = (qbyte*)mt + mt->offsets[gl_miptexLevel.ival];
			mipwidth = tx->width>>gl_miptexLevel.ival;
			mipheight = tx->height>>gl_miptexLevel.ival;
		}
		else
		{
			mipbase = base;
			mipwidth = tx->width;
			mipheight = tx->height;
		}

		tx->mips[0] = BZ_Malloc(mipwidth*mipheight);
		memcpy(tx->mips[0], mipbase, mipwidth*mipheight);
	}
#endif
}

/*
=================
Mod_LoadTextures
=================
*/
qboolean Mod_LoadTextures (model_t *loadmodel, qbyte *mod_base, lump_t *l)
{
	int		i, j, num, max, altmax;
	miptex_t	*mt;
	texture_t	*tx, *tx2;
	texture_t	*anims[10];
	texture_t	*altanims[10];
	dmiptexlump_t *m;

TRACE(("dbg: Mod_LoadTextures: inittexturedescs\n"));

//	Mod_InitTextureDescs(loadname);

	if (!l->filelen)
	{
		Con_Printf(CON_WARNING "warning: %s contains no texture data\n", loadmodel->name);

		loadmodel->numtextures = 1;
		loadmodel->textures = ZG_Malloc(&loadmodel->memgroup, 1 * sizeof(*loadmodel->textures));

		i = 0;
		tx = ZG_Malloc(&loadmodel->memgroup, sizeof(texture_t));
		memcpy(tx, r_notexture_mip, sizeof(texture_t));
		sprintf(tx->name, "unnamed%i", i);
		loadmodel->textures[i] = tx;

		return true;
	}
	m = (dmiptexlump_t *)(mod_base + l->fileofs);

	m->nummiptex = LittleLong (m->nummiptex);

	loadmodel->numtextures = m->nummiptex;
	loadmodel->textures = ZG_Malloc(&loadmodel->memgroup, m->nummiptex * sizeof(*loadmodel->textures));

	for (i=0 ; i<m->nummiptex ; i++)
	{
		m->dataofs[i] = LittleLong(m->dataofs[i]);
		if (m->dataofs[i] == -1)	//e1m2, this happens
		{
			tx = ZG_Malloc(&loadmodel->memgroup, sizeof(texture_t));
			memcpy(tx, r_notexture_mip, sizeof(texture_t));
			sprintf(tx->name, "unnamed%i", i);
			loadmodel->textures[i] = tx;
			continue;
		}
		mt = (miptex_t *)((qbyte *)m + m->dataofs[i]);

	TRACE(("dbg: Mod_LoadTextures: texture %s\n", loadname));

		if (!*mt->name)	//I HATE MAPPERS!
		{
			sprintf(mt->name, "unnamed%i", i);
			Con_Printf(CON_WARNING "warning: unnamed texture in %s, renaming to %s\n", loadmodel->name, mt->name);
		}

		mt->width = LittleLong (mt->width);
		mt->height = LittleLong (mt->height);
		for (j=0 ; j<MIPLEVELS ; j++)
			mt->offsets[j] = LittleLong (mt->offsets[j]);

		if ( (mt->width & 15) || (mt->height & 15) )
			Con_Printf (CON_WARNING "Warning: Texture %s is not 16 aligned", mt->name);
		if (mt->width < 1 || mt->height < 1)
			Con_Printf (CON_WARNING "Warning: Texture %s has no size", mt->name);
		tx = ZG_Malloc(&loadmodel->memgroup, sizeof(texture_t));
		loadmodel->textures[i] = tx;

		memcpy (tx->name, mt->name, sizeof(tx->name));
		tx->width = mt->width;
		tx->height = mt->height;

		if (!mt->offsets[0])	//this is a hl external style texture, load it a little later (from a wad)
		{
			continue;
		}

		Mod_LoadMiptex(loadmodel, tx, mt);
	}
//
// sequence the animations
//
	for (i=0 ; i<m->nummiptex ; i++)
	{
		tx = loadmodel->textures[i];
		if (!tx || tx->name[0] != '+')
			continue;
		if (tx->anim_next)
			continue;	// already sequenced

	// find the number of frames in the animation
		memset (anims, 0, sizeof(anims));
		memset (altanims, 0, sizeof(altanims));

		max = tx->name[1];
		altmax = 0;
		if (max >= 'a' && max <= 'z')
			max -= 'a' - 'A';
		if (max >= '0' && max <= '9')
		{
			max -= '0';
			altmax = 0;
			anims[max] = tx;
			max++;
		}
		else if (max >= 'A' && max <= 'J')
		{
			altmax = max - 'A';
			max = 0;
			altanims[altmax] = tx;
			altmax++;
		}
		else
		{
			Con_Printf (CON_ERROR "Bad animating texture %s\n", tx->name);
			return false;
		}

		for (j=i+1 ; j<m->nummiptex ; j++)
		{
			tx2 = loadmodel->textures[j];
			if (!tx2 || tx2->name[0] != '+')
				continue;
			if (strcmp (tx2->name+2, tx->name+2))
				continue;

			num = tx2->name[1];
			if (num >= 'a' && num <= 'z')
				num -= 'a' - 'A';
			if (num >= '0' && num <= '9')
			{
				num -= '0';
				anims[num] = tx2;
				if (num+1 > max)
					max = num + 1;
			}
			else if (num >= 'A' && num <= 'J')
			{
				num = num - 'A';
				altanims[num] = tx2;
				if (num+1 > altmax)
					altmax = num+1;
			}
			else
			{
				Con_Printf (CON_ERROR "Bad animating texture %s\n", tx->name);
				return false;
			}
		}

#define	ANIM_CYCLE	2
	// link them all together
		for (j=0 ; j<max ; j++)
		{
			tx2 = anims[j];
			if (!tx2)
			{
				Con_Printf (CON_ERROR "Missing frame %i of %s\n",j, tx->name);
				return false;
			}
			tx2->anim_total = max * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j+1) * ANIM_CYCLE;
			tx2->anim_next = anims[ (j+1)%max ];
			if (altmax)
				tx2->alternate_anims = altanims[0];
		}
		for (j=0 ; j<altmax ; j++)
		{
			tx2 = altanims[j];
			if (!tx2)
			{
				Con_Printf (CON_ERROR "Missing frame %i of %s\n",j, tx->name);
				return false;
			}
			tx2->anim_total = altmax * ANIM_CYCLE;
			tx2->anim_min = j * ANIM_CYCLE;
			tx2->anim_max = (j+1) * ANIM_CYCLE;
			tx2->anim_next = altanims[ (j+1)%altmax ];
			if (max)
				tx2->alternate_anims = anims[0];
		}
	}

	return true;
}

void Mod_NowLoadExternal(model_t *loadmodel)
{
	//for halflife bsps where wads are loaded after the map.
#ifndef SERVERONLY
	int i;
	texture_t	*tx;
	char loadname[32];
	COM_FileBase (cl.worldmodel->name, loadname, sizeof(loadname));
	
	if (!strncmp(loadname, "b_", 2))
		Q_strncpyz(loadname, "bmodels", sizeof(loadname));

	for (i=0 ; i<loadmodel->numtextures ; i++)
	{
		tx = loadmodel->textures[i];
		if (!tx)	//e1m2, this happens
			continue;

		if (tx->shader)
			continue;

		Mod_FinishTexture(tx, loadname);
	}
#endif
}

qbyte lmgamma[256];
void BuildLightMapGammaTable (float g, float c)
{
	int i, inf;

//	g = bound (0.1, g, 3);
//	c = bound (1, c, 3);

	if (g == 1 && c == 1)
	{
		for (i = 0; i < 256; i++)
			lmgamma[i] = i;
		return;
	}

	for (i = 0; i < 256; i++)
	{
		inf = 255 * pow ((i + 0.5) / 255.5 * c, g) + 0.5;
		if (inf < 0)
			inf = 0;
		else if (inf > 255)
			inf = 255;		
		lmgamma[i] = inf;
	}
}

/*
=================
Mod_LoadLighting
=================
*/
void Mod_LoadLighting (model_t *loadmodel, qbyte *mod_base, lump_t *l, qboolean interleaveddeluxe)
{
	qboolean luxtmp = true;
	qboolean littmp = true;
	qbyte *luxdata = NULL;
	qbyte *litdata = NULL;
	qbyte *lumdata = NULL;
	qbyte *out;
	unsigned int samples;

	extern cvar_t gl_overbright;
	loadmodel->engineflags &= ~MDLF_RGBLIGHTING;

	//q3 maps have built in 4-fold overbright.
	//if we're not rendering with that, we need to brighten the lightmaps in order to keep the darker parts the same brightness. we loose the 2 upper bits. those bright areas become uniform and indistinct.
	if (loadmodel->fromgame == fg_quake3)
	{
		gl_overbright.flags |= CVAR_LATCH;
		BuildLightMapGammaTable(1, (1<<(2-gl_overbright.ival)));
	}
	else
	//lit file light intensity is made to match the world's light intensity.
//	if (cls.allow_lightmapgamma)
//		BuildLightMapGammaTable(0.6, 2);
//	else
		BuildLightMapGammaTable(1, 1);

	loadmodel->lightdata = NULL;
	loadmodel->deluxdata = NULL;
	if (loadmodel->fromgame == fg_halflife || loadmodel->fromgame == fg_quake2 || loadmodel->fromgame == fg_quake3)
	{
		litdata = mod_base + l->fileofs;
		samples = l->filelen/3;
	}
	else
	{
		lumdata = mod_base + l->fileofs;
		samples = l->filelen;
	}
	if (interleaveddeluxe)
		samples >>= 1;
	if (!samples)
		return;

#ifndef SERVERONLY
	if (!luxdata && r_loadlits.ival && r_deluxemapping.ival)
	{	//the map util has a '-scalecos X' parameter. use 0 if you're going to use only just lux. without lux scalecos 0 is hideous.
		char luxname[MAX_QPATH];
		size_t luxsz = 0;
		if (!luxdata)
		{							
			Q_strncpyz(luxname, loadmodel->name, sizeof(luxname));
			COM_StripExtension(loadmodel->name, luxname, sizeof(luxname));
			COM_DefaultExtension(luxname, ".lux", sizeof(luxname));
			luxdata = FS_LoadMallocGroupFile(&loadmodel->memgroup, luxname, &luxsz);
			luxtmp = false;
		}
		if (!luxdata)
		{
			Q_strncpyz(luxname, "luxs/", sizeof(luxname));
			COM_StripExtension(COM_SkipPath(loadmodel->name), luxname+5, sizeof(luxname)-5);
			Q_strncatz(luxname, ".lux", sizeof(luxname));

			luxdata = FS_LoadMallocGroupFile(&loadmodel->memgroup, luxname, &luxsz);
			luxtmp = false;
		}
		if (!luxdata) //dp...
		{
			COM_StripExtension(loadmodel->name, luxname, sizeof(luxname));
			COM_DefaultExtension(luxname, ".dlit", sizeof(luxname));
			luxdata = FS_LoadMallocGroupFile(&loadmodel->memgroup, luxname, &luxsz);
			luxtmp = false;
		}
		if (!luxdata)
		{
			int size;
			luxdata = Q1BSPX_FindLump("LIGHTINGDIR", &size);
			if (size != samples*3)
				luxdata = NULL;
			luxtmp = true;
		}
		else if (luxdata)
		{
			if (l->filelen && l->filelen != (luxsz-8)/3)
			{
				Con_Printf("deluxmap \"%s\" doesn't match level. Ignored.\n", luxname);
				luxdata=NULL;
			}
			else if (luxdata[0] == 'Q' && luxdata[1] == 'L' && luxdata[2] == 'I' && luxdata[3] == 'T')
			{
				if (LittleLong(*(int *)&luxdata[4]) == 1)
				{
					luxdata+=8;
					loadmodel->deluxdata = luxdata;
				}
				else
				{
					Con_Printf("\"%s\" isn't a version 1 deluxmap\n", luxname);
					luxdata=NULL;
				}
			}
			else
			{
				Con_Printf("lit \"%s\" isn't a deluxmap\n", luxname);
				luxdata=NULL;
			}
		}	
	}

	if (!litdata && r_loadlits.value)
	{
		char *litname;
		char litnamemaps[MAX_QPATH];
		char litnamelits[MAX_QPATH];
		int depthmaps;
		int depthlits;
		size_t litsize;
		
		{							
			Q_strncpyz(litnamemaps, loadmodel->name, sizeof(litnamelits));
			COM_StripExtension(loadmodel->name, litnamemaps, sizeof(litnamemaps));
			COM_DefaultExtension(litnamemaps, ".lit", sizeof(litnamemaps));
			depthmaps = COM_FDepthFile(litnamemaps, false); 
		}
		{
			Q_strncpyz(litnamelits, "lits/", sizeof(litnamelits));
			COM_StripExtension(COM_SkipPath(loadmodel->name), litnamelits+5, sizeof(litnamelits) - 5);
			Q_strncatz(litnamelits, ".lit", sizeof(litnamelits));
			depthlits = COM_FDepthFile(litnamelits, false);
		}

		if (depthmaps <= depthlits)
			litname = litnamemaps;	//maps has priority over lits
		else
		{
			litname = litnamelits;
		}

		litdata = FS_LoadMallocGroupFile(&loadmodel->memgroup, litname, &litsize);
		littmp = false;
		if (!litdata)
		{
			int size;
			litdata = Q1BSPX_FindLump("RGBLIGHTING", &size);
			if (size != samples*3)
				litdata = NULL;
			littmp = true;
		}
		else if (litdata[0] == 'Q' && litdata[1] == 'L' && litdata[2] == 'I' && litdata[3] == 'T')
		{
			if (LittleLong(*(int *)&litdata[4]) == 1 && l->filelen && samples*3 != (litsize-8))
			{
				litdata = NULL;
				Con_Printf("lit \"%s\" doesn't match level. Ignored.\n", litname);
			}
			else if (LittleLong(*(int *)&litdata[4]) != 1)
			{
				Con_Printf("lit \"%s\" isn't version 1.\n", litname);
				litdata = NULL;
			}
			else if (lumdata)
			{
				float prop;
				int i;
				qbyte *lum;
				qbyte *lit;

				litdata += 8;

				//now some cheat protection.
				lum = lumdata;
				lit = litdata;

				for (i = 0; i < samples; i++)	//force it to the same intensity. (or less, depending on how you see it...)
				{
#define m(a, b, c) (a>(b>c?b:c)?a:(b>c?b:c))
					prop = (float)m(lit[0],  lit[1], lit[2]);

					if (!prop)
					{
						lit[0] = *lum;
						lit[1] = *lum;
						lit[2] = *lum;
					}
					else
					{
						prop = *lum / prop;
						lit[0] *= prop;
						lit[1] *= prop;
						lit[2] *= prop;
					}

					lum++;
					lit+=3;
				}
				//end anti-cheat
			}
		}
		else if (litdata)
		{
			Con_Printf("lit \"%s\" isn't a lit\n", litname);
			litdata = NULL;
		}
//		else
			//failed to find
	}
#endif

#ifdef RUNTIMELIGHTING
	if (r_loadlits.value == 2 && !lightmodel && (!litdata || (!luxdata && r_deluxemapping.ival)))
	{
		if (!litdata)
			writelitfile = true;
		numlightdata = l->filelen;
		lightmodel = loadmodel;
		relitsurface = 0;
	}

	/*if we're relighting, make sure there's the proper lit data to be updated*/
	if (lightmodel == loadmodel && !litdata)
	{
		int i;
		litdata = ZG_Malloc(&loadmodel->memgroup, samples*3);
		littmp = false;
		if (lumdata)
		{
			for (i = 0; i < samples; i++)
			{
				litdata[i*3+0] = lumdata[i];
				litdata[i*3+1] = lumdata[i];
				litdata[i*3+2] = lumdata[i];
			}
			lumdata = NULL;
		}
	}
	/*if we're relighting, make sure there's the proper lux data to be updated*/
	if (lightmodel == loadmodel && r_deluxemapping.ival && !luxdata)
	{
		int i;
		luxdata = ZG_Malloc(&loadmodel->memgroup, samples*3);
		for (i = 0; i < samples; i++)
		{
			luxdata[i*3+0] = 0.5f*255;
			luxdata[i*3+1] = 0.5f*255;
			luxdata[i*3+2] = 255;
		}
	}
#endif
	
	if (luxdata && luxtmp)
	{
		loadmodel->engineflags |= MDLF_RGBLIGHTING;
		loadmodel->deluxdata = ZG_Malloc(&loadmodel->memgroup, samples*3);
		memcpy(loadmodel->deluxdata, luxdata, samples*3);
	}
	else if (luxdata)
	{
		loadmodel->deluxdata = luxdata;
	}
	else if (interleaveddeluxe)
		loadmodel->deluxdata = ZG_Malloc(&loadmodel->memgroup, samples*3);

	if (litdata && littmp)
	{
		loadmodel->engineflags |= MDLF_RGBLIGHTING;
		loadmodel->lightdata = ZG_Malloc(&loadmodel->memgroup, samples*3);
		/*the memcpy is below*/
		samples*=3;
	}
	else if (litdata)
	{
		loadmodel->engineflags |= MDLF_RGBLIGHTING;
		loadmodel->lightdata = litdata;
		samples*=3;
	}
	else if (lumdata)
	{
		loadmodel->engineflags &= ~MDLF_RGBLIGHTING;
		loadmodel->lightdata = ZG_Malloc(&loadmodel->memgroup, samples);
		litdata = lumdata;
	}

	/*apply lightmap gamma to the entire lightmap*/
	out = loadmodel->lightdata;
	if (interleaveddeluxe)
	{
		qbyte *luxout = loadmodel->deluxdata;
		samples /= 3;
		while(samples-- > 0)
		{
			*out++ = lmgamma[*litdata++];
			*out++ = lmgamma[*litdata++];
			*out++ = lmgamma[*litdata++];
			*luxout++ = *litdata++;
			*luxout++ = *litdata++;
			*luxout++ = *litdata++;
		}
	}
	else
	{
		while(samples-- > 0)
		{
			*out++ = lmgamma[*litdata++];
		}
	}

#ifndef SERVERONLY
	if ((loadmodel->engineflags & MDLF_RGBLIGHTING) && r_lightmap_saturation.value != 1.0f)
		SaturateR8G8B8(loadmodel->lightdata, l->filelen, r_lightmap_saturation.value);
#endif
}

/*
=================
Mod_LoadVisibility
=================
*/
void Mod_LoadVisibility (model_t *loadmodel, qbyte *mod_base, lump_t *l, qbyte *ptr, size_t len)
{
	if (!ptr)
	{
		ptr = mod_base + l->fileofs;
		len = l->filelen;
	}
	if (!len)
	{
		loadmodel->visdata = NULL;
		return;
	}
	loadmodel->visdata = ZG_Malloc(&loadmodel->memgroup, len);	
	memcpy (loadmodel->visdata, ptr, len);
}

char *Mod_ParseWorldspawnKey(const char *ents, const char *key, char *buffer, size_t sizeofbuffer)
{
	char keyname[64];
	char value[1024];
	while(*ents)
	{
		if (*ents == '{')	//an entity
		{
			ents++;
			while (ents && *ents)
			{
				ents = COM_ParseOut(ents, keyname, sizeof(keyname));
				if (*keyname == '}')
					break;
				ents = COM_ParseOut(ents, value, sizeof(value));
				if (!strcmp(keyname, key) || (*keyname == '_' && !strcmp(keyname+1, key)))
				{
					Q_strncpyz(buffer, value, sizeofbuffer);
					return buffer;
				}
			}
			return "";	//worldspawn only.
		}
	}
	return "";	//err...
}

/*
=================
Mod_LoadEntities
=================
*/
void Mod_LoadEntities (model_t *loadmodel, qbyte *mod_base, lump_t *l)
{
	char fname[MAX_QPATH];
	size_t sz;
	loadmodel->entitiescrc = 0;
	loadmodel->entities = NULL;
	if (!l->filelen)
		return;

	if (mod_loadentfiles.value && !loadmodel->entities)
	{
		COM_StripExtension(loadmodel->name, fname, sizeof(fname));
		Q_strncatz(fname, ".ent", sizeof(fname));
		loadmodel->entities = FS_LoadMallocGroupFile(&loadmodel->memgroup, fname, &sz);
	}
	if (mod_loadentfiles.value && !loadmodel->entities)
	{	//tenebrae compat
		COM_StripExtension(loadmodel->name, fname, sizeof(fname));
		Q_strncatz(fname, ".edo", sizeof(fname));
		loadmodel->entities = FS_LoadMallocGroupFile(&loadmodel->memgroup, fname, &sz);
	}
	if (!loadmodel->entities)
	{
		loadmodel->entities = ZG_Malloc(&loadmodel->memgroup, l->filelen + 1);	
		memcpy (loadmodel->entities, mod_base + l->fileofs, l->filelen);
		loadmodel->entities[l->filelen] = 0;
	}
	else
		loadmodel->entitiescrc = QCRC_Block(loadmodel->entities, strlen(loadmodel->entities));
}


/*
=================
Mod_LoadVertexes
=================
*/
qboolean Mod_LoadVertexes (model_t *loadmodel, qbyte *mod_base, lump_t *l)
{
	dvertex_t	*in;
	mvertex_t	*out;
	int			i, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n", loadmodel->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));	

	loadmodel->vertexes = out;
	loadmodel->numvertexes = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		out->position[0] = LittleFloat (in->point[0]);
		out->position[1] = LittleFloat (in->point[1]);
		out->position[2] = LittleFloat (in->point[2]);
	}

	return true;
}

qboolean Mod_LoadVertexNormals (model_t *loadmodel, qbyte *mod_base, lump_t *l)
{
	float	*in;
	float	*out;
	int			i, count;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(vec3_t))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n", loadmodel->name);
		return false;
	}
	count = l->filelen / sizeof(vec3_t);

	if (count != loadmodel->numvertexes)
		return false;	//invalid number of verts there, can't use this.
	out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(vec3_t));	
	loadmodel->normals = (vec3_t*)out;
	for ( i=0 ; i<count ; i++, in+=3, out+=3)
	{
		out[0] = LittleFloat (in[0]);
		out[1] = LittleFloat (in[1]);
		out[2] = LittleFloat (in[2]);
	}
	return true;
}

/*
=================
Mod_LoadSubmodels
=================
*/
static qboolean hexen2map;
qboolean Mod_LoadSubmodels (model_t *loadmodel, qbyte *mod_base, lump_t *l)
{
	dq1model_t	*inq;
	dh2model_t	*inh;
	mmodel_t	*out;
	int			i, j, count;

	//this is crazy!

	inq = (void *)(mod_base + l->fileofs);
	inh = (void *)(mod_base + l->fileofs);
	if (!inq->numfaces)
	{
		hexen2map = true;
		if (l->filelen % sizeof(*inh))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		count = l->filelen / sizeof(*inh);
		out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

		loadmodel->submodels = out;
		loadmodel->numsubmodels = count;

		for ( i=0 ; i<count ; i++, inh++, out++)
		{
			for (j=0 ; j<3 ; j++)
			{	// spread the mins / maxs by a pixel
				out->mins[j] = LittleFloat (inh->mins[j]) - 1;
				out->maxs[j] = LittleFloat (inh->maxs[j]) + 1;
				out->origin[j] = LittleFloat (inh->origin[j]);
			}
			for (j=0 ; j<MAX_MAP_HULLSDH2 ; j++)
			{
				out->headnode[j] = LittleLong (inh->headnode[j]);
			}
			for ( ; j<MAX_MAP_HULLSM ; j++)
				out->headnode[j] = 0;
			for (j=0 ; j<MAX_MAP_HULLSDH2 ; j++)
				out->hullavailable[j] = true;
			for ( ; j<MAX_MAP_HULLSM ; j++)
				out->hullavailable[j] = false;
			out->visleafs = LittleLong (inh->visleafs);
			out->firstface = LittleLong (inh->firstface);
			out->numfaces = LittleLong (inh->numfaces);
		}

	}
	else
	{
		hexen2map = false;
		if (l->filelen % sizeof(*inq))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		count = l->filelen / sizeof(*inq);
		out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));	

		loadmodel->submodels = out;
		loadmodel->numsubmodels = count;

		for ( i=0 ; i<count ; i++, inq++, out++)
		{
			for (j=0 ; j<3 ; j++)
			{	// spread the mins / maxs by a pixel
				out->mins[j] = LittleFloat (inq->mins[j]) - 1;
				out->maxs[j] = LittleFloat (inq->maxs[j]) + 1;
				out->origin[j] = LittleFloat (inq->origin[j]);
			}
			for (j=0 ; j<MAX_MAP_HULLSDQ1 ; j++)
			{
				out->headnode[j] = LittleLong (inq->headnode[j]);
			}
			for ( ; j<MAX_MAP_HULLSM ; j++)
				out->headnode[j] = 0;
			for (j=0 ; j<4 ; j++)
				out->hullavailable[j] = true;
			for ( ; j<MAX_MAP_HULLSM ; j++)
				out->hullavailable[j] = false;
			out->visleafs = LittleLong (inq->visleafs);
			out->firstface = LittleLong (inq->firstface);
			out->numfaces = LittleLong (inq->numfaces);
		}
	}

	return true;
}

/*
=================
Mod_LoadEdges
=================
*/
qboolean Mod_LoadEdges (model_t *loadmodel, qbyte *mod_base, lump_t *l, qboolean lm)
{
	medge_t *out;
	int 	i, count;
	
	if (lm)
	{
		dledge_t *in = (void *)(mod_base + l->fileofs);
		if (l->filelen % sizeof(*in))
		{
			Con_Printf ("MOD_LoadBmodel: funny lump size in %s\n", loadmodel->name);
			return false;
		}
		count = l->filelen / sizeof(*in);
		out = ZG_Malloc(&loadmodel->memgroup, (count + 1) * sizeof(*out));	

		loadmodel->edges = out;
		loadmodel->numedges = count;

		for ( i=0 ; i<count ; i++, in++, out++)
		{
			out->v[0] = LittleLong(in->v[0]);
			out->v[1] = LittleLong(in->v[1]);
		}
	}
	else
	{
		dsedge_t *in = (void *)(mod_base + l->fileofs);
		if (l->filelen % sizeof(*in))
		{
			Con_Printf ("MOD_LoadBmodel: funny lump size in %s\n", loadmodel->name);
			return false;
		}
		count = l->filelen / sizeof(*in);
		out = ZG_Malloc(&loadmodel->memgroup, (count + 1) * sizeof(*out));

		loadmodel->edges = out;
		loadmodel->numedges = count;

		for ( i=0 ; i<count ; i++, in++, out++)
		{
			out->v[0] = (unsigned short)LittleShort(in->v[0]);
			out->v[1] = (unsigned short)LittleShort(in->v[1]);
		}
	}

	return true;
}

/*
=================
Mod_LoadTexinfo
=================
*/
qboolean Mod_LoadTexinfo (model_t *loadmodel, qbyte *mod_base, lump_t *l)
{
	texinfo_t *in;
	mtexinfo_t *out;
	int 	i, j, count;
	int		miptex;

	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

	loadmodel->texinfo = out;
	loadmodel->numtexinfo = count;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		for (j=0 ; j<4 ; j++)
		{
			out->vecs[0][j] = LittleFloat (in->vecs[0][j]);
			out->vecs[1][j] = LittleFloat (in->vecs[1][j]);
		}
		out->vecscale[0] = 1.0/Length (out->vecs[0]);
		out->vecscale[1] = 1.0/Length (out->vecs[1]);

		miptex = LittleLong (in->miptex);
		out->flags = LittleLong (in->flags);
	
		if (loadmodel->numtextures)
			out->texture = loadmodel->textures[miptex % loadmodel->numtextures];
		else
			out->texture = NULL;
		if (!out->texture)
		{
			out->texture = r_notexture_mip; // texture not found
			out->flags = 0;
		}
	}

	return true;
}

/*
================
CalcSurfaceExtents

Fills in s->texturemins[] and s->extents[]
================
*/

void CalcSurfaceExtents (model_t *mod, msurface_t *s);
/*
{
	float	mins[2], maxs[2], val;
	int		i,j, e;
	mvertex_t	*v;
	mtexinfo_t	*tex;
	int		bmins[2], bmaxs[2];

	mins[0] = mins[1] = 999999;
	maxs[0] = maxs[1] = -99999;

	tex = s->texinfo;
	
	for (i=0 ; i<s->numedges ; i++)
	{
		e = loadmodel->surfedges[s->firstedge+i];
		if (e >= 0)
			v = &loadmodel->vertexes[loadmodel->edges[e].v[0]];
		else
			v = &loadmodel->vertexes[loadmodel->edges[-e].v[1]];
		
		for (j=0 ; j<2 ; j++)
		{
			val = v->position[0] * tex->vecs[j][0] + 
				v->position[1] * tex->vecs[j][1] +
				v->position[2] * tex->vecs[j][2] +
				tex->vecs[j][3];
			if (val < mins[j])
				mins[j] = val;
			if (val > maxs[j])
				maxs[j] = val;
		}
	}

	for (i=0 ; i<2 ; i++)
	{	
		bmins[i] = floor(mins[i]/16);
		bmaxs[i] = ceil(maxs[i]/16);

		s->texturemins[i] = bmins[i];
		s->extents[i] = (bmaxs[i] - bmins[i]);

//		if ( !(tex->flags & TEX_SPECIAL) && s->extents[i] > 512 )	//q2 uses 512.
//			Sys_Error ("Bad surface extents");
	}
}
*/

/*
=================
Mod_LoadFaces
=================
*/
qboolean Mod_LoadFaces (model_t *loadmodel, qbyte *mod_base, lump_t *l, qboolean lm)
{
	dsface_t		*ins;
	dlface_t		*inl;
	msurface_t 	*out;
	int			count, surfnum;
	int			i, planenum, side;
	int tn, lofs;

	unsigned short lmshift, lmscale;
	char buf[64];

	lmscale = atoi(Mod_ParseWorldspawnKey(loadmodel->entities, "lightmap_scale", buf, sizeof(buf)));
	if (!lmscale)
		lmshift = LMSHIFT_DEFAULT;
	else
	{
		for(lmshift = 0; lmscale > 1; lmshift++)
			lmscale >>= 1;
	}

	if (lm)
	{
		ins = NULL;
		inl = (void *)(mod_base + l->fileofs);
		if (l->filelen % sizeof(*inl))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		count = l->filelen / sizeof(*inl);
	}
	else
	{
		ins = (void *)(mod_base + l->fileofs);
		inl = NULL;
		if (l->filelen % sizeof(*ins))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		count = l->filelen / sizeof(*ins);
	}
	out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

//	*meshlist = ZG_Malloc(&loadmodel->memgroup, count*sizeof(**meshlist));
	loadmodel->surfaces = out;
	loadmodel->numsurfaces = count;
	for ( surfnum=0 ; surfnum<count ; surfnum++, out++)
	{
		if (lm)
		{
			planenum = LittleLong(inl->planenum);
			side = LittleLong(inl->side);
			out->firstedge = LittleLong(inl->firstedge);
			out->numedges = LittleLong(inl->numedges);
			tn = LittleLong (inl->texinfo);
			for (i=0 ; i<MAXRLIGHTMAPS ; i++)
				out->styles[i] = inl->styles[i];
			lofs = LittleLong(inl->lightofs);
			inl++;
		}
		else
		{
			planenum = LittleShort(ins->planenum);
			side = LittleShort(ins->side);
			out->firstedge = LittleLong(ins->firstedge);
			out->numedges = LittleShort(ins->numedges);
			tn = LittleShort (ins->texinfo);
			for (i=0 ; i<MAXQ1LIGHTMAPS ; i++)
				out->styles[i] = ins->styles[i];
			lofs = LittleLong(ins->lightofs);
			ins++;
		}
//		(*meshlist)[surfnum].vbofirstvert = out->firstedge;
//		(*meshlist)[surfnum].numvertexes = out->numedges;
		out->flags = 0;

		if (side)
			out->flags |= SURF_PLANEBACK;			

		out->plane = loadmodel->planes + planenum;

		if (tn < 0 || tn >= loadmodel->numtexinfo)
		{
			Con_Printf("texinfo 0 <= %i < %i\n", tn, loadmodel->numtexinfo);
			return false;
		}
		out->texinfo = loadmodel->texinfo + tn;

		out->lmshift = lmshift;

		CalcSurfaceExtents (loadmodel, out);
		if (lofs == -1)
			out->samples = NULL;
		else if ((loadmodel->engineflags & MDLF_RGBLIGHTING) && loadmodel->fromgame != fg_halflife)
			out->samples = loadmodel->lightdata + lofs*3;
		else
			out->samples = loadmodel->lightdata + lofs;

		if (!out->texinfo->texture)
			continue;

		
	// set the drawing flags flag		
		if (!Q_strncmp(out->texinfo->texture->name,"sky",3))	// sky
		{
			out->flags |= (SURF_DRAWSKY | SURF_DRAWTILED);
			continue;
		}
		
		if (!Q_strncmp(out->texinfo->texture->name,"*",1))		// turbulent
		{
			out->flags |= (SURF_DRAWTURB | SURF_DRAWTILED);
			for (i=0 ; i<2 ; i++)
			{
				out->extents[i] = 16384;
				out->texturemins[i] = -8192;
			}
			continue;
		}

		/*if (*out->texinfo->texture->name == '~')
		{
			out->texinfo->flags |= SURF_BLENDED;
			continue;
		}*/
		if (!Q_strncmp(out->texinfo->texture->name,"{",1))		// alpha
		{
			out->flags |= (SURF_DRAWALPHA);
			continue;
		}
		if (!Q_strncmp(out->texinfo->texture->name,"glass",5))		// alpha
		{
			out->flags |= (SURF_DRAWALPHA);
			continue;
		}
		if (out->flags & SURF_DRAWALPHA)
			out->flags &= ~SURF_DRAWALPHA;
	}

	return true;
}

#ifndef SERVERONLY
void ModQ1_Batches_BuildQ1Q2Poly(model_t *mod, msurface_t *surf, void *cookie)
{
	unsigned int vertidx;
	int i, lindex;
	mesh_t *mesh = surf->mesh;
	medge_t *pedge;
	float *vec;
	float s, t, d;
	int sty;

	//output the mesh's indicies
	for (i=0 ; i<mesh->numvertexes-2 ; i++)
	{
		mesh->indexes[i*3] = 0;
		mesh->indexes[i*3+1] = i+1;
		mesh->indexes[i*3+2] = i+2;
	}
	//output the renderable verticies
	for (i=0 ; i<mesh->numvertexes ; i++)
	{
		lindex = mod->surfedges[surf->firstedge + i];

		if (lindex > 0)
		{
			pedge = &mod->edges[lindex];
			vertidx = pedge->v[0];
		}
		else
		{
			pedge = &mod->edges[-lindex];
			vertidx = pedge->v[1];
		}
		vec = mod->vertexes[vertidx].position;

		s = DotProduct (vec, surf->texinfo->vecs[0]) + surf->texinfo->vecs[0][3];
		t = DotProduct (vec, surf->texinfo->vecs[1]) + surf->texinfo->vecs[1][3];

		VectorCopy (vec, mesh->xyz_array[i]);
		mesh->st_array[i][0] = s/surf->texinfo->texture->width;
		mesh->st_array[i][1] = t/surf->texinfo->texture->height;

		if (gl_lightmap_average.ival)
		{
			for (sty = 0; sty < 1; sty++)
			{
				mesh->lmst_array[sty][i][0] = (surf->extents[0]*0.5 + (surf->light_s[sty]<<surf->lmshift) + (1<<surf->lmshift)*0.5) / (mod->lightmaps.width<<surf->lmshift);
				mesh->lmst_array[sty][i][1] = (surf->extents[1]*0.5 + (surf->light_t[sty]<<surf->lmshift) + (1<<surf->lmshift)*0.5) / (mod->lightmaps.height<<surf->lmshift);
			}
		}
		else
		{
			for (sty = 0; sty < 1; sty++)
			{
				mesh->lmst_array[sty][i][0] = (s - surf->texturemins[0] + (surf->light_s[sty]<<surf->lmshift) + (1<<surf->lmshift)*0.5) / (mod->lightmaps.width<<surf->lmshift);
				mesh->lmst_array[sty][i][1] = (t - surf->texturemins[1] + (surf->light_t[sty]<<surf->lmshift) + (1<<surf->lmshift)*0.5) / (mod->lightmaps.height<<surf->lmshift);
			}
		}

		//figure out the texture directions, for bumpmapping and stuff
		if (mod->normals && (surf->texinfo->flags & 0x800) && (mod->normals[vertidx][0] || mod->normals[vertidx][1] || mod->normals[vertidx][2])) 
		{
			//per-vertex normals - used for smoothing groups and stuff.
			VectorCopy(mod->normals[vertidx], mesh->normals_array[i]);
		}
		else
		{
			if (surf->flags & SURF_PLANEBACK)
				VectorNegate(surf->plane->normal, mesh->normals_array[i]);
			else
				VectorCopy(surf->plane->normal, mesh->normals_array[i]);
		}
		VectorCopy(surf->texinfo->vecs[0], mesh->snormals_array[i]);
		VectorNegate(surf->texinfo->vecs[1], mesh->tnormals_array[i]);
		//the s+t vectors are axis-aligned, so fiddle them so they're normal aligned instead
		d = -DotProduct(mesh->normals_array[i], mesh->snormals_array[i]);
		VectorMA(mesh->snormals_array[i], d, mesh->normals_array[i], mesh->snormals_array[i]);
		d = -DotProduct(mesh->normals_array[i], mesh->tnormals_array[i]);
		VectorMA(mesh->tnormals_array[i], d, mesh->normals_array[i], mesh->tnormals_array[i]);
		VectorNormalize(mesh->snormals_array[i]);
		VectorNormalize(mesh->tnormals_array[i]);

		//q1bsp has no colour information (fixme: sample from the lightmap?)
		for (sty = 0; sty < 1; sty++)
		{
			mesh->colors4f_array[sty][i][0] = 1;
			mesh->colors4f_array[sty][i][1] = 1;
			mesh->colors4f_array[sty][i][2] = 1;
			mesh->colors4f_array[sty][i][3] = 1;
		}
	}
}

static void Mod_Batches_BuildModelMeshes(model_t *mod, int maxverts, int maxindicies, void (*build)(model_t *mod, msurface_t *surf, builddata_t *bd), builddata_t *bd)
{
	batch_t *batch;
	msurface_t *surf;
	mesh_t *mesh;
	int numverts = 0;
	int numindicies = 0;
	int j;
	int sortid;
	int sty;
	vbo_t vbo;
	int styles = mod->lightmaps.surfstyles;
	char *ptr;

	memset(&vbo, 0, sizeof(vbo));
	vbo.indicies.sysptr = ZG_Malloc(&mod->memgroup, sizeof(index_t) * maxindicies);
	ptr = ZG_Malloc(&mod->memgroup, (sizeof(vecV_t)+sizeof(vec2_t)*(1+styles)+sizeof(vec3_t)*3+sizeof(vec4_t)*styles)* maxverts);

	vbo.coord.sysptr = ptr;
	ptr += sizeof(vecV_t)*maxverts;
	for (sty = 0; sty < styles; sty++)
	{
		vbo.colours[sty].sysptr = ptr;
		ptr += sizeof(vec4_t)*maxverts;
	}
	for (; sty < MAXRLIGHTMAPS; sty++)
		vbo.colours[sty].sysptr = NULL;
	vbo.texcoord.sysptr = ptr;
	ptr += sizeof(vec2_t)*maxverts;
	sty = 0;
	for (; sty < styles; sty++)
	{
		vbo.lmcoord[sty].sysptr = ptr;
		ptr += sizeof(vec2_t)*maxverts;
	}
	for (; sty < MAXRLIGHTMAPS; sty++)
		vbo.lmcoord[sty].sysptr = NULL;
	vbo.normals.sysptr = ptr;
	ptr += sizeof(vec3_t)*maxverts;
	vbo.svector.sysptr = ptr;
	ptr += sizeof(vec3_t)*maxverts;
	vbo.tvector.sysptr = ptr;
	ptr += sizeof(vec3_t)*maxverts;

	numindicies = 0;
	numverts = 0;

	//build each mesh
	for (sortid=0; sortid<SHADER_SORT_COUNT; sortid++)
	{
		for (batch = mod->batches[sortid]; batch; batch = batch->next)
		{
			for (j = 0; j < batch->maxmeshes; j++)
			{
				surf = (msurface_t*)batch->mesh[j];
				mesh = surf->mesh;
				batch->mesh[j] = mesh;

				mesh->vbofirstvert = numverts;
				mesh->vbofirstelement = numindicies;
				numverts += mesh->numvertexes;
				numindicies += mesh->numindexes;

				//set up the arrays. the arrangement is required for the backend to optimise vbos
				mesh->xyz_array = (vecV_t*)vbo.coord.sysptr + mesh->vbofirstvert;
				mesh->st_array = (vec2_t*)vbo.texcoord.sysptr + mesh->vbofirstvert;
				for (sty = 0; sty < MAXRLIGHTMAPS; sty++)
				{
					if (vbo.lmcoord[sty].sysptr)
						mesh->lmst_array[sty] = (vec2_t*)vbo.lmcoord[sty].sysptr + mesh->vbofirstvert;
					else
						mesh->lmst_array[sty] = NULL;
					if (vbo.colours[sty].sysptr)
						mesh->colors4f_array[sty] = (vec4_t*)vbo.colours[sty].sysptr + mesh->vbofirstvert;
					else
						mesh->colors4f_array[sty] = NULL;
				}
				mesh->normals_array = (vec3_t*)vbo.normals.sysptr + mesh->vbofirstvert;
				mesh->snormals_array = (vec3_t*)vbo.svector.sysptr + mesh->vbofirstvert;
				mesh->tnormals_array = (vec3_t*)vbo.tvector.sysptr + mesh->vbofirstvert;
				mesh->indexes = (index_t*)vbo.indicies.sysptr + mesh->vbofirstelement;

				mesh->vbofirstvert = 0;
				mesh->vbofirstelement = 0;

				build(mod, surf, bd);
			}
			batch->meshes = 0;
			batch->firstmesh = 0;
		}
	}
}

//q1 autoanimates. if the frame is set, it uses the alternate animation.
void Mod_UpdateBatchShader_Q1 (struct batch_s *batch)
{
	texture_t *base = batch->texture;
	int		reletive;
	int		count;

	if (batch->ent->framestate.g[FS_REG].frame[0])
	{
		if (base->alternate_anims)
			base = base->alternate_anims;
	}

	if (base->anim_total)
	{
		reletive = (int)(cl.time*10) % base->anim_total;

		count = 0;
		while (base->anim_min > reletive || base->anim_max <= reletive)
		{
			base = base->anim_next;
			if (!base)
				Sys_Error ("R_TextureAnimation: broken cycle");
			if (++count > 100)
				Sys_Error ("R_TextureAnimation: infinite cycle");
		}
	}

	batch->shader = base->shader;
}

//q2 has direct control over the texture frames used, but typically has the client generate the frame (different flags autogenerate different ranges).
void Mod_UpdateBatchShader_Q2 (struct batch_s *batch)
{
	texture_t *base = batch->texture;
	int		reletive;
	int frame = batch->ent->framestate.g[FS_REG].frame[0];
	if (batch->ent == &r_worldentity)
		frame = cl.time*2;

	if (base->anim_total)
	{
		reletive = frame % base->anim_total;
		while (reletive --> 0)
		{
			base = base->anim_next;
			if (!base)
				Sys_Error ("R_TextureAnimation: broken cycle");
		}
	}

	batch->shader = base->shader;
}

/*
batch->firstmesh is set only in and for this function, its cleared out elsewhere
*/
static void Mod_Batches_Generate(model_t *mod)
{
	int i;
	msurface_t *surf;
	shader_t *shader;
	int sortid;
	batch_t *batch, *lbatch = NULL;
	vec4_t plane;

	//for each surface, find a suitable batch to insert it into.
	//we use 'firstmesh' to avoid chucking out too many verts in a single vbo (gl2 hardware tends to have a 16bit limit)
	for (i=0; i<mod->nummodelsurfaces; i++)
	{
		surf = mod->surfaces + mod->firstmodelsurface + i;
		shader = surf->texinfo->texture->shader;

		if (shader)
		{
			sortid = shader->sort;

			//shaders that are portals need to be split into separate batches to have the same surface planes
			if (sortid == SHADER_SORT_PORTAL || (shader->flags & (SHADER_HASREFLECT | SHADER_HASREFRACT)))
			{
				if (surf->flags & SURF_PLANEBACK)
				{
					VectorNegate(surf->plane->normal, plane);
					plane[3] = -surf->plane->dist;
				}
				else
				{
					VectorCopy(surf->plane->normal, plane);
					plane[3] = surf->plane->dist;
				}
			}
			else
			{
				VectorClear(plane);
				plane[3] = 0;
			}
		}
		else
		{
			sortid = SHADER_SORT_OPAQUE;
			VectorClear(plane);
			plane[3] = 0;
		}

		if (lbatch && (
					lbatch->texture == surf->texinfo->texture &&
					lbatch->lightmap[0] == surf->lightmaptexturenums[0] &&
					Vector4Compare(plane, lbatch->plane) &&
					lbatch->firstmesh + surf->mesh->numvertexes <= MAX_INDICIES) &&
#if MAXRLIGHTMAPS > 1
					lbatch->lightmap[1] == surf->lightmaptexturenums[1] &&
					lbatch->lightmap[2] == surf->lightmaptexturenums[2] &&
					lbatch->lightmap[3] == surf->lightmaptexturenums[3] &&
#endif
					lbatch->fog == surf->fog)
			batch = lbatch;
		else
		{
			for (batch = mod->batches[sortid]; batch; batch = batch->next)
			{
				if (
							batch->texture == surf->texinfo->texture &&
							batch->lightmap[0] == surf->lightmaptexturenums[0] &&
							Vector4Compare(plane, batch->plane) &&
							batch->firstmesh + surf->mesh->numvertexes <= MAX_INDICIES &&
#if MAXRLIGHTMAPS > 1
							batch->lightmap[1] == surf->lightmaptexturenums[1] &&
							batch->lightmap[2] == surf->lightmaptexturenums[2] &&
							batch->lightmap[3] == surf->lightmaptexturenums[3] &&
#endif
							batch->fog == surf->fog)
					break;
			}
		}
		if (!batch)
		{
			batch = ZG_Malloc(&mod->memgroup, sizeof(*batch));
			batch->lightmap[0] = surf->lightmaptexturenums[0];
#if MAXRLIGHTMAPS > 1
			batch->lightmap[1] = surf->lightmaptexturenums[1];
			batch->lightmap[2] = surf->lightmaptexturenums[2];
			batch->lightmap[3] = surf->lightmaptexturenums[3];
#endif
			batch->texture = surf->texinfo->texture;
			batch->shader = surf->texinfo->texture->shader;
			if (surf->texinfo->texture->alternate_anims || surf->texinfo->texture->anim_total)
			{
				if (mod->fromgame == fg_quake2)
					batch->buildmeshes = Mod_UpdateBatchShader_Q2;
				else
					batch->buildmeshes = Mod_UpdateBatchShader_Q1;
			}
			batch->next = mod->batches[sortid];
			batch->ent = &r_worldentity;
			batch->fog = surf->fog;
			Vector4Copy(plane, batch->plane);

			mod->batches[sortid] = batch;
		}

		surf->sbatch = batch;	//let the surface know which batch its in
		batch->maxmeshes++;
		batch->firstmesh += surf->mesh->numvertexes;

		lbatch = batch;
	}
}

typedef struct
{
	int allocated[LMBLOCK_SIZE_MAX];
	int lmnum;
	unsigned int width;
	unsigned int height;
	qboolean deluxe;
} lmalloc_t;
#define LM_FIRST 0x50
static void Mod_LightmapAllocInit(lmalloc_t *lmallocator, qboolean hasdeluxe, unsigned int width, unsigned int height)
{
	memset(lmallocator, 0, sizeof(*lmallocator));
	lmallocator->deluxe = hasdeluxe;
	lmallocator->lmnum = LM_FIRST;

	lmallocator->width = width;
	lmallocator->height = height;
}
static void Mod_LightmapAllocDone(lmalloc_t *lmallocator, model_t *mod)
{
	mod->lightmaps.first = LM_FIRST;
	mod->lightmaps.count = (lmallocator->lmnum - LM_FIRST);
	if (lmallocator->allocated[0])
		mod->lightmaps.count++;

	if (lmallocator->deluxe)
	{
		mod->lightmaps.first*=2;
		mod->lightmaps.count*=2;
		mod->lightmaps.deluxemapping = true;
	}
	else
		mod->lightmaps.deluxemapping = false;
}
static void Mod_LightmapAllocBlock(lmalloc_t *lmallocator, int w, int h, unsigned short *x, unsigned short *y, int *tnum)
{
	int best, best2;
	int i, j;

	for(;;)
	{
		best = lmallocator->height;

		for (i = 0; i <= lmallocator->width - w; i++)
		{
			best2 = 0;

			for (j=0; j < w; j++)
			{
				if (lmallocator->allocated[i+j] >= best)
					break;
				if (lmallocator->allocated[i+j] > best2)
					best2 = lmallocator->allocated[i+j];
			}
			if (j == w)
			{	// this is a valid spot
				*x = i;
				*y = best = best2;
			}
		}

		if (best + h > lmallocator->height)
		{
			memset(lmallocator->allocated, 0, sizeof(lmallocator->allocated));
			lmallocator->lmnum++;
			continue;
		}

		for (i=0; i < w; i++)
			lmallocator->allocated[*x + i] = best + h;

		if (lmallocator->deluxe)
			*tnum = lmallocator->lmnum*2;
		else
			*tnum = lmallocator->lmnum;
		break;
	}
}

static void Mod_LightmapAllocSurf(lmalloc_t *lmallocator, msurface_t *surf, int surfstyle)
{
	int smax, tmax;
	smax = (surf->extents[0]>>surf->lmshift)+1;
	tmax = (surf->extents[1]>>surf->lmshift)+1;

	if (isDedicated ||
		(surf->texinfo->texture->shader && !(surf->texinfo->texture->shader->flags & SHADER_HASLIGHTMAP)) || //fte
		(surf->flags & (SURF_DRAWSKY|SURF_DRAWTURB)) ||	//q1
		(surf->texinfo->flags & TEX_SPECIAL) ||	//the original 'no lightmap'
		(surf->texinfo->flags & (TI_SKY|TI_TRANS33|TI_TRANS66|TI_WARP)) ||	//q2 surfaces
		smax > lmallocator->width || tmax > lmallocator->height || smax < 0 || tmax < 0)	//bugs/bounds/etc
	{
		surf->lightmaptexturenums[surfstyle] = -1;
		return;
	}

	Mod_LightmapAllocBlock (lmallocator, smax, tmax, &surf->light_s[surfstyle], &surf->light_t[surfstyle], &surf->lightmaptexturenums[surfstyle]);
}

static void Mod_Batches_SplitLightmaps(model_t *mod)
{
	batch_t *batch;
	batch_t *nb;
	int i, j, sortid;
	msurface_t *surf;
	int sty;


	for (sortid = 0; sortid < SHADER_SORT_COUNT; sortid++)
	for (batch = mod->batches[sortid]; batch != NULL; batch = batch->next)
	{
		surf = (msurface_t*)batch->mesh[0];
		for (sty = 0; sty < MAXRLIGHTMAPS; sty++)
		{
			batch->lightmap[sty] = surf->lightmaptexturenums[sty];
			batch->lmlightstyle[sty] = surf->styles[sty];
		}

		for (j = 1; j < batch->maxmeshes; j++)
		{
			surf = (msurface_t*)batch->mesh[j];
			for (sty = 0; sty < MAXRLIGHTMAPS; sty++)
			{
				if (surf->lightmaptexturenums[sty] != batch->lightmap[sty] ||
					//fixme: we should merge later (reverted matching) surfaces into the prior batch
					surf->styles[sty] != batch->lmlightstyle[sty] ||
					surf->vlstyles[sty] != batch->vtlightstyle[sty])
					break;
			}
			if (sty < MAXRLIGHTMAPS)
			{
				nb = ZG_Malloc(&mod->memgroup, sizeof(*batch));
				*nb = *batch;
				batch->next = nb;

				nb->mesh = batch->mesh + j*2;
				nb->maxmeshes = batch->maxmeshes - j;
				batch->maxmeshes = j;
				for (sty = 0; sty < MAXRLIGHTMAPS; sty++)
				{
					nb->lightmap[sty] = surf->lightmaptexturenums[sty];
					nb->lmlightstyle[sty] = surf->styles[sty];
					nb->vtlightstyle[sty] = surf->vlstyles[sty];
				}

				memmove(nb->mesh, batch->mesh+j, sizeof(msurface_t*)*nb->maxmeshes);

				for (i = 0; i < nb->maxmeshes; i++)
				{
					surf = (msurface_t*)nb->mesh[i];
					surf->sbatch = nb;
				}

				batch = nb;
				j = 1;
			}
		}
	}
}

/*
allocates lightmaps and splits batches upon lightmap boundaries
*/
static void Mod_Batches_AllocLightmaps(model_t *mod)
{
	batch_t *batch;
	batch_t *nb;
	lmalloc_t lmallocator;
	int i, j, sortid;
	msurface_t *surf;
	int sty;

	size_t samps = 0;

	//small models don't have many surfaces, don't allocate a smegging huge lightmap that simply won't be used.
	for (i=0, j=0; i<mod->nummodelsurfaces; i++)
	{
		surf = mod->surfaces + mod->firstmodelsurface + i;
		if (surf->texinfo->flags & TEX_SPECIAL)
			continue;	//surfaces with no lightmap should not count torwards anything.
		samps += ((surf->extents[0]>>surf->lmshift)+1) * ((surf->extents[1]>>surf->lmshift)+1);

		if (j < (surf->extents[0]>>surf->lmshift)+1)
			j = (surf->extents[0]>>surf->lmshift)+1;
		if (j < (surf->extents[1]>>surf->lmshift)+1)
			j = (surf->extents[1]>>surf->lmshift)+1;
	}
	samps /= 4;
	samps = sqrt(samps);
	mod->lightmaps.width = bound(j, samps, LMBLOCK_SIZE_MAX);
	mod->lightmaps.height = bound(j, samps, LMBLOCK_SIZE_MAX);
	for (i = 0; (1<<i) < mod->lightmaps.width; i++);
	mod->lightmaps.width = 1<<i;
	for (i = 0; (1<<i) < mod->lightmaps.height; i++);
	mod->lightmaps.height = 1<<i;
	mod->lightmaps.width = bound(64, mod->lightmaps.width, sh_config.texture_maxsize);
	mod->lightmaps.height = bound(64, mod->lightmaps.height, sh_config.texture_maxsize);

	Mod_LightmapAllocInit(&lmallocator, mod->deluxdata != NULL, mod->lightmaps.width, mod->lightmaps.height);

	for (sortid = 0; sortid < SHADER_SORT_COUNT; sortid++)
	for (batch = mod->batches[sortid]; batch != NULL; batch = batch->next)
	{
		surf = (msurface_t*)batch->mesh[0];
		Mod_LightmapAllocSurf (&lmallocator, surf, 0);
		for (sty = 1; sty < MAXRLIGHTMAPS; sty++)
			surf->lightmaptexturenums[sty] = -1;
		for (sty = 0; sty < MAXRLIGHTMAPS; sty++)
		{
			batch->lightmap[sty] = surf->lightmaptexturenums[sty];
			batch->lmlightstyle[sty] = 255;//don't do special backend rendering of lightstyles.
			batch->vtlightstyle[sty] = 255;//don't do special backend rendering of lightstyles.
		}

		for (j = 1; j < batch->maxmeshes; j++)
		{
			surf = (msurface_t*)batch->mesh[j];
			Mod_LightmapAllocSurf (&lmallocator, surf, 0);
			for (sty = 1; sty < MAXRLIGHTMAPS; sty++)
				surf->lightmaptexturenums[sty] = -1;
			if (surf->lightmaptexturenums[0] != batch->lightmap[0])
			{
				nb = ZG_Malloc(&mod->memgroup, sizeof(*batch));
				*nb = *batch;
				batch->next = nb;

				nb->mesh = batch->mesh + j*2;
				nb->maxmeshes = batch->maxmeshes - j;
				batch->maxmeshes = j;
				for (sty = 0; sty < MAXRLIGHTMAPS; sty++)
					nb->lightmap[sty] = surf->lightmaptexturenums[sty];

				memmove(nb->mesh, batch->mesh+j, sizeof(msurface_t*)*nb->maxmeshes);

				for (i = 0; i < nb->maxmeshes; i++)
				{
					surf = (msurface_t*)nb->mesh[i];
					surf->sbatch = nb;
				}

				batch = nb;
				j = 0;
			}
		}
	}

	Mod_LightmapAllocDone(&lmallocator, mod);
}

extern void Surf_CreateSurfaceLightmap (msurface_t *surf, int shift);
//if build is NULL, uses q1/q2 surf generation, and allocates lightmaps
void Mod_Batches_Build(model_t *mod, builddata_t *bd)
{
	int i;
	int numverts = 0, numindicies=0;
	msurface_t *surf;
	mesh_t *mesh;
	mesh_t **bmeshes;
	int sortid;
	batch_t *batch;
	mesh_t *meshlist;

	currentmodel = mod;

	if (!mod->textures)
		return;

	if (bd)
		meshlist = NULL;
	else
		meshlist = ZG_Malloc(&mod->memgroup, sizeof(mesh_t) * mod->nummodelsurfaces);

	for (i=0; i<mod->nummodelsurfaces; i++)
	{
		surf = mod->surfaces + i + mod->firstmodelsurface;
		if (meshlist)
		{
			mesh = surf->mesh = &meshlist[i];
			mesh->numvertexes = surf->numedges;
			mesh->numindexes = (surf->numedges-2)*3;
		}
		else
			mesh = surf->mesh;

		numverts += mesh->numvertexes;
		numindicies += mesh->numindexes;
//		surf->lightmaptexturenum = -1;
	}

	/*assign each mesh to a batch, generating as needed*/
	Mod_Batches_Generate(mod);

	bmeshes = ZG_Malloc(&mod->memgroup, sizeof(*bmeshes)*mod->nummodelsurfaces*R_MAX_RECURSE);

	//we now know which batch each surface is in, and how many meshes there are in each batch.
	//allocate the mesh-pointer-lists for each batch. *2 for recursion.
	for (i = 0, sortid = 0; sortid < SHADER_SORT_COUNT; sortid++)
	for (batch = mod->batches[sortid]; batch != NULL; batch = batch->next)
	{
		batch->mesh = bmeshes + i;
		i += batch->maxmeshes*R_MAX_RECURSE;
	}
	//store the *surface* into the batch's mesh list (yes, this is an evil cast hack, but at least both are pointers)
	for (i=0; i<mod->nummodelsurfaces; i++)
	{
		surf = mod->surfaces + mod->firstmodelsurface + i;
		surf->sbatch->mesh[surf->sbatch->meshes++] = (mesh_t*)surf;
	}
	if (bd)	//q3
		Mod_Batches_SplitLightmaps(mod);
	else
		Mod_Batches_AllocLightmaps(mod);

	if (!bd)
	{
		mod->lightmaps.surfstyles = 1;
		Mod_Batches_BuildModelMeshes(mod, numverts, numindicies, ModQ1_Batches_BuildQ1Q2Poly, bd);
	}
	else
		Mod_Batches_BuildModelMeshes(mod, numverts, numindicies, bd->buildfunc, bd);

	if (BE_GenBrushModelVBO)
		BE_GenBrushModelVBO(mod);
}
#endif

/*
=================
Mod_SetParent
=================
*/
void Mod_SetParent (mnode_t *node, mnode_t *parent)
{
	if (!node)
		return;
	node->parent = parent;
	if (node->contents < 0)
		return;
	Mod_SetParent (node->children[0], node);
	Mod_SetParent (node->children[1], node);
}

/*
=================
Mod_LoadNodes
=================
*/
static qboolean Mod_LoadNodes (model_t *loadmodel, qbyte *mod_base, lump_t *l, int lm)
{
	int			i, j, count, p;
	mnode_t 	*out;

	if (lm == 2)
	{
		dl2node_t		*in;
		in = (void *)(mod_base + l->fileofs);
		if (l->filelen % sizeof(*in))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		count = l->filelen / sizeof(*in);
		out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

		loadmodel->nodes = out;
		loadmodel->numnodes = count;

		for ( i=0 ; i<count ; i++, in++, out++)
		{
			for (j=0 ; j<3 ; j++)
			{
				out->minmaxs[j] = LittleFloat (in->mins[j]);
				out->minmaxs[3+j] = LittleFloat (in->maxs[j]);
			}
		
			p = LittleLong(in->planenum);
			out->plane = loadmodel->planes + p;

			out->firstsurface = LittleLong (in->firstface);
			out->numsurfaces = LittleLong (in->numfaces);
			
			for (j=0 ; j<2 ; j++)
			{
				p = LittleLong (in->children[j]);
				if (p >= 0)
					out->children[j] = loadmodel->nodes + p;
				else
					out->children[j] = (mnode_t *)(loadmodel->leafs + (-1 - p));
			}
		}
	}
	else if (lm)
	{
		dl1node_t		*in;
		in = (void *)(mod_base + l->fileofs);
		if (l->filelen % sizeof(*in))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		count = l->filelen / sizeof(*in);
		out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

		loadmodel->nodes = out;
		loadmodel->numnodes = count;

		for ( i=0 ; i<count ; i++, in++, out++)
		{
			for (j=0 ; j<3 ; j++)
			{
				out->minmaxs[j] = LittleShort (in->mins[j]);
				out->minmaxs[3+j] = LittleShort (in->maxs[j]);
			}
		
			p = LittleLong(in->planenum);
			out->plane = loadmodel->planes + p;

			out->firstsurface = LittleLong (in->firstface);
			out->numsurfaces = LittleLong (in->numfaces);
			
			for (j=0 ; j<2 ; j++)
			{
				p = LittleLong (in->children[j]);
				if (p >= 0)
					out->children[j] = loadmodel->nodes + p;
				else
					out->children[j] = (mnode_t *)(loadmodel->leafs + (-1 - p));
			}
		}
	}
	else
	{
		dsnode_t		*in;
		in = (void *)(mod_base + l->fileofs);
		if (l->filelen % sizeof(*in))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		count = l->filelen / sizeof(*in);
		out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

		loadmodel->nodes = out;
		loadmodel->numnodes = count;

		for ( i=0 ; i<count ; i++, in++, out++)
		{
			for (j=0 ; j<3 ; j++)
			{
				out->minmaxs[j] = LittleShort (in->mins[j]);
				out->minmaxs[3+j] = LittleShort (in->maxs[j]);
			}
		
			p = LittleLong(in->planenum);
			out->plane = loadmodel->planes + p;

			out->firstsurface = (unsigned short)LittleShort (in->firstface);
			out->numsurfaces = (unsigned short)LittleShort (in->numfaces);
			
			for (j=0 ; j<2 ; j++)
			{
				p = LittleShort (in->children[j]);
				if (p >= 0)
					out->children[j] = loadmodel->nodes + p;
				else
					out->children[j] = (mnode_t *)(loadmodel->leafs + (-1 - p));
			}
		}
	}
	
	Mod_SetParent (loadmodel->nodes, NULL);	// sets nodes and leafs
	return true;
}

/*
=================
Mod_LoadLeafs
=================
*/
static qboolean Mod_LoadLeafs (model_t *loadmodel, qbyte *mod_base, lump_t *l, int lm, qbyte *ptr, size_t len)
{
	mleaf_t 	*out;
	int			i, j, count, p;

	if (!ptr)
	{
		ptr = mod_base + l->fileofs;
		len = l->filelen;
	}

	if (lm==2)
	{
		dl2leaf_t 	*in;
		in = (void *)ptr;
		if (len % sizeof(*in))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		count = len / sizeof(*in);
		if (count > MAX_MAP_LEAFS)
		{
			Con_Printf (CON_ERROR "Mod_LoadLeafs: %s has more than %i leafs\n",loadmodel->name, MAX_MAP_LEAFS);
			return false;
		}
		out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

		loadmodel->leafs = out;
		loadmodel->numleafs = count;
		loadmodel->numclusters = count-1;

		for ( i=0 ; i<count ; i++, in++, out++)
		{
			for (j=0 ; j<3 ; j++)
			{
				out->minmaxs[j] = LittleFloat (in->mins[j]);
				out->minmaxs[3+j] = LittleFloat (in->maxs[j]);
			}

			p = LittleLong(in->contents);
			out->contents = p;

			out->firstmarksurface = loadmodel->marksurfaces +
				LittleLong(in->firstmarksurface);
			out->nummarksurfaces = LittleLong(in->nummarksurfaces);
			
			p = LittleLong(in->visofs);
			if (p == -1)
				out->compressed_vis = NULL;
			else
				out->compressed_vis = loadmodel->visdata + p;
			
			for (j=0 ; j<4 ; j++)
				out->ambient_sound_level[j] = in->ambient_level[j];

	#ifndef CLIENTONLY
			if (!isDedicated)
	#endif
			{
				// gl underwater warp
				if (out->contents != Q1CONTENTS_EMPTY)
				{
					for (j=0 ; j<out->nummarksurfaces ; j++)
						out->firstmarksurface[j]->flags |= SURF_UNDERWATER;
				}
				if (isnotmap)
				{
					for (j=0 ; j<out->nummarksurfaces ; j++)
						out->firstmarksurface[j]->flags |= SURF_DONTWARP;
				}
			}
		}
	}
	else if (lm)
	{
		dl1leaf_t 	*in;
		in = (void *)(ptr);
		if (len % sizeof(*in))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		count = len / sizeof(*in);
		if (count > MAX_MAP_LEAFS)
		{
			Con_Printf (CON_ERROR "Mod_LoadLeafs: %s has more than %i leafs\n",loadmodel->name, MAX_MAP_LEAFS);
			return false;
		}
		out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

		loadmodel->leafs = out;
		loadmodel->numleafs = count;
		loadmodel->numclusters = count-1;

		for ( i=0 ; i<count ; i++, in++, out++)
		{
			for (j=0 ; j<3 ; j++)
			{
				out->minmaxs[j] = LittleShort (in->mins[j]);
				out->minmaxs[3+j] = LittleShort (in->maxs[j]);
			}

			p = LittleLong(in->contents);
			out->contents = p;

			out->firstmarksurface = loadmodel->marksurfaces +
				LittleLong(in->firstmarksurface);
			out->nummarksurfaces = LittleLong(in->nummarksurfaces);
			
			p = LittleLong(in->visofs);
			if (p == -1)
				out->compressed_vis = NULL;
			else
				out->compressed_vis = loadmodel->visdata + p;
			
			for (j=0 ; j<4 ; j++)
				out->ambient_sound_level[j] = in->ambient_level[j];

	#ifndef CLIENTONLY
			if (!isDedicated)
	#endif
			{
				// gl underwater warp
				if (out->contents != Q1CONTENTS_EMPTY)
				{
					for (j=0 ; j<out->nummarksurfaces ; j++)
						out->firstmarksurface[j]->flags |= SURF_UNDERWATER;
				}
				if (isnotmap)
				{
					for (j=0 ; j<out->nummarksurfaces ; j++)
						out->firstmarksurface[j]->flags |= SURF_DONTWARP;
				}
			}
		}
	}
	else
	{
		dsleaf_t 	*in;
		in = (void *)(ptr);
		if (len % sizeof(*in))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		count = len / sizeof(*in);
		if (count > MAX_MAP_LEAFS)
		{
			Con_Printf (CON_ERROR "Mod_LoadLeafs: %s has more than %i leafs\n",loadmodel->name, MAX_MAP_LEAFS);
			return false;
		}
		out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

		loadmodel->leafs = out;
		loadmodel->numleafs = count;
		loadmodel->numclusters = count-1;

		for ( i=0 ; i<count ; i++, in++, out++)
		{
			for (j=0 ; j<3 ; j++)
			{
				out->minmaxs[j] = LittleShort (in->mins[j]);
				out->minmaxs[3+j] = LittleShort (in->maxs[j]);
			}

			p = LittleLong(in->contents);
			out->contents = p;

			out->firstmarksurface = loadmodel->marksurfaces + (unsigned short)LittleShort(in->firstmarksurface);
			out->nummarksurfaces = (unsigned short)LittleShort(in->nummarksurfaces);
			
			p = LittleLong(in->visofs);
			if (p == -1)
				out->compressed_vis = NULL;
			else
				out->compressed_vis = loadmodel->visdata + p;
			
			for (j=0 ; j<4 ; j++)
				out->ambient_sound_level[j] = in->ambient_level[j];

	#ifndef CLIENTONLY
			if (!isDedicated)
	#endif
			{
				// gl underwater warp
				if (out->contents != Q1CONTENTS_EMPTY)
				{
					for (j=0 ; j<out->nummarksurfaces ; j++)
						out->firstmarksurface[j]->flags |= SURF_UNDERWATER;
				}
				if (isnotmap)
				{
					for (j=0 ; j<out->nummarksurfaces ; j++)
						out->firstmarksurface[j]->flags |= SURF_DONTWARP;
				}
			}
		}
	}

	return true;
}




//these are used to boost other info sizes
int numsuplementryplanes;
int numsuplementryclipnodes;
void *suplementryclipnodes;
void *suplementryplanes;
void *crouchhullfile;

void Mod_LoadCrouchHull(model_t *loadmodel)
{
	int i, h;
	int numsm;
	char crouchhullname[MAX_QPATH];
	int *data;
	int hulls;

//	dclipnode_t *cn;

	memset(loadmodel->hulls, 0, sizeof(loadmodel->hulls));	//ensure all the sizes are 0 (this is how we check for the existance of a hull

	numsuplementryplanes = numsuplementryclipnodes = 0;

	//find a name for a ccn and try to load it.
	strcpy(crouchhullname, loadmodel->name);
	COM_StripExtension(loadmodel->name, crouchhullname, sizeof(crouchhullname));
	COM_DefaultExtension(crouchhullname, ".crh",sizeof(crouchhullname));	//crouch hull

	FS_LoadFile(crouchhullname, &crouchhullfile);
	if (!crouchhullfile)
		return;

	data = crouchhullfile;

	if (LittleLong(*data++) != ('S') + ('C'<<8) + ('N'<<16) + ('P'<<24))	//make sure it's the right version
		return;

	if (LittleLong(*data) == 2)
	{
		data++;
		hulls = LittleLong(*data++);
	}
	else
		return;

	if (hulls > MAX_MAP_HULLSM - MAX_MAP_HULLSDQ1)
	{
		return;
	}

	numsm = LittleLong(*data++);
	if (numsm != loadmodel->numsubmodels)	//not compatible
		return;

	numsuplementryplanes = LittleLong(*data++);
	numsuplementryclipnodes = LittleLong(*data++);

	for (h = 0; h < hulls; h++)
	{
		for (i = 0; i < 3; i++)
			loadmodel->hulls[3+h].clip_mins[i] = LittleLong(*data++);
		for (i = 0; i < 3; i++)
			loadmodel->hulls[3+h].clip_maxs[i] = LittleLong(*data++);

		for (i = 0; i < numsm; i++)	//load headnode references
		{
			loadmodel->submodels[i].headnode[3+h] = LittleLong(*data)+1;
			data++;
		}
	}

	suplementryplanes = data;
	suplementryclipnodes = (qbyte*)data + sizeof(dplane_t)*numsuplementryplanes;
}

/*
=================
Mod_LoadClipnodes
=================
*/
qboolean Mod_LoadClipnodes (model_t *loadmodel, qbyte *mod_base, lump_t *l, qboolean lm)
{
	dsclipnode_t *ins;
	dlclipnode_t *inl;
	mclipnode_t *out;
	int			i, count;
	hull_t		*hull;

	if (lm)
	{
		ins = NULL;
		inl = (void *)(mod_base + l->fileofs);
		if (l->filelen % sizeof(*inl))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		count = l->filelen / sizeof(*inl);
	}
	else
	{
		ins = (void *)(mod_base + l->fileofs);
		inl = NULL;
		if (l->filelen % sizeof(*ins))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		count = l->filelen / sizeof(*ins);
	}
	out = ZG_Malloc(&loadmodel->memgroup, (count+numsuplementryclipnodes)*sizeof(*out));//space for both

	loadmodel->clipnodes = out;
	loadmodel->numclipnodes = count+numsuplementryclipnodes;


	if (hexen2map)
	{	//hexen2.
		hexen2map=false;
		hull = &loadmodel->hulls[1];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -16;
		hull->clip_mins[1] = -16;
		hull->clip_mins[2] = -24;
		hull->clip_maxs[0] = 16;
		hull->clip_maxs[1] = 16;
		hull->clip_maxs[2] = 32;
		hull->available = true;

		hull = &loadmodel->hulls[2];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -24;
		hull->clip_mins[1] = -24;
		hull->clip_mins[2] = -20;
		hull->clip_maxs[0] = 24;
		hull->clip_maxs[1] = 24;
		hull->clip_maxs[2] = 20;
		hull->available = true;

		hull = &loadmodel->hulls[3];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -16;
		hull->clip_mins[1] = -16;
		hull->clip_mins[2] = -12;
		hull->clip_maxs[0] = 16;
		hull->clip_maxs[1] = 16;
		hull->clip_maxs[2] = 16;
		hull->available = true;

		/*
		There is some mission-pack weirdness here
		in the missionpack, hull 4 is meant to be '-8 -8 -8' '8 8 8'
		in the original game, hull 4 is '-40 -40 -42' '40 40 42'
		*/
		hull = &loadmodel->hulls[4];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -8;
		hull->clip_mins[1] = -8;
		hull->clip_mins[2] = -8;
		hull->clip_maxs[0] = 8;
		hull->clip_maxs[1] = 8;
		hull->clip_maxs[2] = 8;
		hull->available = true;

		hull = &loadmodel->hulls[5];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -48;
		hull->clip_mins[1] = -48;
		hull->clip_mins[2] = -50;
		hull->clip_maxs[0] = 48;
		hull->clip_maxs[1] = 48;
		hull->clip_maxs[2] = 50;
		hull->available = true;

		//6 isn't used.
		//7 isn't used.
	}
	else if (loadmodel->fromgame == fg_halflife)
	{
		hull = &loadmodel->hulls[1];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -16;
		hull->clip_mins[1] = -16;
		hull->clip_mins[2] = -36;//-36 is correct here, but mvdsv uses -32 instead. This breaks prediction between the two
		hull->clip_maxs[0] = 16;
		hull->clip_maxs[1] = 16;
		hull->clip_maxs[2] = hull->clip_mins[2]+72;
		hull->available = true;

		hull = &loadmodel->hulls[2];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -32;
		hull->clip_mins[1] = -32;
		hull->clip_mins[2] = -32;
		hull->clip_maxs[0] = 32;
		hull->clip_maxs[1] = 32;
		hull->clip_maxs[2] = hull->clip_mins[2]+64;
		hull->available = true;

		hull = &loadmodel->hulls[3];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -16;
		hull->clip_mins[1] = -16;
		hull->clip_mins[2] = -18;
		hull->clip_maxs[0] = 16;
		hull->clip_maxs[1] = 16;
		hull->clip_maxs[2] = hull->clip_mins[2]+36;
		hull->available = true;
	}
	else
	{
		hull = &loadmodel->hulls[1];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -16;
		hull->clip_mins[1] = -16;
		hull->clip_mins[2] = -24;
		hull->clip_maxs[0] = 16;
		hull->clip_maxs[1] = 16;
		hull->clip_maxs[2] = 32;
		hull->available = true;

		hull = &loadmodel->hulls[2];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -32;
		hull->clip_mins[1] = -32;
		hull->clip_mins[2] = -24;
		hull->clip_maxs[0] = 32;
		hull->clip_maxs[1] = 32;
		hull->clip_maxs[2] = 64;
		hull->available = true;

		hull = &loadmodel->hulls[3];
		hull->clipnodes = out;
		hull->firstclipnode = 0;
		hull->lastclipnode = count-1;
		hull->planes = loadmodel->planes;
		hull->clip_mins[0] = -16;
		hull->clip_mins[1] = -16;
		hull->clip_mins[2] = -6;
		hull->clip_maxs[0] = 16;
		hull->clip_maxs[1] = 16;
		hull->clip_maxs[2] = 30;
		hull->available = false;
	}

	if (lm)
	{
		for (i=0 ; i<count ; i++, out++, inl++)
		{
			out->planenum = LittleLong(inl->planenum);
			out->children[0] = LittleLong(inl->children[0]);
			out->children[1] = LittleLong(inl->children[1]);
		}
	}
	else
	{
		for (i=0 ; i<count ; i++, out++, ins++)
		{
			out->planenum = LittleLong(ins->planenum);
			out->children[0] = (unsigned short)LittleShort(ins->children[0]);
			out->children[1] = (unsigned short)LittleShort(ins->children[1]);

			//if these 'overflow', then they're meant to refer to contents instead, and should be negative
			if (out->children[0] >= count)
				out->children[0] -= 0x10000;
			if (out->children[1] >= count)
				out->children[1] -= 0x10000;
		}
	}

	if (numsuplementryclipnodes)	//now load the crouch ones.
	{
/*This looks buggy*/
		for (i = 3; i < MAX_MAP_HULLSM; i++)
		{
			hull = &loadmodel->hulls[i];
			hull->planes = suplementryplanes;
			hull->clipnodes = out-1;
			hull->firstclipnode = 0;
			hull->lastclipnode = numsuplementryclipnodes;
			hull->available = true;
		}

		ins = suplementryclipnodes;

		for (i=0 ; i<numsuplementryclipnodes ; i++, out++, ins++)
		{
			out->planenum = LittleLong(ins->planenum);
			out->children[0] = LittleShort(ins->children[0]);
			out->children[0] += out->children[0]>=0?1:0;
			out->children[1] = LittleShort(ins->children[1]);
			out->children[1] += out->children[1]>=0?1:0;
		}
	}

	return true;
}

/*
=================
Mod_MakeHull0

Deplicate the drawing hull structure as a clipping hull
=================
*/
void Mod_MakeHull0 (model_t *loadmodel)
{
	mnode_t		*in, *child;
	mclipnode_t *out;
	int			i, j, count;
	hull_t		*hull;

	hull = &loadmodel->hulls[0];	

	in = loadmodel->nodes;
	count = loadmodel->numnodes;
	out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

	hull->clipnodes = out;
	hull->firstclipnode = 0;
	hull->lastclipnode = count-1;
	hull->planes = loadmodel->planes;

	for (i=0 ; i<count ; i++, out++, in++)
	{
		out->planenum = in->plane - loadmodel->planes;
		for (j=0 ; j<2 ; j++)
		{
			child = in->children[j];
			if (child->contents < 0)
				out->children[j] = child->contents;
			else
				out->children[j] = child - loadmodel->nodes;
		}
	}
}

/*
=================
Mod_LoadMarksurfaces
=================
*/
qboolean Mod_LoadMarksurfaces (model_t *loadmodel, qbyte *mod_base, lump_t *l, qboolean lm)
{	
	int		i, j, count;
	msurface_t **out;

	if (lm)
	{
		int		*inl;
		inl = (void *)(mod_base + l->fileofs);
		if (l->filelen % sizeof(*inl))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		count = l->filelen / sizeof(*inl);
		out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

		loadmodel->marksurfaces = out;
		loadmodel->nummarksurfaces = count;

		for ( i=0 ; i<count ; i++)
		{
			j = (unsigned int)LittleLong(inl[i]);
			if (j >= loadmodel->numsurfaces)
			{
				Con_Printf (CON_ERROR "Mod_ParseMarksurfaces: bad surface number\n");
				return false;
			}
			out[i] = loadmodel->surfaces + j;
		}
	}
	else
	{
		short		*ins;
		ins = (void *)(mod_base + l->fileofs);
		if (l->filelen % sizeof(*ins))
		{
			Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
			return false;
		}
		count = l->filelen / sizeof(*ins);
		out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

		loadmodel->marksurfaces = out;
		loadmodel->nummarksurfaces = count;

		for ( i=0 ; i<count ; i++)
		{
			j = (unsigned short)LittleShort(ins[i]);
			if (j >= loadmodel->numsurfaces)
			{
				Con_Printf (CON_ERROR "Mod_ParseMarksurfaces: bad surface number\n");
				return false;
			}
			out[i] = loadmodel->surfaces + j;
		}
	}

	return true;
}

/*
=================
Mod_LoadSurfedges
=================
*/
qboolean Mod_LoadSurfedges (model_t *loadmodel, qbyte *mod_base, lump_t *l)
{	
	int		i, count;
	int		*in, *out;
	
	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	out = ZG_Malloc(&loadmodel->memgroup, count*sizeof(*out));

	loadmodel->surfedges = out;
	loadmodel->numsurfedges = count;

	for ( i=0 ; i<count ; i++)
		out[i] = LittleLong (in[i]);

	return true;
}


/*
=================
Mod_LoadPlanes
=================
*/
qboolean Mod_LoadPlanes (model_t *loadmodel, qbyte *mod_base, lump_t *l)
{
	int			i, j;
	mplane_t	*out;
	dplane_t 	*in;
	int			count;
	int			bits;
	
	in = (void *)(mod_base + l->fileofs);
	if (l->filelen % sizeof(*in))
	{
		Con_Printf (CON_ERROR "MOD_LoadBmodel: funny lump size in %s\n",loadmodel->name);
		return false;
	}
	count = l->filelen / sizeof(*in);
	out = ZG_Malloc(&loadmodel->memgroup, (count+numsuplementryplanes)*2*sizeof(*out));
	
	loadmodel->planes = out;
	loadmodel->numplanes = count+numsuplementryplanes;

	for ( i=0 ; i<count ; i++, in++, out++)
	{
		bits = 0;
		for (j=0 ; j<3 ; j++)
		{
			out->normal[j] = LittleFloat (in->normal[j]);
			if (out->normal[j] < 0)
				bits |= 1<<j;
		}

		out->dist = LittleFloat (in->dist);
		out->type = LittleLong (in->type);
		out->signbits = bits;
	}

	if (numsuplementryplanes)
	{
		in = suplementryplanes;
		suplementryplanes = out;
		for ( i=0 ; i<numsuplementryplanes ; i++, in++, out++)
		{
			bits = 0;
			for (j=0 ; j<3 ; j++)
			{
				out->normal[j] = LittleFloat (in->normal[j]);
				if (out->normal[j] < 0)
					bits |= 1<<j;
			}

			out->dist = LittleFloat (in->dist);
			out->type = LittleLong (in->type);
			out->signbits = bits;
		}
	}

	return true;
}

#ifndef SERVERONLY
//combination of R_AddDynamicLights and R_MarkLights
static void Q1BSP_StainNode (mnode_t *node, float *parms)
{
	mplane_t	*splitplane;
	float		dist;
	msurface_t	*surf;
	int			i;
	
	if (node->contents < 0)
		return;	

	splitplane = node->plane;
	dist = DotProduct ((parms+1), splitplane->normal) - splitplane->dist;
	
	if (dist > (*parms))
	{
		Q1BSP_StainNode (node->children[0], parms);
		return;
	}
	if (dist < (-*parms))
	{
		Q1BSP_StainNode (node->children[1], parms);
		return;
	}

// mark the polygons
	surf = cl.worldmodel->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		if (surf->flags&~(SURF_DRAWALPHA|SURF_DONTWARP|SURF_PLANEBACK))
			continue;
		Surf_StainSurf(surf, parms);
	}

	Q1BSP_StainNode (node->children[0], parms);
	Q1BSP_StainNode (node->children[1], parms);
}
#endif

void Mod_FixupNodeMinsMaxs (mnode_t *node, mnode_t *parent)
{
	if (!node)
		return;

	if (node->contents >= 0)
	{
		Mod_FixupNodeMinsMaxs (node->children[0], node);
		Mod_FixupNodeMinsMaxs (node->children[1], node);
	}

	if (parent)
	{
		if (parent->minmaxs[0] > node->minmaxs[0])
			parent->minmaxs[0] = node->minmaxs[0];
		if (parent->minmaxs[1] > node->minmaxs[1])
			parent->minmaxs[1] = node->minmaxs[1];
		if (parent->minmaxs[2] > node->minmaxs[2])
			parent->minmaxs[2] = node->minmaxs[2];

		if (parent->minmaxs[3] < node->minmaxs[3])
			parent->minmaxs[3] = node->minmaxs[3];
		if (parent->minmaxs[4] < node->minmaxs[4])
			parent->minmaxs[4] = node->minmaxs[4];
		if (parent->minmaxs[5] < node->minmaxs[5])
			parent->minmaxs[5] = node->minmaxs[5];
	}

}

static void Mod_FixupMinsMaxs(model_t *loadmodel)
{
	//q1 bsps are capped to +/- 32767 by the nodes/leafs
	//verts arn't though
	//so if the map is too big, let's figure out what they should be
	float *v;
	msurface_t **mark, *surf;
	mleaf_t *pleaf;
	medge_t *e, *pedges;
	int en, lindex;
	int i, c, lnumverts;
	qboolean needsfixup = false;

	if (loadmodel->mins[0] < -32768)
		needsfixup = true;
	if (loadmodel->mins[1] < -32768)
		needsfixup = true;
	if (loadmodel->mins[2] < -32768)
		needsfixup = true;

	if (loadmodel->maxs[0] > 32767)
		needsfixup = true;
	if (loadmodel->maxs[1] > 32767)
		needsfixup = true;
	if (loadmodel->maxs[2] > 32767)
		needsfixup = true;

	if (!needsfixup)
		return;

	//this is insane.
	//why am I writing this?
	//by the time the world actually gets this large, the floating point errors are going to be so immensly crazy that it's just not worth it.

	pedges = loadmodel->edges;

	for (i = 0; i < loadmodel->numleafs; i++)
	{
		pleaf = &loadmodel->leafs[i];

		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		if (c)
		{
			do
			{
				surf = (*mark++);

				lnumverts = surf->numedges;
				for (en=0 ; en<lnumverts ; en++)
				{
					lindex = loadmodel->surfedges[surf->firstedge + en];

					if (lindex > 0)
					{
						e = &pedges[lindex];
						v = loadmodel->vertexes[e->v[0]].position;
					}
					else
					{
						e = &pedges[-lindex];
						v = loadmodel->vertexes[e->v[1]].position;
					}

					if (pleaf->minmaxs[0] > v[0])
						pleaf->minmaxs[0] = v[0];
					if (pleaf->minmaxs[1] > v[1])
						pleaf->minmaxs[1] = v[1];
					if (pleaf->minmaxs[2] > v[2])
						pleaf->minmaxs[2] = v[2];

					if (pleaf->minmaxs[3] < v[0])
						pleaf->minmaxs[3] = v[0];
					if (pleaf->minmaxs[4] < v[1])
						pleaf->minmaxs[4] = v[1];
					if (pleaf->minmaxs[5] < v[2])
						pleaf->minmaxs[5] = v[2];

				}
			} while (--c);
		}
	}
	Mod_FixupNodeMinsMaxs (loadmodel->nodes, NULL);	// sets nodes and leafs
}

void ModBrush_LoadGLStuff(void *ctx, void *data, size_t a, size_t b)
{
#ifndef SERVERONLY
	model_t *mod = ctx;

	if (!a)
	{	//submodels share textures, so only do this if 'a' is 0 (inline index, 0 = world).
		for (a = 0; a < mod->numfogs; a++)
		{
			mod->fogs[a].shader = R_RegisterShader_Lightmap(mod->fogs[a].shadername);
			R_BuildDefaultTexnums(NULL, mod->fogs[a].shader);
			if (!mod->fogs[a].shader->fog_dist)
			{
				//invalid fog shader, don't use.
				mod->fogs[a].shader = NULL;
				mod->fogs[a].numplanes = 0;
			}
		}

		if (mod->fromgame == fg_quake3)
		{
			for(a = 0; a < mod->numtexinfo; a++)
			{
				mod->textures[a]->shader = R_RegisterShader_Lightmap(mod->textures[a]->name);
				R_BuildDefaultTexnums(NULL, mod->textures[a]->shader);

				mod->textures[a+mod->numtexinfo]->shader = R_RegisterShader_Vertex (mod->textures[a+mod->numtexinfo]->name);
				R_BuildDefaultTexnums(NULL, mod->textures[a+mod->numtexinfo]->shader);
			}
			mod->textures[2*mod->numtexinfo]->shader = R_RegisterShader_Flare("noshader");
		}
		else if (mod->fromgame == fg_quake2)
		{
			for(a = 0; a < mod->numtextures; a++)
			{
				mod->textures[a]->shader = R_RegisterCustom (mod->textures[a]->name, SUF_LIGHTMAP, Shader_DefaultBSPQ2, NULL);
				R_BuildDefaultTexnums(NULL, mod->textures[a]->shader);
			}
		}
		else
		{
			char loadname[MAX_QPATH];
			COM_FileBase (mod->name, loadname, sizeof(loadname));
			if (!strncmp(loadname, "b_", 2))
				Q_strncpyz(loadname, "bmodels", sizeof(loadname));
			for(a = 0; a < mod->numtextures; a++)
				Mod_FinishTexture(mod->textures[a], loadname);
		}
	}
	Mod_Batches_Build(mod, data);
	if (data)
		BZ_Free(data);
#endif
}

struct vispatch_s
{
	void *fileptr;
	size_t filelen;

	void *visptr;
	int vislen;

	void *leafptr;
	int leaflen;
};

static void Mod_FindVisPatch(struct vispatch_s *patch, model_t *mod, size_t leaflumpsize)
{
	char patchname[MAX_QPATH];
	int *lenptr, len;
	int ofs;
	qbyte *file;
	memset(patch, 0, sizeof(*patch));

	if (!mod_external_vis.ival)
		return;

	COM_StripExtension(mod->name, patchname, sizeof(patchname));
	Q_strncatz(patchname, ".vis", sizeof(patchname));

	//ignore the patch file if its in a different gamedir.
	//this file format sucks too much for other verification.
	if (FS_FLocateFile(mod->name,FSLFRT_DEPTH_OSONLY, NULL) != FS_FLocateFile(patchname,FSLFRT_DEPTH_OSONLY, NULL))
		return;

	patch->filelen = FS_LoadFile(patchname, &patch->fileptr);
	if (!patch->fileptr)
		return;
	ofs = 0;
	while (ofs+36 <= patch->filelen)
	{
		file = patch->fileptr;
		file += ofs;
		memcpy(patchname, file, 32);
		patchname[32] = 0;
		file += 32;
		lenptr = (int*)file;
		file += sizeof(int);
		len = LittleLong(*lenptr);
		if (ofs+36+len > patch->filelen)
			break;

//		if (!Q_strcasecmp(patchname, "foo"))
		{
			lenptr = (int*)file;
			patch->vislen = LittleLong(*lenptr);
			file += sizeof(int);
			patch->visptr = file;
			file += patch->vislen;

			lenptr = (int*)file;
			patch->leaflen = LittleLong(*lenptr);
			file += sizeof(int);
			patch->leafptr = file;
			file += patch->leaflen;

			if (sizeof(int)*2 + patch->vislen + patch->leaflen != len || patch->leaflen != leaflumpsize)
			{
				Con_Printf("Vis patch is unsuitable\n");
				patch->visptr = NULL;
				patch->leafptr = NULL;
			}
		}
		ofs += 36+len;
	}
}

/*
=================
Mod_LoadBrushModel
=================
*/
qboolean QDECL Mod_LoadBrushModel (model_t *mod, void *buffer, size_t fsize)
{
	struct vispatch_s vispatch;
	int			i, j;
	dheader_t	*header;
	mmodel_t 	*bm;
	model_t *submod;
	unsigned int chksum;
	qboolean noerrors;
	int longm = false;
	char loadname[32];
	qbyte *mod_base = buffer;
#if (defined(ODE_STATIC) || defined(ODE_DYNAMIC))
	qboolean ode = true;
#else
#define ode true
#endif

	COM_FileBase (mod->name, loadname, sizeof(loadname));
	mod->type = mod_brush;
	
	header = (dheader_t *)buffer;

#ifdef SERVERONLY
	isnotmap = !!sv.world.worldmodel;
#else
	if ((!cl.worldmodel && cls.state>=ca_connected)
#ifndef CLIENTONLY
		|| (!sv.world.worldmodel && sv.state)
#endif
		)
		isnotmap = false;
	else
		isnotmap = true;
#endif

	i = LittleLong (header->version);

	if (i == BSPVERSION || i == BSPVERSIONPREREL)
	{
		mod->fromgame = fg_quake;
		mod->engineflags |= MDLF_NEEDOVERBRIGHT;
	}
	else if (i == BSPVERSION_LONG1)
	{
		longm = true;
		mod->fromgame = fg_quake;
		mod->engineflags |= MDLF_NEEDOVERBRIGHT;
	}
	else if (i == BSPVERSION_LONG2)
	{
		longm = 2;
		mod->fromgame = fg_quake;
		mod->engineflags |= MDLF_NEEDOVERBRIGHT;
	}
	else if (i == BSPVERSIONHL)	//halflife support
		mod->fromgame = fg_halflife;
	else
	{
		Con_Printf (CON_ERROR "Mod_LoadBrushModel: %s has wrong version number (%i should be %i)\n", mod->name, i, BSPVERSION);
		return false;
	}

	mod->lightmaps.width = 128;//LMBLOCK_WIDTH;
	mod->lightmaps.height = 128;//LMBLOCK_HEIGHT; 

// swap all the lumps
	mod_base = (qbyte *)header;

	for (i=0 ; i<sizeof(dheader_t)/4 ; i++)
		((int *)header)[i] = LittleLong ( ((int *)header)[i]);

	Q1BSPX_Setup(mod, mod_base, fsize, header->lumps, HEADER_LUMPS);

// checksum all of the map, except for entities
	mod->checksum = 0;
	mod->checksum2 = 0;

	for (i = 0; i < HEADER_LUMPS; i++)
	{
		if ((unsigned)header->lumps[i].fileofs + (unsigned)header->lumps[i].filelen > fsize)
		{
			Con_Printf (CON_ERROR "Mod_LoadBrushModel: %s appears truncated\n", mod->name);
			return false;
		}
		if (i == LUMP_ENTITIES)
			continue;
		chksum = Com_BlockChecksum(mod_base + header->lumps[i].fileofs, header->lumps[i].filelen);
		mod->checksum ^= chksum;

		if (i == LUMP_VISIBILITY || i == LUMP_LEAFS || i == LUMP_NODES)
			continue;
		mod->checksum2 ^= chksum;
	}

	if (1)//mod_ebfs.value)
	{
		char *id;
		id = (char *)(header + 1);
		if (id[0]=='P' && id[1]=='A' && id[2]=='C' && id[3]=='K')
		{	//EBFS detected.
			COM_LoadMapPackFile(mod->name, sizeof(dheader_t));
		}
	}
		
	noerrors = true;

	crouchhullfile = NULL;

	Mod_FindVisPatch(&vispatch, mod, header->lumps[LUMP_LEAFS].filelen);

// load into heap
	if (!isDedicated || ode)
	{
		TRACE(("Loading verts\n"));
		noerrors = noerrors && Mod_LoadVertexes (mod, mod_base, &header->lumps[LUMP_VERTEXES]);
		TRACE(("Loading edges\n"));
		noerrors = noerrors && Mod_LoadEdges (mod, mod_base, &header->lumps[LUMP_EDGES], longm);
		TRACE(("Loading Surfedges\n"));
		noerrors = noerrors && Mod_LoadSurfedges (mod, mod_base, &header->lumps[LUMP_SURFEDGES]);
	}
	if (!isDedicated)
	{
		TRACE(("Loading Textures\n"));
		noerrors = noerrors && Mod_LoadTextures (mod, mod_base, &header->lumps[LUMP_TEXTURES]);
		TRACE(("Loading Lighting\n"));
		if (noerrors)
			Mod_LoadLighting (mod, mod_base, &header->lumps[LUMP_LIGHTING], false);
	}
	TRACE(("Loading Submodels\n"));
	noerrors = noerrors && Mod_LoadSubmodels (mod, mod_base, &header->lumps[LUMP_MODELS]);
	if (noerrors)
	{
		TRACE(("Loading CH\n"));
		Mod_LoadCrouchHull(mod);
	}
	TRACE(("Loading Planes\n"));
	noerrors = noerrors && Mod_LoadPlanes (mod, mod_base, &header->lumps[LUMP_PLANES]);
	TRACE(("Loading Entities\n"));
	Mod_LoadEntities (mod, mod_base, &header->lumps[LUMP_ENTITIES]);
	if (!isDedicated || ode)
	{
		TRACE(("Loading Texinfo\n"));
		noerrors = noerrors && Mod_LoadTexinfo (mod, mod_base, &header->lumps[LUMP_TEXINFO]);
		TRACE(("Loading Faces\n"));
		noerrors = noerrors && Mod_LoadFaces (mod, mod_base, &header->lumps[LUMP_FACES], longm);
	}
	if (!isDedicated)
	{
		TRACE(("Loading MarkSurfaces\n"));
		noerrors = noerrors && Mod_LoadMarksurfaces (mod, mod_base, &header->lumps[LUMP_MARKSURFACES], longm);	
	}
	if (noerrors)
	{
		TRACE(("Loading Vis\n"));
		Mod_LoadVisibility (mod, mod_base, &header->lumps[LUMP_VISIBILITY], vispatch.visptr, vispatch.vislen);
	}
	noerrors = noerrors && Mod_LoadLeafs (mod, mod_base, &header->lumps[LUMP_LEAFS], longm, vispatch.leafptr, vispatch.leaflen);
	TRACE(("Loading Nodes\n"));
	noerrors = noerrors && Mod_LoadNodes (mod, mod_base, &header->lumps[LUMP_NODES], longm);
	TRACE(("Loading Clipnodes\n"));
	noerrors = noerrors && Mod_LoadClipnodes (mod, mod_base, &header->lumps[LUMP_CLIPNODES], longm);
	if (noerrors)
	{
		TRACE(("Loading hull 0\n"));
		Mod_MakeHull0 (mod);
	}

	TRACE(("sorting shaders\n"));
	if (!isDedicated && noerrors)
		Mod_SortShaders(mod);

	if (crouchhullfile)
	{
		FS_FreeFile(crouchhullfile);
		crouchhullfile=NULL;
	}

	BZ_Free(vispatch.fileptr);

	if (!noerrors)
	{
		return false;
	}

	TRACE(("LoadBrushModel %i\n", __LINE__));
	Q1BSP_LoadBrushes(mod);
	TRACE(("LoadBrushModel %i\n", __LINE__));
	Q1BSP_SetModelFuncs(mod);
	TRACE(("LoadBrushModel %i\n", __LINE__));
#ifndef SERVERONLY
	mod->funcs.LightPointValues		= GLQ1BSP_LightPointValues;
	mod->funcs.MarkLights			= Q1BSP_MarkLights;
	mod->funcs.StainNode			= Q1BSP_StainNode;
#endif

	mod->numframes = 2;		// regular and alternate animation
	

//
// set up the submodels (FIXME: this is confusing)
//
	for (i=0, submod = mod; i<mod->numsubmodels ; i++)
	{
		bm = &mod->submodels[i];

		submod->rootnode = submod->nodes + bm->headnode[0];
		submod->hulls[0].firstclipnode = bm->headnode[0];
		submod->hulls[0].available = true;
		Q1BSP_CheckHullNodes(&submod->hulls[0]);

TRACE(("LoadBrushModel %i\n", __LINE__));
		for (j=1 ; j<MAX_MAP_HULLSM ; j++)
		{
			submod->hulls[j].firstclipnode = bm->headnode[j];
			submod->hulls[j].lastclipnode = submod->numclipnodes-1;

			submod->hulls[j].available &= bm->hullavailable[j];
			if (submod->hulls[j].firstclipnode > submod->hulls[j].lastclipnode)
				submod->hulls[j].available = false;

			if (submod->hulls[j].available)
				Q1BSP_CheckHullNodes(&submod->hulls[j]);
		}
		
		submod->firstmodelsurface = bm->firstface;
		submod->nummodelsurfaces = bm->numfaces;
		
		VectorCopy (bm->maxs, submod->maxs);
		VectorCopy (bm->mins, submod->mins);

		submod->radius = RadiusFromBounds (submod->mins, submod->maxs);

		submod->numclusters = bm->visleafs;

		memset(&submod->batches, 0, sizeof(submod->batches));
		submod->vbos = NULL;
		TRACE(("LoadBrushModel %i\n", __LINE__));
		if (!isDedicated || ode)
		{
			COM_AddWork(0, ModBrush_LoadGLStuff, submod, NULL, i, 0);
		}
		TRACE(("LoadBrushModel %i\n", __LINE__));

		if (i)
			COM_AddWork(0, Mod_ModelLoaded, submod, NULL, MLS_LOADED, 0);
		if (i < submod->numsubmodels-1)
		{	// duplicate the basic information
			char	name[MAX_QPATH];
			model_t *nextmod;

//			if (isnotmap)
				Q_snprintfz (name, sizeof(name), "*%i:%s", i+1, mod->name);
//			else//FIXME: this can bug out if we've still got one of these queued from a previous map change
//				Q_snprintfz (name, sizeof(name), "*%i", i+1);
			nextmod = Mod_FindName (name);
			*nextmod = *submod;
			Q_strncpyz(nextmod->name, name, sizeof(nextmod->name));
			submod = nextmod;
			memset(&submod->memgroup, 0, sizeof(submod->memgroup));
		}
		TRACE(("LoadBrushModel %i\n", __LINE__));
	}
#ifdef RUNTIMELIGHTING
	TRACE(("LoadBrushModel %i\n", __LINE__));
	if (lightmodel == mod)
		LightLoadEntities(lightmodel->entities);
#endif
TRACE(("LoadBrushModel %i\n", __LINE__));
	if (!isDedicated)
		Mod_FixupMinsMaxs(mod);
TRACE(("LoadBrushModel %i\n", __LINE__));

#ifdef TERRAIN
	mod->terrain = Mod_LoadTerrainInfo(mod, loadname, false);
#endif
	return true;
}

/*
==============================================================================

SPRITES

==============================================================================
*/

//=========================================================

//we need to override the rtlight shader for sprites so they get lit properly ignoring n+s+t dirs
//so lets split the shader into parts to avoid too many dupes
#define SPRITE_SHADER_MAIN									\
			"{\n"											\
				"if gl_blendsprites\n"						\
					"program defaultsprite\n"				\
				"else\n"									\
					"program defaultsprite#MASK=1\n"		\
				"endif\n"									\
				"{\n"										\
					"map $diffuse\n"						\
					"if gl_blendsprites\n"					\
						"blendfunc GL_SRC_ALPHA GL_ONE\n"	\
					"else\n"								\
						"alphafunc ge128\n"					\
						"depthwrite\n"						\
					"endif\n"								\
					"rgbgen vertex\n"						\
					"alphagen vertex\n"						\
				"}\n"										\
				"surfaceparm noshadows\n"
#define SPRITE_SHADER_UNLIT	"surfaceparm nodlight\n"
#define SPRITE_SHADER_LIT								\
				"sort seethrough\n"						\
				"bemode rtlight\n"						\
				"{\n"									\
					"program rtlight#NOBUMP\n"			\
					"{\n"								\
						"map $diffuse\n"				\
						"blendfunc add\n"				\
					"}\n"								\
				"}\n"
#define SPRITE_SHADER_FOOTER "}\n"

void Mod_LoadSpriteFrameShader(model_t *spr, int frame, int subframe, mspriteframe_t *frameinfo)
{
#ifndef SERVERONLY
	/*
	A quick note on tenebrae and sprites: In tenebrae, sprites are always lit, unless the light_lev field is set (which makes it fullbright).
	While its generally preferable and more consistent to assume lit sprites, this is incompatible with vanilla quake and thus unacceptable to us, but you can set the mod_assumelitsprites cvar if you want it.
	So for better compatibility, we have a whitelist of 'well-known' sprites that tenebrae uses in this way, which we do lighting on.
	You should still be able to use EF_FULLBRIGHT on these, but light_lev is an imprecise setting and will result in issues. Just be specific about fullbright or additive.
	DP on the other hand, supports lit sprites only when the sprite contains a ! in its name. We support that too.
	*/
	char *forcelitsprites[] =
	{
		"progs/smokepuff.spr",
		NULL
	};
	int i;
	char *shadertext;
	char name[MAX_QPATH];

	if (qrenderer == QR_NONE)
		return;

	if (subframe == -1)
		Q_snprintfz(name, sizeof(name), "%s_%i.tga", spr->name, frame);
	else
		Q_snprintfz(name, sizeof(name), "%s_%i_%i.tga", spr->name, frame, subframe);

	if (mod_litsprites.ival || strchr(spr->name, '!'))
		i = -1;
	else
	{
		for (i = 0; forcelitsprites[i]; i++)
			if (!strcmp(spr->name, forcelitsprites[i]))
			{
				i = -1;
				break;
			}
	}

	if (i == -1)	// a ! in the filename makes it non-fullbright (and can also be lit by rtlights too).
		shadertext = SPRITE_SHADER_MAIN SPRITE_SHADER_LIT SPRITE_SHADER_FOOTER;
	else
		shadertext = SPRITE_SHADER_MAIN SPRITE_SHADER_UNLIT SPRITE_SHADER_FOOTER;
	frameinfo->shader = R_RegisterShader(name, SUF_NONE, shadertext);
	frameinfo->shader->defaulttextures.base = frameinfo->image;
	frameinfo->shader->width = frameinfo->right-frameinfo->left;
	frameinfo->shader->height = frameinfo->up-frameinfo->down;
#endif
}
void Mod_LoadSpriteShaders(model_t *spr)
{
	msprite_t *psprite = spr->meshinfo;
	int i, j;
	mspritegroup_t *group;

	for (i = 0; i < psprite->numframes; i++)
	{
		switch (psprite->frames[i].type)
		{
		case SPR_SINGLE:
			Mod_LoadSpriteFrameShader(spr, i, -1, psprite->frames[i].frameptr);
			break;
		case SPR_ANGLED:
		case SPR_GROUP:
			group = (mspritegroup_t *)psprite->frames[i].frameptr;
			for (j = 0; j < group->numframes; j++)
				Mod_LoadSpriteFrameShader(spr, i, j, group->frames[j]);
			break;
		}
	}
}

#ifdef SPRMODELS
/*
=================
Mod_LoadSpriteFrame
=================
*/
static void * Mod_LoadSpriteFrame (model_t *mod, void *pin, void *pend, mspriteframe_t **ppframe, int framenum, int version, unsigned char *palette)
{
	dspriteframe_t		*pinframe;
	mspriteframe_t		*pspriteframe;
	int					width, height, size, origin[2];
	char				name[64];
	uploadfmt_t			lowresfmt;
	void				*dataptr;

	pinframe = (dspriteframe_t *)pin;

	width = LittleLong (pinframe->width);
	height = LittleLong (pinframe->height);
	size = width * height;

	pspriteframe = ZG_Malloc(&mod->memgroup, sizeof (mspriteframe_t));

	Q_memset (pspriteframe, 0, sizeof (mspriteframe_t));

	*ppframe = pspriteframe;

	origin[0] = LittleLong (pinframe->origin[0]);
	origin[1] = LittleLong (pinframe->origin[1]);

	pspriteframe->up = origin[1];
	pspriteframe->down = origin[1] - height;
	pspriteframe->left = origin[0];
	pspriteframe->right = width + origin[0];

	dataptr = (pinframe + 1);

	if (version == SPRITE32_VERSION)
	{
		size *= 4;
		lowresfmt = TF_RGBA32;
	}
	else if (version == SPRITEHL_VERSION)
		lowresfmt = TF_8PAL32;
	else
		lowresfmt = TF_TRANS8;

	if ((qbyte*)dataptr + size > (qbyte*)pend)
	{
		//tenebrae has a couple of dodgy truncated sprites. yay for replacement textures.
		dataptr = NULL;
		lowresfmt = TF_INVALID;
	}

	Q_snprintfz(name, sizeof(name), "%s_%i.tga", mod->name, framenum);
	pspriteframe->image = Image_GetTexture(name, "sprites", IF_NOMIPMAP|IF_NOGAMMA|IF_CLAMP, dataptr, palette, width, height, lowresfmt);

	return (void *)((qbyte *)(pinframe+1) + size);
}


/*
=================
Mod_LoadSpriteGroup
=================
*/
static void * Mod_LoadSpriteGroup (model_t *mod, void * pin, void *pend, mspriteframe_t **ppframe, int framenum, int version, unsigned char *palette)
{
	dspritegroup_t		*pingroup;
	mspritegroup_t		*pspritegroup;
	int					i, numframes;
	dspriteinterval_t	*pin_intervals;
	float				*poutintervals;
	void				*ptemp;

	pingroup = (dspritegroup_t *)pin;

	numframes = LittleLong (pingroup->numframes);

	pspritegroup = ZG_Malloc(&mod->memgroup, sizeof (mspritegroup_t) + (numframes - 1) * sizeof (pspritegroup->frames[0]));

	pspritegroup->numframes = numframes;

	*ppframe = (mspriteframe_t *)pspritegroup;

	pin_intervals = (dspriteinterval_t *)(pingroup + 1);

	poutintervals = ZG_Malloc(&mod->memgroup, numframes * sizeof (float));

	pspritegroup->intervals = poutintervals;

	for (i=0 ; i<numframes ; i++)
	{
		*poutintervals = LittleFloat (pin_intervals->interval);
		if (*poutintervals <= 0.0)
		{
			Con_Printf (CON_ERROR "Mod_LoadSpriteGroup: interval<=0\n");
			return NULL;
		}

		poutintervals++;
		pin_intervals++;
	}

	ptemp = (void *)pin_intervals;

	for (i=0 ; i<numframes ; i++)
	{
		ptemp = Mod_LoadSpriteFrame (mod, ptemp, pend, &pspritegroup->frames[i], framenum * 100 + i, version, palette);
	}

	return ptemp;
}

/*
=================
Mod_LoadSpriteModel
=================
*/
qboolean QDECL Mod_LoadSpriteModel (model_t *mod, void *buffer, size_t fsize)
{
	int					i;
	int					version;
	dsprite_t			*pin;
	msprite_t			*psprite;
	int					numframes;
	int					size;
	dspriteframetype_t	*pframetype;
	int rendertype=SPRHL_ALPHATEST;
	unsigned char pal[256*4];
	int sptype;
	
	pin = (dsprite_t *)buffer;

	version = LittleLong (pin->version);
	if (version != SPRITE_VERSION)
	if (version != SPRITE32_VERSION)
	if (version != SPRITEHL_VERSION)
	{
		Con_Printf (CON_ERROR "%s has wrong version number "
				 "(%i should be %i)\n", mod->name, version, SPRITE_VERSION);
		return false;
	}

	sptype = LittleLong (pin->type);

	if (LittleLong(pin->version) == SPRITEHL_VERSION)
	{
		pin = (dsprite_t*)((char*)pin + 4);
		rendertype = LittleLong (pin->type);	//not sure what the values mean.
	}

	numframes = LittleLong (pin->numframes);

	size = sizeof (msprite_t) +	(numframes - 1) * sizeof (psprite->frames);

	psprite = ZG_Malloc(&mod->memgroup, size);

	mod->meshinfo = psprite;
	switch(sptype)
	{
	case SPR_VP_PARALLEL_UPRIGHT:
	case SPR_FACING_UPRIGHT:
	case SPR_VP_PARALLEL:
	case SPR_ORIENTED:
//	case SPR_VP_PARALLEL_ORIENTED:
//	case SPRDP_LABEL:
//	case SPRDP_LABEL_SCALE:
//	case SPRDP_OVERHEAD:
		break;
	default:
		Con_DPrintf(CON_ERROR "%s has unsupported sprite type %i\n", mod->name, sptype);
		sptype = SPR_VP_PARALLEL;
		break;
	}
	psprite->type = sptype;

	psprite->maxwidth = LittleLong (pin->width);
	psprite->maxheight = LittleLong (pin->height);
	psprite->beamlength = LittleFloat (pin->beamlength);
	mod->synctype = LittleLong (pin->synctype);
	psprite->numframes = numframes;

	mod->mins[0] = mod->mins[1] = -psprite->maxwidth/2;
	mod->maxs[0] = mod->maxs[1] = psprite->maxwidth/2;
	mod->mins[2] = -psprite->maxheight/2;
	mod->maxs[2] = psprite->maxheight/2;
	if (qrenderer == QR_NONE)
	{
		mod->type = mod_dummy;
		return true;
	}

	if (version == SPRITEHL_VERSION)
	{
		int i;
		short *numi = (short*)(pin+1);
		unsigned char *src = (unsigned char *)(numi+1);
		if (LittleShort(*numi) != 256)
		{
			Con_Printf(CON_ERROR "%s has wrong number of palette indexes (we only support 256)\n", mod->name);
			return false;
		}

		if (rendertype == SPRHL_INDEXALPHA)
		{
			Con_Printf(CON_ERROR "%s: SPRHL_INDEXALPHA sprites are not supported\n", mod->name);
			return false;
		}
		else
		{
			for (i = 0; i < 256; i++)
			{//FIXME: bgr?
				pal[i*4+0] = *src++;
				pal[i*4+1] = *src++;
				pal[i*4+2] = *src++;
				pal[i*4+3] = 255;
			}
			if (rendertype == SPRHL_ALPHATEST)
			{
				pal[255*4+0] = 0;
				pal[255*4+1] = 0;
				pal[255*4+2] = 0;
				pal[255*4+3] = 0;
			}
		}

		pframetype = (dspriteframetype_t *)(src);
	}
	else
		pframetype = (dspriteframetype_t *)(pin + 1);

//
// load the frames
//
	if (numframes < 1)
	{
		Con_Printf (CON_ERROR "Mod_LoadSpriteModel: Invalid # of frames: %d\n", numframes);
		return false;
	}

	mod->numframes = numframes;

	for (i=0 ; i<numframes ; i++)
	{
		spriteframetype_t	frametype;

		frametype = LittleLong (pframetype->type);
		psprite->frames[i].type = frametype;

		if (frametype == SPR_SINGLE)
		{
			pframetype = (dspriteframetype_t *)
					Mod_LoadSpriteFrame (mod, pframetype + 1, (qbyte*)buffer + fsize,
										 &psprite->frames[i].frameptr, i, version, pal);
		}
		else
		{
			pframetype = (dspriteframetype_t *)
					Mod_LoadSpriteGroup (mod, pframetype + 1, (qbyte*)buffer + fsize,
										 &psprite->frames[i].frameptr, i, version, pal);
		}
		if (pframetype == NULL)
		{
			return false;
		}
	}

	mod->type = mod_sprite;

	return true;
}
#endif

#ifdef SP2MODELS
qboolean QDECL Mod_LoadSprite2Model (model_t *mod, void *buffer, size_t fsize)
{
	int					i;
	int					version;
	dmd2sprite_t		*pin;
	msprite_t			*psprite;
	int					numframes;
	int					size;
	dmd2sprframe_t		*pframetype;
	mspriteframe_t		*frame;
	int w, h;
	float origin[2];

	
	pin = (dmd2sprite_t *)buffer;

	version = LittleLong (pin->version);
	if (version != SPRITE2_VERSION)
	{
		Con_Printf (CON_ERROR "%s has wrong version number "
				 "(%i should be %i)", mod->name, version, SPRITE2_VERSION);
		return false;
	}

	numframes = LittleLong (pin->numframes);

	size = sizeof (msprite_t) +	(numframes - 1) * sizeof (psprite->frames);

	psprite = ZG_Malloc(&mod->memgroup, size);

	mod->meshinfo = psprite;

	psprite->type = SPR_VP_PARALLEL;
	psprite->maxwidth = 1;
	psprite->maxheight = 1;
	psprite->beamlength = 1;
	mod->synctype = 0;
	psprite->numframes = numframes;

	mod->mins[0] = mod->mins[1] = -psprite->maxwidth/2;
	mod->maxs[0] = mod->maxs[1] = psprite->maxwidth/2;
	mod->mins[2] = -psprite->maxheight/2;
	mod->maxs[2] = psprite->maxheight/2;
	
//
// load the frames
//
	if (numframes < 1)
	{
		Con_Printf (CON_ERROR "Mod_LoadSpriteModel: Invalid # of frames: %d\n", numframes);
		return false;
	}

	mod->numframes = numframes;

	pframetype = pin->frames;

	for (i=0 ; i<numframes ; i++)
	{
		spriteframetype_t	frametype;

		frametype = SPR_SINGLE;
		psprite->frames[i].type = frametype;

		frame = psprite->frames[i].frameptr = ZG_Malloc(&mod->memgroup, sizeof(mspriteframe_t));

		frame->image = Image_GetTexture(pframetype->name, NULL, IF_NOMIPMAP|IF_NOGAMMA|IF_CLAMP, NULL, NULL, 0, 0, TF_INVALID);

		w = LittleLong(pframetype->width);
		h = LittleLong(pframetype->height);
		origin[0] = LittleLong (pframetype->origin_x);
		origin[1] = LittleLong (pframetype->origin_y);

		frame->down = -origin[1];
		frame->up = h - origin[1];
		frame->left = -origin[0];
		frame->right = w - origin[0];

		pframetype++;
	}

	mod->type = mod_sprite;

	return true;
}
#endif

#ifdef DSPMODELS

typedef struct {
	short width;
	short height;
	short xpos;
	short ypos;
} doomimage_t;
static int QDECL FindDoomSprites(const char *name, qofs_t size, void *param, searchpathfuncs_t *spath)
{
	if (*(int *)param + strlen(name)+1 > 16000)
		Sys_Error("Too many doom sprites\n");

	strcpy((char *)param + *(int *)param, name);
	*(int *)param += strlen(name)+1;

	return true;
}


static void LoadDoomSpriteFrame(model_t *mod, char *imagename, mspriteframedesc_t *pdesc, int anglenum, qboolean xmirrored)
{
	int c;
	int fr;
	int rc;
	unsigned int *colpointers;
	qbyte *data;
	doomimage_t *header;

	qbyte image[256*256];
	qbyte *palette;
	qbyte *coldata;
	mspriteframe_t *pframe;

	if (!anglenum)
	{
		pdesc->type = SPR_SINGLE;
		pdesc->frameptr = pframe = ZG_Malloc(&mod->memgroup, sizeof(*pframe));
	}
	else
	{
		mspritegroup_t *group;

		if (!pdesc->frameptr || pdesc->type != SPR_ANGLED)
		{
			pdesc->type = SPR_ANGLED;
			group = ZG_Malloc(&mod->memgroup, sizeof(*group)+sizeof(mspriteframe_t *)*(8-1));
			pdesc->frameptr = (mspriteframe_t *)group;
			group->numframes = 8;
		}
		else
			group = (mspritegroup_t *)pdesc->frameptr;

		pframe = ZG_Malloc(&mod->memgroup, sizeof(*pframe));
		group->frames[anglenum-1] = pframe;
	}

	palette = COM_LoadTempFile("wad/playpal");
	header = (doomimage_t *)COM_LoadTempMoreFile(imagename);
	data = (qbyte *)header;
	pframe->up = +header->ypos;
	pframe->down = -header->height + header->ypos;

	if (xmirrored)
	{
		pframe->right = -header->xpos;
		pframe->left = header->width - header->xpos;
	}
	else
	{
		pframe->left = -header->xpos;
		pframe->right = header->width - header->xpos;
	}

	if (header->width*header->height > sizeof(image))
		return;

	memset(image, 255, header->width*header->height);
	colpointers = (unsigned int*)(data+sizeof(doomimage_t));
	for (c = 0; c < header->width; c++)
	{
		if (colpointers[c] >= com_filesize)
			break;
		coldata = data + colpointers[c];
		while(1)
		{
			fr = *coldata++;
			if (fr == 255)
				break;

			rc = *coldata++;

			coldata++;

			if ((fr+rc) > header->height)
				break;

			while(rc)
			{
				image[c + fr*header->width] = *coldata++;
				fr++;
				rc--;
			}

			coldata++;
		}
	}

	pframe->shader = R_RegisterShader(imagename, SUF_NONE, 
		"{\n{\nmap $diffuse\nblendfunc blend\n}\n}\n");
	pframe->shader->defaulttextures.base = R_LoadTexture8Pal24(imagename, header->width, header->height, image, palette, IF_CLAMP);
	R_BuildDefaultTexnums(NULL, pframe->shader);
}

/*
=================
Doom Sprites
=================
*/
void Mod_LoadDoomSprite (model_t *mod)
{
	char files[16384];
	char basename[MAX_QPATH];
	int baselen;
	char *name;

	int numframes=0;
	int ofs;

	int size;

	int elements=0;

	int framenum;
	int anglenum;

	msprite_t *psprite;


	COM_StripExtension(mod->name, basename, sizeof(basename));
	baselen = strlen(basename);
	strcat(basename, "*");
	*(int *)files=4;
	COM_EnumerateFiles(basename, FindDoomSprites, files);

	//find maxframes and validate the rest.
	for (ofs = 4; ofs < *(int*)files; ofs+=strlen(files+ofs)+1)
	{
		name = files+ofs+baselen;

		if (!*name)
			Host_Error("Doom sprite componant lacks frame name");
		if (*name - 'a'+1 > numframes)
			numframes = *name - 'a'+1;
		if (name[1] < '0' || name[1] > '8')
			Host_Error("Doom sprite componant has bad angle number");
		if (name[1] == '0')
			elements+=8;
		else
			elements++;
		if (name[2])	//is there a second element?
		{
			if (name[2] - 'a'+1 > numframes)
				numframes = name[2] - 'a'+1;
			if (name[3] < '0' || name[3] > '8')
				Host_Error("Doom sprite componant has bad angle number");

			if (name[3] == '0')
				elements+=8;
			else
				elements++;
		}
	}
	if (elements != numframes*8)
		Host_Error("Doom sprite has wrong componant count");
	if (!numframes)
		Host_Error("Doom sprite componant has no frames");

	size = sizeof (msprite_t) +	(elements - 1) * sizeof (psprite->frames);
	psprite = ZG_Malloc(&mod->memgroup, size);

	psprite->numframes = numframes;

	//do the actual loading.
	for (ofs = 4; ofs < *(int*)files; ofs+=strlen(files+ofs)+1)
	{
		name = files+ofs;
		framenum = name[baselen+0] - 'a';
		anglenum = name[baselen+1] - '0';

		LoadDoomSpriteFrame(mod, name, &psprite->frames[framenum], anglenum, false);

		if (name[baselen+2])	//is there a second element?
		{
			framenum = name[baselen+2] - 'a';
			anglenum = name[baselen+3] - '0';

			LoadDoomSpriteFrame(mod, name, &psprite->frames[framenum], anglenum, true);
		}
	}


	psprite->type = SPR_FACING_UPRIGHT;
	mod->type = mod_sprite;

	mod->meshinfo = psprite;
}
#endif

//=============================================================================

/*
================
Mod_Print
================
*/
void Mod_Print (void)
{
	int		i;
	model_t	*mod;

	Con_Printf ("Cached models:\n");
	for (i=0, mod=mod_known ; i < mod_numknown ; i++, mod++)
	{
		Con_Printf ("%8p : %s\n", mod->meshinfo, mod->name);
	}
}


#endif
