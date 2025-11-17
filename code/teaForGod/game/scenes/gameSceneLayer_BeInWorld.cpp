#include "gameSceneLayer_BeInWorld.h"

#include "..\..\..\core\debug\debug.h"
#include "..\..\..\core\memory\memory.h"

#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\ai\aiMessage.h"
#include "..\..\..\framework\module\modulePresence.h"

#include "..\sceneLayers\gameSceneLayer_DontUpdatePrev.h"
#include "..\sceneLayers\gameSceneLayer_World.h"

#include "..\game.h"

#include "..\..\..\core\vr\iVR.h"

using namespace TeaForGodEmperor;
using namespace GameScenes;

REGISTER_FOR_FAST_CAST(BeInWorld);

BeInWorld::BeInWorld()
{
}

BeInWorld::~BeInWorld()
{
	an_assert(! startingLayer.is_set());
	an_assert(! world.is_set());
}

void BeInWorld::on_start()
{
	base::on_start();

	Player* player = fast_cast<Player>(&fast_cast<Game>(game)->access_player());
	Player* playerTakenControl = fast_cast<Player>(&fast_cast<Game>(game)->access_player_taken_control());
	if (player->get_actor())
	{
		player->set_control_mode(ControlMode::Actor);
	}
	else
	{
		player->set_control_mode(ControlMode::None);
	}

	world = find_layer<GameSceneLayers::World>();
	if (world.is_set())
	{
		// we need to do that, otherwise pausing/resuming will mess up stuff
		world->remove();
		world->use(player, playerTakenControl);
	}
	else
	{
		world = new GameSceneLayers::World(player, playerTakenControl);
	}

	startingLayer = new GameSceneLayers::DontUpdatePrev();
	startingLayer->put_before(this);

	world->put_before(this);

	// finally, remember about fading in
	game->fade_in();
}

void BeInWorld::on_end()
{
	if (startingLayer.is_set())
	{
		startingLayer->pop_up_to(this);
	}
	startingLayer = nullptr;
	world = nullptr;

	base::on_end();
}

void BeInWorld::advance(Framework::GameAdvanceContext const & _advanceContext)
{
	base::advance(_advanceContext);
}

Framework::GameSceneLayer* BeInWorld::propose_layer_to_replace_yourself()
{
	return base::propose_layer_to_replace_yourself();
}
