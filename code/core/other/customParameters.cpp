#include "customParameters.h"

//

CustomParameter CustomParameter::s_notSet;

CustomParameter::~CustomParameter()
{
	clear_cached_values();
}

void CustomParameter::clear_cached_values()
{
	lock.acquire_write();
	for_every_ptr(cachedValue, cachedValues)
	{
		delete cachedValue;
	}
	cachedValues.clear();
	lock.release_write();
}

//

CustomParameters::CustomParameters()
{
}

CustomParameters::~CustomParameters()
{
}

void CustomParameters::set(Name const & _parName, String const & _parValue)
{
	access(_parName).set(_parValue);
}

void CustomParameters::set(String const & _parName, String const & _parValue)
{
	if (_parName.has_suffix(TXT("Param")))
	{
		access(Name(_parName.get_left(_parName.get_length() - 5))).set_as_param_name(Name(_parValue));
	}
	else
	{
		set(Name(_parName), _parValue);
	}
}

bool CustomParameters::is_set(Name const & _parName) const
{
	return parameters.has_key(_parName);
}

CustomParameter const & CustomParameters::get(Name const & _parName) const
{
	if (auto* par = parameters.get_existing(_parName))
	{
		return *par;
	}
	else
	{
		return CustomParameter::not_set();
	}
}

CustomParameter & CustomParameters::access(Name const & _parName)
{
	if (auto* par = parameters.get_existing(_parName))
	{
		return *par;
	}
	else
	{
		parameters[_parName].touch();
		if (auto* par = parameters.get_existing(_parName))
		{
			return *par;
		}
		else
		{
			an_assert(false, TXT("impossible!"));
			return CustomParameter::accessible_not_set();
		}
	}
}

bool CustomParameters::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	for_every(attr, _node->all_attributes())
	{
		set(attr->get_name(), attr->get_as_string());
	}
	for_every(childNode, _node->all_children())
	{
		set(childNode->get_name(), childNode->get_text());
	}

	return result;
}
