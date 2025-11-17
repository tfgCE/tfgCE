#pragma once

#include "..\..\core\containers\array.h"
#include "..\..\core\tags\tag.h"

#include "libraryLoadLevel.h"
#include "usedLibraryStoredAny.h"

namespace Framework
{
	/**
	 *	May reference an object
	 *	or be a set of tags
	 */
	struct LibraryGlobalReferences
	{
	public:
		void clear();

		template<typename Class>
		Class* get(Name const & _id, bool _mayFail = false) const
		{
			Class* retVal = nullptr;
			for_every(ref, references)
			{
				if (ref->id == _id)
				{
					retVal = fast_cast<Class>(ref->stored.get());
					if (retVal)
					{
						break;
					}
				}
			}
			if (!_mayFail && ! retVal)
			{
#ifdef AN_DEVELOPMENT
				ScopedOutputLock outputLock;
				output(TXT("references available:"));
				for_every(ref, references)
				{
					output(TXT("  \"%S\" -> \"%S\""), ref->id.to_char(), ref->stored.get_name().to_string().to_char());
				}
				if (! Framework::is_preview_game())
#endif
				{
					an_assert(retVal, TXT("didn't find reference id \"%S\""), _id.to_char());
				}
			}
			return retVal;
		}

		LibraryName const & get_name(Name const & _id, bool _mayFail = false) const
		{
			for_every(ref, references)
			{
				if (ref->id == _id)
				{
					return ref->stored.get_name();
				}
			}
			if (!_mayFail)
			{
				an_assert(false, TXT("didn't find reference id \"%S\""), _id.to_char());
			}
			return LibraryName::invalid();
		}

		Tags const& get_tags(Name const& _id) const;

		bool load_from_xml(IO::XML::Node const * _containerNode, LibraryLoadingContext & _lc);
		bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		bool unload(LibraryLoadLevel::Type _libraryLoadLevel);

	private:
		struct Entry
		{
			bool mayFail = false;
			Name id;
			UsedLibraryStoredAny stored;
			Tags tags;
		};
		Array<Entry> references;
	};
};
