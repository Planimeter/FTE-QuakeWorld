//gl suppliment for Quake

#define APIENTRYP APIENTRY *

//contains the extra things that would otherwise be found in glext.h

typedef void (APIENTRY *qlpMTex2FUNC) (GLenum, GLfloat, GLfloat);
typedef void (APIENTRY *qlpMTex3FUNC) (GLenum, GLfloat, GLfloat, GLfloat);
typedef void (APIENTRY *qlpSelTexFUNC) (GLenum);

extern qlpSelTexFUNC	qglActiveTextureARB;
extern qlpSelTexFUNC	qglClientActiveTextureARB;
extern qlpMTex3FUNC		qglMultiTexCoord3fARB;
extern qlpMTex2FUNC		qglMultiTexCoord2fARB;

//This stuff is normally supplied in the <GL/glext.h> header file. I don't actually have one of them, so it's here instead.
#if 0	//change to 1 if you do actually have the file in question.
#include <GL/glext.h>	//would be ideal.
#else

#ifndef GL_TEXTURE_WIDTH
#define GL_TEXTURE_WIDTH                  0x1000
#endif
#ifndef GL_TEXTURE_HEIGHT
#define GL_TEXTURE_HEIGHT                 0x1001
#endif
#ifndef GL_TEXTURE_INTERNAL_FORMAT
#define GL_TEXTURE_INTERNAL_FORMAT		0x1003
#endif

//#ifndef GL_VERSION_1_2
#define GL_CLAMP_TO_EDGE                  0x812F
//#endif


#ifndef GL_ARB_multitexture
#define GL_ARB_multitexture 1
#define GL_TEXTURE0_ARB                   0x84C0
#define GL_TEXTURE1_ARB                   0x84C1
#define GL_TEXTURE2_ARB                   0x84C2
#define GL_TEXTURE3_ARB                   0x84C3
#define GL_TEXTURE4_ARB                   0x84C4
#define GL_TEXTURE5_ARB                   0x84C5
#define GL_TEXTURE6_ARB                   0x84C6
#define GL_TEXTURE7_ARB                   0x84C7
#define GL_TEXTURE8_ARB                   0x84C8
#define GL_TEXTURE9_ARB                   0x84C9
#define GL_TEXTURE10_ARB                  0x84CA
#define GL_TEXTURE11_ARB                  0x84CB
#define GL_TEXTURE12_ARB                  0x84CC
#define GL_TEXTURE13_ARB                  0x84CD
#define GL_TEXTURE14_ARB                  0x84CE
#define GL_TEXTURE15_ARB                  0x84CF
#define GL_TEXTURE16_ARB                  0x84D0
#define GL_TEXTURE17_ARB                  0x84D1
#define GL_TEXTURE18_ARB                  0x84D2
#define GL_TEXTURE19_ARB                  0x84D3
#define GL_TEXTURE20_ARB                  0x84D4
#define GL_TEXTURE21_ARB                  0x84D5
#define GL_TEXTURE22_ARB                  0x84D6
#define GL_TEXTURE23_ARB                  0x84D7
#define GL_TEXTURE24_ARB                  0x84D8
#define GL_TEXTURE25_ARB                  0x84D9
#define GL_TEXTURE26_ARB                  0x84DA
#define GL_TEXTURE27_ARB                  0x84DB
#define GL_TEXTURE28_ARB                  0x84DC
#define GL_TEXTURE29_ARB                  0x84DD
#define GL_TEXTURE30_ARB                  0x84DE
#define GL_TEXTURE31_ARB                  0x84DF
#define GL_ACTIVE_TEXTURE_ARB             0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE_ARB      0x84E1
#define GL_MAX_TEXTURE_UNITS_ARB          0x84E2
#endif

#ifndef GL_ARB_texture_cube_map
#define GL_ARB_texture_cube_map 1
#define GL_NORMAL_MAP_ARB                 0x8511
#define GL_REFLECTION_MAP_ARB             0x8512
#define GL_TEXTURE_CUBE_MAP_ARB           0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP_ARB   0x8514
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB 0x851A
#define GL_PROXY_TEXTURE_CUBE_MAP_ARB     0x851B
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB  0x851C
#endif



