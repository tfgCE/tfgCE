#pragma once

#include "..\game\energy.h"

#include "..\..\framework\library\libraryStored.h"
#include "..\..\framework\library\usedLibraryStored.h"

//

namespace TeaForGodEmperor
{
	struct MissionResult;
	struct MissionStateObjectives;

	struct MissionDefinitionObjectives
	{
	public:
		Energy get_experience_for_success() const { return experienceForSuccess.get(Energy::zero()); }
		int get_merit_points_for_success() const { return meritPointsForSuccess.get(0); }

		Energy get_experience_for_visited_cell() const { return experienceForVisitedCell.get(Energy::zero()); }
		int get_merit_points_for_visited_cell() const { return meritPointsForVisitedCell.get(0); }
		int get_intel_for_visited_cell() const { return intelForVisitedCell.get(0); }

		Energy get_experience_for_visited_interface_box() const { return experienceForVisitedInterfaceBox.get(Energy::zero()); }
		int get_merit_points_for_visited_interface_box() const { return meritPointsForVisitedInterfaceBox.get(0); }
		int get_intel_for_visited_interface_box() const { return intelForVisitedInterfaceBox.get(0); }

		Energy get_experience_for_brought_item() const { return experienceForBroughtItem.get(Energy::zero()); }
		int get_merit_points_for_brought_item() const { return meritPointsForBroughtItem.get(0); }
		int get_intel_for_brought_item() const { return intelForBroughtItem.get(0); }
		TagCondition const& get_bring_items_tagged() const { return bringItemsTagged.get(TagCondition::empty()); }
		Name const& get_bring_items_prob_coef_tag() const { return bringItemsProbCoefTag.get(Name::invalid()); }
		Random::Number<int> const& get_bring_items_type_count() const { return bringItemsTypeCount; }

		Energy get_experience_for_hacked_box() const { return experienceForHackedBox.get(Energy::zero()); }
		int get_merit_points_for_hacked_box() const { return meritPointsForHackedBox.get(0); }
		int get_intel_for_hacked_box() const { return intelForHackedBox.get(0); }
		int get_required_hacked_box_count() const { return requiredHackedBoxCount.get(0); }

		Energy get_experience_for_stopped_infestation() const { return experienceForStoppedInfestation.get(Energy::zero()); }
		int get_merit_points_for_stopped_infestation() const { return meritPointsForStoppedInfestation.get(0); }
		int get_intel_for_stopped_infestation() const { return intelForStoppedInfestation.get(0); }
		int get_required_stopped_infestations_count() const { return requiredStoppedInfestationsCount.get(0); }

		Energy get_experience_for_refill() const { return experienceForRefill.get(Energy::zero()); }
		int get_merit_points_for_refill() const { return meritPointsForRefill.get(0); }
		int get_intel_for_refill() const { return intelForRefill.get(0); }
		int get_required_refills_count() const { return requiredRefillsCount.get(0); }

		String get_description() const;

		String get_summary_description(MissionResult const * _result) const;

		void prepare(OUT_ MissionStateObjectives& _objectives, Random::Generator const& _rg) const;

	public:
		bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

	private:
		Optional<Energy> experienceForSuccess;
		Optional<int> meritPointsForSuccess;

		Optional<Energy> experienceForVisitedCell;
		Optional<int> meritPointsForVisitedCell;
		Optional<int> intelForVisitedCell;

		Optional<Energy> experienceForVisitedInterfaceBox;
		Optional<int> meritPointsForVisitedInterfaceBox;
		Optional<int> intelForVisitedInterfaceBox;

		Optional<Energy> experienceForBroughtItem;
		Optional<int> meritPointsForBroughtItem;
		Optional<int> intelForBroughtItem;
		Optional<TagCondition> bringItemsTagged;
		Optional<Name> bringItemsProbCoefTag; // if tag is not present in the item, it is assumed to be 1
		Random::Number<int> bringItemsTypeCount = 1; // different types

