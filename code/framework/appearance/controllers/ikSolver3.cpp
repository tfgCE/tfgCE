#include "ikSolver3.h"

#include "ikUtils.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"

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

DEFINE_STATIC_NAME(IKSolver3);

//

REGISTER_FOR_FAST_CAST(IKSolver3);

IKSolver3::IKSolver3(IKSolver3Data const * _data)
: base(_data)
, ikSolver3Data(_data)
, isValid(false)
{
	bones[BoneTip] = ikSolver3Data->tipBone.get();
	targetPlacementMSVar = ikSolver3Data->targetPlacementMSVar.get();
	straightSegmentVar = ikSolver3Data->straightSegmentVar.get();
}

IKSolver3::~IKSolver3()
{
}

void IKSolver3::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	isValid = true;

	if (Meshes::Skeleton const * skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		// build chain
		bones[BoneTip].look_up(skeleton);
		for (int i = BoneTip - 1; i >= 0; --i)
		{
			bones[i] = Meshes::BoneID(skeleton, skeleton->get_parent_of(bones[i + 1].get_index()));
			isValid &= bones[i].is_valid();
		}
	}
	else
	{
		isValid = false;
	}

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	targetPlacementMSVar.look_up<Transform>(varStorage);
	straightSegmentVar.look_up<float>(varStorage);
}

