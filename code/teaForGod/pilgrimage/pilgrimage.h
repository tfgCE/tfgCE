#pragma once

#include "..\game\energy.h"
#include "..\game\gameDirectorDefinition.h"
#include "..\music\musicDefinition.h"

#include "..\..\framework\library\libraryStored.h"
#include "..\..\framework\text\localisedString.h"

#include "..\..\core\wheresMyPoint\iWMPTool.h"

namespace Loader
{
	interface_class ILoader;
};

namespace Framework
{
	class ActorType;
	class EnvironmentType;
	class ItemType;
	class RoomType;
	class SceneryType;
};

namespace TeaForGodEmperor
{
	class Game;
	class PilgrimageInstance;
	class PilgrimagePart;
	class PilgrimageEnvironmentMix;
	class PilgrimageEnvironmentCycle;
	class PilgrimageOpenWorldDefinition;
	struct PilgrimageEnvironment;

	struct PilgrimageUtilityRoomDefinition
	{
		Name tag;
		Framework::UsedLibraryStored<Framework::RoomType> roomType;

		bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
	};

	struct PilgrimageLockerRoomSpawn
	{
		String name; // may be missing, if set and an object with such a name already exists, it will be kept
		Tags tags;
		Framework::UsedLibraryStored<Framework::ActorType> actorType;
		Framework::UsedLibraryStored<Framework::ItemType> itemType;
		Framework::UsedLibraryStored<Framework::SceneryType> sceneryType;

		SimpleVariableStorage parameters;

		bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
	};

