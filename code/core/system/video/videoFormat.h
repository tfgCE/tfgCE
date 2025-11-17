#pragma once

#include "video.h"
#include "videoGL.h"

#include "..\..\serialisation\serialiserBasics.h"

namespace System
{
	namespace BaseVideoFormat	// gl: format
	{
		enum Type
		{
			None			=	0,
			//
			r				=	GL_RED,
			rg				=	GL_RG,
			rgb				=	GL_RGB,
#ifdef AN_GL
			bgr				=	GL_BGR,
#endif
			rgba			=	GL_RGBA,
#ifdef AN_GL
			bgra			=	GL_BGRA,
#endif
			r_integer		=	GL_RED_INTEGER,
			rg_integer		=	GL_RG_INTEGER,
			rgb_integer		=	GL_RGB_INTEGER,
#ifdef AN_GL
			bgr_integer		=	GL_BGR_INTEGER,
#endif
			rgba_integer	=	GL_RGBA_INTEGER,
#ifdef AN_GL
			bgra_integer	=	GL_BGRA_INTEGER,
			StencilIndex	=	GL_STENCIL_INDEX,
#endif
			DepthComponent	=	GL_DEPTH_COMPONENT,
			DepthStencil	=	GL_DEPTH_STENCIL,
		};

		Type parse(String const & _string);
	};

	namespace DepthStencilFormat
	{
		// this is internal format
		enum Type
		{
			None			=	0,
			//
			d24				=	GL_DEPTH_COMPONENT24,
			d32				=	GL_DEPTH_COMPONENT32F,
			d24s8			=	GL_DEPTH24_STENCIL8,
			d32s8			=	GL_DEPTH32F_STENCIL8, // I only added it to have a comment: "don't use it!"
			defaultFormat   =	d24s8
		};

		inline bool has_depth(DepthStencilFormat::Type _format)
		{
			return _format != None;
		}

		inline bool has_stencil(DepthStencilFormat::Type _format)
		{
			return _format == d24s8;
		}
	};

	namespace VideoFormat	// gl: internal format
	{
		enum Type
		{
			None								=	0,
			//
			DepthComponent						=	GL_DEPTH_COMPONENT,
			DepthStencil						=	GL_DEPTH_STENCIL,
			//
			r									=	GL_RED,
			rg									=	GL_RG,
			rgb									=	GL_RGB,
			rgba								=	GL_RGBA,
			//
			r8									=	GL_R8,
			r8_snorm							=	GL_R8_SNORM,
#ifdef AN_GL
			r16									=	GL_R16,
			r16_snorm							=	GL_R16_SNORM,
#endif
			rg8									=	GL_RG8,
			rg8_snorm							=	GL_RG8_SNORM,
#ifdef AN_GL
			rg16								=	GL_RG16,
			rg16_snorm							=	GL_RG16_SNORM,
			r3_g3_b2							=	GL_R3_G3_B2,
			rgb4								=	GL_RGB4,
			rgb5								=	GL_RGB5,
#endif
			rgb8								=	GL_RGB8,
			rgb8_snorm							=	GL_RGB8_SNORM,
#ifdef AN_GL
			rgb10								=	GL_RGB10,
			rgb12								=	GL_RGB12,
			rgb16_snorm							=	GL_RGB16_SNORM,
			rgba2								=	GL_RGBA2,
#endif
			rgba4								=	GL_RGBA4,
			rgb5_a1								=	GL_RGB5_A1,
			rgba8								=	GL_RGBA8,
			rgba8_snorm							=	GL_RGBA8_SNORM,
			rgb10_a2							=	GL_RGB10_A2,
			rgb10_a2ui							=	GL_RGB10_A2UI,
#ifdef AN_GL
			rgba12								=	GL_RGBA12,
			rgba16								=	GL_RGBA16,
#endif
			srgb8								=	GL_SRGB8,
			srgb8_alpha8						=	GL_SRGB8_ALPHA8,
			r16f								=	GL_R16F,
			rg16f								=	GL_RG16F,
			rgb16f								=	GL_RGB16F,
			rgba16f								=	GL_RGBA16F,
			r32f								=	GL_R32F,
			rg32f								=	GL_RG32F,
			rgb32f								=	GL_RGB32F,
			rgba32f								=	GL_RGBA32F,
			r11f_g11f_b10f						=	GL_R11F_G11F_B10F,
			rgb9_e5								=	GL_RGB9_E5,
			r8i									=	GL_R8I,
			r8ui								=	GL_R8UI,
			r16i								=	GL_R16I,
			r16ui								=	GL_R16UI,
			r32i								=	GL_R32I,
			r32ui	 							=	GL_R32UI,
			rg8i								=	GL_RG8I,
			rg8ui								=	GL_RG8UI,
			rg16i								=	GL_RG16I,
			rg16ui								=	GL_RG16UI,
			rg32i								=	GL_RG32I,
			rg32ui								=	GL_RG32UI,
			rgb8i								=	GL_RGB8I,
			rgb8ui								=	GL_RGB8UI,
			rgb16i								=	GL_RGB16I,
			rgb16ui								=	GL_RGB16UI,
			rgb32i								=	GL_RGB32I,
			rgb32ui								=	GL_RGB32UI,
			rgba8i								=	GL_RGBA8I,
			rgba8ui								=	GL_RGBA8UI,
			rgba16i								=	GL_RGBA16I,
			rgba16ui							=	GL_RGBA16UI,
			rgba32i								=	GL_RGBA32I,
			rgba32ui							=	GL_RGBA32UI,
			//
#ifdef AN_GL
			Compressed_r						=	GL_COMPRESSED_RED,
			Compressed_rg						=	GL_COMPRESSED_RG,
			Compressed_rgb						=	GL_COMPRESSED_RGB,
			Compressed_rgba						=	GL_COMPRESSED_RGBA,
			Compressed_srgb						=	GL_COMPRESSED_SRGB,
			Compressed_srgb_alpha				=	GL_COMPRESSED_SRGB_ALPHA,
			Compressed_r_rgtc1					=	GL_COMPRESSED_RED_RGTC1,
			Compressed_signed_r_rgtc1			=	GL_COMPRESSED_SIGNED_RED_RGTC1,
			Compressed_rg_rgtc2					=	GL_COMPRESSED_RG_RGTC2,
			Compressed_signed_rg_rgtc2			=	GL_COMPRESSED_SIGNED_RG_RGTC2,
			//Compressed_rgba_bptc_unorm		=	GL_COMPRESSED_RGBA_BPTC_UNORM,
			//Compressed_srgb_alpha_bptc_unorm	=	GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM,
			//Compressed_rgb_bptc_signed_float	=	GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT,
			//Compressed_rgb_bptc_unsigned_float=	GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT,
#endif
		};
		BaseVideoFormat::Type to_base_video_format(VideoFormat::Type _vf);
		int get_pixel_size(VideoFormat::Type _vf);
		void encode(VideoFormat::Type _vf, Colour const & _colour, void* _at);
		void decode(VideoFormat::Type _vf, OUT_ Colour & _colour, void const * _at);
		
