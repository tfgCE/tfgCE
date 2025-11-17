#include "pointOfInterestInstance.h"

#include "pointOfInterest.h"

#include "..\advance\advanceContext.h"
#include "..\appearance\mesh.h"
#include "..\game\delayedObjectCreation.h"
#include "..\game\game.h"
#include "..\game\gameConfig.h"
#include "..\library\library.h"
#include "..\module\moduleAI.h"
#include "..\module\moduleAppearance.h"
#include "..\module\modulePresence.h"
#include "..\object\actor.h"
#include "..\object\scenery.h"
#include "..\world\doorInRoom.h"
#include "..\world\room.h"
#include "..\world\roomRegionVariables.inl"
#include "..\world\roomUtils.h"
#include "..\world\subWorld.h"
#include "..\world\world.h"

#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\debug\debugVisualiser.h"
#include "..\..\core\random\randomUtils.h"

#include "..\..\core\system\input.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_OUTPUT_WORLD_GENERATION
#ifdef AN_ALLOW_EXTENSIVE_LOGS
//#define AN_INSPECT_POI_PLACEMENT
#endif
#endif

#ifdef AN_DEVELOPMENT
#ifdef AN_ALLOW_EXTENSIVE_LOGS
//#define DEBUG_POI_PLACEMENT
#endif
#endif

//

using namespace Framework;

//

#ifdef AN_OUTPUT_WORLD_GENERATION
#define OUTPUT_GENERATION
//#define OUTPUT_GENERATION_VARIABLES
#endif

//

#define MIN_POI_PLACEMENT_RADIUS 0.1f

/**
 *	name:function = spawn
 *		desc : will spawn actor when this point of interest is added
 *		params :
 *			float:chance = chance of spawning an actor
 *			bool:delayObjectCreation = delay object creation (can be set in xml for POI as <delayObjectCreation/>
 *			bool:forceDelayObjectCreation = will always delay, no matter if game wants differently, useful for doors
 *			int:delayObjectCreationPriority = priority
 *			bool:delayObjectCreationDevSkippable = devSkippable
 *			LibraryName:actorType = full library name of actor type to spawn
 *			LibraryName:itemType = full library name of item type to spawn
 *			LibraryName:sceneryType = full library name of scenery type to spawn
 *			LibraryName:temporaryObjectType = full library name of temporary object type to spawn
 *			LibraryName:useMes = full library name of mesh to override the default one
 *			LibraryName:useMeshGenerator = full library name of mesh generator to override the default one
 *			Tags:tagSpawned = spawns tagged object
 *			Name:spawnSet = spawn set (this is TEA FOR GOD EMPEROR specific, through game customisation!)
 *			bool:ignoreSpaceBlockers = ignore space blockers that would disallow placement here
 *			bool:activateImmediatelyEvenIfRoomVisible = will activate immediately, even if room is visible (by default waits for the room to not be seen
 *			bool:processPOIsImmediately = process object's POIs immediately
 *			bool:baseOnOwner = base on owner (and lock!) (via presence) (may use attachmentRelativePlacement)
 *			bool:attachToOwner = attach to owner (via presence)
 *			Name:attachToOwnersBone = attach to owner's bone (via presence) (does not require attachToOwner to be set)
 *			Transform:attachmentRelativePlacement = optional relative to bone placement
 *			bool:attachmentPlacementAsIs = optional attach exactly where is, only for temporary objects
 *			bool:attachToFollowing = following for attachment, if not provided, default is true
 *			Name:withinMeshNodeBoundary = will be placed within boundary defined by mesh nodes that use this name, uses <poiPlacementRadius> of objects to know where to place it
 *			Vector3:meshNodeBoundaryUp = optional up
 *			bool:atBoundaryEdge = move at the edge of boundary, not just randomly inside (might be overriden by object's <poiAtBoundaryEdge>)
 *			Range:addYawRotation = when trying to figure out initial rotation, will add some yaw randomly
 *			Range:stickToWallAt = if present, will stick to the wall at a specific altitude from this range - works well with withinMeshNodeBoundary (and offset it by <poiPlacementRadius>), for non mesh node boundary, use FindPlacementParams to add extra back distance?
 *			bool:stickToWallAtAngle = allow sticking to a wall at angle (non upright)
 *			bool:stickToCeiling = if present, will stick to the ceiling, this can be also set in the object <poiStickToCeiling>
 *			Vector2:relativeDoorSize = if available, is set to newly created door
 *			bool:forceVRPlacement = it should be used only for rooms that we may not have proper vr placement available (stations that assume vr placement etc)
 *		object params:
 *			float:poiPlacementRadius
 *			bool:poiAtBoundaryEdge
 *			bool:poiForceStickToWall - ignore being at boundary edge
 *			Range:poiStickToWallAt
 *			bool:poiStickToWallAtAngle
 *			bool:poiClampToMeshNodes = if true, the result will be clamped to mesh nodes figure
 *			bool:poiStickToCeiling
 *			bool:poiAtCeilingUpsideDown
 *			Range:poiAddYawRotation
 *			float:poiRoundYawRotation = rounds yaw if not placed on wall
 *			float:poiNormaliseWallHeight = if our wall/room height is lower than that, we adjust (this and below have to be both defined)
 *			Name:poiNormaliseWallRoomHeightVariable = used to read "room height" value to read from room so we can adjust accordingly
 *
 *	name:function = play sound
 *		desc : play sound sample
 *		params :
 *			float:chance = chance of playing a sample
 *			LibraryName:sample = sample to be played
 *
 *	name:function = door
 *		desc: create door and room (or region) beyond it
 *		params:
 *			float:chance = chance of spawning an actor
 *			bool:delayObjectCreation = delay object creation (can be set in xml for POI as <delayObjectCreation/>
 *			int:delayObjectCreationPriority = priority
 *			LibraryName:doorType = full library name of door type to create
 *			LibraryName:roomType = full library name of room type to create beyond door
 *			WorldSettingCondition:doorType = world setting condition to get door type
 *			WorldSettingCondition:roomType = world setting condition to get room type beyond door
 *			Name:connectToPOIInSameRoom = instead of creating a new room it will create just a second door in room and will place it on another POI
 *			TagCondition:connectToPOITaggedInSameRoom = as above but will look for tagged poi
 *			Name:placeDoorOnPOI = place door on POI in newly created room
 *			bool:ignoreSpaceBlockers = ignore space blockers that would disallow placement here
 *			bool:ignorableVRPlacement = may ignore vr placement (set to door, not just during creation)
 *			bool:ignoreVRPlacementIfNotPresent = ignore vr placement if not present
 *			bool:replacableByDoorTypeReplacer = can't be replaced by door type replacer, has to keep the type as is
 *			bool:reverseSide = reverse linked A/B
 *			int:groupId = set group id for navigation purposes
 *			Tags:markPOIsInside = will mark all POIs inside (generated with room, delayed will be skipped) with these tags
 *			Tags:tagDoor = tags both doors
 *			Tags:tagThisDoor = tags door on this side
 *			Tags:tagOtherDoor = tags door on other side
 *			Tags:tagRoom = tags (newly created) room
 *			bool:skipHoleCheckOnMovingThroughForOtherDoor = will mark skipHoleCheckOnMovingThrough for the door in the new room
 *			bool:cantMoveThroughDoor = will set canMoveThrough to false for both door in rooms
 *			bool:beSourceForEnvironmentOverlaysRoomVars = this room will be source for room vars for environment overlays for newly created room - useful to alter fog outside in vista sceneries
 *			Name:doorOperation
 *		note:
 *			Door will be placed in same vr place (note: this seems to be true only if the other room has the same vr anchor placement) in new room,
 *			it is strongly advised to use either placeDoorOnPOI of room generator (!) or placeDoorOnPOI of this POI
 *			unless we use connectToPOIInSameRoom/connectToPOITaggedInSameRoom, then there will be two DoorInRoom created in the same room
 *
 *	name:function = fuse mesh
 *		desc : generate and fuse mesh
 *		params :
 *			LibraryName:meshGenerator = mesh generator to generate mesh and fuse
 */

