#include "ac_restPointPlacement.h"

#include "..\..\..\framework\appearance\appearanceControllerPoseContext.h"
#include "..\..\..\framework\appearance\appearanceControllers.h"

#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleSound.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\mesh\pose.h"
#include "..\..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(RestPointPlacement);

//

REGISTER_FOR_FAST_CAST(RestPointPlacement);

RestPointPlacement::RestPointPlacement(RestPointPlacementData const * _data)
: base(_data)
, restPointPlacementData(_data)
{
	handHoldPointSocket = restPointPlacementData->handHoldPointSocket.get();
	restHoldPointSocket = restPointPlacementData->restHoldPointSocket.get();
	restPointSocket = restPointPlacementData->restPointSocket.get();
	restPointIdleSocket = restPointPlacementData->restPointIdleSocket.get();
	handObjectVar = restPointPlacementData->handObjectVar.get();
	equipmentObjectVar = restPointPlacementData->equipmentObjectVar.get();
	equipmentHoldPointSocket = restPointPlacementData->equipmentHoldPointSocket.get();
	equipmentRestPointSocket = restPointPlacementData->equipmentRestPointSocket.get();
	restPointPlacementMSVar = restPointPlacementData->restPointPlacementMSVar.get();
	restTVar = restPointPlacementData->restTVar.get();
	restEmptyVar = restPointPlacementData->restEmptyVar.get();

	atPortRestT = restPointPlacementData->atPortRestT.get(0.2f);
	atPortHoldT = restPointPlacementData->atPortHoldT.get(0.2f);
	atPortRestOffset = restPointPlacementData->atPortRestOffset.get(Vector3(0.0f, 0.0f, 0.01f));
	atPortHoldOffset = restPointPlacementData->atPortHoldOffset.get(Vector3(0.0f, 0.0f, 0.01f));
	atPortHoldEntryDir = restPointPlacementData->atPortHoldEntryDir.get(Vector3(0.0f, 0.01f, 0.01f));

	atPortHoldT = min(atPortHoldT, 1.0f - atPortRestT);
}

RestPointPlacement::~RestPointPlacement()
{
}

void RestPointPlacement::initialise(Framework::ModuleAppearance* _owner)
{
	base::initialise(_owner);

	// don't look up handHoldPointSocket as it is in hand object!
	restHoldPointSocket.look_up(get_owner()->get_mesh());
	restPointSocket.look_up(get_owner()->get_mesh());
	restPointIdleSocket.look_up(get_owner()->get_mesh());

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	handObjectVar.look_up<SafePtr<Framework::IModulesOwner>>(varStorage);
	equipmentObjectVar.look_up<SafePtr<Framework::IModulesOwner>>(varStorage);
	// don't look up equipmentHoldPointSocket nor equipmentRestPointSocket as it is in equipment object!
	restPointPlacementMSVar.look_up<Transform>(varStorage);
	restTVar.look_up<float>(varStorage);
	restEmptyVar.look_up<float>(varStorage);
}

