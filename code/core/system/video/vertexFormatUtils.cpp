#include "vertexFormatUtils.h"

#include "..\..\other\typeConversions.h"
#include "..\..\types\vectorPacked.h"

#include "vertexFormat.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace System;

//

bool VertexFormatUtils::do_match(VertexFormat const& _a, VertexFormat const& _b)
{
	an_assert(_a.is_ok_to_be_used() && _b.is_ok_to_be_used());
	if (_a.get_stride() != _b.get_stride() ||
		_a.get_attribs().get_size() != _b.get_attribs().get_size())
	{
		return false;
	}
	auto iA = _a.get_attribs().begin_const();
	auto iB = _b.get_attribs().begin_const();
	auto iAend = _a.get_attribs().end_const();
	while (iA != iAend)
	{
		if (iA->name != iB->name)
		{
			return false;
		}
		++iA;
		++iB;
	}
	return true;
}

bool VertexFormatUtils::convert_data(VertexFormat const & _sourceFormat, void const * _source, int _sourceSize, VertexFormat & _destFormat, void * _dest)
{
	an_assert(_sourceFormat.is_ok_to_be_used());
	_destFormat.calculate_stride_and_offsets();

	/**
	 *	We will be building information how we copy memory from source to destination buffer.
	 *		src attrib 0	-copy->		dest attrib 0		size of 1
	 *		src attrib 1	-skip->		...					size of 1
	 *		src attrib 2	-skip->		...					size of 1
	 *		src attrib 3	-copy->		dest attrib 1		size of 1
	 *		src attrib 4	-copy->		dest attrib 2		size of 1
	 *		...				-new?->		dest attrib 3		size of 1
	 *		src attrib 5	-copy->		dest attrib 4		size of 1
	 *
	 *	should create such array of copy mems:
	 *		sa0, da0, 1
	 *		sa3, da1, 2 (3-4 to 1-2)
	 *		sa5, da4, 1
	 */
	struct CopyMem
	{
		int sourceOffset;
		int destOffset;
		int length;

		int nextBuildingSourceOffset;
		int nextBuildingDestOffset;

		CopyMem()
		: sourceOffset(0)
		, destOffset(0)
		, length(0)
		, nextBuildingSourceOffset(0)
		, nextBuildingDestOffset(0)
		{}

		bool can_continue_in_this(int _sourceOffset, int _destOffset)
		{
			return length == 0 ||
				(_sourceOffset == nextBuildingSourceOffset && _destOffset == nextBuildingDestOffset);
		}

		void add(int _sourceOffset, int _destOffset, int _length)
		{
			if (length == 0)
			{
				sourceOffset = _sourceOffset;
				destOffset = _destOffset;
			}
			else
			{
				an_assert(_sourceOffset == nextBuildingSourceOffset);
				an_assert(_destOffset == nextBuildingDestOffset);
			}
			length += _length;
			nextBuildingSourceOffset += _length;
			nextBuildingDestOffset += _length;
		}
	};
	struct ProvideDefault
	{
		int destOffset;
		int length;
		void const* defaultData = nullptr;

		ProvideDefault()
		: destOffset(0)
		, length(0)
		, defaultData(nullptr)
		{}

		ProvideDefault(int _destOffset, int _length, void const* _defaultData = nullptr)
		: destOffset(_destOffset)
		, length(_length)
		, defaultData(_defaultData)
		{}
	};
	Array<CopyMem> copyMems;
	Array<ProvideDefault> provideDefaults;
	for_every(destAttrib, _destFormat.get_attribs())
	{
		bool hasSource = false;
		for_every(srcAttrib, _sourceFormat.get_attribs())
		{
			if (destAttrib->name == srcAttrib->name)
			{
				if (copyMems.is_empty() ||
					!copyMems.get_last().can_continue_in_this(srcAttrib->offset, destAttrib->offset))
				{
					// we can't continue with last "copyMem"
					copyMems.push_back(CopyMem());
				}
				// we can add data here - it might be in existing copyMem or just created one
				an_assert(srcAttrib->size == destAttrib->size, TXT("maybe we should convert one data to another? or maybe we should change format? for now it should be not possible!"));
				if (srcAttrib->size != destAttrib->size)
				{
					return false;
				}
				copyMems.get_last().add(srcAttrib->offset, destAttrib->offset, srcAttrib->size);
				hasSource = true;
				break;
			}
		}
		if (!hasSource)
		{
			if (_destFormat.get_colour() != VertexFormatColour::None &&
				destAttrib->offset == _destFormat.get_colour_offset())
			{
				provideDefaults.push_back(ProvideDefault(destAttrib->offset, destAttrib->size, &Colour::white));
			}
			else
			{
				provideDefaults.push_back(ProvideDefault(destAttrib->offset, destAttrib->size));
			}
		}
	}

	todo_future(TXT("check if we shouldn't just copying whole thing"));

	int sourceCopiedSoFar = 0;
	int8* sourceData = (int8*)_source;
	int8* destData = (int8*)_dest;
	while (sourceCopiedSoFar < _sourceSize)
	{
		for_every_const(copyMem, copyMems)
		{
			memory_copy(destData + copyMem->destOffset, sourceData + copyMem->sourceOffset, copyMem->length);
		}
		for_every_const(provideDefault, provideDefaults)
		{
			if (provideDefault->defaultData)
			{
				memory_copy(destData + provideDefault->destOffset, provideDefault->defaultData, provideDefault->length);
			}
			else
			{
				memory_zero(destData + provideDefault->destOffset, provideDefault->length);
			}
		}
		sourceCopiedSoFar += _sourceFormat.get_stride();
		sourceData += _sourceFormat.get_stride();
		destData += _destFormat.get_stride();
	}

	return true;
}

