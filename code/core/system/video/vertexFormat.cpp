#include "vertexFormat.h"

#include "vertexFormatUtils.h"

#include "shaderProgram.h"

#include "..\..\containers\arrayStack.h"
#include "..\..\mainSettings.h"
#include "..\..\mesh\skeleton.h"
#include "..\..\other\parserUtils.h"
#include "..\..\serialisation\serialiser.h"
#include "..\..\types\names.h"
#include "..\..\types\vectorPacked.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace System;

//

SIMPLE_SERIALISER_AS_INT32(::System::VertexFormatLocation::Type);
SIMPLE_SERIALISER_AS_INT32(::System::VertexFormatNormal::Type);
SIMPLE_SERIALISER_AS_INT32(::System::VertexFormatColour::Type);
SIMPLE_SERIALISER_AS_INT32(::System::VertexFormatTextureUV::Type);

//

// attribs
DEFINE_STATIC_NAME(inAPosition);
DEFINE_STATIC_NAME(inONormal);
DEFINE_STATIC_NAME(inOColour);
DEFINE_STATIC_NAME(inOUV);
DEFINE_STATIC_NAME(inOSkinningIndex);
DEFINE_STATIC_NAME(inOSkinningIndices);
DEFINE_STATIC_NAME(inOSkinningWeights);

//

VertexFormatCompare::Flag VertexFormatCompare::get_flag_for_attrib(Name const& _attrib)
{
	if (_attrib == NAME(inAPosition)) return Flag::Location;
	if (_attrib == NAME(inONormal)) return Flag::Normal;
	return Flag::NotDefined;
}

//

#define USE_OFFSET(_pointer, _offset) (void*)(((int8*)_pointer) + _offset)

#define ADD_ATTRIB_NAME(_name, _offset, _size, _dataType, _attribType) \
{ \
	int size = _size; \
	_offset = stride; \
	attribs.push_back(VertexFormatAttrib(_name, stride, size, _dataType, _attribType)); \
	stride += size; \
}

#define ADD_ATTRIB(_name, _offset, _size, _dataType, _attribType) \
{ \
	int size = _size; \
	_offset = stride; \
	attribs.push_back(VertexFormatAttrib(NAME(_name), stride, size, _dataType, _attribType)); \
	stride += size; \
}

#define NO_ATTRIB(_offset) \
	_offset = NONE;

VertexFormatAttrib::VertexFormatAttrib()
{

}

VertexFormatAttrib::VertexFormatAttrib(Name const & _name, int _offset, int _size, DataFormatValueType::Type _dataType, VertexFormatAttribType::Type _attribType)
: name(_name)
, offset(_offset)
, size(_size)
, dataType(_dataType)
, attribType(_attribType)
{
}

//

VertexFormatCustomData::VertexFormatCustomData(Name const & _name, DataFormatValueType::Type _dataType, VertexFormatAttribType::Type _attribType, int _count)
: name(_name)
, dataType(_dataType)
, count(_count)
, attribType(_attribType)
{
}

bool VertexFormatCustomData::serialise(Serialiser & _serialiser)
{
	serialise_data(_serialiser, name);
	serialise_data(_serialiser, dataType);
	serialise_data(_serialiser, attribType);
	serialise_data(_serialiser, count);
	return true;
}

//

VertexFormat::VertexFormat()
: okToUse(false)
, paddingAllowed(true)
, location(VertexFormatLocation::XYZ)
, normal(VertexFormatNormal::No)
, colour(VertexFormatColour::None)
, textureUV(VertexFormatTextureUV::None)
, textureUVType(MainSettings::global().get_vertex_data_uv_data_type())
, hasSkinningData(false)
, normalPacked(MainSettings::global().is_vertex_data_normal_packed())
{}

