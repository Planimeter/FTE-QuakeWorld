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

// refresh.h -- public interface to refresh functions

// default soldier colors
#define TOP_DEFAULT		1
#define BOTTOM_DEFAULT	6

#define	TOP_RANGE		(TOP_DEFAULT<<4)
#define	BOTTOM_RANGE	(BOTTOM_DEFAULT<<4)

extern int		r_framecount;

struct msurface_s;
struct batch_s;
struct model_s;
struct texnums_s;
struct texture_s;

static const texid_t r_nulltex = NULL;

//GLES2 requires GL_UNSIGNED_SHORT
//geforce4 only does shorts. gffx can do ints, but with a performance hit (like most things on that gpu)
//ati is generally more capable, but generally also has a smaller market share
//desktop-gl will generally cope with ints, but expect a performance hit from that (so we don't bother)
//dx10 can cope with ints, 
#if 1 || defined(MINIMAL) || defined(D3DQUAKE) || defined(ANDROID)
	#define sizeof_index_t 2
#endif
#if sizeof_index_t == 2
	#define GL_INDEX_TYPE GL_UNSIGNED_SHORT
	#define D3DFMT_QINDEX D3DFMT_INDEX16
	typedef unsigned short index_t;
	#define MAX_INDICIES 0xffff
#else
	#define GL_INDEX_TYPE GL_UNSIGNED_INT
	#define D3DFMT_QINDEX D3DFMT_INDEX32
	typedef unsigned int index_t;
	#define MAX_INDICIES 0xffffffff
#endif

//=============================================================================

//the eye doesn't see different colours in the same proportion.
//must add to slightly less than 1
#define NTSC_RED 0.299
#define NTSC_GREEN 0.587
#define NTSC_BLUE 0.114
#define NTSC_SUM (NTSC_RED + NTSC_GREEN + NTSC_BLUE)

typedef enum {
	RT_MODEL,
	RT_POLY,
	RT_SPRITE,
	RT_BEAM,
	RT_RAIL_CORE,
	RT_RAIL_RINGS,
	RT_LIGHTNING,
	RT_PORTALSURFACE,		// doesn't draw anything, just info for portals

	RT_MAX_REF_ENTITY_TYPE
} refEntityType_t;

typedef unsigned int skinid_t;	//skin 0 is 'unused'

struct dlight_s;
typedef struct entity_s
{
	//FIXME: instancing somehow. separate visentity+visinstance. only viable with full glsl though.
	//will need to generate a vbo somehow for the instances.

	int						keynum;			// for matching entities in different frames
	vec3_t					origin;
	vec3_t					angles;			// fixme: should be redundant.
	vec3_t					axis[3];

	vec4_t					shaderRGBAf; /*colormod+alpha, available for shaders to mix*/
	float					shaderTime;  /*timestamp, for syncing shader times to spawns*/
	vec3_t					glowmod;     /*meant to be a multiplier for the fullbrights*/

	int						light_known; /*bsp lighting has been calced*/
	vec3_t                  light_avg;   /*midpoint level*/
	vec3_t                  light_range; /*avg + this = max, avg - this = min*/
	vec3_t                  light_dir;

	vec3_t					oldorigin;	/*for q2/q3 beams*/

	struct model_s			*model;			// NULL = no model
	int						skinnum;		// for Alias models
	skinid_t				customskin;		// quake3 style skins

	int						playerindex;	//for qw skins
	int						topcolour;		//colourmapping
	int						bottomcolour;	//colourmapping
#ifdef HEXEN2
	int						h2playerclass;	//hexen2's quirky colourmapping
#endif

//	struct efrag_s			*efrag;			// linked list of efrags (FIXME)
//	int						visframe;		// last frame this entity was
											// found in an active leaf
											// only used for static objects
											
//	int						dlightframe;	// dynamic lighting
//	int						dlightbits;
	
// FIXME: could turn these into a union
//	int						trivial_accept;
//	struct mnode_s			*topnode;		// for bmodels, first world node
											//  that splits bmodel, or NULL if
											//  not split

	framestate_t			framestate;

	int flags;

	refEntityType_t rtype;
	float rotation;

	struct shader_s *forcedshader;

#ifdef PEXT_SCALE
	float scale;
#endif
#ifdef PEXT_FATNESS
	float fatness;
#endif
#ifdef PEXT_HEXEN2
	int drawflags;
	int abslight;
#endif
} entity_t;

