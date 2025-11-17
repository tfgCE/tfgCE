#include "region.h"

#include "regionType.h"
#include "room.h"
#include "subWorld.h"
#include "worldAddress.h"

#include "..\game\game.h"
#include "..\world\world.h"

#include "..\..\core\concurrency\scopedMRSWGuard.h"
#include "..\..\core\wheresMyPoint\wmp_context.h"

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

REGISTER_FOR_FAST_CAST(Region);

Region::Region(SubWorld* _inSubWorld, Region* _inRegion, RegionType const * _regionType, Random::Generator const& _individualRandomGenerator)
: SafeObject<Region>(this)
, inSubWorld(nullptr)
, inRegion(nullptr)
, regionType(_regionType)
, worldElementOwner(nullptr)
, defaultEnvironment(nullptr)
{
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("Region [new] g%p"), this);
#endif
	set_individual_random_generator(_individualRandomGenerator);

	an_assert(_inSubWorld);
	_inSubWorld->add(this);
	if (_inRegion)
	{
		_inRegion->add(this);
	}

	setup_environments_from_type();
}

Region::~Region()
{
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("Region [delete] g%p"), this);
#endif
	make_safe_object_unavailable();
	while (!environments.is_empty()) // before rooms, so they will be removed from rooms
	{
		delete *(environments.begin());
	}
	while (!regions.is_empty())
	{
		delete *(regions.begin());
	}
	while (!rooms.is_empty())
	{
		delete *(rooms.begin());
	}
	if (inRegion)
	{
		inRegion->remove(this);
	}
	if (inSubWorld)
	{
		inSubWorld->remove(this);
	}
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("Region [deleted] g%p"), this);
#endif
}

void Region::add(Region* _region)
{
	MRSW_GUARD_WRITE_SCOPE(regionsGuard);
	an_assert(_region->get_in_region() == nullptr, TXT("should not be in other region"));
	regions.push_back(_region);
	_region->set_in_region(this);
	_region->set_world_address_id(nextRegionWorldAddressId);
	++nextRegionWorldAddressId;
}

void Region::remove(Region* _region)
{
	MRSW_GUARD_WRITE_SCOPE(regionsGuard);
	an_assert(_region->get_in_region() == this);
	regions.remove(_region);
	_region->set_in_region(nullptr);
}

void Region::add(Environment* _environment)
{
	MRSW_GUARD_WRITE_SCOPE(environmentsGuard);
	an_assert(!environments.does_contain(_environment), TXT("should be already added to this sub world"));
	environments.push_back(_environment);
	_environment->set_world_address_id(nextEnvironmentWorldAddressId);
	++nextEnvironmentWorldAddressId;
}

void Region::remove(Environment* _environment)
{
	MRSW_GUARD_WRITE_SCOPE(environmentsGuard);
	environments.remove(_environment);
}

void Region::add(Room* _room)
{
	MRSW_GUARD_WRITE_SCOPE(roomsGuard);
	an_assert(_room->get_in_region() == nullptr, TXT("should not be in other region"));
	rooms.push_back(_room);
	_room->set_in_region(this);
	if (Environment* environment = fast_cast<Environment>(_room))
	{
		add(environment);
	}
	else
	{
		_room->set_world_address_id(nextRoomWorldAddressId);
		++nextRoomWorldAddressId;
	}
}

void Region::remove(Room* _room)
{
	MRSW_GUARD_WRITE_SCOPE(roomsGuard);
	an_assert(_room->get_in_region() == this);
	rooms.remove(_room);
	_room->set_in_region(nullptr);
	if (Environment* environment = fast_cast<Environment>(_room))
	{
		remove(environment);
	}
}

Environment* Region::find_own_environment(Name const& _name) const
{
	MRSW_GUARD_READ_SCOPE(environmentsGuard);
	for_every_ptr(environment, environments)
	{
		if (environment->get_name() == _name)
		{
			return environment;
		}
	}
	return nullptr;
}

