template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
Connection<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::Connection()
: node(nullptr)
, connectorIndexInNode(NONE)
{
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
Connection<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::Connection(NodeClass* _node, int _connectorIndexInNode)
: node(_node)
, connectorIndexInNode(_connectorIndexInNode)
{
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
Connection<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::~Connection()
{
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
void Connection<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::connect(NodeClass* _node, int _connectorIndexInNode)
{
	an_assert(node == nullptr, TXT("should be disconnected when connecting"));
	node = _node;
	connectorIndexInNode = _connectorIndexInNode;
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
void Connection<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::disconnect()
{
	node = nullptr;
	connectorIndexInNode = NONE;
}

//

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
Connector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::Connector()
: nodeOwner(nullptr)
, connectorIndex(NONE)
{
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
Connector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::Connector(Name const & _name)
: nodeOwner(nullptr)
, name(_name)
, connectorIndex(NONE)
{
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
Connector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::~Connector()
{
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
void Connector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::setup(NodeClass* _nodeOwner, int _connectorIndex)
{
	nodeOwner = _nodeOwner;
	connectorIndex = _connectorIndex;
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
bool Connector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	name = _node->get_name_attribute(TXT("name"));
	if (!name.is_valid())
	{
		error_loading_xml(_node, TXT("name of connector not given"));
		result = false;
	}
	return result;
}
