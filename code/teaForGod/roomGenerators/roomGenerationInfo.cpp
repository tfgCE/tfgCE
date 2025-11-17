#include "roomGenerationInfo.h"

#include "..\game\playerSetup.h"

#include "..\..\core\vr\iVR.h"

#include "..\..\framework\debug\testConfig.h"
#include "..\..\framework\game\bullshotSystem.h"
#include "..\..\framework\game\game.h"
#include "..\..\framework\text\localisedString.h"
#include "..\..\framework\world\door.h"

// to register

// elements, not actual room generators, they are used mostly to provide background
#include "elementGenerators\doorBasedSceneries.h"
#include "elementGenerators\doorBasedShaft.h"
#include "elementGenerators\doorBasedWalls.h"
#include "elementGenerators\simpleElevatorWall.h"

// actual, more sophisticated room generators
#include "roomGenerators\balconies.h"
#include "roomGenerators\empty.h"
#include "roomGenerators\floorsWithElevators.h"
#include "roomGenerators\platformsLinear.h"
#include "roomGenerators\roomWithDoorBlocks.h"
#include "roomGenerators\simpleCorridor.h"
#include "roomGenerators\simpleElevator.h"
#include "roomGenerators\simpleRoom.h"
#include "roomGenerators\towers.h"
#include "roomGenerators\useMeshes.h"
#include "roomGenerators\useMeshGenerator.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
#define AN_OUTPUT_MIN_H_SCALING
#endif
#define AN_OUTPUT_DETAILS

//

DEFINE_STATIC_NAME_STR(lsTileCountTiny, TXT("room generation info; tile count; tiny"));
DEFINE_STATIC_NAME_STR(lsTileCountBasic, TXT("room generation info; tile count; basic"));
DEFINE_STATIC_NAME_STR(lsTileCountNormal, TXT("room generation info; tile count; normal"));
DEFINE_STATIC_NAME_STR(lsTileCountLong, TXT("room generation info; tile count; long"));
DEFINE_STATIC_NAME_STR(lsTileCountLarge, TXT("room generation info; tile count; large"));
DEFINE_STATIC_NAME_STR(lsTileCountVeryLarge, TXT("room generation info; tile count; very large"));

//

// generation tags
DEFINE_STATIC_NAME(allowCrouch);
DEFINE_STATIC_NAME(allowAnyCrouch);

// bullshot system allowances
DEFINE_STATIC_NAME_STR(bsNoRGITestSettings, TXT("no RGI test settings"));

// test tags
DEFINE_STATIC_NAME(testPlayArea);
DEFINE_STATIC_NAME(testScaling);
DEFINE_STATIC_NAME(testTileSize);
DEFINE_STATIC_NAME(testPreferredTileSize);
DEFINE_STATIC_NAME(testDoorHeight);
DEFINE_STATIC_NAME(testGenerationTags);
DEFINE_STATIC_NAME(testAllowCrouch);
DEFINE_STATIC_NAME(testAllowAnyCrouch);
DEFINE_STATIC_NAME(testImmobileVR);
DEFINE_STATIC_NAME(testGeneralProgressProfile);
DEFINE_STATIC_NAME(testGeneralProgressGameSlot);

//

using namespace TeaForGodEmperor;

RoomGenerationInfo* RoomGenerationInfo::s_globalInfo = nullptr;
float const RoomGenerationInfo::MIN_TILE_SIZE = 0.6f;
float const RoomGenerationInfo::MAX_TILE_SIZE = 1.5f;
float const RoomGenerationInfo::MIN_PLAY_AREA_WIDTH = RoomGenerationInfo::MIN_TILE_SIZE * 3.0f;
float const RoomGenerationInfo::MIN_PLAY_AREA_LENGTH = RoomGenerationInfo::MIN_TILE_SIZE * 2.0f;

