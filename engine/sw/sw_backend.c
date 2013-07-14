#include "quakedef.h"
#ifdef SWQUAKE
#include "sw.h"
#include "shader.h"
#include "glquake.h"

vecV_t vertbuf[65535];

static struct
{
	int foo;
	int numrthreads;
	void *threads[4];
	backendmode_t mode;

	float m_mvp[16];
	vec4_t viewplane;

	entity_t *curentity;
	shader_t *curshader;

	float curtime;
	//this stuff should probably be moved out of the backend
	int wbatch;
	int maxwbatches;
	batch_t *wbatches;
} shaderstate;

////////////////////////////////////////////////////////////////
//start generic tables 
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

static void BE_InitTables(void)
{
	int i;
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
#define R_FastSin(x) sin((x)*(2*M_PI))	//fixme: use r_sintable instead!

//end generic tables 
////////////////////////////////////////////////////////////////
//start matrix functions

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
static void Matrix3_Multiply_Vec3 (mat3_t a, vec3_t b, vec3_t product)
{
	product[0] = a[0][0]*b[0] + a[0][1]*b[1] + a[0][2]*b[2];
	product[1] = a[1][0]*b[0] + a[1][1]*b[1] + a[1][2]*b[2];
	product[2] = a[2][0]*b[0] + a[2][1]*b[1] + a[2][2]*b[2];
}

static int Matrix3_Compare(mat3_t in, mat3_t out)
{
	return memcmp(in, out, sizeof(mat3_t));
}

//end matrix functions
////////////////////////////////////////////////////////////////
//start xyz

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
				Matrix3_Multiply_Vec3(result, tv, quad[j]);
				VectorAdd(rot_centre, quad[j], quad[j]);
			}
		}
		break;

//	case DEFORMV_PROJECTION_SHADOW:
//		break;
	}
}
//end xyz
////////////////////////////////////////////////////////////////

void SWBE_SelectMode(backendmode_t mode)
{
}

void SWBE_TransformVerticies(swvert_t *v, mesh_t *mesh)
{
	extern cvar_t temp1;
	int i;

	vecV_t *xyz;

	/*generate vertex blends*/
	if (mesh->xyz2_array)
	{
		xyz = vertbuf;
		for (i = 0; i < mesh->numvertexes; i++)
		{
			VectorInterpolate(mesh->xyz_array[i], mesh->xyz_blendw[1], mesh->xyz2_array[i], xyz[i]);
		}
	}
	/*else if (skeletal)
	{
	}
	*/
	else
	{
		xyz = mesh->xyz_array;
	}

	/*now apply any shader deforms*/
	if (shaderstate.curshader->numdeforms)
	{
		deformgen(&shaderstate.curshader->deforms[0], mesh->numvertexes, xyz, vertbuf, mesh);
		xyz = vertbuf;
		for (i = 1; i < shaderstate.curshader->numdeforms; i++)
		{
			deformgen(&shaderstate.curshader->deforms[i], mesh->numvertexes, xyz, xyz, mesh);
		}
	}

	for (i = 0; i < mesh->numvertexes; i++, v++)
	{
		VectorCopy(xyz[i], v->vcoord);

		Vector2Copy(mesh->st_array[i], v->tccoord);

//		v->colour[0] = mesh->colors4b_array[i][0];
//		v->colour[1] = mesh->colors4b_array[i][1];
//		v->colour[2] = mesh->colors4b_array[i][2];
//		v->colour[3] = mesh->colors4b_array[i][3];
	}
}
void SWBE_DrawMesh_Single(shader_t *shader, mesh_t *mesh, struct vbo_s *vbo, struct texnums_s *texnums, unsigned int be_flags)
{
	wqcom_t *com;

	shaderstate.curshader = shader;

	if (mesh->istrifan)
	{
		com = SWRast_BeginCommand(&commandqueue, WTC_TRIFAN, mesh->numvertexes*sizeof(swvert_t) + sizeof(com->trifan) - sizeof(com->trifan.verts));

		com->trifan.texture = texnums->base.ptr;
		com->trifan.numverts = mesh->numvertexes;

		SWBE_TransformVerticies(com->trifan.verts, mesh);

		SWRast_EndCommand(&commandqueue, com);
	}
	else
	{
		com = SWRast_BeginCommand(&commandqueue, WTC_TRISOUP, (mesh->numvertexes*sizeof(swvert_t)) + sizeof(com->trisoup) - sizeof(com->trisoup.verts) + (sizeof(index_t)*mesh->numindexes));
		
		com->trisoup.texture = texnums->base.ptr;
		com->trisoup.numverts = mesh->numvertexes;
		com->trisoup.numidx = mesh->numindexes;

		SWBE_TransformVerticies(com->trisoup.verts, mesh);
		memcpy(com->trisoup.verts + mesh->numvertexes, mesh->indexes, sizeof(index_t)*mesh->numindexes);

		SWRast_EndCommand(&commandqueue, com);
	}
}
void SWBE_DrawMesh_List(shader_t *shader, int nummeshes, struct mesh_s **mesh, struct vbo_s *vbo, struct texnums_s *texnums, unsigned int be_flags)
{
	while(nummeshes-->0)
	{
		SWBE_DrawMesh_Single(shader, *mesh++, vbo, texnums, be_flags);
	}
}
void SWBE_SubmitBatch(struct batch_s *batch)
{
	int m;
	SWBE_SelectEntity(batch->ent);
	for (m = 0; m < batch->meshes; m++)
	{
		SWBE_DrawMesh_Single(batch->shader, batch->mesh[m], batch->vbo, batch->skin?batch->skin:&batch->shader->defaulttextures, batch->flags);
	}
}
struct batch_s *SWBE_GetTempBatch(void)
{
	if (shaderstate.wbatch >= shaderstate.maxwbatches)
	{
		shaderstate.wbatch++;
		return NULL;
	}
	return &shaderstate.wbatches[shaderstate.wbatch++];
}

