#include "pilgrimageInstanceLinear.h"

#include "pilgrimage.h"
#include "pilgrimageEnvironment.h"
#include "pilgrimagePart.h"

#include "..\game\damage.h"
#include "..\game\game.h"
#include "..\game\gameSettings.h"
#include "..\library\library.h"
#include "..\modules\gameplay\modulePilgrim.h"
#include "..\modules\custom\health\mc_health.h"
#include "..\roomGenerators\roomGenerationInfo.h"
#include "..\utils\overridePilgrimType.h"

#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\framework\object\actor.h"
#include "..\..\framework\world\doorInRoom.h"
#include "..\..\framework\world\environment.h"

#include "..\..\core\pieceCombiner\pieceCombinerImplementation.h"
#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\system\faultHandler.h"

// demo
#include "..\..\framework\module\moduleMovementPath.h"

#ifdef AN_DEVELOPMENT_OR_PROFILER
#include "..\..\core\pieceCombiner\pieceCombinerDebugVisualiser.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#ifdef AN_OUTPUT
#define AN_OUTPUT_DEV_INFO

#ifdef AN_ALLOW_EXTENSIVE_LOGS
#define AN_OUTPUT_PILGRIMAGE_GENERATION
#endif
#endif

#ifdef AN_OUTPUT_WORLD_GENERATION
//#define OUTPUT_GENERATION_DEBUG_VISUALISER
#endif

//

// pois
DEFINE_STATIC_NAME(door);
DEFINE_STATIC_NAME(inDoor);
DEFINE_STATIC_NAME(outDoor);
DEFINE_STATIC_NAME_STR(vrAnchor, TXT("vr anchor"));

// door in room tags
DEFINE_STATIC_NAME(autoStationDoor);
DEFINE_STATIC_NAME(stationExit);
DEFINE_STATIC_NAME(stationEntrance);

// game tags
DEFINE_STATIC_NAME(linear);
DEFINE_STATIC_NAME(openWorld);

// demo #PGA
DEFINE_STATIC_NAME(demoFlat);

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(PilgrimageInstanceLinear);

PilgrimageInstanceLinear::PilgrimageInstanceLinear(Game* _game, Pilgrimage* _pilgrimage)
: base(_game, _pilgrimage)
{
	linear.active = pilgrimage->is_linear();
}

PilgrimageInstanceLinear::~PilgrimageInstanceLinear()
{
}

void PilgrimageInstanceLinear::post_setup_rgi()
{
	base::post_setup_rgi();
	output_dev_info(0);
}

