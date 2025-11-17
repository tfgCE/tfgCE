#include "aiLogic_barrel.h"

#include "components\aiLogicComponent_confussion.h"

#include "tasks\aiLogicTask_wander.h"

#include "..\aiCommon.h"
#include "..\aiCommonVariables.h"

#include "..\..\library\library.h"
#include "..\..\game\game.h"
#include "..\..\modules\custom\health\mc_health.h"
#include "..\..\modules\gameplay\moduleCorrosionLeak.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiSocial.h"
#include "..\..\..\framework\module\moduleAI.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleCollision.h"
#include "..\..\..\framework\module\moduleController.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\nav\navTask.h"
#include "..\..\..\framework\nav\tasks\navTask_FindPath.h"
#include "..\..\..\framework\nav\navLatentUtils.h"
#include "..\..\..\framework\object\temporaryObject.h"
#include "..\..\..\framework\world\roomUtils.h"

#include "..\..\modules\custom\mc_emissiveControl.h"

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

//#define DEBUG_DRAW_THROWING

//

// ai message names
DEFINE_STATIC_NAME(dealtDamage);
DEFINE_STATIC_NAME(die);

// ai message params
DEFINE_STATIC_NAME(damageSource);
DEFINE_STATIC_NAME(location);
DEFINE_STATIC_NAME(throughDoor);
DEFINE_STATIC_NAME(room);
DEFINE_STATIC_NAME(who);

// global references
DEFINE_STATIC_NAME_STR(grCorrosionLeak, TXT("corrosion leak"));

// cooldowns
DEFINE_STATIC_NAME(collision);

// temporary objects
DEFINE_STATIC_NAME(dying);
DEFINE_STATIC_NAME(explode);

// variables
DEFINE_STATIC_NAME(stopBarrel);
DEFINE_STATIC_NAME(corrosionLeaks);
DEFINE_STATIC_NAME(corrosionLeakInstigator);
DEFINE_STATIC_NAME(characterBodyRadius);
DEFINE_STATIC_NAME(arm_rf_off);
DEFINE_STATIC_NAME(arm_lf_off);
DEFINE_STATIC_NAME(arm_rm_off);
DEFINE_STATIC_NAME(arm_lm_off);
DEFINE_STATIC_NAME(arm_rb_off);
DEFINE_STATIC_NAME(arm_lb_off);

// bones
DEFINE_STATIC_NAME(barrel);

// sockets
DEFINE_STATIC_NAME(barrel_bottom);
DEFINE_STATIC_NAME(barrel_top);

//

REGISTER_FOR_FAST_CAST(Barrel);

Barrel::Barrel(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	corrosionLeakSpawnContext = new CorrosionLeakSpawnContext();
}

Barrel::~Barrel()
{
}

void Barrel::play_sound(Name const & _sound)
{
	if (auto* s = get_mind()->get_owner_as_modules_owner()->get_sound())
	{
		s->play_sound(_sound);
	}
}

