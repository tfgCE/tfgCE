#include "wmp_isLibraryNameValid.h"

#include "..\library\libraryName.h"

using namespace Framework;

//

bool IsLibraryNameValid::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	WheresMyPoint::Value resultValue;
	if (!is_valid(_value, resultValue))
	{
		error_processing_wmp(TXT("not a library name!"));
		result = false;
	}
	_value = resultValue;

	return result;
}

bool IsLibraryNameValid::is_valid(WheresMyPoint::Value const& _value, REF_ WheresMyPoint::Value& _resultValue)
{
	if (_value.get_type() == type_id<LibraryName>())
	{
		_resultValue.set<bool>(_value.get_as<LibraryName>().is_valid());
		return true;
	}
	else
	{
		return false;
	}
}

//

bool IfLibraryNameEmpty::load_from_xml(IO::XML::Node const* _node)
{
	bool result = base::load_from_xml(_node);

	result &= doIfEmptyToolSet.load_from_xml(_node);

	return result;
}

bool IfLibraryNameEmpty::update(REF_ WheresMyPoint::Value& _value, WheresMyPoint::Context& _context) const
{
	bool result = true;

	WheresMyPoint::Value resultValue;
	if (IsLibraryNameValid::is_valid(_value, resultValue))
	{
		if (! resultValue.get_as<bool>())
		{
			result &= doIfEmptyToolSet.update(_value, _context);
		}
	}
	else
	{
		error_processing_wmp(TXT("not a library name!"));
		result = false;
	}

	return result;
}

//

bool IfLibraryNameValid::load_from_xml(IO::XML::Node const* _node)
{
	bool result = base::load_from_xml(_node);

	result &= doIfValidToolSet.load_from_xml(_node);

	return result;
}

bool IfLibraryNameValid::update(REF_ WheresMyPoint::Value& _value, WheresMyPoint::Context& _context) const
{
	bool result = true;

	WheresMyPoint::Value resultValue;
	if (IsLibraryNameValid::is_valid(_value, resultValue))
	{
		if (resultValue.get_as<bool>())
		{
			result &= doIfValidToolSet.update(_value, _context);
		}
	}
	else
	{
		error_processing_wmp(TXT("not a library name!"));
		result = false;
	}

	return result;
}

//
