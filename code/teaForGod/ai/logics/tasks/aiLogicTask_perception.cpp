#include "aiLogicTask_perception.h"

#include "..\aiLogic_npcBase.h"

#include "..\components\aiLogicComponent_confussion.h"

#include "..\..\aiCommonVariables.h"
#include "..\..\aiRayCasts.h"
#include "..\..\perceptionRequests\aipr_FindInVisibleRooms.h"

#include "..\..\..\game\gameDirector.h"
#include "..\..\..\game\gameSettings.h"

#include "..\..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\..\modules\custom\mc_switchSidesHandler.h"
#include "..\..\..\modules\custom\health\mc_health.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\..\music\musicPlayer.h"

#include "..\..\..\..\framework\ai\aiLatentTask.h"
#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\..\framework\ai\aiPerceptionRequest.h"
#include "..\..\..\..\framework\ai\aiSocial.h"
#include "..\..\..\..\framework\game\gameUtils.h"
#include "..\..\..\..\framework\general\cooldowns.h"
#include "..\..\..\..\framework\module\moduleAI.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\module\moduleSound.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\object\actor.h"
#include "..\..\..\..\framework\presence\pathUtils.h"
#include "..\..\..\..\framework\presence\presencePath.h"
#include "..\..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\..\framework\world\doorInRoom.h"
#include "..\..\..\..\framework\world\inRoomPlacement.h"
#include "..\..\..\..\framework\world\room.h"
#include "..\..\..\..\framework\world\world.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_USE_AI_LOG
#define INVESTIGATE_ALERT
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// ai hunch names
DEFINE_STATIC_NAME(investigate);

// ai message names
DEFINE_STATIC_NAME(attract);
DEFINE_STATIC_NAME(dealtDamage);
DEFINE_STATIC_NAME(sound);
DEFINE_STATIC_NAME(alert); // also sound/temporary object
DEFINE_STATIC_NAME(confussion);

// ai hunch/message params
DEFINE_STATIC_NAME(damageSource);
DEFINE_STATIC_NAME(location);
DEFINE_STATIC_NAME(propagationDir);
DEFINE_STATIC_NAME(throughDoor);
DEFINE_STATIC_NAME(room);
DEFINE_STATIC_NAME(who);
DEFINE_STATIC_NAME(target);

// task feedback
//DEFINE_STATIC_NAME(taskFeedback);
//DEFINE_STATIC_NAME(enemyLost);

// cooldowns
DEFINE_STATIC_NAME(simplifyEnemyPath);
DEFINE_STATIC_NAME(enemyProposition);
DEFINE_STATIC_NAME(investigateEnemy);
DEFINE_STATIC_NAME(investigateNoEnemy);
DEFINE_STATIC_NAME(updateEnemySeen);
DEFINE_STATIC_NAME(changeEnemyPropositionIntoEnemy);

// investigate reasons
DEFINE_STATIC_NAME(spawnManager);

//

static void update_offsets(REF_ Vector3 & enemyCentreOffsetOS, REF_ Vector3 & enemyTargetingOffsetOS, Framework::IModulesOwner* imo, Framework::RelativeToPresencePlacement const & enemyPlacement)
{
	if (enemyPlacement.get_target())
	{
		enemyCentreOffsetOS = enemyPlacement.get_target()->get_presence()->get_centre_of_presence_os().get_translation();
		if (auto* ai = imo->get_ai())
		{
			enemyTargetingOffsetOS = ai->get_target_placement_os_for(enemyPlacement.get_target()).get_translation();
		}
		else
		{
			enemyTargetingOffsetOS = Vector3::zero;
		}
	}
	else
	{
		// as wa...?
	}
}

static bool is_imo_moving(Framework::IModulesOwner* imo)
{
	if (!imo)
	{
		return false;
	}
	auto* p = imo->get_presence();
	if (p->get_velocity_linear().length() > 0.1f ||
		p->get_velocity_rotation().length() > 10.0f)
	{
		return true;
	}
	if (auto* mp = imo->get_gameplay_as<ModulePilgrim>())
	{
		for_count(int, hIdx, Hand::MAX)
		{
			if (auto* himo = mp->get_hand((Hand::Type)hIdx))
			{
				auto* p = himo->get_presence();
				if (p->get_velocity_linear().length() > 0.1f ||
					p->get_velocity_rotation().length() > 10.0f)
				{
					return true;
				}
			}
		}
		if (mp->is_something_moving())
		{
			return true;
		}
	}
	if (auto* h = imo->get_custom<CustomModules::Health>())
	{
		if (h->has_any_extra_effect())
		{
			return true;
		}
	}
	return false;
}

// we have it here as otherwise we would miss the first alert (as the variable would be just created
Concurrency::SpinLock s_preceptionAlertAccessLock;
::System::TimeStamp s_preceptionAlertTimeSinceLastAlert; // to avoid spam

void TeaForGodEmperor::AI::Logics::issue_perception_alert_ai_message(Framework::IModulesOwner* _owner, Framework::IModulesOwner* _target, Framework::RelativeToPresencePlacement const & _placement)
{
#ifdef INVESTIGATE_ALERT
	ai_log(_owner->get_ai()->get_mind()->get_logic(), TXT("[perception] issue alert?"));
#endif
	if (!_placement.is_active() || !_owner)
	{
#ifdef INVESTIGATE_ALERT
		ai_log(_owner->get_ai()->get_mind()->get_logic(), TXT("[perception] no placement, no owner"));
#endif
		return;
	}
	if (auto* room = _placement.get_in_final_room())
	{
		{
			Concurrency::ScopedSpinLock lock(s_preceptionAlertAccessLock);
			if (s_preceptionAlertTimeSinceLastAlert.get_time_since() < 5.0f)
			{
#ifdef INVESTIGATE_ALERT
				ai_log(_owner->get_ai()->get_mind()->get_logic(), TXT("[perception] alert too often, skip"));
#endif
				return;
			}
			s_preceptionAlertTimeSinceLastAlert.reset();
		}
	
		if (auto* message = _owner->get_in_world()->create_ai_message(NAME(alert)))
		{
#ifdef INVESTIGATE_ALERT
			ai_log(_owner->get_ai()->get_mind()->get_logic(), TXT("[perception] alert!"));
#endif
			message->delayed(0.25f); // will also have some delay with actual response
			message->access_param(NAME(who)).access_as<SafePtr<Framework::IModulesOwner>>() = _owner->get_top_instigator();
			message->access_param(NAME(room)).access_as<SafePtr<Framework::Room>>() = room;
			message->access_param(NAME(location)).access_as<Vector3>() = _placement.get_placement_in_final_room().location_to_world(_target ? _target->get_presence()->get_centre_of_presence_os().get_translation() : Vector3::zero);
			message->to_all_in_range(_owner->get_presence()->get_in_room(), _owner->get_presence()->get_centre_of_presence_WS(), 20.0f);
		}
	}
}

//

