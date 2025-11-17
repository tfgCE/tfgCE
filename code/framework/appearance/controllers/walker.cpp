#include "walker.h"

#include "ikUtils.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"
#include "..\..\module\moduleCollision.h"
#include "..\..\module\moduleController.h"
#include "..\..\module\moduleMovement.h"
#include "..\..\module\modulePresence.h"

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

DEFINE_STATIC_NAME(walker);

//

WalkerLegData::WalkerLegData()
{
}

bool WalkerLegData::load_from_xml(IO::XML::Node const * _node, IO::XML::Node const* _mainNode, LibraryLoadingContext & _lc)
{
	bool result = true;

	result &= targetPlacementMSVar.load_from_xml(_node, TXT("targetPlacementMSVarID"));
	if (!targetPlacementMSVar.is_set())
	{
		error_loading_xml(_node, TXT("no targetPlacementMSVarID provided"));
		result = false;
	}
	result &= disableVar.load_from_xml(_node, TXT("disableVarID"));
	result &= trajectoryMSVar.load_from_xml(_node, TXT("trajectoryMSVarID"));

	result &= legSetup.load_from_xml(_node, _mainNode, _lc);

	return result;
}

bool WalkerLegData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= legSetup.prepare_for_game(_library, _pfgContext);

	return result;
}

bool WalkerLegData::bake(Walkers::Lib const & _lib)
{
	bool result = true;

	result &= legSetup.bake(_lib);

	return result;
}

void WalkerLegData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	targetPlacementMSVar.fill_value_with(_context);
	disableVar.fill_value_with(_context);
	trajectoryMSVar.fill_value_with(_context);
	legSetup.apply_mesh_gen_params(_context);
}

//

void WalkerLeg::initialise(WalkerLegData const * _data, Walker* _walker)
{
	walker = _walker;
	data = _data;

	auto & varStorage = walker->get_owner()->access_controllers_variable_storage();
	targetPlacementMSVar = data->targetPlacementMSVar.get();
	targetPlacementMSVar.look_up<Transform>(varStorage);
	if (data->disableVar.is_set())
	{
		disableVar = data->disableVar.get();
		disableVar.look_up<bool>(varStorage);
	}
	if (data->trajectoryMSVar.is_set())
	{
		trajectoryMSVar = data->trajectoryMSVar.get();
		trajectoryMSVar.look_up<Vector3>(varStorage);
	}
}

void WalkerLeg::update_vars(Walkers::LegInstance const & _legInstance)
{
	an_assert(_legInstance.get_placement_MS().get_orientation().is_normalised());
	targetPlacementMSVar.access<Transform>() = _legInstance.get_placement_MS();
	if (trajectoryMSVar.is_valid())
	{
		trajectoryMSVar.access<Vector3>() = _legInstance.get_trajectory_MS();
	}

	auto & varStorage = walker->get_owner()->access_controllers_variable_storage();
	for_every(gaitVar, _legInstance.get_gait_vars())
	{
		varStorage.access<float>(gaitVar->varID) = gaitVar->value;
	}
}

//

REGISTER_FOR_FAST_CAST(Walker);

Walker::Walker(WalkerData const * _data)
: base(_data)
, walkerData(_data)
{
}

Walker::~Walker()
{
}

void Walker::initialise(ModuleAppearance* _owner)
{
	walkerInstanceContext.set_owner(_owner);
	walkerInstanceContext.set_instance(&walkerInstance);

	base::initialise(_owner);

	legs.set_size(walkerData->legs.get_size());

	walkerInstance.setup(walkerInstanceContext, walkerData->legs.get_size(), walkerData->walkerSetup);

	WalkerLegData const * legData = walkerData->legs.begin_const();
	int legIndex = 0;
	for_every(leg, legs)
	{
		leg->initialise(legData, this);
		Walkers::LegInstance & legInstance = walkerInstance.access_leg(legIndex);
		legInstance.use_setup(walkerInstanceContext, &legData->legSetup);
		++legData;
		++legIndex;
	}

	{
		auto& varStorage = get_owner()->access_controllers_variable_storage();
		if (walkerData->shouldFallVar.is_set())
		{
			shouldFallVar = walkerData->shouldFallVar.get();
			shouldFallVar.look_up<bool>(varStorage);
		}
	}
}

