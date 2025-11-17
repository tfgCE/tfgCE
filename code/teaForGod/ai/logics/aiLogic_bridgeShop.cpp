#include "aiLogic_bridgeShop.h"

#include "..\..\game\game.h"
#include "..\..\library\library.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_hitIndicator.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\overlayInfo\elements\oie_customDraw.h"
#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"
#include "..\..\utils\reward.h"
#include "..\..\utils\variablesStringFormatterParamsProvider.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\world\door.h"
#include "..\..\..\framework\world\world.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define DESC_WIDTH 4.0f

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// variables
DEFINE_STATIC_NAME(shopId);
DEFINE_STATIC_NAME(shopChangeInteractiveDeviceId);
DEFINE_STATIC_NAME(shopChooseInteractiveDeviceId);

// door tags
DEFINE_STATIC_NAME_STR(dt_exitToCapsule, TXT("exitToCapsule"));

// pilgrim overlay info
DEFINE_STATIC_NAME_STR(poi_bridgeShop, TXT("bridgeShop"));
DEFINE_STATIC_NAME_STR(poi_bridgeShop_dial, TXT("bridgeShop_dial"));
DEFINE_STATIC_NAME_STR(poi_bridgeShop_select, TXT("bridgeShop_select"));

// text colours
DEFINE_STATIC_NAME(title);
DEFINE_STATIC_NAME(status);

// emissives
DEFINE_STATIC_NAME(busy);

// sounds
DEFINE_STATIC_NAME(unlockedUnlockable);

// localised strings
DEFINE_STATIC_NAME_STR(lsChangeDialInfo, TXT("bridge shop; change dial info"));
DEFINE_STATIC_NAME_STR(lsChooseSwitchInfo, TXT("bridge shop; choose switch info"));

DEFINE_STATIC_NAME_STR(lsUnlock, TXT("hub; unlocks; unlock"));
DEFINE_STATIC_NAME_STR(lsUnlockBuy, TXT("hub; unlocks; unlock; buy"));
	DEFINE_STATIC_NAME(cost);
DEFINE_STATIC_NAME_STR(lsCurrentXP, TXT("hub; unlocks; current xp"));
	DEFINE_STATIC_NAME(xpToSpend);
DEFINE_STATIC_NAME_STR(lsUnlockedCount, TXT("hub; unlocks; unlocked count"));
DEFINE_STATIC_NAME_STR(lsUnlockedCountBuy, TXT("hub; unlocks; unlocked count; buy"));
	DEFINE_STATIC_NAME(unlocked);
	DEFINE_STATIC_NAME(limit);

DEFINE_STATIC_NAME_STR(lsCharsExperience, TXT("chars; experience"));
DEFINE_STATIC_NAME_STR(lsCharsMeritPoints, TXT("chars; merit points"));

// global references
DEFINE_STATIC_NAME_STR(grOverlayInfoPermanentEXM, TXT("pi.ov.in; permanent exm line model"));
DEFINE_STATIC_NAME_STR(grOverlayInfoPermanentEXMp1, TXT("pi.ov.in; permanent exm +1 line model"));
DEFINE_STATIC_NAME_STR(grOverlayInfoPassiveEXM, TXT("pi.ov.in; passive exm line model"));
DEFINE_STATIC_NAME_STR(grOverlayInfoActiveEXM, TXT("pi.ov.in; active exm line model"));
DEFINE_STATIC_NAME_STR(grOverlayInfoIntegralEXM, TXT("pi.ov.in; integral exm line model"));

// ai messages
DEFINE_STATIC_NAME_STR(aim_mgp_unlocked, TXT("mission general progress; unlocked"));

//

namespace DebriefingScreenIdx
{
	enum Type
	{
		MissionDesc,
		Stats,
		NUM
	};
};

//

static Transform overlay_info_int_dev_offset()
{
	Transform intDevOffset = Transform(Vector3::zAxis * 0.08f + Vector3::yAxis * 0.10f, Rotator3(-60.0f, 0.0f, 0.0f).to_quat());
	return intDevOffset;
}

static float overlay_info_int_dev_size()
{
	float size = 0.025f;
	return size;
}

static float overlay_info_text_size()
{
	float size = 0.1f;
	return size;
}