//
DEFINE_STATIC_NAME(function);
DEFINE_STATIC_NAME(functionInterval);
DEFINE_STATIC_NAME_STR(functionPlaySound, TXT("play sound"));
DEFINE_STATIC_NAME_STR(functionSpawn, TXT("spawn"));
DEFINE_STATIC_NAME_STR(functionDoor, TXT("door"));
DEFINE_STATIC_NAME_STR(functionFuseMesh, TXT("fuse mesh"));
//
DEFINE_STATIC_NAME(delayObjectCreation);
DEFINE_STATIC_NAME(forceDelayObjectCreation);
DEFINE_STATIC_NAME(delayObjectCreationPriority);
DEFINE_STATIC_NAME(delayObjectCreationDevSkippable);
DEFINE_STATIC_NAME(actorType);
DEFINE_STATIC_NAME(itemType);
DEFINE_STATIC_NAME(sceneryType);
DEFINE_STATIC_NAME(temporaryObjectType);
DEFINE_STATIC_NAME(useMesh);
DEFINE_STATIC_NAME(useMeshGenerator);
DEFINE_STATIC_NAME(spawnSet);
DEFINE_STATIC_NAME(tagSpawned);
DEFINE_STATIC_NAME(chance);
DEFINE_STATIC_NAME(sample);
DEFINE_STATIC_NAME(playFor);
DEFINE_STATIC_NAME(ignoreSpaceBlockers);
DEFINE_STATIC_NAME(ignorableVRPlacement);
DEFINE_STATIC_NAME(ignoreVRPlacementIfNotPresent);
DEFINE_STATIC_NAME(replacableByDoorTypeReplacer);
DEFINE_STATIC_NAME(reverseSide);
DEFINE_STATIC_NAME(groupId);
DEFINE_STATIC_NAME(skipHoleCheckOnMovingThroughForOtherDoor);
DEFINE_STATIC_NAME(cantMoveThroughDoor);
DEFINE_STATIC_NAME(beSourceForEnvironmentOverlaysRoomVars);
DEFINE_STATIC_NAME(activateImmediatelyEvenIfRoomVisible);
DEFINE_STATIC_NAME(processPOIsImmediately);
DEFINE_STATIC_NAME(baseOnOwner);
DEFINE_STATIC_NAME(attachToOwner);
DEFINE_STATIC_NAME(attachToOwnersBone);
DEFINE_STATIC_NAME(attachmentRelativePlacement);
DEFINE_STATIC_NAME(attachToFollowing);
DEFINE_STATIC_NAME(attachmentPlacementAsIs);
DEFINE_STATIC_NAME(withinMeshNodeBoundary);
DEFINE_STATIC_NAME(meshNodeBoundaryUp);
DEFINE_STATIC_NAME(atBoundaryEdge);
DEFINE_STATIC_NAME(addYawRotation);
DEFINE_STATIC_NAME(stickToWallAt);
DEFINE_STATIC_NAME(stickToWallAtAngle);
DEFINE_STATIC_NAME(stickToCeiling);
DEFINE_STATIC_NAME(volume);
DEFINE_STATIC_NAME(pitch);
DEFINE_STATIC_NAME(doorType);
DEFINE_STATIC_NAME(roomType);
DEFINE_STATIC_NAME(placeDoorOnPOI);
DEFINE_STATIC_NAME(doorOperation);
DEFINE_STATIC_NAME(connectToPOIInSameRoom);
DEFINE_STATIC_NAME(connectToPOITaggedInSameRoom);
DEFINE_STATIC_NAME(meshGenerator);
DEFINE_STATIC_NAME(relativeDoorSize);
DEFINE_STATIC_NAME(forceVRPlacement);
//
DEFINE_STATIC_NAME(poiPlacementRadius);
DEFINE_STATIC_NAME(poiStickToWallAt);
DEFINE_STATIC_NAME(poiStickToWallAtAngle);
DEFINE_STATIC_NAME(poiClampToMeshNodes);
DEFINE_STATIC_NAME(poiStickToCeiling);
DEFINE_STATIC_NAME(poiAtCeilingUpsideDown);
DEFINE_STATIC_NAME(poiAtBoundaryEdge);
DEFINE_STATIC_NAME(poiForceStickToWall);
DEFINE_STATIC_NAME(poiAddYawRotation);
DEFINE_STATIC_NAME(poiRoundYawRotation);
DEFINE_STATIC_NAME(poiNormaliseWallHeight);
DEFINE_STATIC_NAME(poiNormaliseWallRoomHeightVariable);

// room type variables
DEFINE_STATIC_NAME(avoidRoomTypeNeighboursTagged);

//

PointOfInterestInstance::PointOfInterestInstance()
: SafeObject<PointOfInterestInstance>(nullptr)
{
}

PointOfInterestInstance::~PointOfInterestInstance()
{
	make_safe_object_unavailable();
	if (playedSound.is_set())
	{
		playedSound.get()->stop();
	}
}

Transform PointOfInterestInstance::calculate_placement() const
{
	int frameId = ::System::Core::get_frame();
	if (placementFrameId == frameId)
	{
		return placement;
	}
	
	Concurrency::ScopedSpinLock lock(placementLock);

	placement = poi->offset;
	if (forMesh && forMeshInstance)
	{
		if (socketIdx != NONE)
		{
			placement = forMesh->calculate_socket_using_ms(socketIdx, forMeshInstance->get_pose_MS()).to_world(placement);
		}
		else
		{
			placement = forMeshInstance->get_placement().to_world(placement);
		}
	}
	if (owner.is_set())
	{
		placement = owner->get_presence()->get_placement().to_world(placement);
	}

	if (! owner.is_set() && ! forMeshInstance)
	{
		placement = originalPlacement.to_world(placement);
	}

	placementFrameId = frameId;
	return placement;
}

void PointOfInterestInstance::update_placement()
{
	// I've must have been seriously ill or drunk when I was adding placement without offset and stuff
	calculate_placement();
}

void PointOfInterestInstance::create_instance(Random::Generator & _rg, PointOfInterest * _poi, Room* _inRoom, IModulesOwner* _owner, int _poiIdx, Transform const & _placementOfOwner, Mesh const * _forMesh, Meshes::Mesh3DInstance* _forMeshInstance, Optional<bool> const & _process)
{
	make_safe_object_available(this);

	// allow creating instance even if we're canceling creation, otherwise further tasks may crash (or would require extra checks just because we didn't setup poi properly)
	an_assert(_poi != nullptr);
	poi = _poi;
	poiIdx = _poiIdx;
	name = _poi->name;
	tags = _poi->tags;
	inRoom = _inRoom;
	owner = _owner;
	socketIdx = NONE;
	originalPlacement = _placementOfOwner;
	placement = _placementOfOwner.to_world(poi->offset);
	if (Game::get()->does_want_to_cancel_creation())
	{
		// this is enough
		return;
	}
	forMesh = _forMesh;
	forMeshInstance = _forMeshInstance;
	if (poi->socketName.is_valid() && forMesh)
	{
		socketIdx = forMesh->find_socket_index(poi->socketName);
	}
	parameters = poi->parameters;
	// copy from owner if possible
	for_every(param, poi->parametersFromOwner.get_all())
	{
		if (auto* existing = _owner->get_variables().find_existing(param->get_name(), param->type_id()))
		{
			auto & into = parameters.find(param->get_name(), param->type_id());
			RegisteredType::copy(param->type_id(), into.access_raw(), existing->get_raw());
		}
	}
	
	randomGenerator = _rg;

	functionIntervalTimeLeft.clear();
	if (_process.get(true))
	{
		process();
	}
}

void PointOfInterestInstance::advance(Framework::AdvanceContext const & _advanceContext)
{
	if (!processingInProgress && functionIntervalTimeLeft.is_set())
	{
		functionIntervalTimeLeft = functionIntervalTimeLeft.get() - _advanceContext.get_delta_time();
		if (functionIntervalTimeLeft.get() <= 0.0f)
		{
			update_placement();
			process();
		}
	}
}

void PointOfInterestInstance::process()
{
	if (Game::get()->does_want_to_cancel_creation() || cancelled)
	{
		// skip this
		return;
	}

	Range const& functionIntervalRange = parameters.get_value<Range>(NAME(functionInterval), Range::zero);
	float functionIntervalFloat = parameters.get_value<float>(NAME(functionInterval), 0.0f);

	bool initialRun = true;
	if (!functionIntervalRange.is_zero() ||
		functionIntervalFloat != 0.0f)
	{
		initialRun = !functionIntervalTimeLeft.is_set();
		functionIntervalTimeLeft = !functionIntervalRange.is_zero() ? randomGenerator.get(functionIntervalRange) : functionIntervalFloat;
		if (functionIntervalTimeLeft.get() <= 0.0f)
		{
			functionIntervalTimeLeft.clear();
		}
		if (initialRun)
		{
			return;
		}
	}

	todo_note(TXT("TN#8 move this somewhere else?"));
	// check for built in functions
	Name const& function = parameters.get_value<Name>(NAME(function), Name::invalid());
	float chance = 1.0f;
	if (float const* ch = parameters.get_existing<float>(NAME(chance)))
	{
		chance = *ch;
	}

	if (chance >= 1.0f || randomGenerator.get_chance(chance))
	{
		process_function(function);
	}
}

