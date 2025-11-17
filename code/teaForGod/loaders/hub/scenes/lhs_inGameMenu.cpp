#include "lhs_inGameMenu.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"

#include "..\scenes\lhs_beAtRightPlace.h"
#include "..\scenes\lhs_options.h"
#include "..\screens\lhc_debugNote.h"
#include "..\screens\lhc_drawingBoard.h"
#include "..\screens\lhc_handGestures.h"
#include "..\screens\lhc_question.h"
#include "..\widgets\lhw_button.h"
#include "..\widgets\lhw_image.h"
#include "..\widgets\lhw_text.h"

#include "..\..\..\developmentNotes.h"

#include "..\..\..\game\game.h"
#include "..\..\..\game\gameDirector.h"
#include "..\..\..\game\gameSettings.h"
#include "..\..\..\library\library.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\..\modules\gameplay\equipment\me_gun.h"

#include "..\..\..\..\framework\library\library.h"
#include "..\..\..\..\framework\object\actor.h"

#include "..\..\..\..\core\buildVer.h"
#include "..\..\..\..\core\system\faultHandler.h"
#include "..\..\..\..\core\system\recentCapture.h"
#include "..\..\..\..\core\system\video\renderTarget.h"

//

#ifdef BUILD_PREVIEW
#define WITH_DEBUG_CHEAT_MENU
#endif

#ifdef WITH_DEBUG_CHEAT_MENU
// debug stuff
#include "..\..\..\modules\custom\health\mc_health.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// localised strings
DEFINE_STATIC_NAME_STR(lsGoBackToGame, TXT("hub; common; back"));
DEFINE_STATIC_NAME_STR(lsOptions, TXT("hub; in game menu; options"));
DEFINE_STATIC_NAME_STR(lsEndGame, TXT("hub; in game menu; end game"));
DEFINE_STATIC_NAME_STR(lsEndGameQuestion, TXT("hub; in game menu; question; end game"));

DEFINE_STATIC_NAME_STR(lsShowHandGestures, TXT("hand gestures; show"));

// screens
DEFINE_STATIC_NAME(inGameMenu);
DEFINE_STATIC_NAME(inGameMenuDebug);
DEFINE_STATIC_NAME(inGameMenuNotes);
DEFINE_STATIC_NAME(inGameMenuDrawingBoard);

// ids (used by tutorials)
DEFINE_STATIC_NAME(analyser);
DEFINE_STATIC_NAME(energyTransfer);
DEFINE_STATIC_NAME(inventory);
DEFINE_STATIC_NAME(weaponPartTransfer);

// fade reasons
DEFINE_STATIC_NAME(backFromInGameMenu);

// global references
DEFINE_STATIC_NAME_STR(grWeaponItem, TXT("weapon item type to use with weapon parts"));

//

using namespace Loader;
using namespace HubScenes;

//

REGISTER_FOR_FAST_CAST(InGameMenu);

void InGameMenu::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	deactivateHub = false;
	get_hub()->show_start_movement_centre();
	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	get_hub()->deactivate_all_screens();

	if (auto* actor = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>()->access_player().get_actor())
	{
		pilgrim = actor->get_gameplay_as<TeaForGodEmperor::ModulePilgrim>();
	}

	show_in_game_menu();
}

void InGameMenu::on_update(float _deltaTime)
{
	base::on_update(_deltaTime);
}

