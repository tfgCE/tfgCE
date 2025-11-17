#include "ac_bob.h"

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
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(bob);

//

REGISTER_FOR_FAST_CAST(Bob);

Bob::Bob(BobData const * _data)
: base(_data)
{
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(bobBlendTime);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(bobAmount);

	for_every(atb, _data->applyToBones)
	{
		ApplyToBone a;
		a.bone = atb->bone.get();
		a.amount = atb->amount.is_set() ? atb->amount.get() : 1.0f;
		applyToBones.push_back(a);
	}
	for_every(l, _data->legs)
	{
		Leg leg;
		leg.bone = l->bone.get();
		leg.relToBone = l->relToBone.get();
		legs.push_back(leg);
	}
}

Bob::~Bob()
{
}

void Bob::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	if (auto* s = get_owner()->get_skeleton()->get_skeleton())
	{
		for_every(applyToBone, applyToBones)
		{
			applyToBone->bone.look_up(get_owner()->get_skeleton()->get_skeleton());
		}
		for_every(leg, legs)
		{
			leg->bone.look_up(get_owner()->get_skeleton()->get_skeleton());
			leg->relToBone.look_up(get_owner()->get_skeleton()->get_skeleton());
		}
	}
}

void Bob::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void Bob::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	if (! get_active())
	{
		return;
	}

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(sway_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		applyToBones.is_empty())
	{
		return;
	}

	Meshes::Pose const* prevPoseMS = get_owner()->get_final_pose_MS(true);
	Meshes::Pose* poseLS = _context.access_pose_LS();

	auto* skeleton = get_owner()->get_skeleton()->get_skeleton();

	float bobDownZ = 0.0f;

	for_every(leg, legs)
	{
		Vector3 legBoneMS = prevPoseMS->get_bone(leg->bone.get_index()).get_translation();
		Vector3 legRelToBoneMS = prevPoseMS->get_bone(leg->relToBone.get_index()).get_translation();
		Vector3 defLegBoneMS = skeleton->get_bones_default_placement_MS()[leg->bone.get_index()].get_translation();
		Vector3 defLegRelToBoneMS = skeleton->get_bones_default_placement_MS()[leg->relToBone.get_index()].get_translation();

		// first, calculate how we should scale default leg length
		float currentBaseLength = abs((legRelToBoneMS - defLegBoneMS).z);
		float defBaseLength = abs((defLegRelToBoneMS - defLegBoneMS).z);
		float lengthCoefFromBase = min(1.0f, currentBaseLength / defBaseLength);

		// calculate what length should it be
		float defLegLength = (defLegRelToBoneMS - defLegBoneMS).length();
		float curLength = defLegLength * lengthCoefFromBase;

		// now check how much should we move on Z axis
		// we have xy offset from "relTo" and we have length, we may calculate valid Z
		float xyOff = (legBoneMS - legRelToBoneMS).length_2d();
		float relToBoneMSValidMSZ;
		{
			float z = sqrt(max(0.0f, sqr(curLength) - sqr(xyOff)));
			relToBoneMSValidMSZ = legBoneMS.z + z;
		}
		bobDownZ = min(bobDownZ, relToBoneMSValidMSZ - legRelToBoneMS.z);
	}

	bobZ = blend_to_using_time(bobZ, bobDownZ * get_active() * bobAmount, bobBlendTime, _context.get_delta_time());
	
	for_every(atb, applyToBones)
	{
		Transform boneMS = poseLS->get_bone_ms_from_ls(atb->bone.get_index());
		{
			Vector3 loc = boneMS.get_translation();
			loc.z += bobZ * atb->amount;
			boneMS.set_translation(loc);
		}
		poseLS->set_bone_ms(atb->bone.get_index(), boneMS);
	}
}

void Bob::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	for_every(atb, applyToBones)
	{
		providesBones.push_back(atb->bone.get_index());
	}
}

//

REGISTER_FOR_FAST_CAST(BobData);

AppearanceControllerData* BobData::create_data()
{
	return new BobData();
}

void BobData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(bob), create_data);
}

BobData::BobData()
{
}

bool BobData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bobBlendTime.load_from_xml(_node, TXT("bobBlendTime"));
	result &= bobAmount.load_from_xml(_node, TXT("bobAmount"));

	applyToBones.clear();
	for_every(node, _node->children_named(TXT("applyTo")))
	{
		ApplyToBone atb;
		atb.bone.load_from_xml(node, TXT("bone"));
		atb.amount.load_from_xml(node, TXT("amount"));
		applyToBones.push_back(atb);
	}

	legs.clear();
	for_every(node, _node->children_named(TXT("leg")))
	{
		Leg leg;
		leg.bone.load_from_xml(node, TXT("bone"));
		leg.relToBone.load_from_xml(node, TXT("relToBone"));
		legs.push_back(leg);
	}

	return result;
}

bool BobData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* BobData::create_copy() const
{
	BobData* copy = new BobData();
	*copy = *this;
	return copy;
}

AppearanceController* BobData::create_controller()
{
	return new Bob(this);
}

void BobData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bobBlendTime.fill_value_with(_context);
	bobAmount.fill_value_with(_context);

	for_every(applyToBone, applyToBones)
	{
		applyToBone->bone.fill_value_with(_context);
		applyToBone->amount.fill_value_with(_context);
	}
	for_every(leg, legs)
	{
		leg->bone.fill_value_with(_context);
		leg->relToBone.fill_value_with(_context);
	}
}

void BobData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

void BobData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	for_every(applyToBone, applyToBones)
	{
		applyToBone->bone.if_value_set([applyToBone, _rename]() { applyToBone->bone = _rename(applyToBone->bone.get(), NP); });
	}
	for_every(leg, legs)
	{
		leg->bone.if_value_set([leg, _rename]() { leg->bone = _rename(leg->bone.get(), NP); });
		leg->relToBone.if_value_set([leg, _rename]() { leg->relToBone = _rename(leg->relToBone.get(), NP); });
	}
}
