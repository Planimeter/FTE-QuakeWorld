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
// r_main.c

#include "quakedef.h"

#ifdef RGLQUAKE
#include "glquake.h"

void R_RenderBrushPoly (msurface_t *fa);

#define PROJECTION_DISTANCE			200
#define MAX_STENCIL_ENTS			128

extern int		gl_canstencil;

PFNGLCOMPRESSEDTEXIMAGE2DARBPROC qglCompressedTexImage2DARB;
PFNGLGETCOMPRESSEDTEXIMAGEARBPROC qglGetCompressedTexImageARB;

extern struct mleaf_s *GLMod_PointInLeaf	(float *p, struct model_s *model);

#define	Q2RF_WEAPONMODEL		4		// only draw through eyes
#define Q2RF_DEPTHHACK 16

entity_t	r_worldentity;

qboolean	r_cache_thrash;		// compatability

vec3_t		modelorg, r_entorigin;
entity_t	*currententity;

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

float		r_wateralphaval;	//allowed or not...

mplane_t	frustum[4];

int			c_brush_polys, c_alias_polys;

qboolean	envmap;				// true during envmap command capture 

int			particletexture;	// little dot for particles
int			explosiontexture;
int			playertextures;		// up to 16 color translated skins

int			mirrortexturenum;	// quake texturenum, not gltexturenum
qboolean	mirror;
mplane_t	*mirror_plane;
msurface_t	*r_mirror_chain;

void R_DrawAliasModel (entity_t *e);

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

float	r_world_matrix[16];
float	r_base_world_matrix[16];

//
// screen size info
//
refdef_t	r_refdef;

mleaf_t		*r_viewleaf, *r_oldviewleaf;
mleaf_t		*r_viewleaf2, *r_oldviewleaf2;
int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

texture_t	*r_notexture_mip;

int		d_lightstylevalue[256];	// 8.8 fraction of base light value


void GLR_MarkLeaves (void);

cvar_t	r_norefresh = {"r_norefresh","0"};
//cvar_t	r_drawentities = {"r_drawentities","1"};
//cvar_t	r_drawviewmodel = {"r_drawviewmodel","1"};
//cvar_t	r_speeds = {"r_speeds","0"};
//cvar_t	r_fullbright = {"r_fullbright","0"};
cvar_t	r_lightmap = {"r_lightmap","0", NULL, CVAR_CHEAT};
cvar_t	r_mirroralpha = {"r_mirroralpha","1", NULL, CVAR_CHEAT};
cvar_t	r_wateralpha = {"r_wateralpha","1", NULL};
//cvar_t	r_waterwarp = {"r_waterwarp", "0"};
cvar_t	r_novis = {"r_novis","0"};
//cvar_t	r_netgraph = {"r_netgraph","0"};

extern cvar_t	gl_part_flame;
extern cvar_t	gl_part_torch;

cvar_t	gl_clear = {"gl_clear","0"};
cvar_t	gl_cull = {"gl_cull","1"};
cvar_t	gl_smoothmodels = {"gl_smoothmodels","1"};
cvar_t	gl_affinemodels = {"gl_affinemodels","0"};
cvar_t	gl_polyblend = {"gl_polyblend","1"};
cvar_t	gl_playermip = {"gl_playermip","0"};
cvar_t	gl_keeptjunctions = {"gl_keeptjunctions","1"};
cvar_t	gl_reporttjunctions = {"gl_reporttjunctions","0"};
cvar_t	gl_finish = {"gl_finish","0"};
cvar_t	gl_contrast = {"gl_contrast", "1"};
cvar_t	gl_dither = {"gl_dither", "1"};
cvar_t	gl_maxdist = {"gl_maxdist", "8192"};

extern cvar_t gl_ati_truform;
extern cvar_t gl_ati_truform_type;
extern cvar_t gl_ati_truform_tesselation;


#ifdef R_XFLIP
cvar_t	r_xflip = {"leftisright", "0"};
#endif

extern	cvar_t	gl_ztrick;
extern	cvar_t	scr_fov;

// post processing stuff
int scenepp_texture;
int scenepp_texture_warp;
int scenepp_texture_edge;

int scenepp_ww_program;
int scenepp_ww_parm_texture0i;
int scenepp_ww_parm_texture1i;
int scenepp_ww_parm_texture2i;
int scenepp_ww_parm_ampscalef;

// KrimZon - init post processing - called in GL_CheckExtensions, when they're called
// I put it here so that only this file need be changed when messing with the post
// processing shaders
void GL_InitSceneProcessingShaders (void)
{
	int vert, frag;

	char *genericvert = "\
		varying vec2 v_texCoord0;\
		varying vec2 v_texCoord1;\
		varying vec2 v_texCoord2;\
		void main (void)\
		{\
			vec4 v = vec4( gl_Vertex.x, gl_Vertex.y, gl_Vertex.z, 1.0 );\
			gl_Position = gl_ModelViewProjectionMatrix * v;\
			v_texCoord0 = gl_MultiTexCoord0.xy;\
			v_texCoord1 = gl_MultiTexCoord1.xy;\
			v_texCoord2 = gl_MultiTexCoord2.xy;\
		}\
		";

	char *wwfrag = "\
		varying vec2 v_texCoord0;\
		varying vec2 v_texCoord1;\
		varying vec2 v_texCoord2;\
		uniform sampler2D theTexture0;\
		uniform sampler2D theTexture1;\
		uniform sampler2D theTexture2;\
		uniform float ampscale;\
		void main (void)\
		{\
			float amptemp;\
			vec3 edge;\
			edge = texture2D( theTexture2, v_texCoord2 );\
			amptemp = ampscale * edge.x;\
			vec3 offset;\
			offset = texture2D( theTexture1, v_texCoord1 );\
			offset.x = (offset.x - 0.5) * 2.0;\
			offset.y = (offset.y - 0.5) * 2.0;\
			vec2 temp;\
			temp.x = v_texCoord0.x + offset.x * amptemp;\
			temp.y = v_texCoord0.y + offset.y * amptemp;\
			gl_FragColor = texture2D( theTexture0, temp );\
		}\
		";

	if (qglGetError())
		Con_Printf("GL Error before initing shader object\n");

	vert = GLSlang_CreateShader(genericvert,	1);//GL_VERTEX_SHADER_ARB);
	frag = GLSlang_CreateShader(wwfrag,			0);//GL_FRAGMENT_SHADER_ARB);

	scenepp_ww_program = GLSlang_CreateProgram(vert, frag);

	scenepp_ww_parm_texture0i	= GLSlang_GetUniformLocation(scenepp_ww_program, "theTexture0");
	scenepp_ww_parm_texture1i	= GLSlang_GetUniformLocation(scenepp_ww_program, "theTexture1");
	scenepp_ww_parm_texture2i	= GLSlang_GetUniformLocation(scenepp_ww_program, "theTexture2");
	scenepp_ww_parm_ampscalef	= GLSlang_GetUniformLocation(scenepp_ww_program, "ampscale");

	GLSlang_UseProgram(scenepp_ww_program);

	GLSlang_SetUniform1i(scenepp_ww_parm_texture0i, 0);
	GLSlang_SetUniform1i(scenepp_ww_parm_texture1i, 1);
	GLSlang_SetUniform1i(scenepp_ww_parm_texture2i, 2);

	GLSlang_UseProgram(0);

	if (qglGetError())
		Con_Printf("GL Error initing shader object\n");
}

