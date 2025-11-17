#include "videoEnums.h"
#include "..\..\types\string.h"

#include "..\..\serialisation\serialiser.h"

using namespace System;

//

SIMPLE_SERIALISER_AS_INT32(::System::TextureWrap::Type);
SIMPLE_SERIALISER_AS_INT32(::System::TextureFiltering::Type);
SIMPLE_SERIALISER_AS_INT32(::System::DataFormatValueType::Type);
SIMPLE_SERIALISER_AS_INT32(::System::VertexFormatAttribType::Type);

//

TextureWrap::Type TextureWrap::parse(String const & _string)
{
	if (_string == TXT("repeat")) { return TextureWrap::repeat; }
	if (_string == TXT("clamp")) { return TextureWrap::clamp; }

	an_assert(false, TXT("texture wrap \"%S\" not recognised"), _string.to_char());
	return TextureWrap::repeat;
}

//

TextureFiltering::Type TextureFiltering::parse(String const & _string)
{
	TextureFiltering::Type result = TextureFiltering::nearest;
	if (parse(_string, REF_ result))
	{
		return result;
	}

	an_assert(false, TXT("texture filtering \"%S\" not recognised"), _string.to_char());
	return TextureFiltering::nearest;
}

bool TextureFiltering::parse(String const & _string, REF_ TextureFiltering::Type & _filtering)
{
	if (_string == TXT("nearest")) { _filtering = TextureFiltering::nearest; return true; }
	if (_string == TXT("linear")) { _filtering = TextureFiltering::linear; return true; }
	if (_string == TXT("linearMipMapLinear")) { _filtering = TextureFiltering::linearMipMapLinear; return true; }

	return false;
}

TextureFiltering::Type TextureFiltering::get_sane_mag(TextureFiltering::Type _type)
{
	if (_type == TextureFiltering::linearMipMapLinear)
	{
		return TextureFiltering::linear;
	}
	return _type;
}

TextureFiltering::Type TextureFiltering::get_sane_min(TextureFiltering::Type _type)
{
	return _type;
}

//

bool BlendOp::parse(String const & _string, REF_ BlendOp::Type & _blendOp)
{
	if (_string == TXT("none")) { _blendOp = BlendOp::None; return true; }
	if (_string == TXT("zero")) { _blendOp = BlendOp::Zero; return true; }
	if (_string == TXT("one")) { _blendOp = BlendOp::One; return true; }
	if (_string == TXT("srcAlpha")) { _blendOp = BlendOp::SrcAlpha; return true; }
	if (_string == TXT("oneMinusSrcAlpha")) { _blendOp = BlendOp::OneMinusSrcAlpha; return true; }

	return _string.is_empty();
}

//

DataFormatValueType::Type DataFormatValueType::parse(String const & _value, Type _default)
{
	if (_value == TXT("float")) return Float;
	if (_value == TXT("uint8") || _value == TXT("int8")) return Uint8;
	if (_value == TXT("uint16") || _value == TXT("int16")) return Uint16;
	if (_value == TXT("uint32") || _value == TXT("int32")) return Uint32;
	if (_value == TXT("p1010102")) return Packed_10_10_10_2;
	if (_value == TXT("pu1010102")) return Packed_Unsigned_10_10_10_2;
	error(TXT("value \"%S\" not recognised"), _value.to_char());
	return _default;
}

bool DataFormatValueType::is_integer_value(DataFormatValueType::Type _type)
{
	return _type == DataFormatValueType::Uint8 ||
		   _type == DataFormatValueType::Uint16 ||
		   _type == DataFormatValueType::Uint32 ||
		   _type == DataFormatValueType::Packed_10_10_10_2 ||
		   _type == DataFormatValueType::Packed_Unsigned_10_10_10_2;
}

bool DataFormatValueType::is_normalised_data(DataFormatValueType::Type _type)
{
	return is_integer_value(_type);
}

uint32 DataFormatValueType::get_size(DataFormatValueType::Type _type)
{
	if (_type == DataFormatValueType::Uint8)  { return 1; }
	else
	if (_type == DataFormatValueType::Uint16) { return 2; }
	else
	if (_type == DataFormatValueType::Uint32 ||
		_type == DataFormatValueType::Float ||
		_type == DataFormatValueType::Packed_10_10_10_2 || 
		_type == DataFormatValueType::Packed_Unsigned_10_10_10_2) {
		return 4;
	}
	else
	{
		an_assert(false);
		return 0;
	}
}

void DataFormatValueType::fill_with_uint(DataFormatValueType::Type _type, uint _value, void* _at)
{
	if (_type == DataFormatValueType::Uint8)  { *(uint8*)_at = (uint8)_value; } else
	if (_type == DataFormatValueType::Uint16) { *(uint16*)_at = (uint16)_value; } else
	if (_type == DataFormatValueType::Uint32) { *(uint32*)_at = (uint32)_value; } else
	{ an_assert(false); }
}

uint DataFormatValueType::get_uint_from(DataFormatValueType::Type _type, void const * _at)
{
	if (_type == DataFormatValueType::Uint8)  { return *(uint8*)_at; }
	if (_type == DataFormatValueType::Uint16) { return *(uint16*)_at; }
	if (_type == DataFormatValueType::Uint32) { return *(uint32*)_at; }
	an_assert(false);
	return 0;
}

//

VertexFormatAttribType::Type VertexFormatAttribType::parse(String const& _value, Type _default)
{
	if (_value == TXT("location")) return Location;
	if (_value == TXT("vector")) return Vector;
	if (_value == TXT("float")) return Float;
	if (_value == TXT("integer")) return Integer;
	if (_value == TXT("skinningIndex")) return SkinningIndex;
	if (_value == TXT("skinningWeight")) return SkinningWeight;
	error(TXT("value \"%S\" not recognised"), _value.to_char());
	return _default;
}

bool VertexFormatAttribType::should_be_mapped_as_integer(Type _type)
{
	return _type == Integer ||
			_type == SkinningIndex;
}

