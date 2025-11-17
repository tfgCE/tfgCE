#include "module.h"

#include "..\interfaces\aiObject.h"
#include "..\object\object.h"
#include "..\object\actor.h"
#include "..\object\temporaryObject.h"

#include "..\..\core\wheresMyPoint\wmp_context.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(Module);

Module::Module(IModulesOwner* _owner)
: moduleData( nullptr )
{
	set_owner(_owner);
}

Module::~Module()
{
}

void Module::ready_to_activate()
{
	// some temporary objects do not require reset on reuse, so if we're short on time, we skip the reset
	an_assert(!readyToActivate
		|| (fast_cast<TemporaryObject>(get_owner()) && fast_cast<TemporaryObject>(get_owner())->get_object_type() && !fast_cast<TemporaryObject>(get_owner())->get_object_type()->should_reset_on_reuse()));
	an_assert(!activated
		|| (fast_cast<TemporaryObject>(get_owner()) && fast_cast<TemporaryObject>(get_owner())->get_object_type() && !fast_cast<TemporaryObject>(get_owner())->get_object_type()->should_reset_on_reuse())); 
	readyToActivate = true;
}

void Module::activate()
{
	an_assert(readyToActivate);
	activated = true;
}

void Module::reset()
{
	setup_with(moduleData);
	activated = false;
	readyToActivate = false;
}

void Module::setup_with(ModuleData const * _moduleData)
{
	moduleData = _moduleData;
	// init all parameters from moduleData

	// update wmp processor
	if (moduleData)
	{
		WheresMyPoint::Context context(this);
		context.set_random_generator(owner->get_individual_random_generator());
		moduleData->get_wheres_my_point_processor_on_init().update(context);
	}
}

void Module::set_owner(IModulesOwner* _owner)
{
	owner = _owner;
	ownerAsObject = fast_cast<Object>(owner);
	ownerAsTemporaryObject = fast_cast<TemporaryObject>(owner);
}

bool Module::store_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value)
{
	TypeId id = _value.get_type();
	SimpleVariableInfo const & info = wmpVariables.find(_byName, id);
	an_assert(RegisteredType::is_plain_data(id)); 
	memory_copy(info.access_raw(), _value.get_raw(), RegisteredType::get_size_of(id));
	return true;
}

bool Module::store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to)
{
	return wmpVariables.convert_existing(_byName, _to);
}

bool Module::rename_value_forwheres_my_point(Name const& _from, Name const& _to)
{
	return wmpVariables.rename_existing(_from, _to);
}

bool Module::restore_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const
{
	TypeId id;
	void const * rawData;
	if (wmpVariables.get_existing_of_any_type_id(_byName, OUT_ id, OUT_ rawData))
	{
		_value.set_raw(id, rawData);
		return true;
	}
	else
	{
		return owner->restore_value_for_wheres_my_point(_byName, OUT_ _value);
	}
}

bool Module::store_global_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value)
{
	return owner->store_value_for_wheres_my_point(_byName, _value);
}

bool Module::restore_global_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const
{
	return owner->restore_value_for_wheres_my_point(_byName, OUT_ _value);
}

bool Module::get_point_for_wheres_my_point(Name const & _byName, OUT_ Vector3 & _point) const
{
	an_assert(false, TXT("no use for this"));
	return false;
}
