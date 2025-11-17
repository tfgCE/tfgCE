#include "aiLogic_upgradeMachine.h"

#include "utils\aiLogicUtil_deviceDisplayStart.h"

#include "..\..\game\game.h"
#include "..\..\game\gameDirector.h"
#include "..\..\library\exmType.h"
#include "..\..\library\extraUpgradeOption.h"
#include "..\..\library\lineModel.h"
#include "..\..\modules\gameplay\moduleEXM.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\music\musicPlayer.h"
#include "..\..\pilgrimage\pilgrimage.h"
#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\core\random\randomUtils.h"
#include "..\..\..\core\system\video\video3dPrimitives.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\gameScript\gameScriptExecution.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\custom\mc_display.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\world\door.h"
#include "..\..\..\framework\world\doorInRoom.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
#define TEST_VARIOUS_ICONS
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
#define INVESTIGATE_ISSUES
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// music tracks
DEFINE_STATIC_NAME_STR(mt_firstUpgradeMachine, TXT("first upgrade machine"));

// variables
DEFINE_STATIC_NAME(upgradeCanisterId);
DEFINE_STATIC_NAME(upgradeCanisterIdLeft);
DEFINE_STATIC_NAME(upgradeCanisterIdCentre);
DEFINE_STATIC_NAME(upgradeCanisterIdRight);
DEFINE_STATIC_NAME(rerollDeviceId);
DEFINE_STATIC_NAME(rerollDeviceIdLeft);
DEFINE_STATIC_NAME(rerollDeviceIdCentre);
DEFINE_STATIC_NAME(rerollDeviceIdRight);
DEFINE_STATIC_NAME(upgradeMachineShouldRemainDisabled);
DEFINE_STATIC_NAME(chosenOption); // pilgrimage device store variable
DEFINE_STATIC_NAME(chosenReroll0); // pilgrimage device store variable
DEFINE_STATIC_NAME(chosenReroll1); // pilgrimage device store variable
DEFINE_STATIC_NAME(chosenReroll2); // pilgrimage device store variable
DEFINE_STATIC_NAME(freeRerolls); // pilgrimage device store variable
DEFINE_STATIC_NAME(chanceOf1Extra);
DEFINE_STATIC_NAME(chanceOf2Extras);
DEFINE_STATIC_NAME(chanceOf3Extras);
DEFINE_STATIC_NAME(upgradesTagged);
DEFINE_STATIC_NAME(extraUpgradesTagged);
DEFINE_STATIC_NAME(upgradeUnlocked);
DEFINE_STATIC_NAME(firstUpgradeMachine);
DEFINE_STATIC_NAME(doAnalysingSequence);
DEFINE_STATIC_NAME(doAnalysingSequence_lineStart);
DEFINE_STATIC_NAME(doAnalysingSequence_lineMid);
DEFINE_STATIC_NAME(doAnalysingSequence_lineEnd);
DEFINE_STATIC_NAME(activateNavigation);
DEFINE_STATIC_NAME(activateNavigation_line);

// parameters
DEFINE_STATIC_NAME(revealSpecialDevicesAmount);
DEFINE_STATIC_NAME(revealSpecialDevicesWithTag);

// emissive layers
DEFINE_STATIC_NAME(ready);
DEFINE_STATIC_NAME(held);
DEFINE_STATIC_NAME(bad);

// sounds
DEFINE_STATIC_NAME(upgradeCanisterActivated);

// pilgrim overlay's show info's ids
DEFINE_STATIC_NAME(upgradeMachineLeft);
DEFINE_STATIC_NAME(upgradeMachineCentre);
DEFINE_STATIC_NAME(upgradeMachineRight);

// sockets
DEFINE_STATIC_NAME(showInfoLeft);
DEFINE_STATIC_NAME(showInfoCentre);
DEFINE_STATIC_NAME(showInfoRight);

// sounds
DEFINE_STATIC_NAME(machineOn);

// tags
DEFINE_STATIC_NAME(probCoef);

// room variables
DEFINE_STATIC_NAME(handleDoorOperation);
DEFINE_STATIC_NAME(handleEntranceDoorOperation);
DEFINE_STATIC_NAME(handleExitDoorOperation);

// door in room tags
DEFINE_STATIC_NAME(exitDoor);
DEFINE_STATIC_NAME(entranceDoor);

// overlay reasons
DEFINE_STATIC_NAME(upgradeMachine);
DEFINE_STATIC_NAME(upgradeCanisterReward);

// ai message
DEFINE_STATIC_NAME(reroll);

// game script traps
DEFINE_STATIC_NAME(upgradeMachineRewardGiven);

// game definition tags
DEFINE_STATIC_NAME(noExplicitTutorials);

//

static void draw_line(REF_ float& drawLines, Colour const & _colour, float _size, Vector2 _a, Vector2 _b)
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

REGISTER_FOR_FAST_CAST(UpgradeMachine);

UpgradeMachine::UpgradeMachine(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	upgradeMachineData = fast_cast<UpgradeMachineData>(_logicData);
	executionData = new ExecutionData();
}

UpgradeMachine::~UpgradeMachine()
{
}

