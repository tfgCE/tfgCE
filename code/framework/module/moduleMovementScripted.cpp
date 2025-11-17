#include "moduleMovementScripted.h"

#include "..\gameScript\gameScriptExecution.h"

#include "..\module\moduleAppearance.h"
#include "..\module\modules.h"
#include "..\module\registeredModule.h"
#include "..\world\room.h"

#include "..\advance\advanceContext.h"

#include "..\..\core\performance\performanceUtils.h"

//

using namespace Framework;

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

DEFINE_STATIC_NAME(speed);

//

bool ScriptedMovement::Rotate::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	if (_node)
	{
		target.load_from_xml(_node, TXT("target"));

		speed = _node->get_float_attribute(TXT("speed"), speed);
		acceleration = _node->get_float_attribute(TXT("acceleration"), acceleration);
	}

	return result;
}

//--

bool ScriptedMovement::load_from_xml(IO::XML::Node const* _node, bool _mayBeInvalid)
{
	bool result = true;

	repeat = _node->get_bool_attribute_or_from_child_presence(TXT("repeat"), repeat);

	toPOI.load_from_xml(_node, TXT("toPOI"));
	offsetLocationOS.load_from_xml(_node, TXT("offsetLocation"));
	offsetLocationOS.load_from_xml(_node, TXT("offsetLocationOS"));
	velocityLinearOS.load_from_xml(_node, TXT("velocityLinearOS"));
	velocityRotationOS.load_from_xml(_node, TXT("velocityRotationOS"));
	velocityRotationOSBlendTime.load_from_xml(_node, TXT("velocityRotationOSBlendTime"));
	offsetRotation.load_from_xml(_node, TXT("offsetRotation"));
	toLocationWS.load_from_xml(_node, TXT("toLocationWS"));
	toLocationZWS.load_from_xml(_node, TXT("toLocationZWS"));
	randomOffsetLocationWS.load_from_xml(_node, TXT("randomOffsetLocationWS"));

	allowedMovementWS.load_from_xml(_node, TXT("allowedMovementWS"));

	linearSpeed.load_from_xml(_node, TXT("linearSpeed"));
	linearAcceleration.load_from_xml(_node, TXT("linearAcceleration"));
	linearDeceleration.load_from_xml(_node, TXT("linearDeceleration"));

	rotationSpeed.load_from_xml(_node, TXT("rotationSpeed"));
	rotatePitch.load_from_xml(_node->first_child_named(TXT("rotatePitch")));
	rotateYaw.load_from_xml(_node->first_child_named(TXT("rotateYaw")));
	rotateRoll.load_from_xml(_node->first_child_named(TXT("rotateRoll")));

	time.load_from_xml(_node, TXT("time"));
	smoothStart.load_from_xml(_node, TXT("smoothStart"));
	smoothEnd.load_from_xml(_node, TXT("smoothEnd"));

	playSound = _node->get_name_attribute(TXT("playSound"), playSound);

	triggerGameScriptExecutionTrapOnDecelerate = _node->get_name_attribute(TXT("triggerGameScriptExecutionTrapOnDecelerate"), triggerGameScriptExecutionTrapOnDecelerate);
	triggerGameScriptExecutionTrapOnDone = _node->get_name_attribute(TXT("triggerGameScriptExecutionTrapOnDone"), triggerGameScriptExecutionTrapOnDone);

	error_loading_xml_on_assert(is_valid() || _mayBeInvalid, result, _node, TXT("has to be speed/accel/decel or time based"));

	return result;
}

//--

REGISTER_FOR_FAST_CAST(ModuleMovementScripted);

static Framework::ModuleMovement* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleMovementScripted(_owner);
}

Framework::RegisteredModule<Framework::ModuleMovement> & ModuleMovementScripted::register_itself()
{
	return Framework::Modules::movement.register_module(String(TXT("scripted")), create_module);
}

ModuleMovementScripted::ModuleMovementScripted(Framework::IModulesOwner* _owner)
: base(_owner)
{
	requiresPrepareMove = false;
}

ModuleMovementScripted::~ModuleMovementScripted()
{
	set_observe_presence(false);
	for_every(m, movements)
	{
		if (m->playedSound.get()) { m->playedSound->stop(); }
	}
}

