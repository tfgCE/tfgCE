#include "moduleMovementCentipede.h"

#include "..\..\framework\framework.h"
#include "..\..\framework\collision\checkCollisionContext.h"
#include "..\..\framework\collision\checkSegmentResult.h"
#include "..\..\framework\module\modules.h"
#include "..\..\framework\module\moduleController.h"
#include "..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define DEBUG_DRAW_CENTIPEDE_MOVEMENT

//#define EXTRA_PRECAUTIORY_TRACES

//

using namespace TeaForGodEmperor;

//

// module params
DEFINE_STATIC_NAME(traceLocOS);
DEFINE_STATIC_NAME(traceCollideWithFlags);
DEFINE_STATIC_NAME(matchZBlendTime);
DEFINE_STATIC_NAME(reorientateUpBlendTime);
DEFINE_STATIC_NAME(followAngle);

// sockets
DEFINE_STATIC_NAME(pivot);
DEFINE_STATIC_NAME(frontConnector);
DEFINE_STATIC_NAME(backConnector);

//

REGISTER_FOR_FAST_CAST(ModuleMovementCentipede);

static Framework::ModuleMovement* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleMovementCentipede(_owner);
}

Framework::RegisteredModule<Framework::ModuleMovement> & ModuleMovementCentipede::register_itself()
{
	return Framework::Modules::movement.register_module(String(TXT("centipede")), create_module);
}

ModuleMovementCentipede::ModuleMovementCentipede(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

void ModuleMovementCentipede::setup_with(Framework::ModuleData const* _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		Vector3 traceLocOS = _moduleData->get_parameter<Vector3>(this, NAME(traceLocOS), Vector3::zero);
		if (!traceLocOS.is_zero())
		{
			traces.set_size(Trace::NUM);
			traces[Trace::LF].locOS = traceLocOS * Vector3(-1.0f,  1.0f, 1.0f);
			traces[Trace::RF].locOS = traceLocOS * Vector3( 1.0f,  1.0f, 1.0f);
			traces[Trace::LB].locOS = traceLocOS * Vector3(-1.0f, -1.0f, 1.0f);
			traces[Trace::RB].locOS = traceLocOS * Vector3( 1.0f, -1.0f, 1.0f);

			for_every(t, traces)
			{
				t->hitOS = t->locOS;
				t->hitOS.z = 0.0f;
			}
		}

		traceCollideWithFlags.apply(_moduleData->get_parameter<String>(this, NAME(traceCollideWithFlags)));

		matchZBlendTime = _moduleData->get_parameter<float>(this, NAME(matchZBlendTime), matchZBlendTime);
		reorientateUpBlendTime = _moduleData->get_parameter<float>(this, NAME(reorientateUpBlendTime), reorientateUpBlendTime);

		followAngle = _moduleData->get_parameter<float>(this, NAME(followAngle), followAngle);
	}

	head.placementOS = Transform(Vector3(0.0f, 10.0f, 0.0f), Quat::identity);
	tail.placementOS = Transform(Vector3(0.0f, -10.0f, 0.0f), Quat::identity);
}

