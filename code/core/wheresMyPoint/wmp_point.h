#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class Point
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		Name pointId;
	};
};
