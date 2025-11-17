String Name::to_string() const
{
	return String(to_char());
}

tchar const * Name::to_char() const
{
	return NameStorage::get_as_char(index);
}

Name::Name()
: index( NONE )
#ifdef AN_NAT_VIS
, natVisNameStorage(&NameStorage::access_storage())
#endif
{
}

Name::Name(String const & _name)
: index( NameStorage::find_or_create(_name.to_char()) )
#ifdef AN_NAT_VIS
, natVisNameStorage(&NameStorage::access_storage())
#endif
{
}

Name::Name(tchar const * _name)
: index(NameStorage::find_or_create(_name))
#ifdef AN_NAT_VIS
, natVisNameStorage(&NameStorage::access_storage())
#endif
{
}

Name::Name(int _index)
#ifdef AN_NAT_VIS
: natVisNameStorage(&NameStorage::access_storage())
#endif
{
	assert_slow(NameStorage::is_index_valid(_index));
	index = _index;
}

bool Name::is_valid() const
{
	return NameStorage::is_index_valid(index) && index != c_invalid.index;
}

int Name::find(tchar const* _name)
{
	return NameStorage::find(_name);
}

//

WholeNameStorage& NameStorage::access_storage()
{
#ifdef AN_CHECK_MEMORY_LEAKS
	supress_memory_leak_detection();
#endif
	static WholeNameStorage storage;
#ifdef AN_CHECK_MEMORY_LEAKS
	resume_memory_leak_detection();
#endif
	return storage;
}

Concurrency::MRSWLock& NameStorage::access_storage_lock()
{
#ifdef AN_CHECK_MEMORY_LEAKS
	supress_memory_leak_detection();
#endif
	static Concurrency::MRSWLock storageLock;
#ifdef AN_CHECK_MEMORY_LEAKS
	resume_memory_leak_detection();
#endif
	return storageLock;
}

NameStorage::NameStorage()
{
#ifdef AN_CLANG
	tsprintf(name, MAX_NAME_LENGTH, TXT("%S"),
#else
	tsprintf(name, MAX_NAME_LENGTH, TXT("%s"),
#endif
		s_none);
}

NameStorage::NameStorage(tchar const* _name)
{
#ifdef AN_CLANG
	tsprintf(name, MAX_NAME_LENGTH, TXT("%S"),
#else
	tsprintf(name, MAX_NAME_LENGTH, TXT("%s"),
#endif
		is_name_valid(_name) ? _name : s_none);
}

NameStorage::~NameStorage()
{
}

bool NameStorage::is_name_valid(tchar const* _name)
{
	return _name && _name[0] != 0;
}

int NameStorage::find(tchar const* _name)
{
	Concurrency::ScopedMRSWLockRead lock(NameStorage::access_storage_lock());
	return find_no_lock(_name);
}

int NameStorage::find_or_create(tchar const* _name)
{
	if (!is_name_valid(_name))
	{
		return NONE;
	}

	int readAtSize = 0;

	int lastValidIndex = NONE;
	int lastICase = NONE;

	{
		Concurrency::ScopedMRSWLockRead lock(NameStorage::access_storage_lock());

		readAtSize = access_storage().get_size();

		int index = find_no_lock(_name, &lastValidIndex, &lastICase);
		if (index != NONE)
		{
			return index;
		}
	}

	{
		Concurrency::ScopedMRSWLockWrite lock(NameStorage::access_storage_lock());

		if (readAtSize != access_storage().get_size())
		{
			int index = find_no_lock(_name, &lastValidIndex, &lastICase);
			if (index != NONE)
			{
				return index;
			}
		}

		return create_at_no_lock(_name, lastValidIndex, lastICase);
	}
}

bool NameStorage::is_storage_valid_no_lock()
{
	for_every(nameStorage, NameStorage::access_storage())
	{
		if (tstrlen(nameStorage->name) > MAX_NAME_LENGTH)
		{
			an_assert(false);
			return false;
		}
		if (tstrlen(nameStorage->name) > 0)
		{
			tchar ch = nameStorage->name[0];
			if (ch == 0)
			{
				an_assert(false, TXT("invalid name for %i out of %i"), for_everys_index(nameStorage), NameStorage::access_storage().get_size());
				return false;
			}
			if (ch > 128)
			{
				an_assert(false, TXT("invalid character for %i out of %i"), for_everys_index(nameStorage), NameStorage::access_storage().get_size());
				return false;
			}
		}
	}
	return true;
}

int NameStorage::find_no_lock(tchar const* _name, OPTIONAL_ OUT_ int* _lastValidIndex, OPTIONAL_ OUT_ int* _lastICase)
{
	int index = 0;

	assert_slow(is_storage_valid_no_lock());

	auto& wholeNameStorage = NameStorage::access_storage();
	auto* nameStorage = ! wholeNameStorage.is_empty()? wholeNameStorage.begin() : nullptr;
	while (nameStorage)
	{
		int icaseRes = String::diff_icase(_name, nameStorage->name);
		if (icaseRes == 0)
		{
			assign_optional_out_param(_lastValidIndex, index);
			assign_optional_out_param(_lastICase, icaseRes);
			return index;
		}
		else if (icaseRes < 0)
		{
			if (nameStorage->icaseNegIdx != NONE)
			{
				index = nameStorage->icaseNegIdx;
			}
			else
			{
				assign_optional_out_param(_lastValidIndex, index);
				assign_optional_out_param(_lastICase, icaseRes);
				return NONE;
			}
		}
		else
		{
			if (nameStorage->icasePosIdx != NONE)
			{
				index = nameStorage->icasePosIdx;
			}
			else
			{
				assign_optional_out_param(_lastValidIndex, index);
				assign_optional_out_param(_lastICase, icaseRes);
				return NONE;
			}
		}
		an_assert(wholeNameStorage.is_index_valid(index));
		nameStorage = &wholeNameStorage[index];
	}

	assign_optional_out_param(_lastValidIndex, NONE);
	assign_optional_out_param(_lastICase, 0);
	return NONE;
}

tchar const * NameStorage::get_as_char(int _index)
{
	//Concurrency::ScopedMRSWLockRead lock(NameStorage::access_storage_lock());
	if (is_index_valid(_index))
	{
		return NameStorage::access_storage()[_index].name;
	}
	return NameStorage::s_none;
}

bool NameStorage::is_index_valid(int _index)
{
	return _index >= 0 && _index < NameStorage::access_storage().get_size();
}

template <>
inline void set_to_default<Name>(Name & _object)
{
	_object = Name::invalid();
}