void RoomGenerationInfo::initialise_static()
{
	an_assert(!s_globalInfo);
	s_globalInfo = new RoomGenerationInfo();

	Framework::RoomGeneratorInfo::register_room_generator_info(String(TXT("balconies")), []() { return new RoomGenerators::BalconiesInfo(); });
	Framework::RoomGeneratorInfo::register_room_generator_info(String(TXT("empty")), []() { return new RoomGenerators::Empty(); });
	Framework::RoomGeneratorInfo::register_room_generator_info(String(TXT("floors with elevators")), []() { return new RoomGenerators::FloorsWithElevatorsInfo(); });
	Framework::RoomGeneratorInfo::register_room_generator_info(String(TXT("door based shaft")), []() { return new RoomGenerators::DoorBasedShaftInfo(); });
	Framework::RoomGeneratorInfo::register_room_generator_info(String(TXT("door based sceneries")), [](){ return new RoomGenerators::DoorBasedSceneriesInfo(); });
	Framework::RoomGeneratorInfo::register_room_generator_info(String(TXT("door based walls")), [](){ return new RoomGenerators::DoorBasedWallsInfo(); });
	Framework::RoomGeneratorInfo::register_room_generator_info(String(TXT("platforms linear")), [](){ return new RoomGenerators::PlatformsLinearInfo(); });
	Framework::RoomGeneratorInfo::register_room_generator_info(String(TXT("room with door blocks")), []() { return new RoomGenerators::RoomWithDoorBlocksInfo(); });
	Framework::RoomGeneratorInfo::register_room_generator_info(String(TXT("simple corridor")), [](){ return new RoomGenerators::SimpleCorridorInfo(); });
	Framework::RoomGeneratorInfo::register_room_generator_info(String(TXT("simple elevator")), [](){ return new RoomGenerators::SimpleElevatorInfo(); });
	Framework::RoomGeneratorInfo::register_room_generator_info(String(TXT("simple elevator wall")), [](){ return new RoomGenerators::SimpleElevatorWallInfo(); });
	Framework::RoomGeneratorInfo::register_room_generator_info(String(TXT("simple room")), [](){ return new RoomGenerators::SimpleRoomInfo(); });
	Framework::RoomGeneratorInfo::register_room_generator_info(String(TXT("towers")), [](){ return new RoomGenerators::TowersInfo(); });
	Framework::RoomGeneratorInfo::register_room_generator_info(String(TXT("use mesh generator")), []() { return new RoomGenerators::UseMeshGeneratorInfo(); });
	Framework::RoomGeneratorInfo::register_room_generator_info(String(TXT("use meshes")), []() { return new RoomGenerators::UseMeshesInfo(); });

	// initial setup, just to have something
	s_globalInfo->setup_internal(false);
}

void RoomGenerationInfo::close_static()
{
	delete_and_clear(s_globalInfo);
}

RoomGenerationInfo::RoomGenerationInfo()
: requestedDoorHeight(Framework::Door::get_default_nominal_door_height())
, doorHeight(Framework::Door::get_default_nominal_door_height())
{
}

static void reduce_tile_count(Random::Generator & _random, int & _tileCount)
{
	float chance = 0.5f;
	while (true)
	{
		if (_random.get_chance(chance) && _tileCount > 3)
		{
			--_tileCount;
		}
		else
		{
			break;
		}
		chance *= 0.5f;
	}
}

void RoomGenerationInfo::modify_randomly(Random::Generator const & _generator)
{
	Random::Generator random = _generator;
	// update tile size
	if (random.get_chance(0.5f))
	{
		// less tiles
		if (random.get_chance(0.3f))
		{
			reduce_tile_count(random, tileCount.x);
			reduce_tile_count(random, tileCount.y);
			tileSize = min(playAreaRectSize.x / (float)tileCount.x, playAreaRectSize.y / (float)tileCount.y);
		}
	}
	else
	{
		// more tiles
		if (random.get_chance(0.7f))
		{
			tileSize = max(MIN_TILE_SIZE, 0.4f + random.get_float(0.0f, tileSize - 0.4f));
		}
	}

	// grow tile size if we would still fit within play area
	float const e = 0.0001f;
	tileCount = VectorInt2((int)(playAreaRectSize.x / tileSize + e), (int)(playAreaRectSize.y / tileSize + e));
	float growingChance = 0.7f;
	while (true)
	{
		float newTileSize = tileSize + 0.01f;
		VectorInt2 newTileCount((int)(playAreaRectSize.x / newTileSize + e), (int)(playAreaRectSize.y / newTileSize + e));
		if (newTileCount.x < tileCount.x ||
			newTileCount.y < tileCount.y ||
			random.get_chance(growingChance))
		{
			break;
		}
		tileSize = newTileSize;
		growingChance = min(0.95f, growingChance + 0.05f);
	}

	calculate_auto_values();
}

void RoomGenerationInfo::limit_to(Vector2 const & _maxSize, Optional<float> const& _forceTileSize, Random::Generator const & _generator)
{
	Random::Generator random = _generator;

	playAreaRectSize.x = min(playAreaRectSize.x, _maxSize.x);
	playAreaRectSize.y = min(playAreaRectSize.y, _maxSize.y);

	if (_forceTileSize.is_set())
	{
		tileSize = _forceTileSize.get();

		float const e = 0.0001f;
		tileCount = VectorInt2((int)(playAreaRectSize.x / tileSize + e), (int)(playAreaRectSize.y / tileSize + e));
		while (tileCount.x < 3 && tileCount.y < 2)
		{
			tileSize = tileSize - 0.01f;
			tileCount = VectorInt2((int)(playAreaRectSize.x / tileSize + e), (int)(playAreaRectSize.y / tileSize + e));
			if (tileSize < MIN_TILE_SIZE)
			{
				tileSize = MIN_TILE_SIZE;
				playAreaRectSize.x = max(playAreaRectSize.x, tileSize * 3.0f);
				playAreaRectSize.y = max(playAreaRectSize.y, tileSize * 2.0f);
				break;
			}
		}
	}
	else
	{
		calculate_tile_count_with_random_growing(random);
	}

	calculate_auto_values();
}

