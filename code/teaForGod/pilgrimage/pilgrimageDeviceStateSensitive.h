#pragma once

#include "..\..\core\fastCast.h"

class SimpleVariableStorage;

namespace TeaForGodEmperor
{
	interface_class IPilgrimageDeviceStateSensitive
	{
		FAST_CAST_DECLARE(IPilgrimageDeviceStateSensitive);
		FAST_CAST_END();

	public:
		virtual ~IPilgrimageDeviceStateSensitive() {}

		virtual void on_restore_pilgrimage_device_state() = 0;
	};
};
