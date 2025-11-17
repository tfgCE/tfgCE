#include "indexFormatUtils.h"

#include "indexFormat.h"

#include "..\..\serialisation\serialiser.h"

//

using namespace System;

//

bool IndexFormatUtils::do_match(IndexFormat const& _a, IndexFormat const& _b)
{
	return _a.get_element_size() == _b.get_element_size();
}

bool IndexFormatUtils::increase_elements_by(IndexFormat const & _format, void* _data, int _dataSize, int _increaseBy)
{
	an_assert(_format.get_element_size() != IndexElementSize::None);
	if (_format.get_element_size() == IndexElementSize::None)
	{
		return false;
	}
	DataFormatValueType::Type indexAsDataFormatValueType = IndexElementSize::to_data_format_value_type(_format.get_element_size());
	int8* data = (int8*)_data;
	int dataHandled = 0;
	while (dataHandled < _dataSize)
	{
		uint index = DataFormatValueType::get_uint_from(indexAsDataFormatValueType, data);
		index += _increaseBy;
		DataFormatValueType::fill_with_uint(indexAsDataFormatValueType, index, data);
		//
		data += _format.get_stride();
		dataHandled += _format.get_stride();
	}

	return true;
}

uint IndexFormatUtils::get_value(IndexFormat const & _format, void const * _at)
{
	an_assert(_format.get_element_size() != IndexElementSize::None);
	if (_format.get_element_size() == IndexElementSize::None)
	{
		return NONE;
	}
	DataFormatValueType::Type indexAsDataFormatValueType = IndexElementSize::to_data_format_value_type(_format.get_element_size());
	return DataFormatValueType::get_uint_from(indexAsDataFormatValueType, _at);
}

void IndexFormatUtils::set_value(IndexFormat const & _format, void* _at, uint _indexValue)
{
	an_assert(_format.get_element_size() != IndexElementSize::None);
	if (_format.get_element_size() == IndexElementSize::None)
	{
		return;
	}
	DataFormatValueType::Type indexAsDataFormatValueType = IndexElementSize::to_data_format_value_type(_format.get_element_size());
	DataFormatValueType::fill_with_uint(indexAsDataFormatValueType, _indexValue, _at);
}

uint IndexFormatUtils::get_value(IndexFormat const & _format, void const * _dataBuffer, int _idx)
{
	an_assert(_format.get_element_size() != IndexElementSize::None);
	if (_format.get_element_size() == IndexElementSize::None)
	{
		return NONE;
	}
	DataFormatValueType::Type indexAsDataFormatValueType = IndexElementSize::to_data_format_value_type(_format.get_element_size());
	return DataFormatValueType::get_uint_from(indexAsDataFormatValueType, (int8 const*)_dataBuffer + _format.get_stride() * _idx);
}

void IndexFormatUtils::set_value(IndexFormat const & _format, void * _dataBuffer, int _idx, uint _indexValue)
{
	an_assert(_format.get_element_size() != IndexElementSize::None);
	if (_format.get_element_size() == IndexElementSize::None)
	{
		return;
	}
	DataFormatValueType::Type indexAsDataFormatValueType = IndexElementSize::to_data_format_value_type(_format.get_element_size());
	DataFormatValueType::fill_with_uint(indexAsDataFormatValueType, _indexValue, (int8 *)_dataBuffer + _format.get_stride() * _idx);
}

bool IndexFormatUtils::convert_data(IndexFormat const & _sourceFormat, void const * _source, int _sourceSize, IndexFormat & _destFormat, void * _dest)
{
	DataFormatValueType::Type sourceIndexAsDataFormatValueType = IndexElementSize::to_data_format_value_type(_sourceFormat.get_element_size());
	DataFormatValueType::Type destIndexAsDataFormatValueType = IndexElementSize::to_data_format_value_type(_destFormat.get_element_size());

	int sourceStride = _sourceFormat.get_stride();
	int destStride = _destFormat.get_stride();
	int8 const * src = (int8 const*)_source;
	int8 * dest = (int8 *)_dest;
	int handled = 0;
	while (handled < _sourceSize)
	{
		uint index = DataFormatValueType::get_uint_from(sourceIndexAsDataFormatValueType, src);
		DataFormatValueType::fill_with_uint(destIndexAsDataFormatValueType, index, dest);

		handled += sourceStride;
		src += sourceStride;
		dest += destStride;
	}
	return true;
}
