template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
Instance<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::Instance(GraphClass const * _graph)
: graph(_graph)
, instanceData(nullptr)
{
	instanceData = new InstanceData(cast_to_nonconst<>(graph), graph->get_instance_data_setup());
}

template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
Instance<GraphClass, NodeClass, InstanceClass, LoadingContextClass>::~Instance()
{
	delete_and_clear(instanceData);
}
