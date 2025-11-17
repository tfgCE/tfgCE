#include "lhd_objectItem.h"

#include "..\..\..\library\library.h"
#include "..\..\..\tutorials\tutorialSystem.h"

#include "..\..\..\..\framework\library\usedLibraryStored.inl"

//

using namespace Loader;
using namespace HubDraggables;

//

REGISTER_FOR_FAST_CAST(ObjectItem);

ObjectItem::ObjectItem(Framework::IModulesOwner* _object)
: object(_object)
{
}

void ObjectItem::add_icon(Framework::TexturePart* _icon)
{
	if (_icon)
	{
		icons.push_back(Framework::UsedLibraryStored<Framework::TexturePart>(_icon));
	}
}
