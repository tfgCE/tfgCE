#pragma once

#include "..\..\framework\library\library.h"

#include "customUpgradeType.h"
#include "damageRules.h"
#include "defaultPilgrimSetupPreset.h"
#include "exmType.h"
#include "extraUpgradeOption.h"
#include "gameDefinition.h"
#include "gameModifier.h"
#include "gameSubDefinition.h"
#include "lineModel.h"
#include "lootSelector.h"
#include "messageSet.h"
#include "missionBiome.h"
#include "missionDefinition.h"
#include "missionDifficultyModifier.h"
#include "missionGeneralProgress.h"
#include "missionObjectivesAndIntel.h"
#include "missionSelectionTier.h"
#include "missionsDefinition.h"
#include "projectileSelector.h"
#include "weaponCoreModifiers.h"
#include "weaponPartType.h"
#include "..\pilgrimage\pilgrimage.h"
#include "..\pilgrimage\pilgrimageDevice.h"
#include "..\story\storyScene.h"
#include "..\story\storyChapter.h"
#include "..\tutorials\tutorial.h"
#include "..\tutorials\tutorialStructure.h"

namespace TeaForGodEmperor
{
	namespace TeaForGodEmperorLibraryPrepareLevel
	{
		enum Type
		{
			StartLevel			= Framework::LibraryPrepareLevel::LevelCount,
				Resolve			= Framework::LibraryPrepareLevel::LevelCount,
			PreparePersistence	= StartLevel + 100,
		};
	};

	class Library
	: public Framework::Library
	{
		typedef Framework::Library base;
	protected:
		override_ void initialise_stores();
		override_ bool load_gameplay(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc, String const & _fileName, OUT_ bool & _loadResult);
		override_ void before_reload();
		override_ bool unload(Framework::LibraryLoadLevel::Type _libraryLoadLevel);
		override_ bool does_allow_loading(Framework::LibraryStoreBase* _store, Framework::LibraryLoadingContext& _lc) const;

		// stores
		LIBRARY_STORE(DamageRuleSet, damageRuleSets, damage_rule_sets);
		LIBRARY_STORE(DefaultPilgrimSetupPreset, defaultPilgrimSetupPresets, default_pilgrim_setup_presets);
		LIBRARY_STORE(Pilgrimage, pilgrimages, pilgrimages);
		LIBRARY_STORE(PilgrimageDevice, pilgrimageDevices, pilgrimage_devices);
		LIBRARY_STORE(CustomUpgradeType, customUpgradeTypes, custom_upgrade_types);
		LIBRARY_STORE(EXMType, exmTypes, exm_types);
		LIBRARY_STORE(ExtraUpgradeOption, extraUpgradeOptions, extra_upgrade_options);
		LIBRARY_STORE(LineModel, lineModels, line_models);
		LIBRARY_STORE(WeaponCoreModifiers, weaponCoreModifiers, weapon_core_modifiers);
		LIBRARY_STORE(WeaponPartType, weaponPartTypes, weapon_part_types);
		LIBRARY_STORE(MessageSet, messageSets, message_sets);
		LIBRARY_STORE(MissionBiome, missionBiomes, mission_biomes);
		LIBRARY_STORE(MissionDefinition, missionDefinitions, mission_definitions);
		LIBRARY_STORE(MissionDifficultyModifier, missionDifficultyModifiers, mission_dificulty_modifiers);
		LIBRARY_STORE(MissionGeneralProgress, missionGeneralProgress, mission_general_progress);
		LIBRARY_STORE(MissionObjectivesAndIntel, missionObjectivesAndIntel, mission_objectives_and_intel);
		LIBRARY_STORE(MissionSelectionTier, missionSelectionTiers, mission_selection_tiers);
		LIBRARY_STORE(MissionsDefinition, missionsDefinitions, missions_definitions);
		LIBRARY_STORE(GameDefinition, gameDefinitions, game_definitions);
		LIBRARY_STORE(GameModifier, gameModifiers, game_modifiers);
		LIBRARY_STORE(GameSubDefinition, gameSubDefinitions, game_sub_definitions);
		LIBRARY_STORE(LootSelector, lootSelectors, loot_selectors);
		LIBRARY_STORE(ProjectileSelector, projectileSelectors, projectile_selectors);
		LIBRARY_STORE(Story::Chapter, storyChapters, story_chapters);
		LIBRARY_STORE(Story::Scene, storyScenes, story_scenes);
		LIBRARY_STORE(Tutorial, tutorials, tutorials);
		LIBRARY_STORE(TutorialStructure, tutorialStructures, tutorial_structures);
	};

};
