#include "aiManagerLogic_shooterWavesManager.h"

#include "..\..\..\game\game.h"
#include "..\..\..\modules\custom\health\mc_health.h"

#include "..\..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\gameScript\gameScriptExecution.h"
#include "..\..\..\..\framework\library\library.h"
#include "..\..\..\..\framework\module\moduleAI.h"
#include "..\..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\..\framework\object\actor.h"
#include "..\..\..\..\framework\world\room.h"

#include "..\..\..\..\core\debug\\debugRenderer.h"

#ifdef AN_ALLOW_BULLSHOTS
#include "..\..\..\..\framework\game\bullshotSystem.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;
using namespace Managers;

//

// ai messages
DEFINE_STATIC_NAME_STR(aim_swm_start, TXT("shooter waves manager; start"));
DEFINE_STATIC_NAME_STR(aim_swm_stop, TXT("shooter waves manager; stop"));
DEFINE_STATIC_NAME_STR(aim_sw_start, TXT("shooter waves; start"));
DEFINE_STATIC_NAME_STR(aim_sw_move, TXT("shooter waves; move"));
DEFINE_STATIC_NAME_STR(aim_sw_attack, TXT("shooter waves; attack"));
DEFINE_STATIC_NAME(targetIMO);
DEFINE_STATIC_NAME(targetPlacementSocket);
DEFINE_STATIC_NAME(followIMO);
DEFINE_STATIC_NAME(limitFollowRelVelocity);
DEFINE_STATIC_NAME(moveTo);
DEFINE_STATIC_NAME(followRelVelocity);
DEFINE_STATIC_NAME(moveDir);
DEFINE_STATIC_NAME(readyToAttack);
DEFINE_STATIC_NAME(attack);
DEFINE_STATIC_NAME(attackLength);
DEFINE_STATIC_NAME(shootStartOffsetPt);
DEFINE_STATIC_NAME(shootStartOffsetTS);
DEFINE_STATIC_NAME(reachedEndDelay);
DEFINE_STATIC_NAME(triggerTrapWhenReachedEnd);

// params/variables
DEFINE_STATIC_NAME(inRoom);

//

bool ShooterActionPoint::load_from_xml(IO::XML::Node const* _node, ::Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("shooterType")))
	{
		result &= shooterType.load_from_xml(node);
	}

	entry = _node->get_bool_attribute_or_from_child_presence(TXT("entry"), entry);
	exit = _node->get_bool_attribute_or_from_child_presence(TXT("exit"), exit);
	
	id = _node->get_name_attribute_or_from_child(TXT("id"), id);
	tags.load_from_xml_attribute_or_child_node(_node, TXT("tags"));
	continueWithTagged.load_from_xml_attribute_or_child_node(_node, TXT("continueWithTagged"));
	relativeToObjectTagged.load_from_xml_attribute_or_child_node(_node, TXT("relativeToObjectTagged"));

	for_every(node, _node->children_named(TXT("relativeToObject")))
	{
		relativeToObjectTagged.load_from_xml_attribute_or_child_node(node, TXT("tagged"));
	}

	for_every(node, _node->children_named(TXT("slot")))
	{
		SlotPoint sp;
		sp.location.load_from_xml(node);
		sp.relVelocity.load_from_xml(node, TXT("relVelocity"));
		slotPoints.push_back(sp);
	}
	
	limitFollowRelVelocity.load_from_xml(_node, TXT("limitFollowRelVelocity"));

	//
	nextJustOneRequired.load_from_xml(_node, TXT("nextJustOneRequired"));
	nextOnDistance.load_from_xml(_node, TXT("nextOnDistance"));

	//
	attackJustOneRequired.load_from_xml(_node, TXT("attackJustOneRequired"));
	readyToAttackOnDistance.load_from_xml(_node, TXT("readyToAttackOnDistance"));
	attackOnDistance.load_from_xml(_node, TXT("attackOnDistance"));
	beforeAttackDelay.load_from_xml(_node, TXT("beforeAttackDelay"));
	attackLength.load_from_xml(_node, TXT("attackLength"));
	shootStartOffsetPt.load_from_xml(_node, TXT("shootStartOffsetPt"));
	shootStartOffsetTS.load_from_xml(_node, TXT("shootStartOffsetTS"));
	postAttackDelay.load_from_xml(_node, TXT("postAttackDelay"));

	//
	disappearOnDistance.load_from_xml(_node, TXT("disappearOnDistance"));
	disappearOnDistanceAway.load_from_xml(_node, TXT("disappearOnDistanceAway"));

	return result;
}

