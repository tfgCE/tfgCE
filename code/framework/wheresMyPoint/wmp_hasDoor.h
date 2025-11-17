#pragma once

#include "..\..\core\wheresMyPoint\wmp.h"

#include "..\library\libraryName.h"
#include "..\meshGen\meshGenParam.h"
#include "..\world\worldSettingCondition.h"

namespace Framework
{
	/**
	 *	Get parameter (custom parameters only!) from library stored object
	 */
	class HasDoor
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;

	private:
		TagCondition tagCondition;
	};
};
