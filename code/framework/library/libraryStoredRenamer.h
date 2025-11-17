#pragma once

#include "usedLibraryStored.h"
#include "usedLibraryStoredAny.h"
#include "..\..\core\containers\array.h"

namespace Framework
{
	struct LibraryStoredReplacer;

	/**
	 *	Renamer allows to rename used library stored object (and find it again if required)
	 */

	struct LibraryStoredRenamer
	{
	public:
		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		bool load_from_xml_child_node(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _childNodeName = TXT("libraryRenamers"));

		template <typename Class>
		bool apply_to(REF_ UsedLibraryStored<Class> & _object, Library* _library = nullptr) const; // returns true if everything is ok, false if could not find changed
		bool apply_to(REF_ UsedLibraryStoredAny & _object, Library* _library = nullptr) const; // returns true if everything is ok, false if could not find changed
		bool apply_to(REF_ LibraryStoredReplacer & _replacer, Library* _library = nullptr) const; // returns true if everything is ok, false if could not find changed
		bool apply_to(REF_ LibraryName & _name) const; // returns true if changed anything

	private:
		struct Rename
		{
			String renameFrom;
			String renameTo;

			Rename() {}
			Rename(String const & _from, String const & _to) : renameFrom(_from), renameTo(_to) {}
		};
		Array<Rename> renames;
	};

	#include "libraryStoredRenamer.inl"

};
