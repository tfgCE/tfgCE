#pragma once

#include "network.h"
#include "../globalDefinitions.h"

#define NETWORK_PACKET_READ_OR_FAIL(_packet, _string) \
	if (!_packet.read(OUT_ _string)) \
	{ \
		return false; \
	}

#define NETWORK_PACKET_READ_OR_BREAK(_packet, _string) \
	if (!_packet.read(OUT_ _string)) \
	{ \
		break; \
	}

namespace Network
{
	struct Package;

	struct Packet
	{
	public:
		Packet();

		void go_to_start() { cursorAt = 0; }

		bool read(OUT_ String & _string);
		bool read(OUT_ float & _float);
		bool read(OUT_ int64 & _value);
		bool read(OUT_ int32 & _value);
		bool read(OUT_ int16 & _value);
		bool read(OUT_ int8 & _value);

		bool add(String const & _string);
		bool add(float const _float);
		bool add(int64 const _value);
		bool add(int32 const _value);
		bool add(int16 const _value);
		bool add(int8 const _value);

		bool receive_from(Package const & _package);
		void add_to_send(Package & _package) const;

	private:
		static const int32 MAX_LENGTH = 32768;
		int32 length;
		int32 cursorAt;
		char buffer[MAX_LENGTH];
		char const * readBuffer;
	};

};