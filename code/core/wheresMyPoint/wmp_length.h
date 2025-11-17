#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	/**
	 *	Calculate length of current value
	 */
	class Length
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};

	/**
	 *	Calculate length 2d of current value
	 */
	class Length2D
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};
};