static float overlay_info_draw_size()
{
	float size = 0.5f;
	return size;
}

//

REGISTER_FOR_FAST_CAST(BridgeShop);

BridgeShop::BridgeShop(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	bridgeShopData = fast_cast<BridgeShopData>(_logicData);
	executionData = new ExecutionData();

	hitIndicatorColour.parse_from_string(String(TXT("hi_activated_upgrade_canister")));
}

BridgeShop::~BridgeShop()
{
	hide_any_overlay_info();
	if (auto* imo = get_imo())
	{
	}
}

void BridgeShop::play_sound(Name const& _sound)
{
	if (auto* imo = get_imo())
	{
		if (auto* s = imo->get_sound())
		{
			s->play_sound(_sound);
		}
	}
}

void BridgeShop::stop_sound(Name const& _sound)
{
	if (auto* imo = get_imo())
	{
		if (auto* s = imo->get_sound())
		{
			s->stop_sound(_sound);
		}
	}
}

void BridgeShop::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	// wait till anything available
	if (!executionData->changeDialHandler.is_valid() ||
		!executionData->chooseSwitchHandler.is_valid() ||
		!executionData->initialSetupDone)
	{
		return;
	}

	// advance controls
	{
		executionData->changeDialHandler.advance(_deltaTime);
		executionData->chooseSwitchHandler.advance(_deltaTime);

		if (!executionData->unlockables.is_empty())
		{
			{
				int idx = mod(executionData->changeDialHandler.get_dial_at(), executionData->unlockables.get_size());
				auto& newSelectedUnlockable = executionData->unlockables[idx];
				if (executionData->selectedUnlockable != newSelectedUnlockable)
				{
					executionData->selectedUnlockable = newSelectedUnlockable;
					executionData->selectedUnlockableRenderableData.mark_new_data_required();
					hide_any_overlay_info(); // will reshow everything
				}
				if (executionData->chooseSwitchHandler.get_switch_at() < 0.1f)
				{
					executionData->clearToUnlock = true;
				}
				if (executionData->chooseSwitchHandler.get_switch_at() >= 0.99f && executionData->clearToUnlock)
				{
					// unlock if possible
					bool unlocked = false;

					unlocked = unlock_selected();

					if (unlocked)
					{
						executionData->clearToUnlock = false;
						if (auto* p = executionData->pilgrim.get())
						{
							if (auto* ha = p->get_custom<CustomModules::HitIndicator>())
							{
								Framework::RelativeToPresencePlacement r2pp;
								r2pp.be_temporary_snapshot();
								auto* heldSwitch = executionData->chooseSwitchHandler.get_switches()[0].get();
								if (r2pp.find_path(p, heldSwitch))
								{
									Vector3 buttonHereWS = r2pp.location_from_target_to_owner(heldSwitch->get_presence()->get_centre_of_presence_WS());
									float delay = 0.0f;
									int hitIndicatorCount = 3;
									for_count(int, i, hitIndicatorCount)
									{
										ha->indicate_location(0.0f, buttonHereWS, CustomModules::HitIndicator::IndicateParams()
											.with_colour_override(hitIndicatorColour).with_delay(delay).try_being_relative_to(heldSwitch).not_damage());
										delay += 0.02f;
									}
								}
							}
							if (auto* s = p->get_sound())
							{
								s->play_sound(NAME(unlockedUnlockable));
							}
						}

						update_unlockables();
					}
				}
			}
		}
	}

	// show if in the same room
	if (auto* imo = get_mind()->get_owner_as_modules_owner()) 
	{
		bool showSomething = false;
		if (auto* r = imo->get_presence()->get_in_room())
		{
			if (auto* p = executionData->pilgrim.get())
			{
				if (p->get_presence()->get_in_room() == r)
				{
					showSomething = true;
				}
			}
		}
		if (showSomething)
		{
			if (!executionData->showingShop || executionData->updateShopRequired)
			{
				executionData->updateShopRequired = false;

				// we must have entered now or requested update of the whole shop, updated unlockables
				update_unlockables();

				hide_any_overlay_info();

				show_overlay_info();
			}
		}
		if (!showSomething)
		{
			executionData->updateShopRequired = false;
			if (executionData->showingShop)
			{
				hide_any_overlay_info();
			}
		}
	}
}

