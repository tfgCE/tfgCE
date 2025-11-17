#include "postProcessNode.h"

#include "..\postProcess.h"
#include "..\postProcessRenderTargetPointer.h"

#include "..\..\debugSettings.h"

#include "..\..\library\library.h"

#include "..\..\..\core\graph\graphImplementation.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

PostProcessNode::AdditionalInputData::AdditionalInputData()
{
}

bool PostProcessNode::AdditionalInputData::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	if (IO::XML::Attribute const* attr = _node->get_attribute(TXT("filteringMag")))
	{
		::System::TextureFiltering::Type newFiltering;
		if (::System::TextureFiltering::parse(attr->get_as_string(), newFiltering))
		{
			inputFilteringMag = newFiltering;
		}
	}

	if (IO::XML::Attribute const* attr = _node->get_attribute(TXT("filteringMin")))
	{
		::System::TextureFiltering::Type newFiltering;
		if (::System::TextureFiltering::parse(attr->get_as_string(), newFiltering))
		{
			inputFilteringMin = newFiltering;
		}
	}

	return result;
}

//

REGISTER_FOR_FAST_CAST(PostProcessNode);

PostProcessNode::PostProcessNode()
{
	set_self_as_node_class(this);
}

PostProcessNode::~PostProcessNode()
{
}

bool PostProcessNode::load_shader_program(Library* _library)
{
	bool result = true;
	return result;
}

bool PostProcessNode::for_every_shader_program_instance(InstanceData & _instanceData, ForShaderProgramInstance _for_every_shader_program_instance) const
{
	bool result = true;
	return result;
}

void PostProcessNode::manage_activation_using(InstanceData & _instanceData, SimpleVariableStorage const & _storage) const
{
}

void PostProcessNode::handle_instance_data(REF_ InstanceDataHandler const & _dataHandler) const
{
	base::handle_instance_data(_dataHandler);

	_dataHandler.handle(iPostProcessingDone);
}

void PostProcessNode::initialise(InstanceData & _instanceData) const
{
	// nothing here
}

void PostProcessNode::release_render_targets(InstanceData & _instanceData) const
{
	// nothing here
}

void PostProcessNode::do_post_process(InstanceData & _instanceData, PostProcessGraphProcessingContext const& _processingContext) const
{
	an_assert(! was_post_processing_done(_instanceData));
	mark_post_processing_done(_instanceData);

	// process inputs (if they were processed, we won't process them again)
	for_every(inputConnector, all_input_connectors())
	{
		if (inputConnector->get_connection().node)
		{
			inputConnector->get_connection().node->do_post_process(_instanceData, _processingContext);
		}
	}

#ifdef AN_LOG_POST_PROCESS
	if (PostProcessDebugSettings::log)
	{
		output(TXT(" :: processed %S (%i)"), get_name().to_char(), this);
	}
#endif
}

void PostProcessNode::ready_for_post_processing(InstanceData & _instanceData) const
{
	_instanceData[iPostProcessingDone] = false;
}

void PostProcessNode::mark_post_processing_done(InstanceData & _instanceData) const
{
	_instanceData[iPostProcessingDone] = true;
}

bool PostProcessNode::was_post_processing_done(InstanceData & _instanceData) const
{
	return _instanceData[iPostProcessingDone];
}

void PostProcessNode::propagate_outputs(InstanceData & _instanceData) const
{
#ifdef AN_LOG_POST_PROCESS
	if (PostProcessDebugSettings::log)
	{
		output(TXT("    NODE %S (%i)"), get_name().to_char(), this);
		output(TXT("      inputs (%S | %i)"), get_name().to_char(), this);
		for_every(inputConnector, all_input_connectors())
		{
			if (inputConnector->get_connection().node)
			{
				PostProcessRenderTargetPointer const & inputRenderTarget = inputConnector->get_connection().node->get_output(_instanceData, inputConnector->get_connection().connectorIndexInNode);

#ifdef AN_LOG_POST_PROCESS
				if (PostProcessDebugSettings::log)
				{
					output(TXT("        +-(%i) %S <-[%S:%S] (rt:%i)"), for_everys_index(inputConnector),
						inputConnector->get_name().to_char(),
						inputConnector->get_connect_to_node_name().to_char(),
						inputConnector->get_connect_to_connector_name().to_char(),
						inputRenderTarget.get_render_target());
				}
#endif
			}
		}
	}
#endif
#ifdef AN_LOG_POST_PROCESS
	if (PostProcessDebugSettings::log)
	{
		output(TXT("      outputs (%S | %i)"), get_name().to_char(), this);
	}
#endif
	int outputIndex = 0;
	for_every(outputConnector, all_output_connectors())
	{
		if (!outputConnector->all_connections().is_empty())
		{
			PostProcessRenderTargetPointer const & outputRenderTarget = get_output(_instanceData, outputIndex);

#ifdef AN_LOG_POST_PROCESS
			if (PostProcessDebugSettings::log)
			{
				output(TXT("        +-(%i) %S (rt:%i)"), for_everys_index(outputConnector), outputConnector->get_name().to_char(), outputRenderTarget.get_render_target());
			}
#endif

			for_every(connection, outputConnector->all_connections())
			{
				connection->node->gather_input(_instanceData, connection->connectorIndexInNode, outputRenderTarget);
			}
		}
		++outputIndex;
	}
}

void PostProcessNode::gather_input(InstanceData & _instanceData, int _inputIndex, PostProcessRenderTargetPointer const & _renderTargetPointer) const
{
	an_assert(false, TXT("implement_, because this node needs this"));
}

PostProcessRenderTargetPointer const & PostProcessNode::get_output(InstanceData & _instanceData, int _outputIndex) const
{
	an_assert(false, TXT("implement_, because this node needs this"));
	return PostProcessRenderTargetPointer::empty();
}
