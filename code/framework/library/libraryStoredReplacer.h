#pragma once

#include "usedLibraryStoredAny.h"
#include "..\..\core\containers\array.h"

namespace Framework
{
	struct LibraryStoredReplacer;
	struct LibraryStoredRenamer;

	/**
	 *	Replacer replaces existing object with different one.
	 *	It has to know what object should be replaced by which one.
	 *	Can be applied at runtime, has to be prepared earlier.
	 */

	struct LibraryStoredReplacerEntry
	{
	private: friend struct LibraryStoredReplacer;
			 friend struct LibraryStoredRenamer;
		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		bool apply_to(REF_ UsedLibraryStoredAny & _usedLibraryStoredAny) const;
		
		template <typename Class>
		bool apply_to(REF_ Class * & _object) const;

	private:
		UsedLibraryStoredAny replace;
		UsedLibraryStoredAny with;
	};

	struct LibraryStoredReplacer
	{
	public:
		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		bool load_from_xml_child_node(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _childNodeName = TXT("replacers"));
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

		bool apply_to(REF_ UsedLibraryStoredAny & _usedLibraryStoredAny) const;

		template <typename Class>
		bool apply_to(REF_ Class * & _object) const;

	private:
		Array<LibraryStoredReplacerEntry> replacers;

		friend struct LibraryStoredRenamer;
	};

};
