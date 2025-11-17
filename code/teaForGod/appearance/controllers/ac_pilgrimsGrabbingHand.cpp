#include "ac_pilgrimsGrabbingHand.h"

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

DEFINE_STATIC_NAME(pilgrimsGrabbingHand);

//

REGISTER_FOR_FAST_CAST(PilgrimsGrabbingHand);

PilgrimsGrabbingHand::PilgrimsGrabbingHand(PilgrimsGrabbingHandData const * _data)
: base(_data)
, pilgrimsGrabbingHandData(_data)
{
	bone = pilgrimsGrabbingHandData->bone.get();
}

PilgrimsGrabbingHand::~PilgrimsGrabbingHand()
{
}

void PilgrimsGrabbingHand::initialise(Framework::ModuleAppearance* _owner)
{
	base::initialise(_owner);

	if (auto* skel = get_owner()->get_skeleton()->get_skeleton())
	{
		bone.look_up(skel);
		parent = Meshes::BoneID(skel, skel->get_parent_of(bone.get_index()));
	}
}

void PilgrimsGrabbingHand::advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void PilgrimsGrabbingHand::calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(pilgrimsGrabbingHand_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid() ||
		!parent.is_valid())
	{
		return;
	}

	float grabbingTarget = 0.0f;
	Transform grabbedPlacementTargetOS = grabbedPlacementOS;
	if (auto* pilgrim = get_owner()->get_owner()->get_gameplay_as<ModulePilgrim>())
	{
		if (auto* grabbed = pilgrim->get_grabbed_object(pilgrimsGrabbingHandData->hand))
		{
			if (grabbedObject == grabbed ||
				! grabbedObject.is_set()) // grabbed object is cleared only if grabbingActive is 0 - when switching we want to blend out from last object and blend into new one
			{
				if (auto* grabable = grabbed->get_custom<CustomModules::Grabable>())
				{
					auto handIdx = pilgrimsGrabbingHandData->hand;
					if (auto * hand = pilgrim->get_hand(handIdx))
					{
						auto & grabbedPath = pilgrim->get_grabbed_object_path(handIdx);

						bool setupGrab = grabbedObject != grabbed;

						grabbedObject = grabbed;
						grabbingTarget = 1.0f;
						{
							// hand's WS
							Transform grabbedWS = grabbedPath.from_target_to_owner(grabbed->get_presence()->get_placement());
							// hand -> pilgrim
							grabbedWS = hand->get_presence()->get_path_to_attached_to()->from_owner_to_target(grabbedWS);
							// to main object's OS
							grabbedPlacementTargetOS = get_owner()->get_owner()->get_presence()->get_placement().to_local(grabbedWS);
						}
						
						if (setupGrab)
						{
							if (auto * heldObjectSize = grabbed->get_variables().get_existing<float>(pilgrim->get_objects_held_object_size_var_ID()))
							{
								Name grabableAxisSocket = grabable->get_grabable_axis_socket_name();
								if (grabableAxisSocket.is_valid())
								{
#ifdef DEBUG_DRAW_GRAB
									debug_filter(pilgrimGrab);
									debug_context(get_owner()->get_owner()->get_presence()->get_in_room());
#endif
									// this is grabbed object's OS
									Transform grabbedGrabableAxisPlacementOS = grabbed->get_appearance()->calculate_socket_os(grabableAxisSocket);
									// to main object's OS
									grabbedGrabableAxisPlacementOS = grabbedPlacementTargetOS.to_world(grabbedGrabableAxisPlacementOS);
									Vector3 grabbedGrabableAxisOS = grabbedGrabableAxisPlacementOS.get_axis(hardcoded Axis::Forward); // hard coded!

									// this is hand's OS
									Transform grabPointPlacementHandOS = hand->get_appearance()->calculate_socket_os(pilgrim->get_pilgrim_data()->get_hand_grab_point_sockets().get(handIdx, true));
									Transform grabPointPlacementOS;
									{
										// hand's WS
										Transform grabPointPlacementWS = hand->get_presence()->get_placement().to_world(grabPointPlacementHandOS);
										// hand -> pilgrim
										grabPointPlacementWS = hand->get_presence()->get_path_to_attached_to()->from_target_to_owner(grabPointPlacementWS);
										// to main object's OS
										grabPointPlacementOS = get_owner()->get_owner()->get_presence()->get_placement().to_local(grabPointPlacementWS);
#ifdef DEBUG_DRAW_GRAB
										debug_draw_transform_size_coloured(true, grabPointPlacementWS, 0.1f, Colour::red, Colour::red, Colour::red);
#endif
									}

#ifdef DEBUG_DRAW_GRAB
									debug_push_transform(get_owner()->get_owner()->get_presence()->get_placement());
									debug_draw_arrow(true, Colour::green, grabPointPlacementOS.get_translation(), grabbedGrabableAxisPlacementOS.get_translation());
									debug_pop_transform();
#endif

									Axis::Type grabPointGrabAxis = pilgrim->get_pilgrim_data()->get_hand_grab_point(handIdx).axis;
									Axis::Type grabPointGrabDirAxis = pilgrim->get_pilgrim_data()->get_hand_grab_point(handIdx).dirAxis;
									float grabPointGrabDirSign = pilgrim->get_pilgrim_data()->get_hand_grab_point(handIdx).dirSign;
									Vector3 grabPointGrabAxisOS = grabPointPlacementOS.get_axis(grabPointGrabAxis);

									// now point where we should be
									Vector3 grabPointRequestedGrabAxisOS = Vector3::dot(grabPointGrabAxisOS, grabbedGrabableAxisOS) > 0.0f ? grabbedGrabableAxisOS : -grabbedGrabableAxisOS;

									Vector3 grabbedLocOS = grabbedGrabableAxisPlacementOS.get_translation();
									Vector3 grabPointRequestedGrabDirOS = (grabPointPlacementOS.get_axis(grabPointGrabDirAxis) * grabPointGrabDirSign).normal();

									if (setupGrab)
									{
										// have initial turn
										grabPointRequestedGrabDirOS = ((grabbedLocOS - grabPointPlacementOS.get_translation()).normal() * 0.3f + 0.7f * grabPointRequestedGrabDirOS).normal();
									}
									else
									{
										// always try to maintain how hand is placed
										grabPointRequestedGrabDirOS = blend_to_using_time(lastGrabPointRequestedGrabDirOS, grabPointRequestedGrabDirOS, 0.1f, _context.get_delta_time()).normal();
									}
									lastGrabPointRequestedGrabDirOS = grabPointRequestedGrabDirOS;

									grabPointRequestedGrabDirOS = grabPointRequestedGrabDirOS.drop_using_normalised(grabPointRequestedGrabAxisOS).normal();

									Transform grabPointRequestedOS;
									Vector3 grabPointRequestedLocOS = grabbedLocOS - grabPointRequestedGrabDirOS * (0.5f * *heldObjectSize);

									grabPointRequestedOS.build_from_two_axes(grabPointGrabAxis, grabPointGrabDirAxis, grabPointRequestedGrabAxisOS, grabPointRequestedGrabDirOS * grabPointGrabDirSign, grabPointRequestedLocOS);

									Transform grabPointRequestedHandOS = grabPointRequestedOS.to_world(grabPointPlacementHandOS.inverted());
									handRelativeToGrabbedPlacement = grabbedPlacementTargetOS.to_local(grabPointRequestedHandOS);

#ifdef DEBUG_DRAW_GRAB
									debug_no_context();
									debug_no_filter();
#endif
								}
							}
							{
								Name grabableDialSocket = grabable->get_grabable_dial_socket_name();
								if (grabableDialSocket.is_valid())
								{
#ifdef DEBUG_DRAW_GRAB
									debug_filter(pilgrimGrab);
									debug_context(get_owner()->get_owner()->get_presence()->get_in_room());
#endif
									// this is grabbed object's OS
									Transform grabbedGrabableDialPlacementOS = grabbed->get_appearance()->calculate_socket_os(grabableDialSocket);
									// to main object's OS
									grabbedGrabableDialPlacementOS = grabbedPlacementTargetOS.to_world(grabbedGrabableDialPlacementOS);
									Vector3 grabbedDialAxisOS = grabbedGrabableDialPlacementOS.get_axis(hardcoded Axis::Up); // hard coded!

									// this is hand's OS
									Transform grabPointPlacementHandOS = hand->get_appearance()->calculate_socket_os(pilgrim->get_pilgrim_data()->get_hand_grab_point_dial_sockets().get(handIdx, true));
									Transform grabPointPlacementOS;
									{
										// hand's WS
										Transform grabPointPlacementWS = hand->get_presence()->get_placement().to_world(grabPointPlacementHandOS);
										// hand -> pilgrim
										grabPointPlacementWS = hand->get_presence()->get_path_to_attached_to()->from_target_to_owner(grabPointPlacementWS);
										// to main object's OS
										grabPointPlacementOS = get_owner()->get_owner()->get_presence()->get_placement().to_local(grabPointPlacementWS);
#ifdef DEBUG_DRAW_GRAB
										debug_draw_transform_size_coloured(true, grabPointPlacementWS, 0.1f, Colour::red, Colour::red, Colour::red);
#endif
									}

#ifdef DEBUG_DRAW_GRAB
									debug_push_transform(get_owner()->get_owner()->get_presence()->get_placement());
									debug_draw_arrow(true, Colour::green, grabPointPlacementOS.get_translation(), grabbedGrabableDialPlacementOS.get_translation());
									debug_pop_transform();
#endif

									Vector3 grabbedDialLocOS = grabbedGrabableDialPlacementOS.get_translation();

									Axis::Type grabPointGrabDialAxis = pilgrim->get_pilgrim_data()->get_hand_grab_point_dial(handIdx).axis;
									float grabPointGrabDialSign = pilgrim->get_pilgrim_data()->get_hand_grab_point_dial(handIdx).axisSign;

									Axis::Type grabPointGrabDirAxis = pilgrim->get_pilgrim_data()->get_hand_grab_point_dial(handIdx).dirAxis;
									float grabPointGrabDirSign = pilgrim->get_pilgrim_data()->get_hand_grab_point_dial(handIdx).dirSign;

									Vector3 grabPointRequestedGrabDirOS = (grabPointPlacementOS.get_axis(grabPointGrabDirAxis) * grabPointGrabDirSign).normal();
									
									Transform grabPointRequestedOS;
									grabPointRequestedOS.build_from_two_axes(grabPointGrabDialAxis, grabPointGrabDirAxis,
										grabbedDialAxisOS * grabPointGrabDialSign, // grab dial comes from the actual device
										grabPointRequestedGrabDirOS * grabPointGrabDirSign, // grab dir comes from hand only
										grabbedDialLocOS);

									Transform grabPointRequestedHandOS = grabPointRequestedOS.to_world(grabPointPlacementHandOS.inverted());
									handRelativeToGrabbedPlacement = grabbedPlacementTargetOS.to_local(grabPointRequestedHandOS);

#ifdef DEBUG_DRAW_GRAB
									debug_no_context();
									debug_no_filter();
#endif
								}
							}
						}
					}
				}
			}
		}
	}

	if (!handRelativeToGrabbedPlacement.is_set())
	{
		grabbingActive = 0.0f;
	}

	float prevGrabbingActive = grabbingActive;
	grabbingActive = blend_to_using_speed_based_on_time(grabbingActive, grabbingTarget, pilgrimsGrabbingHandData->grabbingTime, _context.get_delta_time());
	if (grabbingActive == 0.0f)
	{
		grabbedObject.clear();
	}

	if (grabbingActive == 1.0f)
	{
		grabbedPlacementOS = grabbedPlacementTargetOS;
	}
	else if (grabbingTarget > 0.0f)
	{
		if (prevGrabbingActive == 0.0f)
		{
			grabbedPlacementOS = grabbedPlacementTargetOS;
		}
		else
		{
			grabbedPlacementOS = Transform::lerp(grabbingActive, grabbedPlacementOS, grabbedPlacementTargetOS);
		}
	}

	if (grabbingActive > 0.0f)
	{
		an_assert(handRelativeToGrabbedPlacement.is_set());
		Transform handPlacementOS = grabbedPlacementOS.to_world(handRelativeToGrabbedPlacement.get());

		int const boneIdx = bone.get_index();
		int const parentIdx = parent.get_index();
		Meshes::Pose * poseLS = _context.access_pose_LS();

		Transform boneLS = poseLS->get_bone(boneIdx);
		Transform parentMS = poseLS->get_bone_ms_from_ls(parentIdx);

		Transform handPlacementMS = get_owner()->get_ms_to_os_transform().to_local(handPlacementOS);
		Transform newBoneLS = parentMS.to_local(handPlacementMS);
		poseLS->access_bones()[boneIdx] = Transform::lerp(BlendCurve::cubic(grabbingActive), boneLS, newBoneLS);
#ifdef DEBUG_DRAW_GRAB
		debug_filter(pilgrimGrab);
		debug_context(get_owner()->get_owner()->get_presence()->get_in_room());

		debug_push_transform(get_owner()->get_owner()->get_presence()->get_placement());
		debug_draw_transform_size_coloured(true, grabbedPlacementOS, 0.1f, Colour::green, Colour::green, Colour::green);
		debug_draw_transform_size_coloured(true, handPlacementOS, 0.03f, Colour::orange, Colour::orange, Colour::orange);
		// from desired to grabbed
		debug_draw_arrow(true, Colour::orange, handPlacementOS.get_translation(), grabbedPlacementOS.get_translation());
		// from current to desired
		debug_draw_arrow(true, Colour::blue, get_owner()->get_ms_to_os_transform().location_to_world(parentMS.to_world(boneLS).get_translation()), handPlacementOS.get_translation());
		debug_pop_transform();

		debug_no_context();
		debug_no_filter();
#endif
	}
}

