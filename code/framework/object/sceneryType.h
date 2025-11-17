#pragma once

#include "spaceBlockingObjectType.h"

namespace Framework
{
	class SceneryType
	: public SpaceBlockingObjectType
	{
		FAST_CAST_DECLARE(SceneryType);
		FAST_CAST_BASE(SpaceBlockingObjectType);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef SpaceBlockingObjectType base;
	public:
		SceneryType(Library * _library, LibraryName const & _name);
		virtual ~SceneryType() {}

		String create_instance_name() const;

		bool is_room_scenery() const { return roomScenery; }

	public:	// ObjectType
		override_ void set_defaults();
		override_ Object* create(String const & _name) const;
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

	protected:
		bool roomScenery = false; // integral part of the room, wall, platform etc
	};
};
