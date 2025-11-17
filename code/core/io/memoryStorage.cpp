#include "memoryStorage.h"

#include "..\casts.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace IO;

//

MemoryStorage::MemoryStorage()
: Interface(&read_data_from_memory_storage, &write_data_to_memory_storage, &go_to_in_memory_storage, &get_loc_in_memory_storage)
, activeBufferIndex(NONE)
, pointerInBuffer(0)
{
}

MemoryStorage::~MemoryStorage()
{
	clear();
}

void MemoryStorage::clear()
{
	for_every_ptr(buffer, buffers)
	{
		delete buffer;
	}
	buffers.clear();
	activeBufferIndex = NONE;
	pointerInBuffer = 0;
}

uint MemoryStorage::read_data_from_memory_storage(Interface * io, void * const _data, uint const _numBytes)
{
	MemoryStorage* memoryStorage = (MemoryStorage*)io;

	uint toRead = _numBytes;
	byte * ptr = plain_cast<byte>(_data);

	while (toRead)
	{
		if (! memoryStorage->buffers.is_index_valid(memoryStorage->activeBufferIndex))
		{
			break;
		}

		Buffer* activeBuffer = memoryStorage->buffers[memoryStorage->activeBufferIndex];

		uint readHere = activeBuffer->length - memoryStorage->pointerInBuffer;
		readHere = toRead < readHere ? toRead : readHere;

		// read
		memory_copy(ptr, &(activeBuffer->data[memoryStorage->pointerInBuffer]), readHere);

		// move pointer
		ptr += readHere;
		memoryStorage->pointerInBuffer += readHere;
		toRead -= readHere;

		// move to next buffer
		if (memoryStorage->pointerInBuffer == activeBuffer->length)
		{
			memoryStorage->pointerInBuffer = 0;
			++memoryStorage->activeBufferIndex;
		}
	}

	// return how much was read
	return _numBytes - toRead;
}

uint MemoryStorage::write_data_to_memory_storage(Interface * io, void const * const _data, uint const _numBytes)
{
	MemoryStorage* memoryStorage = (MemoryStorage*)io;

	uint toWrite = _numBytes;
	byte const * ptr = plain_cast<byte const>(_data);

	while (toWrite)
	{
		if (!memoryStorage->buffers.is_index_valid(memoryStorage->activeBufferIndex))
		{
			// add new buffer
			memoryStorage->buffers.push_back(new Buffer());
			memoryStorage->activeBufferIndex = memoryStorage->buffers.get_size() - 1;
		}

		Buffer* activeBuffer = memoryStorage->buffers[memoryStorage->activeBufferIndex];

		uint writeHere = bufSize - memoryStorage->pointerInBuffer;
		writeHere = toWrite < writeHere? toWrite : writeHere;

		// write
		memory_copy(&(activeBuffer->data[memoryStorage->pointerInBuffer]), ptr, writeHere);

		// move pointer
		ptr += writeHere;
		memoryStorage->pointerInBuffer += writeHere;
		toWrite -= writeHere;

		// update buffer length
		activeBuffer->length = activeBuffer->length > memoryStorage->pointerInBuffer ? activeBuffer->length : memoryStorage->pointerInBuffer;

		// move to next buffer
		if (memoryStorage->pointerInBuffer == bufSize)
		{
			memoryStorage->pointerInBuffer = 0;
			++memoryStorage->activeBufferIndex;
		}
	}

	// return how much was wrote
	return _numBytes - toWrite;
}

uint MemoryStorage::go_to_in_memory_storage(Interface * io, uint const _newLoc)
{
	MemoryStorage* memoryStorage = (MemoryStorage*)io;

	memoryStorage->pointerInBuffer = _newLoc % bufSize;
	memoryStorage->activeBufferIndex = (_newLoc - memoryStorage->pointerInBuffer) / bufSize;
	if (memoryStorage->buffers.is_index_valid(memoryStorage->activeBufferIndex))
	{
		memoryStorage->pointerInBuffer = min(memoryStorage->pointerInBuffer, memoryStorage->buffers[memoryStorage->activeBufferIndex]->length);
	}
	else
	{
		memoryStorage->activeBufferIndex = memoryStorage->buffers.get_size() - 1;
		memoryStorage->pointerInBuffer = memoryStorage->buffers.is_empty()? 0 : memoryStorage->buffers.get_last()->length;
	}

	return get_loc_in_memory_storage(io);
}

uint MemoryStorage::get_loc_in_memory_storage(Interface * io)
{
	MemoryStorage* memoryStorage = (MemoryStorage*)io;

	uint loc = 0;
	if (memoryStorage->activeBufferIndex != NONE)
	{
		loc = memoryStorage->activeBufferIndex * bufSize + memoryStorage->pointerInBuffer;
	}
	else if (!memoryStorage->buffers.is_empty())
	{
		loc = (memoryStorage->buffers.get_size() - 1) * bufSize + memoryStorage->buffers.get_last()->length;
	}
	else
	{
		loc = 0;
	}

	return loc;
}

uint MemoryStorage::get_length() const
{
	uint len = 0;
	for_every_ptr(b, buffers)
	{
		len += b->length;
	}
	return len;
}
