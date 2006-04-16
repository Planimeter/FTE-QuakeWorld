/*
Copyright (C) 2002-2003 Victor Luchits

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
// r_shader.c - based on code by Stephen C. Taylor


#include "quakedef.h"
#include "glquake.h"
#include "shader.h"

#include "hash.h"


#include <ctype.h>

extern int missing_texture;



//Spike: Marked code removal areas with FIZME
//readd as porting progresses


#ifdef Q3SHADERS

cvar_t r_vertexlight = SCVAR("r_vertexlight", "0");

#define Q_stricmp stricmp
#define Com_sprintf snprintf
#define clamp(v,min, max) (v) = (((v)<(min))?(min):(((v)>(max))?(max):(v)));

int FS_LoadFile(char *name, void **file)
{
	*file = COM_LoadMallocFile(name);
	return com_filesize;
}
void FS_FreeFile(void *file)
{
	BZ_Free(file);
}

typedef union {
	float			f;
	unsigned int	i;
} float_int_t;
qbyte FloatToByte( float x )
{
	static float_int_t f2i;

	// shift float to have 8bit fraction at base of number
	f2i.f = x + 32768.0f;

	// then read as integer and kill float bits...
	return (qbyte) min(f2i.i & 0x7FFFFF, 255);
}



cvar_t r_detailtextures;


#define MAX_SHADERS 2048	//fixme: this takes a lot of bss in the r_shaders list


#define MAX_TOKEN_CHARS 1024

char *COM_ParseExt (char **data_p, qboolean nl)
{
	int		c;
	int		len;
	char	*data;
	qboolean newlines = false;

	data = *data_p;
	len = 0;
	com_token[0] = 0;
	
	if (!data)
	{
		*data_p = NULL;
		return "";
	}
		
// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
		{
			*data_p = NULL;
			return "";
		}
		if (c == '\n')
			newlines = true;
		data++;
	}

	if ( newlines && !nl ) {
		*data_p = data;
		return com_token;
	}

// skip // comments
	if (c == '/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32);

	if (len == MAX_TOKEN_CHARS)
	{
//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = data;
	return com_token;
}







#define HASH_SIZE	128

typedef struct shaderkey_s
{
    char			*keyword;
    void			(*func)( shader_t *shader, shaderpass_t *pass, char **ptr );
} shaderkey_t;

typedef struct shadercache_s {
	char name[MAX_QPATH];
	char *path;
	unsigned int offset;
	struct shadercache_s *hash_next;
} shadercache_t;

static shadercache_t *shader_hash[HASH_SIZE];
static char shaderbuf[MAX_QPATH * 256];
int shaderbuflen;

shader_t	r_shaders[MAX_SHADERS];

//static char		r_skyboxname[MAX_QPATH];
//static float	r_skyheight;

char *Shader_Skip( char *ptr );
static qboolean Shader_Parsetok( shader_t *shader, shaderpass_t *pass, shaderkey_t *keys,
		char *token, char **ptr );
static void Shader_ParseFunc( char **args, shaderfunc_t *func );
static void Shader_MakeCache( char *path );
static void Shader_GetPathAndOffset( char *name, char **path, unsigned int *offset );

//===========================================================================

static char *Shader_ParseString ( char **ptr )
{
	char *token;

	if ( !ptr || !(*ptr) ) {
		return "";
	}
	if ( !**ptr || **ptr == '}' ) {
		return "";
	}

	token = COM_ParseExt ( ptr, false );
	Q_strlwr ( token );
	
	return token;
}

static float Shader_ParseFloat ( char **ptr )
{
	if ( !ptr || !(*ptr) ) {
		return 0;
	}
	if ( !**ptr || **ptr == '}' ) {
		return 0;
	}

	return atof ( COM_ParseExt ( ptr, false ) );
}

static void Shader_ParseVector ( char **ptr, vec3_t v )
{
	char *token;
	qboolean bracket;

	token = Shader_ParseString ( ptr );
	if ( !Q_stricmp (token, "(") ) {
		bracket = true;
		token = Shader_ParseString ( ptr );
	} else if ( token[0] == '(' ) {
		bracket = true;
		token = &token[1];
	} else {
		bracket = false;
	}

	v[0] = atof ( token );
	v[1] = Shader_ParseFloat ( ptr );

	token = Shader_ParseString ( ptr );
	if ( !token[0] ) {
		v[2] = 0;
	} else if ( token[strlen(token)-1] == ')' ) {
		token[strlen(token)-1] = 0;
		v[2] = atof ( token );
	} else {
		v[2] = atof ( token );
		if ( bracket ) {
			Shader_ParseString ( ptr );
		}
	}
}

static void Shader_ParseSkySides ( char **ptr, int *images )
{
	int i;
	char *token;
	char path[MAX_QPATH];
	static char	*suf[6] = { "rt", "bk", "lf", "ft", "up", "dn" };

	token = Shader_ParseString ( ptr );
	for ( i = 0; i < 6; i++ )
	{
		if ( token[0] == '-' ) {
			images[i] = 0;
		} else {
			Com_sprintf ( path, sizeof(path), "%s_%s", token, suf[i] );
			images[i] = Mod_LoadHiResTexture ( path, NULL, true, false, true);//|IT_SKY );
			if (!images[i])
				images[i] = missing_texture;
		}
	}
}

static void Shader_ParseFunc ( char **ptr, shaderfunc_t *func )
{
	char *token;

	token = Shader_ParseString ( ptr );
	if ( !Q_stricmp (token, "sin") ) {
	    func->type = SHADER_FUNC_SIN;
	} else if ( !Q_stricmp (token, "triangle") ) {
	    func->type = SHADER_FUNC_TRIANGLE;
	} else if ( !Q_stricmp (token, "square") ) {
	    func->type = SHADER_FUNC_SQUARE;
	} else if ( !Q_stricmp (token, "sawtooth") ) {
	    func->type = SHADER_FUNC_SAWTOOTH;
	} else if (!Q_stricmp (token, "inversesawtooth") ) {
	    func->type = SHADER_FUNC_INVERSESAWTOOTH;
	} else if (!Q_stricmp (token, "noise") ) {
	    func->type = SHADER_FUNC_NOISE;
	}

	func->args[0] = Shader_ParseFloat ( ptr );
	func->args[1] = Shader_ParseFloat ( ptr );
	func->args[2] = Shader_ParseFloat ( ptr );
	func->args[3] = Shader_ParseFloat ( ptr );
}

//===========================================================================


enum {
	IT_CLAMP = 1<<0,
	IT_SKY = 1<<1,
	IT_NOMIPMAP = 1<<2,
	IT_NOPICMIP = 1<<3
};
static int Shader_SetImageFlags ( shader_t *shader )
{
	int flags = 0;

	if ( shader->flags & SHADER_SKY ) {
		flags |= IT_SKY;
	}
	if ( shader->flags & SHADER_NOMIPMAPS ) {
		flags |= IT_NOMIPMAP;
	}
	if ( shader->flags & SHADER_NOPICMIP ) {
		flags |= IT_NOPICMIP;
	}

	return flags;
}

static int Shader_FindImage ( char *name, int flags )
{
	if ( !Q_stricmp (name, "$whiteimage") ) {
		return 0;
	} else {
		return Mod_LoadHiResTexture(name, NULL, !(flags & IT_NOMIPMAP), true, true);//GL_FindImage ( name, flags );
    }
}


/****************** shader keyword functions ************************/

static void Shader_Cull ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	char *token;

	shader->flags &= ~(SHADER_CULL_FRONT|SHADER_CULL_BACK);

	token = Shader_ParseString ( ptr );
	if ( !Q_stricmp (token, "disable") || !Q_stricmp (token, "none") || !Q_stricmp (token, "twosided") ) {
	} else if ( !Q_stricmp (token, "front") ) {
		shader->flags |= SHADER_CULL_FRONT;
	} else if ( !Q_stricmp (token, "back") || !Q_stricmp (token, "backside") || !Q_stricmp (token, "backsided") ) {
		shader->flags |= SHADER_CULL_BACK;
	} else {
		shader->flags |= SHADER_CULL_FRONT;
	}
}

static void Shader_NoMipMaps ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	shader->flags |= (SHADER_NOMIPMAPS|SHADER_NOPICMIP);
}

static void Shader_NoPicMip ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	shader->flags |= SHADER_NOPICMIP;
}

static void Shader_DeformVertexes ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	char *token;
	deformv_t *deformv;

	if ( shader->numdeforms >= SHADER_DEFORM_MAX ) {
		return;
	}

	deformv = &shader->deforms[shader->numdeforms];

	token = Shader_ParseString ( ptr );
	if ( !Q_stricmp (token, "wave") ) {
		deformv->type = DEFORMV_WAVE;
		deformv->args[0] = Shader_ParseFloat ( ptr );
		if ( deformv->args[0] ) {
			deformv->args[0] = 1.0f / deformv->args[0];
		}

		Shader_ParseFunc ( ptr, &deformv->func );
	} else if ( !Q_stricmp (token, "normal") ) {
		deformv->type = DEFORMV_NORMAL;
		deformv->args[0] = Shader_ParseFloat ( ptr );
		deformv->args[1] = Shader_ParseFloat ( ptr );
	} else if ( !Q_stricmp (token, "bulge") ) {
		deformv->type = DEFORMV_BULGE;

		Shader_ParseVector ( ptr, deformv->args );
		shader->flags |= SHADER_DEFORMV_BULGE;
	} else if ( !Q_stricmp (token, "move") ) {
		deformv->type = DEFORMV_MOVE;

		Shader_ParseVector ( ptr, deformv->args );
		Shader_ParseFunc ( ptr, &deformv->func );
	} else if ( !Q_stricmp (token, "autosprite") ) {
		deformv->type = DEFORMV_AUTOSPRITE;
		shader->flags |= SHADER_AUTOSPRITE;
	} else if ( !Q_stricmp (token, "autosprite2") ) {
		deformv->type = DEFORMV_AUTOSPRITE2;
		shader->flags |= SHADER_AUTOSPRITE;
	} else if ( !Q_stricmp (token, "projectionShadow") ) {
		deformv->type = DEFORMV_PROJECTION_SHADOW;
	} else {
		return;
	}

	shader->numdeforms++;
}


