#include "gse_hitIndicator.h"

#include "..\..\game\game.h"
#include "..\..\modules\custom\mc_hitIndicator.h"
#include "..\..\modules\gameplay\modulePilgrim.h"

#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool HitIndicator::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	if (auto* attr = _node->get_attribute(TXT("atHand")))
	{
		atHand = Hand::parse(attr->get_value());
	}

	relAngle.load_from_xml(_node, TXT("relAngle"));
	relVerticalAngle.load_from_xml(_node, TXT("relVerticalAngle"));

	colour.load_from_xml(_node, TXT("colour"));
	delay.load_from_xml(_node, TXT("delay"));
	
	damage.load_from_xml(_node, TXT("damage"));
	reversed.load_from_xml(_node, TXT("reversed"));

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type HitIndicator::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* game = Game::get_as<Game>())
	{
		if (auto* actor = game->access_player().get_actor())
		{
			if (auto* hi = actor->get_custom<CustomModules::HitIndicator>())
			{
				CustomModules::HitIndicator::IndicateParams ip;
				ip.with_colour_override(colour);
				ip.with_delay(delay);
				ip.is_damage(damage.get(!colour.is_set())); // if colour is set, most likely it is not a standard damage
				ip.is_reversed(reversed.get(false));
				bool handled = false;
				if (atHand.is_set())
				{
					if (auto* mp = actor->get_gameplay_as<ModulePilgrim>())
					{
						Vector3 locOs = mp->get_hand_relative_placement_os(atHand.get(), true).get_translation();
						hi->indicate_relative_location(0.25f, locOs, ip);
						handled = true;
					}
				}
				if (!handled)
				{
					hi->indicate_relative_dir(0.25f, Vector3(0.0f, 1.0f, 0.0f).rotated_by_pitch(Random::get(relVerticalAngle)).rotated_by_yaw(Random::get(relAngle)), ip);
				}
			}
		}
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
