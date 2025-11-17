#include "aiLogic_elevatorCart.h"

#include "aiLogic_elevatorCartPoint.h"

#include "..\..\utils.h"

#include "..\..\game\game.h"
#include "..\..\game\gameDirector.h"
#include "..\..\game\gameSettings.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\nav\mne_cart.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\moduleMovementPath.h"
#include "..\..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\object\scenery.h"
#include "..\..\..\framework\world\roomRegionVariables.inl"
#include "..\..\..\framework\world\world.h"

#include "..\..\..\core\debug\debugRenderer.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef BUILD_PREVIEW
	#ifdef AN_ALLOW_EXTENSIVE_LOGS
		#define LOG_PLAYER_INFO_ON_BUTTON_PRESS
	#endif
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

DEFINE_STATIC_NAME(moveOnlyWithPlayer); // will move only if player is onboard
DEFINE_STATIC_NAME(directSpeedControl);
DEFINE_STATIC_NAME(closeRoomDoorWhileMoving);
DEFINE_STATIC_NAME(openRoomDoorWhileStoppedAndBasesPlayer);
DEFINE_STATIC_NAME(speedOccupied);
DEFINE_STATIC_NAME(speedEmpty);
DEFINE_STATIC_NAME(acceleration);
DEFINE_STATIC_NAME(doorOpenWhenEmpty);
DEFINE_STATIC_NAME(doorOpenEarlyWhenStopping);
DEFINE_STATIC_NAME(doorOpenOnlyWhenStationary);
DEFINE_STATIC_NAME(doorOpenTime);
DEFINE_STATIC_NAME(waitTimeWithoutPlayerAfterPlayerLeftBaseCart);
DEFINE_STATIC_NAME(waitTimeAfterPlayerLeftBaseCart);
DEFINE_STATIC_NAME(waitTimeAtTarget);
DEFINE_STATIC_NAME(waitTimeAtTargetWithPlayer);

DEFINE_STATIC_NAME(cartSpeedOccupied);
DEFINE_STATIC_NAME(cartSpeedEmpty);
DEFINE_STATIC_NAME(cartAcceleration);
DEFINE_STATIC_NAME(cartDoorOpenWhenEmpty);
DEFINE_STATIC_NAME(cartDoorOpenEarlyWhenStopping);
DEFINE_STATIC_NAME(cartDoorOpenOnlyWhenStationary);
DEFINE_STATIC_NAME(cartDoorOpenTime);
DEFINE_STATIC_NAME(cartWaitTimeWithoutPlayerAfterPlayerLeftBaseCart);
DEFINE_STATIC_NAME(cartWaitTimeAfterPlayerLeftBaseCart);
DEFINE_STATIC_NAME(cartWaitTimeAtTarget);
DEFINE_STATIC_NAME(cartWaitTimeAtTargetWithPlayer);

DEFINE_STATIC_NAME(elevatorCartControlMovementOrder);
DEFINE_STATIC_NAME(elevatorCartControlMovementOrderA);
DEFINE_STATIC_NAME(elevatorCartControlMovementOrderB);
DEFINE_STATIC_NAME(elevatorCartControlMovementOrderStop);
DEFINE_STATIC_NAME(cartPath);

/*
 *	all of these params may be provided by code or as an asset(room / mesh generator etc)
 *	the system uses IMOs:
 *		1x cart
 *		2x cart stops (might be game:none scenery)
 *			this can be platform or some abstract object
 *		2x cart points (might be platforms:platform cart point empty)
 *			this may be attached to cart stop or not, provides door at the exit
 *			also is used for navigation as entry point for whatever is at the cart stop
 *	cart point below is the placement relative to the stop, so we can build a path for module movement
 */
DEFINE_STATIC_NAME(elevatorStopA); // SafePtr<Framework::IModulesOwner> - platform where it stops at A
DEFINE_STATIC_NAME(elevatorStopB); // SafePtr<Framework::IModulesOwner> - platform where it stops at B
DEFINE_STATIC_NAME(elevatorStopATagged); // TagCondition if not provided directly (via code as elevatorStopA), should find a tagged object in the room
DEFINE_STATIC_NAME(elevatorStopBTagged); // TagCondition if not provided directly (via code as elevatorStopB), should find a tagged object in the room
DEFINE_STATIC_NAME(elevatorStopACartPoint); // Transform - cart point relative to stop A
DEFINE_STATIC_NAME(elevatorStopBCartPoint); // Transform - cart point relative to stop B
DEFINE_STATIC_NAME(elevatorStopACartPointFromPOI); // as above but get from object's POI - POI is placed where stop should be and we reverse it
DEFINE_STATIC_NAME(elevatorStopBCartPointFromPOI); // as above but get from object's POI - POI is placed where stop should be and we reverse it
DEFINE_STATIC_NAME(elevatorCartPointATagged); // if provided, will prefer to find cart point this way
DEFINE_STATIC_NAME(elevatorCartPointBTagged); // if provided, will prefer to find cart point this way
DEFINE_STATIC_NAME(doorOpen);
DEFINE_STATIC_NAME(doorMovementCollisionOpen);
DEFINE_STATIC_NAME(open);
DEFINE_STATIC_NAME(close);
DEFINE_STATIC_NAME(atCartPoint);
DEFINE_STATIC_NAME(noLongerAtCartPoint);
DEFINE_STATIC_NAME(noActiveButtonsForElevatorsWithoutPlayer);

DEFINE_STATIC_NAME(movement);
DEFINE_STATIC_NAME_STR(changeDir, TXT("change dir"));

DEFINE_STATIC_NAME(interactiveDeviceId);
DEFINE_STATIC_NAME(relativeMovementDir);
DEFINE_STATIC_NAME(active);
DEFINE_STATIC_NAME(ready);
DEFINE_STATIC_NAME(pulse);
DEFINE_STATIC_NAME(busy);

DEFINE_STATIC_NAME(linear);
DEFINE_STATIC_NAME(curveMoveXZ);

DEFINE_STATIC_NAME(elevatorIgnore);

DEFINE_STATIC_NAME(beAware);
DEFINE_STATIC_NAME(beLazy);

//

REGISTER_FOR_FAST_CAST(ElevatorCart);

