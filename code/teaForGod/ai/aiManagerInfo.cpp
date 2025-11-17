#include "aiManagerInfo.h"

#include "aiManagerData.h"
#include "..\game\game.h"
#include "..\library\library.h"
#include "..\modules\moduleAI.h"

#include "..\..\framework\ai\aiMindInstance.h"
#include "..\..\framework\library\library.h"
#include "..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\framework\module\moduleAI.h"
#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\object\scenery.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;

//

// library global referencer
DEFINE_STATIC_NAME_STR(gr_aiManager, TXT("ai manager"));
DEFINE_STATIC_NAME_STR(gr_gameScriptExecutor, TXT("game script executor"));

// parameters
DEFINE_STATIC_NAME(inRoom);
DEFINE_STATIC_NAME(inRegion);

//

bool ManagerInfo::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("run")))
	{
		RunGameScript rgs;
		rgs.tagCondition.load_from_xml_attribute_or_child_node(node, TXT("tagCondition"));
		rgs.chance.load_from_xml(node, TXT("chance"));
		if (rgs.gameScript.load_from_xml(node, TXT("gameScript")))
		{
			_lc.load_group_into(rgs.gameScript);
			runGameScripts.push_back(rgs);
		}
		else
		{
			error_loading_xml(node, TXT("could not load run (game script) for ai manager"));
			result = false;
		}
	}

	for_every(node, _node->children_named(TXT("aiManager")))
	{
		AIMind aiMind;
		aiMind.tagCondition.load_from_xml_attribute_or_child_node(node, TXT("tagCondition"));
		aiMind.chance.load_from_xml(node, TXT("chance"));
		aiMind.parameters.load_from_xml_child_node(node, TXT("parameters"));
		aiMind.tagged.load_from_xml_attribute_or_child_node(node, TXT("tagged"));
		_lc.load_group_into(aiMind.parameters);
		if (aiMind.aiMind.load_from_xml(node, TXT("aiMind")))
		{
			_lc.load_group_into(aiMind.aiMind);
			aiMinds.push_back(aiMind);
		}
		else
		{
			error_loading_xml(node, TXT("could not load ai mind for ai manager"));
			result = false;
		}
	}

	for_every(node, _node->children_named(TXT("aiManagerData")))
	{
		Name type = node->get_name_attribute(TXT("type"));
		if (type.is_valid())
		{
			if (auto * md = ManagerDatas::create_data(type))
			{
				if (md->load_from_xml(node, _lc))
				{
					datas.push_back(RefCountObjectPtr<AI::ManagerData>(md));
				}
				else
				{
					error_loading_xml(node, TXT("ai manager data not loaded properly"));
					result = false;
				}
			}
			else
			{
				error_loading_xml(node, TXT("type not recognised"));
				result = false;
			}
		}
		else
		{
			error_loading_xml(node, TXT("no type provided"));
			result = false;
		}
	}

	return result;
}

