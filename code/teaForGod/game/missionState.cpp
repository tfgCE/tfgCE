#include "missionState.h"

#include "playerSetup.h"

#include "..\game\game.h"
#include "..\library\library.h"
#include "..\modules\custom\mc_pickup.h"
#include "..\modules\gameplay\equipment\me_gun.h"
#include "..\modules\gameplay\modulePilgrim.h"

#include "..\..\core\random\randomUtils.h"

#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\framework\object\actor.h"
#include "..\..\framework\object\object.h"
#include "..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

MissionState* MissionState::s_current = nullptr;

void MissionState::find_current()
{
	s_current = nullptr;
	if (auto* ps = PlayerSetup::get_current_if_exists())
	{
		if (auto* gs = ps->get_active_game_slot())
		{
			s_current = gs->missionState.get();
		}
	}
}

void MissionState::prepare_for(PlayerGameSlot* _gs)
{
	isReadyToUse = false;

	missionsDefinition.clear();
	missionDefinition.clear();
	missionsDefinitionSetId = Name::invalid();
	difficultyModifier.clear();
	
	_gs->prepare_missions_definition();

	missionsDefinition = _gs->get_missions_definition();

	if (auto* msd = missionsDefinition.get())
	{
		if (!msd->get_sets().is_empty())
		{
			todo_note(TXT("choose missions set? not that when we generate the mission, we already are IN the mission, the first one, so we have to limit ourselves to the current pilgrimage"));
			missionsDefinitionSet = msd->get_sets().get_first();
			if (auto* msds = missionsDefinitionSet.get())
			{
				missionsDefinitionSetId = msds->id;
			}
		}

		if (! msd->get_loot_progress_milestones().is_empty())
		{
			auto& p = _gs->persistence;
			Concurrency::ScopedSpinLock lock(p.access_lock(), true);
			p.access_progress().set_loot_progress_milestones(msd->get_loot_progress_milestones());
			GameDefinition::advance_loot_progress_level(p.access_progress(), 0);
		}
	}

	// commented out as we are choosing now our missions
	/*
	if (auto* msds = missionsDefinitionSet.get())
	{
		if (!msds->missions.is_empty())
		{
			missionDefinition = msds->missions.get_first();
		}
	}
	*/

	build_pilgrimages_list(); // will set isReadyToUse
}

void MissionState::set_mission(MissionDefinition* _md, MissionStateObjectives const* _objectives, MissionDifficultyModifier const * _difficultyModifier)
{
	an_assert(!missionDefinition.is_set());
	missionDefinition = _md;
	{
		if (_objectives)
		{
			objectives.copy_from(*_objectives);
		}
		else
		{
			objectives.reset();
		}
	}
	difficultyModifier = _difficultyModifier;

	build_pilgrimages_list(); // will set isReadyToUse

	prepare_for_gameplay();
}

void MissionState::prepare_for_gameplay()
{
	objectives.prepare_for_gameplay();
}

void MissionState::build_pilgrimages_list()
{
	isReadyToUse = false;

	// build pilgrimages list
	{
		pilgrimages.clear();

		if (auto* msds = missionsDefinitionSet.get())
		{
			for_every(p, msds->startingPilgrimages)
			{
				pilgrimages.push_back(*p);
			}
		}

		if (auto* md = missionDefinition.get())
		{
			for_every(p, md->get_pilgrimages())
			{
				pilgrimages.push_back(*p);
			}
		}
	}

	isReadyToUse = missionDefinition.get() != nullptr;
}

void MissionState::ready_for(PlayerGameSlot* _gs)
{
	isReadyToUse = false;

	if (missionsDefinition.is_name_valid())
	{
		if (!missionsDefinition.get())
		{
			missionsDefinition.find_may_fail(Library::get_current());
		}
	}
	if (!missionsDefinition.get())
	{
		// get from slot if none, maybe something got broken
		missionsDefinition = _gs->get_missions_definition();
	}
	an_assert(missionsDefinition.get());

	missionsDefinitionSet.clear();

	if (auto* msd = missionsDefinition.get())
	{
		for_every_ref(set, msd->get_sets())
		{
			if (missionsDefinitionSetId == set->id)
			{
				missionsDefinitionSet = set;
			}
		}
	}

	if (missionDefinition.is_name_valid())
	{
		if (!missionDefinition.get())
		{
			missionDefinition.find(Library::get_current());
		}
	}

	if (difficultyModifier.is_name_valid())
	{
		if (!difficultyModifier.get())
		{
			difficultyModifier.find(Library::get_current());
		}
	}

	build_pilgrimages_list(); // will set isReadyToUse
}

