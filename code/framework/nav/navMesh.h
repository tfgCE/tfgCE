#pragma once

#include "navDeclarations.h"

#include "..\..\core\containers\array.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\types\name.h"
#include "..\..\core\types\optional.h"

struct Transform;

namespace Framework
{
	class Room;

	namespace Nav
	{
		struct PlacementAtNode;

		namespace Tasks
		{
			class BuildNavMesh;
			class DestroyNavMesh;
		};

		class Mesh
		: public RefCountObject
		{
		public:
			Mesh(Room* _inRoom, Name const & _id = Name::invalid());

			Room* get_room() const { return inRoom; }

			Name const & get_id() const { return id; }
			Array<NodePtr> & access_nodes() { return nodes; }
			Array<NodePtr> const & get_nodes() const { return nodes; }

			bool is_transport_route(int _groupId) const; // checks if there are just two doors at this group meaning that there are just two exits

		public:
			void add(Node* _node);
			void remove(Node* _node);
			void clear();

			void outdate();
			bool is_outdated() const { return inRoom == nullptr; }

		public:
			PlacementAtNode find_location(Transform const & _placement, OUT_ float * _dist = nullptr, Optional<int> const & _navMeshKeepGroup = NP) const;

		public:
			void unify_open_nodes();
			void connect_open_nodes();

		public:
			void debug_draw();

		private: friend class Tasks::BuildNavMesh;
				 friend class Tasks::DestroyNavMesh;
			~Mesh(); // should be destroyed with a task
			void assign_groups();
			void outdate_irrelevant();
			void remove_outdated(); // only during building!

		private:
			Name id; // nav mesh identifier, although for time being I expect to work with one kind (even if that would be hacky)
			Array<NodePtr> nodes;

			Room* inRoom = nullptr;

			int groupCount = 0;
		};
	}
}