void InGameMenu::show_in_game_menu()
{
	get_hub()->keep_only_screen(NAME(inGameMenu));

	// we have to make sure we have pixel data stored (unless we're not doing recent capture at all)
	::System::RecentCapture::force_next_store_pixel_data();

	bool withHandGestures = false;
	if (auto* vr = VR::IVR::get())
	{
		if (vr->is_using_hand_tracking())
		{
			withHandGestures = true;
		}
	}

	float menuWidth = 30.0f;
	float menuHeight = 10.0f;
	if (withHandGestures)
	{
		menuWidth += 10.0f;
	}

	Vector2 fs = HubScreen::s_fontSizeInPixels;
	float fsxy = max(fs.x, fs.y);

#ifdef WITH_DEBUG_CHEAT_MENU
	float maxWidth = menuWidth;
#endif

	{
		HubScreen* screen = get_hub()->get_screen(NAME(inGameMenu));
		if (! screen)
		{
			Vector2 size(menuWidth, menuHeight);
			Vector2 ppa(24.0f, 24.0f);
			Rotator3 atDir = get_hub()->get_current_forward();
			Rotator3 verticalOffset(0.0f, 0.0f, 0.0f);

			screen = new HubScreen(get_hub(), NAME(inGameMenu), size, atDir, get_hub()->get_radius() * 0.55f, true, verticalOffset, ppa);
			//screen->followYawDeadZoneByBorder = HubScreen::DEFAULT_YAW_DEAD_ZONE_BY_BORDER;

			screen->activate();
			get_hub()->add_screen(screen);

			Array<HubScreenButtonInfo> buttons;
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsGoBackToGame), [this]() { go_back_to_game(); }).store_widget_in(&exitWidget)
#ifndef WITH_DRAWING_BOARD
				.activate_on_trigger_hold()
#endif
			);
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsOptions), [this]() { open_options(); }));
			if (withHandGestures)
			{
				buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsShowHandGestures), [this]() { HubScreens::HandGestures::show(get_hub()); }));
			}
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsEndGame), [this]() { end_game(); }));

			screen->add_button_widgets_grid(buttons, VectorInt2(buttons.get_size(), 1),
											Range2(Range(screen->mainResolutionInPixels.x.min + fsxy, screen->mainResolutionInPixels.x.max - fsxy),
												   Range(screen->mainResolutionInPixels.y.min + fsxy, screen->mainResolutionInPixels.y.max - fsxy)),
											fs);
		}
	}

