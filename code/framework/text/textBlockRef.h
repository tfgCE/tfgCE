#pragma once

#include "..\library\usedLibraryStored.h"

#include "..\..\core\tags\tag.h"

#include "..\..\core\memory\refCountObjectPtr.h"

namespace Random
{
	class Generator;
};

namespace Framework
{
	class TextBlock;
	struct TextBlockInstance;
	struct TextBlocks;
	struct TextBlockRef;

	typedef UsedLibraryStored<TextBlock> TextBlockPtr;
	typedef RefCountObjectPtr<TextBlockInstance> TextBlockInstancePtr;

	/**
	 *	works as container for pointer to text blocks (actually, used-libary-stored for text-blocks)
	 */
	struct TextBlocks
	{
	public:
		void copy_from(TextBlockRef const & _source); // only "all"!

		TextBlock* get_one(Random::Generator & _rg) const;

		Array<TextBlockPtr> const & get_all() const {
#ifdef AN_DEVELOPMENT
			an_assert(!all.is_empty() || should_all_be_empty(), TXT("not cached, check TextBlockRef's cache methods"));
#endif
			return all;
		}
		bool is_empty() const { return all.is_empty(); }

		bool serialise(Serialiser & _serialiser);

	protected:
		Array<TextBlockPtr> all;
#ifdef AN_DEVELOPMENT
		virtual bool should_all_be_empty() const { return all.is_empty(); }
#endif
	};

	/**
	 *	Works as set of text blocks, referenced by name, tags or included (private)
	 */
	struct TextBlockRef
	: public TextBlocks
	{
	public:
		bool cache(Library* _library, bool _ignoreMain = false); // cache just those what we include here
		bool cache_everything(Library* _library, bool _ignoreMain = false); // go down into text blocks and cache all included
		void prepare_to_unload();

		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _id, std::function<void(TextBlock*)> _perform_on_loaded_text_block = nullptr);
		bool load_from_xml(IO::XML::Node const * _node, tchar const * _childrenNodeName, LibraryLoadingContext & _lc, tchar const * _id, std::function<void(TextBlock*)> _perform_on_loaded_text_block = nullptr);

		bool is_empty() const { return include.is_empty() && includeTags.is_empty() && includePrivate.is_empty(); }

	protected:
#ifdef AN_DEVELOPMENT
		virtual bool should_all_be_empty() const { return include.is_empty() && includeTags.is_empty() && includePrivate.is_empty(); }
#endif

	private:
		Array<TextBlockPtr> include;
		Array<Tags> includeTags;
		Array<TextBlockPtr> includePrivate;

		bool cache_everything(Library* _library, bool _ignoreMain, TextBlockRef const & _source);
		bool cache_everything_from(Array<TextBlockPtr> & _array, Library* _library, bool _ignoreMain, TextBlockRef const & _source);
	};
};