void ModuleMovementCentipede::apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext& _context)
{
	auto* imo = get_owner();

	base::apply_changes_to_velocity_and_displacement(_deltaTime, _context);

	{
		Vector3 nowAt = imo->get_presence()->get_placement().get_translation();
		Vector3 up = imo->get_presence()->get_placement().get_axis(Axis::Up);
		an_assert_immediate(nowAt.is_ok());
		an_assert_immediate(up.is_ok());
		if ((traceCollisionsDoneAt - nowAt).length() > traceCollisionsDistance ||
			Vector3::dot(traceCollisionsDoneForUp, up) < 0.9f)
		{
			traceCollisionsDoneAt = nowAt;
			traceCollisionsDoneForUp = up;
			traceCollisionsDistance = rg.get_float(0.05f, 0.1f);
			for_every(t, traces)
			{
				Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::OS);
				collisionTrace.add_location(t->locOS);
				collisionTrace.add_location(t->locOS * Vector3(1.0f, 1.0f, 0.0f) - Vector3::zAxis * 1.0f);
				int collisionTraceFlags = Framework::CollisionTraceFlag::ResultInObjectSpace;

				Framework::CheckCollisionContext checkCollisionContext;
				checkCollisionContext.collision_info_needed();
				checkCollisionContext.use_collision_flags(traceCollideWithFlags);
				checkCollisionContext.ignore_actors();
				checkCollisionContext.ignore_items();
				checkCollisionContext.ignore_temporary_objects();
				checkCollisionContext.avoid_up_to_top_instigator(imo); // so we won't hit the hand or body capsule

				Framework::CheckSegmentResult result;

				if (imo->get_presence()->trace_collision(AgainstCollision::Movement, collisionTrace, result, collisionTraceFlags, checkCollisionContext))
				{
					t->hitOS = result.hitLocation;
				}
			}
		}
	}

	if (stationary)
	{
		_context.currentDisplacementRotation = Quat::identity;
		_context.currentDisplacementLinear = Vector3::zero;
		_context.velocityLinear = Vector3::zero;
		_context.velocityRotation = Rotator3::zero;
		return;
	}

	Vector3 upDirOS = Vector3::zAxis;
	float zOffset = 0.0f;
	if (!traces.is_empty())
	{
		// up = Vector3::cross(right, fwd);
		Vector3 upDirLF = Vector3::cross(traces[Trace::RF].hitOS - traces[Trace::LB].hitOS, traces[Trace::LF].hitOS - traces[Trace::LB].hitOS).normal();
		Vector3 upDirLR = Vector3::cross(traces[Trace::RB].hitOS - traces[Trace::LF].hitOS, traces[Trace::RF].hitOS - traces[Trace::RB].hitOS).normal();

		upDirOS = upDirLF + upDirLR;
		upDirOS.normalise();

		for_every(t, traces)
		{
			zOffset += t->hitOS.z;
		}
		zOffset /= (float)traces.get_size();
	}

	_deltaTime = min(_deltaTime, 0.05f); // shouldn't drop below this - we don't want to move to the other side!

	if (_deltaTime != 0.0f)
	{
		float deltaTimeForVelocityCalculation = min(_deltaTime, 0.03f); // we shouldn't exceed such a frame time
		float deltaTimeForPrediction = 0.0f; // do not use it now
		head.weight -= _deltaTime;
		head.weight = 0.0f;
		if (auto* headIMO = head.imo.get())
		{
			if (headIMO->get_presence()->get_in_room() == imo->get_presence()->get_in_room())
			{
				if (auto* a = headIMO->get_appearance())
				{
					// HOS - Head's OS
					Transform headsBackHOS = a->calculate_socket_os(head.connectorThere.get_index());
					Transform headsBackWS = headIMO->get_presence()->get_placement().to_world(headsBackHOS);
					an_assert_immediate(headsBackHOS.is_ok());
					an_assert_immediate(headsBackWS.is_ok());
					headsBackWS.set_translation(headsBackWS.get_translation() + headIMO->get_presence()->get_velocity_linear() * deltaTimeForPrediction);
					head.placementOS = imo->get_presence()->get_placement().to_local(headsBackWS);
					head.weight = 1.0f;

#ifdef DEBUG_DRAW_CENTIPEDE_MOVEMENT
					debug_context(imo->get_presence()->get_in_room());
					debug_filter(ModuleMovementCentipede);
					debug_draw_line(true, Colour::yellow, headsBackWS.get_translation(), headIMO->get_presence()->get_placement().get_translation());
					debug_draw_line(true, Colour::yellow, headsBackWS.get_translation(), imo->get_presence()->get_placement().get_translation());
					debug_no_filter();
					debug_no_context();
#endif
				}
			}
		}
		if (auto* tailIMO = tail.imo.get())
		{
			if (tailIMO->get_presence()->get_in_room() == imo->get_presence()->get_in_room())
			{
				if (auto* a = tailIMO->get_appearance())
				{
					// TOS - Tail's OS
					Transform tailsFrontHOS = a->calculate_socket_os(tail.connectorThere.get_index());
					Transform tailsFrontWS = tailIMO->get_presence()->get_placement().to_world(tailsFrontHOS);
					tailsFrontWS.set_translation(tailsFrontWS.get_translation() + tailIMO->get_presence()->get_velocity_linear() * deltaTimeForPrediction);
					tail.placementOS = imo->get_presence()->get_placement().to_local(tailsFrontWS);
					tail.weight = 1.0f;
				}
			}
		}

		if (head.weight > 0.0f)
		{
			if (auto* a = imo->get_appearance())
			{
				if (head.connectorHere.is_valid())
				{
#ifdef DEBUG_DRAW_CENTIPEDE_MOVEMENT
					debug_context(imo->get_presence()->get_in_room());
					debug_filter(ModuleMovementCentipede);
					debug_push_transform(imo->get_presence()->get_placement());
#endif

					Transform frontOS = a->calculate_socket_os(head.connectorHere.get_index());
					Transform headsBackOS = head.placementOS;
					an_assert_immediate(frontOS.is_ok());
					an_assert_immediate(headsBackOS.is_ok());

#ifdef DEBUG_DRAW_CENTIPEDE_MOVEMENT
					debug_draw_transform_size_coloured(true, frontOS, 0.1f, Colour::blue, Colour::blue, Colour::blue);
					debug_draw_line(true, Colour::blue, frontOS.get_translation(), imo->get_presence()->get_placement().location_to_local(imo->get_presence()->get_placement().get_translation()));
					debug_draw_transform_size_coloured(true, headsBackOS, 0.1f, Colour::magenta, Colour::magenta, Colour::magenta);
					debug_draw_line(true, Colour::magenta, headsBackOS.get_translation(), imo->get_presence()->get_placement().location_to_local(imo->get_presence()->get_placement().get_translation()));
#endif

					// HBSOS - Head's Back Space (in our OS)
					Transform frontHBSOS = headsBackOS.to_local(frontOS);
					an_assert_immediate(frontHBSOS.is_ok());

					Transform backedFrontHBSOS = frontHBSOS.to_world(Transform(-Vector3::yAxis * frontOS.get_translation().y, Quat::identity));
					an_assert_immediate(backedFrontHBSOS.is_ok());

					{
						float const angleBack = followAngle;
						Rotator3 backFrontRotHBSOS = backedFrontHBSOS.get_translation().to_rotator();
						float reversedYaw = Rotator3::normalise_axis(backFrontRotHBSOS.yaw + 180.0f);
						reversedYaw = clamp(reversedYaw, -angleBack, angleBack);
						float fixedYaw = reversedYaw - 180.0f;
						Rotator3 fixedRotHBSOS(backFrontRotHBSOS.pitch, fixedYaw, backFrontRotHBSOS.roll);
						backedFrontHBSOS.set_translation(fixedRotHBSOS.get_forward()* backedFrontHBSOS.get_translation().length());
					}

					// todo - limit loc in HBSOS (we can be only at certain angles)

					Transform backedFrontOS = headsBackOS.to_world(backedFrontHBSOS);

#ifdef DEBUG_DRAW_CENTIPEDE_MOVEMENT
					debug_draw_transform_size_coloured(true, backedFrontOS, 0.1f, Colour::orange, Colour::magenta, Colour::magenta);
					debug_draw_line(true, Colour::orange, backedFrontOS.get_translation(), imo->get_presence()->get_placement().location_to_local(imo->get_presence()->get_placement().get_translation()));

					debug_draw_arrow(true, Colour::red, backedFrontOS.get_translation(), headsBackOS.get_translation());
#endif

					Vector3 fwd = headsBackOS.get_translation() - backedFrontOS.get_translation();
					an_assert_immediate(fwd.is_ok());

					if (!fwd.is_zero())
					{
						fwd.normalise();

						Vector3 upDir = frontHBSOS.get_axis(Axis::Up).z > 0.0f ? Vector3::zAxis : -Vector3::zAxis; // if we ended up upside down for some reason, untangle

						Transform tgtOS = look_matrix(headsBackOS.get_translation(), fwd, upDir).to_transform();
						
						// move back so we're where we're supposed to be
						Transform requestedOS = tgtOS.to_world(frontOS.inverted());

#ifdef DEBUG_DRAW_CENTIPEDE_MOVEMENT
						debug_draw_arrow(true, Colour::green, Vector3::zero, requestedOS.get_translation());
						debug_draw_transform_size(true, requestedOS, 0.2f);
#endif

						_context.currentDisplacementRotation = requestedOS.get_orientation();

						{
							Vector3 displacementLinear = requestedOS.get_translation();
							displacementLinear.z *= 0.2f;

							_context.currentDisplacementLinear = imo->get_presence()->get_placement().vector_to_world(displacementLinear);
						}

						if (_deltaTime != 0.0f)
						{
							_context.velocityLinear = _context.currentDisplacementLinear / deltaTimeForVelocityCalculation;
							//_context.velocityRotation = _context.currentDisplacementRotation.to_rotator() / deltaTimeForVelocityCalculation;
							_context.velocityRotation = Rotator3::zero;
						}
					}

#ifdef DEBUG_DRAW_CENTIPEDE_MOVEMENT
					debug_pop_transform();
					debug_no_filter();
					debug_no_context();
#endif
				}
			}
		}
		else 
		{
			Vector3 move = Vector3::zero;
			Rotator3 turn = Rotator3::zero;
			todo_note(TXT("this is done to work solely with centipede logic, that's why any other way may not work"));
			if (auto const* const controller = imo->get_controller())
			{
				move.y = controller->get_requested_movement_parameters().relativeSpeed.get(0.0f);
				turn = controller->get_requested_relative_velocity_orientation().get(Rotator3::zero);
			}
#ifdef BUILD_NON_RELEASE
#ifdef AN_STANDARD_INPUT
			if (Framework::is_preview_game())
			{
				if (auto* i = ::System::Input::get())
				{
					if (i->is_key_pressed(::System::Key::UpArrow))
					{
						move += Vector3::yAxis;
					}
					if (i->is_key_pressed(::System::Key::DownArrow))
					{
						move -= Vector3::yAxis;
					}
					if (i->is_key_pressed(::System::Key::LeftArrow))
					{
						turn.yaw -= 1.0f;
					}
					if (i->is_key_pressed(::System::Key::RightArrow))
					{
						turn.yaw += 1.0f;
					}

				}
			}
#endif
#endif
			move *= find_speed_of_current_gait();
			currentMove = blend_to_using_time(currentMove, move, 0.1f, _deltaTime);

			turn *= find_orientation_speed_of_current_gait();
			currentTurn = blend_to_using_time(currentTurn, turn, 0.05f, _deltaTime);
			
			currentMove = move;
			currentTurn = turn;

			_context.currentDisplacementLinear = imo->get_presence()->get_placement().vector_to_world(currentMove * _deltaTime);
			_context.currentDisplacementRotation = _context.currentDisplacementRotation.to_world((currentTurn * _deltaTime).to_quat());
			if (_deltaTime != 0.0f)
			{
				_context.velocityLinear = imo->get_presence()->get_placement().vector_to_world(currentMove);
				_context.velocityRotation = currentTurn;
			}
		}

		{
			Vector3 displacementLinear = Vector3::zAxis * (zOffset * clamp(_deltaTime / matchZBlendTime, 0.0f, 1.0f));

			_context.currentDisplacementLinear += imo->get_presence()->get_placement().vector_to_world(displacementLinear);
		}
	}

	if (_deltaTime != 0.0f)
	{
		Quat requestedOrientationOS = matrix_from_up_forward(Vector3::zero, upDirOS, Vector3::yAxis).to_quat();
		{
			Rotator3 requestedRotationOS = requestedOrientationOS.to_rotator();
			requestedRotationOS.yaw = 0.0f;
			requestedOrientationOS = requestedRotationOS.to_quat();
		}

		_context.currentDisplacementRotation = _context.currentDisplacementRotation.to_world(Quat::slerp(clamp(_deltaTime / reorientateUpBlendTime, 0.0f, 1.0f), Quat::identity, requestedOrientationOS));
	}

