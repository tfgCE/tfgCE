#include "moduleAI.h"

#include "registeredModule.h"
#include "modules.h"
#include "moduleAIData.h"

#include "..\advance\advanceContext.h"
#include "..\ai\aiLogic.h"
#include "..\ai\aiMindInstance.h"
#include "..\gameScript\gameScript.h"
#include "..\world\doorInRoom.h"
#include "..\world\world.h"

#include "..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

DEFINE_STATIC_NAME(defaultTargetPlacementSocket);
DEFINE_STATIC_NAME(processAIMessages);
DEFINE_STATIC_NAME(ignoreAIMessages);
DEFINE_STATIC_NAME(acceptAIHunches);
DEFINE_STATIC_NAME(ignoreAIHunches);
DEFINE_STATIC_NAME(rareAdvanceLogic);

// game script variables
DEFINE_STATIC_NAME(self);

//

#ifdef AN_DEVELOPMENT
bool aiAdvanceable = true;
#endif

//

REGISTER_FOR_FAST_CAST(ModuleAI);

static ModuleAI* create_module(IModulesOwner* _owner)
{
	return new ModuleAI(_owner);
}

RegisteredModule<ModuleAI> & ModuleAI::register_itself()
{
	return Modules::ai.register_module(String(TXT("base")), create_module);
}

ModuleAI::ModuleAI(IModulesOwner* _owner)
: base(_owner)
, mind(new AI::MindInstance(fast_cast<IAIMindOwner>(_owner)))
, controlledByPlayer(false)
, considerPlayer(false)
{
}

ModuleAI::~ModuleAI()
{
	if (auto* gse = gameScriptExecution.get())
	{
		delete gse;
		gameScriptExecution.clear();
	}

	delete_and_clear(mind);
}

void ModuleAI::on_owner_destroy()
{
	if (auto* gse = gameScriptExecution.get())
	{
		delete gse;
		gameScriptExecution.clear();
	}

	base::on_owner_destroy();
}

void ModuleAI::set_controlled_by_player(bool _controlledByPlayer)
{
	if (controlledByPlayer != _controlledByPlayer)
	{
		controlledByPlayer = _controlledByPlayer;
		reset_ai();
		if (mind)
		{
			mind->use_mind(nullptr); // clear mind, otherwise we may get stuck with non player 
		}
		// setup it again after ai is reset
		setup_with(get_module_data());
	}
}

void ModuleAI::set_consider_player(bool _considerPlayer)
{
	considerPlayer = _considerPlayer;
}

void ModuleAI::reset()
{
	reset_ai();
	base::reset();
}

void ModuleAI::reset_ai()
{
	if (auto* gse = gameScriptExecution.get())
	{
		delete gse;
		gameScriptExecution.clear();
	}

	// reset but keep same mind
	AI::Mind* usedMind = mind ? mind->get_mind() : nullptr;
	delete_and_clear(mind);
	mind = new AI::MindInstance(fast_cast<IAIMindOwner>(get_owner()));
	if (usedMind)
	{
		mind->use_mind(usedMind);
	}
}

void ModuleAI::setup_with(ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	moduleAIData = fast_cast<ModuleAIData>(_moduleData);
	if (_moduleData)
	{
		raLogic.setup_with(this, _moduleData, NAME(rareAdvanceLogic));
		processAIMessages = _moduleData->get_parameter<bool>(this, NAME(processAIMessages), processAIMessages);
		processAIMessages = ! _moduleData->get_parameter<bool>(this, NAME(ignoreAIMessages), ! processAIMessages);
		acceptAIHunches = _moduleData->get_parameter<bool>(this, NAME(acceptAIHunches), acceptAIHunches);
		ignoreAIHunches.load_from_string(_moduleData->get_parameter<String>(this, NAME(ignoreAIHunches), String::empty()));
		if (!mind->get_mind())
		{
			if (controlledByPlayer)
			{
				mind->use_mind(moduleAIData->mindControlledByPlayer.get());
			}
			else
			{
				mind->use_mind(moduleAIData->mind.get());
			}
		}
		defaultTargetPlacementSocket = _moduleData->get_parameter<Name>(this, NAME(defaultTargetPlacementSocket), defaultTargetPlacementSocket);
	}
}

Transform ModuleAI::get_me_as_target_placement_os() const
{
	auto const * imo = get_owner();
	if (auto const * appearance = imo->get_appearance())
	{
		return appearance->calculate_socket_os(defaultTargetPlacementSocket);
	}
	if (auto const * presence = imo->get_presence())
	{
		return presence->get_centre_of_presence_os();
	}
	return Transform::identity;
}

Transform ModuleAI::get_target_placement_os_for(IAIObject const * _target) const
{
	if (auto const * imo = fast_cast<IModulesOwner>(_target))
	{
		if (auto const * ai = imo->get_ai())
		{
			return ai->get_me_as_target_placement_os();
		}
	}
	return Transform::identity;
}

