#include "postProcessOutputNode.h"

#include "postProcessGraph.h"
#include "postProcessGraphProcessingContext.h"

#include "..\postProcess.h"

#include "..\..\debugSettings.h"

#include "..\..\..\core\system\video\renderTarget.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(PostProcessOutputNode);

PostProcessOutputNode::PostProcessOutputNode()
{
}

PostProcessOutputNode::~PostProcessOutputNode()
{
}

void PostProcessOutputNode::handle_instance_data(REF_ InstanceDataHandler const & _dataHandler) const
{
	base::handle_instance_data(_dataHandler);

	_dataHandler.handle(iOutputRenderTargets);
}

void PostProcessOutputNode::initialise(InstanceData & _instanceData) const
{
	base::initialise(_instanceData);

	// make same number of outputs as there are inputs
	_instanceData[iOutputRenderTargets].set_size(all_input_connectors().get_size());
}

void PostProcessOutputNode::release_render_targets(InstanceData & _instanceData) const
{
	base::initialise(_instanceData);

	for_every(renderTargetPointer, _instanceData[iOutputRenderTargets])
	{
		renderTargetPointer->clear();
	}
}

void PostProcessOutputNode::do_post_process(InstanceData & _instanceData, PostProcessGraphProcessingContext const& _processingContext) const
{
	if (was_post_processing_done(_instanceData))
	{
		// already processed
		return;
	}

	base::do_post_process(_instanceData, _processingContext);

#ifdef AN_LOG_POST_PROCESS
	if (PostProcessDebugSettings::log)
	{
		for_count(int, i, get_output_render_target_count(&_instanceData))
		{
			Name name;
			auto const & rtPtr = get_output_render_target(&_instanceData, i, &name);
			output(TXT("    +-output %S (rt:%i)"), name.to_char(), rtPtr.get_render_target());
		}
	}
#endif

	{	// set filtering for output textures using their default setup or additional input data
		int index = 0;
		Array<PostProcessRenderTargetPointer>& outputRenderTargets = _instanceData[iOutputRenderTargets];
		for_every(inputConnector, all_input_connectors())
		{
			auto& outputRenderTarget = outputRenderTargets[index];
			if (auto* rt = outputRenderTarget.get_render_target())
			{
				auto const & outputDef = outputRenderTargets[index].get_render_target()->get_setup().get_output_texture_definition(outputRenderTargets[index].get_data_texture_index());
				AdditionalInputData const& aid = additionalInputData[index];
				rt->change_filtering(outputRenderTarget.get_data_texture_index(), aid.inputFilteringMag.get(outputDef.get_filtering_mag()), aid.inputFilteringMin.get(outputDef.get_filtering_min()));
			}
			++index;
		}
	}
}

int PostProcessOutputNode::get_output_render_target_count(InstanceData * _data) const
{
	return ((*_data)[iOutputRenderTargets]).get_size();
}

PostProcessRenderTargetPointer const & PostProcessOutputNode::get_output_render_target(InstanceData * _data, int _outputIndex, OUT_ OPTIONAL_ Name * _outputRenderTargetName) const
{
	an_assert((*_data)[iOutputRenderTargets].is_index_valid(_outputIndex));
	if (_outputRenderTargetName)
	{
		an_assert(outputRenderTargetNames.is_index_valid(_outputIndex));
		*_outputRenderTargetName = outputRenderTargetNames[_outputIndex];
	}
	return ((*_data)[iOutputRenderTargets])[_outputIndex];
}

void PostProcessOutputNode::gather_input(InstanceData & _instanceData, int _inputIndex, PostProcessRenderTargetPointer const & _renderTargetPointer) const
{
	_instanceData[iOutputRenderTargets][_inputIndex] = _renderTargetPointer;
}

bool PostProcessOutputNode::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	mainOutputRenderTargetName = _node->get_name_attribute(TXT("main"), mainOutputRenderTargetName);
	mainOutputRenderTargetName = _node->get_name_attribute(TXT("mainOutput"), mainOutputRenderTargetName);

	if (all_input_connectors().get_size() > 1)
	{
		if (!mainOutputRenderTargetName.is_valid())
		{
			error_loading_xml(_node, TXT("no mainOutput defined, but multiple inputs exist"));
			result = false;
		}
		else if (outputRenderTargetNames.get_size() != all_input_connectors().get_size())
		{
			error_loading_xml(_node, TXT("number of output render target names and input connectors does not match"));
			result = false;
		}
		else
		{
			bool mainOutputExists = false;
			for_every(ortn, outputRenderTargetNames)
			{
				if (*ortn == mainOutputRenderTargetName)
				{
					mainOutputExists = true;
				}
			}
			if (!mainOutputExists)
			{
				error_loading_xml(_node, TXT("couldn't find main output render target \"%S\" among defined output render target names"), mainOutputRenderTargetName.to_char());
				result = false;
			}
		}
	}

	mainOutputRenderTargetIndex = NONE;
	for_every(outputRenderTargetName, outputRenderTargetNames)
	{
		if (mainOutputRenderTargetName == *outputRenderTargetName)
		{
			mainOutputRenderTargetIndex = for_everys_index(outputRenderTargetName);
		}
	}
	if (all_input_connectors().get_size() == 1)
	{
		mainOutputRenderTargetIndex = 0;
	}

	return result;
}

bool PostProcessOutputNode::on_input_loaded_from_xml(int _inputIndex, IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::on_input_loaded_from_xml(_inputIndex, _node, _lc);

	while (outputRenderTargetNames.get_size() <= _inputIndex)
	{
		outputRenderTargetNames.push_back(Name::invalid());
	}
	outputRenderTargetNames[_inputIndex] = _node->get_name_attribute(TXT("name"), outputRenderTargetNames[_inputIndex]);
	outputRenderTargetNames[_inputIndex] = _node->get_name_attribute(TXT("outputRenderTarget"), outputRenderTargetNames[_inputIndex]);

	// make sure there is such input data
	additionalInputData.set_size(max(_inputIndex + 1, additionalInputData.get_size()));

	result &= additionalInputData[_inputIndex].load_from_xml(_node, _lc);

	return result;
}
