#include "aiLogic_airfighter.h"

#include "..\..\modules\custom\health\mc_health.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// ai messages
DEFINE_STATIC_NAME_STR(aim_sw_start, TXT("shooter waves; start"));
DEFINE_STATIC_NAME_STR(aim_sw_move, TXT("shooter waves; move"));
DEFINE_STATIC_NAME_STR(aim_sw_attack, TXT("shooter waves; attack"));
DEFINE_STATIC_NAME_STR(aim_addTarget, TXT("airfighter; add target"));
DEFINE_STATIC_NAME_STR(aim_dropAllTargets, TXT("airfighter; drop all targets"));
DEFINE_STATIC_NAME_STR(aim_setup, TXT("airfighter; setup"));
DEFINE_STATIC_NAME_STR(aim_attack, TXT("airfighter; attack"));
DEFINE_STATIC_NAME_STR(aim_attackStop, TXT("airfighter; attack stop"));
DEFINE_STATIC_NAME(targetIMO);
DEFINE_STATIC_NAME(targetPlacementSocket);
DEFINE_STATIC_NAME(attackDistance);
DEFINE_STATIC_NAME(attackLength);
DEFINE_STATIC_NAME(attackDeadLimit);
DEFINE_STATIC_NAME(readyToAttackDistance);
DEFINE_STATIC_NAME(targetDistance);
DEFINE_STATIC_NAME(flyByAttackDistance);
DEFINE_STATIC_NAME(preferredDistance);
DEFINE_STATIC_NAME(preferredAltitude);
DEFINE_STATIC_NAME(preferredApproachAltitude);
DEFINE_STATIC_NAME(preferredAwayAltitude);
DEFINE_STATIC_NAME(limitFollowRelVelocity);
DEFINE_STATIC_NAME(breakOffDelay);
DEFINE_STATIC_NAME(breakOffYaw);
DEFINE_STATIC_NAME(followIMO);
DEFINE_STATIC_NAME(useFollowIMO);
DEFINE_STATIC_NAME(allowFollowOffsetAdditionalRange);
DEFINE_STATIC_NAME(followOffset);
DEFINE_STATIC_NAME(flyByAttack);
DEFINE_STATIC_NAME(moveTo);
DEFINE_STATIC_NAME(moveDir);
DEFINE_STATIC_NAME(followRelVelocity);
DEFINE_STATIC_NAME(readyToAttack);
DEFINE_STATIC_NAME(attack);
DEFINE_STATIC_NAME(shootStartOffsetPt);
DEFINE_STATIC_NAME(shootStartOffsetTS);

// sockets
DEFINE_STATIC_NAME(r_muzzle);
DEFINE_STATIC_NAME(l_muzzle);

// variables
DEFINE_STATIC_NAME(breakingOff);

//

REGISTER_FOR_FAST_CAST(Airfighter);

#define READ_PARAM(_struct, _type, _where) \
if (auto* ptr = _message.get_param(NAME(_where))) \
{ \
	_struct._where = ptr->get_as<_type>(); \
}