VertexFormat & VertexFormat::calculate_stride_and_offsets()
{
	if (okToUse)
	{
		return *this;
	}

	stride = 0;
	attribs.clear();

	// start with vertex, just location
	if (location == VertexFormatLocation::XY)
	{
		ADD_ATTRIB(inAPosition, locationOffset, 2 * sizeof(float), DataFormatValueType::Float, VertexFormatAttribType::Location);
	}
	else if (location == VertexFormatLocation::XYZ)
	{
		ADD_ATTRIB(inAPosition, locationOffset, 3 * sizeof(float), DataFormatValueType::Float, VertexFormatAttribType::Location);
	}
	else
	{
		error(TXT("there has to be location!"));
	}

	// add offsets
	if (normal == VertexFormatNormal::Yes)
	{
		if (is_normal_packed())
		{
			ADD_ATTRIB(inONormal, normalOffset, 1 * sizeof(int32), DataFormatValueType::Packed_10_10_10_2, VertexFormatAttribType::Vector);
		}
		else
		{
			ADD_ATTRIB(inONormal, normalOffset, 3 * sizeof(float), DataFormatValueType::Float, VertexFormatAttribType::Vector);
		}
	}
	else
	{
		NO_ATTRIB(normalOffset);
	}
	if (colour != VertexFormatColour::None)
	{
		ADD_ATTRIB(inOColour, colourOffset, colour * sizeof(float), DataFormatValueType::Float, VertexFormatAttribType::Float);
	}
	else
	{
		NO_ATTRIB(colourOffset);
	}
	if (textureUV != VertexFormatTextureUV::None)
	{
		ADD_ATTRIB(inOUV, textureUVOffset, textureUV * DataFormatValueType::get_size(textureUVType), textureUVType, VertexFormatAttribType::Float);
	}
	else
	{
		NO_ATTRIB(textureUVOffset);
	}
	if (hasSkinningData)
	{
		if (skinningElementCount == 1)
		{
			ADD_ATTRIB(inOSkinningIndex, skinningIndexOffset, DataFormatValueType::get_size(skinningIndexType), skinningIndexType, VertexFormatAttribType::SkinningIndex);
			NO_ATTRIB(skinningIndicesOffset);
			NO_ATTRIB(skinningWeightsOffset);
		}
		else
		{
			NO_ATTRIB(skinningIndexOffset);
			ADD_ATTRIB(inOSkinningIndices, skinningIndicesOffset, skinningElementCount * DataFormatValueType::get_size(skinningIndexType), skinningIndexType, VertexFormatAttribType::SkinningIndex);
			ADD_ATTRIB(inOSkinningWeights, skinningWeightsOffset, skinningElementCount * sizeof(float), DataFormatValueType::Float, VertexFormatAttribType::SkinningWeight);
		}
	}
	else
	{
		NO_ATTRIB(skinningIndexOffset);
		NO_ATTRIB(skinningIndicesOffset);
		NO_ATTRIB(skinningWeightsOffset);
	}
	customDataOffset.clear();
	customDataOffset.make_space_for(customData.get_size());
	for_every(cd, customData)
	{
		int offset;
		ADD_ATTRIB_NAME(cd->name, offset, cd->count * DataFormatValueType::get_size(cd->dataType), cd->dataType, cd->attribType);
		customDataOffset.push_back(offset);
	}

	if (paddingAllowed)
	{
		if (int vertexDataPadding = MainSettings::global().get_vertex_data_padding())
		{
			int overPad = stride % vertexDataPadding;
			if (overPad > 0)
			{
				int pad = vertexDataPadding - overPad;
				stride += pad;
			}
		}
	}

	okToUse = true;

	return *this;
}

#define VER_BASE 0
#define VER_NORMAL_PACKED 1
#define VER_TEXTURE_UV_TYPE 2
#define CURRENT_VERSION VER_TEXTURE_UV_TYPE

bool VertexFormat::serialise(Serialiser & _serialiser)
{
	uint8 version = CURRENT_VERSION;
	serialise_data(_serialiser, version);
	if (version > CURRENT_VERSION)
	{
		error(TXT("invalid vertex format version"));
		return false;
	}
	serialise_data(_serialiser, paddingAllowed);
	serialise_data(_serialiser, normal);
	if (version >= VER_NORMAL_PACKED)
	{
		serialise_data(_serialiser, normalPacked);
	}
	else
	{
		normalPacked = false;
	}
	serialise_data(_serialiser, colour);
	serialise_data(_serialiser, textureUV);
	if (version >= VER_TEXTURE_UV_TYPE)
	{
		serialise_data(_serialiser, textureUVType);
	}
	else
	{
		textureUVType = DataFormatValueType::Float;
	}
	serialise_data(_serialiser, hasSkinningData);
	serialise_data(_serialiser, skinningElementCount);
	serialise_data(_serialiser, skinningIndexType);
	serialise_data(_serialiser, customData);
	if (_serialiser.is_reading())
	{
		calculate_stride_and_offsets();
	}
	return true;
}

