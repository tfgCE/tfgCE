#include "ac_demoRobot.h"

#include "..\..\game\game.h"
#include "..\..\modules\custom\mc_emissiveControl.h"

#include "..\..\..\framework\ai\aiMessage.h"
#include "..\..\..\framework\appearance\appearanceControllerPoseContext.h"
#include "..\..\..\framework\appearance\appearanceControllers.h"
#include "..\..\..\framework\appearance\skeleton.h"
#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleController.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\performance\performanceUtils.h"
#include "..\..\..\core\system\input.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(demoRobot);

// sounds/emissive/sockets
DEFINE_STATIC_NAME_STR(legMovement, TXT("leg movement"));
DEFINE_STATIC_NAME(speak);
DEFINE_STATIC_NAME_STR(speakYes, TXT("speak yes"));
DEFINE_STATIC_NAME_STR(speakNo, TXT("speak no"));

// ai message
DEFINE_STATIC_NAME(elevatorCartControlMovementOrder);

//

REGISTER_FOR_FAST_CAST(DemoRobot);

DemoRobot::DemoRobot(DemoRobotData const * _data)
: base(_data)
{
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(headBone);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(torsoBone);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(leftLegBone);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(rightLegBone);

	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(headLookBlendTime);

	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(walkCycleBlendInTime);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(walkCycleBlendOutTime);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(walkCycleLength);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(walkCycleLengthRange);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(walkCycleLengthNominalSpeed);

	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(legPivotSocket);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(walkCycleLegAngle);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(walkCycleFootAngle);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(walkCycleLegUpDist);

	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(walkCycleJumpDist);

	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(speakYesTime);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(speakYesAngle);

	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(speakNoTime);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(speakNoAngle);

	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(speakBlendInTime);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(speakBlendOutTime);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(speakAngles);
}

DemoRobot::~DemoRobot()
{
}

void DemoRobot::initialise(Framework::ModuleAppearance* _owner)
{
	base::initialise(_owner);

	if (auto* skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		headBone.look_up(skeleton);
		torsoBone.look_up(skeleton);
		leftLegBone.look_up(skeleton);
		rightLegBone.look_up(skeleton);
	}

	if (auto* mesh = get_owner()->get_mesh())
	{
		legPivotSocket.look_up(mesh);
	}
}

void DemoRobot::advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void DemoRobot::calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(demoRobot_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!get_active())
	{
		return;
	}

	auto* imo = get_owner()->get_owner();
	auto* presence = imo->get_presence();

#ifdef AN_STANDARD_INPUT
	auto* input = System::Input::get();
