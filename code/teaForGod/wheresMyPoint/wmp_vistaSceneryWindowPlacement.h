#pragma once

#include "..\..\core\wheresMyPoint\wmp.h"

namespace TeaForGodEmperor
{
	struct MeshNode;

	class VistaSceneryWindowPlacement // Transform
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;
		override_ tchar const* development_description() const { return TXT("gets window placement from vista scenery associated with the cell we're in"); }

	protected:
	};
};
