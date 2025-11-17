#pragma once

#include "navDeclarations.h"

#include "..\..\core\math\math.h"

namespace Framework
{
	interface_class IModulesOwner;
	class DoorInRoom;
	class Room;

	namespace Nav
	{
		class Node;

		struct PlacementAtNode
		{
			// local space to node
			//	(if not used, identity transform assumed, important for polygons as this it means particular point in polygon or any point)
			//	(if not used, for polygons it is the centre of the polygon)
			Optional<Transform> placementLS;
			NodePtr node;

			static PlacementAtNode const & invalid() { return s_invalid; }

			PlacementAtNode();
			explicit PlacementAtNode(Node* _node);

			bool is_valid() const;

			void clear();
			Transform get_current_placement() const;
			Transform get_current_node_placement() const;
			Room* get_room() const;

			void set(Node* _node);
			void set_placement(Optional<Transform> const & _placement = NP);
			void set_location(Vector3 const & _locWS);

#ifdef AN_LOG_NAV_TASKS
			void log(LogInfoContext& _log) const;
#endif
		private:
			static PlacementAtNode s_invalid;
		};

	};
};