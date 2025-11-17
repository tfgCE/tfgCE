#include "ac_providePlacementFromOtherIMOSocketMS.h"

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

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(providePlacementFromOtherIMOSocketMS);

//

REGISTER_FOR_FAST_CAST(ProvidePlacementFromOtherIMOSocketMS);

ProvidePlacementFromOtherIMOSocketMS::ProvidePlacementFromOtherIMOSocketMS(ProvidePlacementFromOtherIMOSocketMSData const * _data)
: base(_data)
{
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(otherIMOVar);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(socketVar);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA_OPTIONAL(offsetVar);
	GET_VALUE_FROM_APPEARANCE_CONTROLLER_DATA(resultPlacementMSVar);
}

ProvidePlacementFromOtherIMOSocketMS::~ProvidePlacementFromOtherIMOSocketMS()
{
}

void ProvidePlacementFromOtherIMOSocketMS::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	otherIMOVar.look_up<SafePtr<IModulesOwner>>(varStorage);
	socketVar.look_up<Name>(varStorage);
	offsetVar.look_up<Transform>(varStorage);
	resultPlacementMSVar.look_up<Transform>(varStorage);
}

void ProvidePlacementFromOtherIMOSocketMS::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void ProvidePlacementFromOtherIMOSocketMS::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(providePlacementFromOtherIMOSocketMS_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();
	if (!mesh)
	{
		return;
	}

	debug_context(this->get_owner()->get_owner()->get_presence()->get_in_room());

	auto& otherIMOPtr = otherIMOVar.get<SafePtr<IModulesOwner>>();
	if (auto* otherIMO = otherIMOPtr.get())
	{
		if (auto* thisIMO = get_owner()->get_owner())
		{
			if (otherIMO->get_presence()->get_in_room() == thisIMO->get_presence()->get_in_room())
			{
				Transform placementWS = otherIMO->get_presence()->get_placement();
				if (auto* oap = otherIMO->get_appearance())
				{
					auto& otherSocket = socketVar.get<Name>();
					if (otherSocket.is_valid())
					{
						Transform otherSocketOS = oap->calculate_socket_os(socketVar.get<Name>());
						debug_push_transform(placementWS);
						debug_draw_arrow(true, Colour::orange, placementWS.get_translation(), placementWS.to_world(otherSocketOS).get_translation());
						debug_pop_transform();
						placementWS = placementWS.to_world(otherSocketOS);
					}
				}

				if (offsetVar.is_valid())
				{
					debug_push_transform(Transform::identity); //ws
					debug_draw_arrow(true, Colour::red, placementWS.get_translation(), placementWS.to_world(offsetVar.get<Transform>()).get_translation());
					debug_pop_transform();
					placementWS = placementWS.to_world(offsetVar.get<Transform>());
				}

				Transform otherThisMS = get_owner()->get_ms_to_ws_transform().to_local(placementWS);

				debug_push_transform(this->get_owner()->get_owner()->get_presence()->get_placement());
				debug_draw_transform(true, otherThisMS);
				debug_pop_transform();

				resultPlacementMSVar.access<Transform>() = otherThisMS;
			}
			else
			{
				warn_dev_investigate(TXT("both objects should be in the same room!"));
			}
		}
	}

	debug_no_context();

}

void ProvidePlacementFromOtherIMOSocketMS::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);
}

//

REGISTER_FOR_FAST_CAST(ProvidePlacementFromOtherIMOSocketMSData);

AppearanceControllerData* ProvidePlacementFromOtherIMOSocketMSData::create_data()
{
	return new ProvidePlacementFromOtherIMOSocketMSData();
}

void ProvidePlacementFromOtherIMOSocketMSData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(providePlacementFromOtherIMOSocketMS), create_data);
}

ProvidePlacementFromOtherIMOSocketMSData::ProvidePlacementFromOtherIMOSocketMSData()
{
}

bool ProvidePlacementFromOtherIMOSocketMSData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= otherIMOVar.load_from_xml(_node, TXT("otherIMOVarID"));
	result &= socketVar.load_from_xml(_node, TXT("socketVarID"));
	result &= offsetVar.load_from_xml(_node, TXT("offsetVarID"));
	result &= resultPlacementMSVar.load_from_xml(_node, TXT("resultPlacementMSVarID"));
	result &= resultPlacementMSVar.load_from_xml(_node, TXT("outPlacementMSVarID"));
	return result;
}

bool ProvidePlacementFromOtherIMOSocketMSData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* ProvidePlacementFromOtherIMOSocketMSData::create_copy() const
{
	ProvidePlacementFromOtherIMOSocketMSData* copy = new ProvidePlacementFromOtherIMOSocketMSData();
	*copy = *this;
	return copy;
}

AppearanceController* ProvidePlacementFromOtherIMOSocketMSData::create_controller()
{
	return new ProvidePlacementFromOtherIMOSocketMS(this);
}

void ProvidePlacementFromOtherIMOSocketMSData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	otherIMOVar.if_value_set([this, _rename]() { otherIMOVar = _rename(otherIMOVar.get(), NP); });
	socketVar.if_value_set([this, _rename]() { socketVar = _rename(socketVar.get(), NP); });
	offsetVar.if_value_set([this, _rename]() { offsetVar = _rename(offsetVar.get(), NP); });
	resultPlacementMSVar.if_value_set([this, _rename]() { resultPlacementMSVar = _rename(resultPlacementMSVar.get(), NP); });
}

void ProvidePlacementFromOtherIMOSocketMSData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	otherIMOVar.fill_value_with(_context);
	socketVar.fill_value_with(_context);
	offsetVar.fill_value_with(_context);
	resultPlacementMSVar.fill_value_with(_context);
}

void ProvidePlacementFromOtherIMOSocketMSData::apply_transform(Matrix44 const& _transform)
{
	base::apply_transform(_transform);
}
