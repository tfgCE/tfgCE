#include "gse_mainEquipment.h"

#include "..\..\game\game.h"
#include "..\..\modules\gameplay\moduleEquipment.h"
#include "..\..\modules\gameplay\modulePilgrim.h"

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

bool MainEquipment::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	if (_node->has_attribute(TXT("hand")))
	{
		hand = Hand::parse(_node->get_string_attribute(TXT("hand")));
	}

	setEnergyTotal.load_from_xml(_node, TXT("setEnergyTotal"));
	if (auto* a = _node->get_attribute(TXT("hand")))
	{
		String hText = a->get_as_string();
		hand = Hand::parse(hText);
	}
	setStorage.load_from_xml(_node, TXT("setStorage"));
	setCoreDamaged.load_from_xml(_node, TXT("setCoreDamaged"));
	dropStorageBy.load_from_xml(_node, TXT("dropStorageBy"));
	blockReleasing.load_from_xml(_node, TXT("blockReleasing"));

	if (auto* attr = _node->get_attribute(TXT("action")))
	{
		if (attr->get_as_string() == TXT("release"))
		{
			actionRelease = true;
		}
		else
		{
			error_loading_xml(_node, TXT("did not recognize weapon request"));
			result = false;
		}
	}

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type MainEquipment::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	bool okToContinue = true;

	if (auto* game = Game::get_as<Game>())
	{
		if (auto* actor = game->access_player().get_actor())
		{
			if (auto* pilgrim = actor->get_gameplay_as<ModulePilgrim>())
			{
				if (! hand.is_set())
				{
					execute_for_hand(pilgrim, Hand::Left);
					execute_for_hand(pilgrim, Hand::Right);
				}
				else
				{
					execute_for_hand(pilgrim, hand.get());
				}
				{
					if (dropStorageBy.is_set())
					{
						pilgrim->ex__drop_energy_by(hand, dropStorageBy.get().get_random());
					}
					if (setStorage.is_set())
					{
						pilgrim->ex__set_energy(hand, setStorage.get());
					}
				}
			}
		}
	}

	if (okToContinue)
	{
		return Framework::GameScript::ScriptExecutionResult::Continue;
	}
	else
	{
		return Framework::GameScript::ScriptExecutionResult::Repeat;
	}
}

void MainEquipment::execute_for_hand(ModulePilgrim* pilgrim, Hand::Type _hand) const
{
	an_assert(_hand == Hand::Left || _hand == Hand::Right);

	if (setEnergyTotal.is_set())
	{
		pilgrim->set_hand_energy_storage(_hand, pilgrim->get_hand_energy_max_storage(_hand));
	}
	if (blockReleasing.is_set())
	{
		pilgrim->ex__block_releasing_equipment(_hand, blockReleasing.get());
	}
	if (actionRelease)
	{
		pilgrim->ex__force_release(_hand);
	}
	if (setCoreDamaged.is_set())
	{
		if (auto* me = pilgrim->get_main_equipment(_hand))
		{
			if (auto* gun = me->get_gameplay_as<ModuleEquipments::Gun>())
			{
				for_every(part, gun->access_weapon_setup().get_parts())
				{
					if (auto* p = part->part.get())
					{
						if (auto* wpt = p->get_weapon_part_type())
						{
							if (wpt->get_type() == WeaponPartType::GunCore)
							{
								p->ex__set_damaged(setCoreDamaged.get());
							}
						}
					}
				}
				gun->ex__update_stats();
				pilgrim->update_pilgrim_setup_for(me, true); // just to store state of the gun
			}
		}
	}
}