void ModuleMovementScripted::set_observe_presence(bool _observe)
{
	if (observingPresence)
	{
		observingPresence->remove_presence_observer(this);
	}
	observingPresence = nullptr;
	if (_observe)
	{
		if (auto* imo = get_owner())
		{
			observingPresence = imo->get_presence();
			observingPresence->add_presence_observer(this);
		}
	}
}

void ModuleMovementScripted::activate()
{
	base::activate();
	
	set_observe_presence(true);
}

void ModuleMovementScripted::on_owner_destroy()
{
	base::on_owner_destroy();
	for_every(m, movements)
	{
		if (m->playedSound.get()) { m->playedSound->stop(); }
	}
	set_observe_presence(false);
}

void ModuleMovementScripted::stop(bool _immediately)
{
	Concurrency::ScopedSpinLock lock(movementLock);

	if (_immediately)
	{
		movements.clear();
	}
	else
	{
		for_every(m, movements)
		{
			m->activeTarget = 0.0f;
		}
	}
}

void ModuleMovementScripted::do_movement(ScriptedMovement const& _movement)
{
	Concurrency::ScopedSpinLock lock(movementLock);

	for_every(m, movements)
	{
		m->activeTarget = 0.0f;
	}

	movements.grow_size(1);
	auto& m = movements.get_last();
	*((ScriptedMovement*)&m) = _movement;
	m.activeTarget = 1.0f;
}

