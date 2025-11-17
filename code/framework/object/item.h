#pragma once

#include "object.h"

#include "itemType.h"

namespace Framework
{
	class Item
	: public Object
	{
		FAST_CAST_DECLARE(Item);
		FAST_CAST_BASE(Object);
		FAST_CAST_END();

		typedef Object base;
	public:
		Item(ItemType const * _itemType, String const & _name); // requires initialisation

		ItemType const* get_item_type() const { return type; }

	public: // IModulesOwner
		override_ Name const & get_object_type_name() const { return Names::item; }

	private:
		ItemType const* type = nullptr;
	};
};

DECLARE_REGISTERED_TYPE(Framework::Item*);