void UpgradeMachine::advance(float _deltaTime)
{
	base::advance(_deltaTime);

#ifdef TEST_VARIOUS_ICONS
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_STANDARD_INPUT
	if (::System::Input::get()->has_key_been_pressed(::System::Key::G))
	{
		if (auto* owner = get_imo())
		{
			if (auto* message = owner->get_in_world()->create_ai_message(NAME(reroll)))
			{
				message->to_room(owner->get_presence()->get_in_room());
			}
		}
	}
	if (::System::Input::get()->has_key_been_pressed(::System::Key::H))
	{
		executionData->freeRerolls = 1000;
	}
#endif
#endif
#endif

	{
		bool upgradSelectionShouldBeVisible = false;
		if (executionData->showUpgradeSelection)
		{
			executionData->forceShowUpgradeSelection = max(0.0f, executionData->forceShowUpgradeSelection - _deltaTime);
			if (executionData->forceShowUpgradeSelection > 0.0f)
			{
				upgradSelectionShouldBeVisible = true;
			}
			else
			{
				upgradSelectionShouldBeVisible = true;
				for_every(m, executionData->machines)
				{
					if (m->canister.get_held_by_owner())
					{
						upgradSelectionShouldBeVisible = false;
						break;
					}
				}
			}
		}

		if (executionData->upgradeSelectionVisible != upgradSelectionShouldBeVisible)
		{
			ModulePilgrim* mp = nullptr;
			if (auto* g = Game::get_as<Game>())
			{
				if (auto* pa = g->access_player().get_actor())
				{
					mp = pa->get_gameplay_as<ModulePilgrim>();
				}
			}

			if (mp && mp->has_exm_equipped(EXMID::Active::reroll()))
			{
				upgradSelectionShouldBeVisible = false;
			}

			if (mp)
			{
				if (upgradSelectionShouldBeVisible)
				{
					mp->access_overlay_info().show_tip(PilgrimOverlayInfoTip::UpgradeSelection,
						PilgrimOverlayInfo::ShowTipParams().be_forced().follow_camera_pitch().with_pitch_offset(-3.0f));
				}
				else
				{
					mp->access_overlay_info().hide_tip(PilgrimOverlayInfoTip::UpgradeSelection);
				}
			}

			executionData->upgradeSelectionVisible = upgradSelectionShouldBeVisible;
		}
	}

	{
		bool mayShowUsage = false;
		bool showingUsage = false;
		if (!executionData->chosenOption.is_set())
		{
			for_every(machine, executionData->machines)
			{
				if (machine->state == Machine::MachineShowContent)
				{
					mayShowUsage = true;
				}
				if (machine->state == Machine::MachineShowUsage)
				{
					showingUsage = true;
				}
			}
		}
		if (mayShowUsage)
		{
			if (!showingUsage)
			{
				if (!executionData->showUsageIn.is_set())
				{
					// first one
					executionData->showUsageIn = Random::get_float(2.5f, 3.0f);
				}
				else
				{
					executionData->showUsageIn = executionData->showUsageIn.get() - _deltaTime;
					if (executionData->showUsageIn.get() <= 0.0f)
					{
						bool show = false;
						int idx = Random::get_int(executionData->machines.get_size());
						auto& machine = executionData->machines[idx];
						if (machine.state == Machine::MachineShowContent)
						{
							show = true;
							machine.state = Machine::MachineShowUsage;
							machine.inStateDrawnFrames = 0;
							machine.inStateTime = 0.0f;
							machine.redrawNow = true;
							machine.timeToRedraw = 0.0f;
						}
						if (show)
						{
							executionData->showUsageIn = Random::get_float(3.0f, 6.0f);
						}
					}
				}
			}
		}
		else
		{
			executionData->showUsageIn.clear();
		}
	}

	for_every(machine, executionData->machines)
	{
		machine->rerollHandler.advance(_deltaTime);

		if (machine->displaySetup)
		{
			machine->timeToRedraw -= _deltaTime;
			machine->inStateTime += _deltaTime;
			if (machine->timeToRedraw <= 0.0f)
			{
				machine->redrawNow = true;
				machine->timeToRedraw = 0.1f;
			}
			bool wasOff = machine->state == Machine::MachineOff;
			auto prevState = machine->state;
			machine->timeToCheckOn -= _deltaTime;
			if (machine->timeToCheckOn <= 0.0f || !executionData->machinesReady)
			{
				if (machine->state == Machine::MachineOff)
				{
					machine->timeToCheckOn = Random::get_float(0.0f, 0.3f);
				}
				else
				{
					machine->timeToCheckOn = 0.0f;
				}

				if (executionData->machinesReady &&
					//(rewardGiven == 0 || executionData->chosenOption == machine->machineIdx) &&
					(machine->canister.content.exm.is_set() || machine->canister.content.extra.is_set()) &&
					(machine->shouldBeOn ||
					 (machine->state != Machine::MachineOff && executionData->chosenOption == machine->machineIdx /* keep showing the chosen option */)))
				{
					if (machine->state == Machine::MachineShutDown ||
						machine->state == Machine::MachineOff)
					{
						machine->state = Machine::MachineStart;
					}
				}
				else
				{
					if (machine->state != Machine::MachineShutDown &&
						machine->state != Machine::MachineOff)
					{
						machine->state = Machine::MachineShutDown;
					}
				}

				if (machine->state != prevState)
				{
					machine->inStateDrawnFrames = 0;
					machine->inStateTime = 0.0f;
					machine->lastRedrawInStateTime = 0.0f;
					machine->redrawNow = true;
					prevState = machine->state;
				}
			}
			if (machine->redrawNow)
			{
				++machine->inStateDrawnFrames;

				// so if we change state we're clear
				machine->variousDrawingVariables.contentDrawLines = 0.0f;
				machine->variousDrawingVariables.usageDrawLines = 0.0f;

				if (machine->state == Machine::MachineStart)
				{
					ModulePilgrim* mp = nullptr;
					if (auto* g = Game::get_as<Game>())
					{
						if (auto* pa = g->access_player().get_actor())
						{
							mp = pa->get_gameplay_as<ModulePilgrim>();
						}
					}

					if (mp && executionData->doAnalysingSequence)
					{
						float lineStartAt = 1.0f;
						float lineMidAt = lineStartAt + (executionData->doAnalysingSequence_lineStart.get() && executionData->doAnalysingSequence_lineStart.get()->get_sample()? executionData->doAnalysingSequence_lineStart.get()->get_sample()->get_length() + 1.0f : 0.0f);
						float lineEndAt = lineMidAt + (executionData->doAnalysingSequence_lineMid.get() && executionData->doAnalysingSequence_lineMid.get()->get_sample()? executionData->doAnalysingSequence_lineMid.get()->get_sample()->get_length() + 0.5f : 0.0f);
						float analyseEndAt = lineEndAt + (executionData->doAnalysingSequence_lineEnd.get() && executionData->doAnalysingSequence_lineEnd.get()->get_sample()? executionData->doAnalysingSequence_lineEnd.get()->get_sample()->get_length() * 0.1f : 0.0f);
						if (machine->machineIdx == 0)
						{
							// make only one of them speak, others should just pretend
							if (machine->inStateTime >= lineStartAt && machine->lastRedrawInStateTime < lineStartAt)
							{
								mp->access_overlay_info().speak(executionData->doAnalysingSequence_lineStart.get());
							}
							if (machine->inStateTime >= lineMidAt && machine->lastRedrawInStateTime < lineMidAt)
							{
								mp->access_overlay_info().speak(executionData->doAnalysingSequence_lineMid.get());
							}
							if (machine->inStateTime >= lineEndAt && machine->lastRedrawInStateTime < lineEndAt)
							{
								mp->access_overlay_info().speak(executionData->doAnalysingSequence_lineEnd.get());
							}
						}
						if (machine->inStateTime >= analyseEndAt)
						{
							machine->state = Machine::MachineShowContent;
							machine->timeToRedraw = 0.1f;
							if (executionData->firstUpgradeMachine)
							{
								if (auto* gd = GameDefinition::get_chosen())
								{
									if (!gd->get_tags().get_tag(NAME(noExplicitTutorials)))
									{
										executionData->showUpgradeSelection = true;
										executionData->forceShowUpgradeSelection = 3.0f;
									}
								}
							}
						}
						machine->timeToRedraw = Random::get_float(0.2f, 0.4f);
					}
					else
					{
						if (machine->inStateTime > 0.5f)
						{
							machine->state = Machine::MachineShowContent;
							machine->timeToRedraw = 0.1f;
						}
					}
					if (machine->state == Machine::MachineShowContent)
					{
						if (machine->machineIdx == 0)
						{
							if (mp && mp->has_exm_equipped(EXMID::Active::reroll()))
							{
								for_count(int, hIdx, Hand::MAX)
								{
									Hand::Type hand = (Hand::Type)hIdx;
									if (auto* exm = mp->get_equipped_exm(EXMID::Active::reroll(), hand))
									{
										if (auto* exmType = exm->get_exm_type())
										{
											exmType->request_input_tip(mp, hand);
										}
									}
								}
							}
						}
					}
				}
				else if (machine->state == Machine::MachineShutDown)
				{
					int frames = 10;
					int frameNo = machine->inStateDrawnFrames;

					machine->timeToRedraw = 0.03f;

					machine->variousDrawingVariables.shutdownAtPt = (float)frameNo / (float)frames;

					if (frameNo >= frames)
					{
						machine->state = Machine::MachineOff;
					}
				}
				else if (machine->state == Machine::MachineShowUsage)
				{
					float timeStart = 1.0f;
					float timeEnd = 1.5f;
					float timeDone = 2.0f;
					machine->variousDrawingVariables.usageDrawAtPt = clamp((machine->inStateTime - timeStart) / (timeEnd - timeStart), 0.0f, 1.0f);
					if (machine->inStateTime > timeDone)
					{
						machine->state = Machine::MachineShowContent;
					}
					machine->timeToRedraw = 0.03f;
					machine->variousDrawingVariables.usageDrawLines = (40.0f * (machine->inStateTime / max(0.2f, timeStart)));
				}
				else if (machine->state == Machine::MachineShowContent)
				{
					machine->variousDrawingVariables.contentLineModel = nullptr;
					machine->variousDrawingVariables.borderLineModel = nullptr;

					if (machine->canister.content.exm.is_set())
					{
						machine->variousDrawingVariables.contentLineModel = machine->canister.content.exm->get_line_model_for_display();
						machine->variousDrawingVariables.borderLineModel = machine->canister.content.exm->is_active()? upgradeMachineData->activeEXMLineModel.get() : upgradeMachineData->passiveEXMLineModel.get();
					}
					if (machine->canister.content.extra.is_set())
					{
						machine->variousDrawingVariables.contentLineModel = machine->canister.content.extra->get_line_model_for_display();
					}

					machine->variousDrawingVariables.contentDrawLines = (float)(machine->inStateDrawnFrames * 2);
					if (machine->variousDrawingVariables.contentLineModel)
					{
						machine->timeToRedraw = 0.03f;
					}
					else
					{
						machine->timeToRedraw = 5.0f;
					}
				}

				if (machine->state != prevState)
				{
					machine->inStateDrawnFrames = 0;
					machine->inStateTime = 0.0f;
					machine->lastRedrawInStateTime = 0.0f;
					machine->redrawNow = true;
				}
				machine->lastRedrawInStateTime = machine->inStateTime;
			}
			bool isOff = machine->state == Machine::MachineOff;
			if ((isOff ^ wasOff) &&
				machine->displayOwner.get())
			{
				if (auto* s = machine->displayOwner->get_sound())
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
		}

		if (machine->display && executionData->machinesReady && !machine->displaySetup)
		{
			machine->displaySetup = true;
			machine->display->draw_all_commands_immediately();
			machine->display->set_on_update_display(this,
				[this, machine](Framework::Display* _display)
				{
					if (!machine->redrawNow)
					{
						return;
					}
					machine->redrawNow = false;

					_display->drop_all_draw_commands();
					_display->add((new Framework::DisplayDrawCommands::CLS(Colour::black)));
					_display->add((new Framework::DisplayDrawCommands::Border(Colour::black)));
					if (machine->state == Machine::MachineStart)
					{
						if (machine->inStateDrawnFrames == 0) _display->add((new Framework::DisplayDrawCommands::CLS(Colour::black)));
						machine->redrawNow = false;
						machine->timeToRedraw = 0.06f;
						int frames = machine->inStateDrawnFrames;
						_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
							[frames](Framework::Display* _display, ::System::Video3D* _v3d)
							{
								Utils::draw_device_display_start(_display, _v3d, frames);
							}));
					}
					else if (machine->state == Machine::MachineShutDown)
					{
						Vector2 bl = _display->get_left_bottom_of_screen() + Vector2::half;
						Vector2 tr = _display->get_right_top_of_screen() - Vector2::half;

						float atY = tr.y + (bl.y - tr.y) * clamp(machine->variousDrawingVariables.shutdownAtPt, 0.0f, 1.0f);

						Vector2 a(bl.x, atY);
						Vector2 b(tr.x, atY);
						_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
							[a, b](Framework::Display* _display, ::System::Video3D* _v3d)
							{
								::System::Video3DPrimitives::line_2d(_display->get_current_ink(), a, b);
							}));
					}
					else if (machine->state == Machine::MachineShowContent)
					{
						auto* lm = machine->variousDrawingVariables.contentLineModel;
						auto* blm = machine->variousDrawingVariables.borderLineModel;
						float drawLines = machine->variousDrawingVariables.contentDrawLines;
						_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
							[lm, blm, drawLines](Framework::Display* _display, ::System::Video3D* _v3d)
							{
								float linesLeft = drawLines;
								Colour const ink = _display->get_current_ink();

								Vector2 bl = _display->get_left_bottom_of_screen() + Vector2::half;
								Vector2 tr = _display->get_right_top_of_screen() - Vector2::half;
								
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
							}));
					}
					else if (machine->state == Machine::MachineShowUsage)
					{
						auto* portLineModel = upgradeMachineData->portLineModel.get();
						auto* canisterLineModel = upgradeMachineData->canisterLineModel.get();

						float drawCanisterAtYPt = machine->variousDrawingVariables.usageDrawAtPt;
						float usageDrawLines = machine->variousDrawingVariables.usageDrawLines;

						_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
							[portLineModel, canisterLineModel, drawCanisterAtYPt, usageDrawLines](Framework::Display* _display, ::System::Video3D* _v3d)
							{
								float drawLines = usageDrawLines;
								Colour const ink = _display->get_current_ink();

								Vector2 bl = _display->get_left_bottom_of_screen() + Vector2::half;
								Vector2 tr = _display->get_right_top_of_screen() - Vector2::half;
								
								Vector2 c = TypeConversions::Normal::f_i_cells((bl + tr) * 0.5f).to_vector2() + Vector2::half;

								Vector2 size(tr.x - c.x, tr.y - c.y);
								size.x = min(size.x, size.y);
								size.y = size.x;

								size = TypeConversions::Normal::f_i_closest(size * 2.0f).to_vector2();

								if (portLineModel && drawLines > 0)
								{
									for_every(l, portLineModel->get_lines())
									{
										if (drawLines <= 0.0f)
										{
											break;
										}

										draw_line(REF_ drawLines, l->colour.get(ink), size.y,
											TypeConversions::Normal::f_i_cells(c + size * l->a.to_vector2()).to_vector2() + Vector2::half,
											TypeConversions::Normal::f_i_cells(c + size * l->b.to_vector2()).to_vector2() + Vector2::half);
									}
								}
								if (canisterLineModel && drawLines > 0)
								{
									Vector2 offset = Vector2::zero;
									offset.y = size.y * 0.35f * drawCanisterAtYPt;
									for_every(l, canisterLineModel->get_lines())
									{
										if (drawLines <= 0.0f)
										{
											break;
										}
										draw_line(REF_ drawLines, l->colour.get(ink), size.y,
											TypeConversions::Normal::f_i_cells(c + size * l->a.to_vector2() + offset).to_vector2() + Vector2::half,
											TypeConversions::Normal::f_i_cells(c + size * l->b.to_vector2() + offset).to_vector2() + Vector2::half);
									}
								}
							}));
						_display->add((new Framework::DisplayDrawCommands::Border(Colour::black)));
					}
				});
		}
	}
}

