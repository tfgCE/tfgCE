#include "ac_heartRoomBeams.h"

#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\gameplay\modulePilgrimData.h"
#include "..\..\modules\custom\mc_grabable.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\appearance\appearanceControllerPoseContext.h"
#include "..\..\..\framework\appearance\appearanceControllers.h"
#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\meshGen\meshGenValueDefImpl.inl"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\performance\performanceUtils.h"

#include "..\..\..\core\debug\debugRenderer.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
#define DEBUG_DRAW_GRAB
//#define DEBUG_OUTPUT_RECOIL
#endif

//

using namespace TeaForGodEmperor;
using namespace AppearanceControllersLib;

//

// ac name
DEFINE_STATIC_NAME(heartRoomBeams);

//

REGISTER_FOR_FAST_CAST(HeartRoomBeams);

HeartRoomBeams::HeartRoomBeams(HeartRoomBeamsData const * _data)
: base(_data)
{
	rotatingBone = _data->rotatingBone.get();
}

HeartRoomBeams::~HeartRoomBeams()
{
}

void HeartRoomBeams::initialise(Framework::ModuleAppearance* _owner)
{
	base::initialise(_owner);

	if (auto* skel = get_owner()->get_skeleton()->get_skeleton())
	{
		rotatingBone.look_up(skel);
	}
}

void HeartRoomBeams::calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext& _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(recoil_finalPose);
#endif

	auto const* mesh = get_owner()->get_mesh();

	if (!mesh)
	{
		return;
	}

	AI::Logics::HeartRoomBeams* hrbAI = nullptr;

	if (auto* ai = get_owner()->get_owner()->get_ai())
	{
		if (auto* mind = ai->get_mind())
		{
			hrbAI = fast_cast<AI::Logics::HeartRoomBeams>(mind->get_logic());
		}
	}

	if (!hrbAI)
	{
		return;
	}

	Meshes::Pose* poseLS = _context.access_pose_LS();

	if (rotatingBone.is_valid())
	{
		int const boneIdx = rotatingBone.get_index();

		Transform boneLS = poseLS->get_bone(boneIdx);
		boneLS = boneLS.to_world(Transform(Vector3::zero, Rotator3(0.0f, hrbAI->get_rotating_yaw(), 0.0f).to_quat()));
		poseLS->set_bone(boneIdx, boneLS);
	}

	hrbAI->fill_beam_infos(beams);

	auto ms2ws = get_owner()->get_ms_to_ws_transform();

	for_every(beam, beams)
	{
		if (beam->boneStart.is_valid() &&
			beam->boneEnd.is_valid())
		{
			Transform startMS = ms2ws.to_local(beam->startWS);
			Transform endMS = ms2ws.to_local(beam->endWS);
			poseLS->set_bone_ms(beam->boneStart.get_index(), startMS);
			poseLS->set_bone_ms(beam->boneEnd.get_index(), endMS);
		}
	}
}

//

REGISTER_FOR_FAST_CAST(HeartRoomBeamsData);

Framework::AppearanceControllerData* HeartRoomBeamsData::create_data()
{
	return new HeartRoomBeamsData();
}

void HeartRoomBeamsData::register_itself()
{
	Framework::AppearanceControllers::register_appearance_controller(NAME(heartRoomBeams), create_data);
}

HeartRoomBeamsData::HeartRoomBeamsData()
{
}

bool HeartRoomBeamsData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= rotatingBone.load_from_xml(_node, TXT("rotatingBone"));

	return result;
}

bool HeartRoomBeamsData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::AppearanceControllerData* HeartRoomBeamsData::create_copy() const
{
	HeartRoomBeamsData* copy = new HeartRoomBeamsData();
	*copy = *this;
	return copy;
}

Framework::AppearanceController* HeartRoomBeamsData::create_controller()
{
	return new HeartRoomBeams(this);
}

void HeartRoomBeamsData::apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	rotatingBone.fill_value_with(_context);

}

void HeartRoomBeamsData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	rotatingBone.if_value_set([this, _rename](){ rotatingBone = _rename(rotatingBone.get(), NP); });
}
