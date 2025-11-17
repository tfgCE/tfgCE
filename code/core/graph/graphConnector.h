#pragma once

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
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> class NodeContainer;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> class Node;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> struct Connector;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> struct Connection;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> struct OutputConnector;

	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
	struct Connection
	{
		NodeClass* node;
		int connectorIndexInNode; // in node

		Connection();
		Connection(NodeClass* _node, int _connectorIndexInNode);
		~Connection();

		void connect(NodeClass* _node, int _connectorIndexInNode);
		void disconnect();

		inline bool is_connected() const { return node != nullptr; }
	};

	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
	struct Connector
	{
	public:
		typedef Graph<GraphClass, NodeClass, InstanceClass, LoadingContextClass> Graph;
		typedef NodeContainer<GraphClass, NodeClass, InstanceClass, LoadingContextClass> NodeContainer;
		typedef Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass> Node;
		typedef Connection<GraphClass, NodeClass, InstanceClass, LoadingContextClass> Connection;

	public:
		Connector();
		~Connector();

		bool load_from_xml(IO::XML::Node const * _node);

		void setup(NodeClass* _nodeOwner, int _connectorIndex);

		inline NodeClass* get_node_owner() const { return nodeOwner; }
		inline Name const & get_name() const { return name; }
		inline int get_connector_index() const { return connectorIndex; }

	protected:
		Connector(Name const & _name);

	private:
		NodeClass* nodeOwner;
		Name name;
		int connectorIndex;
	};

};
