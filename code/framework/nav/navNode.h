#pragma once

#include "navDeclarations.h"
#include "navFlags.h"
#include "navPlacementAtNode.h"

#include "..\..\core\fastCast.h"
#include "..\..\core\concurrency\mrswLock.h"
#include "..\..\core\containers\array.h"
#include "..\..\core\math\math.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\memory\safeObject.h"

namespace Framework
{
	interface_class IModulesOwner;
	class DoorInRoom;
	class Room;

	namespace Nav
	{
		class Mesh;
		struct NodeRequestor;

		namespace Tasks
		{
			class FindPath;
			class GetRandomLocation;
			class GetRandomPath;
			class BuildNavMesh;
		};

		class Node
		: public RefCountObject
		{
			FAST_CAST_DECLARE(Node);
			FAST_CAST_END();

		public:
			struct ConnectionSharedData
			: public PooledObject<ConnectionSharedData>
			, public RefCountObject
			{
			public:
				void set_through_open(bool _throughOpen = true) { throughOpen = _throughOpen; }

				void set_blocked_temporarily(bool _blockedTemporarily = true) { blockedTemporarily = _blockedTemporarily; todo_note(TXT("for time being this is fine, but maybe we should store requests to block temporarily somewhere, as almost immediate tasks?")); }
				bool is_blocked_temporarily() const { return blockedTemporarily; }

				void outdate() { outdated = true; }
				bool is_outdated() const { return outdated; }

			private:
				bool throughOpen = false; // both nodes are open and this is connection created as connection between open nodes
				bool outdated = false;
				bool blockedTemporarily = false; // temporarily blocked
			};
			typedef RefCountObjectPtr<ConnectionSharedData> ConnectionSharedDataPtr;

			struct Connection
			{
				Connection() {}
				Connection(Node* _node) : to(_node) {}

				bool is_outdated() const { return !data.is_set() || data->is_outdated(); }
				void outdate() { if (data.is_set()) { data.get()->outdate(); } }

				ConnectionSharedData * get_data() const { return data.get(); }

				Node* get_to() const { return to.get(); }

				void set_edge_idx(int _edgeIdx) { edgeIdx = _edgeIdx; }
				int get_edge_idx() const { return edgeIdx.get(NONE); }

			protected: friend class Node;
				NodePtr to;
				ConnectionSharedDataPtr data;
				Optional<int> edgeIdx; // this is useful for connecting two convex polygon nodes, to know through which edge should we connect to gracefully string pull
			};

		public:
			Node();
			virtual ~Node();

			bool is_outdated() const { return outdated; }
			void outdate(); // may result in deletion/release

			void place_WS(Transform const & _placement, IModulesOwner* _placementOwner, Optional<bool> const & _onMoveableOwner = NP);
			void place_LS(Transform const & _placement, IModulesOwner* _placementOwner, Optional<bool> const & _onMoveableOwner = NP);

			bool is_blocked_temporarily_stop() const { return blockedTemporarilyStop; }
			void be_blocked_temporarily_stop(bool _blockedTemporarilyStop = true) { blockedTemporarilyStop = _blockedTemporarilyStop; }

			int get_group() const { return groupId; }
			void auto_assign_group(REF_ int & _nextGroupId); // will assign group to all non assigned (and connected)

			bool is_connected_to(Node* _node) const;
			ConnectionSharedData* connect(Node* _node);
			void disconnect(Node* _node);
			Connection* access_connect_by_shared_data(ConnectionSharedData const* _csd);
			Connection const* get_connect_by_shared_data(ConnectionSharedData const* _csd) const;
			virtual void connect_as_open_node();
			virtual void unify_as_open_node();

			DoorInRoom* get_door_in_room() const { return doorInRoom; }
			void be_door_in_room(DoorInRoom* _doorInRoom);

			void be_important_path_node(bool _importantPathNode = true) { importantPathNode = _importantPathNode; }
			bool is_important_path_node() const { return importantPathNode; }

			bool is_open_node() const { return openNode; }
			bool is_open_accepting_node() const { return openNode && openMoveToFind.is_zero(); }
			void be_open_node(bool _openNode = true, Vector3 const & _moveInDirToFind = Vector3::zero, float _maxDistance = 1.0f) { openNode = _openNode; openMoveToFind = _moveInDirToFind * _maxDistance; }

			Transform get_current_placement() const;
			IModulesOwner* get_placement_owner() const { return placementOwner.get(); }
			bool get_on_moveable_owner() const { return onMoveableOwner; }

			Mesh* get_nav_mesh() const { return belongsTo; }
			Room* get_room() const;

#ifdef AN_DEVELOPMENT
			void set_name(Name const & _name) { name = _name; }
			Name const & get_name() const { return name; }
			int get_connections_count() const { return connections.get_size(); }
#else
			inline void set_name(Name const & _name) {}
#endif

			Flags & access_flags() { return flags; }
			Flags const & get_flags() const { return flags; }
			void set_flags_from_nav_mesh();

		public:
			virtual void reset_to_new();

		public:
			virtual void debug_draw();

		public:
			virtual bool find_location(Vector3 const & _at, OUT_ Vector3 & _found, REF_ float & _dist) const { return false; } // not further than dist, modifies _dist and _at

		public:
			void do_for_all_requestors(std::function<void(IModulesOwner* _owner)> _do) const;

		protected:
			bool are_connections_valid() const { return openNode || (calc_non_outdated_connections_count() <= 2); }
			int calc_non_outdated_connections_count() const;

		protected: friend class Mesh;
			void remove_outdated(); // remove stuff that is outdated
			virtual void outdate_if_irrelevant(); // turn into outdated if thinks, is irrelevant

		private: friend struct NodeRequestor;
			void remove_requestor(NodeRequestor* _requestor);
			void add_requestor(NodeRequestor* _requestor);

		protected: friend class Tasks::BuildNavMesh;
			void be_irrelevant_on_single_connection() { irrelevantOnSingleConnection = true; }

		protected:
#ifdef AN_DEVELOPMENT
			Name name;
#endif
			Flags flags;

			Mesh* belongsTo = nullptr; // managed by Mesh itself

			bool importantPathNode = false; // if it is important during path following

			bool blockedTemporarilyStop = false; // this is useful for paths that might be temporarily blocked but we don't want blocked connection to spread further. this is useful especially for nodes that are safe (example: inside cart/elevator, nodes on the other side of connection should propagate blocked temporarily)

			bool outdated = false; // removed from everywhere, not connected to anything
			Array<Connection> connections;
			bool irrelevantOnSingleConnection = false; // if has single connection, is thought to be irrelevant and is outdated then
#ifdef AN_DEVELOPMENT
			bool unified = false; // was unified with other node
#endif

			int groupId = NONE; // to have separate group of nodes within same navmesh (as single navmesh could be shared in one room)

			Transform placement = Transform::identity; // local if owner specified, otherwise world/room
			SafePtr<IModulesOwner> placementOwner; // this is owner only for placement!
			bool onMoveableOwner = false; // if moveable, path pulling should behave differently when switching between non moveable and moveable (or two different moveables) - it should not string pull then

			DoorInRoom* doorInRoom = nullptr; // this nav node represents door in room
			bool openNode = false; // is open node that may plug into other open nodes
			Vector3 openMoveToFind = Vector3::zero; // if open, may look for other points in specific dir to find a match.
													// this will limit all connections to 2 as it should be only used to connect to others
													// in placement's space (placement above)
													// convex polygons, if match edges, always connect to other convex polygons, doesn't matter if they're only accepting or not

			mutable Concurrency::MRSWLock requestorsLock;
			Array<NodeRequestor*> requestors;

			void auto_assign_group_internal(int _groupId);

			void get_unified_values_from(Node const * _other);

			friend class Tasks::FindPath;
			friend class Tasks::GetRandomLocation;
			friend class Tasks::GetRandomPath;
		};
	}
}