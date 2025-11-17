#pragma once

#include "..\..\..\core\types\colour.h"

struct Ragne2;

namespace Framework
{
	class Display;
};

namespace Loader
{
	namespace HubInputFlags
	{
		enum Flag
		{
			Shift = 1
		};

		typedef int Flags;
	};

	struct HubWidgetUsedColours
	{
		Colour border = Colour::none;
		Colour inside = Colour::none;
		Colour textColour = Colour::none;
	};

	typedef std::function<void(Framework::Display* _display, Range2 const& _at, HubWidgetUsedColours const& _useColours)> HubCustomDrawFunc;
};

