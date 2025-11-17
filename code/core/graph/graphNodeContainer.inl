template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
NodeContainer<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::NodeContainer(GraphClass* _inGraph, NodeClass* _inNode)
: inGraph(_inGraph)
, inNode(_inNode)
{
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
NodeContainer<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::~NodeContainer()
{
	// first disconnect all
	for_every_ptr(node, nodes)
	{
		node->disconnect();
	}
	// then delete them
	for_every_ptr(node, nodes)
	{
		delete node;
	}
	nodes.clear_and_prune();
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
bool NodeContainer<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::load_from_xml(IO::XML::Node const * _node, LoadingContextClass & _lc)
{
	bool result = true;
	for_every(childNode, _node->all_children())
	{
		if (NodeClass* node = inGraph->create_node(childNode->get_name(), this))
		{
			node->add_to(inGraph, this);
			if (node->load_from_xml(childNode, _lc))
			{
				nodes.push_back(node);
			}
			else
			{
				error(TXT("problem loading graph node"));
				delete node;
				result = false;
			}
		}
		else
		{
			error(TXT("graph node \"%S\" not recognised"), childNode->get_name().to_char());
			result = false;
		}
	}
	return result;
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
NodeClass* NodeContainer<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::find_node(Name const & _nodeName) const
{
	for_every_ptr(node, nodes)
	{
		if (node->get_name() == _nodeName)
		{
			return node;
		}
	}
	return nullptr;
}
