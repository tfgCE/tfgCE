#include "gameScript.h"

#include "gameScriptElement.h"
#include "registeredGameScriptElements.h"

#include "elements\gse_trap.h"
#include "elements\gse_trapSub.h"

#include "..\..\core\io\xml.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;

//

REGISTER_FOR_FAST_CAST(Script);
LIBRARY_STORED_DEFINE_TYPE(Script, gameScript);

Script::Script(Framework::Library* _library, Framework::LibraryName const& _name)
: base(_library, _name)
{
}

Script::~Script()
{
}

void Script::load_elements_on_demand_if_required()
{
	if (elementsLoadedOnDemand)
	{
		return;
	}
	elementsLoadedOnDemand = true;
	for_every_ref(e, elements)
	{
		e->load_on_demand_if_required();
	}
}


bool Script::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	elements.clear();
	executionTraps.clear();
	for_every(node, _node->all_children())
	{
		if (ScriptElement* e = RegisteredScriptElements::create(node->get_name().to_char()))
		{
			if (e->load_from_xml(node, _lc))
			{
				elements.push_back(RefCountObjectPtr<ScriptElement>(e));
				if (auto* t = fast_cast<Elements::Trap>(e))
				{
					executionTraps.push_back(ExecutionTrap(t->get_name(), elements.get_size() - 1));
				}
				if (auto* t = fast_cast<Elements::TrapSub>(e))
				{
					executionTraps.push_back(ExecutionTrap(t->get_name(), elements.get_size() - 1, true));
				}
			}
			else
			{
				result = false;
			}
		}
		else
		{
			error_loading_xml(node, TXT("not recognised game script element"));
			result = false;
		}
	}

	return result;
}

bool Script::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every_ref(e, elements)
	{
		result &= e->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

void Script::prepare_to_unload()
{
	base::prepare_to_unload();
	elements.clear();
	executionTraps.clear();
}

int Script::get_execution_trap(ScriptExecution & _execution, Name const& _trapName, OUT_ OPTIONAL_ bool* _sub) const
{
	for_every(t, executionTraps)
	{
		if (t->trapName == _trapName)
		{
			if (elements.is_index_valid(t->at))
			{
				if (auto* trap = fast_cast<Elements::Trap>(elements[t->at].get()))
				{
					if (!trap->check_condition(_execution))
					{
						// skip
						continue;
					}
				}
				assign_optional_out_param(_sub, t->sub);
				return t->at;
			}
		}
	}

	return NONE;
}

void Script::log(LogInfoContext& _log) const
{
	_log.log(TXT("script \"%S\""), get_name().to_string().to_char());
	{
		LOG_INDENT(_log);
		for_every_ref(e, elements)
		{
			_log.log(TXT("%03i > %S"), for_everys_index(e), e->get_debug_info());
		}
	}
}
