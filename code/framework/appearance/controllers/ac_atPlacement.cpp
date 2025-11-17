#include "ac_atPlacement.h"

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

DEFINE_STATIC_NAME(atPlacement);

//

REGISTER_FOR_FAST_CAST(AtPlacement);

AtPlacement::AtPlacement(AtPlacementData const * _data)
: base(_data)
, atPlacementData(_data)
{
	bone = atPlacementData->bone.get();
	variable = atPlacementData->variable.is_set() ? atPlacementData->variable.get() : variable;
	outputVariable = atPlacementData->outputVariable.is_set() ? atPlacementData->outputVariable.get() : outputVariable;
	placementType = atPlacementData->placementType;
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(allowedDistance);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(placement0);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(placement1);
	placement0Var = atPlacementData->placement0Var.is_set() ? atPlacementData->placement0Var.get() : placement0Var;
	placement1Var = atPlacementData->placement1Var.is_set() ? atPlacementData->placement1Var.get() : placement1Var;
	value0 = atPlacementData->value0.get();
	value1 = atPlacementData->value1.get();
	blendTime = atPlacementData->blendTime.get();
	locationOnly = atPlacementData->locationOnly.get();
	orientationOnly = atPlacementData->orientationOnly.get();
	movementSound = atPlacementData->movementSound.get(movementSound);
	movementSoundEarlyStop = atPlacementData->movementSoundEarlyStop.is_set()? atPlacementData->movementSoundEarlyStop.get(movementSoundEarlyStop) : movementSoundEarlyStop;
}

AtPlacement::~AtPlacement()
{
}

void AtPlacement::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	if (auto* s = get_owner()->get_skeleton())
	{
		bone.look_up(s->get_skeleton());
	}

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	variable.look_up<float>(varStorage);
	outputVariable.look_up<float>(varStorage);
	placement0Var.look_up<Transform>(varStorage);
	placement1Var.look_up<Transform>(varStorage);
}

void AtPlacement::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void AtPlacement::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(atPlacement_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid())
	{
		return;
	}

	Meshes::Pose * poseLS = _context.access_pose_LS();

	int const boneIdx = bone.get_index();

	// update variable value
	float targetValue = variable.is_valid()? variable.get<float>() : 0.0f;
	if (atPlacementData->remapVariable.is_valid())
	{
		targetValue = atPlacementData->remapVariable.remap(targetValue);
	}
	variableValue = has_just_activated() ? targetValue : blend_to_using_speed_based_on_time(variableValue, targetValue, blendTime, _context.get_delta_time());
	float variableValueEarlyStop = has_just_activated() ? targetValue : blend_to_using_speed_based_on_time(variableValue, targetValue, blendTime, max(_context.get_delta_time(), movementSoundEarlyStop));
	if (outputVariable.is_valid())
	{
		outputVariable.access<float>() = variableValue;
	}

	// sounds
	if (movementSound.is_valid())
	{
		bool samePlace = variableValueEarlyStop == targetValue;
		if (!playedMovementSound.is_set() && !samePlace)
		{
			if (auto* ms = get_owner()->get_owner()->get_sound())
			{
				playedMovementSound = ms->play_sound(movementSound);
			}
		}
		if (playedMovementSound.is_set() && samePlace)
		{
			playedMovementSound->stop();
			playedMovementSound = nullptr;
		}
	}

	// where should we be (placement, curve speaking)
	float shouldBeAt = variableValue;

	if (value1 != value0)
	{
		shouldBeAt = (shouldBeAt - value0) / (value1 - value0);
		shouldBeAt = clamp(shouldBeAt, 0.0f, 1.0f);
	}
	else
	{
		shouldBeAt = 0.0f;
	}

	if (atPlacementData->curve.is_set())
	{
		float t = 0.0f;
		if (atPlacementData->curveAccuracy.is_set())
		{
			t = BezierCurveTBasedPoint<float>::find_t_at(shouldBeAt, atPlacementData->curve.get(), nullptr, atPlacementData->curveAccuracy.get());
		}
		else
		{
			t = BezierCurveTBasedPoint<float>::find_t_at(shouldBeAt, atPlacementData->curve.get(), nullptr);
		}
		shouldBeAt = atPlacementData->curve.get().calculate_at(t).p;
	}

	Transform usePlacement0 = Transform::identity;
	Transform usePlacement1 = Transform::identity;

	if (placementType == AtPlacementType::BS)
	{
		usePlacement0 = placement0.get(Transform::identity);
		usePlacement1 = placement1.get(Transform::identity);
	}
	else if (placementType == AtPlacementType::MS ||
			 placementType == AtPlacementType::WS)
	{
		Transform boneXS = poseLS->get_bone_ms_from_ls(boneIdx);
		if (placementType == AtPlacementType::WS)
		{
			boneXS = get_owner()->get_ms_to_ws_transform().to_world(boneXS);
		}
		usePlacement0 = placement0.get(boneXS);
		usePlacement1 = placement1.get(boneXS);
	}

	usePlacement0 = placement0Var.is_valid()? placement0Var.get<Transform>() : usePlacement0;
	usePlacement1 = placement1Var.is_valid()? placement1Var.get<Transform>() : usePlacement1;

	// linear interpolation
	Transform bone = Transform::lerp(shouldBeAt, usePlacement0, usePlacement1);

	// write back
	auto& boneLS = poseLS->access_bones()[boneIdx];
	if (placementType == AtPlacementType::BS)
	{
		if (allowedDistance > 0.0f)
		{
			Vector3 bLoc = bone.get_translation();
			float bLen = bLoc.length();
			if (bLen > allowedDistance)
			{
				bLoc = bLoc.normal() * allowedDistance;
				bone.set_translation(bLoc);
			}
		}
		if (locationOnly)
		{
			bone.set_orientation(Quat::identity);
		}
		if (orientationOnly)
		{
			bone.set_translation(Vector3::zero);
		}
		boneLS = lerp(get_active(), boneLS, boneLS.to_world(bone));
	}
	else if (placementType == AtPlacementType::MS ||
			 placementType == AtPlacementType::WS)
	{
		int const boneParentIdx = poseLS->get_skeleton()->get_parent_of(boneIdx);
		Transform const boneParentMS = poseLS->get_bone_ms_from_ls(boneParentIdx);
		Transform useBone = bone;
		if (locationOnly)
		{
			Transform const boneMS = poseLS->get_bone_ms_from_ls(boneIdx);
			useBone.set_orientation(boneMS.get_orientation());
		}
		if (orientationOnly)
		{
			Transform const boneMS = poseLS->get_bone_ms_from_ls(boneIdx);
			useBone.set_translation(boneMS.get_translation());
		}
		if (placementType == AtPlacementType::WS)
		{
			useBone = get_owner()->get_ms_to_ws_transform().to_local(useBone);
		}
		if (allowedDistance > 0.0f)
		{
			Vector3 orgLoc = boneParentMS.location_to_world(boneLS.get_translation());
			Vector3 diff = useBone.get_translation() - orgLoc;
			float diffLen = diff.length();
			if (diffLen> allowedDistance)
			{
				Vector3 useLoc = orgLoc + diff.normal() * allowedDistance;
				useBone.set_translation(useLoc);
			}
		}
		boneLS = lerp(get_active(), boneLS, boneParentMS.to_local(useBone));
	}
}

