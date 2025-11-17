#pragma once

#include "iWMPTool.h"
#include "..\random\randomNumber.h"

namespace WheresMyPoint
{
	class Int
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		int value = 0;
	};

	class ToInt
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};

	class IntRandom
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		ToolSet toolSet;

		// both inclusive!
		bool minMaxDefined = false;
		int minValue = 0;
		int maxValue = 0;
		Random::Number<int> randomNumber;
	};
};
