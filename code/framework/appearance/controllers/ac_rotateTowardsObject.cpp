#include "ac_rotateTowardsObject.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"
#include "..\..\module\modulePresence.h"
#include "..\..\object\object.h"
#include "..\..\world\room.h"

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

DEFINE_STATIC_NAME(rotateTowardsObject);

//

REGISTER_FOR_FAST_CAST(RotateTowardsObject);

RotateTowardsObject::RotateTowardsObject(RotateTowardsObjectData const * _data)
: base(_data)
, rotateTowardsObjectData(_data)
{
	bone = rotateTowardsObjectData->bone.get();
	if (rotateTowardsObjectData->toObjectTagged.is_set()) toObjectTagged = rotateTowardsObjectData->toObjectTagged.get();
	if (rotateTowardsObjectData->toBone.is_set()) toBone = rotateTowardsObjectData->toBone.get();
	if (rotateTowardsObjectData->toOffset.is_set()) toOffset = rotateTowardsObjectData->toOffset.get();
	if (rotateTowardsObjectData->toDir.is_set()) toDir = rotateTowardsObjectData->toDir.get();
	if (rotateTowardsObjectData->toSocket.is_set()) toSocket = rotateTowardsObjectData->toSocket.get();
}

RotateTowardsObject::~RotateTowardsObject()
{
}

void RotateTowardsObject::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	bone.look_up(get_owner()->get_skeleton()->get_skeleton());
}

void RotateTowardsObject::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void RotateTowardsObject::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(rotateTowardsObject_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid())
	{
		return;
	}

	if (!toObject.is_set() &&
		!toObjectTagged.is_empty())
	{
		if (auto* r = get_owner()->get_owner()->get_presence()->get_in_room())
		{
			// we're called in advance_physically
			FOR_EVERY_OBJECT_IN_ROOM(o, r)
			{
				if (toObjectTagged.check(o->get_tags()))
				{
					toObject = o;
					if (o->get_appearance()->get_skeleton())
					{
						toBone.look_up(o->get_appearance()->get_skeleton()->get_skeleton());
					}
					toSocket.look_up(o->get_appearance()->get_mesh());
					break;
				}
			}
		}
	}

	auto* too = toObject.get();
	if (! too)
	{
		return;
	}
	if (too->get_presence()->get_in_room() != get_owner()->get_owner()->get_presence()->get_in_room())
	{
		warn(TXT("rotateTowardsObject supported only within room!"));
	}

	Meshes::Pose* poseLS = _context.access_pose_LS();

	int const boneIdx = bone.get_index();

	Transform matchToPlacementWS = too->get_presence()->get_placement();
	if (toBone.is_valid())
	{
		if (auto* fp = too->get_appearance()->get_final_pose_MS())
		{
			matchToPlacementWS = matchToPlacementWS.to_world(too->get_appearance()->get_ms_to_os_transform().to_world(fp->get_bone(toBone.get_index())));
		}
	}
	matchToPlacementWS = matchToPlacementWS.to_world(Transform(toOffset, Quat::identity));

	Transform matchToPlacementMS = get_owner()->get_ms_to_ws_transform().to_local(matchToPlacementWS);

	Transform boneMS = poseLS->get_bone_ms_from_ls(boneIdx);
	Transform targetBS = boneMS.to_local(matchToPlacementMS);

	Vector3 dirVectorBS = targetBS.get_translation().normal();
	if (!toDir.is_almost_zero())
	{
		dirVectorBS = boneMS.vector_to_local(get_owner()->get_ms_to_os_transform().vector_to_local(toDir));
	}
	Transform requiredDiff = Transform::identity;
	requiredDiff.build_from_two_axes(rotateTowardsObjectData->dirAxis, rotateTowardsObjectData->keepAxis, dirVectorBS, Vector3::axis(rotateTowardsObjectData->keepAxis), Vector3::zero);
	
	Transform applyDiff = Transform::lerp(get_active(), Transform::identity, requiredDiff);

	Transform boneLS = poseLS->get_bone(boneIdx);
	boneLS = boneLS.to_world(applyDiff);

	poseLS->access_bones()[boneIdx] = boneLS;
}

void RotateTowardsObject::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
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

REGISTER_FOR_FAST_CAST(RotateTowardsObjectData);

AppearanceControllerData* RotateTowardsObjectData::create_data()
{
	return new RotateTowardsObjectData();
}

void RotateTowardsObjectData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(rotateTowardsObject), create_data);
}

RotateTowardsObjectData::RotateTowardsObjectData()
{
}

bool RotateTowardsObjectData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	dirAxis = Axis::parse_from(_node->get_string_attribute(TXT("dirAxis")), dirAxis);
	keepAxis = Axis::parse_from(_node->get_string_attribute(TXT("keepAxis")), keepAxis);
	result &= toObjectTagged.load_from_xml(_node, TXT("toObjectTagged"));
	result &= toBone.load_from_xml(_node, TXT("toBone"));
	result &= toOffset.load_from_xml(_node, TXT("toOffset"));
	result &= toDir.load_from_xml(_node, TXT("toDir"));
	result &= toSocket.load_from_xml(_node, TXT("toSocket"));

	if (!bone.is_set())
	{
		error_loading_xml(_node, TXT("no bone provided"));
		result = false;
	}
	
	return result;
}

bool RotateTowardsObjectData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* RotateTowardsObjectData::create_copy() const
{
	RotateTowardsObjectData* copy = new RotateTowardsObjectData();
	*copy = *this;
	return copy;
}

AppearanceController* RotateTowardsObjectData::create_controller()
{
	return new RotateTowardsObject(this);
}

void RotateTowardsObjectData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
}

void RotateTowardsObjectData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	toObjectTagged.fill_value_with(_context);
	toBone.fill_value_with(_context);
	toOffset.fill_value_with(_context);
	toDir.fill_value_with(_context);
	toSocket.fill_value_with(_context);
}

