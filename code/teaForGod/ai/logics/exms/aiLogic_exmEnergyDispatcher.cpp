#include "aiLogic_exmEnergyDispatcher.h"

#include "..\..\..\game\gameDirector.h"
#include "..\..\..\game\gameSettings.h"

#include "..\..\..\library\exmType.h"

#include "..\..\..\modules\energyTransfer.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\..\modules\gameplay\modulePilgrimHand.h"
#include "..\..\..\modules\custom\health\mc_health.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// object variables
DEFINE_STATIC_NAME(energyDispatcherMaxDifference);
DEFINE_STATIC_NAME(energyDispatcherTransferSpeed);
DEFINE_STATIC_NAME(energyDispatcherTransferCooldown);

//

REGISTER_FOR_FAST_CAST(EXMEnergyDispatcher);

EXMEnergyDispatcher::EXMEnergyDispatcher(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
{
}

EXMEnergyDispatcher::~EXMEnergyDispatcher()
{
}

void EXMEnergyDispatcher::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	bool allow = true;
	if (auto* gd = GameDirector::get())
	{
		if (gd->are_ammo_storages_unavailable())
		{
			allow = false;
		}
	}

	if (pilgrimOwner.get() && pilgrimModule && owner.get() && hand != Hand::MAX)
	{
		if (!initialised)
		{
			initialised = true;
			maxDifference = Energy::get_from_storage(owner->access_variables(), NAME(energyDispatcherMaxDifference), Energy::zero());
			transferSpeed = Energy::get_from_storage(owner->access_variables(), NAME(energyDispatcherTransferSpeed), Energy(1));
			transferCooldown = owner->get_variables().get_value<float>(NAME(energyDispatcherTransferCooldown), transferCooldown);
		}

		auto* health = pilgrimOwner->get_custom<CustomModules::Health>();
		auto* imoHandL = pilgrimModule->get_hand(Hand::Left);
		auto* imoHandR = pilgrimModule->get_hand(Hand::Right);
		auto* handL = imoHandL ? imoHandL->get_gameplay_as<ModulePilgrimHand>() : nullptr;
		auto* handR = imoHandR ? imoHandR->get_gameplay_as<ModulePilgrimHand>() : nullptr;

		bool commonHandEnergyStorage = GameSettings::get().difficulty.commonHandEnergyStorage;

		if (health && handL && handR)
		{
			Energy maxHealth = health->get_max_total_health();
			Energy maxHandL = pilgrimModule->get_hand_energy_max_storage(GameSettings::actual_hand(Hand::Left));
			Energy maxHandR = commonHandEnergyStorage? Energy::zero() : pilgrimModule->get_hand_energy_max_storage(GameSettings::actual_hand(Hand::Right));
			//
			Energy maxTotal = maxHealth + maxHandL + maxHandR;

			Energy currHealth = health->get_total_health();
			Energy currHandL = pilgrimModule->get_hand_energy_storage(GameSettings::actual_hand(Hand::Left));
			Energy currHandR = commonHandEnergyStorage? Energy::zero() : pilgrimModule->get_hand_energy_storage(GameSettings::actual_hand(Hand::Right));
			//
			Energy total = currHealth + currHandL + currHandR;

			if (currHealth != prevHealth ||
				currHandL != prevHandL ||
				currHandR != prevHandR ||
				!allow)
			{
				cooldownLeft = transferCooldown;
			}
			else
			{
				cooldownLeft = max(0.0f, cooldownLeft - _deltaTime);
			}

			if (cooldownLeft <= 0.0f && !total.is_zero())
			{
				float ptHandL = maxHandL.div_to_float(maxTotal);
				float ptHandR = commonHandEnergyStorage? 0.0f : maxHandR.div_to_float(maxTotal);

				Energy desHandL = total.mul(ptHandL);
				Energy desHandR = total.mul(ptHandR);

				Energy changeHandL = desHandL - currHandL;
				Energy changeHandR = desHandR - currHandR;
				Energy changeHealth = -changeHandL - changeHandR;

				an_assert(!commonHandEnergyStorage || changeHandR.is_zero());

				{
					Energy minHealth = Energy(10);
					// health should not go lower than this, so you won't kill yourself shooting
					if (currHealth + changeHealth < minHealth)
					{
						changeHealth = min(minHealth - currHealth, currHandL + currHandR);
						Energy handSum = currHandL + currHandR - changeHealth;
						Energy handLTgt = commonHandEnergyStorage ? handSum : handSum / 2;
						changeHandL = handLTgt - currHandL;
						changeHandR = -changeHandL - changeHealth;
					}
				}

				an_assert(!commonHandEnergyStorage || changeHandR.is_zero());
				if (commonHandEnergyStorage)
				{
					changeHandR = Energy::zero();
				}

				Energy absChangeHandL = abs(changeHandL);
				Energy absChangeHandR = abs(changeHandR);
				Energy absChangeHealth = abs(changeHealth);

				if (absChangeHandL > maxDifference ||
					absChangeHandR > maxDifference ||
					absChangeHealth > maxDifference)
				{
					Energy maxTransfer = (transferSpeed * (commonHandEnergyStorage? 2 : 3)).timed(_deltaTime, transferBit);

					Energy totalTransfer = absChangeHandL + absChangeHandR + absChangeHealth;
					Energy totalTransferPossible = min(totalTransfer, maxTransfer);

					if (totalTransferPossible < totalTransfer)
					{
						float allowed = totalTransferPossible.div_to_float(totalTransfer);
						changeHandL = changeHandL.mul(allowed);
						changeHandR = changeHandR.mul(allowed);
						changeHealth = -changeHandL - changeHandR;
					}

					int restToIdx = 0;
					int tryCount = 0;
					while (tryCount < 100)
					{
						++tryCount;
						Energy rest = Energy::zero();

						if (!changeHandL.is_zero())
						{
							EnergyTransferRequest etr(changeHandL.is_positive() ? EnergyTransferRequest::Deposit : EnergyTransferRequest::Withdraw);
							etr.instigator = pilgrimOwner.get();
							etr.energyRequested = abs(changeHandL);
							handL->handle_ammo_energy_transfer_request(etr);
							rest += changeHandL.is_positive() ? etr.energyRequested : -etr.energyRequested;
							changeHandL = Energy::zero();
						}
						if (!changeHandR.is_zero() && !commonHandEnergyStorage)
						{
							EnergyTransferRequest etr(changeHandR.is_positive() ? EnergyTransferRequest::Deposit : EnergyTransferRequest::Withdraw);
							etr.instigator = pilgrimOwner.get();
							etr.energyRequested = abs(changeHandR);
							handR->handle_ammo_energy_transfer_request(etr);
							rest += changeHandR.is_positive() ? etr.energyRequested : -etr.energyRequested;
							changeHandR = Energy::zero();
						}
						if (!changeHealth.is_zero())
						{
							EnergyTransferRequest etr(changeHealth.is_positive() ? EnergyTransferRequest::Deposit : EnergyTransferRequest::Withdraw);
							etr.instigator = pilgrimOwner.get();
							etr.energyRequested = abs(changeHealth);
							health->handle_health_energy_transfer_request(etr);
							rest += changeHealth.is_positive() ? etr.energyRequested : -etr.energyRequested;
							changeHealth = Energy::zero();
						}
						if (rest.is_zero())
						{
							break;
						}
						else
						{
							if (restToIdx == 0)
							{
								changeHealth = rest;
							}
							if (restToIdx == 1)
							{
								changeHandL = rest;
							}
							if (restToIdx == 2)
							{
								changeHandR = rest;
							}
							restToIdx = (restToIdx + 1) % (commonHandEnergyStorage ? 2 : 3);
						}
					}
				}
			}

			{
				prevHealth = health->get_total_health();
				prevHandL = pilgrimModule->get_hand_energy_storage(GameSettings::actual_hand(Hand::Left));
				prevHandR = commonHandEnergyStorage ? Energy::zero() : pilgrimModule->get_hand_energy_storage(GameSettings::actual_hand(Hand::Right));
			}
		}
	}
}

