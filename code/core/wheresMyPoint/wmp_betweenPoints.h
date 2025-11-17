#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class BetweenPoints
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		Name pointA;
		Name pointB;
		float at = 0.5f;
		ToolSet atToolSet;
	};
};
