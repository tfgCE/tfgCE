#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class Centre
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
		override_ tchar const* development_description() const { return TXT("get centre value of Range/Range2/Range3 as float/Vector2/Vector3"); }
	};
};
