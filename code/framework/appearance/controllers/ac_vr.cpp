#include "ac_vr.h"

#include "ikUtils.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"
#include "..\..\module\custom\mc_vrPlacementsProvider.h"

#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

#include "..\..\..\core\mesh\pose.h"
#include "..\..\..\core\wheresMyPoint\iWMPTool.h"

#include "..\..\..\core\performance\performanceUtils.h"
#include "..\..\..\core\system\input.h"
#include "..\..\..\core\vr\vrOffsets.h"

#include "..\..\..\core\debug\debugRenderer.h"

#ifdef AN_ALLOW_BULLSHOTS
#include "..\..\game\bullshotSystem.h"
#endif

#ifdef BUILD_NON_RELEASE
#include "..\..\..\framework\debug\previewGame.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define DEBUG_AC_VR

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(vr);

//

REGISTER_FOR_FAST_CAST(AppearanceControllersLib::VR);

AppearanceControllersLib::VR::VR(VRData const * _data)
: base(_data)
, vrData(_data)
, isValid(false)
{
	headBone = vrData->headBone.get();
	if (vrData->headBoneLocationFromSocket.is_set()) headBoneLocationFromSocket = vrData->headBoneLocationFromSocket.get();
	if (vrData->handLeftBone.is_set()) handLeftBone = vrData->handLeftBone.get();
	if (vrData->handRightBone.is_set()) handRightBone = vrData->handRightBone.get();
	headVRViewOffset = vrData->headVRViewOffset.get();
	handLeftNonVRPlayerOffset = vrData->handLeftNonVRPlayerOffset.get();
	handRightNonVRPlayerOffset = vrData->handRightNonVRPlayerOffset.get();
	if (vrData->relativeForwardDirYawOSVar.is_set()) relativeForwardDirYawOSVar = vrData->relativeForwardDirYawOSVar.get();
	if (vrData->relativeForwardDirRollOSVar.is_set()) relativeForwardDirRollOSVar = vrData->relativeForwardDirRollOSVar.get();
	if (vrData->offsetProvidedDir.is_set()) offsetProvidedDir = vrData->offsetProvidedDir.get();
	if (vrData->offsetProvidedDirForVR.is_set()) offsetProvidedDirForVR = vrData->offsetProvidedDirForVR.get();
	if (vrData->provideForwardDirMSVar.is_set()) provideForwardDirMSVar = vrData->provideForwardDirMSVar.get();
	if (vrData->provideTargetPointMSVar.is_set()) provideTargetPointMSVar = vrData->provideTargetPointMSVar.get();
}

AppearanceControllersLib::VR::~VR()
{
}

void AppearanceControllersLib::VR::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	isValid = true;

	auto& varStorage = get_owner()->access_controllers_variable_storage();
	relativeForwardDirYawOSVar.look_up<float>(varStorage);
	relativeForwardDirRollOSVar.look_up<float>(varStorage);
	provideForwardDirMSVar.look_up<Vector3>(varStorage);
	provideTargetPointMSVar.look_up<Vector3>(varStorage);

	headBoneLocationFromSocket.look_up(get_owner()->get_mesh());

	if (Meshes::Skeleton const * skeleton = get_owner()->get_skeleton() ? get_owner()->get_skeleton()->get_skeleton() : nullptr)
	{
		headBone.look_up(skeleton);
		handLeftBone.look_up(skeleton);
		handRightBone.look_up(skeleton);

		if (headBone.is_valid())
		{
			headParentBone = Meshes::BoneID(skeleton, skeleton->get_parent_of(headBone.get_index()));
		}
		else
		{
			error(TXT("could not find head bone \"%S\" for vr appearance controller"), headBone.get_name().to_char());
			isValid = false;
		}
		if (handLeftBone.is_valid())
		{
			handLeftParentBone = Meshes::BoneID(skeleton, skeleton->get_parent_of(handLeftBone.get_index()));
		}
		else if (handLeftBone.is_name_valid())
		{
			error(TXT("could not find hand left bone \"%S\" for vr appearance controller"), handLeftBone.get_name().to_char());
			isValid = false;
		}
		if (handRightBone.is_valid())
		{
			handRightParentBone = Meshes::BoneID(skeleton, skeleton->get_parent_of(handRightBone.get_index()));
		}
		else if (handRightBone.is_name_valid())
		{
			error(TXT("could not find hand right bone \"%S\" for vr appearance controller"), handRightBone.get_name().to_char());
			isValid = false;
		}
	}
	else
	{
		isValid = false;
	}
}