bool VertexFormatUtils::fill_data_at_offset_with(VertexFormat const & _format, void *_data, int _dataSize, int _offset, void const *_fillWith, int _fillSize)
{
	an_assert(_offset != NONE);
	if (_offset == NONE)
	{
		return false;
	}

	int8* data = (int8*)_data + _offset;
	int handled = 0;
	while (handled < _dataSize)
	{
		memory_copy(data, _fillWith, _fillSize);
		data += _format.get_stride();
		handled += _format.get_stride();
	}

	return true;
}

bool VertexFormatUtils::apply_transform_to_data(VertexFormat const & _format, void *_data, int _dataSize, Transform const & _transform)
{
	an_assert(_format.is_ok_to_be_used());

	// check what attribs require transform and transform
	for_every(attrib, _format.get_attribs())
	{
		if (attrib->attribType == VertexFormatAttribType::Location ||
			attrib->attribType == VertexFormatAttribType::Vector)
		{
			if (attrib->dataType == DataFormatValueType::Float)
			{
				int8* data = (int8*)_data + attrib->offset;
				int handled = 0;
				while (handled < _dataSize)
				{
					Vector3* vecData = (Vector3*)data;
					if (attrib->attribType == VertexFormatAttribType::Location)
					{
						*vecData = _transform.location_to_world(*vecData);
					}
					if (attrib->attribType == VertexFormatAttribType::Vector)
					{
						*vecData = _transform.vector_to_world(*vecData);
					}
					data += _format.get_stride();
					handled += _format.get_stride();
				}
			}
			else if (attrib->dataType == DataFormatValueType::Packed_10_10_10_2)
			{
				int8* data = (int8*)_data + attrib->offset;
				int handled = 0;
				while (handled < _dataSize)
				{
					Vector3 vecData = ((VectorPacked*)data)->get_as_vertex_normal();
					if (attrib->attribType == VertexFormatAttribType::Location)
					{
						vecData = _transform.location_to_world(vecData);
					}
					if (attrib->attribType == VertexFormatAttribType::Vector)
					{
						vecData = _transform.vector_to_world(vecData);
					}
					((VectorPacked*)data)->set_as_vertex_normal(vecData);
					data += _format.get_stride();
					handled += _format.get_stride();
				}
			}
			else
			{
				an_assert(false, TXT("not supported right now"));
			}
		}
	}

	return true;
}

