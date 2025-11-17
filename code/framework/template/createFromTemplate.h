#pragma once

#include "..\library\libraryStored.h"

#include "..\..\core\other\simpleVariableStorage.h"

namespace Framework
{
	class Template;

	/**
	 *	check Template
	 */

	class CreateFromTemplate
	: public LibraryStored
	{
		FAST_CAST_DECLARE(CreateFromTemplate);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		CreateFromTemplate(Library * _library, LibraryName const & _name);
		virtual ~CreateFromTemplate();

		LibraryStoredRenamer const & get_renamer() const { return renamer; }
		SimpleVariableStorage const & get_mesh_generator_parameters() const { return meshGeneratorParameters; }

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	private:
		UsedLibraryStored<Template> fromTemplate;
		bool created = false;

		LibraryStoredRenamer renamer;
		SimpleVariableStorage meshGeneratorParameters;
	};

};
