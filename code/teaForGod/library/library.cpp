#include "library.h"

#include "physicalMaterial.h"

#include "..\ai\aiCommon.h"
#include "..\ai\aiManagerData.h"
#include "..\roomGenerators\roomGenerationInfo.h"

#include "..\..\core\types\names.h"

using namespace TeaForGodEmperor;

void Library::initialise_stores()
{
	Framework::Library::initialise_stores();

	ADD_LIBRARY_STORE(DamageRuleSet, damageRuleSets);
	ADD_LIBRARY_STORE(DefaultPilgrimSetupPreset, defaultPilgrimSetupPresets);
	ADD_LIBRARY_STORE(Pilgrimage, pilgrimages);
	ADD_LIBRARY_STORE(PilgrimageDevice, pilgrimageDevices);
	ADD_LIBRARY_STORE(CustomUpgradeType, customUpgradeTypes);
	ADD_LIBRARY_STORE(EXMType, exmTypes);
	ADD_LIBRARY_STORE(ExtraUpgradeOption, extraUpgradeOptions);
	ADD_LIBRARY_STORE(LineModel, lineModels);
	ADD_LIBRARY_STORE(WeaponCoreModifiers, weaponCoreModifiers);
	ADD_LIBRARY_STORE(WeaponPartType, weaponPartTypes);
	ADD_LIBRARY_STORE(MessageSet, messageSets);
	ADD_LIBRARY_STORE(MissionBiome, missionBiomes);
	ADD_LIBRARY_STORE(MissionDefinition, missionDefinitions);
	ADD_LIBRARY_STORE(MissionDifficultyModifier, missionDifficultyModifiers);
	ADD_LIBRARY_STORE(MissionGeneralProgress, missionGeneralProgress);
	ADD_LIBRARY_STORE(MissionObjectivesAndIntel, missionObjectivesAndIntel);
	ADD_LIBRARY_STORE(MissionSelectionTier, missionSelectionTiers);
	ADD_LIBRARY_STORE(MissionsDefinition, missionsDefinitions);
	ADD_LIBRARY_STORE(GameDefinition, gameDefinitions);
	ADD_LIBRARY_STORE(GameModifier, gameModifiers);
	ADD_LIBRARY_STORE(GameSubDefinition, gameSubDefinitions);
	ADD_LIBRARY_STORE(LootSelector, lootSelectors);
	ADD_LIBRARY_STORE(ProjectileSelector, projectileSelectors);
	ADD_LIBRARY_STORE(Story::Chapter, storyChapters);
	ADD_LIBRARY_STORE(Story::Scene, storyScenes);
	ADD_LIBRARY_STORE(Tutorial, tutorials);
	ADD_LIBRARY_STORE(TutorialStructure, tutorialStructures);

	OVERRIDE_LIBRARY_STORE(PhysicalMaterial, physicalMaterials);
}

bool Library::load_gameplay(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc, String const & _fileName, OUT_ bool & _loadResult)
{
	return base::load_gameplay(_node, _lc, _fileName, OUT_ _loadResult);
}

void Library::before_reload()
{
	base::before_reload();

	TeaForGodEmperor::LootSelector::initialise_static();
	TeaForGodEmperor::ProjectileSelector::initialise_static();
}

bool Library::unload(Framework::LibraryLoadLevel::Type _libraryLoadLevel)
{
	if (_libraryLoadLevel <= Framework::LibraryLoadLevel::Main)
	{
		// will be reloaded if needed
		ProjectileSelector::close_static();
		LootSelector::close_static();
	}
	return base::unload(_libraryLoadLevel);
}

bool Library::does_allow_loading(Framework::LibraryStoreBase* _store, Framework::LibraryLoadingContext& _lc) const
{
	return base::does_allow_loading(_store, _lc);
}

