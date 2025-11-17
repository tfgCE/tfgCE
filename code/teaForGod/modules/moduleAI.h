#pragma once

#include "..\..\framework\appearance\socketID.h"
#include "..\..\framework\module\moduleAI.h"

namespace Concurrency
{
	class JobExecutionContext;
};

namespace TeaForGodEmperor
{
	class ModuleAIData;

	class ModuleAI
	: public Framework::ModuleAI
	{
		FAST_CAST_DECLARE(ModuleAI);
		FAST_CAST_BASE(Framework::ModuleAI);
		FAST_CAST_END();

		typedef Framework::ModuleAI base;
	public:
		static Framework::RegisteredModule<Framework::ModuleAI> & register_itself();

		ModuleAI(Framework::IModulesOwner* _owner);
		virtual ~ModuleAI();

		bool is_registered_hostile_at_game_director() const { return registeredHostileAtGameDirector; }
		bool is_registered_immobile_hostile_at_game_director() const { return registeredImmobileHostileAtGameDirector; }

		Transform calculate_investigate_placement_os() const;

	public: // Module
		override_ void activate();
		override_ void setup_with(Framework::ModuleData const* _moduleData);
		override_ void on_owner_destroy();

	public: // Framework::ModuleAI
		override_ void on_advancement_suspended();
		override_ void on_advancement_resumed();

	protected: // Framework::ModuleAI
		override_ void reset_ai();

	public:
		void mark_visible_for_game_director(bool _visible); // for game director
		void mark_non_hostile_for_game_director(bool _nonHostile);

	private:
		bool registerHostileAtGameDirector = false;
		RUNTIME_ bool registeredHostileAtGameDirector = false;

		RUNTIME_ bool registeredAsSwitchedSidesToPlayerAtGameDirector = false;
		RUNTIME_ bool switchedSidesToPlayer = false;
		RUNTIME_ bool suspendedSwitchSides = false;

		void register_hostile_to_game_director();
		void unregister_hostile_from_game_director();

		void update_non_hostile_at_game_director(Optional<bool> const& _switchedSidesToPlayer, Optional<bool> const& _suspended);

	private:
		bool registerImmobileHostileAtGameDirector = false;
		RUNTIME_ bool registeredImmobileHostileAtGameDirector = false;

		RUNTIME_ bool registeredAsHiddenImmobileHostileAtGameDirector = false;
		RUNTIME_ bool hiddenImmobileHostile = false;
		RUNTIME_ bool suspendedImmobileHostile = false;

		Framework::SocketID investigateSocket;

		void register_immobile_hostile_to_game_director();
		void unregister_immobile_hostile_from_game_director();
		
		void update_immobile_hostile_hidden_at_game_director(Optional<bool> const & _hidden, Optional<bool> const & _suspended);
	};
};

