#include "aiLogic_exmShield.h"

#include "..\..\..\library\library.h"
#include "..\..\..\modules\gameplay\moduleEXM.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\..\modules\custom\health\mc_health.h"

#include "..\..\..\..\core\debug\debugRenderer.h"

#include "..\..\..\..\framework\game\game.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\world\room.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
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

// variables
DEFINE_STATIC_NAME(spawnShield);
DEFINE_STATIC_NAME(spawnShieldType);
DEFINE_STATIC_NAME(atHandSocket);

//

REGISTER_FOR_FAST_CAST(EXMShield);

EXMShield::EXMShield(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
{
	SET_EXTRA_DEBUG_INFO(spawnedShields, TXT("EXMShield.spawnedShields"));
}

EXMShield::~EXMShield()
{
	recall_shield(true);
}

void EXMShield::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);
	spawnShield.set_name(_parameters.get_value<Framework::LibraryName>(NAME(spawnShield), spawnShield.get_name()));
	spawnShield.find(Library::get_current());
	{
		Name readSpawnShieldType = _parameters.get_value<Name>(NAME(spawnShieldType), Name::invalid());
		if (readSpawnShieldType.is_valid())
		{
			if (readSpawnShieldType == TXT("hand"))
			{
				spawnShieldType = ShieldType::Hand;
			}
			else if (readSpawnShieldType == TXT("stand"))
			{
				spawnShieldType = ShieldType::Stand;
			}
			else
			{
				error(TXT("not recognised shield type \"%S\""), readSpawnShieldType.to_char());
			}
		}
	}
	atHandSocket = _parameters.get_value<Name>(NAME(atHandSocket), atHandSocket);
}

void EXMShield::recall_shield(bool _removeFromWorldImmediately, int _count)
{
	while (!spawnedShields.is_empty() && _count > 0)
	{
		auto* spawnedShield = spawnedShields.get_first().get();
		if (spawnedShield)
		{
			if (auto* h = spawnedShield->get_custom<CustomModules::Health>())
			{
				Energy currentEnergyState = max(Energy::zero(), h->calculate_total_energy_available(EnergyType::Health));
				if (h->is_alive())
				{
					h->perform_death();
				}

				if (spawnShieldType == ShieldType::Hand)
				{
					if (!initialEnergyState.is_zero() && pilgrimModule && pilgrimOwner.get() && hand != Hand::MAX && exmModule)
					{
						if (auto* ph = pilgrimOwner->get_custom<CustomModules::Health>())
						{
							if (ph->is_alive())
							{
								pilgrimModule->receive_equipment_energy_for(hand, (exmModule->get_cost_single() * currentEnergyState / initialEnergyState).adjusted(exmModule->get_recall_percentage()));
							}
						}
					}
				}
			}
			else
			{
				spawnedShield->cease_to_exist(_removeFromWorldImmediately);
			}
		}
		--_count;
		spawnedShields.remove_at(0);
	}
}