MissionState::MissionState()
{
}

MissionState::~MissionState()
{
	if (s_current == this)
	{
		s_current = nullptr;
	}
}

void MissionState::mark_started()
{
	Concurrency::ScopedSpinLock lock(resultsLock);
	started = true;
}

void MissionState::use_intel_on_start()
{
	Concurrency::ScopedSpinLock lock(resultsLock);
	if (auto* md = missionDefinition.get())
	{
		apply_intel_info(md->get_mission_intel().get_info_on_start());
		Persistence::access_current().add_mission_intel(md->get_mission_intel().get_on_start(), md);
	}
}

void MissionState::store_starting_state()
{
	auto& gs = GameStats::get();
	Concurrency::SpinLock gsLock(gs.access_lock());
	startingState.experience = gs.experience;
	startingState.meritPoints = gs.meritPoints;
}

void MissionState::mark_came_back()
{
	Concurrency::ScopedSpinLock lock(resultsLock);
	cameBack = true;
}

void MissionState::mark_success()
{
	Concurrency::ScopedSpinLock lock(resultsLock);
	success = true;
	failed = false;
}

void MissionState::mark_failed()
{
	Concurrency::ScopedSpinLock lock(resultsLock);
	failed = true;
	success = false;
}

void MissionState::mark_died()
{
	Concurrency::ScopedSpinLock lock(resultsLock);
	died = true;
}

bool MissionState::give_rewards(Energy xp, int mp, int ip, bool _applyGameSettingsModifiers)
{
	bool given = false;

	{
		if (!xp.is_zero())
		{
			if (_applyGameSettingsModifiers)
			{
				xp = xp.adjusted_plus_one(GameSettings::get().experienceModifier);
			}
			PlayerSetup::access_current().stats__experience(xp);
			GameStats::get().add_experience_from_mission(xp);
			Persistence::access_current().provide_experience(xp);

			given |= !xp.is_zero();
		}
	}
	{
		if (mp != 0)
		{
			PlayerSetup::access_current().stats__merit_points(mp);
			GameStats::get().add_merit_points(mp);
			Persistence::access_current().provide_merit_points(mp);

			given |= true;
		}
	}
	{
		if (ip != 0)
		{
			add_intel_from_objectives(ip);
			
			given |= true;
		}
	}

	return given;
}

void MissionState::visited_cell()
{
	{
		Concurrency::ScopedSpinLock lock(resultsLock);
		++visitedCells;
	}
	if (auto* md = missionDefinition.get())
	{
		give_rewards(md->get_mission_objectives().get_experience_for_visited_cell(), md->get_mission_objectives().get_merit_points_for_visited_cell(), md->get_mission_objectives().get_intel_for_visited_cell());
	}
}

void MissionState::visited_interface_box()
{
	{
		Concurrency::ScopedSpinLock lock(resultsLock);
		++visitedInterfaceBoxes;
	}
	bool rewardGiven = false;
	if (auto* md = missionDefinition.get())
	{
		rewardGiven = give_rewards(md->get_mission_objectives().get_experience_for_visited_interface_box(), md->get_mission_objectives().get_merit_points_for_visited_interface_box(), md->get_mission_objectives().get_intel_for_visited_interface_box());
	}
	if (rewardGiven)
	{
		todo_multiplayer_issue(TXT("we just get player here"));
		if (auto* g = Game::get_as<Game>())
		{
			if (auto* pa = g->access_player().get_actor())
			{
				if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
				{
					mp->access_overlay_info().speak(PilgrimOverlayInfoSpeakLine::InterfaceBoxFound);
				}
			}
		}
	}
}

