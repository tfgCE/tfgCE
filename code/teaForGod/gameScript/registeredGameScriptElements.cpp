#include "registeredGameScriptElements.h"

#include "elements/gse_airProhibitedPlace.h"
#include "elements/gse_armoury.h"
#include "elements/gse_blendEmissivesForRoom.h"
#include "elements/gse_cameraRumble.h"
#include "elements/gse_checkpoint.h"
#include "elements/gse_clearOverlayInfo.h"
#include "elements/gse_createMainEquipment.h"
#include "elements/gse_customUniforms.h"
#include "elements/gse_deactivateHub.h"
#include "elements/gse_door.h"
#include "elements/gse_emissive.h"
#include "elements/gse_endMission.h"
#include "elements/gse_environmentPlayer.h"
#include "elements/gse_find.h"
#include "elements/gse_forceCamera.h"
#include "elements/gse_forceSoundCamera.h"
#include "elements/gse_game.h"
#include "elements/gse_gameDirector.h"
#include "elements/gse_gameSlot.h"
#include "elements/gse_generalProgress.h"
#include "elements/gse_getCellCoord.h"
#include "elements/gse_globalTint.h"
#include "elements/gse_health.h"
#include "elements/gse_hideInputPrompt.h"
#include "elements/gse_hitIndicator.h"
#include "elements/gse_mainEquipment.h"
#include "elements/gse_missionState.h"
#include "elements/gse_mixEnvironmentCycle.h"
#include "elements/gse_music.h"
#include "elements/gse_pilgrim.h"
#include "elements/gse_pilgrimage.h"
#include "elements/gse_pilgrimOverlayInfo.h"
#include "elements/gse_player.h"
#include "elements/gse_provideExperience.h"
#include "elements/gse_removeOverlayInfo.h"
#include "elements/gse_setGameSettings.h"
#include "elements/gse_setPlayerPreferences.h"
#include "elements/gse_shoot.h"
#include "elements/gse_showInputPrompt.h"
#include "elements/gse_showOverlayInfo.h"
#include "elements/gse_showRect.h"
#include "elements/gse_showText.h"
#include "elements/gse_sound.h"
#include "elements/gse_soundOverridePlayParams.h"
#include "elements/gse_spawn.h"
#include "elements/gse_spawnEnergyQuantum.h"
#include "elements/gse_storyChangeScene.h"
#include "elements/gse_storyClear.h"
#include "elements/gse_storyEndScene.h"
#include "elements/gse_storySet.h"
#include "elements/gse_subtitle.h"
#include "elements/gse_takeControl.h"
#include "elements/gse_teleport.h"
#include "elements/gse_tutInput.h"
#include "elements/gse_tutClearHighlight.h"
#include "elements/gse_tutClearHubHighlight.h"
#include "elements/gse_tutContinue.h"
#include "elements/gse_tutEnd.h"
#include "elements/gse_tutHighlight.h"
#include "elements/gse_tutHubHighlight.h"
#include "elements/gse_tutHubSelect.h"
#include "elements/gse_tutInGameMenuShouldNotSupressOverlay.h"
#include "elements/gse_tutSet.h"
#include "elements/gse_tutStep.h"
#include "elements/gse_tutWaitForInGameMenu.h"
#include "elements/gse_vignetteForDead.h"
#include "elements/gse_vignetteForMovement.h"
#include "elements/gse_voiceoverSilence.h"
#include "elements/gse_voiceoverSpeak.h"
#include "elements/gse_waitForEnergyTransfer.h"
#include "elements/gse_waitForHub.h"
#include "elements/gse_waitForLook.h"
#include "elements/gse_waitForMainEquipment.h"
#include "elements/gse_waitForNoVoiceover.h"

#include "..\library\library.h"
#include "..\..\framework\library\usedLibraryStored.inl"

#include "..\..\framework\gameScript\registeredGameScriptElements.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;

//

