#include "wmp_getParameterFromLibraryStored.h"

#include "..\meshGen\meshGenGenerationContext.h"
#include "..\meshGen\meshGenParamImpl.inl"

#include "..\library\library.h"

//

using namespace Framework;

//

bool GetParameterFromLibraryStored::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	result &= libraryName.load_from_xml(_node, TXT("libraryName"));
	result &= paramName.load_from_xml(_node, TXT("paramName"));
	result &= libraryType.load_from_xml(_node, TXT("libraryType"));

	mayFail = _node->get_bool_attribute_or_from_child_presence(TXT("mayFail"), mayFail);

	for_every(child, _node->children_named(TXT("onFail")))
	{
		mayFail = true;
		onFail.load_from_xml(child);
	}

	for_every(child, _node->children_named(TXT("onSuccess")))
	{
		mayFail = true;
		onSuccess.load_from_xml(child);
	}

	error_loading_xml_on_assert(libraryName.is_set(), result, _node, TXT("\"libraryName\" missing"));
	error_loading_xml_on_assert(paramName.is_set(), result, _node, TXT("\"paramName\" missing"));

	return result;
}

bool GetParameterFromLibraryStored::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

	LibraryName actualLibraryName;
	result &= libraryName.process_for_wheres_my_point(this, _context.get_owner(), OUT_ actualLibraryName);

	Name actualParamName;
	result &= paramName.process_for_wheres_my_point(this, _context.get_owner(), OUT_ actualParamName);

	Name actualLibraryType;
	result &= libraryType.process_for_wheres_my_point(this, _context.get_owner(), OUT_ actualLibraryType);

	if (result)
	{
		LibraryStored* stored = nullptr;
		if (actualLibraryType.is_valid())
		{
			if (auto* ls = Library::get_current()->get_store(actualLibraryType))
			{
				stored = ls->find_stored(actualLibraryName);
			}
			else
			{
				if (!mayFail)
				{
					error_processing_wmp(TXT("couldn't find param \"%S\" in \"%S\""), actualParamName.to_string().to_char(), stored->get_name().to_string().to_char());
				}
				result = false;
			}
		}
		else
		{
			stored = Library::get_current()->find_stored(actualLibraryName);
		}
		if (stored)
		{
			stored->load_on_demand_if_required();
			if (auto * paramInfo = stored->get_custom_parameters().find_any_existing(actualParamName))
			{
				_value.set_raw(paramInfo->type_id(), paramInfo->get_raw());
			}
			else
			{
				if (!mayFail)
				{
					error_processing_wmp(TXT("couldn't find param \"%S\" in \"%S\""), actualParamName.to_string().to_char(), stored->get_name().to_string().to_char());
				}
				result = false;
			}
		}
		else
		{
			if (!mayFail)
			{
				error_processing_wmp(TXT("couldn't find library stored \"%S\""), actualLibraryName.to_string().to_char());
			}
			result = false;
		}
	}

	if (!result && !onFail.is_empty())
	{
		result = onFail.update(_value, _context);
	}

	if (result && !onSuccess.is_empty())
	{
		result = onSuccess.update(_value, _context);
	}

	if (mayFail)
	{
		// result should always be ok if may fail
		result = true;
	}

	return result;
}
