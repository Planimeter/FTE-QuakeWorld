//These are defined later in the source tree. This file should probably be moved to a later spot.
struct pubprogfuncs_s;
struct globalvars_s;
struct texture_s;
struct texnums_s;
struct vbo_s;
struct mesh_s;
struct batch_s;
struct entity_s;
struct dlight_s;
struct galiasbone_s;
struct dlight_s;
struct font_s;

typedef enum
{
	SKEL_RELATIVE,	//relative to parent.
	SKEL_ABSOLUTE,	//relative to model. doesn't blend very well.
	SKEL_INVERSE_RELATIVE,	//pre-inverted. faster than regular relative but has weirdness with skeletal objects. blends okay.
	SKEL_INVERSE_ABSOLUTE,	//final renderable type.
	SKEL_IDENTITY	//PANIC
} skeltype_t;

#ifdef HALFLIFEMODELS
	#define MAX_BONE_CONTROLLERS 5
#endif

#define FRAME_BLENDS 4
#define FST_BASE 0	//base frames
#define FS_REG 1	//regular frames
#define FS_COUNT 2	//regular frames
typedef struct {
	struct framestateregion_s {
		int frame[FRAME_BLENDS];
		float frametime[FRAME_BLENDS];
		float lerpweight[FRAME_BLENDS];

#ifdef HALFLIFEMODELS
		float subblendfrac;	//hl models are weird
#endif

		int endbone;
	} g[FS_COUNT];

#ifdef SKELETALOBJECTS
	float *bonestate;
	int bonecount;
	skeltype_t skeltype;
#endif

#ifdef HALFLIFEMODELS
	float bonecontrols[MAX_BONE_CONTROLLERS];	//hl special bone controllers
#endif
} framestate_t;




//function prototypes

#if defined(SERVERONLY)
#define qrenderer QR_NONE
#define FNC(n) (n)			//FNC is defined as 'pointer if client build, direct if dedicated server'

#else
#define FNC(n) (*n)
extern r_qrenderer_t qrenderer;
extern char *q_renderername;

apic_t *R2D_LoadAtlasedPic(const char *name);
void R2D_ImageAtlas(float x, float y, float w, float h, float s1, float t1, float s2, float t2, apic_t *pic);
#define R2D_ScalePicAtlas(x,y,w,h,p) R2D_ImageAtlas(x,y,w,h,0,0,1,1,p)

mpic_t *R2D_SafeCachePic (const char *path);
mpic_t *R2D_SafePicFromWad (const char *name);
void R2D_DrawCrosshair (void);
void R2D_ScalePic (float x, float y, float width, float height, mpic_t *pic);
void R2D_SubPic(float x, float y, float width, float height, mpic_t *pic, float srcx, float srcy, float srcwidth, float srcheight);
void R2D_TransPicTranslate (float x, float y, int width, int height, qbyte *pic, unsigned int *palette);
void R2D_TileClear (float x, float y, float w, float h);
void R2D_FadeScreen (void);

void R2D_Font_Changed(void);
void R2D_ConsoleBackground (int firstline, int lastline, qboolean forceopaque);
void R2D_EditorBackground (void);

void R2D_Image(float x, float y, float w, float h, float s1, float t1, float s2, float t2, mpic_t *pic);
void R2D_Image2dQuad(vec2_t points[], vec2_t texcoords[], mpic_t *pic);

void R2D_ImageColours(float r, float g, float b, float a);
void R2D_ImagePaletteColour(unsigned int i, float a);
void R2D_FillBlock(float x, float y, float w, float h);
void R2D_Line(float x1, float y1, float x2, float y2, mpic_t *pic);
extern void (*R2D_Flush)(void);

extern void	(*Draw_Init)							(void);

extern void	(*R_Init)								(void);
extern void	(*R_DeInit)								(void);
extern void	(*R_RenderView)							(void);		// must set r_refdef first

extern qboolean	(*VID_Init)							(rendererstate_t *info, unsigned char *palette);
extern void	(*VID_DeInit)							(void);
extern char *(*VID_GetRGBInfo)						(int prepad, int *truevidwidth, int *truevidheight);
extern void	(*VID_SetWindowCaption)					(char *msg);