#define PP_WARP_TEX_SIZE 64
#define PP_AMP_TEX_SIZE 64
#define PP_AMP_TEX_BORDER 4
void GL_SetupSceneProcessingTextures (void)
{
	int i, x, y;
	unsigned char pp_warp_tex[PP_WARP_TEX_SIZE*PP_WARP_TEX_SIZE*3];
	unsigned char pp_edge_tex[PP_AMP_TEX_SIZE*PP_AMP_TEX_SIZE*3];

	scenepp_texture = texture_extension_number++;
	scenepp_texture_warp = texture_extension_number++;
	scenepp_texture_edge = texture_extension_number++;

	// init warp texture - this specifies offset in 
	for (y=0; y<PP_WARP_TEX_SIZE; y++)
	{
		for (x=0; x<PP_WARP_TEX_SIZE; x++)
		{
			float fx, fy;

			i = (x + y*PP_WARP_TEX_SIZE) * 3;

			fx = sin(((double)y / PP_WARP_TEX_SIZE) * M_PI * 2);
			fy = cos(((double)x / PP_WARP_TEX_SIZE) * M_PI * 2);

			pp_warp_tex[i  ] = (fx+1.0f)*127.0f;
			pp_warp_tex[i+1] = (fy+1.0f)*127.0f;
			pp_warp_tex[i+2] = 0;
		}
	}

	GL_Bind(scenepp_texture_warp);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, PP_WARP_TEX_SIZE, PP_WARP_TEX_SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, pp_warp_tex);

	// TODO: init edge texture - this is ampscale * 2, with ampscale calculated
	// init warp texture - this specifies offset in 
	for (y=0; y<PP_AMP_TEX_SIZE; y++)
	{
		for (x=0; x<PP_AMP_TEX_SIZE; x++)
		{
			float fx = 1, fy = 1;

			i = (x + y*PP_AMP_TEX_SIZE) * 3;

			if (x < PP_AMP_TEX_BORDER)
			{
				fx = (float)x / PP_AMP_TEX_BORDER;
			}
			if (x > PP_AMP_TEX_SIZE - PP_AMP_TEX_BORDER)
			{
				fx = (PP_AMP_TEX_SIZE - (float)x) / PP_AMP_TEX_BORDER;
			}

			if (y < PP_AMP_TEX_BORDER)
			{
				fy = (float)y / PP_AMP_TEX_BORDER;
			}
			if (y > PP_AMP_TEX_SIZE - PP_AMP_TEX_BORDER)
			{
				fy = (PP_AMP_TEX_SIZE - (float)y) / PP_AMP_TEX_BORDER;
			}

			if (fx < fy)
			{
				fy = fx;
			}

			pp_edge_tex[i  ] = fy * 255;
			pp_edge_tex[i+1] = 0;
			pp_edge_tex[i+2] = 0;
		}
	}

	GL_Bind(scenepp_texture_edge);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, PP_WARP_TEX_SIZE, PP_WARP_TEX_SIZE, 0, GL_RGB, GL_UNSIGNED_BYTE, pp_edge_tex);
}

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox (vec3_t mins, vec3_t maxs)
{
	int		i;

	for (i=0 ; i<4 ; i++)
		if (BOX_ON_PLANE_SIDE (mins, maxs, &frustum[i]) == 2)
			return true;
	return false;
}

qboolean R_CullSphere (vec3_t org, float radius)
{
	//four frustrum planes all point inwards in an expanding 'cone'.
	int		i;
	float d;

	for (i=0 ; i<4 ; i++)
	{
		d = DotProduct(frustum[i].normal, org)-frustum[i].dist;
		if (d <= -radius)
			return true;
	}
	return false;
}


void R_RotateForEntity (entity_t *e)
{
	float m[16];
	m[0] = e->axis[0][0];
	m[1] = e->axis[0][1];
	m[2] = e->axis[0][2];
	m[3] = 0;

	m[4] = e->axis[1][0];
	m[5] = e->axis[1][1];
	m[6] = e->axis[1][2];
	m[7] = 0;

	m[8] = e->axis[2][0];
	m[9] = e->axis[2][1];
	m[10] = e->axis[2][2];
	m[11] = 0;

	m[12] = e->origin[0];
	m[13] = e->origin[1];
	m[14] = e->origin[2];
	m[15] = 1;



#if 1
#if 0
	{
		void Matrix4_Multiply(float *a, float *b, float *out);
		float new[16];
	Matrix4_Multiply(m, r_world_matrix, new);
	qglLoadMatrixf(new);
	}
#endif
	qglMultMatrixf(m);

#else
	qglTranslatef (e->origin[0],  e->origin[1],  e->origin[2]);
    qglRotatef (e->angles[1],  0, 0, 1);
    qglRotatef (-e->angles[0],  0, 1, 0);
	//ZOID: fixed z angle
    qglRotatef (e->angles[2],  1, 0, 0);
#endif
}

/*
=============================================================

  SPRITE MODELS

=============================================================
*/

/*
================
R_GetSpriteFrame
================
*/
mspriteframe_t *R_GetSpriteFrame (entity_t *currententity)
{
	msprite_t		*psprite;
	mspritegroup_t	*pspritegroup;
	mspriteframe_t	*pspriteframe;
	int				i, numframes, frame;
	float			*pintervals, fullinterval, targettime, time;

	psprite = currententity->model->cache.data;
	frame = currententity->frame;

	if ((frame >= psprite->numframes) || (frame < 0))
	{
		Con_Printf ("R_DrawSprite: no such frame %d (%s)\n", frame, currententity->model->name);
		frame = 0;
	}

	if (psprite->frames[frame].type == SPR_SINGLE)
	{
		pspriteframe = psprite->frames[frame].frameptr;
	}
	else if (psprite->frames[frame].type == SPR_ANGLED)
	{
		pspritegroup = (mspritegroup_t *)psprite->frames[frame].frameptr;
		pspriteframe = pspritegroup->frames[(int)((r_refdef.viewangles[1]-currententity->angles[1])/360*8 + 0.5-4)&7];
	}
	else
	{
		pspritegroup = (mspritegroup_t *)psprite->frames[frame].frameptr;
		pintervals = pspritegroup->intervals;
		numframes = pspritegroup->numframes;
		fullinterval = pintervals[numframes-1];

		time = cl.time + currententity->syncbase;

	// when loading in Mod_LoadSpriteGroup, we guaranteed all interval values
	// are positive, so we don't have to worry about division by 0
		targettime = time - ((int)(time / fullinterval)) * fullinterval;

		for (i=0 ; i<(numframes-1) ; i++)
		{
			if (pintervals[i] > targettime)
				break;
		}

		pspriteframe = pspritegroup->frames[i];
	}

	return pspriteframe;
}


