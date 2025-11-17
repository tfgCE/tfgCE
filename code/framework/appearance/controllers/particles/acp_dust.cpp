#include "acp_dust.h"

#include "..\..\appearanceControllerPoseContext.h"
#include "..\..\appearanceControllers.h"

#include "..\..\meshParticles.h"

#include "..\..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\..\meshGen\meshGenParamImpl.inl"
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

DEFINE_STATIC_NAME(particlesDust);

//

REGISTER_FOR_FAST_CAST(Dust);

Dust::Dust(DustData const * _data)
: base(_data)
, dustData(_data)
{
	useRoomVelocity = 0.0f;
}

void Dust::activate()
{
	initialDistance = dustData->initialDistance.is_set() ? dustData->initialDistance.get() : Range::empty;
	initialPlacementBox = dustData->initialPlacementBox.is_set() ? dustData->initialPlacementBox.get() : Range3::empty;

	if (dustData->initialPlacementBoxVar.is_valid())
	{
		initialPlacementBox = get_owner()->get_owner()->get_variables().get_value<Range3>(dustData->initialPlacementBoxVar, initialPlacementBox);
	}

	base::activate();

	if (is_continuous_now())
	{
		for_every(particle, particles)
		{
			// negative time means it will be delayed
			particle->timeBeingAlive = min(0.0f, rg.get_float(-dustData->timeAlive * 0.9f, dustData->timeAlive * 0.1f));
			particle->activationFlags = ActivationFlags::MayActivate;
		}
	}
}

void Dust::internal_activate_particle_in_ws(Particle * firstParticle, Particle* endParticle)
{
	base::internal_activate_particle_in_ws(firstParticle, endParticle);

	IModulesOwner* imo = get_owner()->get_owner();
	Transform ownerPlacement = imo->get_presence()->get_placement(); // always activate in WS

	for (Particle* particle = firstParticle; particle != endParticle; ++particle)
	{
		// just randomise orientation, placement should be kept as it was set in base class
		Vector3 loc = Vector3::zero;
		if (!initialDistance.is_empty())
		{
			Vector3 off;
			off.x = rg.get_float(-1.0f, 1.0f);
			off.y = rg.get_float(-1.0f, 1.0f);
			off.z = rg.get_float(-1.0f, 1.0f);
			loc += off.normal() * rg.get(initialDistance);
		}
		if (!initialPlacementBox.is_empty())
		{
			loc.x += rg.get(initialPlacementBox.x);
			loc.y += rg.get(initialPlacementBox.y);
			loc.z += rg.get(initialPlacementBox.z);
		}

		Vector3 locVel = Vector3(rg.get(dustData->initialVelocityVector.x),
								 rg.get(dustData->initialVelocityVector.y),
								 rg.get(dustData->initialVelocityVector.z));

		locVel = locVel.normal() * rg.get(dustData->speed);
		particle->placement.set_translation(ownerPlacement.location_to_world(loc));
		particle->placement.set_orientation(Rotator3(rg.get_float(-90.0f, 90.0f), rg.get_float(-180.0f, 180.0f), rg.get_float(-180.0f, 180.0f)).to_quat());
		particle->velocityLinear = ownerPlacement.vector_to_world(locVel);

		float a = dustData->initialOrientationMaxSpeed;
		particle->velocityRotation = Rotator3(rg.get_float(-a, a), rg.get_float(-a, a), rg.get_float(-a, a));

		particle->customData[CustomData::TimeToChangeVelocity] = rg.get(dustData->changeVelocityInterval);
		particle->customData[CustomData::VelocityRequestedX] = locVel.x;
		particle->customData[CustomData::VelocityRequestedY] = locVel.y;
		particle->customData[CustomData::VelocityRequestedZ] = locVel.z;
	}
}

