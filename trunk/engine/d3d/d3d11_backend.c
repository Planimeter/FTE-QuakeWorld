#include "quakedef.h"
#ifdef D3D11QUAKE
#include "glquake.h"
#include "gl_draw.h"
#include "shader.h"

#define COBJMACROS
#include <d3d11.h>

extern ID3D11Device *pD3DDev11;
extern ID3D11DeviceContext *d3ddevctx;

extern cvar_t r_shadow_realtime_world_lightmaps;
extern cvar_t gl_overbright;
extern cvar_t r_portalrecursion;

void D3D11_TerminateShadowMap(void);
void D3D11BE_BeginShadowmapFace(void);

//#define d3dcheck(foo) foo
#define d3dcheck(foo) do{HRESULT err = foo; if (FAILED(err)) Sys_Error("D3D reported error on backend line %i - error 0x%x\n", __LINE__, (unsigned int)err);} while(0)

#define MAX_TMUS 16

static void BE_RotateForEntity (const entity_t *e, const model_t *mod);
void D3D11BE_SetupLightCBuffer(dlight_t *l, vec3_t colour);
texid_t D3D11_GetShadowMap(int id);

/*========================================== tables for deforms =====================================*/
#if 0
#define frand() (rand()*(1.0/RAND_MAX))
#define FTABLE_SIZE		1024
#define FTABLE_CLAMP(x)	(((int)((x)*FTABLE_SIZE) & (FTABLE_SIZE-1)))
#define FTABLE_EVALUATE(table,x) (table ? table[FTABLE_CLAMP(x)] : frand()*((x)-floor(x)))

static	float	r_sintable[FTABLE_SIZE];
static	float	r_triangletable[FTABLE_SIZE];
static	float	r_squaretable[FTABLE_SIZE];
static	float	r_sawtoothtable[FTABLE_SIZE];
static	float	r_inversesawtoothtable[FTABLE_SIZE];

static float *FTableForFunc ( unsigned int func )
{
	switch (func)
	{
		case SHADER_FUNC_SIN:
			return r_sintable;

		case SHADER_FUNC_TRIANGLE:
			return r_triangletable;

		case SHADER_FUNC_SQUARE:
			return r_squaretable;

		case SHADER_FUNC_SAWTOOTH:
			return r_sawtoothtable;

		case SHADER_FUNC_INVERSESAWTOOTH:
			return r_inversesawtoothtable;
	}

	//bad values allow us to crash (so I can debug em)
	return NULL;
}

static void FTable_Init(void)
{
	unsigned int i;
	double t;
	for (i = 0; i < FTABLE_SIZE; i++)
	{
		t = (double)i / (double)FTABLE_SIZE;

		r_sintable[i] = sin(t * 2*M_PI);

		if (t < 0.25)
			r_triangletable[i] = t * 4.0;
		else if (t < 0.75)
			r_triangletable[i] = 2 - 4.0 * t;
		else
			r_triangletable[i] = (t - 0.75) * 4.0 - 1.0;

		if (t < 0.5)
			r_squaretable[i] = 1.0f;
		else
			r_squaretable[i] = -1.0f;

		r_sawtoothtable[i] = t;
		r_inversesawtoothtable[i] = 1.0 - t;
	}
}

typedef vec3_t mat3_t[3];
static mat3_t axisDefault={{1, 0, 0},
					{0, 1, 0},
					{0, 0, 1}};

static void Matrix3_Transpose (mat3_t in, mat3_t out)
{
	out[0][0] = in[0][0];
	out[1][1] = in[1][1];
	out[2][2] = in[2][2];

	out[0][1] = in[1][0];
	out[0][2] = in[2][0];
	out[1][0] = in[0][1];
	out[1][2] = in[2][1];
	out[2][0] = in[0][2];
	out[2][1] = in[1][2];
}
static void Matrix3_Multiply_Vec3 (const mat3_t a, const vec3_t b, vec3_t product)
{
	product[0] = a[0][0]*b[0] + a[0][1]*b[1] + a[0][2]*b[2];
	product[1] = a[1][0]*b[0] + a[1][1]*b[1] + a[1][2]*b[2];
	product[2] = a[2][0]*b[0] + a[2][1]*b[1] + a[2][2]*b[2];
}

static int Matrix3_Compare(const mat3_t in, const mat3_t out)
{
	return !memcmp(in, out, sizeof(mat3_t));
}
#endif
/*================================================*/

//global constant-buffer
typedef struct
{
	float m_view[16];
	float m_projection[16];
	vec3_t v_eyepos; float v_time;
	vec3_t e_light_ambient; float pad1;
	vec3_t e_light_dir; float pad2;
	vec3_t e_light_mul; float pad3;
} cbuf_view_t;

typedef struct
{
	float l_cubematrix[16];
	vec3_t l_lightposition; float padl1;
	vec3_t l_colour; float pad2;
	vec3_t l_lightcolourscale; float l_lightradius;
	vec4_t l_shadowmapproj;
	vec2_t l_shadowmapscale; vec2_t pad3;
} cbuf_light_t;

//entity-specific constant-buffer
typedef struct
{
	float m_model[16];
	vec3_t e_eyepos;
	float e_time;
	vec3_t e_light_ambient; float pad1;
	vec3_t e_light_dir; float pad2;
	vec3_t e_light_mul; float pad3;
	vec4_t e_lmscale[4];
} cbuf_entity_t;

//vertex attributes
typedef struct
{
	vecV_t coord;
	vec2_t tex;
	vec2_t lm;
	vec3_t ndir;
	vec3_t sdir;
	vec3_t tdir;
	byte_vec4_t colorsb;
} vbovdata_t;

typedef struct blendstates_s
{
	struct blendstates_s *next;
	ID3D11BlendState *val;
	unsigned int bits;
} blendstates_t;

typedef struct
{
	unsigned int inited;

	backendmode_t mode;
	unsigned int flags;

	float	identitylighting;
	float		curtime;
	const entity_t	*curentity;
	const dlight_t	*curdlight;
	vec3_t		curdlight_colours;
	shader_t	*curshader;
	shader_t	*depthonly;
	texnums_t	*curtexnums;
	int			curvertdecl;
	unsigned int shaderbits;
	unsigned int curcull;
	float depthbias;
	float depthfactor;
	float m_model[16];
	unsigned int lastpasscount;
	vbo_t *batchvbo;
	batch_t *curbatch;
	batch_t dummybatch;
	vec4_t lightshadowmapproj;
	vec2_t lightshadowmapscale;

	unsigned int curlmode;
	shader_t	*shader_rtlight[LSHADER_MODES];
	texid_t		curtex[MAX_TMUS];
	unsigned int tmuflags[MAX_TMUS];
	ID3D11SamplerState *cursamplerstate[MAX_TMUS];
	ID3D11SamplerState *sampstate[(SHADER_PASS_NEAREST|SHADER_PASS_CLAMP|SHADER_PASS_DEPTHCMP)+1];
	ID3D11DepthStencilState *depthstates[1u<<4];	//index, its fairly short.
	blendstates_t *blendstates;	//list. this could get big.

	mesh_t		**meshlist;
	unsigned int nummeshes;

#define NUMECBUFFERS 8
	ID3D11Buffer *lcbuffer;
	ID3D11Buffer *vcbuffer;
	ID3D11Buffer *ecbuffers[NUMECBUFFERS];
	int ecbufferidx;

	unsigned int wbatch;
	unsigned int maxwbatches;
	batch_t *wbatches;

	qboolean textureschanged;
	ID3D11ShaderResourceView *pendingtextures[MAX_TMUS];

	float depthrange;

	qboolean purgevertexstream;
#define NUMVBUFFERS 3
	ID3D11Buffer *vertexstream[NUMVBUFFERS];
	int vertexstreamcycle;
	int vertexstreamoffset;
	qboolean purgeindexstream;
#define NUMIBUFFERS 3
	ID3D11Buffer *indexstream[NUMIBUFFERS];
	int indexstreamcycle;
	int indexstreamoffset;


	int numlivevbos;
	int numliveshadowbuffers;
} d3d11backend_t;

#define VERTEXSTREAMSIZE (1024*1024*2)	//2mb = 1 PAE jumbo page

#define DYNVBUFFSIZE 65536
#define DYNIBUFFSIZE 65536

static d3d11backend_t shaderstate;

extern int be_maxpasses;

static void BE_CreateSamplerStates(void)
{
	D3D11_SAMPLER_DESC sampdesc;
	int flags;
	for (flags = 0; flags <= (SHADER_PASS_CLAMP|SHADER_PASS_NEAREST|SHADER_PASS_DEPTHCMP); flags++)
	{
		if (flags & SHADER_PASS_DEPTHCMP)
		{
			if (flags & SHADER_PASS_NEAREST)
				sampdesc.Filter = D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
			else
				sampdesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
			sampdesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
		}
		else
		{
			if (flags & SHADER_PASS_NEAREST)
				sampdesc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			else
				sampdesc.Filter = /*D3D11_FILTER_MIN_MAG_MIP_POINT;*/D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;/*D3D11_FILTER_MIN_MAG_MIP_LINEAR*/;
			sampdesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		}
		if (flags & SHADER_PASS_CLAMP)
		{
			sampdesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			sampdesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			sampdesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		}
		else
		{
			sampdesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			sampdesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			sampdesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		}
		sampdesc.MipLODBias = 0.0f;
		sampdesc.MaxAnisotropy = 1;
		sampdesc.BorderColor[0] = 0;
		sampdesc.BorderColor[1] = 0;
		sampdesc.BorderColor[2] = 0;
		sampdesc.BorderColor[3] = 0;
		sampdesc.MinLOD = 0;
		sampdesc.MaxLOD = D3D11_FLOAT32_MAX;

		ID3D11Device_CreateSamplerState(pD3DDev11, &sampdesc, &shaderstate.sampstate[flags]);
	}
}
static void BE_DestroyVariousStates(void)
{
	blendstates_t *bs;
	int flags;
	int i;
	
	for (i = 0; i < MAX_TMUS/*shaderstate.lastpasscount*/; i++)
	{
		shaderstate.cursamplerstate[i] = NULL;
	}
	if (d3ddevctx && i)
		ID3D11DeviceContext_PSSetSamplers(d3ddevctx, 0, i, shaderstate.cursamplerstate);

	for (flags = 0; flags <= (SHADER_PASS_CLAMP|SHADER_PASS_NEAREST|SHADER_PASS_DEPTHCMP); flags++)
	{
		if (shaderstate.sampstate[flags])
			ID3D11SamplerState_Release(shaderstate.sampstate[flags]);
		shaderstate.sampstate[flags] = NULL;
	}

	if (d3ddevctx)
		ID3D11DeviceContext_OMSetDepthStencilState(d3ddevctx, NULL, 0);
	for (i = 0; i < (1u<<4); i++)
	{
		if (shaderstate.depthstates[i])
			ID3D11DepthStencilState_Release(shaderstate.depthstates[i]);
		shaderstate.depthstates[i] = NULL;
	}

	if (d3ddevctx)
		ID3D11DeviceContext_OMSetBlendState(d3ddevctx, NULL, NULL, 0xffffffff);
	//hopefully the caches inside shaders should get flushed too...
	while(shaderstate.blendstates)
	{
		bs = shaderstate.blendstates;
		shaderstate.blendstates = bs->next;

		if (bs->val)
			ID3D11BlendState_Release(bs->val);
		BZ_Free(bs);
	}

	for (i = 0; i < NUMIBUFFERS; i++)
	{
		if (shaderstate.indexstream[i])
			ID3D11Buffer_Release(shaderstate.indexstream[i]);
		shaderstate.indexstream[i] = NULL;
	}
	
	for (i = 0; i < NUMVBUFFERS; i++)
	{
		if (shaderstate.vertexstream[i])
			ID3D11Buffer_Release(shaderstate.vertexstream[i]);
		shaderstate.vertexstream[i] = NULL;
	}

	if (shaderstate.lcbuffer)
		ID3D11Buffer_Release(shaderstate.lcbuffer);
	shaderstate.lcbuffer = NULL;

	if (shaderstate.vcbuffer)
		ID3D11Buffer_Release(shaderstate.vcbuffer);
	shaderstate.vcbuffer = NULL;

	for (i = 0; i < NUMECBUFFERS; i++)
	{
		if (shaderstate.ecbuffers[i])
			ID3D11Buffer_Release(shaderstate.ecbuffers[i]);
		shaderstate.ecbuffers[i] = NULL;
	}

	//make sure the device doesn't have any textures still referenced.
	for (i = 0; i < MAX_TMUS/*shaderstate.lastpasscount*/; i++)
	{
		shaderstate.pendingtextures[i] = NULL;
	}
	if (d3ddevctx && i)
		ID3D11DeviceContext_PSSetShaderResources(d3ddevctx, 0, i, shaderstate.pendingtextures);
}

static void BE_ApplyTMUState(unsigned int tu, unsigned int flags)
{
	ID3D11SamplerState *nstate;

	flags = flags & (SHADER_PASS_CLAMP|SHADER_PASS_NEAREST|SHADER_PASS_DEPTHCMP);
	nstate = shaderstate.sampstate[flags];
	if (nstate != shaderstate.cursamplerstate[tu])
	{
		shaderstate.cursamplerstate[tu] = nstate;

		//fixme: is it significant to bulk-apply this later?
		ID3D11DeviceContext_PSSetSamplers(d3ddevctx, tu, 1, &nstate);
	}
	/*
	if ((flags ^ shaderstate.tmuflags[tu]) & (SHADER_PASS_NEAREST|SHADER_PASS_CLAMP))
	{
		D3D11_SAMPLER_DESC sampdesc;
		ID3D11SamplerState *sstate;

		shaderstate.tmuflags[tu] = flags;

		if (flags & SHADER_PASS_NEAREST)
			sampdesc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
		else
			sampdesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		if (flags & SHADER_PASS_CLAMP)
		{
			sampdesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			sampdesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			sampdesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		}
		else
		{
			sampdesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			sampdesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			sampdesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		}
		sampdesc.MipLODBias = 0.0f;
		sampdesc.MaxAnisotropy = 1;
		sampdesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		sampdesc.BorderColor[0] = 0;
		sampdesc.BorderColor[1] = 0;
		sampdesc.BorderColor[2] = 0;
		sampdesc.BorderColor[3] = 0;
		sampdesc.MinLOD = 0;
		sampdesc.MaxLOD = D3D11_FLOAT32_MAX;

		if (!FAILED(ID3D11Device_CreateSamplerState(pD3DDev11, &sampdesc, &sstate)))
		{
			ID3D11DeviceContext_PSSetSamplers(d3ddevctx, tu, 1, &sstate);
			ID3D11SamplerState_Release(sstate);
		}
	}
	*/
}

