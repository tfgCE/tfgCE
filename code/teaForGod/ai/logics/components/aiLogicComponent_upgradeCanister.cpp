#include "aiLogicComponent_upgradeCanister.h"

#include "..\..\..\game\game.h"
#include "..\..\..\game\gameDirector.h"
#include "..\..\..\library\extraUpgradeOption.h"
#include "..\..\..\modules\gameplay\moduleEXM.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\..\modules\custom\mc_grabable.h"
#include "..\..\..\modules\custom\mc_hitIndicator.h"
#include "..\..\..\modules\custom\health\mc_health.h"

#include "..\..\..\..\framework\game\gameUtils.h"
#include "..\..\..\..\framework\gameScript\gameScriptExecution.h"
#include "..\..\..\..\framework\library\library.h"
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\module\moduleSound.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\..\framework\object\actor.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define DEBUG_REFILL

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// variables
DEFINE_STATIC_NAME(upgradeCanisterId);

// sockets
DEFINE_STATIC_NAME(showInfo);

// emissive layers
DEFINE_STATIC_NAME(ready);
DEFINE_STATIC_NAME(held);
DEFINE_STATIC_NAME(bad);
DEFINE_STATIC_NAME(used);

// sounds
DEFINE_STATIC_NAME(upgradeCanisterActivated);

// overlay reasons
DEFINE_STATIC_NAME(upgradeCanisterReward);
DEFINE_STATIC_NAME(upgradeCanister);

// exm variables
DEFINE_STATIC_NAME(refillUpTo);

// game script traps
DEFINE_STATIC_NAME(holdActiveUpgradeCanister);

//

UpgradeCanister::UpgradeCanister()
{
	hitIndicatorColour.parse_from_string(String(TXT("hi_activated_upgrade_canister")));
	hitIndicatorHealthColour.parse_from_string(String(TXT("hi_activated_upgrade_canister_health")));
	hitIndicatorAmmoColour.parse_from_string(String(TXT("hi_activated_upgrade_canister_ammo")));

	overlayInfoId = NAME(upgradeCanister);
}

UpgradeCanister::~UpgradeCanister()
{
	drop_showing_reward();
}

bool UpgradeCanister::has_content() const
{
	return content.exm.is_set() || content.extra.is_set();
}

void UpgradeCanister::set_active(bool _active)
{
	active = _active;
	if (!active)
	{
		update_emissive(EmissiveOff);
	}
}

Framework::IModulesOwner* UpgradeCanister::get_held_imo() const
{
	if (auto* hb = isHeldBy.get())
	{
		if (auto* mp = hb->get_gameplay_as<ModulePilgrim>())
		{
			for_every_ref(s, canisterHandler.get_switches())
			{
				if (auto* g = s->get_custom<CustomModules::Grabable>())
				{
					if (g->is_grabbed())
					{
						auto h = mp->get_helding_hand(s);
						if (h.is_set())
						{
							return s;
						}
					}
				}
			}
		}
	}
	return nullptr;
}

void UpgradeCanister::initialise(Framework::IModulesOwner* _owner, Name const& _interactiveDeviceIdVarInOwner)
{
	canisterHandler.initialise(_owner, _interactiveDeviceIdVarInOwner, NAME(upgradeCanisterId));

	if (!showInfoAt.is_set())
	{
		showInfoAt = _owner->get_presence()->get_centre_of_presence_transform_WS();
		if (auto* ap = _owner->get_appearance())
		{
			int socketIdx = ap->find_socket_index(NAME(showInfo));
			if (socketIdx != NONE)
			{
				showInfoAt = _owner->get_presence()->get_placement().to_world(ap->calculate_socket_os(socketIdx));
			}
		}
	}
}

