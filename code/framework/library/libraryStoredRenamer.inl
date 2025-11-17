template <typename Class>
bool LibraryStoredRenamer::apply_to(REF_ UsedLibraryStored<Class> & _object, Library* _library) const
{
	bool result = true;
	if (_object.is_name_valid())
	{
		LibraryName objectName = _object.get_name();
		if (apply_to(REF_ objectName))
		{
			_object.set_name(LibraryName(objectName));
			if (_library)
			{
				result &= _object.find(_library);
			}
		}
	}
	return result;
}
