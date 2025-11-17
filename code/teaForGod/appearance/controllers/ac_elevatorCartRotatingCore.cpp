#include "ac_elevatorCartRotatingCore.h"

#include "..\..\ai\logics\aiLogic_elevatorCart.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\appearance\appearanceControllerPoseContext.h"
#include "..\..\..\framework\appearance\appearanceControllers.h"
#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\object\scenery.h"

#include "..\..\..\core\mesh\pose.h"
#include "..\..\..\core\performance\performanceUtils.h"

#include "..\..\..\core\debug\debugRenderer.h"

//

using namespace TeaForGodEmperor;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(elevatorCartRotatingCore);

//

REGISTER_FOR_FAST_CAST(ElevatorCartRotatingCore);

ElevatorCartRotatingCore::ElevatorCartRotatingCore(ElevatorCartRotatingCoreData const * _data)
: base(_data)
, elevatorCartRotatingCoreData(_data)
{
	bone = elevatorCartRotatingCoreData->bone.get();
}

ElevatorCartRotatingCore::~ElevatorCartRotatingCore()
{
}

void ElevatorCartRotatingCore::initialise(Framework::ModuleAppearance* _owner)
{
	base::initialise(_owner);

	Random::Generator rg = _owner->get_owner()->get_individual_random_generator();
	rg.advance_seed(19295, 20845);
	lastPlacementWS = look_matrix(Vector3::zero, Rotator3(0.0f, rg.get_float(0.0f, 360.0f), 0.0f).get_forward(), Vector3::zAxis).to_transform();
	if (auto* skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		bone.look_up(skeleton);
		parent = Meshes::BoneID(skeleton, skeleton->get_parent_of(bone.get_index()));
	}
}

void ElevatorCartRotatingCore::advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void ElevatorCartRotatingCore::calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(elevatorCartRotatingCore_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();
	AI::Logics::ElevatorCart* logic = nullptr;

	auto* imo = get_owner()->get_owner();
	if (imo)
	{
		if (auto* ai = imo->get_ai())
		{
			if (auto * mind = ai->get_mind())
			{
				logic = fast_cast<AI::Logics::ElevatorCart>(mind->get_logic());
			}
		}
	}

	if (!mesh ||
		!bone.is_valid() ||
		!logic)
	{
		return;
	}

	debug_filter(ac_elevatorCartRotatingCore);
	debug_context(imo->get_presence()->get_in_room());

	Meshes::Pose * poseLS = _context.access_pose_LS();

	auto * cartPointA = logic->get_cart_point_a();
	auto * cartPointB = logic->get_cart_point_b();
	if (cartPointA && cartPointB)
	{
		Transform cpAPlacement = cartPointA->get_presence()->get_placement();
		Transform cpBPlacement = cartPointB->get_presence()->get_placement();

		if (logic->get_cart_path() == AI::Logics::ElevatorCartPath::CurveMoveXZ)
		{
			Plane cpAPlane(cpAPlacement.get_axis(Axis::X), cpAPlacement.get_translation());
			Plane cpBPlane(cpBPlacement.get_axis(Axis::X), cpBPlacement.get_translation());
			Vector3 upAxis = Vector3::cross(cpAPlane.get_normal(), cpBPlane.get_normal()).normal();
			if (Vector3::dot(upAxis, lastPlacementWS.get_axis(Axis::Z)) < 0.0f)
			{
				upAxis = -upAxis;
			}

			/**
			 *						 b
			 *					    /
			 *					   B
			 *					  /
			 *					 /
			 *					/
			 *		-----------u--------A------- a
			 */
			Vector3 origin = cpBPlacement.get_translation();
			Vector3 onBPlanePerpendicularToAxis = Vector3::cross(upAxis, cpBPlane.get_normal()).normal();
			debug_draw_arrow(true, Colour::green, origin, origin + onBPlanePerpendicularToAxis);
			float originDistance = cpAPlane.get_in_front(origin);
			float vectorsAlignment = Vector3::dot(onBPlanePerpendicularToAxis, cpAPlane.get_normal());
			Vector3 newOrigin = origin - onBPlanePerpendicularToAxis * originDistance / vectorsAlignment;
			debug_draw_arrow(true, Colour::red, origin, newOrigin);
			origin = newOrigin;

			Transform placementWS = look_matrix(origin, lastPlacementWS.get_axis(Axis::Y), upAxis).to_transform();

			Transform parentMS = poseLS->get_bone(parent.get_index());
			Transform parentWS = get_owner()->get_ms_to_ws_transform().to_world(parentMS);

			Transform placementLS = parentWS.to_local(placementWS);

			poseLS->access_bones()[bone.get_index()] = placementLS;
		}
		else
		{
			error(TXT("cart path not handled!"));
		}
	}
	debug_no_context();
	debug_no_filter();
}

void ElevatorCartRotatingCore::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
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

REGISTER_FOR_FAST_CAST(ElevatorCartRotatingCoreData);

Framework::AppearanceControllerData* ElevatorCartRotatingCoreData::create_data()
{
	return new ElevatorCartRotatingCoreData();
}

void ElevatorCartRotatingCoreData::register_itself()
{
	Framework::AppearanceControllers::register_appearance_controller(NAME(elevatorCartRotatingCore), create_data);
}

ElevatorCartRotatingCoreData::ElevatorCartRotatingCoreData()
{
}

bool ElevatorCartRotatingCoreData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	
	return result;
}

bool ElevatorCartRotatingCoreData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::AppearanceControllerData* ElevatorCartRotatingCoreData::create_copy() const
{
	ElevatorCartRotatingCoreData* copy = new ElevatorCartRotatingCoreData();
	*copy = *this;
	return copy;
}

Framework::AppearanceController* ElevatorCartRotatingCoreData::create_controller()
{
	return new ElevatorCartRotatingCore(this);
}

void ElevatorCartRotatingCoreData::apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
}

void ElevatorCartRotatingCoreData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

void ElevatorCartRotatingCoreData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
}
