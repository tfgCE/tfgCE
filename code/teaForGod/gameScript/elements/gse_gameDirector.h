#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"
#include "..\..\..\framework\library\usedLibraryStored.h"

namespace TeaForGodEmperor
{
	class PilgrimageDevice;

	namespace Story
	{
		class Scene;
	};

	namespace GameScript
	{
		namespace Elements
		{
			class GameDirector
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("game director"); }

			private:
				Optional<bool> gameMenuBlocked;
				Optional<bool> storingIntermediateGameStatesBlocked;
				Optional<bool> doorsBlocked;
				Optional<bool> accidentalMusicBlocked;
				Optional<bool> mapBlocked;
				Optional<bool> revealMapOnUpgradeBlocked;
				Optional<bool> doorDirectionsBlocked;
				Optional<bool> forearmNavigationBlocked;
				Optional<bool> upgradeCanisterInfoBlocked;
				Optional<bool> realMovementInputTipBlocked;
				Optional<bool> pilgrimageDevicesUnobfusticationClearOnHold;
				Optional<bool> pilgrimageDevicesUnobfusticationOnHold;
				Optional<bool> narrativeRequested; // enemies should ignore fight
				Optional<bool> activeScene; // just an information that one scene is now being played
				Optional<bool> endDemo;
				Optional<bool> endGame;
				Optional<bool> pilgrimOverlayBlocked;
				Optional<bool> pilgrimOverlayInHandStatsBlocked;
				Optional<bool> followGuidanceBlocked;
				Optional<bool> weaponsBlocked;
				Optional<bool> healthStatusBlocked;
				Optional<bool> ammoStatusBlocked;
				Optional<bool> guidanceBlocked;
				Optional<bool> guidanceStatusBlocked;
				Optional<bool> guidanceSimplified; // only door directions and only as objective
				Optional<float> statusBlendTime;
				Optional<bool> timeAdjustmentBlocked;
				Optional<bool> forceWideStats;
				Optional<bool> respawnAndContinueBlocked;
				Optional<bool> immortalHealthRegenBlocked;
				Optional<bool> infiniteAmmoBlocked;
				Optional<bool> ammoStoragesUnavailable;
				Optional<bool> lockIndicatorsDisabled;
				Optional<bool> keepInWorldTemporarilyDisabled;
				Optional<float> temporaryFoveationLevelOffset;

				Framework::UsedLibraryStored<PilgrimageDevice> unobfusticatePilgrimageDevice;
			};
		};
	};
};
