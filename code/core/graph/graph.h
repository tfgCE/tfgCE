#pragma once

#include "graphNode.h"
#include "graphNodeContainer.h"
#include "graphInstanceData.h"
#include "..\instance\instanceDataHandler.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Graph
{
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> class NodeContainer;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> class Node;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> class Instance;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> struct Connector;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> struct Connection;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> struct InputConnector;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> struct OutputConnector;

	/**
	 *	Using graph derived classes
	 *
	 *	Graph's constructor should do this:
	 *		set_self_as_graph_class(this);
	 *		create_root_node_if_missing();
	 *
	 *	Node's constructor should do this:
	 *		set_self_as_node_class(this);
	 *
	 *	After loading everything, adding all nodes, this should be done:
	 *		graph->ready_to_use();
	 */
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
	class Graph
	: public ::Instance::IDataSource
	{
		friend class Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass>;
	public:
		typedef NodeContainer<GraphClass, NodeClass, InstanceClass, LoadingContextClass> NodeContainer;
		typedef Node<GraphClass, NodeClass, InstanceClass, LoadingContextClass> Node;
		typedef Instance<GraphClass, NodeClass, InstanceClass, LoadingContextClass> Instance;
		typedef Connector<GraphClass, NodeClass, InstanceClass, LoadingContextClass> Connector;
		typedef Connection<GraphClass, NodeClass, InstanceClass, LoadingContextClass> Connection;
		typedef InputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass> InputConnector;
		typedef OutputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass> OutputConnector;
		typedef InstanceData InstanceData;
		typedef InstanceDataSetup InstanceDataSetup;
		typedef ::Instance::DataHandler InstanceDataHandler;

	public:
		Graph();
		virtual ~Graph();

		bool ready_to_use();
		InstanceClass* create_instance() const;

		// implement_ by actual graph
		virtual NodeClass* create_node(String const & _name, NodeContainer* _nodeContainer) { an_assert(false, TXT("implement_!")); return nullptr; }
		virtual NodeClass* create_root_node() { an_assert(false, TXT("implement_!")); return nullptr; }

		virtual void set_root_node(NodeClass* _rootNode) { rootNode = _rootNode; }

		virtual bool load_from_xml(IO::XML::Node const * _node, LoadingContextClass & _lc);

		Array<NodeClass*> & access_all_nodes() { return allNodes; }
		Array<NodeClass*> const & get_all_nodes() const { return allNodes; }

		InstanceDataSetup const * get_instance_data_setup() const { return &instanceDataSetup; }

	protected: // ::Instance::IDataSource
		implement_ void handle_instance_data(REF_ InstanceDataHandler const & _dataHandler) const;

	protected: // friend class Node;
		void add(NodeClass* _node);
		void remove(NodeClass* _node);

	protected:
		GraphClass* selfAsGraphClass;
		NodeClass* rootNode;
		Array<NodeClass*> allNodes; // list of all nodes in graph

		InstanceDataSetup instanceDataSetup;

		void set_self_as_graph_class(GraphClass* self) { selfAsGraphClass = self; }
		void create_root_node_if_missing();
	};

};