#endif

	debug_filter(ac_demoRobot);

	debug_context(presence->get_in_room());

	bool shouldPlayLeftLegSound = false;
	bool shouldPlayRightLegSound = false;

	Transform placementWS = presence->get_placement();

	float deltaTime = _context.get_delta_time();

	float walkCycleActiveTarget = 0.0f;
	float walkCurrentSpeed = 0.0f;

	if (auto* c = imo->get_controller())
	{
		walkCurrentSpeed = imo->get_presence()->get_velocity_linear().length_2d();

		if (!c->get_requested_movement_direction().get(Vector3::zero).is_zero() || walkCurrentSpeed > 0.05f)
		{
			walkCycleActiveTarget = 1.0f;
		}

		if (imo->get_presence()->get_velocity_rotation().length() > 10.0f)
		{
			walkCycleActiveTarget = 1.0f;
			walkCurrentSpeed = max(walkCurrentSpeed, walkCycleLengthNominalSpeed != 0.0f? walkCycleLengthNominalSpeed : 1.0f);
		}
	}

	walkCycleActive = blend_to_using_speed_based_on_time(walkCycleActive, walkCycleActiveTarget, walkCycleActive < walkCycleActiveTarget? walkCycleBlendInTime : walkCycleBlendOutTime, deltaTime);

	if (walkCycleActive == 0.0f)
	{
		walkCyclePt = 0.0f;
	}
	else
	{
		float useLength = walkCycleLength;
		if (walkCycleLengthNominalSpeed != 0.0f)
		{
			float coef = walkCurrentSpeed / walkCycleLengthNominalSpeed;
			useLength /= max(0.001f, coef);
		}
		if (!walkCycleLengthRange.is_empty())
		{
			useLength = walkCycleLengthRange.clamp(useLength);
		}
		walkCyclePt = mod(walkCyclePt + deltaTime / useLength, 1.0f);
	}

	Meshes::Pose* poseLS = _context.access_pose_LS();

	// get all placement before torso has been moved
	Transform headBoneMS = poseLS->get_bone_ms_from_ls(headBone.get_index());
	Transform torsoBoneMS = poseLS->get_bone_ms_from_ls(torsoBone.get_index());
	Transform leftLegBoneMS = poseLS->get_bone_ms_from_ls(leftLegBone.get_index());
	Transform rightLegBoneMS = poseLS->get_bone_ms_from_ls(rightLegBone.get_index());

	float walkCyclePtDeg = 360.0f * walkCyclePt;

	// first move torso and store it
	{
		Vector3 torsoLoc = torsoBoneMS.get_translation();
		torsoLoc.z += abs(sin_deg(walkCyclePtDeg)) * walkCycleJumpDist * walkCycleActive;
		torsoBoneMS.set_translation(torsoLoc);

		poseLS->set_bone_ms(torsoBone.get_index(), torsoBoneMS);
	}

	// now legs
	{
		Transform legPivotSocketMS = Transform(Vector3::zAxis * 0.6f, Quat::identity);
		if (legPivotSocket.get_index() != NONE)
		{
			legPivotSocketMS = get_owner()->calculate_socket_ms(legPivotSocket.get_index(), poseLS);
		}
		for_count(int, legIdx, 2)
		{
			Transform& legBoneMS = legIdx == 0 ? leftLegBoneMS : rightLegBoneMS;
			Transform orgLegBoneMS = legBoneMS;
			float legSign = legIdx == 0 ? -1.0f : 1.0f;
			Transform pivotLeg = Transform(Vector3::zero, Rotator3(legSign * sin_deg(walkCyclePtDeg) * walkCycleLegAngle, 0.0f, 0.0f).to_quat());
			legBoneMS = legPivotSocketMS.to_world(pivotLeg.to_world(legPivotSocketMS.to_local(legBoneMS)));
			Transform pivotFoot = Transform(Vector3::zAxis * (((legSign * cos_deg(walkCyclePtDeg) > 0.0f)? sqr(cos_deg(walkCyclePtDeg)): 0.0f) * walkCycleLegUpDist * walkCycleActive),
											Rotator3(legSign * sin_deg(walkCyclePtDeg) * walkCycleFootAngle, 0.0f, 0.0f).to_quat());
			legBoneMS = legBoneMS.to_world(pivotFoot);
			legBoneMS = lerp(walkCycleActive, orgLegBoneMS, legBoneMS);
		}

		poseLS->set_bone_ms(leftLegBone.get_index(), leftLegBoneMS);
		poseLS->set_bone_ms(rightLegBone.get_index(), rightLegBoneMS);
	}

	if (walkCycleActive < 0.01f)
	{
		shouldPlayLeftLegSound = false;
		shouldPlayRightLegSound = false;
	}
	else
	{
		// left leg down at 0 and 1, shift it by 0.5
		shouldPlayLeftLegSound = abs(mod(walkCyclePt + 0.5f, 1.0f) - 0.5f) > 0.2f;

		// right leg down at 0.5
		shouldPlayRightLegSound = abs(walkCyclePt - 0.5f) > 0.2f;
	}

#ifdef AN_STANDARD_INPUT
	if (input && input->has_key_been_pressed(::System::Key::T))
	{
		if (auto* b = imo->get_presence()->get_based_on())
		{
			if (auto* aim = imo->get_in_world()->create_ai_message(NAME(elevatorCartControlMovementOrder)))
			{
				aim->to_ai_object(b);
			}
		}		
	}
