#pragma once

#include "..\framework.h"

#include "..\..\core\fastCast.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\types\optional.h"

#include <functional>

namespace Framework
{
	class Game;
	class GameSceneLayer;
	struct CustomRenderContext;
	struct GameAdvanceContext;
	
	/**
	 *	Game scene layers come together to create game scene.
	 *
	 *	There are game scenes that are actually game scene layers (that create further layers).
	 *	In most cases there should be just one main game scene layer with its sub layers just below it.
	 *	In some cases (like menu) there might be additional game scene layer on top (that stops time, blocks input etc).
	 *
	 *	tl;dr
	 *		scene layer	- single layer providing some functionality, eg. display
	 *		scene - creates layers, provides some greater functionality using different layers, eg. wait using display and dont update prev layers
	 */
	class GameSceneLayer
	: public virtual RefCountObject
	{
		FAST_CAST_DECLARE(GameSceneLayer);
		FAST_CAST_END();
	public:
		GameSceneLayer();
		virtual ~GameSceneLayer();

		void set_game(Game* _game) { game = _game; }
		Game* get_game() const { return game; }
		bool is_placed() const { return placed; }
		bool has_above_layer() const { return next.get(); }

		void replace(GameSceneLayer* _sceneLayer);
		void put_before(GameSceneLayer* _sceneLayer);
		void push_onto(RefCountObjectPtr<GameSceneLayer>& _stack);
		void pop_up_to(GameSceneLayer* _keepThisSceneLayer); // will delete this and all on top of it
		void pop(); // will delete this and all on top of it
		void remove(); // will delete this

		template <typename Class>
		inline Class* find_layer();

		template <typename Class>
		inline void for_every_layer(std::function< bool(Class* _layer) > _do_for_layer);

		template <typename Class>
		inline int count_layers();

		inline GameSceneLayer* get_top();

	public:
		// start and end is called when layer is placed in stack after being nowhere and end is called when layer will be removed and not placed again
		virtual void on_start();
		virtual void on_finish(); // called before on_end, as when replacing layer we first add new one and then remove old one
		void on_finish_if_required() { if (requiresFinish) { on_finish(); } }
		virtual void on_end(); // when replacing this is called AFTER new layer is in place
		virtual bool should_end() const { return false; }

		void remove_all_to_end(); // checks if any layers should end and if so, removes it, in such a case, goes again through all

#ifdef AN_DEVELOPMENT
		virtual bool allows_to_end_on_demand() const { return true; }
#endif

		virtual void on_placed();
		virtual void on_removed();

		virtual void on_paused(); // when covered by don't prev
		virtual void on_resumed(); // when uncovered by don't prev

		virtual GameSceneLayer* propose_layer_to_replace_yourself() { return nullptr; } //

	public:
		virtual void pre_advance(GameAdvanceContext const & _advanceContext);
		virtual void advance(GameAdvanceContext const & _advanceContext);

		virtual Optional<bool> is_input_grabbed() const { return NP; }
		virtual void process_controls(GameAdvanceContext const & _advanceContext); // can't be paused, the input has to be grabbed
		virtual void process_all_time_controls(GameAdvanceContext const & _advanceContext); // ignores if paused
		virtual void process_vr_controls(GameAdvanceContext const & _advanceContext); // as separate because we may want to allow controlling vr always

		virtual void prepare_to_sound(GameAdvanceContext const & _advanceContext);

		virtual void prepare_to_render(CustomRenderContext * _customRC);
		virtual void render_on_render(CustomRenderContext * _customRC);

		virtual void render_after_post_process(CustomRenderContext * _customRC);

	protected:
		Game* game;

	private:
		bool placed;
		bool requiresFinish = false;
		RefCountObjectPtr<GameSceneLayer> prev; // below
		RefCountObjectPtr<GameSceneLayer> next; // above

		void internal_remove(bool _willBePlacedAgain = false);

	};

	template <typename Class>
	Class* GameSceneLayer::find_layer()
	{
		GameSceneLayer* curr = get_top();
		while (curr)
		{
			if (Class* found = fast_cast<Class>(curr))
			{
				return found;
			}
			curr = curr->prev.get();
		}

		return nullptr;
	}

	template <typename Class>
	void GameSceneLayer::for_every_layer(std::function< bool(Class* _layer) > _do_for_layer)
	{
		GameSceneLayer* curr = get_top();
		while (curr)
		{
			if (Class* found = fast_cast<Class>(curr))
			{
				if (_do_for_layer(found))
				{
					break;
				}
			}
			curr = curr->prev.get();
		}
	}

	template <typename Class>
	int GameSceneLayer::count_layers()
	{
		GameSceneLayer* curr = get_top();
		int count = 0;
		while (curr)
		{
			if (Class* found = fast_cast<Class>(curr))
			{
				++ count;
			}
			curr = curr->prev.get();
		}

		return count;
	}

	GameSceneLayer* GameSceneLayer::get_top()
	{
		GameSceneLayer* curr = this;
		while (curr->next.is_set())
		{
			curr = curr->next.get();
		}
		return curr;
	}

};