void RestPointPlacement::advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void RestPointPlacement::calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(restPointPlacement_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh)
	{
		return;
	}

	auto* hand = handObjectVar.access<SafePtr<Framework::IModulesOwner>>().get();
	auto* equipment = equipmentObjectVar.access<SafePtr<Framework::IModulesOwner>>().get();
	if (hand)
	{
		auto* imo = get_owner()->get_owner();
		auto* presence = imo->get_presence();
		auto* handPresence = hand->get_presence();
		if (presence->get_attached_to() &&
			presence->get_in_room() && handPresence->get_in_room() &&
			handPresence->get_attached_to() == presence->get_attached_to())
		{
			debug_context(presence->get_attached_to()->get_presence()->get_in_room());

			auto* appearance = imo->get_appearance();
			auto* handAppearance = hand->get_appearance();
			auto* equipmentAppearance = equipment? equipment->get_appearance() : nullptr;
			auto* handMesh = handAppearance->get_mesh();
			auto* equipmentMesh = equipmentAppearance? equipmentAppearance->get_mesh() : nullptr;

			// update sockets
			if (!handHoldPointSocket.is_valid() || hand != lastHand ||
				handMesh != lastHandMesh)
			{
				handHoldPointSocket.look_up(handAppearance->get_mesh());
			}

			if (equipmentAppearance)
			{
				if (!equipmentHoldPointSocket.is_valid() || equipment != lastEquipment ||
					equipmentMesh != lastEquipmentMesh)
				{
					equipmentHoldPointSocket.look_up(equipmentAppearance->get_mesh());
				}
				if (!equipmentRestPointSocket.is_valid() || equipment != lastEquipment ||
					equipmentMesh != lastEquipmentMesh)
				{
					equipmentRestPointSocket.look_up(equipmentAppearance->get_mesh());
				}
			}

			if (true)
			{
				// in owner space!
				Transform handOWS = handPresence->get_path_to_attached_to()->from_target_to_owner(handPresence->get_placement());
				Transform restOWS = presence->get_path_to_attached_to()->from_target_to_owner(presence->get_placement());

				// get actual hold points
				Transform handHoldPointOWS = handOWS.to_world(handAppearance->calculate_socket_os(handHoldPointSocket.get_index()));
				Transform restHoldPointOWS = restOWS.to_world(appearance->calculate_socket_os(restHoldPointSocket.get_index()));

				//debug_draw_arrow(true, Colour::red, handOWS.get_translation(), handHoldPointOWS.get_translation());
				//debug_draw_arrow(true, Colour::green, restOWS.get_translation(), restHoldPointOWS.get_translation());

				//debug_draw_transform_size(true, handHoldPointOWS, 0.1f);
				//debug_draw_transform_size(true, restHoldPointOWS, 0.1f);

				// where relatively to hold point are we
				if (equipmentAppearance &&
					equipmentHoldPointSocket.is_valid() &&
					equipmentRestPointSocket.is_valid())
				{
					holdToRestPointInEquipment = equipmentAppearance->calculate_socket_os(equipmentHoldPointSocket.get_index()).to_local(equipmentAppearance->calculate_socket_os(equipmentRestPointSocket.get_index()));
				}

				// get rest points at hold (1) and rest (0)
				Transform restPoint1OWS = handHoldPointOWS.to_world(holdToRestPointInEquipment);

				// get actual rest point loc (in owner space!)

				// do it in segments
				float t = restTVar.get<float>();
				Transform restPointOccupiedOWS;
				{	// occupied
					Transform restPoint0OWS = restHoldPointOWS.to_world(holdToRestPointInEquipment);

					Transform restPoint1EntryOWS = restPoint1OWS;
					Transform restPoint0EntryOWS = restPoint0OWS;

					Vector3 restPoint1EntryDirOWS = handHoldPointOWS.vector_to_world(atPortRestOffset);
					Vector3 restPoint0EntryDirOWS = restHoldPointOWS.vector_to_world(atPortHoldOffset);

					restPoint1EntryOWS.set_translation(restPoint1EntryOWS.get_translation() + restPoint1EntryDirOWS);
					restPoint0EntryOWS.set_translation(restPoint0EntryOWS.get_translation() + restPoint0EntryDirOWS);

					if (t <= atPortRestT)
					{
						restPointOccupiedOWS = Transform::lerp(clamp(t / atPortRestT, 0.0f, 1.0f), restPoint0OWS, restPoint0EntryOWS);
					}
					else if (t >= 1.0f - atPortHoldT)
					{
						restPointOccupiedOWS = Transform::lerp(clamp((1.0f - t) / atPortHoldT, 0.0f, 1.0f), restPoint1OWS, restPoint1EntryOWS);
					}
					else
					{
						float tu = clamp((t - atPortRestT) / (1.0f - (atPortHoldT + atPortRestT)), 0.0f, 1.0f);
						restPointOccupiedOWS = Transform::lerp(tu, restPoint0EntryOWS, restPoint1EntryOWS);

						Vector3 restPoint1EntryDirOWS = handHoldPointOWS.vector_to_world(atPortHoldEntryDir);

						BezierCurve<Vector3> curve;
						curve.p0 = restPoint0EntryOWS.get_translation();
						curve.p3 = restPoint1EntryOWS.get_translation();
						curve.p1 = curve.p0 + restPoint0EntryDirOWS * 6.0f;
						curve.p2 = curve.p3 + restPoint1EntryDirOWS * 6.0f;

						/*
						debug_draw_arrow(true, Colour::blue, curve.p0, curve.p1);
						debug_draw_arrow(true, Colour::blue, curve.p3, curve.p2);
						{
							float p = 0.0f;
							for (float a = 0.0f; a <= 1.0f; a += 0.01f)
							{
								debug_draw_line(true, Colour::blue, curve.calculate_at(p), curve.calculate_at(a));
								p = a;

							}
						}
						*/

						restPointOccupiedOWS.set_translation(curve.calculate_at(tu));
					}
				}
				Transform restPointEmptyOWS;
				{	// empty
					Transform restPoint0OWS = restOWS.to_world(appearance->calculate_socket_os(restPointIdleSocket.get_index()));

					Transform restPoint1EntryOWS = restPoint1OWS;

					float const restPointMoveBack = 0.01f;
					Vector3 restPoint1EntryDirOWS = restPoint1OWS.vector_to_world(Vector3(0.0f, -restPointMoveBack, 0.0f));
					Vector3 restPoint0EntryDirOWS = restPoint0OWS.vector_to_world(Vector3(0.0f, 0.0f, restPointMoveBack));

					restPoint1EntryOWS.set_translation(restPoint1EntryOWS.get_translation() + restPoint1EntryDirOWS);

					float atGet = atPortHoldT;
					if (t >= 1.0f - atGet)
					{
						restPointEmptyOWS = Transform::lerp(clamp((1.0f - t) / atGet, 0.0f, 1.0f), restPoint1OWS, restPoint1EntryOWS);
					}
					else
					{
						float tu = clamp(t / (1.0f - atGet), 0.0f, 1.0f);
						restPointEmptyOWS = Transform::lerp(tu, restPoint0OWS, restPoint1EntryOWS);

						BezierCurve<Vector3> curve;
						curve.p0 = restPoint0OWS.get_translation();
						curve.p3 = restPoint1EntryOWS.get_translation();
						curve.p1 = curve.p0 + restPoint0EntryDirOWS * 6.0f;
						curve.p2 = curve.p3 + restPoint1EntryDirOWS * 6.0f;

						restPointEmptyOWS.set_translation(curve.calculate_at(tu));
					}
				}

				float restEmpty = restEmptyVar.get<float>();
				Transform restPointOWS = Transform::lerp(restEmpty, restPointOccupiedOWS, restPointEmptyOWS);

				// get into object space
				Transform restPointOS = restOWS.to_local(restPointOWS);

				// and provide in model space
				restPointPlacementMSVar.access<Transform>() = appearance->get_ms_to_os_transform().to_local(restPointOS);
			}
			else if (restHoldPointSocket.is_valid())
			{
				// more for testing, just if main equipment is not a port equipment
				Transform restPointOS = appearance->calculate_socket_os(restHoldPointSocket.get_index());
				restPointPlacementMSVar.access<Transform>() = appearance->get_ms_to_os_transform().to_local(restPointOS);
			}
			else
			{
				// fallback
				restPointPlacementMSVar.access<Transform>() = Transform::identity;
			}
			debug_no_context();

			lastHandMesh = handMesh;
			lastEquipmentMesh = equipmentMesh;
		}
	}

	lastHand = hand;
	lastEquipment = equipment;
}

