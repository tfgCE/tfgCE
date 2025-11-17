#pragma once

#include "..\framework.h"

#include "..\..\core\fastCast.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\random\random.h"

#include "..\..\framework\game\game.h"
#include "..\..\framework\render\renderScene.h"
#include "..\..\framework\render\renderContext.h"

namespace Framework
{
	class Door;
	class Region;
	class Item;
	struct GameAdvanceContext;
	class GameSceneLayer;

	class Game;
	class GameMode;

	/**
	 *	Game modes should be created through setup struct.
	 *
	 *	Game modes work as act - part of game session having different function in game.
	 *	Each game mode is divided further into game scenes (actualy game scene layers are part of Game class, but game modes create/switch/destroy scenes)
	 *
	 *	Example:
	 *		Three Traits game mode
	 *			it starts up with briefing to location and requirements (intro, pre round)
	 *			continues with actual gameplay in hall (round)
	 *			showing results (post round)
	 *			and displaying any other info/changes (outro)
	 *			then next game mode starts
	 *
	 *		Garage
	 *			allows accessing calendar, messenger etc.
	 *			if everything is done (or time's up?) it ends allowing for next game mode to start
	 *
	 *	Any closed part of functionality of game session should be game mode
	 */
	class GameModeSetup
	: public RefCountObject
	{
		FAST_CAST_DECLARE(GameModeSetup);
		FAST_CAST_END();
	public:
		GameModeSetup(Game* _game): game(_game) {}
		virtual ~GameModeSetup() {}

#ifdef AN_DEVELOPMENT
		virtual tchar const * get_mode_name() const = 0;
#endif

		virtual void fill_missing(Random::Generator & _generator) {} // fill all missing parameters for game mode - parameters that were not provided (will be called always from Game, when mode is started)

		virtual GameMode* create_mode();

		void set_random_generator(Random::Generator & _generator) { randomGenerator = _generator; }
		Random::Generator const & get_random_generator() const { return randomGenerator; }

	protected:
		Game * const game;

		Random::Generator randomGenerator; // will be used by GameMode - this allows for some deterministic behaviour and storing GameModeSetup only
	};

	class GameMode
	: public RefCountObject
	{
		FAST_CAST_DECLARE(GameMode);
		FAST_CAST_END();
	public:
		GameMode(GameModeSetup* _setup);
		virtual ~GameMode();

		void set_game(Game* _game) { game = _game; }
		GameModeSetup const * get_setup() const { return setup.get(); }

		bool should_end() const { return shouldEnd; }

	public:
		virtual void on_start();
		virtual void on_end(bool _abort = false);

	public:
		virtual void pre_advance(GameAdvanceContext const & _advanceContext); // any scene and/or mode creation or destruction should happen here, advances layers
		virtual void pre_game_loop(GameAdvanceContext const & _advanceContext); // anything related to game world happens here

	public:
		virtual void log(LogInfoContext & _log) const;

	protected:
		RefCountObjectPtr<GameModeSetup> setup;

		Game* game;
		bool shouldEnd = false;

		Random::Generator randomGenerator; // set from setup, used for anything random

		RefCountObjectPtr<GameSceneLayer> currentMainLayer;

	protected:
		virtual GameSceneLayer* create_layer_to_replace(GameSceneLayer* _replacingLayer);
		virtual bool does_use_auto_layer_management() const { return false; } // if false, currentMainLayer is not used as well as "create_layer_to_replace"

	};
};
