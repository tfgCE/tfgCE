#include "ac_continuousRotation.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenValueDefImpl.inl"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"
#include "..\..\module\moduleSound.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\performance\performanceUtils.h"

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(continuousRotation);

//

REGISTER_FOR_FAST_CAST(ContinuousRotation);

ContinuousRotation::ContinuousRotation(ContinuousRotationData const * _data)
: base(_data)
, continuousRotationData(_data)
{
	bone = continuousRotationData->bone.get();
	if (continuousRotationData->rotationSpeedVariable.is_set())
	{
		rotationSpeedVariable = continuousRotationData->rotationSpeedVariable.get();
	}

	if (continuousRotationData->sound.is_set())
	{
		sound = continuousRotationData->sound.get();
	}
}

ContinuousRotation::~ContinuousRotation()
{
}

void ContinuousRotation::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	bone.look_up(get_owner()->get_skeleton()->get_skeleton());

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	rotationSpeedVariable.look_up<Rotator3>(varStorage);

	rotationSpeed = Rotator3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f));

	if (!continuousRotationData->rotationSpeed.get().is_zero())
	{
		rotationSpeed *= continuousRotationData->rotationSpeed.get();
	}
	if (!continuousRotationData->speed.get().is_empty() &&
		!continuousRotationData->speed.get().is_zero())
	{
		rotationSpeed = rotationSpeed.normal() * Random::get(continuousRotationData->speed.get());
	}

	{
		Range op = continuousRotationData->oscilationPeriod.get();
		oscilationPeriod.pitch = Random::get(op);
		oscilationPeriod.yaw = Random::get(op);
		oscilationPeriod.roll = Random::get(op);
		oscilationAt = Rotator3::zero;
	}
}

void ContinuousRotation::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void ContinuousRotation::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(rotate_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid())
	{
		return;
	}

	if (rotationSpeedVariable.is_valid())
	{
		rotationSpeed = rotationSpeedVariable.get<Rotator3>();
	}

	if (!oscilationPeriod.is_zero())
	{
		for_count(int, rcIdx, 3)
		{
			RotatorComponent::Type rc = (RotatorComponent::Type)rcIdx;
			if (oscilationPeriod.get_component(rc) != 0.0f)
			{
				auto& oa = oscilationAt.access_component(rc);
				oa += 360.0f * _context.get_delta_time() / oscilationPeriod.get_component(rc);
				oa = mod(oa, 360.0f);
			}
		}
	}

	if (rotationSpeed.is_zero())
	{
		if (playedSound.is_set())
		{
			playedSound->stop();
			playedSound.clear();
		}
	}
	else if (sound.is_valid())
	{
		if (! playedSound.is_set())
		{
			if (auto* ms = get_owner()->get_owner()->get_sound())
			{
				playedSound = ms->play_sound(sound);
			}
		}
	}

	if (get_active() > 0.0f)
	{
		Rotator3 rotateBy = rotationSpeed * _context.get_delta_time() * get_active();
		if (!oscilationPeriod.is_zero())
		{
			rotateBy.pitch *= 1.0f + 0.3f * sin_deg(oscilationAt.pitch);
			rotateBy.yaw *= 1.0f + 0.3f * sin_deg(oscilationAt.yaw);
			rotateBy.roll *= 1.0f + 0.3f * sin_deg(oscilationAt.roll);
		}
		currentRotation = currentRotation.to_world(rotateBy.to_quat());
	}

	Meshes::Pose * poseLS = _context.access_pose_LS();
	int const boneIdx = bone.get_index();
	auto & boneLS = poseLS->access_bones()[boneIdx];
	boneLS.set_orientation(boneLS.get_orientation().to_world(currentRotation));
}

void ContinuousRotation::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (bone.is_valid())
	{
		dependsOnParentBones.push_back(bone.get_index());
		providesBones.push_back(bone.get_index());
	}
	if (rotationSpeedVariable.is_valid())
	{
		dependsOnVariables.push_back(rotationSpeedVariable.get_name());
	}
}

//

REGISTER_FOR_FAST_CAST(ContinuousRotationData);

AppearanceControllerData* ContinuousRotationData::create_data()
{
	return new ContinuousRotationData();
}

void ContinuousRotationData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(continuousRotation), create_data);
}

ContinuousRotationData::ContinuousRotationData()
{
}

bool ContinuousRotationData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= rotationSpeedVariable.load_from_xml(_node, TXT("rotationSpeedVarID"));

	result &= speed.load_from_xml_child_node(_node, TXT("speed"));
	result &= rotationSpeed.load_from_xml_child_node(_node, TXT("rotationSpeed"));
	
	result &= oscilationPeriod.load_from_xml_child_node(_node, TXT("oscilationPeriod"));

	result &= sound.load_from_xml_child_node(_node, TXT("sound"));

	if (!bone.is_set())
	{
		error_loading_xml(_node, TXT("no bone provided"));
		result = false;
	}

	return result;
}

bool ContinuousRotationData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* ContinuousRotationData::create_copy() const
{
	ContinuousRotationData* copy = new ContinuousRotationData();
	*copy = *this;
	return copy;
}

AppearanceController* ContinuousRotationData::create_controller()
{
	return new ContinuousRotation(this);
}

void ContinuousRotationData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
	sound.if_value_set([this, _rename]() { sound = _rename(sound.get(), NP); });
}

void ContinuousRotationData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	speed.fill_value_with(_context);
	rotationSpeed.fill_value_with(_context);
	rotationSpeedVariable.fill_value_with(_context);
	oscilationPeriod.fill_value_with(_context);
	sound.fill_value_with(_context);
}