#ifdef WITH_DEBUG_CHEAT_MENU
	{
		HubScreen* screen = get_hub()->get_screen(NAME(inGameMenuDebug));
		if (! screen)
		{
			Vector2 size(50.0f, 50.0f);
			Vector2 ppa(24.0f, 24.0f);
			Rotator3 atDir = get_hub()->get_current_forward();
			Rotator3 verticalOffset(0.0f, 0.0f, 0.0f);

			maxWidth = max(size.x, maxWidth);

			Array<HubScreenButtonInfo> buttons;
			buttons.push_back(HubScreenButtonInfo(Name(String(TXT("log player's loc"))), String(TXT("log player's loc")), []()
			{
				lock_output();
				output(TXT("[DCM-LOG] PLAYER'S LOC"));
				if (auto* g = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
				{
					if (auto* pa = g->access_player().get_actor())
					{
						if (auto* p = pa->get_presence())
						{
							output(TXT("in room: \"%S\""), p->get_in_room() ? p->get_in_room()->get_name().to_char() : TXT("--"));
							output(TXT("tagged: \"%S\""), p->get_in_room() ? p->get_in_room()->get_tags().to_string().to_char() : TXT("--"));
							output(TXT("loc: %S"), p->get_placement().get_translation().to_string().to_char());
							output(TXT("rot: %S"), p->get_placement().get_orientation().to_rotator().to_string().to_char());
						}
					}
				}
				unlock_output();
			}));
			buttons.push_back(HubScreenButtonInfo(Name(String(TXT("log objects in room"))), String(TXT("log objects in room")), []()
			{
				lock_output();
				output(TXT("[DCM-LOG] OBJECTS IN ROOM"));
				if (auto* g = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
				{
					if (auto* pa = g->access_player().get_actor())
					{
						if (auto* p = pa->get_presence())
						{
							output(TXT("in room: \"%S\""), p->get_in_room() ? p->get_in_room()->get_name().to_char() : TXT("--"));
							if (p->get_in_room())
							{
								for_every_ptr(o, p->get_in_room()->get_objects())
								{
									output(TXT("  %S"), o->ai_get_name().to_char());
								}
							}
						}
					}
				}
				unlock_output();
			}));
			buttons.push_back(HubScreenButtonInfo(Name(String(TXT("full health+ammo"))), String(TXT("full health+ammo")), []()
			{
				output(TXT("[DCM-LOG] FULL HEALTH+AMMO"));
				if (auto* g = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
				{
					if (auto* pa = g->access_player().get_actor())
					{
						if (auto* h = pa->get_custom<TeaForGodEmperor::CustomModules::Health>())
						{
							TeaForGodEmperor::EnergyTransferRequest etr(TeaForGodEmperor::EnergyTransferRequest::Deposit);
							etr.energyRequested = h->get_max_total_health();
							h->handle_health_energy_transfer_request(etr);
						}
						if (auto* mp = pa->get_gameplay_as<TeaForGodEmperor::ModulePilgrim>())
						{
							if (auto* gd = TeaForGodEmperor::GameDirector::get())
							{
								if (! gd->are_ammo_storages_unavailable())
								{
									mp->set_hand_energy_storage(TeaForGodEmperor::GameSettings::actual_hand(Hand::Left), mp->get_hand_energy_max_storage(TeaForGodEmperor::GameSettings::actual_hand(Hand::Left)));
									mp->set_hand_energy_storage(TeaForGodEmperor::GameSettings::actual_hand(Hand::Right), mp->get_hand_energy_max_storage(TeaForGodEmperor::GameSettings::actual_hand(Hand::Right)));
								}
							}
						}
					}
				}
			}));
			buttons.push_back(HubScreenButtonInfo(Name(String(TXT("learn map"))), String(TXT("learn map")), []()
			{
				output(TXT("[DCM-LOG] LEARN MAP"));
				if (auto* g = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
				{
					if (auto* pa = g->access_player().get_actor())
					{
						if (auto* piow = TeaForGodEmperor::PilgrimageInstanceOpenWorld::get())
						{
							auto cellAt = piow->find_cell_at(pa);
							if (cellAt.is_set())
							{
								piow->mark_cell_known(cellAt.get(), 20);
								piow->mark_cell_known_exits(cellAt.get(), 20);
								piow->mark_cell_known_devices(cellAt.get(), 20);
								piow->mark_cell_known_special_devices(cellAt.get(), 20, 100);
								piow->mark_all_pilgrimage_device_directions_known();
							}
						}
					}
				}
			}));
			buttons.push_back(HubScreenButtonInfo(Name(String(TXT("random weapons in hands"))), String(TXT("random weapons in hands")), []()
				{
					output(TXT("[DCM-LOG] RANDOM WEAPONS IN HANDS"));
					if (auto* g = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
					{
						g->add_async_world_job_top_priority(TXT("random weapons in hands"),[g]()
						{
							if (auto* pa = g->access_player().get_actor())
							{
								if (auto* mp = pa->get_gameplay_as<TeaForGodEmperor::ModulePilgrim>())
								{
									if (auto* inRoom = pa->get_presence()->get_in_room())
									{
										Random::Generator rg;
										for_count(int, hIdx, Hand::MAX)
										{
											Hand::Type hand = (Hand::Type)hIdx;
											Framework::IModulesOwner* newe = nullptr;

											Framework::LibraryName itemName = Framework::Library::get_current()->get_global_references().get_name(NAME(grWeaponItem));
											if (auto* itemType = itemName.is_valid() ? Framework::Library::get_current()->get_item_types().find(itemName) : nullptr)
											{
												Framework::Object* spawnedObject = nullptr;
												String objectName = String::printf(TXT("random weapon %S"), rg.get_seed_string().to_char());

												itemType->load_on_demand_if_required();
												{
													Framework::ScopedAutoActivationGroup saag(fast_cast<Framework::WorldObject>(pa), inRoom, false); // will be activated with others
													Framework::Game::get()->perform_sync_world_job(TXT("spawn item"), [itemType, &objectName, &spawnedObject, inRoom]()
													{
														spawnedObject = itemType->create(objectName);
														spawnedObject->init(inRoom->get_in_sub_world());
													});
													spawnedObject->access_individual_random_generator() = rg.spawn();

													{
														bool moduleGunPresent = false;
														spawnedObject->initialise_modules([&moduleGunPresent](Framework::Module* _module)
														{
															if (auto* moduleGun = fast_cast<TeaForGodEmperor::ModuleEquipments::Gun>(_module))
															{
																moduleGunPresent = true;
																moduleGun->fill_with_random_parts();
															}
														});
														if (!moduleGunPresent)
														{
															warn(TXT("item type \"%S\" was expected to have module Gun"), itemName.to_string().to_char());
														}
													}
												}
												
												newe = spawnedObject;

												// just put them into hands
												mp->use(hand, newe);
											}
										}
									}
								}
							}
						});
					}
				}));
			buttons.push_back(HubScreenButtonInfo(Name(String(TXT("random exms"))), String(TXT("random exms")), []()
				{
					output(TXT("[DCM-LOG] RANDOM EXMS"));
					if (auto* g = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
					{
						g->add_async_world_job_top_priority(TXT("random exms"),[g]()
						{
							if (auto* pa = g->access_player().get_actor())
							{
								if (auto* mp = pa->get_gameplay_as<TeaForGodEmperor::ModulePilgrim>())
								{
									TeaForGodEmperor::PilgrimSetup setup(&TeaForGodEmperor::Persistence::access_current());
									setup.copy_from(mp->get_pending_pilgrim_setup());

									Random::Generator rg;

									Array<Name> permanentEXMs;
									Array<Name> activeEXMs;
									Array<Name> passiveEXMs;
									{
										Array<TeaForGodEmperor::EXMType*> allEXMs;
										allEXMs = TeaForGodEmperor::EXMType::get_all_exms(TagCondition());
										for_every_ptr(exm, allEXMs)
										{
											if (!exm->is_integral())
											{
												if (exm->is_permanent())
												{
													permanentEXMs.push_back(exm->get_id());
												}
												else
												{
													if (exm->is_active())
													{
														activeEXMs.push_back(exm->get_id());
													}
													else
													{
														passiveEXMs.push_back(exm->get_id());
													}
												}
											}
										}
									}

									int passiveSlotCount = TeaForGodEmperor::PilgrimBlackboard::get_passive_exm_slot_count(pa);

									auto& pi = mp->access_pilgrim_inventory();

									for_count(int, hIdx, Hand::MAX)
									{
										Hand::Type hand = (Hand::Type)hIdx;
										auto& handSetup = setup.access_hand(hand);
										handSetup.activeEXM.clear();
										handSetup.passiveEXMs.clear();
										if (rg.get_chance(0.9f) &&
											! activeEXMs.is_empty())
										{
											int idx = rg.get_int(activeEXMs.get_size());
											handSetup.activeEXM = TeaForGodEmperor::PilgrimHandEXMSetup(activeEXMs[idx]);
											activeEXMs.remove_fast_at(idx);
											if (handSetup.activeEXM.exm.is_valid() &&
												!pi.is_exm_available(handSetup.activeEXM.exm))
											{
												pi.make_exm_available(handSetup.activeEXM.exm);
											}
										}
										for_count(int, i, passiveSlotCount)
										{
											if (rg.get_chance(0.9f - (float)i * 0.3f) &&
												! passiveEXMs.is_empty())
											{
												int idx = rg.get_int(passiveEXMs.get_size());
												Name exm = passiveEXMs[idx];
												handSetup.passiveEXMs.push_back(TeaForGodEmperor::PilgrimHandEXMSetup(exm));
												passiveEXMs.remove_fast_at(idx);
												if (exm.is_valid() &&
													!pi.is_exm_available(exm))
												{
													pi.make_exm_available(exm);
												}
											}
										}
									}

									if (! permanentEXMs.is_empty())
									{
										int idx = Random::get_int(permanentEXMs.get_size());
										if (!pi.has_permanent_exm(permanentEXMs[idx]))
										{
											auto& exm = permanentEXMs[idx];
											g->perform_sync_world_job(TXT("add permanent exm"),
												[&pi, exm]()
												{
													pi.add_permanent_exm(exm);
												});
										}
									}

									mp->set_pending_pilgrim_setup(setup);
									g->perform_sync_world_job(TXT("create exms"),
										[mp]()
										{
											mp->sync_recreate_exms();
										});
								}
							}
						});
					}
				}));
			buttons.push_back(HubScreenButtonInfo(Name(String(TXT("send note/log"))), String(TXT("send note/log")), [this]()
			{
				lock_output();
				output(TXT("[DCM-LOG] FAULT HANDLER CUSTOM INFO"));
				output(TXT("%S"), ::System::FaultHandler::get_custom_info().to_char());
				output(TXT("[DCM-LOG] PLAYER'S LOC (auto)"));
				if (auto* g = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
				{
					if (auto* pa = g->access_player().get_actor())
					{
						if (auto* p = pa->get_presence())
						{
							output(TXT("in room: \"%S\""), p->get_in_room() ? p->get_in_room()->get_name().to_char() : TXT("--"));
							output(TXT("tagged: \"%S\""), p->get_in_room() ? p->get_in_room()->get_tags().to_string().to_char() : TXT("--"));
							output(TXT("loc: %S"), p->get_placement().get_translation().to_string().to_char());
							output(TXT("rot: %S"), p->get_placement().get_orientation().to_rotator().to_string().to_char());
						}
					}
				}
				unlock_output();
				output(TXT("[DCM-LOG] SEND NOTE/LOG"));
				HubScreens::DebugNote::show(get_hub(), String::empty(), String::empty());
			}));
			buttons.push_back(HubScreenButtonInfo(Name(String(TXT("wall/loading time issue"))), String(TXT("quick send info about~wall/loading time issue")), [this]()
			{
				lock_output();
				output(TXT("[DCM-LOG] FAULT HANDLER CUSTOM INFO"));
				output(TXT("%S"), ::System::FaultHandler::get_custom_info().to_char());
				output(TXT("[DCM-LOG] PLAYER'S LOC (auto)"));
				if (auto* g = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
				{
					if (auto* pa = g->access_player().get_actor())
					{
						if (auto* p = pa->get_presence())
						{
							output(TXT("in room: \"%S\""), p->get_in_room() ? p->get_in_room()->get_name().to_char() : TXT("--"));
							output(TXT("tagged: \"%S\""), p->get_in_room() ? p->get_in_room()->get_tags().to_string().to_char() : TXT("--"));
							output(TXT("loc: %S"), p->get_placement().get_translation().to_string().to_char());
							output(TXT("rot: %S"), p->get_placement().get_orientation().to_rotator().to_string().to_char());
						}
					}
				}
				unlock_output();
				output(TXT("[DCM-LOG] WALL/LOADING TIME ISSUE"));
				HubScreens::DebugNote::show(get_hub(), String::empty(), String(TXT("wall/loading time issue")), true);
			}));
			buttons.push_back(HubScreenButtonInfo(Name(String(TXT("CRASH THE GAME"))), String(TXT("CRASH THE GAME")), []()
				{
					output(TXT("[DCM-LOG] CRASHING THE GAME"));
					AN_FAULT;
					output(TXT("test crash - should not reach here"));
				}).activate_on_hold());

			int buttonX = 2;
			int buttonY = (buttons.get_size() + buttonX - 1) / buttonX;
			size.x = 25.0f * (float)buttonX;
			size.y = 5.0f * (float)(buttonY + 1);

			screen = new HubScreen(get_hub(), NAME(inGameMenuDebug), size, atDir, get_hub()->get_radius() * 0.55f, false, verticalOffset, ppa);
			//screen->followYawDeadZoneByBorder = HubScreen::DEFAULT_YAW_DEAD_ZONE_BY_BORDER;

			screen->activate();
			get_hub()->add_screen(screen);

			Vector2 spacing = Vector2::one * fsxy;
			Range2 at = screen->mainResolutionInPixels.expanded_by(-spacing);

			{
				Range2 eat = at;
				eat.y.min = eat.y.max - fs.y;
				auto* e = new HubWidgets::Text(Name(String(TXT("debug / cheat menu"))), eat, String(TXT("debug / cheat menu")));
				screen->add_widget(e);
				at.y.max = eat.y.min - spacing.y;
			}

			screen->add_button_widgets_grid(buttons, VectorInt2(buttonX, buttonY), at, spacing);

			screen->verticalOffset.pitch = 0.5f * (screen->size.y / ppa.y) + menuHeight + 10.0f;
			screen->be_vertical(true);
		}
	}
	String developmentNotes = development_notes();
	if (! developmentNotes.is_empty())
	{
		HubScreen* screen = get_hub()->get_screen(NAME(inGameMenuNotes));
		if (!screen)
		{
			Vector2 size(50.0f, 90.0f);
			Vector2 ppa(28.0f, 28.0f);
			Rotator3 atDir = get_hub()->get_current_forward();
			Rotator3 verticalOffset(0.0f, 0.0f, 0.0f);
			atDir.yaw += size.x * 0.5f + maxWidth * 0.5f + 2.5f;

			screen = new HubScreen(get_hub(), NAME(inGameMenuNotes), size, atDir, get_hub()->get_radius() * 0.55f, false, verticalOffset, ppa);

			screen->activate();
			get_hub()->add_screen(screen);

			Vector2 spacing = Vector2::one * fsxy;
			Range2 at = screen->mainResolutionInPixels.expanded_by(-spacing);

			{
				Range2 eat = at;
				auto* e = new HubWidgets::Text(Name::invalid(), eat, developmentNotes);
				e->alignPt.x = 0.0f;
				e->alignPt.y = 1.0f;
				screen->add_widget(e);
				e->at.y.min = e->at.y.max - e->calculate_vertical_size();
			}

			screen->compress_vertically();
		}
	}

#ifdef WITH_DRAWING_BOARD
	HubScreen* screen = get_hub()->get_screen(NAME(inGameMenuDrawingBoard));
	if (!screen)
	{
		Vector2 size = Vector2::one * 60.0f;
		if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
		{
			if (auto* db = game->get_drawing_board_rt())
			{
				Vector2 dbSize = db->get_size().to_vector2();
				size.x = size.y * dbSize.x / dbSize.y;
			}
		}
		Vector2 ppa(28.0f, 28.0f);
		Rotator3 atDir = get_hub()->get_current_forward();
		atDir.yaw -= size.x * 0.5f + maxWidth * 0.5f + 2.5f;

		screen = HubScreens::DrawingBoard::show(get_hub(), size, atDir, get_hub()->get_radius() * 0.55f);
	}
#endif
#endif

}

void InGameMenu::go_back_to_game()
{
	/*
	if (pilgrim && !pilgrim->is_up_to_date_with_pilgrim_setup())
	{
		// wait till done
		return;
	}
	*/
	auto* barp = new HubScenes::BeAtRightPlace();
	barp->allow_loading_tips(false);
	barp->with_go_back_button();
	if (barp->is_required(get_hub()))
	{
		get_hub()->set_scene(barp);
	}
	else
	{
		delete barp;
		get_hub()->show_start_movement_centre(false);
		get_hub()->set_beacon_active(false);
		deactivateHub = true;
	}
}

void InGameMenu::open_options()
{
	get_hub()->set_scene(new HubScenes::Options(HubScenes::Options::InGame));
}

void InGameMenu::end_game()
{
	HubScreens::Question::ask(get_hub(), NAME(lsEndGameQuestion),
		[this]()
		{
			get_hub()->show_start_movement_centre(false);
			get_hub()->set_beacon_active(false);
			get_hub()->deactivate_all_screens();
			if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
			{
				// will end mode
				game->request_post_game_summary(TeaForGodEmperor::PostGameSummary::Interrupted);
			}
			deactivateHub = true;
		},
		[this]()
		{
			show_in_game_menu();
		}
	);
}