Environment * Region::find_environment(Name const & _name, bool _allowDefault, bool _allowToFail) const
{
	if (auto* e = find_own_environment(_name))
	{
		return e;
	}
	if (inRegion)
	{
		return inRegion->find_environment(_name);
	}
	if (_allowDefault)
	{
		Environment* def = get_default_environment();
		if (def)
		{
			return def;
		}
	}
	if (!_allowToFail)
	{
		error(TXT("could not find environment \"%S\""), _name.to_char());
	}
	return nullptr;
}

Environment	* Region::get_default_environment() const
{
	MRSW_GUARD_READ_SCOPE(environmentsGuard);
	if (defaultEnvironment)
	{
		return defaultEnvironment;
	}
	if (inRegion)
	{
		return inRegion->get_default_environment();
	}
	return nullptr;
}

void Region::set_in_region(Region* _inRegion)
{
	MRSW_GUARD_WRITE_SCOPE(environmentsGuard);
	inRegion = _inRegion;
	// change parent for environments
	for_every_ptr(environment, environments)
	{
		environment->update_parent_environment(_inRegion);
	}
}

void Region::setup_environments_from_type()
{
	Random::Generator envRG = get_individual_random_generator();
	tags.clear();
	if (regionType)
	{
		tags.base_on(&regionType->get_tags());
		for_every(create, regionType->get_environments().create)
		{
			envRG.advance_seed(12425, 65480);
			bool okToGenerate = true;
			Environment* anyParentEnvironment = nullptr;
			{
				auto* r = this;
				while (r)
				{
					if (auto* e = r->find_own_environment(create->name))
					{
						if (!anyParentEnvironment)
						{
							anyParentEnvironment = e;
						}
						if (create->environmentType.get() == e->get_environment_type())
						{
							okToGenerate = false;
						}
					}
					r = r->get_in_region();
				}
			}
			if (!okToGenerate)
			{
				continue;
			}
			Environment* environment = new Environment(create->name, create->parentName, inSubWorld, this, create->environmentType.get(), envRG);
			if (!environment->get_parent_environment() && anyParentEnvironment)
			{
				environment->set_parent_environment(anyParentEnvironment);
			}
			if (! environment->get_parent_environment() && inRegion)
			{
				environment->set_parent_environment(inRegion->get_default_environment());
			}
			{
				MRSW_GUARD_WRITE_SCOPE(environmentsGuard);
				environments.push_back(environment);
			}
		}
	}
	update_default_environment();
}

void Region::update_default_environment()
{
	defaultEnvironment = nullptr;
	if (regionType)
	{
		Name defaultEnvironmentName = regionType->get_environments().defaultEnvironmentName;
		{
			MRSW_GUARD_WRITE_SCOPE(environmentsGuard);
			for_every_ptr(environment, environments)
			{
				if (environment->get_name() == defaultEnvironmentName)
				{
					defaultEnvironment = environment;
					break;
				}
			}
		}
	}
	if (!defaultEnvironment && !environments.is_empty())
	{
		defaultEnvironment = environments.get_first();
	}
}

void Region::generate_environments()
{
	MRSW_GUARD_WRITE_SCOPE(environmentsGuard);
	for_every_ptr(environment, environments)
	{
		environment->generate();
	}
}

void Region::find_point_of_interests(Optional<Name> const & _poiName, Random::Generator * _randomGenerator, REF_ ArrayStack<PointOfInterestInstance*> & _addTo, IsPointOfInterestValid _is_valid)
{
	find_point_of_interests_within(_poiName, _randomGenerator, REF_ _addTo, _is_valid);
}

