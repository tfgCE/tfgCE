#pragma once

#include "network.h"
#include "..\containers\array.h"

namespace Network
{
	namespace PackageOpResult
	{
		enum Type
		{
			Unknown,
			InProgress, // package in progress - didn't finish
			Ok, // finished
			Invalid, // didn't read/sent anything
		};
	};

	struct Package
	{
	public:
		Package();

		void clear();

		void receive_as(REF_ void const * & _data, REF_ int32 & _dataSize) const; // this pointer has to be used before package is cleared!
		void add_to_send(void const * _data, int32 _dataSize);

		PackageOpResult::Type receive(NetworkTCPSocket _socket); // returns true if package is filled
		PackageOpResult::Type send(NetworkTCPSocket _socket);
		PackageOpResult::Type send_and_clear(NetworkTCPSocket _socket);

		bool is_ok_to_read() const { return sizeof(declaredSize) == declaredSizeRead && declaredSize == packageRead; }

	private:
		int32 declaredSize; // NONE if not declared
		int32 declaredSizeRead; // 0 if not read
		int32 packageRead;
		Array<int8> package;
	};

	struct TwoWayChannel
	{
	public:
		NetworkTCPSocket socket;
		Package inbound;
		Package outbound;

		TwoWayChannel()
		: socket(NONE_SOCKET)
		{
		}
	};

	struct BroadcastChannel
	{
	public:
		Array<NetworkTCPSocket> sockets;
		Package outbound;
	};
};