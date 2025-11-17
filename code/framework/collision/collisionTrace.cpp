#include "collisionTrace.h"

#include "..\..\core\debug\debugRenderer.h"

using namespace Framework;

bool CollisionTrace::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;
	for_every(locationNode, _node->children_named(TXT("location")))
	{
		Vector3 location = Vector3::zero;
		if (location.load_from_xml(locationNode))
		{
			locations.push_back(location);
		}
		else
		{
			result = false;
		}
	}
	return result;
}

void CollisionTrace::debug_draw(Transform const & _usePlacement) const
{
	if (startInRoom)
	{
		debug_context(startInRoom);
	}
	for (int i = 1; i < locations.get_size(); ++i)
	{
		debug_draw_line(true, Colour::grey, _usePlacement.location_to_world(locations[i - 1]), _usePlacement.location_to_world(locations[i]));
	}
	if (startInRoom)
	{
		debug_no_context();
	}
}