void PilgrimsGrabbingHand::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
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

REGISTER_FOR_FAST_CAST(PilgrimsGrabbingHandData);

Framework::AppearanceControllerData* PilgrimsGrabbingHandData::create_data()
{
	return new PilgrimsGrabbingHandData();
}

void PilgrimsGrabbingHandData::register_itself()
{
	Framework::AppearanceControllers::register_appearance_controller(NAME(pilgrimsGrabbingHand), create_data);
}

PilgrimsGrabbingHandData::PilgrimsGrabbingHandData()
{
}

bool PilgrimsGrabbingHandData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	hand = Hand::parse(_node->get_string_attribute_or_from_child(TXT("hand"), String(Hand::to_char(hand))));
	grabbingTime = _node->get_float_attribute_or_from_child(TXT("grabbingTime"), grabbingTime);

	return result;
}

bool PilgrimsGrabbingHandData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::AppearanceControllerData* PilgrimsGrabbingHandData::create_copy() const
{
	PilgrimsGrabbingHandData* copy = new PilgrimsGrabbingHandData();
	*copy = *this;
	return copy;
}

Framework::AppearanceController* PilgrimsGrabbingHandData::create_controller()
{
	return new PilgrimsGrabbingHand(this);
}

void PilgrimsGrabbingHandData::apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
}

void PilgrimsGrabbingHandData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

void PilgrimsGrabbingHandData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
}
