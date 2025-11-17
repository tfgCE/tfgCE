#include "gameMode_Pilgrimage.h"

#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\object\item.h"
#include "..\..\..\framework\object\scenery.h"
#include "..\..\..\framework\module\modulePresence.h"

#include "..\game.h"
#include "..\gameSettings.h"
#include "..\scenes\gameSceneLayer_BeInWorld.h"
#include "..\scenes\gameSceneLayer_Wait.h"

#include "..\..\library\library.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\pilgrimage\pilgrimageInstance.h"
#include "..\..\pilgrimage\pilgrimageInstanceLinear.h"
#include "..\..\roomGenerators\roomGenerationInfo.h"
#include "..\..\utils\overridePilgrimType.h"

#include "..\..\..\framework\debug\testConfig.h"
#include "..\..\..\framework\world\world.h"
#include "..\..\..\framework\world\subWorld.h"

#ifdef AN_DEVELOPMENT_OR_PROFILER
#include "..\..\..\core\pieceCombiner\pieceCombinerDebugVisualiser.h"
#endif

using namespace TeaForGodEmperor;
using namespace GameModes;

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

//#define TEST_SEED 1492011594
//#define TEST_SEED 1492011596
#define TEST_SEED 1494177843

//

// test tags/params
DEFINE_STATIC_NAME(testSeed);
DEFINE_STATIC_NAME(partIdx);
DEFINE_STATIC_NAME(testRoom);
DEFINE_STATIC_NAME(testRegion);
DEFINE_STATIC_NAME(testGenerateRoomsWithCode);

// poi
DEFINE_STATIC_NAME_STR(spawnPoint, TXT("spawn point"));

//

REGISTER_FOR_FAST_CAST(PilgrimageSetup);

PilgrimageSetup::PilgrimageSetup(Game* _game, TeaForGodEmperor::Pilgrimage* _pilgrimage, GameState* _gameState)
: base(_game)
, pilgrimage(_pilgrimage)
, gameState(_gameState)
{
}

Framework::GameMode* PilgrimageSetup::create_mode()
{
	return new Pilgrimage(this);
}

//

REGISTER_FOR_FAST_CAST(GameModes::Pilgrimage);

GameModes::Pilgrimage::Pilgrimage(Framework::GameModeSetup* _setup)
: base(_setup)
, pilgrimageSetup(fast_cast<PilgrimageSetup>(_setup))
{
}

GameModes::Pilgrimage::~Pilgrimage()
{
}

void get_test_seed(Random::Generator & rg)
{
	if (auto * testSeedParam = Framework::TestConfig::get_params().get_existing<VectorInt2>(NAME(testSeed)))
	{
		rg.set_seed(testSeedParam->x, testSeedParam->y);
	}
	if (auto * testSeedParam = Framework::TestConfig::get_params().get_existing<int>(NAME(testSeed)))
	{
		rg.set_seed(*testSeedParam, 0);
	}
}

void get_part_idx(int & partIdx)
{
	if (auto * partIdxParam = Framework::TestConfig::get_params().get_existing<int>(NAME(partIdx)))
	{
		partIdx = *partIdxParam;
	}
}

void GameModes::Pilgrimage::on_start()
{
	base::on_start();

	if (auto * testRoomParam = Framework::TestConfig::get_params().get_existing<Framework::LibraryName>(NAME(testRoom)))
	{
		testRoom = *testRoomParam;
	}

	Random::Generator rg = Random::Generator();
	int partIdx = 0;
#ifdef AN_DEVELOPMENT_OR_PROFILER
	get_test_seed(rg);
	get_part_idx(partIdx);
#endif
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("using random generator %S"), rg.get_seed_string().to_char());
	output(TXT("part idx %i"), partIdx);
