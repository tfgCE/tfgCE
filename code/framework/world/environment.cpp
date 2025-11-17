#include "environment.h"

#include "world.h"

#ifdef AN_CLANG
#include "..\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(Environment);

Environment::Environment(Name const & _name, Name const & _parent, SubWorld* _inSubWorld, Region* _inRegion, EnvironmentType const * _environmentType, Random::Generator const & _rg)
: base(_inSubWorld, _inRegion, _environmentType, _rg)
, name(_name)
, environmentType(_environmentType)
, parent(nullptr)
{
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("Environment [new] e%p"), this);
#endif

	// add here
	_inSubWorld->get_in_world()->add(this);
	_inSubWorld->add(this);

	todo_future(TXT("maybe separate environment to be something else, not derived from room and just contain room? because right now we have this monstrosity of setting up environment in room and then cleaning that up here"));
	base::set_environment(nullptr);
	delete_and_clear(ownEnvironment);

	if (environmentType)
	{
		// copy info from environment
		info.copy_from(environmentType->get_info());

		Name parent = _parent.is_valid() ? _parent : _name;

		if (parent.is_valid() && _inRegion)
		{
			set_parent_environment(_inRegion, parent);
		}
	}
}

Environment::~Environment()
{
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("Environment [delete] e%p"), this);
#endif

	// break parent/children chain
	set_parent_environment(nullptr);
	while (!children.is_empty())
	{
		children.get_first()->set_parent_environment(nullptr);
	}
	while (!rooms.is_empty())
	{
		an_assert(fast_cast<Environment>(rooms.get_first()) == nullptr);
		an_assert(rooms.get_first()->get_environment() == this);
		rooms.get_first()->on_environment_destroyed(this);
	}
	if (inSubWorld)
	{
		inSubWorld->remove(this); // as environment
	}
	if (auto* inWorld = get_in_world())
	{
		inWorld->remove(this); // as environment
	}
	if (inRegion)
	{
		inRegion->remove(this); // as environment
	}
}

void Environment::set_environment(Environment* _environment, Transform const& _environmentPlacement)
{
	an_assert(_environment == nullptr, TXT("do not change the environment for an environment! why did I implement it then?"));
}

void Environment::set_parent_environment(Environment* _parent)
{
	if (parent)
	{
		parent->children.remove(this);
	}
	parent = _parent;
	parentName = Name::invalid();
	if (parent)
	{
		parent->children.push_back(this);
	}
	// reconnect "info"
	info.set_parent(parent ? &parent->info : nullptr);
}

void Environment::set_parent_environment(Region* _inRegion, Name const & _parentName)
{
	if (parent)
	{
		parent->children.remove(this);
	}
	parentName = _parentName;
	parent = nullptr;
	if (_inRegion && parentName.is_valid())
	{
		if ( name != parentName)
		{
			auto* r = _inRegion;
			while (r && ! parent)
			{
				parent = r->find_environment(parentName, true);
				r = r->get_in_region();
			}
		}
	}
	if (parent)
	{
		parent->children.push_back(this);
	}
	// reconnect "info"
	info.set_parent(parent ? &parent->info : nullptr);
}

void Environment::update_parent_environment(Region* _inRegion)
{
	if (parentName.is_valid())
	{
		set_parent_environment(_inRegion, parentName);
	}
	else
	{
		set_parent_environment(_inRegion ? inRegion->get_default_environment() : nullptr);
	}
}

void Environment::add_room(Room* _room)
{
	an_assert(fast_cast<Environment>(_room) == nullptr);
	an_assert(! rooms.does_contain(_room));
	rooms.push_back(_room);
}

void Environment::remove_room(Room* _room)
{
	rooms.remove(_room);
}

bool Environment::generate()
{
	if (is_generated())
	{
		return true;
	}

	if (parent)
	{
		parent->generate();
	}

	if (auto * material = info.get_sky_box_material())
	{
		add_sky_mesh(material, info.get_sky_box_centre(), info.get_sky_box_radius());
	}

	if (auto * material = info.get_clouds_material())
	{
		add_clouds(material, info.get_sky_box_centre(), info.get_sky_box_radius());
	}

	if (info.should_allow_generation_as_room())
	{
		return base::generate();
	}
	else
	{
		place_pending_doors_on_pois();
		mark_vr_arranged();
		mark_mesh_generated();
		return true;
	}
}

void Environment::get_use_environment_from_type()
{
	// nothing here
}

void Environment::setup_using_use_environment()
{
	// nothing here
}

EnvironmentParam const * Environment::get_param(Name const & _name) const
{
	Environment const * e = this;
	while (e)
	{
		if (auto* param = e->get_info().get_params().get_param(_name))
		{
			return param;
		}
		e = e->parent;
	}
	return nullptr;
}

EnvironmentParam * Environment::access_param(Name const & _name)
{
	Environment * e = this;
	while (e)
	{
		if (auto* param = e->access_info().access_params().access_param(_name))
		{
			return param;
		}
		e = e->parent;
	}
	return nullptr;
}