void PointOfInterestInstance::process_function(Name const & function)
{
#ifdef AN_DEVELOPMENT
	if (get_poi()->debugThis)
	{
		AN_BREAK;
	}
#endif

	::SafePtr<Room> useRoom(get_room());
	{
		if (function == NAME(functionSpawn) ||
			function == NAME(functionDoor))
		{
			{
				processingInProgress = true;
				std::function<void()> spawn = nullptr;
				if (function == NAME(functionSpawn))
				{
					::SafePtr<PointOfInterestInstance> safeThis(this);
					spawn = [this, safeThis, useRoom]()
					{
						if (!safeThis.is_set()) // should catch if owner was deleted
						{
							return;
						}
						MEASURE_PERFORMANCE(spawn);
						{
							ObjectType* objectType = nullptr;
							TemporaryObjectType* temporaryObjectType = nullptr;
							Mesh* useMesh = nullptr;
							MeshGenerator* useMeshGenerator = nullptr;
							{
								LibraryName const* actorTypeName = parameters.get_existing<LibraryName>(NAME(actorType));
								LibraryName const* itemTypeName = parameters.get_existing<LibraryName>(NAME(itemType));
								LibraryName const* sceneryTypeName = parameters.get_existing<LibraryName>(NAME(sceneryType));
								LibraryName const* temporaryObjectTypeName = parameters.get_existing<LibraryName>(NAME(temporaryObjectType));
								objectType = actorTypeName ? Library::get_current()->get_actor_types().find(*actorTypeName) : objectType;
								objectType = itemTypeName ? Library::get_current()->get_item_types().find(*itemTypeName) : objectType;
								objectType = sceneryTypeName ? Library::get_current()->get_scenery_types().find(*sceneryTypeName) : objectType;
								temporaryObjectType = temporaryObjectTypeName ? Library::get_current()->get_temporary_object_types().find(*temporaryObjectTypeName) : temporaryObjectType;
							}
							{
								LibraryName const* useMeshName = parameters.get_existing<LibraryName>(NAME(useMesh));
								useMesh = useMeshName? Library::get_current()->get_meshes().find(*useMeshName) : useMesh;
							}
							{
								LibraryName const* useMeshGeneratorName = parameters.get_existing<LibraryName>(NAME(useMeshGenerator));
								useMeshGenerator = useMeshGeneratorName? Library::get_current()->get_mesh_generators().find(*useMeshGeneratorName) : useMeshGenerator;
							}
							if (objectType) { objectType->load_on_demand_if_required(); }
							if (useMesh) { useMesh->load_on_demand_if_required(); }
							if (useMeshGenerator) { useMeshGenerator->load_on_demand_if_required(); }
							setup_function_spawn(OUT_ objectType, OUT_ temporaryObjectType);
							if (objectType && (owner.get() || useRoom.get()))
							{
								std::function<bool(Object* _object)> place =
									[this, useRoom](Object* spawnedObject)
								{
									bool const * baseOnOwnerPtr = parameters.get_existing<bool>(NAME(baseOnOwner));
									bool const * attachToOwnerPtr = parameters.get_existing<bool>(NAME(attachToOwner));
									Name const * attachToOwnersBonePtr = parameters.get_existing<Name>(NAME(attachToOwnersBone));
									bool const * attachToFollowingPtr = parameters.get_existing<bool>(NAME(attachToFollowing));
									Transform const * attachmentRelativePlacementPtr = parameters.get_existing<Transform>(NAME(attachmentRelativePlacement));
									bool result = true;
									if (attachToOwnersBonePtr && attachToOwnersBonePtr->is_valid() && owner.is_set() && owner->get_presence())
									{
										Name attachToOwnersBone = *attachToOwnersBonePtr;
										bool attachToFollowing = attachToFollowingPtr ? *attachToFollowingPtr : true;
										Game::get()->perform_sync_world_job(TXT("attach object"), [this, spawnedObject, attachToOwnersBone, attachToFollowing, attachmentRelativePlacementPtr, useRoom]()
										{
											if (!useRoom.is_set() && !attachmentRelativePlacementPtr)
											{
												warn(TXT("no attachmentRelativePlacement nor useRoom provided, assuming attaching directly to bone"));
												spawnedObject->get_presence()->attach_to_bone(owner.get(), attachToOwnersBone, attachToFollowing, Transform::identity);
											}
											else if (owner.is_set())
											{
												if (attachmentRelativePlacementPtr)
												{
													spawnedObject->get_presence()->attach_to_bone(owner.get(), attachToOwnersBone, attachToFollowing, *attachmentRelativePlacementPtr);
												}
												else
												{
													an_assert(useRoom.get() == owner->get_presence()->get_in_room());
													spawnedObject->get_presence()->attach_to_bone_using_in_room_placement(owner.get(), attachToOwnersBone, attachToFollowing, placement);
												}
											}
										});
									}
									else if (attachToOwnerPtr && *attachToOwnerPtr && owner.is_set() && owner->get_presence())
									{
										bool attachToFollowing = attachToFollowingPtr ? *attachToFollowingPtr : true;
										Game::get()->perform_sync_world_job(TXT("attach object"), [this, spawnedObject, attachToFollowing, attachmentRelativePlacementPtr, useRoom]()
										{
											if (!useRoom.is_set())
											{
												warn(TXT("no attachmentRelativePlacement nor useRoom provided, assuming attaching directly to object"));
												spawnedObject->get_presence()->attach_to(owner.get(), attachToFollowing, Transform::identity);
											}
											else if (owner.is_set())
											{
												an_assert(useRoom.get() == owner->get_presence()->get_in_room());
												if (attachmentRelativePlacementPtr)
												{
													spawnedObject->get_presence()->attach_to(owner.get(), attachToFollowing, *attachmentRelativePlacementPtr);
												}
												else
												{
													spawnedObject->get_presence()->attach_to_using_in_room_placement(owner.get(), attachToFollowing, placement);
												}
											}
										});
									}
									else if (baseOnOwnerPtr && *baseOnOwnerPtr && owner.is_set() && owner->get_presence())
									{
										Game::get()->perform_sync_world_job(TXT("base object"), [this, spawnedObject, attachmentRelativePlacementPtr, useRoom]()
										{
											if (!useRoom.is_set())
											{
												warn(TXT("no attachmentRelativePlacement nor useRoom provided, assuming placing directly at object"));
												spawnedObject->get_presence()->place_in_room(useRoom.get(), owner->get_presence()->get_placement());
											}
											else if (owner.is_set())
											{
												an_assert(useRoom.get() == owner->get_presence()->get_in_room());
												if (attachmentRelativePlacementPtr)
												{
													spawnedObject->get_presence()->place_in_room(useRoom.get(), owner->get_presence()->get_placement().to_world(*attachmentRelativePlacementPtr));
												}
												else
												{
													spawnedObject->get_presence()->place_in_room(useRoom.get(), placement);
												}
												spawnedObject->get_presence()->force_base_on(owner.get());
											}
										});
									}
									else
									{
										Optional<Transform> goodPlacement = find_placement_for(spawnedObject, useRoom.get(), randomGenerator.spawn());

										if (!goodPlacement.is_set())
										{
											// won't be placed at all
											warn(TXT("no good placement found, not placing POI"));
											return false;
										}

										Transform usePlacement = goodPlacement.get();

										Game::get()->perform_sync_world_job(TXT("place object"), [spawnedObject, useRoom, &result, usePlacement]()
										{
											if (!useRoom.is_set())
											{
												warn(TXT("no room provided or does not longer exist, not placing POI"));
												result = false;
											}
											else
											{
												spawnedObject->get_presence()->place_in_room(useRoom.get(), usePlacement);
											}
										});
									}

									return result;
								};

								String objectName = String::printf(TXT("%S > %S %04i"), poi->name.to_string().to_char(), objectType->get_name().to_string().to_char(), Random::get_int(1000));

								Random::Generator objectRG = randomGenerator.spawn();
								objectRG.advance_seed(1297, 237579); // to get same result

#ifdef OUTPUT_GENERATION
								::System::TimeStamp ts;
								output(TXT("spawning \"%S\" named \"%S\" on poi..."), objectType->get_name().to_string().to_char(), objectName.to_char());
#endif

								{
									RefCountObjectPtr<DelayedObjectCreation> doc(new DelayedObjectCreation());
									if (!doc->inSubWorld && useRoom.get())
									{
										doc->inSubWorld = useRoom->get_in_sub_world();
									}
									if (!doc->inSubWorld && owner.get())
									{
										doc->inSubWorld = owner->get_in_sub_world();
									}
									doc->imoOwner = owner;
									doc->inRoom = useRoom.get();
									doc->name = objectName;
									doc->objectType = objectType;
									doc->tagSpawned = parameters.get_value<Tags>(NAME(tagSpawned), Tags());
									doc->placement = Transform::identity;
									doc->randomGenerator = objectRG;
									doc->activateImmediatelyEvenIfRoomVisible = parameters.get_value<bool>(NAME(activateImmediatelyEvenIfRoomVisible), doc->activateImmediatelyEvenIfRoomVisible);
									doc->processPOIsImmediately = parameters.get_value<bool>(NAME(processPOIsImmediately), doc->processPOIsImmediately);
									doc->checkSpaceBlockers = !parameters.get_value<bool>(NAME(ignoreSpaceBlockers), false);
									{
										// check if we want to place it on our own
										// if so, we won't be using space blockers here
										bool const* baseOnOwnerPtr = parameters.get_existing<bool>(NAME(baseOnOwner));
										bool const* attachToOwnerPtr = parameters.get_existing<bool>(NAME(attachToOwner));
										Name const* attachToOwnersBonePtr = parameters.get_existing<Name>(NAME(attachToOwnersBone));
										Name const* withinMeshNodeBoundaryPtr = parameters.get_existing<Name>(NAME(withinMeshNodeBoundary));
										if ((attachToOwnersBonePtr && attachToOwnersBonePtr->is_valid() && owner.is_set() && owner->get_presence())
										 || (attachToOwnerPtr && *attachToOwnerPtr && owner.is_set() && owner->get_presence())
										 || (baseOnOwnerPtr && *baseOnOwnerPtr && owner.is_set() && owner->get_presence()))
										{
											// will be attached or based on, no need to check space blockers
											doc->checkSpaceBlockers = false;
										}
										if (withinMeshNodeBoundaryPtr && withinMeshNodeBoundaryPtr->is_valid()) // if using within mesh node boundary, we do check in place function
										{
											doc->checkSpaceBlockers = false;
										}
									}
									if (doc->checkSpaceBlockers)
									{
										// we should provide a valid placement!
										// and we will rely on DOC on placing the object
										doc->placement = placement;
									}
									else
									{
										doc->place_function = place;
									}

									if (useRoom.get()) // for items created inside we don't have a room to use
									{
										useRoom->collect_variables(doc->variables);
									}
									doc->variables.set_from(parameters);
#ifdef OUTPUT_GENERATION_VARIABLES
									{
										LogInfoContext log;
										log.log(TXT("POI spawn with variables"));
										{
											LOG_INDENT(log);
											doc->variables.log(log);
										}
										log.output_to_output();
									}
#endif
									doc->pre_initialise_modules_function = [this](Object* _object) {_object->learn_from(parameters); };
									doc->pre_setup_module_function = [useMeshGenerator, useMesh](Framework::Module* _module)
									{
										if (useMesh)
										{
											if (auto* moduleAppearance = fast_cast<Framework::ModuleAppearance>(_module))
											{
												moduleAppearance->use_mesh(useMesh);
											}
										}
										else if (useMeshGenerator)
										{
											if (auto* moduleAppearance = fast_cast<Framework::ModuleAppearance>(_module))
											{
												moduleAppearance->use_mesh_generator_on_setup(useMeshGenerator);
											}
										}
									};

									doc->priority = parameters.get_value<int>(NAME(delayObjectCreationPriority), doc->priority);
									doc->devSkippable = parameters.get_value<bool>(NAME(delayObjectCreationDevSkippable), doc->devSkippable);

									if (Game::get()->should_process_object_creation(doc.get()))
									{
										if ((!Game::get()->is_forced_instant_object_creation() &&
											 parameters.get_value<bool>(NAME(delayObjectCreation), false)) ||
											parameters.get_value<bool>(NAME(forceDelayObjectCreation), false))
										{
#ifdef OUTPUT_GENERATION
											output(TXT("delay spawning of \"%S\""), objectType->get_name().to_string().to_char());
#endif
											Game::get()->queue_delayed_object_creation(doc.get(), true);
										}
										else
										{
#ifdef OUTPUT_GENERATION
											output(TXT("instant spawning of \"%S\""), objectType->get_name().to_string().to_char());
#endif
											doc->process(Game::get(), true);
										}
									}
								}
#ifdef OUTPUT_GENERATION
								output(TXT("done in %.2fs"), ts.get_time_since());
#endif
							}
							else if (temporaryObjectType)
							{
#ifdef OUTPUT_GENERATION
								::System::TimeStamp ts;
								output(TXT("spawning temporary object \"%S\" on poi \"%S\"..."), temporaryObjectType->get_name().to_string().to_char(), name.to_char());
								output(TXT(" use room r%p \"%S\""), useRoom.get(), useRoom.get()? useRoom->get_name().to_char(): TXT("--"));
#endif
								SubWorld* inSubWorld = nullptr;
								if (!inSubWorld && useRoom.get())
								{
									inSubWorld = useRoom->get_in_sub_world();
								}
								if (!inSubWorld && owner.get())
								{
									inSubWorld = owner->get_in_sub_world();
								}
								if (inSubWorld)
								{
									if (auto* temporaryObject = inSubWorld->get_one_for(temporaryObjectType, nullptr))
									{
										if (owner.get())
										{
											if (auto* ownerAsObject = fast_cast<Object>(owner.get()))
											{
												bool const * baseOnOwnerPtr = parameters.get_existing<bool>(NAME(baseOnOwner));
												bool const * attachToOwnerPtr = parameters.get_existing<bool>(NAME(attachToOwner));
												Name const * attachToOwnersBonePtr = parameters.get_existing<Name>(NAME(attachToOwnersBone));
												bool const * attachToFollowingPtr = parameters.get_existing<bool>(NAME(attachToFollowing));
												Transform const * attachmentRelativePlacementPtr = parameters.get_existing<Transform>(NAME(attachmentRelativePlacement));
												Transform attachmentPlacementAsIsRelativePlacement = parameters.get_value<bool>(NAME(attachmentPlacementAsIs), false) ? owner->get_presence()->get_placement().to_local(placement) : Transform::identity;

												if (attachToOwnersBonePtr && attachToOwnersBonePtr->is_valid())
												{
													Name attachToOwnersBone = *attachToOwnersBonePtr;
													bool attachToFollowing = attachToFollowingPtr ? *attachToFollowingPtr : true;
													Transform attachmentRelativePlacement = attachmentRelativePlacementPtr ? *attachmentRelativePlacementPtr : attachmentPlacementAsIsRelativePlacement;
													temporaryObject->on_activate_attach_to_bone(owner.get(), attachToOwnersBone, attachToFollowing, attachmentRelativePlacement);
#ifdef OUTPUT_GENERATION
													output(TXT("on activate attach to bone"));
#endif
												}
												else if (attachToOwnerPtr && *attachToOwnerPtr)
												{
													bool attachToFollowing = attachToFollowingPtr ? *attachToFollowingPtr : true;
													Transform attachmentRelativePlacement = attachmentRelativePlacementPtr ? *attachmentRelativePlacementPtr : attachmentPlacementAsIsRelativePlacement;
													temporaryObject->on_activate_attach_to(owner.get(), attachToFollowing, attachmentRelativePlacement);
#ifdef OUTPUT_GENERATION
													output(TXT("on activate attach to attach-to"));
#endif
												}
												else if (baseOnOwnerPtr && *baseOnOwnerPtr)
												{
													warn(TXT("base on owner not supported for temporary objects, using attachment"));
													bool attachToFollowing = attachToFollowingPtr ? *attachToFollowingPtr : true;
													Transform attachmentRelativePlacement = attachmentRelativePlacementPtr ? *attachmentRelativePlacementPtr : attachmentPlacementAsIsRelativePlacement;
													temporaryObject->on_activate_attach_to(owner.get(), attachToFollowing, attachmentRelativePlacement);
#ifdef OUTPUT_GENERATION
													output(TXT("on activate attach to base"));
#endif
												}
												else
												{
													temporaryObject->on_activate_attach_to(owner.get(), false, attachmentPlacementAsIsRelativePlacement);
#ifdef OUTPUT_GENERATION
													output(TXT("on activate just attach"));
#endif
												}

												ownerAsObject->mark_to_activate(temporaryObject);
											}
											else
											{
												an_assert(false, TXT("it should be an object!"));
											}
										}
										else if (useRoom.get())
										{
#ifdef OUTPUT_GENERATION
											output(TXT("on activate place at %S in %S"), placement.get_translation().to_string().to_char(), useRoom.get()->get_name().to_char());
#endif
											temporaryObject->on_activate_place_in(useRoom.get(), placement);
											useRoom->mark_to_activate(temporaryObject);
										}
										else
										{
											an_assert(false, TXT("where are we then?"));
										}
										temporaryObject->access_variables().set_from(parameters);
#ifdef OUTPUT_GENERATION
										output(TXT("marked to activate o%p"), temporaryObject);
#endif
									}
								}
								else
								{
									warn(TXT("no sub world provided, skip temporary object"));
								}
#ifdef OUTPUT_GENERATION
								output(TXT("done in %.2fs"), ts.get_time_since());
#endif
							}
							else
							{
#ifndef INVESTIGATE_SOUND_SYSTEM
								warn(TXT("nothing defined to spawn for \"spawn\" function for poi \"%S\" in room \"%S\""), poi->name.to_char(), useRoom.is_set() ? useRoom->get_name().to_char() : TXT("--"));
#endif
							}
						}
						processingInProgress = false;
						if (!functionIntervalTimeLeft.is_set())
						{
							cancel();
						}
					};
				}
				if (function == NAME(functionDoor))
				{
					::SafePtr<PointOfInterestInstance> safeThis(this);
					spawn = [this, safeThis, useRoom]()
					{
						if (!safeThis.is_set()) // should catch if owner was deleted
						{
							return;
						}
						MEASURE_PERFORMANCE(spawnDoor);
#ifdef OUTPUT_GENERATION
						output(TXT("spawn door and room? in %S"), useRoom.get()? useRoom.get()->get_name().to_char() : TXT("--"));
#endif
						LibraryName const* doorTypeName = parameters.get_existing<LibraryName>(NAME(doorType));
						DoorType* doorType = doorTypeName ? Library::get_current()->get_door_types().find(*doorTypeName) : nullptr;
						bool spawn = true;
						// early check
						spawn &= check_space_blockers(useRoom.get(), NP, [doorType](OUT_ SpaceBlockers& _spaceBlockers)
							{
								if (doorType)
								{
									doorType->create_space_blocker(OUT_ _spaceBlockers);
								}
							});
						if (spawn && useRoom.get())
						{
							LibraryName const * roomTypeName = parameters.get_existing<LibraryName>(NAME(roomType));
							Name const * placeDoorOnPOI = parameters.get_existing<Name>(NAME(placeDoorOnPOI));
							Name const * doorOperationPtr = parameters.get_existing<Name>(NAME(doorOperation));
							Optional<DoorOperation::Type> doorOperation;
							if (doorOperationPtr)
							{
								doorOperation = DoorOperation::parse(doorOperationPtr->to_string());
							}
							Name connectToPOIInSameRoom = parameters.get_value<Name>(NAME(connectToPOIInSameRoom), Name::invalid());
							TagCondition connectToPOITaggedInSameRoom = parameters.get_value<TagCondition>(NAME(connectToPOITaggedInSameRoom), TagCondition());
							RoomType * roomType = roomTypeName ? Library::get_current()->get_room_types().find(*roomTypeName) : nullptr;
							setup_function_door(useRoom.get(), OUT_ doorType, OUT_ roomType);
							if (doorType &&
								!roomType && doorType->get_operation() != DoorOperation::StayClosed && ! doorType->can_be_closed()
								&& !connectToPOIInSameRoom.is_valid() && connectToPOITaggedInSameRoom.is_empty())
							{
								error(TXT("can't use door \"%S\" that is not set to stay closed and has no room behind"), doorType->get_name().to_string().to_char());
							}
							if (doorType &&
								(roomType || 
								 (doorType->get_operation() == DoorOperation::StayClosed || doorType->can_be_closed()) ||
								 connectToPOIInSameRoom.is_valid() || ! connectToPOITaggedInSameRoom.is_empty()))
							{
#ifdef OUTPUT_GENERATION
								::System::TimeStamp ts;
								output(TXT("spawning room \"%S\" through door \"%S\"..."), roomType? roomType->get_name().to_string().to_char() : TXT("[not needed]"), doorType->get_name().to_string().to_char());
#endif

								{
									an_assert(useRoom.get());
									RefCountObjectPtr<DelayedObjectCreation> doc(new DelayedObjectCreation());
									doc->inRoom = useRoom.get();
									doc->roomType = roomType;
									doc->doorType = doorType;
									doc->placement = placement;
									doc->randomGenerator = randomGenerator.spawn();
									doc->placeDoorOnPOI = placeDoorOnPOI ? *placeDoorOnPOI : Name::invalid();
									doc->connectToPOIInSameRoom = connectToPOIInSameRoom;
									doc->connectToPOITaggedInSameRoom = connectToPOITaggedInSameRoom;
									doc->checkSpaceBlockers = !parameters.get_value<bool>(NAME(ignoreSpaceBlockers), false);
									doc->ignorableVRPlacement = parameters.get_value<bool>(NAME(ignorableVRPlacement), false);
									doc->ignoreVRPlacementIfNotPresent = parameters.get_value<bool>(NAME(ignoreVRPlacementIfNotPresent), false);
									if (auto* r = parameters.get_existing<bool>(NAME(replacableByDoorTypeReplacer)))
									{
										doc->replacableByDoorTypeReplacer = *r;
									}
									doc->reverseSide = parameters.get_value<bool>(NAME(reverseSide), false);
									doc->groupId = parameters.get_value<int>(NAME(groupId), NONE);
									doc->skipHoleCheckOnMovingThroughForOtherDoor = parameters.get_value<bool>(NAME(skipHoleCheckOnMovingThroughForOtherDoor), false);
									doc->cantMoveThroughDoor = parameters.get_value<bool>(NAME(cantMoveThroughDoor), false);
									doc->beSourceForEnvironmentOverlaysRoomVars = parameters.get_value<bool>(NAME(beSourceForEnvironmentOverlaysRoomVars), false);;
									if (!roomType && 
										!connectToPOIInSameRoom.is_valid() &&
										connectToPOITaggedInSameRoom.is_empty() && 
										doorType->can_be_closed())
									{
										doorOperation = DoorOperation::StayClosed;
									}
									if (doorOperation.is_set())
									{
										doc->doorOperation = doorOperation.get();
									}
									doc->variables.set_from(parameters);
									transferableParameters = parameters;
									// do not transfer it further these parameters:
									{
										transferableParameters.invalidate<LibraryName>(NAME(doorType));
										transferableParameters.invalidate<LibraryName>(NAME(roomType));
										transferableParameters.invalidate<Name>(NAME(placeDoorOnPOI));
										transferableParameters.invalidate<Name>(NAME(connectToPOIInSameRoom));
										transferableParameters.invalidate<TagCondition>(NAME(connectToPOITaggedInSameRoom));
										transferableParameters.invalidate<bool>(NAME(ignoreSpaceBlockers));
										// vr placement are fine to copy
										transferableParameters.invalidate<int>(NAME(groupId));
										transferableParameters.invalidate<bool>(NAME(skipHoleCheckOnMovingThroughForOtherDoor));
										transferableParameters.invalidate<bool>(NAME(cantMoveThroughDoor));
										transferableParameters.invalidate<bool>(NAME(beSourceForEnvironmentOverlaysRoomVars));
										// delayObjectCreationPriority fine to copy
									}
#ifdef OUTPUT_GENERATION_VARIABLES
									{
										LogInfoContext log;
										log.log(TXT("POI room with variables (DOC)"));
										{
											LOG_INDENT(log);
											doc->variables.log(log);
										}
										log.log(TXT("POI room with variables (for room)"));
										{
											LOG_INDENT(log);
											transferableParameters.log(log);
										}
										log.output_to_output();
									}
#endif

									doc->post_create_room_function = [this](Room* _room) {_room->access_variables().set_from(transferableParameters); };
									
									doc->priority = parameters.get_value<int>(NAME(delayObjectCreationPriority), doc->priority);

									if (Game::get()->should_process_object_creation(doc.get()))
									{
										if ((!Game::get()->is_forced_instant_object_creation() &&
											 parameters.get_value<bool>(NAME(delayObjectCreation), false)) ||
											parameters.get_value<bool>(NAME(forceDelayObjectCreation), false))
										{
#ifdef OUTPUT_GENERATION
											output(TXT("delay spawning of \"%S\" through \"%S\" (dc:%p)"), roomType? roomType->get_name().to_string().to_char() : TXT("[not needed]"), doorType->get_name().to_string().to_char(), doc.get());
#endif
											Game::get()->queue_delayed_object_creation(doc.get(), true);
										}
										else
										{
#ifdef OUTPUT_GENERATION
											output(TXT("instant spawning of \"%S\" through \"%S\""), roomType? roomType->get_name().to_string().to_char() : TXT("[not needed]"), doorType->get_name().to_string().to_char());
#endif
											doc->process(Game::get(), true);
										}
									}
									else
									{
#ifdef OUTPUT_GENERATION
										output(TXT("spawning ignored"));
#endif
									}
								}
#ifdef OUTPUT_GENERATION
								output(TXT("done in %.2fs"), ts.get_time_since());
#endif
							}
							else
							{
#ifdef OUTPUT_GENERATION
								if (!doorType)
								{
									output(TXT("no doorType provided for poi \"%S\""), poi->name.to_char());
								}
								if (!roomType)
								{
									output(TXT("no roomType provided for poi \"%S\""), poi->name.to_char());
								}
#endif
							}
						}
						else
						{
#ifdef OUTPUT_GENERATION
							if (!spawn)
							{
								output(TXT("blocked by space blocker"));
							}
							if (!useRoom.get())
							{
								output(TXT("no room?"));
							}
#endif
						}
						processingInProgress = false;
						if (!functionIntervalTimeLeft.is_set())
						{
							cancel();
						}
					};
				}
				an_assert(spawn);

				// check if we should do this async or not
				// if we're not active, we should do this immediately
				// if we're active, spawn async job
				bool doAsync = false;
				if (auto* o = fast_cast<Object>(owner.get()))
				{
					doAsync |= o->is_world_active();
				}
				if (!owner.get() && useRoom.get() && useRoom->is_world_active())
				{
					doAsync |= true;
				}
				if (doAsync)
				{
					Game::get()->add_async_world_job(TXT("async process"), spawn);
				}
				else
				{
					spawn();
				}
			}
		}
		if (function == NAME(functionPlaySound))
		{
			if (useRoom.get())
			{
				if (LibraryName const * sampleName = parameters.get_existing<LibraryName>(NAME(sample)))
				{
					float const * playForFloat = parameters.get_existing<float>(NAME(playFor));
					Range const * playForRange = parameters.get_existing<Range>(NAME(playFor));

					if (Sample * sample = Library::get_current()->get_samples().find(*sampleName))
					{
						sample->load_on_demand_if_required();
						if (playedSound.is_set())
						{
							playedSound->stop();
						}
						float volume = 1.0f;
						if (auto* vol = parameters.get_existing<float>(NAME(volume)))
						{
							volume = *vol;
						}
						else if (auto* vol = parameters.get_existing<Range>(NAME(volume)))
						{
							volume = randomGenerator.get(*vol);
						}
						float pitch = 1.0f;
						if (auto* pit = parameters.get_existing<float>(NAME(pitch)))
						{
							pitch = *pit;
						}
						else if (auto* pit = parameters.get_existing<Range>(NAME(pitch)))
						{
							pitch = randomGenerator.get(*pit);
						}

						playedSound = useRoom->access_sounds().play(sample, useRoom.get(), placement);
						playedSound->access_sample_instance().set_volume(volume);
						playedSound->access_sample_instance().set_pitch(pitch);
						if (playForFloat)
						{
							playedSound->set_auto_stop_time(*playForFloat);
						}
						else if (playForRange)
						{
							playedSound->set_auto_stop_time(randomGenerator.get(*playForRange));
						}
					}
					else
					{
						error(TXT("could not find sample \"%S\""), sampleName->to_string().to_char());
					}
				}
				else
				{
					warn(TXT("there's no \"sample\" parameter provided for \"play sound\" function for poi \"%S\""), poi->name.to_char());
				}
			}
			if (!functionIntervalTimeLeft.is_set())
			{
				cancel();
			}
		}
		if (function == NAME(functionFuseMesh))
		{
			LibraryName const * meshGeneratorName = parameters.get_existing<LibraryName>(NAME(meshGenerator));

			MeshGenerator* meshGenerator = nullptr;
			if (meshGeneratorName)
			{
				meshGenerator = Library::get_current()->get_mesh_generators().find(*meshGeneratorName);
			}

			if (meshGenerator)
			{
				if (auto* iOwner = owner.get())
				{
					if (auto* appearance = iOwner->get_appearance())
					{
						Framework::Mesh* mesh = appearance->access_mesh();
						if (!mesh->is_unique_instance())
						{
							// use copy if sharing mesh or not generated it
							auto* newMesh = mesh->create_temporary_hard_copy();
							newMesh->be_unique_instance();
							appearance->use_mesh(newMesh);
							newMesh = appearance->access_mesh();
						}
						appearance->generate_and_fuse(meshGenerator, &iOwner->get_variables());
					}
				}
				else
				{
					error(TXT("\"fuse mesh\" can be run now only for POIs owned by IModulesOwner"));
				}
			}
			else
			{
				error(TXT("no meshGenerator provided to fuse mesh"));
			}
		}
	}
}

