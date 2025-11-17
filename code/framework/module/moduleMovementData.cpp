#include "moduleMovementData.h"

#include "moduleMovement.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"

#include "..\..\core\other\parserUtils.h"

//

using namespace Framework;

//

DEFINE_STATIC_NAME(allowMovementDirectionDifference);
DEFINE_STATIC_NAME(limitMovementDirectionDifference);
DEFINE_STATIC_NAME(speed);
DEFINE_STATIC_NAME(acceleration);
DEFINE_STATIC_NAME(verticalSpeed);
DEFINE_STATIC_NAME(verticalAcceleration);
DEFINE_STATIC_NAME(orientationSpeed);
DEFINE_STATIC_NAME(orientationAcceleration);
DEFINE_STATIC_NAME(disallowOrientationMatchOvershoot);
DEFINE_STATIC_NAME(useGravityFromTraceDotLimit);
DEFINE_STATIC_NAME(floorLevelOffset);
DEFINE_STATIC_NAME(allowsUsingGravityFromOutboundTraces);

//

REGISTER_FOR_FAST_CAST(ModuleMovementData);

//

ModuleMovementData::Gait::Gait()
{
}

bool ModuleMovementData::Gait::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, ModuleMovementData* _forData)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"), name);
	for_every(node, _node->all_children())
	{
		if (node->get_name() == TXT("default"))
		{
			continue;
		}
		if (!_forData->load_gait_param_from_xml(*this, node, _lc))
		{
			error_loading_xml(node, TXT("problem loading gait param \"%S\""), node->get_name().to_char());
			result = false;
		}
	}

	// we need to have something in case during loading we cleared that (as we override_ values)
	if (auto* r = params.access_existing<Rotator3>(NAME(orientationSpeed)))
	{
		if (r->pitch == 0.0f) r->pitch = ModuleMovement::default_orientation_speed().pitch;
		if (r->yaw == 0.0f) r->yaw = ModuleMovement::default_orientation_speed().yaw;
		if (r->roll == 0.0f) r->roll = ModuleMovement::default_orientation_speed().roll;
	}
	if (auto* r = params.access_existing<Rotator3>(NAME(orientationAcceleration)))
	{
		if (r->pitch == 0.0f) r->pitch = ModuleMovement::default_orientation_acceleration().pitch;
		if (r->yaw == 0.0f) r->yaw = ModuleMovement::default_orientation_acceleration().yaw;
		if (r->roll == 0.0f) r->roll = ModuleMovement::default_orientation_acceleration().roll;
	}

	return result;
}

//

bool ModuleMovementData::AlignWithGravity::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	type = _node->get_name_attribute(TXT("type"), type);
	type = _node->get_name_attribute(TXT("name"), type);

	priority = _node->get_int_attribute(TXT("priority"), priority);
	weight = _node->get_float_attribute(TXT("weight"), weight);

	requiresPresenceGravityTraces = _node->get_bool_attribute_or_from_child_presence(TXT("requiresPresenceGravityTraces"), requiresPresenceGravityTraces);

	return result;
}

//

ModuleMovementData::ModuleMovementData(LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
, defaultGait(nullptr)
{
	baseGait.isBase = true;
}

ModuleMovementData::~ModuleMovementData()
{
	for_every_ptr(gait, gaits)
	{
		delete gait;
	}
	gaits.clear();
	defaultGait = nullptr;
}

bool ModuleMovementData::read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc)
{
	if (_attr->get_name() == TXT("name"))
	{
		movementName = _attr->get_as_name();
		return true;
	}
	else
	{
		return base::read_parameter_from(_attr, _lc);
	}
}

