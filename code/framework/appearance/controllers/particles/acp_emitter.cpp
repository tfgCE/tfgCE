#include "acp_emitter.h"

#include "..\..\appearanceControllerPoseContext.h"
#include "..\..\appearanceControllers.h"

#include "..\..\meshParticles.h"

#include "..\..\..\module\moduleAppearance.h"
#include "..\..\..\module\moduleSound.h"

#include "..\..\..\modulesOwner\modulesOwner.h"

#include "..\..\..\framework.h"

#include "..\..\..\..\core\mesh\pose.h"

#include "..\..\..\..\core\performance\performanceUtils.h"

#include "..\..\..\..\core\debug\debugRenderer.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;
using namespace Particles;

//

DEFINE_STATIC_NAME(particlesEmitter);

//

REGISTER_FOR_FAST_CAST(Emitter);

Emitter::Emitter(EmitterData const * _data)
: base(_data)
, emitterData(_data)
{
}

void Emitter::activate()
{
	base::activate();

	if (is_continuous_now())
	{
		for_every(particle, particles)
		{
			// negative time means it will be delayed
			particle->timeBeingAlive = min(0.0f, rg.get_float(-emitterData->timeAlive * 0.9f, emitterData->timeAlive * 0.1f) - rg.get_float(0.0f, 1.0f) * rg.get(emitterData->timeBetween)) + min(0.0f, -rg.get(emitterData->initialDelay));
			particle->activationFlags = ActivationFlags::RequiresActivation;
		}
	}
	else
	{
		for_every(particle, particles)
		{
			// negative time means it will be delayed
			particle->timeBeingAlive = min(0.0f, -rg.get(emitterData->initialDelay));
			particle->activationFlags = ActivationFlags::RequiresActivation;
		}
	}
	for_every(particle, particles)
	{
		// mark it is initial activation
		particle->activationFlags |= ActivationFlags::PendingInitialActivation;
	}
}

void Emitter::internal_activate_particle_in_ws(Particle * firstParticle, Particle* endParticle)
{
	base::internal_activate_particle_in_ws(firstParticle, endParticle);

	IModulesOwner* imo = get_owner()->get_owner();
	Transform ownerPlacement = imo->get_presence()->get_placement(); // always activate in WS

	for (Particle* particle = firstParticle; particle != endParticle; ++particle)
	{
		// just randomise orientation, placement should be kept as it was set in base class

		{
			Vector3 relLoc;
			relLoc.x = rg.get(emitterData->initialRelativeLocOffset.x);
			relLoc.y = rg.get(emitterData->initialRelativeLocOffset.y);
			relLoc.z = rg.get(emitterData->initialRelativeLocOffset.z);
			particle->placement.set_translation(particle->placement.location_to_world(relLoc));
		}

		{
			Transform randomiseRotationRelativeToPlacement = emitterData->randomiseRotationRelativeToParticlePlacement ? particle->placement : Transform::identity;
			Rotator3 offsetRotation = Rotator3(rg.get_float(-90.0f, 90.0f), rg.get_float(-180.0f, 180.0f), rg.get_float(-180.0f, 180.0f)) * emitterData->randomiseOrientation;
			offsetRotation.pitch += rg.get(emitterData->randomiseOrientationAngle.x);
			offsetRotation.yaw += rg.get(emitterData->randomiseOrientationAngle.z);
			offsetRotation.roll += rg.get(emitterData->randomiseOrientationAngle.y);
			particle->placement.set_orientation(randomiseRotationRelativeToPlacement.get_orientation().to_world(offsetRotation.to_quat()));
		}

		particle->placement.set_translation(particle->placement.get_translation()
								+ rg.get(emitterData->initialRadius) * Vector3(rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f)).normal());
		{
			Transform initialVelocityRelativeToPlacement = emitterData->initialVelocityRelativeToParticlePlacement? particle->placement : ownerPlacement;
			particle->velocityLinear = initialVelocityRelativeToPlacement.vector_to_world(Vector3(rg.get_float(emitterData->initialVelocityVector.x.min, emitterData->initialVelocityVector.x.max),
																								  rg.get_float(emitterData->initialVelocityVector.y.min, emitterData->initialVelocityVector.y.max),
																								  rg.get_float(emitterData->initialVelocityVector.z.min, emitterData->initialVelocityVector.z.max))).normal()
									 * rg.get_float(emitterData->initialSpeed.min, emitterData->initialSpeed.max);
		}
		float a = emitterData->initialOrientationMaxSpeed;
		particle->velocityRotation = Rotator3(rg.get_float(-a, a), rg.get_float(-a, a), rg.get_float(-a, a));

		if (emitterData->playSoundDetachedOnParticleActivate.is_valid())
		{
			if (auto* imo = get_owner()->get_owner())
			{
				if (auto* s = imo->get_sound())
				{
					s->play_sound_in_room(emitterData->playSoundDetachedOnParticleActivate, imo->get_presence()->get_in_room(), particle->placement);
				}
			}
		}
	}
}

