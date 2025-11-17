#include "postProcessInputNode.h"
#include "postProcessGraph.h"
#include "postProcessGraphProcessingContext.h"

#include "..\..\debugSettings.h"

#include "..\postProcess.h"

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(PostProcessInputNode);

PostProcessInputNode::PostProcessInputNode()
{
}

PostProcessInputNode::~PostProcessInputNode()
{
}

void PostProcessInputNode::handle_instance_data(REF_ InstanceDataHandler const & _dataHandler) const
{
	base::handle_instance_data(REF_ _dataHandler);

	_dataHandler.handle(iInputRenderTargets);
}

void PostProcessInputNode::initialise(InstanceData & _instanceData) const
{
	base::initialise(_instanceData);

	// same number as outputs
	_instanceData[iInputRenderTargets].set_size(all_output_connectors().get_size(), true);
}

void PostProcessInputNode::release_render_targets(InstanceData & _instanceData) const
{
	base::initialise(_instanceData);

	for_every(inputRenderTarget, _instanceData[iInputRenderTargets])
	{
		inputRenderTarget->clear();
	}
}

void PostProcessInputNode::do_post_process(InstanceData & _instanceData, PostProcessGraphProcessingContext const& _processingContext) const
{
	if (was_post_processing_done(_instanceData))
	{
		// already processed
		return;
	}

	base::do_post_process(_instanceData, _processingContext.for_previous());

	// fill input render targets basing on data provided by processing context
	auto outputConnector = all_output_connectors().begin_const();
	for_every(inputRenderTarget, _instanceData[iInputRenderTargets])
	{
		an_assert(_processingContext.inputRenderTargetsProvider);
		*inputRenderTarget = _processingContext.inputRenderTargetsProvider->get_input_render_target(outputConnector->get_name());
		inputRenderTarget->mark_next_frame_releasable();
#ifdef AN_LOG_POST_PROCESS
		if (PostProcessDebugSettings::log)
		{
			output(TXT("    +-input %S (rt:%i)"), outputConnector->get_name().to_char(), inputRenderTarget->get_render_target());
		}
#endif
		++ outputConnector;
	}

	// we have post processed outputs now, propagate them
	propagate_outputs(_instanceData);
}

PostProcessRenderTargetPointer const & PostProcessInputNode::get_output(InstanceData & _instanceData, int _outputIndex) const
{
	return _instanceData[iInputRenderTargets][_outputIndex];
}