static void *D3D11BE_GenerateBlendState(unsigned int bits)
{
	D3D11_BLEND_DESC  blend = {0};
	ID3D11BlendState *newblendstate;
	blend.IndependentBlendEnable = FALSE;
	blend.AlphaToCoverageEnable = FALSE;	//FIXME

	if (bits & SBITS_BLEND_BITS)
	{
		switch(bits & SBITS_SRCBLEND_BITS)
		{
		case SBITS_SRCBLEND_ZERO:					blend.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;				break;
		case SBITS_SRCBLEND_ONE:					blend.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;				break;
		case SBITS_SRCBLEND_DST_COLOR:				blend.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_COLOR;		break;
		case SBITS_SRCBLEND_ONE_MINUS_DST_COLOR:	blend.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_DEST_COLOR;	break;
		case SBITS_SRCBLEND_SRC_ALPHA:				blend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;			break;
		case SBITS_SRCBLEND_ONE_MINUS_SRC_ALPHA:	blend.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_SRC_ALPHA;		break;
		case SBITS_SRCBLEND_DST_ALPHA:				blend.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_ALPHA;		break;
		case SBITS_SRCBLEND_ONE_MINUS_DST_ALPHA:	blend.RenderTarget[0].SrcBlend = D3D11_BLEND_INV_DEST_ALPHA;	break;
		case SBITS_SRCBLEND_ALPHA_SATURATE:			blend.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA_SAT;		break;
		default:	Sys_Error("Bad shader blend src\n"); return NULL;
		}
		switch(bits & SBITS_DSTBLEND_BITS)
		{
		case SBITS_DSTBLEND_ZERO:					blend.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;				break;
		case SBITS_DSTBLEND_ONE:					blend.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;				break;
		case SBITS_DSTBLEND_SRC_ALPHA:				blend.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_ALPHA;		break;
		case SBITS_DSTBLEND_ONE_MINUS_SRC_ALPHA:	blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;	break;
		case SBITS_DSTBLEND_DST_ALPHA:				blend.RenderTarget[0].DestBlend = D3D11_BLEND_DEST_ALPHA;		break;
		case SBITS_DSTBLEND_ONE_MINUS_DST_ALPHA:	blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_DEST_ALPHA;	break;
		case SBITS_DSTBLEND_SRC_COLOR:				blend.RenderTarget[0].DestBlend = D3D11_BLEND_SRC_COLOR;		break;
		case SBITS_DSTBLEND_ONE_MINUS_SRC_COLOR:	blend.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_COLOR;	break;
		default:	Sys_Error("Bad shader blend dst\n"); return NULL;
		}
		blend.RenderTarget[0].BlendEnable = TRUE;
	}
	else
	{
		blend.RenderTarget[0].SrcBlend = D3D11_BLEND_ZERO;
		blend.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
		blend.RenderTarget[0].BlendEnable = FALSE;
	}
	blend.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blend.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;//blend.RenderTarget[0].SrcBlend;
	blend.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;//blend.RenderTarget[0].DestBlend;
	blend.RenderTarget[0].BlendOpAlpha = blend.RenderTarget[0].BlendOp;

	if (bits&SBITS_MASK_BITS)
	{
		blend.RenderTarget[0].RenderTargetWriteMask = 0;
		if (!(bits&SBITS_MASK_RED))
			blend.RenderTarget[0].RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_RED;
		if (!(bits&SBITS_MASK_GREEN))
			blend.RenderTarget[0].RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_GREEN;
		if (!(bits&SBITS_MASK_BLUE))
			blend.RenderTarget[0].RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_BLUE;
		if (!(bits&SBITS_MASK_ALPHA))
			blend.RenderTarget[0].RenderTargetWriteMask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;
	}
	else
		blend.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	if (!FAILED(ID3D11Device_CreateBlendState(pD3DDev11, &blend, &newblendstate)))
		return newblendstate;
	return NULL;
}

static void D3D11BE_ApplyShaderBits(unsigned int bits, void **blendstatecache)
{
	unsigned int delta;

	if (shaderstate.flags & (BEF_FORCEADDITIVE|BEF_FORCETRANSPARENT|BEF_FORCENODEPTH|BEF_FORCEDEPTHTEST|BEF_FORCEDEPTHWRITE))
	{
		blendstatecache = NULL;

		if (shaderstate.flags & BEF_FORCEADDITIVE)
			bits = (bits & ~(SBITS_MISC_DEPTHWRITE|SBITS_BLEND_BITS|SBITS_ATEST_BITS))
						| (SBITS_SRCBLEND_SRC_ALPHA | SBITS_DSTBLEND_ONE);
		else if (shaderstate.flags & BEF_FORCETRANSPARENT)
		{
			if ((bits & SBITS_BLEND_BITS) == (SBITS_SRCBLEND_ONE|SBITS_DSTBLEND_ZERO) || !(bits & SBITS_BLEND_BITS)) 	/*if transparency is forced, clear alpha test bits*/
				bits = (bits & ~(SBITS_MISC_DEPTHWRITE|SBITS_BLEND_BITS|SBITS_ATEST_BITS))
							| (SBITS_SRCBLEND_SRC_ALPHA | SBITS_DSTBLEND_ONE_MINUS_SRC_ALPHA);
		}

		if (shaderstate.flags & BEF_FORCENODEPTH) 	/*EF_NODEPTHTEST dp extension*/
			bits |= SBITS_MISC_NODEPTHTEST;
		else
		{
			if (shaderstate.flags & BEF_FORCEDEPTHTEST)
				bits &= ~SBITS_MISC_NODEPTHTEST;
			if (shaderstate.flags & BEF_FORCEDEPTHWRITE)
				bits |= SBITS_MISC_DEPTHWRITE;
		}
	}

	delta = bits ^ shaderstate.shaderbits;
	if (!delta)
		return;
	shaderstate.shaderbits = bits;

	if (delta & (SBITS_BLEND_BITS|SBITS_MASK_BITS))
	{
		int sbits = bits & (SBITS_BLEND_BITS|SBITS_MASK_BITS);
		if (blendstatecache && *blendstatecache)
			ID3D11DeviceContext_OMSetBlendState(d3ddevctx, *blendstatecache, NULL, 0xffffffff);
		else
		{
			blendstates_t *bs;
			for (bs = shaderstate.blendstates; bs; bs = bs->next)
			{
				if (bs->bits == sbits)
					break;
			}
			if (!bs)
			{
				bs = BZ_Malloc(sizeof(*bs));
				bs->next = shaderstate.blendstates;
				shaderstate.blendstates = bs;
				bs->bits = sbits;
				bs->val = D3D11BE_GenerateBlendState(sbits);
			}
			ID3D11DeviceContext_OMSetBlendState(d3ddevctx, bs->val, NULL, 0xffffffff);
			if (blendstatecache)
				*blendstatecache = bs->val;
		}
	}

	if (delta & SBITS_ATEST_BITS)
	{
/*
		switch(bits & SBITS_ATEST_BITS)
		{
		case SBITS_ATEST_NONE:
			IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHATESTENABLE, FALSE);
	//		IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHAREF, 0);
	//		IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHAFUNC, 0);
			break;
		case SBITS_ATEST_GT0:
			IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHATESTENABLE, TRUE);
			IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHAREF, 0);
			IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHAFUNC, D3DCMP_GREATER);
			break;
		case SBITS_ATEST_LT128:
			IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHATESTENABLE, TRUE);
			IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHAREF, 128);
			IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHAFUNC, D3DCMP_LESS);
			break;
		case SBITS_ATEST_GE128:
			IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHATESTENABLE, TRUE);
			IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHAREF, 128);
			IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
			break;
		}
*/
	}

	if (delta & (SBITS_MISC_DEPTHEQUALONLY|SBITS_MISC_DEPTHCLOSERONLY|SBITS_MISC_NODEPTHTEST|SBITS_MISC_DEPTHWRITE))
	{
		unsigned int key = 0;
		if (bits & SBITS_MISC_DEPTHEQUALONLY)
			key |= 1u<<0;
		if (bits & SBITS_MISC_DEPTHCLOSERONLY)
			key |= 1u<<1;
		if (bits & SBITS_MISC_NODEPTHTEST)
			key |= 1u<<2;
		if (bits & SBITS_MISC_DEPTHWRITE)
			key |= 1u<<3;

		if (shaderstate.depthstates[key])
			ID3D11DeviceContext_OMSetDepthStencilState(d3ddevctx, shaderstate.depthstates[key], 0);
		else
		{
			D3D11_DEPTH_STENCIL_DESC depthdesc;
			if (bits & SBITS_MISC_NODEPTHTEST)
				depthdesc.DepthEnable = false;
			else
				depthdesc.DepthEnable = true;
			if (bits & SBITS_MISC_DEPTHWRITE)
				depthdesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			else
				depthdesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

			switch(bits & (SBITS_MISC_DEPTHEQUALONLY|SBITS_MISC_DEPTHCLOSERONLY))
			{
			default:
			case 0:
				depthdesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
				break;
			case SBITS_MISC_DEPTHEQUALONLY:
				depthdesc.DepthFunc = D3D11_COMPARISON_EQUAL;
				break;
			case SBITS_MISC_DEPTHCLOSERONLY:
				depthdesc.DepthFunc = D3D11_COMPARISON_LESS;
				break;
			}

			//make sure the stencil part is actually valid, even if we're not using it.
			depthdesc.StencilEnable = false;
			depthdesc.StencilReadMask = 0xFF;
			depthdesc.StencilWriteMask = 0xFF;
			depthdesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			depthdesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			depthdesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			depthdesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
			depthdesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
			depthdesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
			depthdesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
			depthdesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

			//and change it
			if (!FAILED(ID3D11Device_CreateDepthStencilState(pD3DDev11, &depthdesc, &shaderstate.depthstates[key])))
				ID3D11DeviceContext_OMSetDepthStencilState(d3ddevctx, shaderstate.depthstates[key], 0);
		}
	}
}

void D3D11BE_Reset(qboolean before)
{
	int i;
	if (!shaderstate.inited)
		return;

	if (before)
	{
		/*backbuffer is going away, release stuff so it can be destroyed cleanly*/
	}
	else
	{
		/*we have a new backbuffer etc, reassert state*/
		for (i = 0; i < MAX_TMUS; i++)
		{
			shaderstate.tmuflags[i] = ~0;
			BE_ApplyTMUState(i, 0);
		}

		/*force all state to change, thus setting a known state*/
		shaderstate.shaderbits = ~0;
		D3D11BE_ApplyShaderBits(0, NULL);
	}
}

static const char LIGHTPASS_SHADER[] = "\
{\n\
	program rtlight\n\
	{\n\
		map $diffuse\n\
		blendfunc add\n\
	}\n\
	{\n\
		map $normalmap\n\
	}\n\
	{\n\
		map $specular\n\
	}\n\
	{\n\
		map $lightcubemap\n\
	}\n\
	{\n\
		map $shadowmap\n\
	}\n\
	{\n\
		map $loweroverlay\n\
	}\n\
	{\n\
		map $upperoverlay\n\
	}\n\
}";

void D3D11BE_Init(void)
{
	D3D11_BUFFER_DESC bd;
	int i;

	be_maxpasses = MAX_TMUS;
	memset(&shaderstate, 0, sizeof(shaderstate));
	shaderstate.inited = true;
	shaderstate.curvertdecl = -1;
	for (i = 0; i < MAXRLIGHTMAPS; i++)
		shaderstate.dummybatch.lightmap[i] = -1;

	BE_CreateSamplerStates();

//	FTable_Init();

/*	shaderstate.dynxyz_size = sizeof(vecV_t) * DYNVBUFFSIZE;
	shaderstate.dyncol_size = sizeof(byte_vec4_t) * DYNVBUFFSIZE;
	shaderstate.dynst_size = sizeof(vec2_t) * DYNVBUFFSIZE;
	shaderstate.dynidx_size = sizeof(index_t) * DYNIBUFFSIZE;
*/
	D3D11BE_Reset(false);

	//set up the constant buffers
	for (i = 0; i < NUMECBUFFERS; i++)
	{
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(cbuf_entity_t);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;
		if (FAILED(ID3D11Device_CreateBuffer(pD3DDev11, &bd, NULL, &shaderstate.ecbuffers[i])))
			return;
	}
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(cbuf_view_t);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.MiscFlags = 0;
	bd.StructureByteStride = 0;
	if (FAILED(ID3D11Device_CreateBuffer(pD3DDev11, &bd, NULL, &shaderstate.vcbuffer)))
		return;

	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(cbuf_light_t);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	bd.MiscFlags = 0;
	bd.StructureByteStride = 0;
	if (FAILED(ID3D11Device_CreateBuffer(pD3DDev11, &bd, NULL, &shaderstate.lcbuffer)))
		return;

	//generate the streaming buffers for stuff that doesn't provide info in nice static vbos
	for (i = 0; i < NUMIBUFFERS; i++)
	{
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.ByteWidth = VERTEXSTREAMSIZE;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;
		bd.Usage = D3D11_USAGE_DYNAMIC;
		if (FAILED(ID3D11Device_CreateBuffer(pD3DDev11, &bd, NULL, &shaderstate.indexstream[i])))
			return;
	}
	for (i = 0; i < NUMVBUFFERS; i++)
	{
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.ByteWidth = VERTEXSTREAMSIZE;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;
		bd.Usage = D3D11_USAGE_DYNAMIC;
		if (FAILED(ID3D11Device_CreateBuffer(pD3DDev11, &bd, NULL, &shaderstate.vertexstream[i])))
			return;
	}

	/*
	for (i = 0; i < LSHADER_MODES; i++)
	{
		if ((i & LSHADER_CUBE) && (i & LSHADER_SPOT))
			continue;
		shaderstate.shader_rtlight[i] = R_RegisterShader(va("rtlight%s%s%s", 
															(i & LSHADER_SMAP)?"#PCF":"",
															(i & LSHADER_SPOT)?"#SPOT":"",
															(i & LSHADER_CUBE)?"#CUBE":"")
														, SUF_NONE, LIGHTPASS_SHADER);
	}
	*/
//	shaderstate.shader_rtlight = R_RegisterShader("rtlight", SUF_NONE, LIGHTPASS_SHADER);
	shaderstate.depthonly = R_RegisterShader("depthonly", SUF_NONE, 
				"{\n"
					"program depthonly\n"
					"{\n"
						"depthwrite\n"
						"maskcolor\n"
					"}\n"
				"}\n");

	R_InitFlashblends();
}

void D3D11BE_Shutdown(void)
{
	shaderstate.inited = false;
#ifdef RTLIGHTS
	D3D11_TerminateShadowMap();
#endif
	BE_DestroyVariousStates();
	Z_Free(shaderstate.wbatches);
	shaderstate.wbatches = NULL;
}

#if 0
static void allocvertexbuffer(ID3D11Buffer *buff, unsigned int bmaxsize, unsigned int *offset, void **data, unsigned int bytes)
{
	unsigned int boff;
	if (*offset + bytes > bmaxsize)
	{
		boff = 0;
		*offset = bytes;
	}
	else
	{
		boff = *offset;
		*offset += bytes;
	}
//	d3dcheck(IDirect3DVertexBuffer9_Lock(buff, boff, bytes, data, boff?D3DLOCK_NOOVERWRITE:D3DLOCK_DISCARD));
}

static unsigned int allocindexbuffer(void **dest, unsigned int entries)
{
	unsigned int bytes = entries*sizeof(index_t);
	unsigned int offset;
/*
	if (shaderstate.dynidx_offs + bytes > DYNIBUFFSIZE)
	{
		offset = 0;
		shaderstate.dynidx_offs = 0;
	}
	else
	{
		offset = shaderstate.dynidx_offs;
		shaderstate.dynidx_offs += bytes;
	}
*/
//	d3dcheck(IDirect3DIndexBuffer9_Lock(shaderstate.dynidx_buff, offset, (unsigned int)entries, dest, offset?D3DLOCK_NOOVERWRITE:D3DLOCK_DISCARD));
	return offset/sizeof(index_t);
}
#endif

ID3D11ShaderResourceView *D3D11_Image_View(const texid_t *id);
static void BindTexture(unsigned int tu, const texid_t *id)
{
	ID3D11ShaderResourceView *view = D3D11_Image_View(id);
	if (shaderstate.pendingtextures[tu] != view)
	{
		shaderstate.textureschanged = true;
		shaderstate.pendingtextures[tu] = view;
	}
}

void D3D11BE_UnbindAllTextures(void)
{
	int i;
	for (i = 0; i < shaderstate.lastpasscount; i++)
		shaderstate.pendingtextures[i] = NULL;
	if (i)
	{
		ID3D11DeviceContext_PSSetShaderResources(d3ddevctx, 0, i, shaderstate.pendingtextures);
		shaderstate.lastpasscount = 0;
	}
}

