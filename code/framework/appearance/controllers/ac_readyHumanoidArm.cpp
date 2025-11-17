#include "ac_readyHumanoidArm.h"

#include "ikUtils.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\mesh\pose.h"
#include "..\..\..\core\wheresMyPoint\iWMPTool.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\performance\performanceUtils.h"
#include "..\..\..\core\system\input.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(ReadyHumanoidArm);

//

REGISTER_FOR_FAST_CAST(ReadyHumanoidArm);

ReadyHumanoidArm::ReadyHumanoidArm(ReadyHumanoidArmData const * _data)
: base(_data)
, readyHumanoidArmData(_data)
{
	armBone = readyHumanoidArmData->armBone.get();
	handBone = readyHumanoidArmData->handBone.get();
	handControlBone = readyHumanoidArmData->handControlBone.get();

	outUpDirMSVar = readyHumanoidArmData->outUpDirMSVar.get();
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(allowStretchVar);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(disallowStretchVar);
	outStretchArmVar = readyHumanoidArmData->outStretchArmVar.get();
	outStretchForearmVar = readyHumanoidArmData->outStretchForearmVar.get();
	if (readyHumanoidArmData->outRequestedLengthXYPtVar.is_set()) { outRequestedLengthXYPtVar = readyHumanoidArmData->outRequestedLengthXYPtVar.get(); }

	maxStretchedLengthPt = readyHumanoidArmData->maxStretchedLengthPt.get(1.1f);
	startStretchingAtPt = readyHumanoidArmData->startStretchingAtPt.get(0.95f);
	
	//

	right = handControlBone.get_name().to_string().has_prefix(readyHumanoidArmData->rightPrefix);
}

ReadyHumanoidArm::~ReadyHumanoidArm()
{
}

void ReadyHumanoidArm::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	outUpDirMSVar.look_up<Vector3>(varStorage);
	allowStretchVar.look_up<float>(varStorage);
	disallowStretchVar.look_up<float>(varStorage);
	outStretchArmVar.look_up<Transform>(varStorage);
	outStretchForearmVar.look_up<Transform>(varStorage);
	outRequestedLengthXYPtVar.look_up<float>(varStorage);

	if (Meshes::Skeleton const * skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		armBone.look_up(skeleton);
		handBone.look_up(skeleton);
		handControlBone.look_up(skeleton);

		{
			int forearm = skeleton->get_parent_of(handBone.get_index());
			forearmBone = Meshes::BoneID(skeleton, forearm);
			int arm = skeleton->get_parent_of(forearm);
			if (arm != armBone.get_index())
			{
				error(TXT("armBone and arm taken from hand do not match!"));
			}
		}			
	}
}

