#pragma once

#include "..\types\name.h"

#include "graphConnector.h"

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
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> class NodeContainer;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> class Node;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> struct Connector;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> struct Connection;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> struct OutputConnector;

	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
	struct InputConnector
	: public Connector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>
	{
	public:
		typedef OutputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass> OutputConnector;

#ifdef AN_CLANG
		typedef NodeContainer<GraphClass, NodeClass, InstanceClass, LoadingContextClass> NodeContainer;
		typedef Connection<GraphClass, NodeClass, InstanceClass, LoadingContextClass> Connection;
		typedef Connector<GraphClass, NodeClass, InstanceClass, LoadingContextClass> base;
#else
		typedef Connector base;
#endif
	public:
		InputConnector();
		~InputConnector();

		bool connect_within(NodeContainer* _withinContainer);
		void disconnect();
		void disconnect(OutputConnector const * _connector);

		bool load_from_xml(IO::XML::Node const * _node);

		Connection const & get_connection() const { return connection; }

		Name const & get_connect_to_node_name() const { return connectedToNodeName; }
		Name const & get_connect_to_connector_name() const { return connectedToConnectorName; }

	private:
		Name connectedToNodeName;
		Name connectedToConnectorName;

		Connection connection;
	};

};