void RestPointPlacement::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (handObjectVar.is_valid())
	{
		dependsOnVariables.push_back(handObjectVar.get_name());
	}
	if (equipmentObjectVar.is_valid())
	{
		dependsOnVariables.push_back(equipmentObjectVar.get_name());
	}	
	if (restTVar.is_valid())
	{
		dependsOnVariables.push_back(restTVar.get_name());
	}
	if (restEmptyVar.is_valid())
	{
		dependsOnVariables.push_back(restEmptyVar.get_name());
	}	
	if (restPointPlacementMSVar.is_valid())
	{
		providesVariables.push_back(restPointPlacementMSVar.get_name());
	}
}

//

REGISTER_FOR_FAST_CAST(RestPointPlacementData);

Framework::AppearanceControllerData* RestPointPlacementData::create_data()
{
	return new RestPointPlacementData();
}

void RestPointPlacementData::register_itself()
{
	Framework::AppearanceControllers::register_appearance_controller(NAME(RestPointPlacement), create_data);
}

RestPointPlacementData::RestPointPlacementData()
{
}

bool RestPointPlacementData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= handHoldPointSocket.load_from_xml(_node, TXT("handHoldPointSocket"));
	result &= restHoldPointSocket.load_from_xml(_node, TXT("restHoldPointSocket"));
	result &= restPointSocket.load_from_xml(_node, TXT("restPointSocket"));
	result &= restPointIdleSocket.load_from_xml(_node, TXT("restPointIdleSocket"));

	result &= handObjectVar.load_from_xml(_node, TXT("handObjectVarID"));
	result &= equipmentObjectVar.load_from_xml(_node, TXT("equipmentObjectVarID"));
	result &= equipmentHoldPointSocket.load_from_xml(_node, TXT("equipmentHoldPointSocket"));
	result &= equipmentRestPointSocket.load_from_xml(_node, TXT("equipmentRestPointSocket"));
	result &= restPointPlacementMSVar.load_from_xml(_node, TXT("restPointPlacementMSVarID"));
	result &= restTVar.load_from_xml(_node, TXT("restTVarID"));
	result &= restEmptyVar.load_from_xml(_node, TXT("restEmptyVarID"));

	result &= atPortRestT.load_from_xml(_node, TXT("atPortRestT"));
	result &= atPortHoldT.load_from_xml(_node, TXT("atPortHoldT"));
	result &= atPortRestOffset.load_from_xml(_node, TXT("atPortRestOffset"));
	result &= atPortHoldOffset.load_from_xml(_node, TXT("atPortHoldOffset"));
	result &= atPortHoldEntryDir.load_from_xml(_node, TXT("atPortHoldEntryDir"));

	return result;
}

