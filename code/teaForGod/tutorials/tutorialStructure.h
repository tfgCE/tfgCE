#pragma once

#include "tutorialTree.h"

#include "..\..\framework\library\libraryStored.h"

namespace TeaForGodEmperor
{
	// works as interface to TutorialTreeElement
	class TutorialStructure
	: public Framework::LibraryStored
	, public TutorialTreeElement
	{
		FAST_CAST_DECLARE(TutorialStructure);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_BASE(TutorialTreeElement);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		TutorialStructure(Framework::Library* _library, Framework::LibraryName const& _name);
		virtual ~TutorialStructure();

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
	};
};
