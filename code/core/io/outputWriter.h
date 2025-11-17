#pragma once

#include "memoryStorage.h"

namespace IO
{
	class OutputWriter
	: public MemoryStorage
	{
	public:
		OutputWriter();
		~OutputWriter();

		void flush_to_output();
	};

};
