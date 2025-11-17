#include "aiLogic_weaponMixer.h"

#include "utils\aiLogicUtil_deviceDisplayStart.h"

#include "..\..\teaForGodTest.h"
#include "..\..\game\game.h"
#include "..\..\library\library.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_itemHolder.h"
#include "..\..\modules\custom\mc_weaponLocker.h"
#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\core\random\randomUtils.h"
#include "..\..\..\core\system\video\video3dPrimitives.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\appearance\controllers\ac_particlesUtils.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\module\custom\mc_display.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"

#ifdef AN_DEVELOPMENT_OR_PROFILER
#include "..\..\..\core\system\input.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define ALLOW_SELECTING_CHASSIS
#ifdef AN_ALLOW_EXTENSIVE_LOGS
//#define DEBUG_SELECTED_WEAPON_PART_IDX
#endif

//#define MEASURE_SHOW_INFO_SIZES 20
//#define TEST_SHOW_INFO_SIZES

#ifndef MEASURE_SHOW_INFO_SIZES
	#define SCALE_SHOW_INFO
#else
	#ifdef TEST_SHOW_INFO_SIZES
		#define SCALE_SHOW_INFO
	#endif
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// variables
DEFINE_STATIC_NAME(weaponContainerId);
DEFINE_STATIC_NAME(weaponContainerIdLeft);
DEFINE_STATIC_NAME(weaponContainerIdRight);
DEFINE_STATIC_NAME(choosePartId);
DEFINE_STATIC_NAME(choosePartIdLeft);
DEFINE_STATIC_NAME(choosePartIdRight);
DEFINE_STATIC_NAME(swapPartsId);
DEFINE_STATIC_NAME(swappingParts);
DEFINE_STATIC_NAME(weaponInside); // container's
DEFINE_STATIC_NAME(weaponInsideRG); // container's

// sockets
DEFINE_STATIC_NAME(weaponPartInfo);
DEFINE_STATIC_NAME(weaponInfo);

// sounds
DEFINE_STATIC_NAME(machineOn);
DEFINE_STATIC_NAME(newItem);

// emissives
DEFINE_STATIC_NAME(empty);
DEFINE_STATIC_NAME(active);
DEFINE_STATIC_NAME(swap);
DEFINE_STATIC_NAME(swapSafe);

// shader param
DEFINE_STATIC_NAME(highlightColour);
DEFINE_STATIC_NAME(totalColour);

// pilgrim overlay info ids
DEFINE_STATIC_NAME(weaponMixerWeaponLeft);
DEFINE_STATIC_NAME(weaponMixerWeaponRight);
DEFINE_STATIC_NAME(weaponMixerWeaponPartLeft);
DEFINE_STATIC_NAME(weaponMixerWeaponPartRight);

// ai messages
DEFINE_STATIC_NAME_STR(aim_pilgrimBlackboardUpdated, TXT("pilgrimBlackboardUpdated"));
	DEFINE_STATIC_NAME(who);

DEFINE_STATIC_NAME_STR(aim_wm_weaponMixed, TXT("weapon mixer; weapon mixed"));

//

static void draw_line(Colour const & _colour, Vector2 _a, Vector2 _b)
{
	::System::Video3DPrimitives::line_2d(_colour, _a, _b);
}

//

REGISTER_FOR_FAST_CAST(WeaponMixer);

WeaponMixer::WeaponMixer(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	weaponMixerData = fast_cast<WeaponMixerData>(_logicData);
	executionData = new ExecutionData();
}

WeaponMixer::~WeaponMixer()
{
}

