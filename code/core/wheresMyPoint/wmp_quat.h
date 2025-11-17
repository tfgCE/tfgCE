#pragma once

#include "iWMPTool.h"

#include "..\math\math.h"

namespace WheresMyPoint
{
	class Quat
	: public ITool
	{
		typedef ITool base;
	public:
		bool is_empty() const { return toolSet.is_empty() && value.is_zero(); }

	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		::Quat value = ::Quat::identity;
		ToolSet toolSet;
	};

	class ToQuat
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value& _value, Context& _context) const;
	};
};