/*
=================
R_DrawSpriteModel

=================
*/
void R_DrawSpriteModel (entity_t *e)
{
	vec3_t	point;
	mspriteframe_t	*frame;
	vec3_t		forward, right, up;
	msprite_t		*psprite;

	// don't even bother culling, because it's just a single
	// polygon without a surface cache
	frame = R_GetSpriteFrame (e);
	psprite = currententity->model->cache.data;
//	frame = 0x05b94140;

	if (psprite->type == SPR_ORIENTED)
	{	// bullet marks on walls
		AngleVectors (currententity->angles, forward, right, up);
	}
	else if (psprite->type == SPR_FACING_UPRIGHT)
	{
		up[0] = 0;up[1] = 0;up[2]=1;
		right[0] = e->origin[1] - r_origin[1];
		right[1] = -(e->origin[0] - r_origin[0]);
		right[2] = 0;
		VectorNormalize (right);
	}
	else if (psprite->type == SPR_VP_PARALLEL_UPRIGHT)
	{
		up[0] = 0;up[1] = 0;up[2]=1;
		VectorCopy (vright, right);
	}
	else
	{	// normal sprite
		VectorCopy(vup, up);
		VectorCopy(vright, right);
	}
	up[0]*=currententity->scale;
	up[1]*=currententity->scale;
	up[2]*=currententity->scale;
	right[0]*=currententity->scale;
	right[1]*=currententity->scale;
	right[2]*=currententity->scale;

	qglColor4f (1,1,1, e->alpha);

	GL_DisableMultitexture();

    GL_Bind(frame->gl_texturenum);

	if (e->alpha<1)
	{
		qglEnable(GL_BLEND);
		qglTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	}
	else
		qglEnable (GL_ALPHA_TEST);

	qglDisable(GL_CULL_FACE);
	qglBegin (GL_QUADS);

	qglTexCoord2f (0, 1);
	VectorMA (e->origin, frame->down, up, point);
	VectorMA (point, frame->left, right, point);
	qglVertex3fv (point);

	qglTexCoord2f (0, 0);
	VectorMA (e->origin, frame->up, up, point);
	VectorMA (point, frame->left, right, point);
	qglVertex3fv (point);

	qglTexCoord2f (1, 0);
	VectorMA (e->origin, frame->up, up, point);
	VectorMA (point, frame->right, right, point);
	qglVertex3fv (point);

	qglTexCoord2f (1, 1);
	VectorMA (e->origin, frame->down, up, point);
	VectorMA (point, frame->right, right, point);
	qglVertex3fv (point);
	
	qglEnd ();

	qglDisable(GL_BLEND);
	qglDisable (GL_ALPHA_TEST);
}
#if 0
extern int gldepthfunc;
typedef struct decal_s {
	vec3_t origin;
	vec3_t normal;
	int modelindex;
	float endtime;
	float starttime;
	float size;
	struct decal_s *next;
} decal_t;
decal_t *firstdecal;
void vectoangles(vec3_t vec, vec3_t ang);
void R_DrawDecals(void)
{
//	vec3_t	point;
//	vec3_t		right, up;

	entity_t ent;
	extern int cl_spikeindex;
	extern model_t	mod_known[];

	decal_t *dec = firstdecal;	
//	glDisable(GL_TEXTURE_2D);
	glDisable (GL_ALPHA_TEST);
	glEnable (GL_BLEND);
//	glDepthFunc(GL_LEQUAL);
//	glDisable(GL_CULL_TEST);
//	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	GL_Bind(particletexture);
//	glDepthMask(0);
//	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(0, 0, 0, 0.5);



//   glClearStencil(0x0);
//   glEnable(GL_STENCIL_TEST);


memset(&ent, 0, sizeof(ent));
	
	while(dec)
	{
//		if (dec->modelindex)
		{
			ent.origin[0] = dec->origin[0];
			ent.origin[1] = dec->origin[1];
			ent.origin[2] = dec->origin[2];
			ent.angles[0] = -dec->normal[0];
			ent.angles[1] = -dec->normal[1];
			ent.angles[2] = -dec->normal[2];
			vectoangles(ent.angles, ent.angles);
			ent.model = &mod_known[cl_spikeindex];//dec->modelindex;
			currententity = &ent;
			switch(currententity->model->type)
			{
			case mod_alias:
				R_DrawAliasModel(currententity);
				break;
			case mod_alias3:
				R_DrawAlias3Model(currententity);
				break;
			}
			dec = dec->next;
			continue;
		}
/*
		PerpendicularVector(up, dec->normal);
		CrossProduct(dec->normal, up, right);

#if 0
   glClear(GL_STENCIL_BUFFER_BIT);
   glStencilFunc (GL_ALWAYS, 0x1, 0x1);
   glStencilOp (GL_REPLACE, GL_REPLACE, GL_REPLACE);
   glBegin(GL_QUADS);
      glVertex2f (-1.0, 0.0);
      glVertex2f (0.0, 1.0);
      glVertex2f (1.0, 0.0);
      glVertex2f (0.0, -1.0);
	glEnd();


		
		glStencilFunc (GL_EQUAL, 0x1, 0x1);	//where we drew to the stencil buffer.
		glStencilOp (GL_ZERO, GL_KEEP, GL_KEEP);
#endif


//		glColor4f(1, 1, 1, (dec->starttime-dec->endtime) * (cl.time-dec->starttime));		

		glBegin (GL_QUADS);	

		glTexCoord2f (0, 0.5);
		VectorMA (dec->origin, dec->size, up, point);
		VectorMA (point, -dec->size, right, point);
		glVertex3fv (point);

		glTexCoord2f (0, 0);
		VectorMA (dec->origin, -dec->size, up, point);
		VectorMA (point, -dec->size, right, point);
		glVertex3fv (point);

		glTexCoord2f (0.5, 0);
		VectorMA (dec->origin, -dec->size, up, point);
		VectorMA (point, dec->size, right, point);
		glVertex3fv (point);

		glTexCoord2f (0.5, 0.5);
		VectorMA (dec->origin, dec->size, up, point);
		VectorMA (point, dec->size, right, point);
		glVertex3fv (point);
		glEnd ();

		dec = dec->next;
		*/
	}	
//	glDisable(GL_STENCIL_TEST);
//	glDepthMask(1);
	glEnable(GL_TEXTURE_2D);
	glDisable (GL_BLEND);

//	glDepthFunc(gldepthfunc);
}

void TraceLineN (vec3_t start, vec3_t end, vec3_t impact, vec3_t normal);
void GLR_AddDecals(vec3_t org)
{
	decal_t *dec;
	vec3_t end, impact, norm;
	vec3_t dir[] = {
		{0, 0, 10},
		{0, 0, -10},
		{0, 10, 0},
		{0, -10, 0},
		{10, 0, 0},
		{-10, 0, 0}
	};
	int i;
#define STOP_EPSILON 0.01

	return;

	for (i = 0; i < 6; i++)
	{
		VectorAdd(org, dir[i], end);
		TraceLineN(org, end, impact, norm);

		if (!((end[0]==impact[0] && end[1]==impact[1] && end[2]==impact[2]) || (!impact[0] && !impact[1] && !impact[2])))
		{
			dec = Z_Malloc(sizeof(decal_t));			
			VectorCopy(norm, dec->normal);
//			VectorCopy(impact, dec->origin);
			VectorMA(impact, STOP_EPSILON, norm, dec->origin);			
			dec->next = firstdecal;
			firstdecal = dec;
		}
	}
}
#endif


//==================================================================================

