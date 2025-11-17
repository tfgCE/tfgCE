#include "aiLogic_permanentUpgradeMachine.h"

#include "utils\aiLogicUtil_deviceDisplayStart.h"

#include "..\..\game\game.h"
#include "..\..\library\exmType.h"
#include "..\..\library\lineModel.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\custom\mc_display.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

#ifdef AN_CLANG
#include "..\..\..\framework\library\usedLibraryStored.inl"
#endif

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
DEFINE_STATIC_NAME(energyKioskId);
DEFINE_STATIC_NAME(chooseUpgradeId);
DEFINE_STATIC_NAME(upgradeCanisterId);
DEFINE_STATIC_NAME(interactiveDeviceId);
DEFINE_STATIC_NAME(maxStorage); // energy kiosk
DEFINE_STATIC_NAME(energyInStorage); // energy kiosk
DEFINE_STATIC_NAME(initialisedByPermanentUpgradeMachine); // energy kiosk
DEFINE_STATIC_NAME(upgradesTagged);
DEFINE_STATIC_NAME(speakDeviceRequiresEnergyLine);
DEFINE_STATIC_NAME(speakDeviceReadyLine);

// ai messages
DEFINE_STATIC_NAME(forceEnergyKioskOff); // energy kiosk

// tags
DEFINE_STATIC_NAME(energyKiosk);
DEFINE_STATIC_NAME(permanentEXM);

// emissive layers
DEFINE_STATIC_NAME(ready);
DEFINE_STATIC_NAME(held);
DEFINE_STATIC_NAME(bad);

//

static void draw_line(REF_ float& drawLines, Colour const& _colour, float _size, Vector2 _a, Vector2 _b)
{
	float lineLength = (_b - _a).length() / (_size * 0.3f);

	if (lineLength > 0.0f && lineLength > drawLines)
	{
		_b = _a + (_b - _a) * (drawLines / lineLength);
		drawLines = 0.0f;
	}
	else
	{
		drawLines -= lineLength;
	}
	::System::Video3DPrimitives::line_2d(_colour, _a, _b);
}

//

REGISTER_FOR_FAST_CAST(PermanentUpgradeMachine);

PermanentUpgradeMachine::PermanentUpgradeMachine(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	permanentUpgradeMachineData = fast_cast<PermanentUpgradeMachineData>(_logicData);
}

PermanentUpgradeMachine::~PermanentUpgradeMachine()
{
	if (display && displayOwner.get())
	{
		display->use_background(nullptr);
		display->set_on_update_display(this, nullptr);
	}
}

void PermanentUpgradeMachine::setup_energy_kiosk(Framework::IModulesOwner* _pilgrimIMO)
{
	an_assert(_pilgrimIMO);
	auto* mp = _pilgrimIMO->get_gameplay_as<ModulePilgrim>();
	an_assert(mp);
	if (auto* ekIMO = energyKiosk.get())
	{
		MODULE_OWNER_LOCK_FOR_IMO(ekIMO, TXT("PermanentUpgradeMachine::setup_energy_kiosk"));
		Energy & e = ekIMO->access_variables().access<Energy>(NAME(energyInStorage));
		Energy & m = ekIMO->access_variables().access<Energy>(NAME(maxStorage));
		bool & initialised = ekIMO->access_variables().access<bool>(NAME(initialisedByPermanentUpgradeMachine));

		currentEnergyKioskPrice = GameplayBalance::permanent_upgrade_machine__cost_base();
		priceLevel = mp->get_pilgrim_inventory().permanentEXMs.get_size();
		priceLevel -= mp->get_pilgrim_inventory().get_count_of_permanent_EXMs_unlocked_in_persistence(); // if you unlocked in persistence, the price for consecutive will be lower
		currentEnergyKioskPrice += GameplayBalance::permanent_upgrade_machine__cost_step() * priceLevel;

		m = max(e, currentEnergyKioskPrice);
		if (!initialised)
		{
			e = GameplayBalance::permanent_upgrade_machine__base_energy();
			initialised = true;
		}
	}
}

bool PermanentUpgradeMachine::is_energy_kiosk_full()
{
	if (auto* ekIMO = energyKiosk.get())
	{
		MODULE_OWNER_LOCK_FOR_IMO(ekIMO, TXT("PermanentUpgradeMachine::is_energy_kiosk_full"));
		Energy e = ekIMO->get_variables().get_value<Energy>(NAME(energyInStorage), Energy::zero());
		Energy m = ekIMO->get_variables().get_value<Energy>(NAME(maxStorage), Energy::zero());
		if (!m.is_zero() && e >= m)
		{
			return true;
		}
	}
	return false;
}

