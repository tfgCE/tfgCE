#pragma once

#include "..\core\globalDefinitions.h"

#ifndef AN_TEA
//#error switch build ver to tea
#endif

// this is to switch name of the game from "Tea For God Emperor" to "Tea For God"
#define TEA_FOR_GOD

#ifdef TEA_FOR_GOD
#define TEA_FOR_GOD_NAME TXT("Tea For God")
#else
#define TEA_FOR_GOD_NAME TXT("Tea For God Emperor")
#endif

//#define WITH_OBJ
//#define WITH_VR

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#ifdef AN_DEVELOPMENT_OR_PROFILER
		//#define MEASURE_ADVANCE_SESSION
		#define MEASURE_WORLD_CREATION
		#define OUTPUT_WORLD_GENERATION_INFO
		#define OUTPUT_WORLD_GENERATION_INFO_LIST
		//#define OUTPUT_WORLD_GENERATION_INFO_DETAILED
		#define MEASURE_ON_DEATH_RELEASE
	#endif
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
	#define FPS_INFO_SHOW_SIMPLE
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#define OUTPUT_PERSISTENCE_VALUES
#endif

// for time being, allow even in final
#define USE_KEYBOARD_INPUT

//#define FOR_TSHIRTS

#define ARMOUR_DEFLECTION_ETC_INTO_ADDITIONAL_STATS

#define DEFAULT_SET_EMISSIVE_FROM_ENVIRONMENT true
#define DEFAULT_ALLOW_EMISSIVE_FROM_ENVIRONMENT true
#define DEFAULT_ZERO_EMISSIVE_FROM_ENVIRONMENT false

//#define SINGLE_GAME_SLOT

//#define NO_MUSIC
#define MUSIC_SINCE_START

//#define WITH_SLOW_DOWN_ON_LOW_HEALTH

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#define OUTPUT_STORY
#endif

#define MAX_PASSIVE_EXMS 4
#define MAX_POCKETS 6

//#define WITH_CRYSTALITE

//#define WITH_ENERGY_PULLING

//#define CHECK_BUILD_AVAILABLE

//#define HAND_GESTURES_IN_MAIN_MENU

#define DEFAULT_PLAYER_NAME TXT("player")

#ifdef AN_DEVELOPMENT_OR_PROFILER
#define FORCE_SAVING_ICON
#endif
#define WITH_SAVING_ICON

#ifdef AN_DEVELOPMENT_OR_PROFILER
#define AN_ALLOW_AUTO_TEST
#endif

//#define PLACE_INFO_ABOUT_CELL_IN_PILGRIM_OVERLAY_INFO__AT_ANY_WALL
//#define PLACE_INFO_ABOUT_CELL_IN_PILGRIM_OVERLAY_INFO__ALWAYS_IN_FRONT

#ifndef BUILD_PUBLIC_RELEASE
//#define WITH_DRAWING_BOARD
#endif

#define ALLOW_SENDING_DEV_NOTES_FROM_OPTIONS

//#define STORE_BUILD_ACKNOWLEDGMENTS_FOR_GAME_SLOTS

#define ALLOW_ALL_CHAPTERS_FOR_PREVIEW_BUILDS

//#define GUN_STATS__SHOW__NO_HIT_BONE_DAMAGE

// on the left in hud
//#define WITH_SHORT_EQUIPMENT_STATS

#define ALLOW_LOADING_TIPS_WITH_BE_AT_RIGHT_PLACE

// controls
#define CONTROLS_THRESHOLD 0.5f
#define BLOCK_RELEASING_TIME 2.5f

#define INVESTIGATE_GAME_SCRIPT

#ifndef BUILD_PUBLIC_RELEASE
	#ifdef INVESTIGATE_SOUND_SYSTEM
		#define SOUND_MASTERING
	#endif
#endif

//#define DEMO_ROBOT_SHADOW

//#define SHOW_INFO_ABOUT_CELL_WHEN_CHANGING_CELLS

#define ALL_GAME_MODES_AVAILABLE_SINCE_START

#define REMOTE_CONTROL_EFFECT_TIME 10.0f

#define WEAPON_NODE_KINETIC_ENERGY_IMPULSE_SHOW 10.0f

namespace TeaForGodEmperor
{
	void determine_demo();
	bool is_demo();
	void be_demo();
};