static void SelectPassTexture(unsigned int tu, const shaderpass_t *pass)
{
	extern texid_t r_whiteimage, missing_texture_gloss, missing_texture_normal;
	texid_t foo;
	switch(pass->texgen)
	{
	default:

	case T_GEN_DIFFUSE:
		BindTexture(tu, &shaderstate.curtexnums->base);
		break;
	case T_GEN_NORMALMAP:
		if (TEXVALID(shaderstate.curtexnums->bump))
			BindTexture(tu, &shaderstate.curtexnums->bump);
		else
			BindTexture(tu, &missing_texture_normal);
		break;
	case T_GEN_SPECULAR:
		if (TEXVALID(shaderstate.curtexnums->specular))
			BindTexture(tu, &shaderstate.curtexnums->specular);
		else
			BindTexture(tu, &missing_texture_gloss);
		break;
	case T_GEN_UPPEROVERLAY:
		BindTexture(tu, &shaderstate.curtexnums->upperoverlay);
		break;
	case T_GEN_LOWEROVERLAY:
		BindTexture(tu, &shaderstate.curtexnums->loweroverlay);
		break;
	case T_GEN_FULLBRIGHT:
		BindTexture(tu, &shaderstate.curtexnums->fullbright);
		break;
	case T_GEN_ANIMMAP:
		BindTexture(tu, &pass->anim_frames[(int)(pass->anim_fps * shaderstate.curtime) % pass->anim_numframes]);
		break;
	case T_GEN_3DMAP:
	case T_GEN_CUBEMAP:
	case T_GEN_SINGLEMAP:
		BindTexture(tu, &pass->anim_frames[0]);
		break;
	case T_GEN_DELUXMAP:
		{
			int lmi = shaderstate.curbatch->lightmap[0];
			if (lmi < 0 || !lightmap[lmi]->hasdeluxe)
				BindTexture(tu, &r_nulltex);
			else
			{
				lmi+=1;
				BindTexture(tu, &lightmap[lmi]->lightmap_texture);
			}
		}
		break;
	case T_GEN_LIGHTMAP:
		{
			int lmi = shaderstate.curbatch->lightmap[0];
			if (lmi < 0)
				BindTexture(tu, &r_whiteimage);
			else
				BindTexture(tu, &lightmap[lmi]->lightmap_texture);
		}
		break;

	/*case T_GEN_CURRENTRENDER:
		FIXME: no code to grab the current screen and convert to a texture
		break;*/
	case T_GEN_VIDEOMAP:
#ifndef NOMEDIA
		if (pass->cin)
		{
			foo = Media_UpdateForShader(pass->cin);
			BindTexture(tu, &foo);
			break;
		}
#endif
		BindTexture(tu, &r_nulltex);
		break;

	case T_GEN_LIGHTCUBEMAP:	//light's projected cubemap
		BindTexture(tu, &shaderstate.curdlight->cubetexture);
		break;

	case T_GEN_SHADOWMAP:	//light's depth values.
#ifdef RTLIGHTS
		if (shaderstate.curdlight)
		{
			foo = D3D11_GetShadowMap(shaderstate.curdlight->fov>0);
			BindTexture(tu, &foo);
			break;
		}
#endif
		BindTexture(tu, &r_nulltex);
		break;

	case T_GEN_CURRENTRENDER://copy the current screen to a texture, and draw that

	case T_GEN_SOURCECOLOUR: //used for render-to-texture targets
	case T_GEN_SOURCEDEPTH:	//used for render-to-texture targets

	case T_GEN_REFLECTION:	//reflection image (mirror-as-fbo)
	case T_GEN_REFRACTION:	//refraction image (portal-as-fbo)
	case T_GEN_REFRACTIONDEPTH:	//refraction image (portal-as-fbo)
	case T_GEN_RIPPLEMAP:	//ripplemap image (water surface distortions-as-fbo)

	case T_GEN_SOURCECUBE:	//used for render-to-texture targets
		BindTexture(tu, &r_nulltex);
		break;
	}

	BE_ApplyTMUState(tu, pass->flags);

	//pass blend modes are skipped - they're really only useful for fixed function. we should just use blend modes instead.
}

#if 0
static void colourgenbyte(const shaderpass_t *pass, int cnt, byte_vec4_t *srcb, vec4_t *srcf, byte_vec4_t *dst, const mesh_t *mesh)
{
/*
	D3DCOLOR block;
	switch (pass->rgbgen)
	{
	case RGB_GEN_ENTITY:
		block = D3DCOLOR_COLORVALUE(shaderstate.curentity->shaderRGBAf[0], shaderstate.curentity->shaderRGBAf[1], shaderstate.curentity->shaderRGBAf[2], shaderstate.curentity->shaderRGBAf[3]);
		while((cnt)--)
		{
			((D3DCOLOR*)dst)[cnt] = block;
		}
		break;
	case RGB_GEN_ONE_MINUS_ENTITY:
		block = D3DCOLOR_COLORVALUE(1-shaderstate.curentity->shaderRGBAf[0], 1-shaderstate.curentity->shaderRGBAf[1], 1-shaderstate.curentity->shaderRGBAf[2], 1-shaderstate.curentity->shaderRGBAf[3]);
		while((cnt)--)
		{
			((D3DCOLOR*)dst)[cnt] = block;
		}
		break;
	case RGB_GEN_VERTEX_LIGHTING:
	case RGB_GEN_VERTEX_EXACT:
		if (srcb)
		{
			while((cnt)--)
			{
				qbyte r, g, b;
				r=srcb[cnt][0];
				g=srcb[cnt][1];
				b=srcb[cnt][2];
				dst[cnt][0] = b;
				dst[cnt][1] = g;
				dst[cnt][2] = r;
			}
		}
		else if (srcf)
		{
			while((cnt)--)
			{
				int r, g, b;
				r=srcf[cnt][0]*255;
				g=srcf[cnt][1]*255;
				b=srcf[cnt][2]*255;
				dst[cnt][0] = bound(0, b, 255);
				dst[cnt][1] = bound(0, g, 255);
				dst[cnt][2] = bound(0, r, 255);
			}
		}
		else
			goto identity;
		break;
	case RGB_GEN_ONE_MINUS_VERTEX:
		if (srcb)
		{
			while((cnt)--)
			{
				qbyte r, g, b;
				r=255-srcb[cnt][0];
				g=255-srcb[cnt][1];
				b=255-srcb[cnt][2];
				dst[cnt][0] = b;
				dst[cnt][1] = g;
				dst[cnt][2] = r;
			}
		}
		else if (srcf)
		{
			while((cnt)--)
			{
				int r, g, b;
				r=255-srcf[cnt][0]*255;
				g=255-srcf[cnt][1]*255;
				b=255-srcf[cnt][2]*255;
				dst[cnt][0] = bound(0, b, 255);
				dst[cnt][1] = bound(0, g, 255);
				dst[cnt][2] = bound(0, r, 255);
			}
		}
		else
			goto identity;
		break;
	case RGB_GEN_IDENTITY_LIGHTING:
		//compensate for overbrights
		block = D3DCOLOR_RGBA(255, 255, 255, 255); //shaderstate.identitylighting
		while((cnt)--)
		{
			((D3DCOLOR*)dst)[cnt] = block;
		}
		break;
	default:
	identity:
	case RGB_GEN_IDENTITY:
		block = D3DCOLOR_RGBA(255, 255, 255, 255);
		while((cnt)--)
		{
			((D3DCOLOR*)dst)[cnt] = block;
		}
		break;
	case RGB_GEN_CONST:
		block = D3DCOLOR_COLORVALUE(pass->rgbgen_func.args[0], pass->rgbgen_func.args[1], pass->rgbgen_func.args[2], 1);
		while((cnt)--)
		{
			((D3DCOLOR*)dst)[cnt] = block;
		}
		break;
	case RGB_GEN_LIGHTING_DIFFUSE:
		//collect lighting details for mobile entities
		if (!mesh->normals_array)
		{
			block = D3DCOLOR_RGBA(255, 255, 255, 255);
			while((cnt)--)
			{
				((D3DCOLOR*)dst)[cnt] = block;
			}
		}
		else
		{
			R_LightArraysByte_BGR(shaderstate.curentity , mesh->xyz_array, dst, cnt, mesh->normals_array);
		}
		break;
	case RGB_GEN_WAVE:
		{
			float *table;
			float c;

			table = FTableForFunc(pass->rgbgen_func.type);
			c = pass->rgbgen_func.args[2] + shaderstate.curtime * pass->rgbgen_func.args[3];
			c = FTABLE_EVALUATE(table, c) * pass->rgbgen_func.args[1] + pass->rgbgen_func.args[0];
			c = bound(0.0f, c, 1.0f);
			block = D3DCOLOR_COLORVALUE(c, c, c, 1);

			while((cnt)--)
			{
				((D3DCOLOR*)dst)[cnt] = block;
			}
		}
		break;

	case RGB_GEN_TOPCOLOR:
	case RGB_GEN_BOTTOMCOLOR:
#ifdef warningmsg
#pragma warningmsg("fix 24bit player colours")
#endif
		block = D3DCOLOR_RGBA(255, 255, 255, 255);
		while((cnt)--)
		{
			((D3DCOLOR*)dst)[cnt] = block;
		}
	//	Con_Printf("RGB_GEN %i not supported\n", pass->rgbgen);
		break;
	}
*/
}

static void alphagenbyte(const shaderpass_t *pass, int cnt, byte_vec4_t *srcb, vec4_t *srcf, byte_vec4_t *dst, const mesh_t *mesh)
{
	/*FIXME: Skip this if the rgbgen did it*/
/*
	float *table;
	unsigned char t;
	float f;
	vec3_t v1, v2;

	switch (pass->alphagen)
	{
	default:
	case ALPHA_GEN_IDENTITY:
		if (shaderstate.flags & BEF_FORCETRANSPARENT)
		{
			f = shaderstate.curentity->shaderRGBAf[3];
			if (f < 0)
				t = 0;
			else if (f >= 1)
				t = 255;
			else
				t = f*255;
			while(cnt--)
				dst[cnt][3] = t;
		}
		else
		{
			while(cnt--)
				dst[cnt][3] = 255;
		}
		break;

	case ALPHA_GEN_CONST:
		t = pass->alphagen_func.args[0]*255;
		while(cnt--)
			dst[cnt][3] = t;
		break;

	case ALPHA_GEN_WAVE:
		table = FTableForFunc(pass->alphagen_func.type);
		f = pass->alphagen_func.args[2] + shaderstate.curtime * pass->alphagen_func.args[3];
		f = FTABLE_EVALUATE(table, f) * pass->alphagen_func.args[1] + pass->alphagen_func.args[0];
		t = bound(0.0f, f, 1.0f)*255;
		while(cnt--)
			dst[cnt][3] = t;
		break;

	case ALPHA_GEN_PORTAL:
		//FIXME: should this be per-vert?
		VectorAdd(mesh->xyz_array[0], shaderstate.curentity->origin, v1);
		VectorSubtract(r_origin, v1, v2);
		f = VectorLength(v2) * (1.0 / 255.0);
		t = bound(0.0f, f, 1.0f)*255;

		while(cnt--)
			dst[cnt][3] = t;
		break;

	case ALPHA_GEN_VERTEX:
		if (srcb)
		{
			while(cnt--)
			{
				dst[cnt][3] = srcb[cnt][3];
			}
		}
		else if (srcf)
		{
			while(cnt--)
			{
				dst[cnt][3] = bound(0, srcf[cnt][3]*255, 255);
			}
		}
		else
		{
			while(cnt--)
			{
				dst[cnt][3] = 255;
			}
		}
		break;

	case ALPHA_GEN_ENTITY:
		t = bound(0, shaderstate.curentity->shaderRGBAf[3], 1)*255;
		while(cnt--)
		{
			dst[cnt][3] = t;
		}
		break;

	case ALPHA_GEN_SPECULAR:
		{
			int i;
			VectorSubtract(r_origin, shaderstate.curentity->origin, v1);

			if (!Matrix3_Compare(shaderstate.curentity->axis, (void *)axisDefault))
			{
				Matrix3_Multiply_Vec3(shaderstate.curentity->axis, v2, v2);
			}
			else
			{
				VectorCopy(v1, v2);
			}

			for (i = 0; i < cnt; i++)
			{
				VectorSubtract(v2, mesh->xyz_array[i], v1);
				f = DotProduct(v1, mesh->normals_array[i] ) * Q_rsqrt(DotProduct(v1,v1));
				f = f * f * f * f * f;
				dst[i][3] = bound (0.0f, (int)(f*255), 255);
			}
		}
		break;
	}
	*/
}

static unsigned int BE_GenerateColourMods(unsigned int vertcount, const shaderpass_t *pass)
{
	unsigned int ret = 0;
	unsigned char *map;
	const mesh_t *m;
	unsigned int mno;

	m = shaderstate.meshlist[0];

	if (pass->flags & SHADER_PASS_NOCOLORARRAY)
	{
		shaderstate.passsinglecolour = true;
//		shaderstate.passcolour = D3DCOLOR_RGBA(255,255,255,255);
		colourgenbyte(pass, 1, (byte_vec4_t*)&shaderstate.passcolour, NULL, (byte_vec4_t*)&shaderstate.passcolour, m);
		alphagenbyte(pass, 1, (byte_vec4_t*)&shaderstate.passcolour, NULL, (byte_vec4_t*)&shaderstate.passcolour, m);
		/*FIXME: just because there's no rgba set, there's no reason to assume it should be a single colour (unshaded ents)*/
//		d3dcheck(IDirect3DDevice9_SetStreamSource(pD3DDev9, STRM_COL, NULL, 0, 0));
	}
	else
	{
		shaderstate.passsinglecolour = false;

		ret |= D3D_VDEC_COL4B;
		if (shaderstate.batchvbo && (m->colors4f_array[0] &&
						((pass->rgbgen == RGB_GEN_VERTEX_LIGHTING) ||
						(pass->rgbgen == RGB_GEN_VERTEX_EXACT) ||
						(pass->rgbgen == RGB_GEN_ONE_MINUS_VERTEX)) &&
						(pass->alphagen == ALPHA_GEN_VERTEX)))
		{
//			d3dcheck(IDirect3DDevice9_SetStreamSource(pD3DDev9, STRM_COL, shaderstate.batchvbo->colours.d3d.buff, shaderstate.batchvbo->colours.d3d.offs, sizeof(byte_vec4_t)));
		}
		else
		{
/*			allocvertexbuffer(shaderstate.dyncol_buff, shaderstate.dyncol_size, &shaderstate.dyncol_offs, (void**)&map, vertcount*sizeof(D3DCOLOR));
			for (vertcount = 0, mno = 0; mno < shaderstate.nummeshes; mno++)
			{
				m = shaderstate.meshlist[mno];
				colourgenbyte(pass, m->numvertexes, m->colors4b_array, m->colors4f_array[0], (byte_vec4_t*)map, m);
				alphagenbyte(pass, m->numvertexes, m->colors4b_array, m->colors4f_array[0], (byte_vec4_t*)map, m);
				map += m->numvertexes*4;
				vertcount += m->numvertexes;
			}
			d3dcheck(IDirect3DVertexBuffer9_Unlock(shaderstate.dyncol_buff));
			d3dcheck(IDirect3DDevice9_SetStreamSource(pD3DDev9, STRM_COL, shaderstate.dyncol_buff, shaderstate.dyncol_offs - vertcount*sizeof(D3DCOLOR), sizeof(D3DCOLOR)));
*/
		}
	}
	return ret;
}
#endif
/*********************************************************************************************************/
/*========================================== texture coord generation =====================================*/
#if 0
static void tcgen_environment(float *st, unsigned int numverts, float *xyz, float *normal)
{
	int			i;
	vec3_t		viewer, reflected;
	float		d;

	vec3_t		rorg;

	RotateLightVector(shaderstate.curentity->axis, shaderstate.curentity->origin, r_origin, rorg);

	for (i = 0 ; i < numverts ; i++, xyz += 3, normal += 3, st += 2 )
	{
		VectorSubtract (rorg, xyz, viewer);
		VectorNormalizeFast (viewer);

		d = DotProduct (normal, viewer);

		reflected[0] = normal[0]*2*d - viewer[0];
		reflected[1] = normal[1]*2*d - viewer[1];
		reflected[2] = normal[2]*2*d - viewer[2];

		st[0] = 0.5 + reflected[1] * 0.5;
		st[1] = 0.5 - reflected[2] * 0.5;
	}
}

static float *tcgen(const shaderpass_t *pass, int cnt, float *dst, const mesh_t *mesh)
{
	int i;
	vecV_t *src;
	switch (pass->tcgen)
	{
	default:
	case TC_GEN_BASE:
		return (float*)mesh->st_array;
	case TC_GEN_LIGHTMAP:
		return (float*)mesh->lmst_array;
	case TC_GEN_NORMAL:
		return (float*)mesh->normals_array;
	case TC_GEN_SVECTOR:
		return (float*)mesh->snormals_array;
	case TC_GEN_TVECTOR:
		return (float*)mesh->tnormals_array;
	case TC_GEN_ENVIRONMENT:
		tcgen_environment(dst, cnt, (float*)mesh->xyz_array, (float*)mesh->normals_array);
		return dst;

	case TC_GEN_DOTPRODUCT:
		return dst;//mesh->st_array[0];
	case TC_GEN_VECTOR:
		src = mesh->xyz_array;
		for (i = 0; i < cnt; i++, dst += 2)
		{
			static vec3_t tc_gen_s = { 1.0f, 0.0f, 0.0f };
			static vec3_t tc_gen_t = { 0.0f, 1.0f, 0.0f };

			dst[0] = DotProduct(tc_gen_s, src[i]);
			dst[1] = DotProduct(tc_gen_t, src[i]);
		}
		return dst;
	}
}

