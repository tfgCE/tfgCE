#include "ikSolver2.h"

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

DEFINE_STATIC_NAME(IKSolver2);

//

REGISTER_FOR_FAST_CAST(IKSolver2);

IKSolver2::IKSolver2(IKSolver2Data const * _data)
: base(_data)
, ikSolver2Data(_data)
, isValid(false)
{
	bones[BoneTip] = ikSolver2Data->tipBone.get();
	targetPlacementMSVar = ikSolver2Data->targetPlacementMSVar.get();
	upDirMSVar = ikSolver2Data->upDirMSVar.get(Name::invalid());
	upDirMS = ikSolver2Data->upDirMS.get(Vector3::zero);
	forcePreferRightDirMS = ikSolver2Data->forcePreferRightDirMS.get(false);
	preferRightDirMSVar = ikSolver2Data->preferRightDirMSVar.get(Name::invalid());
	preferRightDirMS = ikSolver2Data->preferRightDirMS.get(Vector3::zero);
	remapBone0 = ikSolver2Data->remapBone0.get(Transform::identity);
	remapBone1 = ikSolver2Data->remapBone1.get(Transform::identity);
	remapBone2 = ikSolver2Data->remapBone2.get(Transform::identity);
	remapBone0Inv = remapBone0.inverted();
	remapBone1Inv = remapBone1.inverted();
	remapBone2Inv = remapBone2.inverted();
}

IKSolver2::~IKSolver2()
{
}

void IKSolver2::initialise(ModuleAppearance* _owner)
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
	upDirMSVar.look_up<Vector3>(varStorage);
	preferRightDirMSVar.look_up<Vector3>(varStorage);
}

