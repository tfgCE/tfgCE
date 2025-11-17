#include "netPacket.h"
#include "netPackage.h"

#include "..\types\string.h"

using namespace Network;

Packet::Packet()
: length(0)
, cursorAt(0)
, readBuffer(nullptr)
{
}

bool Packet::read(OUT_ String & _string)
{
	int32 stringLength;
	if (cursorAt > length - (int32)sizeof(stringLength))
	{
		return false;
	}
	memory_copy(&stringLength, &(readBuffer?readBuffer : buffer)[cursorAt], sizeof(stringLength));
	cursorAt += sizeof(stringLength);
	if (cursorAt > length - stringLength)
	{
		return false;
	}
	_string.access_data().set_size(stringLength + 1);
	_string.access_data()[stringLength] = 0;

	memory_copy(_string.access_data().get_data(), &(readBuffer ? readBuffer : buffer)[cursorAt], stringLength * sizeof(tchar));
	cursorAt += stringLength;

	return true;
}

bool Packet::add(String const & _string)
{
	int32 stringLength = _string.get_length();
	if (length > MAX_LENGTH - (stringLength + (int32)sizeof(stringLength)))
	{
		return false;
	}
	memory_copy(&buffer[length], &stringLength, sizeof(stringLength));
	length += sizeof(stringLength);
	memory_copy(&buffer[length], _string.to_char(), stringLength * sizeof(tchar));
	length += stringLength;
	return true;
}

bool Packet::read(OUT_ float & _float)
{
	if (cursorAt > length - (int32)sizeof(float))
	{
		return false;
	}
	memory_copy(&_float, &(readBuffer ? readBuffer : buffer)[cursorAt], sizeof(float));
	cursorAt += sizeof(float);

	return true;
}

bool Packet::add(float const _float)
{
	if (length > MAX_LENGTH - (sizeof(float)))
	{
		return false;
	}
	memory_copy(&buffer[length], &_float, sizeof(float));
	length += sizeof(float);
	return true;
}

bool Packet::read(OUT_ int64 & _value)
{
	if (cursorAt > length - (int32)sizeof(int64))
	{
		return false;
	}
	memory_copy(&_value, &(readBuffer ? readBuffer : buffer)[cursorAt], sizeof(int64));
	cursorAt += sizeof(int64);

	return true;
}

bool Packet::add(int64 const _value)
{
	if (length > MAX_LENGTH - (sizeof(int64)))
	{
		return false;
	}
	memory_copy(&buffer[length], &_value, sizeof(int64));
	length += sizeof(int64);
	return true;
}

bool Packet::read(OUT_ int32 & _value)
{
	if (cursorAt > length - (int32)sizeof(int32))
	{
		return false;
	}
	memory_copy(&_value, &(readBuffer ? readBuffer : buffer)[cursorAt], sizeof(int32));
	cursorAt += sizeof(int32);

	return true;
}

bool Packet::add(int32 const _value)
{
	if (length > MAX_LENGTH - (sizeof(int32)))
	{
		return false;
	}
	memory_copy(&buffer[length], &_value, sizeof(int32));
	length += sizeof(int32);
	return true;
}

bool Packet::read(OUT_ int16 & _value)
{
	if (cursorAt > length - (int32)sizeof(int16))
	{
		return false;
	}
	memory_copy(&_value, &(readBuffer ? readBuffer : buffer)[cursorAt], sizeof(int16));
	cursorAt += sizeof(int16);

	return true;
}

bool Packet::add(int16 const _value)
{
	if (length > MAX_LENGTH - (sizeof(int16)))
	{
		return false;
	}
	memory_copy(&buffer[length], &_value, sizeof(int16));
	length += sizeof(int16);
	return true;
}

bool Packet::read(OUT_ int8 & _value)
{
	if (cursorAt > length - (int32)sizeof(int8))
	{
		return false;
	}
	memory_copy(&_value, &(readBuffer ? readBuffer : buffer)[cursorAt], sizeof(int8));
	cursorAt += sizeof(int8);

	return true;
}

bool Packet::add(int8 const _value)
{
	if (length > MAX_LENGTH - (sizeof(int8)))
	{
		return false;
	}
	memory_copy(&buffer[length], &_value, sizeof(int8));
	length += sizeof(int8);
	return true;
}

bool Packet::receive_from(Package const & _package)
{
	void const * rb;
	_package.receive_as(REF_ rb, REF_ length);
	readBuffer = (int8 const *)rb;
	return length > 0 && length <= MAX_LENGTH;
}

void Packet::add_to_send(Package & _package) const
{
	// we might be resending same packet
	_package.add_to_send(readBuffer? readBuffer : buffer, length);
}
