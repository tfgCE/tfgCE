#include "gameMode.h"

#include "game.h"

#include "..\library\library.h"

#include "..\..\core\types\names.h"

#include "..\game\gameSceneLayer.h"
#include "..\world\subWorld.h"

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(GameModeSetup);

GameMode* GameModeSetup::create_mode()
{
	return new GameMode(this);
}

//

REGISTER_FOR_FAST_CAST(GameMode);

GameMode::GameMode(GameModeSetup* _setup)
: setup(_setup)
, game(nullptr)
, randomGenerator(_setup->get_random_generator())
{
}

GameMode::~GameMode()
{
	currentMainLayer = nullptr;
}

void GameMode::on_start()
{
}

void GameMode::on_end(bool _abort)
{
	game->clear_scene_layer_stack();
}

void GameMode::pre_advance(GameAdvanceContext const & _advanceContext)
{
	if (!does_use_auto_layer_management())
	{
		return;
	}

	bool addNext = false;
	if (!currentMainLayer.is_set())
	{
		addNext = true;
	}
	else if (currentMainLayer->should_end())
	{
		addNext = true;
	}

	if (addNext)
	{
		RefCountObjectPtr<GameSceneLayer> prevMainLayer(currentMainLayer);
		// check if currently ending main layer doesn't want to replace itself with something better
		currentMainLayer = nullptr;
		if (prevMainLayer.is_set())
		{
			currentMainLayer = prevMainLayer->propose_layer_to_replace_yourself();
			prevMainLayer->on_finish_if_required();
		}
		if (!currentMainLayer.is_set())
		{
			// no, it doesn't, ask game mode for replacement
			currentMainLayer = create_layer_to_replace(prevMainLayer.get());
		}
		if (currentMainLayer.is_set())
		{
			// ok, we have one, add it on stack, keep prev main layer as we may want to get some bits from it
			game->push_scene_layer(currentMainLayer.get());
		}
		else
		{
			// there's nothing more we can do,
			shouldEnd = true;
		}
		if (prevMainLayer.get())
		{
			// remove previous one - whatever there was to be reused, it is reused
			prevMainLayer.get()->remove();
		}
	}
}

void GameMode::pre_game_loop(GameAdvanceContext const& _advanceContext)
{
}

GameSceneLayer* GameMode::create_layer_to_replace(GameSceneLayer* _replacingLayer)
{
	todo_important(TXT("forgot to implement_?"));
	return nullptr;
}

void GameMode::log(LogInfoContext & _log) const
{
	_log.log(TXT("<no info>"));
}