void PermanentUpgradeMachine::use_energy_kiosk()
{
	if (auto* ekIMO = energyKiosk.get())
	{
		MODULE_OWNER_LOCK_FOR_IMO(ekIMO, TXT("PermanentUpgradeMachine::use_energy_kiosk"));
		auto& e = ekIMO->access_variables().access<Energy>(NAME(energyInStorage));
		e = max(Energy::zero(), e - currentEnergyKioskPrice);
	}
}

void PermanentUpgradeMachine::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	chooseUpgradeHandler.advance(_deltaTime);

	if (display && displaySetup)
	{
		{
			timeToRedraw -= _deltaTime;
			inStateTime += _deltaTime;
			if (timeToRedraw <= 0.0f)
			{
				redrawNow = true;
				timeToRedraw = 0.1f;
				if (state == ShowContent)
				{
					timeToRedraw = variousDrawingVariables.contentDrawn ? 0.3f : 0.02f;
				}
			}
		}
		State prevState = state;
		{
			if (state == Off)
			{
				currentPilgrim.clear();
			}
			if (currentPilgrim != presentPilgrim || !currentPilgrim.is_set())
			{
				if (state != Off && state != ShutDown)
				{
					state = ShutDown;
				}
			}
			if (currentPilgrim != presentPilgrim && !currentPilgrim.is_set() && presentPilgrim.is_set() && state == Off)
			{
				currentPilgrim = presentPilgrim;
				setup_energy_kiosk(currentPilgrim.get());
			}
			if (currentPilgrim.is_set() && state == Off)
			{
				currentPilgrim = presentPilgrim;

				updateAvailableUpgrades = true;
			}
			if (updateAvailableUpgrades)
			{
				availableUpgrades.clear();
				if (currentPilgrim.is_set())
				{
					updateAvailableUpgrades = false;

					TagCondition upgradesTagged = get_mind()->get_owner_as_modules_owner()->get_value<TagCondition>(NAME(upgradesTagged), permanentUpgradeMachineData->upgradesTagged);
					availableUpgrades = EXMType::get_all_exms(upgradesTagged);

					TagCondition gdRequirement;
					if (auto* gd = GameDefinition::get_chosen())
					{
						gdRequirement = gd->get_exms_tagged();
					}

					// drop non permanent
					{
						for (int i = 0; i < availableUpgrades.get_size(); ++i)
						{
							if (! gdRequirement.check(availableUpgrades[i]->get_tags()) ||
								!availableUpgrades[i]->is_permanent())
							{
								availableUpgrades.remove_at(i);
								--i;
							}
						}
					}

					if (ModulePilgrim* mp = currentPilgrim->get_gameplay_as<ModulePilgrim>())
					{
						auto& inventory = mp->access_pilgrim_inventory();
						Concurrency::ScopedMRSWLockRead lock(inventory.exmsLock);
						for (int i = 0; i < availableUpgrades.get_size(); ++i)
						{
							if (auto* aEXM = availableUpgrades[i])
							{
								bool isOk = true;
								if (isOk && aEXM->get_permanent_limit() > 0)
								{
									int existing = inventory.get_permanent_exm_count(aEXM->get_id());
									isOk = existing < aEXM->get_permanent_limit();
								}
								if (isOk)
								{
									for_every(pr, aEXM->get_permanent_prerequisites())
									{
										if (!inventory.has_permanent_exm(*pr))
										{
											isOk = false;
											break;
										}
									}
								}
								if (!isOk)
								{
									availableUpgrades.remove_at(i);
									--i;
									continue;
								}
							}
						}
					}
					if (state == Off)
					{
						state = Start;
					}
				}
				upgradeIdx = 0;
			}
			if (state != prevState)
			{
				inStateDrawnFrames = 0;
				inStateTime = 0.0f;
				redrawNow = true;
				prevState = state;
			}
		}
		{
			bool newForceEnergyKioskOff = ! presentPilgrim.is_set();
			if (newForceEnergyKioskOff ^ forceEnergyKioskOff)
			{
				forceEnergyKioskOff = newForceEnergyKioskOff;
				if (auto* ekIMO = energyKiosk.get())
				{
					if (auto* world = ekIMO->get_in_world())
					{
						if (auto* message = world->create_ai_message(NAME(forceEnergyKioskOff)))
						{
							message->to_ai_object(ekIMO);
							message->access_param(NAME(forceEnergyKioskOff)).access_as<bool>() = forceEnergyKioskOff;
						}
					}
				}
			}
		}
		// update selected exm to show and in canister
		{
			EXMType const* selectedEXM = nullptr;

			if (state == ShowContent)
			{
				auto& from = availableUpgrades;
				auto& idx = upgradeIdx;

				{
					int prevIdx = idx;

					idx = from.is_empty() ? 0 : mod(chooseUpgradeHandler.get_dial_at(), from.get_size());

					if (idx != prevIdx)
					{
						redrawNow = true;
					}
				}

				if (from.is_index_valid(idx))
				{
					selectedEXM = from[idx];
				}
			}

			{
				LineModel* contentLineModel = nullptr;
				LineModel* borderLineModel = nullptr;

				if (selectedEXM)
				{
					contentLineModel = selectedEXM->get_line_model_for_display();
					borderLineModel = permanentUpgradeMachineData->permanentEXMLineModel.get();
				}

				if (variousDrawingVariables.contentLineModel != contentLineModel ||
					variousDrawingVariables.borderLineModel != borderLineModel)
				{
					variousDrawingVariables.contentDrawLines = 1000.0f;
					variousDrawingVariables.contentDrawn = false;
					variousDrawingVariables.contentLineModel = contentLineModel;
					variousDrawingVariables.borderLineModel = borderLineModel;
					if (state == ShowContent)
					{
						redrawNow = true;
					}
				}
			}

			canister.content.exm = selectedEXM;
		}
		if (redrawNow)
		{
			++inStateDrawnFrames;
			variousDrawingVariables.contentDrawLines += 10.0f;
			variousDrawingVariables.energyRequiredInterval += 1;

			if (state == Start)
			{
				if (inStateTime > 0.5f)
				{
					state = EnergyRequired;
					timeToRedraw = 0.1f;
				}
			}
			else if (state == ShutDown)
			{
				int frames = 10;
				int frameNo = inStateDrawnFrames;

				timeToRedraw = 0.03f;

				variousDrawingVariables.shutdownAtPt = (float)frameNo / (float)frames;

				if (frameNo >= frames)
				{
					state = Off;
				}
			}
			else if (state == ShowContent)
			{
				if (! is_energy_kiosk_full())
				{
					state = EnergyRequired;
				}
			}

			if (state == EnergyRequired)
			{
				if (is_energy_kiosk_full())
				{
					state = ShowContent;

					todo_multiplayer_issue(TXT("which one should learn about this"));
					if (auto* imo = presentPilgrim.get())
					{
						if (auto* mp = imo->get_gameplay_as<ModulePilgrim>())
						{
							mp->access_overlay_info().speak(speakDeviceReadyLine.get());
						}
					}
				}
			}
		}
	}

	{
		canister.set_active(state == ShowContent);
		canister.advance(_deltaTime);

		if (state == EnergyRequired &&
			!canister.was_held())
		{
			if (auto* imo = canister.get_held_by_owner())
			{
				if (auto* mp = imo->get_gameplay_as<ModulePilgrim>())
				{
					mp->access_overlay_info().speak(speakDeviceRequiresEnergyLine.get());
				}
			}
		}

		if (state == ShowContent &&
			canister.should_give_content())
		{
			if (canister.give_content())
			{
				updateAvailableUpgrades = true;

#ifdef AN_ALLOW_EXTENSIVE_LOGS
				output(TXT("[PermanentUpgradeMachine] content given"));
#endif

				use_energy_kiosk();
				if (auto* g = Game::get())
				{
					if (!markedAsKnownForOpenWorldDirection)
					{
						if (auto* piow = PilgrimageInstanceOpenWorld::get())
						{
							piow->mark_pilgrimage_device_direction_known(get_imo());
							markedAsKnownForOpenWorldDirection = true;
						}
					}

					{
						Energy experience = GameplayBalance::permanent_upgrade_machine__xp_base();
						experience += GameplayBalance::permanent_upgrade_machine__xp_step() * priceLevel;
						if (experience.is_positive())
						{
							experience = experience.adjusted_plus_one(GameSettings::get().experienceModifier);
							PlayerSetup::access_current().stats__experience(experience);
							GameStats::get().add_experience(experience);
							Persistence::access_current().provide_experience(experience);
						}

						// this is to disallow unlocking to get xp
						if (auto* piow = PilgrimageInstanceOpenWorld::get())
						{
							Optional<VectorInt2> cellAt = piow->find_cell_at(get_mind()->get_owner_as_modules_owner());
							if (cellAt.is_set())
							{
								piow->set_last_visited_haven(cellAt.get());
#ifdef AN_ALLOW_EXTENSIVE_LOGS
								output(TXT("[PermanentUpgradeMachine] depleted at %ix%i"), cellAt.get().x, cellAt.get().y);
#endif
								piow->mark_pilgrim_device_state_depleted(cellAt.get(), get_imo());
							}
						}
						output(TXT("store game state - upgrade received"));
						Game::get_as<Game>()->add_async_store_game_state(true, GameStateLevel::Checkpoint);
					}

					// we do immediate sync world job as give_content just issues a sync world job, we will be added to the queue
					::SafePtr<::Framework::IModulesOwner> safeIMO;
					safeIMO = get_mind()->get_owner_as_modules_owner();
					g->add_immediate_sync_world_job(TXT("setup"), [this, safeIMO]()
					{
						if (safeIMO.is_set())
						{
							if (auto* p = currentPilgrim.get())
							{
								setup_energy_kiosk(p);
							}
						}
					});
				}
			}
		}
	}

	if (display && !displaySetup)
	{
		displaySetup = true;
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
				if (state != Start)
				{
					_display->add((new Framework::DisplayDrawCommands::CLS(Colour::black)));
				}
				_display->add((new Framework::DisplayDrawCommands::Border(Colour::black)));
				{
					auto* lm = variousDrawingVariables.contentLineModel;
					auto* blm = variousDrawingVariables.borderLineModel;
					float drawLines = variousDrawingVariables.contentDrawLines;
					_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
						[this, lm, blm, drawLines](Framework::Display* _display, ::System::Video3D* _v3d)
						{
							if (state == Start)
							{
								if (inStateDrawnFrames == 0) _display->add((new Framework::DisplayDrawCommands::CLS(Colour::black)));
								redrawNow = false;
								timeToRedraw = 0.06f;
								int frames = inStateDrawnFrames;
								_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
									[frames](Framework::Display* _display, ::System::Video3D* _v3d)
									{
										Utils::draw_device_display_start(_display, _v3d, frames);
									}));
							}
							else if (state == ShutDown)
							{
								Vector2 bl = _display->get_left_bottom_of_screen() + Vector2::half;
								Vector2 tr = _display->get_right_top_of_screen() - Vector2::half;

								float atY = tr.y + (bl.y - tr.y) * clamp(variousDrawingVariables.shutdownAtPt, 0.0f, 1.0f);

								Vector2 a(bl.x, atY);
								Vector2 b(tr.x, atY);
								_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
									[a, b](Framework::Display* _display, ::System::Video3D* _v3d)
									{
										::System::Video3DPrimitives::line_2d(_display->get_current_ink(), a, b);
									}));
							}
							else if (state == State::EnergyRequired)
							{
								Colour const ink = _display->get_current_ink();
								{
									int period = 20;
									variousDrawingVariables.energyRequiredInterval = variousDrawingVariables.energyRequiredInterval % period;
									if (variousDrawingVariables.energyRequiredInterval < period / 2)
									{
										Vector2 bl = _display->get_left_bottom_of_screen() + Vector2::half;
										Vector2 tr = _display->get_right_top_of_screen() - Vector2::half;
										bl.y = tr.y - (tr.x - bl.x); // draw using upper part

										Vector2 c = TypeConversions::Normal::f_i_cells((bl + tr) * 0.5f).to_vector2() + Vector2::half;

										Vector2 size(tr.x - c.x, tr.y - c.y);
										size.x = min(size.x, size.y);
										size.y = size.x;

										size = TypeConversions::Normal::f_i_closest(size * 2.0f).to_vector2();

										if (auto* lm = this->permanentUpgradeMachineData->energyRequiredLineModel.get())
										{
											for_every(l, lm->get_lines())
											{
												::System::Video3DPrimitives::line_2d(l->colour.get(ink), 
													TypeConversions::Normal::f_i_cells(c + size * l->a.to_vector2()).to_vector2() + Vector2::half,
													TypeConversions::Normal::f_i_cells(c + size * l->b.to_vector2()).to_vector2() + Vector2::half);
											}
										}
									}
								}
							}
							else if (state == State::ShowContent)
							{
								Colour const ink = _display->get_current_ink();
								{
									float linesLeft = drawLines;

									Vector2 bl = _display->get_left_bottom_of_screen() + Vector2::half;
									Vector2 tr = _display->get_right_top_of_screen() - Vector2::half;
									bl.y = tr.y - (tr.x - bl.x); // draw using upper part

									Vector2 c = TypeConversions::Normal::f_i_cells((bl + tr) * 0.5f).to_vector2() + Vector2::half;

									Vector2 size(tr.x - c.x, tr.y - c.y);
									size.x = min(size.x, size.y);
									size.y = size.x;

									size = TypeConversions::Normal::f_i_closest(size * 2.0f).to_vector2();

									if (blm)
									{
										for_every(l, blm->get_lines())
										{
											if (linesLeft <= 0.0f)
											{
												break;
											}
											draw_line(REF_ linesLeft, l->colour.get(ink), size.y,
												TypeConversions::Normal::f_i_cells(c + size * l->a.to_vector2()).to_vector2() + Vector2::half,
												TypeConversions::Normal::f_i_cells(c + size * l->b.to_vector2()).to_vector2() + Vector2::half);
										}

										size *= 0.8f;
									}
									if (lm)
									{
										for_every(l, lm->get_lines())
										{
											if (linesLeft <= 0.0f)
											{
												break;
											}
											draw_line(REF_ linesLeft, l->colour.get(ink), size.y,
												TypeConversions::Normal::f_i_cells(c + size * l->a.to_vector2()).to_vector2() + Vector2::half,
												TypeConversions::Normal::f_i_cells(c + size * l->b.to_vector2()).to_vector2() + Vector2::half);
										}
									}
									else
									{
										// draw anything for time being
										//::System::Video3DPrimitives::line_2d(ink, Vector2(bl.x, bl.y), Vector2(tr.x, tr.y));
										//::System::Video3DPrimitives::line_2d(ink, Vector2(bl.x, tr.y), Vector2(tr.x, bl.y));
									}
									if (linesLeft > 5.0f)
									{
										variousDrawingVariables.contentDrawn = true;
									}
								}
							}
						}));
				}
			});
	}

}