void RoomGenerationInfo::calculate_tile_count_with_random_growing(Random::Generator const & _generator)
{
	Random::Generator random = _generator;

	// grow tile size if we would still fit within play area
	float const e = 0.0001f;
	tileCount = VectorInt2((int)(playAreaRectSize.x / tileSize + e), (int)(playAreaRectSize.y / tileSize + e));
	float growingChance = 0.7f;
	while (true)
	{
		float newTileSize = tileSize + 0.01f;
		VectorInt2 newTileCount((int)(playAreaRectSize.x / newTileSize + e), (int)(playAreaRectSize.y / newTileSize + e));
		if (newTileCount.x < tileCount.x ||
			newTileCount.y < tileCount.y ||
			random.get_chance(growingChance))
		{
			break;
		}
		tileSize = newTileSize;
		growingChance = min(0.95f, growingChance + 0.05f);
	}

	tileSize = clamp(tileSize, MIN_TILE_SIZE, MAX_TILE_SIZE);
}

void RoomGenerationInfo::setup_internal(bool _output, bool _preview)
{
	bool allowTestSettings = _preview; // allow test settings for preview only

#ifndef BUILD_PUBLIC_RELEASE
	allowTestSettings = true;
#endif

#ifdef AN_ALLOW_BULLSHOTS
	if (Framework::BullshotSystem::is_setting_active(NAME(bsNoRGITestSettings)))
	{
		allowTestSettings = false;
	}
#endif

	playAreaEnlarged = false;
	playAreaForcedScaled = false;

	if (!_preview)
	{
		if (auto const* vr = VR::IVR::get())
		{
			if (vr->is_ok())
			{
				Vector2 rawPlayAreaRectSize;
				rawPlayAreaRectSize.x = vr->get_raw_whole_play_area_rect_half_right().length() * 2.0f;
				rawPlayAreaRectSize.y = vr->get_raw_whole_play_area_rect_half_forward().length() * 2.0f;
				float useMinHScaling = clamp(max(MIN_PLAY_AREA_WIDTH / max(0.01f, rawPlayAreaRectSize.x), MIN_PLAY_AREA_LENGTH / max(0.01f, rawPlayAreaRectSize.y)), 1.0f, 2.0f);
				if (vr->may_have_invalid_boundary())
				{
					useMinHScaling = 1.0f;
				}
#ifdef AN_OUTPUT_MIN_H_SCALING
				if (_output)
				{
					output(TXT("raw play area rect size: %.2f x %.2f"), rawPlayAreaRectSize.x, rawPlayAreaRectSize.y);
					output(TXT("min play area size required: %.2f x %.2f"), MIN_PLAY_AREA_WIDTH, MIN_PLAY_AREA_LENGTH);
					output(TXT("use min h-scaling %.1f%%"), useMinHScaling * 100.0f);
				}
#endif

				MainConfig::access_global().set_vr_horizontal_scaling_min_value(useMinHScaling);

				playAreaForcedScaled = useMinHScaling > 1.05f; // accept up to 5% scale, pretend we're good
			}
		}
	}

	// assume by default we are working with non-vr (development) build
	// if we want to test particular size, do it via testPlayArea debug/test variable
	// otherwise pretend it is immobile vr
	playAreaRectSize = Vector2(10.0f, 10.0f); // for immobile vr, pretend we have lots of space

	bool usingVR = allowTestSettings? testVR : false;

	float useScaling = 1.0f;

	if (auto const * vr = VR::IVR::get())
	{
		if (vr->is_ok())
		{
			usingVR = true;
			playAreaRectSize.x = vr->get_play_area_rect_half_right().length() * 2.0f;
			playAreaRectSize.y = vr->get_play_area_rect_half_forward().length() * 2.0f;
			useScaling = vr->get_scaling().general;
#ifdef AN_OUTPUT_DETAILS
			if (_output)
			{
				output(TXT("play area from vr: %.2f x %.2f"), playAreaRectSize.x, playAreaRectSize.y);
			}
#endif
		}
	}

	if (allowTestSettings &&
		testPlayAreaRectSize.is_set() && (!testPlayAreaRectSizeLoaded || forceTestPlayAreaRectSize))
	{
		playAreaRectSize = testPlayAreaRectSize.get();
		usingVR = true;
#ifdef AN_OUTPUT_DETAILS
		if (_output)
		{
			output(TXT("using testPlayAreaRectSize: %.2f x %.2f"), playAreaRectSize.x, playAreaRectSize.y);
		}
#endif
	}

	if (allowTestSettings &&
		testVRScaling.is_set())
	{
		useScaling = testVRScaling.get();
	}

#ifdef AN_OUTPUT_DETAILS
	if (_output)
	{
		LogInfoContext log;
		log.log(TXT("will be using test config:"));
		{
			LOG_INDENT(log);
			Framework::TestConfig::get_params().log(log);
		}
		log.output_to_output();
	}
#endif

	// this should be at the end as it is the most important setting
	if (allowTestSettings)
	{	// forced params
		if (auto* param = Framework::TestConfig::get_params().get_existing<bool>(NAME(testImmobileVR)))
		{
			// use it as it may affect gameplay, but only if vr is on, otherwise we're stuck to debug/dev that works as immobile vr
			if (VR::IVR::get())
			{
				MainConfig::access_global().set_immobile_vr(*param? Option::True : Option::False);
			}
		}
		if (auto * param = Framework::TestConfig::get_params().get_existing<Vector2>(NAME(testPlayArea)))
		{
			playAreaRectSize.x = param->x;
			playAreaRectSize.y = param->y;
#ifdef AN_OUTPUT_DETAILS
			if (_output)
			{
				output(TXT("test play area: %.2f x %.2f"), playAreaRectSize.x, playAreaRectSize.y);
			}
#endif
		}
		if (auto * param = Framework::TestConfig::get_params().get_existing<float>(NAME(testScaling)))
		{
			useScaling = *param;
		}
		if (auto * param = Framework::TestConfig::get_params().get_existing<float>(NAME(testTileSize)))
		{
			if (*param > 0.0f)
			{
				testTileSize = *param;
			}
			else
			{
				testTileSize.clear();
			}
		}
		if (auto * param = Framework::TestConfig::get_params().get_existing<Name>(NAME(testPreferredTileSize)))
		{
			preferredTileSize = PreferredTileSize::parse(param->to_string());
		}
		if (auto * param = Framework::TestConfig::get_params().get_existing<float>(NAME(testDoorHeight)))
		{
			testDoorHeight = *param;
		}
		if (auto * param = Framework::TestConfig::get_params().get_existing<String>(NAME(testGenerationTags)))
		{
			testGenerationTags = Tags();
			testGenerationTags.access().load_from_string(*param);
		}		
		if (auto * param = Framework::TestConfig::get_params().get_existing<bool>(NAME(testAllowCrouch)))
		{
			testAllowCrouch = *param;
		}
		if (auto * param = Framework::TestConfig::get_params().get_existing<bool>(NAME(testAllowAnyCrouch)))
		{
			testAllowAnyCrouch = *param;
		}
		if (auto* param = Framework::TestConfig::get_params().get_existing<String>(NAME(testGeneralProgressProfile)))
		{
			Tags tags;
			tags.load_from_string(*param);

			if (auto* p = PlayerSetup::access_current_if_exists())
			{
				p->access_general_progress().clear();
				p->access_general_progress().set_tags_from(tags);
			}
		}
		if (auto* param = Framework::TestConfig::get_params().get_existing<String>(NAME(testGeneralProgressGameSlot)))
		{
			Tags tags;
			tags.load_from_string(*param);

			if (auto* p = PlayerSetup::access_current_if_exists())
			{
				if (auto* gs = p->access_active_game_slot())
				{
					gs->access_general_progress().clear();
					gs->access_general_progress().set_tags_from(tags);
				}
			}
		}
	}

#ifdef AN_OUTPUT_DETAILS
	if (_output)
	{
		output(TXT("before rounding to centimeters: %.6f x %.6f"), playAreaRectSize.x, playAreaRectSize.y);
	}
#endif

	// round to centimeters
	playAreaRectSize.x = round(playAreaRectSize.x * 100.0f) / 100.0f;
	playAreaRectSize.y = round(playAreaRectSize.y * 100.0f) / 100.0f;

#ifdef AN_OUTPUT_DETAILS
	if (_output)
	{
		output(TXT("rounded to centimeters: %.6f x %.6f"), playAreaRectSize.x, playAreaRectSize.y);
	}
#endif

	// this is a bold assumption assuming that play area zone is centred
	playAreaZone.be_rect(playAreaRectSize);

	if (!usingVR)
	{
		if (allowTestSettings &&
			testTileSize.is_set())
		{
			tileSize = max(MIN_TILE_SIZE, testTileSize.get());
#ifdef AN_OUTPUT_DETAILS
			if (_output)
			{
				output(TXT("[nvr] test tile size: %.3f"), tileSize);
			}
#endif
		}
		else if (forceTileSize.is_set())
		{
			tileSize = max(MIN_TILE_SIZE, forceTileSize.get());
#ifdef AN_OUTPUT_DETAILS
			if (_output)
			{
				output(TXT("[nvr] force tile size: %.3f"), tileSize);
			}
#endif
		}
		else
		{
			PreferredTileSize::Type pts = get_preferred_tile_size();
			if (pts == PreferredTileSize::Auto)
			{
				float smallerSize = max(playAreaRectSize.x, playAreaRectSize.y);
				float count = 2.0f;
				tileSize = smallerSize / count;
				float safeTileSize = tileSize;
				while (tileSize > 0.7f && (tileSize > 1.1f || count < 6.0f))
				{
					safeTileSize = tileSize;
					count += 1.0f;
					tileSize = smallerSize / count;
				}
				tileSize = safeTileSize;
#ifdef AN_OUTPUT_DETAILS
				if (_output)
				{
					output(TXT("[nvr] auto tile size: %.3f (smaller-size:%.3f count?:%.0f)"), tileSize, smallerSize, count);
				}
#endif
			}
			else if (pts == PreferredTileSize::Largest)
			{
				tileSize = MAX_TILE_SIZE;
#ifdef AN_OUTPUT_DETAILS
				if (_output)
				{
					output(TXT("[nvr] largest tile size: %.3f"), tileSize);
				}
#endif
			}
			else if (pts == PreferredTileSize::Large)
			{
				tileSize = 1.2f;
#ifdef AN_OUTPUT_DETAILS
				if (_output)
				{
					output(TXT("[nvr] large tile size: %.3f"), tileSize);
				}
#endif
			}
			else if (pts == PreferredTileSize::Medium)
			{
				tileSize = 0.75f;
#ifdef AN_OUTPUT_DETAILS
				if (_output)
				{
					output(TXT("[nvr] medium tile size: %.3f"), tileSize);
				}
#endif
			}
			else
			{
				tileSize = MIN_TILE_SIZE;
#ifdef AN_OUTPUT_DETAILS
				if (_output)
				{
					output(TXT("[nvr] smallest tile size: %.3f"), tileSize);
				}
#endif
			}
		}
		playAreaRectSize = Vector2(5.0f, 4.0f) * tileSize; // this is for test (4x3?), for non vr it is better to have tileSize 0.9
		playAreaZone.be_rect(playAreaRectSize);
	}

	if (playAreaRectSize.x < MIN_PLAY_AREA_WIDTH || playAreaRectSize.y < MIN_PLAY_AREA_LENGTH)
	{
		if (!_preview)
		{
			if (_output)
			{
				warn(TXT("play area too small (%.2fx%.2f), growing to least %.2fx%.2f!"),
					playAreaRectSize.x,
					playAreaRectSize.y,
					max(playAreaRectSize.x, MIN_PLAY_AREA_WIDTH),
					max(playAreaRectSize.y, MIN_PLAY_AREA_LENGTH));
			}
		}

		if (playAreaRectSize.x < MIN_PLAY_AREA_WIDTH || playAreaRectSize.y < MIN_PLAY_AREA_LENGTH)
		{
			playAreaRectSize.x = max(playAreaRectSize.x, MIN_PLAY_AREA_WIDTH);
			playAreaRectSize.y = max(playAreaRectSize.y, MIN_PLAY_AREA_LENGTH);
			playAreaEnlarged = true;
		}
		playAreaZone.be_rect(playAreaRectSize);
	}

	// assume tile size depending on play area size (if play area is bigger, allow bigger tiles
	float biggerSize = max(playAreaRectSize.x, playAreaRectSize.y);
	float smallerSize = min(playAreaRectSize.x, playAreaRectSize.y);
	tileSize = MIN_TILE_SIZE;
	{
		if (allowTestSettings &&
			testTileSize.is_set())
		{
			tileSize = max(MIN_TILE_SIZE, testTileSize.get());
#ifdef AN_OUTPUT_DETAILS
			if (_output)
			{
				output(TXT("test tile size: %.3f"), tileSize);
			}
#endif
		}
		else if (forceTileSize.is_set())
		{
			tileSize = max(MIN_TILE_SIZE, forceTileSize.get());
#ifdef AN_OUTPUT_DETAILS
			if (_output)
			{
				output(TXT("force tile size: %.3f"), tileSize);
			}
#endif
		}
		else
		{
			PreferredTileSize::Type pts = get_preferred_tile_size();
			if (pts == PreferredTileSize::Auto)
			{
				float count = smallerSize > MIN_PLAY_AREA_WIDTH ? 3.0f : 2.0f;
				tileSize = smallerSize / count;
				float safeTileSize = tileSize;
				while (tileSize > 0.7f && (tileSize > 1.1f || count < 6.0f))
				{
					safeTileSize = tileSize;
					count += 1.0f;
					tileSize = smallerSize / count;
				}
				tileSize = safeTileSize;
#ifdef AN_OUTPUT_DETAILS
				if (_output)
				{
					output(TXT("auto tile size: %.3f (smaller-size:%.3f count?:%.0f)"), tileSize, smallerSize, count);
				}
#endif
			}
			else if (pts == PreferredTileSize::Largest)
			{
				tileSize = min(MAX_TILE_SIZE, smallerSize / 2.0f);
#ifdef AN_OUTPUT_DETAILS
				if (_output)
				{
					output(TXT("largest tile size: %.3f"), tileSize);
				}
#endif
			}
			else if (pts == PreferredTileSize::Large)
			{
				tileSize = min(1.2f, smallerSize / 2.0f);
#ifdef AN_OUTPUT_DETAILS
				if (_output)
				{
					output(TXT("large tile size: %.3f"), tileSize);
				}
#endif
			}
			else if (pts == PreferredTileSize::Medium)
			{
				tileSize = min(0.75f, smallerSize / 2.0f);
#ifdef AN_OUTPUT_DETAILS
				if (_output)
				{
					output(TXT("medium tile size: %.3f"), tileSize);
				}
#endif
			}
			else
			{
				tileSize = MIN_TILE_SIZE;
#ifdef AN_OUTPUT_DETAILS
				if (_output)
				{
					output(TXT("smallest tile size: %.3f"), tileSize);
				}
#endif
			}
		}
		if (usingVR)
		{
			if (auto const* vr = VR::IVR::get())
			{
				// if we're scaling up, make tiles bigger a bit (not as the scale but just a bit)
				if (useScaling > 1.0f)
				{
					tileSize *= max(1.0f, 1.0f + (useScaling - 1.0f) * 0.05f);
				}
			}
		}
		// we need at least 2 tiles on y and 3 tiles on x!
		float maxTileSize = min(playAreaRectSize.y / 2.0f, playAreaRectSize.x / 3.0f);
		tileSize = min(tileSize, maxTileSize - 0.01f);
#ifdef AN_OUTPUT_DETAILS
		if (_output)
		{
			output(TXT("tile size (fixed): %.3f"), tileSize);
		}
#endif
	}

	float const e = 0.0001f;
	tileCount = VectorInt2((int)(playAreaRectSize.x / tileSize + e), (int)(playAreaRectSize.y / tileSize + e));
	// grow tile size if we would still fit within play area
	if (!forceTileSize.is_set() && 
		(!allowTestSettings || !testTileSize.is_set()))
	{
		// we're growing, there is no chance tile count would go bigger, if it goes smaller, stop
		// but we want to check both axes, if one is much longer, allow other to grow getting more square-like shape but to use smaller space
		VectorInt2 allowSmaller = VectorInt2::zero;
		if (biggerSize >= smallerSize + tileSize)
		{
			// note that we may get smaller by more than one tile, we will use difference to stay square at the worst scenario
			if (playAreaRectSize.x > playAreaRectSize.y)
			{
				allowSmaller = VectorInt2(tileCount.y - tileCount.x, 0);
			}
			else
			{
				allowSmaller = VectorInt2(0, tileCount.x - tileCount.y);
			}
		}
		{
			while (true)
			{
				float newTileSize = tileSize + 0.01f;
				VectorInt2 newTileCount((int)(playAreaRectSize.x / newTileSize + e), (int)(playAreaRectSize.y / newTileSize + e));
				if (newTileCount.x < tileCount.x + allowSmaller.x ||
					newTileCount.y < tileCount.y + allowSmaller.y ||
					newTileCount.x < 3 ||
					newTileCount.y < 2)
				{
					break;
				}
				tileSize = newTileSize;
			}
		}
#ifdef AN_OUTPUT_DETAILS
		if (_output)
		{
			output(TXT("tile size (grown): %.3f"), tileSize);
		}
#endif
		// update in case we grew smaller
		tileCount = VectorInt2((int)(playAreaRectSize.x / tileSize + e), (int)(playAreaRectSize.y / tileSize + e));
	}
	// make tiles smaller to get at least 3x2
	while (true)
	{
		float newTileSize = tileSize - 0.01f;
		VectorInt2 newTileCount((int)(playAreaRectSize.x / newTileSize + e), (int)(playAreaRectSize.y / newTileSize + e));
		if ((newTileCount.x >= 3 &&
			 newTileCount.y >= 2) ||
			newTileSize < MIN_TILE_SIZE)
		{
			break;
		}
		tileSize = newTileSize;
	}
#ifdef AN_OUTPUT_DETAILS
	if (_output)
	{
		output(TXT("tile size (fixed for 3x2): %.3f"), tileSize);
	}
#endif

	tileSize = clamp(tileSize, MIN_TILE_SIZE, MAX_TILE_SIZE);

	calculate_auto_values();

	if (allowTestSettings)
	{
		doorHeight = testDoorHeight.get(requestedDoorHeight);
		generationTags = testGenerationTags.get(requestedGenerationTags);
	}
	else
	{
		doorHeight = requestedDoorHeight;
		generationTags = requestedGenerationTags;
	}

	{
		allowCrouch = requestedAllowCrouch == Option::Allow;
		allowAnyCrouch = requestedAllowCrouch == Option::Allow || requestedAllowCrouch == Option::Auto;
		if (requestedAllowCrouch == Option::Auto && (!allowTestSettings || ! testAllowCrouch.is_set()))
		{
			allowCrouch = tileSize >= 0.70f;
		}

		if (allowTestSettings)
		{
			allowCrouch = testAllowCrouch.get(allowCrouch);
			allowAnyCrouch = testAllowAnyCrouch.get(allowAnyCrouch);
		}

		if (allowCrouch)
		{
			generationTags.set_tag(NAME(allowCrouch));
		}
		else
		{
			generationTags.remove_tag(NAME(allowCrouch));
		}
		if (allowAnyCrouch)
		{
			generationTags.set_tag(NAME(allowAnyCrouch));
		}
		else
		{
			generationTags.remove_tag(NAME(allowAnyCrouch));
		}
	}

	if (_output)
	{
		output_colour_system();
		output(TXT("room generator: %S"), usingVR ? TXT("using vr") : TXT("non vr"));
		output(TXT("play area rect size: %.2fm x %.2fm"), playAreaRectSize.x, playAreaRectSize.y);
		if (auto const* vr = VR::IVR::get())
		{
			if (vr->is_ok())
			{
				output(TXT("vr scale: %.2f%%"), vr->get_scaling().general * 100.0f);
				output(TXT("vr h-scale: %.2f%%"), vr->get_scaling().horizontal * 100.0f);
			}
		}
		output(TXT("use scaling: %.2f%%"), useScaling * 100.0f);
		output(TXT("use min h-scaling: %.2f%%"), MainConfig::access_global().get_vr_horizontal_scaling_min_value() * 100.0f);
		output(TXT("tile size: %.2fm"), tileSize);
		output(TXT("tile count: %i x %i"), tileCount.x, tileCount.y);
		output(TXT("door height: %.2fm"), doorHeight);
		output(TXT("allow crouch: %S"), allowCrouch? TXT("true") : TXT("false"));
		output(TXT("allow any crouch: %S"), allowAnyCrouch? TXT("true") : TXT("false"));
		output(TXT("generation tags: %S"), generationTags.to_string().to_char());
		output_colour();
	}

	if (!_preview)
	{
		Framework::Door::set_nominal_door_size(tileSize, doorHeight);

		if (requiresDoorRegeneration)
		{
			requiresDoorRegeneration = false;
			Framework::DoorType::generate_all_meshes();
		}

		if (auto* gsi = Framework::GameStaticInfo::get())
		{
			gsi->set_vr_zone_and_tile_size(get_play_area_zone(), get_tile_size());
		}
	}
}