void AppearanceControllersLib::VR::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void AppearanceControllersLib::VR::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	if (! isValid)
	{
		return;
	}
	
#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(vr_finalPose);
#endif

	Meshes::Pose * poseLS = _context.access_pose_LS();

	if (ModuleAppearance const * appearance = get_owner())
	{
		if (IModulesOwner const * owner = appearance->get_owner())
		{
			if (ModulePresence const * presence = owner->get_presence())
			{
				bool isActive = get_active() > 0.5f;

				if (isActive)
				{
					activeForFrames = min(1024, activeForFrames + 1);
				}
				else
				{
					activeForFrames = 0;
				}

				Transform const msToOs = appearance->get_ms_to_os_transform();

				bool doOrientation = vrData->doOrientation;
				bool doTranslation = vrData->doTranslation && ! presence->do_eyes_have_fixed_location();

				int headBoneIdx = headBone.get_index();
				int handLeftBoneIdx = handLeftBone.is_valid()? handLeftBone.get_index() : NONE;
				int handRightBoneIdx = handRightBone.is_valid() ? handRightBone.get_index() : NONE;

				Transform headBoneMS = poseLS->get_bone_ms_from_ls(headBoneIdx);
				Transform handLeftBoneMS = handLeftBoneIdx != NONE? poseLS->get_bone_ms_from_ls(handLeftBoneIdx) : Transform::identity;
				Transform handRightBoneMS = handRightBoneIdx != NONE ? poseLS->get_bone_ms_from_ls(handRightBoneIdx) : Transform::identity;

				Transform headBoneOS = msToOs.to_world(headBoneMS);
				Transform handLeftBoneOS = msToOs.to_world(handLeftBoneMS);
				Transform handRightBoneOS = msToOs.to_world(handRightBoneMS);

				Transform orgHeadBoneOS = headBoneOS;
				Transform orgHandLeftBoneOS = handLeftBoneOS;
				Transform orgHandRightBoneOS = handRightBoneOS;

				if (isActive)
				{
					// no lagging for head as it is filter for relative orientation!
#ifdef BUILD_NON_RELEASE
					if (auto* pg = fast_cast<Framework::PreviewGame>(Framework::Game::get()))
					{
						auto& prvc = pg->get_preview_vr_controllers();
						if (prvc.inUse)
						{
							if (prvc.absolutePlacement)
							{
								headBoneOS = presence->get_placement().to_local(prvc.head);
								handLeftBoneOS = presence->get_placement().to_local(prvc.hands[Hand::Left]);
								handRightBoneOS = presence->get_placement().to_local(prvc.hands[Hand::Right]);
							}
							else
							{
								headBoneOS = prvc.head;
								handLeftBoneOS = prvc.hands[Hand::Left];
								handRightBoneOS = prvc.hands[Hand::Right];
							}
						}
					}
					else
#endif
					if (presence->is_vr_movement())
					{
						// for vr movement requested relative look is against floor
						headBoneOS = presence->get_requested_relative_look_for_vr();
						headBoneOS = headBoneOS.to_world(headVRViewOffset); // apply offset as head bone is in centre
						handLeftBoneOS = presence->get_requested_relative_hand_for_vr(0);
						handRightBoneOS = presence->get_requested_relative_hand_for_vr(1);
						headBoneOS.set_translation(headBoneOS.get_translation() + Vector3(0.0f, 0.0f, presence->get_vertical_look_offset()));
						if (MainConfig::global().should_be_immobile_vr())
						{
							presence->make_head_bone_os_safe(REF_ headBoneOS);
						}
						handLeftBoneOS.set_translation(handLeftBoneOS.get_translation() + Vector3(0.0f, 0.0f, presence->get_vertical_look_offset()));
						handRightBoneOS.set_translation(handRightBoneOS.get_translation() + Vector3(0.0f, 0.0f, presence->get_vertical_look_offset()));

#ifdef DEBUG_AC_VR
						debug_context(presence->get_in_room());
						debug_push_transform(presence->get_placement());
						debug_draw_transform_size_coloured_lerp(true, Transform::identity, 0.3f, Colour::orange, 0.9f);
						debug_draw_transform_size_coloured_lerp(true, headBoneOS, 0.3f, Colour::orange, 0.9f);
						debug_pop_transform();
						debug_no_context();
#endif
					}
					else
					{
						// lower head a bit as we're walking
						headBoneOS = headBoneOS.to_world(Transform(Vector3::zAxis * (-0.05f), Quat::identity));
						headBoneOS = headBoneOS.to_world(presence->get_requested_relative_look());
						if (vrData->useAbsoluteRotation)
						{
							headBoneOS.set_orientation(presence->get_requested_relative_look().get_orientation());
						}
						headBoneOS.set_translation(headBoneOS.get_translation() + Vector3(0.0f, 0.0f, presence->get_vertical_look_offset()));
						presence->make_head_bone_os_safe(REF_ headBoneOS);
						todo_note(TXT("at the moment hands are relative to final head bone, think if this should stay like this"));
						handLeftBoneOS = headBoneOS.to_world(presence->get_requested_relative_hand(0));
						handRightBoneOS = headBoneOS.to_world(presence->get_requested_relative_hand(1));
						if (Framework::GameUtils::is_controlled_by_local_player(get_owner()->get_owner()))
						{
#ifdef AN_STANDARD_INPUT
#ifdef AN_DEVELOPMENT_OR_PROFILER
							static bool handsDown = false;
							if (::System::Input::get()->has_key_been_pressed(System::Key::F))
							{
								handsDown = !handsDown;
							}
#endif
#endif
							for (int i = 0; i < 2; ++i)
							{
								Transform& additionalHandNonVRPlayerOffset = i == 0 ? additionalHandLeftNonVRPlayerOffset : additionalHandRightNonVRPlayerOffset;
								Vector3 targetOffset = Vector3::zero;
								float blendTime = 0.1f;
#ifdef AN_STANDARD_INPUT
#ifdef AN_DEVELOPMENT_OR_PROFILER
								if (::System::Input::get()->is_key_pressed(System::Key::LeftAlt))
								{
									blendTime = 0.005f;
								}

								if (::System::Input::get()->is_key_pressed(i == 0 ? System::Key::Q : System::Key::E))
								{
									float dist = 0.33f;
									if (::System::Input::get()->is_key_pressed(System::Key::LeftShift))
									{
										dist = 0.5f;
									}
									targetOffset = Vector3(0.0f, 0.65f, 0.0f) * dist;
								}
								else if (handsDown)
								{
									targetOffset = Vector3((i == 0 ? -1.0f : 1.0f) * 0.25f, -0.05f, 0.0f);
								}
#endif
#endif
								Vector3 currentOffset = additionalHandNonVRPlayerOffset.get_translation();
								currentOffset = blend_to_using_time(currentOffset, targetOffset, blendTime, _context.get_delta_time());
								additionalHandNonVRPlayerOffset.set_translation(currentOffset);
							}

							handLeftBoneOS = handLeftBoneOS.to_world(handLeftNonVRPlayerOffset.to_world(additionalHandLeftNonVRPlayerOffset));
							handRightBoneOS = handRightBoneOS.to_world(handRightNonVRPlayerOffset.to_world(additionalHandRightNonVRPlayerOffset));
						}
					}

					if (auto* vrpp = get_owner()->get_owner()->get_custom<CustomModules::VRPlacementsProvider>())
					{
						if (vrpp->get_head().is_set())
						{
							headBoneOS = vrpp->get_head().get();
						}
						if (vrpp->get_hand_left().is_set())
						{
							handLeftBoneOS = vrpp->get_hand_left().get();
						}
						if (vrpp->get_hand_right().is_set())
						{
							handRightBoneOS = vrpp->get_hand_right().get();
						}
					}

#ifdef AN_ALLOW_BULLSHOTS
					BullshotSystem::get_vr_placements(headBoneOS, handLeftBoneOS, handRightBoneOS);
#endif
				}

				if (vrData->relativeInOSToInitialHeadPlacementOnActivation)
				{
					if (isActive)
					{
						// should be enough with one frame. we may have controller not updated properly
						// also, we should have switch covered with "taking controls" effect
						if (activeForFrames <= 2) 
						{
							float baseYaw = 0.0f;
							if (relativeForwardDirYawOSVar.is_valid())
							{
								baseYaw = relativeForwardDirYawOSVar.get<float>();
							}
							Transform flat = look_matrix(headBoneOS.get_translation(), headBoneOS.get_axis(Axis::Forward).normal_2d().rotated_by_yaw(-baseYaw), Vector3::zAxis).to_transform();
							if (relativeForwardDirRollOSVar.is_valid())
							{
								flat = flat.to_world(Transform(Vector3::zero, Rotator3(0.0f, 0.0f, relativeForwardDirRollOSVar.get<float>()).to_quat()));
							}
							relativeToHeadBoneOS = flat;
						}
					}

					headBoneOS = relativeToHeadBoneOS.to_local(headBoneOS);
					handLeftBoneOS = relativeToHeadBoneOS.to_local(handLeftBoneOS);
					handRightBoneOS = relativeToHeadBoneOS.to_local(handRightBoneOS);
				}

				if (isActive)
				{
					if (!doOrientation)
					{
						headBoneOS.set_orientation(orgHeadBoneOS.get_orientation());
						handLeftBoneOS.set_orientation(orgHandLeftBoneOS.get_orientation());
						handRightBoneOS.set_orientation(orgHandRightBoneOS.get_orientation());
					}
					if (!doTranslation)
					{
						headBoneOS.set_translation(orgHeadBoneOS.get_translation());
						handLeftBoneOS.set_translation(orgHandLeftBoneOS.get_translation());
						handRightBoneOS.set_translation(orgHandRightBoneOS.get_translation());
					}

					if (headBoneLocationFromSocket.is_valid())
					{
						Transform fromSocketOS = appearance->calculate_socket_os(headBoneLocationFromSocket.get_index(), poseLS);
						headBoneOS.set_translation(fromSocketOS.get_translation());
					}
					lastHeadBoneOS = headBoneOS;
					lastHandLeftBoneOS = handLeftBoneOS;
					lastHandRightBoneOS = handRightBoneOS;
				}
				else
				{
					headBoneOS = lastHeadBoneOS;
					handLeftBoneOS = lastHandLeftBoneOS;
					handRightBoneOS = lastHandRightBoneOS;
				}

				headBoneMS = msToOs.to_local(headBoneOS);
				handLeftBoneMS = msToOs.to_local(handLeftBoneOS);
				handRightBoneMS = msToOs.to_local(handRightBoneOS);

				if (isActive &&
					(provideForwardDirMSVar.is_valid() || 
					 provideTargetPointMSVar.is_valid()))
				{
					Vector3 forwardDir = headBoneMS.get_axis(Axis::Forward);
					Rotator3 useOffsetProvidedDir = presence->is_vr_movement()? offsetProvidedDirForVR : offsetProvidedDir;
					if (!useOffsetProvidedDir.is_zero())
					{
						forwardDir = (forwardDir.to_rotator() + useOffsetProvidedDir).get_forward();
					}
					if (provideForwardDirMSVar.is_valid())
					{
						provideForwardDirMSVar.access<Vector3>() = forwardDir;
					}
					if (provideTargetPointMSVar.is_valid())
					{
						Vector3 targetPointMS = forwardDir * 1000.0f;
						provideTargetPointMSVar.access<Vector3>() = targetPointMS;
					}
				}

				if (!vrData->skipSettingBones)
				{
					Transform headParentBoneMS = headParentBone.is_valid() ? poseLS->get_bone_ms_from_ls(headParentBone.get_index()) : Transform::identity;
					Transform handLeftParentBoneMS = handLeftParentBone.is_valid() ? poseLS->get_bone_ms_from_ls(handLeftParentBone.get_index()) : Transform::identity;
					Transform handRightParentBoneMS = handRightParentBone.is_valid() ? poseLS->get_bone_ms_from_ls(handRightParentBone.get_index()) : Transform::identity;
					poseLS->set_bone(headBoneIdx, headParentBoneMS.to_local(headBoneMS));
					if (handLeftBoneIdx != NONE) poseLS->set_bone(handLeftBoneIdx, handLeftParentBoneMS.to_local(handLeftBoneMS));
					if (handRightBoneIdx != NONE) poseLS->set_bone(handRightBoneIdx, handRightParentBoneMS.to_local(handRightBoneMS));

#ifndef AN_CLANG
					an_assert(poseLS->is_ok());
#endif
				}
			}
		}
	}
}

