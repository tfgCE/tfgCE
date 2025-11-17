#include "acp_scale.h"

#include "..\..\appearanceControllerPoseContext.h"
#include "..\..\appearanceControllers.h"

#include "..\..\meshParticles.h"

#include "..\..\..\module\moduleAppearance.h"

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

DEFINE_STATIC_NAME(particlesScale);

//

REGISTER_FOR_FAST_CAST(Scale);

Scale::Scale(ScaleData const * _data)
: base(_data)
, scaleData(_data)
{
}

void Scale::activate()
{
	base::activate();

	scale = 0.0f;
	shouldDeactivate = false;
}

void Scale::desire_to_deactivate()
{
	shouldDeactivate = true;
}

void Scale::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	MEASURE_PERFORMANCE(particles_Scale_preliminaryPose);

	float deltaTime = _context.get_delta_time();

	IModulesOwner* imo = get_owner()->get_owner();

	if (shouldDeactivate)
	{
		scale = blend_to_using_speed_based_on_time(scale, 0.0f, scaleData->scaleDownTime, deltaTime);
	}
	else
	{
		scale = blend_to_using_speed_based_on_time(scale, 1.0f, scaleData->scaleUpTime, deltaTime);
	}

	imo->get_presence()->set_scale(max(0.001f, scale * scaleData->scale));

	base::advance_and_adjust_preliminary_pose(_context);

	if (scale <= 0.0f && shouldDeactivate)
	{
		particles_deactivate();
	}
}

//

REGISTER_FOR_FAST_CAST(ScaleData);

AppearanceControllerData* ScaleData::create_data()
{
	return new ScaleData();
}

void ScaleData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(particlesScale), create_data);
}

ScaleData::ScaleData()
{
}

bool ScaleData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	scale = _node->get_float_attribute_or_from_child(TXT("scale"), scale);
	scaleUpTime = _node->get_float_attribute_or_from_child(TXT("scaleUpTime"), scaleUpTime);
	scaleDownTime = _node->get_float_attribute_or_from_child(TXT("scaleDownTime"), scaleDownTime);
	
	return result;
}

AppearanceControllerData* ScaleData::create_copy() const
{
	ScaleData* copy = new ScaleData();
	*copy = *this;
	return copy;
}

AppearanceController* ScaleData::create_controller()
{
	return new Scale(this);
}
