#include "missionDefinition.h"

#include "..\game\game.h"
#include "..\library\library.h"
#include "..\utils\tagsStringFormatterParamsProvider.h"
#include "..\utils\variablesStringFormatterParamsProvider.h"

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

using namespace TeaForGodEmperor;

//

static void desc_new_line(REF_ String& description)
{
	if (!description.is_empty())
	{
		while (description.get_right(2) != TXT("~~"))
		{
			description += TXT("~");
		}
	}
}

//

void AvailableMission::get_available_missions(OUT_ Array<AvailableMission>& _availableMissions, MissionDefinitionIntel::ProvideInfo const* _ifProvidedInfo)
{
	_availableMissions.clear();

	auto& p = Persistence::get_current();
	int intelAvailable = p.get_mission_intel();
	Tags intelInfoAvailable = p.get_mission_intel_info();
	if (_ifProvidedInfo)
	{
		intelInfoAvailable.remove_tags(_ifProvidedInfo->removeInfo);
		intelInfoAvailable.add_tags_by_relevance(_ifProvidedInfo->addInfo);
		intelInfoAvailable.set_tags_from(_ifProvidedInfo->setInfo);
	}
	if (auto* ms = MissionState::get_current())
	{
		Random::Generator rg;
		if (auto* set = ms->get_missions_set())
		{
			for_every_ref(m, set->missions)
			{
				auto& mi = m->get_mission_intel();
				if (mi.get_requires() > intelAvailable ||
					!mi.get_requires_info().check(intelInfoAvailable))
				{
					continue;
				}
				bool locked = false;
				int cost = 0;
				if (auto* mb = m->get_mission_biome())
				{
					if (!mb->is_available_to_play())
					{
						// skip this one
						continue;
					}
				}
				if (auto* mgp = m->get_mission_general_progress())
				{
					if (mgp->is_available_to_play())
					{
						// unlocked, good
					}
					else if (mgp->is_available_to_unlock()) // no context, we want all possible missions
					{
						// locked but add it
						locked = true;
						cost = mgp->get_unlock_cost();
					}
					else
					{
						// not available yet
						continue;
					}
				}
				{
					Random::Generator urg = rg.spawn();
					_availableMissions.push_back();
					auto& am = _availableMissions.get_last();
					am.locked = locked;
					am.unlockCost = cost;
					am.intelAvailabilityCap = m->get_mission_intel().get_availability_cap();
					am.mission = m;
					am.mission->get_mission_objectives().prepare(am.objectives, urg);
					am.difficultyModifier = am.mission->select_difficulty_modifier(urg);
				}
			}
		}
	}

	{
		int intel = Persistence::get_current().get_mission_intel();
		bool allAtIntelAvailabilityCap = true;
		for_every(am, _availableMissions)
		{
			if (am->intelAvailabilityCap.is_set())
			{
				if (intel < am->intelAvailabilityCap.get())
				{
					allAtIntelAvailabilityCap = false;
					break;
				}
			}
			else
			{
				// no cap
				allAtIntelAvailabilityCap = false;
				break;
			}
		}
		if (!allAtIntelAvailabilityCap)
		{
			// remove all that are at intel cap (if any)
			for (int i = 0; i < _availableMissions.get_size(); ++i)
			{
				auto& am = _availableMissions[i];
				if (am.intelAvailabilityCap.is_set() &&
					intel >= am.intelAvailabilityCap.get())
				{
					_availableMissions.remove_at(i);
					--i;
				}
			}
		}
		else
		{
			warn(TXT("all missions are at intel cap, we're not achievieng anything now"));
		}
	}
	// limit by tiers
	{
		int maxTier = 0;
		// get max tier
		for_every(am, _availableMissions)
		{
			if (auto* st = am->mission->get_mission_selection_tier())
			{
				am->tier = st->get_tier();
			}
			else
			{
				am->tier = 0;
			}
			maxTier = max(maxTier, am->tier);
		}
		// now get the limits
		for_every(am, _availableMissions)
		{
			if (auto* st = am->mission->get_mission_selection_tier())
			{
				am->tierLimit = st->get_limit_for_tier_diff(maxTier - am->tier);
			}
			else
			{
				am->tierLimit = 0;
			}
		}
		// now equalise tier limits (to max value)
		for_every(am, _availableMissions)
		{
			int maxTierLimit = am->tierLimit;
			// get max and equalise
			for_every(amn, _availableMissions)
			{
				if (amn->tier == am->tier)
				{
					maxTierLimit = max(maxTierLimit, amn->tierLimit);
				}
			}
			am->tierLimit = maxTierLimit;
		}
	}
	// randomise order (we will be sorting by tiers but we still want it to be randomised within tier)
	{
		Random::Generator rg;
		rg.set_seed(Persistence::get_current().get_mission_seed_cumulative(), 0);
		for_count(int, i, 50)
		{
			int a = rg.get_int(_availableMissions.get_size());
			int b = rg.get_int(_availableMissions.get_size());
			if (a != b)
			{
				swap(_availableMissions[a], _availableMissions[b]);
			}
		}
	}
	// remove exceeding
	{
		Array<Random::Generator> rgTiers;
		struct AvailableMissionIndex
		{
			float probCoef;
			int index;
		};
		Array<AvailableMissionIndex> availableMissionIndices;
		while (true)
		{
			bool allOk = true;
			for_every(am, _availableMissions)
			{
				int count = 0;
				for_every(amn, _availableMissions)
				{
					if (amn->tier == am->tier)
					{
						++count;
					}
				}
				if (count > am->tierLimit)
				{
					int useTier = clamp(am->tier, 0, 100);
					while (!rgTiers.is_index_valid(useTier))
					{
						rgTiers.push_back();
						rgTiers.get_last().set_seed(Persistence::get_current().get_mission_seed(rgTiers.get_size() - 1), 0);
					}
					availableMissionIndices.clear();
					for_every(amn, _availableMissions)
					{
						if (amn->tier == am->tier)
						{
							availableMissionIndices.push_back();
							auto& ami = availableMissionIndices.get_last();
							ami.index = for_everys_index(amn);
							ami.probCoef = am->mission->get_prob_coef_from_tags();
							ami.probCoef = ami.probCoef != 0.0f ? 1.0f / ami.probCoef : 100000.0f;
						}
					}
					while (count > am->tierLimit)
					{
						int amiIndex = RandomUtils::ChooseFromContainer<Array<AvailableMissionIndex>, AvailableMissionIndex>::choose(rgTiers[useTier], availableMissionIndices, [](AvailableMissionIndex const& ami) { return ami.probCoef; });
						_availableMissions.remove_fast_at(availableMissionIndices[amiIndex].index);
						availableMissionIndices.remove_at(amiIndex);
						for (int i = amiIndex; i < availableMissionIndices.get_size(); ++i)
						{
							--availableMissionIndices[i].index;
						}
						--count;
					}
					allOk = false;
					break;
				}
			}
			if (allOk)
			{
				break;
			}
		}
	}
	// sort by tier
	sort(_availableMissions, AvailableMission::compare_tier);
}

