#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class CustomTool
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	protected:
		struct Parameter
		{
			enum Store
			{
				Normal,
				Global,
				Temp,
			};
			Store store = Store::Normal;
			Name valueNameInStore;
			Name parameterName;
		};
		Name customToolName;
		mutable CACHED_ RegisteredCustomTool const * customTool = nullptr;
		Array<Parameter> inputParameters;
		Array<Parameter> outputParameters;

		override_ bool update_using(REF_ Value& _value, Context& _context, RegisteredCustomTool const* _customTool) const;
	};

	class CallCustomTool
	: public CustomTool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value& _value, Context& _context) const;

	protected:
		ToolSet toolSet;
	};
};
