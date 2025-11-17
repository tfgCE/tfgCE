#include "netPackage.h"

using namespace Network;

Package::Package()
{
	clear();
}

void Package::clear()
{
	declaredSize = NONE;
	declaredSizeRead = 0;
	packageRead = 0;
	package.clear();
}

PackageOpResult::Type Package::receive(NetworkTCPSocket _socket)
{
	int readSoFar = 0;
	if (declaredSizeRead < sizeof(declaredSize))
	{
		int received = network_tcp_recv(_socket, &(((int8*)(&declaredSize))[declaredSizeRead]), sizeof(declaredSize) - declaredSizeRead);
		if (received == 0)
		{
			return PackageOpResult::Invalid;
		}
		readSoFar += received;
		declaredSizeRead += received;
		if (declaredSizeRead < sizeof(declaredSize))
		{
			return PackageOpResult::InProgress;
		}
		packageRead = 0;
		package.set_size(declaredSize);
	}
	if (packageRead < declaredSize)
	{
		int received = network_tcp_recv(_socket, &(package[packageRead]), declaredSize - packageRead);
		readSoFar += received;
		packageRead += received;
		if (packageRead < declaredSize)
		{
			return PackageOpResult::InProgress;
		}
		return PackageOpResult::Ok;
	}
	return readSoFar == 0 ? PackageOpResult::Invalid : PackageOpResult::Unknown;
}

PackageOpResult::Type Package::send(NetworkTCPSocket _socket)
{
	// try to send everything
	int sentSoFar = 0;
	int declaredSizeSent = 0;
	declaredSize = package.get_size();
	if (declaredSize > 0)
	{
		declaredSizeRead = sizeof(declaredSize);
		while (declaredSizeRead > declaredSizeSent)
		{
			int sent = network_tcp_send(_socket, &(((int8*)(&declaredSize))[declaredSizeSent]), declaredSizeRead - declaredSizeSent);
			if (sent == 0)
			{
				return PackageOpResult::Invalid;
			}
			sentSoFar += sent;
			declaredSizeSent += sent;
		}

		int packageSent = 0;
		while (declaredSize > packageSent)
		{
			int sent = network_tcp_send(_socket, &(package[packageSent]), declaredSize - packageSent);
			if (sent == 0)
			{
				return PackageOpResult::Invalid;
			}
			sentSoFar += sent;
			packageSent += sent;
		}
	}

	return PackageOpResult::Ok;
}

PackageOpResult::Type Package::send_and_clear(NetworkTCPSocket _socket)
{
	PackageOpResult::Type opResult = send(_socket);
	if (opResult == PackageOpResult::Ok)
	{
		clear();
	}
	return opResult;
}

void Package::receive_as(REF_ void const * & _data, REF_ int32 & _dataSize) const
{
	_data = &package[0];
	_dataSize = package.get_size();
}

void Package::add_to_send(void const * _data, int32 _dataSize)
{
	int prevSize = package.get_size();
	package.set_size(prevSize + _dataSize);
	memory_copy(&package[prevSize], _data, _dataSize);
}