void PermanentUpgradeMachine::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);
}

LATENT_FUNCTION(PermanentUpgradeMachine::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai permanent upgrade machine] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<PermanentUpgradeMachine>(logic);

	LATENT_BEGIN_CODE();

	{
		while (!self->display)
		{
			if (auto* cmd = imo->get_custom<Framework::CustomModules::Display>())
			{
				self->display = cmd->get_display();
				self->displayOwner = imo;
				self->redrawNow = true;
			}

			self->chooseUpgradeHandler.initialise(imo, NAME(chooseUpgradeId));
			self->canister.initialise(imo, NAME(upgradeCanisterId));

			self->energyKiosk.clear();
			if (auto* id = imo->get_variables().get_existing<int>(NAME(energyKioskId)))
			{
				imo->get_in_world()->for_every_object_with_id(hardcoded NAME(energyKioskId), *id,
					[self, imo](Framework::Object* _object)
					{
						if (_object != imo &&
							_object->get_tags().get_tag(NAME(energyKiosk)))
						{
							self->energyKiosk = _object;
							return true;
						}
						return false; // keep going on
					});
			}

			LATENT_WAIT(Random::get_float(0.2f, 0.5f));
		}
	}

	{
		self->speakDeviceRequiresEnergyLine.set_name(imo->get_variables().get_value<Framework::LibraryName>(NAME(speakDeviceRequiresEnergyLine), Framework::LibraryName::invalid()));
		self->speakDeviceReadyLine.set_name(imo->get_variables().get_value<Framework::LibraryName>(NAME(speakDeviceReadyLine), Framework::LibraryName::invalid()));
		if (auto* lib = Framework::Library::get_current())
		{
			self->speakDeviceRequiresEnergyLine.find(lib);
			self->speakDeviceReadyLine.find(lib);
		}
	}

	while (true)
	{
		{
			todo_multiplayer_issue(TXT("we just get player here, we should get all players?"));
			ModulePilgrim* mp = nullptr;
			if (auto* g = Game::get_as<Game>())
			{
				if (auto* pa = g->access_player().get_actor())
				{
					if (pa->get_presence() &&
						pa->get_presence()->get_in_room() == imo->get_presence()->get_in_room())
					{
						mp = pa->get_gameplay_as<ModulePilgrim>();
					}
				}
			}

			self->presentPilgrim = mp? mp->get_owner() : nullptr;

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

REGISTER_FOR_FAST_CAST(PermanentUpgradeMachineData);

bool PermanentUpgradeMachineData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	selectColour.load_from_xml_child_or_attr(_node, TXT("selectColour"));

	upgradesTagged.load_from_xml_attribute_or_child_node(_node, TXT("upgradesTagged"));

	permanentEXMLineModel.load_from_xml(_node, TXT("permanentEXMLineModel"), _lc);
	energyRequiredLineModel.load_from_xml(_node, TXT("energyRequiredLineModel"), _lc);

	return result;
}

bool PermanentUpgradeMachineData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= permanentEXMLineModel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= energyRequiredLineModel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}
