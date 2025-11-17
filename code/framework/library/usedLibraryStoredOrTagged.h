#pragma once

#include "usedLibraryStored.h"

// don't forget to include usedLibraryStoredOrTagged.inl in cpp

namespace Framework
{
	class Library;
	struct LibraryLoadingContext;
	struct LibraryPrepareContext;

	template <typename Class>
	struct UsedLibraryStoredOrTagged
	{
	public:
		// if _mayIgnoreTaggedAdditionally is true then it will first try to use _taggedAdditionally but if that fails, will try without it
		static Class* choose_random(UsedLibraryStoredOrTagged<Class> const& _from, REF_ Random::Generator& _rg, TagCondition const& _taggedAdditionally = TagCondition::empty(), bool _mayIgnoreTaggedAdditionally = false);
		static Class* choose_random(List<UsedLibraryStoredOrTagged<Class>> const& _from, REF_ Random::Generator& _rg, TagCondition const & _taggedAdditionally = TagCondition::empty(), bool _mayIgnoreTaggedAdditionally = false);

	public:
		AN_NOT_CLANG_INLINE UsedLibraryStoredOrTagged();
		AN_NOT_CLANG_INLINE UsedLibraryStoredOrTagged(UsedLibraryStoredOrTagged const & _source);
		AN_NOT_CLANG_INLINE ~UsedLibraryStoredOrTagged();

	public:
		Array<UsedLibraryStored<Class>> const& get() const { return cached; }

	public:
		bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc) { return load_from_xml(_node, TXT("name"), TXT("tagged"), _lc); }
		bool load_from_xml(IO::XML::Node const* _node, tchar const* _attrName, tchar const* _attrNameTagged, LibraryLoadingContext& _lc);

		// will use renamer of pfgContext
		bool prepare_for_game_find(Library* _library, LibraryPrepareContext& _pfgContext, int32 _atLevel);

	private:
		UsedLibraryStored<Class> libraryStored;
		TagCondition tagged;

		Array<UsedLibraryStored<Class>> cached;
	};
};

#ifdef AN_CLANG
//#include "usedLibraryStoredOrTagged.inl"
#endif
