#include "aiLogic_launchChamber.h"

#include "..\..\game\game.h"
#include "..\..\game\playerSetup.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\gameplay\moduleEnergyQuantum.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\game\delayedObjectCreation.h"
#include "..\..\..\framework\gameScript\gameScriptExecution.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\module\custom\mc_display.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\world\room.h"
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
DEFINE_STATIC_NAME(initialState);
DEFINE_STATIC_NAME(minStateRequired);
DEFINE_STATIC_NAME(speedPerMinute);
DEFINE_STATIC_NAME(worksFor);
DEFINE_STATIC_NAME(ammoEnergyQuantumType);
DEFINE_STATIC_NAME(healthEnergyQuantumType);

// variables
DEFINE_STATIC_NAME(interactiveDeviceId);

// emissive layers for buttons
DEFINE_STATIC_NAME(ready);
DEFINE_STATIC_NAME(bad);
DEFINE_STATIC_NAME_STR(badPulse, TXT("bad pulse"));
DEFINE_STATIC_NAME(operating);
DEFINE_STATIC_NAME(busy);

// elements
DEFINE_STATIC_NAME(display);
DEFINE_STATIC_NAME_STR(mainControl, TXT("main control"));
DEFINE_STATIC_NAME(generator);

// ai message
DEFINE_STATIC_NAME_STR(aim_broadcast, TXT("launch chamber; broadcast")); // to notify displays
DEFINE_STATIC_NAME_STR(aim_generatorOff, TXT("launch chamber; generator off"));
DEFINE_STATIC_NAME_STR(aim_generatorOn, TXT("launch chamber; generator on"));

// ai messages to control via script
DEFINE_STATIC_NAME_STR(aim_drain, TXT("launch chamber; drain"));
DEFINE_STATIC_NAME_STR(aim_spawnHealth, TXT("launch chamber; spawn health"));
DEFINE_STATIC_NAME_STR(aim_spawnAmmo, TXT("launch chamber; spawn ammo"));

// ai message params
DEFINE_STATIC_NAME(energy);
DEFINE_STATIC_NAME(perMinute);

// temporary objects
DEFINE_STATIC_NAME(sparks);

// sounds
DEFINE_STATIC_NAME(up);
DEFINE_STATIC_NAME(down);
DEFINE_STATIC_NAME(on);
DEFINE_STATIC_NAME(drain);

//

REGISTER_FOR_FAST_CAST(LaunchChamber);

LaunchChamber::LaunchChamber(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	launchChamberData = fast_cast<LaunchChamberData>(_logicData);

	element = launchChamberData->element;
}

LaunchChamber::~LaunchChamber()
{
	if (display.display)
	{
		display.display->use_background(nullptr);
		display.display->set_on_update_display(this, nullptr);
	}
}

void LaunchChamber::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	mainControl.initialState = _parameters.get_value<float>(NAME(initialState), mainControl.initialState);
	mainControl.minStateRequired = _parameters.get_value<float>(NAME(minStateRequired), mainControl.minStateRequired);
	mainControl.speedPerMinute = _parameters.get_value<float>(NAME(speedPerMinute), mainControl.speedPerMinute);

	generator.worksFor = _parameters.get_value<Range>(NAME(worksFor), generator.worksFor);

	{
		Framework::LibraryName sit = _parameters.get_value<Framework::LibraryName>(NAME(ammoEnergyQuantumType), Framework::LibraryName::invalid());
		if (sit.is_valid())
		{
			mainControl.ammoEnergyQuantumType.set_name(Framework::LibraryName(sit));
			mainControl.ammoEnergyQuantumType.find(Framework::Library::get_current());
		}
	}
	{
		Framework::LibraryName sit = _parameters.get_value<Framework::LibraryName>(NAME(healthEnergyQuantumType), Framework::LibraryName::invalid());
		if (sit.is_valid())
		{
			mainControl.healthEnergyQuantumType.set_name(Framework::LibraryName(sit));
			mainControl.healthEnergyQuantumType.find(Framework::Library::get_current());
		}
	}
}