/*src and dst can be the same address when tcmods are chained*/
static void tcmod(const tcmod_t *tcmod, int cnt, const float *src, float *dst, const mesh_t *mesh)
{
	float *table;
	float t1, t2;
	float cost, sint;
	int j;
#define R_FastSin(x) sin((x)*(2*M_PI))
	switch (tcmod->type)
	{
		case SHADER_TCMOD_ROTATE:
			cost = tcmod->args[0] * shaderstate.curtime;
			sint = R_FastSin(cost);
			cost = R_FastSin(cost + 0.25);

			for (j = 0; j < cnt; j++, dst+=2,src+=2)
			{
				t1 = cost * (src[0] - 0.5f) - sint * (src[1] - 0.5f) + 0.5f;
				t2 = cost * (src[1] - 0.5f) + sint * (src[0] - 0.5f) + 0.5f;
				dst[0] = t1;
				dst[1] = t2;
			}
			break;

		case SHADER_TCMOD_SCALE:
			t1 = tcmod->args[0];
			t2 = tcmod->args[1];

			for (j = 0; j < cnt; j++, dst+=2,src+=2)
			{
				dst[0] = src[0] * t1;
				dst[1] = src[1] * t2;
			}
			break;

		case SHADER_TCMOD_TURB:
			t1 = tcmod->args[2] + shaderstate.curtime * tcmod->args[3];
			t2 = tcmod->args[1];

			for (j = 0; j < cnt; j++, dst+=2,src+=2)
			{
				dst[0] = src[0] + R_FastSin (src[0]*t2+t1) * t2;
				dst[1] = src[1] + R_FastSin (src[1]*t2+t1) * t2;
			}
			break;

		case SHADER_TCMOD_STRETCH:
			table = FTableForFunc(tcmod->args[0]);
			t2 = tcmod->args[3] + shaderstate.curtime * tcmod->args[4];
			t1 = FTABLE_EVALUATE(table, t2) * tcmod->args[2] + tcmod->args[1];
			t1 = t1 ? 1.0f / t1 : 1.0f;
			t2 = 0.5f - 0.5f * t1;
			for (j = 0; j < cnt; j++, dst+=2,src+=2)
			{
				dst[0] = src[0] * t1 + t2;
				dst[1] = src[1] * t1 + t2;
			}
			break;

		case SHADER_TCMOD_SCROLL:
			t1 = tcmod->args[0] * shaderstate.curtime;
			t2 = tcmod->args[1] * shaderstate.curtime;

			for (j = 0; j < cnt; j++, dst += 2, src+=2)
			{
				dst[0] = src[0] + t1;
				dst[1] = src[1] + t2;
			}
			break;

		case SHADER_TCMOD_TRANSFORM:
			for (j = 0; j < cnt; j++, dst+=2, src+=2)
			{
				t1 = src[0];
				t2 = src[1];
				dst[0] = t1 * tcmod->args[0] + t2 * tcmod->args[2] + tcmod->args[4];
				dst[1] = t2 * tcmod->args[1] + t1 * tcmod->args[3] + tcmod->args[5];
			}
			break;

		default:
			break;
	}
}

static void GenerateTCMods(const shaderpass_t *pass, float *dest)
{
	mesh_t *mesh;
	unsigned int mno;
	// unsigned int fvertex = 0; //unused variable
	int i;
	float *src;
	for (mno = 0; mno < shaderstate.nummeshes; mno++)
	{
		mesh = shaderstate.meshlist[mno];
		src = tcgen(pass, mesh->numvertexes, dest, mesh);
		//tcgen might return unmodified info
		if (pass->numtcmods)
		{
			tcmod(&pass->tcmods[0], mesh->numvertexes, src, dest, mesh);
			for (i = 1; i < pass->numtcmods; i++)
			{
				tcmod(&pass->tcmods[i], mesh->numvertexes, dest, dest, mesh);
			}
		}
		else if (src != dest)
		{
			memcpy(dest, src, sizeof(vec2_t)*mesh->numvertexes);
		}
		dest += mesh->numvertexes*2;
	}
}
#endif
//end texture coords
/*******************************************************************************************************************/
#if 0
static void deformgen(const deformv_t *deformv, int cnt, vecV_t *src, vecV_t *dst, const mesh_t *mesh)
{
	float *table;
	int j, k;
	float args[4];
	float deflect;
	switch (deformv->type)
	{
	default:
	case DEFORMV_NONE:
		if (src != dst)
			memcpy(dst, src, sizeof(*src)*cnt);
		break;

	case DEFORMV_WAVE:
		if (!mesh->normals_array)
		{
			if (src != dst)
				memcpy(dst, src, sizeof(*src)*cnt);
			return;
		}
		args[0] = deformv->func.args[0];
		args[1] = deformv->func.args[1];
		args[3] = deformv->func.args[2] + deformv->func.args[3] * shaderstate.curtime;
		table = FTableForFunc(deformv->func.type);

		for ( j = 0; j < cnt; j++ )
		{
			deflect = deformv->args[0] * (src[j][0]+src[j][1]+src[j][2]) + args[3];
			deflect = FTABLE_EVALUATE(table, deflect) * args[1] + args[0];

			// Deflect vertex along its normal by wave amount
			VectorMA(src[j], deflect, mesh->normals_array[j], dst[j]);
		}
		break;

	case DEFORMV_NORMAL:
		//normal does not actually move the verts, but it does change the normals array
		//we don't currently support that.
		if (src != dst)
			memcpy(dst, src, sizeof(*src)*cnt);
/*
		args[0] = deformv->args[1] * shaderstate.curtime;

		for ( j = 0; j < cnt; j++ )
		{
			args[1] = normalsArray[j][2] * args[0];

			deflect = deformv->args[0] * R_FastSin(args[1]);
			normalsArray[j][0] *= deflect;
			deflect = deformv->args[0] * R_FastSin(args[1] + 0.25);
			normalsArray[j][1] *= deflect;
			VectorNormalizeFast(normalsArray[j]);
		}
*/		break;

	case DEFORMV_MOVE:
		table = FTableForFunc(deformv->func.type);
		deflect = deformv->func.args[2] + shaderstate.curtime * deformv->func.args[3];
		deflect = FTABLE_EVALUATE(table, deflect) * deformv->func.args[1] + deformv->func.args[0];

		for ( j = 0; j < cnt; j++ )
			VectorMA(src[j], deflect, deformv->args, dst[j]);
		break;

	case DEFORMV_BULGE:
		args[0] = deformv->args[0]/(2*M_PI);
		args[1] = deformv->args[1];
		args[2] = shaderstate.curtime * deformv->args[2]/(2*M_PI);

		for (j = 0; j < cnt; j++)
		{
			deflect = R_FastSin(mesh->st_array[j][0]*args[0] + args[2])*args[1];
			dst[j][0] = src[j][0]+deflect*mesh->normals_array[j][0];
			dst[j][1] = src[j][1]+deflect*mesh->normals_array[j][1];
			dst[j][2] = src[j][2]+deflect*mesh->normals_array[j][2];
		}
		break;

	case DEFORMV_AUTOSPRITE:
		if (mesh->numindexes < 6)
			break;

		for (j = 0; j < cnt-3; j+=4, src+=4, dst+=4)
		{
			vec3_t mid, d;
			float radius;
			mid[0] = 0.25*(src[0][0] + src[1][0] + src[2][0] + src[3][0]);
			mid[1] = 0.25*(src[0][1] + src[1][1] + src[2][1] + src[3][1]);
			mid[2] = 0.25*(src[0][2] + src[1][2] + src[2][2] + src[3][2]);
			VectorSubtract(src[0], mid, d);
			radius = 2*VectorLength(d);

			for (k = 0; k < 4; k++)
			{
				dst[k][0] = mid[0] + radius*((mesh->st_array[k][0]-0.5)*r_refdef.m_view[0+0]-(mesh->st_array[k][1]-0.5)*r_refdef.m_view[0+1]);
				dst[k][1] = mid[1] + radius*((mesh->st_array[k][0]-0.5)*r_refdef.m_view[4+0]-(mesh->st_array[k][1]-0.5)*r_refdef.m_view[4+1]);
				dst[k][2] = mid[2] + radius*((mesh->st_array[k][0]-0.5)*r_refdef.m_view[8+0]-(mesh->st_array[k][1]-0.5)*r_refdef.m_view[8+1]);
			}
		}
		break;

	case DEFORMV_AUTOSPRITE2:
		if (mesh->numindexes < 6)
			break;

		for (k = 0; k < mesh->numindexes; k += 6)
		{
			int long_axis, short_axis;
			vec3_t axis;
			float len[3];
			mat3_t m0, m1, m2, result;
			float *quad[4];
			vec3_t rot_centre, tv;

			quad[0] = (float *)(dst + mesh->indexes[k+0]);
			quad[1] = (float *)(dst + mesh->indexes[k+1]);
			quad[2] = (float *)(dst + mesh->indexes[k+2]);

			for (j = 2; j >= 0; j--)
			{
				quad[3] = (float *)(dst + mesh->indexes[k+3+j]);
				if (!VectorEquals (quad[3], quad[0]) &&
					!VectorEquals (quad[3], quad[1]) &&
					!VectorEquals (quad[3], quad[2]))
				{
					break;
				}
			}

			// build a matrix were the longest axis of the billboard is the Y-Axis
			VectorSubtract(quad[1], quad[0], m0[0]);
			VectorSubtract(quad[2], quad[0], m0[1]);
			VectorSubtract(quad[2], quad[1], m0[2]);
			len[0] = DotProduct(m0[0], m0[0]);
			len[1] = DotProduct(m0[1], m0[1]);
			len[2] = DotProduct(m0[2], m0[2]);

			if ((len[2] > len[1]) && (len[2] > len[0]))
			{
				if (len[1] > len[0])
				{
					long_axis = 1;
					short_axis = 0;
				}
				else
				{
					long_axis = 0;
					short_axis = 1;
				}
			}
			else if ((len[1] > len[2]) && (len[1] > len[0]))
			{
				if (len[2] > len[0])
				{
					long_axis = 2;
					short_axis = 0;
				}
				else
				{
					long_axis = 0;
					short_axis = 2;
				}
			}
			else //if ( (len[0] > len[1]) && (len[0] > len[2]) )
			{
				if (len[2] > len[1])
				{
					long_axis = 2;
					short_axis = 1;
				}
				else
				{
					long_axis = 1;
					short_axis = 2;
				}
			}

			if (DotProduct(m0[long_axis], m0[short_axis]))
			{
				VectorNormalize2(m0[long_axis], axis);
				VectorCopy(axis, m0[1]);

				if (axis[0] || axis[1])
				{
					VectorVectors(m0[1], m0[2], m0[0]);
				}
				else
				{
					VectorVectors(m0[1], m0[0], m0[2]);
				}
			}
			else
			{
				VectorNormalize2(m0[long_axis], axis);
				VectorNormalize2(m0[short_axis], m0[0]);
				VectorCopy(axis, m0[1]);
				CrossProduct(m0[0], m0[1], m0[2]);
			}

			for (j = 0; j < 3; j++)
				rot_centre[j] = (quad[0][j] + quad[1][j] + quad[2][j] + quad[3][j]) * 0.25;

			if (shaderstate.curentity)
			{
				VectorAdd(shaderstate.curentity->origin, rot_centre, tv);
			}
			else
			{
				VectorCopy(rot_centre, tv);
			}
			VectorSubtract(r_origin, tv, tv);

			// filter any longest-axis-parts off the camera-direction
			deflect = -DotProduct(tv, axis);

			VectorMA(tv, deflect, axis, m1[2]);
			VectorNormalizeFast(m1[2]);
			VectorCopy(axis, m1[1]);
			CrossProduct(m1[1], m1[2], m1[0]);

			Matrix3_Transpose(m1, m2);
			Matrix3_Multiply(m2, m0, result);

			for (j = 0; j < 4; j++)
			{
				VectorSubtract(quad[j], rot_centre, tv);
				Matrix3_Multiply_Vec3((void *)result, tv, quad[j]);
				VectorAdd(rot_centre, quad[j], quad[j]);
			}
		}
		break;

//	case DEFORMV_PROJECTION_SHADOW:
//		break;
	}
}
#endif

#if 0
/*does not do the draw call, does not consider indicies (except for billboard generation) */
static qboolean BE_DrawMeshChain_SetupPass(shaderpass_t *pass, unsigned int vertcount)
{
	int vdec;
	void *map;
	int i;
	unsigned int passno = 0, tmu;

	int lastpass = pass->numMergedPasses;

	for (i = 0; i < lastpass; i++)
	{
		if (pass[i].texgen == T_GEN_UPPEROVERLAY && !TEXVALID(shaderstate.curtexnums->upperoverlay))
			continue;
		if (pass[i].texgen == T_GEN_LOWEROVERLAY && !TEXVALID(shaderstate.curtexnums->loweroverlay))
			continue;
		if (pass[i].texgen == T_GEN_FULLBRIGHT && !TEXVALID(shaderstate.curtexnums->fullbright))
			continue;
		break;
	}
	if (i == lastpass)
		return false;

	/*all meshes in a chain must have the same features*/
	vdec = 0;

	/*we only use one colour, generated from the first pass*/
	vdec |= BE_GenerateColourMods(vertcount, pass);

	tmu = 0;
	/*activate tmus*/
	for (passno = 0; passno < lastpass; passno++)
	{
		if (pass[passno].texgen == T_GEN_UPPEROVERLAY && !TEXVALID(shaderstate.curtexnums->upperoverlay))
			continue;
		if (pass[passno].texgen == T_GEN_LOWEROVERLAY && !TEXVALID(shaderstate.curtexnums->loweroverlay))
			continue;
		if (pass[passno].texgen == T_GEN_FULLBRIGHT && !TEXVALID(shaderstate.curtexnums->fullbright))
			continue;

		SelectPassTexture(tmu, pass+passno);

		vdec |= D3D_VDEC_ST0<<tmu;
/*
		if (shaderstate.batchvbo && pass[passno].tcgen == TC_GEN_BASE && !pass[passno].numtcmods)
			d3dcheck(IDirect3DDevice9_SetStreamSource(pD3DDev9, STRM_TC0+tmu, shaderstate.batchvbo->texcoord.d3d.buff, shaderstate.batchvbo->texcoord.d3d.offs, sizeof(vec2_t)));
		else if (shaderstate.batchvbo && pass[passno].tcgen == TC_GEN_LIGHTMAP && !pass[passno].numtcmods)
			d3dcheck(IDirect3DDevice9_SetStreamSource(pD3DDev9, STRM_TC0+tmu, shaderstate.batchvbo->lmcoord[0].d3d.buff, shaderstate.batchvbo->lmcoord[0].d3d.offs, sizeof(vec2_t)));
		else
		{
			allocvertexbuffer(shaderstate.dynst_buff[tmu], shaderstate.dynst_size, &shaderstate.dynst_offs[tmu], &map, vertcount*sizeof(vec2_t));
			GenerateTCMods(pass+passno, map);
			d3dcheck(IDirect3DVertexBuffer9_Unlock(shaderstate.dynst_buff[tmu]));
			d3dcheck(IDirect3DDevice9_SetStreamSource(pD3DDev9, STRM_TC0+tmu, shaderstate.dynst_buff[tmu], shaderstate.dynst_offs[tmu] - vertcount*sizeof(vec2_t), sizeof(vec2_t)));
		}
*/
		tmu++;
	}
	/*deactivate any extras*/
	for (; tmu < shaderstate.lastpasscount; )
	{
//		d3dcheck(IDirect3DDevice9_SetStreamSource(pD3DDev9, STRM_TC0+tmu, NULL, 0, 0));
		BindTexture(tmu, NULL);
//		d3dcheck(IDirect3DDevice9_SetTextureStageState(pD3DDev9, tmu, D3DTSS_COLOROP, D3DTOP_DISABLE));
		tmu++;
	}
	shaderstate.lastpasscount = tmu;

//	if (meshchain->normals_array &&
//		meshchain->2 &&
//		meshchain->tnormals_array)
//		vdec |= D3D_VDEC_NORMS;

	if (vdec != shaderstate.curvertdecl)
	{
		shaderstate.curvertdecl = vdec;
//		d3dcheck(IDirect3DDevice9_SetVertexDeclaration(pD3DDev9, vertexdecls[shaderstate.curvertdecl]));
	}

	D3D11BE_ApplyShaderBits(pass->shaderbits);
	return true;
}
#endif