void AppearanceControllersLib::VR::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (headBone.is_valid())
	{
		dependsOnParentBones.push_back(headBone.get_index());
		providesBones.push_back(headBone.get_index());
	}
	if (handLeftBone.is_valid())
	{
		dependsOnParentBones.push_back(handLeftBone.get_index());
		providesBones.push_back(handLeftBone.get_index());
	}
	if (handRightBone.is_valid())
	{
		dependsOnParentBones.push_back(handRightBone.get_index());
		providesBones.push_back(handRightBone.get_index());
	}
}

//

REGISTER_FOR_FAST_CAST(VRData);

AppearanceControllerData* VRData::create_data()
{
	return new VRData();
}

void VRData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(vr), create_data);
}

VRData::VRData()
{
}

bool VRData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("head")))
	{
		headBone.load_from_xml(node, TXT("bone"));
		headBoneLocationFromSocket.load_from_xml(node, TXT("locationFromSocket"));
		headVRViewOffset.load_from_xml(node, TXT("vrViewOffset"));
	}

	for_every(node, _node->children_named(TXT("handLeft")))
	{
		handLeftBone.load_from_xml(node, TXT("bone"));
		handLeftNonVRPlayerOffset.load_from_xml(node, TXT("nonVRPlayerOffset"));
	}

	for_every(node, _node->children_named(TXT("handRight")))
	{
		handRightBone.load_from_xml(node, TXT("bone"));
		handRightNonVRPlayerOffset.load_from_xml(node, TXT("nonVRPlayerOffset"));
	}

	doTranslation = true;
	doOrientation = true;
	if (_node->get_bool_attribute_or_from_child_presence(TXT("rotationOnly")))
	{
		doTranslation = false;
	}

	if (headBoneLocationFromSocket.is_set())
	{
		doTranslation = false;
	}

	skipSettingBones = _node->get_bool_attribute_or_from_child_presence(TXT("skipSettingBones"), skipSettingBones);
	useAbsoluteRotation = _node->get_bool_attribute_or_from_child_presence(TXT("useAbsoluteRotation"), useAbsoluteRotation);

	relativeInOSToInitialHeadPlacementOnActivation = _node->get_bool_attribute_or_from_child_presence(TXT("relativeInOSToInitialHeadPlacementOnActivation"), relativeInOSToInitialHeadPlacementOnActivation);
	result &= relativeForwardDirYawOSVar.load_from_xml(_node, TXT("relativeForwardDirYawOSVarID"));
	result &= relativeForwardDirRollOSVar.load_from_xml(_node, TXT("relativeForwardDirRollOSVarID"));

	warn_loading_xml_on_assert(!relativeInOSToInitialHeadPlacementOnActivation || !doTranslation, _node, TXT("if using relativeInOSToInitialHeadPlacementOnActivation, avoid using translation, add rotationOnly"));
	
	result &= offsetProvidedDir.load_from_xml(_node, TXT("offsetProvidedDir"));
	result &= offsetProvidedDirForVR.load_from_xml(_node, TXT("offsetProvidedDirForVR"));
	result &= provideForwardDirMSVar.load_from_xml(_node, TXT("provideForwardDirMSVarID"));
	result &= provideTargetPointMSVar.load_from_xml(_node, TXT("provideTargetPointMSVarID"));

	return result;
}

