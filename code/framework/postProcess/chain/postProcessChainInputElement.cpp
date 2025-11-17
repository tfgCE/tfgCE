#include "postProcessChainInputElement.h"

#include "postProcessChain.h"
#include "..\..\..\core\system\video\renderTarget.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(PostProcessChainInputElement);

PostProcessChainInputElement::Input::Input()
: usageCount(0)
{
}

void PostProcessChainInputElement::Input::clear_usage()
{
	usageCount = 0;
}

void PostProcessChainInputElement::Input::add_usage()
{
	++ usageCount;
}

//

PostProcessChainInputElement::PostProcessChainInputElement()
{
}

PostProcessChainInputElement::~PostProcessChainInputElement()
{
}

void PostProcessChainInputElement::use_input(PostProcessChain* _postProcessChain, ::System::RenderTarget* _renderTarget, Name const * _nameOverride)
{
	bool redoOutputs = false;
	::System::RenderTargetSetup const & useSetup = _renderTarget->get_setup();
	an_assert(_nameOverride == nullptr || useSetup.get_output_texture_count() == 1, TXT("atm name override_ should be used only for render targets with one output texture"));
	if (outputs.get_size() != useSetup.get_output_texture_count())
	{
		// different number of outputs
		redoOutputs = true;
	}
	else
	{
		// check if names are different - it should be enough as we link through names anyway
		for (int idx = 0; idx < outputs.get_size(); ++idx)
		{
			if ((_nameOverride == nullptr && outputs[idx].get_name() != useSetup.get_output_texture_definition(idx).get_name()) ||
				(_nameOverride != nullptr && outputs[idx].get_name() != *_nameOverride))
			{
				redoOutputs = true;
				break;
			}
		}
	}

	if (redoOutputs)
	{
		// create outputs
		outputs.clear();
		for (int idx = 0; idx < useSetup.get_output_texture_count(); ++idx)
		{
			add_output(_nameOverride ? *_nameOverride : useSetup.get_output_texture_definition(idx).get_name());
		}

		_postProcessChain->mark_linking_required();
	}

	// setup input render targets
	inputs.set_size(useSetup.get_output_texture_count());
	int idx = 0;
	for_every(input, inputs)
	{
		input->renderTarget.set(_renderTarget, idx);
		++idx;
	}
}

int PostProcessChainInputElement::get_output_render_target_count() const
{
	return inputs.get_size();
}

PostProcessRenderTargetPointer const & PostProcessChainInputElement::get_output_render_target(int _index) const
{
	return inputs[_index].renderTarget;
}

void PostProcessChainInputElement::do_post_process(PostProcessChainProcessingContext const & _processingContext)
{
	base::do_post_process(_processingContext); // will propagate outputs
	// release input render targets - they won't be used anymore
	for_every(input, inputs)
	{
		input->renderTarget.clear();
	}
}

void PostProcessChainInputElement::release_render_targets()
{
	base::release_render_targets();
	for_every(input, inputs)
	{
		input->renderTarget.clear();
	}
}

void PostProcessChainInputElement::clear_input_usage()
{
	for_every(input, inputs)
	{
		input->clear_usage();
	}
}

void PostProcessChainInputElement::add_input_usage(int _index)
{
	an_assert(inputs.is_index_valid(_index));
	inputs[_index].add_usage();
}

void PostProcessChainInputElement::setup_render_target_output_usage(::System::RenderTarget* _inputRenderTarget)
{
	_inputRenderTarget->mark_all_outputs_required();
#ifndef AN_DEVELOPMENT
	int inputIdx = 0;
	for_every(input, inputs)
	{
		if (!input->is_used())
		{
			_inputRenderTarget->mark_output_not_required(inputIdx);
		}
		++inputIdx;
	}
#endif
}
