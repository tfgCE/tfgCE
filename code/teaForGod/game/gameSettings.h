#pragma once

#include "energy.h"

#include "..\teaEnums.h"

#include "..\..\core\concurrency\spinLock.h"
#include "..\..\core\containers\array.h"
#include "..\..\core\debug\debug.h"
#include "..\..\core\math\math.h"
#include "..\..\core\tags\tag.h"
#include "..\..\core\types\hand.h"
#include "..\..\core\types\optional.h"

#include "..\..\framework\text\localisedString.h"

class SimpleVariableStorage;

namespace Framework
{
	class Game;
	struct DebugPanel;
	struct DebugPanelOption;
};

namespace TeaForGodEmperor
{
	class Game;
	class GameSettingsObserver;

	struct GameSettings
	{
	public:
		struct AIAgressiveness
		{
			int temp;
			enum Type
			{
				Ignores = -1,
				Curious = 0,
				NotFriendly = 1,
				Aggressive = 2,
				VeryAggressive = 3,
				Maniac = 4,
				MAX
			};

			inline static tchar const * to_char(Type _type)
			{
				if (_type == Ignores) return TXT("ignores");
				if (_type == Curious) return TXT("curious");
				if (_type == NotFriendly) return TXT("not friendly");
				if (_type == Aggressive) return TXT("aggressive");
				if (_type == VeryAggressive) return TXT("very aggressive");
				if (_type == Maniac) return TXT("maniac");
				return TXT("??");
			}
		};

		struct NPCS
		{
			int temp;
			enum Type
			{
				NoNPCs,
				NoHostiles,
				NonAggressive,
				Normal,
				MAX
			};

			inline static tchar const* to_char(Type _type)
			{
				if (_type == NoNPCs) return TXT("no npcs");
				if (_type == NoHostiles) return TXT("no hostiles");
				if (_type == NonAggressive) return TXT("non aggressive");
				if (_type == Normal) return TXT("normal");
				return TXT("??");
			}
		};

		struct RestartMode
		{
			int temp;
			enum Type
			{
				RespawnAndContinue,
				AnyCheckpoint,
				ReachedChapter,
				UnlockedChapter,
				WholeThing,
				MAX
			};

			inline static tchar const* to_char(Type _type)
			{
				if (_type == RespawnAndContinue) return TXT("respawn and continue");
				if (_type == AnyCheckpoint) return TXT("any checkpoint");
				if (_type == ReachedChapter) return TXT("reached chapter");
				if (_type == UnlockedChapter) return TXT("unlocked chapter");
				if (_type == WholeThing) return TXT("whole thing");
				return TXT("??");
			}
		};


	public:
		// possible options
		struct RoomSize
		{
			String name;
			Optional<Vector2> size;
			RoomSize() {}
			RoomSize(tchar const * _name, Optional<Vector2> const & _size = NP) : name(_name), size(_size) {}
		};
		Array<RoomSize> roomSizes;

		struct Settings
		{
			// other info
			bool restartRequired = false;

			// general
			int roomSize = 0;

			// game
			
			// options/preferences
			bool realityAnchors = false;

			// tweaking
#ifdef AN_DEVELOPMENT
			float pickupDistance = 0.3f; // 2.0f;
#else
			float pickupDistance = 0.3f;
#endif
			bool mainEquipmentCanBePutIntoPocket = false;
			float grabDistance = 0.15f;
			float grabRelease = 0.25f;
			float energyAttractDistance = 1.0f;
			float pickupFromPocketDistance = 0.15f;
			float pocketDistance = 0.2f;
		};