IModulesOwner* PointOfInterestInstance::get_owner() const
{
	return owner.get();
}

Room* PointOfInterestInstance::get_room() const
{
	return owner.is_set() ? owner->get_presence()->get_in_room() : inRoom.get();
}

Nav::PlacementAtNode PointOfInterestInstance::get_nav_placement(bool _veryLazy)
{
	int frameId = ::System::Core::get_frame();
	if (navPlacementFrameId == frameId)
	{
		return navPlacement;
	}

	Concurrency::ScopedSpinLock lock(navPlacementLock);

	if (_veryLazy &&
		navPlacement.is_valid())
	{
		// is ok
		return navPlacement;
	}

	Room* useRoom = get_room();

	if (useRoom)
	{
		float dist;
		navPlacement = useRoom->find_nav_location(calculate_placement(), &dist);
		if (navPlacement.is_valid() && dist > magic_number 0.2f)
		{
			// we found something but it was too far away
			navPlacement.clear();
		}
	}
	else
	{
		navPlacement.clear();
	}
	
	navPlacementFrameId = frameId;

	return navPlacement;
}

void PointOfInterestInstance::debug_draw(bool _setDebugContext)
{
	update_placement();

	if (auto* useRoom = get_room())
	{
		Optional<Colour> colour;
		if (!cancelled)
		{
			colour = Colour::grey;
		}
		if (_setDebugContext)
		{
			debug_context(useRoom);
		}
		debug_draw_transform_size(true, placement, 0.3f);
		debug_draw_text(true, colour, placement.get_translation(), NP, true, NP, false, poi->name.to_char());
		if (_setDebugContext)
		{
			debug_no_context();
		}
	}
}