		Optional<Energy> experienceForHackedBox;
		Optional<int> meritPointsForHackedBox;
		Optional<int> intelForHackedBox;
		Optional<int> requiredHackedBoxCount; // how many boxes do we require to consider this a success

		Optional<Energy> experienceForStoppedInfestation;
		Optional<int> meritPointsForStoppedInfestation;
		Optional<int> intelForStoppedInfestation;
		Optional<int> requiredStoppedInfestationsCount; // how many infestations do we require to stop to consider this a success

		Optional<Energy> experienceForRefill;
		Optional<int> meritPointsForRefill;
		Optional<int> intelForRefill;
		Optional<int> requiredRefillsCount; // how many refills do we require to do to consider this a success
	};

	/*
	 *	Intel is two things:
	 *		intel points - integer value that you have to perform certain amount of stuff to make it possible to start a mission
	 *		intel info - tags that make it possible to start a mission
	 */
	struct MissionDefinitionIntel
	{
	public:
		struct ProvideInfo
		{
			float probCoef = 1.0f;
			TagCondition ifInfo;
			Tags setInfo;
			Tags addInfo;
			Tags removeInfo;
			bool load_from_xml(IO::XML::Node const* _node);
		};

	public:
		int get_requires() const { return requires.get(0); }
		
		int get_on_start() const { return onStart.get(0); }
		
		int get_on_success() const { return onSuccess.get(0); }
		int get_on_failure() const { return onFailure.get(0); }
		int get_on_came_back() const { return onCameBack.get(0); }
		int get_on_did_not_come_back() const { return onDidNotComeBack.get(0); }

		Optional<int> const & get_limit_from_objectives() const { return limitFromObjectives; }
		
		Optional<int> const & get_cap() const { return cap; }
		Optional<int> const & get_availability_cap() const { return availabilityCap.is_set()? availabilityCap : cap; }
		
	public:
		TagCondition const & get_requires_info() const { return requiresInfo; }
		Array<ProvideInfo> const & get_info_on_start() const { return infoOnStart; }
		Array<ProvideInfo> const & get_info_on_success() const { return infoOnSuccess; }
		Array<ProvideInfo> const & get_info_on_failure() const { return infoOnFailure; }
		Array<ProvideInfo> const & get_info_on_next_tier() const { return infoOnNextTier; }

	public:
		bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

	private:
		Optional<int> requires; // requires to be possible to be selected
		
		Optional<int> onStart; // processed when mission starts

		// these are processed when mission ends
		Optional<int> onSuccess;
		Optional<int> onFailure;
		Optional<int> onCameBack;
		Optional<int> onDidNotComeBack;
		
		Optional<int> limitFromObjectives; // only from objectives (based on MissionDefinitionsObjectives), onSuccess/onFailure are separate
		
		Optional<int> cap; // can't get overall intel to more than this
		Optional<int> availabilityCap; // if intel exceeds this value, the mission is not available, if not provided will use normal "cap"

		TagCondition requiresInfo; // requires this to be available in the mission menu
		Array<ProvideInfo> infoOnStart; // uses info when mission has been started (not just selected)
		Array<ProvideInfo> infoOnSuccess;
		Array<ProvideInfo> infoOnFailure;
		Array<ProvideInfo> infoOnNextTier;
	};


	class MissionObjectivesAndIntel
	: public Framework::LibraryStored
	{
		FAST_CAST_DECLARE(MissionObjectivesAndIntel);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		MissionObjectivesAndIntel(Framework::Library* _library, Framework::LibraryName const& _name);
		virtual ~MissionObjectivesAndIntel();

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

		MissionDefinitionObjectives const& get_mission_objectives() const { return missionObjectives; }
		
		MissionDefinitionIntel const& get_mission_intel() const { return missionIntel; }

	private:
		MissionDefinitionObjectives missionObjectives;
		
		MissionDefinitionIntel missionIntel;
	};
};
