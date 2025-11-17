#include "ac_orientAsOtherBone.h"

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

DEFINE_STATIC_NAME(OrientAsOtherBone);

//

REGISTER_FOR_FAST_CAST(OrientAsOtherBone);

OrientAsOtherBone::OrientAsOtherBone(OrientAsOtherBoneData const * _data)
: base(_data)
, orientAsOtherBoneData(_data)
{
	bone = orientAsOtherBoneData->bone.get();
	otherBone = orientAsOtherBoneData->otherBone.get();
	otherBoneB = orientAsOtherBoneData->otherBoneB.get(Name::invalid());
	otherBoneBWeight = orientAsOtherBoneData->otherBoneBWeight.get(0.0f);
	smoothOrientateToTime = orientAsOtherBoneData->smoothOrientateToTime.get(0.2f);
}

OrientAsOtherBone::~OrientAsOtherBone()
{
}

void OrientAsOtherBone::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	if (auto* skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		bone.look_up(skeleton);
		otherBone.look_up(skeleton);
		otherBoneB.look_up(skeleton);

		if (bone.get_index() != NONE)
		{
			int const boneParentIdx = skeleton->get_parent_of(bone.get_index());

			if (boneParentIdx != NONE)
			{
				boneParent = Meshes::BoneID(skeleton, boneParentIdx);
			}
		}
	}
}

void OrientAsOtherBone::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void OrientAsOtherBone::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(OrientAsOtherBone_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid() ||
		!otherBone.is_valid() ||
		!boneParent.is_valid())
	{
		return;
	}

	Meshes::Pose * poseLS = _context.access_pose_LS();

	Transform boneMS = poseLS->get_bone_ms_from_ls(bone.get_index());
	Transform otherBoneMS = poseLS->get_bone_ms_from_ls(otherBone.get_index());
	Transform otherBoneBMS = otherBoneB.is_valid()? poseLS->get_bone_ms_from_ls(otherBoneB.get_index()) : otherBoneMS;

	Quat orientateTo = otherBoneMS.get_orientation();
	if (otherBoneBWeight > 0.0f)
	{
		orientateTo = Quat::slerp(otherBoneBWeight, orientateTo, otherBoneBMS.get_orientation());
		// smooth
		{
			if (has_just_activated())
			{
				prevOrientateTo.clear();
			}
			if (prevOrientateTo.is_set())
			{
				orientateTo = Quat::slerp(clamp(smoothOrientateToTime == 0.0f? 1.0f : _context.get_delta_time() / smoothOrientateToTime, 0.0f, 1.0f), prevOrientateTo.get(), orientateTo);
			}
			prevOrientateTo = orientateTo;
		}
	}
	boneMS.set_orientation(orientateTo);

	Transform const boneParentMS = poseLS->get_bone_ms_from_ls(boneParent.get_index());
	poseLS->access_bones()[bone.get_index()] = boneParentMS.to_local(boneMS);
}

void OrientAsOtherBone::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (bone.is_valid())
	{
		dependsOnParentBones.push_back(bone.get_index());
		providesBones.push_back(bone.get_index());
	}
	if (otherBone.is_valid())
	{
		dependsOnParentBones.push_back(otherBone.get_index());
	}
	if (otherBoneB.is_valid())
	{
		dependsOnParentBones.push_back(otherBoneB.get_index());
	}
}

//

REGISTER_FOR_FAST_CAST(OrientAsOtherBoneData);

AppearanceControllerData* OrientAsOtherBoneData::create_data()
{
	return new OrientAsOtherBoneData();
}

void OrientAsOtherBoneData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(OrientAsOtherBone), create_data);
}

OrientAsOtherBoneData::OrientAsOtherBoneData()
{
}

bool OrientAsOtherBoneData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= otherBone.load_from_xml(_node, TXT("alignTo"));
	result &= otherBone.load_from_xml(_node, TXT("otherBone"));
	result &= otherBoneB.load_from_xml(_node, TXT("otherBoneB"));
	result &= otherBoneBWeight.load_from_xml(_node, TXT("otherBoneBWeight"));
	result &= smoothOrientateToTime.load_from_xml(_node, TXT("smoothOrientateToTime"));

	error_loading_xml_on_assert(bone.is_set(), result, _node, TXT("no bone provided"));
	error_loading_xml_on_assert(otherBone.is_set(), result, _node, TXT("no bone provided"));

	return result;
}

bool OrientAsOtherBoneData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* OrientAsOtherBoneData::create_copy() const
{
	OrientAsOtherBoneData* copy = new OrientAsOtherBoneData();
	*copy = *this;
	return copy;
}

AppearanceController* OrientAsOtherBoneData::create_controller()
{
	return new OrientAsOtherBone(this);
}

void OrientAsOtherBoneData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
	otherBone.if_value_set([this, _rename](){ otherBone = _rename(otherBone.get(), NP); });
	otherBoneB.if_value_set([this, _rename](){ otherBoneB = _rename(otherBoneB.get(), NP); });
}

void OrientAsOtherBoneData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	otherBone.fill_value_with(_context);
	otherBoneB.fill_value_with(_context);
	otherBoneBWeight.fill_value_with(_context);
	smoothOrientateToTime.fill_value_with(_context);
}