void PilgrimageInstanceLinear::output_dev_info(int _partIdx) const
{
#ifdef AN_OUTPUT_DEV_INFO
	{	// output in an easier to load way
		lock_output();
		::System::FaultHandler::store_custom_info(String(TXT("pilgrimage")));
		s_a_o(TXT("dev info (config test string)"));
		if (GameDefinition::get_chosen())
		{
			s_a_o(TXT("    <libraryName name=\"testGameDefinition\">%S</libraryName>"), GameDefinition::get_chosen()->get_name().to_string().to_char());
		}
		if (auto* gsd = GameDefinition::get_chosen_sub(0))
		{
			s_a_o(TXT("    <libraryName name=\"testGameSubDefinition\">%S</libraryName>"), gsd->get_name().to_string().to_char());
		}
		s_a_o(TXT("    <libraryName name=\"testPilgrimage\">%S</libraryName>"), get_pilgrimage()->get_name().to_string().to_char());
		s_a_o(TXT("    <int name=\"partIdx\">%i</int>"), _partIdx);
		if (randomSeed.get_seed_lo_number() == 0)
		{
			s_a_o(TXT("    <int name=\"testSeed\">%u</int>"), randomSeed.get_seed_hi_number());
		}
		else
		{
			s_a_o(TXT("    <vectorInt2 name=\"testSeed\" x=\"%u\" y=\"%u\"/>"), randomSeed.get_seed_hi_number(), randomSeed.get_seed_lo_number());
		}
		s_a_o(TXT("    <bool name=\"testImmobileVR\">%S</bool>"), Option::to_char(MainConfig::global().get_immobile_vr()));
		s_a_o(TXT("    <bool name=\"testImmobileVRAuto\">%S</bool>"), MainConfig::global().get_immobile_vr_auto() ? TXT("true") : TXT("false"));
		s_a_o(TXT("    <vector2 name=\"testPlayArea\" x=\"%.2f\" y=\"%.2f\"/>"), RoomGenerationInfo::get().get_play_area_rect_size().x, RoomGenerationInfo::get().get_play_area_rect_size().y);
		if (auto* vr = VR::IVR::get())
		{
			s_a_o(TXT("    <float name=\"testScaling\">%.9f</float>"), vr->get_scaling().general);
		}
		else
		{
			s_a_o(TXT("    <float name=\"testScaling\">1.0</float>"));
		}
		s_a_o(TXT("    <name name=\"testPreferredTileSize\">%S</name>"), PreferredTileSize::to_char(RoomGenerationInfo::get().get_preferred_tile_size()));
		s_a_o(TXT("    <float name=\"testTileSize\">%.9f</float>"), RoomGenerationInfo::get().get_tile_size());
		s_a_o(TXT("    <float name=\"testDoorHeight\">%.9f</float>"), RoomGenerationInfo::get().get_door_height());
		s_a_o(TXT("    <bool name=\"testAllowCrouch\">%S</bool>"), RoomGenerationInfo::get().does_allow_crouch() ? TXT("true") : TXT("false"));
		s_a_o(TXT("    <bool name=\"testAllowAnyCrouch\">%S</bool>"), RoomGenerationInfo::get().does_allow_any_crouch() ? TXT("true") : TXT("false"));
		s_a_o(TXT("    <string name=\"testGenerationTags\">%S</string>"), RoomGenerationInfo::get().get_generation_tags().to_string(true).to_char());
		{
			Tags difficultySettingsTags;
			TeaForGodEmperor::GameSettings::get().difficulty.store_as_tags(difficultySettingsTags);
			s_a_o(TXT("    <string name=\"testDifficultySettings\">%S</string>"), difficultySettingsTags.to_string(true).to_char());
		}
		unlock_output();
	}
#endif
}

void PilgrimageInstanceLinear::create_and_start(GameState* _fromGameState, bool _asNextPilgrimageStart)
{
	todo_important(TXT("shouldn't you update this before using?"));
	game->access_game_tags().remove_tag(NAME(openWorld));
	game->access_game_tags().set_tag(NAME(linear));

	isBusyAsync = true;

	base::create_and_start(_fromGameState, _asNextPilgrimageStart);

	RefCountObjectPtr<PilgrimageInstance> keepThis(this);
	game->add_async_world_job(TXT("create and start"), [this, keepThis, _asNextPilgrimageStart]()
	{
		MEASURE_PERFORMANCE(createAndStart);
		refresh_force_instant_object_creation(false);
		Framework::ActivationGroupPtr activationGroup = game->push_new_activation_group();
		async_generate_base(_asNextPilgrimageStart);
		async_generate_starting_station(linear.partIdx);
		async_add_pilgrim_and_start();
		game->pop_activation_group(activationGroup.get());
		refresh_force_instant_object_creation(false); // clear
		isBusyAsync = false;
	});
}

void PilgrimageInstanceLinear::async_generate_starting_station(int _partIdx)
{
	ASSERT_ASYNC;

	refresh_force_instant_object_creation();

	an_assert(!linear.currentStation.is_set());
	linear.partIdx = _partIdx;
	linear.partTryIdx = 0;
	if (linear.active)
	{
		linear.currentPart = pilgrimage->linear__get_starting_part(randomSeed);
		for_count(int, i, linear.partIdx)
		{
			linear.currentPart = pilgrimage->linear__get_next_part(linear.currentPart.get(), randomSeed, i);
			if (!linear.currentPart.is_set())
			{
				break;
			}
		}

		linear.currentStation = create_station(0);

		generatedStartingStation = true;
	}
	else
	{
		todo_implement;
	}
}

