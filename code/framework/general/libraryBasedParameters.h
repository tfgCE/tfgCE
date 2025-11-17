#pragma once

#include "..\library\libraryStored.h"
#include "..\library\usedLibraryStored.h"

#include "..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	// it made more sense when custom parameters were not present? Now it is just LibraryStored with a different name, easier to find as a separate class/library store
	class LibraryBasedParameters
	: public LibraryStored
	{
		FAST_CAST_DECLARE(LibraryBasedParameters);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		LibraryBasedParameters(Library * _library, LibraryName const & _name);

		SimpleVariableStorage const& get_parameters() const { return get_custom_parameters(); }
		SimpleVariableStorage& access_parameters() { return access_custom_parameters(); }

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		override_ void prepare_to_unload();
	};
};
