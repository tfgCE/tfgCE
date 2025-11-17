#pragma once

#include "navDeclarations.h"
#include "navNode.h"
#include "navPlacementAtNode.h"

#include "..\..\core\containers\array.h"
#include "..\..\core\math\math.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\memory\softPooledObject.h"

namespace Framework
{
	interface_class IModulesOwner;
	class DoorInRoom;

	namespace Nav
	{
		class Mesh;
		class Path;

		namespace Tasks
		{
			class FindPath;
			class GetRandomPath;
		};

		struct PathNode
		{
			PlacementAtNode placementAtNode; // for path this might be string pulled already
			Node::ConnectionSharedDataPtr connectionData; // for connection leading to this point
			
			bool stringPulled = false; // if this node is a result of string pulling
			bool stringPulledCorner = false; // so we know it is a corner we had to take
			Optional<Segment> stringPulledWindow;

			DoorInRoom* door = nullptr; // door to go through

			bool is_outdated() const;

			bool can_be_string_pulled() const;
		};

		typedef RefCountObjectPtr<Path> PathPtr;

		struct PathRequestInfo
		{
#ifdef AN_DEVELOPMENT_OR_PROFILER
			IModulesOwner const* devOwner = nullptr;
			tchar const* devInfo = TXT("");
#endif
			float collisionRadius = 0.25f;
			bool transportPath = false; // see comment in path
			bool throughClosedDoors = false;
			bool withinSameNavigationGroup = false; // will remain in the same group (that will switch when going room to room)
			bool withinSameRoomAndNavigationGroup = false; // will remain in room and in group
			Optional<Nav::Flags> useNavFlags; // if not set, then uses taken from mind

			explicit PathRequestInfo(IModulesOwner const* imo) {
#ifdef AN_DEVELOPMENT_OR_PROFILER
				devOwner = imo;
#endif
				collision_radius_for(imo); }
#ifdef AN_DEVELOPMENT_OR_PROFILER
			PathRequestInfo& with_dev_info(tchar const* _devInfo) { devInfo = _devInfo; return *this; }
#else
			PathRequestInfo& with_dev_info(tchar const* _devInfo) { return *this; }
#endif
			PathRequestInfo& collision_radius_for(IModulesOwner const* imo);
			PathRequestInfo& collision_radius(float _collisionRadius) { collisionRadius = _collisionRadius; return *this; }
			PathRequestInfo& transport_path(bool _transportPath = true) { transportPath = _transportPath; return *this; }
			PathRequestInfo& through_closed_doors(bool _throughClosedDoors = true) { throughClosedDoors = _throughClosedDoors; return *this; }
			PathRequestInfo& within_same_navigation_group(bool _withinSameNavigationGroup = true) { withinSameNavigationGroup = _withinSameNavigationGroup; return *this; }
			PathRequestInfo& within_same_room_and_navigation_group(bool _withinSameRoomAndNavigationGroup = true) { withinSameRoomAndNavigationGroup = _withinSameRoomAndNavigationGroup; return *this; }
		};

		class Path
		: public SoftPooledObject<Path>
		, public RefCountObject
		{
		public:
			Path();

			Array<PathNode> const & get_path_nodes() const { return pathNodes; }

			int find_next_node(Room* _room, Vector3 const & _loc, OUT_ OPTIONAL_ Vector3* _onPath = nullptr) const; // NONE if not found, returns next node on path
			bool find_forward_location(Room* _room, Vector3 const & _loc, float _distance, OUT_ Vector3 & _found, OUT_ OPTIONAL_ Vector3* _onPath = nullptr, OUT_ OPTIONAL_ bool* _hitTemporaryBlock = nullptr) const; // where do we want to be

			float calculate_length(bool _flat = false) const; // if just trying to check if we reached end, check is_close_to_the_end
			float calculate_distance(Room* _room, Vector3 const & _loc, bool _flat = false) const; // if just trying to check if we reached end, check is_close_to_the_end
			bool is_close_to_the_end(Room* _room, Vector3 const& _loc, float _distance, Optional<bool> const & _flat = NP, OUT_ float* _pathDistance = nullptr) const;

			bool is_path_blocked_temporarily(int _nextPathIdx, Vector3 const & imoLoc, float distToCover = 0.25f magic_number) const;

			void log_node(LogInfoContext & _log, int _idx, tchar const * _prefix = nullptr) const;

			Room* get_start_room() const;

			Room* get_final_room() const;
			Vector3 get_final_node_location() const;

			bool is_transport_path() const { return transportPath; }

			bool is_through_closed_doors() const { return throughClosedDoors; }

		protected:	// SoftPooledObject
					friend class SoftPooledObject<Path>;
			implement_ void on_get();
			implement_ void on_release();

		protected: // RefCountObject
			override_ void destroy_ref_count_object() { release(); }

		private:
			Array<PathNode> pathNodes;

			float collisionRadius = 0.25f;
			bool transportPath = false; // we want to be close to the final destination but we have to be in the same room!

			bool throughClosedDoors = false;

			void post_process(); // add nodes between polygons, remove polygons between points, string pull etc

			void setup_with(PathRequestInfo& _pathRequestInfo);

			friend class Tasks::FindPath;
			friend class Tasks::GetRandomPath;
		};
	}
}

DECLARE_REGISTERED_TYPE(RefCountObjectPtr<Framework::Nav::Path>);

TYPE_AS_CHAR(Framework::Nav::Path);
