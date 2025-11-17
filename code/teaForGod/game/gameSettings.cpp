#include "gameSettings.h"

#include "game.h"
#include "gameSettingsObserver.h"

#include "..\roomGenerators\roomGenerationInfo.h"

#include "..\..\core\concurrency\scopedSpinLock.h"
#include "..\..\core\memory\memory.h"
#include "..\..\core\utils.h"

#include "..\..\framework\ai\aiSocialRules.h"
#include "..\..\framework\debug\debugPanel.h"
#include "..\..\framework\display\display.h"
#include "..\..\framework\game\gameUtils.h"
#include "..\..\framework\game\gameMode.h"
#include "..\..\framework\debug\testConfig.h"

using namespace TeaForGodEmperor;

//

DEFINE_STATIC_NAME(dboRestart);
DEFINE_STATIC_NAME(dboLogPilgrim);

DEFINE_STATIC_NAME(dboRoomSize);
DEFINE_STATIC_NAME(dboPreferredTileSize);

DEFINE_STATIC_NAME(dboHandedness);
DEFINE_STATIC_NAME(dboCartsAutoMovement);
DEFINE_STATIC_NAME(dboCartsStopAndStay);
DEFINE_STATIC_NAME(dboCartsTopSpeed);
DEFINE_STATIC_NAME(dboCartsSpeedPct);
DEFINE_STATIC_NAME(dboDisplayTouchAsPress);

DEFINE_STATIC_NAME(dboHitIndicator);
DEFINE_STATIC_NAME(dboHealthIndicator);
DEFINE_STATIC_NAME(dboRealityAnchors);
DEFINE_STATIC_NAME(dboHighlightInteractives);

DEFINE_STATIC_NAME(dboPickupDistance);
DEFINE_STATIC_NAME(dboMainEquipmentCanBePutIntoPocket);
DEFINE_STATIC_NAME(dboGrabDistance);
DEFINE_STATIC_NAME(dboGrabRelease);
DEFINE_STATIC_NAME(dboEnergyAttractDistance);
DEFINE_STATIC_NAME(dboPickupFromPocketDistance);
DEFINE_STATIC_NAME(dboPocketDistance);


DEFINE_STATIC_NAME(dboPlayerImmortal);
DEFINE_STATIC_NAME(dboWeaponInfinite);
DEFINE_STATIC_NAME(dboArmourIneffective);
#ifdef WITH_CRYSTALITE
DEFINE_STATIC_NAME(dboWithCrystalite);
DEFINE_STATIC_NAME(dboInstantCrystalite);
#endif
DEFINE_STATIC_NAME(dboAutoMapOnly);
DEFINE_STATIC_NAME(dboNoTriangulationHelp);
DEFINE_STATIC_NAME(dboPathOnly);
DEFINE_STATIC_NAME(dboEnergyQuantumsStayTime);
DEFINE_STATIC_NAME(dboAIAggressiveness);
DEFINE_STATIC_NAME(dboNPCs);
DEFINE_STATIC_NAME(dboPlayerDamageGiven);
DEFINE_STATIC_NAME(dboPlayerDamageTaken);
DEFINE_STATIC_NAME(dboAIReactionTime);
DEFINE_STATIC_NAME(dboBlockSpawningInVisitedRoomsTime);
DEFINE_STATIC_NAME(dboLootHealth);
DEFINE_STATIC_NAME(dboLootAmmo);
DEFINE_STATIC_NAME(dboGenerationTagsLonger);

// generation tags
DEFINE_STATIC_NAME(testOne);
DEFINE_STATIC_NAME(onlyNew);
DEFINE_STATIC_NAME(shorter);

// test config settings
DEFINE_STATIC_NAME(testDifficultySettings);

// test config settings' tags
DEFINE_STATIC_NAME(playerImmortal);
DEFINE_STATIC_NAME(weaponInfinite);
DEFINE_STATIC_NAME(armourIneffective);
DEFINE_STATIC_NAME(armourEffectivenessLimit);
#ifdef WITH_CRYSTALITE
DEFINE_STATIC_NAME(withCrystalite);
DEFINE_STATIC_NAME(instantCrystalite);
#endif
DEFINE_STATIC_NAME(autoMapOnly);
DEFINE_STATIC_NAME(noTriangulationHelp);
DEFINE_STATIC_NAME(pathOnly);
DEFINE_STATIC_NAME(npcs);
DEFINE_STATIC_NAME(playerDamageGiven);
DEFINE_STATIC_NAME(playerDamageTaken);
DEFINE_STATIC_NAME(aiReactionTime);
DEFINE_STATIC_NAME(aiMean);
DEFINE_STATIC_NAME(aiSpeedBasedModifier);
DEFINE_STATIC_NAME(allowTelegraphOnSpawn);
DEFINE_STATIC_NAME(allowAnnouncePresence);
DEFINE_STATIC_NAME(forceMissChanceOnLowHealth);
DEFINE_STATIC_NAME(forceHitChanceOnFullHealth);
DEFINE_STATIC_NAME(blockSpawningInVisitedRoomsTime);
DEFINE_STATIC_NAME(spawnCountModifier);
DEFINE_STATIC_NAME(lootHealth);
DEFINE_STATIC_NAME(lootAmmo);
DEFINE_STATIC_NAME(healthOnContinue);

//

GameSettings* GameSettings::s_gameSettings = nullptr;

void GameSettings::create(Game* _game)
{
	an_assert(!s_gameSettings);
	s_gameSettings = new GameSettings(_game);

	s_gameSettings->create_debug_panel(_game);
}

void GameSettings::destroy()
{
	an_assert(s_gameSettings);
	delete_and_clear(s_gameSettings);
}

GameSettings::GameSettings(Game* _game)
:	game(_game)
{
	roomSizes.push_back(RoomSize(TXT("as is")));
	roomSizes.push_back(RoomSize(TXT("1.8 x 1.2"), Vector2(1.8f, 1.2f))); // smallest that make sense
	roomSizes.push_back(RoomSize(TXT("2.0 x 1.5"), Vector2(2.0f, 1.5f))); // vive minimal
	roomSizes.push_back(RoomSize(TXT("2.0 x 2.0"), Vector2(2.0f, 2.0f)));
	roomSizes.push_back(RoomSize(TXT("2.5 x 2.5"), Vector2(2.5f, 2.5f)));
	roomSizes.push_back(RoomSize(TXT("3.0 x 2.0"), Vector2(3.0f, 2.0f)));
	roomSizes.push_back(RoomSize(TXT("3.0 x 3.0"), Vector2(3.0f, 3.0f)));
	roomSizes.push_back(RoomSize(TXT("3.5 x 3.5"), Vector2(3.5f, 3.5f)));
	roomSizes.push_back(RoomSize(TXT("4.0 x 3.0"), Vector2(4.0f, 3.0f)));
	an_assert(observers.is_empty());
	delete_and_clear(debugPanel);

	Framework::AI::SocialRules::use_for_is_enemy_external([this](Framework::IAIObject const* _aiOwner, Framework::IAIObject const* _ai, bool _isEnemy)
	{
		if (this->difficulty.npcs <= NPCS::NonAggressive &&
			! Framework::GameUtils::is_controlled_by_player(fast_cast<Framework::IModulesOwner>(_aiOwner)))
		{
			return Optional<bool>(false);
		}
		return Optional<bool>();
	});
}