static void Shader_SkyParms ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	int	i;
	skydome_t *skydome;
	float skyheight;

	if ( shader->skydome ) {
		for ( i = 0; i < 5; i++ ) {
			Z_Free ( shader->skydome->meshes[i].xyz_array );
			Z_Free ( shader->skydome->meshes[i].normals_array );
			Z_Free ( shader->skydome->meshes[i].st_array );
		}

		Z_Free ( shader->skydome );
	}

	skydome = (skydome_t *)Z_Malloc ( sizeof(skydome_t) );
	shader->skydome = skydome;

	Shader_ParseSkySides ( ptr, skydome->farbox_textures );

	skyheight = Shader_ParseFloat ( ptr );
	if ( !skyheight ) {
		skyheight = 512.0f;
	}

	Shader_ParseSkySides ( ptr, skydome->nearbox_textures );

//	R_CreateSkydome ( shader, skyheight );

	shader->flags |= SHADER_SKY;
	shader->sort = SHADER_SORT_SKY;
}

static void Shader_FogParms ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	float div;
	vec3_t color, fcolor;

//	if ( !r_ignorehwgamma->value )
//		div = 1.0f / pow(2, max(0, floor(r_overbrightbits->value)));
//	else
		div = 1.0f;

	Shader_ParseVector ( ptr, color );
	VectorScale ( color, div, color );
	ColorNormalize ( color, fcolor );

	shader->fog_color[0] = FloatToByte ( fcolor[0] );
	shader->fog_color[1] = FloatToByte ( fcolor[1] );
	shader->fog_color[2] = FloatToByte ( fcolor[2] );
	shader->fog_color[3] = 255;	
	shader->fog_dist = Shader_ParseFloat ( ptr );

	if ( shader->fog_dist <= 0.0f ) {
		shader->fog_dist = 128.0f;
	}
	shader->fog_dist = 1.0f / shader->fog_dist;
}

static void Shader_SurfaceParm ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	char *token;

	token = Shader_ParseString ( ptr );
	if ( !Q_stricmp( token, "nodraw" ) )
		shader->flags = SHADER_NODRAW;
}

static void Shader_Sort ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	char *token;

	token = Shader_ParseString ( ptr );
	if ( !Q_stricmp( token, "portal" ) ) {
		shader->sort = SHADER_SORT_PORTAL;
	} else if( !Q_stricmp( token, "sky" ) ) {
		shader->sort = SHADER_SORT_SKY;
	} else if( !Q_stricmp( token, "opaque" ) ) {
		shader->sort = SHADER_SORT_OPAQUE;
	} else if( !Q_stricmp( token, "banner" ) ) {
		shader->sort = SHADER_SORT_BANNER;
	} else if( !Q_stricmp( token, "underwater" ) ) {
		shader->sort = SHADER_SORT_UNDERWATER;
	} else if( !Q_stricmp( token, "additive" ) ) {
		shader->sort = SHADER_SORT_ADDITIVE;
	} else if( !Q_stricmp( token, "nearest" ) ) {
		shader->sort = SHADER_SORT_NEAREST;
	} else {
		shader->sort = atoi ( token );
		clamp ( shader->sort, SHADER_SORT_NONE, SHADER_SORT_NEAREST );
	}
}

static void Shader_Portal ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	shader->sort = SHADER_SORT_PORTAL;
}

static void Shader_PolygonOffset ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	shader->flags |= SHADER_POLYGONOFFSET;
}

static void Shader_EntityMergable ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	shader->flags |= SHADER_ENTITY_MERGABLE;
}

static void Shader_ProgramName ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	void *vert, *frag;
	char *token;
	if (shader->programhandle)
	{	//this allows fallbacks
		token = Shader_ParseString ( ptr );
		token = Shader_ParseString ( ptr );
		return;
	}
	token = Shader_ParseString ( ptr );
	FS_LoadFile(token, &vert);
	token = Shader_ParseString ( ptr );
	FS_LoadFile(token, &frag);
	if (vert && frag)
		shader->programhandle = GLSlang_CreateProgram("", (char *)vert, (char *)frag);
	if (vert)
		FS_FreeFile(vert);
	if (frag)
		FS_FreeFile(frag);
}

static void Shader_ProgramParam ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	cvar_t *cv;
	int specialint = 0;
	float specialfloat = 0;
	enum shaderprogparmtype_e parmtype = SP_BAD;
	char *token;
	unsigned int uniformloc;

	token = Shader_ParseString ( ptr );
	if (!Q_stricmp(token, "texture"))
	{
		token = Shader_ParseString ( ptr );
		specialint = atoi(token);
		parmtype = SP_TEXTURE;
	}
	else if (!Q_stricmp(token, "cvari"))
	{
		token = Shader_ParseString ( ptr );
		cv = Cvar_Get(token, "", 0, "GLSL Shader parameters");
		if (cv)
		{	//Cvar_Get returns null if the cvar is the name of a command
			specialint = atoi(cv->string);
			specialfloat = cv->value;
		}
		parmtype = SP_CVARI;
	}
	else if (!Q_stricmp(token, "cvarf"))
	{
		token = Shader_ParseString ( ptr );
		cv = Cvar_Get(token, "", 0, "GLSL Shader parameters");
		if (cv)
		{	//Cvar_Get returns null if the cvar is the name of a command
			specialint = atoi(cv->string);
			specialfloat = cv->value;
		}
		parmtype = SP_CVARF;
	}
	else if (!Q_stricmp(token, "time"))
	{
		parmtype = SP_TIME;
	}


	if (!shader->programhandle)
	{
		Con_Printf("shader %s: param without program set\n", shader->name);
		token = Shader_ParseString ( ptr );
	}
	else
	{
		GLSlang_UseProgram(shader->programhandle);

		token = Shader_ParseString ( ptr );
		uniformloc = GLSlang_GetUniformLocation(shader->programhandle, token);

		if (uniformloc == -1)
		{
			Con_Printf("shader %s: param without uniform \"%s\"\n", shader->name, token);
			return;
		}
		else
		{
			switch(parmtype)
			{
			case SP_TEXTURE:
			case SP_CVARI:
				GLSlang_SetUniform1i(uniformloc, specialint);
				break;
			case SP_CVARF:
				GLSlang_SetUniform1f(uniformloc, specialfloat);
				break;
			default:
				shader->progparm[shader->numprogparams].type = parmtype;
				shader->progparm[shader->numprogparams].handle = uniformloc;
				shader->numprogparams++;
				break;
			}
		}
		GLSlang_UseProgram(0);
	}
}

static shaderkey_t shaderkeys[] =
{
    {"cull",			Shader_Cull },
    {"skyparms",		Shader_SkyParms },
	{"fogparms",		Shader_FogParms },
	{"surfaceparm",		Shader_SurfaceParm },
    {"nomipmaps",		Shader_NoMipMaps },
	{"nopicmip",		Shader_NoPicMip },
	{"polygonoffset",	Shader_PolygonOffset },
	{"sort",			Shader_Sort },
    {"deformvertexes",	Shader_DeformVertexes },
	{"portal",			Shader_Portal },
	{"entitymergable",	Shader_EntityMergable },

	{"program",			Shader_ProgramName },	//glsl
	{"param",			Shader_ProgramParam },

    {NULL,				NULL}
};

// ===============================================================

static void Shaderpass_Map ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	int flags;
	char *token;

	token = Shader_ParseString ( ptr );
	if ( !Q_stricmp (token, "$lightmap") )
	{
		pass->tcgen = TC_GEN_LIGHTMAP;
		pass->flags |= SHADER_PASS_LIGHTMAP;
		pass->anim_frames[0] = 0;
	}
	else if ( !Q_stricmp (token, "$deluxmap") )
	{
		pass->tcgen = TC_GEN_LIGHTMAP;
		pass->flags |= SHADER_PASS_DELUXMAP;
		pass->anim_frames[0] = 0;
	}
	else
	{
		flags = Shader_SetImageFlags ( shader );

		pass->tcgen = TC_GEN_BASE;
		pass->anim_frames[0] = Shader_FindImage ( token, flags );

		if ( !pass->anim_frames[0] ) {
			pass->anim_frames[0] = missing_texture;
			Con_DPrintf (S_WARNING "Shader %s has a stage with no image: %s.\n", shader->name, token );
		}
    }
}

static void Shaderpass_AnimMap ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
    int flags;
	char *token;
	int image;

	flags = Shader_SetImageFlags ( shader );

	pass->tcgen = TC_GEN_BASE;
    pass->flags |= SHADER_PASS_ANIMMAP;
    pass->anim_fps = (int)Shader_ParseFloat ( ptr );
	pass->anim_numframes = 0;

    for ( ; ; ) {
		token = Shader_ParseString ( ptr );
		if ( !token[0] ) {
			break;
		}

		if ( pass->anim_numframes < SHADER_ANIM_FRAMES_MAX ) {
			image = Shader_FindImage ( token, flags );

			if ( !image ) {
				pass->anim_frames[pass->anim_numframes++] = missing_texture;
				Con_DPrintf (S_WARNING "Shader %s has an animmap with no image: %s.\n", shader->name, token );
			} else {
				pass->anim_frames[pass->anim_numframes++] = image;
			}
		}
	}
}

static void Shaderpass_ClampMap ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	int flags;
	char *token;

	token = Shader_ParseString ( ptr );
	flags = Shader_SetImageFlags ( shader );

	pass->tcgen = TC_GEN_BASE;
	pass->anim_frames[0] = Shader_FindImage ( token, flags | IT_CLAMP );

	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	if ( !pass->anim_frames[0] ) {
		pass->anim_frames[0] = missing_texture;
		Con_DPrintf (S_WARNING "Shader %s has a stage with no image: %s.\n", shader->name, token );
    }
}

static void Shaderpass_VideoMap ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	char		*token;

	token = Shader_ParseString ( ptr );

	if ( pass->cin )
		Z_Free ( pass->cin );

	pass->cin = Media_StartCin(token);
	if (!pass->cin)
		pass->cin = Media_StartCin(va("video/%s.roq", token));
	else
		Con_DPrintf (S_WARNING "(shader %s) Couldn't load video %s\n", shader->name, token );

	pass->anim_frames[0] = texture_extension_number++;
	pass->flags |= SHADER_PASS_VIDEOMAP;
	shader->flags |= SHADER_VIDEOMAP;
}

