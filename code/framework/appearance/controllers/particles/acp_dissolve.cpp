#include "acp_dissolve.h"

#include "..\..\appearanceControllerPoseContext.h"
#include "..\..\appearanceControllers.h"

#include "..\..\meshParticles.h"

#include "..\..\..\module\moduleAppearance.h"
#include "..\..\..\module\moduleAppearanceImpl.inl"

#include "..\..\..\..\core\mesh\pose.h"

#include "..\..\..\..\core\performance\performanceUtils.h"

using namespace Framework;
using namespace AppearanceControllersLib;
using namespace Particles;

//

DEFINE_STATIC_NAME(particlesDissolve);

// shader
DEFINE_STATIC_NAME(dissolve);

//

REGISTER_FOR_FAST_CAST(Dissolve);

Dissolve::Dissolve(DissolveData const * _data)
: base(_data)
, dissolveData(_data)
{
}

void Dissolve::activate()
{
	base::activate();

	visible = 0.0f;
	shouldDeactivate = false;
}

void Dissolve::desire_to_deactivate()
{
	shouldDeactivate = true;
}

void Dissolve::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	MEASURE_PERFORMANCE(particles_Dissolve_preliminaryPose);

	float deltaTime = _context.get_delta_time();

	IModulesOwner* imo = get_owner()->get_owner();

	if (shouldDeactivate)
	{
		visible = blend_to_using_speed_based_on_time(visible, 0.0f, dissolveData->dissolveTime, deltaTime);
	}
	else
	{
		visible = blend_to_using_speed_based_on_time(visible, 1.0f, dissolveData->appearTime, deltaTime);
	}

	an_assert(imo->get_appearances().get_size() == 1);
	imo->get_appearance()->set_shader_param(NAME(dissolve), 1.0f - visible);

	base::advance_and_adjust_preliminary_pose(_context);

	if (visible <= 0.0f && shouldDeactivate)
	{
		particles_deactivate();
	}
}

//

REGISTER_FOR_FAST_CAST(DissolveData);

AppearanceControllerData* DissolveData::create_data()
{
	return new DissolveData();
}

void DissolveData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(particlesDissolve), create_data);
}

DissolveData::DissolveData()
{
}

bool DissolveData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	appearTime = _node->get_float_attribute_or_from_child(TXT("appearTime"), appearTime);
	dissolveTime = _node->get_float_attribute_or_from_child(TXT("dissolveTime"), dissolveTime);
	
	return result;
}

AppearanceControllerData* DissolveData::create_copy() const
{
	DissolveData* copy = new DissolveData();
	*copy = *this;
	return copy;
}

AppearanceController* DissolveData::create_controller()
{
	return new Dissolve(this);
}
