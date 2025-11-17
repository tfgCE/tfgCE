#include "aiLogic_pilgrimHand.h"

#include "aiLogic_pilgrim.h"

#include "..\perceptionRequests\aipr_FindClosest.h"

#include "..\..\teaForGod.h"
#include "..\..\utils.h"

#include "..\..\game\game.h"
#include "..\..\game\gameSettings.h"
#include "..\..\game\modes\gameMode_Pilgrimage.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_grabable.h"
#include "..\..\modules\custom\mc_pickup.h"
#include "..\..\modules\custom\health\mc_health.h"
#include "..\..\modules\custom\health\mc_healthRegen.h"
#include "..\..\modules\gameplay\moduleEnergyQuantum.h"
#include "..\..\modules\gameplay\moduleOrbItem.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\gameplay\equipment\me_gun.h"
#include "..\..\tutorials\tutorial.h"
#include "..\..\tutorials\tutorialSystem.h"

#include "..\..\..\core\vr\iVR.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\appearance\controllers\ac_particlesUtils.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\display\displayText.h"
#include "..\..\..\framework\display\displayUtils.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\module\custom\mc_display.h"
#include "..\..\..\framework\game\gameInput.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"
//#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
//#include "..\..\..\framework\module\moduleAppearance.h"
//#include "..\..\..\framework\object\actor.h"
//#include "..\..\..\framework\world\environment.h"
//#include "..\..\..\framework\world\room.h"

#include "..\..\..\framework\library\library.h" // !@# demo

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

DEFINE_STATIC_NAME(hand);
DEFINE_STATIC_NAME(right);
DEFINE_STATIC_NAME(left);
DEFINE_STATIC_NAME(cross);

// sounds & temporary objects
DEFINE_STATIC_NAME(energyTaken);
DEFINE_STATIC_NAME(orbItemTaken);
DEFINE_STATIC_NAME(paralysed);

// ai messages
#ifdef WITH_ENERGY_PULLING
DEFINE_STATIC_NAME(pullEnergy);
#endif
DEFINE_STATIC_NAME_STR(paralyseEnergyQuantum, TXT("paralyse energy quantum"));

// tutorial highlights
DEFINE_STATIC_NAME_STR(displaysHealth, TXT("displays:health"));
DEFINE_STATIC_NAME_STR(displaysHealthMain, TXT("displays:health:main"));
DEFINE_STATIC_NAME_STR(displaysHealthBackup, TXT("displays:health:backup"));
DEFINE_STATIC_NAME_STR(displaysWeapon, TXT("displays:weapon"));
DEFINE_STATIC_NAME_STR(displaysWeaponAmmo, TXT("displays:weapon:ammo"));
DEFINE_STATIC_NAME_STR(displaysWeaponEnergy, TXT("displays:weapon:energy"));
DEFINE_STATIC_NAME_STR(displaysWeaponMagazine, TXT("displays:weapon:magazine"));
DEFINE_STATIC_NAME_STR(displaysWeaponStorage, TXT("displays:weapon:storage"));
DEFINE_STATIC_NAME_STR(displaysWeaponChamber, TXT("displays:weapon:chamber"));

//

REGISTER_FOR_FAST_CAST(PilgrimHand);

PilgrimHand::PilgrimHand(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const* _logicData)
: base(_mind, _logicData, execute_logic)
, pilgrimHandData(fast_cast<PilgrimHandData>(_logicData))
{
}

PilgrimHand::~PilgrimHand()
{
	if (display)
	{
		display->set_on_advance_display(nullptr, nullptr);
	}
}

void PilgrimHand::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (isValid)
	{
		bool inputProvided = false;
		input.clear();
		if (auto * game = Game::get_as<Game>())
		{
			if (Player* player = fast_cast<Player>(&game->access_player()))
			{
				auto const & useInput = hand == Hand::Right ? player->get_input_vr_right() : player->get_input_vr_left();

				an_assert(pilgrimHandData);

				pilgrimHandData->inputDefinition.process(input, useInput);
				inputProvided = true;
			}
		}

		if (!inputProvided)
		{
			todo_note(TXT("what to do if input not provided?"));
		}

#ifdef USE_KEYBOARD_INPUT
#ifdef AN_STANDARD_INPUT
		// pretend we have actual input, already translated
		DEFINE_STATIC_NAME(cursor);
		input.set_mouse(NAME(cursor), Vector2::zero);
		DEFINE_STATIC_NAME(cursorFakeTouch);
		input.remove_button(NAME(cursorFakeTouch));
		if (auto * game = Game::get_as<Game>())
		{
			if (game->does_debug_control_player())
			{
				if (Player * player = fast_cast<Player>(&game->access_player()))
				{
					if (::System::Input::get()->is_key_pressed(::System::Key::N1) ||
						::System::Input::get()->is_key_pressed(::System::Key::N2))
					{
						player->access_input_non_vr().disable();
						if ((::System::Input::get()->is_key_pressed(::System::Key::N1) && hand == Hand::Left) ||
							(::System::Input::get()->is_key_pressed(::System::Key::N2) && hand == Hand::Right))
						{
							DEFINE_STATIC_NAME(cursorAbsolute01);
							DEFINE_STATIC_NAME(cursorPress);
							DEFINE_STATIC_NAME(cursorTouch);
							DEFINE_STATIC_NAME(cursorFakeTouch);
							DEFINE_STATIC_NAME(cursorRelative);
							DEFINE_STATIC_NAME(stick);
							DEFINE_STATIC_NAME(press);

							input.set_mouse(NAME(cursor), ::System::Input::get()->get_mouse_relative_location());
							input.remove_stick(NAME(cursorAbsolute01));
							input.set_button_state(NAME(cursorPress), ::System::Input::get()->is_mouse_button_pressed(0) ? 1.0f : 0.0f);
							if (!::System::Input::get()->is_key_pressed(::System::Key::N3))
							{
								input.set_button_state(NAME(cursorTouch), 1.0f);
							}
							if (::System::Input::get()->is_key_pressed(::System::Key::N4))
							{
								// to test virtual displays
								input.remove_button(NAME(cursorTouch));
								input.set_button_state(NAME(cursorFakeTouch), 1.0f);
							}
							input.set_mouse(NAME(cursorRelative), ::System::Input::get()->get_mouse_relative_location() * 0.1f);
							Vector2 stick = Vector2::zero;
							if (::System::Input::get()->is_key_pressed(::System::Key::Q))
							{
								stick.x -= 1.0f;
							}
							if (::System::Input::get()->is_key_pressed(::System::Key::E))
							{
								stick.x += 1.0f;
							}
							input.set_stick(NAME(stick), stick);
							input.set_button_state(NAME(press), ::System::Input::get()->is_key_pressed(::System::Key::Space) ? 1.0f : 0.0f);
						}
					}
					else
					{
						player->access_input_non_vr().enable();
					}
				}
			}
		}
#endif
#endif

		if (display)
		{
			update_interface_continuous(_deltaTime);
		}
	}
}

void PilgrimHand::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);

	isValid = pilgrimHandData != nullptr;

	an_assert(!display);
	display = nullptr;
	if (auto* mind = get_mind())
	{
		if (auto* imo = mind->get_owner_as_modules_owner())
		{
			if (auto* displayModule = imo->get_custom<::Framework::CustomModules::Display>())
			{
				display = displayModule->get_display();
				display->set_on_advance_display(this, 
					[this](Framework::Display* _display, float _deltaTime)
					{
						_display->process_input(input, _deltaTime);
					}
				);
			}
		}
	}
	Name vrHandName = hand == Hand::Right ? NAME(right) : Name::invalid();
	vrHandName = _parameters.get_value<Name>(NAME(hand), vrHandName);
	if (vrHandName == NAME(right))
	{
		hand = Hand::Right;
	}
	else if (vrHandName == NAME(left))
	{
		hand = Hand::Left;
	}
	else
	{
		error(TXT("no hand provided (right or left"));
		isValid = false;
	}
}

static float const segRadius = 0.15f;

