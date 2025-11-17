#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	/**
	 *	Perform operations but keep pointer at value after ending this local block
	 *	Useful when preparing temp variables
	 */
	class Local
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		ToolSet toolSet;
	};
};
