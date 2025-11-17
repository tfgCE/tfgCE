#include "acp_airTraffic.h"

#include "..\..\appearanceControllers.h"

#include "..\..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\..\core\mesh\pose.h"
#include "..\..\..\..\core\performance\performanceUtils.h"

#include "..\..\..\..\framework\framework.h"
#include "..\..\..\..\framework\appearance\appearanceController.h"
#include "..\..\..\..\framework\appearance\appearanceControllerPoseContext.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define DEBUG_DRAW_AT

//

using namespace TeaForGodEmperor;
using namespace AppearanceControllersLib;
using namespace Particles;

//

// patterns
DEFINE_STATIC_NAME(circle);
DEFINE_STATIC_NAME(freeRange);
DEFINE_STATIC_NAME(inDir);

// appearance controller name
DEFINE_STATIC_NAME(airTraffic);

//

REGISTER_FOR_FAST_CAST(AirTraffic);

AirTraffic::AirTraffic(AirTrafficData const * _data)
: base(_data)
{
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(pattern);

	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(circlePattern.radius);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(circlePattern.verticalOffset);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(circlePattern.moveClockwiseChance);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(circlePattern.moveCounterClockwiseChance);

	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(freeRangePattern.radius);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(freeRangePattern.verticalOffset);

	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(inDirPattern.yaw);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(inDirPattern.rangeX);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(inDirPattern.rangeY);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(inDirPattern.verticalOffset);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(inDirPattern.looped);

	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(rotationOffset.applyDirToPitch);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(rotationOffset.applyAccelXToRoll);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(rotationOffset.pitchBlendTime);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(rotationOffset.rollBlendTime);

	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(movementSpeed.speed);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(movementSpeed.changeInterval);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(movementSpeed.acceleration);

	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(targetDir.pitchOffset);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(targetDir.yawOffset);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(targetDir.changeInterval);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(targetDir.maxChangePerSecond);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(targetDir.blendTime);
}

void AirTraffic::initialise(Framework::ModuleAppearance* _owner)
{
	base::initialise(_owner);

	elements.set_size(particles.get_size());
}

void AirTraffic::activate()
{
	base::activate();
}

void AirTraffic::internal_activate_particle_in_ws(Particle * firstParticle, Particle* endParticle)
{
	base::internal_activate_particle_in_ws(firstParticle, endParticle);

	Transform placementWS = get_owner()->get_owner()->get_presence()->get_placement();

	for (Particle* particle = firstParticle; particle != endParticle; ++particle)
	{
		Element& e = elements[(int)(particle - particles.get_data())];

		e.dir = Vector3::zero;

		e.targetDirOffset = Rotator3::zero; // applied on top of pattern target dir
		e.rotationOffset = Rotator3::zero;

		e.targetSpeed = rg.get(movementSpeed.speed);

		// only location is important for placement
		Vector3 newLoc = Vector3::zero;

		if (pattern == NAME(circle))
		{
			bool moveClockwise = false;
			{
				float sumChance = circlePattern.moveClockwiseChance + circlePattern.moveCounterClockwiseChance;
				if (sumChance == 0.0f)
				{
					moveClockwise = rg.get_bool();
				}
				else if (circlePattern.moveCounterClockwiseChance == 0.0f)
				{
					moveClockwise = true;
				}
				else
				{
					moveClockwise = rg.get_float(0.0f, sumChance) < circlePattern.moveClockwiseChance;
				}
			}

			e.patternTargetDir = Rotator3(0.0f, moveClockwise ? 90.0f : -90.0f, 0.0f);

			newLoc = Rotator3(0.0f, rg.get_float(-180.0f, 180.0f), 0.0f).get_forward();
			newLoc *= rg.get(circlePattern.radius);
			newLoc.z = rg.get(circlePattern.verticalOffset);
		}
		else if (pattern == NAME(freeRange))
		{
			e.patternTargetDir = Rotator3(0.0f, rg.get_float(-180.0f, 180.0f), 0.0f);

			newLoc = Rotator3(0.0f, rg.get_float(-180.0f, 180.0f), 0.0f).get_forward();
			newLoc *= rg.get_float(0.0f, freeRangePattern.radius);
			newLoc.z = rg.get(freeRangePattern.verticalOffset);
		}
		else if (pattern == NAME(inDir))
		{
			e.patternTargetDir = Rotator3(0.0f, inDirPattern.yaw, 0.0f);

			newLoc.x = rg.get(inDirPattern.rangeX);
			newLoc.y = rg.get(inDirPattern.rangeY);
			newLoc.z = rg.get(inDirPattern.verticalOffset);
		}

		particle->placement = Transform(newLoc, Quat::identity);
		particle->placement = placementWS.to_world(particle->placement); // we have to provide result in WS

		if (movementSpeed.changeInterval.is_empty())
		{
			e.speedChangeTimeLeft.clear();
			e.targetSpeed = rg.get(movementSpeed.speed);
		}
		else
		{
			e.speedChangeTimeLeft = 0.0f; // should start with doing offset immediately
		}

		if (targetDir.changeInterval.is_empty())
		{
			e.targetDirChangeTimeLeft.clear();
			e.targetDirOffset = Rotator3::zero;
			e.targetDirOffset.pitch = rg.get(targetDir.pitchOffset);
			e.targetDirOffset.yaw = rg.get(targetDir.yawOffset);
		}
		else
		{
			e.targetDirChangeTimeLeft = 0.0f; // should start with doing offset immediately
		}
	}
}

