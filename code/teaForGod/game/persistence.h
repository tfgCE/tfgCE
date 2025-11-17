#pragma once

#include "weaponPart.h"
#include "weaponSetup.h"

#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\containers\array.h"
#include "..\..\core\math\math.h"
#include "..\..\core\memory\safeObject.h"
#include "..\..\core\types\name.h"

#include "..\..\framework\library\usedLibraryStored.h"

#include "..\gameplayBalance.h"

#include "..\library\exmType.h"

//

#ifndef INVESTIGATE_SOUND_SYSTEM
	#define OUTPUT_PERSISTENCE_LAST_SETUP
#endif

//

namespace Framework
{
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class GameDefinition;
	class Persistence;
	class MissionDefinition;
	class MissionGeneralProgress;
	class ModulePilgrim;
	struct PilgrimHandEXMSetup;
	struct WeaponSetup;

	struct PersistenceInfoProgress
	{
		enum State
		{
			No,
			ToShow,
			Done
		};

		State welcomeMessagePresented = No;
		State loadoutSetupAvailable = No;

		bool load_from_xml(IO::XML::Node const* _node);
		bool save_to_xml(IO::XML::Node* _node) const;

		void force_to_show_if_no(); 
		void advance_to_show(Persistence const & persistence);
		void on_pilgrimage_started();
	};

	struct PersistenceProgressLevel
	{
		Tags mileStones;
		int progressLevel100 = 0;
		int runTimePendingSeconds = 0;

		bool load_from_xml(IO::XML::Node const* _node);
		bool save_to_xml(IO::XML::Node* _node) const;
	};

	struct PersistenceProgress
	{
		bool load_from_xml(IO::XML::Node const* _node);
		bool save_to_xml(IO::XML::Node* _node) const;

		float get_loot_progress_level() const { return (float)lootProgressLevel.progressLevel100 / 100.0f; }
		
		void set_loot_progress_level_at_least(float _level) { lootProgressLevel.progressLevel100 = max(lootProgressLevel.progressLevel100, (int)(_level * 100.0f)); }

		void set_loot_progress_milestones(Tags const& _milestones) { lootProgressLevel.mileStones.set_tags_from(_milestones); }

	private:
		PersistenceProgressLevel lootProgressLevel;

		friend class GameDefinition;
	};

	class Persistence
	: public SafeObject<Persistence>
	{
	public:
		struct UsedSetup
		{
			// whenever we change something we should update last used setup
			Name activeEXMs[Hand::MAX];
			Array<Name> passiveEXMs[Hand::MAX];
			Array<Name> permanentEXMs;
			
			Array<Name> availableEXMs;

			void clear();
			bool load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childName);
			bool save_to_xml_child_node(IO::XML::Node* _node, tchar const* _childName) const;
		};

		struct WeaponOnPilgrimInfo
		{
			bool onPilgrim = false;
			Optional<Hand::Type> mainWeaponForHand;
			Optional<int> inPocket;

			void fill(Framework::IModulesOwner* pilgrim, Framework::IModulesOwner* item);

			bool operator==(WeaponOnPilgrimInfo const& _other) const { return onPilgrim == _other.onPilgrim && mainWeaponForHand == _other.mainWeaponForHand && inPocket == _other.inPocket; }
			bool operator!=(WeaponOnPilgrimInfo const& _other) const { return ! operator==(_other); }
		};

		struct WeaponInArmoury
		{
			// armoury setup is done only when we're really near to the armoury, which means that the player decided to use the armoury
			// therefore it is possible that some weapons don't have id set
			// alreadyExists is used only when getting back to the base, when mission ends and weapons are added to the armoury and we keep some of the weapons on us - we bring existing weapons in

			Optional<int> id; // this is given to make it easier to associate certain weapons, this is valid for game time mostly, for ai logic responsible for armoury management
			// id is always set, if there's an exiting weapon, it will 
			// if not set, will be put anywhere, this is used for new weapons or weapons left in weapon container (unless the pilgrimage has started, if so only weapons with valid at will be kept)
			// x - relative to central column
			// y - from bottom
			Optional<VectorInt2> at;
			WeaponOnPilgrimInfo onPilgrimInfo; // if we save on the ship, we should store on which hand the weapon is
			bool alreadyExists = false; // set by setup when weapon may already exist in the world
			Framework::IModulesOwner* imo = nullptr; // this is just a reference to catch quicker an existing weapon, not stored in a save, avoid using directly
			WeaponSetup setup;