void PointOfInterestInstance::cancel()
{
	cancelled = true;
}

bool PointOfInterestInstance::check_space_blockers(Room* _useRoom, Optional<Transform> const& _placement, std::function<void(OUT_ SpaceBlockers& _spaceBlockers)> _get_space_blockers) const
{
	bool result = true;
	bool ignoreSpaceBlockers = parameters.get_value<bool>(NAME(ignoreSpaceBlockers), false);
	if (! ignoreSpaceBlockers && _useRoom)
	{
		SpaceBlockers spaceBlockers;
		_get_space_blockers(OUT_ spaceBlockers);
		Transform usePlacement = _placement.get(placement);
		result = _useRoom->check_space_blockers(spaceBlockers, usePlacement);
#ifdef DEBUG_POI_PLACEMENT
		DebugVisualiserPtr dv(new DebugVisualiser(String(TXT("check space blockers"))));
		dv->activate();
		dv->be_3d();
		dv->start_gathering_data();
		dv->clear();

		_useRoom->get_space_blockers().debug_visualise(dv.get());
		spaceBlockers.debug_visualise(dv.get(), usePlacement);

		dv->end_gathering_data();
		dv->show_and_wait_for_key();
		dv->deactivate();
#endif
	}
	return result;
}


void PointOfInterestInstance::setup_function_door(Room const* _forInRoom, OUT_ DoorType * & _doorType, OUT_ RoomType * & _roomTypeBehindDoor)
{
	if (WorldSettingCondition const * settingCondition = get_parameters().get_existing<WorldSettingCondition>(NAME(doorType)))
	{
		WorldSettingConditionContext wscc(get_room());
		Array<LibraryStored*> available;
		wscc.get(Library::get_current(), DoorType::library_type(), available, *settingCondition);
		if (!available.is_empty())
		{
			int idx = RandomUtils::ChooseFromContainer<Array<LibraryStored*>, LibraryStored*>::choose(access_random_generator(), available, LibraryStored::get_prob_coef_from_tags<LibraryStored>);
			_doorType = fast_cast<DoorType>(available[idx]);
		}
	}
	if (_doorType)
	{
		_doorType = _doorType->process_for_spawning_through_poi(_forInRoom, access_random_generator());
		an_assert(_doorType);
	}
	struct GetAvoidRoomTypeNeighboursTagged
	{
		static TagCondition const* from(Room const* _room)
		{
			if (_room)
			{
				return from(_room->get_room_type());
			}
			return nullptr;
		}

		static TagCondition const* from(RoomType const* _roomType)
		{
			if (_roomType)
			{
				return _roomType->get_custom_parameters().get_existing<TagCondition>(NAME(avoidRoomTypeNeighboursTagged));
			}
			return nullptr;
		}
	};
	TagCondition const* avoidRoomTypeNeighboursTagged = GetAvoidRoomTypeNeighboursTagged::from(_forInRoom);
	int tryIdx = 0;
	while (tryIdx < 5)
	{
		++tryIdx;
		if (WorldSettingCondition const * settingCondition = get_parameters().get_existing<WorldSettingCondition>(NAME(roomType)))
		{
			WorldSettingConditionContext wscc(get_room());
			Array<LibraryStored*> available;
			wscc.get(Library::get_current(), RoomType::library_type(), available, *settingCondition);
			if (!available.is_empty())
			{
				int idx = RandomUtils::ChooseFromContainer<Array<LibraryStored*>, LibraryStored*>::choose(access_random_generator(), available, LibraryStored::get_prob_coef_from_tags<LibraryStored>);
				_roomTypeBehindDoor = fast_cast<RoomType>(available[idx]);
			}
		}
		if (_roomTypeBehindDoor && avoidRoomTypeNeighboursTagged && avoidRoomTypeNeighboursTagged->check(_roomTypeBehindDoor->get_tags()))
		{
			continue;
		}
		if (_roomTypeBehindDoor && _forInRoom)
		{
			an_assert(_forInRoom->get_room_type());
			TagCondition const* avoidRoomTypeNeighboursTagged = GetAvoidRoomTypeNeighboursTagged::from(_roomTypeBehindDoor);
			if (avoidRoomTypeNeighboursTagged && avoidRoomTypeNeighboursTagged->check(_forInRoom->get_room_type()->get_tags()))
			{
				continue;
			}
		}
		break;
	}
}