static void Shaderpass_RGBGen ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	char		*token;

	token = Shader_ParseString ( ptr );
	if ( !Q_stricmp (token, "identitylighting") )
		pass->rgbgen = RGB_GEN_IDENTITY_LIGHTING;
	else if ( !Q_stricmp (token, "identity") )
		pass->rgbgen = RGB_GEN_IDENTITY;
	else if ( !Q_stricmp (token, "wave") )
	{
		pass->rgbgen = RGB_GEN_WAVE;
		Shader_ParseFunc ( ptr, &pass->rgbgen_func );
	}
	else if ( !Q_stricmp(token, "entity") )
		pass->rgbgen = RGB_GEN_ENTITY;
	else if ( !Q_stricmp (token, "oneMinusEntity") )
		pass->rgbgen = RGB_GEN_ONE_MINUS_ENTITY;
	else if ( !Q_stricmp (token, "vertex"))
		pass->rgbgen = RGB_GEN_VERTEX;
	else if ( !Q_stricmp (token, "oneMinusVertex") )
		pass->rgbgen = RGB_GEN_ONE_MINUS_VERTEX;
	else if ( !Q_stricmp (token, "lightingDiffuse") )
		pass->rgbgen = RGB_GEN_LIGHTING_DIFFUSE;
	else if ( !Q_stricmp (token, "exactvertex") )
		pass->rgbgen = RGB_GEN_EXACT_VERTEX;
	else if ( !Q_stricmp (token, "const") || !Q_stricmp (token, "constant") )
	{
		pass->rgbgen = RGB_GEN_CONST;
		pass->rgbgen_func.type = SHADER_FUNC_CONSTANT;

		Shader_ParseVector ( ptr, pass->rgbgen_func.args );
	}
	else if ( !Q_stricmp (token, "topcolor") )
		pass->rgbgen = RGB_GEN_TOPCOLOR;
	else if ( !Q_stricmp (token, "bottomcolor") )
		pass->rgbgen = RGB_GEN_BOTTOMCOLOR;
}

static void Shaderpass_AlphaGen ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	char		*token;

	token = Shader_ParseString ( ptr );
	if ( !Q_stricmp (token, "portal") ) {
		pass->alphagen = ALPHA_GEN_PORTAL;
		shader->flags |= SHADER_AGEN_PORTAL;
	} else if ( !Q_stricmp (token, "vertex") ) {
		pass->alphagen = ALPHA_GEN_VERTEX;
	} else if ( !Q_stricmp (token, "entity") ) {
		pass->alphagen = ALPHA_GEN_ENTITY;
	} else if ( !Q_stricmp (token, "wave") ) {
		pass->alphagen = ALPHA_GEN_WAVE;

		Shader_ParseFunc ( ptr, &pass->alphagen_func );
	} else if ( !Q_stricmp (token, "lightingspecular") ) {
		pass->alphagen = ALPHA_GEN_SPECULAR;
	} else if ( !Q_stricmp (token, "const") || !Q_stricmp (token, "constant") ) {
		pass->alphagen = ALPHA_GEN_CONST;
		pass->alphagen_func.type = SHADER_FUNC_CONSTANT;
		pass->alphagen_func.args[0] = fabs( Shader_ParseFloat (ptr) );
	}
}
static void Shaderpass_AlphaShift ( shader_t *shader, shaderpass_t *pass, char **ptr )	//for alienarena
{
	float speed;
	float min, max;
	pass->alphagen = ALPHA_GEN_WAVE;

	pass->alphagen_func.type = SHADER_FUNC_SIN;


	//arg0 = add
	//arg1 = scale
	//arg2 = timeshift
	//arg3 = timescale

	speed = Shader_ParseFloat ( ptr );
	min = Shader_ParseFloat ( ptr );
	max = Shader_ParseFloat ( ptr );

	pass->alphagen_func.args[0] = min + (max - min)/2;
	pass->alphagen_func.args[1] = (max - min)/2;
	pass->alphagen_func.args[2] = 0;
	pass->alphagen_func.args[3] = 1/speed;
}

static void Shaderpass_BlendFunc ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	char		*token;

	token = Shader_ParseString ( ptr );
	if ( !Q_stricmp (token, "blend") ) {
		pass->blendsrc = GL_SRC_ALPHA;
		pass->blenddst = GL_ONE_MINUS_SRC_ALPHA;
	} else if ( !Q_stricmp (token, "filter") ) {
		pass->blendsrc = GL_DST_COLOR;
		pass->blenddst = GL_ZERO;
	} else if ( !Q_stricmp (token, "add") ) {
		pass->blendsrc = pass->blenddst = GL_ONE;
	} else {
		int i;
		unsigned int *blend;

		for ( i = 0; i < 2; i++ )
		{
			blend = (i == 0) ? &pass->blendsrc : &pass->blenddst;

			if ( !Q_stricmp ( token, "gl_zero") )
				*blend = GL_ZERO;
			else if ( !Q_stricmp (token, "gl_one") )
				*blend = GL_ONE;
			else if ( !Q_stricmp (token, "gl_dst_color") )
				*blend = GL_DST_COLOR;
			else if ( !Q_stricmp (token, "gl_one_minus_src_alpha") )
				*blend = GL_ONE_MINUS_SRC_ALPHA;
			else if ( !Q_stricmp (token, "gl_src_alpha") )
				*blend = GL_SRC_ALPHA;
			else if ( !Q_stricmp (token, "gl_src_color") )
				*blend = GL_SRC_COLOR;
			else if ( !Q_stricmp (token, "gl_one_minus_dst_color") )
				*blend = GL_ONE_MINUS_DST_COLOR;
			else if ( !Q_stricmp (token, "gl_one_minus_src_color") )
				*blend = GL_ONE_MINUS_SRC_COLOR;
			else if ( !Q_stricmp (token, "gl_dst_alpha") )
				*blend = GL_DST_ALPHA;
			else if ( !Q_stricmp (token, "gl_one_minus_dst_alpha") )
				*blend = GL_ONE_MINUS_DST_ALPHA;
			else
				*blend = GL_ONE;

			if ( !i ) {
				token = Shader_ParseString ( ptr );
			}
		}
    }

	pass->flags |= SHADER_PASS_BLEND;
}

static void Shaderpass_AlphaFunc ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	char *token;

	token = Shader_ParseString ( ptr );
    if ( !Q_stricmp (token, "gt0") ) {
		pass->alphafunc = SHADER_ALPHA_GT0;
	} else if ( !Q_stricmp (token, "lt128") ) {
		pass->alphafunc = SHADER_ALPHA_LT128;
	} else if ( !Q_stricmp (token, "ge128") ) {
		pass->alphafunc = SHADER_ALPHA_GE128;
    } else {
		return;
	}

    pass->flags |= SHADER_PASS_ALPHAFUNC;
}

static void Shaderpass_DepthFunc ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	char *token;

	token = Shader_ParseString ( ptr );
    if ( !Q_stricmp (token, "equal") )
		pass->depthfunc = GL_EQUAL;
    else if ( !Q_stricmp (token, "lequal") )
		pass->depthfunc = GL_LEQUAL;
	else if ( !Q_stricmp (token, "gequal") )
		pass->depthfunc = GL_GEQUAL;
}

static void Shaderpass_DepthWrite ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
    shader->flags |= SHADER_DEPTHWRITE;
    pass->flags |= SHADER_PASS_DEPTHWRITE;
}

static void Shaderpass_TcMod ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	int i;
	tcmod_t *tcmod;
	char *token;

	if (pass->numtcmods >= SHADER_TCMOD_MAX) {
		return;
	}

	tcmod = &pass->tcmods[pass->numtcmods];

	token = Shader_ParseString ( ptr );
	if ( !Q_stricmp (token, "rotate") ) {
		tcmod->args[0] = -Shader_ParseFloat ( ptr ) / 360.0f;
		if ( !tcmod->args[0] ) {
			return;
		}

		tcmod->type = SHADER_TCMOD_ROTATE;
	} else if ( !Q_stricmp (token, "scale") ) {
		tcmod->args[0] = Shader_ParseFloat ( ptr );
		tcmod->args[1] = Shader_ParseFloat ( ptr );
		tcmod->type = SHADER_TCMOD_SCALE;
	} else if ( !Q_stricmp (token, "scroll") ) {
		tcmod->args[0] = Shader_ParseFloat ( ptr );
		tcmod->args[1] = Shader_ParseFloat ( ptr );
		tcmod->type = SHADER_TCMOD_SCROLL;
	} else if ( !Q_stricmp (token, "stretch") ) {
		shaderfunc_t func;

		Shader_ParseFunc ( ptr, &func );

		tcmod->args[0] = func.type;
		for (i = 1; i < 5; ++i)
			tcmod->args[i] = func.args[i-1];
		tcmod->type = SHADER_TCMOD_STRETCH;
	} else if ( !Q_stricmp (token, "transform") ) {
		for (i = 0; i < 6; ++i)
			tcmod->args[i] = Shader_ParseFloat ( ptr );
		tcmod->type = SHADER_TCMOD_TRANSFORM;
	} else if ( !Q_stricmp (token, "turb") ) {
		for (i = 0; i < 4; i++)
			tcmod->args[i] = Shader_ParseFloat ( ptr );
		tcmod->type = SHADER_TCMOD_TURB;
	} else {
		return;
	}

	pass->numtcmods++;
}

static void Shaderpass_Scale ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	//seperate x and y
	char *token;
	tcmod_t *tcmod;

	tcmod = &pass->tcmods[pass->numtcmods];

	token = Shader_ParseString ( ptr );
	if (!strcmp(token, "static"))
	{
		tcmod->type = SHADER_TCMOD_SCALE;
		tcmod->args[0] = Shader_ParseFloat ( ptr );
	}
	else
	{
		Con_Printf("Bad shader scale\n");
		return;
	}

	token = Shader_ParseString ( ptr );
	if (!strcmp(token, "static"))
	{
		tcmod->type = SHADER_TCMOD_SCALE;
		tcmod->args[1] = Shader_ParseFloat ( ptr );
	}
	else
	{
		Con_Printf("Bad shader scale\n");
		return;
	}

	pass->numtcmods++;
}