void Dust::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	MEASURE_PERFORMANCE(particles_Dust_preliminaryPose);

	float deltaTime = _context.get_delta_time();

	Transform ownerPlacement = get_owner_placement_in_proper_space();

	bool anyActive = is_continuous_now();

	for_every(particle, particles)
	{
		float timeLeft = dustData->timeAlive - particle->timeBeingAlive;
		float adjustTime = clamp(particle->randomNumber * dustData->timeAlive * dustData->timeAliveVarietyCoef, 0.0f, 0.999f);
		float adjustTimeAlive = adjustTime * dustData->timeAlive;

		if (!is_continuous_now() && particle->timeBeingAlive >= dustData->timeAlive)
		{
			// shouldn't be activated any more
			particle->activationFlags = 0; // shouldn't be activated
		}

		float actualTimeLeft = timeLeft - adjustTimeAlive;
		if ((is_flag_set(particle->activationFlags, ActivationFlags::MayActivate) && actualTimeLeft <= 0.0f && (is_continuous_now())) ||
			(is_flag_set(particle->activationFlags, ActivationFlags::MayActivate) && particle->timeBeingAlive + _context.get_delta_time() >= 0.0f
										  && particle->timeBeingAlive < dustData->timeAlive))
		{
			activate_particle(particle);
		}

		if (particle->timeBeingAlive >= 0.0f)
		{
			Vector3 loc = ownerPlacement.location_to_local(particle->placement.get_translation());
			Vector3 locVel = ownerPlacement.vector_to_local(particle->velocityLinear);

			particle->customData[CustomData::TimeToChangeVelocity] -= deltaTime;
			if (particle->customData[CustomData::TimeToChangeVelocity] < 0.0f)
			{
				particle->customData[CustomData::TimeToChangeVelocity] = rg.get(dustData->changeVelocityInterval);
				Vector3 newLocVel = Vector3(rg.get(dustData->initialVelocityVector.x),
											rg.get(dustData->initialVelocityVector.y),
											rg.get(dustData->initialVelocityVector.z));

				newLocVel = newLocVel.normal() * rg.get(dustData->speed);
				particle->customData[CustomData::VelocityRequestedX] = newLocVel.x;
				particle->customData[CustomData::VelocityRequestedY] = newLocVel.y;
				particle->customData[CustomData::VelocityRequestedZ] = newLocVel.z;
			}

			{
				Vector3 newLocVel;
				newLocVel.x = particle->customData[CustomData::VelocityRequestedX];
				newLocVel.y = particle->customData[CustomData::VelocityRequestedY];
				newLocVel.z = particle->customData[CustomData::VelocityRequestedZ];
				locVel = blend_to_using_time(locVel, newLocVel, dustData->changeVelocityBlendTime, deltaTime);
				loc += locVel * deltaTime;
			}

			particle->placement.set_translation(ownerPlacement.location_to_world(loc));
			particle->velocityLinear = ownerPlacement.vector_to_world(locVel);

			particle->alpha = dustData->calculate_alpha(particle->timeBeingAlive, dustData->timeAlive - adjustTimeAlive);
			particle->dissolve = dustData->calculate_dissolve(particle->timeBeingAlive, dustData->timeAlive - adjustTimeAlive);
			float actualScale = dustData->calculate_scale(particle->timeBeingAlive, dustData->timeAlive - adjustTimeAlive);
			float fullScale = dustData->calculate_full_scale(particle->timeBeingAlive, dustData->timeAlive - adjustTimeAlive);
			particle->placement.set_scale(actualScale);
			anyActive |= (fullScale > 0.0f && particle->alpha > 0.0f) || // visible
						 particle->timeBeingAlive == 0.0f; // just started

			if (particle->timeBeingAlive >= dustData->timeAlive - adjustTimeAlive)
			{
				// will end life next frame
				particle->timeBeingAlive = dustData->timeAlive;
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

REGISTER_FOR_FAST_CAST(DustData);

AppearanceControllerData* DustData::create_data()
{
	return new DustData();
}

void DustData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(particlesDust), create_data);
}

DustData::DustData()
{
	initialPlacement = ParticlesPlacement::OwnerPlacement;
}

bool DustData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	XML_LOAD_FLOAT_ATTR(_node, timeAlive);
	XML_LOAD_FLOAT_ATTR(_node, timeAliveVarietyCoef);
	XML_LOAD(_node, speed);
	initialVelocityVector.load_from_xml_child_node(_node, TXT("initialVelocityVector"));
	XML_LOAD(_node, changeVelocityInterval);
	XML_LOAD_FLOAT_ATTR(_node, changeVelocityBlendTime);
	initialDistance.load_from_xml(_node, TXT("initialDistance"));
	initialPlacementBox.load_from_xml(_node, TXT("initialPlacementBox"));
	XML_LOAD_NAME_ATTR(_node, initialPlacementBoxVar);
	XML_LOAD_FLOAT_ATTR(_node, initialOrientationMaxSpeed);

	return result;
}

AppearanceControllerData* DustData::create_copy() const
{
	DustData* copy = new DustData();
	*copy = *this;
	return copy;
}

AppearanceController* DustData::create_controller()
{
	return new Dust(this);
}

void DustData::apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const& _context)
{
	base::apply_mesh_gen_params(_context);

	initialDistance.fill_value_with(_context, AllowToFail);
	initialPlacementBox.fill_value_with(_context, AllowToFail);
}
