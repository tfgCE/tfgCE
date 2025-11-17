#include "gsc_pilgrimOverlayInfo.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\game\game.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\pilgrim\pilgrimOverlayInfo.h"

#include "..\..\..\framework\object\actor.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Conditions;

//

bool Conditions::PilgrimOverlayInfo::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	mapVisible.load_from_xml(_node, TXT("mapVisible"));
	
	speaking.load_from_xml(_node, TXT("speaking"));

	return result;
}

bool Conditions::PilgrimOverlayInfo::check(Framework::GameScript::ScriptExecution const & _execution) const
{
	bool result = false;

	if (auto* g = Game::get_as<Game>())
	{
		todo_multiplayer_issue(TXT("we assume sp"));
		if (auto* pa = g->access_player().get_actor())
		{
			if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
			{
				auto& pio = mp->access_overlay_info();

				if (mapVisible.is_set())
				{
					if (pio.is_trying_to_show_map() == mapVisible.get())
					{
						return true;
					}
				}
				if (speaking.is_set())
				{
					if (pio.is_speaking_or_pending() == speaking.get())
					{
						return true;
					}
				}
			}
		}
	}

	return result;
}
