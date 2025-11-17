#pragma once

#include "customLibraryStoredData.h"

#define REGISTER_CUSTOM_LIBRARY_STORED_DATA(_name, _class) \
	::Framework::CustomLibraryStoredData::register_custom_library_data(Name(TXT(#_name)), []() { return new _class(); }); 

namespace Framework
{
	namespace CustomLibraryDatas
	{
		void initialise_static();
	};
};
