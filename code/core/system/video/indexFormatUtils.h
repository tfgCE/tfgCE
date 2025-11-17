#pragma once

#include "video3d.h"

namespace System
{
	struct IndexFormat;

	namespace IndexFormatUtils
	{
		bool do_match(IndexFormat const& _a, IndexFormat const& _b);

		// increase each element by given value
		bool increase_elements_by(IndexFormat const & _format, void* _data, int _dataSize, int _increaseBy);
		void set_value(IndexFormat const & _format, void* _at, uint _indexValue);
		uint get_value(IndexFormat const & _format, void const * _at);
		void set_value(IndexFormat const & _format, void* _dataBuffer, int _index, uint _indexValue);
		uint get_value(IndexFormat const & _format, void const * _dataBuffer, int _index);
		bool convert_data(IndexFormat const & _sourceFormat, void const * _source, int _sourceSize, IndexFormat & _destFormat, void * _dest);
	};
};