void Walker::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);

	if (has_just_deactivated())
	{
		walkerInstance.stop_sounds();
	}

	if (!get_active())
	{
		return;
	}

	MEASURE_PERFORMANCE(walker_preliminaryPose);

	// plan:
	// update main "animation"
	// advance leg "animations" to keep up with main
	// check whether leg should move - this can be due to "animation" or being to far away
	// find spot for leg (or update if spot has moved too much)
	// move leg

	walkerInstanceContext.reset_for_new_frame();

	// update so anything else using MS locations before movement may use it
	update_leg_variables_input();

	if (has_just_activated())
	{
		walkerInstanceContext.mark_just_activated();
	}

	walkerInstanceContext.use_pose_LS(_context.access_pose_LS());

	// check whether we should change gait
	walkerInstanceContext.set_current_gait(walkerInstance.get_current_gait_name());

	// get preferred gravity alignment from previous frame to know if we should rotate (maybe slow down to rotate)
	walkerInstanceContext.set_preferred_gravity_alignment(walkerInstance.get_preferred_gravity_alignment());

	bool vrMovementActive = false;
	if (ModulePresence const * presence = get_owner()->get_owner()->get_presence())
	{
		if (presence->is_vr_movement())
		{
			vrMovementActive = true;
			if (!vrMovement.active)
			{
				vrMovement = VRMovement();
			}
			vrMovement.active = true;
			float deltaTime = _context.get_delta_time();
			vrMovement.timeToUpdate = max(0.0f, vrMovement.timeToUpdate - deltaTime);
			vrMovement.accumulatedInterval += deltaTime;
			if (vrMovement.timeToUpdate <= 0.0f)
			{
				// time to update not used now?
				Transform vrPlacement = presence->get_vr_anchor().to_local(presence->get_placement());
				vrMovement.vrPlacementPrev = vrMovement.vrPlacementCurr;
				vrMovement.vrPlacementCurr = vrPlacement;
				if (vrMovement.vrPlacementCurr.is_set() && vrMovement.vrPlacementPrev.is_set())
				{
					Transform relPlacement = vrMovement.vrPlacementPrev.get().to_local(vrMovement.vrPlacementCurr.get());
					if (vrMovement.accumulatedInterval > 0.0f)
					{
						vrMovement.relativeVelocityLinear = relPlacement.get_translation() / vrMovement.accumulatedInterval;
						if (relPlacement.get_translation().is_zero())
						{
							vrMovement.relativeMovementDirection.clear();
						}
						else
						{
							vrMovement.relativeMovementDirection = Rotator3::get_yaw(relPlacement.get_translation());
						}
						vrMovement.velocityOrientation = relPlacement.get_orientation().to_rotator() / vrMovement.accumulatedInterval;
					}
					else
					{
						vrMovement.relativeVelocityLinear.clear();
						vrMovement.relativeMovementDirection.clear();
						vrMovement.velocityOrientation.clear();
					}
				}
				vrMovement.accumulatedInterval = 0.0f;
			}
			walkerInstanceContext.set_relative_velocity_linear(vrMovement.relativeVelocityLinear.get(Vector3::zero));
			walkerInstanceContext.set_relative_requested_movement_direction(vrMovement.relativeMovementDirection.get(0.0f));
			walkerInstanceContext.set_velocity_orientation(vrMovement.velocityOrientation.get(Rotator3::zero));
		}
		else
		{
			Vector3 relativeVelocity = presence->get_placement().vector_to_local(presence->get_velocity_linear());
			walkerInstanceContext.set_relative_velocity_linear(relativeVelocity);
			walkerInstanceContext.set_relative_requested_movement_direction(Rotator3::get_yaw(relativeVelocity));
			walkerInstanceContext.set_velocity_orientation(presence->get_velocity_rotation());
		}
		walkerInstanceContext.set_gravity_presence_traces_touch_surroundings(presence->do_gravity_presence_traces_touch_surroundings());
	}

	if (!vrMovementActive)
	{
		vrMovement.active = false;
	}
	if (ModuleCollision const * collision = get_owner()->get_owner()->get_collision())
	{
		walkerInstanceContext.set_colliding_with_non_scenery(collision->is_colliding_with_non_scenery());
	}
	if (ModuleController const * controller = get_owner()->get_owner()->get_controller())
	{
		todo_note(TXT("basing only on requested velocity may not work as there are movement parameters that may override_ that"));
		/*
		Optional<Vector3> const & requestedVelocity = controller->get_requested_velocity_linear();
		if (requestedVelocity.is_set())
		{
			walkerInstanceContext.set_relative_velocity_linear(requestedVelocity.get());
		}
		*/
		Optional<Vector3> const & requestedMovementDirection = controller->get_relative_requested_movement_direction();
		if (requestedMovementDirection.is_set())
		{
			walkerInstanceContext.set_relative_requested_movement_direction(Rotator3::get_yaw(requestedMovementDirection.get()));
		}
		walkerInstanceContext.set_gait_name_requested_by_controller(controller->get_requested_movement_parameters().gaitName);
		todo_note(TXT("turn speed?"));
	}

	walkerInstanceContext.prepare();
	Walkers::GaitOption const & requestedGait = walkerData->chooseGait.run_for(walkerInstanceContext);
	if (walkerInstance.get_current_gait_name() != requestedGait.gait)
	{
		if (Walkers::LockGait::is_allowed(walkerInstance.get_current_gait_name(), requestedGait.gait, walkerData->lockGaits, walkerInstance, walkerInstanceContext))
		{
			walkerInstance.set_requested_gait(requestedGait.gait.is_valid() ? walkerData->lib.find_gait(requestedGait.gait) : nullptr, requestedGait);
		}
		else
		{
#ifdef AN_ALLOW_EXTENDED_DEBUG
			IF_EXTENDED_DEBUG(WalkersLegMovementContinuous)
			{
				output(TXT("can't switch gait \"%S\" to \"%S\" - locked"), walkerInstance.get_current_gait_name().to_char(), requestedGait.gait.to_char());
			}
#endif
		}
	}
	if (ModuleController * controller = get_owner()->get_owner()->get_controller())
	{
		Name provideAllowedGait = walkerData->provideAllowedGait.get_provided(requestedGait.gait);
		if (provideAllowedGait.is_valid())
		{
			controller->set_appearace_allowed_gait(provideAllowedGait);
		}
	}

	// advance gait instance
	walkerInstance.advance(walkerInstanceContext, _context.get_delta_time());

	// update so anything else using MS locations before movement may use it
	update_leg_variables(false);

	update_gait_vars(requestedGait.gait);

	if (shouldFallVar.is_valid())
	{
		shouldFallVar.access<bool>() = walkerInstance.should_fall() && get_owner()->get_owner()->get_presence()->does_allow_falling();
	}
}

