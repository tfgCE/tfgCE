#include "missionObjectivesAndIntel.h"

#include "..\game\game.h"
#include "..\library\library.h"
#include "..\utils\reward.h"

#include "..\..\core\random\randomUtils.h"
#include "..\..\core\tags\tag.h"

#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\framework\meshGen\meshGenValueDefImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// localised strings
DEFINE_STATIC_NAME_STR(lsMissionObjectiveVisitedCell, TXT("missions; obj; visited cells"));
	DEFINE_STATIC_NAME(value);
DEFINE_STATIC_NAME_STR(lsMissionObjectiveSummaryVisitedCell, TXT("missions; obj summary; visited cells"));
	DEFINE_STATIC_NAME(visitedCellsCount);

DEFINE_STATIC_NAME_STR(lsMissionObjectiveVisitedInterfaceBox, TXT("missions; obj; visited interface boxes"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveSummaryVisitedInterfaceBox, TXT("missions; obj summary; visited interface boxes"));
	DEFINE_STATIC_NAME(visitedInterfaceBoxesCount);

DEFINE_STATIC_NAME_STR(lsMissionObjectiveBroughtItem, TXT("missions; obj; brought items"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveSummaryBroughtItem, TXT("missions; obj summary; brought items"));
	DEFINE_STATIC_NAME(broughtItemsCount);

DEFINE_STATIC_NAME_STR(lsMissionObjectiveHackBoxesReward, TXT("missions; obj; hacked boxes; reward"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveHackBoxesRequirement, TXT("missions; obj; hacked boxes; requirement"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveSummaryHackBoxes, TXT("missions; obj summary; hacked boxes"));
	DEFINE_STATIC_NAME(hackedBoxesCount);

DEFINE_STATIC_NAME_STR(lsMissionObjectiveStopInfestationsReward, TXT("missions; obj; infestations; reward"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveStopInfestationsRequirement, TXT("missions; obj; infestations; requirement"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveSummaryStopInfestations, TXT("missions; obj summary; infestations"));
	DEFINE_STATIC_NAME(stoppedInfestationsCount);
	DEFINE_STATIC_NAME(stoppedInfestationsLimit);

DEFINE_STATIC_NAME_STR(lsMissionObjectiveRefillsReward, TXT("missions; obj; refills; reward"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveRefillsRequirement, TXT("missions; obj; refills; requirement"));
DEFINE_STATIC_NAME_STR(lsMissionObjectiveSummaryRefills, TXT("missions; obj summary; refills"));
	DEFINE_STATIC_NAME(refillsCount);
	DEFINE_STATIC_NAME(refillsLimit);

DEFINE_STATIC_NAME_STR(lsMissionObjectiveSuccess, TXT("missions; obj; success"));

//

using namespace TeaForGodEmperor;

//

// all lines should be next to each other
static void desc_new_line(REF_ String& description)
{
	if (!description.is_empty())
	{
		while (description.get_right(1) != TXT("~"))
		{
			description += TXT("~");
		}
	}
}

//

REGISTER_FOR_FAST_CAST(MissionObjectivesAndIntel);
LIBRARY_STORED_DEFINE_TYPE(MissionObjectivesAndIntel, missionObjectivesAndIntel);

MissionObjectivesAndIntel::MissionObjectivesAndIntel(Framework::Library* _library, Framework::LibraryName const& _name)
: base(_library, _name)
{
}

MissionObjectivesAndIntel::~MissionObjectivesAndIntel()
{
}

bool MissionObjectivesAndIntel::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// reset
	//
	
	missionObjectives = MissionDefinitionObjectives();
	missionIntel = MissionDefinitionIntel();

	// load
	//

	for_every(n, _node->children_named(TXT("missionObjectives")))
	{
		result &= missionObjectives.load_from_xml(n, _lc);
	}
	
	for_every(n, _node->children_named(TXT("missionIntel")))
	{
		result &= missionIntel.load_from_xml(n, _lc);
	}
	
	return result;
}

bool MissionObjectivesAndIntel::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

//--

bool MissionDefinitionObjectives::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	result &= experienceForSuccess.load_from_xml(_node, TXT("experienceForSuccess"));
	result &= meritPointsForSuccess.load_from_xml(_node, TXT("meritPointsForSuccess"));

	result &= experienceForVisitedCell.load_from_xml(_node, TXT("experienceForVisitedCell"));
	result &= meritPointsForVisitedCell.load_from_xml(_node, TXT("meritPointsForVisitedCell"));
	result &= intelForVisitedCell.load_from_xml(_node, TXT("intelForVisitedCell"));

	result &= experienceForVisitedInterfaceBox.load_from_xml(_node, TXT("experienceForVisitedInterfaceBox"));
	result &= meritPointsForVisitedInterfaceBox.load_from_xml(_node, TXT("meritPointsForVisitedInterfaceBox"));
	result &= intelForVisitedInterfaceBox.load_from_xml(_node, TXT("intelForVisitedInterfaceBox"));

	result &= experienceForBroughtItem.load_from_xml(_node, TXT("experienceForBroughtItem"));
	result &= meritPointsForBroughtItem.load_from_xml(_node, TXT("meritPointsForBroughtItem"));
	result &= intelForBroughtItem.load_from_xml(_node, TXT("intelForBroughtItem"));
	result &= bringItemsTagged.load_from_xml(_node, TXT("bringItemsTagged"));
	result &= bringItemsProbCoefTag.load_from_xml(_node, TXT("bringItemsProbCoefTag"));
	result &= bringItemsTypeCount.load_from_xml(_node, TXT("bringItemsTypeCount"));

	result &= experienceForHackedBox.load_from_xml(_node, TXT("experienceForHackedBox"));
	result &= meritPointsForHackedBox.load_from_xml(_node, TXT("meritPointsForHackedBox"));
	result &= intelForHackedBox.load_from_xml(_node, TXT("intelForHackedBox"));
	result &= requiredHackedBoxCount.load_from_xml(_node, TXT("requiredHackedBoxCount"));

	result &= experienceForStoppedInfestation.load_from_xml(_node, TXT("experienceForStoppedInfestation"));
	result &= meritPointsForStoppedInfestation.load_from_xml(_node, TXT("meritPointsForStoppedInfestation"));
	result &= intelForStoppedInfestation.load_from_xml(_node, TXT("intelForStoppedInfestation"));
	result &= requiredStoppedInfestationsCount.load_from_xml(_node, TXT("requiredStoppedInfestationsCount"));

	result &= experienceForRefill.load_from_xml(_node, TXT("experienceForRefill"));
	result &= meritPointsForRefill.load_from_xml(_node, TXT("meritPointsForRefill"));
	result &= intelForRefill.load_from_xml(_node, TXT("intelForRefill"));
	result &= requiredRefillsCount.load_from_xml(_node, TXT("requiredRefillsCount"));

	return result;
}

String MissionDefinitionObjectives::get_description() const
{
	String desc;

	{
		Energy xp = get_experience_for_visited_cell();
		int pp = get_merit_points_for_visited_cell();
		if (!xp.is_zero() || pp != 0)
		{
			if (!xp.is_zero())
			{
				xp = xp.adjusted_plus_one(GameSettings::get().experienceModifier);
			}
			desc_new_line(REF_ desc);
			
			Reward value(xp, pp);

			desc += Framework::StringFormatter::format_loc_str(NAME(lsMissionObjectiveVisitedCell), Framework::StringFormatterParams()
				.add(NAME(value), value.as_string()));
		}
	}

	{
		Energy xp = get_experience_for_visited_interface_box();
		int pp = get_merit_points_for_visited_interface_box();
		if (!xp.is_zero() || pp != 0)
		{
			if (!xp.is_zero())
			{
				xp = xp.adjusted_plus_one(GameSettings::get().experienceModifier);
			}
			desc_new_line(REF_ desc);
			
			Reward value(xp, pp);

			desc += Framework::StringFormatter::format_loc_str(NAME(lsMissionObjectiveVisitedInterfaceBox), Framework::StringFormatterParams()
				.add(NAME(value), value.as_string()));
		}
	}

	{
		Energy xp = get_experience_for_brought_item();
		int pp = get_merit_points_for_brought_item();
		if (!xp.is_zero() || pp != 0)
		{
			if (!xp.is_zero())
			{
				xp = xp.adjusted_plus_one(GameSettings::get().experienceModifier);
			}
			desc_new_line(REF_ desc);

			Reward value(xp, pp);

			desc += Framework::StringFormatter::format_loc_str(NAME(lsMissionObjectiveBroughtItem), Framework::StringFormatterParams()
				.add(NAME(value), value.as_string()));
		}
	}

	{
		int count = get_required_hacked_box_count();
		if (count > 0)
		{
			desc_new_line(REF_ desc);

			desc += Framework::StringFormatter::format_loc_str(NAME(lsMissionObjectiveHackBoxesRequirement), Framework::StringFormatterParams()
				.add(NAME(value), count));
		}
	}

	{
		Energy xp = get_experience_for_hacked_box();
		int pp = get_merit_points_for_hacked_box();
		if (!xp.is_zero() || pp != 0)
		{
			if (!xp.is_zero())
			{
				xp = xp.adjusted_plus_one(GameSettings::get().experienceModifier);
			}
			desc_new_line(REF_ desc);

			Reward value(xp, pp);

			desc += Framework::StringFormatter::format_loc_str(NAME(lsMissionObjectiveHackBoxesReward), Framework::StringFormatterParams()
				.add(NAME(value), value.as_string()));
		}
	}

	{
		int count = get_required_stopped_infestations_count();
		if (count > 0)
		{
			desc_new_line(REF_ desc);

			desc += Framework::StringFormatter::format_loc_str(NAME(lsMissionObjectiveHackBoxesRequirement), Framework::StringFormatterParams()
				.add(NAME(value), count));
		}
	}

	{
		Energy xp = get_experience_for_stopped_infestation();
		int pp = get_merit_points_for_stopped_infestation();
		if (!xp.is_zero() || pp != 0)
		{
			if (!xp.is_zero())
			{
				xp = xp.adjusted_plus_one(GameSettings::get().experienceModifier);
			}
			desc_new_line(REF_ desc);

			Reward value(xp, pp);

			desc += Framework::StringFormatter::format_loc_str(NAME(lsMissionObjectiveStopInfestationsRequirement), Framework::StringFormatterParams()
				.add(NAME(value), value.as_string()));
		}
	}

	{
		Energy xp = get_experience_for_success();
		int pp = get_merit_points_for_success();
		if (!xp.is_zero() || pp != 0)
		{
			if (!xp.is_zero())
			{
				xp = xp.adjusted_plus_one(GameSettings::get().experienceModifier);
			}
			desc_new_line(REF_ desc);

			Reward value(xp, pp);

			desc += Framework::StringFormatter::format_loc_str(NAME(lsMissionObjectiveSuccess), Framework::StringFormatterParams()
				.add(NAME(value), value.as_string()));
		}
	}

	return desc;
}

String MissionDefinitionObjectives::get_summary_description(MissionResult const * _result) const
{
	String desc;

	{
		Energy xp = get_experience_for_visited_cell();
		int pp = get_merit_points_for_visited_cell();
		if (!xp.is_zero() || pp != 0)
		{
			desc_new_line(REF_ desc);

			desc += Framework::StringFormatter::format_loc_str(NAME(lsMissionObjectiveSummaryVisitedCell), Framework::StringFormatterParams()
				.add(NAME(visitedCellsCount), _result->get_visited_cells()));
		}
	}

	{
		Energy xp = get_experience_for_visited_interface_box();
		int pp = get_merit_points_for_visited_interface_box();
		if (!xp.is_zero() || pp != 0)
		{
			desc_new_line(REF_ desc);

			desc += Framework::StringFormatter::format_loc_str(NAME(lsMissionObjectiveSummaryVisitedInterfaceBox), Framework::StringFormatterParams()
				.add(NAME(visitedInterfaceBoxesCount), _result->get_visited_interface_boxes()));
		}
	}

	{
		Energy xp = get_experience_for_brought_item();
		int pp = get_merit_points_for_brought_item();
		if (!xp.is_zero() || pp != 0)
		{
			desc_new_line(REF_ desc);

			desc += Framework::StringFormatter::format_loc_str(NAME(lsMissionObjectiveSummaryBroughtItem), Framework::StringFormatterParams()
				.add(NAME(broughtItemsCount), _result->get_brought_items()));
		}
	}

	{
		Energy xp = get_experience_for_hacked_box();
		int pp = get_merit_points_for_hacked_box();
		if (!xp.is_zero() || pp != 0)
		{
			desc_new_line(REF_ desc);

			desc += Framework::StringFormatter::format_loc_str(NAME(lsMissionObjectiveSummaryHackBoxes), Framework::StringFormatterParams()
				.add(NAME(hackedBoxesCount), _result->get_hacked_boxes()));
		}
	}

	{
		Energy xp = get_experience_for_stopped_infestation();
		int pp = get_merit_points_for_stopped_infestation();
		if (!xp.is_zero() || pp != 0)
		{
			desc_new_line(REF_ desc);

			desc += Framework::StringFormatter::format_loc_str(NAME(lsMissionObjectiveSummaryStopInfestations), Framework::StringFormatterParams()
				.add(NAME(stoppedInfestationsCount), _result->get_stopped_infestations())
				.add(NAME(stoppedInfestationsLimit), get_required_stopped_infestations_count()));
		}
	}

	{
		Energy xp = get_experience_for_refill();
		int pp = get_merit_points_for_refill();
		if (!xp.is_zero() || pp != 0)
		{
			desc_new_line(REF_ desc);

			desc += Framework::StringFormatter::format_loc_str(NAME(lsMissionObjectiveSummaryRefills), Framework::StringFormatterParams()
				.add(NAME(refillsCount), _result->get_refills())
				.add(NAME(refillsLimit), get_required_refills_count()));
		}
	}

	return desc;
}

void MissionDefinitionObjectives::prepare(OUT_ MissionStateObjectives& _objectives, Random::Generator const& _rg) const
{
	Random::Generator rg = _rg;

	Concurrency::ScopedMRSWLockWrite(_objectives.lock, true);

	// reset
	//

	_objectives.reset();

	// prepare
	//

	if (bringItemsTagged.is_set())
	{
		TagCondition useBringItemsTagged = bringItemsTagged.get();
		if (!useBringItemsTagged.is_empty())
		{
			int count = bringItemsTypeCount.get(rg);
			count = max(1, count);
			struct BringItemType
			{
				Framework::ObjectType* itemType = nullptr;
				float probCoef = 1.0f;
				BringItemType() {}
				void setup(Framework::ObjectType* _itemType, Name const & _bringItemsProbCoefTag)
				{
					itemType = _itemType;
					probCoef = 1.0f;
					if (_bringItemsProbCoefTag.is_valid())
					{
						if (_itemType->get_tags().has_tag(_bringItemsProbCoefTag))
						{
							probCoef = _itemType->get_tags().has_tag(_bringItemsProbCoefTag);
						}
					}
				}
			};
			Name useBringItemsProbCoefTag;
			if (bringItemsProbCoefTag.is_set())
			{
				useBringItemsProbCoefTag = bringItemsProbCoefTag.get();
			}
			Array<BringItemType> chooseFrom;
			for_every_ptr(ls, Library::get_current()->get_actor_types().get_tagged(useBringItemsTagged))
			{
				chooseFrom.push_back(); chooseFrom.get_last().setup(ls, useBringItemsProbCoefTag);
			}
			for_every_ptr(ls, Library::get_current()->get_item_types().get_tagged(useBringItemsTagged))
			{
				chooseFrom.push_back(); chooseFrom.get_last().setup(ls, useBringItemsProbCoefTag);
			}
			count = min(count, chooseFrom.get_size());
			while (count > 0)
			{
				int idx = RandomUtils::ChooseFromContainer<Array<BringItemType>, BringItemType>::choose(
					rg, chooseFrom, [](BringItemType const & _bit) { return _bit.probCoef; });

				_objectives.bringItemsTypes.push_back();
				_objectives.bringItemsTypes.get_last() = chooseFrom[idx].itemType;

				chooseFrom.remove_fast_at(idx);

				--count;
			}
		}
	}
}

//--

bool MissionDefinitionIntel::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	// reset
	//
	requires.clear();
	onStart.clear();
	onSuccess.clear();
	onFailure.clear();
	onCameBack.clear();
	onDidNotComeBack.clear();
	limitFromObjectives.clear();
	cap.clear();
	availabilityCap.clear();

	requiresInfo.clear();
	infoOnStart.clear();
	infoOnSuccess.clear();
	infoOnFailure.clear();
	infoOnNextTier.clear();

	// load
	//

	requires.load_from_xml(_node, TXT("requires"));
	
	onStart.load_from_xml(_node, TXT("onStart"));

	onSuccess.load_from_xml(_node, TXT("onSuccess"));
	onFailure.load_from_xml(_node, TXT("onFailure"));
	onCameBack.load_from_xml(_node, TXT("onCameBack"));
	onDidNotComeBack.load_from_xml(_node, TXT("onDidNotComeBack"));

	limitFromObjectives.load_from_xml(_node, TXT("limitFromObjectives"));

	cap.load_from_xml(_node, TXT("cap"));
	availabilityCap.load_from_xml(_node, TXT("availabilityCap"));

	requiresInfo.load_from_xml_attribute(_node, TXT("requiresInfo"));
	for_every(n, _node->children_named(TXT("infoOnStart")))
	{
		infoOnStart.push_back();
		auto& i = infoOnStart.get_last();
		i.load_from_xml(n);
	}
	for_every(n, _node->children_named(TXT("infoOnSuccess")))
	{
		infoOnSuccess.push_back();
		auto& i = infoOnSuccess.get_last();
		i.load_from_xml(n);
	}
	for_every(n, _node->children_named(TXT("infoOnFailure")))
	{
		infoOnFailure.push_back();
		auto& i = infoOnFailure.get_last();
		i.load_from_xml(n);
	}
	for_every(n, _node->children_named(TXT("onNextTier")))
	{
		infoOnNextTier.push_back();
		auto& i = infoOnNextTier.get_last();
		i.load_from_xml(n);
	}

	return result;
}

//--

bool MissionDefinitionIntel::ProvideInfo::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	probCoef = _node->get_float_attribute_or_from_child(TXT("probCoef"), probCoef);
	ifInfo.load_from_xml_attribute_or_child_node(_node, TXT("ifInfo"));
	setInfo.load_from_xml_attribute_or_child_node(_node, TXT("setInfo"));
	addInfo.load_from_xml_attribute_or_child_node(_node, TXT("addInfo"));
	removeInfo.load_from_xml_attribute_or_child_node(_node, TXT("removeInfo"));

	return result;
}
