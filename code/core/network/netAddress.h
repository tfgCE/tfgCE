#pragma once

#include "network.h"

namespace Network
{
	class ServerTCP;
	class ClientTCP;

	struct Address
	{
	public:
		Address();
		Address(String const & _addressPort);
		Address(tchar const * _addr, uint16 _port);
		Address(String const & _addr, uint16 _port);

		static Address local(uint16 _port = 0);
		static Address server(uint16 _port = 0);

		bool is_ok() const { return isOk; }

		bool operator==(Address const& _other) const { return memory_compare(&ip, &_other.ip, sizeof(NetworkIPAddress)); }

	private:
		NetworkIPAddress ip;
		bool isOk;

		friend class ServerTCP;
		friend class ClientTCP;
	};
};
