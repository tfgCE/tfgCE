#pragma once

#include "object.h"

#include "spaceBlockingObjectType.h"

namespace Framework
{
	class SpaceBlockingObject
	: public Object
	{
		FAST_CAST_DECLARE(SpaceBlockingObject);
		FAST_CAST_BASE(Object);
		FAST_CAST_END();

		typedef Object base;
	public:
		SpaceBlockingObject(SpaceBlockingObjectType const * _spaceBlockingObjectType, String const & _name); // requires initialisation

	public:
		SpaceBlockers const& get_space_blocker_local() const;

	private:
		SpaceBlockingObjectType const* spaceBlockingObjectType;
	};
};

DECLARE_REGISTERED_TYPE(Framework::SpaceBlockingObject*);
