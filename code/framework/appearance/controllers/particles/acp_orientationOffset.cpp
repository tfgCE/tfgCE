#include "acp_orientationOffset.h"

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

DEFINE_STATIC_NAME(particlesOrientationOffset);

//

REGISTER_FOR_FAST_CAST(OrientationOffset);

OrientationOffset::OrientationOffset(OrientationOffsetData const * _data)
: base(_data)
, orientationOffsetData(_data)
{
}

void OrientationOffset::activate()
{
	base::activate();

	orientationOffsets.clear();
	shouldDeactivate = false;
}

void OrientationOffset::desire_to_deactivate()
{
	shouldDeactivate = true;
}

void OrientationOffset::calculate_final_pose(REF_ AppearanceControllerPoseContext& _context)
{
	base::calculate_final_pose(_context);

	if (get_owner()->get_skeleton())
	{
		Meshes::Pose* poseLS = _context.access_pose_LS();

		if (orientationOffsets.get_size() < poseLS->get_num_bones())
		{
			orientationOffsets.make_space_for(poseLS->get_num_bones());
			while (orientationOffsets.get_size() < poseLS->get_num_bones())
			{
				Rotator3 ro;
				ro.pitch = rg.get(orientationOffsetData->pitch);
				ro.yaw = rg.get(orientationOffsetData->yaw);
				ro.roll = rg.get(orientationOffsetData->roll);
				orientationOffsets.push_back(ro.to_quat());
			}
		}

		auto ro = orientationOffsets.begin();
		for_every(boneLS, poseLS->access_bones())
		{
			boneLS->set_orientation(boneLS->get_orientation().to_world(*ro));
			++ro;
		}
	}
}


//

REGISTER_FOR_FAST_CAST(OrientationOffsetData);

AppearanceControllerData* OrientationOffsetData::create_data()
{
	return new OrientationOffsetData();
}

void OrientationOffsetData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(particlesOrientationOffset), create_data);
}

OrientationOffsetData::OrientationOffsetData()
{
}

bool OrientationOffsetData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	pitch.load_from_attribute_or_from_child(_node, TXT("pitch"));
	yaw.load_from_attribute_or_from_child(_node, TXT("yaw"));
	roll.load_from_attribute_or_from_child(_node, TXT("roll"));
	
	return result;
}

AppearanceControllerData* OrientationOffsetData::create_copy() const
{
	OrientationOffsetData* copy = new OrientationOffsetData();
	*copy = *this;
	return copy;
}

AppearanceController* OrientationOffsetData::create_controller()
{
	return new OrientationOffset(this);
}
