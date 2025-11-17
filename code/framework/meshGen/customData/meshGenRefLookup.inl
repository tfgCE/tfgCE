template<typename ClassRef>
bool RefLookup<ClassRef>::load_from_xml(IO::XML::Node const * _node, tchar const * const _childrenName, tchar const * _useObjAttr, tchar const * _ownObjChild, LibraryLoadingContext & _lc)
{
	if (!_node)
	{
		return true;
	}
	bool result = true;
	for_every(node, _node->children_named(_childrenName))
	{
		Element newElement;
		newElement.id = node->get_name_attribute(TXT("id"));
		if (!newElement.id.is_valid())
		{
			error_loading_xml(node, TXT("no \"id\" defined for ref lookup"));
			result = false;
		}
		else
		{
			if (newElement.ref.load_from_xml(node, _useObjAttr, _ownObjChild, _lc))
			{
				bool found = false;
				for_every(ref, refs)
				{
					if (ref->id == newElement.id)
					{
						error_loading_xml(node, TXT("two or more ref lookup with id \"%S\" exist"), newElement.id.to_char());
						result = false;
					}
				}
				if (!found)
				{
					refs.push_back(newElement);
				}
			}
		}
	}

	return result;
}

template<typename ClassRef>
bool RefLookup<ClassRef>::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		for_every(ref, refs)
		{
			result &= ref->ref.prepare_for_game(_library, _pfgContext);
		}
	}
	return result;
}

template<typename ClassRef>
ClassRef const * RefLookup<ClassRef>::get(Name const & _id) const
{
	for_every(ref, refs)
	{
		if (ref->id == _id)
		{
			return &ref->ref;
		}
	}
	return nullptr;
}