ElevatorCart::ElevatorCart(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
{
	doorOpenACVarID = NAME(doorOpen);
	doorMovementCollisionOpenACVarID = NAME(doorMovementCollisionOpen);

	_mind->access_execution().register_message_handler(NAME(elevatorCartControlMovementOrder), this);
	_mind->access_execution().register_message_handler(NAME(elevatorCartControlMovementOrderA), this);
	_mind->access_execution().register_message_handler(NAME(elevatorCartControlMovementOrderB), this);
	_mind->access_execution().register_message_handler(NAME(elevatorCartControlMovementOrderStop), this);
}

ElevatorCart::~ElevatorCart()
{
	get_mind()->access_execution().unregister_message_handler(NAME(elevatorCartControlMovementOrder), this);
	get_mind()->access_execution().unregister_message_handler(NAME(elevatorCartControlMovementOrderA), this);
	get_mind()->access_execution().unregister_message_handler(NAME(elevatorCartControlMovementOrderB), this);
	get_mind()->access_execution().unregister_message_handler(NAME(elevatorCartControlMovementOrderStop), this);
}

bool does_object_base_player(Framework::IModulesOwner* imo)
{
	if (!imo)
	{
		return false;
	}
	for_every_ptr(based, imo->get_presence()->get_based())
	{
		if (Framework::GameUtils::is_player(based))
		{
			return true;
		}
	}
	return false;
}

bool does_object_base_player_facing(Framework::IModulesOwner* imo, Framework::IModulesOwner* facesIMO)
{
	if (!imo ||!facesIMO)
	{
		return false;
	}
	Transform facesIMOPlacement = facesIMO->get_presence()->get_placement();
	for_every_ptr(based, imo->get_presence()->get_based())
	{
		if (Framework::GameUtils::is_player(based))
		{
			Transform pPlacement = based->get_presence()->get_placement();
			Vector3 p2f = facesIMOPlacement.get_translation() - pPlacement.get_translation();
			if (p2f.length() < magic_number 1.0f &&
				Vector3::dot(p2f.drop_using_normalised(pPlacement.get_axis(Axis::Up)).normal(), pPlacement.get_axis(Axis::Forward)) > magic_number 0.85f)
			{
				// is close and is looking at the cart, allow player to control it
				return true;
			}
		}
	}
	return false;
}

bool does_object_base(Framework::IModulesOwner* base, Framework::IModulesOwner* who)
{
	if (!base || !who)
	{
		return false;
	}
	for_every_ptr(based, base->get_presence()->get_based())
	{
		if (based == who)
		{
			return true;
		}
	}
	return false;
}

bool has_player_close(Framework::DoorInRoom* _door)
{
	if (_door)
	{
		for_every_ptr(object, _door->get_in_room()->get_objects())
		{
			if (Framework::GameUtils::is_player(object))
			{
				if ((_door->get_placement().get_translation() - object->get_presence()->get_placement().get_translation()).length() < 1.0f)
				{
					return true;
				}
			}
		}
	}
	return false;
}

void ElevatorCart::handle_message(::Framework::AI::Message const & _message)
{
	if (_message.get_name() == NAME(elevatorCartControlMovementOrder))
	{
		order_to_move();
	}
	if (_message.get_name() == NAME(elevatorCartControlMovementOrderA))
	{
		order_to_move(elevatorStopA.get());
	}
	if (_message.get_name() == NAME(elevatorCartControlMovementOrderB))
	{
		order_to_move(elevatorStopB.get());
	}
	if (_message.get_name() == NAME(elevatorCartControlMovementOrderStop))
	{
		order_to_stop();
	}
}

Framework::IModulesOwner* ElevatorCart::where_would_order_to_move() const
{
	if (stayFor <= 0.0f)
	{
		// change during movement, otherwise target stop is already set
		return elevatorStopA == targetStop  ? elevatorStopB.get() : elevatorStopA.get();
	}
	else
	{
		return targetStop;
	}
}

void ElevatorCart::order_to_move(Framework::IModulesOwner* _target)
{
	if (_target)
	{
		targetStop = _target;
	}
	else if (stayFor <= 0.0f)
	{
		// change during movement, otherwise target stop is already set
		targetStop = elevatorStopA == targetStop ? elevatorStopB.get() : elevatorStopA.get();
	}
	shouldMove = true;
	stayFor = 0.0f;
	waitingForSomeone = false;
	waitingForAI = false;
	aiOnBoard = false;
	forcedMove = true;
	forcedStop = false;
#ifdef DEBUG_ELEVATOR_MOVING_ALONE
	if (moveOnlyWithPlayer)
	{
		output(TXT("[ELEVATOR] ordered to move"));
	}
#endif
}

void ElevatorCart::order_to_stop()
{
	shouldMove = false;
	forcedMove = false;
	forcedStop = true;
}

enum ElevatorState
{
	StoppedNoPlayer,
	StoppedWithPlayer,
	Moves
};

enum ButtonState
{
	Disabled,
	PressToMove,
	Busy,
	Pressed,
};

void ElevatorCart::log_logic_info(LogInfoContext& _log) const
{
	base::log_logic_info(_log);

#ifdef AN_DEVELOPMENT_OR_PROFILER
	_log.log(TXT("wtAdvancedAt             %6.2f"), wtAdvancedAt);
	_log.log(TXT(""));
#endif
	_log.log(TXT("forcedMove        %S"), forcedMove ? TXT("YES") : TXT("no"));
	_log.log(TXT("forcedStop        %S"), forcedStop ? TXT("YES") : TXT("no"));
	_log.log(TXT("shouldMove        %S"), shouldMove ? TXT("YES") : TXT("no"));
	_log.log(TXT("isMoving          %S"), isMoving ? TXT("YES") : TXT("no"));
	_log.log(TXT("waitingForSomeone %S"), waitingForSomeone ? TXT("YES") : TXT("no"));
	_log.log(TXT("waitingForAI      %S"), waitingForAI ? TXT("YES") : TXT("no"));
	_log.log(TXT("aiOnBoard         %S"), aiOnBoard ? TXT("YES") : TXT("no"));
	_log.log(TXT("movedWithPlayer   %S"), movedWithPlayer ? TXT("YES") : TXT("no"));
	_log.log(TXT("targetStop        %S"), elevatorStopA == targetStop ? TXT("A") : TXT("B"));
	_log.log(TXT("roomDoor (c)      %S"), closeRoomDoorWhileMoving ? TXT("close while moving") : TXT("--"));
	_log.log(TXT("roomDoor (o)      %S"), openRoomDoorWhileStoppedAndBasesPlayer ? TXT("open while stopped and bases player") : TXT("--"));
	_log.log(TXT("directControl     %S"), directSpeedControl ? TXT("SPEED") : TXT("no"));
	_log.log(TXT(""));
	_log.log(TXT("stayFor                  %6.2f"), stayFor);
	_log.log(TXT("forceRemainStationary    %6.2f"), forceRemainStationary);
	_log.log(TXT("timeStationary           %6.2f"), timeStationary);
	_log.log(TXT("timeToCheckPlayer        %6.2f"), timeToCheckPlayer);
	_log.log(TXT(""));
	_log.log(TXT("consecutiveChangesTime   %6.2f"), consecutiveChangesTime);
	_log.log(TXT("consecutiveChangesCount  %6i"), consecutiveChangesCount);
	_log.log(TXT(""));
	if (directSpeedRequested.is_set())
	{
		_log.log(TXT("directSpeedReq           %6.2f"), directSpeedRequested.get(0.0f));
	}
	else
	{
		_log.log(TXT("directSpeedReq           none"));
	}
}

void ElevatorCart::advance(float _deltaTime)
{
	if (findStopsAndCartPoints)
	{
		// do it here as we're placed in the room
		findStopsAndCartPoints = false;
		find_stops_and_cart_points(false);
	}

	base::advance(_deltaTime);

#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (auto* imo = get_imo())
	{
		if (auto* w = imo->get_in_world())
		{
			wtAdvancedAt = w->get_world_time();
		}
	}
#endif

	if (!initialisedFromRoom)
	{
		if (auto* imo = get_imo())
		{
			if (auto* p = imo->get_presence())
			{
				if (auto* r = p->get_in_room())
				{
					initialisedFromRoom = true;

					speedOccupied = r->get_value<float>(NAME(cartSpeedOccupied), speedOccupied, false);
					speedEmpty = r->get_value<float>(NAME(cartSpeedEmpty), speedEmpty, false);
					acceleration = r->get_value<float>(NAME(cartAcceleration), acceleration, false);

					doorOpenWhenEmpty = r->get_value<bool>(NAME(cartDoorOpenWhenEmpty), doorOpenWhenEmpty, false);
					doorOpenEarlyWhenStopping = r->get_value<bool>(NAME(cartDoorOpenEarlyWhenStopping), doorOpenEarlyWhenStopping, false);
					doorOpenOnlyWhenStationary = r->get_value<bool>(NAME(cartDoorOpenOnlyWhenStationary), doorOpenOnlyWhenStationary, false);
					doorOpenTime = r->get_value<float>(NAME(cartDoorOpenTime), doorOpenTime, false);
					{
						float val = r->get_value<float>(NAME(cartWaitTimeWithoutPlayerAfterPlayerLeftBaseCart), -1.0f);
						if (val >= 0.0f)
						{
							waitTimeWithoutPlayerAfterPlayerLeftBaseCart = Range(val); // single value
						}
					}
					waitTimeWithoutPlayerAfterPlayerLeftBaseCart = r->get_value<Range>(NAME(waitTimeWithoutPlayerAfterPlayerLeftBaseCart), waitTimeWithoutPlayerAfterPlayerLeftBaseCart);
					waitTimeAfterPlayerLeftBaseCart = r->get_value<float>(NAME(waitTimeAfterPlayerLeftBaseCart), waitTimeAfterPlayerLeftBaseCart);
					waitTimeAtTarget = r->get_value<Range>(NAME(waitTimeAtTarget), waitTimeAtTarget);
					waitTimeAtTargetWithPlayer = r->get_value<float>(NAME(waitTimeAtTargetWithPlayer), waitTimeAtTargetWithPlayer);
				}
			}
		}
	}

	if (isValid)
	{
		if (auto* cart = get_mind()->get_owner_as_modules_owner())
		{
			debug_context(get_mind()->get_owner_as_modules_owner()->get_presence()->get_in_room());
			debug_filter(ai_elevatorCart);
			debug_subject(cart);
#ifdef AN_DEBUG_RENDERER
			Vector3 debugAt = get_mind()->get_owner_as_modules_owner()->get_presence()->get_centre_of_presence_WS();
			float debugFontSize = 1.0f;
			Vector3 debugMove = -Vector3::zAxis * debugFontSize * 0.1f;
#endif

			if (!initialised)
			{
				initialised = true;
				auto* world = cart->get_in_world();
				an_assert(world);

				if (auto* id = cart->get_variables().get_existing<int>(NAME(interactiveDeviceId)))
				{
					world->for_every_object_with_id(NAME(interactiveDeviceId), *id,
						[this, cart](Framework::Object* _object)
					{
						if (_object != cart)
						{
							Interactive i(_object);
							if (auto* relativeMovementDir = _object->get_variables().get_existing<Vector3>(NAME(relativeMovementDir)))
							{
								i.relativeMovementDir = *relativeMovementDir;
							}
							interactives.push_back(i);
						}
						return false; // keep going on
					});
				}
			}
			if (checkForDoorBeyondDelay > 0.0f)
			{
				checkForDoorBeyondDelay -= _deltaTime;
				if (checkForDoorBeyondDelay <= 0.0f)
				{
					bool doItAgain = false;
					for (int pIdx = 0; pIdx < 2; ++pIdx)
					{
						auto & elevatorStopDoor = pIdx == 0 ? elevatorStopADoor : elevatorStopBDoor;

						if (elevatorStopDoor.get())
						{
							auto* otherRoom = elevatorStopDoor->get_world_active_room_on_other_side();
							doItAgain |= otherRoom == nullptr;
							if (otherRoom &&
								otherRoom->get_all_doors().get_size() == 2)
							{
								auto* otherSideDoor = elevatorStopDoor->get_door_on_other_side();
								for_every_ptr(door, otherRoom->get_all_doors())
								{
									// invisible doors are fine here
									if (!door->is_placed())
									{
										doItAgain = true;
										break;
									}
									if (door != otherSideDoor &&
										(door->get_placement().get_translation() - otherSideDoor->get_placement().get_translation()).length() < magic_number 0.4f)
									{
										auto & elevatorStopDoorBeyond = (pIdx == 0 ? elevatorStopADoorBeyond : elevatorStopBDoorBeyond);
										elevatorStopDoorBeyond = door;
										break;
									}
								}
							}
						}
					}
					if (doItAgain)
					{
						checkForDoorBeyondDelay = 1.0f;
					}
				}
			}
			else if (!elevatorStopADoorBeyond.is_set() || !elevatorStopBDoorBeyond.is_set())
			{
				// try to restore
				checkForDoorBeyondDelay = 5.0f;
			}
			{
				Vector3 pa2pb = elevatorStopB->get_presence()->get_placement().get_translation() - elevatorStopA->get_presence()->get_placement().get_translation();

				ElevatorState elevatorState = isMoving ? ElevatorState::Moves : (does_object_base_player(cart)? ElevatorState::StoppedWithPlayer : ElevatorState::StoppedNoPlayer);
				directSpeedRequested.clear();
				for_every(interactive, interactives)
				{
					if (auto * mms = fast_cast<Framework::ModuleMovementSwitch>(interactive->imo->get_movement()))
					{
						auto* gd = GameDirector::get_active();
						if (directSpeedControl)
						{
							if (gd && gd->are_elevators_blocked())
							{
								directSpeedRequested = 0.0f;
							}
							else
							{
								directSpeedRequested = directSpeedRequested.get(0.0f) + mms->get_output();
#ifdef DEBUG_ELEVATOR_MOVING_ALONE
								if (moveOnlyWithPlayer && directSpeedRequested.get() != 0.0f)
								{
									output(TXT("[elevspeed] requesting direct speed %.3f has player onboard %S"), directSpeedRequested.get(), does_object_base_player(cart)? TXT("YES") : TXT("NO"));
								}
#endif
							}
						}
						else
						{
							if (gd && gd->are_elevators_blocked())
							{
								interactive->buttonPressed = false;
							}
							else if (mms->is_active_at(1)) // order by a button press
							{
								if (!interactive->buttonPressed) // we need to check it this way as we rarely advance
								{
#ifdef LOG_PLAYER_INFO_ON_BUTTON_PRESS
									{
										LogInfoContext log;
										log.log(TXT("CART o%p \"%S\" ordered to move"), cart, cart->ai_get_name().to_char());
										if (auto* r = cart->get_presence()->get_in_room())
										{
											log.log(TXT("  in r%p \"%S\""), r, r->get_name().to_char());
										}
										log.log(TXT("  %S"), does_object_base_player(cart)? TXT("cart bases player") : TXT("cart does not base player"));
										if (auto* g = Game::get_as<Game>())
										{
											if (auto* pa = g->access_player().get_actor())
											{
												if (auto* r = pa->get_presence()->get_in_room())
												{
													log.log(TXT("  player in r%p \"%S\""), r, r->get_name().to_char());
												}
												if (auto* b = pa->get_presence()->get_based_on())
												{
													log.log(TXT("  based on o%p \"%S\""), b, b->ai_get_name().to_char());
												}
											}
										}
										log.output_to_output();
									}
#endif

									if (! interactive->relativeMovementDir.is_zero())
									{
										Vector3 worldDir = cart->get_presence()->get_placement().vector_to_world(interactive->relativeMovementDir);
										if (Vector3::dot(pa2pb, worldDir) > 0.0f)
										{
											order_to_move(elevatorStopB.get());
										}
										else
										{
											order_to_move(elevatorStopA.get());
										}
									}
									else
									{
										order_to_move();
									}
								}
								interactive->buttonPressed = true;
							}
							else
							{
								interactive->buttonPressed = false;
							}
						}
					}
				}

				if (directSpeedControl)
				{
					if (!does_object_base_player(cart))
					{
#ifdef DEBUG_ELEVATOR_MOVING_ALONE
						if (moveOnlyWithPlayer && directSpeedRequested.get() != 0.0f)
						{
							output(TXT("[elevspeed] no player on board, drop direct speed request"));
						}
#endif
						directSpeedRequested.clear();
					}
					else
					{
						directSpeedRequested = clamp(directSpeedRequested.get(0.0f), -1.0f, 1.0f);
					}
				}
				else
				{
					for_every(interactive, interactives)
					{
						ButtonState buttonState = ButtonState::Disabled;
						if (!interactive->relativeMovementDir.is_zero())
						{
							Vector3 worldDir = cart->get_presence()->get_placement().vector_to_world(interactive->relativeMovementDir);
							auto* movement = fast_cast<Framework::ModuleMovementPath>(cart->get_movement());
							float interactivesTarget = 0.0f;
							Framework::IModulesOwner* elevatorStopTarget = nullptr;
							if (Vector3::dot(pa2pb, worldDir) > 0.0f)
							{
								interactivesTarget = 1.0f;
								elevatorStopTarget = elevatorStopB.get();
							}
							else
							{
								interactivesTarget = 0.0f;
								elevatorStopTarget = elevatorStopA.get();
							}
							{
								if (movement && movement->get_t() == interactivesTarget)
								{
									buttonState = ButtonState::Disabled;
								}
								else if (elevatorState == ElevatorState::Moves)
								{
									if (targetStop != elevatorStopTarget)
									{
										buttonState = ButtonState::PressToMove; // to change direction
									}
									else
									{
										buttonState = ButtonState::Busy;
									}
								}
								else if (elevatorState == ElevatorState::StoppedWithPlayer ||
										 allowActiveButtonWithoutPlayer) // if at the other end, we already have it covered above
								{
									buttonState = ButtonState::PressToMove;
									if (auto* gd = GameDirector::get_active())
									{
										if (gd->are_elevators_blocked())
										{
											buttonState = ButtonState::Disabled;
										}
									}
								}
								else // no player?
								{
									buttonState = ButtonState::Disabled;
								}
							}
						}
						else
						{
							if (elevatorState == ElevatorState::Moves)
							{
								buttonState = ButtonState::Busy;
							}
							else if (elevatorState == ElevatorState::StoppedWithPlayer)
							{
								buttonState = ButtonState::PressToMove;
								if (auto* gd = GameDirector::get_active())
								{
									if (gd->are_elevators_blocked())
									{
										buttonState = ButtonState::Disabled;
									}
								}
							}
							else
							{
								buttonState = ButtonState::Disabled;
							}
						}
						if (auto* mms = fast_cast<Framework::ModuleMovementSwitch>(interactive->imo->get_movement()))
						{
							if (mms->is_active_at(1))
							{
								buttonState = ButtonState::Pressed;
							}
						}
						if (interactive->buttonState != buttonState)
						{
							if (auto * ec = interactive->imo->get_custom<CustomModules::EmissiveControl>())
							{
								if (buttonState == ButtonState::PressToMove)
								{
									ec->emissive_deactivate(NAME(active));
									ec->emissive_activate(NAME(ready));
									ec->emissive_activate(NAME(pulse));
									ec->emissive_deactivate(NAME(busy));
								}
								else if (buttonState == ButtonState::Busy)
								{
									ec->emissive_deactivate(NAME(active));
									ec->emissive_deactivate(NAME(ready));
									ec->emissive_deactivate(NAME(pulse));
									ec->emissive_activate(NAME(busy));
								}
								else if (buttonState == ButtonState::Pressed)
								{
									ec->emissive_activate(NAME(active));
									ec->emissive_deactivate(NAME(ready));
									ec->emissive_deactivate(NAME(pulse));
									ec->emissive_deactivate(NAME(busy));
								}
								else
								{
									ec->emissive_deactivate(NAME(active));
									ec->emissive_deactivate(NAME(ready));
									ec->emissive_deactivate(NAME(pulse));
									ec->emissive_deactivate(NAME(busy));
								}
							}
						}
						interactive->buttonState = buttonState;
					}
				}
			}

			if (cart->get_presence()->get_in_room())
			{
				bool atCartPoint = false;
				bool wasMoving = isMoving;
				bool changedTarget = false;
				bool doorShouldBeOpen = doorOpen == 1.0f;
				if (auto* movement = fast_cast<Framework::ModuleMovementPath>(cart->get_movement()))
				{
					bool cartBasesPlayer = does_object_base_player(cart);
					bool stopABasesPlayer = does_object_base_player(elevatorStopA.get());
					bool stopBBasesPlayer = does_object_base_player(elevatorStopB.get());

					bool isEmpty = cart->get_presence()->get_based().is_empty();

					if (!isEmpty)
					{
						isEmpty = true;
						for_every_ptr(based, cart->get_presence()->get_based())
						{
							if (based->get_ai())
							{
								if (auto * o = based->get_as_object())
								{
									if (! o->get_tags().get_tag(NAME(elevatorIgnore)))
									{
										isEmpty = false;
										break;
									}
								}
							}
						}
					}

					bool followsPlayerOnly = false;
					if (auto* cartInRoom = cart->get_presence()->get_in_room())
					{
						if (cartInRoom->was_recently_seen_by_player())
						{
							if (auto* gd = GameDirector::get())
							{
								if (gd->game_do_elavators_follow_player_only())
								{
									followsPlayerOnly = true;
								}
							}
						}
					}

					if (!movement->is_using_any_path())
					{
						movement->use_linear_path(Transform::identity, Transform::identity, 0.0f);
					}

					float useSpeed = isEmpty ? speedEmpty : speedOccupied;
#ifdef AN_DEVELOPMENT_OR_PROFILER
					if (PlayerSetup::access_current_if_exists()) // check for preview
#endif
					{
						if (PlayerSetup::get_current().get_preferences().cartsTopSpeed > 0.0f && cartBasesPlayer)
						{
							useSpeed = min(useSpeed, PlayerSetup::get_current().get_preferences().cartsTopSpeed);
						}
						if (cartBasesPlayer)
						{
							useSpeed *= PlayerSetup::get_current().get_preferences().cartsSpeedPct;
						}
						else
						{
							useSpeed *= 1.0f - (1.0f - PlayerSetup::get_current().get_preferences().cartsSpeedPct) * 0.5f; // faster a bit
						}
					}
					useSpeed *= abs(directSpeedRequested.get(1.0f)); // we will limit the speed to what we request
					if (forcedStop)
					{
						useSpeed = 0.0f;
					}
					movement->set_speed(useSpeed);

					// if we have vr user on board, do not use accelerations
					bool vrOnBoard = false;
					for_every_ptr(imo, cart->get_presence()->get_based())
					{
						if (imo->get_presence()->is_vr_movement())
						{
							vrOnBoard = true;
						}
					}
					movement->set_acceleration(vrOnBoard? 0.0f : acceleration);

					// to keep updated values
					Transform worldCartPointA = elevatorStopA->get_presence()->get_placement().to_world(elevatorStopACartPoint);
					Transform worldCartPointB = elevatorStopB->get_presence()->get_placement().to_world(elevatorStopBCartPoint);
					float t = movement->get_t();

					float stayForDeltaTimeCoef = 1.0f; // to allow slower stayFor modifications

					// handle AI movement
					if (!moveOnlyWithPlayer &&
						!forcedMove &&
						!forcedStop &&
						forceRemainStationary == 0.0f)
					{
						bool handledByAI = false;
						bool allowAI = !followsPlayerOnly;
						if (allowAI && navElement && !cartBasesPlayer &&
							!does_object_base_player_facing(elevatorStopA.get(), cart) &&
							!does_object_base_player_facing(elevatorStopB.get(), cart))
						{
							auto * cartInRoom = cart->get_presence()->get_in_room();
							Vector3 cartPlacement = cart->get_presence()->get_placement().get_translation();

							Framework::IModulesOwner* closest = nullptr;
							float closestDistSq = 0.0f;
							Framework::IModulesOwner* closestTargetStop = nullptr;

#ifdef AN_DEBUG_RENDERER
							bool anyRequestor = false;
#endif

							// check if cart is used (requested) by someone (get closest one) (->closest)
							// and determine where do they want to go (->closestTargetStop)
							navElement->for_nodes([this, cartInRoom, cartPlacement, &closest, &closestDistSq, &closestTargetStop, &stayForDeltaTimeCoef
#ifdef AN_DEBUG_RENDERER
								, &anyRequestor
#endif
								](Framework::Nav::Node* navNode)
							{
								navNode->do_for_all_requestors([this, navNode, cartInRoom, cartPlacement, &closest, &closestDistSq, &closestTargetStop, &stayForDeltaTimeCoef
#ifdef AN_DEBUG_RENDERER
									, &anyRequestor
#endif
									](Framework::IModulesOwner* _owner)
								{
#ifdef AN_DEBUG_RENDERER
									anyRequestor = true;
#endif
									if (_owner->get_presence()->get_in_room() == cartInRoom)
									{
										float distSq = Vector3::distance_squared(_owner->get_presence()->get_placement().get_translation(), cartPlacement);
										if (!closest || distSq < closestDistSq)
										{
											closestDistSq = distSq;
											closest = _owner;
											closestTargetStop = nullptr;
											if (navNode == navElement->get_cart_point_a_node())
											{
												closestTargetStop = elevatorStopA.get();
											}
											else if (navNode == navElement->get_cart_point_b_node())
											{
												closestTargetStop = elevatorStopB.get();
											}
										}
									}
									else
									{
										stayForDeltaTimeCoef = 0.2f; // just make it slower due to AI requesting it but being outside the room
									}
								});
							});

#ifdef AN_DEBUG_RENDERER
							if (anyRequestor || closest)
							{
								if (debug_is_filter_allowed())
								{
									if (closest)
									{
										debug_draw_text(true, Colour::red, debugAt, NP, true, debugFontSize, NP, TXT("CL-REQ"));
									}
									else
									{
										debug_draw_text(true, Colour::c64LightBlue, debugAt, NP, true, debugFontSize, NP, TXT("req"));
									}
									debugAt += debugMove;
								}
							}
#endif

							if (closest)
							{
								if (!closestTargetStop)
								{
									// most probably we were waiting for a cart and it is coming our way
									// we point at node at the cart, so we may require some time before we enter it
									if (does_object_base(elevatorStopA.get(), closest))
									{
										closestTargetStop = elevatorStopA.get();
									}
									else if (does_object_base(elevatorStopB.get(), closest))
									{
										closestTargetStop = elevatorStopB.get();
									}
								}
								if (!closestTargetStop && does_object_base(cart, closest))
								{
									// we entered the cart, stay wherever we are
									closestTargetStop = targetStop;
								}
								if (closestTargetStop)
								{
#ifdef AN_DEBUG_RENDERER
									if (debug_is_filter_allowed())
									{
										debug_draw_text(true, Colour::red, debugAt, NP, true, debugFontSize, NP, TXT("AI"));
										debugAt += debugMove;
									}
#endif
									handledByAI = true;
									waitingForAI = true;
									if (does_object_base(cart, closest))
									{
										if (!aiOnBoard)
										{
											aiOnBoard = true;
											if (closest->get_presence()->get_velocity_linear().length() > 0.05f)
											{
												stayFor = 1.0f;
											}
										}
										if (closest->get_presence()->get_velocity_linear().length() <= 0.05f)
										{
											stayFor = min(stayFor, 0.1f);
										}
									}
									else
									{
										stayFor = 0.0f;
										aiOnBoard = false;
									}
									targetStop = closestTargetStop;
									waitingForSomeone = true;
								}
							}
						}
						if (! handledByAI)
						{
							if (stopABasesPlayer)
							{
								targetStop = elevatorStopA.get();
								waitingForSomeone = true;
								stayFor = 0.0f; // stay
							}
							else if (stopBBasesPlayer)
							{
								targetStop = elevatorStopB.get();
								waitingForSomeone = true;
								stayFor = 0.0f; // stay
							}
							else
							{
								bool handled = false;
								bool playerOnCart = cartBasesPlayer;
								if (!playerOnCart)
								{
									for (int pIdx = 0; pIdx < 2; ++ pIdx)
									{
										for (int pBeyond = 0; pBeyond < 2; ++pBeyond)
										{
											auto & elevatorStopDoor = pIdx == 0 ? (pBeyond ? elevatorStopADoorBeyond : elevatorStopADoor) : (pBeyond ? elevatorStopBDoorBeyond : elevatorStopBDoor);
											if (elevatorStopDoor.get())
											{
												if (has_player_close(elevatorStopDoor.get()) ||
													has_player_close(elevatorStopDoor->get_door_on_other_side()))
												{
													targetStop = pIdx == 0? elevatorStopA.get() : elevatorStopB.get();
													waitingForSomeone = true;
													stayFor = 0.0f;
													handled = true;
#ifdef AN_DEBUG_RENDERER
													if (debug_is_filter_allowed())
													{
														debug_draw_text(true, Colour::red, debugAt, NP, true, debugFontSize, NP, TXT("plr"));
														debugAt += debugMove;
													}
#endif
												}
											}
										}
									}
								}
								if (!handled)
								{
									if (waitingForAI)
									{
										// go where you wanted to go, AI may struggle to remain in same room
										stayFor = 0.0f;
									}
									else if (waitingForSomeone)
									{
										// release and go somewhere else
										targetStop = t == 0.0f ? elevatorStopB.get() : elevatorStopA.get();
										stayFor = playerOnCart? waitTimeAfterPlayerLeftBaseCart : waitTimeWithoutPlayerAfterPlayerLeftBaseCart.get_at(Random::get_float(0.0f, 1.0f));
									}
									else if (followsPlayerOnly)
									{
										// stay where you are
										targetStop = t == 0.0f ? elevatorStopA.get() : elevatorStopB.get();
										stayFor = 1.0f;
									}
									waitingForSomeone = false;
								}
							}
							waitingForAI = false;
						}
					}
#ifdef AN_DEBUG_RENDERER
					if (debug_is_filter_allowed())
					{
						debug_draw_text(true, Colour::green, debugAt, NP, true, debugFontSize, NP, elevatorStopA == targetStop ? TXT("A"): TXT("B"));
						debugAt += debugMove;
						if (forcedMove)
						{
							debug_draw_text(true, Colour::red, debugAt, NP, true, debugFontSize, NP, TXT("forced"));
							debugAt += debugMove;
						}
						if (forcedStop)
						{
							debug_draw_text(true, Colour::red, debugAt, NP, true, debugFontSize, NP, TXT("STOP"));
							debugAt += debugMove;
						}
						if (forceRemainStationary > 0.0f)
						{
							debug_draw_text(true, Colour::magenta, debugAt, NP, true, debugFontSize, NP, TXT("frs:%.3f"), forceRemainStationary);
							debugAt += debugMove;
						}
						if (timeStationary > 0.0f)
						{
							debug_draw_text(true, Colour::c64LightBlue, debugAt, NP, true, debugFontSize, NP, TXT("ts:%.3f"), timeStationary);
							debugAt += debugMove;
						}
					}
#endif

					float targetT = elevatorStopA == targetStop ? 0.0f : 1.0f;
					{
						if (cartPath == ElevatorCartPath::CurveMoveXZ)
						{
							BezierCurve<Vector3> path;
							path.p0 = worldCartPointA.get_translation();
							path.p3 = worldCartPointB.get_translation();
							path.p1 = path.p0 + (path.p3 - path.p0).drop_using(worldCartPointA.get_axis(Axis::Y));
							path.p2 = path.p3 + (path.p0 - path.p3).drop_using(worldCartPointB.get_axis(Axis::Y));
							path.make_roundly_separated();
							Framework::ModuleMovementPath::CurveSinglePathExtraParams curveExtraParams;
							curveExtraParams.moveInwards = 0.12f; // this should be enough to prevent intersection in most of tha cases
							curveExtraParams.moveInwardsT = 0.1f; // just at the ends
							movement->use_curve_single_path(worldCartPointA, worldCartPointB, path, t, curveExtraParams); // place where we think we are
						}
						else
						{
							movement->use_linear_path(worldCartPointA, worldCartPointB, t); // place where we think we are
						}
					}

					if (forcedStop)
					{
						shouldMove = false;
					}
					else
					{
						if (directSpeedControl)
						{
							stayFor = 0.0f;
							waitingForSomeone = false;
							waitingForAI = false;
							aiOnBoard = false;
							if (directSpeedRequested.is_set())
							{
								forcedMove = true;
								if (directSpeedRequested.get() > 0.0f)
								{
									targetT = 1.0f;
									targetStop = elevatorStopB.get();
								}
								else if (directSpeedRequested.get() < 0.0f)
								{
									targetT = 0.0f;
									targetStop = elevatorStopA.get();
								}
#ifdef DEBUG_ELEVATOR_MOVING_ALONE
								bool prevShouldMove = shouldMove;
#endif
								shouldMove = abs(directSpeedRequested.get()) > 0.01f || targetT != t;
#ifdef DEBUG_ELEVATOR_MOVING_ALONE
								if (moveOnlyWithPlayer && 
									shouldMove && !prevShouldMove)
								{
									output(TXT("[ELEVATOR] ordered to move via direct speed request"));
								}
#endif
							}
							else
							{
								forcedMove = false;
								shouldMove = false;
							}
						}
						else
						{
							// check if we should move
							if (targetT == t)
							{
								if (!waitingForSomeone && !moveOnlyWithPlayer)
								{
									// switch (logic) but stay for a while, we will do the actual movement when the time passes
									targetStop = t == 0.0f ? elevatorStopB.get() : elevatorStopA.get();
									stayFor = get_wait_time_at_target(cart);
								}
								shouldMove = false;
#ifdef AN_DEVELOPMENT_OR_PROFILER
								if (forcedMove)
								{
									ai_log(this, TXT("ended forced move at %0.3f"), t);
								}
#endif
								forcedMove = false;
							}
							else if (forcedMove)
							{
#ifdef DEBUG_ELEVATOR_MOVING_ALONE
								bool prevShouldMove = shouldMove;
#endif
								stayFor = 0.0f;
								shouldMove = true;
#ifdef DEBUG_ELEVATOR_MOVING_ALONE
								if (moveOnlyWithPlayer &&
									shouldMove && !prevShouldMove)
								{
									output(TXT("[ELEVATOR] forced to move"));
								}
#endif
							}
							else
							{
								stayFor = min(stayFor, get_wait_time_at_target(cart));
								stayFor -= _deltaTime * stayForDeltaTimeCoef;
#ifdef AN_DEVELOPMENT_OR_PROFILER
								if (cartBasesPlayer && Game::get_as<Game>()->is_autopilot_on())
								{
									stayFor = min(stayFor, movedWithPlayer? 7.0f : 0.5f); // enough to get off
								}
#endif
								if (stayFor < 0.0f)
								{
									if ((
#ifdef AN_DEVELOPMENT_OR_PROFILER
										PlayerSetup::access_current_if_exists() && // check for preview
#endif
										!moveOnlyWithPlayer &&
										(PlayerSetup::get_current().get_preferences().cartsAutoMovement ||
										 !cartBasesPlayer))
#ifdef AN_DEVELOPMENT_OR_PROFILER
										|| Game::get_as<Game>()->is_autopilot_on()
#endif
										)
									{
#ifdef DEBUG_ELEVATOR_MOVING_ALONE
										bool prevShouldMove = shouldMove;
#endif
										shouldMove = true;
#ifdef DEBUG_ELEVATOR_MOVING_ALONE
										if (moveOnlyWithPlayer &&
											shouldMove && !prevShouldMove)
										{
											output(TXT("[ELEVATOR] move because of long stay"));
										}
#endif
									}
									else
									{
										stayFor = 0.001f; // pretend we're still waiting to start (otherwise ordering with connectable won't work properly) - obsolete? no longer needed?
									}
								}
							}
						}
					}

					bool closeDoor = shouldMove;

					if (forcedStop || (!forcedMove && forceRemainStationary > 0.0f && (t == 0.0f || t == 1.0f)))
					{
						// remain where you are
						shouldMove = false;
						closeDoor = t != 0.0f && t != 1.0f;
						targetT = t;
					}

					if (doorOpenOnlyWhenStationary && doorOpen > 0.0f)
					{
						// keep stationary till we move
						shouldMove = false;
					}

					float requestedTargetT = targetT;
					if (shouldMove || (forcedStop && t != 0.0f && t != 1.0f))
					{
						cart->get_presence()->lock_as_base(true, true); // vr and player
						if (cartBasesPlayer)
						{
							movedWithPlayer = true;
						}
					}
					else
					{
						cart->get_presence()->unlock_as_base();
						targetT = t;
						if (!cartBasesPlayer)
						{
							movedWithPlayer = false;
						}
					}
					
					if (forcedStop)
					{
						changedTarget = false;
						movement->clear_target_t();
						isMoving = false;
					}
					else
					{
						changedTarget = movement->get_target_t().get(targetT) != targetT;
						movement->set_target_t(targetT);
						isMoving = t != targetT;
						if (directSpeedRequested.is_set())
						{
							if (abs(directSpeedRequested.get()) < 0.01f)
							{
								isMoving = false;
							}
						}
					}
					bool wantsToMove = t != requestedTargetT && ! forcedStop;

					// do this check only if changed target, it's enough
					if (changedTarget && ! forcedStop)
					{
						ai_log(this, TXT("changed target to %S"), elevatorStopA == targetStop? TXT("A") : TXT("B"));
						if (is_player_waiting_at(elevatorStopA == targetStop))
						{
							ai_log(this, TXT("player waiting there, force going there"));
							// we have to go to the player, period
							order_to_move(targetStop);
							forceRemainStationary = magic_number 3.0f;
						}
						else
						{
							if (consecutiveChangesTime > 3.0f)
							{
								consecutiveChangesTime = 0.0f;
								consecutiveChangesCount = 0;
							}
							++consecutiveChangesCount = 0;
							if (consecutiveChangesCount > 2)
							{
								// force to go in one direction (if it is by player, player will use buttons)
								ai_log(this, TXT("too many times changed mind in a short time, force going somewhere"));
								order_to_move(targetStop);
							}
						}
					}
					else
					{
						consecutiveChangesTime += _deltaTime;
					}

					// if we're stationary, check if there is player waiting on other platform and go there
					if (!forcedStop && !wantsToMove && timeStationary > 5.0f && timeToCheckPlayer <= 0.0f && moveOnlyWithPlayer)
					{
						timeToCheckPlayer = 1.0f;
						if (is_player_waiting_at(targetT == 1.0f)) // check other stop
						{
							order_to_move(targetT == 1.0f? elevatorStopA.get() : elevatorStopB.get());
							forceRemainStationary = magic_number 3.0f;
						}
					}

					atCartPoint = t == 0.0f || t == 1.0f;
					doorShouldBeOpen = !closeDoor && targetT == t;
					if (!doorOpenOnlyWhenStationary)
					{
						if (doorOpenWhenEmpty && isEmpty)
						{
							doorShouldBeOpen = true;
						}
						if (doorOpenEarlyWhenStopping && !forcedStop)
						{
							Vector3 lane = worldCartPointB.get_translation() - worldCartPointA.get_translation();
							float distanceLeft = (lane * (targetT - t)).length();
							if (distanceLeft <= useSpeed * doorOpenTime * 1.1f)
							{
								doorShouldBeOpen = true;
							}
						}
					}
					{
						float newDoorOpen = blend_to_using_speed_based_on_time(doorOpen, doorShouldBeOpen? 1.0f : 0.0f, doorOpenTime, _deltaTime);
						set_advance_logic_rarely(newDoorOpen == doorOpen? Optional<Range>() : Range::empty); // if standing with open or closed doors, update rarely, otherwise every frame
						set_advance_appearance_rarely(newDoorOpen == doorOpen && !isMoving && !cartBasesPlayer && !stopABasesPlayer && ! stopBBasesPlayer? Range(0.5f, 2.0f) : Range::empty); // rare or every frame (to update doors)
						doorOpen = newDoorOpen;
					}

					// handle messages (create and send)
					if (doorShouldBeOpen)
					{
						if (targetT == 0.0f)
						{
							be_open(cartPointA.get(), REF_ cartPointAOpen);
							be_closed(cartPointB.get(), REF_ cartPointBOpen);
						}
						else
						{
							be_closed(cartPointA.get(), REF_ cartPointAOpen);
							be_open(cartPointB.get(), REF_ cartPointBOpen);
						}
					}
					else
					{
						be_closed(cartPointA.get(), REF_ cartPointAOpen);
						be_closed(cartPointB.get(), REF_ cartPointBOpen);
					}

					if (atCartPoint && doorOpen == 1.0f)
					{
						forceRemainStationary = max(0.0f, forceRemainStationary - _deltaTime);
						timeStationary += _deltaTime;
						timeToCheckPlayer = max(0.0f, timeToCheckPlayer - _deltaTime);
					}
					else
					{
						// keep forceRemainStationary
						timeStationary = 0.0f;
						timeToCheckPlayer = 0.0f;
					}

					if (atCartPoint)
					{
						if (targetT == 0.0f)
						{
							be_at_cart_point(cartPointA.get(), atCartPointA);
							be_not_at_cart_point(cartPointB.get(), atCartPointB);
						}
						else
						{
							be_not_at_cart_point(cartPointA.get(), atCartPointA);
							be_at_cart_point(cartPointB.get(), atCartPointB);
						}
					}
					else
					{
						be_not_at_cart_point(cartPointA.get(), atCartPointA);
						be_not_at_cart_point(cartPointB.get(), atCartPointB);
					}

					if (closeRoomDoorWhileMoving || openRoomDoorWhileStoppedAndBasesPlayer)
					{
						for_count(int, dIdx, 2)
						{
							if (auto* elevatorStopDoor = dIdx == 0 ? elevatorStopADoor.get() : elevatorStopBDoor.get())
							{
								if (auto* d = elevatorStopDoor->get_door())
								{
									if ((dIdx == 0 && atCartPointA) ||
										(dIdx == 1 && atCartPointB))
									{
										// keep auto eager to close if set for any reason
										if (d->get_operation() != Framework::DoorOperation::AutoEagerToClose)
										{
											bool allowAuto = true;
											if (openRoomDoorWhileStoppedAndBasesPlayer)
											{
												if (does_object_base_player(cart))
												{
													allowAuto = false;
												}
											}
											d->set_operation(Framework::DoorOperation::StayOpen, Framework::Door::DoorOperationParams().allow_auto(allowAuto));
										}
									}
									else if (closeRoomDoorWhileMoving)
									{
										d->set_operation(Framework::DoorOperation::StayClosedTemporarily);
									}
								}
							}
						}
					}
				}

				if (auto * appearance = cart->get_appearance())
				{
					auto & acVars = appearance->access_controllers_variable_storage();
					if (!doorOpenACVar.is_valid())
					{
						doorOpenACVar = acVars.find<float>(doorOpenACVarID);
					}
					if (!doorMovementCollisionOpenACVar.is_valid())
					{
						doorMovementCollisionOpenACVar = acVars.find<float>(doorMovementCollisionOpenACVarID);
					}

					doorOpenACVar.access<float>() = doorOpen;
					doorMovementCollisionOpenACVar.access<float>() = (doorOpen > 0.99f || (doorOpen > 0.3f && doorShouldBeOpen)) && atCartPoint ? 1.0f : 0.0f; // this is to make sure character won't go off while on moving cart
				}

				if (isMoving != wasMoving)
				{
					bool beAware = isMoving;
					if (beAware)
					{
						if (auto* r = cart->get_presence()->get_in_room())
						{
							beAware = AVOID_CALLING_ACK_ r->was_recently_seen_by_player();
						}
					}
					be_aware(cartPointA.get(), beAware, REF_ cartPointAAware);
					be_aware(cartPointB.get(), beAware, REF_ cartPointBAware);
				}


				if (auto* sound = cart->get_sound())
				{
					if (isMoving && changedTarget)
					{
						sound->play_sound(NAME(changeDir));
					}
					if (isMoving && !wasMoving)
					{
						sound->play_sound(NAME(movement));
					}
					else if (! isMoving && wasMoving)
					{
						sound->stop_sound(NAME(movement));
					}
				}

				if (navElement)
				{
					if (auto * cpAConnection = navElement->access_cart_point_a_connection())
					{
						bool open = cartPointALogic ? cartPointALogic->get_door_open() == 1.0f : true;
						open &= atCartPointA;
						todo_note(TXT("create nav task to change blocked status - lazy one - ?"));
						cpAConnection->set_blocked_temporarily(!open);
					}
					if (auto * cpBConnection = navElement->access_cart_point_b_connection())
					{
						bool open = cartPointBLogic ? cartPointBLogic->get_door_open() == 1.0f : true;
						open &= atCartPointB;
						todo_note(TXT("create nav task to change blocked status - lazy one - ?"));
						cpBConnection->set_blocked_temporarily(!open);
					}
				}
			}

			debug_no_context();
			debug_no_filter();
			debug_no_subject();
		}
	}
}

bool ElevatorCart::is_player_waiting_at(bool _atA) const
{
	if (auto * cart = get_mind()->get_owner_as_modules_owner())
	{
		auto* elevatorStop = _atA ? elevatorStopA.get() : elevatorStopB.get();
		if (elevatorStop && does_object_base_player_facing(elevatorStop, cart))
		{
			return true;
		}
		for (int pBeyond = 0; pBeyond < 2; ++pBeyond)
		{
			auto& elevatorStopDoor = _atA? (pBeyond ? elevatorStopADoorBeyond : elevatorStopADoor) : (pBeyond ? elevatorStopBDoorBeyond : elevatorStopBDoor);
			if (elevatorStopDoor.get())
			{
				if (has_player_close(elevatorStopDoor.get()) ||
					has_player_close(elevatorStopDoor->get_door_on_other_side()))
				{
					return true;
				}
			}
		}
	}
	return false;
}

float ElevatorCart::get_wait_time_at_target(Framework::IModulesOwner* cart) const
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (PlayerSetup::access_current_if_exists()) // check for preview
#endif
	if (does_object_base_player(cart) && movedWithPlayer)
	{
		return PlayerSetup::get_current().get_preferences().cartsStopAndStay ? 900000.0f : waitTimeAtTargetWithPlayer;
	}
	return currentWaitTimeAtTarget;
}