Energy EXMEnergyDispatcher::redistribute_energy_from(IEnergyTransfer* _source, EnergyType::Type _energyType, Energy const& _amount)
{
	Energy transferredTotal = Energy::zero();
	if (auto* gd = GameDirector::get())
	{
		if (gd->are_ammo_storages_unavailable())
		{
			return transferredTotal;
		}
	}
	ModulePilgrim* pilgrimModule = nullptr;
	if (auto* module = fast_cast<Framework::Module>(_source))
	{
		if (auto* imo = module->get_owner())
		{
			if (auto* inst = imo->get_top_instigator())
			{
				pilgrimModule = inst->get_gameplay_as<ModulePilgrim>();
			}
		}
	}
	if (pilgrimModule)
	{
		if (pilgrimModule->has_exm_equipped(EXMID::Passive::energy_dispatcher()))
		{
			if (auto* pilgrimIMO = pilgrimModule->get_owner())
			{
				struct Dest
				{
					IEnergyTransfer* it;
					EnergyType::Type energyType;
					Dest() {}
					Dest(IEnergyTransfer* _it, EnergyType::Type _energyType) : it(_it), energyType(_energyType) {}
				};
				ArrayStatic<Dest, 5> dests;
				bool oneOfSystems = false;
				if (auto* h = pilgrimIMO->get_custom<CustomModules::Health>())
				{
					if (h == _source)
					{
						oneOfSystems = true;
					}
					else
					{
						dests.push_back(Dest(h, EnergyType::Health));
					}
				}
				//bool commonHandEnergyStorage = GameSettings::get().difficulty.commonHandEnergyStorage;
				// ignore common hand energy storage issue?
				for_count(int, hIdx, Hand::MAX)
				{
					if (auto* handIMO = pilgrimModule->get_hand((Hand::Type)hIdx))
					{
						if (auto* it = handIMO->get_gameplay_as<IEnergyTransfer>())
						{
							if (it == _source)
							{
								oneOfSystems = true;
							}
							if (it != _source || _energyType != EnergyType::Ammo) // we allow health to go to the same hand
							{
								dests.push_back(Dest(it, EnergyType::Ammo));
							}
						}
					}
				}
				if (oneOfSystems)
				{
					Energy energyLeft = _amount;
					int tryIdx = 0;
					while (energyLeft.is_positive() && tryIdx < 5)
					{
						++tryIdx;
						Energy energyLeftNow = energyLeft;
						for_count(int, i, dests.get_size())
						{
							Energy transferNow = energyLeft / dests.get_size();
							if (i == dests.get_size() - 1)
							{
								transferNow = energyLeftNow;
							}
							else
							{
								transferNow = min(transferNow, energyLeftNow);
							}
							auto& dest = dests[i];
							Energy transferNowLeft = transferNow;
							IEnergyTransfer::transfer_energy_between_two_monitor_energy(pilgrimIMO, REF_ transferNowLeft, _source, _energyType, dest.it, dest.energyType);
							Energy transferred = (transferNow - transferNowLeft);
							energyLeftNow -= transferred;
							energyLeft -= transferred;
							transferredTotal += transferred;
						}
					}
				}
			}
		}
	}
	return transferredTotal;
}

