#pragma once

#include "iWMPTool.h"
#include "..\random\randomNumber.h"

namespace WheresMyPoint
{
	class ChooseOne
		: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const* _node);
		override_ bool update(REF_ Value& _value, Context& _context) const;

	private:
		struct Element
		{
			float probCoef = 1.0f;
			ToolSet doToolSet;
		};

		Array<Element> elements;
	};
};

