#include "reverb.h"

#include "..\library\library.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(Framework::Reverb);
LIBRARY_STORED_DEFINE_TYPE(Framework::Reverb, reverb);

Reverb::Reverb(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
{
}

bool Framework::Reverb::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = LibraryStored::load_from_xml(_node, _lc);

	result &= reverb.load_from_xml(_node);

	return result;
}
