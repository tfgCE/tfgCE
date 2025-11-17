#pragma once

#include "serialiserBasics.h"

#include "iSerialisable.h"
#include "..\containers\array.h"
#include "..\containers\arrayStatic.h"
#include "..\containers\map.h"
#include "..\types\name.h"
#include "..\memory\refCountObjectPtr.h"

#include <functional>

namespace IO
{
	class Interface;
};

namespace SerialiserMode
{
	enum Type
	{
		Reading,
		Writing
	};
};

class Serialiser
{
public:
	Serialiser(IO::Interface* _io, SerialiserMode::Type _mode, bool _nameLibrary = false);
	~Serialiser();

	Serialiser& temp_ref() { return *this; }

	static Serialiser reader(IO::Interface* _io, bool _nameLibrary = false) { return Serialiser(_io, SerialiserMode::Reading, _nameLibrary); }
	static Serialiser writer(IO::Interface* _io, bool _nameLibrary = false) { return Serialiser(_io, SerialiserMode::Writing, _nameLibrary); }

	bool is_reading() const { return mode == SerialiserMode::Reading; }
	bool is_writing() const { return mode == SerialiserMode::Writing; }
	bool is_using_name_library() const { return usingNameLibrary; }

	bool serialise_name_using_name_library(Name & _name);

	IO::Interface* get_io() { return io; }

	void pre_serialise_adjust_endian(void * const _data, uint const _numBytes);
	void pre_serialise_adjust_endian_series(void * const _data, uint const _numBytesSingle, uint const _numBytes);
	void pre_serialise_adjust_endian_stride(void * const _data, uint const _numBytesSingle, uint const _strideCount, uint const _strideSize);

	void post_serialise_adjust_endian(void * const _data, uint const _numBytes);
	void post_serialise_adjust_endian_series(void * const _data, uint const _numBytesSingle, uint const _numBytes);
	void post_serialise_adjust_endian_stride(void * const _data, uint const _numBytesSingle, uint const _strideCount, uint const _strideSize);

	void set_game_version(int _gameVersion) { gameVersion = _gameVersion; }
	int get_game_version() const { an_assert(gameVersion != NONE); return gameVersion; }

private:
	IO::Interface* io;
	SerialiserMode::Type mode;
	bool usingNameLibrary = false;
	int nameLibraryInfoLoc = NONE; // where info about name library is located
	int nameLibraryLoc = NONE; // where name library (dictionary of names is located)
	int gameVersion = NONE; // if we are dependent on game version, we should store it in serialised file, restore it and allow accesisng it other parts of code - this works as a context

	Array<Name> nameLibrary;
};

template <typename Class>
bool serialise_data(Serialiser & _serialiser, Class & _data)
{
	// default implementation for all types - simple types are handled in serialiser.cpp
	return _data.serialise(_serialiser);
}

template <typename Class>
bool serialise_data(Serialiser & _serialiser, Class *& _data)
{
	// default implementation for all types - simple types are handled in serialiser.cpp
	return _data->serialise(_serialiser);
}

template <typename Class>
bool serialise_data_if_present_create(Serialiser & _serialiser, Class *& _data, std::function<Class* ()> _create)
{
	bool result = true;

	bool isPresent = _data ? true : false;
	result &= serialise_data(_serialiser, isPresent);
	if (_serialiser.is_writing())
	{
		if (isPresent)
		{
			result &= serialise_data(_serialiser, *_data);
		}
	}
	if (_serialiser.is_reading())
	{
		if (isPresent)
		{
			if (!_data) _data = _create();
			result &= serialise_data(_serialiser, *_data);
		}
		else
		{
			delete_and_clear(_data);
		}
	}

	return result;
}

template <typename Class>
bool serialise_data_if_present(Serialiser & _serialiser, Class *& _data)
{
	return serialise_data_if_present_create<Class>(_serialiser, _data, []() { return new Class(); });
}

template <typename Class>
bool serialise_ref_data_if_present(Serialiser & _serialiser, RefCountObjectPtr<Class> & _data)
{
	bool result = true;
	Class * obj = _data.get();
	result &= serialise_data_if_present<Class>(_serialiser, obj);
	if (_serialiser.is_reading())
	{
		_data = obj;
	}
	return result;
}

template <typename Class>
bool serialise_data(Serialiser & _serialiser, RefCountObjectPtr<Class>& _data)
{
	return serialise_ref_data_if_present(_serialiser, _data);
}

template <typename ArrayType>
bool serialise_plain_data_array(Serialiser & _serialiser, ArrayType & _data)
{
	int size = _data.get_size();
	serialise_data(_serialiser, size);
	if (_serialiser.is_reading())
	{
		_data.set_size(size);
	}
	if (! _data.is_empty())
	{
		_serialiser.pre_serialise_adjust_endian_series(_data.get_data(), _data.get_single_data_size(), _data.get_data_size());
		serialise_plain_data(_serialiser, _data.get_data(), _data.get_data_size());
		_serialiser.post_serialise_adjust_endian_series(_data.get_data(), _data.get_single_data_size(), _data.get_data_size());
	}
	return true;
}

