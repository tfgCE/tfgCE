#include "ac_centipedeBackPlate.h"

#include "..\..\modules\moduleMovementCentipede.h"

#include "..\..\..\framework\appearance\appearanceControllerPoseContext.h"
#include "..\..\..\framework\appearance\appearanceControllers.h"
#include "..\..\..\framework\appearance\skeleton.h"
#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleSound.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(centipedeBackPlate);

//

REGISTER_FOR_FAST_CAST(CentipedeBackPlate);

CentipedeBackPlate::CentipedeBackPlate(CentipedeBackPlateData const * _data)
: base(_data)
, centipedeBackPlateData(_data)
{
	bone = centipedeBackPlateData->bone.get(Name::invalid());
	towardsSocket = centipedeBackPlateData->towardsSocket.get(Name::invalid());
}

CentipedeBackPlate::~CentipedeBackPlate()
{
}

void CentipedeBackPlate::initialise(Framework::ModuleAppearance* _owner)
{
	base::initialise(_owner);

	if (auto* skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		bone.look_up(skeleton);
	}
	if (auto* mesh = get_owner()->get_mesh())
	{
		towardsSocket.look_up(mesh);
	}

	//auto& varStorage = get_owner()->access_controllers_variable_storage();
}

void CentipedeBackPlate::advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void CentipedeBackPlate::calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(centipedeBackPlate_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!get_active() ||
		!bone.is_valid())
	{
		return;
	}

	auto* appearance = get_owner();
	auto* imo = appearance->get_owner();
	auto* presence = imo->get_presence();

	if (auto* cm = fast_cast<ModuleMovementCentipede>(imo->get_movement()))
	{
		if (auto* tail = cm->get_tail())
		{
			if (auto* a = tail->get_appearance())
			{
				if (towardsIMO != tail)
				{
					towardsIMO = tail;
					towardsSocket.look_up(a->get_mesh());
				}
				if (towardsSocket.is_valid())
				{
					Vector3 targetOOS = a->calculate_socket_os(towardsSocket.get_index()).get_translation(); // should be always the same as main body bone doesn't move much
					Vector3 targetWS = tail->get_presence()->get_placement().location_to_world(targetOOS);
					targetOS = presence->get_placement().location_to_local(targetWS);
				}
			}
		}
	}

	Meshes::Pose* poseLS = _context.access_pose_LS();

	int boneIdx = bone.get_index();
	Transform boneMS = poseLS->get_bone_ms_from_ls(boneIdx);
	if (boneMS.get_scale() > 0.0f)
	{
		Vector3 targetMS = appearance->get_ms_to_os_transform().location_to_world(targetOS);
		Vector3 targetBS = boneMS.location_to_local(targetMS);

		Rotator3 targetDirBS = targetBS.to_rotator();

		targetDirBS = targetDirBS.normal_axes();

		targetDirBS.roll = 0.0f;
		targetDirBS.yaw = clamp(targetDirBS.yaw, -90.0f, 90.0f);
		targetDirBS.pitch = clamp(targetDirBS.pitch, -10.0f, 70.0f);

		Transform boneLS = poseLS->get_bone(boneIdx);
		boneLS = boneLS.to_world(Transform(Vector3::zero, targetDirBS.to_quat()));

		poseLS->set_bone(boneIdx, boneLS);
	}
}

void CentipedeBackPlate::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	providesBones.push_back(bone.get_index());
}

//

REGISTER_FOR_FAST_CAST(CentipedeBackPlateData);

Framework::AppearanceControllerData* CentipedeBackPlateData::create_data()
{
	return new CentipedeBackPlateData();
}

void CentipedeBackPlateData::register_itself()
{
	Framework::AppearanceControllers::register_appearance_controller(NAME(centipedeBackPlate), create_data);
}

CentipedeBackPlateData::CentipedeBackPlateData()
{
}

bool CentipedeBackPlateData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= towardsSocket.load_from_xml(_node, TXT("towardsSocket"));

	return result;
}

bool CentipedeBackPlateData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::AppearanceControllerData* CentipedeBackPlateData::create_copy() const
{
	CentipedeBackPlateData* copy = new CentipedeBackPlateData();
	*copy = *this;
	return copy;
}

Framework::AppearanceController* CentipedeBackPlateData::create_controller()
{
	return new CentipedeBackPlate(this);
}

void CentipedeBackPlateData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
}

void CentipedeBackPlateData::apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	towardsSocket.fill_value_with(_context);
}

