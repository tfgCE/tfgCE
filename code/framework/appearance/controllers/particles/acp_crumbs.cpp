#include "acp_crumbs.h"

#include "..\..\appearanceControllerPoseContext.h"
#include "..\..\appearanceControllers.h"

#include "..\..\meshParticles.h"

#include "..\..\..\module\moduleAppearance.h"

#include "..\..\..\..\core\debug\debugRenderer.h"

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

DEFINE_STATIC_NAME(particlesCrumbs);

//

REGISTER_FOR_FAST_CAST(Crumbs);

Crumbs::Crumbs(CrumbsData const * _data)
: base(_data)
, crumbsData(_data)
{
	useRoomVelocity = 1.0f;
	useRoomVelocityInTime = 0.0f;
	useOwnerMovement = 0.0f;
	useOwnerMovementForTime = 0.0f;
}

void Crumbs::activate()
{
	base::activate();

	quickerFallBack = QuickerFallBack();

	for_every(particle, particles)
	{
		particle->placement = Transform::identity;
		particle->placement.set_scale(0.0f);
		particle->alpha = 0.0f;
		particle->dissolve = 1.0f;
		particle->timeBeingAlive = -1.0f;
		particle->activationFlags = ActivationFlags::MayActivate;
	}
}

void Crumbs::desire_to_deactivate()
{
	base::desire_to_deactivate();
	if (crumbsData->quickerFallBack.fallBackTime > 0.0f)
	{
		quickerFallBack.active = true;
	}
}

void Crumbs::internal_activate_particle_in_ws(Particle * firstParticle, Particle* endParticle)
{
	IModulesOwner* imo = get_owner()->get_owner();
	Transform ownerPlacement = imo->get_presence()->get_placement(); // always activate in WS
	Transform particlePlacement = ownerPlacement;
	if (crumbsData->alignCrumbsToMovement)
	{
		if (lastLocWS.is_set())
		{
			Vector3 forward = (particlePlacement.get_translation() - lastLocWS.get()).normal();
			Vector3 up = abs(forward.z) < 0.7f ? Vector3::zAxis : Vector3::yAxis;
			particlePlacement = look_matrix(particlePlacement.get_translation(), forward, up).to_transform();
		}
		else
		{
			return; // do not activate
		}
	}

	base::internal_activate_particle_in_ws(firstParticle, endParticle);

	//debug_context(imo->get_presence()->get_in_room());

	for (Particle* particle = firstParticle; particle != endParticle; ++particle)
	{
		// just randomise orientation, placement should be kept as it was set in base class
		particle->placement = particlePlacement;
		particle->velocityLinear = Vector3::zero;
		if (!crumbsData->initialOrientationOff.is_zero())
		{
			particle->placement = particle->placement.to_world(Transform(Vector3::zero,
				Rotator3(Random::get_float(-1.0f, 1.0f) * crumbsData->initialOrientationOff.pitch,
					Random::get_float(-1.0f, 1.0f) * crumbsData->initialOrientationOff.yaw,
					Random::get_float(-1.0f, 1.0f) * crumbsData->initialOrientationOff.roll).to_quat()));
		}

		particle->velocityRotation = Rotator3(Random::get_float(-1.0f, 1.0f) * crumbsData->initialOrientationVelocity.pitch,
			Random::get_float(-1.0f, 1.0f) * crumbsData->initialOrientationVelocity.yaw,
			Random::get_float(-1.0f, 1.0f) * crumbsData->initialOrientationVelocity.roll);

		//debug_draw_time_based(10.0f, debug_draw_transform(true, particle->placement));
	}

	//debug_no_context();
}

