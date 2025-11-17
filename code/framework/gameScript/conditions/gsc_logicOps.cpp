#include "gsc_logicOps.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\..\core\io\xml.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Conditions;

//

bool Not::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	if (auto* node = _node->first_child())
	{
		RefCountObjectPtr<ScriptCondition> sc;
		sc = RegisteredScriptConditions::create(node->get_name().to_char());
		if (sc.get())
		{
			if (sc->load_from_xml(node, _lc))
			{
				subCondition = sc;
			}
		}
	}
	else
	{
		error_loading_xml(node, TXT("no sub condition"))
		result = false;
	}
	return result;
}

bool Not::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	if (subCondition.is_set())
	{
		result &= subCondition->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

bool Not::check(ScriptExecution const & _execution) const
{
	bool result = true;

	if (subCondition.is_set())
	{
		result = !subCondition->check(_execution);
	}
	else
	{
		result = false;
	}

	return result;
}

//

bool LogicOp2::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	for_every(node, _node->all_children())
	{
		RefCountObjectPtr<ScriptCondition> sc;
		sc = RegisteredScriptConditions::create(node->get_name().to_char());
		if (sc.get())
		{
			if (sc->load_from_xml(node, _lc))
			{
				subConditions.push_back(sc);
			}
		}
	}

	return result;
}

bool LogicOp2::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every_ref(sc, subConditions)
	{
		result &= sc->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

//

bool Or::check(ScriptExecution const & _execution) const
{
	bool result = false;

	for_every_ref(sc, subConditions)
	{
		if (sc->check(_execution))
		{
			result = true;
			break;
		}
	}

	return result;
}

//

bool And::check(ScriptExecution const & _execution) const
{
	bool result = true;

	for_every_ref(sc, subConditions)
	{
		if (! sc->check(_execution))
		{
			result = false;
			break;
		}
	}

	return result;
}