void WeaponMixer::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	executionData->swapPartsHandler.advance(_deltaTime);

	if (( (executionData->swapPartsHandler.get_switch_at() == 1.0f &&
		   executionData->swapPartsHandler.get_prev_switch_at() < 1.0f)
#ifdef TEST_WEAPON_MIXER_SWAPPING_PARTS
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_STANDARD_INPUT
		||(
			(::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl) ||
			 ::System::Input::get()->is_key_pressed(::System::Key::RightCtrl)) &&
			::System::Input::get()->has_key_been_pressed(::System::Key::H)
			)
#endif
#endif
#endif
		)
		&& ! executionData->swappingParts)
	{
		struct SwapPart
		{
			ModuleEquipments::Gun* gun;
			WeaponPartAddress address;
			RefCountObjectPtr<WeaponPart> weaponPart;
			WeaponPartType::Type weaponPartType;
		};
		ArrayStatic<SwapPart, 2> swapParts; SET_EXTRA_DEBUG_INFO(swapParts, TXT("WeaponMixer::advance.swapParts"));
		for_every(container, executionData->containers)
		{
			if (auto* imo = container->containerIMO.get())
			{
				MODULE_OWNER_LOCK_FOR_IMO(imo, TXT("WeaponMixer  swap part for container"));
				if (auto* ih = imo->get_custom<CustomModules::ItemHolder>())
				{
					if (auto* h = ih->get_held())
					{
						if (auto* g = h->get_gameplay_as<ModuleEquipments::Gun>())
						{
							if (container->weaponParts.is_index_valid(container->selectedWeaponPartIdx))
							{
								SwapPart sp;
								sp.gun = g;
								sp.address = container->weaponParts[container->selectedWeaponPartIdx].address;
								if (auto* wp = g->get_weapon_setup().get_part_at(sp.address))
								{
									sp.weaponPart = wp;
									if (auto* wpt = wp->get_weapon_part_type())
									{
										sp.weaponPartType = wpt->get_type();
										swapParts.push_back(sp);
									}
								}
							}
						}
					}
				}
			}
		}
		if (swapParts.get_size() == 2)
		{
			auto& sp0 = swapParts[0];
			auto& sp1 = swapParts[1];
			if (sp0.weaponPartType == sp1.weaponPartType &&
				sp0.weaponPartType != WeaponPartType::GunChassis)
			{
				executionData->swappingParts = true;
				executionData->stopSwappingParts = false;
				executionData->swappingPartsTime = 0.0f;
				for_every(container, executionData->containers)
				{
					if (auto* imo = container->containerIMO.get())
					{
						if (auto* to = imo->get_temporary_objects())
						{
							to->spawn_all(NAME(swappingParts), NP, &container->swappingPartsTemporaryObjects);
						}
					}
				}
				if (auto* s = get_mind()->get_owner_as_modules_owner()->get_sound())
				{
					s->play_sound(NAME(swappingParts));
				}
				MODULE_OWNER_LOCK_FOR_IMO(sp0.gun->get_owner(), TXT("lock for weapon part swap"));
				MODULE_OWNER_LOCK_FOR_IMO(sp1.gun->get_owner(), TXT("lock for weapon part swap"));
				sp0.gun->access_weapon_setup().remove_part(sp0.address);
				sp1.gun->access_weapon_setup().remove_part(sp1.address);
				sp0.gun->access_weapon_setup().add_part(sp0.address, sp1.weaponPart.get());
				sp1.gun->access_weapon_setup().add_part(sp1.address, sp0.weaponPart.get());

				sp0.gun->request_async_create_parts(false);
				sp1.gun->request_async_create_parts(false);

				executionData->swappingPartsIn.push_back(SafePtr<Framework::IModulesOwner>(sp0.gun->get_owner()));
				executionData->swappingPartsIn.push_back(SafePtr<Framework::IModulesOwner>(sp1.gun->get_owner()));

				SafePtr<Framework::IModulesOwner> keeper(get_mind()->get_owner_as_modules_owner());
				Game::get_as<Game>()->add_async_world_job_top_priority(TXT("end swapping parts"),
					[keeper, this]()
					{
						if (keeper.get())
						{
							executionData->stopSwappingParts = true;
							for_every(container, executionData->containers)
							{
								container->store_weapon_setup();
							}
						}

						if (auto* message = Game::get()->get_world()->create_ai_message(NAME(aim_wm_weaponMixed)))
						{
							message->to_sub_world(get_mind()->get_owner_as_modules_owner()->get_in_sub_world());
						}

					});

			}
		}
	}

	if (executionData->swappingParts)
	{
		executionData->swappingPartsTime += _deltaTime;
		if (executionData->swappingPartsTime > 0.5f && executionData->stopSwappingParts)
		{
			executionData->swappingParts = false;
			for_every_ref(spi, executionData->swappingPartsIn)
			{
				if (auto* ap = spi->get_appearance())
				{
					ap->setup_with_pending_mesh();
				}
			}
			executionData->swappingPartsIn.clear();

			if (auto* s = get_mind()->get_owner_as_modules_owner()->get_sound())
			{
				s->stop_sound(NAME(swappingParts));
			}
			for_every(container, executionData->containers)
			{
				for_every_ref(to, container->swappingPartsTemporaryObjects)
				{
					Framework::ParticlesUtils::desire_to_deactivate(to);
				}
				container->swappingPartsTemporaryObjects.clear();
			}

			if (!markedAsKnownForOpenWorldDirection)
			{
				if (auto* piow = PilgrimageInstanceOpenWorld::get())
				{
					piow->mark_pilgrimage_device_direction_known(get_mind()->get_owner_as_modules_owner());
					markedAsKnownForOpenWorldDirection = true;
				}
			}
		}
	}

	for_every(container, executionData->containers)
	{
		if (auto* imo = container->containerIMO.get())
		{
			imo->access_variables().access<bool>(NAME(swappingParts)) = executionData->swappingParts;
		}
	}

	Optional<int> dominantContainer;

	{
		::Framework::IModulesOwner* newPilgrim = nullptr;
		newPilgrim = executionData->presentPilgrim.get();
		/*
		{
			for_every(container, executionData->containers)
			{
				if (auto* gby = container->choosePartHandler.get_grabbed_by())
				{
					gby = gby->get_top_instigator();
					if (executionData->holdingDialsPilgrim == gby)
					{
						newPilgrim = gby;
					}
					else if (!newPilgrim)
					{
						if (gby->get_gameplay_as<ModulePilgrim>())
						{
							newPilgrim = gby;
						}
					}
				}
			}
			if (auto* gby = executionData->swapPartsHandler.get_grabbed_by())
			{
				gby = gby->get_top_instigator();
				if (executionData->holdingDialsPilgrim == gby)
				{
					newPilgrim = gby;
				}
				else if (!newPilgrim)
				{
					if (gby->get_gameplay_as<ModulePilgrim>())
					{
						newPilgrim = gby;
					}
				}
			}
		}
		*/
		if (executionData->holdingDialsPilgrim != newPilgrim)
		{
			if (executionData->holdingDialsPilgrim.is_set())
			{
				if (auto* mp = executionData->holdingDialsPilgrim->get_gameplay_as<ModulePilgrim>())
				{
					mp->access_overlay_info().hide_show_info(NAME(weaponMixerWeaponPartLeft));
					mp->access_overlay_info().hide_show_info(NAME(weaponMixerWeaponPartRight));
				}
			}
			executionData->holdingDialsPilgrim = newPilgrim;
			for_every(container, executionData->containers)
			{
				container->showWeaponPartInfo.clear();
			}
		}
		if (executionData->currentPilgrim != executionData->presentPilgrim)
		{
			if (executionData->currentPilgrim.is_set())
			{
				if (auto* mp = executionData->currentPilgrim->get_gameplay_as<ModulePilgrim>())
				{
					mp->access_overlay_info().hide_show_info(NAME(weaponMixerWeaponLeft));
					mp->access_overlay_info().hide_show_info(NAME(weaponMixerWeaponRight));
				}
			}
			executionData->currentPilgrim = executionData->presentPilgrim;
		}
	}

	for_every(container, executionData->containers)
	{
		container->choosePartHandler.advance(_deltaTime);

		if (container->displaySetup)
		{
			container->timeToRedraw -= _deltaTime;
			container->inStateTime += _deltaTime;
			if (container->timeToRedraw <= 0.0f)
			{
				container->redrawNow = true;
				container->timeToRedraw = 0.1f;
			}
			bool wasOff = container->state == Container::ContainerOff;
			auto prevState = container->state;
			container->timeToCheckOn -= _deltaTime;
			if (container->timeToCheckOn <= 0.0f || !executionData->containersReady)
			{
				if (container->state == Container::ContainerOff)
				{
					container->timeToCheckOn = Random::get_float(0.0f, 0.3f);
				}
				else
				{
					container->timeToCheckOn = 0.0f;
				}

				if (executionData->containersReady &&
					container->shouldBeOn)
				{
					if (container->state == Container::ContainerShutDown ||
						container->state == Container::ContainerOff)
					{
						container->state = Container::ContainerStart;
					}
				}
				else
				{
					if (container->state != Container::ContainerShutDown &&
						container->state != Container::ContainerOff)
					{
						container->state = Container::ContainerShutDown;
					}
				}

				if (container->state != prevState)
				{
					container->inStateDrawnFrames = 0;
					container->inStateTime = 0.0f;
					container->redrawNow = true;
					prevState = container->state;
				}
			}

			if (container->redrawNow)
			{
				++container->inStateDrawnFrames;

				if (container->state == Container::ContainerStart)
				{
					if (container->inStateTime > 0.8f)
					{
						container->state = !container->does_hold_weapon() ? Container::ContainerShowEmpty : Container::ContainerShowContent;
						container->timeToRedraw = 0.1f;
					}
				}
				else if (container->state == Container::ContainerShutDown)
				{
					int frames = 10;
					int frameNo = container->inStateDrawnFrames;

					container->timeToRedraw = 0.03f;

					container->variousDrawingVariables.shutdownAtPt = (float)frameNo / (float)frames;

					if (frameNo >= frames)
					{
						container->state = Container::ContainerOff;
					}
				}
				else if (container->state == Container::ContainerShowEmpty)
				{
					container->timeToRedraw = 0.03f;
					if (container->does_hold_weapon())
					{
						container->state = Container::ContainerShowContent;
						if (auto* s = container->containerIMO->get_sound())
						{
							s->play_sound(NAME(newItem));
						}
					}
				}
				else if (container->state == Container::ContainerShowContent)
				{
					container->timeToRedraw = 0.03f;
					if (!container->does_hold_weapon())
					{
						container->state = Container::ContainerShowEmpty;
					}
					else
					{
						/*
						container->variousDrawingVariables.contentLineModel = nullptr;
						container->variousDrawingVariables.borderLineModel = nullptr;

						if (container->canister.content.exm.is_set())
						{
							container->variousDrawingVariables.contentLineModel = container->canister.content.exm->get_line_model_for_display();
							container->variousDrawingVariables.borderLineModel = container->canister.content.exm->is_active()? weaponMixerData->activeEXMLineModel.get() : weaponMixerData->passiveEXMLineModel.get();
						}
						if (container->canister.content.extra.is_set())
						{
							container->variousDrawingVariables.contentLineModel = container->canister.content.extra->get_line_model_for_display();
						}

						container->variousDrawingVariables.contentDrawLines = (float)(container->inStateDrawnFrames * 2);
						if (container->variousDrawingVariables.contentLineModel)
						{
							container->timeToRedraw = 0.03f;
						}
						else
						{
							container->timeToRedraw = 5.0f;
						}
						*/
					}
				}

				if (container->state != prevState)
				{
					container->inStateDrawnFrames = 0;
					container->inStateTime = 0.0f;
					container->redrawNow = true;

					container->store_weapon_setup();
				}

			}
			bool isOff = container->state == Container::ContainerOff;
			if ((isOff ^ wasOff) &&
				container->containerIMO.get())
			{
				if (auto* s = container->containerIMO->get_sound())
				{
					if (wasOff)
					{
						s->play_sound(NAME(machineOn));
					}
					else
					{
						s->stop_sound(NAME(machineOn));
					}
				}
			}
			{
				Name shouldBeEmissive;

				if (executionData->swappingParts)
				{
					if (PlayerPreferences::are_currently_flickering_lights_allowed())
					{
						shouldBeEmissive = NAME(swap);
					}
					else
					{
						shouldBeEmissive = NAME(swapSafe);
					}
				}
				else if (container->state == Container::ContainerShowEmpty)
				{
					shouldBeEmissive = NAME(empty);
				}
				else if (container->state == Container::ContainerShowContent)
				{
					shouldBeEmissive = NAME(active);
				}

				if (shouldBeEmissive != container->activeEmissive)
				{
					container->activeEmissive = shouldBeEmissive;
					if (auto* e = container->containerIMO->get_custom<CustomModules::EmissiveControl>())
					{
						e->emissive_deactivate_all();
						if (container->activeEmissive.is_valid())
						{
							e->emissive_activate(container->activeEmissive);
						}
					}
				}
			}
		}

		if (container->display && executionData->containersReady && !container->displaySetup)
		{
			container->displaySetup = true;
			container->display->draw_all_commands_immediately();
			container->display->set_on_update_display(this,
				[this, container](Framework::Display* _display)
				{
					if (!container->redrawNow)
					{
						return;
					}
					container->redrawNow = false;

					if (executionData->swappingParts)
					{
						auto* cr = new Framework::DisplayDrawCommands::ClearRegion();
						Framework::RangeCharCoord2 sr = _display->get_char_screen_rect();
						Framework::RangeCharCoord2 r = sr;
						if (Random::get_chance(0.75f))
						{
							r.x.max = r.x.min = Random::get_int_from_range(r.x.min, r.x.max);
							r.x.min -= Random::get_int_from_range(0, 3);
							r.x.max -= Random::get_int_from_range(0, 3);
							r.y.max = r.y.min = Random::get_int_from_range(r.y.min, r.y.max);
							r.y.min -= Random::get_int_from_range(0, 3);
							r.y.max -= Random::get_int_from_range(0, 3);
						}
						else if (Random::get_bool())
						{
							r.x.max = r.x.min = Random::get_int_from_range(r.x.min, r.x.max);
							r.x.min -= Random::get_int_from_range(0, 1);
							r.x.max -= Random::get_int_from_range(0, 1);
						}
						else
						{
							r.y.max = r.y.min = Random::get_int_from_range(r.y.min, r.y.max);
							r.y.min -= Random::get_int_from_range(0, 1);
							r.y.max -= Random::get_int_from_range(0, 1);
						}
						{
							r.x.min = sr.x.clamp(r.x.min);
							r.x.max = sr.x.clamp(r.x.max);
							r.y.min = sr.y.clamp(r.y.min);
							r.y.max = sr.y.clamp(r.y.max);
						}
						cr->rect(r);
						if (Random::get_chance(0.6f))
						{
							Colour c = Colour::black;
							float g = 0.6f;
							if (Random::get_chance(0.6f))
							{
								c.r = g;
							}
							else if (Random::get_chance(0.6f))
							{
								c.r = g;
								c.g = g;
								c.b = g;
							}
							cr->paper(c);
						}
						_display->add(cr);
						return;
					}

					if (container->state != Container::ContainerShowContent || container->drawnForState != Container::ContainerShowContent) // show content will clear only if has enough parts
					{
						_display->drop_all_draw_commands();
						if (container->state != Container::ContainerStart)
						{
							_display->add((new Framework::DisplayDrawCommands::CLS(Colour::black)));
						}
						_display->add((new Framework::DisplayDrawCommands::Border(Colour::black)));
					}

					if (container->state == Container::ContainerStart)
					{
						if (container->inStateDrawnFrames == 0) _display->add((new Framework::DisplayDrawCommands::CLS(Colour::black)));
						container->redrawNow = false;
						container->timeToRedraw = 0.06f;
						int frames = container->inStateDrawnFrames;
						_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
							[frames](Framework::Display* _display, ::System::Video3D* _v3d)
							{
								Utils::draw_device_display_start(_display, _v3d, frames);
							}));
					}
					else if (container->state == Container::ContainerShutDown)
					{
						Vector2 bl = _display->get_left_bottom_of_screen() + Vector2::half;
						Vector2 tr = _display->get_right_top_of_screen() - Vector2::half;

						float atY = tr.y + (bl.y - tr.y) * clamp(container->variousDrawingVariables.shutdownAtPt, 0.0f, 1.0f);

						Vector2 a(bl.x, atY);
						Vector2 b(tr.x, atY);
						_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
							[a, b](Framework::Display* _display, ::System::Video3D* _v3d)
							{
								::System::Video3DPrimitives::line_2d(_display->get_current_ink(), a, b);
							}));
					}
					else if (container->state == Container::ContainerShowEmpty)
					{
						if (::System::Core::get_timer_mod(1.0f) < 0.5f)
						{
							// show flickering weapon, use core timer mod to have both screens blink in sync
							auto* lm = weaponMixerData->emptyContainerLineModel.get();
							Vector2 side(container->containerIdx == 0 ? 1.0f : -1.0f, 1.0f);
							_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
								[lm, side](Framework::Display* _display, ::System::Video3D* _v3d)
								{
									Colour const ink = _display->get_current_ink();

									Vector2 bl = _display->get_left_bottom_of_screen() + Vector2::half;
									Vector2 tr = _display->get_right_top_of_screen() - Vector2::half;

									Vector2 c = TypeConversions::Normal::f_i_cells((bl + tr) * 0.5f).to_vector2() + Vector2::half;

									Vector2 size(tr.x - c.x, tr.y - c.y);
									size.x = min(size.x, size.y);
									size.y = size.x;

									size = TypeConversions::Normal::f_i_closest(size * 2.0f).to_vector2();

									size *= 0.8f;
									if (lm)
									{
										for_every(l, lm->get_lines())
										{
											draw_line(l->colour.get(ink),
												TypeConversions::Normal::f_i_cells(c + size * side * l->a.to_vector2()).to_vector2() + Vector2::half,
												TypeConversions::Normal::f_i_cells(c + size * side * l->b.to_vector2()).to_vector2() + Vector2::half);
										}
									}
								}
							));
						}
					}
					else if (container->state == Container::ContainerShowContent)
					{
						if (auto* ih = container->containerIMO->get_custom<CustomModules::ItemHolder>())
						{
							if (auto* h = ih->get_held())
							{
								if (auto* g = h->get_gameplay_as<ModuleEquipments::Gun>())
								{
									auto& ws = g->get_weapon_setup();

									if (container->weaponPartsForSetup != ws)
									{
										container->weaponPartsForSetup.copy_from(ws);

										Concurrency::ScopedSpinLock lock(container->weaponPartsLock);

										container->weaponParts.clear();

										for_count(int, pass, 2)
										{
											for_every(p, container->weaponPartsForSetup.get_parts())
											{
												if (auto* wp = p->part.get())
												{
													if (auto* wpt = wp->get_weapon_part_type())
													{
														bool isGunChasiss = wpt->get_type() == WeaponPartType::GunChassis;
														if ((pass == 0 && isGunChasiss) ||
															(pass == 1 && !isGunChasiss))
														{
															container->weaponParts.grow_size(1);
															container->weaponParts.get_last().weaponPart = wp;
															container->weaponParts.get_last().weaponPartType = wpt->get_type();;
															container->weaponParts.get_last().address = p->at;
														}
													}
												}
											}
										}

										sort(container->weaponParts); // sort by address

#ifdef DEBUG_SELECTED_WEAPON_PART_IDX
										output(TXT("[weapon mixer] weapon parts for container %i"), container->containerIdx);
										for_every(part, container->weaponParts)
										{
											output(TXT("[weapon mixer]   %i wpt%i addrs:%S"), for_everys_index(part), part->weaponPartType, part->address.to_string().to_char());
										}
#endif

										// get locations
										for_every(wp, container->weaponParts)
										{
											wp->renderAt.clear();
											wp->angle.clear();
										}
									}
								}
							}
						}
						bool allPresent = true;
						{
							Concurrency::ScopedSpinLock lock(container->weaponPartsLock);
							if (!container->weaponParts.is_empty())
							{
								if (auto* wp = container->weaponParts.get_first().weaponPart.get())
								{
									if (!wp->get_container_schematic_mesh_instance())
									{
										allPresent = false;
									}
								}
								if (allPresent)
								{
									for_every(part, container->weaponParts)
									{
										if (auto* wp = part->weaponPart.get())
										{
											if (!wp->get_schematic_mesh_instance())
											{
												allPresent = false;
												break;
											}
										}
									}
								}
							}
						}

						if (allPresent)
						{
							_display->drop_all_draw_commands();
							_display->add((new Framework::DisplayDrawCommands::CLS(Colour::black)));
							_display->add((new Framework::DisplayDrawCommands::Border(Colour::black)));

							_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
								[container](Framework::Display* _display, ::System::Video3D* _v3d)
								{
									Concurrency::ScopedSpinLock lock(container->weaponPartsLock);
									if (!container->weaponParts.is_empty())
									{
										Range2 screen = Range2::empty;
										screen.include(_display->get_left_bottom_of_screen());
										screen.include(_display->get_right_top_of_display());

										float scaleToContainer = screen.x.length() / WeaponPart::get_container_schematic_size().x;

										if (auto* wpch = container->weaponParts.get_first().weaponPart.get())
										{
											wpch->make_sure_schematic_mesh_exists();
											if (auto* mi = cast_to_nonconst(wpch->get_container_schematic_mesh_instance()))
											{
												{
													System::VideoMatrixStack& modelViewStack = _v3d->access_model_view_matrix_stack();
													Vector2 mat = screen.centre();
													modelViewStack.push_to_world(Transform(mat.to_vector3(), Rotator3(0.0f, 0.0f, 0.0f).to_quat(), scaleToContainer).to_matrix());
													Meshes::Mesh3DRenderingContext renderingContext;
													mi->render(_v3d, renderingContext);
													modelViewStack.pop();
												}

#ifndef ALLOW_SELECTING_CHASSIS
												if (container->selectedWeaponPartIdx == 0)
												{
													container->selectedWeaponPartIdx = 1;
												}
#endif

												// only if we have the right mesh instance
												bool wereAllAnglesValid = true;
												int validAngles = 0;
												for_every(part, container->weaponParts)
												{
													if (!part->renderAt.is_set())
													{
														wereAllAnglesValid = false;
														auto at = wpch->get_container_schematic_slot_at(part->address);
														if (at.is_set())
														{
															part->renderAt = at;
															if (part->weaponPartType == WeaponPartType::GunChassis)
															{
																part->angle = 0.0f;
															}
															else
															{
																part->angle = mod(at.get().centre().angle(), 360.0f);
															}
															++validAngles;
														}
													}
													else
													{
														++validAngles;
													}
													if (part->renderAt.is_set())
													{
														if (auto* wp = part->weaponPart.get())
														{
															if (auto* mi = cast_to_nonconst(wp->get_schematic_mesh_instance()))
															{
																System::VideoMatrixStack& modelViewStack = _v3d->access_model_view_matrix_stack();
																Vector2 mat = screen.centre() + part->renderAt.get().centre() * scaleToContainer;
																modelViewStack.push_to_world(Transform(mat.to_vector3(), Rotator3(0.0f, 0.0f, 0.0f).to_quat(), scaleToContainer).to_matrix());
																Meshes::Mesh3DRenderingContext renderingContext;
																{
																	bool selected = for_everys_index(part) == container->selectedWeaponPartIdx;
																	Colour highlightColour = selected ? Colour::red.with_alpha(0.3f) : Colour::none;
																	Colour totalColour = Colour::white;
																	for_count(int, idx, mi->get_material_instance_count())
																	{
																		mi->access_material_instance(idx)->set_uniform(NAME(highlightColour), highlightColour.to_vector4());
																		mi->access_material_instance(idx)->set_uniform(NAME(totalColour), totalColour.to_vector4());
																	}
																}
																mi->render(_v3d, renderingContext);
																modelViewStack.pop();
															}
														}
													}
												}

												if (!wereAllAnglesValid && validAngles == container->weaponParts.get_size())
												{
													bool ok = false;
													while (! ok)
													{
														ok = true;
														for_every(part, container->weaponParts)
														{
															for_every_continue_after(other, part)
															{
																if (other->angle.get() == part->angle.get())
																{
																	ok = false;
																	other->angle = other->angle.get() + 0.01f;
																}
															}
														}
													}
												}
											}
										}
									}
								}));
						}
					}
					container->drawnForState = container->state;
				});
		}
	}

	// make sure all values point
	{
		for_every(container, executionData->containers)
		{
			if (container->state == Container::ContainerShowContent)
			{
				int dialAt = container->choosePartHandler.get_dial_at();
#ifdef TEST_WEAPON_MIXER_SWAPPING_PARTS
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_STANDARD_INPUT
				if (container->containerIdx == 1 &&
					::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl))
				{
					if (::System::Input::get()->has_key_been_pressed(::System::Key::J))
					{
						--dialAt;
					}
					if (::System::Input::get()->has_key_been_pressed(::System::Key::K))
					{
						++dialAt;
					}
				}
				if (container->containerIdx == 2 &&
					::System::Input::get()->is_key_pressed(::System::Key::RightCtrl))
				{
					if (::System::Input::get()->has_key_been_pressed(::System::Key::J))
					{
						--dialAt;
					}
					if (::System::Input::get()->has_key_been_pressed(::System::Key::K))
					{
						++dialAt;
					}
				}
#endif
#endif
#endif

				if (dialAt != 0 && ! executionData->swappingParts)
				{
					if (container->weaponParts.get_size() > 1)
					{
						int idx = container->selectedWeaponPartIdx;
						while (dialAt != 0)
						{
							float atAngle = container->weaponParts[idx].angle.get(0.0f);
							{
								int bestIdx = NONE;
								float bestDiff = 0.0f;
								for_every(part, container->weaponParts)
								{
									int partIdx = for_everys_index(part);
									if (partIdx == idx
#ifndef ALLOW_SELECTING_CHASSIS
										|| partIdx == 0
#endif
										)
									{
										continue;
									}
									float pang = part->angle.get(0.0f);
									float diff = Rotator3::normalise_axis(pang - atAngle);
									if (dialAt < 0)
									{
										diff = -diff;
									}
									diff = mod(diff, 360.0f);
									if (bestIdx == NONE || bestDiff > diff)
									{
										bestIdx = partIdx;
										bestDiff = diff;
									}
								}
								idx = bestIdx;
							}
							dialAt += (dialAt > 0 ? -1 : 1);
						}
						container->selectedWeaponPartIdx = idx;
#ifdef DEBUG_SELECTED_WEAPON_PART_IDX
						output(TXT("[weapon mixer] container %i changed selected to %i wpt%i"), container->containerIdx, container->selectedWeaponPartIdx, container->weaponParts[container->selectedWeaponPartIdx].weaponPartType);
#endif
					}
					container->choosePartHandler.reset_dial_zero();
					dominantContainer = container->containerIdx;
				}

				dominantContainer.set_if_not_set(container->containerIdx);
				container->selectedWeaponPartIdxPerWeaponPartType.set_size(WeaponPartType::NUM);
				// set for currently selected
				if (container->weaponParts.is_index_valid(container->selectedWeaponPartIdx))
				{
					auto& part = container->weaponParts[container->selectedWeaponPartIdx];
					if (part.weaponPartType >= 0)
					{
#ifdef DEBUG_SELECTED_WEAPON_PART_IDX
						if (container->selectedWeaponPartIdxPerWeaponPartType[part.weaponPartType] != container->selectedWeaponPartIdx)
						{
							output(TXT("[weapon mixer] container %i, remember selected part %i for wpt%i"), container->containerIdx, container->selectedWeaponPartIdx, part.weaponPartType);
						}
#endif
						container->selectedWeaponPartIdxPerWeaponPartType[part.weaponPartType] = container->selectedWeaponPartIdx;
					}
				}
				else
				{
#ifdef DEBUG_SELECTED_WEAPON_PART_IDX
					output(TXT("[weapon mixer] container %i, select part idx %i "), container->containerIdx, container->selectedWeaponPartIdx);
#endif
#ifdef ALLOW_SELECTING_CHASSIS
					container->selectedWeaponPartIdx = 0;
#else
					container->selectedWeaponPartIdx = 1;
#endif
				}
				// clear if don't match and provide any valid
				for_every(swpipwt, container->selectedWeaponPartIdxPerWeaponPartType)
				{
					// clear if don't match
					if (swpipwt->is_set())
					{
						if (container->weaponParts.is_index_valid(swpipwt->get()))
						{
							auto& part = container->weaponParts[swpipwt->get()];
							if (part.weaponPartType != for_everys_index(swpipwt))
							{
#ifdef DEBUG_SELECTED_WEAPON_PART_IDX
								output(TXT("[weapon mixer] container %i, clear swpt%i (type mismatch wpt%i)"), container->containerIdx, for_everys_index(swpipwt), part.weaponPartType);
#endif
								swpipwt->clear();
							}
						}
						else
						{
#ifdef DEBUG_SELECTED_WEAPON_PART_IDX
							output(TXT("[weapon mixer] container %i, clear swpt%i (out of bounds)"), container->containerIdx, for_everys_index(swpipwt));
#endif
							swpipwt->clear();
						}
					}
					// provide any valid
					if (!swpipwt->is_set())
					{
						for_every(part, container->weaponParts)
						{
							if (part->weaponPartType == for_everys_index(swpipwt))
							{
								swpipwt->set(for_everys_index(part));
#ifdef DEBUG_SELECTED_WEAPON_PART_IDX
								output(TXT("[weapon mixer] container %i, provide any swpt%i = %i"), container->containerIdx, for_everys_index(swpipwt), swpipwt->get());
#endif
								break;
							}
						}
					}
				}
			}
		}
	}

	// at this point we should have only valid values and selected dominant container
	// now make sure that the other container points at the same type of weapon part, so we can swap them properly
	if (dominantContainer.is_set())
	{
		auto& dominant = executionData->containers[dominantContainer.get()];
		if (dominant.state == Container::ContainerShowContent)
		{
			if (dominant.weaponParts.is_index_valid(dominant.selectedWeaponPartIdx))
			{
				WeaponPartType::Type matchWeaponPartType = dominant.weaponParts[dominant.selectedWeaponPartIdx].weaponPartType;
				auto& other = executionData->containers[1 - dominantContainer.get()];
				if (other.state == Container::ContainerShowContent)
				{
					if (other.selectedWeaponPartIdxPerWeaponPartType.is_index_valid(matchWeaponPartType) &&
						other.selectedWeaponPartIdxPerWeaponPartType[matchWeaponPartType].is_set())
					{
#ifdef DEBUG_SELECTED_WEAPON_PART_IDX
						if (other.selectedWeaponPartIdx != other.selectedWeaponPartIdxPerWeaponPartType[matchWeaponPartType].get())
						{
							output(TXT("[weapon mixer] container %i -> %i match wpt%i = %i"), dominantContainer.get(), 1 - dominantContainer.get(), matchWeaponPartType, other.selectedWeaponPartIdxPerWeaponPartType[matchWeaponPartType].get());
						}
#endif
						other.selectedWeaponPartIdx = other.selectedWeaponPartIdxPerWeaponPartType[matchWeaponPartType].get();
					}
					else
					{
						for_every(part, other.weaponParts)
						{
							if (part->weaponPartType == matchWeaponPartType)
							{
#ifdef DEBUG_SELECTED_WEAPON_PART_IDX
								if (other.selectedWeaponPartIdx != for_everys_index(part))
								{
									output(TXT("[weapon mixer] container %i -> %i match any wpt%i = %i"), dominantContainer.get(), 1 - dominantContainer.get(), matchWeaponPartType, for_everys_index(part));
								}
#endif
								other.selectedWeaponPartIdx = for_everys_index(part);
								break;
							}
						}
					}
				}
			}
		}
	}

	// show infos
	{
		if (executionData->holdingDialsPilgrim.is_set())
		{
			if (auto* mp = executionData->holdingDialsPilgrim->get_gameplay_as<ModulePilgrim>())
			{
				for_every(container, executionData->containers)
				{
					Concurrency::ScopedSpinLock lock(container->weaponPartsLock);

					Name weaponMixerWeaponPart = container->containerIdx == 0 ? NAME(weaponMixerWeaponPartLeft) : NAME(weaponMixerWeaponPartRight);
					RefCountObjectPtr<WeaponPart> showPart;
					if (container->state == Container::ContainerShowContent &&
						! executionData->swappingParts)
					{
						if (container->weaponParts.is_index_valid(container->selectedWeaponPartIdx))
						{
							showPart = container->weaponParts[container->selectedWeaponPartIdx].weaponPart.get();
						}
					}
					if (container->showWeaponPartInfo != showPart)
					{
						mp->access_overlay_info().hide_show_info(weaponMixerWeaponPart);

						if (showPart.is_set() && container->containerIMO.is_set())
						{
							Transform showInfoAt = container->containerIMO->get_presence()->get_placement();
							if (auto* ap = container->containerIMO->get_appearance())
							{
								showInfoAt = showInfoAt.to_world(ap->calculate_socket_os(NAME(weaponPartInfo)));
							}
							OverlayInfo::TextColours textColours;
							List<WeaponPart::StatInfo> statsInfo;
							showPart->build_overlay_stats_info(OUT_ statsInfo, OPTIONAL_ OUT_ &textColours);
							float size = 0.015f;
#ifdef MEASURE_SHOW_INFO_SIZES
							statsInfo.clear();
							for_count(int, i, MEASURE_SHOW_INFO_SIZES)
							{
								statsInfo.push_back(String::printf(TXT("line %i  lines %i"), i, i + 1));
							}
#endif
#ifdef SCALE_SHOW_INFO
							{	// scale to match
								int maxLines = hardcoded 13;
								size *= (float)maxLines / (float)max(maxLines, statsInfo.get_size());
							}
#endif
							String statsInfoString;
							String newLine(TXT("~"));
							for_every(si, statsInfo)
							{
								if (!statsInfoString.is_empty())
								{
									statsInfoString += newLine;
								}
								statsInfoString += si->text;
							}

							mp->access_overlay_info().show_info(weaponMixerWeaponPart, showInfoAt, statsInfoString, textColours,
								size, hardcoded /* width */0.7f, false, Vector2(0.0f, 1.0f));
						}
						else
						{
							showPart.clear();
						}
						container->showWeaponPartInfo = showPart;
					}
				}
			}
		}
		else
		{
			for_every(container, executionData->containers)
			{
				container->showWeaponPartInfo.clear();
			}
		}

		//

		if (executionData->currentPilgrim.is_set())
		{
			//if (!executionData->swappingParts) // keep them as they are
			{
				if (auto* mp = executionData->currentPilgrim->get_gameplay_as<ModulePilgrim>())
				{
					for_every(container, executionData->containers)
					{
						WeaponSetup const* ws = nullptr;
						ModuleEquipments::Gun const* gun = nullptr;
						if (!executionData->swappingParts && ! executionData->redoGunStats)
						{
							if (auto* ih = container->containerIMO->get_custom<CustomModules::ItemHolder>())
							{
								if (auto* h = ih->get_held())
								{
									if (auto* g = h->get_gameplay_as<ModuleEquipments::Gun>())
									{
										gun = g;
										ws = &g->get_weapon_setup();
									}
								}
							}
						}

						Name weaponMixerWeaponSetup = container->containerIdx == 0 ? NAME(weaponMixerWeaponLeft) : NAME(weaponMixerWeaponRight);

						if ((!ws && !container->showWeaponSetupInfo.is_empty()) ||
							(ws && container->showWeaponSetupInfo != *ws) ||
							(container->gunStatsForWeaponPartIdx != container->selectedWeaponPartIdx && container->weaponParts.is_index_valid(container->selectedWeaponPartIdx)))
						{
							mp->access_overlay_info().hide_show_info(weaponMixerWeaponSetup);
							container->showWeaponSetupInfo.clear();
							if (container->weaponParts.is_index_valid(container->selectedWeaponPartIdx))
							{
								container->gunStatsForWeaponPartIdx = container->selectedWeaponPartIdx; // the clearing will effectively redo this
							}
						}
						if (ws &&
							(container->showWeaponSetupInfo.is_empty() ||
							 container->showWeaponSetupInfo != *ws))
						{
							container->showWeaponSetupInfo.copy_from(*ws);
							an_assert(gun);

							Transform showInfoAt = container->containerIMO->get_presence()->get_placement();
							if (auto* ap = container->containerIMO->get_appearance())
							{
								showInfoAt = showInfoAt.to_world(ap->calculate_socket_os(NAME(weaponInfo)));
							}
							OverlayInfo::TextColours textColours;
							List<ModuleEquipments::GunChosenStats::StatInfo> statsInfo;
							{
								executionData->workingGunChosenStats.reset();
								gun->build_chosen_stats(REF_ executionData->workingGunChosenStats, true, executionData->currentPilgrim.get(), container->containerIdx == 0 ? Hand::Left : Hand::Right /* does not actually matter ATM */);
								executionData->workingGunChosenStats.build_overlay_stats_info(OUT_ statsInfo, false, OPTIONAL_ OUT_ &textColours);
							}
							// add stat info for selected weapon part
							{
								WeaponStatAffection::ready_to_use();
								String prefix;
								WeaponPart* wp = nullptr;
								if (container->weaponParts.is_index_valid(container->selectedWeaponPartIdx))
								{
									wp = container->weaponParts[container->selectedWeaponPartIdx].weaponPart.get();
								}
								for_every(s, statsInfo)
								{
									prefix = String::space();
									for_every(affectedBy, s->affectedBy)
									{
										if (WeaponPart::is_same_content(affectedBy->weaponPart, wp)) // they could be different instances
										{
											prefix = WeaponStatAffection::to_ui_symbol(affectedBy->how);
										}
									}
									prefix = prefix.get_left(1);
									if (s->text.find_first_of('#') == 0)
									{
										int putAt = s->text.find_first_of('#', 1);
										if (putAt == NONE)
										{
											putAt = 0;
										}
										else
										{
											++putAt;
										}
										s->text = s->text.get_left(putAt) + prefix + String::space() + s->text.get_sub(putAt);
									}
									else
									{
										s->text = prefix + String::space() + s->text;
									}
								}
							}
							float size = 0.015f;
#ifdef MEASURE_SHOW_INFO_SIZES
							statsInfo.clear();
							for_count(int, i, MEASURE_SHOW_INFO_SIZES)
							{
								statsInfo.push_back(String::printf(TXT("line %i  lines %i"), i, i + 1));
							}
#endif
#ifdef SCALE_SHOW_INFO
							{	// scale to match
								int maxLines = hardcoded 17;
								size *= (float)maxLines / (float)max(maxLines, statsInfo.get_size());
							}
#endif
							String statsInfoString;
							String newLine(TXT("~"));
							for_every(si, statsInfo)
							{
								if (!statsInfoString.is_empty())
								{
									statsInfoString += newLine;
								}
								statsInfoString += si->text;
							}

							mp->access_overlay_info().show_info(weaponMixerWeaponSetup, showInfoAt, statsInfoString, textColours,
								size, hardcoded /* width */0.7f, false, Vector2(0.0f, 1.0f));
						}
					}
				}
			}
		}
		else
		{
			for_every(container, executionData->containers)
			{
				container->showWeaponSetupInfo.clear();
			}
		}
	}

	executionData->redoGunStats = false;
}