void RegisteredScriptElements::initialise_static()
{
	// add elements
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("airProhibitedPlace")), []() { return new Elements::AirProhibitedPlace(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("armoury")), []() { return new Elements::Armoury(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("blendEmissivesForRoom")), []() { return new Elements::BlendEmissivesForRoom(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("cameraRumble")), []() { return new Elements::CameraRumble(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("checkpoint")), []() { return new Elements::Checkpoint(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("clearOverlayInfo")), []() { return new Elements::ClearOverlayInfo(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("createMainEquipment")), []() { return new Elements::CreateMainEquipment(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("customUniforms")), []() { return new Elements::CustomUniforms(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("deactivateHub")), []() { return new Elements::DeactivateHub(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("door")), []() { return new Elements::Door(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("emissive")), []() { return new Elements::Emissive(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("endMission")), []() { return new Elements::EndMission(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("environmentPlayer")), []() { return new Elements::EnvironmentPlayer(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("find")), []() { return new Elements::Find(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("forceSoundCamera")), []() { return new Elements::ForceSoundCamera(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("game")), []() { return new Elements::Game(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("gameDirector")), []() { return new Elements::GameDirector(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("gameSlot")), []() { return new Elements::GameSlot(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("generalProgress")), []() { return new Elements::GeneralProgress(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("getCellCoord")), []() { return new Elements::GetCellCoord(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("globalTint")), []() { return new Elements::GlobalTint(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("health")), []() { return new Elements::Health(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("hideInputPrompt")), []() { return new Elements::HideInputPrompt(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("hitIndicator")), []() { return new Elements::HitIndicator(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("mainEquipment")), []() { return new Elements::MainEquipment(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("missionState")), []() { return new Elements::MissionState(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("mixEnvironmentCycle")), []() { return new Elements::MixEnvironmentCycle(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("music")), []() { return new Elements::Music(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("pilgrim")), []() { return new Elements::Pilgrim(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("pilgrimage")), []() { return new Elements::Pilgrimage(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("pilgrimOverlayInfo")), []() { return new Elements::PilgrimOverlayInfo(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("player")), []() { return new Elements::Player(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("provideExperience")), []() { return new Elements::ProvideExperience(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("removeOverlayInfo")), []() { return new Elements::RemoveOverlayInfo(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("setGameSettings")), []() { return new Elements::SetGameSettings(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("setPlayerPreferences")), []() { return new Elements::SetPlayerPreferences(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("shoot")), []() { return new Elements::Shoot(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("showInputPrompt")), []() { return new Elements::ShowInputPrompt(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("showOverlayInfo")), []() { return new Elements::ShowOverlayInfo(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("showRect")), []() { return new Elements::ShowRect(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("showText")), []() { return new Elements::ShowText(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("sound")), []() { return new Elements::Sound(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("soundOverridePlayParams")), []() { return new Elements::SoundOverridePlayParams(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("spawn")), []() { return new Elements::Spawn(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("spawnEnergyQuantum")), []() { return new Elements::SpawnEnergyQuantum(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("storyChangeScene")), []() { return new Elements::StoryChangeScene(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("storyClear")), []() { return new Elements::StoryClear(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("storyEndScene")), []() { return new Elements::StoryEndScene(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("storySet")), []() { return new Elements::StorySet(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("subtitle")), []() { return new Elements::Subtitle(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("takeControl")), []() { return new Elements::TakeControl(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("teleport")), []() { return new Elements::Teleport(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("tutAllowAllInput")), []() { return new Elements::TutAllowAllInput(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("tutAllowHubInput")), []() { return new Elements::TutAllowHubInput(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("tutAllowInput")), []() { return new Elements::TutAllowInput(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("tutAllowOverWidget")), []() { return new Elements::TutAllowOverWidget(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("tutBlockAllInput")), []() { return new Elements::TutBlockAllInput(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("tutBlockHubInput")), []() { return new Elements::TutBlockHubInput(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("tutBlockInput")), []() { return new Elements::TutBlockInput(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("tutClearHighlight")), []() { return new Elements::TutClearHighlight(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("tutClearHubHighlight")), []() { return new Elements::TutClearHubHighlight(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("tutContinue")), []() { return new Elements::TutContinue(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("tutEnd")), []() { return new Elements::TutEnd(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("tutHighlight")), []() { return new Elements::TutHighlight(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("tutHubHighlight")), []() { return new Elements::TutHubHighlight(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("tutHubSelect")), []() { return new Elements::TutHubSelect(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("tutInGameMenuShouldNotSupressOverlay")), []() { return new Elements::TutInGameMenuShouldNotSupressOverlay(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("tutRestoreOverWidget")), []() { return new Elements::TutRestoreOverWidget(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("tutSet")), []() { return new Elements::TutSet(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("tutStep")), []() { return new Elements::TutStep(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("tutWaitForInGameMenu")), []() { return new Elements::TutWaitForInGameMenu(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("vignetteForDead")), []() { return new Elements::VignetteForDead(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("vignetteForMovement")), []() { return new Elements::VignetteForMovement(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("voiceoverSilence")), []() { return new Elements::VoiceoverSilence(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("voiceoverSpeak")), []() { return new Elements::VoiceoverSpeak(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("waitForEnergyTransfer")), []() { return new Elements::WaitForEnergyTransfer(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("waitForHub")), []() { return new Elements::WaitForHub(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("waitForLook")), []() { return new Elements::WaitForLook(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("waitForMainEquipment")), []() { return new Elements::WaitForMainEquipment(); }));
	Framework::GameScript::RegisteredScriptElements::add(Framework::GameScript::RegisteredScriptElement(Name(TXT("waitForNoVoiceover")), []() { return new Elements::WaitForNoVoiceover(); }));
}

void RegisteredScriptElements::close_static()
{
}
