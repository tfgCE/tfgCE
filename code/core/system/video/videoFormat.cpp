#include "videoFormat.h"

#include "..\..\other\typeConversions.h"
#include "..\..\types\colour.h"
#include "..\..\types\vectorPacked.h"

#include "..\..\serialisation\serialiser.h"

using namespace System;

//

SIMPLE_SERIALISER_AS_INT32(::System::BaseVideoFormat::Type);
SIMPLE_SERIALISER_AS_INT32(::System::VideoFormat::Type);
SIMPLE_SERIALISER_AS_INT32(::System::VideoFormatData::Type);

//

BaseVideoFormat::Type BaseVideoFormat::parse(String const & _string)
{
	if (_string == TXT("r"))	{ return BaseVideoFormat::r; }
	if (_string == TXT("rg"))	{ return BaseVideoFormat::rg; }
	if (_string == TXT("rgb"))	{ return BaseVideoFormat::rgb; }
	if (_string == TXT("rgba"))	{ return BaseVideoFormat::rgba; }

	an_assert(_string.is_empty(), TXT("base video format not recognised"));
	return BaseVideoFormat::None;
}

//

VideoFormatData::Type VideoFormatData::parse(String const & _string)
{
	if (_string == TXT("unsignedByte"))		{ return VideoFormatData::UnsignedByte; }
	if (_string == TXT("byte"))				{ return VideoFormatData::Byte; }
	if (_string == TXT("unsignedShort"))	{ return VideoFormatData::UnsignedShort; }
	if (_string == TXT("short"))			{ return VideoFormatData::Short; }
	if (_string == TXT("unsignedInt"))		{ return VideoFormatData::UnsignedInt; }
	if (_string == TXT("int"))				{ return VideoFormatData::Int; }
	if (_string == TXT("float"))			{ return VideoFormatData::Float; }

	an_assert(_string.is_empty(), TXT("video format data not recognised"));
	return VideoFormatData::None;
}

VideoFormatData::Type VideoFormatData::from(VideoFormat::Type _vf)
{
	switch (_vf)
	{
	case VideoFormat::r										:	return VideoFormatData::UnsignedByte;
	case VideoFormat::rg									:	return VideoFormatData::UnsignedByte;
	case VideoFormat::rgb									:	return VideoFormatData::UnsignedByte;
	case VideoFormat::rgba									:	return VideoFormatData::UnsignedByte;
	//
	case VideoFormat::r8									:	return VideoFormatData::UnsignedByte;
#ifdef AN_GL
	case VideoFormat::r16									:	return VideoFormatData::UnsignedShort;
#endif
	case VideoFormat::rg8									:	return VideoFormatData::UnsignedByte;
#ifdef AN_GL
	case VideoFormat::rg16									:	return VideoFormatData::UnsignedShort;
#endif
	case VideoFormat::rgb8									:	return VideoFormatData::UnsignedByte;
#ifdef AN_GL
	case VideoFormat::rgb10									:	return VideoFormatData::UnsignedInt_10_10_10_2;
#endif
	case VideoFormat::rgba8									:	return VideoFormatData::UnsignedByte;
	case VideoFormat::srgb8_alpha8							:	return VideoFormatData::UnsignedByte;
#ifdef AN_GL
	case VideoFormat::rgb10_a2								:	return VideoFormatData::UnsignedInt_10_10_10_2;
	case VideoFormat::rgb10_a2ui							:	return VideoFormatData::UnsignedInt_10_10_10_2;
#endif
#ifdef AN_GLES
	case VideoFormat::rgb10_a2								:	return VideoFormatData::UnsignedInt_2_10_10_10_rev;
	case VideoFormat::rgb10_a2ui							:	return VideoFormatData::UnsignedInt_2_10_10_10_rev;
#endif
	case VideoFormat::r16f									:	return VideoFormatData::Float;
	case VideoFormat::rg16f									:	return VideoFormatData::Float;
	case VideoFormat::rgb16f								:	return VideoFormatData::Float;
	case VideoFormat::rgba16f								:	return VideoFormatData::Float;
	case VideoFormat::r32f									:	return VideoFormatData::Float;
	case VideoFormat::rg32f									:	return VideoFormatData::Float;
	case VideoFormat::rgb32f								:	return VideoFormatData::Float;
	case VideoFormat::rgba32f								:	return VideoFormatData::Float;
	//
	default: an_assert(false, TXT("Video format not recognised"));
	}
	return VideoFormatData::None;
}