void WeaponMixer::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);
}

LATENT_FUNCTION(WeaponMixer::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai weapon mixer] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	//LATENT_VAR(Optional<VectorInt2>, cellAt);
	LATENT_VAR(float, playerCheckTimeLeft);
	LATENT_VAR(bool, playerIn);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<WeaponMixer>(logic);

	playerCheckTimeLeft -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	ai_log(self, TXT("upgrade container, hello!"));

	playerIn = false;
	playerCheckTimeLeft = 0.0f;

	messageHandler.use_with(mind);
	{
		RefCountObjectPtr<ExecutionData> keepExecutionData;
		keepExecutionData = self->executionData;
		messageHandler.set(NAME(aim_pilgrimBlackboardUpdated), [keepExecutionData](Framework::AI::Message const& _message)
			{
				if (auto* who = _message.get_param(NAME(who)))
				{
					auto* whoImo = who->get_as<SafePtr<Framework::IModulesOwner>>().get();
					if (whoImo == keepExecutionData->currentPilgrim.get())
					{
						keepExecutionData->redoGunStats = true;
					}
				}
			}
		);
	}

	if (imo)
	{
		self->executionData->containers.clear();
		self->executionData->containers.set_size(2);
		self->executionData->containers[0].weaponMixer = self;
		self->executionData->containers[1].weaponMixer = self;
		self->executionData->containers[0].containerIdx = 0;
		self->executionData->containers[1].containerIdx = 1;
		{
			while (!self->executionData->containers[0].containerIMO.get() ||
				   !self->executionData->containers[1].containerIMO.get())
			{
				{
					for_every(container, self->executionData->containers)
					{
						{
							int i = for_everys_index(container);
							{
								Name wcIdName = i == 0 ? NAME(weaponContainerIdLeft) : NAME(weaponContainerIdRight);
								if (auto* id = imo->get_variables().get_existing<int>(wcIdName))
								{
									auto* world = imo->get_in_world();
									an_assert(world);

									world->for_every_object_with_id(NAME(weaponContainerId), *id,
										[imo, &container](Framework::Object* _object)
										{
											if (_object != imo)
											{
												container->containerIMO = _object;
												if (auto* cmd = _object->get_custom<Framework::CustomModules::Display>())
												{
													container->display = cmd->get_display();
													container->redrawNow = true;
													return true; // one is enough
												}
											}
											return false; // keep going on
										});
								}
							}
							{
								Name cpIdName = i == 0 ? NAME(choosePartIdLeft) : NAME(choosePartIdRight);
								if (auto* id = imo->get_variables().get_existing<int>(cpIdName))
								{
#ifdef AN_DEVELOPMENT
									auto* world = imo->get_in_world();
									an_assert(world);
#endif

									container->choosePartHandler.initialise(imo, cpIdName);
								}
							}
						}
					}
				}
				LATENT_WAIT(Random::get_float(0.2f, 0.5f));
			}
			while (self->executionData->swapPartsHandler.get_switches().is_empty())
			{
				self->executionData->swapPartsHandler.initialise(imo, NAME(swapPartsId));
				LATENT_WAIT(Random::get_float(0.2f, 0.5f));
			}
		}
		self->executionData->containersReady = true;
		ai_log(self, TXT("containers available"));
	}

	while (true)
	{
		if (playerCheckTimeLeft <= 0.0f)
		{
			playerCheckTimeLeft = Random::get_float(0.1f, 0.4f);
			bool isPlayerInNow = false;
			{
				//FOR_EVERY_OBJECT_IN_ROOM(object, imo->get_presence()->get_in_room())
				for_every_ptr(object, imo->get_presence()->get_in_room()->get_objects())
				{
					if (object->get_gameplay_as<ModulePilgrim>())
					{
						if (!self->executionData->presentPilgrim.is_set())
						{
							self->executionData->presentPilgrim = object;
						}
						isPlayerInNow = true;
						break;
					}
				}
			}
			if (!isPlayerInNow)
			{
				self->executionData->presentPilgrim.clear();
			}
			for_every(container, self->executionData->containers)
			{
				container->shouldBeOn = isPlayerInNow;
			}

			playerIn = isPlayerInNow;
		}
		{
			for_every(container, self->executionData->containers)
			{
				container->update(LATENT_DELTA_TIME);
			}
		}
		LATENT_YIELD(); // to allow advance every frame
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(WeaponMixerData);

bool WeaponMixerData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= emptyContainerLineModel.load_from_xml(_node, TXT("emptyContainerLineModel"), _lc);

	return result;
}

