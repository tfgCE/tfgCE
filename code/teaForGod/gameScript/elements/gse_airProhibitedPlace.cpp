#include "gse_airProhibitedPlace.h"

#include "..\..\game\game.h"
#include "..\..\game\gameDirector.h"
#include "..\..\story\storyExecution.h"

#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool Elements::AirProhibitedPlace::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	clearAll.load_from_xml(_node, TXT("clearAll"));
	clear.load_from_xml(_node, TXT("clear"));
	add.load_from_xml(_node, TXT("add"));

	a.load_from_xml(_node, TXT("a"));
	a.load_from_xml(_node, TXT("at"));
	a.load_from_xml(_node, TXT("from"));
	b.load_from_xml(_node, TXT("b"));
	b.load_from_xml(_node, TXT("to"));
	radius.load_from_xml(_node, TXT("radius"));
	
	relativeToPlayersTileLoc.load_from_xml(_node, TXT("relativeToPlayersTileLoc"));
	if (_node->first_child_named(TXT("relativeToPlayersTileLoc")))
	{
		relativeToPlayersTileLoc = _node->get_bool_attribute_or_from_child_presence(TXT("relativeToPlayersTileLoc"), relativeToPlayersTileLoc.get(false));
	}

	return result;
}

bool Elements::AirProhibitedPlace::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}


Framework::GameScript::ScriptExecutionResult::Type Elements::AirProhibitedPlace::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* gd = TeaForGodEmperor::GameDirector::get_active())
	{
		if (clearAll.is_set() && clearAll.get())
		{
			gd->remove_all_air_prohibited_places();
		}
		if (clear.is_set())
		{
			gd->remove_air_prohibited_place(clear.get());
		}
		if (add.is_set())
		{
			if (a.is_set())
			{
				gd->add_air_prohibited_place(add.get(), Segment(a.get(), b.get(a.get())), radius.get(60.0f), relativeToPlayersTileLoc.get(false));
			}
			else
			{
				gd->add_air_prohibited_place(add.get(), Segment(Vector3::zero, Vector3::zero), radius.get(60.0f), relativeToPlayersTileLoc.get(true));
			}
		}
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
