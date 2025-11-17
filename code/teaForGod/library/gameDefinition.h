#pragma once

#include "gameSubDefinition.h"
#include "..\game\energy.h"
#include "..\game\gameSettings.h"
#include "..\game\weaponPart.h"
#include "..\pilgrim\pilgrimGear.h"
#include "..\pilgrim\pilgrimSetup.h"
#include "..\pilgrimage\conditionalPilgrimage.h"

#include "..\..\core\memory\safeObject.h"
#include "..\..\core\other\remapValue.h"

#include "..\..\framework\library\libraryStored.h"

struct Tags;

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	struct LibraryLoadingContext;
};

namespace TeaForGodEmperor
{
	class MissionsDefinition;
	class Pilgrimage;
	class WeaponCoreModifiers;
	struct PersistenceProgress;

	struct GameProgressLevelStep
	{
		TagCondition requiredMilestones; // if not met, won't work
		int minProgressLevel100 = NONE; // if min is set and progress level is below, bumps to min, also if progress level exceeds min, it switches to next step
		int capProgressLevel100 = NONE; // don't cap if NONE, can't reach cap, stops at -1

		// this is based on run time to bump progress level a bit
		int stepProgressLevel100 = 1;
		int stepIntervalMinutes = 5;

		bool load_from_xml(IO::XML::Node const* _node);
	};

	struct GameProgressLevelSteps
	{
		Array<GameProgressLevelStep> steps;

		bool load_from_xml(IO::XML::Node const* _node);

		void advance_progress_level(REF_ PersistenceProgressLevel & _progressLevel, int _runTimeSeconds) const;
	};

	class GameDefinition
	: public Framework::LibraryStored
	, public SafeObject<GameDefinition>
	{
		FAST_CAST_DECLARE(GameDefinition);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		typedef ::SafePtr<GameDefinition> GameDefinitionSafePtr;

		enum Type
		{
			ExperienceRules,
			SimpleRules,
			ComplexRules,
			QuickGame,
			Tutorial,
			Test,
		};

	public:
		static void initialise_static();
		static void close_static();

		static void setup_difficulty_using_chosen(bool _changeExistingKeepAsMuchAsPossible = false);
		static void choose(GameDefinition const * _gameDefinition, Array<GameSubDefinition*> const & _gameSubDefinitions);
		static GameDefinition* get_chosen();
		static GameSubDefinition* get_chosen_sub(int _idx);

		static bool check_chosen(TagCondition const& _tagged);

	public:
		static bool check_loot_availability_against_progress_level(Tags const& _tags);
		static Optional<float> should_override_loot_prob_coef(Name const& _by);
		static bool does_allow_extra_tips();

		static void advance_loot_progress_level(REF_ PersistenceProgress& _progressLevel, int _runTimeSeconds);

	public:
		DifficultySettings const& get_difficulty() const { return difficultySettings; }
		
	public:
		GameDefinition(Framework::Library* _library, Framework::LibraryName const& _name);
		virtual ~GameDefinition();

		Type get_type() const { return type; }

		float get_selection_placement() const { return selectionPlacement; }
		TagCondition const& get_player_profile_requirements() const { return playerProfileRequirements; }

		Framework::LocalisedString const& get_name_for_ui() const { return nameForUI; }
		Framework::LocalisedString const& get_desc_for_selection_ui() const { return descForSelectionUI; }
		Framework::LocalisedString const& get_desc_for_ui() const { return descForUI; }
		
		String const& get_general_desc_for_ui(Array<Framework::UsedLibraryStored<GameSubDefinition>> const& _gameSubDefinitions) const;

		PilgrimGear const* get_default_starting_gear() const;
		PilgrimGear const* get_starting_gear(int _idx) const;
		int get_default_passive_exm_slot_count() const { return defaultPassiveEXMSlotCount; }

		WeaponCoreModifiers const* get_weapon_core_modifiers() const;

		TagCondition const& get_pilgrimage_devices_tagged() const { return pilgrimageDevicesTagged; }
		TagCondition const& get_exms_tagged() const { return exmsTagged; }

		Array<Framework::UsedLibraryStored<GameSubDefinition>> const& get_default_game_sub_definitions() const { return defaultGameSubDefinitions; }

		MissionsDefinition* get_starting_missions_definition() const { return startingMissionsDefinition.get(); }

		Framework::ActorType* get_pilgrim_actor_override() const { return pilgrimActorOverride.get(); }

		Array<Framework::UsedLibraryStored<Pilgrimage>> const& get_pilgrimages() const { return pilgrimages; }
		Array<ConditionalPilgrimage> const& get_conditional_pilgrimages() const { return conditionalPilgrimages; }

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	private:
		struct ChosenSet
		{
			Tags tags;
			GameDefinitionSafePtr chosenGameDefinition;
			ArrayStatic<GameSubDefinition::GameSubDefinitionSafePtr, 4> chosenGameSubDefinitions;
		};
		static ChosenSet* s_chosenSet;

		Type type = Type::QuickGame;

		float selectionPlacement = 0.0f;
		TagCondition playerProfileRequirements; // to hide some from the player

		Framework::LocalisedString nameForUI;
		Framework::LocalisedString descForSelectionUI;
		Framework::LocalisedString descForUI;

		struct GeneralDescForUI
		{
			Framework::LocalisedString generalDescForUI;
			TagCondition forGameSubDefinitionTagged;
		};
		Array<GeneralDescForUI> generalDescsForUI;

		Array<Framework::UsedLibraryStored<GameSubDefinition>> defaultGameSubDefinitions; // if missing

		Framework::UsedLibraryStored<MissionsDefinition> startingMissionsDefinition;

		Framework::UsedLibraryStored<Framework::ActorType> pilgrimActorOverride;

		Array<Framework::UsedLibraryStored<Pilgrimage>> pilgrimages; // pilgrimages/chapters
		Array<ConditionalPilgrimage> conditionalPilgrimages;

		GameProgressLevelSteps lootProgressLevelSteps;

		DifficultySettings difficultySettings;
		bool allowExtraTips = true;

		int defaultPassiveEXMSlotCount = 1;

		Array<RefCountObjectPtr<PilgrimGear>> startingGears;

		Framework::UsedLibraryStored<WeaponCoreModifiers> weaponCoreModifiers;

		TagCondition pilgrimageDevicesTagged;
		TagCondition exmsTagged;
	};
};
