#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class MathComparison
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	public:
		virtual bool handle_comparison(REF_ Value & _value, Value const & _valueToCompare) const = 0;

	private:
		ToolSet toolSet;
		bool floatValuePresent = false;
		float floatValue = 0.0f;
	};

	class EqualTo
	: public MathComparison
	{
	public: // MathComparison
		override_ bool handle_comparison(REF_ Value & _value, Value const & _valueToCompare) const;
	};

	class LessThan
	: public MathComparison
	{
	public: // MathComparison
		override_ bool handle_comparison(REF_ Value & _value, Value const & _valueToCompare) const;
	};

	class LessOrEqual
	: public MathComparison
	{
	public: // MathComparison
		override_ bool handle_comparison(REF_ Value & _value, Value const & _valueToCompare) const;
	};

	class GreaterThan
	: public MathComparison
	{
	public: // MathComparison
		override_ bool handle_comparison(REF_ Value & _value, Value const & _valueToCompare) const;
	};

	class GreaterOrEqual
	: public MathComparison
	{
	public: // MathComparison
		override_ bool handle_comparison(REF_ Value & _value, Value const & _valueToCompare) const;
	};

};