void Barrel::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	if (corrosionLeakSpawnContext->corrosionLeaksQueued > 0 &&
		! corrosionLeakSpawnContext->spawningCorrosionLeak &&
		get_mind())
	{
		--corrosionLeakSpawnContext->corrosionLeaksQueued;
		corrosionLeakSpawnContext->spawningCorrosionLeak = true;

		RefCountObjectPtr<CorrosionLeakSpawnContext> corrosionLeakSpawnContextKeeper = corrosionLeakSpawnContext;
		SafePtr<Framework::IModulesOwner> owner;
		owner = get_mind()->get_owner_as_modules_owner();
		Game::get()->add_async_world_job_top_priority(TXT("spawn corrosion leak"), [corrosionLeakSpawnContextKeeper, owner]
		{
			if (owner.is_set()) // should catch if owner was deleted
			{
				if (auto* a = owner->get_appearance())
				{
					float bodyRadius = owner->get_variables().get_value(NAME(characterBodyRadius), 0.0f);
					if (bodyRadius > 0.0f)
					{
						Transform barrelBottomOS = a->calculate_socket_os(NAME(barrel_bottom));
						Transform barrelTopOS = a->calculate_socket_os(NAME(barrel_top));

						Vector3 dir = (barrelTopOS.get_translation() - barrelBottomOS.get_translation()).normal();
						float offset = 0.1f;
						Vector3 start = barrelBottomOS.get_translation() + dir * offset;
						Vector3 end = barrelTopOS.get_translation() - dir * offset;

						auto rg = owner->get_individual_random_generator();
						rg.advance_seed(corrosionLeakSpawnContextKeeper->corrosionLeaksSpawned * 9725, 12974);

						float atPt = rg.get_float(0.0f, 1.0f);
						float atYaw = rg.get_float(0.0f, 360.0f);

						Transform upPlacement = up_matrix33(dir).to_transform();
						Vector3 at = start * (1.0f - atPt) + atPt * end;
						Vector3 normal = upPlacement.vector_to_world(Vector3::yAxis.rotated_by_yaw(atYaw));
						at += normal * bodyRadius;

						Transform placement = up_matrix33(normal).to_transform();
						placement.set_translation(at);

						Framework::SceneryType* corrosionLeakType = Library::get_current()->get_global_references().get<Framework::SceneryType>(NAME(grCorrosionLeak));
					
						if (corrosionLeakType)
						{
							corrosionLeakType->load_on_demand_if_required();

							auto* game = Game::get();

							Framework::ScopedAutoActivationGroup saag(owner->get_in_world());

							SafePtr<Framework::IModulesOwner> object;
							game->perform_sync_world_job(TXT("spawn"), [&object, corrosionLeakType, owner, corrosionLeakSpawnContextKeeper]()
							{
								scoped_call_stack_info(TXT("spawning corrosion leak"));
								scoped_call_stack_info_str_printf(TXT("corrosionLeakType: %S"), corrosionLeakType? corrosionLeakType->get_name().to_string().to_char() : TXT("--"));
								scoped_call_stack_info_str_printf(TXT("owner: %S"), owner.get() ? owner->ai_get_name().to_char() : TXT("--"));
								if (owner.get())
								{
									if (auto* c = corrosionLeakSpawnContextKeeper.get())
									{
										object = corrosionLeakType->create(owner->ai_get_name() + String::printf(TXT("leak %i"), c->corrosionLeaksSpawned));
										object->init(owner->get_in_sub_world());
									}
								}
							});

							if (object.get())
							{
								object->access_variables().set_from(owner->get_variables());
								object->access_individual_random_generator() = rg;

								object->set_instigator(owner.get());

								if (auto* r = owner->get_presence()->get_in_room())
								{
									object->set_fallback_wmp_owner(r);
								}
								object->initialise_modules();
							}

							game->perform_sync_world_job(TXT("place"), [owner, object, placement]()
							{
								if (object.get())
								{
									if (owner.get())
									{
										object->get_presence()->attach_to_bone_using_in_room_placement(owner.get(), NAME(barrel), false, owner->get_presence()->get_placement().to_world(placement), true);
										object->get_presence()->cease_to_exist_when_not_attached(true);
									}
									else
									{
										object->cease_to_exist(true);
									}
								}
							});

							if (object.get())
							{
								object->be_non_autonomous();
							}

							// auto activate (if was destroyed, will be skipped)

							if (auto* c = corrosionLeakSpawnContextKeeper.get())
							{
								++c->corrosionLeaksSpawned;
							}
						}
						else
						{
							error(TXT("\"corrosion leak\" global reference not found (at least of scenery type)"));
						}
					}
					else
					{
						error(TXT("\"characterBodyRadius\" not set"));
					}
				}
			}

			if (auto* c = corrosionLeakSpawnContextKeeper.get())
			{
				c->spawningCorrosionLeak = false;
			}
		});
	}
}

void Barrel::drop_target()
{
	runAwayFrom.reset();
	runAwayNow = false;
}

void Barrel::run_away_from(Framework::IModulesOwner const* _from)
{
	if (!get_mind())
	{
		return;
	}

	runAwayFrom.reset();
	if (runAwayFrom.find_path(get_mind()->get_owner_as_modules_owner(), cast_to_nonconst(_from), false, 16))
	{
		runAwayNow = true;
	}
	else
	{
		runAwayNow = false;
	}
}

