#pragma once

#include "iWMPTool.h"
#include "..\random\randomNumber.h"

namespace WheresMyPoint
{
	/*
		examples:

		<@someFloatValue />
		<chooseByIntValue var="idx">
			<choose for="0">
				<mul>-0.5</mul>
				<add><@platformRide_platformSize.y /><half/></add>
			</choose>
			<choose for="1">
				<mul>0.5</mul>
				<sub><@platformRide_platformSize.y /><half/></sub>
			</choose>
		</chooseByIntValue>

		<intRandom min="0" max="1"/>
		<chooseByIntValue>
			<choose for="0"><@footbridge_doorAtP0 /></choose>
			<choose for="1"><@footbridge_doorAtP1 /></choose>
		</chooseByIntValue>

		<chooseByIntValue var="idx">
			<choose for="0"><float>-1</float></choose>
			<choose for="1"><float> 1</float></choose>
		</chooseByIntValue>
	 */

	class ChooseByIntValue
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const* _node);
		override_ bool update(REF_ Value& _value, Context& _context) const;

		override_ tchar const* development_description() const { return TXT("perform toolset by choosing by int value"); }

	private:
		struct Element
		{
			Optional<int> value;
			ToolSet doToolSet;
			Optional<int> provideInt;
			Optional<float> provideFloat;
		};

		Name varName;
		Array<Element> elements;
	};
};