void Crumbs::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	MEASURE_PERFORMANCE(particles_Crumbs_preliminaryPose);

	float deltaTime = _context.get_delta_time();

	if (quickerFallBack.active)
	{
		auto const& qfbData = crumbsData->quickerFallBack;
		quickerFallBack.timeBeingAlive += deltaTime;
		if (qfbData.catchUpTimeLimit <= 0.0f || quickerFallBack.catchUpTime < qfbData.catchUpTimeLimit)
		{
			quickerFallBack.activeTime += deltaTime * qfbData.catchUpSpeed;
			quickerFallBack.catchUpTime += deltaTime * qfbData.catchUpSpeed;
			quickerFallBack.catchUpTime = min(quickerFallBack.catchUpTime, qfbData.catchUpTimeLimit);
		}
		else
		{
			if (qfbData.catchUpSpeedBeyondLimit > 0.0f)
			{
				quickerFallBack.activeTime += deltaTime * qfbData.catchUpSpeedBeyondLimit;
				quickerFallBack.catchUpTime += deltaTime * qfbData.catchUpSpeedBeyondLimit;
			}
			else
			{
				quickerFallBack.activeTime += deltaTime;
			}
		}
	}

	IModulesOwner* imo = get_owner()->get_owner();

	Transform placementWS = imo->get_presence()->get_placement();

	bool anyActive = false;

	if (is_continuous_now())
	{
		anyActive = true;
		timeNotContinuous = 0.0f;
	}
	else
	{
		timeNotContinuous += deltaTime;
	}

	bool needsToActivate = false;
	{
		bool canActivateNew = (is_continuous_now() || (crumbsData->keepActivatingParticlesTime && timeNotContinuous < crumbsData->keepActivatingParticlesTime));
		if (crumbsData->crumbDistance.is_set())
		{
			if (lastLocWS.is_set())
			{
				distanceToCrumb -= (placementWS.get_translation() - lastLocWS.get()).length();
			}
			if (distanceToCrumb <= 0.0f && canActivateNew)
			{
				needsToActivate = true;
			}
		}
		if (crumbsData->crumbPeriod.is_set())
		{
			timeToCrumb -= deltaTime;
			if (timeToCrumb <= 0.0f && canActivateNew)
			{
				needsToActivate = true;
			}
		}
		if (needsToActivate)
		{
			if (crumbsData->crumbDistance.is_set())
			{
				distanceToCrumb = Random::get(crumbsData->crumbDistance.get());
			}
			if (crumbsData->crumbPeriod.is_set())
			{
				timeToCrumb = Random::get(crumbsData->crumbPeriod.get());
			}
		}
	}

#ifdef AN_DEVELOPMENT
	bool activatedParticle = false;
#endif

	for_every(particle, particles)
	{
		float timeLeft = crumbsData->timeAlive - particle->timeBeingAlive;
		float adjustTime = clamp(particle->randomNumber * crumbsData->timeAlive * crumbsData->timeAliveVarietyCoef, 0.0f, 0.999f);
		float adjustTimeAlive = adjustTime * crumbsData->timeAlive;

		if (quickerFallBack.active)
		{
			auto const& qfb = quickerFallBack;
			auto const& qfbData = crumbsData->quickerFallBack;
			float relTime = particle->timeBeingAlive - qfb.timeBeingAlive;
			if (qfb.catchUpTime > relTime + qfbData.catchUpTime)
			{
				float activeTime = qfb.activeTime - qfbData.catchUpTime - relTime;

				float wholeTimeAlive = crumbsData->timeAlive - adjustTimeAlive;
				wholeTimeAlive -= (activeTime) * (wholeTimeAlive / qfbData.fallBackTime);
				wholeTimeAlive = clamp(wholeTimeAlive, 0.0f, crumbsData->timeAlive);
				adjustTimeAlive = crumbsData->timeAlive - wholeTimeAlive;
			}
		}

		if (particle->timeBeingAlive >= 0.0f &&
			crumbsData->keepActivatingParticlesTime > 0.0f)
		{
			adjustTimeAlive = lerp(clamp(timeNotContinuous / (crumbsData->keepActivatingParticlesTime * 0.5f), 0.0f, 1.0f), adjustTimeAlive, timeLeft);
		}

		float actualTimeLeft = timeLeft - adjustTimeAlive;
		if (needsToActivate)
		{
			if ((actualTimeLeft <= 0.0f && is_continuous_now()) ||
				(particle->timeBeingAlive < 0.0f || is_flag_set(particle->activationFlags, ActivationFlags::MayActivate)))
			{
				activate_particle(particle);
				needsToActivate = false;
#ifdef AN_DEVELOPMENT
				activatedParticle = true;
#endif
			}
		}

		float timeAliveForThisParticle = crumbsData->timeAlive - adjustTimeAlive;
		if (particle->timeBeingAlive >= 0.0f && particle->timeBeingAlive < timeAliveForThisParticle)
		{
			particle->alpha = crumbsData->calculate_alpha(particle->timeBeingAlive, timeAliveForThisParticle);
			particle->dissolve = crumbsData->calculate_dissolve(particle->timeBeingAlive, timeAliveForThisParticle);
			float actualScale = crumbsData->calculate_scale(particle->timeBeingAlive, timeAliveForThisParticle);
			float fullScale = crumbsData->calculate_full_scale(particle->timeBeingAlive, timeAliveForThisParticle);
			particle->placement.set_scale(actualScale);
			anyActive |= (fullScale > 0.0f && particle->alpha > 0.0f) || // visible
						 particle->timeBeingAlive == 0.0f; // just started
		}
		else
		{
			// to reuse!
			particle->timeBeingAlive = -1.0f; // stay there
			particle->activationFlags = ActivationFlags::MayActivate;
			particle->placement.set_scale(0.0f);
		}
	}