/*
=============
R_DrawEntitiesOnList
=============
*/
void GLR_DrawEntitiesOnList (void)
{
	int		i;

	if (!r_drawentities.value)
		return;

	// draw sprites seperately, because of alpha blending
	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		currententity = &cl_visedicts[i];

		if (cl.viewentity[r_refdef.currentplayernum] && currententity->keynum == cl.viewentity[r_refdef.currentplayernum])
			continue;

		if (!Cam_DrawPlayer(0, currententity->keynum-1))
			continue;

		if (!currententity->model)
			continue;

		if (cls.allow_anyparticles || currententity->visframe)	//allowed or static
		{
			if (currententity->model->particleeffect>=0)
			{
				if (currententity->model->particleengulphs)
				{
					if (gl_part_flame.value)
					{
						R_TorchEffect(currententity->origin, currententity->model->particleeffect);
						currententity->model = NULL;
						continue;
					}
				}
				else
				{
					if (gl_part_torch.value)
					{
						R_TorchEffect(currententity->origin, currententity->model->particleeffect);
					}
				}
			}
		}

		switch (currententity->model->type)
		{
		case mod_alias:
			if (r_refdef.flags & 1 || !cl.worldmodel || cl.worldmodel->fromgame == fg_doom)
				R_DrawGAliasModel (currententity);
			break;
		
#ifdef HALFLIFEMODELS
		case mod_halflife:
			R_DrawHLModel (currententity);
			break;
#endif

		case mod_brush:
			if (cl.worldmodel->fromgame == fg_doom)
				PPL_BaseBModelTextures (currententity);
			break;

		default:
			break;
		}
	}

	for (i=0 ; i<cl_numvisedicts ; i++)
	{
		currententity = &cl_visedicts[i];

		if (cl.viewentity[r_refdef.currentplayernum] && currententity->keynum == cl.viewentity[r_refdef.currentplayernum])
			continue;

		if (!currententity->model)
			continue;

		if (cls.allow_anyparticles || currententity->visframe)	//allowed or static
		{
			if (currententity->model->particleeffect>=0)
			{
				if (currententity->model->particleengulphs)
				{
					if (gl_part_flame.value)
					{
						continue;
					}
				}
			}
		}


		switch (currententity->model->type)
		{
		case mod_sprite:
			R_DrawSpriteModel (currententity);
			break;

		default :
			break;
		}
	}
}

/*
=============
R_DrawViewModel
=============
*/
void GLR_DrawViewModel (void)
{
//	float		ambient[4], diffuse[4];
//	int			j;
//	int			lnum;
//	vec3_t		dist;
//	float		add;
//	dlight_t	*dl;
//	int			ambientlight, shadelight;

	static struct model_s *oldmodel[MAX_SPLITS];	
	static float lerptime[MAX_SPLITS];
	static int prevframe[MAX_SPLITS];

#ifdef SIDEVIEWS
	extern qboolean r_secondaryview;
	if (r_secondaryview==1)
		return;
#endif

	if (!r_drawviewmodel.value || !Cam_DrawViewModel(r_refdef.currentplayernum))
		return;

	if (envmap)
		return;

#ifdef Q2CLIENT
	if (cls.q2server)
		return;
#endif

	if (!r_drawentities.value)
		return;

	if (cl.stats[r_refdef.currentplayernum][STAT_ITEMS] & IT_INVISIBILITY)
		return;

	if (cl.stats[r_refdef.currentplayernum][STAT_HEALTH] <= 0)
		return;

	currententity = &cl.viewent[r_refdef.currentplayernum];
	if (!currententity->model)
		return;

//	if (cls.allow_anyparticles || currententity->visframe)	//allowed or static
	{
		if (currententity->model->particleeffect>=0)
		{
			if (currententity->model->particleengulphs)
			{
				if (gl_part_flame.value)
				{
					R_TorchEffect(currententity->origin, currententity->model->particleeffect);
					currententity->model = NULL;
					return;
				}
			}
			else
			{
				if (gl_part_torch.value)
				{
					R_TorchEffect(currententity->origin, currententity->model->particleeffect);
				}
			}
		}
	}



#ifdef PEXT_SCALE
	currententity->scale = 1;
#endif
	if (r_drawviewmodel.value > 0 && r_drawviewmodel.value < 1)
		currententity->alpha = r_drawviewmodel.value;
	else
		currententity->alpha = 1;

	if (currententity->frame != prevframe[r_refdef.currentplayernum])
	{
		currententity->oldframe = prevframe[r_refdef.currentplayernum];
		lerptime[r_refdef.currentplayernum] = realtime;
	}
	prevframe[r_refdef.currentplayernum] = currententity->frame;

	if (currententity->model != oldmodel[r_refdef.currentplayernum])
	{
		oldmodel[r_refdef.currentplayernum] = currententity->model;
		currententity->oldframe = currententity->frame;
		lerptime[r_refdef.currentplayernum] = realtime;
	}
	currententity->lerptime = 1-(realtime-lerptime[r_refdef.currentplayernum])*10;
	if (currententity->lerptime<0)currententity->lerptime=0;
	if (currententity->lerptime>1)currententity->lerptime=1;


	currententity->flags = Q2RF_WEAPONMODEL|Q2RF_DEPTHHACK;

	switch(currententity->model->type)
	{
	case mod_sprite:
		R_DrawSpriteModel (currententity);
		break;

	case mod_alias:
		R_DrawGAliasModel (currententity);
		break;

#ifdef HALFLIFEMODELS
	case mod_halflife:
		R_DrawHLModel (currententity);
		break;
#else
	case mod_halflife:	//no gcc warning please
		break;
#endif

		//we don't support these as view models
	case mod_brush:
	case mod_dummy:
		break;
	}
}


/*
============
R_PolyBlend
============
*/
void R_PolyBlend (void)
{
	extern qboolean gammaworks;
	if (!v_blend[3] || gammaworks)
		return;

//Con_Printf("R_PolyBlend(): %4.2f %4.2f %4.2f %4.2f\n",v_blend[0], v_blend[1],	v_blend[2],	v_blend[3]);

 	GL_DisableMultitexture();

	qglDisable (GL_ALPHA_TEST);
	qglEnable (GL_BLEND);
	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_TEXTURE_2D);

    qglLoadIdentity ();

    qglRotatef (-90,  1, 0, 0);	    // put Z going up
    qglRotatef (90,  0, 0, 1);	    // put Z going up

	qglColor4fv (v_blend);

	qglBegin (GL_QUADS);

	qglVertex3f (10, 100, 100);
	qglVertex3f (10, -100, 100);
	qglVertex3f (10, -100, -100);
	qglVertex3f (10, 100, -100);
	qglEnd ();

	qglDisable (GL_BLEND);
	qglEnable (GL_TEXTURE_2D);
	qglEnable (GL_ALPHA_TEST);
}

void GLR_BrightenScreen (void)
{
	extern float vid_gamma;
	float f;

	RSpeedMark();

	if (gl_contrast.value <= 1.0)
		return;

	f = gl_contrast.value;
	f = min (f, 3);

	f = pow (f, vid_gamma);
	
	qglDisable (GL_TEXTURE_2D);
	qglEnable (GL_BLEND);
	qglBlendFunc (GL_DST_COLOR, GL_ONE);
	qglBegin (GL_QUADS);
	while (f > 1) {
		if (f >= 2)
			qglColor3f (1,1,1);
		else
			qglColor3f (f - 1, f - 1, f - 1);
		qglVertex2f (0, 0);
		qglVertex2f (vid.width, 0);
		qglVertex2f (vid.width, vid.height);
		qglVertex2f (0, vid.height);
		f *= 0.5;
	}
	qglEnd ();
	qglBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglEnable (GL_TEXTURE_2D);
	qglDisable (GL_BLEND);
	qglColor3f(1, 1, 1);

	RSpeedEnd(RSPEED_PALETTEFLASHES);
}

