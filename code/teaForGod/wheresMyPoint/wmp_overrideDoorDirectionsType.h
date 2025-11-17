#pragma once

#include "..\..\core\types\dirFourClockwise.h"
#include "..\..\core\wheresMyPoint\wmp.h"

namespace TeaForGodEmperor
{
	struct MeshNode;

	class OverrideDoorDirectionsType
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const* _node);
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;
		override_ tchar const* development_description() const { return TXT("override door directions type (if set)"); }

	private:
		Name varName;
	};

};
