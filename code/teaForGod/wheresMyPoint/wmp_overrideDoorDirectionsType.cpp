#include "wmp_overrideDoorDirectionsType.h"

#include "..\teaEnums.h"
#include "..\game\playerSetup.h"

#include "..\..\framework\framework.h"
#include "..\..\framework\meshGen\meshGenGenerationContext.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

DEFINE_STATIC_NAME(top);
DEFINE_STATIC_NAME(sides);

//

bool OverrideDoorDirectionsType::load_from_xml(IO::XML::Node const* _node)
{
	bool result = base::load_from_xml(_node);

	varName = _node->get_name_attribute(TXT("var"), varName);

	return result;
}

bool OverrideDoorDirectionsType::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	DoorDirectionsVisible::Type ddv = PlayerPreferences::get_door_directions_visible();
	if (ddv != DoorDirectionsVisible::Auto)
	{
		WheresMyPoint::Value value;
		if (ddv == DoorDirectionsVisible::Above)
		{
			value.set(NAME(top));
		}
		else if (ddv == DoorDirectionsVisible::AtEyeLevel)
		{
			value.set(NAME(sides));
		}
		else
		{
			todo_implement;
		}

		_context.get_owner()->store_value_for_wheres_my_point(varName, value);
	}

	return result;
}
