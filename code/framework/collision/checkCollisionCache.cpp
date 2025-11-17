#include "checkCollisionCache.h"

#include "..\module\moduleCollision.h"
#include "..\module\modulePresence.h"

#include "..\object\object.h"
#include "..\world\presenceLink.h"
#include "..\world\room.h"

#include "..\..\core\performance\performanceUtils.h"

//

using namespace Framework;

//

CheckCollisionCache::CheckCollisionCache()
{
	SET_EXTRA_DEBUG_INFO(cachedPresenceLinks, TXT("CheckCollisionCache.cachedPresenceLinks"));
}

void CheckCollisionCache::build_from_presence_links(Room const * _room, Range3 const & _inRoomBoundingBox)
{
	MEASURE_PERFORMANCE(checkCollisionCache_build);
	if (forRoom != _room || !isValid)
	{
		forRoom = _room;
		cachedPresenceLinks.clear();
		isValid = true;
	}
	if (!_room)
	{
		return;
	}
	PresenceLink const * link = _room->get_presence_links();
	while (link)
	{
		an_assert(link->get_in_room() == _room);
		if (link->is_valid_for_collision() &&
			link->get_collision_bounding_box().overlaps(_inRoomBoundingBox))
		{
			if (auto const * imo = link->get_modules_owner())
			{
				if (auto const * cm = link->get_modules_owner()->get_collision())
				{
					if (cachedPresenceLinks.get_size() == MAX_PRESENCE_LINKS)
					{
						isValid = false;
						return;
					}
					cachedPresenceLinks.push_back(link);
				}
			}
		}

		link = link->get_next_in_room();
	}
}

