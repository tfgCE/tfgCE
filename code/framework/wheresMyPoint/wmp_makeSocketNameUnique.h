#pragma once

#include "..\..\core\wheresMyPoint\wmp.h"

namespace Framework
{
	/**
	 *	So it won't be the same as other sockets already generated
	 */
	class MakeSocketNameUnique
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;
		override_ tchar const* development_description() const { return TXT("make socket name unique by adding \"_%number%\" suffix"); }
	};
};
