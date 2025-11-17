#include "gameSceneLayer_Wait.h"

#include "..\game.h"
#include "..\gameConfig.h"
#include "..\sceneLayers\gameSceneLayer_DontUpdatePrev.h"
#include "..\sceneLayers\gameSceneLayer_Utils.h"

#include "..\..\library\library.h"

using namespace TeaForGodEmperor;
using namespace GameScenes;

REGISTER_FOR_FAST_CAST(Wait);

Wait::Wait(FinishedWaitingFunc _finishedWaitingFunc)
: finishedWaitingFunc(_finishedWaitingFunc)
{
}

Wait::~Wait()
{
	an_assert(!startingLayer.is_set());
}

void Wait::on_start()
{
	base::on_start();

	// setup layers
	startingLayer = (new GameSceneLayers::DontUpdatePrev())->provide_background();
	startingLayer->put_before(this);

	todo_note(TXT("add some vector based background here!"));

	DEFINE_STATIC_NAME(ui);
	input.use(NAME(ui));

	// fade out so we will always fade in
	game->fade_out_immediately();

	// finally, remember about fading in
	game->fade_in(fadeInTime);
}

void Wait::on_end()
{
	if (startingLayer.is_set())
	{
		startingLayer->pop_up_to(this);
	}
	startingLayer = nullptr;

	base::on_end();

	// fade in whatever was below/next
	game->fade_out_immediately();
	game->fade_in(0.1f);
}

void Wait::on_resumed()
{
	base::on_resumed();
}

void Wait::process_controls(Framework::GameAdvanceContext const & _advanceContext)
{
	base::process_controls(_advanceContext);
}

void Wait::advance(Framework::GameAdvanceContext const & _advanceContext)
{
	base::advance(_advanceContext);
	if (timeToEnd == 0.0f)
	{
		if (finishedWaitingFunc())
		{
			finish();
		}
	}

	if (timeToEnd > 0.0f)
	{
		timeToEnd -= _advanceContext.get_world_delta_time();
		if (timeToEnd <= 0.0f)
		{
			shouldEnd = true;
		}
	}

	activeFor.advance_ms(_advanceContext.get_main_delta_time());
}

void Wait::finish()
{
	if (timeToEnd == 0.0f)
	{
		timeToEnd = fadeOutTime;
		game->fade_out(fadeOutTime);
	}
}
