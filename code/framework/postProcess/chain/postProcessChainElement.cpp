#include "postProcessChainElement.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(PostProcessChainElement);

PostProcessChainElement::PostProcessChainElement()
{
}

PostProcessChainElement::~PostProcessChainElement()
{
	clear_connections();
}

void PostProcessChainElement::clear_connections()
{
	for_every(inputConnector, inputs)
	{
		inputConnector->clear_connection();
	}
	for_every(outputConnector, outputs)
	{
		outputConnector->clear_connections();
	}
}

void PostProcessChainElement::release_render_targets()
{
	for_every(inputConnector, inputs)
	{
		inputConnector->release_render_target();
	}
}

void PostProcessChainElement::gather_input(int _index)
{
	an_assert(inputs.is_index_valid(_index));
	PostProcessChainInputConnector & input = inputs[_index];
	if (input.connection.connectedTo)
	{
		input.inputRenderTarget = input.connection.connectedTo->get_output_render_target(input.connection.connectedToIndex);
	}
	else
	{
		input.inputRenderTarget.clear();
	}
}

void PostProcessChainElement::propagate_outputs()
{
	for_every(outputConnector, outputs)
	{
		for_every(connection, outputConnector->connections)
		{
			connection->connectedTo->gather_input(connection->connectedToIndex);
		}
	}
}

int PostProcessChainElement::get_output_render_target_count() const
{
	an_assert(false, TXT("PostProcessChainElement doesn't provide any render targets"));
	
	return 0;
}

PostProcessRenderTargetPointer const & PostProcessChainElement::get_output_render_target(int _index) const
{
	an_assert(false, TXT("PostProcessChainElement doesn't provide any render targets"));

	return PostProcessRenderTargetPointer::empty();
}

void PostProcessChainElement::add_input(Name const & _name)
{
	PostProcessChainInputConnector newInput = PostProcessChainInputConnector();
	newInput.setup(_name, inputs.get_size());
	inputs.push_back(newInput);
}

void PostProcessChainElement::add_output(Name const & _name)
{
	PostProcessChainOutputConnector newOutput = PostProcessChainOutputConnector();
	newOutput.setup(_name, outputs.get_size());
	outputs.push_back(newOutput);
}

void PostProcessChainElement::do_post_process(PostProcessChainProcessingContext const & _processingContext)
{
	// all nodes connected to outputs should store render targets they want to use
	propagate_outputs();
	// we no longer need inputs, get rid of them
	for_every(inputConnector, inputs)
	{
		inputConnector->release_render_target();
	}
}

void PostProcessChainElement::for_every_shader_program_instance(ForShaderProgramInstance _for_every_shader_program_instance)
{
	// nothing
}