#define MAX_GEOMSETS 32
#define Q1UNSPECIFIED 0x00ffffff	//0xffRRGGBB or 0x0000000V are both valid values. so this is an otherwise-illegal value to say its not been set.
typedef struct
{
	char skinname[MAX_QPATH];
	int nummappings;
	int maxmappings;
	qbyte geomset[MAX_GEOMSETS];	//allows selecting a single set of geometry from alternatives. this might be a can of worms.
	char qwskinname[MAX_QPATH];
	struct qwskin_s *qwskin;
	unsigned int q1upper;	//Q1UNSPECIFIED
	unsigned int q1lower;	//Q1UNSPECIFIED
	struct
	{
		char surface[MAX_QPATH];
		shader_t *shader;
		texnums_t texnums;
		int needsfree;	//which textures need to be freed.
	} mappings[1];
} skinfile_t;

// plane_t structure
typedef struct mplane_s
{
	vec3_t	normal;
	float	dist;
	qbyte	type;			// for texture axis selection and fast side tests
	qbyte	signbits;		// signx + signy<<1 + signz<<1
	qbyte	pad[2];
} mplane_t;
#define MAXFRUSTUMPLANES 7	//4 side, 1 near, 1 far (fog), 1 water plane.

typedef struct
{
	//note: uniforms expect specific padding/ordering. be really careful with reordering this
	vec3_t colour;		//w_fog[0].xyz
	float alpha;		//w_fog[0].w scales clamped fog value
	float density;		//w_fog[1].x egads, everyone has a different opinion.
	float depthbias;	//w_fog[1].y distance until the fog actually starts
	float glslpad1;		//w_fog[1].z
	float glslpad2;		//w_fog[1].w

//	float alpha;
//	float start;
//	float end;
//	float height;
//	float fadedepth;

	float time;	//timestamp for when its current.
} fogstate_t;
void CL_BlendFog(fogstate_t *result, fogstate_t *oldf, float time, fogstate_t *newf);
void CL_ResetFog(int fogtype);

typedef struct {
	char texname[MAX_QPATH];
} rtname_t;
#define R_MAX_RENDERTARGETS 8

#ifndef R_MAX_RECURSE
#define R_MAX_RECURSE	6
#endif
#define RDFD_FOV 1
typedef struct
{
	vrect_t		grect;				// game rectangle. fullscreen except for csqc/splitscreen/hud.
	vrect_t		vrect;				// subwindow in grect for 3d view. equal to grect if no hud.

	vec3_t		pvsorigin;			/*render the view using this point for pvs (useful for mirror views)*/
	vec3_t		vieworg;			/*logical view center*/
	vec3_t		viewangles;
	vec3_t		viewaxis[3];		/*forward, left, up (NOT RIGHT)*/

	float		fov_x, fov_y, afov;

	qboolean	drawsbar;
	qboolean	drawcrosshair;
	int			flags;	//(Q2)RDF_ flags
	int			dirty;

	playerview_t *playerview;
//	int			currentplayernum;

	float		time;
//	float		waterheight;	//updated by the renderer. stuff sitting at this height generate ripple effects

	float		m_projection[16];
	float		m_view[16];

	mplane_t	frustum[MAXFRUSTUMPLANES];
	int			frustum_numplanes;

	fogstate_t	globalfog;
	float		hdr_value;

	pxrect_t	pxrect;		/*vrect, but in pixels rather than virtual coords*/
	qboolean	externalview; /*draw external models and not viewmodels*/
	int			recurse;	/*in a mirror/portal/half way through drawing something else*/
	qboolean	forcevis;	/*if true, vis comes from the forcedvis field instead of recalculated*/
	unsigned int	flipcull;	/*reflected/flipped view, requires inverted culling (should be set to SHADER_CULL_FLIPPED or 0)*/
	qboolean	useperspective; /*not orthographic*/

	rtname_t	rt_destcolour[R_MAX_RENDERTARGETS];	/*used for 2d. written by 3d*/
	rtname_t	rt_sourcecolour;	/*read by 2d. not used for 3d. */
	rtname_t	rt_depth;			/*read by 2d. used by 3d (renderbuffer used if not set)*/
	rtname_t	rt_ripplemap;		/*read by 2d. used by 3d (internal ripplemap buffer used if not set)*/

	qbyte		*forcedvis;
	qboolean	areabitsknown;
	qbyte		areabits[MAX_MAP_AREA_BYTES];
} refdef_t;

