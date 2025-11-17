#include "moduleMovementPilgrimKeeper.h"

#include "..\game\game.h"
#include "..\game\gameDirector.h"
#include "..\game\playerSetup.h"

#include "..\..\framework\advance\advanceContext.h"

#include "..\..\framework\module\moduleAppearance.h"
#include "..\..\framework\module\modules.h"
#include "..\..\framework\module\registeredModule.h"

#include "..\..\framework\object\actor.h"

#include "..\..\framework\world\doorInRoom.h"
#include "..\..\framework\world\room.h"

#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\vr\iVR.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// module parameters
DEFINE_STATIC_NAME_AS(mp_maxDistance, maxDistance);
DEFINE_STATIC_NAME_AS(mp_maxDistanceWarn, maxDistanceWarn);

// door tags
DEFINE_STATIC_NAME_AS(dt_impassable, impassable);

//

REGISTER_FOR_FAST_CAST(ModuleMovementPilgrimKeeper);

::System::TimeStamp ModuleMovementPilgrimKeeper::s_disabled;
float ModuleMovementPilgrimKeeper::s_disabledLimit = 0.0f;

static Framework::ModuleMovement* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleMovementPilgrimKeeper(_owner);
}

Framework::RegisteredModule<Framework::ModuleMovement> & ModuleMovementPilgrimKeeper::register_itself()
{
	return Framework::Modules::movement.register_module(String(TXT("pilgrimKeeper")), create_module);
}

bool ModuleMovementPilgrimKeeper::is_disabled()
{
	if (s_disabledLimit > 0.0f && s_disabled.get_time_since() < s_disabledLimit)
	{
		return true;
	}
	if (GameDirector::get()->is_keep_in_world_temporarily_disabled())
	{
		return true;
	}
	return false;
}

void ModuleMovementPilgrimKeeper::disable_for(float _time)
{
	s_disabled.reset();
	s_disabledLimit = _time;
}

ModuleMovementPilgrimKeeper::ModuleMovementPilgrimKeeper(Framework::IModulesOwner* _owner)
: base(_owner)
{
	requiresPrepareMove = false;
}

ModuleMovementPilgrimKeeper::~ModuleMovementPilgrimKeeper()
{
	set_pilgrim(nullptr);
}

void ModuleMovementPilgrimKeeper::setup_with(Framework::ModuleData const* _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		maxDistance = _moduleData->get_parameter<float>(this, NAME(mp_maxDistance), maxDistance);
		maxDistanceWarn = _moduleData->get_parameter<float>(this, NAME(mp_maxDistanceWarn), maxDistanceWarn);
	}
}

void ModuleMovementPilgrimKeeper::activate()
{
	pathToPilgrim.set_owner(get_owner());
}

void ModuleMovementPilgrimKeeper::on_owner_destroy()
{
	set_pilgrim(nullptr);
	pathToPilgrim.reset();
}

void ModuleMovementPilgrimKeeper::set_pilgrim(Framework::IModulesOwner* _pilgrim)
{
	if (observingPresence)
	{
		observingPresence->remove_presence_observer(this);
	}
	observingPresence = nullptr;
	pilgrim = _pilgrim;
	pathToPilgrim.clear_target();
	if (auto* p = pilgrim.get())
	{
		observingPresence = p->get_presence();
		observingPresence->add_presence_observer(this);
	}
}

void ModuleMovementPilgrimKeeper::invalidate()
{
	pathToPilgrim.clear_target();
}

