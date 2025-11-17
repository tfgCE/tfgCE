#include "ac_readyRestPointArm.h"

#include "..\..\..\framework\appearance\appearanceControllerPoseContext.h"
#include "..\..\..\framework\appearance\appearanceControllers.h"

#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleSound.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\mesh\pose.h"
#include "..\..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(ReadyRestPointArm);

//

REGISTER_FOR_FAST_CAST(ReadyRestPointArm);

ReadyRestPointArm::ReadyRestPointArm(ReadyRestPointArmData const * _data)
: base(_data)
, readyRestPointArmData(_data)
{
	tipBone = readyRestPointArmData->tipBone.get();
	targetPlacementMSVar = readyRestPointArmData->targetPlacementMSVar.get();
	normalUpDirMS = readyRestPointArmData->normalUpDirMS.get();
	belowUpDirMS = readyRestPointArmData->belowUpDirMS.get();
	outUpDirMSVar = readyRestPointArmData->outUpDirMSVar.get();
	zRangeBelowToNormal = readyRestPointArmData->zRangeBelowToNormal.get();

	outStretchArmVar = readyRestPointArmData->outStretchArmVar.get();
	outStretchForearmVar = readyRestPointArmData->outStretchForearmVar.get();
	maxStretchedLengthPt = readyRestPointArmData->maxStretchedLengthPt.get(1.9f);
	startStretchingAtPt = readyRestPointArmData->startStretchingAtPt.get(0.5f);
	
	emptyArmStretchCoef = readyRestPointArmData->emptyArmStretchCoef.get(0.025f);
	emptyForearmStretchCoef = readyRestPointArmData->emptyForearmStretchCoef.get(0.0f);

	restEmptyVar = readyRestPointArmData->restEmptyVar.get();
}

ReadyRestPointArm::~ReadyRestPointArm()
{
}

void ReadyRestPointArm::initialise(Framework::ModuleAppearance* _owner)
{
	base::initialise(_owner);

	if (Meshes::Skeleton const * skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		tipBone.look_up(skeleton);
		forearmBone = Meshes::BoneID(skeleton, skeleton->get_parent_of(tipBone.get_index()));
		armBone = Meshes::BoneID(skeleton, skeleton->get_parent_of(forearmBone.get_index()));
	}

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	targetPlacementMSVar.look_up<Transform>(varStorage);
	outUpDirMSVar.look_up<Vector3>(varStorage);
	outStretchArmVar.look_up<Transform>(varStorage);
	outStretchForearmVar.look_up<Transform>(varStorage);
	restEmptyVar.look_up<float>(varStorage);
}

void ReadyRestPointArm::advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void ReadyRestPointArm::calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(readyRestPointArm_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh)
	{
		return;
	}

	Transform const targetPlacementMS = targetPlacementMSVar.get<Transform>();

	Meshes::Pose * poseLS = _context.access_pose_LS();

	Transform armMS = poseLS->get_bone_ms_from_ls(armBone.get_index());
	Transform forearmMS = armMS.to_world(poseLS->get_bone(forearmBone.get_index()));
	Transform tipMS = forearmMS.to_world(poseLS->get_bone(tipBone.get_index()));

	float z = targetPlacementMS.get_translation().z - armMS.get_translation().z;

	float pt = clamp(zRangeBelowToNormal.get_pt_from_value(z), 0.0f, 1.0f);
	
	outUpDirMSVar.access<Vector3>() = Vector3::lerp(pt, belowUpDirMS, normalUpDirMS).normal();

	// check stretching
	float armLength = (forearmMS.get_translation() - armMS.get_translation()).length();
	float forearmLength = (tipMS.get_translation() - forearmMS.get_translation()).length();
	float totalLength = armLength + forearmLength;
	float requestedLength = (targetPlacementMS.get_translation() - armMS.get_translation()).length();

	float stretchArmTarget = 0.0f;
	float stretchForearmTarget = 0.0f;

	float minStretchLength = totalLength * startStretchingAtPt;
	if (requestedLength > totalLength * startStretchingAtPt &&
		maxStretchedLengthPt > startStretchingAtPt)
	{
		float maxStretchLength = totalLength * maxStretchedLengthPt;
		float stretchToLength = min(requestedLength, maxStretchLength);
		float stretchByPt = ((stretchToLength - minStretchLength) / (maxStretchLength - minStretchLength)) * (maxStretchedLengthPt - 1.0f);

		stretchArmTarget = stretchByPt * armLength;
		stretchForearmTarget = stretchByPt * forearmLength;
	}

	if (restEmptyVar.is_valid())
	{
		float restEmpty = restEmptyVar.get<float>();
		stretchArmTarget = max(stretchArmTarget, armLength * emptyArmStretchCoef * (restEmpty));
		stretchForearmTarget = max(stretchForearmTarget, forearmLength * emptyForearmStretchCoef * (restEmpty));
	}

	stretchArm = blend_to_using_speed_based_on_time(stretchArm, stretchArmTarget, 0.05f, _context.get_delta_time());
	stretchForearm = blend_to_using_speed_based_on_time(stretchForearm, stretchForearmTarget, 0.05f, _context.get_delta_time());

	if (outStretchArmVar.is_valid())
	{
		outStretchArmVar.access<Transform>() = Transform(Vector3::yAxis * stretchArm, Quat::identity);
	}
	if (outStretchForearmVar.is_valid())
	{
		outStretchForearmVar.access<Transform>() = Transform(Vector3::yAxis * stretchForearm, Quat::identity);
	}
}