//

VideoFormat::Type VideoFormat::parse(String const & _string)
{
	if (_string == TXT("r"))			{ return VideoFormat::r; }
	if (_string == TXT("r8"))			{ return VideoFormat::r8; }
	if (_string == TXT("rg"))			{ return VideoFormat::rg; }
	if (_string == TXT("rg8"))			{ return VideoFormat::rg8; }
	if (_string == TXT("rgb"))			{ return VideoFormat::rgb; }
	if (_string == TXT("rgba"))			{ return VideoFormat::rgba; }
	if (_string == TXT("rgb8"))			{ return VideoFormat::rgb8; }
	if (_string == TXT("rgb10a2"))		{ return VideoFormat::rgb10_a2; }
	if (_string == TXT("rgba8"))		{ return VideoFormat::rgba8; }
	if (_string == TXT("rgb16f"))		{ return VideoFormat::rgb16f; }
	if (_string == TXT("rgba16f"))		{ return VideoFormat::rgba16f; }
	if (_string == TXT("rgb32f"))		{ return VideoFormat::rgb32f; }
	if (_string == TXT("rgba32f"))		{ return VideoFormat::rgba32f; }
	if (_string == TXT("r11g11b10f"))	{ return VideoFormat::r11f_g11f_b10f; }
	if (_string == TXT("srgb8_alpha8"))	{ return VideoFormat::srgb8_alpha8; }

	an_assert(_string.is_empty(), TXT("video format not recognised"));
	return VideoFormat::None;
}