#endif

	// head last
	{
		Quat headLookTarget = imo->get_presence()->get_requested_relative_look().get_orientation();
#ifdef AN_STANDARD_INPUT
		if (input && input->is_key_pressed(::System::Key::Q))
		{
			headLookTarget = Rotator3(30.0f, 0.0f, 0.0f).to_quat();
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
			if (auto* g = Game::get_as<Game>())
			{
				Optional<Transform> externalCameraVRAnchorOffset = g->get_external_camera_vr_anchor_offset();
				if (externalCameraVRAnchorOffset.is_set())
				{
					Rotator3 r = externalCameraVRAnchorOffset.get().get_translation().to_rotator();
					r.pitch *= 0.5f;
					headLookTarget = imo->get_presence()->get_placement().to_local(imo->get_presence()->get_vr_anchor().to_world(r.to_quat()));
				}
			}
#endif
		}
#endif
		headLookTarget = (headLookTarget.to_rotator() * Rotator3(0.5f, 1.0f, 0.0f)).to_quat();

		if (headLookBlendTime > 0.0f)
		{
			headLook = Quat::slerp(deltaTime / headLookBlendTime, headLook, headLookTarget);
		}
		else
		{
			headLook = headLookTarget;
		}

		Quat useHeadLook = headLook;

		{
#ifdef AN_STANDARD_INPUT
			if (input && input->has_key_been_pressed(::System::Key::R))
			{
				if (speakYesActive == 0.0f)
				{
					speakNoActive = min(speakNoActive, 0.99f);
					speakYesActive = 1.0f;
					speakYesPt = 0.0f;
					if (auto* s = imo->get_sound())
					{
						s->stop_sound(NAME(speak), 0.05f);
						s->stop_sound(NAME(speakYes), 0.05f);
						s->stop_sound(NAME(speakNo), 0.05f);
						s->play_sound(NAME(speakYes), NAME(speak));
					}
				}
			}
			if (input && input->has_key_been_pressed(::System::Key::F))
			{
				if (speakNoActive == 0.0f)
				{
					speakYesActive = min(speakYesActive, 0.99f);
					speakNoActive = 1.0f;
					speakNoPt = 0.0f;
					if (auto* s = imo->get_sound())
					{
						s->stop_sound(NAME(speak), 0.05f);
						s->stop_sound(NAME(speakYes), 0.05f);
						s->stop_sound(NAME(speakNo), 0.05f);
						s->play_sound(NAME(speakNo), NAME(speak));
					}
				}
			}
			if (input && input->has_key_been_pressed(::System::Key::E))
			{
				speakTime = 0.0f;
				Rotator3 sa;
				sa.pitch = rg.get(speakAngles.x);
				sa.yaw = rg.get(speakAngles.z);
				sa.roll = rg.get(speakAngles.y);
				speakAngle = sa.to_quat();
				speakYesActive = min(speakYesActive, 0.99f);
				speakNoActive = min(speakNoActive, 0.99f);
				if (auto* s = imo->get_sound())
				{
					s->stop_sound(NAME(speak), 0.05f);
					s->stop_sound(NAME(speakYes), 0.05f);
					s->stop_sound(NAME(speakNo), 0.05f);
					s->play_sound(NAME(speak), NAME(speak));
				}
			}
#endif

			if (speakYesActive > 0.0f)
			{
				speakYesPt += deltaTime / speakYesTime;
				if (speakYesPt > 1.0f)
				{
					speakYesPt = 0.0f;
					speakYesActive = 0.0f;
				}
				if (speakYesActive < 1.0f)
				{
					speakYesActive = blend_to_using_speed_based_on_time(speakYesActive, 0.0f, 0.1f, deltaTime);
				}
			}
			if (speakNoActive > 0.0f)
			{
				speakNoPt += deltaTime / speakNoTime;
				if (speakNoPt > 1.0f)
				{
					speakNoPt = 0.0f;
					speakNoActive = 0.0f;
				}
				if (speakNoActive < 1.0f)
				{
					speakNoActive = blend_to_using_speed_based_on_time(speakNoActive, 0.0f, 0.1f, deltaTime);
				}
			}

			if (auto* e = imo->get_custom<CustomModules::EmissiveControl>())
			{
				if (speakTime.is_set() && speakTime.get() < speakBlendInTime * 0.5f)
				{
					e->emissive_activate(NAME(speak), speakBlendInTime * 0.5f);
				}
				else
				{
					e->emissive_deactivate(NAME(speak), speakBlendOutTime * 0.5f);
				}
				if (speakYesActive == 1.0f && speakYesPt < 0.8f)
				{
					e->emissive_activate(NAME(speakYes));
				}
				else
				{
					e->emissive_deactivate(NAME(speakYes));
				}
				if (speakNoActive == 1.0f && speakNoPt < 0.8f)
				{
					e->emissive_activate(NAME(speakNo));
				}
				else
				{
					e->emissive_deactivate(NAME(speakNo));
				}
			}

			if (speakTime.is_set())
			{
				if (deltaTime != 0.0f)
				{
					float prevTime = speakTime.get();
					speakTime = prevTime + deltaTime;
					if (prevTime < speakBlendInTime)
					{
						speakTime = min(speakTime.get(), speakBlendInTime);
						float prevAtPt = prevTime / speakBlendInTime;
						float atPt = speakTime.get() / speakBlendInTime;

						speakOrientation = Quat::slerp(clamp((atPt - prevAtPt) / (1.0f - prevAtPt), 0.0f, 1.0f), speakOrientation, speakAngle);
					}
					else
					{
						float prevAtPt = (prevTime - speakBlendInTime) / speakBlendOutTime;
						float atPt = (speakTime.get() - speakBlendInTime) / speakBlendOutTime;

						atPt = clamp(atPt, prevAtPt, 1.0f);

						speakOrientation = Quat::slerp(clamp((atPt - prevAtPt) / (1.0f - prevAtPt), 0.0f, 1.0f), speakOrientation, Quat::identity);

						if (atPt >= 1.0f)
						{
							speakTime.clear();
						}
					}
				}
			}
			else
			{
				speakOrientation = Quat::slerp(clamp(deltaTime / 0.5f, 0.0f, 1.0f), speakOrientation, Quat::identity);
			}

			Quat yesOrientation = (Rotator3(sqr(sin_deg(180.0f * speakYesPt)) * speakYesAngle, 0.0f, 0.0f) * speakYesActive).to_quat();
			Quat noOrientation = (Rotator3(0.0f, sin_deg(360.0f * speakNoPt) * speakNoAngle, 0.0f) * speakNoActive * clamp(4.0f * sin_deg(180.0f * speakNoPt), 0.0f, 1.0f)).to_quat();

			useHeadLook = useHeadLook.to_world(yesOrientation).to_world(noOrientation).to_world(speakOrientation);
		}

		headBoneMS = poseLS->get_bone_ms_from_ls(headBone.get_index());
		headBoneMS.set_orientation(useHeadLook);

		poseLS->set_bone_ms(headBone.get_index(), headBoneMS);
	}

	debug_no_context();

	debug_no_filter();

	if (auto* s = imo->get_sound())
	{
		if (leftLegSound.playing != shouldPlayLeftLegSound)
		{
			leftLegSound.playing = shouldPlayLeftLegSound;
			if (leftLegSound.playing)
			{
				leftLegSound.sound = s->play_sound(NAME(legMovement));
			}
			else if (leftLegSound.sound.get())
			{
				leftLegSound.sound->stop();
			}
		}
		if (rightLegSound.playing != shouldPlayRightLegSound)
		{
			rightLegSound.playing = shouldPlayRightLegSound;
			if (rightLegSound.playing)
			{
				rightLegSound.sound = s->play_sound(NAME(legMovement));
			}
			else if (rightLegSound.sound.get())
			{
				rightLegSound.sound->stop();
			}
		}
	}
}

