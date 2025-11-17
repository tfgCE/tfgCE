template <typename Class>
Class* ::Framework::AssetPackage::get() const
{
	return get<Class>(Name::invalid(), 0, true);
}

template <typename Class>
Class * ::Framework::AssetPackage::get(Name const& _id, int _idx, bool _allowDefault) const
{
	Name libType = Class::library_type();

	Entry const * foundDefault = nullptr;
	Entry const * found = nullptr;
	for_every(e, entries)
	{
		if (e->libraryStored.get_type() == libType)
		{
			if (e->id == _id)
			{
				if (e->idx == _idx)
				{
					found = e;
					break;
				}
				if (!found)
				{
					found = e;
				}
			}

			if (_allowDefault && e->isDefault && !foundDefault)
			{
				foundDefault = e;
			}
		}
	}

	if (_allowDefault && !found) found = foundDefault;

	if (found)
	{
		Class* ls = fast_cast<Class>(found->libraryStored.get());
		return ls;
	}

	return nullptr;
}

template <typename Class>
Class * ::Framework::AssetPackage::get_clamp(Name const& _id, int & _idx, bool _allowDefault) const
{
	an_assert(_idx >= 0);

	Name libType = Class::library_type();

	Entry const* foundDefault = nullptr;
	Entry const* found = nullptr;

	int idx = _idx;
	int idxDefault = _idx;

	bool foundId = false;
	for_every(e, entries)
	{
		if (e->libraryStored.get_type() == libType)
		{
			if (e->id == _id)
			{
				foundId = true;
				idx = clamp(idx, 0, e->count - 1);
				if (e->idx == idx)
				{
					found = e;
					break;
				}
			}
			if (_allowDefault && e->isDefault)
			{
				idxDefault = clamp(idxDefault, 0, e->count - 1);
				if (e->idx == idxDefault)
				{
					foundDefault = e;
				}
			}
		}
	}

	if (_allowDefault && !foundId)
	{
		found = foundDefault;
		idx = idxDefault;
	}

	if (found)
	{
		_idx = idx;
		Class* ls = fast_cast<Class>(found->libraryStored.get());
		return ls;
	}

	return nullptr;
}

template <typename Class>
Class* ::Framework::AssetPackage::get_mod(Name const& _id, int& _idx, bool _allowDefault) const
{
	an_assert(_idx >= 0);

	Name libType = Class::library_type();

	Entry const* foundDefault = nullptr;
	Entry const* found = nullptr;

	int idx = _idx;
	int idxDefault = _idx;

	bool foundId = false;
	for_every(e, entries)
	{
		if (e->libraryStored.get_type() == libType)
		{
			if (e->id == _id)
			{
				foundId = true;
				idx = mod(idx, e->count);
				if (e->idx == idx)
				{
					found = e;
					break;
				}
			}
			if (_allowDefault && e->isDefault)
			{
				idxDefault = mod(idxDefault, e->count);
				if (e->idx == idxDefault)
				{
					foundDefault = e;
				}
			}
		}
	}

	if (_allowDefault && !foundId)
	{
		found = foundDefault;
		idx = idxDefault;
	}

	if (found)
	{
		_idx = idx;
		Class* ls = fast_cast<Class>(found->libraryStored.get());
		return ls;
	}

	return nullptr;
}

template <typename Class>
Class * ::Framework::AssetPackage::get_random(Name const& _id, Random::Generator& _rg, bool _allowDefault) const
{
	an_assert(_idx >= 0);

	Name libType = Class::library_type();

	int count = 0;
	int countDefault = 0;

	bool foundId = false;
	for_every(e, entries)
	{
		if (e->libraryStored.get_type() == libType)
		{
			if (e->id == _id)
			{
				foundId = true;
				count = e->count;
				break;
			}
			if (_allowDefault && e->isDefault)
			{
				countDefault = e->count;
			}
		}
	}
	if (_allowDefault && !foundId)
	{
		count = countDefault;
	}

	int idx = _rg.get_int(max(1, count));

	return get(_id, idx);
}

template <typename Class>
Class * ::Framework::AssetPackage::get_random_any(Random::Generator& _rg) const
{
	Name libType = Class::library_type();

	int count = 0;

	for_every(e, entries)
	{
		if (e->libraryStored.get_type() == libType)
		{
			++count;
		}
	}

	if (count > 0)
	{
		int idx = _rg.get_int(count);
		for_every(e, entries)
		{
			if (e->libraryStored.get_type() == libType)
			{
				if (idx == 0)
				{
					Class* ls = fast_cast<Class>(e->libraryStored.get());
					return ls;
				}
				--idx;
			}
		}
	}

	return nullptr;
}

template <typename Class>
void ::Framework::AssetPackage::create_temporary_copies_of()
{
	create_temporary_copies_of_type(Class::library_type());
}

template <typename Class>
void ::Framework::AssetPackage::do_for_all_of(std::function<void(Class* _stored, Name const& _id, int _idx)> _do)
{
	Name libType = Class::library_type();

	for_every(e, entries)
	{
		if (e->libraryStored.get_type() == libType)
		{
			if (auto* ls = fast_cast<Class>(e->libraryStored.get()))
			{
				_do(ls, e->id, e->idx);
			}
		}
	}
}

template <typename Class>
void ::Framework::AssetPackage::do_for_all_unique_of(std::function<void(Class* _stored, Name const& _id, int _idx)> _do)
{
	do_for_all_unique_of_type(Class::library_type(), [&_do](LibraryStored* _ls, Name const& _id, int _idx)
		{
			auto* o = fast_cast<Class>(_ls);
			an_assert(o);
			_do(o, _id, _idx);
		});
}