bool ModuleMovementPilgrimKeeper::is_at_pilgrim(OPTIONAL_ OUT_ bool* _warning, OPTIONAL_ OUT_ float* _penetrationPt) const
{
	if (is_disabled())
	{
		return true;
	}
	assign_optional_out_param(_warning, false);
	assign_optional_out_param(_penetrationPt, 0.0f);
	if (validPlacementVR.is_set())
	{
		if (auto* t = pathToPilgrim.get_target())
		{
			Transform oior = get_owner()->get_presence()->get_placement();
			Transform pior = pathToPilgrim.get_placement_in_owner_room();
			Vector3 oiorl = oior.get_translation();
			Vector3 piorl = pior.location_to_world(pathToPilgrim.get_target_presence()->calculate_move_centre_offset_os().get_translation());
			float distance = (piorl - oiorl).length();
			distance *= safe_inv(MainConfig::global().should_be_immobile_vr() ? 1.0f : MainConfig::global().get_vr_horizontal_scaling());

			float makeMaxDistanceSmaller = maxDistance * 0.9f * (1.0f - PlayerSetup::get_current().get_preferences().keepInWorldSensitivity);
			assign_optional_out_param(_warning, distance < maxDistanceWarn);
			assign_optional_out_param(_penetrationPt, collisionTime == 0.0f? 0.0f : clamp((distance - makeMaxDistanceSmaller) / (maxDistance - makeMaxDistanceSmaller), 0.0f, 1.0f));
			if (invalidPlacementTime > 0.1f)
			{
				return distance <= maxDistance;
			}
		}
	}
	return true;
}

void ModuleMovementPilgrimKeeper::request_teleport()
{
	pathToPilgrim.clear_target();
	if (auto* p = pilgrim.get())
	{
		auto* pp = p->get_presence();
		auto* op = get_owner()->get_presence();
		Transform pipr = pp->get_placement();
		pipr = pipr.to_world(pp->calculate_move_centre_offset_os());
		op->request_teleport(pp->get_in_room(), pipr);
	}
}

