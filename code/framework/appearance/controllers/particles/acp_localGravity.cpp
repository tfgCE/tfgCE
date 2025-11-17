#include "acp_localGravity.h"

#include "..\..\appearanceControllerPoseContext.h"
#include "..\..\appearanceControllers.h"

#include "..\..\meshParticles.h"

#include "..\..\..\module\moduleAppearance.h"
#include "..\..\..\module\moduleCollision.h"

#include "..\..\..\..\core\mesh\pose.h"

#include "..\..\..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;
using namespace Particles;

//

DEFINE_STATIC_NAME(particlesLocalGravity);

//

REGISTER_FOR_FAST_CAST(LocalGravity);

LocalGravity::LocalGravity(LocalGravityData const * _data)
: base(_data)
, localGravityData(_data)
{
	useRoomVelocity = 0.0f;
}

void LocalGravity::activate()
{
	scaleInitialPlacement = localGravityData->scaleInitialPlacement;
	initialPlacementOffset = localGravityData->initialPlacementOffset;
	initialPlacementAtYaw = localGravityData->initialPlacementAtYaw;
	initialDistance = localGravityData->initialDistance;
	if (localGravityData->useAttachedToForInitialPlacementOffset)
	{
		IModulesOwner* imo = get_owner()->get_owner();
		an_assert(imo->get_presence());
		if (auto* ato = imo->get_presence()->get_attached_to())
		{
			auto* atop = ato->get_presence();
			an_assert(atop);
			if (auto* c = ato->get_collision())
			{
				float radius = c->get_collision_primitive_radius();
				Vector3 centreDistance = c->get_collision_primitive_centre_distance();
				Vector3 offset = c->get_collision_primitive_centre_offset();
				if (radius == 0.0f)
				{
					// if collision does not provide anything useful, revert to presence
					radius = atop->get_presence_radius() * 0.6f;
					centreDistance = atop->get_presence_centre_distance();
					offset = atop->get_centre_of_presence_os().get_translation(); // should be relative to centre of presence
				}

				an_assert(radius != 0.0f);

				initialPlacementOffset = Range3::empty;
				initialPlacementOffset.include(offset + centreDistance * 0.5f);
				initialPlacementOffset.include(offset - centreDistance * 0.5f);

				radius *= 0.8f; // make it smaller
				initialPlacementOffset.expand_by(Vector3(radius) * localGravityData->useAttachedToForInitialPlacementOffsetAmount);
				if (initialPlacementOffset.x.min == initialPlacementOffset.x.max) { initialPlacementOffset.x.expand_by(0.01f); }
				if (initialPlacementOffset.y.min == initialPlacementOffset.y.max) { initialPlacementOffset.y.expand_by(0.01f); }
				if (initialPlacementOffset.z.min == initialPlacementOffset.z.max) { initialPlacementOffset.z.expand_by(0.01f); }
				initialDistance = localGravityData->useAttachedToForInitialPlacementOffsetRadiusPt * radius;
			}
		}
		else
		{
			warn_dev_investigate(TXT("local gravity particles meant to be used for attached to are not attached to anything"));
		}
	}
	initialPlacementBox.setup(initialPlacementOffset);

	// although it is a box, it is axis aligned as we based it on range3, remember that!
	initialPlacementBoxCollapsed = initialPlacementBox;
	Vector3 halfSize = initialPlacementBoxCollapsed.get_half_size();
	float minSize = min(halfSize.x, min(halfSize.y, halfSize.z));
	halfSize.x -= minSize;
	halfSize.y -= minSize;
	halfSize.z -= minSize;
	initialPlacementBoxCollapsed.set_half_size(halfSize);

	base::activate();

	if (is_continuous_now())
	{
		for_every(particle, particles)
		{
			// negative time means it will be delayed
			particle->timeBeingAlive = min(0.0f, rg.get_float(-localGravityData->timeAlive * 0.9f, localGravityData->timeAlive * 0.1f));
			particle->activationFlags = ActivationFlags::MayActivate;
		}
	}
}