#ifdef EXTRA_PRECAUTIORY_TRACES
	// in an unlikely event a part will end up on the other side, move it towards the head
	if (auto* himo = head.imo.get())
	{
		if (rg.get_chance(0.05f))
		{
			Vector3 nowAt = imo->get_presence()->get_placement().get_translation();
			Vector3 headAt = himo->get_presence()->get_placement().get_translation();

			Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::OS);
			collisionTrace.add_location(headAt);
			collisionTrace.add_location(nowAt);
			int collisionTraceFlags = Framework::CollisionTraceFlag::ResultInPresenceRoom;

			Framework::CheckCollisionContext checkCollisionContext;
			checkCollisionContext.collision_info_needed();
			checkCollisionContext.use_collision_flags(traceCollideWithFlags);
			checkCollisionContext.ignore_actors();
			checkCollisionContext.ignore_items();
			checkCollisionContext.ignore_temporary_objects();
			checkCollisionContext.avoid_up_to_top_instigator(imo); // so we won't hit the hand or body capsule

			Framework::CheckSegmentResult result;

			if (imo->get_presence()->trace_collision(AgainstCollision::Movement, collisionTrace, result, collisionTraceFlags, checkCollisionContext))
			{
				Vector3 headToHit = result.hitLocation - headAt;
				Vector3 beAt = headAt + headToHit * 0.8f;
				_context.currentDisplacementLinear = beAt - nowAt; // enforce to avoid going through stuff
			}
		}
	}
	else
	{
		// for head, validate the displacement - make sure we're not moving through collision

		// check if we would move through a collision
		Vector3 nowAt = imo->get_presence()->get_placement().get_translation();
		Vector3 beAt = nowAt + _context.currentDisplacementLinear;

		Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::OS);
		collisionTrace.add_location(nowAt);
		collisionTrace.add_location(beAt);
		int collisionTraceFlags = Framework::CollisionTraceFlag::ResultInPresenceRoom;

		Framework::CheckCollisionContext checkCollisionContext;
		checkCollisionContext.collision_info_needed();
		checkCollisionContext.use_collision_flags(traceCollideWithFlags);
		checkCollisionContext.ignore_actors();
		checkCollisionContext.ignore_items();
		checkCollisionContext.ignore_temporary_objects();
		checkCollisionContext.avoid_up_to_top_instigator(imo); // so we won't hit the hand or body capsule

		Framework::CheckSegmentResult result;

		if (imo->get_presence()->trace_collision(AgainstCollision::Movement, collisionTrace, result, collisionTraceFlags, checkCollisionContext))
		{
			beAt = nowAt + (result.hitLocation - nowAt) * 0.8f;
		}
		_context.currentDisplacementLinear = beAt - nowAt;
	}
