#pragma once

#include "bhj_enums.h"

namespace IO
{
	namespace JSON
	{
		class Node;
	};
};

namespace bhapticsJava
{
	struct BhapticsDevice
	{
		String deviceName;
		String address;
		int battery = 0;
		an_bhaptics::PositionType::Type position = an_bhaptics::PositionType::Vest;
		bool isConnected = false;
		bool isPaired = false;

		static void parse(String const & _devicesJSON, OUT_ Array<BhapticsDevice>& _devices);
		static void parse(IO::JSON::Node const* _json, OUT_ Array<BhapticsDevice>& _devices);
	};
};
