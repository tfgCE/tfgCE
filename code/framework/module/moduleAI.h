#pragma once

#include "module.h"
#include "..\gameScript\gameScriptExecution.h"
#include "..\jobSystem\jobSystemsImmediateJobPerformer.h"
#include "..\..\core\math\math.h"

class SimpleVariableStorage;

namespace Concurrency
{
	class JobExecutionContext;
};

namespace Framework
{
	namespace AI
	{
		class MindInstance;
	};

	class DoorInRoom;
	class ModuleAIData;
	class ModulePresence;

	class ModuleAI
	: public Module
	{
		FAST_CAST_DECLARE(ModuleAI);
		FAST_CAST_BASE(Module);
		FAST_CAST_END();
		
		typedef Module base;
	public:
		static RegisteredModule<ModuleAI> & register_itself();

		ModuleAI(IModulesOwner* _owner);
		virtual ~ModuleAI();

		ModuleRareAdvance& access_rare_logic_advance() { return raLogic; }
		void update_rare_logic_advance(float _deltaTime); // updates only raLogic structure depending on whether we see the enemy or not

	public:
		AI::MindInstance * get_mind() const { return mind; }

		bool does_require_perception_advance() const;

		bool is_controlled_by_player() const { return controlledByPlayer; }
		void set_controlled_by_player(bool _controlledByPlayer);
		bool is_considered_player() const { return considerPlayer; }
		void set_consider_player(bool _considerPlayer);
		bool does_handle_ai_message(Name const& _messageName) const;
		AI::MessagesToProcess * access_ai_messages_to_process();
		AI::Hunches* access_ai_hunches();

		AI::Hunch* give_hunch(Name const& _name, Name const& _reason, Optional<float> const & _expirationTime = NP);

		void learn_from(SimpleVariableStorage & _parameters); // not const as it should be possible to modify this storage (for any reason)

		bool is_suspended() const { return suspended; }
		void suspend() { suspended = true; }
		void resume() { suspended = false; }

		virtual Transform get_me_as_target_placement_os() const; // if someone would like to shoot at me - current target placement or use from setup
		virtual Transform get_target_placement_os_for(IAIObject const * _target) const; // if we want to shoot at someone - aim at hands, head etc
		Transform get_target_placement_for(IAIObject const * _target) const;

		DoorInRoom* get_entered_through_door() const { return enteredThroughDoor.get(); }

		// advance - begin
		static void advance__process_ai_messages(IMMEDIATE_JOB_PARAMS);
		static void advance__advance_perception(IMMEDIATE_JOB_PARAMS);
		static void advance__advance_logic(IMMEDIATE_JOB_PARAMS);
		static void advance__advance_locomotion(IMMEDIATE_JOB_PARAMS);
		// advance - end

		bool does_require_process_ai_messages() const;

#ifdef AN_DEVELOPMENT
		static void set_ai_advanceable(bool _advanceable);
		static bool is_ai_advanceable();
#endif

	public:
		void start_game_script(GameScript::Script* _gs); // will force into non rare advancement
		GameScript::ScriptExecution* get_game_script_execution() const { return gameScriptExecution.get(); }

	protected:
		virtual void advance_non_mind_logic();

	public:
		virtual void on_advancement_suspended() {}
		virtual void on_advancement_resumed() {}

	public:
		void log_hunches(LogInfoContext& _log) const;

	public:	// Module
		override_ void ready_to_activate();
		override_ void reset();
		override_ void setup_with(ModuleData const * _moduleData);
		override_ void on_owner_destroy();

	private: friend class ModulePresence;
		void internal_change_room(DoorInRoom* _enterThrough);

	protected:
		virtual void reset_ai();

	private:
		ModuleAIData const * moduleAIData;
		AI::MindInstance* mind;
		ModuleRareAdvance raLogic;
		bool suspended = false; // this is done if this particular AI has to be suspended (as it is carried by someone etc)
		bool processAIMessages = true;
		bool acceptAIHunches = false;
		Tags ignoreAIHunches; // list of hunches to ignore
		bool controlledByPlayer;
		bool considerPlayer; // this is just an information for some of the code to consider the object a player (if player is immortal, we should allow escorted object to not get killed)
		Name defaultTargetPlacementSocket;

		SafePtr<DoorInRoom> enteredThroughDoor; // to know/remember through which door we have entered

		SafePtr<GameScript::ScriptExecution> gameScriptExecution;

	};
};