void LocalGravity::internal_activate_particle_in_ws(Particle * firstParticle, Particle* endParticle)
{
	base::internal_activate_particle_in_ws(firstParticle, endParticle);

	IModulesOwner* imo = get_owner()->get_owner();
	Transform ownerPlacement = imo->get_presence()->get_placement(); // always activate in WS

	Vector3 initialPlacementBoxCentre = initialPlacementBox.get_centre();
	for (Particle* particle = firstParticle; particle != endParticle; ++particle)
	{
		// just randomise orientation, placement should be kept as it was set in base class
		Vector3 loc; // local (to placement box) placement
		float distance = rg.get(initialDistance);

		Vector3 locVel = Vector3(rg.get_float(localGravityData->initialVelocityVector.x.min, localGravityData->initialVelocityVector.x.max),
								 rg.get_float(localGravityData->initialVelocityVector.y.min, localGravityData->initialVelocityVector.y.max),
								 rg.get_float(localGravityData->initialVelocityVector.z.min, localGravityData->initialVelocityVector.z.max));

		if (!initialPlacementAtYaw.is_empty())
		{
			loc = Vector3(rg.get_float(-1.0f, 1.0f),
						  rg.get_float(-1.0f, 1.0f),
						  rg.get_float(-1.0f, 1.0f));
			loc *= initialPlacementBox.get_half_size();
			
			float initialYaw = rg.get(initialPlacementAtYaw);
			loc += Vector3::yAxis.rotated_by_yaw(initialYaw) * (distance);

			locVel = locVel.rotated_by_yaw(initialYaw).normal();
		}
		else
		{
			loc = Vector3(rg.get_float(-1.0f, 1.0f),
						  rg.get_float(-1.0f, 1.0f),
						  rg.get_float(-1.0f, 1.0f));
			loc = initialPlacementBox.get_centre_offset_on_surface(loc);

			// if we're only on one side, use only that side
			loc.x = loc.x == 0.0f ? rg.get_float(-1.0f, 1.0f) : sign(loc.x) * rg.get_float(0.0f, 1.0f);
			loc.y = loc.y == 0.0f ? rg.get_float(-1.0f, 1.0f) : sign(loc.y) * rg.get_float(0.0f, 1.0f);
			loc.z = loc.z == 0.0f ? rg.get_float(-1.0f, 1.0f) : sign(loc.z) * rg.get_float(0.0f, 1.0f);

			// keep loc on one side if initial velocity points to one side - we don't want to collapse into what we are now
			if (localGravityData->initialVelocityVector.x.min > 0.0f) loc.x = abs(loc.x);
			if (localGravityData->initialVelocityVector.x.max < 0.0f) loc.x = -abs(loc.x);
			if (localGravityData->initialVelocityVector.y.min > 0.0f) loc.y = abs(loc.y);
			if (localGravityData->initialVelocityVector.y.max < 0.0f) loc.y = -abs(loc.y);
			if (localGravityData->initialVelocityVector.z.min > 0.0f) loc.z = abs(loc.z);
			if (localGravityData->initialVelocityVector.z.max < 0.0f) loc.z = -abs(loc.z);

			Vector3 dirLoc;
			float locDistance = initialPlacementBoxCollapsed.calculate_outside_distance_local(loc, &dirLoc);
			loc = loc + dirLoc * (distance - locDistance); // drop at centre and offset then

			// align with initial velocity vector range
			for_count(int, i, 5)
			{
				locVel = (locVel * 0.5f + 0.5f * locVel.drop_using_normalised(dirLoc)).normal();
				locVel = localGravityData->initialVelocityVector.clamp(locVel);
			}
		}
		locVel = locVel.normal();

		// mv2 = mgh
		// v2 = gh
		// v = \/gh
		float staySpeed = distance <= 0.0f? 0.0f : sqrt(max(0.0f, localGravityData->localGravity / distance));

		locVel *= rg.get(localGravityData->initialStaySpeed) * staySpeed;

		particle->placement.set_translation(ownerPlacement.location_to_world(initialPlacementBox.location_to_world(loc * scaleInitialPlacement)));
		particle->placement.set_orientation(Rotator3(rg.get_float(-90.0f, 90.0f), rg.get_float(-180.0f, 180.0f), rg.get_float(-180.0f, 180.0f)).to_quat());
		particle->velocityLinear = ownerPlacement.vector_to_world(initialPlacementBox.vector_to_world(locVel));

		float a = localGravityData->initialOrientationMaxSpeed;
		particle->velocityRotation = Rotator3(rg.get_float(-a, a), rg.get_float(-a, a), rg.get_float(-a, a));
	}
}

