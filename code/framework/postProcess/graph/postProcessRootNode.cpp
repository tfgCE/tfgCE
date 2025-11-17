#include "postProcessRootNode.h"

#include "..\..\..\core\types\names.h"
#include "..\..\..\core\graph\graphImplementation.h"

#include "..\..\library\library.h"

#include "postProcessInputNode.h"
#include "postProcessOutputNode.h"
#include "postProcessProcessorNode.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(PostProcessRootNode);

PostProcessRootNode::PostProcessRootNode()
: nodeContainer(nullptr)
{
	set_name(Names::root);
}

PostProcessRootNode::~PostProcessRootNode()
{
	delete_and_clear(nodeContainer);
}

bool PostProcessRootNode::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	if (!nodeContainer)
	{
		nodeContainer = new NodeContainer(get_in_graph(), this);
	}

	result &= nodeContainer->load_from_xml(_node, _lc);

	result &= find_input_and_output();

	return result;
}

bool PostProcessRootNode::find_input_and_output()
{
	bool result = true;
	inputNode = nullptr;
	outputNode = nullptr;

	for_every_ptr(node, nodeContainer->all_nodes())
	{
		if (PostProcessInputNode* foundInputNode = fast_cast<PostProcessInputNode>(node))
		{
			if (inputNode)
			{
				error(TXT("more than one input node"));
				result = false;
			}
			inputNode = foundInputNode;
		}
		if (PostProcessOutputNode* foundOutputNode = fast_cast<PostProcessOutputNode>(node))
		{
			if (outputNode)
			{
				error(TXT("more than one output node"));
				result = false;
			}
			outputNode = foundOutputNode;
		}
	}

	return result;
}

bool PostProcessRootNode::ready_to_use()
{
	bool result = base::ready_to_use();

	result &= find_input_and_output();

	for_every_ptr(node, nodeContainer->all_nodes())
	{
		if (PostProcessProcessorNode * processorNode = fast_cast<PostProcessProcessorNode>(node))
		{
			processorNode->calculate_size_multiplier();
		}
	}

	return result;
}

bool PostProcessRootNode::load_shader_program(Library* _library)
{
	bool result = base::load_shader_program(_library);
	an_assert(nodeContainer, TXT("should be created by this time"));
	for_every_ptr(node, nodeContainer->all_nodes())
	{
		result &= node->load_shader_program(_library);
	}
	return result;
}

bool PostProcessRootNode::for_every_shader_program_instance(InstanceData & _instanceData, ForShaderProgramInstance _for_every_shader_program_instance) const
{
	bool result = base::for_every_shader_program_instance(_instanceData, _for_every_shader_program_instance);
	an_assert(nodeContainer, TXT("should be created by this time"));
	for_every_ptr(node, nodeContainer->all_nodes())
	{
		result &= node->for_every_shader_program_instance(_instanceData, _for_every_shader_program_instance);
		if (!result)
		{
			break;
		}
	}
	return result;
}

void PostProcessRootNode::manage_activation_using(InstanceData & _instanceData, SimpleVariableStorage const & _storage) const
{
	base::manage_activation_using(_instanceData, _storage);
	an_assert(nodeContainer, TXT("should be created by this time"));
	for_every_ptr(node, nodeContainer->all_nodes())
	{
		node->manage_activation_using(_instanceData, _storage);
	}
}