Framework::Room* PilgrimageInstanceLinear::create_station(int _offset) const
{
	// generate starting room (ending room will be generated in a separate call)
	// 1. create room
	Framework::Room* room = nullptr;
	Framework::RoomType* roomType = _offset == 0 ? linear.currentPart->get_station_room_type() : linear.nextPart->get_station_room_type();
	Game::get()->perform_sync_world_job(TXT("create station"), [this, &room, roomType, _offset]()
	{
		room = new Framework::Room(subWorld, mainRegion.get(), roomType, get_station_rg(_offset));
		ROOM_HISTORY(room, TXT("station"));
	});
	// 2. generate room? or leave so it can be generated on demand or by other means
	{
		// generate before any door is put in, as for stations we base door on POIs
		room->generate();
	}

	room->ready_for_game();
	
	return room;
}

void PilgrimageInstanceLinear::async_add_pilgrim_and_start()
{
	ASSERT_ASYNC;

	refresh_force_instant_object_creation();

	auto* pilgrimType = pilgrimage->get_pilgrim();
	override_pilgrim_type(REF_ pilgrimType, nullptr);
	an_assert(pilgrimType);
	pilgrimType->load_on_demand_if_required();

	Framework::Actor* playerActor;
	game->perform_sync_world_job(TXT("create pilgrim"), [this, &playerActor, pilgrimType]()
	{
		playerActor = new Framework::Actor(pilgrimType, String(TXT("player")));
		playerActor->init(subWorld);
	});

	{
		Random::Generator rg = randomSeed;
		rg.advance_seed(239782, 23978234);
		playerActor->access_individual_random_generator() = rg;
	}

	game->access_player().bind_actor(playerActor, Framework::Player::BindActorParams().lazy_bind());
	playerActor->initialise_modules();
	game->access_player().bind_actor(playerActor);
	an_assert(linear.active, TXT("implement for others"));
	{	// place at vr anchor, will shift to proper position
		Transform vrAnchor = Transform::identity;
		linear.currentStation->for_every_point_of_interest(NAME(vrAnchor), [&vrAnchor](Framework::PointOfInterestInstance * _fpoi)
		{
			vrAnchor = _fpoi->calculate_placement();
		});
		game->perform_sync_world_job(TXT("place pilgrim"), [this, &playerActor, vrAnchor]()
		{
			playerActor->get_presence()->place_in_room(linear.currentStation.get(), vrAnchor);
		});
	}

	game->request_ready_and_activate_all_inactive(game->get_current_activation_group());
	
	game->request_initial_world_advance_required();

	game->add_sync_world_job(TXT("change vr anchor for room"), [this, playerActor]()
	{
		// makes sense only if vr is active
		if (VR::IVR::get() &&
			VR::IVR::get()->can_be_used() &&
			VR::IVR::get()->is_controls_view_valid())
		{
			Vector3 vrViewLoc = VR::IVR::get()->get_controls_view().get_translation();
			Transform playerVRAnchor = playerActor->get_presence()->get_vr_anchor();
			Transform doorVRAnchor;
			Transform doorPlacement;
			get_poi_placement(linear.currentStation.get(), NAME(outDoor), NAME(door), OUT_ doorVRAnchor, OUT_ doorPlacement);

			Vector3 vrDoorLoc = playerVRAnchor.location_to_local(doorPlacement.get_translation()); // player VR!
			Vector3 vrRotatedDoorLoc = -vrDoorLoc;

			float distNormal = (vrViewLoc - vrDoorLoc).length();
			float distRotated = (vrViewLoc - vrRotatedDoorLoc).length();

			if (distRotated > distNormal)
			{
				// rotate player's vr to place it further from the door
				playerActor->get_presence()->turn_vr_anchor_by_180();
			}
		}
	});

	// mark that we will be using world
	game->post_use_world(true);

	// switch into performance mode
	game->post_set_performance_mode(true);

	generatedPilgrimAndStarted = true;
}