void LocalGravity::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	MEASURE_PERFORMANCE(particles_LocalGravity_preliminaryPose);

	float deltaTime = _context.get_delta_time();

	Transform ownerPlacement = get_owner_placement_in_proper_space();

	bool anyActive = is_continuous_now();

	for_every(particle, particles)
	{
		float timeLeft = localGravityData->timeAlive - particle->timeBeingAlive;
		float adjustTime = clamp(particle->randomNumber * localGravityData->timeAlive * localGravityData->timeAliveVarietyCoef, 0.0f, 0.999f);
		float adjustTimeAlive = adjustTime * localGravityData->timeAlive;

		if (!is_continuous_now() && particle->timeBeingAlive >= localGravityData->timeAlive)
		{
			// shouldn't be activated any more
			particle->activationFlags = 0; // shouldn't be activated
		}

		float actualTimeLeft = timeLeft - adjustTimeAlive;
		if ((is_flag_set(particle->activationFlags, ActivationFlags::MayActivate) && actualTimeLeft <= 0.0f && (is_continuous_now())) ||
			(is_flag_set(particle->activationFlags, ActivationFlags::MayActivate) && particle->timeBeingAlive + _context.get_delta_time() >= 0.0f
										  && particle->timeBeingAlive < localGravityData->timeAlive))
		{
			activate_particle(particle);
		}

		if (particle->timeBeingAlive >= 0.0f)
		{
			Vector3 loc = initialPlacementBox.location_to_local(ownerPlacement.location_to_local(particle->placement.get_translation()));
			Vector3 locVel = initialPlacementBox.vector_to_local(ownerPlacement.vector_to_local(particle->velocityLinear));

			Vector3 dirLoc;
			float locDistance = initialPlacementBoxCollapsed.calculate_outside_distance_local(loc, &dirLoc);

			if (locDistance != 0.0f)
			{
				locVel += (-dirLoc * localGravityData->localGravity / sqr(locDistance)) * deltaTime;
			}

			if (localGravityData->maxSpeed > 0.0f)
			{
				locVel = locVel.normal() * min(locVel.length(), localGravityData->maxSpeed);
			}
			particle->placement.set_translation(ownerPlacement.location_to_world(initialPlacementBox.location_to_world(loc)));
			particle->velocityLinear = ownerPlacement.vector_to_world(initialPlacementBox.vector_to_world(locVel));

			particle->alpha = localGravityData->calculate_alpha(particle->timeBeingAlive, localGravityData->timeAlive - adjustTimeAlive);
			particle->dissolve = localGravityData->calculate_dissolve(particle->timeBeingAlive, localGravityData->timeAlive - adjustTimeAlive);
			float actualScale = localGravityData->calculate_scale(particle->timeBeingAlive, localGravityData->timeAlive - adjustTimeAlive);
			float fullScale = localGravityData->calculate_full_scale(particle->timeBeingAlive, localGravityData->timeAlive - adjustTimeAlive);
			particle->placement.set_scale(actualScale);
			anyActive |= (fullScale > 0.0f && particle->alpha > 0.0f) || // visible
						 particle->timeBeingAlive == 0.0f; // just started

			if (initialPlacementBox.is_inside_local(loc) ||
				particle->timeBeingAlive >= localGravityData->timeAlive - adjustTimeAlive)
			{
				// will end life next frame
				particle->timeBeingAlive = localGravityData->timeAlive;
				if (is_continuous_now())
				{
					particle->activationFlags = ActivationFlags::MayActivate; // we should activate it again only if continuous
				}
			}
		}
		else
		{
			particle->placement.set_scale(0.0f);
			if (is_continuous_now())
			{
				particle->activationFlags = ActivationFlags::MayActivate; // we should activate it again only if continuous
			}
		}
	}

	base::advance_and_adjust_preliminary_pose(_context);

	if (! anyActive)
	{
		particles_deactivate();
	}
}