void AirTraffic::advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	Framework::IModulesOwner* imo = get_owner()->get_owner();

	MEASURE_PERFORMANCE(particles_airTraffic_preliminaryPose);

	float deltaTime = _context.get_delta_time();

#ifdef AN_DEVELOPMENT
#ifndef AN_CLANG
	Transform ownerPlacement = get_owner_placement_in_proper_space();
#endif
#endif

	bool advance = imo->was_recently_seen_by_player();

	if (!advance)
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
		if (!Framework::is_preview_game())
#endif
		{
			return;
		}
	}

#ifdef DEBUG_DRAW_AT
	debug_context(imo->get_presence()->get_in_room());
	debug_filter(apc_airTraffic); 

	debug_push_transform(imo->get_presence()->get_placement());

#endif

	for_every(particle, particles)
	{
		Element& e = elements[(int)(particle - particles.get_data())];

		Vector3 loc = particle->placement.get_translation();

		if (e.speedChangeTimeLeft.is_set())
		{
			e.speedChangeTimeLeft = e.speedChangeTimeLeft.get() - deltaTime;
			if (e.speedChangeTimeLeft.get() < 0.0f)
			{
				e.speedChangeTimeLeft = rg.get(movementSpeed.changeInterval);
				e.targetSpeed = rg.get(movementSpeed.speed);
			}
		}

		if (e.targetDirChangeTimeLeft.is_set())
		{
			e.targetDirChangeTimeLeft = e.targetDirChangeTimeLeft.get() - deltaTime;
			if (e.targetDirChangeTimeLeft.get() < 0.0f)
			{
				e.targetDirChangeTimeLeft = rg.get(targetDir.changeInterval);
				e.targetDirOffset = Rotator3::zero;
				e.targetDirOffset.pitch = rg.get(targetDir.pitchOffset);
				e.targetDirOffset.yaw = rg.get(targetDir.yawOffset);
			}
		}

		Rotator3 prcTargetDir = e.patternTargetDir;

		// calculate current target dir from pattern
		if (pattern == NAME(circle))
		{
			float dist2d = loc.length_2d();

			if (loc.z < circlePattern.verticalOffset.min)
			{
				e.targetDirOffset.pitch = abs(e.targetDirOffset.pitch);
			}
			if (loc.z > circlePattern.verticalOffset.max)
			{
				e.targetDirOffset.pitch = -abs(e.targetDirOffset.pitch);
			}

			Optional<float> correctDist;

			if (dist2d < circlePattern.radius.min)
			{
				e.targetDirOffset.yaw = abs(e.targetDirOffset.yaw) * (e.patternTargetDir.yaw > 0.0f? -1.0f : 1.0f);
				correctDist = circlePattern.radius.min;
			}
			if (dist2d > circlePattern.radius.max)
			{
				e.targetDirOffset.yaw = abs(e.targetDirOffset.yaw) * (e.patternTargetDir.yaw > 0.0f ? 1.0f : -1.0f);
				correctDist = circlePattern.radius.max;
			}

			if (correctDist.is_set())
			{
				float z = loc.z;
				loc = loc.normal_2d() * correctDist.get();
				loc.z = z;
			}
		}
		if (pattern == NAME(freeRange))
		{
			float dist2d = loc.length_2d();

			if (loc.z < freeRangePattern.verticalOffset.min)
			{
				e.targetDirOffset.pitch = abs(e.targetDirOffset.pitch);
			}
			if (loc.z > freeRangePattern.verticalOffset.max)
			{
				e.targetDirOffset.pitch = -abs(e.targetDirOffset.pitch);
			}

			if (dist2d > freeRangePattern.radius)
			{
				if (Vector3::dot(prcTargetDir.get_forward(), loc) > 0.0f)
				{
					e.patternTargetDir.yaw = Rotator3::get_yaw(loc) + 180.0f + rg.get_float(-50.0f, 50.0f);
					e.patternTargetDir.yaw = Rotator3::normalise_axis(e.patternTargetDir.yaw);
					prcTargetDir = e.patternTargetDir;
				}
			}
		}
		if (pattern == NAME(inDir))
		{
			if (loc.z < inDirPattern.verticalOffset.min)
			{
				e.targetDirOffset.pitch = abs(e.targetDirOffset.pitch);
			}
			if (loc.z > inDirPattern.verticalOffset.max)
			{
				e.targetDirOffset.pitch = -abs(e.targetDirOffset.pitch);
			}

			if (inDirPattern.looped)
			{
				if (loc.x < inDirPattern.rangeX.min)
				{
					loc.x += inDirPattern.rangeX.length();
				}
				else if (loc.x > inDirPattern.rangeX.max)
				{
					loc.x -= inDirPattern.rangeX.length();
				}
				if (loc.y < inDirPattern.rangeY.min)
				{
					loc.y += inDirPattern.rangeY.length();
				}
				else if (loc.y > inDirPattern.rangeY.max)
				{
					loc.y -= inDirPattern.rangeY.length();
				}
			}
		}

		prcTargetDir += e.targetDirOffset;
		
		Vector3 prcTargetDirV = prcTargetDir.get_forward();

		if (e.dir.is_zero()) e.dir = prcTargetDirV;
		if (e.speed == 0.0f) e.speed = e.targetSpeed;

		Vector3 prevVelocity = e.dir * e.speed;

		{
			Vector3 newEDir = blend_to_using_time(e.dir, prcTargetDirV, targetDir.blendTime, deltaTime).normal();
			Vector3 diff = newEDir - e.dir;
			if (targetDir.maxChangePerSecond > 0.0f)
			{
				diff = diff.normal() * min(diff.length(), targetDir.maxChangePerSecond * deltaTime);
				e.dir = (e.dir + diff).normal();
			}
			else
			{
				e.dir = newEDir;
			}
		}

		if (e.speed < e.targetSpeed)
		{
			e.speed = min(e.targetSpeed, e.speed + movementSpeed.acceleration * deltaTime);
		}
		else
		{
			e.speed = max(e.targetSpeed, e.speed - movementSpeed.acceleration * deltaTime);
		}

		Vector3 currVelocity = e.dir * e.speed;
		Vector3 fwd = e.dir;

		if (pattern == NAME(circle))
		{
			// apply circular movement
			float dist2d = loc.length_2d();
			float timeRef = 1.0f; // we shouldn't be working on too small values and yet we shouldn't be doing very small circles
			float deltaTimeRef = deltaTime / timeRef;
			Vector3 coveredRef = currVelocity * timeRef;
			float coveredRefAngle = 360.0f * (coveredRef.x / (2.0f * pi<float>() * dist2d));
			float yaw = Rotator3::get_yaw(loc);
			float newYaw = yaw + coveredRefAngle * deltaTimeRef;
			float newDist2d = dist2d + coveredRef.y * deltaTimeRef;
			float newZ = loc.z + coveredRef.z * deltaTimeRef;
			loc = (Vector3::yAxis * newDist2d).rotated_by_yaw(newYaw);
			loc.z = newZ;
			fwd.rotate_by_yaw(newYaw);
		}
		else
		{
			loc += currVelocity * deltaTime;
		}

		Transform placement = look_matrix(loc, fwd, Vector3::zAxis).to_transform();

		Rotator3 targetRotationOffset = Rotator3::zero;
		if (rotationOffset.applyDirToPitch != 0.0f)
		{
			targetRotationOffset.pitch = e.dir.to_rotator().pitch * rotationOffset.applyDirToPitch;
		}
		if (rotationOffset.applyAccelXToRoll != 0.0f)
		{
			Vector3 accel = deltaTime != 0.0f ? (currVelocity - prevVelocity) / deltaTime : Vector3::zero;

			Vector3 accelOS = placement.vector_to_local(accel);

			targetRotationOffset.roll = accelOS.x * rotationOffset.applyAccelXToRoll;
		}

		e.rotationOffset.pitch = blend_to_using_time(e.rotationOffset.pitch, targetRotationOffset.pitch, rotationOffset.pitchBlendTime, deltaTime);
		e.rotationOffset.roll = blend_to_using_time(e.rotationOffset.roll, targetRotationOffset.roll, rotationOffset.rollBlendTime, deltaTime);

		placement.set_orientation(placement.get_orientation().to_world(e.rotationOffset.to_quat()));

		particle->alpha = 1.0f;
		particle->dissolve = 0.0f;
		particle->placement.set_scale(1.0f);
		particle->placement = placement;

#ifdef DEBUG_DRAW_AT
		debug_draw_line(true, Colour::greyLight, Vector3::zero, loc);
		debug_draw_line(true, Colour::red, loc, loc + fwd);
#endif
	}

