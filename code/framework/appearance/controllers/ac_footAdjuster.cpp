#include "ac_footAdjuster.h"

#include "ikUtils.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"
#include "..\..\module\modulePresence.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(footAdjuster);

//

REGISTER_FOR_FAST_CAST(FootAdjuster);

FootAdjuster::FootAdjuster(FootAdjusterData const * _data)
: base(_data)
, footAdjusterData(_data)
, isValid(false)
{
	footBone = footAdjusterData->footBone.get();
	plantStateVar = footAdjusterData->plantStateVar.get();
	trajectoryMSVar = footAdjusterData->trajectoryMSVar.get();
	targetPlacementMSVar = footAdjusterData->targetPlacementMSVar.get();
	adjustedTargetPlacementMSVar = footAdjusterData->adjustedTargetPlacementMSVar.get();
	footSize = footAdjusterData->footSize.is_set() ? footAdjusterData->footSize.get() : footSize;
	rotationLimits = footAdjusterData->rotationLimits.get();
	zeroLengthRotation = footAdjusterData->zeroLengthRotation.get();
	fullRotationLimitsTrajectoryLength = footAdjusterData->fullRotationLimitsTrajectoryLength.get();
}

FootAdjuster::~FootAdjuster()
{
}

void FootAdjuster::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	isValid = true;

	if (Meshes::Skeleton const * skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		// build chain
		footBone.look_up(skeleton);
		isValid &= footBone.is_valid();
	}
	else
	{
		isValid = false;
	}

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	plantStateVar.look_up<float>(varStorage);
	trajectoryMSVar.look_up<Vector3>(varStorage);
	targetPlacementMSVar.look_up<Transform>(varStorage);
	adjustedTargetPlacementMSVar.look_up<Transform>(varStorage);

	footToBaseOffset = Transform::identity;
	footSize = footAdjusterData->footSize.is_set() ? footAdjusterData->footSize.get() : footSize;
	if (isValid && get_owner()->get_skeleton())
	{
		Transform footBasePlacementMS = Transform::identity;
		// foot base
		if (footAdjusterData->footBaseSocket.is_set() && 
			footAdjusterData->footBaseSocket.get().is_valid())
		{
			Name footBaseSocket = footAdjusterData->footBaseSocket.get();
			if (footBaseSocket.is_valid())
			{
				footBasePlacementMS = get_owner()->calculate_socket_ms(footBaseSocket, get_owner()->get_skeleton()->get_skeleton()->get_default_pose_ls());
				Transform footBonePlacementMS = get_owner()->get_skeleton()->get_skeleton()->get_bones_default_placement_MS()[footBone.get_index()];
				footToBaseOffset = footBasePlacementMS.to_local(footBonePlacementMS);
			}
		}
		else
		{
			error(TXT("foot base socket invalid or missing"));
			isValid = false;
		}

		for_every(footTipSocket, footAdjusterData->footTipSockets)
		{
			if (footTipSocket->is_set() &&
				footTipSocket->get().is_valid())
			{
				Transform footTipPlacementMS = get_owner()->calculate_socket_ms(footTipSocket->get(), get_owner()->get_skeleton()->get_skeleton()->get_default_pose_ls());
				footSize.include(footTipPlacementMS.get_translation().to_vector2());
			}
		}
	}
	else
	{
		isValid = false;
	}
}

