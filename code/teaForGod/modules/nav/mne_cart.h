#pragma once

#include "..\..\..\framework\module\moduleNavElement.h"
#include "..\..\..\framework\nav\navNode.h"

namespace Framework
{
	struct PresencePath;
};

namespace TeaForGodEmperor
{
	namespace ModuleNavElements
	{
		class Cart
		: public Framework::ModuleNavElement
		{
			FAST_CAST_DECLARE(Cart);
			FAST_CAST_BASE(Framework::ModuleNavElement);
			FAST_CAST_END();

			typedef Framework::ModuleNavElement base;
		public:
			static Framework::RegisteredModule<Framework::ModuleNavElement> & register_itself();

			Cart(Framework::IModulesOwner* _owner);
			virtual ~Cart();

			void for_nodes(std::function<void(Framework::Nav::Node* _node)> _do) const;
			Framework::Nav::Node* get_cart_point_a_node() const { return cartPointANode.get(); }
			Framework::Nav::Node* get_cart_point_b_node() const { return cartPointBNode.get(); }
			Framework::Nav::Node::ConnectionSharedData* access_cart_point_a_connection() { return cartPointAConnection.get(); }
			Framework::Nav::Node::ConnectionSharedData* access_cart_point_b_connection() { return cartPointBConnection.get(); }

		public: // ModuleNavElement
			implement_ void add_to(Framework::Nav::Mesh* _navMesh);
			implement_ bool is_moveable() const { return true; }

		private:
			mutable Concurrency::MRSWLock navLock;
			Array<Framework::Nav::NodePtr> nodes; // just for reference (kept with lock as we might be accessing it while it is being built)
			Framework::Nav::NodePtr cartNodeA; // points on carts
			Framework::Nav::NodePtr cartNodeB;
			Framework::Nav::NodePtr cartPointANode; // points at cart stops, open, point outside to connect to other parts
			Framework::Nav::NodePtr cartPointBNode;
			Framework::Nav::Node::ConnectionSharedDataPtr cartPointAConnection;
			Framework::Nav::Node::ConnectionSharedDataPtr cartPointBConnection;

			void clear();
		};
	};
};