#ifdef DEBUG_DRAW_AT
	debug_pop_transform();
	debug_no_filter();
	debug_no_context();
#endif

	base::advance_and_adjust_preliminary_pose(_context);
}

//

REGISTER_FOR_FAST_CAST(AirTrafficData);

Framework::AppearanceControllerData* AirTrafficData::create_data()
{
	return new AirTrafficData();
}

void AirTrafficData::register_itself()
{
	Framework::AppearanceControllers::register_appearance_controller(NAME(airTraffic), create_data);
}

AirTrafficData::AirTrafficData()
{
	inLocalSpace = false; // by default we are in local space!
	initialPlacement = Framework::AppearanceControllersLib::ParticlesPlacement::Undefined;
}

bool AirTrafficData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	XML_LOAD(_node, pattern);

	for_every(node, _node->children_named(TXT("circlePattern")))
	{
		pattern = NAME(circle);
		XML_LOAD_STR(node, circlePattern.radius, TXT("radius"));
		XML_LOAD_STR(node, circlePattern.verticalOffset, TXT("verticalOffset"));
		XML_LOAD_STR(node, circlePattern.moveClockwiseChance, TXT("moveClockwiseChance"));
		XML_LOAD_STR(node, circlePattern.moveCounterClockwiseChance, TXT("moveCounterClockwiseChance"));
	}

	for_every(node, _node->children_named(TXT("freeRangePattern")))
	{
		pattern = NAME(freeRange);
		XML_LOAD_STR(node, freeRangePattern.radius, TXT("radius"));
		XML_LOAD_STR(node, freeRangePattern.verticalOffset, TXT("verticalOffset"));
	}

	for_every(node, _node->children_named(TXT("inDirPattern")))
	{
		pattern = NAME(inDir);
		XML_LOAD_STR(node, inDirPattern.yaw, TXT("yaw"));
		XML_LOAD_STR(node, inDirPattern.rangeX, TXT("rangeX"));
		XML_LOAD_STR(node, inDirPattern.rangeY, TXT("rangeY"));
		XML_LOAD_STR(node, inDirPattern.verticalOffset, TXT("verticalOffset"));
		XML_LOAD_STR(node, inDirPattern.looped, TXT("looped"));
	}

	for_every(node, _node->children_named(TXT("rotationOffset")))
	{
		XML_LOAD_STR(node, rotationOffset.applyDirToPitch, TXT("applyDirToPitch"));
		XML_LOAD_STR(node, rotationOffset.applyAccelXToRoll, TXT("applyAccelXToRoll"));
		XML_LOAD_STR(node, rotationOffset.pitchBlendTime, TXT("blendTime"));
		XML_LOAD_STR(node, rotationOffset.pitchBlendTime, TXT("pitchBlendTime"));
		XML_LOAD_STR(node, rotationOffset.rollBlendTime, TXT("blendTime"));
		XML_LOAD_STR(node, rotationOffset.rollBlendTime, TXT("rollBlendTime"));
	}

	for_every(node, _node->children_named(TXT("movementSpeed")))
	{
		XML_LOAD_STR(node, movementSpeed.speed, TXT("speed"));
		XML_LOAD_STR(node, movementSpeed.changeInterval, TXT("changeInterval"));
		XML_LOAD_STR(node, movementSpeed.acceleration, TXT("acceleration"));
	}

	for_every(node, _node->children_named(TXT("targetDir")))
	{
		XML_LOAD_STR(node, targetDir.pitchOffset, TXT("pitchOffset"));
		XML_LOAD_STR(node, targetDir.yawOffset, TXT("yawOffset"));
		XML_LOAD_STR(node, targetDir.changeInterval, TXT("changeInterval"));
		XML_LOAD_STR(node, targetDir.maxChangePerSecond, TXT("maxChangePerSecond"));
		XML_LOAD_STR(node, targetDir.blendTime, TXT("blendTime"));
	}

	return result;
}

Framework::AppearanceControllerData* AirTrafficData::create_copy() const
{
	AirTrafficData* copy = new AirTrafficData();
	*copy = *this;
	return copy;
}

Framework::AppearanceController* AirTrafficData::create_controller()
{
	return new AirTraffic(this);
}