bool VertexFormatUtils::interpolate_vertex(VertexFormat const & _format, void *_data, void const * _0, void const * _1, float _pt)
{
	bool result = true;

	_pt = clamp(_pt, 0.0f, 1.0f);

	an_assert(_format.is_ok_to_be_used());

	// check what attribs require transform and transform
	for_every(attrib, _format.get_attribs())
	{
		int8* data = (int8*)_data + attrib->offset;
		int8 const * s0 = (int8 const*)_0 + attrib->offset;
		int8 const * s1 = (int8 const*)_1 + attrib->offset;
		if (attrib->attribType == VertexFormatAttribType::Location)
		{
			for (int offset = 0; offset < attrib->size; offset += sizeof(float))
			{
				*((float*)(data + offset)) = lerp(_pt, *(float const*)(s0 + offset), *(float const*)(s1 + offset));
			}
		}
		else if (attrib->attribType == VertexFormatAttribType::Float)
		{
			for (int offset = 0; offset < attrib->size; offset += DataFormatValueType::get_size(attrib->dataType))
			{
				if (attrib->dataType == DataFormatValueType::Float)
				{
					*((float*)(data + offset)) = lerp(_pt, *(float const*)(s0 + offset), *(float const*)(s1 + offset));
				}
				else if (attrib->dataType == DataFormatValueType::Uint32)
				{
					*((uint32*)(data + offset)) = (uint32)lerp(_pt, (float) * (uint32 const*)(s0 + offset), (float) * (uint32 const*)(s1 + offset));
				}
				else if (attrib->dataType == DataFormatValueType::Uint16)
				{
					*((uint16*)(data + offset)) = (uint16)lerp(_pt, (float) * (uint16 const*)(s0 + offset), (float) * (uint16 const*)(s1 + offset));
				}
				else if (attrib->dataType == DataFormatValueType::Uint8)
				{
					*((uint8*)(data + offset)) = (uint8)lerp(_pt, (float) * (uint8 const*)(s0 + offset), (float) * (uint8 const*)(s1 + offset));
				}
				else
				{
					an_assert(false, TXT("not implemented"));
				}
			}
		}
		else if (attrib->attribType == VertexFormatAttribType::Vector)
		{
			if (attrib->dataType == DataFormatValueType::Packed_10_10_10_2)
			{
				an_assert(attrib->size == sizeof(int32), TXT("only some modes supported right now"));
				Vector3 v0 = ((VectorPacked const*)s0)->get_as_vertex_normal();
				Vector3 v1 = ((VectorPacked const*)s1)->get_as_vertex_normal();

				((VectorPacked*)data)->set_as_vertex_normal(Vector3::lerp(_pt, v0, v1).normal());
			}
			else if (attrib->dataType == DataFormatValueType::Float)
			{
				an_assert(attrib->size == 3 * sizeof(float), TXT("only some modes supported right now (attrib size is %i)"), attrib->size);
				*((Vector3*)data) = Vector3::lerp(_pt, *(Vector3 const*)s0, *(Vector3 const*)s1).normal();
			}
			else
			{
				an_assert(false);
			}
		}
		else if (attrib->attribType == VertexFormatAttribType::SkinningIndex)
		{
			struct SkinningElement
			{
				uint i;
				float w;

				static int compare(void const * _a, void const * _b)
				{
					SkinningElement const * a = plain_cast<SkinningElement>(_a);
					SkinningElement const * b = plain_cast<SkinningElement>(_b);
					float diff = a->w - b->w;
					return diff > 0.0f? -1 : (diff < 0.0f? 1 : 0);
				}
			};
			ARRAY_STATIC(SkinningElement, se, 20);
			an_assert(_format.get_skinning_element_count() * 2 <= se.get_max_size());
			SkinningElement seWorking;
					
			// gather non zero
			for_count(int, i, _format.get_skinning_element_count())
			{
				if (get_skinning_info(_format, _0, i, seWorking.i, seWorking.w))
				{
					seWorking.w *= (1.0f - _pt);
					if (seWorking.w > 0.0f)
					{
						se.push_back(seWorking);
					}
				}
				if (get_skinning_info(_format, _1, i, seWorking.i, seWorking.w))
				{
					seWorking.w *= _pt;
					if (seWorking.w > 0.0f)
					{
						se.push_back(seWorking);
					}
				}
			}

			// sum existing pairs
			for (int i = 0; i < se.get_size(); ++i)
			{
				auto & sei = se[i];
				for (int j = i + 1; j < se.get_size(); ++j)
				{
					if (se[j].i == sei.i)
					{
						sei.w += se[j].w;
						se.remove_fast_at(j);
						--j;
					}
				}
			}

			// sort to allow to keep only most important ones
			sort(se);

			// fill with empty
			seWorking.i = NONE; // should end as proper NONE for uint8 (255), uint16 (65535) or uint32 (that big number I won't remember)
			seWorking.w = 0.0f;
			while (se.get_size() < _format.get_skinning_element_count())
			{
				se.push_back(seWorking);
			}

			// no more than skinning element count
			se.set_size(_format.get_skinning_element_count());

			// normalise
			float totalWeight = 0.0f;
			for_every(s, se)
			{
				totalWeight += s->w;
			}
			float invTotalWeight = totalWeight != 0.0f ? 1.0f / totalWeight : 0.0f;
			for_every(s, se)
			{
				s->w *= invTotalWeight;
			}

			// feed back
			for_every(s, se)
			{
				set_skinning_info(_format, _data, for_everys_index(s), s->i, s->w);
			}
		}
		else
		{
			todo_note(TXT("Integer? choose one?"));
			// just copy
			memory_copy(data, s0, attrib->size);
		}
	}

	return result;
}

