#pragma once

#include "graph.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Graph
{
	/**
	 *	(Graph::)Instance holds instance data for graph.
	 *	This class is just base for actual InstanceClass and only thing it does it creates and destroys instanceData.
	 */
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
	class Instance
	{
	public:
		typedef Graph<GraphClass, NodeClass, InstanceClass, LoadingContextClass> Graph;
		typedef NodeContainer<GraphClass, NodeClass, InstanceClass, LoadingContextClass> NodeContainer;
		typedef Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass> Node;
		typedef Connector<GraphClass, NodeClass, InstanceClass, LoadingContextClass> Connector;
		typedef Connection<GraphClass, NodeClass, InstanceClass, LoadingContextClass> Connection;
		typedef InputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass> InputConnector;
		typedef OutputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass> OutputConnector;

	public:
		Instance(GraphClass const * _graph);
		virtual ~Instance();

		GraphClass const * get_graph() const { return graph; }
		InstanceData* get_data() const { return instanceData; }

	protected:
		GraphClass const * graph;
		InstanceData* instanceData;
	};

};
