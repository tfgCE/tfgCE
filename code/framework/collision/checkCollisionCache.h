#pragma once

#include "againstCollision.h"
#include "..\..\core\math\math.h"

namespace Collision
{
	interface_class ICollidableObject;
	class ModelInstanceSet;
};

namespace Framework
{
	class PresenceLink;
	class Room;
	struct CollisionTrace;

	struct CheckCollisionCache
	{
	public:
		static const int MAX_PRESENCE_LINKS = 64;

		CheckCollisionCache();

		void build_from_presence_links(Room const * _room, Range3 const & _inRoomBoundingBox);
		template <typename CollisionTraceClass>
		inline void build_from_presence_links(Room const * _room, Transform const & _placement, Array<CollisionTraceClass> const & _traces);
		template <typename CollisionTraceClass>
		inline void build_from_presence_links(Room const * _room, Transform const & _placement, CollisionTraceClass const & _trace);

		bool is_valid() const { return isValid; }

		Room const * get_for_room() const { return forRoom; }

		ArrayStatic<PresenceLink const*, MAX_PRESENCE_LINKS> const & get_cached_presence_links() const { return cachedPresenceLinks; }

	private:
		bool isValid = true;

		Room const * forRoom = nullptr;
		ArrayStatic<PresenceLink const*, MAX_PRESENCE_LINKS> cachedPresenceLinks;
	};

};

#include "checkCollisionCache.inl"