#endif
}

Vector3 ModuleMovementCentipede::calculate_requested_velocity(float _deltaTime, bool _adjustForStoppingEtc, PrepareMoveContext& _context) const
{
	return base::calculate_requested_velocity(_deltaTime, _adjustForStoppingEtc, _context);
}

void ModuleMovementCentipede::set_head(Framework::IModulesOwner* imo)
{
	head.imo = imo;

	head.connectorHere.set_name(NAME(frontConnector));
	head.connectorThere.set_name(NAME(backConnector));

	update_connectors(head);
}

void ModuleMovementCentipede::set_tail(Framework::IModulesOwner* imo)
{
	tail.imo = imo;

	tail.connectorHere.set_name(NAME(backConnector));
	tail.connectorThere.set_name(NAME(frontConnector));

	update_connectors(tail);
}

void ModuleMovementCentipede::update_connectors(OtherSegment& _segment)
{
	if (auto* a = get_owner()->get_appearance())
	{
		_segment.connectorHere.look_up(a->get_mesh());
		if (!pivot.is_valid())
		{
			pivot.set_name(NAME(pivot));
			pivot.look_up(a->get_mesh());
		}
	}
	else
	{
		_segment.connectorHere.invalidate();
	}
	if (auto* a = (_segment.imo.get() ? _segment.imo->get_appearance() : nullptr))
	{
		_segment.connectorThere.look_up(a->get_mesh());
	}
	else
	{
		_segment.connectorThere.invalidate();
	}
}

Framework::IModulesOwner* ModuleMovementCentipede::get_tail() const
{
	return tail.imo.get();
}
