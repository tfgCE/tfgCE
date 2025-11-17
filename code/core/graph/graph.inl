template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
Graph<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::Graph()
: selfAsGraphClass(nullptr)
, rootNode(nullptr)
{
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
Graph<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::~Graph()
{
	delete_and_clear(rootNode);
	an_assert(allNodes.is_empty());
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
bool Graph<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::ready_to_use()
{
	an_assert(rootNode, TXT("should have root node by this point"));
	bool result = true;
	// connect inputs and outputs first to have solid graph
	for_every_ptr(node, allNodes)
	{
		result &= node->connect_inputs_and_outputs();
	}
	// ready for use
	for_every_ptr(node, allNodes)
	{
		result &= node->ready_to_use();
	}
	// after nodes were readied, prepare setup for instance data
	InstanceDataHandler::prepare_setup_and_datums(this, &instanceDataSetup);
	return result;
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
void Graph<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::handle_instance_data(REF_ InstanceDataHandler const & _dataHandler) const
{
	an_assert(rootNode, TXT("should have root node by this point"));
	for_every_ptr(node, allNodes)
	{
		node->handle_instance_data(REF_ _dataHandler);
	}
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
InstanceClass* Graph<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::create_instance() const
{
	an_assert(selfAsGraphClass, TXT("should be already set"));
	return new InstanceClass(selfAsGraphClass);
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
bool Graph<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::load_from_xml(IO::XML::Node const * _node, LoadingContextClass & _lc)
{
	bool result = true;
	create_root_node_if_missing();
	result &= rootNode->load_from_xml(_node, _lc);
	return result;
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
void Graph<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::create_root_node_if_missing()
{
	if (!rootNode)
	{
		an_assert(selfAsGraphClass, TXT("should be already set"));
		set_root_node(create_root_node());
		rootNode->add_to(selfAsGraphClass, nullptr);
	}
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
void Graph<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::add(NodeClass* _node)
{
	an_assert(!allNodes.does_contain(_node));
	allNodes.push_back(_node);
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
void Graph<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::remove(NodeClass* _node)
{
	an_assert(allNodes.does_contain(_node));
	allNodes.remove(_node);
}
