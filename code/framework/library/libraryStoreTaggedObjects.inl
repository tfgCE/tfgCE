template <typename StoredClass>
LibraryStoreTaggedObjects<StoredClass>::LibraryStoreTaggedObjects(LibraryStore<StoredClass>* _store, TagCondition const & _tagCondition)
: store(_store)
, storeBegin(this, _store->get_all_stored().begin())
, storeEnd(this, _store->get_all_stored().end())
, tagCondition(&_tagCondition)
{
	// get to first tagged
	while (storeBegin != storeEnd &&
		   ! check(*storeBegin))
	{
		++ storeBegin;
	}
}

template <typename StoredClass>
LibraryStoreTaggedObjects<StoredClass>::LibraryStoreTaggedObjects(LibraryStore<StoredClass>* _store, WorldSettingCondition const & _settingCondition, WorldSettingConditionContext const & _settingConditionContext)
: store(_store)
, storeBegin(this, _store->get_all_stored().begin())
, storeEnd(this, _store->get_all_stored().end())
, settingCondition(&_settingCondition)
{
	// get to first tagged
	while (storeBegin != storeEnd &&
		   ! check(*storeBegin))
	{
		++ storeBegin;
	}

	if (storeBegin == storeEnd)
	{
		settingConditionContext.try_fallback();
		storeBegin = Iterator(this, _store->get_all_stored().begin());

		// get to first tagged
		while (storeBegin != storeEnd &&
			!check(*storeBegin))
		{
			++storeBegin;
		}
	}
}

template <typename StoredClass>
bool LibraryStoreTaggedObjects<StoredClass>::check(StoredClass const * _object) const
{
	if (tagCondition)
	{
		return tagCondition->check(_object->get_tags());
	}
	else if (settingCondition)
	{
		return settingConditionContext.check(_object, *settingCondition);
	}
	return false;
}

template <typename StoredClass>
LibraryStoreTaggedObjects<StoredClass>::Iterator::Iterator(LibraryStoreTaggedObjects const* _owner, typename List<RefCountObjectPtr<StoredClass>>::ConstIterator const & _startAt)
: owner(_owner)
, iterator(_startAt)
{
}

template <typename StoredClass>
LibraryStoreTaggedObjects<StoredClass>::Iterator::Iterator(Iterator const & _other)
: owner(_other.owner)
, iterator(_other.iterator)
{
}

template <typename StoredClass>
typename LibraryStoreTaggedObjects<StoredClass>::Iterator & LibraryStoreTaggedObjects<StoredClass>::Iterator::operator ++ ()
{
	an_assert(iterator != owner->storeEnd.iterator);
	++ iterator;
	while (iterator != owner->storeEnd.iterator)
	{
		if (owner->check((*iterator).get()))
		{
			break;
		}
		++ iterator;
	}
	return *this;
}

template <typename StoredClass>
typename LibraryStoreTaggedObjects<StoredClass>::Iterator & LibraryStoreTaggedObjects<StoredClass>::Iterator::operator -- ()
{
	an_assert(iterator != owner->storeBegin);
	-- iterator;
	while (iterator != owner->storeBegin)
	{
		if (owner->check((*iterator).get()))
		{
			break;
		}
		-- iterator;
	}
	return *this;
}

template <typename StoredClass>
typename LibraryStoreTaggedObjects<StoredClass>::Iterator LibraryStoreTaggedObjects<StoredClass>::Iterator::operator ++ (int)
{
	Iterator copy = *this;
	operator ++();
	return copy;
}

template <typename StoredClass>
typename LibraryStoreTaggedObjects<StoredClass>::Iterator LibraryStoreTaggedObjects<StoredClass>::Iterator::operator -- (int)
{
	Iterator copy = *this;
	operator --();
	return copy;
}