Airfighter::Airfighter(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData)
{
	airfighterData = fast_cast<AirfighterData>(_logicData);

	useCanister = false;

	arms.setup(_mind, airfighterData->armsData);

	messageHandler.use_with(_mind);
	{
		messageHandler.set(NAME(aim_sw_start), [this](Framework::AI::Message const& _message)
			{
				shooterWaveSetup = ShooterWaveSetup();
				shooterWaveActive = true;
				shooterWaveActiveStart = true;
				beActive = false;
				beActiveAttack = false;
				if (auto* ptr = _message.get_param(NAME(targetIMO)))
				{
					shooterWaveSetup.targetIMO = ptr->get_as< SafePtr<Framework::IModulesOwner> >();
				}
				if (auto* ptr = _message.get_param(NAME(targetPlacementSocket)))
				{
					shooterWaveSetup.targetPlacementSocket = ptr->get_as<Name>();
				}
				if (auto* ptr = _message.get_param(NAME(followIMO)))
				{
					shooterWaveSetup.followIMO = ptr->get_as< SafePtr<Framework::IModulesOwner> >();
				}
				READ_PARAM(shooterWaveSetup, float, limitFollowRelVelocity);
				READ_PARAM(shooterWaveSetup, Vector3, moveTo);
				// doesn't read moveDir and followRelVelocity
				READ_PARAM(shooterWaveSetup, bool, readyToAttack);
				READ_PARAM(shooterWaveSetup, bool, attack);
			});
		messageHandler.set(NAME(aim_sw_move), [this](Framework::AI::Message const& _message)
			{
				shooterWaveSetup = ShooterWaveSetup();
				shooterWaveActive = true;
				beActive = false;
				beActiveAttack = false;
				if (auto* ptr = _message.get_param(NAME(targetIMO)))
				{
					shooterWaveSetup.targetIMO = ptr->get_as< SafePtr<Framework::IModulesOwner> >();
				}
				if (auto* ptr = _message.get_param(NAME(targetPlacementSocket)))
				{
					shooterWaveSetup.targetPlacementSocket = ptr->get_as<Name>();
				}
				if (auto* ptr = _message.get_param(NAME(followIMO)))
				{
					shooterWaveSetup.followIMO = ptr->get_as< SafePtr<Framework::IModulesOwner> >();
				}
				READ_PARAM(shooterWaveSetup, float, limitFollowRelVelocity);
				READ_PARAM(shooterWaveSetup, Vector3, moveTo);
				READ_PARAM(shooterWaveSetup, Vector3, moveDir);
				READ_PARAM(shooterWaveSetup, Vector3, followRelVelocity);
				READ_PARAM(shooterWaveSetup, bool, readyToAttack);
				READ_PARAM(shooterWaveSetup, bool, attack);
			});
		messageHandler.set(NAME(aim_sw_attack), [this](Framework::AI::Message const& _message)
			{
				if (auto* ptr = _message.get_param(NAME(targetIMO)))
				{
					shooterWaveSetup.targetIMO = ptr->get_as< SafePtr<Framework::IModulesOwner> >();
				}
				if (auto* ptr = _message.get_param(NAME(targetPlacementSocket)))
				{
					shooterWaveSetup.targetPlacementSocket = ptr->get_as<Name>();
				}
				shooterWaveSetup.readyToAttack.clear();
				shooterWaveSetup.attack.clear();
				READ_PARAM(shooterWaveSetup, bool, readyToAttack);
				READ_PARAM(shooterWaveSetup, bool, attack);
				READ_PARAM(shooterWaveSetup, float, attackLength);
				READ_PARAM(shooterWaveSetup, float, shootStartOffsetPt);
				READ_PARAM(shooterWaveSetup, Vector3, shootStartOffsetTS);
				if (shooterWaveSetup.attack.is_set() &&
					shooterWaveSetup.attack.get())
				{
					shooterWaveSetup.readyToAttack = true;
				}
			});
		messageHandler.set(NAME(aim_addTarget), [this](Framework::AI::Message const& _message)
			{
				beActive = true;
				shooterWaveActive = false;
				if (auto* ptr = _message.get_param(NAME(targetIMO)))
				{
					TargetInfo ti;
					ti.target = ptr->get_as< SafePtr<Framework::IModulesOwner> >();
					if (auto* atiPtr = _message.get_param(NAME(attackLength)))
					{
						ti.attackLength = atiPtr->get_as<float>();
					}
					if (auto* atiPtr = _message.get_param(NAME(attackDeadLimit)))
					{
						ti.attackDeadLimit = atiPtr->get_as<float>();
					}
					targetList.push_back(ti);
				}
			});
		messageHandler.set(NAME(aim_dropAllTargets), [this](Framework::AI::Message const& _message)
			{
				beActive = false;
				shooterWaveActive = false;
				if (auto* ptr = _message.get_param(NAME(targetIMO)))
				{
					targetList.clear();
				}
			});
		messageHandler.set(NAME(aim_setup), [this](Framework::AI::Message const& _message)
			{
				beActive = true;
				shooterWaveActive = false;
				if (auto* ptr = _message.get_param(NAME(followIMO)))
				{
					airfighterSetup.followIMO = ptr->get_as< SafePtr<Framework::IModulesOwner> >();
				}
				READ_PARAM(airfighterSetup, bool, useFollowIMO);
				READ_PARAM(airfighterSetup, bool, allowFollowOffsetAdditionalRange);
				READ_PARAM(airfighterSetup, float, attackDistance);
				READ_PARAM(airfighterSetup, float, readyToAttackDistance);
				READ_PARAM(airfighterSetup, float, targetDistance);
				READ_PARAM(airfighterSetup, float, flyByAttackDistance);
				READ_PARAM(airfighterSetup, float, preferredDistance);
				READ_PARAM(airfighterSetup, float, preferredAltitude);
				READ_PARAM(airfighterSetup, float, preferredApproachAltitude);
				READ_PARAM(airfighterSetup, float, preferredAwayAltitude);
				READ_PARAM(airfighterSetup, float, limitFollowRelVelocity);
				READ_PARAM(airfighterSetup, float, breakOffDelay);
				READ_PARAM(airfighterSetup, float, breakOffYaw);
				READ_PARAM(airfighterSetup, Vector3, followOffset);
				READ_PARAM(airfighterSetup, bool, flyByAttack);
			});
		messageHandler.set(NAME(aim_attack), [this](Framework::AI::Message const& _message)
			{
				beActiveAttack = true;
				if (auto* ptr = _message.get_param(NAME(targetIMO)))
				{
					auto targetIMOPtr = ptr->get_as< SafePtr<Framework::IModulesOwner> >();
					currentTarget.clear();
					for (int tiIdx = 0; tiIdx < targetList.get_size(); ++ tiIdx)
					{
						auto& ti = targetList[tiIdx];
						if (ti.target == targetIMOPtr.get())
						{
							targetList.remove_at(tiIdx);
							--tiIdx;
						}
					}
					TargetInfo ti;
					ti.target = targetIMOPtr.get();
					if (auto* atiPtr = _message.get_param(NAME(attackLength)))
					{
						ti.attackLength = atiPtr->get_as<float>();
					}
					if (auto* atiPtr = _message.get_param(NAME(attackDeadLimit)))
					{
						ti.attackDeadLimit = atiPtr->get_as<float>();
					}
					targetList.push_front(ti);
				}
				READ_PARAM(airfighterSetup, float, attackDistance);
				READ_PARAM(airfighterSetup, float, readyToAttackDistance);
				READ_PARAM(airfighterSetup, float, targetDistance);
				READ_PARAM(airfighterSetup, float, breakOffDelay);
				READ_PARAM(airfighterSetup, float, breakOffYaw);
			});
		messageHandler.set(NAME(aim_attackStop), [this](Framework::AI::Message const& _message)
			{
				beActiveAttack = false;
			});
	}
}

