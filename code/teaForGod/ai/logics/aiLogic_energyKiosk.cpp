#include "aiLogic_energyKiosk.h"

#include "exms\aiLogic_exmEnergyDispatcher.h"

#include "..\..\game\game.h"
#include "..\..\game\gameLog.h"

#include "..\..\modules\energyTransfer.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\gameplay\modulePilgrimHand.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_pickup.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\appearance\controllers\ac_particlesUtils.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleGameplay.h"
#include "..\..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\module\custom\mc_display.h"
#include "..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\framework\world\room.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\performance\performanceUtils.h"
#include "..\..\..\core\system\video\video3dPrimitives.h"

#ifdef AN_CLANG
#include "..\..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
//#define LOG_ENERGY_KIOSK
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// variables
DEFINE_STATIC_NAME(leftSliderInteractiveDeviceId);
DEFINE_STATIC_NAME(rightSliderInteractiveDeviceId);
DEFINE_STATIC_NAME(bayRadius);
DEFINE_STATIC_NAME(bayRadiusDetect);
DEFINE_STATIC_NAME(energy);
DEFINE_STATIC_NAME(storage);
DEFINE_STATIC_NAME(maxEnergy);
DEFINE_STATIC_NAME(maxStorage);
DEFINE_STATIC_NAME(leftTransferSpeed);
DEFINE_STATIC_NAME(rightTransferSpeed);
DEFINE_STATIC_NAME(energyInStorage); // pilgrimage device store variable, used also by other devices (lock imo if using)

// ai messages
DEFINE_STATIC_NAME(forceEnergyKioskOff);

// sockets
DEFINE_STATIC_NAME(bay);
DEFINE_STATIC_NAME(inBayEnergy); // also pilgrim overlay info
DEFINE_STATIC_NAME(energyKioskEnergy); // also pilgrim overlay info

// sounds 
DEFINE_STATIC_NAME(transfer);
DEFINE_STATIC_NAME_STR(depositEmpty, TXT("deposit empty"));
DEFINE_STATIC_NAME_STR(depositFull, TXT("deposit full"));
DEFINE_STATIC_NAME_STR(withdrawEmpty, TXT("withdraw empty"));
DEFINE_STATIC_NAME_STR(withdrawFull, TXT("withdraw full"));

// temporary objects
DEFINE_STATIC_NAME_STR(leftDeposit, TXT("left deposit"));
DEFINE_STATIC_NAME_STR(leftWithdraw, TXT("left withdraw"));
DEFINE_STATIC_NAME_STR(rightDeposit, TXT("right deposit"));
DEFINE_STATIC_NAME_STR(rightWithdraw, TXT("right withdraw"));

// tags
DEFINE_STATIC_NAME(hand);

// emissive layers
DEFINE_STATIC_NAME(active);
DEFINE_STATIC_NAME_STR(wrongItem, TXT("wrong item"));
DEFINE_STATIC_NAME(empty);
DEFINE_STATIC_NAME(left);
DEFINE_STATIC_NAME(right);
DEFINE_STATIC_NAME_STR(leftRequiresNeutral, TXT("left requires neutral"));
DEFINE_STATIC_NAME_STR(rightRequiresNeutral, TXT("right requires neutral"));

//

void EnergyInStorageDisplayData::update(int _maxAmount, float _fullPt, int _minUsed, int _maxMoveLines)
{
	while (used.get_size() + notUsed.get_size() < _maxAmount)
	{
		notUsed.push_back(used.get_size() + notUsed.get_size());
	}

	int total = _maxAmount;

	int moveLines = min(_maxMoveLines, (Random::get_chance(0.2f) ? 1 : 0) + (int)((float)total * _fullPt * 0.1f));

	int shouldBeUsed = max(max(0, _minUsed), TypeConversions::Normal::f_i_closest((float)total * _fullPt));

	while ((moveLines > 0 && used.get_size() > 0) || used.get_size() > shouldBeUsed)
	{
		int i = Random::get_int(used.get_size());
		int u = used[i];
		used.remove_fast_at(i);
		notUsed.push_back(u);
		--moveLines;
	}

	while (used.get_size() < shouldBeUsed && notUsed.get_size() > 0)
	{
		int i = Random::get_int(notUsed.get_size());
		int u = notUsed[i];
		notUsed.remove_fast_at(i);
		used.push_back(u);
	}
}

//

REGISTER_FOR_FAST_CAST(EnergyKiosk);

EnergyKiosk::EnergyKiosk(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	energyKioskData = fast_cast<EnergyKioskData>(_logicData);

	left.energyType = energyKioskData->leftEnergyType;
	right.energyType = energyKioskData->rightEnergyType;

	left.transferSpeed = Energy(10);
	right.transferSpeed = Energy(10);
}

EnergyKiosk::~EnergyKiosk()
{
	if (display)
	{
		display->use_background(nullptr);
		display->set_on_update_display(this, nullptr);
	}
}

