#include "gse_createMainEquipment.h"

#include "..\..\game\game.h"
#include "..\..\modules\gameplay\moduleEquipment.h"
#include "..\..\modules\gameplay\modulePilgrim.h"

#include "..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

// script execution variable
DEFINE_STATIC_NAME(asyncJobDone);

//

bool CreateMainEquipment::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	if (_node->has_attribute(TXT("hand")))
	{
		hand = Hand::parse(_node->get_string_attribute(TXT("hand")));
	}
	else
	{
		error_loading_xml(_node, TXT("no \"hand\" provided"));
		result = false;
	}
	
	result &= itemType.load_from_xml(_node, TXT("itemType"), _lc);

	result &= energy.load_from_attribute_or_from_child(_node, TXT("energy"));

	return result;
}

bool CreateMainEquipment::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= itemType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type CreateMainEquipment::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (is_flag_set(_flags, Framework::GameScript::ScriptExecution::Entered))
	{
		SafePtr<Framework::GameScript::ScriptExecution> execution(&_execution);
		execution->access_variables().access<bool>(NAME(asyncJobDone)) = false;

		if (auto* game = Game::get_as<Game>())
		{
			auto handCopy = hand;
			auto itemTypeCopy = itemType;
			game->add_async_world_job(TXT("create main equipment"),
				[execution, handCopy, itemTypeCopy]()
			{
				if (auto* game = Game::get_as<Game>())
				{
					if (auto* playerActor = game->access_player().get_actor())
					{
						if (auto* pilgrim = playerActor->get_gameplay_as<ModulePilgrim>())
						{
							Framework::ScopedAutoActivationGroup saag(game->get_world());
							if (handCopy == Hand::MAX)
							{
								error(TXT("no valid hand provided"));
							}
							else
							{
								if (auto* eq = pilgrim->create_equipment_internal(handCopy, itemTypeCopy.get()))
								{
									if (auto* meq = eq->get_gameplay_as<ModuleEquipment>())
									{
										todo_important(TXT("was removed, reimplement if required"));
										//meq->reset_energy_state(energyCopy);
									}
									eq->set_allow_rendering_since_core_time(::System::Core::get_timer_as_double() + 0.3); // time required to move arms in place
								}
							}
						}
					}
				}
				if (auto* ex = execution.get())
				{
					ex->access_variables().access<bool>(NAME(asyncJobDone)) = true;
				}
			});
		}
	}

	if (_execution.get_variables().get_value<bool>(NAME(asyncJobDone), false))
	{
		return Framework::GameScript::ScriptExecutionResult::Continue;
	}
	else
	{
		return Framework::GameScript::ScriptExecutionResult::Repeat;
	}
}
