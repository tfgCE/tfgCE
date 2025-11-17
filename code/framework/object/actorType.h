#pragma once

#include "spaceBlockingObjectType.h"

namespace Framework
{
	class ActorType
	: public SpaceBlockingObjectType
	{
		FAST_CAST_DECLARE(ActorType);
		FAST_CAST_BASE(SpaceBlockingObjectType);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef SpaceBlockingObjectType base;
	public:
		ActorType(Library * _library, LibraryName const & _name);
		virtual ~ActorType() {}

		String create_instance_name() const;

	public:	// ObjectType
		override_ void set_defaults();
		override_ Object* create(String const & _name) const;
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);

	private:
	};
};
