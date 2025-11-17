#pragma once

#include "..\..\core\wheresMyPoint\wmp.h"

namespace Framework
{
	/**
	 *	Current value has to be name of bone to get
	 */
	class BonePlacement
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;
	};
};