void RoomGenerationInfo::calculate_auto_values()
{
	float const e = 0.0001f;
	tileCount = VectorInt2((int)(playAreaRectSize.x / tileSize + e), (int)(playAreaRectSize.y / tileSize + e));

	an_assert(tileCount.x >= 3);
	an_assert(tileCount.y >= 2);

	playAreaOffset.x = -playAreaRectSize.x * 0.5f;
	playAreaOffset.y = -playAreaRectSize.y * 0.5f;

	// calculate offset for first tile
	firstTileCentreOffset.x = -((float)tileCount.x - 1.0f) * 0.5f * tileSize;
	firstTileCentreOffset.y = -((float)tileCount.y - 1.0f) * 0.5f * tileSize;

	// tiles zone rectangle
	tilesZoneOffset.x = firstTileCentreOffset.x - tileSize * 0.5f;
	tilesZoneOffset.y = firstTileCentreOffset.y - tileSize * 0.5f;

	tileZone.be_rect(Vector2(tileSize, tileSize));
	tilesZone.be_rect(Vector2(tileSize * (float(tileCount.x)), tileSize * (float(tileCount.y))));
}

bool RoomGenerationInfo::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("roomGenerationInfo")))
	{
		if (!node->get_bool_attribute_or_from_child_presence(TXT("keepSettings")))
		{
			// clear only if the node exists
			preferredTileSize = PreferredTileSize::Auto;
			forceTileSize.clear();
			requestedGenerationTags.clear();
		}

		String attr = node->get_string_attribute_or_from_child(TXT("preferredTileSize"));
		if (!attr.is_empty())
		{
			preferredTileSize = PreferredTileSize::parse(attr, preferredTileSize);
		}
		{
			float temp = node->get_float_attribute_or_from_child(TXT("forceTileSize"));
			if (temp > 0.0f)
			{
				forceTileSize = max(temp, MIN_TILE_SIZE);
			}
		}
		testVR = node->get_bool_attribute_or_from_child(TXT("testVR"), testVR);
		testPlayAreaRectSize.load_from_xml(node, TXT("testPlayAreaRectSize"));
		testPlayAreaRectSizeLoaded = testPlayAreaRectSize.is_set();
		forceTestPlayAreaRectSize = node->get_bool_attribute_or_from_child_presence(TXT("forceTestPlayAreaRectSize"), forceTestPlayAreaRectSize);
		requestedGenerationTags.load_from_xml_attribute_or_child_node(node, TXT("generationTags"));
	}

	calculate_auto_values(); // will recalculate on setup

	return result;
}

