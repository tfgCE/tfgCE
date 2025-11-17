#ifndef AN_CLANG
#include "..\..\..\library\library.h"
#include "..\..\..\library\usedLibraryStored.inl"
#else
#include "library.h"
#include "usedLibraryStored.inl"
#endif

//

template<typename Class>
bool ::Framework::CustomLibraryDataSingle<Class>::load_from_xml(IO::XML::Node const * _node, tchar const * const _attr, LibraryLoadingContext & _lc)
{
	if (!_node)
	{
		return true;
	}
	bool result = true;

	use.load_from_xml(_node, _attr, _lc);
	if (!use.is_name_valid())
	{
		error_loading_xml(_node, TXT("no \"%S\" defined for custom data lookup"), _attr);
		result = false;
	}

	return result;
}

template<typename Class>
bool ::Framework::CustomLibraryDataSingle<Class>::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		result &= use.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		if (use.get())
		{
			lookedUp = fast_cast<Class>(use.get()->access_data());
		}
		if (!lookedUp)
		{
			error(TXT("could not resolve custom data \"%S\" to requested type"), use.get_name().to_string().to_char());
			result = false;
		}
	}
	return result;
}

//

template<typename Class>
bool ::Framework::CustomLibraryDataLookup<Class>::load_from_xml(IO::XML::Node const * _node, tchar const * const _childrenName, LibraryLoadingContext & _lc)
{
	if (! _node)
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
			error_loading_xml(node, TXT("no \"id\" defined for custom data lookup"));
			result = false;
		}
		else
		{
			if (newElement.data.load_from_xml(node, TXT("use"), _lc))
			{
				bool found = false;
				for_every(data, datas)
				{
					if (data->id == newElement.id)
					{
						error_loading_xml(node, TXT("two or more custom data lookup with id \"%S\" exist"), newElement.id.to_char());
						result = false;
					}
				}
				if (!found)
				{
					datas.push_back(newElement);
				}
			}
		}
	}

	return result;
}

template<typename Class>
bool ::Framework::CustomLibraryDataLookup<Class>::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		for_every(data, datas)
		{
			result &= data->data.prepare_for_game(_library, _pfgContext);
		}
	}
	return result;
}

template<typename Class>
Class* ::Framework::CustomLibraryDataLookup<Class>::get(Name const & _id) const
{
	for_every(data, datas)
	{
		if (data->id == _id)
		{
			return data->data.get();
		}
	}
	return nullptr;
}