extern	refdef_t	r_refdef;
extern vec3_t	r_origin, vpn, vright, vup;

extern	struct texture_s	*r_notexture_mip;

extern	entity_t	r_worldentity;

void BE_GenModelBatches(struct batch_s **batches, const struct dlight_s *dl, unsigned int bemode);	//if dl, filters based upon the dlight.

//gl_alias.c
void R_GAliasFlushSkinCache(qboolean final);
void R_GAlias_DrawBatch(struct batch_s *batch);
void R_GAlias_GenerateBatches(entity_t *e, struct batch_s **batches);
void R_LightArraysByte_BGR(const entity_t *entity, vecV_t *coords, byte_vec4_t *colours, int vertcount, vec3_t *normals);
void R_LightArrays(const entity_t *entity, vecV_t *coords, vec4_t *colours, int vertcount, vec3_t *normals, float scale);

void R_DrawSkyChain (struct batch_s *batch); /*called from the backend, and calls back into it*/
void R_InitSky (shader_t *shader, const char *skyname, qbyte *src, unsigned int width, unsigned int height);	/*generate q1 sky texnums*/

void R_Clutter_Emit(struct batch_s **batches);
void R_Clutter_Purge(void);

//r_surf.c
void Surf_NewMap (void);
void Surf_PreNewMap(void);
void Surf_SetupFrame(void);	//determine pvs+viewcontents
void Surf_DrawWorld(void);
void Surf_GenBrushBatches(struct batch_s **batches, entity_t *ent);
void Surf_StainSurf(struct msurface_s *surf, float *parms);
void Surf_AddStain(vec3_t org, float red, float green, float blue, float radius);
void Surf_LessenStains(void);
void Surf_WipeStains(void);
void Surf_DeInit(void);
void Surf_Clear(struct model_s *mod);
void Surf_BuildLightmaps(void);				//enables Surf_BuildModelLightmaps, calls it for each bsp.
void Surf_ClearLightmaps(void);				//stops Surf_BuildModelLightmaps from working.
void Surf_BuildModelLightmaps (struct model_s *m);	//rebuild lightmaps for a single bsp. beware of submodels.
void Surf_RenderDynamicLightmaps (struct msurface_s *fa);
void Surf_RenderAmbientLightmaps (struct msurface_s *fa, int ambient);
int Surf_LightmapShift (struct model_s *model);
#define LMBLOCK_SIZE_MAX 2048	//single axis
typedef struct glRect_s {
	unsigned short l,t,w,h;
} glRect_t;
typedef unsigned char stmap;
struct mesh_s;
typedef struct {
	texid_t lightmap_texture;
	qboolean	modified;
	qboolean	external;
	qboolean	hasdeluxe;
	int			width;
	int			height;
	glRect_t	rectchange;
	qbyte		*lightmaps;//[4*LMBLOCK_WIDTH*LMBLOCK_HEIGHT];
	stmap		*stainmaps;//[3*LMBLOCK_WIDTH*LMBLOCK_HEIGHT];	//rgb no a. added to lightmap for added (hopefully) speed.
} lightmapinfo_t;
extern lightmapinfo_t **lightmap;
extern int numlightmaps;
//extern texid_t		*lightmap_textures;
//extern texid_t		*deluxmap_textures;
extern int			lightmap_bytes;		// 1, 3, or 4
extern qboolean		lightmap_bgra;		/*true=bgra, false=rgba*/

void QDECL Surf_RebuildLightmap_Callback (struct cvar_s *var, char *oldvalue);


void R_SetSky(char *skyname);		/*override all sky shaders*/

#if defined(GLQUAKE)
void GLR_Init (void);
void GLR_InitTextures (void);
void GLR_InitEfrags (void);
void GLR_RenderView (void);		// must set r_refdef first
								// called whenever r_refdef or vid change
void GLR_DrawPortal(struct batch_s *batch, struct batch_s **blist, struct batch_s *depthmasklist[2], int portaltype);

void GLR_PushDlights (void);
void GLR_DrawWaterSurfaces (void);

void GLVID_DeInit (void);
void GLR_DeInit (void);
void GLSCR_DeInit (void);
void GLVID_Console_Resize(void);
#endif
int R_LightPoint (vec3_t p);
void R_RenderDlights (void);