Optional<Transform> PointOfInterestInstance::find_placement_for(Object* _spawnedObject, Room* _inRoom, Optional<Random::Generator> const& _useRG, Optional<FindPlacementParams> const& _findPlacementParamsWithinMeshNodeBoundary, Optional<FindPlacementParams> const& _findPlacementParamsNoMeshNodeBoundary) const
{
#ifdef AN_INSPECT_POI_PLACEMENT
	output(TXT("get placement for \"%S\" in \"%S\" of type \"%S\""),
		_spawnedObject->get_name().to_char(),
		_inRoom? _inRoom->get_name().to_char() : TXT("<no room>"),
		_inRoom && _inRoom->get_room_type()? _inRoom->get_room_type()->get_name().to_string().to_char() : TXT("<no room type>"));
#endif
	Random::Generator rg = _useRG.get(randomGenerator);
	Name const* withinMeshNodeBoundaryPtr = parameters.get_existing<Name>(NAME(withinMeshNodeBoundary));
	if (withinMeshNodeBoundaryPtr &&
		withinMeshNodeBoundaryPtr->is_valid())
	{
		Name const withinMeshNodeBoundary = *withinMeshNodeBoundaryPtr;
		Array<Vector3> meshNodePoints;

		// find mesh node points
		if (!_inRoom)
		{
#ifdef AN_INSPECT_POI_PLACEMENT
			output(TXT("  no room"));
#endif
			return NP;
		}

		if (auto* ownerIMO = owner.get())
		{
			if (auto* appearance = ownerIMO->get_appearance())
			{
				if (auto* mesh = appearance->get_mesh())
				{
					for_every_ref(mn, mesh->get_mesh_nodes())
					{
						if (mn->name == withinMeshNodeBoundary)
						{
							meshNodePoints.push_back(mn->placement.get_translation());
						}
					}
				}
			}
		}
		else
		{
			Array<Object*> roomObjects;
			Game::get()->perform_sync_world_job(TXT("get objects for nav mesh"),
				[&roomObjects, _inRoom]()
				{
					for_every_ptr(object, _inRoom->get_objects())
					{
						roomObjects.push_back(object);
					}
				}
			);

			for_every_ptr(object, roomObjects)
			{
				if (auto* appearance = object->get_appearance())
				{
					if (auto* mesh = appearance->get_mesh())
					{
						for_every_ref(mn, mesh->get_mesh_nodes())
						{
							if (mn->name == withinMeshNodeBoundary)
							{
								meshNodePoints.push_back(mn->placement.get_translation());
							}
						}
					}
				}
			}

			for_every_ptr(meshInstance, _inRoom->get_appearance().get_instances())
			{
				if (auto* mesh = _inRoom->find_used_mesh_for(meshInstance->get_mesh()))
				{
					for_every_ref(mn, mesh->get_mesh_nodes())
					{
						if (mn->name == withinMeshNodeBoundary)
						{
							meshNodePoints.push_back(meshInstance->get_placement().location_to_world(mn->placement.get_translation()));
						}
					}
				}
			}
		}

		Vector3 const* meshNodeBoundaryUpPtr = parameters.get_existing<Vector3>(NAME(meshNodeBoundaryUp));

		if (meshNodePoints.get_size() >= 3)
		{
			Vector3 up = meshNodeBoundaryUpPtr ? *meshNodeBoundaryUpPtr : (owner.get() ? owner->get_presence()->get_placement().get_axis(Axis::Up) : -_inRoom->get_gravity_dir().get(-Vector3::zAxis));

			Transform meshNodeRef;
			meshNodeRef = matrix_from_up_forward(meshNodePoints.get_first(), up.normal(), (meshNodePoints[1] - meshNodePoints[0].normal())).to_transform();

			ConvexPolygon cp;
			for_every(mnp, meshNodePoints)
			{
				cp.add(meshNodeRef.location_to_local(*mnp).to_vector2());
			}

			cp.build();

			float inRadius = _spawnedObject->get_variables().get_value(NAME(poiPlacementRadius), 0.0f);
			if (inRadius <= 0.0f)
			{
				warn(TXT("no poiPlacementRadius for \"%S\""), _spawnedObject->ai_get_name().to_char());
			}
			inRadius = max(inRadius, MIN_POI_PLACEMENT_RADIUS);

			Optional<Transform> goodPlacement;

			bool atBoundaryEdge = parameters.get_value<bool>(NAME(atBoundaryEdge), false);
			if (auto* abe = _spawnedObject->get_variables().get_existing<bool>(NAME(poiAtBoundaryEdge)))
			{
				atBoundaryEdge = *abe;
			}

			auto findPlacementParams = _findPlacementParamsWithinMeshNodeBoundary.get(FindPlacementParams());
			if (!findPlacementParams.stickToWallRayCastDistance.is_set())
			{
				findPlacementParams.stick_to_wall_ray_cast_distance(2.0f); // check back a lot
			}

			int triesLeft = 50;
			while (!goodPlacement.is_set() && triesLeft)
			{
				Vector2 point;
				point.x = rg.get_float(cp.get_range().x);
				point.y = rg.get_float(cp.get_range().y);

				if (atBoundaryEdge)
				{
					point = cp.move_to_edge(point);
				}

				Optional<Vector2> moved = cp.move_inside(point, inRadius);

				if (moved.is_set())
				{
					Vector2 forward = Vector2::yAxis.rotated_by_angle(rg.get_float(-180.0f, 180.0f));
					if (atBoundaryEdge)
					{
						if (auto* edge = cp.get_closest_edge(moved.get()))
						{
							forward = edge->in;
						}
					}

					Transform checkPlacement = meshNodeRef.to_world(matrix_from_up_forward(moved.get().to_vector3(), Vector3::zAxis, forward.to_vector3()).to_transform());

					goodPlacement = check_placement(findPlacementParams, checkPlacement, inRadius, _spawnedObject, _inRoom, up, rg.spawn(), atBoundaryEdge);
				}

				--triesLeft;
			}

			if (!goodPlacement.is_set() && atBoundaryEdge)
			{
				int factor = 2;
				while (!goodPlacement.is_set() && factor < 5)
				{
					// try placing evenly on edges
					for_count(int, i, cp.get_edge_count())
					{
						if (auto* e = cp.get_edge(i))
						{
							Vector2 forward = e->in;
							for_range(int, j, 1, factor - 1)
							{
								Vector2 atPt = e->get_at_pt((float)j / (float)factor);
								Transform checkPlacement = meshNodeRef.to_world(matrix_from_up_forward(atPt.to_vector3(), Vector3::zAxis, forward.to_vector3()).to_transform());

								goodPlacement = check_placement(findPlacementParams, checkPlacement, 0.0f, _spawnedObject, _inRoom, up, rg.spawn(), atBoundaryEdge);

								if (goodPlacement.is_set())
								{
									break;
								}
							}
						}
						if (goodPlacement.is_set())
						{
							break;
						}
					}
					++factor;
				}
			}

			if (!goodPlacement.is_set())
			{
				// won't be placed at all
#ifdef AN_INSPECT_POI_PLACEMENT
				output(TXT("  no placement found"));
#endif
				return NP;
			}

			an_assert(goodPlacement.get().get_orientation().is_normalised());
#ifdef AN_INSPECT_POI_PLACEMENT
			output(TXT("  good placement found"));
#endif
			return goodPlacement;
		}
		else
		{
#ifdef AN_INSPECT_POI_PLACEMENT
			output(TXT("  no mesh nodes"));
#endif
			return NP;
		}
	}
	else
	{
		Vector3 up = _inRoom ? -_inRoom->get_gravity_dir().get(-Vector3::zAxis) : Vector3::zAxis;
	
		Transform checkPlacement = calculate_placement();

		auto findPlacementParams = _findPlacementParamsNoMeshNodeBoundary.get(FindPlacementParams());
		if (!findPlacementParams.stickToWallInAnyDir.is_set())
		{
			findPlacementParams.stick_to_wall_in_any_dir(true);
		}
		if (!findPlacementParams.stickToWallRayCastDistance.is_set())
		{
			findPlacementParams.stick_to_wall_ray_cast_distance(2.0f); // check back a lot
		}

		Optional<Transform> goodPlacement;
		int triesLeft = 10;
		while (triesLeft > 0 && ! goodPlacement.is_set())
		{
			goodPlacement = check_placement(findPlacementParams, checkPlacement, NP, _spawnedObject, _inRoom, up, rg.spawn(), false);
			--triesLeft;
		}

#ifdef AN_INSPECT_POI_PLACEMENT
		if (goodPlacement.is_set())
		{
			output(TXT("  good placement found"));
		}
		else
		{
			output(TXT("  no good placement"));
		}
#endif
		return goodPlacement;
	}
}