bool VertexFormatUtils::get_skinning_info(VertexFormat const & _format, void const *_at, int _index, OUT_ uint & _skinningIndex, OUT_ float & _weight)
{
	if (_format.has_skinning_data())
	{
		int8 const* atVertex = (int8 const*)_at;
		if (_format.get_skinning_element_count() == 1)
		{
			if (_index > 0)
			{
				return false;
			}
			if (_format.get_skinning_index_type() == DataFormatValueType::Uint8)
			{
				_skinningIndex = (uint)*(uint8 const*)(atVertex + _format.get_skinning_index_offset());
				if (_skinningIndex == (uint8)NONE)
				{
					_skinningIndex = NONE;
				}
				_weight = 1.0f;
				return true;
			}
			if (_format.get_skinning_index_type() == DataFormatValueType::Uint16)
			{
				_skinningIndex = (uint)*(uint16 const*)(atVertex + _format.get_skinning_index_offset());
				if (_skinningIndex == (uint16)NONE)
				{
					_skinningIndex = NONE;
				}
				_weight = 1.0f;
				return true;
			}
			if (_format.get_skinning_index_type() == DataFormatValueType::Uint32)
			{
				_skinningIndex = (uint)*(uint32 const*)(atVertex + _format.get_skinning_index_offset());
				if (_skinningIndex == (uint32)NONE)
				{
					_skinningIndex = NONE;
				}
				_weight = 1.0f;
				return true;
			}
		}
		else
		{
			if (_index > _format.get_skinning_element_count())
			{
				return false;
			}
			_weight = *((float const*)(atVertex + _format.get_skinning_weights_offset()) + _index);
			if (_format.get_skinning_index_type() == DataFormatValueType::Uint8)
			{
				_skinningIndex = (uint)*((uint8 const*)(atVertex + _format.get_skinning_indices_offset()) + _index);
				if (_skinningIndex == (uint8)NONE)
				{
					_skinningIndex = NONE;
				}
				return true;
			}
			if (_format.get_skinning_index_type() == DataFormatValueType::Uint16)
			{
				_skinningIndex = (uint)*((uint16 const*)(atVertex + _format.get_skinning_indices_offset()) + _index);
				if (_skinningIndex == (uint16)NONE)
				{
					_skinningIndex = NONE;
				}
				return true;
			}
			if (_format.get_skinning_index_type() == DataFormatValueType::Uint32)
			{
				_skinningIndex = (uint)*((uint32 const*)(atVertex + _format.get_skinning_indices_offset()) + _index);
				if (_skinningIndex == (uint32)NONE)
				{
					_skinningIndex = NONE;
				}
				return true;
			}
		}
	}
	return false;
}