void Barrel::add_corrosion_leak()
{
	++corrosionLeakSpawnContext->corrosionLeaksQueued;
}

LATENT_FUNCTION(Barrel::perception_blind)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai barrel] perception blind"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("perception blind"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::CooldownsSmall, cooldowns);
	LATENT_VAR(::Framework::IModulesOwner const *, lastTarget);
	LATENT_VAR(::Framework::IModulesOwner const *, newTarget);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	LATENT_VAR(Confussion, confussion);

	auto* imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<Barrel>(mind->get_logic());

	confussion.advance(LATENT_DELTA_TIME);

	cooldowns.advance(LATENT_DELTA_TIME);

	LATENT_BEGIN_CODE();

	confussion.setup(mind);

	messageHandler.use_with(mind);
	{
		auto * framePtr = &_frame;
		messageHandler.set(NAME(dealtDamage), [framePtr, mind, self](Framework::AI::Message const & _message)
		{
			if (auto * source = _message.get_param(NAME(damageSource)))
			{
				if (auto* damageInstigator = source->get_as<SafePtr<Framework::IModulesOwner>>().get())
				{
					ai_log_colour(mind->get_logic(), Colour::cyan);
					ai_log(mind->get_logic(), TXT("[perception] damage source (%S)"), damageInstigator->ai_get_name().to_char());
					ai_log_no_colour(mind->get_logic());
					damageInstigator = damageInstigator->get_instigator_first_actor_or_valid_top_instigator();

					self->run_away_from(damageInstigator);
				}
			}
			framePtr->end_waiting();
			AI_LATENT_STOP_LONG_RARE_ADVANCE();
		}
		);
	}

	while (true)
	{
		if (confussion.is_confused())
		{
			self->drop_target();
		}
		else if (cooldowns.is_available(NAME(collision)))
		{
			PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai barrel] collision"));
			if (auto* mc = imo->get_collision())
			{
				if (auto* ai = imo->get_ai())
				{
					auto const & aiSocial = ai->get_mind()->get_social();
					if (!self->runAwayNow)
					{
						lastTarget = nullptr;
					}
					newTarget = lastTarget;
					for_every(oc, mc->get_collided_with())
					{
						if (Framework::IModulesOwner const * object = fast_cast<Framework::IModulesOwner>(oc->collidedWithObject.get()))
						{
							object = object->get_instigator_first_actor_or_valid_top_instigator();

							if (aiSocial.is_enemy(object))
							{
								cooldowns.set(NAME(collision), 1.0f);

								::Framework::RelativeToPresencePlacement r2pp;
								r2pp.be_temporary_snapshot(); 
								if (r2pp.find_path(imo, cast_to_nonconst(object)))
								{
									Vector3 relLocation = r2pp.get_target_centre_relative_to_owner();
									Vector3 flatDir = (relLocation * Vector3::xy).normal();
									ai_log(mind->get_logic(), TXT("flatDir.y %.3f"), flatDir.y);
									if (flatDir.y > -0.4f)
									{
										if (!lastTarget || Random::get_chance(0.2f))
										{
											ai_log(mind->get_logic(), TXT("collided, target!"));
											lastTarget = object;
											self->run_away_from(object);
										}
									}
								}
							}
						}
					}
					lastTarget = newTarget;
				}
			}
		}

		LATENT_WAIT(Random::get_float(0.1f, 0.3f));
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	//

	LATENT_RETURN();
}