void Region::find_point_of_interests_within(Optional<Name> const & _poiName, Random::Generator * _randomGenerator, REF_ ArrayStack<PointOfInterestInstance*> & _addTo, IsPointOfInterestValid _is_valid) const
{
	ASSERT_NOT_ASYNC_OR(inSubWorld->get_in_world()->multithread_check__reading_world_is_safe());
	{
		MRSW_GUARD_READ_SCOPE(roomsGuard);
		for_every_ptr(room, rooms)
		{
			room->for_every_point_of_interest(_poiName, [&_addTo, &_is_valid, _randomGenerator](PointOfInterestInstance* _fpoi) {if (!_is_valid || _is_valid(_fpoi)) { _addTo.push_back_or_replace(_fpoi, *_randomGenerator); } });
		}
	}
	{
		MRSW_GUARD_READ_SCOPE(regionsGuard);
		for_every_ptr(region, regions)
		{
			region->find_point_of_interests_within(_poiName, _randomGenerator, REF_ _addTo, _is_valid);
		}
	}
}

void Region::for_every_point_of_interest(Optional<Name> const & _poiName, OnFoundPointOfInterest _for_every_point_of_interest, IsPointOfInterestValid _is_valid) const
{
	ASSERT_NOT_ASYNC_OR(inSubWorld->get_in_world()->multithread_check__reading_world_is_safe());
	{
		MRSW_GUARD_READ_SCOPE(roomsGuard);
		for_every_ptr(room, rooms)
		{
			room->for_every_point_of_interest(_poiName, _for_every_point_of_interest, NP, _is_valid);
		}
	}
	{
		MRSW_GUARD_READ_SCOPE(regionsGuard);
		for_every_ptr(region, regions)
		{
			region->for_every_point_of_interest(_poiName, _for_every_point_of_interest, _is_valid);
		}
	}
}

bool Region::find_any_point_of_interest(Optional<Name> const & _poiName, OUT_ PointOfInterestInstance* & _outPOI, Random::Generator * _randomGenerator, IsPointOfInterestValid _is_valid)
{
	ARRAY_STACK(PointOfInterestInstance*, pois, 32);
	Random::Generator randomGenerator;
	if (!_randomGenerator)
	{
		_randomGenerator = &randomGenerator;
	}

	find_point_of_interests_within(_poiName, _randomGenerator, REF_ pois, _is_valid);

	if (pois.get_size() > 0)
	{
		_outPOI = pois[_randomGenerator->get_int(pois.get_size())];
		return true;
	}
	else
	{
		_outPOI = nullptr;
		return false;
	}
}

void Region::set_world_element_owner(IWorldElementOwner * _owner, bool _inSubRegionsToo)
{
	worldElementOwner = _owner;
	if (_inSubRegionsToo)
	{
		MRSW_GUARD_WRITE_SCOPE(regionsGuard);
		for_every_ptr(region, regions)
		{
			if (region->get_world_element_owner() == nullptr ||
				region->get_world_element_owner() == _owner)
			{
				region->set_world_element_owner(_owner, _inSubRegionsToo);
			}
		}
	}
}

Door* Region::find_door_tagged(Name const & _tag) const
{
	ASSERT_NOT_ASYNC_OR(inSubWorld->get_in_world()->multithread_check__reading_world_is_safe());
	{
		MRSW_GUARD_READ_SCOPE(roomsGuard);
		for_every_ptr(room, rooms)
		{
			if (Door* door = room->find_door_tagged(_tag))
			{
				return door;
			}
		}
	}
	{
		MRSW_GUARD_READ_SCOPE(regionsGuard);
		for_every_ptr(region, regions)
		{
			if (Door* door = region->find_door_tagged(_tag))
			{
				return door;
			}
		}
	}
	return nullptr;
}

Room* Region::find_room_tagged(Name const & _tag) const
{
	ASSERT_NOT_ASYNC_OR(inSubWorld->get_in_world()->multithread_check__reading_world_is_safe());
	{
		MRSW_GUARD_READ_SCOPE(roomsGuard);
		for_every_ptr(room, rooms)
		{
			if (room->get_tags().get_tag(_tag) != 0.0f)
			{
				return room;
			}
		}
	}
	{
		MRSW_GUARD_READ_SCOPE(regionsGuard);
		for_every_ptr(region, regions)
		{
			if (Room* room = region->find_room_tagged(_tag))
			{
				return room;
			}
		}
	}
	return nullptr;
}

