#include "modulesOwnerVariable.h"

#include "..\library\libraryLoadingContext.h"
#include "..\library\libraryName.h"

using namespace Framework;

bool Framework::load_group_into(ModulesOwnerVariable<LibraryName> & _libraryName, LibraryLoadingContext & _lc)
{
	if (_libraryName.is_value_set())
	{
		auto lnValue = _libraryName.get_value();
		if (!lnValue.get_group().is_valid())
		{
			lnValue.set_group(_lc.get_current_group());
			_libraryName.set(lnValue);
		}
	}

	return true;
}