void LaunchChamber::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (!readyAndRunning)
	{
		return;
	}

	if (element == NAME(mainControl))
	{
		bool wasWorking = mainControl.working;
		mainControl.working = false;
		if (mainControl.drainPerMinute > 0.0f)
		{
			mainControl.state -= mainControl.drainPerMinute * _deltaTime / 60.0f;
			if (mainControl.state < 0.0f)
			{
				mainControl.state = 0.0f;
				mainControl.drainPerMinute = 0.0f;
			}
		}
		else if (mainControl.workingGeneratorsCount > 0 &&
				 mainControl.workingGeneratorsCount == mainControl.generators.get_size())
		{
			if (!mainControl.wasEverWorking)
			{
				mainControl.wasEverWorking = true;
				if (launchChamberData->triggerGameScriptTrapOnWorkingFirstTime.is_valid())
				{
					Framework::GameScript::ScriptExecution::trigger_execution_trap(launchChamberData->triggerGameScriptTrapOnWorkingFirstTime);
				}
			}
			mainControl.working = true;

			float prevState = mainControl.state;
			mainControl.state += mainControl.speedPerMinute * _deltaTime / 60.0f;

			if (prevState < mainControl.minStateRequired &&
				mainControl.state >= mainControl.minStateRequired)
			{
				if (launchChamberData->triggerGameScriptTrapOnMinStateReached.is_valid())
				{
					Framework::GameScript::ScriptExecution::trigger_execution_trap(launchChamberData->triggerGameScriptTrapOnMinStateReached);
				}
			}
		}

		if (wasWorking != mainControl.working)
		{
			if (mainControl.working)
			{
				if (launchChamberData->triggerGameScriptTrapOnGeneratorWorking.is_valid())
				{
					Framework::GameScript::ScriptExecution::trigger_execution_trap(launchChamberData->triggerGameScriptTrapOnGeneratorWorking);
				}
			}
			else
			{
				if (launchChamberData->triggerGameScriptTrapOnGeneratorNotWorking.is_valid())
				{
					Framework::GameScript::ScriptExecution::trigger_execution_trap(launchChamberData->triggerGameScriptTrapOnGeneratorNotWorking);
				}
			}
		}
		mainControl.state = clamp(mainControl.state, 0.0f, 1.0f);
	}

	if (element == NAME(generator))
	{
		struct ApplyEmissive
		{
			static void to(Name const& _emissive, Array<SafePtr<Framework::IModulesOwner>> const& imos)
			{
				for_every_ref(imo, imos)
				{
					if (auto* e = imo->get_custom<CustomModules::EmissiveControl>())
					{
						e->emissive_deactivate_all();
						if (_emissive.is_valid())
						{
							e->emissive_activate(_emissive);
						}
					}
				}
			}
		};

		generator.buttonHandler.advance(_deltaTime);
		if (generator.workingTimeLeft > 0.0f)
		{
			generator.workingTimeLeft -= _deltaTime;
			if (generator.workingTimeLeft <= 0.0f)
			{
				generator.workingTimeLeft = 0.0f;
				generator.updateEmissiveRequired = true;

				auto* imo = get_mind()->get_owner_as_modules_owner();
				if (auto* message = imo->get_presence()->get_in_world()->create_ai_message(NAME(aim_generatorOff)))
				{
					message->to_room(imo->get_presence()->get_in_room());
					message->access_param(NAME(generator)).access_as< SafePtr < Framework::IModulesOwner>>() = imo;
				}

				if (auto* t = imo->get_temporary_objects())
				{
					t->spawn_all(NAME(sparks));
				}

				if (auto* s = imo->get_sound())
				{
					s->play_sound(NAME(down));
					s->stop_sound(NAME(on));
				}
			}
		}
		else
		{
			if (generator.buttonHandler.is_triggered())
			{
				generator.workingTimeLeft = rg.get(generator.worksFor);
				generator.updateEmissiveRequired = true;

				auto* imo = get_mind()->get_owner_as_modules_owner();
				if (auto* message = imo->get_presence()->get_in_world()->create_ai_message(NAME(aim_generatorOn)))
				{
					message->to_room(imo->get_presence()->get_in_room());
					message->access_param(NAME(generator)).access_as< SafePtr < Framework::IModulesOwner>>() = imo;
				}

				if (auto* s = imo->get_sound())
				{
					s->play_sound(NAME(up));
					s->play_sound(NAME(on));
				}
			}
		}

		if (generator.updateEmissiveRequired)
		{
			generator.updateEmissiveRequired = false;

			if (generator.workingTimeLeft <= 0.0f)
			{
				ApplyEmissive::to(NAME(badPulse), generator.buttonHandler.get_buttons());
			}
			else
			{
				ApplyEmissive::to(NAME(busy), generator.buttonHandler.get_buttons());
			}
		}
	}

	if (element == NAME(display))
	{
		if (auto* mcImo = display.mainControl.get())
		{
			auto* ai = mcImo->get_ai();
			if (auto* mind = ai->get_mind())
			{
				if (auto* mcLogic = fast_cast<LaunchChamber>(mind->get_logic()))
				{
					// read only
					display.currentState = mcLogic->mainControl.state;
					display.currentWorking = mcLogic->mainControl.working;
					display.minStateRequired = mcLogic->mainControl.minStateRequired;
				}
			}
		}
		if (display.currentWorking)
		{
			display.flashTimeLeft -= _deltaTime;
			if (display.flashTimeLeft < 0.0f)
			{
				display.currentFlash = ! display.currentFlash.get(false);
				if (display.currentFlash.get())
				{
					display.flashTimeLeft = 0.2f;
				}
				else
				{
					display.flashTimeLeft = 0.2f;
				}
			}
		}
		else
		{
			display.flashTimeLeft = 0.0f;
			display.currentFlash.clear();
		}

		if (!display.display)
		{
			if (auto* mind = get_mind())
			{
				if (auto* imo = mind->get_owner_as_modules_owner())
				{
					if (auto* displayModule = imo->get_custom<::Framework::CustomModules::Display>())
					{
						display.display = displayModule->get_display();

						if (display.display)
						{
							display.display->set_on_update_display(this,
								[this](Framework::Display* _display)
								{
									if (display.displayedState == display.currentState &&
										display.displayedFlash == display.currentFlash)
									{
										return;
									}
									display.displayedState = display.currentState;
									display.displayedFlash = display.currentFlash;

									_display->drop_all_draw_commands();
									_display->add((new Framework::DisplayDrawCommands::CLS(Colour::black)));
									_display->add((new Framework::DisplayDrawCommands::Border(Colour::black)));

									{
										float pt = display.currentState;
										Colour ink = !display.currentWorking && display.currentState < display.minStateRequired ? Colour::red : Colour::white;
										if (display.currentState < display.minStateRequired)
										{
											ink = ink.mul_rgb(0.5f);
										}
										Colour reqInk = display.currentState < display.minStateRequired ? Colour::red : Colour::white;
										float reqPt = display.minStateRequired;
										bool flashDown = display.displayedFlash.is_set() && !display.displayedFlash.get(false);
										_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
											[pt, reqPt, ink, reqInk, flashDown](Framework::Display* _display, ::System::Video3D* _v3d)
											{
												Vector2 bl = _display->get_left_bottom_of_screen();
												Vector2 tr = _display->get_right_top_of_screen();

												{
													float y = round(lerp(reqPt, bl.y, tr.y));
													Vector2 l[4];
													l[0] = bl;
													l[1] = Vector2(bl.x, y);
													l[2] = Vector2(tr.x, y);
													l[3] = Vector2(tr.x, bl.y);
													::System::Video3DPrimitives::line_strip_2d(reqInk, l, 4);
												}

												{
													float ly = round(lerp(reqPt, bl.y, tr.y)) + 2.0f;
													float ty = round(lerp(1.0f, bl.y, tr.y));
													Vector2 l[4];
													l[0] = Vector2(bl.x, ly);
													l[1] = Vector2(bl.x, ty);
													l[2] = Vector2(tr.x, ty);
													l[3] = Vector2(tr.x, ly);
													::System::Video3DPrimitives::line_strip_2d(reqInk, l, 4);
												}

												{
													Range2 r = Range2::empty;
													float y = round(lerp(pt, bl.y, tr.y));
													if (flashDown)
													{
														y -= 2.0f;
														if (y < 0.0f)
														{
															return;
														}
													}
													r.include(Vector2(bl.x + 3.0f, bl.y));
													r.include(Vector2(tr.x - 3.0f, y));
													::System::Video3DPrimitives::fill_rect_2d(ink, r);
												}

											}));
									}

									_display->draw_all_commands_immediately();
								});
						}
					}
				}
			}
		}
	}
}

