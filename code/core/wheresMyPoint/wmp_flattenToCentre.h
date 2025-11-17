#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class FlattenToCentre
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
		override_ tchar const* development_description() const { return TXT("flatten Range/Range2/Range3 to the centre value"); }
	};
};
