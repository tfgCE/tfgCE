#include "ac_mushroomHeadSide.h"

#include "..\..\..\framework\appearance\appearanceControllerPoseContext.h"
#include "..\..\..\framework\appearance\appearanceControllers.h"

#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\performance\performanceUtils.h"

//

using namespace TeaForGodEmperor;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(MushroomHeadSide);

//

REGISTER_FOR_FAST_CAST(MushroomHeadSide);

MushroomHeadSide::MushroomHeadSide(MushroomHeadSideData const * _data)
: base(_data)
, mushroomHeadSideData(_data)
{
	bone = mushroomHeadSideData->bone.get();
	headScaleBone = mushroomHeadSideData->headScaleBone.get();
}

MushroomHeadSide::~MushroomHeadSide()
{
}

void MushroomHeadSide::initialise(Framework::ModuleAppearance* _owner)
{
	base::initialise(_owner);

	if (auto* skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		bone.look_up(skeleton);
		headScaleBone.look_up(skeleton);
		boneParent = Meshes::BoneID(skeleton, skeleton->get_parent_of(bone.get_index()));
	}
}

void MushroomHeadSide::advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void MushroomHeadSide::calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(MushroomHeadSide_finalPose);
#endif

	todo_note(TXT("check health!"));

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid() ||
		!boneParent.is_valid() ||
		!headScaleBone.is_valid())
	{
		return;
	}

	int const boneIdx = bone.get_index();

	Meshes::Pose * poseLS = _context.access_pose_LS();

	Transform boneParentMS = poseLS->get_bone_ms_from_ls(boneParent.get_index());
	Transform boneMS = boneParentMS.to_world(poseLS->get_bone(boneIdx));
	Transform headScaleBoneMS = poseLS->get_bone_ms_from_ls(headScaleBone.get_index());

	// time advance
	if (has_just_activated() || ! locationMS.is_set())
	{
		locationMS = boneMS.get_translation();
	}

	float const deltaTime = _context.get_delta_time();

	timeToNewVelocity -= deltaTime;
	if (timeToNewVelocity <= 0.0f)
	{
		timeToNewVelocity = Random::get_float(0.2f, 3.0f);
		float dist = (boneMS.get_translation() - headScaleBoneMS.get_translation()).length();
		velocityTargetMS.x = Random::get_float(-1.0f, 1.0f);
		velocityTargetMS.y = Random::get_float(-1.0f, 1.0f);
		velocityTargetMS.z = Random::get_float(-1.0f, 1.0f);
		velocityTargetMS *= dist * 0.2f;
	}

	// apply headScale's scale
	boneMS.set_translation(headScaleBoneMS.get_translation() + headScaleBoneMS.get_scale() * (boneMS.get_translation() - headScaleBoneMS.get_translation()));

	locationMS = locationMS.get() + velocityMS * deltaTime;

	velocityMS = blend_to_using_time(velocityMS, velocityTargetMS, 0.5f, deltaTime);
	velocityTargetMS = blend_to_using_time(velocityTargetMS, Vector3::zero, 1.2f, deltaTime);
 
	// go back
	float dist = (boneMS.get_translation() - locationMS.get()).length();
	locationMS.access() = blend_to_using_time(locationMS.get(), boneMS.get_translation(), clamp(0.5f - dist / 0.05f, 0.025f, 0.5f), deltaTime);

	boneMS.set_translation(locationMS.get());
	boneMS.set_scale(headScaleBoneMS.get_scale());

	// write back
	poseLS->access_bones()[boneIdx] = boneParentMS.to_local(boneMS);
}

void MushroomHeadSide::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
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

REGISTER_FOR_FAST_CAST(MushroomHeadSideData);

Framework::AppearanceControllerData* MushroomHeadSideData::create_data()
{
	return new MushroomHeadSideData();
}

void MushroomHeadSideData::register_itself()
{
	Framework::AppearanceControllers::register_appearance_controller(NAME(MushroomHeadSide), create_data);
}

MushroomHeadSideData::MushroomHeadSideData()
{
}

bool MushroomHeadSideData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= headScaleBone.load_from_xml(_node, TXT("headScaleBone"));

	return result;
}

bool MushroomHeadSideData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::AppearanceControllerData* MushroomHeadSideData::create_copy() const
{
	MushroomHeadSideData* copy = new MushroomHeadSideData();
	*copy = *this;
	return copy;
}

Framework::AppearanceController* MushroomHeadSideData::create_controller()
{
	return new MushroomHeadSide(this);
}

void MushroomHeadSideData::apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	headScaleBone.fill_value_with(_context);
}

void MushroomHeadSideData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

void MushroomHeadSideData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
	headScaleBone.if_value_set([this, _rename]() { headScaleBone = _rename(headScaleBone.get(), NP); });
}
