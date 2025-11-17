#pragma once

#include "video3d.h"

#include "..\..\mesh\remapBoneArray.h"

class Serialiser;

namespace System
{
	namespace VertexFormatLocation
	{
		enum Type
		{
			XY = 2,
			XYZ = 3,
		};

		inline Type parse(String const & _value)
		{
			if (_value == TXT("xy")) return XY;
			if (_value == TXT("xyz")) return XYZ;
			error(TXT("value \"%S\" not recognised"), _value.to_char());
			return XYZ;
		}
	};

	namespace VertexFormatNormal
	{
		enum Type
		{
			No	= false,
			Yes	= true
		};

		inline Type parse(String const & _value)
		{
			if (_value == TXT("yes")) return Yes;
			if (_value == TXT("no")) return No;
			error(TXT("value \"%S\" not recognised"), _value.to_char());
			return No;
		}
	};
	
	namespace VertexFormatColour
	{
		enum Type
		{
			None	= 0,
			RGB		= 3,
			RGBA	= 4,
		};

		inline Type parse(String const & _value)
		{
			if (_value == TXT("rgb")) return RGB;
			if (_value == TXT("rgba")) return RGBA;
			if (_value == TXT("none")) return None;
			error(TXT("value \"%S\" not recognised"), _value.to_char());
			return None;
		}
	};

	namespace VertexFormatTextureUV
	{
		enum Type
		{
			None	= 0,
			U		= 1,
			UV		= 2,
			UVW		= 3,
		};

		inline Type parse(String const & _value)
		{
			if (_value == TXT("u")) return U;
			if (_value == TXT("uv")) return UV;
			if (_value == TXT("uvw")) return UVW;
			error(TXT("value \"%S\" not recognised"), _value.to_char());
			return UV;
		}
	};

	namespace VertexFormatViewRay
	{
		enum Type
		{
			No = false,
			Yes = true
		};

		inline Type parse(String const & _value)
		{
			if (_value == TXT("yes")) return Yes;
			if (_value == TXT("no")) return No;
			error(TXT("value \"%S\" not recognised"), _value.to_char());
			return No;
		}
	};

	struct VertexFormatAttrib
	{
		Name name;
		int offset;
		int size;
		DataFormatValueType::Type dataType;
		VertexFormatAttribType::Type attribType;
		VertexFormatAttrib();
		VertexFormatAttrib(Name const & _name, int _offset, int _size, DataFormatValueType::Type _dataType, VertexFormatAttribType::Type _attribType);
	};

	struct VertexFormatCustomData
	{
		Name name;
		DataFormatValueType::Type dataType;
		int count;
		VertexFormatAttribType::Type attribType;

		VertexFormatCustomData() {}
		VertexFormatCustomData(Name const & _name, DataFormatValueType::Type _dataType, VertexFormatAttribType::Type _attribType, int _count = 1);

		bool operator == (VertexFormatCustomData const & _other) const { return name == _other.name && dataType == _other.dataType && count == _other.count && attribType == _other.attribType; }
		bool operator != (VertexFormatCustomData const & _other) const { return ! operator==(_other); }

		bool serialise(Serialiser & _serialiser);
	};

	/**
	 *	Describes data layout in vertex buffer
	 *	Composed of:
	 *		location of vertex (3 floats)
	 *		normal (optional)
	 *		colour (optional)
	 *		texture uv (optional)
	 *		skinning (optional) (skinning to single bone (index); skinning to multiple bones (index + weight))
	 *		view ray (optional)
	 */
	struct VertexFormat
	{
	public:
		VertexFormat();

		inline ~VertexFormat()
		{
			// there might be no more access to this vertex format, inform Video3D about it (in case it uses it)
			if (okToUse)
			{
				if (auto * v3d = Video3D::get())
				{
					v3d->invalidate_vertex_format(this);
				}
			}
		}

		inline VertexFormat & no_padding() { paddingAllowed = false; return *this; }
		inline VertexFormat & with_padding() { paddingAllowed = true; return *this; }

		inline VertexFormat & used_for_dynamic_data() { usedForDynamicData = true; return *this; }

		inline VertexFormat & with_location_2d() { location = VertexFormatLocation::XY; okToUse = false; return *this; }
		inline VertexFormat & with_location_3d() { location = VertexFormatLocation::XYZ; okToUse = false; return *this; }
		inline VertexFormat & with_normal() { normal = VertexFormatNormal::Yes; okToUse = false; return *this; }
		inline VertexFormat & with_colour_rgb() { colour = VertexFormatColour::RGB; okToUse = false; return *this; }
		inline VertexFormat & with_colour_rgba() { colour = VertexFormatColour::RGBA; okToUse = false; return *this; }
		inline VertexFormat & with_texture_uv(VertexFormatTextureUV::Type _type = VertexFormatTextureUV::UV) { textureUV = _type; okToUse = false; return *this; }
		inline VertexFormat & with_texture_uv_type(DataFormatValueType::Type _type = DataFormatValueType::Float) {textureUVType = _type; okToUse = false; return *this; }
		inline VertexFormat & with_skinning_data(DataFormatValueType::Type _type = DataFormatValueType::Uint8, int _elementCount = 4) { hasSkinningData = true; skinningIndexType = _type; skinningElementCount = _elementCount; okToUse = false; return *this; }
		inline VertexFormat & with_skinning_to_single_bone_data(DataFormatValueType::Type _type = DataFormatValueType::Uint8) { return with_skinning_data(_type, 1); }
		inline VertexFormat & with_custom(Name const& _name, DataFormatValueType::Type _type, VertexFormatAttribType::Type _attribType, int _count = 1) { customData.push_back(VertexFormatCustomData(_name, _type, _attribType, _count)); okToUse = false; return *this; }