bool RestPointPlacementData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::AppearanceControllerData* RestPointPlacementData::create_copy() const
{
	RestPointPlacementData* copy = new RestPointPlacementData();
	*copy = *this;
	return copy;
}

Framework::AppearanceController* RestPointPlacementData::create_controller()
{
	return new RestPointPlacement(this);
}

void RestPointPlacementData::apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	handHoldPointSocket.fill_value_with(_context);
	restHoldPointSocket.fill_value_with(_context);
	restPointSocket.fill_value_with(_context);
	restPointIdleSocket.fill_value_with(_context);
	handObjectVar.fill_value_with(_context);
	equipmentObjectVar.fill_value_with(_context);
	equipmentHoldPointSocket.fill_value_with(_context);
	equipmentRestPointSocket.fill_value_with(_context);
	restPointPlacementMSVar.fill_value_with(_context);
	restTVar.fill_value_with(_context);
	restEmptyVar.fill_value_with(_context);

	atPortRestT.fill_value_with(_context);
	atPortHoldT.fill_value_with(_context);
	atPortRestOffset.fill_value_with(_context);
	atPortHoldOffset.fill_value_with(_context);
	atPortHoldEntryDir.fill_value_with(_context);
}

void RestPointPlacementData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

void RestPointPlacementData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	handHoldPointSocket.if_value_set([this, _rename]() { handHoldPointSocket = _rename(handHoldPointSocket.get(), NP); });
	restHoldPointSocket.if_value_set([this, _rename]() { restHoldPointSocket = _rename(restHoldPointSocket.get(), NP); });
	restPointSocket.if_value_set([this, _rename]() { restPointSocket = _rename(restPointSocket.get(), NP); });
	restPointIdleSocket.if_value_set([this, _rename]() { restPointIdleSocket = _rename(restPointIdleSocket.get(), NP); });
	handObjectVar.if_value_set([this, _rename]() { handObjectVar = _rename(handObjectVar.get(), NP); });
	equipmentObjectVar.if_value_set([this, _rename]() { equipmentObjectVar = _rename(equipmentObjectVar.get(), NP); });
	equipmentHoldPointSocket.if_value_set([this, _rename]() { equipmentHoldPointSocket = _rename(equipmentHoldPointSocket.get(), NP); });
	equipmentRestPointSocket.if_value_set([this, _rename]() { equipmentRestPointSocket = _rename(equipmentRestPointSocket.get(), NP); });
	restPointPlacementMSVar.if_value_set([this, _rename]() { restPointPlacementMSVar = _rename(restPointPlacementMSVar.get(), NP); });
	restTVar.if_value_set([this, _rename]() { restTVar = _rename(restTVar.get(), NP); });
	restEmptyVar.if_value_set([this, _rename]() { restEmptyVar = _rename(restEmptyVar.get(), NP); });
}
