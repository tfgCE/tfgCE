#include "ac_shake.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\meshGen\meshGenValueDefImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\performance\performanceUtils.h"

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(shake);

//

REGISTER_FOR_FAST_CAST(Shake);

Shake::Shake(ShakeData const * _data)
: base(_data)
, shakeData(_data)
{
	bone = shakeData->bone.get();
	shakeLength = shakeData->shakeLength.get();
	shakeLocationBS = shakeData->shakeLocationBS.get();
	shakeRotationBS = shakeData->shakeRotationBS.get();
}

Shake::~Shake()
{
}

void Shake::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	bone.look_up(get_owner()->get_skeleton()->get_skeleton());
}

void Shake::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void Shake::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	if (! get_active())
	{
		return;
	}

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(shake_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid())
	{
		return;
	}

	// time advance
	if (has_just_activated())
	{
		shakeAt = 0.0f;
	}

	shakeLength = max(shakeLength, 0.001f);
	shakeAt = mod(shakeAt + _context.get_delta_time(), shakeLength);

	// where in the shake are we
	float pt = shakeAt / shakeLength;
	an_assert(pt >= 0.0f && pt <= 1.0f);

	float applyShake = get_active() * sin_deg(360.0f * pt);

	// calculate shake to apply
	Transform shakeTotal = Transform::identity;

	shakeTotal.set_translation(shakeLocationBS * applyShake);
	shakeTotal.set_orientation((shakeRotationBS * applyShake).to_quat());

	shakeTotal = Transform::lerp(get_active(), Transform::identity, shakeTotal);

	// apply!
	Meshes::Pose * poseLS = _context.access_pose_LS();
	int const boneIdx = bone.get_index();
	Transform boneLS = poseLS->get_bone(boneIdx);
	boneLS = boneLS.to_world(shakeTotal);

	// write back
	poseLS->access_bones()[boneIdx] = boneLS;
}

void Shake::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
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

REGISTER_FOR_FAST_CAST(ShakeData);

AppearanceControllerData* ShakeData::create_data()
{
	return new ShakeData();
}

void ShakeData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(shake), create_data);
}

ShakeData::ShakeData()
{
}

bool ShakeData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= shakeLength.load_from_xml(_node, TXT("shakeLength"));
	result &= shakeLocationBS.load_from_xml_child_node(_node, TXT("shakeLocationBS"));
	result &= shakeRotationBS.load_from_xml_child_node(_node, TXT("shakeRotationBS"));

	return result;
}

bool ShakeData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* ShakeData::create_copy() const
{
	ShakeData* copy = new ShakeData();
	*copy = *this;
	return copy;
}

AppearanceController* ShakeData::create_controller()
{
	return new Shake(this);
}

void ShakeData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	shakeLength.fill_value_with(_context);
	shakeLocationBS.fill_value_with(_context);
	shakeRotationBS.fill_value_with(_context);
}

void ShakeData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);

	// this should get mirrors
	Vector3 resZAxis = Vector3::cross(_transform.get_x_axis(), _transform.get_y_axis());
	if (Vector3::dot(resZAxis, _transform.get_z_axis()) < 0.0f)
	{
		shakeLocationBS.if_value_set([this]() { shakeLocationBS.access() = -shakeLocationBS.get(); });
		shakeRotationBS.if_value_set([this]() { shakeRotationBS.access() = -shakeRotationBS.get(); });
	}
}

void ShakeData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
}
