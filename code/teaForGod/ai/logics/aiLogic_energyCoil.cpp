#include "aiLogic_energyCoil.h"

#include "exms\aiLogic_exmEnergyDispatcher.h"

#include "..\..\game\game.h"
#include "..\..\game\gameDirector.h"
#include "..\..\modules\energyTransfer.h"
#include "..\..\modules\gameplay\moduleEXM.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_hitIndicator.h"
#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\appearance\controllers\ac_particlesUtils.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
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
DEFINE_STATIC_NAME(bayRadius);
DEFINE_STATIC_NAME(bayRadiusDetect);
DEFINE_STATIC_NAME(energy);
DEFINE_STATIC_NAME(maxStorage);
DEFINE_STATIC_NAME(energyType);
DEFINE_STATIC_NAME(transferSpeed);

// state variables
DEFINE_STATIC_NAME(energyInStorage);

// sockets
DEFINE_STATIC_NAME(bay);

// emissive layers
DEFINE_STATIC_NAME(active);
DEFINE_STATIC_NAME(empty);

// sounds 
DEFINE_STATIC_NAME_STR(withdrawEmpty, TXT("withdraw empty"));
DEFINE_STATIC_NAME_STR(withdrawFull, TXT("withdraw full"));
DEFINE_STATIC_NAME(transfer);

// temporary objects
DEFINE_STATIC_NAME_STR(healthWithdraw, TXT("health withdraw"));
DEFINE_STATIC_NAME_STR(ammoWithdraw, TXT("ammo withdraw"));
DEFINE_STATIC_NAME_STR(healthIdle, TXT("health idle"));
DEFINE_STATIC_NAME_STR(ammoIdle, TXT("ammo idle"));

// exm variables
DEFINE_STATIC_NAME(energyDispenserDrain);

// game script traps
DEFINE_STATIC_NAME_STR(gstCalibrateAmmoStorages, TXT("calibrate ammo storages"));

//

REGISTER_FOR_FAST_CAST(EnergyCoil);

EnergyCoil::EnergyCoil(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	energyCoilData = fast_cast<EnergyCoilData>(_logicData);
}

EnergyCoil::~EnergyCoil()
{
	if (display)
	{
		display->use_background(nullptr);
		display->set_on_update_display(this, nullptr);
	}
}