LATENT_FUNCTION(Barrel::run_away)
{
	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("[barrel] run away"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::Nav::PathPtr, path);
	LATENT_VAR(::Framework::Nav::TaskPtr, pathTask);

	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<Barrel>(mind->get_logic());

	LATENT_BEGIN_CODE();

	ai_log_colour(mind->get_logic(), Colour::blue);
	ai_log(mind->get_logic(), TXT("run away"));
	ai_log_no_colour(mind->get_logic());

	while (self->runAwayNow)
	{
		LATENT_CLEAR_LOG();
		LATENT_LOG(TXT("[%i]"), System::Core::get_frame());
		if (!pathTask.is_set())
		{
			auto& locomotion = mind->access_locomotion();
			if (locomotion.is_done_with_move() || locomotion.has_finished_move(1.0f))
			{
				Framework::Nav::PathRequestInfo pathRequestInfo(imo);
				pathRequestInfo.with_dev_info(TXT("barrel run away"));
				if (!Mobility::may_leave_room_to_wander(imo, imo->get_presence()->get_in_room()))
				{
					pathRequestInfo.within_same_room_and_navigation_group();
				}

				auto* presence = imo->get_presence();
				if (mind->access_navigation().is_at_transport_route())
				{
					LATENT_LOG(TXT("run away -- transport route"));
					if (! pathTask.is_set())
					{
						LATENT_LOG(TXT("go through random door but not the one we've entered"));
						pathTask = mind->access_navigation().find_path_through(Framework::RoomUtils::get_random_door(presence->get_in_room(), imo->get_ai()->get_entered_through_door()), 0.5f, pathRequestInfo);
					}
				}
				else
				{
					if (!pathTask.is_set() && self->runAwayFrom.is_active())
					{
						LATENT_LOG(TXT("run away from"));
						Vector3 towards = self->runAwayFrom.get_direction_towards_placement_in_owner_room();
						pathTask = mind->access_navigation().get_random_path(Random::get_float(5.0f, 20.0f), -towards, pathRequestInfo);
					}
				}
			}
		}
		
		if (pathTask.is_set())
		{
			LATENT_LOG(TXT("wait for nav task to end"));

			LATENT_NAV_TASK_WAIT_IF_SUCCEED(pathTask)
			{
				if (auto* task = fast_cast<Framework::Nav::Tasks::PathTask>(pathTask.get()))
				{
					LATENT_LOG(TXT("got a path"));
					path = task->get_path();
				}
				else
				{
					LATENT_LOG(TXT("what?"));
					path.clear();
				}
			}
			else
			{
				LATENT_LOG(TXT("failed"));
				path.clear();
			}
			pathTask = nullptr;
		}

		MOVE:
		{
			{
				auto& locomotion = mind->access_locomotion();
				if (! locomotion.check_if_path_is_ok(path.get()))
				{
					locomotion.dont_move();
					goto MOVED;
				}
				else if (path.get() && !path->is_close_to_the_end(imo->get_presence()->get_in_room(), imo->get_presence()->get_placement().get_translation(), 0.25f))
				{
					if (!locomotion.is_following_path(path.get()))
					{
						LATENT_LOG(TXT("move"));
						locomotion.follow_path_2d(path.get(), NP, Framework::MovementParameters().full_speed());
						locomotion.turn_follow_path_2d();
					}
				}
				else
				{
					locomotion.dont_move();
				}
			}

			{
				auto& locomotion = mind->access_locomotion();
				if (locomotion.has_finished_move(0.5f))
				{
					LATENT_LOG(TXT("reached destination"));
					locomotion.dont_move();
					goto MOVED;
				}
			}
			LATENT_YIELD();
			//LATENT_WAIT(Random::get_float(0.1f, 0.3f));
		}
		goto MOVE;

		MOVED:

		self->runAwayNow = false;
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	{
		auto & locomotion = mind->access_locomotion();
		locomotion.stop();
	}

	self->runAwayNow = false;
	self->runAwayFrom.reset();

	LATENT_END_CODE();
	LATENT_RETURN();
}

static LATENT_FUNCTION(death_sequence)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai barrel] death sequence"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("death sequence"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::LatentTaskHandle*, thisTaskHandle); // auto

	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::IModulesOwner*, imo);
	LATENT_VAR(SafePtr<Framework::IModulesOwner>, dying);
	LATENT_VAR(::System::TimeStamp, timePassed);

	LATENT_BEGIN_CODE();

	ai_log(mind->get_logic(), TXT("death sequence"));

	imo = mind->get_owner_as_modules_owner();

	{
		imo->get_appearance()->access_controllers_variable_storage().access<float>(NAME(stopBarrel)) = 1.0f;
	}
	
	if (auto* em = imo->get_custom<CustomModules::EmissiveControl>())
	{
		em->emissive_deactivate_all();
	}

	{
		auto & locomotion = mind->access_locomotion();
		locomotion.stop();
		locomotion.dont_control();
		if (auto* c = imo->get_controller())
		{
			c->set_requested_relative_velocity_orientation(Rotator3(0.0f, Random::get_float(0.6f, 1.6f) /* a bit faster than 100% */ * (Random::get_chance(0.6f /* turn right, as the barrel inside does */)? 1.0f: -1.0f), 0.0f));
		}
	}

	if (auto* tos = imo->get_temporary_objects())
	{
		dying = tos->spawn_attached(NAME(dying));
	}

	timePassed.reset();

	while (timePassed.get_time_since() < 2.0f)
	{
		LATENT_WAIT(min(2.0f - timePassed.get_time_since(), Random::get_float(0.3f, 1.0f)));
		{
			Concurrency::ScopedSpinLock lock(imo->get_presence()->access_attached_lock());
			ARRAY_STACK(ModuleCorrosionLeak*, notLeaking, 10);
			for_every_ptr(att, imo->get_presence()->get_attached())
			{
				if (auto* cl = att->get_gameplay_as<ModuleCorrosionLeak>())
				{
					if (!cl->is_leaking())
					{
						notLeaking.push_back_if_possible(cl);
					}
				}
			}
			if (!notLeaking.is_empty())
			{
				notLeaking[Random::get_int(notLeaking.get_size())]->leak_now(DamageInfo());
			}
			if (auto* c = imo->get_controller())
			{
				c->set_requested_relative_velocity_orientation(Rotator3(0.0f, Random::get_float(0.6f, 1.6f) /* a bit faster than 100% */ * (Random::get_chance(0.6f /* turn right, as the barrel inside does */) ? 1.0f : -1.0f), 0.0f));
			}
		}
	}

	{
		Concurrency::ScopedSpinLock lock(imo->get_presence()->access_attached_lock());
		for_every_ptr(att, imo->get_presence()->get_attached())
		{
			if (auto* cl = att->get_gameplay_as<ModuleCorrosionLeak>())
			{
				cl->leak_now(DamageInfo());
			}
		}
	}
	timePassed.reset();

	while (timePassed.get_time_since() < 1.0f)
	{
		LATENT_WAIT(min(1.0f - timePassed.get_time_since(), Random::get_float(0.2f, 0.7f)));
		{
			if (auto* c = imo->get_controller())
			{
				c->set_requested_relative_velocity_orientation(Rotator3(0.0f, Random::get_float(0.6f, 1.6f) /* a bit faster than 100% */ * (Random::get_chance(0.6f /* turn right, as the barrel inside does */) ? 1.0f : -1.0f), 0.0f));
			}
		}
	}

	if (auto* tos = imo->get_temporary_objects())
	{
		Vector3 explosionLocOS = imo->get_presence()->get_random_point_within_presence_os(0.8f);
		Transform explosionWS = imo->get_presence()->get_placement().to_world(Transform(explosionLocOS, Quat::identity));
		Framework::Room* inRoom;
		imo->get_presence()->move_through_doors(explosionWS, OUT_ inRoom);
		tos->spawn_in_room(NAME(explode), inRoom, explosionWS);
	}

	ai_log(mind->get_logic(), TXT("perform death"));
	if (auto* h = imo->get_custom<CustomModules::Health>())
	{
		Framework::IModulesOwner* deathInstigator = nullptr;
		{
			SafePtr<Framework::IModulesOwner> di;
			auto* imo = mind->get_owner_as_modules_owner();
			di = imo->get_variables().get_value<SafePtr<Framework::IModulesOwner>>(NAME(corrosionLeakInstigator), SafePtr<Framework::IModulesOwner>());
			deathInstigator = di.get();
		}
		h->setup_death_params(false, false, deathInstigator);
		h->perform_death(deathInstigator);
	}

	LATENT_ON_BREAK();
	//
	if (mind)
	{
		::Framework::AI::Locomotion & locomotion = mind->access_locomotion();
		locomotion.stop();
	}

	LATENT_END_CODE();
	LATENT_RETURN();
}

