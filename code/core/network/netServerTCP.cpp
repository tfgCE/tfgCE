#include "netServerTCP.h"
#include "netPacket.h"
#include "../containers/arrayStack.h"
#include "../debug/debug.h"

using namespace Network;

ServerTCP::ServerTCP(Address const & _address)
: waitIfNoClients(false)
, checkClientsActively(false)
, serverSocket(NONE_SOCKET)
, retryTime(0.0f)
{
	network_init_socket_set(listenSocketSet);
	address = _address;
#ifdef AN_SDL_NETWORK
	an_assert(address.ip.host == INADDR_NONE || address.ip.host == INADDR_ANY);
#endif
}

void ServerTCP::create()
{
	serverSocket = network_create_listen_socket(address.ip);

	if (serverSocket != NONE_SOCKET)
	{
		network_alloc_socket_set(listenSocketSet, 16);
		network_add_socket_to_set(listenSocketSet, serverSocket);
		on_client_connection_changed();
	}
}

ServerTCP::~ServerTCP()
{
	close();
}

void ServerTCP::close()
{
	network_remove_socket_from_set(listenSocketSet, serverSocket);
	network_free_socket_set(listenSocketSet);
	if (serverSocket != NONE_SOCKET)
	{
		network_close_socket(serverSocket);
		serverSocket = NONE_SOCKET;
	}
	for_every_ptr(client, clients)
	{
		delete client;
	}
	clients.clear();
	broadcastChannel.sockets.clear();
}

void ServerTCP::accept_client(AcceptIncomingConnection _acceptIncomingConnection)
{
	NetworkIPAddress clientAddr;
	NetworkTCPSocket acceptedSocket = network_accept_socket(serverSocket, clientAddr);
	if (acceptedSocket != NONE_SOCKET)
	{
#ifndef AN_SDL_NETWORK
		fcntl(acceptedSocket, F_SETFL, O_NONBLOCK);
#endif
#ifdef LOG_NETWORK
		output(TXT("network: new connection (server:%i)"), acceptedSocket);
#endif
		if (! checkClientsActively)
		{
			network_add_socket_to_set(listenSocketSet, acceptedSocket);
		}
		TwoWayChannel* client = new TwoWayChannel();
		client->socket = acceptedSocket;
		clients.push_back(client);
		broadcastChannel.sockets.push_back(acceptedSocket);
		_acceptIncomingConnection(acceptedSocket, clientAddr);
		on_client_connection_changed();
	}
}

void ServerTCP::close_client(NetworkTCPSocket _clientSocket, ClosingConnection _closingConnection)
{
#ifdef LOG_NETWORK
	output(TXT("network: connection closed (server:%i)"), _clientSocket);
#endif
#ifdef AN_SDL_NETWORK
	if (listenSocketSet)
#endif
	if (! checkClientsActively)
	{
		network_remove_socket_from_set(listenSocketSet, _clientSocket);
	}
	int idx = 0;
	for_every_ptr(client, clients)
	{
		if (client->socket == _clientSocket)
		{
			delete client;
			clients.remove_at(idx);
			if (_closingConnection)
			{
				_closingConnection(_clientSocket);
			}
			break;
		}
		++ idx;
	}
	broadcastChannel.sockets.remove(_clientSocket);
	on_client_connection_changed();
}