void PilgrimageInstanceLinear::linear__end_part_and_advance_to_next()
{
	if (!linear.generatedPart || isBusyAsync)
	{
		return;
	}

	linear.generatedPart = false;
	linear.currentOpen = false;
	isBusyAsync = true;

	// check if there is something beyond nextPart (this is the station we're at, ending current->next)
	linear.reachedEnd = pilgrimage->linear__get_next_part(linear.nextPart.get(), randomSeed, linear.partIdx + 1) == nullptr;
	
	++reachedStations;

	RefCountObjectPtr<PilgrimageInstance> keepThis(this);
	game->add_async_world_job(TXT("end part and go to next"), [this, keepThis]()
	{
		MEASURE_PERFORMANCE(endPartAndGoToNext);
#ifdef AN_DEVELOPMENT
		{
			LogInfoContext log;
			Library::get_current()->log_short_info(log);
			log.output_to_output();
		}
#endif

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
		output_colour(0, 1, 1, 1);
		output(TXT("end this part..."));
		output(TXT("part idx %i"), linear.partIdx);
		output_colour();
		::System::TimeStamp timeStamp;
#endif

		if (linear.nextStation.is_set())
		{
			MEASURE_PERFORMANCE(crawlingDestruction);
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
			output(TXT("start crawling destruction from next station to delete current station too"));
#endif
			for_every_ptr(dir, linear.nextStation->get_all_doors())
			{
				// invisible doors are fine
				if (dir->get_tags().get_tag(NAME(autoStationDoor)))
				{
					// it is sync operation inside, issues asyncs to destroy what's beyond doors
					dir->start_crawling_destruction();
				}
			}
		}
		if (linear.currentRegion.is_set())
		{
			// will issue destruction of all leftover rooms
			linear.currentRegion->start_destruction();
		}

		pilgrimsDestination.clear();
		linear.currentRegion = nullptr;
		linear.currentStationDoor = nullptr;
		linear.nextStationDoor = nullptr;

		++linear.partIdx;
		linear.partTryIdx = 0;
		linear.currentPart = pilgrimage->linear__get_next_part(linear.currentPart.get(), randomSeed, linear.partIdx);
		linear.nextPart = pilgrimage->linear__get_next_part(linear.currentPart.get(), randomSeed, linear.partIdx + 1);
		environmentPt = 0.0f;
		linear.stationToStationDistance = 0.0f;
		linear.currentToStationDistance = 0.0f;

		linear.currentStation = linear.nextStation;
		linear.nextStation = nullptr;
		
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
		output_colour(0, 1, 1, 1);
		output(TXT("ended part in %.2fs"), timeStamp.get_time_since());
		output_colour();
#endif

		// because we reached end, remove unused temporary objects
		if (linear.reachedEnd)
		{
			game->request_removing_unused_temporary_objects();
		}

		isBusyAsync = false;
	});
}