bool MissionState::is_item_required(Framework::IModulesOwner* _imo) const
{
	return objectives.is_item_required(_imo);
}

void MissionState::process_brought_item(Framework::IModulesOwner* _imo)
{
	if (!is_item_required(_imo))
	{
		return;
	}

	{
		Concurrency::ScopedSpinLock lock(resultsLock);
		++broughtItems;
	}
	if (auto* md = missionDefinition.get())
	{
		give_rewards(md->get_mission_objectives().get_experience_for_brought_item(), md->get_mission_objectives().get_merit_points_for_brought_item(), md->get_mission_objectives().get_intel_for_brought_item());
	}
}

void MissionState::hacked_box()
{
	{
		Concurrency::ScopedSpinLock lock(resultsLock);
		++hackedBoxes;
	}
	if (auto* md = missionDefinition.get())
	{
		give_rewards(md->get_mission_objectives().get_experience_for_hacked_box(), md->get_mission_objectives().get_merit_points_for_hacked_box(), md->get_mission_objectives().get_intel_for_hacked_box());
	}
}

void MissionState::stopped_infestation()
{
	{
		Concurrency::ScopedSpinLock lock(resultsLock);
		++stoppedInfestations;
	}
	if (auto* md = missionDefinition.get())
	{
		give_rewards(md->get_mission_objectives().get_experience_for_stopped_infestation(), md->get_mission_objectives().get_merit_points_for_stopped_infestation(), md->get_mission_objectives().get_intel_for_stopped_infestation());
	}
	{
		todo_multiplayer_issue(TXT("we just get player here"));
		if (auto* g = Game::get_as<Game>())
		{
			if (auto* pa = g->access_player().get_actor())
			{
				if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
				{
					mp->access_overlay_info().speak(PilgrimOverlayInfoSpeakLine::InfestationEnded);
				}
			}
		}
	}
}

void MissionState::refilled()
{
	{
		Concurrency::ScopedSpinLock lock(resultsLock);
		++refills;
	}
	if (auto* md = missionDefinition.get())
	{
		give_rewards(md->get_mission_objectives().get_experience_for_refill(), md->get_mission_objectives().get_merit_points_for_refill(), md->get_mission_objectives().get_intel_for_refill());
	}
	{
		todo_multiplayer_issue(TXT("we just get player here"));
		if (auto* g = Game::get_as<Game>())
		{
			if (auto* pa = g->access_player().get_actor())
			{
				if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
				{
					mp->access_overlay_info().speak(PilgrimOverlayInfoSpeakLine::SystemStabilised);
				}
			}
		}
	}
}

bool MissionState::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	if (missionsDefinition.is_name_valid())
	{
		_node->set_attribute(TXT("missionsDefinition"), missionsDefinition.get_name().to_string());
	}
	_node->set_attribute(TXT("missionsDefinitionSetId"), missionsDefinitionSetId.to_string());
	if (missionDefinition.is_name_valid())
	{
		_node->set_attribute(TXT("missionDefinition"), missionDefinition.get_name().to_string());
	}
	if (difficultyModifier.is_name_valid())
	{
		_node->set_attribute(TXT("difficultyModifier"), difficultyModifier.get_name().to_string());
	}

	if (auto* n = _node->add_node(TXT("startingState")))
	{
		if (startingState.experience.is_set())
		{
			n->set_attribute(TXT("experience"), startingState.experience.get().as_string_auto_decimals());
		}
		if (startingState.meritPoints.is_set())
		{
			n->set_int_attribute(TXT("meritPoints"), startingState.meritPoints.get());
		}
	}

	_node->set_bool_attribute(TXT("started"), started);
	_node->set_bool_attribute(TXT("cameBack"), cameBack);
	_node->set_bool_attribute(TXT("success"), success);
	_node->set_bool_attribute(TXT("failed"), failed);
	_node->set_bool_attribute(TXT("died"), died);
	_node->set_int_attribute(TXT("visitedCells"), visitedCells);
	_node->set_int_attribute(TXT("visitedInterfaceBoxes"), visitedInterfaceBoxes);
	_node->set_int_attribute(TXT("broughtItems"), broughtItems);
	_node->set_int_attribute(TXT("hackedBoxes"), hackedBoxes);
	_node->set_int_attribute(TXT("stoppedInfestations"), stoppedInfestations);
	_node->set_int_attribute(TXT("refills"), refills);
	
	_node->set_int_attribute(TXT("startingMaxMissionTier"), startingMaxMissionTier);
	_node->set_int_attribute(TXT("intelFromObjectives"), intelFromObjectives);

	result &= objectives.save_to_xml(_node);

	return result;
}

