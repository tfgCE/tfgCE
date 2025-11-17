template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
InputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::InputConnector()
{
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
InputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::~InputConnector()
{
	an_assert(!connection.is_connected(), TXT("should be disconnected before deletion"));
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
void InputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::disconnect()
{
	Connection wasConnected = connection;
	connection.disconnect();
	if (wasConnected.node)
	{
		wasConnected.node->disconnect_output(wasConnected.connectorIndexInNode, this);
	}
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
void InputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::disconnect(OutputConnector const * _connector)
{
	if (connection.node == _connector->get_node_owner() &&
		connection.connectorIndexInNode == _connector->get_connector_index())
	{
		disconnect();
	}
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
bool InputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::connect_within(NodeContainer* _withinContainer)
{
	bool result = true;
	if (connectedToNodeName.is_valid())
	{
		bool connected = false;
		if (NodeClass* node = _withinContainer->find_node(connectedToNodeName))
		{
			int connectorIndexInNode = node->find_output_connector_index(connectedToConnectorName);
			if (connectorIndexInNode != NONE)
			{
				connection.connect(node, connectorIndexInNode);
				node->access_output_connectors()[connectorIndexInNode].connected_by(base::get_node_owner(), base::get_connector_index());
				connected = true;
			}
			else
			{
				error(TXT("could not connect to found node \"%S\" but it doesn't have output connector \"%S\""), connectedToNodeName.to_char(), connectedToConnectorName.to_char());
			}
		}
		else
		{
			error(TXT("could not connect to not found node \"%S\""), connectedToNodeName.to_char());
		}
		if (!connected)
		{
			result = false;
		}
	}
	return result;
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
bool InputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);
	connectedToNodeName = _node->get_name_attribute(TXT("connectToNode"));
	connectedToConnectorName = _node->get_name_attribute(TXT("connectToConnector"));
	if (!connectedToNodeName.is_valid())
	{
		error_loading_xml(_node, TXT("name of node to connect to not given"));
		result = false;
	}
	if (!connectedToConnectorName.is_valid())
	{
		error_loading_xml(_node, TXT("name of connector to connect to not given"));
		result = false;
	}
	return result;
}
