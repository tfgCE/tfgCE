#include "acpm_scaleEach.h"

#include "..\..\appearanceControllerPoseContext.h"
#include "..\..\appearanceControllers.h"

#include "..\..\meshParticles.h"

#include "..\..\..\module\moduleAppearance.h"

#include "..\..\..\..\core\mesh\pose.h"

#include "..\..\..\..\core\performance\performanceUtils.h"

using namespace Framework;
using namespace AppearanceControllersLib;
using namespace ParticleModifiers;

//

DEFINE_STATIC_NAME(particleModifierScaleEach);

//

REGISTER_FOR_FAST_CAST(ScaleEach);

ScaleEach::ScaleEach(ScaleEachData const * _data)
: base(_data)
, scaleEachData(_data)
{
}

void ScaleEach::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	MEASURE_PERFORMANCE(particles_ScaleEach_preliminaryPose);

	base::calculate_final_pose(_context);

	Meshes::Pose* poseLS = _context.access_pose_LS();

	float scaleEach = scaleEachData->scale;
	for_every(t, poseLS->access_bones())
	{
		t->set_scale(t->get_scale() * scaleEach);
	}
}

//

REGISTER_FOR_FAST_CAST(ScaleEachData);

AppearanceControllerData* ScaleEachData::create_data()
{
	return new ScaleEachData();
}

void ScaleEachData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(particleModifierScaleEach), create_data);
}

ScaleEachData::ScaleEachData()
{
}

bool ScaleEachData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	scale = _node->get_float_attribute_or_from_child(TXT("scale"), scale);
	
	return result;
}

AppearanceControllerData* ScaleEachData::create_copy() const
{
	ScaleEachData* copy = new ScaleEachData();
	*copy = *this;
	return copy;
}

AppearanceController* ScaleEachData::create_controller()
{
	return new ScaleEach(this);
}
