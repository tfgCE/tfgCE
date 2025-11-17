#pragma once

#include "..\..\core\containers\arrayStatic.h"

namespace Framework
{
	class Room;
	class Region;

	struct WorldAddress
	{
	public:
		WorldAddress() { SET_EXTRA_DEBUG_INFO(address, TXT("WorldAddress.address")); }
		WorldAddress(WorldAddress const& _other) : address(_other.address) { SET_EXTRA_DEBUG_INFO(address, TXT("WorldAddress.address")); }

		WorldAddress& operator=(WorldAddress const& _other) { address = _other.address; return *this; }
		bool operator==(WorldAddress const& _other) const { return address == _other.address; }

		void build_for(Room const* _room);
		void build_for(Region const* _room);

		bool is_valid() const { return ! address.is_empty(); }

		bool parse(String const& _string);
		String to_string() const;

	private:
		ArrayStatic<int, 32> address;
	};
};

DECLARE_REGISTERED_TYPE(Framework::WorldAddress);