#ifndef GL_EXT_texture3D
#define define GL_EXT_texture3D 1
#define GL_PACK_SKIP_IMAGES               0x806B
#define GL_PACK_SKIP_IMAGES_EXT           0x806B
#define GL_PACK_IMAGE_HEIGHT              0x806C
#define GL_PACK_IMAGE_HEIGHT_EXT          0x806C
#define GL_UNPACK_SKIP_IMAGES             0x806D
#define GL_UNPACK_SKIP_IMAGES_EXT         0x806D
#define GL_UNPACK_IMAGE_HEIGHT            0x806E
#define GL_UNPACK_IMAGE_HEIGHT_EXT        0x806E
#define GL_TEXTURE_3D                     0x806F
#define GL_TEXTURE_3D_EXT                 0x806F
#define GL_PROXY_TEXTURE_3D               0x8070
#define GL_PROXY_TEXTURE_3D_EXT           0x8070
#define GL_TEXTURE_DEPTH                  0x8071
#define GL_TEXTURE_DEPTH_EXT              0x8071
#define GL_TEXTURE_WRAP_R                 0x8072
#define GL_TEXTURE_WRAP_R_EXT             0x8072
#define GL_MAX_3D_TEXTURE_SIZE            0x8073
#define GL_MAX_3D_TEXTURE_SIZE_EXT        0x8073
#endif








//some of these were needed.
//They were also not in the ones I could find on the web.
//GL_ARB_texture_env_combine
#define  GL_COMBINE_ARB					0x8570 
#define  GL_COMBINE_RGB_ARB				0x8571 
#define  GL_COMBINE_ALPHA_ARB			0x8572 
#define  GL_SOURCE0_RGB_ARB				0x8580 
#define  GL_SOURCE1_RGB_ARB				0x8581 
#define  GL_SOURCE2_RGB_ARB				0x8582 
#define  GL_SOURCE0_ALPHA_ARB			0x8588 
#define  GL_SOURCE1_ALPHA_ARB			0x8589 
#define  GL_SOURCE2_ALPHA_ARB			0x858A 
#define  GL_OPERAND0_RGB_ARB			0x8590 
#define  GL_OPERAND1_RGB_ARB			0x8591 
#define  GL_OPERAND2_RGB_ARB			0x8592 
#define  GL_OPERAND0_ALPHA_ARB			0x8598 
#define  GL_OPERAND1_ALPHA_ARB			0x8599 
#define  GL_OPERAND2_ALPHA_ARB			0x859A 
#define  GL_RGB_SCALE_ARB				0x8573 
#define  GL_ADD_SIGNED_ARB				0x8574 
#define  GL_INTERPOLATE_ARB				0x8575 
#define  GL_SUBTRACT_ARB				0x84E7 
#define  GL_CONSTANT_ARB				0x8576 
#define  GL_PRIMARY_COLOR_ARB			0x8577 
#define  GL_PREVIOUS_ARB				0x8578 


#define  GL_DOT3_RGB_ARB   0x86AE 
#define  GL_DOT3_RGBA_ARB   0x86AF 

//GL_EXT_texture_env_combine
#define  GL_COMBINE_EXT					0x8570
#define  GL_COMBINE_RGB_EXT				0x8571
#define  GL_COMBINE_ALPHA_EXT			0x8572
#define  GL_SOURCE0_RGB_EXT				0x8580
#define  GL_SOURCE1_RGB_EXT				0x8581
#define  GL_SOURCE2_RGB_EXT				0x8582
#define  GL_SOURCE0_ALPHA_EXT			0x8588
#define  GL_SOURCE1_ALPHA_EXT			0x8589
#define  GL_SOURCE2_ALPHA_EXT			0x858A
#define  GL_OPERAND0_RGB_EXT			0x8590
#define  GL_OPERAND1_RGB_EXT			0x8591
#define  GL_OPERAND2_RGB_EXT			0x8592
#define  GL_OPERAND0_ALPHA_EXT			0x8598
#define  GL_OPERAND1_ALPHA_EXT			0x8599
#define  GL_OPERAND2_ALPHA_EXT			0x859A
#define  GL_RGB_SCALE_EXT				0x8573
#define  GL_ADD_SIGNED_EXT				0x8574
#define  GL_INTERPOLATE_EXT				0x8575
#define  GL_CONSTANT_EXT				0x8576
#define  GL_PRIMARY_COLOR_EXT			0x8577
#define  GL_PREVIOUS_EXT				0x8578