int SignbitsForPlane (mplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}


void R_SetFrustum (void)
{
	int		i;

	if ((int)r_novis.value & 4)
		return;

	if (r_refdef.fov_x == 90) 
	{
		// front side is visible

		VectorAdd (vpn, vright, frustum[0].normal);
		VectorSubtract (vpn, vright, frustum[1].normal);

		VectorAdd (vpn, vup, frustum[2].normal);
		VectorSubtract (vpn, vup, frustum[3].normal);
	}
	else
	{

		// rotate VPN right by FOV_X/2 degrees
		RotatePointAroundVector( frustum[0].normal, vup, vpn, -(90-r_refdef.fov_x / 2 ) );
		// rotate VPN left by FOV_X/2 degrees
		RotatePointAroundVector( frustum[1].normal, vup, vpn, 90-r_refdef.fov_x / 2 );
		// rotate VPN up by FOV_X/2 degrees
		RotatePointAroundVector( frustum[2].normal, vright, vpn, 90-r_refdef.fov_y / 2 );
		// rotate VPN down by FOV_X/2 degrees
		RotatePointAroundVector( frustum[3].normal, vright, vpn, -( 90 - r_refdef.fov_y / 2 ) );
	}

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane (&frustum[i]);
	}
}

/*
===============
R_SetupFrame
===============
*/
void GLR_SetupFrame (void)
{
// don't allow cheats in multiplayer
	r_wateralphaval = r_wateralpha.value;
	if (!cls.allow_watervis)
		r_wateralphaval = 1;

	GLR_AnimateLight ();

	r_framecount++;

// build the transformation matrix for the given view angles
	VectorCopy (r_refdef.vieworg, r_origin);

	AngleVectors (r_refdef.viewangles, vpn, vright, vup);

// current viewleaf
	if (r_refdef.flags & 1)
	{
	}
#ifdef Q2BSPS
	else if (cl.worldmodel && (cl.worldmodel->fromgame == fg_quake2 || cl.worldmodel->fromgame == fg_quake3))
	{
		static mleaf_t fakeleaf;
		mleaf_t	*leaf;

		r_viewleaf = &fakeleaf;	//so we can use quake1 rendering routines for q2 bsps.
		r_viewleaf->contents = Q1CONTENTS_EMPTY;
		r_viewleaf2 = NULL;

		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf = GLMod_PointInLeaf (r_origin, cl.worldmodel);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents)
		{	// look down a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] -= 16;
			leaf = GLMod_PointInLeaf (temp, cl.worldmodel);
			if ( !(leaf->contents & Q2CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
		else
		{	// look up a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] += 16;
			leaf = GLMod_PointInLeaf (temp, cl.worldmodel);
			if ( !(leaf->contents & Q2CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
	}
#endif
	else
	{
		mleaf_t	*leaf;
		vec3_t	temp;

		r_oldviewleaf = r_viewleaf;
		r_oldviewleaf2 = r_viewleaf2;
		r_viewleaf = GLMod_PointInLeaf (r_origin, cl.worldmodel);

		if (!r_viewleaf)
		{
		}
		else if (r_viewleaf->contents == Q1CONTENTS_EMPTY)
		{	//look down a bit			
			VectorCopy (r_origin, temp);
			temp[2] -= 16;
			leaf = GLMod_PointInLeaf (temp, cl.worldmodel);
			if (leaf->contents <= Q1CONTENTS_WATER && leaf->contents >= Q1CONTENTS_LAVA)
				r_viewleaf2 = leaf;
			else
				r_viewleaf2 = NULL;
		}
		else if (r_viewleaf->contents <= Q1CONTENTS_WATER && r_viewleaf->contents >= Q1CONTENTS_LAVA)
		{	//in water, look up a bit.
		
			VectorCopy (r_origin, temp);
			temp[2] += 16;
			leaf = GLMod_PointInLeaf (temp, cl.worldmodel);
			if (leaf->contents == Q1CONTENTS_EMPTY)
				r_viewleaf2 = leaf;
			else
				r_viewleaf2 = NULL;
		}
		else
			r_viewleaf2 = NULL;
		
		if (r_viewleaf)
			V_SetContentsColor (r_viewleaf->contents);
	}
	GLV_CalcBlend ();

	r_cache_thrash = false;

	c_brush_polys = 0;
	c_alias_polys = 0;

}


void MYgluPerspective( GLdouble fovy, GLdouble aspect,
		     GLdouble zNear, GLdouble zFar )
{
#if 1	//for the sake of the d3d...
#else
	GLfloat matrix[16];
#endif
	GLdouble xmin, xmax, ymin, ymax;

	ymax = zNear * tan( fovy * M_PI / 360.0 );
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

#if 1	//for the sake of the d3d...
	qglFrustum( xmin, xmax, ymin, ymax, zNear, zFar );
#else

	matrix[0] = (2*zNear) / (xmax - xmin);
	matrix[4] = 0;
	matrix[8] = (xmax + xmin) / (xmax - xmin);
	matrix[12] = 0;

	matrix[1] = 0;
	matrix[5] = (2*zNear) / (ymax - ymin);
	matrix[9] = (ymax + ymin) / (ymax - ymin);
	matrix[13] = 0;

	matrix[2] = 0;
	matrix[6] = 0;
	matrix[10] = - (zFar+zNear)/(zFar-zNear);
	matrix[14] = - (2.0f*zFar*zNear)/(zFar-zNear);
	
	matrix[3] = 0;
	matrix[7] = 0;
	matrix[11] = -1;
	matrix[15] = 0;

	qglMultMatrixf(matrix);
#endif
}

void GL_InfinatePerspective( GLdouble fovy, GLdouble aspect,
		     GLdouble zNear)
{
	GLfloat matrix[16];

	// nudge infinity in just slightly for lsb slop
    GLfloat nudge = 1;// - 1.0 / (1<<23);

	GLdouble xmin, xmax, ymin, ymax;

	ymax = zNear * tan( fovy * M_PI / 360.0 );
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

	matrix[0] = (2*zNear) / (xmax - xmin);
	matrix[4] = 0;
	matrix[8] = (xmax + xmin) / (xmax - xmin);
	matrix[12] = 0;

	matrix[1] = 0;
	matrix[5] = (2*zNear) / (ymax - ymin);
	matrix[9] = (ymax + ymin) / (ymax - ymin);
	matrix[13] = 0;

	matrix[2] = 0;
	matrix[6] = 0;
	matrix[10] = -1  * nudge;
	matrix[14] = -2*zNear * nudge;
	
	matrix[3] = 0;
	matrix[7] = 0;
	matrix[11] = -1;
	matrix[15] = 0;

	qglMultMatrixf(matrix);
}


/*
=============
R_SetupGL
=============
*/
void R_SetupGL (void)
{
	float	screenaspect;
	extern	int glwidth, glheight;
	int		x, x2, y2, y, w, h;

	//
	// set up viewpoint
	//
	x = r_refdef.vrect.x * glwidth/vid.width;
	x2 = (r_refdef.vrect.x + r_refdef.vrect.width) * glwidth/vid.width;
	y = (vid.height-r_refdef.vrect.y) * glheight/vid.height;
	y2 = ((int)vid.height - (r_refdef.vrect.y + r_refdef.vrect.height)) * glheight/(int)vid.height;

	// fudge around because of frac screen scale
	if (x > 0)
		x--;
	if (x2 < glwidth)
		x2++;
	if (y2 < 0)
		y2--;
	if (y < glheight)
		y++;

	w = x2 - x;
	h = y - y2;

	if (envmap)
	{
		x = y2 = 0;
		w = h = 256;
	}

	qglViewport (glx + x, gly + y2, w, h);

	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity ();

	screenaspect = (float)r_refdef.vrect.width/r_refdef.vrect.height;
	if (!r_shadows.value || !gl_canstencil)//gl_nv_range_clamp)
	{
//		yfov = 2*atan((float)r_refdef.vrect.height/r_refdef.vrect.width)*180/M_PI;
//		yfov = (2.0 * tan (scr_fov.value/360*M_PI)) / screenaspect;
//		yfov = 2*atan((float)r_refdef.vrect.height/r_refdef.vrect.width)*(scr_fov.value*2)/M_PI;
//		MYgluPerspective (yfov,  screenaspect,  4,  4096);
		MYgluPerspective (r_refdef.fov_y,  screenaspect,  4,  gl_maxdist.value);
	}
	else
	{
		GL_InfinatePerspective(r_refdef.fov_y, screenaspect, 4);
	}

	if (mirror)
	{
		if (mirror_plane->normal[2])
			qglScalef (1, -1, 1);
		else
			qglScalef (-1, 1, 1);
		qglCullFace(GL_BACK);
	}
	else
		qglCullFace(GL_FRONT);

	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity ();

    qglRotatef (-90,  1, 0, 0);	    // put Z going up
    qglRotatef (90,  0, 0, 1);	    // put Z going up

#ifdef R_XFLIP
	if (r_xflip.value)
	{
		qglScalef (1, -1, 1);
		qglCullFace(GL_BACK);
	}
#endif


    qglRotatef (-r_refdef.viewangles[2],  1, 0, 0);
    qglRotatef (-r_refdef.viewangles[0],  0, 1, 0);
    qglRotatef (-r_refdef.viewangles[1],  0, 0, 1);
    qglTranslatef (-r_refdef.vieworg[0],  -r_refdef.vieworg[1],  -r_refdef.vieworg[2]);

	qglGetFloatv (GL_MODELVIEW_MATRIX, r_world_matrix);

	//
	// set drawing parms
	//
	if (gl_cull.value)
		qglEnable(GL_CULL_FACE);
	else
		qglDisable(GL_CULL_FACE);

	qglDisable(GL_BLEND);
	qglDisable(GL_ALPHA_TEST);
	qglEnable(GL_DEPTH_TEST);

//#ifndef D3DQUAKE
//	glClearDepth(1.0f);
//#endif

//		if (gl_lightmap_format == GL_LUMINANCE)
//		glBlendFunc (GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
/*	else if (gl_lightmap_format == GL_INTENSITY)
	{
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glColor4f (0,0,0,1);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else if (gl_lightmap_format == GL_RGBA)
	{
		glBlendFunc (GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
	}

  */

	if (gl_dither.value)
	{
		qglEnable(GL_DITHER);
	}
	else
	{
		qglDisable(GL_DITHER);
	}

	GL_DisableMultitexture();
}

/*
================
R_RenderScene

r_refdef must be set before the first call
================
*/
void R_RenderScene (void)
{
	qboolean GLR_DoomWorld(void);
	if (!mirror)
	GLR_SetupFrame ();

	TRACE(("dbg: calling R_SetFrustrum\n"));
	R_SetFrustum ();

	TRACE(("dbg: calling R_SetupGL\n"));
	R_SetupGL ();

	if (!(r_refdef.flags & 1))
	{
#ifdef DOOMWADS
		if (!GLR_DoomWorld ())
#endif
		{
			TRACE(("dbg: calling GLR_MarkLeaves\n"));
			GLR_MarkLeaves ();	// done here so we know if we're in water
			TRACE(("dbg: calling R_DrawWorld\n"));
			R_DrawWorld ();		// adds static entities to the list
		}
	}

	S_ExtraUpdate ();	// don't let sound get messed up if going slow

	TRACE(("dbg: calling GLR_DrawEntitiesOnList\n"));
	GLR_DrawEntitiesOnList ();

//	R_DrawDecals();

	TRACE(("dbg: calling GL_DisableMultitexture\n"));
	GL_DisableMultitexture();

	TRACE(("dbg: calling R_RenderDlights\n"));
	R_RenderDlights ();

	if (cl.worldmodel)
	{
		TRACE(("dbg: calling R_DrawParticles\n"));
		R_DrawParticles ();
	}

#ifdef GLTEST
	Test_Draw ();
#endif

}


/*
=============
R_Clear
=============
*/
int gldepthfunc;
void R_Clear (void)
{
	if (r_mirroralpha.value != 1.0)
	{
		if (gl_clear.value && !r_secondaryview)
			qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		else
			qglClear (GL_DEPTH_BUFFER_BIT);
		gldepthmin = 0;
		gldepthmax = 0.5;
		qglDepthFunc (gldepthfunc=GL_LEQUAL);
	}
#ifdef SIDEVIEWS
	else if (gl_ztrick.value && !gl_ztrickdisabled)
#else
	else if (gl_ztrick.value)
#endif
	{
		static int trickframe;

		if (gl_clear.value && !(r_refdef.flags & 1))
			qglClear (GL_COLOR_BUFFER_BIT);

		trickframe++;
		if (trickframe & 1)
		{
			gldepthmin = 0;
			gldepthmax = 0.49999;
			qglDepthFunc (gldepthfunc=GL_LEQUAL);
		}
		else
		{
			gldepthmin = 1;
			gldepthmax = 0.5;
			qglDepthFunc (gldepthfunc=GL_GEQUAL);
		}
	}
	else
	{
		if (gl_clear.value && !r_secondaryview)
			qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		else
			qglClear (GL_DEPTH_BUFFER_BIT);
		gldepthmin = 0;
		gldepthmax = 1;
		qglDepthFunc (gldepthfunc=GL_LEQUAL);
	}

	qglDepthRange (gldepthmin, gldepthmax);
}

//#if 0 //!!! FIXME, Zoid, mirror is disabled for now
/*
=============
R_Mirror
=============
*/

void CL_AddFlagModels (entity_t *ent, int team);
void R_MirrorAddPlayerModels (void)
{
	extern int cl_playerindex;
	extern cvar_t cl_predict_players, cl_predict_players2;
	player_state_t	*state;
	player_state_t	exact;
	player_info_t	*info=cl.players + cl.playernum[0];
	double			playertime;
	entity_t		*ent;
	int				msec;
	frame_t			*frame;
	int				oldphysent;
	extern cvar_t spectator;

	playertime = realtime - cls.latency + 0.02;
	if (playertime > realtime)
		playertime = realtime;

	frame = &cl.frames[cl.parsecount&UPDATE_MASK];

	state=&frame->playerstate[cl.playernum[0]];

	if (!state->modelindex || spectator.value)
		return;

	ent = &cl_visedicts[cl_numvisedicts];
	cl_numvisedicts++;

	ent->keynum = cl.playernum[0]+1;

	ent->model = cl.model_precache[state->modelindex];
	ent->skinnum = state->skinnum;
	ent->frame = state->frame;
	ent->oldframe = state->oldframe;
	if (state->lerpstarttime)
	{
		ent->lerptime = 1-(realtime - state->lerpstarttime)*10;
		if (ent->lerptime < 0)
			ent->lerptime = 0;
	}
	else
		ent->lerptime = 0;

	ent->colormap = cl.players[cl.playernum[0]].translations;
	if (state->modelindex == cl_playerindex)
		ent->scoreboard = &cl.players[cl.playernum[0]];		// use custom skin
	else
		ent->scoreboard = NULL;

#ifdef PEXT_SCALE
	ent->scale = state->scale;
	if (!ent->scale)
		ent->scale = 1;
#endif
#ifdef PEXT_TRANS
	ent->alpha = state->trans;
	if (!ent->alpha)
		ent->alpha = 1;
#endif

	//
	// angles
	//
	ent->angles[PITCH] = -r_refdef.viewangles[PITCH]/3;
	ent->angles[YAW] = r_refdef.viewangles[YAW];
//	ent->angles[ROLL] = 0;
	ent->angles[ROLL] = V_CalcRoll (ent->angles, state->velocity)*4;
	AngleVectors(ent->angles, ent->axis[0], ent->axis[1], ent->axis[2]);
	VectorInverse(ent->axis[1]);

	// only predict half the move to minimize overruns
	msec = 500*(playertime - state->state_time);
	if (msec <= 0 || (!cl_predict_players.value && !cl_predict_players2.value))
	{
		VectorCopy (state->origin, ent->origin);
//Con_DPrintf ("nopredict\n");
	}
	else
	{
		// predict players movement
		if (msec > 255)
			msec = 255;
		state->command.msec = msec;
//Con_DPrintf ("predict: %i\n", msec);

		oldphysent = pmove.numphysent;
		CL_SetSolidPlayers (cl.playernum[0]);
		CL_PredictUsercmd (0, state, &exact, &state->command);
		pmove.numphysent = oldphysent;
		VectorCopy (exact.origin, ent->origin);
	}

	VectorCopy(cl.simorg[0], ent->origin);

	if (state->effects & EF_FLAG1)
		CL_AddFlagModels (ent, 0);
	else if (state->effects & EF_FLAG2)
		CL_AddFlagModels (ent, 1);

	if (info->vweapindex)
		CL_AddVWeapModel(ent, info->vweapindex);

}


void R_Mirror (void)
{
	float		d;
	msurface_t	*s;
//	entity_t	*ent;

	int oldvisents;
	vec3_t oldangles, oldorg;	//cache - for rear view mirror and stuff.

	if (!mirror)
		return;

	oldvisents = cl_numvisedicts;
	R_MirrorAddPlayerModels();	//we need to add the player model. Invisible in mirror otherwise.

	memcpy(oldangles, r_refdef.viewangles, sizeof(vec3_t));
	memcpy(oldorg, r_refdef.vieworg, sizeof(vec3_t));
	
	memcpy (r_base_world_matrix, r_world_matrix, sizeof(r_base_world_matrix));

	d = DotProduct (r_refdef.vieworg, mirror_plane->normal) - mirror_plane->dist;
	VectorMA (r_refdef.vieworg, -2*d, mirror_plane->normal, r_refdef.vieworg);
	memcpy(r_origin, r_refdef.vieworg, sizeof(vec3_t));	

	d = DotProduct (vpn, mirror_plane->normal);
	VectorMA (vpn, -2*d, mirror_plane->normal, vpn);

	r_refdef.viewangles[0] = -asin (vpn[2])/M_PI*180;
	r_refdef.viewangles[1] = atan2 (vpn[1], vpn[0])/M_PI*180;
	r_refdef.viewangles[2] = -r_refdef.viewangles[2];	
/*
	r_refdef.vieworg[0] = 400;
	r_refdef.vieworg[1] = 575;
	r_refdef.vieworg[2] = 64;
*/

	AngleVectors (r_refdef.viewangles, vpn, vright, vup);


	gldepthmin = 0.5;
	gldepthmax = 1;
	qglDepthRange (gldepthmin, gldepthmax);
	qglDepthFunc (GL_LEQUAL);

	R_RenderScene ();

	GLR_DrawWaterSurfaces ();


	gldepthmin = 0;
	gldepthmax = 0.5;
	qglDepthRange (gldepthmin, gldepthmax);
	qglDepthFunc (GL_LEQUAL);	

	
	memcpy(r_refdef.viewangles, oldangles, sizeof(vec3_t));

	qglMatrixMode(GL_PROJECTION);
	if (mirror_plane->normal[2])
		qglScalef (1,-1,1);
	else
		qglScalef (-1,1,1);
	qglCullFace(GL_FRONT);
	qglMatrixMode(GL_MODELVIEW);

	qglLoadMatrixf (r_base_world_matrix);

	

	// blend on top
	qglDisable(GL_ALPHA_TEST);
	qglEnable (GL_BLEND);
	qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	qglColor4f (1,1,1,r_mirroralpha.value);
	s = r_mirror_chain;
	for ( ; s ; s=s->texturechain)
		R_RenderBrushPoly (s);
	cl.worldmodel->textures[mirrortexturenum]->texturechain = NULL;
	qglDisable (GL_BLEND);
	qglColor4f (1,1,1,1);
	
	//put things back for rear views
	qglCullFace(GL_BACK);

	memcpy(r_refdef.viewangles, oldangles, sizeof(vec3_t));
	memcpy(r_refdef.vieworg, oldorg, sizeof(vec3_t));
	cl_numvisedicts = oldvisents;
}
//#endif

/*
================
R_RenderView

r_refdef must be set before the first call
================
*/
void GLR_RenderView (void)
{
	extern msurface_t  *r_alpha_surfaces;
	double	time1 = 0, time2;

	if (r_norefresh.value || !glwidth || !glheight)
	{
		GL_DoSwap();
		return;
	}

	if (!(r_refdef.flags & 1))
		if (!r_worldentity.model || !cl.worldmodel)
		{
			GL_DoSwap();
			return;
		}
//		Sys_Error ("R_RenderView: NULL worldmodel");



	if (qglPNTrianglesiATI)
	{
		if (gl_ati_truform_type.value)
		{	//linear
			qglPNTrianglesiATI(GL_PN_TRIANGLES_NORMAL_MODE_ATI, GL_PN_TRIANGLES_NORMAL_MODE_LINEAR_ATI);
			qglPNTrianglesiATI(GL_PN_TRIANGLES_POINT_MODE_ATI, GL_PN_TRIANGLES_POINT_MODE_CUBIC_ATI);
		}
		else
		{	//quadric
			qglPNTrianglesiATI(GL_PN_TRIANGLES_NORMAL_MODE_ATI, GL_PN_TRIANGLES_NORMAL_MODE_QUADRATIC_ATI);
			qglPNTrianglesiATI(GL_PN_TRIANGLES_POINT_MODE_ATI, GL_PN_TRIANGLES_POINT_MODE_CUBIC_ATI);
		}
	    qglPNTrianglesfATI(GL_PN_TRIANGLES_TESSELATION_LEVEL_ATI, gl_ati_truform_tesselation.value);
	}




	if (gl_finish.value)
	{
		RSpeedMark();
		qglFinish ();
		RSpeedEnd(RSPEED_FINISH);
	}

	if (r_speeds.value)
	{
		time1 = Sys_DoubleTime ();
		c_brush_polys = 0;
		c_alias_polys = 0;
	}

	mirror = false;

	R_Clear ();
/*
	if (r_viewleaf)// && r_viewleaf->contents != CONTENTS_EMPTY)
	{
		//	static fogcolour;
	float fogcol[4]={0};
	float fogperc;
	float fogdist;
#pragma comment (lib, "opengl32.lib")	//temp only.

		fogperc=0;
		fogdist=512;
		switch(r_viewleaf->contents)
		{
		case CONTENTS_WATER:
			fogcol[0] = 64/255.0;
			fogcol[1] = 128/255.0;
			fogcol[2] = 192/255.0;
			fogperc=0.2;
			fogdist=512;
			break;
		case CONTENTS_SLIME:
			fogcol[0] = 32/255.0;
			fogcol[1] = 192/255.0;
			fogcol[2] = 92/255.0;
			fogperc=1;
			fogdist=256;
			break;
		case CONTENTS_LAVA:
			fogcol[0] = 192/255.0;
			fogcol[1] = 32/255.0;
			fogcol[2] = 64/255.0;
			fogperc=1;
			fogdist=128;
			break;
		default:
			fogcol[0] = 192/255.0;
			fogcol[1] = 192/255.0;
			fogcol[2] = 192/255.0;
			fogperc=1;
			fogdist=1024;
			break;
		}
		if (fogperc)
		{
			glFogi(GL_FOG_MODE, GL_LINEAR);
			glFogfv(GL_FOG_COLOR, fogcol);
			glFogf(GL_FOG_DENSITY, fogperc);
			glFogf(GL_FOG_START, 1);
			glFogf(GL_FOG_END, fogdist);
			glEnable(GL_FOG);
		}
	}
*/
	r_alpha_surfaces = NULL;

	// render normal view
	R_RenderScene ();	
	GLR_DrawViewModel ();
	GLR_DrawWaterSurfaces ();
	GLR_DrawAlphaSurfaces ();

	// render mirror view
	R_Mirror ();

	R_PolyBlend ();

//	glDisable(GL_FOG);

	if (r_speeds.value)
	{
//		glFinish ();
		time2 = Sys_DoubleTime ();

		RQuantAdd(RQUANT_MSECS, (int)((time2-time1)*1000000));

		RQuantAdd(RQUANT_WPOLYS, c_brush_polys);
		RQuantAdd(RQUANT_EPOLYS, c_alias_polys);
	//	Con_Printf ("%3i ms  %4i wpoly %4i epoly\n", (int)((time2-time1)*1000), c_brush_polys, c_alias_polys); 
	}

	// SCENE POST PROCESSING
	// we check if we need to use any shaders - currently it's just waterwarp
	if ((gl_config.arb_shader_objects) && (r_waterwarp.value && r_viewleaf && r_viewleaf->contents <= Q1CONTENTS_WATER))
	{
		extern int char_texture;
		float vwidth = 1, vheight = 1;
		float vs, vt;

		// get the powers of 2 for the size of the texture that will hold the scene
		while (vwidth < glwidth)
		{
			vwidth *= 2;
		}
		while (vheight < glheight)
		{
			vheight *= 2;
		}

		if (qglGetError())
			Con_Printf("GL Error before drawing with shaderobjects\n");

		// get the maxtexcoords while we're at it
		vs = glwidth / vwidth;
		vt = glheight / vheight;

		// 2d mode, but upside down to quake's normal 2d drawing
		// this makes grabbing the sreen a lot easier
		qglViewport (glx, gly, glwidth, glheight);

		qglMatrixMode(GL_PROJECTION);
		// Push the matrices to go into 2d mode, that matches opengl's mode
		qglPushMatrix();
		qglLoadIdentity ();
		// TODO: use actual window width and height
		qglOrtho  (0, glwidth, 0, glheight, -99999, 99999);

		qglMatrixMode(GL_MODELVIEW);
		qglPushMatrix();
		qglLoadIdentity ();

		qglDisable (GL_DEPTH_TEST);
		qglDisable (GL_CULL_FACE);
		qglDisable (GL_BLEND);
		qglEnable (GL_ALPHA_TEST);

		// copy the scene to texture
		GL_Bind(scenepp_texture);
		qglCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, glx, gly, vwidth, vheight, 0);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (qglGetError())
			Con_Printf("GL Error after qglCopyTexImage2D\n");

		// Here we apply the shaders - currently just waterwarp
		GLSlang_UseProgram(scenepp_ww_program);
		//keep the amp proportional to the size of the scene in texture coords
		// WARNING - waterwarp can change the amplitude, but if it's too big it'll exceed
		// the size determined by the edge texture, after which black bits will be shown.
		// Suggest clamping to a suitable range.
		GLSlang_SetUniform1f(scenepp_ww_parm_ampscalef, (0.005 / 0.625) * vs*r_waterwarp.value);

		if (qglGetError())
			Con_Printf("GL Error after GLSlang_UseProgram\n");

		{
			float xmin, xmax, ymin, ymax;

			xmin = cl.time * 0.25;
			ymin = cl.time * 0.25;
			xmax = xmin + 1;
			ymax = ymin + 1/vt*vs;

			GL_EnableMultitexture();
			GL_Bind (scenepp_texture_warp);

			GL_SelectTexture(mtexid1+1);
			qglEnable(GL_TEXTURE_2D);
			GL_Bind(scenepp_texture_edge);

			qglBegin(GL_QUADS);

			qglMTexCoord2fSGIS (mtexid0, 0, 0);
			qglMTexCoord2fSGIS (mtexid1, xmin, ymin);
			qglMTexCoord2fSGIS (mtexid1+1, 0, 0);
			qglVertex2f(0, 0);

			qglMTexCoord2fSGIS (mtexid0, vs, 0);
			qglMTexCoord2fSGIS (mtexid1, xmax, ymin);
			qglMTexCoord2fSGIS (mtexid1+1, 1, 0);
			qglVertex2f(glwidth, 0);

			qglMTexCoord2fSGIS (mtexid0, vs, vt);
			qglMTexCoord2fSGIS (mtexid1, xmax, ymax);
			qglMTexCoord2fSGIS (mtexid1+1, 1, 1);
			qglVertex2f(glwidth, glheight);

			qglMTexCoord2fSGIS (mtexid0, 0, vt);
			qglMTexCoord2fSGIS (mtexid1, xmin, ymax);
			qglMTexCoord2fSGIS (mtexid1+1, 0, 1);
			qglVertex2f(0, glheight);
			
			qglEnd();

			qglDisable(GL_TEXTURE_2D);
			GL_SelectTexture(mtexid1);

			GL_DisableMultitexture();
		}

		// Disable shaders
		GLSlang_UseProgram(0);

		// After all the post processing, pop the matrices
		qglMatrixMode(GL_PROJECTION);
		qglPopMatrix();
		qglMatrixMode(GL_MODELVIEW);
		qglPopMatrix();

		if (qglGetError())
			Con_Printf("GL Error after drawing with shaderobjects\n");
	}
}

#endif
