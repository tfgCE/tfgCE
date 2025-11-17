#include "customLibraryDatas.h"
#include "customLibraryStoredData.h"

#include "..\appearance\controllers\walkers.h"

using namespace Framework;

void ::Framework::CustomLibraryDatas::initialise_static()
{
	REGISTER_CUSTOM_LIBRARY_STORED_DATA(walkerLib, ::Framework::AppearanceControllersLib::Walkers::Lib);
}