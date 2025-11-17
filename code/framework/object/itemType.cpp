#include "itemType.h"

#include "item.h"

#include "..\library\usedLibraryStored.inl"

#include "..\library\library.h"

#include "..\module\modules.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(ItemType);
LIBRARY_STORED_DEFINE_TYPE(ItemType, itemType);

void ItemType::set_defaults()
{
	base::set_defaults();
	individualityFactor = 0.0f;
}

ItemType::ItemType(Library * _library, LibraryName const & _name)
: base(_library, _name)
{
}

Object* ItemType::create(String const & _name) const
{
	return new Item(this, _name);
}

String ItemType::create_instance_name() const
{
	// TODO some rules needed
	return get_name().to_string();
}

bool ItemType::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (! _node ||
		_node->get_name() != TXT("itemType"))
	{
		//TODO Utils.Log.error("invalid xml node when loading ItemType");
		return false;
	}
	gameplay.provide_default_type(_lc.get_library()->get_game()->get_customisation().library.defaultGameplayModuleTypeForItemType);
	return base::load_from_xml(_node, _lc);
}