void ServerTCP::advance(float _deltaTime, HandleIncomingData _handleIncomingData, AcceptIncomingConnection _acceptIncomingConnection, ClosingConnection _closingConnection)
{
#ifdef AN_SDL_NETWORK
	if (!listenSocketSet)
#else
	if (serverSocket == NONE_SOCKET)
#endif
	{
		retryTime -= _deltaTime;
		if (retryTime < 0.0f)
		{
			retryTime += 1.0f;
			create();
		}
		return;
	}

	NetworkSocketSet readSockets;
#ifdef AN_SDL_NETWORK
	// accept incoming
	int socketsReady = SDLNet_CheckSockets(listenSocketSet, 0);
#else
	readSockets = listenSocketSet;
	timeval timeOut;
    timeOut.tv_sec = 0;
    timeOut.tv_usec = 0;
	// check clients actively, if there are no clients or it its not required, wait until something comes
	bool checkClientsActivelyNow = checkClientsActively && ! clients.is_empty();
	if (select(highestSocket + 1, &readSockets, nullptr, nullptr,
		checkClientsActivelyNow || ! waitIfNoClients? &timeOut : nullptr) == -1)
	{
		if (! checkClientsActivelyNow)
		{
			return;
		}
	}
	int socketsReady = clients.get_size() + 1;
#endif
	if (socketsReady > 0 && serverSocket != NONE_SOCKET)
	{
		if (network_is_socket_ready(readSockets, serverSocket))
		{
			--socketsReady;
			accept_client(_acceptIncomingConnection);
		}
	}
	if (socketsReady > 0)
	{
		ARRAY_STACK(NetworkTCPSocket, clientsToClose, clients.get_size());
		for_every_ptr(client, clients)
		{
			bool checkClient = false;
			if (! checkClientsActively)
			{
				checkClient = network_is_socket_ready(readSockets, client->socket);
			}
			else
			{
				#ifdef AN_SDL_NETWORK
					todo_important(TXT("MISSING!"));
				#else
					int result = network_is_anything_on_socket(client->socket);
					if (result == NETWORK_DATA_ON_SOCKET)
					{
						checkClient = true;
					}
					else if (result == NETWORK_UNKNOWN)
					{
						// just close this one
						clientsToClose.push_back(client->socket);
					}
				#endif
			}
			
			#ifdef AN_SDL_NETWORK
			if (checkClientsActively)
			{
				todo_important(TXT("MISSING!"));
			}
			if ((! checkClientsActively && network_is_socket_ready(readSockets, client->socket)))
			#else
			if ((! checkClientsActively && network_is_socket_ready(readSockets, client->socket)) ||
				(checkClientsActively && network_is_anything_on_socket(client->socket)))
			#endif
			{
				PackageOpResult::Type opResult = client->inbound.receive(client->socket);
				if (opResult == PackageOpResult::Ok)
				{
					_handleIncomingData(client->inbound, true);
					client->inbound.clear();
				}
				else if (opResult == PackageOpResult::Invalid ||
						 opResult == PackageOpResult::Unknown)
				{
					// if not, add socket to be removed
					clientsToClose.push_back(client->socket);
				}
				--socketsReady;
				if (socketsReady == 0)
				{
					break;
				}
			}
		}
		// close all clients with errors
		for_every(clientToClose, clientsToClose)
		{
			close_client(*clientToClose, _closingConnection);
		}
	}
}

void ServerTCP::add_to_send_to_all(Packet const & _packet)
{
	_packet.add_to_send(broadcastChannel.outbound);
}

void ServerTCP::add_to_send_to_individual(Packet const & _packet, NetworkTCPSocket _socket)
{
	for_every_ptr(client, clients)
	{
		_packet.add_to_send(client->outbound);
	}
}

void ServerTCP::send_pending(ClosingConnection _closingConnection)
{
	// first go individual
	{
		ARRAY_STACK(NetworkTCPSocket, clientsToClose, clients.get_size());
		for_every_ptr(client, clients)
		{
			PackageOpResult::Type opResult = client->outbound.send_and_clear(client->socket);
			if (opResult == PackageOpResult::Invalid ||
				opResult == PackageOpResult::Unknown)
			{
				clientsToClose.push_back(client->socket);
			}
		}
		// close all clients with errors
		for_every(clientToClose, clientsToClose)
		{
			close_client(*clientToClose, _closingConnection);
		}
	}

	// now all
	{
		ARRAY_STACK(NetworkTCPSocket, clientsToClose, broadcastChannel.sockets.get_size());
		for_every(clientSocket, broadcastChannel.sockets)
		{
			PackageOpResult::Type opResult = broadcastChannel.outbound.send(*clientSocket);
			if (opResult == PackageOpResult::Invalid ||
				opResult == PackageOpResult::Unknown)
			{
				clientsToClose.push_back(*clientSocket);
			}
		}
		broadcastChannel.outbound.clear();
		// close all clients with errors
		for_every(clientToClose, clientsToClose)
		{
			close_client(*clientToClose, _closingConnection);
		}
	}
}

void ServerTCP::on_client_connection_changed()
{
#ifndef AN_SDL_NETWORK
	highestSocket = serverSocket;
	if (! checkClientsActively)
	{
		for_every_ptr(client, clients)
		{
			highestSocket = max(highestSocket, client->socket);
		}
	}
#endif
}

