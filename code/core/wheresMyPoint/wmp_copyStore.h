#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	/**
	 *	Works as restore and store called one after another
	 *
	 *	<copyStore from="a" as="b"/>
	 *	<copyStore from="a" to="b"/>
	 *	<copyStore var="a"/>
	 */
	class CopyStore
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		bool onlyIfNotSet = false;
		bool mightBeMissing = false;
		Name from;
		Name as;
		ToolSet toolSet;
	};
};
