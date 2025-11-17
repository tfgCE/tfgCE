#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class Include
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;
		override_ tchar const* development_description() const { return TXT("include float in Range\ninclude Vector2 in Range2\ninclude Vector3 in Range3"); }

	private:
		ToolSet toolSet;
	};
};
