#include "library.h"

#include "libraryStore.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(LibraryStoreBase);

LibraryStoreBase::LibraryStoreBase(Name const & _typeName)
:	typeName( _typeName )
{
}

LibraryStoreBase::~LibraryStoreBase()
{
}

bool LibraryStoreBase::call_prepare_for_game(Library* _library, LibraryStored* _storedObject, LibraryPrepareContext& _pfgContext)
{
	return _library->prepare_for_game(_storedObject, _pfgContext);
}

bool LibraryStoreBase::check_for_preparing_problems(Library* _library, bool _objectPreparedProperly) const
{
#ifdef AN_DEVELOPMENT
	return (_library && _library->should_ignore_preparing_problems()) || _objectPreparedProperly;
#else
	return _objectPreparedProperly;
#endif
}
