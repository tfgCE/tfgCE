#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class Bool
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		bool value = false;
	};

	class True
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};

	class False
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};

	class BoolRandom
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

		override_ tchar const* development_description() const { return TXT("[toolset] or [float value] chance"); }

	private:
		ToolSet toolSet;
		bool floatValuePresent = false;
		float floatValue = 0.0f;
	};
};
