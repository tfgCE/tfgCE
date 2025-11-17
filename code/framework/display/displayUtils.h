#pragma once

#include "..\..\core\types\name.h"
#include "..\..\core\types\optional.h"

namespace Framework
{
	class Display;

	class DisplayUtils
	{
	public:
		static void clear_all(Display* _display, Optional<Name> const & _colourPair = NP);
		static void cls(Display* _display, Optional<Name> const & _colourPair = NP);
		static void border(Display* _display, Optional<Name> const & _colourPair = NP);
	};
};