void ReadyHumanoidArm::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void ReadyHumanoidArm::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	auto const * mesh = get_owner()->get_mesh();
	if (!mesh)
	{
		return;
	}

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(readyHumanoidArm_finalPose);
#endif

	debug_filter(ac_readyHumanoidArm);
	debug_context(this->get_owner()->get_owner()->get_presence()->get_in_room());
	debug_push_transform(this->get_owner()->get_owner()->get_presence()->get_placement());

	float rightOne = right ? 1.0f : -1.0f;

	Meshes::Pose* poseLS = _context.access_pose_LS();

	Transform armMS = poseLS->get_bone_ms_from_ls(armBone.get_index());
	Transform forearmMS = armMS.to_world(poseLS->get_bone(forearmBone.get_index()));
	Transform handMS = forearmMS.to_world(poseLS->get_bone(handBone.get_index()));
	Transform handControlMS = poseLS->get_bone_ms_from_ls(handControlBone.get_index());

	debug_draw_transform_size(true, armMS, 0.2f);

	Vector3 fromMS = armMS.get_translation();
	Vector3 atMS = handControlMS.get_translation();
	Vector3 fwdMS = (atMS - fromMS).normal();
	float distance2d = (atMS - fromMS).length_2d();

	Vector3 handFwdMS = handControlMS.get_axis(Axis::Y);


	// model up is most important but we also pay attention to hand dirs
	Vector3 newArmUpDirMS = (Vector3(0.0f, 0.0f, 1.0f) * 0.9f + 0.3f * handControlMS.get_axis(Axis::Z) + 0.4f * handFwdMS).normal();

	Matrix44 armLookAtMatrix = look_at_matrix(fromMS, atMS, newArmUpDirMS);

	float distFromAt = (atMS - fromMS).length();
	float toSideReinforce = 1.0f + clamp((0.3f - distFromAt) * 8.0f, 0.0f, 1.0f);

	Vector3 outUpDirMS = armLookAtMatrix.vector_to_world(Vector3((rightOne * -0.9f) * toSideReinforce, -0.2f, 1.0f));
	outUpDirMS.normalise();
	debug_draw_arrow(true, Colour::c64Brown, fromMS, fromMS + outUpDirMS);

	float useHandFwd = 1.0f * clamp(1.0f - abs(Vector3::dot(handFwdMS, fwdMS)), 0.0f, 1.0f);
	outUpDirMS = outUpDirMS * (1.0f - useHandFwd) + handFwdMS * useHandFwd;
	debug_draw_arrow(true, Colour::orange, fromMS, fromMS + outUpDirMS);
	debug_draw_arrow(true, Colour::orange, fromMS, fromMS + handFwdMS * 0.2f);
	outUpDirMS.normalise();

	// if too close, use just arms right
	float useCloseCase = clamp(1.0f - (distance2d - 0.2f) / 0.15f, 0.0f, 1.0f);
	if (useCloseCase > 0.0f)
	{
		debug_draw_arrow(true, Colour::orange, fromMS, fromMS + outUpDirMS);
		Vector3 upCloseCaseDirMS = Vector3(-rightOne, 0.0f, 0.5f).normal();
		debug_draw_arrow(true, Colour::red, fromMS, fromMS + upCloseCaseDirMS);
		outUpDirMS = outUpDirMS * (1.0f - useCloseCase) + useCloseCase * upCloseCaseDirMS;
		outUpDirMS.normalise();
	}

	outUpDirMSVar.access<Vector3>() = outUpDirMS;

	debug_draw_arrow(true, Colour::yellow, fromMS, fromMS + outUpDirMS);

	// check stretching
	float armLength = (forearmMS.get_translation() - armMS.get_translation()).length();
	float forearmLength = (handMS.get_translation() - forearmMS.get_translation()).length();
	float totalLength = armLength + forearmLength;
	float requestedLength = (handControlMS.get_translation() - armMS.get_translation()).length();
	float requestedLengthXY = (handControlMS.get_translation() - armMS.get_translation()).length_2d();

	float stretchArm = 0.0f;
	float stretchForearm = 0.0f;

	float minStretchLength = totalLength * startStretchingAtPt;
	if (requestedLength > minStretchLength &&
		maxStretchedLengthPt > startStretchingAtPt)
	{
		float maxStretchLength = totalLength * maxStretchedLengthPt;
		float stretchToLength = min(requestedLength, maxStretchLength);
		float stretchByPt = ((stretchToLength - minStretchLength) / (maxStretchLength - minStretchLength)) * (maxStretchedLengthPt - 1.0f);

		stretchArm = stretchByPt * armLength;
		stretchForearm = stretchByPt * forearmLength;
	}

	{
		float allowStretch = 1.0f;
		if (allowStretchVar.is_valid()) allowStretch *= allowStretchVar.get<float>();
		if (disallowStretchVar.is_valid()) allowStretch *= (1.0f - disallowStretchVar.get<float>());
		stretchArm *= allowStretch;
		stretchForearm *= allowStretch;
	}

	if (outStretchArmVar.is_valid())
	{
		outStretchArmVar.access<Transform>() = Transform(Vector3::yAxis * stretchArm, Quat::identity);
	}
	if (outStretchForearmVar.is_valid())
	{
		outStretchForearmVar.access<Transform>() = Transform(Vector3::yAxis * stretchForearm, Quat::identity);
	}
	if (outRequestedLengthXYPtVar.is_valid())
	{
		outRequestedLengthXYPtVar.access<float>() = totalLength != 0.0f? requestedLengthXY / totalLength : 0.0f;
	}

	debug_pop_transform();
	debug_no_context();
	debug_no_filter();
}

