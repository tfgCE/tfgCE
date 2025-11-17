#pragma once

#include "network.h"
#include "netAddress.h"
#include "netPackage.h"

#ifdef AN_WINDOWS
#include <Windows.h>
#endif

namespace Network
{
	struct Packet;

	class ClientTCP
	{
	public:
		ClientTCP(Address const & _address);
		~ClientTCP();

		void connect();
		void close();

		bool advance(float _deltaTime, HandleIncomingData _handleIncomingData);
		void send_pending();

		void add_to_send(Packet const & _packet);

		void connect_from_thread();

		bool has_connection_failed() const { return connectionFailed; }

		NetworkTCPSocket get_socket() const { return asClient.socket; }

		bool recv_raw_8bit_text(OUT_ String& _string);
		bool send_raw_8bit_text(String const& _string, bool _sendNullTerminator = true);
		bool send_raw_8bit_text(char const * _text, bool _sendNullTerminator = true);

	private:
		Address address;
		TwoWayChannel asClient;
		NetworkSocketSet listenSocketSet;

		float retryTime;
		bool connectionFailed;

#ifdef AN_WINDOWS
		HANDLE connectThreadHandle;
#endif
	};
};