void UpgradeMachine::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);

	if (auto* mind = get_mind())
	{
		if (auto* imo = mind->get_owner_as_modules_owner())
		{
			int chosenOption = imo->access_variables().get_value<int>(NAME(chosenOption), 0);
			chosenOption -= 1;
			if (chosenOption >= 0)
			{
				executionData->chosenOption = chosenOption;
			}
			else
			{
				executionData->chosenOption.clear();
			}
			Name chosenRerolls[] = { NAME(chosenReroll0),  NAME(chosenReroll1), NAME(chosenReroll2) };
			for_count(int, i, 3)
			{
				int chosenReroll = imo->access_variables().get_value<int>(chosenRerolls[i], 0);
				chosenReroll -= 1;
				if (chosenReroll >= 0)
				{
					executionData->chosenReroll[i] = chosenReroll;
				}
				else
				{
					executionData->chosenReroll[i].clear();
				}
			}
			executionData->freeRerolls = imo->get_variables().get_value<int>(NAME(freeRerolls), 0);
		}
	}

}

void UpgradeMachine::set_chosen_option(int _idx)
{
#ifdef INVESTIGATE_ISSUES
	output(TXT("[UpgradeMachine] chosen option %i"), _idx);
#endif
	executionData->chosenOption = _idx;
	if (auto* imo = get_imo())
	{
		imo->access_variables().access<int>(NAME(chosenOption)) = _idx + 1;
	}
}

void UpgradeMachine::add_chosen_reroll(int _idx)
{
#ifdef INVESTIGATE_ISSUES
	output(TXT("[UpgradeMachine] chosen reroll %i"), _idx);
#endif
	if (executionData->freeRerolls < freeRerollsLimit)
	{
		++executionData->freeRerolls;
	}
	else
	{
		for_count(int, i, 3)
		{
			if (!executionData->chosenReroll[i].is_set() || executionData->chosenReroll[i].get() == _idx)
			{
				executionData->chosenReroll[i] = _idx;
				break;
			}
		}
	}
	if (auto* imo = get_imo())
	{
		imo->access_variables().access<int>(NAME(chosenReroll0)) = executionData->chosenReroll[0].get(-1) + 1;
		imo->access_variables().access<int>(NAME(chosenReroll1)) = executionData->chosenReroll[1].get(-1) + 1;
		imo->access_variables().access<int>(NAME(chosenReroll2)) = executionData->chosenReroll[2].get(-1) + 1;
		imo->access_variables().access<int>(NAME(freeRerolls)) = executionData->freeRerolls;
	}
}

