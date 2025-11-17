#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class VectorOp
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	protected:
		virtual void handle_operation(REF_ Value & _value, Value const & _otherValue) const = 0;

	private:
		ToolSet otherValueToolSet;
	};

	class Dot
	: public VectorOp
	{
	protected:
		implement_ void handle_operation(REF_ Value & _value, Value const & _otherValue) const;
	};

	class Cross
	: public VectorOp
	{
	protected:
		implement_ void handle_operation(REF_ Value & _value, Value const & _otherValue) const;
	};

	class DropUsing
	: public VectorOp
	{
	protected:
		implement_ void handle_operation(REF_ Value & _value, Value const & _otherValue) const;
	};

	class Along
	: public VectorOp
	{
	protected:
		implement_ void handle_operation(REF_ Value & _value, Value const & _otherValue) const;
	};

	//

	class IsZeroVector
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};

};
