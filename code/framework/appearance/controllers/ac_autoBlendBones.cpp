#include "ac_autoBlendBones.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"
#include "..\..\module\moduleSound.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(autoBlendBones);

//

REGISTER_FOR_FAST_CAST(AutoBlendBones);

AutoBlendBones::AutoBlendBones(AutoBlendBonesData const * _data)
: base(_data)
{
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(bone);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(monitorBone);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(outAtTargetVar);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(linearSpeed);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(rotationSpeed);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(linearSpeedVar);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(rotationSpeedVar);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(blendTimeVar);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(blendCurve);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(sound);
}

AutoBlendBones::~AutoBlendBones()
{
}

void AutoBlendBones::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	if (auto* skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		bone.look_up(skeleton);
		monitorBone.look_up(skeleton);
		linearSpeedVar.look_up<float>(_owner->get_owner()->access_variables());
		rotationSpeedVar.look_up<float>(_owner->get_owner()->access_variables());
		blendTimeVar.look_up<float>(_owner->get_owner()->access_variables());
		outAtTargetVar.look_up<bool>(_owner->get_owner()->access_variables());

		if (bone.get_index() != NONE)
		{
			bones.set_size(skeleton->get_num_bones());
			for_every(b, bones)
			{
				int bIdx = for_everys_index(b);
				if (bIdx == bone.get_index())
				{
					b->inUse = true;
				}
				else
				{
					int boneParentIdx = skeleton->get_parent_of(bIdx);
					if (boneParentIdx != NONE &&
						bones[boneParentIdx].inUse)
					{
						b->inUse = true;
					}
				}
			}
		}
	}
}

void AutoBlendBones::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void AutoBlendBones::calculate_final_pose(REF_ AppearanceControllerPoseContext& _context)
{
	base::calculate_final_pose(_context);

	auto const* mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid() ||
		!monitorBone.is_valid())
	{
		return;
	}

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(autoBlendBones_finalPose);
#endif

	if (has_just_activated())
	{
		lastMonitorBonePoseMS.clear();
		blendTimeLeft = 0.0f;
	}

	Meshes::Pose* poseLS = _context.access_pose_LS();

	if (get_active() > 0.0f && get_active_target() > 0.0f)
	{
		Transform currMonitorBonePoseMS = poseLS->get_bone_ms_from_ls(monitorBone.get_index());
		if (lastMonitorBonePoseMS.is_set())
		{
			if (!Transform::same_with_orientation(lastMonitorBonePoseMS.get(), currMonitorBonePoseMS))
			{
				float useLinearSpeed = linearSpeed;
				float useRotationSpeed = rotationSpeed;
				if (linearSpeedVar.is_valid()) useLinearSpeed = linearSpeedVar.get<float>();
				if (rotationSpeedVar.is_valid()) useRotationSpeed = rotationSpeedVar.get<float>();
				Transform diff = lastMonitorBonePoseMS.get().to_local(currMonitorBonePoseMS);
				Rotator3 diffR = diff.get_orientation().to_rotator();
				float timeFromRotation = useRotationSpeed != 0.0f? diffR.length() / useRotationSpeed : 0.0f;
				float timeFromMovement = useLinearSpeed != 0.0f? diff.get_translation().length() / useLinearSpeed : 0.0f;
				currentBlendTime = max(0.1f, max(timeFromRotation, timeFromMovement));
				if (blendTimeVar.is_valid())
				{
					currentBlendTime = blendTimeVar.get<float>();
				}
				blendTimeLeft = currentBlendTime;
				// use the last pose as the starting one
				for_every(b, bones)
				{
					if (b->inUse)
					{
						b->blendStartPoseLS = b->poseLS;
					}
				}
			}

		}
		lastMonitorBonePoseMS = currMonitorBonePoseMS;
	}

	if (blendTimeLeft == 0.0f)
	{
		if (playedSound.is_set())
		{
			playedSound->stop();
			playedSound.clear();
		}
	}
	else if (sound.is_valid())
	{
		if (!playedSound.is_set())
		{
			if (auto* ms = get_owner()->get_owner()->get_sound())
			{
				playedSound = ms->play_sound(sound);
			}
		}
	}

	if (blendTimeLeft > 0.0f)
	{
		// check how much should we move forward
		blendTimeLeft = max(0.0f, blendTimeLeft - _context.get_delta_time());
		float atPt = currentBlendTime != 0.0f? clamp(1.0f - blendTimeLeft / currentBlendTime, 0.0f, 1.0f) : 1.0f;
		atPt = BlendCurve::apply(blendCurve, atPt);

		// blend pose
		auto* destBonePoseLS = poseLS->access_bones().begin();
		for_every(b, bones)
		{
			if (b->inUse)
			{
				*destBonePoseLS = Transform::lerp(atPt, b->blendStartPoseLS, *destBonePoseLS);
			}
			++destBonePoseLS;
		}
	}

	if (outAtTargetVar.is_valid())
	{
		outAtTargetVar.access<bool>() = blendTimeLeft == 0.0f;
	}

	// store current pose
	{
		auto* srcBonePoseLS = poseLS->get_bones().begin();
		for_every(b, bones)
		{
			if (b->inUse)
			{
				b->poseLS = *srcBonePoseLS;
			}
			++srcBonePoseLS;
		}
	}
}