	/**
	 *	Pilgrimages:
	 *		1. LINEAR - level by level, the player goes from one station to another
	 *		2. OPEN WORLD - map can go in any direction or can be limited to a specific section
	 */
	class Pilgrimage
	: public Framework::LibraryStored
	{
		FAST_CAST_DECLARE(Pilgrimage);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		Pilgrimage(Framework::Library * _library, Framework::LibraryName const & _name);
		virtual ~Pilgrimage();

		Pilgrimage* get_default_next_pilgrimage() const { return defaultNextPilgrimage.get(); }
		Framework::ActorType* get_pilgrim() const { return pilgrim.get(); }
		bool should_start_here_with_exms_from_last_setup() const { return startHereWithEXMsFromLastSetup; }
		bool should_start_here_with_only_unlocked_exms() const { return startHereWithOnlyUnlockedEXMs; }
		bool should_start_here_without_exms() const { return startHereWithoutEXMs; }
		bool should_start_here_without_weapons() const { return startHereWithoutWeapons; }
		bool should_start_here_with_weapons_from_armoury() const { return startHereWithWeaponsFromArmoury; }
		bool should_start_here_with_damaged_weapons() const { return startHereWithDamagedWeapons; }

		PilgrimageInstance* create_instance(Game* _game);

		void load_on_demand_all_required();
		void unload_for_load_on_demand_all_required();

	public: // common 
		Array<RefCountObjectPtr<PilgrimageEnvironmentCycle>> const& get_environment_cycles() const { return environmentCycles; }
		float get_environment_cycle_speed() const { return environmentCycleSpeed; }
		Optional<float> const & get_lock_environment_cycle_until_seen() const { return lockEnvironmentCycleUntilSeen; }
		TagCondition const & get_lock_environment_cycle_until_seen_room_tagged() const { return lockEnvironmentCycleUntilSeenRoomTagged; }
		Array<PilgrimageEnvironment> const& get_environments() const { return environments; }
		Optional<float> const & get_forced_noon_yaw() const { return forcedNoonYaw; }
		Optional<float> const & get_forced_noon_pitch() const { return forcedNoonPitch; }

		Framework::LocalisedString const& get_pilgrimage_name_ls() const { return pilgrimageName; }
		Framework::LocalisedString const& get_pilgrimage_name_pre_short_ls() const { return pilgrimageNamePreShort; }
		Framework::LocalisedString const& get_pilgrimage_name_pre_long_ls() const { return pilgrimageNamePreLong; }
		Framework::LocalisedString const& get_pilgrimage_name_pre_connector_ls() const { return pilgrimageNamePreConnector; }

		Array<PilgrimageUtilityRoomDefinition> const& get_utility_rooms() const { return utilityRooms; }
		
		List<PilgrimageLockerRoomSpawn> const& get_locker_room_spawns() const { return lockerRoomSpawns; }

		SimpleVariableStorage const& get_equipment_parameters() const { return equipmentParameters; }

	public: // unlocking
		TagCondition const & get_auto_unlock_on_profile_general_progress() const { return unlocks.autoUnlockOnProfileGeneralProgress; }
		bool should_auto_unlock_when_reached() const { return unlocks.autoUnlockWhenReached; }
		Energy get_experience_for_reaching_for_first_time() const { return unlocks.experienceForReachingForFirstTime; }
		bool can_be_unlocked_for_experience() const { return unlocks.canBeUnlockedForExperience; }
		bool should_appear_as_unlocked_for_experience() const { return unlocks.appearAsUnlockedForExperience; }
		RangeInt const & get_allowed_exm_levels() const { return unlocks.allowedEXMLevels; }

	public: // linear
		bool is_linear() const { return linear.active; }
		PilgrimagePart* linear__get_starting_part(Random::Generator const & _rg) const;
		PilgrimagePart* linear__get_next_part(PilgrimagePart* _part, Random::Generator const & _rg, int _partIdx) const;

	public: // open-world
		bool is_open_world() const { return openWorld.active; }
		PilgrimageOpenWorldDefinition* open_world__get_definition() const { return openWorld.definition.get(); }

	public: // music
		MusicDefinition const& get_music_definition() const { return musicDefinition; }

	public:
		bool can_create_next_pilgrimage() const { return canCreateNextPilgrimage; }

	public: // game director and game related
		bool does_use_game_director() const { return gameDirectorDefinition.useGameDirector; }
		GameDirectorDefinition const& get_game_director_definition() const { return gameDirectorDefinition; }

		bool does_allow_auto_store_of_persistence() const { return allowAutoStoreOfPersistence; }
		bool does_allow_store_intermediate_game_state() const { return allowStoreIntermediateGameState; }
		bool does_allow_store_chapter_start_game_state() const { return allowStoreChapterStartGameState; }

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	public: // custom loader
		bool has_custom_loader() const { return customLoader.is_valid(); }
		Loader::ILoader* create_custom_loader() const;
		Optional<Transform> provide_initial_be_at_right_place_for_custom_loader() const;
		Optional<float> get_starting_point_radius_for_custom_loader() const { return customLoaderStartingPointRadius; }

	protected:
		bool isTemplate = false;

	private:
		struct Linear
		{
			bool active = false;
			Name startingPart;
			Array<RefCountObjectPtr<PilgrimagePart>> parts;
			bool looped = false;
			bool random = false;
		} linear;

		struct OpenWorld
		{
			bool active = false;
			RefCountObjectPtr<PilgrimageOpenWorldDefinition> definition;
		} openWorld;

		Framework::UsedLibraryStored<Pilgrimage> defaultNextPilgrimage; // if not provided, will default to this one

		Framework::LocalisedString pilgrimageName; // the outer rim
		Framework::LocalisedString pilgrimageNamePreShort; // I
		Framework::LocalisedString pilgrimageNamePreLong; // chapter I
		Framework::LocalisedString pilgrimageNamePreConnector; // optional ", " or " - ", it has to be exactly what it is, use &#160; for spaces

		Framework::UsedLibraryStored<Framework::ActorType> pilgrim;
		bool startHereWithEXMsFromLastSetup = false; // exms from persistence, from last setup
		bool startHereWithOnlyUnlockedEXMs = false; // only unlocked exms (persistence) are allowed
		bool startHereWithoutEXMs = false; // no exms
		bool startHereWithoutWeapons = false; // no weapons
		bool startHereWithWeaponsFromArmoury = false; // weapons are taken from armoury (as marked being on pilgrim)
		bool startHereWithDamagedWeapons = false; // if starting from here, weapons are damaged (hardcoded in PilgrimageInstance, damages core, 50%)

		struct Unlocks
		{
			TagCondition autoUnlockOnProfileGeneralProgress;
			bool autoUnlockWhenReached = false;
			Energy experienceForReachingForFirstTime = Energy::zero();
			bool canBeUnlockedForExperience = false;
			bool appearAsUnlockedForExperience = false; // will appear as unlocked if unlocked
			
			RangeInt allowedEXMLevels = RangeInt::empty; // if empty, allows all
		} unlocks;

		MusicDefinition musicDefinition;

		bool canCreateNextPilgrimage = true; // some may not allow to do that (and will require scripts or code to allow it first)

		GameDirectorDefinition gameDirectorDefinition;
		bool allowAutoStoreOfPersistence = false; // useful for briefing, will only store persistence, means that some of the systems may require to update persistence periodically (a good example is armoury storing weapon setup)
		bool allowStoreIntermediateGameState = true; // checkpoint/interrupted
		bool allowStoreChapterStartGameState = true; // chapter start (mostly to hide preludes broken into parts)

		float environmentCycleSpeed = 0.0f;
		Optional<float> lockEnvironmentCycleUntilSeen;
		TagCondition lockEnvironmentCycleUntilSeenRoomTagged;
		Array<RefCountObjectPtr<PilgrimageEnvironmentCycle>> environmentCycles; // if you want to use an environment cycle, add entry in environments as well (same name!)
		Optional<float> forcedNoonYaw;
		Optional<float> forcedNoonPitch;

		Array<PilgrimageEnvironment> environments;

		Array<PilgrimageUtilityRoomDefinition> utilityRooms; // utility rooms are reused by following pilgrimages if the tag matches, if not, they are created/removed
		
		List<PilgrimageLockerRoomSpawn> lockerRoomSpawns;

		Array<Framework::UsedLibraryStoredAny> preloads; // make sure these assets are loaded when starting the pilgrimage

		SimpleVariableStorage equipmentParameters; // params set to weapons created in this pilgrimage

		Name customLoader;
		WheresMyPoint::ToolSet initialBeAtRightPlaceForCustomLoader;
		SimpleVariableStorage customLoaderParams;
		Optional<float> customLoaderStartingPointRadius;
	};
};