bool WeaponMixerData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= emptyContainerLineModel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

//

WeaponMixer::Container::~Container()
{
	if (display && containerIMO.get())
	{
		display->use_background(nullptr);
		display->set_on_update_display(this, nullptr);
	}
}

void WeaponMixer::Container::reset_display()
{
	if (display && containerIMO.get())
	{
		display->use_background(nullptr);
		display->set_on_update_display(this, nullptr);
	}
}

WeaponMixer::Container::Container()
: weaponPartsForSetup(nullptr)
, showWeaponSetupInfo(nullptr)
{
	SET_EXTRA_DEBUG_INFO(weaponParts, TXT("WeaponMixer::Container.weaponParts"));
}

void WeaponMixer::Container::store_weapon_setup()
{
	if (auto* imo = containerIMO.get())
	{
		if (auto* wl = imo->get_custom<CustomModules::WeaponLocker>())
		{
			wl->store_weapon_setup();
		}
	}
}

bool WeaponMixer::Container::does_hold_weapon() const
{
	if (auto* imo = containerIMO.get())
	{
		if (auto* ih = imo->get_custom<CustomModules::ItemHolder>())
		{
			if (auto* held = ih->get_held())
			{
				return held->get_gameplay_as<ModuleEquipments::Gun>() != nullptr;
			}
		}
	}

	return false;
}

void WeaponMixer::Container::update(float _deltaTime)
{
	if (state == ContainerOff ||
		state == ContainerStart ||
		state == ContainerShutDown)
	{
	}
	else 
	{
		/*
		if (rerollHandler.get_switch_at() >= 1.0f)
		{
			bool isActive = true;
			for_count(int, i, 3)
			{
				if (weaponMixer->executionData->chosenReroll[i].get(-1) == containerIdx)
				{
					isActive = false;
				}
			}
			if (isActive)
			{
				weaponMixer->add_chosen_reroll(containerIdx);
				weaponMixer->issue_choose_unlocks();
			}
		}

		if (canister.should_give_content())
		{
			if (give_reward())
			{
				// off
				_depleted = true;
			}
		}

		update_reroll_emissive(true);
		*/
	}
}
