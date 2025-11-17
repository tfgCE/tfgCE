template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
OutputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::OutputConnector()
{
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
OutputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::OutputConnector(Name const & _name)
: base(_name)
{
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
OutputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::~OutputConnector()
{
	an_assert(connections.is_empty(), TXT("should be disconnected before deletion"));
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
void OutputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::disconnect()
{
	while (!connections.is_empty())
	{
		Connection connection = connections.get_last();
		connections.pop_back();
		connection.node->disconnect_input(connection.connectorIndexInNode, this);
	}
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
void OutputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::disconnect(InputConnector const * _connector)
{
	for_every(connection, connections)
	{
		if (connection->node == _connector->get_node_owner() &&
			connection->connectorIndexInNode == _connector->get_connector_index())
		{
			Connection oldConnection = connections.get_last();
			connections.pop_back();
			oldConnection.node->disconnect_input(oldConnection.connectorIndexInNode, this);
#ifdef AN_ASSERT
			for_every(connection, connections)
			{
				an_assert(connection->node != _connector->get_node_owner() || connection->connectorIndexInNode != _connector->get_connector_index(), TXT("still connected?"));
			}
#endif
			break;
		}
	}
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
void OutputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::connected_by(NodeClass* _node, int _connectorIndex)
{
	connections.push_back(Connection(_node, _connectorIndex));
}
