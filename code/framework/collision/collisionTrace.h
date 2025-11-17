#pragma once

#include "..\..\core\containers\array.h"
#include "..\..\core\math\math.h"

namespace Framework
{
	class Room;

	namespace CollisionTraceInSpace
	{
		enum Type
		{
			OS,
			WS,
		};
	};

	struct CollisionTrace
	{
		static const int MAX_LOCATIONS = 32;

		CollisionTrace() { SET_EXTRA_DEBUG_INFO(locations, TXT("CollisionTrace.locations")); }
		CollisionTrace(CollisionTraceInSpace::Type _locationsIn) : locationsIn(_locationsIn) { SET_EXTRA_DEBUG_INFO(locations, TXT("CollisionTrace.locations")); }
		CollisionTrace(Room const * _startInRoom, Transform const & _intoStartRoomTransform = Transform::identity) : locationsIn(CollisionTraceInSpace::WS), startInRoom(_startInRoom), intoStartRoomTransform(_intoStartRoomTransform) { SET_EXTRA_DEBUG_INFO(locations, TXT("CollisionTrace.locations")); }

		bool load_from_xml(IO::XML::Node const * _node);

		CollisionTraceInSpace::Type get_locations_in() const { return locationsIn; }
		ArrayStatic<Vector3, MAX_LOCATIONS> const & get_locations() const { return locations; }
		Room const * get_start_in_room() const { return startInRoom; }
		Transform const & get_into_start_room_transform() const { return intoStartRoomTransform; }

		void add_location(Vector3 const & _location) { locations.push_back(_location); }

		void debug_draw(Transform const & _usePlacement = Transform::identity) const;

	private:
		CollisionTraceInSpace::Type locationsIn = CollisionTraceInSpace::WS;
		ArrayStatic<Vector3, MAX_LOCATIONS> locations; // local location - in actor space

		Room const * startInRoom = nullptr;
		Transform intoStartRoomTransform = Transform::identity;
	};
};


