#include "mggc_parameter.h"

#include "..\meshGenGenerationContext.h"

#include "..\..\library\libraryName.h"

#include "..\..\..\core\io\xml.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;
using namespace GenerationConditions;

//

template <typename Class>
Class const * ParameterBase::get_parameter(ElementInstance& _instance, Name const& _name) const
{
	if (useGlobal)
	{
		return _instance.access_context().get_global_parameter<Class>(_name);
	}
	else
	{
		return _instance.access_context().get_parameter<Class>(_name);
	}
}

bool ParameterBase::has_any_parameter(ElementInstance& _instance, Name const& _name) const
{
	if (useGlobal)
	{
		TypeId unusedTypeId;
		void const* unusedValue;
		return _instance.access_context().get_any_global_parameter(_name, unusedTypeId, unusedValue);
	}
	else
	{
		return _instance.access_context().has_any_parameter(_name);
	}
}

bool ParameterBase::check(ElementInstance & _instance) const
{
	// check one condition after another until it fails
	if (!has_any_parameter(_instance, parameter))
	{
		return false;
	}
	if (isEqualToName.is_set())
	{
		if (Name const * value = get_parameter<Name>(_instance, parameter))
		{
			if (*value != isEqualToName.get())
			{
				return false;
			}
		}
		else
		{
			// not found, it can't be equal
			return false;
		}
	}
	else if (isEqualToInt.is_set())
	{
		if (int const * value = get_parameter<int>(_instance, parameter))
		{
			if (*value != isEqualToInt.get())
			{
				return false;
			}
		}
		else
		{
			// not found, it can't be equal
			return false;
		}
	}
	else if (isEqualToBool.is_set())
	{
		if (bool const * value = get_parameter<bool>(_instance, parameter))
		{
			if (*value != isEqualToBool.get())
			{
				return false;
			}
		}
		else
		{
			// not found, it can't be equal
			return false;
		}
	}
	else
	{
		if (bool const * value = get_parameter<bool>(_instance, parameter))
		{
			return *value;
		}
		if (LibraryName const * ln = get_parameter<LibraryName>(_instance, parameter))
		{
			return ln->is_valid();
		}
		if (Name const * value = get_parameter<Name>(_instance, parameter))
		{
			return value->is_valid();
		}
		if (int const * value = get_parameter<int>(_instance, parameter))
		{
			return *value != 0;
		}
		if (float const * value = get_parameter<float>(_instance, parameter))
		{
			return *value != 0.0f;
		}
		if (Vector2 const * value = get_parameter<Vector2>(_instance, parameter))
		{
			return ! value->is_zero();
		}
		if (Vector3 const * value = get_parameter<Vector3>(_instance, parameter))
		{
			return ! value->is_zero();
		}
		if (Range2 const * value = get_parameter<Range2>(_instance, parameter))
		{
			return ! value->is_empty() && ! value->length().is_zero();
		}
		if (RangeInt2 const * value = get_parameter<RangeInt2>(_instance, parameter))
		{
			return ! value->is_empty();
		}

		error_generating_mesh(_instance, TXT("don't know how to handle parameter \"%S\""), parameter.to_char())
	}
	return true;
}

bool ParameterBase::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	parameter = _node->get_name_attribute(TXT("name"), parameter);
	if (!parameter.is_valid())
	{
		error_loading_xml(_node, TXT("generation condition parameter has no parameter provided"));
		result = false;
	}

	if (_node->has_attribute(TXT("isEqualName")))
	{
		error_loading_xml(_node, TXT("isEqualName is depreacated, won't be loaded, use isEqualToName"));
		result = false;
	}
	if (_node->has_attribute(TXT("isEqualBool")))
	{
		error_loading_xml(_node, TXT("isEqualBool is depreacated, won't be loaded, use isEqualToBool"));
		result = false;
	}
	isEqualToName.load_from_xml(_node->get_attribute(TXT("isEqualToName")));
	isEqualToInt.load_from_xml(_node->get_attribute(TXT("isEqualToInt")));
	isEqualToBool.load_from_xml(_node->get_attribute(TXT("isEqualToBool")));

	return result;
}