static void Shaderpass_Scroll ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	//seperate x and y
	char *token;
	tcmod_t *tcmod;

	tcmod = &pass->tcmods[pass->numtcmods];

	token = Shader_ParseString ( ptr );
	if (!strcmp(token, "static"))
	{
		tcmod->type = SHADER_TCMOD_SCROLL;
		tcmod->args[0] = Shader_ParseFloat ( ptr );
	}
	else
	{
		Con_Printf("Bad shader scale\n");
		return;
	}

	token = Shader_ParseString ( ptr );
	if (!strcmp(token, "static"))
	{
		tcmod->type = SHADER_TCMOD_SCROLL;
		tcmod->args[1] = Shader_ParseFloat ( ptr );
	}
	else
	{
		Con_Printf("Bad shader scale\n");
		return;
	}

	pass->numtcmods++;
}


static void Shaderpass_TcGen ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	char *token;

	token = Shader_ParseString ( ptr );
	if ( !Q_stricmp (token, "base") ) {
		pass->tcgen = TC_GEN_BASE;
	} else if ( !Q_stricmp (token, "lightmap") ) {
		pass->tcgen = TC_GEN_LIGHTMAP;
	} else if ( !Q_stricmp (token, "environment") ) {
		pass->tcgen = TC_GEN_ENVIRONMENT;
	} else if ( !Q_stricmp (token, "vector") ) {
		pass->tcgen = TC_GEN_BASE;
	}
}
static void Shaderpass_EnvMap ( shader_t *shader, shaderpass_t *pass, char **ptr )	//for alienarena
{
	pass->tcgen = TC_GEN_ENVIRONMENT;
}

static void Shaderpass_Detail ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	pass->flags |= SHADER_PASS_DETAIL;
}

static void Shaderpass_AlphaMask ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	pass->flags |= SHADER_PASS_ALPHAFUNC;
	pass->alphafunc = SHADER_ALPHA_GE128;
}

static void Shaderpass_NoLightMap ( shader_t *shader, shaderpass_t *pass, char **ptr )
{
	pass->rgbgen = RGB_GEN_IDENTITY;
}

static shaderkey_t shaderpasskeys[] =
{
    {"rgbgen",		Shaderpass_RGBGen },
    {"blendfunc",	Shaderpass_BlendFunc },
    {"depthfunc",	Shaderpass_DepthFunc },
    {"depthwrite",	Shaderpass_DepthWrite },
    {"alphafunc",	Shaderpass_AlphaFunc },
    {"tcmod",		Shaderpass_TcMod },
    {"map",			Shaderpass_Map },
    {"animmap",		Shaderpass_AnimMap },
    {"clampmap",	Shaderpass_ClampMap },
	{"videomap",	Shaderpass_VideoMap },
    {"tcgen",		Shaderpass_TcGen },
	{"envmap",		Shaderpass_EnvMap },//for alienarena
	{"nolightmap",	Shaderpass_NoLightMap },//for alienarena
	{"scale",		Shaderpass_Scale },//for alienarena
	{"scroll",		Shaderpass_Scroll },//for alienarena
	{"alphagen",	Shaderpass_AlphaGen },
	{"alphashift",	Shaderpass_AlphaShift },//for alienarena
	{"alphamask",	Shaderpass_AlphaMask },//for alienarena
	{"detail",		Shaderpass_Detail },
    {NULL,			NULL }
};

// ===============================================================

int Shader_InitCallback (char *name, int size, void *param)
{
	strcpy(shaderbuf+shaderbuflen, name);
	Shader_MakeCache(shaderbuf+shaderbuflen);
	shaderbuflen += strlen(name)+1;

	return true;
}

qboolean Shader_Init (void)
{
	shaderbuflen = 0;

	Con_Printf ( "Initializing Shaders.\n" );

	COM_EnumerateFiles("shaders/*.shader", Shader_InitCallback, NULL);
	COM_EnumerateFiles("scripts/*.shader", Shader_InitCallback, NULL);
//	COM_EnumerateFiles("scripts/*.rscript", Shader_InitCallback, NULL);

	/*
	char *dirptr;
	int i, dirlen, numdirs;

	numdirs = FS_GetFileList ( "scripts", "shader", shaderbuf, sizeof(shaderbuf) );
	if ( !numdirs ) {
		Con_Printf ("Could not find any shaders!");
		return false;
	}

	// now load all the scripts
	dirptr = shaderbuf;
	memset ( shader_hash, 0, sizeof(shadercache_t *)*HASH_SIZE );

	for (i=0; i<numdirs; i++, dirptr += dirlen+1) {
		dirlen = strlen(dirptr);
		if ( !dirlen ) {
			continue;
		}

		Shader_MakeCache ( dirptr );
	}
*/
	return true;
}

static void Shader_MakeCache ( char *path )
{
	unsigned int key, i;
	char filename[MAX_QPATH];
	char *buf, *ptr, *token, *t;
	shadercache_t *cache;
	int size;

	Com_sprintf( filename, sizeof(filename), "%s", path );
	Con_DPrintf ( "...loading '%s'\n", filename );

	size = FS_LoadFile ( filename, (void **)&buf );
	if ( !buf || size <= 0 ) {
		return;
	}

	ptr = buf;
	do
	{
		if ( ptr - buf >= size )
			break;

		token = COM_ParseExt ( &ptr, true );
		if ( !token[0] || ptr - buf >= size )
			break;

		COM_CleanUpPath(token);

		t = NULL;
		Shader_GetPathAndOffset ( token, &t, &i );
		if ( t ) {
			ptr = Shader_Skip ( ptr );
			continue;
		}

		key = Hash_Key ( token, HASH_SIZE );

		cache = ( shadercache_t * )Z_Malloc ( sizeof(shadercache_t) );
		cache->hash_next = shader_hash[key];
		cache->path = path;
		cache->offset = ptr - buf;
		Com_sprintf ( cache->name, MAX_QPATH, token );
		shader_hash[key] = cache;

		ptr = Shader_Skip ( ptr );
	} while ( ptr );

	FS_FreeFile ( buf );
}

char *Shader_Skip ( char *ptr )
{	
	char *tok;
    int brace_count;

    // Opening brace
    tok = COM_ParseExt ( &ptr, true );

	if (!ptr)
		return NULL;
    
	if ( tok[0] != '{' ) {
		tok = COM_ParseExt ( &ptr, true );
	}

    for (brace_count = 1; brace_count > 0 ; ptr++)
    {
		tok = COM_ParseExt ( &ptr, true );

		if ( !tok[0] )
			return NULL;

		if (tok[0] == '{') {
			brace_count++;
		} else if (tok[0] == '}') {
			brace_count--;
		}
    }

	return ptr;
}

static void Shader_GetPathAndOffset ( char *name, char **path, unsigned int *offset )
{
	unsigned int key;
	shadercache_t *cache;

	key = Hash_Key ( name, HASH_SIZE );
	cache = shader_hash[key];

	for ( ; cache; cache = cache->hash_next ) {
		if ( !Q_stricmp (cache->name, name) ) {
			*path = cache->path;
			*offset = cache->offset;
			return;
		}
	}

	path = NULL;
}

void Shader_FreePass (shaderpass_t *pass)
{
	if ( pass->flags & SHADER_PASS_VIDEOMAP )
	{
		Media_ShutdownCin(pass->cin);
		pass->cin = NULL;
	}
}

void Shader_Free (shader_t *shader)
{
	int i;
	shaderpass_t *pass;

	if ( shader->skydome ) {
		for ( i = 0; i < 5; i++ ) {
			if (shader->skydome->meshes[i].xyz_array)
			{
				Z_Free ( shader->skydome->meshes[i].xyz_array );
				Z_Free ( shader->skydome->meshes[i].normals_array );
				Z_Free ( shader->skydome->meshes[i].st_array );
			}
		}

		Z_Free ( shader->skydome );
	}

	pass = shader->passes;
	for ( i = 0; i < shader->numpasses; i++, pass++ ) {
		Shader_FreePass ( pass );
	}
}

void Shader_Shutdown (void)
{
	int i;
	shader_t *shader;
	shadercache_t *cache, *cache_next;

	shader = r_shaders;
	for (i = 0; i < MAX_SHADERS; i++, shader++)
	{
		if ( !shader->registration_sequence )
			continue;
		
		Shader_Free ( shader );
	}

	for ( i = 0; i < HASH_SIZE; i++ ) {
		cache = shader_hash[i];

		for ( ; cache; cache = cache_next ) {
			cache_next = cache->hash_next;
			cache->hash_next = NULL;
			Z_Free ( cache );
		}
	}

	memset (r_shaders, 0, sizeof(shader_t)*MAX_SHADERS);

	memset (shader_hash, 0, sizeof(shader_hash));
}

void Shader_SetBlendmode ( shaderpass_t *pass )
{
	if ( !pass->anim_frames[0] && !(pass->flags & SHADER_PASS_LIGHTMAP) ) {
		pass->blendmode = 0;
		return;
	}

	if ( !(pass->flags & SHADER_PASS_BLEND) && qglMTexCoord2fSGIS ) {
		if ( (pass->rgbgen == RGB_GEN_IDENTITY) && (pass->alphagen == ALPHA_GEN_IDENTITY) ) {
			pass->blendmode = GL_REPLACE;
		} else {
			pass->blendsrc = GL_ONE;
			pass->blenddst = GL_ZERO;
			pass->blendmode = GL_MODULATE;
		}
		return;
	}

	if ( (pass->blendsrc == GL_ZERO && pass->blenddst == GL_SRC_COLOR) ||
		(pass->blendsrc == GL_DST_COLOR && pass->blenddst == GL_ZERO) )
		pass->blendmode = GL_MODULATE;
	else if ( pass->blendsrc == GL_ONE && pass->blenddst == GL_ONE )
		pass->blendmode = GL_ADD;
	else if ( pass->blendsrc == GL_SRC_ALPHA && pass->blenddst == GL_ONE_MINUS_SRC_ALPHA )
		pass->blendmode = GL_DECAL;
	else
		pass->blendmode = 0;
}

