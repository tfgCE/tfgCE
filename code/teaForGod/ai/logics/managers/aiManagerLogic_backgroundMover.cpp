#include "aiManagerLogic_backgroundMover.h"

#include "..\..\managerDatas\aiManagerData_backgroundMover.h"

#include "..\..\..\modules\modulePresenceBackgroundMoverBase.h"
#include "..\..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"
#include "..\..\..\roomGenerators\roomGeneratorBase.h"
#include "..\..\..\sound\voiceoverSystem.h"

#include "..\..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\gameScript\gameScriptExecution.h"
#include "..\..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\..\framework\module\moduleAI.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\module\moduleSound.h"
#include "..\..\..\..\framework\object\object.h"
#include "..\..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\..\framework\world\presenceLink.h"
#include "..\..\..\..\framework\world\room.h"

#include "..\..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\..\core\random\randomUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#define AVOID_AIRSHIP 1
#define AVOID_OBSTACLE 2

#define TELEPORT_PRIORITY__AT_REST_PLACE 5
#define TELEPORT_PRIORITY__PUT_IN_PLACE 10
#define TELEPORT_PRIORITY__MOVE 20

//#define INSPECT_BACKGROUND_MOVER

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
using namespace Managers;

//

// ai messages
DEFINE_STATIC_NAME_STR(backgroundMover_reset, TXT("background mover; reset"));
DEFINE_STATIC_NAME_STR(backgroundMover_startMoving, TXT("background mover; start movement"));
DEFINE_STATIC_NAME_STR(backgroundMover_requestStop, TXT("background mover; request stop"));
DEFINE_STATIC_NAME_STR(backgroundMover_changeSpeed, TXT("background mover; change speed"));

// ai message params
DEFINE_STATIC_NAME(backgroundMoverName);
DEFINE_STATIC_NAME(triggerTrapWhenReachedEnd);
DEFINE_STATIC_NAME(endOnVoiceoverActor);
DEFINE_STATIC_NAME(endOnVoiceoverActorOffset);
DEFINE_STATIC_NAME(endWhenCanExitPilgrimagesStartingRoom);
DEFINE_STATIC_NAME(timeToStop);
DEFINE_STATIC_NAME(speed);
DEFINE_STATIC_NAME(forceSpeed);
DEFINE_STATIC_NAME(adjustSpeedTarget);
DEFINE_STATIC_NAME(applyOffset);

// object variables
DEFINE_STATIC_NAME(inRoom);
DEFINE_STATIC_NAME(backgroundMoverData);
DEFINE_STATIC_NAME(sizeForBackgroundMover);

// object tags
DEFINE_STATIC_NAME(backgroundMoverChance);

//

REGISTER_FOR_FAST_CAST(BackgroundMover);

BackgroundMover::BackgroundMover(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	random = _mind->get_owner_as_modules_owner()->get_individual_random_generator();
	random.advance_seed(2397, 9752904);
}

BackgroundMover::~BackgroundMover()
{
	output(TXT("background mover destroyed"));
}