void UpgradeCanister::advance(float _deltaTime)
{
	canisterHandler.advance(_deltaTime);

	{
		prevCanisterAt = canisterAt;
		canisterAt = canisterHandler.get_switch_at();
	}

	if (dropShowEXMsRewardIn.is_set())
	{
		dropShowEXMsRewardIn = dropShowEXMsRewardIn.get() - _deltaTime;
		if (dropShowEXMsRewardIn.get() <= 0.0f)
		{
			dropShowEXMsRewardIn.clear();
			if (auto* pa = rewardGivenTo.get())
			{
				if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
				{
					auto& poi = mp->access_overlay_info();
					poi.keep_showing_exms(NAME(upgradeCanisterReward));
				}
			}
		}
	}

	if (auto* pa = rewardGivenTo.get())
	{
		bool stillHere = false;
		{
			auto* inRoom = pa->get_presence()->get_in_room();
			for_every_ref(imo, canisterHandler.get_switches())
			{
				if (imo && imo->get_presence()->get_in_room() == inRoom)
				{
					stillHere = true;
					break;
				}
			}
		}
		if (!stillHere)
		{
			drop_showing_reward();
		}
	}

	{
		todo_multiplayer_issue(TXT("we just get player here"));
		ModulePilgrim* mp = nullptr;
		if (auto* g = Game::get_as<Game>())
		{
			if (auto* pa = g->access_player().get_actor())
			{
				mp = pa->get_gameplay_as<ModulePilgrim>();
			}
		}

		bool held = false;
		Framework::IModulesOwner* heldBy = nullptr;
		Optional<Hand::Type> heldByHand;
		{
			for_every_ref(s, canisterHandler.get_switches())
			{
				if (auto* g = s->get_custom<CustomModules::Grabable>())
				{
					if (g->is_grabbed())
					{
						if (mp)
						{
							auto h = mp->get_helding_hand(s);
							if (h.is_set())
							{
								heldByHand = h;
								held = true;
								heldBy = mp->get_owner();
								break;
							}
						}
					}
				}
			}
		}

		if (content.exm.is_set() ||
			content.extra.is_set())
		{
			if (held)
			{
				update_emissive(UpgradeCanister::EmissiveHeld);
			}
			else
			{
				update_emissive(UpgradeCanister::EmissiveReady);
			}
		}
		else
		{
			update_emissive(UpgradeCanister::EmissiveOff);
		}
#ifdef AN_ALLOW_EXTENSIVE_LOGS
		REMOVE_AS_SOON_AS_POSSIBLE_
		if (held && (isHeld ^ held))
		{
			output(TXT("grabbed upgrade chanister"));
			output(TXT("info shown    : %S"), infoShown.to_char());
			output(TXT("game director : %S"), GameDirector::get() ? TXT("YES") : TXT("no"));
			output(TXT("can show info : %S"), GameDirector::get() && GameDirector::get()->is_upgrade_canister_info_blocked() ? TXT("BLOCKED") : TXT("ok"));
			output(TXT("mp            : %S"), mp ? TXT("YES") : TXT("no"));
			output(TXT("active        : %S"), active ? TXT("YES") : TXT("no"));
		}
#endif
		if (held)
		{
			Name shouldShow;
			if (!GameDirector::get() ||
				!GameDirector::get()->is_upgrade_canister_info_blocked())
			{
				if (active)
				{
					if (content.exm.is_set())
					{
						shouldShow = content.exm->get_ui_desc_id();
					}
					if (content.extra.is_set())
					{
						shouldShow = content.extra->get_ui_desc_id();
					}
				}
			}
			if (shouldShow != infoShown)
			{
				if (mp)
				{
					auto& poi = mp->access_overlay_info();
					if (shouldShow.is_valid())
					{
						String info;
						if (content.exm.is_set())
						{
							info = content.exm->get_ui_desc();
							an_assert(!content.exm->is_integral());
							if (content.exm->is_permanent())
							{
								poi.show_permanent_exms(overlayInfoId);
							}
							else
							{
								poi.show_exms(overlayInfoId);
								poi.show_exm_proposal(heldByHand.get(), !content.exm->is_active(), content.exm->get_id());
							}
						}
						if (content.extra.is_set())
						{
							info = content.extra->get_ui_desc();
						}
#ifdef AN_ALLOW_EXTENSIVE_LOGS
						output(TXT("info to show \"%S\""), info.to_char());
#endif
						// hide whatever there is first
						poi.hide_show_info(overlayInfoId);
						if (!info.is_empty())
						{
							an_assert(showInfoAt.is_set());
							poi.show_info(overlayInfoId, showInfoAt.get(), info, NP, 0.017f, 0.6f, false);
						}
					}
					else
					{
						poi.hide_show_info(overlayInfoId);
					}
					infoShown = shouldShow;
				}
			}
		}

		if (isHeld ^ held)
		{
			if (mp)
			{
				auto& poi = mp->access_overlay_info();
				if (held)
				{
					if (active)
					{
						Framework::GameScript::ScriptExecution::trigger_execution_trap(NAME(holdActiveUpgradeCanister));
					}
				}
				else
				{
					poi.hide_show_info(overlayInfoId);
					poi.hide_exms(overlayInfoId);
					poi.hide_permanent_exms(overlayInfoId);
					// this hand can't hold anything
					if (isHeldByHand.is_set())
					{
						poi.clear_exm_proposal(isHeldByHand.get(), true);
						poi.clear_exm_proposal(isHeldByHand.get(), false);
					}
					infoShown = Name::invalid();
				}
			}
		}
		wasHeld = isHeld;
		isHeld = held;
		isHeldByHand = heldByHand;
		isHeldBy = heldBy;
	}
}