BaseVideoFormat::Type VideoFormat::to_base_video_format(VideoFormat::Type _vf)
{
	switch (_vf)
	{
	case VideoFormat::DepthComponent						:	return BaseVideoFormat::DepthComponent;
	case VideoFormat::DepthStencil							:	return BaseVideoFormat::DepthStencil;
	case VideoFormat::r										:	return BaseVideoFormat::r;
	case VideoFormat::rg									:	return BaseVideoFormat::rg;
	case VideoFormat::rgb									:	return BaseVideoFormat::rgb;
	case VideoFormat::rgba									:	return BaseVideoFormat::rgba;
	//
	case VideoFormat::r8									:	return BaseVideoFormat::r;
	case VideoFormat::r8_snorm								:	return BaseVideoFormat::r;
#ifdef AN_GL
	case VideoFormat::r16									:	return BaseVideoFormat::r;
	case VideoFormat::r16_snorm								:	return BaseVideoFormat::r;
#endif
	case VideoFormat::rg8									:	return BaseVideoFormat::rg;
	case VideoFormat::rg8_snorm								:	return BaseVideoFormat::rg;
#ifdef AN_GL
	case VideoFormat::rg16									:	return BaseVideoFormat::rg;
	case VideoFormat::rg16_snorm							:	return BaseVideoFormat::rg;
	case VideoFormat::r3_g3_b2								:	return BaseVideoFormat::rgb;
	case VideoFormat::rgb4									:	return BaseVideoFormat::rgb;
	case VideoFormat::rgb5									:	return BaseVideoFormat::rgb;
#endif
	case VideoFormat::rgb8									:	return BaseVideoFormat::rgb;
	case VideoFormat::rgb8_snorm							:	return BaseVideoFormat::rgb;
#ifdef AN_GL
	case VideoFormat::rgb10									:	return BaseVideoFormat::rgb;
	case VideoFormat::rgb12									:	return BaseVideoFormat::rgb;
	case VideoFormat::rgb16_snorm							:	return BaseVideoFormat::rgb;
	case VideoFormat::rgba2									:	return BaseVideoFormat::rgb;
#endif
	case VideoFormat::rgba4									:	return BaseVideoFormat::rgb;
	case VideoFormat::rgb5_a1								:	return BaseVideoFormat::rgba;
	case VideoFormat::rgba8									:	return BaseVideoFormat::rgba;
	case VideoFormat::rgba8_snorm							:	return BaseVideoFormat::rgba;
	case VideoFormat::rgb10_a2								:	return BaseVideoFormat::rgba;
	case VideoFormat::rgb10_a2ui							:	return BaseVideoFormat::rgba;
#ifdef AN_GL
	case VideoFormat::rgba12								:	return BaseVideoFormat::rgba;
	case VideoFormat::rgba16								:	return BaseVideoFormat::rgba;
#endif
	case VideoFormat::srgb8									:	return BaseVideoFormat::rgb;
	case VideoFormat::srgb8_alpha8							:	return BaseVideoFormat::rgba;
	case VideoFormat::r16f									:	return BaseVideoFormat::r;
	case VideoFormat::rg16f									:	return BaseVideoFormat::rg;
	case VideoFormat::rgb16f								:	return BaseVideoFormat::rgb;
	case VideoFormat::rgba16f								:	return BaseVideoFormat::rgba;
	case VideoFormat::r32f									:	return BaseVideoFormat::r;
	case VideoFormat::rg32f									:	return BaseVideoFormat::rg;
	case VideoFormat::rgb32f								:	return BaseVideoFormat::rgb;
	case VideoFormat::rgba32f								:	return BaseVideoFormat::rgba;
	case VideoFormat::r11f_g11f_b10f						:	return BaseVideoFormat::rgb;
	case VideoFormat::rgb9_e5								:	return BaseVideoFormat::rgb;
	case VideoFormat::r8i									:	return BaseVideoFormat::r;
	case VideoFormat::r8ui									:	return BaseVideoFormat::r;
	case VideoFormat::r16i									:	return BaseVideoFormat::r;
	case VideoFormat::r16ui									:	return BaseVideoFormat::r;
	case VideoFormat::r32i									:	return BaseVideoFormat::r;
	case VideoFormat::r32ui	 								:	return BaseVideoFormat::r;
	case VideoFormat::rg8i									:	return BaseVideoFormat::rg;
	case VideoFormat::rg8ui									:	return BaseVideoFormat::rg;
	case VideoFormat::rg16i									:	return BaseVideoFormat::rg;
	case VideoFormat::rg16ui								:	return BaseVideoFormat::rg;
	case VideoFormat::rg32i									:	return BaseVideoFormat::rg;
	case VideoFormat::rg32ui								:	return BaseVideoFormat::rg;
	case VideoFormat::rgb8i									:	return BaseVideoFormat::rgb;
	case VideoFormat::rgb8ui								:	return BaseVideoFormat::rgb;
	case VideoFormat::rgb16i								:	return BaseVideoFormat::rgb;
	case VideoFormat::rgb16ui								:	return BaseVideoFormat::rgb;
	case VideoFormat::rgb32i								:	return BaseVideoFormat::rgb;
	case VideoFormat::rgb32ui								:	return BaseVideoFormat::rgb;
	case VideoFormat::rgba8i								:	return BaseVideoFormat::rgba;
	case VideoFormat::rgba8ui								:	return BaseVideoFormat::rgba;
	case VideoFormat::rgba16i								:	return BaseVideoFormat::rgba;
	case VideoFormat::rgba16ui								:	return BaseVideoFormat::rgba;
	case VideoFormat::rgba32i								:	return BaseVideoFormat::rgba;
	case VideoFormat::rgba32ui								:	return BaseVideoFormat::rgba;
	//
#ifdef AN_GL
	case VideoFormat::Compressed_r							:	return BaseVideoFormat::r;
	case VideoFormat::Compressed_rg							:	return BaseVideoFormat::rg;
	case VideoFormat::Compressed_rgb						:	return BaseVideoFormat::rgb;
	case VideoFormat::Compressed_rgba						:	return BaseVideoFormat::rgba;
	case VideoFormat::Compressed_srgb						:	return BaseVideoFormat::rgb;
	case VideoFormat::Compressed_srgb_alpha					:	return BaseVideoFormat::rgba;
	case VideoFormat::Compressed_r_rgtc1					:	return BaseVideoFormat::r;
	case VideoFormat::Compressed_signed_r_rgtc1				:	return BaseVideoFormat::r;
	case VideoFormat::Compressed_rg_rgtc2					:	return BaseVideoFormat::rg;
	case VideoFormat::Compressed_signed_rg_rgtc2			:	return BaseVideoFormat::rg;
	//case VideoFormat::Compressed_rgba_bptc_unorm			:	return BaseVideoFormat::rgba;
	//case VideoFormat::Compressed_srgb_alpha_bptc_unorm	:	return BaseVideoFormat::rgba;
	//case VideoFormat::Compressed_rgb_bptc_signed_float	:	return BaseVideoFormat::rgb;
	//case VideoFormat::Compressed_rgb_bptc_unsigned_float	:	return BaseVideoFormat::rgb;
#endif
	default: an_assert(false, TXT("Video format not recognised"));
	}
	return BaseVideoFormat::None;
}

