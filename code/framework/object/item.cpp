#include "item.h"

#include "..\game\game.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(Item);

Item::Item(ItemType const * _itemType, String const & _name)
: Object(_itemType, !_name.is_empty() ? _name : _itemType->create_instance_name())
, type(_itemType)
{
}