void LaunchChamber::set_generator(Framework::IModulesOwner* _genImo, bool _working)
{
	bool found = false;
	for_every(g, mainControl.generators)
	{
		if (g->generator == _genImo)
		{
			g->isOn = _working;
			found = true;
			break;
		}
	}

	if (!found)
	{
		MainControl::Generator g;
		g.generator = _genImo;
		g.isOn = _working;
		mainControl.generators.push_back(g);
	}

	mainControl.workingGeneratorsCount = 0;
	for_every(g, mainControl.generators)
	{
		if (g->isOn)
		{
			++ mainControl.workingGeneratorsCount;
		}
	}
}

LATENT_FUNCTION(LaunchChamber::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[launch chamber] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

#ifdef AUTO_TEST
	LATENT_VAR(::System::TimeStamp, autoTestTS);
#endif

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<LaunchChamber>(logic);

	LATENT_BEGIN_CODE();

	ai_log(self, TXT("launch chamber, hello!"));

	if (self->element == NAME(mainControl))
	{
		messageHandler.use_with(mind);
		{
			messageHandler.set(NAME(aim_generatorOff), [self](Framework::AI::Message const& _message)
				{
					if (auto* p = _message.get_param(NAME(generator)))
					{
						if (auto* who = p->get_as<SafePtr<Framework::IModulesOwner>>().get())
						{
							self->set_generator(who, false);
						}
					}
				}
			);
			messageHandler.set(NAME(aim_generatorOn), [self](Framework::AI::Message const& _message)
				{
					if (auto* p = _message.get_param(NAME(generator)))
					{
						if (auto* who = p->get_as<SafePtr<Framework::IModulesOwner>>().get())
						{
							self->set_generator(who, true);
						}
					}
				}
			);
			messageHandler.set(NAME(aim_drain), [self](Framework::AI::Message const& _message)
				{
					if (auto* p = _message.get_param(NAME(perMinute)))
					{
						self->mainControl.drainPerMinute = p->get_as<float>();
					}
				}
			);
			messageHandler.set(NAME(aim_spawnHealth), [self](Framework::AI::Message const& _message)
				{
					if (auto* p = _message.get_param(NAME(energy)))
					{
						self->mainControl.spawnHealthEnergyQuantum = p->get_as<Energy>();
					}
				}
			);
			messageHandler.set(NAME(aim_spawnAmmo), [self](Framework::AI::Message const& _message)
				{
					if (auto* p = _message.get_param(NAME(energy)))
					{
						self->mainControl.spawnAmmoEnergyQuantum = p->get_as<Energy>();
					}
				}
			);
		}
	}

	if (self->element == NAME(display))
	{
		messageHandler.use_with(mind);
		{
			messageHandler.set(NAME(aim_broadcast), [self](Framework::AI::Message const& _message)
				{
					if (auto* p = _message.get_param(NAME(mainControl)))
					{
						if (auto* who = p->get_as<SafePtr<Framework::IModulesOwner>>().get())
						{
							self->display.mainControl = who;
						}
					}
				}
			);
			messageHandler.set(NAME(aim_drain), [self](Framework::AI::Message const& _message)
				{
					if (auto* mind = self->get_mind())
					{
						if (auto* imo = mind->get_owner_as_modules_owner())
						{
							if (auto* s = imo->get_sound())
							{
								s->play_sound(NAME(drain));
							}
						}
					}
				}
			);
		}
	}

	if (self->element == NAME(generator))
	{
		messageHandler.use_with(mind);
		{
			messageHandler.set(NAME(aim_broadcast), [self](Framework::AI::Message const& _message)
				{
					auto* imo = self->get_mind()->get_owner_as_modules_owner();
					if (auto* message = imo->get_presence()->get_in_world()->create_ai_message(NAME(aim_generatorOff)))
					{
						message->to_room(imo->get_presence()->get_in_room());
						message->access_param(NAME(generator)).access_as< SafePtr < Framework::IModulesOwner>>() = imo;
					}
				}
			);
		}
	}

	LATENT_WAIT(0.1f);

	if (self->element == NAME(mainControl))
	{
		if (auto* message = imo->get_presence()->get_in_world()->create_ai_message(NAME(aim_broadcast)))
		{
			message->to_room(imo->get_presence()->get_in_room());
			message->access_param(NAME(mainControl)).access_as< SafePtr < Framework::IModulesOwner>>() = imo;
		}
	}

	if (self->element == NAME(generator))
	{
		self->generator.buttonHandler.initialise(imo, NAME(interactiveDeviceId));
	}

	LATENT_WAIT(0.1f);

	self->mainControl.state = self->mainControl.initialState;

	self->readyAndRunning = true;

	while (true)
	{
		if (self->element == NAME(mainControl))
		{
			for_count(int, quantumIdx, 2)
			{
				auto& spawnEQ = quantumIdx == 0 ? self->mainControl.spawnAmmoEnergyQuantum : self->mainControl.spawnHealthEnergyQuantum;
				if (spawnEQ.is_positive())
				{
					auto& eqType = quantumIdx == 0 ? self->mainControl.ammoEnergyQuantumType : self->mainControl.healthEnergyQuantumType;
					if (auto* sit = eqType.get())
					{
						auto* inRoom = imo->get_presence()->get_in_room();
						Framework::DelayedObjectCreation* doc = new Framework::DelayedObjectCreation();
						doc->activateImmediatelyEvenIfRoomVisible = true;
						doc->wmpOwnerObject = imo;
						doc->inRoom = inRoom;
						doc->name = TXT("energy item");
						doc->objectType = sit;
						Transform placement = imo->get_presence()->get_placement();
						{
							Vector3 off = Vector3::zero;
							off.x = self->rg.get_float(-1.0f, 1.0f);
							off.y = self->rg.get_float(-1.0f, 1.0f);
							off.z = self->rg.get_float(-1.0f, 1.0f);
							off = off.normal();
							off *= self->rg.get_float(0.2f, 0.4f);

							placement.set_translation(placement.get_translation() + off);
						}
						doc->placement = placement;
						doc->randomGenerator = self->rg.spawn();
						doc->priority = 1000;
						doc->checkSpaceBlockers = false;

						Energy energy = spawnEQ;
						EnergyType::Type energyType = quantumIdx == 0? EnergyType::Ammo : EnergyType::Health;
						if (energyType == EnergyType::Health)
						{
							energy = energy.mul(GameSettings::get().difficulty.lootHealth);
						}
						if (energyType == EnergyType::Ammo)
						{
							energy = energy.mul(GameSettings::get().difficulty.lootAmmo);
						}
						doc->post_initialise_modules_function = [energy, energyType](Framework::Object* spawnedObject)
						{
							if (auto* meq = spawnedObject->get_gameplay_as<ModuleEnergyQuantum>())
							{
								meq->start_energy_quantum_setup();
								meq->set_energy(energy);
								meq->set_energy_type(energyType);
								meq->end_energy_quantum_setup();
								meq->no_time_limit();
							}
						};

						Game::get()->queue_delayed_object_creation(doc);
					}
					spawnEQ = Energy::zero();

					break; // spawn second in a moment
				}
			}

		}
		LATENT_WAIT_NO_RARE_ADVANCE(self->rg.get_float(0.1f, 0.2f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}
//

REGISTER_FOR_FAST_CAST(LaunchChamberData);

bool LaunchChamberData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	element = _node->get_name_attribute(TXT("element"));
	triggerGameScriptTrapOnWorkingFirstTime = _node->get_name_attribute(TXT("triggerGameScriptTrapOnWorkingFirstTime"));
	triggerGameScriptTrapOnMinStateReached = _node->get_name_attribute(TXT("triggerGameScriptTrapOnMinStateReached"));

	triggerGameScriptTrapOnGeneratorWorking = _node->get_name_attribute(TXT("triggerGameScriptTrapOnGeneratorWorking"));
	triggerGameScriptTrapOnGeneratorNotWorking = _node->get_name_attribute(TXT("triggerGameScriptTrapOnGeneratorNotWorking"));

	return result;
}

bool LaunchChamberData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