#ifdef AN_DEVELOPMENT
	if (needsToActivate && !activatedParticle)
	{
		warn_dev_ignore(TXT("not enough particles"));
	}
#endif

	base::advance_and_adjust_preliminary_pose(_context);

	if (! anyActive)
	{
		particles_deactivate();
	}

	lastLocWS = placementWS.get_translation();
}

//

REGISTER_FOR_FAST_CAST(CrumbsData);

AppearanceControllerData* CrumbsData::create_data()
{
	return new CrumbsData();
}

void CrumbsData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(particlesCrumbs), create_data);
}

CrumbsData::CrumbsData()
{
	continuous = true;
	initialPlacement = ParticlesPlacement::OwnerPlacement;
}

bool CrumbsData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("quickerFallBack")))
	{
		quickerFallBack.catchUpTime = node->get_float_attribute_or_from_child(TXT("catchUpTime"), quickerFallBack.catchUpTime);
		quickerFallBack.catchUpSpeed = node->get_float_attribute_or_from_child(TXT("catchUpSpeed"), quickerFallBack.catchUpSpeed);
		quickerFallBack.catchUpSpeedBeyondLimit = node->get_float_attribute_or_from_child(TXT("catchUpSpeedBeyondLimit"), quickerFallBack.catchUpSpeedBeyondLimit);
		quickerFallBack.catchUpTimeLimit = node->get_float_attribute_or_from_child(TXT("catchUpTimeLimit"), quickerFallBack.catchUpTimeLimit);
		quickerFallBack.fallBackTime = node->get_float_attribute_or_from_child(TXT("fallBackTime"), quickerFallBack.fallBackTime);
	}
	timeAlive = _node->get_float_attribute_or_from_child(TXT("timeAlive"), timeAlive);
	timeAliveVarietyCoef = _node->get_float_attribute_or_from_child(TXT("timeAliveVarietyCoef"), timeAliveVarietyCoef);
	keepActivatingParticlesTime = _node->get_float_attribute_or_from_child(TXT("keepActivatingParticlesTime"), keepActivatingParticlesTime);
	crumbPeriod.load_from_xml(_node, TXT("crumbPeriod"));
	crumbDistance.load_from_xml(_node, TXT("crumbDistance"));
	if (_node->first_child_named(TXT("initialOrientationVelocity")))
	{
		initialOrientationVelocity = Rotator3::zero;
		initialOrientationVelocity.load_from_xml_child_node(_node, TXT("initialOrientationVelocity"));
	}
	if (_node->first_child_named(TXT("initialOrientationOff")))
	{
		initialOrientationOff = Rotator3::zero;
		initialOrientationOff.load_from_xml_child_node(_node, TXT("initialOrientationOff"));
	}
	alignCrumbsToMovement = _node->get_bool_attribute_or_from_child_presence(TXT("alignCrumbsToMovement"), alignCrumbsToMovement);

	return result;
}

AppearanceControllerData* CrumbsData::create_copy() const
{
	CrumbsData* copy = new CrumbsData();
	*copy = *this;
	return copy;
}

AppearanceController* CrumbsData::create_controller()
{
	return new Crumbs(this);
}