void IKSolver3::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void IKSolver3::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	if (!isValid)
	{
		return;
	}

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(ikSolver3_finalPose);
#endif

	debug_filter(ac_ikSolver3);
	debug_context(this->get_owner()->get_owner()->get_presence()->get_in_room());
	debug_push_transform(this->get_owner()->get_owner()->get_presence()->get_placement());

	Meshes::Pose * poseLS = _context.access_pose_LS();

	Transform const targetPlacementMS = targetPlacementMSVar.get<Transform>();
	float const straightSegment = straightSegmentVar.get<float>();

	// because we have three segments we are using additional parameter that defines which segment do we make straight
	//
	//				  (1)--------(2)
	//				   |		  |			straight segment
	//				   |	      |			0.0		01->02 is straight
	//				   |		  |			1.0		12->2T is straight
	//		(P) - - - (0)		 (T)
	//
	// we actually calculate where (1) is on circle when we do those calculations
	//
	//				   ..
	//					 ''.
	//						'.
	//						 '.
	//		(P) - - - (0)	  .	 (T)
	//

	// read all MS transforms and lengths
	Transform bonesMS[BoneCount];
	float lengths[3];
	bonesMS[BoneParent] = poseLS->get_bone_ms_from_ls(bones[BoneParent].get_index());
	// ls to ms
	{
		Meshes::BoneID* bone = &bones[BoneFirstSegment];
		Transform* boneMS = &bonesMS[BoneFirstSegment];
		Transform* prevBoneMS = &bonesMS[BoneParent];
		for (int i = BoneFirstSegment; i < BoneCount; ++i, ++bone, ++boneMS, ++prevBoneMS)
		{
			Transform const& boneLS = poseLS->get_bone(bone->get_index());
			*boneMS = prevBoneMS->to_world(boneLS);
		}
		{
			Meshes::BoneID* bone = &bones[BoneFirstSegment + 1];
			float* length = lengths;
			for (int i = 0; i < 3; ++i, ++bone, ++length)
			{
				Transform const& boneLS = poseLS->get_bone(bone->get_index());
				*length = boneLS.get_translation().length();
			}
		}
	}

	Vector3 const firstMS = bonesMS[BoneFirstSegment].get_translation();
	Vector3 const tipMS = bonesMS[BoneTip].get_translation();
	Vector3 tipTargetMS = targetPlacementMS.get_translation();

	if (ikSolver3Data->restrictTargetPlacementToBendPlane)
	{
		// put back onto bend plane
		Vector3 right = bonesMS[BoneFirstSegment].get_orientation().get_x_axis().normal();

		tipTargetMS = tipTargetMS - right * Vector3::dot(tipTargetMS - firstMS, right);
	}

	if (ikSolver3Data->allowStretching)
	{
		float targetLength = (tipTargetMS - firstMS).length();
		float availableLength = lengths[0] + lengths[1] + lengths[2];
		if (availableLength < targetLength && availableLength != 0.0f)
		{
			float scaleUp = targetLength / availableLength;
			lengths[0] *= scaleUp;
			lengths[1] *= scaleUp;
			lengths[2] *= scaleUp;
		}
	}
	debug_draw_line(true, Colour::green, firstMS, tipMS);
	debug_draw_line(true, Colour::red, firstMS, tipTargetMS);

	// calculate our reference matrix
	Vector3 const loc = firstMS;
	Vector3 const orgFwd = (tipMS - loc).normal();
	Vector3 const orgRight = bonesMS[BoneFirstSegment].get_orientation().get_x_axis();
	Vector3 const orgUp = Vector3::cross(orgRight, orgFwd).normal();
	Vector3 const fwd = (tipTargetMS - loc).normal();
	//debug_draw_line(true, Colour::lerp(0.3f, Colour::black, Colour::red), loc, loc + orgFwd * 0.1f);
	//debug_draw_line(true, Colour::lerp(0.3f, Colour::black, Colour::green), loc, loc + orgRight * 0.1f);
	//debug_draw_line(true, Colour::lerp(0.3f, Colour::black, Colour::blue), loc, loc + orgUp * 0.1f);
	Vector3 right;
	Vector3 up;
	if (ikSolver3Data->restrictTargetPlacementToBendPlane)
	{
		// try to keep "up" (axis) dir the same
		// because we're already restricted
		right = Vector3::cross(fwd, orgUp).normal();
		if (Vector3::dot(orgRight, right) < 0.0f)
		{
			right = -right;
		}
	}
	else
	{
		// try to keep "right" (axis) dir the same
		// try to realign using last right dir and find new up (as forward is most important, second important is right, and up is just a result)
		// but we will use up to calculate proper right dir
		if (lastRightMS.is_zero())
		{
			lastRightMS = orgRight;
		}
		up = Vector3::cross(lastRightMS, fwd).normal();
		right = Vector3::cross(fwd, up).normal();
		//debug_draw_line(true, Colour::orange, loc, loc + orgRight * 0.2f);
		//debug_draw_line(true, Colour::green, loc, loc + lastRightMS * 0.2f);
		if (Vector3::dot(lastRightMS, right) < 0.0f)
		{
			// we want right to be in the same direction, even if this means that up will flip
			right = -right;
		}
		lastRightMS = right;
	}
	up = Vector3::cross(right, fwd).normal();

	//debug_draw_line(true, Colour::red, loc, loc + fwd * 0.1f);
	//debug_draw_line(true, Colour::green, loc, loc + right * 0.1f);
	//debug_draw_line(true, Colour::blue, loc, loc + up * 0.1f);

	float const dist0T = (tipTargetMS - firstMS).length();
	float const lenghts01 = lengths[0] + lengths[1];

	if (dist0T < lenghts01 + lengths[2])
	{
		//float maxAllowed02Straight = clamp(lengths[0] + lengths[1], dist0T - lengths[2], dist0T + lengths[2]);
		//float maxAllowed1TStraight = clamp(lengths[1] + lengths[2], dist0T - lengths[0], dist0T + lengths[0]);
		float const maxAllowed02Straight = lengths[0] + lengths[1];
		float const maxAllowed1TStraight = lengths[1] + lengths[2];

		// calculate where how we will be bending when straight in boht extremes
		Vector2 const joint02Straight = IKUtils::ik2_calculate_joint_loc(maxAllowed02Straight, lengths[2], dist0T);
		Vector2 const joint1TStraight = IKUtils::ik2_calculate_joint_loc(lengths[0], maxAllowed1TStraight, dist0T);

		// calculate where that point should be
		Vector2 const point1_02S = (joint02Straight * lengths[0]) / lenghts01;
		Vector2 const point1_1TS = joint1TStraight;

		// calculate angles
		float const invLength0 = 1.0f / lengths[0];
		float const angle02S = acos_deg(clamp(point1_02S.x * invLength0, -1.0f, 1.0f));
		float const angle1TS = acos_deg(clamp(point1_1TS.x * invLength0, -1.0f, 1.0f));

		float const angle = (1.0f - straightSegment) * angle02S + straightSegment * angle1TS;
		float const point1xNormalised = cos_deg(angle);
		Vector2 const point1 = Vector2(point1xNormalised, max(0.0f, sqrt(1.0f - sqr(point1xNormalised)))) * lengths[0];

		// we have point 1 location calculated

		// calculate point 1 in MS
		Vector3 const point1MS = loc + fwd * point1.x + up * point1.y * ikSolver3Data->bendFirstJoint.get();

		// store our final bone
		bonesMS[BoneFirstSegment] = IKUtils::ik2_bone(loc, point1MS, right).to_transform().make_sane();

		// now simplier, ik-2 case for two bones
		{
			float const dist1T = (tipTargetMS - point1MS).length();
			Vector2 const point2 = IKUtils::ik2_calculate_joint_loc(lengths[1], lengths[2], dist1T);

			// calculate reference matrix for ik-2 case - they share right dir!
			Vector3 const second_loc = point1MS;
			Vector3 const second_fwd = (tipTargetMS - second_loc).normal();
			Vector3 const second_right = right; // same!
			Vector3 const second_up = Vector3::cross(second_right, second_fwd);

			// calculate point 2 in MS
			Vector3 const point2MS = second_loc + second_fwd * point2.x + second_up * point2.y * ikSolver3Data->bendSecondJoint.get();

			// and calculate transforms
			bonesMS[BoneSecondSegment] = IKUtils::ik2_bone(second_loc, point2MS, second_right).to_transform().make_sane();
			bonesMS[BoneThirdSegment] = IKUtils::ik2_bone(point2MS, tipTargetMS, second_right).to_transform().make_sane();
			bonesMS[BoneTip] = Transform(tipTargetMS, targetPlacementMS.get_orientation()).make_sane();
		}
	}
	else
	{
		// all straight towards tip
		bonesMS[BoneFirstSegment] = IKUtils::ik2_bone(loc, tipTargetMS, right).to_transform().make_sane();
		bonesMS[BoneSecondSegment] = IKUtils::ik2_bone(bonesMS[BoneFirstSegment].get_translation() + fwd * lengths[0], tipTargetMS, right).to_transform().make_sane();
		bonesMS[BoneThirdSegment] = IKUtils::ik2_bone(bonesMS[BoneSecondSegment].get_translation() + fwd * lengths[1], tipTargetMS, right).to_transform().make_sane();
		bonesMS[BoneTip] = Transform(bonesMS[BoneThirdSegment].get_translation() + fwd * lengths[2], targetPlacementMS.get_orientation()).make_sane();
	}

	debug_draw_line(true, Colour::orange, bonesMS[BoneFirstSegment].get_translation(), bonesMS[BoneSecondSegment].get_translation());
	debug_draw_line(true, Colour::orange, bonesMS[BoneSecondSegment].get_translation(), bonesMS[BoneThirdSegment].get_translation());
	debug_draw_line(true, Colour::orange, bonesMS[BoneThirdSegment].get_translation(), bonesMS[BoneTip].get_translation());

	// ms to ls
	{
		Meshes::BoneID* bone = &bones[BoneFirstSegment];
		Transform* boneMS = &bonesMS[BoneFirstSegment];
		Transform* prevBoneMS = &bonesMS[BoneParent];
		for (int i = BoneFirstSegment; i <= BoneTip; ++i, ++bone, ++boneMS, ++prevBoneMS)
		{
			poseLS->set_bone(bone->get_index(), prevBoneMS->to_local(*boneMS));
		}
	}

