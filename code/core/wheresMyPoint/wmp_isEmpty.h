#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class IsEmpty
	: public ITool
	{
		typedef ITool base;
	public:
		static bool is_empty(Value const & _value, REF_ Value & _resultValue);

	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		ToolSet toolSet;
	};

	class IfIsEmpty
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		ToolSet doIfEmptyToolSet;
	};

	class IfIsNotEmpty
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		ToolSet doIfNotEmptyToolSet;
	};
};