template <typename Type>
bool serialise_inlined_array(Serialiser & _serialiser, Array<Type> & _data)
{
	int size = _data.get_size();
	serialise_data(_serialiser, size);
	if (_serialiser.is_reading())
	{
		_data.set_size(size);
	}
	for_every(datum, _data)
	{
		serialise_data(_serialiser, *datum);
	}
	return true;
}

template <typename Type, int Size>
bool serialise_inlined_array_static(Serialiser & _serialiser, ArrayStatic<Type, Size> & _data)
{
	int size = _data.get_size();
	serialise_data(_serialiser, size);
	if (_serialiser.is_reading())
	{
		_data.set_size(size);
	}
	for_every(datum, _data)
	{
		serialise_data(_serialiser, *datum);
	}
	return true;
}

template <typename Class>
bool serialise_data(Serialiser & _serialiser, Array<Class>& _data)
{
	return serialise_inlined_array(_serialiser, _data);
}

template <typename Class, int Size>
bool serialise_data(Serialiser & _serialiser, ArrayStatic<Class, Size>& _data)
{
	return serialise_inlined_array_static(_serialiser, _data);
}

template <typename Type>
bool serialise_inlined_list(Serialiser& _serialiser, List<Type>& _data)
{
	int size = _data.get_size();
	serialise_data(_serialiser, size);
	if (_serialiser.is_reading())
	{
		_data.clear();
		for_count(int, i, size)
		{
			_data.push_back(Type());
			serialise_data(_serialiser, _data.get_last());
		}
		return true;
	}
	else
	{
		int i = 0;
		for_every(datum, _data)
		{
			serialise_data(_serialiser, *datum);
			++i;
		}
		return i == size;
	}
}

template <typename Class>
bool serialise_data(Serialiser& _serialiser, List<Class>& _data)
{
	return serialise_inlined_list(_serialiser, _data);
}

template <typename Key, typename Type>
bool serialise_inlined_map(Serialiser & _serialiser, Map<Key, Type> & _data)
{
	int size = 0;
	if (_serialiser.is_writing())
	{
		for_every(datum, _data)
		{
			++size;
		}
	}
	serialise_data(_serialiser, size);
	if (_serialiser.is_reading())
	{
		_data.clear();
		for_count(int, i, size)
		{
			Key key;
			serialise_data(_serialiser, key);
			serialise_data(_serialiser, _data[key]);
		}
	}
	if (_serialiser.is_writing())
	{
		for_every(datum, _data)
		{
			serialise_data(_serialiser, for_everys_map_key(datum));
			serialise_data(_serialiser, *datum);
		}
	}
	return true;
}

template <typename Key, typename Type>
bool serialise_data(Serialiser & _serialiser, Map<Key, Type> & _data)
{
	return serialise_inlined_map(_serialiser, _data);
}

bool serialise_plain_data(Serialiser & _serialiser, void * _data, uint _size);

// useful for enums if we do not expect enum's values to change, for example when they are all set to specific values anyway, and maybe even related to GL
#define SIMPLE_SERIALISER_AS_INT32(Type) template <> bool serialise_data<Type>(Serialiser & _serialiser, Type & _data) { int32 value = (int32)_data; bool retVal = serialise_data(_serialiser, value); _data = (Type)value; return retVal; }

// place in cpp
// currently there's no solution for when NumberType would change, but as I don't expect that would be an issue anywhere in close future, I leave it as it is
#define BEGIN_SERIALISER_FOR_ENUM(Type, NumberType) \
	template <> bool serialise_data<Type>(Serialiser & _serialiser, Type & _data) \
	{ \
		NumberType value = (NumberType)_data; \
		bool retVal = false; \
		if (_serialiser.is_reading()) { retVal = serialise_data(_serialiser, value); } \
		if (0) {}

#define ADD_SERIALISER_FOR_ENUM(SerialisedValue, UnserialisedValue) \
				else if (_serialiser.is_reading() && value == SerialisedValue) \
		{ \
			value = UnserialisedValue; \
		} \
				else if (_serialiser.is_writing() && value == UnserialisedValue) \
		{ \
			value = SerialisedValue; \
		}

#define END_SERIALISER_FOR_ENUM(Type) \
		else { error_stop(TXT("not recognised")); } \
		if (_serialiser.is_writing()) { retVal = serialise_data(_serialiser, value); } \
		if (_serialiser.is_reading()) { _data = (Type)value; } \
		return retVal; \
	}

DECLARE_SERIALISER_FOR(float);
DECLARE_SERIALISER_FOR(int);
DECLARE_SERIALISER_FOR(bool);

DECLARE_SERIALISER_FOR(int8);
DECLARE_SERIALISER_FOR(int16);
DECLARE_SERIALISER_FOR(int32);
DECLARE_SERIALISER_FOR(int64);

DECLARE_SERIALISER_FOR(uint8);
DECLARE_SERIALISER_FOR(uint16);
DECLARE_SERIALISER_FOR(uint32);
DECLARE_SERIALISER_FOR(uint64);