List<Room*> const& Region::get_own_rooms() const
{
	ASSERT_NOT_ASYNC_OR(inSubWorld->get_in_world()->multithread_check__reading_world_is_safe());
	{
		MRSW_GUARD_READ_SCOPE(roomsGuard);
		return rooms;
	}
}

List<Region*> const& Region::get_own_regions() const
{
	ASSERT_NOT_ASYNC_OR(inSubWorld->get_in_world()->multithread_check__reading_world_is_safe());
	{
		MRSW_GUARD_READ_SCOPE(regionsGuard);
		return regions;
	}
}

void Region::for_every_room(std::function<void(Room*)> _for_every_room) const
{
	{
		MRSW_GUARD_READ_SCOPE(roomsGuard);
		for_every_ptr(r, rooms)
		{
			_for_every_room(r);
		}
	}
	{
		MRSW_GUARD_READ_SCOPE(regionsGuard);
		for_every_ptr(r, regions)
		{
			r->for_every_room(_for_every_room);
		}
	}
}

void Region::for_every_region(std::function<void(Region*)> _for_every_region)
{
	_for_every_region(this);
	{
		MRSW_GUARD_READ_SCOPE(regionsGuard);
		for_every_ptr(r, regions)
		{
			r->for_every_region(_for_every_region);
		}
	}
}

RoomType const * Region::get_door_vr_corridor_room_room_type(Random::Generator& _rg) const
{
	RoomType const * result = doorVRCorridor.get_room(_rg);
	if (!result && regionType)
	{
		result = regionType->get_door_vr_corridor_room_room_type(_rg);
	}
	if (!result && inRegion)
	{
		result = inRegion->get_door_vr_corridor_room_room_type(_rg);
	}
	return result;
}

RoomType const * Region::get_door_vr_corridor_elevator_room_type(Random::Generator& _rg) const
{
	RoomType const * result = doorVRCorridor.get_elevator(_rg);
	if (!result && regionType)
	{
		result = regionType->get_door_vr_corridor_elevator_room_type(_rg);
	}
	if (!result && inRegion)
	{
		result = inRegion->get_door_vr_corridor_elevator_room_type(_rg);
	}
	return result;
}

float Region::get_door_vr_corridor_priority(Random::Generator& _rg, float _default) const
{
	float temp;
	if (doorVRCorridor.get_priority(_rg, temp)) return temp;
	if (regionType)
	{
		float result = 0.0f;
		if (regionType->get_door_vr_corridor_priority(_rg, OUT_ result))
		{
			return result;
		}
	}
	if (inRegion)
	{
		return inRegion->get_door_vr_corridor_priority(_rg, _default);
	}
	return _default;
}

Range Region::get_door_vr_corridor_priority_range() const
{
	Range temp;
	if (doorVRCorridor.get_priority_range(temp)) return temp;
	if (regionType)
	{
		if (regionType->get_door_vr_corridor_priority_range(OUT_ temp))
		{
			return temp;
		}
	}
	if (inRegion)
	{
		return inRegion->get_door_vr_corridor_priority_range();
	}
	return Range::empty;
}

void Region::set_individual_random_generator(Random::Generator const & _individualRandomGenerator)
{
	individualRandomGenerator = _individualRandomGenerator;
}