extern void SCR_Init								(void);
extern void SCR_DeInit								(void);
extern void (*SCR_UpdateScreen)						(void);
extern void SCR_BeginLoadingPlaque					(void);
extern void SCR_EndLoadingPlaque					(void);
extern void SCR_DrawConsole							(qboolean noback);
extern void SCR_SetUpToDrawConsole					(void);
extern void SCR_CenterPrint							(int pnum, char *str, qboolean skipgamecode);

void R_DrawTextField(int x, int y, int w, int h, const char *text, unsigned int defaultmask, unsigned int fieldflags, struct font_s *font, vec2_t fontscale);
#define CPRINT_LALIGN		(1<<0)	//L
#define CPRINT_TALIGN		(1<<1)	//T
#define CPRINT_RALIGN		(1<<2)	//R
#define CPRINT_BALIGN		(1<<3)	//B
#define CPRINT_BACKGROUND	(1<<4)	//P

#define CPRINT_OBITUARTY	(1<<16)	//O (show at 2/3rds from top)
#define CPRINT_PERSIST		(1<<17)	//P (doesn't time out)
#define CPRINT_TYPEWRITER	(1<<18)	//  (char at a time)

#endif

//mod_purge flags
enum mod_purge_e
{
	MP_MAPCHANGED,	//new map. old stuff no longer needed
	MP_FLUSH,		//user flush command. anything flushable goes.
	MP_RESET		//*everything* is destroyed. renderer is going down.
};
enum mlverbosity_e
{
	MLV_SILENT,
	MLV_WARN,
	MLV_ERROR
};

extern void	Mod_ClearAll						(void);
extern void Mod_Purge							(enum mod_purge_e type);
extern struct model_s *Mod_FindName				(const char *name);	//find without loading. needload should be set.
extern struct model_s *Mod_ForName				(const char *name, enum mlverbosity_e verbosity);	//finds+loads
extern struct model_s *Mod_LoadModel			(struct model_s *mod, enum mlverbosity_e verbose);	//makes sure a model is loaded
extern void	*Mod_Extradata						(struct model_s *mod);	// handles caching
extern void	Mod_TouchModel						(const char *name);
extern const char *Mod_FixName					(const char *modname, const char *worldname);	//remaps the name appropriately
char *Mod_ParseWorldspawnKey					(const char *ents, const char *key, char *buffer, size_t sizeofbuffer);

extern void	Mod_Think							(void);
extern int Mod_SkinNumForName					(struct model_s *model, int surfaceidx, const char *name);
extern int Mod_FrameNumForName					(struct model_s *model, int surfaceidx, const char *name);
extern float Mod_GetFrameDuration				(struct model_s *model, int surfaceidx, int framenum);

#undef FNC

extern qboolean	Mod_GetTag						(struct model_s *model, int tagnum, framestate_t *framestate, float *transforms);
extern int Mod_TagNumForName					(struct model_s *model, const char *name);

int Mod_GetNumBones(struct model_s *model, qboolean allowtags);
int Mod_GetBoneRelations(struct model_s *model, int firstbone, int lastbone, framestate_t *fstate, float *result);
int Mod_GetBoneParent(struct model_s *model, int bonenum);
struct galiasbone_s *Mod_GetBoneInfo(struct model_s *model, int *numbones);
char *Mod_GetBoneName(struct model_s *model, int bonenum);

void Draw_FunString(float x, float y, const void *str);
void Draw_AltFunString(float x, float y, const void *str);
void Draw_FunStringWidth(float x, float y, const void *str, int width, int rightalign, qboolean highlight);

extern int r_regsequence;

#ifdef SERVERONLY
#define Mod_Q1LeafPVS Mod_LeafPVS
// qbyte *Mod_LeafPVS (struct mleaf_s *leaf, struct model_s *model, qbyte *buffer);
#endif

