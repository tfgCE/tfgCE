#pragma once

#include "..\physicalSensationsSettings.h"

#include "..\..\types\string.h"

namespace an_bhaptics // airo noevl
{
	namespace PositionType
	{
		enum Type
		{
			Left,
			Right,
			Head,
			VestFront,
			VestBack,
			Vest,
			HandL,
			HandR,
			FootL,
			FootR,
			ForearmL,
			ForearmR,

			MAX
		};

		tchar const* to_char(Type _value);
		Type parse(String const& _value, Type _default = Vest);
	};
};
