#include "ac_elevatorCartAttacherArm.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\appearance\appearanceControllerPoseContext.h"
#include "..\..\..\framework\appearance\appearanceControllers.h"
#include "..\..\..\framework\collision\checkCollisionContext.h"
#include "..\..\..\framework\collision\checkSegmentResult.h"
#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\meshGen\meshGenValueDefImpl.inl"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleMovement.h"
#include "..\..\..\framework\object\scenery.h"

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

DEFINE_STATIC_NAME(elevatorCartAttacherArm);

//

REGISTER_FOR_FAST_CAST(ElevatorCartAttacherArm);

ElevatorCartAttacherArm::ElevatorCartAttacherArm(ElevatorCartAttacherArmData const * _data)
: base(_data)
, elevatorCartAttacherArmData(_data)
{
	rg.set_seed(this);

	attacherBone = elevatorCartAttacherArmData->attacherBone.get();
	armBone = elevatorCartAttacherArmData->armBone.get();
	attachDirRange = elevatorCartAttacherArmData->attachDirRange.is_set() ? elevatorCartAttacherArmData->attachDirRange.get() : attachDirRange;
	placementVar = elevatorCartAttacherArmData->placementVar.is_set() ? elevatorCartAttacherArmData->placementVar.get() : placementVar;
}

ElevatorCartAttacherArm::~ElevatorCartAttacherArm()
{
}

void ElevatorCartAttacherArm::initialise(Framework::ModuleAppearance* _owner)
{
	base::initialise(_owner);

	rg = _owner->get_owner()->get_individual_random_generator();
	rg.advance_seed(8903457, 9263);

	if (auto* skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		attacherBone.look_up(skeleton);
		armBone.look_up(skeleton);
	}

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	placementVar.look_up<Transform>(varStorage);

	placementWS.clear();
}

void ElevatorCartAttacherArm::advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void ElevatorCartAttacherArm::calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(elevatorCartAttacherArm_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();
	auto * imo = get_owner()->get_owner();

	if (!mesh ||
		!imo ||
		!attacherBone.is_valid() ||
		!armBone.is_valid())
	{
		return;
	}

	debug_filter(ac_elevatorCartAttacherArm);
	debug_context(imo->get_presence()->get_in_room());

	Meshes::Pose * poseLS = _context.access_pose_LS();

	if (!placementWS.is_set())
	{
		Transform attacherMS = poseLS->get_bone_ms_from_ls(attacherBone.get_index());
		Transform armMS = poseLS->get_bone_ms_from_ls(armBone.get_index());

		float maxLength = 0.0f;
		{
			// use skeleton to get default length
			Transform attacherMS = poseLS->get_skeleton()->get_bones_default_placement_MS()[attacherBone.get_index()];
			Transform armMS = poseLS->get_skeleton()->get_bones_default_placement_MS()[armBone.get_index()];
			maxLength = (attacherMS.get_translation() - armMS.get_translation()).length();
		}
		float minLength = maxLength * 0.5f;

		Transform attacherWS = get_owner()->get_ms_to_ws_transform().to_world(attacherMS);
		Transform armWS = get_owner()->get_ms_to_ws_transform().to_world(armMS);

		Vector3 tryDir = Vector3(rg.get(attachDirRange.x), rg.get(attachDirRange.y), rg.get(attachDirRange.z)).normal();

		auto* p = imo->get_presence();

		Framework::CheckCollisionContext checkCollisionContext;
		checkCollisionContext.collision_info_needed();
		checkCollisionContext.use_collision_flags(elevatorCartAttacherArmData->attachToFlags);
		checkCollisionContext.start_with_nothing_to_check();
		checkCollisionContext.check_room();
		checkCollisionContext.check_room_scenery();
		checkCollisionContext.check_scenery();
		checkCollisionContext.avoid(imo);

		Framework::CollisionTrace collisionTrace(Framework::CollisionTraceInSpace::WS);
		collisionTrace.add_location(armWS.get_translation());
		Vector3 endAtWS = armWS.get_translation() + armWS.vector_to_world(tryDir) * maxLength;
		collisionTrace.add_location(endAtWS);

		int collisionTraceFlags = Framework::CollisionTraceFlag::ResultInPresenceRoom;

		debug_draw_arrow(true, Colour::red, armWS.get_translation(), endAtWS);

		Framework::CheckSegmentResult result;
		if (p->trace_collision(AgainstCollision::Movement, collisionTrace, result, collisionTraceFlags, checkCollisionContext))
		{
			if ((result.hitLocation - armWS.get_translation()).length() >= minLength)
			{
				auto* hitImo = fast_cast<Framework::IModulesOwner>(result.object);
				bool ok = hitImo == nullptr; // room!
				if (!ok && hitImo)
				{
					debug_draw_text(true, Colour::red, result.hitLocation, NP, true, 0.1f, false, TXT("hitImo"));
					if (!hitImo->get_movement() || hitImo->get_movement()->is_immovable())
					{
						ok = true;
					}
				}
				if (ok)
				{
					Vector3 forwardAxis = attacherWS.get_axis(Axis::Forward);
					while (abs(Vector3::dot(forwardAxis, result.hitNormal)) > 0.9f)
					{
						forwardAxis = Vector3(rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f)).normal();
					}
					Transform placement;
					placement.build_from_two_axes(Axis::Up, Axis::Forward, result.hitNormal, forwardAxis, result.hitLocation);
					placementWS = placement;
					debug_draw_time_based(10000.0f, debug_draw_arrow(true, Colour::green, armWS.get_translation(), result.hitLocation));
				}
			}
			else
			{
				debug_draw_text(true, Colour::red, result.hitLocation, NP, true, 0.1f, false, TXT("%.3f < %.3f"), (result.hitLocation - armWS.get_translation()).length(), minLength);
			}
		}
	}

	Transform usePlacementWS;
	if (placementWS.is_set())
	{
		usePlacementWS = placementWS.get();
	}
	else
	{
		Transform armMS = poseLS->get_bone_ms_from_ls(armBone.get_index());

		float maxLength = 0.0f;
		{
			// use skeleton to get default length
			Transform attacherMS = poseLS->get_skeleton()->get_bones_default_placement_MS()[attacherBone.get_index()];
			Transform armMS = poseLS->get_skeleton()->get_bones_default_placement_MS()[armBone.get_index()];
			maxLength = (attacherMS.get_translation() - armMS.get_translation()).length();
		}

		Transform armWS = get_owner()->get_ms_to_ws_transform().to_world(armMS);

		// along middle dir?
		// if still not set
		usePlacementWS = get_owner()->get_ms_to_ws_transform().to_world(poseLS->get_bone_ms_from_ls(attacherBone.get_index()));
		usePlacementWS.set_translation(armWS.get_translation() + maxLength * armWS.vector_to_world(attachDirRange.centre().normal()));

		debug_draw_arrow(true, Colour::yellow, armWS.get_translation(), usePlacementWS.get_translation());
	}

	placementVar.access<Transform>() = usePlacementWS;

	debug_no_context();
	debug_no_filter();
}

