#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class Lerp
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;
		override_ tchar const* development_description() const { return TXT("lerp \"value0\" and \"value1\" using \"at\""); }

	private:
		ToolSet value0ToolSet;
		ToolSet value1ToolSet;
		float at = 0.5f;
		ToolSet atToolSet;
	};
};
