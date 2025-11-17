#include "gse_gameDirector.h"

#include "..\..\game\game.h"
#include "..\..\game\gameDirector.h"
#include "..\..\story\storyExecution.h"

#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool Elements::GameDirector::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	gameMenuBlocked.load_from_xml(_node, TXT("gameMenuBlocked"));
	storingIntermediateGameStatesBlocked.load_from_xml(_node, TXT("storingIntermediateGameStatesBlocked"));
	doorsBlocked.load_from_xml(_node, TXT("doorsBlocked"));
	accidentalMusicBlocked.load_from_xml(_node, TXT("accidentalMusicBlocked"));
	mapBlocked.load_from_xml(_node, TXT("mapBlocked"));
	revealMapOnUpgradeBlocked.load_from_xml(_node, TXT("revealMapOnUpgradeBlocked"));
	doorDirectionsBlocked.load_from_xml(_node, TXT("doorDirectionsBlocked"));
	forearmNavigationBlocked.load_from_xml(_node, TXT("forearmNavigationBlocked"));
	upgradeCanisterInfoBlocked.load_from_xml(_node, TXT("upgradeCanisterInfoBlocked"));
	realMovementInputTipBlocked.load_from_xml(_node, TXT("realMovementInputTipBlocked"));
	pilgrimageDevicesUnobfusticationClearOnHold.load_from_xml(_node, TXT("pilgrimageDevicesUnobfusticationClearOnHold"));
	pilgrimageDevicesUnobfusticationOnHold.load_from_xml(_node, TXT("pilgrimageDevicesUnobfusticationOnHold"));

	narrativeRequested.load_from_xml(_node, TXT("requestNarrative"));
	narrativeRequested.load_from_xml(_node, TXT("narrative"));
	activeScene.load_from_xml(_node, TXT("activeScene"));
	
	endDemo.load_from_xml(_node, TXT("endDemo"));
	endGame.load_from_xml(_node, TXT("endGame"));

	unobfusticatePilgrimageDevice.load_from_xml(_node, TXT("unobfusticatePilgrimageDevice"), _lc);

	pilgrimOverlayBlocked.load_from_xml(_node, TXT("pilgrimOverlayBlocked"));
	pilgrimOverlayInHandStatsBlocked.load_from_xml(_node, TXT("pilgrimOverlayInHandStatsBlocked"));
	followGuidanceBlocked.load_from_xml(_node, TXT("followGuidanceBlocked"));
	weaponsBlocked.load_from_xml(_node, TXT("weaponsBlocked"));
	healthStatusBlocked.load_from_xml(_node, TXT("healthStatusBlocked"));
	ammoStatusBlocked.load_from_xml(_node, TXT("ammoStatusBlocked"));
	guidanceBlocked.load_from_xml(_node, TXT("guidanceBlocked"));
	guidanceStatusBlocked.load_from_xml(_node, TXT("guidanceStatusBlocked"));
	guidanceSimplified.load_from_xml(_node, TXT("guidanceSimplified"));
	statusBlendTime.load_from_xml(_node, TXT("statusBlendTime"));
	timeAdjustmentBlocked.load_from_xml(_node, TXT("timeAdjustmentBlocked"));
	forceWideStats.load_from_xml(_node, TXT("forceWideStats"));
	respawnAndContinueBlocked.load_from_xml(_node, TXT("respawnAndContinueBlocked"));
	immortalHealthRegenBlocked.load_from_xml(_node, TXT("immortalHealthRegenBlocked"));
	infiniteAmmoBlocked.load_from_xml(_node, TXT("infiniteAmmoBlocked"));
	ammoStoragesUnavailable.load_from_xml(_node, TXT("ammoStoragesUnavailable"));
	lockIndicatorsDisabled.load_from_xml(_node, TXT("lockIndicatorsDisabled"));
	keepInWorldTemporarilyDisabled.load_from_xml(_node, TXT("keepInWorldTemporarilyDisabled"));
	temporaryFoveationLevelOffset.load_from_xml(_node, TXT("temporaryFoveationLevelOffset"));

	return result;
}