void ElevatorCart::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);

	if (auto* cart = get_mind()->get_owner_as_modules_owner())
	{
		closeRoomDoorWhileMoving = _parameters.get_value<bool>(NAME(closeRoomDoorWhileMoving), closeRoomDoorWhileMoving);
		openRoomDoorWhileStoppedAndBasesPlayer = _parameters.get_value<bool>(NAME(openRoomDoorWhileStoppedAndBasesPlayer), openRoomDoorWhileStoppedAndBasesPlayer);
		directSpeedControl = _parameters.get_value<bool>(NAME(directSpeedControl), directSpeedControl);
		navElement = fast_cast<ModuleNavElements::Cart>(cart->get_nav_element());
		if (auto* movement = fast_cast<Framework::ModuleMovementPath>(cart->get_movement()))
		{
			speedOccupied = _parameters.get_value<float>(NAME(speedOccupied), movement->get_speed());
			speedEmpty = _parameters.get_value<float>(NAME(speedEmpty), movement->get_speed());
			acceleration = _parameters.get_value<float>(NAME(acceleration), movement->get_acceleration());
		}
		speedOccupied = _parameters.get_value<float>(NAME(speedOccupied), speedOccupied);
		speedEmpty = _parameters.get_value<float>(NAME(speedEmpty), speedEmpty);
		acceleration = _parameters.get_value<float>(NAME(acceleration), acceleration);
	}

	if (_parameters.get_value<bool>(NAME(noActiveButtonsForElevatorsWithoutPlayer), false))
	{
		allowActiveButtonWithoutPlayer = false;
	}

	moveOnlyWithPlayer = _parameters.get_value<bool>(NAME(moveOnlyWithPlayer), moveOnlyWithPlayer);

	doorOpenWhenEmpty = _parameters.get_value<bool>(NAME(doorOpenWhenEmpty), doorOpenWhenEmpty);
	doorOpenEarlyWhenStopping = _parameters.get_value<bool>(NAME(doorOpenEarlyWhenStopping), doorOpenEarlyWhenStopping);
	doorOpenOnlyWhenStationary = _parameters.get_value<bool>(NAME(doorOpenOnlyWhenStationary), doorOpenOnlyWhenStationary);
	doorOpenTime = _parameters.get_value<float>(NAME(doorOpenTime), doorOpenTime);
	if (auto* val = _parameters.get_existing<float>(NAME(waitTimeWithoutPlayerAfterPlayerLeftBaseCart)))
	{
		waitTimeWithoutPlayerAfterPlayerLeftBaseCart = Range(*val); // single value
	}
	waitTimeWithoutPlayerAfterPlayerLeftBaseCart = _parameters.get_value<Range>(NAME(waitTimeWithoutPlayerAfterPlayerLeftBaseCart), waitTimeWithoutPlayerAfterPlayerLeftBaseCart);
	waitTimeAfterPlayerLeftBaseCart = _parameters.get_value<float>(NAME(waitTimeAfterPlayerLeftBaseCart), waitTimeAfterPlayerLeftBaseCart);
	waitTimeAtTarget = _parameters.get_value<Range>(NAME(waitTimeAtTarget), waitTimeAtTarget);
	waitTimeAtTargetWithPlayer = _parameters.get_value<float>(NAME(waitTimeAtTargetWithPlayer), waitTimeAtTargetWithPlayer);

	Name cartPathName = _parameters.get_value<Name>(NAME(cartPath), Name::invalid());
	if (cartPathName.is_valid())
	{
		if (cartPathName == NAME(linear))
		{
			cartPath = ElevatorCartPath::Linear;
		}
		else if (cartPathName == NAME(curveMoveXZ))
		{
			cartPath = ElevatorCartPath::CurveMoveXZ;
		}
		else
		{
			error(TXT("cartPath \"%S\" not recognised"), cartPathName.to_char());
		}
	}

	findStopsAndCartPoints = ! find_stops_and_cart_points();

	currentWaitTimeAtTarget = waitTimeAtTarget.get_at(Random::get_float(0.0f, 1.0f));
}