void UpgradeMachine::issue_choose_unlocks()
{
	MEASURE_PERFORMANCE(issue_choose_unlocks);
	RefCountObjectPtr<ExecutionData> execData = executionData;
	SafePtr<Framework::IModulesOwner> safeIMO(get_imo());
	{
		execData->machinesReady = false;
		// clear current setup
		for_every(machine, execData->machines)
		{
			machine->canister.content.exm.clear();
			machine->canister.content.extra.clear();
		}
	}
	if (auto* g = Game::get_as<Game>())
	{
		g->add_async_world_job_top_priority(TXT("choose unlocks"), [execData, safeIMO, this]()
		{
			if (!safeIMO.get())
			{
				return;
			}

#ifdef INVESTIGATE_ISSUES
			output(TXT("[UpgradeMachine] choose unlocks"));
#endif

			Random::Generator rg = safeIMO->get_individual_random_generator();
			rg.advance_seed(9076508, 90725);

			ArrayStatic<bool, 3> extra; SET_EXTRA_DEBUG_INFO(extra, TXT("UpgradeMachine::issue_choose_unlocks.extra"));
			ArrayStatic<bool, 3> shouldBeOff; SET_EXTRA_DEBUG_INFO(shouldBeOff, TXT("UpgradeMachine::issue_choose_unlocks.shouldBeOff"));

			UpgradeMachineData::UpgradeMachineSets::Set const* useSet = nullptr;

			{
				int setIdx = NONE;
				if (executionData->firstUpgradeMachine)
				{
#ifdef INVESTIGATE_ISSUES
					output(TXT("[UpgradeMachine] first upgrade machine"));
#endif
					setIdx = 0;
				}
				else if (auto* pi = PilgrimageInstance::get())
				{
					int unlocked = pi->get_unlocked_upgrade_machines();
#ifdef INVESTIGATE_ISSUES
					output(TXT("[UpgradeMachine] unlocked upgrade machines: %i"), unlocked);
#endif
					if (unlocked > 0)
					{
						setIdx = unlocked;
					}
				}
#ifdef INVESTIGATE_ISSUES
				output(TXT("[UpgradeMachine] set index to use (if available): %i"), setIdx);
#endif

				UpgradeMachineData::UpgradeMachineSets const* ums = nullptr;
				if (auto* gd = GameDefinition::get_chosen())
				{
					for_every(i, upgradeMachineData->upgradeMachineSets)
					{
						if (GameDefinition::check_chosen(i->forGameDefinitionTagged))
						{
							ums = i;
							break;
						}
					}
				}
				if (ums && ums->sets.is_index_valid(setIdx))
				{
#ifdef INVESTIGATE_ISSUES
					output(TXT("[UpgradeMachine] set index to use (present, available to use): %i"), setIdx);
#endif
					useSet = &ums->sets[setIdx];
				}
			}

			// choose spots for extras
			{
				while (extra.get_size() < 3) extra.push_back(false);
				while (shouldBeOff.get_size() < 3) shouldBeOff.push_back(false);

				int extraCount = 0;
				if (! useSet)
				{
					float chance = rg.get_float(0.0f, 1.0f);
					float chanceOf1Extra = safeIMO->get_value<float>(NAME(chanceOf1Extra), upgradeMachineData->chanceOf1Extra);
					float chanceOf2Extras = safeIMO->get_value<float>(NAME(chanceOf2Extras), upgradeMachineData->chanceOf2Extras);
					float chanceOf3Extras = safeIMO->get_value<float>(NAME(chanceOf3Extras), upgradeMachineData->chanceOf3Extras);
					if (chance <= chanceOf1Extra) extraCount = 1;
					if (chance <= chanceOf2Extras) extraCount = 2;
					if (chance <= chanceOf3Extras) extraCount = 3;
				}

#ifdef INVESTIGATE_ISSUES
				output(TXT("[UpgradeMachine] extra count %i"), extraCount);
#endif

				while (extraCount > 0)
				{
					int eIdx = rg.get_int(extra.get_size());
					while (extra[eIdx])
					{
						eIdx = mod(eIdx + 1, extra.get_size());
					}
					extra[eIdx] = true;
					--extraCount;
				}
			}

			int rerollsDone = 0;

			// exms
			{
				todo_multiplayer_issue(TXT("we just get player here, we should get all players?"));
				ModulePilgrim* mp = nullptr;
				if (auto* g = Game::get_as<Game>())
				{
					if (auto* pa = g->access_player().get_actor())
					{
						mp = pa->get_gameplay_as<ModulePilgrim>();
					}
				}

				Array<EXMType*> firstEXMs;
				if (useSet)
				{
					for_every(exmID, useSet->exms)
					{
						if (auto* exm = EXMType::find(*exmID))
						{
							firstEXMs.push_back(cast_to_nonconst(exm));
						}
					}
				}
				Array<EXMType*> secondEXMs;
				Array<EXMType*> thirdEXMs;
				bool readAllEXMs = false;
				bool keepOnlyEXMsAvailableToPilgrim = false;
				if (firstEXMs.is_empty())
				{
					readAllEXMs = true;
				}
				if (executionData->firstUpgradeMachine && GameSettings::get().difficulty.allowEXMReinstalling && mp)
				{
					readAllEXMs = true;
					keepOnlyEXMsAvailableToPilgrim = true;
					// and remove already available exms from the firstEXMs
					for (int i = 0; i < firstEXMs.get_size(); ++i)
					{
						auto* exm = firstEXMs[i];
						if (mp->get_pilgrim_inventory().is_exm_available(exm->get_id()))
						{
							firstEXMs.remove_at(i);
							--i;
						}
					}
				}
				if (readAllEXMs)
				{
					TagCondition upgradesTagged = safeIMO->get_value<TagCondition>(NAME(upgradesTagged), upgradeMachineData->upgradesTagged);
					RangeInt exmLevels = RangeInt::empty;
					if (auto* piow = PilgrimageInstanceOpenWorld::get())
					{
						exmLevels = piow->get_pilgrimage()->get_allowed_exm_levels();
					}
					secondEXMs = EXMType::get_all_exms(upgradesTagged, exmLevels);

					TagCondition gdRequirement;
					if (auto* gd = GameDefinition::get_chosen())
					{
						gdRequirement = gd->get_exms_tagged();
					}

					// drop permanent (unless allowed)
					{
						for (int i = 0; i < secondEXMs.get_size(); ++i)
						{
							auto* exm = secondEXMs[i];
							if (!gdRequirement.check(exm->get_tags()) ||
								(exm->is_permanent() && !GameSettings::get().difficulty.allowPermanentEXMsAtUpgradeMachine) ||
								exm->is_integral())
							{
								secondEXMs.remove_at(i);
								--i;
							}
						}
					}
					thirdEXMs = secondEXMs;

					// drop all EXMs that we have already on us - no need to reinstall something we have
					{
						for (int i = 0; i < secondEXMs.get_size(); ++i)
						{
							auto* exm = secondEXMs[i];
							if (mp->has_exm_equipped(exm->get_id()))
							{
								secondEXMs.remove_at(i);
								--i;
							}
						}
					}

					// drop all EXMs that do not belong to pilgrim (this is for special cases)
					if (keepOnlyEXMsAvailableToPilgrim)
					{
						for (int i = 0; i < secondEXMs.get_size(); ++i)
						{
							auto* exm = secondEXMs[i];
							if (!mp->get_pilgrim_inventory().is_exm_available(exm->get_id()))
							{
								secondEXMs.remove_at(i);
								--i;
							}
						}
					}
				}

				struct Slot
				{
					bool isEXM = true;
					EXMType* chosen = nullptr;
					int iteration = 0;
					Random::Generator rg;
					Array<EXMType*> firstEXMs; // provided by upgrade machine
					Array<EXMType*> secondEXMs; // all EXMs, except for not available to the player
					Array<EXMType*> thirdEXMs; // all EXMs
										
					static bool check_if_unique(ArrayStatic<Slot, 3>& slots)
					{
						bool unique = true;
						for_every(slot, slots)
						{
							if (slot->isEXM && slot->chosen)
							{
								for_every_continue_after(nslot, slot)
								{
									if (nslot->isEXM &&
										nslot->chosen == slot->chosen)
									{
										unique = false;
										if (nslot->iteration >= slot->iteration)
										{
											nslot->chosen = nullptr;
										}
										else
										{
											slot->chosen = nullptr;
											break;
										}
									}
								}
							}
						}
						return unique;
					}

					static bool check_if_already_used(ArrayStatic<Slot, 3>& slots, ModulePilgrim* mp, bool _firstUpgradeMachine)
					{
						if (!mp || _firstUpgradeMachine)
						{
							// same set for first upgrade machine - always, even if you already unlocked
							return true; // ok
						}
						bool alreadyUsed = false;
						bool allowEXMReinstalling = GameSettings::get().difficulty.allowEXMReinstalling;
						for_every(slot, slots)
						{
							if (slot->isEXM &&
								slot->chosen)
							{
								bool isOk = true;
								if (slot->chosen->is_integral())
								{
									an_assert(false, TXT("we shouldn't be installing integral EXMs this way"));
									isOk = false;
								}
								else if (slot->chosen->is_permanent())
								{
									if (slot->chosen->get_permanent_limit() > 0)
									{
										int existing = mp->get_pilgrim_inventory().get_permanent_exm_count(slot->chosen->get_id());
										if (existing >= slot->chosen->get_permanent_limit())
										{
											isOk = false;
										}
									}
								}
								else
								{
									if (allowEXMReinstalling)
									{
										if (mp->get_pilgrim_inventory().is_exm_available(slot->chosen->get_id()))
										{
											isOk = false;
										}
									}
									else
									{
										if (mp->get_pending_pilgrim_setup().get_hand(Hand::Left).has_exm_installed(slot->chosen->get_id()) ||
											mp->get_pending_pilgrim_setup().get_hand(Hand::Right).has_exm_installed(slot->chosen->get_id()))
										{
											isOk = false;
										}
									}
								}
								if (! isOk)
								{
									slot->chosen = nullptr;
									alreadyUsed = true;
								}
							}
						}
						return !alreadyUsed;
					}

					static bool check_if_nothing_more_to_do(ArrayStatic<Slot, 3>& slots)
					{
						bool somethingToBeDone = false;
						for_every(slot, slots)
						{
							if (slot->isEXM &&
								!slot->chosen &&
								!slot->firstEXMs.is_empty() &&
								!slot->secondEXMs.is_empty() &&
								!slot->thirdEXMs.is_empty())
							{
								somethingToBeDone = true;
							}
						}
						return !somethingToBeDone;
					}


				};
				ArrayStatic<Slot, 3> slots; SET_EXTRA_DEBUG_INFO(slots, TXT("UpgradeMachine::issue_choose_unlocks.slots"));
				slots.set_size(3);
				for_every(slot, slots)
				{
					slot->isEXM = !extra[for_everys_index(slot)];
					slot->firstEXMs = firstEXMs;
					slot->secondEXMs = secondEXMs;
					slot->thirdEXMs = thirdEXMs;
					slot->rg = rg.spawn();
				}

				int rerollIdx = -1;
				int freeRerollIdx = 0;
				while (freeRerollIdx < executionData->freeRerolls || rerollIdx < 3)
				{
#ifdef INVESTIGATE_ISSUES
					output(TXT("[UpgradeMachine] free reroll %i/%i, rerollIdx %i"), freeRerollIdx, executionData->freeRerolls, rerollIdx);
#endif

					while (true) // choose exms
					{
						for_every(slot, slots)
						{
							if (slot->isEXM && !slot->chosen)
							{
								if (slot->firstEXMs.is_empty() &&
									!slot->secondEXMs.is_empty()) // no need to check thirdEXMs
								{
									slot->firstEXMs = slot->secondEXMs;
									slot->secondEXMs = slot->thirdEXMs;
									slot->thirdEXMs.clear();
								}
								if (!slot->firstEXMs.is_empty())
								{
									int idx = RandomUtils::ChooseFromContainer<Array<EXMType*>, EXMType*>::choose(
										slot->rg, slot->firstEXMs, [](EXMType* _e) { return _e->get_tags().get_tag(NAME(probCoef), 1.0f); });
									slot->chosen = slot->firstEXMs[idx];
									slot->firstEXMs.remove_fast_at(idx);
									++slot->iteration;
								}
							}
						}

						if (!Slot::check_if_already_used(slots, mp, executionData->firstUpgradeMachine))
						{
							continue;
						}

						if (!Slot::check_if_unique(slots))
						{
							continue;
						}

						if (!Slot::check_if_nothing_more_to_do(slots))
						{
							continue;
						}

						break;
					}

#ifdef INVESTIGATE_ISSUES
					for_every(slot, slots)
					{
						output(TXT("[UpgradeMachine] slot %i : %S : %S"), for_everys_index(slot), slot->isEXM? TXT("exm") : TXT("extra"), slot->chosen? slot->chosen->get_id().to_char() : TXT("--"));
					}
#endif

					{
						bool doReroll = false;
						if (freeRerollIdx < executionData->freeRerolls)
						{
							doReroll = true;
							++freeRerollIdx;
						}
						else
						{
							++rerollIdx;
							if (rerollIdx >= 0 && rerollIdx <= 2)
							{
								if (execData->chosenReroll[rerollIdx].is_set())
								{
									// change rerolled to extra
									int slotIdx = execData->chosenReroll[rerollIdx].get();
									auto& slot = slots[slotIdx];
									slot.isEXM = false;
									extra[slotIdx] = true;
									doReroll = true;
								}
							}
						}
						if (doReroll)
						{
							// clear all - we're to reroll them!
							for_every(slot, slots)
							{
								slot->chosen = nullptr;
							}
							if (rerollIdx > 0)
							{
								shouldBeOff[execData->chosenReroll[rerollIdx - 1].get()] = true;
							}
							++rerollsDone;
						}
					}
				}

#ifdef INVESTIGATE_ISSUES
				for_every(slot, slots)
				{
					output(TXT("[UpgradeMachine] slot %i : %S : %S"), for_everys_index(slot), slot->isEXM ? TXT("exm") : TXT("extra"), slot->chosen ? slot->chosen->get_id().to_char() : TXT("--"));
				}
#endif
				// no exms? make them extras
				for_every(slot, slots)
				{
					if (slot->isEXM)
					{
						int i = for_everys_index(slot);
						if (slot->chosen)
						{
							execData->machines[i].canister.content.exm = slot->chosen;
						}
						else
						{
							extra[i] = true;
						}
					}
				}
			}

#ifdef AN_DEVELOPMENT
			{
				// test
				//execData->machines[0].canister.content.exm = Library::get_current_as<Library>()->get_exm_types().find(Framework::LibraryName(String(TXT("exms:energy dispatcher"))));
			}
#endif

			// fill extras
			{
				Array<ExtraUpgradeOption*> availableEUOs;
				{
					TagCondition extraUpgradesTagged = safeIMO->get_value<TagCondition>(NAME(extraUpgradesTagged), upgradeMachineData->extraUpgradesTagged);
					availableEUOs = ExtraUpgradeOption::get_all(extraUpgradesTagged);
				}

				Random::Generator euoRG = rg.spawn();
				euoRG.advance_seed(rerollsDone, rerollsDone);

				for_every(machine, execData->machines)
				{
					if (!machine->canister.content.exm.get() &&
						!machine->canister.content.extra.get() &&
						!availableEUOs.is_empty())
					{
						int idx = RandomUtils::ChooseFromContainer<Array<ExtraUpgradeOption*>, ExtraUpgradeOption*>::choose(
							euoRG, availableEUOs, [](ExtraUpgradeOption* _e) { return _e->get_tags().get_tag(NAME(probCoef), 1.0f); });
						machine->canister.content.extra = availableEUOs[idx];
						availableEUOs.remove_fast_at(idx);
					}
				}
			}

#ifdef INVESTIGATE_ISSUES
			output(TXT("[UpgradeMachine] machines setup"));
			for_every(machine, execData->machines)
			{
				output(TXT("[UpgradeMachine] machine %i : %S : %S"),
					for_everys_index(machine),
					machine->canister.content.exm.get() ? TXT("exm") : TXT("extra"),
					machine->canister.content.exm.get() ? machine->canister.content.exm->get_id().to_char() :
					  (machine->canister.content.extra.get() ? machine->canister.content.extra->get_name().to_string().to_char() : TXT("--")));
			}
#endif

			for_every(machine, execData->machines)
			{
				if (shouldBeOff[for_everys_index(machine)])
				{
					machine->canister.content.exm.clear();
					machine->canister.content.extra.clear();
				}
			}

#ifdef INVESTIGATE_ISSUES
			output(TXT("[UpgradeMachine] after machines should be off check"));
			for_every(machine, execData->machines)
			{
				output(TXT("[UpgradeMachine] machine %i : %S : %S"),
					for_everys_index(machine),
					machine->canister.content.exm.get() ? TXT("exm") : TXT("extra"),
					machine->canister.content.exm.get() ? machine->canister.content.exm->get_id().to_char() :
					(machine->canister.content.extra.get() ? machine->canister.content.extra->get_name().to_string().to_char() : TXT("--")));
			}
#endif

			execData->machinesReady = true;
		});
	}
}

