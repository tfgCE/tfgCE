#include "packedBytes.h"

#include "..\math\math.h"

//

static int hex_to_int(tchar _ch)
{
	if (_ch >= '0' && _ch <= '9')
	{
		return _ch - '0';
	}
	if (_ch >= 'a' && _ch <= 'f')
	{
		return _ch - 'a' + 10;
	}
	if (_ch >= 'A' && _ch <= 'F')
	{
		return _ch - 'A' + 10;
	}
	return 0;
}

static tchar int_to_hex(int _v)
{
	if (_v >= 0 && _v <= 9)
	{
		return '0' + _v;
	}
	if (_v >= 10 && _v <= 15)
	{
		return 'a' + _v - 10;
	}
	an_assert(false, TXT("int value out of range"));
	return '?';
}

//

PackedBytes emptyPackedBytes;

PackedBytes const& PackedBytes::empty() { return emptyPackedBytes; }

PackedBytes::PackedBytes()
{
}

void PackedBytes::set_size(int _numElements)
{
	an_assert(_numElements <= PACKED_BYTES_COUNT);
	_numElements = clamp(_numElements, 0, PACKED_BYTES_COUNT);

	if (numElements < _numElements)
	{
		memory_zero(&bytes[numElements], (_numElements - numElements) * sizeof(byte));
	}
	
	numElements = _numElements;
}

byte PackedBytes::get(int _idx) const
{
	an_assert(_idx >= 0 && _idx < numElements);
	return bytes[_idx];
}

void PackedBytes::set(int _idx, byte _value)
{
	an_assert(_idx >= 0 && _idx < numElements);
	bytes[_idx] = _value;
}

bool PackedBytes::load_from_string(String const& _string)
{
	// two chars per byte, hex
	int count = _string.get_length() / 2;

	// clear
	set_size(0);
	set_size(count);

	for_count(int, i, numElements)
	{
		tchar higher = _string[i * 2];
		tchar lower = _string[i * 2 + 1];

		int higherValue = hex_to_int(higher);
		int lowerValue = hex_to_int(lower);

		set(i, (byte)(higherValue * 16 + lowerValue));
	}

	return true;
}

bool PackedBytes::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	result &= load_from_string(_node->get_string_attribute(TXT("hex")));

	return result;
}

bool PackedBytes::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	String hexString;
	for_count(int, i, numElements)
	{
		int val = get(i);
		int higherValue = (val & 0xf0) >> 4;
		int lowerValue = (val & 0x0f);

		hexString += int_to_hex(higherValue);
		hexString += int_to_hex(lowerValue);
	}

	return result;
}

String PackedBytes::to_string() const
{
	String asString;
	for_count(int, i, numElements)
	{
		if (!asString.is_empty())
		{
			asString += TXT(", ");
		}

		asString += String::printf(TXT("i"), (int)get(i));
	}
	return asString;
}