#endif

	// test room is for testing purposes only, it has some hardcoded stuff we don't care about
	if (testRoom.is_valid())
	{
#ifdef AN_DEVELOPMENT
		ignore_debug_on_error_warn();
#endif

		RefCountObjectPtr<Framework::GameMode> keepThis(this);

		game->request_world(rg); // will create sub world for us
		game->add_async_world_job(TXT("create test sub world"), [this, rg, keepThis]() // by value as rg will be inaccessible soon
		{
			MEASURE_PERFORMANCE(createTestSubWorld);

			Framework::SubWorld* subWorld;
			Random::Generator userg = rg;
			game->perform_sync_world_job(TXT("create sub world"), [this, &subWorld]()
			{
				subWorld = game->get_sub_world();
				subWorld->access_collision_info_provider().set_gravity_dir_based(-Vector3::zAxis, 9.81f);
				subWorld->access_collision_info_provider().set_kill_gravity_distance(1000.0f);
#ifdef AN_DEVELOPMENT
				subWorld->be_test_sub_world();
#endif
				an_assert(subWorld);
			});

			if (Framework::RoomType* roomType = ::Framework::Library::get_current()->get_room_types().find(testRoom))
			{
				Framework::ActivationGroupPtr activationGroup = game->push_new_activation_group();

				Framework::LibraryName testRegion = Framework::LibraryName(String(hardcoded TXT("test:test region")));
				if (auto* testRegionParam = Framework::TestConfig::get_params().get_existing<Framework::LibraryName>(NAME(testRegion)))
				{
					testRegion = *testRegionParam;
				}
				Framework::Region* region = nullptr;
				auto* regionType = Library::get_current()->get_region_types().find(testRegion); // we need to have some region to place our room inside
				Game::get()->perform_sync_world_job(TXT("create region"), [&region, subWorld, regionType]()
				{
					region = new Framework::Region(subWorld, nullptr, regionType, Random::Generator().be_zero_seed());
					region->set_world_address_id(0);
				});

				region->update_default_environment();

				// generate starting room (ending room will be generated in a separate call)
				// 1. create room
				Framework::Room* room = nullptr;
				Game::get()->perform_sync_world_job(TXT("create room"), [&room, roomType, subWorld, region, userg]()
				{
					room = new Framework::Room(subWorld, region, roomType, userg);
					room->access_tags().set_tag(Name(String(TXT("testRoom1"))));
				});
				// 2. generate room? or leave so it can be generated on demand or by other means
				{
					room->generate();
				}

				bool generateRoomsWithCode = false;
				if (auto* testGenerateRoomsWithCodeParam = Framework::TestConfig::get_params().get_existing<bool>(NAME(testGenerateRoomsWithCode)))
				{
					generateRoomsWithCode = *testGenerateRoomsWithCodeParam;
				}

				if (generateRoomsWithCode)
				{
					Framework::Room* secondRoom = nullptr;
					Game::get()->perform_sync_world_job(TXT("create second room"), [&secondRoom, subWorld, region, userg]()
					{
						secondRoom = new Framework::Room(subWorld, region, nullptr, userg);
						secondRoom->access_tags().set_tag(Name(String(TXT("testRoom2"))));
					});
					// 2. generate room? or leave so it can be generated on demand or by other means
					{
						secondRoom->generate();
					}

					Framework::Door* door;
					Framework::DoorInRoom* dirA;
					Framework::DoorInRoom* dirB;
					auto* doorType = Library::get_current()->get_door_types().find(Framework::LibraryName(String(hardcoded TXT("pilgrimage, shared:door")))); // well, any door would be fine
					Game::get()->perform_sync_world_job(TXT("create second room"), [subWorld, doorType, &door, &dirA, &dirB, room, secondRoom]()
					{
						Transform vrPlacement = look_at_matrix(Vector3(3.0f, 0.0f, 0.0f), Vector3(6.0f, 0.0f, 0.0f), Vector3::zAxis).to_transform();
						door = new Framework::Door(subWorld, doorType);
						dirA = new Framework::DoorInRoom(room);
						dirA->set_vr_space_placement(vrPlacement);
						dirA->set_placement(dirA->get_vr_space_placement());
						dirB = new Framework::DoorInRoom(secondRoom);
						dirB->set_vr_space_placement(Transform(Vector3::zero, Quat::identity));
						//dirB->set_vr_space_placement(vrPlacement.to_world(Transform(Vector3::zero, Rotator3(0.0f, 180.0f, 0.0f).to_quat())));

						dirB->set_placement(dirB->get_vr_space_placement());
						door->link(dirA);
						door->link(dirB);
					});

					dirA->init_meshes();
					dirB->init_meshes();
				}

				// this is a hack for this testing
				room->set_door_vr_placement_if_not_set();

				room->ready_for_game();

				// actor type!
				auto* pilgrimType = Library::get_current()->get_actor_types().find(Framework::LibraryName(String(hardcoded TXT("pilgrims:actor human"))));
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
				Transform spawnSpot = Transform::identity;
				Framework::PointOfInterestInstance* fpoi;
				if (room->find_any_point_of_interest(NAME(spawnPoint), fpoi))
				{
					spawnSpot = fpoi->calculate_placement();
				}
				game->perform_sync_world_job(TXT("add pilgrim and place"), [room, playerActor, spawnSpot]()
				{
					playerActor->get_presence()->place_in_room(room, spawnSpot);
				});

				game->request_ready_and_activate_all_inactive(activationGroup.get());

				game->pop_activation_group(activationGroup.get());

				game->request_initial_world_advance_required();

				// mark that we will be using world
				game->post_use_world(true);

				// switch into performance mode
				game->post_set_performance_mode(true);

				testRoomCreated = true;
			}
			else
			{
				error(TXT("no room type for next room!"));
			}
		});
	}
	else
	{
		pilgrimageInstance = pilgrimageSetup->get_pilgrimage()->create_instance(fast_cast<Game>(game));
		pilgrimageInstance->set_seed(rg);
		if (auto* pil = fast_cast<PilgrimageInstanceLinear>(pilgrimageInstance.get()))
		{
			pil->linear__set_part_idx(partIdx);
		}

		PilgrimageInstance::reset_deferred_job_counters(); // in case we wanted to cancel
		pilgrimageInstance->create_and_start(pilgrimageSetup->get_game_state()); // at proper time will step out of waiting and will start actual game
	}
}