void EnergyKiosk::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	machineIsOn = blend_to_using_speed_based_on_time(machineIsOn, machineIsOnTarget, machineIsOn < machineIsOnTarget ? 1.5f : 0.6f, _deltaTime);

	timeToUpdateDisplayLeft -= _deltaTime;
	if (timeToUpdateDisplayLeft < 0.0f)
	{
		updateDisplay = true;
		timeToUpdateDisplayLeft = 0.4f;
		displayMoveLines = 1;

		if (left.objectInBay.is_active() || right.objectInBay.is_active())
		{
			bool goodItem = true;
			if (left.objectInBay.is_active() && !left.goodToTransfer)
			{
				goodItem = false;
			}
			else if (right.objectInBay.is_active() && !right.goodToTransfer)
			{
				goodItem = false;
			}
			if (goodItem)
			{
				timeToUpdateDisplayLeft = 0.1f;
				displayMoveLines = 2;
			}
		}
	}

	{
		bool showKioskEnergy = left.objectInBay.get_target() || right.objectInBay.get_target();
		bool showInBayEnergy = left.switchHandler.is_grabbed() || right.switchHandler.is_grabbed();
		bool updateShowingForPilgrim = false;
		// hide (and check if we should udpate pilgrim)
		{
			struct HideUtils
			{
				static bool hide(bool _show, Optional<Energy>& _showing, ModulePilgrim* showingForPilgrim, Name const& _socketInfoName)
				{
					if (_show ^ _showing.is_set())
					{
						if (!_show)
						{
							if (showingForPilgrim)
							{
#ifdef LOG_ENERGY_KIOSK
								output(TXT("[EnergyKiosk] hide overlay info"));
#endif
								showingForPilgrim->access_overlay_info().hide_show_info(_socketInfoName);
							}
							_showing.clear();
						}
						return true;
					}
					return false;
				}
			};
			updateShowingForPilgrim |= HideUtils::hide(showKioskEnergy, showingKioskEnergy, showingForPilgrim, NAME(energyKioskEnergy));
			updateShowingForPilgrim |= HideUtils::hide(showInBayEnergy, showingInBayEnergy, showingForPilgrim, NAME(inBayEnergy));
		}

		if (updateShowingForPilgrim)
		{
			bool requiresPilgrim = showKioskEnergy || showInBayEnergy;

#ifdef LOG_ENERGY_KIOSK
			output(TXT("[EnergyKiosk] update"));
			output(TXT("[EnergyKiosk] left.objectInBay = %S"), left.objectInBay.get_target()? left.objectInBay.get_target()->ai_get_name().to_char() : TXT("--"));
			output(TXT("[EnergyKiosk] right.objectInBay = %S"), right.objectInBay.get_target()? right.objectInBay.get_target()->ai_get_name().to_char() : TXT("--"));
			output(TXT("[EnergyKiosk] left.switchHandler.is_grabbed() = %S"), left.switchHandler.is_grabbed()? TXT("GRABBED") : TXT("no"));
			output(TXT("[EnergyKiosk] right.switchHandler.is_grabbed() = %S"), right.switchHandler.is_grabbed()? TXT("GRABBED") : TXT("no"));
#endif
			if (requiresPilgrim)
			{
				if (!showingForPilgrim)
				{
					todo_multiplayer_issue(TXT("we just get player here, we should get all players?"));
					showingForPilgrim = nullptr;
					if (auto* g = Game::get_as<Game>())
					{
						if (auto* pa = g->access_player().get_actor())
						{
							showingForPilgrim = pa->get_gameplay_as<ModulePilgrim>();
						}
					}
				}
			}
			else
			{
				showingForPilgrim = nullptr;
			}
		}

		// show
		{
			struct ShowUtils
			{
				static void show(bool _show, Optional<Energy>& _showing, ModulePilgrim* showingForPilgrim, Framework::AI::MindInstance * mind, Name const & _socketInfoName, float _size)
				{
					if (_show ^ _showing.is_set())
					{
						if (_show && showingForPilgrim)
						{
							if (auto* imo = mind->get_owner_as_modules_owner())
							{
								if (auto* imoAp = imo->get_appearance())
								{
									Transform at = imoAp->get_os_to_ws_transform().to_world(imoAp->calculate_socket_os(_socketInfoName));

									if (!showingForPilgrim->access_overlay_info().is_showing_info(_socketInfoName))
									{
#ifdef LOG_ENERGY_KIOSK
										output(TXT("[EnergyKiosk] show overlay info"));
#endif
										showingForPilgrim->access_overlay_info().show_info(_socketInfoName, at, String::empty(), NP, _size, NP, false);
									}

									_showing = Energy(-1);
								}
							}
						}
					}
				}
			};
			ShowUtils::show(showKioskEnergy, showingKioskEnergy, showingForPilgrim, get_mind(), NAME(energyKioskEnergy), 0.03f);
			ShowUtils::show(showInBayEnergy, showingInBayEnergy, showingForPilgrim, get_mind(), NAME(inBayEnergy), 0.04f);
		}

		// update
		{
			if (showingKioskEnergy.is_set() && showingKioskEnergy.get() != energy)
			{
				if (showingForPilgrim)
				{
					String maxEnergyStr = maxEnergy.as_string(0);
					String energyStr = energy.as_string(0).pad_left_with(maxEnergyStr.get_length(), '0');
					if (showingForPilgrim->access_overlay_info().update_info(NAME(energyKioskEnergy), energyStr + TXT("/") + maxEnergyStr))
					{
#ifdef LOG_ENERGY_KIOSK
						output(TXT("[EnergyKiosk] update energy kiosk energy"));
#endif
						showingKioskEnergy = energy;
					}
				}
			}
			if (showingInBayEnergy.is_set())
			{
				Energy showEnergy = Energy::zero();
				if (left.switchHandler.is_grabbed())
				{
					showEnergy = get_available_energy_for(left);
				}
				else if (right.switchHandler.is_grabbed())
				{
					showEnergy = get_available_energy_for(right);
				}
				if (showingInBayEnergy.get() != showEnergy)
				{
					if (showingForPilgrim)
					{
						String showEnergyStr = showEnergy.as_string(0).pad_left_with(3, '0');
						if (showingForPilgrim->access_overlay_info().update_info(NAME(inBayEnergy), showEnergyStr))
						{
#ifdef LOG_ENERGY_KIOSK
							output(TXT("[EnergyKiosk] update in bay energy"));
#endif
							showingInBayEnergy = showEnergy;
						}
					}
				}
			}
		}
	}
}