PilgrimOverlayInfo* BridgeShop::access_overlay_info()
{
	if (auto* imo = executionData->pilgrim.get())
	{
		if (auto* mp = imo->get_gameplay_as<ModulePilgrim>())
		{
			return &mp->access_overlay_info();
		}
	}
	return nullptr;
}

void BridgeShop::hide_any_overlay_info()
{
	if (auto* poi = access_overlay_info())
	{
		poi->hide_show_info(NAME(poi_bridgeShop));
		poi->hide_show_info(NAME(poi_bridgeShop_dial));
		poi->hide_show_info(NAME(poi_bridgeShop_select));
	}
	executionData->showingShop = false;
}

void BridgeShop::show_overlay_info()
{
	auto* poi = access_overlay_info();

	if (!poi)
	{
		return;
	}

	//Transform intDevOffset = overlay_info_int_dev_offset();
	float textSize = overlay_info_text_size();
	float drawSize = overlay_info_draw_size();
	float spacingSize = drawSize * 0.1f;
	float iconSizeRel = 1.0f;

	Transform placement = Transform::identity;
	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		if (auto* p = imo->get_presence())
		{
			if (auto* r = p->get_in_room())
			{
				Framework::PointOfInterestInstance* foundPOI = nullptr;
				if (r->find_any_point_of_interest(bridgeShopData->overlayPOI, foundPOI))
				{
					placement = foundPOI->calculate_placement();
				}
			}
		}
	}
	
	Transform descAt = placement;

	if (executionData->selectedUnlockable.exm ||
		executionData->selectedUnlockable.customUpgradeType)
	{
		String text;
		int unlockedCount = 0;
		int unlockLimit = 0; // non 0 only if important

		{
			if (auto* exm = executionData->selectedUnlockable.exm)
			{
				text = exm->get_ui_desc();

				if (exm->is_permanent())
				{
					unlockedCount = UnlockableEXMs::count_unlocked(exm->get_id());
					unlockLimit = exm->get_permanent_limit();
				}
			}
			if (auto* cu = executionData->selectedUnlockable.customUpgradeType)
			{
				text = cu->get_ui_desc();
				if (cu->get_unlock_limit() > 1)
				{
					unlockedCount = cu->get_unlocked_count();
					unlockLimit = cu->get_unlock_limit();
				}
			}
		}

		if (executionData->selectedUnlockableRenderableData.should_do_new_data_to_render())
		{
			float rdSize = iconSizeRel;

			auto& rd = executionData->selectedUnlockableRenderableData;
			rd.clear();

			{
				Matrix44 offset = matrix_from_up_forward(Vector3::zero, -Vector3::yAxis, Vector3::zAxis);

				if (auto* exm = executionData->selectedUnlockable.exm)
				{
					LineModel* lm = nullptr;
					if (exm->is_permanent())
					{
						lm = executionData->oiPermanentEXM.get();
					}
					else if (exm->is_integral())
					{
						lm = executionData->oiIntegralEXM.get();
					}
					else if (exm->is_active())
					{
						lm = executionData->oiActiveEXM.get();
					}
					else
					{
						lm = executionData->oiPassiveEXM.get();
					}
					if (lm)
					{
						for_every(l, lm->get_lines())
						{
							Colour c = l->colour.get(Colour::white);
							Vector3 a = offset.location_to_world(l->a * rdSize);
							Vector3 b = offset.location_to_world(l->b * rdSize);

							rd.add_line(a, b, c);
						}

						rdSize *= 0.8f;
					}

					{
						if (auto* lm = exm->get_line_model())
						{
							for_every(l, lm->get_lines())
							{
								Colour c = l->colour.get(Colour::white);
								Vector3 a = offset.location_to_world(l->a * rdSize);
								Vector3 b = offset.location_to_world(l->b * rdSize);

								rd.add_line(a, b, c);
							}
						}
					}
				}
				if (auto* cu = executionData->selectedUnlockable.customUpgradeType)
				{
					for_count(int, i, 2)
					{
						// first outer as we will reduce size
						LineModel* lm = i == 0 ? cu->get_outer_line_model() : cu->get_line_model();
						if (lm)
						{
							for_every(l, lm->get_lines())
							{
								Colour c = l->colour.get(Colour::white);
								Vector3 a = offset.location_to_world(l->a * rdSize);
								Vector3 b = offset.location_to_world(l->b * rdSize);

								rd.add_line(a, b, c);
							}

							rdSize *= 0.8f;
						}
					}
				}
			}

			rd.mark_new_data_done();
			rd.mark_data_available_to_render();
		}

		if (!executionData->selectedUnlockableRenderableData.is_empty())
		{
			float useSize = drawSize;
			Transform iconAt = descAt;
			iconAt.set_translation(iconAt.get_translation() - Vector3::zAxis * drawSize * 0.5f);
			descAt.set_translation(descAt.get_translation() - Vector3::zAxis * (drawSize + spacingSize));

			RefCountObjectPtr<ExecutionData> keepExecutionData = executionData;
			poi->show_info(NAME(poi_bridgeShop), iconAt, [keepExecutionData, useSize]()
				{
					auto* e = new OverlayInfo::Elements::CustomDraw();

					e->with_draw([keepExecutionData](float _active, float _pulse, Colour const& _colour)
						{
							if (!keepExecutionData->selectedUnlockableRenderableData.is_empty())
							{
								if (auto* imo = keepExecutionData->pilgrim.get())
								{
									if (auto* mp = imo->get_gameplay_as<ModulePilgrim>())
									{
										auto& poi = mp->access_overlay_info();
										keepExecutionData->selectedUnlockableRenderableData.render(_active, _pulse, _colour, NP, poi.get_overlay_shader_instance());
									}
								}
							}
						});
					e->with_size(useSize, false);

					return e;
				}, false);
		}

		if (!text.is_empty())
		{
			poi->show_info(NAME(poi_bridgeShop), descAt, text, NP, textSize, DESC_WIDTH, false, Vector2(0.5f, 1.0f));
		}

		{
			Transform unlockAt = placement;
			unlockAt.set_translation(unlockAt.get_translation() - Vector3::zAxis * (textSize * 16.0f));
			Transform unlockLimitAt = unlockAt;
			unlockLimitAt.set_translation(unlockLimitAt.get_translation() + Vector3::zAxis * (textSize * 1.2f));
			Transform xpToSpendAt = unlockAt;
			xpToSpendAt.set_translation(xpToSpendAt.get_translation() + Vector3::zAxis * (textSize * -2.2f));

			bool costsXP = false;
			bool costsMP = false;
			{
				Reward cost(calculate_unlock_xp_cost(), calculate_unlock_merit_points_cost());
				costsXP = cost.has_xp();
				costsMP = cost.has_merit_points();
				if (!cost.is_empty())
				{
					Name unlockLS = NAME(lsUnlock);
					if (auto* cu = executionData->selectedUnlockable.customUpgradeType)
					{
						if (cu->is_for_buying())
						{
							unlockLS = NAME(lsUnlockBuy);
						}
					}

					String text = String::space() + Framework::StringFormatter::format_loc_str(unlockLS, Framework::StringFormatterParams()
						.add(NAME(cost), cost.as_string()));

					poi->show_info(NAME(poi_bridgeShop), unlockAt, text, NP, textSize, NP, false, Vector2(0.5f, 1.0f));
				}
			}

			if (unlockLimit > 0)
			{
				Name unlockLS = NAME(lsUnlockedCount);
				if (auto* cu = executionData->selectedUnlockable.customUpgradeType)
				{
					if (cu->is_for_buying())
					{
						unlockLS = NAME(lsUnlockedCountBuy);
					}
				}
				String text = Framework::StringFormatter::format_loc_str(unlockLS, Framework::StringFormatterParams()
					.add(NAME(unlocked), unlockedCount)
					.add(NAME(limit), unlockLimit));
				poi->show_info(NAME(poi_bridgeShop), unlockLimitAt, text, NP, textSize, NP, false, Vector2(0.5f, 1.0f));
			}

			{
				Reward toSpend(executionData->xpToSpend, executionData->meritPointsToSpend);

				String text = Framework::StringFormatter::format_loc_str(NAME(lsCurrentXP), Framework::StringFormatterParams()
					.add(NAME(xpToSpend), toSpend.as_string(costsXP, costsMP)));
				poi->show_info(NAME(poi_bridgeShop), xpToSpendAt, text, NP, textSize, NP, false, Vector2(0.5f, 1.0f));
			}

		}

	}
	/*
	{
		{
			String desc;
			desc += TXT("#title#");
			desc += executionData->showingMissionInfo->get_mission_name();
			desc += TXT("~~");
			desc += executionData->showingMissionInfo->get_mission_description();

			{
				VariablesStringFormatterParamsProvider vsfpp(executionData->showingMissionInfo->get_custom_parameters());
				desc = Framework::StringFormatter::format(desc, Framework::StringFormatterParams().add_params_provider(&vsfpp));
			}

			{
				String objDesc = executionData->showingMissionInfo->get_mission_objectives().get_description();
				if (!objDesc.is_empty())
				{
					desc += TXT("~~");
					desc += objDesc;
				}
			}

			float size = 0.1f;
			OverlayInfo::TextColours textColours;
			textColours.add(NAME(title), executionData->chosenMission? missionSelectedColour : missionTitleColour);
			poi->show_info(NAME(poi_bridgeShop), descAt, desc, textColours, size, 5.0f, false, Vector2(0.0f, 1.0f));
		}

		if (! executionData->chosenMission)
		{
			update_dial_info();

			{
				for_every_ref(imo, executionData->chooseSwitchHandler.get_switches())
				{
					auto* useImo = imo;
					if (auto* atImo = useImo->get_presence()->get_attached_to())
					{
						useImo = atImo;
					}
					Transform at = useImo->get_presence()->get_placement();
					at = at.to_world(intDevOffset);
					poi->show_info(NAME(poi_bridgeShop), at, LOC_STR(NAME(lsChooseSwitchInfo)), NP, size, NP, false);
				}
			}
		}
	}
	*/

	update_dial_info();
	update_select_info();

	executionData->showingShop = true;
}