		struct Difficulty
		{
			// actual game settings (that affect difficulty)
			bool playerImmortal = false;
			bool weaponInfinite = false;
			bool armourIneffective = false; // adjust damage doesn't work if smaller
			float armourEffectivenessLimit = 1.0f; // at 0, armour is ineffective, at 0.25, armour is effective in 25% (adjust damage is 0.75), LIMITS effectiveness, not multiplies
#ifdef WITH_CRYSTALITE
			bool withCrystalite = false;
			bool instantCrystalite = false;
#endif
			bool autoMapOnly = false; // if set to true, you don't know your position on map (unless using auto map), you don't draw your map
			bool noTriangulationHelp = false; // if set to true, no triangulation assist is provided, you have to find targets on your own
			bool pathOnly = false; // if set to true, door directions show only path to the objective
			NPCS::Type npcs = NPCS::Normal;
			float energyQuantumsStayTime = 1.0f; // 0.0 they do not disappear
			AIAgressiveness::Type aiAggressiveness = AIAgressiveness::NotFriendly;
			float playerDamageGiven = 1.0f;
			float playerDamageTaken = 1.0f;
			float aiReactionTime = 1.0f;
			float aiMean = 0.0f;
			float aiSpeedBasedModifier = 0.0f; // +50% etc this is for all stuff based on speed (intervals for random things, moving platforms etc)
			float allowTelegraphOnSpawn = 0.0f;
			float allowAnnouncePresence = 0.0f;
			float forceMissChanceOnLowHealth = 0.0f;
			float forceHitChanceOnFullHealth = 0.0f;
			float blockSpawningInVisitedRoomsTime = 240.0f; // should be non-zero, at least 0.1
			float spawnCountModifier = 1.0f;
			float lootAmmo = 1.0f;
			float lootHealth = 1.0f;
			// not adjustable by modifiers (add them to set_non_game_modifiers_from)
			// some are part of IncompleteDifficulty as they might be part of GameSubDefinition
			bool commonHandEnergyStorage = false;
			bool regenerateEnergy = false;
			bool simplerMazes = false;
			bool linearLevels = false; // only towards objective/end, objectives get auto updated, devices are skipped
			bool simplerPuzzles = false;
			bool showDirectionsOnlyToObjective = false; // to objective as opposed to all door directions, plus pilgrimage devices unless pathOnly is set
			bool showDirectionsOnlyToNonDepleted = false;
			bool showLineModelAlwaysOtherwiseOnMap = false; // some places may be marked to always appear on map (always otherwise -> always if not known)
			float energyCoilsAmount = 1.0f;
			int passiveEXMSlots = 1;
			bool allowPermanentEXMsAtUpgradeMachine = false;
			bool allowEXMReinstalling = true; // there are EXM reinstallers, so you can reinstall EXM that was dropped using it. if this is false, upgrade machines may reintroduce dropped EXMs
											  // also, if this is set to true, at the first upgrade machine, there will be all unlocked EXMs, not just the basic set
			bool obfuscateSymbols = true;
			bool displayMapOnUpgrade = true; // if set to true, the map is displayed on upgrade
			bool revealDevicesOnUpgrade = true; // if set to true, the devices are revealed on upgrade
			RestartMode::Type restartMode = RestartMode::WholeThing;
			bool respawnAndContinueOnlyIfEnoughEnergy = false; // if respawn and continue possible, do it only if there's enough energy
			bool mapAlwaysAvailable = true; // ignores mapAvailable (has to be one of two (the other is in player preferences)
			int healthOnContinue = 0; // if restartMode is set to "continue" we may want to limit the energy instead of giving full recovery
			bool noOverlayInfo = false; // no overlay info at all

			bool save_to_xml(IO::XML::Node* _node) const;
			bool load_from_xml(IO::XML::Node const * _node);
			void set_from_tags(Tags const& _tags);
			void store_as_tags(Tags& _tags) const;

