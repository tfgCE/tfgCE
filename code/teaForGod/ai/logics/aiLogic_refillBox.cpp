#include "aiLogic_refillBox.h"

#include "..\..\game\game.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_hitIndicator.h"
#include "..\..\modules\custom\mc_itemHolder.h"
#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\appearance\controllers\ac_particlesUtils.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\game\delayedObjectCreation.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\gameScript\gameScriptExecution.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleMovementSwitch.h"
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
DEFINE_STATIC_NAME(wallBoxDoorOpen);
DEFINE_STATIC_NAME(wallBoxDoorOpenActual);
DEFINE_STATIC_NAME(coilOut);
DEFINE_STATIC_NAME(coilOutActual);
DEFINE_STATIC_NAME(lockerBoxContent);
DEFINE_STATIC_NAME(energy);
DEFINE_STATIC_NAME(maxEnergy);
DEFINE_STATIC_NAME(maxEnergyForCommonHandEnergyStorage);
DEFINE_STATIC_NAME(energyInStorage);

DEFINE_STATIC_NAME(bayRadius);
DEFINE_STATIC_NAME(coil);

DEFINE_STATIC_NAME(transferSpeed);

// sounds
DEFINE_STATIC_NAME(done);
DEFINE_STATIC_NAME(transfer);

//

REGISTER_FOR_FAST_CAST(RefillBox);

RefillBox::RefillBox(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	refillBoxData = fast_cast<RefillBoxData>(_logicData);

	openVar.set_name(NAME(wallBoxDoorOpen));
	coilOutVar.set_name(NAME(coilOut));
	openActualVar.set_name(NAME(wallBoxDoorOpenActual));
	coilOutActualVar.set_name(NAME(coilOutActual));
}

RefillBox::~RefillBox()
{
	if (display)
	{
		display->use_background(nullptr);
		display->set_on_update_display(this, nullptr);
	}
}

void RefillBox::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	timeToCheckPlayer -= _deltaTime;

	if (timeToCheckPlayer < 0.0f)
	{
		timeToCheckPlayer = Random::get_float(0.1f, 0.3f);

		bool isPlayerInNow = false;

		{
			auto* imo = get_mind()->get_owner_as_modules_owner();
			for_every_ptr(object, imo->get_presence()->get_in_room()->get_objects())
			{
				if (object->get_gameplay_as<ModulePilgrim>())
				{
					isPlayerInNow = true;
					break;
				}
			}
		}

		if (isPlayerInNow ^ playerIsIn)
		{
			playerIsIn = isPlayerInNow;
		}
	}

	{
		timeToUpdateDisplayLeft -= _deltaTime;
		if (timeToUpdateDisplayLeft < 0.0f)
		{
			updateDisplay = true;
			timeToUpdateDisplayLeft = 0.4f;
			displayMoveLines = 1;
		}
	}
}

void RefillBox::learn_from(SimpleVariableStorage & _parameters)
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
								int minDraw = 1;
								float amountPt = energy.div_to_float(maxEnergy);

								int totalLines = TypeConversions::Normal::f_i_closest(_display->get_right_top_of_screen().y - _display->get_left_bottom_of_screen().y) + 1;
								displayData.update(totalLines, amountPt * machineIsOn, minDraw, displayMoveLines);
								displayMoveLines = 0;
							}

							Colour ink = energy < maxEnergy? Colour::white : Colour::lerp(0.7f, Colour::white, Colour::blue);
							_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
								[ink, this](Framework::Display* _display, ::System::Video3D* _v3d)
								{
									Vector2 a(_display->get_left_bottom_of_screen());
									Vector2 b(_display->get_right_top_of_screen());
									for_every(u, displayData.used)
									{
										int y = *u;
										a.y = _display->get_left_bottom_of_screen().y + (float)y;
										b.y = a.y;
										::System::Video3DPrimitives::line_2d(ink, a, b, false);
									}
								}));
							_display->draw_all_commands_immediately();
						});
				}
			}
		}
	}

	energyHandling.transferSpeed = Energy::get_from_storage(_parameters, NAME(transferSpeed), energyHandling.transferSpeed);

	energy = Energy::get_from_storage(_parameters, NAME(energy), energy);
	maxEnergy = Energy::get_from_storage(_parameters, GameSettings::get().difficulty.commonHandEnergyStorage? NAME(maxEnergyForCommonHandEnergyStorage) : NAME(maxEnergy), maxEnergy);

	if (!maxEnergy.is_zero())
	{
		maxEnergy = max(maxEnergy, energy);
	}

	if (auto* mind = get_mind())
	{
		if (auto* imo = mind->get_owner_as_modules_owner())
		{
			imo->access_variables().access<Energy>(NAME(energyInStorage)) = energy;
		}
	}
}

