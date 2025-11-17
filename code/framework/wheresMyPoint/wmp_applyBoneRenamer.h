#pragma once

#include "..\..\core\wheresMyPoint\wmp.h"

namespace Framework
{
	class ApplyBoneRenamer
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;
	};
};
