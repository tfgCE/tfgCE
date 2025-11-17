#pragma once

#include <functional>

#include "../globalDefinitions.h"
#include "../debug/debug.h"
#include "../memory/memory.h"

#ifdef AN_ALLOW_EXTENSIVE_LOGS
// #define LOG_NETWORK
#endif

struct String;

void check_if_network_available();

#ifdef AN_SDL_NETWORK
	#include "SDL_net.h"

	#define NONE_SOCKET nullptr

	typedef SDLNet_SocketSet NetworkSocketSet;
	typedef TCPsocket NetworkTCPSocket;
	typedef IPaddress NetworkIPAddress;
	
	inline bool network_resolve_host(NetworkIPAddress * _ip, char const * _address, int _port)
	{
		check_if_network_available();
		return SDLNet_ResolveHost(_ip, _address, _port) == 0;
	}
	
	inline uint32 network_tcp_recv(NetworkTCPSocket _socket, void* _at, uint32 _sizeBytes)
	{
		check_if_network_available();
		int result = SDLNet_TCP_Recv(_socket, _at, _sizeBytes);
		if (result <= 0 || result > (int)_sizeBytes)
		{
			return 0;
		}
		else
		{
			return (uint32)result;
		}
	}

	inline uint32 network_tcp_send(NetworkTCPSocket _socket, void const * _at, uint32 _sizeBytes)
	{
		check_if_network_available();

		int result = SDLNet_TCP_Send(_socket, _at, _sizeBytes);
		if (result <= 0 || result > (int)_sizeBytes)
		{
			return 0;
		}
		else
		{
			return (uint32)result;
		}
	}

	inline uint32 network_swap_BE32(uint32 _data)
	{
		return SDL_SwapBE32(_data);
	}
	
	inline uint16 network_swap_BE16(uint16 _data)
	{
		return SDL_SwapBE16(_data);
	}

	inline bool network_does_socket_set_exist(NetworkSocketSet& _socketSet)
	{
		return _socketSet != nullptr;
	}
	
	inline void network_init_socket_set(NetworkSocketSet& _socketSet)
	{
		_socketSet = nullptr;
	}

	inline void network_alloc_socket_set(NetworkSocketSet& _socketSet, int _count)
	{
		check_if_network_available();
		_socketSet = SDLNet_AllocSocketSet(_count);
	}

	inline void network_free_socket_set(NetworkSocketSet& _socketSet)
	{
		if (_socketSet != nullptr)
		{
			check_if_network_available();
			SDLNet_FreeSocketSet(_socketSet);
			_socketSet = nullptr;
		}
	}

	inline void network_add_socket_to_set(NetworkSocketSet & _socketSet, NetworkTCPSocket & _socket)
	{
		check_if_network_available();
		SDLNet_TCP_AddSocket(_socketSet, _socket);
	}

	inline void network_remove_socket_from_set(NetworkSocketSet & _socketSet, NetworkTCPSocket & _socket)
	{
		check_if_network_available();
		SDLNet_TCP_DelSocket(_socketSet, _socket);
	}

	inline bool network_is_socket_ready(NetworkSocketSet & _socketSet, NetworkTCPSocket & _socket)
	{
		check_if_network_available();
		return SDLNet_SocketReady(_socket) != 0;
	}

	inline NetworkTCPSocket network_create_connect_socket(NetworkIPAddress& _address)
	{
		check_if_network_available();
		return SDLNet_TCP_Open(&_address);
	}

	inline NetworkTCPSocket network_create_listen_socket(NetworkIPAddress& _address)
	{
		check_if_network_available();
		return SDLNet_TCP_Open(&_address);
	}

	inline NetworkTCPSocket network_accept_socket(NetworkTCPSocket & _socket, NetworkIPAddress & _address)
	{
		check_if_network_available();
		return SDLNet_TCP_Accept(_socket);
	}

	inline void network_close_socket(NetworkTCPSocket & _socket)
	{
		check_if_network_available();
		SDLNet_TCP_Close(_socket);
	}
	
	inline bool network_check_sockets(NetworkSocketSet& _socketSet)
	{
		return SDLNet_CheckSockets(_socketSet, 0) > 0;
	}

