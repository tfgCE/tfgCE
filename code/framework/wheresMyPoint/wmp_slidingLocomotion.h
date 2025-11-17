#pragma once

#include "..\..\core\wheresMyPoint\wmp.h"

namespace Framework
{
	struct MeshNode;

	class SlidingLocomotion
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;
		override_ tchar const* development_description() const { return TXT("provides info if sliding locomotion is in use"); }
	};
};
