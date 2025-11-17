#include "moduleMovementPath.h"

#include "registeredModule.h"
#include "modules.h"

#include "..\advance\advanceContext.h"

#include "..\..\core\performance\performanceUtils.h"

using namespace Framework;

//

DEFINE_STATIC_NAME(speed);
DEFINE_STATIC_NAME(acceleration);

//

REGISTER_FOR_FAST_CAST(ModuleMovementPath);

static ModuleMovement* create_module(IModulesOwner* _owner)
{
	return new ModuleMovementPath(_owner);
}

RegisteredModule<ModuleMovement> & ModuleMovementPath::register_itself()
{
	return Modules::movement.register_module(String(TXT("path")), create_module);
}

ModuleMovementPath::ModuleMovementPath(IModulesOwner* _owner)
: base(_owner)
{
	requiresPrepareMove = false;
}

void ModuleMovementPath::setup_with(ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		speed = _moduleData->get_parameter<float>(this, NAME(speed), speed);
		acceleration = _moduleData->get_parameter<float>(this, NAME(acceleration), acceleration);
	}
}

void ModuleMovementPath::use_linear_path(Transform const & _a, Transform const & _b, float _t)
{
	path = P_Linear;
	pathLinear.a = _a;
	pathLinear.b = _b;
	// t
	t = _t;
	be_initial_placement();
	// leave target as is
}

void ModuleMovementPath::use_curve_single_path(Transform const & _a, Transform const & _b, BezierCurve<Vector3> const& _curve, float _t, Optional<CurveSinglePathExtraParams> const& _extraParams)
{
	path = P_CurveSingle;
	pathCurveSingle.a = _a;
	pathCurveSingle.b = _b;
	pathCurveSingle.curve = _curve;
	pathCurveSingle.extraParams = _extraParams.get(CurveSinglePathExtraParams());
	// reset others
	pathCurveSingle.reorientAfter.clear();
	pathCurveSingle.alignAxisAlongT.clear();
	pathCurveSingle.alignAxisAlongTInvert = false;
	// t
	t = _t;
	be_initial_placement();
	// cache some values
	{
		Vector3 tangent0 = pathCurveSingle.curve.calculate_tangent_at(0.0f).normal();
		Vector3 p0to1 = (pathCurveSingle.curve.calculate_at(1.0f) - pathCurveSingle.curve.calculate_at(0.0f)).normal();
		Vector3 axis = Vector3::cross(tangent0, p0to1).normal();
		Vector3 perp0 = Vector3::cross(axis, tangent0).normal();
		pathCurveSingle.axis = axis;
		float dotP = Vector3::dot(perp0, p0to1); // > 0.0f means perp is inwards as it is
		pathCurveSingle.perpInwardsDirSign = dotP > 0.0f? 1.0f : -1.0f;
	}
	// leave target as is
}