void PilgrimageInstanceLinear::linear__create_part()
{
	an_assert(is_linear());
	if (isBusyAsync || linear.reachedEnd)
	{
		return;
	}
	an_assert(!linear.generatedPart);
	isBusyAsync = true;

	RefCountObjectPtr<PilgrimageInstance> keepThis(this);
	game->add_async_world_job(TXT("create next part"), [this, keepThis]()
	{
		MEASURE_PERFORMANCE(createNextPart);

		refresh_force_instant_object_creation();

		linear.currentOpen = false;
		linear.nextPart = pilgrimage->linear__get_next_part(linear.currentPart.get(), randomSeed, linear.partIdx + 1);

		if (!linear.nextPart.is_set())
		{
			linear.reachedEnd = true;
			isBusyAsync = false;

			return;
		}

		Framework::ActivationGroupPtr activationGroup = game->push_new_activation_group();

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
		output_colour(0, 1, 1, 1);
		output(TXT("create next part..."));
		output(TXT("seed %S"), randomSeed.get_seed_string().to_char());
		output(TXT("part idx %i:%i"), linear.partIdx, linear.partTryIdx);
		output_colour();
#endif

		output_dev_info(linear.partIdx);

#ifdef AN_OUTPUT_DEV_INFO
		{	// output in an easier to load way
			s_a_o(TXT("dev info (pilgrimage and setup)"));
			s_a_o(TXT("  pilgrimage: %S"), pilgrimage.get() ? pilgrimage->get_name().to_string().to_char() : TXT("--"));
			s_a_o(TXT("  region variables:"));
			SimpleVariableStorage regionVariables;
			GameSettings::get().setup_variables(regionVariables);
			LogInfoContext log;
			log.increase_indent();
			log.increase_indent();
			regionVariables.log(log);
			log.decrease_indent();
			log.decrease_indent();
			log.output_to_output();
			{
				String info;
				log.output_to_string(info);
				System::FaultHandler::add_custom_info(info);
			}
		}
#endif

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
		::System::TimeStamp timeStamp;
#endif
		
		game->clear_generation_issues();

		/**
		 *	plan of rooms that are going to be created:
		 *
		 *		current station -- current station exit -- <region> -- next station entrance -- next station
		 *
		 *	each -- is a door
		 */
		// create next station
		{
			MEASURE_PERFORMANCE(createNextStation);
			linear.nextStation = create_station(1);
		}

		// create station doors
		{
			MEASURE_PERFORMANCE(createStationDoors);

			linear.currentStationDoor = create_station_door(linear.currentStation.get(), NAME(outDoor), linear.currentPart->get_station_door_type(), Tags().set_tag(NAME(stationExit)));
			linear.nextStationDoor = create_station_door(linear.nextStation.get(), NAME(inDoor), linear.nextPart->get_station_door_type(), Tags().set_tag(NAME(stationEntrance)));
		}

		Framework::Region *newRegion = nullptr;

		// create current region
		{
			MEASURE_PERFORMANCE(createCurrentRegion);
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
			output_colour(0, 1, 1, 0);
			output(TXT("create region..."));
			output_colour();
			::System::TimeStamp ts;
#endif
			if (auto* rt = linear.currentPart->get_container_region_type()) { rt->load_on_demand_if_required(); }
			game->perform_sync_world_job(TXT("create current station"), [this]()
			{
				linear.currentRegion = new Framework::Region(subWorld, mainRegion.get(), linear.currentPart->get_container_region_type(), Random::Generator().be_zero_seed());
				linear.currentRegion->set_world_address_id(linear.currentRegionIdx);
			});

			// create station entrance/exit rooms
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
			output(TXT("create station entrance/exit rooms..."));
#endif
			Framework::Room* currentStationExit = create_station_entrance_exit_room(0);
			Framework::Room* nextStationEntrance = create_station_entrance_exit_room(1);

			// connect them to station doors
			if (!game->does_want_to_cancel_creation())
			{
				create_door_in_room(linear.currentStationDoor.get(), currentStationExit, Tags().set_tag(NAME(stationExit)));
				create_door_in_room(linear.nextStationDoor.get(), nextStationEntrance, Tags().set_tag(NAME(stationEntrance)));
			}

			if (!game->does_want_to_cancel_creation())
			{
				Random::Generator useRG = linear__get_part_rg();
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
				output(TXT("generate region using rg %S"), useRG.get_seed_string().to_char());
#endif

				if (Framework::RegionGenerator::generate_and_add_to_sub_world(subWorld, linear.currentRegion.get(), linear.currentPart->get_inner_region_type(), useRG, &useRG,
					[this, currentStationExit, nextStationEntrance, &useRG](PieceCombiner::Generator<Framework::RegionGenerator> & _generator, Framework::Region* _forRegion)
					{
						_generator.access_generation_context().generationTags = RoomGenerationInfo::get().get_generation_tags();

#ifdef AN_DEVELOPMENT_OR_PROFILER
						if (auto * dv = _generator.access_debug_visualiser())
						{
#ifdef OUTPUT_GENERATION_DEBUG_VISUALISER
							dv->activate();
#endif
						}
#endif
						// get slots
						Name startConnectorSlot(TXT("start"));
						Name endConnectorSlot(TXT("end"));
						PieceCombiner::Connector<Framework::RegionGenerator> const * useStartConnectorSlot = linear.currentPart->get_inner_region_type()->find_inside_slot_connector(startConnectorSlot);
						PieceCombiner::Connector<Framework::RegionGenerator> const * useEndConnectorSlot = linear.currentPart->get_inner_region_type()->find_inside_slot_connector(endConnectorSlot);
						an_assert(useStartConnectorSlot);
						an_assert(useEndConnectorSlot);

						// add door to entrance/exit basing on type
						if (!game->does_want_to_cancel_creation())
						{
							Framework::Door* startDoor = create_door(useStartConnectorSlot->def.doorType.get(), currentStationExit);
							Framework::Door* endDoor = create_door(useEndConnectorSlot->def.doorType.get(), nextStationEntrance);

							// setup slots (we connect everywhere here with A, so those will be B)
							_generator.add_outer_connector(useStartConnectorSlot, nullptr, Framework::RegionGenerator::ConnectorInstanceData(startDoor), startDoor->get_linked_door_a() ? PieceCombiner::ConnectorSide::A : PieceCombiner::ConnectorSide::B);
							_generator.add_outer_connector(useEndConnectorSlot, nullptr, Framework::RegionGenerator::ConnectorInstanceData(endDoor), endDoor->get_linked_door_a() ? PieceCombiner::ConnectorSide::A : PieceCombiner::ConnectorSide::B);
						}

						// may require outer connectors
						Framework::RegionGenerator::setup_generator(_generator, _forRegion, linear.currentPart->get_inner_region_type()->get_library(), useRG);
					},
					[](Framework::Region* region)
					{
						GameSettings::get().setup_variables(region->access_variables());
					},
					&newRegion, false))
				{}
			}

			if (!game->does_want_to_cancel_creation())
			{
				linear.currentStation->ready_for_game();
			}

#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
			output_colour(0, 1, 1, 0);
			output(TXT("generated current region in %.2fs"), ts.get_time_since());
			output_colour();
#endif
		}

#ifndef ALLOW_CANCELING_DUE_TO_GENERATION_ISSUES
		game->request_ready_and_activate_all_inactive(activationGroup.get());
		game->pop_activation_group(activationGroup.get());

		if (!game->does_want_to_cancel_creation() && !game->get_generation_issues().is_empty())
		{
			output_colour(1, 0, 0, 1);
			output(TXT("should cancel next part?"));
			if (game->get_generation_issues().is_empty())
			{
				output(TXT("no generation issues reported"));
			}
			else
			{
				for_every(issue, game->get_generation_issues())
				{
					output(TXT("  %S"), issue->to_char());
				}
			}
			output_colour();
		}
#else
		if (!game->does_want_to_cancel_creation() && game->get_generation_issues().is_empty())
		{
			game->request_ready_and_activate_all_inactive(activationGroup.get());
		}

		game->pop_activation_group(activationGroup.get());

		if (!game->does_want_to_cancel_creation() && !game->get_generation_issues().is_empty())
		{
			cancel_next_part();
		}
		else
#endif
		{
			if (!game->does_want_to_cancel_creation())
			{
				game->add_sync_world_job(TXT("end next region creation"),
					[this, newRegion, keepThis
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
					, timeStamp
#endif
					]()
				{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
					output_colour(0, 1, 1, 1);
					output(TXT("created next region in %.2fs"), timeStamp.get_time_since());
					output_colour();
#endif
					linear.currentRegion = newRegion;
					++linear.currentRegionIdx;
					pilgrimsDestination = linear.nextStation;
					pilgrimsDestinationSystem = Name::invalid();

					game->request_removing_unused_temporary_objects();
				});
			}

			linear.stationToStationDistance = 0.0f;
			linear.currentToStationDistance = 0.0f;
			environmentPt = 0.0f;

			linear.generatedPart = true;
			isBusyAsync = false;
		}
	});
}

