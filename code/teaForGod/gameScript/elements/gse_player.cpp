#include "gse_player.h"

#include "..\..\game\game.h"

#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\object\actor.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool Elements::Player::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	bindToObjectVar.load_from_xml(_node, TXT("bindToObjectVar"));
	findVRAnchor = _node->get_bool_attribute_or_from_child_presence(TXT("findVRAnchor"), findVRAnchor);
	keepVRAnchor = _node->get_bool_attribute_or_from_child_presence(TXT("keepVRAnchor"), keepVRAnchor);
	startVRAnchorOSLock = _node->get_bool_attribute_or_from_child_presence(TXT("startVRAnchorOSLock"), startVRAnchorOSLock);
	endVRAnchorOSLock = _node->get_bool_attribute_or_from_child_presence(TXT("endVRAnchorOSLock"), endVRAnchorOSLock);

	storeObjectVar.load_from_xml(_node, TXT("storeObjectVar"));

	considerPlayerObjectVar.load_from_xml(_node, TXT("considerPlayerObjectVar"));
	considerNotPlayerObjectVar.load_from_xml(_node, TXT("considerNotPlayerObjectVar"));

	return result;
}

bool Elements::Player::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type Elements::Player::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* g = Game::get_as<Game>())
	{
		if (bindToObjectVar.is_set())
		{
			if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(bindToObjectVar.get()))
			{
				if (auto* actor = fast_cast<Framework::Actor>(exPtr->get()))
				{
					g->access_player().bind_actor(actor, Framework::Player::BindActorParams().keep_vr_anchor(keepVRAnchor).find_vr_anchor(findVRAnchor));
				}
			}
		}
		if (startVRAnchorOSLock)
		{
			g->access_player().start_vr_anchor_os_lock();
		}
		if (endVRAnchorOSLock)
		{
			g->access_player().end_vr_anchor_os_lock();
		}
		if (storeObjectVar.is_set())
		{
			auto& storeObjectPtr = _execution.access_variables().access<SafePtr<Framework::IModulesOwner>>(storeObjectVar.get());
			storeObjectPtr = g->access_player().get_actor();
		}
		if (considerPlayerObjectVar.is_set())
		{
			if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(considerPlayerObjectVar.get()))
			{
				if (auto* imo = exPtr->get())
				{
					if (auto* ai = imo->get_ai())
					{
						ai->set_consider_player(true);
					}
				}
			}
		}
		if (considerNotPlayerObjectVar.is_set())
		{
			if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(considerNotPlayerObjectVar.get()))
			{
				if (auto* imo = exPtr->get())
				{
					if (auto* ai = imo->get_ai())
					{
						ai->set_consider_player(false);
					}
				}
			}
		}
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