void Walker::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	if (!get_active())
	{
		return;
	}

	MEASURE_PERFORMANCE(walker_finalPose);

	// because for final pose we've already moved, we need to recalculate MS placements
	update_leg_variables(true);
}

void Walker::update_gait_vars(Name const & _gaitName)
{
	if (walkerData->gaitVars.is_empty())
	{
		return;
	}

	auto & varStorage = get_owner()->access_controllers_variable_storage();

	for_every(gaitVar, walkerData->gaitVars)
	{
		varStorage.access<float>(gaitVar->get_var_id()) = gaitVar->get_value(_gaitName);
	}
}

void Walker::update_leg_variables_input()
{
	int legIndex = 0;
	for_every(leg, legs)
	{
		Walkers::LegInstance& legInstance = walkerInstance.access_leg(legIndex);
		if (leg->disableVar.is_valid())
		{
			legInstance.request_active(!leg->disableVar.get<bool>());
		}

		++legIndex;
	}
}

void Walker::update_leg_variables(bool _recalculateLegPlacements)
{
	// update variables associated with legs
	int legIndex = 0;
	for_every(leg, legs)
	{
		Walkers::LegInstance & legInstance = walkerInstance.access_leg(legIndex);
		// update MS position as we could move!
		if (_recalculateLegPlacements)
		{
			legInstance.calculate_leg_placements_MS(walkerInstanceContext);
		}
		leg->update_vars(legInstance);
		++legIndex;
	}
}

