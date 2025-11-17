template <typename Class>
bool ::Framework::LibraryStoredReplacerEntry::apply_to(REF_ Class * & _object) const
{
	if (_object == replace.get())
	{
		_object = fast_cast<Class>(with.get());
		return true;
	}
	else
	{
		return false;
	}
}

//

template <typename Class>
bool ::Framework::LibraryStoredReplacer::apply_to(REF_ Class * & _object) const
{
	for_every(replacer, replacers)
	{
		if (replacer->apply_to<Class>(REF_ _object))
		{
			return true;
		}
	}
	return false;
}