bool ShooterActionPoint::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

//

REGISTER_FOR_FAST_CAST(ShooterWavesManager);

ShooterWavesManager::ShooterWavesManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
, shooterWavesManagerData(fast_cast<ShooterWavesManagerData>(_logicData))
{
	random = _mind->get_owner_as_modules_owner()->get_individual_random_generator();
	random.advance_seed(2397, 9752904);
}

ShooterWavesManager::~ShooterWavesManager()
{
}

LATENT_FUNCTION(ShooterWavesManager::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto

	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto * self = fast_cast<ShooterWavesManager>(logic);
	auto * imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	self->started = false;

	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(aim_swm_start), [self](Framework::AI::Message const& _message)
			{
				if (!self->started)
				{
					if (auto* param = _message.get_param(NAME(triggerTrapWhenReachedEnd)))
					{
						self->triggerTrapWhenReachedEnd = param->get_as<Name>();
					}
					self->started = true;
					self->waveIdx = 0;
					if (self->shooterWavesManagerData->waveCount.is_empty())
					{
						self->waveCount = NONE;
					}
					else
					{
						self->waveCount = self->random.get(self->shooterWavesManagerData->waveCount);
					}
				}
			});
		messageHandler.set(NAME(aim_swm_stop), [self](Framework::AI::Message const& _message)
			{
				self->reached_end();
			});
	}

	// wait a bit to make sure POIs are there
	LATENT_WAIT(1.0f);

	// get information where it does work
	self->inRoom = imo->get_variables().get_value<SafePtr<Framework::Room>>(NAME(inRoom), SafePtr<Framework::Room>());
	if (!self->inRoom.get())
	{
		self->inRoom = imo->get_presence()->get_in_room();
	}

	ai_log_colour(self, Colour::blue);
	ai_log(self, TXT("random \"%S\""), self->random.get_seed_string().to_char());
	ai_log_no_colour(self);

	{
		if (!self->shooterWavesManagerData->targetTagged.is_empty())
		{
			for_every_ptr(o, self->inRoom->get_objects())
			{
				if (self->shooterWavesManagerData->targetTagged.check(o->get_tags()))
				{
					self->targetIMO = o;
					break;
				}
			}
		}

		for_every_ref(sap, self->shooterWavesManagerData->actionPoints)
		{
			RefCountObjectPtr<ShooterActionPoint> ap;
			ap = new ShooterActionPoint();
			*(ap.get()) = *sap;

			if (!ap->shooterType.is_empty())
			{
				Framework::WorldSettingConditionContext wscc(self->inRoom.get());
				wscc.get(Framework::Library::get_current(), Framework::ActorType::library_type(), ap->shooterTypes, ap->shooterType);
				wscc.get(Framework::Library::get_current(), Framework::SceneryType::library_type(), ap->shooterTypes, ap->shooterType);
			}
			if (!ap->relativeToObjectTagged.is_empty())
			{
				//FOR_EVERY_OBJECT_IN_ROOM(o, imo->get_presence()->get_in_room())
				for_every_ptr(o, self->inRoom->get_objects())
				{
					if (ap->relativeToObjectTagged.check(o->get_tags()))
					{
						ap->relativeToObject = o;
						break;
					}
				}
			}
			self->actionPoints.push_back(ap);
			if (ap->entry)
			{
				self->entryActionPoints.push_back(ap);
			}
		}
	}

	/*
	if (auto* ai = imo->get_ai())
	{
		auto& arla = ai->access_rare_logic_advance();
		arla.reset_to_no_rare_advancement();
	}
	*/

	while (true)
	{
		
		LATENT_WAIT_NO_RARE_ADVANCE(Random::get_float(0.1f, 0.5f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

void ShooterWavesManager::reached_end(bool _wavesDone)
{
	if (ended)
	{
		return;
	}
	ended = true;
	if (_wavesDone)
	{
		if (triggerTrapWhenReachedEnd.is_valid())
		{
			Framework::GameScript::ScriptExecution::trigger_execution_trap(triggerTrapWhenReachedEnd);
		}
	}
}

void ShooterWavesManager::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (! started)
	{
		return;
	}
	if (auto* imo = get_mind()->get_owner_as_modules_owner())
	{
		bool spawning = false;
		if (nextWaveTimeLeft.is_set())
		{
			nextWaveTimeLeft = nextWaveTimeLeft.get() - _deltaTime;
		}
		if (!entryActionPoints.is_empty())
		{
			if (!ended &&
				(waveCount == NONE || waveIdx < waveCount) &&
				(waves.is_empty() || nextWaveTimeLeft.is_set()))
			{
				int useDataWaveIdx = clamp(waveIdx, 0, shooterWavesManagerData->waves.get_size() - 1);
				int apIdx = random.get_int(entryActionPoints.get_size());
				if (shooterWavesManagerData->waves.is_index_valid(useDataWaveIdx))
				{
					auto fap = shooterWavesManagerData->waves[useDataWaveIdx].forceActionPoint;
					if (fap.is_valid())
					{
						for_every_ref(ap, entryActionPoints)
						{
							if (ap->id == fap)
							{
								apIdx = for_everys_index(ap);
								break;
							}
						}
					}
				}
				auto & eap = entryActionPoints[apIdx];
				if (auto* ap = eap.get())
				{
					RefCountObjectPtr<ShooterWave> wave;
					wave = new ShooterWave();

					wave->currentActionPoint = ap;
					wave->slotCount = ap->slotPoints.get_size();
					if (shooterWavesManagerData->waves.is_index_valid(useDataWaveIdx))
					{
						wave->slotCount = min(shooterWavesManagerData->waves[useDataWaveIdx].slotCount.get(random), wave->slotCount);
					}
					wave->slotIdxOffset = random.get_int(ap->slotPoints.get_size());
					wave->waitToStart = nextWaveTimeLeft.get(0.0f);

					waves.push_back(wave);
				}
				waveIdx++;
			}
			if (!currentWave.get() && !waves.is_empty())
			{
				currentWave = waves.get_first();
			}
			bool allWavesDone = waves.is_empty();
			if (!allWavesDone)
			{
				allWavesDone = true;
				for_every_ref(wave, waves)
				{
					if (auto* ap = wave->currentActionPoint.get())
					{
						if (! (ap->disappearOnDistance.is_set() ||
							   ap->disappearOnDistanceAway.is_set()))
						{
							allWavesDone = false;
							break;
						}
					}
				}
			}
			if (allWavesDone && waveCount != NONE && waveIdx >= waveCount)
			{
				reachedEndTime += _deltaTime;
				if (reachedEndTime > reachedEndDelay)
				{
					reached_end(true);
				}
			}
			else
			{
				reachedEndTime = 0.0f;
			}
		}
		for_every_ref(wave, waves)
		{
			wave->waitToStart = max(0.0f, wave->waitToStart - _deltaTime);
			if (auto* ap = wave->currentActionPoint.get())
			{
				if (wave->state == ShooterWave::State::Started)
				{
					if (ap->entry)
					{
						if (!ap->shooterTypes.is_empty())
						{
							if (!spawning)
							{
								wave->state = ShooterWave::State::Spawning;
							}
						}
						else
						{
							error(TXT("no shooterTypes"));
						}
					}
					else
					{
						if (ap->disappearOnDistance.is_set() ||
							ap->disappearOnDistanceAway.is_set())
						{
							wave->state = ShooterWave::State::WaitForDisappear;
						}
						else if (ap->attackLength.is_set())
						{
							wave->state = ShooterWave::State::WaitForAttack;
						}
						else if (ap->nextOnDistance.is_set())
						{
							wave->state = ShooterWave::State::WaitForNext;
						}
						else
						{
							wave->state = ShooterWave::State::Ended;
						}

						for_every(shooter, wave->shooters)
						{
							if (auto* simo = shooter->shooter.get())
							{
								if (Framework::AI::Message* message = simo->get_in_world()->create_ai_message(NAME(aim_sw_move)))
								{
									auto& sp = ap->slotPoints[mod(for_everys_index(shooter) + wave->slotIdxOffset, ap->slotPoints.get_size())];
									if (ap->relativeToObject.is_set())
									{
										message->access_param(NAME(followIMO)).access_as<SafePtr<Framework::IModulesOwner>>() = ap->relativeToObject;
										if (ap->limitFollowRelVelocity.is_set())
										{
											message->access_param(NAME(limitFollowRelVelocity)).access_as<float>() = ap->limitFollowRelVelocity.get();
										}
										if (sp.location.is_set())
										{
											message->access_param(NAME(moveTo)).access_as<Vector3>() = sp.location.get();
										}
										if (sp.relVelocity.is_set())
										{
											message->access_param(NAME(followRelVelocity)).access_as<Vector3>() = sp.relVelocity.get();
										}
									}
									else
									{
										if (sp.relVelocity.is_set())
										{
											message->access_param(NAME(moveDir)).access_as<Vector3>() = sp.relVelocity.get();
										}
										else if (sp.location.is_set())
										{
											message->access_param(NAME(moveTo)).access_as<Vector3>() = sp.location.get();
										}
									}
									message->access_param(NAME(readyToAttack)).access_as<bool>() = wave->readyToAttack;
									message->to_ai_object(simo);
								}
							}
						}
					}
				}
				if (wave->state == ShooterWave::State::Spawning)
				{
					spawning = true;
					if (auto* doc = spawningShooterDOC.get())
					{
						if (doc->is_done())
						{
							ShooterInstance shooter;
							shooter.shooter = doc->createdObject.get();
							if (auto* simo = shooter.shooter.get())
							{
								wave->shooters.push_back(shooter);
								wave->shooters.get_last().placementOnActionPointStart = simo->get_presence()->get_placement();
							}
							spawningShooterDOC.clear();
						}
					}

					if (!spawningShooterDOC.get())
					{
						if (wave->shooters.get_size() < wave->slotCount)
						{
							Framework::DelayedObjectCreation* doc = new Framework::DelayedObjectCreation();
							doc->activateImmediatelyEvenIfRoomVisible = true;
							doc->wmpOwnerObject = imo;
							doc->inRoom = inRoom.get();
							doc->name = TXT("shooter");
							doc->objectType = fast_cast<Framework::ObjectType>(wave->currentActionPoint->shooterTypes[random.get_int(wave->currentActionPoint->shooterTypes.get_size())]);
							Transform targetPlacement = calculate_target_placement(wave, wave->shooters.get_size(), true);
							doc->placement = targetPlacement;
							doc->randomGenerator = random.spawn();
							doc->priority = 1000;
							doc->checkSpaceBlockers = false;

							inRoom->collect_variables(doc->variables);

							spawningShooterDOC = doc;

							Game::get()->queue_delayed_object_creation(doc);
						}
						else if (wave->waitToStart <= 0.0f)
						{
							wave->state = ShooterWave::State::PlacePostSpawn;
							nextWaveTimeLeft.clear();
						}
					}
				}
				if (wave->state == ShooterWave::State::PlacePostSpawn)
				{
					for_every(shooter, wave->shooters)
					{
						if (auto* simo = shooter->shooter.get())
						{
							if (Framework::AI::Message* message = simo->get_in_world()->create_ai_message(NAME(aim_sw_start)))
							{
								message->access_param(NAME(targetIMO)).access_as<SafePtr<Framework::IModulesOwner>>() = targetIMO;
								if (shooterWavesManagerData->targetPlacementSocket.is_valid())
								{
									message->access_param(NAME(targetPlacementSocket)).access_as<Name>() = shooterWavesManagerData->targetPlacementSocket;
								}
								if (ap->relativeToObject.is_set())
								{
									message->access_param(NAME(followIMO)).access_as<SafePtr<Framework::IModulesOwner>>() = ap->relativeToObject;
								}
								auto& sp = ap->slotPoints[mod(for_everys_index(shooter) + wave->slotIdxOffset, ap->slotPoints.get_size())];
								if (sp.location.is_set())
								{
									message->access_param(NAME(moveTo)).access_as<Vector3>() = sp.location.get();
								}
								else
								{
									error(TXT("location not provided for start"));
								}
								message->to_ai_object(simo);
							}
						}
					}
					wave->state = ShooterWave::State::Ended;
				}
				if (wave->state == ShooterWave::State::WaitForNext)
				{
					if (ap->nextOnDistance.is_set())
					{
						if (done_distance(wave, ap->nextOnDistance.get(), NP, ap->nextJustOneRequired))
						{
							wave->state = ShooterWave::State::Ended;
						}
					}
					else
					{
						wave->state = ShooterWave::State::Ended;
					}
				}
				if (wave->state == ShooterWave::State::WaitForAttack)
				{
					bool shouldBeReadyToAttack = wave->readyToAttack;
					if (ended)
					{
						wave->state = ShooterWave::State::Ended;
					}
					else if (ap->attackOnDistance.is_set())
					{
						if (done_distance(wave, ap->attackOnDistance.get(), NP, ap->attackJustOneRequired))
						{
							wave->state = ShooterWave::State::BeforeAttack;
						}
						if (! wave->readyToAttack && done_distance(wave, ap->readyToAttackOnDistance.get(min(ap->attackOnDistance.get() * 3.0f, ap->attackOnDistance.get() + 5.0f)), NP, ap->attackJustOneRequired))
						{
							shouldBeReadyToAttack = true;
						}
					}
					else
					{
						wave->state = ShooterWave::State::BeforeAttack;
						shouldBeReadyToAttack = true;
					}

					if (shouldBeReadyToAttack && ! wave->readyToAttack)
					{
						wave->readyToAttack = shouldBeReadyToAttack;
						for_every(shooter, wave->shooters)
						{
							if (auto* simo = shooter->shooter.get())
							{
								if (Framework::AI::Message* message = simo->get_in_world()->create_ai_message(NAME(aim_sw_attack)))
								{
									message->access_param(NAME(targetIMO)).access_as<SafePtr<Framework::IModulesOwner>>() = targetIMO;
									if (shooterWavesManagerData->targetPlacementSocket.is_valid())
									{
										message->access_param(NAME(targetPlacementSocket)).access_as<Name>() = shooterWavesManagerData->targetPlacementSocket;
									}
									message->access_param(NAME(readyToAttack)).access_as<bool>() = true;
									message->access_param(NAME(shootStartOffsetPt)).access_as<float>() = 1.0f; // this is to force to use full offset
									message->access_param(NAME(shootStartOffsetTS)).access_as<Vector3>() = ap->shootStartOffsetTS.get(Vector3::zero);
									message->to_ai_object(simo);
								}
							}
						}
					}
				}
				if (wave->state == ShooterWave::State::BeforeAttack)
				{
					if (ended)
					{
						wave->state = ShooterWave::State::Ended;
					}
					else if (ap->beforeAttackDelay.is_set())
					{
						if (!wave->timeLeft.is_set())
						{
							wave->timeLeft = ap->beforeAttackDelay.get();
						}
						else
						{
							wave->timeLeft = wave->timeLeft.get() - _deltaTime;
							if (wave->timeLeft.get() <= 0.0f)
							{
								wave->timeLeft.clear();
								wave->state = ShooterWave::State::Attack;
							}
						}
					}
					else
					{
						wave->state = ShooterWave::State::Attack;
					}
				}
				if (wave->state == ShooterWave::State::Attack)
				{
					float attackLength = ap->attackLength.get(1.0f);
					{
						Optional<bool> attackMessage;
						if (ended)
						{
							wave->timeLeft.clear();
							wave->state = ShooterWave::State::Ended;
							attackMessage = false;
						}
						else if (!wave->timeLeft.is_set())
						{
							wave->timeLeft = attackLength;
							attackMessage = true;
						}
						else
						{
							wave->timeLeft = wave->timeLeft.get() - _deltaTime;
							if (wave->timeLeft.get() <= 0.0f)
							{
								wave->timeLeft.clear();
								wave->state = ShooterWave::State::PostAttack;
								attackMessage = false;
							}
						}
						if (attackMessage.is_set())
						{
							for_every(shooter, wave->shooters)
							{
								if (auto* simo = shooter->shooter.get())
								{
									if (Framework::AI::Message* message = simo->get_in_world()->create_ai_message(NAME(aim_sw_attack)))
									{
										message->access_param(NAME(targetIMO)).access_as<SafePtr<Framework::IModulesOwner>>() = targetIMO;
										if (shooterWavesManagerData->targetPlacementSocket.is_valid())
										{
											message->access_param(NAME(targetPlacementSocket)).access_as<Name>() = shooterWavesManagerData->targetPlacementSocket;
										}
										message->access_param(NAME(attack)).access_as<bool>() = attackMessage.get();
										message->access_param(NAME(attackLength)).access_as<float>() = attackLength;
										message->access_param(NAME(shootStartOffsetPt)).access_as<float>() = ap->shootStartOffsetPt.get(1.0f);
										message->access_param(NAME(shootStartOffsetTS)).access_as<Vector3>() = ap->shootStartOffsetTS.get(Vector3::zero);
										message->to_ai_object(simo);
									}
								}
							}
						}
					}
				}
				if (wave->state == ShooterWave::State::PostAttack)
				{
					if (ended)
					{
						wave->state = ShooterWave::State::Ended;
					}
					else if (ap->postAttackDelay.is_set())
					{
						if (!wave->timeLeft.is_set())
						{
							wave->timeLeft = ap->postAttackDelay.get();
						}
						else
						{
							wave->timeLeft = wave->timeLeft.get() - _deltaTime;
							if (wave->timeLeft.get() <= 0.0f)
							{
								wave->timeLeft.clear();
								wave->state = ShooterWave::State::Ended;
							}
						}
					}
					else
					{
						wave->state = ShooterWave::State::Ended;
					}
				}
				if (wave->state == ShooterWave::State::WaitForDisappear)
				{
					bool makeThemDisappear = false;
					{
						an_assert(ap->disappearOnDistance.is_set() ||
								  ap->disappearOnDistanceAway.is_set())
						if (done_distance(wave, ap->disappearOnDistance, ap->disappearOnDistanceAway))
						{
							makeThemDisappear = true;
						}
					}
					if (makeThemDisappear)
					{
						wave->state = ShooterWave::State::Ended;
						for_every(shooter, wave->shooters)
						{
							if (auto* simo = shooter->shooter.get())
							{
								simo->set_timer(1 /* one frame */,
									[](Framework::IModulesOwner* _imo)
									{
										if (_imo)
										{
											if (auto* o = _imo->get_as_object())
											{
												o->cease_to_exist(true);
											}
										}
									});
							}
						}
					}
				}
				if (wave->state == ShooterWave::State::Ended)
				{
					if (auto* ap = wave->currentActionPoint.get())
					{
						wave->state = ShooterWave::State::Started;
						Array<ShooterActionPoint*> aps;
						if (!ap->exit)
						{
							for_every_ref(nap, actionPoints)
							{
								if (ap->continueWithTagged.check(nap->tags) &&
									ap->slotPoints.get_size() >= wave->slotCount)
								{
									aps.push_back(nap);
								}
							}
						}
						if (aps.is_empty())
						{
							wave->currentActionPoint.clear();
						}
						else
						{
							wave->currentActionPoint = aps[random.get_int(aps.get_size())];
							for_every(shooter, wave->shooters)
							{
								if (auto* simo = shooter->shooter.get())
								{
									shooter->placementOnActionPointStart = simo->get_presence()->get_placement();
								}
							}
							if (wave->currentActionPoint->disappearOnDistance.is_set() ||
								wave->currentActionPoint->disappearOnDistanceAway.is_set())
							{
								wave->readyToAttack = false;
							}
						}
					}
				}
			}
		}
		for_every_ref(wave, waves)
		{
			bool shouldDelete = false;
			if (auto* ap = wave->currentActionPoint.get())
			{
				if (!ap->entry)
				{
					bool anyAlive = false;
					for_every(shooter, wave->shooters)
					{
						if (auto* simo = shooter->shooter.get())
						{
							if (auto* h = simo->get_custom<CustomModules::Health>())
							{
								if (h->get_health().is_positive())
								{
									anyAlive = true;
								}
							}
						}
					}
					if (!anyAlive)
					{
						shouldDelete = true;
					}
				}
			}
			if (! wave->currentActionPoint.is_set() || shouldDelete)
			{
				if (currentWave == wave)
				{
					currentWave.clear();
				}
				waves.remove_at(for_everys_index(wave));
				break;
			}
			if (wave->waitToStart > 0.0f)
			{
				spawning = true;
				nextWaveTimeLeft.clear(); // we will restart it when we finish spawning
			}
		}

		if (!spawning)
		{
			if (!nextWaveTimeLeft.is_set() && !shooterWavesManagerData->waveInterval.is_empty())
			{
				nextWaveTimeLeft = random.get(shooterWavesManagerData->waveInterval);
				if (!shooterWavesManagerData->waveIntervalQuick.is_empty())
				{
					float quickTimeLeft = random.get(shooterWavesManagerData->waveIntervalQuick);
					float useSlow = clamp(GameSettings::get().difficulty.aiReactionTime, 0.0f, 1.0f);
					nextWaveTimeLeft = lerp(useSlow, quickTimeLeft, nextWaveTimeLeft.get());
				}
			}
		}
	}
}

bool ShooterWavesManager::done_distance(ShooterWave* wave, Optional<float> const & _distanceClose, Optional<float> const& _distanceAway, Optional<bool> const & _justOneRequired)
{
	bool justOneRequired = _justOneRequired.get(false);
	if (auto* ap = wave->currentActionPoint.get())
	{
		int failed = 0;
		int match = 0;
		for_every(shooter, wave->shooters)
		{
			if (auto* simo = shooter->shooter.get())
			{
				Vector3 loc = simo->get_presence()->get_placement().get_translation();
				int shooterIdx = for_everys_index(shooter);
				Transform targetPlacement = calculate_target_placement(wave, shooterIdx, false);
				Vector3 targetLoc = targetPlacement.get_translation();

				an_assert(ap->slotPoints.is_index_valid(mod(shooterIdx + wave->slotIdxOffset, ap->slotPoints.get_size())));
				float distSq = (loc - targetLoc).length_squared();
				if (_distanceClose.is_set())
				{
					if (distSq > sqr(_distanceClose.get()))
					{
						++failed;
						continue;
					}
				}
				if (_distanceAway.is_set())
				{
					if (distSq < sqr(_distanceAway.get()))
					{
						++failed;
						continue;
					}
				}
				match++;
			}
		}

		return (justOneRequired && match) ||
			  (!justOneRequired && !failed);
	}
	return true;
}

Transform ShooterWavesManager::calculate_target_placement(ShooterWave* wave, int _shooterIdx, bool _getOrientation)
{
	if (auto* ap = wave->currentActionPoint.get())
	{
		Transform targetPlacement = Transform::identity;
		if (auto* rimo = ap->relativeToObject.get())
		{
			targetPlacement = rimo->get_presence()->get_placement().to_world(targetPlacement);
		}

		an_assert(ap->slotPoints.is_index_valid(mod(_shooterIdx + wave->slotIdxOffset, ap->slotPoints.get_size())));
		auto& sp = ap->slotPoints[mod(_shooterIdx + wave->slotIdxOffset, ap->slotPoints.get_size())];
		Vector3 loc = sp.location.is_set()? sp.location.get() : wave->shooters[_shooterIdx].placementOnActionPointStart.get(Transform::identity).get_translation();
		targetPlacement.set_translation(targetPlacement.location_to_world(loc));

		if (_getOrientation)
		{
			if (auto* rimo = ap->relativeToObject.get())
			{
				Vector3 velocity = rimo->get_presence()->get_velocity_linear();
				if (!velocity.is_zero())
				{
					targetPlacement = look_at_matrix(targetPlacement.get_translation(), velocity.normal_2d(), Vector3::zAxis).to_transform();
				}
			}
		}

		return targetPlacement;
	}
	error(TXT("no action point"));
	return Transform::identity;
}

void ShooterWavesManager::debug_draw(Framework::Room* _room) const
{
/*
#ifdef AN_DEBUG_RENDERER
	debug_context(_room);

	{
		Concurrency::ScopedSpinLock lock(shootersLock);

		for_every(shooter, shooters)
		{
			float _scale = 0.1f;
			Vector3 loc = shooter->location * _scale;
			Vector3 undLoc = loc * Vector3::xy;
			Colour colour = Colour::green;
			if (shooter->location.z > 300.0f)
			{
				colour = Colour::orange;
			}
			if (shooter->location.z > 1000.0f)
			{
				colour = Colour::blue;
			}
			debug_draw_line(true, colour.with_alpha(0.5f), Vector3::zero, undLoc);
			debug_draw_line(true, colour.with_alpha(0.5f), undLoc, loc);
			debug_draw_sphere(true, true, colour, 0.5f, Sphere(loc, 0.2f));
			debug_draw_arrow(true, colour, loc, loc + Rotator3(0.0f, shooter->dir, 0.0f).get_forward());
			if (shooter->targetDir.is_set())
			{
				debug_draw_arrow(true, Colour::red, loc, loc + Rotator3(0.0f, shooter->targetDir.get(), 0.0f).get_forward());
			}
		}

	}

	debug_no_context();
#endif
*/
}

//

REGISTER_FOR_FAST_CAST(ShooterWavesManagerData);

ShooterWavesManagerData::ShooterWavesManagerData()
{
}

ShooterWavesManagerData::~ShooterWavesManagerData()
{
}

bool ShooterWavesManagerData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	targetTagged.load_from_xml_attribute_or_child_node(_node, TXT("targetTagged"));
	targetPlacementSocket = _node->get_name_attribute_or_from_child(TXT("targetPlacementSocket"), targetPlacementSocket);

	waveInterval.load_from_xml(_node, TXT("waveInterval"));
	waveIntervalQuick.load_from_xml(_node, TXT("waveIntervalQuick"));
	waveCount.load_from_xml(_node, TXT("waveCount"));

	waves.clear();
	if (!waveCount.is_empty())
	{
		waves.make_space_for(waveCount.max);
	}

	for_every(node, _node->children_named(TXT("wave")))
	{
		Wave wave;
		wave.slotCount.load_from_xml(node, TXT("useSlots"));
		wave.forceActionPoint = node->get_name_attribute_or_from_child(TXT("forceActionPoint"), wave.forceActionPoint);
		int amount = max(1, node->get_int_attribute(TXT("amount"), 1));
		if (!waveCount.is_empty())
		{
			amount = min(amount, waveCount.max);
		}
		for_count(int, i, amount)
		{
			waves.push_back(wave);
		}
	}

	bool hasEntryNode = false;
	for_every(node, _node->children_named(TXT("actionPoint")))
	{
		RefCountObjectPtr<ShooterActionPoint> ap;
		ap = new ShooterActionPoint();
		if (ap->load_from_xml(node, _lc))
		{
			actionPoints.push_back(ap);
			hasEntryNode |= ap->entry;
		}
		else
		{
			result = false;
		}
	}

	//error_loading_xml_on_assert(!targetTagged.is_empty(), result, _node, TXT("no targetTagged"));
	error_loading_xml_on_assert(!actionPoints.is_empty(), result, _node, TXT("no actionPoint nodes"));
	error_loading_xml_on_assert(hasEntryNode, result, _node, TXT("no entry actionPoint"));

	return result;
}

bool ShooterWavesManagerData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every_ref(ap, actionPoints)
	{
		result &= ap->prepare_for_game(_library, _pfgContext);
	}

	return result;
}