LATENT_FUNCTION(UpgradeMachine::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai upgrade machine] execute logic"));
	MEASURE_PERFORMANCE(UpgradeMachine_execute_logic);

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(Optional<VectorInt2>, cellAt);
	LATENT_VAR(bool, depleted);
	LATENT_VAR(float, playerCheckTimeLeft);
	LATENT_VAR(bool, playerIn);
	LATENT_VAR(int, machineIdx);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<UpgradeMachine>(logic);

	playerCheckTimeLeft -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	ai_log(self, TXT("upgrade machine, hello!"));

	depleted = false;
	playerIn = false;
	playerCheckTimeLeft = 0.0f;

	if (imo)
	{
		{
			MEASURE_PERFORMANCE(setup_1);
			self->executionData->firstUpgradeMachine = false;
			self->executionData->doAnalysingSequence = false;
			if (auto* piow = PilgrimageInstanceOpenWorld::get())
			{
				self->executionData->firstUpgradeMachine = imo->get_variables().get_value<bool>(NAME(firstUpgradeMachine), false);
				self->executionData->doAnalysingSequence = imo->get_variables().get_value<bool>(NAME(doAnalysingSequence), false);
				self->executionData->doAnalysingSequence_lineStart.set_name(imo->get_variables().get_value<Framework::LibraryName>(NAME(doAnalysingSequence_lineStart), Framework::LibraryName::invalid()));
				self->executionData->doAnalysingSequence_lineMid.set_name(imo->get_variables().get_value<Framework::LibraryName>(NAME(doAnalysingSequence_lineMid), Framework::LibraryName::invalid()));
				self->executionData->doAnalysingSequence_lineEnd.set_name(imo->get_variables().get_value<Framework::LibraryName>(NAME(doAnalysingSequence_lineEnd), Framework::LibraryName::invalid()));
				self->executionData->activateNavigation = imo->get_variables().get_value<bool>(NAME(activateNavigation), false);
				self->executionData->activateNavigation_line.set_name(imo->get_variables().get_value<Framework::LibraryName>(NAME(activateNavigation_line), Framework::LibraryName::invalid()));
				if (auto* lib = Framework::Library::get_current())
				{
					self->executionData->doAnalysingSequence_lineStart.find(lib);
					self->executionData->doAnalysingSequence_lineMid.find(lib);
					self->executionData->doAnalysingSequence_lineEnd.find(lib);
					self->executionData->activateNavigation_line.find(lib);
				}
				cellAt = piow->find_cell_at(imo);
				if (cellAt.is_set())
				{
					depleted = piow->is_pilgrim_device_state_depleted(cellAt.get(), imo);
					ai_log(self, depleted ? TXT("depleted") : TXT("available"));
					if (!depleted)
					{
						// this will issue a world job to make sure these exist
						int radius = self->upgradeMachineData->longRangeRevealSpecialDevicesCellRadius;
						output(TXT("upgrade machine requires to make sure cells are ready at %ix%i, %i"), cellAt.get().x, cellAt.get().y, radius);
						piow->make_sure_cells_are_ready(cellAt.get(), radius);
					}
				}
			}
		}
		LATENT_YIELD();

		{
			self->executionData->machines.clear();
			self->executionData->machines.set_size(3);
			self->executionData->machines[0].canister.overlayInfoId = NAME(upgradeMachineLeft);
			self->executionData->machines[1].canister.overlayInfoId = NAME(upgradeMachineCentre);
			self->executionData->machines[2].canister.overlayInfoId = NAME(upgradeMachineRight);
			self->executionData->machines[0].upgradeMachine = self;
			self->executionData->machines[1].upgradeMachine = self;
			self->executionData->machines[2].upgradeMachine = self;
			self->executionData->machines[0].machineIdx = 0;
			self->executionData->machines[1].machineIdx = 1;
			self->executionData->machines[2].machineIdx = 2;
			{
				ai_log(self, TXT("look for components"));
				while (!self->executionData->machines[0].displayOwner.get() ||
					   !self->executionData->machines[1].displayOwner.get() ||
					   !self->executionData->machines[2].displayOwner.get() ||
					   !self->executionData->machines[0].canister.has_canister_imo() ||
					   !self->executionData->machines[1].canister.has_canister_imo() ||
					   !self->executionData->machines[2].canister.has_canister_imo())
				{
	#ifdef INVESTIGATE_ISSUES
					output(TXT("[UpgradeMachine] look for machines and canisters"));
					for_count(int, i, 3)
					{
						output(TXT("[UpgradeMachine] machine %i : %S"), i, self->executionData->machines[i].displayOwner.get() ? TXT("OK") : TXT("looking..."));
					}
					for_count(int, i, 3)
					{
						output(TXT("[UpgradeMachine] canister %i : %S"), i, self->executionData->machines[i].canister.has_canister_imo() ? TXT("OK") : TXT("looking..."));
					}
	#endif
					{
						for(machineIdx = 0; machineIdx< self->executionData->machines.get_size(); ++ machineIdx)
						{
							{
								auto* machine = &self->executionData->machines[machineIdx];
								int i = machineIdx;
								{
									Name ucIdName = i == 0 ? NAME(upgradeCanisterIdLeft) : (i == 1 ? NAME(upgradeCanisterIdCentre) : NAME(upgradeCanisterIdRight));
									if (auto* id = imo->get_variables().get_existing<int>(ucIdName))
									{
										auto* world = imo->get_in_world();
										an_assert(world);

										machine->canister.showInfoAt = imo->get_presence()->get_centre_of_presence_transform_WS();
										if (auto* ap = imo->get_appearance())
										{
											Name socketId = i == 0 ? NAME(showInfoLeft) : (i == 1 ? NAME(showInfoCentre) : NAME(showInfoRight));
											machine->canister.showInfoAt = imo->get_presence()->get_placement().to_world(ap->calculate_socket_os(socketId));
										}

										machine->canister.initialise(imo, ucIdName);

										world->for_every_object_with_id(NAME(upgradeCanisterId), *id,
											[imo, &machine](Framework::Object* _object)
											{
												if (_object != imo)
												{
													if (auto* cmd = _object->get_custom<Framework::CustomModules::Display>())
													{
														machine->display = cmd->get_display();
														machine->displayOwner = _object;
														machine->redrawNow = true;
														return true; // one is enough
													}
												}
												return false; // keep going on
											});
									}
								}
								{
									Name rdIdName = i == 0 ? NAME(rerollDeviceIdLeft) : (i == 1 ? NAME(rerollDeviceIdCentre) : NAME(rerollDeviceIdRight));
									if (auto* id = imo->get_variables().get_existing<int>(rdIdName))
									{
	#ifdef AN_DEVELOPMENT
										auto* world = imo->get_in_world();
										an_assert(world);
	#endif

										machine->rerollHandler.initialise(imo, rdIdName, NAME(rerollDeviceId));
									}
								}
							}
							LATENT_WAIT(Random::get_float(0.1f, 0.25f));
						}
					}
					LATENT_WAIT(Random::get_float(0.2f, 0.5f));
				}
				ai_log(self, TXT("all components found"));
			}
		}
		if (auto* g = Game::get())
		{
			self->issue_choose_unlocks();
		}
		ai_log(self, TXT("machines available"));

		if (auto* r = imo->get_presence()->get_in_room())
		{
			self->manageExitDoor = r->get_value<bool>(NAME(handleDoorOperation), false, false /* just this room */) ||
								   r->get_value<bool>(NAME(handleExitDoorOperation), false, false /* just this room */);
			self->manageEntranceDoor = r->get_value<bool>(NAME(handleDoorOperation), false, false /* just this room */) ||
									   r->get_value<bool>(NAME(handleEntranceDoorOperation), false, false /* just this room */);
			//FOR_EVERY_DOOR_IN_ROOM(dir, r)
			for_every_ptr(dir, r->get_doors())
			{
				// invisible doors are ok here
				if (dir->get_tags().get_tag(NAME(entranceDoor)))
				{
					self->entranceDoor = dir;
					self->initialRoomBehindEntranceDoor = dir->get_room_on_other_side();
				}
				if (dir->get_tags().get_tag(NAME(exitDoor)))
				{
					self->exitDoor = dir;
				}
			}
		}
	}

	{
		for_every(machine, self->executionData->machines)
		{
			machine->canister.update_emissive(UpgradeCanister::EmissiveOff, true);
			machine->update_reroll_emissive(false, true);
		}
	}

	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(reroll), [self](Framework::AI::Message const& _message)
			{
#ifdef INVESTIGATE_ISSUES
				output(TXT("[UpgradeMachine] reroll message received"));
#endif
				if (self->executionData->chosenOption.is_set())
				{
#ifdef INVESTIGATE_ISSUES
					output(TXT("[UpgradeMachine] can't reroll, already chosen %i"), self->executionData->chosenOption.get());
#endif
					// already chosen, nothing to reroll
					return;
				}
				ArrayStatic<int, 3> availableRerolls; SET_EXTRA_DEBUG_INFO(availableRerolls, TXT("UpgradeMachine::execute_logic.availableRerolls"));
				while (availableRerolls.has_place_left())
				{
					availableRerolls.push_back(availableRerolls.get_size());
				}
				for_count(int, i, 3)
				{
					if (self->executionData->chosenReroll[i].is_set())
					{
#ifdef INVESTIGATE_ISSUES
						output(TXT("[UpgradeMachine] already rerolled reroll %i"), self->executionData->chosenReroll[i].get());
#endif
						availableRerolls.remove_fast(self->executionData->chosenReroll[i].get());
					}
				}
#ifdef INVESTIGATE_ISSUES
				output(TXT("[UpgradeMachine] available to reroll: count %i"), availableRerolls.get_size());
#endif
				if (availableRerolls.get_size() > 0)
				{
					int arIdx = Random::get_int(availableRerolls.get_size());
					int umIdx = availableRerolls[arIdx];
#ifdef INVESTIGATE_ISSUES
					output(TXT("[UpgradeMachine] reroll using %i"), umIdx);
#endif
					self->add_chosen_reroll(umIdx);
					self->issue_choose_unlocks();
				}
			}
		);
	}

	while (true)
	{
		if (playerCheckTimeLeft <= 0.0f)
		{
			MEASURE_PERFORMANCE(playerCheckTimeLeft);
			playerCheckTimeLeft = Random::get_float(0.1f, 0.4f);
			if (imo->get_variables().get_value<bool>(NAME(upgradeMachineShouldRemainDisabled), false))
			{
				for_every(machine, self->executionData->machines)
				{
					machine->shouldBeOn = false;
				}

				playerIn = false;
			}
			else
			{
				bool isPlayerInNow = false;
				{
					//FOR_EVERY_OBJECT_IN_ROOM(object, imo->get_presence()->get_in_room())
					for_every_ptr(object, imo->get_presence()->get_in_room()->get_objects())
					{
						if (object->get_gameplay_as<ModulePilgrim>())
						{
							isPlayerInNow = true;
							break;
						}
					}
				}
				for_every(machine, self->executionData->machines)
				{
					machine->shouldBeOn = isPlayerInNow && (!depleted || self->executionData->chosenOption == machine->machineIdx);
					machine->contentGiven = self->executionData->chosenOption == machine->machineIdx;
				}

				if (playerIn != isPlayerInNow && isPlayerInNow)
				{
					// update for the last time, this is a special case for upgrade machines in simple rules mode where we deplete on the go
					if (auto* piow = PilgrimageInstanceOpenWorld::get())
					{
						if (cellAt.is_set())
						{
							depleted = piow->is_pilgrim_device_state_depleted(cellAt.get(), imo) ||
									   piow->is_pilgrim_device_use_exceeded(cellAt.get(), imo);
						}
					}

					if (!depleted)
					{
						// reevaluate unlocks
						self->issue_choose_unlocks();
					}
				}

				playerIn = isPlayerInNow;
			}
		}
		{
			bool prevDepleted = depleted;
			{
				MEASURE_PERFORMANCE(updateMachines);

				for_every(machine, self->executionData->machines)
				{
					machine->update(REF_ depleted, LATENT_DELTA_TIME);
				}
			}
			if (depleted && !prevDepleted)
			{
				MEASURE_PERFORMANCE(becomeDepleted);
				if (cellAt.is_set())
				{
					if (auto* piow = PilgrimageInstanceOpenWorld::get())
					{
						piow->mark_pilgrim_device_state_depleted(cellAt.get(), imo);
					}
				}
				for_every(machine, self->executionData->machines)
				{
					machine->redrawNow = true;
				}
			}
		}
		if (self->executionData->firstUpgradeMachine)
		{
			if (auto* gd = GameDirector::get())
			{
				gd->set_real_movement_input_tip_blocked(playerIn && ! depleted);
			}
		}
		if (self->manageExitDoor || self->manageEntranceDoor)
		{
			MEASURE_PERFORMANCE(manageDoors);
			auto* exitDoor = self->exitDoor.get();
			auto* entranceDoor = self->entranceDoor.get();
			if (exitDoor && entranceDoor)
			{
				Optional<bool> openEntranceDoor;
				Optional<bool> openExitDoor;
				if (self->entranceDoor == self->exitDoor)
				{
					auto* initialRoomBehindEntranceDoor = self->initialRoomBehindEntranceDoor.get();
					if (depleted && exitDoor->get_room_on_other_side() != initialRoomBehindEntranceDoor)
					{
						if (playerIn)
						{
							openExitDoor = true;
						}
						else if (!exitDoor->get_in_room() || AVOID_CALLING_ACK_ !exitDoor->get_in_room()->was_recently_seen_by_player())
						{
							openExitDoor = false;
						}
					}
					else
					{
						if (playerIn)
						{
							openExitDoor = false; // they're in, close
						}
						else
						{
							openExitDoor = true; // let them in
						}
					}
				}
				else
				{
					if (depleted)
					{
						openEntranceDoor = false;
						if (playerIn)
						{
							openExitDoor = true;
						}
						else if (! exitDoor->get_in_room() || AVOID_CALLING_ACK_ ! exitDoor->get_in_room()->was_recently_seen_by_player())
						{
							openExitDoor = false;
						}
					}
					else
					{
						openExitDoor = false;
						if (!playerIn)
						{
							openEntranceDoor = true;
						}
						else
						{
							openEntranceDoor = false;
						}
					}
				}
				{
					if (entranceDoor != exitDoor &&
						(!entranceDoor->get_room_on_other_side() ||
						 !entranceDoor->get_room_on_other_side()->is_world_active()))
					{
						openEntranceDoor = false;
					}
					if (!exitDoor->get_room_on_other_side() ||
						!exitDoor->get_room_on_other_side()->is_world_active())
					{
						openExitDoor = false;
					}
				}
				if (self->manageEntranceDoor &&
					entranceDoor != exitDoor &&
					openEntranceDoor.is_set())
				{
					if (openEntranceDoor.get())
					{
						entranceDoor->get_door()->set_operation(Framework::DoorOperation::StayOpen);
					}
					else
					{
						entranceDoor->get_door()->set_operation(Framework::DoorOperation::StayClosed);
					}
				}
				if (self->manageExitDoor &&
					openExitDoor.is_set())
				{
					if (openExitDoor.get())
					{
						exitDoor->get_door()->set_operation(Framework::DoorOperation::StayOpen);
					}
					else
					{
						exitDoor->get_door()->set_operation(Framework::DoorOperation::StayClosed);
					}
				}
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

REGISTER_FOR_FAST_CAST(UpgradeMachineData);

bool UpgradeMachineData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	revealSpecialDevicesCellRadius = _node->get_int_attribute_or_from_child(TXT("revealSpecialDevicesCellRadius"), revealSpecialDevicesCellRadius);
	longRangeRevealSpecialDevicesCellRadius = _node->get_int_attribute_or_from_child(TXT("longRangeRevealSpecialDevicesCellRadius"), longRangeRevealSpecialDevicesCellRadius);
	revealSpecialDevicesAmount = _node->get_int_attribute_or_from_child(TXT("revealSpecialDevicesAmount"), revealSpecialDevicesAmount);
	revealSpecialDevicesWithTag = _node->get_name_attribute_or_from_child(TXT("revealSpecialDevicesWithTag"), revealSpecialDevicesWithTag);
	
	revealExitsCellRadius = _node->get_int_attribute_or_from_child(TXT("revealExitsCellRadius"), revealExitsCellRadius);

	autoMapOnlyRevealCellRadius = _node->get_int_attribute_or_from_child(TXT("autoMapOnlyRevealCellRadius"), autoMapOnlyRevealCellRadius);
	autoMapOnlyRevealExitsCellRadius = _node->get_int_attribute_or_from_child(TXT("autoMapOnlyRevealExitsCellRadius"), autoMapOnlyRevealExitsCellRadius);
	autoMapOnlyRevealDevicesCellRadius = _node->get_int_attribute_or_from_child(TXT("autoMapOnlyRevealDevicesCellRadius"), autoMapOnlyRevealDevicesCellRadius);

	portLineModel.load_from_xml(_node, TXT("portLineModel"), _lc);
	canisterLineModel.load_from_xml(_node, TXT("canisterLineModel"), _lc);
	activeEXMLineModel.load_from_xml(_node, TXT("activeEXMLineModel"), _lc);
	passiveEXMLineModel.load_from_xml(_node, TXT("passiveEXMLineModel"), _lc);

	chanceOf1Extra = _node->get_float_attribute_or_from_child(TXT("chanceOf1Extra"), chanceOf1Extra);
	chanceOf2Extras = _node->get_float_attribute_or_from_child(TXT("chanceOf2Extras"), chanceOf2Extras);
	chanceOf3Extras = _node->get_float_attribute_or_from_child(TXT("chanceOf3Extras"), chanceOf3Extras);

	upgradesTagged.load_from_xml_attribute_or_child_node(_node, TXT("upgradesTagged"));
	extraUpgradesTagged.load_from_xml_attribute_or_child_node(_node, TXT("extraUpgradesTagged"));

	for_every(node, _node->children_named(TXT("upgradeMachineSets")))
	{
		for_every(umsNode, node->children_named(TXT("for")))
		{
			UpgradeMachineSets ums;
			ums.forGameDefinitionTagged.load_from_xml_attribute_or_child_node(umsNode, TXT("forGameDefinitionTagged"));
			for_every(setNode, umsNode->children_named(TXT("set")))
			{
				UpgradeMachineSets::Set umSet;
				for_every(n, setNode->children_named(TXT("exm")))
				{
					Name exmId = n->get_name_attribute_or_from_child(TXT("id"), Name::invalid());
					if (exmId.is_valid())
					{
						umSet.exms.push_back_unique(exmId);
					}
					else
					{
						error_loading_xml(n, TXT("no exm id"));
						result = false;
					}
				}
				ums.sets.push_back(umSet);
			}
			upgradeMachineSets.push_back(ums);
		}
	}

	return result;
}

bool UpgradeMachineData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= portLineModel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= canisterLineModel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= activeEXMLineModel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= passiveEXMLineModel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

//

UpgradeMachine::Machine::~Machine()
{
	if (display && displayOwner.get())
	{
		display->use_background(nullptr);
		display->set_on_update_display(this, nullptr);
	}
}

void UpgradeMachine::Machine::update_reroll_emissive(bool _on, bool _force)
{
	if (rerollEmissive == _on && !_force)
	{
		return;
	}
	rerollEmissive = _on;
	for_every_ref(s, rerollHandler.get_switches())
	{
		if (auto* ec = s->get_custom<CustomModules::EmissiveControl>())
		{
			ec->emissive_deactivate_all();
			bool isActive = rerollEmissive;
			if (isActive)
			{
				for_count(int, i, 3)
				{
					if (upgradeMachine->executionData->chosenReroll[i].get(-1) == machineIdx)
					{
						isActive = false;
					}
				}
				if (isActive)
				{
					ec->emissive_activate(NAME(bad));
				}
			}
		}
	}
}

void UpgradeMachine::Machine::reset_display()
{
	if (display && displayOwner.get())
	{
		display->use_background(nullptr);
		display->set_on_update_display(this, nullptr);
	}
}

void UpgradeMachine::Machine::update(REF_ bool & _depleted, float _deltaTime)
{
	/*
	if (_depleted)
	{
		for_every_ref(s, canisterHandler.get_switches())
		{
			if (auto* g = s->get_custom<CustomModules::Grabable>())
			{
				g->set_force_release();
			}
		}
	}
	*/
	if (state == MachineOff ||
		state == MachineStart ||
		state == MachineShutDown ||
		_depleted)
	{
		canister.set_active(false);
		canister.advance(_deltaTime);
		canister.force_emissive(contentGiven? UpgradeCanister::EmissiveUsed : UpgradeCanister::EmissiveOff);
	}
	else 
	{
		canister.set_active(true);
		canister.advance(_deltaTime);
		canister.force_emissive(NP);
		if (rerollHandler.get_switch_at() >= 1.0f)
		{
			bool isActive = true;
			for_count(int, i, 3)
			{
				if (upgradeMachine->executionData->chosenReroll[i].get(-1) == machineIdx)
				{
					isActive = false;
				}
			}
			if (isActive)
			{
				upgradeMachine->add_chosen_reroll(machineIdx);
				upgradeMachine->issue_choose_unlocks();
			}
		}

		if (canister.should_give_content())
		{
			Machine::GiveRewardContext giveRewardContext;
			if (give_reward(REF_ giveRewardContext))
			{
				// off
				_depleted = true;
				contentGiven = true;
				if (upgradeMachine->executionData->firstUpgradeMachine)
				{
					MusicPlayer::request_incidental(NAME(mt_firstUpgradeMachine));
				}
				else
				{
					{
						Energy experience = GameplayBalance::upgrade_machine__xp();
						if (experience.is_positive())
						{
							experience = experience.adjusted_plus_one(GameSettings::get().experienceModifier);
							PlayerSetup::access_current().stats__experience(experience);
							GameStats::get().add_experience(experience);
							Persistence::access_current().provide_experience(experience);
						}
					}

					// this is to disallow unlocking to get xp
					if (auto* piow = PilgrimageInstanceOpenWorld::get())
					{
						Optional<VectorInt2> cellAt = piow->find_cell_at(upgradeMachine->get_imo());
						if (cellAt.is_set())
						{
							piow->set_last_visited_haven(cellAt.get());
						}
					}
					output(TXT("store game state - upgrade received"));
					Game::get_as<Game>()->add_async_store_game_state(true, GameStateLevel::Checkpoint);
				}
				if (auto* pi = PilgrimageInstance::get())
				{
					if (upgradeMachine->executionData->firstUpgradeMachine)
					{
#ifdef INVESTIGATE_ISSUES
						output(TXT("[UpgradeMachine] unlocked first upgrade machine set"));
#endif
						pi->set_unlocked_upgrade_machines_to(1);
					}
					else
					{
#ifdef INVESTIGATE_ISSUES
						output(TXT("[UpgradeMachine] unlocked next upgrade machine set (%i)"), pi->get_unlocked_upgrade_machines() + 1);
#endif
						pi->set_unlocked_upgrade_machines_to(pi->get_unlocked_upgrade_machines() + 1);
					}
				}
				if (auto* mp = giveRewardContext.showMapTo)
				{
					auto& poi = mp->access_overlay_info();
					upgradeMachine->executionData->showUpgradeSelection = false;

					if (!GameDirector::get_active() ||
						!GameDirector::get_active()->is_reveal_map_on_upgrade_blocked())
					{
						poi.new_map_available(true, true);
					}
					poi.clear_map_highlights();
					for_every(sd, giveRewardContext.revealedSpecialDevices)
					{
						poi.highlight_map_at(sd->to_vector_int3());
					}
				}
				if (!upgradeMachine->markedAsKnownForOpenWorldDirection)
				{
					if (auto* piow = PilgrimageInstanceOpenWorld::get())
					{
						piow->mark_pilgrimage_device_direction_known(upgradeMachine->get_imo());
						upgradeMachine->markedAsKnownForOpenWorldDirection = true;
					}
				}
				if (auto* mp = giveRewardContext.showMapTo)
				{
					if (!GameSettings::get().difficulty.showDirectionsOnlyToObjective)
					{
						if (upgradeMachine->executionData->activateNavigation)
						{
							auto& poi = mp->access_overlay_info();
							poi.speak(upgradeMachine->executionData->activateNavigation_line.get());
						}
					}
				}
			}
		}

		update_reroll_emissive(true);
	}
}

bool UpgradeMachine::Machine::give_reward(REF_ GiveRewardContext& _context)
{
	_context.showMapTo = nullptr;
	if (! canister.has_content())
	{
		// nothing to give
		return false;
	}
	Concurrency::ScopedSpinLock lock(upgradeMachine->rewardGivenLock);
	if (upgradeMachine->rewardGiven.get() > 0)
	{
		// already given
		return false;
	}
	bool given = false;
	todo_multiplayer_issue(TXT("we just get player here"));
	if (auto* pa = canister.get_held_by_owner())
	{
		auto* mp = pa->get_gameplay_as<ModulePilgrim>();

		if (upgradeMachine->executionData->doAnalysingSequence)
		{
			mp->access_overlay_info().clear_main_log();
		}

		upgradeMachine->executionData->showUpgradeSelection = false;

		given = canister.give_content(pa, true);

		if (given)
		{
			upgradeMachine->rewardGiven.increase();

			int revealSpecialDevicesAmount = upgradeMachine->get_imo()->get_value<int>(NAME(revealSpecialDevicesAmount), upgradeMachine->upgradeMachineData->revealSpecialDevicesAmount);
			Name revealSpecialDevicesWithTag = upgradeMachine->get_imo()->get_value<Name>(NAME(revealSpecialDevicesWithTag), upgradeMachine->upgradeMachineData->revealSpecialDevicesWithTag);
			if (!GameSettings::get().difficulty.revealDevicesOnUpgrade)
			{
				revealSpecialDevicesAmount = 0;
				revealSpecialDevicesWithTag = Name::invalid();
			}
			{
				bool showMap = true; // always show?
				if (auto* piow = PilgrimageInstanceOpenWorld::get())
				{
					Optional<VectorInt2> cellAt = piow->find_cell_at(upgradeMachine->get_imo());
					if (cellAt.is_set())
					{
						bool longRangeRules = false;
						if ((mp && mp->has_exm_equipped(EXMID::Passive::long_range_automap())) ||
							(canister.content.exm.is_set() && canister.content.exm->get_id() == EXMID::Passive::long_range_automap()))
						{
							longRangeRules = true;
						}

						int radius = upgradeMachine->upgradeMachineData->revealSpecialDevicesCellRadius;
						if (longRangeRules)
						{
							radius = upgradeMachine->upgradeMachineData->longRangeRevealSpecialDevicesCellRadius;
						}
						int knownExitsRadius = upgradeMachine->upgradeMachineData->revealExitsCellRadius;

						showMap |= piow->mark_cell_known_special_devices(cellAt.get(), radius, revealSpecialDevicesAmount, revealSpecialDevicesWithTag, &_context.revealedSpecialDevices);

						if (piow->get_special_rules().upgradeMachinesProvideMap)
						{
							if (!GameSettings::get().difficulty.revealDevicesOnUpgrade)
							{
								showMap = true; // always
							}

							if (longRangeRules)
							{
								if (GameSettings::get().difficulty.autoMapOnly)
								{
									if (int r = upgradeMachine->upgradeMachineData->autoMapOnlyRevealCellRadius)
									{
										piow->mark_cell_known(cellAt.get(), r + 2);
									}
									if (int r = upgradeMachine->upgradeMachineData->autoMapOnlyRevealExitsCellRadius)
									{
										piow->mark_cell_known_exits(cellAt.get(), r + 2);
									}
									if (int r = upgradeMachine->upgradeMachineData->autoMapOnlyRevealDevicesCellRadius)
									{
										piow->mark_cell_known_devices(cellAt.get(), r + 2);
									}
								}
								else if (knownExitsRadius > 0)
								{
									piow->mark_cell_known_exits(cellAt.get(), knownExitsRadius + 4);
									piow->mark_cell_known_devices(cellAt.get(), knownExitsRadius + 2);
								}
							}
							else
							{
								if (GameSettings::get().difficulty.autoMapOnly)
								{
									if (int r = upgradeMachine->upgradeMachineData->autoMapOnlyRevealCellRadius)
									{
										piow->mark_cell_known(cellAt.get(), r);
									}
									if (int r = upgradeMachine->upgradeMachineData->autoMapOnlyRevealExitsCellRadius)
									{
										piow->mark_cell_known_exits(cellAt.get(), r);
									}
									if (int r = upgradeMachine->upgradeMachineData->autoMapOnlyRevealDevicesCellRadius)
									{
										piow->mark_cell_known_devices(cellAt.get(), r);
									}
								}
								else if (knownExitsRadius > 0)
								{
									piow->mark_cell_known_exits(cellAt.get(), knownExitsRadius);
								}
							}
						}
					}
				}
				if (GameSettings::get().difficulty.displayMapOnUpgrade &&
					mp)
				{
					if (showMap)
					{
						_context.showMapTo = mp;
					}
				}
			}
			Framework::GameScript::ScriptExecution::trigger_execution_trap(NAME(upgradeMachineRewardGiven));
		}
	}

	if (given)
	{
		upgradeMachine->set_chosen_option(machineIdx);
	}

	return given;
}
