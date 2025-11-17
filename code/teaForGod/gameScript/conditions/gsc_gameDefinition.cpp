#include "gsc_gameDefinition.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\library\gameDefinition.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Conditions;

//

bool Conditions::GameDefinition::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	tagged.load_from_xml_attribute_or_child_node(_node, TXT("tagged"));

	return result;
}

bool Conditions::GameDefinition::check(Framework::GameScript::ScriptExecution const& _execution) const
{
	bool result = false;

	if (! tagged.is_empty())
	{
		if (TeaForGodEmperor::GameDefinition::check_chosen(tagged))
		{
			result = true;
		}
	}

	return result;
}