void UpgradeCanister::drop_showing_reward()
{
	if (auto* pa = rewardGivenTo.get())
	{
		if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
		{
			auto& poi = mp->access_overlay_info();
			poi.hide_exms(NAME(upgradeCanisterReward));
		}
		rewardGivenTo.clear();
		dropShowEXMsRewardIn.clear();
	}
}

bool UpgradeCanister::should_give_content(Framework::IModulesOwner* _onlyTo) const
{
	if (isHeldBy.get() && (!_onlyTo || _onlyTo == isHeldBy.get()))
	{
		return canisterAt >= 1.0f && prevCanisterAt < 1.0f && isHeldByHand.is_set();
	}
	else
	{
		return false;
	}
}

bool UpgradeCanister::give_content(Framework::IModulesOwner* _onlyTo, bool _upgradeMachineOrigin, bool _forceGiven)
{
	if (!should_give_content(_onlyTo))
	{
		return false;
	}

	bool given = _forceGiven;
	bool givenHealth = false;
	bool givenAmmo = false;
	if (auto* hb = isHeldBy.get())
	{
		if (auto* mp = hb->get_gameplay_as<ModulePilgrim>())
		{
			int hitIndicatorCount = 3;
			if (content.exm.is_set())
			{
				Name exmId = content.exm->get_id();
				Optional<Hand::Type> heldByHand = isHeldByHand;
				if (heldByHand.is_set())
				{
					mp->access_pilgrim_inventory().make_exm_available(exmId); // won't change anything if already available

					auto* exmType = EXMType::find(exmId);

					if (exmType->is_permanent())
					{
						hitIndicatorCount = 5;
					}
					if (exmType->is_integral())
					{
						hitIndicatorCount = 3;
					}
					if (auto* g = Game::get_as<Game>())
					{
						// safer to do as an immediate sync world job
						g->add_immediate_sync_world_job(TXT("change pilgrim setup"), [mp, heldByHand, exmType, exmId]()
						{
							auto & persistence = Persistence::access_current();
							PilgrimSetup setup(&persistence);
							setup.copy_from(mp->get_pending_pilgrim_setup());

							if (exmType)
							{
								auto& handSetup = setup.access_hand(heldByHand.get());
								auto& otherHandSetup = setup.access_hand(Hand::other_hand(heldByHand.get()));
								if (exmType->is_integral())
								{
									an_assert(false, TXT("we shouldn't be installing integral exms this way"));
								}
								else if (exmType->is_permanent())
								{
									mp->access_pilgrim_inventory().add_permanent_exm(exmId);
								}
								else
								{
									{
										bool simpleRules = true;
										if (auto* gd = TeaForGodEmperor::GameDefinition::get_chosen())
										{
											if (gd->get_type() == TeaForGodEmperor::GameDefinition::Type::ComplexRules)
											{
												simpleRules = false;
											}
										}
										if (simpleRules) // for simple rules
										{
											Concurrency::ScopedSpinLock lock(persistence.access_lock());
											if (!persistence.access_all_exms().does_contain(exmId))
											{
												persistence.access_all_exms().push_back(exmId);
												TeaForGodEmperor::PlayerSetup::access_current().stats__unlock_all_exms();
												persistence.cache_exms();
											}
										}
									}
									if (exmType->is_active())
									{
										handSetup.activeEXM.exm = exmId;
										if (otherHandSetup.activeEXM.exm == exmId)
										{
											otherHandSetup.activeEXM.exm = Name::invalid();
										}
									}
									else
									{
										PilgrimHandEXMSetup exmSetup;
										exmSetup.exm = exmId;
										// make sure they do not double
										handSetup.make_sure_there_is_no_exm(exmId);
										otherHandSetup.make_sure_there_is_no_exm(exmId);
										// remove if we don't have enough place
										if (!handSetup.passiveEXMs.has_place_left())
										{
											handSetup.passiveEXMs.pop_back();
										}
										handSetup.passiveEXMs.push_front(exmSetup);
									}

									mp->set_pending_pilgrim_setup(setup);
								}
								mp->sync_recreate_exms();
							}

						});
					}

					given = true;

					mp->access_overlay_info().show_exms(NAME(upgradeCanisterReward));
					dropShowEXMsRewardIn = 1.0f;
					rewardGivenTo = mp->get_owner();

					if (exmType && exmType->is_active() && exmType->can_be_activated_at_any_time())
					{
						exmType->request_input_tip(mp, heldByHand.get());
					}

					mp->access_overlay_info().on_upgrade_installed();
				}
			}
			if (auto* euo = content.extra.get())
			{
				if (auto* h = hb->get_custom<CustomModules::Health>())
				{
					if (euo->get_health_amount().is_positive())
					{
						givenHealth = true;
						EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
						etr.energyRequested = euo->get_health_amount();
						h->handle_health_energy_transfer_request(etr);
						given |= !etr.energyResult.is_zero();
					}
					if (euo->get_health_missing().is_positive())
					{
						givenHealth = true;
						Energy missing = h->calculate_total_energy_limit() - h->calculate_total_energy_available(EnergyType::Health);
						EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
						etr.energyRequested = missing.adjusted(euo->get_health_missing());
						h->handle_health_energy_transfer_request(etr);
						given |= !etr.energyResult.is_zero();
					}
				}
				for_count(int, iHand, Hand::MAX)
				{
					Hand::Type hand = (Hand::Type)iHand;
					if (euo->should_apply_to_both_hands() || (isHeldByHand.is_set() && hand == isHeldByHand.get()))
					{
						if (auto* h = mp->get_hand(hand))
						{
							if (auto* iet = fast_cast<IEnergyTransfer>(h->get_gameplay()))
							{
								if (euo->get_ammo_amount().is_positive())
								{
									givenAmmo = true;
									EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
									etr.energyRequested = euo->get_ammo_amount();
									iet->handle_ammo_energy_transfer_request(etr);
									given |= !etr.energyResult.is_zero();
								}
								if (euo->get_ammo_missing().is_positive())
								{
									givenAmmo = true;
									Energy missing = mp->get_hand_energy_max_storage((hand)) - mp->get_hand_energy_storage((hand));
									EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
									etr.energyRequested = missing.adjusted(euo->get_ammo_missing());
									iet->handle_ammo_energy_transfer_request(etr);
									given |= !etr.energyResult.is_zero();
								}
							}
						}
					}
				}
			}
			if (_upgradeMachineOrigin && mp)
			{
				auto* grantRefill = mp->get_equipped_exm(EXMID::Passive::upgrade_machine_refill());
				Energy refillUpTo = Energy::zero();
				if (grantRefill)
				{
					refillUpTo = grantRefill->get_owner()->get_variables().get_value<Energy>(NAME(refillUpTo), refillUpTo);
				}
				if (!grantRefill &&
					content.exm.is_set() &&
					content.exm->get_id() == EXMID::Passive::upgrade_machine_refill())
				{
					refillUpTo = content.exm->get_custom_parameters().get_value<Energy>(NAME(refillUpTo), refillUpTo);;
				}
				if (refillUpTo.is_positive())
				{
#ifdef DEBUG_REFILL
					output(TXT("refill up to %S"), refillUpTo.as_string_auto_decimals().to_char());
#endif
					givenHealth = true;
					givenAmmo = true;

					MODULE_OWNER_LOCK_FOR(mp, TXT("UpgradeCanister refill"));

					// how much do we miss
					Energy handMissing[Hand::MAX];
					Energy healthMissing = Energy::zero();

					bool oneHandStorage = GameSettings::get().difficulty.commonHandEnergyStorage;
					int handCount = oneHandStorage ? 1 : Hand::MAX;

					for_count(int, iHandIdx, handCount)
					{
						Hand::Type hand = (Hand::Type)iHandIdx;
						handMissing[hand] = mp->get_hand_energy_max_storage((hand)) - mp->get_hand_energy_storage((hand));
					}
					if (oneHandStorage)
					{
						handMissing[Hand::Right] = Energy::zero();
					}
					if (auto* h = mp->get_owner()->get_custom<CustomModules::Health>())
					{
						healthMissing = h->get_max_total_health() - h->get_total_health();
					}

					// limit missing to refillUpTo
					Energy totalMissing = handMissing[Hand::Left] + handMissing[Hand::Right] + healthMissing;
#ifdef DEBUG_REFILL
					output(TXT("missing %S (hand:L%S R%S health:%S)"),
						totalMissing.as_string_auto_decimals().to_char(),
						handMissing[Hand::Left].as_string_auto_decimals().to_char(),
						handMissing[Hand::Right].as_string_auto_decimals().to_char(),
						healthMissing.as_string_auto_decimals().to_char());
#endif
					if (totalMissing > refillUpTo)
					{
						EnergyCoef useCoef = refillUpTo.as_part_of(totalMissing, true);
						Energy filled = Energy::zero();

						for_count(int, iHandIdx, handCount)
						{
							Hand::Type hand = (Hand::Type)iHandIdx;
							Energy fill = handMissing[hand].adjusted(useCoef);
							filled += fill;
							if (filled > refillUpTo)
							{
								Energy reduceBy = refillUpTo - filled;
								filled -= reduceBy;
								fill -= reduceBy;
							}
							handMissing[hand] = fill;
						}
						if (auto* h = mp->get_owner()->get_custom<CustomModules::Health>())
						{
							healthMissing = refillUpTo - filled;
						}
					}

#ifdef DEBUG_REFILL
					output(TXT("adjusted missing %S (hand:L%S R%S health:%S)"),
						totalMissing.as_string_auto_decimals().to_char(),
						handMissing[Hand::Left].as_string_auto_decimals().to_char(),
						handMissing[Hand::Right].as_string_auto_decimals().to_char(),
						healthMissing.as_string_auto_decimals().to_char());
#endif

					// do actual refill

					for_count(int, iHandIdx, handCount)
					{
						Hand::Type hand = (Hand::Type)iHandIdx;
						mp->add_hand_energy_storage((hand), handMissing[hand]);
					}

					if (auto* h = mp->get_owner()->get_custom<CustomModules::Health>())
					{
						EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
						etr.energyRequested = healthMissing;
						h->handle_health_energy_transfer_request(etr);
					}
				}
			}
			if (given)
			{
				if (auto* origin = get_held_imo())
				{
					if (auto* ha = hb->get_custom<CustomModules::HitIndicator>())
					{
						Framework::RelativeToPresencePlacement r2pp;
						r2pp.be_temporary_snapshot();
						if (r2pp.find_path(hb, origin))
						{
							Vector3 buttonHereWS = r2pp.location_from_target_to_owner(origin->get_presence()->get_centre_of_presence_WS());
							float delay = 0.0f;
							for_count(int, i, hitIndicatorCount)
							{
								ha->indicate_location(0.0f, buttonHereWS, CustomModules::HitIndicator::IndicateParams()
										.with_colour_override(hitIndicatorColour).with_delay(delay).try_being_relative_to(origin).not_damage());
								delay += 0.02f;
							}

							if (givenHealth)
							{
								delay += 0.07f;
								for_count(int, i, 2)
								{
									ha->indicate_location(0.0f, buttonHereWS, CustomModules::HitIndicator::IndicateParams()
										.with_colour_override(hitIndicatorHealthColour).with_delay(delay).try_being_relative_to(origin).not_damage());
									delay += 0.02f;
								}
							}
							if (givenAmmo)
							{
								delay += 0.07f;
								for_count(int, i, 2)
								{
									ha->indicate_location(0.0f, buttonHereWS, CustomModules::HitIndicator::IndicateParams()
										.with_colour_override(hitIndicatorAmmoColour).with_delay(delay).try_being_relative_to(origin).not_damage());
									delay += 0.02f;
								}
							}
						}
					}
					if (auto* s = hb->get_sound())
					{
						s->play_sound(NAME(upgradeCanisterActivated));
					}
				}

				if (Framework::GameUtils::is_local_player(hb))
				{
					PhysicalSensations::OngoingSensation s(PhysicalSensations::OngoingSensation::UpgradeReceived, 0.5f, false);
					if (isHeldByHand.is_set())
					{
						s.for_hand(isHeldByHand.get());
					}
					PhysicalSensations::start_sensation(s);
				}

				auto& poi = mp->access_overlay_info();
				for_count(int, iHand, Hand::MAX)
				{
					Hand::Type hand = (Hand::Type)iHand;
					poi.clear_exm_proposal(hand, true);
					poi.clear_exm_proposal(hand, false);
				}
			}
		}
	}

	return given;
}

bool UpgradeCanister::update_emissive(Emissive _emissive, bool _forceUpdate)
{
	_emissive = forcedEmissive.get(_emissive);
	if (emissive == _emissive && !_forceUpdate)
	{
		return false;
	}
	emissive = _emissive;
	for_every_ref(s, canisterHandler.get_switches())
	{
		if (auto* ec = s->get_custom<CustomModules::EmissiveControl>())
		{
			ec->emissive_deactivate_all();
			if (emissive == EmissiveReady)
			{
				ec->emissive_activate(NAME(ready));
			}
			if (emissive == EmissiveUsed)
			{
				ec->emissive_activate(NAME(used));
			}
			if (emissive == EmissiveHeld)
			{
				ec->emissive_activate(NAME(held));
			}
		}
	}

	return true;
}

bool UpgradeCanister::force_emissive(Optional<Emissive> const & _emissive)
{
	forcedEmissive = _emissive;
	return update_emissive(forcedEmissive.get(emissive));
}
