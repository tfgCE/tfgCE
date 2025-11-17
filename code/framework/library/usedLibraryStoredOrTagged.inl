#pragma once

#ifdef AN_CLANG
#include "library.h"
#include "libraryPrepareContext.h"
#include "..\..\core\serialisation\serialiser.h"
#endif

#ifdef AN_DEVELOPMENT
#include "..\framework.h"
#endif

template<typename Class>
::Framework::UsedLibraryStoredOrTagged<Class>::UsedLibraryStoredOrTagged()
{
}

template<typename Class>
::Framework::UsedLibraryStoredOrTagged<Class>::UsedLibraryStoredOrTagged(UsedLibraryStoredOrTagged const & _source)
: libraryStored(_source.libraryStored)
, tagged(_source.tagged)
, cached(_source.cached)
{
}

template<typename Class>
::Framework::UsedLibraryStoredOrTagged<Class>::~UsedLibraryStoredOrTagged()
{
}

template<typename Class>
bool ::Framework::UsedLibraryStoredOrTagged<Class>::load_from_xml(IO::XML::Node const* _node, tchar const* _attrName, tchar const* _attrNameTagged, LibraryLoadingContext& _lc)
{
	bool result = true;

	if (_node)
	{
		if (IO::XML::Attribute const * attr = _node->get_attribute(_attrName))
		{
			result &= libraryStored.load_from_xml(attr, _lc);
		}
		if (IO::XML::Attribute const * attr = _node->get_attribute(_attrNameTagged))
		{
			result &= tagged.load_from_xml(attr);
		}
	}

	return result;
}

template<typename Class>
bool ::Framework::UsedLibraryStoredOrTagged<Class>::prepare_for_game_find(Library* _library, LibraryPrepareContext& _pfgContext, int32 _atLevel)
{
	bool result = true;
	IF_PREPARE_LEVEL(_pfgContext, _atLevel)
	{
		cached.clear();
		result &= libraryStored.prepare_for_game_find(_library, _pfgContext, _atLevel);
		if (auto* lst = libraryStored.get())
		{
			cached.push_back(UsedLibraryStored<Class>(lst));
		}
		if (!tagged.is_empty())
		{
			if (auto* s = _library->get_store(Class::library_type()))
			{
				s->do_for_every([this](Framework::LibraryStored* _libraryStored)
					{
						if (tagged.check(_libraryStored->get_tags()))
						{
							if (Class* asObj = fast_cast<Class>(_libraryStored))
							{
								cached.push_back(UsedLibraryStored<Class>(asObj));

							}
						}
					});
			}
		}
	}
	return result;
}

template<typename Class>
Class* ::Framework::UsedLibraryStoredOrTagged<Class>::choose_random(UsedLibraryStoredOrTagged<Class> const& _from, REF_ Random::Generator& _rg, TagCondition const& _taggedAdditionally, bool _mayIgnoreTaggedAdditionally)
{
	{
		Array<Class*> lsAll;
		for_every_ref(lsObj, _from.get())
		{
			if (_taggedAdditionally.check(lsObj->get_tags()))
			{
				lsAll.push_back(lsObj);
			}
		}
		if (!lsAll.is_empty())
		{
			int idx = _rg.get_int(lsAll.get_size());
			return lsAll[idx];
		}
		else if (_mayIgnoreTaggedAdditionally && !_taggedAdditionally.is_empty())
		{
			return choose_random(_from, _rg);
		}
	}
	return nullptr;
}

template<typename Class>
Class* ::Framework::UsedLibraryStoredOrTagged<Class>::choose_random(List<UsedLibraryStoredOrTagged<Class>> const& _from, REF_ Random::Generator& _rg, TagCondition const& _taggedAdditionally, bool _mayIgnoreTaggedAdditionally)
{
	if (! _from.is_empty())
	{
		Array<Class*> lsAll;
		for_every(ulsot, _from)
		{
			for_every_ref(lsObj, ulsot->get())
			{
				if (_taggedAdditionally.check(lsObj->get_tags()))
				{
					lsAll.push_back(lsObj);
				}
			}
		}

		sort(lsAll, LibraryStored::compare_for_sort);

		if (!lsAll.is_empty())
		{
			int idx = _rg.get_int(lsAll.get_size());
			return lsAll[idx];
		}
		else if (_mayIgnoreTaggedAdditionally && !_taggedAdditionally.is_empty())
		{
			return choose_random(_from, _rg);
		}
	}
	return nullptr;
}