Optional<int> RefillBox::determine_priority(::Framework::IModulesOwner* _object)
{
	if (!_object)
	{
		return NP;
	}
	if (auto* it = IEnergyTransfer::get_energy_transfer_module_from(_object, EnergyType::Ammo))
	{
		EnergyTransferRequest etr(EnergyTransferRequest::Query);
		if (it->handle_ammo_energy_transfer_request(etr))
		{
			return etr.priority;
		}
	}
	return NP;
}

void RefillBox::update_object_in_coil(::Framework::IModulesOwner* imo)
{
	auto* imoAp = imo->get_appearance();
	if (!imoAp)
	{
		return;
	}

	float const bayRadiusSq = sqr(bayRadius);
	Vector3 bayLoc = imoAp->get_os_to_ws_transform().location_to_world(imoAp->calculate_socket_os(baySocket.get_index()).get_translation());
	float inBayDist = 0.0f;

	::Framework::RelativeToPresencePlacement& objectInBay = energyHandling.objectInBay;

	todo_multiplayer_issue(TXT("not checking which player has seen it, any does?"));
	// allow for object in bay only if is in a room that has been recently seen by a player

	if (auto* ib = objectInBay.get_target())
	{
		if (ib->get_presence()->get_in_room() != imo->get_presence()->get_in_room() ||
			!imo->get_presence()->get_in_room() ||
			!imo->get_presence()->get_in_room()->was_recently_seen_by_player())
		{
			objectInBay.clear_target();
			updateDisplay = true;
		}
		else
		{
			float inBayDist = (bayLoc - ib->get_presence()->get_centre_of_presence_WS()).length();
			if (inBayDist > bayRadius * hardcoded magic_number 1.2f)
			{
				objectInBay.clear_target();
				updateDisplay = true;
			}
		}
	}

	float bestDistSq;
	Framework::IModulesOwner* bestImo = nullptr;
	Optional<int> bestKindPriority;

	// we can check just our room and just objects, don't check presence links
	if (! depleted &&
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
					if (distSq < bayRadiusSq)
					{
						Optional<int> newKindPriority = determine_priority(o);
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
		Optional<int> inBayKindPriority = determine_priority(objectInBay.get_target());
		if (bestKindPriority.get() > inBayKindPriority.get(bestKindPriority.get() - 1))
		{
			objectInBay.find_path(imo, bestImo);
			updateDisplay = true;
		}
		else if (inBayKindPriority.is_set() &&
			bestKindPriority.get() == inBayKindPriority.get())
		{
			float bestDist = sqrt(bestDistSq);
			if (bestDist < inBayDist * hardcoded magic_number 0.8f)
			{
				objectInBay.find_path(imo, bestImo);
				updateDisplay = true;
			}
		}
	}
	else if (objectInBay.get_target() != bestImo)
	{
		Optional<int> inBayKindPriority = determine_priority(objectInBay.get_target());
		if (!inBayKindPriority.is_set())
		{
			objectInBay.find_path(imo, bestImo);
			updateDisplay = true;
		}
	}
}

void RefillBox::transfer_energy(::Framework::IModulesOwner* imo, float _deltaTime, bool _allowTransfer)
{
	float transferSpeed = 1.0f;

	bool prevTransferring = energyHandling.transferring;

	{
		if (energyHandling.objectInBay.is_active() && _allowTransfer)
		{
			energyHandling.timeToNextTransfer -= _deltaTime * transferSpeed;
			Energy energyToTransfer = Energy(0);
			while (energyHandling.timeToNextTransfer <= 0.0f)
			{
				energyHandling.timeToNextTransfer += 1.0f / (energyHandling.transferSpeed.as_float());
				energyToTransfer += Energy(1);
			}
			if (!energyToTransfer.is_zero())
			{
				if (auto* it = IEnergyTransfer::get_energy_transfer_module_from(energyHandling.objectInBay.get_target(), EnergyType::Ammo))
				{
					EnergyTransferRequest etr(EnergyTransferRequest::Withdraw); // withdraw from object
					etr.energyRequested = energyToTransfer;
					etr.energyRequested = min(etr.energyRequested, maxEnergy - energy);
					if (!etr.energyRequested.is_zero())
					{
						energyHandling.transferring = false;

						bool success = false;
						success = it->handle_ammo_energy_transfer_request(etr);
						if (etr.energyResult.is_zero() || !success)
						{
							if (auto* s = imo->get_sound())
							{
								//s->play_sound(switchOutput == DEPOSIT_IN_KIOSK ? NAME(depositFull) : NAME(withdrawEmpty));
							}
						}
						else if (success)
						{
							energy += etr.energyResult;
							energyHandling.transferring = true;
						}
					}
					else
					{
						energyHandling.transferring = false;

						if (prevTransferring)
						{
							if (auto* s = imo->get_sound())
							{
								//s->play_sound(switchOutput == DEPOSIT_IN_KIOSK ? NAME(depositEmpty) : NAME(withdrawFull));
							}
						}
					}
				}
				else
				{
					energyHandling.transferring = false;
				}
			}
		}
		else
		{
			energyHandling.transferring = false;
		}
	}

	if (prevTransferring != energyHandling.transferring)
	{
		if (auto* s = imo->get_sound())
		{
			if (energyHandling.transferring)
			{
				s->play_sound(NAME(transfer));
			}
			else
			{
				s->stop_sound(NAME(transfer));
			}
		}
		energyTransferPhysSens.update(energyHandling.transferring, energyHandling.objectInBay.get_target());

		{
			for_every_ref(o, energyTransferTemporaryObjects)
			{
				if (auto* to = fast_cast<Framework::TemporaryObject>(o))
				{
					Framework::ParticlesUtils::desire_to_deactivate(to);
				}
			}
			energyTransferTemporaryObjects.clear();

			if (energyHandling.transferring)
			{
				if (auto* to = imo->get_temporary_objects())
				{
					to->spawn_all(NAME(transfer), NP, &energyTransferTemporaryObjects);
				}
			}

		}
	}
}

LATENT_FUNCTION(RefillBox::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai refillBox] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(bool, madeKnown);
	LATENT_VAR(bool, madeVisited);
	LATENT_VAR(Optional<VectorInt2>, cellAt);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<RefillBox>(logic);

	LATENT_BEGIN_CODE();

	self->bayRadius = imo->get_variables().get_value(NAME(bayRadius), self->bayRadius);

	self->baySocket.set_name(NAME(coil));
	self->baySocket.look_up(imo->get_appearance()->get_mesh());

	self->depleted = false;

	ai_log(self, TXT("refill box, hello!"));

	self->openVar.look_up<float>(imo->access_variables());
	self->openActualVar.look_up<float>(imo->access_variables());
	self->coilOutVar.look_up<float>(imo->access_variables());
	self->coilOutActualVar.look_up<float>(imo->access_variables());

	if (imo)
	{
		if (auto* piow = PilgrimageInstanceOpenWorld::get())
		{
			cellAt = piow->find_cell_at(imo);
			if (cellAt.is_set())
			{
				self->depleted = piow->is_pilgrim_device_state_depleted(cellAt.get(), imo);
				ai_log(self, self->depleted? TXT("depleted") : TXT("available"));
			}
		}
	}

	madeKnown = false;
	madeVisited = false;

	if (auto* piow = PilgrimageInstanceOpenWorld::get())
	{
		madeVisited = piow->is_pilgrim_device_state_visited(cellAt.get(), imo);
	}

	while (true)
	{
		self->openVar.access<float>() = (!self->depleted && self->playerIsIn) || self->coilOutActualVar.get<float>() > 0.2f ? 1.0f : 0.0f;
		self->coilOutVar.access<float>() = (!self->depleted && self->playerIsIn && self->openActualVar.get<float>() > 0.8f) ? 1.0f : 0.0f;

		if (self->playerIsIn)
		{
			if (!madeVisited)
			{
				madeVisited = true;
				if (auto* piow = PilgrimageInstanceOpenWorld::get())
				{
					piow->mark_pilgrim_device_state_visited(cellAt.get(), imo);
				}
				if (auto* ms = MissionState::get_current())
				{
					ms->visited_interface_box();
				}
			}

			if (!madeKnown)
			{
				madeKnown = true;
				if (auto* piow = PilgrimageInstanceOpenWorld::get())
				{
					piow->mark_pilgrimage_device_direction_known(imo);
				}
			}
		}

		if (!self->depleted || self->energyHandling.transferring)
		{
			self->update_object_in_coil(imo);

			Energy prevEnergy = self->energy;

			{
				MODULE_OWNER_LOCK_FOR_IMO(imo, TXT("get values for energy kiosk"));
				// restore from state variable
				{
					self->energy = imo->get_variables().get_value<Energy>(NAME(energyInStorage), self->energy);
					self->maxEnergy = imo->get_variables().get_value<Energy>(NAME(maxEnergy), self->maxEnergy);
					self->maxEnergy = max(self->maxEnergy, self->energy);
				}
				// transfer energy (run just to stop the process if not stopped
				{
					self->transfer_energy(imo, LATENT_DELTA_TIME, ! self->depleted);
				}
				// store as state variable
				{
					imo->access_variables().access<Energy>(NAME(energyInStorage)) = self->energy;
				}
			}

			if (prevEnergy != self->energy)
			{
				self->updateDisplay = true;
				self->displayMoveLines = 0;
			}

			if (! self->depleted && self->energy >= self->maxEnergy)
			{
				self->depleted = true;
				if (auto* s = imo->get_sound())
				{
					s->play_sound(NAME(done));
				}
				if (auto* ms = MissionState::get_current())
				{
					ms->refilled();
				}
				if (auto* piow = PilgrimageInstanceOpenWorld::get())
				{
					if (cellAt.is_set())
					{
						piow->mark_pilgrim_device_state_depleted(cellAt.get(), imo);
						piow->mark_pilgrimage_device_direction_known(imo);
					}
				}
			}
		}

		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(RefillBoxData);

bool RefillBoxData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool RefillBoxData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