void Region::collect_variables(REF_ SimpleVariableStorage & _variables) const
{
	// first with types, set only missing!
	// then set forced from actual individual variables that should override_ everything else

	// this means that most important variables are (from most important to least)
	//	overriding-regions ((this)type->region  -->  parent(type->region) --> and so on up to the most important)
	//	this region
	//	parent
	//	grand parent 
	//	... (so on) ...
	//	this region type
	//	parent's type
	//	...
	//	top region type

	if (Region const * startRegion = this)
	{
		int inRegionsCount = 0;
		{
			Region const * region = startRegion;
			while (region)
			{
				region = region->get_in_region();
				++inRegionsCount;
			}
		}
		ARRAY_STACK(Region const *, inRegions, inRegionsCount);
		{
			Region const * region = startRegion;
			while (region)
			{
				inRegions.push_back(region);
				region = region->get_in_region();
			}
		}
		for_every_reverse_ptr(region, inRegions)
		{
			_variables.set_from(region->get_variables());
		}
		for_every_ptr(region, inRegions)
		{
			if (auto * rt = region->get_region_type())
			{
				_variables.set_missing_from(rt->get_custom_parameters());
			}
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
	}
}

bool Region::store_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value)
{
	TypeId id = _value.get_type();
	SimpleVariableInfo const & info = variables.find(_byName, id);
	an_assert(RegisteredType::is_plain_data(id));
	memory_copy(info.access_raw(), _value.get_raw(), RegisteredType::get_size_of(id));
	return true;
}

bool Region::restore_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const
{
	TypeId id;
	void const * rawData;
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
		Region const * region = this;
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
		Region const * region = this;
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

bool Region::store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to)
{
	return variables.convert_existing(_byName, _to);
}

bool Region::rename_value_forwheres_my_point(Name const& _from, Name const& _to)
{
	return variables.rename_existing(_from, _to);
}

bool Region::store_global_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value)
{
	return store_value_for_wheres_my_point(_byName, _value);
}

bool Region::restore_global_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const
{
	return restore_value_for_wheres_my_point(_byName, OUT_ _value);
}

bool Region::get_point_for_wheres_my_point(Name const & _byName, OUT_ Vector3 & _point) const
{
	an_assert(false, TXT("no use for this"));
	return false;
}

Room* Region::get_any_room(bool _canBeEnvironment, TagCondition* _usingTagCondition) const
{
	{
		MRSW_GUARD_READ_SCOPE(roomsGuard);
		if (!rooms.is_empty())
		{
			for_every_ptr(room, rooms)
			{
				if (_usingTagCondition &&
					!_usingTagCondition->check(room->get_tags()))
				{
					// failed, skip
					continue;
				}
				if (_canBeEnvironment || !fast_cast<Environment>(room))
				{
					// we don't want to provide environments as rooms
					return room;
				}
			}
		}
	}
	{
		MRSW_GUARD_READ_SCOPE(regionsGuard);
		for_every_ptr(region, regions)
		{
			if (auto* r = region->get_any_room(_canBeEnvironment, _usingTagCondition))
			{
				return r;
			}
		}
	}
	return nullptr;
}

void Region::log(LogInfoContext & _log, Region* const & _region)
{
	if (_region && _region->get_region_type())
	{
		_log.log(_region->get_region_type()->get_name().to_string().to_char());
	}
	else
	{
		_log.log(TXT("--"));
	}
}

void Region::log(LogInfoContext & _log, ::SafePtr<Region> const & _region)
{
	if (_region.get() && _region->get_region_type())
	{
		_log.log(_region->get_region_type()->get_name().to_string().to_char());
	}
	else
	{
		_log.log(TXT("--"));
	}
}

RegionGeneratorInfo const * Region::get_region_generator_info() const
{
	if (auto* rt = get_region_type())
	{
		return rt->get_region_generator_info();
	}
	else
	{
		return nullptr;
	}
}

void Region::register_ai_manager(IModulesOwner* _aiManager)
{
	Concurrency::ScopedMRSWLockWrite lock(aiManagersLock);

	for (int i = 0; i < aiManagers.get_size(); ++i)
	{
		if (!aiManagers[i].get())
		{
			// remove any non existent
			aiManagers.remove_at(i);
			--i;
		}
		else if (aiManagers[i].get() == _aiManager)
		{
			// don't add
			return;
		}
	}

	aiManagers.push_back(::SafePtr<IModulesOwner>(_aiManager));
}

