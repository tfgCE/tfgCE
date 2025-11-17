#include "acp_freeRange.h"

#include "..\..\appearanceControllerPoseContext.h"
#include "..\..\appearanceControllers.h"

#include "..\..\meshParticles.h"

#include "..\..\..\module\moduleAppearance.h"

#include "..\..\..\..\core\mesh\pose.h"
#include "..\..\..\..\core\vr\iVR.h"

#include "..\..\..\..\core\performance\performanceUtils.h"

using namespace Framework;
using namespace AppearanceControllersLib;
using namespace Particles;

//

DEFINE_STATIC_NAME(particlesFreeRange);

//

REGISTER_FOR_FAST_CAST(FreeRange);

FreeRange::FreeRange(FreeRangeData const * _data)
: base(_data)
, freeRangeData(_data)
{
	fovLimit = freeRangeData->fovLimit;
	if (freeRangeData->fovLimitFromVR)
	{
		if (VR::IVR::can_be_used())
		{
			auto & fovEyes = VR::IVR::get()->get_render_info().fov;
			float fov = max(fovEyes[0], fovEyes[1]);
			fovLimit = fov;
		}
#ifdef AN_DEVELOPMENT
		else
		{
			// just test
			fovLimit = 90.0f;
		}
#endif
	}
}

void FreeRange::activate()
{
	base::activate();

	Transform ownerPlacement = get_owner_placement_in_proper_space();

	for_every(particle, particles)
	{
		setup_particle(particle, ownerPlacement);
	}
}

float const fovLimitAngleMargin = 2.0f;
float const fovLimitDistanceMargin = 0.5f; // to allow us going backwards to have anything there

void FreeRange::setup_particle(Particle* _particle, Transform const & _ownerPlacement, bool _atMaxDistance)
{
	_particle->placement = _ownerPlacement;
	_particle->placement.set_orientation(Rotator3(rg.get_float(-90.0f, 90.0f), rg.get_float(-180.0f, 180.0f), rg.get_float(-180.0f, 180.0f)).to_quat());
	Vector3 offset = Vector3(rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f));
	if (fovLimit > 0.0f)
	{
		// move into range
		float fovLimitXZtoY = fovLimit > 0.0f ? 1.0f / tan_deg(min(179.0f, fovLimit) / 2.0f) : 0.0f;
		while (abs(offset.x * fovLimitXZtoY) > offset.y + 0.2f || abs(offset.y * fovLimitXZtoY) > offset.y + 0.2f)
		{
			offset = Vector3(rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f));
			float scale = 1.0f * 0.2f + 0.8f * abs(offset.y);
			offset.x *= scale;
			offset.z *= scale;
		}
	}
	if (_atMaxDistance)
	{
		offset.normalise();
	}
	_particle->placement.set_translation(_ownerPlacement.location_to_world(offset * freeRangeData->maxDistance));
	_particle->velocityLinear = _ownerPlacement.vector_to_world(Vector3(rg.get_float(freeRangeData->initialVelocityVector.x.min, freeRangeData->initialVelocityVector.x.max),
																		rg.get_float(freeRangeData->initialVelocityVector.y.min, freeRangeData->initialVelocityVector.y.max),
																		rg.get_float(freeRangeData->initialVelocityVector.z.min, freeRangeData->initialVelocityVector.z.max))).normal()
							  * rg.get_float(freeRangeData->speed.min, freeRangeData->speed.max);
	_particle->velocityRotation = Rotator3(rg.get_float(freeRangeData->initialVelocityOrientation.x.min, freeRangeData->initialVelocityOrientation.x.max),
										   rg.get_float(freeRangeData->initialVelocityOrientation.z.min, freeRangeData->initialVelocityOrientation.z.max),
										   rg.get_float(freeRangeData->initialVelocityOrientation.y.min, freeRangeData->initialVelocityOrientation.y.max));
	_particle->timeBeingAlive = 0.0f;
	_particle->activationFlags = 0; // this is sort of an activation
}

