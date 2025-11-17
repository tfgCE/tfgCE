#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class DefaultValue
	: public ITool
	{
		typedef ITool base;

	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

		override_ tchar const* development_description() const { return TXT("provide default value for a variable\nby default, if not exist\nadd \"ifEmpty\" bool parameter to also check if value is empty\nadd \"onlyIfDoesntExist\" bool parameter to also check only for non existent values"); }

	private:
		Name forVar;
		bool ifEmpty = true; // if not set, will only check if exists
		ToolSet toolSet;
	};
};