void EXMShield::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (pilgrimOwner.get() && pilgrimModule && exmModule && owner.get() && hand != Hand::MAX)
	{
		if (spawnedShields.is_empty())
		{
			if (spawnShieldType == ShieldType::Hand)
			{
				exmModule->mark_exm_active(false);
			}
		}
		else
		{
			for (int i = 0; i < spawnedShields.get_size(); ++i)
			{
				if (!spawnedShields[i].get())
				{
					spawnedShields.remove_at(i);
					--i;
				}
			}
		}
		if (pilgrimModule->has_controls_use_exm_been_pressed(hand) &&
			pilgrimModule->is_active_exm_equipped(hand, owner.get()))
		{
			{
				if (exmModule->is_exm_active() && spawnShieldType == ShieldType::Hand)
				{
					recall_shield();
					exmModule->mark_exm_active(false);
				}
				else
				{
					{
						bool spawnShieldNow = false;
						if (spawnShieldType == ShieldType::Hand)
						{
							spawnShieldNow = exmModule->mark_exm_active(true);
						}
						else if (spawnShieldType == ShieldType::Stand)
						{
							spawnShieldNow = exmModule->mark_exm_active_blink();
						}
						if (spawnShieldNow)
						{
							Framework::IModulesOwner* handActor = pilgrimModule->get_hand(hand);
							an_assert(handActor);
							an_assert(spawnShield.get());
							if (handActor && spawnShield.get())
							{
								if (spawnShieldType == ShieldType::Hand)
								{
									recall_shield();
								}
								else if (spawnShieldType == ShieldType::Stand)
								{
									if (spawnedShields.get_space_left() < 1)
									{
										recall_shield(false, 1);
									}
								}

								Framework::Room* inRoom = handActor->get_presence()->get_in_room();
								Transform placementWS = handActor->get_presence()->get_placement();
								if (spawnShieldType == ShieldType::Stand)
								{
									Framework::IModulesOwner* top = nullptr;
									handActor->get_presence()->get_attached_placement_for_top(OUT_ inRoom, OUT_ placementWS, pilgrimOwner.get(), &top);
									if (top == pilgrimOwner.get() &&
										inRoom == pilgrimOwner->get_presence()->get_in_room())
									{
										float lerpShieldDirUsingHandSocket = 0.0f;
										if (atHandSocket.is_valid())
										{
											Transform socketOS = handActor->get_appearance()->calculate_socket_os(atHandSocket);
#ifdef ALLOW_GLOBAL_DEBUG
											{
												LogInfoContext log;
												socketOS.log(log);
												log.output_to_output();
											}
#endif
											placementWS = placementWS.to_world(socketOS);
											lerpShieldDirUsingHandSocket = 0.8f;
										}
										//debug_push_filter(alwaysDraw);
										//debug_context(inRoom);
										//debug_draw_time_based(2.0f, debug_draw_line(true, Colour::green, pilgrimOwner->get_presence()->get_placement().get_translation(), placementWS.get_translation()));
										//debug_draw_time_based(2.0f, debug_draw_transform_size(true, placementWS, 0.02f));
										//debug_no_context();
										//debug_pop_filter();

										Transform ownerPlacement = pilgrimOwner->get_presence()->get_placement();
										Vector3 up = ownerPlacement.get_axis(Axis::Up);
										Vector3 flatDiff = (placementWS.get_translation() - ownerPlacement.get_translation());
										flatDiff = flatDiff.drop_using(up);
										float const distanceBeyondHand = 0.1f;
										flatDiff = flatDiff.normal() * (flatDiff.length() + distanceBeyondHand);
										Vector3 shieldDir = placementWS.get_axis(Axis::Forward);
										// mix pure shield dir with flat diff (if not aligned to XY plane, we will use flat diff more)
										// only after then drop using up dir and normalise
										shieldDir = lerp(lerpShieldDirUsingHandSocket, flatDiff.normal(), shieldDir).drop_using(up).normal();
										placementWS = look_matrix(ownerPlacement.get_translation() + flatDiff, shieldDir, up).to_transform();
									}
									else
									{
										float const distance = 0.5f;
										warn(TXT("hand detached?!"));
										inRoom = pilgrimOwner->get_presence()->get_in_room();
										placementWS = pilgrimOwner->get_presence()->get_placement();
										placementWS = placementWS.to_world(Transform(Vector3(0.0f, 0.0f, distance), Quat::identity));
									}

									pilgrimOwner->get_presence()->move_through_doors(placementWS, inRoom);
								}

								SafePtr<Framework::IModulesOwner> imoSafe;
								imoSafe = pilgrimOwner;

								Framework::Game::get()->add_immediate_sync_world_job(TXT("spawn shield"),
									[imoSafe, inRoom, placementWS, this, handActor]()
								{
									Framework::Object* spawnedObject = nullptr;

									if (imoSafe.get())
									{
										Framework::ScopedAutoActivationGroup saag(fast_cast<Framework::WorldObject>(imoSafe.get()), inRoom);

										Framework::Game::get()->perform_sync_world_job(TXT("spawn shield"), [this, &spawnedObject, inRoom]()
										{
											spawnedObject = spawnShield->create(spawnShield->get_name().to_string());
											spawnedObject->init(inRoom->get_in_sub_world());
										});
										spawnedObject->access_individual_random_generator() = Random::Generator();

										if (imoSafe.is_set())
										{
											spawnedObject->access_variables().set_from(imoSafe->get_variables());
											spawnedObject->learn_from(imoSafe->access_variables());
										}

										spawnedObject->initialise_modules();
										spawnedObject->set_instigator(imoSafe.get());
										spawnedObject->be_non_autonomous();

										// store 
										spawnedShields.push_back(SafePtr<Framework::IModulesOwner>(spawnedObject));
										initialEnergyState = Energy::zero();

										if (spawnShieldType == ShieldType::Hand)
										{
											spawnedObject->get_presence()->attach_to_socket(handActor, atHandSocket, true);
											if (auto * h = spawnedObject->get_custom<CustomModules::Health>())
											{
												initialEnergyState = h->calculate_total_energy_available(EnergyType::Health);
											}
										}
										else if (spawnShieldType == ShieldType::Stand)
										{
											spawnedObject->get_presence()->place_in_room(inRoom, placementWS);
											if (auto* b = imoSafe->get_presence()->get_based_on())
											{
												// share same base - so we can spawn shield outside an elevator while bing in an elevator
												spawnedObject->get_presence()->force_base_on(b);
											}
										}
										else
										{
											todo_important(TXT("implement"));
										}

										// auto activate
									}
								});
							}
						}
					}
				}
			}
		}
		pilgrimModule->set_block_display(hand, PilgrimDisplayBlock::Shield, exmModule->is_exm_active());
	}
}
