#pragma once

#include "..\..\core\concurrency\mrswGuard.h"
#include "..\..\core\containers\list.h"
#include "..\..\core\memory\safeObject.h"
#include "..\..\core\random\random.h"
#include "..\..\core\system\video\shaderProgramBindingContext.h"

#include "doorVRCorridor.h"
#include "environmentOverlay.h"
#include "pointOfInterestInstance.h"
#include "roomGeneratorInfoSet.h"

#include <functional>

namespace Framework
{
	class JobSystem;

	class IWorldElementOwner;
	
	class Environment;
	class Door;
	class Room;
	class SubWorld;
	class RegionType;
	class RegionGenerator;
	class RegionGeneratorInfo;
	class RoomType;
	
	struct PointOfInterestInstance;
	struct WorldAddress;
		
	/*
	 *	Region contains rooms and environments
	 *	Allows rooms to choose environments after name
	 */
	class Region
	: public SafeObject<Region>
	, public WheresMyPoint::IOwner
	{
		FAST_CAST_DECLARE(Region);
		FAST_CAST_BASE(WheresMyPoint::IOwner);
		FAST_CAST_END();
	public:
		Region(SubWorld* _inSubWorld, Region* _inRegion, RegionType const * _regionType, Random::Generator const& _individualRandomGenerator);
		~Region();

		inline RegionType const * get_region_type() const { return regionType; }
		
		bool is_overriding_region() const { return overridingRegion; }
		void be_overriding_region(bool _overridingRegion = true) { overridingRegion = _overridingRegion; }

		Tags& access_tags() { return tags; }
		Tags const& get_tags() const { return tags; }

		void set_world_address_id(int _id) { worldAddressID = _id; }
		int get_world_address_id() const { return worldAddressID; }
		WorldAddress get_world_address() const;

		SimpleVariableStorage const & get_variables() const { return variables; }
		SimpleVariableStorage & access_variables() { return variables; }
		void collect_variables(REF_ SimpleVariableStorage & _variables) const;

		// has to include roomRegionVariables.inl
		// up means that first we do own and then proceed up (region is least important), for down we start from the top (region is most important)
		template <typename Class>
		void on_each_variable_up(Name const & _var, std::function<void(Class const &)> _do) const;
		template <typename Class>
		void on_each_variable_down(Name const & _var, std::function<void(Class const &)> _do) const;
		template <typename Class>
		void on_own_variable(Name const & _var, std::function<void(Class const &)> _do) const;

		template <typename Class>
		Class const * get_variable(Name const & _var) const;

		Random::Generator const & get_individual_random_generator() const { return individualRandomGenerator; }
		void set_individual_random_generator(Random::Generator const & _individualRandomGenerator);

		RegionGeneratorInfo const * get_region_generator_info() const;

		void run_wheres_my_point_processor_setup(); // use this if we don't generate but want wmp to run

		inline SubWorld* get_in_sub_world() const { return inSubWorld; }
		inline Region* get_in_region() const { return inRegion; }
		bool is_in_region(Region const* _region) const;

		Room* get_any_room(bool _canBeEnvironment = true, TagCondition* _usingTagCondition = nullptr) const; // any room, it can be environment, actual room, it is used only to place ai managers somewhere

		void update_default_environment();
		Environment	* find_own_environment(Name const & _name) const;
		Environment	* find_environment(Name const & _name, bool _allowDefault = false, bool _allowToFail = false) const;
		Environment	* get_default_environment() const;

		Array<UsedLibraryStored<EnvironmentOverlay>> const& get_environment_overlays() const { return environmentOverlays; }
		Array<UsedLibraryStored<EnvironmentOverlay>>& access_environment_overlays() { return environmentOverlays; }

		void set_world_element_owner(IWorldElementOwner * _owner, bool _inSubRegionsToo = false); // won't set in subregion if it already has owner set
		IWorldElementOwner* get_world_element_owner() const { return worldElementOwner; }

 		::System::ShaderProgramBindingContext const & get_shader_program_binding_context() const { return shaderProgramBindingContext; }
		::System::ShaderProgramBindingContext & access_shader_program_binding_context() { return shaderProgramBindingContext; }

		void for_every_point_of_interest(Optional<Name> const & _poiName, OnFoundPointOfInterest _for_every_point_of_interest, IsPointOfInterestValid _is_valid = nullptr) const;
		bool find_any_point_of_interest(Optional<Name> const & _poiName, OUT_ PointOfInterestInstance* & _outPOI, Random::Generator * _randomGenerator = nullptr, IsPointOfInterestValid _is_valid = nullptr);
		void find_point_of_interests(Optional<Name> const & _poiName, Random::Generator * _randomGenerator, REF_ ArrayStack<PointOfInterestInstance*> & _addTo, IsPointOfInterestValid _is_valid = nullptr);

		Door* find_door_tagged(Name const & _tag) const;
		Room* find_room_tagged(Name const & _tag) const;

		List<Room*> const & get_own_rooms() const; // not all inside but just own
		void for_every_room(std::function<void(Room*)> _for_every_room) const;

		List<Region*> const& get_own_regions() const;
		void for_every_region(std::function<void(Region*)> _for_every_region); // including itself

		RoomType const * get_door_vr_corridor_room_room_type(Random::Generator& _rg) const;
		RoomType const * get_door_vr_corridor_elevator_room_type(Random::Generator& _rg) const;
		float get_door_vr_corridor_priority(Random::Generator& _rg, float _default) const;
		Range get_door_vr_corridor_priority_range() const;

		static void log(LogInfoContext & _log, Region* const & _region);
		static void log(LogInfoContext & _log, ::SafePtr<Region> const & _region);

		void start_destruction(); // marks rooms to delete (if not marked already and destroys them)

	public:
		void register_ai_manager(IModulesOwner*);
		void unregister_ai_manager(IModulesOwner*);
		void for_every_ai_manager(std::function<void(IModulesOwner*)> _do);

	public:
		void add(Region* _region);
		void remove(Region* _region);

		void add(Environment* _environment);
		void remove(Environment* _environment);

		void add(Room* _room);
		void remove(Room* _room);

	public: // WheresMyPoint::IOwner
		override_ bool store_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value);
		override_ bool restore_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const;
		override_ bool store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to);
		override_ bool rename_value_forwheres_my_point(Name const& _from, Name const& _to);
		override_ bool store_global_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value);
		override_ bool restore_global_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const;
		override_ bool get_point_for_wheres_my_point(Name const & _byName, OUT_ Vector3 & _point) const; // shouldn't be called!

		override_ WheresMyPoint::IOwner* get_wmp_owner() { return inRegion; }

	protected: friend class SubWorld;
		inline void set_in_sub_world(SubWorld* _inSubWorld) { inSubWorld = _inSubWorld; }
		void set_in_region(Region* _inRegion);

	public: friend class RegionGenerator;
		void generate_environments();

	private:
		SubWorld* inSubWorld;
		Region* inRegion;
		RegionType const * regionType;
		Tags tags;
		Random::Generator generatorUsed;
		IWorldElementOwner* worldElementOwner;

		// if region is marked as overriding:
		//		its custom parameters are the most important
		//		its environments are the most important
		bool overridingRegion = false; 

		int worldAddressID = NONE; // partial, of this one

		Concurrency::MRSWLock aiManagersLock;
		Array<::SafePtr<IModulesOwner>> aiManagers;

		Random::Generator individualRandomGenerator; // used as a base
		SimpleVariableStorage variables; // used for various modules (eg. when creating mesh via generator)

		DoorVRCorridor doorVRCorridor; // seems to only contain priority

		::System::ShaderProgramBindingContext shaderProgramBindingContext; // for any parameters

		MRSW_GUARD_MUTABLE(environmentsGuard);
		MRSW_GUARD_MUTABLE(regionsGuard);
		MRSW_GUARD_MUTABLE(roomsGuard);

		Array<UsedLibraryStored<EnvironmentOverlay>> environmentOverlays;

		Environment* defaultEnvironment; // has to be on the list
		List<Environment*> environments;
		List<Region*> regions;
		List<Room*> rooms;
		int nextEnvironmentWorldAddressId = 0;
		int nextRegionWorldAddressId = 0;
		int nextRoomWorldAddressId = 0;

		void setup_environments_from_type();

		void find_point_of_interests_within(Optional<Name> const & _poiName, Random::Generator * _randomGenerator, REF_ ArrayStack<PointOfInterestInstance*> & _addTo, IsPointOfInterestValid _is_valid = nullptr) const;
	};
};

DECLARE_REGISTERED_TYPE(Framework::Region*);
DECLARE_REGISTERED_TYPE(SafePtr<Framework::Region>);
