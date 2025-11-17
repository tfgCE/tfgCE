#include "agn_defaultPose.h"

#include "..\animationGraphNodes.h"

#include "..\..\..\core\mesh\pose.h"
#include "..\..\..\core\mesh\skeleton.h"

//

using namespace Framework;
using namespace AnimationGraphNodesLib;

//

REGISTER_FOR_FAST_CAST(DefaultPose);

IAnimationGraphNode* DefaultPose::create_node()
{
	return new DefaultPose();
}

bool DefaultPose::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool DefaultPose::prepare_runtime(AnimationGraphRuntime& _runtime, AnimationGraphNodeSet& _inSet)
{
	bool result = base::prepare_runtime(_runtime, _inSet);

	//ANIMATION_GRAPH_RUNTIME_VARIABLES_START_DECLARING(DefaultPose);

	return result;
}

void DefaultPose::prepare_instance(AnimationGraphRuntime& _runtime) const
{
	base::prepare_instance(_runtime);

	//ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(DefaultPose);
}

void DefaultPose::on_activate(AnimationGraphRuntime& _runtime) const
{
	base::on_activate(_runtime);

	//ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(DefaultPose);
}

void DefaultPose::on_reactivate(AnimationGraphRuntime& _runtime) const
{
	base::on_reactivate(_runtime);

	//ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(DefaultPose);
}

void DefaultPose::on_deactivate(AnimationGraphRuntime& _runtime) const
{
	base::on_deactivate(_runtime);

	//ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(DefaultPose);
}

void DefaultPose::execute(AnimationGraphRuntime& _runtime, AnimationGraphExecuteContext& _context, OUT_ AnimationGraphOutput& _output) const
{
	base::execute(_runtime, _context, _output);

	//ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(DefaultPose);

	auto* pose = _output.access_pose();
	auto* skel = pose->get_skeleton();

	pose->fill_with(skel->get_bones_default_placement_LS());
}