bool MissionState::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	isReadyToUse = false;

	missionsDefinition.load_from_xml(_node, TXT("missionsDefinition")); // full name expected!
	missionsDefinitionSetId = _node->get_name_attribute(TXT("missionsDefinitionSetId"), missionsDefinitionSetId);
	missionDefinition.load_from_xml(_node, TXT("missionDefinition")); // full name expected!
	difficultyModifier.load_from_xml(_node, TXT("difficultyModifier")); // full name expected!

	startingState = StartingState();
	for_every(n, _node->children_named(TXT("startingState")))
	{
		if (auto* attr = n->get_attribute(TXT("experience")))
		{
			startingState.experience = Energy::parse(attr->get_as_string());
		}
		if (auto* attr = n->get_attribute(TXT("meritPoints")))
		{
			startingState.meritPoints.load_from_xml(attr);
		}
	}

	XML_LOAD_BOOL_ATTR_CHILD_PRESENCE(_node, started);
	XML_LOAD_BOOL_ATTR_CHILD_PRESENCE(_node, cameBack);
	XML_LOAD_BOOL_ATTR_CHILD_PRESENCE(_node, success);
	XML_LOAD_BOOL_ATTR_CHILD_PRESENCE(_node, failed);
	XML_LOAD_BOOL_ATTR_CHILD_PRESENCE(_node, died);
	XML_LOAD_INT_ATTR_CHILD(_node, visitedCells);
	XML_LOAD_INT_ATTR_CHILD(_node, visitedInterfaceBoxes);
	XML_LOAD_INT_ATTR_CHILD(_node, broughtItems);
	XML_LOAD_INT_ATTR_CHILD(_node, hackedBoxes);
	XML_LOAD_INT_ATTR_CHILD(_node, stoppedInfestations);
	XML_LOAD_INT_ATTR_CHILD(_node, refills);
	
	XML_LOAD_INT_ATTR_CHILD(_node, startingMaxMissionTier);
	XML_LOAD_INT_ATTR_CHILD(_node, intelFromObjectives);

	result &= objectives.load_from_xml(_node);

	return result;
}

void MissionState::end(EndParams const& _params)
{
	process_end(_params);

	// clear mission state and save
	Game::get()->add_async_world_job_top_priority(TXT("mission end"),
		[]()
		{
			async_on_end();
		});
}

void MissionState::async_abort(bool _save)
{
	if (auto* ms = get_current())
	{
		ms->process_end(EndParams());
	}

	async_on_end(_save);
}

void MissionState::async_on_end(bool _save)
{
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		if (auto* ps = TeaForGodEmperor::PlayerSetup::access_current_if_exists())
		{
			if (auto* gs = ps->access_active_game_slot())
			{
				// behave like we just started
				gs->clear_game_states();
				gs->clear_mission_state();
			}
		}
		if (_save)
		{
			game->save_config_file();
			game->save_player_profile(true); // save active game slot, to store mission state and game states
		}
	}
}

