#include "gameSceneLayer.h"

#include "game.h"

#include "..\library\library.h"

#include "..\..\core\types\names.h"

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(GameSceneLayer);

GameSceneLayer::GameSceneLayer()
: game(nullptr)
, placed(false)
, prev(nullptr)
, next(nullptr)
{
}

GameSceneLayer::~GameSceneLayer()
{
	internal_remove();
}

void GameSceneLayer::replace(GameSceneLayer* _sceneLayer)
{
	if (_sceneLayer->prev.is_set())
	{
		_sceneLayer->prev->next = this;
		_sceneLayer->prev = nullptr;
	}
	else if (game->sceneLayerStack.get() == _sceneLayer)
	{
		game->sceneLayerStack = this;
	}
	if (_sceneLayer->next.is_set())
	{
		_sceneLayer->next->prev = this;
		_sceneLayer->next = nullptr;
	}

	if (!game)
	{
		set_game(_sceneLayer->game);
	}

	_sceneLayer->remove();

	placed = true;
	on_placed();
	on_start();
}

void GameSceneLayer::put_before(GameSceneLayer* _sceneLayer)
{
	add_ref();
	bool wasPlaced = placed;
	internal_remove(wasPlaced);
	next = _sceneLayer; // if it's on top of stack, it will stay there
	prev = _sceneLayer->prev;
	if (_sceneLayer->prev.is_set())
	{
		_sceneLayer->prev->next = this;
	}
	_sceneLayer->prev = this;
	if (!game)
	{
		set_game(_sceneLayer->game);
	}
	placed = true;
	on_placed();
	if (!wasPlaced)
	{
		on_start();
	}
	release_ref();
}

void GameSceneLayer::push_onto(RefCountObjectPtr<GameSceneLayer>& _stack)
{
	bool wasPlaced = placed;
	internal_remove(wasPlaced);
	prev = _stack;
	if (prev.is_set())
	{
		prev->next = this;
	}
	_stack = this;
	placed = true;
	on_placed();
	if (!wasPlaced)
	{
		on_start();
	}
}

void GameSceneLayer::remove()
{
	internal_remove();
}

void GameSceneLayer::pop_up_to(GameSceneLayer* _keepThisSceneLayer)
{
	if (next.is_set() && next.get() != _keepThisSceneLayer)
	{
		next->pop_up_to(_keepThisSceneLayer);
	}
	remove();
}

void GameSceneLayer::pop()
{
	if (next.is_set())
	{
		next->pop();
	}
	remove();
}

void GameSceneLayer::internal_remove(bool _willBePlacedAgain)
{
	if (!placed)
	{
		return;
	}
	add_ref(); // keep it in case we would be removing it from next, prev, stack and it would be gone midway
	placed = false; // so it won't be removed twice
	if (!_willBePlacedAgain)
	{
		on_finish_if_required();
		on_end();
	}
	on_removed();
	if (game->sceneLayerStack.get() == this)
	{
		an_assert(! next.is_set());
		game->sceneLayerStack = prev;
	}
	if (prev.is_set())
	{
		prev->next = next;
	}
	if (next.is_set())
	{
		next->prev = prev;
	}
	prev = nullptr;
	next = nullptr;
	release_ref();
}

void GameSceneLayer::on_start()
{
	requiresFinish = true;
}

void GameSceneLayer::on_finish()
{
	requiresFinish = false;
}

void GameSceneLayer::on_end()
{
}

void GameSceneLayer::on_placed()
{
}

void GameSceneLayer::on_removed()
{
}

void GameSceneLayer::on_paused()
{
	if (prev.is_set())
	{
		prev->on_paused();
	}
}

void GameSceneLayer::on_resumed()
{
	if (prev.is_set())
	{
		prev->on_resumed();
	}
}

void GameSceneLayer::pre_advance(GameAdvanceContext const & _advanceContext)
{
	if (prev.is_set())
	{
		prev->pre_advance(_advanceContext);
	}
}

void GameSceneLayer::advance(GameAdvanceContext const & _advanceContext)
{
	if (prev.is_set())
	{
		prev->advance(_advanceContext);
	}
}

void GameSceneLayer::process_controls(GameAdvanceContext const & _advanceContext)
{
	if (prev.is_set())
	{
		prev->process_controls(_advanceContext);
	}
}

void GameSceneLayer::process_all_time_controls(GameAdvanceContext const & _advanceContext)
{
	if (prev.is_set())
	{
		prev->process_all_time_controls(_advanceContext);
	}
}

void GameSceneLayer::process_vr_controls(GameAdvanceContext const & _advanceContext)
{
	if (prev.is_set())
	{
		prev->process_vr_controls(_advanceContext);
	}
}

void GameSceneLayer::prepare_to_sound(GameAdvanceContext const & _advanceContext)
{
	if (prev.is_set())
	{
		prev->prepare_to_sound(_advanceContext);
	}
}

void GameSceneLayer::prepare_to_render(CustomRenderContext * _customRC)
{
	if (prev.is_set())
	{
		prev->prepare_to_render(_customRC);
	}
}

void GameSceneLayer::render_on_render(CustomRenderContext * _customRC)
{
	if (prev.is_set())
	{
		prev->render_on_render(_customRC);
	}
}

void GameSceneLayer::render_after_post_process(CustomRenderContext * _customRC)
{
	if (prev.is_set())
	{
		prev->render_after_post_process(_customRC);
	}
}

void GameSceneLayer::remove_all_to_end()
{
	while (true)
	{
		if (prev.is_set())
		{
			GameSceneLayer* prevPrev = prev.get();
			prev->remove_all_to_end();
			if (prevPrev != prev.get())
			{
				// prev has changed, try again
				continue;
			}
		}

		// prev is the same, we're done now
		break;
	}

	if (should_end())
	{
		remove();
	}
}

