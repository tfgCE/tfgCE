#include "netClientTCP.h"
#include "netPacket.h"
#include "..\containers\arrayStack.h"
#include "..\debug\debug.h"
#include "..\types\string.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Network;

//

ClientTCP::ClientTCP(Address const & _address)
: retryTime(0.0f)
, connectionFailed(false)
{
	network_init_socket_set(listenSocketSet);

	address = _address;
	//an_assert(address.ip.host == INADDR_NONE || address.ip.host == INADDR_ANY);
#ifdef AN_WINDOWS
	connectThreadHandle = 0;
#endif
}

void ClientTCP::connect()
{
	asClient.socket = network_create_connect_socket(address.ip);
	if (asClient.socket != NONE_SOCKET)
	{
		connectionFailed = false;
#ifdef LOG_NETWORK
		output(TXT("network: connected (%i)"), asClient.socket);
#endif
		network_alloc_socket_set(listenSocketSet, 1);
		network_add_socket_to_set(listenSocketSet, asClient.socket);
	}
	else
	{
		connectionFailed = true;
#ifdef AN_SDL_NETWORK
		String error = String::from_char8(SDLNet_GetError());
#ifdef LOG_NETWORK
		output(TXT("network: SDLNet_TCP_Open: %S\n"), error.to_char());
#endif
#else
#ifdef LOG_NETWORK
		output(TXT("network: socket open error\n"));
#endif
#endif
	}
}

ClientTCP::~ClientTCP()
{
#ifdef AN_WINDOWS
	if (connectThreadHandle != 0)
	{
		TerminateThread(connectThreadHandle, 0);
		CloseHandle(connectThreadHandle);
		connectThreadHandle = 0;
	}
#endif
	close();
}

void ClientTCP::close()
{
#ifdef LOG_NETWORK
	output(TXT("network: connection closed (%i)"), asClient.socket);
#endif
	network_free_socket_set(listenSocketSet);
	if (asClient.socket != NONE_SOCKET)
	{
		network_close_socket(asClient.socket);
		asClient.socket = NONE_SOCKET;
	}
}

#ifdef AN_WINDOWS
DWORD WINAPI connect_thread_func(LPVOID _client)
{
	// register thread, collect data, delete context and run
	ClientTCP* client = (ClientTCP*)_client;
	client->connect_from_thread();
	return 0;
}
#endif

void ClientTCP::connect_from_thread()
{
	connect();
#ifdef AN_WINDOWS
	connectThreadHandle = 0;
#endif
}

bool ClientTCP::advance(float _deltaTime, HandleIncomingData _handleIncomingData)
{
	if (! network_does_socket_set_exist(listenSocketSet))
	{
		retryTime -= _deltaTime;
		if (retryTime < 0.0f)
		{
			// run on separate thread to not lock client
#ifdef AN_WINDOWS
			if (connectThreadHandle == 0)
			{
				retryTime += 1.0f;
#ifdef LOG_NETWORK
				output(TXT("network: trying to connect..."));
#endif
				connectThreadHandle = CreateThread(nullptr, 0, connect_thread_func, this, 0, nullptr);
			}
			else if (connectThreadHandle != 0)
			{
				// still trying to connect
			}
			else
#endif
			{
				retryTime += 1.0f;
#ifdef LOG_NETWORK
				output(TXT("network: trying to connect..."));
#endif
				connect();
			}
		}
		if (!network_does_socket_set_exist(listenSocketSet))
		{
			return false;
		}
	}
	retryTime = 1.0f; // if we were disconnected
	// accept incoming data
	bool socketsReady = network_check_sockets(listenSocketSet);
	if (socketsReady && asClient.socket)
	{
		if (network_is_socket_ready(listenSocketSet, asClient.socket))
		{
			PackageOpResult::Type opResult = asClient.inbound.receive(asClient.socket);
			if (opResult == PackageOpResult::Ok)
			{
				_handleIncomingData(asClient.inbound, false);
				asClient.inbound.clear();
			}
			else if (opResult == PackageOpResult::Invalid ||
					 opResult == PackageOpResult::Unknown)
			{
				close();
			}
		}
	}
	return true;
}

void ClientTCP::add_to_send(Packet const & _packet)
{
	if (asClient.socket)
	{
		_packet.add_to_send(asClient.outbound);
	}
}

void ClientTCP::send_pending()
{
	if (asClient.socket)
	{
		PackageOpResult::Type opResult = asClient.outbound.send_and_clear(asClient.socket);
		if (opResult == PackageOpResult::Invalid ||
			opResult == PackageOpResult::Unknown)
		{
			close();
		}
	}
}

bool ClientTCP::recv_raw_8bit_text(OUT_ String& _string)
{
	Array<char8> recvString;
	recvString.make_space_for(8192);
	Array<char8> immediateBuffer;
	immediateBuffer.set_size(8192);
	while (true)
	{
		int recvAmount = network_tcp_recv(asClient.socket, immediateBuffer.get_data(), 8192);
		if (recvAmount == 0)
		{
			break;
		}
		char8 const* ch = immediateBuffer.get_data();
		for_count(int, i, recvAmount)
		{
			if (*ch == 0)
			{
				break;
			}
			recvString.push_back(*ch);
			++ch;
		}
	}
	recvString.push_back(0);
	_string = String::from_char8(recvString.get_data());
	return true;
}

bool ClientTCP::send_raw_8bit_text(String const& _string, bool _sendNullTerminator)
{
	auto char8array = _string.to_char8_array();
	int sent = network_tcp_send(asClient.socket, char8array.get_data(), char8array.get_data_size() + (_sendNullTerminator? 0 : -1));
	return sent != 0;
}

bool ClientTCP::send_raw_8bit_text(char const * _text, bool _sendNullTerminator)
{
	int sent = network_tcp_send(asClient.socket, _text, (int)strlen(_text) + (_sendNullTerminator? 1 : 0));
	return sent != 0;
}
