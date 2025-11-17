#pragma once

#include "iWMPTool.h"
#include "..\random\randomNumber.h"

namespace WheresMyPoint
{
	class LoopIterator
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;
		override_ tchar const* development_description() const { return TXT("loop iterator, \"initVar\" or \"advanceVar\" inside is initial value or lessThan value"); }

	private:
		Name initVar;
		Name advanceVar;
		ToolSet toolSet; // value

		bool intValuePresent = false;
		int intValue = 0;
	};
};
