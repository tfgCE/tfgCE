#pragma once

#include "..\framework.h"

#include "..\debugSettings.h"

#include "libraryName.h"
#include "libraryStored.h"
#include "libraryStoreTaggedObjects.h"

#include "..\..\core\concurrency\mrswLock.h"
#include "..\..\core\memory\refCountObjectPtr.h"

// .inl
#include "..\..\core\concurrency\scopedMRSWLock.h"
#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\containers\arrayStack.h"

#ifdef AN_DEVELOPMENT
#define OUTPUT_UNLOAD
#endif

namespace Framework
{
	class Library;
	struct LibraryLoadSummary;

	void store_library_loading_time(Library* _library, Name const& _name, float _time);

	class LibraryStoreBase
	{
		FAST_CAST_DECLARE(LibraryStoreBase);
		FAST_CAST_END();
	public:
		LibraryStoreBase(Name const & _typeName);
		virtual ~LibraryStoreBase();

		void use_construct_object_with_name(LibraryStored::ConstructStoredClassObjectWithNameFunc _construct_object_with_name) { construct_object_with_name = _construct_object_with_name; }

		virtual bool does_type_name_match(Name const & _typeName) const { return typeName == _typeName; }
		virtual bool should_load_from_xml(IO::XML::Node const * _node) const { return _node && _node->get_name() == typeName.to_string(); }
		virtual bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, LibraryName const & _fallbackName = LibraryName::invalid(), OPTIONAL_ LibraryLoadSummary* _summary = nullptr) { return false; }
		virtual bool create_standalone_object_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, OUT_ RefCountObjectPtr<LibraryStored> & _object) { return false; } // not added into the store
		virtual LibraryStored * find_stored(LibraryName const & _name) const { return nullptr; }
		virtual LibraryStored * find_stored_or_create(LibraryName const & _name) { return nullptr; }
		virtual void do_for_every(DoForLibraryStored _do_for_library_stored) { an_assert(false); }
		virtual void remove_stored(LibraryStored * _object) {}
		virtual bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext) { return true; }
		virtual bool unload(LibraryLoadLevel::Type _libraryLoadLevel) { return true; }
		virtual void mark_prepared_for_game() {}
		virtual void list_all_objects(LogInfoContext & _log) const {}
		virtual int count_all_objects() const { return 0; }

		Name const & get_type_name() const { return typeName; }

	public:
		virtual LibraryStored* create_temporary_object(LibraryName const& _temporaryName = LibraryName::invalid()) { return nullptr; }

	protected:
		Name typeName;
		// bunch of delegates, you could call this an embassy
		LibraryStored::ConstructStoredClassObjectWithNameFunc construct_object_with_name = nullptr;

	protected:
		bool call_prepare_for_game(Library* _library, LibraryStored* _storedObject, LibraryPrepareContext& _pfgContext);
		bool check_for_preparing_problems(Library* _library, bool _objectPreparedProperly) const;

	private:
		LibraryStoreBase(LibraryStoreBase const & _source) {} // prohibited
	};

	/**
	 *	Store for library data. All StoredClass objects need to be derived from LibraryStored.
	 */
	template <typename StoredClass>
	class LibraryStore
	: public LibraryStoreBase
	{
		FAST_CAST_DECLARE(LibraryStore);
		FAST_CAST_BASE(LibraryStoreBase);
		FAST_CAST_END();
	public:
		LibraryStore(Library * _library, Name const & _typeName, LibraryStored::ConstructStoredClassObjectWithNameFunc _construct_object_with_name);
		virtual ~LibraryStore();

		void clear();
		StoredClass * find(LibraryName const & _name) const;
		StoredClass * find_may_fail(LibraryName const & _name) const;
		StoredClass * find_or_create(LibraryName const & _name);
		StoredClass * create_temporary(LibraryName const & _temporaryName = LibraryName::invalid()); // with invalid name

		void copy_from(LibraryStore<StoredClass> const & other);
		
		bool is_empty() const { return storedObjects.is_empty(); }
		int get_size() const { return storedObjects.get_size(); }
		List<RefCountObjectPtr<StoredClass>> const & get_all_stored() { return storedObjects; }
		// use with for_every_ptr!
		LibraryStoreTaggedObjects<StoredClass> const get_tagged(TagCondition const & _tagCondition) { return LibraryStoreTaggedObjects<StoredClass>(this, _tagCondition); }
		LibraryStoreTaggedObjects<StoredClass> const get_using(WorldSettingCondition const & _worldSettingCondition, WorldSettingConditionContext const & _worldSettingConditionContext) { return LibraryStoreTaggedObjects<StoredClass>(this, _worldSettingCondition, _worldSettingConditionContext); }

	public: // LibraryStoreBase
		override_ LibraryStored * find_stored(LibraryName const & _name) const;
		override_ LibraryStored * find_stored_or_create(LibraryName const & _name);
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, LibraryName const & _fallbackName = LibraryName::invalid(), OPTIONAL_ LibraryLoadSummary* _summary = nullptr);
		override_ bool create_standalone_object_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, OUT_ RefCountObjectPtr<LibraryStored> & _object); // not added into the store
		override_ void do_for_every(DoForLibraryStored _do_for_library_stored);
		override_ void remove_stored(LibraryStored * _object);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		override_ bool unload(LibraryLoadLevel::Type _libraryLoadLevel);
		override_ void mark_prepared_for_game();
		override_ void list_all_objects(LogInfoContext & _log) const;
		override_ int count_all_objects() const { return get_size(); }

		override_ LibraryStored* create_temporary_object(LibraryName const& _temporaryName = LibraryName::invalid()) { return create_temporary(_temporaryName); }

	private:
		Library* library;
		mutable Concurrency::MRSWLock storedObjectsLock; // read - we do not modify the list, write - we modify the list
		List<RefCountObjectPtr<StoredClass>> storedObjects; // TODO map and use LibraryName as index?

		LibraryStored* find_internal(LibraryName const & _name, bool _constructIfDoesntExist = false);
		LibraryStored* find_internal(LibraryName const & _name) const;
		LibraryStored* add_internal(LibraryName const & _name, bool _doNotCheckForCopies = false);

	private:
		LibraryStore(LibraryStore const & _source) {} // prohibited
	};

	template <typename StoredClass>
	REGISTER_FOR_FAST_CAST(LibraryStore<StoredClass>);

	#include "libraryStore.inl"
};
