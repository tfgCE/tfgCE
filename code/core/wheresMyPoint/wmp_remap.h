#pragma once

#include "iWMPTool.h"

#include "..\math\math.h"

namespace WheresMyPoint
{
	class Remap
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		float from0 = 0.0f;
		float from1 = 1.0f;
		float to0 = 0.0f;
		float to1 = 1.0f;
		
		bool clampValue = true;

		ToolSet from0ToolSet;
		ToolSet from1ToolSet;
		ToolSet to0ToolSet;
		ToolSet to1ToolSet;

		static bool load_into(IO::XML::Node const* _node, tchar const* _name, REF_ float& _into0, REF_ float& _into1);
		static bool load_into(IO::XML::Node const* _node, tchar const* _name, REF_ float& _into, REF_ ToolSet& _intoToolset);
		bool run_for(REF_ float& _val, REF_ ToolSet const & _toolSet, Context& _context) const;
	};
};
