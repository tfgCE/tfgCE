#include "ac_atRotator.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"
#include "..\..\module\moduleSound.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(atRotator);

//

REGISTER_FOR_FAST_CAST(AtRotator);

AtRotator::AtRotator(AtRotatorData const * _data)
: base(_data)
, atRotatorData(_data)
{
	relativeToDefault = atRotatorData->relativeToDefault;
	bone = atRotatorData->bone.get();
	variable = atRotatorData->variable.is_set() ? atRotatorData->variable.get() : variable;
	outputVariable = atRotatorData->outputVariable.is_set() ? atRotatorData->outputVariable.get() : outputVariable;
	rotator0 = atRotatorData->rotator0.get();
	rotator1 = atRotatorData->rotator1.get(rotator0);
	value0 = atRotatorData->value0.get();
	value1 = atRotatorData->value1.get();
	blendTime = atRotatorData->blendTime.get();
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(blendCurve);
	rotationSound = atRotatorData->rotationSound.get(rotationSound);
}

AtRotator::~AtRotator()
{
}

void AtRotator::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	bone.look_up(get_owner()->get_skeleton()->get_skeleton());

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	variable.look_up<float>(varStorage);
	outputVariable.look_up<float>(varStorage);
}

void AtRotator::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void AtRotator::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(atRotator_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid() ||
		get_active() == 0.0f)
	{
		return;
	}

	Meshes::Pose * poseLS = _context.access_pose_LS();

	int const boneIdx = bone.get_index();

	// where should we be
	float shouldBeAt = variable.is_valid() ? (has_just_activated() ? variable.access<float>() : blend_to_using_speed_based_on_time(variableValue, variable.access<float>(), blendTime, _context.get_delta_time())) : 0.0f;
	variableValue = shouldBeAt;
	shouldBeAt = BlendCurve::apply(blendCurve, shouldBeAt);
	if (outputVariable.is_valid())
	{
		outputVariable.access<float>() = variableValue;
	}
	if (value1 != value0)
	{
		shouldBeAt = (shouldBeAt - value0) / (value1 - value0);
		shouldBeAt = clamp(shouldBeAt, 0.0f, 1.0f);
	}
	else
	{
		shouldBeAt = 0.0f;
	}

	// sounds
	if (rotationSound.is_valid())
	{
		bool sameRot = shouldBeAt == lastShouldBeAt;
		if (!playedRotationSound.is_set() && !sameRot)
		{
			if (auto * ms = get_owner()->get_owner()->get_sound())
			{
				playedRotationSound = ms->play_sound(rotationSound);
			}
		}
		if (playedRotationSound.is_set() && sameRot)
		{
			playedRotationSound->stop();
			playedRotationSound = nullptr;
		}
	}

	lastShouldBeAt = shouldBeAt;

	Rotator3 const finalRotator = (rotator0 + (rotator1 - rotator0) * shouldBeAt);

	// write back
	auto& boneLS = poseLS->access_bones()[boneIdx];
	if (relativeToDefault)
	{
		boneLS.set_orientation(Quat::slerp(get_active(), boneLS.get_orientation(), poseLS->get_skeleton()->get_default_pose_ls()->get_bone(boneIdx).to_world(finalRotator.to_quat())));
	}
	else
	{
		boneLS = boneLS.to_world(Transform(Vector3::zero, (finalRotator * get_active()).to_quat()));
	}
}

void AtRotator::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (bone.is_valid())
	{
		dependsOnParentBones.push_back(bone.get_index());
		providesBones.push_back(bone.get_index());
	}
	if (variable.is_valid()) dependsOnVariables.push_back(variable.get_name());
}

//

REGISTER_FOR_FAST_CAST(AtRotatorData);

AppearanceControllerData* AtRotatorData::create_data()
{
	return new AtRotatorData();
}

void AtRotatorData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(atRotator), create_data);
}

AtRotatorData::AtRotatorData()
{
}

bool AtRotatorData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	relativeToDefault = _node->get_bool_attribute_or_from_child_presence(TXT("relativeToDefault"), relativeToDefault);
	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= variable.load_from_xml(_node, TXT("varID"));
	result &= outputVariable.load_from_xml(_node, TXT("outputVarID"));
	result &= rotator0.load_from_xml_child_node(_node, TXT("rotator"));
	result &= rotator1.load_from_xml_child_node(_node, TXT("rotator"));
	result &= rotator0.load_from_xml_child_node(_node, TXT("rotator0"));
	result &= rotator1.load_from_xml_child_node(_node, TXT("rotator1"));
	result &= value0.load_from_xml(_node, TXT("value0"));
	result &= value1.load_from_xml(_node, TXT("value1"));
	result &= blendTime.load_from_xml(_node, TXT("blendTime"));
	result &= blendCurve.load_from_xml(_node, TXT("blendCurve"));
	result &= rotationSound.load_from_xml(_node, TXT("rotationSound"));

	if (!bone.is_set())
	{
		error_loading_xml(_node, TXT("no bone provided"));
		result = false;
	}

	return result;
}

bool AtRotatorData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* AtRotatorData::create_copy() const
{
	AtRotatorData* copy = new AtRotatorData();
	*copy = *this;
	return copy;
}

AppearanceController* AtRotatorData::create_controller()
{
	return new AtRotator(this);
}

void AtRotatorData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
	variable.if_value_set([this, _rename](){ variable = _rename(variable.get(), NP); });
	outputVariable.if_value_set([this, _rename](){ outputVariable = _rename(outputVariable.get(), NP); });
}

void AtRotatorData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	variable.fill_value_with(_context);
	outputVariable.fill_value_with(_context);
	rotator0.fill_value_with(_context);
	rotator1.fill_value_with(_context);
	value0.fill_value_with(_context);
	value1.fill_value_with(_context);
	blendTime.fill_value_with(_context);
	blendCurve.fill_value_with(_context);
	rotationSound.fill_value_with(_context);
}

