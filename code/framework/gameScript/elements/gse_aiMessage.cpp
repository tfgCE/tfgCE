#include "gse_aiMessage.h"

#include "..\gameScript.h"

#include "..\..\game\game.h"
#include "..\..\library\library.h"
#include "..\..\module\moduleAI.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\ai\aiMessage.h"
#include "..\..\library\usedLibraryStored.inl"
#include "..\..\module\modulePresence.h"
#include "..\..\world\room.h"
#include "..\..\world\world.h"

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(self);

//

bool AIMessage::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"), name);

	toObjectVar = _node->get_name_attribute(TXT("toObjectVar"), toObjectVar);
	toObjectVarsRoom = _node->get_name_attribute(TXT("toObjectVarsRoom"), toObjectVarsRoom);
	toRoomVar = _node->get_name_attribute(TXT("toRoomVar"), toRoomVar);
	toAll = _node->get_bool_attribute_or_from_child_presence(TXT("toAll"), toAll);
	mayHaveNoRecipent = _node->get_bool_attribute_or_from_child_presence(TXT("mayHaveNoRecipent"), mayHaveNoRecipent);

	if (!toObjectVar.is_valid() &&
		!toObjectVarsRoom.is_valid() &&
		!toRoomVar.is_valid() &&
		!toAll)
	{
		toObjectVar = NAME(self);
	}

	result &= params.load_from_xml_child_node(_node, TXT("params"));
	_lc.load_group_into(params);

	for_every(node, _node->children_named(TXT("copyParam")))
	{
		Name from;
		from = node->get_name_attribute_or_from_child(TXT("param"), from);
		from = node->get_name_attribute_or_from_child(TXT("from"), from);
		Name to = from;
		to = node->get_name_attribute_or_from_child(TXT("to"), to);
		to = node->get_name_attribute_or_from_child(TXT("as"), to);
		if (from.is_valid() && to.is_valid())
		{
			copyParams.push_back({ from, to });
		}
		else
		{
			error_loading_xml(node, TXT("missing from-to"));
			result = false;
		}
	}
	return result;
}

bool AIMessage::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

ScriptExecutionResult::Type AIMessage::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (auto* game = Game::get_as<Game>())
	{
		if (auto* world = game->get_world())
		{
			if (Framework::AI::Message* message = world->create_ai_message(name))
			{
				message->set_params_from(params);
				for_every(cp, copyParams)
				{
					TypeId ofType;
					void const* rawValue;
					if (_execution.get_variables().get_existing_of_any_type_id(cp->from, ofType, rawValue))
					{
						auto& param = message->access_param(cp->to);
						param.set(ofType, rawValue);
					}
				}

#ifdef AN_DEVELOPMENT_OR_PROFILER
				bool hasRecipient = false;
#endif
				if (toObjectVar.is_valid())
				{
					if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(toObjectVar))
					{
						if (auto* imo = exPtr->get())
						{
							message->to_ai_object(imo);
#ifdef AN_DEVELOPMENT_OR_PROFILER
							hasRecipient = true;
#endif
						}
					}
				}
				if (toObjectVarsRoom.is_valid())
				{
					if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(toObjectVarsRoom))
					{
						if (auto* imo = exPtr->get())
						{
							if (auto* p = imo->get_presence())
							{
								message->to_room(p->get_in_room());
#ifdef AN_DEVELOPMENT_OR_PROFILER
								hasRecipient = true;
#endif
							}
						}
					}
				}
				if (toRoomVar.is_valid())
				{
					if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(toRoomVar))
					{
						if (auto* room = exPtr->get())
						{
							message->to_room(room);
#ifdef AN_DEVELOPMENT_OR_PROFILER
							hasRecipient = true;
#endif
						}
					}
				}
				if (toAll)
				{
					message->to_world(world);
#ifdef AN_DEVELOPMENT_OR_PROFILER
					hasRecipient = true;
#endif
				}

#ifdef AN_DEVELOPMENT_OR_PROFILER
				an_assert(mayHaveNoRecipent || hasRecipient);
#endif
			}
		}
	}

	return ScriptExecutionResult::Continue;
}