bool RoomGenerationInfo::save_to_xml(IO::XML::Node * _node) const
{
	auto* saveToNode = _node->first_child_named(TXT("roomGenerationInfo"));
	if (!saveToNode)
	{
		saveToNode = _node->add_node(TXT("roomGenerationInfo"));
	}
	an_assert(saveToNode);
	{
		saveToNode->set_string_to_child(TXT("preferredTileSize"), PreferredTileSize::to_char(preferredTileSize));
		if (forceTileSize.is_set())
		{
			saveToNode->set_float_to_child(TXT("forceTileSize"), forceTileSize.get());
		}
		saveToNode->set_attribute(TXT("generationTags"), requestedGenerationTags.to_string(true));
	}

	return true;
}

Range2 RoomGenerationInfo::get_door_required_space(Transform const & _doorPlacementVRS, float _tileSize)
{
	Range2 result = Range2::empty;

	Vector3 helper(_tileSize * 0.5f, _tileSize, 0.0f);
	result.include(_doorPlacementVRS.location_to_world(helper * Vector3( 1.0f,  1.0f, 1.0f)).to_vector2());
	result.include(_doorPlacementVRS.location_to_world(helper * Vector3(-1.0f,  1.0f, 1.0f)).to_vector2());
	result.include(_doorPlacementVRS.location_to_world(helper * Vector3( 1.0f, -1.0f, 1.0f)).to_vector2());
	result.include(_doorPlacementVRS.location_to_world(helper * Vector3(-1.0f, -1.0f, 1.0f)).to_vector2());

	return result;
}

