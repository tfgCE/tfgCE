#pragma once

#include "missionObjectivesAndIntel.h"
#include "..\game\missionStateObjectives.h"

#include "..\..\framework\text\localisedString.h"

//

namespace TeaForGodEmperor
{
	class MissionBiome;
	class MissionDefinition;
	class MissionDifficultyModifier;
	class MissionGeneralProgress;
	class MissionSelectionTier;
	class Pilgrimage;
	struct MissionResult;

	struct MissionStateObjectives;

	struct AvailableMission
	{
		bool locked = false;
		int unlockCost = 0;
		Optional<int> intelAvailabilityCap; // we should avoid missions that won't give us intel
		MissionDefinition* mission;
		MissionStateObjectives objectives;
		MissionDifficultyModifier const* difficultyModifier;
		int tier = 0;
		int tierLimit = 0; // will maximise this value per tier

		static inline int compare_tier(void const* _a, void const* _b)
		{
			AvailableMission const& a = *plain_cast<AvailableMission>(_a);
			AvailableMission const& b = *plain_cast<AvailableMission>(_b);
			if (a.tier > b.tier) return A_BEFORE_B;
			if (a.tier < b.tier) return B_BEFORE_A;
			return A_AS_B;
		}

		static void get_available_missions(OUT_ Array<AvailableMission>& _availableMissions, MissionDefinitionIntel::ProvideInfo const * _ifProvidedInfo = nullptr);
	};

	class MissionDefinition
	: public Framework::LibraryStored
	{
		FAST_CAST_DECLARE(MissionDefinition);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		MissionDefinition(Framework::Library* _library, Framework::LibraryName const& _name);
		virtual ~MissionDefinition();

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

		Array<Framework::UsedLibraryStored<Pilgrimage>> const & get_pilgrimages() const { return pilgrimages; }

		MissionBiome const* get_mission_biome() const;
		MissionGeneralProgress const* get_mission_general_progress() const;
		MissionSelectionTier const* get_mission_selection_tier() const;

		String get_mission_type() const;
		String get_mission_name() const;
		String get_poi_objective() const;
		String get_mission_description() const;
		String get_success_description() const;
		String get_next_tier_description() const;
		String get_no_next_tier_description() const;
		String get_failed_description() const;
		String get_not_success_not_failed_description() const;
		String get_came_back_description() const;
		String get_did_not_come_back_description() const;
		String get_died_description() const;

		MissionDefinitionObjectives const& get_mission_objectives() const;
		MissionDefinitionIntel const& get_mission_intel() const;

		bool get_new_pilgrimage_random_seed_on_start() const { return newPilgrimageRandomSeedOnStart; }

		bool get_no_success() const { return noSuccess; }
		bool get_no_failure() const { return noFailure; }

	private:
		struct DescString
		{
			Framework::LocalisedString string;
			TagCondition forGameDefinitionTagged;
		};

		Framework::UsedLibraryStored<MissionBiome> missionBiome;
		Framework::UsedLibraryStored<MissionGeneralProgress> missionGeneralProgress;
		Framework::UsedLibraryStored<MissionSelectionTier> missionSelectionTier;
		TagCondition useDifficultyModifersTagged;
		Framework::LocalisedString missionType;
		Framework::LocalisedString missionName;
		Framework::LocalisedString poiObjective; // objective for pilgrim overlay info
		Array<DescString> missionDescriptions;
		Array<Framework::UsedLibraryStored<Pilgrimage>> pilgrimages;

		bool newPilgrimageRandomSeedOnStart = true; // negative in xml is: keepPilgrimageRandomSeedOnStart, noNewPilgrimageRandomSeedOnStart

		bool noSuccess = false; // result can't be success, if so, will be cleared
		bool noFailure = false; // result can't be failed, if so, will be cleared

		Array<DescString> successDescriptions;
		Array<DescString> nextTierDescriptions; // when we unlock next tiers
		Array<DescString> noNextTierDescriptions; // when we don't unlock next tiers
		Array<DescString> failedDescriptions;
		Array<DescString> notSuccessNotFailedDescriptions;
		Array<DescString> cameBackDescriptions;
		Array<DescString> didNotComeBackDescriptions;
		Array<DescString> diedDescriptions;

		Framework::UsedLibraryStored<MissionObjectivesAndIntel> missionObjectivesAndIntel; // if set, will use these instead of inlined
		MissionDefinitionObjectives missionObjectives;
		MissionDefinitionIntel missionIntel;

		bool load_description_from_xml(Array<DescString> & _descriptions, IO::XML::Node const* _node, tchar const * _childNode, tchar const * _locStrId, Framework::LibraryLoadingContext& _lc);
		String get_description(Array<DescString> const& _descriptions) const;

		MissionDifficultyModifier const* select_difficulty_modifier(Random::Generator& rg) const;

		friend struct AvailableMission;
	};
};