static void BE_SubmitMeshChain(int idxfirst)
{
	int starti, endi;
	int m;
	mesh_t *mesh;

	/*if (shaderstate.batchvbo)
	{
		ID3D11DeviceContext_DrawIndexed(d3ddevctx, shaderstate.batchvbo->indexcount, 0, 0);
		return;
	}*/

	for (m = 0, mesh = shaderstate.meshlist[0]; m < shaderstate.nummeshes; )
	{
		starti = mesh->vbofirstelement;

		endi = starti+mesh->numindexes;

		//find consecutive surfaces
		for (++m; m < shaderstate.nummeshes; m++)
		{
			mesh = shaderstate.meshlist[m];
			if (endi == mesh->vbofirstelement)
			{
				endi = mesh->vbofirstelement+mesh->numindexes;
			}
			else
			{
				break;
			}
		}

		ID3D11DeviceContext_DrawIndexed(d3ddevctx, endi - starti, starti, 0);
		RQuantAdd(RQUANT_DRAWS, 1);
 	}
}

static void BE_ApplyUniforms(program_t *prog, int permu)
{
	ID3D11Buffer *cbuf[3] =
	{
		shaderstate.ecbuffers[shaderstate.ecbufferidx],	//entity buffer
		shaderstate.vcbuffer,							//view buffer that changes rarely
		shaderstate.lcbuffer							//light buffer that changes rarelyish
	};
	//FIXME: how many of these calls can we avoid?
	ID3D11DeviceContext_IASetInputLayout(d3ddevctx, prog->permu[permu].handle.hlsl.layout);
	ID3D11DeviceContext_VSSetShader(d3ddevctx, prog->permu[permu].handle.hlsl.vert, NULL, 0);
	ID3D11DeviceContext_HSSetShader(d3ddevctx, prog->permu[permu].handle.hlsl.hull, NULL, 0);
	ID3D11DeviceContext_DSSetShader(d3ddevctx, prog->permu[permu].handle.hlsl.domain, NULL, 0);
	ID3D11DeviceContext_PSSetShader(d3ddevctx, prog->permu[permu].handle.hlsl.frag, NULL, 0);
	ID3D11DeviceContext_IASetPrimitiveTopology(d3ddevctx, prog->permu[permu].handle.hlsl.topology);

	ID3D11DeviceContext_VSSetConstantBuffers(d3ddevctx, 0, 3, cbuf);
	ID3D11DeviceContext_HSSetConstantBuffers(d3ddevctx, 0, 3, cbuf);
	ID3D11DeviceContext_DSSetConstantBuffers(d3ddevctx, 0, 3, cbuf);
	ID3D11DeviceContext_PSSetConstantBuffers(d3ddevctx, 0, 3, cbuf);
}

static void BE_RenderMeshProgram(const shader_t *s, unsigned int vertcount, unsigned int idxfirst, unsigned int idxcount)
{
	int passno;
	int perm = 0;

	program_t *p = s->prog;

	if (TEXVALID(shaderstate.curtexnums->bump) && p->permu[perm|PERMUTATION_BUMPMAP].handle.hlsl.vert)
		perm |= PERMUTATION_BUMPMAP;
	if (TEXVALID(shaderstate.curtexnums->fullbright) && p->permu[perm|PERMUTATION_FULLBRIGHT].handle.hlsl.vert)
		perm |= PERMUTATION_FULLBRIGHT;
	if (p->permu[perm|PERMUTATION_UPPERLOWER].handle.hlsl.vert && (TEXVALID(shaderstate.curtexnums->upperoverlay) || TEXVALID(shaderstate.curtexnums->loweroverlay)))
		perm |= PERMUTATION_UPPERLOWER;
	if (r_refdef.globalfog.density && p->permu[perm|PERMUTATION_FOG].handle.hlsl.vert)
		perm |= PERMUTATION_FOG;
//	if (r_glsl_offsetmapping.ival && TEXVALID(shaderstate.curtexnums->bump) && p->handle[perm|PERMUTATION_OFFSET.hlsl.vert)
//		perm |= PERMUTATION_OFFSET;

	BE_ApplyUniforms(p, perm);


	D3D11BE_ApplyShaderBits(s->passes->shaderbits, &s->passes->becache);

	/*activate tmus*/
	for (passno = 0; passno < s->numpasses; passno++)
	{
		SelectPassTexture(passno, s->passes+passno);
	}
	/*deactivate any extras*/
	for (; passno < shaderstate.lastpasscount; passno++)
	{
		shaderstate.pendingtextures[passno] = NULL;
		shaderstate.textureschanged = true;
	}
	if (shaderstate.textureschanged)
		ID3D11DeviceContext_PSSetShaderResources(d3ddevctx, 0, max(passno, s->numpasses), shaderstate.pendingtextures);
	shaderstate.lastpasscount = s->numpasses;

	BE_SubmitMeshChain(idxfirst);
}

static void D3D11BE_Cull(unsigned int cullflags)
{
	HRESULT hr;
	D3D11_RASTERIZER_DESC rasterdesc;
	ID3D11RasterizerState *newrasterizerstate;

	cullflags ^= r_refdef.flipcull;

	if (shaderstate.curcull != cullflags)
	{
		shaderstate.curcull = cullflags;


		rasterdesc.AntialiasedLineEnable = false;

		if (shaderstate.curcull & 1)
		{
			if (shaderstate.curcull & SHADER_CULL_FRONT)
				rasterdesc.CullMode = D3D11_CULL_FRONT;
			else if (shaderstate.curcull & SHADER_CULL_BACK)
				rasterdesc.CullMode = D3D11_CULL_BACK;
			else
				rasterdesc.CullMode = D3D11_CULL_NONE;
		}
		else
		{
			if (shaderstate.curcull & SHADER_CULL_FRONT)
				rasterdesc.CullMode = D3D11_CULL_BACK;
			else if (shaderstate.curcull & SHADER_CULL_BACK)
				rasterdesc.CullMode = D3D11_CULL_FRONT;
			else
				rasterdesc.CullMode = D3D11_CULL_NONE;
		}


		rasterdesc.DepthBias = 0;
		rasterdesc.DepthBiasClamp = 0.0f;
		rasterdesc.DepthClipEnable = true;
		rasterdesc.FillMode = 0?D3D11_FILL_WIREFRAME:D3D11_FILL_SOLID;
		rasterdesc.FrontCounterClockwise = false;
		rasterdesc.MultisampleEnable = false;
		rasterdesc.ScissorEnable = false;//true;
		rasterdesc.SlopeScaledDepthBias = 0.0f;

		if (FAILED(hr=ID3D11Device_CreateRasterizerState(pD3DDev11, &rasterdesc, &newrasterizerstate)))
		{
			if (hr == DXGI_ERROR_DEVICE_REMOVED)
			{
				hr = ID3D11Device_GetDeviceRemovedReason(pD3DDev11);
				switch(hr)
				{
				case DXGI_ERROR_DEVICE_HUNG:
					Sys_Error("DXGI_ERROR_DEVICE_HUNG\nThe application's device failed due to badly formed commands sent by the application.\n");
					break;
				case DXGI_ERROR_DEVICE_REMOVED:
					Sys_Error("DXGI_ERROR_DEVICE_REMOVED\nThe video card has been physically removed from the system, or a driver upgrade for the video card has occurred.\n");
					break;
				case DXGI_ERROR_DEVICE_RESET:
					Sys_Error("DXGI_ERROR_DEVICE_RESET\nThe device failed due to a badly formed command.\n");
					break;
				case DXGI_ERROR_DRIVER_INTERNAL_ERROR:
					Sys_Error("DXGI_ERROR_DRIVER_INTERNAL_ERROR\nThe driver encountered a problem and was put into the device removed state.\n");
					break;
				case DXGI_ERROR_INVALID_CALL:
					Sys_Error("invalid call! oh noes!\n");
					break;
				default:
					break;
				}
			}
			else
				Con_Printf("ID3D11Device_CreateRasterizerState failed\n");
			return;
		}
		ID3D11DeviceContext_RSSetState(d3ddevctx, newrasterizerstate);
		ID3D11RasterizerState_Release(newrasterizerstate);
	}
}

static void BE_DrawMeshChain_Internal(void)
{
	const shader_t *altshader;
	unsigned int vertcount, idxcount, idxfirst;
	mesh_t *m;
//	void *map;
//	int i;
	unsigned int mno;
	unsigned int passno = 0;
	shaderpass_t *pass = shaderstate.curshader->passes;
	extern cvar_t r_polygonoffset_submodel_factor;
//	float pushdepth;
//	float pushfactor;

	if (0)//shaderstate.force2d)
	{
		RQuantAdd(RQUANT_2DBATCHES, 1);
	}
	else if (shaderstate.curentity == &r_worldentity)
	{
		RQuantAdd(RQUANT_WORLDBATCHES, 1);
	}
	else
	{
		RQuantAdd(RQUANT_ENTBATCHES, 1);
	}


	D3D11BE_Cull(shaderstate.curshader->flags & (SHADER_CULL_FRONT | SHADER_CULL_BACK));
/*
	pushdepth = (shaderstate.curshader->polyoffset.factor + ((shaderstate.flags & BEF_PUSHDEPTH)?r_polygonoffset_submodel_factor.value:0))/0xffff;
	if (pushdepth != shaderstate.depthbias)
	{
		shaderstate.depthbias = pushdepth;
		IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_DEPTHBIAS, *(DWORD*)&shaderstate.depthbias);
	}
	pushdepth = shaderstate.curshader->polyoffset.unit/-1;// + ((shaderstate.flags & BEF_PUSHDEPTH)?8:0);
	pushfactor = shaderstate.curshader->polyoffset.factor/-1;
	if (pushfactor != shaderstate.depthfactor)
	{
		shaderstate.depthfactor = pushfactor;
		IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&shaderstate.depthfactor);
	}
*/

	if (shaderstate.batchvbo)
	{
		vertcount = shaderstate.batchvbo->vertcount;
		idxcount = shaderstate.batchvbo->indexcount;
	}
	else
	{
		for (mno = 0, vertcount = 0, idxcount = 0; mno < shaderstate.nummeshes; mno++)
		{
			m = shaderstate.meshlist[mno];
			vertcount += m->numvertexes;
			idxcount += m->numindexes;
		}
	}

	/*vertex buffers are common to all passes*/
	if (shaderstate.batchvbo)
	{
		unsigned int strides[] = {sizeof(vbovdata_t)};
		ID3D11DeviceContext_IASetVertexBuffers(d3ddevctx, 0, 1, (ID3D11Buffer**)&shaderstate.batchvbo->coord.d3d.buff, strides, &shaderstate.batchvbo->coord.d3d.offs);
//		d3dcheck(IDirect3DDevice9_SetStreamSource(pD3DDev9, STRM_VERT, shaderstate.batchvbo->coord.d3d.buff, shaderstate.batchvbo->coord.d3d.offs, sizeof(vecV_t)));
	}
	else
	{
		return;
/*		allocvertexbuffer(shaderstate.dynxyz_buff, shaderstate.dynxyz_size, &shaderstate.dynxyz_offs, &map, vertcount*sizeof(vecV_t));
		for (mno = 0, vertcount = 0; mno < shaderstate.nummeshes; mno++)
		{
			vecV_t *dest = (vecV_t*)((char*)map+vertcount*sizeof(vecV_t));
			m = shaderstate.meshlist[mno];
			deformgen(&shaderstate.curshader->deforms[0], m->numvertexes, m->xyz_array, dest, m);
			for (i = 1; i < shaderstate.curshader->numdeforms; i++)
			{
				deformgen(&shaderstate.curshader->deforms[i], m->numvertexes, dest, dest, m);
			}
			vertcount += m->numvertexes;
		}
		d3dcheck(IDirect3DVertexBuffer9_Unlock(shaderstate.dynxyz_buff));
		d3dcheck(IDirect3DDevice9_SetStreamSource(pD3DDev9, STRM_VERT, shaderstate.dynxyz_buff, shaderstate.dynxyz_offs - vertcount*sizeof(vecV_t), sizeof(vecV_t)));
*/
	}

	/*so are index buffers*/
	if (shaderstate.batchvbo)
	{
		ID3D11DeviceContext_IASetIndexBuffer(d3ddevctx, shaderstate.batchvbo->indicies.d3d.buff, DXGI_FORMAT_R16_UINT, shaderstate.batchvbo->indicies.d3d.offs);
		idxfirst = 0;
	}
	else
	{
		return;
/*		idxfirst = allocindexbuffer(&map, idxcount);
		for (mno = 0, vertcount = 0; mno < shaderstate.nummeshes; mno++)
		{
			m = shaderstate.meshlist[mno];
			for (i = 0; i < m->numindexes; i++)
				((index_t*)map)[i] = m->indexes[i]+vertcount;
			map = (char*)map + m->numindexes*sizeof(index_t);
			vertcount += m->numvertexes;
		}
		d3dcheck(IDirect3DIndexBuffer9_Unlock(shaderstate.dynidx_buff));
		d3dcheck(IDirect3DDevice9_SetIndices(pD3DDev9, shaderstate.dynidx_buff));
*/
	}

	switch (shaderstate.mode)
	{
	case BEM_LIGHT:
		if (shaderstate.shader_rtlight[shaderstate.curlmode]->prog)
			BE_RenderMeshProgram(shaderstate.shader_rtlight[shaderstate.curlmode], vertcount, idxfirst, idxcount);
		break;
	case BEM_DEPTHONLY:
		altshader = shaderstate.curshader->bemoverrides[bemoverride_depthonly];
		if (!altshader)
			altshader = shaderstate.depthonly;
		if (altshader->prog)
			BE_RenderMeshProgram(altshader, vertcount, idxfirst, idxcount);
		break;
	default:
	case BEM_STANDARD:
		if (shaderstate.curshader->prog)
		{
			BE_RenderMeshProgram(shaderstate.curshader, vertcount, idxfirst, idxcount);
		}
		else if (developer.ival)
			Con_DPrintf("Shader %s has no hlsl program\n", shaderstate.curshader->name);
		//else d3d11 has no fixed function pipeline.
		break;
	}
}

void D3D11BE_SelectMode(backendmode_t mode)
{
	shaderstate.mode = mode;

	if (mode == BEM_STENCIL)
		D3D11BE_ApplyShaderBits(SBITS_MASK_BITS, NULL);
}
qboolean D3D11BE_GenerateRTLightShader(unsigned int lmode)
{
	if (!shaderstate.shader_rtlight[lmode])
	{
		shaderstate.shader_rtlight[lmode] = R_RegisterShader(va("rtlight%s%s%s", 
															(lmode & LSHADER_SMAP)?"#PCF":"",
															(lmode & LSHADER_SPOT)?"#SPOT":"",
															(lmode & LSHADER_CUBE)?"#CUBE":"")
														, SUF_NONE, LIGHTPASS_SHADER);
	}
	if (!shaderstate.shader_rtlight[lmode]->prog)
		return false;
	return true;
}
qboolean D3D11BE_SelectDLight(dlight_t *dl, vec3_t colour, unsigned int lmode)
{
	if (!D3D11BE_GenerateRTLightShader(lmode))
	{
		lmode &= ~(LSHADER_SMAP|LSHADER_CUBE);
		if (!D3D11BE_GenerateRTLightShader(lmode))
			return false;
	}
	shaderstate.curdlight = dl;
	shaderstate.curlmode = lmode;
	VectorCopy(colour, shaderstate.curdlight_colours);

	D3D11BE_SetupLightCBuffer(dl, colour);
	return true;
}

#ifdef RTLIGHTS
void D3D11BE_SetupForShadowMap(dlight_t *dl, qboolean isspot, int texwidth, int texheight, float shadowscale)
{
#define SHADOWMAP_SIZE 512
	extern cvar_t r_shadow_shadowmapping_nearclip, r_shadow_shadowmapping_bias;
	float nc = r_shadow_shadowmapping_nearclip.value;
	float bias = r_shadow_shadowmapping_bias.value;

	//much of the projection matrix cancels out due to symmetry and stuff
	//we need to scale between -0.5,0.5 within the sub-image. the fragment shader will center on the subimage based upon the major axis.
	//in d3d, the depth value is scaled between 0 and 1 (gl is -1 to 1).
	//d3d's framebuffer is upside down or something annoying like that.
	shaderstate.lightshadowmapproj[0] = shadowscale * (1.0-(1.0/texwidth)) * 0.5/3.0;	//pinch x inwards
	shaderstate.lightshadowmapproj[1] = -shadowscale * (1.0-(1.0/texheight)) * 0.5/2.0;	//pinch y inwards
	shaderstate.lightshadowmapproj[2] = 0.5*(dl->radius+nc)/(nc-dl->radius);	//proj matrix 10
	shaderstate.lightshadowmapproj[3] = (dl->radius*nc)/(nc-dl->radius) - bias*nc*(1024/texheight);	//proj matrix 14	

	shaderstate.lightshadowmapscale[0] = 1.0/(SHADOWMAP_SIZE*3);
	shaderstate.lightshadowmapscale[1] = -1.0/(SHADOWMAP_SIZE*2);
}
#endif

