#pragma once

#include "localisedString.h"
#include "stringFormatter.h"

#include "..\library\libraryStored.h"
#include "..\library\usedLibraryStored.h"

#include "..\..\core\random\random.h"
#include "..\..\core\pieceCombiner\pieceCombiner.h"

#include "textBlockGenerator.h"

namespace Framework
{
	struct TextBlockInstance;

	class TextBlock
	: public LibraryStored
	{
		FAST_CAST_DECLARE(TextBlock);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		TextBlock(Library * _library, LibraryName const & _name);

		bool is_main() const { return isMain.is_set()? isMain.get() : true; }
		bool is_private() const { return isPrivate; }

		void be_main() { isMain = true; }
		void be_private() { isPrivate = true; }

		LocalisedString const & get_text() const { return text; }

		int get_allow_empty_lines() const { return allowEmptyLines; }

	public:
		TextBlockInstancePtr create_instance(tchar const * _format, StringFormatterParams & _params, Random::Generator _usingGenerator = Random::Generator()) const;
		TextBlockInstancePtr create_instance(tchar const * _format, IStringFormatterParamsProvider const * _provider, Random::Generator _usingGenerator = Random::Generator()) const;
		TextBlockInstancePtr create_instance(tchar const * _format, ProvideStringFormatterParams _provideFunc, Random::Generator _usingGenerator = Random::Generator()) const;

		String format(tchar const * _format, StringFormatterParams & _params) const;
		String format(tchar const * _format, IStringFormatterParamsProvider const * _provider) const;
		String format(tchar const * _format, ProvideStringFormatterParams _provideFunc) const;

	public: // loading utils
		static bool create_private_from_xml(bool _main, IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _id, int _idx, OUT_ RefCountObjectPtr<TextBlock> & _created); // load private - requires id
		static bool load_private_into_array_from_xml(bool _main, IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _id, REF_ Array<TextBlockPtr> & _array, std::function<void(TextBlock*)> _perform_on_loaded_text_block = nullptr);

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		override_ void prepare_to_unload();

	private: friend struct TextBlockInstance; // this is function used as formatting by text block instance
		String format_text_for_instance(tchar const * _format, StringFormatterParams & _params) const;

	private:
		Optional<bool> isMain; // if can be used as main block - rule of thumb: private are not main text blocks, if not set, assumed main block
		bool isPrivate = false; // private - is inside an object and should not be accessible from outside, although stored in library
		// all are stored in library - for many purposes. one advantage is that we call prepare_to_unload from library only
		// for generation, private are used only within this generation, although all that are private from main are used!
		
		bool textOnly = true; // if no params set, we do very simple instance generation skipping TextBlockGenerator
		int allowEmptyLines = 0; // no empty lines allowed between blocks of text

		LocalisedString text;
		CACHED_ Array<Name> paramsRequiredByText; // only by text, others are done with text block instance, cached in prepare_for_game

		TextBlockRef include; // for generation

		// allows loading through paramConnector and paramTextBlocks structure
		// loads as [plug]/[socket] pair with multiple paramConnectors
		// textBlockPiece may be loaded as piece combiner piece to add additional functionality
		RefCountObjectPtr<PieceCombiner::Piece<TextBlockGenerator>> textBlockPiece;

		Array<TextBlockPtr> subTextBlocks; // paramTextBlocks

		void add_sub_text_blocks(REF_ PieceCombiner::Generator<TextBlockGenerator> & _generator) const;

		friend struct TextBlockRef;
	};

	// helper macros to access proper text blocks, filtering main or not private
	#define for_all_main_text_blocks(textBlock, library) for_every(ptr##textBlock, library->get_text_blocks().get_all_stored()) if (auto * textBlock = ptr##textBlock->get()) textBlock->is_main_text_block())
	#define for_available_text_blocks(textBlock, library) for_every(ptr##textBlock, library->get_text_blocks().get_all_stored()) if (auto * textBlock = ptr##textBlock->get()) if (! textBlock->is_private())
};
