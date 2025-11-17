#include "ac_springPlacementMS.h"

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

DEFINE_STATIC_NAME(springPlacementMS);

//

REGISTER_FOR_FAST_CAST(SpringPlacementMS);

SpringPlacementMS::SpringPlacementMS(SpringPlacementMSData const * _data)
: base(_data)
, springPlacementMSData(_data)
{
	placementMSVar = springPlacementMSData->placementMSVar.get();

	refSocket.set_name(springPlacementMSData->refSocket.get(Name::invalid()));

	useMovement = springPlacementMSData->useMovement.get(0.8f);
	movementBlendTime = springPlacementMSData->movementBlendTime.get(0.05f);
	dampEffect = springPlacementMSData->dampEffect.get(0.1f);
}

SpringPlacementMS::~SpringPlacementMS()
{
}

void SpringPlacementMS::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	placementMSVar.look_up<Transform>(varStorage);

	refSocket.look_up(get_owner()->get_mesh());

	velocityLinear = Vector3::zero;
	velocityOrientation = Rotator3::zero;
}

void SpringPlacementMS::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void SpringPlacementMS::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(springPlacementMS_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();
	if (!mesh || !get_active())
	{
		return;
	}

	Transform currentPlacementMS = placementMSVar.get<Transform>();

	Transform requestedPlacementMS = currentPlacementMS;

	if (! has_just_activated())
	{
		currentPlacementMS = lastPlacementMS;
	}
	else
	{
		velocityLinear = Vector3::zero;
		velocityOrientation = Rotator3::zero;
	}

	float const deltaTime = clamp(_context.get_delta_time(), 0.0f, 0.05f);

	if (deltaTime > 0.0f)
	{
		auto* imo = get_owner()->get_owner();

		//debug_context(imo->get_presence()->get_in_room());
		//debug_push_transform(imo->get_presence()->get_placement());
		//debug_push_transform(get_owner()->get_ms_to_os_transform().inverted());

		Transform prevPlacementOffset = get_owner()->get_ms_to_os_transform().to_local(imo->get_presence()->get_prev_placement_offset());

		prevPlacementOffset = Transform::lerp(useMovement, Transform::identity, prevPlacementOffset);

		currentPlacementMS = prevPlacementOffset.to_world(currentPlacementMS);

		Vector3 requestedVelocityLinear = requestedPlacementMS.get_translation() - currentPlacementMS.get_translation();
		Rotator3 requestedVelocityOrientation = currentPlacementMS.get_orientation().to_local(requestedPlacementMS.get_orientation()).to_rotator();
		{
			float coef = ((1.0f - dampEffect) / deltaTime);
			requestedVelocityLinear = requestedVelocityLinear * coef;
			requestedVelocityOrientation = requestedVelocityOrientation * coef;
		}

		velocityLinear = blend_to_using_time(velocityLinear, requestedVelocityLinear, movementBlendTime, deltaTime);
		velocityOrientation = blend_to_using_time(velocityOrientation, requestedVelocityOrientation, movementBlendTime, deltaTime);

		Transform applyVelocity(velocityLinear * deltaTime, (velocityOrientation * deltaTime).to_quat());

		currentPlacementMS = currentPlacementMS.to_world(applyVelocity);
		an_assert(currentPlacementMS.is_ok());

		if (get_active() != 1.0f)
		{
			currentPlacementMS = Transform::lerp(get_active(), placementMSVar.get<Transform>(), currentPlacementMS);
			an_assert(currentPlacementMS.is_ok());
		}

		//debug_draw_transform_size(true, requestedPlacementMS, 0.05f);
		//debug_draw_arrow(true, Colour::grey, currentPlacementMS.get_translation(), requestedPlacementMS.get_translation());
		//debug_draw_arrow(true, Colour::red, currentPlacementMS.get_translation(), currentPlacementMS.get_translation() + requestedVelocityLinear);
		//debug_draw_arrow(true, Colour::green, currentPlacementMS.get_translation(), currentPlacementMS.get_translation() + velocityLinear);
		//debug_draw_transform_size(true, currentPlacementMS, 0.05f);

		//debug_pop_transform();
		//debug_pop_transform();
		//debug_no_context();
	}

	an_assert(currentPlacementMS.is_ok());

	lastPlacementMS = currentPlacementMS;
	placementMSVar.access<Transform>() = currentPlacementMS;
}

void SpringPlacementMS::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);
}

//

REGISTER_FOR_FAST_CAST(SpringPlacementMSData);

AppearanceControllerData* SpringPlacementMSData::create_data()
{
	return new SpringPlacementMSData();
}

void SpringPlacementMSData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(springPlacementMS), create_data);
}

SpringPlacementMSData::SpringPlacementMSData()
{
}

bool SpringPlacementMSData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= placementMSVar.load_from_xml(_node, TXT("placementMSVarID"));

	result &= refSocket.load_from_xml(_node, TXT("refSocket"));
	
	result &= useMovement.load_from_xml(_node, TXT("useMovement"));
	result &= movementBlendTime.load_from_xml(_node, TXT("movementBlendTime"));
	result &= dampEffect.load_from_xml(_node, TXT("dampEffect"));

	return result;
}

bool SpringPlacementMSData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* SpringPlacementMSData::create_copy() const
{
	SpringPlacementMSData* copy = new SpringPlacementMSData();
	*copy = *this;
	return copy;
}

AppearanceController* SpringPlacementMSData::create_controller()
{
	return new SpringPlacementMS(this);
}

void SpringPlacementMSData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	placementMSVar.if_value_set([this, _rename]() { placementMSVar = _rename(placementMSVar.get(), NP); });
	refSocket.if_value_set([this, _rename]() { refSocket = _rename(refSocket.get(), NP); });
}

void SpringPlacementMSData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	placementMSVar	.fill_value_with(_context);
	refSocket.fill_value_with(_context);
}

void SpringPlacementMSData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}
