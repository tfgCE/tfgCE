#include "wmp_roomGeneratorBalconies.h"

#include "..\roomGenerators\roomGenerators\balconies.h"

//

using namespace TeaForGodEmperor;

//

bool RoomGeneratorBalconies::load_from_xml(IO::XML::Node const* _node)
{
	bool result = base::load_from_xml(_node);

	getValue = _node->get_name_attribute(TXT("get"), getValue);

	storeMaxSize = _node->get_name_attribute(TXT("storeMaxSize"), storeMaxSize);
	storeMaxBalconyWidthAs = _node->get_name_attribute(TXT("storeMaxBalconyWidthAs"), storeMaxBalconyWidthAs);
	storeAvailableSpaceForBalconyAs = _node->get_name_attribute(TXT("storeAvailableSpaceForBalconyAs"), storeAvailableSpaceForBalconyAs);
	storeBalconyDoorWidthAs = _node->get_name_attribute(TXT("storeBalconyDoorWidthAs"), storeBalconyDoorWidthAs);
	storeBalconyDoorHeightAs = _node->get_name_attribute(TXT("storeBalconyDoorHeightAs"), storeBalconyDoorHeightAs);

	return result;
}

#define GET_VALUE(type, what) \
	if (getValue == TXT(#what)) \
	{ \
		_value.set<type>(bpvContext.what); \
	}

#define STORE_VALUE(type, what, storeAs) \
	if (storeAs.is_valid()) \
	{ \
		WheresMyPoint::Value value; \
		value.set<type>(bpvContext.what); \
		wmpOwner->store_value_for_wheres_my_point(storeAs, value); \
	}

bool RoomGeneratorBalconies::update(REF_ WheresMyPoint::Value& _value, WheresMyPoint::Context& _context) const
{
	bool result = true;

	RoomGenerators::BalconiesPrepareVariablesContext bpvContext;
	bpvContext.use(RoomGenerationInfo::get());
	bpvContext.use(_context.get_owner());
	bpvContext.process();

	GET_VALUE(Vector2, maxSize);
	GET_VALUE(float, maxBalconyWidth);
	GET_VALUE(Vector2, availableSpaceForBalcony);
	GET_VALUE(float, balconyDoorWidth);
	GET_VALUE(float, balconyDoorHeight);

	if (auto* wmpOwner = _context.get_owner())
	{
		STORE_VALUE(Vector2, maxSize, storeMaxSize);
		STORE_VALUE(float, maxBalconyWidth, storeMaxBalconyWidthAs);
		STORE_VALUE(Vector2, availableSpaceForBalcony, storeAvailableSpaceForBalconyAs);
		STORE_VALUE(float, balconyDoorWidth, storeBalconyDoorWidthAs);
		STORE_VALUE(float, balconyDoorHeight, storeBalconyDoorHeightAs);
	}

	return result;
}

//