bool Elements::GameDirector::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= unobfusticatePilgrimageDevice.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

#define GD_STATE(what) \
	if (what.is_set()) \
	{ \
		gd->state.what = what.get(); \
	}

#define GD_STATE_AS(what, as) \
	if (what.is_set()) \
	{ \
		gd->state.as = what.get(); \
	}

Framework::GameScript::ScriptExecutionResult::Type Elements::GameDirector::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* gd = TeaForGodEmperor::GameDirector::get_active())
	{
		Concurrency::ScopedSpinLock lock(gd->state.lock);
		GD_STATE(gameMenuBlocked);
		GD_STATE(storingIntermediateGameStatesBlocked);
		GD_STATE_AS(doorsBlocked, blockDoors);
		GD_STATE(accidentalMusicBlocked);
		GD_STATE(mapBlocked);
		GD_STATE(revealMapOnUpgradeBlocked);
		GD_STATE(doorDirectionsBlocked);
		GD_STATE(forearmNavigationBlocked);
		GD_STATE(upgradeCanisterInfoBlocked);
		GD_STATE(realMovementInputTipBlocked);
		if (pilgrimageDevicesUnobfusticationClearOnHold.is_set())
		{
			if (!pilgrimageDevicesUnobfusticationClearOnHold.get())
			{
				if (auto* piow = PilgrimageInstanceOpenWorld::get())
				{
					piow->clear_pilgrimage_device_directions_known_on_hold();
				}
			}
		}
		if (pilgrimageDevicesUnobfusticationOnHold.is_set())
		{
			gd->state.pilgrimageDevicesUnobfusticationOnHold = pilgrimageDevicesUnobfusticationOnHold.get();
			if (!gd->state.pilgrimageDevicesUnobfusticationOnHold)
			{
				if (auto* piow = PilgrimageInstanceOpenWorld::get())
				{
					piow->update_pilgrimage_device_directions_known_on_hold();
				}
			}
		}
		if (auto* pd = unobfusticatePilgrimageDevice.get())
		{
			if (auto* piow = PilgrimageInstanceOpenWorld::get())
			{
				piow->mark_pilgrimage_device_direction_known(pd);
			}
		}
		GD_STATE(narrativeRequested);
		GD_STATE(activeScene);
		if (endDemo.is_set() && endDemo.get())
		{
			if (auto* g = Game::get_as<Game>())
			{
				g->request_post_game_summary(PostGameSummary::ReachedDemoEnd);
			}
		}
		if (endGame.is_set() && endGame.get())
		{
			if (auto* g = Game::get_as<Game>())
			{
				g->request_post_game_summary(PostGameSummary::ReachedEnd);
			}
		}
		GD_STATE(pilgrimOverlayBlocked);
		GD_STATE(pilgrimOverlayInHandStatsBlocked);
		GD_STATE(followGuidanceBlocked);
		GD_STATE(weaponsBlocked);
		GD_STATE(healthStatusBlocked);
		GD_STATE(ammoStatusBlocked);
		GD_STATE(guidanceBlocked);
		GD_STATE(guidanceStatusBlocked);
		GD_STATE(guidanceSimplified);
		GD_STATE(statusBlendTime);
		GD_STATE(timeAdjustmentBlocked);
		GD_STATE(forceWideStats);
		GD_STATE(respawnAndContinueBlocked);
		GD_STATE(immortalHealthRegenBlocked);
		GD_STATE(infiniteAmmoBlocked);
		GD_STATE(ammoStoragesUnavailable);
		GD_STATE(lockIndicatorsDisabled);
		GD_STATE(keepInWorldTemporarilyDisabled);
		GD_STATE(temporaryFoveationLevelOffset);
	}
	return Framework::GameScript::ScriptExecutionResult::Continue;
}
