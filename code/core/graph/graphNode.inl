template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::Node()
: selfAsNodeClass(nullptr)
, inGraph(nullptr)
, inContainer(nullptr)
{
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
void Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::add_to(GraphClass* _graph, NodeContainer* _nodeContainer)
{
	an_assert(selfAsNodeClass, TXT("should be set by this point"));
	if (inGraph)
	{
		inGraph->remove(selfAsNodeClass);
	}
	inGraph = _graph;
	inContainer = _nodeContainer;
	inGraph->add(selfAsNodeClass);
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::~Node()
{
	if (inGraph)
	{
		an_assert(selfAsNodeClass, TXT("should be set by this point"));
		inGraph->remove(selfAsNodeClass);
	}
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
void Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::disconnect()
{
	for_every(inputConnector, inputs)
	{
		inputConnector->disconnect();
	}
	for_every(outputConnector, outputs)
	{
		outputConnector->disconnect();
	}
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
bool Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::connect_inputs_and_outputs()
{
	bool result = true;
	// connect through inputs
	for_every(inputConnector, inputs)
	{
		result &= inputConnector->connect_within(inContainer);
	}
	return result;
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
bool Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::ready_to_use()
{
	return true;
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
void Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::handle_instance_data(REF_::Instance::DataHandler const & _dataHandler) const
{
	// nothing here
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
bool Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::load_from_xml(IO::XML::Node const * _node, LoadingContextClass & _lc)
{
	bool result = true;
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("name")))
	{
		name = attr->get_as_name();
	}
	if (! name.is_valid())
	{
		error_loading_xml(_node, TXT("lack of proper name for graph node"));
		result = false;
	}
	for_every(connectorNode, _node->children_named(TXT("input")))
	{
		InputConnector connector;
		if (connector.load_from_xml(connectorNode))
		{
			int newConnectorIndex = add_input_connector(connector);
			result &= on_input_loaded_from_xml(newConnectorIndex, connectorNode, _lc);
		}
		else
		{
			result = false;
		}
	}
	for_every(connectorNode, _node->children_named(TXT("output")))
	{
		OutputConnector connector;
		if (connector.load_from_xml(connectorNode))
		{
			add_output_connector(connector);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
void Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::clear_input_connectors()
{
	inputs.clear();
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
void Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::clear_output_connectors()
{
	outputs.clear();
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
int Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::add_input_connector(InputConnector const & _inputConnector)
{
	int newConnectorIndex = inputs.get_size();
	inputs.push_back(_inputConnector);
	inputs.get_last().setup(get_as_node_class(), newConnectorIndex);
	return newConnectorIndex;
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
int Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::add_output_connector(OutputConnector const & _outputConnector)
{
	int newConnectorIndex = outputs.get_size();
	outputs.push_back(_outputConnector);
	outputs.get_last().setup(get_as_node_class(), newConnectorIndex);
	return newConnectorIndex;
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
int Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::find_input_connector_index(Name const & _inputName) const
{
	int index = 0;
	for_every(input, inputs)
	{
		if (input->get_name() == _inputName)
		{
			return index;
		}
		++index;
	}
	return NONE;
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
int Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::find_output_connector_index(Name const & _outputName) const
{
	int index = 0;
	for_every(output, outputs)
	{
		if (output->get_name() == _outputName)
		{
			return index;
		}
		++index;
	}
	return NONE;
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
Name const & Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::get_input_connector_name(int _idx) const
{
	if (inputs.is_index_valid(_idx))
	{
		return inputs[_idx].get_name();
	}
	return Name::invalid();
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
Name const & Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::get_output_connector_name(int _idx) const
{
	if (outputs.is_index_valid(_idx))
	{
		return outputs[_idx].get_name();
	}
	return Name::invalid();
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
void Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::disconnect_input(int _connectorIndex, OutputConnector* _connector)
{
	an_assert(inputs.is_index_valid(_connectorIndex));
	inputs[_connectorIndex].disconnect(_connector);
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
void Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::disconnect_output(int _connectorIndex, InputConnector* _connector)
{
	an_assert(outputs.is_index_valid(_connectorIndex));
	outputs[_connectorIndex].disconnect(_connector);
}
