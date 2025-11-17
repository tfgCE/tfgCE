#include "ac_pilgrimsFakePointing.h"

#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\gameplay\modulePilgrimData.h"
#include "..\..\modules\custom\mc_grabable.h"

#include "..\..\..\framework\appearance\appearanceControllerPoseContext.h"
#include "..\..\..\framework\appearance\appearanceControllers.h"

#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\performance\performanceUtils.h"

#include "..\..\..\core\debug\debugRenderer.h"

//

using namespace TeaForGodEmperor;
using namespace AppearanceControllersLib;

//

#ifdef AN_DEVELOPMENT_OR_PROFILER
#define DEBUG_DRAW_GRAB
#endif

//

DEFINE_STATIC_NAME(pilgrimsFakePointing);

//

REGISTER_FOR_FAST_CAST(PilgrimsFakePointing);

PilgrimsFakePointing::PilgrimsFakePointing(PilgrimsFakePointingData const * _data)
: base(_data)
, pilgrimsFakePointingData(_data)
{
	bone = pilgrimsFakePointingData->bone.get();
}

PilgrimsFakePointing::~PilgrimsFakePointing()
{
}

void PilgrimsFakePointing::initialise(Framework::ModuleAppearance* _owner)
{
	base::initialise(_owner);

	if (auto* skel = get_owner()->get_skeleton()->get_skeleton())
	{
		bone.look_up(skel);
	}
}

void PilgrimsFakePointing::advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void PilgrimsFakePointing::calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(pilgrimsFakePointing_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid())
	{
		return;
	}

	float pointingTarget = 0.0f;
	if (auto* pilgrim = get_owner()->get_owner()->get_gameplay_as<ModulePilgrim>())
	{
		pointingTarget = pilgrim->get_fake_pointing(pilgrimsFakePointingData->hand);
	}

	pointingActive = pointingTarget;// blend_to_using_speed_based_on_time(pointingActive, pointingTarget, pilgrimsFakePointingData->pointingTime, _context.get_delta_time());

	if (pointingActive > 0.0f)
	{
		int const boneIdx = bone.get_index();
		Meshes::Pose * poseLS = _context.access_pose_LS();

		Transform boneLS = poseLS->get_bone(boneIdx);

		Transform newBoneLS = boneLS.to_world(Transform(pilgrimsFakePointingData->pointingOffset, pilgrimsFakePointingData->pointingRotation.to_quat()));
		poseLS->access_bones()[boneIdx] = Transform::lerp(pointingActive, boneLS, newBoneLS);
	}
}

void PilgrimsFakePointing::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (bone.is_valid())
	{
		providesBones.push_back(bone.get_index());
	}
}

//

REGISTER_FOR_FAST_CAST(PilgrimsFakePointingData);

Framework::AppearanceControllerData* PilgrimsFakePointingData::create_data()
{
	return new PilgrimsFakePointingData();
}

void PilgrimsFakePointingData::register_itself()
{
	Framework::AppearanceControllers::register_appearance_controller(NAME(pilgrimsFakePointing), create_data);
}

PilgrimsFakePointingData::PilgrimsFakePointingData()
{
}

bool PilgrimsFakePointingData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	hand = Hand::parse(_node->get_string_attribute_or_from_child(TXT("hand"), String(Hand::to_char(hand))));
	pointingTime = _node->get_float_attribute_or_from_child(TXT("pointingTime"), pointingTime);

	pointingOffset.load_from_xml_child_node(_node, TXT("offset"));
	pointingRotation.load_from_xml_child_node(_node, TXT("rotation"));

	return result;
}

bool PilgrimsFakePointingData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::AppearanceControllerData* PilgrimsFakePointingData::create_copy() const
{
	PilgrimsFakePointingData* copy = new PilgrimsFakePointingData();
	*copy = *this;
	return copy;
}

Framework::AppearanceController* PilgrimsFakePointingData::create_controller()
{
	return new PilgrimsFakePointing(this);
}

void PilgrimsFakePointingData::apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
}

void PilgrimsFakePointingData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

void PilgrimsFakePointingData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
}