//

REGISTER_FOR_FAST_CAST(LocalGravityData);

AppearanceControllerData* LocalGravityData::create_data()
{
	return new LocalGravityData();
}

void LocalGravityData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(particlesLocalGravity), create_data);
}

LocalGravityData::LocalGravityData()
{
	initialPlacement = ParticlesPlacement::OwnerPlacement;
}

bool LocalGravityData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	localGravity = _node->get_float_attribute_or_from_child(TXT("localGravity"), localGravity);
	maxSpeed = _node->get_float_attribute_or_from_child(TXT("maxSpeed"), maxSpeed);
	timeAlive = _node->get_float_attribute_or_from_child(TXT("timeAlive"), timeAlive);
	timeAliveVarietyCoef = _node->get_float_attribute_or_from_child(TXT("timeAliveVarietyCoef"), timeAliveVarietyCoef);
	if (_node->first_child_named(TXT("initialVelocityVector")))
	{
		initialVelocityVector = Range3::zero;
		initialVelocityVector.load_from_xml_child_node(_node, TXT("initialVelocityVector"));
	}
	initialStaySpeed.load_from_xml(_node, TXT("initialStaySpeed"));
	initialStaySpeed.load_from_xml_child_node(_node, TXT("initialStaySpeed"));
	initialDistance.load_from_xml(_node, TXT("initialDistance"));
	initialDistance.load_from_xml_child_node(_node, TXT("initialDistance"));
	scaleInitialPlacement.load_from_xml_child_node(_node, TXT("scaleInitialPlacement"));
	useAttachedToForInitialPlacementOffset = _node->get_bool_attribute_or_from_child_presence(TXT("useAttachedToForInitialPlacementOffset"), useAttachedToForInitialPlacementOffset);
	useAttachedToForInitialPlacementOffsetAmount.load_from_xml_child_node(_node, TXT("useAttachedToForInitialPlacementOffset"));
	useAttachedToForInitialPlacementOffsetRadiusPt.load_from_xml_child_node(_node, TXT("useAttachedToForInitialPlacementOffset"), TXT("radiusPt"));
	if (_node->first_child_named(TXT("initialPlacementOffset")))
	{
		initialPlacementOffset = Range3::zero;
		initialPlacementOffset.load_from_xml_child_node(_node, TXT("initialPlacementOffset"));
	}	
	initialPlacementAtYaw.load_from_xml(_node, TXT("initialPlacementAtYaw"));
	if (_node->first_child_named(TXT("initialPlacementAtYaw")))
	{
		initialPlacementAtYaw = Range::zero;
		initialPlacementAtYaw.load_from_xml_child_node(_node, TXT("initialPlacementAtYaw"));
	}	
	initialOrientationMaxSpeed = _node->get_float_attribute_or_from_child(TXT("initialOrientationMaxSpeed"), initialOrientationMaxSpeed);

	return result;
}

AppearanceControllerData* LocalGravityData::create_copy() const
{
	LocalGravityData* copy = new LocalGravityData();
	*copy = *this;
	return copy;
}

AppearanceController* LocalGravityData::create_controller()
{
	return new LocalGravity(this);
}