enum imageflags
{
	/*warning: many of these flags only apply the first time it is requested*/
	IF_CLAMP = 1<<0,	//disable texture coord wrapping.
	IF_NOMIPMAP = 1<<1,	//disable mipmaps.
	IF_NEAREST = 1<<2,	//force nearest
	IF_LINEAR = 1<<3,	//force linear
	IF_UIPIC = 1<<4,	//subject to texturemode2d
	/*WARNING: If the above are changed, be sure to change shader pass flags*/

	IF_NOPICMIP = 1<<5,
	IF_NOALPHA = 1<<6,	/*hint rather than requirement*/
	IF_NOGAMMA = 1<<7,
	IF_3DMAP = 1<<8,	/*waning - don't test directly*/
	IF_CUBEMAP = 1<<9,	/*waning - don't test directly*/
	IF_TEXTYPE = (1<<8) | (1<<9), /*0=2d, 1=3d, 2=cubeface, 3=?*/
	IF_TEXTYPESHIFT = 8, /*0=2d, 1=3d, 2-7=cubeface*/
	IF_MIPCAP = 1<<10,
	IF_PREMULTIPLYALPHA = 1<<12,	//rgb *= alpha

	IF_NOPURGE = 1<<22,
	IF_HIGHPRIORITY = 1<<23,
	IF_LOWPRIORITY = 1<<24,
	IF_LOADNOW = 1<<25,			/*hit the disk now, and delay the gl load until its actually needed. this is used only so that the width+height are known in advance*/
	IF_NOPCX = 1<<26,			/*block pcx format. meaning qw skins can use team colours and cropping*/
	IF_TRYBUMP = 1<<27,			/*attempt to load _bump if the specified _norm texture wasn't found*/
	IF_RENDERTARGET = 1<<28,	/*never loaded from disk, loading can't fail*/
	IF_EXACTEXTENSION = 1<<29,	/*don't mangle extensions, use what is specified and ONLY that*/
	IF_NOREPLACE = 1<<30,		/*don't load a replacement, for some reason*/
	IF_NOWORKER = 1u<<31		/*don't pass the work to a loader thread. this gives fully synchronous loading. only valid from the main thread.*/
};

#define R_LoadTexture8(id,w,h,d,f,t)		Image_GetTexture(id, NULL, f, d, NULL, w, h, t?TF_TRANS8:TF_SOLID8)
#define R_LoadTexture32(id,w,h,d,f)			Image_GetTexture(id, NULL, f, d, NULL, w, h, TF_RGBA32)
#define R_LoadTextureFB(id,w,h,d,f)			Image_GetTexture(id, NULL, f, d, NULL, w, h, TF_TRANS8_FULLBRIGHT)
#define R_LoadTexture(id,w,h,fmt,d,fl)		Image_GetTexture(id, NULL, fl, d, NULL, w, h, fmt)

image_t *Image_FindTexture	(const char *identifier, const char *subpath, unsigned int flags);
image_t *Image_CreateTexture(const char *identifier, const char *subpath, unsigned int flags);
image_t *Image_GetTexture	(const char *identifier, const char *subpath, unsigned int flags, void *fallbackdata, void *fallbackpalette, int fallbackwidth, int fallbackheight, uploadfmt_t fallbackfmt);
qboolean Image_UnloadTexture(image_t *tex);	//true if it did something.
void Image_DestroyTexture	(image_t *tex);
void Image_Upload			(texid_t tex, uploadfmt_t fmt, void *data, void *palette, int width, int height, unsigned int flags);
void Image_Purge(void);	//purge any textures which are not needed any more (releases memory, but doesn't give null pointers).
void Image_Init(void);
void Image_Shutdown(void);

image_t *Image_LoadTexture	(const char *identifier, int width, int height, uploadfmt_t fmt, void *data, unsigned int flags);

#ifdef D3D9QUAKE
void		D3D9_UpdateFiltering	(image_t *imagelist, int filtermip[3], int filterpic[3], int mipcap[2], float anis);
qboolean	D3D9_LoadTextureMips	(texid_t tex, struct pendingtextureinfo *mips);
void		D3D9_DestroyTexture		(texid_t tex);
#endif
#ifdef D3D11QUAKE
void		D3D11_UpdateFiltering	(image_t *imagelist, int filtermip[3], int filterpic[3], int mipcap[2], float anis);
qboolean	D3D11_LoadTextureMips	(texid_t tex, struct pendingtextureinfo *mips);
void		D3D11_DestroyTexture	(texid_t tex);
#endif