int VideoFormat::get_pixel_size(VideoFormat::Type _vf)
{
	switch (_vf)
	{
	case VideoFormat::r8									:	return 1 * 1;
	case VideoFormat::r8_snorm								:	return 1 * 1;
#ifdef AN_GL
	case VideoFormat::r16									:	return 1 * 2;
	case VideoFormat::r16_snorm								:	return 1 * 2;
#endif
	case VideoFormat::rg8									:	return 2 * 1;
	case VideoFormat::rg8_snorm								:	return 2 * 1;
#ifdef AN_GL
	case VideoFormat::rg16									:	return 2 * 2;
	case VideoFormat::rg16_snorm							:	return 2 * 2;
	case VideoFormat::r3_g3_b2								:	return 1;
#endif
	case VideoFormat::rgb8									:	return 3 * 1;
	case VideoFormat::rgb8_snorm							:	return 3 * 1;
#ifdef AN_GL
	case VideoFormat::rgb16_snorm							:	return 3 * 2;
#endif
	case VideoFormat::rgba8									:	return 4 * 1;
	case VideoFormat::rgba8_snorm							:	return 4 * 1;
	case VideoFormat::srgb8_alpha8							:	return 4 * 1;
	case VideoFormat::rgb10_a2								:	return 4 * 1;
	case VideoFormat::r16f									:	return 1 * 2;
	case VideoFormat::rg16f									:	return 2 * 2;
	case VideoFormat::rgb16f								:	return 3 * 2;
	case VideoFormat::rgba16f								:	return 4 * 2;
	case VideoFormat::r32f									:	return 1 * 4;
	case VideoFormat::rg32f									:	return 2 * 4;
	case VideoFormat::rgb32f								:	return 3 * 4;
	case VideoFormat::rgba32f								:	return 4 * 4;
	case VideoFormat::r8i									:	return 1 * 1;
	case VideoFormat::r8ui									:	return 1 * 1;
	case VideoFormat::r16i									:	return 1 * 2;
	case VideoFormat::r16ui									:	return 1 * 2;
	case VideoFormat::r32i									:	return 1 * 4;
	case VideoFormat::r32ui	 								:	return 1 * 4;
	case VideoFormat::rg8i									:	return 2 * 1;
	case VideoFormat::rg8ui									:	return 2 * 1;
	case VideoFormat::rg16i									:	return 2 * 2;
	case VideoFormat::rg16ui								:	return 2 * 2;
	case VideoFormat::rg32i									:	return 2 * 4;
	case VideoFormat::rg32ui								:	return 2 * 4;
	case VideoFormat::rgb8i									:	return 3 * 1;
	case VideoFormat::rgb8ui								:	return 3 * 1;
	case VideoFormat::rgb16i								:	return 3 * 2;
	case VideoFormat::rgb16ui								:	return 3 * 2;
	case VideoFormat::rgb32i								:	return 3 * 4;
	case VideoFormat::rgb32ui								:	return 3 * 4;
	case VideoFormat::rgba8i								:	return 4 * 1;
	case VideoFormat::rgba8ui								:	return 4 * 1;
	case VideoFormat::rgba16i								:	return 4 * 2;
	case VideoFormat::rgba16ui								:	return 4 * 2;
	case VideoFormat::rgba32i								:	return 4 * 4;
	case VideoFormat::rgba32ui								:	return 4 * 4;
	default: an_assert(false, TXT("not supported"));
	}
	return 0;
}