// important:
//	loc is loc in previous room or in this one (used for string pulling)
//	string pulled loc is location inside this room on entering door
//		therefore with every recursive call:
//			stringPulledLoc becomes loc (we then have some string pulling)
//			stringPulledLoc found inside hole becomes new stringPulledLoc (bad approximation!)
// more:
//	distances left are from door only
//	previousRoomsDistance is distance covered in previous rooms (approximated badly) (used to calculate fit)
static bool find_closest_pickup_or_grabable(REF_ Framework::IModulesOwner* & closest, REF_ float & bestFit, Framework::IModulesOwner* ignoreObject, Vector3 const & loc, Vector3 const & stringPulledLoc, Vector3 dir, Framework::Room* inRoom, Framework::DoorInRoom* throughDoor, float previousRoomsDistance, float pickupDistanceLeft, float pickupFromPocketDistanceLeft, float grabDistanceLeft, Framework::DoorInRoomArrayPtr & throughDoorsWorking, Framework::DoorInRoomArrayPtr & throughDoorsBest)
{
	bool result = false;
	Segment seg(loc + dir * segRadius, loc + dir * 1.0f);
	for_every_ptr(obj, inRoom->get_objects())
	{
		if (obj == ignoreObject)
		{
			continue;
		}
		auto* pickup = obj->get_custom<CustomModules::Pickup>();
		auto* grabable = obj->get_custom<CustomModules::Grabable>();
		if ((pickup && pickup->can_be_picked_up()) ||
			(grabable && grabable->can_be_grabbed()))
		{
			Vector3 oLoc = obj->get_presence()->get_centre_of_presence_transform_WS().get_translation();
			if (pickup && pickup->get_pickup_reference_socket().is_valid())
			{
				int	socketIdx = obj->get_appearance()->find_socket_index(pickup->get_pickup_reference_socket());
				if (socketIdx != NONE)
				{
					oLoc = obj->get_presence()->get_placement().location_to_world(obj->get_appearance()->calculate_socket_os(socketIdx).get_translation());
				}
			}
			if (grabable && grabable->get_grabable_axis_socket_name().is_valid())
			{
				int	socketIdx = obj->get_appearance()->find_socket_index(grabable->get_grabable_axis_socket_name());
				if (socketIdx != NONE)
				{
					oLoc = obj->get_presence()->get_placement().location_to_world(obj->get_appearance()->calculate_socket_os(socketIdx).get_translation());
				}
			}
			if (grabable && grabable->get_grabable_dial_socket_name().is_valid())
			{
				int	socketIdx = obj->get_appearance()->find_socket_index(grabable->get_grabable_dial_socket_name());
				if (socketIdx != NONE)
				{
					oLoc = obj->get_presence()->get_placement().location_to_world(obj->get_appearance()->calculate_socket_os(socketIdx).get_translation());
				}
			}
			Vector3 compLoc = loc;
			if (throughDoor)
			{
				compLoc = throughDoor->find_inside_hole_for_string_pulling(loc, oLoc);
			}
			float dist = (oLoc - compLoc).length(); // distance in this room only
			if (auto* me = obj->get_gameplay_as<ModuleEquipment>())
			{
				if (me->is_main_equipment())
				{
					// harder to grab main equipment
					dist *= 2.0f;
				}
			}
			if ((pickup && dist < (pickup->is_in_pocket()? pickupFromPocketDistanceLeft : pickupDistanceLeft)) ||
				(grabable && dist < grabDistanceLeft))
			{
				if (dist > 0.05f && Vector3::dot((oLoc - loc).normal(), dir) > 0.2f) // has to be below the hand
				{
					float fit = -(dist + previousRoomsDistance) - max(0.0f, seg.get_distance(oLoc) - segRadius) * 2.0f;
					if (!closest || bestFit < fit)
					{
						closest = obj;
						bestFit = fit;
						result = true;
						throughDoorsBest.copy_from(throughDoorsWorking);
					}
				}
			}
		}
	}
	
	for_every_ptr(door, inRoom->get_doors())
	{
		if (door != throughDoor &&
			door->is_visible() &&
			door->get_world_active_room_on_other_side())
		{
			Plane plane = door->get_plane();
			if (plane.get_in_front(loc) > 0.0f)
			{
				float dist = door->calculate_dist_of(stringPulledLoc);
				if (dist < pickupDistanceLeft ||
					dist < grabDistanceLeft)
				{
					throughDoorsWorking.push_door(door);
					Vector3 onDoorLoc = door->find_inside_hole(stringPulledLoc);
					float wholeRoomsDistance = previousRoomsDistance;
					wholeRoomsDistance += (onDoorLoc - stringPulledLoc).length();
					if (find_closest_pickup_or_grabable(REF_ closest, REF_ bestFit, ignoreObject, door->get_other_room_transform().location_to_local(stringPulledLoc), door->get_other_room_transform().location_to_local(onDoorLoc), door->get_other_room_transform().vector_to_local(dir), door->get_world_active_room_on_other_side(), door->get_door_on_other_side(), wholeRoomsDistance, pickupDistanceLeft - dist, pickupFromPocketDistanceLeft - dist, grabDistanceLeft - dist, throughDoorsWorking, throughDoorsBest))
					{
						result = true;
					}
					throughDoorsWorking.pop_door();
				}
			}
		}
	}

	return result;
}

#ifdef WITH_ENERGY_PULLING
static bool find_closest_orb(Framework::IModulesOwner* & closest, float & bestFit, Vector3 & bestToLocInClosestRoom, Vector3 const & loc, Vector3 const & stringPulledLoc, Vector3 dir, Framework::Room* inRoom, Framework::DoorInRoom* throughDoor, float previousRoomsDistance, float distanceLeft)
{
	if (!inRoom)
	{
		return false;
	}
	bool result = false;
	Segment seg(loc + dir * segRadius, loc + dir * 1.0f);
	for_every_ptr(obj, inRoom->get_objects())
	{
		auto* meq = obj->get_gameplay_as<ModuleEnergyQuantum>();
		auto* oi = obj->get_gameplay_as<ModuleOrbItem>();
		if ((meq && meq->has_energy()) || (oi && ! oi->was_taken()))
		{
			Vector3 oLoc = obj->get_presence()->get_centre_of_presence_transform_WS().get_translation();
			Vector3 compLoc = loc;
			if (throughDoor)
			{
				compLoc = throughDoor->find_inside_hole_for_string_pulling(loc, oLoc, NP, 0.2f);
			}
			float dist = (oLoc - compLoc).length(); // distance in this room only
			if (dist < distanceLeft)
			{
				float fit = -(dist + previousRoomsDistance) - max(0.0f, seg.get_distance(oLoc) - segRadius) * 2.0f;
				if (!closest || bestFit < fit)
				{
					closest = obj;
					bestFit = fit;
					result = true;
					bestToLocInClosestRoom = (compLoc - oLoc).normal();
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
				float dist = door->calculate_dist_of(stringPulledLoc);
				if (dist < distanceLeft)
				{
					Vector3 onDoorLoc = door->find_inside_hole(stringPulledLoc, 0.2f);
					float wholeRoomsDistance = previousRoomsDistance;
					wholeRoomsDistance += (onDoorLoc - stringPulledLoc).length();
					if (find_closest_orb(REF_ closest, REF_ bestFit, REF_ bestToLocInClosestRoom, door->get_other_room_transform().location_to_local(stringPulledLoc), door->get_other_room_transform().location_to_local(onDoorLoc), door->get_other_room_transform().vector_to_local(dir), door->get_world_active_room_on_other_side(), door->get_door_on_other_side(), wholeRoomsDistance, distanceLeft - dist))
					{
						result = true;
					}
				}
			}
		}
	}

	return result;
}
#else
static bool find_closest_energy_quantum(float _deltaTime, Framework::IModulesOwner*& closest, float& bestFit, Vector3& bestToLocInClosestRoom, Vector3 const& loc, Vector3 const& stringPulledLoc, Vector3 dir, Framework::Room* inRoom, Framework::DoorInRoom* throughDoor, float previousRoomsDistance, float distanceLeft)
{
	if (!inRoom)
	{
		return false;
	}
	bool result = false;
	Segment seg(loc + dir * segRadius, loc + dir * 1.0f);
	for_every_ptr(obj, inRoom->get_objects())
	{
		auto* meq = obj->get_gameplay_as<ModuleEnergyQuantum>();
		if ((meq && meq->has_energy()))
		{
			Vector3 oLoc = obj->get_presence()->get_centre_of_presence_transform_WS().get_translation();
			Vector3 compLoc = loc;
			if (throughDoor)
			{
				compLoc = throughDoor->find_inside_hole_for_string_pulling(loc, oLoc, NP, 0.2f);
			}
			float dist = (oLoc - compLoc).length(); // distance in this room only
			if (dist < distanceLeft)
			{
				{
					Vector3 velocityRequested = 0.6f * ((compLoc - oLoc).normal() + 0.2f * Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f))).normal();
					Vector3 currentVelocity = obj->get_presence()->get_velocity_linear();
					Vector3 diff = velocityRequested - currentVelocity;
					obj->get_presence()->add_velocity_impulse(diff.normal() * (diff.length() * 2.0f * _deltaTime));
				}
				float fit = -(dist + previousRoomsDistance) - max(0.0f, seg.get_distance(oLoc) - segRadius) * 2.0f;
				if (!closest || bestFit < fit)
				{
					closest = obj;
					bestFit = fit;
					result = true;
					bestToLocInClosestRoom = (compLoc - oLoc).normal();
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
				float dist = door->calculate_dist_of(stringPulledLoc);
				if (dist < distanceLeft)
				{
					Vector3 onDoorLoc = door->find_inside_hole(stringPulledLoc, 0.2f);
					float wholeRoomsDistance = previousRoomsDistance;
					wholeRoomsDistance += (onDoorLoc - stringPulledLoc).length();
					if (find_closest_energy_quantum(_deltaTime, REF_ closest, REF_ bestFit, REF_ bestToLocInClosestRoom, door->get_other_room_transform().location_to_local(stringPulledLoc), door->get_other_room_transform().location_to_local(onDoorLoc), door->get_other_room_transform().vector_to_local(dir), door->get_world_active_room_on_other_side(), door->get_door_on_other_side(), wholeRoomsDistance, distanceLeft - dist))
					{
						result = true;
					}
				}
			}
		}
	}

	return result;
}
#endif

