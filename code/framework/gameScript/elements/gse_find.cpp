#include "gse_find.h"

#include "gse_label.h"

#include "..\gameScript.h"

#include "..\..\game\game.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\library\library.h"
#include "..\..\module\modulePresence.h"
#include "..\..\object\actor.h"
#include "..\..\world\door.h"
#include "..\..\world\room.h"
#include "..\..\world\world.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

bool Find::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	tagged.load_from_xml_attribute_or_child_node(_node, TXT("tagged"));
	roomTagged.load_from_xml_attribute_or_child_node(_node, TXT("roomTagged"));
	doorTagged.load_from_xml_attribute_or_child_node(_node, TXT("doorTagged"));

	inRoomVar = _node->get_name_attribute_or_from_child(TXT("inRoomVar"), inRoomVar);
	
	attachedToVar = _node->get_name_attribute_or_from_child(TXT("attachedToVar"), attachedToVar);

	inRange.load_from_xml_child_node(_node, TXT("inRange"));

	//error_loading_xml_on_assert(!tagged.is_empty() || !roomTagged.is_empty() || !doorTagged.is_empty() || !lockerRoom, result, _node, TXT("no \"tagged\", \"roomTagged\" nor \"doorTagged\" defined, no \"lockerRoom\" neither"));

	storeVar = _node->get_name_attribute_or_from_child(TXT("storeVar"), storeVar);
	
	goToLabelOnFail = _node->get_name_attribute_or_from_child(TXT("goToLabelOnFail"), goToLabelOnFail);
	if (goToLabelOnFail.is_valid())
	{
		mayFail = true;
		storeOnlyIfFound = true;
	}

	storeOnlyIfFound = _node->get_bool_attribute_or_from_child_presence(TXT("storeOnlyIfFound"), storeOnlyIfFound);
	mayFail = _node->get_bool_attribute_or_from_child_presence(TXT("mayFail"), mayFail);

	chooseRandomly = _node->get_bool_attribute_or_from_child_presence(TXT("chooseRandomly"), chooseRandomly);

	return result;
}

bool Find::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

World* Find::get_world(ScriptExecution& _execution) const
{
	return nullptr;
}

void Find::find_object(ScriptExecution& _execution, Results& _results) const
{
	if (tagged.is_empty())
	{
		return;
	}

#ifdef INVESTIGATE_GAME_SCRIPT
	output(TXT("look for tagged \"%S\""), tagged.to_string().to_char());
#endif

	auto* r = get_room_for_find_object(_execution);
	if (r)
	{
		//FOR_EVERY_OBJECT_IN_ROOM(obj, r)
		for_every_ptr(obj, r->get_objects())
		{
			if (tagged.check(obj->get_tags())
				&& check_conditions_for(_execution, obj))
			{
				if (inRange.is_empty() ||
					inRange.does_contain(obj->get_presence()->get_placement().get_translation()))
				{
					if (chooseRandomly)
					{
						_results.allFound.push_back(SafePtr<Framework::IModulesOwner>(obj));
					}
					else
					{
						_results.found = obj;
						break;
					}
				}
			}
		}
	}
	else if (auto* world = get_world(_execution))
	{
		for_every_ptr(obj, world->get_all_objects())
		{
			if (tagged.check(obj->get_tags())
				&& check_conditions_for(_execution, obj))
			{
				if (inRange.is_empty() ||
					inRange.does_contain(obj->get_presence()->get_placement().get_translation()))
				{
					if (chooseRandomly)
					{
						_results.allFound.push_back(SafePtr<Framework::IModulesOwner>(obj));
					}
					else
					{
						_results.found = obj;
						break;
					}
				}
			}
		}
	}
}

Room* Find::get_room_for_find_object(ScriptExecution& _execution) const
{
	if (inRoomVar.is_valid())
	{
#ifdef INVESTIGATE_GAME_SCRIPT
		output(TXT("in room var"));
#endif
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(inRoomVar))
		{
			if (auto* r = exPtr->get())
			{
				return r;
			}
		}
	}

	return nullptr;
}

