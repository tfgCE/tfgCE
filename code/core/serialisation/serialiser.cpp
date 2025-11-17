#include "serialiser.h"

#include "..\io\io.h"

template <typename SimpleType>
bool serialise_simple_type(Serialiser & _serialiser, SimpleType & _data)
{
	if (_serialiser.is_reading())
	{
		_serialiser.get_io()->read_single(&_data, sizeof(_data));
		return true;
	}
	else if (_serialiser.is_writing())
	{
		_serialiser.get_io()->write_single(&_data, sizeof(_data));
		return true;
	}
	todo_important(TXT("what should I do?"));
	return false;
}

#define SIMPLE_SERIALISER_FOR(Type) template <> bool serialise_data<Type>(Serialiser & _serialiser, Type & _data) { return serialise_simple_type(_serialiser, _data); }

SIMPLE_SERIALISER_FOR(float);
SIMPLE_SERIALISER_FOR(bool);

SIMPLE_SERIALISER_FOR(int8);
SIMPLE_SERIALISER_FOR(int16);
SIMPLE_SERIALISER_FOR(int32);
SIMPLE_SERIALISER_FOR(int64);

SIMPLE_SERIALISER_FOR(uint8);
SIMPLE_SERIALISER_FOR(uint16);
SIMPLE_SERIALISER_FOR(uint32);
SIMPLE_SERIALISER_FOR(uint64);

bool serialise_plain_data(Serialiser & _serialiser, void * _data, uint _size)
{
	if (_serialiser.is_reading())
	{
		_serialiser.get_io()->read(_data, _size);
		return true;
	}
	else if (_serialiser.is_writing())
	{
		_serialiser.get_io()->write(_data, _size);
		return true;
	}
	todo_important(TXT("what should I do?"));
	return false;
}

//

Serialiser::Serialiser(IO::Interface* _io, SerialiserMode::Type _mode, bool _nameLibrary)
: io(_io)
, mode(_mode)
, usingNameLibrary(_nameLibrary)
{
	if (usingNameLibrary)
	{
		nameLibraryInfoLoc = io->get_loc();
		serialise_data(*this, nameLibraryLoc); // reader will read, writer will just store int

		if (is_reading())
		{
			io->go_to(nameLibraryLoc);

			// load name library
			usingNameLibrary = false; // to allow normal name serialisation (without name library)
			serialise_data(*this, nameLibrary);
			usingNameLibrary = true;

			io->go_to(nameLibraryInfoLoc);
			serialise_data(*this, nameLibraryLoc); // read once again to jump to next value
		}
	}
}

Serialiser::~Serialiser()
{
	if (usingNameLibrary && is_writing())
	{
		nameLibraryLoc = io->get_loc();
		io->go_to(nameLibraryInfoLoc);
		serialise_data(*this, nameLibraryLoc);
		io->go_to(nameLibraryLoc);

		// store name library
		usingNameLibrary = false; // to allow normal name serialisation (without name library)
		serialise_data(*this, nameLibrary);
		usingNameLibrary = true;
	}
}

bool Serialiser::serialise_name_using_name_library(Name & _name)
{
	an_assert(usingNameLibrary);
	if (is_writing())
	{
		int index = NONE;
		if (_name.is_valid())
		{
			for_every(name, nameLibrary)
			{
				if (*name == _name)
				{
					index = for_everys_index(name);
					break;
				}
			}
			if (index == NONE)
			{
				index = nameLibrary.get_size();
				nameLibrary.push_back(_name);
			}
		}
		return serialise_data(*this, index);
	}
	if (is_reading())
	{
		int index = NONE;
		if (serialise_data(*this, index))
		{
			if (nameLibrary.is_index_valid(index))
			{
				_name = nameLibrary[index];
				return true;
			}
			else if (index == NONE)
			{
				_name = Name();
				return true;
			}
			else
			{
				return false;
			}
		}
		return false;
	}
	an_assert(false);
	return false;
}

void Serialiser::pre_serialise_adjust_endian(void * const _data, uint const _numBytes)
{
	if (is_writing())
	{
		adjust_endian(_data, _numBytes);
	}
}

void Serialiser::pre_serialise_adjust_endian_series(void * const _data, uint const _numBytesSingle, uint const _numBytes)
{
	if (is_writing())
	{
		adjust_endian_series(_data, _numBytesSingle, _numBytes);
	}
}

void Serialiser::pre_serialise_adjust_endian_stride(void * const _data, uint const _numBytesSingle, uint const _strideCount, uint const _strideSize)
{
	if (is_writing())
	{
		adjust_endian_stride(_data, _numBytesSingle, _strideCount, _strideSize);
	}
}

void Serialiser::post_serialise_adjust_endian(void * const _data, uint const _numBytes)
{
	adjust_endian(_data, _numBytes);
}

void Serialiser::post_serialise_adjust_endian_series(void * const _data, uint const _numBytesSingle, uint const _numBytes)
{
	adjust_endian_series(_data, _numBytesSingle, _numBytes);
}

void Serialiser::post_serialise_adjust_endian_stride(void * const _data, uint const _numBytesSingle, uint const _strideCount, uint const _strideSize)
{
	adjust_endian_stride(_data, _numBytesSingle, _strideCount, _strideSize);
}
