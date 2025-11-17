#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class Normalise
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};
};