void ModuleMovementPath::apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext & _context)
{
#ifdef AN_DEBUG_RENDERER
	debug_subject(get_owner());
	debug_filter(moduleMovementPath);
	debug_context(get_owner()->get_presence()->get_in_room());
#endif

	Transform const isAt = get_owner()->get_presence()->get_placement();
	Transform shouldBeAt = isAt;

	float dist = 0.0f;
	if (path == P_Linear)
	{
		dist = (pathLinear.a.get_translation() - pathLinear.b.get_translation()).length();
	}
	else if (path == P_CurveSingle)
	{
		dist = pathCurveSingle.curve.length();
	}
	if (dist != 0.0f)
	{
		float at = t * dist;

		if (this->targetT.is_set())
		{
			float const targetT = this->targetT.get();

			if (acceleration == 0.0f)
			{
				currentSpeed = targetT != t ? sign(targetT - t) * speed : 0.0f;
			}
			else
			{
				/*
				s = vt + at^2 / 2
				0 = v - at
				t = v / a

				s = v'/a + a * v' / a' / 2
				s = v'/a + v' / a / 2
				s = v'/2*a to slow down
				*/

				float const distLeft = (targetT - t) * dist;
				float const distToSlowDown = sqr(currentSpeed) / (2.0f * acceleration);
				float targetSpeed = 0.0f;
				bool const slowDown = abs(distToSlowDown) > abs(distLeft);
				if (!slowDown && targetT != t)
				{
					targetSpeed = sign(targetT - t) * speed;
				}
				currentSpeed = blend_to_using_speed(currentSpeed, targetSpeed, acceleration, _deltaTime);
			}
		}
		else
		{
			if (acceleration == 0.0f)
			{
				currentSpeed = 0.0f;
			}
			else
			{
				currentSpeed = blend_to_using_speed(currentSpeed, 0.0f, acceleration, _deltaTime);
			}
		}

		at = clamp(at + currentSpeed * _deltaTime, 0.0f, dist);
		
		t = at / dist;
	}

	if (path == P_Linear)
	{
#ifdef AN_DEBUG_RENDERER
		if (debug_is_filter_allowed())
		{
			debug_draw_line(true, Colour::orange, pathLinear.a.get_translation(), pathLinear.b.get_translation());
		}
#endif

		shouldBeAt = Transform::lerp(t, pathLinear.a, pathLinear.b);
	}
	else if (path == P_CurveSingle)
	{
#ifdef AN_DEBUG_RENDERER
		if (debug_is_filter_allowed())
		{
			float t = 0.0f;
			Optional<Vector3> prev;
			while (t <= 1.0f)
			{
				Vector3 curr = pathCurveSingle.curve.calculate_at(t);
				if (prev.is_set())
				{
					debug_draw_line(true, Colour::orange, prev.get(), curr);
				}
				prev = curr;
				t += 0.05f;
			}
		}
#endif
		float curveT = pathCurveSingle.curve.get_t_at_distance(t * dist);
		shouldBeAt = Transform::lerp(t, pathCurveSingle.a, pathCurveSingle.b);
		shouldBeAt.set_translation(pathCurveSingle.curve.calculate_at(curveT));
		float applyAlong = 1.0f;
		if (pathCurveSingle.reorientAfter.is_set())
		{
			if (pathCurveSingle.reorientAfter.get() != 1.0f)
			{
				applyAlong = clamp((1.0f - t) / (1.0f - pathCurveSingle.reorientAfter.get()), 0.0f, 1.0f);
			}
			else
			{
				applyAlong = t < 1.0f ? 1.0f : 0.0f;
			}
		}
		if (pathCurveSingle.alignAxisAlongT.is_set())
		{
			Vector3 t = pathCurveSingle.curve.calculate_tangent_at(curveT) * (pathCurveSingle.alignAxisAlongTInvert ? -1.0f : 1.0f);
			Transform rotated;
			if (pathCurveSingle.alignAxisAlongT.get() == Axis::Z)
			{
				rotated.build_from_axes(Axis::Z, Axis::Y, shouldBeAt.get_axis(Axis::Y), shouldBeAt.get_axis(Axis::X), t, shouldBeAt.get_translation());
			}
			else if (pathCurveSingle.alignAxisAlongT.get() == Axis::Y)
			{
				rotated.build_from_axes(Axis::Y, Axis::Z, t, shouldBeAt.get_axis(Axis::X), shouldBeAt.get_axis(Axis::Z), shouldBeAt.get_translation());
			}
			else
			{
				rotated.build_from_axes(Axis::X, Axis::Z, shouldBeAt.get_axis(Axis::Y), t, shouldBeAt.get_axis(Axis::Z), shouldBeAt.get_translation());
			}
			if (applyAlong < 1.0f)
			{
				shouldBeAt = Transform::lerp(applyAlong, shouldBeAt, rotated);
			}
			else
			{
				shouldBeAt = rotated;
			}
		}
		if (pathCurveSingle.extraParams.moveInwards.get(0.0f) != 0.0f)
		{
			float moveInwardsT = pathCurveSingle.extraParams.moveInwardsT.get(1.0f);
			moveInwardsT = min(0.5f, moveInwardsT);
			if (moveInwardsT != 0.0f)
			{
				moveInwardsT = (t < 0.5f ? t : (1.0f - t)) / moveInwardsT;
				moveInwardsT = clamp(moveInwardsT, 0.0f, 1.0f);

				Vector3 inwards;

				// find inwards dir
				{
					// find direction inwards the path
					Vector3 tangentT = pathCurveSingle.curve.calculate_tangent_at(t);
					Vector3 perpT = Vector3::cross(pathCurveSingle.axis, tangentT).normal();

					inwards = perpT * pathCurveSingle.perpInwardsDirSign;
				}

				// move off a bit
				shouldBeAt.set_translation(shouldBeAt.get_translation() + inwards * moveInwardsT * pathCurveSingle.extraParams.moveInwards.get());
			}
		}
	}

	_context.currentDisplacementLinear = shouldBeAt.get_translation() - isAt.get_translation();
	_context.currentDisplacementRotation = isAt.get_orientation().to_local(shouldBeAt.get_orientation());
	an_assert(_context.currentDisplacementRotation.is_normalised(0.1f));

	if (initialPlacement)
	{
		initialPlacement = false;
		_context.velocityLinear = Vector3::zero;
		_context.velocityRotation = Rotator3::zero;
	}
	else
	{
		float invDeltaTime = _deltaTime != 0.0f ? 1.0f / _deltaTime : 0.0f;
		_context.velocityLinear = (shouldBeAt.get_translation() - isAt.get_translation()) * invDeltaTime;
		_context.velocityRotation = (isAt.get_orientation().to_local(shouldBeAt.get_orientation())).to_rotator() * invDeltaTime;
	}

	// won't do much work due to does_use_controls_for_movement() returning false)
	base::apply_changes_to_velocity_and_displacement(_deltaTime, _context);

#ifdef AN_DEBUG_RENDERER
	debug_no_context();
	debug_no_filter();
	debug_no_subject();
#endif
}