void Shader_Readpass (shader_t *shader, char **ptr)
{
    char *token;
	shaderpass_t *pass;
	qboolean ignore;
	static shader_t dummy;

	if ( shader->numpasses >= SHADER_PASS_MAX ) {
		ignore = true;
		shader = &dummy;
		shader->numpasses = 1;
		pass = shader->passes;
	} else {
		ignore = false;
		pass = &shader->passes[shader->numpasses++];
	}
	
    // Set defaults
    pass->flags = 0;
    pass->anim_frames[0] = 0;
	pass->anim_numframes = 0;
    pass->depthfunc = GL_LEQUAL;
    pass->rgbgen = RGB_GEN_UNKNOWN;
	pass->alphagen = ALPHA_GEN_IDENTITY;
	pass->tcgen = TC_GEN_BASE;
	pass->numtcmods = 0;

	// default to R_RenderMeshGeneric
	pass->numMergedPasses = 1;
	pass->flush = R_RenderMeshGeneric;

	while ( ptr )
	{
		token = COM_ParseExt (ptr, true);

		if ( !token[0] ) {
			continue;
		} else if ( token[0] == '}' ) {
			break;
		} else if ( Shader_Parsetok (shader, pass, shaderpasskeys, token, ptr) ) {
			break;
		}
	}

	// check some things 
	if ( ignore ) {
		Shader_Free ( shader );
		return;
	}

	if ( (pass->blendsrc == GL_ONE) && (pass->blenddst == GL_ZERO) ) {
		pass->flags |= SHADER_PASS_DEPTHWRITE;
		shader->flags |= SHADER_DEPTHWRITE;
	}

	switch (pass->rgbgen)
	{
		case RGB_GEN_IDENTITY_LIGHTING:
		case RGB_GEN_IDENTITY:
		case RGB_GEN_CONST:
		case RGB_GEN_WAVE:
		case RGB_GEN_ENTITY:
		case RGB_GEN_ONE_MINUS_ENTITY:
		case RGB_GEN_UNKNOWN:	// assume RGB_GEN_IDENTITY or RGB_GEN_IDENTITY_LIGHTING

			switch (pass->alphagen)
			{
				case ALPHA_GEN_IDENTITY:
				case ALPHA_GEN_CONST:
				case ALPHA_GEN_WAVE:
				case ALPHA_GEN_ENTITY:
					pass->flags |= SHADER_PASS_NOCOLORARRAY;
					break;
				default:
					break;
			}

			break;
		default:
			break;
	}

	Shader_SetBlendmode ( pass );

	if ( (shader->flags & SHADER_SKY) && (shader->flags & SHADER_DEPTHWRITE) ) {
		if ( pass->flags & SHADER_PASS_DEPTHWRITE ) {
			pass->flags &= ~SHADER_PASS_DEPTHWRITE;
		}
	}
}

static qboolean Shader_Parsetok (shader_t *shader, shaderpass_t *pass, shaderkey_t *keys, char *token, char **ptr)
{
    shaderkey_t *key;

	for (key = keys; key->keyword != NULL; key++)
	{
		if (!Q_stricmp (token, key->keyword))
		{
			if (key->func)
				key->func ( shader, pass, ptr );

			return ( ptr && *ptr && **ptr == '}' );
		}
	}

	if (!Q_stricmp(token, "if"))
	{
		int indent = 0;
		cvar_t *cv;
		qboolean conditiontrue = true;
		token = COM_ParseExt ( ptr, false );
		if (*token == '!')
		{
			conditiontrue = false;
			token++;
		}
		cv = Cvar_Get(token, "", 0, "Shader Conditions");
		if (cv)
			conditiontrue = conditiontrue == !!cv->value;

		if (conditiontrue)
		{
			while ( ptr )
			{
				token = COM_ParseExt (ptr, true);
				if ( !token[0] )
					continue;
				else if (token[0] == ']' || token[0] == '}')
					indent--;
				else if (token[0] == '[')
					indent++;
				else
					Shader_Parsetok (shader, pass, keys, token, ptr);
				if (!indent)
					break;
			}
		}
		else
		{
			while ( ptr )
			{
				token = COM_ParseExt (ptr, true);
				if (!token[0])
					continue;
				else if (token[0] == ']' || token[0] == '}')
					indent--;
				else if (token[0] == '[')
					indent++;
				if (!indent)
					break;
			}
		}

		return ( ptr && *ptr && **ptr == '}' );
	}

	// Next Line
	while (ptr)
	{
		token = COM_ParseExt ( ptr, false );
		if ( !token[0] ) {
			break;
		}
	}

	return false;
}

void Shader_SetPassFlush ( shaderpass_t *pass, shaderpass_t *pass2 )
{
	if ( ((pass->flags & SHADER_PASS_DETAIL) && !r_detailtextures.value) ||
		((pass2->flags & SHADER_PASS_DETAIL) && !r_detailtextures.value) ||
		 (pass->flags & SHADER_PASS_VIDEOMAP) || (pass2->flags & SHADER_PASS_VIDEOMAP) || 
		 ((pass->flags & SHADER_PASS_ALPHAFUNC) && (pass2->depthfunc != GL_EQUAL)) ) {
		return;
	}

	if ( pass2->rgbgen != RGB_GEN_IDENTITY || pass2->alphagen != ALPHA_GEN_IDENTITY ) {
		return;
	}
	if (pass->rgbgen != RGB_GEN_IDENTITY || pass->alphagen != ALPHA_GEN_IDENTITY ) 
		return;

	// check if we can use R_RenderMeshCombined

	if ( gl_config.tex_env_combine || gl_config.nv_tex_env_combine4 )
	{
		if ( pass->blendmode == GL_REPLACE )
		{
			if ((pass2->blendmode == GL_DECAL && gl_config.tex_env_combine) ||
				(pass2->blendmode == GL_ADD && gl_config.env_add) ||
				(pass2->blendmode && pass2->blendmode != GL_ADD) ||	gl_config.nv_tex_env_combine4 )
			{
				pass->flush = R_RenderMeshCombined;
			}
		}
		else if ( pass->blendmode == GL_ADD && 
			pass2->blendmode == GL_ADD && gl_config.env_add )
		{
			pass->flush = R_RenderMeshCombined;
		} else if ( pass->blendmode == GL_MODULATE && 
			pass2->blendmode == GL_MODULATE )
		{
			pass->flush = R_RenderMeshCombined;
		}
	}
	else if ( qglMTexCoord2fSGIS )
	{
		// check if we can use R_RenderMeshMultitextured
		if ( pass->blendmode == GL_REPLACE )
		{
			if ( pass2->blendmode == GL_ADD && gl_config.env_add )
			{
				pass->flush = R_RenderMeshMultitextured;
				pass->numMergedPasses = 2;
			}
			else if ( pass2->blendmode && pass2->blendmode != GL_DECAL )
			{
				pass->flush = R_RenderMeshMultitextured;
				pass->numMergedPasses = 2;
			}
		}
		else if ( pass->blendmode == GL_MODULATE && 
			pass2->blendmode == GL_MODULATE )
		{
			pass->flush = R_RenderMeshMultitextured;
		}
		else if ( pass->blendmode == GL_ADD && 
			pass2->blendmode == GL_ADD && gl_config.env_add )
		{
			pass->flush = R_RenderMeshCombined;
		}
	}

	if ( pass->flush != R_RenderMeshGeneric ) {
		pass->numMergedPasses = 2;
	}
}

void Shader_SetFeatures ( shader_t *s )
{
	int i;
	qboolean trnormals;
	shaderpass_t *pass;

	s->features = MF_NONE;
	
	for ( i = 0, trnormals = true; i < s->numdeforms; i++ ) {
		switch ( s->deforms[i].type ) {
			case DEFORMV_BULGE:
			case DEFORMV_WAVE:
				trnormals = false;
			case DEFORMV_NORMAL:
				s->features |= MF_NORMALS;
				break;
			case DEFORMV_MOVE:
				break;
			default:
				trnormals = false;
				break;
		}
	}

	if ( trnormals ) {
		s->features |= MF_TRNORMALS;
	}

	for ( i = 0, pass = s->passes; i < s->numpasses; i++, pass++ ) {
		switch ( pass->rgbgen ) {
			case RGB_GEN_LIGHTING_DIFFUSE:
				s->features |= MF_NORMALS;
				break;
			case RGB_GEN_VERTEX:
			case RGB_GEN_ONE_MINUS_VERTEX:
			case RGB_GEN_EXACT_VERTEX:
				s->features |= MF_COLORS;
				break;
			default:
				break;
		}

		switch ( pass->alphagen ) {
			case ALPHA_GEN_SPECULAR:
				s->features |= MF_NORMALS;
				break;
			case ALPHA_GEN_VERTEX:
				s->features |= MF_COLORS;
				break;
			default:
				break;
		}

		switch ( pass->tcgen ) {
			default:
				s->features |= MF_STCOORDS;
				break;
			case TC_GEN_LIGHTMAP:
				s->features |= MF_LMCOORDS;
				break;
			case TC_GEN_ENVIRONMENT:
				s->features |= MF_NORMALS;
				break;
		}
	}
}

