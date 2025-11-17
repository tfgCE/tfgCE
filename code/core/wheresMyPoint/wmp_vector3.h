#pragma once

#include "iWMPTool.h"

#include "..\math\math.h"

namespace WheresMyPoint
{
	class Vector3
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		::Vector3 value = ::Vector3::zero;
		ToolSet toolSet;
	};

	class ToVector3
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};
};
