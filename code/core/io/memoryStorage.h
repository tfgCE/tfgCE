#pragma once

#include "io.h"

namespace IO
{
	class MemoryStorage
	: public Interface
	{
	public:
		MemoryStorage();
		~MemoryStorage();

		void clear();

		uint get_length() const;

	protected:
		static uint read_data_from_memory_storage(Interface * io, void * const _data, uint const _numBytes);
		static uint write_data_to_memory_storage(Interface * io, void const * const _data, uint const _numBytes);
		static uint go_to_in_memory_storage(Interface * io, uint const _newLoc);
		static uint get_loc_in_memory_storage(Interface * io);

	protected:
		enum Constants
		{
			noFile = -1,
			bufSize = 16384
		};

		// ---

		struct Buffer
		{
			byte data[bufSize];
			uint length;

			Buffer() : length(0) {}
		private:
			Buffer(Buffer const &);
			Buffer& operator=(Buffer const &);
		};

		Array<Buffer*> buffers;
		uint activeBufferIndex;
		uint pointerInBuffer;
	};

};
