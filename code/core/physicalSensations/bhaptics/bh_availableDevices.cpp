#include "bh_availableDevices.h"

//

using namespace an_bhaptics;

//

void AvailableDevices::set(PositionType::Type _type, bool _setTo)
{
	if (_setTo)
	{
		set_flag(availableDevices, bit(_type));
	}
	else
	{
		clear_flag(availableDevices, bit(_type));
	}
}

bool AvailableDevices::has(PositionType::Type _type) const
{
	return is_flag_set(availableDevices, bit(_type));
}

String AvailableDevices::to_string() const
{
	String result;

	for_count(int, i, PositionType::MAX)
	{
		auto pt = PositionType::Type(i);

		if (has(pt))
		{
			result += TXT(" ");
			result += PositionType::to_char(pt);
		}
	}

	return result.trim();
}