void D3D11BE_SelectEntity(entity_t *ent)
{
	BE_RotateForEntity(ent, ent->model);
}

static qboolean BE_GenTempMeshVBO(vbo_t **vbo, mesh_t *mesh)
{
	static vbo_t tmpvbo;
	D3D11_MAPPED_SUBRESOURCE msr;
	int i;

	D3D11_MAP type;
	int sz;
	ID3D11Buffer *buf;

	//vbo first
	{
		vbovdata_t *out;

		sz = sizeof(*out) * mesh->numvertexes;
		if (shaderstate.purgevertexstream || shaderstate.vertexstreamoffset + sz > VERTEXSTREAMSIZE)
		{
			shaderstate.purgevertexstream = false;
			shaderstate.vertexstreamoffset = 0;
			type = D3D11_MAP_WRITE_DISCARD;
			shaderstate.vertexstreamcycle++;
			if (shaderstate.vertexstreamcycle == NUMVBUFFERS)
				shaderstate.vertexstreamcycle = 0;
		}
		else
		{
			type = D3D11_MAP_WRITE_NO_OVERWRITE;	//yes sir, sorry sir, we promise to not break anything
		}
		buf = shaderstate.vertexstream[shaderstate.vertexstreamcycle];
		if (FAILED(ID3D11DeviceContext_Map(d3ddevctx, (ID3D11Resource*)buf, 0, type, 0, &msr)))
		{
			Con_Printf("BE_RotateForEntity: failed to map vertex stream buffer start\n");
			return false;
		}

		//figure out where our pointer is and mark it as consumed
		out = (vbovdata_t*)((qbyte*)msr.pData + shaderstate.vertexstreamoffset);
		//FIXME: do we actually need to bother setting all this junk?
		tmpvbo.coord.d3d.buff = buf;
		tmpvbo.coord.d3d.offs = (quintptr_t)&out[0].coord - (quintptr_t)&out[0] + shaderstate.vertexstreamoffset;
		tmpvbo.texcoord.d3d.buff = buf;
		tmpvbo.texcoord.d3d.offs = (quintptr_t)&out[0].tex - (quintptr_t)&out[0] + shaderstate.vertexstreamoffset;
		tmpvbo.lmcoord[0].d3d.buff = buf;
		tmpvbo.lmcoord[0].d3d.offs = (quintptr_t)&out[0].lm - (quintptr_t)&out[0] + shaderstate.vertexstreamoffset;
		tmpvbo.normals.d3d.buff = buf;
		tmpvbo.normals.d3d.offs = (quintptr_t)&out[0].ndir - (quintptr_t)&out[0] + shaderstate.vertexstreamoffset;
		tmpvbo.svector.d3d.buff = buf;
		tmpvbo.svector.d3d.offs = (quintptr_t)&out[0].sdir - (quintptr_t)&out[0] + shaderstate.vertexstreamoffset;
		tmpvbo.tvector.d3d.buff = buf;
		tmpvbo.tvector.d3d.offs = (quintptr_t)&out[0].tdir - (quintptr_t)&out[0] + shaderstate.vertexstreamoffset;
		tmpvbo.colours[0].d3d.buff = buf;
		tmpvbo.colours[0].d3d.offs = (quintptr_t)&out[0].colorsb - (quintptr_t)&out[0] + shaderstate.vertexstreamoffset;
		//consumed
		shaderstate.vertexstreamoffset += sz;
		
		//now vomit into the buffer
		if (!mesh->normals_array && mesh->colors4f_array[0])
		{
			//2d drawing
			for (i = 0; i < mesh->numvertexes; i++)
			{
				VectorCopy(mesh->xyz_array[i], out[i].coord);
				Vector2Copy(mesh->st_array[i], out[i].tex);
				VectorClear(out[i].ndir);
				VectorClear(out[i].sdir);
				VectorClear(out[i].tdir);
				Vector4Scale(mesh->colors4f_array[0][i], 255, out[i].colorsb);
			}
		}
		else if (!mesh->normals_array && mesh->colors4b_array)
		{
			//2d drawing, ish
			for (i = 0; i < mesh->numvertexes; i++)
			{
				VectorCopy(mesh->xyz_array[i], out[i].coord);
				Vector2Copy(mesh->st_array[i], out[i].tex);
				VectorClear(out[i].ndir);
				VectorClear(out[i].sdir);
				VectorClear(out[i].tdir);
				*(unsigned int*)out[i].colorsb = *(unsigned int*)mesh->colors4b_array[i];
			}
		}
		else if (mesh->normals_array && !mesh->colors4f_array[0] && !mesh->colors4b_array)
		{
			//hlsl-lit models
			for (i = 0; i < mesh->numvertexes; i++)
			{
				VectorCopy(mesh->xyz_array[i], out[i].coord);
				Vector2Copy(mesh->st_array[i], out[i].tex);
				VectorCopy(mesh->normals_array[i], out[i].ndir);
				VectorCopy(mesh->snormals_array[i], out[i].sdir);
				VectorCopy(mesh->tnormals_array[i], out[i].tdir);
				*(unsigned int*)out[i].colorsb = 0xffffffff;	//write colours to ensure nothing is read back within the cpu cache block.
			}
		}
		else
		{
			//common stuff
			for (i = 0; i < mesh->numvertexes; i++)
			{
				VectorCopy(mesh->xyz_array[i], out[i].coord);
				Vector2Copy(mesh->st_array[i], out[i].tex);
			}
			//not so common stuff
			if (mesh->normals_array)
			{
				for (i = 0; i < mesh->numvertexes; i++)
				{
					VectorCopy(mesh->normals_array[i], out[i].ndir);
					VectorCopy(mesh->snormals_array[i], out[i].sdir);
					VectorCopy(mesh->tnormals_array[i], out[i].tdir);
				}
			}
			//some sort of colours
			if (mesh->colors4b_array)
			{
				for (i = 0; i < mesh->numvertexes; i++)
				{
					Vector4Copy(mesh->colors4b_array[i], out[i].colorsb);
				}
			}
			else if (mesh->colors4f_array[0])
			{
				for (i = 0; i < mesh->numvertexes; i++)
				{
					Vector4Scale(mesh->colors4f_array[0][i], 255, out[i].colorsb);
				}
			}
			else
			{
				for (i = 0; i < mesh->numvertexes; i++)
				{
					Vector4Set(out[i].colorsb, 255, 255, 255, 255);
				}
			}
		}

		//and we're done
		ID3D11DeviceContext_Unmap(d3ddevctx, (ID3D11Resource*)buf, 0);
	}

	//now ebo
	{
		index_t *out;
		sz = sizeof(*out) * mesh->numindexes;
		if (shaderstate.purgeindexstream || shaderstate.indexstreamoffset + sz > VERTEXSTREAMSIZE)
		{
			shaderstate.purgeindexstream = false;
			shaderstate.indexstreamoffset = 0;
			type = D3D11_MAP_WRITE_DISCARD;
			shaderstate.indexstreamcycle++;
			if (shaderstate.indexstreamcycle == NUMVBUFFERS)
				shaderstate.indexstreamcycle = 0;
		}
		else
		{
			type = D3D11_MAP_WRITE_NO_OVERWRITE;
		}
		buf = shaderstate.indexstream[shaderstate.indexstreamcycle];
		if (FAILED(ID3D11DeviceContext_Map(d3ddevctx, (ID3D11Resource*)buf, 0, type, 0, &msr)))
		{
			Con_Printf("BE_RotateForEntity: failed to map vertex stream buffer start\n");
			return false;
		}

		out = (index_t*)((qbyte*)msr.pData + shaderstate.indexstreamoffset);
		tmpvbo.indicies.d3d.buff = buf;
		tmpvbo.indicies.d3d.offs = shaderstate.indexstreamoffset;
		//consumed
		shaderstate.indexstreamoffset += sz;

		memcpy(out, mesh->indexes, sz);

		//and we're done
		ID3D11DeviceContext_Unmap(d3ddevctx, (ID3D11Resource*)buf, 0);
	}

	tmpvbo.indexcount = mesh->numindexes;
	tmpvbo.vertcount = mesh->numvertexes;
	tmpvbo.next = NULL;

	*vbo = &tmpvbo;

	return true;
}

void D3D11BE_GenBatchVBOs(vbo_t **vbochain, batch_t *firstbatch, batch_t *stopbatch)
{
	int maxvboelements;
	int maxvboverts;
	int vert = 0, idx = 0;
	batch_t *batch;
	vbo_t *vbo;
	int i, j;
	mesh_t *m;
	ID3D11Buffer *vbuff;
	ID3D11Buffer *ebuff;
	index_t *vboedata, *vboedatastart;
	vbovdata_t *vbovdata, *vbovdatastart;
	D3D11_BUFFER_DESC vbodesc;
	D3D11_BUFFER_DESC ebodesc;
	D3D11_SUBRESOURCE_DATA srd;

	vbo = Z_Malloc(sizeof(*vbo));

	maxvboverts = 0;
	maxvboelements = 0;
	for(batch = firstbatch; batch != stopbatch; batch = batch->next)
	{
		for (i=0 ; i<batch->maxmeshes ; i++)
		{
			m = batch->mesh[i];
			maxvboelements += m->numindexes;
			maxvboverts += m->numvertexes;
		}
	}

	vbovdatastart = vbovdata = BZ_Malloc(sizeof(*vbovdata) * maxvboverts);
	vboedatastart = vboedata = BZ_Malloc(sizeof(*vboedata) * maxvboelements);

	for(batch = firstbatch; batch != stopbatch; batch = batch->next)
	{
		batch->vbo = vbo;
		for (j=0 ; j<batch->maxmeshes ; j++)
		{
			m = batch->mesh[j];
			m->vbofirstvert = vert;
			for (i = 0; i < m->numvertexes; i++)
			{
				VectorCopy(m->xyz_array[i],			vbovdata->coord);
				vbovdata->coord[3] = 1;
				Vector2Copy(m->st_array[i],			vbovdata->tex);
				if (m->lmst_array[0])
					Vector2Copy(m->lmst_array[0][i],		vbovdata->lm);
				else
					Vector2Copy(m->st_array[i],			vbovdata->tex);
				if (m->normals_array)
					VectorCopy(m->normals_array[i],		vbovdata->ndir);
				else
					VectorSet(vbovdata->ndir, 0, 0, 1);
				if (m->snormals_array)
					VectorCopy(m->snormals_array[i],	vbovdata->sdir);
				else
					VectorSet(vbovdata->sdir, 1, 0, 0);
				if (m->tnormals_array)
					VectorCopy(m->tnormals_array[i],	vbovdata->tdir);
				else
					VectorSet(vbovdata->tdir, 0, 1, 0);
				if (m->colors4f_array[0])
					Vector4Scale(m->colors4f_array[0][i],	255, vbovdata->colorsb);
				else if (m->colors4b_array)
					Vector4Copy(m->colors4b_array[i],	vbovdata->colorsb);
				else
					Vector4Set(vbovdata->colorsb, 255, 255, 255, 255);

				vbovdata++;
			}

			m->vbofirstelement = idx;
			for (i = 0; i < m->numindexes; i++)
			{
				*vboedata++ = vert + m->indexes[i];
			}
			idx += m->numindexes;
			vert += m->numvertexes;
		}
	}

	//generate the ebo, and submit the data to the driver
	ebodesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ebodesc.ByteWidth = sizeof(*vboedata) * maxvboelements;
	ebodesc.CPUAccessFlags = 0;
	ebodesc.MiscFlags = 0;
	ebodesc.StructureByteStride = 0;
	ebodesc.Usage = D3D11_USAGE_DEFAULT;
	srd.pSysMem = vboedatastart;
	srd.SysMemPitch = 0;
	srd.SysMemSlicePitch = 0;
	ID3D11Device_CreateBuffer(pD3DDev11, &ebodesc, &srd, &ebuff);
	shaderstate.numlivevbos++;
	BZ_Free(vboedatastart);

	//generate the vbo, and submit the data to the driver
	vbodesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbodesc.ByteWidth = sizeof(*vbovdata) * maxvboverts;
	vbodesc.CPUAccessFlags = 0;
	vbodesc.MiscFlags = 0;
	vbodesc.StructureByteStride = 0;
	vbodesc.Usage = D3D11_USAGE_DEFAULT;
	srd.pSysMem = vbovdatastart;
	srd.SysMemPitch = 0;
	srd.SysMemSlicePitch = 0;
	ID3D11Device_CreateBuffer(pD3DDev11, &vbodesc, &srd, &vbuff);
	shaderstate.numlivevbos++;
	BZ_Free(vbovdatastart);

	vbovdata = NULL;
	vbo->coord.d3d.buff = vbuff;
	vbo->coord.d3d.offs = (quintptr_t)&vbovdata->coord;
	vbo->texcoord.d3d.buff = vbuff;
	vbo->texcoord.d3d.offs = (quintptr_t)&vbovdata->tex;
	vbo->lmcoord[0].d3d.buff = vbuff;
	vbo->lmcoord[0].d3d.offs = (quintptr_t)&vbovdata->lm;
	vbo->normals.d3d.buff = vbuff;
	vbo->normals.d3d.offs = (quintptr_t)&vbovdata->ndir;
	vbo->svector.d3d.buff = vbuff;
	vbo->svector.d3d.offs = (quintptr_t)&vbovdata->sdir;
	vbo->tvector.d3d.buff = vbuff;
	vbo->tvector.d3d.offs = (quintptr_t)&vbovdata->tdir;
	vbo->colours[0].d3d.buff = vbuff;
	vbo->colours[0].d3d.offs = (quintptr_t)&vbovdata->colorsb;
	vbo->indicies.d3d.buff = ebuff;
	vbo->indicies.d3d.offs = 0;

	vbo->indexcount = maxvboelements;
	vbo->vertcount = maxvboverts;

	vbo->next = *vbochain;
	*vbochain = vbo;
}

void D3D11BE_GenBrushModelVBO(model_t *mod)
{
	unsigned int vcount;


	batch_t *batch, *fbatch;
	int sortid;
	int i;

	fbatch = NULL;
	vcount = 0;
	for (sortid = 0; sortid < SHADER_SORT_COUNT; sortid++)
	{
		if (!mod->batches[sortid])
			continue;

		for (fbatch = batch = mod->batches[sortid]; batch != NULL; batch = batch->next)
		{
			//firstmesh got reused as the number of verticies in each batch
			if (vcount + batch->firstmesh > MAX_INDICIES)
			{
				D3D11BE_GenBatchVBOs(&mod->vbos, fbatch, batch);
				fbatch = batch;
				vcount = 0;
			}

			for (i = 0; i < batch->maxmeshes; i++)
				vcount += batch->mesh[i]->numvertexes;
		}

		D3D11BE_GenBatchVBOs(&mod->vbos, fbatch, batch);
	}
}

/*Wipes a vbo*/
void D3D11BE_ClearVBO(vbo_t *vbo)
{
	ID3D11Buffer *vbuff = vbo->coord.d3d.buff;
	ID3D11Buffer *ebuff = vbo->indicies.d3d.buff;
	if (vbuff)
	{
		ID3D11Buffer_Release(vbuff);
		shaderstate.numlivevbos--;
	}
	if (ebuff)
	{
		ID3D11Buffer_Release(ebuff);
		shaderstate.numlivevbos--;
	}
	vbo->coord.d3d.buff = NULL;
	vbo->indicies.d3d.buff = NULL;

	BZ_Free(vbo);
}

/*upload all lightmaps at the start to reduce lags*/
static void BE_UploadLightmaps(qboolean force)
{
	int i;

	for (i = 0; i < numlightmaps; i++)
	{
		if (!lightmap[i])
			continue;

		if (force)
		{
			lightmap[i]->rectchange.l = 0;
			lightmap[i]->rectchange.t = 0;
			lightmap[i]->rectchange.w = LMBLOCK_WIDTH;
			lightmap[i]->rectchange.h = LMBLOCK_HEIGHT;
			lightmap[i]->modified = true;
		}

		if (lightmap[i]->modified)
		{
			D3D11_UploadLightmap(lightmap[i]);
		}
	}
}

void D3D11BE_UploadAllLightmaps(void)
{
	BE_UploadLightmaps(true);
}

