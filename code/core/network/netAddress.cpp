#include "netAddress.h"

#include "../other/parserUtils.h"
#include "../types/string.h"

using namespace Network;

#define DEFAULT_ADDR "127.0.0.1"
#define DEFAULT_PORT 6547

Address::Address()
{
#ifdef AN_SDL_NETWORK
	ip.host = SDL_SwapBE32(INADDR_NONE);
	ip.port = SDL_SwapBE16(DEFAULT_PORT);
#else
	ip.sin_family = AF_INET;
	ip.sin_addr.s_addr = INADDR_NONE;
	ip.sin_port = htons(DEFAULT_PORT);
#endif
	isOk = true;
}

Address Address::server(uint16 _port)
{
	Address result;
#ifdef AN_SDL_NETWORK
	result.ip.host = SDL_SwapBE32(INADDR_NONE);
	result.ip.port = SDL_SwapBE16(_port != 0 ? _port : DEFAULT_PORT);
#else
	result.ip.sin_family = AF_INET;
	result.ip.sin_addr.s_addr = htonl(INADDR_ANY);
	result.ip.sin_port = htons(_port != 0 ? _port : DEFAULT_PORT);
#endif
	result.isOk = true;
	return result;
}

Address Address::local(uint16 _port)
{
	Address result;
	_port = _port != 0 ? _port : DEFAULT_PORT;
#ifdef AN_SDL_NETWORK
	if (SDLNet_GetLocalAddresses(&result.ip, 1) == 1)
	{
		result.ip.port = SDL_SwapBE16(_port);
		result.isOk = true;
	}
	else
	{
		result.isOk = SDLNet_ResolveHost(&result.ip, "127.0.0.1", _port) == 0;
	}
#else
	result.ip.sin_family = AF_INET;
	result.ip.sin_addr.s_addr = INADDR_ANY;
	result.ip.sin_port = htons(_port);
#endif
	return result;
}

Address::Address(tchar const * _addr, uint16 _port)
{
	an_assert(_addr != nullptr);
	String addr = String(_addr);
	isOk = network_resolve_host(&ip, addr.to_char8_array().get_data(), _port);
}

Address::Address(String const & _addr, uint16 _port)
{
	an_assert(! _addr.is_empty());
	isOk = network_resolve_host(&ip, _addr.to_char8_array().get_data(), _port);
}

Address::Address(String const & _addressPort)
{
	int separatorAt = _addressPort.find_first_of(':');
	uint16 port;
	if (separatorAt != NONE)
	{
		port = ParserUtils::parse_int(_addressPort.get_sub(separatorAt + 1, DEFAULT_PORT));
	}
	else
	{
		port = DEFAULT_PORT;
	}

	if (_addressPort.is_empty())
	{
		isOk = network_resolve_host(&ip, DEFAULT_ADDR, DEFAULT_PORT);
	}
	else if (separatorAt == NONE)
	{
		isOk = network_resolve_host(&ip, _addressPort.to_char8_array().get_data(), port);
	}
	else
	{
		isOk = network_resolve_host(&ip, _addressPort.get_left(separatorAt).to_char8_array().get_data(), port);
	}
}
