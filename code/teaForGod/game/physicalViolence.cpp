#include "physicalViolence.h"

using namespace TeaForGodEmperor;

bool PhysicalViolence::load_from_xml(IO::XML::Node const* _node)
{
	if (!_node)
	{
		return true;
	}

	bool result = true;

	result &= minSpeed.load_from_xml(_node, TXT("minSpeed"));
	result &= minSpeedStrong.load_from_xml(_node, TXT("minSpeedStrong"));
	result &= minSpeed.load_from_xml(_node, TXT("minSpeedMul"));
	result &= minSpeedStrong.load_from_xml(_node, TXT("minSpeedStrongMul"));
	result &= damage.load_from_xml(_node, TXT("damage"));
	result &= damageStrong.load_from_xml(_node, TXT("damageStrong"));

	return result;
}

#define OVERRIDE(what) if (_pv.what.is_set()) what = _pv.what;

void PhysicalViolence::override_with(PhysicalViolence const& _pv)
{
	OVERRIDE(minSpeed);
	OVERRIDE(minSpeedStrong);
	OVERRIDE(damage);
	OVERRIDE(damageStrong);
}
