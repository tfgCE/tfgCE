#include "aiLogicTask_grabEnergy.h"

#include "..\..\..\game\game.h"
#include "..\..\..\modules\custom\health\mc_health.h"
#include "..\..\..\modules\gameplay\moduleEnergyQuantum.h"
#include "..\..\..\modules\gameplay\moduleEquipment.h"
#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\ai\aiTaskHandle.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\module\moduleSound.h"
#include "..\..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\object\object.h"
#include "..\..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\..\framework\world\room.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// sounds & temporary objects
DEFINE_STATIC_NAME(energyTaken);

// sockets
DEFINE_STATIC_NAME(grabEnergy);

// vars
DEFINE_STATIC_NAME(grabAnyEnergyButDontProcess);
DEFINE_STATIC_NAME(grabAnyEnergyAsHealth);

//

static bool find_closest(Framework::IModulesOwner* & closest, float & bestFit, Vector3 & bestToLocInClosestRoom, Vector3 loc, Vector3 dir, Framework::Room* inRoom, Framework::DoorInRoom* throughDoor, float distanceLeft)
{
	bool result = false;
	if (! inRoom)
	{
		return false;
	}
	for_every_ptr(obj, inRoom->get_objects())
	{
		auto* meq = obj->get_gameplay_as<ModuleEnergyQuantum>();
		if (meq && meq->has_energy())
		{
			Vector3 oLoc = obj->get_presence()->get_centre_of_presence_transform_WS().get_translation();
			float dist = (oLoc - loc).length(); // distance alone for now
			if (dist < distanceLeft)
			{
				float fit = -dist;
				if (!closest || bestFit < fit)
				{
					closest = obj;
					bestFit = fit;
					result = true;
					bestToLocInClosestRoom = loc - oLoc;
				}
			}
		}
	}

	for_every_ptr(door, inRoom->get_doors())
	{
		if (door != throughDoor &&
			door->is_visible() &&
			door->has_world_active_room_on_other_side())
		{
			Plane plane = door->get_plane();
			if (plane.get_in_front(loc) > 0.0f)
			{
				float dist = door->calculate_dist_of(loc);
				if (dist < distanceLeft)
				{
					if (find_closest(REF_ closest, REF_ bestFit, REF_ bestToLocInClosestRoom, door->get_other_room_transform().location_to_local(loc), door->get_other_room_transform().vector_to_local(dir), door->get_room_on_other_side(), door->get_door_on_other_side(), distanceLeft - dist))
					{
						result = true;
					}
				}
			}
		}
	}

	return result;
}

LATENT_FUNCTION(TeaForGodEmperor::AI::Logics::Tasks::grab_energy)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("grab energy"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(SafePtr<Framework::IModulesOwner>, object);
	LATENT_VAR(float, objectFit);
	LATENT_VAR(Vector3, objectToUs);

	LATENT_VAR(bool, grabAnyEnergyButDontProcess);
	LATENT_VAR(bool, grabAnyEnergyAsHealth);
	LATENT_VAR(bool, requiresEnergy);

	LATENT_VAR(int, grabEnergySocketIdx);

	auto * imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	{
		auto& var = imo->access_variables().find<bool>(NAME(grabAnyEnergyButDontProcess));
		grabAnyEnergyButDontProcess = var.get<bool>();
	}

	{
		auto& var = imo->access_variables().find<bool>(NAME(grabAnyEnergyAsHealth));
		grabAnyEnergyAsHealth = var.get<bool>();
	}

	grabEnergySocketIdx = imo->get_appearance()->find_socket_index(NAME(grabEnergy));

	while (true)
	{
		requiresEnergy = grabAnyEnergyButDontProcess;
		if (grabAnyEnergyAsHealth)
		{
			if (auto* h = imo->get_custom<CustomModules::Health>())
			{
				requiresEnergy |= h->get_health() < h->get_max_health();
			}
		}
		if (requiresEnergy)
		{
			// find objects close to us
			Framework::IModulesOwner* closest = nullptr;
			float bestFit = 0.0f;
			Vector3 loc;
			Vector3 dir;
			if (grabEnergySocketIdx != NONE)
			{
				Transform placement = imo->get_appearance()->calculate_socket_os(grabEnergySocketIdx);
				loc = placement.get_translation();
				dir = placement.get_axis(Axis::Y);
			}
			else
			{
				Transform placement = imo->get_presence()->get_centre_of_presence_transform_WS();
				loc = placement.get_translation();
				dir = (2.0f * placement.get_axis(Axis::Y) - placement.get_axis(Axis::Z)).normal();
			}
			Vector3 bestToLocInClosestRoom;
			find_closest(REF_ closest, REF_ bestFit, REF_ bestToLocInClosestRoom, loc, dir, imo->get_presence()->get_in_room(), nullptr, magic_number 1.0f);

			object = closest;
			objectFit = bestFit;
			objectToUs = bestToLocInClosestRoom;
		}
		else
		{
			object.clear();
		}
		if (object.is_set())
		{
			if (auto* eq = object->get_gameplay_as<ModuleEnergyQuantum>())
			{
				object->get_presence()->add_velocity_impulse(objectToUs * 2.0f * Game::get()->get_delta_time());
				if (auto* col = object->get_collision())
				{
					if (col->does_collide_with(fast_cast<Collision::ICollidableObject>(imo)))
					{
						Framework::IModulesOwner* wasObject = object.get();
						if (grabAnyEnergyButDontProcess)
						{
							EnergyQuantumContext eqc; // nothing, no receiver, we just want to clear energy through processing
							eq->process_energy_quantum(eqc, true);
						}
						else
						{
							EnergyQuantumContext eqc;
							eqc.energy_type_override(grabAnyEnergyAsHealth ? Optional<EnergyType::Type>(EnergyType::Health) : NP);
							if (auto* meq = imo->get_gameplay_as<ModuleEquipment>())
							{
								eqc.side_effects_receiver(imo);
								meq->ready_energy_quantum_context(eqc);
							}
							else
							{
								eqc.receiver(imo);
							}
							eq->process_energy_quantum(eqc, true);
						}
						if (auto* s = imo->get_sound())
						{
							s->play_sound(NAME(energyTaken));
						}
						if (wasObject)
						{
							if (auto * to = wasObject->get_temporary_objects())
							{
								to->spawn_attached_to(NAME(energyTaken), imo, Framework::ModuleTemporaryObjects::SpawnParams().at_socket(NAME(energyTaken)));
							}
						}
						object.clear();
					}
				}
			}
		}
		//
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();

	LATENT_END_CODE();
	LATENT_RETURN();
}