void Walker::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	providesControllers.push_back(NAME(walker));

	for_every(leg, legs)
	{
		if (leg->targetPlacementMSVar.is_valid())
		{
			providesVariables.push_back(leg->targetPlacementMSVar.get_name());
		}
	}
}

//

REGISTER_FOR_FAST_CAST(WalkerData);

AppearanceControllerData* WalkerData::create_data()
{
	return new WalkerData();
}

void WalkerData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(walker), create_data);
}

WalkerData::WalkerData()
{
}

bool WalkerData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	if (IO::XML::Node const * container = _node->first_child_named(TXT("legs")))
	{
		for_every(legNode, container->children_named(TXT("leg")))
		{
			WalkerLegData walkLegData;
			if (walkLegData.load_from_xml(legNode, _node, _lc))
			{
				legs.push_back(walkLegData);
			}
			else
			{
				error_loading_xml(legNode, TXT("error loading leg"));
				result = false;
			}
		}
	}

	if (IO::XML::Node const * container = _node->first_child_named(TXT("walkerLib")))
	{
		result &= lib.load_from_xml(container, _lc);
	}
	result &= walkerSetup.load_from_xml(_node, _lc);
	result &= chooseGait.load_from_xml(_node->first_child_named(TXT("chooseGait")), _lc);
	result &= provideAllowedGait.load_from_xml(_node->first_child_named(TXT("provideAllowedGait")), _lc);

	for_every(node, _node->children_named(TXT("gaitVars")))
	{
		for_every(varNode, node->children_named(TXT("var")))
		{
			Walkers::GaitVar gv;
			if (gv.load_from_xml(varNode, _lc))
			{
				gaitVars.push_back(gv);
			}
			else
			{
				error_loading_xml(varNode, TXT("problem loading gaitVar"));
				result = false;
			}
		}
	}

	for_every(node, _node->children_named(TXT("lockGaits")))
	{
		for_every(lockNode, node->children_named(TXT("lock")))
		{
			Walkers::LockGait lg;
			if (lg.load_from_xml(lockNode, _lc))
			{
				lockGaits.push_back(lg);
			}
			else
			{
				error_loading_xml(lockNode, TXT("problem loading lg"));
				result = false;
			}
		}
	}

	result &= shouldFallVar.load_from_xml(_node, TXT("shouldFallVarID"));

	return result;
}

bool WalkerData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(leg, legs)
	{
		result &= leg->prepare_for_game(_library, _pfgContext);
	}

	result &= lib.prepare_for_game(_library, _pfgContext);
	result &= walkerSetup.prepare_for_game(_library, _pfgContext);
	result &= chooseGait.prepare_for_game(_library, _pfgContext);
	result &= provideAllowedGait.prepare_for_game(_library, _pfgContext);

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::BakeWalkersGaits)
	{
		result &= lib.bake();
		for_every(leg, legs)
		{
			result &= leg->bake(lib);
		}
	}

	return result;
}

AppearanceControllerData* WalkerData::create_copy() const
{
	WalkerData* copy = new WalkerData();
	*copy = *this;
	return copy;
}

AppearanceController* WalkerData::create_controller()
{
	return new Walker(this);
}

void WalkerData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	walkerSetup.apply_mesh_gen_params(_context);
	
	for_every(leg, legs)
	{
		leg->apply_mesh_gen_params(_context);
	}

	shouldFallVar.fill_value_with(_context);
}
