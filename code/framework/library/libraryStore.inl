#ifndef AN_SKIP_LIBRARY_STORE_INL
template <typename StoredClass>
LibraryStore<StoredClass>::LibraryStore(Library * _library, Name const & _typeName, LibraryStored::ConstructStoredClassObjectWithNameFunc _construct_object_with_name)
: LibraryStoreBase(_typeName)
, library(_library)
{
	use_construct_object_with_name(_construct_object_with_name);
	an_assert(_typeName.to_string().get_length() > 0);
}

template <typename StoredClass>
LibraryStore<StoredClass>::~LibraryStore()
{
	clear();
}

template <typename StoredClass>
void LibraryStore<StoredClass>::clear()
{
	Concurrency::ScopedMRSWLockRead scopedLock(storedObjectsLock);
	for_every_ref(storedObject, storedObjects)
	{
		storedObject->set_library(nullptr); // no longer in a library!
	}
	storedObjects.clear();
}

template <typename StoredClass>
LibraryStored * LibraryStore<StoredClass>::find_stored(LibraryName const & _name) const
{
	if (_name.is_valid())
	{
		return find_internal(_name);
	}
	else
	{
		return nullptr;
	}
}

template <typename StoredClass>
LibraryStored * LibraryStore<StoredClass>::find_stored_or_create(LibraryName const & _name)
{
	return find_or_create(_name);
}

template <typename StoredClass>
StoredClass * LibraryStore<StoredClass>::find(LibraryName const & _name) const
{
	StoredClass * found = fast_cast<StoredClass>(find_stored(_name));
	if (!found && _name.is_valid())
	{
#ifdef AN_DEVELOPMENT
		if (is_preview_game())
		{
			if (does_preview_game_require_info_about_missing_library_stored())
			{
				output(TXT("couldn't find \"%S\" of type \"%S\""), _name.to_string().to_char(), get_type_name().to_char());
			}
		}
		else
#endif
		{
#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (!Framework::Library::may_ignore_missing_references())
#endif
			{
				error(TXT("couldn't find \"%S\" of type \"%S\""), _name.to_string().to_char(), get_type_name().to_char());
			}
		}
	}
	return found;
}

template <typename StoredClass>
StoredClass * LibraryStore<StoredClass>::find_may_fail(LibraryName const & _name) const
{
	return fast_cast<StoredClass>(find_stored(_name));
}

template <typename StoredClass>
StoredClass * LibraryStore<StoredClass>::create_temporary(LibraryName const & _temporaryName)
{
	StoredClass* temp = fast_cast<StoredClass>(add_internal(_temporaryName, true));
	an_assert(temp);
	temp->be_temporary();
	return temp;
}

template <typename StoredClass>
StoredClass * LibraryStore<StoredClass>::find_or_create(LibraryName const & _name)
{
	if (_name.is_valid())
	{
		return fast_cast<StoredClass>(find_internal(_name, true));
	}
	else
	{
		return nullptr;
	}
}

template <typename StoredClass>
LibraryStored * LibraryStore<StoredClass>::find_internal(LibraryName const & _name) const
{
	Concurrency::ScopedMRSWLockRead scopedLock(storedObjectsLock);
	for_every(storedObject, storedObjects)
	{
		if (storedObject->get()->get_name() == _name)
		{
			return storedObject->get();
		}
	}
	return nullptr;
}

template <typename StoredClass>
LibraryStored * LibraryStore<StoredClass>::find_internal(LibraryName const & _name, bool _constructIfDoesntExist)
{
	{
		Concurrency::ScopedMRSWLockRead scopedLock(storedObjectsLock);
		for_every(storedObject, storedObjects)
		{
			if (storedObject->get()->get_name() == _name)
			{
				return storedObject->get();
			}
		}
	}
	if (_constructIfDoesntExist)
	{
		return add_internal(_name, true);
	}
	return nullptr;
}

