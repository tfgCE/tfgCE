#include "animationGraphNode.h"

#include "animationGraphNodes.h"

#include "..\..\core\mesh\pose.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(IAnimationGraphNode);

bool IAnimationGraphNode::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	XML_LOAD_NAME_ATTR(_node, id);

	return result;
}

//--

AnimationGraphNodeSet::AnimationGraphNodeSet()
{
}

AnimationGraphNodeSet::~AnimationGraphNodeSet()
{
	clear();
}

void AnimationGraphNodeSet::clear()
{
	nodes.clear();
}

bool AnimationGraphNodeSet::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	clear();

	for_every(node, _node->children_named(TXT("node")))
	{
		Name type;
		XML_LOAD_NAME_ATTR(node, type);
		if (auto* agNode = AnimationGraphNodes::create_node(type))
		{
			if (agNode->load_from_xml(node, _lc))
			{
				nodes.push_back();
				nodes.get_last() = agNode;
			}
			else
			{
				delete agNode;
				result = false;
			}
		}
		else
		{
			error_loading_xml(node, TXT("couldn't recognise animation graph node type \"%S\""), type.to_char());
			result = false;
		}
	}

	return result;
}

bool AnimationGraphNodeSet::prepare_runtime(AnimationGraphRuntime& _runtime)
{
	bool result = true;
	for_every_ref(n, nodes)
	{
		result &= n->prepare_runtime(_runtime, *this);
	}
	return result;
}

void AnimationGraphNodeSet::prepare_instance(AnimationGraphRuntime& _runtime) const
{
	for_every_ref(n, nodes)
	{
		n->prepare_instance(_runtime);
	}
}

IAnimationGraphNode* AnimationGraphNodeSet::find_node(Name const& _id) const
{
	for_every_ref(n, nodes)
	{
		if (n->get_id() == _id)
		{
			return n;
		}
	}
	return nullptr;
}

//--

bool AnimationGraphNodeInput::load_from_xml(IO::XML::Node const* _node, tchar const* _attrName)
{
	bool result = true;

	id = _node->get_name_attribute(_attrName, id);

	return result;
}

bool AnimationGraphNodeInput::prepare_runtime(AnimationGraphNodeSet& _inSet)
{
	bool result = true;

	node = nullptr;
	if (id.is_valid())
	{
		if (auto* found = _inSet.find_node(id))
		{
			node = found;
		}
		else
		{
			error(TXT("could not find animation graph node \"%S\""), id.to_char());
			result = false;
		}
	}

	return result;
}

//--

AnimationGraphOutput::~AnimationGraphOutput()
{
	drop_pose();
}

void AnimationGraphOutput::drop_pose()
{
	if (ownPose)
	{
		pose->release();
	}
	pose = nullptr;
}

void AnimationGraphOutput::use_pose(Meshes::Pose* _pose)
{
	drop_pose();
	pose = _pose;
	ownPose = false;
}

void AnimationGraphOutput::use_own_pose(Meshes::Skeleton const * _skeleton)
{
	drop_pose();
	pose = Meshes::Pose::get_one(_skeleton);
	ownPose = true;
}

void AnimationGraphOutput::blend_in(float _blendInWeight, AnimationGraphOutput const& _blendInOutput, bool _blendRootMotion)
{
	pose->blend_in(_blendInWeight, _blendInOutput.pose);
	if (_blendRootMotion)
	{
		lerp(_blendInWeight, rootMotion, _blendInOutput.rootMotion);
	}
}

//--

REGISTER_FOR_FAST_CAST(AnimationGraphNodeWithSingleInput);

bool AnimationGraphNodeWithSingleInput::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= inputNode.load_from_xml(_node);

	error_loading_xml_on_assert(inputNode.get_id().is_valid(), result, _node, TXT("no input node id provided \"inputNodeId\""));
	
	return result;
}

bool AnimationGraphNodeWithSingleInput::prepare_runtime(AnimationGraphRuntime& _runtime, AnimationGraphNodeSet& _inSet)
{
	bool result = base::prepare_runtime(_runtime, _inSet);

	result &= inputNode.prepare_runtime(_inSet);

	return result;
}

void AnimationGraphNodeWithSingleInput::prepare_instance(AnimationGraphRuntime& _runtime) const
{
	base::prepare_instance(_runtime);
}

void AnimationGraphNodeWithSingleInput::on_activate(AnimationGraphRuntime& _runtime) const
{
	base::on_activate(_runtime);

	if (auto* node = inputNode.get_node())
	{
		node->on_activate(_runtime);
	}
}

void AnimationGraphNodeWithSingleInput::on_reactivate(AnimationGraphRuntime& _runtime) const
{
	base::on_reactivate(_runtime);

	if (auto* node = inputNode.get_node())
	{
		node->on_reactivate(_runtime);
	}
}

void AnimationGraphNodeWithSingleInput::on_deactivate(AnimationGraphRuntime& _runtime) const
{
	base::on_deactivate(_runtime);

	if (auto* node = inputNode.get_node())
	{
		node->on_deactivate(_runtime);
	}
}

void AnimationGraphNodeWithSingleInput::execute(AnimationGraphRuntime& _runtime, AnimationGraphExecuteContext& _context, OUT_ AnimationGraphOutput& _output) const
{
	base::execute(_runtime, _context, _output);

	if (auto* node = inputNode.get_node())
	{
		node->execute(_runtime, _context, _output);
	}
}

//--

Optional<float>& AnimationGraphRuntime::access_sync_playback(Name const& _id)
{
	for_every(s, syncPlaybacks)
	{
		if (s->id == _id)
		{
			return s->pt;
		}
	}

	syncPlaybacks.push_back();
	auto& s = syncPlaybacks.get_last();
	s.id = _id;
	return s.pt;
}

Optional<float> const * AnimationGraphRuntime::get_prev_sync_playback(Name const& _id) const
{
	for_every(s, syncPlaybacks)
	{
		if (s->id == _id)
		{
			return &s->prevPt;
		}
	}

	return nullptr;
}

Optional<float> const* AnimationGraphRuntime::get_any_set_sync_playback(Name const& _id) const
{
	for_every(s, syncPlaybacks)
	{
		if (s->id == _id)
		{
			if (s->pt.is_set())
			{
				return &s->pt;
			}
			if (s->prevPt.is_set())
			{
				return &s->prevPt;
			}
			return nullptr;
		}
	}

	return nullptr;
}

void AnimationGraphRuntime::ready_to_execute()
{
	for_every(s, syncPlaybacks)
	{
		s->prevPt = s->pt;
		s->pt.clear();
	}
}