GameSettings::~GameSettings()
{
	Framework::AI::SocialRules::use_for_is_enemy_external(nullptr);
		
	an_assert(observers.is_empty());
	delete_and_clear(debugPanel);
}

void GameSettings::restart_required()
{
	if (restartOption)
	{
		restartOption->set_text(TXT("restart required"));
	}
	settings.restartRequired = true;
}

void GameSettings::restart_not_required()
{
	if (restartOption)
	{
		restartOption->set_text(TXT("restart"));
	}
	settings.restartRequired = false;
}

void GameSettings::create_debug_panel(Game* _game)
{
	an_assert(!debugPanel);

	if (!_game)
	{
		return;
	}

	debugPanel = new Framework::DebugPanel(_game);

	debugPanel->buttonSize = Vector2(300.0f, 120.0f);
	debugPanel->buttonSpacing = Vector2(20.0f, 10.0f);
	debugPanel->fontScale = 1.5f;

	{
		auto & option = debugPanel->add_option(NAME(dboRestart), TXT("restart"), Colour::red.mul_rgb(0.3f), Colour::yellow)
			.no_options()
			.set_on_press([](int _value) { Game::get_as<Game>()->end_mode(true); Game::get()->change_show_debug_panel(nullptr, false); Game::get()->change_show_debug_panel(nullptr, true); })
			.do_not_highlight()
			;
		restartOption = &option;
	}

	Colour generalColour = Colour::black;
	Colour playerConvenienceColour = Colour::orange.mul_rgb(0.3f);
	Colour optionsPreferncesColour = Colour::purple.mul_rgb(0.3f);
#ifdef AN_DEVELOPMENT
	Colour tweakingColours = Colour::c64Brown.mul_rgb(0.3f);
#endif
	Colour actualGameSettingsColour = Colour::blue.mul_rgb(0.3f);

	// general
	{
		auto & option = debugPanel->add_option(NAME(dboRoomSize), TXT("room size"), generalColour)
			.set_on_press([this](int _value) { settings.roomSize = _value;  RoomGenerationInfo::access().set_test_play_area_rect_size(roomSizes[settings.roomSize].size); inform_observers(); restart_required(); })
			.do_not_highlight()
			;
		for_every(rs, roomSizes)
		{
			option.add(rs->name.to_char());
		}
	}
	debugPanel->add_option(NAME(dboPreferredTileSize), TXT("preferred tiles"), generalColour)
		.add(TXT("auto"))
		.add(TXT("smallest"))
		.add(TXT("medium"))
		.add(TXT("largest"))
		.set_on_press([this](int _value) { RoomGenerationInfo::access().set_preferred_tile_size(PreferredTileSize::Type(_value)); inform_observers(); restart_required(); })
		.do_not_highlight()
		;

	// player convenience
	debugPanel->add_option(NAME(dboHandedness), TXT("handedness"), playerConvenienceColour)
		.add(TXT("any"))
		.add(TXT("right"))
		.add(TXT("left"))
		.set_on_press([this](int _value) { if (_value == 0) PlayerSetup::access_current().access_preferences().rightHanded.clear(); else PlayerSetup::access_current().access_preferences().rightHanded = _value == 2; inform_observers(); })
		.do_not_highlight()
		;

	/*
	debugPanel->add_option(NAME(dboCartsAutoMovement), TXT("cart movement"), playerConvenienceColour)
		.add(TXT("player input required"))
		.add(TXT("auto"))
		.set_on_press([this](int _value) { PlayerSetup::access_current().access_preferences().cartsAutoMovement = _value == 1; inform_observers(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboCartsStopAndStay), TXT("cart stop"), playerConvenienceColour)
		.add(TXT("keep moving"))
		.add(TXT("stop & stay"))
		.set_on_press([this](int _value) { PlayerSetup::access_current().access_preferences().cartsStopAndStay = _value == 1; inform_observers(); })
		.do_not_highlight()
		;
	*/

	debugPanel->add_option(NAME(dboCartsTopSpeed), TXT("cart top speed"), playerConvenienceColour)
		.add(TXT("unlimited"), 0)
		.add(TXT("very slow"), 75)
		.add(TXT("slow"), 150)
		.set_on_press([this](int _value) { PlayerSetup::access_current().access_preferences().cartsTopSpeed = (float)_value / 100.0f; inform_observers(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboCartsSpeedPct), TXT("speed"), playerConvenienceColour)
		.add(TXT("very slow"), 25)
		.add(TXT("slower"), 50)
		.add(TXT("slow"), 75)
		.add(TXT("normal"), 100)
		.set_on_press([this](int _value) { PlayerSetup::access_current().access_preferences().cartsSpeedPct = (float)_value / 100.0f; inform_observers(); })
		.do_not_highlight()
		;
	/*

	debugPanel->add_option(NAME(dboDisplayTouchAsPress), TXT("display touch as press"), playerConvenienceColour)
		.add(TXT("no"))
		.add(TXT("yes"))
		.set_on_press([this](int _value) { Framework::Display::allow_touch_as_press(_value != 0); })
		.do_not_highlight()
		;
	*/

	// options/preferences
	debugPanel->add_option(NAME(dboHitIndicator), TXT("hit indicator"), optionsPreferncesColour)
		.add(HitIndicatorType::to_char(HitIndicatorType::Off), HitIndicatorType::Off)
		.add(HitIndicatorType::to_char(HitIndicatorType::Full), HitIndicatorType::Full)
		.add(HitIndicatorType::to_char(HitIndicatorType::StrongerTowardsHitSource), HitIndicatorType::StrongerTowardsHitSource)
		.add(HitIndicatorType::to_char(HitIndicatorType::WeakerTowardsHitSource), HitIndicatorType::WeakerTowardsHitSource)
		.add(HitIndicatorType::to_char(HitIndicatorType::SidesOnly), HitIndicatorType::SidesOnly)
		.add(HitIndicatorType::to_char(HitIndicatorType::NotAtHitSource), HitIndicatorType::NotAtHitSource)
		.set_on_press([this](int _value) { PlayerSetup::access_current().access_preferences().hitIndicatorType = HitIndicatorType::Type(_value); inform_observers(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboHealthIndicator), TXT("health indicator"), optionsPreferncesColour)
		.add_off_on()
		.set_on_press([this](int _value) { PlayerSetup::access_current().access_preferences().healthIndicator = _value != 0; inform_observers(); })
		.do_not_highlight()
		;

	/*
	debugPanel->add_option(NAME(dboRealityAnchors), TXT("reality anchors"), optionsPreferncesColour)
		.add_off_on()
		.set_on_press([this](int _value) { settings.realityAnchors = _value != 0; inform_observers(); })
		.do_not_highlight()
		;
	*/

	debugPanel->add_option(NAME(dboHighlightInteractives), TXT("highlight interactives"), optionsPreferncesColour)
		.add_off_on()
		.set_on_press([this](int _value) { PlayerSetup::access_current().access_preferences().highlightInteractives = _value != 0; inform_observers(); })
		.do_not_highlight()
		;	

	debugPanel->add_option(NAME(dboMainEquipmentCanBePutIntoPocket), TXT("main equipment"), generalColour)
		.add(TXT("cannot be put into pocket"), 0)
		.add(TXT("can be put into pocket"), 1)
		.set_on_press([this](int _value) { settings.mainEquipmentCanBePutIntoPocket = _value != 0; inform_observers(); })
		.do_not_highlight()
		;
	
#ifdef AN_DEVELOPMENT
	// tweaking
	debugPanel->add_option(NAME(dboPickupDistance), TXT("pick up distance"), tweakingColours)
		.add(TXT("0.10"), 10)
		.add(TXT("0.15"), 15)
		.add(TXT("0.20"), 20)
		.add(TXT("0.25"), 25)
		.add(TXT("0.30"), 30)
		.add(TXT("0.40"), 40)
		.add(TXT("0.50"), 50)
		.add(TXT("1.00"), 100)
		.add(TXT("2.00"), 200)
		.add(TXT("5.00"), 500)
		.set_on_press([this](int _value) { settings.pickupDistance = (float)_value / 100.0f; inform_observers(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboGrabDistance), TXT("grab distance"), tweakingColours)
		.add(TXT("0.05"), 5)
		.add(TXT("0.10"), 10)
		.add(TXT("0.15"), 15)
		.add(TXT("0.20"), 20)
		.add(TXT("0.25"), 25)
		.add(TXT("0.30"), 30)
		.set_on_press([this](int _value) { settings.grabDistance = (float)_value / 100.0f; inform_observers(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboGrabRelease), TXT("grab release"), tweakingColours)
		.add(TXT("0.05"), 5)
		.add(TXT("0.10"), 10)
		.add(TXT("0.15"), 15)
		.add(TXT("0.20"), 20)
		.add(TXT("0.25"), 25)
		.add(TXT("0.30"), 30)
		.add(TXT("0.35"), 35)
		.add(TXT("0.40"), 40)
		.add(TXT("0.45"), 45)
		.add(TXT("0.50"), 50)
		.set_on_press([this](int _value) { settings.grabRelease = (float)_value / 100.0f; inform_observers(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboPickupFromPocketDistance), TXT("pickup from pocket distance"), tweakingColours)
		.add(TXT("0.10"), 10)
		.add(TXT("0.15"), 15)
		.add(TXT("0.20"), 20)
		.add(TXT("0.25"), 25)
		.add(TXT("0.30"), 30)
		.add(TXT("0.40"), 40)
		.add(TXT("0.50"), 50)
		.set_on_press([this](int _value) { settings.pickupFromPocketDistance = (float)_value / 100.0f; inform_observers(); })
		.do_not_highlight()
		;
	
	debugPanel->add_option(NAME(dboPocketDistance), TXT("pocket distance"), tweakingColours)
		.add(TXT("0.10"), 10)
		.add(TXT("0.15"), 15)
		.add(TXT("0.20"), 20)
		.add(TXT("0.25"), 25)
		.add(TXT("0.30"), 30)
		.add(TXT("0.40"), 40)
		.add(TXT("0.50"), 50)
		.set_on_press([this](int _value) { settings.pocketDistance = (float)_value / 100.0f; inform_observers(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboEnergyAttractDistance), TXT("energy attract distance"), tweakingColours)
		.add(TXT("0.25"), 25)
		.add(TXT("0.50"), 50)
		.add(TXT("0.75"), 75)
		.add(TXT("1.00"), 100)
		.add(TXT("1.25"), 125)
		.add(TXT("1.50"), 150)
		.add(TXT("1.75"), 175)
		.add(TXT("2.00"), 200)
		.set_on_press([this](int _value) { settings.energyAttractDistance = (float)_value / 100.0f; inform_observers(); })
		.do_not_highlight()
		;
#endif

	// actual game settings (that affect difficulty)
	debugPanel->add_option(NAME(dboPlayerImmortal), TXT("player"), actualGameSettingsColour)
		.add(TXT("immortal"), 1, Colour::cyan.mul_rgb(0.3f))
		.add(TXT("can die"), 0)
		.set_on_press([this](int _value) { difficulty.playerImmortal = _value == 1; inform_observers(); })
		.do_not_highlight()
		;

	// actual game settings (that affect difficulty)
	debugPanel->add_option(NAME(dboWeaponInfinite), TXT("weapon"), actualGameSettingsColour)
		.add(TXT("infinite"), 1, Colour::cyan.mul_rgb(0.3f))
		.add(TXT("normal"), 0)
		.set_on_press([this](int _value) { difficulty.weaponInfinite = _value == 1; inform_observers(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboArmourIneffective), TXT("armour"), actualGameSettingsColour)
		.add(TXT("ineffective"), 1, Colour::cyan.mul_rgb(0.3f))
		.add(TXT("normal"), 0)
		.set_on_press([this](int _value) { difficulty.armourIneffective = _value == 1; inform_observers(); })
		.do_not_highlight()
		;

#ifdef WITH_CRYSTALITE
	debugPanel->add_option(NAME(dboWithCrystalite), TXT("crystalite"), actualGameSettingsColour)
		.add(TXT("no"), 0)
		.add(TXT("yes"), 1)
		.set_on_press([this](int _value) { difficulty.withCrystalite = _value == 1; inform_observers(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboInstantCrystalite), TXT("crystalite"), actualGameSettingsColour)
		.add(TXT("instant"), 1, Colour::cyan.mul_rgb(0.3f))
		.add(TXT("to grab"), 0)
		.set_on_press([this](int _value) { difficulty.instantCrystalite = _value == 1; inform_observers(); })
		.do_not_highlight()
		;
#endif

	debugPanel->add_option(NAME(dboAutoMapOnly), TXT("map"), actualGameSettingsColour)
		.add(TXT("map+"), 0)
		.add(TXT("auto map only"), 1, Colour::cyan.mul_rgb(0.3f))
		.set_on_press([this](int _value) { difficulty.autoMapOnly = _value == 1; inform_observers(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboNoTriangulationHelp), TXT("triangulation"), actualGameSettingsColour)
		.add(TXT("assisted"), 0)
		.add(TXT("no help"), 1, Colour::cyan.mul_rgb(0.3f))
		.set_on_press([this](int _value) { difficulty.noTriangulationHelp = _value == 1; inform_observers(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboPathOnly), TXT("door directions"), actualGameSettingsColour)
		.add(TXT("all"), 0)
		.add(TXT("objective only"), 1, Colour::cyan.mul_rgb(0.3f))
		.set_on_press([this](int _value) { difficulty.pathOnly = _value == 1; inform_observers(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboNPCs), TXT("NPCs"), actualGameSettingsColour)
		.add(TXT("no NPCs"), NPCS::NoNPCs)
		.add(TXT("no hostiles"), NPCS::NoHostiles)
		.add(TXT("non aggressive"), NPCS::NonAggressive)
		.add(TXT("normal"), NPCS::Normal)
		.set_on_press([this](int _value) { difficulty.npcs= (NPCS::Type)_value; inform_observers(); restart_required(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboEnergyQuantumsStayTime), TXT("energy quantums stay time"), actualGameSettingsColour)
		.add(TXT("don't disappear"), 0)
		.add(TXT("2.00"), 200)
		.add(TXT("1.50"), 150)
		.add(TXT("1.00"), 100)
		.add(TXT("0.75"), 75)
		.add(TXT("0.50"), 50)
		.add(TXT("0.25"), 25)
		.set_on_press([this](int _value) { difficulty.energyQuantumsStayTime = (float)_value / 100.0f; inform_observers(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboAIAggressiveness), TXT("ai"), actualGameSettingsColour)
		.add(TXT("ignores"), AIAgressiveness::Ignores)
		.add(TXT("curious"), AIAgressiveness::Curious)
		.add(TXT("not friendy"), AIAgressiveness::NotFriendly)
		.add(TXT("aggressive"), AIAgressiveness::Aggressive)
		.add(TXT("very aggressive"), AIAgressiveness::VeryAggressive)
		.add(TXT("maniac"), AIAgressiveness::Maniac)
		.set_on_press([this](int _value) { difficulty.aiAggressiveness = (AIAgressiveness::Type)_value; inform_observers(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboPlayerDamageGiven), TXT("player dmg given"), actualGameSettingsColour)
		.add(TXT("x 3.00"), 300)
		.add(TXT("x 2.00"), 200)
		.add(TXT("x 1.50"), 150)
		.add(TXT("x 1.25"), 125)
		.add(TXT("x 1.00"), 100)
		.add(TXT("x 0.75"), 75)
		.add(TXT("x 0.50"), 50)
		.add(TXT("x 0.25"), 25)
		.set_on_press([this](int _value) { difficulty.playerDamageGiven = (float)_value / 100.0f; inform_observers(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboPlayerDamageTaken), TXT("player dmg taken"), actualGameSettingsColour)
		.add(TXT("x 0.25"), 25)
		.add(TXT("x 0.50"), 50)
		.add(TXT("x 0.75"), 75)
		.add(TXT("x 1.00"), 100)
		.add(TXT("x 1.25"), 125)
		.add(TXT("x 1.50"), 150)
		.add(TXT("x 2.00"), 200)
		.add(TXT("x 3.00"), 300)
		.set_on_press([this](int _value) { difficulty.playerDamageTaken = (float)_value / 100.0f; inform_observers(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboAIReactionTime), TXT("ai reaction time"), actualGameSettingsColour)
		.add(TXT("x 2.00"), 200)
		.add(TXT("x 1.50"), 150)
		.add(TXT("x 1.00"), 100)
		.add(TXT("x 0.75"), 75)
		.add(TXT("x 0.50"), 50)
		.add(TXT("x 0.25"), 25)
		.set_on_press([this](int _value) { difficulty.aiReactionTime = (float)_value / 100.0f; inform_observers(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboBlockSpawningInVisitedRoomsTime), TXT("block spawning in visited rooms"), actualGameSettingsColour)
		.add(TXT("10 minutes"), 600)
		.add(TXT("8 minutes"), 480)
		.add(TXT("6 minutes"), 360)
		.add(TXT("5 minutes"), 300)
		.add(TXT("4 minutes"), 240)
		.add(TXT("3 minutes"), 180)
		.add(TXT("2 minutes"), 120)
		.add(TXT("1 minute"), 60)
		.add(TXT("don't"), 0)
		.set_on_press([this](int _value) { difficulty.blockSpawningInVisitedRoomsTime = (float)_value; inform_observers(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboLootHealth), TXT("loot health"), actualGameSettingsColour)
		.add(TXT("x 2.00"), 200)
		.add(TXT("x 1.50"), 150)
		.add(TXT("x 1.25"), 125)
		.add(TXT("x 1.00"), 100)
		.add(TXT("x 0.75"), 75)
		.add(TXT("x 0.50"), 50)
		.add(TXT("x 0.25"), 25)
		.set_on_press([this](int _value) { difficulty.lootHealth = (float)_value / 100.0f; inform_observers(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboLootAmmo), TXT("loot ammo"), actualGameSettingsColour)
		.add(TXT("x 2.00"), 200)
		.add(TXT("x 1.50"), 150)
		.add(TXT("x 1.25"), 125)
		.add(TXT("x 1.00"), 100)
		.add(TXT("x 0.75"), 75)
		.add(TXT("x 0.50"), 50)
		.add(TXT("x 0.25"), 25)
		.set_on_press([this](int _value) { difficulty.lootAmmo = (float)_value / 100.0f; inform_observers(); })
		.do_not_highlight()
		;

	debugPanel->add_option(NAME(dboGenerationTagsLonger), TXT("gen_longer"), actualGameSettingsColour)
		.add(TXT("shorter"), 0)
		.add(TXT("longer"), 1)
		.add(TXT("test one"), 2)
		.add(TXT("only new"), 3)
		.set_on_press([this](int _value) {
			RoomGenerationInfo::access().access_requested_generation_tags().remove_tag(NAME(shorter));
			RoomGenerationInfo::access().access_requested_generation_tags().remove_tag(NAME(testOne));
			RoomGenerationInfo::access().access_requested_generation_tags().remove_tag(NAME(onlyNew));
			if (_value == 0) RoomGenerationInfo::access().access_requested_generation_tags().set_tag(NAME(shorter));
			if (_value == 2) RoomGenerationInfo::access().access_requested_generation_tags().set_tag(NAME(testOne));
			if (_value == 3) RoomGenerationInfo::access().access_requested_generation_tags().set_tag(NAME(onlyNew));
			inform_observers(); restart_required(); })
		.do_not_highlight()
		;
	
	{
		debugPanel->add_option(NAME(dboLogPilgrim), TXT("log pilgrimage info"), Colour::white, Colour::black)
			.no_options()
			.set_on_press([](int _value)
			{
				if (auto* gm = Game::get_as<Game>()->get_mode())
				{
					LogInfoContext log;
					log.log(TXT("PILGRIMAGE INFO REQUESTED"));
					{
						LOG_INDENT(log);
						gm->log(log);
					}
					log.output_to_output();
				}
			})
			.do_not_highlight()
			;
	}
}

Framework::DebugPanel* GameSettings::update_and_get_debug_panel()
{
	if (!debugPanel)
	{
		return nullptr;
	}

	debugPanel->set_option(NAME(dboRoomSize), settings.roomSize);
	debugPanel->set_option(NAME(dboPreferredTileSize), RoomGenerationInfo::get().get_preferred_tile_size());

	debugPanel->set_option(NAME(dboCartsAutoMovement), PlayerSetup::get_current().get_preferences().cartsAutoMovement);
	debugPanel->set_option(NAME(dboCartsStopAndStay), PlayerSetup::get_current().get_preferences().cartsStopAndStay);
	debugPanel->set_option(NAME(dboCartsTopSpeed), (int)(PlayerSetup::get_current().get_preferences().cartsTopSpeed * 100.0f));
	debugPanel->set_option(NAME(dboCartsSpeedPct), (int)(PlayerSetup::get_current().get_preferences().cartsSpeedPct * 100.0f));
	debugPanel->set_option(NAME(dboDisplayTouchAsPress), (int)(Framework::Display::should_allow_touch_as_press()? 1 : 0));

	debugPanel->set_option(NAME(dboHitIndicator), PlayerSetup::get_current().get_preferences().hitIndicatorType);
	debugPanel->set_option(NAME(dboHealthIndicator), PlayerSetup::get_current().get_preferences().healthIndicator);
	debugPanel->set_option(NAME(dboRealityAnchors), settings.realityAnchors);
	debugPanel->set_option(NAME(dboHighlightInteractives), PlayerSetup::get_current().get_preferences().highlightInteractives);

	debugPanel->set_option(NAME(dboPickupDistance), (int)(settings.pickupDistance * 100.0f));
	debugPanel->set_option(NAME(dboMainEquipmentCanBePutIntoPocket), settings.mainEquipmentCanBePutIntoPocket? 1 : 0);
	debugPanel->set_option(NAME(dboGrabDistance), (int)(settings.grabDistance * 100.0f));
	debugPanel->set_option(NAME(dboGrabRelease), (int)(settings.grabRelease * 100.0f));
	debugPanel->set_option(NAME(dboEnergyAttractDistance), (int)(settings.energyAttractDistance * 100.0f));
	debugPanel->set_option(NAME(dboPickupFromPocketDistance), (int)(settings.pickupFromPocketDistance * 100.0f));
	debugPanel->set_option(NAME(dboPocketDistance), (int)(settings.pocketDistance * 100.0f));

	debugPanel->set_option(NAME(dboPlayerImmortal), difficulty.playerImmortal ? 1 : 0);
	debugPanel->set_option(NAME(dboWeaponInfinite), difficulty.weaponInfinite ? 1 : 0);
	debugPanel->set_option(NAME(dboArmourIneffective), difficulty.armourIneffective ? 1 : 0);
#ifdef WITH_CRYSTALITE
	debugPanel->set_option(NAME(dboWithCrystalite), difficulty.withCrystalite ? 1 : 0);
	debugPanel->set_option(NAME(dboInstantCrystalite), difficulty.instantCrystalite ? 1 : 0);
#endif
	debugPanel->set_option(NAME(dboAutoMapOnly), difficulty.autoMapOnly ? 1 : 0);
	debugPanel->set_option(NAME(dboNoTriangulationHelp), difficulty.noTriangulationHelp ? 1 : 0);
	debugPanel->set_option(NAME(dboPathOnly), difficulty.pathOnly ? 1 : 0);
	debugPanel->set_option(NAME(dboNPCs), difficulty.npcs);
	debugPanel->set_option(NAME(dboEnergyQuantumsStayTime), (int)(difficulty.energyQuantumsStayTime * 100.0f));
	debugPanel->set_option(NAME(dboAIAggressiveness), difficulty.aiAggressiveness);
	debugPanel->set_option(NAME(dboPlayerDamageGiven), (int)(difficulty.playerDamageGiven * 100.0f));
	debugPanel->set_option(NAME(dboPlayerDamageTaken), (int)(difficulty.playerDamageTaken * 100.0f));
	debugPanel->set_option(NAME(dboAIReactionTime), (int)(difficulty.aiReactionTime * 100.0f));
	debugPanel->set_option(NAME(dboBlockSpawningInVisitedRoomsTime), (int)(difficulty.blockSpawningInVisitedRoomsTime));
	debugPanel->set_option(NAME(dboLootHealth), (int)(difficulty.lootHealth * 100.0f));
	debugPanel->set_option(NAME(dboLootAmmo), (int)(difficulty.lootAmmo * 100.0f));
	debugPanel->set_option(NAME(dboGenerationTagsLonger), RoomGenerationInfo::access().access_requested_generation_tags().has_tag(NAME(onlyNew))? 3 :
														 (RoomGenerationInfo::access().access_requested_generation_tags().has_tag(NAME(testOne))? 2
													   : (RoomGenerationInfo::access().access_requested_generation_tags().has_tag(NAME(shorter))? 0 : 1)));

	return debugPanel;
}

void GameSettings::add_observer(GameSettingsObserver* _observer)
{
	Concurrency::ScopedSpinLock lock(observersLock);

	observers.push_back_unique(_observer);
}

void GameSettings::remove_observer(GameSettingsObserver* _observer)
{
	Concurrency::ScopedSpinLock lock(observersLock);

	observers.remove_fast(_observer);
}

void GameSettings::inform_observers()
{
	Concurrency::ScopedSpinLock lock(observersLock);

	for_every_ptr(observer, observers)
	{
		observer->on_game_settings_channged(*this);
	}
}

void GameSettings::setup_variables(SimpleVariableStorage & _storage)
{
}

void GameSettings::load_test_difficulty_settings()
{
	if (auto* param = Framework::TestConfig::get_params().get_existing<String>(NAME(testDifficultySettings)))
	{
		Tags testDifficultySettings = Tags();
		testDifficultySettings.load_from_string(*param);

		difficulty.set_from_tags(testDifficultySettings);
	}
}

//

template <typename EnumStruct, typename EnumStructType>
static EnumStructType parse_using_enum_MAX(String const & _value, EnumStructType _default)
{
	for_count(int, i, EnumStructType::MAX)
	{
		if (_value == EnumStruct::to_char((EnumStructType)i))
		{
			return (EnumStructType)i;
		}
	}
	return _default;
}

#define SAVE_BOOL(name) \
	_node->set_bool_attribute(TXT(#name), name);

#define SAVE_FLOAT(name) \
	_node->set_float_attribute(TXT(#name), name);

#define SAVE_INT(name) \
	_node->set_int_attribute(TXT(#name), name);

#define SAVE_ENUM(type, name) \
	_node->set_attribute(TXT(#name), String(type::to_char(name)));

#define LOAD_BOOL(name) \
	name = _node->get_bool_attribute(TXT(#name), name);

#define LOAD_FLOAT(name) \
	name = _node->get_float_attribute(TXT(#name), name);

#define LOAD_INT(name) \
	name = _node->get_int_attribute(TXT(#name), name);

#define LOAD_ENUM(type, name) \
	name = parse_using_enum_MAX<type, type::Type>(_node->get_string_attribute(TXT(#name)), name);

bool GameSettings::Difficulty::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	SAVE_BOOL(playerImmortal);
	SAVE_BOOL(weaponInfinite);
	SAVE_BOOL(armourIneffective);
	SAVE_FLOAT(armourEffectivenessLimit);
#ifdef WITH_CRYSTALITE
	SAVE_BOOL(withCrystalite);
	SAVE_BOOL(instantCrystalite);
#endif
	SAVE_BOOL(autoMapOnly);
	SAVE_BOOL(noTriangulationHelp);
	SAVE_BOOL(pathOnly);
	SAVE_ENUM(NPCS, npcs);
	SAVE_FLOAT(energyQuantumsStayTime);
	SAVE_ENUM(AIAgressiveness, aiAggressiveness);
	SAVE_FLOAT(playerDamageGiven);
	SAVE_FLOAT(playerDamageTaken);
	SAVE_FLOAT(aiReactionTime);
	SAVE_FLOAT(aiMean);
	SAVE_FLOAT(aiSpeedBasedModifier);
	SAVE_FLOAT(allowTelegraphOnSpawn);
	SAVE_FLOAT(allowAnnouncePresence);
	SAVE_FLOAT(forceMissChanceOnLowHealth);
	SAVE_FLOAT(forceHitChanceOnFullHealth);
	SAVE_FLOAT(blockSpawningInVisitedRoomsTime);
	SAVE_FLOAT(spawnCountModifier);
	SAVE_FLOAT(lootHealth);
	SAVE_FLOAT(lootAmmo);
	//
	SAVE_BOOL(commonHandEnergyStorage);
	SAVE_BOOL(regenerateEnergy);
	SAVE_BOOL(simplerMazes);
	SAVE_BOOL(linearLevels);
	SAVE_BOOL(simplerPuzzles);
	SAVE_BOOL(showDirectionsOnlyToObjective);
	SAVE_BOOL(showDirectionsOnlyToNonDepleted);
	SAVE_BOOL(showLineModelAlwaysOtherwiseOnMap);
	SAVE_FLOAT(energyCoilsAmount);
	SAVE_INT(passiveEXMSlots);
	SAVE_BOOL(allowPermanentEXMsAtUpgradeMachine);
	SAVE_BOOL(allowEXMReinstalling);
	SAVE_BOOL(obfuscateSymbols);
	SAVE_BOOL(displayMapOnUpgrade);
	SAVE_BOOL(revealDevicesOnUpgrade);
	SAVE_ENUM(RestartMode, restartMode);
	SAVE_BOOL(respawnAndContinueOnlyIfEnoughEnergy);
	SAVE_BOOL(mapAlwaysAvailable);
	SAVE_INT(healthOnContinue);
	SAVE_BOOL(noOverlayInfo);

	return result;
}

bool GameSettings::Difficulty::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	LOAD_BOOL(playerImmortal);
	LOAD_BOOL(weaponInfinite);
	LOAD_BOOL(armourIneffective);
	LOAD_FLOAT(armourEffectivenessLimit);
#ifdef WITH_CRYSTALITE
	LOAD_BOOL(withCrystalite);
	LOAD_BOOL(instantCrystalite);
#endif
	LOAD_BOOL(autoMapOnly);
	LOAD_BOOL(noTriangulationHelp);
	LOAD_BOOL(pathOnly);
	LOAD_ENUM(NPCS, npcs);
	LOAD_FLOAT(energyQuantumsStayTime);
	LOAD_ENUM(AIAgressiveness, aiAggressiveness);
	LOAD_FLOAT(playerDamageGiven);
	LOAD_FLOAT(playerDamageTaken);
	LOAD_FLOAT(aiReactionTime);
	LOAD_FLOAT(aiMean);
	LOAD_FLOAT(aiSpeedBasedModifier);
	LOAD_FLOAT(allowTelegraphOnSpawn);
	LOAD_FLOAT(allowAnnouncePresence);
	LOAD_FLOAT(forceMissChanceOnLowHealth);
	LOAD_FLOAT(forceHitChanceOnFullHealth);
	LOAD_FLOAT(blockSpawningInVisitedRoomsTime);
	LOAD_FLOAT(spawnCountModifier);
	LOAD_FLOAT(lootHealth);
	LOAD_FLOAT(lootAmmo);
	//
	LOAD_BOOL(commonHandEnergyStorage);
	LOAD_BOOL(regenerateEnergy);
	LOAD_BOOL(simplerMazes);
	LOAD_BOOL(linearLevels);
	LOAD_BOOL(simplerPuzzles);
	LOAD_BOOL(showDirectionsOnlyToObjective);
	LOAD_BOOL(showDirectionsOnlyToNonDepleted);
	LOAD_BOOL(showLineModelAlwaysOtherwiseOnMap);
	LOAD_FLOAT(energyCoilsAmount);
	LOAD_INT(passiveEXMSlots);
	LOAD_BOOL(allowPermanentEXMsAtUpgradeMachine);
	LOAD_BOOL(allowEXMReinstalling);
	LOAD_BOOL(obfuscateSymbols);
	LOAD_BOOL(displayMapOnUpgrade);
	LOAD_BOOL(revealDevicesOnUpgrade);
	LOAD_ENUM(RestartMode, restartMode);
	LOAD_BOOL(respawnAndContinueOnlyIfEnoughEnergy);
	LOAD_BOOL(mapAlwaysAvailable);
	LOAD_INT(healthOnContinue);
	LOAD_BOOL(noOverlayInfo);

	return result;
}

#define READ_FROM_TAG_BOOL(difficultySetting) if (_tags.has_tag(NAME(difficultySetting))) difficultySetting = _tags.get_tag_as_int(NAME(difficultySetting)) != 0;
#define READ_FROM_TAG_FLOAT(difficultySetting) if (_tags.has_tag(NAME(difficultySetting))) difficultySetting = _tags.get_tag(NAME(difficultySetting));
#define READ_FROM_TAG_AS(difficultySetting, asType) if (_tags.has_tag(NAME(difficultySetting))) difficultySetting = (asType)_tags.get_tag_as_int(NAME(difficultySetting));

void GameSettings::Difficulty::set_from_tags(Tags const& _tags)
{
	READ_FROM_TAG_BOOL(playerImmortal);
	READ_FROM_TAG_BOOL(weaponInfinite);
	READ_FROM_TAG_BOOL(armourIneffective);
	READ_FROM_TAG_FLOAT(armourEffectivenessLimit);
#ifdef WITH_CRYSTALITE
	READ_FROM_TAG_BOOL(withCrystalite);
	READ_FROM_TAG_BOOL(instantCrystalite);
#endif
	READ_FROM_TAG_BOOL(autoMapOnly);
	READ_FROM_TAG_BOOL(noTriangulationHelp);
	READ_FROM_TAG_BOOL(pathOnly);
	READ_FROM_TAG_AS(npcs, TeaForGodEmperor::GameSettings::NPCS::Type);
	READ_FROM_TAG_FLOAT(playerDamageGiven);
	READ_FROM_TAG_FLOAT(playerDamageTaken);
	READ_FROM_TAG_FLOAT(aiReactionTime);
	READ_FROM_TAG_FLOAT(aiMean);
	READ_FROM_TAG_FLOAT(aiSpeedBasedModifier);
	READ_FROM_TAG_FLOAT(allowTelegraphOnSpawn);
	READ_FROM_TAG_FLOAT(allowAnnouncePresence);
	READ_FROM_TAG_FLOAT(forceMissChanceOnLowHealth);
	READ_FROM_TAG_FLOAT(forceHitChanceOnFullHealth);
	READ_FROM_TAG_FLOAT(blockSpawningInVisitedRoomsTime);
	READ_FROM_TAG_FLOAT(spawnCountModifier);
	READ_FROM_TAG_FLOAT(lootHealth);
	READ_FROM_TAG_FLOAT(lootAmmo);
	
	// sanitise
	if (playerDamageGiven == 0.0f) playerDamageGiven = 1.0f;
	if (playerDamageTaken == 0.0f) playerDamageTaken = 1.0f;
	if (aiReactionTime == 0.0f) aiReactionTime = 1.0f;
	if (blockSpawningInVisitedRoomsTime == 0.0f) blockSpawningInVisitedRoomsTime = 240.0f;
	if (spawnCountModifier == 0.0f) spawnCountModifier = 1.0f;
	if (lootHealth == 0.0f) lootHealth = 1.0f;
	if (lootAmmo == 0.0f) lootAmmo = 1.0f;
	if (energyCoilsAmount == 0.0f) energyCoilsAmount = 1.0f;
	if (passiveEXMSlots == 0) passiveEXMSlots = 1;
}

void GameSettings::Difficulty::store_as_tags(Tags & _tags) const
{
	_tags.set_tag(NAME(playerImmortal), playerImmortal ? 1 : 0);
	_tags.set_tag(NAME(weaponInfinite), weaponInfinite ? 1 : 0);
	_tags.set_tag(NAME(armourIneffective), armourIneffective ? 1 : 0);
	_tags.set_tag(NAME(armourEffectivenessLimit), armourEffectivenessLimit);
#ifdef WITH_CRYSTALITE
	_tags.set_tag(NAME(withCrystalite), withCrystalite ? 1 : 0);
	_tags.set_tag(NAME(instantCrystalite), instantCrystalite ? 1 : 0);
#endif
	_tags.set_tag(NAME(autoMapOnly), autoMapOnly ? 1 : 0);
	_tags.set_tag(NAME(noTriangulationHelp), noTriangulationHelp ? 1 : 0);
	_tags.set_tag(NAME(pathOnly), pathOnly ? 1 : 0);
	_tags.set_tag(NAME(npcs), npcs);
	_tags.set_tag(NAME(playerDamageGiven), playerDamageGiven);
	_tags.set_tag(NAME(playerDamageTaken), playerDamageTaken);
	_tags.set_tag(NAME(aiReactionTime), aiReactionTime);
	_tags.set_tag(NAME(aiMean), aiMean);
	_tags.set_tag(NAME(aiSpeedBasedModifier), aiSpeedBasedModifier);
	_tags.set_tag(NAME(allowTelegraphOnSpawn), allowTelegraphOnSpawn);
	_tags.set_tag(NAME(allowAnnouncePresence), allowAnnouncePresence);
	_tags.set_tag(NAME(forceMissChanceOnLowHealth), forceMissChanceOnLowHealth);
	_tags.set_tag(NAME(forceHitChanceOnFullHealth), forceHitChanceOnFullHealth);
	_tags.set_tag(NAME(blockSpawningInVisitedRoomsTime), blockSpawningInVisitedRoomsTime);
	_tags.set_tag(NAME(spawnCountModifier), spawnCountModifier);
	_tags.set_tag(NAME(lootHealth), lootHealth);
	_tags.set_tag(NAME(lootAmmo), lootAmmo);
}

//

#define APPLY_INCOMPLETE(what) \
	if (what.is_set()) _difficulty.what = what.get();

#define APPLY_INCOMPLETE_ADD(what) \
	if (what.is_set()) _difficulty.what += what.get();

void GameSettings::IncompleteDifficulty::apply_to(Difficulty& _difficulty) const
{
	APPLY_INCOMPLETE(playerImmortal);
	APPLY_INCOMPLETE(weaponInfinite);
	APPLY_INCOMPLETE(armourIneffective);
	APPLY_INCOMPLETE(armourEffectivenessLimit);
#ifdef WITH_CRYSTALITE
	APPLY_INCOMPLETE(withCrystalite);
	APPLY_INCOMPLETE(instantCrystalite);
#endif
	APPLY_INCOMPLETE(autoMapOnly);
	APPLY_INCOMPLETE(noTriangulationHelp);
	APPLY_INCOMPLETE(pathOnly);
	APPLY_INCOMPLETE(npcs);
	APPLY_INCOMPLETE(energyQuantumsStayTime);
	APPLY_INCOMPLETE(aiAggressiveness);
	APPLY_INCOMPLETE(playerDamageGiven);
	APPLY_INCOMPLETE(playerDamageTaken);
	APPLY_INCOMPLETE(aiReactionTime);
	APPLY_INCOMPLETE(aiMean);
	APPLY_INCOMPLETE_ADD(aiSpeedBasedModifier);
	APPLY_INCOMPLETE(allowTelegraphOnSpawn);
	APPLY_INCOMPLETE(allowAnnouncePresence);
	APPLY_INCOMPLETE(forceMissChanceOnLowHealth);
	APPLY_INCOMPLETE(forceHitChanceOnFullHealth);
	APPLY_INCOMPLETE(blockSpawningInVisitedRoomsTime);
	APPLY_INCOMPLETE(spawnCountModifier);
	APPLY_INCOMPLETE(lootAmmo);
	APPLY_INCOMPLETE(lootHealth);
	APPLY_INCOMPLETE(restartMode);
}

#define SAVE_INCOMPLETE_BOOL(name) \
	if (name.is_set()) _node->set_bool_attribute(TXT(#name), name.get());

#define SAVE_INCOMPLETE_FLOAT(name) \
	if (name.is_set()) _node->set_float_attribute(TXT(#name), name.get());

#define SAVE_INCOMPLETE_ENUM(type, name) \
	if (name.is_set()) _node->set_attribute(TXT(#name), String(type::to_char(name.get())));

#define LOAD_INCOMPLETE_BOOL(name) \
	name.load_from_xml(_node, TXT(#name));

#define LOAD_INCOMPLETE_FLOAT(name) \
	name.load_from_xml(_node, TXT(#name));

#define LOAD_INCOMPLETE_ENUM(type, name) \
	if (_node->has_attribute(TXT(#name))) \
		name = parse_using_enum_MAX<type, type::Type>(_node->get_string_attribute(TXT(#name)), (type::Type)0);

bool GameSettings::IncompleteDifficulty::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	SAVE_INCOMPLETE_BOOL(playerImmortal);
	SAVE_INCOMPLETE_BOOL(weaponInfinite);
	SAVE_INCOMPLETE_BOOL(armourIneffective);
	SAVE_INCOMPLETE_FLOAT(armourEffectivenessLimit);
#ifdef WITH_CRYSTALITE
	SAVE_INCOMPLETE_BOOL(withCrystalite);
	SAVE_INCOMPLETE_BOOL(instantCrystalite);
#endif
	SAVE_INCOMPLETE_BOOL(autoMapOnly);
	SAVE_INCOMPLETE_BOOL(noTriangulationHelp);
	SAVE_INCOMPLETE_BOOL(pathOnly);
	SAVE_INCOMPLETE_ENUM(NPCS, npcs);
	SAVE_INCOMPLETE_FLOAT(energyQuantumsStayTime);
	SAVE_INCOMPLETE_ENUM(AIAgressiveness, aiAggressiveness);
	SAVE_INCOMPLETE_FLOAT(playerDamageGiven);
	SAVE_INCOMPLETE_FLOAT(playerDamageTaken);
	SAVE_INCOMPLETE_FLOAT(aiReactionTime);
	SAVE_INCOMPLETE_FLOAT(aiMean);
	SAVE_INCOMPLETE_FLOAT(aiSpeedBasedModifier);
	SAVE_INCOMPLETE_FLOAT(allowTelegraphOnSpawn);
	SAVE_INCOMPLETE_FLOAT(allowAnnouncePresence);
	SAVE_INCOMPLETE_FLOAT(forceMissChanceOnLowHealth);
	SAVE_INCOMPLETE_FLOAT(forceHitChanceOnFullHealth);
	SAVE_INCOMPLETE_FLOAT(blockSpawningInVisitedRoomsTime);
	SAVE_INCOMPLETE_FLOAT(spawnCountModifier);
	SAVE_INCOMPLETE_FLOAT(lootAmmo);
	SAVE_INCOMPLETE_FLOAT(lootHealth);
	SAVE_INCOMPLETE_ENUM(RestartMode, restartMode);

	return result;
}

bool GameSettings::IncompleteDifficulty::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	LOAD_INCOMPLETE_BOOL(playerImmortal);
	LOAD_INCOMPLETE_BOOL(weaponInfinite);
	LOAD_INCOMPLETE_BOOL(armourIneffective);
	LOAD_INCOMPLETE_FLOAT(armourEffectivenessLimit);
#ifdef WITH_CRYSTALITE
	LOAD_INCOMPLETE_BOOL(withCrystalite);
	LOAD_INCOMPLETE_BOOL(instantCrystalite);
#endif
	LOAD_INCOMPLETE_BOOL(autoMapOnly);
	LOAD_INCOMPLETE_BOOL(noTriangulationHelp);
	LOAD_INCOMPLETE_BOOL(pathOnly);
	LOAD_INCOMPLETE_ENUM(NPCS, npcs);
	LOAD_INCOMPLETE_FLOAT(energyQuantumsStayTime);
	LOAD_INCOMPLETE_ENUM(AIAgressiveness, aiAggressiveness);
	LOAD_INCOMPLETE_FLOAT(playerDamageGiven);
	LOAD_INCOMPLETE_FLOAT(playerDamageTaken);
	LOAD_INCOMPLETE_FLOAT(aiReactionTime);
	LOAD_INCOMPLETE_FLOAT(aiMean);
	LOAD_INCOMPLETE_FLOAT(aiSpeedBasedModifier);
	LOAD_INCOMPLETE_FLOAT(allowTelegraphOnSpawn);
	LOAD_INCOMPLETE_FLOAT(allowAnnouncePresence);
	LOAD_INCOMPLETE_FLOAT(forceMissChanceOnLowHealth);
	LOAD_INCOMPLETE_FLOAT(forceHitChanceOnFullHealth);
	LOAD_INCOMPLETE_FLOAT(blockSpawningInVisitedRoomsTime);
	LOAD_INCOMPLETE_FLOAT(spawnCountModifier);
	LOAD_INCOMPLETE_FLOAT(lootAmmo);
	LOAD_INCOMPLETE_FLOAT(lootHealth);
	LOAD_INCOMPLETE_ENUM(RestartMode, restartMode);

	return result;
}

//

bool DifficultySettings::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	result &= difficulty.load_from_xml(_node);

	return result;
}

void DifficultySettings::save_to_xml(IO::XML::Node* _node, bool _saveLocalisedStrings) const
{
	difficulty.save_to_xml(_node);
}

