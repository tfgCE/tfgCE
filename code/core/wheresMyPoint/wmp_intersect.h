#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class Intersect
	: public ITool
	{
		typedef ITool base;
	public:
		virtual ~Intersect();

	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		Array<ITool*> intersecting;
	};
};