void ElevatorCartAttacherArm::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (attacherBone.is_valid())
	{
		dependsOnParentBones.push_back(attacherBone.get_index());
		providesBones.push_back(attacherBone.get_index());
	}

	if (armBone.is_valid())
	{
		dependsOnParentBones.push_back(armBone.get_index());
		providesBones.push_back(armBone.get_index());
	}
}

//

REGISTER_FOR_FAST_CAST(ElevatorCartAttacherArmData);

Framework::AppearanceControllerData* ElevatorCartAttacherArmData::create_data()
{
	return new ElevatorCartAttacherArmData();
}

void ElevatorCartAttacherArmData::register_itself()
{
	Framework::AppearanceControllers::register_appearance_controller(NAME(elevatorCartAttacherArm), create_data);
}

ElevatorCartAttacherArmData::ElevatorCartAttacherArmData()
{
}

bool ElevatorCartAttacherArmData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= attacherBone.load_from_xml(_node, TXT("attacherBone"));
	result &= armBone.load_from_xml(_node, TXT("armBone"));

	result &= placementVar.load_from_xml(_node, TXT("placementVarID"));
	
	result &= attachToFlags.load_from_xml(_node, TXT("attachToFlags"));

	result &= attachDirRange.load_from_xml(_node, TXT("attachDirRange"));

	error_loading_xml_on_assert(! attachToFlags.is_empty(), result, _node, TXT("provide non empty \"attachToFlags\""));

	return result;
}

bool ElevatorCartAttacherArmData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::AppearanceControllerData* ElevatorCartAttacherArmData::create_copy() const
{
	ElevatorCartAttacherArmData* copy = new ElevatorCartAttacherArmData();
	*copy = *this;
	return copy;
}

Framework::AppearanceController* ElevatorCartAttacherArmData::create_controller()
{
	return new ElevatorCartAttacherArm(this);
}

void ElevatorCartAttacherArmData::apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	attacherBone.fill_value_with(_context);
	armBone.fill_value_with(_context);
	placementVar.fill_value_with(_context);
	attachDirRange.fill_value_with(_context);
}

void ElevatorCartAttacherArmData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

void ElevatorCartAttacherArmData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	attacherBone.if_value_set([this, _rename](){ attacherBone = _rename(attacherBone.get(), NP); });
	armBone.if_value_set([this, _rename]() { armBone = _rename(armBone.get(), NP); });
	placementVar.if_value_set([this, _rename]() { placementVar = _rename(placementVar.get(), NP); });
}
