#pragma once

#include "..\..\core\tags\tagCondition.h"
#include "..\..\core\wheresMyPoint\wmp.h"
#include "..\..\framework\library\usedLibraryStored.h"

namespace TeaForGodEmperor
{
	struct MeshNode;
	class Pilgrimage;

	class PilgrimageInstanceForWheresMyPoint
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const* _node);
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;
		override_ tchar const* development_description() const { return TXT("checks current/next pilgrimage instance"); }

	private:
		Framework::UsedLibraryStored<Pilgrimage> currentOrNext;
	};

};
