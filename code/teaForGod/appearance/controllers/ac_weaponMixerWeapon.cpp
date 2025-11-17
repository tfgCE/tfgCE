#include "ac_weaponMixerWeapon.h"

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

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AppearanceControllersLib;

//

#ifdef AN_DEVELOPMENT_OR_PROFILER
#define DEBUG_DRAW_GRAB
#endif

//

DEFINE_STATIC_NAME(weaponMixerWeapon);

DEFINE_STATIC_NAME(swappingParts);

//

REGISTER_FOR_FAST_CAST(WeaponMixerWeapon);

WeaponMixerWeapon::WeaponMixerWeapon(WeaponMixerWeaponData const * _data)
: base(_data)
, weaponMixerWeaponData(_data)
{
	bone = weaponMixerWeaponData->bone.get();
}

WeaponMixerWeapon::~WeaponMixerWeapon()
{
}

void WeaponMixerWeapon::initialise(Framework::ModuleAppearance* _owner)
{
	base::initialise(_owner);

	if (auto* skel = get_owner()->get_skeleton()->get_skeleton())
	{
		bone.look_up(skel);
	}
}

void WeaponMixerWeapon::advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void WeaponMixerWeapon::calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(weaponMixerWeapon_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid())
	{
		return;
	}

	float deltaTime = _context.get_delta_time();

	bool swappingParts = get_owner()->get_owner()->get_variables().get_value<bool>(NAME(swappingParts), false);

	if (swappingParts)
	{
		randomImpulseTimeLeft = 0.0f;
		swappingPartsChangeTimeLeft -= deltaTime;
		if (swappingPartsChangeTimeLeft <= 0.0f)
		{
			swappingPartsChangeTimeLeft = Random::get(weaponMixerWeaponData->swappingPartsChangeTime);
			offset.set_translation(Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f)).normal() * weaponMixerWeaponData->maxOffLinear * 0.5f);
			offset.set_orientation((Rotator3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f)).normal() * weaponMixerWeaponData->maxOffRotation * 0.5f).to_quat());

			velocityLinear = -offset.get_translation() * 0.3f;
			velocityRotation = -offset.get_orientation().to_rotator() * 0.3f;
		}
	}
	else
	{
		swappingPartsChangeTimeLeft = 0.0f;
		randomImpulseTimeLeft -= deltaTime;

		if (randomImpulseTimeLeft <= 0.0f)
		{
			randomImpulseTimeLeft = Random::get(weaponMixerWeaponData->randomImpulseTime);
			impulseLinear = (-offset.get_translation() + Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f)).normal() * weaponMixerWeaponData->maxOffLinear) * weaponMixerWeaponData->randomImpulsePower;
			impulseRotation = (-offset.get_orientation().to_rotator() + Rotator3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f)).normal() * weaponMixerWeaponData->maxOffRotation) * weaponMixerWeaponData->randomImpulsePower;
		}

		if (offset.get_translation().length() > weaponMixerWeaponData->maxOffLinear)
		{
			impulseLinear = (-offset.get_translation() + Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f)).normal() * weaponMixerWeaponData->maxOffLinear) * weaponMixerWeaponData->backImpulsePower;
		}

		if (offset.get_orientation().to_rotator().length() > weaponMixerWeaponData->maxOffRotation)
		{
			impulseRotation = (-offset.get_orientation().to_rotator() + Rotator3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f)).normal() * weaponMixerWeaponData->maxOffRotation) * weaponMixerWeaponData->backImpulsePower;
		}

		velocityLinear += impulseLinear * weaponMixerWeaponData->applyImpulsePower * deltaTime;
		velocityRotation += impulseRotation * weaponMixerWeaponData->applyImpulsePower * deltaTime;

		velocityLinear = velocityLinear.normal() * max(0.0f, velocityLinear.length() - weaponMixerWeaponData->stopAccelerationLinear * deltaTime);
		velocityRotation = velocityRotation.normal() * max(0.0f, velocityRotation.length() - weaponMixerWeaponData->stopAccelerationRotation * deltaTime);

		if (velocityLinear.length() > weaponMixerWeaponData->maxVelocityLinear)
		{
			velocityLinear = velocityLinear.normal() * weaponMixerWeaponData->maxVelocityLinear;
		}

		if (velocityRotation.length() > weaponMixerWeaponData->maxVelocityRotation)
		{
			velocityRotation = velocityRotation.normal() * weaponMixerWeaponData->maxVelocityRotation;
		}

		Transform change(velocityLinear * deltaTime, (velocityRotation * deltaTime).to_quat());
		offset = offset.to_world(change);
	}

	int const boneIdx = bone.get_index();
	Meshes::Pose* poseLS = _context.access_pose_LS();
	Transform boneLS = poseLS->get_bone(boneIdx);
	poseLS->access_bones()[boneIdx] = boneLS.to_world(offset);
}

void WeaponMixerWeapon::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
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

REGISTER_FOR_FAST_CAST(WeaponMixerWeaponData);

Framework::AppearanceControllerData* WeaponMixerWeaponData::create_data()
{
	return new WeaponMixerWeaponData();
}

void WeaponMixerWeaponData::register_itself()
{
	Framework::AppearanceControllers::register_appearance_controller(NAME(weaponMixerWeapon), create_data);
}

WeaponMixerWeaponData::WeaponMixerWeaponData()
{
}

bool WeaponMixerWeaponData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));

	swappingPartsChangeTime.load_from_xml(_node, TXT("swappingPartsChangeTime"));
	randomImpulseTime.load_from_xml(_node, TXT("randomImpulseTime"));

	backImpulsePower = _node->get_float_attribute_or_from_child(TXT("backImpulsePower"), backImpulsePower);
	randomImpulsePower = _node->get_float_attribute_or_from_child(TXT("randomImpulsePower"), randomImpulsePower);

	applyImpulsePower = _node->get_float_attribute_or_from_child(TXT("applyImpulsePower"), applyImpulsePower);

	maxVelocityLinear = _node->get_float_attribute_or_from_child(TXT("maxVelocityLinear"), maxVelocityLinear);
	maxVelocityRotation = _node->get_float_attribute_or_from_child(TXT("maxVelocityRotation"), maxVelocityRotation);

	stopAccelerationLinear = _node->get_float_attribute_or_from_child(TXT("stopAccelerationLinear"), stopAccelerationLinear);
	stopAccelerationRotation = _node->get_float_attribute_or_from_child(TXT("stopAccelerationRotation"), stopAccelerationRotation);

	maxOffLinear = _node->get_float_attribute_or_from_child(TXT("maxOffLinear"), maxOffLinear);
	maxOffRotation = _node->get_float_attribute_or_from_child(TXT("maxOffRotation"), maxOffRotation);

	return result;
}

bool WeaponMixerWeaponData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::AppearanceControllerData* WeaponMixerWeaponData::create_copy() const
{
	WeaponMixerWeaponData* copy = new WeaponMixerWeaponData();
	*copy = *this;
	return copy;
}

Framework::AppearanceController* WeaponMixerWeaponData::create_controller()
{
	return new WeaponMixerWeapon(this);
}

void WeaponMixerWeaponData::apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
}

void WeaponMixerWeaponData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

void WeaponMixerWeaponData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
}
