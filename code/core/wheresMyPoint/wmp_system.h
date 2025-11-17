#pragma once

#include "iWMPTool.h"
#include "..\tags\tagCondition.h"

namespace WheresMyPoint
{
	class System
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value& _value, Context& _context) const;
		override_ tchar const* development_description() const { return TXT("system values"); }

	private:
		TagCondition systemTagRequired;
	};
};