bool VertexFormatUtils::set_skinning_info(VertexFormat const & _format, void *_at, int _index, uint _skinningIndex, float _weight)
{
	if (_format.has_skinning_data())
	{
		int8 * atVertex = (int8 *)_at;
		if (_format.get_skinning_element_count() == 1)
		{
			if (_index > 0)
			{
				return false;
			}
			if (_format.get_skinning_index_type() == DataFormatValueType::Uint8)
			{
				*(uint8 *)(atVertex + _format.get_skinning_index_offset()) = (uint8)_skinningIndex;
				return true;
			}
			if (_format.get_skinning_index_type() == DataFormatValueType::Uint16)
			{
				*(uint16 *)(atVertex + _format.get_skinning_index_offset()) = (uint16)_skinningIndex;
				return true;
			}
			if (_format.get_skinning_index_type() == DataFormatValueType::Uint32)
			{
				*(uint32 *)(atVertex + _format.get_skinning_index_offset()) = (uint32)_skinningIndex;
				return true;
			}
		}
		else
		{
			if (_index > _format.get_skinning_element_count())
			{
				return false;
			}
			*((float *)(atVertex + _format.get_skinning_weights_offset()) + _index) = _weight;
			if (_format.get_skinning_index_type() == DataFormatValueType::Uint8)
			{
				*((uint8 *)(atVertex + _format.get_skinning_indices_offset()) + _index) = (uint8)_skinningIndex;
				return true;
			}
			if (_format.get_skinning_index_type() == DataFormatValueType::Uint16)
			{
				*((uint16 *)(atVertex + _format.get_skinning_indices_offset()) + _index) = (uint16)_skinningIndex;
				return true;
			}
			if (_format.get_skinning_index_type() == DataFormatValueType::Uint32)
			{
				*((uint32 *)(atVertex + _format.get_skinning_indices_offset()) + _index) = (uint32)_skinningIndex;
				return true;
			}
		}
	}
	return false;
}

bool VertexFormatUtils::remap_bones(VertexFormat const & _format, void *_data, int _dataSize, Meshes::RemapBoneArray const & _remapBones)
{
	if (_format.has_skinning_data())
	{
		int offset = _format.get_skinning_element_count() == 1? _format.get_skinning_index_offset() : _format.get_skinning_indices_offset();
		int elementSize = DataFormatValueType::get_size(_format.get_skinning_index_type());
		int8* atVertexSkinningElement = (int8*)_data + offset;
		int handled = 0;

		if (_format.get_skinning_index_type() == DataFormatValueType::Uint8)
		{
			while (handled < _dataSize)
			{
				int8* atElement = atVertexSkinningElement;
				for_count(int, i, _format.get_skinning_element_count())
				{
					uint skinningIndex = (uint)*(uint8 const*)(atElement);
					skinningIndex = _remapBones[skinningIndex];
					*(uint8 *)(atElement) = skinningIndex;
					atElement += elementSize;
				}
				atVertexSkinningElement += _format.get_stride();
				handled += _format.get_stride();
			}
			return true;
		}
		if (_format.get_skinning_index_type() == DataFormatValueType::Uint16)
		{
			while (handled < _dataSize)
			{
				int8* atElement = atVertexSkinningElement;
				for_count(int, i, _format.get_skinning_element_count())
				{
					uint skinningIndex = (uint)*(uint16 const*)(atElement);
					skinningIndex = _remapBones[skinningIndex];
					*(uint16 *)(atElement) = skinningIndex;
					atElement += elementSize;
				}
				atVertexSkinningElement += _format.get_stride();
				handled += _format.get_stride();
			}
			return true;
		}
		if (_format.get_skinning_index_type() == DataFormatValueType::Uint32)
		{
			while (handled < _dataSize)
			{
				int8* atElement = atVertexSkinningElement;
				for_count(int, i, _format.get_skinning_element_count())
				{
					uint skinningIndex = (uint)*(uint32 const*)(atElement);
					skinningIndex = _remapBones[skinningIndex];
					*(uint32 *)(atElement) = skinningIndex;
					atElement += elementSize;
				}
				atVertexSkinningElement += _format.get_stride();
				handled += _format.get_stride();
			}
			return true;
		}
	}
	return false;
}

bool VertexFormatUtils::compare(VertexFormat const & _format, void const *_a, void const *_b, VertexFormatCompare::Flags _flags)
{
	an_assert(_format.is_ok_to_be_used());

	int8 const * a = (int8 const*)_a;
	int8 const * b = (int8 const*)_b;

	// check what attribs require transform and transform
	for_every(attrib, _format.get_attribs())
	{
		VertexFormatCompare::Flag attribFlag = VertexFormatCompare::get_flag_for_attrib(attrib->name);
		if (attribFlag == VertexFormatCompare::Flag::NotDefined || (_flags & attribFlag))
		{
			if (!memory_compare(a + attrib->offset, b + attrib->offset, attrib->size))
			{
				return false;
			}
		}
	}

	return true;
}