void DemoRobot::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	// no need
}

//

REGISTER_FOR_FAST_CAST(DemoRobotData);

Framework::AppearanceControllerData* DemoRobotData::create_data()
{
	return new DemoRobotData();
}

void DemoRobotData::register_itself()
{
	Framework::AppearanceControllers::register_appearance_controller(NAME(demoRobot), create_data);
}

DemoRobotData::DemoRobotData()
{
}

bool DemoRobotData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= headBone.load_from_xml(_node, TXT("headBone"));
	result &= torsoBone.load_from_xml(_node, TXT("torsoBone"));
	result &= leftLegBone.load_from_xml(_node, TXT("leftLegBone"));
	result &= rightLegBone.load_from_xml(_node, TXT("rightLegBone"));

	result &= headLookBlendTime.load_from_xml(_node, TXT("headLookBlendTime"));

	result &= walkCycleBlendInTime.load_from_xml(_node, TXT("walkCycleBlendTime"));
	result &= walkCycleBlendOutTime.load_from_xml(_node, TXT("walkCycleBlendTime"));
	result &= walkCycleBlendInTime.load_from_xml(_node, TXT("walkCycleBlendInTime"));
	result &= walkCycleBlendOutTime.load_from_xml(_node, TXT("walkCycleBlendOutTime"));
	result &= walkCycleLength.load_from_xml(_node, TXT("walkCycleLength"));
	result &= walkCycleLengthRange.load_from_xml(_node, TXT("walkCycleLengthRange"));
	result &= walkCycleLengthNominalSpeed.load_from_xml(_node, TXT("walkCycleLengthNominalSpeed"));

	result &= legPivotSocket.load_from_xml(_node, TXT("legPivotSocket"));
	result &= walkCycleLegAngle.load_from_xml(_node, TXT("walkCycleLegAngle"));
	result &= walkCycleFootAngle.load_from_xml(_node, TXT("walkCycleFootAngle"));
	result &= walkCycleLegUpDist.load_from_xml(_node, TXT("walkCycleLegUpDist"));

	result &= walkCycleJumpDist.load_from_xml(_node, TXT("walkCycleJumpDist"));

	result &= speakYesTime.load_from_xml(_node, TXT("speakYesTime"));
	result &= speakYesAngle.load_from_xml(_node, TXT("speakYesAngle"));

	result &= speakNoTime.load_from_xml(_node, TXT("speakNoTime"));
	result &= speakNoAngle.load_from_xml(_node, TXT("speakNoAngle"));

	result &= speakBlendInTime.load_from_xml(_node, TXT("speakBlendInTime"));
	result &= speakBlendOutTime.load_from_xml(_node, TXT("speakBlendOutTime"));
	result &= speakAngles.load_from_xml(_node, TXT("speakAngles"));

	return result;
}

