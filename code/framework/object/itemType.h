#pragma once

#include "objectType.h"

namespace Framework
{
	class ItemType
	: public ObjectType
	{
		FAST_CAST_DECLARE(ItemType);
		FAST_CAST_BASE(ObjectType);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef ObjectType base;
	public:
		ItemType(Library * _library, LibraryName const & _name);
		virtual ~ItemType() {}

		String create_instance_name() const;

	public:	// ObjectType
		override_ void set_defaults();
		override_ Object* create(String const & _name) const;
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
	};
};
