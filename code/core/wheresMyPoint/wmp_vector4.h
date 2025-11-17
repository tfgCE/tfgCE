#pragma once

#include "iWMPTool.h"

#include "..\math\math.h"

namespace WheresMyPoint
{
	class Vector4
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		::Vector4 value = ::Vector4::zero;
		ToolSet toolSet;
	};

	class ToVector4
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};
};
