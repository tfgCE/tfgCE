#include "aiLogic_pilgrim.h"

#include "..\..\library\library.h"
#include "..\..\game\game.h"
#include "..\..\game\gameDirector.h"
#include "..\..\game\modes\gameMode_Pilgrimage.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\pilgrimage\pilgrimageInstanceLinear.h"
#include "..\..\sound\voiceoverSystem.h"
#include "..\..\tutorials\tutorialSystem.h"

#include "..\..\..\core\debug\debugRenderer.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleMovementScripted.h"
#include "..\..\..\framework\nav\navPath.h"
#include "..\..\..\framework\nav\navTask.h"
#include "..\..\..\framework\nav\tasks\navTask_FindPath.h"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakEXMSystemDetected, TXT("pi.ov.in; speak; exm system detected"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakReachedEXMSystemRoom, TXT("pi.ov.in; speak; reached exm system room"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakInterfaceBoxDetected, TXT("pi.ov.in; speak; interface box detected"));
DEFINE_STATIC_NAME_STR(grOverlayInfoSpeakInfestationDetected, TXT("pi.ov.in; speak; infestation detected"));

// input
DEFINE_STATIC_NAME(immobileVRMovement);
DEFINE_STATIC_NAME(immobileVRRotation);

// pilgrimage systems
DEFINE_STATIC_NAME(exm);
DEFINE_STATIC_NAME(interfaceBox);
DEFINE_STATIC_NAME(infestation);

//

REGISTER_FOR_FAST_CAST(Pilgrim);

Pilgrim::Pilgrim(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
, pilgrimData(fast_cast<PilgrimData>(_logicData))
{
}

Pilgrim::~Pilgrim()
{
}

void Pilgrim::advance(float _deltaTime)
{
	base::advance(_deltaTime);
}

LATENT_FUNCTION(Pilgrim::guidance)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto

	LATENT_PARAM(::Framework::AI::Logic*, logic);
	LATENT_END_PARAMS();

	LATENT_VAR(SafePtr<Framework::Room>, pilgrimsDestinationRoom);
	LATENT_VAR(PilgrimageDevice const*, pilgrimsDestinationDevice);
	LATENT_VAR(Name, pilgrimsDestinationSystem);
	LATENT_VAR(float, timeSinceInPilgrimsDestinationRoom);
	LATENT_VAR(SafePtr<Framework::IModulesOwner>, pilgrimsDestinationObject);
	LATENT_VAR(SafePtr<Framework::IModulesOwner>, pilgrimsDestinationMainObject);
	LATENT_VAR(bool, newPathRequired);
	LATENT_VAR(float, newPathCooldown);
#ifdef AN_DEVELOPMENT
	LATENT_VAR(int, consequtiveFails);
#endif
	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, navTask);
	LATENT_VAR(Framework::RelativeToPresencePlacement, toLastPointOnPath); // in case we changed door
	
	LATENT_VAR(Framework::UsedLibraryStored<Framework::Sample>, speakEXMSystemDetectedLine); // in case we changed door
	LATENT_VAR(Framework::UsedLibraryStored<Framework::Sample>, speakReachedEXMSystemRoomLine); // in case we changed door
	LATENT_VAR(Framework::UsedLibraryStored<Framework::Sample>, speakInterfaceBoxDetectedLine); // in case we changed door
	LATENT_VAR(Framework::UsedLibraryStored<Framework::Sample>, speakInfestationDetectedLine); // in case we changed door

	LATENT_VAR(float, timeToUpdateDistance);

	auto* self = fast_cast<Pilgrim>(logic);

	newPathCooldown = max(0.0f, newPathCooldown - LATENT_DELTA_TIME);
	timeToUpdateDistance = max(0.0f, timeToUpdateDistance - LATENT_DELTA_TIME);
	timeSinceInPilgrimsDestinationRoom = min(1000.0f, timeSinceInPilgrimsDestinationRoom + LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	pilgrimsDestinationRoom = nullptr;
	pilgrimsDestinationDevice = nullptr;
	pilgrimsDestinationSystem = Name::invalid();
	pilgrimsDestinationObject.clear();
	pilgrimsDestinationMainObject.clear();
	timeSinceInPilgrimsDestinationRoom = 1000.0f;

	newPathRequired = false;
	newPathCooldown = 0.0f;
	timeToUpdateDistance = 0.0f;
	toLastPointOnPath.set_owner(mind->get_owner_as_modules_owner());

	{
		if (auto* lib = Framework::Library::get_current())
		{
			speakEXMSystemDetectedLine = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakEXMSystemDetected));
			speakReachedEXMSystemRoomLine = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakReachedEXMSystemRoom));
			speakInterfaceBoxDetectedLine = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakInterfaceBoxDetected));
			speakInfestationDetectedLine = lib->get_global_references().get<Framework::Sample>(NAME(grOverlayInfoSpeakInfestationDetected));
		}
	}

