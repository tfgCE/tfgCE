#include "ac_head.h"

#include "ikUtils.h"

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

DEFINE_STATIC_NAME(head);

//

REGISTER_FOR_FAST_CAST(Head);

Head::Head(HeadData const * _data)
: base(_data)
, headData(_data)
, isValid(false)
{
	headBone = headData->headBone.get();
	eyesBone = headData->eyesBone.get();
	headVRViewOffset = headData->headVRViewOffset;
}

Head::~Head()
{
}

void Head::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	isValid = true;

	if (Meshes::Skeleton const * skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		headBone.look_up(skeleton);
		eyesBone.look_up(skeleton);

		if (headBone.is_valid())
		{
			neckBone = Meshes::BoneID(skeleton, skeleton->get_parent_of(headBone.get_index()));
		}
		else
		{
			error(TXT("could not find head bone \"%S\" for head appearance controller"), headBone.get_name().to_char());
			isValid = false;
		}
	}
	else
	{
		isValid = false;
	}
}

void Head::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void Head::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	if (! isValid)
	{
		return;
	}
	
#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(atHead_finalPose);
#endif

	Meshes::Pose * poseLS = _context.access_pose_LS();

	if (ModuleAppearance const * appearance = get_owner())
	{
		if (IModulesOwner const * owner = appearance->get_owner())
		{
			if (ModulePresence const * presence = owner->get_presence())
			{
				Transform const msToOs = appearance->get_ms_to_os_transform();
				Transform headBoneMS = poseLS->get_bone_ms_from_ls(headBone.get_index());
				Transform headBoneOS = msToOs.to_world(headBoneMS);

				// no lagging for head as it is filter for relative orientation!
				if (presence->is_vr_movement())
				{
					// for vr movement requested relative look is against floor
					headBoneOS = presence->get_requested_relative_look_for_vr();
					headBoneOS = headBoneOS.to_world(headVRViewOffset); // apply offset as head bone is in centre
					headBoneOS.set_translation(headBoneOS.get_translation() + Vector3(0.0f, 0.0f, presence->get_vertical_look_offset()));
					if (MainConfig::global().should_be_immobile_vr())
					{
						presence->make_head_bone_os_safe(REF_ headBoneOS);
					}
				}
				else
				{
					todo_note(TXT("use eyes bones if presence has look relative placement for eyes bones"));
					headBoneOS = headBoneOS.to_world(presence->get_requested_relative_look());
					headBoneOS.set_translation(headBoneOS.get_translation() + Vector3(0.0f, 0.0f, presence->get_vertical_look_offset()));
					presence->make_head_bone_os_safe(REF_ headBoneOS);
				}

				headBoneMS = msToOs.to_local(headBoneOS);

				Transform neckBoneMS = neckBone.is_valid() ? poseLS->get_bone_ms_from_ls(neckBone.get_index()) : Transform::identity;
				poseLS->set_bone(headBone.get_index(), neckBoneMS.to_local(headBoneMS));

#ifndef AN_CLANG
				an_assert(poseLS->is_ok());
#endif
			}
		}
	}
}

void Head::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (neckBone.is_valid())
	{
		dependsOnParentBones.push_back(neckBone.get_index());
		providesBones.push_back(neckBone.get_index());
	}
	if (headBone.is_valid())
	{
		providesBones.push_back(headBone.get_index());
	}
	if (eyesBone.is_valid())
	{
		providesBones.push_back(eyesBone.get_index());
	}
}

//

REGISTER_FOR_FAST_CAST(HeadData);

AppearanceControllerData* HeadData::create_data()
{
	return new HeadData();
}

void HeadData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(head), create_data);
}

HeadData::HeadData()
{
}

bool HeadData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	headBone.load_from_xml(_node, TXT("headBone"));
	eyesBone.load_from_xml(_node, TXT("eyesBone"));
	headVRViewOffset.load_from_xml_child_node(_node, TXT("headVRViewOffset"));

	return result;
}

bool HeadData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* HeadData::create_copy() const
{
	HeadData* copy = new HeadData();
	*copy = *this;
	return copy;
}

AppearanceController* HeadData::create_controller()
{
	return new Head(this);
}

void HeadData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	headBone.if_value_set([this, _rename](){ headBone = _rename(headBone.get(), NP); });
	eyesBone.if_value_set([this, _rename](){ eyesBone = _rename(eyesBone.get(), NP); });
}

void HeadData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	headBone.fill_value_with(_context);
	eyesBone.fill_value_with(_context);
}