bool System::VertexFormatUtils::fill_custom_data(VertexFormat const& _format, void* _data, int _dataSize, SimpleVariableStorage const& _values)
{
	bool result = true;

	for_every(cd, _format.get_custom_data())
	{
		TypeId typeId;
		void const* value;
		if (_values.get_existing_of_any_type_id(cd->name, typeId, value))
		{
			FillCustomDataParams params;
			bool ok = false;
			if (typeId == type_id<float>())
			{
				params.with_float(*(float*)(value));
				ok = true;
			}
			if (typeId == type_id<int>())
			{
				params.with_int(*(int*)(value));
				ok = true;
			}
			if (typeId == type_id<Vector3>())
			{
				params.with_vector3(*(Vector3*)(value));
				ok = true;
			}
			if (typeId == type_id<Vector4>())
			{
				params.with_vector4(*(Vector4*)(value));
				ok = true;
			}
			if (ok)
			{
				result &= fill_custom_data(_format, _data, _dataSize, *cd, params);
			}
		}
	}

	return result;
}

bool System::VertexFormatUtils::fill_custom_data(VertexFormat const& _format, void* _data, int _dataSize, VertexFormatCustomData const& _cd, FillCustomDataParams const& _params)
{
	int cdOffset = _format.get_custom_data_offset(_cd.name);

	int8* currentVertexData = (int8*)_data;

	int stride = _format.get_stride();
	int vertexCount = _dataSize / stride;

	if (_cd.dataType == ::System::DataFormatValueType::Float)
	{
		if (_params.asVector3.is_set() &&
			(_cd.attribType == ::System::VertexFormatAttribType::Vector ||
			 _cd.attribType == ::System::VertexFormatAttribType::Location))
		{
			Vector3 v3 = _params.asVector3.get(Vector3::zero);
			for (int i = 0; i < vertexCount; ++i, currentVertexData += stride)
			{
				float* dest = (float*)(currentVertexData + cdOffset);
				if (_params.addToExisting)
				{
					*dest += v3.x; ++dest;
					*dest += v3.y; ++dest;
					*dest += v3.z;
				}
				else
				{
					*dest = v3.x; ++dest;
					*dest = v3.y; ++dest;
					*dest = v3.z;
				}
			}
		}
		else if (_cd.count == 3 && _params.asVector3.is_set())
		{
			Vector3 v3 = _params.asVector3.get(Vector3::zero);
			for (int i = 0; i < vertexCount; ++i, currentVertexData += stride)
			{
				float* dest = (float*)(currentVertexData + cdOffset);
				if (_params.addToExisting)
				{
					*dest += v3.x; ++dest;
					*dest += v3.y; ++dest;
					*dest += v3.z;
				}
				else
				{
					*dest = v3.x; ++dest;
					*dest = v3.y; ++dest;
					*dest = v3.z;
				}
			}
		}
		else if (_cd.count == 4 && _params.asVector4.is_set())
		{
			Vector4 v4 = _params.asVector4.get(Vector4::zero);
			for (int i = 0; i < vertexCount; ++i, currentVertexData += stride)
			{
				float* dest = (float*)(currentVertexData + cdOffset);
				if (_params.addToExisting)
				{
					*dest += v4.x; ++dest;
					*dest += v4.y; ++dest;
					*dest += v4.z; ++dest;
					*dest += v4.w;
				}
				else
				{
					*dest = v4.x; ++dest;
					*dest = v4.y; ++dest;
					*dest = v4.z; ++dest;
					*dest = v4.w;
				}
			}
		}
		else
		{
			float v = _params.asFloat.get(0.0f);
			for (int i = 0; i < vertexCount; ++i, currentVertexData += stride)
			{
				float* dest = (float*)(currentVertexData + cdOffset);
				for_count(int, i, _cd.count)
				{
					if (_params.addToExisting)
					{
						*dest += v;
					}
					else
					{
						*dest = v;
					}
					++dest;
				}
			}
		}
	}
	else if (_cd.dataType == ::System::DataFormatValueType::Uint8)
	{
		uint8 v = (uint8)_params.asInt.get(0);
		for (int i = 0; i < vertexCount; ++i, currentVertexData += stride)
		{
			uint8* dest = (uint8*)(currentVertexData + cdOffset);
			for_count(int, i, _cd.count)
			{
				if (_params.addToExisting)
				{
					*dest += v;
				}
				else
				{
					*dest = v;
				}
				++dest;
			}
		}
	}
	else if (_cd.dataType == ::System::DataFormatValueType::Uint16)
	{
		uint16 v = (uint16)_params.asInt.get(0);
		for (int i = 0; i < vertexCount; ++i, currentVertexData += stride)
		{
			uint16* dest = (uint16*)(currentVertexData + cdOffset);
			for_count(int, i, _cd.count)
			{
				if (_params.addToExisting)
				{
					*dest += v;
				}
				else
				{
					*dest = v;
				}
				++dest;
			}
		}
	}
	else if (_cd.dataType == ::System::DataFormatValueType::Uint32)
	{
		uint32 v = (uint32)_params.asInt.get(0);
		for (int i = 0; i < vertexCount; ++i, currentVertexData += stride)
		{
			uint32* dest = (uint32*)(currentVertexData + cdOffset);
			for_count(int, i, _cd.count)
			{
				if (_params.addToExisting)
				{
					*dest += v;
				}
				else
				{
					*dest = v;
				}
				++dest;
			}
		}
	}
	else
	{
		todo_important(TXT("implement_ another type"));
		return false;
	}

	return true;
}

