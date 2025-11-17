#include "acp_palmersCircles.h"

#include "..\..\appearanceControllers.h"

#include "..\..\..\..\core\mesh\pose.h"
#include "..\..\..\..\core\performance\performanceUtils.h"

#include "..\..\..\..\framework\appearance\appearanceController.h"
#include "..\..\..\..\framework\appearance\appearanceControllerPoseContext.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\world\room.h"

using namespace TeaForGodEmperor;
using namespace AppearanceControllersLib;
using namespace Particles;

//

DEFINE_STATIC_NAME(palmersCircles);

//

REGISTER_FOR_FAST_CAST(PalmersCircles);

PalmersCircles::PalmersCircles(PalmersCirclesData const * _data)
: base(_data)
, palmersCirclesData(_data)
{
}

void PalmersCircles::initialise(Framework::ModuleAppearance* _owner)
{
	base::initialise(_owner);
	palmers.set_size(particles.get_size());

	auto* owner = _owner->get_owner();
	centre = palmersCirclesData->centre.get(owner->get_variables(), palmersCirclesData->centre.get_value(), ShouldAllowToFail::AllowToFail);

	outerRing = palmersCirclesData->outerRing.get(owner->get_variables(), palmersCirclesData->outerRing.get_value(), ShouldAllowToFail::AllowToFail);
	innerRing = palmersCirclesData->innerRing.get(owner->get_variables(), palmersCirclesData->innerRing.get_value(), ShouldAllowToFail::AllowToFail);

	speed = palmersCirclesData->speed.get(owner->get_variables(), palmersCirclesData->speed.get_value(), ShouldAllowToFail::AllowToFail);
	speedKeptForTime = palmersCirclesData->speedKeptForTime.get(owner->get_variables(), palmersCirclesData->speedKeptForTime.get_value(), ShouldAllowToFail::AllowToFail);

	radiusKeptForTime = palmersCirclesData->radiusKeptForTime.get(owner->get_variables(), palmersCirclesData->radiusKeptForTime.get_value(), ShouldAllowToFail::AllowToFail);

}

void PalmersCircles::activate()
{
	for_every(palmer, palmers)
	{
		palmer->active = false;
	}
	base::activate();
}

void PalmersCircles::internal_activate_particle_in_ws(Particle * firstParticle, Particle* endParticle)
{
	base::internal_activate_particle_in_ws(firstParticle, endParticle);

	for (Particle* particle = firstParticle; particle != endParticle; ++particle)
	{
		Palmer& palmer = palmers[(int)(particle - particles.get_data())];
		palmer.active = true;
		palmer.speedKeptForTime = 0.0f;
		palmer.radiusKeptForTime = rg.get(radiusKeptForTime);
		palmer.findClosestIn = rg.get_float(0.5f, 1.5f);

		palmer.innerRingDeg = rg.get_float(0.0f, 360.0f);

		float outerRingSize = sqr(outerRing.max) - sqr(outerRing.min);
		float innerRingSize = sqr(innerRing.max) - sqr(innerRing.min);

		bool inInnerRing = rg.get_chance(innerRingSize / max(0.01f, innerRingSize + outerRingSize));

		Vector3 loc;
		float yaw;

		if (inInnerRing)
		{
			palmer.radius = rg.get(innerRing);
		}
		else
		{
			palmer.radius = rg.get(outerRing);
		}
		loc = centre + (Vector3::yAxis * palmer.radius).rotated_by_yaw(rg.get_float(0.0f, 360.0f));

		yaw = Rotator3::get_yaw(centre - loc);
		if (! inInnerRing)
		{
			yaw -= 90.0f;
		}

		// just randomise orientation, placement should be kept as it was set in base class
		particle->placement = Transform(loc, Rotator3(0.0f, yaw, 0.0f).to_quat());
		particle->velocityLinear = Vector3::zero;
		particle->velocityRotation = Rotator3::zero;
	}

}

void PalmersCircles::advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	Framework::IModulesOwner* imo = get_owner()->get_owner();

	MEASURE_PERFORMANCE(particles_palmersCircles_preliminaryPose);

	float deltaTime = _context.get_delta_time();

#ifdef AN_DEVELOPMENT
#ifndef AN_CLANG
	Transform ownerPlacement = get_owner_placement_in_proper_space();
