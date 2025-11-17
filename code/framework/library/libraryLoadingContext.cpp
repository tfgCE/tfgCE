#include "libraryLoadingContext.h"

#include "libraryName.h"

#include "..\meshGen\meshGenValueDef.h"

#include "..\..\core\other\simpleVariableStorage.h"

using namespace Framework;

String LibraryLoadingContext::build_id() const
{
	String result;

	for_every(id, idStack)
	{
		if (!result.is_empty())
		{
			result += TXT("; ");
		}
		result = result + *id;
	}

	return result;
}

bool LibraryLoadingContext::load_group_into(SimpleVariableStorage & _storage) const
{
	_storage.do_for_every([this](SimpleVariableInfo & variable)
	{
		if (variable.type_id() == type_id<LibraryName>())
		{
			LibraryName libName = variable.get<LibraryName>();
			// only if group not defined but name is defined
			// leave if:
			//		1. group defined
			//		2. whole library name invalid (leave it empty)
			if (!libName.get_group().is_valid() &&
				libName.get_name().is_valid())
			{
				libName.set_group(get_current_group());
				variable.access<LibraryName>() = libName;
			}
		}
	});

	return true;
}

bool LibraryLoadingContext::load_group_into(MeshGeneration::ValueDef<LibraryName> & _libraryName) const
{
	if (_libraryName.is_value_set())
	{
		auto lnValue = _libraryName.get_value();
		if (!lnValue.get_group().is_valid())
		{
			lnValue.set_group(get_current_group());
			_libraryName.set(lnValue);
		}
	}

	return true;
}