Airfighter::~Airfighter()
{
}

void Airfighter::advance(float _deltaTime)
{
	if (shooterWaveActive)
	{
		if (airfighterSetup.allowFollowOffsetAdditionalRange)
		{
			float off = 0.2f;
			followOffsetAdditionalRange = Range3(Range(-off, off), Range(-off, off), Range(-off, off));
			followOffsetAdditionalInterval = Range(0.4f, 0.9f);
		}
		else
		{
			followOffsetAdditionalRange.clear();
			followOffsetAdditionalInterval.clear();
		}

		currentTarget = shooterWaveSetup.targetIMO;
		currentTargetPlacementSocket = shooterWaveSetup.targetPlacementSocket.get(Name::invalid());

		followIMO = shooterWaveSetup.followIMO;
		matchYaw.clear();
		matchPitch.clear();
		followRelVelocity.clear();
		noLocationGoForward = false;
		if (shooterWaveSetup.followIMO.is_set())
		{
			followOffset = shooterWaveSetup.moveTo;
			matchLocation.clear();
			if (shooterWaveSetup.followRelVelocity.is_set())
			{
				followRelVelocity = shooterWaveSetup.followRelVelocity.get();
			}
		}
		else
		{
			followOffset.clear();
			if (shooterWaveSetup.moveDir.is_set())
			{
				Rotator3 moveDirRot = shooterWaveSetup.moveDir.get().to_rotator();
				matchYaw = moveDirRot.yaw;
				matchPitch = moveDirRot.pitch;
				noLocationGoForward = true;
			}
			else
			{
				matchLocation = shooterWaveSetup.moveTo;
			}
		}
		limitFollowRelVelocity = shooterWaveSetup.limitFollowRelVelocity;

		if (shooterWaveActiveStart)
		{
			shooterWaveActiveStart = false;

			instantFollowing = true;
		}

		if (shooterWaveSetup.readyToAttack.is_set())
		{
			if (!readyToAttack && shooterWaveSetup.readyToAttack.get())
			{
				arms.be_armed(NP, true);
				arms.target(currentTarget.get(), currentTargetPlacementSocket);
				readyToAttack = true;
			}
			if (shooterWaveSetup.shootStartOffsetTS.is_set() &&
				! shooterWaveSetup.shootStartOffsetTS.get().is_zero())
			{
				float pt = clamp(attackTime / max(0.1f, shooterWaveSetup.shootStartOffsetPt.get(1.0f) * shooterWaveSetup.attackLength.get(0.0f)), 0.0f, 1.0f);
				arms.target(currentTarget.get(), currentTargetPlacementSocket, shooterWaveSetup.shootStartOffsetTS.get() * (1.0f - pt));
			}
			if (readyToAttack && ! shooterWaveSetup.readyToAttack.get())
			{
				arms.fold_arms();
				readyToAttack = false;
			}
		}

		{
			bool shouldAttack = false;
			if (!dying)
			{
				if (shooterWaveSetup.attack.is_set())
				{
					shouldAttack |= shooterWaveSetup.attack.get();
				}
			}
			{
				if (!attack && shouldAttack)
				{
					arms.shooting(true);
					attack = true;
				}
				if (attack && !shouldAttack)
				{
					arms.shooting(false);
					attack = false;
				}
			}
		}
	}

	if (attack)
	{
		attackTime += _deltaTime;
	}
	else
	{
		attackTime = 0.0f;
		attackDeadTime = 0.0f;
	}

	base::advance(_deltaTime);

	if (beActive || beActiveAttack)
	{
		an_assert(!shooterWaveActive);
		if (attack)
		{
			if (currentTarget.is_set() && !targetList.is_empty() &&
				targetList.get_first().target == currentTarget &&
				targetList.get_first().attackLength.is_set() &&
				attackTime >= targetList.get_first().attackLength.get())
			{
				currentTarget.clear();
				targetList.pop_front();
			}
		}
		if (!CustomModules::Health::does_exists_and_is_alive(currentTarget.get()))
		{
			attackDeadTime += _deltaTime;
			bool done = true;
			if (! targetList.is_empty() && targetList.get_first().target == currentTarget)
			{
				if (targetList.get_first().attackDeadLimit.is_set())
				{
					done = ! attack || attackDeadTime > targetList.get_first().attackDeadLimit.get();
				}
			}
			if (done)
			{
				currentTarget.clear();
				state = State::None;
				if (beActive)
				{
					if (airfighterSetup.allowFollowOffsetAdditionalRange)
					{
						float off = 0.2f;
						followOffsetAdditionalRange = Range3(Range(-off, off), Range(-off, off), Range(-off, off));
						followOffsetAdditionalInterval = Range(0.4f, 0.9f);
					}
					else
					{
						followOffsetAdditionalRange.clear();
						followOffsetAdditionalInterval.clear();
					}
				}
				if (!targetList.is_empty())
				{
					currentTarget = targetList.get_first().target.get();
					attackTime = 0.0f;
					if (!CustomModules::Health::does_exists_and_is_alive(currentTarget.get()))
					{
						currentTarget.clear();
						targetList.pop_front();
					}
				}
			}
		}
		bool shouldBeReadyToAttack = false;
		bool shouldAttack = false;

		if (beActive)
		{
			noLocationGoForward = false;
		}
		if (currentTarget.get())
		{
			if (auto* imo = get_mind()->get_owner_as_modules_owner())
			{
				if (imo->get_presence()->get_in_room() == currentTarget->get_presence()->get_in_room())
				{
					Vector3 loc = imo->get_presence()->get_placement().get_translation();
					Vector3 targetLoc = currentTarget->get_presence()->get_placement().get_translation();
					float distance = (imo->get_presence()->get_centre_of_presence_WS() - currentTarget->get_presence()->get_centre_of_presence_WS()).length_2d();
					Vector3 targetRelLoc = imo->get_presence()->get_placement().location_to_local(currentTarget->get_presence()->get_centre_of_presence_WS());
					Vector3 relToTargetLoc = currentTarget->get_presence()->get_placement().location_to_local(loc);

					shouldBeReadyToAttack = readyToAttack;
					if (beActive)
					{
						if (airfighterSetup.flyByAttack)
						{
							if (state == State::None)
							{
								state = State::FlyByApproach;

								//
								followIMO = airfighterSetup.useFollowIMO ? airfighterSetup.followIMO : currentTarget;
								followOffset = airfighterSetup.followOffset.get(Vector3(0.0f, 0.0f, airfighterSetup.preferredAltitude.get(4.0f)));
								followOffset.access().z = airfighterSetup.preferredApproachAltitude.get(followOffset.access().z * 0.2f + 0.8f * relToTargetLoc.z);
								followRelVelocity = (targetLoc - loc).normal_2d() * setup.transit.linSpeed;
								limitFollowRelVelocity.clear();
							}
							if (state == State::FlyByApproach)
							{
								if (distance < airfighterSetup.flyByAttackDistance.get(60.0f))
								{
									state = State::FlyByAttack;

									//
									followIMO = airfighterSetup.useFollowIMO ? airfighterSetup.followIMO : currentTarget;
									followOffset = airfighterSetup.followOffset.get(Vector3(0.0f, 0.0f, airfighterSetup.preferredAltitude.get(4.0f)));
									followRelVelocity = (targetLoc - loc).normal_2d() * airfighterSetup.limitFollowRelVelocity.get(setup.transit.linSpeed);
									limitFollowRelVelocity = airfighterSetup.limitFollowRelVelocity;
								}
							}
							if (state == State::FlyByAttack)
							{
								if (targetRelLoc.y < 0.5f)
								{
									state = State::FlyAway;
									readyToAttack = false;
									shouldBeReadyToAttack = false;
									arms.fold_arms();

									//
									followIMO = airfighterSetup.useFollowIMO ? airfighterSetup.followIMO : currentTarget;
									// keep offset
									followOffset.access().z = airfighterSetup.preferredAwayAltitude.get(followOffset.access().z);
									followRelVelocity = followRelVelocity.get().normal() * setup.transit.linSpeed;
									limitFollowRelVelocity.clear();
								}
								else
								{
									shouldBeReadyToAttack = true;
								}
							}
						}
						else
						{
							if (state == State::None)
							{
								state = State::RemainClose;

								//
								followIMO = airfighterSetup.useFollowIMO ? airfighterSetup.followIMO : currentTarget;
								followOffset = airfighterSetup.followOffset.get(Vector3(0.0f, 0.0f, airfighterSetup.preferredAltitude.get(2.0f)) + relToTargetLoc.normal_2d() * airfighterSetup.preferredDistance.get(4.0f));
								limitFollowRelVelocity = airfighterSetup.limitFollowRelVelocity;
								requestedDistance = followOffset.get().length_2d() - 2.0f;
							}
							if (state == State::RemainClose)
							{
								shouldBeReadyToAttack = true;
							}
						}
					}
					else
					{
						shouldBeReadyToAttack = true;
					}

					if (shouldBeReadyToAttack && distance > airfighterSetup.readyToAttackDistance.get(40.0f))
					{
						shouldBeReadyToAttack = false;
					}
					if (readyToAttack && distance < airfighterSetup.targetDistance.get(40.0f))
					{
						arms.target(currentTarget.get(), currentTargetPlacementSocket);
					}
					if (readyToAttack && distance < airfighterSetup.attackDistance.get(30.0f))
					{
						shouldAttack = true;
					}
				}
			}
		}
		else if (beActive)
		{
			if (targetList.is_empty())
			{
				if (airfighterSetup.breakOffDelay.get(0.0f) > 0.0f)
				{
					airfighterSetup.breakOffDelay = airfighterSetup.breakOffDelay.get() - _deltaTime;
				}
				else
				{
					if (!breakingOff)
					{
						breakingOff = true;
						if (auto* imo = get_mind()->get_owner_as_modules_owner())
						{
							MODULE_OWNER_LOCK_FOR_IMO(imo, TXT("set breaking off"));
							imo->access_variables().access<bool>(NAME(breakingOff)) = true;
						}
					}
					// break off - move fast forward or in a specific direction
					followIMO.clear();
					matchYaw = airfighterSetup.breakOffYaw;
					matchPitch.clear();
					matchLocation.clear();
					followRelVelocity.clear();
					limitFollowRelVelocity.clear();
					noLocationGoForward = true;
				}
			}
		}

		if (shouldBeReadyToAttack && !readyToAttack)
		{
			readyToAttack = true;
			arms.be_armed();
		}
		if (!shouldBeReadyToAttack && readyToAttack)
		{
			arms.fold_arms();
			readyToAttack = false;
		}

		if (dying)
		{
			shouldAttack = false;
		}
		{
			if (!attack && shouldAttack)
			{
				arms.shooting(true);
				attack = true;
			}
			if (attack && !shouldAttack)
			{
				arms.shooting(false);
				attack = false;
			}
		}
	}

	if (! dying)
	{
		AirfighterArms::AdvanceParams params;
		params.with_delta_time(_deltaTime);
		params.with_owner(get_mind()->get_owner_as_modules_owner());
		params.with_placement_WS(get_placement());
		params.with_velocity_lineary(get_velocity_linear());
		params.with_velocity_rotation(get_velocity_rotation());
		if (params.owner)
		{
			if (auto* r = params.owner->get_presence()->get_in_room())
			{
				params.with_room_velocity(r->get_room_velocity());
			}
		}
		arms.advance(params);
	}
}

//

REGISTER_FOR_FAST_CAST(AirfighterData);

bool AirfighterData::load_from_xml(IO::XML::Node const* _node, ::Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("arms")))
	{
		result &= armsData.load_from_xml(node, _lc);
	}

	return result;
}

bool AirfighterData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= armsData.prepare_for_game(_library, _pfgContext);

	return result;
}

