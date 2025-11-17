#pragma once

#include "..\..\library\libraryStored.h"

namespace Framework
{
	class PostProcess;
	class PostProcessChain;

	class PostProcessChainDefinition
	: public LibraryStored
	{
		FAST_CAST_DECLARE(PostProcessChainDefinition);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		PostProcessChainDefinition(Library * _library, LibraryName const & _name);

		void setup(PostProcessChain* _chain);
		void add_to(PostProcessChain* _chain);

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	private:
		struct Element
		{
			UsedLibraryStored<PostProcess> postProcess;
			Tags requires;
			Tags disallows;
			
			bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
			bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		};
		Array<Element> elements;

	};

};