bool ModuleMovementData::read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (_node->get_name() == TXT("name"))
	{
		movementName = Name(_node->get_text());
		return true;
	}
	else if (_node->get_name() == TXT("disallowMovementLinear"))
	{
		Name v = _node->get_name_attribute(TXT("var"));
		if (v.is_valid())
		{
			disallowMovementLinearVars.push_back(v);
		}
		return true;
	}
	else if (_node->get_name() == TXT("gait"))
	{
		Name gaitName = _node->get_name_attribute(TXT("name"));
		if (gaitName.is_valid())
		{
			if (! gaits.has_key(gaitName))
			{
				gaits[gaitName] = new Gait();
			}
			if (_node->get_bool_attribute(TXT("default")) || _node->first_child_named(TXT("default")))
			{
				defaultGait = gaits[gaitName];
			}
			return gaits[gaitName]->load_from_xml(_node, _lc, this);
		}
		else
		{
			return false;
		}
	}
	else if (_node->get_name() == TXT("baseGait"))
	{
		return baseGait.load_from_xml(_node, _lc, this);
	}
	else if (_node->get_name() == TXT("alignWithGravity"))
	{
		AlignWithGravity awg;
		if (awg.load_from_xml(_node, _lc))
		{
			if (!awg.type.is_valid())
			{
				error_loading_xml(_node, TXT("\"type\" not provided for \"alignWithGravity\""));
				return false;
			}
			alignWithGravity[awg.type] = awg;
			return true;
		}
		else
		{
			return false;
		}
	}
	else if (_node->get_name() == TXT("moduleAlignWithGravity"))
	{
		return moduleAlignWithGravity.load_from_xml(_node, _lc);
	}
	else
	{
		return base::read_parameter_from(_node, _lc);
	}
}

bool ModuleMovementData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every_ptr(gait, gaits)
	{
		if (!defaultGait)
		{
			defaultGait = gait;
		}
	}

	return result;
}

bool ModuleMovementData::load_gait_param_from_xml(ModuleMovementData::Gait & _gait, IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	LOAD_GAITS_PROP(float, allowMovementDirectionDifference, 0.0f);
	LOAD_GAITS_PROP(float, limitMovementDirectionDifference, -1.0f);
	LOAD_GAITS_PROP(float, speed, 0.0f);
	LOAD_GAITS_PROP(float, acceleration, 10.0f);
	LOAD_GAITS_PROP(float, verticalSpeed, 0.0f);
	LOAD_GAITS_PROP(float, verticalAcceleration, 0.0f);
	LOAD_GAITS_PROP(Rotator3, orientationSpeed, ModuleMovement::default_orientation_speed());
	LOAD_GAITS_PROP(Rotator3, orientationAcceleration, ModuleMovement::default_orientation_acceleration());
	LOAD_GAITS_PROP(float, useGravityFromTraceDotLimit, 0.6f);
	if (_node->get_name() == TXT("disallowOrientationMatchOvershoot"))
	{
		_gait.params.access<Optional<bool>>(NAME(disallowOrientationMatchOvershoot)) = ParserUtils::parse_bool(_node->get_text(), true);
		return true;
	}
	if (_node->get_name() == TXT("allowOrientationMatchOvershoot"))
	{
		_gait.params.access<Optional<bool>>(NAME(disallowOrientationMatchOvershoot)) = ! ParserUtils::parse_bool(_node->get_text(), false);
		return true;
	}
	if (_node->get_name() == TXT("allowWallClimbing"))
	{
		_gait.params.access<float>(NAME(useGravityFromTraceDotLimit)) = -0.2f;
		return true;
	}
	if (_node->get_name() == TXT("disallowWallClimbing"))
	{
		_gait.params.access<float>(NAME(useGravityFromTraceDotLimit)) = 0.6f;
		return true;
	}
	LOAD_GAITS_PROP(float, floorLevelOffset, 0.0f);
	LOAD_GAITS_PROP(bool, allowsUsingGravityFromOutboundTraces, false);
	return false;
}

ModuleMovementData::Gait const * ModuleMovementData::find_gait(Name const & _gaitName) const
{
	if (ModuleMovementData::Gait const * const * gait = gaits.get_existing(_gaitName))
	{
		return *gait;
	}
	else
	{
		return defaultGait;
	}
}

ModuleMovementData::AlignWithGravity const * ModuleMovementData::find_align_with_gravity(Name const & _type) const
{
	if (_type.is_valid())
	{
		if (ModuleMovementData::AlignWithGravity const * awg = alignWithGravity.get_existing(_type))
		{
			return awg;
		}
		else
		{
			return nullptr;
		}
	}
	else
	{
		return &moduleAlignWithGravity;
	}
}

#ifdef AN_DEVELOPMENT_OR_PROFILER
Array<Name> ModuleMovementData::list_gaits() const
{
	Array<Name> result;
	for_every(gait, gaits)
	{
		result.push_back(for_everys_map_key(gait));
	}
	return result;
}
#endif
