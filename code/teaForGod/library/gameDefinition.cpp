#include "gameDefinition.h"

#include "..\game\game.h"
#include "..\library\library.h"

#include "..\..\core\tags\tag.h"

#include "..\..\framework\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// tags
DEFINE_STATIC_NAME(progressLevel);

// game progress overrides for loot
DEFINE_STATIC_NAME(ammoRequired);
DEFINE_STATIC_NAME(healthRequired);

//

REGISTER_FOR_FAST_CAST(GameDefinition);
LIBRARY_STORED_DEFINE_TYPE(GameDefinition, gameDefinition);

GameDefinition::ChosenSet* GameDefinition::s_chosenSet = nullptr;

void GameDefinition::initialise_static()
{
	an_assert(s_chosenSet == nullptr);
	s_chosenSet = new ChosenSet();
}

void GameDefinition::close_static()
{
	delete_and_clear(s_chosenSet);
}

void GameDefinition::setup_difficulty_using_chosen(bool _changeExistingKeepAsMuchAsPossible)
{
	auto& gs = TeaForGodEmperor::GameSettings::get();

	// default settings
	{
		if (!_changeExistingKeepAsMuchAsPossible)
		{
			gs.difficulty = TeaForGodEmperor::GameSettings::Difficulty();
		}
		if (auto* gd = GameDefinition::get_chosen())
		{
			if (_changeExistingKeepAsMuchAsPossible)
			{
				gs.difficulty.set_non_game_modifiers_from(gd->get_difficulty().difficulty);
			}
			else
			{
				gs.difficulty = gd->get_difficulty().difficulty; // as base
			}
		}
		for (int i = 0; ; ++i)
		{
			if (auto* gsd = GameDefinition::get_chosen_sub(i))
			{
				gsd->get_difficulty().apply_to(gs.difficulty);
			}
			else
			{
				break;
			}
		}
	}
}

bool GameDefinition::check_chosen(TagCondition const& _tagged)
{
	if (s_chosenSet)
	{
		return _tagged.check(s_chosenSet->tags);
	}
	return false;
}

void GameDefinition::choose(GameDefinition const * _gameDefinition, Array<GameSubDefinition*> const& _gameSubDefinitions)
{
	an_assert(s_chosenSet);
	s_chosenSet->chosenGameDefinition = cast_to_nonconst(_gameDefinition);
	s_chosenSet->chosenGameSubDefinitions.clear();
	for_every_ptr(gsd, _gameSubDefinitions)
	{
		an_assert(s_chosenSet->chosenGameSubDefinitions.has_place_left());
		s_chosenSet->chosenGameSubDefinitions.push_back(GameSubDefinition::GameSubDefinitionSafePtr(gsd));
	}
	if (s_chosenSet->chosenGameSubDefinitions.is_empty())
	{
		if (auto* gd = s_chosenSet->chosenGameDefinition.get())
		{
			if (!gd->get_default_game_sub_definitions().is_empty())
			{
				for_every_ref(gsd, gd->get_default_game_sub_definitions())
				{
					an_assert(s_chosenSet->chosenGameSubDefinitions.has_place_left());
					s_chosenSet->chosenGameSubDefinitions.push_back(GameSubDefinition::GameSubDefinitionSafePtr(gsd));
				}
			}
		}
	}
	s_chosenSet->tags.clear();
	if (auto* gd = s_chosenSet->chosenGameDefinition.get())
	{
		s_chosenSet->tags.set_tags_from(gd->get_tags());
	}
	for_every_ref(gsd, s_chosenSet->chosenGameSubDefinitions)
	{
		s_chosenSet->tags.set_tags_from(gsd->get_tags());
	}

	{	// reupdate using run setup to use test difficulty settings etc
		RunSetup runSetup;
		runSetup.read_into_this();
		runSetup.update_using_this(); // will reset game definition here
	}
}

GameDefinition* GameDefinition::get_chosen()
{
	return s_chosenSet ? s_chosenSet->chosenGameDefinition.get() : nullptr;
}

GameSubDefinition* GameDefinition::get_chosen_sub(int _idx)
{
	return s_chosenSet && s_chosenSet->chosenGameSubDefinitions.is_index_valid(_idx)? s_chosenSet->chosenGameSubDefinitions[_idx].get() : nullptr;
}