void PilgrimageInstanceLinear::cancel_next_part()
{
	an_assert(linear.active, TXT("implement others"));
	linear.partTryIdx++;
	if (!game->does_want_to_cancel_creation())
	{
		game->add_sync_world_job(TXT("end next region creation"),
			[this]()
		{
#ifdef AN_OUTPUT_PILGRIMAGE_GENERATION
			output_colour(1, 0, 0, 1);
			output(TXT("cancel next part!"));
			for_every(issue, game->get_generation_issues())
			{
				output(TXT("  %S"), issue->to_char());
			}
			output_colour();
#endif
			linear.nextStation = nullptr;
			if (game)
			{
				game->drop_all_delayed_object_creations();
			}
			an_assert(isBusyAsync);
			for_every_ptr(dir, linear.currentStation->get_all_doors())
			{
				// invisible doors are fine
				if (dir->get_tags().get_tag(NAME(autoStationDoor)))
				{
					// it is sync operation inside
					dir->start_crawling_destruction();
				}
			}

			game->request_removing_unused_temporary_objects();

			wait_for_no_world_jobs_to_end_generation();
		});
	}
}

Framework::Door* PilgrimageInstanceLinear::create_station_door(Framework::Room* _station, Name const & _doorPOIName, Framework::DoorType* _doorType, Tags const & _doorInRoomTags) const
{ 
	MEASURE_PERFORMANCE(createStationDoor);

	Transform doorPlacement = Transform::identity;
	Transform vrAnchor = Transform::identity;
	get_poi_placement(_station, _doorPOIName, NAME(door), OUT_ vrAnchor, OUT_ doorPlacement);

	Framework::Door* door = create_door(_doorType);
	Framework::DoorInRoom* dir = create_door_in_room(door, _station, _doorInRoomTags);

	dir->access_tags().set_tag(NAME(autoStationDoor));
	dir->set_vr_space_placement(vrAnchor.to_local(doorPlacement));
	dir->set_placement(doorPlacement);
	dir->init_meshes();

	return door;
}