void ReadyRestPointArm::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (tipBone.is_valid())
	{
		usesBones.push_back(tipBone.get_index());
	}
	if (targetPlacementMSVar.is_valid())
	{
		dependsOnVariables.push_back(targetPlacementMSVar.get_name());
	}
	if (outUpDirMSVar.is_valid())
	{
		providesVariables.push_back(outUpDirMSVar.get_name());
	}
	if (outStretchArmVar.is_valid())
	{
		providesVariables.push_back(outStretchArmVar.get_name());
	}
	if (outStretchForearmVar.is_valid())
	{
		providesVariables.push_back(outStretchForearmVar.get_name());
	}
	if (restEmptyVar.is_valid())
	{
		dependsOnVariables.push_back(restEmptyVar.get_name());
	}
}

//

REGISTER_FOR_FAST_CAST(ReadyRestPointArmData);

Framework::AppearanceControllerData* ReadyRestPointArmData::create_data()
{
	return new ReadyRestPointArmData();
}

void ReadyRestPointArmData::register_itself()
{
	Framework::AppearanceControllers::register_appearance_controller(NAME(ReadyRestPointArm), create_data);
}

ReadyRestPointArmData::ReadyRestPointArmData()
{
}

bool ReadyRestPointArmData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= tipBone.load_from_xml(_node, TXT("tipBone"));
	result &= targetPlacementMSVar.load_from_xml(_node, TXT("targetPlacementMSVarID"));
	result &= normalUpDirMS.load_from_xml(_node, TXT("normalUpDirMS"));
	result &= belowUpDirMS.load_from_xml(_node, TXT("belowUpDirMS"));
	result &= outUpDirMSVar.load_from_xml(_node, TXT("outUpDirMSVarID"));
	result &= zRangeBelowToNormal.load_from_xml(_node, TXT("zRangeBelowToNormal"));
	result &= outStretchArmVar.load_from_xml(_node, TXT("outStretchArmVarID"));
	result &= outStretchForearmVar.load_from_xml(_node, TXT("outStretchForearmVarID"));
	result &= maxStretchedLengthPt.load_from_xml(_node, TXT("maxStretchedLengthPt"));
	result &= startStretchingAtPt.load_from_xml(_node, TXT("startStretchingAtPt"));
	result &= emptyArmStretchCoef.load_from_xml(_node, TXT("emptyArmStretchCoef"));
	result &= emptyForearmStretchCoef.load_from_xml(_node, TXT("emptyForearmStretchCoef"));
	result &= restEmptyVar.load_from_xml(_node, TXT("restEmptyVarID"));

	return result;
}

bool ReadyRestPointArmData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::AppearanceControllerData* ReadyRestPointArmData::create_copy() const
{
	ReadyRestPointArmData* copy = new ReadyRestPointArmData();
	*copy = *this;
	return copy;
}

Framework::AppearanceController* ReadyRestPointArmData::create_controller()
{
	return new ReadyRestPointArm(this);
}

void ReadyRestPointArmData::apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	tipBone.fill_value_with(_context);
	targetPlacementMSVar.fill_value_with(_context);
	normalUpDirMS.fill_value_with(_context);
	belowUpDirMS.fill_value_with(_context);
	outUpDirMSVar.fill_value_with(_context);
	zRangeBelowToNormal.fill_value_with(_context);
	outStretchArmVar.fill_value_with(_context);
	outStretchForearmVar.fill_value_with(_context);
	maxStretchedLengthPt.fill_value_with(_context);
	startStretchingAtPt.fill_value_with(_context);
	emptyArmStretchCoef.fill_value_with(_context);
	emptyForearmStretchCoef.fill_value_with(_context);
	restEmptyVar.fill_value_with(_context);
}

void ReadyRestPointArmData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);

	normalUpDirMS.if_value_set([this, _transform]() { normalUpDirMS.access() = _transform.vector_to_world(normalUpDirMS.get()); });
	belowUpDirMS.if_value_set([this, _transform]() { belowUpDirMS.access() = _transform.vector_to_world(belowUpDirMS.get()); });
}

void ReadyRestPointArmData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	tipBone.if_value_set([this, _rename]() { tipBone = _rename(tipBone.get(), NP); });
	targetPlacementMSVar.if_value_set([this, _rename]() { targetPlacementMSVar = _rename(targetPlacementMSVar.get(), NP); });
	outUpDirMSVar.if_value_set([this, _rename]() { outUpDirMSVar = _rename(outUpDirMSVar.get(), NP); });
	outStretchArmVar.if_value_set([this, _rename]() { outStretchArmVar = _rename(outStretchArmVar.get(), NP); });
	outStretchForearmVar.if_value_set([this, _rename]() { outStretchForearmVar = _rename(outStretchForearmVar.get(), NP); });
	restEmptyVar.if_value_set([this, _rename]() { restEmptyVar = _rename(restEmptyVar.get(), NP); });
}