#ifndef AN_CLANG
	an_assert(poseLS->is_ok());
#endif

	debug_pop_transform();
	debug_no_context();
	debug_no_filter();
}

void IKSolver3::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	dependsOnBones.push_back(bones[BoneParent].get_index());
	dependsOnBones.push_back(bones[BoneFirstSegment].get_index());
	dependsOnBones.push_back(bones[BoneSecondSegment].get_index());
	dependsOnBones.push_back(bones[BoneThirdSegment].get_index());
	providesBones.push_back(bones[BoneFirstSegment].get_index());
	providesBones.push_back(bones[BoneSecondSegment].get_index());
	providesBones.push_back(bones[BoneThirdSegment].get_index());
	providesBones.push_back(bones[BoneTip].get_index());

	if (targetPlacementMSVar.is_valid())
	{
		dependsOnVariables.push_back(targetPlacementMSVar.get_name());
	}

	if (straightSegmentVar.is_valid())
	{
		dependsOnVariables.push_back(straightSegmentVar.get_name());
	}	
}

//

REGISTER_FOR_FAST_CAST(IKSolver3Data);

AppearanceControllerData* IKSolver3Data::create_data()
{
	return new IKSolver3Data();
}

void IKSolver3Data::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(IKSolver3), create_data);
}

IKSolver3Data::IKSolver3Data()
: bendFirstJoint(1.0f)
, bendSecondJoint(1.0f)
, restrictTargetPlacementToBendPlane(false)
, allowStretching(false)
{
}