bool ElevatorCart::find_stops_and_cart_points(bool _mayFail)
{
	auto* imo = get_mind()->get_owner_as_modules_owner();
	if (!imo)
	{
		return false;
	}

	MODULE_OWNER_LOCK_FOR_IMO(imo, TXT("ElevatorCart::find_stops_and_cart_points"));

	auto& useParameters = imo->get_variables();

	Framework::IModulesOwner* foundElevatorStopA = nullptr;
	Framework::IModulesOwner* foundElevatorStopB = nullptr;
	Framework::IModulesOwner* foundElevatorCartPointA = nullptr;
	Framework::IModulesOwner* foundElevatorCartPointB = nullptr;
	Optional<Transform> foundElevatorStopACartPoint;
	Optional<Transform> foundElevatorStopBCartPoint;
	for_count(int, i, 2)
	{
		Framework::IModulesOwner*& found = i == 0 ? foundElevatorStopA : foundElevatorStopB;
		if (!found)
		{
			if (auto* elevatorStopParam = useParameters.get_existing<SafePtr<Framework::IModulesOwner>>(i == 0? NAME(elevatorStopA) : NAME(elevatorStopB)))
			{
				found = elevatorStopParam->get();
			}
		}
		if (!found)
		{
			if (auto* elevatorStopTagged = useParameters.get_existing<TagCondition>(i == 0 ? NAME(elevatorStopATagged) : NAME(elevatorStopBTagged)))
			{
				if (auto* inRoom = imo->get_presence()->get_in_room())
				{
					for_every_ptr(o, inRoom->get_objects())
					{
						if (elevatorStopTagged->check(o->get_tags()))
						{
							found = o;
							break;
						}
					}
				}
			}
		}
		//
		Optional<Transform>& foundCP = i == 0 ? foundElevatorStopACartPoint : foundElevatorStopBCartPoint;
		if (!foundCP.is_set())
		{
			if (auto* elevatorStopCartPointParam = useParameters.get_existing<Transform>(i == 0 ? NAME(elevatorStopACartPoint) : NAME(elevatorStopBCartPoint)))
			{
				foundCP = *elevatorStopCartPointParam;
			}
		}
		if (!foundCP.is_set())
		{
			if (auto* elevatorStopCartPointFromPOIParam = useParameters.get_existing<Name>(i == 0 ? NAME(elevatorStopACartPointFromPOI) : NAME(elevatorStopBCartPointFromPOI)))
			{
				if (auto* ap = imo->get_appearance())
				{
					ap->for_every_point_of_interest(*elevatorStopCartPointFromPOIParam,
						[&foundCP](Framework::PointOfInterestInstance* _fpoi)
						{
							foundCP = _fpoi->calculate_placement().to_local(Transform::identity);
						}
					);
				}
			}
		}
		//
		Framework::IModulesOwner*& foundCartPoint = i == 0 ? foundElevatorCartPointA : foundElevatorCartPointB;
		if (!foundCartPoint)
		{
			if (auto* elevatorCartPointTagged = useParameters.get_existing<TagCondition>(i == 0 ? NAME(elevatorCartPointATagged) : NAME(elevatorCartPointBTagged)))
			{
				if (auto* inRoom = imo->get_presence()->get_in_room())
				{
					for_every_ptr(o, inRoom->get_objects())
					{
						if (elevatorCartPointTagged->check(o->get_tags()))
						{
							foundCartPoint = o;
							break;
						}
					}
				}
			}
		}
	}
	if (foundElevatorStopA &&
		foundElevatorStopB &&
		foundElevatorStopACartPoint.is_set() &&
		foundElevatorStopBCartPoint.is_set())
	{
		elevatorStopA = foundElevatorStopA;
		elevatorStopB = foundElevatorStopB;
		elevatorStopACartPoint = foundElevatorStopACartPoint.get();
		elevatorStopBCartPoint = foundElevatorStopBCartPoint.get();
		isValid = elevatorStopA.get() && elevatorStopB.get();

		cartPointA = foundElevatorCartPointA? foundElevatorCartPointA : find_cart_point(elevatorStopA.get(), elevatorStopACartPoint);
		cartPointB = foundElevatorCartPointB? foundElevatorCartPointB : find_cart_point(elevatorStopB.get(), elevatorStopBCartPoint);
		cartPointALogic = get_cart_point_logic(cartPointA.get());
		cartPointBLogic = get_cart_point_logic(cartPointB.get());

		elevatorStopADoor.clear();
		elevatorStopBDoor.clear();
		elevatorStopADoorBeyond.clear();
		elevatorStopBDoorBeyond.clear();
		auto* cart = get_mind()->get_owner_as_modules_owner();
		if (cart)
		{
			for (int p = 0; p < 2; ++p)
			{
				auto * elevatorStop = p == 0 ? elevatorStopA.get() : elevatorStopB.get();
				auto cpPlacementWS = elevatorStop->get_presence()->get_placement().to_world(p == 0 ? elevatorStopACartPoint : elevatorStopBCartPoint);
				Framework::DoorInRoom* bestDir = nullptr;
				float bestDist = 0.0f;
				for_every_ptr(dir, elevatorStop->get_presence()->get_in_room()->get_all_doors())
				{
					// invisible doors are fine here
					float dist = (dir->get_placement().get_translation() - cpPlacementWS.get_translation()).length();
					if (dist < 1.0f && magic_number // this is to focus only on doors that are close
						(dist < bestDist || !bestDir))
					{
						bestDir = dir;
						bestDist = dist;
					}
				}
				if (bestDist)
				{
					auto & elevatorStopDoor = (p == 0 ? elevatorStopADoor : elevatorStopBDoor);
					elevatorStopDoor = bestDir;
					checkForDoorBeyondDelay = 1.0f; // delay it until doors in other room are placed
				}
			}
		}

		// ready for randomly delayed movement to other stop (or stay where you are)
		if (moveOnlyWithPlayer)
		{
			targetStop = elevatorStopA.get();
			stayFor = 0.0f;
		}
		else
		{
			targetStop = elevatorStopB.get();
			if (cart)
			{
				stayFor = get_wait_time_at_target(cart) * Random::get_float(0.0f, 1.0f);
			}
			else
			{
				stayFor = Random::get_float(0.0f, 5.0f);
			}
		}
		shouldMove = false;

		return true;
	}
	else
	{
		if (!_mayFail)
		{
			error(TXT("not provided required parameters for elevator cart!"));
		}
		return false;
	}
}