qboolean D3D11BE_LightCullModel(vec3_t org, model_t *model)
{
#ifdef RTLIGHTS
	if ((shaderstate.mode == BEM_LIGHT || shaderstate.mode == BEM_STENCIL))
	{
		/*true if hidden from current light*/
		/*we have no rtlight support, so mneh*/
	}
#endif
	return false;
}

batch_t *D3D11BE_GetTempBatch(void)
{
	if (shaderstate.wbatch >= shaderstate.maxwbatches)
	{
		shaderstate.wbatch++;
		return NULL;
	}
	return &shaderstate.wbatches[shaderstate.wbatch++];
}

float projd3dtogl[16] = 
{
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 2.0, 0.0,
	0.0, 0.0, -1.0, 1.0
};
float projgltod3d[16] = 
{
	1.0, 0.0, 0.0, 0.0,
	0.0, 1.0, 0.0, 0.0,
	0.0, 0.0, 0.5, 0.0,
	0.0, 0.0, 0.5, 1.0
};
void D3D11BE_SetupViewCBuffer(void)
{
	cbuf_view_t *cbv;
	D3D11_MAPPED_SUBRESOURCE msr;
	if (FAILED(ID3D11DeviceContext_Map(d3ddevctx, (ID3D11Resource*)shaderstate.vcbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr)))
	{
		Con_Printf("BE_RotateForEntity: failed to map constant buffer\n");
		return;
	}
	cbv = (cbuf_view_t*)msr.pData;

	//we internally use gl-style projection matricies.
	//gl's viewport is based upon -1 to 1 depth.
	//d3d uses 0 to 1 depth.
	//so we scale the projection matrix by a bias
#if 1
	Matrix4_Multiply(projgltod3d, r_refdef.m_projection, cbv->m_projection);
#else
	memcpy(cbv->m_projection, r_refdef.m_projection, sizeof(cbv->m_projection));
	cbv->m_projection[10] = r_refdef.m_projection[10] * 0.5;
#endif
	memcpy(cbv->m_view, r_refdef.m_view, sizeof(cbv->m_view));
	VectorCopy(r_origin, cbv->v_eyepos);
	cbv->v_time = r_refdef.time;

	ID3D11DeviceContext_Unmap(d3ddevctx, (ID3D11Resource*)shaderstate.vcbuffer, 0);
}
void D3D11BE_SetupLightCBuffer(dlight_t *l, vec3_t colour)
{
	extern cvar_t gl_specular;
	cbuf_light_t *cbl;
	D3D11_MAPPED_SUBRESOURCE msr;
	if (FAILED(ID3D11DeviceContext_Map(d3ddevctx, (ID3D11Resource*)shaderstate.lcbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr)))
	{
		Con_Printf("BE_RotateForEntity: failed to map constant buffer\n");
		return;
	}
	cbl = (cbuf_light_t*)msr.pData;

	cbl->l_lightradius = l->radius;

	Matrix4x4_CM_LightMatrixFromAxis(cbl->l_cubematrix, l->axis[0], l->axis[1], l->axis[2], l->origin);
	VectorCopy(l->origin, cbl->l_lightposition);
	cbl->padl1 = 0;
	VectorCopy(colour, cbl->l_colour);
#ifdef RTLIGHTS
	VectorCopy(l->lightcolourscales, cbl->l_lightcolourscale);
	cbl->l_lightcolourscale[0] = l->lightcolourscales[0];
	cbl->l_lightcolourscale[1] = l->lightcolourscales[1];
	cbl->l_lightcolourscale[2] = l->lightcolourscales[2] * gl_specular.value;
#endif
	cbl->l_lightradius = l->radius;
	Vector4Copy(shaderstate.lightshadowmapproj, cbl->l_shadowmapproj);
	Vector2Copy(shaderstate.lightshadowmapscale, cbl->l_shadowmapscale);

	ID3D11DeviceContext_Unmap(d3ddevctx, (ID3D11Resource*)shaderstate.lcbuffer, 0);
}


//also updates the entity constant buffer
static void BE_RotateForEntity (const entity_t *e, const model_t *mod)
{
	int i;
	float ndr;
	float mv[16], modelinv[16];
	float *m = shaderstate.m_model;
	cbuf_entity_t *cbe;
	D3D11_MAPPED_SUBRESOURCE msr;
	shaderstate.ecbufferidx = (shaderstate.ecbufferidx + 1) & (NUMECBUFFERS-1);
	if (FAILED(ID3D11DeviceContext_Map(d3ddevctx, (ID3D11Resource*)shaderstate.ecbuffers[shaderstate.ecbufferidx], 0, D3D11_MAP_WRITE_DISCARD, 0, &msr)))
	{
		Con_Printf("BE_RotateForEntity: failed to map constant buffer\n");
		return;
	}
	cbe = (cbuf_entity_t*)msr.pData;


	shaderstate.curentity = e;

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

	if (e->scale != 1 && e->scale != 0)	//hexen 2 stuff
	{
		float z;
		float escale;
		escale = e->scale;
		switch(e->drawflags&SCALE_TYPE_MASKIN)
		{
		default:
		case SCALE_TYPE_UNIFORM:
			VectorScale((m+0), escale, (m+0));
			VectorScale((m+4), escale, (m+4));
			VectorScale((m+8), escale, (m+8));
			break;
		case SCALE_TYPE_XYONLY:
			VectorScale((m+0), escale, (m+0));
			VectorScale((m+4), escale, (m+4));
			break;
		case SCALE_TYPE_ZONLY:
			VectorScale((m+8), escale, (m+8));
			break;
		}
		if (mod && (e->drawflags&SCALE_TYPE_MASKIN) != SCALE_TYPE_XYONLY)
		{
			switch(e->drawflags&SCALE_ORIGIN_MASKIN)
			{
			case SCALE_ORIGIN_CENTER:
				z = ((mod->maxs[2] + mod->mins[2]) * (1-escale))/2;
				VectorMA((m+12), z, e->axis[2], (m+12));
				break;
			case SCALE_ORIGIN_BOTTOM:
				VectorMA((m+12), mod->mins[2]*(1-escale), e->axis[2], (m+12));
				break;
			case SCALE_ORIGIN_TOP:
				VectorMA((m+12), -mod->maxs[2], e->axis[2], (m+12));
				break;
			}
		}
	}
	else if (mod && !strcmp(mod->name, "progs/eyes.mdl"))
	{
		/*resize eyes, to make them easier to see*/
		m[14] -= (22 + 8);
		VectorScale((m+0), 2, (m+0));
		VectorScale((m+4), 2, (m+4));
		VectorScale((m+8), 2, (m+8));
	}
	if (mod && !ruleset_allow_larger_models.ival && mod->clampscale != 1)
	{	//possibly this should be on a per-frame basis, but that's a real pain to do
		Con_DPrintf("Rescaling %s by %f\n", mod->name, mod->clampscale);
		VectorScale((m+0), mod->clampscale, (m+0));
		VectorScale((m+4), mod->clampscale, (m+4));
		VectorScale((m+8), mod->clampscale, (m+8));
	}

	if (e->flags & RF_WEAPONMODEL)
	{
		/*FIXME: no bob*/
		float iv[16];
		Matrix4_Invert(r_refdef.m_view, iv);
		Matrix4x4_CM_NewRotation(90, 1, 0, 0);
		Matrix4_Multiply(iv, m, mv);
		Matrix4_Multiply(mv, Matrix4x4_CM_NewRotation(-90, 1, 0, 0), iv);
		Matrix4_Multiply(iv, Matrix4x4_CM_NewRotation(90, 0, 0, 1), mv);

		memcpy(cbe->m_model, mv, sizeof(cbe->m_model));
	}
	else
	{
		memcpy(cbe->m_model, m, sizeof(cbe->m_model));
	}

	Matrix4_Invert(shaderstate.m_model, modelinv);

	cbe->e_time = r_refdef.time - shaderstate.curentity->shaderTime;

	VectorCopy(e->light_avg, cbe->e_light_ambient);
	VectorCopy(e->light_dir, cbe->e_light_dir);
	VectorCopy(e->light_range, cbe->e_light_mul);

	//various stuff in modelspace
	Matrix4x4_CM_Transform3(modelinv, r_origin, cbe->e_eyepos);

	for (i = 0; i < MAXRLIGHTMAPS ; i++)
	{
		extern cvar_t gl_overbright;
		unsigned char s = shaderstate.curbatch?shaderstate.curbatch->lmlightstyle[i]:0;
		float sc;
		if (s == 255)
		{
			if (i == 0)
			{
				if (shaderstate.curentity->model && shaderstate.curentity->model->engineflags & MDLF_NEEDOVERBRIGHT)
					sc = (1<<bound(0, gl_overbright.ival, 2)) * shaderstate.identitylighting;
				else
					sc = shaderstate.identitylighting;
				cbe->e_lmscale[i][0] = sc;
				cbe->e_lmscale[i][1] = sc;
				cbe->e_lmscale[i][2] = sc;
				cbe->e_lmscale[i][3] = 1;
				i++;
			}
			for (; i < MAXRLIGHTMAPS ; i++)
			{
				cbe->e_lmscale[i][0] = 0;
				cbe->e_lmscale[i][1] = 0;
				cbe->e_lmscale[i][2] = 0;
				cbe->e_lmscale[i][3] = 1;
			}
			break;
		}
		if (shaderstate.curentity->model && shaderstate.curentity->model->engineflags & MDLF_NEEDOVERBRIGHT)
			sc = (1<<bound(0, gl_overbright.ival, 2)) * shaderstate.identitylighting;
		else
			sc = shaderstate.identitylighting;
		sc *= d_lightstylevalue[s]/256.0f;
		Vector4Set(cbe->e_lmscale[i], sc, sc, sc, 1);
	}

	ID3D11DeviceContext_Unmap(d3ddevctx, (ID3D11Resource*)shaderstate.ecbuffers[shaderstate.ecbufferidx], 0);

	ndr = (e->flags & RF_DEPTHHACK)?0.333:1;
	if (ndr != shaderstate.depthrange)
	{
		D3D11_VIEWPORT vport;

		shaderstate.depthrange = ndr;

		vport.TopLeftX = r_refdef.pxrect.x;
		vport.TopLeftY = r_refdef.pxrect.y;
		vport.Width = r_refdef.pxrect.width;
		vport.Height = r_refdef.pxrect.height;
		vport.MinDepth = 0;
		vport.MaxDepth = shaderstate.depthrange;
		ID3D11DeviceContext_RSSetViewports(d3ddevctx, 1, &vport);
	}
}

void D3D11BE_SubmitBatch(batch_t *batch)
{
	shaderstate.nummeshes = batch->meshes - batch->firstmesh;
	if (!shaderstate.nummeshes)
		return;
	if (shaderstate.curentity != batch->ent)
	{
		BE_RotateForEntity(batch->ent, batch->ent->model);
		shaderstate.curtime = r_refdef.time - shaderstate.curentity->shaderTime;
	}
	shaderstate.curbatch = batch;
	shaderstate.batchvbo = batch->vbo;
	shaderstate.meshlist = batch->mesh + batch->firstmesh;
	shaderstate.curshader = batch->shader;
	shaderstate.curtexnums = batch->skin?batch->skin:&batch->shader->defaulttextures;
	shaderstate.flags = batch->flags;

	if (!shaderstate.batchvbo)
	{
		if (!BE_GenTempMeshVBO(&shaderstate.batchvbo, batch->mesh[0]))
			return;
		BE_DrawMeshChain_Internal();
	}
	else
		BE_DrawMeshChain_Internal();
}

void D3D11BE_DrawMesh_List(shader_t *shader, int nummeshes, mesh_t **meshlist, vbo_t *vbo, texnums_t *texnums, unsigned int beflags)
{
	shaderstate.curbatch = &shaderstate.dummybatch;
	shaderstate.batchvbo = vbo;
	shaderstate.curshader = shader;
	shaderstate.curtexnums = texnums;
	shaderstate.meshlist = meshlist;
	shaderstate.nummeshes = nummeshes;
	shaderstate.flags = beflags;

	if (!shaderstate.batchvbo)
	{
		if (!BE_GenTempMeshVBO(&shaderstate.batchvbo, meshlist[0]))
			return;
		shaderstate.nummeshes = 1;
		BE_DrawMeshChain_Internal();
	}
	else
		BE_DrawMeshChain_Internal();
}

void D3D11BE_DrawMesh_Single(shader_t *shader, mesh_t *meshchain, vbo_t *vbo, texnums_t *texnums, unsigned int beflags)
{
	shaderstate.curbatch = &shaderstate.dummybatch;
	shaderstate.batchvbo = vbo;
	shaderstate.curtime = realtime;
	shaderstate.curshader = shader;
	shaderstate.curtexnums = texnums?texnums:&shader->defaulttextures;
	shaderstate.meshlist = &meshchain;
	shaderstate.nummeshes = 1;
	shaderstate.flags = beflags;

	if (!shaderstate.batchvbo)
	{
		if (!BE_GenTempMeshVBO(&shaderstate.batchvbo, meshchain))
			return;
		BE_DrawMeshChain_Internal();
	}
	else
		BE_DrawMeshChain_Internal();
}

static void BE_SubmitMeshesSortList(batch_t *sortlist)
{
	batch_t *batch;
	for (batch = sortlist; batch; batch = batch->next)
	{
		if (batch->meshes == batch->firstmesh)
			continue;

		if (batch->buildmeshes)
			batch->buildmeshes(batch);

		if (batch->shader->flags & SHADER_NODLIGHT)
			if (shaderstate.mode == BEM_LIGHT)
				continue;

		if (batch->shader->flags & SHADER_SKY)
		{
			if (!batch->shader->prog)
			{
				if (shaderstate.mode == BEM_STANDARD)
					R_DrawSkyChain (batch);
				continue;
			}
		}

		BE_SubmitBatch(batch);
	}
}


/*generates a new modelview matrix, as well as vpn vectors*/
static void R_MirrorMatrix(plane_t *plane)
{
	float mirror[16];
	float view[16];
	float result[16];

	vec3_t pnorm;
	VectorNegate(plane->normal, pnorm);

	mirror[0] = 1-2*pnorm[0]*pnorm[0];
	mirror[1] = -2*pnorm[0]*pnorm[1];
	mirror[2] = -2*pnorm[0]*pnorm[2];
	mirror[3] = 0;

	mirror[4] = -2*pnorm[1]*pnorm[0];
	mirror[5] = 1-2*pnorm[1]*pnorm[1];
	mirror[6] = -2*pnorm[1]*pnorm[2] ;
	mirror[7] = 0;

	mirror[8]  = -2*pnorm[2]*pnorm[0];
	mirror[9]  = -2*pnorm[2]*pnorm[1];
	mirror[10] = 1-2*pnorm[2]*pnorm[2];
	mirror[11] = 0;

	mirror[12] = -2*pnorm[0]*plane->dist;
	mirror[13] = -2*pnorm[1]*plane->dist;
	mirror[14] = -2*pnorm[2]*plane->dist;
	mirror[15] = 1;

	view[0] = vpn[0];
	view[1] = vpn[1];
	view[2] = vpn[2];
	view[3] = 0;

	view[4] = -vright[0];
	view[5] = -vright[1];
	view[6] = -vright[2];
	view[7] = 0;

	view[8]  = vup[0];
	view[9]  = vup[1];
	view[10] = vup[2];
	view[11] = 0;

	view[12] = r_refdef.vieworg[0];
	view[13] = r_refdef.vieworg[1];
	view[14] = r_refdef.vieworg[2];
	view[15] = 1;

	VectorMA(r_refdef.vieworg, 0.25, plane->normal, r_refdef.pvsorigin);

	Matrix4_Multiply(mirror, view, result);

	vpn[0] = result[0];
	vpn[1] = result[1];
	vpn[2] = result[2];

	vright[0] = -result[4];
	vright[1] = -result[5];
	vright[2] = -result[6];

	vup[0] = result[8];
	vup[1] = result[9];
	vup[2] = result[10];

	r_refdef.vieworg[0] = result[12];
	r_refdef.vieworg[1] = result[13];
	r_refdef.vieworg[2] = result[14];
}
static entity_t *R_NearestPortal(plane_t *plane)
{
	int i;
	entity_t *best = NULL;
	float dist, bestd = 0;
	//for q3-compat, portals on world scan for a visedict to use for their view.
	for (i = 0; i < cl_numvisedicts; i++)
	{
		if (cl_visedicts[i].rtype == RT_PORTALSURFACE)
		{
			dist = DotProduct(cl_visedicts[i].origin, plane->normal)-plane->dist;
			dist = fabs(dist);
			if (dist < 64 && (!best || dist < bestd))
				best = &cl_visedicts[i];
		}
	}
	return best;
}