Energy BridgeShop::calculate_unlock_xp_cost()
{
	if (auto* exm = executionData->selectedUnlockable.exm)
	{
		if (auto* persistence = TeaForGodEmperor::Persistence::access_current_if_exists())
		{
			Concurrency::ScopedSpinLock lock(persistence->access_lock(), true);
			if (exm->is_permanent())
			{
				return TeaForGodEmperor::GameplayBalance::persistent_permanent_exm_cost(persistence->get_permanent_exms().get_size());
			}
			else
			{
				return TeaForGodEmperor::GameplayBalance::persistent_normal_exm_cost();
			}
		}
	}
	if (auto* cu = executionData->selectedUnlockable.customUpgradeType)
	{
		return cu->calculate_unlock_xp_cost();
	}
	return Energy::zero();
}

int BridgeShop::calculate_unlock_merit_points_cost()
{
	if (auto* cu = executionData->selectedUnlockable.customUpgradeType)
	{
		return cu->calculate_unlock_merit_points_cost();
	}
	return 0;
}

bool BridgeShop::unlock_selected()
{
	if (auto* persistence = TeaForGodEmperor::Persistence::access_current_if_exists())
	{
		Concurrency::ScopedSpinLock lock(persistence->access_lock());
		TeaForGodEmperor::Energy costXP = calculate_unlock_xp_cost();
		int costMP = calculate_unlock_merit_points_cost();
		bool unlocked = false;
		if (persistence->get_experience_to_spend() >= costXP &&
			persistence->get_merit_points_to_spend() >= costMP)
		{
			if (auto* exmType = executionData->selectedUnlockable.exm)
			{
				Name exm = exmType->get_id();
				if (exmType->is_permanent())
				{
					persistence->access_all_exms().push_back(exm);
					TeaForGodEmperor::PlayerSetup::access_current().stats__unlock_all_exms();
					persistence->cache_exms();
					unlocked = true;
				}
				else
				{
					if (!persistence->get_unlocked_exms().does_contain(exm))
					{
						persistence->access_all_exms().push_back(exm);
						TeaForGodEmperor::PlayerSetup::access_current().stats__unlock_all_exms();
						persistence->cache_exms();
						unlocked = true;
					}
				}
			}
			if (auto* cu = executionData->selectedUnlockable.customUpgradeType)
			{
				unlocked = cu->process_unlock();
			}
			if (unlocked)
			{
				persistence->spend_experience(costXP); // we should still have it, we locked persistence
				persistence->spend_merit_points(costMP); // we should still have it, we locked persistence
				if (auto* g = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
				{
					{
						// do not do it via store game state as that should be only called from within pilgrimage
#ifdef AN_ALLOW_EXTENSIVE_LOGS
						output(TXT("store player profile - bridge shop unlock"));
#endif
						g->add_async_save_player_profile();
					}
					{
						RefCountObjectPtr<ExecutionData> keepExecutionData = executionData;
						g->add_immediate_sync_world_job(TXT("update exms"), [keepExecutionData]()
							{
								if (auto* pa = keepExecutionData->pilgrim.get())
								{
									if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
									{
										mp->update_exms_in_inventory();
										mp->sync_recreate_exms();
										mp->update_blackboard();
									}
								}
							});
					}
				}
				hide_any_overlay_info(); // will update and show
				return true;
			}
		}
		return false;
	}

	return false;
}

void BridgeShop::update_dial_info()
{
	Transform intDevOffset = overlay_info_int_dev_offset();
	float size = overlay_info_int_dev_size();

	if (auto* poi = access_overlay_info())
	{
		poi->hide_show_info(NAME(poi_bridgeShop_dial));

		if (executionData->unlockables.get_size() > 1)
		{
			String text = LOC_STR(NAME(lsChangeDialInfo));
			text += TXT("~");
			{
				int count = executionData->unlockables.get_size();
				int idx = mod(executionData->changeDialHandler.get_dial_at(), count);
				text += String::printf(TXT("%i/%i"), idx + 1, count);
			}

			for_every_ref(imo, executionData->changeDialHandler.get_dials())
			{
				auto* useImo = imo;
				if (auto* atImo = useImo->get_presence()->get_attached_to())
				{
					useImo = atImo;
				}
				Transform at = useImo->get_presence()->get_placement();
				at = at.to_world(intDevOffset);

				poi->show_info(NAME(poi_bridgeShop_dial), at, text, NP, size, NP, false);
			}
		}
	}
}

void BridgeShop::update_select_info()
{
	Transform intDevOffset = overlay_info_int_dev_offset();
	float size = overlay_info_int_dev_size();

	if (auto* poi = access_overlay_info())
	{
		poi->hide_show_info(NAME(poi_bridgeShop_select));

		// if can unlock
		if (executionData->unlockables.get_size() > 0 &&
			executionData->xpToSpend >= calculate_unlock_xp_cost() &&
			executionData->meritPointsToSpend >= calculate_unlock_merit_points_cost())
		{
			String text = LOC_STR(NAME(lsChooseSwitchInfo));
			text += TXT("~");

			for_every_ref(imo, executionData->chooseSwitchHandler.get_switches())
			{
				auto* useImo = imo;
				if (auto* atImo = useImo->get_presence()->get_attached_to())
				{
					useImo = atImo;
				}
				Transform at = useImo->get_presence()->get_placement();
				at = at.to_world(intDevOffset);

				poi->show_info(NAME(poi_bridgeShop_select), at, text, NP, size, NP, false);
			}
		}
	}
}

void BridgeShop::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);

	if (auto* lib = Framework::Library::get_current())
	{
		executionData->oiPermanentEXM = lib->get_global_references().get<LineModel>(NAME(grOverlayInfoPermanentEXM));
		executionData->oiPermanentEXMp1 = lib->get_global_references().get<LineModel>(NAME(grOverlayInfoPermanentEXMp1));
		executionData->oiPassiveEXM = lib->get_global_references().get<LineModel>(NAME(grOverlayInfoPassiveEXM));
		executionData->oiActiveEXM = lib->get_global_references().get<LineModel>(NAME(grOverlayInfoActiveEXM));
		executionData->oiIntegralEXM = lib->get_global_references().get<LineModel>(NAME(grOverlayInfoIntegralEXM));
	}
}

