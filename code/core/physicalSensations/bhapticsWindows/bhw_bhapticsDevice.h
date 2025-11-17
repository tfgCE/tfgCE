#pragma once

#include "bhw_settings.h"

#include "..\physicalSensationsSettings.h"

#include "..\bhaptics\bh_enums.h"

#include "..\..\types\string.h"

namespace IO
{
	namespace JSON
	{
		class Node;
	};
};

namespace bhapticsWindows
{
	struct BhapticsDevice
	{
		String deviceName;
		an_bhaptics::PositionType::Type position = an_bhaptics::PositionType::Vest;

		BhapticsDevice() {}
		BhapticsDevice(an_bhaptics::PositionType::Type _position);

		static void parse(String const & _devicesJSON, OUT_ Array<BhapticsDevice>& _devices);
		static void parse(IO::JSON::Node const* _json, OUT_ Array<BhapticsDevice>& _devices);
	};
};