void VertexFormat::bind_vertex_attrib(Video3D* _v3d, ShaderProgram * _shaderProgram, void const * _dataPointer) const
{
	// unbind first and then only fill missing
	_v3d->unbind_all_vertex_attribs();
	if (get_location() == VertexFormatLocation::XY)
	{
		_shaderProgram->bind_vertex_attrib_array(_v3d, _shaderProgram->get_in_position_attribute_index(), 2, DataFormatValueType::Float, VertexFormatAttribType::Location, get_stride(), USE_OFFSET(_dataPointer, get_location_offset()));
	}
	else if (get_location() == VertexFormatLocation::XYZ)
	{
		_shaderProgram->bind_vertex_attrib_array(_v3d, _shaderProgram->get_in_position_attribute_index(), 3, DataFormatValueType::Float, VertexFormatAttribType::Location, get_stride(), USE_OFFSET(_dataPointer, get_location_offset()));
	}
	else
	{
		error(TXT("there has to be location!"));
	}
	if (get_normal() == VertexFormatNormal::Yes)
	{
		if (is_normal_packed())
		{
			_shaderProgram->bind_vertex_attrib_array(_v3d, _shaderProgram->get_in_normal_attribute_index(), 4, DataFormatValueType::Packed_10_10_10_2, VertexFormatAttribType::Vector, get_stride(), USE_OFFSET(_dataPointer, get_normal_offset()));
		}
		else
		{
			_shaderProgram->bind_vertex_attrib_array(_v3d, _shaderProgram->get_in_normal_attribute_index(), 3, DataFormatValueType::Float, VertexFormatAttribType::Vector, get_stride(), USE_OFFSET(_dataPointer, get_normal_offset()));
		}
	}
	else
	{
		_shaderProgram->bind_vertex_attrib(_v3d, _shaderProgram->get_in_normal_attribute_index(), Vector3::zAxis);
	}
	if (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes())
	{
		if (get_colour() != VertexFormatColour::None)
		{
			_shaderProgram->bind_vertex_attrib_array(_v3d, _shaderProgram->get_in_colour_attribute_index(), get_colour(), DataFormatValueType::Float, VertexFormatAttribType::Float, get_stride(), USE_OFFSET(_dataPointer, get_colour_offset()));
		}
		else
		{
			Colour fallbackColour = _v3d->get_fallback_colour();
			_shaderProgram->bind_vertex_attrib(_v3d, _shaderProgram->get_in_colour_attribute_index(), fallbackColour);
		}
	}
	if (get_texture_uv() != VertexFormatTextureUV::None)
	{
		_shaderProgram->bind_vertex_attrib_array(_v3d, _shaderProgram->get_in_uv_attribute_index(), get_texture_uv(), textureUVType, VertexFormatAttribType::Float, get_stride(), USE_OFFSET(_dataPointer, get_texture_uv_offset()));
	}
	else
	{
		_shaderProgram->bind_vertex_attrib(_v3d, _shaderProgram->get_in_uv_attribute_index(), Vector2::zero);
	}
	if (has_skinning_data() && _shaderProgram->does_support_skinning())
	{
		if (_shaderProgram->does_support_skinning_to_single_bone())
		{
			_shaderProgram->bind_vertex_attrib_array(_v3d, _shaderProgram->get_in_skinning_index_attribute_index(), 1, get_skinning_index_type(), VertexFormatAttribType::SkinningIndex, get_stride(), USE_OFFSET(_dataPointer, get_skinning_index_offset()));
		}
		else
		{
			_shaderProgram->bind_vertex_attrib_array(_v3d, _shaderProgram->get_in_skinning_indices_attribute_index(), get_skinning_element_count(), get_skinning_index_type(), VertexFormatAttribType::SkinningIndex, get_stride(), USE_OFFSET(_dataPointer, get_skinning_indices_offset()));
			_shaderProgram->bind_vertex_attrib_array(_v3d, _shaderProgram->get_in_skinning_weights_attribute_index(), get_skinning_element_count(), DataFormatValueType::Float, VertexFormatAttribType::SkinningWeight, get_stride(), USE_OFFSET(_dataPointer, get_skinning_weights_offset()));
		}
	}
	uint32 const *cdOffset = customDataOffset.begin();
	for_every(cd, customData)
	{
		_shaderProgram->bind_vertex_attrib_array(_v3d, _shaderProgram->find_attribute_index(cd->name), cd->count, cd->dataType, cd->attribType, get_stride(), USE_OFFSET(_dataPointer, *cdOffset));
		++cdOffset;
	}
}

