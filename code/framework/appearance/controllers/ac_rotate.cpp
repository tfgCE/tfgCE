#include "ac_rotate.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\performance\performanceUtils.h"

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(rotate);

//

REGISTER_FOR_FAST_CAST(Rotate);

Rotate::Rotate(RotateData const * _data)
	: base(_data)
	, rotateData(_data)
{
	relativeToDefault = rotateData->relativeToDefault;
	ignoreOwnerRotation = rotateData->ignoreOwnerRotation;
	bone = rotateData->bone.get();
	speed = rotateData->speed.get();
	if (rotateData->rotationDir.is_set())
	{
		rotationDir = rotateData->rotationDir.get();
	}
	if (rotateData->angleLimit.is_set())
	{
		angleLimit = rotateData->angleLimit.get();
	}
	rotatorVariable = rotateData->rotatorVariable;
	quatVariable = rotateData->quatVariable;
}

Rotate::~Rotate()
{
}

void Rotate::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	bone.look_up(get_owner()->get_skeleton()->get_skeleton());

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	rotatorVariable.look_up<Rotator3>(varStorage);
	quatVariable.look_up<Quat>(varStorage);
}

void Rotate::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void Rotate::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(rotate_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid() ||
		(!rotatorVariable.is_valid() &&
			!quatVariable.is_valid() &&
			!rotationDir.is_set()))
	{
		return;
	}

	float active = get_active();

	if (rotationDir.is_set())
	{
		Rotator3 rotationDirNormalised = rotationDir.get().normal();
		if (ignoreOwnerRotation && active > 0.0f)
		{
			Rotator3 rotatedRecentlyNeg = get_owner()->get_owner()->get_presence()->get_prev_placement_offset().get_orientation().to_rotator();
			rotatedRecentlyNeg *= rotationDirNormalised;
			rotatedRecentlyNeg *= active;
			currentRotation = currentRotation.to_world(rotatedRecentlyNeg.to_quat());
		}
		currentRotation = currentRotation.to_world((rotationDirNormalised * (_context.get_delta_time() * speed * active)).to_quat());

		Meshes::Pose* poseLS = _context.access_pose_LS();
		int const boneIdx = bone.get_index();
		auto& boneLS = poseLS->access_bones()[boneIdx];

		boneLS.set_orientation(boneLS.get_orientation().to_world(currentRotation));
	}
	else
	{
		Quat targetRotation = currentRotation;
		if (rotatorVariable.is_valid())
		{
			targetRotation = rotatorVariable.get<Rotator3>().to_quat();
		}
		if (quatVariable.is_valid())
		{
			targetRotation = quatVariable.get<Quat>();
		}

		if (speed <= 0.0f || has_just_activated())
		{
			currentRotation = targetRotation;
		}
		else
		{
			Quat const diffQuat = currentRotation.to_local(targetRotation);
			Rotator3 const diffRot = diffQuat.to_rotator();
			float const distance = diffRot.length();
			if (distance > 0.0f)
			{
				float distanceCovered = _context.get_delta_time() * speed;
				distanceCovered = min(distance, distanceCovered);
				Quat const applyQuat = Quat::slerp(clamp(distanceCovered / distance, 0.0f, 1.0f), Quat::identity, diffQuat);
				currentRotation = currentRotation.to_world(applyQuat);
			}
		}

		if (angleLimit.is_set())
		{
			Rotator3 cr = currentRotation.to_rotator();
			float angle = sqrt(sqr(cr.yaw) + sqr(cr.pitch) + sqr(cr.roll));
			if (angle > angleLimit.get())
			{
				currentRotation = Quat::slerp(angleLimit.get() / angle, Quat::identity, currentRotation);
			}
		}

		if (active > 0.0f)
		{
			Meshes::Pose* poseLS = _context.access_pose_LS();
			int const boneIdx = bone.get_index();
			auto& boneLS = poseLS->access_bones()[boneIdx];

			if (active < 1.0f)
			{
				if (relativeToDefault)
				{
					boneLS.set_orientation(Quat::slerp(get_active(), boneLS.get_orientation(), poseLS->get_skeleton()->get_default_pose_ls()->get_bone(boneIdx).to_world(currentRotation)));
				}
				else
				{
					boneLS.set_orientation(boneLS.get_orientation().to_world(Quat::slerp(active, Quat::identity, currentRotation)));
				}
			}
			else
			{
				if (relativeToDefault)
				{
					boneLS.set_orientation(poseLS->get_skeleton()->get_default_pose_ls()->get_bone(boneIdx).to_world(currentRotation));
				}
				else
				{
					boneLS.set_orientation(boneLS.get_orientation().to_world(currentRotation));
				}
			}
		}
	}
}

void Rotate::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (bone.is_valid())
	{
		dependsOnParentBones.push_back(bone.get_index());
		providesBones.push_back(bone.get_index());
	}
	if (rotatorVariable.is_valid())
	{
		dependsOnVariables.push_back(rotatorVariable.get_name());
	}
	if (quatVariable.is_valid())
	{
		dependsOnVariables.push_back(quatVariable.get_name());
	}
}

//

REGISTER_FOR_FAST_CAST(RotateData);

AppearanceControllerData* RotateData::create_data()
{
	return new RotateData();
}

void RotateData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(rotate), create_data);
}

RotateData::RotateData()
{
}

bool RotateData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	relativeToDefault = _node->get_bool_attribute_or_from_child_presence(TXT("relativeToDefault"), relativeToDefault);
	ignoreOwnerRotation = _node->get_bool_attribute_or_from_child_presence(TXT("ignoreOwnerRotation"), ignoreOwnerRotation);
	
	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= rotationDir.load_from_xml(_node, TXT("rotationDir"));
	result &= rotatorVariable.load_from_xml(_node, TXT("rotatorVarID"));
	result &= quatVariable.load_from_xml(_node, TXT("quatVarID"));

	result &= speed.load_from_xml_child_node(_node, TXT("speed"));
	result &= angleLimit.load_from_xml_child_node(_node, TXT("angleLimit"));
	
	if (!bone.is_set())
	{
		error_loading_xml(_node, TXT("no bone provided"));
		result = false;
	}
	if (!rotatorVariable.get_name().is_valid() &&
		!quatVariable.get_name().is_valid() &&
		!rotationDir.is_set())
	{
		error_loading_xml(_node, TXT("no rotator nor quat variable id, no rotationDir provided"));
		result = false;
	}

	return result;
}

bool RotateData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* RotateData::create_copy() const
{
	RotateData* copy = new RotateData();
	*copy = *this;
	return copy;
}

AppearanceController* RotateData::create_controller()
{
	return new Rotate(this);
}

void RotateData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
}

void RotateData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	rotationDir.fill_value_with(_context);
	speed.fill_value_with(_context);
	angleLimit.fill_value_with(_context);
}