#else
	#include <stdio.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <errno.h>
	#include <fcntl.h>

	#define NONE_SOCKET NONE

	#ifndef __FDSET_LONGS
	#define __FDSET_LONGS FD_SETSIZE / NFDBITS
	#endif

	typedef fd_set NetworkSocketSet;
	typedef int NetworkTCPSocket;
	typedef sockaddr_in NetworkIPAddress;

	inline bool network_resolve_host(NetworkIPAddress * _ip, char const * _address, uint16 _port)
	{
		check_if_network_available(); // not really required but kept to be symmetrical

		addrinfo hints, *servinfo;
		int rv;
	 
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
		hints.ai_socktype = SOCK_STREAM;
	 
		if ((rv = getaddrinfo(_address, nullptr, &hints, &servinfo)) != 0) 
		{
			return false;
		}
	 
		// loop through all the results and connect to the first we can
		for (addrinfo *p = servinfo; p != nullptr; p = p->ai_next) 
		{
			sockaddr_in *h = (sockaddr_in *) p->ai_addr;
			*_ip = *h;
			_ip->sin_port = htons(_port);
			freeaddrinfo(servinfo);
			return true;
		}
		 
		freeaddrinfo(servinfo);
		return false;
	}

	inline uint32 network_tcp_recv(NetworkTCPSocket _socket, void* _at, uint32 _sizeBytes)
	{
		check_if_network_available(); // not really required but kept to be symmetrical

		int result = recv(_socket, _at, _sizeBytes, 0);
		if (result <= 0 || result > (int)_sizeBytes)
		{
			return 0;
		}
		else
		{
			return (uint32)result;
		}
	}

	#define NETWORK_NOTHING_ON_SOCKET 0
	#define NETWORK_DATA_ON_SOCKET 1
	#define NETWORK_UNKNOWN -1
	
	inline int network_is_anything_on_socket(NetworkTCPSocket _socket)
	{
		check_if_network_available(); // not really required but kept to be symmetrical

		char oneByte;
		int result = recv(_socket, &oneByte, 1, MSG_DONTWAIT | MSG_PEEK);
		if (result == 1)
		{
			return NETWORK_DATA_ON_SOCKET;
		}
		else
		{
			result = errno;
			if (result == EWOULDBLOCK)
			{
				return NETWORK_NOTHING_ON_SOCKET;
			}
			else
			{
				return NETWORK_UNKNOWN;
			}
		}
	}
	
	inline uint32 network_tcp_send(NetworkTCPSocket _socket, void const * _at, uint32 _sizeBytes)
	{
		check_if_network_available(); // not really required but kept to be symmetrical

		int result = sendto(_socket, _at, _sizeBytes, MSG_NOSIGNAL, nullptr, 0);
		if (result <= 0 || result > (int)_sizeBytes)
		{
			return 0;
		}
		else
		{
			return (uint32)result;
		}
	}

	inline uint32 network_swap_BE32(uint32 _data)
	{
		return htonl(_data);
	}
	
	inline uint16 network_swap_BE16(uint16 _data)
	{
		return htons(_data);
	}

	inline bool network_does_socket_set_exist(NetworkSocketSet& _socketSet)
	{
		for(int i = 0; i < __FDSET_LONGS; ++i)
		{
			if (_socketSet.fds_bits[i] != 0)
			{
				return true;
			}
		}
		return false;
	}

	inline void network_init_socket_set(NetworkSocketSet& _socketSet)
	{
		check_if_network_available(); // not really required but kept to be symmetrical

		FD_ZERO(&_socketSet);
	}

	inline void network_alloc_socket_set(NetworkSocketSet& _socketSet, int _count)
	{
		check_if_network_available(); // not really required but kept to be symmetrical

		FD_ZERO(&_socketSet);
	}

	inline void network_free_socket_set(NetworkSocketSet& _socketSet)
	{
		check_if_network_available(); // not really required but kept to be symmetrical

		FD_ZERO(&_socketSet);
	}

	inline void network_add_socket_to_set(NetworkSocketSet & _socketSet, NetworkTCPSocket & _socket)
	{
		check_if_network_available(); // not really required but kept to be symmetrical

		FD_SET(_socket, &_socketSet);
	}

	inline void network_remove_socket_from_set(NetworkSocketSet & _socketSet, NetworkTCPSocket & _socket)
	{
		check_if_network_available(); // not really required but kept to be symmetrical

		FD_CLR(_socket, &_socketSet);
	}

	inline bool network_is_socket_ready(NetworkSocketSet & _socketSet, NetworkTCPSocket & _socket)
	{
		check_if_network_available(); // not really required but kept to be symmetrical

		return FD_ISSET(_socket, &_socketSet);
	}

	inline NetworkTCPSocket network_create_connect_socket(NetworkIPAddress& _address)
	{
		check_if_network_available(); // not really required but kept to be symmetrical

		NetworkTCPSocket resultSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (resultSocket != NONE_SOCKET)
		{
			//int yesPlease = 1;
			//setsockopt(resultSocket, SOL_SOCKET, SO_REUSEADDR, &yesPlease, sizeof(int));
			//setsockopt(resultSocket, SOL_SOCKET, SO_NOSIGPIPE, &yesPlease, sizeof(int));
			NetworkIPAddress address = _address;
			memory_zero(&address.__pad, sizeof(address.__pad));
			if (connect(resultSocket, (struct sockaddr*) &address, sizeof(address)) < 0)
			{
#ifdef LOG_NETWORK
				output(TXT("network: ! connect"));
#endif
				return NONE_SOCKET;
			}
		}
		else
		{
#ifdef LOG_NETWORK
			output(TXT("network: ! resultSocket"));
#endif
			return NONE_SOCKET;
		}
		return resultSocket;
	}

	inline NetworkTCPSocket network_create_listen_socket(NetworkIPAddress& _address)
	{
		check_if_network_available(); // not really required but kept to be symmetrical

		NetworkTCPSocket resultSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (resultSocket != NONE_SOCKET)
		{
			int yesPlease = 1;
			setsockopt(resultSocket, SOL_SOCKET, SO_REUSEADDR, &yesPlease, sizeof(int));
			//setsockopt(resultSocket, SOL_SOCKET, SO_NOSIGPIPE, &yesPlease, sizeof(int));
			if (bind(resultSocket, (struct sockaddr*) & _address, sizeof(NetworkIPAddress)) == -1)
			{
#ifdef LOG_NETWORK
				output(TXT("network: ! bind"));
#endif
			}
			if (listen(resultSocket, 10) == -1)
			{
#ifdef LOG_NETWORK
				output(TXT("network: ! listen"));
#endif
			}
		}
		else
		{
#ifdef LOG_NETWORK
			output(TXT("network: ! resultSocket"));
#endif
		}
		return resultSocket;
	}
	
	inline NetworkTCPSocket network_accept_socket(NetworkTCPSocket& _socket, NetworkIPAddress& _address)
	{
		check_if_network_available(); // not really required but kept to be symmetrical

		socklen_t addrLen = sizeof(_address);
		return accept(_socket, (struct sockaddr *)&_address, &addrLen);
	}
	
	inline void network_close_socket(NetworkTCPSocket & _socket)
	{
		check_if_network_available(); // not really required but kept to be symmetrical

		close(_socket);
	}

	inline bool network_check_sockets(NetworkSocketSet& _socketSet)
	{
		struct timeval tv;
		tv.tv_sec = 5; // five seconds?
		tv.tv_usec = 0;

		int result = select(1, &_socketSet, NULL, NULL, &tv);
		return result != -1 && result != 0;
	}

#endif

#define NETWORK_RECEIVE_STRING_OR_FAIL(_socket, _string) \
	if (!Network::Network::receive_string(_socket, REF_ _string)) \
	{ \
		return false; \
	}

namespace Network
{
	struct Package;

	typedef std::function< void(Package const & _package, bool _server) > HandleIncomingData;
	typedef std::function< bool(NetworkTCPSocket _socket, NetworkIPAddress const & _address) > AcceptIncomingConnection;
	typedef std::function< void(NetworkTCPSocket _socket) > ClosingConnection;

	class Network
	{
		friend class Core;

	public:

		static bool receive_string(NetworkTCPSocket _socket, OUT_ String & _string);

	public:
		static void create();
		static void terminate();
		static Network* get() { return s_current; }

	private:
		static Network* s_current;

		bool isOk;

		Network();
		~Network();
	};
};