LATENT_FUNCTION(BackgroundMover::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto

	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto * self = fast_cast<BackgroundMover>(logic);
	auto * imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(backgroundMover_reset), [self](Framework::AI::Message const& _message)
			{
				Transform restPlacement = self->backgroundMoverData->restPlacement;
				for_every(uo, self->usedObjects)
				{
					if (uo->inUse)
					{
						uo->inUse = false;
						if (auto* obj = uo->object.get())
						{
							auto* objp = obj->get_presence();
#ifdef INSPECT_BACKGROUND_MOVER
							output(TXT("move o%p to the rest placement (reset)"), get_mind()->get_owner_as_modules_owner());
#endif
							objp->request_teleport_within_room(restPlacement, Framework::ModulePresence::TeleportRequestParams().silent_teleport().with_teleport_priority(TELEPORT_PRIORITY__AT_REST_PLACE).be_visible(false));
						}
					}
				}
				self->applyOffset.clear();
				if (auto* param = _message.get_param(NAME(applyOffset)))
				{
					Transform useApplyOffset = param->get_as<Transform>();
					self->applyOffset = useApplyOffset;
				}
				self->reset_background_mover();
			}
		);
		messageHandler.set(NAME(backgroundMover_startMoving), [self](Framework::AI::Message const& _message)
			{
				bool moveThisOne = true;
				if (auto* param = _message.get_param(NAME(backgroundMoverName)))
				{
					Name backgroundMoverName = param->get_as<Name>();
					if (! self->backgroundMoverData.is_set() ||
						self->backgroundMoverData->get_name() != backgroundMoverName)
					{
						moveThisOne = false;
						ai_log(self, TXT("message to move [ignored]"));
					}
				}
				if (moveThisOne)
				{
					ai_log(self, TXT("message to move"));
					self->shouldStartMovement = true;
					self->triggerTrapWhenReachedEnd = Name::invalid();
					if (auto* param = _message.get_param(NAME(triggerTrapWhenReachedEnd)))
					{
						self->triggerTrapWhenReachedEnd = param->get_as<Name>();
					}
					self->endOnVoiceoverActor = Name::invalid();
					if (auto* param = _message.get_param(NAME(endOnVoiceoverActor)))
					{
						self->endOnVoiceoverActor = param->get_as<Name>();
						ai_log(self, TXT("end on voiceover actor \"%S\""), self->endOnVoiceoverActor.to_char());
					}
					self->endOnVoiceoverActorOffset = 0.0f;
					if (auto* param = _message.get_param(NAME(endOnVoiceoverActorOffset)))
					{
						self->endOnVoiceoverActorOffset = param->get_as<float>();
						ai_log(self, TXT("end on voiceover actor offset %.3f"), self->endOnVoiceoverActorOffset);
					}
					self->endWhenCanExitPilgrimagesStartingRoom = false;
					if (auto* param = _message.get_param(NAME(endWhenCanExitPilgrimagesStartingRoom)))
					{
						self->endWhenCanExitPilgrimagesStartingRoom = param->get_as<bool>();
						ai_log(self, TXT("end when can exit pilgrimages starting room %S"), self->endWhenCanExitPilgrimagesStartingRoom? TXT("") : TXT(", not"));
					}
					if (auto* param = _message.get_param(NAME(timeToStop)))
					{
						self->timeToStop = param->get_as<float>();
						ai_log(self, TXT("time to stop %.3f"), self->timeToStop);
					}
					if (auto* param = _message.get_param(NAME(adjustSpeedTarget)))
					{
						self->adjustSpeedTarget = param->get_as<bool>();
						ai_log(self, TXT("adjust speed target %S"), self->adjustSpeedTarget? TXT("YES") : TXT("no"));
					}
				}
			}
		);
		messageHandler.set(NAME(backgroundMover_requestStop), [self](Framework::AI::Message const& _message)
			{
				ai_log(self, TXT("request to stop"));
				self->shouldEnd = true;
				if (auto* param = _message.get_param(NAME(triggerTrapWhenReachedEnd)))
				{
					self->triggerTrapWhenReachedEnd = param->get_as<Name>();
				}
			}
		);
		messageHandler.set(NAME(backgroundMover_changeSpeed), [self](Framework::AI::Message const& _message)
			{
				ai_log(self, TXT("change speed"));
				if (auto* param = _message.get_param(NAME(speed)))
				{
					self->currentSpeedTarget = param->get_as<float>();
				}
				if (auto* param = _message.get_param(NAME(forceSpeed)))
				{
					self->forcedSpeed = param->get_as<float>();
				}
			}
		);
	}

	// get information where it does work
	self->inRoom = imo->get_variables().get_value<SafePtr<Framework::Room>>(NAME(inRoom), SafePtr<Framework::Room>());
	if (auto* r = self->inRoom.get())
	{
		Name backgroundMoverDataName = imo->get_variables().get_value<Name>(NAME(backgroundMoverData), Name::invalid());
		if (backgroundMoverDataName.is_valid())
		{
			if (auto* rb = fast_cast<RoomGenerators::Base>(r->get_room_generator_info()))
			{
				if (auto* bmd = rb->get_ai_managers().get_data<AI::ManagerDatasLib::BackgroundMover>(backgroundMoverDataName))
				{
					self->backgroundMoverData = bmd;
				}
			}
		}
		else
		{
			error(TXT("background mover in \"%S\" (\"%S\") does not have \"backgroundMoverData\" (Name) parameter provided, provide via aiManager aiMind parameters"),
				r->get_name().to_char(),
				r->get_room_type() ? r->get_room_type()->get_name().to_string().to_char() : TXT("--"));
		}
	}

	ai_log_colour(self, Colour::blue);
	ai_log(self, TXT("random \"%S\""), self->random.get_seed_string().to_char());
	ai_log_no_colour(self);

	if (!self->backgroundMoverData.is_set())
	{
		ai_log_colour(self, Colour::red);
		ai_log(self, TXT("no background mover data provided"));
		ai_log_no_colour(self);
		auto* r = self->inRoom.get();
		error(TXT("background mover in \"%S\" (\"%S\") does not have access to background mover data"),
			r? r->get_name().to_char() : TXT("--"),
			r && r->get_room_type() ? r->get_room_type()->get_name().to_string().to_char() : TXT("--"));
	}

	if (auto* ai = imo->get_ai())
	{
		auto& arla = ai->access_rare_logic_advance();
		arla.reset_to_no_rare_advancement();
	}

	if (self->backgroundMoverData.is_set() &&
		self->inRoom.is_set())
	{
		auto& tagged = self->backgroundMoverData->useObjectsTagged;
		auto& backgroundMoverBasePresenceOwners = self->backgroundMoverData->move.backgroundMoverBasePresenceOwnersTagged;
		auto& moveDoorsWithBackgroundTagged = self->backgroundMoverData->doors.moveWithBackgroundTagged;
		self->usedObjects.clear();
		self->sounds.clear();
		self->sounds.make_space_for(self->backgroundMoverData->sounds.get_size());
		self->backgroundMoverBasePresenceOwners.clear();
		for_every(ps, self->backgroundMoverData->sounds)
		{
			Sound s;
			s.playSound = ps->play;
			s.onSocket = ps->onSocket;
			self->sounds.push_back(s);
		}
		//FOR_EVERY_OBJECT_IN_ROOM(obj, self->inRoom.get())
		for_every_ptr(obj, self->inRoom->get_objects())
		{
			if (tagged.check(obj->get_tags()))
			{
				UsedObject uo;
				uo.object = obj;
				uo.inUse = false;
				uo.alongDirSize = obj->get_variables().get_value<float>(NAME(sizeForBackgroundMover), uo.alongDirSize);
				uo.chance = obj->get_tags().get_tag(NAME(backgroundMoverChance), 1.0f);
				obj->get_presence()->request_teleport_within_room(self->backgroundMoverData->restPlacement, Framework::ModulePresence::TeleportRequestParams().silent_teleport().with_teleport_priority(TELEPORT_PRIORITY__AT_REST_PLACE).be_visible(false)); 
				self->usedObjects.push_back(uo);
			}
			if (backgroundMoverBasePresenceOwners.check(obj->get_tags()))
			{
				self->backgroundMoverBasePresenceOwners.push_back(SafePtr<Framework::IModulesOwner>(obj));
			}
			for_every(ps, self->backgroundMoverData->sounds)
			{
				if (ps->onTagged.check(obj->get_tags()))
				{
					Sound::OnObject oo;
					oo.object = obj;
					oo.setSpeedVar = ps->setSpeedVar;
					self->sounds[for_everys_index(ps)].onObjects.push_back(oo);
				}
			}
		}
		self->tempObjects.make_space_for(self->usedObjects.get_size());
		self->usedDoors.clear();
		//FOR_EVERY_DOOR_IN_ROOM(dir, self->inRoom.get())
		for_every_ptr(dir, self->inRoom->get_doors())
		{
			todo_note(TXT("process invisible doors ?"));
			UsedDoor ud;
			ud.door = dir;
			ud.order = for_everys_index(dir) * self->backgroundMoverData->doors.order;
			if (self->backgroundMoverData->doors.alongOpenWorldDir.is_set())
			{
				ud.order = PilgrimageInstanceOpenWorld::get_door_distance_in_dir(dir, self->backgroundMoverData->doors.alongOpenWorldDir.get());
			}
			if (! self->backgroundMoverData->doors.orderByTagged.is_empty())
			{
				for_every(obt, self->backgroundMoverData->doors.orderByTagged)
				{
					if (obt->check(dir->get_tags()))
					{
						ud.order = - for_everys_index(obt); /* by highest order value*/
					}
				}
			}
			if (!moveDoorsWithBackgroundTagged.is_empty())
			{
				ud.moveWithBackground = moveDoorsWithBackgroundTagged.check(dir->get_tags());
			}
			for_every(pdapoi, self->backgroundMoverData->doors.placeDoorsAtPOI)
			{
				if (pdapoi->tagged.check(dir->get_tags()))
				{
					ud.moveWithBackground = false;
					ud.placeAtPOINamed = pdapoi->atPOI;
					Framework::PointOfInterestInstance* foundPOI;
					if (self->inRoom->find_any_point_of_interest(ud.placeAtPOINamed, foundPOI, true))
					{
						ud.placeAtPOI = foundPOI;
					}
					an_assert(ud.placeAtPOI.get(), TXT("not found, if you're using one POI, note that after spawning a door it will get cancelled, use two POIs"));
				}
			}

			self->usedDoors.push_back(ud);
		}
		sort(self->usedDoors);
		if (! self->backgroundMoverData->doors.noStartingDoor)
		{
			for_every(ud, self->usedDoors)
			{
				if (!ud->moveWithBackground &&
					! ud->placeAtPOINamed.is_valid())
				{
					ud->startingDoor = true;
					break;
				}
			}
		}
		self->chainAnchorPOIPlacement = Transform::identity;
		if (self->backgroundMoverData->chain.anchorPOI.is_valid())
		{
			Framework::PointOfInterestInstance* foundPOI;
			if (self->inRoom->find_any_point_of_interest(self->backgroundMoverData->chain.anchorPOI, foundPOI))
			{
				self->chainAnchorPOIPlacement = foundPOI->calculate_placement();
			}
		}
		self->doorAnchorPOIPlacement = Transform::identity;
		if (self->backgroundMoverData->doors.anchorPOI.is_valid())
		{
			Framework::PointOfInterestInstance* foundPOI;
			if (self->inRoom->find_any_point_of_interest(self->backgroundMoverData->doors.anchorPOI, foundPOI))
			{
				self->doorAnchorPOIPlacement = foundPOI->calculate_placement();
			}
		}
		self->started = true;
		self->movedDoorsOnStart = false;
		self->reset_background_mover();
#ifdef INSPECT_BACKGROUND_MOVER
		{
			LogInfoContext l;
			self->log_logic_info(l);
			l.log(TXT("background mover o%p initialised"), imo);
			l.output_to_output();
		}
#endif
	}

	while (true)
	{
		LATENT_WAIT_NO_RARE_ADVANCE(Random::get_float(0.1f, 0.5f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

void BackgroundMover::reset_background_mover()
{
	shouldStartMovement = backgroundMoverData->move.autoStart;
	immediateFullSpeed = shouldStartMovement;
	{
		speed = backgroundMoverData->move.speed.get(inRoom.get(), 0.0f, ShouldAllowToFail::AllowToFail);
	}
	if (backgroundMoverData->move.curveRadius.is_set())
	{
		curveRadius = backgroundMoverData->move.curveRadius.get(inRoom.get(), 0.0f, ShouldAllowToFail::AllowToFail);
		if (curveRadius.get(0.0f) == 0.0f)
		{
			curveRadius.clear();
		}
	}
	forcedSpeed.clear();
	processChain.at.clear();
	processChain.repeatCount.clear();
	appearDistance = backgroundMoverData->chain.appearDistance;
	appearDistanceOffset = 0.0f;
	if (applyOffset.is_set())
	{
		offsetChainAnchorPOIPlacement = chainAnchorPOIPlacement.to_world(applyOffset.get());
		offsetDoorAnchorPOIPlacement = doorAnchorPOIPlacement.to_world(applyOffset.get());
	}
	else
	{
		offsetChainAnchorPOIPlacement = chainAnchorPOIPlacement;
		offsetDoorAnchorPOIPlacement = doorAnchorPOIPlacement;
	}
	advance_and_apply(0.0f);
}

void BackgroundMover::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (!started)
	{
		return;
	}

	advance_and_apply(_deltaTime);
}

Vector3 BackgroundMover::calculate_move_dir() const
{
	Vector3 useMoveDir = backgroundMoverData->move.dir;
	if (applyOffset.is_set())
	{
		useMoveDir = chainAnchorPOIPlacement.vector_to_local(useMoveDir);
		useMoveDir = offsetChainAnchorPOIPlacement.vector_to_world(useMoveDir);
	}
	return useMoveDir;
}

void BackgroundMover::advance_and_apply(float _deltaTime)
{
	for_every(uo, usedObjects)
	{
		if (uo->inUse)
		{
			if (auto* obj = uo->object.get())
			{
				auto* objp = obj->get_presence();
				if (objp->does_request_teleport())
				{
					// if we can't move an object, don't move anything, wait here
					_deltaTime = 0.0f;
					break;
				}
			}
		}
	}

	bool moveDoors = false;
	if (!movedDoorsOnStart)
	{
		if (backgroundMoverData->doors.moveDoors)
		{
			for_every(door, usedDoors)
			{
				if (!door->moveWithBackground &&
					!door->placeAtPOINamed.is_valid())
				{
					door->atDistance = door->startingDoor ? 0.0f : get_current_appear_distance() + 1.0f;
				}
			}
			moveDoors = true;
		}
		movedDoorsOnStart = true;
	}

	if (shouldStartMovement &&
		(!endAtDistance.is_set() || endAtDistance.get() > 0.001f))
	{
		shouldStartMovement = false;
		on_start_movement();
	}

	if (forcedSpeed.is_set())
	{
		if (currentSpeed != forcedSpeed.get())
		{
			currentSpeed = forcedSpeed.get();
			on_speed_update();
		}
	}
	else
	{
		if (adjustSpeedTarget && endAtDistance.is_set() &&
			currentSpeedTarget != 0.0f)
		{
			if (endOnVoiceoverActor.is_valid())
			{
				if (auto* vos = VoiceoverSystem::get())
				{
					float timeRequired = vos->get_time_to_end_speaking(endOnVoiceoverActor) + endOnVoiceoverActorOffset;
					currentSpeedTarget = endAtDistance.get() / max(0.01f, timeRequired);
				}
			}
			adjustSpeedTarget = false; // adjusted (or not)
		}

		if (!adjustSpeedTarget) // first adjust the speed
		{
			Optional<float> useAccel = backgroundMoverData->move.acceleration;
			Optional<float> useDecel = backgroundMoverData->move.deceleration;

			if (!useDecel.is_set())
			{
				useDecel = useAccel;
			}

			if (useDecel.is_set() && useDecel.get() != 0.0f && endAtDistance.is_set())
			{
				// order to stop
				float distanceLeft = endAtDistance.get();
				float timeToStop = currentSpeed / useDecel.get();
				//float distanceCovered = currentSpeed * timeToStop - useDecel.get() * sqr(timeToStop) / 2.0f;
				timeToStop += _deltaTime;
				float distanceCoveredInc = currentSpeed * timeToStop - useDecel.get() * sqr(timeToStop) / 2.0f;
				if (distanceCoveredInc > distanceLeft)
				{
					currentSpeedTarget = 0.0f;
				}
			}

			if (currentSpeed < currentSpeedTarget)
			{
				if (immediateFullSpeed)
				{
					immediateFullSpeed = false;
					currentSpeed = currentSpeedTarget;
				}
				else if (useAccel.is_set())
				{
					currentSpeed = min(currentSpeedTarget, currentSpeed + useAccel.get() * _deltaTime);
				}
				else
				{
					currentSpeed = currentSpeedTarget;
				}
				on_speed_update();
			}
			if (currentSpeed > currentSpeedTarget)
			{
				if (useDecel.is_set())
				{
					currentSpeed = max(speed * 0.02f, currentSpeed - useDecel.get() * _deltaTime);
				}
				else
				{
					currentSpeed = currentSpeedTarget;
				}
				on_speed_update();
			}
		}
	}

	float moveByDistance = currentSpeed * _deltaTime;
	distanceCovered += moveByDistance;
	appearDistanceOffset = max(0.0f, appearDistanceOffset - moveByDistance);
	if (endAtDistance.is_set())
	{
		moveByDistance = min(moveByDistance, endAtDistance.get());
		if (endAtDistance.get() < 0.001f)
		{
			// stop!
			on_stop_movement();
		}
	}

	float useAppearDistance = get_current_appear_distance();

	Vector3 useMoveDir = calculate_move_dir();

	// move these always at POI
	if (!backgroundMoverData->doors.placeDoorsAtPOI.is_empty())
	{
		Vector3 moveDoorsBy = -useMoveDir * moveByDistance;
		for_every(ud, usedDoors)
		{
			if (ud->placeAtPOINamed.is_valid())
			{
				if (auto* dir = ud->door.get())
				{
					if (auto* poi = ud->placeAtPOI.get())
					{
						Transform placement = poi->calculate_placement();
						placement.set_translation(placement.get_translation() + moveDoorsBy);
						dir->request_placement(placement);
					}
				}
			}
		}
	}

	{
		// make sure objects are where they should be
		Transform restPlacement = backgroundMoverData->restPlacement;
		for_every(uo, usedObjects)
		{
			if (! uo->inUse)
			{
				if (auto* obj = uo->object.get())
				{
					auto* objp = obj->get_presence();
					if (!objp->does_request_teleport())
					{
						Transform placement = objp->get_placement();
						if (! Transform::same_with_orientation(placement, restPlacement))
						{
#ifdef INSPECT_BACKGROUND_MOVER
							output(TXT("move o%p to the rest placement (as it is not there)"), get_mind()->get_owner_as_modules_owner());
#endif
							warn_dev_investigate(TXT("object used by backgroundMover is not at the restPlacement!"));
							objp->request_teleport_within_room(restPlacement, Framework::ModulePresence::TeleportRequestParams().silent_teleport().with_teleport_priority(TELEPORT_PRIORITY__AT_REST_PLACE).be_visible(false));
						}
					}
				}
			}
#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (auto* obj = uo->object.get())
			{
				auto* objp = obj->get_presence();
				an_assert_log_always(! objp->get_presence_links() || Transform::same_with_orientation(objp->get_presence_links()->get_placement_in_room(), objp->get_placement()),
					TXT("placement mismatch v presence link or presence link missing"));
			}
#endif
		}
	}

	Vector3 objectsVelocity = -useMoveDir * currentSpeed;
	if (moveByDistance != 0.0f)
	{
		atDistance -= moveByDistance;
		if (endAtDistance.is_set())
		{
			endAtDistance = endAtDistance.get() - moveByDistance;
		}
		if (shouldEnd && !endAtDistance.is_set() &&
			!backgroundMoverData->chain.containsEndPlacement &&
			atDistance >= useAppearDistance) // only if far away
		{
			endAtDistance = atDistance;
		}

		Vector3 moveObjectsBy = -useMoveDir * moveByDistance;


		for_every(uo, usedObjects)
		{
			if (uo->inUse)
			{
				if (auto* obj = uo->object.get())
				{
#ifdef AN_DEVELOPMENT_OR_PROFILER
					if (auto* o = obj->get_as_object())
					{
						if (auto* ot = o->get_object_type())
						{
							if (ot->should_advance_once())
							{
								error_dev_investigate(TXT("objects moveable by background mover should be advanceable, remove <advanceOnce/> from \"%S\""), ot->get_name().to_string().to_char());
							}
						}
					}
#endif
					auto* objp = obj->get_presence();
					Transform placement = objp->get_placement();
					objp->get_request_teleport_details(nullptr, &placement); // maybe we're ordered to teleport to the play area, offset teleport then (if we're not teleporting, nothing will happen here)
					if (curveRadius.is_set())
					{
						placement = uo->placement;
					}
					placement.set_translation(placement.get_translation() + moveObjectsBy);
					Vector3 anchorPOI = offsetChainAnchorPOIPlacement.get_translation();
					float alongDirDist = Vector3::dot(placement.get_translation() - anchorPOI, useMoveDir);
					if (alongDirDist + uo->alongDirSize < backgroundMoverData->chain.disappearDistance)
					{
#ifdef INSPECT_BACKGROUND_MOVER
						output(TXT("deactivate o%p"), uo->object.get());
#endif
						uo->inUse = false;
						placement = backgroundMoverData->restPlacement;
#ifdef INSPECT_BACKGROUND_MOVER
						output(TXT("move o%p into \"rest\" placement as it went through disappear distance"), obj);
#endif
					}
					uo->placement = placement;
					if (uo->inUse)
					{
						offset_placement_on_curve(placement, anchorPOI, useMoveDir);
					}
					objp->request_teleport_within_room(placement, Framework::ModulePresence::TeleportRequestParams().silent_teleport().with_teleport_priority(uo->inUse ? TELEPORT_PRIORITY__MOVE : TELEPORT_PRIORITY__AT_REST_PLACE).be_visible(true));
					objp->set_velocity_linear(objectsVelocity);
				}
			}
		}

		if (backgroundMoverData->doors.moveDoors)
		{
			for_every(ud, usedDoors)
			{
				if (!ud->moveWithBackground &&
					!ud->placeAtPOINamed.is_valid())
				{
					if (ud->startingDoor)
					{
						ud->atDistance = max(ud->atDistance - moveByDistance, backgroundMoverData->chain.disappearDistance - 10.0f);
					}
					else
					{
						if (endAtDistance.is_set())
						{
							ud->atDistance = endAtDistance.get();
						}
						else
						{
							// stay beyond range
							ud->atDistance = useAppearDistance + 1.0f;
						}
					}
					moveDoors = true;
				}
			}
		}

		if (!backgroundMoverData->doors.moveWithBackgroundTagged.is_empty())
		{
			for_every(ud, usedDoors)
			{
				if (ud->moveWithBackground)
				{
					if (auto* dir = ud->door.get())
					{
						Transform placement = dir->get_placement();
						placement.set_translation(placement.get_translation() + moveObjectsBy);
						dir->request_placement(placement);
					}
				}
			}
		}
	}

	if (backgroundMoverData->doors.moveDoors && moveDoors)
	{
		for_every(door, usedDoors)
		{
			if (!door->moveWithBackground &&
				!door->placeAtPOINamed.is_valid())
			{
				if (auto* dir = door->door.get())
				{
					Transform placement = offsetDoorAnchorPOIPlacement;
					placement.set_translation(placement.get_translation() + door->atDistance * useMoveDir);
					dir->request_placement(placement);
				}
			}
		}
	}

	if (timeToStop.is_set() &&
		!shouldEnd)
	{
		timeToStop = timeToStop.get() - _deltaTime;
	}

	// add new if required
	{
		while (true)
		{
			{
				if (endOnVoiceoverActor.is_valid() &&
					!shouldEnd)
				{
					if (auto* vos = VoiceoverSystem::get())
					{
						if (vos->get_time_to_end_speaking(endOnVoiceoverActor) + endOnVoiceoverActorOffset < (atDistance / speed))
						{
							shouldEnd = true;
						}
					}
				}

				if (timeToStop.is_set() &&
					!shouldEnd)
				{
					float dist = atDistance / speed;
#ifdef AN_DEVELOPMENT_OR_PROFILER
					dist = distanceCoveredToTimeToStop.get(dist);
#endif
					if (timeToStop.get() < dist)
					{
#ifdef AN_DEVELOPMENT_OR_PROFILER
						distanceCoveredToTimeToStop = dist;
#endif
						shouldEnd = true;
					}
				}

				if (endWhenCanExitPilgrimagesStartingRoom)
				{
					if (auto* piow = PilgrimageInstanceOpenWorld::get())
					{
						if (piow->can_exit_starting_room())
						{
							if (!endOnVoiceoverActor.is_valid() &&
								!timeToStop.is_set())
							{
								// the only condition
								shouldEnd = true;
							}
						}
						else
						{
							// we are more of a don't do it unless we can exit
							shouldEnd = false;
						}
					}
				}
			}

			bool stopProcessing = false;
			if (auto const* chEl = backgroundMoverData->chain.root.get())
			{
				if (processChain.at.is_empty())
				{
					// start!
					processChain.at.push_back(0);
					processChain.repeatCount.push_back(0);
				}
				bool processChElNow = true;
				// find element to process
				for (int d = 0; d < ProcessChain::MAX_DEPTH && d < processChain.at.get_size(); ++d)
				{
					if (chEl->elements.is_index_valid(processChain.at[d]))
					{
						chEl = chEl->elements[processChain.at[d]].get();
						if (!chEl)
						{
							error(TXT("problem processing chain - no element - but should be"));
							processChain.at.clear();
							processChain.repeatCount.clear();
							stopProcessing = true;
							break;
						}
					}
					else
					{
						bool repeatThisOne = false;
						if (chEl->type == ManagerDatasLib::BackgroundMover::ChainElement::Repeat)
						{
							++processChain.repeatCount[d];
							if (!chEl->amount.is_set() ||
								processChain.repeatCount[d] < chEl->amount.get())
							{
								repeatThisOne = true;
							}
						}
						if (chEl->type == ManagerDatasLib::BackgroundMover::ChainElement::RepeatTillEnd)
						{
							repeatThisOne = !shouldEnd;
						}
						if (repeatThisOne)
						{
							processChain.at.set_size(d + 1);
							processChain.repeatCount.set_size(d + 1);
							processChain.at[d] = 0; // start again
						}
						else
						{
							// end this one and move to the next one
							processChain.at.set_size(d);
							processChain.repeatCount.set_size(d);
							if (d >= 0)
							{
								--d;
								++processChain.at[d];
								processChain.repeatCount[d] = 0;
							}
							else
							{
								// we reached end?
								stopProcessing = true;
							}
						}
						processChElNow = false; // we should go back/again to find the right chEl
						break;
					}
				}
				if (processChElNow)
				{
					if (chEl->type == ManagerDatasLib::BackgroundMover::ChainElement::End)
					{
						// just stay here
						stopProcessing = true;
						atDistance = useAppearDistance + 1.0f;
					}
					else
					{
						bool advanceChainElement = true;
						switch (chEl->type)
						{
						case ManagerDatasLib::BackgroundMover::ChainElement::Idle:
							stopProcessing = true;
							advanceChainElement = false;
							break;
						case ManagerDatasLib::BackgroundMover::ChainElement::AtZero:
							atDistance = 0.0f;
							break;
						case ManagerDatasLib::BackgroundMover::ChainElement::AtAppearDistance:
							atDistance = backgroundMoverData->chain.appearDistance;
							break;
						case ManagerDatasLib::BackgroundMover::ChainElement::AtDisappearDistance:
							atDistance = backgroundMoverData->chain.disappearDistance;
							break;
						case ManagerDatasLib::BackgroundMover::ChainElement::MoveBy:
							atDistance += random.get(chEl->valueR);
							break;
						case ManagerDatasLib::BackgroundMover::ChainElement::AddAndMove:
						case ManagerDatasLib::BackgroundMover::ChainElement::AddAndStay:
							if (atDistance >= useAppearDistance && !chEl->force
								&& (!adjustSpeedTarget || endAtDistance.is_set())/* keep going till end is found*/)
							{
								stopProcessing = true;
								advanceChainElement = false;
							}
							else
							{
								tempObjects.clear();
								for_every(uo, usedObjects)
								{
									if (!uo->inUse &&
										uo->object.get())
									{
										if (auto* obj = uo->object->get_as_object())
										{
											if (chEl->tagged.check(obj->get_tags()))
											{
												tempObjects.push_back(TempObject(for_everys_index(uo), uo->chance));
											}
										}
									}
								}
								if (!tempObjects.is_empty())
								{
									int chosenIdx = RandomUtils::ChooseFromContainer<Array<TempObject>, TempObject>::choose(
										random, tempObjects, [](TempObject const& _e) { return _e.chance; });
									an_assert(tempObjects.is_index_valid(chosenIdx));

									auto& to = tempObjects[chosenIdx];
									an_assert(usedObjects.is_index_valid(to.uoIdx));
									auto& uo = usedObjects[to.uoIdx];

#ifdef INSPECT_BACKGROUND_MOVER
									output(TXT("activate o%p"), uo.object.get());
#endif
									uo.inUse = true;
									if (auto* obj = uo.object.get())
									{
										auto* objp = obj->get_presence();

										Transform placement = offsetChainAnchorPOIPlacement;
										placement.set_translation(placement.get_translation() + useMoveDir * atDistance);

										{
											Vector3 locOffset = Vector3::zero;
											locOffset.x = random.get(chEl->transformOffset.x);
											locOffset.y = random.get(chEl->transformOffset.y);
											locOffset.z = random.get(chEl->transformOffset.z);
											Rotator3 rotOffset = Rotator3::zero;
											rotOffset.pitch = random.get(chEl->transformOffset.pitch);
											rotOffset.yaw = random.get(chEl->transformOffset.yaw);
											rotOffset.roll = random.get(chEl->transformOffset.roll);
											Transform offset = Transform(locOffset, rotOffset.to_quat());
											placement = placement.to_world(offset);
										}

										uo.placement = placement;
										offset_placement_on_curve(placement, offsetChainAnchorPOIPlacement.get_translation(), useMoveDir);
										objp->request_teleport_within_room(placement, Framework::ModulePresence::TeleportRequestParams().silent_teleport().with_teleport_priority(TELEPORT_PRIORITY__PUT_IN_PLACE).be_visible());
										objp->set_velocity_linear(objectsVelocity);

										ai_log(this, TXT("at %.3f (%.3f) added o%p \"%S\""), atDistance, distanceCovered + atDistance, obj, obj->get_as_object() ? obj->get_as_object()->get_object_type()->get_name().to_string().to_char() : obj->ai_get_name().to_char());
#ifdef INSPECT_BACKGROUND_MOVER
										output(TXT("move o%p into \"active\" placement"), obj);
#endif
									}

									if (chEl->endPlacementInCentre)
									{
										if (!endAtDistance.is_set())
										{
											endAtDistance = atDistance + uo.alongDirSize * 0.5f;
										}
									}

									if (chEl->type == ManagerDatasLib::BackgroundMover::ChainElement::AddAndMove)
									{
										atDistance += uo.alongDirSize;
									}
								}
								else
								{
									if (!adjustSpeedTarget)
									{
										error_dev_ignore(TXT("no objects available for background mover tagged \"%S\" add/spawn via roomGeneratorInfo's \"spawn\"!"), chEl->tagged.to_string().to_char());
									}
									stopProcessing = true;
								}
							}
							break;
						case ManagerDatasLib::BackgroundMover::ChainElement::EndPlacement:
							if (!endAtDistance.is_set())
							{
								endAtDistance = atDistance;
							}
							break;
						case ManagerDatasLib::BackgroundMover::ChainElement::SetAppearDistance:
							appearDistanceLimit = useAppearDistance;
							appearDistance = chEl->value;
							appearDistanceOffset = max(0.0f, atDistance - appearDistance);
							if (appearDistance >= appearDistanceLimit.get())
							{
								// growing distance, we should open as quickly as possible
								appearDistanceLimit.clear();
							}
							break;
						default:
							break;
						}
						if (advanceChainElement)
						{
							if (!chEl->elements.is_empty())
							{
								// start processing children
								processChain.at.push_back(0);
								processChain.repeatCount.push_back(0);
								// if we run out of children, we will go to the next one
							}
							else
							{
								// no children, jump straight to the next one at this depth
								++processChain.at.get_last();
							}
						}
#ifdef INSPECT_BACKGROUND_MOVER
						{
							LogInfoContext l;
							log_logic_info(l);
							l.log(TXT("processed element o%p"), get_mind()->get_owner_as_modules_owner());
							l.output_to_output();
						}
#endif
					}
				}
			}
			else
			{
				processChain.at.clear();
				processChain.repeatCount.clear();
				stopProcessing = true;
			}

			if (stopProcessing)
			{
				break;
			}
		}
	}

	if (backgroundMoverData->setDistanceCovered.roomVar.is_valid())
	{
		if (auto* r = inRoom.get())
		{
			if (auto* toVar = r->access_variables().access_existing<float>(backgroundMoverData->setDistanceCovered.roomVar))
			{
				*toVar = distanceCovered * backgroundMoverData->setDistanceCovered.mul;
			}
			else
			{
				error(TXT("room \"%S\" does not have a variable \"%S\" [float] available to alter"), r->get_name().to_char(), backgroundMoverData->setDistanceCovered.roomVar.to_char());
			}
		}
	}

	for_every(s, sounds)
	{
		for_every(oo, s->onObjects)
		{
			if (oo->setSpeedVar.is_valid())
			{
				if (auto* o = oo->object.get())
				{
					MODULE_OWNER_LOCK_FOR_IMO(o, TXT("set sound variables from background mover"));
					o->access_variables().access<float>(oo->setSpeedVar) = currentSpeed;
				}
			}
		}
	}
}

void BackgroundMover::on_speed_update()
{
	if (backgroundMoverData.is_set() &&
		backgroundMoverData->move.setRoomVelocityCoef.is_set())
	{
		if (auto* r = inRoom.get())
		{
			r->set_room_velocity(backgroundMoverData->move.setRoomVelocityCoef.get() * currentSpeed * calculate_move_dir());
		}
	}
}

void BackgroundMover::offset_placement_on_curve(REF_ Transform& _placement, Vector3 const& _anchorPOI, Vector3 const& _dir) const
{
	if (curveRadius.is_set())
	{
		Vector3 loc = _placement.get_translation();

		Vector3 dir = _dir;
		Vector3 intoCurve = dir.rotated_right();
		float radius = curveRadius.get();
		if (radius < 0.0f)
		{
			radius = -radius;
			intoCurve = -intoCurve;
			dir = -dir;
		}

		float alongLine = Vector3::dot(loc - _anchorPOI, dir);
		float fullCircleLength = 2.0f * pi<float>() * abs(radius);
		float alongFullCirclePt = alongLine / fullCircleLength;
		float angle = alongFullCirclePt * 360.0f;

		float rightOffset = radius - (float)(cos_deg<double>(angle) * (double)radius);
		float forwardOffset = (float)(sin_deg<double>(angle) * (double)radius) - alongLine;
		loc += intoCurve * rightOffset;
		loc += dir * forwardOffset;
		_placement.set_translation(loc);
		_placement.set_orientation((_placement.get_orientation().to_rotator() + Rotator3(0.0f, angle, 0.0f)).to_quat());
	}
}

void BackgroundMover::on_start_movement()
{
	if (currentSpeedTarget != 0.0f)
	{
		return;
	}
	ai_log(this, TXT("start movement"));
	currentSpeedTarget = speed;
	for_every(s, sounds)
	{
		for_every(oo, s->onObjects)
		{
			if (auto* o = oo->object.get())
			{
				if (auto* sm = o->get_sound())
				{
					Framework::SoundSourcePtr ssp;
					ssp = sm->play_sound(s->playSound, s->onSocket);
					s->playedSounds.push_back(ssp);
				}
			}
		}
	}
	for_every_ref(imo, backgroundMoverBasePresenceOwners)
	{
		if (auto* p = fast_cast<ModulePresenceBackgroundMoverBase>(imo->get_presence()))
		{
			p->set_moving_for_background_mover(true);
		}
	}
}

void BackgroundMover::on_stop_movement()
{
	if (currentSpeed == 0.0f)
	{
		return;
	}
	ai_log(this, TXT("stop movement"));
	currentSpeed = 0.0f;
	currentSpeedTarget = 0.0f;
	on_speed_update();
	if (triggerTrapWhenReachedEnd.is_valid())
	{
		Framework::GameScript::ScriptExecution::trigger_execution_trap(triggerTrapWhenReachedEnd);
	}
	for_every(s, sounds)
	{
		for_every_ref(ps, s->playedSounds)
		{
			ps->stop();
		}
		s->playedSounds.clear();
	}
	for_every_ref(imo, backgroundMoverBasePresenceOwners)
	{
		if (auto* p = fast_cast<ModulePresenceBackgroundMoverBase>(imo->get_presence()))
		{
			p->set_moving_for_background_mover(false);
		}
	}
}

void BackgroundMover::debug_draw(Framework::Room* _room) const
{
#ifdef AN_DEBUG_RENDERER
	debug_context(_room);

	debug_no_context();
#endif
}

void BackgroundMover::log_logic_info(LogInfoContext& _log) const
{
	base::log_logic_info(_log);

	if (backgroundMoverData.get())
	{
		_log.log(TXT("name %S"), backgroundMoverData->get_name());
	}
	if (started)
	{
		_log.log(TXT("current speed      %8.3f"), currentSpeed);
		if (forcedSpeed.is_set())
		{
			_log.log(TXT("forced speed       %8.3f"), forcedSpeed.get());
		}
		_log.log(TXT("current speed tgt  %8.3f"), currentSpeedTarget);
		_log.log(TXT("should end         %S"), shouldEnd? TXT("     yes") : TXT("      no"));
		if (endOnVoiceoverActor.is_valid())
		{
			bool ok = false;
			if (auto* vos = VoiceoverSystem::get())
			{
				ok = vos->get_time_to_end_speaking(endOnVoiceoverActor) + endOnVoiceoverActorOffset < (atDistance / speed);
			}
			_log.set_colour(ok ? Colour::green : Colour::red);
			_log.log(TXT("  end on vo actor \"%S\" + %.2f"), endOnVoiceoverActor.to_char(), endOnVoiceoverActorOffset);
			_log.set_colour();
		}
		if (timeToStop.is_set())
		{
			float dist = atDistance / speed;
#ifdef AN_DEVELOPMENT_OR_PROFILER
			dist = distanceCoveredToTimeToStop.get(dist);
#endif
			_log.set_colour(timeToStop.get() <= dist ? Colour::green : Colour::red);
			_log.log(TXT("  time to stop     %8.3f - %8.3f = %8.3f"), timeToStop.get(), dist, timeToStop.get() - dist);
			_log.set_colour();
		}
		if (endWhenCanExitPilgrimagesStartingRoom)
		{
			bool ok = false;
			if (auto* piow = PilgrimageInstanceOpenWorld::get())
			{
				ok = piow->can_exit_starting_room();
			}
			_log.set_colour(ok ? Colour::green : Colour::red);
			_log.log(TXT("  end when can exit pilgrimages starting room (%S)"), ok ? TXT("ok to end") : TXT("wait"));
			_log.set_colour();
		}
		_log.log(TXT(""));
		_log.log(TXT("at distance        %8.3f"), atDistance);
		_log.log(TXT("distance covered   %8.3f"), distanceCovered);
		if (endAtDistance.is_set())
		{
			_log.log(TXT("end at distance    %8.3f"), endAtDistance.get());
		}
		else
		{
			_log.log(TXT("end at distance"));
		}
		_log.log(TXT(""));
		_log.log(TXT("appear distance    %8.3f"), get_current_appear_distance());
		_log.log(TXT("  base             %8.3f"), appearDistance);
		_log.log(TXT("  + offset         %8.3f"), appearDistanceOffset);
		if (appearDistanceLimit.is_set())
		{
			_log.log(TXT("  limit to         %8.3f"), appearDistanceLimit.get());
		}
		else
		{
			_log.log(TXT("  don't limit"));
		}
		_log.log(TXT("disappear distance %8.3f"), backgroundMoverData->chain.disappearDistance);
		_log.log(TXT(""));

		{
			struct Ordered
			{
				float atDistance = 0.0f;
				Framework::IModulesOwner* obj;

				static int compare(void const* _a, void const* _b)
				{
					Ordered const* a = plain_cast<Ordered>(_a);
					Ordered const* b = plain_cast<Ordered>(_b);
					if (a->atDistance < b->atDistance) return A_BEFORE_B;
					if (a->atDistance > b->atDistance) return B_BEFORE_A;
					return A_AS_B;
				}
			};
			Array<Ordered> ordered;

			_log.log(TXT("used objects       %8i"), usedObjects.get_size());
			int activeCount = 0;
			for_every(uo, usedObjects)
			{
				if (uo->inUse)
				{
					++activeCount;
					Ordered o;
					o.obj = uo->object.get();
					o.atDistance = 0.0f;
					if (o.obj)
					{
						if (auto* p = o.obj->get_presence())
						{
							Vector3 relLoc = p->get_placement().get_translation() - offsetChainAnchorPOIPlacement.get_translation();
							o.atDistance = Vector3::dot(relLoc, calculate_move_dir());
						}
					}
					ordered.push_back(o);
				}
			}
			sort(ordered);
			_log.log(TXT("  active           %8i"), activeCount);
			{
				for_every(o, ordered)
				{
					_log.log(TXT("    o%p %12.3f %S"), o->obj, o->atDistance, !o->obj? TXT("--") :
						(o->obj->get_as_object() ? o->obj->get_as_object()->get_object_type()->get_name().to_string().to_char() : o->obj->ai_get_name().to_char()));
#ifdef INSPECT_BACKGROUND_MOVER
					_log.log(TXT("          | loc %S"), o->obj ? o->obj->get_presence()->get_placement().get_translation().to_string().to_char() : TXT("--"));
					_log.log(TXT("          | mesh m%p"), o->obj && o->obj->get_appearance()->get_mesh() ? o->obj->get_appearance()->get_mesh() : 0);
#endif
				}
			}
		}

	}
	else
	{
		_log.set_colour(Colour::red);
		_log.log(TXT("couldn't start"));
		_log.set_colour();
	}
}

float BackgroundMover::get_current_appear_distance() const
{
	float useAppearDistance = appearDistanceOffset + appearDistance;
	useAppearDistance = min(useAppearDistance, appearDistanceLimit.get(useAppearDistance));
	return useAppearDistance;
}

