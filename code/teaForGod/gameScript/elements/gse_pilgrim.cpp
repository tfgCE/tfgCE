#include "gse_pilgrim.h"

#include "..\..\game\game.h"
#include "..\..\modules\gameplay\modulePilgrim.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\..\framework\module\modulePresence.h"
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

bool Pilgrim::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	storeInRoomVar = _node->get_name_attribute_or_from_child(TXT("storeInRoomVar"), storeInRoomVar);

	storeLastSetup.load_from_xml(_node, TXT("storeLastSetup"));
	clearExtraEXMsInInventory.load_from_xml(_node, TXT("clearExtraEXMsInInventory"));
	updateEXMsInInventoryFromPersistence.load_from_xml(_node, TXT("updateEXMsInInventoryFromPersistence"));

	setHandsPoseIdle.load_from_xml(_node, TXT("setHandsPoseIdle"));
	showWeaponsBlocked.load_from_xml(_node, TXT("showWeaponsBlocked"));

	if (auto* attr = _node->get_attribute(TXT("loseHand")))
	{
		Hand::Type hand = Hand::parse(attr->get_as_string());
		loseHand = hand;
	}

	copyEXMsToObjectVar.load_from_xml(_node, TXT("copyEXMsToObjectVar"));

	return result;
}

bool Pilgrim::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type Pilgrim::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	Framework::GameScript::ScriptExecutionResult::Type result = Framework::GameScript::ScriptExecutionResult::Continue;

	if (auto* g = Game::get_as<Game>())
	{
		todo_multiplayer_issue(TXT("we assume it's sp"));
		if (auto* pa = g->access_player().get_actor())
		{
			if (auto* r = pa->get_presence()->get_in_room())
			{
				if (storeInRoomVar.is_valid())
				{
					_execution.access_variables().access< SafePtr<Framework::Room> >(storeInRoomVar) = r;
				}
				if (auto* p = pa->get_gameplay_as<ModulePilgrim>())
				{
					if (storeLastSetup.get(false))
					{
						if (auto* persistence = Persistence::access_current_if_exists())
						{
							persistence->store_last_setup(p);
						}
					}
					if (clearExtraEXMsInInventory.get(false))
					{
						SafePtr<Framework::IModulesOwner> keepPilgrim;
						keepPilgrim = pa;
						g->add_immediate_sync_world_job(TXT("gse pilgrim"), [keepPilgrim]()
							{
								if (auto* pa = keepPilgrim.get())
								{
									if (auto* p = pa->get_gameplay_as<ModulePilgrim>())
									{
										p->clear_extra_exms_in_inventory();
									}
								}
							});
						result = Framework::GameScript::ScriptExecutionResult::Yield; // we'll be back after sync job
					}
					if (updateEXMsInInventoryFromPersistence.get(false))
					{
						SafePtr<Framework::IModulesOwner> keepPilgrim;
						keepPilgrim = pa;
						g->add_immediate_sync_world_job(TXT("gse pilgrim"), [keepPilgrim]()
							{
								if (auto* pa = keepPilgrim.get())
								{
									if (auto* p = pa->get_gameplay_as<ModulePilgrim>())
									{
										p->update_exms_in_inventory();
									}
								}
							});
						result = Framework::GameScript::ScriptExecutionResult::Yield; // we'll be back after sync job
					}
					if (setHandsPoseIdle.is_set())
					{
						p->idle_pose_hand(Hand::Left, setHandsPoseIdle.get());
						p->idle_pose_hand(Hand::Right, setHandsPoseIdle.get());
					}
					if (showWeaponsBlocked.get(false))
					{
						p->show_weapons_blocked_overlay_message();
					}
					if (loseHand.is_set())
					{
						p->lose_hand(loseHand.get()); // will udpate pilgrim setup as well
					}

					if (copyEXMsToObjectVar.is_set())
					{
						if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(copyEXMsToObjectVar.get()))
						{
							if (auto* toIMO = exPtr->get())
							{
								if (auto* toP = toIMO->get_gameplay_as<ModulePilgrim>())
								{
									SafePtr<Framework::IModulesOwner> srcIMO;
									SafePtr<Framework::IModulesOwner> destIMO;
									srcIMO = pa;
									destIMO = toIMO;
									g->add_immediate_sync_world_job(TXT("copy exms"), [srcIMO, destIMO]()
										{
											if (srcIMO.get() &&
												destIMO.get())
											{
												auto* srcP = srcIMO->get_gameplay_as<ModulePilgrim>();
												auto* destP = destIMO->get_gameplay_as<ModulePilgrim>();
												if (srcP && destP)
												{
													// inventory to make all exms available
													PilgrimInventory::sync_copy_exms(srcP->get_pilgrim_inventory(), destP->access_pilgrim_inventory());

													// now reinstall exms
													{
														PilgrimSetup setup(&Persistence::access_current());
														setup.copy_exms_from(srcP->get_pending_pilgrim_setup());
														destP->set_pending_pilgrim_setup(setup);
													}
													destP->sync_recreate_exms();

													// permanent upgrades should get activated and energy reset to full
													destP->update_blackboard();
													destP->reset_energy_state();
												}
											}

										});

								}
							}
						}
					}
				}
			}
		}
	}

	return result;
}
