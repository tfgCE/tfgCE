#include "acp_decal.h"

#include "..\..\appearanceControllerPoseContext.h"
#include "..\..\appearanceControllers.h"

#include "..\..\meshParticles.h"

#include "..\..\..\module\moduleAppearance.h"

#include "..\..\..\..\core\mesh\pose.h"

#include "..\..\..\..\core\performance\performanceUtils.h"

using namespace Framework;
using namespace AppearanceControllersLib;
using namespace Particles;

//

DEFINE_STATIC_NAME(particlesDecal);

//

REGISTER_FOR_FAST_CAST(Decal);

Decal::Decal(DecalData const * _data)
: base(_data)
, decalData(_data)
{
}

void Decal::activate()
{
	base::activate();

	activeDecal = 0.0f;
	shouldDeactivate = false;
}

void Decal::desire_to_deactivate()
{
	shouldDeactivate = true;
}

void Decal::internal_activate_particle_in_ws(Particle * firstParticle, Particle* endParticle)
{
	base::internal_activate_particle_in_ws(firstParticle, endParticle);

	IModulesOwner* imo = get_owner()->get_owner();
	Transform ownerPlacement = imo->get_presence()->get_placement(); // always activate in WS

	for (Particle* particle = firstParticle; particle != endParticle; ++particle)
	{
		// just randomise orientation, placement should be kept as it was set in base class
		particle->placement = ownerPlacement;
		particle->velocityLinear = Vector3::zero;
		Vector3 offset;
		offset.x = rg.get(decalData->placementOffset.x);
		offset.y = rg.get(decalData->placementOffset.y);
		offset.z = rg.get(decalData->placementOffset.z);
		particle->placement.set_translation(particle->placement.location_to_world(offset));
		particle->placement.set_orientation(Rotator3(rg.get_float(-180.0f, 180.0f), rg.get_float(-180.0f, 180.0f), rg.get_float(-180.0f, 180.0f)).to_quat());
		particle->velocityLinear = Vector3::zero;
		particle->velocityRotation = Rotator3::zero;
	}
}

void Decal::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	MEASURE_PERFORMANCE(particles_Decal_preliminaryPose);

	float deltaTime = _context.get_delta_time();

	if (shouldDeactivate)
	{
		activeDecal = blend_to_using_speed_based_on_time(activeDecal, 0.0f, decalData->decayTime, deltaTime);
	}
	else
	{
		activeDecal = blend_to_using_speed_based_on_time(activeDecal, 1.0f, decalData->attackTime, deltaTime);
	}

	bool anyActive = is_continuous_now();

	for_every(particle, particles)
	{
		if (activeDecal > 0.0f)
		{
			particle->alpha = decalData->modifyAlpha? activeDecal : 1.0f;
			particle->dissolve = 0.0f;
			float fullScale = decalData->modifyScale? activeDecal : 1.0f;
			particle->placement.set_scale(fullScale);
			anyActive |= (fullScale > 0.0f && particle->alpha > 0.0f) || // visible
											  particle->timeBeingAlive == 0.0f; // just started
		}
		else
		{
			particle->timeBeingAlive = -1.0f; // stay there
			particle->activationFlags = ActivationFlags::MayActivate;
			particle->placement.set_scale(0.0f);
		}
	}

	base::advance_and_adjust_preliminary_pose(_context);

	if (! anyActive)
	{
		particles_deactivate();
	}
}

//

REGISTER_FOR_FAST_CAST(DecalData);

AppearanceControllerData* DecalData::create_data()
{
	return new DecalData();
}

void DecalData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(particlesDecal), create_data);
}

DecalData::DecalData()
{
	inLocalSpace = true; // by default we are in local space!
	initialPlacement = ParticlesPlacement::OwnerPlacement;
}

bool DecalData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	placementOffset.load_from_xml_child_node(_node, TXT("placementOffset"));
	attackTime = _node->get_float_attribute_or_from_child(TXT("attackTime"), attackTime);
	decayTime = _node->get_float_attribute_or_from_child(TXT("decayTime"), decayTime);
	modifyAlpha = _node->get_bool_attribute_or_from_child_presence(TXT("modifyAlpha"), modifyAlpha);
	modifyScale = _node->get_bool_attribute_or_from_child_presence(TXT("modifyScale"), modifyScale);

	error_loading_xml_on_assert(modifyAlpha || modifyScale, result, _node, TXT(" set modifyAlpha and/or modifyScale"));
	
	return result;
}

AppearanceControllerData* DecalData::create_copy() const
{
	DecalData* copy = new DecalData();
	*copy = *this;
	return copy;
}

AppearanceController* DecalData::create_controller()
{
	return new Decal(this);
}
