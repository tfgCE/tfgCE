#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	/**
	 *	Allows to do something on true, false or both.
	 *	Input/current state has to be bool!
	 */
	class If
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	protected:
		ToolSet trueToolSet;
		ToolSet falseToolSet;
	};

	/**
	 *	As above, but only true.
	 */
	class IfTrue
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	protected:
		ToolSet trueToolSet;
	};

	/**
	 *	As above, but only false.
	 */
	class IfFalse
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	protected:
		ToolSet falseToolSet;
	};
};
