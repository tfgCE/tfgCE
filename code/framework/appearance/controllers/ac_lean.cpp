#include "ac_lean.h"

#include "walker.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
//#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(lean);

//

REGISTER_FOR_FAST_CAST(Lean);

Lean::Lean(LeanData const * _data)
: base(_data)
, leanData(_data)
{
	bone = leanData->bone.get();
	pivotSocket = leanData->pivotSocket.get();
	leanTo = leanData->leanTo;
	maxAngle = leanData->maxAngle.get();
	speedToAngleCoef = leanData->speedToAngleCoef.get();
	angleBlendTime = leanData->angleBlendTime.get();
}

Lean::~Lean()
{
}

void Lean::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	if (auto* skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		bone.look_up(skeleton);
		parentBone = Meshes::BoneID(skeleton, skeleton->get_parent_of(bone.get_index()));
	}

	pivotSocket.look_up(get_owner()->get_mesh());
}

void Lean::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void Lean::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	if (! get_active())
	{
		return;
	}

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(lean_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid())
	{
		return;
	}

	Rotator3 targetAngle = leanAngle;

	if (_context.get_delta_time() != 0.0f)
	{
		targetAngle = Rotator3::zero;
		if (leanTo == LeanTo::LinearVelocity)
		{
			auto* imo = get_owner()->get_owner();
			Vector3 velocityWS = imo->get_presence()->get_velocity_linear();
			Vector3 velocityOS = imo->get_presence()->get_placement().vector_to_local(velocityWS);
			targetAngle.roll =  velocityOS.x * speedToAngleCoef;
			targetAngle.pitch =  velocityOS.y * speedToAngleCoef;
		}
		if (leanTo == LeanTo::RotationVelocity ||
			leanTo == LeanTo::OrientationDifference)
		{
			float angle = 0.0f;
			auto* imo = get_owner()->get_owner();
			if (leanTo == LeanTo::RotationVelocity)
			{
				angle = imo->get_presence()->get_velocity_rotation().yaw;
			}
			else
			{
				float prevDeltaTime = imo->get_presence()->get_prev_placement_offset_delta_time();
				if (prevDeltaTime != 0.0f)
				{
					angle = -imo->get_presence()->get_prev_placement_offset().get_orientation().to_rotator().yaw / prevDeltaTime;
				}
			}
			targetAngle.roll = angle * speedToAngleCoef;
		}
	}

	leanAngle = blend_to_using_time(leanAngle, targetAngle, angleBlendTime, _context.get_delta_time());
	leanAngle.pitch = clamp(leanAngle.pitch, -maxAngle, maxAngle);
	leanAngle.yaw = clamp(leanAngle.yaw, -maxAngle, maxAngle);
	leanAngle.roll = clamp(leanAngle.roll, -maxAngle, maxAngle);

	if (has_just_activated())
	{
		leanAngle = targetAngle;
	}

	Meshes::Pose * poseLS = _context.access_pose_LS();

	Transform pivotMS = Transform::identity;
	
	if (pivotSocket.is_valid())
	{
		pivotMS = get_owner()->calculate_socket_ms(pivotSocket.get_index(), poseLS);
	}

	Transform const parentMS = poseLS->get_bone_ms_from_ls(parentBone.get_index());
	Transform boneMS = parentMS.to_world(poseLS->get_bone(bone.get_index()));

	Transform boneLS = pivotMS.to_local(boneMS);

	Transform rotateBone = Transform(Vector3::zero, (leanAngle * get_active()).to_quat());
	boneLS = rotateBone.to_world(boneLS);
		
	boneMS = pivotMS.to_world(boneLS);

	// write back
	poseLS->access_bones()[bone.get_index()] = parentMS.to_local(boneMS);
}

void Lean::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (bone.is_valid())
	{
		dependsOnParentBones.push_back(bone.get_index());
		providesBones.push_back(bone.get_index());
	}
}

//

REGISTER_FOR_FAST_CAST(LeanData);

AppearanceControllerData* LeanData::create_data()
{
	return new LeanData();
}

void LeanData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(lean), create_data);
}

LeanData::LeanData()
{
}

bool LeanData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= pivotSocket.load_from_xml(_node, TXT("pivotSocket"));
	result &= maxAngle.load_from_xml(_node, TXT("maxAngle"));
	result &= speedToAngleCoef.load_from_xml(_node, TXT("speedToAngleCoef"));
	result &= angleBlendTime.load_from_xml(_node, TXT("angleBlendTime"));

	String leanToStr = _node->get_string_attribute_or_from_child(TXT("leanTo"));
	if (!leanToStr.is_empty())
	{
		if (leanToStr == TXT("linearVelocity"))
		{
			leanTo = LeanTo::LinearVelocity;
		}
		else if (leanToStr == TXT("rotationVelocity"))
		{
			leanTo = LeanTo::RotationVelocity;
		}
		else if (leanToStr == TXT("orientationDifference"))
		{
			leanTo = LeanTo::OrientationDifference;
		}
		else
		{
			error_loading_xml(_node, TXT("could not recognise \"%S\" lean to type"), leanToStr.to_char());
			result = false;
		}
	}

	return result;
}

bool LeanData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* LeanData::create_copy() const
{
	LeanData* copy = new LeanData();
	*copy = *this;
	return copy;
}

AppearanceController* LeanData::create_controller()
{
	return new Lean(this);
}

void LeanData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	pivotSocket.fill_value_with(_context);
	maxAngle.fill_value_with(_context);
	speedToAngleCoef.fill_value_with(_context);
	angleBlendTime.fill_value_with(_context);
}

void LeanData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

void LeanData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
	pivotSocket.if_value_set([this, _rename]() { pivotSocket = _rename(pivotSocket.get(), NP); });
}