template <typename StoredClass>
LibraryStored * LibraryStore<StoredClass>::add_internal(LibraryName const & _name, bool _doNotCheckForCopies)
{
	if (! _doNotCheckForCopies)
	{
		LibraryStored* exists = find_internal(_name, false);
		if (exists)
		{
			return exists;
		}
	}
	if (construct_object_with_name != nullptr)
	{
		Concurrency::ScopedMRSWLockWrite scopedLock(storedObjectsLock);
		LibraryStored* newObject = construct_object_with_name(library, _name);
		an_assert(fast_cast<StoredClass>(newObject), TXT("for some odd reason it has different class than expected"));
		storedObjects.push_back(RefCountObjectPtr<StoredClass>(fast_cast<StoredClass>(newObject)));
		return newObject;
	}
	else
	{
		// TODO Utils.Log.error(TXT("Can't add object of type %S (most likely it is name for combined type)."), typeName.asString);
		return nullptr;
	}
}

template <typename StoredClass>
bool LibraryStore<StoredClass>::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, LibraryName const & _fallbackName, OPTIONAL_ LibraryLoadSummary* _summary)
{
	if (! should_load_from_xml(_node))
	{
		return false;
	}
	LibraryName name = LibraryName::from_xml(_node, String(TXT("name")), _lc);
	if (!name.is_valid())
	{
		name = _fallbackName;
	}
	if (! name.is_valid())
	{
		error_loading_xml(_node, TXT("name of object is invalid"));
		return false;
	}
	if (_lc.is_reloading_only())
	{
#ifdef AN_DEVELOPMENT
		LibraryStored* existingObject = find_internal(name, false);
		if (existingObject)
		{
			if (existingObject == _lc.get_reload_only() ||
				_lc.should_be_reloaded(existingObject))
			{
				existingObject->ready_for_reload();
				ForLibraryLoadingContext flc(_lc);
				flc.push_id(name.to_string());
				flc.push_owner(existingObject);
				return existingObject->load_from_xml(_node, _lc);
			}
			else
			{
				return true;
			}
		}
#else
		return true;
#endif
	}
	LibraryStored* newObject = find_internal(name, true);
	if (newObject)
	{
		bool result;
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
		::System::TimeStamp loadTimeStamp;
#endif
		ForLibraryLoadingContext flc(_lc);
		flc.push_id(name.to_string());
		flc.push_owner(newObject);
		result = newObject->load_from_xml_for_store(_node, _lc);
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES
		store_library_loading_time(_lc.get_library(), get_type_name(), loadTimeStamp.get_time_since());
#endif
		if (_summary)
		{
			_summary->loaded(newObject);
		}
		return result;
	}
	else
	{
		return false;
	}
}

template <typename StoredClass>
bool LibraryStore<StoredClass>::create_standalone_object_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, OUT_ RefCountObjectPtr<LibraryStored> & _object)
{
	_object = nullptr;
	if (!should_load_from_xml(_node))
	{
		return false;
	}
	LibraryName name = LibraryName::from_xml(_node, String(TXT("name")), _lc);
	if (!name.is_valid())
	{
		error_loading_xml(_node, TXT("name of object is invalid"));
		return false;
	}
	if (_lc.is_reloading_only())
	{
#ifdef AN_DEVELOPMENT
		todo_important(TXT("allow reloading? for now there is no need for that but it might be required in the future"));
#else
		return true;
#endif
	}

	// always creates new
	if (construct_object_with_name != nullptr)
	{
		bool result = true;
		_object = construct_object_with_name(_lc.get_library(), name);
		an_assert(fast_cast<StoredClass>(_object.get()), TXT("for some odd reason it has different class than expected"));
		ForLibraryLoadingContext flc(_lc);
		flc.push_id(name.to_string());
		flc.push_owner(_object.get());
		result = _object->load_from_xml(_node, _lc);
		return result;
	}
	else
	{
		// TODO Utils.Log.error(TXT("Can't add object of type %S (most likely it is name for combined type)."), typeName.asString);
		return false;
	}
}

template <typename StoredClass>
void LibraryStore<StoredClass>::copy_from(LibraryStore<StoredClass> const & other)
{
	// clear everything
	storedObjects.clear();
	// add everything from other
	storedObjects.copy_from(other.storedObjects);
}