void Shader_Finish ( shader_t *s )
{
	int i;
	shaderpass_t *pass;

	if ( !Q_stricmp (s->name, "flareShader") ) {
		s->flags |= SHADER_FLARE;
	}

	if (!s->numpasses && !(s->flags & (SHADER_NODRAW|SHADER_SKY)) && !s->fog_dist)
	{
		pass = &s->passes[s->numpasses++];
		pass = &s->passes[0];
		pass->tcgen = TC_GEN_BASE;
		pass->anim_frames[0] = Mod_LoadHiResTexture(s->name, NULL, true, false, true);//GL_FindImage (shortname, 0);
		if (!pass->anim_frames[0])
			pass->anim_frames[0] = missing_texture;
		pass->depthfunc = GL_LEQUAL;
		pass->flags = SHADER_PASS_DEPTHWRITE;
		pass->rgbgen = RGB_GEN_VERTEX;
		pass->alphagen = ALPHA_GEN_IDENTITY;
		pass->blendmode = GL_MODULATE;
		pass->numMergedPasses = 1;
		pass->flush = R_RenderMeshGeneric;
		Con_Printf("Shader %s with no passes and no surfaceparm nodraw, inserting pass\n", s->name);
	}

	if ( !s->numpasses && !s->sort ) {
		s->sort = SHADER_SORT_ADDITIVE;
		return;
	}

	if ( (s->flags & SHADER_POLYGONOFFSET) && !s->sort ) {
		s->sort = SHADER_SORT_ADDITIVE - 1;
	}

	if ( r_vertexlight.value && !s->programhandle)
	{
		// do we have a lightmap pass?
		pass = s->passes;
		for ( i = 0; i < s->numpasses; i++, pass++ )
		{
			if ( pass->flags & SHADER_PASS_LIGHTMAP )
				break;
		}

		if ( i == s->numpasses )
		{
			goto done;
		}

		// try to find pass with rgbgen set to RGB_GEN_VERTEX
		pass = s->passes;
		for ( i = 0; i < s->numpasses; i++, pass++ )
		{
			if ( pass->rgbgen == RGB_GEN_VERTEX )
				break;
		}

		if ( i < s->numpasses )
		{		// we found it
			pass->flags |= SHADER_CULL_FRONT;
			pass->flags &= ~(SHADER_PASS_BLEND|SHADER_PASS_ANIMMAP);
			pass->blendmode = 0;
			pass->flags |= SHADER_PASS_DEPTHWRITE;
			pass->alphagen = ALPHA_GEN_IDENTITY;
			pass->numMergedPasses = 1;
			pass->flush = R_RenderMeshGeneric;
			s->flags |= SHADER_DEPTHWRITE;
			s->sort = SHADER_SORT_OPAQUE;
			s->numpasses = 1;
			memcpy ( &s->passes[0], pass, sizeof(shaderpass_t) );
		}
		else
		{	// we didn't find it - simply remove all lightmap passes
			pass = s->passes;
			for ( i = 0; i < s->numpasses; i++, pass++ )
			{
				if ( pass->flags & SHADER_PASS_LIGHTMAP )
					break;
			}
			
			if ( i == s->numpasses -1 )
			{
				s->numpasses--;
			}
			else if ( i < s->numpasses - 1 )
			{
				for ( ; i < s->numpasses - 1; i++, pass++ )
				{
					memcpy ( pass, &s->passes[i+1], sizeof(shaderpass_t) );
				}
				s->numpasses--;
			}
			
			if ( s->passes[0].numtcmods )
			{
				pass = s->passes;
				for ( i = 0; i < s->numpasses; i++, pass++ )
				{
					if ( !pass->numtcmods )
						break;
				}
				
				memcpy ( &s->passes[0], pass, sizeof(shaderpass_t) );
			}
			
			s->passes[0].rgbgen = RGB_GEN_VERTEX;
			s->passes[0].alphagen = ALPHA_GEN_IDENTITY;
			s->passes[0].blendmode = 0;
			s->passes[0].flags &= ~(SHADER_PASS_BLEND|SHADER_PASS_ANIMMAP|SHADER_PASS_NOCOLORARRAY);
			s->passes[0].flags |= SHADER_PASS_DEPTHWRITE;
			s->passes[0].numMergedPasses = 1;
			s->passes[0].flush = R_RenderMeshGeneric;
			s->numpasses = 1;
			s->flags |= SHADER_DEPTHWRITE;
		}
	}
done:;

	pass = s->passes;
	for ( i = 0; i < s->numpasses; i++, pass++ ) {
		if ( !(pass->flags & SHADER_PASS_BLEND) ) {
			break;
		}
	}

	// all passes have blendfuncs
	if ( i == s->numpasses ) {
		int opaque;

		opaque = -1;
		pass = s->passes;
		for ( i = 0; i < s->numpasses; i++, pass++ ) {
			if ( (pass->blendsrc == GL_ONE) && (pass->blenddst == GL_ZERO) ) {
				opaque = i;
			}

			if ( pass->rgbgen == RGB_GEN_UNKNOWN ) { 
				if ( !s->fog_dist && !(pass->flags & SHADER_PASS_LIGHTMAP) ) 
					pass->rgbgen = RGB_GEN_IDENTITY_LIGHTING;
				else
					pass->rgbgen = RGB_GEN_IDENTITY;
			}
		}

		if ( !( s->flags & SHADER_SKY ) && !s->sort ) {
			if ( opaque == -1 )
				s->sort = SHADER_SORT_ADDITIVE;
			else if ( s->passes[opaque].flags & SHADER_PASS_ALPHAFUNC )
				s->sort = SHADER_SORT_OPAQUE + 1;
			else
				s->sort = SHADER_SORT_OPAQUE;
		}
	} else {
		int	j;
		shaderpass_t *sp;

		sp = s->passes;
		for ( j = 0; j < s->numpasses; j++, sp++ ) {
			if ( sp->rgbgen == RGB_GEN_UNKNOWN ) { 
				sp->rgbgen = RGB_GEN_IDENTITY;
			}
		}

		if ( !s->sort ) {
			if ( pass->flags & SHADER_PASS_ALPHAFUNC )
				s->sort = SHADER_SORT_OPAQUE + 1;
		}

		if ( !( s->flags & SHADER_DEPTHWRITE ) &&
			!( s->flags & SHADER_SKY ) )
		{
			pass->flags |= SHADER_PASS_DEPTHWRITE;
			s->flags |= SHADER_DEPTHWRITE;
		}
	}

	if ( s->numpasses >= 2 )
	{
		pass = s->passes;
		for ( i = 0; i < s->numpasses; )
		{
			if ( i == s->numpasses - 1 )
				break;

			pass = s->passes + i;
			Shader_SetPassFlush ( pass, pass + 1 );

			i += pass->numMergedPasses;
		}
	}

	if ( !s->sort ) {
		s->sort = SHADER_SORT_OPAQUE;
	}

	if ( (s->flags & SHADER_SKY) && (s->flags & SHADER_DEPTHWRITE) ) {
		s->flags &= ~SHADER_DEPTHWRITE;
	}

	if (s->programhandle)
	{
		if (!s->numpasses)
			s->numpasses = 1;
		s->passes->numMergedPasses = s->numpasses;
		s->passes->flush = R_RenderMeshProgram;
	}

	Shader_SetFeatures ( s );
}
/*
void Shader_UpdateRegistration (void)
{
	int i, j, l;
	shader_t *shader;
	shaderpass_t *pass;

#ifdef FIZME
	if ( chars_shader )
		chars_shader->registration_sequence = registration_sequence;

	if ( propfont1_shader )
		propfont1_shader->registration_sequence = registration_sequence;

	if ( propfont1_glow_shader )
		propfont1_glow_shader->registration_sequence = registration_sequence;

	if ( propfont2_shader )
		propfont2_shader->registration_sequence = registration_sequence;

	if ( particle_shader )
		particle_shader->registration_sequence = registration_sequence;
#endif
	shader = r_shaders;
	for (i = 0; i < MAX_SHADERS; i++, shader++)
	{
		if ( !shader->registration_sequence )
			continue;
		if ( shader->registration_sequence != registration_sequence ) {
			Shader_Free ( shader );
			shader->registration_sequence = 0;
			continue;
		}
#ifdef FIZME: skydomes
		if ( shader->flags & SHADER_SKY && shader->skydome ) {
			if ( shader->skydome->farbox_textures[0] ) {
				for ( j = 0; j < 6; j++ ) {
					if ( shader->skydome->farbox_textures[j] )
						shader->skydome->farbox_textures[j]->registration_sequence = registration_sequence;
				}
			}

			if ( shader->skydome->nearbox_textures[0] ) {
				for ( j = 0; j < 6; j++ ) {
					if ( shader->skydome->nearbox_textures[j] )
						shader->skydome->nearbox_textures[j]->registration_sequence = registration_sequence;
				}
			}
		}
#endif
		pass = shader->passes;
		for (j = 0; j < shader->numpasses; j++, pass++)
		{
			if ( pass->flags & SHADER_PASS_ANIMMAP ) {
				for (l = 0; l < pass->anim_numframes; l++) 
				{
					if ( pass->anim_frames[l] )
						pass->anim_frames[l]->registration_sequence = registration_sequence;
				}
			} else if ( pass->flags & SHADER_PASS_VIDEOMAP ) {
				// Shader_RunCinematic will do the job
//				pass->cin->frame = -1;
			} else if ( !(pass->flags & SHADER_PASS_LIGHTMAP) ) {
				if ( pass->anim_frames[0] )
					pass->anim_frames[0]->registration_sequence = registration_sequence;
			} 
		}
	}
}
*/
/*
void Shader_UploadCinematic (shader_t *shader)
{
	int j;
	shaderpass_t *pass;

	// upload cinematics
	pass = shader->passes;
	for ( j = 0; j < shader->numpasses; j++, pass++ ) {
		if ( pass->flags & SHADER_PASS_VIDEOMAP ) {
			pass->anim_frames[0] = GL_ResampleCinematicFrame ( pass );
		}
	}
}

void Shader_RunCinematic (void)
{
	int i, j;
	shader_t *shader;
	shaderpass_t *pass;

	shader = r_shaders;
	for ( i = 0; i < MAX_SHADERS; i++, shader++ ) {
		if ( !shader->registration_sequence )
			continue;
		if ( !(shader->flags & SHADER_VIDEOMAP) )
			continue;

		pass = shader->passes;
		for ( j = 0; j < shader->numpasses; j++, pass++ ) {
			if ( !(pass->flags & SHADER_PASS_VIDEOMAP) )
				continue;

			// reinitialize
			if ( pass->cin->frame == -1 ) {
				GL_StopCinematic ( pass->cin );
				GL_PlayCinematic( pass->cin );

				if ( pass->cin->time == 0 ) {		// not found
					pass->flags &= ~SHADER_PASS_VIDEOMAP;
					Z_Free ( pass->cin );
				}

				continue;
			}

			GL_RunCinematic ( pass->cin );
		}
	}
}
*/