static void SWBE_SubmitMeshesSortList(batch_t *sortlist)
{
	batch_t *batch;
	for (batch = sortlist; batch; batch = batch->next)
	{
		if (batch->meshes == batch->firstmesh)
			continue;

		if (batch->flags & BEF_NODLIGHT)
			if (shaderstate.mode == BEM_LIGHT || shaderstate.mode == BEM_SMAPLIGHT)
				continue;
		if (batch->flags & BEF_NOSHADOWS)
			if (shaderstate.mode == BEM_STENCIL)
				continue;

		if (batch->buildmeshes)
			batch->buildmeshes(batch);
		else if (batch->texture)
			batch->shader = R_TextureAnimation(batch->ent->framestate.g[FS_REG].frame[0], batch->texture)->shader;

		if (batch->shader->flags & SHADER_NODRAW)
			continue;
		if (batch->shader->flags & SHADER_NODLIGHT)
			if (shaderstate.mode == BEM_LIGHT || shaderstate.mode == BEM_SMAPLIGHT)
				continue;
		if (batch->shader->flags & SHADER_SKY)
		{
			if (shaderstate.mode == BEM_STANDARD || shaderstate.mode == BEM_DEPTHDARK)
			{
				if (!batch->shader->prog)
				{
					R_DrawSkyChain (batch);
					continue;
				}
			}
			else if (shaderstate.mode != BEM_FOG && shaderstate.mode != BEM_CREPUSCULAR)
				continue;
		}

		SWBE_SubmitBatch(batch);
	}
}

void SWBE_SubmitMeshes (qboolean drawworld, batch_t **blist, int start, int stop)
{
	model_t *model = cl.worldmodel;
	int i;

	for (i = start; i <= stop; i++)
	{
		if (drawworld)
		{
//			if (i == SHADER_SORT_PORTAL && !r_noportals.ival && !r_refdef.recurse)
//				SWBE_SubmitMeshesPortals(model->batches, blist[i]);

			SWBE_SubmitMeshesSortList(model->batches[i]);
		}
		SWBE_SubmitMeshesSortList(blist[i]);
	}
}