void GameModes::Pilgrimage::on_end(bool _abort)
{
#ifdef DEBUG_DESTROY_WORLD
	output(TXT("GameModes::Pilgrimage::on_end"));
#endif
	if (pilgrimageInstance.get())
	{
		if (!pilgrimageResult.is_set())
		{
			warn_dev_ignore(TXT("pilgrimage result not provided, we assume abnormal end, like cancelation"));
		}
		pilgrimageInstance->on_end(pilgrimageResult.get(PilgrimageResult::Unknown));
	}
	game->use_world(false);
	game->set_performance_mode(false);
	game->request_destroy_world();
	
	base::on_end(_abort);
}

Framework::GameSceneLayer* GameModes::Pilgrimage::create_layer_to_replace(Framework::GameSceneLayer* _replacingLayer)
{
	todo_note(TXT("waiting here does not work as expected - add waiting room? and remove loader?"));
	if (_replacingLayer == nullptr)
	{
		/*
		return new GameScenes::Wait([this](){return testRoomCreated || (pilgrimageInstance.is_set() && pilgrimageInstance->has_started()); });
	}
	else
	if (fast_cast<GameScenes::Wait>(_replacingLayer) != nullptr)
	{
	*/
		return new GameScenes::BeInWorld();
	}
	else
	{
		// just end
		return nullptr;
	}
}

void GameModes::Pilgrimage::pre_advance(Framework::GameAdvanceContext const& _advanceContext)
{
	base::pre_advance(_advanceContext);
}

void GameModes::Pilgrimage::pre_game_loop(Framework::GameAdvanceContext const& _advanceContext)
{
	Framework::Game::WorldManagerImmediateSyncJobsScopedLock lockImmediateSyncJobs(game);

	base::pre_game_loop(_advanceContext);	

	if (auto* pi = PilgrimageInstance::get())
	{
		// update current pilgrimage
		if (pilgrimageInstance != pi)
		{
			pilgrimageInstance = pi;
		}
	}

	if (pilgrimageInstance.get())
	{
		MEASURE_PERFORMANCE_COLOURED(pilgrimageInstanceAdvance, Colour::c64LightGreen);
		todo_note(TXT("why is this called in pre advance, pre pre-advance?!"));
		pilgrimageInstance->advance(_advanceContext.get_world_delta_time());
	}

	if (!testRoom.is_valid() && pilgrimageInstance.get())
	{
		MEASURE_PERFORMANCE_COLOURED(pilgrimageInstancePreAdvance, Colour::c64LightBlue);
		pilgrimageInstance->pre_advance();
	}
}

void GameModes::Pilgrimage::log(LogInfoContext & _log) const
{
	_log.log(TXT("Pilgrimage"));
	if (pilgrimageInstance.get())
	{
		LOG_INDENT(_log);
		pilgrimageInstance->log(_log);
	}
}

void GameModes::Pilgrimage::set_pilgrimage_result(PilgrimageResult::Type _pilgrimageResult)
{
	pilgrimageResult = _pilgrimageResult;
}

#define PILinear fast_cast<PilgrimageInstanceLinear>(pilgrimageInstance.get())

bool GameModes::Pilgrimage::is_linear() const { return PILinear && PILinear->is_linear(); }
bool GameModes::Pilgrimage::linear__is_station(Framework::Room* _room) const { return PILinear ? PILinear->linear__is_station(_room) : false; }
Framework::Region* GameModes::Pilgrimage::linear__get_current_region() const { return PILinear ? PILinear->linear__get_current_region() : nullptr; }
int GameModes::Pilgrimage::linear__get_current_region_idx() const { return PILinear ? PILinear->linear__get_current_region_idx() : 0; }
bool GameModes::Pilgrimage::linear__is_current_station(Framework::Room* _room) const { return PILinear ? PILinear->linear__is_current_station(_room) : false; }
bool GameModes::Pilgrimage::linear__is_next_station(Framework::Room* _room) const { return PILinear ? PILinear->linear__is_next_station(_room) : false; }
Framework::Room* GameModes::Pilgrimage::linear__get_current_station() const { return PILinear ? PILinear->linear__get_current_station() : nullptr; }
Framework::Room* GameModes::Pilgrimage::linear__get_next_station() const { return PILinear ? PILinear->linear__get_next_station() : nullptr; }
bool GameModes::Pilgrimage::linear__has_reached_end() const { return PILinear ? PILinear->linear__has_reached_end() : false; }