			WeaponInArmoury() : setup(nullptr) {}
			WeaponInArmoury(WeaponInArmoury const& _other) : id(_other.id), at(_other.at), onPilgrimInfo(_other.onPilgrimInfo), alreadyExists(_other.alreadyExists), imo(_other.imo), setup(_other.setup.get_persistence()) { setup.copy_from(_other.setup); }
			WeaponInArmoury(Persistence* _persistence) : setup(_persistence) {}
		};

		// used by setup_weapons_in_armoury_for_game
		struct ExistingWeaponFromArmoury
		{
			int id; // this should be always set as it is returned by setup_weapons_in_armoury_for_game
			Optional<VectorInt2> at; // if we're aware of some guns we should provide "at" if they are placed on the actual slots
			::SafePtr<Framework::IModulesOwner> imo;
		};

	public:
		// this will access persistence from the current game slot (which means it could be tutorial's)
		static Persistence* access_current_if_exists();
		static Persistence & access_current();
		static Persistence const& get_current();

		Concurrency::SpinLock& access_lock() const { return persistenceLock; }

		Persistence();
		virtual ~Persistence();

		void copy_from(Persistence const& _source);

		void setup_auto();

		UsedSetup const& get_last_used_setup() const { return lastUsedSetup; }
		void store_last_setup(ModulePilgrim* _pilgrim);

	public:
		void start_pilgrimage();
		void end_pilgrimage();

	public:
		Array<Name>& access_all_exms() { return allEXMs; }
		Array<Name> const& get_all_exms() const { return allEXMs; }
		void cache_exms();

		Array<Name> const & get_unlocked_exms() const { return unlockedEXMs; }

		Array<Name> const & get_permanent_exms() const { return permanentEXMs; }

		PersistenceInfoProgress& access_info_progress() { return infoProgress; }
		PersistenceInfoProgress const& get_info_progress() const { return infoProgress; }

		PersistenceProgress& access_progress() { return progress; }
		PersistenceProgress const& get_progress() const { return progress; }

		void mark_killed(Framework::IModulesOwner const* _imo);
		int get_kill_count(Framework::IModulesOwner const* _imo) const;

		void provide_experience(Energy const& _experience);
		Energy get_experience_to_spend() const { Energy exp; { Concurrency::ScopedSpinLock lock(persistenceLock, true); exp = experienceToSpend; } return exp; }
		bool spend_experience(Energy const& _experience);

		void provide_merit_points(int _meritPoints);
		int get_merit_points_to_spend() const { int pp; { Concurrency::ScopedSpinLock lock(persistenceLock, true); pp = meritPointsToSpend; } return pp; }
		bool spend_merit_points(int _meritPoints);

		void add_mission_intel(int _intel, MissionDefinition const* _mission);
		int get_mission_intel() const { int ip; { Concurrency::ScopedSpinLock lock(persistenceLock, true); ip = missionIntel; } return ip; }

		void add_mission_intel_info(Tags const & _add);
		void set_mission_intel_info(Tags const & _set);
		void remove_mission_intel_info(Tags const & _clear);
		Tags get_mission_intel_info() const { Tags info; { Concurrency::ScopedSpinLock lock(persistenceLock, true); info = missionIntelInfo; } return info; }
		Tags & access_mission_intel_info() { an_assert(persistenceLock.is_locked_on_this_thread()); return missionIntelInfo; }
		Tags const & access_mission_intel_info() const { an_assert(persistenceLock.is_locked_on_this_thread()); return missionIntelInfo; }

		void set_mission_seed(int _tierIdx, int _seed);
		void advance_mission_seed(int _tierIdx, int _by = 1);
		int get_mission_seed(int _tierIdx) const;
		int get_mission_seed_cumulative() const;

		void add_mission_general_progress_info(Tags const& _add);
		void remove_mission_general_progress_info(Tags const& _clear);
		Tags get_mission_general_progress_info() const { Tags info; { Concurrency::ScopedSpinLock lock(persistenceLock, true); info = missionGeneralProgressInfo; } return info; }

		bool unlock_mission_general_progress(MissionGeneralProgress const * _mgp); // returns true if unlocked
		bool is_mission_general_progress_unlocked(MissionGeneralProgress const* _mgp) const;

	public:
		// there should be only one user for weapons in armoury in the game, armoury logic