LATENT_FUNCTION(TeaForGodEmperor::AI::Logics::Tasks::perception)
{
	/**
	 *	Perception task
	 *	Focuses on finding enemy and provides assumed location
	 */
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("perception"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(bool, perceptionStartOff);
	LATENT_COMMON_VAR(bool, perceptionSightImpaired);
	LATENT_COMMON_VAR(bool, perceptionNoSight);
	LATENT_COMMON_VAR(bool, perceptionSightMovementBased);
	LATENT_COMMON_VAR(bool, perceptionNoHearing);
	LATENT_COMMON_VAR(PerceptionPause, perceptionPaused);
	LATENT_COMMON_VAR(PerceptionLock, perceptionEnemyLock);
	LATENT_COMMON_VAR(SafePtr<Framework::IModulesOwner>, enemy);
	LATENT_COMMON_VAR(Framework::RelativeToPresencePlacement, enemyPlacement); // where we think the enemy is
	LATENT_COMMON_VAR(Vector3, defaultTargetingOffsetOS);
	LATENT_COMMON_VAR(Vector3, enemyTargetingOffsetOS);
	LATENT_COMMON_VAR(Vector3, enemyCentreOffsetOS);
	LATENT_COMMON_VAR(Framework::PresencePath, enemyPath); // where enemy actually is
	LATENT_COMMON_VAR(Framework::InRoomPlacement, investigate);
	LATENT_COMMON_VAR(int, investigateIdx);
	LATENT_COMMON_VAR(bool, investigateGlance);
	LATENT_COMMON_VAR(bool, encouragedInvestigate);
	LATENT_COMMON_VAR(bool, forceInvestigate);
	LATENT_COMMON_VAR(Name, forceInvestigateReason);
	LATENT_COMMON_VAR(bool, investigateDir);
	LATENT_COMMON_VAR(bool, ignoreViolenceDisallowed);
	LATENT_COMMON_VAR_END();

	LATENT_VAR(Framework::RelativeToPresencePlacement, enemyPlacementLastValid); // it is updated only if we are sure we see the enemy, it is used to point at last known location of the enemy

	LATENT_VAR(SafePtr<Framework::IModulesOwner>, queuedEnemy);

	LATENT_VAR(Framework::PresencePath, enemyProposition);
	LATENT_VAR(Framework::CooldownsSmall, cooldowns);
	LATENT_VAR(int, doesntSeeCount);

	LATENT_VAR(::Framework::AI::PerceptionRequestPtr, perceptionRequest); // one perception should be enough

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	LATENT_VAR(float, clearInvestigateTimeLeft);
	LATENT_VAR(float, clearInvestigateGlanceTimeLeft);

	LATENT_VAR(CustomModules::SwitchSidesHandler*, switchSidesHandler);
	LATENT_VAR(SafePtr<Framework::IModulesOwner>, prevSwitchSidesTo);

	LATENT_VAR(bool, narrativeSafeRequested);
	LATENT_VAR(bool, ignoreEnemiesNow);
	
	LATENT_VAR(Confussion, confussion);
		
	typedef RemapValue<float, float> RemapValueFloatFloat;
	LATENT_VAR(RemapValueFloatFloat, adjustWaitTime);
	
	auto* imo = mind->get_owner_as_modules_owner();
	auto* npcBase = fast_cast<NPCBase>(mind->get_logic());

	if (clearInvestigateTimeLeft > 0.0f)
	{
		clearInvestigateTimeLeft -= LATENT_DELTA_TIME;
		if (clearInvestigateTimeLeft <= 0.0f)
		{
			ai_log_colour(mind->get_logic(), Colour::cyan);
			ai_log(mind->get_logic(), TXT("[perception] clear investigate"));
			ai_log_no_colour(mind->get_logic());
			investigate.clear();
			++investigateIdx;
			investigateGlance = false;
			encouragedInvestigate = false;
			forceInvestigate = false;
			forceInvestigateReason = Name::invalid();
			investigateDir = false;
		}
	}

	if (clearInvestigateGlanceTimeLeft > 0.0f)
	{
		clearInvestigateGlanceTimeLeft -= LATENT_DELTA_TIME;
		if (clearInvestigateGlanceTimeLeft <= 0.0f)
		{
			ai_log_colour(mind->get_logic(), Colour::cyan);
			ai_log(mind->get_logic(), TXT("[perception] clear investigate glance"));
			ai_log_no_colour(mind->get_logic());
			investigateGlance = false;
			encouragedInvestigate = false;
		}
	}

	confussion.advance(LATENT_DELTA_TIME);

	cooldowns.advance(LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	// react quickly when very close
	adjustWaitTime.clear();
	adjustWaitTime.add(2.0f, 0.1f);
	adjustWaitTime.add(5.0f, 1.0f);
	adjustWaitTime.add(10.0f, 3.0f);

#ifdef AN_DEVELOPMENT_OR_PROFILER
	enemyPlacement.usageInfo = Name(TXT("perception; enemyPlacement"));
	enemyPlacementLastValid.usageInfo = Name(TXT("perception; enemyPlacementLastValid"));
#endif

	an_assert(npcBase);

	switchSidesHandler = imo->get_custom<CustomModules::SwitchSidesHandler>();

	confussion.setup(mind);

	defaultTargetingOffsetOS = Vector3::zAxis * hardcoded magic_number 1.6f; 
	enemyTargetingOffsetOS = Vector3::zero;
	enemyCentreOffsetOS = Vector3::zero;

	if (npcBase && npcBase->should_always_look_for_new_enemy())
	{
		perceptionEnemyLock.set_base_lock(AI::PerceptionLock::LookForNew); // always look for a new one
	}

	messageHandler.use_with(mind);
	{
		auto * framePtr = &_frame;
		messageHandler.set(NAME(dealtDamage), [framePtr, mind, &perceptionStartOff, &perceptionPaused, &perceptionEnemyLock, &enemy, &enemyPath, &investigate, &investigateIdx,
			&enemyPlacement, &enemyPlacementLastValid, &queuedEnemy, &ignoreViolenceDisallowed](Framework::AI::Message const & _message)
		{
			if (perceptionPaused.is_paused())
			{
				// ignore
				return;
			}
			perceptionStartOff = false;
			ignoreViolenceDisallowed = true; // always on damage
			if (auto * source = _message.get_param(NAME(damageSource)))
			{
				if (auto* damageInstigator = source->get_as<SafePtr<Framework::IModulesOwner>>().get())
				{
					ai_log_colour(mind->get_logic(), Colour::cyan);
					ai_log(mind->get_logic(), TXT("[perception] damage source (%S)"), damageInstigator->ai_get_name().to_char());
					ai_log_no_colour(mind->get_logic());
					damageInstigator = damageInstigator->get_instigator_first_actor_or_valid_top_instigator();

					auto* imo = mind->get_owner_as_modules_owner();
					if (damageInstigator == imo)
					{
						ai_log(mind->get_logic(), TXT("[perception] oh, we damaged ourselves, ignore"));
					}
					else
					{
						if (auto* ai = imo->get_ai())
						{
							auto& social = ai->get_mind()->get_social();
							if (social.is_never_against_allies())
							{
								if (social.is_ally_or_owned_by_ally(damageInstigator))
								{
									ai_log(mind->get_logic(), TXT("[perception] won't attack a friend"));
									damageInstigator = nullptr;
								}
							}
						}
						if (damageInstigator)
						{
							if (enemy.get() != damageInstigator)
							{
								ai_log_colour(mind->get_logic(), Colour::cyan);
								if ((!enemy.get() || perceptionEnemyLock.get() != PerceptionLock::Lock))
								{
									ai_log_colour(mind->get_logic(), Colour::cyan);
									ai_log(mind->get_logic(), TXT("[perception] perception, someone damaged us, he is our enemy now (%S)"), damageInstigator->ai_get_name().to_char());
									ai_log_no_colour(mind->get_logic());
									enemy = damageInstigator;
									enemyPath.find_path(mind->get_owner_as_modules_owner(), enemy.get(), true);
									enemyPlacement.clear_target();
									enemyPlacementLastValid.reset();
									queuedEnemy.clear();
								}
								else if (queuedEnemy.get() != damageInstigator)
								{
									ai_log_colour(mind->get_logic(), Colour::cyan);
									ai_log(mind->get_logic(), TXT("[perception] perception, someone damaged us, queue him (%S)"), damageInstigator->ai_get_name().to_char());
									ai_log_no_colour(mind->get_logic());
									queuedEnemy = damageInstigator;
								}
								ai_log_no_colour(mind->get_logic());
							}
							if (enemy.get() == damageInstigator)
							{
								if (!enemyPlacement.is_active() ||
									!enemyPlacement.get_target())
								{
									ai_log_colour(mind->get_logic(), Colour::cyan);
									ai_log(mind->get_logic(), TXT("[perception] perception, enemy damaged us, look at the enemy, stop investigating (%S)"), damageInstigator->ai_get_name().to_char());
									ai_log_no_colour(mind->get_logic());
									enemyPlacement = enemyPath;
									queuedEnemy.clear();
									investigate.clear();
									++investigateIdx;
									enemyPlacementLastValid.reset();
									if (auto* s = mind->get_owner_as_modules_owner()->get_sound())
									{
										if (s->play_sound(NAME(alert)))
										{
											if (Framework::GameUtils::is_controlled_by_player(enemy.get()))
											{
												MusicPlayer::request_combat_auto_indicate_presence();

												issue_perception_alert_ai_message(mind->get_owner_as_modules_owner(), enemy.get(), enemyPlacement);
											}
										}
									}
								}
							}
						}
					}
				}
			}
			framePtr->end_waiting();
			AI_LATENT_STOP_LONG_RARE_ADVANCE();
		}
		);
		messageHandler.set(NAME(alert), [framePtr, mind, imo, &perceptionStartOff, &perceptionPaused, &enemyPlacement, &investigate, & investigateIdx,
			&investigateGlance, &encouragedInvestigate, &forceInvestigate, &forceInvestigateReason, &investigateDir, &clearInvestigateTimeLeft, &clearInvestigateGlanceTimeLeft](Framework::AI::Message const & _message)
		{
#ifdef INVESTIGATE_ALERT
			ai_log(mind->get_logic(), TXT("[perception] (alerted) process?"));
#endif
			if (perceptionPaused.is_paused())
			{
#ifdef INVESTIGATE_ALERT
				ai_log(mind->get_logic(), TXT("[perception] (alerted) ignore, paused"));
#endif
				// ignore
				return;
			}
			perceptionStartOff = false;
			if (!enemyPlacement.get_target())
			{
#ifdef INVESTIGATE_ALERT
				ai_log(mind->get_logic(), TXT("[perception] (alerted) no enemy placement"));
#endif
				if (auto* whoParam = _message.get_param(NAME(who)))
				{
					if (auto* who = whoParam->get_as<SafePtr<Framework::IModulesOwner>>().get())
					{
						who = fast_cast<Framework::IModulesOwner>(cast_to_nonconst(who->ai_instigator()));
						if (auto* ai = imo->get_ai())
						{
							if (!ai->get_mind()->get_social().is_enemy(who))
							{
								bool isOk = true;
								if (auto* targetParam = _message.get_param(NAME(target)))
								{
									if (auto* target = targetParam->get_as<SafePtr<Framework::IModulesOwner>>().get())
									{
										target = fast_cast<Framework::IModulesOwner>(cast_to_nonconst(target->ai_instigator()));
										if (!ai->get_mind()->get_social().is_enemy(target))
										{
											isOk = false;
										}
									}
								}
								if (isOk)
								{
									auto* roomParam = _message.get_param(NAME(room));
									auto* locationParam = _message.get_param(NAME(location));
									investigate = Framework::InRoomPlacement(roomParam->get_as<SafePtr<Framework::Room>>().get(), locationParam->get_as<Vector3>());
									++investigateIdx;
									investigateDir = false;
									clearInvestigateTimeLeft = Random::get_float(5.0f, 20.0f);
									encouragedInvestigate = roomParam->get_as<SafePtr<Framework::Room>>().get() == imo->get_presence()->get_in_room();
									float glanceChance = 1.0f;
									if (auto* npcBase = fast_cast<NPCBase>(mind->get_logic()))
									{
										glanceChance = npcBase->get_glance_chance();
									}
									if (Random::get_chance(glanceChance))
									{
										investigateGlance = imo->get_presence()->get_in_room()->was_recently_seen_by_player() && !enemyPlacement.is_active();
										clearInvestigateGlanceTimeLeft = Random::get_float(2.0f, 3.0f);
									}
									else
									{
										investigateGlance = false;
									}
									forceInvestigate = false;
									forceInvestigateReason = Name::invalid();
#ifdef AN_USE_AI_LOG
									if (investigate.is_valid())
									{
										ai_log_colour(mind->get_logic(), Colour::cyan);
										ai_log(mind->get_logic(), TXT("[perception] (alerted) investigate at %S in %S"), investigate.placement.get_translation().to_string().to_char(), investigate.inRoom.get()->get_name().to_char());
										ai_log_no_colour(mind->get_logic());
									}
#endif
									framePtr->end_waiting();
									AI_LATENT_STOP_LONG_RARE_ADVANCE();
								}
							}
						}
					}
				}
			}
		}
		);
		messageHandler.set(NAME(sound), [framePtr, mind, imo,
			&perceptionStartOff, &perceptionNoHearing, &perceptionPaused, &enemy, &investigate, & investigateIdx,
			&enemyPlacement, &investigateGlance, &encouragedInvestigate, &forceInvestigate, &forceInvestigateReason , &investigateDir, &clearInvestigateTimeLeft, &clearInvestigateGlanceTimeLeft](Framework::AI::Message const & _message)
		{
			if (perceptionPaused.is_paused() || perceptionNoHearing)
			{
				// ignore
				return;
			}
			perceptionStartOff = false;
			if (auto * whoParam = _message.get_param(NAME(who)))
			{
				if (auto * who = whoParam->get_as<SafePtr<Framework::IModulesOwner>>().get())
				{
					who = fast_cast<Framework::IModulesOwner>(cast_to_nonconst(who->ai_instigator()));
					if (auto* ai = imo->get_ai())
					{
						if (enemy.get() == who ||
							(!enemy.is_set() && ai->get_mind()->get_social().is_enemy(who)))
						{
							auto* roomParam = _message.get_param(NAME(room));
							auto* locationParam = _message.get_param(NAME(location));
							auto* propagationDirParam = _message.get_param(NAME(propagationDir));
							investigate = Framework::InRoomPlacement(roomParam->get_as<SafePtr<Framework::Room>>().get(), locationParam->get_as<Vector3>());
							++investigateIdx;
							investigateDir = false;
							if (auto* throughDoorParam = _message.get_param(NAME(throughDoor)))
							{
								if (auto* throughDoor = throughDoorParam->get_as<SafePtr<Framework::DoorInRoom>>().get())
								{
									ai_log(mind->get_logic(), TXT("[perception] heard a sound choose to investigate through door"));
									Vector3 loc = throughDoor->get_hole_centre_WS();
									float usePropagationDir = clamp(1.0f - (loc - imo->get_presence()->get_centre_of_presence_WS()).length() / 2.0f, 0.0f, 1.0f);
									Vector3 propagationDir = propagationDirParam ? propagationDirParam->get_as<Vector3>() : Vector3::zero;
									loc += 5.0f * ((usePropagationDir * -propagationDir) + (1.0f - usePropagationDir) * throughDoor->get_outbound_matrix().vector_to_world(Vector3::yAxis));
									investigate = Framework::InRoomPlacement(throughDoor->get_in_room(), loc);
									investigateDir = true;
								}
							}
							clearInvestigateTimeLeft = Random::get_float(5.0f, 20.0f);
							encouragedInvestigate = roomParam->get_as<SafePtr<Framework::Room>>().get() == imo->get_presence()->get_in_room();
							float glanceChance = 1.0f;
							if (auto* npcBase = fast_cast<NPCBase>(mind->get_logic()))
							{
								glanceChance = npcBase->get_glance_chance();
							}
							if (Random::get_chance(glanceChance))
							{
								investigateGlance = imo->get_presence()->get_in_room()->was_recently_seen_by_player() && !enemyPlacement.is_active();
								clearInvestigateGlanceTimeLeft = Random::get_float(2.0f, 3.0f);
							}
							else
							{
								investigateGlance = false;
							}
							clearInvestigateGlanceTimeLeft = Random::get_float(2.0f, 3.0f);
							forceInvestigate = false;
							forceInvestigateReason = Name::invalid();
#ifdef AN_USE_AI_LOG
							if (investigate.is_valid())
							{
								ai_log_colour(mind->get_logic(), Colour::cyan);
								ai_log(mind->get_logic(), TXT("[perception] (heard) investigate at %S in %S"), investigate.placement.get_translation().to_string().to_char(), investigate.inRoom.get()->get_name().to_char());
								ai_log_no_colour(mind->get_logic());
							}
#endif
							framePtr->end_waiting();
							AI_LATENT_STOP_LONG_RARE_ADVANCE();
						}
					}
				}
			}
		}
		);
		messageHandler.set(NAME(attract), [framePtr, mind, imo,
			&perceptionStartOff, &perceptionNoHearing, &perceptionPaused, &enemy, & enemyPath, &enemyPlacement, &enemyPlacementLastValid, &queuedEnemy,
			&investigate, &investigateIdx, &investigateGlance, &encouragedInvestigate, &forceInvestigate, &forceInvestigateReason, &investigateDir,
			&clearInvestigateTimeLeft, &clearInvestigateGlanceTimeLeft, &ignoreViolenceDisallowed](Framework::AI::Message const & _message)
		{
			if (perceptionPaused.is_paused() || perceptionNoHearing)
			{
				// ignore
				return;
			}
			perceptionStartOff = false;
			if (auto * whoParam = _message.get_param(NAME(who)))
			{
				if (auto * who = whoParam->get_as<SafePtr<Framework::IModulesOwner>>().get())
				{
					who = fast_cast<Framework::IModulesOwner>(cast_to_nonconst(who->ai_instigator()));
					if (auto * ai = imo->get_ai())
					{
						if (ai->get_mind()->get_social().is_enemy(who))
						{
							if (enemy.is_set() && enemy.get() != who &&
								Random::get_chance(0.5f))
							{
								ai_log(mind->get_logic(), TXT("[perception] switch enemy to attract"));
								enemy = who;
								enemyPath.find_path(mind->get_owner_as_modules_owner(), enemy.get(), true);
								enemyPlacement.clear_target();
								enemyPlacementLastValid.reset();
								queuedEnemy.clear();
							}
						}
						if (!enemy.is_set() || enemy.get() == who)
						{
							auto* roomParam = _message.get_param(NAME(room));
							auto* locationParam = _message.get_param(NAME(location));
							++investigateIdx;
							investigate = Framework::InRoomPlacement(roomParam->get_as<SafePtr<Framework::Room>>().get(), locationParam->get_as<Vector3>());
							investigateDir = false;
							if (auto * throughDoorParam = _message.get_param(NAME(throughDoor)))
							{
								if (auto * throughDoor = throughDoorParam->get_as<SafePtr<Framework::DoorInRoom>>().get())
								{
									ai_log(mind->get_logic(), TXT("[perception] choose to investigate through door (attract)"));
									investigate = Framework::InRoomPlacement(throughDoor->get_in_room(), throughDoor->get_hole_centre_WS() + throughDoor->get_outbound_matrix().vector_to_world(Vector3::yAxis * 5.0f));
									investigateDir = true;
								}
							}
							clearInvestigateTimeLeft = Random::get_float(30.0f, 60.0f);
							encouragedInvestigate = true;
							investigateGlance = false;
							clearInvestigateGlanceTimeLeft = 0.0f;
							forceInvestigate = false;
							forceInvestigateReason = Name::invalid();
							ignoreViolenceDisallowed = true;
#ifdef AN_USE_AI_LOG
							if (investigate.is_valid())
							{
								ai_log_colour(mind->get_logic(), Colour::cyan);
								ai_log(mind->get_logic(), TXT("[perception] investigate at %S in %S (attract)"), investigate.placement.get_translation().to_string().to_char(), investigate.inRoom.get()->get_name().to_char());
								ai_log_no_colour(mind->get_logic());
							}
#endif
							framePtr->end_waiting();
							AI_LATENT_STOP_LONG_RARE_ADVANCE();
						}
					}
				}
			}
		}
		);
		messageHandler.set(NAME(confussion), [framePtr, mind](Framework::AI::Message const & _message)
		{
			framePtr->end_waiting();
			AI_LATENT_STOP_LONG_RARE_ADVANCE();
		}
		);
	}

	while (true)
	{
		LATENT_CLEAR_LOG();
		narrativeSafeRequested = GameDirector::is_violence_disallowed();
		ignoreViolenceDisallowed &= narrativeSafeRequested; // if no narrative safe requested, clear ignore
		narrativeSafeRequested &= !ignoreViolenceDisallowed; // if ignoring, clear narrative
		ignoreEnemiesNow = narrativeSafeRequested || confussion.is_confused();
		if (ignoreEnemiesNow)
		{
			if (forceInvestigate &&
				forceInvestigateReason == NAME(spawnManager))
			{
				ai_log_colour(mind->get_logic(), Colour::cyan);
				ai_log(mind->get_logic(), TXT("[perception] drop investigation as we're in safe space mode"));
				ai_log_no_colour(mind->get_logic());
				investigate.clear();
				investigateGlance = false;
				encouragedInvestigate = false;
				forceInvestigate = false;
				forceInvestigateReason = Name::invalid();
				investigateDir = false;
			}
			if ((enemyPath.is_active() || enemy.is_pointing_at_something() || enemyPlacement.is_active() || enemyPlacementLastValid.is_active()) && perceptionEnemyLock.get() != PerceptionLock::Lock)
			{
				MEASURE_PERFORMANCE(switchSidesChanged);
				ai_log_colour(mind->get_logic(), Colour::cyan);
				ai_log(mind->get_logic(), TXT("[perception] entered narrative mode"));
				ai_log_no_colour(mind->get_logic());
				enemy.clear();
				enemyPath.clear_target();
				enemyPlacement.clear_target();
				enemyPlacementLastValid.reset();
				enemyProposition.clear_target();
			}
		}
		if (!perceptionPaused.is_paused() && !ignoreEnemiesNow)
		{
			SCOPED_PERFORMANCE_LIMIT_GUARD(0.0004f, TXT("perception as whole"));
			MEASURE_PERFORMANCE(perception);
#ifdef AN_DEVELOPMENT
			debug_filter(ai_enemyPerception);
			debug_subject(imo);
			Framework::debug_draw_path(enemyPlacement, Colour::red, npcBase->get_perception_thinking_time().max);
			Framework::debug_draw_path(enemyPlacementLastValid, Colour::magenta.with_alpha(0.4f), npcBase->get_perception_thinking_time().max);
			Framework::debug_draw_path(enemyPath, Colour::green.with_alpha(0.5f), npcBase->get_perception_thinking_time().max);
			debug_no_filter();
			debug_no_subject();
#endif
			{
				LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("+- hunches"));
				auto& hunches = mind->access_execution().access_hunches();
				if (!hunches.is_empty())
				{
					MEASURE_PERFORMANCE(hunches);
					for_every_ref(hunch, hunches)
					{
						if (hunch->get_name() == NAME(investigate))
						{
							auto* roomParam = hunch->get_param(NAME(room));
							auto* locationParam = hunch->get_param(NAME(location));
							++investigateIdx;
							investigate = Framework::InRoomPlacement(roomParam->get_as<SafePtr<Framework::Room>>().get(), locationParam->get_as<Vector3>());
							investigateGlance = false;
							encouragedInvestigate = false;
							forceInvestigate = true;
							forceInvestigateReason = hunch->get_reason();
							investigateDir = false;
							clearInvestigateTimeLeft = Random::get_float(30.0f, 80.0f);
							hunch->consume();
							ai_log_colour(mind->get_logic(), Colour::cyan);
							ai_log(mind->get_logic(), TXT("[perception] we got a hunch, investigate [%S ; %S]"),
								roomParam->get_as<SafePtr<Framework::Room>>().get()? roomParam->get_as<SafePtr<Framework::Room>>().get()->get_name().to_char() : TXT("[no room provided]"),
								locationParam->get_as<Vector3>().to_string().to_char()
								);
							ai_log_no_colour(mind->get_logic());
						}
					}
				}
			}

			if (enemyPath.is_active())
			{
				if (cooldowns.is_available(NAME(simplifyEnemyPath)))
				{
					LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("+- simplifyEnemyPath"));
					MEASURE_PERFORMANCE(simplifyEnemyPath);
					cooldowns.set(NAME(simplifyEnemyPath), Random::get_float(0.5f, 2.0f));

					Framework::PresencePath simplifiedEnemyPath;
					simplifiedEnemyPath.be_temporary_snapshot();
					if (simplifiedEnemyPath.find_path(enemyPath.get_owner(), enemyPath.get_target(), true, 4))
					{
						if (simplifiedEnemyPath.get_through_doors().get_size() < enemyPath.get_through_doors().get_size() ||
							simplifiedEnemyPath.calculate_string_pulled_distance() < enemyPath.calculate_string_pulled_distance() - 1.0f)
						{
							ai_log_colour(mind->get_logic(), Colour::cyan);
							ai_log(mind->get_logic(), TXT("[perception] shorter path found"));
							ai_log_no_colour(mind->get_logic());
							enemyPath = simplifiedEnemyPath;
						}
					}
				}
			}
			{
				LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("+- manage enemy"));
				/**
				 *	This is simplified approach:
				 *		1. know your enemy (anywhere it is)
				 *		2. check if you can see enemy, basing on whether you know where it is and if you are looking in general direction of it (take distance, movement etc into account)
				 *			note that someone else (AI) should decide what we do, if we investigate last known location or look anywhere else
				 */
				if (queuedEnemy.is_set() && perceptionEnemyLock.get() != PerceptionLock::Lock)
				{
					LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("   +- switchToQueuedEnemy"));
					MEASURE_PERFORMANCE(switchToQueuedEnemy);
					ai_log_colour(mind->get_logic(), Colour::cyan);
					ai_log(mind->get_logic(), TXT("[perception] switch to queued enemy"));
					ai_log_no_colour(mind->get_logic());
					enemy = queuedEnemy;
					enemyPath.find_path(mind->get_owner_as_modules_owner(), enemy.get(), true);
					enemyPlacement.clear_target();
					enemyPlacementLastValid.reset();
					queuedEnemy.clear();
				}
				if (switchSidesHandler && prevSwitchSidesTo.get() != switchSidesHandler->get_switch_sides_to() && perceptionEnemyLock.get() != PerceptionLock::Lock)
				{
					LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("   +- switchSidesChanged"));
					MEASURE_PERFORMANCE(switchSidesChanged);
					ai_log_colour(mind->get_logic(), Colour::cyan);
					ai_log(mind->get_logic(), TXT("[perception] switch sides changed, clear enemy and enemy proposition"));
					ai_log_no_colour(mind->get_logic());
					prevSwitchSidesTo = switchSidesHandler->get_switch_sides_to();
					enemy.clear();
					enemyPath.clear_target();
					enemyPlacement.clear_target();
					enemyPlacementLastValid.reset();
					enemyProposition.clear_target();
				}
				if (switchSidesHandler && switchSidesHandler->get_switch_sides_to() && enemy.is_set())
				{
					LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("   +- switchSides"));
					MEASURE_PERFORMANCE(switchSides);
					bool sameSides = false;
					if (enemy == switchSidesHandler->get_switch_sides_to())
					{
						sameSides = true;
					}
					else if (auto* enemySSH = enemy->get_custom<CustomModules::SwitchSidesHandler>())
					{
						if (enemySSH->get_switch_sides_to() == switchSidesHandler->get_switch_sides_to())
						{
							sameSides = true;
						}
					}
					if (sameSides)
					{
						ai_log_colour(mind->get_logic(), Colour::cyan);
						ai_log(mind->get_logic(), TXT("[perception] we're on the same side now, we don't want to be enemies"));
						ai_log_no_colour(mind->get_logic());
						enemy.clear();
						enemyPath.clear_target();
						enemyPlacement.clear_target();
						enemyPlacementLastValid.reset();
						enemyProposition.clear_target();
					}
				}
			}

			// check if collided
			if (auto* mc = imo->get_collision())
			{
				LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("+- checkCollisions"));
				MEASURE_PERFORMANCE(checkCollisions);
				for_every(oc, mc->get_collided_with())
				{
					if (Framework::IModulesOwner const * object = fast_cast<Framework::IModulesOwner>(oc->collidedWithObject.get()))
					{
						bool investigateNewEnemy = false;
						object = object->get_instigator_first_actor_or_valid_top_instigator();
						if (!enemy.get())
						{
							if (cooldowns.is_available(NAME(investigateNoEnemy)))
							{
								cooldowns.set(NAME(investigateNoEnemy), Random::get_float(0.1f, 0.8f));
								auto* ai = imo->get_ai();
								if (ai && ai->get_mind()->get_social().is_enemy(object))
								{
									ai_log_colour(mind->get_logic(), Colour::cyan);
									ai_log(mind->get_logic(), TXT("[perception] we didn't have an enemy, but now we collided with a good candidate"));
									ai_log_no_colour(mind->get_logic());
									enemy = object;
									investigateNewEnemy = true;
								}
								else if (!forceInvestigate && !enemyProposition.get_target() && fast_cast<Framework::Actor>(object) && Random::get_chance(magic_number 0.1f))
								{
									// we don't have enemy, it is not an enemy
									// investigate basing on chance
									++investigateIdx;
									clearInvestigateTimeLeft = Random::get_float(1.0f, 4.0f);
									investigate = Framework::InRoomPlacement(object->get_presence()->get_in_room(), object->get_presence()->get_centre_of_presence_WS());
									investigateGlance = false;
									encouragedInvestigate = false;
									forceInvestigate = false;
									forceInvestigateReason = Name::invalid();
									investigateDir = false;
									LATENT_LOG(TXT("hit something, investigate"));
									ai_log_colour(mind->get_logic(), Colour::cyan);
									ai_log(mind->get_logic(), TXT("[perception] hit something, investigate \"%S\", investigate"), object->ai_get_name().to_char());
									ai_log_no_colour(mind->get_logic());
								}
							}
						}
						if ((enemy.get() == object || enemyProposition.get_target() == object) &&// only our current enemy
							(investigateNewEnemy || cooldowns.is_available(NAME(investigateEnemy)))) // if we're forced to investigate new neemy or we may try to investigate current one
						{
							// check if we actually want to investigate or just update target. or maybe just continue as is
							cooldowns.set(NAME(investigateEnemy), Random::get_float(0.5f, 2.0f));
							// update enemy and path to it
							if (enemy.get() != object || ! enemyPath.is_active())
							{
								enemy = object;
								enemyPath.find_path(mind->get_owner_as_modules_owner(), enemy.get(), true);
								if (npcBase->is_omniscient())
								{
									enemyPlacement = enemyPath;
									enemyPlacementLastValid = enemyPath;
								}
								else
								{
									enemyPlacement.clear_target();
									enemyPlacementLastValid.reset();
								}
							}

							bool doInvestigate = enemy.get() != object || Random::get_chance(0.1f); // only by chance if this is our enemy
							if (npcBase->is_omniscient())
							{
								doInvestigate = false;
							}
							else
							{
								Transform enemyPlaced = enemyPath.get_placement_in_owner_room();
								Vector3 toEnemy = (enemyPlaced.get_translation() - imo->get_presence()->get_placement().get_translation());
								float inFrontAngle = 80.0f;
								float dist = 1.0f;
								if (toEnemy.length_squared() < sqr(dist) &&
									Vector3::dot(toEnemy.normal(), imo->get_presence()->get_placement().get_axis(Axis::Y)) >= cos_deg(inFrontAngle))
								{
									doInvestigate = Random::get_chance(0.1f);
									ai_log_colour(mind->get_logic(), Colour::cyan);
									ai_log(mind->get_logic(), TXT("[perception] collided with enemy in front and close \"%S\", %S"), enemy->ai_get_name().to_char(), doInvestigate? TXT("investigate by chance") : TXT("just allow to attack"));
									ai_log_no_colour(mind->get_logic());
								}
								else
								{
									doInvestigate = true;
#ifdef AN_DEVELOPMENT
#ifdef AN_USE_AI_LOG
									float realDist = toEnemy.length_squared();
									float realInFrontAngle = acos_deg(clamp(Vector3::dot(toEnemy.normal(), imo->get_presence()->get_placement().get_axis(Axis::Y)), -1.0f, 1.0f));
									ai_log_colour(mind->get_logic(), Colour::cyan);
									ai_log(mind->get_logic(), TXT("[perception] collided with enemy not in front %.3f and not close %.3f"), realInFrontAngle, realDist);
									ai_log_no_colour(mind->get_logic());
#endif
#endif
								}
							}
							if (doInvestigate)
							{
								// we will be looking at the enemy
								++investigateIdx;
								investigate = Framework::InRoomPlacement(object->get_presence()->get_in_room(), object->get_presence()->get_centre_of_presence_WS());
								investigateGlance = false;
								encouragedInvestigate = false;
								forceInvestigate = false;
								forceInvestigateReason = Name::invalid();
								investigateDir = false;
								LATENT_LOG(TXT("collision with enemy, investigate"));
								ai_log_colour(mind->get_logic(), Colour::cyan);
								ai_log(mind->get_logic(), TXT("[perception] collided with enemy \"%S\", investigate"), enemy->ai_get_name().to_char());
								ai_log_no_colour(mind->get_logic());
							}
							else
							{
								// update find closest path if far
								if (enemyPlacement.calculate_string_pulled_distance() > 2.0f)
								{
									enemyPlacement.find_path(mind->get_owner_as_modules_owner(), enemy.get(), false);
									enemyPlacementLastValid.copy_just_target_placement_from(enemyPlacement);
								}
								if (investigate.is_valid())
								{
									++investigateIdx;
									investigate.clear();
									investigateGlance = false;
									encouragedInvestigate = false; 
									forceInvestigate = false;
									forceInvestigateReason = Name::invalid();
									investigateDir = false;
									LATENT_LOG(TXT("collision with enemy, do not investigate"));
									ai_log_colour(mind->get_logic(), Colour::cyan);
									ai_log(mind->get_logic(), TXT("[perception] collided with enemy \"%S\", do not investigate"), enemy->ai_get_name().to_char());
									ai_log_no_colour(mind->get_logic());
								}
							}
						}
					}
				}
			}

			// get enemy proposition, so we know what we're looking for
			if (!enemyProposition.is_active() || perceptionEnemyLock.get() == PerceptionLock::LookForNew)
			{
				LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("+- lookForNewEnemy"));
				MEASURE_PERFORMANCE(lookForNewEnemy);

				LATENT_LOG(TXT("look for new"));
				if (!perceptionRequest.is_set() && (!enemyProposition.is_active() || cooldowns.is_available(NAME(enemyProposition))))
				{
					LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("   +- lookForNewEnemy.createRequest"));
					MEASURE_PERFORMANCE(createRequest);
					LATENT_LOG(TXT("create request"));
					auto* ai = imo->get_ai();
					auto* sst = switchSidesHandler ? switchSidesHandler->get_switch_sides_to() : nullptr;
					if (sst)
					{
						ai = sst->get_ai();
					}
					perceptionRequest = mind->access_perception().add_request(new AI::PerceptionRequests::FindInVisibleRooms(imo, [ai, sst](Framework::IModulesOwner const* _object)
					{
						if (sst)
						{
							if (sst == _object)
							{
								// we switched sides to it!
								return false;
							}
							if (auto* ossh = _object->get_custom<CustomModules::SwitchSidesHandler>())
							{
								if (sst == ossh->get_switch_sides_to())
								{
									// we're good
									return false;
								}
							}
						}
						return ai->get_mind()->get_social().is_enemy(_object);
					}));
				}
				if (perceptionRequest.is_set() &&
					perceptionRequest->is_processed())
				{
					LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("   +- lookForNewEnemy.processResult"));
					MEASURE_PERFORMANCE(processResult);
					LATENT_LOG(TXT("request procesed"));
					if (auto * request = fast_cast<AI::PerceptionRequests::FindInVisibleRooms>(perceptionRequest.get()))
					{
#ifdef AN_DEVELOPMENT
						auto* prevEP = enemyProposition.get_target();
#endif
						if (request->has_found_anything())
						{
							enemyProposition = request->get_path_to_best();
#ifdef AN_DEVELOPMENT
							if (enemyProposition.get_target() != prevEP)
							{
								ai_log_colour(mind->get_logic(), Colour::cyan);
								ai_log(mind->get_logic(), TXT("[perception] new enemy proposition %S [look for new]"), enemyProposition.get_target() ? enemyProposition.get_target()->ai_get_name().to_char() : TXT("! NONE !"));
								ai_log_no_colour(mind->get_logic());
							}
#endif
						}
						else
						{
							enemyProposition.clear_target();
#ifdef AN_DEVELOPMENT
							if (enemyProposition.get_target() != prevEP)
							{
								ai_log_colour(mind->get_logic(), Colour::cyan);
								ai_log(mind->get_logic(), TXT("[perception] clear enemy proposition [look for new]"));
								ai_log_no_colour(mind->get_logic());
							}
#endif
						}
					}
					perceptionRequest = nullptr;
					cooldowns.set(NAME(enemyProposition), Random::get_float(0.2f, 0.5f));
				}
			}
			else if (perceptionEnemyLock.get() != PerceptionLock::Lock) // locking this means that if enemy dies and we don't know it
			{
				LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("+- not locked"));
				MEASURE_PERFORMANCE(maybeOtherEnemyIsBetter);
				LATENT_LOG(TXT("not locked"));
				if (!perceptionRequest.is_set() && cooldowns.is_available(NAME(enemyProposition)))
				{
					LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("   +- not locked.createRequest"));
					MEASURE_PERFORMANCE(createRequest);
					LATENT_LOG(TXT("create request"));
					perceptionRequest = mind->access_perception().add_request(new AI::PerceptionRequests::FindInVisibleRooms(imo, [enemyProposition](Framework::IModulesOwner const* _object)
					{
						return _object == enemyProposition.get_target();
					}));
				}
				if (perceptionRequest.is_set() &&
					perceptionRequest->is_processed())
				{
					LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("   +- not locked.processResult"));
					MEASURE_PERFORMANCE(processResult);
					LATENT_LOG(TXT("request processed"));
					if (auto * request = fast_cast<AI::PerceptionRequests::FindInVisibleRooms>(perceptionRequest.get()))
					{
#ifdef AN_DEVELOPMENT
						auto* prevEP = enemyProposition.get_target();
#endif
						if (request->has_found_anything())
						{
							enemyProposition = request->get_path_to_best();
#ifdef AN_DEVELOPMENT
							if (enemyProposition.get_target() != prevEP)
							{
								ai_log_colour(mind->get_logic(), Colour::cyan);
								ai_log(mind->get_logic(), TXT("[perception] new enemy proposition %S [not locked]"), enemyProposition.get_target() ? enemyProposition.get_target()->ai_get_name().to_char() : TXT("! NONE !"));
								ai_log_no_colour(mind->get_logic());
							}
#endif
						}
						else
						{
							enemyProposition.clear_target();
#ifdef AN_DEVELOPMENT
							if (enemyProposition.get_target() != prevEP)
							{
								ai_log_colour(mind->get_logic(), Colour::cyan);
								ai_log(mind->get_logic(), TXT("[perception] clear enemy proposition [not locked]"));
								ai_log_no_colour(mind->get_logic());
							}
#endif
						}
					}
					perceptionRequest = nullptr;
					cooldowns.set(NAME(enemyProposition), Random::get_float(0.5f, 1.5f));
				}
			}

			if (npcBase)
			{
				LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("+- checkIfSeesEnemy"));
				MEASURE_PERFORMANCE(checkIfSeesEnemy);
				todo_note(TXT("get perception socket and perception FOV"));
				Transform perceptionSocketOS = npcBase->get_perception_socket_index() != NONE? imo->get_appearance()->calculate_socket_os(npcBase->get_perception_socket_index()) : imo->get_presence()->get_centre_of_presence_os();
				Transform perceptionSocketWS = imo->get_presence()->get_placement().to_world(perceptionSocketOS);
				Optional<Transform> secondaryPerceptionSocketWS;
				if (npcBase->get_secondary_perception_socket_index() != NONE)
				{
					Transform perceptionSocketOS = imo->get_appearance()->calculate_socket_os(npcBase->get_secondary_perception_socket_index());
					if (perceptionSocketOS.get_scale() > 0.0f)
					{
						secondaryPerceptionSocketWS = imo->get_presence()->get_placement().to_world(perceptionSocketOS);
					}
				}
				Range perceptionFOV = npcBase->get_perception_fov();
				Range perceptionVerticalFOV = npcBase->get_perception_vertical_fov();

				if (switchSidesHandler && switchSidesHandler->get_switch_sides_to())
				{
					todo_hack(TXT("this is to allow AI associated to player to have greater perception FOV to note enemies more easily"));
					perceptionFOV = perceptionFOV * 1.3f;
					perceptionVerticalFOV = perceptionVerticalFOV * 1.3f;
				}

				perceptionFOV = perceptionFOV.clamp_to(Range(-180.0f, 180.0f));
				perceptionVerticalFOV = perceptionVerticalFOV.clamp_to(Range(-90.0f, 90.0f));

				if (!enemy.is_set())
				{
					LATENT_LOG(TXT("no enemy"));
					enemyPath.clear_target();
					if (enemy.is_pointing_at_something())
					{
						// we still want to think that enemy is there
						ai_log_colour(mind->get_logic(), Colour::cyan);
						LATENT_LOG(TXT("perception, enemy still there?"));
						ai_log_no_colour(mind->get_logic());
						if (enemyPlacementLastValid.is_active())
						{
							enemyPlacement = enemyPlacementLastValid;
						}
						else
						{
							enemyPlacement.clear_target_but_follow();
						}
						enemyPlacementLastValid.reset();
					}
					else if (enemyPlacement.is_active())
					{
						// we no longer have an enemy
						ai_log_colour(mind->get_logic(), Colour::cyan);
						LATENT_LOG(TXT("perception, lost enemy"));
						ai_log_no_colour(mind->get_logic());
						enemyPlacement.clear_target();
						enemyPlacementLastValid.reset();
					}
				}
				else if (enemy.get() != enemyPath.get_target())
				{
					ai_log_colour(mind->get_logic(), Colour::cyan);
					LATENT_LOG(TXT("perception, enemy changed"));
					ai_log_no_colour(mind->get_logic());
					// enemy was changed by someone else!
					if (enemyPath.find_path(imo, enemy.get(), true))
					{
						if (npcBase->is_omniscient())
						{
							enemyPlacement = enemyPath;
							enemyPlacementLastValid = enemyPath;
						}
						else
						{
							enemyPlacement.clear_target();
							enemyPlacementLastValid.reset();
						}
					}
					else
					{
						ai_log_colour(mind->get_logic(), Colour::cyan);
						ai_log(mind->get_logic(), TXT("[perception] we couldn't find path to enemy, forget about this one"));
						ai_log_no_colour(mind->get_logic());
						enemy.clear();
						enemyPath.clear_target();
						enemyPlacement.clear_target();
						enemyPlacementLastValid.reset();
						enemyProposition.clear_target();
					}
				}

				// we should have either enemy set or not but same in both
				an_assert(enemy.get() == enemyPath.get_target());

				{
					LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("   +- allowNewEnemy?"));

					bool allowNewEnemy = false;

					if (!perceptionStartOff)
					{
						allowNewEnemy = true;
					}

					// we check enemy placement only if we point at actual enemy
					// we do that only to lose it if we won't notice enemy there
					// then we turn it into pointing at pure placement and we try to find
					// actual enemy somewhere in front
					if (allowNewEnemy &&
						enemyPlacement.is_active() &&
						enemyPlacement.get_target())
					{
						LATENT_LOG(TXT("enemy visible check"));
						if (cooldowns.is_available(NAME(updateEnemySeen)))
						{
							LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("      +- updateEnemySeen"));
							MEASURE_PERFORMANCE(updateEnemySeen);
							cooldowns.set(NAME(updateEnemySeen), Random::get_float(0.1f, 0.3f));
							bool reachedTarget = false;
							if (npcBase->is_omniscient())
							{
								reachedTarget = true;
							}
							else if (!perceptionSightImpaired && !perceptionNoSight)
							{
								reachedTarget = check_clear_perception_ray_cast(CastInfo(), perceptionSocketWS, secondaryPerceptionSocketWS, perceptionFOV, perceptionVerticalFOV, imo,
									enemyPlacement.get_target(),
									enemyPlacement.get_in_final_room(), enemyPlacement.get_placement_in_owner_room());
								if (reachedTarget && perceptionSightMovementBased)
								{
									reachedTarget = is_imo_moving(enemyPlacement.get_target());
								}
							}

							if (reachedTarget)
							{
								update_offsets(enemyCentreOffsetOS, enemyTargetingOffsetOS, imo, enemyPlacement);
								enemyPlacementLastValid.copy_just_target_placement_from(enemyPlacement);
								doesntSeeCount = 0;
							}
							else
							{
								++doesntSeeCount;
								if (doesntSeeCount > 1)
								{
									if (enemyPlacementLastValid.is_active())
									{
										enemyPlacement = enemyPlacementLastValid;
									}
									enemyPlacement.clear_target_but_follow();
									enemyPlacementLastValid.reset();
#ifdef AN_DEVELOPMENT
									ai_log_colour(mind->get_logic(), Colour::cyan);
									LATENT_LOG(TXT("perception, not seen"));
									ai_log_no_colour(mind->get_logic());
#endif
								}
							}
						}
						allowNewEnemy = false;
					}

					if (!perceptionStartOff)
					{
						allowNewEnemy |= perceptionEnemyLock.get() == PerceptionLock::LookForNew || (enemyPath.is_active() && enemy.get() == enemyPath.get_target());
					}

					if (allowNewEnemy)
					{
						LATENT_LOG(TXT("new enemy allowed"));
						if (cooldowns.is_available(NAME(changeEnemyPropositionIntoEnemy)))
						{
							LATENT_SCOPE_ELEMENT_TIME_LIMIT_INFO(TXT("      +- changeEnemyPropositionIntoEnemy"));
							MEASURE_PERFORMANCE(changeEnemyPropositionIntoEnemy);
							LATENT_LOG(TXT("change enemy proposition (or path) into enemy"));
							cooldowns.set(NAME(changeEnemyPropositionIntoEnemy), perceptionEnemyLock.get() == PerceptionLock::LookForNew? Random::get_float(0.1f, 0.3f) : Random::get_float(0.4f, 1.0f));

							todo_note(TXT("use chance? use fov offset?"));

							Framework::PresencePath* checkPath = enemy.is_set() && enemyPath.is_active() ? &enemyPath : (enemyProposition.is_active() ? &enemyProposition : nullptr);

							if (checkPath)
							{
								LATENT_LOG(TXT("check path!"));
								checkPath->update_shortcuts();

								// check if by throwing a ray towards real location (or close to it) we end in right room and location
								// if we do, that's our new enemy and real enemy placement
								bool reachedTarget = false;
								if (npcBase->is_omniscient())
								{
									reachedTarget = true;
								}
								else if ((!perceptionSightImpaired && !perceptionNoSight) &&
									checkPath->get_target())
								{
									reachedTarget = check_clear_perception_ray_cast(CastInfo(), perceptionSocketWS, secondaryPerceptionSocketWS, perceptionFOV, perceptionVerticalFOV, imo,
										checkPath->get_target(),
										checkPath->get_target_presence()->get_in_room(), checkPath->from_target_to_owner(checkPath->get_target_presence()->get_placement()));
									if (reachedTarget && perceptionSightMovementBased)
									{
										reachedTarget = is_imo_moving(checkPath->get_target());
									}
								}

								if (reachedTarget && checkPath->get_target())
								{
									enemy = checkPath->get_target();
									enemyPath = *checkPath;
									enemyPlacement = *checkPath;
									enemyPlacement.follow_if_target_lost();
									update_offsets(enemyCentreOffsetOS, enemyTargetingOffsetOS, imo, enemyPlacement);
									enemyPlacementLastValid.copy_just_target_placement_from(enemyPlacement);
									if (auto* s = imo->get_sound())
									{
										if (s->play_sound(NAME(alert)))
										{
											if (Framework::GameUtils::is_controlled_by_player(enemy.get()))
											{
												if (!npcBase || npcBase->is_ok_to_play_combat_music(*checkPath))
												{
													MusicPlayer::request_combat_auto_indicate_presence();
												}

												issue_perception_alert_ai_message(mind->get_owner_as_modules_owner(), enemy.get(), enemyPlacement);
											}
										}
									}
#ifdef AN_DEVELOPMENT_OR_PROFILER
									if (investigate.is_valid())
									{
										ai_log_colour(mind->get_logic(), Colour::cyan);
										ai_log(mind->get_logic(), TXT("[perception] investigate cleared, enemy reached"));
										ai_log_no_colour(mind->get_logic());
									}
#endif
									investigate.clear();
									++investigateIdx;
#ifdef AN_DEVELOPMENT
									ai_log_colour(mind->get_logic(), Colour::cyan);
									LATENT_LOG(TXT("perception, reached target"));
									ai_log(mind->get_logic(), TXT("[perception] new enemy (perception reached target, stop investigating) %S"), enemy->ai_get_name().to_char());
									ai_log_no_colour(mind->get_logic());
#endif
								}
								else
								{
									LATENT_LOG(TXT("not reached"));
									// we won't get new enemy, enemy placement is already pointing at placement, not target
								}
							}
							else
							{
								LATENT_LOG(TXT("no enemy to check"));
								// we don't have an enemy
							}
						}
					}
				}
			}
		}
		LATENT_WAIT(mind->get_logic()->adjust_time_depending_on_distance(npcBase->get_perception_thinking_time(), adjustWaitTime)
					* (0.6f + 0.4f * GameSettings::get().difficulty.aiReactionTime)); // thinking time
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();

	LATENT_RETURN();
}