void MissionState::process_end(EndParams const& _params)
{
	RefCountObjectPtr<MissionResult> lastMissionResult;
	if (auto* gs = PlayerSetup::access_current().access_active_game_slot())
	{
		lastMissionResult = gs->create_new_last_mission_result();
	}

	Array<Framework::IModulesOwner*> items;
	if (!_params.processItemsInRooms.is_empty())
	{
		for_every_ptr(room, _params.processItemsInRooms)
		{
			for_every_ptr(obj, room->get_objects())
			{
				if (obj->get_custom<CustomModules::Pickup>())
				{
					items.push_back_unique(obj);
				}
			}
		}
	}

	// no else, we use push_back_unique for doubled items
	if (cameBack)
	{
		todo_multiplayer_issue(TXT("we just get player here"));
		if (auto* g = Game::get_as<Game>())
		{
			if (auto* pa = g->access_player().get_actor())
			{
				if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
				{
					ArrayStatic<Framework::IModulesOwner*, 16> gear; SET_EXTRA_DEBUG_INFO(gear, TXT("ModulePilgrim::add_gear_to_persistence.gear"));
					for_count(int, hIdx, Hand::MAX)
					{
						Hand::Type hand = (Hand::Type)hIdx;
						if (auto* imo = mp->get_main_equipment(hand))
						{
							gear.push_back_unique(imo);
						}
						if (auto* imo = mp->get_hand_equipment(hand))
						{
							gear.push_back_unique(imo);
						}
					}

					mp->get_in_pockets(gear, NP, true);

					for_every_ptr(item, gear)
					{
						items.push_back_unique(item);
					}
				}
			}
		}
	}

	if (cameBack)
	{
		// process brought items now, if we have to drop them, we will do that basing on last mission result
		for_every_ptr(item, items)
		{
			process_brought_item(item);
		}
	}

	if (cameBack)
	{
		// move weapons to armoury
		auto& p = Persistence::access_current();
		Concurrency::ScopedSpinLock lock(p.access_lock());

		struct WeaponHeldByPlayer
		{
			Persistence::WeaponOnPilgrimInfo onPilgrimInfo;
			ModuleEquipments::Gun* weapon;
		};

		// we prioritise weapons held by player, so if there's not enough space, we should keep them first
		Array<WeaponHeldByPlayer> weaponsHeldByPlayer;
		Array<ModuleEquipments::Gun*> otherWeapons;

		Framework::IModulesOwner* playerActor = nullptr;
		{
			todo_multiplayer_issue(TXT("we just get player here"));
			if (auto* g = Game::get_as<Game>())
			{
				if (auto* pa = g->access_player().get_actor())
				{
					playerActor = pa;
				}
			}
		}

		for (int i = 0; i < items.get_size(); ++i)
		{
			auto* item = items[i];
			if (auto* gun = item->get_gameplay_as< ModuleEquipments::Gun>())
			{
				{
					Persistence::WeaponOnPilgrimInfo onPilgrimInfo;

					if (playerActor)
					{
						onPilgrimInfo.fill(playerActor, item);
					}

					if (onPilgrimInfo.onPilgrim)
					{
						weaponsHeldByPlayer.push_back();
						weaponsHeldByPlayer.get_last().weapon = gun;
						weaponsHeldByPlayer.get_last().onPilgrimInfo = onPilgrimInfo;
					}
					else
					{
						otherWeapons.push_back(gun);
					}
				}
				items.remove_at(i);
				--i;
			}
		}

		for_every(w, weaponsHeldByPlayer)
		{
			p.add_weapon_to_armoury(w->weapon->get_weapon_setup(), false, true, w->weapon->get_owner(), w->onPilgrimInfo);
		}

		for_every_ptr(w, otherWeapons)
		{
			p.add_weapon_to_armoury(w->get_weapon_setup(), false, true, w->get_owner());
		}
	}

	// advance seed now as we will want to have new missions available
	{
		int tier = 0;
		if (auto* md = missionDefinition.get())
		{
			if (auto* mst = md->get_mission_selection_tier())
			{
				tier = mst->get_tier();
			}
		}
		// advance this tier and all lower
		for (int t = 0; t <= tier; ++t)
		{
			Persistence::access_current().advance_mission_seed(t);
		}
	}

	// fill last mission result
	if (lastMissionResult.get())
	{
		auto* md = missionDefinition.get();
		lastMissionResult->set_mission(missionDefinition.get(), difficultyModifier.get());
		lastMissionResult->objectives = objectives;
		lastMissionResult->started = started;
		lastMissionResult->cameBack = cameBack;
		lastMissionResult->success = success;
		lastMissionResult->failed = failed;
		lastMissionResult->died = died;
		lastMissionResult->visitedCells = visitedCells;
		lastMissionResult->visitedInterfaceBoxes = visitedInterfaceBoxes;
		lastMissionResult->broughtItems = broughtItems;
		lastMissionResult->hackedBoxes = hackedBoxes;
		lastMissionResult->stoppedInfestations = stoppedInfestations;
		lastMissionResult->refills = refills;
		lastMissionResult->intelFromObjectives = intelFromObjectives;
		if (!lastMissionResult->success &&
			!lastMissionResult->failed)
		{
			warn(TXT("couldn't tell if success or not, check if there are clear objectives"));
			bool anyObjectiveDone = false;
			bool anyObjectiveFailed = false;
			if (!lastMissionResult->objectives.bringItemsTypes.is_empty())
			{
				bool ok = lastMissionResult->broughtItems != 0;
				anyObjectiveDone |= ok;
				anyObjectiveFailed |= !ok;
			}
			if (md && md->get_mission_objectives().get_required_hacked_box_count() > 0)
			{
				bool ok = lastMissionResult->hackedBoxes >= md->get_mission_objectives().get_required_hacked_box_count();
				anyObjectiveDone |= ok;
				anyObjectiveFailed |= !ok;
			}
			if (md && md->get_mission_objectives().get_required_stopped_infestations_count() > 0)
			{
				bool ok = lastMissionResult->stoppedInfestations >= md->get_mission_objectives().get_required_stopped_infestations_count();
				anyObjectiveDone |= ok;
				anyObjectiveFailed |= !ok;
			}
			if (md && md->get_mission_objectives().get_required_refills_count() > 0)
			{
				bool ok = lastMissionResult->refills >= md->get_mission_objectives().get_required_refills_count();
				anyObjectiveDone |= ok;
				anyObjectiveFailed |= !ok;
			}

			if (anyObjectiveDone)
			{
				lastMissionResult->success = true;
			}
			else if (anyObjectiveFailed)
			{
				lastMissionResult->failed = true;
			}
		}
		if (!lastMissionResult->success &&
			!lastMissionResult->failed)
		{
			warn(TXT("couldn't tell if success or not, check rewards"));
			bool anyObjectiveDone = false;
			bool anyObjectiveFailed = false;
			if (md && md->get_mission_objectives().get_merit_points_for_visited_cell() > 0)
			{
				bool ok = lastMissionResult->visitedCells != 0;
				ok &= (lastMissionResult->cameBack && !lastMissionResult->died); // but only if we came back
				anyObjectiveDone |= ok;
				anyObjectiveFailed |= !ok;
			}
			if (md && md->get_mission_objectives().get_merit_points_for_visited_interface_box() > 0)
			{
				bool ok = lastMissionResult->visitedInterfaceBoxes != 0;
				ok &= (lastMissionResult->cameBack && !lastMissionResult->died); // but only if we came back
				anyObjectiveDone |= ok;
				anyObjectiveFailed |= !ok;
			}
			if (md && md->get_mission_objectives().get_merit_points_for_brought_item() > 0)
			{
				bool ok = lastMissionResult->broughtItems != 0;
				anyObjectiveDone |= ok;
				anyObjectiveFailed |= !ok;
			}
			if (md && md->get_mission_objectives().get_merit_points_for_hacked_box() > 0)
			{
				bool ok = lastMissionResult->hackedBoxes != 0;
				anyObjectiveDone |= ok;
				anyObjectiveFailed |= !ok;
			}
			if (md && md->get_mission_objectives().get_merit_points_for_stopped_infestation() > 0)
			{
				bool ok = lastMissionResult->stoppedInfestations != 0;
				anyObjectiveDone |= ok;
				anyObjectiveFailed |= !ok;
			}
			if (md && md->get_mission_objectives().get_merit_points_for_refill() > 0)
			{
				bool ok = lastMissionResult->refills != 0;
				anyObjectiveDone |= ok;
				anyObjectiveFailed |= !ok;
			}
			if (anyObjectiveDone)
			{
				lastMissionResult->success = true;
			}
			else if (anyObjectiveFailed)
			{
				lastMissionResult->failed = true;
			}
		}
		/*
		if (!lastMissionResult->success &&
			!lastMissionResult->failed)
		{
			warn(TXT("still couldn't tell if success or not, consider it success only if came back alive"));
			if (lastMissionResult->cameBack && !lastMissionResult->died)
			{
				lastMissionResult->success = true;
			}
			else
			{
				lastMissionResult->failed = true;
			}
		}
		*/
		if (md)
		{
			if (md->get_no_success())
			{
				lastMissionResult->success = false;
			}
			if (md->get_no_failure())
			{
				lastMissionResult->failed = false;
			}
		}

		// aftermath
		if (md)
		{
			MissionDefinitionIntel::ProvideInfo const* nextTierInfo = nullptr;

			// check what would happen if we got new tier
			{
				auto& mi = md->get_mission_intel();
				nextTierInfo = get_intel_info(mi.get_info_on_next_tier());
				if (nextTierInfo)
				{
					output(TXT("[mission] has possible next tier info"));
				}
			}

			// aftermath - xp+mp, intel, general progress
			{
				// success xp + mp
				{
					if (lastMissionResult->success)
					{
						give_rewards(md->get_mission_objectives().get_experience_for_success(), md->get_mission_objectives().get_merit_points_for_success(), 0);
					}
				}

				// intel
				{
					auto& mi = md->get_mission_intel();
					auto& p = Persistence::access_current();
					if (lastMissionResult->success)
					{
						p.add_mission_intel(mi.get_on_success(), md);
						apply_intel_info(mi.get_info_on_success());
					}
					if (lastMissionResult->failed)
					{
						p.add_mission_intel(mi.get_on_failure(), md);
						apply_intel_info(mi.get_info_on_failure());
					}
					if (lastMissionResult->cameBack)
					{
						p.add_mission_intel(mi.get_on_came_back(), md);
					}
					else
					{
						p.add_mission_intel(mi.get_on_did_not_come_back(), md);
					}
				}

				// mission general progress
				{
					if (auto* mgp = md->get_mission_general_progress())
					{
						auto& p = Persistence::access_current();
						p.add_mission_general_progress_info(mgp->get_playing_gives());
						if (lastMissionResult->success)
						{
							p.add_mission_general_progress_info(mgp->get_success_gives());
						}
						if (lastMissionResult->failed)
						{
							p.add_mission_general_progress_info(mgp->get_failure_gives());
						}
					}
				}
			}

			// check if we unlock next tier with our new tier info
			{
				output(TXT("[mission] check next tier"));
				Array<AvailableMission> availableMissions;
				// use new tier info as we want to check what would happen if we had new tier available (some may require values we set here)
				AvailableMission::get_available_missions(availableMissions, nextTierInfo);
				int maxTier = startingMaxMissionTier;
				for_every(am, availableMissions)
				{
					maxTier = max(maxTier, am->tier);
				}
				lastMissionResult->nextTierMadeAvailable = maxTier > startingMaxMissionTier;

				output(TXT("[mission] starting max tier: %i, next max tier: %i"), startingMaxMissionTier, maxTier);

				// check if we raised tier from this one, if so, use next tier info
				{
					bool useNextTierInfo = false;

					if (lastMissionResult->nextTierMadeAvailable)
					{
						if (auto* mst = md->get_mission_selection_tier())
						{
							output(TXT("[mission] starting mission tier %i"), mst->get_tier());
							if (mst->get_tier() == startingMaxMissionTier)
							{
								useNextTierInfo = true;
							}
						}
					}

					if (!useNextTierInfo)
					{
						// we won't use it for any reason
						nextTierInfo = nullptr;
					}
				}
			}

			// aftermath - intel info, next tier
			{
				if (nextTierInfo)
				{
					output(TXT("[mission] apply next tier info"));
					apply_intel_info(nextTierInfo);
				}
			}
		}

		// apply bonus from mdm
		if (auto* mdm = get_difficulty_modifier())
		{
			MissionDifficultyModifier::BonusCoef bonus;

			bonus += mdm->get_on_end_bonus();
			if (lastMissionResult->success)
			{
				bonus += mdm->get_on_success_bonus();
			}
			if (lastMissionResult->cameBack)
			{
				bonus += mdm->get_on_come_back_bonus();
			}

			if (bonus.experienceCoef != 0.0f ||
				bonus.meritPointsCoef != 0.0f)
			{
				Energy bonusXP = Energy::zero();
				int bonusMP = 0;
				{
					auto& gs = GameStats::get();
					Concurrency::SpinLock lock(gs.access_lock());
					if (bonus.experienceCoef != 0.0f && startingState.experience.is_set())
					{
						bonusXP = gs.experience - startingState.experience.get();
						bonusXP = bonusXP.mul(bonus.experienceCoef);
					}
					if (bonus.meritPointsCoef != 0.0f && startingState.meritPoints.is_set())
					{
						bonusMP = gs.meritPoints - startingState.meritPoints.get();
						bonusMP = TypeConversions::Normal::f_i_closest((float)(bonusMP) * bonus.experienceCoef);
					}
				}
				// do not apply game settings modifiers as we use difference as a base and the game settings modifiers were already used there
				give_rewards(bonusXP, bonusMP, 0, false);
			}

			lastMissionResult->appliedBonus = bonus;
		}

		// at the end as we want to get all values
		{
			auto& gs = GameStats::get();
			Concurrency::SpinLock lock(gs.access_lock());
			lastMissionResult->stats = gs;
			gs.reset();
		}
	}
}