void FootAdjuster::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void FootAdjuster::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	if (!isValid)
	{
		return;
	}

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(footAdjuster_finalPose);
#endif

	debug_filter(ac_footAdjuster);
	debug_context(get_owner()->get_owner()->get_presence()->get_in_room());
	debug_subject(get_owner()->get_owner());
	debug_push_transform(get_owner()->get_owner()->get_presence()->get_placement());
	debug_push_transform(get_owner()->get_owner()->get_appearance()->get_ms_to_os_transform().inverted());

	float active = get_active();

	Transform const targetPlacementMS = targetPlacementMSVar.get<Transform>();
	an_assert_info(targetPlacementMS.get_scale() > 0.0f, TXT("target placement MS scale is zero! for \"%S\""), targetPlacementMSVar.get_name().to_char());

	Transform adjustedTargetPlacementMS = targetPlacementMS;

	if (active > 0.0f && adjustedTargetPlacementMS.get_scale() > 0.0f)
	{
		// get foot state + foot movement trajectory to learn in which direction we should rotate
		float plantState = clamp(plantStateVar.get<float>(), -1.0f, 1.0f);
		Vector3 providedFootMovementTrajectory = trajectoryMSVar.get<Vector3>();
		Vector2 newFootMovementTrajectory = targetPlacementMS.vector_to_local(providedFootMovementTrajectory).to_vector2();
		if (lastRotationNonZero)
		{
			footMovementTrajectory = blend_to_using_time(footMovementTrajectory, newFootMovementTrajectory, 0.1f, _context.get_delta_time());
		}
		else
		{
			footMovementTrajectory = newFootMovementTrajectory;
		}
		Vector2 footMovementTrajectoryDir = footMovementTrajectory.normal();

		Rotator3 rotateFoot = Rotator3::zero;
		rotateFoot.pitch = -plantState * footMovementTrajectoryDir.y; // going forward should rotate downwards
		rotateFoot.roll = plantState * footMovementTrajectoryDir.x; // going right should rotate clockwise

		float useRotationLimits = 1.0f;
		if (fullRotationLimitsTrajectoryLength != 0.0f)
		{
			useRotationLimits = clamp(footMovementTrajectory.length() / fullRotationLimitsTrajectoryLength, 0.0f, 1.0f);
		}

		// adjust rotation by limits
		rotateFoot.pitch = rotateFoot.pitch * (rotateFoot.pitch >= 0.0f ? rotationLimits.y.max : -rotationLimits.y.min) * useRotationLimits + (1.0f - useRotationLimits) * zeroLengthRotation.y * abs(plantState);
		rotateFoot.roll = rotateFoot.roll * (rotateFoot.roll >= 0.0f ? rotationLimits.x.max : -rotationLimits.x.min) * useRotationLimits + (1.0f - useRotationLimits) * zeroLengthRotation.x * abs(plantState);

		rotateFoot *= active;

		/*
		 * if we pitch downward, we need to use y.max distance to keep that end of foot on the ground
		 * therefore we pick it up by vector rotated:
		 *
		 *						 adjusted base
		 *							*--..
		 *								 ''--..
		 *	------------------base-+-----------''o-tip--------
		 *							''--..
		 *								  ''--.*
		 *								  rotated tip
		 *
		 * Or as you can see, we can use movement dir, force clamp it foot size (tip!), rotate it and use <tip> - <rotated tip> to turn <base> into <adjusted base>
		 */
		Vector2 tip = footMovementTrajectoryDir.normal_in_square();
		todo_note(TXT("tip * 2.0f and then clamp to -1,1?"));
		tip.x = abs(tip.x) * (rotateFoot.roll >= 0.0f ? footSize.x.max : footSize.x.min);
		tip.y = abs(tip.y) * (rotateFoot.pitch <= 0.0f ? footSize.y.max : footSize.y.min);

		// use foot size (+rotation) to learn how much do we need to offset in terms of location
		Vector3 tip3 = tip.to_vector3();
		Quat rotateFootQuat = rotateFoot.to_quat();
		Vector3 rotatedTip3 = rotateFootQuat.rotate(tip3);

		debug_draw_arrow(true, Colour::orange, adjustedTargetPlacementMS.get_translation(), adjustedTargetPlacementMS.get_translation() + providedFootMovementTrajectory);
		debug_draw_arrow(true, Colour::blue, adjustedTargetPlacementMS.get_translation(), adjustedTargetPlacementMS.location_to_world(tip3));
		debug_draw_arrow(true, Colour::green, adjustedTargetPlacementMS.get_translation(), adjustedTargetPlacementMS.location_to_world(rotatedTip3));

		Transform rotateAndOffsetBase(tip3 - rotatedTip3, rotateFootQuat); // offset due to rotation
		adjustedTargetPlacementMS = adjustedTargetPlacementMS.to_world(rotateAndOffsetBase);

		// store for next frame
		lastRotationNonZero = rotateFoot.length_squared() > 0.1f;
	}

	// offset by <foot relative to base>
	adjustedTargetPlacementMS = adjustedTargetPlacementMS.to_world(footToBaseOffset);

	// store adjusted
	adjustedTargetPlacementMSVar.access<Transform>() = adjustedTargetPlacementMS;

	debug_pop_transform();
	debug_pop_transform();
	debug_no_subject();
	debug_no_context();
	debug_no_filter();
}

