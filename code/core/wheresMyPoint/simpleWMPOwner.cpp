#include "simpleWMPOwner.h"

//

using namespace WheresMyPoint;

//

REGISTER_FOR_FAST_CAST(SimpleWMPOwner);

bool SimpleWMPOwner::store_value_for_wheres_my_point(Name const& _byName, WheresMyPoint::Value const& _value)
{
	TypeId id = _value.get_type();
	SimpleVariableInfo const& info = variables.find(_byName, id);
	an_assert(RegisteredType::is_plain_data(id));
	memory_copy(info.access_raw(), _value.get_raw(), RegisteredType::get_size_of(id));
	return true;
}

bool SimpleWMPOwner::restore_value_for_wheres_my_point(Name const& _byName, OUT_ WheresMyPoint::Value& _value) const
{
	TypeId id;
	void const* rawData;
	{
		rawData = nullptr;
		if (auto* info = variables.find_any_existing(_byName))
		{
			id = info->type_id();
			rawData = info->get_raw();
		}
		if (rawData)
		{
			_value.set_raw(id, rawData);
			return true;
		}
	}
	return false;
}

bool SimpleWMPOwner::store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to)
{
	return variables.convert_existing(_byName, _to);
}

bool SimpleWMPOwner::rename_value_forwheres_my_point(Name const& _from, Name const &_to)
{
	return variables.rename_existing(_from, _to);
}