Transform ModuleAI::get_target_placement_for(IAIObject const * _target) const
{
	an_assert(_target);
	if (auto const * imo = fast_cast<IModulesOwner>(_target))
	{
		if (auto const * appearance = imo->get_appearance())
		{
			return appearance->get_os_to_ws_transform().to_world(get_target_placement_os_for(_target));
		}
	}
	return _target->ai_get_placement();
}

bool ModuleAI::does_require_process_ai_messages() const
{
	if (!suspended && mind)
	{
		return mind->access_execution().does_require_process_messages();
	}
	return false;
}

void ModuleAI::advance__process_ai_messages(IMMEDIATE_JOB_PARAMS)
{
#ifdef AN_DEVELOPMENT
	if (!aiAdvanceable)
	{
		return;
	}
#endif
	FOR_EVERY_IMMEDIATE_JOB_DATA(IModulesOwner, modulesOwner)
	{
		MEASURE_PERFORMANCE_CONTEXT(modulesOwner->get_performance_context_info());
		MEASURE_PERFORMANCE(advanceAIProcessMessages);
		if (ModuleAI* self = modulesOwner->get_ai())
		{
			scoped_call_stack_info(TXT("process ai messages"));
			scoped_call_stack_info(modulesOwner->ai_get_name().to_char());
			if (self->mind && !self->suspended)
			{
				self->mind->access_execution().process_messages();
			}
		}
		else
		{
			an_assert(false, TXT("shouldn't be called"));
		}
	}
}

void ModuleAI::advance__advance_perception(IMMEDIATE_JOB_PARAMS)
{
#ifdef AN_DEVELOPMENT
	if (!aiAdvanceable)
	{
		return;
	}
#endif
	FOR_EVERY_IMMEDIATE_JOB_DATA(IModulesOwner, modulesOwner)
	{
		MEASURE_PERFORMANCE_CONTEXT(modulesOwner->get_performance_context_info());
		MEASURE_PERFORMANCE(advanceAIAdvancePerception);
		if (ModuleAI* self = modulesOwner->get_ai())
		{
			if (self->mind && !self->suspended)
			{
				scoped_call_stack_info(modulesOwner->ai_get_name().to_char());
				PERFORMANCE_LIMIT_GUARD_START();
				self->mind->advance_perception();
				PERFORMANCE_LIMIT_GUARD_STOP(0.001f, TXT("advance_perception for \"%S\""), self->get_owner()->ai_get_name().to_char());
			}
		}
		else
		{
			an_assert(false, TXT("shouldn't be called"));
		}
	}
}

void ModuleAI::advance__advance_logic(IMMEDIATE_JOB_PARAMS)
{
#ifdef AN_DEVELOPMENT
	if (!aiAdvanceable)
	{
		return;
	}
#endif
	FOR_EVERY_IMMEDIATE_JOB_DATA(IModulesOwner, modulesOwner)
	{
		MEASURE_PERFORMANCE_CONTEXT(modulesOwner->get_performance_context_info());
		MEASURE_PERFORMANCE(advanceAIAdvanceLogic);
		if (ModuleAI* self = modulesOwner->get_ai())
		{
			scoped_call_stack_ptr(modulesOwner);
			scoped_call_stack_info(modulesOwner->ai_get_name().to_char());
			scoped_call_stack_info(TXT("in room"));
			scoped_call_stack_ptr(modulesOwner->get_presence()? modulesOwner->get_presence()->get_in_room() : nullptr);
			{
				MEASURE_PERFORMANCE(advanceAIAdvanceLogic_nonMindAdvanceLogic);
				self->advance_non_mind_logic();
			}

			if (self->mind && !self->suspended)
			{
				// raLogic is advanced when checking if advance_logic should be called
				if (self->raLogic.should_advance())
				{
					MEASURE_PERFORMANCE(advanceAIAdvanceLogic_mindAdvanceLogic);
					PERFORMANCE_GUARD_LIMIT(0.005f, String::printf(TXT("[AI] %S"), self->get_owner()->ai_get_name().to_char()).to_char());
					PERFORMANCE_LIMIT_GUARD_START();
					self->mind->advance_logic(self->raLogic.get_advance_delta_time());
					PERFORMANCE_LIMIT_GUARD_STOP(0.001f, TXT("advance_logic for \"%S\""), self->get_owner()->ai_get_name().to_char());
				}
			}
		}
		else
		{
			an_assert(false, TXT("shouldn't be called"));
		}
	}
}