void EnergyKiosk::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	display = nullptr;
	updateDisplay = true;
	if (auto* mind = get_mind())
	{
		if (auto* imo = mind->get_owner_as_modules_owner())
		{
			if (auto* displayModule = imo->get_custom<::Framework::CustomModules::Display>())
			{
				display = displayModule->get_display();

				if (display)
				{
					display->set_on_update_display(this,
						[this](Framework::Display* _display)
						{
							if (!updateDisplay)
							{
								return;
							}
							updateDisplay = false;
							_display->drop_all_draw_commands();
							_display->add((new Framework::DisplayDrawCommands::CLS(Colour::black)));
							_display->add((new Framework::DisplayDrawCommands::Border(Colour::black)));

							{
								int minDraw = energy.is_positive() && machineIsOnTarget > 0.0f ? 1 : 0;
								float amountPt = energy.div_to_float(maxEnergy);

								int totalLines = TypeConversions::Normal::f_i_closest(_display->get_right_top_of_screen().x - _display->get_left_bottom_of_screen().x) + 1;
								displayData.update(totalLines, amountPt * machineIsOn, minDraw, displayMoveLines);
								displayMoveLines = 0;
							}

							Colour ink = Colour::white;
							if (energyKioskData)
							{
								if (left.objectInBay.is_active() || right.objectInBay.is_active())
								{
									bool goodItem = true;
									if (left.objectInBay.is_active() && !left.goodToTransfer)
									{
										goodItem = false;
									}
									else if (right.objectInBay.is_active() && !right.goodToTransfer)
									{
										goodItem = false;
									}
									ink = goodItem ? energyKioskData->displayActiveColour : energyKioskData->displayWrongItemColour;
								}
								else
								{
									ink = energyKioskData->displayInactiveColour;
								}
							}
							_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
								[ink, this](Framework::Display* _display, ::System::Video3D* _v3d)
								{
									Vector2 a(_display->get_left_bottom_of_screen());
									Vector2 b(_display->get_right_top_of_screen());
									for_every(u, displayData.used)
									{
										int x = *u;
										a.x = _display->get_left_bottom_of_screen().x + (float)x;
										b.x = a.x;
										::System::Video3DPrimitives::line_2d(ink, a, b, false);
									}
								}));
							_display->draw_all_commands_immediately();
						});
				}
			}
		}
	}

	energy = Energy::get_from_storage(_parameters, NAME(energy), energy);
	energy = Energy::get_from_storage(_parameters, NAME(storage), energy);
	maxEnergy = Energy::get_from_storage(_parameters, NAME(maxEnergy), maxEnergy);
	maxEnergy = Energy::get_from_storage(_parameters, NAME(maxStorage), maxEnergy);
	if (!maxEnergy.is_zero())
	{
		maxEnergy = max(maxEnergy, energy);
	}
	left.transferSpeed = Energy::get_from_storage(_parameters, NAME(leftTransferSpeed), left.transferSpeed);
	right.transferSpeed = Energy::get_from_storage(_parameters, NAME(rightTransferSpeed), right.transferSpeed);

	if (auto* mind = get_mind())
	{
		if (auto* imo = mind->get_owner_as_modules_owner())
		{
			imo->access_variables().access<Energy>(NAME(energyInStorage)) = energy;
		}
	}
}

Energy EnergyKiosk::get_available_energy_for(EnergyHandling & _energyHandling)
{
	auto* object = _energyHandling.objectInBay.get_target();
	if (!object)
	{
		return Energy::zero();
	}
	if (auto* it = IEnergyTransfer::get_energy_transfer_module_from(object, _energyHandling.energyType))
	{
		return it->calculate_total_energy_available(_energyHandling.energyType);
	}
	return Energy::zero();
}

