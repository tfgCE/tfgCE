#include "animationGraphNodes.h"

#include "graphNodes\agn_animationPlayback.h"
#include "graphNodes\agn_atPlacement.h"
#include "graphNodes\agn_blendList.h"
#include "graphNodes\agn_defaultPose.h"
#include "graphNodes\agn_prepareVariables.h"
#include "graphNodes\agn_randomAnimationPlayback.h"
#include "graphNodes\agn_stateMachine.h"

//

using namespace Framework;

//

void AnimationGraphNodes::initialise_static()
{
	base::initialise_static();

	// lib
	AnimationGraphNodes::register_animation_graph_node(Name(TXT("animation playback")), AnimationGraphNodesLib::AnimationPlayback::create_node);
	AnimationGraphNodes::register_animation_graph_node(Name(TXT("at placement")), AnimationGraphNodesLib::AtPlacement::create_node);
	AnimationGraphNodes::register_animation_graph_node(Name(TXT("blend list")), AnimationGraphNodesLib::BlendList::create_node);
	AnimationGraphNodes::register_animation_graph_node(Name(TXT("default pose")), AnimationGraphNodesLib::DefaultPose::create_node);
	AnimationGraphNodes::register_animation_graph_node(Name(TXT("prepare variables")), AnimationGraphNodesLib::PrepareVariables::create_node);
	AnimationGraphNodes::register_animation_graph_node(Name(TXT("random animation playback")), AnimationGraphNodesLib::RandomAnimationPlayback::create_node);
	AnimationGraphNodes::register_animation_graph_node(Name(TXT("state machine")), AnimationGraphNodesLib::StateMachine::create_node);
}

void AnimationGraphNodes::close_static()
{
	base::close_static();
}

