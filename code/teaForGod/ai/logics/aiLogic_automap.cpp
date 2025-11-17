#include "aiLogic_automap.h"

#include "..\..\game\game.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_hitIndicator.h"
#include "..\..\pilgrimage\pilgrimage.h"
#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\gameScript\gameScriptExecution.h"
#include "..\..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\custom\mc_display.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\world\world.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// variables
DEFINE_STATIC_NAME(interactiveDeviceId);

// emissive layers
DEFINE_STATIC_NAME(pulse);

// sounds
DEFINE_STATIC_NAME(automapActivated);

// game script traps
DEFINE_STATIC_NAME_STR(gstAutomapTriggered, TXT("automap triggered"));

//

REGISTER_FOR_FAST_CAST(Automap);

Automap::Automap(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	automapData = fast_cast<AutomapData>(_logicData);
}

Automap::~Automap()
{
	if (display)
	{
		display->use_background(nullptr);
		display->set_on_update_display(this, nullptr);
	}
}

void Automap::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	{
		bool newDrawMap = true;

		{
			todo_multiplayer_issue(TXT("we just get player here"));
			if (auto* g = Game::get_as<Game>())
			{
				if (auto* pa = g->access_player().get_actor())
				{
					if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
					{
						auto& poi = mp->access_overlay_info();
						if (poi.is_trying_to_show_map())
						{
							newDrawMap = false;
						}
					}
				}
			}
		}

		if (drawMap != newDrawMap)
		{
			timeToRedraw = 0.0f;
		}
		drawMap = newDrawMap;
	}

	timeToRedraw -= _deltaTime;
	if (timeToRedraw <= 0.0f)
	{
		timeToRedraw = Random::get_float(0.5f, 2.0f);
		redrawNow = true;
	}

	if (! display)
	{
		if (auto* mind = get_mind())
		{
			if (auto* imo = mind->get_owner_as_modules_owner())
			{
				if (auto* displayModule = imo->get_custom<::Framework::CustomModules::Display>())
				{
					display = displayModule->get_display();

					if (display)
					{
						display->draw_all_commands_immediately();
						display->set_on_update_display(this,
						[this](Framework::Display* _display)
						{
							if (!redrawNow)
							{
								return;
							}
							redrawNow = false;

							_display->drop_all_draw_commands();
							_display->add((new Framework::DisplayDrawCommands::CLS(Colour::black)));
							_display->add((new Framework::DisplayDrawCommands::Border(Colour::black)));

							if (drawMap)
							{
								_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
								[](Framework::Display* _display, ::System::Video3D* _v3d)
								{
									Colour const ink = _display->get_current_ink();

									Vector2 bl = _display->get_left_bottom_of_screen();
									Vector2 tr = _display->get_right_top_of_screen();

									float chanceOfExit = 0.85f;
									int count = Random::get_int_from_range(1, 8);
									ARRAY_STACK(Optional<float>, downAtX, count);
									Vector2 size = (tr - bl) / (float)count;
									for_count(int, x, count)
									{
										downAtX.push_back(Random::get_chance(chanceOfExit)? Optional<float>(Random::get_float(-0.4f, 0.4f)) : NP);
									}
									for_count(int, y, count)
									{
										Optional<float> leftAtY = Random::get_chance(chanceOfExit) ? Optional<float>(Random::get_float(-0.4f, 0.4f)) : NP;
										for_count(int, x, count)
										{
											Vector2 c = bl + size * 0.5f;
											c.x += size.x * (float)x;
											c.y += size.y * (float)y;

											c = round(c);

											Vector2 atPt(Random::get_float(-0.35f, 0.35f), Random::get_float(-0.35f, 0.35f));
											float ds = Random::get_float(0.05f, 0.45f);
											ds = min(ds, max(0.05f, min(0.42f - abs(atPt.x), 0.42f - abs(atPt.y))));
											Vector2 ac = c + atPt * size;
											ac = round(ac);
											Vector2 l = ac;
											Vector2 u = ac;
											Vector2 d = ac;
											Vector2 r = ac;
											float dssi = round(ds * size.x);
											u.y += dssi;
											r.x += dssi;
											d.y -= dssi;
											l.x -= dssi;
											Vector2 bl = ac;
											Vector2 bu = ac;
											Vector2 bd = ac;
											Vector2 br = ac;
											bu.y = c.y + size.y * 0.5f;
											br.x = c.x + size.x * 0.5f;
											bd.y = c.y - size.y * 0.5f;
											bl.x = c.x - size.x * 0.5f;
											::System::Video3DPrimitives::line_2d(ink, u, r);
											::System::Video3DPrimitives::line_2d(ink, r, d);
											::System::Video3DPrimitives::line_2d(ink, d, l);
											::System::Video3DPrimitives::line_2d(ink, l, u);

											if (downAtX[x].is_set())
											{
												::System::Video3DPrimitives::line_2d(ink, d, bd);
												if (y > 0)
												{
													::System::Video3DPrimitives::line_2d(ink, bd, Vector2(downAtX[x].get(), bd.y));
												}
											}
											if (leftAtY.is_set())
											{
												::System::Video3DPrimitives::line_2d(ink, l, bl);
												if (x > 0)
												{
													::System::Video3DPrimitives::line_2d(ink, bl, Vector2(bl.x, leftAtY.get()));
												}
											}
											leftAtY.clear();
											downAtX[x].clear();
											if (Random::get_chance(chanceOfExit))
											{
												downAtX[x] = bu.x;
												::System::Video3DPrimitives::line_2d(ink, u, bu);
											}
											if (Random::get_chance(chanceOfExit))
											{
												leftAtY = br.y;
												::System::Video3DPrimitives::line_2d(ink, r, br);
											}
										}
									}
								}));
							}
						});
					}
				}
			}
		}
	}
}