bool VRData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* VRData::create_copy() const
{
	VRData* copy = new VRData();
	*copy = *this;
	return copy;
}

AppearanceController* VRData::create_controller()
{
	return new VR(this);
}

void VRData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	headBone.if_value_set([this, _rename]() { headBone = _rename(headBone.get(), NP); });
	headBoneLocationFromSocket.if_value_set([this, _rename]() { headBoneLocationFromSocket = _rename(headBoneLocationFromSocket.get(), NP); });
	handLeftBone.if_value_set([this, _rename]() { handLeftBone = _rename(handLeftBone.get(), NP); });
	handRightBone.if_value_set([this, _rename]() { handRightBone = _rename(handRightBone.get(), NP); });
	relativeForwardDirYawOSVar.if_value_set([this, _rename]() { relativeForwardDirYawOSVar = _rename(relativeForwardDirYawOSVar.get(), NP); });
	relativeForwardDirRollOSVar.if_value_set([this, _rename]() { relativeForwardDirRollOSVar = _rename(relativeForwardDirRollOSVar.get(), NP); });
	provideForwardDirMSVar.if_value_set([this, _rename]() { provideForwardDirMSVar = _rename(provideForwardDirMSVar.get(), NP); });
	provideTargetPointMSVar.if_value_set([this, _rename]() { provideTargetPointMSVar = _rename(provideTargetPointMSVar.get(), NP); });
}

void VRData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	headBone.fill_value_with(_context);
	headVRViewOffset.fill_value_with(_context);
	headBoneLocationFromSocket.fill_value_with(_context);
	handLeftBone.fill_value_with(_context);
	handRightBone.fill_value_with(_context);
	handLeftNonVRPlayerOffset.fill_value_with(_context);
	handRightNonVRPlayerOffset.fill_value_with(_context);
	relativeForwardDirYawOSVar.fill_value_with(_context);
	relativeForwardDirRollOSVar.fill_value_with(_context);
	offsetProvidedDir.fill_value_with(_context);
	offsetProvidedDirForVR.fill_value_with(_context);
	provideForwardDirMSVar.fill_value_with(_context);
	provideTargetPointMSVar.fill_value_with(_context);
}