		/**
		 *	Armoury is a place where we can select weapons.
		 *	If we start a pilgrimage (endless mode), we start with nothing. We select weapons in the armoury.
		 *	1.	When we leave the armoury, we take the weapons with us and on the pilgrimage switch, the weapons held by Pilgrim (or in the transition room!) are removed from the persistence.
		 *		This is handled by the armoury (Armoury::on_pilgrimage_instance_switch)
		 *	2.	When we end the pilgrimage, the weapons are added to the persistence (ModulePilgrim::add_gear_to_persistence)
		 *		This should be done explicitly by game script when we're ending the mission (gse armoury)
		 *	3.	If we interrupt the pilgrimage, and we're still around the armoury, we rearrange all the weapons.
		 *		If we're not around the armoury, this means we're during the mission and we should not mess with persistence.
		 */
		static int get_weapons_in_armoury_rows() { return hardcoded 3; }
		static float get_weapons_in_armoury_column_spacing() { return hardcoded 0.3f; }
		static float get_weapons_in_armoury_row_spacing() { return hardcoded 0.3f; }

		int get_max_weapons_in_armoury() const { return get_weapons_in_armoury_columns() * get_weapons_in_armoury_rows(); }
		int get_base_weapons_in_armoury_columns() const { return baseWeaponsInArmouryColumns; }
		int get_weapons_in_armoury_columns() const { return weaponsInArmouryColumns; }
		void set_weapons_in_armoury_columns(int _weaponsInArmouryColumns) { weaponsInArmouryColumns = _weaponsInArmouryColumns; }
		RangeInt get_weapons_in_armoury_columns_range() const;
		List<WeaponInArmoury> const& get_weapons_in_armoury() const { return weaponsInArmoury; }
		List<WeaponInArmoury> & access_weapons_in_armoury() { return weaponsInArmoury; }
		void add_weapon_to_armoury(WeaponSetup const& _setup, bool _allowDuplicates = false, bool _allowTooMany = false, Framework::IModulesOwner* _imo = nullptr, Optional<WeaponOnPilgrimInfo> const & _onPilgrimInfo = NP); // any weapon, anywhere

		bool remove_weapon_from_armoury(int _id);
		bool update_weapon_in_armoury(int _id, WeaponSetup const & _setup, Optional<VectorInt2> const & _at = NP, Optional<WeaponOnPilgrimInfo> const& _onPilgrimInfo = NP);
		bool move_weapon_in_armoury_to_end(WeaponSetup const& _setup); // might be useful when weapon has been discarded and we may decide to ignore it when there's not enough space

		void setup_weapons_in_armoury_for_game(Framework::IModulesOwner* _armoury = nullptr, OUT_ Array<ExistingWeaponFromArmoury> * _existingWeapons = nullptr, bool _armouryIgnoreExistingWeapons = false); // will fill id values, place wherever possible, if detects that weapons exists, will add it to _existingWeapons, may ignore existing weapons if we're supposed to add them again
		bool discard_invalid_weapons_from_armoury(); // discard if don't have ids or at set or at is outside the range

	public:
		void do_for_every_experience_gained(int _limit, std::function<void(Energy const& _amount)> _do);
		int get_experience_gained_ver() const { return experienceGainedVer; }

	public:
		bool load_from_xml_child_node(IO::XML::Node const* _node, tchar const * _childName);
		bool save_to_xml_child_node(IO::XML::Node * _node, tchar const* _childName) const;
		bool resolve_library_links();

	private:
		mutable Concurrency::SpinLock persistenceLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.Persistence.persistenceLock"));

		// don't forget to copy!

		// allEXMs double functionality that's why whenever we do anything to allEXMs, we should call cache_exms()
		Array<Name> allEXMs; // works as a history
		CACHED_ Array<Name> unlockedEXMs;
		CACHED_ Array<Name> permanentEXMs;

		UsedSetup lastUsedSetup;

		Energy experienceToSpend = Energy::zero();
		int meritPointsToSpend = 0;
		int missionIntel = 0;
		Tags missionIntelInfo;
		
		Tags missionGeneralProgressInfo;
		Array<Framework::UsedLibraryStored<MissionGeneralProgress>> unlockedMissionGeneralProgress; // only if requires unlock

		Array<int> missionSeeds; // this is used to choose available missions. it is advanced when mission is ended. this is stored per tier!

		struct ExperienceGained
		{
			Energy amount;
			::System::TimeStamp when;
		};
		List<ExperienceGained> experienceGained; // not saved, just for display purposes
		int experienceGainedVer = NONE;

		// weapons in the armoury
		const int baseWeaponsInArmouryColumns = 4;
		int weaponsInArmouryColumns = 4; // there are always the same number of rows (including base)
		List<WeaponInArmoury> weaponsInArmoury;

		PersistenceInfoProgress infoProgress;
		
		struct KillInfo
		{
			Framework::LibraryName what;
			int count = 0;
		};
		Array<KillInfo> killInfos;

		PersistenceProgress progress;
		bool pilgrimageInProgress = false;

		void remove_too_old_experiences_gained();
	};

};