void AtPlacement::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
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

REGISTER_FOR_FAST_CAST(AtPlacementData);

AppearanceControllerData* AtPlacementData::create_data()
{
	return new AtPlacementData();
}

void AtPlacementData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(atPlacement), create_data);
}

AtPlacementData::AtPlacementData()
{
}

bool AtPlacementData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= variable.load_from_xml(_node, TXT("varID"));
	result &= allowedDistance.load_from_xml_child_node(_node, TXT("allowedDistance"));
	result &= placement0.load_from_xml_child_node(_node, TXT("placement"));
	result &= placement0.load_from_xml_child_node(_node, TXT("placement0"));
	result &= placement1.load_from_xml_child_node(_node, TXT("placement1"));
	result &= placement0Var.load_from_xml(_node, TXT("placementVarID"));
	result &= placement0Var.load_from_xml(_node, TXT("placement0VarID"));
	result &= placement1Var.load_from_xml(_node, TXT("placement1VarID"));
	result &= outputVariable.load_from_xml(_node, TXT("outputVarID"));
	result &= value0.load_from_xml(_node, TXT("value0"));
	result &= value1.load_from_xml(_node, TXT("value1"));
	result &= blendTime.load_from_xml(_node, TXT("blendTime"));
	result &= locationOnly.load_from_xml(_node, TXT("locationOnly"));
	result &= orientationOnly.load_from_xml(_node, TXT("orientationOnly"));
	result &= movementSound.load_from_xml(_node, TXT("movementSound"));
	result &= movementSoundEarlyStop.load_from_xml(_node, TXT("movementSoundEarlyStop"));

	if (auto const* node = _node->first_child_named(TXT("varMap")))
	{
		remapVariable.clear();
		remapVariable.load_from_xml(node);
	}

	if (auto const* node = _node->first_child_named(TXT("curve")))
	{
		curveAccuracy.load_from_xml(node, TXT("accuracy"));
		BezierCurve<BezierCurveTBasedPoint<float>> c;
		c.p0.t = 0.0f;
		c.p0.p = 0.0f;
		c.p3.t = 1.0f;
		c.p3.p = 1.0f;
		c.make_linear();
		c.make_roundly_separated();
		if (c.load_from_xml(node))
		{
			curve = c;
		}
	}

	placementType = AtPlacementType::parse(_node->get_string_attribute_or_from_child(TXT("type")), placementType);

	if (!bone.is_set())
	{
		error_loading_xml(_node, TXT("no bone provided"));
		result = false;
	}
	
	return result;
}

bool AtPlacementData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* AtPlacementData::create_copy() const
{
	AtPlacementData* copy = new AtPlacementData();
	*copy = *this;
	return copy;
}

AppearanceController* AtPlacementData::create_controller()
{
	return new AtPlacement(this);
}

void AtPlacementData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
	variable.if_value_set([this, _rename](){ variable = _rename(variable.get(), NP); });
	outputVariable.if_value_set([this, _rename](){ outputVariable = _rename(outputVariable.get(), NP); });
	placement0Var.if_value_set([this, _rename]() { placement0Var = _rename(placement0Var.get(), NP); });
	placement1Var.if_value_set([this, _rename]() { placement1Var = _rename(placement1Var.get(), NP); });
}

void AtPlacementData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	variable.fill_value_with(_context);
	outputVariable.fill_value_with(_context);
	allowedDistance.fill_value_with(_context);
	placement0.fill_value_with(_context);
	placement1.fill_value_with(_context);
	placement0Var.fill_value_with(_context);
	placement1Var.fill_value_with(_context);
	value0.fill_value_with(_context);
	value1.fill_value_with(_context);
	blendTime.fill_value_with(_context);
	locationOnly.fill_value_with(_context);
	orientationOnly.fill_value_with(_context);
	movementSound.fill_value_with(_context);
	movementSoundEarlyStop.fill_value_with(_context);
}

