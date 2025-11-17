#pragma once

#include "iWMPTool.h"
#include "..\random\randomNumber.h"

namespace WheresMyPoint
{
	class UniqueInt
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
		override_ tchar const* development_description() const { return TXT("provides unique int value"); }
	};
};