void FootAdjuster::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	dependsOnBones.push_back(footBone.get_index());
	providesBones.push_back(footBone.get_index());

	if (plantStateVar.is_valid())
	{
		dependsOnVariables.push_back(plantStateVar.get_name());
	}
	if (trajectoryMSVar.is_valid())
	{
		dependsOnVariables.push_back(trajectoryMSVar.get_name());
	}
	if (targetPlacementMSVar.is_valid())
	{
		dependsOnVariables.push_back(targetPlacementMSVar.get_name());
	}
	if (adjustedTargetPlacementMSVar.is_valid())
	{
		dependsOnVariables.push_back(adjustedTargetPlacementMSVar.get_name());
	}
}

//

REGISTER_FOR_FAST_CAST(FootAdjusterData);

AppearanceControllerData* FootAdjusterData::create_data()
{
	return new FootAdjusterData();
}

void FootAdjusterData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(footAdjuster), create_data);
}

FootAdjusterData::FootAdjusterData()
{
}

bool FootAdjusterData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= footBone.load_from_xml(_node, TXT("footBone"));
	result &= footBaseSocket.load_from_xml(_node, TXT("footBaseSocket"));
	result &= footSize.load_from_xml(_node, TXT("footSize"));
	for_every(node, _node->children_named(TXT("footTip")))
	{
		MeshGenParam<Name> footTipSocket;
		result &= footTipSocket.load_from_xml(node, TXT("socket"));
		footTipSockets.push_back(footTipSocket);
	}

	result &= plantStateVar.load_from_xml(_node, TXT("plantStateVarID"));
	result &= trajectoryMSVar.load_from_xml(_node, TXT("trajectoryMSVarID"));
	result &= targetPlacementMSVar.load_from_xml(_node, TXT("targetPlacementMSVarID"));
	result &= adjustedTargetPlacementMSVar.load_from_xml(_node, TXT("adjustedTargetPlacementMSVarID"));
	
	result &= rotationLimits.load_from_xml(_node, TXT("rotationLimits"));
	result &= zeroLengthRotation.load_from_xml(_node, TXT("zeroLengthRotation"));
	result &= fullRotationLimitsTrajectoryLength.load_from_xml(_node, TXT("fullRotationLimitsTrajectoryLength"));

	return result;
}

bool FootAdjusterData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* FootAdjusterData::create_copy() const
{
	FootAdjusterData* copy = new FootAdjusterData();
	*copy = *this;
	return copy;
}

AppearanceController* FootAdjusterData::create_controller()
{
	return new FootAdjuster(this);
}

void FootAdjusterData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	footBone.if_value_set([this, _rename](){ footBone = _rename(footBone.get(), NP); });
	footBaseSocket.if_value_set([this, _rename](){ footBaseSocket = _rename(footBaseSocket.get(), NP); });
	for_every(footTipSocket, footTipSockets)
	{
		footTipSocket->if_value_set([_rename, &footTipSocket](){ *footTipSocket = _rename(footTipSocket->get(), NP); });
	}
	plantStateVar.if_value_set([this, _rename](){ plantStateVar = _rename(plantStateVar.get(), NP); });
	trajectoryMSVar.if_value_set([this, _rename](){ trajectoryMSVar = _rename(trajectoryMSVar.get(), NP); });
	targetPlacementMSVar.if_value_set([this, _rename](){ targetPlacementMSVar = _rename(targetPlacementMSVar.get(), NP); });
	adjustedTargetPlacementMSVar.if_value_set([this, _rename](){ adjustedTargetPlacementMSVar = _rename(adjustedTargetPlacementMSVar.get(), NP); });
}

void FootAdjusterData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

void FootAdjusterData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	footBone.fill_value_with(_context);
	footBaseSocket.fill_value_with(_context);
	footSize.fill_value_with(_context);
	for_every(footTipSocket, footTipSockets)
	{
		footTipSocket->fill_value_with(_context);
	}
	plantStateVar.fill_value_with(_context);
	trajectoryMSVar.fill_value_with(_context);
	targetPlacementMSVar.fill_value_with(_context);
	adjustedTargetPlacementMSVar.fill_value_with(_context);
	rotationLimits.fill_value_with(_context);
	zeroLengthRotation.fill_value_with(_context);
	fullRotationLimitsTrajectoryLength.fill_value_with(_context);
}