void BridgeShop::initial_setup()
{
	executionData->simpleRules = true;
	if (auto* gd = GameDefinition::get_chosen())
	{
		if (gd->get_type() == GameDefinition::Type::ComplexRules)
		{
			executionData->simpleRules = false;
		}
	}

	executionData->initialSetupDone = true;
}

void BridgeShop::update_unlockables()
{
	executionData->unlockableEXMs.update(executionData->simpleRules, true); // always get non permanent from the last setup
	executionData->unlockableCustomUpgrades.update();

	if (auto* persistence = Persistence::access_current_if_exists())
	{
		Concurrency::ScopedSpinLock lock(persistence->access_lock(), true);

		executionData->xpToSpend = persistence->get_experience_to_spend();
		executionData->meritPointsToSpend = persistence->get_merit_points_to_spend();
	}

	executionData->unlockables.clear();
	for_every(cu, executionData->unlockableCustomUpgrades.availableCustomUpgrades)
	{
		executionData->unlockables.push_back();
		executionData->unlockables.get_last().customUpgradeType = cu->upgrade;
	}
	for_every(exm, executionData->unlockableEXMs.availableEXMs)
	{
		executionData->unlockables.push_back();
		executionData->unlockables.get_last().exm = EXMType::find(*exm);
	}
}