ElevatorCartPoint* ElevatorCart::get_cart_point_logic(Framework::IModulesOwner* _actor)
{
	return ElevatorCartPoint::get_logic_as<ElevatorCartPoint>(_actor);
}

Framework::IModulesOwner* ElevatorCart::find_cart_point(Framework::IModulesOwner* _elevatorStop, Transform const & _relativePlacement) const
{
	Framework::IModulesOwner* cartPoint = nullptr;
	float bestDistance = 0.0f;
	for_every_ptr(based, _elevatorStop->get_presence()->get_based())
	{
		if (Framework::IModulesOwner* basedActor = fast_cast<Framework::IModulesOwner>(based))
		{
			if (get_cart_point_logic(basedActor))
			{
				float distance = (_elevatorStop->get_presence()->get_placement().to_local(based->get_presence()->get_placement()).get_translation() - _relativePlacement.get_translation()).length();
				if (distance < bestDistance || !cartPoint)
				{
					bestDistance = distance;
					cartPoint = basedActor;
				}
			}
		}
	}
	return cartPoint;
}

void ElevatorCart::be_aware(Framework::IModulesOwner* _cartPoint, bool _beAware, REF_ bool & _isAware)
{
	if (!_cartPoint || _isAware == _beAware)
	{
		return;
	}

	_isAware = _beAware;

	if (auto* world = _cartPoint->get_in_world())
	{
		if (auto* message = world->create_ai_message(_beAware? NAME(beAware) : NAME(beLazy)))
		{
			message->to_ai_object(_cartPoint);
		}
	}
}