bool DemoRobotData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::AppearanceControllerData* DemoRobotData::create_copy() const
{
	DemoRobotData* copy = new DemoRobotData();
	*copy = *this;
	return copy;
}

Framework::AppearanceController* DemoRobotData::create_controller()
{
	return new DemoRobot(this);
}

void DemoRobotData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	// no reskinning here
}

void DemoRobotData::apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	headBone.fill_value_with(_context);
	torsoBone.fill_value_with(_context);
	leftLegBone.fill_value_with(_context);
	rightLegBone.fill_value_with(_context);

	headLookBlendTime.fill_value_with(_context);

	walkCycleBlendInTime.fill_value_with(_context);
	walkCycleBlendOutTime.fill_value_with(_context);
	walkCycleLength.fill_value_with(_context);
	walkCycleLengthRange.fill_value_with(_context);
	walkCycleLengthNominalSpeed.fill_value_with(_context);

	legPivotSocket.fill_value_with(_context);
	walkCycleLegAngle.fill_value_with(_context);
	walkCycleFootAngle.fill_value_with(_context);
	walkCycleLegUpDist.fill_value_with(_context);

	walkCycleJumpDist.fill_value_with(_context);

	speakYesTime.fill_value_with(_context);
	speakYesAngle.fill_value_with(_context);

	speakNoTime.fill_value_with(_context);
	speakNoAngle.fill_value_with(_context);

	speakBlendInTime.fill_value_with(_context);
	speakBlendOutTime.fill_value_with(_context);
	speakAngles.fill_value_with(_context);
}