template <typename StoredClass>
bool LibraryStore<StoredClass>::unload(LibraryLoadLevel::Type _libraryLoadLevel)
{
	// gather all objects in an array, remove them from storedObjects array (refcounting helps)
	// actual removal of objects will happen when toRemove is cleared
	ARRAY_STACK(RefCountObjectPtr<StoredClass>, toRemove, storedObjects.get_size());
	{
		Concurrency::ScopedMRSWLockRead scopedLock(storedObjectsLock);
		for_every(storedObject, storedObjects)
		{
			if (storedObject->get()->get_library_load_level() >= _libraryLoadLevel)
			{
#ifdef OUTPUT_UNLOAD
				output(TXT("unloading %S..."), storedObject->get()->get_name().to_string().to_char());
#endif
				storedObject->get()->prepare_to_unload();
				toRemove.push_back(*storedObject);
			}
		}
	}
	{
		Concurrency::ScopedMRSWLockWrite scopedLock(storedObjectsLock);
		for_every_reverse(remove, toRemove)
		{
			storedObjects.remove(*remove);
		}
	}
	toRemove.clear();
	return true;
}

template <typename StoredClass>
bool LibraryStore<StoredClass>::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;
	// do it with indices as we may create new objects when preparing for game
	// as we're using library's prepare for game, we may end up with new objects created in this level that may be not prepared
	// we won't skip anything, we just need to go further
	int idx = 0;
	while (true)
	{
		StoredClass* storedObject;
		{
			Concurrency::ScopedMRSWLockRead lock(storedObjectsLock);
			if (idx >= storedObjects.get_size())
			{
				break;
			}
			storedObject = storedObjects[idx].get();
		}
		if (!storedObject->is_prepared_for_game())
		{
			//MEASURE_AND_OUTPUT_PERFORMANCE_MIN(0.005f, TXT("preparing [%S] %S : %03i"), get_type_name().to_string().to_char(), storedObject->get_name().to_string().to_char(), _pfgContext.get_current_level());
			bool objectPreparedProperly = call_prepare_for_game(_library, storedObject, _pfgContext);
			if (!objectPreparedProperly)
			{
				error(TXT("error preparing %S %S"), get_type_name().to_char(), storedObject->get_name().to_string().to_char());
			}
			an_assert(check_for_preparing_problems(_library, objectPreparedProperly));

			result = objectPreparedProperly && result;
		}
		++idx;
	}
	return result;
}

template <typename StoredClass>
void LibraryStore<StoredClass>::remove_stored(LibraryStored * _object)
{
	Concurrency::ScopedMRSWLockWrite scopedLock(storedObjectsLock);
	for_every(storedObject, storedObjects)
	{
		if (storedObject->get() == _object)
		{
			storedObjects.remove(*storedObject);
		}
	}
}

template <typename StoredClass>
void LibraryStore<StoredClass>::do_for_every(DoForLibraryStored _do_for_library_stored)
{
	Concurrency::ScopedMRSWLockRead scopedLock(storedObjectsLock);
	for_every(storedObject, storedObjects)
	{
		_do_for_library_stored(storedObject->get());
	}
}

template <typename StoredClass>
void LibraryStore<StoredClass>::mark_prepared_for_game()
{
	Concurrency::ScopedMRSWLockRead scopedLock(storedObjectsLock);
	for_every(storedObject, storedObjects)
	{
		if (!storedObject->get()->does_require_further_loading())
		{
			storedObject->get()->mark_prepared_for_game();
		}
	}
}

template <typename StoredClass>
void LibraryStore<StoredClass>::list_all_objects(LogInfoContext & _log) const
{
	Concurrency::ScopedMRSWLockRead scopedLock(storedObjectsLock);
	_log.set_colour(Colour::grey);
	_log.log(TXT(" store \"%S\""), get_type_name().to_char());
	int idx = 0;
	for_every(storedObject, storedObjects)
	{
		_log.log(TXT("  %3i. %S"), idx, storedObject->get()->get_name().to_string().to_char());
		++idx;
	}
	_log.set_colour();
}
#endif