void FreeRange::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	IModulesOwner* imo = get_owner()->get_owner();

	MEASURE_PERFORMANCE(particles_freeRange_preliminaryPose);

	Transform ownerPlacement = get_owner_placement_in_proper_space();

	Vector3 const gravityOS = imo->get_presence()->get_placement().vector_to_local(imo->get_presence()->get_gravity());
	Vector3 const gravityComponent = ownerPlacement.vector_to_world(gravityOS * freeRangeData->useGravity);

	Vector3 location = ownerPlacement.get_translation();

	float fovLimitXZtoY = fovLimit > 0.0f ? 1.0f / tan_deg(min(179.0f, fovLimit + fovLimitAngleMargin) / 2.0f) : 0.0f;
	for_every(particle, particles)
	{
		float distance = (particle->placement.get_translation() - location).length();
		if (distance > freeRangeData->maxDistance * 1.2f)
		{
			setup_particle(particle, ownerPlacement, false);
		}
		if (fovLimitXZtoY > 0.0f)
		{
			Vector3 localPlacement = ownerPlacement.location_to_local(particle->placement.get_translation());
			Vector3 inZSpace = localPlacement;
			inZSpace.x = abs(inZSpace.x * fovLimitXZtoY);
			inZSpace.z = abs(inZSpace.z * fovLimitXZtoY);
			inZSpace.y += fovLimitDistanceMargin; // margin
			if (inZSpace.x > inZSpace.y || inZSpace.z > inZSpace.y)
			{
				if (localPlacement.y > 0.0f || rg.get_chance(0.1f))
				{
					if (inZSpace.x > inZSpace.y)
					{
						localPlacement.x = -localPlacement.x;
					}
					if (inZSpace.z > inZSpace.y)
					{
						localPlacement.z = -localPlacement.z;
					}
					// move closer to fit into range
					localPlacement.x *= 0.9f;
					localPlacement.z *= 0.9f;
					localPlacement.y += fovLimitDistanceMargin * 0.75f;
					particle->placement.set_translation(ownerPlacement.location_to_world(localPlacement));
				}
				else
				{
					setup_particle(particle, ownerPlacement, false);
				}
			}
		}
		particle->velocityLinear -= gravityComponent;
		particle->velocityLinear = particle->velocityLinear.length() * particle->placement.get_orientation().get_y_axis();
		particle->velocityLinear = particle->velocityLinear.normal() * clamp(particle->velocityLinear.length(), freeRangeData->speed.min, freeRangeData->speed.max);
		particle->velocityLinear += gravityComponent;
		float fullScale = clamp(freeRangeData->maxDistance - distance, 0.0f, 1.0f) * clamp(particle->timeBeingAlive / 0.7f, 0.0f, 1.0f);
		fullScale = 1.0f - fullScale;
		fullScale = sqr(sqr(fullScale));
		fullScale = 1.0f - fullScale;
		particle->placement.set_scale(fullScale);
	}

	base::advance_and_adjust_preliminary_pose(_context);
}

//

REGISTER_FOR_FAST_CAST(FreeRangeData);

AppearanceControllerData* FreeRangeData::create_data()
{
	return new FreeRangeData();
}

void FreeRangeData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(particlesFreeRange), create_data);
}

FreeRangeData::FreeRangeData()
{
}

bool FreeRangeData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	useGravity = _node->get_float_attribute_or_from_child(TXT("useGravity"), useGravity);
	maxDistance = _node->get_float_attribute_or_from_child(TXT("maxDistance"), maxDistance);
	initialVelocityVector.load_from_xml_child_node(_node, TXT("initialVelocityVector"));
	speed.load_from_xml(_node, TXT("speed"));
	speed.load_from_xml_child_node(_node, TXT("speed"));
	initialVelocityOrientation.load_from_xml_child_node(_node, TXT("initialVelocityOrientation"));
	fovLimit = _node->get_float_attribute_or_from_child(TXT("fovLimit"), fovLimit);
	fovLimitFromVR = _node->get_bool_attribute_or_from_child_presence(TXT("fovLimitFromVR"), fovLimitFromVR);
	return result;
}

AppearanceControllerData* FreeRangeData::create_copy() const
{
	FreeRangeData* copy = new FreeRangeData();
	*copy = *this;
	return copy;
}

AppearanceController* FreeRangeData::create_controller()
{
	return new FreeRange(this);
}