void ElevatorCart::be_open(Framework::IModulesOwner* _cartPoint, REF_ bool & _cartPointOpen)
{
	if (_cartPointOpen || !_cartPoint)
	{
		return;
	}

	_cartPointOpen = true;
	if (auto* world = _cartPoint->get_in_world())
	{
		if (auto* message = world->create_ai_message(NAME(open)))
		{
			message->to_ai_object(_cartPoint);
			message->access_param(NAME(doorOpenTime)).access_as<float>() = doorOpenTime;
		}
	}
}

void ElevatorCart::be_closed(Framework::IModulesOwner* _cartPoint, REF_ bool & _cartPointOpen)
{
	if (!_cartPointOpen || !_cartPoint)
	{
		return;
	}

	_cartPointOpen = false;
	if (auto* world = _cartPoint->get_in_world())
	{
		if (auto* message = world->create_ai_message(NAME(close)))
		{
			message->to_ai_object(_cartPoint);
			message->access_param(NAME(doorOpenTime)).access_as<float>() = doorOpenTime;
		}
	}
}

void ElevatorCart::be_at_cart_point(Framework::IModulesOwner* _cartPoint, REF_ bool & _atCartPoint)
{
	if (_atCartPoint || !_cartPoint)
	{
		return;
	}

	_atCartPoint = true;
	if (auto* world = _cartPoint->get_in_world())
	{
		if (auto* message = world->create_ai_message(NAME(atCartPoint)))
		{
			message->to_ai_object(_cartPoint);
		}
	}
}

void ElevatorCart::be_not_at_cart_point(Framework::IModulesOwner* _cartPoint, REF_ bool & _atCartPoint)
{
	if (!_atCartPoint || !_cartPoint)
	{
		return;
	}

	_atCartPoint = false;
	if (auto* world = _cartPoint->get_in_world())
	{
		if (auto* message = world->create_ai_message(NAME(noLongerAtCartPoint)))
		{
			message->to_ai_object(_cartPoint);
			message->access_param(NAME(doorOpenTime)).access_as<float>() = doorOpenTime;
		}
	}
}

bool ElevatorCart::is_vertical() const
{
	if (elevatorStopA.get() && elevatorStopB.get())
	{
		Transform worldCartPointA = elevatorStopA->get_presence()->get_placement().to_world(elevatorStopACartPoint);
		Transform worldCartPointB = elevatorStopB->get_presence()->get_placement().to_world(elevatorStopBCartPoint);
		Vector3 a2b = worldCartPointB.get_translation() - worldCartPointA.get_translation();
		return abs(a2b.z) > a2b.length_2d();
	}

	return false;
}

