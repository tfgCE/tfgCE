#include "digested.h"

#include "xml.h"
#include "..\serialisation\serialiser.h"
#include "memoryStorage.h"
#include "..\types\string.h"

using namespace IO;

Digested::Digested()
{
	memory_zero(value, c_length);
}

Digested::Digested(Interface* _toDigest)
{
	digest(_toDigest);
}

bool Digested::is_valid() const
{
	return !memory_compare_is_zero(value, c_length);
}

bool Digested::operator==(Digested const & _digested) const
{
	return memory_compare(value, _digested.value, c_length) && is_valid();
}

void Digested::digest(IO::XML::Node const * _node)
{
	MemoryStorage memoryStorage;
	memoryStorage.set_type(IO::InterfaceType::Text);
	_node->save_xml(&memoryStorage, false);
	memoryStorage.go_to(0);
	digest(&memoryStorage);
}

void Digested::digest(String const & _toDigest)
{
	MemoryStorage memoryStorage;
	memoryStorage.set_type(IO::InterfaceType::Text);
	memoryStorage.write_text(_toDigest.to_char());
	memoryStorage.go_to(0);
	digest(&memoryStorage);
}

void Digested::digest(Interface* _toDigest)
{
	if (!_toDigest)
	{
		memory_zero(value, c_length);
	}

	uint prevLoc = _toDigest->get_loc();

	memory_zero(value, c_length);
	int index = 0;
	int readAlready = 0;

	_toDigest->go_to(0);
	uint8 one;
	uint8 prev = 13;
	while (_toDigest->read(&one, 1) || readAlready < c_length)
	{
		value[index] = value[index] + (one - 83) + (prev & readAlready);
		prev = one;
		one += 26;
		index = (index + 1) % c_length;
		++readAlready;
	}

	_toDigest->go_to(prevLoc);
}

String Digested::to_string() const
{
	String retVal;
	tchar hex[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
	for_count(int, index, c_length)
	{
		uint8 val = value[index];
		retVal += hex[(val & 0xf0) >> 4];
		retVal += hex[(val & 0x0f)];
	}
	return retVal;
}

bool Digested::serialise(Serialiser & _serialiser)
{
	if (_serialiser.is_reading())
	{
		_serialiser.get_io()->read(value, c_length);
		return true;
	}
	else if (_serialiser.is_writing())
	{
		_serialiser.get_io()->write(value, c_length);
		return true;
	}
	todo_important(TXT("what should I do?"));
	return false;
}

void Digested::add(Digested const& _digested)
{
	uint8* srcV = value;
	uint8 const * addV = _digested.value;
	for_count(int, i, c_length)
	{
		*srcV = *srcV ^ *addV;
		++srcV;
		++addV;
	}
}