Optional<Transform> PointOfInterestInstance::check_placement(FindPlacementParams const& _findPlacementParams, Transform const & _placement, Optional<float> const & _forwardDistForClamping, Object* _spawnedObject, Room* _inRoom, Vector3 const & _up, Random::Generator const& _useRG, bool _keepForwardAsItIs) const
{
	Random::Generator rg = _useRG;

	float inRadius = _spawnedObject->get_variables().get_value(NAME(poiPlacementRadius), 0.0f);
	float insideRadius = 0.01f; // just in case we're not at wall
	// we do not use MIN_POI_PLACEMENT_RADIUS as we want to be exactly where the objects orders us to be
	bool clampToMeshNodes = _forwardDistForClamping.is_set() && _spawnedObject->get_variables().get_value(NAME(poiClampToMeshNodes), false);
	Range const* stickToWallAtPtr = parameters.get_existing<Range>(NAME(stickToWallAt));
	Range const* poiStickToWallAtPtr = _spawnedObject->get_variables().get_existing<Range>(NAME(poiStickToWallAt));
	bool const* stickToWallAtAnglePtr = parameters.get_existing<bool>(NAME(stickToWallAtAngle));
	bool const* poiStickToWallAtAnglePtr = _spawnedObject->get_variables().get_existing<bool>(NAME(poiStickToWallAtAngle));
	bool const* poiForceStickToWallPtr = _spawnedObject->get_variables().get_existing<bool>(NAME(poiForceStickToWall));
	Range const* addYawRotationPtr = parameters.get_existing<Range>(NAME(addYawRotation));
	bool stickToCeiling = parameters.get_value<bool>(NAME(stickToCeiling), false) || _spawnedObject->get_variables().get_value<bool>(NAME(poiStickToCeiling), false);
	bool atCeilingUpsideDown = _spawnedObject->get_variables().get_value<bool>(NAME(poiAtCeilingUpsideDown), false);
	Range const* poiAddYawRotationPtr = _spawnedObject->get_variables().get_existing<Range>(NAME(poiAddYawRotation));
	float const* poiRoundYawRotationPtr = _spawnedObject->get_variables().get_existing<float>(NAME(poiRoundYawRotation));

	float const * poiNormaliseWallHeight = _spawnedObject->get_variables().get_existing<float>(NAME(poiNormaliseWallHeight));
	Name const * poiNormaliseWallRoomHeightVariable = _spawnedObject->get_variables().get_existing<Name>(NAME(poiNormaliseWallRoomHeightVariable));

	// override some of the params
	if (poiStickToWallAtPtr)
	{
		stickToWallAtPtr = poiStickToWallAtPtr;
	}
	if (poiStickToWallAtAnglePtr)
	{
		stickToWallAtAnglePtr = poiStickToWallAtAnglePtr;
	}
	if (poiAddYawRotationPtr)
	{
		addYawRotationPtr = poiAddYawRotationPtr;
	}

	float roundYawRotation = 0.0f;
	if (poiRoundYawRotationPtr)
	{
		roundYawRotation = *poiRoundYawRotationPtr;
	}

	Transform checkPlacement = _placement;

	if (!_keepForwardAsItIs)
	{
		if (addYawRotationPtr)
		{
			float yaw = rg.get(*addYawRotationPtr);
			if (roundYawRotation)
			{
				yaw = round_to(yaw, roundYawRotation);
			}
			checkPlacement = checkPlacement.to_world(Transform(Vector3::zero, Rotator3(0.0f, yaw, 0.0f).to_quat()));
		}
		else if (roundYawRotation > 0.0f)
		{
			Rotator3 ori = checkPlacement.get_orientation().to_rotator();
			ori.yaw = round_to(ori.yaw, roundYawRotation);
			checkPlacement.set_orientation(ori.to_quat());
		}
	}
	
	bool isOk = true;

	if (stickToWallAtPtr)
	{
#ifdef AN_INSPECT_POI_PLACEMENT
		output(TXT("  wants to be at wall"));
#endif
		Transform checkPlacementNow = checkPlacement;
		if (_findPlacementParams.stickToWallInAnyDir.get(false))
		{
			float yaw = rg.get_float(-180.0f, 180.0f);
			checkPlacementNow = checkPlacementNow.to_world(Transform(Vector3::zero, Rotator3(0.0f, yaw, 0.0f).to_quat()));
		}
		float stickToWallRayCastDistance = _findPlacementParams.stickToWallRayCastDistance.get(0.5f);
		float atOffset = rg.get(*stickToWallAtPtr);
		if (poiNormaliseWallHeight && poiNormaliseWallRoomHeightVariable && _inRoom &&
			*poiNormaliseWallHeight > 0.0f)
		{
			float wallHeight = _inRoom->get_value<float>(*poiNormaliseWallRoomHeightVariable, *poiNormaliseWallHeight);
			if (wallHeight < *poiNormaliseWallHeight)
			{
				atOffset *= wallHeight / *poiNormaliseWallHeight;
			}
		}
		Vector3 offGroundOffset = _up * max(0.0f, 0.1f - atOffset);
		checkPlacementNow.set_translation(checkPlacementNow.get_translation() + _up * atOffset);

		Segment segment(checkPlacementNow.location_to_world(Vector3::yAxis * insideRadius) + offGroundOffset, checkPlacementNow.location_to_world(-Vector3::yAxis * (inRadius * 2.0f + stickToWallRayCastDistance)) + offGroundOffset);
		CheckSegmentResult result;
		CheckCollisionContext context;
		Room const* endsInRoom;
		Transform intoEndRoomTransform;
		context.start_with_nothing_to_check();
		context.check_room();
		context.check_room_scenery();
		context.use_collision_flags(GameConfig::point_of_interest_collides_with_flags());
		isOk = false;
		if (_inRoom->check_segment_against(AgainstCollision::Movement, segment, result, context, endsInRoom, intoEndRoomTransform))
		{
			if (_inRoom == endsInRoom)
			{
#ifdef AN_INSPECT_POI_PLACEMENT
				output(TXT("  successfully planted at wall"));
#endif

				isOk = true;
				if (!poiForceStickToWallPtr || !(*poiForceStickToWallPtr))
				{
					if (clampToMeshNodes && _forwardDistForClamping.is_set())
					{
						float moveForwardBy = max(0.0f, Vector3::dot(checkPlacementNow.get_axis(Axis::Y), checkPlacementNow.get_translation() - result.hitLocation) - _forwardDistForClamping.get());
						result.hitLocation = result.hitLocation + checkPlacementNow.get_axis(Axis::Y) * moveForwardBy;
					}
				}
				Vector3 loc = result.hitLocation - offGroundOffset + checkPlacementNow.vector_to_world(Vector3::yAxis * inRadius);
				Vector3 up = _up;
				bool keepFwd = !(stickToWallAtAnglePtr && *stickToWallAtAnglePtr) && _keepForwardAsItIs;
				Vector3 fwd = keepFwd ? checkPlacementNow.get_axis(Axis::Forward) : result.hitNormal;
				if (stickToWallAtAnglePtr && *stickToWallAtAnglePtr)
				{
					checkPlacement = look_matrix(loc, fwd.normal(), up.normal()).to_transform();
				}
				else
				{
					checkPlacement = matrix_from_up_forward(loc, up, fwd).to_transform();
				}
			}
		}
	}

	if (stickToCeiling)
	{
#ifdef AN_INSPECT_POI_PLACEMENT
		output(TXT("  wants to be at ceiling"));
#endif
		Transform checkPlacementNow = checkPlacement;
		Segment segment(checkPlacementNow.location_to_world(_up * 0.5f /* most likely we are at the floor */), checkPlacementNow.location_to_world(_up * 5.0f));
		CheckSegmentResult result;
		CheckCollisionContext context;
		Room const* endsInRoom;
		Transform intoEndRoomTransform;
		context.start_with_nothing_to_check();
		context.check_room();
		context.check_room_scenery();
		context.use_collision_flags(GameConfig::point_of_interest_collides_with_flags());
		isOk = false;
		if (_inRoom->check_segment_against(AgainstCollision::Movement, segment, result, context, endsInRoom, intoEndRoomTransform))
		{
			if (_inRoom == endsInRoom &&
				Vector3::dot(result.hitNormal, -_up) > 0.9f)
			{
#ifdef AN_INSPECT_POI_PLACEMENT
				output(TXT("  successfully planted at ceiling"));
#endif
				isOk = true;
				checkPlacement = matrix_from_up_forward(result.hitLocation, atCeilingUpsideDown? -_up : _up, checkPlacementNow.get_axis(Axis::Forward)).to_transform();
			}
		}
	}

	if (isOk)
	{
		if (auto* sbo = fast_cast<SpaceBlockingObject>(_spawnedObject))
		{
			isOk = check_space_blockers(_inRoom, checkPlacement,
				[sbo](OUT_ SpaceBlockers& _spaceBlockers)
				{
					_spaceBlockers = sbo->get_space_blocker_local();
				});
#ifdef AN_INSPECT_POI_PLACEMENT
			if (isOk)
			{
				output(TXT("  ok with space blockers"));
			}
			else
			{
				output(TXT("  problem with space blockers"));
			}
#endif
		}
	}

	if (isOk)
	{
		return checkPlacement;
	}

	return NP;
}
