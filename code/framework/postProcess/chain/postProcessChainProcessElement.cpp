#include "postProcessChainProcessElement.h"
#include "..\postProcess.h"
#include "..\postProcessInstance.h"
#include "..\graph\postProcessInputNode.h"
#include "..\graph\postProcessOutputNode.h"
#include "..\graph\postProcessGraphProcessingContext.h"
#include "postProcessChain.h"
#include "postProcessChainProcessingContext.h"

#include "..\..\game\game.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(PostProcessChainProcessElement);

PostProcessChainProcessElement::PostProcessChainProcessElement(PostProcess* _postProcess, Tags const & _requires, Tags const & _disallows)
: requires(_requires)
, disallows(_disallows)
{
	an_assert(_postProcess != nullptr);
	postProcessInstance = new PostProcessInstance(_postProcess);
	postProcessInstance->initialise();
	PostProcessGraphInstance* graphInstance = postProcessInstance->get_graph_instance();
	an_assert(graphInstance != nullptr);
	PostProcessGraph const * graph = graphInstance->get_graph();
	an_assert(graph != nullptr);
	PostProcessRootNode const * rootNode = graph->get_root_node();
	an_assert(rootNode != nullptr);

	// add inputs basing on outputs of graph's input node
	inputNode = rootNode->get_input_node();
	for_every(outputConnector, inputNode->all_output_connectors())
	{
		add_input(outputConnector->get_name());
	}

	// add outputs basing on inputs of graph's output node
	outputNode = rootNode->get_output_node();
	for_every(inputConnector, outputNode->all_input_connectors())
	{
		add_output(inputConnector->get_name());
	}
}

PostProcessChainProcessElement::~PostProcessChainProcessElement()
{
	an_assert(postProcessInstance != nullptr);
	delete_and_clear(postProcessInstance);
}

int PostProcessChainProcessElement::get_output_render_target_count() const
{
	an_assert(postProcessInstance != nullptr);
	return outputNode->get_output_render_target_count(postProcessInstance->get_graph_instance()->get_data());
}

PostProcessRenderTargetPointer const & PostProcessChainProcessElement::get_output_render_target(int _index) const
{
	an_assert(postProcessInstance != nullptr);
	return outputNode->get_output_render_target(postProcessInstance->get_graph_instance()->get_data(), _index);
}

void PostProcessChainProcessElement::release_render_targets()
{
	base::release_render_targets();
	postProcessInstance->get_graph_instance()->release_render_targets();
}

void PostProcessChainProcessElement::do_post_process(PostProcessChainProcessingContext const & _processingContext)
{
	PostProcessGraphProcessingContext processingContext;
	processingContext.inputRenderTargetsProvider = this;
	processingContext.renderTargetManager = _processingContext.renderTargetManager;
	processingContext.camera = _processingContext.camera;
	processingContext.video3D = _processingContext.video3D;
	postProcessInstance->get_graph_instance()->do_post_process(processingContext);
	// propagate outputs
	base::do_post_process(_processingContext);
	// we no longer need render targets inside behavior graph
	postProcessInstance->get_graph_instance()->release_render_targets();
}

PostProcessRenderTargetPointer const & PostProcessChainProcessElement::get_input_render_target(Name const & _inputName) const
{
	for_every(input, inputs)
	{
		if (input->get_name() == _inputName)
		{
			return input->inputRenderTarget;
		}
	}
	return PostProcessRenderTargetPointer::empty();
}

bool PostProcessChainProcessElement::is_active() const
{
	bool requiresOk = true;
	bool disallowedBySomething = false;

	if (Game::get())
	{
		requires.do_for_every_tag([&requiresOk](Tag const & _tag)
		{
			if (!Game::get()->is_option_set(_tag.get_tag()))
			{
				requiresOk = false;
			}
		});
		disallows.do_for_every_tag([&disallowedBySomething](Tag const & _tag)
		{
			if (Game::get()->is_option_set(_tag.get_tag()))
			{
				disallowedBySomething = true;
			}
		});
	}

	return requiresOk && !disallowedBySomething;
}

void PostProcessChainProcessElement::for_every_shader_program_instance(ForShaderProgramInstance _for_every_shader_program_instance)
{
	base::for_every_shader_program_instance(_for_every_shader_program_instance);
	postProcessInstance->get_graph_instance()->for_every_shader_program_instance(_for_every_shader_program_instance);
}