//extern int image_width, image_height;
texid_t R_LoadReplacementTexture(const char *name, const char *subpath, unsigned int flags, void *lowres, int lowreswidth, int lowresheight, uploadfmt_t fmt);
texid_tf R_LoadHiResTexture(const char *name, const char *subpath, unsigned int flags);
texid_tf R_LoadBumpmapTexture(const char *name, const char *subpath);
void R_LoadNumberedLightTexture(struct dlight_s *dl, int cubetexnum);

qbyte *Read32BitImageFile(qbyte *buf, int len, int *width, int *height, qboolean *hasalpha, char *fname);

extern	texid_t	particletexture;
extern	texid_t particlecqtexture;
extern	texid_t explosiontexture;
extern	texid_t balltexture;
extern	texid_t beamtexture;
extern	texid_t ptritexture;

skinid_t Mod_RegisterSkinFile(const char *skinname);
skinid_t Mod_ReadSkinFile(const char *skinname, const char *skintext);
void Mod_WipeSkin(skinid_t id);
skinfile_t *Mod_LookupSkin(skinid_t id);

void	Mod_Init (qboolean initial);
void Mod_Shutdown (qboolean final);
int Mod_TagNumForName(struct model_s *model, const char *name);
int Mod_SkinNumForName(struct model_s *model, int surfaceidx, const char *name);
int Mod_FrameNumForName(struct model_s *model, int surfaceidx, const char *name);
float Mod_GetFrameDuration(struct model_s *model, int surfaceidx, int frameno);

void Mod_ResortShaders(void);
void	Mod_ClearAll (void);
struct model_s *Mod_FindName (const char *name);
void	*Mod_Extradata (struct model_s *mod);	// handles caching
void	Mod_TouchModel (const char *name);
void Mod_RebuildLightmaps (void);

typedef struct
{
	unsigned int *offsets;
	unsigned short *extents;
	unsigned char *styles;
	unsigned char *shifts;
} lightmapoverrides_t;
void Mod_LoadLighting (struct model_s *loadmodel, qbyte *mod_base, lump_t *l, qboolean interleaveddeluxe, lightmapoverrides_t *overrides);

struct mleaf_s *Mod_PointInLeaf (struct model_s *model, float *p);

void Mod_Think (void);
void Mod_NowLoadExternal(struct model_s *loadmodel);
void GLR_LoadSkys (void);
void R_BloomRegister(void);

int Mod_RegisterModelFormatText(void *module, const char *formatname, char *magictext, qboolean (QDECL *load) (struct model_s *mod, void *buffer, size_t fsize));
int Mod_RegisterModelFormatMagic(void *module, const char *formatname, unsigned int magic, qboolean (QDECL *load) (struct model_s *mod, void *buffer, size_t fsize));
void Mod_UnRegisterModelFormat(void *module, int idx);
void Mod_UnRegisterAllModelFormats(void *module);
void Mod_ModelLoaded(void *ctx, void *data, size_t a, size_t b);

#ifdef RUNTIMELIGHTING
struct relight_ctx_s;
struct llightinfo_s;
void LightFace (struct relight_ctx_s *ctx, struct llightinfo_s *threadctx, int surfnum);	//version that is aware of bsp trees
void LightPlane (struct relight_ctx_s *ctx, struct llightinfo_s *threadctx, qbyte surf_styles[4], qbyte *surf_rgbsamples, qbyte *surf_deluxesamples, vec4_t surf_plane, vec4_t surf_texplanes[2], vec2_t exactmins, vec2_t exactmaxs, int texmins[2], int texsize[2], float lmscale);	//special version that doesn't know what a face is or anything.
struct relight_ctx_s *LightStartup(struct relight_ctx_s *ctx, struct model_s *model, qboolean shadows);
void LightReloadEntities(struct relight_ctx_s *ctx, char *entstring);
void LightShutdown(struct relight_ctx_s *ctx, struct model_s *mod);
extern const size_t lightthreadctxsize;
#endif


extern struct model_s		*currentmodel;

qboolean Media_ShowFilm(void);
void Media_CaptureDemoEnd(void);
void Media_RecordFrame (void);
qboolean Media_PausedDemo (qboolean fortiming);
int Media_Capturing (void);
double Media_TweekCaptureFrameTime(double oldtime, double time);

