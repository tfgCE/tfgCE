#pragma once

#include "missionStateObjectives.h"
#include "..\library\missionObjectivesAndIntel.h"

#include "..\..\core\functionParamsStruct.h"
#include "..\..\core\memory\safeObject.h"
#include "..\..\framework\library\usedLibraryStored.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	class Room;
	class ObjectType;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class MissionDefinition;
	class MissionDifficultyModifier;
	class MissionsDefinition;
	class Pilgrimage;
	struct Energy;
	struct MissionsDefinitionSet;
	struct PlayerGameSlot;

	/*
		This structure is used by pilgrimage instance to tell the next pilgrimage
		It uses mission definition set to get first and last pilgrimages and mission definition to get the middle part
	*/
	struct MissionState
	: public RefCountObject
	{
	public:
		static void find_current(); // from game slot
		static MissionState* get_current() { return s_current; }

		MissionState();
		virtual ~MissionState();

		bool is_ready_to_use() const { return isReadyToUse; }
		bool has_started() const { return started; }

		void prepare_for(PlayerGameSlot* _gs);

		MissionsDefinitionSet* get_missions_set() const { return missionsDefinitionSet.get(); }

		void set_mission(MissionDefinition* _md, MissionStateObjectives const * _objectives, MissionDifficultyModifier const * _difficultyModifier);
		MissionDefinition* get_mission() const { return missionDefinition.get(); }
		MissionDifficultyModifier * get_difficulty_modifier() const { return difficultyModifier.get(); }

		bool does_require_items_to_bring() const;
		bool is_item_required(Framework::IModulesOwner* _imo) const;
		void process_brought_item(Framework::IModulesOwner* _imo);
		
		MissionStateObjectives const& get_objectives() const { return objectives; }

		void prepare_for_gameplay(); // find usedLibraryStored 

		void ready_for(PlayerGameSlot* _gs);

		void use_intel_on_start();
		void store_starting_state();
		
		void mark_started();
		void mark_came_back();
		void mark_success();
		void mark_failed();
		void mark_died();

		void visited_cell();
		void visited_interface_box();

		void hacked_box();

		void stopped_infestation();
		
		void refilled();

	public:
		void set_starting_max_mission_tier(int _maxTier) { startingMaxMissionTier = _maxTier; }

	public:
		int get_visited_cells() const { return visitedCells; }
		int get_visited_interface_boxes() const { return visitedInterfaceBoxes; }
		int get_held_items_to_bring(Framework::IModulesOwner* imo) const;
		int get_already_brough_items() const { return broughtItems; }
		int get_hacked_boxes() const { return hackedBoxes; }
		int get_stopped_infestations() const { return stoppedInfestations; }
		int get_refills() const { return refills; }

	public:
		struct EndParams
		{
			ADD_FUNCTION_PARAM_PLAIN(EndParams, Array<Framework::Room*>, processItemsInRooms, process_items_in_rooms);
		};
		// will store weapons in armoury etc
		void end(EndParams const & _params);

	private:
		void process_end(EndParams const& _params);

	public:
		static void async_abort(bool _save = true);

	private:
		static void async_on_end(bool _save = true);

	public:
		bool save_to_xml(IO::XML::Node* _node) const;
		bool load_from_xml(IO::XML::Node const * _node);

		Array<Framework::UsedLibraryStored<Pilgrimage>> const& get_pilgrimages() const { return pilgrimages; }

	private:
		static MissionState* s_current;

		bool isReadyToUse = false;

		Framework::UsedLibraryStored<MissionsDefinition> missionsDefinition;
		Name missionsDefinitionSetId; // to restore from game definition's missions definition
		RefCountObjectPtr<MissionsDefinitionSet> missionsDefinitionSet; // looked up
		Framework::UsedLibraryStored<MissionDefinition> missionDefinition;

		CACHED_ Array<Framework::UsedLibraryStored<Pilgrimage>> pilgrimages;

		// - requirement
		// this is set when preparing a mission
		MissionStateObjectives objectives; // randomised objectives
		Framework::UsedLibraryStored<MissionDifficultyModifier> difficultyModifier;

		// - starting state
		// this is used to calculate bonus from mission difficulty modifier
		struct StartingState
		{
			Optional<Energy> experience;
			Optional<int> meritPoints;
		} startingState;

		// - results
		Concurrency::SpinLock resultsLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.MissionState.resultsLock"));
		bool started = false;
		bool cameBack = false;
		bool success = false;
		bool failed = false;
		bool died = false;
		int visitedCells = 0;
		int visitedInterfaceBoxes = 0;
		int broughtItems = 0;
		int hackedBoxes = 0;
		int stoppedInfestations = 0;
		int refills = 0;

		int startingMaxMissionTier = 0;
		int intelFromObjectives = 0;

		void build_pilgrimages_list();

		void add_intel_from_objectives(int _intel);

		bool give_rewards(Energy xp, int mp, int ip, bool _applyGameSettingsModifiers = true); // returns true if anything given

		void apply_intel_info(Array<MissionDefinitionIntel::ProvideInfo> const& _info);
		void apply_intel_info(MissionDefinitionIntel::ProvideInfo const*);
		MissionDefinitionIntel::ProvideInfo const * get_intel_info(Array<MissionDefinitionIntel::ProvideInfo> const& _info);
	};

};
