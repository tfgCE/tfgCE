#include "ac_matchPlacementObject.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\meshGen\meshGenValueDefImpl.inl"
#include "..\..\module\moduleAppearance.h"
#include "..\..\module\modulePresence.h"
#include "..\..\object\object.h"
#include "..\..\world\room.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\performance\performanceUtils.h"

#include "..\..\..\core\debug\debugRenderer.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(matchPlacementObject);

//

REGISTER_FOR_FAST_CAST(MatchPlacementObject);

MatchPlacementObject::MatchPlacementObject(MatchPlacementObjectData const * _data)
: base(_data)
, matchPlacementObjectData(_data)
{
	bone = matchPlacementObjectData->bone.get();
	if (matchPlacementObjectData->alongBoneAxis.is_set()) alongBoneAxis = matchPlacementObjectData->alongBoneAxis.get().normal();
	if (matchPlacementObjectData->onBonePlane.is_set()) onBonePlane = matchPlacementObjectData->onBonePlane.get();
	if (matchPlacementObjectData->toInitialPlacement.is_set()) toInitialPlacement = matchPlacementObjectData->toInitialPlacement.get();
	if (matchPlacementObjectData->toObjectTagged.is_set()) toObjectTagged = matchPlacementObjectData->toObjectTagged.get();
	if (matchPlacementObjectData->toBone.is_set()) toBone = matchPlacementObjectData->toBone.get();
	if (matchPlacementObjectData->toOffset.is_set()) toOffset = matchPlacementObjectData->toOffset.get();
	if (matchPlacementObjectData->toSocket.is_set()) toSocket = matchPlacementObjectData->toSocket.get();
	if (matchPlacementObjectData->applyRotation.is_set()) applyRotation = matchPlacementObjectData->applyRotation.get();
}

MatchPlacementObject::~MatchPlacementObject()
{
}

void MatchPlacementObject::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	bone.look_up(get_owner()->get_skeleton()->get_skeleton());
}

void MatchPlacementObject::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void MatchPlacementObject::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(matchPlacementObject_finalPose);
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
	if (! too && ! toInitialPlacement)
	{
		return;
	}

	if (too && too->get_presence()->get_in_room() != get_owner()->get_owner()->get_presence()->get_in_room())
	{
		warn(TXT("matchPlacementObject supported only within room!"));
	}

	Meshes::Pose* poseLS = _context.access_pose_LS();

	int const boneIdx = bone.get_index();

	if (toInitialPlacement && (has_just_activated() || !initialPlacementWS.is_set()))
	{
		Transform initialPlacementMS = poseLS->get_bone_ms_from_ls(boneIdx);
		initialPlacementWS = get_owner()->get_ms_to_ws_transform().to_world(initialPlacementMS);
	}

	Transform matchToPlacementWS = Transform::identity;
	if (too)
	{
		matchToPlacementWS = too->get_presence()->get_placement();
		if (toSocket.is_valid())
		{
			matchToPlacementWS = matchToPlacementWS.to_world(too->get_appearance()->calculate_socket_os(toSocket.get_index()));
		}
		if (toBone.is_valid())
		{
			if (auto* fp = too->get_appearance()->get_final_pose_MS())
			{
				matchToPlacementWS = matchToPlacementWS.to_world(too->get_appearance()->get_ms_to_os_transform().to_world(fp->get_bone(toBone.get_index())));
			}
		}
	}
	else if (toInitialPlacement && initialPlacementWS.is_set())
	{
		matchToPlacementWS = initialPlacementWS.get();
	}
	else
	{
		return;
	}

	matchToPlacementWS = matchToPlacementWS.to_world(Transform(toOffset, Quat::identity));

	Transform matchToPlacementMS = get_owner()->get_ms_to_ws_transform().to_local(matchToPlacementWS);

	Transform boneMS = poseLS->get_bone_ms_from_ls(boneIdx);
	Transform requiredDiff = boneMS.to_local(matchToPlacementMS);

	requiredDiff = Transform::lerp(get_active(), Transform::identity, requiredDiff);

	Transform applyDiff = Transform::identity;
	if (alongBoneAxis.is_set())
	{
		applyDiff.set_translation(requiredDiff.get_translation().along_normalised(alongBoneAxis.get()));
	}
	else if (onBonePlane.is_set())
	{
		applyDiff.set_translation(requiredDiff.get_translation().drop_using_normalised(onBonePlane.get().get_normal()));
	}
	else
	{
		applyDiff.set_translation(requiredDiff.get_translation());
	}

	applyDiff.set_orientation(Quat::slerp(applyRotation, Quat::identity, requiredDiff.get_orientation()));

	/*
	debug_context(get_owner()->get_owner()->get_presence()->get_in_room());
	debug_draw_arrow(true, Colour::green,
		get_owner()->get_owner()->get_appearance()->get_ms_to_ws_transform().to_world(boneMS).get_translation(),
		get_owner()->get_owner()->get_appearance()->get_ms_to_ws_transform().to_world(boneMS.to_world(applyDiff)).get_translation());
	debug_no_context();
	*/

	Transform boneLS = poseLS->get_bone(boneIdx);
	boneLS = boneLS.to_world(applyDiff);

	poseLS->access_bones()[boneIdx] = boneLS;
}

void MatchPlacementObject::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
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

REGISTER_FOR_FAST_CAST(MatchPlacementObjectData);

AppearanceControllerData* MatchPlacementObjectData::create_data()
{
	return new MatchPlacementObjectData();
}

void MatchPlacementObjectData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(matchPlacementObject), create_data);
}

MatchPlacementObjectData::MatchPlacementObjectData()
{
}

bool MatchPlacementObjectData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= alongBoneAxis.load_from_xml(_node, TXT("alongBoneAxis"));
	result &= onBonePlane.load_from_xml(_node, TXT("onBonePlane"));
	result &= toInitialPlacement.load_from_xml(_node, TXT("toInitialPlacement"));
	result &= toObjectTagged.load_from_xml(_node, TXT("toObjectTagged"));
	result &= toBone.load_from_xml(_node, TXT("toBone"));
	result &= toOffset.load_from_xml(_node, TXT("toOffset"));
	result &= toSocket.load_from_xml(_node, TXT("toSocket"));
	result &= applyRotation.load_from_xml(_node, TXT("applyRotation"));

	if (!bone.is_set())
	{
		error_loading_xml(_node, TXT("no bone provided"));
		result = false;
	}
	
	return result;
}

bool MatchPlacementObjectData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* MatchPlacementObjectData::create_copy() const
{
	MatchPlacementObjectData* copy = new MatchPlacementObjectData();
	*copy = *this;
	return copy;
}

AppearanceController* MatchPlacementObjectData::create_controller()
{
	return new MatchPlacementObject(this);
}

void MatchPlacementObjectData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
}

void MatchPlacementObjectData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	alongBoneAxis.fill_value_with(_context);
	onBonePlane.fill_value_with(_context);
	toInitialPlacement.fill_value_with(_context);
	toObjectTagged.fill_value_with(_context);
	toBone.fill_value_with(_context);
	toOffset.fill_value_with(_context);
	toSocket.fill_value_with(_context);
	applyRotation.fill_value_with(_context);
}