LATENT_FUNCTION(Barrel::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai barrel] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_COMMON_VAR_BEGIN();
	LATENT_COMMON_VAR(Name, scriptedBehaviour);
	LATENT_COMMON_VAR_END();
	
	LATENT_VAR(::Framework::AI::LatentTaskHandle, perceptionTask);
	LATENT_VAR(::Framework::AI::LatentTaskHandle, hatchTask);

	LATENT_VAR(::Framework::AI::LatentTaskHandle, currentTask);

	LATENT_VAR(bool, emissiveFriendly);

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	LATENT_VAR(bool, dieNow);
	
	auto* imo = mind->get_owner_as_modules_owner();
	auto* self = fast_cast<Barrel>(logic);

	LATENT_BEGIN_CODE();

	{
		::Framework::AI::LatentTaskInfoWithParams taskInfo;
		taskInfo.propose(AI_LATENT_TASK_FUNCTION(perception_blind));
		perceptionTask.start_latent_task(mind, taskInfo);
	}

	emissiveFriendly = false;

	messageHandler.use_with(mind);
	{
		auto * framePtr = &_frame;
		messageHandler.set(NAME(die), [framePtr, mind, &dieNow](Framework::AI::Message const & _message)
		{
			dieNow = true;
			framePtr->end_waiting();
			AI_LATENT_STOP_LONG_RARE_ADVANCE();
		}
		);
	}

	{
		int corrosionLeaksToSpawn = Random::get_int_from_range(2, 3);
		while (corrosionLeaksToSpawn)
		{
			self->add_corrosion_leak();
			--corrosionLeaksToSpawn;
		}
	}
	

	//LATENT_WAIT(1.0f);
	//dieNow = true;

	while (true)
	{
		// check if we should die
		{
			{
				if (imo->get_variables().get_value<int>(NAME(corrosionLeaks), 0) > 0)
				{
					dieNow = true;
				}
			}
			{
				int legsOff = 0;
				legsOff += imo->get_variables().get_value<bool>(NAME(arm_rf_off), 0) ? 1 : 0;
				legsOff += imo->get_variables().get_value<bool>(NAME(arm_lf_off), 0) ? 1 : 0;
				legsOff += imo->get_variables().get_value<bool>(NAME(arm_rm_off), 0) ? 1 : 0;
				legsOff += imo->get_variables().get_value<bool>(NAME(arm_lm_off), 0) ? 1 : 0;
				legsOff += imo->get_variables().get_value<bool>(NAME(arm_rb_off), 0) ? 1 : 0;
				legsOff += imo->get_variables().get_value<bool>(NAME(arm_lb_off), 0) ? 1 : 0;
				if (legsOff >= 3)
				{
					dieNow = true;
				}
			}
		}
		if (dieNow)
		{
			{
				::Framework::AI::LatentTaskInfoWithParams nextTask;
				nextTask.propose(AI_LATENT_TASK_FUNCTION(death_sequence), NP, NP, false, true);
				currentTask.start_latent_task(mind, nextTask);
			}
			while (true)
			{
				// just wait here until we die
				LATENT_YIELD();
			}
		}
		if (Common::handle_scripted_behaviour(mind, scriptedBehaviour, currentTask))
		{
			// doing scripted behaviour
		}
		else
		{
			::Framework::AI::LatentTaskInfoWithParams nextTask;
			// choose best action for now
			nextTask.propose(AI_LATENT_TASK_FUNCTION(Tasks::wander));

			if (self->runAwayNow)
			{
				nextTask.propose(AI_LATENT_TASK_FUNCTION(run_away), 20);
			}

			// check if we can start new one, if so, start it
			if (currentTask.can_start(nextTask))
			{
				currentTask.start_latent_task(mind, nextTask);
			}
		}

		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//
	perceptionTask.stop();
	hatchTask.stop();
	currentTask.stop();

	LATENT_END_CODE();
	LATENT_RETURN();
}