#ifdef WITH_ENERGY_PULLING
static void pull_each_orb_towards(::Framework::IModulesOwner* _imo, Hand::Type _hand, Vector3 const & loc, Vector3 const & stringPulledLoc, Framework::Room* inRoom, Framework::DoorInRoom* throughDoor, float distanceLeft, int _depth = 0)
{
	if (distanceLeft <= 0.0f || !inRoom)
	{
		return;
	}

	float modifyDistanceLeft = throughDoor && AVOID_CALLING_ACK_ ! inRoom->was_recently_seen_by_player()? PilgrimBlackboard::get_pull_energy_distance(_imo, _hand, false) - PilgrimBlackboard::get_pull_energy_distance(_imo, _hand, true) : 0;

	for_every_ptr(obj, inRoom->get_objects())
	{
		auto* meq = obj->get_gameplay_as<ModuleEnergyQuantum>();
		auto* oi = obj->get_gameplay_as<ModuleOrbItem>();
		if ((meq && meq->has_energy()) || (oi && !oi->was_taken()))
		{
			Vector3 oLoc = obj->get_presence()->get_centre_of_presence_transform_WS().get_translation();
			Vector3 compLoc = loc;
			if (throughDoor)
			{
				compLoc = throughDoor->find_inside_hole_for_string_pulling(loc, oLoc, NP, 0.2f);
			}
			float dist = (oLoc - compLoc).length(); // distance in this room only
			if (dist < distanceLeft + modifyDistanceLeft)
			{
				obj->get_presence()->add_velocity_impulse(1.4f * ((compLoc - oLoc).normal() + 0.2f * Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f))).normal() - obj->get_presence()->get_velocity_linear() * 0.5f);
				obj->set_instigator(_imo->get_top_instigator());
				if (meq)
				{
					meq->mark_pulled_by_pilgrim(distanceLeft);
				}
			}
		}
	}

	for_every_ptr(door, inRoom->get_doors())
	{
		if (door != throughDoor &&
			door->is_visible() &&
			door->get_room_on_other_side())
		{
			Plane plane = door->get_plane();
			if (plane.get_in_front(loc) > 0.0f)
			{
				float dist = door->calculate_dist_of(stringPulledLoc);
				if (dist < distanceLeft + modifyDistanceLeft && _depth < 6)
				{
					Vector3 onDoorLoc = door->find_inside_hole(stringPulledLoc, 0.2f);
					pull_each_orb_towards(_imo, _hand, door->get_other_room_transform().location_to_local(stringPulledLoc), door->get_other_room_transform().location_to_local(onDoorLoc), door->get_world_active_room_on_other_side(), door->get_door_on_other_side(), distanceLeft - dist, _depth + 1);
				}
			}
		}
	}
}
#endif

LATENT_FUNCTION(PilgrimHand::handle_pickups)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

#ifdef WITH_ENERGY_PULLING
	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	LATENT_VAR(bool, pullEnergy);

	LATENT_VAR(SafePtr<Framework::IModulesOwner>, objectOrb);
	LATENT_VAR(Vector3, objectOrbToUs);
#endif

	LATENT_VAR(SafePtr<Framework::IModulesOwner>, objectPickup);

	LATENT_VAR(int, throughDoorsLimit);
	LATENT_VAR(Framework::StaticDoorArray, throughDoorsWorking);
	LATENT_VAR(Framework::StaticDoorArray, throughDoorsBest);

	auto * logic = mind->get_logic();
	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<PilgrimHand>(logic);

	LATENT_BEGIN_CODE();

#ifdef WITH_ENERGY_PULLING
	pullEnergy = false;
	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(pullEnergy), [&pullEnergy](Framework::AI::Message const & _message)
		{
			pullEnergy = true;
		}
		);
	}