void ModuleMovementScripted::apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext& _context)
{
	if (_deltaTime == 0.0f)
	{
		return;
	}
	auto* presence = get_owner()->get_presence();
	Transform const isAt = presence->get_placement();
	Vector3 velocityLinear = presence->get_velocity_linear();
	Rotator3 velocityRotation = presence->get_velocity_rotation();
	Optional<Vector3> displacementLinear;
	Optional<Quat> displacementOrientation;
	
	requiresContinuousMovement = !movements.is_empty();

	bool requiresRemoval = false;
	for_every(m, movements)
	{
		bool shouldPlaySound = false;

		bool wasDecelerating = m->decelerating;
		bool wasAtDestination = m->atDestination;

		m->decelerating = false;
		m->atDestination = false;
 
		bool setupThisFrame = m->requiresSetup;
		m->requiresSetup = false;

		if (setupThisFrame || get_owner()->get_presence()->has_teleported())
		{
			m->targetPlacementWS = isAt;
			if (m->offsetLocationOS.is_set())
			{
				m->targetPlacementWS.set_translation(m->targetPlacementWS.location_to_world(m->offsetLocationOS.get()));
			}
			if (m->offsetRotation.is_set())
			{
				m->targetPlacementWS.set_orientation(m->targetPlacementWS.get_orientation().to_world(m->offsetRotation.get().to_quat()));
			}
			if (m->toLocationWS.is_set())
			{
				m->targetPlacementWS.set_translation(m->toLocationWS.get());
			}
			if (m->toLocationZWS.is_set())
			{
				Vector3 tpWS = m->targetPlacementWS.get_translation();
				tpWS.z = m->toLocationZWS.get();
				m->targetPlacementWS.set_translation(tpWS);
			}
			if (m->toPOI.is_set())
			{
				Framework::PointOfInterestInstance* foundPOI = nullptr;
				if (presence->get_in_room()->find_any_point_of_interest(m->toPOI, foundPOI, true))
				{
					m->targetPlacementWS = foundPOI->calculate_placement();

					if (m->offsetLocationOS.is_set())
					{
						m->targetPlacementWS.set_translation(m->targetPlacementWS.location_to_world(m->offsetLocationOS.get()));
					}
					if (m->offsetRotation.is_set())
					{
						m->targetPlacementWS.set_orientation(m->targetPlacementWS.get_orientation().to_world(m->offsetRotation.get().to_quat()));
					}
				}
			}
			m->startingPlacementWS = isAt;

			if (m->allowedMovementWS.is_set())
			{
				Vector3 startLoc = m->startingPlacementWS.get_translation();
				Vector3 targetLoc = m->targetPlacementWS.get_translation();
				targetLoc = startLoc + (targetLoc - startLoc) * m->allowedMovementWS.get();
				m->targetPlacementWS.set_translation(targetLoc);
			}

			m->timeIn = 0.0f;
		}

		if (m->timeIn == 0.0f)
		{
			m->startingPlacementWS = isAt;
		}

		m->timeIn += _deltaTime;

		m->active = blend_to_using_speed_based_on_time(m->active, m->activeTarget, m->active <= m->activeTarget ? m->blendInTime : m->blendOutTime, _deltaTime);

		if (m->active == 0.0f)
		{
			requiresRemoval = true;
		}
		if (m->activeTarget == 1.0f) // use only active for now
		{
			Vector3 resultDisplacementLinear = velocityLinear * _deltaTime;
			Vector3 resultVelocityLinear = velocityLinear;

			Quat resultDisplacementRotation = (velocityRotation * _deltaTime).to_quat();
			Rotator3 resultVelocityRotation = velocityRotation;

			bool processVelocityRotationOS = false;;
			bool processRotates = false;
			if (m->velocityRotationOS.is_set())
			{
				processVelocityRotationOS = true;
			}
			else  if (m->rotatePitch.target.is_set() ||
					  m->rotateYaw.target.is_set() ||
					  m->rotateRoll.target.is_set())
			{
				processRotates = true;
			}

			if (m->velocityLinearOS.is_set())
			{
				Vector3 tgtVL = presence->get_placement().vector_to_world(m->velocityLinearOS.get());
				if (m->linearAcceleration.is_set())
				{
					Vector3 diff = tgtVL - resultVelocityLinear;
					Vector3 apply = diff.normal() * min(diff.length(), m->linearAcceleration.get() * _deltaTime);
					resultVelocityLinear += apply;
					//resultVelocityLinear += diff;
				}
				else
				{
					resultVelocityLinear = tgtVL;
				}
			}
			else
			{
				Transform useTargetPlacementWS = m->targetPlacementWS;
				if (m->randomOffsetLocationWS.is_set() &&
					!m->useRandomOffsetLocationWS.is_set())
				{
					Random::Generator rg;
					m->useRandomOffsetLocationWS = Vector3(rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f)).normal() * rg.get_float(0.0f, m->randomOffsetLocationWS.get());
				}

				useTargetPlacementWS.set_translation(useTargetPlacementWS.get_translation() + m->useRandomOffsetLocationWS.get(Vector3::zero));

				Vector3 diff = useTargetPlacementWS.get_translation() - isAt.get_translation();

				bool mayRequireRotation = false; // time based handles rotation, if we don't do anything about it, at least stop where it is
				if (diff.is_almost_zero())
				{
					m->atDestination = true;
					resultVelocityLinear = Vector3::zero;
					resultDisplacementLinear = Vector3::zero;
					if (m->repeat)
					{
						m->reset_movement();
						shouldPlaySound = true;
					}
					mayRequireRotation = true;
				}
				else
				{
					if (m->is_time_based())
					{
						if (!m->useTime.is_set())
						{
							m->useTime = Random::get(m->time.get());
						}
						float atT = m->useTime.get() <= 0.0f? 1.0f : clamp(m->timeIn / m->useTime.get(), 0.0f, 1.0f);

						if (setupThisFrame)
						{
							m->smoothCurve.p0 = 0.0f;
							m->smoothCurve.p3 = 1.0f;

							float smoothStart = m->smoothStart.get(0.0f);
							float smoothEnd = m->smoothEnd.get(0.0f);

							// we need to calculate the middle
							// if we have both values the same, it should be right in the middle 0.5
							// if one requires smoothing and other not, it should be closer to the smoothed one
							// we calculate raw mid point and then we average it
							float midPointIn = 1.0f - smoothStart;
							float midPointOut = smoothEnd;
							float midPoint = (midPointIn + midPointOut) / 2.0f;

							// now we calculate p1 and p2
							// for linear the values for p0 to p3 are 0 1/3 2/3 1
							// for smooth are 0 0 1 1
							// we tend to keep that 
							m->smoothCurve.p1 = midPoint * 2.0f / 3.0f * (1.0f - smoothStart);
							m->smoothCurve.p2 = 1.0f - (1.0f - midPoint) * 2.0f / 3.0f * (1.0f - smoothEnd);
						}

						atT = m->smoothCurve.calculate_at(atT);

						Transform shouldBeAt = Transform::lerp(atT, m->startingPlacementWS, useTargetPlacementWS);
						resultDisplacementLinear = shouldBeAt.get_translation() - isAt.get_translation();
						resultVelocityLinear = resultDisplacementLinear / _deltaTime;

						if (!processRotates && !processVelocityRotationOS)
						{
							resultDisplacementRotation = isAt.get_orientation().to_local(shouldBeAt.get_orientation());
							resultVelocityRotation = resultDisplacementRotation.to_rotator() / _deltaTime;
						}

						shouldPlaySound |= atT < 1.0f || m->repeat;

						if (m->repeat && m->timeIn >= m->useTime.get())
						{
							m->reset_movement();
						}
					}
					else if (m->is_speed_based())
					{
						m->useTime.clear();

						Vector3 dir = diff.normal();
						float dist = diff.length();
						float velocityAlongDiff = Vector3::dot(velocityLinear, dir);

						shouldPlaySound |= (dist > 0.01f);

						float targetVelocityAlongDiff = 10000.0f;
						if (m->linearSpeed.is_set())
						{
							targetVelocityAlongDiff = min(targetVelocityAlongDiff, m->linearSpeed.get());
						}
						if (m->linearDeceleration.is_set())
						{
							// s = vt - at^2 / 2
							// v - at = 0a
							// v = at
							// s = at^2 - at^2 / 2
							// s = at^2 / 2
							// t = \/2 * s/a'
							float timeReq = sqrt(max(0.0f, 2.0f * dist / m->linearDeceleration.get()));
							targetVelocityAlongDiff = min(targetVelocityAlongDiff, timeReq * m->linearDeceleration.get());
						}
						float timeToStop = 0.0f;
						if (m->linearDeceleration.is_set() && m->linearDeceleration.get() != 0.0f)
						{
							// v - at = 0 v = at t = v/a
							timeToStop = velocityAlongDiff / m->linearDeceleration.get();

							float timeToStopAd = timeToStop + _deltaTime;
							float distToStopAd = velocityAlongDiff * timeToStopAd - m->linearDeceleration.get() * sqr(timeToStopAd) / 2.0f;

							if (distToStopAd >= dist)
							{
								targetVelocityAlongDiff = 0.0f;
							}
						}

						if (velocityAlongDiff > targetVelocityAlongDiff)
						{
							m->decelerating = true;
							if (m->linearDeceleration.is_set())
							{
								velocityAlongDiff = max(targetVelocityAlongDiff, velocityAlongDiff - m->linearDeceleration.get() * _deltaTime);
							}
							else
							{
								// will stop if >= dist
							}
						}
						else if (velocityAlongDiff < targetVelocityAlongDiff)
						{
							if (m->linearAcceleration.is_set())
							{
								velocityAlongDiff = min(targetVelocityAlongDiff, velocityAlongDiff + m->linearAcceleration.get() * _deltaTime);
							}
							else
							{
								velocityAlongDiff = targetVelocityAlongDiff;
							}
						}

						if (velocityAlongDiff * _deltaTime >= dist)
						{
							resultVelocityLinear = Vector3::zero;
							resultDisplacementLinear = diff;
							m->atDestination = true;
						}
						else
						{
							resultVelocityLinear = dir * velocityAlongDiff;
							resultDisplacementLinear = resultVelocityLinear * _deltaTime;
						}
					}
					else
					{
						m->useTime.clear();
					}
				}
				if (! m->is_time_based() || mayRequireRotation)
				{
					if (processRotates || processVelocityRotationOS)
					{
						// handled below
					}
					else if (m->rotationSpeed.is_set())
					{
						Quat requiredReorientation = isAt.get_orientation().to_local(useTargetPlacementWS.get_orientation());
						Rotator3 requiredRot = requiredReorientation.to_rotator();
						float rrLen = requiredRot.length();
						float rotNow = m->rotationSpeed.get() * _deltaTime;
						if (rotNow > rrLen && rrLen > 0.0f)
						{
							float amountPt = clamp(rotNow / rrLen, 0.0f, 1.0f);
							resultDisplacementRotation = Quat::slerp(amountPt, Quat::identity, requiredReorientation);
						}
						else
						{
							resultDisplacementRotation = requiredReorientation;
						}
						resultVelocityRotation = resultDisplacementRotation.to_rotator() / _deltaTime;
					}
					else
					{
						// don't move at all
						resultVelocityRotation = Rotator3::zero;
						resultDisplacementRotation = Quat::identity;
					}
				}
			}

			if (processVelocityRotationOS)
			{
				an_assert(m->velocityRotationOS.is_set());
				if (m->velocityRotationOSBlendTime.is_set())
				{
					resultVelocityRotation = blend_to_using_time(resultVelocityRotation, m->velocityRotationOS.get(), m->velocityRotationOSBlendTime.get(), _deltaTime);
				}
				else
				{
					resultVelocityRotation = m->velocityRotationOS.get();
				}
				resultDisplacementRotation = (resultVelocityRotation * _deltaTime).to_quat();
			}
			else if (processRotates)
			{
				// find final rot
				Rotator3 targetRWS = isAt.get_orientation().to_rotator();
				{
					for_count(int, iComponent, 3)
					{
						ScriptedMovement::Rotate& r = iComponent == 0 ? m->rotatePitch : (iComponent == 1 ? m->rotateYaw : m->rotateRoll);

						if (!r.target.is_set())
						{
							continue;
						}

						RotatorComponent::Type rotatorComponent = iComponent == 0 ? RotatorComponent::Pitch : (iComponent == 1 ? RotatorComponent::Yaw : RotatorComponent::Roll);
						targetRWS.access_component(rotatorComponent) = r.target.get();
					}
				}
				Quat targetOWS = targetRWS.to_quat();
				Quat targetOOS = isAt.get_orientation().to_local(targetOWS);
				Rotator3 targetROS = targetOOS.to_rotator();

				Rotator3 displacementROS = Rotator3::zero;
				for_count(int, iComponent, 3)
				{
					RotatorComponent::Type rotatorComponent = iComponent == 0 ? RotatorComponent::Pitch : (iComponent == 1 ? RotatorComponent::Yaw : RotatorComponent::Roll);
					ScriptedMovement::Rotate& r = iComponent == 0 ? m->rotatePitch : (iComponent == 1 ? m->rotateYaw : m->rotateRoll);

					if (!r.target.is_set() || r.speed == 0.0f)
					{
						resultVelocityRotation.access_component(rotatorComponent) = 0.0f;
						continue;
					}

					float diffToTarget = targetROS.get_component(rotatorComponent);

					if (r.acceleration != 0.0f)
					{
						float currentSpeed = resultVelocityRotation.get_component(rotatorComponent);
						float targetSpeed = r.speed * sign(diffToTarget);

						if (diffToTarget * currentSpeed > 0.0f) // same dir
						{
							// v - at = 0 v = at t = v/a
							float timeToStop = abs(currentSpeed) / r.acceleration;

							float timeToStopAd = timeToStop + _deltaTime;
							float distToStopAd = abs(currentSpeed) * timeToStopAd - r.acceleration * sqr(timeToStopAd) / 2.0f;

							float dist = abs(diffToTarget);
							if (distToStopAd >= dist)
							{
								targetSpeed = 0.0f;
							}
						}

						if (currentSpeed < targetSpeed)
						{
							currentSpeed = min(targetSpeed, currentSpeed + r.acceleration * _deltaTime);
						}
						else if (currentSpeed > targetSpeed)
						{
							currentSpeed = max(targetSpeed, currentSpeed - r.acceleration * _deltaTime);
						}

						resultVelocityRotation.access_component(rotatorComponent) = currentSpeed;
						displacementROS.access_component(rotatorComponent) = currentSpeed * _deltaTime;
					}
					else
					{
						if (abs(diffToTarget) > 0.0001f)
						{
							resultVelocityRotation.access_component(rotatorComponent) = r.speed * sign(diffToTarget);
							if (r.speed * _deltaTime > abs(diffToTarget))
							{
								displacementROS.access_component(rotatorComponent) = diffToTarget;
							}
							else
							{
								displacementROS.access_component(rotatorComponent) = r.speed * _deltaTime * sign(diffToTarget);
							}
						}
						else
						{
							resultVelocityRotation.access_component(rotatorComponent) = 0.0f;
							displacementROS.access_component(rotatorComponent) = 0.0f;
						}
					}
				}
				resultDisplacementRotation = displacementROS.to_quat();
			}

			displacementLinear = resultDisplacementLinear;
			velocityLinear = resultVelocityLinear;

			displacementOrientation = resultDisplacementRotation;
			velocityRotation = resultVelocityRotation;
		}

		if (m->triggerGameScriptExecutionTrapOnDone.is_valid() &&
			m->atDestination && !wasAtDestination)
		{
			GameScript::ScriptExecution::trigger_execution_trap(m->triggerGameScriptExecutionTrapOnDone);
		}
		else if (m->triggerGameScriptExecutionTrapOnDecelerate.is_valid() &&
				 m->decelerating && !wasDecelerating)
		{
			GameScript::ScriptExecution::trigger_execution_trap(m->triggerGameScriptExecutionTrapOnDecelerate);
		}

		if (m->playSound.is_valid())
		{
			if (shouldPlaySound)
			{
				if (!m->playedSound.get() ||
					!m->playedSound->is_active())
				{
					if (auto* s = get_owner()->get_sound())
					{
						m->playedSound = s->play_sound(m->playSound);
					}
				}
			}
			else
			{
				if (m->playedSound.get())
				{
					m->playedSound->stop();
					m->playedSound.clear();
				}
			}
		}
	}

	if (requiresRemoval)
	{
		for (int i = 0; i < movements.get_size(); ++i)
		{
			if (movements[i].active == 0.0f)
			{
				auto& m = movements[i];
				if (m.playedSound.get())
				{
					m.playedSound->stop();
					m.playedSound.clear();
				}

				movements.remove_at(i);
				--i;
			}
		}
	}

	// force displacements
	{
		_context.currentDisplacementLinear = displacementLinear.get(velocityLinear * _deltaTime);
		_context.currentDisplacementRotation = displacementOrientation.get((velocityRotation * _deltaTime).to_quat());
		_context.velocityLinear = velocityLinear;
		_context.velocityRotation = velocityRotation;
	}

	// won't do much work due to does_use_controls_for_movement() returning false)
	base::apply_changes_to_velocity_and_displacement(_deltaTime, _context);
}

