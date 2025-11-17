#include "postProcessChainOutputElement.h"
#include "postProcessChain.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(PostProcessChainOutputElement);

PostProcessChainOutputElement::PostProcessChainOutputElement()
{
}

PostProcessChainOutputElement::~PostProcessChainOutputElement()
{
}

void PostProcessChainOutputElement::use_output(::System::FragmentShaderOutputSetup const & _outputDefinition)
{
	// (re)initialise inputs with number of outputs from postProcessChain
	inputs.clear();
	for_count(int, idx, _outputDefinition.get_output_texture_count())
	{
		add_input(_outputDefinition.get_output_texture_definition(idx).get_name());
	}

	// (re)initialise outputRenderTargets with number of outputs from postProcessChain
	outputRenderTargets.clear();
	outputRenderTargets.set_size(_outputDefinition.get_output_texture_count());
}

void PostProcessChainOutputElement::do_post_process(PostProcessChainProcessingContext const & _processingContext)
{
	// store input connectors as output elements
	an_assert(inputs.get_size() == outputRenderTargets.get_size(), TXT("they should match thanks to constructor's actions"));
	auto inputConnector = inputs.begin();
	for_every(outputRenderTarget, outputRenderTargets)
	{
		*outputRenderTarget = inputConnector->inputRenderTarget;
		++inputConnector;
	}
	base::do_post_process(_processingContext); // will propagate outputs and clear inputs
}

void PostProcessChainOutputElement::release_render_targets()
{
	base::release_render_targets();
	for_every(outputRenderTarget, outputRenderTargets)
	{
		outputRenderTarget->clear();
	}
}

int PostProcessChainOutputElement::get_output_render_target_count() const
{
	return outputRenderTargets.get_size();
}

PostProcessRenderTargetPointer const & PostProcessChainOutputElement::get_output_render_target(int _index) const
{
	return outputRenderTargets[_index];
}
