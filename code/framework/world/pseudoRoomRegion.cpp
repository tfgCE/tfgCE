#include "pseudoRoomRegion.h"

#include "region.h"
#include "regionType.h"
#include "roomType.h"

#include "..\..\core\wheresMyPoint\wmp_context.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(PseudoRoomRegion);

PseudoRoomRegion::PseudoRoomRegion(Region* _inRegion, RegionType const* _regionType)
: inRegion(_inRegion)
, regionType(_regionType)
{
}

PseudoRoomRegion::PseudoRoomRegion(Region* _inRegion, RoomType const* _roomType)
: inRegion(_inRegion)
, roomType(_roomType)
{
}

PseudoRoomRegion::~PseudoRoomRegion()
{
}

void PseudoRoomRegion::set_individual_random_generator(Random::Generator const & _individualRandomGenerator)
{
	individualRandomGenerator = _individualRandomGenerator;
}

bool PseudoRoomRegion::store_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value)
{
	TypeId id = _value.get_type();
	SimpleVariableInfo const & info = variables.find(_byName, id);
	an_assert(RegisteredType::is_plain_data(id));
	memory_copy(info.access_raw(), _value.get_raw(), RegisteredType::get_size_of(id));
	return true;
}

bool PseudoRoomRegion::restore_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const
{
	TypeId id;
	void const * rawData;
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
	{
		rawData = nullptr;
		Region const* region = inRegion;
		while (region)
		{
			if (region->is_overriding_region())
			{
				if (region->get_variables().get_existing_of_any_type_id(_byName, OUT_ id, OUT_ rawData))
				{
				}
				else if (auto* rt = region->get_region_type())
				{
					rt->get_custom_parameters().get_existing_of_any_type_id(_byName, OUT_ id, OUT_ rawData);
				}
			}
			region = region->get_in_region();
		}
		if (rawData)
		{
			_value.set_raw(id, rawData);
			return true;
		}
	}
	{
		if (auto* rt = regionType)
		{
			if (regionType && regionType->get_custom_parameters().get_existing_of_any_type_id(_byName, OUT_ id, OUT_ rawData))
			{
				_value.set_raw(id, rawData);
				return true;
			}
		}
	}
	{
		if (auto* rt = roomType)
		{
			if (roomType && roomType->get_custom_parameters().get_existing_of_any_type_id(_byName, OUT_ id, OUT_ rawData))
			{
				_value.set_raw(id, rawData);
				return true;
			}
		}
	}
	{
		Region const * region = inRegion;
		while (region)
		{
			if (region->get_variables().get_existing_of_any_type_id(_byName, OUT_ id, OUT_ rawData))
			{
				_value.set_raw(id, rawData);
				return true;
			}
			region = region->get_in_region();
		}
	}
	{
		Region const * region = inRegion;
		while (region)
		{
			if (region->get_region_type() && region->get_region_type()->get_custom_parameters().get_existing_of_any_type_id(_byName, OUT_ id, OUT_ rawData))
			{
				_value.set_raw(id, rawData);
				return true;
			}
			region = region->get_in_region();
		}
	}
	return false;
}

bool PseudoRoomRegion::store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to)
{
	return variables.convert_existing(_byName, _to);
}

bool PseudoRoomRegion::rename_value_forwheres_my_point(Name const& _from, Name const& _to)
{
	return variables.rename_existing(_from, _to);
}

bool PseudoRoomRegion::store_global_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value)
{
	return store_value_for_wheres_my_point(_byName, _value);
}

bool PseudoRoomRegion::restore_global_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const
{
	return restore_value_for_wheres_my_point(_byName, OUT_ _value);
}

bool PseudoRoomRegion::get_point_for_wheres_my_point(Name const & _byName, OUT_ Vector3 & _point) const
{
	an_assert(false, TXT("no use for this"));
	return false;
}

void PseudoRoomRegion::run_wheres_my_point_processor_setup_parameters()
{
	if (auto* rt = regionType)
	{
		WheresMyPoint::Context context(this);
		Random::Generator rg = get_individual_random_generator();
		context.set_random_generator(rg);
		rt->get_wheres_my_point_processor_setup_parameters().update(context);
	}
	else if (auto* rt = roomType)
	{
		WheresMyPoint::Context context(this);
		Random::Generator rg = get_individual_random_generator();
		context.set_random_generator(rg);
		rt->get_wheres_my_point_processor_setup_parameters().update(context);
	}
}

void PseudoRoomRegion::collect_variables(REF_ SimpleVariableStorage& _variables) const
{
	// first with types, set only missing!
	// then set forced from actual individual variables that should override_ everything else

	// this means that most important variables are (from most important to least)
	//	this room
	//	overriding-regions ((this)type->region  -->  parent(type->region) --> and so on up to the most important)
	//	in region
	//	region's parent, grand parent
	//	... (so on) ...
	//	this room/region type
	//	in region type
	//	...
	//	top region type

	if (roomType)
	{
		_variables.set_missing_from(roomType->get_custom_parameters());
	}
	if (regionType)
	{
		_variables.set_missing_from(regionType->get_custom_parameters());
	}
	int inRegionsCount = 0;
	if (Region const* startRegion = get_in_region())
	{
		Region const* region = startRegion;
		while (region)
		{
			if (auto* rt = region->get_region_type())
			{
				_variables.set_missing_from(rt->get_custom_parameters());
			}
			region = region->get_in_region();
			++inRegionsCount;
		}
	}

	ARRAY_STACK(Region const*, inRegions, inRegionsCount);
	if (Region const* startRegion = get_in_region())
	{
		Region const* region = startRegion;
		while (region)
		{
			inRegions.push_back(region);
			region = region->get_in_region();
		}
	}
	for_every_reverse_ptr(region, inRegions)
	{
		_variables.set_missing_from(region->get_variables());
	}
	// apply overriding regions
	{
		for_every_ptr(region, inRegions)
		{
			if (region->is_overriding_region())
			{
				if (auto* rt = region->get_region_type())
				{
					_variables.set_from(rt->get_custom_parameters());
				}
				_variables.set_from(region->get_variables());
			}
		}
	}
	_variables.set_from(get_variables());
}