void System::VertexFormatUtils::store(int8* _at, DataFormatValueType::Type _type, Colour const& _value)
{
	if (_type == DataFormatValueType::Float)
	{
		*(Colour*)(_at) = _value;
	}
	else if (_type == DataFormatValueType::Uint8)
	{
		*(uint8*)(_at + sizeof(uint8) * 0) = TypeConversions::Vertex::f_ui8(_value.r);
		*(uint8*)(_at + sizeof(uint8) * 1) = TypeConversions::Vertex::f_ui8(_value.g);
		*(uint8*)(_at + sizeof(uint8) * 2) = TypeConversions::Vertex::f_ui8(_value.b);
		*(uint8*)(_at + sizeof(uint8) * 3) = TypeConversions::Vertex::f_ui8(_value.a);
	}
	else
	{
		todo_important(TXT("implement!"));
	}
};

void System::VertexFormatUtils::store(int8* _at, DataFormatValueType::Type _type, Vector2 const& _value)
{
	if (_type == DataFormatValueType::Float)
	{
		*(Vector2*)(_at) = _value;
	}
	else if (_type == DataFormatValueType::Uint16)
	{
		*(uint16*)(_at + sizeof(uint16) * 0) = TypeConversions::Vertex::f_ui16(_value.x);
		*(uint16*)(_at + sizeof(uint16) * 1) = TypeConversions::Vertex::f_ui16(_value.y);
	}
	else
	{
		todo_important(TXT("implement!"));
	}
};

void System::VertexFormatUtils::store(int8* _at, DataFormatValueType::Type _type, Vector3 const& _value)
{
	if (_type == DataFormatValueType::Float)
	{
		*(Vector3*)(_at) = _value;
	}
	else if (_type == DataFormatValueType::Uint16)
	{
		*(uint16*)(_at + sizeof(uint16) * 0) = TypeConversions::Vertex::f_ui16(_value.x);
		*(uint16*)(_at + sizeof(uint16) * 1) = TypeConversions::Vertex::f_ui16(_value.y);
		*(uint16*)(_at + sizeof(uint16) * 2) = TypeConversions::Vertex::f_ui16(_value.z);
	}
	else
	{
		todo_important(TXT("implement!"));
	}
};

void System::VertexFormatUtils::store(int8* _at, DataFormatValueType::Type _type, float _value, Optional<int> const& _offset)
{
	if (_type == DataFormatValueType::Float)
	{
		_at += sizeof(float) * _offset.get(0);
		*(float*)(_at) = _value;
	}
	else if (_type == DataFormatValueType::Uint8)
	{
		_at += sizeof(uint8) * _offset.get(0);
		*(uint8*)(_at) = TypeConversions::Vertex::f_ui8(_value);
	}
	else if (_type == DataFormatValueType::Uint16)
	{
		_at += sizeof(uint16) * _offset.get(0);
		*(uint16*)(_at) = TypeConversions::Vertex::f_ui16(_value);
	}
	else if (_type == DataFormatValueType::Uint32)
	{
		_at += sizeof(uint32) * _offset.get(0);
		*(uint32*)(_at) = TypeConversions::Vertex::f_ui32(_value);
	}
	else
	{
		todo_important(TXT("implement!"));
	}
};