bool VideoFormat::is_srgb(VideoFormat::Type _vf)
{
	switch (_vf)
	{
	case VideoFormat::srgb8									:	return true;
	case VideoFormat::srgb8_alpha8							:	return true;

	default: break;
	}
	return false;
}

void VideoFormat::encode(VideoFormat::Type _vf, Colour const & _colour, void* _at)
{
	todo_future(TXT("actually we should support different modes here too"));
	int8* at = (int8*)_at;
	switch (_vf)
	{
	case VideoFormat::r8									:	*((uint8*)at) = TypeConversions::VideoFormat::f_ui8(_colour.r);	break;
	case VideoFormat::rg8									:	*((uint8*)at) = TypeConversions::VideoFormat::f_ui8(_colour.r);	at += sizeof(uint8);
																*((uint8*)at) = TypeConversions::VideoFormat::f_ui8(_colour.g);	break;
	case VideoFormat::rgb8									:	*((uint8*)at) = TypeConversions::VideoFormat::f_ui8(_colour.r);	at += sizeof(uint8);
																*((uint8*)at) = TypeConversions::VideoFormat::f_ui8(_colour.g);	at += sizeof(uint8);
																*((uint8*)at) = TypeConversions::VideoFormat::f_ui8(_colour.b);	break;
	case VideoFormat::rgba8									:	*((uint8*)at) = TypeConversions::VideoFormat::f_ui8(_colour.r);	at += sizeof(uint8);
																*((uint8*)at) = TypeConversions::VideoFormat::f_ui8(_colour.g);	at += sizeof(uint8);
																*((uint8*)at) = TypeConversions::VideoFormat::f_ui8(_colour.b);	at += sizeof(uint8);
																*((uint8*)at) = TypeConversions::VideoFormat::f_ui8(_colour.a);	break;
	case VideoFormat::rgb10_a2								:  ((VectorPacked*)at)->set_as_video_format_colour(_colour); break;
	case VideoFormat::r32f									:	*((float*)at) = _colour.r;	break;
	case VideoFormat::rg32f									:	*((float*)at) = _colour.r;	at += sizeof(float);
																*((float*)at) = _colour.g;	break;
	case VideoFormat::rgb32f								:	*((float*)at) = _colour.r;	at += sizeof(float);
																*((float*)at) = _colour.g;	at += sizeof(float);
																*((float*)at) = _colour.b;	break;
	case VideoFormat::rgba32f								:	*((float*)at) = _colour.r;	at += sizeof(float);
																*((float*)at) = _colour.g;	at += sizeof(float);
																*((float*)at) = _colour.b;	at += sizeof(float);
																*((float*)at) = _colour.a;	break;
	default: an_assert(false, TXT("not supported"));
	}
}

