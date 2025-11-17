#pragma once

#include "video3d.h"

#include "..\..\mesh\remapBoneArray.h"

namespace System
{
	struct VertexFormat;
	struct VertexFormatCustomData;

	namespace VertexFormatCompare
	{
		typedef int Flags;
		enum Flag
		{
			NotDefined  = 0,
			Location    = 1 << 0,
			Normal      = 1 << 1,

			All = 0xffffffff
		};

		Flag get_flag_for_attrib(Name const& _attrib);
	};

	namespace VertexFormatUtils
	{
		bool do_match(VertexFormat const& _a, VertexFormat const& _b);

		// will add (blank/invalid) or remove data in target buffer - requires that there is enough space - there is no check for that!
		bool convert_data(VertexFormat const& _sourceFormat, void const* _source, int _sourceSize, VertexFormat& _destFormat, void* _dest);

		// fill in _data accordingly to _format data that is at _offset with data from _fillWith
		bool fill_data_at_offset_with(VertexFormat const& _format, void* _data, int _dataSize, int _offset, void const* _fillWith, int _fillSize);

		// apply transform to data - checks attribs and applies transform as location or vector
		bool apply_transform_to_data(VertexFormat const& _format, void* _data, int _dataSize, Transform const& _transform);

		// interpolate between two vertices
		bool interpolate_vertex(VertexFormat const& _format, void* _data, void const* _0, void const* _1, float _pt);

		// get or set skinning index (_index) at element
		bool get_skinning_info(VertexFormat const& _format, void const* _at, int _index, OUT_ uint& _skinningIndex, OUT_ float& _weight);
		bool set_skinning_info(VertexFormat const& _format, void* _at, int _index, uint _skinningIndex, float _weight);

		// apply transform to data - checks attribs and applies transform as location or vector
		bool remap_bones(VertexFormat const& _format, void* _data, int _dataSize, Meshes::RemapBoneArray const& _remapBones);

		// compare two vertices
		bool compare(VertexFormat const& _format, void const* _a, void const* _b, VertexFormatCompare::Flags _flags = VertexFormatCompare::Flag::All);

		// will check for _values and will fill data for each vertex
		bool fill_custom_data(VertexFormat const& _format, void* _data, int _dataSize, SimpleVariableStorage const & _values);

		// will check for _values and will fill data for each vertex
		struct FillCustomDataParams
		{
			ADD_FUNCTION_PARAM_PLAIN_INITIAL_DEF(FillCustomDataParams, bool, addToExisting, add_to_existing, false, true);
			ADD_FUNCTION_PARAM(FillCustomDataParams, float, asFloat, with_float);
			ADD_FUNCTION_PARAM(FillCustomDataParams, int, asInt, with_int);
			ADD_FUNCTION_PARAM(FillCustomDataParams, Vector3, asVector3, with_vector3);
			ADD_FUNCTION_PARAM(FillCustomDataParams, Vector4, asVector4, with_vector4);
		};
		bool fill_custom_data(VertexFormat const& _format, void* _data, int _dataSize, VertexFormatCustomData const & _cd, FillCustomDataParams const & _params);

		//

		void store(int8* _at, DataFormatValueType::Type _type, Colour const& _value);
		void store(int8* _at, DataFormatValueType::Type _type, Vector2 const& _value);
		void store(int8* _at, DataFormatValueType::Type _type, Vector3 const& _value);
		void store(int8* _at, DataFormatValueType::Type _type, float _value, Optional<int> const & _offset = NP);
		//
		float restore_as_float(int8 const * _at, DataFormatValueType::Type _type, Optional<int> const& _offset = NP);

		void store_normal(int8* _start, int _vertexIndex, VertexFormat const& _vFormat, Vector3 const& _normal);
		void store_normal(int8* _at, VertexFormat const& _vFormat, Vector3 const& _normal);
		//
		Vector3 restore_normal(int8 const * _start, int _vertexIndex, VertexFormat const& _vFormat);
		Vector3 restore_normal(int8 const * _at, VertexFormat const& _vFormat);

		void store_location(int8* _start, int _vertexIndex, VertexFormat const& _vFormat, Vector3 const& _location);
		void store_location(int8* _at, VertexFormat const& _vFormat, Vector3 const& _location);
		//
		Vector3 restore_location(int8 const * _start, int _vertexIndex, VertexFormat const& _vFormat);
		Vector3 restore_location(int8 const * _at, VertexFormat const& _vFormat);
	};
};