void ReadyHumanoidArm::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);
}

//

REGISTER_FOR_FAST_CAST(ReadyHumanoidArmData);

AppearanceControllerData* ReadyHumanoidArmData::create_data()
{
	return new ReadyHumanoidArmData();
}

void ReadyHumanoidArmData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(ReadyHumanoidArm), create_data);
}

ReadyHumanoidArmData::ReadyHumanoidArmData()
{
}

bool ReadyHumanoidArmData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= armBone.load_from_xml(_node, TXT("armBone"));
	result &= handBone.load_from_xml(_node, TXT("handBone"));
	result &= handControlBone.load_from_xml(_node, TXT("handControlBone"));
	result &= outUpDirMSVar.load_from_xml(_node, TXT("outUpDirMSVarID"));
	result &= allowStretchVar.load_from_xml(_node, TXT("allowStretchVarID"));
	result &= disallowStretchVar.load_from_xml(_node, TXT("disallowStretchVarID"));
	result &= outStretchArmVar.load_from_xml(_node, TXT("outStretchArmVarID"));
	result &= outStretchForearmVar.load_from_xml(_node, TXT("outStretchForearmVarID"));
	result &= outRequestedLengthXYPtVar.load_from_xml(_node, TXT("outRequestedLengthXYPtVarID"));
	result &= maxStretchedLengthPt.load_from_xml(_node, TXT("maxStretchedLengthPt"));
	result &= startStretchingAtPt.load_from_xml(_node, TXT("startStretchingAtPt"));

	rightPrefix = _node->get_string_attribute_or_from_child(TXT("rightPrefix"), rightPrefix);
	leftPrefix = _node->get_string_attribute_or_from_child(TXT("leftPrefix"), leftPrefix);

	return result;
}

bool ReadyHumanoidArmData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* ReadyHumanoidArmData::create_copy() const
{
	ReadyHumanoidArmData* copy = new ReadyHumanoidArmData();
	*copy = *this;
	return copy;
}

AppearanceController* ReadyHumanoidArmData::create_controller()
{
	return new ReadyHumanoidArm(this);
}

void ReadyHumanoidArmData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	armBone.if_value_set([this, _rename]() { armBone = _rename(armBone.get(), NP); });
	handBone.if_value_set([this, _rename]() { handBone = _rename(handBone.get(), NP); });
	handControlBone.if_value_set([this, _rename]() { handControlBone = _rename(handControlBone.get(), NP); });
	outUpDirMSVar.if_value_set([this, _rename]() { outUpDirMSVar = _rename(outUpDirMSVar.get(), NP); });
	allowStretchVar.if_value_set([this, _rename]() { allowStretchVar = _rename(allowStretchVar.get(), NP); });
	disallowStretchVar.if_value_set([this, _rename]() { disallowStretchVar = _rename(disallowStretchVar.get(), NP); });
	outStretchArmVar.if_value_set([this, _rename]() { outStretchArmVar = _rename(outStretchArmVar.get(), NP); });
	outStretchForearmVar.if_value_set([this, _rename]() { outStretchForearmVar = _rename(outStretchForearmVar.get(), NP); });
	outRequestedLengthXYPtVar.if_value_set([this, _rename]() { outRequestedLengthXYPtVar = _rename(outRequestedLengthXYPtVar.get(), NP); });
}

void ReadyHumanoidArmData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	armBone.fill_value_with(_context);
	handBone.fill_value_with(_context);
	handControlBone.fill_value_with(_context);
	outUpDirMSVar.fill_value_with(_context);
	allowStretchVar.fill_value_with(_context);
	disallowStretchVar.fill_value_with(_context);
	outStretchArmVar.fill_value_with(_context);
	outStretchForearmVar.fill_value_with(_context);
	outRequestedLengthXYPtVar.fill_value_with(_context);
	maxStretchedLengthPt.fill_value_with(_context);
	startStretchingAtPt.fill_value_with(_context);
}

void ReadyHumanoidArmData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}