typedef enum uploadfmt
{
        TF_INVALID,
        TF_RGBA32,              /*rgba byte order*/
        TF_BGRA32,              /*bgra byte order*/
        TF_RGBX32,              /*rgb byte order, with extra wasted byte after blue*/
		TF_BGRX32,              /*rgb byte order, with extra wasted byte after blue*/
        TF_RGB24,               /*rgb byte order, no alpha channel nor pad, and regular top down*/
		TF_BGR24,               /*bgr byte order, no alpha channel nor pad, and regular top down*/
        TF_BGR24_FLIP,  /*bgr byte order, no alpha channel nor pad, and bottom up*/
		TF_LUM8,		/*8bit greyscale image*/
		TF_MIP4_LUM8,	/*8bit 4-mip greyscale image*/
		TF_MIP4_SOLID8,	/*8bit 4-mip image*/
        TF_SOLID8,      /*8bit quake-palette image*/
        TF_TRANS8,      /*8bit quake-palette image, index 255=transparent*/
        TF_TRANS8_FULLBRIGHT,   /*fullbright 8 - fullbright texels have alpha 255, everything else 0*/
        TF_HEIGHT8,     /*image data is greyscale, convert to a normalmap and load that, uploaded alpha contains the original heights*/
        TF_HEIGHT8PAL, /*source data is palette values rather than actual heights, generate a fallback heightmap*/
        TF_H2_T7G1, /*8bit data, odd indexes give greyscale transparence*/
        TF_H2_TRANS8_0, /*8bit data, 0 is transparent, not 255*/
        TF_H2_T4A4,     /*8bit data, weird packing*/

        /*this block requires a palette*/
        TF_PALETTES,
        TF_8PAL24,
        TF_8PAL32,

		/*for render targets*/
		TF_DEPTH16,
		TF_DEPTH24,
		TF_DEPTH32,
		TF_RGBA16F,
		TF_RGBA32F,

		/*for weird systems where the gl driver needs to do the decode (read: webgl)*/
		TF_SYSTEMDECODE
} uploadfmt_t;

enum
{
	TEX_NOTLOADED,
	TEX_LOADING,
	TEX_LOADED,
	TEX_FAILED
};
typedef struct image_s
{
	char *ident;	//allocated on end
	char *subpath;	//allocated on end
	int regsequence;
	int width;	//this is the logical size. the physical size is not considered important (except for render targets, which should not be loaded from disk).
	int height;
	int status;	//TEX_
	unsigned int flags;
	struct image_s *next;
	struct image_s *prev;
	struct image_s *aliasof;
#if defined(D3DQUAKE) || defined(SWQUAKE)
	void *ptr;	//texture
	void *ptr2;	//view
#endif
#ifdef GLQUAKE
	int num;
#endif

	void *fallbackdata;
	int fallbackwidth;
	int fallbackheight;
	uploadfmt_t fallbackfmt;
} image_t;

#if 1
typedef image_t *texid_t;
#define texid_tf texid_t
#define TEXASSIGN(d,s) d=s
#define TEXASSIGNF(d,s) d=s
#define TEXVALID(t) ((t))
#define TEXLOADED(tex) ((tex) && (tex)->status == TEX_LOADED)
#define TEXDOWAIT(tex)	if ((tex) && (tex)->status == TEX_LOADING)	COM_WorkerPartialSync((tex), &(tex)->status, TEX_LOADING)
#else
typedef struct texid_s texid_t[1];
typedef struct texid_s texid_tf;
#define TEXASSIGN(d,s) memcpy(&d,&s,sizeof(d))
#define TEXASSIGNF(d,s) memcpy(&d,&s,sizeof(d))
#define TEXVALID(t) 1
#endif

struct pendingtextureinfo
{
	enum
	{
		PTI_2D,
		PTI_3D,
		PTI_CUBEMAP	//mips are packed (to make d3d11 happy)
	} type;
	enum
	{
		//these formats are specified as direct byte access
		PTI_RGBA8,	//rgba byte ordering
		PTI_RGBX8,	//rgb pad byte ordering
		PTI_BGRA8,	//alpha channel
		PTI_BGRX8,	//no alpha channel
		//these formats are specified in native endian order
		PTI_RGB565,		//16bit alphaless format.
		PTI_RGBA4444,	//16bit format (gl)
		PTI_ARGB4444,	//16bit format (d3d)
		PTI_RGBA5551,	//16bit alpha format (gl).
		PTI_ARGB1555,	//16bit alpha format (d3d).
		//floating point formats
		PTI_RGBA16F,
		PTI_RGBA32F,
		//small formats.
		PTI_R8,
		PTI_RG8,
		//compressed formats
		PTI_S3RGB1,
		PTI_S3RGBA1,
		PTI_S3RGBA3,
		PTI_S3RGBA5,
		//weird specialcase mess to take advantage of webgl so we don't need redundant bloat where we're already strugging with potential heap limits
		PTI_WHOLEFILE,
		//depth formats
		PTI_DEPTH16,
		PTI_DEPTH24,
		PTI_DEPTH32,
		PTI_DEPTH24_8,
		PTI_MAX
	} encoding;	//0
	int mipcount;
	struct
	{
		void *data;
		size_t datasize;
		int width;
		int height;
		qboolean needfree;
	} mip[32];
	void *extrafree;
};