bool ModuleMovementScripted::is_at_destination() const
{
	for_every(m, movements)
	{
		if ((m->active == 1.0f || m->activeTarget == 1.0f) && !m->atDestination)
		{
			return false;
		}
	}
	return true;
}

void ModuleMovementScripted::on_presence_changed_room(Framework::ModulePresence* _presence, Framework::Room* _intoRoom, Transform const& _intoRoomTransform, Framework::DoorInRoom* _exitThrough, Framework::DoorInRoom* _enterThrough, Framework::DoorInRoomArrayPtr const* _throughDoors)
{
	Concurrency::ScopedSpinLock lock(movementLock);

	for_every(m, movements)
	{
		m->startingPlacementWS = _intoRoomTransform.to_local(m->startingPlacementWS);
		m->targetPlacementWS = _intoRoomTransform.to_local(m->targetPlacementWS);
		if (m->useRandomOffsetLocationWS.is_set())
		{
			m->useRandomOffsetLocationWS = _intoRoomTransform.vector_to_local(m->useRandomOffsetLocationWS.get());
		}
	}
}

void ModuleMovementScripted::on_presence_added_to_room(Framework::ModulePresence* _presence, Framework::Room* _room, Framework::DoorInRoom* _enteredThroughDoor)
{
	// ?
}

void ModuleMovementScripted::on_presence_removed_from_room(Framework::ModulePresence* _presence, Framework::Room* _room)
{
	// ?
}

void ModuleMovementScripted::on_presence_destroy(Framework::ModulePresence* _presence)
{
	set_observe_presence(false);
}
