#pragma once

#include "registeredType.h"

#define PACKED_BYTES_COUNT 32

struct PackedBytes
{
public:
	static PackedBytes const& empty();

	PackedBytes();

	int get_size() { return numElements; }
	void set_size(int _numElements);

	byte get(int _idx) const;
	void set(int _idx, byte _value);

	bool is_index_valid(int _idx) const { return _idx >= 0 && _idx < numElements; }

	static int max_size() { return PACKED_BYTES_COUNT; }

public:
	bool load_from_string(String const& _string);
	bool load_from_xml(IO::XML::Node const* _node);
	bool save_to_xml(IO::XML::Node* _node) const;

	String to_string() const;

private:
	int numElements = 0;
	byte bytes[PACKED_BYTES_COUNT];

};

DECLARE_REGISTERED_TYPE(PackedBytes);