void Emitter::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	IModulesOwner* imo = get_owner()->get_owner();

	MEASURE_PERFORMANCE(particles_emitter_preliminaryPose);

	float deltaTime = _context.get_delta_time();

	// we always want owner placement in world space for gravity
	Transform ownerPlacement = get_owner_placement_in_proper_space();
	
	Vector3 gravityOS = imo->get_presence()->get_placement().vector_to_local(imo->get_presence()->get_gravity());
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (Framework::is_preview_game() && gravityOS.is_almost_zero())
	{
		gravityOS.z = -9.81f;
	}
#endif
	Vector3 const addGravity = ownerPlacement.vector_to_world(gravityOS * deltaTime * emitterData->useGravity);

	bool anyActive = is_continuous_now();

	float const infiniteTime = 100.0f;

	for_every(particle, particles)
	{
		float timeLeft = emitterData->timeAlive - particle->timeBeingAlive;
		float adjustTime = clamp(particle->randomNumber * emitterData->timeAliveVarietyCoef, 0.0f, 0.999f);
		float adjustTimeAlive = adjustTime * emitterData->timeAlive;

		float actualTimeLeft = timeLeft - adjustTimeAlive;
		if (emitterData->timeAlive == 0.0f)
		{
			actualTimeLeft = 1.0f; // always something
			adjustTimeAlive = 0.0f;
			if (are_particles_active() && is_continuous_now())
			{
				particle->timeBeingAlive = min(particle->timeBeingAlive, infiniteTime);
			}
			else
			{
				particle->timeBeingAlive = max(particle->timeBeingAlive, infiniteTime);
			}
		}
		bool blockActivationThisFrame = false;
		if (actualTimeLeft < 0.0f && particle->activationFlags == 0)
		{
			// disable, ended
			particle->timeBeingAlive = -1.0f; // reset
			particle->activationFlags = ActivationFlags::MayActivate;
			blockActivationThisFrame = true;
		}
		if (is_continuous_now() && (particle->activationFlags & ActivationFlags::MayActivate))
		{
			// queue it to activate
			particle->timeBeingAlive = min(-_context.get_delta_time() * 2.0f /* give it a moment to breathe */, -rg.get(emitterData->timeBetween));
			particle->activationFlags = ActivationFlags::RequiresActivation;
			blockActivationThisFrame = true;
		}
		if (! is_continuous_now() && is_flag_set(particle->activationFlags, ActivationFlags::PendingInitialActivation))
		{
			// to allow even if not continuous
			anyActive = true;
		}
		if (! is_continuous_now() && is_flag_set(particle->activationFlags, ActivationFlags::ShouldActivate) && ! is_flag_set(particle->activationFlags, ActivationFlags::PendingInitialActivation))
		{
			// shouldn't activate
			particle->timeBeingAlive = -1.0f; // reset
			particle->activationFlags = ActivationFlags::MayActivate; // shouldn't be activated but should be available for activation
		}
		if (!blockActivationThisFrame &&
			((particle->activationFlags & ActivationFlags::RequiresActivation)
					&& particle->timeBeingAlive + _context.get_delta_time() >= 0.0f
					&& particle->timeBeingAlive < emitterData->timeAlive))
		{
			activate_particle(particle);
		}

		if (particle->timeBeingAlive >= 0.0f)
		{
			particle->velocityLinear += addGravity;
			if (emitterData->slowDownTime > 0.0f)
			{
				float useSlow = clamp(_context.get_delta_time() / emitterData->slowDownTime, 0.0f, 1.0f);
				particle->velocityLinear = useSlow * particle->velocityLinear.normal() * emitterData->slowSpeed + (1.0f - useSlow) * particle->velocityLinear;
			}
			float actualScale = 1.0f;
			float fullScale = 1.0f;
			if (emitterData->timeAlive == 0.0f)
			{
				particle->alpha = emitterData->calculate_alpha(particle->timeBeingAlive, infiniteTime + emitterData->changeScale.decayTime);
				particle->dissolve = emitterData->calculate_dissolve(particle->timeBeingAlive, infiniteTime + emitterData->changeScale.decayTime);
				actualScale = emitterData->calculate_scale(particle->timeBeingAlive, infiniteTime + emitterData->changeScale.decayTime);
				fullScale = emitterData->calculate_full_scale(particle->timeBeingAlive, infiniteTime + emitterData->changeScale.decayTime);
			}
			else
			{
				particle->alpha = emitterData->calculate_alpha(particle->timeBeingAlive, emitterData->timeAlive - adjustTimeAlive);
				particle->dissolve = emitterData->calculate_dissolve(particle->timeBeingAlive, emitterData->timeAlive - adjustTimeAlive);
				actualScale = emitterData->calculate_scale(particle->timeBeingAlive, emitterData->timeAlive - adjustTimeAlive);
				fullScale = emitterData->calculate_full_scale(particle->timeBeingAlive, emitterData->timeAlive - adjustTimeAlive);
			}
			particle->placement.set_scale(actualScale);
			anyActive |= (fullScale > 0.0f && particle->alpha > 0.0f) || // visible
						 particle->timeBeingAlive == 0.0f; // just started
		}
		else
		{
			particle->placement.set_scale(0.0f);
			if (is_continuous_now())
			{
				particle->activationFlags = ActivationFlags::RequiresActivation;
			}
			else if (!is_flag_set(particle->activationFlags, ActivationFlags::PendingInitialActivation))
			{
				particle->activationFlags = ActivationFlags::MayActivate;
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

REGISTER_FOR_FAST_CAST(EmitterData);

AppearanceControllerData* EmitterData::create_data()
{
	return new EmitterData();
}

void EmitterData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(particlesEmitter), create_data);
}

EmitterData::EmitterData()
{
	initialPlacement = ParticlesPlacement::OwnerPlacement;
}

bool EmitterData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	useGravity = _node->get_float_attribute_or_from_child(TXT("useGravity"), useGravity);
	timeAlive = _node->get_float_attribute_or_from_child(TXT("timeAlive"), timeAlive);
	timeAliveVarietyCoef = _node->get_float_attribute_or_from_child(TXT("timeAliveVarietyCoef"), timeAliveVarietyCoef);
	timeBetween.load_from_xml(_node, TXT("timeBetween"));
	initialDelay.load_from_xml(_node, TXT("initialDelay"));
	slowDownTime = _node->get_float_attribute_or_from_child(TXT("slowDownTime"), slowDownTime);
	initialRadius.load_from_xml(_node, TXT("initialRadius"));
	initialRadius.load_from_xml_child_node(_node, TXT("initialRadius"));
	if (_node->first_child_named(TXT("initialRelativeLocOffset")))
	{
		initialRelativeLocOffset = Range3::zero;
		initialRelativeLocOffset.load_from_xml_child_node(_node, TXT("initialRelativeLocOffset"));
	}
	if (_node->first_child_named(TXT("initialVelocityVector")))
	{
		initialVelocityVector = Range3::zero;
		initialVelocityVector.load_from_xml_child_node(_node, TXT("initialVelocityVector"));
	}
	initialVelocityRelativeToParticlePlacement = _node->get_bool_attribute_or_from_child_presence(TXT("initialVelocityRelativeToParticlePlacement"), initialVelocityRelativeToParticlePlacement);
	initialSpeed.load_from_xml(_node, TXT("initialSpeed"));
	initialSpeed.load_from_xml_child_node(_node, TXT("initialSpeed"));
	slowSpeed = _node->get_float_attribute_or_from_child(TXT("slowSpeed"), slowSpeed);
	initialOrientationMaxSpeed = _node->get_float_attribute_or_from_child(TXT("initialOrientationMaxSpeed"), initialOrientationMaxSpeed);
	if (_node->first_child_named(TXT("randomiseOrientation")))
	{
		randomiseOrientation = Rotator3::zero;
		randomiseOrientation.load_from_xml_child_node(_node, TXT("randomiseOrientation"));
	}
	if (_node->first_child_named(TXT("randomiseOrientationAngle")))
	{
		randomiseOrientationAngle = Range3::zero;
		randomiseOrientationAngle.load_from_xml_child_node(_node, TXT("randomiseOrientationAngle"));
	}
	randomiseRotationRelativeToParticlePlacement = _node->get_bool_attribute_or_from_child_presence(TXT("randomiseRotationRelativeToParticlePlacement"), randomiseRotationRelativeToParticlePlacement);
	
	playSoundDetachedOnParticleActivate = _node->get_name_attribute_or_from_child(TXT("playSoundDetachedOnParticleActivate"), playSoundDetachedOnParticleActivate);

	return result;
}

AppearanceControllerData* EmitterData::create_copy() const
{
	EmitterData* copy = new EmitterData();
	*copy = *this;
	return copy;
}

AppearanceController* EmitterData::create_controller()
{
	return new Emitter(this);
}
