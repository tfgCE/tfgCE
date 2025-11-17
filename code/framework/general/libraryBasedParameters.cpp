#include "libraryBasedParameters.h"

#include "..\library\library.h"
#include "..\library\libraryLoadingContext.h"
#include "..\library\usedLibraryStored.inl"

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(LibraryBasedParameters);
LIBRARY_STORED_DEFINE_TYPE(LibraryBasedParameters, libraryBasedParameters);

LibraryBasedParameters::LibraryBasedParameters(Library * _library, LibraryName const & _name)
: base(_library, _name)
{
}

bool LibraryBasedParameters::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);
 
	// load directly from the node
	access_custom_parameters().load_from_xml(_node);
	_lc.load_group_into(access_custom_parameters());

	return result;
}

bool LibraryBasedParameters::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

void LibraryBasedParameters::prepare_to_unload()
{
	base::prepare_to_unload();
}

