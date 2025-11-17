#include "postProcessGraphInstance.h"

#include "..\..\..\core\graph\graphImplementation.h"

#include "..\..\debugSettings.h"

#include "..\postProcess.h"

#include "postProcessGraph.h"
#include "postProcessGraphProcessingContext.h"
#include "postProcessRootNode.h"
#include "postProcessOutputNode.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

PostProcessGraphInstance::PostProcessGraphInstance(PostProcessGraph const * _graph)
: base(_graph)
, postProcessInstance(nullptr)
, outputNode(nullptr)
, initialised(false)
{
}

PostProcessGraphInstance::~PostProcessGraphInstance()
{
}

void PostProcessGraphInstance::initialise(PostProcessInstance* _postProcessInstance)
{
	postProcessInstance = _postProcessInstance;
	outputNode = nullptr;
	for_every_ptr(node, graph->get_all_nodes())
	{
		node->initialise(*instanceData);
		if (PostProcessOutputNode const * foundOutputNode = fast_cast<PostProcessOutputNode>(node))
		{
			an_assert(outputNode == nullptr, TXT("there shouldn't be more than one output"));
			outputNode = foundOutputNode;
		}
	}
	initialised = true;
}

void PostProcessGraphInstance::release_render_targets()
{
	for_every_ptr(node, graph->get_all_nodes())
	{
		node->release_render_targets(*instanceData);
	}
}

void PostProcessGraphInstance::do_post_process(PostProcessGraphProcessingContext & _processingContext)
{
#ifdef AN_LOG_POST_PROCESS
	if (PostProcessDebugSettings::log)
	{
		output(TXT("do post process (graph:%i)-----------------------------------*"), get_graph());
	}
#endif

	// update all states
	//_processingContext.video3D->requires_send_use_textures();
	_processingContext.video3D->mark_disable_blend();
	//_processingContext.video3D->requires_send_state();

	for_every_ptr(node, graph->get_all_nodes())
	{
		node->ready_for_post_processing(*instanceData);
	}
	
#ifdef AN_LOG_POST_PROCESS
	if (PostProcessDebugSettings::log)
	{
		output(TXT("  ready, do!"));
	}
#endif

	// start with output it will get to nodes that it needs
	graph->get_root_node()->get_output_node()->do_post_process(*instanceData, _processingContext);

#ifdef AN_LOG_POST_PROCESS
	if (PostProcessDebugSettings::log)
	{
		output(TXT("done post process------------------------<<"));
	}
#endif
}

void PostProcessGraphInstance::for_every_shader_program_instance(ForShaderProgramInstance _for_every_shader_program_instance)
{
	// start with output it will get to nodes that it needs
	graph->get_root_node()->for_every_shader_program_instance(*instanceData, _for_every_shader_program_instance);
}

void PostProcessGraphInstance::manage_activation_using(SimpleVariableStorage const & _storage)
{
	// start with output it will get to nodes that it needs
	graph->get_root_node()->manage_activation_using(*instanceData, _storage);
}

PostProcessRenderTargetPointer const & PostProcessGraphInstance::get_output_render_target(int _outputIndex, OUT_ OPTIONAL_ Name * _outputRenderTargetName) const
{
	if (outputNode)
	{
		return outputNode->get_output_render_target(instanceData, _outputIndex, _outputRenderTargetName);
	}
	else
	{
		return PostProcessRenderTargetPointer::empty();
	}
}

PostProcessRenderTargetPointer const & PostProcessGraphInstance::get_output_render_target() const
{
	if (outputNode)
	{
		return outputNode->get_output_render_target(instanceData, outputNode->get_main_output_render_target_index());
	}
	else
	{
		return PostProcessRenderTargetPointer::empty();
	}
}