void VideoFormat::decode(VideoFormat::Type _vf, OUT_ Colour & _colour, void const * _at)
{
	_colour.r = _colour.g = _colour.b = _colour.a = 1.0f;
	todo_future(TXT("actually we should support different modes here too"));
	int8 const * at = (int8*)_at;
	switch (_vf)
	{
	case VideoFormat::r8									:	_colour.r = _colour.g = _colour.b = TypeConversions::VideoFormat::ui8_f(*((uint8*)at)); break;
	case VideoFormat::rg8									:	_colour.r = TypeConversions::VideoFormat::ui8_f(*((uint8*)at));	at += sizeof(uint8);
																_colour.g = TypeConversions::VideoFormat::ui8_f(*((uint8*)at));	at += sizeof(uint8);
																_colour.b = _colour.r;								break;
	case VideoFormat::rgb8									:	_colour.r = TypeConversions::VideoFormat::ui8_f(*((uint8*)at));	at += sizeof(uint8);
																_colour.g = TypeConversions::VideoFormat::ui8_f(*((uint8*)at));	at += sizeof(uint8);
																_colour.b = TypeConversions::VideoFormat::ui8_f(*((uint8*)at));	break;
	case VideoFormat::rgba8									:	_colour.r = TypeConversions::VideoFormat::ui8_f(*((uint8*)at));	at += sizeof(uint8);
																_colour.g = TypeConversions::VideoFormat::ui8_f(*((uint8*)at));	at += sizeof(uint8);
																_colour.b = TypeConversions::VideoFormat::ui8_f(*((uint8*)at));	at += sizeof(uint8);
																_colour.a = TypeConversions::VideoFormat::ui8_f(*((uint8*)at));	break;
	case VideoFormat::rgb10_a2								:	_colour = ((VectorPacked*)at)->get_as_video_format_colour();		break;
	case VideoFormat::r32f									:	_colour.r = _colour.g = _colour.b = (*((float*)at)); break;
	case VideoFormat::rg32f									:	_colour.r = (*((float*)at));				at += sizeof(float);
																_colour.g = (*((float*)at));				at += sizeof(float);
																_colour.b = _colour.r;						break;
	case VideoFormat::rgb32f								:	_colour.r = (*((float*)at));				at += sizeof(float);
																_colour.g = (*((float*)at));				at += sizeof(float);
																_colour.b = (*((float*)at));				break;
	case VideoFormat::rgba32f								:	_colour.r = (*((float*)at));				at += sizeof(float);
																_colour.g = (*((float*)at));				at += sizeof(float);
																_colour.b = (*((float*)at));				at += sizeof(float);
																_colour.a = (*((float*)at));				break;
	default: an_assert(false, TXT("not supported"));
	}
}

bool VideoFormat::does_require_conversion(VideoFormat::Type _vf)
{
	// for GLES
	if (_vf == VideoFormat::rgb10_a2 ||
		_vf == VideoFormat::rgb10_a2ui)
	{
		bool rgb10_a2_requiresConversion = false;
#ifdef AN_GLES
		rgb10_a2_requiresConversion = true;
#endif
		return rgb10_a2_requiresConversion;
	}
	return false;
}

void VideoFormat::convert(VideoFormat::Type _vf, void* _dst, void const* _src, int _size)
{
	an_assert(does_require_conversion(_vf), TXT("always check if requires conversion!"));
	// for GLES
#ifdef AN_GLES
	if (_vf == VideoFormat::rgb10_a2 ||
		_vf == VideoFormat::rgb10_a2ui)
	{
		int8 const* srcStart = (int8 const*)_src;
		int8 const* src = srcStart;
		int8* dst = (int8*)_dst;
		while (src - srcStart < _size)
		{
			// change	bbbb-bbaa gggg-bbbb rrgg-gggg rrrr-rrrr
			//	into	rrrr-rrrr gggg-ggrr bbbb-gggg aabb-bbbb
			uint16 r = 0;
			uint16 g = 0;
			uint16 b = 0;
			uint8 a = 0;
			a |= (*(src) & 0x03);
			b |= ((uint16) * (src)) >> 2;				++src;
			b |= (((uint16) * (src)) & 0x0f) << 6;
			g |= ((uint16) * (src)) >> 4;				++src;
			g |= (((uint16) * (src)) & 0x3f) << 4;
			r |= ((uint16) * (src)) >> 6;				++src;
			r |= ((uint16) * (src)) << 2;				++src;
			//
			*(dst) = (uint8)(r);						++dst;
			*(dst) = (uint8)((g << 2) | (r >> 8));		++dst;
			*(dst) = (uint8)((b << 4) | (g >> 6));		++dst;
			*(dst) = (uint8)((b >> 4) | (a << 6));		++dst;
		}
	}
#endif
}