void VertexFormat::debug_vertex_data(int _numberOfVertices, void const *_vertexData) const
{
#ifdef AN_OUTPUT
	output(TXT("contents:"));
	int8 const * atVertex = (int8 const*)_vertexData;
	for (int i = 0; i < _numberOfVertices; ++i)
	{
		output(TXT("   %05i :"), i);
		if (locationOffset != NONE)
		{
			float const * d = (float const*)(atVertex + locationOffset);
			output(TXT("     loc : %.3f %.3f %.3f"), *(d), *(d + 1), *(d + 2));
		}
		if (normalOffset != NONE)
		{
			if (is_normal_packed())
			{
				Vector3 n = ((VectorPacked const*)(atVertex + normalOffset))->get_as_vertex_normal();
				output(TXT("     nor : %.3f %.3f %.3f"), n.x, n.y, n.z);
			}
			else
			{
				float const * d = (float const*)(atVertex + normalOffset);
				output(TXT("     nor : %.3f %.3f %.3f"), *(d), *(d + 1), *(d + 2));
			}
		}
		if (colourOffset != NONE)
		{
			float const * d = (float const*)(atVertex + colourOffset);
			if (colour == VertexFormatColour::RGB)
			{
				output(TXT("     rgb : %.3f %.3f %.3f"), *(d), *(d + 1), *(d + 2));
			}
			if (colour == VertexFormatColour::RGBA)
			{
				output(TXT("     rgb : %.3f %.3f %.3f %.3f"), *(d), *(d + 1), *(d + 2), *(d + 3));
			}
		}
		if (textureUVOffset != NONE)
		{
			int8 const * d = (int8 const*)(atVertex + textureUVOffset);
			int dataSize = DataFormatValueType::get_size(textureUVType);
			if (textureUV == VertexFormatTextureUV::U)
			{
				output(TXT("       u : %.6f"),
					VertexFormatUtils::restore_as_float(d, textureUVType));
			}
			if (textureUV == VertexFormatTextureUV::UV)
			{
				output(TXT("      uv : %.6f %.6f"),
					VertexFormatUtils::restore_as_float(d, textureUVType),
					VertexFormatUtils::restore_as_float(d + dataSize, textureUVType));
			}
			if (textureUV == VertexFormatTextureUV::UVW)
			{
				output(TXT("     uvw : %.6f %.6f %.6f"),
					VertexFormatUtils::restore_as_float(d, textureUVType),
					VertexFormatUtils::restore_as_float(d + dataSize, textureUVType),
					VertexFormatUtils::restore_as_float(d + dataSize * 2, textureUVType));
			}
		}
		if (hasSkinningData)
		{
			if (skinningElementCount == 1)
			{
				if (skinningIndexType == DataFormatValueType::Uint8)
				{
					uint8 const * s = (uint8 const*)(atVertex + skinningIndexOffset);
					output(TXT("     skn : %03i"), *(s));
				}
				if (skinningIndexType == DataFormatValueType::Uint16)
				{
					uint16 const * s = (uint16 const*)(atVertex + skinningIndexOffset);
					output(TXT("     skn : %03i"), *(s));
				}
				if (skinningIndexType == DataFormatValueType::Uint32)
				{
					uint32 const * s = (uint32 const*)(atVertex + skinningIndexOffset);
					output(TXT("     skn : %03i"), *(s));
				}
			}
			else
			{
				if (skinningIndexType == DataFormatValueType::Uint8)
				{
					uint8 const * s = (uint8 const*)(atVertex + skinningIndicesOffset);
					float const * w = (float const*)(atVertex + skinningWeightsOffset);
					for (int b = 0; b < skinningElementCount; ++b)
					{
						output(TXT("     sk%i : %03i %8.5f"), b, *(s), *(w + 0));
						++s;
						++w;
					}
				}
				if (skinningIndexType == DataFormatValueType::Uint16)
				{
					uint16 const * s = (uint16 const*)(atVertex + skinningIndicesOffset);
					float const * w = (float const*)(atVertex + skinningWeightsOffset);
					for (int b = 0; b < skinningElementCount; ++b)
					{
						output(TXT("     sk%i : %03i %8.5f"), b, *(s), *(w));
						++s;
						++w;
					}
				}
				if (skinningIndexType == DataFormatValueType::Uint32)
				{
					uint32 const * s = (uint32 const*)(atVertex + skinningIndicesOffset);
					float const * w = (float const*)(atVertex + skinningWeightsOffset);
					for (int b = 0; b < skinningElementCount; ++b)
					{
						output(TXT("     sk%i : %03i %8.5f"), b, *(s), *(w));
						++s;
						++w;
					}
				}
			}
		}
		atVertex += get_stride();
	}
#endif
}