void Shader_DefaultBSP(char *shortname, shader_t *s)
{
	shaderpass_t *pass;

	int bumptex;
	extern cvar_t gl_bump;

	if (gl_config.arb_texture_env_dot3)
	{
		if (gl_bump.value)
			bumptex = Mod_LoadHiResTexture(va("normalmaps/%s", shortname), NULL, true, false, false);//GL_FindImage (shortname, 0);
		else
			bumptex = 0;
	}
	else
		bumptex = 0;

	if (bumptex)
	{
		pass = &s->passes[s->numpasses++];
		pass->flags = SHADER_PASS_DELUXMAP | SHADER_PASS_DEPTHWRITE | SHADER_PASS_NOCOLORARRAY;
		pass->tcgen = TC_GEN_LIGHTMAP;
		pass->anim_frames[0] = 0;
		pass->depthfunc = GL_LEQUAL;
		pass->blendmode = GL_REPLACE;
		pass->alphagen = ALPHA_GEN_IDENTITY;
		pass->rgbgen = RGB_GEN_IDENTITY;
		pass->numMergedPasses = 2;
		if (pass->numMergedPasses > gl_mtexarbable)
			pass->numMergedPasses = gl_mtexarbable;
		pass->flush = R_RenderMeshCombined;


		pass = &s->passes[s->numpasses++];
		pass->flags = SHADER_PASS_BLEND | SHADER_PASS_NOCOLORARRAY;
		pass->tcgen = TC_GEN_BASE;
		pass->anim_frames[0] = bumptex;
		pass->anim_numframes = 1;
		pass->blendmode = GL_DOT3_RGB_ARB;
		pass->rgbgen = RGB_GEN_IDENTITY;
		pass->alphagen = ALPHA_GEN_IDENTITY;


		pass = &s->passes[s->numpasses++];
		pass->flags = SHADER_PASS_NOCOLORARRAY | SHADER_PASS_BLEND;
		pass->tcgen = TC_GEN_BASE;
		pass->anim_frames[0] = Mod_LoadHiResTexture(shortname, NULL, true, false, true);//GL_FindImage (shortname, 0);
		if (!pass->anim_frames[0])
			pass->anim_frames[0] = missing_texture;
		pass->depthfunc = GL_LEQUAL;
		pass->blendsrc = GL_ZERO;
		pass->blenddst = GL_SRC_COLOR;
		pass->blendmode = GL_MODULATE;
		pass->alphagen = ALPHA_GEN_IDENTITY;
		pass->rgbgen = RGB_GEN_IDENTITY;
		pass->numMergedPasses = 2;
		pass->flush = R_RenderMeshMultitextured;

		pass = &s->passes[s->numpasses++];
		pass->flags = SHADER_PASS_LIGHTMAP | SHADER_PASS_NOCOLORARRAY | SHADER_PASS_BLEND;
		pass->tcgen = TC_GEN_LIGHTMAP;
		pass->anim_frames[0] = 0;
		pass->depthfunc = GL_LEQUAL;
		pass->blendsrc = GL_ZERO;
		pass->blenddst = GL_SRC_COLOR;
		pass->blendmode = GL_MODULATE;
		pass->alphagen = ALPHA_GEN_IDENTITY;
		pass->rgbgen = RGB_GEN_IDENTITY;
		pass->numMergedPasses = 1;
		pass->flush = R_RenderMeshGeneric;
	}
	else
	{
		pass = &s->passes[0];
		pass->flags = SHADER_PASS_LIGHTMAP | SHADER_PASS_DEPTHWRITE | SHADER_PASS_NOCOLORARRAY;
		pass->tcgen = TC_GEN_LIGHTMAP;
		pass->anim_frames[0] = 0;
		pass->depthfunc = GL_LEQUAL;
		pass->blendmode = GL_REPLACE;
		pass->alphagen = ALPHA_GEN_IDENTITY;
		pass->rgbgen = RGB_GEN_IDENTITY;
		pass->numMergedPasses = 2;

		if ( qglMTexCoord2fSGIS )
		{
			pass->numMergedPasses = 2;
			pass->flush = R_RenderMeshMultitextured;
		}
		else
		{
			pass->numMergedPasses = 1;
			pass->flush = R_RenderMeshGeneric;
		}
			
		pass = &s->passes[1];
		pass->flags = SHADER_PASS_BLEND | SHADER_PASS_NOCOLORARRAY;
		pass->tcgen = TC_GEN_BASE;
		pass->anim_frames[0] = Mod_LoadHiResTexture(shortname, NULL, true, false, true);//GL_FindImage (shortname, 0);
		if (!pass->anim_frames[0])
			pass->anim_frames[0] = missing_texture;
		pass->anim_numframes = 1;
		pass->blendsrc = GL_ZERO;
		pass->blenddst = GL_SRC_COLOR;
		pass->blendmode = GL_MODULATE;
		pass->depthfunc = GL_LEQUAL;
		pass->rgbgen = RGB_GEN_IDENTITY;
		pass->alphagen = ALPHA_GEN_IDENTITY;

		pass->numMergedPasses = 1;
		pass->flush = R_RenderMeshGeneric;

		if ( !pass->anim_frames[0] ) {
			Con_DPrintf (S_WARNING "Shader %s has a stage with no image: %s.\n", s->name, shortname );
			pass->anim_frames[0] = missing_texture;
		}

		s->numpasses = 2;
	}
	s->numdeforms = 0;
	s->flags = SHADER_DEPTHWRITE|SHADER_CULL_FRONT;
	s->features = MF_STCOORDS|MF_LMCOORDS|MF_TRNORMALS;
	s->sort = SHADER_SORT_OPAQUE;
	s->registration_sequence = 1;//fizme: registration_sequence;
}