#endif

	throughDoorsLimit = 32;
	throughDoorsWorking.set_size(throughDoorsLimit);
	throughDoorsBest.set_size(throughDoorsLimit);

	while (true)
	{
		if (!TutorialSystem::check_active() || TutorialSystem::get()->should_allow_pickup_orbs())
		{
			Transform placement = imo->get_presence()->get_centre_of_presence_transform_WS();
			Vector3 loc = placement.get_translation();
			Vector3 dir = (0.8f * placement.get_axis(Axis::Y) - 2.0f * placement.get_axis(Axis::Z)).normal();

			for_count(int, i, 2)
			{
				// find objects close to us
				Framework::IModulesOwner* closest = nullptr;
				float bestFit = 0.0f;
				Vector3 bestToLocInClosestRoom;
				if (i == 0)
				{
#ifdef WITH_ENERGY_PULLING
					find_closest_orb(REF_ closest, REF_ bestFit, REF_ bestToLocInClosestRoom, loc, loc, dir, imo->get_presence()->get_in_room(), nullptr, 0.0f, GameSettings::get().settings.energyAttractDistance);

					objectOrb = closest;
					objectOrbToUs = bestToLocInClosestRoom.normal();
#endif
				}
				else if (imo->get_presence()->get_in_room())
				{
					ModulePilgrim* mp = nullptr;
					if (auto* top = imo->get_top_instigator())
					{
						mp = top->get_gameplay_as<ModulePilgrim>();
					}
							
					Framework::DoorInRoomArrayPtr throughDoorsWorkingArrayPtr(throughDoorsWorking.get_data(), throughDoorsLimit);
					Framework::DoorInRoomArrayPtr throughDoorsBestArrayPtr(throughDoorsBest.get_data(), throughDoorsLimit);
					// but ignore main equipment for this hand - we shouldn't consider getting main equipment here
					find_closest_pickup_or_grabable(REF_ closest, REF_ bestFit, mp? mp->get_main_equipment(self->hand) : nullptr, loc, loc, dir, imo->get_presence()->get_in_room(), nullptr, 0.0f, GameSettings::get().settings.pickupDistance, GameSettings::get().settings.pickupFromPocketDistance, GameSettings::get().settings.grabDistance, throughDoorsWorkingArrayPtr, throughDoorsBestArrayPtr);

					objectPickup = closest;

					if (mp)
					{
						mp->set_object_to_grab_proposition(self->hand, objectPickup.get(), &throughDoorsBestArrayPtr);
					}
				}
			}
#ifdef WITH_ENERGY_PULLING
			if (pullEnergy && ! self->blockPulling)
			{
				pull_each_orb_towards(imo, self->hand, loc, loc, imo->get_presence()->get_in_room(), nullptr, PilgrimBlackboard::get_pull_energy_distance(imo, self->hand, true));
				pullEnergy = false;
			}
#endif
		}
#ifdef WITH_ENERGY_PULLING
		if (objectOrb.is_set() &&
			(!TutorialSystem::check_active() || TutorialSystem::get()->should_allow_pickup_orbs()))
		{
			auto* eq = objectOrb->get_gameplay_as<ModuleEnergyQuantum>();
			auto* oi = objectOrb->get_gameplay_as<ModuleOrbItem>();
			if (eq || oi)
			{
				if (!self->blockPulling)
				{
					objectOrb->get_presence()->add_velocity_impulse(objectOrbToUs * 1.0f * Game::get()->get_delta_time());
					objectOrb->set_instigator(imo->get_top_instigator());
				}
				if (auto* col = objectOrb->get_collision())
				{
					if (auto * top = imo->get_top_instigator())
					{
						if (auto* mp = top->get_gameplay_as<ModulePilgrim>())
						{
							Framework::IModulesOwner* colliderWithHealth = nullptr;
							if (mp->does_hand_collide_with(self->hand, col, OUT_ &colliderWithHealth))
							{
								if (eq)
								{
									Framework::IModulesOwner* ammoReceiver = mp->get_main_equipment(self->hand); // always main equipment!
									Framework::IModulesOwner* sideEffectsHealthReceiver = colliderWithHealth;
									if (sideEffectsHealthReceiver && sideEffectsHealthReceiver->get_custom<CustomModules::Health>())
									{
										// sideEffectsHealthReceiver is ok
									}
									else
									{
										sideEffectsHealthReceiver = top;
									}
									Framework::IModulesOwner* wasObject = objectOrb.get();
									eq->process_energy_quantum(EnergyQuantumContext()
										.with_hand(self->hand)
										.adjust_by(top)
										.ammo_receiver(ammoReceiver)
										.health_receiver(top)
										.side_effects_receiver(imo)
										.side_effects_health_receiver(sideEffectsHealthReceiver),
										true);
									// will auto disappear on lack of energy
									{
										if (auto* s = imo->get_sound())
										{
											s->play_sound(NAME(energyTaken));
										}
										if (wasObject)
										{
											if (auto * to = wasObject->get_temporary_objects())
											{
												// we attach from taken object to us, that's why here's spawn_attached_to
												to->spawn_attached_to(NAME(energyTaken), imo, NAME(energyTaken));
											}
										}
									}
									objectOrb.clear();
								}
								else if (oi)
								{
									if (oi->process_taken(mp))
									{
										Framework::IModulesOwner* wasObject = objectOrb.get();
										{
											if (auto* s = imo->get_sound())
											{
												s->play_sound(NAME(orbItemTaken));
											}
											if (wasObject)
											{
												if (auto* to = wasObject->get_temporary_objects())
												{
													// we attach from taken object to us, that's why here's spawn_attached_to
													to->spawn_attached_to(NAME(orbItemTaken), imo, NAME(orbItemTaken));
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
#else
		// run this code only if energy quantums exist
		if (ModuleEnergyQuantum::does_any_exist())
		{
			if (auto* top = self->get_mind()->get_owner_as_modules_owner()->get_top_instigator())
			{
				if (ModulePilgrim* mp = top->get_gameplay_as<ModulePilgrim>())
				{
					Transform placement = imo->get_presence()->get_centre_of_presence_transform_WS();
					Vector3 loc = placement.get_translation();
					Vector3 dir = (0.8f * placement.get_axis(Axis::Y) - 2.0f * placement.get_axis(Axis::Z)).normal();

					Framework::IModulesOwner* closest = nullptr;
					float bestFit = 0.0f;
					Vector3 bestToLocInClosestRoom;
					float deltaTime = LATENT_DELTA_TIME;
					find_closest_energy_quantum(deltaTime, REF_ closest, REF_ bestFit, REF_ bestToLocInClosestRoom, loc, loc, dir, imo->get_presence()->get_in_room(), nullptr, 0.0f, 0.4f);

					if (auto* cimo = closest)
					{
						if (auto* eq = cimo->get_gameplay_as<ModuleEnergyQuantum>())
						{
							Framework::IModulesOwner* colliderWithHealth = nullptr;
							if (mp->does_hand_collide_with(self->hand, closest->get_collision(), OUT_ & colliderWithHealth))
							{
								Framework::IModulesOwner* handIMO = mp->get_hand(self->hand); // hands are receivers now
								Framework::IModulesOwner* wasObject = cimo;
								eq->process_energy_quantum(EnergyQuantumContext()
									.with_hand(self->hand)
									.adjust_by(top)
									.ammo_receiver(handIMO)
									.health_receiver(handIMO)
									.side_effects_receiver(imo)
									.side_effects_health_receiver(handIMO),
									true);
								// will auto disappear on lack of energy
								{
									if (auto* s = imo->get_sound())
									{
										s->play_sound(NAME(energyTaken));
									}
									if (wasObject)
									{
										if (auto * to = wasObject->get_temporary_objects())
										{
											// we attach from taken object to us, that's why here's spawn_attached_to
											to->spawn_attached_to(NAME(energyTaken), imo, Framework::ModuleTemporaryObjects::SpawnParams().at_socket(NAME(energyTaken)));
										}
									}
								}
							}
						}
					}
				}
			}
		}
#endif
		//
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(PilgrimHand::be_paralysed)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(SafePtr<::Framework::IModulesOwner>, paralysedTO);
	LATENT_VAR(float, paralysedPulseTimeLeft);

	auto * logic = mind->get_logic();
	auto * self = fast_cast<PilgrimHand>(logic);

	paralysedPulseTimeLeft -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	paralysedPulseTimeLeft = 0.0f;

	{
		if (auto * top = self->get_mind()->get_owner_as_modules_owner()->get_top_instigator())
		{
			if (ModulePilgrim * mp = top->get_gameplay_as<ModulePilgrim>())
			{
				mp->paralyse_hand(self->hand, true);
			}
		}
	}

	if (auto* imo = self->get_mind()->get_owner_as_modules_owner())
	{
		if (auto* to = imo->get_temporary_objects())
		{
			paralysedTO = to->spawn(NAME(paralysed));
		}
	}

	while (true)
	{
		if (paralysedPulseTimeLeft <= 0.0f)
		{
			paralysedPulseTimeLeft = Random::get(self->pilgrimHandData->paralysedPulseInterval);
			if (auto* vr = VR::IVR::get())
			{
				vr->trigger_pulse(Hand::get_vr_hand_index(self->hand), self->pilgrimHandData->paralysedPulses[Random::get_int(self->pilgrimHandData->paralysedPulses.get_size())]);
			}
		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	LATENT_ON_END();

	{
		if (auto * top = self->get_mind()->get_owner_as_modules_owner()->get_top_instigator())
		{
			if (ModulePilgrim * mp = top->get_gameplay_as<ModulePilgrim>())
			{
				mp->paralyse_hand(self->hand, false);
			}
		}
	}

	Framework::ParticlesUtils::desire_to_deactivate(paralysedTO.get());

	LATENT_END_CODE();

	LATENT_RETURN();
}

LATENT_FUNCTION(PilgrimHand::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, pickupTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, paralyseTask);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);
	LATENT_VAR(float, shouldBeParalysedFor);

	shouldBeParalysedFor -= LATENT_DELTA_TIME;

	LATENT_BEGIN_CODE();

	LATENT_WAIT(Random::get_float(0.1f, 0.6f));

#ifdef AN_DEVELOPMENT
	while (Framework::is_preview_game())
	{
		LATENT_YIELD();
	}
#endif

	shouldBeParalysedFor = 0.0f;

	messageHandler.use_with(mind);
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
		messageHandler.set(NAME(paralyseEnergyQuantum), [&shouldBeParalysedFor](Framework::AI::Message const & _message)
		{
			shouldBeParalysedFor = 10.0f;
		}
		);
#endif
	}

	while (true)
	{
		{
			::Framework::AI::LatentTaskInfoWithParams taskInfo;
			taskInfo.clear();
			taskInfo.propose(AI_LATENT_TASK_FUNCTION(handle_pickups));
			if (taskInfo.is_proposed() &&
				pickupTask.can_start(taskInfo))
			{
				pickupTask.start_latent_task(mind, taskInfo);
			}
		}
		{
			::Framework::AI::LatentTaskInfoWithParams taskInfo;
			taskInfo.clear();
			if (shouldBeParalysedFor > 0.0f)
			{
				taskInfo.propose(AI_LATENT_TASK_FUNCTION(be_paralysed));
			}
			if (taskInfo.is_proposed())
			{
				if (paralyseTask.can_start(taskInfo))
				{
					paralyseTask.start_latent_task(mind, taskInfo);
				}
			}
			else if (paralyseTask.is_running())
			{
				paralyseTask.stop();
			}
		}

		LATENT_YIELD();
	}

	LATENT_ON_BREAK();

	LATENT_END_CODE();
	LATENT_RETURN();
}

void PilgrimHand::update_interface(float _deltaTime)
{
	if (!inControlOfDisplay || !display)
	{
		return;
	}

	display->redraw_controls();
}

void PilgrimHand::update_interface_continuous(float _deltaTime)
{
	bool drewSomething = false;

#ifdef FOR_TSHIRTS
	{
		Framework::DisplayUtils::clear_all(display, Name::invalid());
		{
			if (auto* tp = Framework::Library::get_current()->get_texture_parts().find(Framework::LibraryName(String(TXT("pilgrims:pray kill compute")))))
			{
				Framework::DisplayDrawCommands::TexturePart* part = new Framework::DisplayDrawCommands::TexturePart();
				part->use(tp)
					->at(VectorInt2(0,0));
				display->add(part);
			}
		}

		display->draw_all_commands_immediately();
		return;
	}
#endif

	if (!inControlOfDisplay || !display || !display->is_updated_continuously())
	{
		// reset
		ui = UI();
	}
	else
	{
		Framework::IModulesOwner* newEq = nullptr;
		ModulePilgrim* mp = nullptr;
		if (auto * top = get_mind()->get_owner_as_modules_owner()->get_top_instigator())
		{
			if ((mp = top->get_gameplay_as<ModulePilgrim>()))
			{
				if (auto* eq = mp->get_hand_equipment(hand))
				{
					newEq = eq;
				}
				else if (auto* eq = mp->get_main_equipment(hand))
				{
					// if no equipment available, get main
					newEq = eq;
				}
			}
		}
		if (equipmentInHand != newEq)
		{
			equipmentInHand = newEq;
			if (ui.mode != UI::PrayKillCompute)
			{
				ui.active = false; // redraw
			}
		}

		if (display)
		{
			int rotateDisplay = 0;
			if (auto* owner = get_mind()->get_owner_as_modules_owner())
			{
				if (auto* pilgrim = owner->get_presence()->get_attached_to())
				{
					if (auto* pilgrimAI = pilgrim->get_ai())
					{
						if (auto* pilgrimLogic = fast_cast<Logics::Pilgrim>(pilgrimAI->get_mind()->get_logic()))
						{
							bool atStation = false;
							if (auto* game = Game::get_as<Game>())
							{
								if (auto* pilgrimage = fast_cast<GameModes::Pilgrimage>(game->get_mode()))
								{
									atStation = pilgrimage->linear__is_station(owner->get_presence()->get_in_room());
								}
							}

							/*
							if (atStation)
							{
								pray_kill_compute(1.0f);
								pray(PilgrimHandWord::Highlight);
							}
							else if (ui.atStation)
							{
								pray_kill_compute(2.0f);
								kill(PilgrimHandWord::Blink);
							}
							*/
							ui.atStation = atStation;

							if (equipmentInHand.is_set())
							{
								if (auto* gun = equipmentInHand->get_gameplay_as<ModuleEquipments::Gun>())
								{
									if (gun->get_time_since_last_shot() < 0.2f)
									{
										ui.prayKillComputeBlockedFor = max(ui.prayKillComputeBlockedFor, 0.1f);
									}
								}
								if (auto* eq = equipmentInHand->get_gameplay_as<ModuleEquipment>())
								{
									if (eq->get_time_since_last_kill() < 0.5f)
									{
										ui.prayKillComputeBlockedFor = 0.0f;
										pray_kill_compute(0.01f);
										pray(PilgrimHandWord::DontShow);
										kill(PilgrimHandWord::Highlight);
										compute(PilgrimHandWord::DontShow);
									}
								}
							}

							bool doesRequireDisplayExtra = false;
							if (equipmentInHand.is_set())
							{
								if (auto* eq = equipmentInHand->get_gameplay_as<ModuleEquipment>())
								{
									doesRequireDisplayExtra = eq->does_require_display_extra();
									if (doesRequireDisplayExtra)
									{
										// block it for a while, let equipment to display
										ui.prayKillComputeBlockedFor = 0.1f;
									}
								}
							}

							if (ui.prayKillComputeBlockedFor > 0.0f)
							{
								ui.prayKillComputeBlockedFor -= _deltaTime;
								if (ui.prayKillComputeBlockedFor <= 0.0f &&
									ui.prayKillComputeTimeLeft > 0.0f)
								{
									// go back
									ui.mode = UI::PrayKillCompute;
								}
								else if (ui.mode == UI::PrayKillCompute)
								{
									ui.mode = UI::Normal;
								}
							}

							if (ui.mode != UI::PrayKillCompute)
							{
								ui.mode = UI::Normal;
								if (doesRequireDisplayExtra)
								{
									ui.mode = UI::Equipment;
								}
							}

							if (!ui.active || ui.prevMode != ui.mode)
							{
								ui.active = false; // redraw
								Framework::DisplayUtils::clear_all(display, Name::invalid());
							}

							UI::Mode nextMode = ui.mode;

							bool healthIsActive = false;
							bool weaponIsActive = false;
							bool otherEquipmentIsActive = false;

							bool tutorialHighlightRequired = TutorialSystem::should_be_highlighted_now();
							bool forceRedrawDueToTutorialHighlight = (ui.wasTutorialHighlighting != tutorialHighlightRequired);
							ui.wasTutorialHighlighting = tutorialHighlightRequired;

							if (ui.mode == UI::Normal)
							{
								if (forceRedrawDueToTutorialHighlight)
								{
									display->add((new Framework::DisplayDrawCommands::CLS()));
								}

								// stats
								{
									bool sectionDistActive = pilgrimLogic->is_section_active();
									float sectionDist = pilgrimLogic->get_section_dist();
									float totalDist = pilgrimLogic->get_total_dist();

									if (ui.sectionDistDisplayed != sectionDist ||
										ui.sectionDistActive != sectionDistActive ||
										!ui.active ||
										forceRedrawDueToTutorialHighlight)
									{
										ui.sectionDistDisplayed = sectionDist;
										ui.sectionDistActive = sectionDistActive;
										Framework::DisplayDrawCommands::TextAt* text = new Framework::DisplayDrawCommands::TextAt();
										text->text(String::printf(pilgrimHandData->stats.sectionDistFormat.to_char(), sectionDist).get_right(pilgrimHandData->stats.sectionDistLength).to_char())
											->use_font(pilgrimHandData->stats.font.get())
											->at(offset_at(pilgrimHandData->stats.sectionDistAt))
											->length(pilgrimHandData->stats.sectionDistLength)
											->use_coordinates(Framework::DisplayCoordinates::Pixel);
										if (ui.sectionDistActive)
										{
											text->use_colourise_ink(pilgrimHandData->stats.sectionDistColourActive);
										}
										else
										{
											text->use_colourise_ink(pilgrimHandData->stats.sectionDistColourInactive);
										}
										display->add(text);
										drewSomething = true;
									}

									if (ui.totalDistDisplayed != totalDist || !ui.active ||
										forceRedrawDueToTutorialHighlight)
									{
										ui.totalDistDisplayed = totalDist;
										display->add((new Framework::DisplayDrawCommands::TextAt())
											->text(String::printf(pilgrimHandData->stats.totalDistFormat.to_char(), totalDist).get_right(pilgrimHandData->stats.totalDistLength).to_char())
											->use_font(pilgrimHandData->stats.font.get())
											->at(offset_at(pilgrimHandData->stats.totalDistAt))
											->length(pilgrimHandData->stats.totalDistLength)
											->use_coordinates(Framework::DisplayCoordinates::Pixel)
											->use_colourise_ink(pilgrimHandData->stats.totalDistColour)
										);
										drewSomething = true;
									}
								}

								// health
								{
									healthIsActive = true;
									todo_note(TXT("get resource: health"));
									Energy health = Energy(100);
									Optional<Energy> healthBackup;
									bool atTopHealth = true;
									bool regenerating = false;
									if (auto * top = get_mind()->get_owner_as_modules_owner()->get_top_instigator())
									{
										if (auto* h = top->get_custom<CustomModules::Health>())
										{
											health = h->get_health();
											atTopHealth = h->get_health() >= h->get_max_health();
										}
										if (auto* h = top->get_custom<CustomModules::HealthRegen>())
										{
											healthBackup = h->get_health_backup();
										}
									}

									float prevExtraFor = ui.healthExtraFor;
									ui.healthExtraFor = max(0.0f, ui.healthExtraFor - _deltaTime);
									if (healthBackup.is_set() && healthBackup.get() > ui.healthBackupDisplayed)
									{
										ui.healthExtraFor = 1.0f;
									}

									if (!ui.healthActive)
									{
										ui.healthExtraFor = 0.0f;
									}

									bool forceRedraw = (prevExtraFor == 0.0f && ui.healthExtraFor != 0.0f) ||
													   (prevExtraFor != 0.0f && ui.healthExtraFor == 0.0f);

									VectorInt2 const * at = &pilgrimHandData->health.at;
									VectorInt2 const * at2 = &pilgrimHandData->health.atSecondary;
									VectorInt2 const * at3 = &pilgrimHandData->health.atTertiary;

									Framework::Font* font = pilgrimHandData->health.font.get();
									Framework::Font* fontN = pilgrimHandData->health.fontOther.get();
									String const * format = &pilgrimHandData->health.format;
									String const * formatN = &pilgrimHandData->health.formatOther;

									regenerating = health > ui.healthDisplayed;

									health = max(health, Energy(0));
									if (forceRedraw || ui.healthDisplayed != health || !ui.active || ui.healthWentDownTime < 1.5f ||
										ui.atTopHealthDisplayed != atTopHealth ||
										ui.regeneratingDisplayed != regenerating ||
										forceRedrawDueToTutorialHighlight)
									{
										if (health < ui.healthDisplayed)
										{
											ui.healthWentDownTime = 0.0f;
										}
										ui.healthDisplayed = health;
										ui.atTopHealthDisplayed = atTopHealth;
										ui.regeneratingDisplayed = regenerating;
										Energy displayValue = clamp(health, Energy(0), Energy(999));

										Optional<Colour> useColour;
										if (tutorialHighlightRequired &&
											(TutorialSystem::check_highlighted(NAME(displaysHealth)) ||
											 TutorialSystem::check_highlighted(NAME(displaysHealthMain))))
										{
											useColour = TutorialSystem::highlight_colour();
										}
										display->add((new Framework::DisplayDrawCommands::TextAt())
											->text(displayValue.format(*format).get_right(pilgrimHandData->health.length).to_char())
											->use_font(font)
											->at(offset_at(*at))
											->length(pilgrimHandData->health.length)
											->use_coordinates(Framework::DisplayCoordinates::Pixel)
											->use_colourise_ink(useColour.is_set() ? useColour : pilgrimHandData->health.colour)/*(ui.healthExtraFor > 0.0f? pilgrimHandData->health.extraColour.get() : (atTopHealth || regenerating || ui.healthWentDownTime >= 1.0f ? pilgrimHandData->health.colour : pilgrimHandData->health.notFullColour)))*/
										);
									}
									at = at2;
									at2 = at3;
									font = fontN;
									format = formatN->is_empty()? format : formatN;

									if (healthBackup.is_set())
									{
										if (forceRedraw || ui.healthBackupDisplayed != healthBackup.get() || !ui.active ||
											forceRedrawDueToTutorialHighlight)
										{
											ui.healthBackupDisplayed = healthBackup.get();
											Energy displayValue = clamp(healthBackup.get(), Energy(0), Energy(999));

											Optional<Colour> useColour;
											if (tutorialHighlightRequired &&
												(TutorialSystem::check_highlighted(NAME(displaysHealth)) ||
												 TutorialSystem::check_highlighted(NAME(displaysHealthBackup))))
											{
												useColour = TutorialSystem::highlight_colour();
											}
											display->add((new Framework::DisplayDrawCommands::TextAt())
												->text(displayValue.format(*format).get_right(pilgrimHandData->health.length).to_char())
												->use_font(font)
												->at(offset_at(*at))
												->length(pilgrimHandData->health.length)
												->use_coordinates(Framework::DisplayCoordinates::Pixel)
												->use_colourise_ink(useColour.is_set() ? useColour : pilgrimHandData->health.colour)/*((ui.healthExtraFor > 0.0f ? pilgrimHandData->health.extraColour.get() : (displayValue > Energy(0) ? pilgrimHandData->health.colour : pilgrimHandData->health.notFullColour)))*/
											);
										}
										at = at2;
										at2 = at3;
										font = fontN;
										format = formatN->is_empty() ? format : formatN;
									}

									if (atTopHealth || (healthBackup.is_set() && healthBackup.get() == Energy(0)))
									{
										ui.healthBlinking = 0.0f;
									}

									if (!ui.active || !atTopHealth || regenerating ||
										forceRedrawDueToTutorialHighlight)
									{
										display->add((new Framework::DisplayDrawCommands::TexturePart())
											->use(pilgrimHandData->health.indicatorTexturePart.get())
											->at(offset_at(pilgrimHandData->health.indicatorAt))
											->use_colourise_ink(atTopHealth ? pilgrimHandData->health.colour : (ui.healthBlinking < 0.25f ? pilgrimHandData->health.notFullColour : pilgrimHandData->health.colour))
										);
									}

									ui.healthBlinking = mod(ui.healthBlinking + _deltaTime * (regenerating ? 4.0f : 2.0f), 1.0f);
									ui.healthWentDownTime += _deltaTime;
								}

								// weapon
								if (equipmentInHand.is_set())
								{
									if (auto* gun = equipmentInHand->get_gameplay_as<ModuleEquipments::Gun>())
									{
										weaponIsActive = true;
										
										todo_note(TXT("get resource: weapon"));
										Energy weaponChamberSize = Energy(1);
										Energy weaponChamber = Energy(0);
										bool weaponChamberReady = true;
										bool weaponChamberInUse = true;
										Energy weaponMagazine = Energy(0);
										bool weaponMagazineInUse = true;
										Energy weaponStorage = Energy(0);
										bool noMore = false;

										{
											weaponChamber = gun->get_chamber();
											weaponChamberSize = gun->get_chamber_size();
											weaponChamberReady = gun->is_chamber_ready();
											todo_note(TXT("chamber is now not used as it would just display 1 or 0!"));
											weaponChamberInUse = false;
											weaponMagazine = gun->get_magazine();
											weaponMagazineInUse = gun->is_using_magazine();
											weaponStorage = gun->get_storage();
										}

										VectorInt2 const * at = &pilgrimHandData->weapon.at;
										VectorInt2 const * at2 = &pilgrimHandData->weapon.atSecondary;
										VectorInt2 const * at3 = &pilgrimHandData->weapon.atTertiary;

										Framework::Font* font = pilgrimHandData->weapon.font.get();
										Framework::Font* fontN = pilgrimHandData->weapon.fontOther.get();
										String const * format = &pilgrimHandData->weapon.format;
										String const * formatN = &pilgrimHandData->weapon.formatOther;

										// display that one in the chamber and display the magazine in a storage
										Energy addToMagazine = weaponChamber;
										Energy addToStorage = weaponChamber + weaponMagazine;

										if (!weaponChamberReady)
										{
											noMore = weaponMagazine + weaponStorage + weaponChamber < weaponChamberSize;
										}

										float prevExtraFor = ui.weaponExtraFor;
										ui.weaponExtraFor = max(0.0f, ui.weaponExtraFor - _deltaTime);
										if (weaponStorage + addToStorage > ui.weaponStorageDisplayed)
										{
											ui.weaponExtraFor = 1.0f;
										}

										if (! ui.weaponActive)
										{
											ui.weaponExtraFor = 0.0f;
										}

										bool forceRedraw = (prevExtraFor == 0.0f && ui.weaponExtraFor != 0.0f) ||
														   (prevExtraFor != 0.0f && ui.weaponExtraFor == 0.0f);

										auto* mainEquipment = mp->get_main_equipment(hand);
										PilgrimHandResourceData const & weaponColours = equipmentInHand == mainEquipment? pilgrimHandData->weapon : pilgrimHandData->weaponInterim;
										
										// total number is most important
										{
											Energy weaponStorageToDisplay = max(weaponStorage + addToStorage, Energy(0));
											if (forceRedraw ||
												!ui.active ||
												ui.weaponStorageDisplayed != weaponStorageToDisplay ||
												ui.weaponStorageChanging ||
												ui.weaponMagazineInUseDisplayed != weaponMagazineInUse ||
												ui.weaponChamberInUseDisplayed != weaponChamberInUse ||
												ui.weaponChamberReadyDisplayed != weaponChamberReady ||
												forceRedrawDueToTutorialHighlight)
											{
												ui.weaponStorageChanging = ui.weaponStorageDisplayed != weaponStorageToDisplay;
												ui.weaponStorageDisplayed = weaponStorageToDisplay;
												ui.weaponMagazineInUseDisplayed = weaponMagazineInUse;
												ui.weaponChamberInUseDisplayed = weaponChamberInUse;
												ui.weaponChamberReadyDisplayed = weaponChamberReady;
												Energy displayValue = clamp(weaponStorageToDisplay / weaponChamberSize, Energy(0), Energy(999));

												Optional<Colour> useColour;
												if (tutorialHighlightRequired &&
													(TutorialSystem::check_highlighted(NAME(displaysWeapon)) ||
													 TutorialSystem::check_highlighted(NAME(displaysWeaponAmmo)) ||
													 TutorialSystem::check_highlighted(NAME(displaysWeaponStorage))))
												{
													useColour = TutorialSystem::highlight_colour();
												}
												display->add((new Framework::DisplayDrawCommands::TextAt())
													->text(displayValue.format(*format).get_right(pilgrimHandData->weapon.length).to_char())
													->use_font(font)
													->at(offset_at(*at))
													->length(pilgrimHandData->weapon.length)
													->use_coordinates(Framework::DisplayCoordinates::Pixel)
													//->use_colourise_ink(useColour.is_set()? useColour : (ui.weaponExtraFor > 0.0f ? weaponColours.extraColour.get() : ((!weaponMagazineInUse && !weaponChamberInUse && !weaponChamberReady) || ((weaponMagazineInUse || weaponChamberInUse) && ui.weaponStorageChanging) || displayValue == 0.0f ? weaponColours.notFullColour : weaponColours.colour)))
													->use_colourise_ink(useColour.is_set() ? useColour : weaponColours.colour)/*(ui.weaponExtraFor > 0.0f ? weaponColours.extraColour.get() : ((!weaponChamberInUse && !weaponChamberReady) || displayValue == Energy(0) ? weaponColours.notFullColour : weaponColours.colour)))*/
												);
											}
											at = at2;
											at2 = at3;
											font = fontN;
											format = formatN->is_empty() ? format : formatN;
										}

										if (weaponMagazineInUse)
										{
											Energy weaponMagazineToDisplay = max(weaponMagazine + addToMagazine, Energy(0));
											if (forceRedraw ||
												!ui.active ||
												ui.weaponMagazineDisplayed != weaponMagazineToDisplay ||
												ui.weaponMagazineChanging ||
												forceRedrawDueToTutorialHighlight)
											{
												ui.weaponMagazineChanging = ui.weaponMagazineDisplayed != weaponMagazineToDisplay;
												ui.weaponMagazineDisplayed = weaponMagazineToDisplay;
												Energy displayValue = clamp(weaponMagazineToDisplay / weaponChamberSize, Energy(0), Energy(999));

												Optional<Colour> useColour;
												if (tutorialHighlightRequired &&
													(TutorialSystem::check_highlighted(NAME(displaysWeapon)) ||
													 TutorialSystem::check_highlighted(NAME(displaysWeaponAmmo)) ||
													 TutorialSystem::check_highlighted(NAME(displaysWeaponMagazine))))
												{
													useColour = TutorialSystem::highlight_colour();
												}
												display->add((new Framework::DisplayDrawCommands::TextAt())
													->text(displayValue.format(*format).get_right(pilgrimHandData->weapon.length).to_char())
													->use_font(font)
													->at(offset_at(*at))
													->length(pilgrimHandData->weapon.length)
													->use_coordinates(Framework::DisplayCoordinates::Pixel)
													//->use_colourise_ink(ui.weaponExtraFor > 0.0f ? weaponColours.extraColour.get() : ((!weaponChamberInUse && !weaponChamberReady) || (weaponChamberInUse && ui.weaponMagazineChanging) || displayValue == 0.0f ? weaponColours.notFullColour : weaponColours.colour))
													->use_colourise_ink(useColour.is_set() ? useColour : weaponColours.colour)/*((ui.weaponExtraFor > 0.0f ? weaponColours.extraColour.get() : (ui.weaponMagazineChanging || displayValue == Energy(0) ? weaponColours.notFullColour : weaponColours.colour)))*/
												);
											}
											at = at2;
											at2 = at3;
											font = fontN;
											format = formatN->is_empty() ? format : formatN;
										}

										// chamber is less important (unless there is no magazine)
										if (weaponChamberInUse)
										{
											weaponChamber = max(weaponChamber, Energy(0));
											if (forceRedraw ||
												!ui.active ||
												ui.weaponChamberDisplayed != weaponChamber ||
												forceRedrawDueToTutorialHighlight)
											{
												ui.weaponChamberDisplayed = weaponChamber;
												Energy displayValue = clamp(weaponChamber, Energy(0), Energy(999));

												Optional<Colour> useColour;
												if (tutorialHighlightRequired &&
													(TutorialSystem::check_highlighted(NAME(displaysWeapon)) ||
													 TutorialSystem::check_highlighted(NAME(displaysWeaponChamber))))
												{
													useColour = TutorialSystem::highlight_colour();
												}
												display->add((new Framework::DisplayDrawCommands::TextAt())
													->text(displayValue.format(*format).get_right(pilgrimHandData->weapon.length).to_char())
													->use_font(font)
													->at(offset_at(*at))
													->length(pilgrimHandData->weapon.length)
													->use_coordinates(Framework::DisplayCoordinates::Pixel)
													->use_colourise_ink(useColour.is_set() ? useColour : weaponColours.colour)/*((ui.weaponExtraFor > 0.0f ? weaponColours.extraColour.get() : (!weaponChamberReady ? weaponColours.notFullColour : weaponColours.colour)))*/
												);
											}
											at = at2;
											at2 = at3;
											font = fontN;
											format = formatN->is_empty() ? format : formatN;
										}

										//if (!ui.active || ui.weaponStorageChanging || ui.weaponMagazineChanging || ! weaponChamberReady)
										{
											display->add((new Framework::DisplayDrawCommands::TexturePart())
												->use(pilgrimHandData->weapon.indicatorTexturePart.get())
												->at(offset_at(pilgrimHandData->weapon.indicatorAt))
												->use_colourise_ink(noMore || ((ui.weaponStorageChanging || ui.weaponMagazineChanging || !weaponChamberReady) && ui.weaponBlinking < 0.33f) ? weaponColours.notFullColour : weaponColours.colour)
											);
										}
										ui.weaponBlinking = mod(ui.weaponBlinking + _deltaTime * 4.0f, 1.0f);
									}
									else if (auto* eq = equipmentInHand->get_gameplay_as<ModuleEquipment>())
									{
										todo_note(TXT("get resource: other equipment"));
										Optional<Energy> primaryState = eq->get_primary_state_value();
										Optional<Energy> secondaryState = eq->get_secondary_state_value();

										bool forceRedraw = forceRedrawDueToTutorialHighlight;
										if (primaryState.is_set())
										{
											float prevPrimaryExtraFor = ui.otherEquipmentPrimaryExtraFor;
											ui.otherEquipmentPrimaryExtraFor = max(0.0f, ui.otherEquipmentPrimaryExtraFor - _deltaTime);
											if (primaryState.get() > ui.otherEquipmentPrimaryDisplayed)
											{
												ui.otherEquipmentPrimaryExtraFor = 1.0f;
											}
											if (!ui.otherEquipmentActive)
											{
												ui.otherEquipmentPrimaryExtraFor = 0.0f;
											}
											forceRedraw |= (prevPrimaryExtraFor == 0.0f && ui.otherEquipmentPrimaryExtraFor != 0.0f) ||
														   (prevPrimaryExtraFor != 0.0f && ui.otherEquipmentPrimaryExtraFor == 0.0f) ||
															ui.otherEquipmentPrimaryWentDownTime < 1.5f;
										}
										if (secondaryState.is_set())
										{
											float prevSecondaryExtraFor = ui.otherEquipmentSecondaryExtraFor;
											ui.otherEquipmentSecondaryExtraFor = max(0.0f, ui.otherEquipmentSecondaryExtraFor - _deltaTime);
											if (secondaryState.get() > ui.otherEquipmentSecondaryDisplayed)
											{
												ui.otherEquipmentSecondaryExtraFor = 1.0f;
											}
											if (!ui.otherEquipmentActive)
											{
												ui.otherEquipmentSecondaryExtraFor = 0.0f;
											}
											forceRedraw |= (prevSecondaryExtraFor == 0.0f && ui.otherEquipmentSecondaryExtraFor != 0.0f) ||
														   (prevSecondaryExtraFor != 0.0f && ui.otherEquipmentSecondaryExtraFor == 0.0f) ||
															ui.otherEquipmentSecondaryWentDownTime < 1.5f;
										}

										VectorInt2 const * at = &pilgrimHandData->otherEquipment.at;
										VectorInt2 const * at2 = &pilgrimHandData->otherEquipment.atSecondary;
										VectorInt2 const * at3 = &pilgrimHandData->otherEquipment.atTertiary;

										Framework::Font* font = pilgrimHandData->otherEquipment.font.get();
										Framework::Font* fontN = pilgrimHandData->otherEquipment.fontOther.get();
										String const * format = &pilgrimHandData->otherEquipment.format;
										String const * formatN = &pilgrimHandData->otherEquipment.formatOther;

										if (primaryState.is_set())
										{
											if (forceRedraw || ui.otherEquipmentPrimaryDisplayed != primaryState.get() || !ui.active ||
												forceRedrawDueToTutorialHighlight)
											{
												if (primaryState.get() < ui.otherEquipmentPrimaryDisplayed)
												{
													ui.otherEquipmentPrimaryWentDownTime = 0.0f;
												}
												ui.otherEquipmentPrimaryDisplayed = primaryState.get();
												Energy displayValue = clamp(primaryState.get(), Energy(0), Energy(999));

												display->add((new Framework::DisplayDrawCommands::TextAt())
													->text(displayValue.format(*format).get_right(pilgrimHandData->otherEquipment.length).to_char())
													->use_font(font)
													->at(offset_at(*at))
													->length(pilgrimHandData->otherEquipment.length)
													->use_coordinates(Framework::DisplayCoordinates::Pixel)
													->use_colourise_ink(ui.otherEquipmentPrimaryExtraFor > 0.0f ? pilgrimHandData->otherEquipment.extraColour.get() : (ui.otherEquipmentPrimaryWentDownTime >= 1.0f ? pilgrimHandData->otherEquipment.colour : pilgrimHandData->otherEquipment.notFullColour))
												);
											}
											at = at2;
											at2 = at3;
											font = fontN;
											format = formatN->is_empty() ? format : formatN;
										}

										if (secondaryState.is_set())
										{
											if (forceRedraw || ui.otherEquipmentSecondaryDisplayed != secondaryState.get() || !ui.active ||
												forceRedrawDueToTutorialHighlight)
											{
												if (secondaryState.get() < ui.otherEquipmentSecondaryDisplayed)
												{
													ui.otherEquipmentSecondaryWentDownTime = 0.0f;
												}
												ui.otherEquipmentSecondaryDisplayed = secondaryState.get();
												Energy displayValue = clamp(secondaryState.get(), Energy(0), Energy(999));

												display->add((new Framework::DisplayDrawCommands::TextAt())
													->text(displayValue.format(*format).get_right(pilgrimHandData->otherEquipment.length).to_char())
													->use_font(font)
													->at(offset_at(*at))
													->length(pilgrimHandData->otherEquipment.length)
													->use_coordinates(Framework::DisplayCoordinates::Pixel)
													->use_colourise_ink(ui.otherEquipmentSecondaryExtraFor > 0.0f ? pilgrimHandData->otherEquipment.extraColour.get() : (ui.otherEquipmentSecondaryWentDownTime >= 1.0f ? pilgrimHandData->otherEquipment.colour : pilgrimHandData->otherEquipment.notFullColour))
												);
											}
											at = at2;
											at2 = at3;
											font = fontN;
											format = formatN->is_empty() ? format : formatN;
										}

										if ((!ui.active && pilgrimHandData->otherEquipment.indicatorTexturePart.get()) ||
											forceRedrawDueToTutorialHighlight)
										{
											display->add((new Framework::DisplayDrawCommands::TexturePart())
												->use(pilgrimHandData->otherEquipment.indicatorTexturePart.get())
												->at(offset_at(pilgrimHandData->otherEquipment.indicatorAt))
												->use_colourise_ink(pilgrimHandData->otherEquipment.colour)
											);
										}
										ui.otherEquipmentPrimaryWentDownTime += _deltaTime;
										ui.otherEquipmentSecondaryWentDownTime += _deltaTime;
									}
								}
							}
							if (ui.mode == UI::PrayKillCompute)
							{
								if (ui.prevMode != ui.mode)
								{
									ui.prayKillComputeBlink = 0.0f;
								}
								else
								{
									ui.prayKillComputeBlink = mod(ui.prayKillComputeBlink + _deltaTime * 2.0f, 1.0f);
								}
								if (ui.pray != PilgrimHandWord::DontShow ||
									ui.kill != PilgrimHandWord::DontShow ||
									ui.compute != PilgrimHandWord::DontShow)
								{
									Framework::DisplayUtils::clear_all(display, Name::invalid());
								}

								if (ui.pray != PilgrimHandWord::DontShow)
								{
									Framework::DisplayDrawCommands::TexturePart* part = new Framework::DisplayDrawCommands::TexturePart();
									part->use(pilgrimHandData->prayKillCompute.prayTexturePart.get())
										->at(VectorInt2(0, 0))
										->use_colourise_ink(ui.pray == PilgrimHandWord::Highlight || (ui.pray == PilgrimHandWord::Blink && ui.prayKillComputeBlink < 0.33f) ? pilgrimHandData->prayKillCompute.highlightColour : pilgrimHandData->prayKillCompute.colour)
										;
									display->add(part);
								}
								if (ui.kill != PilgrimHandWord::DontShow)
								{
									Framework::DisplayDrawCommands::TexturePart* part = new Framework::DisplayDrawCommands::TexturePart();
									part->use(pilgrimHandData->prayKillCompute.killTexturePart.get())
										->at(VectorInt2(0, 0))
										->use_colourise_ink(ui.kill == PilgrimHandWord::Highlight || (ui.kill == PilgrimHandWord::Blink && ui.prayKillComputeBlink < 0.33f) ? pilgrimHandData->prayKillCompute.highlightColour : pilgrimHandData->prayKillCompute.colour)
										;
									display->add(part);
								}
								if (ui.compute != PilgrimHandWord::DontShow)
								{
									Framework::DisplayDrawCommands::TexturePart* part = new Framework::DisplayDrawCommands::TexturePart();
									part->use(pilgrimHandData->prayKillCompute.computeTexturePart.get())
										->at(VectorInt2(0, 0))
										->use_colourise_ink(ui.compute == PilgrimHandWord::Highlight || (ui.compute == PilgrimHandWord::Blink && ui.prayKillComputeBlink < 0.33f) ? pilgrimHandData->prayKillCompute.highlightColour : pilgrimHandData->prayKillCompute.colour)
										;
									display->add(part);
								}
								ui.prayKillComputeTimeLeft -= _deltaTime;
								if (ui.prayKillComputeTimeLeft <= 0.0f)
								{
									nextMode = UI::Normal;
								}
							}
							if (equipmentInHand.is_set())
							{
								if (auto* eq = equipmentInHand->get_gameplay_as<ModuleEquipment>())
								{
									if (ui.mode == UI::Equipment)
									{
										eq->display_extra(display, ui.prevMode != ui.mode);
									}
									// always allow to rotate, no matter of mode
									if (eq->is_display_rotated())
									{
										rotateDisplay = hand == Hand::Right ? 1 : -1;
									}
								}
							}

							ui.prevMode = ui.mode;
							ui.mode = nextMode;

							ui.healthActive = healthIsActive;
							ui.weaponActive = weaponIsActive;
							ui.otherEquipmentActive = otherEquipmentIsActive;
						}
					}
				}
			}
			display->set_rotate_display(rotateDisplay);
		}

		ui.active = true;
	}

	if (playAnim && animSequence)
	{
		bool redraw = playAnimDrawFirst;
		playAnimDrawFirst = false;

		animTime += _deltaTime;
		int prevIdx = animIdx;
		float useInterval = animInterval.get(animSequence->get_interval());
		while (animTime >= useInterval)
		{
			animTime -= useInterval;
			animIdx = (animIdx + 1) % animSequence->length();
			redraw = true;
		}

		if (redraw)
		{
			if (auto* tp = animSequence->get(prevIdx))
			{
				display->add((new Framework::DisplayDrawCommands::ClearRegion())
					->rect(animAt - tp->get_rel_anchor().to_vector_int2_cells(), tp->get_size().to_vector_int2_cells())
					->use_coordinates(Framework::DisplayCoordinates::Pixel));
			}
			if (auto* tp = animSequence->get(animIdx))
			{
				display->add((new Framework::DisplayDrawCommands::TexturePart())
					->at(animAt)
					->use(tp)
					->use_colourise_ink(animColourise)
					);
			}

		}
	}

	if (drewSomething)
	{
		Framework::DisplayUtils::border(display);
	}
}

void PilgrimHand::pray_kill_compute(float _timeLeft)
{
	ui.pray = PilgrimHandWord::Normal;
	ui.kill = PilgrimHandWord::Normal;
	ui.compute = PilgrimHandWord::Normal;
	ui.prayKillComputeTimeLeft = _timeLeft;
	ui.mode = UI::PrayKillCompute;
}

void PilgrimHand::pray(PilgrimHandWord::Type _how)
{
	ui.pray = _how;
}

void PilgrimHand::kill(PilgrimHandWord::Type _how)
{
	ui.kill = _how;
}

void PilgrimHand::compute(PilgrimHandWord::Type _how)
{
	ui.compute = _how;
}

VectorInt2 PilgrimHand::offset_at(VectorInt2 const & _at)
{
	VectorInt2 at = _at;

	return at;
}

void PilgrimHand::update_emissive()
{
}

void PilgrimHand::on_game_settings_channged(GameSettings& _gameSettings)
{
	ui.active = false; // will force redraw
}

//

bool PilgrimHandStatsData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	result &= font.load_from_xml(_node, TXT("font"), _lc);

	sectionDistAt.load_from_xml_child_or_attr(_node, TXT("sectionDistAt"));
	sectionDistLength = _node->get_int_attribute(TXT("sectionDistLength"), sectionDistLength);
	sectionDistFormat = _node->get_string_attribute(TXT("sectionDistFormat"), sectionDistFormat);
	sectionDistColourActive.load_from_xml(_node, TXT("sectionDistColourActive"));
	sectionDistColourInactive.load_from_xml(_node, TXT("sectionDistColourInactive"));

	totalDistAt.load_from_xml_child_or_attr(_node, TXT("totalDistAt"));
	totalDistLength = _node->get_int_attribute(TXT("totalDistLength"), totalDistLength);
	totalDistFormat = _node->get_string_attribute(TXT("totalDistFormat"), totalDistFormat);
	totalDistColour.load_from_xml(_node, TXT("totalDistColour"));

	return result;
}

bool PilgrimHandStatsData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= font.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);

	return result;
}

//

bool PilgrimHandResourceData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	result &= font.load_from_xml(_node, TXT("font"), _lc);
	result &= fontOther.load_from_xml(_node, TXT("fontOther"), _lc);

	at.load_from_xml_child_or_attr(_node, TXT("at"));
	atSecondary.load_from_xml_child_or_attr(_node, TXT("atSecondary"));
	atTertiary.load_from_xml_child_or_attr(_node, TXT("atTertiary"));
	length = _node->get_int_attribute(TXT("length"), length);
	format = _node->get_string_attribute(TXT("format"), format);
	formatOther = _node->get_string_attribute(TXT("formatOther"), formatOther);

	result &= indicatorTexturePart.load_from_xml(_node, TXT("indicatorTexturePart"), _lc);
	
	indicatorAt.load_from_xml_child_or_attr(_node, TXT("indicatorAt"));
	colour.load_from_xml(_node, TXT("colour"));
	notFullColour.load_from_xml(_node, TXT("notFullColour"));
	extraColour.load_from_xml(_node, TXT("extraColour"));

	return result;
}

bool PilgrimHandResourceData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= font.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);
	result &= fontOther.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);

	result &= indicatorTexturePart.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);

	return result;
}

//

bool PilgrimHandPrayKillComputeData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	result &= prayTexturePart.load_from_xml(_node, TXT("prayTexturePart"), _lc);
	result &= killTexturePart.load_from_xml(_node, TXT("killTexturePart"), _lc);
	result &= computeTexturePart.load_from_xml(_node, TXT("computeTexturePart"), _lc);

	colour.load_from_xml(_node, TXT("colour"));
	highlightColour.load_from_xml(_node, TXT("highlightColour"));

	return result;
}

bool PilgrimHandPrayKillComputeData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= prayTexturePart.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);
	result &= killTexturePart.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);
	result &= computeTexturePart.prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);

	return result;
}