bool MissionState::does_require_items_to_bring() const
{
	return !objectives.bringItemsTypes.is_empty();
}

void MissionState::add_intel_from_objectives(int _intel)
{
	Concurrency::ScopedSpinLock lock(resultsLock);

	int newIntel = intelFromObjectives + _intel;

	if (auto* md = missionDefinition.get())
	{
		auto const& limit = md->get_mission_intel().get_limit_from_objectives();
		if (limit.is_set())
		{
			newIntel = min(limit.get(), newIntel);
		}
	}

	int intelChange = newIntel - intelFromObjectives;

	Persistence::access_current().add_mission_intel(intelChange, missionDefinition.get());

	intelFromObjectives = newIntel;
}

int MissionState::get_held_items_to_bring(Framework::IModulesOwner* imo) const
{
	int itemCount = 0;

	if (auto* mp = imo->get_gameplay_as<ModulePilgrim>())
	{
		ARRAY_STACK(Framework::IModulesOwner*, items, 32);
		mp->get_all_items(items);
		for_every_ptr(i, items)
		{
			if (is_item_required(i))
			{
				++itemCount;
			}
		}
	}

	return itemCount;
}

void MissionState::apply_intel_info(Array<MissionDefinitionIntel::ProvideInfo> const& _info)
{
	apply_intel_info(get_intel_info(_info));
}

