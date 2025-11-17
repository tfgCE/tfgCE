#pragma once

#include "..\containers\array.h"
#include "..\types\name.h"
#include "graphInputConnector.h"
#include "graphOutputConnector.h"
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
	struct LoadingContext
	{
		int temp;
	};

	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> class Graph;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> class NodeContainer;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> class Node;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> struct Connector;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> struct Connection;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> struct InputConnector;
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> struct OutputConnector;

	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
	class Node
	{
		friend class Graph<GraphClass, NodeClass, InstanceClass, LoadingContextClass>;
		friend class NodeContainer<GraphClass, NodeClass, InstanceClass, LoadingContextClass>;
		friend struct InputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>;
		friend struct OutputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>;
	public:
		typedef Graph<GraphClass, NodeClass, InstanceClass, LoadingContextClass> Graph;
		typedef NodeContainer<GraphClass, NodeClass, InstanceClass, LoadingContextClass> NodeContainer;
		typedef Connector<GraphClass, NodeClass, InstanceClass, LoadingContextClass> Connector;
		typedef Connection<GraphClass, NodeClass, InstanceClass, LoadingContextClass> Connection;
		typedef InputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass> InputConnector;
		typedef OutputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass> OutputConnector;
		typedef InstanceData InstanceData;
		typedef InstanceDataSetup InstanceDataSetup;
		typedef ::Instance::DataHandler InstanceDataHandler;

	public:
		Node();
		virtual ~Node();

		Name const & get_name() const { return name; }

		virtual bool load_from_xml(IO::XML::Node const * _node, LoadingContextClass & _lc);
		virtual bool on_input_loaded_from_xml(int _inputIndex, IO::XML::Node const * _node, LoadingContextClass & _lc) { return true; }

		void disconnect(); // all

	public: // to override_/implement_ (called per node)
		virtual bool connect_inputs_and_outputs();
		virtual bool ready_to_use();
		virtual void handle_instance_data(REF_::Instance::DataHandler const & _dataHandler) const;

	public:
		Array<InputConnector> const & all_input_connectors() const { return inputs; }
		Array<OutputConnector> const & all_output_connectors() const { return outputs; }

		void clear_input_connectors();
		void clear_output_connectors();

		int add_input_connector(InputConnector const & _inputConnector);
		int add_output_connector(OutputConnector const & _outputConnector);

		int find_input_connector_index(Name const & _inputName) const;
		int find_output_connector_index(Name const & _outputName) const;

		Name const & get_input_connector_name(int _idx) const;
		Name const & get_output_connector_name(int _idx) const;

		NodeClass* get_as_node_class() { an_assert(selfAsNodeClass); return selfAsNodeClass; }
		NodeClass const * get_as_node_class() const { an_assert(selfAsNodeClass); return selfAsNodeClass; }

		GraphClass* get_in_graph() const { return inGraph; }

	protected:
		void set_name(Name const & _name) { name = _name; }
		void set_self_as_node_class(NodeClass* self) { selfAsNodeClass = self; }

	private: // friend struct OutputConnector;
		void disconnect_input(int _connectorIndex, OutputConnector* _connector);

	private: // friend struct InputConnector;
		void disconnect_output(int _connectorIndex, InputConnector* _connector);
		Array<OutputConnector> & access_output_connectors() { return outputs; }

	private: // friend class NodeContainer; // for all nodes
			 // friend class Graph; // for root node
		void add_to(GraphClass* _graph, NodeContainer* _nodeContainer);

	private:
		NodeClass* selfAsNodeClass;
		GraphClass* inGraph;
		NodeContainer* inContainer;
		Name name;
		Array<InputConnector> inputs;
		Array<OutputConnector> outputs;
	};

};