void ModuleMovementPilgrimKeeper::apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext& _context)
{
	debug_filter(moduleMovementDebug);
	debug_subject(get_owner());
	debug_context(get_owner()->get_presence()->get_in_room());

	invalidPlacementTime += _deltaTime;
	{
		bool allowCheck = false;
		if (notInControlDisabledTimeLeft > 0.0f)
		{
			allowCheck = true; // check every time
		}
		else if (notInControlDisabledTimeLeft < -0.2f)
		{
			allowCheck = true; // check every now and then
		}
		notInControlDisabledTimeLeft -= _deltaTime;
		if (notInControlDisabledTimeLeft <= 0.0f && allowCheck)
		{
			notInControlDisabledTimeLeft = 0.0f;
			bool inControl = pilgrim.get() != nullptr;
			if (auto* g = Game::get_as<Game>())
			{
				if (g->access_player().get_actor() == pilgrim.get())
				{
					inControl = true;
				}
				if (g->access_player_taken_control().get_actor() &&
					g->access_player_taken_control().get_actor() != pilgrim.get())
				{
					inControl = false;
				}
			}
			if (!inControl)
			{
				notInControlDisabledTimeLeft = 1.0f;
			}
		}
	}

	disabled = is_disabled();
	if (notInControlDisabledTimeLeft > 0.0f)
	{
		disabled = true;
	}
	if (disabled)
	{
		// keep on teleporting - we don't want to stay somewhere far
		invalidPlacementTime = 0.0f;
		pathToPilgrim.clear_target();
		request_teleport();
		return;
	}

	if (auto* p = pilgrim.get())
	{
		auto* pp = p->get_presence();
		auto* op = get_owner()->get_presence();
		if (!pathToPilgrim.is_active())
		{
			bool teleportRequired = false;
			if (pp->get_in_room() == op->get_in_room())
			{
				Transform oior = op->get_placement();
				Transform pior = pp->get_placement();
				Vector3 oiorl = oior.get_translation();
				Vector3 piorl = pior.location_to_world(pp->calculate_move_centre_offset_os().get_translation());
				float distance = (piorl - oiorl).length();
				distance *= safe_inv(MainConfig::global().should_be_immobile_vr() ? 1.0f : MainConfig::global().get_vr_horizontal_scaling());

				teleportRequired = distance > maxDistance;
			}
			else
			{
				teleportRequired = true;
			}
			if (teleportRequired)
			{
				request_teleport();
			}
			else
			{
				// same room!
				pathToPilgrim.set_target(p);
			}
			invalidPlacementTime = 0.0f;
		}
		if (pathToPilgrim.is_active()) // could have activated
		{
			// update based on, may help when pilgrim is moving on something with a higher speed
			{
				auto* pbase = op->get_in_room() == pp->get_in_room() ? pp->get_based_on() : nullptr;
				if (op->get_based_on() != pbase)
				{
					op->force_base_on(pbase);
				}
			}
			Transform oior = op->get_placement();
			Transform pior = pathToPilgrim.get_placement_in_owner_room();
			pior = pior.to_world(pathToPilgrim.get_target_presence()->calculate_move_centre_offset_os());
			Vector3 diff = pior.get_translation() - oior.get_translation();
			_context.currentDisplacementLinear = diff;
			if (diff.length() * safe_inv(MainConfig::global().should_be_immobile_vr() ? 1.0f : MainConfig::global().get_vr_horizontal_scaling()) < maxDistance * (0.8f - 0.3f * PlayerSetup::get_current().get_preferences().keepInWorldSensitivity))
			{
				// start invalid placement earlier
				invalidPlacementTime = 0.0f;
			}
			{
				todo_note(TXT("maybe this should be changed, so we're not checking vr unless we're in current control?"));

				auto* vr = VR::IVR::get();

				if (vr && vr->get_last_valid_render_pose_set().view.is_set()) // although it should always be valid at this point!
				{
					validPlacementVR = vr->get_last_valid_render_pose_set().view.get().to_world(vr->get_movement_centre_in_view_space());
				}
#ifdef AN_DEVELOPMENT_OR_PROFILER
				else if (!vr)
				{
					validPlacementVR = op->get_vr_anchor().to_local(pior);
				}
#endif
				// always use keeper's location
				if (validPlacementVR.is_set())
				{
					Vector3 ol = op->get_placement().get_translation(); // owner's location
					Vector3 olpr = pathToPilgrim.location_from_owner_to_target(ol); // owner's location in pilgrim's room
					Vector3 olvr = pp->get_vr_anchor().location_to_local(olpr); // owner's location in vr
					validPlacementVR.access().set_translation(olvr);
				}
			}
		}
	}

	bool colliding = false;
	// slide with collision
	{
		if (Framework::ModuleCollision const* collision = get_owner()->get_collision())
		{
			Vector3 accelerationFromGradient = collision->get_gradient();
			if (!accelerationFromGradient.is_zero())
			{
				Vector3 gradNor = accelerationFromGradient.normal();
				float gradLen = accelerationFromGradient.length();
				float d = Vector3::dot(_context.currentDisplacementLinear, gradNor);
				if (d < 0.0f)
				{
					_context.currentDisplacementLinear -= gradNor * d;
				}
				// allow to penetrate a bit
				_context.currentDisplacementLinear += gradNor * max(0.0f, gradLen - 0.01f);

				colliding = true;
			}
		}
	}

#ifdef AN_DEBUG_RENDERER
	if (auto* p = pilgrim.get())
	{
		auto* pp = p->get_presence();
		auto* op = get_owner()->get_presence();

		if (op->get_in_room())
		{
			debug_draw_sphere(true, false, Colour::red, 0.5f, Sphere(op->get_placement().get_translation(), 0.01f));

			if (pathToPilgrim.is_active())
			{
				Transform pior = pathToPilgrim.get_placement_in_owner_room();
				Vector3 piorl = pior.location_to_world(pp->calculate_move_centre_offset_os().get_translation());
				debug_draw_sphere(true, false, Colour::green, 0.5f, Sphere(piorl, 0.02f));
			}

			if (validPlacementVR.is_set())
			{
				Vector3 vrp = Vector3::zero;
				if (pathToPilgrim.is_active())
				{
					vrp = pathToPilgrim.location_from_target_to_owner(pp->get_vr_anchor().location_to_world(validPlacementVR.get().get_translation()));
				}
				else if (op->get_in_room())
				{
					vrp = op->get_in_room()->get_vr_anchor().location_to_world(validPlacementVR.get().get_translation());
				}
				debug_draw_sphere(true, false, Colour::blue, 0.2f, Sphere(vrp, 0.03f));
			}
		}
	}
#endif

	// validate movement (check with a smaller radius - if we collide anything, bail out
	if (auto* collision = get_owner()->get_collision())
	{
		float radiusCoef = 0.4f;
		// check if we may collide (with a smaller radius), if we will, move back
		Vector3 displacement = _context.currentDisplacementLinear;
		float displacementLength = displacement.length();
		float step = collision->get_radius_for_gradient() * radiusCoef * magic_number 0.8f;
		{
			MEASURE_PERFORMANCE(moduleMovementPilgrimKeeper_largeDisplacement);
			Vector3 dir = displacement.normal();
			float testDist = 0.0f;
			bool stopAtTestDist = false;
			while (testDist < displacementLength)
			{
				if (collision->check_if_colliding(dir * testDist, radiusCoef))
				{
					stopAtTestDist = true;
					break;
				}

				testDist += step;
			}
			testDist = min(testDist, displacementLength);

			if (stopAtTestDist)
			{
				colliding = true; // still we were colliding with something, even if we allow it later
			}

			// because we might be caught at the start (if we moved through a window we will be very close to the wall)
			// we should check if we're actually hitting anything by pure ray cast
			// if we don't, assume the move is valid
			// do this only for short test dist
			if (stopAtTestDist && testDist < step * 1.5f)
			{
				stopAtTestDist = false;

				Vector3 partialDisplacement = _context.currentDisplacementLinear * (max(step, testDist) / displacementLength);
				{
					auto* op = get_owner()->get_presence();

					Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::WS);

					Vector3 startLocation = op->get_placement().get_translation();

					collisionTrace.add_location(startLocation);
					collisionTrace.add_location(startLocation + partialDisplacement);

					Framework::CheckSegmentResult result;
					int collisionTraceFlags = Framework::CollisionTraceFlag::ResultInFinalRoom;
					Framework::CheckCollisionContext checkCollisionContext;
					checkCollisionContext.use_collision_flags(collision->get_collides_with_flags());

					if (op->trace_collision(AgainstCollision::Precise, collisionTrace, result, collisionTraceFlags, checkCollisionContext))
					{
						stopAtTestDist = true;
					}
				}
			}

			if (stopAtTestDist)
			{
				float stopAt = max(0.0f, testDist - step) / displacementLength;
				_context.currentDisplacementLinear *= stopAt;
				_context.currentDisplacementRotation = Quat::lerp(stopAt, Quat::identity, _context.currentDisplacementRotation);
				an_assert(_context.currentDisplacementRotation.is_normalised(0.1f));
			}
		}
	}

	if (colliding)
	{
		collisionTime += _deltaTime;
	}
	else
	{
		collisionTime = 0.0f;
	}

	debug_no_context();
	debug_no_subject();
	debug_no_filter();

	// this is commented out for reason unknown
	// if there's collision, it may override displacement
	//_context.applyCollision = true;
	//base::apply_changes_to_velocity_and_displacement(_deltaTime, _context);
}