#ifdef AN_DEVELOPMENT
	consequtiveFails = 0;
#endif

	while (true)
	{
		if (auto* game = Game::get_as<Game>())
		{
			bool lookForNavTarget = true;
			bool resetPath = false;
			/*
			if (lookForNavTarget &&
				TutorialSystem::check_active())
			{
				if (auto* guidanceTarget = TutorialSystem::get()->get_guidance_target())
				{
					if (pilgrimsDestinationObject != guidanceTarget)
					{
						pilgrimsDestinationRoom = nullptr;
						pilgrimsDestinationDevice = nullptr;
						pilgrimsDestinationSystem = Name::invalid();
						pilgrimsDestinationObject = guidanceTarget;
						timeSinceInPilgrimsDestinationRoom = 1000.0f;
						newPathRequired = true;
						resetPath = true;
					}
					lookForNavTarget = false;
				}
			}
			*/
			if (lookForNavTarget)
			{
				if (auto* pilgrimage = fast_cast<GameModes::Pilgrimage>(game->get_mode()))
				{
					Name actualSystem;
					PilgrimageDevice const* actualDevice = nullptr;
					Framework::Room* newPilgrimsDestinationRoom = nullptr;
					Framework::IModulesOwner* newPilgrimsDestinationMainObject = nullptr;
					if (auto* pi = PilgrimageInstance::get())
					{
						newPilgrimsDestinationRoom = pi->get_pilgrims_destination(OUT_ & actualSystem, OUT_ &actualDevice, OUT_ & newPilgrimsDestinationMainObject);
					}

					bool deviceChanged = pilgrimsDestinationMainObject != newPilgrimsDestinationMainObject;
					pilgrimsDestinationDevice = actualDevice;
					pilgrimsDestinationMainObject = newPilgrimsDestinationMainObject;
					pilgrimsDestinationSystem = actualSystem;
					if (pilgrimsDestinationRoom != newPilgrimsDestinationRoom)
					{
						pilgrimsDestinationRoom = newPilgrimsDestinationRoom;
						if (pilgrimsDestinationRoom == mind->get_owner_as_modules_owner()->get_presence()->get_in_room())
						{
							timeSinceInPilgrimsDestinationRoom = 0.0f; // if we start in the room, we should not talk about it
						}
						else
						{
							timeSinceInPilgrimsDestinationRoom = 1000.0f;
							{
								if (newPilgrimsDestinationRoom && deviceChanged)
								{
									if (auto* mp = mind->get_owner_as_modules_owner()->get_gameplay_as<ModulePilgrim>())
									{
										if (actualSystem == NAME(exm))
										{
											if (!GameSettings::get().difficulty.showDirectionsOnlyToObjective)
											{
												mp->access_overlay_info().speak(speakEXMSystemDetectedLine.get());
											}
										}
										else if (actualSystem == NAME(interfaceBox))
										{
											mp->access_overlay_info().speak(speakInterfaceBoxDetectedLine.get());
										}
										else if (actualSystem == NAME(infestation))
										{
											mp->access_overlay_info().speak(speakInfestationDetectedLine.get());
										}
									}									
								}
							}
						}
						newPathRequired = true;
						resetPath = true;
					}

					pilgrimsDestinationObject = nullptr;
					pilgrimsDestinationMainObject = nullptr;
					lookForNavTarget = false;
				}
			}
			if (lookForNavTarget)
			{
				if (pilgrimsDestinationRoom.is_set() ||
					pilgrimsDestinationObject.is_set())
				{
					pilgrimsDestinationRoom = nullptr;
					pilgrimsDestinationSystem = Name::invalid();
					pilgrimsDestinationObject = nullptr;
					timeSinceInPilgrimsDestinationRoom = 1000.0f;
					resetPath = true;
				}
			}
			if (resetPath)
			{
				if (navTask.is_set())
				{
					navTask->cancel();
					navTask.clear();
				}
				if (path.is_set())
				{
					path.clear();
				}
			}
		}

		if (pilgrimsDestinationRoom.is_set() && (pilgrimsDestinationRoom == mind->get_owner_as_modules_owner()->get_presence()->get_in_room()))
		{
			if (timeSinceInPilgrimsDestinationRoom > 30.0f)
			{
				if (pilgrimsDestinationDevice &&
					pilgrimsDestinationDevice->is_revealed_upon_entrance())
				{
					if (auto* piow = PilgrimageInstanceOpenWorld::get())
					{
						piow->mark_pilgrimage_device_direction_known(pilgrimsDestinationDevice);
					}
				}
				if (auto* mp = mind->get_owner_as_modules_owner()->get_gameplay_as<ModulePilgrim>())
				{
					if (pilgrimsDestinationSystem == NAME(exm) &&
						!GameSettings::get().difficulty.showDirectionsOnlyToObjective)
					{
						mp->access_overlay_info().speak(speakReachedEXMSystemRoomLine.get());
					}
				}
			}
			timeSinceInPilgrimsDestinationRoom = 0.0f;
		}

		if (((pilgrimsDestinationRoom.is_set() && (pilgrimsDestinationRoom != mind->get_owner_as_modules_owner()->get_presence()->get_in_room())) ||
			(pilgrimsDestinationObject.is_set()))
#ifdef AN_DEVELOPMENT_OR_PROFILER
			//|| Game::get_as<Game>()->is_autopilot_on() // if auto pilot is on, always move
#endif
			)
		{
			if (!navTask.is_set() &&
				(!path.is_set() || newPathRequired))
			{
				if (newPathCooldown <= 0.0f)
				{
					auto* imo = mind->get_owner_as_modules_owner();
					if (pilgrimsDestinationObject.is_set() && pilgrimsDestinationObject->get_presence())
					{
						navTask = mind->access_navigation().find_path_to(pilgrimsDestinationObject->get_presence()->get_in_room(), pilgrimsDestinationObject->get_presence()->get_centre_of_presence_WS(),
							Framework::Nav::PathRequestInfo(imo).through_closed_doors() // we want to know proper direction
						);
						newPathRequired = false;
					}
					else if (pilgrimsDestinationRoom.is_set())
					{
						navTask = mind->access_navigation().find_path_to(pilgrimsDestinationRoom.get(), Vector3::zero,
							Framework::Nav::PathRequestInfo(imo).through_closed_doors() // we want to know proper direction
						);
						newPathRequired = false;
					}
				}
			}
			if (navTask.is_set() &&
				navTask->is_done())
			{
				if (navTask->has_succeed())
				{
#ifdef AN_DEVELOPMENT
					consequtiveFails = 0;
#endif
					if (auto* task = fast_cast<Framework::Nav::Tasks::PathTask>(navTask.get()))
					{
						path = task->get_path();
						toLastPointOnPath.clear_target();
					}
				}
				// if failed, maybe we're waiting for nav system to build nav mesh?
#ifdef AN_DEVELOPMENT
				else
				{
					++consequtiveFails;
					if (consequtiveFails >= 5)
					{
						warn(TXT("could not find path for guidance!"));
					}
					LATENT_WAIT(1.0f); // try again after a wait
				}
#endif
				navTask = nullptr;
				newPathCooldown = 1.0f; // wait a second before requesting new path
			}

			if (path.is_set())
			{
				if (auto* imo = mind->get_owner_as_modules_owner())
				{
					if (auto* presence = imo->get_presence())
					{
						Vector3 currLoc = presence->get_placement().get_translation();
						Vector3 closestLoc;
						Vector3 targetLoc;

						if (timeToUpdateDistance == 0.0f)
						{
							if (auto* game = Game::get_as<Game>())
							{
								if (auto* pilgrimage = fast_cast<GameModes::Pilgrimage>(game->get_mode()))
								{
									if (auto* pil = pilgrimage->access_pilgrimage_instance_as<PilgrimageInstanceLinear>())
									{
										pil->linear__provide_distance_to_next_station(path->calculate_distance(presence->get_in_room(), currLoc, true));
									}
								}
							}
							timeToUpdateDistance = Random::get_float(0.01f, 0.15f);
						}
#ifdef AN_DEBUG_GUIDANCE
						{
							debug_filter(pilgrim_guidance);
							Framework::Nav::PathNode const* prev = nullptr;
							for_every(pathNode, path->get_path_nodes())
							{
								if (prev && prev->placementAtNode.get_room() == pathNode->placementAtNode.get_room())
								{
									debug_context(pathNode->placementAtNode.get_room());
									Colour colour = Colour::cyan;
									if (pathNode->connectionData.is_set() &&
										pathNode->connectionData->is_blocked_temporarily())
									{
										colour = Colour::blue;
									}
									debug_draw_arrow(true, colour, prev->placementAtNode.get_current_placement().get_translation(), pathNode->placementAtNode.get_current_placement().get_translation());
									debug_no_context();
								}
								prev = pathNode;
							}
							debug_no_filter();
						}
#endif
						// always find closest location to path. we could move on a platform
						bool hitTemporaryBlock = false;
						float const goToLastPointOnPathDist = 1.0f;
						if (path->find_forward_location(presence->get_in_room(), currLoc, 0.7f, OUT_ targetLoc, OUT_ & closestLoc, OUT_ & hitTemporaryBlock))
						{
							toLastPointOnPath.clear_target();
							toLastPointOnPath.set_placement_in_final_room(Transform(closestLoc, Quat::identity));

							float toLastPointOnPathDist = toLastPointOnPath.is_active() ? toLastPointOnPath.calculate_string_pulled_distance() : 0.0f;
							bool goToLastPointOnPath = toLastPointOnPathDist > goToLastPointOnPathDist;
#ifdef AN_DEBUG_GUIDANCE
							debug_filter(pilgrim_guidance);
							debug_draw_arrow(true, Colour::yellow, presence->get_placement().get_translation(),
								toLastPointOnPath.is_active() ? toLastPointOnPath.get_placement_in_owner_room().get_translation()
								: presence->get_placement().get_translation());
							debug_no_filter();
#endif
							if (goToLastPointOnPath)
							{
								newPathRequired = true;
								self->directionOS = presence->get_os_to_ws_transform().location_to_local(toLastPointOnPath.get_placement_in_owner_room().get_translation());
#ifdef AN_DEBUG_GUIDANCE
								Transform currPlacement = presence->get_placement();
								debug_filter(pilgrim_guidance);
								debug_context(presence->get_in_room());
								debug_draw_arrow(true, Colour::cyan, currLoc, toLastPointOnPath.get_placement_in_owner_room().get_translation());
								debug_push_transform(currPlacement);
								debug_draw_arrow(true, Colour::orange, Vector3::zero, self->directionOS);
								debug_pop_transform();
								debug_no_context();
								debug_no_filter();
#endif
							}
							else
							{
								Transform currPlacement = presence->get_placement();
								float distToClosestLoc = (currPlacement.get_translation() - closestLoc).length();
								float useClosestLoc = clamp(2.0f * (1.0f - distToClosestLoc / goToLastPointOnPathDist), 0.0f, 1.0f);
								currPlacement.set_translation(currPlacement.get_translation() * (1.0f - useClosestLoc) + closestLoc * useClosestLoc);

								self->directionOS = currPlacement.location_to_local(targetLoc);

#ifdef AN_DEBUG_GUIDANCE
								debug_filter(pilgrim_guidance);
								debug_context(presence->get_in_room());
								debug_draw_arrow(true, Colour::cyan, currLoc, targetLoc);
								debug_push_transform(currPlacement);
								debug_draw_arrow(true, Colour::green, Vector3::zero, self->directionOS);
								debug_pop_transform();
								debug_no_context();
								debug_no_filter();
#endif
							}
						}
						else if (toLastPointOnPath.is_active())
						{
							self->directionOS = presence->get_os_to_ws_transform().location_to_local(toLastPointOnPath.get_placement_in_owner_room().get_translation());
							newPathRequired = true;
#ifdef AN_DEBUG_GUIDANCE
							debug_filter(pilgrim_guidance);
							debug_context(presence->get_in_room());
							debug_push_transform(presence->get_placement());
							debug_draw_arrow(true, Colour::red * 0.5f, Vector3::zero, self->directionOS);
							debug_pop_transform();
							debug_no_context();
							debug_no_filter();
#endif
						}
						else
						{
							self->directionOS = Vector3::zero;
						}
						if (hitTemporaryBlock)
						{
							//self->directionOS = Vector3::zero;
						}
						self->directionOS.normalise();
						// get vertical
						if (abs(self->directionOS.z) > 0.4f)
						{
							self->directionOS = Vector3::zAxis * sign(self->directionOS.z);
						}
						// get closer to XY plane
						else if (self->directionOS.z > 0.0f)
						{
							self->directionOS.z = max(0.0f, self->directionOS.z - 0.3f);
						}
						else if (self->directionOS.z < 0.0f)
						{
							self->directionOS.z = min(0.0f, self->directionOS.z + 0.3f);
						}
						self->directionOS.normalise();
					}
				}
			}
			else
			{
				self->directionOS = Vector3::zero;
			}
		}
		else
		{
			self->directionOS = Vector3::zero;
			path.clear();
			if (navTask.is_set())
			{
				navTask->cancel();
				navTask.clear();
			}
		}

		LATENT_YIELD();
		//LATENT_WAIT(Random::get_float(0.05f, 0.1f));
	}

	LATENT_ON_BREAK();

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Pilgrim::update_dist)
{
	scoped_call_stack_info(TXT("Pilgrim::update_dist"));

	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	
	LATENT_PARAM(::Framework::AI::Logic*, logic);
	LATENT_END_PARAMS();

	LATENT_VAR(Optional<Vector3>, lastVRCheckpoint); // we add distance
	LATENT_VAR(bool, updateSection);
	LATENT_VAR(float, timeSinceLastUpdate);
	
	LATENT_VAR(float, timeUpdateDistActive);
	LATENT_VAR(bool, shownInfoAboutMoving);
	LATENT_VAR(float, shownInfoAboutMovingAtTotalDist);
	LATENT_VAR(bool, showingNowInfoAboutMoving);
	LATENT_VAR(bool, movedRoomScaleAtLeastOnce);
	LATENT_VAR(bool, mayStillShowInfoAboutMovingDueToUseOfJoystick);
	LATENT_VAR(Optional<float>, showInfoDueToUseOfJoystickTimeLeft);

	auto * self = fast_cast<Pilgrim>(logic);

	float const updateDistThresholdDist = 0.25f; // more generous as when turning head it will cover that too...
	float const updateDistThresholdTime = 1.0f;

	timeSinceLastUpdate += LATENT_DELTA_TIME;
	timeUpdateDistActive += LATENT_DELTA_TIME;
	if (showInfoDueToUseOfJoystickTimeLeft.is_set())
	{
		showInfoDueToUseOfJoystickTimeLeft = showInfoDueToUseOfJoystickTimeLeft.get() - LATENT_DELTA_TIME;
	}

	LATENT_BEGIN_CODE();

	timeSinceLastUpdate = 0.0f;
	updateSection = false;

	timeUpdateDistActive = 0.0f;
	shownInfoAboutMoving = false;
	showingNowInfoAboutMoving = false;
	movedRoomScaleAtLeastOnce = false;
	mayStillShowInfoAboutMovingDueToUseOfJoystick = true;
	if (auto* p = PlayerSetup::get_current_if_exists())
	{
		movedRoomScaleAtLeastOnce = p->get_preferences().movedRoomScaleAtLeastOnce;
	}

	while (true)
	{
		if (auto* imo = mind->get_owner_as_modules_owner())
		{
			scoped_call_stack_info(TXT("Pilgrim::update_dist loop"));
			bool shouldUpdateSection = true;
			int sectionIdx = self->sectionIdx;
			if (auto* game = Game::get_as<Game>())
			{
				scoped_call_stack_info(TXT("check pilgrimage"));
				if (auto* pilgrimage = fast_cast<GameModes::Pilgrimage>(game->get_mode()))
				{
					if (pilgrimage->is_linear())
					{
						shouldUpdateSection = !pilgrimage->linear__is_station(imo->get_presence()->get_in_room()) && pilgrimage->linear__get_current_region();
						sectionIdx = pilgrimage->linear__get_current_region_idx();
					}
				}
			}
			if (shouldUpdateSection)
			{
				scoped_call_stack_info(TXT("shouldUpdateSection"));
				if (self->sectionIdx != sectionIdx)
				{
					// reset section
					self->sectionIdx = sectionIdx;
					self->sectionDist = 0.0f;
				}
			}
			updateSection = shouldUpdateSection;
			self->sectionActive = updateSection;

			if (fast_cast<Framework::ModuleMovementScripted>(imo->get_movement()) == nullptr)
			{
				scoped_call_stack_info(TXT("vr location?"));
				float addDistance = 0.0f;
				if (Game::is_using_sliding_locomotion())
				{
					// check relative to base, this way we will remove the base's movement
					addDistance = imo->get_presence()->get_prev_placement_offset().get_translation().length_2d();
					auto* b = imo->get_presence()->get_based_on();
					if (b)
					{
						Transform placementWS = imo->get_presence()->get_placement();
						Transform prevPlacementWS = placementWS.to_world(imo->get_presence()->get_prev_placement_offset());
						Transform basePlacementWS = b->get_presence()->get_placement();
						Transform prevBasePlacementWS = basePlacementWS.to_world(b->get_presence()->get_prev_placement_offset());

						Transform placementOnBase = basePlacementWS.to_local(placementWS);
						Transform prevPlacementOnBase = prevBasePlacementWS.to_local(prevPlacementWS);

						Vector3 offsetOnBase = (placementOnBase.get_translation() - prevPlacementOnBase.get_translation());
						addDistance = prevPlacementOnBase.vector_to_local(offsetOnBase).length_2d();
					}
				}
				else
				{
					Vector3 vrLocation = imo->get_presence()->get_placement_in_vr_space().get_translation();
					if (!vrLocation.is_almost_zero()) // highly unlikely that someone will be standing exactly at 0,0,0 but we still got it covered if they move outside, this may happen when head is not tracked at the beginning
					{
						scoped_call_stack_info(TXT("vr location"));
						if (!lastVRCheckpoint.is_set())
						{
							lastVRCheckpoint = vrLocation;
						}
						Vector3 dirCovered = (vrLocation - lastVRCheckpoint.get());
						if (auto* vr = VR::IVR::get())
						{
							float scaling = vr->get_scaling().general * vr->get_scaling().horizontal;
							if (scaling != 0.0f)
							{
								dirCovered /= scaling; // we want actual movement only!
							}
						}
						float distCovered = dirCovered.length_2d();
						dirCovered.normalise_2d();
						bool tooFar = distCovered > updateDistThresholdDist;
						bool tooLate = timeSinceLastUpdate >= updateDistThresholdTime;
						if (tooFar || tooLate)
						{
							if (tooLate)
							{
								distCovered *= 0.2f; // we're probably standing in place, we will move our vr checkpoint just a bit
							}
							addDistance = distCovered;
							lastVRCheckpoint = tooLate ? lastVRCheckpoint.get() + dirCovered * distCovered : vrLocation;
							timeSinceLastUpdate = 0.0f;
						}
					}
				}
				if (addDistance > 0.0f)
				{
					if (updateSection)
					{
						self->sectionDist += addDistance;
					}
					self->totalDist += addDistance;
					self->distToConsume += addDistance;
					PlayerSetup::access_current().stats__increase_distance(addDistance);
				}
			}
			if (! Game::is_using_sliding_locomotion())
			{
				if (VR::IVR::get())
				{
					if ((!shownInfoAboutMoving && ! movedRoomScaleAtLeastOnce) || // this is to automatically show once after a certain type
						mayStillShowInfoAboutMovingDueToUseOfJoystick) // this is to allow repeated use of joysticks (at least for a while)
					{
						if (auto* gd = GameDirector::get())
						{
							if (gd->is_real_movement_input_tip_blocked())
							{
								timeUpdateDistActive = 0.0f; // stop and restart
							}
						}
						bool showNow = false;
						if (auto* g = Game::get_as<Game>())
						{
							auto& player = g->access_player();
							if (!shownInfoAboutMoving && !movedRoomScaleAtLeastOnce)
							{
								if ((self->totalDist < 1.0f && timeUpdateDistActive > 10.0f) ||
									(self->totalDist < 2.0f && timeUpdateDistActive > 20.0f))
								{
									if (auto* p = PlayerSetup::get_current_if_exists())
									{
										if (!p->get_preferences().movedRoomScaleAtLeastOnce)
										{
											bool allow = true;
											if (auto* vs = VoiceoverSystem::get())
											{
												if (vs->is_anyone_speaking())
												{
													allow = false;
												}
											}
											if (allow)
											{
												showInfoDueToUseOfJoystickTimeLeft.clear();
												showNow = true;
											}
										}
									}
								}
							}
							if (mayStillShowInfoAboutMovingDueToUseOfJoystick)
							{
								if (timeUpdateDistActive > 60.0f)
								{
									mayStillShowInfoAboutMovingDueToUseOfJoystick = false;
								}
								if (player.get_actor() == imo)
								{
									Vector2 movement = player.get_input_non_vr().get_stick(NAME(immobileVRMovement))
													 + player.get_input_vr_left().get_stick(NAME(immobileVRMovement))
													 + player.get_input_vr_right().get_stick(NAME(immobileVRMovement));
									Vector2 rotation = player.get_input_non_vr().get_stick(NAME(immobileVRRotation))
													 + player.get_input_vr_left().get_stick(NAME(immobileVRRotation))
													 + player.get_input_vr_right().get_stick(NAME(immobileVRRotation));

									if (movement.length() > 0.8f ||
										rotation.length() > 0.8f)
									{
										if (shownInfoAboutMoving)
										{
											// this is for the repeated use of joystick
											showInfoDueToUseOfJoystickTimeLeft = 5.0f;
										}
										showNow = true;
									}
								}
							}
							if (showNow)
							{
								if (auto* pilgrim = imo->get_gameplay_as< ModulePilgrim>())
								{
									if (pilgrim->access_overlay_info().is_showing_main())
									{
										// if showing main, means map, don't show anything
										showNow = false;
									}
								}
							}
							if (showNow)
							{
								if (!showingNowInfoAboutMoving)
								{
									shownInfoAboutMovingAtTotalDist = self->totalDist;
								}
								shownInfoAboutMoving = true;
								showingNowInfoAboutMoving = true;
#ifdef AN_DEVELOPMENT_OR_PROFILER
								// never show it for development / profiler builds
								showingNowInfoAboutMoving = false;
#endif
							}
						}
					}
					// spam it!
					if (showingNowInfoAboutMoving)
					{
						if (auto* pilgrim = imo->get_gameplay_as< ModulePilgrim>())
						{
							auto& poi = pilgrim->access_overlay_info();
							if (! poi.is_showing_tip(PilgrimOverlayInfoTip::InputRealMovement))
							{
								TeaForGodEmperor::PilgrimOverlayInfo::ShowTipParams params;
								params.be_forced();
								poi.show_tip(PilgrimOverlayInfoTip::InputRealMovement, params);
							}
							bool hideTip = false;
							if (self->totalDist > shownInfoAboutMovingAtTotalDist + 0.5f)
							{
								hideTip = true;
							}
							if (showInfoDueToUseOfJoystickTimeLeft.is_set())
							{
								if (showInfoDueToUseOfJoystickTimeLeft.get() <= 0.0f)
								{
									showInfoDueToUseOfJoystickTimeLeft.clear();
									hideTip = true;
								}
							}
							if (hideTip || ! PlayerPreferences::should_show_tips_during_game())
							{
								showingNowInfoAboutMoving = false;
								poi.hide_tip(PilgrimOverlayInfoTip::InputRealMovement);
							}
						}
					}

					if (!movedRoomScaleAtLeastOnce)
					{
						// definitely moved
						if (self->totalDist > 2.0f)
						{
							if (auto* p = PlayerSetup::access_current_if_exists())
							{
								p->access_preferences().movedRoomScaleAtLeastOnce = true;
								movedRoomScaleAtLeastOnce = true;
							}
						}
					}
				}
			}
		}

		if (Game::is_using_sliding_locomotion())
		{
			LATENT_YIELD(); // each frame
		}
		else
		{
			LATENT_WAIT(0.1f);
		}
	}

	LATENT_ON_BREAK();

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Pilgrim::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::LatentTaskHandle, guidanceTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, distTask);

	LATENT_BEGIN_CODE();

	{	// guidance
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.clear();
		if (taskInfo.propose(AI_LATENT_TASK_FUNCTION(guidance)))
		{
			ADD_LATENT_TASK_INFO_PARAM(taskInfo, ::Framework::AI::Logic*, logic);
		}
		guidanceTask.start_latent_task(mind, taskInfo);
	}

	{	// update distance
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.clear();
		if (taskInfo.propose(AI_LATENT_TASK_FUNCTION(update_dist)))
		{
			ADD_LATENT_TASK_INFO_PARAM(taskInfo, ::Framework::AI::Logic*, logic);
		}
		distTask.start_latent_task(mind, taskInfo);
	}

	while (true)
	{
		// hang in here
		LATENT_WAIT(1.0f);
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	guidanceTask.stop();
	distTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}


//

REGISTER_FOR_FAST_CAST(PilgrimData);

bool PilgrimData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}