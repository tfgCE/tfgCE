#pragma once

#include "wmp_float.h"

namespace WheresMyPoint
{
	class GetPt
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const* _node);
		override_ bool update(REF_ Value& _value, Context& _context) const;

		bool is_empty() const { return toolSet.is_empty() && !value.is_set(); }
	private:
		ToolSet toolSet;

		Optional<float> value;

		template <typename Class>
		bool update_value(REF_ Value& _value, Context& _context) const;
	};
};