void MYgluPerspective(double fovx, double fovy, double zNear, double zFar);

void	R_PushDlights				(void);
qbyte *R_MarkLeaves_Q1 (void);
qbyte *R_CalcVis_Q1 (void);
qbyte *R_MarkLeaves_Q2 (void);
qbyte *R_MarkLeaves_Q3 (void);
void R_SetFrustum (float projmat[16], float viewmat[16]);
void R_SetRenderer(rendererinfo_t *ri);
void R_AnimateLight (void);
void R_UpdateHDR(vec3_t org);
void R_UpdateLightStyle(unsigned int style, const char *stylestring, float r, float g, float b);
struct texture_s *R_TextureAnimation (int frame, struct texture_s *base);	//mostly deprecated, only lingers for rtlights so world only.
struct texture_s *R_TextureAnimation_Q2 (struct texture_s *base);	//mostly deprecated, only lingers for rtlights so world only.
void RQ_Init(void);
void RQ_Shutdown(void);

void CLQ2_EntityEvent(entity_state_t *es);
void CLQ2_TeleporterParticles(entity_state_t *es);
void CLQ2_IonripperTrail(vec3_t oldorg, vec3_t neworg);
void CLQ2_TrackerTrail(vec3_t oldorg, vec3_t neworg, int flags);
void CLQ2_Tracker_Shell(vec3_t org);
void CLQ2_TagTrail(vec3_t oldorg, vec3_t neworg, int flags);
void CLQ2_FlagTrail(vec3_t oldorg, vec3_t neworg, int flags);
void CLQ2_TrapParticles(entity_t *ent);
void CLQ2_BfgParticles(entity_t *ent);
struct q2centity_s;
void CLQ2_FlyEffect(struct q2centity_s *ent, vec3_t org);
void CLQ2_DiminishingTrail(vec3_t oldorg, vec3_t neworg, struct q2centity_s *ent, unsigned int effects);
void CLQ2_BlasterTrail2(vec3_t oldorg, vec3_t neworg);

void WritePCXfile (const char *filename, enum fs_relative fsroot, qbyte *data, int width, int height, int rowbytes, qbyte *palette, qboolean upload); //data is 8bit.
qbyte *ReadPCXFile(qbyte *buf, int length, int *width, int *height);
qbyte *ReadTargaFile(qbyte *buf, int length, int *width, int *height, qboolean *hasalpha, int asgrey);
qbyte *ReadJPEGFile(qbyte *infile, int length, int *width, int *height);
qbyte *ReadPNGFile(qbyte *buf, int length, int *width, int *height, const char *name);
qbyte *ReadPCXPalette(qbyte *buf, int len, qbyte *out);
void Image_ResampleTexture (unsigned *in, int inwidth, int inheight, unsigned *out,  int outwidth, int outheight);

void BoostGamma(qbyte *rgba, int width, int height);
void SaturateR8G8B8(qbyte *data, int size, float sat);
void AddOcranaLEDsIndexed (qbyte *image, int h, int w);

void Renderer_Init(void);
void Renderer_Start(void);
qboolean Renderer_Started(void);
void R_ShutdownRenderer(qboolean videotoo);
void R_RestartRenderer_f (void);//this goes here so we can save some stack when first initing the sw renderer.

//used to live in glquake.h
qbyte GetPaletteIndex(int red, int green, int blue);
extern	cvar_t	r_norefresh;
extern	cvar_t	r_drawentities;
extern	cvar_t	r_drawworld;
extern	cvar_t	r_drawviewmodel;
extern	cvar_t	r_drawviewmodelinvis;
extern	cvar_t	r_speeds;
extern	cvar_t	r_waterwarp;
extern	cvar_t	r_fullbright;
extern	cvar_t	r_lightmap;
extern	cvar_t	r_glsl_offsetmapping;
extern	cvar_t	r_shadow_realtime_dlight, r_shadow_realtime_dlight_shadows;
extern	cvar_t	r_shadow_realtime_dlight_ambient;
extern	cvar_t	r_shadow_realtime_dlight_diffuse;
extern	cvar_t	r_shadow_realtime_dlight_specular;
extern	cvar_t	r_shadow_realtime_world, r_shadow_realtime_world_shadows, r_shadow_realtime_world_lightmaps;
extern	cvar_t	r_shadow_shadowmapping;
extern	cvar_t	r_editlights_import_radius;
extern	cvar_t	r_editlights_import_ambient;
extern	cvar_t	r_editlights_import_diffuse;
extern	cvar_t	r_editlights_import_specular;
extern	cvar_t	r_mirroralpha;
extern	cvar_t	r_wateralpha;
extern	cvar_t	r_lavaalpha;
extern	cvar_t	r_slimealpha;
extern	cvar_t	r_telealpha;
extern	cvar_t	r_waterstyle;
extern	cvar_t	r_lavastyle;
extern	cvar_t	r_slimestyle;
extern	cvar_t	r_telestyle;
extern	cvar_t	r_dynamic;
extern	cvar_t	r_novis;
extern	cvar_t	r_netgraph;
extern	cvar_t	r_deluxemapping_cvar;
extern	qboolean r_deluxemapping;
extern	cvar_t r_softwarebanding_cvar;
extern	qboolean r_softwarebanding;

