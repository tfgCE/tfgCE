#ifdef AN_CLANG
#include "library.h"
#endif
#include "libraryCombinedStore.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(CombinedLibraryStoreBase);

CombinedLibraryStoreBase::CombinedLibraryStoreBase(Name const & _typeName)
: LibraryStoreBase(_typeName)
{
}

void CombinedLibraryStoreBase::add(LibraryStoreBase* _store)
{
	stores.push_back(_store);
}

LibraryStored * CombinedLibraryStoreBase::find_stored(LibraryName const & _name) const
{
	for_every_ptr(store, stores)
	{
		if (LibraryStored * found = store->find_stored(_name))
		{
			return found;
		}
	}
	return nullptr;
}

bool CombinedLibraryStoreBase::does_type_name_match(Name const & _typeName) const
{
	if (LibraryStoreBase::does_type_name_match(_typeName))
	{
		return true;
	}
	for_every_ptr(store, stores)
	{
		if (store->does_type_name_match(_typeName))
		{
			return true;
		}
	}
	return false;
}

void CombinedLibraryStoreBase::do_for_every(DoForLibraryStored _do_for_library_stored)
{
	for_every_ptr(store, stores)
	{
		store->do_for_every(_do_for_library_stored);
	}
}

void CombinedLibraryStoreBase::remove_stored(LibraryStored * _object)
{
	for_every_ptr(store, stores)
	{
		store->remove_stored(_object);
	}
}

void CombinedLibraryStoreBase::do_for_type(DoForLibraryType _do_for_library_type)
{
	for_every_ptr(store, stores)
	{
		if (_do_for_library_type(store->get_type_name()))
		{
			return;
		}
	}
}