//small context for easy vbo creation.
typedef struct
{
	size_t maxsize;
	size_t pos;
	int vboid[2];
	void *vboptr[2];
	void *fallback;
} vbobctx_t;

typedef struct vboarray_s
{
	union
	{
		void *sysptr;
#ifdef GLQUAKE
		struct
		{
			int vbo;
			void *addr;
		} gl;
#endif
#if defined(D3D9QUAKE) || defined(D3D11QUAKE)
		struct
		{
			void *buff;
			unsigned int offs;
		} d3d;
#endif
	};
} vboarray_t;

//scissor rects
typedef struct
{
	float x;
	float y;
	float width;
	float height;
	double dmin;
	double dmax;
} srect_t;

typedef struct texnums_s {
	char	mapname[MAX_QPATH];	//the 'official' name of the diffusemap. used to generate filenames for other textures.
	texid_t base;			//regular diffuse texture. may have alpha if surface is transparent
	texid_t bump;			//normalmap. height values packed in alpha.
	texid_t specular;		//specular lighting values.
	texid_t upperoverlay;	//diffuse texture for the upper body(shirt colour). no alpha channel. added to base.rgb
	texid_t loweroverlay;	//diffuse texture for the lower body(trouser colour). no alpha channel. added to base.rgb
	texid_t paletted;		//8bit paletted data, just because.
	texid_t fullbright;
	texid_t reflectcube;
	texid_t reflectmask;
} texnums_t;

//not all modes accept meshes - STENCIL(intentional) and DEPTHONLY(not implemented)
typedef enum backendmode_e
{
        BEM_STANDARD,			//regular mode to draw surfaces akin to q3 (aka: legacy mode). lightmaps+delux+ambient
        BEM_DEPTHONLY,			//just a quick depth pass. textures used only for alpha test (shadowmaps).
		BEM_WIREFRAME,			//for debugging or something
        BEM_STENCIL,			//used for drawing shadow volumes to the stencil buffer.
        BEM_DEPTHDARK,			//a quick depth pass. textures used only for alpha test. additive textures still shown as normal.
		BEM_CREPUSCULAR,		//sky is special, everything else completely black
		BEM_DEPTHNORM,			//all opaque stuff drawn using 'depthnorm' shader
		BEM_FOG,				//drawing a fog volume
        BEM_LIGHT,				//we have a valid light
} backendmode_t;

