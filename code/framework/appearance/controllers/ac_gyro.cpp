#include "ac_gyro.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\meshGen\meshGenValueDefImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\performance\performanceUtils.h"

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(gyro);

//

REGISTER_FOR_FAST_CAST(Gyro);

Gyro::Gyro(GyroData const * _data)
: base(_data)
, gyroData(_data)
{
	bone = gyroData->bone.get();
	allowRotation = gyroData->allowRotation.get();
}

Gyro::~Gyro()
{
	stop_observing_presence();
}

void Gyro::on_presence_destroy(ModulePresence* _presence)
{
	an_assert(observingPresence == _presence);
	stop_observing_presence();
}

void Gyro::stop_observing_presence()
{
	if (observingPresence)
	{
		observingPresence->remove_presence_observer(this);
		observingPresence = nullptr;
	}
}

void Gyro::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	bone.look_up(get_owner()->get_skeleton()->get_skeleton());

	if (IModulesOwner* imo = get_owner()->get_owner())
	{
		if (!observingPresence)
		{
			imo->get_presence()->add_presence_observer(this);
			observingPresence = imo->get_presence();
		}
		else
		{
			an_assert(observingPresence == imo->get_presence());
		}
	}
}

void Gyro::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void Gyro::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(gyro_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid())
	{
		return;
	}

	//float active = get_active();

	Meshes::Pose * poseLS = _context.access_pose_LS();
	int const boneIdx = bone.get_index();
	auto boneMS = poseLS->get_bone_ms_from_ls(boneIdx);

	auto ms2ws = get_owner()->get_ms_to_ws_transform();
	auto boneWS = ms2ws.to_world(boneMS);

	debug_filter(ac_gyro);
	debug_context(get_owner()->get_owner()->get_presence()->get_in_room());
	debug_draw_transform_size(true, Transform(boneWS.get_translation(), currentRotation), 0.1f);

	Quat diffRequired = boneWS.get_orientation().to_local(currentRotation);
	Rotator3 diffRequiredRot = diffRequired.to_rotator();
	Rotator3 diffAllowedRot = diffRequiredRot * allowRotation;
	Quat diffAllowed = diffAllowedRot.to_quat();
	boneWS.set_orientation(boneWS.get_orientation().to_world(diffAllowed));
	currentRotation = boneWS.get_orientation();

	auto boneLS = poseLS->get_bone(boneIdx);
	boneLS.set_orientation(boneLS.get_orientation().to_world(diffAllowed));
	poseLS->set_bone(boneIdx, boneLS);

	debug_no_context();
	debug_no_filter();
}

void Gyro::on_presence_changed_room(ModulePresence* _presence, Room* _intoRoom, Transform const& _intoRoomTransform, DoorInRoom* _exitThrough, DoorInRoom* _enterThrough, DoorInRoomArrayPtr const* _throughDoors)
{
	currentRotation = _intoRoomTransform.to_local(currentRotation);
}

void Gyro::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
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

REGISTER_FOR_FAST_CAST(GyroData);

AppearanceControllerData* GyroData::create_data()
{
	return new GyroData();
}

void GyroData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(gyro), create_data);
}

GyroData::GyroData()
{
}

bool GyroData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= allowRotation.load_from_xml(_node, TXT("allowRotation"));

	if (!bone.is_set())
	{
		error_loading_xml(_node, TXT("no bone provided"));
		result = false;
	}

	return result;
}

bool GyroData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* GyroData::create_copy() const
{
	GyroData* copy = new GyroData();
	*copy = *this;
	return copy;
}

AppearanceController* GyroData::create_controller()
{
	return new Gyro(this);
}

void GyroData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
}

void GyroData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	allowRotation.fill_value_with(_context);
}

