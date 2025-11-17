#pragma once

#include "..\..\core\wheresMyPoint\wmp.h"

#include "..\library\libraryName.h"
#include "..\meshGen\meshGenParam.h"
#include "..\world\worldSettingCondition.h"

namespace Framework
{
	/**
	 *	Choose from random sets of directly identified and through world setting conditions
	 */
	class ChooseLibraryStored
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;

	private:
		Name type;
		struct Entry
		{
			LibraryName name;
			WorldSettingCondition worldSettingCondition;
			float probabilityCoef = 1.0f;

			bool load_from_xml(IO::XML::Node const * _node);
		};
		Array<Entry> entries;

	};
};
