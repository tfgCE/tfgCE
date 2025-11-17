#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	/**
	 *	allows accessing sub field (returns whole variable)
	 *	if you want to get sub field outside, use "get"
	 */
	class Access
	: public IPrefixedTool
	{
		typedef IPrefixedTool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool load_prefixed_from_xml(IO::XML::Node const* _node, String const& _prefix);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		::Name name;
		ToolSet toolSet;
	};
};
