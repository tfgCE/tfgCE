#include "moduleData.h"

#include "..\..\core\types\names.h"
#include "..\..\core\other\parserUtils.h"

#include "..\library\library.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(ModuleData);

ModuleData::ModuleData(LibraryStored* _inLibraryStored)
: inLibraryStored(_inLibraryStored)
{
}

bool ModuleData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	inGroup = _lc.get_current_group();
	if (! _node)
	{
		return true;
	}
	if (auto const * typeAttr = _node->get_attribute(TXT("type")))
	{
		type = typeAttr->get_as_name();
	}
	// store parameters
	// first as attributes
	for_every(attr, _node->all_attributes())
	{
		if (attr->get_name() != TXT("type"))
		{
			result &= read_parameter_from(attr, _lc);
		}
	}
	// next as children (may override_ attributes)
	for_every(childNode, _node->all_children())
	{
		if (childNode->get_name() == TXT("wheresMyPoint"))
		{
			warn_loading_xml(childNode, TXT("maybe you wanted to name it wheresMyPointOnInit?"));
		}
		if (childNode->get_name() == TXT("wheresMyPointOnInit"))
		{
			result &= wheresMyPointProcessorOnInit.load_from_xml(childNode);
		}
		else
		{
			result &= read_parameter_from(childNode, _lc);
		}
	}
	return result;
}

bool ModuleData::read_parameter_from(IO::XML::Attribute const * _attr, LibraryLoadingContext & _lc)
{
	parameters.set(Name(_attr->get_name()), _attr->get_value());
	return true;
}

bool ModuleData::read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	parameters.set(Name(_node->get_name()), _node->get_text());
	return true;
}

bool ModuleData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	return true;
}

/*		
void log(String const & _name)
{
	Utils.Log.log(" +- %S : %S", _name, type.asString);
	debug_log_internal();
}
		
protected virtual void debug_log_internal()
{
	if (type.is_valid())
	{
		Gee.MapIterator<string, string> parVal = parameters.map_iterator();
		while (parVal.next())
		{
			Utils.Log.log("     %S = %S", parVal.get_key(), parVal.get_value());
		}
	}
}
*/