//

REGISTER_FOR_FAST_CAST(MissionDefinition);
LIBRARY_STORED_DEFINE_TYPE(MissionDefinition, missionDefinition);

MissionDefinition::MissionDefinition(Framework::Library* _library, Framework::LibraryName const& _name)
: base(_library, _name)
{
}

MissionDefinition::~MissionDefinition()
{
}

bool MissionDefinition::load_description_from_xml(Array<DescString> & _descriptions, IO::XML::Node const* _node, tchar const* _childNode, tchar const* _locStrId, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	for_every(n, _node->children_named(_childNode))
	{
		_descriptions.push_back();
		auto& description = _descriptions.get_last();
		description.string.load_from_xml(n, _lc, String::printf(TXT("%S_%i"), _locStrId, _descriptions.get_size() - 1).to_char());
		description.forGameDefinitionTagged.load_from_xml_attribute(n, TXT("forGameDefinitionTagged"));
	}

	return result;
}

bool MissionDefinition::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// reset
	//
	
	missionBiome.clear();
	missionGeneralProgress.clear();
	missionSelectionTier.clear();
	useDifficultyModifersTagged.clear();
	missionType = Framework::LocalisedString();
	missionName = Framework::LocalisedString();
	poiObjective = Framework::LocalisedString();
	missionDescriptions.clear();
	pilgrimages.clear();
	successDescriptions.clear();
	nextTierDescriptions.clear();
	noNextTierDescriptions.clear();
	failedDescriptions.clear();
	notSuccessNotFailedDescriptions.clear();
	cameBackDescriptions.clear();
	didNotComeBackDescriptions.clear();
	diedDescriptions.clear();
	missionObjectivesAndIntel.clear();
	missionObjectives = MissionDefinitionObjectives();
	missionIntel = MissionDefinitionIntel();

	newPilgrimageRandomSeedOnStart = true;
	
	noSuccess = false;
	noFailure = false;

	// load
	//

	missionBiome.load_from_xml(_node, TXT("missionBiome"), _lc);
	missionGeneralProgress.load_from_xml(_node, TXT("missionGeneralProgress"), _lc);
	missionSelectionTier.load_from_xml(_node, TXT("missionSelectionTier"), _lc);
	useDifficultyModifersTagged.load_from_xml_attribute_or_child_node(_node, TXT("useDifficultyModifersTagged"));

	missionType.load_from_xml_child(_node, TXT("missionType"), _lc, TXT("missionType"));
	missionName.load_from_xml_child(_node, TXT("missionName"), _lc, TXT("missionName"));
	poiObjective.load_from_xml_child(_node, TXT("poiObjective"), _lc, TXT("poiObjective"));

	result &= load_description_from_xml(missionDescriptions, _node, TXT("missionDescription"), TXT("missDesc"), _lc);
	result &= load_description_from_xml(successDescriptions, _node, TXT("successDescription"), TXT("succDesc"), _lc);
	result &= load_description_from_xml(nextTierDescriptions, _node, TXT("nextTierDescription"), TXT("nxtTDesc"), _lc);
	result &= load_description_from_xml(noNextTierDescriptions, _node, TXT("noNextTierDescription"), TXT("noNTDesc"), _lc);
	result &= load_description_from_xml(failedDescriptions, _node, TXT("failedDescription"), TXT("failDesc"), _lc);
	result &= load_description_from_xml(notSuccessNotFailedDescriptions, _node, TXT("notSuccessNotFailedDescription"), TXT("nFnSDesc"), _lc);
	result &= load_description_from_xml(cameBackDescriptions, _node, TXT("cameBackDescription"), TXT("backDesc"), _lc);
	result &= load_description_from_xml(didNotComeBackDescriptions, _node, TXT("didNotComeBackDescription"), TXT("notBackDesc"), _lc);
	result &= load_description_from_xml(diedDescriptions, _node, TXT("diedDescription"), TXT("diedDesc"), _lc);

	missionObjectivesAndIntel.load_from_xml(_node, TXT("missionObjectivesAndIntel"), _lc);
	for_every(n, _node->children_named(TXT("missionObjectives")))
	{
		result &= missionObjectives.load_from_xml(n, _lc);
	}
	for_every(n, _node->children_named(TXT("missionIntel")))
	{
		result &= missionIntel.load_from_xml(n, _lc);
	}
	
	XML_LOAD_BOOL_ATTR_CHILD_PRESENCE(_node, newPilgrimageRandomSeedOnStart);
	newPilgrimageRandomSeedOnStart = ! _node->get_bool_attribute_or_from_child_presence(TXT("noNewPilgrimageRandomSeedOnStart"), !newPilgrimageRandomSeedOnStart);
	newPilgrimageRandomSeedOnStart = ! _node->get_bool_attribute_or_from_child_presence(TXT("keepPilgrimageRandomSeedOnStart"), !newPilgrimageRandomSeedOnStart);
	
	XML_LOAD_BOOL_ATTR_CHILD_PRESENCE(_node, noSuccess);
	XML_LOAD_BOOL_ATTR_CHILD_PRESENCE(_node, noFailure);

	for_every(n, _node->children_named(TXT("pilgrimage")))
	{
		Framework::UsedLibraryStored<Pilgrimage> p;
		if (p.load_from_xml(n, TXT("pilgrimage"), _lc))
		{
			pilgrimages.push_back(p);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

bool MissionDefinition::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= missionBiome.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= missionGeneralProgress.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= missionSelectionTier.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	for_every(p, pilgrimages)
	{
		result &= p->prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}

	result &= missionObjectivesAndIntel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

MissionBiome const* MissionDefinition::get_mission_biome() const
{
	return missionBiome.get();
}

MissionGeneralProgress const* MissionDefinition::get_mission_general_progress() const
{
	return missionGeneralProgress.get();
}

MissionSelectionTier const* MissionDefinition::get_mission_selection_tier() const
{
	return missionSelectionTier.get();
}

String MissionDefinition::get_mission_name() const
{
	return missionName.get();
}

String MissionDefinition::get_mission_type() const
{
	return missionType.get();
}

String MissionDefinition::get_poi_objective() const
{
	return poiObjective.get();
}

String MissionDefinition::get_description(Array<DescString> const& _descriptions) const
{
	Concurrency::ScopedSpinLock lock(Persistence::get_current().access_lock());
	TagsStringFormatterParamsProvider tsfpp(Persistence::get_current().access_mission_intel_info());
	VariablesStringFormatterParamsProvider vsfpp(get_custom_parameters());
	Framework::StringFormatterParams formatterParams;
	formatterParams.add_params_provider(&tsfpp);
	formatterParams.add_params_provider(&vsfpp);

	String description;
	for_every(d, _descriptions)
	{
		if (GameDefinition::check_chosen(d->forGameDefinitionTagged))
		{
			String md = Framework::StringFormatter::format_loc_str(d->string, formatterParams);
			desc_new_line(REF_ description);
			description += md;
		}
	}
	return description;
}

String MissionDefinition::get_mission_description() const
{
	return get_description(missionDescriptions);
}

String MissionDefinition::get_success_description() const
{
	return get_description(successDescriptions);
}

String MissionDefinition::get_next_tier_description() const
{
	return get_description(nextTierDescriptions);
}

String MissionDefinition::get_no_next_tier_description() const
{
	return get_description(noNextTierDescriptions);
}

String MissionDefinition::get_failed_description() const
{
	return get_description(failedDescriptions);
}

String MissionDefinition::get_not_success_not_failed_description() const
{
	return get_description(notSuccessNotFailedDescriptions);
}

String MissionDefinition::get_came_back_description() const
{
	return get_description(cameBackDescriptions);
}

String MissionDefinition::get_did_not_come_back_description() const
{
	return get_description(didNotComeBackDescriptions);
}

String MissionDefinition::get_died_description() const
{
	return get_description(diedDescriptions);
}

MissionDefinitionObjectives const& MissionDefinition::get_mission_objectives() const
{
	if (auto* moi = missionObjectivesAndIntel.get())
	{
		return moi->get_mission_objectives();
	}
	return missionObjectives;
}

MissionDefinitionIntel const& MissionDefinition::get_mission_intel() const
{
	if (auto* moi = missionObjectivesAndIntel.get())
	{
		return moi->get_mission_intel();
	}
	return missionIntel;
}

MissionDifficultyModifier const* MissionDefinition::select_difficulty_modifier(Random::Generator& rg) const
{
	Array<MissionDifficultyModifier const*> mdms;
	for_every_ptr(mdm, Library::get_current_as<Library>()->get_mission_dificulty_modifiers().get_tagged(useDifficultyModifersTagged))
	{
		mdms.push_back(mdm);
	}

	if (!mdms.is_empty())
	{
		int idx = RandomUtils::ChooseFromContainer<Array< MissionDifficultyModifier const*>, MissionDifficultyModifier const*>::choose(rg, mdms, [](MissionDifficultyModifier const* _mdm) { return _mdm->get_prob_coef_from_tags(); });
		return mdms[idx];
	}
	
	return nullptr;
}