GameDefinition::GameDefinition(Framework::Library* _library, Framework::LibraryName const& _name)
: base(_library, _name)
, SafeObject<GameDefinition>(this)
{
}

GameDefinition::~GameDefinition()
{
	make_safe_object_unavailable();
}

PilgrimGear const* GameDefinition::get_default_starting_gear() const
{
	if (!startingGears.is_empty())
	{
		return startingGears.get_first().get();
	}
	return nullptr;
}

PilgrimGear const* GameDefinition::get_starting_gear(int _idx) const
{
	if (!startingGears.is_empty() &&
		startingGears.is_index_valid(_idx))
	{
		return startingGears[_idx].get();
	}
	return nullptr;
}

bool GameDefinition::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("selection")))
	{
		selectionPlacement = node->get_float_attribute_or_from_child(TXT("placement"), selectionPlacement);
		playerProfileRequirements.load_from_xml_attribute_or_child_node(node, TXT("playerProfileRequirements"));
	}

	if (auto* attr = _node->get_attribute(TXT("gameType")))
	{
		if (attr->get_as_string() == TXT("quick game")) type = Type::QuickGame; else
		if (attr->get_as_string() == TXT("experience rules")) type = Type::ExperienceRules; else
		if (attr->get_as_string() == TXT("roguelite")) type = Type::ComplexRules; else
		if (attr->get_as_string() == TXT("complex rules")) type = Type::ComplexRules; else
		if (attr->get_as_string() == TXT("arcade")) type = Type::SimpleRules; else
		if (attr->get_as_string() == TXT("simple rules")) type = Type::SimpleRules; else
		if (attr->get_as_string() == TXT("tutorial")) type = Type::Tutorial; else
		if (attr->get_as_string() == TXT("test")) type = Type::Test; else
		{
			error_loading_xml(_node, TXT("could not recognise gameType \"%S\""), attr->get_as_string().to_char());
			result = false;
		}
	}

	nameForUI.load_from_xml_child(_node, TXT("nameForUI"), _lc, TXT("nameForUI"));
	descForSelectionUI.load_from_xml_child(_node, TXT("descForSelectionUI"), _lc, TXT("descForSelectionUI"));
	descForUI.load_from_xml_child(_node, TXT("descForUI"), _lc, TXT("descForUI"));
	
	result &= lootProgressLevelSteps.load_from_xml(_node->first_child_named(TXT("lootProgressLevel")));

	for_every(node, _node->children_named(TXT("generalDescForUI")))
	{
		GeneralDescForUI gdf;
		gdf.forGameSubDefinitionTagged.load_from_xml_attribute(node, TXT("forGameSubDefinitionTagged"));
		String tagged = String(TXT("general desc; ")) + gdf.forGameSubDefinitionTagged.to_string();
		gdf.generalDescForUI.load_from_xml(node, _lc, tagged.to_char());
		generalDescsForUI.push_back(gdf);
	}

	for_every(containerNode, _node->children_named(TXT("defaultGameSubDefinitions")))
	{
		for_every(node, containerNode->children_named(TXT("gameSubDefinition")))
		{
			Framework::UsedLibraryStored<GameSubDefinition> gsd;
			if (gsd.load_from_xml(node, TXT("name"), _lc))
			{
				defaultGameSubDefinitions.push_back(gsd);
			}
			else
			{
				result = false;
			}
		}
	}

	startingMissionsDefinition.load_from_xml(_node, TXT("startingMissionsDefinition"), _lc);

	pilgrimActorOverride.load_from_xml(_node, TXT("pilgrimActorOverride"), _lc);

	for_every(containerNode, _node->children_named(TXT("pilgrimages")))
	{
		for_every(node, containerNode->children_named(TXT("pilgrimage")))
		{
			Framework::UsedLibraryStored<Pilgrimage> p;
			if (p.load_from_xml(node, TXT("name"), _lc))
			{
				pilgrimages.push_back(p);
			}
			else
			{
				result = false;
			}
		}
	}

	for_every(containerNode, _node->children_named(TXT("conditionalPilgrimages")))
	{
		for_every(node, containerNode->children_named(TXT("pilgrimage")))
		{
			ConditionalPilgrimage cp;
			cp.generalProgressRequirement.load_from_xml_attribute(node, TXT("generalProgressRequirement"));
			cp.gameSlotGeneralProgressRequirement.load_from_xml_attribute(node, TXT("gameSlotGeneralProgressRequirement"));
			cp.profileGeneralProgressRequirement.load_from_xml_attribute(node, TXT("profileGeneralProgressRequirement"));
			if (cp.pilgrimage.load_from_xml(node, TXT("name"), _lc))
			{
				conditionalPilgrimages.push_back(cp);
			}
			else
			{
				result = false;
			}
		}
	}

	for_every(node, _node->children_named(TXT("difficultySettings")))
	{
		difficultySettings = DifficultySettings();
		result &= difficultySettings.load_from_xml(node);
	}

	allowExtraTips = _node->get_bool_attribute_or_from_child_presence(TXT("allowExtraTips"), allowExtraTips);
	allowExtraTips = ! _node->get_bool_attribute_or_from_child_presence(TXT("disallowExtraTips"), !allowExtraTips);

	for_every(containerNode, _node->children_named(TXT("startingGears")))
	{
		for_every(node, containerNode->children_named(TXT("startingGear")))
		{
			RefCountObjectPtr<PilgrimGear> startingGear(new PilgrimGear());
			if (startingGear->load_from_xml(node))
			{
				startingGears.push_back(startingGear);
			}
			else
			{
				result = false;
			}
		}
	}

	defaultPassiveEXMSlotCount = _node->get_int_attribute_or_from_child(TXT("defaultPassiveEXMSlotCount"), defaultPassiveEXMSlotCount);

	weaponCoreModifiers.load_from_xml(_node, TXT("weaponCoreModifiers"));
	error_loading_xml_on_assert(weaponCoreModifiers.is_name_valid(), result, _node, TXT("no \"weaponCoreModifiers\" provided"));

	pilgrimageDevicesTagged.load_from_xml_attribute_or_child_node(_node, TXT("pilgrimageDevicesTagged"));
	
	exmsTagged.load_from_xml_attribute_or_child_node(_node, TXT("exmsTagged"));

	return result;
}