void IKSolver2::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void IKSolver2::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	if (!isValid)
	{
		return;
	}

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(ikSolver2_finalPose);
#endif

	debug_filter(ac_ikSolver2);
	debug_context(this->get_owner()->get_owner()->get_presence()->get_in_room());
	debug_push_transform(this->get_owner()->get_owner()->get_presence()->get_placement());

	Meshes::Pose * poseLS = _context.access_pose_LS();

	Transform const targetPlacementMS = targetPlacementMSVar.get<Transform>();

	// just do simple ik-2

	// read all MS transforms and lengths
	Transform bonesMS[BoneCount];
	float lengths[2];
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
			for (int i = 0; i < 2; ++i, ++bone, ++length)
			{
				Transform const& boneLS = poseLS->get_bone(bone->get_index());
				*length = boneLS.get_translation().length();
			}
		}
	}
	
	bonesMS[1] = bonesMS[1].to_world(remapBone0);
	bonesMS[2] = bonesMS[2].to_world(remapBone1);
	bonesMS[3] = bonesMS[3].to_world(remapBone2);

	Vector3 const firstMS = bonesMS[BoneFirstSegment].get_translation();
	Vector3 const tipMS = bonesMS[BoneTip].get_translation();
	Vector3 tipTargetMS = targetPlacementMS.get_translation();

	if (ikSolver2Data->restrictTargetPlacementToBendPlane)
	{
		// put back onto bend plane
		Vector3 right = bonesMS[BoneFirstSegment].get_orientation().get_x_axis().normal();

		tipTargetMS = tipTargetMS - right * Vector3::dot(tipTargetMS - firstMS, right);
	}

	debug_draw_line(true, Colour::green, firstMS, tipMS);
	debug_draw_line(true, Colour::red, firstMS, tipTargetMS);

	// calculate our reference matrix
	Vector3 const loc = firstMS;
	Vector3 const orgFwd = (tipMS - loc).normal();
	Vector3 const orgRight = bonesMS[BoneFirstSegment].get_orientation().get_x_axis();
	Vector3 const orgUp = Vector3::cross(orgRight, orgFwd).normal();
	Vector3 const fwd = (tipTargetMS - loc).normal();
	debug_draw_line(true, Colour::lerp(0.3f, Colour::black, Colour::red), loc, loc + orgFwd * 0.1f);
	debug_draw_line(true, Colour::lerp(0.3f, Colour::black, Colour::green), loc, loc + orgRight * 0.1f);
	debug_draw_line(true, Colour::lerp(0.3f, Colour::black, Colour::blue), loc, loc + orgUp * 0.1f);
	Vector3 right;
	Vector3 up;
	if (ikSolver2Data->restrictTargetPlacementToBendPlane)
	{
		// try to keep "up" (axis) dir the same
		// because we're already restricted
		right = Vector3::cross(fwd, orgUp).normal();
		if (Vector3::dot(orgRight, right) < 0.0f)
		{
			right = -right;
		}
	}
	else if (upDirMSVar.is_valid())
	{
		Vector3 upDirMS = upDirMSVar.get<Vector3>();
		up = upDirMS.normal();
		right = Vector3::cross(fwd, up).normal();
		debug_draw_line(true, Colour::orange, loc, loc + orgRight * 0.2f);
		debug_draw_line(true, Colour::purple, loc, loc + right * 0.2f);
	}
	else if (! upDirMS.is_zero())
	{
		up = upDirMS.normal();
		right = Vector3::cross(fwd, up).normal();
		debug_draw_line(true, Colour::orange, loc, loc + orgRight * 0.2f);
		debug_draw_line(true, Colour::purple, loc, loc + right * 0.2f);
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
		if (preferRightDirMSVar.is_valid() || ! preferRightDirMS.is_zero())
		{
			Vector3 prefRightDirMS = preferRightDirMS;
			if (preferRightDirMSVar.is_valid())
			{
				prefRightDirMS = preferRightDirMSVar.get<Vector3>();
			}
			if (Vector3::dot(lastRightMS, prefRightDirMS) < 0.0f)
			{
				if (forcePreferRightDirMS)
				{
					lastRightMS = -lastRightMS;
				}
				else
				{
					prefRightDirMS = -prefRightDirMS;
				}
			}
			lastRightMS = blend_to_using_time(lastRightMS, prefRightDirMS, 0.3f, _context.get_delta_time());
			lastRightMS.normalise();
		}
		up = Vector3::cross(lastRightMS, fwd).normal();
		right = Vector3::cross(fwd, up).normal();
		debug_draw_line(true, Colour::orange, loc, loc + orgRight * 0.2f);
		debug_draw_line(true, Colour::green, loc, loc + lastRightMS * 0.2f);
		if (Vector3::dot(lastRightMS, right) < 0.0f)
		{
			// we want right to be in the same direction, even if this means that up will flip
			right = -right;
		}
		lastRightMS = right;
	}
	up = Vector3::cross(right, fwd).normal();

	debug_draw_line(true, Colour::red, loc, loc + fwd * 0.3f);
	debug_draw_line(true, Colour::green, loc, loc + right * 0.3f);
	debug_draw_line(true, Colour::blue, loc, loc + up * 0.3f);

	float const dist0T = (tipTargetMS - firstMS).length();
	float const lenghtTotal = lengths[0] + lengths[1];
	if (dist0T < lenghtTotal)
	{
		Vector2 const point1 = IKUtils::ik2_calculate_joint_loc(lengths[0], lengths[1], dist0T);

		// calculate point 1 in MS
		Vector3 const point1MS = loc + fwd * point1.x + up * point1.y * ikSolver2Data->bendJoint.get();

		// and calculate transforms
		bonesMS[BoneFirstSegment] = IKUtils::ik2_bone(loc, point1MS, right).to_transform();
		bonesMS[BoneSecondSegment] = IKUtils::ik2_bone(point1MS, tipTargetMS, right).to_transform();
		bonesMS[BoneTip] = Transform(tipTargetMS, targetPlacementMS.get_orientation());
	}
	else
	{
		// all straight towards tip
		bonesMS[BoneFirstSegment] = IKUtils::ik2_bone(loc, tipTargetMS, right).to_transform();
		bonesMS[BoneSecondSegment] = IKUtils::ik2_bone(bonesMS[BoneFirstSegment].get_translation() + fwd * lengths[0], tipTargetMS, right).to_transform();
		bonesMS[BoneTip] = Transform(bonesMS[BoneSecondSegment].get_translation() + fwd * lengths[1], targetPlacementMS.get_orientation());
	}

	bonesMS[1] = bonesMS[1].to_world(remapBone0Inv);
	bonesMS[2] = bonesMS[2].to_world(remapBone1Inv);
	bonesMS[3] = bonesMS[3].to_world(remapBone2Inv);

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

