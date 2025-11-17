#pragma once

#include "..\library\libraryStored.h"

namespace Framework
{
	class CreateFromTemplate;

	/**
	 *	Template is set of library stored objects that are copied/instanciated when template is processed (with CreateFromTemplate object)
	 *	CreateFromTemplate uses template at very first stage of prepare_for_game to add new extra objects to the library
	 *	Not all objects support being created with template (it will have to be implemented)
	 *	When creating template, objects should use CreateFromTemplate's renamer to update all library names
	 *	CreateFromTemplate may only update existing objects - fill some data, add item variants etc.
	 *	Each type's create_from_template should know how to do that!
	 */
	
	class Template
	: public LibraryStored
	{
		FAST_CAST_DECLARE(Template);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		Template(Library * _library, LibraryName const & _name);
		virtual ~Template();

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		override_ void prepare_to_unload();

	private:
		Array<RefCountObjectPtr<LibraryStored>> objects; // all templates for objects to be created, those are held within template

		friend class CreateFromTemplate;
	};

};