		inline VertexFormat & no_skinning_data() { hasSkinningData = false; return *this; }

		inline bool is_ok_to_be_used() const { return okToUse; }
		
		inline bool is_used_for_dynamic_data() const { return usedForDynamicData; }

		VertexFormat & calculate_stride_and_offsets();

		bool has_skinning_data() const { return hasSkinningData; } // any skinning (to single bone or multiple bones)
		int get_skinning_element_count() const { an_assert(has_skinning_data()); return skinningElementCount; }
		DataFormatValueType::Type get_skinning_index_type() const { an_assert(has_skinning_data()); return skinningIndexType; }

		VertexFormatLocation::Type get_location() const { return location; }
		VertexFormatNormal::Type get_normal() const { return normal; } // remember about normal packed
		VertexFormatColour::Type get_colour() const { return colour; }
		VertexFormatTextureUV::Type get_texture_uv() const { return textureUV; }
		Array<VertexFormatCustomData> const & get_custom_data() const { return customData; }
		VertexFormatCustomData const * get_custom_data(Name const & _name) const;
		inline bool has_custom_data(Name const& _name) const { return get_custom_data(_name) != nullptr; }

		Array<VertexFormatAttrib> const & get_attribs() const { return attribs; }

		uint32 get_stride() const { return stride; }
		uint32 get_location_offset() const { return locationOffset; }
		uint32 get_normal_offset() const { return normalOffset; } // remember about normal packed
		uint32 get_colour_offset() const { return colourOffset; }
		uint32 get_texture_uv_offset() const { return textureUVOffset; }
		uint32 get_skinning_index_offset() const { return skinningIndexOffset; }
		uint32 get_skinning_indices_offset() const { return skinningIndicesOffset; }
		uint32 get_skinning_weights_offset() const { return skinningWeightsOffset; }
		uint32 get_custom_data_offset(Name const & _name) const;
		bool is_normal_packed() const { return normalPacked; }

		DataFormatValueType::Type get_texture_uv_type() const { return textureUVType; }
		DataFormatValueType::Type get_custom_data_type(Name const& _name) const;

		bool serialise(Serialiser & _serialiser);

		void bind_vertex_attrib(Video3D* _v3d, ShaderProgram * _shaderProgram, void const * _dataPointer) const;

		void debug_vertex_data(int _numberOfVertices, void const *_vertexData) const;

		bool load_from_xml(IO::XML::Node const * _node); // can adjust existing format, will calculate stride

		bool operator==(VertexFormat const & _other) const;
		bool operator!=(VertexFormat const& _other) const { return !(*this == _other); }

	private:
		bool okToUse;
		
		bool usedForDynamicData = false; // means it has to be reloaded/resetup everyframe

		bool paddingAllowed;
		VertexFormatLocation::Type location;
		VertexFormatNormal::Type normal;
		VertexFormatColour::Type colour;
		VertexFormatTextureUV::Type textureUV;
		DataFormatValueType::Type textureUVType;
		Array<VertexFormatCustomData> customData;

		bool hasSkinningData;
		int skinningElementCount;
		DataFormatValueType::Type skinningIndexType;

		bool normalPacked;

		// in bytes
		uint32 stride;
		//
		uint32 locationOffset; // should be always 0 as that how the gpus require it to be
		uint32 normalOffset;
		uint32 colourOffset;
		uint32 textureUVOffset;
		uint32 skinningIndexOffset;
		uint32 skinningIndicesOffset;
		uint32 skinningWeightsOffset;
		Array<uint32> customDataOffset;

		Array<VertexFormatAttrib> attribs; // calculated when calculating stride and offset
	};
	inline VertexFormat vertex_format() { return VertexFormat(); }

};

bool serialise(Serialiser& _serialiser, ::System::VertexFormatLocation::Type & _data);
bool serialise(Serialiser& _serialiser, ::System::VertexFormatNormal::Type & _data);
bool serialise(Serialiser& _serialiser, ::System::VertexFormatColour::Type & _data);
bool serialise(Serialiser& _serialiser, ::System::VertexFormatTextureUV::Type & _data);
bool serialise(Serialiser& _serialiser, ::System::VertexFormatViewRay::Type & _data);
