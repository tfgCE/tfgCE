#pragma once

#include "iWMPTool.h"
#include "..\random\randomNumber.h"

namespace WheresMyPoint
{
	class Float
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

		bool is_empty() const { return toolSet.is_empty() && ! value.is_set(); }

	private:
		ToolSet toolSet;

		Optional<float> value;
	};

	class ToFloat
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};

	class FloatRandom
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

		override_ tchar const* development_description() const { return TXT("[min=, max=] or [random number definition] random value from the range"); }

	private:
		ToolSet toolSet;

		bool minMaxDefined = false;
		float minValue = 0.0f;
		float maxValue = 0.0f;
		Random::Number<float> randomNumber;
	};
};
