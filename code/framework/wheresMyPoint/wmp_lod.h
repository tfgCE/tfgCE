#pragma once

#include "..\..\core\wheresMyPoint\wmp.h"

namespace Framework
{
	struct MeshNode;

	class LOD
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const* _node);
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;

		override_ tchar const* development_description() const { return TXT("if \"range\" provided, will check against it (returns bool), if not returns current LOD value (int)"); }

	private:
		RangeInt range = RangeInt::empty; // if empty will provide current LOD value, if not empty will check and return false/true
	};

};