bool GameDefinition::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(gsd, defaultGameSubDefinitions)
	{
		result &= gsd->prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	}

	startingMissionsDefinition.prepare_for_game_find_may_fail(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	pilgrimActorOverride.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	for (int i = 0; i < pilgrimages.get_size(); ++i)
	{
		// we may skip pilgrimages that are not present
		auto& p = pilgrimages[i];
		p.prepare_for_game_find_may_fail(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
		// remove if not found
		if (_pfgContext.get_current_level() >= Framework::LibraryPrepareLevel::Resolve && !p.get())
		{
			pilgrimages.remove_at(i);
			--i;
		}
	}

	weaponCoreModifiers.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	IF_PREPARE_LEVEL(_pfgContext, Framework::LibraryPrepareLevel::Resolve)
	{
		for_every_ref(sg, startingGears)
		{
			an_assert(_library == Library::get_current());
			result &= sg->resolve_library_links();
		}

		for (int i = 0; i < pilgrimages.get_size(); ++i)
		{
			if (!pilgrimages[i].get())
			{
				pilgrimages.remove_at(i);
				--i;
			}
		}
	}

	return result;
}

String const& GameDefinition::get_general_desc_for_ui(Array<Framework::UsedLibraryStored<GameSubDefinition>> const& _gameSubDefinitions) const
{
	for_every(gdf, generalDescsForUI)
	{
		bool ok = false;
		for_every_ref(gsd, _gameSubDefinitions)
		{
			if (gdf->forGameSubDefinitionTagged.check(gsd->get_tags()))
			{
				ok = true;
			}
		}
		if (ok)
		{
			return gdf->generalDescForUI.get();
		}
	}
	return String::empty();
}

bool GameDefinition::check_loot_availability_against_progress_level(Tags const& _tags)
{
	if (!s_chosenSet->chosenGameDefinition.is_set())
	{
		return true;
	}

	float availableProgressLevel = Persistence::get_current().get_progress().get_loot_progress_level();
	if (auto* game = Game::get_as<Game>())
	{
		if (game->is_using_quick_game_player_profile())
		{
			// available up to a certain degree
			todo_note(TXT("set it during setup?"));
			availableProgressLevel = 2.0f;
		}
	}
	float lootProgressLevel = _tags.get_tag(NAME(progressLevel));
	return lootProgressLevel <= availableProgressLevel;
}

Optional<float> GameDefinition::should_override_loot_prob_coef(Name const& _by)
{
	if (!s_chosenSet->chosenGameDefinition.is_set())
	{
		return NP;
	}
	return NP;
}

bool GameDefinition::does_allow_extra_tips()
{
	if (!s_chosenSet->chosenGameDefinition.is_set())
	{
		return true;;
	}

	return s_chosenSet->chosenGameDefinition.get()->allowExtraTips;
}

void GameDefinition::advance_loot_progress_level(REF_ PersistenceProgress& _progressLevel, int _runTimeSeconds)
{
	if (!s_chosenSet->chosenGameDefinition.is_set())
	{
		return;
	}

	s_chosenSet->chosenGameDefinition.get()->lootProgressLevelSteps.advance_progress_level(REF_ _progressLevel.lootProgressLevel, _runTimeSeconds);
}

WeaponCoreModifiers const* GameDefinition::get_weapon_core_modifiers() const
{
	return weaponCoreModifiers.get();
}

//

bool GameProgressLevelSteps::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	if (!_node)
	{
		return result;
	}

	for_every(node, _node->children_named(TXT("step")))
	{
		GameProgressLevelStep step;
		if (step.load_from_xml(node))
		{
			steps.push_back(step);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

void GameProgressLevelSteps::advance_progress_level(REF_ PersistenceProgressLevel& _progressLevel, int _runTimeSeconds) const
{
	GameProgressLevelStep const* step = nullptr;
	for_every_reverse(s, steps)
	{
		if (! s->requiredMilestones.check(_progressLevel.mileStones) || // not reached milestone
			(s->requiredMilestones.is_empty() && s->minProgressLevel100 != NONE && _progressLevel.progressLevel100 < s->minProgressLevel100)) // no milestone requirement but not reached min level
		{
			break;
		}
		step = s;
	}

	if (!step)
	{
		// if the first one has no requirements, use it
		if (!steps.is_empty() && steps[0].requiredMilestones.is_empty())
		{
			step = &steps[0];
		}
	}

	if (!step)
	{
		return;
	}

	if (step->minProgressLevel100 != NONE)
	{
		_progressLevel.progressLevel100 = max(_progressLevel.progressLevel100, step->minProgressLevel100);
	}

	_progressLevel.runTimePendingSeconds += _runTimeSeconds;
	if (step->stepIntervalMinutes > 0)
	{
		int intervalInSeconds = step->stepIntervalMinutes * 60;
		while (_progressLevel.runTimePendingSeconds >= intervalInSeconds)
		{
			_progressLevel.runTimePendingSeconds -= intervalInSeconds;
			_progressLevel.progressLevel100 += step->stepProgressLevel100;
		}
	}
	else
	{
		_progressLevel.runTimePendingSeconds = 0;
	}

	if (step->capProgressLevel100 != NONE)
	{
		_progressLevel.progressLevel100 = min(_progressLevel.progressLevel100, step->capProgressLevel100 - 1);
	}
}

//

bool GameProgressLevelStep::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	if (!_node)
	{
		return result;
	}
	
	requiredMilestones.load_from_xml_attribute(_node, TXT("requiredMilestones"), TagConditionOperator::Or);

	if (auto* attr = _node->get_attribute(TXT("min")))
	{
		minProgressLevel100 = (int)(attr->get_as_float() * 100.0f);
	}
	if (auto* attr = _node->get_attribute(TXT("cap")))
	{
		capProgressLevel100 = (int)(attr->get_as_float() * 100.0f);
	}
	if (auto* attr = _node->get_attribute(TXT("step")))
	{
		stepProgressLevel100 = (int)(attr->get_as_float() * 100.0f);
	}
	if (auto* attr = _node->get_attribute(TXT("stepIntervalMinutes")))
	{
		stepIntervalMinutes = attr->get_as_int();
	}

	return result;
}

//