//GL_NV_texture_env_combine4
#define  GL_COMBINE4_NV					0x8503
#define  GL_SOURCE3_RGB_NV				0x8583
#define  GL_SOURCE3_ALPHA_NV			0x858B
#define  GL_OPERAND3_RGB_NV				0x8593
#define  GL_OPERAND3_ALPHA_NV			0x859B



/* GL_ARB_texture_compression */
#ifndef GL_ARB_texture_compression
#define GL_ARB_texture_compression 1

#define GL_COMPRESSED_ALPHA_ARB                                 0x84E9
#define GL_COMPRESSED_LUMINANCE_ARB                             0x84EA
#define GL_COMPRESSED_LUMINANCE_ALPHA_ARB                       0x84EB
#define GL_COMPRESSED_INTENSITY_ARB                             0x84EC
#define GL_COMPRESSED_RGB_ARB                                   0x84ED
#define GL_COMPRESSED_RGBA_ARB                                  0x84EE
#define GL_TEXTURE_COMPRESSION_HINT_ARB                         0x84EF
#define GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB                    0x86A0
#define GL_TEXTURE_COMPRESSED_ARB                               0x86A1
#define GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB                   0x86A2
#define GL_COMPRESSED_TEXTURE_FORMATS_ARB                       0x86A3

typedef void (APIENTRY *PFNGLCOMPRESSEDTEXIMAGE3DARBPROC)	(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid* data);
typedef void (APIENTRY *PFNGLCOMPRESSEDTEXIMAGE2DARBPROC)	(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid* data);
typedef void (APIENTRY *PFNGLCOMPRESSEDTEXIMAGE1DARBPROC)	(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid* data);
typedef void (APIENTRY *PFNGLCOMPRESSEDTEXSUBIMAGE3DARBPROC)	(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid* data);
typedef void (APIENTRY *PFNGLCOMPRESSEDTEXSUBIMAGE2DARBPROC)	(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid* data);
typedef void (APIENTRY *PFNGLCOMPRESSEDTEXSUBIMAGE1DARBPROC)	(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid* data);
typedef void (APIENTRY *PFNGLGETCOMPRESSEDTEXIMAGEARBPROC)	(GLenum target, GLint lod, const GLvoid* img);

#endif /* GL_ARB_texture_compression */


#ifndef GL_ATI_pn_triangles	//ati truform
#define GL_PN_TRIANGLES_ATI							0x87F0
#define GL_MAX_PN_TRIANGLES_TESSELATION_LEVEL_ATI	0x87F1
#define GL_PN_TRIANGLES_POINT_MODE_ATI				0x87F2
#define GL_PN_TRIANGLES_NORMAL_MODE_ATI				0x87F3
#define GL_PN_TRIANGLES_TESSELATION_LEVEL_ATI		0x87F4
#define GL_PN_TRIANGLES_POINT_MODE_LINEAR_ATI		0x87F5
#define GL_PN_TRIANGLES_POINT_MODE_CUBIC_ATI		0x87F6
#define GL_PN_TRIANGLES_NORMAL_MODE_LINEAR_ATI		0x87F7
#define GL_PN_TRIANGLES_NORMAL_MODE_QUADRATIC_ATI	0x87F8

typedef void (APIENTRY *PFNGLPNTRIANGLESIATIPROC)(GLenum pname, GLint param);
typedef void (APIENTRY *PFNGLPNTRIANGLESFATIPROC)(GLenum pname, GLfloat param);
#endif


#ifndef GL_EXT_stencil_two_side
#define GL_EXT_stencil_two_side 1

#define GL_STENCIL_TEST_TWO_SIDE_EXT				0x8910
#define GL_ACTIVE_STENCIL_FACE_EXT					0x8911

typedef void (APIENTRY * PFNGLACTIVESTENCILFACEEXTPROC) (GLenum face);
#endif

#ifndef GL_EXT_stencil_wrap
#define GL_EXT_stencil_wrap 1
#define GL_INCR_WRAP_EXT							0x8507
#define GL_DECR_WRAP_EXT							0x8508
#endif

#ifndef GL_EXT_texture_filter_anisotropic
#define GL_EXT_texture_filter_anisotropic 1
#define GL_TEXTURE_MAX_ANISOTROPY_EXT				0x84FE
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT			0x84FF
#endif