			void set_all_from(Difficulty const& _other)
			{
				*this = _other;
			}
			#define COPY(what) what = _other.what;
			void set_non_game_modifiers_from(Difficulty const& _other)
			{
				COPY(commonHandEnergyStorage);
				COPY(regenerateEnergy);
				COPY(simplerMazes);
				COPY(linearLevels);
				COPY(simplerPuzzles);
				COPY(showDirectionsOnlyToObjective);
				COPY(showDirectionsOnlyToNonDepleted);
				COPY(showLineModelAlwaysOtherwiseOnMap);
				COPY(energyCoilsAmount);
				COPY(passiveEXMSlots);
				COPY(allowPermanentEXMsAtUpgradeMachine);
				COPY(allowEXMReinstalling);
				COPY(obfuscateSymbols);
				COPY(displayMapOnUpgrade);
				COPY(revealDevicesOnUpgrade);
				COPY(restartMode);
				COPY(respawnAndContinueOnlyIfEnoughEnergy);
				COPY(mapAlwaysAvailable);
				COPY(healthOnContinue);
				COPY(noOverlayInfo);
			}
			#undef COPY
		};

		struct IncompleteDifficulty
		{
			Optional<bool> playerImmortal;
			Optional<bool> weaponInfinite;
			Optional<bool> armourIneffective;
			Optional<float> armourEffectivenessLimit;
#ifdef WITH_CRYSTALITE
			Optional<bool> withCrystalite;
			Optional<bool> instantCrystalite;
#endif
			Optional<bool> autoMapOnly;
			Optional<bool> noTriangulationHelp;
			Optional<bool> pathOnly;
			Optional<NPCS::Type> npcs;
			Optional<float> energyQuantumsStayTime;
			Optional<AIAgressiveness::Type> aiAggressiveness;
			Optional<float> playerDamageGiven;
			Optional<float> playerDamageTaken;
			Optional<float> aiReactionTime;
			Optional<float> aiMean;
			Optional<float> aiSpeedBasedModifier;
			Optional<float> allowTelegraphOnSpawn;
			Optional<float> allowAnnouncePresence;
			Optional<float> forceMissChanceOnLowHealth;
			Optional<float> forceHitChanceOnFullHealth;
			Optional<float> blockSpawningInVisitedRoomsTime;
			Optional<float> spawnCountModifier;
			Optional<float> lootAmmo;
			Optional<float> lootHealth;
			Optional<RestartMode::Type> restartMode;

			void apply_to(Difficulty& _difficulty) const;
			bool save_to_xml(IO::XML::Node* _node) const;
			bool load_from_xml(IO::XML::Node const* _node);
		};

		Settings settings;
		Difficulty difficulty;

		EnergyCoef experienceModifier = EnergyCoef::zero();

		void inform_observers();

		void setup_variables(SimpleVariableStorage & _storage);

	public:
		float ai_agrressiveness_as_float() const { return (float)difficulty.aiAggressiveness; }

	public:
		static bool is_available() { return s_gameSettings != nullptr; }
		static GameSettings& get() { an_assert(s_gameSettings);  return *s_gameSettings; }

		static void create(Game* _game);
		static void destroy();

		static Hand::Type any_use_hand(Hand::Type _hand) { return get().difficulty.commonHandEnergyStorage ? Hand::Left : _hand; }
		static Hand::Type actual_hand(Hand::Type _hand) { return _hand; }

		GameSettings(Game* _game);
		~GameSettings();

		Framework::DebugPanel* update_and_get_debug_panel();

		void restart_required();
		void restart_not_required();
		
		void load_test_difficulty_settings();

	private: friend class GameSettingsObserver;
		void add_observer(GameSettingsObserver* _observer);
		void remove_observer(GameSettingsObserver* _observer);

	private:
		static GameSettings* s_gameSettings;
		
		Game* game = nullptr;
		Framework::DebugPanel* debugPanel = nullptr;
		Framework::DebugPanelOption* restartOption = nullptr;

		Concurrency::SpinLock observersLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.GameSettings.observersLock"));
		Array<GameSettingsObserver*> observers;

		void create_debug_panel(Game* _game);
	};

	struct DifficultySettings
	{
		GameSettings::Difficulty difficulty;

		bool load_from_xml(IO::XML::Node const* _node);
		void save_to_xml(IO::XML::Node* _node, bool _saveLocalisedStrings) const;
	};

};