bool VertexFormat::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	for_every(node, _node->all_children())
	{
		if (node->get_name() == TXT("2d"))
		{
			location = VertexFormatLocation::XY;
		}
		else if (node->get_name() == TXT("3d"))
		{
			location = VertexFormatLocation::XYZ;
		}
		if (node->get_name() == TXT("normal"))
		{
			normal = VertexFormatNormal::Yes;
		}
		else if (node->get_name() == TXT("noNormal"))
		{
			normal = VertexFormatNormal::No;
		}
		else if (node->get_name() == TXT("colour") ||
				 node->get_name() == TXT("color") ||
				 node->get_name() == TXT("colourRGB") || 
				 node->get_name() == TXT("colorRGB") ||
				 node->get_name() == TXT("RGB"))
		{
			colour = VertexFormatColour::RGB;
		}
		else if (node->get_name() == TXT("colourWithAlpha") ||
				 node->get_name() == TXT("colorWithAlpha") ||
				 node->get_name() == TXT("colourRGBA") || 
				 node->get_name() == TXT("colorRGBA") ||
				 node->get_name() == TXT("RGBA"))
		{
			colour = VertexFormatColour::RGBA;
		}
		else if (node->get_name() == TXT("noColour") ||
				 node->get_name() == TXT("noColor"))
		{
			colour = VertexFormatColour::None;
		}
		else if (node->get_name() == TXT("textureU"))
		{
			textureUV = VertexFormatTextureUV::U;
		}
		else if (node->get_name() == TXT("textureUV"))
		{
			textureUV = VertexFormatTextureUV::UV;
		}
		else if (node->get_name() == TXT("textureUVW"))
		{
			textureUV = VertexFormatTextureUV::UVW;
		}
		else if (node->get_name() == TXT("noTexture") || 
				 node->get_name() == TXT("noTextureU") ||
				 node->get_name() == TXT("noTextureUV") ||
				 node->get_name() == TXT("noTextureUvW"))
		{
			textureUV = VertexFormatTextureUV::None;
		}
		else if (node->get_name() == TXT("skinningToSingleBone"))
		{
			hasSkinningData = true;
			skinningElementCount = 1;
			skinningIndexType = DataFormatValueType::Uint8;
			if (IO::XML::Attribute const * attr = node->get_attribute(TXT("indexType")))
			{
				skinningIndexType = DataFormatValueType::parse(attr->get_as_string());
			}
		}
		else if (node->get_name() == TXT("skinning"))
		{
			hasSkinningData = true;
			skinningElementCount = 4;
			skinningIndexType = DataFormatValueType::Uint8;
			if (IO::XML::Attribute const * attr = node->get_attribute(TXT("elementCount")))
			{
				skinningElementCount = ParserUtils::parse_int(attr->get_as_string(), skinningElementCount);
				if (skinningElementCount < 1)
				{
					skinningElementCount = 1;
					error_loading_xml(node, TXT("if skinning is required, it should have at least 1 bone"));
					result = false;
				}
				if (skinningElementCount > 4)
				{
					skinningElementCount = 4;
					error_loading_xml(node, TXT("no more than 4 bones supported for skinning"));
					result = false;
				}
			}
			if (IO::XML::Attribute const * attr = node->get_attribute(TXT("indexType")))
			{
				skinningIndexType = DataFormatValueType::parse(attr->get_as_string());
			}
		}
		else if (node->get_name() == TXT("noSkinning"))
		{
			hasSkinningData = false;
		}
		else if (node->get_name() == TXT("custom"))
		{
			Name name = node->get_name_attribute(TXT("name"));
			DataFormatValueType::Type dataType = DataFormatValueType::Float;
			if (auto* attr = node->get_attribute(TXT("type")))
			{
				warn_loading_xml(node, TXT("\"type\" is deprecated, use \"dataType\""));
				dataType = DataFormatValueType::parse(attr->get_as_string(), dataType);
			}
			if (auto * attr = node->get_attribute(TXT("dataType")))
			{
				dataType = DataFormatValueType::parse(attr->get_as_string(), dataType);
			}
			VertexFormatAttribType::Type attribType = VertexFormatAttribType::Float;
			if (auto * attr = node->get_attribute(TXT("attribType")))
			{
				attribType = VertexFormatAttribType::parse(attr->get_as_string());
			}
			int count = 1;
			if (attribType == VertexFormatAttribType::Location ||
				attribType == VertexFormatAttribType::Vector)
			{
				count = 3;
			}
			count = node->get_int_attribute(TXT("count"), count);
			customData.push_back(VertexFormatCustomData(name, dataType, attribType, count));
		}
		else
		{
			error_loading_xml(node, TXT("not recognised node type \"%S\" for vertex format"), node->get_name().to_char());
			result = false;
		}
	}

	calculate_stride_and_offsets();

	return result;
}

