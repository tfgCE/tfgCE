#include "ac_a_containerJump.h"

#include "..\..\..\framework\appearance\appearanceControllerPoseContext.h"
#include "..\..\..\framework\appearance\appearanceControllers.h"
#include "..\..\..\framework\appearance\skeleton.h"
#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleSound.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(a_containerJump);

//

REGISTER_FOR_FAST_CAST(A_ContainerJump);

A_ContainerJump::A_ContainerJump(A_ContainerJumpData const * _data)
: base(_data)
, a_containerJumpData(_data)
{
	bone = a_containerJumpData->bone.get(Name::invalid());
	jumpVar = a_containerJumpData->jumpVar.get(Name::invalid());

	interval = a_containerJumpData->interval.get();
	maxDistance = a_containerJumpData->maxDistance.get();
	maxYaw = a_containerJumpData->maxYaw.get();
	jumpTime = a_containerJumpData->jumpTime.get();

	makeSound = a_containerJumpData->makeSound.get(Name::invalid());
}

A_ContainerJump::~A_ContainerJump()
{
}

void A_ContainerJump::initialise(Framework::ModuleAppearance* _owner)
{
	base::initialise(_owner);

	if (auto* skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		bone.look_up(skeleton);
	}

	auto& varStorage = get_owner()->access_controllers_variable_storage();
	jumpVar.look_up<bool>(varStorage);
}

void A_ContainerJump::advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void A_ContainerJump::calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(a_containerJump_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!get_active())
	{
		return;
	}

	auto* imo = get_owner()->get_owner();
	auto* presence = imo->get_presence();

	debug_filter(ac_a_containerJump);

	debug_context(presence->get_in_room());

	Transform placementWS = presence->get_placement();

	float deltaTime = _context.get_delta_time();

	timeLeftToJump -= deltaTime;
	if (timeLeftToJump <= 0.0f)
	{
		timeLeftToJump = 0.0f;
		if (jumpVar.get<bool>())
		{
			Random::Generator rg;
			timeLeftToJump = rg.get(interval);
			if (timeLeftToJump > 0.0f)
			{
				jumpTimeLeft = rg.get(jumpTime);
				Vector3 off = Rotator3(0.0f, rg.get_float(0.0f, 360.0f), 0.0f).get_forward() * rg.get_float(0.0f, maxDistance);
				Rotator3 rot = Rotator3(0.0f, rg.get_float(-maxYaw, maxYaw), 0.0f);

				jumpDest = Transform(off, rot.to_quat());
				if (makeSound.is_valid())
				{
					if (auto* s = imo->get_sound())
					{
						s->play_sound(makeSound);
					}
				}
			}
		}
	}

	float prevJumpTimeLeft = jumpTimeLeft;
	if (prevJumpTimeLeft > 0.0f)
	{
		jumpTimeLeft -= deltaTime;
		jumpTimeLeft = max(jumpTimeLeft, 0.0f);
		float coveredPt = clamp(1.0f - jumpTimeLeft / prevJumpTimeLeft, 0.0f, 1.0f);
		Transform res = Transform::lerp(coveredPt, jumpAt, jumpDest);
		jumpAt = res;
	}

	Meshes::Pose* poseLS = _context.access_pose_LS();

	int boneIdx = bone.get_index();
	Transform boneLS = poseLS->get_bone(boneIdx);
	boneLS = jumpAt;
	poseLS->set_bone(boneIdx, boneLS);

	debug_no_context();

	debug_no_filter();
}

void A_ContainerJump::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	providesBones.push_back(bone.get_index());

	dependsOnVariables.push_back(jumpVar.get_name());
}

//

REGISTER_FOR_FAST_CAST(A_ContainerJumpData);

Framework::AppearanceControllerData* A_ContainerJumpData::create_data()
{
	return new A_ContainerJumpData();
}

void A_ContainerJumpData::register_itself()
{
	Framework::AppearanceControllers::register_appearance_controller(NAME(a_containerJump), create_data);
}

A_ContainerJumpData::A_ContainerJumpData()
{
}

bool A_ContainerJumpData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= jumpVar.load_from_xml(_node, TXT("jumpVar"));
	
	result &= interval.load_from_xml(_node, TXT("interval"));
	result &= maxDistance.load_from_xml(_node, TXT("maxDistance"));
	result &= maxYaw.load_from_xml(_node, TXT("maxYaw"));
	result &= jumpTime.load_from_xml(_node, TXT("jumpTime"));

	result &= makeSound.load_from_xml(_node, TXT("makeSound"));

	return result;
}

bool A_ContainerJumpData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::AppearanceControllerData* A_ContainerJumpData::create_copy() const
{
	A_ContainerJumpData* copy = new A_ContainerJumpData();
	*copy = *this;
	return copy;
}

Framework::AppearanceController* A_ContainerJumpData::create_controller()
{
	return new A_ContainerJump(this);
}

void A_ContainerJumpData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
}

void A_ContainerJumpData::apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	jumpVar.fill_value_with(_context);

	interval.fill_value_with(_context);
	maxDistance.fill_value_with(_context);
	maxYaw.fill_value_with(_context);
	jumpTime.fill_value_with(_context);

	makeSound.fill_value_with(_context);
}

