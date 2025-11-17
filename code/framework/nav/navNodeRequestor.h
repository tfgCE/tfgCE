#pragma once

#include "navDeclarations.h"

namespace Framework
{
	interface_class IModulesOwner;

	namespace Nav
	{
		/**
		 *	Node requestor is a small structure that requests nav node - nav node is informed that is being requested (and information about owner is stored there)
		 *	This might be used to create intelligent environment that is aware of NPCs going into particular places.
		 *	Player has to be handled separately as player's intentions are not known.
		 */
		struct NodeRequestor
		{
		public:
			NodeRequestor(IModulesOwner* _owner);
			~NodeRequestor();

			void request(Node* _node);
			void clear();

			IModulesOwner* get_owner() const { return owner; }

		private:
			IModulesOwner* owner = nullptr;
			NodePtr node;

		private: // inaccessible
			NodeRequestor(NodeRequestor const & _other);
			NodeRequestor& operator=(NodeRequestor const & _other);
		};
	}
}