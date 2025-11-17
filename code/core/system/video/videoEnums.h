#pragma once

#include "..\..\globalDefinitions.h"

#include "..\..\serialisation\serialiserBasics.h"

#include "videoGL.h"

class Serialiser;

struct String;

namespace System
{
	namespace TextureWrap
	{
		enum Type
		{
			repeat = GL_REPEAT,
			clamp = GL_CLAMP_TO_EDGE,
		};

		Type parse(String const & _string);
	};

	namespace TextureFiltering
	{
		enum Type
		{
			nearest = GL_NEAREST,
			linear = GL_LINEAR,
			linearMipMapLinear = GL_LINEAR_MIPMAP_LINEAR,
		};

		Type parse(String const & _string);
		bool parse(String const & _string, REF_ Type & _filtering);
		Type get_sane_mag(Type _type);
		Type get_sane_min(Type _type);

		inline Type get_proper_for(Type _type, bool _withMipMaps)
		{
			if (_type == nearest) return nearest;
			return _withMipMaps? linearMipMapLinear : linear;
		}

		inline bool does_match(Type _a, Type _b)
		{
			return _a == _b ||
				   ((_a == linear || _a == linearMipMapLinear) &&
				    (_b == linear || _b == linearMipMapLinear));
		}
	};

	namespace MaterialShaderUsage
	{
		enum Type
		{
			Static,
			Skinned,
			SkinnedToSingleBone,
			//--
			Num
		};
	};

	namespace MaterialSetting
	{
		enum Type
		{
			Default,
			NotIndividualInstance, // if MaterialInstance is used, but it is not individual instance but it might be instance for archetype/template
			IndividualInstance, // individual instance - very single object in the world uses this material instance
			Forced, // forced, should be only used when debugging
		};
	};

	namespace BlendOp
	{
		enum Type
		{
			None,
			Zero = GL_ZERO,
			One = GL_ONE,
			SrcAlpha = GL_SRC_ALPHA,
			OneMinusSrcAlpha = GL_ONE_MINUS_SRC_ALPHA
			// add more if needed
		};

		bool parse(String const & _string, REF_ Type & _blendOp);
	};

	namespace DataFormatValueType
	{
		enum Type
		{
			Float = GL_FLOAT,
			Uint8 = GL_UNSIGNED_BYTE,
			Uint16 = GL_UNSIGNED_SHORT,
			Uint32 = GL_UNSIGNED_INT,
			Packed_10_10_10_2 = GL_INT_2_10_10_10_REV,
			Packed_Unsigned_10_10_10_2 = GL_UNSIGNED_INT_2_10_10_10_REV,
		};

		Type parse(String const& _value, Type _default = Uint8);
		bool is_integer_value(DataFormatValueType::Type _type);
		bool is_normalised_data(DataFormatValueType::Type _type);
		uint32 get_size(DataFormatValueType::Type _type);
		void fill_with_uint(DataFormatValueType::Type _type, uint _value, void* _at);
		uint get_uint_from(DataFormatValueType::Type _type, void const* _at);

	};

	namespace VertexFormatAttribType
	{
		enum Type
		{
			Other,
			Location,
			Vector, // has to be normalised
			Float,
			Integer,
			SkinningIndex,
			SkinningWeight
		};

		Type parse(String const& _value, Type _default = Other);

		bool should_be_mapped_as_integer(Type _type);
	};

	namespace ShaderType
	{
		enum Type
		{
			Vertex = GL_VERTEX_SHADER,
			Fragment = GL_FRAGMENT_SHADER,
			FunctionLib
		};
	};

};

DECLARE_SERIALISER_FOR(::System::TextureWrap::Type);
DECLARE_SERIALISER_FOR(::System::TextureFiltering::Type);
DECLARE_SERIALISER_FOR(::System::DataFormatValueType::Type);
DECLARE_SERIALISER_FOR(::System::VertexFormatAttribType::Type);
