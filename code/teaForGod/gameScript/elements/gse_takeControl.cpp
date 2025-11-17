#include "gse_takeControl.h"

#include "..\..\game\game.h"

#include "..\..\..\framework\object\actor.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool TakeControl::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	dropControls = _node->get_bool_attribute_or_from_child_presence(TXT("dropControls"), dropControls);
	controlsOnly = _node->get_bool_attribute_or_from_child_presence(TXT("controlsOnly"), controlsOnly);
	immediate = _node->get_bool_attribute_or_from_child_presence(TXT("immediate"), immediate);
	appearReversedOut = _node->get_bool_attribute_or_from_child_presence(TXT("appearReversed"), appearReversedOut);
	appearReversedIn = _node->get_bool_attribute_or_from_child_presence(TXT("appearReversed"), appearReversedIn);
	appearReversedOut = _node->get_bool_attribute_or_from_child_presence(TXT("appearReversedOut"), appearReversedOut);
	appearReversedIn = _node->get_bool_attribute_or_from_child_presence(TXT("appearReversedIn"), appearReversedIn);
	blendTime.load_from_xml(_node, TXT("blendTime"));
	controlBoth.load_from_xml(_node, TXT("controlBoth"));
	seeThroughEyesOfControlled.load_from_xml(_node, TXT("seeThroughEyesOfControlled"));

	takeControlOverObjectVar.load_from_xml(_node, TXT("objectVar"));

	return result;
}

bool TakeControl::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type TakeControl::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	Framework::GameScript::ScriptExecutionResult::Type result = Framework::GameScript::ScriptExecutionResult::Continue;
	if (auto* g = Game::get_as<Game>())
	{
		while (true)
		{
			if (dropControls)
			{
				if (immediate)
				{
					g->access_player_taken_control().unbind_actor();
					result = Framework::GameScript::ScriptExecutionResult::Continue;
					break;
				}
				if (is_flag_set(_flags, Framework::GameScript::ScriptExecution::Flag::Entered) && !g->access_player_taken_control().get_actor())
				{
					// no need to drop controls
					if (g->get_taking_controls_at() >= 0.0f &&
						g->get_taking_controls_target() == 0.0f)
					{
						result = Framework::GameScript::ScriptExecutionResult::Continue;
						break;
					}
					else
					{
						result = Framework::GameScript::ScriptExecutionResult::Repeat;
						break;
					}
				}
				else
				{
					g->set_taking_controls_target(appearReversedOut? 1.0f : -1.0f, blendTime);
					if ((appearReversedOut && g->get_taking_controls_at() >= 1.0f) ||
						(!appearReversedOut && g->get_taking_controls_at() <= -1.0f))
					{
						g->set_taking_controls_at(appearReversedIn ? -1.0f : 1.0f, 0.0f);
						g->access_player_taken_control().unbind_actor();
						result = Framework::GameScript::ScriptExecutionResult::Continue;
						break;
					}
					else
					{
						// wait to reach target
						result = Framework::GameScript::ScriptExecutionResult::Repeat;
						break;
					}
				}
			}
			if (takeControlOverObjectVar.is_set())
			{
				if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(takeControlOverObjectVar.get()))
				{
					if (auto* actor = fast_cast<Framework::Actor>(exPtr->get()))
					{
						if (controlsOnly)
						{
							g->access_player_taken_control().bind_actor(actor);
							g->access_player_taken_control().set_allow_controlling_both(true, seeThroughEyesOfControlled.get(false)); // has to be done post bind
							// as we don't switch visuals, 
							result = Framework::GameScript::ScriptExecutionResult::Continue;
							break;
						}
						else if (immediate)
						{
							g->access_player_taken_control().bind_actor(actor);
							result = Framework::GameScript::ScriptExecutionResult::Continue;
							break;
						}
						else
						{
							g->set_taking_controls_target(appearReversedOut? -1.0f : 1.0f, blendTime);
							if ((appearReversedOut && g->get_taking_controls_at() <= -1.0f) ||
								(!appearReversedOut && g->get_taking_controls_at() >= 1.0f))
							{
								g->set_taking_controls_at(appearReversedIn? 1.0f : -1.0f, 0.0f);
								g->access_player_taken_control().bind_actor(actor);
								result = Framework::GameScript::ScriptExecutionResult::Continue;
								break;
							}
							else
							{
								// wait to reach target
								result = Framework::GameScript::ScriptExecutionResult::Repeat;
								break;
							}
						}
					}
					else
					{
						error(TXT("no actor to take control"));
					}
				}
				else
				{
					error(TXT("no var to get actor to take control"));
				}
				g->set_taking_controls_target(0.0f, blendTime);
				result = Framework::GameScript::ScriptExecutionResult::Continue;
				break;
			}

			break;
		}

		if (controlBoth.is_set())
		{
			g->access_player_taken_control().set_allow_controlling_both(controlBoth.get(), seeThroughEyesOfControlled.get(false));
		}
	}
	return result;
}