void AutoBlendBones::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (bone.is_valid())
	{
		dependsOnParentBones.push_back(bone.get_index());
		providesBones.push_back(bone.get_index());
	}
	if (monitorBone.is_valid())
	{
		dependsOnParentBones.push_back(bone.get_index());
	}
	if (outAtTargetVar.is_valid())
	{
		providesVariables.push_back(outAtTargetVar.get_name());
	}
}

//

REGISTER_FOR_FAST_CAST(AutoBlendBonesData);

AppearanceControllerData* AutoBlendBonesData::create_data()
{
	return new AutoBlendBonesData();
}

void AutoBlendBonesData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(autoBlendBones), create_data);
}

AutoBlendBonesData::AutoBlendBonesData()
{
}

bool AutoBlendBonesData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= monitorBone.load_from_xml(_node, TXT("monitorBone"));
	result &= outAtTargetVar.load_from_xml(_node, TXT("outAtTargetVarID"));
	result &= linearSpeed.load_from_xml(_node, TXT("linearSpeed"));
	result &= rotationSpeed.load_from_xml(_node, TXT("rotationSpeed"));
	result &= linearSpeedVar.load_from_xml(_node, TXT("linearSpeedVarID"));
	result &= rotationSpeedVar.load_from_xml(_node, TXT("rotationSpeedVarID"));
	result &= blendTimeVar.load_from_xml(_node, TXT("blendTimeVarID"));
	result &= blendCurve.load_from_xml(_node, TXT("blendCurve"));
	result &= sound.load_from_xml_child_node(_node, TXT("sound"));

	return result;
}

bool AutoBlendBonesData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* AutoBlendBonesData::create_copy() const
{
	AutoBlendBonesData* copy = new AutoBlendBonesData();
	*copy = *this;
	return copy;
}

AppearanceController* AutoBlendBonesData::create_controller()
{
	return new AutoBlendBones(this);
}

void AutoBlendBonesData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
	monitorBone.if_value_set([this, _rename](){ monitorBone = _rename(monitorBone.get(), NP); });
	linearSpeedVar.if_value_set([this, _rename](){ linearSpeedVar = _rename(linearSpeedVar.get(), NP); });
	rotationSpeedVar.if_value_set([this, _rename](){ rotationSpeedVar = _rename(rotationSpeedVar.get(), NP); });
	blendTimeVar.if_value_set([this, _rename](){ blendTimeVar = _rename(blendTimeVar.get(), NP); });
	outAtTargetVar.if_value_set([this, _rename](){ outAtTargetVar = _rename(outAtTargetVar.get(), NP); });
	blendCurve.if_value_set([this, _rename](){ blendCurve = _rename(blendCurve.get(), NP); });
	sound.if_value_set([this, _rename](){ sound = _rename(sound.get(), NP); });
}

void AutoBlendBonesData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

void AutoBlendBonesData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	monitorBone.fill_value_with(_context);
	outAtTargetVar.fill_value_with(_context);
	linearSpeed.fill_value_with(_context);
	rotationSpeed.fill_value_with(_context);
	linearSpeedVar.fill_value_with(_context);
	rotationSpeedVar.fill_value_with(_context);
	blendTimeVar.fill_value_with(_context);
	blendCurve.fill_value_with(_context);
	sound.fill_value_with(_context);
}