void Automap::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);
}

void Automap::trigger_pilgrim(bool _justMap)
{
	todo_multiplayer_issue(TXT("we just get player here"));
	if (auto* g = Game::get_as<Game>())
	{
		if (auto* pa = g->access_player().get_actor())
		{
			if (!_justMap)
			{
				bool doEffects = true;
				if (auto* piow = PilgrimageInstanceOpenWorld::get())
				{
					doEffects = !piow->get_special_rules().silentAutomaps;
				}
				if (doEffects)
				{
					if (auto* ha = pa->get_custom<CustomModules::HitIndicator>())
					{
						Framework::RelativeToPresencePlacement r2pp;
						r2pp.be_temporary_snapshot();
						if (r2pp.find_path(pa, button.get()))
						{
							Vector3 buttonHereWS = r2pp.location_from_target_to_owner(button->get_presence()->get_centre_of_presence_WS());
							ha->indicate_location(0.0f, buttonHereWS, CustomModules::HitIndicator::IndicateParams()
								.with_colour_override(automapData->hitIndicatorColour).not_damage());
						}
					}
					if (auto* s = pa->get_sound())
					{
						s->play_sound(NAME(automapActivated));
					}
					if (Framework::GameUtils::is_local_player(pa))
					{
						PhysicalSensations::OngoingSensation s(PhysicalSensations::OngoingSensation::UpgradeReceived, 0.5f, false);
						PhysicalSensations::start_sensation(s);
					}
				}
			}
			if (!markedAsKnownForOpenWorldDirection)
			{
				if (auto* piow = PilgrimageInstanceOpenWorld::get())
				{
					piow->mark_pilgrimage_device_direction_known(get_imo());
					markedAsKnownForOpenWorldDirection = true;
				}
			}
			if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
			{
				auto& poi = mp->access_overlay_info();
				Optional<VectorInt2> showLocation;
				if (GameSettings::get().difficulty.autoMapOnly)
				{
					showLocation = mapLoc;
				}
				poi.new_map_available(true, ! _justMap, showLocation);
			}
		}
	}
}

void Automap::update_emissive(bool _available)
{
	ai_log(this, TXT("update emissive [%S]"), _available? TXT("available") : TXT("depleted"));

	if (button.get())
	{
		if (auto* ec = button->get_custom<CustomModules::EmissiveControl>())
		{
			if (_available)
			{
				{
					auto& layer = ec->emissive_access(NAME(pulse));
					layer.set_colour(automapData->buttonColour);
				}

				ec->emissive_activate(NAME(pulse));
			}
			else
			{
				ec->emissive_deactivate(NAME(pulse));
			}
		}
	}
}

