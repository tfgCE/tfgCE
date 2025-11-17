#include "gse_globalTint.h"

#include "..\..\game\game.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool GlobalTint::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	for_every(n, _node->children_named(TXT("set")))
	{
		active.load_from_xml(n, TXT("active"));
		current.load_from_xml(n);
	}
	for_every(n, _node->children_named(TXT("target")))
	{
		targetActive.load_from_xml(n, TXT("active"));
		blendTime.load_from_xml(n, TXT("blendTime"));
		targetColour.load_from_xml(n);
	}
	blendTime.load_from_xml(_node, TXT("blendTime"));

	clear = _node->get_bool_attribute_or_from_child_presence(TXT("clear"), clear);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type GlobalTint::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* game = Game::get_as<Game>())
	{
		if (clear)
		{
			game->clear_scripted_tint();
		}
		else
		{
			if (active.is_set() || current.is_set())
			{
				game->set_scripted_tint_at(current.get(game->get_scripted_tint_current()), active.get(game->get_scripted_tint_active()));
			}
			if (targetActive.is_set() || targetColour.is_set() || blendTime.is_set())
			{
				game->set_scripted_tint_target(targetColour.get(game->get_scripted_tint_target()), targetActive.get(game->get_scripted_tint_target_active()), blendTime.get(game->get_scripted_tint_blend_time()));
			}
		}
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
