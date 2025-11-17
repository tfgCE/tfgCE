#include "spaceBlockingObjectType.h"

#include "..\library\usedLibraryStored.inl"

#include "..\library\library.h"

#include "..\module\modules.h"

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(SpaceBlockingObjectType);
LIBRARY_STORED_DEFINE_TYPE(SpaceBlockingObjectType, spaceBlockingObjectType);

SpaceBlockingObjectType::SpaceBlockingObjectType(Library * _library, LibraryName const & _name)
: base(_library, _name)
{
}

bool SpaceBlockingObjectType::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (! _node)
	{
		return false;
	}

	bool result = base::load_from_xml(_node, _lc);
	
	result &= spaceBlockers.load_from_xml_child_node(_node);
	
	return result;
}

void SpaceBlockingObjectType::prepare_to_unload()
{
	base::prepare_to_unload();

	spaceBlockers.clear();
}

