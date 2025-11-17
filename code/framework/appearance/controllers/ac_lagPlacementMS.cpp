#include "ac_lagPlacementMS.h"

#include "ikUtils.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\mesh\pose.h"
#include "..\..\..\core\wheresMyPoint\iWMPTool.h"

#include "..\..\..\core\performance\performanceUtils.h"
#include "..\..\..\core\system\input.h"

#include "..\..\..\core\debug\debugRenderer.h"

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(lagPlacementMS);

//

REGISTER_FOR_FAST_CAST(LagPlacementMS);

LagPlacementMS::LagPlacementMS(LagPlacementMSData const * _data)
: base(_data)
, lagPlacementMSData(_data)
{
	placementMSVar = lagPlacementMSData->placementMSVar.get();

	refSocket.set_name(lagPlacementMSData->refSocket.get(Name::invalid()));

	useMovement = lagPlacementMSData->useMovement.get(0.8f);
	movementBlendTime = lagPlacementMSData->movementBlendTime.get(0.05f);
	lagEffect = lagPlacementMSData->lagEffect.get(0.1f);
	maxDistance = lagPlacementMSData->maxDistance.get(Vector3::zero);
	maxDistanceCoef = lagPlacementMSData->maxDistanceCoef.get(Vector3::one);
}

LagPlacementMS::~LagPlacementMS()
{
}

void LagPlacementMS::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	placementMSVar.look_up<Transform>(varStorage);

	refSocket.look_up(get_owner()->get_mesh());
}

void LagPlacementMS::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void LagPlacementMS::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(lagPlacementMS_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();
	if (!mesh || !get_active())
	{
		return;
	}

	Transform currentPlacementMS = placementMSVar.get<Transform>();

	Transform requestedPlacementMS = currentPlacementMS;

	Transform resultPlacementMS = currentPlacementMS;

	if (! has_just_activated())
	{
		currentPlacementMS = lastPlacementMS;
	}

	float const deltaTime = _context.get_delta_time();

	if (deltaTime > 0.0f)
	{
		auto* imo = get_owner()->get_owner();

		//debug_context(imo->get_presence()->get_in_room());
		//debug_push_transform(imo->get_presence()->get_placement());
		//debug_push_transform(get_owner()->get_ms_to_os_transform().inverted());

		Transform prevPlacementOffset = get_owner()->get_ms_to_os_transform().to_local(imo->get_presence()->get_prev_placement_offset());

		prevPlacementOffset = Transform::lerp(useMovement, Transform::identity, prevPlacementOffset);

		currentPlacementMS = prevPlacementOffset.to_world(currentPlacementMS);

		currentPlacementMS = Transform::lerp((1.0f - lagEffect) * (movementBlendTime != 0.0f? deltaTime / movementBlendTime : 1.0f), currentPlacementMS, requestedPlacementMS);

		if (! maxDistance.is_zero())
		{
			Vector3 distance = requestedPlacementMS.location_to_local(currentPlacementMS.get_translation());

			resultPlacementMS = currentPlacementMS;
			Vector3 shouldBeAtDistance = distance;

			for_count(int, e, 3)
			{
				float & distanceE = distance.access_element(e);
				float const & maxDistanceE = maxDistance.access_element(e);
				float const & maxDistanceCoefE = maxDistanceCoef.access_element(e);
				float & shouldBeAtDistanceE = shouldBeAtDistance.access_element(e);
				if (distanceE != 0.0f)
				{
					shouldBeAtDistanceE = sign(distanceE) * maxDistanceE * (1.0f - 1.0f / (1.0f + (abs(distanceE) / maxDistanceE) * maxDistanceCoefE));
				}
				else
				{
					resultPlacementMS = currentPlacementMS;
				}
			}
			resultPlacementMS.set_translation(requestedPlacementMS.location_to_world(shouldBeAtDistance));
		}
		else
		{
			resultPlacementMS = currentPlacementMS;
		}

		if (get_active() != 1.0f)
		{
			resultPlacementMS = Transform::lerp(get_active(), placementMSVar.get<Transform>(), resultPlacementMS);
		}

		//debug_draw_arrow(true, Colour::grey, currentPlacementMS.get_translation(), requestedPlacementMS.get_translation());
		//debug_draw_arrow(true, Colour::green, resultPlacementMS.get_translation(), currentPlacementMS.get_translation());

		//debug_pop_transform();
		//debug_pop_transform();
		//debug_no_context();
	}

	lastPlacementMS = currentPlacementMS;
	placementMSVar.access<Transform>() = resultPlacementMS;
}

void LagPlacementMS::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);
}

//

REGISTER_FOR_FAST_CAST(LagPlacementMSData);

AppearanceControllerData* LagPlacementMSData::create_data()
{
	return new LagPlacementMSData();
}

void LagPlacementMSData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(lagPlacementMS), create_data);
}

LagPlacementMSData::LagPlacementMSData()
{
}

bool LagPlacementMSData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= placementMSVar.load_from_xml(_node, TXT("placementMSVarID"));

	result &= refSocket.load_from_xml(_node, TXT("refSocket"));
	
	result &= useMovement.load_from_xml(_node, TXT("useMovement"));
	result &= movementBlendTime.load_from_xml(_node, TXT("movementBlendTime"));
	result &= lagEffect.load_from_xml(_node, TXT("lagEffect"));
	result &= maxDistance.load_from_xml(_node, TXT("maxDistance"));
	result &= maxDistanceCoef.load_from_xml(_node, TXT("maxDistanceCoef"));

	return result;
}

bool LagPlacementMSData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* LagPlacementMSData::create_copy() const
{
	LagPlacementMSData* copy = new LagPlacementMSData();
	*copy = *this;
	return copy;
}

AppearanceController* LagPlacementMSData::create_controller()
{
	return new LagPlacementMS(this);
}

void LagPlacementMSData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	placementMSVar.if_value_set([this, _rename]() { placementMSVar = _rename(placementMSVar.get(), NP); });
	refSocket.if_value_set([this, _rename]() { refSocket = _rename(refSocket.get(), NP); });
}

void LagPlacementMSData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	placementMSVar.fill_value_with(_context);
	refSocket.fill_value_with(_context);
}

void LagPlacementMSData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}
