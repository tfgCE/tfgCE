#include "aiLogic_exmEnergyManipulator.h"

#include "..\..\..\game\gameDirector.h"
#include "..\..\..\game\gameSettings.h"

#include "..\..\..\modules\energyTransfer.h"
#include "..\..\..\modules\gameplay\moduleEXM.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\..\modules\gameplay\modulePilgrimHand.h"
#include "..\..\..\modules\custom\health\mc_health.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\module\moduleSound.h"

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
DEFINE_STATIC_NAME(energyManipulatorMaxDifference);
DEFINE_STATIC_NAME(energyManipulatorTransferSpeed);

// sounds
DEFINE_STATIC_NAME(transfer);

//

REGISTER_FOR_FAST_CAST(EXMEnergyManipulator);

EXMEnergyManipulator::EXMEnergyManipulator(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
{
}

EXMEnergyManipulator::~EXMEnergyManipulator()
{
}

void EXMEnergyManipulator::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	bool prevTransferring = transferring;
	bool prevRequested = requested;
	transferring = false;
	requested = false;
	if (pilgrimOwner.get() && pilgrimModule && owner.get() && hand != Hand::MAX)
	{
		if (!initialised)
		{
			initialised = true;
			transferSpeed = Energy::get_from_storage(owner->access_variables(), NAME(energyManipulatorTransferSpeed), Energy(1));
		}

		bool allow = true;
		if (auto* gd = GameDirector::get())
		{
			if (gd->are_ammo_storages_unavailable())
			{
				allow = false;
			}
		}

		if (allow && pilgrimModule->is_controls_use_exm_pressed(hand) &&
			pilgrimModule->is_active_exm_equipped(hand, owner.get()))
		{
			requested = true;
			auto* health = pilgrimOwner->get_custom<CustomModules::Health>();
			auto* imoHandL = pilgrimModule->get_hand(Hand::Left);
			auto* imoHandR = pilgrimModule->get_hand(Hand::Right);
			auto* handL = imoHandL ? imoHandL->get_gameplay_as<ModulePilgrimHand>() : nullptr;
			auto* handR = imoHandR ? imoHandR->get_gameplay_as<ModulePilgrimHand>() : nullptr;

			bool commonHandEnergyStorage = GameSettings::get().difficulty.commonHandEnergyStorage;

			float transferBalance = 0.0f; // 0 balance, 1 to health, -1 to main storages

			if (auto* hIMO = pilgrimModule->get_hand(hand))
			{
				auto* p = hIMO->get_presence();
				Transform placementPOS = Transform::identity; // pilgrim's object space
				if (auto* path = p->get_path_to_attached_to())
				{
					if (auto* pil = path->get_target())
					{
						an_assert(pil == pilgrimOwner.get());
						Transform placementPRS = path->from_owner_to_target(p->get_placement());
						placementPOS = pil->get_presence()->get_placement().to_local(placementPRS);
					}
				}
				// works better (than using degs) as balancing is quite greedy and we should hold it back as much as possible
				transferBalance = clamp(placementPOS.get_axis(Axis::Forward).z, -1.0f, 1.0f);
				//float sinvalue = clamp(placementPOS.get_axis(Axis::Forward).z, -1.0f, 1.0f);
				//transferBalance = clamp(asin_deg(sinvalue) / 90.0f, -1.0f, 1.0f);
			}

			if (health && handL && handR)
			{
				Energy maxHealth = health->get_max_total_health();
				Energy maxHandL = pilgrimModule->get_hand_energy_max_storage(GameSettings::actual_hand(Hand::Left));
				Energy maxHandR = commonHandEnergyStorage ? Energy::zero() : pilgrimModule->get_hand_energy_max_storage(GameSettings::actual_hand(Hand::Right));
				//
				Energy maxTotal = maxHealth + maxHandL + maxHandR;

				Energy currHealth = health->get_total_health();
				Energy currHandL = pilgrimModule->get_hand_energy_storage(GameSettings::actual_hand(Hand::Left));
				Energy currHandR = commonHandEnergyStorage ? Energy::zero() : pilgrimModule->get_hand_energy_storage(GameSettings::actual_hand(Hand::Right));
				//
				Energy total = currHealth + currHandL + currHandR;

				Energy maxTransfer = (transferSpeed).timed(_deltaTime, transferBit);

				float const borderValue = 0.2f;
				float transferToBalance = clamp(1.0f + borderValue - abs(transferBalance) * (1.0f + 2.0f * borderValue), 0.0f, 1.0f);
				an_assert(transferToBalance >= 0.0f && transferToBalance <= 1.0f);

				Energy changeHandL = Energy::zero();
				Energy changeHandR = Energy::zero();
				Energy changeHealth = Energy::zero();

				Energy transferForBalancing = maxTransfer.mul(transferToBalance);
				Energy transforForGiving = maxTransfer - transferForBalancing;

				// both methods work separately, we just split transfer between two
				// we shouldn't get outside valid range, energy transfer loop below takes care of that and if something is dodgy, redistributes rest

				// balancing
				if (!transferForBalancing.is_zero())
				{
					float ptHandL = maxHandL.div_to_float(maxTotal);
					float ptHandR = commonHandEnergyStorage ? 0.0f : maxHandR.div_to_float(maxTotal);

					Energy desHandL = total.mul(ptHandL);
					Energy desHandR = total.mul(ptHandR);

					Energy balanceHandL = desHandL - currHandL;
					Energy balanceHandR = desHandR - currHandR;
					Energy balanceHealth = -balanceHandL - balanceHandR;

					an_assert(!commonHandEnergyStorage || balanceHandR.is_zero());
					if (commonHandEnergyStorage)
					{
						balanceHandR = Energy::zero();
					}

					Energy absBalanceHandL = abs(balanceHandL);
					Energy absBalanceHandR = abs(balanceHandR);
					Energy absBalanceHealth = abs(balanceHealth);

					Energy totalTransfer = absBalanceHandL + absBalanceHandR + absBalanceHealth;
					Energy totalTransferPossible = min(totalTransfer, transferForBalancing);

					if (totalTransferPossible < totalTransfer)
					{
						float allowed = totalTransferPossible.div_to_float(totalTransfer);
						balanceHandL = balanceHandL.mul(allowed);
						balanceHandR = balanceHandR.mul(allowed);
						balanceHealth = -balanceHandL - balanceHandR;
					}

					changeHandL += balanceHandL;
					changeHandR += balanceHandR;
					changeHealth += balanceHealth;
				}

				// giving
				if (!transforForGiving.is_zero())
				{
					if (transferBalance > 0.0f)
					{
						Energy giveHealth = min(transforForGiving, maxHealth - currHealth);
						Energy takeHands = min(transforForGiving, currHandL + currHandR);

						Energy transferEnergy = min(giveHealth, takeHands);

						changeHealth += transferEnergy;

						{
							float takeR = commonHandEnergyStorage ? 0.0f : currHandR.div_to_float(currHandR + currHandL);
							Energy takeHandR = transferEnergy.mul(takeR);
							Energy takeHandL = transferEnergy - takeHandR;
							changeHandL -= takeHandL;
							changeHandR -= takeHandR;
						}
					}
					else
					{
						Energy giveHands = min(transforForGiving, (maxHandL - currHandL) + (maxHandR - currHandR));
						Energy takeHealth = min(transforForGiving, currHealth);

						Energy transferEnergy = min(giveHands, takeHealth);

						changeHealth -= transferEnergy;

						{
							float fillR = commonHandEnergyStorage ? 0.0f : (maxHandR - currHandR).div_to_float((maxHandR - currHandR) + (maxHandL - currHandL));
							Energy fillHandR = transferEnergy.mul(fillR);
							Energy fillHandL = transferEnergy - fillHandR;
							changeHandL += fillHandL;
							changeHandR += fillHandR;
						}
					}
				}

				{
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
							etr.minLeft = Energy::one(); // we require SOME energy to be left in health
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

				Energy postHealth = health->get_total_health();
				Energy postHandL = pilgrimModule->get_hand_energy_storage(GameSettings::actual_hand(Hand::Left));
				Energy postHandR = commonHandEnergyStorage ? Energy::zero() : pilgrimModule->get_hand_energy_storage(GameSettings::actual_hand(Hand::Right));

				transferring = postHealth != currHealth || postHandL != currHandL || postHandR != currHandR;
			}
		}
	}
	if (prevRequested != requested)
	{
		if (exmModule)
		{
			exmModule->mark_exm_active(requested);
		}
	}
	if (prevTransferring != transferring)
	{
		if (auto* mind = get_mind())
		{
			if (auto* imo = mind->get_owner_as_modules_owner())
			{
				if (auto* s = imo->get_sound())
				{
					s->stop_sound(NAME(transfer));
					if (transferring)
					{
						s->play_sound(NAME(transfer));
					}
				}
			}
		}
	}
}