void Shader_DefaultBSPVertex(char *shortname, shader_t *s)
{
	shaderpass_t *pass;
	pass = &s->passes[0];
	pass->tcgen = TC_GEN_BASE;
	pass->anim_frames[0] = Mod_LoadHiResTexture(shortname, NULL, true, false, true);//GL_FindImage (shortname, 0);
	if (!pass->anim_frames[0])
		pass->anim_frames[0] = missing_texture;
	pass->depthfunc = GL_LEQUAL;
	pass->flags = SHADER_PASS_DEPTHWRITE;
	pass->rgbgen = RGB_GEN_VERTEX;
	pass->alphagen = ALPHA_GEN_IDENTITY;
	pass->blendmode = GL_MODULATE;
	pass->numMergedPasses = 1;
	pass->flush = R_RenderMeshGeneric;

	if ( !pass->anim_frames[0] ) {
		Con_DPrintf (S_WARNING "Shader %s has a stage with no image: %s.\n", s->name, shortname );
		pass->anim_frames[0] = missing_texture;
	}

	s->numpasses = 1;
	s->numdeforms = 0;
	s->flags = SHADER_DEPTHWRITE|SHADER_CULL_FRONT;
	s->features = MF_STCOORDS|MF_COLORS|MF_TRNORMALS;
	s->sort = SHADER_SORT_OPAQUE;
	s->registration_sequence = 1;//fizme: registration_sequence;
}
void Shader_DefaultBSPFlare(char *shortname, shader_t *s)
{
	shaderpass_t *pass;
	pass = &s->passes[0];
	pass->flags = SHADER_PASS_BLEND | SHADER_PASS_NOCOLORARRAY;
	pass->blendsrc = GL_ONE;
	pass->blenddst = GL_ONE;
	pass->blendmode = GL_MODULATE;
	pass->anim_frames[0] = Mod_LoadHiResTexture(shortname, NULL, true, true, true);//GL_FindImage (shortname, 0);
	if (!pass->anim_frames[0])
		pass->anim_frames[0] = missing_texture;
	pass->depthfunc = GL_LEQUAL;
	pass->rgbgen = RGB_GEN_VERTEX;
	pass->alphagen = ALPHA_GEN_IDENTITY;
	pass->numtcmods = 0;
	pass->tcgen = TC_GEN_BASE;
	pass->numMergedPasses = 1;
	pass->flush = R_RenderMeshGeneric;

	if ( !pass->anim_frames[0] ) {
		Con_DPrintf (S_WARNING "Shader %s has a stage with no image: %s.\n", s->name, shortname );
		pass->anim_frames[0] = missing_texture;
	}

	s->numpasses = 1;
	s->numdeforms = 0;
	s->flags = SHADER_FLARE;
	s->features = MF_STCOORDS|MF_COLORS;
	s->sort = SHADER_SORT_ADDITIVE;
	s->registration_sequence = 1;//fizme: registration_sequence;
}
void Shader_DefaultSkin(char *shortname, shader_t *s)
{
	int tex;
	shaderpass_t *pass;

	s->numpasses = 0;

	tex = Mod_LoadHiResTexture(shortname, NULL, true, true, true);
	if (!tex)
		tex = missing_texture;
//	if (tex)
	{
		pass = &s->passes[s->numpasses++];
		pass->flags = SHADER_PASS_DEPTHWRITE;
		pass->anim_frames[0] = tex;
		pass->depthfunc = GL_LEQUAL;
		pass->rgbgen = RGB_GEN_LIGHTING_DIFFUSE;
		pass->numtcmods = 0;
		pass->tcgen = TC_GEN_BASE;
		pass->blendsrc = GL_SRC_ALPHA;
		pass->blenddst = GL_ONE_MINUS_SRC_ALPHA;
		pass->blendmode = GL_MODULATE;
		pass->numMergedPasses = 1;
		pass->flush = R_RenderMeshGeneric;
		if (!pass->anim_frames[0])
		{
			Con_DPrintf (S_WARNING "Shader %s has a stage with no image: %s.\n", s->name, shortname );
			pass->anim_frames[0] = missing_texture;
		}
	}

	tex = Mod_LoadHiResTexture(va("%s_shirt", shortname), NULL, true, true, true);
	if (tex)
	{
		pass = &s->passes[s->numpasses++];
		pass->flags = SHADER_PASS_BLEND;
		pass->anim_frames[0] = tex;
		pass->depthfunc = GL_EQUAL;
		pass->rgbgen = RGB_GEN_TOPCOLOR;
		pass->numtcmods = 0;
		pass->tcgen = TC_GEN_BASE;
		pass->blendsrc = GL_ONE;
		pass->blenddst = GL_ONE;
		pass->blendmode = GL_MODULATE;
		pass->numMergedPasses = 1;
		pass->flush = R_RenderMeshGeneric;
		if (!pass->anim_frames[0])
		{
			Con_DPrintf (S_WARNING "Shader %s has a stage with no image: %s.\n", s->name, shortname );
			pass->anim_frames[0] = missing_texture;
		}
	}

	tex = Mod_LoadHiResTexture(va("%s_pants", shortname), NULL, true, true, true);
	if (tex)
	{
		pass = &s->passes[s->numpasses++];
		pass->flags = SHADER_PASS_BLEND;
		pass->anim_frames[0] = tex;
		pass->depthfunc = GL_EQUAL;
		pass->rgbgen = RGB_GEN_BOTTOMCOLOR;
		pass->numtcmods = 0;
		pass->tcgen = TC_GEN_BASE;
		pass->blendsrc = GL_ONE;
		pass->blenddst = GL_ONE;
		pass->blendmode = GL_MODULATE;
		pass->numMergedPasses = 1;
		pass->flush = R_RenderMeshGeneric;
		if (!pass->anim_frames[0])
		{
			Con_DPrintf (S_WARNING "Shader %s has a stage with no image: %s.\n", s->name, shortname );
			pass->anim_frames[0] = missing_texture;
		}
	}

	tex = Mod_LoadHiResTexture(va("%s_glow", shortname), NULL, true, true, true);
	if (tex)
	{
		pass = &s->passes[s->numpasses++];
		pass->flags = SHADER_PASS_BLEND;
		pass->anim_frames[0] = tex;
		pass->depthfunc = GL_EQUAL;
		pass->rgbgen = RGB_GEN_LIGHTING_DIFFUSE;
		pass->numtcmods = 0;
		pass->tcgen = TC_GEN_BASE;
		pass->blendsrc = GL_ONE;
		pass->blenddst = GL_ONE;
		pass->blendmode = GL_MODULATE;
		pass->numMergedPasses = 1;
		pass->flush = R_RenderMeshGeneric;
		if (!pass->anim_frames[0])
		{
			Con_DPrintf (S_WARNING "Shader %s has a stage with no image: %s.\n", s->name, shortname);
			pass->anim_frames[0] = missing_texture;
		}
	}

	s->numdeforms = 0;
	s->flags = SHADER_DEPTHWRITE|SHADER_CULL_FRONT;
	s->features = MF_STCOORDS|MF_NORMALS;
	s->sort = SHADER_SORT_OPAQUE;
	s->registration_sequence = 1;//fizme: registration_sequence;
}
void Shader_DefaultSkinShell(char *shortname, shader_t *s)
{
	shaderpass_t *pass;
	pass = &s->passes[0];
	pass->flags = SHADER_PASS_DEPTHWRITE | SHADER_PASS_BLEND;
	pass->anim_frames[0] = Mod_LoadHiResTexture(shortname, NULL, true, true, true);//GL_FindImage (shortname, 0);
	if (!pass->anim_frames[0])
		pass->anim_frames[0] = missing_texture;
	pass->depthfunc = GL_LEQUAL;
	pass->rgbgen = RGB_GEN_ENTITY;
	pass->alphagen = ALPHA_GEN_ENTITY;
	pass->numtcmods = 0;
	pass->tcgen = TC_GEN_BASE;
	pass->blendsrc = GL_SRC_ALPHA;
	pass->blenddst = GL_ONE_MINUS_SRC_ALPHA;
	pass->blendmode = GL_MODULATE;
	pass->numMergedPasses = 1;
	pass->flush = R_RenderMeshGeneric;

	if ( !pass->anim_frames[0] ) {
		Con_DPrintf (S_WARNING "Shader %s has a stage with no image: %s.\n", s->name, shortname );
		pass->anim_frames[0] = missing_texture;
	}

	s->numpasses = 1;
	s->numdeforms = 0;
	s->flags = SHADER_DEPTHWRITE|SHADER_CULL_FRONT;
	s->features = MF_STCOORDS|MF_NORMALS;
	s->sort = SHADER_SORT_OPAQUE;
	s->registration_sequence = 1;//fizme: registration_sequence;
}
void Shader_Default2D(char *shortname, shader_t *s)
{
	mpic_t *mp;

	shaderpass_t *pass;
	pass = &s->passes[0];
	pass->flags = SHADER_PASS_BLEND | SHADER_PASS_NOCOLORARRAY;
	pass->blendsrc = GL_SRC_ALPHA;
	pass->blenddst = GL_ONE_MINUS_SRC_ALPHA;
	pass->blendmode = GL_MODULATE;
	pass->anim_frames[0] = Mod_LoadHiResTexture(shortname, NULL, false, true, true);//GL_FindImage (shortname, IT_NOPICMIP|IT_NOMIPMAP);
	if (!pass->anim_frames[0])
	{
		mp = Draw_SafeCachePic(va("%s.lmp", shortname));
		if (mp)
			pass->anim_frames[0] = *(int*)mp->data;

		if (!pass->anim_frames[0])
			pass->anim_frames[0] = missing_texture;
	}
	pass->depthfunc = GL_LEQUAL;
	pass->rgbgen = RGB_GEN_VERTEX;
	pass->alphagen = ALPHA_GEN_VERTEX;
	pass->numtcmods = 0;
	pass->tcgen = TC_GEN_BASE;
	pass->numMergedPasses = 1;
	pass->flush = R_RenderMeshGeneric;

	if ( !pass->anim_frames[0] ) {
		Con_DPrintf (S_WARNING "Shader %s has a stage with no image: %s.\n", s->name, shortname );
		pass->anim_frames[0] = missing_texture;
	}

	s->numpasses = 1;
	s->numdeforms = 0;
	s->flags = SHADER_NOPICMIP|SHADER_NOMIPMAPS|SHADER_BLEND;
	s->features = MF_STCOORDS|MF_COLORS;
	s->sort = SHADER_SORT_ADDITIVE;
	s->registration_sequence = 1;//fizme: registration_sequence;
}

qboolean Shader_ParseShader(char *shortname, char *usename, shader_t *s)
{
	unsigned int offset = 0, length;
	char path[MAX_QPATH];
	char *buf = NULL, *ts = NULL;

	Shader_GetPathAndOffset( shortname, &ts, &offset );

	if ( ts )
	{
		Com_sprintf ( path, sizeof(path), "%s", ts );
		length = FS_LoadFile ( path, (void **)&buf );
	}
	else
		length = 0;

	// the shader is in the shader scripts
	if ( ts && buf && (offset < length) )
	{
		char *file, *token;


		file = buf + offset;
		token = COM_ParseExt (&file, true);
		if ( !file || token[0] != '{' )
		{
			FS_FreeFile(buf);
			return false;
		}


		memset ( s, 0, sizeof( shader_t ) );

		Com_sprintf ( s->name, MAX_QPATH, usename );

	// set defaults
		s->flags = SHADER_CULL_FRONT;
		s->registration_sequence = 1;//fizme: registration_sequence;

//		if (!strcmp(COM_FileExtension(ts), "rscript"))
//		{
//			Shader_DefaultBSP(shortname, s);
//		}

		while ( file )
		{
			token = COM_ParseExt (&file, true);

			if ( !token[0] )
				continue;
			else if ( token[0] == '}' )
				break;
			else if ( token[0] == '{' )
				Shader_Readpass ( s, &file );
			else if ( Shader_Parsetok (s, NULL, shaderkeys, token, &file ) )
				break;
		}

		Shader_Finish ( s );

		FS_FreeFile(buf);
		return true;
	}

	if (buf)
		FS_FreeFile(buf);

	return false;
}

int R_LoadShader ( char *name, void(*defaultgen)(char *name, shader_t*))
{
	int i, f = -1;
	char shortname[MAX_QPATH];
	shader_t *s;

	COM_StripExtension ( name, shortname, sizeof(shortname));

	COM_CleanUpPath(shortname);

	// test if already loaded
	for (i = 0; i < MAX_SHADERS; i++)
	{
		if (!r_shaders[i].registration_sequence)
		{
			if ( f == -1 )	// free shader
				f = i;
			continue;
		}

		if (!Q_stricmp (shortname, r_shaders[i].name) )
		{
			r_shaders[i].registration_sequence = 1;//fizme: registration_sequence;
			return i;
		}
	}

	if ( f == -1 )
	{
		Sys_Error( "R_LoadShader: Shader limit exceeded.");
		return f;
	}

	s = &r_shaders[f];

	if (gl_config.arb_shader_objects)
	{
		if (Shader_ParseShader(va("%s_glsl", shortname), shortname, s))
			return f;
	}
	if (Shader_ParseShader(shortname, shortname, s))
		return f;

	// make a default shader

	if (defaultgen)
	{
		memset ( s, 0, sizeof( shader_t ) );
		Com_sprintf ( s->name, MAX_QPATH, shortname );
		defaultgen(shortname, s);

		return f;
	}
	return -1;
}

shader_t *R_RegisterPic (char *name) 
{
	return &r_shaders[R_LoadShader (name, Shader_Default2D)];
}

shader_t *R_RegisterShader (char *name)
{
	return &r_shaders[R_LoadShader (name, Shader_DefaultBSP)];
}

shader_t *R_RegisterShader_Vertex (char *name)
{
	return &r_shaders[R_LoadShader (name, Shader_DefaultBSPVertex)];
}

shader_t *R_RegisterShader_Flare (char *name)
{
	return &r_shaders[R_LoadShader (name, Shader_DefaultBSPFlare)];
}

shader_t *R_RegisterSkin (char *name)
{
	return &r_shaders[R_LoadShader (name, Shader_DefaultSkin)];
}
shader_t *R_RegisterCustom (char *name, void(*defaultgen)(char *name, shader_t*))
{
	int i;
	i = R_LoadShader (name, defaultgen);
	if (i < 0)
		return NULL;
	return &r_shaders[i];
}
#endif