//

REGISTER_FOR_FAST_CAST(PilgrimHandData);

bool PilgrimHandData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("input")))
	{
		result &= inputDefinition.load_from_xml(node);
	}

	for_every(node, _node->children_named(TXT("stats")))
	{
		result &= stats.load_from_xml(node, _lc);
	}

	for_every(node, _node->children_named(TXT("health")))
	{
		result &= health.load_from_xml(node, _lc);
	}

	for_every(node, _node->children_named(TXT("weapon")))
	{
		result &= weapon.load_from_xml(node, _lc);
	}

	for_every(node, _node->children_named(TXT("weaponInterim")))
	{
		result &= weaponInterim.load_from_xml(node, _lc);
	}

	otherEquipment = weapon;
	for_every(node, _node->children_named(TXT("otherEquipment")))
	{
		result &= otherEquipment.load_from_xml(node, _lc);
	}

	for_every(node, _node->children_named(TXT("prayKillCompute")))
	{
		result &= prayKillCompute.load_from_xml(node, _lc);
	}

	for_every(node, _node->children_named(TXT("paralysed")))
	{
		result &= paralysedPulseInterval.load_from_xml(node, TXT("pulseInterval"));
		for_every(n, node->children_named(TXT("vrPulse")))
		{
			VR::Pulse pulse;
			if (pulse.load_from_xml(n))
			{
				paralysedPulses.push_back(pulse);
			}
			else
			{
				error_loading_xml(n, TXT("pulse loading error"));
				result = false;
			}
		}
	}

	return result;
}

bool PilgrimHandData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= stats.prepare_for_game(_library, _pfgContext);
	result &= health.prepare_for_game(_library, _pfgContext);
	result &= weapon.prepare_for_game(_library, _pfgContext);
	result &= weaponInterim.prepare_for_game(_library, _pfgContext);
	result &= otherEquipment.prepare_for_game(_library, _pfgContext);
	result &= prayKillCompute.prepare_for_game(_library, _pfgContext);

	return result;
}