		bool does_require_conversion(VideoFormat::Type _vf);
		void convert(VideoFormat::Type _vf, void * _dst, void const* _src, int _size);

		Type parse(String const & _string);

		bool is_srgb(VideoFormat::Type _vf);
	};

	namespace VideoFormatData
	{
		enum Type
		{
			None						=	0,
			//
			UnsignedByte				=	GL_UNSIGNED_BYTE,
			Byte						=	GL_BYTE,
			UnsignedShort				=	GL_UNSIGNED_SHORT,
			Short						=	GL_SHORT,
			UnsignedInt					=	GL_UNSIGNED_INT,
			Int							=	GL_INT,
			Float						=	GL_FLOAT,
#ifdef AN_GL
			UnsignedByte_3_3_2			=	GL_UNSIGNED_BYTE_3_3_2,
			UnsignedByte_2_3_3_rev		=	GL_UNSIGNED_BYTE_2_3_3_REV,
#endif
			UnsignedShort_5_6_5			=	GL_UNSIGNED_SHORT_5_6_5,
#ifdef AN_GL
			UnsignedShort_5_6_5_rev		=	GL_UNSIGNED_SHORT_5_6_5_REV,
#endif
			UnsignedShort_4_4_4_4		=	GL_UNSIGNED_SHORT_4_4_4_4,
#ifdef AN_GL
			UnsignedShort_4_4_4_4_rev	=	GL_UNSIGNED_SHORT_4_4_4_4_REV,
#endif
			UnsignedShort_5_5_5_1		=	GL_UNSIGNED_SHORT_5_5_5_1,
#ifdef AN_GL
			UnsignedShort_1_5_5_5_rev	=	GL_UNSIGNED_SHORT_1_5_5_5_REV,
			UnsignedInt_8_8_8_8			=	GL_UNSIGNED_INT_8_8_8_8,
			UnsignedInt_8_8_8_8_rev		=	GL_UNSIGNED_INT_8_8_8_8_REV,
			UnsignedInt_10_10_10_2		=	GL_UNSIGNED_INT_10_10_10_2,
#endif
			UnsignedInt_2_10_10_10_rev	=	GL_UNSIGNED_INT_2_10_10_10_REV,
		};

		Type parse(String const & _string);

		Type from(VideoFormat::Type _vf);
	};

};

DECLARE_SERIALISER_FOR(::System::BaseVideoFormat::Type);
DECLARE_SERIALISER_FOR(::System::VideoFormat::Type);
DECLARE_SERIALISER_FOR(::System::VideoFormatData::Type);
