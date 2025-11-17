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
	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass> struct InputConnector;

	template <typename GraphClass, typename NodeClass, typename InstanceClass, typename LoadingContextClass>
	struct OutputConnector
	: public Connector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>
	{
		friend struct InputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass>;
	public:
		typedef InputConnector<GraphClass, NodeClass, InstanceClass, LoadingContextClass> InputConnector;

#ifdef AN_CLANG
		typedef Connection<GraphClass, NodeClass, InstanceClass, LoadingContextClass> Connection;
		typedef Connector<GraphClass, NodeClass, InstanceClass, LoadingContextClass> base;
#else
		typedef Connector base;
#endif
	public:
		OutputConnector();
		OutputConnector(Name const & _name);
		~OutputConnector();

		OutputConnector& temp_ref() { return *this; }

		void disconnect();
		void disconnect(InputConnector const * _connector);

		Array<Connection> const & all_connections() const { return connections; }

	private: // friend struct InputConnector;
		void connected_by(NodeClass* _node, int _connectorIndex);

	private:
		Array<Connection> connections;

	};

};