LATENT_FUNCTION(BridgeShop::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai bridge shop] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<BridgeShop>(logic);

	LATENT_BEGIN_CODE();

	ai_log(self, TXT("bridge shop, hello!"));

	messageHandler.use_with(mind);
	{
		RefCountObjectPtr<ExecutionData> keepExecutionData;
		keepExecutionData = self->executionData;
		messageHandler.set(NAME(aim_mgp_unlocked), [keepExecutionData](Framework::AI::Message const& _message)
			{
				keepExecutionData->updateShopRequired = true;
			}
		);
	}

	// wait a bit for interactive controls to appear
	LATENT_WAIT(0.1f);

	// wait for mission state to become active (should be created by mission briefer)
	while (!MissionState::get_current())
	{
		LATENT_YIELD();
	}

	self->initial_setup();

	if (imo)
	{
		{
			if (auto* r = imo->get_presence()->get_in_room())
			{
				if (auto* id = imo->get_variables().get_existing<int>(NAME(shopChangeInteractiveDeviceId)))
				{
					int shopChangeInteractiveDeviceId = *id;

					auto* world = imo->get_in_world();
					an_assert(world);

					world->for_every_object_with_id(NAME(shopChangeInteractiveDeviceId), shopChangeInteractiveDeviceId,
						[self, imo](Framework::Object* _object)
						{
							if (_object != imo)
							{
								self->executionData->changeDialHandler.initialise(_object, NAME(shopChangeInteractiveDeviceId));
							}
							return false; // keep going on - we want to get all
						});
				}
				if (auto* id = imo->get_variables().get_existing<int>(NAME(shopChooseInteractiveDeviceId)))
				{
					int shopChooseInteractiveDeviceId = *id;

					auto* world = imo->get_in_world();
					an_assert(world);

					world->for_every_object_with_id(NAME(shopChooseInteractiveDeviceId), shopChooseInteractiveDeviceId,
						[self, imo](Framework::Object* _object)
						{
							if (_object != imo)
							{
								self->executionData->chooseSwitchHandler.initialise(_object, NAME(shopChooseInteractiveDeviceId));
							}
							return false; // keep going on - we want to get all
						});
				}
			}
		}
	}

	{
		self->executionData->pilgrim.clear();
		todo_multiplayer_issue(TXT("we just get player here"));
		if (auto* g = Game::get_as<Game>())
		{
			if (auto* pa = g->access_player().get_actor())
			{
				if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
				{
					self->executionData->pilgrim = pa;
				}
			}
		}
	}


	// just wait endlessly
	while (true)
	{
		LATENT_WAIT(0.1f);
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(BridgeShopData);

bool BridgeShopData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	overlayPOI = _node->get_name_attribute(TXT("overlayPOI"));
	overlayScale = _node->get_float_attribute(TXT("overlayScale"), 1.0f);

	return result;
}

bool BridgeShopData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
