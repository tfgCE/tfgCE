template <typename StoredClass>
CombinedLibraryStore<StoredClass>::CombinedLibraryStore(Name const &_typeName)
:	CombinedLibraryStoreBase( _typeName )
{
}

template <typename StoredClass>
StoredClass * CombinedLibraryStore<StoredClass>::find(LibraryName const & _name) const
{
	return fast_cast<StoredClass>(find_stored(_name));
}
