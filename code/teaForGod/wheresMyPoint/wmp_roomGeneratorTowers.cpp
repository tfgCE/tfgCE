#include "wmp_roomGeneratorTowers.h"

#include "..\roomGenerators\roomGenerators\towers.h"

//

using namespace TeaForGodEmperor;

//

bool RoomGeneratorTowers::load_from_xml(IO::XML::Node const* _node)
{
	bool result = base::load_from_xml(_node);

	getValue = _node->get_name_attribute(TXT("get"), getValue);

	storeTowerRadiusAs = _node->get_name_attribute(TXT("storeTowerRadiusAs"), storeTowerRadiusAs);
	storeInnerTowerRadiusAs = _node->get_name_attribute(TXT("storeInnerTowerRadiusAs"), storeInnerTowerRadiusAs);
	storeHalfTowerAs = _node->get_name_attribute(TXT("storeHalfTowerAs"), storeHalfTowerAs);

	return result;
}

#define GET_VALUE(type, what) \
	if (getValue == TXT(#what)) \
	{ \
		_value.set<type>(tpvContext.what); \
	}

#define STORE_VALUE(type, what, storeAs) \
	if (storeAs.is_valid()) \
	{ \
		WheresMyPoint::Value value; \
		value.set<type>(tpvContext.what); \
		wmpOwner->store_value_for_wheres_my_point(storeAs, value); \
	}

bool RoomGeneratorTowers::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	RoomGenerators::TowersPrepareVariablesContext tpvContext;
	tpvContext.use(RoomGenerationInfo::get());
	tpvContext.use(_context.get_owner());
	tpvContext.process();

	GET_VALUE(float, towerRadius);
	GET_VALUE(float, innerTowerRadius);
	GET_VALUE(bool, halfTower);

	if (auto* wmpOwner = _context.get_owner())
	{
		STORE_VALUE(float, towerRadius, storeTowerRadiusAs);
		STORE_VALUE(float, innerTowerRadius, storeInnerTowerRadiusAs);
		STORE_VALUE(bool, halfTower, storeHalfTowerAs);
	}

	return result;
}

//