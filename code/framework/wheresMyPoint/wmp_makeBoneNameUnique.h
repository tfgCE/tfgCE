#pragma once

#include "..\..\core\wheresMyPoint\wmp.h"

namespace Framework
{
	/**
	 *	So it won't be the same as other bones already generated
	 */
	class MakeBoneNameUnique
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;
	};
};
