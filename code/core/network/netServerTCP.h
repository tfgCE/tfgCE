#pragma once

#include "network.h"
#include "netAddress.h"
#include "netPackage.h"

namespace Network
{
	struct Packet;

	class ServerTCP
	{
	public:
		ServerTCP(Address const & _address);
		~ServerTCP();

		void will_check_clients_actively(bool _checkClientsActively = true) { checkClientsActively = _checkClientsActively; }
		void will_wait_if_no_clients(bool _waitIfNoClients = true) { waitIfNoClients = _waitIfNoClients; }
		
		void create();
		void close();

		void advance(float _deltaTime, HandleIncomingData _handleIncomingData, AcceptIncomingConnection _acceptIncomingConnection, ClosingConnection _closingConnection);
		void send_pending(ClosingConnection _closingConnection);

		void add_to_send_to_all(Packet const & _packet);
		void add_to_send_to_individual(Packet const & _packet, NetworkTCPSocket _socket);

	private:
		Address address;
		bool waitIfNoClients;
		bool checkClientsActively;
		NetworkTCPSocket serverSocket;
		NetworkSocketSet listenSocketSet;
		Array<TwoWayChannel*> clients;
#ifndef AN_SDL_NETWORK
		NetworkTCPSocket highestSocket;
#endif
		BroadcastChannel broadcastChannel;

		float retryTime;

		void accept_client(AcceptIncomingConnection _acceptIncomingConnection);
		void close_client(NetworkTCPSocket _clientSocket, ClosingConnection _closingConnection);
		
		void on_client_connection_changed();
	};
};
