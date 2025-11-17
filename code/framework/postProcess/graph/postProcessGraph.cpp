#include "postProcessGraph.h"

#include "..\..\..\core\graph\graphImplementation.h"

#include "postProcessInputNode.h"
#include "postProcessOutputNode.h"
#include "postProcessProcessorNode.h"

using namespace Framework;

PostProcessGraph::PostProcessGraph()
: postProcessRootNode(nullptr)
{
	set_self_as_graph_class(this);
	create_root_node_if_missing();
}

bool PostProcessGraph::load_shader_programs(Library* _library)
{
	bool result = true;
	
	an_assert(postProcessRootNode, TXT("should be created by this time"));

	result &= postProcessRootNode->load_shader_program(_library);

	return result;
}

PostProcessNode* PostProcessGraph::create_node(String const & _name, NodeContainer* _nodeContainer)
{
	if (_name == TXT("processorNode"))
	{
		return new PostProcessProcessorNode();
	}
	if (_name == TXT("inputNode"))
	{
		return new PostProcessInputNode();
	}
	if (_name == TXT("outputNode"))
	{
		return new PostProcessOutputNode();
	}
	return base::create_node(_name, _nodeContainer);
}

PostProcessNode* PostProcessGraph::create_root_node()
{
	return new PostProcessRootNode();
}

void PostProcessGraph::set_root_node(PostProcessNode* _rootNode)
{
	base::set_root_node(_rootNode);
	postProcessRootNode = fast_cast<PostProcessRootNode>(_rootNode);
	an_assert(postProcessRootNode);
}