Optional<int> EnergyKiosk::determine_priority(EnergyHandling * _energyHandling, ::Framework::IModulesOwner* _object)
{
	if (!_object)
	{
		return NP;
	}
	if (auto* it = IEnergyTransfer::get_energy_transfer_module_from(_object, _energyHandling->energyType))
	{
		if (_energyHandling->energyType == EnergyType::Ammo)
		{
			EnergyTransferRequest etr(EnergyTransferRequest::Query);
			if (it->handle_ammo_energy_transfer_request(etr))
			{
				return etr.priority;
			}
		}
		else if (_energyHandling->energyType == EnergyType::Health)
		{
			EnergyTransferRequest etr(EnergyTransferRequest::Query);
			if (it->handle_health_energy_transfer_request(etr))
			{
				return etr.priority;
			}
		}
	}
	return NP;
}

void EnergyKiosk::handle_switch_and_transfer_energy(EnergyHandling * _energyHandling, float _deltaTime, ::Framework::IModulesOwner* imo)
{
	_energyHandling->switchHandler.advance(_deltaTime);

	float const threshold = 0.333f;
	float switchOutputF = _energyHandling->switchHandler.get_output();
	int switchOutput = switchOutputF >= threshold ? 1 : (switchOutputF <= -threshold ? -1 : 0);
	float transferSpeed = sqrt(Range::transform_clamp(abs(switchOutputF), Range(threshold, 1.0f), Range(0.0f, 1.0f)));

	/*
	debug_context(imo->get_presence()->get_in_room());
	auto* imoAp = imo->get_appearance();
	Vector3 bayLoc = imoAp->get_os_to_ws_transform().location_to_world(imoAp->calculate_socket_os(baySocket.get_index()).get_translation());
	debug_draw_text(true, Colour::white, bayLoc + Vector3::zAxis * (_energyHandling == &left? 0.1f : -0.1f), Vector2::half, true, 0.8f, false, TXT("%.3f -> %.3f"), switchOutputF, transferSpeed);
	debug_no_context();
	*/

	EnergyTransfer prevEnergyTransfer = energyTransfer;

	if (transferSpeed == 0.0f)
	{
		_energyHandling->requiresNeutral = false;
		_energyHandling->timeToNextTransfer = 0.0f;
		if (activeTransfer == _energyHandling)
		{
			activeTransfer = nullptr;
			energyTransfer = EnergyTransfer::None;
		}
	}
	else if (!activeTransfer || activeTransfer == _energyHandling)
	{
		if (!_energyHandling->requiresNeutral && _energyHandling->objectInBay.is_active())
		{
			_energyHandling->timeToNextTransfer -= _deltaTime * transferSpeed;
			Energy energyToTransfer = Energy(0);
			while (_energyHandling->timeToNextTransfer <= 0.0f)
			{
				_energyHandling->timeToNextTransfer += 1.0f / (_energyHandling->transferSpeed.as_float());
				energyToTransfer += Energy(1);
			}
			if (!energyToTransfer.is_zero())
			{
				if (auto* it = IEnergyTransfer::get_energy_transfer_module_from(_energyHandling->objectInBay.get_target(), _energyHandling->energyType))
				{
					if (!markedAsKnownForOpenWorldDirection)
					{
						if (auto* piow = PilgrimageInstanceOpenWorld::get())
						{
							piow->mark_pilgrimage_device_direction_known(get_mind()->get_owner_as_modules_owner());
							markedAsKnownForOpenWorldDirection = true;
						}
					}

					EnergyTransferRequest etr(switchOutput == DEPOSIT_IN_OBJECT ? EnergyTransferRequest::Deposit : EnergyTransferRequest::Withdraw);
					etr.energyRequested = energyToTransfer;
					if (switchOutput == DEPOSIT_IN_OBJECT)
					{
						etr.energyRequested = min(etr.energyRequested, energy);
					}
					else if (maxEnergy.is_positive())
					{
						etr.energyRequested = min(etr.energyRequested, maxEnergy - energy);
					}
					if (!etr.energyRequested.is_zero())
					{
						bool success = false;
						for_count(int, transferIdx, 2)
						{
							success = false;
							if (_energyHandling->energyType == EnergyType::Ammo)
							{
								success = it->handle_ammo_energy_transfer_request(etr);
							}
							else if (_energyHandling->energyType == EnergyType::Health)
							{
								success = it->handle_health_energy_transfer_request(etr);
							}
							if (transferIdx == 0 && etr.energyResult.is_zero())
							{
								// transferred nothing, check if we can redistribute energy we could transfer
								Energy toRedistribute = etr.energyRequested;
								Energy redistributed = AI::Logics::EXMEnergyDispatcher::redistribute_energy_from(it, _energyHandling->energyType, toRedistribute);
								if (redistributed.is_positive())
								{
									// redisitributed, try again
									continue;
								}
							}
							break;
						}
						if (etr.energyResult.is_zero() || !success)
						{
							if (energyTransfer != EnergyTransfer::None)
							{
								ai_log(this, switchOutput == DEPOSIT_IN_KIOSK ? TXT("deposit full") : TXT("withdraw empty"));
								if (auto* s = imo->get_sound())
								{
									s->play_sound(switchOutput == DEPOSIT_IN_KIOSK ? NAME(depositFull) : NAME(withdrawEmpty));
								}
							}
							energyTransfer = EnergyTransfer::None;
						}
						else if (success)
						{
							if (!activeTransfer)
							{
								if (Framework::GameUtils::is_local_player(_energyHandling->objectInBay.get_target()->get_top_instigator()))
								{
									if (switchOutput == DEPOSIT_IN_OBJECT)
									{
										GameLog::get().increase_counter(GameLog::TransferKioskDeposit);
									}
									else
									{
										GameLog::get().increase_counter(GameLog::TransferKioskWithdraw);
									}
								}
							}
							activeTransfer = _energyHandling;
							if (switchOutput == DEPOSIT_IN_OBJECT)
							{
								energy -= etr.energyResult;
							}
							else
							{
								energy += etr.energyResult;
							}
							if (switchOutput == DEPOSIT_IN_KIOSK)
							{
								energyTransfer = _energyHandling == &left ? EnergyTransfer::LeftDeposit : EnergyTransfer::RightDeposit;
							}
							else
							{
								energyTransfer = _energyHandling == &left ? EnergyTransfer::LeftWithdraw : EnergyTransfer::RightWithdraw;
							}
						}
					}
					else
					{
						if (energyTransfer != EnergyTransfer::None)
						{
							ai_log(this, switchOutput == DEPOSIT_IN_KIOSK ? TXT("deposit empty") : TXT("withdraw full"));
							if (auto* s = imo->get_sound())
							{
								s->play_sound(switchOutput == DEPOSIT_IN_KIOSK ? NAME(depositEmpty) : NAME(withdrawFull));
							}
						}
						energyTransfer = EnergyTransfer::None;
					}
				}
				else
				{
					energyTransfer = EnergyTransfer::None;
				}
			}
		}
		else
		{
			energyTransfer = EnergyTransfer::None;
		}
	}

	if (prevEnergyTransfer != energyTransfer)
	{
		if (auto* s = imo->get_sound())
		{
			s->stop_sound(NAME(transfer));
			if (energyTransfer != EnergyTransfer::None)
			{
				s->play_sound(NAME(transfer));
			}
		}
		energyTransferPhysSens.update(energyTransfer != EnergyTransfer::None, _energyHandling->objectInBay.get_target());
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

Name EnergyKiosk::get_energy_transfer_temporary_object_name(EnergyTransfer _energyTransfer)
{
	switch (_energyTransfer)
	{
	case EnergyTransfer::LeftDeposit: return NAME(leftDeposit);
	case EnergyTransfer::LeftWithdraw: return NAME(leftWithdraw);
	case EnergyTransfer::RightDeposit: return NAME(rightDeposit);
	case EnergyTransfer::RightWithdraw: return NAME(rightWithdraw);
	case EnergyTransfer::None: break;
	}
	return Name::invalid();
}

void EnergyKiosk::handle_energy_transfer_temporary_objects(EnergyTransfer _prevEnergyTransfer, EnergyTransfer _energyTransfer, ::Framework::IModulesOwner* imo)
{
	if (_prevEnergyTransfer == _energyTransfer)
	{
		return;
	}

	ai_log(this, TXT("started %S (%S) a:%.3f h:%.3f"), get_energy_transfer_temporary_object_name(_energyTransfer).to_char(), get_energy_transfer_temporary_object_name(_prevEnergyTransfer).to_char(), left.switchHandler.get_output(), right.switchHandler.get_output());

	for_every_ref(o, energyTransferTemporaryObjects)
	{
		if (auto* to = fast_cast<Framework::TemporaryObject>(o))
		{
			Framework::ParticlesUtils::desire_to_deactivate(to);
		}
	}
	energyTransferTemporaryObjects.clear();

	if (_energyTransfer != EnergyTransfer::None)
	{
		if (auto * to = imo->get_temporary_objects())
		{
			to->spawn_all(get_energy_transfer_temporary_object_name(_energyTransfer), NP, &energyTransferTemporaryObjects);
		}
	}
}

void EnergyKiosk::update_object_in_bay(EnergyHandling * _energyHandling, ::Framework::IModulesOwner* imo)
{
	auto* imoAp = imo->get_appearance();
	if (!imoAp)
	{
		return;
	}

	debug_context(imo->get_presence()->get_in_room());
	debug_filter(ai_energyKiosk);

	float const bayRadiusSq = sqr(bayRadius);
	Vector3 bayLoc = imoAp->get_os_to_ws_transform().location_to_world(imoAp->calculate_socket_os(baySocket.get_index()).get_translation());
	float inBayDist = 0.0f;
	
	::Framework::RelativeToPresencePlacement & objectInBay = _energyHandling->objectInBay;

	if (!objectInBay.is_active() &&
		_energyHandling->objectInBayWasSet)
	{
		_energyHandling->requiresNeutral = true;
	}

	todo_multiplayer_issue(TXT("not checking which player has seen it, any does?"));
	// allow for object in bay only if is in a room that has been recently seen by a player

	if (auto* ib = objectInBay.get_target())
	{
		if (ib->get_presence()->get_in_room() != imo->get_presence()->get_in_room() ||
			! imo->get_presence()->get_in_room() ||
			! imo->get_presence()->get_in_room()->was_recently_seen_by_player())
		{
			objectInBay.clear_target();
			_energyHandling->requiresNeutral = true;
			updateDisplay = true;
		}
		else
		{
			float inBayDist = (bayLoc - ib->get_presence()->get_centre_of_presence_WS()).length();
			if (inBayDist > bayRadius * hardcoded magic_number 1.2f)
			{
				objectInBay.clear_target();
				_energyHandling->requiresNeutral = true;
				updateDisplay = true;
			}
		}
	}

	float bestDistSq;
	Framework::IModulesOwner* bestImo = nullptr;
	Optional<int> bestKindPriority;

	// we can check just our room and just objects, don't check presence links
	if (machineIsOnTarget > 0.0f &&
		(imo->get_presence()->get_in_room() && imo->get_presence()->get_in_room()->was_recently_seen_by_player()))
	{
		Framework::IModulesOwner* inBay = objectInBay.get_target();
		//FOR_EVERY_OBJECT_IN_ROOM(o, imo->get_presence()->get_in_room())
		for_every_ptr(o, imo->get_presence()->get_in_room()->get_objects())
		{
			if (o != inBay &&
				o != imo) // it doesn't make much sense to include itself
			{
				if (auto* op = o->get_presence())
				{
					Vector3 oCentre = op->get_centre_of_presence_WS();
					float distSq = (bayLoc - oCentre).length_squared();
					debug_draw_arrow(true, distSq < bayRadiusSq? Colour::green : Colour::red, bayLoc, oCentre);
					if (distSq < bayRadiusSq)
					{
						Optional<int> newKindPriority = determine_priority(_energyHandling, o);
						if (!bestImo ||
							(newKindPriority.is_set() && newKindPriority.get() > bestKindPriority.get(NONE)) ||
							(newKindPriority.is_set() && newKindPriority.get() == bestKindPriority.get(NONE) && distSq < bestDistSq)
							)
						{
							bestImo = o;
							bestDistSq = distSq;
							bestKindPriority = newKindPriority;
						}
					}
				}
			}
		}
	}

	if (bestKindPriority.is_set())
	{
		Optional<int> inBayKindPriority = determine_priority(_energyHandling, objectInBay.get_target());
		if (bestKindPriority.get() > inBayKindPriority.get(bestKindPriority.get() - 1))
		{
			objectInBay.find_path(imo, bestImo);
			_energyHandling->requiresNeutral = true;
			_energyHandling->goodToTransfer = true;
			updateDisplay = true;
		}
		else if (inBayKindPriority.is_set() &&
				 bestKindPriority.get() == inBayKindPriority.get())
		{
			float bestDist = sqrt(bestDistSq);
			if (bestDist < inBayDist * hardcoded magic_number 0.8f)
			{
				objectInBay.find_path(imo, bestImo);
				_energyHandling->requiresNeutral = true;
				_energyHandling->goodToTransfer = true;
				updateDisplay = true;
			}
		}
	}
	else if (objectInBay.get_target() != bestImo)
	{
		Optional<int> inBayKindPriority = determine_priority(_energyHandling, objectInBay.get_target());
		if (!inBayKindPriority.is_set())
		{
			objectInBay.find_path(imo, bestImo);
			_energyHandling->requiresNeutral = true;
			_energyHandling->goodToTransfer = false;
			updateDisplay = true;
		}
	}

#ifdef AN_DEVELOPMENT
	if (objectInBay.is_active())
	{
		debug_draw_sphere(true, true, Colour::blue, 0.5f, Sphere(bayLoc, 0.05f));
	}
#endif
	_energyHandling->objectInBayWasSet = objectInBay.is_active();

	debug_no_context();
	debug_no_filter();
}

void EnergyKiosk::update_bay(::Framework::IModulesOwner* imo, bool _force)
{
	if (auto* e = imo->get_custom<CustomModules::EmissiveControl>())
	{
		int newBayOccupiedEmissive = 0;
		if (left.objectInBay.is_active() || right.objectInBay.is_active())
		{
			newBayOccupiedEmissive = 1;
			if (left.objectInBay.is_active() && left.goodToTransfer)
			{
				newBayOccupiedEmissive = 2;
			}
			if (right.objectInBay.is_active() && right.goodToTransfer)
			{
				newBayOccupiedEmissive = 2;
			}
		}

		if (machineIsOnTarget == 0.0f)
		{
			e->emissive_deactivate(NAME(empty));
			e->emissive_deactivate(NAME(active));
			e->emissive_deactivate(NAME(wrongItem));
		}
		else if ((bayOccupiedEmissive != newBayOccupiedEmissive) || _force)
		{
			if (newBayOccupiedEmissive)
			{
				bool goodItem = newBayOccupiedEmissive == 2;
				if (goodItem)
				{
#ifdef LOG_ENERGY_KIOSK
					output(TXT("[EnergyKiosk] emissive: good item = %S %S"),
						left.objectInBay.get_target() ? left.objectInBay.get_target()->ai_get_name().to_char() : TXT("--"),
						right.objectInBay.get_target() ? right.objectInBay.get_target()->ai_get_name().to_char() : TXT("--"));
#endif
					e->emissive_activate(NAME(active));
					e->emissive_deactivate(NAME(wrongItem));
				}
				else
				{
#ifdef LOG_ENERGY_KIOSK
					output(TXT("[EnergyKiosk] emissive: wrong item = %S %S"),
						left.objectInBay.get_target() ? left.objectInBay.get_target()->ai_get_name().to_char() : TXT("--"),
						right.objectInBay.get_target() ? right.objectInBay.get_target()->ai_get_name().to_char() : TXT("--"));
#endif
					e->emissive_deactivate(NAME(active));
					e->emissive_activate(NAME(wrongItem));
				}
				e->emissive_deactivate(NAME(empty));
			}
			else
			{
#ifdef LOG_ENERGY_KIOSK
				output(TXT("[EnergyKiosk] emissive: no item"));
#endif
				e->emissive_deactivate(NAME(active));
				e->emissive_deactivate(NAME(wrongItem));
				e->emissive_activate(NAME(empty));
			}
		}
		bayOccupiedEmissive = newBayOccupiedEmissive;
	}
}

void EnergyKiosk::update_indicators(EnergyHandling * _energyHandling, ::Framework::IModulesOwner* imo, bool _force)
{
	if (auto* e = imo->get_custom<CustomModules::EmissiveControl>())
	{
		Name indicator;
		Name requiresNeutralIndicator;
		if (_energyHandling == &left)
		{
			indicator = NAME(left);
			requiresNeutralIndicator = NAME(leftRequiresNeutral);
		}
		else
		{
			indicator = NAME(right);
			requiresNeutralIndicator = NAME(rightRequiresNeutral);
		}
		if (machineIsOnTarget == 0.0f && machineIsOn < 0.3f)
		{
			e->emissive_deactivate(requiresNeutralIndicator);
			e->emissive_deactivate(indicator);
			_energyHandling->indicateOn = false;
		}
		else
		{
			if (_energyHandling->requiresNeutralIndicateOn)
			{
				_energyHandling->indicateOn = false;
			}
			if ((_energyHandling->requiresNeutralIndicateOn ^ _energyHandling->requiresNeutralIndicateWasOn) || _force)
			{
				_energyHandling->requiresNeutralIndicateWasOn = _energyHandling->requiresNeutralIndicateOn;
				if (_energyHandling->requiresNeutralIndicateOn)
				{
					e->emissive_activate(requiresNeutralIndicator);
				}
				else
				{
					e->emissive_deactivate(requiresNeutralIndicator);
				}
			}
			if ((_energyHandling->indicateOn ^ _energyHandling->indicateWasOn) || _force)
			{
				_energyHandling->indicateWasOn = _energyHandling->indicateOn;
				if (_energyHandling->indicateOn)
				{
					e->emissive_activate(indicator);
				}
				else
				{
					e->emissive_deactivate(indicator);
				}
			}
		}
	}
}

LATENT_FUNCTION(EnergyKiosk::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai energy kiosk] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(bool, indicateLeftNow);
	LATENT_VAR(float, indicateTimeToSwitch);

	LATENT_VAR(float, accumulatedDeltaTime);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<EnergyKiosk>(logic);

	accumulatedDeltaTime += LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	accumulatedDeltaTime = 0.0f;

	self->left.switchHandler.initialise(imo, NAME(leftSliderInteractiveDeviceId));
	self->right.switchHandler.initialise(imo, NAME(rightSliderInteractiveDeviceId));

	self->bayRadius = imo->get_variables().get_value(NAME(bayRadius), self->bayRadius);
	self->bayRadius = imo->get_variables().get_value(NAME(bayRadiusDetect), self->bayRadius);

	self->baySocket.set_name(NAME(bay));
	self->baySocket.look_up(imo->get_appearance()->get_mesh());

	self->left.objectInBay.set_owner(imo);
	self->right.objectInBay.set_owner(imo);

	self->update_bay(imo, true);
	self->update_indicators(&self->left, imo, true);
	self->update_indicators(&self->right, imo, true);

	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(forceEnergyKioskOff), [self](Framework::AI::Message const& _message)
			{
				if (auto* offParam = _message.get_param(NAME(forceEnergyKioskOff)))
				{
					self->machineIsOnTarget = offParam->get_as<bool>() ? 0.0f : 1.0f;
					self->update_bay(self->get_imo(), true);
				}
			}
			);
	}

	while (true)
	{
		// update in-bay object
		{
			// check if we have moved too far
			self->update_object_in_bay(&self->left, imo);
			self->update_object_in_bay(&self->right, imo);
		}
		if (self->machineIsOnTarget > 0.0f)
		{
			MODULE_OWNER_LOCK_FOR_IMO(imo, TXT("get values for energy kiosk"));
			Energy prevEnergy = self->energy;
			// restore from state variable
			{
				self->energy = imo->get_variables().get_value<Energy>(NAME(energyInStorage), self->energy);
				self->maxEnergy = imo->get_variables().get_value<Energy>(NAME(maxEnergy), self->maxEnergy);
				self->maxEnergy = imo->get_variables().get_value<Energy>(NAME(maxStorage), self->maxEnergy);
				self->maxEnergy = max(self->maxEnergy, self->energy);
			}
			// check state of switches and transfer energy
			{
				EnergyTransfer prevEnergyTransfer = self->energyTransfer;
				self->handle_switch_and_transfer_energy(&self->left, accumulatedDeltaTime, imo);
				self->handle_switch_and_transfer_energy(&self->right, accumulatedDeltaTime, imo);
				self->handle_energy_transfer_temporary_objects(prevEnergyTransfer, self->energyTransfer, imo);
			}
			// store as state variable
			{
				imo->access_variables().access<Energy>(NAME(energyInStorage)) = self->energy;
			}
			if (prevEnergy != self->energy)
			{
				self->updateDisplay = true;
				self->displayMoveLines = 0;
			}
		}

		{
			indicateTimeToSwitch -= accumulatedDeltaTime;
			while (indicateTimeToSwitch < 0.0f)
			{
				indicateTimeToSwitch += 0.5f;
				indicateLeftNow = !indicateLeftNow;
			}
		}

		self->update_bay(imo);

		if (self->left.requiresNeutral || self->right.requiresNeutral)
		{
			if (self->left.requiresNeutral &&
				self->right.requiresNeutral)
			{
				if (indicateLeftNow)
				{
					self->left.requiresNeutralIndicateOn = self->left.requiresNeutral;
					self->right.requiresNeutralIndicateOn = false;
				}
				else
				{
					self->left.requiresNeutralIndicateOn = false;
					self->right.requiresNeutralIndicateOn = self->right.requiresNeutral;
				}
			}
			else
			{
				if (self->left.requiresNeutral)
				{
					self->left.requiresNeutralIndicateOn = self->left.requiresNeutral;
					self->right.requiresNeutralIndicateOn = false;
				}
				else
				{
					self->left.requiresNeutralIndicateOn = false;
					self->right.requiresNeutralIndicateOn = self->right.requiresNeutral;
				}
			}
			self->left.indicateOn = false;
			self->right.indicateOn = false;
			self->update_indicators(&self->left, imo);
			self->update_indicators(&self->right, imo);
			accumulatedDeltaTime = 0.0f;
			LATENT_YIELD();
		}
		else if (self->left.objectInBay.is_active() ||
				 self->right.objectInBay.is_active())
		{
			if (self->left.objectInBay.is_active() &&
				self->right.objectInBay.is_active())
			{
				if (indicateLeftNow)
				{
					self->left.indicateOn = true;
					self->right.indicateOn = false;
				}
				else
				{
					self->left.indicateOn = false;
					self->right.indicateOn = true;
				}
			}
			else
			{
				if (self->left.objectInBay.is_active())
				{
					self->left.indicateOn = true;
					self->right.indicateOn = false;
				}
				else
				{
					self->left.indicateOn = false;
					self->right.indicateOn = true;
				}
			}
			self->left.requiresNeutralIndicateOn = false;
			self->right.requiresNeutralIndicateOn = false;
			self->update_indicators(&self->left, imo);
			self->update_indicators(&self->right, imo);
			accumulatedDeltaTime = 0.0f;
			LATENT_YIELD();
		}
		else
		{
			self->left.indicateOn = false;
			self->right.indicateOn = false;
			self->left.requiresNeutralIndicateOn = false;
			self->right.requiresNeutralIndicateOn = false;
			self->update_indicators(&self->left, imo);
			self->update_indicators(&self->right, imo);
			accumulatedDeltaTime = 0.0f;
			LATENT_WAIT_NO_RARE_ADVANCE(Random::get_float(0.1f, 0.2f));
		}
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(EnergyKioskData);

bool EnergyKioskData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	if (auto* node = _node->first_child_named(TXT("display")))
	{
		displayInactiveColour.load_from_xml_child_or_attr(node, TXT("inactiveColour"));
		displayActiveColour.load_from_xml_child_or_attr(node, TXT("activeColour"));
		displayWrongItemColour.load_from_xml_child_or_attr(node, TXT("wrongItemColour"));
	}

	leftEnergyType = EnergyType::parse(_node->get_string_attribute(TXT("leftEnergyType")));
	rightEnergyType = EnergyType::parse(_node->get_string_attribute(TXT("rightEnergyType")));
	result &= right.load_from_xml(_node->first_child_named(TXT("right")), _lc);
	result &= left.load_from_xml(_node->first_child_named(TXT("left")), _lc);

	return result;
}

bool EnergyKioskData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= right.prepare_for_game(_library, _pfgContext);
	result &= left.prepare_for_game(_library, _pfgContext);

	return result;
}

//

bool EnergyKioskData::Element::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	if (!_node)
	{
		error_loading_xml(_node, TXT("missing node!"));
		return false;
	}
	bool result = true;

	result &= texturePart.load_from_xml(_node, TXT("texturePart"), _lc);
	result &= at.load_from_xml_child_node(_node, TXT("at"));
	result &= ink.load_from_xml_child_or_attr(_node, TXT("ink"));
	result &= inkInactive.load_from_xml_child_or_attr(_node, TXT("inkInactive"));

	return result;
}

bool EnergyKioskData::Element::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= texturePart.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);

	return result;
}