static void SWBE_UpdateUniforms(void)
{
	wqcom_t *com;
	com = SWRast_BeginCommand(&commandqueue, WTC_UNIFORMS, sizeof(com->uniforms));

	memcpy(com->uniforms.u.matrix, shaderstate.m_mvp, sizeof(com->uniforms.u.matrix));
	Vector4Copy(shaderstate.viewplane, com->uniforms.u.viewplane);

	SWRast_EndCommand(&commandqueue, com);
}
void SWBE_Set2D(void)
{
	extern cvar_t gl_screenangle;
	float ang, rad, w, h;
	float tmp[16];
	float tmp2[16];
	ang = (gl_screenangle.value>0?(gl_screenangle.value+45):(gl_screenangle.value-45))/90;
	ang = (int)ang * 90;
	if (ang)
	{ /*more expensive maths*/
		rad = (ang * M_PI) / 180;

		w = fabs(cos(rad)) * (vid.width) + fabs(sin(rad)) * (vid.height);
		h = fabs(sin(rad)) * (vid.width) + fabs(cos(rad)) * (vid.height);

		Matrix4x4_CM_Orthographic(r_refdef.m_projection, w/-2.0f, w/2.0f, h/2.0f, h/-2.0f, -99999, 99999);

		Matrix4x4_Identity(tmp);
		Matrix4_Multiply(Matrix4x4_CM_NewTranslation((vid.width/-2.0f), (vid.height/-2.0f), 0), tmp, tmp2);
		Matrix4_Multiply(Matrix4x4_CM_NewRotation(-ang,  0, 0, 1), tmp2, r_refdef.m_view);
	}
	else
	{
		if (0)
			Matrix4x4_CM_Orthographic(r_refdef.m_projection, 0, vid.width, 0, vid.height, 0, 99999);
		else
			Matrix4x4_CM_Orthographic(r_refdef.m_projection, 0, vid.width, vid.height, 0, 0, 99999);
		Matrix4x4_Identity(r_refdef.m_view);
	}

	memcpy(shaderstate.m_mvp, r_refdef.m_projection, sizeof(shaderstate.m_mvp));

	shaderstate.viewplane[0] = -r_refdef.m_view[0*4+2];
	shaderstate.viewplane[1] = -r_refdef.m_view[1*4+2];
	shaderstate.viewplane[2] = -r_refdef.m_view[2*4+2];
	VectorNormalize(shaderstate.viewplane);
	VectorScale(shaderstate.viewplane, 1.0/99999, shaderstate.viewplane);
	shaderstate.viewplane[3] = DotProduct(vec3_origin, shaderstate.viewplane);// - 0.5;

	SWBE_UpdateUniforms();
}
void SWBE_DrawWorld(qboolean drawworld, qbyte *vis)
{
	batch_t *batches[SHADER_SORT_COUNT];

	if (!r_refdef.recurse)
	{
		if (shaderstate.wbatch + 50 > shaderstate.maxwbatches)
		{
			int newm = shaderstate.wbatch + 100;
			shaderstate.wbatches = BZ_Realloc(shaderstate.wbatches, newm * sizeof(*shaderstate.wbatches));
			memset(shaderstate.wbatches + shaderstate.maxwbatches, 0, (newm - shaderstate.maxwbatches) * sizeof(*shaderstate.wbatches));
			shaderstate.maxwbatches = newm;
		}

		shaderstate.wbatch = 0;
	}
	BE_GenModelBatches(batches);
//	R_GenDlightBatches(batches);

	shaderstate.curentity = NULL;
	SWBE_SelectEntity(&r_worldentity);

	SWBE_SubmitMeshes(drawworld, batches, SHADER_SORT_PORTAL, SHADER_SORT_NEAREST);

	SWBE_Set2D();
}
void SWBE_Init(void)
{
	BE_InitTables();
}
void SWBE_GenBrushModelVBO(struct model_s *mod)
{
}
void SWBE_ClearVBO(struct vbo_s *vbo)
{
}
void SWBE_UploadAllLightmaps(void)
{
}
void SWBE_SelectEntity(struct entity_s *ent)
{
	float modelmatrix[16];
	float modelviewmatrix[16];
	vec3_t vieworg;

	if (shaderstate.curentity == ent)
		return;
	shaderstate.curentity = ent;

	R_RotateForEntity(modelmatrix, modelviewmatrix, shaderstate.curentity, shaderstate.curentity->model);
	Matrix4_Multiply(r_refdef.m_projection, modelviewmatrix, shaderstate.m_mvp);
	shaderstate.viewplane[0] = vpn[0];//-modelviewmatrix[0];//0*4+2];
	shaderstate.viewplane[1] = vpn[1];//-modelviewmatrix[1];//1*4+2];
	shaderstate.viewplane[2] = vpn[2];//-modelviewmatrix[2];//2*4+2];
	VectorNormalize(shaderstate.viewplane);
	VectorScale(shaderstate.viewplane, 1.0/8192, shaderstate.viewplane);
	vieworg[0] = modelviewmatrix[3*4+0];
	vieworg[1] = modelviewmatrix[3*4+1];
	vieworg[2] = modelviewmatrix[3*4+2];
	VectorMA(r_refdef.vieworg, 256, shaderstate.viewplane, vieworg);
	shaderstate.viewplane[3] = DotProduct(vieworg, shaderstate.viewplane);

	SWBE_UpdateUniforms();
}
void SWBE_SelectDLight(struct dlight_s *dl, vec3_t colour)
{
}
qboolean SWBE_LightCullModel(vec3_t org, struct model_s *model)
{
	return false;
}
#endif