static void TransformCoord(vec3_t in, vec3_t planea[3], vec3_t planeo, vec3_t viewa[3], vec3_t viewo, vec3_t result)
{
	int		i;
	vec3_t	local;
	vec3_t	transformed;
	float	d;

	local[0] = in[0] - planeo[0];
	local[1] = in[1] - planeo[1];
	local[2] = in[2] - planeo[2];

	VectorClear(transformed);
	for ( i = 0 ; i < 3 ; i++ )
	{
		d = DotProduct(local, planea[i]);
		VectorMA(transformed, d, viewa[i], transformed);
	}

	result[0] = transformed[0] + viewo[0];
	result[1] = transformed[1] + viewo[1];
	result[2] = transformed[2] + viewo[2];
}
static void TransformDir(vec3_t in, vec3_t planea[3], vec3_t viewa[3], vec3_t result)
{
	int		i;
	float	d;
	vec3_t tmp;

	VectorCopy(in, tmp);

	VectorClear(result);
	for ( i = 0 ; i < 3 ; i++ )
	{
		d = DotProduct(tmp, planea[i]);
		VectorMA(result, d, viewa[i], result);
	}
}
static void R_RenderScene(void)
{
//	IDirect3DDevice9_SetTransform(pD3DDev9, D3DTS_PROJECTION, (D3DMATRIX*)d3d_trueprojection);
//	IDirect3DDevice9_SetTransform(pD3DDev9, D3DTS_VIEW, (D3DMATRIX*)r_refdef.m_view);
	R_SetFrustum (r_refdef.m_projection, r_refdef.m_view);
	Surf_DrawWorld();
}

static void R_DrawPortal(batch_t *batch, batch_t **blist)
{
	entity_t *view;
	float glplane[4];
	plane_t plane;
	refdef_t oldrefdef;
	mesh_t *mesh = batch->mesh[batch->firstmesh];
	int sort;

	if (r_refdef.recurse || !r_portalrecursion.ival)
		return;

	VectorCopy(mesh->normals_array[0], plane.normal);
	plane.dist = DotProduct(mesh->xyz_array[0], plane.normal);

	//if we're too far away from the surface, don't draw anything
	if (batch->shader->flags & SHADER_AGEN_PORTAL)
	{
		/*there's a portal alpha blend on that surface, that fades out after this distance*/
		if (DotProduct(r_refdef.vieworg, plane.normal)-plane.dist > batch->shader->portaldist)
			return;
	}
	//if we're behind it, then also don't draw anything.
	if (DotProduct(r_refdef.vieworg, plane.normal)-plane.dist < 0)
		return;

	view = R_NearestPortal(&plane);
	//if (!view)
	//	return;

	oldrefdef = r_refdef;
	r_refdef.recurse = true;

	r_refdef.externalview = true;

	if (!view || VectorCompare(view->origin, view->oldorigin))
	{
		r_refdef.flipcull ^= SHADER_CULL_FLIP;
		R_MirrorMatrix(&plane);
	}
	else
	{
		float d;
		vec3_t paxis[3], porigin, vaxis[3], vorg;
		void PerpendicularVector( vec3_t dst, const vec3_t src );

		/*calculate where the surface is meant to be*/
		VectorCopy(mesh->normals_array[0], paxis[0]);
		PerpendicularVector(paxis[1], paxis[0]);
		CrossProduct(paxis[0], paxis[1], paxis[2]);
		d = DotProduct(view->origin, plane.normal) - plane.dist;
		VectorMA(view->origin, -d, paxis[0], porigin);

		/*grab the camera origin*/
		VectorNegate(view->axis[0], vaxis[0]);
		VectorNegate(view->axis[1], vaxis[1]);
		VectorCopy(view->axis[2], vaxis[2]);
		VectorCopy(view->oldorigin, vorg);

		VectorCopy(vorg, r_refdef.pvsorigin);

		/*rotate it a bit*/
		RotatePointAroundVector(vaxis[1], vaxis[0], view->axis[1], sin(realtime)*4);
		CrossProduct(vaxis[0], vaxis[1], vaxis[2]);

		TransformCoord(oldrefdef.vieworg, paxis, porigin, vaxis, vorg, r_refdef.vieworg);
		TransformDir(vpn, paxis, vaxis, vpn);
		TransformDir(vright, paxis, vaxis, vright);
		TransformDir(vup, paxis, vaxis, vup);
	}
	Matrix4x4_CM_ModelViewMatrixFromAxis(r_refdef.m_view, vpn, vright, vup, r_refdef.vieworg);
	VectorAngles(vpn, vup, r_refdef.viewangles);
	VectorCopy(r_refdef.vieworg, r_origin);

/*FIXME: the batch stuff should be done in renderscene*/

	/*fixup the first mesh index*/
	for (sort = 0; sort < SHADER_SORT_COUNT; sort++)
	for (batch = blist[sort]; batch; batch = batch->next)
	{
		batch->firstmesh = batch->meshes;
	}

	/*FIXME: can we get away with stenciling the screen?*/
	/*Add to frustum culling instead of clip planes?*/
	glplane[0] = plane.normal[0];
	glplane[1] = plane.normal[1];
	glplane[2] = plane.normal[2];
	glplane[3] = -plane.dist;
//	IDirect3DDevice9_SetClipPlane(pD3DDev9, 0, glplane);
//	IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_CLIPPLANEENABLE, D3DCLIPPLANE0);
	R_RenderScene();
//	IDirect3DDevice9_SetRenderState(pD3DDev9, D3DRS_CLIPPLANEENABLE, 0);

	for (sort = 0; sort < SHADER_SORT_COUNT; sort++)
	for (batch = blist[sort]; batch; batch = batch->next)
	{
		batch->firstmesh = 0;
	}
	r_refdef = oldrefdef;

	/*broken stuff*/
	AngleVectors (r_refdef.viewangles, vpn, vright, vup);
	VectorCopy (r_refdef.vieworg, r_origin);

//	IDirect3DDevice9_SetTransform(pD3DDev9, D3DTS_PROJECTION, (D3DMATRIX*)d3d_trueprojection);
//	IDirect3DDevice9_SetTransform(pD3DDev9, D3DTS_VIEW, (D3DMATRIX*)r_refdef.m_view);
	R_SetFrustum (r_refdef.m_projection, r_refdef.m_view);
}

static void BE_SubmitMeshesPortals(batch_t **worldlist, batch_t *dynamiclist)
{
	batch_t *batch, *old;
	int i;
	/*attempt to draw portal shaders*/
	if (shaderstate.mode == BEM_STANDARD)
	{
		for (i = 0; i < 2; i++)
		{
			for (batch = i?dynamiclist:worldlist[SHADER_SORT_PORTAL]; batch; batch = batch->next)
			{
				if (batch->meshes == batch->firstmesh)
					continue;

				if (batch->buildmeshes)
					batch->buildmeshes(batch);

				/*draw already-drawn portals as depth-only, to ensure that their contents are not harmed*/
				BE_SelectMode(BEM_DEPTHONLY);
				for (old = worldlist[SHADER_SORT_PORTAL]; old && old != batch; old = old->next)
				{
					if (old->meshes == old->firstmesh)
						continue;
					BE_SubmitBatch(old);
				}
				if (!old)
				{
					for (old = dynamiclist; old != batch; old = old->next)
					{
						if (old->meshes == old->firstmesh)
							continue;
						BE_SubmitBatch(old);
					}
				}
				BE_SelectMode(BEM_STANDARD);

				R_DrawPortal(batch, worldlist);

				/*clear depth again*/
//				IDirect3DDevice9_Clear(pD3DDev9, 0, NULL, D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 1, 0);
			}
		}
	}
}

void D3D11BE_SubmitMeshes (qboolean drawworld, batch_t **blist, int first, int stop)
{
	model_t *model = cl.worldmodel;
	int i;

	for (i = first; i < stop; i++)
	{
		if (drawworld)
		{
			if (i == SHADER_SORT_PORTAL /*&& !r_noportals.ival*/ && !r_refdef.recurse)
				BE_SubmitMeshesPortals(model->batches, blist[i]);

			BE_SubmitMeshesSortList(model->batches[i]);
		}
		BE_SubmitMeshesSortList(blist[i]);
	}
}

#ifdef RTLIGHTS
void D3D11BE_BaseEntTextures(void)
{
	batch_t *batches[SHADER_SORT_COUNT];
	BE_GenModelBatches(batches, shaderstate.curdlight, shaderstate.mode);
	D3D11BE_SubmitMeshes(false, batches, SHADER_SORT_PORTAL, SHADER_SORT_DECAL);
	BE_SelectEntity(&r_worldentity);
}

void D3D11BE_GenerateShadowBuffer(void **vbuf_out, vecV_t *verts, int numverts, void **ibuf_out, index_t *indicies, int numindicies)
{
	D3D11_BUFFER_DESC desc;
	D3D11_SUBRESOURCE_DATA srd;
	ID3D11Buffer *vbuf;
	ID3D11Buffer *ibuf;


	//generate the ebo, and submit the data to the driver
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.ByteWidth = sizeof(*verts) * numverts;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	srd.pSysMem = verts;
	srd.SysMemPitch = 0;
	srd.SysMemSlicePitch = 0;
	ID3D11Device_CreateBuffer(pD3DDev11, &desc, &srd, &vbuf);

	//generate the vbo, and submit the data to the driver
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.ByteWidth = sizeof(*indicies) * numindicies;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	srd.pSysMem = indicies;
	srd.SysMemPitch = 0;
	srd.SysMemSlicePitch = 0;
	ID3D11Device_CreateBuffer(pD3DDev11, &desc, &srd, &ibuf);

	shaderstate.numliveshadowbuffers++;

	*vbuf_out = vbuf;
	*ibuf_out = ibuf;
}
void D3D11_DestroyShadowBuffer(void *vbuf_in, void *ibuf_in)
{
	ID3D11Buffer *vbuf = vbuf_in;
	ID3D11Buffer *ibuf = ibuf_in;

	if (vbuf && ibuf)
	{
		ID3D11Buffer_Release(vbuf);
		ID3D11Buffer_Release(ibuf);
		shaderstate.numliveshadowbuffers--;
	}
}
//draws all depth-only surfaces from the perspective of the light.
void D3D11BE_RenderShadowBuffer(unsigned int numverts, void *vbuf, unsigned int numindicies, void *ibuf)
{
	ID3D11Buffer *vbufs[] = {vbuf};
	int vstrides[] = {sizeof(vecV_t)};
	int voffsets[] = {0};
	int i;

	if (!shaderstate.depthonly->prog)
		return;

	D3D11BE_SetupViewCBuffer();

	D3D11BE_Cull(SHADER_CULL_FRONT);

	for (i = 0; i < shaderstate.lastpasscount; i++)
	{
		shaderstate.pendingtextures[i] = NULL;
		shaderstate.textureschanged = true;
	}
	if (shaderstate.textureschanged)
		ID3D11DeviceContext_PSSetShaderResources(d3ddevctx, 0, shaderstate.lastpasscount, shaderstate.pendingtextures);
	shaderstate.lastpasscount = 0;

	ID3D11DeviceContext_IASetVertexBuffers(d3ddevctx, 0, 1, vbufs, vstrides, voffsets);
	ID3D11DeviceContext_IASetIndexBuffer(d3ddevctx, ibuf, DXGI_FORMAT_R16_UINT, 0);

	BE_ApplyUniforms(shaderstate.depthonly->prog, 0);

	ID3D11DeviceContext_DrawIndexed(d3ddevctx, numindicies, 0, 0);
}
void D3D11BE_DoneShadows(void)
{
	D3D11BE_SetupViewCBuffer();
	BE_SelectEntity(&r_worldentity);

	D3D11BE_BeginShadowmapFace();
}
#endif

void D3D11BE_DrawWorld (qboolean drawworld, qbyte *vis)
{
	batch_t *batches[SHADER_SORT_COUNT];
	RSpeedLocals();

	shaderstate.curentity = NULL;

	shaderstate.depthrange = 0;

	if (!r_refdef.recurse)
	{
		if (shaderstate.wbatch > shaderstate.maxwbatches)
		{
			int newm = shaderstate.wbatch;
			Z_Free(shaderstate.wbatches);
			shaderstate.wbatches = Z_Malloc(newm * sizeof(*shaderstate.wbatches));
			memset(shaderstate.wbatches + shaderstate.maxwbatches, 0, (newm - shaderstate.maxwbatches) * sizeof(*shaderstate.wbatches));
			shaderstate.maxwbatches = newm;
		}
		shaderstate.wbatch = 0;
	}

	D3D11BE_SetupViewCBuffer();

	shaderstate.curdlight = NULL;
	BE_GenModelBatches(batches, shaderstate.curdlight, BEM_STANDARD);

	if (vis)
	{
		BE_UploadLightmaps(false);

		//make sure the world draws correctly
		r_worldentity.shaderRGBAf[0] = 1;
		r_worldentity.shaderRGBAf[1] = 1;
		r_worldentity.shaderRGBAf[2] = 1;
		r_worldentity.shaderRGBAf[3] = 1;
		r_worldentity.axis[0][0] = 1;
		r_worldentity.axis[1][1] = 1;
		r_worldentity.axis[2][2] = 1;

#ifdef RTLIGHTS
		if (vis && r_shadow_realtime_world.ival)
			shaderstate.identitylighting = r_shadow_realtime_world_lightmaps.value;
		else
#endif
			shaderstate.identitylighting = 1;
//		shaderstate.identitylightmap = shaderstate.identitylighting / (1<<gl_overbright.ival);

		BE_SelectMode(BEM_STANDARD);

		RSpeedRemark();
		D3D11BE_SubmitMeshes(true, batches, SHADER_SORT_PORTAL, SHADER_SORT_DECAL);
		RSpeedEnd(RSPEED_WORLD);

#ifdef RTLIGHTS
		RSpeedRemark();
		D3D11BE_SelectEntity(&r_worldentity);
		Sh_DrawLights(vis);
		RSpeedEnd(RSPEED_STENCILSHADOWS);
#endif

		D3D11BE_SubmitMeshes(true, batches, SHADER_SORT_DECAL, SHADER_SORT_COUNT);
	}
	else
	{
		RSpeedRemark();
		shaderstate.identitylighting = 1;
		D3D11BE_SubmitMeshes(false, batches, SHADER_SORT_PORTAL, SHADER_SORT_COUNT);
		RSpeedEnd(RSPEED_DRAWENTITIES);
	}

	R_RenderDlights ();

	BE_RotateForEntity(&r_worldentity, NULL);
}

void D3D11BE_VBO_Begin(vbobctx_t *ctx, unsigned int maxsize)
{
}
void D3D11BE_VBO_Data(vbobctx_t *ctx, void *data, unsigned int size, vboarray_t *varray)
{
}
void D3D11BE_VBO_Finish(vbobctx_t *ctx, void *edata, unsigned int esize, vboarray_t *earray)
{
}
void D3D11BE_VBO_Destroy(vboarray_t *vearray)
{
}

void D3D11BE_Scissor(srect_t *rect)
{
	D3D11_RECT drect;
	if (rect)
	{
		drect.left = (rect->x)*vid.pixelwidth;
		drect.right = (rect->x + rect->width)*vid.pixelwidth;
		drect.bottom = (1-(rect->y))*vid.pixelheight;
		drect.top = (1-(rect->y + rect->height))*vid.pixelheight;
	}
	else
	{
		drect.left = 0;
		drect.right = vid.pixelwidth;
		drect.top = 0;
		drect.bottom = vid.pixelheight;
	}
	ID3D11DeviceContext_RSSetScissorRects(d3ddevctx, 1, &drect);
}

#ifdef RTLIGHTS
void D3D11BE_BeginShadowmapFace(void)
{
	D3D11_VIEWPORT vport;

	vport.TopLeftX = r_refdef.pxrect.x;
	vport.TopLeftY = r_refdef.pxrect.y;
	vport.Width = r_refdef.pxrect.width;
	vport.Height = r_refdef.pxrect.height;
	vport.MinDepth = 0;
	vport.MaxDepth = 1;
	ID3D11DeviceContext_RSSetViewports(d3ddevctx, 1, &vport);

	D3D11BE_Cull(SHADER_CULL_FRONT);
}
#endif

#endif