bool VertexFormat::operator==(VertexFormat const & _other) const
{
	if (stride == _other.stride &&
		paddingAllowed == _other.paddingAllowed &&
		location == _other.location &&
		normal == _other.normal &&
		normalPacked == _other.normalPacked &&
		colour == _other.colour &&
		textureUV == _other.textureUV &&
		textureUVType == _other.textureUVType &&
		hasSkinningData == _other.hasSkinningData &&
		(!hasSkinningData || (skinningElementCount == _other.skinningElementCount && skinningIndexType == _other.skinningIndexType)) &&
		customData.get_size() == _other.customData.get_size())
	{
		VertexFormatCustomData const * otherCd = _other.customData.begin();
		for_every(cd, customData)
		{
			if (*cd != *otherCd)
			{
				return false;
			}
			++otherCd;
		}
		return true;
	}
	return false;
}

VertexFormatCustomData const * VertexFormat::get_custom_data(Name const & _name) const
{
	for_every(cd, customData)
	{
		if (cd->name == _name)
		{
			return cd;
		}
	}
	return nullptr;
}

uint32 VertexFormat::get_custom_data_offset(Name const & _name) const
{
	for_every(cd, customData)
	{
		if (cd->name == _name)
		{
			int const idx = for_everys_index(cd);
			return customDataOffset.is_index_valid(idx)? customDataOffset[idx] : NONE;
		}
	}
	return NONE;
}

DataFormatValueType::Type VertexFormat::get_custom_data_type(Name const& _name) const
{
	for_every(cd, customData)
	{
		if (cd->name == _name)
		{
			return cd->dataType;
		}
	}
	return DataFormatValueType::Float;
}

#undef USE_OFFSET
#undef ADD_ATTRIB
#undef NO_ATTRIB
