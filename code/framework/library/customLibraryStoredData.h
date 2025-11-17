#pragma once

#include <functional>

#include "libraryStored.h"

namespace Framework
{
	class CustomLibraryData
	: public RefCountObject
	{
		FAST_CAST_DECLARE(CustomLibraryData);
		FAST_CAST_END();
	public:
		CustomLibraryData() {}
		virtual ~CustomLibraryData() {}

#ifdef AN_DEVELOPMENT
		virtual void ready_for_reload() { todo_note(TXT("implement_ me!")); }
#endif
		virtual bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc) { return true; }
		virtual bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext) { return true; }
	};

	class CustomLibraryStoredData
	: public LibraryStored
	// IDebugableObject
	{
		FAST_CAST_DECLARE(CustomLibraryStoredData);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		typedef std::function< CustomLibraryData* ()> ConstructCustomLibraryData;

		static void register_custom_library_data(Name const & _name, ConstructCustomLibraryData _func);
		static void initialise_static();
		static void close_static();

		CustomLibraryStoredData(Library * _library, LibraryName const & _name);
		virtual ~CustomLibraryStoredData();

		CustomLibraryData * access_data() { return data.get(); }
		CustomLibraryData const * get_data() const { return data.get(); }

	public: // LibraryStored
#ifdef AN_DEVELOPMENT
		override_ void ready_for_reload();
#endif
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	private:
		Name type;
		RefCountObjectPtr<CustomLibraryData> data;

		static CustomLibraryData* create_library_data(Name const & _type);
	};
};
