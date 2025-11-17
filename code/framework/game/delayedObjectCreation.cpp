#include "delayedObjectCreation.h"

#include "game.h"

#include "..\module\moduleAppearance.h"
#include "..\module\modulePresence.h"
#include "..\object\object.h"
#include "..\object\sceneryType.h"
#include "..\world\door.h"
#include "..\world\doorInRoom.h"
#include "..\world\doorType.h"
#include "..\world\room.h"
#include "..\world\roomType.h"
#include "..\world\subWorld.h"

#ifdef AN_OUTPUT_WORLD_GENERATION
#define OUTPUT_GENERATION
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

DEFINE_STATIC_NAME(relativeDoorSize);
DEFINE_STATIC_NAME(forceVRPlacement);
DEFINE_STATIC_NAME(tagDoor);
DEFINE_STATIC_NAME(tagThisDoor);
DEFINE_STATIC_NAME(tagOtherDoor);
DEFINE_STATIC_NAME(tagRoom);
DEFINE_STATIC_NAME(markPOIsInside);
DEFINE_STATIC_NAME(markRoomInside);
DEFINE_STATIC_NAME(markRoomInsideIncrement);
DEFINE_STATIC_NAME(requiredRoomTags);
DEFINE_STATIC_NAME(attachToOwner);
DEFINE_STATIC_NAME(vrElevatorAltitude);

// output/set parameters
DEFINE_STATIC_NAME(throughDoorSize); // this way we know what size of window we're going through

//

using namespace Framework;

bool DelayedObjectCreation::can_be_processed() const
{
	return inRoom.get() && inRoom->is_world_active();
}

bool DelayedObjectCreation::can_ever_be_processed() const
{
	return ! inRoom.is_pointing_at_something() || inRoom.get() != nullptr;
}