Framework::Room* PilgrimageInstanceLinear::create_station_entrance_exit_room(int _offset) const
{
	an_assert(linear.active, TXT("implement others"));
	an_assert(linear.currentRegion.is_set());

	Random::Generator rg = get_station_rg(_offset);
	if (_offset == 0)
	{
		rg = linear__offset_station_rg_to_exit(rg);
	}
	else
	{
		rg = linear__offset_station_rg_to_entrance(rg);
	}

	Framework::Room* stationEntranceExitRoom;
	Game::get()->perform_sync_world_job(TXT("create station entrance/exit"), [this, &stationEntranceExitRoom, _offset, rg]()
	{
		stationEntranceExitRoom = new Framework::Room(subWorld, linear.currentRegion.get(), _offset == 0? linear.currentPart->get_station_exit_room_type() : linear.nextPart->get_station_entrance_room_type(), rg);
		ROOM_HISTORY(stationEntranceExitRoom, TXT("create_station_entrance_exit_room"));
	});
	an_assert(stationEntranceExitRoom);

	return stationEntranceExitRoom;
}

Random::Generator PilgrimageInstanceLinear::linear__get_part_rg(int _offset) const
{
	Random::Generator rg = randomSeed;
	for_count(int, i, linear.partIdx + _offset + linear.partTryIdx)
	{
		rg.advance_seed(31497, 108153);
	}
	return rg;
}

Random::Generator PilgrimageInstanceLinear::get_station_rg(int _offset) const
{
	an_assert(linear.active, TXT("implement others"));
	Random::Generator rg = linear__get_part_rg(_offset);
	rg.advance_seed(235975, 925);
	return rg;
}

