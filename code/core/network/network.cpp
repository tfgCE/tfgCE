#include "network.h"

#include "..\types\string.h"

::Network::Network* ::Network::Network::s_current = nullptr;

void check_if_network_available()
{
	an_assert(Network::Network::get() != nullptr, TXT("call Network::Network::create"));
}

void ::Network::Network::create()
{
	an_assert(s_current == nullptr);
	s_current = new ::Network::Network();

	if (!s_current->isOk)
	{
		terminate();
	}
}

void ::Network::Network::terminate()
{
	if (s_current)
	{
		delete s_current;
	}
	s_current = nullptr;
}

::Network::Network::Network()
{
#ifdef AN_SDL_NETWORK
	isOk = SDLNet_Init() != -1;
#else
	isOk = true;
#endif
}

::Network::Network::~Network()
{
#ifdef AN_SDL_NETWORK
	SDLNet_Quit();
#endif
}

bool ::Network::Network::receive_string(NetworkTCPSocket _socket, OUT_ String & _string)
{
	uint32 length;
	uint32 result;
	
	result = network_tcp_recv(_socket, &length, sizeof(length));
	if (result != sizeof(length))
	{
		// closed or error
		return false;
	}

	length = network_swap_BE32(length);
	_string.access_data().set_size(length + 1);
	_string.access_data()[length] = 0;

	result = network_tcp_recv(_socket, _string.access_data().get_data(), length);
	if (result != length)
	{
		return false;
	}

	return true;
}
