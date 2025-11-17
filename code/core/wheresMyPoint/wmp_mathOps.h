#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class MathOp
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	public:
		virtual bool handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const = 0;

	private:
		ToolSet toolSet;
		bool floatValuePresent = false;
		float floatValue = 0.0f;
	};

	class Add
	: public MathOp
	{
	public: // ITool (to handle addString)
		override_ bool load_from_xml(IO::XML::Node const* _node);
		override_ bool update(REF_ Value& _value, Context& _context) const;
	public: // MathOp
		override_ bool handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const;
	private:
		::String addString;
	};

	class Sub
	: public MathOp
	{
	public: // MathOp
		override_ bool handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const;
	};

	class Mul
	: public MathOp
	{
	public: // MathOp
		override_ bool handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const;
	};

	class Div
	: public MathOp
	{
	public: // MathOp
		override_ bool handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const;
	};

	class Max
	: public MathOp
	{
	public: // MathOp
		override_ bool handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const;
	};

	class Min
	: public MathOp
	{
	public: // MathOp
		override_ bool handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const;
	};

	class Mod
	: public MathOp
	{
	public: // MathOp
		override_ bool handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const;
	};

	class Round
	: public MathOp
	{
		typedef MathOp base;
	public: // MathOp
		override_ bool handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const;
	};

	class Tan
	: public MathOp
	{
		typedef MathOp base;
	public: // MathOp
		override_ bool handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const;
	};

	class Sin
	: public MathOp
	{
		typedef MathOp base;
	public: // MathOp
		override_ bool handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const;
	};

	class Cos
	: public MathOp
	{
		typedef MathOp base;
	public: // MathOp
		override_ bool handle_operation(REF_ Value & _value, Value const & _valueToOp, bool _valueToOpDefaulted) const;
	};
};
