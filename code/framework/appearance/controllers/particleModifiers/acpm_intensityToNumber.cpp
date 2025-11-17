#include "acpm_intensityToNumber.h"

#include "..\..\appearanceControllerPoseContext.h"
#include "..\..\appearanceControllers.h"

#include "..\..\meshParticles.h"

#include "..\..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\..\meshGen\meshGenParamImpl.inl"
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
using namespace ParticleModifiers;

//

DEFINE_STATIC_NAME(particleModifierIntensityToNumber);

//

REGISTER_FOR_FAST_CAST(IntensityToNumber);

IntensityToNumber::IntensityToNumber(IntensityToNumberData const * _data)
: base(_data)
, intensityToNumberData(_data)
{
	intensityVar = intensityToNumberData->intensityVar.get();
	minIntensity = intensityToNumberData->minIntensity.get();
	if (intensityToNumberData->maxIntensity.is_set())
	{
		maxIntensity = intensityToNumberData->maxIntensity.get();
	}
	intensityBlendTime = intensityToNumberData->intensityBlendTime.get();
}

void IntensityToNumber::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto& varStorage = get_owner()->access_controllers_variable_storage();
	intensityVar.look_up<float>(varStorage);
}

void IntensityToNumber::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	MEASURE_PERFORMANCE(particles_IntensityToNumber_preliminaryPose);

	base::calculate_final_pose(_context);

	Meshes::Pose* poseLS = _context.access_pose_LS();

	float intensityTarget = intensityVar.get<float>();

	if (minIntensity > 0.0f && minIntensity < 1.0f)
	{
		intensityTarget = minIntensity + (intensityTarget - 0.0f) / (1.0f - minIntensity);
		intensityTarget = max(intensityTarget, minIntensity);
	}
	if (maxIntensity.is_set())
	{
		intensityTarget = min(intensityTarget, maxIntensity.get());
	}

	if (has_just_activated())
	{
		intensity = intensityTarget;
	}
	else
	{
		intensity = blend_to_using_time(intensity, intensityTarget, intensityBlendTime, _context.get_delta_time());
	}

	{
		float intensityPerBone = 1.0f / (float)poseLS->get_bones().get_size();
		float invIntensityPerBone = intensityPerBone == 0.0f? 0.0f : 1.0f / intensityPerBone;
		float intensityLeft = intensity;
		for_every(t, poseLS->access_bones())
		{
			float pt = clamp(intensityLeft * invIntensityPerBone, 0.0f, 1.0f);
			t->set_scale(t->get_scale() * pt);
			intensityLeft -= intensityPerBone;
		}
	}
}

void IntensityToNumber::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (intensityVar.is_valid())
	{
		dependsOnVariables.push_back(intensityVar.get_name());
	}
}

//

REGISTER_FOR_FAST_CAST(IntensityToNumberData);

AppearanceControllerData* IntensityToNumberData::create_data()
{
	return new IntensityToNumberData();
}

void IntensityToNumberData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(particleModifierIntensityToNumber), create_data);
}

IntensityToNumberData::IntensityToNumberData()
{
}

bool IntensityToNumberData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= intensityVar.load_from_xml(_node, TXT("intensityVarID"));
	result &= minIntensity.load_from_xml(_node, TXT("minIntensity"));
	result &= maxIntensity.load_from_xml(_node, TXT("maxIntensity"));
	result &= intensityBlendTime.load_from_xml(_node, TXT("intensityBlendTime"));

	return result;
}

AppearanceControllerData* IntensityToNumberData::create_copy() const
{
	IntensityToNumberData* copy = new IntensityToNumberData();
	*copy = *this;
	return copy;
}

AppearanceController* IntensityToNumberData::create_controller()
{
	return new IntensityToNumber(this);
}

void IntensityToNumberData::apply_mesh_gen_params(MeshGeneration::GenerationContext const& _context)
{
	base::apply_mesh_gen_params(_context);

	intensityVar.fill_value_with(_context);
	minIntensity.fill_value_with(_context);
	maxIntensity.fill_value_with(_context);
	intensityBlendTime.fill_value_with(_context);
}