#endif
#endif

	bool advance = imo->was_recently_seen_by_player();
	for_every(particle, particles)
	{
		Palmer& palmer = palmers[(int)(particle - particles.get_data())];
		if (palmer.active)
		{
			particle->alpha = 1.0f;
			particle->dissolve = 0.0f;
			particle->placement.set_scale(1.0f);

			if (advance)
			{
				Vector3 loc = particle->placement.get_translation();
				float yaw = particle->placement.get_orientation().to_rotator().yaw;

				float atCircleDeg = Rotator3::get_yaw(centre - loc);
				Vector3 locOffCentre = loc - centre;

				bool inInnerRing = innerRing.does_contain(palmer.radius);

				float targetYaw = yaw;
				Vector3 targetVelocity = palmer.velocity;
				Vector3 target = loc;

				palmer.findClosestIn -= deltaTime;
				if (palmer.findClosestIn <= 0.0f)
				{
					palmer.findClosestIn = rg.get_float(0.5f, 1.5f);
					palmer.closest.clear();
					ARRAY_STACK(float, closestDist, palmer.closest.get_max_size());
					for_every(op, particles)
					{
						if (op != particle)
						{
							float dist = (op->placement.get_translation() - loc).length_squared_2d();
							bool placed = false;
							for_every(cd, closestDist)
							{
								if (dist < *cd)
								{
									int idx = for_everys_index(cd);
									closestDist.insert_at(idx, dist);
									palmer.closest.insert_at(idx, &palmers[(int)(op - particles.get_data())]);
									placed = true;
									break;
								}
							}
							if (!placed && closestDist.has_place_left())
							{
								closestDist.push_back(dist);
								palmer.closest.push_back(&palmers[(int)(op - particles.get_data())]);
							}
						}
					}
				}

				palmer.radiusKeptForTime -= deltaTime;
				if (palmer.radiusKeptForTime <= 0.0f)
				{
					palmer.radiusKeptForTime = rg.get(radiusKeptForTime);
					palmer.radius = inInnerRing ? innerRing.clamp(palmer.radius + rg.get_float(-1.0f, 1.0f)) : rg.get(outerRing);
					palmer.innerRingDeg += rg.get_float(-30.0f, 30.0f);
					palmer.innerRingDeg = Rotator3::normalise_axis(palmer.innerRingDeg);
				}

				palmer.speedKeptForTime -= deltaTime;
				if (palmer.speedKeptForTime <= 0.0f)
				{
					palmer.speedKeptForTime = rg.get(speedKeptForTime);
					palmer.speed = rg.get(speed);
				}

				if (inInnerRing)
				{
					targetYaw = Rotator3::get_yaw(-locOffCentre);

					float diffDeg = Rotator3::normalise_axis(palmer.innerRingDeg - atCircleDeg);
					target = centre + locOffCentre.rotated_by_yaw(clamp(diffDeg, -10.0f, 10.0f)).normal() * palmer.radius;

					if ((target - loc).length() < 0.5f)
					{
						// we moved, stay here
						palmer.speed = 0.0f;
					}
					else if (palmer.speed == 0.0f)
					{
						// move to our target loc
						palmer.speed = speed.min * 0.5f;
					}

					targetVelocity = (target - loc).normal() * palmer.speed;
				}
				else
				{
					target = centre + locOffCentre.rotated_by_yaw(10.0f).normal() * palmer.radius;

					targetYaw = Rotator3::get_yaw(target - loc);

					targetVelocity = (Vector3::yAxis * palmer.speed).rotated_by_yaw(targetYaw);
				}

				{
					Vector3 pushAwayVelocity = Vector3::zero;
					float const pushAwayDist = 1.1f;
					for_every_ptr(op, palmer.closest)
					{
						Vector3 opLoc = particles[(int)(op - palmers.get_data())].placement.get_translation();
						float dist = (opLoc - loc).length_2d();

						float pushAwayStrength = clamp(4.0f * (1.0f - dist / pushAwayDist), 0.0f, 1.0f);
						if (pushAwayStrength > 0.0f)
						{
							pushAwayVelocity += (loc - opLoc).normal_2d() * pushAwayStrength;
						}
					}
					if (!pushAwayVelocity.is_zero())
					{
						float maxSpeed = max(targetVelocity.length(), speed.min);
						targetVelocity += pushAwayVelocity;
						if (targetVelocity.length() > maxSpeed)
						{
							targetVelocity = targetVelocity.normal() * maxSpeed;
						}
					}
				}

				palmer.velocity = blend_to_using_time(palmer.velocity, targetVelocity, 0.2f, deltaTime);
				loc += palmer.velocity * deltaTime;

				targetYaw = yaw + Rotator3::normalise_axis(targetYaw - yaw);
				yaw = blend_to_using_speed(yaw, targetYaw, 90.0f, deltaTime);

				particle->placement = Transform(loc, Rotator3(0.0f, yaw, 0.0f).to_quat());
			}
		}
		else
		{
			particle->timeBeingAlive = -1.0f; // stay there
			particle->placement.set_scale(0.0f);
		}
	}

	base::advance_and_adjust_preliminary_pose(_context);
}

//

REGISTER_FOR_FAST_CAST(PalmersCirclesData);

Framework::AppearanceControllerData* PalmersCirclesData::create_data()
{
	return new PalmersCirclesData();
}

void PalmersCirclesData::register_itself()
{
	Framework::AppearanceControllers::register_appearance_controller(NAME(palmersCircles), create_data);
}

PalmersCirclesData::PalmersCirclesData()
{
	inLocalSpace = false; // by default we are in local space!
	initialPlacement = Framework::AppearanceControllersLib::ParticlesPlacement::Undefined;
}

bool PalmersCirclesData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= centre.load_from_xml_child_node(_node, TXT("centre"));

	result &= outerRing.load_from_xml(_node, TXT("outerRing"));
	result &= innerRing.load_from_xml(_node, TXT("innerRing"));

	result &= speed.load_from_xml(_node, TXT("speed"));
	result &= speedKeptForTime.load_from_xml(_node, TXT("speedKeptForTime"));

	result &= radiusKeptForTime.load_from_xml(_node, TXT("radiusKeptForTime"));

	return result;
}

Framework::AppearanceControllerData* PalmersCirclesData::create_copy() const
{
	PalmersCirclesData* copy = new PalmersCirclesData();
	*copy = *this;
	return copy;
}

Framework::AppearanceController* PalmersCirclesData::create_controller()
{
	return new PalmersCircles(this);
}
