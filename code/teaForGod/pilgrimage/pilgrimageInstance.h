#pragma once

#include "..\teaEnums.h"

#include "..\..\core\containers\map.h"
#include "..\..\core\memory\refCountObjectPtr.h"
#include "..\..\core\memory\safeObject.h"
#include "..\..\core\random\random.h"
#include "..\..\core\tags\tagCondition.h"
#include "..\..\framework\library\usedLibraryStored.h"
#include "..\..\framework\world\worldAddress.h"

namespace Framework
{
	class Door;
	class DoorInRoom;
	class DoorType;
	class Environment;
	class EnvironmentType;
	class Region;
	class RegionType;
	class Room;
	class SubWorld;
	class World;
	interface_class IModulesOwner;
};

// store and output
#define s_a_o(...) { String line = String::printf(__VA_ARGS__); output(TXT("%S"), line.to_char()); ::System::FaultHandler::add_custom_info(line); }
#define s_a_o_line(...) { lines.push_back(String::printf(__VA_ARGS__)); }

namespace TeaForGodEmperor
{
	class Game;
	class Pilgrimage;
	class PilgrimageDevice;
	class PilgrimagePart;
	class PilgrimageEnvironmentCycle;
	class PilgrimageEnvironmentMix;
	class PilgrimSetup;
	struct GameState;
	struct StoreGameStateParams;

	struct PilgrimageKilledInfo
	{
		Framework::WorldAddress worldAddress;
		int poiIdx;
	};