void Find::find_room(ScriptExecution& _execution, OUT_ Results& _results) const
{
	if (!roomTagged.is_empty())
	{
		if (auto* world = get_world(_execution))
		{
			for_every_ptr(r, world->get_all_rooms())
			{
				if (roomTagged.check(r->get_tags()))
				{
					_results.foundRoom = r;
					break;
				}
			}
		}
	}
}

void Find::find_door(ScriptExecution& _execution, OUT_ Results& _results) const
{
	if (!doorTagged.is_empty())
	{
		if (inRoomVar.is_valid())
		{
			if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(inRoomVar))
			{
				if (auto* r = exPtr->get())
				{
					for_every_ptr(dir, r->get_all_doors())
					{
						todo_note(TXT("invisible are used here but they're a bit iffy"));
						if (auto* d = dir->get_door())
						{
							if (doorTagged.check(dir->get_tags()) ||
								doorTagged.check(d->get_tags()))
							{
								_results.foundDoor = d;
								break;
							}
						}
					}
				}
			}
		}
		else if (auto* world = get_world(_execution))
		{ 
			for_every_ptr(r, world->get_all_rooms())
			{
				for_every_ptr(dir, r->get_all_doors())
				{
					todo_note(TXT("invisible are used here but they're a bit iffy"));
					if (auto* d = dir->get_door())
					{
						if (doorTagged.check(dir->get_tags()) ||
							doorTagged.check(d->get_tags()))
						{
							_results.foundDoor = d;
							break;
						}
					}
				}
			}
		}
	}
}

ScriptExecutionResult::Type Find::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	Results results;

	find_object(_execution, OUT_ results);
	
	if (!results.allFound.is_empty())
	{
		results.found = results.allFound[Random::get_int(results.allFound.get_size())];
	}

	//

	find_room(_execution, OUT_ results);

	//

	find_door(_execution, OUT_ results);

	//

	if (!storeOnlyIfFound || results.found.get())
	{
		if (storeVar.is_valid() && !tagged.is_empty())
		{
			_execution.access_variables().access< SafePtr<Framework::IModulesOwner> >(storeVar) = results.found;
		}
	}
	if (!storeOnlyIfFound || results.foundRoom.get())
	{
		if (storeVar.is_valid() && !roomTagged.is_empty())
		{
			_execution.access_variables().access< SafePtr<Framework::Room> >(storeVar) = results.foundRoom;
		}
	}
	if (!storeOnlyIfFound || results.foundDoor.get())
	{
		if (storeVar.is_valid() && !doorTagged.is_empty())
		{
			_execution.access_variables().access< SafePtr<Framework::Door> >(storeVar) = results.foundDoor;
		}
	}

	if (!results.found.get() && !results.foundRoom.get() && !results.foundDoor.get())
	{
		if (!mayFail)
		{
#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (! Framework::Library::may_ignore_missing_references())
#endif
			error(TXT("[gse] find did not find anything"));
		}
		if (goToLabelOnFail.is_valid())
		{
			int labelAt = NONE;
			if (auto* script = _execution.get_script())
			{
				for_every_ref(e, script->get_elements())
				{
					if (auto* label = fast_cast<Label>(e))
					{
						if (label->get_id() == goToLabelOnFail)
						{
							labelAt = for_everys_index(e);
							break;
						}
					}
				}
			}

			if (labelAt != NONE)
			{
				_execution.set_at(labelAt);
			}
			else
			{
				error(TXT("did not find label \"%S\""), goToLabelOnFail.to_char());
			}

			return ScriptExecutionResult::SetNextInstruction;
		}
	}

	return ScriptExecutionResult::Continue;
}

bool Find::check_conditions_for(ScriptExecution& _execution, Framework::IModulesOwner* imo) const
{
	if (attachedToVar.is_valid())
	{
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(attachedToVar))
		{
			if (auto* attachetToImo = exPtr->get())
			{
				if (!imo->get_presence()->is_attached_at_all_to(attachetToImo))
				{
					return false;
				}
			}
		}
	}
	return true;
}