#ifndef GL_ARB_vertex_program
#define GL_COLOR_SUM_ARB                  0x8458
#define GL_VERTEX_PROGRAM_ARB             0x8620
#define GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB 0x8622
#define GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB   0x8623
#define GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB 0x8624
#define GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB   0x8625
#define GL_CURRENT_VERTEX_ATTRIB_ARB      0x8626
#define GL_PROGRAM_LENGTH_ARB             0x8627
#define GL_PROGRAM_STRING_ARB             0x8628
#define GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB 0x862E
#define GL_MAX_PROGRAM_MATRICES_ARB       0x862F
#define GL_CURRENT_MATRIX_STACK_DEPTH_ARB 0x8640
#define GL_CURRENT_MATRIX_ARB             0x8641
#define GL_VERTEX_PROGRAM_POINT_SIZE_ARB  0x8642
#define GL_VERTEX_PROGRAM_TWO_SIDE_ARB    0x8643
#define GL_VERTEX_ATTRIB_ARRAY_POINTER_ARB 0x8645
#define GL_PROGRAM_ERROR_POSITION_ARB     0x864B
#define GL_PROGRAM_BINDING_ARB            0x8677
#define GL_MAX_VERTEX_ATTRIBS_ARB         0x8869
#define GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB 0x886A
#define GL_PROGRAM_ERROR_STRING_ARB       0x8874
#define GL_PROGRAM_FORMAT_ASCII_ARB       0x8875
#define GL_PROGRAM_FORMAT_ARB             0x8876
#define GL_PROGRAM_INSTRUCTIONS_ARB       0x88A0
#define GL_MAX_PROGRAM_INSTRUCTIONS_ARB   0x88A1
#define GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB 0x88A2
#define GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB 0x88A3
#define GL_PROGRAM_TEMPORARIES_ARB        0x88A4
#define GL_MAX_PROGRAM_TEMPORARIES_ARB    0x88A5
#define GL_PROGRAM_NATIVE_TEMPORARIES_ARB 0x88A6
#define GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB 0x88A7
#define GL_PROGRAM_PARAMETERS_ARB         0x88A8
#define GL_MAX_PROGRAM_PARAMETERS_ARB     0x88A9
#define GL_PROGRAM_NATIVE_PARAMETERS_ARB  0x88AA
#define GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB 0x88AB
#define GL_PROGRAM_ATTRIBS_ARB            0x88AC
#define GL_MAX_PROGRAM_ATTRIBS_ARB        0x88AD
#define GL_PROGRAM_NATIVE_ATTRIBS_ARB     0x88AE
#define GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB 0x88AF
#define GL_PROGRAM_ADDRESS_REGISTERS_ARB  0x88B0
#define GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB 0x88B1
#define GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB 0x88B2
#define GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB 0x88B3
#define GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB 0x88B4
#define GL_MAX_PROGRAM_ENV_PARAMETERS_ARB 0x88B5
#define GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB 0x88B6
#define GL_TRANSPOSE_CURRENT_MATRIX_ARB   0x88B7
#define GL_MATRIX0_ARB                    0x88C0
#define GL_MATRIX1_ARB                    0x88C1
#define GL_MATRIX2_ARB                    0x88C2
#define GL_MATRIX3_ARB                    0x88C3
#define GL_MATRIX4_ARB                    0x88C4
#define GL_MATRIX5_ARB                    0x88C5
#define GL_MATRIX6_ARB                    0x88C6
#define GL_MATRIX7_ARB                    0x88C7
#define GL_MATRIX8_ARB                    0x88C8
#define GL_MATRIX9_ARB                    0x88C9
#define GL_MATRIX10_ARB                   0x88CA
#define GL_MATRIX11_ARB                   0x88CB
#define GL_MATRIX12_ARB                   0x88CC
#define GL_MATRIX13_ARB                   0x88CD
#define GL_MATRIX14_ARB                   0x88CE
#define GL_MATRIX15_ARB                   0x88CF
#define GL_MATRIX16_ARB                   0x88D0
#define GL_MATRIX17_ARB                   0x88D1
#define GL_MATRIX18_ARB                   0x88D2
#define GL_MATRIX19_ARB                   0x88D3
#define GL_MATRIX20_ARB                   0x88D4
#define GL_MATRIX21_ARB                   0x88D5
#define GL_MATRIX22_ARB                   0x88D6
#define GL_MATRIX23_ARB                   0x88D7
#define GL_MATRIX24_ARB                   0x88D8
#define GL_MATRIX25_ARB                   0x88D9
#define GL_MATRIX26_ARB                   0x88DA
#define GL_MATRIX27_ARB                   0x88DB
#define GL_MATRIX28_ARB                   0x88DC
#define GL_MATRIX29_ARB                   0x88DD
#define GL_MATRIX30_ARB                   0x88DE
#define GL_MATRIX31_ARB                   0x88DF
#endif

