#include "aiLogic_heartRoom.h"

#include "..\..\game\game.h"
#include "..\..\game\playerSetup.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\gameplay\moduleEnergyQuantum.h"
#include "..\..\overlayInfo\overlayInfoElement.h"

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
DEFINE_STATIC_NAME(speedPerMinutePerGenerator);
DEFINE_STATIC_NAME(speedPerMinuteBonusAllGenerators);
DEFINE_STATIC_NAME(ammoEnergyQuantumType);
DEFINE_STATIC_NAME(healthEnergyQuantumType);
DEFINE_STATIC_NAME(spawnAmmoEnergyQuantumOnOn);
DEFINE_STATIC_NAME(spawnHealthEnergyQuantumOnOn);

// variables
DEFINE_STATIC_NAME(interactiveDeviceId);

// emissive layers for buttons
DEFINE_STATIC_NAME(ready);
DEFINE_STATIC_NAME(bad);
DEFINE_STATIC_NAME_STR(badPulse, TXT("bad pulse"));
DEFINE_STATIC_NAME(operating);
DEFINE_STATIC_NAME(busy);
DEFINE_STATIC_NAME(off);
DEFINE_STATIC_NAME(active);

// elements
DEFINE_STATIC_NAME(display);
DEFINE_STATIC_NAME_STR(mainControl, TXT("main control"));
DEFINE_STATIC_NAME(generator);

// ai message
DEFINE_STATIC_NAME_STR(aim_broadcast, TXT("heart room; broadcast")); // to notify displays
DEFINE_STATIC_NAME_STR(aim_generatorOff, TXT("heart room; generator off"));
DEFINE_STATIC_NAME_STR(aim_generatorOn, TXT("heart room; generator on"));
DEFINE_STATIC_NAME_STR(aim_showState, TXT("heart room; show status"));
DEFINE_STATIC_NAME_STR(aim_hideState, TXT("heart room; hide status"));
DEFINE_STATIC_NAME_STR(aim_statusWarning, TXT("heart room; status warning"));
DEFINE_STATIC_NAME_STR(aim_statusClear, TXT("heart room; status clear"));

// ai message params
DEFINE_STATIC_NAME(energy);
DEFINE_STATIC_NAME(perMinute);

// temporary objects
DEFINE_STATIC_NAME(sparks);

//

REGISTER_FOR_FAST_CAST(HeartRoom);

HeartRoom::HeartRoom(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	heartRoomData = fast_cast<HeartRoomData>(_logicData);

	element = heartRoomData->element;
}

HeartRoom::~HeartRoom()
{
	if (display.display)
	{
		display.display->use_background(nullptr);
		display.display->set_on_update_display(this, nullptr);
	}
}

void HeartRoom::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	mainControl.speedPerMinuteBonusAllGenerators = _parameters.get_value<float>(NAME(speedPerMinuteBonusAllGenerators), mainControl.speedPerMinuteBonusAllGenerators);
	mainControl.speedPerMinutePerGenerator = _parameters.get_value<float>(NAME(speedPerMinutePerGenerator), mainControl.speedPerMinutePerGenerator);

	generator.spawnAmmoEnergyQuantumOnOn = Energy::get_from_storage(_parameters, NAME(spawnAmmoEnergyQuantumOnOn), generator.spawnAmmoEnergyQuantumOnOn);
	generator.spawnHealthEnergyQuantumOnOn = Energy::get_from_storage(_parameters, NAME(spawnHealthEnergyQuantumOnOn), generator.spawnHealthEnergyQuantumOnOn);
	{
		Framework::LibraryName sit = _parameters.get_value<Framework::LibraryName>(NAME(ammoEnergyQuantumType), Framework::LibraryName::invalid());
		if (sit.is_valid())
		{
			generator.ammoEnergyQuantumType.set_name(Framework::LibraryName(sit));
			generator.ammoEnergyQuantumType.find(Framework::Library::get_current());
		}
	}
	{
		Framework::LibraryName sit = _parameters.get_value<Framework::LibraryName>(NAME(healthEnergyQuantumType), Framework::LibraryName::invalid());
		if (sit.is_valid())
		{
			generator.healthEnergyQuantumType.set_name(Framework::LibraryName(sit));
			generator.healthEnergyQuantumType.find(Framework::Library::get_current());
		}
	}
}