bool ManagerInfo::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every_ref(data, datas)
	{
		result &= data->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

bool ManagerInfo::add_to(Framework::Room* _room, Framework::Region* _region, SimpleVariableStorage const * _additionalOptionalParameters) const
{
	scoped_call_stack_info(TXT("ManagerInfo::add_to"));
	scoped_call_stack_info_str_printf(TXT("room r%p"), _room);
	scoped_call_stack_info_str_printf(TXT("region g%p"), _region);

	bool result = true;

	an_assert(_region || _room);

	Random::Generator rg;
	if (_region)
	{
		rg = _region->get_individual_random_generator();
	}
	else if (_room)
	{
		rg = _room->get_individual_random_generator();
	}

	if (!_room)
	{
		_room = _region->get_any_room();
	}

	rg.advance_seed(32497, 9723);

	if (!runGameScripts.is_empty())
	{
		if (auto* rgsSceneryType = Framework::Library::get_current()->get_global_references().get<Framework::SceneryType>(NAME(gr_gameScriptExecutor), false))
		{
			for_every(rgs, runGameScripts)
			{
				if (!rgs->tagCondition.is_empty())
				{
					Tags againstTags;
					Framework::Region* checkRegion = _region;
					if (_room)
					{
						againstTags.set_tags_from(_room->get_tags());
						checkRegion = _room->get_in_region();
					}
					while (checkRegion)
					{
						againstTags.set_tags_from(checkRegion->get_tags());
						checkRegion = checkRegion->get_in_region();
					}
					if (!rgs->tagCondition.check(againstTags))
					{
						continue;
					}
				}

				SimpleVariableStorage parameters;
				if (_region)
				{
					_region->collect_variables(parameters);
				}
				else
				{
					_room->collect_variables(parameters);
				}
				if (_additionalOptionalParameters)
				{
					parameters.set_from(*_additionalOptionalParameters);
				}
				
				{
					float chance = rgs->chance.get(parameters, 1.0f, AllowToFail);
					if (!rg.get_chance(chance))
					{
						continue;
					}
				}

				Framework::LibraryName gsName;

				gsName = rgs->gameScript.get(parameters, Framework::LibraryName::invalid(), AllowToFail);

				auto* gsFound = gsName.is_valid() ? Library::get_current_as<Library>()->get_game_scripts().find(gsName) : nullptr;

				if (gsFound)
				{
					gsFound->load_on_demand_if_required();
					gsFound->load_elements_on_demand_if_required();

					rgsSceneryType->load_on_demand_if_required();

					Framework::Scenery* rgsScenery = nullptr;
					// subworld is same for both region and room
					Game::get()->perform_sync_world_job(TXT("spawn run game script executor"), [&rgsScenery, rgsSceneryType, _room, gsName]()
					{
						rgsScenery = new Framework::Scenery(rgsSceneryType, gsName.to_string());
						rgsScenery->init(_room->get_in_sub_world());
					});

					rgsScenery->access_variables().set_from(parameters);
					rgsScenery->access_individual_random_generator() = rg.spawn();

					rgsScenery->initialise_modules();

					if (auto* ai = fast_cast<ModuleAI>(rgsScenery->get_ai()))
					{
						gsFound->load_elements_on_demand_if_required();
						ai->start_game_script(gsFound);
					}
					else
					{
						error(TXT("game script executor scenery type \"%S\" does not have an ai module"), rgsSceneryType->get_name().to_string().to_char());
						result = false;
					}

					int rgsIndex = for_everys_index(rgs);
					// use room, we need to be placed in any room
					Game::get()->perform_sync_world_job(TXT("place game script executor"), [&rgsScenery, _room, rgsIndex]()
					{
						Vector3 const placementLoc = Vector3::zAxis * 100.0f;
						Vector3 const placementOffset = Vector3::xAxis;

#ifdef DEBUG_WORLD_CREATION_AND_DESTRUCTION
						output(TXT("add game script executor o%p to room r%p"), rgsScenery, _room);
#endif
						rgsScenery->get_presence()->place_in_room(_room, Transform(placementLoc + placementOffset * (float)(rgsIndex), Quat::identity)); // place it in separate location
					});

					if (_region)
					{
						rgsScenery->access_variables().access<SafePtr<Framework::Region>>(NAME(inRegion)) = _region;
					}
					else
					{
						rgsScenery->access_variables().access<SafePtr<Framework::Room>>(NAME(inRoom)) = _room;
					}
				}
				else if (gsName.is_valid())
				{
					error(TXT("couldn't find \"%S\" gameScript"), gsName.to_string().to_char());
				}
			}
		}
		else
		{
			error(TXT("could not find global referencer \"game script executor\" of scenery type"));
			result = false;
		}
	}

	if (!aiMinds.is_empty())
	{
		if (auto* aiManagerSceneryType = Framework::Library::get_current()->get_global_references().get<Framework::SceneryType>(NAME(gr_aiManager), false))
		{
			for_every(aiMind, aiMinds)
			{
				if (!aiMind->tagCondition.is_empty())
				{
					Tags againstTags;
					Framework::Region* checkRegion = _region;
					if (_room)
					{
						againstTags.set_tags_from(_room->get_tags());
						checkRegion = _room->get_in_region();
					}
					while (checkRegion)
					{
						againstTags.set_tags_from(checkRegion->get_tags());
						checkRegion = checkRegion->get_in_region();
					}
					if (! aiMind->tagCondition.check(againstTags))
					{
						continue;
					}
				}

				SimpleVariableStorage parameters;
				parameters.set_from(aiMind->parameters);
				if (_region)
				{
					_region->collect_variables(parameters);
				}
				else
				{
					_room->collect_variables(parameters);
				}
				if (_additionalOptionalParameters)
				{
					parameters.set_from(*_additionalOptionalParameters);
				}
				Framework::LibraryName aiMindName;
				
				aiMindName = aiMind->aiMind.get(parameters, Framework::LibraryName::invalid(), AllowToFail);
				
				{
					float chance = aiMind->chance.get(parameters, 1.0f, AllowToFail);
					if (!rg.get_chance(chance))
					{
						continue;
					}
				}

				auto * aiMindFound = aiMindName.is_valid() ? Framework::Library::get_current()->get_ai_minds().find(aiMindName) : nullptr;

				if (aiMindFound)
				{
					Framework::Scenery* aiManagerScenery = nullptr;
					// subworld is same for both region and room
					String location;
					if (auto* piow = PilgrimageInstanceOpenWorld::get())
					{
						Optional<VectorInt2> at;
						if (_region)
						{
							at = piow->find_cell_at(_region);
						}
						else if (_room)
						{
							at = piow->find_cell_at(_room);
						}
						if (!at.is_set())
						{
							if (piow->is_cell_context_set())
							{
								at = piow->get_cell_context();
							}
						}
						if (at.is_set())
						{
							location = String::printf(TXT("%ix%i"), at.get().x, at.get().y);
						}
					}

					aiManagerSceneryType->load_on_demand_if_required();
					Game::get()->perform_sync_world_job(TXT("spawn ai manager"), [&aiManagerScenery, aiManagerSceneryType, aiMindName, _room, &location]()
					{
						aiManagerScenery = new Framework::Scenery(aiManagerSceneryType, String::printf(TXT("[%S] \"%S\" via ai manager"), location.to_char(), aiMindName.to_string().to_char()));
						aiManagerScenery->init(_room->get_in_sub_world());
					});

					aiManagerScenery->access_tags().set_tags_from(aiMind->tagged);
					aiManagerScenery->access_variables().set_from(parameters);
					aiManagerScenery->access_individual_random_generator() = rg.spawn();

					aiManagerScenery->initialise_modules();

					if (auto* ai = aiManagerScenery->get_ai())
					{
						if (!ai->get_mind()->get_mind())
						{
							ai->get_mind()->use_mind(aiMindFound);
						}
						else
						{
							error(TXT("ai manager scenery type \"%S\" should not have mind set"), aiManagerSceneryType->get_name().to_string().to_char());
							result = false;
						}
					}
					else
					{
						error(TXT("ai manager scenery type \"%S\" does not have an ai module"), aiManagerSceneryType->get_name().to_string().to_char());
						result = false;
					}

					int aiMindIndex = for_everys_index(aiMind);
					// use room, we need to be placed in any room
					Game::get()->perform_sync_world_job(TXT("place ai manager"), [&aiManagerScenery, _room, aiMindIndex]()
					{
						Vector3 const placementLoc = Vector3::zAxis * 100.0f;
						Vector3 const placementOffset = Vector3::xAxis;

#ifdef DEBUG_WORLD_CREATION_AND_DESTRUCTION
						output(TXT("add ai manager o%p to room r%p"), aiManagerScenery, _room);
#endif
						aiManagerScenery->get_presence()->place_in_room(_room, Transform(placementLoc + placementOffset * (float)(aiMindIndex), Quat::identity)); // place it in separate location
					});

					if (_region)
					{
						aiManagerScenery->access_variables().access<SafePtr<Framework::Region>>(NAME(inRegion)) = _region;
					}
					else
					{
						aiManagerScenery->access_variables().access<SafePtr<Framework::Room>>(NAME(inRoom)) = _room;
					}
				}
			}
		}
		else
		{
			error(TXT("could not find global referencer \"ai manager\" of scenery type"));
			result = false;
		}
	}

	return result;
}

bool ManagerInfo::add_to_room(Framework::Room* _room, SimpleVariableStorage const * _additionalOptionalParameters) const
{
	return add_to(_room, nullptr, _additionalOptionalParameters);
}

bool ManagerInfo::add_to_region(Framework::Region* _region, SimpleVariableStorage const * _additionalOptionalParameters) const
{
	return add_to(nullptr, _region, _additionalOptionalParameters);
}

void ManagerInfo::log(LogInfoContext& _log) const
{
	for_every(mind, aiMinds)
	{
		if (mind->aiMind.is_value_set())
		{
			_log.log(TXT("ai mind: %S"), mind->aiMind.get_value().to_string().to_char());
		}
		else if (mind->aiMind.get_value_param().is_set())
		{
			_log.log(TXT("ai mind [param]: %S"), mind->aiMind.get_value_param().get().to_char());
		}
		else
		{
			_log.log(TXT("ai mind: ?"));
		}
		LOG_INDENT(_log);
		if (mind->chance.is_set())
		{
			if (mind->chance.is_value_set())
			{
				_log.log(TXT("chance: %.3f"), mind->chance.get_value());
			}
			else if (mind->chance.get_value_param().is_set())
			{
				_log.log(TXT("chance [param]: %S"), mind->chance.get_value_param().get().to_char());
			}
			else
			{
				_log.log(TXT("chance: ?"));
			}
		}
		if (! mind->tagCondition.is_empty())
		{
			_log.log(TXT("tag condition: %S"), mind->tagCondition.to_string().to_char());
		}
	}
	for_every_ref(data, datas)
	{
		data->log(_log);
	}
}
