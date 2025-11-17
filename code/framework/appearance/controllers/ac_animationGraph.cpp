#include "ac_animationGraph.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\mesh\pose.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(animationGraph);

//

REGISTER_FOR_FAST_CAST(AppearanceControllersLib::AnimationGraph);

AppearanceControllersLib::AnimationGraph::AnimationGraph(AnimationGraphData const * _data)
: base(_data)
{
	animationGraph = _data->animationGraph;
}

AppearanceControllersLib::AnimationGraph::~AnimationGraph()
{
}

void AppearanceControllersLib::AnimationGraph::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	animationGraph->prepare_instance(animationGraphRuntime, _owner);
}

void AppearanceControllersLib::AnimationGraph::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void AppearanceControllersLib::AnimationGraph::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(animationGraph_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh)
	{
		return;
	}

	Meshes::Pose * poseLS = _context.access_pose_LS();

	if (has_just_activated())
	{
		animationGraph->on_activate(animationGraphRuntime);
	}
	if (has_just_deactivated())
	{
		animationGraph->on_deactivate(animationGraphRuntime);
	}

	if (get_active() > 0.0f)
	{
		AnimationGraphExecuteContext context;
		context.deltaTime = _context.get_delta_time();

		AnimationGraphOutput output;
		if (get_active() >= 1.0f)
		{
			output.use_pose(poseLS);
		}
		else
		{
			output.use_own_pose(poseLS->get_skeleton());
		}

		animationGraphRuntime.ready_to_execute();

		animationGraph->execute(animationGraphRuntime, context, output);

		if (get_active() <= 1.0f)
		{
			poseLS->blend_in(get_active(), output.access_pose());
		}
	}
}

void AppearanceControllersLib::AnimationGraph::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);
}

//

REGISTER_FOR_FAST_CAST(AnimationGraphData);

AppearanceControllerData* AnimationGraphData::create_data()
{
	return new AnimationGraphData();
}

void AnimationGraphData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(animationGraph), create_data);
}

AnimationGraphData::AnimationGraphData()
{
}

bool AnimationGraphData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= animationGraph.load_from_xml(_node, TXT("animationGraph"), _lc);
	
	return result;
}

bool AnimationGraphData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= animationGraph.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	return result;
}

AppearanceControllerData* AnimationGraphData::create_copy() const
{
	AnimationGraphData* copy = new AnimationGraphData();
	*copy = *this;
	return copy;
}

AppearanceController* AnimationGraphData::create_controller()
{
	return new AnimationGraph(this);
}

void AnimationGraphData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);
}

void AnimationGraphData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);
}