void HeartRoom::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (!readyAndRunning)
	{
		return;
	}

	if (element == NAME(mainControl))
	{
		if (mainControl.workingGeneratorsCount > mainControl.workingGeneratorsCountPrev)
		{
			mainControl.workingGeneratorsCountPrev = mainControl.workingGeneratorsCount;
			if (heartRoomData->triggerGameScriptTrapOnGeneratorWorking.is_valid())
			{
				Framework::GameScript::ScriptExecution::trigger_execution_trap(heartRoomData->triggerGameScriptTrapOnGeneratorWorking);
			}
		}
		if (mainControl.workingGeneratorsCount > 0)
		{
			float prevState = mainControl.state;
			float useSpeed = mainControl.speedPerMinutePerGenerator * (float)mainControl.workingGeneratorsCount;
			if (mainControl.workingGeneratorsCount == mainControl.generators.get_size())
			{
				useSpeed += mainControl.speedPerMinuteBonusAllGenerators;
			}
			mainControl.state += useSpeed * _deltaTime / 60.0f;

			for_every(tgs, heartRoomData->triggerGameScript)
			{
				if (prevState < tgs->on && mainControl.state >= tgs->on)
				{
					if (tgs->trap.is_valid())
					{
						Framework::GameScript::ScriptExecution::trigger_execution_trap(tgs->trap);
					}
				}
			}

			if (prevState < 1.0f &&
				mainControl.state >= 1.0f)
			{
				if (heartRoomData->triggerGameScriptTrapOnReachedGoal.is_valid())
				{
					Framework::GameScript::ScriptExecution::trigger_execution_trap(heartRoomData->triggerGameScriptTrapOnReachedGoal);
				}
			}
		}
		mainControl.state = clamp(mainControl.state, 0.0f, 1.0f);

		{
			mainControl.overlayStatus.set_highlight_colour(Colour::greyLight);
			mainControl.overlayStatus.set_highlight_at_value_or_below(0);

			int value = 0;
			if (mainControl.state >= 1.0f)
			{
				value = 100;
			}
			else if (mainControl.state > 0.0f)
			{
				value = clamp(TypeConversions::Normal::f_i_cells(mainControl.state * 100.0f), 1, 99);
			}

			mainControl.overlayStatus.update(value);
		}
	}

	if (element == NAME(generator))
	{
		struct ApplyEmissive
		{
			static void to(Name const& _emissive, Framework::IModulesOwner* imo)
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
			static void to(Name const& _emissive, Array<SafePtr<Framework::IModulesOwner>> const& imos)
			{
				for_every_ref(imo, imos)
				{
					to(_emissive, imo);
				}
			}
		};

		generator.buttonHandler.advance(_deltaTime);
		if (! generator.working)
		{
			if (generator.buttonHandler.is_triggered())
			{
				generator.working = true;
				generator.updateEmissiveRequired = true;

				auto* imo = get_mind()->get_owner_as_modules_owner();
				if (auto* message = imo->get_presence()->get_in_world()->create_ai_message(NAME(aim_generatorOn)))
				{
					message->to_room(imo->get_presence()->get_in_room());
					message->access_param(NAME(generator)).access_as< SafePtr < Framework::IModulesOwner>>() = imo;
				}

				for_count(int, quantumIdx, 2)
				{
					auto& spawnEQ = quantumIdx == 0 ? generator.spawnAmmoEnergyQuantumOnOn : generator.spawnHealthEnergyQuantumOnOn;
					if (spawnEQ.is_positive())
					{
						auto& eqType = quantumIdx == 0 ? generator.ammoEnergyQuantumType : generator.healthEnergyQuantumType;
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
								off.x = rg.get_float(-1.0f, 1.0f);
								off.y = rg.get_float(-1.0f, 1.0f);
								off.z = rg.get_float(-1.0f, 1.0f);
								off = off.normal();
								off *= rg.get_float(0.1f, 0.2f);
								off.y -= 0.3f;
								off.z += 2.0f;

								placement.set_translation(placement.location_to_world(off));
							}
							doc->placement = placement;
							doc->randomGenerator = rg.spawn();
							doc->priority = 1000;
							doc->checkSpaceBlockers = false;

							Energy energy = spawnEQ;
							EnergyType::Type energyType = quantumIdx ? EnergyType::Ammo : EnergyType::Health;
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
					}
				}
			}
		}

		if (generator.updateEmissiveRequired)
		{
			generator.updateEmissiveRequired = false;

			auto* imo = get_mind()->get_owner_as_modules_owner();
			if (! generator.working)
			{
				ApplyEmissive::to(NAME(badPulse), generator.buttonHandler.get_buttons());
				ApplyEmissive::to(NAME(off), imo);
			}
			else
			{
				ApplyEmissive::to(NAME(busy), generator.buttonHandler.get_buttons());
				ApplyEmissive::to(NAME(active), imo);
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
				if (auto* mcLogic = fast_cast<HeartRoom>(mind->get_logic()))
				{
					// read only
					display.currentState = mcLogic->mainControl.state;
				}
			}
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
									if (display.displayedState == display.currentState)
									{
										return;
									}
									display.displayedState = display.currentState;

									_display->drop_all_draw_commands();
									_display->add((new Framework::DisplayDrawCommands::CLS(Colour::black)));
									_display->add((new Framework::DisplayDrawCommands::Border(Colour::black)));

									{
										float pt = display.currentState;
										Colour ink = Colour::white;
										Colour reqInk = Colour::white;
										_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
											[pt, reqInk, ink](Framework::Display* _display, ::System::Video3D* _v3d)
											{
												Vector2 bl = _display->get_left_bottom_of_screen();
												Vector2 tr = _display->get_right_top_of_screen();

												{
													float y = tr.y;
													Vector2 l[4];
													l[0] = bl;
													l[1] = Vector2(bl.x, y);
													l[2] = Vector2(tr.x, y);
													l[3] = Vector2(tr.x, bl.y);
													::System::Video3DPrimitives::line_strip_2d(reqInk, l, 4);
												}

												{
													Range2 r = Range2::empty;
													float y = round(lerp(pt, bl.y, tr.y - 2.0f));
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

void HeartRoom::set_generator(Framework::IModulesOwner* _genImo, bool _working)
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

LATENT_FUNCTION(HeartRoom::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[heart room] execute logic"));

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
	auto * self = fast_cast<HeartRoom>(logic);

	LATENT_BEGIN_CODE();

	ai_log(self, TXT("heart room, hello!"));

	if (self->element == NAME(mainControl))
	{
		messageHandler.use_with(mind);
		{
			messageHandler.set(NAME(aim_broadcast), [self](Framework::AI::Message const& _message)
				{
					if (auto* imo = self->get_imo())
					{
						if (auto* message = imo->get_presence()->get_in_world()->create_ai_message(NAME(aim_broadcast)))
						{
							message->to_room(imo->get_presence()->get_in_room());
							if (self->element == NAME(mainControl))
							{
								message->access_param(NAME(mainControl)).access_as< SafePtr < Framework::IModulesOwner>>() = imo;
							}
						}
					}
				}
			);
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
			messageHandler.set(NAME(aim_showState), [self](Framework::AI::Message const& _message)
				{
					self->mainControl.overlayStatus.show(true);
				}
			);
			messageHandler.set(NAME(aim_hideState), [self](Framework::AI::Message const& _message)
				{
					self->mainControl.overlayStatus.show(false);
				}
			);
			messageHandler.set(NAME(aim_statusWarning), [self](Framework::AI::Message const& _message)
				{
					self->mainControl.overlayStatus.set_colour(Colour::red);
				}
			);
			messageHandler.set(NAME(aim_statusClear), [self](Framework::AI::Message const& _message)
				{
					self->mainControl.overlayStatus.set_colour(NP);
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
		}
	}

	if (self->element == NAME(generator))
	{
		messageHandler.use_with(mind);
		{
			messageHandler.set(NAME(aim_broadcast), [self](Framework::AI::Message const& _message)
				{
					auto* imo = self->get_mind()->get_owner_as_modules_owner();
					if (auto* message = imo->get_presence()->get_in_world()->create_ai_message(self->generator.working? NAME(aim_generatorOn) : NAME(aim_generatorOff)))
					{
						message->to_room(imo->get_presence()->get_in_room());
						message->access_param(NAME(generator)).access_as< SafePtr < Framework::IModulesOwner>>() = imo;
					}
				}
			);
		}
	}

	LATENT_WAIT(0.1f);

	{
		if (auto* message = imo->get_presence()->get_in_world()->create_ai_message(NAME(aim_broadcast)))
		{
			message->to_room(imo->get_presence()->get_in_room());
			if (self->element == NAME(mainControl))
			{
				message->access_param(NAME(mainControl)).access_as< SafePtr < Framework::IModulesOwner>>() = imo;
			}
		}
	}

	if (self->element == NAME(generator))
	{
		self->generator.buttonHandler.initialise(imo, NAME(interactiveDeviceId));
	}

	LATENT_WAIT(0.1f);

	self->mainControl.state = 0.0f;

	self->readyAndRunning = true;

	while (true)
	{
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

REGISTER_FOR_FAST_CAST(HeartRoomData);

bool HeartRoomData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	element = _node->get_name_attribute(TXT("element"));
	for_every(node, _node->children_named(TXT("triggerGameScript")))
	{
		TriggerGameScript tgs;
		tgs.on = node->get_float_attribute(TXT("on"), tgs.on);
		tgs.trap = node->get_name_attribute(TXT("trap"), tgs.trap);
		triggerGameScript.push_back(tgs);
	}

	triggerGameScriptTrapOnReachedGoal = _node->get_name_attribute(TXT("triggerGameScriptTrapOnReachedGoal"));

	triggerGameScriptTrapOnGeneratorWorking = _node->get_name_attribute(TXT("triggerGameScriptTrapOnGeneratorWorking"));

	return result;
}

bool HeartRoomData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