void ModuleMovementPilgrimKeeper::on_presence_changed_room(Framework::ModulePresence* _presence, Framework::Room* _intoRoom, Transform const& _intoRoomTransform, Framework::DoorInRoom* _exitThrough, Framework::DoorInRoom* _enterThrough, Framework::DoorInRoomArrayPtr const* _throughDoors)
{
	if (auto* d = _exitThrough->get_door())
	{
		if (d->get_tags().get_tag(NAME(dt_impassable)))
		{
			// stay with pilgrim keeper where we are
			return;
		}
	}
	//if (_exitThrough->should_skip_hole_check_on_moving_through())
	// for time being try doing it always as I expect some doors might be botched and have collisions leaking
	{
		// will reset and teleport
		pathToPilgrim.clear_target();
	}
}

void ModuleMovementPilgrimKeeper::on_presence_added_to_room(Framework::ModulePresence* _presence, Framework::Room* _room, Framework::DoorInRoom* _enteredThroughDoor)
{
	// will reset and teleport
	pathToPilgrim.clear_target();
}

void ModuleMovementPilgrimKeeper::on_presence_removed_from_room(Framework::ModulePresence* _presence, Framework::Room* _room)
{
	// will reset and teleport? if knows where?
	pathToPilgrim.clear_target();
}

void ModuleMovementPilgrimKeeper::on_presence_destroy(Framework::ModulePresence* _presence)
{
	set_pilgrim(nullptr);
}
