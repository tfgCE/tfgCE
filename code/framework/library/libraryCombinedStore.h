#pragma once

#include "libraryName.h"
#include "libraryStored.h"
#include "libraryStore.h"

namespace Framework
{
	class CombinedLibraryStoreBase
	:	public LibraryStoreBase
	{
		FAST_CAST_DECLARE(CombinedLibraryStoreBase);
		FAST_CAST_BASE(LibraryStoreBase);
		FAST_CAST_END();
	public:
		CombinedLibraryStoreBase(Name const & _typeName);

		void add(LibraryStoreBase* _store);
		void do_for_type(DoForLibraryType _do_for_library_type);

	public: // LibraryStoreBase
		override_ LibraryStored * find_stored(LibraryName const & _name) const;
		override_ bool does_type_name_match(Name const & _typeName) const;
		override_ bool should_load_from_xml(IO::XML::Node const * _node) const { return false; } // combined never load from xml
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, LibraryName const & _fallbackName = LibraryName::invalid(), OPTIONAL_ LibraryLoadSummary* _summary = nullptr) { return false; }
		override_ void do_for_every(DoForLibraryStored _do_for_library_stored);
		override_ void remove_stored(LibraryStored * _object);

	private:
		List<LibraryStoreBase*> stores; // order is important, especially when lookin for something that shares name

	};

	/**
	 *	Combined library store
	 */
	template <typename StoredClass>
	class CombinedLibraryStore
	:	public CombinedLibraryStoreBase
	{
		FAST_CAST_DECLARE(CombinedLibraryStore);
		FAST_CAST_BASE(CombinedLibraryStoreBase);
		FAST_CAST_END();
	public:
		CombinedLibraryStore(Name const & _typeName);

		StoredClass * find(LibraryName const & _name) const;
	};

	template <typename StoredClass>
	REGISTER_FOR_FAST_CAST(CombinedLibraryStore<StoredClass>);

	#include "libraryCombinedStore.inl"
};
