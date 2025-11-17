#pragma once

#include "objectType.h"

namespace Framework
{
	class SpaceBlockingObjectType
	: public ObjectType
	{
		FAST_CAST_DECLARE(SpaceBlockingObjectType);
		FAST_CAST_BASE(ObjectType);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef ObjectType base;
	public:
		SpaceBlockingObjectType(Library * _library, LibraryName const & _name);
		virtual ~SpaceBlockingObjectType() {}

	public:
		SpaceBlockers const& get_space_blocker_local() const { return spaceBlockers; }

	public:	// ObjectType
		override_ void prepare_to_unload();
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

	private:
		SpaceBlockers spaceBlockers;
	};
};
