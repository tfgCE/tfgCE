#pragma once

#include "..\containers\array.h"
#include "..\types\name.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Graph
{
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> class Graph;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> class Node;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> struct Connector;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> struct Connection;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> struct InputConnector;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> struct OutputConnector;

	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
	class NodeContainer
	{
	public:
		typedef Graph<GraphClass, NodeClass, InstanceClass, LoadingContextClass> Graph;
		typedef Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass> Node;
		typedef Connector<GraphClass, NodeClass, InstanceClass, LoadingContextClass> Connector;
		typedef Connection<GraphClass, NodeClass, InstanceClass, LoadingContextClass> Connection;
		typedef InputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass> InputConnector;
		typedef OutputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass> OutputConnector;

	public:
		NodeContainer(GraphClass* _inGraph, NodeClass* _inNode);
		virtual ~NodeContainer();

		NodeClass* find_node(Name const & _nodeName) const;

		Array<NodeClass*> const & all_nodes() const { return nodes; } // const but to just access nodes, not to add them

		virtual bool load_from_xml(IO::XML::Node const * _node, LoadingContextClass & _lc);

	protected:
		GraphClass* inGraph;
		NodeClass* inNode;
		Array<NodeClass*> nodes;
	};

};
