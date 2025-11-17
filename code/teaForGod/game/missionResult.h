#pragma once

#include "..\..\core\functionParamsStruct.h"
#include "..\..\core\memory\safeObject.h"
#include "..\..\framework\library\usedLibraryStored.h"

#include "gameStats.h"
#include "missionStateObjectives.h"
#include "..\library\missionDifficultyModifier.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace TeaForGodEmperor
{
	class MissionDefinition;
	class MissionsDefinition;
	class Pilgrimage;
	struct MissionsDefinitionSet;
	struct MissionState;
	struct PlayerGameSlot;

	struct MissionResult
	: public RefCountObject
	{
	public:
		MissionResult();
		virtual ~MissionResult();

		void set_mission(MissionDefinition* _md, MissionDifficultyModifier* _mdm);
		MissionDefinition* get_mission() const { return missionDefinition.get(); }
		MissionDifficultyModifier* get_difficulty_modifier() const { return difficultyModifier.get(); }

		GameStats const& get_game_stats() const { return stats; }

		MissionStateObjectives const& get_objectives() const { return objectives; }

	public:
		bool has_started() const { return started; }
		bool is_success() const { return success; }
		bool has_next_tier_been_made_available() const { return nextTierMadeAvailable; }
		bool is_failed() const { return failed; }
		bool has_came_back() const { return cameBack; }
		bool has_died() const { return died; }

		MissionDifficultyModifier::BonusCoef const& get_applied_bonus() const { return appliedBonus; }

		int get_visited_cells() const { return visitedCells; }
		int get_visited_interface_boxes() const { return visitedInterfaceBoxes; }
		int get_brought_items() const { return broughtItems; }
		int get_hacked_boxes() const { return hackedBoxes; }
		int get_stopped_infestations() const { return stoppedInfestations; }
		int get_refills() const { return refills; }

	public:
		bool save_to_xml(IO::XML::Node* _node) const;
		bool load_from_xml(IO::XML::Node const * _node);
		bool resolve_library_links();

	private:
		Framework::UsedLibraryStored<MissionDefinition> missionDefinition;
		Framework::UsedLibraryStored<MissionDifficultyModifier> difficultyModifier;

		// - requirement
		// this comes from the mission state
		MissionStateObjectives objectives; // randomised objectives

		bool started = false; // if actually the mission has started (drop capsule launched etc)
		bool cameBack = false;
		bool success = false;
		bool nextTierMadeAvailable = false;
		bool failed = false;
		bool died = false;

		int visitedCells = 0;
		int visitedInterfaceBoxes = 0;
		int broughtItems = 0;
		int hackedBoxes = 0;
		int stoppedInfestations = 0;
		int refills = 0;
		
		int intelFromObjectives = 0;

		MissionDifficultyModifier::BonusCoef appliedBonus;

		GameStats stats;

		void build_pilgrimages_list();

		friend struct MissionState;
	};

};