#ifdef R_XFLIP
extern cvar_t	r_xflip;
#endif

extern cvar_t r_lightprepass;
extern cvar_t gl_maxdist;
extern	cvar_t	r_clear;
extern	cvar_t	gl_poly;
extern	cvar_t	gl_affinemodels;
extern	cvar_t r_renderscale;
extern	cvar_t	gl_nohwblend;
extern	cvar_t	r_coronas, r_flashblend, r_flashblendscale;
extern	cvar_t	r_lightstylesmooth;
extern	cvar_t	r_lightstylesmooth_limit;
extern	cvar_t	r_lightstylespeed;
extern	cvar_t	r_lightstylescale;
extern	cvar_t	gl_nocolors;
extern	cvar_t	gl_load24bit;
extern	cvar_t	gl_finish;

extern	cvar_t	gl_max_size;
extern	cvar_t	gl_playermip;

extern  cvar_t	r_lightmap_saturation;

enum {
	RSPEED_TOTALREFRESH,
	RSPEED_LINKENTITIES,
	RSPEED_PROTOCOL,
	RSPEED_WORLDNODE,
	RSPEED_WORLD,
	RSPEED_DRAWENTITIES,
	RSPEED_STENCILSHADOWS,
	RSPEED_FULLBRIGHTS,
	RSPEED_DYNAMIC,
	RSPEED_PARTICLES,
	RSPEED_PARTICLESDRAW,
	RSPEED_PALETTEFLASHES,
	RSPEED_2D,
	RSPEED_SERVER,
	RSPEED_FINISH,

	RSPEED_MAX
};
extern int rspeeds[RSPEED_MAX];

enum {
	RQUANT_MSECS,	//old r_speeds
	RQUANT_PRIMITIVEINDICIES,
	RQUANT_DRAWS,
	RQUANT_ENTBATCHES,
	RQUANT_WORLDBATCHES,
	RQUANT_2DBATCHES,

	RQUANT_SHADOWINDICIES,
	RQUANT_SHADOWEDGES,
	RQUANT_SHADOWSIDES,
	RQUANT_LITFACES,

	RQUANT_RTLIGHT_DRAWN,
	RQUANT_RTLIGHT_CULL_FRUSTUM,
	RQUANT_RTLIGHT_CULL_PVS,
	RQUANT_RTLIGHT_CULL_SCISSOR,

	RQUANT_MAX
};
extern int rquant[RQUANT_MAX];

#define RQuantAdd(type,quant) rquant[type] += quant

#if defined(NDEBUG) || !defined(_WIN32)
#define RSpeedLocals()
#define RSpeedMark()
#define RSpeedRemark()
#define RSpeedEnd(spt)
#else
#define RSpeedLocals() double rsp
#define RSpeedMark() double rsp = (r_speeds.ival>1)?Sys_DoubleTime()*1000000:0
#define RSpeedRemark() rsp = (r_speeds.ival>1)?Sys_DoubleTime()*1000000:0

#if defined(_WIN32) && defined(GLQUAKE)
extern void (_stdcall *qglFinish) (void);
#define RSpeedEnd(spt) do {if(r_speeds.ival > 1){if(r_speeds.ival > 2 && qglFinish)qglFinish(); rspeeds[spt] += (double)(Sys_DoubleTime()*1000000) - rsp;}}while (0)
#else
#define RSpeedEnd(spt) rspeeds[spt] += (r_speeds.ival>1)?Sys_DoubleTime()*1000000 - rsp:0
#endif
#endif