LATENT_FUNCTION(Automap::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai automap] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(bool, depleted);

	LATENT_VAR(bool, buttonPressed);
	LATENT_VAR(Optional<VectorInt2>, cellAt);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Automap>(logic);

	LATENT_BEGIN_CODE();

	buttonPressed = false;
	depleted = false;

	ai_log(self, TXT("automap, hello!"));

	if (imo)
	{
		if (auto* piow = PilgrimageInstanceOpenWorld::get())
		{
			cellAt = piow->find_cell_at(imo);
			if (cellAt.is_set())
			{
				depleted = piow->is_pilgrim_device_state_depleted(cellAt.get(), imo);
				ai_log(self, depleted? TXT("depleted") : TXT("available"));
				if (!depleted)
				{
					// this will issue a world job to make sure these exist
					int radius = max(self->automapData->longRangeRevealExitsCellRadius, self->automapData->longRangeRevealDevicesCellRadius);
					radius = max(radius, self->automapData->detectLongDistanceDetectableDevicesRadius);
#ifdef AN_ALLOW_EXTENSIVE_LOGS
					output(TXT("automap requires to make sure cells are ready at %ix%i, %i"), cellAt.get().x, cellAt.get().y, radius);
#endif
					piow->make_sure_cells_are_ready(cellAt.get(), radius);
				}
				self->mapLoc = cellAt;
			}
		}

		ai_log(self, TXT("get button"));

#ifdef AN_USE_AI_LOG
		if (auto* id = imo->get_variables().get_existing<int>(NAME(interactiveDeviceId)))
		{
			ai_log(self, TXT("id = %i"), *id);
		}
#endif

		while (!self->button.get())
		{
			if (auto* id = imo->get_variables().get_existing<int>(NAME(interactiveDeviceId)))
			{
				ai_log(self, TXT("id = %i"), *id);
				auto* world = imo->get_in_world();
				an_assert(world);

				world->for_every_object_with_id(NAME(interactiveDeviceId), *id,
					[self, imo](Framework::Object* _object)
					{
						if (_object != imo)
						{
							self->button = _object;
							return true; // one is enough
						}
						return false; // keep going on
					});
			}
			LATENT_WAIT(Random::get_float(0.2f, 0.5f));
		}

		ai_log(self, TXT("button accessed"));
	}

	self->update_emissive(! depleted);

	while (true)
	{
		{
			bool newButtonPressed = false;
			if (self->button.get())
			{
				if (auto* mms = fast_cast<Framework::ModuleMovementSwitch>(self->button->get_movement()))
				{
					if (mms->is_active_at(1))
					{
						newButtonPressed = true;
					}
				}
			
				if (newButtonPressed && !buttonPressed)
				{
					if (auto* piow = PilgrimageInstanceOpenWorld::get())
					{
						int detectLongDistanceDetectableDevicesAmount = self->automapData->detectLongDistanceDetectableDevicesAmount;
						int detectLongDistanceDetectableDevicesRadius = self->automapData->detectLongDistanceDetectableDevicesRadius;

						if (auto* p = piow->get_pilgrimage())
						{
							if (auto* owdef = p->open_world__get_definition())
							{
								detectLongDistanceDetectableDevicesAmount = owdef->get_special_rules().detectLongDistanceDetectableDevicesAmount.get(detectLongDistanceDetectableDevicesAmount);
								detectLongDistanceDetectableDevicesRadius = owdef->get_special_rules().detectLongDistanceDetectableDevicesRadius.get(detectLongDistanceDetectableDevicesRadius);
							}
						}

						piow->add_long_distance_detection(cellAt.get(), detectLongDistanceDetectableDevicesRadius, detectLongDistanceDetectableDevicesAmount);
					}

					if (!depleted)
					{
						ai_log(self, TXT("activate automap and deplete"));
						if (cellAt.is_set())
						{
							if (auto* piow = PilgrimageInstanceOpenWorld::get())
							{
								int revealCellRadius = self->automapData->revealExitsCellRadius;
								int revealExitsCellRadius = self->automapData->revealExitsCellRadius;
								int revealDevicesCellRadius = self->automapData->revealDevicesCellRadius;
								
								int autoMapOnlyRevealCellRadius = self->automapData->autoMapOnlyRevealCellRadius;
								int autoMapOnlyRevealExitsCellRadius = self->automapData->autoMapOnlyRevealExitsCellRadius;
								int autoMapOnlyRevealDevicesCellRadius = self->automapData->autoMapOnlyRevealDevicesCellRadius;
								{
									todo_multiplayer_issue(TXT("single player assumption"));
									ModulePilgrim* mp = nullptr;
									if (auto* g = Game::get_as<Game>())
									{
										if (auto* pa = g->access_player().get_actor())
										{
											mp = pa->get_gameplay_as<ModulePilgrim>();
											if (mp->has_exm_equipped(EXMID::Passive::long_range_automap()))
											{
												revealCellRadius = self->automapData->longRangeRevealCellRadius;
												revealExitsCellRadius = self->automapData->longRangeRevealExitsCellRadius;
												revealDevicesCellRadius = self->automapData->longRangeRevealDevicesCellRadius;
												autoMapOnlyRevealExitsCellRadius = self->automapData->autoMapOnlyLongRangeRevealExitsCellRadius;
												autoMapOnlyRevealCellRadius = self->automapData->autoMapOnlyLongRangeRevealCellRadius;
												autoMapOnlyRevealDevicesCellRadius = self->automapData->autoMapOnlyLongRangeRevealDevicesCellRadius;
											}
										}
									}
								}
								if (GameSettings::get().difficulty.autoMapOnly)
								{
									// always, even if 0
									piow->mark_cell_known(cellAt.get(), autoMapOnlyRevealCellRadius);
									if (autoMapOnlyRevealExitsCellRadius >= 0)
									{
										piow->mark_cell_known_exits(cellAt.get(), autoMapOnlyRevealExitsCellRadius);
									}
									if (autoMapOnlyRevealDevicesCellRadius >= 0)
									{
										piow->mark_cell_known_devices(cellAt.get(), autoMapOnlyRevealDevicesCellRadius);
									}
								}
								else
								{
									if (revealCellRadius >= 0)
									{
										piow->mark_cell_known(cellAt.get(), revealCellRadius);
									}
									if (revealExitsCellRadius >= 0)
									{
										piow->mark_cell_known_exits(cellAt.get(), revealExitsCellRadius);
									}
									if (revealDevicesCellRadius >= 0)
									{
										piow->mark_cell_known_devices(cellAt.get(), revealDevicesCellRadius);
									}
								}
								piow->mark_pilgrim_device_state_depleted(cellAt.get(), imo);
							}
							self->trigger_pilgrim();
							{
								Energy experience = GameplayBalance::automap__xp();
								if (experience.is_positive())
								{
									experience = experience.adjusted_plus_one(GameSettings::get().experienceModifier);
									PlayerSetup::access_current().stats__experience(experience);
									GameStats::get().add_experience(experience);
									Persistence::access_current().provide_experience(experience);
								}
							}
						}
						depleted = true;
						self->update_emissive(false);
					}
					else
					{
						self->trigger_pilgrim(true);
					}
					Framework::GameScript::ScriptExecution::trigger_execution_trap(NAME(gstAutomapTriggered));
				}
			}

			buttonPressed = newButtonPressed;
		}
		LATENT_WAIT(Random::get_float(0.1f, 0.2f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(AutomapData);

bool AutomapData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	revealCellRadius = _node->get_int_attribute_or_from_child(TXT("revealCellRadius"), revealCellRadius);
	revealExitsCellRadius = _node->get_int_attribute_or_from_child(TXT("revealExitsCellRadius"), revealExitsCellRadius);
	revealDevicesCellRadius = _node->get_int_attribute_or_from_child(TXT("revealDevicesCellRadius"), revealDevicesCellRadius);

	longRangeRevealCellRadius = _node->get_int_attribute_or_from_child(TXT("longRangeRevealCellRadius"), longRangeRevealCellRadius);
	longRangeRevealExitsCellRadius = _node->get_int_attribute_or_from_child(TXT("longRangeRevealExitsCellRadius"), longRangeRevealExitsCellRadius);
	longRangeRevealDevicesCellRadius = _node->get_int_attribute_or_from_child(TXT("longRangeRevealDevicesCellRadius"), longRangeRevealDevicesCellRadius);

	autoMapOnlyRevealCellRadius = _node->get_int_attribute_or_from_child(TXT("autoMapOnlyRevealCellRadius"), autoMapOnlyRevealCellRadius);
	autoMapOnlyRevealExitsCellRadius = _node->get_int_attribute_or_from_child(TXT("autoMapOnlyRevealExitsCellRadius"), autoMapOnlyRevealExitsCellRadius);
	autoMapOnlyRevealDevicesCellRadius = _node->get_int_attribute_or_from_child(TXT("autoMapOnlyRevealDevicesCellRadius"), autoMapOnlyRevealDevicesCellRadius);

	autoMapOnlyLongRangeRevealCellRadius = _node->get_int_attribute_or_from_child(TXT("autoMapOnlyLongRangeRevealCellRadius"), autoMapOnlyLongRangeRevealCellRadius);
	autoMapOnlyLongRangeRevealExitsCellRadius = _node->get_int_attribute_or_from_child(TXT("autoMapOnlyLongRangeRevealExitsCellRadius"), autoMapOnlyLongRangeRevealExitsCellRadius);
	autoMapOnlyLongRangeRevealDevicesCellRadius = _node->get_int_attribute_or_from_child(TXT("autoMapOnlyLongRangeRevealDevicesCellRadius"), autoMapOnlyLongRangeRevealDevicesCellRadius);

	detectLongDistanceDetectableDevicesAmount = _node->get_int_attribute_or_from_child(TXT("detectLongDistanceDetectableDevicesAmount"), detectLongDistanceDetectableDevicesAmount);
	detectLongDistanceDetectableDevicesRadius = _node->get_int_attribute_or_from_child(TXT("detectLongDistanceDetectableDevicesRadius"), detectLongDistanceDetectableDevicesRadius);

	buttonColour.load_from_xml_child_node(_node, TXT("buttonColour"));
	hitIndicatorColour.load_from_xml_child_node(_node, TXT("hitIndicatorColour"));

	return result;
}

bool AutomapData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