void EnergyCoil::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	timeToRedraw -= _deltaTime;
	if (timeToRedraw <= 0.0f)
	{
		timeToRedraw = 0.05f;
		redrawNow = true;

		float lengthPt = energy.div_to_float(maxStorage);

		if (energyTransfer == EnergyTransfer::Withdraw)
		{
			displayBottomAtPt *= 0.3f;
			displayBottomDir = 1.0f;
		}
		else
		{
			if (displayBottomDir == 0.0f)
			{
				displayBottomDir = 1.0f;
			}
			float pt = energy.div_to_float(maxStorage);
			displayBottomAtPt += timeToRedraw * displayBottomDir * (1.0f - pt);
			if (displayBottomAtPt <= 0.0f)
			{
				displayBottomAtPt = 0.0f;
				displayBottomDir = 1.0f;
			}
			else if (displayBottomAtPt >= 1.0f - lengthPt)
			{
				displayBottomAtPt = 1.0f - lengthPt;
				displayBottomDir = -1.0f;
			}
		}
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
						Colour const ink = bay.energyType == EnergyType::Health ? energyCoilData->displayHealthColour : energyCoilData->displayAmmoColour;
						
						display->set_on_update_display(this,
							[this, ink](Framework::Display* _display)
							{
								if (!redrawNow)
								{
									return;
								}
								redrawNow = false;

								_display->drop_all_draw_commands();
								_display->add((new Framework::DisplayDrawCommands::CLS(Colour::black)));
								_display->add((new Framework::DisplayDrawCommands::Border(Colour::black)));

								if (energy.is_positive())
								{
									float pt = energy.div_to_float(maxStorage);
									float atPt = displayBottomAtPt;
									_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
										[pt, atPt, ink](Framework::Display* _display, ::System::Video3D* _v3d)
										{
											Vector2 bl = _display->get_left_bottom_of_screen();
											Vector2 tr = _display->get_right_top_of_screen();
											Range2 r = Range2::empty;
											float ptSize = round((tr.y - bl.y) * pt);
											float bottom = bl.y + round((tr.y - bl.y) * atPt);
											bottom = min(bottom, tr.y - ptSize);
											r.include(Vector2(bl.x, bottom));
											r.include(Vector2(tr.x, bottom + ptSize));
											::System::Video3DPrimitives::fill_rect_2d(ink, r);
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

void EnergyCoil::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);

	{
		String et = _parameters.get_value<String>(NAME(energyType), String::empty());
		if (!et.is_empty())
		{
			bay.energyType = EnergyType::parse(et, bay.energyType);
		}
	}
	energy = Energy::get_from_storage(_parameters, NAME(energy), energy);
	maxStorage = Energy::get_from_storage(_parameters, NAME(maxStorage), maxStorage);

	maxStorage = max(maxStorage, energy);
	bay.transferSpeed = Energy::get_from_storage(_parameters, NAME(transferSpeed), bay.transferSpeed);

	if (auto* mind = get_mind())
	{
		if (auto* imo = mind->get_owner_as_modules_owner())
		{
			// store
			imo->access_variables().access<Energy>(NAME(energyInStorage)) = energy;
			imo->access_variables().access<Energy>(NAME(maxStorage)) = maxStorage;
			imo->access_variables().access<String>(NAME(energyType)) = EnergyType::to_char(bay.energyType);
		}
	}

	ai_log(this, TXT("starting energy %S"), energy.as_string_auto_decimals().to_char());
}

void EnergyCoil::update_object_in_bay(::Framework::IModulesOwner* imo)
{
	auto* imoAp = imo->get_appearance();
	if (!imoAp)
	{
		return;
	}

	float const bayRadiusSq = sqr(bayRadius);
	Vector3 bayLoc = imoAp->get_os_to_ws_transform().location_to_world(imoAp->calculate_socket_os(baySocket.get_index()).get_translation());
	float inBayDist = 0.0f;

	::Framework::RelativeToPresencePlacement& objectInBay = bay.objectInBay;

	if (auto* ib = objectInBay.get_target())
	{
		if (ib->get_presence()->get_in_room() != imo->get_presence()->get_in_room())
		{
			objectInBay.clear_target();
		}
		else
		{
			float inBayDist = (bayLoc - ib->get_presence()->get_centre_of_presence_WS()).length();
			if (inBayDist > bayRadius * hardcoded magic_number 1.2f)
			{
				objectInBay.clear_target();
			}
		}
	}

	float bestDistSq;
	Framework::IModulesOwner* bestImo = nullptr;
	Optional<int> bestKindPriority;

	// we can check just our room and just objects, don't check presence links
	{
		Framework::IModulesOwner* inBay = objectInBay.get_target();
		//FOR_EVERY_OBJECT_IN_ROOM(o, imo->get_presence()->get_in_room())
		for_every_ptr(o, imo->get_presence()->get_in_room()->get_objects())
		{
			if (o != inBay)
			{
				if (auto* op = o->get_presence())
				{
					Vector3 oCentre = op->get_centre_of_presence_WS();
					float distSq = (bayLoc - oCentre).length_squared();
					if (distSq < bayRadiusSq &&
						(!bestImo || distSq < bestDistSq))
					{
						Optional<int> newKindPriority = determine_priority(o);
						if (newKindPriority.is_set())
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
		Optional<int> inBayKindPriority = determine_priority(objectInBay.get_target());
		if (bestKindPriority.get() > inBayKindPriority.get(bestKindPriority.get() - 1))
		{
			objectInBay.find_path(imo, bestImo);
		}
		else if (inBayKindPriority.is_set() &&
			bestKindPriority.get() == inBayKindPriority.get())
		{
			float bestDist = sqrt(bestDistSq);
			if (bestDist < inBayDist * hardcoded magic_number 0.8f)
			{
				objectInBay.find_path(imo, bestImo);
			}
		}
	}
}

Optional<int> EnergyCoil::determine_priority(::Framework::IModulesOwner* _object)
{
	if (!_object)
	{
		return NP;
	}
	if (auto* it = IEnergyTransfer::get_energy_transfer_module_from(_object, bay.energyType))
	{
		if (bay.energyType == EnergyType::Ammo)
		{
			EnergyTransferRequest etr(EnergyTransferRequest::Query);
			if (it->handle_ammo_energy_transfer_request(etr))
			{
				return etr.priority;
			}
		}
		else if (bay.energyType == EnergyType::Health)
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

void EnergyCoil::update_bay(::Framework::IModulesOwner* imo, bool _force)
{
	if (auto* e = imo->get_custom<CustomModules::EmissiveControl>())
	{
		bool newBayOccupiedEmissive = bay.objectInBay.is_active();

		if ((bayOccupiedEmissive ^ newBayOccupiedEmissive) || _force)
		{
			if (newBayOccupiedEmissive)
			{
				e->emissive_activate(NAME(active));
				e->emissive_deactivate(NAME(empty));
			}
			else
			{
				e->emissive_deactivate(NAME(active));
				e->emissive_activate(NAME(empty));
			}
		}
		bayOccupiedEmissive = newBayOccupiedEmissive;
	}
}

void EnergyCoil::handle_transfer_energy(float _deltaTime, ::Framework::IModulesOwner* imo)
{
	float transferSpeed = 1.0f;

	EnergyTransfer prevEnergyTransfer = energyTransfer;

	bool demandMarkingPilgrimageDeviceKnown = false;
	Optional<float> delayDemandMarkingPilgrimageDeviceKnown;

	if (bay.objectInBay.is_active())
	{
		bay.timeToNextTransfer -= _deltaTime * transferSpeed;
		Energy energyToTransfer = Energy(0);
		while (bay.timeToNextTransfer <= 0.0f)
		{
			bay.timeToNextTransfer += 1.0f / (bay.transferSpeed.as_float());
			energyToTransfer += Energy(1);
		}
		if (!energyToTransfer.is_zero())
		{
			if (auto* it = IEnergyTransfer::get_energy_transfer_module_from(bay.objectInBay.get_target(), bay.energyType))
			{
				EnergyTransferRequest etr(EnergyTransferRequest::Deposit /*withdraw here, deposit in object*/);
				etr.energyRequested = energyToTransfer;
				etr.energyRequested = min(etr.energyRequested, energy);
				if (!etr.energyRequested.is_zero())
				{
					demandMarkingPilgrimageDeviceKnown = true;

					EnergyCoef drainCoef = EnergyCoef::zero();
					ModulePilgrim* toPilgrim = nullptr;
					{
						if (auto* mp = bay.objectInBay.get_target()->get_top_instigator()->get_gameplay_as<ModulePilgrim>())
						{
							toPilgrim = mp;
							if (auto* exm = mp->get_equipped_exm(EXMID::Passive::energy_dispenser_drain()))
							{
								drainCoef = exm->get_owner()->get_variables().get_value<EnergyCoef>(NAME(energyDispenserDrain), EnergyCoef::zero());
							}
						}
						drainCoef += EnergyCoef::one();
						{
							if (bay.energyType == EnergyType::Health)
							{
								drainCoef = drainCoef.mul(GameSettings::get().difficulty.lootHealth);
							}
							if (bay.energyType == EnergyType::Ammo)
							{
								drainCoef = drainCoef.mul(GameSettings::get().difficulty.lootAmmo);
							}
						}
					}
					etr.energyRequested = etr.energyRequested.adjusted(drainCoef);
					bool success = false;
					bool calibrated = false;
					for_count(int, transferIdx, 2)
					{
						success = false;
						if (bay.energyType == EnergyType::Ammo)
						{
							if (toPilgrim &&
								GameDirector::get()->are_ammo_storages_unavailable())
							{
								todo_hack(TXT("not to be fixed, this is partially hardcoded solution for calibration, hacked to make it work"))
								// fix ammo storage via game script
								Framework::GameScript::ScriptExecution::trigger_execution_trap(NAME(gstCalibrateAmmoStorages));
								// delay it to allow the script to handle blocking info, to play voiceover etc
								// we don't want to make the device symbols pop up immediately
								delayDemandMarkingPilgrimageDeviceKnown = 1.0f;
								// use all energy to calibrate
								energy = Energy::zero();
								calibrated = true;
								break;
							}
							else
							{
								success = it->handle_ammo_energy_transfer_request(etr);
							}
						}
						else if (bay.energyType == EnergyType::Health)
						{
							success = it->handle_health_energy_transfer_request(etr);
						}
						if (transferIdx == 0 && etr.energyResult.is_zero())
						{
							// transferred nothing, check if we can redistribute energy we could transfer
							Energy toRedistribute = etr.energyRequested;
							Energy redistributed = AI::Logics::EXMEnergyDispatcher::redistribute_energy_from(it, bay.energyType, toRedistribute);
							if (redistributed.is_positive())
							{
								// redisitributed, try again
								continue;
							}
						}
						break;
					}
					if (calibrated)
					{
						energyTransfer = prevEnergyTransfer == EnergyTransfer::None? EnergyTransfer::Withdraw : EnergyTransfer::None; // to force change
					}
					else if (etr.energyResult.is_zero() || !success)
					{
						if (energyTransfer != EnergyTransfer::None)
						{
							ai_log(this, TXT("withdraw full"));
							if (auto* s = imo->get_sound())
							{
								s->play_sound(NAME(withdrawFull));
							}
						}
						energyTransfer = EnergyTransfer::None;
					}
					else if (success)
					{
						etr.energyResult = etr.energyResult.adjusted_inv(drainCoef);
						energy -= etr.energyResult;
						energyTransfer = EnergyTransfer::Withdraw;
					}
				}
				else
				{
					if (energyTransfer != EnergyTransfer::None)
					{
						ai_log(this, TXT("withdraw empty"));
						if (auto* s = imo->get_sound())
						{
							s->play_sound(NAME(withdrawEmpty));
						}
						if (auto* piow = PilgrimageInstanceOpenWorld::get())
						{
							Optional<VectorInt2> cellAt = piow->find_cell_at(imo);
							if (cellAt.is_set())
							{
								piow->mark_pilgrim_device_state_depleted(cellAt.get(), imo);
							}
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
		bay.timeToNextTransfer = 0.0f;
		energyTransfer = EnergyTransfer::None;
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
		energyTransferPhysSens.update(energyTransfer != EnergyTransfer::None, bay.objectInBay.get_target());

		demandMarkingPilgrimageDeviceKnown = true;
	}

	if (demandMarkingPilgrimageDeviceKnown)
	{
		if (!markedAsKnownForOpenWorldDirection)
		{
			if (auto* piow = PilgrimageInstanceOpenWorld::get())
			{
				if (delayDemandMarkingPilgrimageDeviceKnown.is_set())
				{
					imo->set_timer(max(0.01f, delayDemandMarkingPilgrimageDeviceKnown.get()),
						[](Framework::IModulesOwner* _imo)
						{
							if (auto* piow = PilgrimageInstanceOpenWorld::get())
							{
								piow->mark_pilgrimage_device_direction_known(_imo);
							}
						});
				}
				else
				{
					piow->mark_pilgrimage_device_direction_known(imo);
				}
				markedAsKnownForOpenWorldDirection = true;
			}
		}
	}
}

Name EnergyCoil::get_energy_transfer_temporary_object_name(EnergyTransfer _energyTransfer)
{
	if (_energyTransfer == EnergyTransfer::Withdraw)
	{
		switch (bay.energyType)
		{
		case EnergyType::Health: return NAME(healthWithdraw);
		case EnergyType::Ammo: return NAME(ammoWithdraw);
		case EnergyType::None: break;
		default: break;
		}
	}
	if (_energyTransfer == EnergyTransfer::None && energy.is_positive())
	{
		switch (bay.energyType)
		{
		case EnergyType::Health: return NAME(healthIdle);
		case EnergyType::Ammo: return NAME(ammoIdle);
		case EnergyType::None: break;
		default: break;
		}
	}
	return Name::invalid();
}

void EnergyCoil::handle_energy_transfer_temporary_objects(EnergyTransfer _prevEnergyTransfer, EnergyTransfer _energyTransfer, ::Framework::IModulesOwner* imo, bool _force)
{
	if (_prevEnergyTransfer == _energyTransfer && ! _force)
	{
		return;
	}

	ai_log(this, TXT("started %S (%S)"), get_energy_transfer_temporary_object_name(_energyTransfer).to_char(), get_energy_transfer_temporary_object_name(_prevEnergyTransfer).to_char());

	for_every_ref(o, energyTransferTemporaryObjects)
	{
		if (auto* to = fast_cast<Framework::TemporaryObject>(o))
		{
			Framework::ParticlesUtils::desire_to_deactivate(to);
		}
	}
	energyTransferTemporaryObjects.clear();

	Name toName = get_energy_transfer_temporary_object_name(_energyTransfer);
	if (toName.is_valid())
	{
		if (auto* to = imo->get_temporary_objects())
		{
			to->spawn_all(get_energy_transfer_temporary_object_name(_energyTransfer), NP, &energyTransferTemporaryObjects);
		}
	}
}

LATENT_FUNCTION(EnergyCoil::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai energy coil] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<EnergyCoil>(logic);

	LATENT_BEGIN_CODE();

	self->bayRadius = imo->get_variables().get_value(NAME(bayRadius), self->bayRadius);
	self->bayRadius = imo->get_variables().get_value(NAME(bayRadiusDetect), self->bayRadius);

	self->baySocket.set_name(NAME(bay));
	self->baySocket.look_up(imo->get_appearance()->get_mesh());

	ai_log(self, TXT("energy coil, hello!"));

	self->handle_energy_transfer_temporary_objects(self->energyTransfer, self->energyTransfer, imo, true);

	LATENT_WAIT(1.0f); // give it a bit of time
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("energy coil %S, energy in storage: %S [%S]"), imo->ai_get_name().to_char(), self->energy.as_string().to_char(), imo->get_variables().get_value<Energy>(NAME(energyInStorage), self->energy).as_string().to_char());
#endif

	while (true)
	{
		self->update_object_in_bay(imo);
		// restore from state variable
		{
#ifdef AN_USE_AI_LOG
			Energy prevEnergy = self->energy;
#endif
			self->energy = imo->get_variables().get_value<Energy>(NAME(energyInStorage), self->energy);
#ifdef AN_USE_AI_LOG
			if (prevEnergy != self->energy)
			{
				ai_log(self, TXT("update energy from state variable to %S"), self->energy.as_string_auto_decimals().to_char());
			}
#endif

		}
		// check state of switches and transfer energy
		{
			EnergyTransfer prevEnergyTransfer = self->energyTransfer;
			self->handle_transfer_energy(LATENT_DELTA_TIME, imo);
			self->handle_energy_transfer_temporary_objects(prevEnergyTransfer, self->energyTransfer, imo, false);
		}
		// store as state variable
		{
			imo->access_variables().access<Energy>(NAME(energyInStorage)) = self->energy;
		}

		if (self->bay.objectInBay.is_active())
		{
			LATENT_YIELD();
		}
		else
		{
			LATENT_WAIT_NO_RARE_ADVANCE(Random::get_float(0.1f, 0.2f)); // to still call ::advance
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

REGISTER_FOR_FAST_CAST(EnergyCoilData);

bool EnergyCoilData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	displayAmmoColour.load_from_xml_child_node(_node, TXT("displayAmmoColour"));
	displayHealthColour.load_from_xml_child_node(_node, TXT("displayHealthColour"));

	return result;
}

bool EnergyCoilData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
