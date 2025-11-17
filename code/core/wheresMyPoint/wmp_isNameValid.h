#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	struct MeshNode;

	class IsNameValid
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;
	};
};
