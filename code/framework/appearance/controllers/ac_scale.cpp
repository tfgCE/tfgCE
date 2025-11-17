#include "ac_scale.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\performance\performanceUtils.h"

using namespace Framework;
using namespace AppearanceControllersLib;

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

DEFINE_STATIC_NAME(scale);

//

REGISTER_FOR_FAST_CAST(Scale);

Scale::Scale(ScaleData const * _data)
: base(_data)
, scaleData(_data)
{
	bone = scaleData->bone.get();
	variable = scaleData->variable.is_set()? scaleData->variable.get() : variable;
	if (scaleData->initialValue.is_set()) initialValue = scaleData->initialValue.get();
	value0 = scaleData->value0.get();
	value1 = scaleData->value1.get();
	scale0 = scaleData->scale0.get();
	scale1 = scaleData->scale1.get();
	blendTime = scaleData->blendTime.get();
	delayTime = scaleData->delayTime.get();
}

Scale::~Scale()
{
}

void Scale::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	bone.look_up(get_owner()->get_skeleton()->get_skeleton());

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	variable.look_up<float>(varStorage);
}

void Scale::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void Scale::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(scale_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid() ||
		get_active() == 0.0f)
	{
		return;
	}

	// where should we be
	float shouldBeAt = 0.0f;
	if (variable.is_valid())
	{
		if (variable.type_id() == type_id<float>())
		{
			shouldBeAt = variable.get<float>();
		}
		else
		{
			error(TXT("unsupported"));
		}		
	}
	else
	{
		shouldBeAt = 1.0f;
	}
	if (has_just_activated())
	{
		variableValue = initialValue.get(shouldBeAt);
	}
	else
	{
		variableValue = blend_to_using_speed_based_on_time(variableValue, shouldBeAt, blendTime, _context.get_delta_time());
	}
	shouldBeAt = variableValue;
	if (value1 != value0)
	{
		shouldBeAt = (shouldBeAt - value0) / (value1 - value0);
		shouldBeAt = clamp(shouldBeAt, 0.0f, 1.0f);
	}

	if (delayTime != 0.0f && !has_just_activated())
	{
		if (delayedShouldBeAtTarget != shouldBeAt)
		{
			delayedShouldBeAtTarget = shouldBeAt;
			delayedTime = 0.0f;
		}
		if (delayedShouldBeAt != shouldBeAt)
		{
			delayedTime += _context.get_delta_time();
			if (delayedTime >= delayTime)
			{
				delayedShouldBeAt = shouldBeAt;
			}
		}
		shouldBeAt = delayedShouldBeAt;
	}
	else
	{
		delayedShouldBeAt = shouldBeAt;
		delayedTime = 0.0f;
	}
	Meshes::Pose * poseLS = _context.access_pose_LS();
	int const boneIdx = bone.get_index();
	auto & boneLS = poseLS->access_bones()[boneIdx];
	float newScale = (scale0 * (1.0f - shouldBeAt) + scale1 * shouldBeAt);
	newScale = get_active() * newScale + (1.0f - get_active());
	boneLS.set_scale(boneLS.get_scale() * newScale);
}

void Scale::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (bone.is_valid())
	{
		dependsOnParentBones.push_back(bone.get_index());
		providesBones.push_back(bone.get_index());
	}
	if (variable.is_valid())
	{
		dependsOnVariables.push_back(variable.get_name());
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
	AppearanceControllers::register_appearance_controller(NAME(scale), create_data);
}

ScaleData::ScaleData()
{
}

bool ScaleData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= variable.load_from_xml(_node, TXT("varID"));

	result &= initialValue.load_from_xml(_node, TXT("initialValue"));
	result &= value0.load_from_xml(_node, TXT("value"));
	result &= value1.load_from_xml(_node, TXT("value"));
	result &= value0.load_from_xml(_node, TXT("value0"));
	result &= value1.load_from_xml(_node, TXT("value1"));
	result &= scale0.load_from_xml(_node, TXT("scale"));
	result &= scale1.load_from_xml(_node, TXT("scale"));
	result &= scale0.load_from_xml(_node, TXT("scale0"));
	result &= scale1.load_from_xml(_node, TXT("scale1"));
	result &= blendTime.load_from_xml(_node, TXT("blendTime"));
	result &= delayTime.load_from_xml(_node, TXT("delayTime"));

	if (!bone.is_set())
	{
		error_loading_xml(_node, TXT("no bone provided"));
		result = false;
	}

	return result;
}

bool ScaleData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

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

void ScaleData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
	variable.if_value_set([this, _rename](){ variable = _rename(variable.get(), NP); });
}

void ScaleData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	variable.fill_value_with(_context);
	initialValue.fill_value_with(_context);
	value0.fill_value_with(_context);
	value1.fill_value_with(_context);
	scale0.fill_value_with(_context);
	scale1.fill_value_with(_context);
	blendTime.fill_value_with(_context);
	delayTime.fill_value_with(_context);
}