void DelayedObjectCreation::process(Game* _game, bool _immediate)
{
	scoped_call_stack_info(TXT("DelayedObjectCreation::process"));

	RefCountObjectPtr<DelayedObjectCreation> keepMyselfAlive(this);
	std::function<void()> do_process = 
	[this, _game, _immediate, keepMyselfAlive]()
	{
		scoped_call_stack_info(TXT("process delayed object creation"));

#ifdef OUTPUT_GENERATION
		output(TXT("process delayed creation (%p) (%S)"), this, _immediate? TXT("immediate") : TXT("normal"));
#endif
		if (inRoom.is_pointing_at_something() && !inRoom.get())
		{
#ifdef OUTPUT_GENERATION
			output(TXT("delayed creation (%p) cannot happen as was meant for room that no longer exists"), this);
#endif
		}
		else
		{
			bool ok = true;
			{
				bool allowRequiredRoomTags = true;
				if (auto* attachToOwner = variables.get_existing<bool>(NAME(attachToOwner)))
				{
					if (*attachToOwner)
					{
						allowRequiredRoomTags = false;
					}
				}
				if (allowRequiredRoomTags)
				{
					if (auto* requiredRoomTags = variables.get_existing<TagCondition>(NAME(requiredRoomTags)))
					{
						if (!requiredRoomTags->check(inRoom->get_tags()))
						{
#ifdef OUTPUT_GENERATION
							output(TXT("skip, required room tags do not match"));
#endif
							ok = false;
						}
					}
					if (roomType)
					{
						if (auto* requiredRoomTags = variables.get_existing<TagCondition>(NAME(requiredRoomTags)))
						{
							if (!requiredRoomTags->check(inRoom->get_tags()))
							{
#ifdef OUTPUT_GENERATION
								output(TXT("skip, required room tags do not match"));
#endif
								ok = false;
							}
						}
					}
				}
			}
			if (ok && (inRoom.is_set() || (inSubWorld && place_function)) && objectType)
			{
#ifdef OUTPUT_GENERATION
				::System::TimeStamp ts;
				if (!_immediate)
				{
					output(TXT("delayed creation (%p) of \"%S\" named \"%S\""), this, objectType->get_name().to_string().to_char(), name.to_char());
				}
#endif
				bool shouldCreate = true;
				if (checkSpaceBlockers && inRoom.get())
				{
					if (auto* sboType = fast_cast<SpaceBlockingObjectType>(objectType))
					{
						SpaceBlockers spaceBlockers = sboType->get_space_blocker_local();
						shouldCreate = inRoom->check_space_blockers(spaceBlockers, placement);
#ifdef OUTPUT_GENERATION
						if (!shouldCreate)
						{
							output(TXT("skip creation due to space blockers"));
						}
#endif					
					}
				}
				if (shouldCreate)
				{
					scoped_call_stack_info(objectType->get_name().to_string().to_char());

					WorldObject* worldObjectOwner = nullptr;
					if (auto* wo = fast_cast<WorldObject>(imoOwner.get()))
					{
						worldObjectOwner = wo;
					}
					ScopedAutoActivationGroup saag(worldObjectOwner, inRoom.get(), inRoom.is_set() ? inRoom->get_in_world() : inSubWorld->get_in_world());

#ifdef OUTPUT_GENERATION
					output(TXT("[DOC] spawn object \"%S\""), objectType->get_name().to_string().to_char());
#endif					

					objectType->load_on_demand_if_required();

					Object* object = nullptr;
					_game->perform_sync_world_job(TXT("spawn"), [this, &object]()
					{
						object = objectType->create(name);
						object->init(inSubWorld? inSubWorld : inRoom->get_in_sub_world());
					});

#ifdef OUTPUT_GENERATION
					output(TXT("[DOC] spawned object, set variables"));
#endif					
					object->access_tags().set_tags_from(tagSpawned);

					createdObject = object;

					object->access_variables().set_from(variables);
					an_assert(! randomGenerator.is_zero_seed());
					object->access_individual_random_generator() = randomGenerator;

					if (pre_initialise_modules_function)
					{
#ifdef OUTPUT_GENERATION
						output(TXT("[DOC] pre initialise modules function"));
#endif					
						pre_initialise_modules_function(object);
					}

#ifdef OUTPUT_GENERATION
					output(TXT("[DOC] set fallback wmp owners"));
#endif					
					if (wmpOwnerObject)
					{
						object->set_fallback_wmp_owner(wmpOwnerObject);
					}
					else if (inRoom.is_set())
					{
						object->set_fallback_wmp_owner(inRoom.get());
					}
#ifdef OUTPUT_GENERATION
					output(TXT("[DOC] initialise modules"));
#endif					
					object->initialise_modules(pre_setup_module_function);

					if (post_initialise_modules_function)
					{
#ifdef OUTPUT_GENERATION
						output(TXT("[DOC] post initialise modules function"));
#endif					
						post_initialise_modules_function(object);
					}

#ifdef OUTPUT_GENERATION
					output(TXT("[DOC] place"));
#endif					
					_game->perform_sync_world_job(TXT("place"), [this, object]()
					{
						if (place_function)
						{
							if (place_function(object))
							{
#ifdef OUTPUT_GENERATION
								output(TXT("[DOC] placed object (via place_function) in \"%S\" at %S"), object->get_presence()->get_in_room()? object->get_presence()->get_in_room()->get_name().to_char() : TXT("--"),
									object->get_presence()->get_placement().get_translation().to_string().to_char());
								if (auto* aimo = object->get_presence()->get_attached_to())
								{
									output(TXT("[DOC] attached to o%p"), aimo);
								}
#endif					
							}
							else
							{
#ifdef OUTPUT_GENERATION
								output(TXT("[DOC] couldn't place using place function, destroy"));
#endif					
								object->destroy();
							}
						}
						else
						{
							if (inRoom.get())
							{
								object->get_presence()->place_in_room(inRoom.get(), placement);
#ifdef OUTPUT_GENERATION
								output(TXT("[DOC] placed object (default) in \"%S\" at %S"), object->get_presence()->get_in_room() ? object->get_presence()->get_in_room()->get_name().to_char() : TXT("--"),
									object->get_presence()->get_placement().get_translation().to_string().to_char());
#endif					
							}
							else
							{
#ifdef OUTPUT_GENERATION
								output(TXT("[DOC] no room to place, destroy"));
#endif					
								object->destroy();
							}
						}
					});
					
					if (!object->is_destroyed())
					{
						_game->on_newly_created_object(object);

						if (processPOIsImmediately)
						{
							for_every_ptr(a, object->get_appearances())
							{
								a->process_mesh_pois();
							}
						}
					}

					if (inRoom.get() && !activateImmediatelyEvenIfRoomVisible)
					{
						saag.if_required_delay_until_room_not_visible(inRoom.get());
					}

					// auto activate (if was destroyed, will be skipped)
				}

#ifdef OUTPUT_GENERATION
				if (!_immediate)
				{
					output(TXT("delayed creation done in %.2fs"), ts.get_time_since());
				}
#endif
			}
			if (doorType)
			{
				if (ok && inRoom.is_set() &&
					(roomType ||
					(doorType->get_operation() == DoorOperation::StayClosed || (doorOperation.is_set() && doorOperation.get() == DoorOperation::StayClosed) || doorType->can_be_closed()) ||
					 connectToPOIInSameRoom.is_valid() || !connectToPOITaggedInSameRoom.is_empty()))
				{
					scoped_call_stack_info(TXT("door"));

	#ifdef OUTPUT_GENERATION
					::System::TimeStamp ts;
					if (!_immediate)
					{
						output(TXT("delayed creation of \"%S\" through \"%S\" in \"%S\""), roomType? roomType->get_name().to_string().to_char() : TXT("[not needed]"), doorType->get_name().to_string().to_char(), inRoom->get_name().to_char());
					}
	#endif

					an_assert(inSubWorld == nullptr, TXT("should not be used for room/door"));

					bool shouldCreate = true;
					if (checkSpaceBlockers && inRoom.get())
					{
						if (doorType)
						{
							SpaceBlockers spaceBlockers;
							doorType->create_space_blocker(OUT_ spaceBlockers);
							shouldCreate = inRoom->check_space_blockers(spaceBlockers, placement);
	#ifdef OUTPUT_GENERATION
							if (!shouldCreate)
							{
								output(TXT("skip creation due to space blockers"));
							}
	#endif					
						}
					}
					if (shouldCreate)
					{
						ScopedAutoActivationGroup saag(inRoom->get_in_world());

						Framework::Room* newRoom = nullptr;
						if (roomType)
						{
							_game->perform_sync_world_job(TXT("create room beyond door"), [this, &newRoom]()
							{
								static int beyondDoorRoom = 0;
								newRoom = new Framework::Room(inRoom->get_in_sub_world(), inRoom->get_in_region(), roomType, randomGenerator.spawn());
								ROOM_HISTORY(newRoom, TXT("beyond door [doc]"));
								newRoom->set_name(String::printf(TXT("beyond door room %i \"%S\""), beyondDoorRoom, roomType->get_name().to_string().to_char()));
							});
						}
						if (newRoom || (! roomType || connectToPOIInSameRoom.is_valid() ||!connectToPOITaggedInSameRoom.is_empty()))
						{
							if (newRoom)
							{
								newRoom->access_tags().set_tags_from(tagSpawned);

								if (post_create_room_function)
								{
									post_create_room_function(newRoom);
								}
							}

							Vector2 const * relativeDoorSize = variables.get_existing<Vector2>(NAME(relativeDoorSize));
							bool const * forceVRPlacement = variables.get_existing<bool>(NAME(forceVRPlacement));

							Framework::Door* door = nullptr;
							Framework::DoorInRoom* dirA = nullptr;
							Framework::DoorInRoom* dirB = nullptr;
							_game->perform_sync_world_job(TXT("create door"), [this, &door, &dirA, &dirB, newRoom, relativeDoorSize]()
							{
								door = new Framework::Door(inRoom->get_in_sub_world(), doorType);
								if (replacableByDoorTypeReplacer.is_set())
								{
									door->set_replacable_by_door_type_replacer(replacableByDoorTypeReplacer.get());
								}
								if (doorOperation.is_set())
								{
									door->set_initial_operation(doorOperation);
								}
								if (!newRoom &&
									! connectToPOIInSameRoom.is_valid() &&
									connectToPOITaggedInSameRoom.is_empty() &&
									door->can_be_closed())
								{
									door->set_initial_operation(DoorOperation::StayClosed);
								}
								if (relativeDoorSize)
								{
									door->set_relative_size(*relativeDoorSize);
								}
								dirA = new Framework::DoorInRoom(inRoom.get());
								DOOR_IN_ROOM_HISTORY(dirA, TXT("create door [doc]"));
								dirA->set_placement(placement);
								if (reverseSide)
								{
									door->link_b(dirA);
								}
								else
								{
									door->link(dirA);
								}
								if (newRoom)
								{
									dirB = new Framework::DoorInRoom(newRoom);
									DOOR_IN_ROOM_HISTORY(dirB, TXT("create door in new room [doc]"));
									dirB->set_placement(turn_matrix_xy_180(placement.to_matrix()).to_transform());
									door->link(dirB);
								}
								else if (connectToPOIInSameRoom.is_valid() ||
										 !connectToPOITaggedInSameRoom.is_empty())
								{
									dirB = new Framework::DoorInRoom(inRoom.get());
									DOOR_IN_ROOM_HISTORY(dirB, TXT("create door in the same room [doc]"));
									dirB->set_placement(turn_matrix_xy_180(placement.to_matrix()).to_transform());
									door->link(dirB);
								}
							});

#ifndef INVESTIGATE_SOUND_SYSTEM
	#ifndef BUILD_PUBLIC_RELEASE
							{	REMOVE_AS_SOON_AS_POSSIBLE_
								output(TXT("DOC door inRoom r%p"), inRoom.get());
								output(TXT("DOC door door d%p"), door);
								output(TXT("DOC door dirA dr'%p"), dirA);
								output(TXT("DOC door dirB dr'%p"), dirB);
							}
	#endif
#endif

							if (ignorableVRPlacement)
							{
								dirA->set_ignorable_vr_placement(ignorableVRPlacement);
								if (dirB)
								{
									dirB->set_ignorable_vr_placement(ignorableVRPlacement);
								}
							}

							if (dirB && skipHoleCheckOnMovingThroughForOtherDoor)
							{
								dirB->skip_hole_check_on_moving_through();
							}

							if (cantMoveThroughDoor)
							{
								door->mark_cant_move_through();
							}

							if (newRoom && beSourceForEnvironmentOverlaysRoomVars)
							{
								newRoom->set_room_with_vars_for_environment_overlays(inRoom.get());
							}

							if (GameStaticInfo::get()->is_sensitive_to_vr_anchor())
							{
								if (forceVRPlacement && *forceVRPlacement && inRoom->is_vr_arranged())
								{
									error(TXT("can't force as we're already vr arranged, maybe drop forceVRPlacement or do not delay?"));
								}
								else if (forceVRPlacement && *forceVRPlacement && !inRoom->is_vr_arranged())
								{
									DOOR_IN_ROOM_HISTORY(dirA, TXT("set vr placement [doc]"));
									dirA->set_vr_space_placement(dirA->get_placement());
								}
								else
								{
									// to set vr placement for dirA, as inRoom was already readier for vr, it shall propagate further
									inRoom->set_door_vr_placement_for(dirA, ignoreVRPlacementIfNotPresent? AllowToFail : DisallowToFail);
								}
							}

							if (groupId != NONE)
							{
								dirA->set_group_id(groupId);
							}

							dirA->init_meshes();
							if (dirB) dirB->init_meshes();

							if (newRoom && dirB)
							{
								newRoom->access_variables().access<Range2>(NAME(throughDoorSize)) = dirB->calculate_size();
							}

							if (inRoom->is_vr_arranged())
							{
								inRoom->ready_for_game();
							}
						
							if (auto* tagDoor = variables.get_existing<Tags>(NAME(tagDoor)))
							{
								if (!tagDoor->is_empty())
								{
									if (tagDoor->has_tag(NAME(vrElevatorAltitude)))
									{
										door->set_vr_elevator_altitude(tagDoor->get_tag(NAME(vrElevatorAltitude)));
									}
									dirA->access_tags().set_tags_from(*tagDoor);
									if (dirB)
									{
										dirB->access_tags().set_tags_from(*tagDoor);
									}
								}
							}
						
							if (auto* tagThisDoor = variables.get_existing<Tags>(NAME(tagThisDoor)))
							{
								if (!tagThisDoor->is_empty())
								{
									if (tagThisDoor->has_tag(NAME(vrElevatorAltitude)))
									{
										dirA->set_vr_elevator_altitude(tagThisDoor->get_tag(NAME(vrElevatorAltitude)));
									}
									dirA->access_tags().set_tags_from(*tagThisDoor);
								}
							}
						
							if (dirB)
							{
								if (auto* tagOtherDoor = variables.get_existing<Tags>(NAME(tagOtherDoor)))
								{
									if (!tagOtherDoor->is_empty())
									{
										if (tagOtherDoor->has_tag(NAME(vrElevatorAltitude)))
										{
											dirB->set_vr_elevator_altitude(tagOtherDoor->get_tag(NAME(vrElevatorAltitude)));
										}
										dirB->access_tags().set_tags_from(*tagOtherDoor);
									}
								}
							}
						
							if (newRoom)
							{
								if (auto* tagRoom = variables.get_existing<Tags>(NAME(tagRoom)))
								{
									if (!tagRoom->is_empty())
									{
										newRoom->access_tags().set_tags_from(*tagRoom);
									}
								}

								if (auto* markPOIsInside = variables.get_existing<Tags>(NAME(markPOIsInside)))
								{
									if (!markPOIsInside->is_empty())
									{
										newRoom->mark_POIs_inside(*markPOIsInside);
										newRoom->for_every_point_of_interest(NP, [markPOIsInside](PointOfInterestInstance * _poi)
										{
											_poi->access_tags().set_tags_from(*markPOIsInside);
										});
									}
								}
						
								if (auto* markRoomInside = variables.get_existing<Tags>(NAME(markRoomInside)))
								{
									if (!markRoomInside->is_empty())
									{
										newRoom->access_tags().set_tags_from(*markRoomInside);
									}
								}

								if (auto* markRoomInsideIncrement = variables.get_existing<Tags>(NAME(markRoomInsideIncrement)))
								{
									if (!markRoomInsideIncrement->is_empty())
									{
										markRoomInsideIncrement->do_for_every_tag([this, newRoom](Tag const& _tag)
											{
												int tagValue = inRoom->get_tags().get_tag_as_int(_tag.get_tag());
												int tagValueAlreadySet = newRoom->get_tags().get_tag_as_int(_tag.get_tag());
												newRoom->access_tags().set_tag(_tag.get_tag(), max(tagValue + 1, max(tagValueAlreadySet, _tag.get_relevance_as_int())));
											});
									}
								}

								if (placeDoorOnPOI.is_valid() && dirB)
								{
									newRoom->place_door_on_poi(dirB, placeDoorOnPOI);
								}
							}

							if (connectToPOIInSameRoom.is_valid() && dirB)
							{
								inRoom->place_door_on_poi(dirB, connectToPOIInSameRoom);
							}

							if (! connectToPOITaggedInSameRoom.is_empty() && dirB)
							{
								inRoom->place_door_on_poi_tagged(dirB, connectToPOITaggedInSameRoom);
							}

							{
								_game->on_newly_created_door_in_room(dirA);
								if (dirB)
								{
									_game->on_newly_created_door_in_room(dirB);
								}
							}
						}

						// auto activate or leave for activation
					}

	#ifdef OUTPUT_GENERATION
					if (!_immediate)
					{
						output(TXT("delayed creation done in %.2fs"), ts.get_time_since());
					}
	#endif
				}
				else
				{
#ifdef OUTPUT_GENERATION
					output(TXT("door type \"%S\" to spawn but can't"), doorType->get_name().to_string().to_char());
#endif
				}
			}
		}
		done = true;
		if (!_immediate)
		{
			scoped_call_stack_info(TXT("done delayed object creation"));
			_game->done_delayed_object_creation(this);
		}
#ifdef OUTPUT_GENERATION
		output(TXT("process delayed creation finished (%p)"), this);
#endif
	};
	if (_immediate)
	{
#ifdef OUTPUT_GENERATION
		output(TXT("process immediately (%p)"), this);
#endif
		do_process();
	}
	else
	{
#ifdef OUTPUT_GENERATION
		output(TXT("queue for later (%p)"), this);
#endif
		_game->add_async_world_job(TXT("delayed object creation"), do_process);
	}
}