#ifndef GL_ARB_fragment_program
#define GL_FRAGMENT_PROGRAM_ARB           0x8804
#define GL_PROGRAM_ALU_INSTRUCTIONS_ARB   0x8805
#define GL_PROGRAM_TEX_INSTRUCTIONS_ARB   0x8806
#define GL_PROGRAM_TEX_INDIRECTIONS_ARB   0x8807
#define GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB 0x8808
#define GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB 0x8809
#define GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB 0x880A
#define GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB 0x880B
#define GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB 0x880C
#define GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB 0x880D
#define GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB 0x880E
#define GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB 0x880F
#define GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB 0x8810
#define GL_MAX_TEXTURE_COORDS_ARB         0x8871
#define GL_MAX_TEXTURE_IMAGE_UNITS_ARB    0x8872
#endif





#ifndef GL_ARB_vertex_program
#define GL_ARB_vertex_program 1
typedef void (APIENTRYP PFNGLVERTEXATTRIB1DARBPROC) (GLuint index, GLdouble x);
typedef void (APIENTRYP PFNGLVERTEXATTRIB1DVARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB1FARBPROC) (GLuint index, GLfloat x);
typedef void (APIENTRYP PFNGLVERTEXATTRIB1FVARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB1SARBPROC) (GLuint index, GLshort x);
typedef void (APIENTRYP PFNGLVERTEXATTRIB1SVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB2DARBPROC) (GLuint index, GLdouble x, GLdouble y);
typedef void (APIENTRYP PFNGLVERTEXATTRIB2DVARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB2FARBPROC) (GLuint index, GLfloat x, GLfloat y);
typedef void (APIENTRYP PFNGLVERTEXATTRIB2FVARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB2SARBPROC) (GLuint index, GLshort x, GLshort y);
typedef void (APIENTRYP PFNGLVERTEXATTRIB2SVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB3DARBPROC) (GLuint index, GLdouble x, GLdouble y, GLdouble z);
typedef void (APIENTRYP PFNGLVERTEXATTRIB3DVARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB3FARBPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRYP PFNGLVERTEXATTRIB3FVARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB3SARBPROC) (GLuint index, GLshort x, GLshort y, GLshort z);
typedef void (APIENTRYP PFNGLVERTEXATTRIB3SVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4NBVARBPROC) (GLuint index, const GLbyte *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4NIVARBPROC) (GLuint index, const GLint *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4NSVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4NUBARBPROC) (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4NUBVARBPROC) (GLuint index, const GLubyte *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4NUIVARBPROC) (GLuint index, const GLuint *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4NUSVARBPROC) (GLuint index, const GLushort *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4BVARBPROC) (GLuint index, const GLbyte *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4DARBPROC) (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4DVARBPROC) (GLuint index, const GLdouble *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4FARBPROC) (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4FVARBPROC) (GLuint index, const GLfloat *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4IVARBPROC) (GLuint index, const GLint *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4SARBPROC) (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4SVARBPROC) (GLuint index, const GLshort *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4UBVARBPROC) (GLuint index, const GLubyte *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4UIVARBPROC) (GLuint index, const GLuint *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIB4USVARBPROC) (GLuint index, const GLushort *v);
typedef void (APIENTRYP PFNGLVERTEXATTRIBPOINTERARBPROC) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
typedef void (APIENTRYP PFNGLENABLEVERTEXATTRIBARRAYARBPROC) (GLuint index);
typedef void (APIENTRYP PFNGLDISABLEVERTEXATTRIBARRAYARBPROC) (GLuint index);
typedef void (APIENTRYP PFNGLPROGRAMSTRINGARBPROC) (GLenum target, GLenum format, GLsizei len, const GLvoid *string);
typedef void (APIENTRYP PFNGLBINDPROGRAMARBPROC) (GLenum target, GLuint program);
typedef void (APIENTRYP PFNGLDELETEPROGRAMSARBPROC) (GLsizei n, const GLuint *programs);
typedef void (APIENTRYP PFNGLGENPROGRAMSARBPROC) (GLsizei n, GLuint *programs);
typedef void (APIENTRYP PFNGLPROGRAMENVPARAMETER4DARBPROC) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRYP PFNGLPROGRAMENVPARAMETER4DVARBPROC) (GLenum target, GLuint index, const GLdouble *params);
typedef void (APIENTRYP PFNGLPROGRAMENVPARAMETER4FARBPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRYP PFNGLPROGRAMENVPARAMETER4FVARBPROC) (GLenum target, GLuint index, const GLfloat *params);
typedef void (APIENTRYP PFNGLPROGRAMLOCALPARAMETER4DARBPROC) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
typedef void (APIENTRYP PFNGLPROGRAMLOCALPARAMETER4DVARBPROC) (GLenum target, GLuint index, const GLdouble *params);
typedef void (APIENTRYP PFNGLPROGRAMLOCALPARAMETER4FARBPROC) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
typedef void (APIENTRYP PFNGLPROGRAMLOCALPARAMETER4FVARBPROC) (GLenum target, GLuint index, const GLfloat *params);
typedef void (APIENTRYP PFNGLGETPROGRAMENVPARAMETERDVARBPROC) (GLenum target, GLuint index, GLdouble *params);
typedef void (APIENTRYP PFNGLGETPROGRAMENVPARAMETERFVARBPROC) (GLenum target, GLuint index, GLfloat *params);
typedef void (APIENTRYP PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC) (GLenum target, GLuint index, GLdouble *params);
typedef void (APIENTRYP PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC) (GLenum target, GLuint index, GLfloat *params);
typedef void (APIENTRYP PFNGLGETPROGRAMIVARBPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETPROGRAMSTRINGARBPROC) (GLenum target, GLenum pname, GLvoid *string);
typedef void (APIENTRYP PFNGLGETVERTEXATTRIBDVARBPROC) (GLuint index, GLenum pname, GLdouble *params);
typedef void (APIENTRYP PFNGLGETVERTEXATTRIBFVARBPROC) (GLuint index, GLenum pname, GLfloat *params);
typedef void (APIENTRYP PFNGLGETVERTEXATTRIBIVARBPROC) (GLuint index, GLenum pname, GLint *params);
typedef void (APIENTRYP PFNGLGETVERTEXATTRIBPOINTERVARBPROC) (GLuint index, GLenum pname, GLvoid* *pointer);
typedef GLboolean (APIENTRYP PFNGLISPROGRAMARBPROC) (GLuint program);
#endif

