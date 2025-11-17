#include "ac_sway.h"

#include "walker.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\performance\performanceUtils.h"

#ifdef AN_ALLOW_OPTIMIZE_OFF
//#pragma optimize("", off)
#endif

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(sway);
DEFINE_STATIC_NAME(walker);

//

REGISTER_FOR_FAST_CAST(Sway);

Sway::Sway(SwayData const * _data)
: base(_data)
, swayData(_data)
{
	bone = swayData->bone.get();
	swayTo = swayData->swayTo;
	legSwayAngle = swayData->legSwayAngle.get();
	legSwayDistance = swayData->legSwayDistance.get();
	legSwayVerticalDistance = swayData->legSwayVerticalDistance.get();
	angleBlendTime = swayData->angleBlendTime.get();
	distanceBlendTime = swayData->distanceBlendTime.get();
}

Sway::~Sway()
{
}

void Sway::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	bone.look_up(get_owner()->get_skeleton()->get_skeleton());

	for_every_ref(ac, _owner->get_controllers())
	{
		if (Walker* asWalker = fast_cast<Walker>(ac))
		{
			walker = asWalker;
			break;
		}
	}
}

void Sway::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void Sway::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	if (! get_active())
	{
		return;
	}

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(sway_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid())
	{
		return;
	}

	// time advance
	if (has_just_activated())
	{
		swayRotationBS = Rotator3::zero;
		swayLocationBS = Vector3::zero;
	}

	Rotator3 targetSwayRotationBS = Rotator3::zero;
	Vector3 targetSwayLocationBS = Vector3::zero;

	int const boneIdx = bone.get_index();
	Meshes::Pose * poseLS = _context.access_pose_LS();

	// get our bone's MS location - we will be using that to calculate angles properly
	Transform const boneMS = poseLS->get_bone_ms_from_ls(boneIdx);

	if (swayTo == SwayTo::Walker && walker.is_set())
	{
		Vector3 const upAxisMS = Vector3::zAxis; // we assumed this is our up axis for all objects (it is MS)

		for_count(int, legIdx, walker->get_legs().get_size())
		{
			auto const & leg = walker->get_walker_instance().get_leg(legIdx);
			if (leg.get_action() == Walkers::LegAction::Up)
			{
				Vector3 toLegMS = leg.get_placement_MS().get_translation() - boneMS.get_translation();
				Vector3 turnAxisMS = Vector3::cross(toLegMS, upAxisMS).normal();
				if (!turnAxisMS.is_zero())
				{
					targetSwayRotationBS += Quat::axis_rotation_normalised(boneMS.vector_to_local(turnAxisMS), legSwayAngle).to_rotator();
					targetSwayLocationBS += boneMS.vector_to_local(toLegMS).drop_using_normalised(upAxisMS).normal() * legSwayDistance;
					targetSwayLocationBS += boneMS.vector_to_local(upAxisMS) * legSwayVerticalDistance;
				}
				else
				{
					warn(TXT("sway turn axis was zero"));
				}
			}
		}

		// limit to walkers activation
		targetSwayRotationBS *= walker->get_active();
		targetSwayLocationBS *= walker->get_active();
	}
	else if (swayTo == SwayTo::DefaultPlacement)
	{
		Transform const defaultBoneMS = get_owner()->get_skeleton()->get_skeleton()->get_bones_default_placement_MS()[boneIdx];
		Transform const toBS = boneMS.to_local(defaultBoneMS);
		targetSwayRotationBS += toBS.get_orientation().to_rotator();
		targetSwayLocationBS += toBS.get_translation();
	}

	float const deltaTime = _context.get_delta_time();
	swayRotationBS = blend_to_using_time(swayRotationBS, targetSwayRotationBS, angleBlendTime, deltaTime);
	swayLocationBS = blend_to_using_time(swayLocationBS, targetSwayLocationBS, distanceBlendTime, deltaTime);

	float applySway = get_active();

	// calculate sway to apply
	Transform swayTotal = Transform::identity;

	swayTotal.set_translation(swayLocationBS * applySway);
	swayTotal.set_orientation((swayRotationBS * applySway).to_quat());

	swayTotal = Transform::lerp(get_active(), Transform::identity, swayTotal);

	// apply!
	Transform boneLS = poseLS->get_bone(boneIdx);
	boneLS = boneLS.to_world(swayTotal);

	// write back
	poseLS->access_bones()[boneIdx] = boneLS;
}

void Sway::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (bone.is_valid())
	{
		dependsOnParentBones.push_back(bone.get_index());
		providesBones.push_back(bone.get_index());
	}
	if (swayTo == SwayTo::Walker && walker.is_set())
	{
		dependsOnControllers.push_back(NAME(walker));
	}
}

//

REGISTER_FOR_FAST_CAST(SwayData);

AppearanceControllerData* SwayData::create_data()
{
	return new SwayData();
}

void SwayData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(sway), create_data);
}

SwayData::SwayData()
{
}

bool SwayData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= legSwayAngle.load_from_xml(_node, TXT("legSwayAngle"));
	result &= legSwayDistance.load_from_xml(_node, TXT("legSwayDistance"));
	result &= legSwayVerticalDistance.load_from_xml(_node, TXT("legSwayVerticalDistance"));
	result &= angleBlendTime.load_from_xml(_node, TXT("angleBlendTime"));
	result &= distanceBlendTime.load_from_xml(_node, TXT("distanceBlendTime"));

	String swayToStr = _node->get_string_attribute_or_from_child(TXT("to"));
	if (!swayToStr.is_empty())
	{
		if (swayToStr == TXT("walker"))
		{
			swayTo = SwayTo::Walker;
		}
		else if (swayToStr == TXT("defaultPlacement"))
		{
			swayTo = SwayTo::DefaultPlacement;
		}
		else
		{
			error_loading_xml(_node, TXT("could not recognise \"%S\" sway to type"), swayToStr.to_char());
			result = false;
		}
	}

	return result;
}

bool SwayData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* SwayData::create_copy() const
{
	SwayData* copy = new SwayData();
	*copy = *this;
	return copy;
}

AppearanceController* SwayData::create_controller()
{
	return new Sway(this);
}

void SwayData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	legSwayAngle.fill_value_with(_context);
	legSwayDistance.fill_value_with(_context);
	legSwayVerticalDistance.fill_value_with(_context);
	angleBlendTime.fill_value_with(_context);
	distanceBlendTime.fill_value_with(_context);
}

void SwayData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

void SwayData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
}
