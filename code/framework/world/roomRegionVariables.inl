
#include "region.h"
#include "regionType.h"
#include "room.h"
#include "roomType.h"

//

template <typename Class>
void ::Framework::Region::on_own_variable(Name const & _var, std::function<void(Class const &)> _do) const
{
	if (auto* v = get_variables().get_existing<Class>(_var))
	{
		_do(*v);
	}
	if (regionType)
	{
		if (auto* v = regionType->get_custom_parameters().get_existing<Class>(_var))
		{
			_do(*v);
		}
	}
}

template <typename Class>
void ::Framework::Region::on_each_variable_up(Name const & _var, std::function<void(Class const &)> _do) const
{
	on_own_variable(_var, _do);
	if (inRegion)
	{
		inRegion->on_each_variable_up(_var, _do);
	}
}

template <typename Class>
void ::Framework::Region::on_each_variable_down(Name const & _var, std::function<void(Class const &)> _do) const
{
	if (inRegion)
	{
		inRegion->on_each_variable_down(_var, _do);
	}
	on_own_variable(_var, _do);
}

template <typename Class>
Class const* ::Framework::Region::get_variable(Name const& _var) const
{
	if (auto* v = get_variables().get_existing<Class>(_var))
	{
		return v;
	}
	{
		auto* r = get_in_region();
		while (r)
		{
			if (r->is_overriding_region())
			{
				if (auto* v = r->get_variables().get_existing<Class>(_var))
				{
					return v;
				}
				if (auto* rt = r->get_region_type())
				{
					if (auto* v = rt->get_custom_parameters().get_existing<Class>(_var))
					{
						return v;
					}
				}
			}
			r = r->get_in_region();
		}
	}
	{
		auto* r = get_in_region();
		while (r)
		{
			if (auto* v = r->get_variables().get_existing<Class>(_var))
			{
				return v;
			}
			r = r->get_in_region();
		}
	}
	if (regionType)
	{
		if (auto* v = regionType->get_custom_parameters().get_existing<Class>(_var))
		{
			return v;
		}
	}
	{
		auto* r = get_in_region();
		while (r)
		{
			if (auto* rt = r->get_region_type())
			{
				if (auto* v = rt->get_custom_parameters().get_existing<Class>(_var))
				{
					return v;
				}
			}
			r = r->get_in_region();
		}
	}
	return nullptr;
}

//

template <typename Class>
void ::Framework::Room::on_own_variable(Name const & _var, std::function<void(Class const &)> _do) const
{
	if (auto* v = get_variables().get_existing<Class>(_var))
	{
		_do(*v);
	}
	if (roomType)
	{
		if (auto* v = roomType->get_custom_parameters().get_existing<Class>(_var))
		{
			_do(*v);
		}
	}
}

template <typename Class>
void ::Framework::Room::on_each_variable_up(Name const & _var, std::function<void(Class const &)> _do) const
{
	on_own_variable(_var, _do);
	if (inRegion)
	{
		inRegion->on_each_variable_up(_var, _do);
	}
}

template <typename Class>
void ::Framework::Room::on_each_variable_down(Name const & _var, std::function<void(Class const &)> _do) const
{
	if (inRegion)
	{
		inRegion->on_each_variable_down(_var, _do);
	}
	on_own_variable(_var, _do);
}

template <typename Class>
Class const* ::Framework::Room::get_variable(Name const& _var, bool _checkOriginRoom) const
{
	if (_checkOriginRoom)
	{
		if (auto* r = get_origin_room())
		{
			if (auto* v = r->get_variables().get_existing<Class>(_var))
			{
				return v;
			}
		}
	}
	if (auto* v = get_variables().get_existing<Class>(_var))
	{
		return v;
	}
	{
		auto* r = get_in_region();
		while (r)
		{
			if (r->is_overriding_region())
			{
				if (auto* v = r->get_variables().get_existing<Class>(_var))
				{
					return v;
				}
				if (auto* rt = r->get_region_type())
				{
					if (auto* v = rt->get_custom_parameters().get_existing<Class>(_var))
					{
						return v;
					}
				}
			}
			r = r->get_in_region();
		}
	}
	{
		auto* r = get_in_region();
		while (r)
		{
			if (auto* v = r->get_variables().get_existing<Class>(_var))
			{
				return v;
			}
			r = r->get_in_region();
		}
	}
	if (_checkOriginRoom)
	{
		if (auto* r = get_origin_room())
		{
			if (auto* rt = r->get_room_type())
			{
				if (auto* v = rt->get_custom_parameters().get_existing<Class>(_var))
				{
					return v;
				}
			}
		}
	}
	if (auto* rt = get_room_type())
	{
		if (auto* v = rt->get_custom_parameters().get_existing<Class>(_var))
		{
			return v;
		}
	}
	{
		auto* r = get_in_region();
		while (r)
		{
			if (auto* rt = r->get_region_type())
			{
				if (auto* v = rt->get_custom_parameters().get_existing<Class>(_var))
				{
					return v;
				}
			}
			r = r->get_in_region();
		}
	}
	return nullptr;
}