void MissionState::apply_intel_info(MissionDefinitionIntel::ProvideInfo const* _info)
{
	if (_info)
	{
		auto& p = Persistence::access_current();

		p.remove_mission_intel_info(_info->removeInfo);
		p.set_mission_intel_info(_info->setInfo);
		p.add_mission_intel_info(_info->addInfo);
	}
}

MissionDefinitionIntel::ProvideInfo const * MissionState::get_intel_info(Array<MissionDefinitionIntel::ProvideInfo> const& _info)
{
	if (_info.is_empty())
	{
		return nullptr;
	}

	Array<MissionDefinitionIntel::ProvideInfo const*> info;
	{
		auto& p = Persistence::access_current();

		Concurrency::ScopedSpinLock lock(p.access_lock());
		for_every(i, _info)
		{
			if (i->ifInfo.check(p.access_mission_intel_info()))
			{
				info.push_back(i);
			}
		}
	}

	if (!info.is_empty())
	{
		int idx = RandomUtils::ChooseFromContainer<Array<MissionDefinitionIntel::ProvideInfo const*>, MissionDefinitionIntel::ProvideInfo const*> ::choose(Random::next_int(), info,
			[](MissionDefinitionIntel::ProvideInfo const* info) { return info->probCoef; });
		return info[idx];
	}
	else
	{
		return nullptr;
	}
}