void IKSolver2::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	dependsOnBones.push_back(bones[BoneParent].get_index());
	dependsOnBones.push_back(bones[BoneFirstSegment].get_index());
	dependsOnBones.push_back(bones[BoneSecondSegment].get_index());
	providesBones.push_back(bones[BoneFirstSegment].get_index());
	providesBones.push_back(bones[BoneSecondSegment].get_index());
	providesBones.push_back(bones[BoneTip].get_index());

	if (targetPlacementMSVar.is_valid())
	{
		dependsOnVariables.push_back(targetPlacementMSVar.get_name());
	}
	if (upDirMSVar.is_valid())
	{
		dependsOnVariables.push_back(upDirMSVar.get_name());
	}	
	if (preferRightDirMSVar.is_valid())
	{
		dependsOnVariables.push_back(preferRightDirMSVar.get_name());
	}	
}

//

REGISTER_FOR_FAST_CAST(IKSolver2Data);

AppearanceControllerData* IKSolver2Data::create_data()
{
	return new IKSolver2Data();
}

void IKSolver2Data::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(IKSolver2), create_data);
}

IKSolver2Data::IKSolver2Data()
: bendJoint(1.0f)
, restrictTargetPlacementToBendPlane(false)
{
}

bool IKSolver2Data::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= tipBone.load_from_xml(_node, TXT("tipBone"));

	restrictTargetPlacementToBendPlane = _node->get_bool_attribute(TXT("restrictTargetPlacementToBendPlane"), restrictTargetPlacementToBendPlane);

	result &= targetPlacementMSVar.load_from_xml(_node, TXT("targetPlacementMSVarID"));
	result &= upDirMSVar.load_from_xml(_node, TXT("upDirMSVarID"));
	result &= upDirMS.load_from_xml(_node, TXT("upDirMS"));
	result &= forcePreferRightDirMS.load_from_xml(_node, TXT("forcePreferRightDirMS"));
	result &= preferRightDirMSVar.load_from_xml(_node, TXT("preferRightDirMSVarID"));
	result &= preferRightDirMS.load_from_xml(_node, TXT("preferRightDirMS"));

	result &= remapBone0.load_from_xml(_node, TXT("remapBone0"));
	result &= remapBone1.load_from_xml(_node, TXT("remapBone1"));
	result &= remapBone2.load_from_xml(_node, TXT("remapBone2"));

	result &= bendJoint.load_from_xml(_node, TXT("bendJoint"));
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("bendJoint")))
	{
		bendJoint = (attr->get_as_string() == TXT("negative") || attr->get_as_string() == TXT("neg")) ? -1.0f : 1.0f;
	}

	return result;
}

bool IKSolver2Data::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* IKSolver2Data::create_copy() const
{
	IKSolver2Data* copy = new IKSolver2Data();
	*copy = *this;
	return copy;
}

AppearanceController* IKSolver2Data::create_controller()
{
	return new IKSolver2(this);
}

void IKSolver2Data::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	tipBone.if_value_set([this, _rename](){ tipBone = _rename(tipBone.get(), NP); });
	targetPlacementMSVar.if_value_set([this, _rename](){ targetPlacementMSVar = _rename(targetPlacementMSVar.get(), NP); });
	upDirMSVar.if_value_set([this, _rename]() { upDirMSVar = _rename(upDirMSVar.get(), NP); });
	preferRightDirMSVar.if_value_set([this, _rename]() { preferRightDirMSVar = _rename(preferRightDirMSVar.get(), NP); });
}

void IKSolver2Data::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);

	upDirMS.if_value_set([this, _transform]() { upDirMS.access() = _transform.vector_to_world(upDirMS.get()); });
	preferRightDirMS.if_value_set([this, _transform]() { preferRightDirMS.access() = _transform.vector_to_world(preferRightDirMS.get()); });
	remapBone0.if_value_set([this, _transform]() { remapBone0.access() = _transform.to_transform().to_world(remapBone0.get()); });
	remapBone1.if_value_set([this, _transform]() { remapBone1.access() = _transform.to_transform().to_world(remapBone1.get()); });
	remapBone2.if_value_set([this, _transform]() { remapBone2.access() = _transform.to_transform().to_world(remapBone2.get()); });
}

void IKSolver2Data::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	tipBone.fill_value_with(_context);
	bendJoint.fill_value_with(_context);
	targetPlacementMSVar.fill_value_with(_context);
	upDirMSVar.fill_value_with(_context);
	upDirMS.fill_value_with(_context);
	forcePreferRightDirMS.fill_value_with(_context);
	preferRightDirMSVar.fill_value_with(_context);
	preferRightDirMS.fill_value_with(_context);
	remapBone0.fill_value_with(_context);
	remapBone1.fill_value_with(_context);
	remapBone2.fill_value_with(_context);

	// make it sane
	bendJoint = sign(bendJoint.get());
}