Random::Generator PilgrimageInstanceLinear::linear__offset_station_rg_to_entrance(Random::Generator const & _rg) const
{
	Random::Generator rg = _rg;
	rg.advance_seed(2552, 9768324);
	return rg;
}

Random::Generator PilgrimageInstanceLinear::linear__offset_station_rg_to_exit(Random::Generator const & _rg) const
{
	Random::Generator rg = _rg;
	rg.advance_seed(9725, 139675);
	return rg;
}

bool PilgrimageInstanceLinear::linear__may_advance_to_next_part() const
{
	return !isBusyAsync &&
		linear.generatedPart &&
		linear.nextStationDoor.is_set() &&
		linear.nextStationDoor->get_open_factor() == 0.0f; // has to be closed
}

bool PilgrimageInstanceLinear::linear__may_create_new_part() const
{
	return !isBusyAsync &&
		linear.currentStation.is_set() &&
		!linear.nextStation.is_set() &&
		!linear.generatedPart;
}

void PilgrimageInstanceLinear::linear__provide_distance_to_next_station(float _distance)
{
	an_assert(linear.active);
	if (linear.generatedPart)
	{
		if (linear.stationToStationDistance == 0.0f)
		{
			linear.stationToStationDistance = _distance;
		}
		linear.currentToStationDistance = _distance;
	}
}

void PilgrimageInstanceLinear::add_environment_mixes_if_no_cycles()
{
	add_mix(linear.currentPart->get_environments(), 1.0f - environmentPt);
	if (linear.nextPart.is_set())
	{
		add_mix(linear.nextPart->get_environments(), environmentPt);
	}
}

void PilgrimageInstanceLinear::pre_advance()
{
	base::pre_advance();

	if (auto* player = game->access_player().get_actor())
	{
		if (player->get_presence()->get_in_room() == linear__get_next_station() &&
			linear__may_advance_to_next_part())
		{
			linear__end_part_and_advance_to_next();
			if (linear__has_reached_end())
			{
				// #demo or maybe we should end game?
				game->end_demo();
			}
		}
		if (linear__may_create_new_part())
		{
			linear__create_part();
		}
	}
}

void PilgrimageInstanceLinear::advance(float _deltaTime)
{
	if (environmentCycleSpeed == 0.0f)
	{
		float targetEnvironmentPt = environmentPt;
		// this will work only if no cycle speed
		if (linear.active && linear.stationToStationDistance != 0.0f && linear.nextPart.is_set())
		{
			targetEnvironmentPt = clamp(1.0f - linear.currentToStationDistance / linear.stationToStationDistance, 0.0f, 1.0f);
		}
		environmentPt = blend_to_using_time(environmentPt, targetEnvironmentPt, firstAdvance ? 0.0f : 0.3f, _deltaTime);
	} // else handled by base

	base::advance(_deltaTime);
}

void PilgrimageInstanceLinear::debug_log_environments(LogInfoContext & _log) const
{
	if (linear.active)
	{
		_log.log(TXT("distance : %5.3f / %5.3f"), linear.currentToStationDistance, linear.stationToStationDistance);
	}
	base::debug_log_environments(_log);
}

String PilgrimageInstanceLinear::get_random_seed_info() const
{
	String info = String::printf(TXT("%S [%i]"), base::get_random_seed_info().to_char(), linear.partIdx);
	return info;
}

void PilgrimageInstanceLinear::log(LogInfoContext & _log) const
{
	base::log(_log);
	_log.log(TXT("part : %i"), linear.partIdx);
}

PilgrimageInstance::TransitionRoom PilgrimageInstanceLinear::async_create_transition_room_for_previous_pilgrimage(PilgrimageInstance* _previousPilgrimage)
{
	todo_implement;

	refresh_force_instant_object_creation();

	return PilgrimageInstance::TransitionRoom();
}