	class PilgrimageInstance
	: public RefCountObject
	{
		FAST_CAST_DECLARE(PilgrimageInstance);
		FAST_CAST_END();
	public:
		static void initialise_static();
		static void close_static();
		static void drop_all_pilgrimage_data(bool _dontUnloadJustDrop); // clean up before quit

	public:
		static PilgrimageInstance* get() { return s_current; }
		static PilgrimageInstance* get_based_on_room(Framework::Room* _inRoom);
		bool is_current() const { return this == s_current; }

		PilgrimageInstance(Game* _game, Pilgrimage* _pilgrimage);
		virtual ~PilgrimageInstance();

		Pilgrimage* get_pilgrimage() const { return pilgrimage.get(); }

		PilgrimageInstance* get_next_pilgrimage_instance() const { return nextPilgrimage.get(); }
		Pilgrimage* get_next_pilgrimage() const;

		void set_can_create_next_pilgrimage(bool _canCreateNextPilgrimage = true);

		void set_seed(Random::Generator const & _rg) { randomSeed = _rg; }
		Random::Generator const& get_seed() const { return randomSeed; }

		virtual void store_game_state(GameState* _gameState, StoreGameStateParams const& _params);
		// will add async job
		// use _asNextPilgrimageStart if we're starting the next pilgrimage, note that you should call "will_continue" before
		// each pilgrimage instance gets called create_and_start once with _asNextPilgrimageStart=false and then, if it is a continuation, with _asNextPilgrimageStart=true
		virtual void create_and_start(GameState* _fromGameState = nullptr, bool _asNextPilgrimageStart = false);
		virtual bool has_started() const { return false; }

		virtual void will_continue(PilgrimageInstance const* _pi, Framework::Room* _transitionRoom, Framework::Door* _transitionRoomExitDoor); // setup random seed from the old one etc
		void make_current();
		void do_clean_start_for_game_director();

		virtual void on_end(PilgrimageResult::Type _result); // this is when we're not switching, but ending

		float get_mix_environment_cycle_var(Name const& _environmentCycleName) const;
		void set_mix_environment_cycle_var(Name const& _environmentCycleName, float _value);

		// because utility rooms exist outside of pilgrimage main regions (as they may be kept throughout the game),
		// we may want to find a utility room that was meant for main region of the object that is in that main region
		static void for_our_utility_rooms(Framework::IModulesOwner* _forObject, std::function<void(Framework::Room* _room)> _do);

		void adjust_starting_pilgrim_setup(PilgrimSetup& _pilgrimSetup) const;

		static void reset_deferred_job_counters();

		void set_manage_transition_room_entrance_door(bool _manageTransitionRoomDoors = true) { manageTransitionRoomDoors = _manageTransitionRoomDoors; }

		Framework::Region* get_main_region() const { return mainRegion.get(); }

	public:
		virtual void pre_advance();
		virtual void advance(float _deltaTime);

		Framework::Room* get_pilgrims_destination(OPTIONAL_ OUT_ Name* _actualSystem = nullptr, OPTIONAL_ OUT_ PilgrimageDevice const** _actualDevice = nullptr, OPTIONAL_ OUT_ Framework::IModulesOwner** _actualDeviceIMO = nullptr) const;
		void reset_pilgrims_destination(); // useful when we want to update because our guidance target has changed

	public:
		virtual void debug_log_environments(LogInfoContext& _log) const;
		virtual void log(LogInfoContext & _log) const;

	public:
		bool has_been_killed(Framework::WorldAddress const& _worldAddress, int _poiIndex) const;
		void mark_killed(Framework::WorldAddress const& _worldAddress, int _poiIndex);

	public:
		bool is_follow_guidance_blocked() const;

	public:
		int get_unlocked_upgrade_machines() const { return upgradeMachinesUnlocked; }
		void set_unlocked_upgrade_machines_to(int _unlocked) { upgradeMachinesUnlocked = _unlocked; }

	public:
		void set_environment_rotation_cycle_length(float _length) { environmentRotationCycleLength = (double)_length; }
		float get_environment_rotation_yaw() const { return (float)environmentRotationYaw; }

	public:
		void modify_advance_environment_cycle_by(float _value);

	protected:
		bool forceInstantObjectCreation = false;
		void refresh_force_instant_object_creation(Optional<bool> const& _newValue = NP);

	protected:
		static PilgrimageInstance* s_current;

		static Concurrency::Counter s_pilgrimageInstance_deferredEndJobs;

		float currentFor = 0.0f;

		Game* game = nullptr;
		Framework::UsedLibraryStored<Pilgrimage> pilgrimage;

		RefCountObjectPtr<PilgrimageInstance> nextPilgrimage;

		bool requestSavingGameStateAsChapterStart = false;

		Random::Generator randomSeed; // will be offset to get seed for stations, parts etc
		
		bool generatedBase = false;
		int upgradeMachinesUnlocked = 0;

		Framework::SubWorld* subWorld = nullptr;
		SafePtr<Framework::Region> mainRegion; // containing everything, environments too

		SafePtr<Framework::Room> pilgrimsDestination; // room where we're going
		SafePtr<Framework::IModulesOwner> pilgrimsDestinationDeviceMainIMO; // the main device (not "guidanceTowards")
		PilgrimageDevice const* pilgrimsDestinationDevice = nullptr;
		Name pilgrimsDestinationSystem = Name::invalid(); // if the target is an upgrade room 

		bool isBusyAsync = false;
		bool firstAdvance = true;

		bool lockerRoomSpawnsDone = false;

		struct TransitionRoom
		{
			SafePtr<Framework::Room> room;
			SafePtr<Framework::Door> entranceDoor; // might be the same as exit door
			SafePtr<Framework::Door> exitDoor;
			bool manageEntranceDoor = true;
			bool allowEntranceDoorAuto = true;
			Optional<float> manageEntranceDoorAtLeastIn;
		};
		Array<TransitionRoom> transitionRooms;
		bool manageTransitionRoomDoors = true;

		bool canCreateNextPilgrimage = true;
		bool issuedCreateNextPilgrimage = false;
		bool switchToNextPilgrimage = false;

		Map<Name, SafePtr<Framework::Room>> environments;
		float environmentPt = 0.0f; // to blend between prev and next part's environments
		struct EnvironmentMix
		{
			Name name;
			struct Part
			{
				float mix = 0.0f;
				float target = 0.0f;
				Framework::EnvironmentType* environment = nullptr;
				PilgrimageEnvironmentCycle const* fromCycle = nullptr;

				Part() {}
				Part(Framework::EnvironmentType* _environment, float _mix, PilgrimageEnvironmentCycle const* _fromCycle) : mix(_mix), environment(_environment), fromCycle(_fromCycle) {}
			};
			Array<Part> parts;

			EnvironmentMix() {}
			EnvironmentMix(Name const & _name, Framework::EnvironmentType* _environment);

			void clear_targets();
			void advance(float _deltaTime, Framework::Environment* _alter, bool _firstAdvance);
			void remove_unused();
		};
		Array<EnvironmentMix> environmentMixes;
		bool environmentMixesReadyToUse = false;
		float environmentCycleSpeed = 0.0f; // if zero, aligns with normal enviro pt
		float currentEnvironmentCycleSpeed = 0.0f;
		int environmentCycleIdx = 0; // in which part of cycle are we, we should have same number of cycles! (at least ATM, it should be variable)
		float environmentCyclePt = 0.0f;
		Optional<float> lockEnvironmentCycleUntilSeen;
		TagCondition lockEnvironmentCycleUntilSeenRoomTagged;
		Array<float> mixEnvironmentCycleVars;

		// this is used to rotation environment and rotate specific objects in background - used for train ride to travel around the tower
		// double for higher precision as it may require long time to do a full circle
		double environmentRotationYaw = 0.0;
		double environmentRotationCycleLength = 0.0; // can be negative to go the other way around

		// killed info (for those that want to keep it)
		mutable Concurrency::MRSWLock killedInfoLock;
		Array<PilgrimageKilledInfo> killedInfos;

		void async_generate_base(bool _asNextPilgrimageStart); // sub world, main region, environments
		virtual Framework::RegionType* get_main_region_type() const { return nullptr; }

		bool get_poi_placement(Framework::Room* _station, Name const & _poiName, Optional<Name> const& _optionalPOIName, OUT_ Transform & _vrAnchor, OUT_ Transform & _placement, ShouldAllowToFail _allowToFail = ShouldAllowToFail::DisallowToFail) const;

		Framework::Door* create_door(Framework::DoorType* _doorType, Framework::Room* _room = nullptr, Tags const& _tags = Tags::none) const;
		Framework::DoorInRoom* create_door_in_room(Framework::Door* _door, Framework::Room* _room, Tags const& _tags) const;

		Framework::DoorInRoom* create_and_place_door_in_room(Framework::Door* _door, Framework::Room* _room, Tags const& _tags, Name const& _atPOINamed, Optional<Name> const& _atPOINamedAlt = NP) const;

		void advance_environments(float _deltaTime, PilgrimageInstance* _beShadowOf);

		virtual void add_environment_mixes_if_no_cycles() {}
		void add_mix(Array<RefCountObjectPtr<PilgrimageEnvironmentMix>> const & _environments, float _strength, PilgrimageEnvironmentCycle const * _fromCycle = nullptr);
		void add_mix(PilgrimageEnvironmentMix const * _environmentMix, float _strength, PilgrimageEnvironmentCycle const * _fromCycle = nullptr);

		void update_auto_environment_variables(Framework::Environment* _env);

		void wait_for_no_world_jobs_to_end_generation();

		virtual String get_random_seed_info() const;

		float noonYaw = -20.0f; // carry from a previous one, leave as it is?

		float get_noon_yaw() const { return noonYaw; }

		void make_no_longer_current(PilgrimageInstance const * _switchingTo = nullptr);

		void issue_utility_rooms_creation(bool _keepUtilityRoomsOfCurrentPilgrimage);
		void async_create_utility_rooms(bool _keepUtilityRoomsOfCurrentPilgrimage);

		void issue_create_next_pilgrimage();
		void async_create_next_pilgrimage();
		virtual void post_create_next_pilgrimage();
		void issue_switch_to_next_pilgrimage(); // this is called when door has been closed

		TransitionRoom async_create_transition_room_to_next_pilgrimage();
		virtual TransitionRoom async_create_transition_room_for_previous_pilgrimage(PilgrimageInstance * _previousPilgrimage) = 0;

		virtual void post_setup_rgi() {} // to allow info output
	};
};