bool IKSolver3Data::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= tipBone.load_from_xml(_node, TXT("tipBone"));

	restrictTargetPlacementToBendPlane = _node->get_bool_attribute(TXT("restrictTargetPlacementToBendPlane"), restrictTargetPlacementToBendPlane);
	allowStretching = _node->get_bool_attribute(TXT("allowStretching"), allowStretching);

	result &= targetPlacementMSVar.load_from_xml(_node, TXT("targetPlacementMSVarID"));
	result &= straightSegmentVar.load_from_xml(_node, TXT("straightSegmentVarID"));

	result &= bendFirstJoint.load_from_xml(_node, TXT("bendFirstJoint"));
	result &= bendSecondJoint.load_from_xml(_node, TXT("bendSecondJoint"));
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("bendFirstJoint")))
	{
		bendFirstJoint = (attr->get_as_string() == TXT("negative") || attr->get_as_string() == TXT("neg")) ? -1.0f : 1.0f;
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("bendSecondJoint")))
	{
		bendSecondJoint = (attr->get_as_string() == TXT("negative") || attr->get_as_string() == TXT("neg")) ? -1.0f : 1.0f;
	}

	return result;
}

bool IKSolver3Data::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* IKSolver3Data::create_copy() const
{
	IKSolver3Data* copy = new IKSolver3Data();
	*copy = *this;
	return copy;
}

AppearanceController* IKSolver3Data::create_controller()
{
	return new IKSolver3(this);
}

void IKSolver3Data::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	tipBone.if_value_set([this, _rename](){ tipBone = _rename(tipBone.get(), NP); });
	targetPlacementMSVar.if_value_set([this, _rename](){ targetPlacementMSVar = _rename(targetPlacementMSVar.get(), NP); });
	straightSegmentVar.if_value_set([this, _rename](){ straightSegmentVar = _rename(straightSegmentVar.get(), NP); });
}

void IKSolver3Data::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	tipBone.fill_value_with(_context);
	bendFirstJoint.fill_value_with(_context);
	bendSecondJoint.fill_value_with(_context);
	targetPlacementMSVar.fill_value_with(_context);
	straightSegmentVar.fill_value_with(_context);

	// make it sane
	bendFirstJoint = sign(bendFirstJoint.get());
	bendSecondJoint = sign(bendSecondJoint.get());
}

