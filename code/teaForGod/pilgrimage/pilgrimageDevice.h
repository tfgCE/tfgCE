#pragma once

#include "..\teaEnums.h"

#include "..\..\framework\library\libraryStored.h"
#include "..\..\framework\library\usedLibraryStored.h"

#include "..\ai\logics\managers\aiManagerLogic_spawnManager.h"

namespace Framework
{
	class SceneryType;
	class RoomType;
	class TexturePart;
};

namespace TeaForGodEmperor
{
	class LineModel;

	class PilgrimageDevice
	: public Framework::LibraryStored
	{
		FAST_CAST_DECLARE(PilgrimageDevice);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		PilgrimageDevice(Framework::Library* _library, Framework::LibraryName const& _name);
		virtual ~PilgrimageDevice();

		Framework::SceneryType* get_scenery_type() const { return sceneryType.get(); }
		Framework::RoomType* get_room_type() const { return roomType.get(); }

		Name const& get_open_world_cell_slot() const { return openWorldCellSlot; }
		Name const& get_open_world_cell_slot_secondary() const { return openWorldCellSlotSecondary; }
		TagCondition const & get_open_world_cell_tag_condition() const { return openWorldCellTagged; }
		Name const& get_open_world_cell_dist_tag() const { return openWorldCellDistTag; }

		Range const& get_open_world_distance() const { return openWorldDistance; }

		Framework::TexturePart* get_direction_texture_part() const { return directionTexturePart.get(); }
		Vector2 const & get_slot_offset() const { return slotOffset; }
		LineModel* get_for_map_line_model() const { return forMapLineModel.get(); }

		bool may_appear_depleted_for_auto_map_only() const { return ! neverAppearAsDepletedForAutoMapOnly; }
		bool may_appear_depleted() const { return ! neverAppearAsDepleted; }
		bool should_reverse_appear_depleted() const { return reverseAppearAsDepleted; }
		bool may_appear_on_map() const { return appearOnMap; }
		bool may_appear_on_map_only_if_not_depleted() const { return appearOnMapOnlyIfNotDepleted; }
		bool may_appear_on_map_only_if_depleted() const { return appearOnMapOnlyIfDepleted; }
		bool may_appear_on_map_only_if_visited() const { return appearOnMapOnlyIfVisited; }
		
		bool is_known_from_start() const { return knownFromStart; }
		bool is_unobfuscated_from_start() const { return unobfuscatedFromStart; }

		int get_priority() const { return priority; }
		float get_chance() const { return chance; }

		bool is_special() const { return special; }
		Name const & get_system() const { return system; }

		bool is_revealed_upon_entrance() const { return revealedUponEntrance; }

		bool is_guidance_beacon() const { return guidanceBeacon; }
		bool is_guidance_beacon_only_if_not_depleted() const { return guidanceBeaconIfNotDepleted; }

		bool is_long_distance_detectable() const { return longDistanceDetectable; }
		bool is_long_distance_detectable_only_if_not_depleted() const { return longDistanceDetectableOnlyIfNotDepleted; }
		bool is_long_distance_detectable_by_direction() const { return longDistanceDetectableByDirection; }

		Array<AI::Logics::Managers::SpawnManagerPOIDefinition> const& get_poi_definitions() const { return poiDefinitions; }

		SimpleVariableStorage const& get_device_setup_variables() const { return deviceSetupVariables; }
		SimpleVariableStorage const& get_state_variables() const { return stateVariables; }

		bool does_provide(EnergyType::Type _energyType) const { return providesEnergy[_energyType]; }

		bool should_show_door_directions() const { return showDoorDirections; }
		bool should_show_door_directions_only_if_depleted() const { return showDoorDirectionsOnlyIfDepleted; }
		bool should_show_door_directions_only_if_not_depleted() const { return showDoorDirectionsOnlyIfNotDepleted; }
		bool should_show_door_directions_only_if_visited() const { return showDoorDirectionsOnlyIfVisited; }
		
		bool may_fail_placing() const { return mayFailPlacing; }

		DoorDirectionObjectiveOverride::Type get_door_direction_objective_override() const { return doorDirectionObjectiveOverride; }

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	private:
		// scenery type should always be provided
		Framework::UsedLibraryStored<Framework::SceneryType> sceneryType;
		Framework::UsedLibraryStored<Framework::RoomType> roomType; // if defined, if this devices should be placed in an open world cell, it will always spawn such a room
		
		Name openWorldCellSlot; // each pilgrimage device requires "slot" - two different devices can't use same slot - this is to avoid similar devices
		Name openWorldCellSlotSecondary; // secondaries also have to differ (unless they're empty)
		TagCondition openWorldCellTagged; // can be placed in open world cell only if it has specific tags defined, this 
		Name openWorldCellDistTag; // this is tag for doors so when we calculate distances, we can learn which one
		
		Range openWorldDistance = Range::empty; // might be overriden by what is set in the pilgrimage

		Framework::UsedLibraryStored<Framework::TexturePart> directionTexturePart;
		
		Vector2 slotOffset = Vector2::zero; // just visual
		Framework::UsedLibraryStored<LineModel> forMapLineModel;

		DoorDirectionObjectiveOverride::Type doorDirectionObjectiveOverride = DoorDirectionObjectiveOverride::None;

		// appear on map
		bool neverAppearAsDepletedForAutoMapOnly = false; // this is just an information for other systems. logic does not use this. overrides all appear on map "only if"
		bool neverAppearAsDepleted = false; // will appear as not depleted - not grayed out etc
		bool reverseAppearAsDepleted = false; // will appear as depleted when not depleted
		bool appearOnMap = true; 
		bool appearOnMapOnlyIfNotDepleted = false;
		bool appearOnMapOnlyIfDepleted = false;
		bool appearOnMapOnlyIfVisited = false;

		bool knownFromStart = false; // the pilgrimage device is known when the pilgrimage starts
		bool unobfuscatedFromStart = false; // if true, the symbol is not hidden

		int priority = 0; // with a higher priority, devices are placed first, if they have same priority, the first one is randomly chosen
		float chance = 0.0f; // if device can be placed randomly, this is its chance

		bool special = false; // to allow selective show of devices (if they are known or not), two special devices can't be in the same cell
		
		Name system; // for example, exm system (upgrade machine)

		bool revealedUponEntrance = false; // if we enter the same room, we reveal its icon

		bool guidanceBeacon = false; // guidance will lead to it
		bool guidanceBeaconIfNotDepleted = false; // if true, device can't be depleted, requires above but if only this is loaded, guidanceBeacon is assumed to be true

		bool longDistanceDetectable = false; // detectable at long distance as a circle
		bool longDistanceDetectableOnlyIfNotDepleted = false; // can be detected only if not depleted
		bool longDistanceDetectableByDirection = false; // if true, will draw line and will require only 2 lines to pinpoint a location

		// only show directions!
		bool showDoorDirections = true;
		bool showDoorDirectionsOnlyIfDepleted = false; // has to be depleted to show door directions
		bool showDoorDirectionsOnlyIfNotDepleted = false; // has to be not depleted to show door directions
		bool showDoorDirectionsOnlyIfVisited = false; // has to be visited to show door directions
		
		bool mayFailPlacing = true; // in some cases we allow to fail being placed

		Array<AI::Logics::Managers::SpawnManagerPOIDefinition> poiDefinitions;

		// these are called on spawn
		SimpleVariableStorage deviceSetupVariables;

		// these variables are copied from the map when object is created and copied back when is destroyed, these are not actual variables, just lookup
		// ai logic or something else should set them
		SimpleVariableStorage stateVariables;

		bool providesEnergy[EnergyType::MAX];
	};

	struct PilgrimageDeviceInstance
	{
		Framework::UsedLibraryStored<PilgrimageDevice> device;
		SafePtr<Framework::IModulesOwner> object;
	};
};