void Region::unregister_ai_manager(IModulesOwner* _aiManager)
{
	Concurrency::ScopedMRSWLockWrite lock(aiManagersLock);

	for (int i = 0; i < aiManagers.get_size(); ++i)
	{
		if (!aiManagers[i].get())
		{
			// remove any non existent
			aiManagers.remove_at(i);
			--i;
		}
		else if (aiManagers[i].get() == _aiManager)
		{
			// remove
			aiManagers.remove_at(i);
			return;
		}
	}
}

void Region::for_every_ai_manager(std::function<void(IModulesOwner*)> _do)
{
	Concurrency::ScopedMRSWLockRead lock(aiManagersLock);

	for_every_ref(aiManager, aiManagers)
	{
		if (aiManager)
		{
			_do(aiManager);
		}
	}
}

void Region::start_destruction()
{
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("Region [start destruction of a region] g%p"), this);
#endif
	for_every_ptr(room, rooms)
	{
		bool deleteRoom = false;
		Game::get()->perform_sync_world_job(TXT("mark room to be deleted [region's destruction]"), [room, &deleteRoom]()
		{
			if (room && !room->is_deletion_pending())
			{
				room->mark_to_be_deleted();
				// disconnect all doors so no one enters us
				room->sync_destroy_all_doors();
				deleteRoom = true;
			}
		});

		if (deleteRoom)
		{
			::SafePtr<Room> safeRoom(room);
			Game::get()->add_sync_world_job(TXT("destroy room"), [safeRoom]()
			{
				if (auto* room = safeRoom.get())
				{
#ifdef AN_OUTPUT_WORLD_GENERATION
					output(TXT("region destruction - destroy actual room \"%S\""), room->get_name().to_char());
#endif
					// and destroy room
					delete room;
#ifdef AN_OUTPUT_WORLD_GENERATION
					output(TXT("do destruction of room - done"));
#endif
				}
			});
		}
	}

	Game::get()->perform_sync_world_job(TXT("remove rooms"), [this]()
	{
		rooms.clear();
	});

	for_every_ptr(region, regions)
	{
		region->start_destruction();
	}

	// because we call start_destruction for nested regions, they will add sync world job to delete/destroy region earlier 
	// use safe ptr in case it got destroyed as nested
	::SafePtr<Region> safeRegion(this);
	Game::get()->add_sync_world_job(TXT("destroy region"), [safeRegion]()
	{
		if (auto* self = safeRegion.get())
		{
#ifdef AN_OUTPUT_WORLD_GENERATION
			output(TXT("destruction of region \"%S\""), self->get_region_type()? self->get_region_type()->get_name().to_string().to_char() : TXT("??"));
#endif
			if (auto* r = self->get_in_region())
			{
				r->remove(self);
			}
			an_assert(safeRegion.get());
			// and destroy this region
			delete self;
		}
	});
}

WorldAddress Region::get_world_address() const
{
	WorldAddress wa;
	wa.build_for(this);
	return wa;
}

bool Region::is_in_region(Region const* _region) const
{
	Region const* r = this;
	while (r)
	{
		if (r == _region)
		{
			return true;
		}
		r = r->get_in_region();
	}
	return false;
}

void Region::run_wheres_my_point_processor_setup()
{
	if (auto* rt = get_region_type())
	{
		{
			WheresMyPoint::Context context(this);
			Random::Generator rg = get_individual_random_generator();
			context.set_random_generator(rg);
			rt->get_wheres_my_point_processor_setup_parameters().update(context);
		}

		{
			WheresMyPoint::Context context(this);
			Random::Generator rg = get_individual_random_generator();
			context.set_random_generator(rg);
			rt->get_wheres_my_point_processor_on_setup_generator().update(context);
		}

		{
			WheresMyPoint::Context context(this);
			Random::Generator rg = get_individual_random_generator();
			context.set_random_generator(rg);
			rt->get_wheres_my_point_processor_on_generate().update(context);
		}
	}
}
