#include "gameMode_Tutorial.h"

#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\module\modulePresence.h"

#include "..\game.h"

#include "..\scenes\gameSceneLayer_BeInWorld.h"

#include "..\..\library\library.h"
#include "..\..\roomGenerators\roomGenerationInfo.h"
#include "..\..\tutorials\tutorial.h"
#include "..\..\tutorials\tutorialSystem.h"
#include "..\..\utils\overridePilgrimType.h"

#include "..\..\..\core\performance\performanceUtils.h"

//

using namespace TeaForGodEmperor;
using namespace GameModes;

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

REGISTER_FOR_FAST_CAST(TutorialSetup);

TutorialSetup::TutorialSetup(Game* _game, TeaForGodEmperor::Tutorial const * _tutorial)
: base(_game)
, tutorial(_tutorial)
{
}

Framework::GameMode* TutorialSetup::create_mode()
{
	return new Tutorial(this);
}

//

REGISTER_FOR_FAST_CAST(GameModes::Tutorial);

GameModes::Tutorial::Tutorial(Framework::GameModeSetup* _setup)
: base(_setup)
, tutorialSetup(fast_cast<TutorialSetup>(_setup))
{
	tutorial = tutorialSetup->get_tutorial();
	an_assert(tutorial);
}

GameModes::Tutorial::~Tutorial()
{
}

void GameModes::Tutorial::on_start()
{
	base::on_start();

	auto* createRegion = tutorial->get_region_type();
	auto* createRoom = tutorial->get_room_type();

	Random::Generator rg;

	if (createRegion)
	{
		RefCountObjectPtr<Framework::GameMode> keepThis(this);

		game->request_world(rg); // will create sub world for us
		game->add_async_world_job(TXT("create tutorial sub world"), [this, rg, keepThis, createRoom, createRegion]() // by value as rg will be inaccessible soon
		{
			MEASURE_PERFORMANCE(createTutorialSubWorld);

			Framework::SubWorld* subWorld;
			Random::Generator userg = rg;
			game->perform_sync_world_job(TXT("create sub world"), [this, &subWorld]()
			{
				subWorld = game->get_sub_world();
				subWorld->access_collision_info_provider().set_gravity_dir_based(-Vector3::zAxis, 9.81f);
				subWorld->access_collision_info_provider().set_kill_gravity_distance(1000.0f);
				an_assert(subWorld);
			});

			{
				Framework::ActivationGroupPtr activationGroup = game->push_new_activation_group();

				Framework::Region* region = nullptr;
				if (createRoom)
				{
					Game::get()->perform_sync_world_job(TXT("create region"), [&region, subWorld, createRegion]()
					{
						region = new Framework::Region(subWorld, nullptr, createRegion, Random::Generator().be_zero_seed());
						region->set_world_address_id(0);
					});

					region->update_default_environment();

					// generate starting room (ending room will be generated in a separate call)
					// 1. create room
					Framework::Room* room = nullptr;
					Game::get()->perform_sync_world_job(TXT("create room"), [&room, createRoom, subWorld, region, userg]()
					{
						room = new Framework::Room(subWorld, region, createRoom, userg);
					});
					// 2. generate room? or leave so it can be generated on demand or by other means
					{
						room->generate();
					}

					// this is a hack for this testing
					room->set_door_vr_placement_if_not_set();

					room->ready_for_game();
				}
				else
				{
					if (Framework::RegionGenerator::generate_and_add_to_sub_world(subWorld, nullptr, createRegion, userg, &userg,
						[createRegion, &userg](PieceCombiner::Generator<Framework::RegionGenerator> & _generator, Framework::Region* _forRegion)
						{
							_generator.access_generation_context().generationTags = RoomGenerationInfo::get().get_generation_tags();

							// may require outer connectors
							Framework::RegionGenerator::setup_generator(_generator, _forRegion, createRegion->get_library(), userg);
						},
						[](Framework::Region* _region)
						{
							GameSettings::get().setup_variables(_region->access_variables());
						},
						&region, false))
					{}
				}

				Framework::PointOfInterestInstance* playerSpawnPOI;
				subWorld->find_any_point_of_interest(tutorial->get_player_spawn_poi(), playerSpawnPOI);

				if (!playerSpawnPOI)
				{
					error(TXT("no playerSpawnPOI \"%S\" found!"), tutorial->get_player_spawn_poi().to_char());
				}

				// actor type!
				auto* pilgrimType = Library::get_current()->get_actor_types().find(Framework::LibraryName(String(hardcoded TXT("pilgrims:actor"))));
				override_pilgrim_type(REF_ pilgrimType, nullptr);
				an_assert(pilgrimType);
				pilgrimType->load_on_demand_if_required();
				Framework::Actor* playerActor = new Framework::Actor(pilgrimType, String(hardcoded TXT("player")));// Framework::Library::get_current()->get_actor_types().find(Framework::LibraryName(String(TXT("base")), String(TXT("player")))), String(TXT("player")));
				game->perform_sync_world_job(TXT("init pilgrim"), [playerActor, subWorld]()
				{
					playerActor->init(subWorld);
				});
				playerActor->randomise_individual_random_generator();
				fast_cast<Game>(game)->access_player().bind_actor(playerActor, Framework::Player::BindActorParams().lazy_bind());
				playerActor->initialise_modules();
				fast_cast<Game>(game)->access_player().bind_actor(playerActor);

				todo_important(TXT("uncomment and fix"));
				//game->perform_sync_world_job(TXT("add pilgrim and place"), [playerSpawnPOI, playerActor]()
				//{
				//	playerActor->get_presence()->place_in_room(playerSpawnPOI->get_room(), playerSpawnPOI->calculate_placement());
				//});

				game->request_ready_and_activate_all_inactive(activationGroup.get());

				game->pop_activation_group(activationGroup.get());

				game->request_initial_world_advance_required();

				// mark that we will be using world
				game->post_use_world(true);

				// switch into performance mode
				game->post_set_performance_mode(true);
			}
		});
	}
	else
	{
		todo_important(TXT("what now?"));
	}
}

void GameModes::Tutorial::on_end(bool _abort)
{
#ifdef DEBUG_DESTROY_WORLD
	output(TXT("GameModes::Tutorial::on_end"));
#endif
	game->use_world(false);
	game->set_performance_mode(false);
	game->request_destroy_world();
	
	base::on_end(_abort);
}

Framework::GameSceneLayer* GameModes::Tutorial::create_layer_to_replace(Framework::GameSceneLayer* _replacingLayer)
{
	if (_replacingLayer == nullptr)
	{
		return new GameScenes::BeInWorld();
	}
	else
	{
		// just end
		return nullptr;
	}
}

void GameModes::Tutorial::pre_advance(Framework::GameAdvanceContext const & _advanceContext)
{
	base::pre_advance(_advanceContext);

	if (auto* ts = TutorialSystem::get())
	{
		if (!ts->is_active())
		{
			if (!fast_cast<Game>(game)->is_post_game_summary_requested())
			{
				fast_cast<Game>(game)->request_post_game_summary(PostGameSummary::ReachedEnd);
			}
		}
	}
}

void GameModes::Tutorial::log(LogInfoContext & _log) const
{
	_log.log(TXT("Tutorial"));
}