#ifndef GL_ARB_fragment_program
#define GL_ARB_fragment_program 1
/* All ARB_fragment_program entry points are shared with ARB_vertex_program. */
#endif




#ifndef GL_ARB_shader_objects
#define GL_ARB_shader_objects 1
#define GL_PROGRAM_OBJECT_ARB			0x8B40
#define GL_OBJECT_TYPE_ARB				0x8B4E
#define GL_OBJECT_SUBTYPE_ARB			0x8B4F
#define GL_OBJECT_DELETE_STATUS_ARB		0x8B80
#define GL_OBJECT_COMPILE_STATUS_ARB	0x8B81
#define GL_OBJECT_LINK_STATUS_ARB		0x8B82
#define GL_OBJECT_VALIDATE_STATUS_ARB	0x8B83
#define GL_OBJECT_INFO_LOG_LENGTH_ARB	0x8B84
#define GL_OBJECT_ATTACHED_OBJECTS_ARB	0x8B85
#define GL_OBJECT_ACTIVE_UNIFORMS_ARB	0x8B86
#define GL_OBJECT_ACTIVE_UNIFORM_MAX_LENGTH_ARB	0x8B87
#define GL_OBJECT_SHADER_SOURCE_LENGTH_ARB		0x8B88
#define GL_SHADER_OBJECT_ARB			0x8B48
#define GL_FLOAT						0x1406
#define GL_FLOAT_VEC2_ARB				0x8B50
#define GL_FLOAT_VEC3_ARB				0x8B51
#define GL_FLOAT_VEC4_ARB				0x8B52
//#define GL_INT							0x1404
#define GL_INT_VEC2_ARB					0x8B53
#define GL_INT_VEC3_ARB					0x8B54
#define GL_INT_VEC4_ARB					0x8B55
#define GL_BOOL_ARB						0x8B56
#define GL_BOOL_VEC2_ARB				0x8B57
#define GL_BOOL_VEC3_ARB				0x8B58
#define GL_BOOL_VEC4_ARB				0x8B59
#define GL_FLOAT_MAT2_ARB				0x8B5A
#define GL_FLOAT_MAT3_ARB				0x8B5B
#define GL_FLOAT_MAT4_ARB				0x8B5C
#define GL_SAMPLER_1D_ARB				0x8B5D
#define GL_SAMPLER_2D_ARB				0x8B5E
#define GL_SAMPLER_3D_ARB				0x8B5F
#define GL_SAMPLER_CUBE_ARB				0x8B60
#define GL_SAMPLER_1D_SHADOW_ARB		0x8B61
#define GL_SAMPLER_2D_SHADOW_ARB		0x8B62
#define GL_SAMPLER_2D_RECT_ARB			0x8B63
#define GL_SAMPLER_2D_RECT_SHADOW_ARB	0x8B64
// dont know if these two should go somewhere better:
typedef unsigned int GLhandleARB;
typedef char         GLcharARB;
typedef void		(APIENTRYP PFNGLDELETEOBJECTARBPROC)		(GLhandleARB obj);
typedef GLhandleARB	(APIENTRYP PFNGLGETHANDLEARBPROC)			(GLenum pname);
typedef void		(APIENTRYP PFNGLDETACHOBJECTARBPROC)		(GLhandleARB containerObj, GLhandleARB attachedObj);
typedef GLhandleARB	(APIENTRYP PFNGLCREATESHADEROBJECTARBPROC)	(GLenum shaderType);
typedef void		(APIENTRYP PFNGLSHADERSOURCEARBPROC)		(GLhandleARB shaderObj, GLsizei count, const GLcharARB* *string, const GLint *length);
typedef void		(APIENTRYP PFNGLCOMPILESHADERARBPROC)		(GLhandleARB shaderObj);
typedef GLhandleARB	(APIENTRYP PFNGLCREATEPROGRAMOBJECTARBPROC)	(void);
typedef void		(APIENTRYP PFNGLATTACHOBJECTARBPROC)		(GLhandleARB containerObj, GLhandleARB obj);
typedef void		(APIENTRYP PFNGLLINKPROGRAMARBPROC)			(GLhandleARB programObj);
typedef void		(APIENTRYP PFNGLUSEPROGRAMOBJECTARBPROC)	(GLhandleARB programObj);
typedef void		(APIENTRYP PFNGLVALIDATEPROGRAMARBPROC)		(GLhandleARB programObj);
typedef void		(APIENTRYP PFNGLUNIFORM1FARBPROC)			(GLint location, GLfloat v0);
typedef void		(APIENTRYP PFNGLUNIFORM2FARBPROC)			(GLint location, GLfloat v0, GLfloat v1);
typedef void		(APIENTRYP PFNGLUNIFORM3FARBPROC)			(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void		(APIENTRYP PFNGLUNIFORM4FARBPROC)			(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void		(APIENTRYP PFNGLUNIFORM1IARBPROC)			(GLint location, GLint v0);
typedef void		(APIENTRYP PFNGLUNIFORM2IARBPROC)			(GLint location, GLint v0, GLint v1);
typedef void		(APIENTRYP PFNGLUNIFORM3IARBPROC)			(GLint location, GLint v0, GLint v1, GLint v2);
typedef void		(APIENTRYP PFNGLUNIFORM4IARBPROC)			(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
typedef void		(APIENTRYP PFNGLUNIFORM1FVARBPROC)			(GLint location, GLsizei count, GLfloat *value);
typedef void		(APIENTRYP PFNGLUNIFORM2FVARBPROC)			(GLint location, GLsizei count, GLfloat *value);
typedef void		(APIENTRYP PFNGLUNIFORM3FVARBPROC)			(GLint location, GLsizei count, GLfloat *value);
typedef void		(APIENTRYP PFNGLUNIFORM4FVARBPROC)			(GLint location, GLsizei count, GLfloat *value);
typedef void		(APIENTRYP PFNGLUNIFORM1IVARBPROC)			(GLint location, GLsizei count, GLint *value);
typedef void		(APIENTRYP PFNGLUNIFORM2IVARBPROC)			(GLint location, GLsizei count, GLint *value);
typedef void		(APIENTRYP PFNGLUNIFORM3IVARBPROC)			(GLint location, GLsizei count, GLint *value);
typedef void		(APIENTRYP PFNGLUNIFORM4IVARBPROC)			(GLint location, GLsizei count, GLint *value);
typedef void		(APIENTRYP PFNGLUNIFORMMATRIX2FVARBPROC)	(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void		(APIENTRYP PFNGLUNIFORMMATRIX3FVARBPROC)	(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void		(APIENTRYP PFNGLUNIFORMMATRIX4FVARBPROC)	(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void        (APIENTRYP PFNGLGETOBJECTPARAMETERFVARBPROC) (GLhandleARB obj, GLenum pname, GLfloat *params);
typedef void        (APIENTRYP PFNGLGETOBJECTPARAMETERIVARBPROC) (GLhandleARB obj, GLenum pname, GLint *params);
typedef void		(APIENTRYP PFNGLGETINFOLOGARBPROC)			(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
typedef void		(APIENTRYP PFNGLGETATTACHEDOBJECTSARB)		(GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj);
typedef GLint		(APIENTRYP PFNGLGETUNIFORMLOCATIONARBPROC)	(GLhandleARB programObj, const GLcharARB *name);
typedef void		(APIENTRYP PFNGLGETACTIVEUNIFORMARBPROC)	(GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLsizei *size, GLenum *type, GLcharARB *name);
typedef void		(APIENTRYP PFNGLGETUNIFORMFVARBPROC)		(GLhandleARB programObj, GLint location, GLfloat *parms);
typedef void		(APIENTRYP PFNGLGETUNIFORMIVARBPROC)		(GLhandleARB programObj, GLint location, GLint *parms);
typedef void		(APIENTRYP PFNGLGETSHADERSOURCEARBPROC)		(GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source);
#endif // GL_ARB_shader_objects

#ifndef GL_ARB_vertex_shader
#define GL_VERTEX_SHADER_ARB						0x8B31
#define GL_MAX_VERTEX_UNIFORM_COMPONENTS_ARB		0x8B4A
#define GL_MAX_VARYING_FLOATS_ARB					0x8B4B
#define GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS_ARB		0x8B4C
#define GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS_ARB		0x8B4D
#define GL_OBJECT_ACTIVE_ATTRIBUTES_ARB				0x8B89
#define GL_OBJECT_ACTIVE_ATTRIBUTE_MAX_LENGTH_ARB	0x8B8A
#endif

#ifndef GL_ARB_fragment_shader
#define GL_FRAGMENT_SHADER_ARB						0x8B30
#define GL_MAX_FRAGMENT_UNIFORM_COMPONENTS_ARB		0x8B49
#endif





#ifndef GL_SGIS_generate_mipmap
#define GL_SGIS_generate_mipmap 1

#define GL_GENERATE_MIPMAP_SGIS           0x8191
#define GL_GENERATE_MIPMAP_HINT_SGIS      0x8192
#endif


#ifndef GL_EXT_compiled_vertex_array
#define GL_ARRAY_ELEMENT_LOCK_FIRST_EXT   0x81A8
#define GL_ARRAY_ELEMENT_LOCK_COUNT_EXT   0x81A9

#define GL_EXT_compiled_vertex_array 1
#ifdef GL_GLEXT_PROTOTYPES
extern void APIENTRY glLockArraysEXT (GLint, GLsizei);
extern void APIENTRY glUnlockArraysEXT (void);
#endif /* GL_GLEXT_PROTOTYPES */
typedef void (APIENTRY * PFNGLLOCKARRAYSEXTPROC) (GLint first, GLsizei count);
typedef void (APIENTRY * PFNGLUNLOCKARRAYSEXTPROC) (void);
#endif

#endif