float System::VertexFormatUtils::restore_as_float(int8 const * _at, DataFormatValueType::Type _type, Optional<int> const& _offset)
{
	if (_type == DataFormatValueType::Float)
	{
		_at += sizeof(float) * _offset.get(0);
		return *(float*)(_at);
	}
	else if (_type == DataFormatValueType::Uint8)
	{
		_at += sizeof(uint8) * _offset.get(0);
		return TypeConversions::Vertex::ui8_f(*(uint8*)(_at));
	}
	else if (_type == DataFormatValueType::Uint16)
	{
		_at += sizeof(uint16) * _offset.get(0);
		return TypeConversions::Vertex::ui16_f(*(uint16*)(_at));
	}
	else if (_type == DataFormatValueType::Uint32)
	{
		_at += sizeof(uint32) * _offset.get(0);
		return TypeConversions::Vertex::ui32_f(*(uint32*)(_at));
	}
	else
	{
		todo_important(TXT("implement!"));
	}
	return 0.0f;
};

void System::VertexFormatUtils::store_normal(int8* _start, int _vertexIndex, VertexFormat const& _vFormat, Vector3 const& _normal)
{
	store_normal(_start + _vertexIndex * _vFormat.get_stride() + _vFormat.get_normal_offset(), _vFormat, _normal);
}

void System::VertexFormatUtils::store_normal(int8* _at, VertexFormat const& _vFormat, Vector3 const& _normal)
{
	an_assert(_vFormat.get_normal() == System::VertexFormatNormal::Yes);
	if (_vFormat.is_normal_packed())
	{
		cast_to_value<VectorPacked>(_at).set_as_vertex_normal(_normal);
	}
	else
	{
		cast_to_value<Vector3>(_at) = _normal;
	}
}

Vector3 System::VertexFormatUtils::restore_normal(int8 const* _start, int _vertexIndex, VertexFormat const& _vFormat)
{
	return restore_normal(_start + _vertexIndex * _vFormat.get_stride() + _vFormat.get_normal_offset(), _vFormat);
}

Vector3 System::VertexFormatUtils::restore_normal(int8 const* _at, VertexFormat const& _vFormat)
{
	an_assert(_vFormat.get_normal() == System::VertexFormatNormal::Yes);
	if (_vFormat.is_normal_packed())
	{
		return cast_to_value<VectorPacked>(_at).get_as_vertex_normal();
	}
	else
	{
		return cast_to_value<Vector3>(_at);
	}
}

void System::VertexFormatUtils::store_location(int8* _start, int _vertexIndex, VertexFormat const& _vFormat, Vector3 const& _location)
{
	store_location(_start + _vertexIndex * _vFormat.get_stride() + _vFormat.get_location_offset(), _vFormat, _location);
}

void System::VertexFormatUtils::store_location(int8* _at, VertexFormat const& _vFormat, Vector3 const& _location)
{
	if (_vFormat.get_location() == ::System::VertexFormatLocation::XYZ)
	{
		cast_to_value<Vector3>(_at) = _location;
	}
	else
	{
		an_assert(_vFormat.get_location() == ::System::VertexFormatLocation::XY);
		cast_to_value<Vector2>(_at) = _location.to_vector2();
	}
}

Vector3 System::VertexFormatUtils::restore_location(int8 const* _start, int _vertexIndex, VertexFormat const& _vFormat)
{
	return restore_location(_start + _vertexIndex * _vFormat.get_stride() + _vFormat.get_location_offset(), _vFormat);
}

Vector3 System::VertexFormatUtils::restore_location(int8 const* _at, VertexFormat const& _vFormat)
{
	if (_vFormat.get_location() == ::System::VertexFormatLocation::XYZ)
	{
		return cast_to_value<Vector3>(_at);
	}
	else
	{
		an_assert(_vFormat.get_location() == ::System::VertexFormatLocation::XY);
		return cast_to_value<Vector2>(_at).to_vector3();
	}
}