void ModuleAI::advance__advance_locomotion(IMMEDIATE_JOB_PARAMS)
{
#ifdef AN_DEVELOPMENT
	if (!aiAdvanceable)
	{
		return;
	}
#endif
	AdvanceContext const * ac = plain_cast<AdvanceContext const>(_executionContext);
	FOR_EVERY_IMMEDIATE_JOB_DATA(IModulesOwner, modulesOwner)
	{
		scoped_call_stack_info(TXT("advance locomotion"));
		scoped_call_stack_info(modulesOwner->ai_get_name().to_char());
#ifdef DEBUG_WORLD_LIFETIME
		scoped_call_stack_info_str_printf(TXT("o%p"), modulesOwner);
#endif
		MEASURE_PERFORMANCE_CONTEXT(modulesOwner->get_performance_context_info());
		MEASURE_PERFORMANCE(advanceAIAdvanceLocomotion);
		if (ModuleAI* self = modulesOwner->get_ai())
		{
			float deltaTime = ac->get_delta_time();

			if (self->mind && !self->suspended)
			{
				PERFORMANCE_LIMIT_GUARD_START();
				// debug note: if you're here and wondering why AI advances rarely, check LATENT_WAIT. maybe it should be LATENT_WAIT_NO_RARE_ADVANCE?
				self->mind->advance_locomotion(deltaTime);
				PERFORMANCE_LIMIT_GUARD_STOP(0.001f, TXT("advance_locomotion for \"%S\""), self->get_owner()->ai_get_name().to_char());
			}
		}
		else
		{
			an_assert(false, TXT("shouldn't be called"));
		}
	}
}

void ModuleAI::start_game_script(GameScript::Script* _gs)
{
	MODULE_OWNER_LOCK(TXT("start_game_script"));

	if (!gameScriptExecution.is_set())
	{
		gameScriptExecution = new GameScript::ScriptExecution();

		// built in, hardcoded variables
		gameScriptExecution->access_variables().access< SafePtr<Framework::IModulesOwner> >(NAME(self)) = get_owner();
	}

	gameScriptExecution->start(_gs);

	access_rare_logic_advance().reset_to_no_rare_advancement();
	access_rare_logic_advance().force_no_rare();
}

void ModuleAI::advance_non_mind_logic()
{
	if (gameScriptExecution.is_set())
	{
		gameScriptExecution->execute();
	}
}

bool ModuleAI::does_handle_ai_message(Name const& _messageName) const
{
	return mind && processAIMessages ? mind->access_execution().does_handle_message(_messageName) : false;
}

AI::MessagesToProcess * ModuleAI::access_ai_messages_to_process()
{
	return mind && processAIMessages ? &mind->access_execution().access_messages_to_process() : nullptr;
}

AI::Hunches * ModuleAI::access_ai_hunches()
{
	return mind && acceptAIHunches ? &mind->access_execution().access_hunches() : nullptr;
}

void ModuleAI::learn_from(SimpleVariableStorage & _parameters)
{
	if (mind)
	{
		mind->learn_from(_parameters);
	}
}

void ModuleAI::ready_to_activate()
{
	base::ready_to_activate();
	if (mind)
	{
		mind->ready_to_activate();
	}
}

bool ModuleAI::does_require_perception_advance() const
{
	return mind && mind->get_perception().does_require_advance();
}

#ifdef AN_DEVELOPMENT
void ModuleAI::set_ai_advanceable(bool _advanceable)
{
	aiAdvanceable = _advanceable;
}

bool ModuleAI::is_ai_advanceable()
{
	return aiAdvanceable;
}
#endif

void ModuleAI::internal_change_room(DoorInRoom* _enterThrough)
{
	enteredThroughDoor = _enterThrough;
}

AI::Hunch* ModuleAI::give_hunch(Name const& _name, Name const & _reason, Optional<float> const& _expirationTime)
{
	if (auto* hunches = access_ai_hunches())
	{
		if (!ignoreAIHunches.get_tag(_name))
		{
			if (AI::Hunch* hunch = AI::Hunch::get_one())
			{
				hunch->name_hunch(_name);
				hunch->set_reason(_reason);

				if (auto* imo = get_owner())
				{
					if (auto* w = imo->get_in_world())
					{
						float wt = w->get_world_time();
						hunch->set_issue_time(wt);
						if (_expirationTime.is_set())
						{
							hunch->set_expire_by(wt + _expirationTime.get());
						}
					}
				}

				hunches->push_back(AI::Hunch::Ptr(hunch));

				return hunch;
			}
		}
	}
	return nullptr;
}

void ModuleAI::log_hunches(LogInfoContext& _log) const
{
	if (mind && acceptAIHunches)
	{
		for_every_ref(hunch, mind->access_execution().access_hunches())
		{
			_log.log(TXT("%S"), hunch->get_name().to_char());
		}
	}
}

void ModuleAI::update_rare_logic_advance(float _deltaTime)
{
	if (mind && mind->get_logic())
	{
		mind->get_logic()->update_rare_logic_advance(_deltaTime);
	}
	else
	{
		// this is done to reset if we swap minds but if we have gamescript execution (game) it breaks it
		access_rare_logic_advance().reset();
	}
}