typedef struct rendererinfo_s {
	char *description;
	char *name[4];
	r_qrenderer_t rtype;

	void	(*Draw_Init)				(void);
	void	(*Draw_Shutdown)			(void);

	void	(*IMG_UpdateFiltering)		(image_t *imagelist, int filtermip[3], int filterpic[3], int mipcap[2], float anis);
	qboolean (*IMG_LoadTextureMips)		(texid_t tex, struct pendingtextureinfo *mips);
	void    (*IMG_DestroyTexture)		(texid_t tex);

	void	(*R_Init)					(void); //FIXME - merge implementations
	void	(*R_DeInit)					(void);	//FIXME - merge implementations
	void	(*R_RenderView)				(void);	// must set r_refdef first

	qboolean (*VID_Init)				(rendererstate_t *info, unsigned char *palette);
	void	 (*VID_DeInit)				(void);
	void	(*VID_SwapBuffers)			(void);	//force a buffer swap, regardless of what's displayed.
	qboolean (*VID_ApplyGammaRamps)		(unsigned short *ramps);

	void *(*VID_CreateCursor)			(char *filename, float hotx, float hoty, float scale);	//may be null, stub returns null
	qboolean (*VID_SetCursor)			(void *cursor);	//may be null
	void (*VID_DestroyCursor)			(void *cursor);	//may be null

	void	(*VID_SetWindowCaption)		(char *msg);
	char	*(*VID_GetRGBInfo)			(int prepad, int *truevidwidth, int *truevidheight);

	void	(*SCR_UpdateScreen)			(void);

	
	//Select the current render mode and modifier flags
	void	(*BE_SelectMode)(backendmode_t mode);
	/*Draws an entire mesh list from a VBO. vbo can be null, in which case the chain may be drawn without batching.
	  Rules for using a list: Every mesh must be part of the same VBO, shader, lightmap, and must have the same pointers set*/
	void	(*BE_DrawMesh_List)(shader_t *shader, int nummeshes, struct mesh_s **mesh, struct vbo_s *vbo, struct texnums_s *texnums, unsigned int be_flags);
	void	(*BE_DrawMesh_Single)(shader_t *shader, struct mesh_s *meshchain, struct vbo_s *vbo, unsigned int be_flags);
	void	(*BE_SubmitBatch)(struct batch_s *batch);
	struct batch_s *(*BE_GetTempBatch)(void);
	//Asks the backend to invoke DrawMeshChain for each surface, and to upload lightmaps as required
	void	(*BE_DrawWorld) (qboolean drawworld, qbyte *vis);
	//called at init, force the display to the right defaults etc
	void	(*BE_Init)(void);
	//Generates an optimised VBO, one for each texture on the map
	void (*BE_GenBrushModelVBO)(struct model_s *mod);
	//Destroys the given vbo
	void (*BE_ClearVBO)(struct vbo_s *vbo);
	//Uploads all modified lightmaps
	void (*BE_UploadAllLightmaps)(void);
	void (*BE_SelectEntity)(struct entity_s *ent);
	qboolean (*BE_SelectDLight)(struct dlight_s *dl, vec3_t colour, vec3_t axis[3], unsigned int lmode);
	void (*BE_Scissor)(srect_t *rect);
	/*check to see if an ent should be drawn for the selected light*/
	qboolean (*BE_LightCullModel)(vec3_t org, struct model_s *model);
	void (*BE_VBO_Begin)(vbobctx_t *ctx, size_t maxsize);
	void (*BE_VBO_Data)(vbobctx_t *ctx, void *data, size_t size, vboarray_t *varray);
	void (*BE_VBO_Finish)(vbobctx_t *ctx, void *edata, size_t esize, vboarray_t *earray);
	void (*BE_VBO_Destroy)(vboarray_t *vearray);
	void (*BE_RenderToTextureUpdate2d)(qboolean destchanged);
	char *alignment;
} rendererinfo_t;

#define rf currentrendererstate.renderer

#define VID_SwapBuffers		rf->VID_SwapBuffers

#define BE_Init					rf->BE_Init
#define BE_SelectMode			rf->BE_SelectMode
#define BE_GenBrushModelVBO		rf->BE_GenBrushModelVBO
#define BE_ClearVBO				rf->BE_ClearVBO
#define BE_UploadAllLightmaps	rf->BE_UploadAllLightmaps
#define BE_LightCullModel		rf->BE_LightCullModel
#define BE_SelectEntity			rf->BE_SelectEntity
#define BE_SelectDLight			rf->BE_SelectDLight
#define BE_GetTempBatch			rf->BE_GetTempBatch
#define BE_SubmitBatch			rf->BE_SubmitBatch
#define BE_DrawMesh_List		rf->BE_DrawMesh_List
#define BE_DrawMesh_Single		rf->BE_DrawMesh_Single
#define BE_SubmitMeshes			rf->BE_SubmitMeshes
#define BE_DrawWorld			rf->BE_DrawWorld
#define BE_VBO_Begin 			rf->BE_VBO_Begin
#define BE_VBO_Data				rf->BE_VBO_Data
#define BE_VBO_Finish			rf->BE_VBO_Finish
#define BE_VBO_Destroy			rf->BE_VBO_Destroy
#define BE_Scissor				rf->BE_Scissor

#define BE_RenderToTextureUpdate2d rf->BE_RenderToTextureUpdate2d

texid_t R2D_RT_Configure(const char *id, int width, int height, uploadfmt_t rtfmt);
texid_t R2D_RT_GetTexture(const char *id, unsigned int *width, unsigned int *height);


