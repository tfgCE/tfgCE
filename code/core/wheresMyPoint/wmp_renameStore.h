#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class RenameStore
	: public IPrefixedTool
	{
		typedef IPrefixedTool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;
		override_ tchar const* development_description() const { return TXT("rename value in store \"from\" \"to\""); }

	private:
		::Name nameFrom;
		::Name nameTo;
	};
};