#define HAS_CHANGED(what) copy.what != what

bool RoomGenerationInfo::does_require_setup() const
{
	RoomGenerationInfo copy = *this;
	copy.setup_for_preview();
	return HAS_CHANGED(playAreaRectSize) ||
		   HAS_CHANGED(tileSize) ||
		   HAS_CHANGED(tileCount) ||
		   HAS_CHANGED(doorHeight) ||
		   HAS_CHANGED(allowCrouch) ||
		   HAS_CHANGED(allowAnyCrouch) ||
		   ! generationTags.is_same_as(copy.generationTags)
		   ;
}

RoomGenerationInfo::TileCountInfo RoomGenerationInfo::get_tile_count_info() const
{
	if (is_small()) return Tiny;
	if (min(tileCount.x, tileCount.y) >= 7) return VeryLarge;
	if (min(tileCount.x, tileCount.y) >= 5) return Large;
	if (max(tileCount.x, tileCount.y) >= 6) return Long;
	if (tileCount.x == 3 && tileCount.y == 3) return Basic;
	return Basic; // always as basic
	//return Normal;
}

tchar const* RoomGenerationInfo::get_tile_count_info_as_char() const
{
	RoomGenerationInfo::TileCountInfo tci = get_tile_count_info();
	if (tci == Tiny) return LOC_STR(NAME(lsTileCountTiny)).to_char();
	if (tci == Basic) return LOC_STR(NAME(lsTileCountBasic)).to_char();
	if (tci == Normal) return LOC_STR(NAME(lsTileCountNormal)).to_char();
	if (tci == Long) return LOC_STR(NAME(lsTileCountLong)).to_char();
	if (tci == Large) return LOC_STR(NAME(lsTileCountLarge)).to_char();
	if (tci == VeryLarge) return LOC_STR(NAME(lsTileCountVeryLarge)).to_char();
	return TXT("??");
}
