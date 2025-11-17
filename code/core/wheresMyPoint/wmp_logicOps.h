#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class LogicOp
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	public:
		virtual bool should_handle_operation(REF_ Value & _value) const = 0;
		virtual bool handle_operation(REF_ Value & _value, Value const & _valueToOp) const = 0;

	private:
		ToolSet toolSet;
	};

	class And
	: public LogicOp
	{
	public: // LogicOp
		implement_ bool should_handle_operation(REF_ Value & _value) const;
		implement_ bool handle_operation(REF_ Value & _value, Value const & _valueToOp) const;
	};

	class Or
	: public LogicOp
	{
	public: // LogicOp
		implement_ bool should_handle_operation(REF_ Value & _value) const;
		implement_ bool handle_operation(REF_ Value & _value, Value const & _valueToOp) const;
	};

	class Xor
	: public LogicOp
	{
	public: // LogicOp
		implement_ bool should_handle_operation(REF_ Value & _value) const;
		implement_ bool handle_operation(REF_ Value & _value, Value const & _valueToOp) const;
	};

	class Not
	: public LogicOp
	{
	public: // LogicOp
		implement_ bool should_handle_operation(REF_ Value & _value) const;
		implement_ bool handle_operation(REF_ Value & _value, Value const & _valueToOp) const;
	};
};
