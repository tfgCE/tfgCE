#include "spaceBlockingObject.h"

#include "..\module\moduleAppearance.h"

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(SpaceBlockingObject);

SpaceBlockingObject::SpaceBlockingObject(SpaceBlockingObjectType const * _spaceBlockingObjectType, String const & _name)
: base(_spaceBlockingObjectType, _name)
, spaceBlockingObjectType(_spaceBlockingObjectType)
{
}

SpaceBlockers const& SpaceBlockingObject::get_space_blocker_local() const
{
	if (auto* a = get_appearance())
	{
		if (auto* m = a->get_mesh())
		{
			if (!m->get_space_blockers().is_empty())
			{
				return m->get_space_blockers();
			}
		}
	}
	return spaceBlockingObjectType->get_space_blocker_local();
}
