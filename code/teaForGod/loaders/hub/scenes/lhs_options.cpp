#include "lhs_options.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"

#include "..\screens\lhc_debugNote.h"
#include "..\screens\lhc_enterText.h"
#include "..\screens\lhc_message.h"
#include "..\screens\lhc_messageSetBrowser.h"
#include "..\screens\lhc_question.h"
#include "..\widgets\lhw_button.h"
#include "..\widgets\lhw_customDraw.h"
#include "..\widgets\lhw_image.h"
#include "..\widgets\lhw_list.h"
#include "..\widgets\lhw_text.h"

#include "..\..\..\teaEnums.h"
#include "..\..\..\teaForGodMain.h"

#include "..\..\..\game\game.h"
#include "..\..\..\game\gameConfig.h"
#include "..\..\..\game\gameSettings.h"
#include "..\..\..\game\playerSetup.h"
#include "..\..\..\game\screenShotContext.h"
#include "..\..\..\library\library.h"
#include "..\..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\..\roomGenerators\roomGenerationInfo.h"

#include "..\..\..\..\core\mainSettings.h"
#include "..\..\..\..\core\other\parserUtils.h"
#include "..\..\..\..\core\physicalSensations\allPhysicalSensations.h"
#include "..\..\..\..\core\system\sound\soundSystem.h"
#include "..\..\..\..\core\system\video\video3dPrimitives.h"

#include "..\..\..\..\framework\jobSystem\jobSystem.h"
#include "..\..\..\..\framework\library\library.h"
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\..\framework\module\modulePresence.h"
#include "..\..\..\..\framework\object\actor.h"
#include "..\..\..\..\framework\time\dateTime.h"

#include "..\..\..\..\core\system\video\foveatedRenderingSetup.h"

#ifdef AN_DEVELOPMENT_OR_PROFILER
// debug stuff
#include "..\..\..\..\framework\render\roomProxy.h"
#endif

#ifdef BUILD_NON_RELEASE
#include "..\..\..\..\core\vr\vrOffsets.h"
#endif

#ifdef BUILD_PREVIEW
#define WITH_DEBUG_CHEAT_MENU
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define VERY_LOW_BATTERY 15
#define LOW_BATTERY 30

#define VERY_LOW_BATTERY_COLOUR (Colour::lerp(0.4f, Colour::red, Colour::white))
#define LOW_BATTERY_COLOUR (Colour::lerp(0.4f, Colour::orange, Colour::white))

//

// core advance
DEFINE_STATIC_NAME(advanceRunTimer);

// localised strings
DEFINE_STATIC_NAME_STR(lsDebugNote, TXT("hub; opt; debug note"));
DEFINE_STATIC_NAME_STR(lsDebugNoteInfo, TXT("hub; opt; debug note info"));

DEFINE_STATIC_NAME_STR(lsGeneral, TXT("hub; opt; general"));
DEFINE_STATIC_NAME_STR(lsPlayArea, TXT("hub; opt; play area"));
DEFINE_STATIC_NAME_STR(lsVideo, TXT("hub; opt; video"));
DEFINE_STATIC_NAME_STR(lsAesthetics, TXT("hub; opt; aesthetics"));
DEFINE_STATIC_NAME_STR(lsDevices, TXT("hub; opt; devices"));
DEFINE_STATIC_NAME_STR(lsSound, TXT("hub; opt; sound"));
DEFINE_STATIC_NAME_STR(lsControls, TXT("hub; opt; controls"));
DEFINE_STATIC_NAME_STR(lsGameplay, TXT("hub; opt; gameplay"));
DEFINE_STATIC_NAME_STR(lsComfort, TXT("hub; opt; comfort"));
DEFINE_STATIC_NAME_STR(lsApply, TXT("hub; common; apply"));
DEFINE_STATIC_NAME_STR(lsCancel, TXT("hub; common; cancel"));
DEFINE_STATIC_NAME_STR(lsBack, TXT("hub; common; back"));

DEFINE_STATIC_NAME_STR(lsDevicesControllers, TXT("hub; opt; devices; controllers"));
DEFINE_STATIC_NAME_STR(lsDevicesOWO, TXT("hub; opt; devices; owo"));
DEFINE_STATIC_NAME_STR(lsDevicesBhaptics, TXT("hub; opt; devices; bhaptics"));

DEFINE_STATIC_NAME_STR(lsDevicesControllersVRHapticFeedback, TXT("hub; opt; devices; controllers vr haptic feedback"));

DEFINE_STATIC_NAME_STR(lsDevicesOWOAddress, TXT("hub; opt; devices; owo address"));
DEFINE_STATIC_NAME_STR(lsDevicesOWOAddressMissing, TXT("hub; opt; devices; owo address; missing"));
DEFINE_STATIC_NAME_STR(lsDevicesOWOPort, TXT("hub; opt; devices; owo port"));

DEFINE_STATIC_NAME_STR(lsDevicesBhapticsName, TXT("hub; opt; devices; bhaptics name"));
DEFINE_STATIC_NAME_STR(lsDevicesBhapticsAddress, TXT("hub; opt; devices; bhaptics address"));
DEFINE_STATIC_NAME_STR(lsDevicesBhapticsPosition, TXT("hub; opt; devices; bhaptics position"));
DEFINE_STATIC_NAME_STR(lsDevicesBhapticsPositionToggle, TXT("hub; opt; devices; bhaptics position; toggle"));
DEFINE_STATIC_NAME_STR(lsDevicesBhapticsBattery, TXT("hub; opt; devices; bhaptics battery"));
DEFINE_STATIC_NAME_STR(lsDevicesBhapticsConnection, TXT("hub; opt; devices; bhaptics connection"));
DEFINE_STATIC_NAME_STR(lsDevicesBhapticsDisconnected, TXT("hub; opt; devices; bhaptics connection; disconnected"));
DEFINE_STATIC_NAME_STR(lsDevicesBhapticsConnecting, TXT("hub; opt; devices; bhaptics connection; connecting"));
DEFINE_STATIC_NAME_STR(lsDevicesBhapticsConnected, TXT("hub; opt; devices; bhaptics connection; connected"));
DEFINE_STATIC_NAME_STR(lsDevicesBhapticsPaired, TXT("hub; opt; devices; bhaptics connection; paired"));

DEFINE_STATIC_NAME_STR(lsDevicesBhapticsNoPlayerDetected, TXT("hub; opt; devices; bhaptics; no player detected"));

//

DEFINE_STATIC_NAME_STR(lsDevicesStatus, TXT("hub; opt; devices; status"));
DEFINE_STATIC_NAME_STR(lsDevicesConnected, TXT("hub; opt; devices; connected"));
DEFINE_STATIC_NAME_STR(lsDevicesConnecting, TXT("hub; opt; devices; connecting"));
DEFINE_STATIC_NAME_STR(lsDevicesNotConnected, TXT("hub; opt; devices; not connected"));
DEFINE_STATIC_NAME_STR(lsDevicesTest, TXT("hub; opt; devices; test"));
DEFINE_STATIC_NAME_STR(lsDevicesScan, TXT("hub; opt; devices; scan"));
DEFINE_STATIC_NAME_STR(lsDevicesScanning, TXT("hub; opt; devices; scanning"));
DEFINE_STATIC_NAME_STR(lsDevicesAccept, TXT("hub; opt; devices; accept"));
DEFINE_STATIC_NAME_STR(lsDevicesDisable, TXT("hub; opt; devices; disable"));

DEFINE_STATIC_NAME_STR(lsSoundMainVolume, TXT("hub; opt; sound; main volume"));
DEFINE_STATIC_NAME_STR(lsSoundSoundVolume, TXT("hub; opt; sound; sound volume"));
DEFINE_STATIC_NAME_STR(lsSoundEnvironmentVolume, TXT("hub; opt; sound; environment volume"));
DEFINE_STATIC_NAME_STR(lsSoundUIVolume, TXT("hub; opt; sound; ui volume"));
DEFINE_STATIC_NAME_STR(lsSoundMusicVolume, TXT("hub; opt; sound; music volume"));
DEFINE_STATIC_NAME_STR(lsSoundVoiceoverVolume, TXT("hub; opt; sound; voiceover volume"));
DEFINE_STATIC_NAME_STR(lsSoundPilgrimOverlayVolume, TXT("hub; opt; sound; pilgrim overlay volume"));
DEFINE_STATIC_NAME_STR(lsSoundSubtitles, TXT("hub; opt; sound; subtitles"));
DEFINE_STATIC_NAME_STR(lsSoundSubtitlesOff, TXT("hub; opt; sound; subtitles off"));
DEFINE_STATIC_NAME_STR(lsSoundSubtitlesOn, TXT("hub; opt; sound; subtitles on"));
DEFINE_STATIC_NAME_STR(lsSoundSubtitlesScale, TXT("hub; opt; sound; subtitles scale"));
DEFINE_STATIC_NAME_STR(lsSoundNarrativeVoiceover, TXT("hub; opt; sound; narrative voiceover"));
DEFINE_STATIC_NAME_STR(lsSoundNarrativeVoiceoverOff, TXT("hub; opt; sound; narrative voiceover off"));
DEFINE_STATIC_NAME_STR(lsSoundNarrativeVoiceoverOn, TXT("hub; opt; sound; narrative voiceover on"));
DEFINE_STATIC_NAME_STR(lsSoundPilgrimOverlayVoiceover, TXT("hub; opt; sound; pilgrim overlay voiceover"));
DEFINE_STATIC_NAME_STR(lsSoundPilgrimOverlayVoiceoverOff, TXT("hub; opt; sound; pilgrim overlay voiceover off"));
DEFINE_STATIC_NAME_STR(lsSoundPilgrimOverlayVoiceoverOn, TXT("hub; opt; sound; pilgrim overlay voiceover on"));

DEFINE_STATIC_NAME_STR(lsAestheticsGeneral, TXT("hub; opt; aesthetics; general"));
DEFINE_STATIC_NAME_STR(lsAestheticsGeneralNormal, TXT("hub; opt; aesthetics; general; normal"));
DEFINE_STATIC_NAME_STR(lsAestheticsGeneralRetro, TXT("hub; opt; aesthetics; general; retro"));
DEFINE_STATIC_NAME_STR(lsAestheticsDitheredTransparency, TXT("hub; opt; aesthetics; dithered transparency"));
DEFINE_STATIC_NAME_STR(lsAestheticsDitheredTransparencyOn, TXT("hub; opt; aesthetics; dithered transparency; on"));
DEFINE_STATIC_NAME_STR(lsAestheticsDitheredTransparencyOff, TXT("hub; opt; aesthetics; dithered transparency; off"));
DEFINE_STATIC_NAME_STR(lsAestheticsQuestion, TXT("hub; opt; aesthetics; question"));

DEFINE_STATIC_NAME_STR(lsVideoMSAA, TXT("hub; opt; video; msaa"));
DEFINE_STATIC_NAME_STR(lsVideoMSAAOff, TXT("hub; opt; video; msaa off"));
DEFINE_STATIC_NAME_STR(lsVideoFXAA, TXT("hub; opt; video; fxaa"));
DEFINE_STATIC_NAME_STR(lsVideoFXAAOff, TXT("hub; opt; video; fxaa off"));
DEFINE_STATIC_NAME_STR(lsVideoFXAAOn, TXT("hub; opt; video; fxaa on"));
DEFINE_STATIC_NAME_STR(lsVideoBloom, TXT("hub; opt; video; bloom"));
DEFINE_STATIC_NAME_STR(lsVideoBloomOff, TXT("hub; opt; video; bloom off"));
DEFINE_STATIC_NAME_STR(lsVideoBloomOn, TXT("hub; opt; video; bloom on"));
DEFINE_STATIC_NAME_STR(lsVideoExtraWindows, TXT("hub; opt; video; extra windows"));
DEFINE_STATIC_NAME_STR(lsVideoExtraWindowsOff, TXT("hub; opt; video; extra windows off"));
DEFINE_STATIC_NAME_STR(lsVideoExtraWindowsOn, TXT("hub; opt; video; extra windows on"));
DEFINE_STATIC_NAME_STR(lsVideoExtraDoors, TXT("hub; opt; video; extra doors"));
DEFINE_STATIC_NAME_STR(lsVideoExtraDoorsOff, TXT("hub; opt; video; extra doors off"));
DEFINE_STATIC_NAME_STR(lsVideoExtraDoorsOn, TXT("hub; opt; video; extra doors on"));
DEFINE_STATIC_NAME_STR(lsVideoDisplay, TXT("hub; opt; video; display"));
DEFINE_STATIC_NAME_STR(lsVideoResolution, TXT("hub; opt; video; resolution"));
DEFINE_STATIC_NAME_STR(lsVideoVRResolutionCoef, TXT("hub; opt; video; vr resolution coef"));
DEFINE_STATIC_NAME_STR(lsVideoAspectRatioCoef, TXT("hub; opt; video; aspect ratio coef"));
DEFINE_STATIC_NAME_STR(lsVideoAspectRatioCoefNone, TXT("hub; opt; video; aspect ratio coef; none"));
DEFINE_STATIC_NAME_STR(lsVideoDirectlyToVR, TXT("hub; opt; video; directly to vr"));
DEFINE_STATIC_NAME_STR(lsVideoDirectlyToVROff, TXT("hub; opt; video; directly to vr off"));
DEFINE_STATIC_NAME_STR(lsVideoDirectlyToVROn, TXT("hub; opt; video; directly to vr on"));
DEFINE_STATIC_NAME_STR(lsVideoVRFoveatedRenderingLevel, TXT("hub; opt; video; vr foveated rendering level"));
DEFINE_STATIC_NAME_STR(lsVideoVRFoveatedRenderingMaxDepth, TXT("hub; opt; video; vr foveated rendering max depth"));
DEFINE_STATIC_NAME_STR(lsVideoVRFoveatedRenderingTest, TXT("hub; opt; video; vr foveated rendering test"));

DEFINE_STATIC_NAME_STR(lsVideoAutoScale, TXT("hub; opt; video; auto scale"));
DEFINE_STATIC_NAME_STR(lsVideoAutoScaleOff, TXT("hub; opt; video; auto scale off"));
DEFINE_STATIC_NAME_STR(lsVideoAutoScaleOn, TXT("hub; opt; video; auto scale on"));
DEFINE_STATIC_NAME_STR(lsVideoFullScreen, TXT("hub; opt; video; full screen"));
DEFINE_STATIC_NAME_STR(lsVideoFullScreenNo, TXT("hub; opt; video; full screen no"));
DEFINE_STATIC_NAME_STR(lsVideoFullScreenWindowScaled, TXT("hub; opt; video; full screen window scaled"));
DEFINE_STATIC_NAME_STR(lsVideoFullScreenYes, TXT("hub; opt; video; full screen yes"));
DEFINE_STATIC_NAME_STR(lsVideoDisplayOnScreenLayout, TXT("hub; opt; video; display on screen layout"));
DEFINE_STATIC_NAME_STR(lsVideoDisplayOnScreen, TXT("hub; opt; video; display on screen"));
DEFINE_STATIC_NAME_STR(lsVideoDisplayOnScreenNo, TXT("hub; opt; video; display on screen; no"));
DEFINE_STATIC_NAME_STR(lsVideoDisplayOnScreenYes, TXT("hub; opt; video; display on screen; yes"));
DEFINE_STATIC_NAME_STR(lsVideoToDefaults, TXT("hub; opt; video; reset to defaults"));
//
DEFINE_STATIC_NAME_STR(lsVideoSettingsChanged, TXT("hub; opt; video; settings changed"));

DEFINE_STATIC_NAME_STR(lsVideoLayoutMain, TXT("hub; opt; video; scr.lay; main"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutSecondary, TXT("hub; opt; video; scr.lay; secondary"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutShow, TXT("hub; opt; video; scr.lay; show ext"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutShowOff, TXT("hub; opt; video; scr.lay; show ext; off"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutShowOn, TXT("hub; opt; video; scr.lay; show ext; on"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutSize, TXT("hub; opt; video; scr.lay; size"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutZoom, TXT("hub; opt; video; scr.lay; zoom"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutZoomOff, TXT("hub; opt; video; scr.lay; zoom; off"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutZoomOn, TXT("hub; opt; video; scr.lay; zoom; on"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutPlacement, TXT("hub; opt; video; scr.lay; placement"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutPlacementVertical, TXT("hub; opt; video; scr.lay; placement vertical"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutPlacementVerticalBottom, TXT("hub; opt; video; scr.lay; placement v bottom"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutPlacementVerticalTop, TXT("hub; opt; video; scr.lay; placement v top"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutPlacementHorizontal, TXT("hub; opt; video; scr.lay; placement horizontal"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutPlacementHorizontalLeft, TXT("hub; opt; video; scr.lay; placement h left"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutPlacementHorizontalRight, TXT("hub; opt; video; scr.lay; placement h right"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutPIP, TXT("hub; opt; video; scr.lay; pip"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutPIPOff, TXT("hub; opt; video; scr.lay; pip; off"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutPIPOn, TXT("hub; opt; video; scr.lay; pip; on"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutPIPSwap, TXT("hub; opt; video; scr.lay; pip swap"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutPIPSwapOff, TXT("hub; opt; video; scr.lay; pip swap; off"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutPIPSwapOn, TXT("hub; opt; video; scr.lay; pip swap; on"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutBoundary, TXT("hub; opt; video; scr.lay; boundary"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutBoundaryOff, TXT("hub; opt; video; scr.lay; boundary; off"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutBoundaryFloorOnly, TXT("hub; opt; video; scr.lay; boundary; floor"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutBoundaryBackWalls, TXT("hub; opt; video; scr.lay; boundary; back walls"));
DEFINE_STATIC_NAME_STR(lsVideoLayoutBoundaryAllWalls, TXT("hub; opt; video; scr.lay; boundary; all walls"));

DEFINE_STATIC_NAME_STR(lsGameplayTurnCounter, TXT("hub; opt; gameplay; turn counter"));
DEFINE_STATIC_NAME_STR(lsGameplayTurnCounterOff, TXT("hub; opt; gameplay; turn counter; off"));
DEFINE_STATIC_NAME_STR(lsGameplayTurnCounterAllow, TXT("hub; opt; gameplay; turn counter; allow"));
DEFINE_STATIC_NAME_STR(lsGameplayTurnCounterOn, TXT("hub; opt; gameplay; turn counter; on"));
DEFINE_STATIC_NAME_STR(lsGameplayTurnCounterReset, TXT("hub; opt; gameplay; turn counter; reset"));
DEFINE_STATIC_NAME_STR(lsGameplayHitIndicatorType, TXT("hub; opt; gameplay; hit indicator type"));
DEFINE_STATIC_NAME_STR(lsGameplayHitIndicatorTypeOff, TXT("hub; opt; gameplay; hit indicator type; off"));
DEFINE_STATIC_NAME_STR(lsGameplayHitIndicatorTypeOn, TXT("hub; opt; gameplay; hit indicator type; on"));
DEFINE_STATIC_NAME_STR(lsGameplayHealthIndicator, TXT("hub; opt; gameplay; health indicator"));
DEFINE_STATIC_NAME_STR(lsGameplayHealthIndicatorOff, TXT("hub; opt; gameplay; health indicator; off"));
DEFINE_STATIC_NAME_STR(lsGameplayHealthIndicatorOn, TXT("hub; opt; gameplay; health indicator; on"));
DEFINE_STATIC_NAME_STR(lsGameplayHealthIndicatorIntensity, TXT("hub; opt; gameplay; health indicator intensity"));
DEFINE_STATIC_NAME_STR(lsGameplayHitIndicatorIntensity, TXT("hub; opt; gameplay; hit indicator intensity"));
DEFINE_STATIC_NAME_STR(lsGameplayHighlightInteractives, TXT("hub; opt; gameplay; highlight interactives"));
DEFINE_STATIC_NAME_STR(lsGameplayHighlightInteractivesOff, TXT("hub; opt; gameplay; highlight interactives; off"));
DEFINE_STATIC_NAME_STR(lsGameplayHighlightInteractivesOn, TXT("hub; opt; gameplay; highlight interactives; on"));
DEFINE_STATIC_NAME_STR(lsGameplayShowInHandEquipmentStatsAtGlance, TXT("hub; opt; gameplay; show stats at glance"));
DEFINE_STATIC_NAME_STR(lsGameplayShowInHandEquipmentStatsAtGlanceOff, TXT("hub; opt; gameplay; show stats at glance; off"));
DEFINE_STATIC_NAME_STR(lsGameplayShowInHandEquipmentStatsAtGlanceOn, TXT("hub; opt; gameplay; show stats at glance; on"));
DEFINE_STATIC_NAME_STR(lsGameplayPocketsVerticalAdjustment, TXT("hub; opt; gameplay; pockets vertical adjustment"));
DEFINE_STATIC_NAME_STR(lsGameplayKeepInWorld, TXT("hub; opt; gameplay; keep in world"));
DEFINE_STATIC_NAME_STR(lsGameplayKeepInWorldOff, TXT("hub; opt; gameplay; keep in world; off"));
DEFINE_STATIC_NAME_STR(lsGameplayKeepInWorldOn, TXT("hub; opt; gameplay; keep in world; on"));
DEFINE_STATIC_NAME_STR(lsGameplayKeepInWorldSensitivity, TXT("hub; opt; gameplay; keep in world; sensitivity"));
DEFINE_STATIC_NAME_STR(lsGameplayShowSavingIcon, TXT("hub; opt; gameplay; show saving icon"));
DEFINE_STATIC_NAME_STR(lsGameplayShowSavingIconOff, TXT("hub; opt; gameplay; show saving icon; off"));
DEFINE_STATIC_NAME_STR(lsGameplayShowSavingIconOn, TXT("hub; opt; gameplay; show saving icon; on"));
DEFINE_STATIC_NAME_STR(lsGameplayMapAvailable, TXT("hub; opt; gameplay; map available"));
DEFINE_STATIC_NAME_STR(lsGameplayMapAvailableOff, TXT("hub; opt; gameplay; map available; off"));
DEFINE_STATIC_NAME_STR(lsGameplayMapAvailableOn, TXT("hub; opt; gameplay; map available; on"));
DEFINE_STATIC_NAME_STR(lsGameplayDoorDirections, TXT("hub; opt; gameplay; door directions"));
DEFINE_STATIC_NAME_STR(lsGameplayDoorDirectionsAuto, TXT("hub; opt; gameplay; door directions; auto"));
DEFINE_STATIC_NAME_STR(lsGameplayDoorDirectionsAbove, TXT("hub; opt; gameplay; door directions; above"));
DEFINE_STATIC_NAME_STR(lsGameplayDoorDirectionsAtEyeLevel, TXT("hub; opt; gameplay; door directions; at eye-level"));
DEFINE_STATIC_NAME_STR(lsGameplayPilgrimOverlayLockedToHead, TXT("hub; opt; gameplay; pi.ov locked to head"));
DEFINE_STATIC_NAME_STR(lsGameplayPilgrimOverlayLockedToHeadOff, TXT("hub; opt; gameplay; pi.ov locked to head; off"));
DEFINE_STATIC_NAME_STR(lsGameplayPilgrimOverlayLockedToHeadOn, TXT("hub; opt; gameplay; pi.ov locked to head; on"));
DEFINE_STATIC_NAME_STR(lsGameplayPilgrimOverlayLockedToHeadAllow, TXT("hub; opt; gameplay; pi.ov locked to head; Allow"));
DEFINE_STATIC_NAME_STR(lsGameplayFollowGuidance, TXT("hub; opt; gameplay; follow guidance"));
DEFINE_STATIC_NAME_STR(lsGameplayFollowGuidanceDisallow, TXT("hub; opt; gameplay; follow guidance; disallow"));
DEFINE_STATIC_NAME_STR(lsGameplayFollowGuidanceAllow, TXT("hub; opt; gameplay; follow guidance; allow"));
DEFINE_STATIC_NAME_STR(lsGameplayToDefaults, TXT("hub; opt; gameplay; reset to defaults"));
DEFINE_STATIC_NAME_STR(lsGameplaySights, TXT("hub; opt; gameplay; sights"));
DEFINE_STATIC_NAME_STR(lsGameplaySightsOuterRingRadius, TXT("hub; opt; gameplay; sights; outer ring radius"));

DEFINE_STATIC_NAME_STR(lsComfortVignetteForMovement, TXT("hub; opt; comfort; vignette for movement"));
DEFINE_STATIC_NAME_STR(lsComfortVignetteForMovementOff, TXT("hub; opt; comfort; vignette for movement; off"));
DEFINE_STATIC_NAME_STR(lsComfortVignetteForMovementOn, TXT("hub; opt; comfort; vignette for movement; on"));
DEFINE_STATIC_NAME_STR(lsComfortVignetteForMovementStrength, TXT("hub; opt; comfort; vignette for movement; strength"));
DEFINE_STATIC_NAME_STR(lsComfortCartsTopSpeed, TXT("hub; opt; comfort; carts top speed"));
DEFINE_STATIC_NAME_STR(lsComfortCartsTopSpeedVerySlow, TXT("hub; opt; comfort; carts top speed; very slow"));
DEFINE_STATIC_NAME_STR(lsComfortCartsTopSpeedSlow, TXT("hub; opt; comfort; carts top speed; slow"));
DEFINE_STATIC_NAME_STR(lsComfortCartsTopSpeedUnlimited, TXT("hub; opt; comfort; carts top speed; unlimited"));
DEFINE_STATIC_NAME_STR(lsComfortCartsSpeedPct, TXT("hub; opt; comfort; carts speed pct"));
DEFINE_STATIC_NAME_STR(lsComfortCartsSpeedPctVerySlow, TXT("hub; opt; comfort; carts speed pct; very slow"));
DEFINE_STATIC_NAME_STR(lsComfortCartsSpeedPctSlower, TXT("hub; opt; comfort; carts speed pct; slower"));
DEFINE_STATIC_NAME_STR(lsComfortCartsSpeedPctSlow, TXT("hub; opt; comfort; carts speed pct; slow"));
DEFINE_STATIC_NAME_STR(lsComfortCartsSpeedPctNormal, TXT("hub; opt; comfort; carts speed pct; normal"));
DEFINE_STATIC_NAME_STR(lsComfortFlickeringLights, TXT("hub; opt; comfort; flickering lights"));
DEFINE_STATIC_NAME_STR(lsComfortFlickeringLightsDisallow, TXT("hub; opt; comfort; flickering lights; disallow"));
DEFINE_STATIC_NAME_STR(lsComfortFlickeringLightsAllow, TXT("hub; opt; comfort; flickering lights; allow"));
DEFINE_STATIC_NAME_STR(lsComfortCameraRumble, TXT("hub; opt; comfort; camera rumble"));
DEFINE_STATIC_NAME_STR(lsComfortCameraRumbleDisallow, TXT("hub; opt; comfort; camera rumble; disallow"));
DEFINE_STATIC_NAME_STR(lsComfortCameraRumbleAllow, TXT("hub; opt; comfort; camera rumble; allow"));
DEFINE_STATIC_NAME_STR(lsComfortRotatingElevators, TXT("hub; opt; comfort; rotating elevators"));
DEFINE_STATIC_NAME_STR(lsComfortRotatingElevatorsDisallow, TXT("hub; opt; comfort; rotating elevators; disallow"));
DEFINE_STATIC_NAME_STR(lsComfortRotatingElevatorsAllow, TXT("hub; opt; comfort; rotating elevators; allow"));
DEFINE_STATIC_NAME_STR(lsComfortForcedEnvironmentMixPt, TXT("hub; opt; comfort; forced environment mix pt"));
DEFINE_STATIC_NAME_STR(lsComfortForcedEnvironmentMixPtOn, TXT("hub; opt; comfort; forced environment mix pt; on"));
DEFINE_STATIC_NAME_STR(lsComfortForcedEnvironmentMixPtOff, TXT("hub; opt; comfort; forced environment mix pt; off"));
DEFINE_STATIC_NAME_STR(lsComfortToDefaults, TXT("hub; opt; comfort; reset to defaults"));

DEFINE_STATIC_NAME_STR(lsControlsAutoMainEquipment, TXT("hub; opt; ctrls; auto main equipment"));
DEFINE_STATIC_NAME_STR(lsControlsAutoMainEquipmentNo, TXT("hub; opt; ctrls; auto main equipment; no"));
DEFINE_STATIC_NAME_STR(lsControlsAutoMainEquipmentDependsOnController, TXT("hub; opt; ctrls; auto main equipment; depends on controller"));
DEFINE_STATIC_NAME_STR(lsControlsAutoMainEquipmentYes, TXT("hub; opt; ctrls; auto main equipment; yes"));
DEFINE_STATIC_NAME_STR(lsControlsJoystickInputBlocked, TXT("hub; opt; ctrls; joystick input"));
DEFINE_STATIC_NAME_STR(lsControlsJoystickInputBlockedOn, TXT("hub; opt; ctrls; joystick input; blocked"));
DEFINE_STATIC_NAME_STR(lsControlsJoystickInputBlockedOff, TXT("hub; opt; ctrls; joystick input; allowed"));
DEFINE_STATIC_NAME_STR(lsControlsJoystickSwap, TXT("hub; opt; ctrls; joystick swap"));
DEFINE_STATIC_NAME_STR(lsControlsJoystickSwapOn, TXT("hub; opt; ctrls; joystick swap; on"));
DEFINE_STATIC_NAME_STR(lsControlsJoystickSwapOff, TXT("hub; opt; ctrls; joystick swap; off"));
DEFINE_STATIC_NAME_STR(lsControlsGameInputOptionOn, TXT("hub; opt; ctrls; game input option; on"));
DEFINE_STATIC_NAME_STR(lsControlsGameInputOptionOff, TXT("hub; opt; ctrls; game input option; off"));
DEFINE_STATIC_NAME_STR(lsControlsDominantHand, TXT("hub; opt; ctrls; dominant hand"));
DEFINE_STATIC_NAME_STR(lsControlsDominantHandAuto, TXT("hub; opt; ctrls; dominant hand; auto"));
DEFINE_STATIC_NAME_STR(lsControlsDominantHandRight, TXT("hub; opt; ctrls; dominant hand; right"));
DEFINE_STATIC_NAME_STR(lsControlsDominantHandLeft, TXT("hub; opt; ctrls; dominant hand; left"));
DEFINE_STATIC_NAME_STR(lsControlsVROffsetsOpenInfo, TXT("hub; opt; ctrls; vr.of; open info"));
DEFINE_STATIC_NAME_STR(lsControlsVROffsetsOpen, TXT("hub; opt; ctrls; vr.of; open"));
DEFINE_STATIC_NAME_STR(lsControlsVROffsets, TXT("hub; opt; ctrls; vr.of"));
DEFINE_STATIC_NAME_STR(lsControlsVROffsetsPitch, TXT("hub; opt; ctrls; vr.of; pitch"));
DEFINE_STATIC_NAME_STR(lsControlsVROffsetsYaw, TXT("hub; opt; ctrls; vr.of; yaw"));
DEFINE_STATIC_NAME_STR(lsControlsVROffsetsRoll, TXT("hub; opt; ctrls; vr.of; roll"));
DEFINE_STATIC_NAME_STR(lsControlsVROffsetsTest, TXT("hub; opt; ctrls; vr.of; test"));
DEFINE_STATIC_NAME_STR(lsControlsVROffsetsConfirm, TXT("hub; opt; ctrls; vr.of; confirm"));
	DEFINE_STATIC_NAME(secondsLeft);
DEFINE_STATIC_NAME_STR(lsControlsVROffsetsAngleValue, TXT("hub; opt; ctrls; vr.of; angle value"));

DEFINE_STATIC_NAME_STR(lsPlayAreaImmobileVR, TXT("hub; opt; pl.ar; im.vr"));
DEFINE_STATIC_NAME_STR(lsPlayAreaImmobileVRFalse, TXT("hub; opt; pl.ar; im.vr; false"));
DEFINE_STATIC_NAME_STR(lsPlayAreaImmobileVRTrue, TXT("hub; opt; pl.ar; im.vr; true"));
DEFINE_STATIC_NAME_STR(lsPlayAreaImmobileVRAuto, TXT("hub; opt; pl.ar; im.vr; auto"));
DEFINE_STATIC_NAME_STR(lsPlayAreaImmobileVRSize, TXT("hub; opt; pl.ar; im.vr size"));
DEFINE_STATIC_NAME_STR(lsPlayAreaImmobileVRTurning, TXT("hub; opt; pl.ar; im.vr turning"));
DEFINE_STATIC_NAME_STR(lsPlayAreaImmobileVRTurningSnap, TXT("hub; opt; pl.ar; im.vr turning; snap"));
DEFINE_STATIC_NAME_STR(lsPlayAreaImmobileVRTurningContinuous, TXT("hub; opt; pl.ar; im.vr turning; continuous"));
DEFINE_STATIC_NAME_STR(lsPlayAreaImmobileVRTurningSnapTurnAngle, TXT("hub; opt; pl.ar; im.vr turning; snap turn angle"));
DEFINE_STATIC_NAME_STR(lsPlayAreaImmobileVRTurningContinuousTurnSpeed, TXT("hub; opt; pl.ar; im.vr turning; continuous turn speed"));
DEFINE_STATIC_NAME_STR(lsPlayAreaImmobileVRTurningSnapTurnAngleValue, TXT("hub; opt; pl.ar; im.vr turning; snap turn angle; value"));
DEFINE_STATIC_NAME_STR(lsPlayAreaImmobileVRTurningContinuousTurnSpeedValue, TXT("hub; opt; pl.ar; im.vr turning; continuous turn speed; value"));
	DEFINE_STATIC_NAME(angle);
DEFINE_STATIC_NAME_STR(lsPlayAreaImmobileVRSpeed, TXT("hub; opt; pl.ar; im.vr speed"));
DEFINE_STATIC_NAME_STR(lsPlayAreaImmobileVRMovementRelativeTo, TXT("hub; opt; pl.ar; im.vr movement relative to"));
DEFINE_STATIC_NAME_STR(lsPlayAreaImmobileVRMovementRelativeToHead, TXT("hub; opt; pl.ar; im.vr movement relative to; head"));
DEFINE_STATIC_NAME_STR(lsPlayAreaImmobileVRMovementRelativeToLeftHand, TXT("hub; opt; pl.ar; im.vr movement relative to; left hand"));
DEFINE_STATIC_NAME_STR(lsPlayAreaImmobileVRMovementRelativeToRightHand, TXT("hub; opt; pl.ar; im.vr movement relative to; right hand"));
DEFINE_STATIC_NAME_STR(lsPlayAreaCurrentSize, TXT("hub; opt; pl.ar; current size"));
DEFINE_STATIC_NAME_STR(lsPlayAreaCurrentTileCount, TXT("hub; opt; pl.ar; current tile count"));
DEFINE_STATIC_NAME_STR(lsPlayAreaCurrentTileSize, TXT("hub; opt; pl.ar; current tile size"));
DEFINE_STATIC_NAME_STR(lsPlayAreaCurrentDoorHeight, TXT("hub; opt; pl.ar; current door height"));
DEFINE_STATIC_NAME_STR(lsPlayAreaCurrentScaleUpPlayer, TXT("hub; opt; pl.ar; current scale up player"));
DEFINE_STATIC_NAME_STR(lsPlayAreaCurrentAllowCrouch, TXT("hub; opt; pl.ar; current allow crouch"));
DEFINE_STATIC_NAME_STR(lsPlayAreaEnlarged, TXT("hub; opt; pl.ar; enlarged"));
DEFINE_STATIC_NAME_STR(lsPlayAreaBorder, TXT("hub; opt; pl.ar; border"));
DEFINE_STATIC_NAME_STR(lsPlayAreaHorizontalScaling, TXT("hub; opt; pl.ar; horizontal scaling"));
DEFINE_STATIC_NAME_STR(lsPlayAreaPreferredTileSize, TXT("hub; opt; pl.ar; preferred tile size"));
DEFINE_STATIC_NAME_STR(lsPlayAreaPreferredTileSizeSmallest, TXT("hub; opt; pl.ar; preferred tile size; smallest"));
DEFINE_STATIC_NAME_STR(lsPlayAreaPreferredTileSizeMedium, TXT("hub; opt; pl.ar; preferred tile size; medium"));
DEFINE_STATIC_NAME_STR(lsPlayAreaPreferredTileSizeLarge, TXT("hub; opt; pl.ar; preferred tile size; large"));
DEFINE_STATIC_NAME_STR(lsPlayAreaPreferredTileSizeLargest, TXT("hub; opt; pl.ar; preferred tile size; largest"));
DEFINE_STATIC_NAME_STR(lsPlayAreaPreferredTileSizeAuto, TXT("hub; opt; pl.ar; preferred tile size; auto"));
DEFINE_STATIC_NAME_STR(lsPlayAreaDoorHeight, TXT("hub; opt; pl.ar; door height"));
DEFINE_STATIC_NAME_STR(lsPlayAreaScaleUpPlayer, TXT("hub; opt; pl.ar; scale up player"));
DEFINE_STATIC_NAME_STR(lsPlayAreaScaleUpPlayerNo, TXT("hub; opt; pl.ar; scale up player; no"));
DEFINE_STATIC_NAME_STR(lsPlayAreaScaleUpPlayerYes, TXT("hub; opt; pl.ar; scale up player; yes"));
DEFINE_STATIC_NAME_STR(lsPlayAreaAllowCrouch, TXT("hub; opt; pl.ar; allow crouch"));
DEFINE_STATIC_NAME_STR(lsPlayAreaAllowCrouchAllow, TXT("hub; opt; pl.ar; allow crouch; allow"));
DEFINE_STATIC_NAME_STR(lsPlayAreaAllowCrouchDisallow, TXT("hub; opt; pl.ar; allow crouch; disallow"));
DEFINE_STATIC_NAME_STR(lsPlayAreaAllowCrouchAuto, TXT("hub; opt; pl.ar; allow crouch; auto"));
DEFINE_STATIC_NAME_STR(lsPlayAreaToDefaults, TXT("hub; opt; pl.ar; reset to defaults"));
//
DEFINE_STATIC_NAME_STR(lsPlayAreaCalibrate, TXT("hub; opt; pl.ar; calibrate"));
DEFINE_STATIC_NAME_STR(lsPlayAreaCalibratePlacement, TXT("hub; opt; pl.ar; calibrate; placement"));
DEFINE_STATIC_NAME_STR(lsPlayAreaCalibrateSize, TXT("hub; opt; pl.ar; calibrate; size"));
DEFINE_STATIC_NAME_STR(lsPlayAreaCalibrateReset, TXT("hub; opt; pl.ar; calibrate; reset"));
//
DEFINE_STATIC_NAME_STR(lsColourComponentRed, TXT("colour; component; red"));
DEFINE_STATIC_NAME_STR(lsColourComponentGreen, TXT("colour; component; green"));
DEFINE_STATIC_NAME_STR(lsColourComponentBlue, TXT("colour; component; blue"));

// ?
//
DEFINE_STATIC_NAME_STR(lsGeneralReportBugs, TXT("hub; opt; general; report bugs"));
DEFINE_STATIC_NAME_STR(lsGeneralReportBugsNo, TXT("hub; opt; general; report bugs; no"));
DEFINE_STATIC_NAME_STR(lsGeneralReportBugsYes, TXT("hub; opt; general; report bugs; yes"));
DEFINE_STATIC_NAME_STR(lsGeneralReportBugsAsk, TXT("hub; opt; general; report bugs; ask"));
DEFINE_STATIC_NAME_STR(lsGeneralShowTipsOnLoading, TXT("hub; opt; general; show tips on loading"));
DEFINE_STATIC_NAME_STR(lsGeneralShowTipsOnLoadingNo, TXT("hub; opt; general; show tips on loading; no"));
DEFINE_STATIC_NAME_STR(lsGeneralShowTipsOnLoadingYes, TXT("hub; opt; general; show tips on loading; yes"));
DEFINE_STATIC_NAME_STR(lsGeneralShowTipsDuringGame, TXT("hub; opt; general; show tips during game"));
DEFINE_STATIC_NAME_STR(lsGeneralShowTipsDuringGameNo, TXT("hub; opt; general; show tips during game; no"));
DEFINE_STATIC_NAME_STR(lsGeneralShowTipsDuringGameYes, TXT("hub; opt; general; show tips during game; yes"));
DEFINE_STATIC_NAME_STR(lsGeneralLanguage, TXT("hub; opt; general; language"));
DEFINE_STATIC_NAME_STR(lsGeneralLanguageAuto, TXT("hub; opt; general; language; auto"));
DEFINE_STATIC_NAME_STR(lsGeneralMeasurementSystem, TXT("hub; opt; general; measurement system"));
DEFINE_STATIC_NAME_STR(lsGeneralMeasurementSystemAuto, TXT("hub; opt; general; measurement system; auto"));
DEFINE_STATIC_NAME_STR(lsGeneralMeasurementSystemMetric, TXT("hub; opt; general; measurement system; metric"));
DEFINE_STATIC_NAME_STR(lsGeneralMeasurementSystemImperial, TXT("hub; opt; general; measurement system; imperial"));
DEFINE_STATIC_NAME_STR(lsGeneralAllowScreenShots, TXT("hub; opt; general; allow screen shots"));
DEFINE_STATIC_NAME_STR(lsGeneralAllowScreenShotsNo, TXT("hub; opt; general; allow screen shots; no"));
DEFINE_STATIC_NAME_STR(lsGeneralAllowScreenShotsYesSmall, TXT("hub; opt; general; allow screen shots; yes; small"));
DEFINE_STATIC_NAME_STR(lsGeneralAllowScreenShotsYes, TXT("hub; opt; general; allow screen shots; yes"));
DEFINE_STATIC_NAME_STR(lsGeneralShowGameplayInfo, TXT("hub; opt; general; show gameplay info"));
DEFINE_STATIC_NAME_STR(lsGeneralShowGameplayInfoNo, TXT("hub; opt; general; show gameplay info; no"));
DEFINE_STATIC_NAME_STR(lsGeneralShowGameplayInfoYes, TXT("hub; opt; general; show gameplay info; yes"));
DEFINE_STATIC_NAME_STR(lsGeneralShowRealTime, TXT("hub; opt; general; show real time"));
DEFINE_STATIC_NAME_STR(lsGeneralShowRealTimeNo, TXT("hub; opt; general; show real time; no"));
DEFINE_STATIC_NAME_STR(lsGeneralShowRealTimeYes, TXT("hub; opt; general; show real time; yes"));

// screens
DEFINE_STATIC_NAME(optionsMain);
DEFINE_STATIC_NAME(optionsSound);
DEFINE_STATIC_NAME(optionsDebug);
DEFINE_STATIC_NAME(optionsCheats);
DEFINE_STATIC_NAME(optionsDevices);
DEFINE_STATIC_NAME(optionsDevicesControllers);
DEFINE_STATIC_NAME(optionsDevicesOWO);
DEFINE_STATIC_NAME(optionsDevicesBhaptics);
DEFINE_STATIC_NAME(optionsAesthetics);
DEFINE_STATIC_NAME(optionsVideo);
DEFINE_STATIC_NAME(optionsVideoLayout);
DEFINE_STATIC_NAME(optionsGameplay);
DEFINE_STATIC_NAME(optionsComfort);
DEFINE_STATIC_NAME(optionsControls);
DEFINE_STATIC_NAME(optionsControlsVROffsets);
DEFINE_STATIC_NAME(optionsPlayArea);
DEFINE_STATIC_NAME(optionsPlayAreaCalibrate);
DEFINE_STATIC_NAME(optionsGeneral);
DEFINE_STATIC_NAME(optionsSetupFoveationParams);

// hardcoded channel volumes
DEFINE_STATIC_NAME(sound);
DEFINE_STATIC_NAME(environment);
DEFINE_STATIC_NAME(music);
DEFINE_STATIC_NAME(voiceover);
DEFINE_STATIC_NAME(pilgrimOverlay);
DEFINE_STATIC_NAME(ui);

// screens
DEFINE_STATIC_NAME(hubBackground);

// ids
DEFINE_STATIC_NAME(idCurrentSize);
DEFINE_STATIC_NAME(idCurrentTileCount);
DEFINE_STATIC_NAME(idCurrentTileCountInfo);
DEFINE_STATIC_NAME(idCurrentTileSize);
DEFINE_STATIC_NAME(idCurrentDoorHeight);
DEFINE_STATIC_NAME(idCurrentScaleUpPlayer);
DEFINE_STATIC_NAME(idCurrentAllowCrouch);
DEFINE_STATIC_NAME(idDevicesStatus);
DEFINE_STATIC_NAME(idDevicesList);
DEFINE_STATIC_NAME(idCalibrationOption);
DEFINE_STATIC_NAME(idCalibrationSize);
DEFINE_STATIC_NAME(idCalibrationHelp);
DEFINE_STATIC_NAME(idForcedEnvironmentMix);
DEFINE_STATIC_NAME(idForcedEnvironmentMixPt);
DEFINE_STATIC_NAME(idShowImmobileVR);
DEFINE_STATIC_NAME(idTestConfirmVROffsets);
DEFINE_STATIC_NAME(idResolutions);
DEFINE_STATIC_NAME(idSights);

// game input definition
DEFINE_STATIC_NAME(game);

// game input definition option
DEFINE_STATIC_NAME(cs_blockOverlayInfoOnHeadSideTouch);

// inputs
DEFINE_STATIC_NAME(calibrate);
DEFINE_STATIC_NAME(calibrateSwitch);
DEFINE_STATIC_NAME(shift);

// options
DEFINE_STATIC_NAME(bloom);
DEFINE_STATIC_NAME(noExtraWindows);
DEFINE_STATIC_NAME(noExtraDoors);

// shader options
DEFINE_STATIC_NAME(retro);
DEFINE_STATIC_NAME(noDitheredTransparency);

// haptics ops
DEFINE_STATIC_NAME(test);
DEFINE_STATIC_NAME(toggle);

// global references
DEFINE_STATIC_NAME_STR(grHelpOptionsPlayareaCalibrate, TXT("hub; help; options; playarea; calibrate"));

//

using namespace Loader;
using namespace HubScenes;

//

#ifdef BUILD_NON_RELEASE
struct DevVROffsets
{
	int vrOffsetPitch = 0;
	int vrOffsetYaw = 0;
	int vrOffsetRoll = 0;

	int vrOffsetX = 0;
	int vrOffsetY = 0;
	int vrOffsetZ = 0;

	int confirmedVROffsetPitch = 0;
	int confirmedVROffsetYaw = 0;
	int confirmedVROffsetRoll = 0;

	int confirmedVROffsetX = 0;
	int confirmedVROffsetY = 0;
	int confirmedVROffsetZ = 0;

	void read()
	{
		{
			auto& t = VR::Offsets::dev_access_hand(Hand::Right);
			auto tl = t.get_translation();
			auto tr = t.get_orientation().to_rotator();
			vrOffsetPitch = TypeConversions::Normal::f_i_closest(tr.pitch * 10.0f);
			vrOffsetYaw = TypeConversions::Normal::f_i_closest(tr.yaw * 10.0f);
			vrOffsetRoll = TypeConversions::Normal::f_i_closest(tr.roll * 10.0f);
			vrOffsetX = TypeConversions::Normal::f_i_closest(tl.x * 1000.0f);
			vrOffsetY = TypeConversions::Normal::f_i_closest(tl.y * 1000.0f);
			vrOffsetZ = TypeConversions::Normal::f_i_closest(tl.z * 1000.0f);

			confirmedVROffsetPitch = vrOffsetPitch;
			confirmedVROffsetYaw = vrOffsetYaw;
			confirmedVROffsetRoll = vrOffsetRoll;
			confirmedVROffsetX = vrOffsetX;
			confirmedVROffsetY = vrOffsetY;
			confirmedVROffsetZ = vrOffsetZ;
		}
	}

	void test()
	{
		write(false);
	}

	void confirm()
	{
		confirmedVROffsetPitch = vrOffsetPitch;
		confirmedVROffsetYaw = vrOffsetYaw;
		confirmedVROffsetRoll = vrOffsetRoll;
		confirmedVROffsetX = vrOffsetX;
		confirmedVROffsetY = vrOffsetY;
		confirmedVROffsetZ = vrOffsetZ;

		write(true);
	}

	void restore()
	{
		// write the confirmed ones
		write(true);
	}

	void write(bool _confirmed)
	{
		Vector3 tl;
		tl.x = (float)(_confirmed? confirmedVROffsetX : vrOffsetX) / 1000.0f;
		tl.y = (float)(_confirmed? confirmedVROffsetY : vrOffsetY) / 1000.0f;
		tl.z = (float)(_confirmed? confirmedVROffsetZ : vrOffsetZ) / 1000.0f;
		Rotator3 tr;
		tr.pitch = (float)(_confirmed ? confirmedVROffsetPitch : vrOffsetPitch) / 10.0f;
		tr.yaw = (float)(_confirmed ? confirmedVROffsetYaw : vrOffsetYaw) / 10.0f;
		tr.roll = (float)(_confirmed ? confirmedVROffsetRoll : vrOffsetRoll) / 10.0f;

		output(TXT("dev vr offsets"));
		output(TXT("<translation x=\"%.3f\" y=\"%.3f\" z=\"%.3f\"/>"), tl.x, tl.y, tl.z);
		output(TXT("<rotation pitch=\"%.3f\" yaw=\"%.3f\" roll=\"%.3f\"/>"), tr.pitch, tr.yaw, tr.roll);
			
		{
			auto & t = VR::Offsets::dev_access_hand(Hand::Right);
			t = Transform(tl, tr.to_quat());
		}
		{
			auto & t = VR::Offsets::dev_access_hand(Hand::Left);
			t = Transform(tl * Vector3(-1.0f, 1.0f, 1.0f), (tr * Rotator3(1.0f, -1.0f, -1.0f)).to_quat());
		}
	}

} devVROffsets;
#endif

//

#ifdef PHYSICAL_SENSATIONS_BHAPTICS_WINDOWS
	#define USING_PHYSICAL_SENSATIONS_BHAPTICS
#else
	#ifdef PHYSICAL_SENSATIONS_BHAPTICS_JAVA
		#define USING_PHYSICAL_SENSATIONS_BHAPTICS
	#endif
#endif
#ifdef USING_PHYSICAL_SENSATIONS_BHAPTICS
class BhapticsDeviceInfo
: public Loader::IHubDraggableData
{
	FAST_CAST_DECLARE(BhapticsDeviceInfo);
	FAST_CAST_BASE(Loader::IHubDraggableData);
	FAST_CAST_END();
public:
	String idInfo; // address or anything else
	String shortInfo;
	String name;
	String position;
	String address;
	int battery = 0;
	bool isConnected = false;
	bool isPaired = false;
};
REGISTER_FOR_FAST_CAST(BhapticsDeviceInfo);
#endif

//

bool should_appear_as_immobile_vr(int immobileVR)
{
	return immobileVR == Option::True ||
		(immobileVR == Option::Auto && MainConfig::global().should_be_immobile_vr_auto());
}

//

void Options::DevicesControllersOptions::read()
{
	auto const& psp = TeaForGodEmperor::PlayerSetup::get_current().get_preferences();
	vrHapticFeedback = (int)(psp.vrHapticFeedback * 100.0f);
}

void Options::DevicesControllersOptions::apply()
{
	auto& psp = TeaForGodEmperor::PlayerSetup::access_current().access_preferences();
	psp.vrHapticFeedback = ((float)vrHapticFeedback) / 100.0f;
	psp.apply();
}

//

void Options::DevicesOWOOptions::read()
{
	auto const & mc = MainConfig::global();

	owoAddress = mc.get_owo_address();
	owoPort = mc.get_owo_port();
}

void Options::DevicesOWOOptions::apply(bool _initialise)
{
#ifdef PHYSICAL_SENSATIONS_OWO
	auto& mc = MainConfig::access_global();
	mc.set_physical_sensations_system(PhysicalSensations::OWO::system_id());
	mc.set_owo_address(owoAddress);
	mc.set_owo_port(owoPort);
	if (_initialise)
	{
		PhysicalSensations::initialise();
		auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>();
		game->get_job_system()->do_asynchronous_off_system_job(PhysicalSensations::IPhysicalSensations::do_async_init_thread_func, nullptr);
	}
#endif
}

//

void Options::SoundOptions::read()
{
	auto const & sc = MainConfig::global().get_sound();

	mainVolume = (int)(sc.volume * 100.0f);

	if (auto* ch = sc.channels.get_existing(NAME(sound)))
	{
		soundVolume = (int)(ch->volume * 100.0f);
	}
	if (auto* ch = sc.channels.get_existing(NAME(environment)))
	{
		environmentVolume = (int)(ch->volume * 100.0f);
	}
	if (auto* ch = sc.channels.get_existing(NAME(ui)))
	{
		uiVolume = (int)(ch->volume * 100.0f);
	}
	if (auto* ch = sc.channels.get_existing(NAME(music)))
	{
		musicVolume = (int)(ch->volume * 100.0f);
	}
	if (auto* ch = sc.channels.get_existing(NAME(voiceover)))
	{
		voiceoverVolume = (int)(ch->volume * 100.0f);
	}
	if (auto* ch = sc.channels.get_existing(NAME(pilgrimOverlay)))
	{
		pilgrimOverlayVolume = (int)(ch->volume * 100.0f);
	}

	if (auto* ss = ::System::Sound::get())
	{
		ss->update_volumes();
		ss->update_audio_duck();
	}

	auto const & psp = TeaForGodEmperor::PlayerSetup::get_current().get_preferences();
	subtitles = psp.subtitles;
	subtitlesScale = (int)(psp.subtitlesScale * 100.0f);
	narrativeVoiceover = psp.narrativeVoiceovers == Option::On? 1 : 0;
	pilgrimOverlayVoiceover = psp.pilgrimOverlayVoiceovers == Option::On? 1 : 0;
}

void Options::SoundOptions::apply()
{
	apply_volumes();

	auto& psp = TeaForGodEmperor::PlayerSetup::access_current().access_preferences();
	psp.subtitles = subtitles;
	psp.subtitlesScale = (float)subtitlesScale / 100.0f;
	psp.narrativeVoiceovers = narrativeVoiceover? Option::On : Option::Off;
	psp.pilgrimOverlayVoiceovers = pilgrimOverlayVoiceover ? Option::On : Option::Off;
	psp.apply();
}

void Options::SoundOptions::apply_volumes()
{
	auto & sc = MainConfig::access_global().access_sound();

	sc.volume = ((float)mainVolume) / 100.0f;

	if (auto* ch = sc.channels.get_existing(NAME(sound)))
	{
		ch->volume = ((float)soundVolume) / 100.0f;
	}
	if (auto* ch = sc.channels.get_existing(NAME(environment)))
	{
		ch->volume = ((float)environmentVolume) / 100.0f;
	}
	if (auto* ch = sc.channels.get_existing(NAME(ui)))
	{
		ch->volume = ((float)uiVolume) / 100.0f;
	}
	if (auto* ch = sc.channels.get_existing(NAME(music)))
	{
		ch->volume = ((float)musicVolume) / 100.0f;
	}
	if (auto* ch = sc.channels.get_existing(NAME(voiceover)))
	{
		ch->volume = ((float)voiceoverVolume) / 100.0f;
	}
	if (auto* ch = sc.channels.get_existing(NAME(pilgrimOverlay)))
	{
		ch->volume = ((float)pilgrimOverlayVolume) / 100.0f;
	}

	if (auto* ss = ::System::Sound::get())
	{
		ss->update_volumes();
		ss->update_audio_duck();
	}
}

//

void Options::AestheticsOptions::read()
{
	style = 0;
	if (::System::Core::get_system_tags().get_tag(NAME(retro)))
	{
		style = 1;
	}
	ditheredTransparency = 1;
	if (::System::Core::get_system_tags().get_tag(NAME(noDitheredTransparency)))
	{
		ditheredTransparency = 0;
	}
}

void Options::AestheticsOptions::apply()
{
	::System::Core::clear_extra_requested_system_tags();
	::System::Core::set_extra_requested_system_tags(String::printf(TXT("retro=%i"), style == 1? 1 : 0).to_char(), true);
	::System::Core::set_extra_requested_system_tags(String::printf(TXT("noDitheredTransparency=%i"), ditheredTransparency? 0 : 1).to_char(), true);
	::System::Core::changed_extra_requested_system_tags(); // mark as system tags have changed, so when we save the file, we're not saving some of the options that should be reset to defaults
	TeaForGodEmperor::Game::get()->save_system_tags_file();
}

//

void Options::VideoOptions::read(::System::VideoConfig const* _vc)
{
	if (!_vc)
	{
		_vc = &MainConfig::global().get_video();
	}
	auto const& vc = *_vc;

	msaaSamples = vc.msaa ? vc.msaaSamples : 0;
	fxaa = vc.fxaa ? 1 : 0;
	bloom = vc.options.get_tag(NAME(bloom)) > 0.5f ? 1 : 0;
	extraWindows = vc.options.get_tag(NAME(noExtraWindows)) > 0.5f ? 0 : 1;
	extraDoors = vc.options.get_tag(NAME(noExtraDoors)) > 0.5f ? 0 : 1;
	displayIndex = vc.displayIndex;
	fullScreen = vc.fullScreen;
	vrResolutionCoef = (int)(vc.vrResolutionCoef * 100.0f);
	aspectRatioCoef = TypeConversions::Normal::f_i_closest(log2f(vc.aspectRatioCoef) * 100.0f);
	directlyToVR = vc.directlyToVR ? 1 : 0;
#ifdef USE_LESS_PIXEL_FOR_FOVEATED_RENDERING
	vrFoveatedRenderingLevel = TypeConversions::Normal::f_i_closest(vc.vrFoveatedRenderingLevel * 100.0f);
	vrFoveatedRenderingMaxDepth = clamp(vc.vrFoveatedRenderingMaxDepth, 1, ::System::FoveatedRenderingSetup::get_use_less_pixel_for_foveated_rendering_max_depth());
#else
	if (auto * vr = VR::IVR::get())
	{
		auto vraf = vr->get_available_functionality();

		if (!vraf.directToVR)
		{
			directlyToVR = false;
		}
		vrFoveatedRenderingLevel = vraf.convert_foveated_rendering_config_to_level(vc.vrFoveatedRenderingLevel);
		vrFoveatedRenderingMaxDepth = vc.vrFoveatedRenderingMaxDepth;
	}
	else
	{
		vrFoveatedRenderingLevel = 0;
		vrFoveatedRenderingMaxDepth = 1;
	}
#endif

	allowAutoScale = vc.allowAutoAdjustForVR ? 1 : 0;
	displayOnScreen = !vc.skipDisplayBufferForVR;
	resolution = vc.resolution;

	if (TeaForGodEmperor::GameConfig const* gc = TeaForGodEmperor::GameConfig::get_as<TeaForGodEmperor::GameConfig>())
	{
		auto const& svi = gc->get_secondary_view_info();
		layoutShowSecondary = svi.show ? 1 : 0;
		layoutPIP = svi.pip ? 1 : 0;
		layoutPIPSwap = svi.pipSwap ? 1 : 0;
		layoutMainSize = TypeConversions::Normal::f_i_closest(svi.mainSize * 100.0f);
		layoutMainZoom = svi.mainZoom ? 1 : 0;
		layoutMainPlacementX = TypeConversions::Normal::f_i_closest(svi.mainAt.x * 100.0f);
		layoutMainPlacementY = TypeConversions::Normal::f_i_closest(svi.mainAt.y * 100.0f);
		layoutPIPSize = TypeConversions::Normal::f_i_closest(svi.pipSize * 100.0f);
		layoutSecondaryZoom = svi.secondaryZoom ? 1 : 0;
		layoutSecondaryPlacementX = TypeConversions::Normal::f_i_closest(svi.secondaryAt.x * 100.0f);
		layoutSecondaryPlacementY = TypeConversions::Normal::f_i_closest(svi.secondaryAt.y * 100.0f);

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
		auto const& evi = gc->get_external_view_info();
		layoutShowBoundary = evi.showBoundary;
#endif
	}
}

void Options::VideoOptions::update_resolution_index(Array<VectorInt2> const & _resolutions)
{
	resolutionIndex = NONE;
	for_count(int, i, _resolutions.get_size())
	{
		if (resolution == _resolutions[i])
		{
			resolutionIndex = i;
			return;
		}
	}
	int closestDist = 0;
	for_count(int, i, _resolutions.get_size())
	{
		int dist = abs(resolution.x - _resolutions[i].x) + abs(resolution.y - _resolutions[i].y);
		if (resolutionIndex == NONE || dist < closestDist)
		{
			resolutionIndex = i;
			closestDist = dist;
		}
	}
}

void Options::VideoOptions::update_resolution(Array<VectorInt2> const & _resolutions)
{
	if (_resolutions.is_index_valid(resolutionIndex))
	{
		resolution = _resolutions[resolutionIndex];
	}
}

void Options::VideoOptions::apply()
{
	auto & vc = MainConfig::access_global().access_video();

	vc.msaa = msaaSamples != 0;
	if (vc.msaa)
	{
		vc.msaaSamples = msaaSamples;
	}
	if (directlyToVR)
	{
		fxaa = 0;
		bloom = 0;
		allowAutoScale = 0;
	}
	vc.fxaa = fxaa != 0;
	vc.options.set_tag(NAME(bloom), bloom != 0? 1.0f : 0.0f);
	vc.options.set_tag(NAME(noExtraWindows), extraWindows != 1? 0.0f : 1.0f);
	vc.options.set_tag(NAME(noExtraDoors), extraDoors != 1? 0.0f : 1.0f);
	vc.displayIndex = displayIndex;
	vc.fullScreen = (System::FullScreen::Type)fullScreen;
	vc.vrResolutionCoef = (float)vrResolutionCoef / 100.0f;
	vc.aspectRatioCoef = pow(2.0f, (float)aspectRatioCoef / 100.0f);
	vc.directlyToVR = directlyToVR != 0;
#ifdef USE_LESS_PIXEL_FOR_FOVEATED_RENDERING
	vc.vrFoveatedRenderingLevel = (float)vrFoveatedRenderingLevel / 100.0f;
	vc.vrFoveatedRenderingMaxDepth = vc.vrFoveatedRenderingMaxDepth;
#else
	if (auto* vr = VR::IVR::get())
	{
		auto vraf = vr->get_available_functionality();

		vc.vrFoveatedRenderingLevel = vraf.convert_foveated_rendering_level_to_config(vrFoveatedRenderingLevel);
		if (vraf.foveatedRenderingMaxDepth)
		{
			vc.vrFoveatedRenderingMaxDepth = vrFoveatedRenderingMaxDepth;
		}
	}
#endif
	vc.allowAutoAdjustForVR = allowAutoScale != 0;
	vc.skipDisplayBufferForVR = !displayOnScreen;
	vc.resolution = resolution;
	vc.resolutionProvided = resolution; // to save it!

	if (TeaForGodEmperor::GameConfig * gc = TeaForGodEmperor::GameConfig::get_as<TeaForGodEmperor::GameConfig>())
	{
		auto & svi = gc->access_secondary_view_info();
		svi.show = layoutShowSecondary != 0;
		svi.pip = layoutPIP != 0;
		svi.pipSwap = layoutPIPSwap != 0;
		svi.mainSize = (float)layoutMainSize / 100.0f;
		svi.mainZoom = layoutMainZoom != 0;
		svi.mainAt.x = (float)layoutMainPlacementX / 100.0f;
		svi.mainAt.y = (float)layoutMainPlacementY / 100.0f;
		svi.pipSize = (float)layoutPIPSize / 100.0f;
		svi.secondaryZoom = layoutSecondaryZoom != 0;
		svi.secondaryAt.x = (float)layoutSecondaryPlacementX / 100.0f;
		svi.secondaryAt.y = (float)layoutSecondaryPlacementY / 100.0f;

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
		auto & evi = gc->access_external_view_info();
		evi.showBoundary = (TeaForGodEmperor::ExternalViewShowBoundary::Type)layoutShowBoundary;
#endif
	}
}

//

#define MAX_POCKETS_VERTICAL_ADJUSTMENT 0.2f

void Options::GameplayOptions::read(TeaForGodEmperor::PlayerPreferences const* _psp)
{
	auto const & psp = _psp? *_psp : TeaForGodEmperor::PlayerSetup::get_current().get_preferences();

	turnCounter = psp.turnCounter;
	hitIndicatorType = psp.hitIndicatorType;
	healthIndicator = psp.healthIndicator ? 1 : 0;
	healthIndicatorIntensity = (int)(psp.healthIndicatorIntensity * 100.0f);
	hitIndicatorIntensity = (int)(psp.hitIndicatorIntensity * 100.0f);
	highlightInteractives = psp.highlightInteractives ? 1 : 0;
	pocketsVerticalAdjustment = (int)(psp.pocketsVerticalAdjustment * 100.0f / MAX_POCKETS_VERTICAL_ADJUSTMENT);
	overlayInfoOnHeadSideTouch = psp.gameInputOptions.get_tag(NAME(cs_blockOverlayInfoOnHeadSideTouch)) == 0.0f; // inOptionsNegated
	showInHandEquipmentStatsAtGlance = psp.showInHandEquipmentStatsAtGlance == Option::On? 1 : 0;
	keepInWorld = psp.keepInWorld? 1 : 0;
	keepInWorldSensitivity = (int)(psp.keepInWorldSensitivity * 100.0f);
	showSavingIcon = psp.showSavingIcon? 1 : 0;
	mapAvailable = psp.mapAvailable ? 1 : 0;
	doorDirections = psp.doorDirectionsVisible;
	pilgrimOverlayLockedToHead = psp.pilgrimOverlayLockedToHead;
	followGuidance = psp.followGuidance;
	sightsOuterRingRadius = (int)(psp.sightsOuterRingRadius * 10000.0f);
	sightsColourRed = (int)(psp.sightsColour.r * 100.0f);
	sightsColourGreen = (int)(psp.sightsColour.g * 100.0f);
	sightsColourBlue = (int)(psp.sightsColour.b * 100.0f);
}

void Options::GameplayOptions::apply()
{
	auto & psp = TeaForGodEmperor::PlayerSetup::access_current().access_preferences();

	psp.turnCounter = (Option::Type)turnCounter;
	psp.hitIndicatorType = (TeaForGodEmperor::HitIndicatorType::Type)hitIndicatorType;
	psp.healthIndicator = healthIndicator;
	psp.healthIndicatorIntensity = (float)healthIndicatorIntensity / 100.0f;
	psp.hitIndicatorIntensity = (float)hitIndicatorIntensity / 100.0f;
	psp.highlightInteractives = highlightInteractives;
	psp.pocketsVerticalAdjustment = (float)pocketsVerticalAdjustment * MAX_POCKETS_VERTICAL_ADJUSTMENT / 100.0f;
	psp.showInHandEquipmentStatsAtGlance = showInHandEquipmentStatsAtGlance? Option::On : Option::Off;
	psp.keepInWorld = keepInWorld;
	psp.keepInWorldSensitivity = (float)keepInWorldSensitivity / 100.0f;
	psp.showSavingIcon = showSavingIcon;
	psp.mapAvailable = mapAvailable;
	if (psp.doorDirectionsVisible != doorDirections)
	{
		TeaForGodEmperor::RoomGenerationInfo::access().set_requires_door_regeneration();
	}
	psp.doorDirectionsVisible = (TeaForGodEmperor::DoorDirectionsVisible::Type)doorDirections;
	psp.pilgrimOverlayLockedToHead = (Option::Type)pilgrimOverlayLockedToHead;
	psp.followGuidance = (Option::Type)followGuidance;
	psp.sightsOuterRingRadius = (float)sightsOuterRingRadius / 10000.0f;
	psp.sightsColour.r = (float)sightsColourRed / 100.0f;
	psp.sightsColour.g = (float)sightsColourGreen / 100.0f;
	psp.sightsColour.b = (float)sightsColourBlue / 100.0f;
	psp.sightsColour.a = 1.0f;
	if (overlayInfoOnHeadSideTouch)
	{
		psp.gameInputOptions.remove_tag(NAME(cs_blockOverlayInfoOnHeadSideTouch)); // inOptionsNegated
	}
	else 
	{
		psp.gameInputOptions.set_tag(NAME(cs_blockOverlayInfoOnHeadSideTouch)); // inOptionsNegated
	}

	TeaForGodEmperor::GameSettings::get().inform_observers();
}

//

void Options::ComfortOptions::read(TeaForGodEmperor::PlayerPreferences const* _psp)
{
	auto const & psp = _psp ? *_psp : TeaForGodEmperor::PlayerSetup::get_current().get_preferences();

	forcedEnvironmentMix = psp.forcedEnvironmentMixPt.is_set()? 1 : 0;
	forcedEnvironmentMixPt = clamp((int)(psp.forcedEnvironmentMixPt.get(0.5f) * 24.0f * 60.0f), 0, 24 * 60);
	useVignetteForMovement = psp.useVignetteForMovement? 1 : 0;
	vignetteForMovementStrength = (int)(psp.vignetteForMovementStrength * 100.0f);
	cartsTopSpeed = (int)(psp.cartsTopSpeed * 100.0f);
	cartsSpeedPct = (int)(psp.cartsSpeedPct * 100.0f);
	allowCrouch = psp.allowCrouch;
	turnCounter = psp.turnCounter;
	flickeringLights = psp.flickeringLights;
	cameraRumble = psp.cameraRumble;
	rotatingElevators = psp.rotatingElevators;
}

void Options::ComfortOptions::apply()
{
	auto & psp = TeaForGodEmperor::PlayerSetup::access_current().access_preferences();

	if (forcedEnvironmentMix)
	{
		psp.forcedEnvironmentMixPt = clamp((float)forcedEnvironmentMixPt / (24.0f * 60.0f), 0.0f, 1.0f);
	}
	else
	{
		psp.forcedEnvironmentMixPt.clear();
	}
	psp.useVignetteForMovement = useVignetteForMovement != 0;
	psp.vignetteForMovementStrength = (float)vignetteForMovementStrength / 100.0f;
	psp.cartsTopSpeed = (float)cartsTopSpeed / 100.0f;
	psp.cartsSpeedPct = (float)cartsSpeedPct / 100.0f;
	psp.allowCrouch = (Option::Type)allowCrouch;
	psp.turnCounter = (Option::Type)turnCounter;
	psp.flickeringLights = (Option::Type)flickeringLights;
	psp.cameraRumble = (Option::Type)cameraRumble;
	psp.rotatingElevators = (Option::Type)rotatingElevators;
	psp.apply(); // instead of apply_to_vr_and_room_generation_info (scaling etc) as we apply flickering lights too

	TeaForGodEmperor::GameSettings::get().inform_observers();
}

//

void Options::ControlsOptions::read(TeaForGodEmperor::PlayerPreferences const* _psp)
{
	MainConfig::access_global().validate_immobile_vr();
	auto const& mc = MainConfig::global();
	auto const& psp = _psp ? *_psp : TeaForGodEmperor::PlayerSetup::get_current().get_preferences();

	autoMainEquipment = psp.autoMainEquipment.is_set() ? (psp.autoMainEquipment.get() ? 2 : 0) : 1;
	dominantHand = psp.rightHanded.is_set() ? (psp.rightHanded.get() ? 1 : -1) : 0;
	joystickInputBlocked = mc.is_joystick_input_blocked();
	joystickSwap = psp.swapJoysticks;
	immobileVR = mc.get_immobile_vr();

	immobileVRSnapTurn = psp.immobileVRSnapTurn? 1 : 0;
	immobileVRSnapTurnAngle = (int)psp.immobileVRSnapTurnAngle;
	immobileVRContinuousTurnSpeed = (int)psp.immobileVRContinuousTurnSpeed;
	immobileVRMovementRelativeTo = psp.immobileVRMovementRelativeTo;
	immobileVRSpeed = (int)(psp.immobileVRSpeed * 100.0f);

	if (auto* gid = Framework::GameInputDefinitions::get_definition(NAME(game))) // hardcoded
	{
		options.clear();
		for_every(opt, gid->get_options())
		{
			if (Framework::GameInput::check_condition(opt->condition.get()))
			{
				GIOption gio;
				gio.name = opt->name;
				gio.inOptionsNegated = opt->inOptionsNegated;
				gio.onOff = gio.inOptionsNegated? psp.gameInputOptions.get_tag(gio.name) == 0.0f : psp.gameInputOptions.get_tag(gio.name) != 0.0f;
				gio.locStrId = opt->get_loc_str_id();
				options.push_back(gio);
			}
		}
	}

	{
		MainConfig::VROffset vrOffset = mc.get_vr_offset(Hand::Right);

		vrOffsetPitch = TypeConversions::Normal::f_i_closest(vrOffset.orientation.pitch * 10.0f);
		vrOffsetYaw = TypeConversions::Normal::f_i_closest(vrOffset.orientation.yaw * 10.0f);
		vrOffsetRoll = TypeConversions::Normal::f_i_closest(vrOffset.orientation.roll * 10.0f);

		confirmedVROffsetPitch = vrOffsetPitch;
		confirmedVROffsetYaw = vrOffsetYaw;
		confirmedVROffsetRoll = vrOffsetRoll;
	}
}

void Options::ControlsOptions::apply()
{
	auto& mc = MainConfig::access_global();
	auto& psp = TeaForGodEmperor::PlayerSetup::access_current().access_preferences();

	if (autoMainEquipment == 1)
	{
		psp.autoMainEquipment.clear();
	}
	else
	{
		psp.autoMainEquipment = autoMainEquipment == 2;
	}
	if (dominantHand == 0)
	{
		psp.rightHanded.clear();
	}
	else
	{
		psp.rightHanded = dominantHand == 1;
	}

	mc.set_joystick_input_blocked(joystickInputBlocked != 0);
	psp.swapJoysticks = joystickSwap != 0;
	mc.set_immobile_vr((Option::Type)immobileVR);

	psp.immobileVRSnapTurn = immobileVRSnapTurn != 0;
	psp.immobileVRSnapTurnAngle = (float)immobileVRSnapTurnAngle;
	psp.immobileVRContinuousTurnSpeed = (float)immobileVRContinuousTurnSpeed;
	psp.immobileVRMovementRelativeTo = (TeaForGodEmperor::PlayerVRMovementRelativeTo::Type)immobileVRMovementRelativeTo;
	psp.immobileVRSpeed = (float)immobileVRSpeed / 100.0f;

	for_every(gio, options)
	{
		if (gio->inOptionsNegated)
		{
			if (gio->onOff)
			{
				psp.gameInputOptions.remove_tag(gio->name);
			}
			else
			{
				psp.gameInputOptions.set_tag(gio->name);
			}
		}
		else
		{
			if (gio->onOff)
			{
				psp.gameInputOptions.set_tag(gio->name);
			}
			else
			{
				psp.gameInputOptions.remove_tag(gio->name);
			}
		}
	}

	TeaForGodEmperor::GameSettings::get().inform_observers();

	psp.apply_to_vr_and_room_generation_info(); // will set vr scaling too (due to immobile vr)
	psp.apply_controls_to_main_config();

	apply_vr_offsets(true);
#ifdef BUILD_NON_RELEASE
	devVROffsets.confirm();
#endif
}

void Options::ControlsOptions::confirm_vr_offsets()
{
	confirmedVROffsetPitch = vrOffsetPitch;
	confirmedVROffsetYaw = vrOffsetYaw;
	confirmedVROffsetRoll = vrOffsetRoll;
	apply_vr_offsets(true);
#ifdef BUILD_NON_RELEASE
	devVROffsets.confirm();
#endif
}

void Options::ControlsOptions::apply_vr_offsets(bool _confirmed)
{
	auto& mc = MainConfig::access_global();

	Rotator3 leftVROffset;
	Rotator3 rightVROffset;

	Rotator3 vrOffset;

	vrOffset.pitch = (float)(_confirmed? confirmedVROffsetPitch : vrOffsetPitch) / 10.0f;
	vrOffset.yaw = (float)(_confirmed ? confirmedVROffsetYaw : vrOffsetYaw) / 10.0f;
	vrOffset.roll = (float)(_confirmed ? confirmedVROffsetRoll : vrOffsetRoll) / 10.0f;

	rightVROffset.pitch = vrOffset.pitch;
	rightVROffset.yaw = vrOffset.yaw;
	rightVROffset.roll = vrOffset.roll;

	leftVROffset.pitch = vrOffset.pitch;
	leftVROffset.yaw = -vrOffset.yaw;
	leftVROffset.roll = -vrOffset.roll;

	mc.set_vr_offsets(MainConfig::VROffset(leftVROffset), MainConfig::VROffset(rightVROffset));
}

//

MeasurementSystem::Type Options::PlayAreaOptions::get_measurement_system() const
{
	if (measurementSystem >= 0)
	{
		return (MeasurementSystem::Type)measurementSystem;
	}
	else
	{
		return TeaForGodEmperor::PlayerPreferences::get_auto_measurement_system();
	}
}

void Options::PlayAreaOptions::read()
{
	MainConfig::access_global().validate_immobile_vr();
	auto const & mc = MainConfig::global();
	auto const& psp = TeaForGodEmperor::PlayerSetup::get_current().get_preferences();
	immobileVR = mc.get_immobile_vr();
	immobileVRSize = (int)(mc.get_immobile_vr_size().x * 100.0f);
	measurementSystem = psp.measurementSystem.is_set() ? psp.measurementSystem.get() : -1;
	border = (int)round(mc.get_vr_map_margin().x * 100.0f);
	horizontalScaling = (int)round(mc.get_vr_horizontal_scaling() * 100.0f);
	preferredTileSize = TeaForGodEmperor::RoomGenerationInfo::get().get_preferred_tile_size();
	enlarged = TeaForGodEmperor::RoomGenerationInfo::get().was_play_area_enlarged()? 1 : 0;
	doorHeight = (int)(psp.doorHeight * 100.0f);
	scaleUpPlayer = psp.scaleUpPlayer? 1 : 0;
	allowCrouch = psp.allowCrouch;
	turnCounter = psp.turnCounter;
	keepInWorld = psp.keepInWorld;
	keepInWorldSensitivity = (int)(psp.keepInWorldSensitivity * 100.0f);
	manualRotate = mc.get_vr_map_space_rotate();	// in deg
	manualOffsetX = mc.get_vr_map_space_offset().x; // in cm
	manualOffsetY = mc.get_vr_map_space_offset().y; // in cm
	manualSizeX = mc.get_vr_map_space_size().get(Vector2::zero).x; // in cm
	manualSizeY = mc.get_vr_map_space_size().get(Vector2::zero).y; // in cm
}

void Options::PlayAreaOptions::apply_vr_map(bool _modifySize, bool _applyToVR)
{
	auto& mc = MainConfig::access_global();
	auto& psp = TeaForGodEmperor::PlayerSetup::access_current().access_preferences();
	{
		mc.set_vr_map_space_rotate(round_to(manualRotate, 0.1f));
		mc.set_vr_map_space_offset(Vector3(round_to(manualOffsetX, 0.01f), round_to(manualOffsetY, 0.01f), 0.0f));
		if (_modifySize)
		{
			if (manualSizeX == 0 || manualSizeY == 0)
			{
				mc.set_vr_map_space_size(NP);
			}
			else
			{
				mc.set_vr_map_space_size(Vector2(round_to(manualSizeX, 0.01f), round_to(manualSizeY, 0.01f)));
			}
		}
	}
	if (_applyToVR)
	{
		psp.apply_to_vr_and_room_generation_info(); // will set vr scaling too
	}
}

void Options::PlayAreaOptions::apply(bool _modifySize)
{
	auto& mc = MainConfig::access_global();
	mc.set_immobile_vr((Option::Type)immobileVR);
	auto& psp = TeaForGodEmperor::PlayerSetup::access_current().access_preferences();
	if (measurementSystem >= 0)
	{
		psp.measurementSystem = (MeasurementSystem::Type)measurementSystem;
	}
	else
	{
		psp.measurementSystem.clear();
	}
	//if (should_appear_as_immobile_vr(immobileVR) && _modifySize)
	if (_modifySize)
	{
		Vector2 size = Vector2::one * ((float)immobileVRSize / 100.0f);
		mc.set_immobile_vr_size(size);
	}
	//if (!should_appear_as_immobile_vr(immobileVR))
	{
		float borderAsFloat = (float)border / 100.0f;
		mc.set_vr_map_margin(Vector2(borderAsFloat, borderAsFloat));
		float horizontalScalingAsFloat = (float)horizontalScaling / 100.0f;
		mc.set_vr_horizontal_scaling(horizontalScalingAsFloat);
	}
	TeaForGodEmperor::RoomGenerationInfo::access().set_preferred_tile_size(get_preferred_tile_size());
	psp.doorHeight = (float)doorHeight / 100.0f;
	psp.scaleUpPlayer = scaleUpPlayer != 0;
	psp.allowCrouch = (Option::Type)allowCrouch;
	psp.turnCounter = (Option::Type)turnCounter;
	//if (!should_appear_as_immobile_vr(immobileVR))
	{
		psp.keepInWorld = keepInWorld;
		psp.keepInWorldSensitivity = (float)keepInWorldSensitivity / 100.0f;
	}
	apply_vr_map(_modifySize, true /* psp.apply_to_vr_and_room_generation_info */);
}

//

void Options::GeneralOptions::read()
{
	auto const & mc = MainConfig::global();
	auto const & psp = TeaForGodEmperor::PlayerSetup::get_current().get_preferences();
	reportBugs = mc.get_report_bugs();
	showTipsOnLoading = psp.showTipsOnLoading? 1 : 0;
	showTipsDuringGame = psp.showTipsDuringGame ? 1 : 0;
	immobileVR = mc.get_immobile_vr();
	measurementSystem = psp.measurementSystem.is_set()? psp.measurementSystem.get() : -1;
	language = psp.language.is_set()? Framework::LocalisedStrings::find_language_index(psp.get_language()) : -1;
	if (!psp.language.is_set())
	{
		// if auto chosen and no system provided, use default
		if (!Framework::LocalisedStrings::get_system_language().is_valid())
		{
			language = Framework::LocalisedStrings::find_language_index(Framework::LocalisedStrings::get_default_language());
		}
		if (Framework::LocalisedStrings::get_suggested_default_language().is_valid())
		{
			language = Framework::LocalisedStrings::find_language_index(Framework::LocalisedStrings::get_suggested_default_language());
		}
	}
	subtitles = psp.subtitles;
	subtitlesScale = (int)(psp.subtitlesScale * 100.0f);
	narrativeVoiceover = psp.narrativeVoiceovers == Option::On ? 1 : 0;
	pilgrimOverlayVoiceover = psp.pilgrimOverlayVoiceovers == Option::On ? 1 : 0;
	showRealTime = psp.showRealTime;
	if (TeaForGodEmperor::GameConfig const * gc = TeaForGodEmperor::GameConfig::get_as<TeaForGodEmperor::GameConfig>())
	{
		allowVRScreenShots = gc->get_allow_vr_screen_shots();
		showGameplayInfo = gc->does_show_vr_on_screen_with_info();
	}
	else
	{
		allowVRScreenShots = 0;
		showGameplayInfo = false;
	}
}

void Options::GeneralOptions::apply(bool _applyToVR)
{
	auto& mc = MainConfig::access_global();
	auto & psp = TeaForGodEmperor::PlayerSetup::access_current().access_preferences();
	mc.set_report_bugs((Option::Type)reportBugs);
	psp.showTipsOnLoading = showTipsOnLoading == 1;
	psp.showTipsDuringGame = showTipsDuringGame == 1;
	if (_applyToVR)
	{
		mc.set_immobile_vr((Option::Type)immobileVR);
	}
	if (measurementSystem >= 0)
	{
		psp.measurementSystem = (MeasurementSystem::Type)measurementSystem;
	}
	else
	{
		psp.measurementSystem.clear();
	}
	psp.showRealTime = showRealTime;
	apply_language_subtitles_voiceovers();
	if (TeaForGodEmperor::GameConfig * gc = TeaForGodEmperor::GameConfig::get_as<TeaForGodEmperor::GameConfig>())
	{
		gc->allow_vr_screen_shots((TeaForGodEmperor::AllowScreenShots::Type)allowVRScreenShots);
		gc->show_vr_on_screen_with_info(showGameplayInfo != 0);
	}
	if (_applyToVR)
	{
		auto& psp = TeaForGodEmperor::PlayerSetup::access_current().access_preferences();
		psp.apply_to_vr_and_room_generation_info(); // will set vr scaling too
	}
}

void Options::GeneralOptions::apply_language_subtitles_voiceovers()
{
	auto & psp = TeaForGodEmperor::PlayerSetup::access_current().access_preferences();
	if (language >= 0)
	{
		psp.language = Framework::LocalisedStrings::get_languages()[language].id;
	}
	else
	{
		psp.language.clear();
	}
	psp.subtitles = subtitles;
	psp.subtitlesScale = (float)subtitlesScale / 100.0f;
	psp.narrativeVoiceovers = narrativeVoiceover ? Option::On : Option::Off;
	psp.pilgrimOverlayVoiceovers = pilgrimOverlayVoiceover ? Option::On : Option::Off;
	psp.apply();
}

//

REGISTER_FOR_FAST_CAST(Options);

Options::Options(Type _type, int _flags)
: type(_type)
, flags(_flags)
{
}

Options::~Options()
{
	if (auto * vr = VR::IVR::get())
	{
		vr->force_bounds_rendering(false);
	}
#ifdef AN_DEVELOPMENT_OR_PROFILER
	get_hub()->allowDebugMovement = true;
#endif
}

void Options::go_back()
{
	Array<Name> screensToKeep;
	screensToKeep.push_back(NAME(hubBackground));
	//screensToKeep.push_back(NAME(optionsMain));
	//
	get_hub()->keep_only_screens(screensToKeep);

	{
		auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>();
		Framework::IModulesOwner* playerActor = nullptr;
		if (!playerActor)
		{
			playerActor = game->access_player().get_actor();
		}
		if (playerActor)
		{
			if (auto * mp = playerActor->get_gameplay_as<TeaForGodEmperor::ModulePilgrim>())
			{
				auto const& psp = TeaForGodEmperor::PlayerSetup::get_current().get_preferences();
				if (mp->set_pending_pockets_vertical_adjustment(psp.pocketsVerticalAdjustment))
				{
					SafePtr<Framework::IModulesOwner> safePlayerActor;
					safePlayerActor = playerActor;
					game->add_async_world_job(TXT("pilgrims recreate mesh generator"), [safePlayerActor, game]()
						{
							if (!game->does_want_to_cancel_creation())
							{
								if (auto * playerActor = safePlayerActor.get())
								{
									if (auto * mp = playerActor->get_gameplay_as<TeaForGodEmperor::ModulePilgrim>())
									{
										Framework::ScopedAutoActivationGroup saag(game->get_world());
										mp->recreate_mesh_accordingly_to_pending_setup();
									}
								}
							}
						});
				}
			}
		}
	}
	get_hub()->set_scene(prevScene.get());
}

void Options::on_activate(HubScene* _prev)
{
	if (!prevScene.is_set())
	{
		prevScene = _prev;
	}

	base::on_activate(_prev);

	videoOptionsAsInitialised.read();

	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	Array<Name> screensToKeep;
	screensToKeep.push_back(NAME(hubBackground));
	//screensToKeep.push_back(NAME(optionsMain));
	//
	get_hub()->keep_only_screens(screensToKeep);

	show_screen(MainScreen, true);

	// would be great to have a mechanism to automatically handle specific options giving particular values

	System::Core::block_on_advance(NAME(advanceRunTimer)); // just in case we entered from in game menu
}

void Options::on_deactivate(HubScene* _next)
{
	System::Core::allow_on_advance(NAME(advanceRunTimer)); // unblock

	base::on_deactivate(_next);
}

#define DEBUG_OPTION_BOOL(caption, option) \
	buttons.push_back(HubScreenButtonInfo(Name::invalid(), String(caption), \
		[this]() { option = !option; show_screen(DebugOptionsScreen, true); }).with_colour(option ? Optional<Colour>(HubColours::selected_highlighted()) : NP));

Rotator3 Options::read_forward_dir() const
{
	return is_flag_set(flags, Flags::GetCurrentForward) ? get_hub()->get_current_forward() : get_hub()->get_background_dir_or_main_forward();
}

void Options::on_update(float _deltaTime)
{
	base::on_update(_deltaTime);

	if (auto* scr = get_hub()->get_screen(NAME(optionsControlsVROffsets)))
	{
		if (auto* w = fast_cast<HubWidgets::Button>(scr->get_widget(NAME(idTestConfirmVROffsets))))
		{
			Optional<int> showSecondsNow;
			if (testVROffsetsTimeLeft.is_set())
			{
				testVROffsetsTimeLeft = testVROffsetsTimeLeft.get() - _deltaTime;
				if (testVROffsetsTimeLeft.get() <= 0.0f)
				{
					testVROffsetsTimeLeft.clear();
					controlsOptions.apply_vr_offsets(true); // restore to the last confirmed we had
#ifdef BUILD_NON_RELEASE
					devVROffsets.restore();
#endif
				}
				else
				{
					showSecondsNow = TypeConversions::Normal::f_i_closest(ceil(testVROffsetsTimeLeft.get()));
				}
			}
			if (testVROffsetsTimeLeftShowSeconds != showSecondsNow)
			{
				testVROffsetsTimeLeftShowSeconds = showSecondsNow;
				String text;
				if (testVROffsetsTimeLeftShowSeconds.is_set())
				{
					text = Framework::StringFormatter::format_loc_str(NAME(lsControlsVROffsetsConfirm), Framework::StringFormatterParams()
						.add(NAME(secondsLeft), testVROffsetsTimeLeftShowSeconds.get()));
				}
				else
				{
					text = LOC_STR(NAME(lsControlsVROffsetsTest));
				}
				w->set(text);
				w->set_highlighted(testVROffsetsTimeLeft.is_set());
			}
		}
	}
	else
	{
		testVROffsetsTimeLeft.clear();
		testVROffsetsTimeLeftShowSeconds.clear();
	}

	if (delayedScreen.is_set())
	{
		delayedScreenDelay -= _deltaTime;
		if (delayedScreenDelay <= 0.0f)
		{
			if (delayedScreenResetForward)
			{
				get_hub()->force_reset_and_update_forward();
			}
			show_screen(delayedScreen.get(), delayedScreenRefresh);
		}
	}
}

void Options::delayed_show_screen(float _delay, Screen _screen, Optional<bool> _refresh, Optional<bool> _resetForward)
{
	delayedScreen = _screen;
	delayedScreenRefresh = _refresh.get(false);
	delayedScreenResetForward = _resetForward.get(false);
	delayedScreenDelay = _delay;
}

void Options::show_screen(Screen _screen, bool _refresh, bool _keepForwardDir)
{
	requestFoveatedRenderingTestTS.reset(-10.0f);
	delayedScreen.clear();
	if (screen != _screen)
	{
		if (auto * vr = VR::IVR::get())
		{
			vr->force_bounds_rendering(_screen == PlayAreaOptionsScreen || _screen == PlayAreaCalibrateScreen);
#ifdef AN_DEVELOPMENT_OR_PROFILER
			get_hub()->allowDebugMovement = !(_screen == PlayAreaOptionsScreen || _screen == PlayAreaCalibrateScreen);
#endif
		}
	}

	screen = _screen;

	Vector2 ppa(24.0f, 24.0f);
	float sizeCoef = 4.0f / 6.0f;

	if (!_keepForwardDir)
	{
		forwardDir = read_forward_dir();
	}

	switch (screen)
	{
	case MainScreen:
	{
		Array<Name> screensToKeep;
		screensToKeep.push_back(NAME(hubBackground));
		//screensToKeep.push_back(NAME(optionsMain));
		//
		get_hub()->keep_only_screens(screensToKeep);

		bool fill = _refresh;
		auto* scr = get_hub()->get_screen(NAME(optionsMain));
		int optionsCount = 8;

		Array<HubScreenButtonInfo> buttons;
		{
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsGeneral), [this]() { show_screen(GeneralOptionsScreen); }));
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsPlayArea), [this]() { show_screen(PlayAreaOptionsScreen); }));
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsControls), [this]() { show_screen(ControlsOptionsScreen); }));
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsComfort), [this]() { show_screen(ComfortOptionsScreen); }));
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsVideo), [this]() { show_screen(VideoOptionsScreen); }));
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsGameplay), [this]() { show_screen(GameplayOptionsScreen); }));
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsSound), [this]() { show_screen(SoundOptionsScreen); }));
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsDevices), [this]() { show_screen(DevicesOptionsScreen); }).with_colour(PhysicalSensations::IPhysicalSensations::get() ? Optional<Colour>(HubColours::selected_highlighted()) : NP));
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsAesthetics), [this]() { show_screen(AestheticsOptionsScreen); }));
#ifndef BUILD_PUBLIC_RELEASE
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), String(TXT("debug")), [this]() { show_screen(DebugOptionsScreen); }));
#endif
			{
				bool addCheatMenu = false;
#ifdef WITH_DEBUG_CHEAT_MENU
				addCheatMenu = true;
#endif
				if (get_hub()->get_hand(Hand::Left).input.is_button_pressed(NAME(shift)) &&
					get_hub()->get_hand(Hand::Right).input.is_button_pressed(NAME(shift)))
				{
					addCheatMenu = true;
				}
				if (addCheatMenu)
				{
					buttons.push_back(HubScreenButtonInfo(Name::invalid(), String(TXT("cheats")), [this]() { show_screen(CheatsOptionsScreen); }));
				}
			}
#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), String(TXT("foveation setup")), [this]() { show_screen(SetupFoveationParamsScreen); }));
#endif
#ifdef ALLOW_SENDING_DEV_NOTES_FROM_OPTIONS
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsDebugNote), [this]() { HubScreens::DebugNote::show(get_hub(), LOC_STR(NAME(lsDebugNoteInfo)), String::empty(), false, true); } ));
#endif
		}
		int columnsCount = 2;
		optionsCount = buttons.get_size() + 1 /* back */;

		int rowsCount = ceil_by(optionsCount, columnsCount);

		if (!scr)
		{
			scr = new HubScreen(get_hub(), NAME(optionsMain), Vector2(50.0f * sizeCoef, (float)rowsCount * 10.0f * sizeCoef), forwardDir, get_hub()->get_radius() * 0.65f, NP, NP, ppa);
			//scr->followYawDeadZoneByBorder = HubScreen::DEFAULT_YAW_DEAD_ZONE_BY_BORDER;
			get_hub()->add_screen(scr);
			fill = true;
		}
		if (fill)
		{
			scr->clear();

			while (buttons.get_size() < rowsCount * 2 - 1)
			{
				buttons.push_back(HubScreenButtonInfo());
			}
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsBack), [this]() { go_back(); }).activate_on_trigger_hold());
			Vector2 fs = HubScreen::s_fontSizeInPixels;
			scr->add_button_widgets_grid(buttons, VectorInt2(2, rowsCount), Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y)), scr->mainResolutionInPixels.y.expanded_by(-max(fs.x, fs.y))), fs);
		}
	} break;
	case SetupFoveationParamsScreen:
	{
		Array<Name> screensToKeep;
		screensToKeep.push_back(NAME(hubBackground));
		//screensToKeep.push_back(NAME(optionsMain));
		//
		get_hub()->keep_only_screens(screensToKeep);

		bool fill = _refresh;
		auto* scr = get_hub()->get_screen(NAME(optionsSetupFoveationParams));
		if (!scr)
		{
			scr = new HubScreen(get_hub(), NAME(optionsSetupFoveationParams), Vector2(80.0f * sizeCoef, 140.0f), forwardDir, get_hub()->get_radius() * 0.55f, NP, NP, ppa * 0.9f);
			scr->followScreen = NAME(optionsMain);
			get_hub()->add_screen(scr);
			scr->be_modal();
			fill = true;
		}
		if (fill)
		{
			static bool useTweakingTemplate = false;

			scr->clear();

			Vector2 fs = HubScreen::s_fontSizeInPixels;

			Array<HubScreenOption> options;
			options.make_space_for(20);

#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS
			auto& att = ::System::FoveatedRenderingSetup::access_tweaking_template();
			att.focalPoints.set_size(2);

			static int focalPoint_at_x[2];
			static int focalPoint_at_y[2];
			static int focalPoint_offset_x_left[2];
			static int focalPoint_offset_x_right[2];
			static int focalPoint_gain_x[2];
			static int focalPoint_gain_y[2];
			static int focalPoint_area[2];
			
			static int minDensity;

			static int testLevel;
			static int maxDepth;
			static int templateLevel;
			
			static int testAtAnyTime;

			testAtAnyTime = ::System::FoveatedRenderingSetup::testAtAnyTime != 0;

			testLevel = (int)(::MainConfig::access_global().access_video().vrFoveatedRenderingLevel * 100.0f);
			maxDepth = ::MainConfig::access_global().access_video().vrFoveatedRenderingMaxDepth;
			templateLevel = (int)(att.forFoveatedRenderingLevel * 100.0f);

			if (useTweakingTemplate)
			{
				::System::FoveatedRenderingSetup::access_tweaking_template();
			}
			else
			{
				::System::FoveatedRenderingSetup::dont_use_tweaking_template();
			}
						
			options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), &testAtAnyTime)
				.add(0, String(TXT("testable only in options")))
				.add(1, String(TXT("testable at any time")))
				.do_post_update([]()
					{
						::System::FoveatedRenderingSetup::testAtAnyTime = testAtAnyTime;
					})
			);
				
			options.push_back(HubScreenOption(String(TXT("test level"))));
			options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), &testLevel, HubScreenOption::Slider)
				.with_slider_width_pct(0.7f)
				.to_get_text_for_slider([](int _v) {return String::printf(TXT("%.2f"), (float)_v / 100.0f); })
				.slider_range(0, (int)(::System::FoveatedRenderingSetup::get_max_vr_foveated_rendering_level() * 100.0f))
				.slider_step(25)
				.with_on_click([]()
					{
						::System::FoveatedRenderingSetup::dont_use_tweaking_template();
						::MainConfig::access_global().access_video().vrFoveatedRenderingLevel = (float)testLevel / 100.0f;
						::System::FoveatedRenderingSetup::force_set_foveation();
						useTweakingTemplate = false;
					})
				.do_post_update([]()
					{
						::System::FoveatedRenderingSetup::dont_use_tweaking_template();
						::MainConfig::access_global().access_video().vrFoveatedRenderingLevel = (float)testLevel / 100.0f;
						::System::FoveatedRenderingSetup::force_set_foveation();
						useTweakingTemplate = false;
					})
			);

			options.push_back(HubScreenOption(String(TXT("max depth"))));
			options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), &maxDepth, HubScreenOption::Slider)
				.with_slider_width_pct(0.7f)
				.to_get_text_for_slider([](int _v) {return String::printf(TXT("%i"), _v);  })
				.slider_range(1, 8)
				.slider_step(1)
				.with_on_click([]()
					{
						::System::FoveatedRenderingSetup::dont_use_tweaking_template();
						::MainConfig::access_global().access_video().vrFoveatedRenderingMaxDepth = maxDepth;
						::System::FoveatedRenderingSetup::force_set_foveation();
						useTweakingTemplate = false;
					})
				.do_post_update([]()
					{
						::System::FoveatedRenderingSetup::dont_use_tweaking_template();
						::MainConfig::access_global().access_video().vrFoveatedRenderingMaxDepth = maxDepth;
						::System::FoveatedRenderingSetup::force_set_foveation();
						useTweakingTemplate = false;
					})
			);

			options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), nullptr, HubScreenOption::Button)
				.with_text_on_button(String(TXT("test template")))
				.with_on_click([this]()
					{
						::System::FoveatedRenderingSetup::access_tweaking_template().forFoveatedRenderingLevel = ::MainConfig::access_global().access_video().vrFoveatedRenderingLevel;
						show_screen(SetupFoveationParamsScreen);
						useTweakingTemplate = true;
					})
			);
			/*
			options.push_back(HubScreenOption(String(TXT("template for level"))));
			options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), &templateLevel, HubScreenOption::Slider)
				.with_slider_width_pct(0.7f)
				.to_get_text_for_slider([](int _v) {return String::printf(TXT("%.2f"), (float)_v / 100.0f); })
				.slider_range(0, 300)
				.slider_step(25)
				.do_post_update([]()
					{
						::System::FoveatedRenderingSetup::access_tweaking_template().forFoveatedRenderingLevel = (float)templateLevel / 100.0f;
						::System::FoveatedRenderingSetup::force_set_foveation();
					})
			);
			*/

			options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), nullptr, HubScreenOption::Button)
				.with_text_on_button(String(TXT("copy current to template")))
				.with_on_click([this]()
					{
						::System::FoveatedRenderingSetup::set_tweaking_template_from_current();
						::System::FoveatedRenderingSetup::access_tweaking_template().forFoveatedRenderingLevel = ::MainConfig::access_global().access_video().vrFoveatedRenderingLevel;
						show_screen(SetupFoveationParamsScreen);
						useTweakingTemplate = true;
						output(TXT("after copy:"));
						::System::FoveatedRenderingSetup::access_tweaking_template().debug_log();
					})
			);

			options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), nullptr, HubScreenOption::Button)
				.with_text_on_button(String(TXT("copy template to current")))
				.with_on_click([this]()
					{
						::System::FoveatedRenderingSetup::store_tweaking_template_as_current();
						show_screen(SetupFoveationParamsScreen);
						useTweakingTemplate = false;
						output(TXT("after copy:"));
						::System::FoveatedRenderingSetup::access_tweaking_template().debug_log();
					})
			);

			options.push_back(HubScreenOption(String(TXT("min density"))));
			options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), &minDensity, HubScreenOption::Slider)
				.with_slider_width_pct(0.7f)
				.to_get_text_for_slider([](int _v) {return String::printf(TXT("%.2f"), (float)_v / 1000.0f); })
				.slider_range(0, 1000)
				.slider_step(1)
				.do_post_update([]()
					{
						::System::FoveatedRenderingSetup::access_tweaking_template().minDensity = (float)minDensity / 1000.0f;
						::System::FoveatedRenderingSetup::force_set_foveation();
						output(TXT("options changed to:"));
						::System::FoveatedRenderingSetup::access_tweaking_template().debug_log();
						useTweakingTemplate = true;
					})
			);

			minDensity = (int)(att.minDensity * 1000.0f);
			for_count(int, i, 2)
			{
				focalPoint_at_x[i] = (int)(att.focalPoints[i].at.x * 10000.0f);
				focalPoint_at_y[i] = (int)(att.focalPoints[i].at.y * 10000.0f);
				focalPoint_offset_x_left[i] = (int)(att.focalPoints[i].offsetXLeft * 10000.0f);
				focalPoint_offset_x_right[i] = (int)(att.focalPoints[i].offsetXRight * 10000.0f);
				focalPoint_gain_x[i] = (int)(att.focalPoints[i].gain.x * 10000.0f);
				focalPoint_gain_y[i] = (int)(att.focalPoints[i].gain.y * 10000.0f);
				focalPoint_area[i] = (int)(att.focalPoints[i].foveArea * 100.0f);

				options.push_back(HubScreenOption(String::printf(TXT("focal point %i"), i)));
				options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), &focalPoint_at_x[i], HubScreenOption::Slider)
					.with_slider_width_pct(0.7f)
					.to_get_text_for_slider([](int _v) {return String::printf(TXT("%.5f"), (float)_v / 10000.0f); })
					.slider_range(-10000, 10000)
					.slider_step(100)
					.do_post_update([i]()
						{
							::System::FoveatedRenderingSetup::access_tweaking_template().focalPoints[i].at.x = (float)focalPoint_at_x[i] / 10000.0f;
							::System::FoveatedRenderingSetup::force_set_foveation();
							output(TXT("options changed to:"));
							::System::FoveatedRenderingSetup::access_tweaking_template().debug_log();
							useTweakingTemplate = true;
						})
				);
				options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), &focalPoint_at_y[i], HubScreenOption::Slider)
					.with_slider_width_pct(0.7f)
					.to_get_text_for_slider([](int _v) {return String::printf(TXT("%.5f"), (float)_v / 10000.0f); })
					.slider_range(-10000, 10000)
					.slider_step(100)
					.do_post_update([i]()
						{
							::System::FoveatedRenderingSetup::access_tweaking_template().focalPoints[i].at.y = (float)focalPoint_at_y[i] / 10000.0f;
							::System::FoveatedRenderingSetup::force_set_foveation();
							output(TXT("options changed to:"));
							::System::FoveatedRenderingSetup::access_tweaking_template().debug_log();
							useTweakingTemplate = true;
						})
				);
				options.push_back(HubScreenOption(String::printf(TXT("offset left"))));
				options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), &focalPoint_offset_x_left[i], HubScreenOption::Slider)
					.with_slider_width_pct(0.7f)
					.to_get_text_for_slider([](int _v) {return String::printf(TXT("%.5f"), (float)_v / 10000.0f); })
					.slider_range(-1000, 1000)
					.slider_step(100)
					.do_post_update([i]()
						{
							::System::FoveatedRenderingSetup::access_tweaking_template().focalPoints[i].offsetXLeft = (float)focalPoint_offset_x_left[i] / 10000.0f;
							::System::FoveatedRenderingSetup::force_set_foveation();
							output(TXT("options changed to:"));
							::System::FoveatedRenderingSetup::access_tweaking_template().debug_log();
							useTweakingTemplate = true;
						})
				);
				options.push_back(HubScreenOption(String::printf(TXT("offset right"))));
				options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), &focalPoint_offset_x_right[i], HubScreenOption::Slider)
					.with_slider_width_pct(0.7f)
					.to_get_text_for_slider([](int _v) {return String::printf(TXT("%.5f"), (float)_v / 10000.0f); })
					.slider_range(-1000, 1000)
					.slider_step(100)
					.do_post_update([i]()
						{
							::System::FoveatedRenderingSetup::access_tweaking_template().focalPoints[i].offsetXRight = (float)focalPoint_offset_x_right[i] / 10000.0f;
							::System::FoveatedRenderingSetup::force_set_foveation();
							output(TXT("options changed to:"));
							::System::FoveatedRenderingSetup::access_tweaking_template().debug_log();
							useTweakingTemplate = true;
						})
				);

				options.push_back(HubScreenOption(String(TXT("gain"))));
				options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), &focalPoint_gain_x[i], HubScreenOption::Slider)
					.with_slider_width_pct(0.7f)
					.to_get_text_for_slider([](int _v) {return String::printf(TXT("%.5f"), (float)_v / 10000.0f); })
					.slider_range(0, 400000)
					.slider_step(1000)
					.do_post_update([i]()
						{
							::System::FoveatedRenderingSetup::access_tweaking_template().focalPoints[i].gain.x = (float)focalPoint_gain_x[i] / 10000.0f;
							::System::FoveatedRenderingSetup::force_set_foveation();
							output(TXT("options changed to:"));
							::System::FoveatedRenderingSetup::access_tweaking_template().debug_log();
							useTweakingTemplate = true;
						})
				);
				options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), &focalPoint_gain_y[i], HubScreenOption::Slider)
					.with_slider_width_pct(0.7f)
					.to_get_text_for_slider([](int _v) {return String::printf(TXT("%.5f"), (float)_v / 10000.0f); })
					.slider_range(0, 400000)
					.slider_step(1000)
					.do_post_update([i]()
						{
							::System::FoveatedRenderingSetup::access_tweaking_template().focalPoints[i].gain.y = (float)focalPoint_gain_y[i] / 10000.0f;
							::System::FoveatedRenderingSetup::force_set_foveation();
							output(TXT("options changed to:"));
							::System::FoveatedRenderingSetup::access_tweaking_template().debug_log();
							useTweakingTemplate = true;
						})
				);

				options.push_back(HubScreenOption(String(TXT("fove area"))));
				options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), &focalPoint_area[i], HubScreenOption::Slider)
					.with_slider_width_pct(0.7f)
					.to_get_text_for_slider([](int _v) {return String::printf(TXT("%.2f"), (float)_v / 100.0f); })
					.slider_range(0, 1000)
					.slider_step(10)
					.do_post_update([i]()
						{
							::System::FoveatedRenderingSetup::access_tweaking_template().focalPoints[i].foveArea = (float)focalPoint_area[i] / 100.0f;
							::System::FoveatedRenderingSetup::force_set_foveation();
							output(TXT("options changed to:"));
							::System::FoveatedRenderingSetup::access_tweaking_template().debug_log();
							useTweakingTemplate = true;
						})
				);
			}
#endif

			scr->add_option_widgets(options, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y)), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f, scr->mainResolutionInPixels.y.max - max(fs.x, fs.y) * 1.0f)),
				NP, min(HubScreen::s_fontSizeInPixels.x, HubScreen::s_fontSizeInPixels.y) * 0.2f);

			Array<HubScreenButtonInfo> buttons;
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsBack), [this]() {
#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS
				::System::FoveatedRenderingSetup::dont_use_tweaking_template();
#endif
				show_screen(MainScreen, true);
				useTweakingTemplate = false;
				}));

			scr->add_button_widgets(buttons, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y) * 6.0f), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 1.0f, scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f)), max(fs.x, fs.y) * 2.0f);

			scr->compress_vertically();
		}

	} break;
	case DebugOptionsScreen:
	{
		Array<Name> screensToKeep;
		screensToKeep.push_back(NAME(hubBackground));
		//screensToKeep.push_back(NAME(optionsMain));
		//
		get_hub()->keep_only_screens(screensToKeep);

		Array<HubScreenButtonInfo> buttons;

#ifdef WITH_PRESENCE_INDICATOR
		DEBUG_OPTION_BOOL(TXT("presence indicator"), Framework::ModulePresence::showPresenceIndicator);
#endif
#ifdef DEBUG_RENDER_DOOR_HOLES
		DEBUG_OPTION_BOOL(TXT("room proxy door (partially)"), Framework::Render::RoomProxy::debugRenderDoorHolesPartially);
		DEBUG_OPTION_BOOL(TXT("room proxy door (part - near)"), Framework::Render::RoomProxy::debugRenderDoorHolesPartiallyOnlyNear);
		DEBUG_OPTION_BOOL(TXT("room proxy door (part - normal)"), Framework::Render::RoomProxy::debugRenderDoorHolesPartiallyOnlyNormal);
		DEBUG_OPTION_BOOL(TXT("room proxy door (smaller cap)"), Framework::Render::RoomProxy::debugRenderDoorHolesSmallerCap);
		DEBUG_OPTION_BOOL(TXT("room proxy door (no extend)"), Framework::Render::RoomProxy::debugRenderDoorHolesNoExtend);
		DEBUG_OPTION_BOOL(TXT("room proxy door (negative extend)"), Framework::Render::RoomProxy::debugRenderDoorHolesNegativeExtend);
		DEBUG_OPTION_BOOL(TXT("room proxy door (front cap z sub zero)"), Framework::Render::RoomProxy::debugRenderDoorHolesFrontCapZSubZero);
		DEBUG_OPTION_BOOL(TXT("room proxy door (front cap z zero)"), Framework::Render::RoomProxy::debugRenderDoorHolesFrontCapZZero);
		DEBUG_OPTION_BOOL(TXT("room proxy door (front cap z smaller)"), Framework::Render::RoomProxy::debugRenderDoorHolesFrontCapZSmaller);
		DEBUG_OPTION_BOOL(TXT("room proxy door (front cap z small)"), Framework::Render::RoomProxy::debugRenderDoorHolesFrontCapZSmall);
		DEBUG_OPTION_BOOL(TXT("room proxy door (front cap z larger)"), Framework::Render::RoomProxy::debugRenderDoorHolesFrontCapZLarger);
		DEBUG_OPTION_BOOL(TXT("room proxy door (close always depth)"), Framework::Render::RoomProxy::debugRenderDoorHolesCloseAlwaysDepth);
		DEBUG_OPTION_BOOL(TXT("room proxy door (don't use depth range)"), Framework::Render::RoomProxy::debugRenderDoorHolesDontUseDepthRange);
		DEBUG_OPTION_BOOL(TXT("room proxy door (padded depth range)"), Framework::Render::RoomProxy::debugRenderDoorHolesPaddedDepthRange);
#endif

		buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsBack), [this]() { show_screen(MainScreen, true); }).activate_on_trigger_hold());

		bool fill = _refresh;
		bool compressOnFill = false;
		auto* scr = get_hub()->get_screen(NAME(optionsDebug));
		if (!scr)
		{
			scr = new HubScreen(get_hub(), NAME(optionsDebug), Vector2(60.0f * sizeCoef, ((float)buttons.get_size() / 2.0f) * 8.0f * sizeCoef), forwardDir, get_hub()->get_radius() * 0.55f, NP, NP, Vector2(25.0f, 25.0f));
			scr->followScreen = NAME(optionsMain);
			get_hub()->add_screen(scr);
			scr->be_modal();
			fill = true;
			compressOnFill = true;
		}
		if (fill)
		{
			scr->clear();

			Vector2 fs = HubScreen::s_fontSizeInPixels;

			scr->add_button_widgets_grid(buttons, VectorInt2(2, (buttons.get_size() + 1) / 2), Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y)), scr->mainResolutionInPixels.y.expanded_by(-max(fs.x, fs.y))), fs);

			if (compressOnFill)
			{
				scr->compress_vertically();
			}
		}
	} break;
	case CheatsOptionsScreen:
	{
		Array<Name> screensToKeep;
		screensToKeep.push_back(NAME(hubBackground));
		//screensToKeep.push_back(NAME(optionsMain));
		//
		get_hub()->keep_only_screens(screensToKeep);

		Array<HubScreenButtonInfo> buttons;

		{
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), String(TXT("+1000XP")),
				[]() {
					TeaForGodEmperor::Energy experience = TeaForGodEmperor::Energy(1000.0f);
					TeaForGodEmperor::PlayerSetup::access_current().stats__experience(experience);
					TeaForGodEmperor::GameStats::get().add_experience(experience);
					TeaForGodEmperor::Persistence::access_current().provide_experience(experience);
				}));
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), String(TXT("+10MP")),
				[]() {
					int meritPoints = 100;
					TeaForGodEmperor::PlayerSetup::access_current().stats__merit_points(meritPoints);
					TeaForGodEmperor::GameStats::get().add_merit_points(meritPoints);
					TeaForGodEmperor::Persistence::access_current().provide_merit_points(meritPoints);
				}));
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), String(TXT("all a+p exms")),
				[]() {
					Array<Name> allEXMs = TeaForGodEmperor::EXMType::get_all();
					auto& persistence = TeaForGodEmperor::Persistence::access_current();
					Concurrency::ScopedSpinLock lock(persistence.access_lock());
					for_every(exmId, allEXMs)
					{
						if (auto* exm = TeaForGodEmperor::EXMType::find(*exmId))
						{
							if (!exm->is_permanent() &&
								!exm->is_integral())
							{
								persistence.access_all_exms().push_back_unique(*exmId);
							}
						}
					}
				}));
			if (auto* gs = TeaForGodEmperor::PlayerSetup::get_current().get_active_game_slot())
			{
				if (! gs->is_game_slot_mode_available(TeaForGodEmperor::GameSlotMode::Missions))
				{
					buttons.push_back(HubScreenButtonInfo(Name::invalid(), String(TXT("unlock tasks")),
						[]()
						{
							TeaForGodEmperor::PlayerSetup::access_current().access_active_game_slot()->make_game_slot_mode_available(TeaForGodEmperor::GameSlotMode::Missions);
						}));
				}
			}
		}

		buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsBack), [this]() { show_screen(MainScreen, true); }).activate_on_trigger_hold());

		bool fill = _refresh;
		bool compressOnFill = false;
		auto* scr = get_hub()->get_screen(NAME(optionsCheats));
		if (!scr)
		{
			scr = new HubScreen(get_hub(), NAME(optionsCheats), Vector2(60.0f * sizeCoef, ((float)buttons.get_size() / 2.0f) * 8.0f * sizeCoef), forwardDir, get_hub()->get_radius() * 0.55f, NP, NP, Vector2(25.0f, 25.0f));
			scr->followScreen = NAME(optionsMain);
			get_hub()->add_screen(scr);
			scr->be_modal();
			fill = true;
			compressOnFill = true;
		}
		if (fill)
		{
			scr->clear();

			Vector2 fs = HubScreen::s_fontSizeInPixels;

			scr->add_button_widgets_grid(buttons, VectorInt2(2, (buttons.get_size() + 1) / 2), Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y)), scr->mainResolutionInPixels.y.expanded_by(-max(fs.x, fs.y))), fs);

			if (compressOnFill)
			{
				scr->compress_vertically();
			}
		}
	} break;
	case DevicesOptionsScreen:
	{
		Array<Name> screensToKeep;
		screensToKeep.push_back(NAME(hubBackground));
		//screensToKeep.push_back(NAME(optionsMain));
		//screensToKeep.push_back(NAME(optionsDevices));
		//
		get_hub()->keep_only_screens(screensToKeep);

		Array<HubScreenButtonInfo> buttons;

		buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsDevicesControllers), [this]() { show_screen(DevicesControllersOptionsScreen); }));
#ifdef WITH_PHYSICAL_SENSATIONS
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_JAVA
		buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsDevicesBhaptics), [this]() { show_screen(DevicesBhapticsOptionsScreen); }).with_colour(PhysicalSensations::IPhysicalSensations::get() && MainConfig::global().get_physical_sensations_system() == PhysicalSensations::BhapticsJava::system_id() ? Optional<Colour>(HubColours::selected_highlighted()) : NP));
#endif
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_WINDOWS
		buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsDevicesBhaptics), [this]() { show_screen(DevicesBhapticsOptionsScreen); }).with_colour(PhysicalSensations::IPhysicalSensations::get() && MainConfig::global().get_physical_sensations_system() == PhysicalSensations::BhapticsWindows::system_id() ? Optional<Colour>(HubColours::selected_highlighted()) : NP));
#endif
#ifdef PHYSICAL_SENSATIONS_OWO
		buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsDevicesOWO), [this]() { show_screen(DevicesOWOOptionsScreen); }).with_colour(PhysicalSensations::IPhysicalSensations::get() && MainConfig::global().get_physical_sensations_system() == PhysicalSensations::OWO::system_id() ? Optional<Colour>(HubColours::selected_highlighted()) : NP));
#endif
#endif
		buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsBack), [this]() { show_screen(MainScreen, true); }).activate_on_trigger_hold());

		bool fill = _refresh;
		bool compressOnFill = false;
		auto* scr = get_hub()->get_screen(NAME(optionsDevices));
		if (!scr)
		{
			scr = new HubScreen(get_hub(), NAME(optionsDevices), Vector2(32.0f * sizeCoef, (float)buttons.get_size() * 14.0f * sizeCoef), forwardDir, get_hub()->get_radius() * 0.55f, NP, NP, ppa);
			scr->followScreen = NAME(optionsMain);
			get_hub()->add_screen(scr);
			scr->be_modal();
			fill = true;
			compressOnFill = true;
		}
		if (fill)
		{
			scr->clear();

			Vector2 fs = HubScreen::s_fontSizeInPixels;

			scr->add_button_widgets_grid(buttons, VectorInt2(1, buttons.get_size()), Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y)), scr->mainResolutionInPixels.y.expanded_by(-max(fs.x, fs.y))), fs);

			if (compressOnFill)
			{
				scr->compress_vertically();
			}
		}
	} break;
	case DevicesBhapticsOptionsScreen:
	{
#ifdef USING_PHYSICAL_SENSATIONS_BHAPTICS
		// start right now if not started
		{
			RefCountObjectPtr<PhysicalSensations::IPhysicalSensations> keepPS;
			keepPS = PhysicalSensations::IPhysicalSensations::get_as_is();
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_JAVA
			if (fast_cast<PhysicalSensations::BhapticsJava>(keepPS.get()) == nullptr)
			{
				auto& mc = MainConfig::access_global();
				mc.set_physical_sensations_system(PhysicalSensations::BhapticsJava::system_id());

				PhysicalSensations::initialise();
				auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>();
				game->get_job_system()->do_asynchronous_off_system_job(PhysicalSensations::IPhysicalSensations::do_async_init_thread_func, nullptr);
			}
#endif
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_WINDOWS
			if (fast_cast<PhysicalSensations::BhapticsWindows>(keepPS.get()) == nullptr)
			{
				auto& mc = MainConfig::access_global();
				mc.set_physical_sensations_system(PhysicalSensations::BhapticsWindows::system_id());

				PhysicalSensations::initialise();
				auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>();
				game->get_job_system()->do_asynchronous_off_system_job(PhysicalSensations::IPhysicalSensations::do_async_init_thread_func, nullptr);
			}
#endif
		}
		bool fill = _refresh;
		auto* scr = get_hub()->get_screen(NAME(optionsDevicesBhaptics));
		if (!scr)
		{
			scr = new HubScreen(get_hub(), NAME(optionsDevicesBhaptics), Vector2(70.0f * sizeCoef, 120.0f), forwardDir, get_hub()->get_radius() * 0.55f, NP, NP, ppa);
			scr->followScreen = NAME(optionsMain);
			get_hub()->add_screen(scr);
			scr->be_modal();
			scr->alwaysRender = true;
			fill = true;
		}
		else if (fill)
		{
			Vector2 size = scr->get_size();
			size.y = 120.0f;
			scr->set_size(size, false);
		}
		devicesBhapticsOptions.showedNoPlayer = false;
		if (fill)
		{
			scr->clear();

			Vector2 fs = HubScreen::s_fontSizeInPixels;
			float fsxy = max(fs.x, fs.y);
			float widgetSpacing = fsxy;

			Range2 availableAt = scr->mainResolutionInPixels.expanded_by(-Vector2::one * fsxy);

			{
				Array<HubScreenButtonInfo> buttons;
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_JAVA
				buttons.push_back(HubScreenButtonInfo(NAME(lsDevicesBhapticsPositionToggle), NAME(lsDevicesBhapticsPositionToggle), [this]() { bhaptics_op(NAME(toggle)); }));
				buttons.push_back(HubScreenButtonInfo(NAME(lsDevicesTest), NAME(lsDevicesTest), [this]() { bhaptics_op(NAME(test)); }));
#endif
				buttons.push_back(HubScreenButtonInfo(NAME(lsDevicesAccept), NAME(lsDevicesAccept), [this]() { haptics_accept(); }));
				buttons.push_back(HubScreenButtonInfo(NAME(lsDevicesDisable), NAME(lsDevicesDisable), [this]() { haptics_disable(); }));
#ifdef BUILD_PREVIEW
				buttons.push_back(HubScreenButtonInfo(Name::invalid(), String(TXT("debug test all")), [this]() { haptics_debug_test_all(); }));
#endif

				Range2 widgetAt = availableAt;

				VectorInt2 buttonCount = VectorInt2(2, 1);
				buttonCount.y = ceil_by(buttons.get_size(), buttonCount.x);

				widgetAt.y.max = widgetAt.y.min + fs.y * 2.0f * (float)buttonCount.y + fs.y * (float)(buttonCount.y - 1);

				scr->add_button_widgets_grid(buttons, buttonCount, widgetAt, Vector2(fsxy, fs.y));

				availableAt.y.min = widgetAt.y.max + widgetSpacing;
			}

			// for windows it doesn't make sense
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_JAVA
			{
				Array<HubScreenOption> options;
				options.make_space_for(20);

				options.push_back(HubScreenOption(NAME(lsDevicesBhapticsName), NAME(lsDevicesBhapticsName), nullptr, HubScreenOption::Centred));
				options.push_back(HubScreenOption(NAME(lsDevicesBhapticsPosition), NAME(lsDevicesBhapticsPosition), nullptr, HubScreenOption::Centred));
				options.push_back(HubScreenOption(NAME(lsDevicesBhapticsConnection), NAME(lsDevicesBhapticsConnection), nullptr, HubScreenOption::Centred));
				options.push_back(HubScreenOption(NAME(lsDevicesBhapticsAddress), NAME(lsDevicesBhapticsAddress), nullptr, HubScreenOption::Centred));
				options.push_back(HubScreenOption(NAME(lsDevicesBhapticsBattery), NAME(lsDevicesBhapticsBattery), nullptr, HubScreenOption::Centred));

				float lineSpacing = fsxy * 0.5f;
				float lineSize = fsxy * 1.2f;
				Range2 widgetAt = availableAt;
				widgetAt.y.max = widgetAt.y.min + lineSize * (float)(options.get_size()) + lineSpacing * (float)(options.get_size() - 1);

				scr->add_option_widgets(options, widgetAt, lineSize, lineSpacing, NP, 0.3f);

				availableAt.y.min = widgetAt.y.max + widgetSpacing;
			}
#endif

			{
				float inElementMargin = fs.y * 0.4f;
				float elementHeight = fs.y + inElementMargin * 2.0f;
				float elementSpacing = fs.y * 0.2f;
				int elementCount = 4; // 2x arms + 1x vest + 1x head

				Range2 widgetAt = availableAt;

				widgetAt.y.min = widgetAt.y.max - ((float)elementCount * elementHeight + (float)(elementCount - 1) * elementSpacing);

				auto* list = new HubWidgets::List(NAME(idDevicesList), widgetAt);
				list->allowScrollWithStick = true;
				list->scroll.scrollType = Loader::HubWidgets::InnerScroll::VerticalScroll;
				list->elementSize = Vector2(list->at.x.length(), elementHeight);
				list->elementSpacing = Vector2(elementSpacing, elementSpacing);
				list->draw_element = [list, inElementMargin](Framework::Display* _display, Range2 const& _at, HubDraggable const* _element)
				{
					if (auto* bhdi = fast_cast<BhapticsDeviceInfo>(_element->data.get()))
					{
						if (auto* font = _display->get_font())
						{
							Colour textColour = list->hub->is_selected(list, _element) ? HubColours::selected_highlighted() : HubColours::text();

							font->draw_text_at(::System::Video3D::get(), bhdi->shortInfo.to_char(), textColour, Vector2(_at.x.min + inElementMargin, _at.y.centre()), Vector2::one, Vector2(0.0f, 0.5f));
						}
					}
				};

				scr->add_widget(list);
			}

			scr->compress_vertically();
		}
#endif
	} break;
	case DevicesControllersOptionsScreen:
	{
		bool fill = _refresh;
		auto* scr = get_hub()->get_screen(NAME(optionsDevicesControllers));
		if (!scr)
		{
			scr = new HubScreen(get_hub(), NAME(optionsDevicesControllers), Vector2(60.0f * sizeCoef, 120.0f), forwardDir, get_hub()->get_radius() * 0.55f, NP, NP, ppa);
			scr->followScreen = NAME(optionsMain);
			get_hub()->add_screen(scr);
			scr->be_modal();
			fill = true;

			usedDevicesControllersOptions.read();
			devicesControllersOptions = usedDevicesControllersOptions;
		}
		else if (fill)
		{
			Vector2 size = scr->get_size();
			size.y = 120.0f;
			scr->set_size(size, false);
		}
		if (fill)
		{
			scr->clear();

			Vector2 fs = HubScreen::s_fontSizeInPixels;

			Array<HubScreenOption> options;
			options.make_space_for(20);

			options.push_back(HubScreenOption(Name::invalid(), NAME(lsDevicesControllersVRHapticFeedback), &devicesControllersOptions.vrHapticFeedback, HubScreenOption::Slider)
					.with_slider_width_pct(0.4f)
					.to_get_text_for_slider([](int _v) {return String::printf(TXT("%i%%"), _v); })
					.slider_range(0, 200)
					.slider_step(10)
					);

			scr->add_option_widgets(options, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y)), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f, scr->mainResolutionInPixels.y.max - max(fs.x, fs.y) * 1.0f)));

			Array<HubScreenButtonInfo> buttons;
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsApply), [this]() { apply_devices_controllers_options(); }).activate_on_trigger_hold());
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsCancel), [this]() { devicesControllersOptions = usedDevicesControllersOptions; apply_devices_controllers_options(); }));

			scr->add_button_widgets(buttons, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y) * 6.0f), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 1.0f, scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f)), max(fs.x, fs.y) * 2.0f);

			scr->compress_vertically();
		}
	} break;
	case DevicesOWOOptionsScreen:
	{
		bool fill = _refresh;
		auto* scr = get_hub()->get_screen(NAME(optionsDevicesOWO));
		if (!scr)
		{
			scr = new HubScreen(get_hub(), NAME(optionsDevicesOWO), Vector2(50.0f * sizeCoef, 120.0f), forwardDir, get_hub()->get_radius() * 0.55f, NP, NP, ppa);
			scr->followScreen = NAME(optionsMain);
			get_hub()->add_screen(scr);
			scr->be_modal();
			fill = true;

			usedDevicesOWOOptions.read();
			devicesOWOOptions = usedDevicesOWOOptions;
		}
		else if (fill)
		{
			Vector2 size = scr->get_size();
			size.y = 120.0f;
			scr->set_size(size, false);
		}
		if (fill)
		{
			scr->clear();

			Vector2 fs = HubScreen::s_fontSizeInPixels;

			Array<HubScreenOption> options;
			options.make_space_for(20);

			options.push_back(HubScreenOption(Name::invalid(), NAME(lsDevicesOWOAddress), nullptr, HubScreenOption::Button)
								.with_text_on_button(devicesOWOOptions.owoAddress.is_empty() ? LOC_STR(NAME(lsDevicesOWOAddressMissing)) : devicesOWOOptions.owoAddress)
								.with_on_click([this]()
									{
										HubScreens::EnterText::show(get_hub(), LOC_STR(NAME(lsDevicesOWOAddress)), devicesOWOOptions.owoAddress,
											[this](String const& _text)
											{
												devicesOWOOptions.owoAddress = _text;
												show_screen(DevicesOWOOptionsScreen, true);
											},
											[]()
											{
											},
											false, true,
											HubScreens::EnterText::NetworkAddress);
									})
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsDevicesOWOPort), nullptr, HubScreenOption::Button)
								.with_text_on_button(String::printf(TXT("%i"), devicesOWOOptions.owoPort))
								.with_on_click([this]()
									{
										HubScreens::EnterText::show(get_hub(), LOC_STR(NAME(lsDevicesOWOPort)), String::printf(TXT("%i"), devicesOWOOptions.owoPort),
											[this](String const& _text)
											{
												devicesOWOOptions.owoPort = ParserUtils::parse_int(_text);
												show_screen(DevicesOWOOptionsScreen, true);
											},
											[]()
											{
											},
											false, true,
											HubScreens::EnterText::NetworkPort);
									})
								);

			options.push_back(HubScreenOption(NAME(idDevicesStatus), NAME(lsDevicesStatus), nullptr, HubScreenOption::Text));
			devicesOWOOptions.visibleStatus.clear();
			devicesOWOOptions.testPending = false;
			devicesOWOOptions.exitOnConnected = false;

			scr->add_option_widgets(options, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y)), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f, scr->mainResolutionInPixels.y.max - max(fs.x, fs.y) * 1.0f)));

			Array<HubScreenButtonInfo> buttons;
			buttons.push_back(HubScreenButtonInfo(NAME(lsDevicesTest), NAME(lsDevicesTest), [this]() { owo_test_devices(); }));
			buttons.push_back(HubScreenButtonInfo(NAME(lsDevicesAccept), NAME(lsDevicesAccept), [this]() { apply_devices_owo_options(true); }).activate_on_trigger_hold());
			buttons.push_back(HubScreenButtonInfo(NAME(lsDevicesDisable), NAME(lsDevicesDisable), [this]() { devicesOWOOptions = usedDevicesOWOOptions; haptics_disable(); }));

			scr->add_button_widgets(buttons, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y)), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 1.0f, scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f)), max(fs.x, fs.y) * 2.0f);

			scr->compress_vertically();
		}
	} break;
	case SoundOptionsScreen:
	{
		bool fill = _refresh;
		auto* scr = get_hub()->get_screen(NAME(optionsSound));
		if (!scr)
		{
			scr = new HubScreen(get_hub(), NAME(optionsSound), Vector2(50.0f * sizeCoef, 120.0f), forwardDir, get_hub()->get_radius() * 0.55f, NP, NP, ppa);
			scr->followScreen = NAME(optionsMain);
			get_hub()->add_screen(scr);
			scr->be_modal();
			fill = true;

			usedSoundOptions.read();
			soundOptions = usedSoundOptions;
		}
		else if (fill)
		{
			Vector2 size = scr->get_size();
			size.y = 120.0f;
			scr->set_size(size, false);
		}
		if (fill)
		{
			scr->clear();

			Vector2 fs = HubScreen::s_fontSizeInPixels;

			Array<HubScreenOption> options;
			options.make_space_for(20);

			options.push_back(HubScreenOption(Name::invalid(), NAME(lsSoundMainVolume), &soundOptions.mainVolume, HubScreenOption::Slider)
								.with_slider_width_pct(0.4f)
								.slider_range(0, 150)
								.slider_step(5)
								.to_get_text_for_slider([](int _v) { return String::printf(TXT("%i%S"), _v, _v > 100? TXT("!") : TXT("")); })
								.do_post_update([this]() { apply_sound_volumes(); })
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsSoundSoundVolume), &soundOptions.soundVolume, HubScreenOption::Slider)
								.with_slider_width_pct(0.4f)
								.slider_step(5)
								//.to_get_text_for_slider([](int _v) {return String::printf(TXT("%i%%"), _v); })
								.do_post_update([this]() { apply_sound_volumes(); })
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsSoundEnvironmentVolume), &soundOptions.environmentVolume, HubScreenOption::Slider)
								.with_slider_width_pct(0.4f)
								.slider_step(5)
								//.to_get_text_for_slider([](int _v) {return String::printf(TXT("%i%%"), _v); })
								.do_post_update([this]() { apply_sound_volumes(); })
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsSoundVoiceoverVolume), &soundOptions.voiceoverVolume, HubScreenOption::Slider)
								.with_slider_width_pct(0.4f)
								.slider_step(5)
								//.to_get_text_for_slider([](int _v) {return String::printf(TXT("%i%%"), _v); })
								.do_post_update([this]() { apply_sound_volumes(); })
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsSoundPilgrimOverlayVolume), &soundOptions.pilgrimOverlayVolume, HubScreenOption::Slider)
								.with_slider_width_pct(0.4f)
								.slider_step(5)
								//.to_get_text_for_slider([](int _v) {return String::printf(TXT("%i%%"), _v); })
								.do_post_update([this]() { apply_sound_volumes(); })
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsSoundMusicVolume), &soundOptions.musicVolume, HubScreenOption::Slider)
								.with_slider_width_pct(0.4f)
								.slider_step(5)
								//.to_get_text_for_slider([](int _v) {return String::printf(TXT("%i%%"), _v); })
								.do_post_update([this]() { apply_sound_volumes(); })
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsSoundUIVolume), &soundOptions.uiVolume, HubScreenOption::Slider)
								.with_slider_width_pct(0.4f)
								.slider_step(5)
								//.to_get_text_for_slider([](int _v) {return String::printf(TXT("%i%%"), _v); })
								.do_post_update([this]() { apply_sound_volumes(); })
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsSoundSubtitles), &soundOptions.subtitles)
								.add(0, NAME(lsSoundSubtitlesOff))
								.add(1, NAME(lsSoundSubtitlesOn))
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsSoundSubtitlesScale), &soundOptions.subtitlesScale, HubScreenOption::Slider)
								.with_slider_width_pct(0.4f)
								.slider_range(50, 200)
								.slider_step(10)
								.to_get_text_for_slider([](int _v) {return String::printf(TXT("%i%%"), _v); })
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsSoundNarrativeVoiceover), &soundOptions.narrativeVoiceover)
								.add(0, NAME(lsSoundNarrativeVoiceoverOff))
								.add(1, NAME(lsSoundNarrativeVoiceoverOn))
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsSoundPilgrimOverlayVoiceover), &soundOptions.pilgrimOverlayVoiceover)
								.add(0, NAME(lsSoundPilgrimOverlayVoiceoverOff))
								.add(1, NAME(lsSoundPilgrimOverlayVoiceoverOn))
								);

			scr->add_option_widgets(options, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y)), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f, scr->mainResolutionInPixels.y.max - max(fs.x, fs.y) * 1.0f)));

			Array<HubScreenButtonInfo> buttons;
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsApply), [this]() { apply_sound_options(); }).activate_on_trigger_hold());
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsCancel), [this]() { soundOptions = usedSoundOptions; apply_sound_options(); }));

			scr->add_button_widgets(buttons, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y) * 6.0f), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 1.0f, scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f)), max(fs.x, fs.y) * 2.0f);

			scr->compress_vertically();
		}
	} break;
	case AestheticsOptionsScreen:
	{
		bool fill = _refresh;
		auto* scr = get_hub()->get_screen(NAME(optionsAesthetics));
		if (!scr)
		{
			scr = new HubScreen(get_hub(), NAME(optionsAesthetics), Vector2(60.0f * sizeCoef, 120.0f), forwardDir, get_hub()->get_radius() * 0.55f, NP, NP, ppa);
			scr->followScreen = NAME(optionsMain);
			get_hub()->add_screen(scr);
			scr->be_modal();
			fill = true;

			usedAestheticsOptions.read();
			aestheticsOptions = usedAestheticsOptions;
		}
		else if (fill)
		{
			Vector2 size = scr->get_size();
			size.y = 120.0f;
			scr->set_size(size, false);
		}
		if (fill)
		{
			scr->clear();

			Vector2 fs = HubScreen::s_fontSizeInPixels;

			Array<HubScreenOption> options;
			options.make_space_for(20);

			if (type == Main)
			{
				/*
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsAestheticsGeneral), &aestheticsOptions.style)
								.add(0, NAME(lsAestheticsGeneralNormal))
								.add(1, NAME(lsAestheticsGeneralRetro))
								);
				*/
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsAestheticsDitheredTransparency), &aestheticsOptions.ditheredTransparency)
								.add(0, NAME(lsAestheticsDitheredTransparencyOff))
								.add(1, NAME(lsAestheticsDitheredTransparencyOn))
								);
			}
			else
			{
				/*
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsAestheticsGeneral), nullptr, HubScreenOption::Text)
					.with_text(aestheticsOptions.style == 0? LOC_STR(NAME(lsAestheticsGeneralNormal)) : LOC_STR(NAME(lsAestheticsGeneralRetro)))
				);
				*/
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsAestheticsDitheredTransparency), nullptr, HubScreenOption::Text)
					.with_text(aestheticsOptions.ditheredTransparency == 0 ? LOC_STR(NAME(lsAestheticsDitheredTransparencyOff)) : LOC_STR(NAME(lsAestheticsDitheredTransparencyOn)))
				);
				
			}

			scr->add_option_widgets(options, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y)), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f, scr->mainResolutionInPixels.y.max - max(fs.x, fs.y) * 1.0f)));

			Array<HubScreenButtonInfo> buttons;
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsApply), [this]() { apply_aesthetics_options(); }).activate_on_trigger_hold());
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsCancel), [this]() { show_screen(MainScreen, true); }));

			scr->add_button_widgets(buttons, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y) * 6.0f), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 1.0f, scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f)), max(fs.x, fs.y) * 2.0f);

			scr->compress_vertically();
		}
	} break;
	case VideoOptionsScreen:
	{
		bool fill = _refresh;
		if (auto* scr = get_hub()->get_screen(NAME(optionsVideoLayout)))
		{
			scr->deactivate();
		}
		auto* scr = get_hub()->get_screen(NAME(optionsVideo));
		if (!scr)
		{
			scr = new HubScreen(get_hub(), NAME(optionsVideo), Vector2(60.0f * sizeCoef, 120.0f), forwardDir, get_hub()->get_radius() * 0.55f, NP, NP, ppa);
			scr->followScreen = NAME(optionsMain);
			get_hub()->add_screen(scr);
			scr->be_modal();
			fill = true;

			if (!_refresh)
			{
				usedVideoOptions.read();
				videoOptions = usedVideoOptions;
			}
		}
		else if (fill)
		{
			Vector2 size = scr->get_size();
			size.y = 120.0f;
			scr->set_size(size, false);
		}
		if (fill)
		{
			scr->clear();

			Vector2 fs = HubScreen::s_fontSizeInPixels;

			Array<HubScreenOption> options;
			options.make_space_for(20);

			VR::AvailableFunctionality vraf;
			if (auto* vr = VR::IVR::get())
			{
				vraf = vr->get_available_functionality();
			}

			todo_note(TXT("this requires changing vr to show/hide these options"));
			bool usesSeparateOutputRenderTarget = false;
			if (!videoOptions.directlyToVR)
			{
				if (auto* vr = VR::IVR::get())
				{
					usesSeparateOutputRenderTarget = vr->does_use_separate_output_render_target();
				}
			}

			if (! MainSettings::global().get_shader_options().get_tag(NAME(retro)))
			{
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoMSAA), &videoOptions.msaaSamples/*, HubScreenOption::List*/)
								.add(0, NAME(lsVideoMSAAOff))
								.add(2, String(TXT("x2")))
								.add(4, String(TXT("x4")))
#ifndef AN_ANDROID
								.add(8, String(TXT("x8")))
								.add(16, String(TXT("x16")))
#endif
								);
				if (! videoOptions.directlyToVR &&
					usesSeparateOutputRenderTarget &&
					vraf.postProcess &&
					::System::VideoConfig::general_is_post_process_allowed())
				{
					options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoFXAA), &videoOptions.fxaa)
									.add(0, NAME(lsVideoFXAAOff))
									.add_if(!videoOptions.directlyToVR, 1, NAME(lsVideoFXAAOn))
									);

					options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoBloom), &videoOptions.bloom)
									.add(0, NAME(lsVideoBloomOff))
									.add_if(!videoOptions.directlyToVR, 1, NAME(lsVideoBloomOn))
									);
				}
			}

			/*
			if (type == Main)
			{
				options.push_back(HubScreenOption()); -----
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoExtraWindows), &videoOptions.extraWindows)
									.add(0, NAME(lsVideoExtraWindowsOff))
									.add(1, NAME(lsVideoExtraWindowsOn))
									);
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoExtraDoors), &videoOptions.extraDoors)
									.add(0, NAME(lsVideoExtraDoorsOff))
									.add(1, NAME(lsVideoExtraDoorsOn))
									);
			}
			else
			{
				options.push_back(HubScreenOption()); -----
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoExtraWindows), &videoOptions.extraWindows, HubScreenOption::Text)
									.with_text(videoOptions.extraWindows? LOC_STR(NAME(lsVideoExtraWindowsOn)) : LOC_STR(NAME(lsVideoExtraWindowsOff)))
									);
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoExtraDoors), &videoOptions.extraDoors, HubScreenOption::Text)
									.with_text(videoOptions.extraWindows? LOC_STR(NAME(lsVideoExtraDoorsOn)) : LOC_STR(NAME(lsVideoExtraDoorsOff)))
									);
			}
			*/

			if (auto* vr = VR::IVR::get())
			{
				options.push_back(HubScreenOption()); /* ----- */

#ifndef AN_WINDOWS
				if (vraf.directToVR)
				{
					options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoDirectlyToVR), &videoOptions.directlyToVR)
								.add(0, NAME(lsVideoDirectlyToVROff))
								.add(1, NAME(lsVideoDirectlyToVROn))
								.do_post_update([this]() { show_screen(VideoOptionsScreen, true); })
								);
				}
#endif
#ifdef USE_LESS_PIXEL_FOR_FOVEATED_RENDERING
			}
			{
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoVRFoveatedRenderingLevel), &videoOptions.vrFoveatedRenderingLevel, HubScreenOption::Slider)
					.with_slider_width_pct(0.4f)
					.to_get_text_for_slider([](int _v) {return String::printf(TXT("%i%%"), _v); })
					.slider_range(0, 200)
					.slider_step(10)
					.with_on_hold([this]() {::System::FoveatedRenderingSetup::test_use_less_pixel_for_foveated_rendering((float)videoOptions.vrFoveatedRenderingLevel / 100.0f, videoOptions.vrFoveatedRenderingMaxDepth); })
					.with_on_release([]() {::System::FoveatedRenderingSetup::test_use_less_pixel_for_foveated_rendering(NP, NP); })
				);
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoVRFoveatedRenderingMaxDepth), &videoOptions.vrFoveatedRenderingMaxDepth, HubScreenOption::Slider)
					.with_slider_width_pct(0.4f)
					.to_get_text_for_slider([](int _v) {return String::printf(TXT("%i"), _v); })
					.slider_range(1, ::System::FoveatedRenderingSetup::get_use_less_pixel_for_foveated_rendering_max_depth())
					.with_on_hold([this]() {::System::FoveatedRenderingSetup::test_use_less_pixel_for_foveated_rendering((float)videoOptions.vrFoveatedRenderingLevel / 100.0f, videoOptions.vrFoveatedRenderingMaxDepth); })
					.with_on_release([]() {::System::FoveatedRenderingSetup::test_use_less_pixel_for_foveated_rendering(NP, NP); })
				);
			}
			if (auto* vr = VR::IVR::get())
			{
#else
				if (vraf.foveatedRenderingMax > 0)
				{
					options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoVRFoveatedRenderingLevel), &videoOptions.vrFoveatedRenderingLevel, HubScreenOption::Slider)
								.with_slider_width_pct(0.4f)
								.to_get_text_for_slider([](int _v) {return String::printf(TXT("%i"), _v); })
								.slider_range(0, vraf.foveatedRenderingMax)
								);

					int minDepth = 2;
					if (vraf.foveatedRenderingMaxDepth > minDepth)
					{
						options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoVRFoveatedRenderingMaxDepth), &videoOptions.vrFoveatedRenderingMaxDepth, HubScreenOption::Slider)
									.with_slider_width_pct(0.4f)
									.to_get_text_for_slider([](int _v)
										{
											//int m = TypeConversions::Normal::f_i_closest(pow(2.0f, (float)(max(1, _v) - 1.0f)));
											//return String::printf(TXT("%i (%ix%i)"), _v, m, m);
											if (_v == 2) return String(TXT("2 (1x1)"));
											if (_v == 3) return String(TXT("3 (1x2)"));
											if (_v == 4) return String(TXT("4 (2x2)"));
											if (_v == 5) return String(TXT("5 (2x4)"));
											if (_v == 6) return String(TXT("6 (4x4)"));
											return String::printf(TXT("%i"), _v);
										})
									.slider_range(minDepth, vraf.foveatedRenderingMaxDepth)
									);
					}
					options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), nullptr, HubScreenOption::Button)
								.with_text_on_button(LOC_STR(NAME(lsVideoVRFoveatedRenderingTest)))
								.with_on_hold([this](){requestFoveatedRenderingTestTS.reset();})
								.with_on_release([this](){requestFoveatedRenderingTestTS.reset(-10.0f);})
								);
				}
#endif

				options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoVRResolutionCoef), &videoOptions.vrResolutionCoef, HubScreenOption::Slider)
								.with_slider_width_pct(0.4f)
								.to_get_text_for_slider([](int _v) {return String::printf(TXT("x%.1f"), (float)_v / 100.0f); })
								.slider_range(40, 200)
								.slider_step(10)
								);

				if (!videoOptions.directlyToVR &&
					usesSeparateOutputRenderTarget)
				{
					options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoAutoScale), &videoOptions.allowAutoScale)
								.add(0, NAME(lsVideoAutoScaleOff))
								.add_if(!videoOptions.directlyToVR, 1, NAME(lsVideoAutoScaleOn))
								);
				}
			}

			options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoAspectRatioCoef), &videoOptions.aspectRatioCoef, HubScreenOption::Slider)
								.with_slider_width_pct(0.4f)
								.to_get_text_for_slider([](int _v) {
									float aspectRatioCoef = pow(2.0f, (float)_v / 100.0f);
									if (aspectRatioCoef > 1.0f)
									{
										return String::printf(TXT("Y x%.0f%%"), (1.0f / aspectRatioCoef) * 100.0f);
									}
									else if (aspectRatioCoef < 1.0f)
									{
										return String::printf(TXT("X x%.0f%%"), aspectRatioCoef * 100.0f);
									}
									else
									{
										return LOC_STR(NAME(lsVideoAspectRatioCoefNone));
									}
								})	
								.slider_range(-200, 200)
								.slider_step(10)
								);

			bool allowScreenOptions = true;
			if (auto * vr = VR::IVR::get())
			{
				allowScreenOptions = vraf.renderToScreen;
			}

			if (allowScreenOptions)
			{
				options.push_back(HubScreenOption()); /* ----- */

				options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), nullptr, HubScreenOption::Button)
								.with_text_on_button(LOC_STR(NAME(lsVideoDisplayOnScreenLayout)))
								.with_on_click([this]() { show_screen(VideoOptionsLayoutScreen); })
								);

				options.push_back(HubScreenOption()); /* ----- */

				if (::System::Video::get_display_count() > 1)
				{
					HubScreenOption opt = HubScreenOption(Name::invalid(), NAME(lsVideoDisplay), &videoOptions.displayIndex);
					for_count(int, i, ::System::Video::get_display_count())
					{
						opt.add(i, String::printf(TXT("%i"), i + 1));
					}
					opt.do_post_update([this]() { show_screen(VideoOptionsScreen, true); });
					options.push_back(opt);
				}

				update_resolutions();

				{
					HubScreenOption opt = HubScreenOption(NAME(idResolutions), NAME(lsVideoResolution), &videoOptions.resolutionIndex, HubScreenOption::List);
					for_count(int, i, resolutionsForDisplay.get_size())
					{
						opt.add(i, String::printf(TXT("%i x %i"), resolutionsForDisplay[i].x, resolutionsForDisplay[i].y));
					}
					opt.do_post_update([this]() { videoOptions.update_resolution(resolutionsForDisplay); });
					options.push_back(opt);
				}

				options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoFullScreen), &videoOptions.fullScreen)
								.add(System::FullScreen::No, NAME(lsVideoFullScreenNo))
								.add(System::FullScreen::WindowScaled, NAME(lsVideoFullScreenWindowScaled))
								.add(System::FullScreen::Yes, NAME(lsVideoFullScreenYes))
								);

				options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoDisplayOnScreen), &videoOptions.displayOnScreen)
								.add(0, NAME(lsVideoDisplayOnScreenNo))
								.add(1, NAME(lsVideoDisplayOnScreenYes))
								);
			}
			options.push_back(HubScreenOption()); /* ----- */
			options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), nullptr, HubScreenOption::Button)
								.with_text_on_button(LOC_STR(NAME(lsVideoToDefaults)))
								.with_on_click([this]() {reset_video_options();
#ifdef USE_LESS_PIXEL_FOR_FOVEATED_RENDERING
									::System::FoveatedRenderingSetup::test_use_less_pixel_for_foveated_rendering(NP, NP);
#endif
									})
								);

			scr->add_option_widgets(options, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y)), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f, scr->mainResolutionInPixels.y.max - max(fs.x, fs.y) * 1.0f)));

			Array<HubScreenButtonInfo> buttons;
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsApply), [this]() { apply_video_options();
#ifdef USE_LESS_PIXEL_FOR_FOVEATED_RENDERING
				::System::FoveatedRenderingSetup::test_use_less_pixel_for_foveated_rendering(NP, NP);
#endif
				}).activate_on_trigger_hold());
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsCancel), [this]() {
#ifdef USE_LESS_PIXEL_FOR_FOVEATED_RENDERING
				::System::FoveatedRenderingSetup::test_use_less_pixel_for_foveated_rendering(NP, NP);
#endif
				show_screen(MainScreen, true); }));

			scr->add_button_widgets(buttons, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y) * 6.0f), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 1.0f, scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f)), max(fs.x, fs.y) * 2.0f);

			scr->compress_vertically();
		}
	} break;
	case VideoOptionsLayoutScreen:
	{
		bool fill = _refresh;
		auto* scr = get_hub()->get_screen(NAME(optionsVideoLayout));
		if (!scr)
		{
			scr = new HubScreen(get_hub(), NAME(optionsVideoLayout), Vector2(60.0f * sizeCoef, 120.0f), forwardDir, get_hub()->get_radius() * 0.55f, NP, NP, ppa);
			scr->followScreen = NAME(optionsMain);
			get_hub()->add_screen(scr);
			scr->be_modal();
			fill = true;

			// options are read already
		}
		else if (fill)
		{
			Vector2 size = scr->get_size();
			size.y = 120.0f;
			scr->set_size(size, false);
		}
		if (fill)
		{
			scr->clear();

			Vector2 fs = HubScreen::s_fontSizeInPixels;

			Array<HubScreenOption> options;
			options.make_space_for(20);

			options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoLayoutShow), &videoOptions.layoutShowSecondary)
								.add(0, NAME(lsVideoLayoutShowOff))
								.add(1, NAME(lsVideoLayoutShowOn))
								.do_post_update([this]() { show_screen(VideoOptionsLayoutScreen, true); })
								);
			if (videoOptions.layoutShowSecondary)
			{
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoLayoutPIP), &videoOptions.layoutPIP)
								.add(0, NAME(lsVideoLayoutPIPOff))
								.add(1, NAME(lsVideoLayoutPIPOn))
								.do_post_update([this]() { show_screen(VideoOptionsLayoutScreen, true); })
								);
				if (videoOptions.layoutPIP)
				{
					options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoLayoutPIPSwap), &videoOptions.layoutPIPSwap)
								.add(0, NAME(lsVideoLayoutPIPSwapOff))
								.add(1, NAME(lsVideoLayoutPIPSwapOn))
								);
				}
			}
			options.push_back(HubScreenOption()); /* ----- */
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoLayoutMain)));
			if (videoOptions.layoutShowSecondary)
			{
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoLayoutSize), &videoOptions.layoutMainSize, HubScreenOption::Slider)
								.with_slider_width_pct(0.4f)
								.to_get_text_for_slider([](int _v) {return String::printf(TXT("%i%%"), _v); })
								.slider_range(10, 100)
								.slider_step(5)
								);
			}
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoLayoutZoom), &videoOptions.layoutMainZoom)
								.add(0, NAME(lsVideoLayoutZoomOff))
								.add(1, NAME(lsVideoLayoutZoomOn))
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoLayoutPlacement)));
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoLayoutPlacementHorizontal), &videoOptions.layoutMainPlacementX, HubScreenOption::Slider)
								.with_slider_width_pct(0.4f)
								.to_get_text_for_slider([](int _v) {
										String result;
										if (_v < 50) result += LOC_STR(NAME(lsVideoLayoutPlacementHorizontalLeft)) + String::space();
										if (_v > 50) result += LOC_STR(NAME(lsVideoLayoutPlacementHorizontalRight)) + String::space();
										return result + String::printf(TXT("%i%%"), (_v - 50) * 2);
									})
								.slider_range(0, 100)
								.slider_step(5)
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoLayoutPlacementVertical), &videoOptions.layoutMainPlacementY, HubScreenOption::Slider)
								.with_slider_width_pct(0.4f)
								.to_get_text_for_slider([](int _v) {
										String result;
										if (_v < 50) result += LOC_STR(NAME(lsVideoLayoutPlacementVerticalBottom)) + String::space();
										if (_v > 50) result += LOC_STR(NAME(lsVideoLayoutPlacementVerticalTop)) + String::space();
										return result + String::printf(TXT("%i%%"), (_v - 50) * 2);
									})
								.slider_range(0, 100)
								.slider_step(5)
								);

			if (videoOptions.layoutShowSecondary)
			{
				options.push_back(HubScreenOption()); /* ----- */
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoLayoutSecondary)));
				if (videoOptions.layoutPIP)
				{
					options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoLayoutSize), &videoOptions.layoutPIPSize, HubScreenOption::Slider)
								.with_slider_width_pct(0.4f)
								.to_get_text_for_slider([](int _v) {return String::printf(TXT("%i%%"), _v); })
								.slider_range(10, 90)
								.slider_step(5)
								);
				}
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoLayoutZoom), &videoOptions.layoutSecondaryZoom)
								.add(0, NAME(lsVideoLayoutZoomOff))
								.add(1, NAME(lsVideoLayoutZoomOn))
								);
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoLayoutPlacement)));
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoLayoutPlacementHorizontal), &videoOptions.layoutSecondaryPlacementX, HubScreenOption::Slider)
								.with_slider_width_pct(0.4f)
								.to_get_text_for_slider([](int _v) {
										String result;
										if (_v < 50) result += LOC_STR(NAME(lsVideoLayoutPlacementHorizontalLeft)) + String::space();
										if (_v > 50) result += LOC_STR(NAME(lsVideoLayoutPlacementHorizontalRight)) + String::space();
										return result + String::printf(TXT("%i%%"), (_v - 50) * 2);
									})
								.slider_range(0, 100)
								.slider_step(5)
								);
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoLayoutPlacementVertical), &videoOptions.layoutSecondaryPlacementY, HubScreenOption::Slider)
								.with_slider_width_pct(0.4f)
								.to_get_text_for_slider([](int _v) {
										String result;
										if (_v < 50) result += LOC_STR(NAME(lsVideoLayoutPlacementVerticalBottom)) + String::space();
										if (_v > 50) result += LOC_STR(NAME(lsVideoLayoutPlacementVerticalTop)) + String::space();
										return result + String::printf(TXT("%i%%"), (_v - 50) * 2);
									})
								.slider_range(0, 100)
								.slider_step(5)
								);
			}
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
			if (videoOptions.layoutShowSecondary)
			{
				options.push_back(HubScreenOption()); /* ----- */
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsVideoLayoutBoundary), &videoOptions.layoutShowBoundary)
								.add(TeaForGodEmperor::ExternalViewShowBoundary::No, NAME(lsVideoLayoutBoundaryOff))
								.add(TeaForGodEmperor::ExternalViewShowBoundary::FloorOnly, NAME(lsVideoLayoutBoundaryFloorOnly))
								.add(TeaForGodEmperor::ExternalViewShowBoundary::BackWalls, NAME(lsVideoLayoutBoundaryBackWalls))
								.add(TeaForGodEmperor::ExternalViewShowBoundary::AllWalls, NAME(lsVideoLayoutBoundaryAllWalls))
								);
			}
#endif

			scr->add_option_widgets(options, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y)), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f, scr->mainResolutionInPixels.y.max - max(fs.x, fs.y) * 1.0f)));

			Array<HubScreenButtonInfo> buttons;
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsBack), [this]() { show_screen(VideoOptionsScreen, true); }));

			scr->add_button_widgets(buttons, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y) * 2.0f), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 0.5f, scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.0f)), max(fs.x, fs.y) * 2.0f);

			scr->compress_vertically();
		}
	} break;
	case GameplayOptionsScreen:
	{
		bool fill = _refresh;
		auto* scr = get_hub()->get_screen(NAME(optionsGameplay));
		if (!scr)
		{
			scr = new HubScreen(get_hub(), NAME(optionsGameplay), Vector2(60.0f * sizeCoef, 120.0f), forwardDir, get_hub()->get_radius() * 0.55f, NP, NP, ppa);
			scr->followScreen = NAME(optionsMain);
			get_hub()->add_screen(scr);
			scr->be_modal();
			fill = true;

			usedGameplayOptions.read();
			gameplayOptions = usedGameplayOptions;
		}
		else if (fill)
		{
			Vector2 size = scr->get_size();
			size.y = 120.0f;
			scr->set_size(size, false);
		}
		if (fill)
		{
			scr->clear();

			Vector2 fs = HubScreen::s_fontSizeInPixels;

			Array<HubScreenOption> options;
			options.make_space_for(20);

			if (!TeaForGodEmperor::GameSettings::get().difficulty.mapAlwaysAvailable &&
				!TeaForGodEmperor::GameSettings::get().difficulty.noOverlayInfo)
			{
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplayMapAvailable), &gameplayOptions.mapAvailable)
								.add(0, NAME(lsGameplayMapAvailableOff))
								.add(1, NAME(lsGameplayMapAvailableOn))
								);
			}			

			if (TeaForGodEmperor::GameSettings::get().difficulty.showDirectionsOnlyToObjective)
			{
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplayFollowGuidance), &gameplayOptions.followGuidance)
								.add(Option::Disallow, NAME(lsGameplayFollowGuidanceDisallow))
								.add(Option::Allow, NAME(lsGameplayFollowGuidanceAllow))
								);
			}			

			if (type == Main)
			{
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplayDoorDirections), &gameplayOptions.doorDirections)
								.add(TeaForGodEmperor::DoorDirectionsVisible::Auto, NAME(lsGameplayDoorDirectionsAuto))
								.add(TeaForGodEmperor::DoorDirectionsVisible::Above, NAME(lsGameplayDoorDirectionsAbove))
								.add(TeaForGodEmperor::DoorDirectionsVisible::AtEyeLevel, NAME(lsGameplayDoorDirectionsAtEyeLevel))
								);
			}
			else
			{
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplayDoorDirections), nullptr, HubScreenOption::Text)
								.with_text(gameplayOptions.doorDirections == TeaForGodEmperor::DoorDirectionsVisible::Above ? LOC_STR(NAME(lsGameplayDoorDirectionsAbove)) :
										  (gameplayOptions.doorDirections == TeaForGodEmperor::DoorDirectionsVisible::AtEyeLevel ? LOC_STR(NAME(lsGameplayDoorDirectionsAtEyeLevel)) :
											LOC_STR(NAME(lsGameplayDoorDirectionsAuto)))));
			}

			if (!options.is_empty())
			{
				options.push_back(HubScreenOption()); /* ----- */
			}

			{
				options.push_back(HubScreenOption(NAME(idSights), NAME(lsGameplaySights), nullptr, HubScreenOption::Static));

				options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplaySightsOuterRingRadius), &gameplayOptions.sightsOuterRingRadius, HubScreenOption::Slider)
								.with_slider_width_pct(0.35f)
								.slider_range(0, 80)
								.slider_step(4)
								.to_get_text_for_slider([](int _v) { return String::printf(TXT("%i%%"), (_v * 100) / 40); }) // 40 is default value
								);

				options.push_back(HubScreenOption(Name::invalid(), NAME(lsColourComponentRed), &gameplayOptions.sightsColourRed, HubScreenOption::Slider)
								.with_slider_width_pct(0.35f)
								.slider_range(0, 100)
								.slider_step(5)
								.to_get_text_for_slider([](int _v) { return String::printf(TXT("%i%%"), _v); })
								.with_on_hold([this](){update_sights_colour(true);})
								.with_on_release([this](){update_sights_colour(false);})
								);
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsColourComponentGreen), &gameplayOptions.sightsColourGreen, HubScreenOption::Slider)
								.with_slider_width_pct(0.35f)
								.slider_range(0, 100)
								.slider_step(5)
								.to_get_text_for_slider([](int _v) { return String::printf(TXT("%i%%"), _v); })
								.with_on_hold([this](){update_sights_colour(true);})
								.with_on_release([this](){update_sights_colour(false);})
								);
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsColourComponentBlue), &gameplayOptions.sightsColourBlue, HubScreenOption::Slider)
								.with_slider_width_pct(0.35f)
								.slider_range(0, 100)
								.slider_step(5)
								.to_get_text_for_slider([](int _v) { return String::printf(TXT("%i%%"), _v); })
								.with_on_hold([this](){update_sights_colour(true);})
								.with_on_release([this](){update_sights_colour(false);})
								);
			}

			options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplayHitIndicatorType), &gameplayOptions.hitIndicatorType)
								.add(TeaForGodEmperor::HitIndicatorType::Off, NAME(lsGameplayHitIndicatorTypeOff))
								.add(TeaForGodEmperor::HitIndicatorType::SidesOnly, NAME(lsGameplayHitIndicatorTypeOn))
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplayHealthIndicator), &gameplayOptions.healthIndicator)
								.add(0, NAME(lsGameplayHealthIndicatorOff))
								.add(1, NAME(lsGameplayHealthIndicatorOn))
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplayHealthIndicatorIntensity), &gameplayOptions.healthIndicatorIntensity, HubScreenOption::Slider)
								.with_slider_width_pct(0.35f)
								.slider_range(0, 100)
								.slider_step(5)
								.to_get_text_for_slider([](int _v) { return String::printf(TXT("%i%%"), _v); })
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplayHitIndicatorIntensity), &gameplayOptions.hitIndicatorIntensity, HubScreenOption::Slider)
								.with_slider_width_pct(0.35f)
								.slider_range(0, 100)
								.slider_step(5)
								.to_get_text_for_slider([](int _v) { return String::printf(TXT("%i%%"), _v); })
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplayHighlightInteractives), &gameplayOptions.highlightInteractives)
								.add(0, NAME(lsGameplayHighlightInteractivesOff))
								.add(1, NAME(lsGameplayHighlightInteractivesOn))
								);
			options.push_back(HubScreenOption()); /* ----- */
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplayKeepInWorld), &gameplayOptions.keepInWorld)
								.add(1, NAME(lsGameplayKeepInWorldOn))
								.add(0, NAME(lsGameplayKeepInWorldOff))
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplayKeepInWorldSensitivity), &gameplayOptions.keepInWorldSensitivity, HubScreenOption::Slider)
								.with_slider_width_pct(0.35f)
								.slider_range(0, 100)
								.slider_step(5)
								.to_get_text_for_slider([](int _v) { return String::printf(TXT("%i%%"), _v); })
								);
			options.push_back(HubScreenOption()); /* ----- */
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplayPocketsVerticalAdjustment), &gameplayOptions.pocketsVerticalAdjustment, HubScreenOption::Slider)
								.with_slider_width_pct(0.35f)
								.slider_range(-100, 100)
								.slider_step(5)
								.to_get_text_for_slider([](int _v)
								{
									MeasurementSystem::Type ms = TeaForGodEmperor::PlayerSetup::get_current().get_preferences().get_measurement_system();
									float vInMeters = (float)_v * MAX_POCKETS_VERTICAL_ADJUSTMENT / 100.0f;
									if (ms == MeasurementSystem::Imperial)
									{
										return MeasurementSystem::as_inches(vInMeters, NP, TXT("+"), 2);
									}
									else
									{
										return MeasurementSystem::as_centimeters(vInMeters, NP, TXT("+"), 0);
									}
								})
								);
			options.push_back(HubScreenOption()); /* ----- */
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplayPilgrimOverlayLockedToHead), &gameplayOptions.pilgrimOverlayLockedToHead)
								.add(Option::Off, NAME(lsGameplayPilgrimOverlayLockedToHeadOff))
								.add(Option::On, NAME(lsGameplayPilgrimOverlayLockedToHeadOn))
								.add(Option::Allow, NAME(lsGameplayPilgrimOverlayLockedToHeadAllow))
								);

			options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplayShowInHandEquipmentStatsAtGlance), &gameplayOptions.showInHandEquipmentStatsAtGlance)
								.add(0, NAME(lsGameplayShowInHandEquipmentStatsAtGlanceOff))
								.add(1, NAME(lsGameplayShowInHandEquipmentStatsAtGlanceOn))
								);
			if (auto* gid = Framework::GameInputDefinitions::get_definition(NAME(game))) // hardcoded
			{
				for_every(opt, gid->get_options())
				{
					if (opt->name == NAME(cs_blockOverlayInfoOnHeadSideTouch) &&
						Framework::GameInput::check_condition(opt->condition.get()))
					{
						options.push_back(HubScreenOption(Name::invalid(), opt->get_loc_str_id(), &gameplayOptions.overlayInfoOnHeadSideTouch)
								.add(0, NAME(lsControlsGameInputOptionOff))
								.add(1, NAME(lsControlsGameInputOptionOn))
								);
					}
				}
			}
			options.push_back(HubScreenOption()); /* ----- */
			options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), nullptr)
								.add(0, NAME(lsGameplayToDefaults))
								.do_post_update([this](){reset_gameplay_options();})
								);
			options.push_back(HubScreenOption()); /* ----- */
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplayTurnCounter), &gameplayOptions.turnCounter)
								.add(Option::Off, NAME(lsGameplayTurnCounterOff))
								.add(Option::Allow, NAME(lsGameplayTurnCounterAllow))
								.add(Option::On, NAME(lsGameplayTurnCounterOn))
								);
			options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), nullptr)
								.add(0, NAME(lsGameplayTurnCounterReset))
								.do_post_update([this](){reset_turn_counter();})
								);
			options.push_back(HubScreenOption()); /* ----- */
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplayShowSavingIcon), &gameplayOptions.showSavingIcon)
								.add(0, NAME(lsGameplayShowSavingIconOff))
								.add(1, NAME(lsGameplayShowSavingIconOn))
								);

			scr->add_option_widgets(options, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y)), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f, scr->mainResolutionInPixels.y.max - max(fs.x, fs.y) * 1.0f)));

			Array<HubScreenButtonInfo> buttons;
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsApply), [this]() { apply_gameplay_options(); }).activate_on_trigger_hold());
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsCancel), [this]() { gameplayOptions = usedGameplayOptions; apply_gameplay_options(); }));

			scr->add_button_widgets(buttons, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y) * 6.0f), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 1.0f, scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f)), max(fs.x, fs.y) * 2.0f);

			scr->compress_vertically();
		}
	} break;
	case ComfortOptionsScreen:
	{
		bool fill = _refresh;
		auto* scr = get_hub()->get_screen(NAME(optionsComfort));
		if (!scr)
		{
			scr = new HubScreen(get_hub(), NAME(optionsComfort), Vector2(60.0f * sizeCoef, 120.0f), forwardDir, get_hub()->get_radius() * 0.55f, NP, NP, ppa);
			scr->followScreen = NAME(optionsMain);
			get_hub()->add_screen(scr);
			scr->be_modal();
			fill = true;

			usedComfortOptions.read();
			comfortOptions = usedComfortOptions;
		}
		else if (fill)
		{
			Vector2 size = scr->get_size();
			size.y = 120.0f;
			scr->set_size(size, false);
		}
		if (fill)
		{
			scr->clear();

			Vector2 fs = HubScreen::s_fontSizeInPixels;

			Array<HubScreenOption> options;
			options.make_space_for(20);

			options.push_back(HubScreenOption(NAME(idForcedEnvironmentMix), NAME(lsComfortForcedEnvironmentMixPt), &comfortOptions.forcedEnvironmentMix)
								.add(0, NAME(lsComfortForcedEnvironmentMixPtOff))
								.add(1, NAME(lsComfortForcedEnvironmentMixPtOn))
								.do_post_update([this]() { comfortOptions.ignoreForcedEnvironmentMixPt = ! comfortOptions.forcedEnvironmentMix; })
								);
			options.push_back(HubScreenOption(NAME(idForcedEnvironmentMixPt), Name::invalid(), &comfortOptions.forcedEnvironmentMixPt, HubScreenOption::Slider)
								.with_slider_width_pct(0.8f)
								.slider_range(0, 24 * 60)
								.slider_step(15)
								.do_post_update([this]() { comfortOptions.ignoreForcedEnvironmentMixPt = 0; })
								.to_get_text_for_slider([](int _v)
									{
										Framework::DateTime dt;
										dt.set_date(1, 1, 1); // we need anything there
										dt.set_time(_v / 60, mod(_v, 60));
										return dt.get_string_compact_time(Framework::DateTimeFormat());
									})
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsComfortVignetteForMovement), &comfortOptions.useVignetteForMovement)
								.add(0, NAME(lsComfortVignetteForMovementOff))
								.add(1, NAME(lsComfortVignetteForMovementOn))
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsComfortVignetteForMovementStrength), &comfortOptions.vignetteForMovementStrength, HubScreenOption::Slider)
								.with_slider_width_pct(0.4f)
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsComfortCameraRumble), &comfortOptions.cameraRumble)
								.add(Option::Disallow, NAME(lsComfortCameraRumbleDisallow))
								.add(Option::Allow, NAME(lsComfortCameraRumbleAllow))
								);
			options.push_back(HubScreenOption()); /* ----- */
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsComfortCartsTopSpeed), &comfortOptions.cartsTopSpeed/*, HubScreenOption::List*/)
								.add(0, NAME(lsComfortCartsTopSpeedUnlimited))
								.add(150, NAME(lsComfortCartsTopSpeedSlow))
								.add(75, NAME(lsComfortCartsTopSpeedVerySlow))
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsComfortCartsSpeedPct), &comfortOptions.cartsSpeedPct/*, HubScreenOption::List*/)
								.add(100, NAME(lsComfortCartsSpeedPctNormal))
								.add(75, NAME(lsComfortCartsSpeedPctSlow))
								.add(50, NAME(lsComfortCartsSpeedPctSlower))
								.add(25, NAME(lsComfortCartsSpeedPctVerySlow))
								);
			options.push_back(HubScreenOption()); /* ----- */
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsComfortFlickeringLights), &comfortOptions.flickeringLights)
								.add(Option::Disallow, NAME(lsComfortFlickeringLightsDisallow))
								.add(Option::Allow, NAME(lsComfortFlickeringLightsAllow))
								);
			options.push_back(HubScreenOption()); /* ----- */
			if (type == Main)
			{
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsComfortRotatingElevators), &comfortOptions.rotatingElevators)
								.add(Option::Disallow, NAME(lsComfortRotatingElevatorsDisallow))
								.add(Option::Allow, NAME(lsComfortRotatingElevatorsAllow))
								);
			}
			else
			{
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsComfortRotatingElevators), nullptr, HubScreenOption::Text)
								.with_text(comfortOptions.rotatingElevators == Option::Disallow? LOC_STR(NAME(lsComfortRotatingElevatorsDisallow)) : LOC_STR(NAME(lsComfortRotatingElevatorsAllow))));
			}
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsPlayAreaAllowCrouch), &comfortOptions.allowCrouch) // shared
								.add(Option::Disallow, NAME(lsPlayAreaAllowCrouchDisallow))
								.add(Option::Allow, NAME(lsPlayAreaAllowCrouchAllow))
								.add(Option::Auto, NAME(lsPlayAreaAllowCrouchAuto))
								.do_post_update([]() { /*update_preview_play_area();*/ })
								);
			options.push_back(HubScreenOption()); /* ----- */
			options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), nullptr)
								.add(0, NAME(lsComfortToDefaults))
								.do_post_update([this]() {reset_comfort_options(); })
								);
			options.push_back(HubScreenOption()); /* ----- */
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplayTurnCounter), &comfortOptions.turnCounter)
								.add(Option::Off, NAME(lsGameplayTurnCounterOff))
								.add(Option::Allow, NAME(lsGameplayTurnCounterAllow))
								.add(Option::On, NAME(lsGameplayTurnCounterOn))
								);
			options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), nullptr)
								.add(0, NAME(lsGameplayTurnCounterReset))
								.do_post_update([this](){reset_turn_counter();})
								);


			scr->add_option_widgets(options, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y)), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f, scr->mainResolutionInPixels.y.max - max(fs.x, fs.y) * 1.0f)));

			Array<HubScreenButtonInfo> buttons;
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsApply), [this]() { apply_comfort_options(); }).activate_on_trigger_hold());
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsCancel), [this]() { comfortOptions = usedComfortOptions; apply_comfort_options(); }));

			scr->add_button_widgets(buttons, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y) * 6.0f), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 1.0f, scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f)), max(fs.x, fs.y) * 2.0f);

			scr->compress_vertically();
		}
	} break;
	case ControlsOptionsScreen:
	{
		bool fill = _refresh;
		auto* scr = get_hub()->get_screen(NAME(optionsControls));
		if (!scr)
		{
			scr = new HubScreen(get_hub(), NAME(optionsControls), Vector2(60.0f * sizeCoef, 120.0f), forwardDir, get_hub()->get_radius() * 0.55f, NP, NP, ppa);
			scr->followScreen = NAME(optionsMain);
			get_hub()->add_screen(scr);
			scr->be_modal();
			fill = true;

			usedControlsOptions.read();
			controlsOptions = usedControlsOptions;
		}
		else if (fill)
		{
			Vector2 size = scr->get_size();
			size.y = 120.0f;
			scr->set_size(size, false);
		}
		if (fill)
		{
			scr->clear();

			Vector2 fs = HubScreen::s_fontSizeInPixels;

			Array<HubScreenOption> options;
			options.make_space_for(20);

			if (type == Main)
			{
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsPlayAreaImmobileVR), &controlsOptions.immobileVR)
									.add(Option::False, NAME(lsPlayAreaImmobileVRFalse))
									.add(Option::Auto, NAME(lsPlayAreaImmobileVRAuto))
									.add(Option::True, NAME(lsPlayAreaImmobileVRTrue))
									.do_post_update([this]()
										{
											show_screen(ControlsOptionsScreen, true, true /* keep forward dir */);
										})
									);
			}
			else
			{
				options.push_back(HubScreenOption(NAME(idShowImmobileVR), NAME(lsPlayAreaImmobileVR), nullptr, HubScreenOption::Text));
			}
			if (should_appear_as_immobile_vr(controlsOptions.immobileVR))
			{
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsPlayAreaImmobileVRTurning), &controlsOptions.immobileVRSnapTurn)
									.add(0, NAME(lsPlayAreaImmobileVRTurningContinuous))
									.add(1, NAME(lsPlayAreaImmobileVRTurningSnap))
									.do_post_update([this]()
									{
										show_screen(ControlsOptionsScreen, true, true /* keep forward dir */);
									})
								);
				if (controlsOptions.immobileVRSnapTurn)
				{
					options.push_back(HubScreenOption(Name::invalid(), NAME(lsPlayAreaImmobileVRTurningSnapTurnAngle), &controlsOptions.immobileVRSnapTurnAngle, HubScreenOption::Slider)
									.with_slider_width_pct(0.4f)
									.slider_range(RangeInt(15, 90))
									.slider_step(15)
									.to_get_text_for_slider([](int _v)
									{
										return Framework::StringFormatter::format_loc_str(NAME(lsPlayAreaImmobileVRTurningSnapTurnAngleValue), Framework::StringFormatterParams()
											.add(NAME(angle), _v));
									})
								);
				}
				else
				{
					options.push_back(HubScreenOption(Name::invalid(), NAME(lsPlayAreaImmobileVRTurningContinuousTurnSpeed), &controlsOptions.immobileVRContinuousTurnSpeed, HubScreenOption::Slider)
									.with_slider_width_pct(0.4f)
									.slider_range(RangeInt(30, 270))
									.slider_step(10)
									.to_get_text_for_slider([](int _v)
									{
										return Framework::StringFormatter::format_loc_str(NAME(lsPlayAreaImmobileVRTurningContinuousTurnSpeedValue), Framework::StringFormatterParams()
											.add(NAME(angle), _v));
									})
								);
				}
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsPlayAreaImmobileVRMovementRelativeTo), &controlsOptions.immobileVRMovementRelativeTo)
									.add(TeaForGodEmperor::PlayerVRMovementRelativeTo::Head, NAME(lsPlayAreaImmobileVRMovementRelativeToHead))
									.add(TeaForGodEmperor::PlayerVRMovementRelativeTo::LeftHand, NAME(lsPlayAreaImmobileVRMovementRelativeToLeftHand))
									.add(TeaForGodEmperor::PlayerVRMovementRelativeTo::RightHand, NAME(lsPlayAreaImmobileVRMovementRelativeToRightHand))
								);
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsPlayAreaImmobileVRSpeed), &controlsOptions.immobileVRSpeed, HubScreenOption::Slider)
								.with_slider_width_pct(0.4f)
								.slider_range(RangeInt(50, 200))
								.slider_step(25)
								.to_get_text_for_slider([this](int _v)
								{
										MeasurementSystem::Type ms = playAreaOptions.get_measurement_system();
										float vInMeters = (float)_v / 100.0f;
										if (ms == MeasurementSystem::Imperial)
										{
											return MeasurementSystem::as_feet_speed(vInMeters);
										}
										else
										{
											return MeasurementSystem::as_meters_speed(vInMeters);
										}
									})
							);
			}
			options.push_back(HubScreenOption()); /* ----- */

			options.push_back(HubScreenOption(Name::invalid(), NAME(lsControlsAutoMainEquipment), &controlsOptions.autoMainEquipment)
								.add(0, NAME(lsControlsAutoMainEquipmentNo))
								.add(1, NAME(lsControlsAutoMainEquipmentDependsOnController))
								.add(2, NAME(lsControlsAutoMainEquipmentYes))
								);

			options.push_back(HubScreenOption()); /* ----- */

			for_every(opt, controlsOptions.options)
			{
				options.push_back(HubScreenOption(Name::invalid(), opt->locStrId, &opt->onOff)
								.add(0, NAME(lsControlsGameInputOptionOff))
								.add(1, NAME(lsControlsGameInputOptionOn))
								);
			}
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsControlsJoystickInputBlocked), &controlsOptions.joystickInputBlocked)
								.add(0, NAME(lsControlsJoystickInputBlockedOff))
								.add(1, NAME(lsControlsJoystickInputBlockedOn))
								);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsControlsJoystickSwap), &controlsOptions.joystickSwap)
								.add(0, NAME(lsControlsJoystickSwapOff))
								.add(1, NAME(lsControlsJoystickSwapOn))
								);

			options.push_back(HubScreenOption()); /* ----- */

			options.push_back(HubScreenOption(Name::invalid(), NAME(lsControlsDominantHand), &controlsOptions.dominantHand)
								.add(0, NAME(lsControlsDominantHandAuto))
								.add(1, NAME(lsControlsDominantHandRight))
								.add(-1, NAME(lsControlsDominantHandLeft))
								);

			options.push_back(HubScreenOption(Name::invalid(), NAME(lsControlsVROffsetsOpenInfo), nullptr, HubScreenOption::Button)
								.with_text_on_button(LOC_STR(NAME(lsControlsVROffsetsOpen)))
								.with_on_click(
									[this]()
									{
										show_screen(ControlsVROffsetsScreen);
									})
								);

			scr->add_option_widgets(options, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y)), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f, scr->mainResolutionInPixels.y.max - max(fs.x, fs.y) * 1.0f)));

			Array<HubScreenButtonInfo> buttons;
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsApply), [this]() { apply_controls_options(); }).activate_on_trigger_hold());
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsCancel), [this]() { controlsOptions = usedControlsOptions; apply_controls_options(); }));

			scr->add_button_widgets(buttons, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y) * 6.0f), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 1.0f, scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f)), max(fs.x, fs.y) * 2.0f);

			scr->compress_vertically();
		}
	} break;
	case ControlsVROffsetsScreen:
	{
		bool fill = _refresh;
		auto* scr = get_hub()->get_screen(NAME(optionsControlsVROffsets));
		if (!scr)
		{
			scr = new HubScreen(get_hub(), NAME(optionsControlsVROffsets), Vector2(60.0f * sizeCoef, 120.0f), forwardDir, get_hub()->get_radius() * 0.5f, NP, NP, ppa);
			scr->followScreen = NAME(optionsMain);
			get_hub()->add_screen(scr);
			scr->be_modal();
			fill = true;

#ifdef BUILD_NON_RELEASE
			devVROffsets.read();
#endif
		}
		else if (fill)
		{
			Vector2 size = scr->get_size();
			size.y = 120.0f;
			scr->set_size(size, false);
		}
		if (fill)
		{
			scr->clear();

			Vector2 fs = HubScreen::s_fontSizeInPixels;

			Array<HubScreenOption> options;
			options.make_space_for(20);

			options.push_back(HubScreenOption(Name::invalid(), NAME(lsControlsVROffsets), nullptr, HubScreenOption::Static));

			options.push_back(HubScreenOption(Name::invalid(), NAME(lsControlsVROffsetsPitch), &controlsOptions.vrOffsetPitch, HubScreenOption::Slider)
							.with_slider_width_pct(0.4f)
							.slider_range(RangeInt(-600, 600))
							.slider_step(50)
							.to_get_text_for_slider([](int _v)
							{
								return Framework::StringFormatter::format_loc_str(NAME(lsControlsVROffsetsAngleValue), Framework::StringFormatterParams()
									.add(NAME(angle), (float)_v / 10.0f));
							})
						);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsControlsVROffsetsYaw), &controlsOptions.vrOffsetYaw, HubScreenOption::Slider)
							.with_slider_width_pct(0.4f)
							.slider_range(RangeInt(-600, 600))
							.slider_step(50)
							.to_get_text_for_slider([](int _v)
							{
								return Framework::StringFormatter::format_loc_str(NAME(lsControlsVROffsetsAngleValue), Framework::StringFormatterParams()
									.add(NAME(angle), (float)_v / 10.0f));
							})
						);
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsControlsVROffsetsRoll), &controlsOptions.vrOffsetRoll, HubScreenOption::Slider)
							.with_slider_width_pct(0.4f)
							.slider_range(RangeInt(-1800, 1800))
							.slider_step(500)
							.to_get_text_for_slider([](int _v)
							{
								return Framework::StringFormatter::format_loc_str(NAME(lsControlsVROffsetsAngleValue), Framework::StringFormatterParams()
									.add(NAME(angle), (float)_v / 10.0f));
							})
						);

#ifdef BUILD_NON_RELEASE
			options.push_back(HubScreenOption(Name::invalid(), String(TXT("dev pitch")), &devVROffsets.vrOffsetPitch, HubScreenOption::Slider)
							.with_slider_width_pct(0.4f)
							.slider_range(RangeInt(-600, 600))
							.slider_step(50)
							.to_get_text_for_slider([](int _v)
							{
								return Framework::StringFormatter::format_loc_str(NAME(lsControlsVROffsetsAngleValue), Framework::StringFormatterParams()
									.add(NAME(angle), (float)_v / 10.0f));
							})
						);
			options.push_back(HubScreenOption(Name::invalid(), String(TXT("dev yaw")), &devVROffsets.vrOffsetYaw, HubScreenOption::Slider)
							.with_slider_width_pct(0.4f)
							.slider_range(RangeInt(-600, 600))
							.slider_step(50)
							.to_get_text_for_slider([](int _v)
							{
								return Framework::StringFormatter::format_loc_str(NAME(lsControlsVROffsetsAngleValue), Framework::StringFormatterParams()
									.add(NAME(angle), (float)_v / 10.0f));
							})
						);
			options.push_back(HubScreenOption(Name::invalid(), String(TXT("dev roll")), &devVROffsets.vrOffsetRoll, HubScreenOption::Slider)
							.with_slider_width_pct(0.4f)
							.slider_range(RangeInt(-1800, 1800))
							.slider_step(500)
							.to_get_text_for_slider([](int _v)
							{
								return Framework::StringFormatter::format_loc_str(NAME(lsControlsVROffsetsAngleValue), Framework::StringFormatterParams()
									.add(NAME(angle), (float)_v / 10.0f));
							})
						);
			options.push_back(HubScreenOption(Name::invalid(), String(TXT("dev x")), &devVROffsets.vrOffsetX, HubScreenOption::Slider)
							.with_slider_width_pct(0.4f)
							.slider_range(RangeInt(-100, 100))
							.slider_step(5)
							.to_get_text_for_slider([](int _v)
							{
								return String::printf(TXT("%.1fcm"), (float)_v / 10.0f);
							})
						);
			options.push_back(HubScreenOption(Name::invalid(), String(TXT("dev y")), &devVROffsets.vrOffsetY, HubScreenOption::Slider)
							.with_slider_width_pct(0.4f)
							.slider_range(RangeInt(-100, 100))
							.slider_step(5)
							.to_get_text_for_slider([](int _v)
							{
								return String::printf(TXT("%.1fcm"), (float)_v / 10.0f);
							})
						);
			options.push_back(HubScreenOption(Name::invalid(), String(TXT("dev z")), &devVROffsets.vrOffsetZ, HubScreenOption::Slider)
							.with_slider_width_pct(0.4f)
							.slider_range(RangeInt(-100, 100))
							.slider_step(5)
							.to_get_text_for_slider([](int _v)
							{
								return String::printf(TXT("%.1fcm"), (float)_v / 10.0f);
							})
						);
#endif

			scr->add_option_widgets(options, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y)), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f, scr->mainResolutionInPixels.y.max - max(fs.x, fs.y) * 1.0f)));

			Array<HubScreenButtonInfo> buttons;
			buttons.push_back(HubScreenButtonInfo(NAME(idTestConfirmVROffsets), NAME(lsControlsVROffsetsTest), [this]()
				{
					if (testVROffsetsTimeLeft.is_set())
					{
						testVROffsetsTimeLeft.clear();
						controlsOptions.confirm_vr_offsets();
					}
					else
					{
						apply_controls_vr_offsets_options(true);
					}
				}
				));
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsApply), [this]() { testVROffsetsTimeLeft.clear(); controlsOptions.confirm_vr_offsets();  apply_controls_vr_offsets_options(); }).activate_on_trigger_hold());
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsCancel), [this]() { testVROffsetsTimeLeft.clear(); controlsOptions.copy_vr_offsets_from(usedControlsOptions); apply_controls_vr_offsets_options(); }));

			scr->add_button_widgets(buttons,
						Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y) * 2.0f),
							   Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 1.0f, scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f)), max(fs.x, fs.y) * 2.0f);

			scr->compress_vertically();
		}
	} break;
	case PlayAreaOptionsScreen:
	{
		bool fill = _refresh;
		bool wereCalibrating = false;
		bool wereInPlayAreaOptions = false;
		if (auto * scr = get_hub()->get_screen(NAME(optionsPlayAreaCalibrate)))
		{
			// we were calibrating
			scr->deactivate();
			wereCalibrating = true;
		}
		if (auto * scr = get_hub()->get_screen(NAME(optionsPlayArea)))
		{
			// we were calibrating
			scr->deactivate();
			wereInPlayAreaOptions = true;
		}
		{
			Array<Name> screensToKeep;
			screensToKeep.push_back(NAME(hubBackground));
			//screensToKeep.push_back(NAME(optionsMain));
			//
			get_hub()->keep_only_screens(screensToKeep);
		}
		auto* scr = get_hub()->get_screen(NAME(optionsPlayArea));
		if (!scr)
		{
			scr = new HubScreen(get_hub(), NAME(optionsPlayArea), Vector2(60.0f * sizeCoef, 120.0f), forwardDir, get_hub()->get_radius() * 0.55f, NP, NP, ppa);
			scr->followScreen = NAME(optionsMain);
			get_hub()->add_screen(scr);
			scr->be_modal();
			fill = true;

			if (wereInPlayAreaOptions)
			{
				update_preview_play_area();
			}
			else if (!wereCalibrating)
			{
				usedPlayAreaOptions.read();
				playAreaOptions = usedPlayAreaOptions;
				update_preview_play_area();
			}
		}
		else if (fill)
		{
			Vector2 size = scr->get_size();
			size.y = 120.0f;
			scr->set_size(size, false);
		}
		if (fill)
		{
			scr->clear();

			Vector2 fs = HubScreen::s_fontSizeInPixels;

			Array<HubScreenOption> options;
			options.make_space_for(20);

			if (type == Main)
			{
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsPlayAreaImmobileVR), &playAreaOptions.immobileVR)
									.add(Option::False, NAME(lsPlayAreaImmobileVRFalse))
									.add(Option::Auto, NAME(lsPlayAreaImmobileVRAuto))
									.add(Option::True, NAME(lsPlayAreaImmobileVRTrue))
									.do_post_update([this]()
										{
											show_screen(PlayAreaOptionsScreen, true, true /* keep forward dir */);
											// update_preview_play_area is called above
										})
									);
				if (should_appear_as_immobile_vr(playAreaOptions.immobileVR))
				{
					options.push_back(HubScreenOption(Name::invalid(), NAME(lsPlayAreaImmobileVRSize), &playAreaOptions.immobileVRSize, HubScreenOption::Slider)
									.with_slider_width_pct(0.4f)
									.slider_range(RangeInt(200, 2000))
									.slider_step(100)
									.to_get_text_for_slider([this](int _v)
									{
										MeasurementSystem::Type ms = playAreaOptions.get_measurement_system();
										float vInMeters = (float)_v / 100.0f;
										if (ms == MeasurementSystem::Imperial)
										{
											return MeasurementSystem::as_feet_inches(vInMeters);
										}
										else
										{
											return MeasurementSystem::as_meters(vInMeters);
										}
									})
									.do_post_update([this]() { update_preview_play_area(); })
									);
				}
			}
			else
			{
				options.push_back(HubScreenOption(NAME(idShowImmobileVR), NAME(lsPlayAreaImmobileVR), nullptr, HubScreenOption::Text));
			}

			options.push_back(HubScreenOption()); /* ----- */

			options.push_back(HubScreenOption(NAME(idCurrentSize), NAME(lsPlayAreaCurrentSize), nullptr, HubScreenOption::Text));
			options.push_back(HubScreenOption(NAME(idCurrentTileCount), NAME(lsPlayAreaCurrentTileCount), nullptr, HubScreenOption::Text));
			options.push_back(HubScreenOption(NAME(idCurrentTileCountInfo), Name::invalid(), nullptr, HubScreenOption::Text));
			options.push_back(HubScreenOption(NAME(idCurrentTileSize), NAME(lsPlayAreaCurrentTileSize), nullptr, HubScreenOption::Text));
			if (type != Main)
			{
				options.push_back(HubScreenOption(NAME(idCurrentDoorHeight), NAME(lsPlayAreaCurrentDoorHeight), nullptr, HubScreenOption::Text));
				options.push_back(HubScreenOption(NAME(idCurrentScaleUpPlayer), NAME(lsPlayAreaCurrentScaleUpPlayer), nullptr, HubScreenOption::Text).with_min_lines(2));
				options.push_back(HubScreenOption(NAME(idCurrentAllowCrouch), NAME(lsPlayAreaCurrentAllowCrouch), nullptr, HubScreenOption::Text));
			}

			if (playAreaOptions.enlarged)
			{
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsPlayAreaEnlarged), nullptr, HubScreenOption::Static).with_colour(HubColours::warning()));
			}

			if (type == Main)
			{
				options.push_back(HubScreenOption()); /* ----- */

				options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplayKeepInWorld), &playAreaOptions.keepInWorld)
									.add(1, NAME(lsGameplayKeepInWorldOn))
									.add(0, NAME(lsGameplayKeepInWorldOff))
									);
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplayKeepInWorldSensitivity), &playAreaOptions.keepInWorldSensitivity, HubScreenOption::Slider)
									.with_slider_width_pct(0.4f)
									.slider_range(0, 100)
									.slider_step(5)
									.to_get_text_for_slider([](int _v) { return String::printf(TXT("%i%%"), _v); })
									);

				options.push_back(HubScreenOption()); /* ----- */

				if (! should_appear_as_immobile_vr(playAreaOptions.immobileVR))
				{
					options.push_back(HubScreenOption(Name::invalid(), NAME(lsPlayAreaHorizontalScaling), &playAreaOptions.horizontalScaling, HubScreenOption::Slider)
									.with_slider_width_pct(0.4f)
									.slider_range(RangeInt(100, 200)) // 100-200%
									//.slider_range_dynamic([](RangeInt & _range) { _range.min = (int)(MainConfig::global().get_vr_horizontal_scaling_min_value() * 100.0f);})
									.slider_step(5)
									.to_get_text_for_slider([](int _v)
									{
										int minHScaling = (int)(MainConfig::global().get_vr_horizontal_scaling_min_value() * 100.0f);
										if (_v < minHScaling)
										{
											return String::printf(TXT("+%i%%!"), minHScaling - 100);
										}
										else
										{
											return String::printf(TXT("+%i%%"), _v - 100);
										}
									})
									.do_post_update([this]() { update_preview_play_area(); })
									);
					options.push_back(HubScreenOption(Name::invalid(), NAME(lsPlayAreaBorder), &playAreaOptions.border, HubScreenOption::Slider)
									.with_slider_width_pct(0.4f)
									.slider_range(RangeInt(0, 100)) // 1m is enough?
									.to_get_text_for_slider([this](int _v)
									{
										MeasurementSystem::Type ms = playAreaOptions.get_measurement_system();
										float vInMeters = (float)_v / 100.0f;
										if (ms == MeasurementSystem::Imperial)
										{
											return MeasurementSystem::as_feet_inches(vInMeters);
										}
										else
										{
											return MeasurementSystem::as_centimeters(vInMeters);
										}
									})
									.do_post_update([this]() { update_preview_play_area(); })
									);
				}
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsPlayAreaPreferredTileSize), &playAreaOptions.preferredTileSize)
									.add(TeaForGodEmperor::PreferredTileSize::Auto, NAME(lsPlayAreaPreferredTileSizeAuto))
									.add(TeaForGodEmperor::PreferredTileSize::Smallest, NAME(lsPlayAreaPreferredTileSizeSmallest))
									.add(TeaForGodEmperor::PreferredTileSize::Medium, NAME(lsPlayAreaPreferredTileSizeMedium))
									.add(TeaForGodEmperor::PreferredTileSize::Large, NAME(lsPlayAreaPreferredTileSizeLarge))
									.add(TeaForGodEmperor::PreferredTileSize::Largest, NAME(lsPlayAreaPreferredTileSizeLargest))
									.do_post_update([this]() { update_preview_play_area(); })
									);
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsPlayAreaDoorHeight), &playAreaOptions.doorHeight, HubScreenOption::Slider)
									.with_slider_width_pct(0.4f)
									.slider_range(RangeInt(100, 300))
									.slider_step(5)
									.to_get_text_for_slider([this](int _v)
									{
										MeasurementSystem::Type ms = playAreaOptions.get_measurement_system();
										float vInMeters = (float)_v / 100.0f;
										if (ms == MeasurementSystem::Imperial)
										{
											return MeasurementSystem::as_feet_inches(vInMeters);
										}
										else
										{
											return MeasurementSystem::as_centimeters(vInMeters);
										}
									})
									.do_post_update([this]() { update_preview_play_area(); })
									);
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsPlayAreaScaleUpPlayer), &playAreaOptions.scaleUpPlayer)
									.add(0, NAME(lsPlayAreaScaleUpPlayerNo))
									.add(1, NAME(lsPlayAreaScaleUpPlayerYes))
									.do_post_update([this]() { update_preview_play_area(); })
									);
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsPlayAreaAllowCrouch), &playAreaOptions.allowCrouch)
									.add(Option::Disallow, NAME(lsPlayAreaAllowCrouchDisallow))
									.add(Option::Allow, NAME(lsPlayAreaAllowCrouchAllow))
									.add(Option::Auto, NAME(lsPlayAreaAllowCrouchAuto))
									.do_post_update([this]() { update_preview_play_area(); })
									);
				options.push_back(HubScreenOption()); /* ----- */
				options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), nullptr)
									.add(0, NAME(lsPlayAreaToDefaults))
									.do_post_update([this](){reset_play_area_options();})
									);
				options.push_back(HubScreenOption()); /* ----- */
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsGameplayTurnCounter), &playAreaOptions.turnCounter)
									.add(Option::Off, NAME(lsGameplayTurnCounterOff))
									.add(Option::Allow, NAME(lsGameplayTurnCounterAllow))
									.add(Option::On, NAME(lsGameplayTurnCounterOn))
									);
				options.push_back(HubScreenOption(Name::invalid(), Name::invalid(), nullptr)
									.add(0, NAME(lsGameplayTurnCounterReset))
									.do_post_update([this](){reset_turn_counter();})
									);
			}

			options.push_back(HubScreenOption()); /* ----- */
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsGeneralMeasurementSystem), &playAreaOptions.measurementSystem)
								.add(-1, NAME(lsGeneralMeasurementSystemAuto))
								.add(MeasurementSystem::Metric, NAME(lsGeneralMeasurementSystemMetric))
								.add(MeasurementSystem::Imperial, NAME(lsGeneralMeasurementSystemImperial))
								.do_post_update([this]() { update_preview_play_area(); })
								);

			scr->add_option_widgets(options, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y)), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f, scr->mainResolutionInPixels.y.max - max(fs.x, fs.y) * 1.0f)));

			Array<HubScreenButtonInfo> buttons;
			if (type == Main)
			{
				buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsApply), [this]() { apply_play_area_options(); }).activate_on_trigger_hold());
				buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsPlayAreaCalibrate), [this]() { show_screen(PlayAreaCalibrateScreen); }));
				buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsCancel), [this]() { playAreaOptions = usedPlayAreaOptions; apply_play_area_options(); }));
			}
			else
			{
				buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsBack), [this]() { show_screen(MainScreen, true); }));
			}

			scr->add_button_widgets(buttons, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y) * 2.0f), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 0.5f, scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.0f)), max(fs.x, fs.y) * 2.0f);

			scr->compress_vertically();

			update_preview_play_area();
		}
	} break;
	case PlayAreaCalibrateScreen:
	{
		bool fill = _refresh;
		{
			Array<Name> screensToKeep;
			screensToKeep.push_back(NAME(hubBackground));
			//screensToKeep.push_back(NAME(optionsMain));
			//
			get_hub()->keep_only_screens(screensToKeep);
		}
		auto* scr = get_hub()->get_screen(NAME(optionsPlayAreaCalibrate));
		if (scr)
		{
			scr->deactivate();
			scr = nullptr;
		}
		if (!scr)
		{
			Vector2 scrSize = Vector2(80.0f * sizeCoef, 70.0f * sizeCoef);
			scr = new HubScreen(get_hub(), NAME(optionsPlayAreaCalibrate), scrSize, forwardDir, get_hub()->get_radius() * 0.55f, NP, NP, ppa);
			if (get_hub()->get_use_scene_scale() < 0.3f)
			{
				scr->followYawDeadZone = 4.0f;
				scr->followHead = true;
			}
			else
			{
				scr->followScreen = NAME(optionsMain);
			}
			get_hub()->add_screen(scr);
			scr->be_modal();
			fill = true;

			// we come from playareaoptions
			playAreaOptionsBackupForCalibration = playAreaOptions;
			update_preview_play_area();
		}
		if (fill)
		{
			scr->clear();

			Vector2 fs = HubScreen::s_fontSizeInPixels;

			{
				Vector2 border = fs * 0.5f;
				Range2 at = scr->mainResolutionInPixels;
				at.x.min += border.x;
				at.x.max -= border.x;
				at.y.min += max(fs.x, fs.y) * 3.0f;
				at.y.max -= border.y;
				auto* w = new HubWidgets::CustomDraw(Name::invalid(), at, [this](Framework::Display* _display, Range2 const& _at)
				{
					// currently it tries to access some stuff from outside, if we need it, uncomment and fix
					if (auto * vr = VR::IVR::get())
					{
						Vector3 manualOffset = Vector3(playAreaOptions.manualOffsetX, playAreaOptions.manualOffsetY, 0.0f);
						float manualRotate = (float)playAreaOptions.manualRotate;
						//float borderAsFloat = (float)playAreaOptions.border / 100.0f;

						Transform mapVRSpace = vr->calculate_map_vr_space(manualOffset, manualRotate);
						Transform rotateVRSpace;
						{
							float controlsViewYaw = vr->get_controls_view().get_orientation().to_rotator().yaw;
							float mapVRSpaceYaw = vr->get_map_vr_space().get_orientation().to_rotator().yaw;
							rotateVRSpace = Rotator3(0.0f, controlsViewYaw + mapVRSpaceYaw, 0.0f).to_quat().to_matrix().to_transform();
						}
						
						mapVRSpace = rotateVRSpace.to_local(mapVRSpace);

						auto const& rawBoundaryPoints = vr->get_raw_boundary();
						ARRAY_STACK(Vector2, boundaryPoints, rawBoundaryPoints.get_size());
						for_every(rbp, rawBoundaryPoints)
						{
							boundaryPoints.push_back(rotateVRSpace.location_to_local(rbp->to_vector3()).to_vector2());
						}

						Range2 rbpRange = Range2::empty;
						for_every(bp, rawBoundaryPoints)
						{
							rbpRange.include(*bp);
						}

						if (rbpRange.x.is_empty() ||
							rbpRange.y.is_empty())
						{
							// have anything to show what we're doing
							Vector2 half = Vector2::zero;
							half.x = vr->get_raw_whole_play_area_rect_half_right().length();
							half.y = vr->get_raw_whole_play_area_rect_half_forward().length();

							if (playAreaOptions.manualSizeX != 0.0f &&
								playAreaOptions.manualSizeY != 0.0f)
							{
								half.x = playAreaOptions.manualSizeX * 0.5f;
								half.y = playAreaOptions.manualSizeY * 0.5f;
							}

							float halfSize = half.length();

							halfSize *= 1.5f;
							rbpRange.x = Range(-halfSize, halfSize);
							rbpRange.y = Range(-halfSize, halfSize);
						}

						if (!rbpRange.x.is_empty() &&
							!rbpRange.y.is_empty())
						{
							// no matter how we rotate, we should be scaled the same
							float scales = 0.9f * min(_at.x.length() / (2.0f * max(abs(rbpRange.x.min), abs(rbpRange.x.max))),
													  _at.y.length() / (2.0f * max(abs(rbpRange.y.min), abs(rbpRange.y.max))));
							Vector2 scale(scales, scales);
							Vector2 centre = Vector2::zero;// bpRange.centre();

							if (!boundaryPoints.is_empty())
							{
								Vector2 lastPoint = boundaryPoints.get_last();
								for_every(point, boundaryPoints)
								{
									::System::Video3DPrimitives::line_2d(Colour::blue, (lastPoint - centre) * scale + _at.centre(), (*point - centre) * scale + _at.centre(), false);
									lastPoint = *point;
								}
							}

							{
								ARRAY_STACK(Vector2, playAreaPoints, 10);

								Vector2 half = Vector2::zero;
								half.x = vr->get_raw_whole_play_area_rect_half_right().length();
								half.y = vr->get_raw_whole_play_area_rect_half_forward().length();

								if (playAreaOptions.manualSizeX != 0.0f &&
									playAreaOptions.manualSizeY != 0.0f)
								{
									half.x = playAreaOptions.manualSizeX * 0.5f;
									half.y = playAreaOptions.manualSizeY * 0.5f;
								}

								//half.x -= borderAsFloat;
								//half.y -= borderAsFloat;

								float tip = 0.1f;
								playAreaPoints.push_back(mapVRSpace.location_to_world(Vector3(-half.x,  half.y, 0.0f)).to_vector2());
								playAreaPoints.push_back(mapVRSpace.location_to_world(Vector3(-tip,     half.y, 0.0f)).to_vector2());
								playAreaPoints.push_back(mapVRSpace.location_to_world(Vector3( 0.0f,    half.y + tip, 0.0f)).to_vector2());
								playAreaPoints.push_back(mapVRSpace.location_to_world(Vector3( tip,     half.y, 0.0f)).to_vector2());
								playAreaPoints.push_back(mapVRSpace.location_to_world(Vector3( half.x,  half.y, 0.0f)).to_vector2());
								playAreaPoints.push_back(mapVRSpace.location_to_world(Vector3( half.x, -half.y, 0.0f)).to_vector2());
								playAreaPoints.push_back(mapVRSpace.location_to_world(Vector3(-half.x, -half.y, 0.0f)).to_vector2());

								{
									Vector2 lastPoint = playAreaPoints.get_last();
									for_every(point, playAreaPoints)
									{
										::System::Video3DPrimitives::line_2d(Colour::green, (lastPoint - centre) * scale + _at.centre(), (*point - centre) * scale + _at.centre(), false);
										lastPoint = *point;
									}
								}
							}

						}
					}
				});
				w->always_requires_rendering();

				scr->add_widget(w);
			}

			calibrationOption = CalibrationOption::Placement;
			visibleCalibrationOption.clear();
			visibleCalibrationSize.clear();

			{
				Range2 at = scr->mainResolutionInPixels;
				at.expand_by(-Vector2::one * max(fs.x, fs.y));
				at.y.min = at.y.max - max(fs.x, fs.y);
				auto* w = new HubWidgets::Text(NAME(idCalibrationSize), at, String::empty());				
				scr->add_widget(w);
			}

			Array<HubScreenButtonInfo> buttons;
			buttons.push_back(HubScreenButtonInfo(NAME(idCalibrationOption), NAME(lsPlayAreaCalibratePlacement), [this]()
				{
					calibrationOption = (CalibrationOption)((calibrationOption + 1) % CalibrationOption::Count);
				}));
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsPlayAreaCalibrateReset), [this]()
				{
					playAreaOptions.manualRotate = 0.0f;
					playAreaOptions.manualOffsetX = 0.0f;
					playAreaOptions.manualOffsetY = 0.0f;
					if (type == Main)
					{
						playAreaOptions.manualSizeX = 0.0f;
						playAreaOptions.manualSizeY = 0.0f;
					}
				}).activate_on_hold());
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsApply), [this]() { 
				an_assert(screen == PlayAreaCalibrateScreen);
				update_and_apply_vr_changes();
				// as we reset forward after a delay anyway
				delayed_show_screen(0.1f, PlayAreaOptionsScreen, NP, true);
				}).activate_on_trigger_hold());
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsCancel), [this]() {
				playAreaOptions = playAreaOptionsBackupForCalibration;
				update_preview_play_area(); // to show up whatever values were set before
				get_hub()->force_reset_and_update_forward();
				delayed_show_screen(0.1f, PlayAreaOptionsScreen); 
				}));

			auto* helpButton = scr->add_help_button();
			helpButton->on_click = [this, scr](HubWidget* _widget, Vector2 const& _at, HubInputFlags::Flags _flags)
			{
				if (auto* lib = TeaForGodEmperor::Library::get_current_as<TeaForGodEmperor::Library>())
				{
					TeaForGodEmperor::MessageSet* ms = nullptr;
					ms = lib->get_global_references().get<TeaForGodEmperor::MessageSet>(NAME(grHelpOptionsPlayareaCalibrate));
					if (ms)
					{
						Vector2 ppa(20.0f, 20.0f);

						Vector2 hSize = Vector2(55.0f, 38.0f);
						hSize.x = hSize.y;
						hSize.y = round(hSize.y * 0.8f);
						Rotator3 hAt = forwardDir;
						//hAt.yaw += scrSize.x * 0.5f + scrSize.x * 0.5f + spacing;
						HubScreens::MessageSetBrowser* msb = HubScreens::MessageSetBrowser::browse(ms, HubScreens::MessageSetBrowser::BrowseParams(), get_hub(), NAME(idCalibrationHelp), true, hSize, hAt, scr->radius - 0.5f, scr->beVertical, scr->verticalOffset, ppa, 1.0f, Vector2(0.0f, 1.0f));
						msb->dont_follow();
						msb->be_modal();
					}
				}
			};

			Range2 at = Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y) * 1.0f),
				Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 1.0f, scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * (1.0f + 3.5f)));
			at.x.max = helpButton->at.x.min - max(fs.x, fs.y);
			scr->add_button_widgets(buttons, at, max(fs.x, fs.y) * 1.0f);

			update_preview_play_area();
		}
	} break;
	case GeneralOptionsScreen:
	{
		bool fill = _refresh;
		auto* scr = get_hub()->get_screen(NAME(optionsGeneral));
		if (!scr)
		{
			scr = new HubScreen(get_hub(), NAME(optionsGeneral), Vector2(60.0f * sizeCoef, 120.0f), forwardDir, get_hub()->get_radius() * 0.55f, NP, NP, ppa);
			scr->followScreen = NAME(optionsMain);
			get_hub()->add_screen(scr);
			scr->be_modal();
			fill = true;

			usedGeneralOptions.read();
			generalOptions = usedGeneralOptions;
		}
		else if (fill)
		{
			Vector2 size = scr->get_size();
			size.y = 120.0f;
			scr->set_size(size, false);
		}
		if (fill)
		{
			scr->clear();

			Vector2 fs = HubScreen::s_fontSizeInPixels;

			Array<HubScreenOption> options;
			options.make_space_for(20);

			if (Framework::LocalisedStrings::get_languages().get_size() > 1)
			{
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsGeneralLanguage), &generalOptions.language, HubScreenOption::List)
					.scale_pixels_per_angle(0.8f)
					.do_post_update([this]()
						{
							apply_language();
							show_screen(GeneralOptionsScreen, true);
						})
				);
				if (Framework::LocalisedStrings::get_system_language().is_valid())
				{
					options.get_last().add(-1, NAME(lsGeneralLanguageAuto));
				}
				auto sortedLanguages = Framework::LocalisedStrings::get_sorted_languages();
				for_every(sl, sortedLanguages)
				{
					options.get_last().add(sl->idx, sl->name);
				}
			}
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsGeneralMeasurementSystem), &generalOptions.measurementSystem)
								.add(-1, NAME(lsGeneralMeasurementSystemAuto))
								.add(MeasurementSystem::Metric, NAME(lsGeneralMeasurementSystemMetric))
								.add(MeasurementSystem::Imperial, NAME(lsGeneralMeasurementSystemImperial))
								);

			options.push_back(HubScreenOption()); /* ----- */

			{
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsSoundSubtitles), &generalOptions.subtitles)
					.add(0, NAME(lsSoundSubtitlesOff))
					.add(1, NAME(lsSoundSubtitlesOn))
				);
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsSoundSubtitlesScale), &generalOptions.subtitlesScale, HubScreenOption::Slider)
					.with_slider_width_pct(0.4f)
					.slider_range(50, 200)
					.slider_step(10)
					.to_get_text_for_slider([](int _v) {return String::printf(TXT("%i%%"), _v); })
				);
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsSoundNarrativeVoiceover), &generalOptions.narrativeVoiceover)
					.add(0, NAME(lsSoundNarrativeVoiceoverOff))
					.add(1, NAME(lsSoundNarrativeVoiceoverOn))
				);
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsSoundPilgrimOverlayVoiceover), &generalOptions.pilgrimOverlayVoiceover)
					.add(0, NAME(lsSoundPilgrimOverlayVoiceoverOff))
					.add(1, NAME(lsSoundPilgrimOverlayVoiceoverOn))
				);
			}

			options.push_back(HubScreenOption(Name::invalid(), NAME(lsGeneralShowTipsOnLoading), &generalOptions.showTipsOnLoading)
								.add(0, NAME(lsGeneralShowTipsOnLoadingNo))
								.add(1, NAME(lsGeneralShowTipsOnLoadingYes))
								);

			options.push_back(HubScreenOption(Name::invalid(), NAME(lsGeneralShowTipsDuringGame), &generalOptions.showTipsDuringGame)
								.add(0, NAME(lsGeneralShowTipsDuringGameNo))
								.add(1, NAME(lsGeneralShowTipsDuringGameYes))
								);
			
			options.push_back(HubScreenOption()); /* ----- */
			
			if (type == Main)
			{
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsPlayAreaImmobileVR), &generalOptions.immobileVR)
								.add(Option::False, NAME(lsPlayAreaImmobileVRFalse))
								.add(Option::Auto, NAME(lsPlayAreaImmobileVRAuto))
								.add(Option::True, NAME(lsPlayAreaImmobileVRTrue))
								);
			}
			else
			{
				options.push_back(HubScreenOption(NAME(idShowImmobileVR), NAME(lsPlayAreaImmobileVR), nullptr, HubScreenOption::Text));
			}

			if (TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>()->does_allow_screenshots())
			{
				options.push_back(HubScreenOption()); /* ----- */

				options.push_back(HubScreenOption(Name::invalid(), NAME(lsGeneralAllowScreenShots), &generalOptions.allowVRScreenShots)
								.add(TeaForGodEmperor::AllowScreenShots::No, NAME(lsGeneralAllowScreenShotsNo))
								.add(TeaForGodEmperor::AllowScreenShots::Big, NAME(lsGeneralAllowScreenShotsYes))
								.add(TeaForGodEmperor::AllowScreenShots::Small, NAME(lsGeneralAllowScreenShotsYesSmall))
								);
			}

			VR::AvailableFunctionality vraf;
			if (auto * vr = VR::IVR::get())
			{
				vraf = vr->get_available_functionality();
			}

			if (vraf.renderToScreen)
			{
				todo_note(TXT("restore show gameplay info when implemented"));
				/*
				options.push_back(HubScreenOption(Name::invalid(), NAME(lsGeneralShowGameplayInfo), &generalOptions.showGameplayInfo)
								.add(0, NAME(lsGeneralShowGameplayInfoNo))
								.add(1, NAME(lsGeneralShowGameplayInfoYes))
								);
				*/
			}

			options.push_back(HubScreenOption()); /* ----- */
			options.push_back(HubScreenOption(Name::invalid(), NAME(lsGeneralShowRealTime), &generalOptions.showRealTime)
								.add(0, NAME(lsGeneralShowRealTimeNo))
								.add(1, NAME(lsGeneralShowRealTimeYes))
								);

			options.push_back(HubScreenOption(Name::invalid(), NAME(lsGeneralReportBugs), &generalOptions.reportBugs)
								.add(Option::False, NAME(lsGeneralReportBugsNo))
#ifndef AN_ANDROID
								.add(Option::Ask, NAME(lsGeneralReportBugsAsk))
#endif
								.add(Option::True, NAME(lsGeneralReportBugsYes))
								);
			
			scr->add_option_widgets(options, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y)), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f, scr->mainResolutionInPixels.y.max - max(fs.x, fs.y) * 1.0f)));

			Array<HubScreenButtonInfo> buttons;
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsApply), [this]() { apply_general_options(); }).activate_on_trigger_hold());
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsCancel), [this]() { generalOptions = usedGeneralOptions; apply_general_options(); }));

			scr->add_button_widgets(buttons, Range2(scr->mainResolutionInPixels.x.expanded_by(-max(fs.x, fs.y) * 6.0f), Range(scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 1.0f, scr->mainResolutionInPixels.y.min + max(fs.x, fs.y) * 2.5f)), max(fs.x, fs.y) * 2.0f);

			scr->compress_vertically();
		}
	} break;
	default:
		go_back();
	}
}

void Options::haptics_debug_test_all()
{
	struct DebugTestAll
	{
		RefCountObjectPtr<HubScreen> waitScreen;
		RefCountObjectPtr<HubWidget> waitScreenText;

		DebugTestAll() {}
		DebugTestAll(HubScreen* _waitScreen, HubWidget* _waitScreenText) : waitScreen(_waitScreen), waitScreenText(_waitScreenText) {}

		void show(tchar const* _text)
		{
			if (auto* t = fast_cast<HubWidgets::Text>(waitScreenText.get()))
			{
				t->set(String(_text));
			}
		}

		static void test_all(void* _data)
		{
			DebugTestAll* dtaData = (DebugTestAll*)_data;

			{
				dtaData->show(TXT("right hand fire/recoil"));
				PhysicalSensations::SingleSensation s(PhysicalSensations::SingleSensation::RecoilMedium);
				s.for_hand(Hand::Right);
				PhysicalSensations::start_sensation(s);
				::System::Core::sleep(1.0f);
			}
			{
				dtaData->show(TXT("left hand fire/recoil"));
				PhysicalSensations::SingleSensation s(PhysicalSensations::SingleSensation::RecoilMedium);
				s.for_hand(Hand::Left);
				PhysicalSensations::start_sensation(s);
				::System::Core::sleep(1.0f);
			}
			{
				dtaData->show(TXT("power up"));
				PhysicalSensations::SingleSensation s(PhysicalSensations::SingleSensation::PowerUp);
				PhysicalSensations::start_sensation(s);
				::System::Core::sleep(1.0f);
			}
			{
				dtaData->show(TXT("shot from front"));
				PhysicalSensations::SingleSensation s(PhysicalSensations::SingleSensation::Shot);
				s.with_dir_os(Vector3(0.0f, -1.0f, 0.0f));
				PhysicalSensations::start_sensation(s);
				::System::Core::sleep(1.0f);
			}
			{
				dtaData->show(TXT("shot from front right"));
				PhysicalSensations::SingleSensation s(PhysicalSensations::SingleSensation::Shot);
				s.with_dir_os(Vector3(-1.0f, -1.0f, 0.0f));
				PhysicalSensations::start_sensation(s);
				::System::Core::sleep(1.0f);
			}
			{
				dtaData->show(TXT("shot from back"));
				PhysicalSensations::SingleSensation s(PhysicalSensations::SingleSensation::Shot);
				s.with_dir_os(Vector3(0.0f, 1.0f, 0.0f));
				PhysicalSensations::start_sensation(s);
				::System::Core::sleep(1.0f);
			}
			{
				dtaData->show(TXT("shot from back left"));
				PhysicalSensations::SingleSensation s(PhysicalSensations::SingleSensation::Shot);
				s.with_dir_os(Vector3(1.0f, 1.0f, 0.0f));
				PhysicalSensations::start_sensation(s);
				::System::Core::sleep(1.0f);
			}
			{
				dtaData->show(TXT("electrocuted from front"));
				PhysicalSensations::SingleSensation s(PhysicalSensations::SingleSensation::Electrocution);
				s.with_dir_os(Vector3(0.0f, -1.0f, 0.0f));
				PhysicalSensations::start_sensation(s);
				::System::Core::sleep(1.0f);
			}
			
			if (dtaData->waitScreen.get())
			{
				dtaData->waitScreen->deactivate();
			}

			delete dtaData;
		}
	};

	HubScreen* waitScreen;
	HubWidget* waitScreenText;
	{
		int width = 30;
		int lineCount = 4;
		Vector2 fs = HubScreen::s_fontSizeInPixels;
		Vector2 ppa = HubScreen::s_pixelsPerAngle;
		float margin = max(fs.x, fs.y) * 1.0f;
		Vector2 size = Vector2(((float)(width + 2) * fs.x + margin * 2.0f) / ppa.x,
			(HubScreen::s_fontSizeInPixels.y * (float)(lineCount)+margin * 2.0f) / ppa.y);
		waitScreen = new HubScreen(get_hub(), Name::invalid(), size, read_forward_dir(), get_hub()->get_radius() * 0.5f, NP, NP, ppa);
		waitScreen->be_modal();

		{
			HubWidgets::Text* t = new HubWidgets::Text();
			t->at = waitScreen->mainResolutionInPixels.expanded_by(-Vector2::one * margin);
			t->with_align_pt(Vector2::half);
			waitScreen->add_widget(t);
			waitScreenText = t;
		}
	}
	
	waitScreen->activate();
	get_hub()->add_screen(waitScreen);

	auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>();
	game->get_job_system()->do_asynchronous_off_system_job(DebugTestAll::test_all, new DebugTestAll(waitScreen, waitScreenText));
}

void Options::haptics_disable()
{
	auto& mc = MainConfig::access_global();
	mc.set_physical_sensations_system(Name::invalid());
	PhysicalSensations::terminate();

	on_options_applied();
	show_screen(DevicesOptionsScreen, true);
}

void Options::haptics_accept()
{
	// general acceptance of the options and the setup, as they are (should be running at this point, bhaptics java does so)

#ifdef PHYSICAL_SENSATIONS_BHAPTICS_JAVA
	if (PhysicalSensations::BhapticsJava* bhj = fast_cast<PhysicalSensations::BhapticsJava>(PhysicalSensations::IPhysicalSensations::get()))
	{
		if (auto* bhm = bhj->get_bhaptics())
		{
			bhm->write_setup_to_main_config();
		}
	}
#endif

	on_options_applied();
	show_screen(DevicesOptionsScreen, true);
}

void Options::bhaptics_op(Name const& _op)
{
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_JAVA
	if (PhysicalSensations::BhapticsJava* bhj = fast_cast<PhysicalSensations::BhapticsJava>(PhysicalSensations::IPhysicalSensations::get()))
	{
		if (auto* bhm = bhj->get_bhaptics())
		{
			String address;
			if (auto* sd = get_hub()->get_selected_draggable())
			{
				if (auto* bhdi = fast_cast<BhapticsDeviceInfo>(sd->data.get()))
				{
					address = bhdi->address;
				}
			}
			if (_op == NAME(test) &&
				!address.is_empty())
			{
				bhm->ping(address);
			}
			if (_op == NAME(toggle) &&
				!address.is_empty())
			{
				bhm->toggle_position(address);
			}
		}
	}
#endif
}

void Options::bhaptics_auto_scan()
{
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_JAVA
	if (PhysicalSensations::BhapticsJava* bh = fast_cast<PhysicalSensations::BhapticsJava>(PhysicalSensations::IPhysicalSensations::get()))
#endif
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_WINDOWS
	if (PhysicalSensations::BhapticsWindows* bh = fast_cast<PhysicalSensations::BhapticsWindows>(PhysicalSensations::IPhysicalSensations::get()))
#endif
#ifdef PHYSICAL_SENSATIONS_BHAPTICS
	{
		if (devicesBhapticsOptions.getDevicesTS.get_time_since() > 1.0f)
		{
			devicesBhapticsOptions.getDevicesTS.reset();
			if (auto* bhm = bh->get_bhaptics())
			{
				bhm->update_pending_devices();
			}
		}
	}
#endif
}

void Options::owo_test_devices()
{
	devicesOWOOptions.apply(true);
	devicesOWOOptions.testPending = true;
	devicesOWOOptions.exitOnConnected = false;
}

void Options::apply_devices_controllers_options()
{
	devicesControllersOptions.apply();
	on_options_applied();
	show_screen(DevicesOptionsScreen, true);
}

void Options::apply_devices_owo_options(bool _initialise)
{
	devicesOWOOptions.apply(_initialise);
	if (_initialise)
	{
		// we will be waiting for a valid connection first
		devicesOWOOptions.testPending = true;
		devicesOWOOptions.exitOnConnected = true;
	}
	else
	{
		on_options_applied();
		show_screen(DevicesOptionsScreen, true);
	}
}

void Options::apply_sound_options()
{
	soundOptions.apply();
	on_options_applied();
	show_screen(MainScreen, true);
}

void Options::apply_sound_volumes()
{
	soundOptions.apply_volumes();
	// do not save, just volumes
}

void Options::apply_aesthetics_options()
{
	HubScreens::Question::ask(get_hub(), NAME(lsAestheticsQuestion),
		[this]()
		{
			aestheticsOptions.apply();
			on_options_applied(); // to save config file, as during aestheticsOptions.apply() we update changed system tags, we might not be saving some of the options here that should be reset to defaults
			::System::Core::restartRequired = true;
			get_hub()->quit_game();
		},
		nullptr
		);
}

void Options::apply_video_options(bool _remainWhereWeAre)
{
	if (videoOptionsAsInitialised == videoOptions)
	{
		if (!_remainWhereWeAre)
		{
			show_screen(MainScreen, true);
		}
	}
	else
	{
		an_assert(TeaForGodEmperor::Game::get());
		TeaForGodEmperor::Game::get()->supress_saving_config_file(true);
		videoOptions.apply();

		bool softReinitialise = true
			&& videoOptions.soft_reinitialise_is_ok(videoOptionsAsInitialised)
			;
		
		bool newRenderTargetsNeeded = ! softReinitialise
			|| videoOptions.are_new_render_targets_required(videoOptionsAsInitialised)
			;

		TeaForGodEmperor::Game::get()->reinitialise_video(softReinitialise, newRenderTargetsNeeded);

		videoOptionsAsInitialised = videoOptions;

		if (!_remainWhereWeAre)
		{
#ifndef AN_ANDROID
			HubScreens::Question::ask(get_hub(), NAME(lsVideoSettingsChanged),
			[this]()
			{
#endif
				TeaForGodEmperor::Game::get()->supress_saving_config_file(false);
				// save config
				on_options_applied();
				show_screen(MainScreen, true);
#ifndef AN_ANDROID
			},
			[this, softReinitialise, newRenderTargetsNeeded]()
			{
				// restore former
				usedVideoOptions.apply();
				TeaForGodEmperor::Game::get()->reinitialise_video(softReinitialise, newRenderTargetsNeeded);
				TeaForGodEmperor::Game::get()->supress_saving_config_file(false);

				show_screen(VideoOptionsScreen);
			},
			15
			);
#endif
		}
	}
}

void Options::update_resolutions()
{
	resolutionsForDisplay.clear();
	::System::Video::enumerate_resolutions(videoOptions.displayIndex, OUT_ resolutionsForDisplay);
	videoOptions.update_resolution_index(resolutionsForDisplay);
}

void Options::apply_gameplay_options()
{
	gameplayOptions.apply();
	on_options_applied();
	show_screen(MainScreen, true);
}

void Options::apply_comfort_options()
{
	comfortOptions.apply();
	TeaForGodEmperor::Game::get()->reinitialise_video(true, false);
	on_options_applied();
	show_screen(MainScreen, true);
}

void Options::apply_controls_options()
{
	controlsOptions.apply();
	update_and_apply_vr_changes(); 
	on_options_applied();
	show_screen(MainScreen, true);
}

void Options::apply_controls_vr_offsets_options(bool _test_remainWhereWeAre)
{
	if (_test_remainWhereWeAre)
	{
		controlsOptions.apply_vr_offsets(false);
#ifdef BUILD_NON_RELEASE
		devVROffsets.test();
#endif
		testVROffsetsTimeLeft = 5.0f;
	}
	else
	{
		controlsOptions.apply_vr_offsets(true); // used confirmed ones
#ifdef BUILD_NON_RELEASE
		devVROffsets.restore();
#endif
		if (auto* scr = get_hub()->get_screen(NAME(optionsControlsVROffsets)))
		{
			scr->deactivate();
		}
		show_screen(ControlsOptionsScreen, true);
	}
}

void Options::update_and_apply_vr_changes()
{
	float prevRotation = MainConfig::access_global().get_vr_map_space_rotate();
	if (screen == PlayAreaOptionsScreen)
	{
		playAreaOptions.apply(type == Main);
	}
	else if (screen == PlayAreaCalibrateScreen)
	{
		playAreaOptions.apply_vr_map(type == Main);
	}
	else if (screen == GeneralOptionsScreen)
	{
		generalOptions.apply(type == Main);
	}
	// to reflect play area size changes
	get_hub()->update_scene_mesh();
	// do not setup RGI options, they should be updated only at the specific time, beginning of a pilgrimage
	if (screen == PlayAreaOptionsScreen ||
		screen == PlayAreaCalibrateScreen)
	{
		update_preview_play_area();
	}
	// remove this bit?
	// -start
	float currRotation = MainConfig::access_global().get_vr_map_space_rotate();
	get_hub()->rotate_everything_by((prevRotation - currRotation));
	get_hub()->force_reset_and_update_forward();
	// -end

	// auto update basing on MainConfig
	if (type == Main)
	{
		TeaForGodEmperor::Game::update_use_sliding_locomotion();
	}
}

void Options::apply_play_area_options()
{
	{
		Array<Name> screensToKeep;
		screensToKeep.push_back(NAME(hubBackground));
		get_hub()->keep_only_screens(screensToKeep);
		get_hub()->remove_deactivated_screens();
	}

	an_assert(screen == PlayAreaOptionsScreen);
	update_and_apply_vr_changes();
	on_options_applied();

	delayed_show_screen(0.05f, MainScreen, true, true);
}

void Options::apply_general_options()
{
	// do:
	//	generalOptions.apply(type == Main);
	// implicitly via update_and_apply_vr_changes
	an_assert(screen == GeneralOptionsScreen);
	update_and_apply_vr_changes();
	on_options_applied();
	show_screen(MainScreen, true);
}

void Options::apply_language()
{
	generalOptions.apply_language_subtitles_voiceovers();
}

void Options::reset_turn_counter()
{
	if (auto* vr = VR::IVR::get())
	{
		vr->reset_turn_count();
	}
}

void Options::reset_gameplay_options()
{
	auto storedGameplayOptions = gameplayOptions;
	TeaForGodEmperor::PlayerPreferences psp;
	gameplayOptions.read(&psp);
	// some of the options should not reset
	gameplayOptions.turnCounter = storedGameplayOptions.turnCounter;
	gameplayOptions.showSavingIcon = storedGameplayOptions.showSavingIcon;
	if (type != Main)
	{
		gameplayOptions.doorDirections = storedGameplayOptions.doorDirections;
	}
	show_screen(GameplayOptionsScreen, true);
}

void Options::reset_comfort_options()
{
	TeaForGodEmperor::PlayerPreferences psp;
	comfortOptions.read(&psp);
	comfortOptions.allowCrouch = Option::Auto;
	comfortOptions.flickeringLights = Option::Allow;
	comfortOptions.cameraRumble = Option::Allow;
	comfortOptions.rotatingElevators = Option::Allow;
	show_screen(ComfortOptionsScreen, true);
}

void Options::reset_play_area_options()
{
	todo_note(TXT("default play area options are hardcoded!"));
	playAreaOptions.border = 0;
	playAreaOptions.preferredTileSize = 0; // auto
	playAreaOptions.doorHeight = (int)(Framework::Door::get_default_nominal_door_height() * 100.0f);
	playAreaOptions.scaleUpPlayer = true;
	playAreaOptions.allowCrouch = Option::Auto;
	show_screen(PlayAreaOptionsScreen, true);
}

void Options::reset_video_options()
{
	VideoOptions tempCopy = videoOptions;
	videoOptions.read(&MainConfig::defaults().get_video());

	// restore resolution and window options
	videoOptions.displayIndex = tempCopy.displayIndex;
	videoOptions.fullScreen = tempCopy.fullScreen;
	videoOptions.resolution = tempCopy.resolution;
	videoOptions.resolutionIndex = tempCopy.resolutionIndex;
	videoOptions.displayOnScreen = tempCopy.displayOnScreen;

	show_screen(VideoOptionsScreen, true);
}

void Options::on_options_applied()
{
	if (auto* game = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
	{
		game->supress_saving_config_file(false);
		game->save_config_file();
		game->add_async_save_player_profile(false);
	}
	usedSoundOptions.read();
	soundOptions = usedSoundOptions;
	usedVideoOptions.read();
	videoOptions = usedVideoOptions;
	usedGameplayOptions.read();
	gameplayOptions = usedGameplayOptions;
	usedComfortOptions.read();
	comfortOptions = usedComfortOptions;
	usedControlsOptions.read();
	controlsOptions = usedControlsOptions;
	usedPlayAreaOptions.read();
	playAreaOptions = usedPlayAreaOptions;
	update_preview_play_area();
	TeaForGodEmperor::PlayerSetup::get_current().get_preferences().apply();
}

Vector2 playAreaWhenNoVR = Vector2(4.0f, 3.0f);

void Options::on_post_render(Framework::Render::Scene* _scene, Framework::Render::Context & _context)
{
#ifdef USING_PHYSICAL_SENSATIONS_BHAPTICS
	if (screen == DevicesBhapticsOptionsScreen)
	{
		if (devicesBhapticsOptions.autoScanning)
		{
			bhaptics_auto_scan();
		}

		RefCountObjectPtr<PhysicalSensations::IPhysicalSensations> keepPS;
		keepPS = PhysicalSensations::IPhysicalSensations::get_as_is();
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_JAVA
		auto* bh = fast_cast<PhysicalSensations::BhapticsJava>(keepPS.get());
#endif
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_WINDOWS
		auto* bh = fast_cast<PhysicalSensations::BhapticsWindows>(keepPS.get());
#endif
		if (bh)
		{
			if (bh->is_ok())
			{
				if (auto* bhm = bh->get_bhaptics())
				{
					if (auto* scr = get_hub()->get_screen(NAME(optionsDevicesBhaptics)))
					{
						if (auto* l = fast_cast<HubWidgets::List>(scr->get_widget(NAME(idDevicesList))))
						{
							if (bhm->update_devices_with_pending_devices() ||
								l->elements.is_empty() /* we will always update if empty */)
							{
								Array<
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_JAVA
									bhapticsJava::BhapticsDevice
#endif
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_WINDOWS
									bhapticsWindows::BhapticsDevice
#endif
								> devices;
								{
									Concurrency::ScopedSpinLock lock(bhm->access_devices_lock());
									devices = bhm->get_devices();
								}
								String selectByIDInfo;
								{
									if (auto* sd = l->hub->get_selected_draggable())
									{
										if (auto* bhdi = fast_cast<BhapticsDeviceInfo>(sd->data.get()))
										{
											selectByIDInfo = bhdi->idInfo;
										}
									}
								}
								HubDraggable* selectDraggable = nullptr;
								l->clear();
								for_every(d, devices)
								{
									auto* bhdi = new BhapticsDeviceInfo();
									bool isValid = !d->deviceName.is_empty();
									bhdi->name = d->deviceName;
									bhdi->position = isValid ? an_bhaptics::PositionType::to_char(d->position) : TXT("");
									bhdi->shortInfo = isValid ? String::printf(TXT("%S"), bhdi->position.to_char()) : String::empty();
									bhdi->idInfo = bhdi->shortInfo;
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_JAVA
									if (isValid && !d->isConnected)
									{
										bhdi->shortInfo += String::printf(TXT(" [%S]"), LOC_STR(NAME(lsDevicesNotConnected)).to_char());
									}
									bhdi->address = isValid ? d->address : String::empty();
									bhdi->isPaired = isValid ? d->isPaired : false;
									bhdi->isConnected = isValid ? d->isConnected : false;
									bhdi->battery = isValid ? d->battery : 0;
									bhdi->idInfo = d->address;
#endif
									auto* draggable = l->add(bhdi);
									if (!selectDraggable)
									{
										selectDraggable = draggable; // choose the first one automatically
									}
									if (selectByIDInfo == bhdi->idInfo)
									{
										selectDraggable = draggable;
									}
								}
								if (selectDraggable)
								{
									l->hub->select(l, selectDraggable);
								}
							}

							if (auto* sd = l->hub->get_selected_draggable())
							{
								auto* bhdi = fast_cast<BhapticsDeviceInfo>(sd->data.get());
								if (auto* w = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(lsDevicesBhapticsName))))
								{
									w->set(bhdi ? bhdi->name : String::empty());
								}
								if (auto* w = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(lsDevicesBhapticsPosition))))
								{
									w->set(bhdi ? bhdi->position : String::empty());
								}
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_JAVA
								if (auto* w = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(lsDevicesBhapticsAddress))))
								{
									w->set(bhdi ? bhdi->address : String::empty());
								}
#endif
								if (auto* w = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(lsDevicesBhapticsBattery))))
								{
									if (bhdi && !bhdi->name.is_empty())
									{
										w->set_colour(NP);
										if (bhdi->battery < 0)
										{
											w->set(String::empty());
										}
										else
										{
											w->set(String::printf(TXT("%i%%"), bhdi->battery));
											if (bhdi->battery < VERY_LOW_BATTERY)
											{
												w->set_colour(VERY_LOW_BATTERY_COLOUR);
											}
											else if (bhdi->battery < LOW_BATTERY)
											{
												w->set_colour(LOW_BATTERY_COLOUR);
											}
										}
									}
									else
									{
										w->set(String::empty());
										w->set_colour(NP);
									}
								}
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_JAVA
								if (auto* w = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(lsDevicesBhapticsConnection))))
								{
									if (bhdi && !bhdi->name.is_empty())
									{
										if (bhdi->isConnected)
										{
											w->set(NAME(lsDevicesBhapticsConnected));
											w->set_colour(HubColours::selected_highlighted());
										}
										else if (bhdi->isPaired)
										{
											w->set(NAME(lsDevicesBhapticsPaired));
											w->set_colour(Colour::lerp(0.5f, HubColours::screen_interior(), HubColours::text()));
										}
										else
										{
											w->set(NAME(lsDevicesBhapticsDisconnected));
											w->set_colour(Colour::lerp(0.5f, HubColours::screen_interior(), HubColours::text()));
										}
									}
									else
									{
										w->set(String::empty());
										w->set_colour(NP);
									}
								}
#endif
							}
						}
					}
				}
			}
			else
			{
#ifdef PHYSICAL_SENSATIONS_BHAPTICS_WINDOWS
				if (auto* bhm = bh->get_bhaptics())
				{
					if (bhm->done_initialisation())
					{
						if (!devicesBhapticsOptions.showedNoPlayer)
						{
							devicesBhapticsOptions.showedNoPlayer = true;
							HubScreens::Message::show(get_hub(), NAME(lsDevicesBhapticsNoPlayerDetected), [this]()
								{
									show_screen(DevicesOptionsScreen, true);
								});
						}
					}
				}
#endif
			}
		}
	}
#endif
#ifdef PHYSICAL_SENSATIONS_OWO
	if (screen == DevicesOWOOptionsScreen)
	{
		devicesOWOOptions.status = DevicesOWOOptions::NotConnected;
		RefCountObjectPtr<PhysicalSensations::IPhysicalSensations> keepPS;
		keepPS = PhysicalSensations::IPhysicalSensations::get_as_is();
		if (auto* owo = fast_cast<PhysicalSensations::OWO>(keepPS.get()))
		{
			if (owo->is_ok())
			{
				devicesOWOOptions.status = DevicesOWOOptions::Connected;
				if (devicesOWOOptions.testPending)
				{
					// play recoil on both hands
					for_count(int, iHandIdx, Hand::MAX)
					{
						PhysicalSensations::SingleSensation s(PhysicalSensations::SingleSensation::RecoilMedium);
						s.for_hand(Hand::Type(iHandIdx));
						PhysicalSensations::start_sensation(s);
					}
					devicesOWOOptions.testPending = false;
				}
				if (devicesOWOOptions.exitOnConnected)
				{
					devicesOWOOptions.exitOnConnected = false;
					on_options_applied();
					show_screen(DevicesOptionsScreen, true);
				}
			}
			else if (owo->is_connecting())
			{
				devicesOWOOptions.status = DevicesOWOOptions::Connecting;
			}
		}
		if (!devicesOWOOptions.visibleStatus.is_set() ||
			devicesOWOOptions.visibleStatus.get() != devicesOWOOptions.status)
		{
			if (auto* scr = get_hub()->get_screen(NAME(optionsDevicesOWO)))
			{
				if (auto* w = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(idDevicesStatus))))
				{
					devicesOWOOptions.visibleStatus = devicesOWOOptions.status;
					Name locStrId = NAME(lsDevicesNotConnected);
					if (devicesOWOOptions.status == DevicesOWOOptions::Connecting) locStrId = NAME(lsDevicesConnecting);
					if (devicesOWOOptions.status == DevicesOWOOptions::Connected) locStrId = NAME(lsDevicesConnected);
					w->set(locStrId);
				}
				if (auto* w = fast_cast<HubWidgets::Button>(scr->get_widget(NAME(lsDevicesTest))))
				{
					w->enable(devicesOWOOptions.status != DevicesOWOOptions::Connecting);
				}
				if (auto* w = fast_cast<HubWidgets::Button>(scr->get_widget(NAME(lsApply))))
				{
					w->enable(devicesOWOOptions.status != DevicesOWOOptions::Connecting);
				}
				if (auto* w = fast_cast<HubWidgets::Button>(scr->get_widget(NAME(lsCancel))))
				{
					w->enable(devicesOWOOptions.status != DevicesOWOOptions::Connecting);
				}
			}
		}
	}
#endif
	if (screen == PlayAreaOptionsScreen ||
		screen == PlayAreaCalibrateScreen)
	{
		float horizontalScaling = (float)playAreaOptions.horizontalScaling / 100.0f;
		float invHorizontalScaling = horizontalScaling != 0.0f ? 1.0f / horizontalScaling : 1.0f;

		if (screen == PlayAreaCalibrateScreen)
		{
			if (auto* vr = VR::IVR::get())
			{
				//Vector3 manualOffset = Vector3(playAreaOptions.manualOffsetX, playAreaOptions.manualOffsetY, 0.0f);
				//float manualRotate = (float)playAreaOptions.manualRotate;
				//Transform mapVRSpace = vr->calculate_map_vr_space(manualOffset, manualRotate);
				Transform rotateVRSpace;
				{
					float controlsViewYaw = vr->get_controls_view().get_orientation().to_rotator().yaw;
					float mapVRSpaceYaw = vr->get_map_vr_space().get_orientation().to_rotator().yaw;
					rotateVRSpace = Rotator3(0.0f, controlsViewYaw + mapVRSpaceYaw, 0.0f).to_quat().to_matrix().to_transform();
				}

				auto currentCalibrateOption = calibrationOption;
				if (auto* hub = get_hub())
				{
					bool switchNow = false;
					for_count(int, hIdx, Hand::MAX)
					{
						auto& h = hub->get_hand((Hand::Type)hIdx);
						switchNow |= h.input.is_button_pressed(NAME(calibrateSwitch));
					}
					if (switchNow)
					{
						currentCalibrateOption = (CalibrationOption)((currentCalibrateOption + 1) % CalibrationOption::Count);
					}
				}

				if (!visibleCalibrationOption.is_set() ||
					visibleCalibrationOption.get() != currentCalibrateOption)
				{
					if (auto* scr = get_hub()->get_screen(NAME(optionsPlayAreaCalibrate)))
					{
						if (auto* t = fast_cast<HubWidgets::Button>(scr->get_widget(NAME(idCalibrationOption))))
						{
							visibleCalibrationOption = currentCalibrateOption;
							if (currentCalibrateOption == CalibrationOption::Placement)
							{
								t->set(NAME(lsPlayAreaCalibratePlacement));
							}
							if (currentCalibrateOption == CalibrationOption::Size)
							{
								t->set(NAME(lsPlayAreaCalibrateSize));
							}
						}
					}
				}

				bool changedAnything = false;

				if (currentCalibrateOption == CalibrationOption::Placement)
				{
					if (auto* hub = get_hub())
					{
						{
							auto& h = hub->get_hand(Hand::Left);
							Vector2 changeBy = Vector2::zero;
							changeBy += h.input.get_stick(NAME(calibrate)) * ::System::Core::get_raw_delta_time();
							changeBy += h.input.get_mouse(NAME(calibrate));
							changeBy = rotateVRSpace.vector_to_world(changeBy.to_vector3()).to_vector2();
							playAreaOptions.manualOffsetX += changeBy.x * 0.25f;
							playAreaOptions.manualOffsetY += changeBy.y * 0.25f;
							changedAnything |= !changeBy.is_zero();
						}
						{
							auto& h = hub->get_hand(Hand::Right);
							float changeBy = 0.0f;
							changeBy += h.input.get_stick(NAME(calibrate)).x * ::System::Core::get_raw_delta_time();
							changeBy += h.input.get_mouse(NAME(calibrate)).x;
							playAreaOptions.manualRotate += changeBy * 15.0f;
							changedAnything |= changeBy != 0.0f;
						}
					}
				}
				if (type == Main &&
					currentCalibrateOption == CalibrationOption::Size)
				{
					Vector2 changeBy = Vector2::zero;
					if (auto* hub = get_hub())
					{
						for_count(int, hIdx, Hand::MAX)
						{
							auto& h = hub->get_hand((Hand::Type)hIdx);
							changeBy += h.input.get_stick(NAME(calibrate)) * ::System::Core::get_raw_delta_time();
							changeBy += h.input.get_mouse(NAME(calibrate));
						}
					}
					if (!changeBy.is_zero())
					{
						changedAnything |= true;
						if (playAreaOptions.manualSizeX == 0.0f || playAreaOptions.manualSizeY == 0.0f)
						{
							if (auto* vr = VR::IVR::get())
							{
								playAreaOptions.manualSizeX = vr->get_raw_whole_play_area_rect_half_right().length() * 2.0f;
								playAreaOptions.manualSizeY = vr->get_raw_whole_play_area_rect_half_forward().length() * 2.0f;
							}
						}

						playAreaOptions.manualSizeX += changeBy.x * 0.25f;
						playAreaOptions.manualSizeY += changeBy.y * 0.25f;
					}
				}
				if (playAreaOptions.manualSizeX != 0.0f && playAreaOptions.manualSizeY != 0.0f)
				{
					playAreaOptions.manualSizeX = max(0.90f, playAreaOptions.manualSizeX);
					playAreaOptions.manualSizeY = max(0.60f, playAreaOptions.manualSizeY);
				}

				if (changedAnything)
				{
					update_preview_play_area();
				}

				Vector2 realSize(playAreaOptions.manualSizeX, playAreaOptions.manualSizeY);
				if (realSize.x == 0.0f || realSize.y == 0.0f)
				{
					realSize.x = vr->get_raw_whole_play_area_rect_half_right().length() * 2.0f;
					realSize.y = vr->get_raw_whole_play_area_rect_half_forward().length() * 2.0f;
				}
				Vector2 apparentSize = realSize * horizontalScaling;
				if (!visibleCalibrationSize.is_set() ||
					visibleCalibrationSize.get() != apparentSize)
				{
					visibleCalibrationSize = apparentSize;
					if (auto* scr = get_hub()->get_screen(NAME(optionsPlayAreaCalibrate)))
					{
						if (auto* w = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(idCalibrationSize))))
						{
							MeasurementSystem::Type ms = playAreaOptions.get_measurement_system();// TeaForGodEmperor::PlayerSetup::get_current().get_preferences().get_measurement_system();
							if (ms == MeasurementSystem::Imperial)
							{
								w->set(String::printf(TXT("%S x %S~(%S x %S)"),
									MeasurementSystem::as_feet_inches(apparentSize.x * invHorizontalScaling).to_char(),
									MeasurementSystem::as_feet_inches(apparentSize.y * invHorizontalScaling).to_char(),
									MeasurementSystem::as_feet_inches(apparentSize.x).to_char(),
									MeasurementSystem::as_feet_inches(apparentSize.y).to_char()
								));
							}
							else
							{
								w->set(String::printf(TXT("%S x %S~(%S x %S)"),
									MeasurementSystem::as_meters(apparentSize.x * invHorizontalScaling).to_char(),
									MeasurementSystem::as_meters(apparentSize.y * invHorizontalScaling).to_char(),
									MeasurementSystem::as_meters(apparentSize.x).to_char(),
									MeasurementSystem::as_meters(apparentSize.y).to_char()
								));
							}
						}
					}
				}
			}
		}

		Vector3 manualOffset = Vector3(playAreaOptions.manualOffsetX, playAreaOptions.manualOffsetY, 0.0f);
		float manualRotate = playAreaOptions.manualRotate;
		float borderAsFloat = (float)playAreaOptions.border / 100.0f;
		float doorHeight = (float)playAreaOptions.doorHeight / 100.0f;
		bool scaleUp = playAreaOptions.scaleUpPlayer != 0;
		float generalScaling = TeaForGodEmperor::PlayerPreferences::calculate_vr_scaling(doorHeight, scaleUp);
		float actualGeneralScaling = 1.0f;
		if (auto* vr = VR::IVR::get())
		{
			actualGeneralScaling = vr->get_scaling().general;
		}
		float invActualGeneralScaling = actualGeneralScaling != 0.0f ? 1.0f / actualGeneralScaling : 1.0f;
		float applyGeneralScaling = generalScaling * invActualGeneralScaling;
		float invApplyGeneralScaling = applyGeneralScaling != 0.0f ? 1.0f / applyGeneralScaling : 1.0f;

		::System::Video3D* v3d = ::System::Video3D::get();
		v3d->setup_for_2d_display();
		v3d->depth_test(::System::Video3DCompareFunc::None);
		v3d->depth_mask(false);
		
		auto& viewMatrixStack = v3d->access_model_view_matrix_stack();

		Transform offsetDueToScaling = Transform::identity;

		Transform p = Transform::identity;
		Transform op = Transform::identity;
		if (auto* vr = VR::IVR::get())
		{
			{
				// let's calculate true 0 and render (using actual vr scaling) offset, to compensate horizontal scaling, so the grid is wobbly only for horizontal scaling
				Vector3 whereWeAreRelativeTo0 = vr->get_render_view().get_translation() * Vector3::xy;
				Vector3 where0ShouldBeRelativeToUs = -vr->get_render_view().get_translation() * Vector3::xy * (horizontalScaling / vr->get_scaling().horizontal);
				offsetDueToScaling.set_translation(whereWeAreRelativeTo0 + where0ShouldBeRelativeToUs);
			}

			op = vr->calculate_map_vr_space(Vector3::zero, 0.0f);
			p = vr->calculate_map_vr_space(manualOffset, manualRotate);
			// we want to show it relative to where we are
			op = vr->get_map_vr_space().to_local(op);
			p = vr->get_map_vr_space().to_local(p);
		}

		viewMatrixStack.push_to_world(offsetDueToScaling.to_matrix());

		struct Points
		{
			Array<Vector3> points;

			void clear() { points.clear(); }
			void add(Vector3 const& _p) { points.push_back(_p); }
			void loop() { points.push_back(points.get_first()); }
			void render(Colour const& _colour)
			{
				::System::Video3DPrimitives::line_strip(_colour, points.get_data(), points.get_size(), false);
			}
			void add_play_area(Transform p, Vector2 half, Optional<float> _tip = NP, float z = 0.0f)
			{
				float tip = _tip.get(0.1f);
				add(p.location_to_world(Vector3(-half.x, -half.y, z)));
				add(p.location_to_world(Vector3(-half.x,  half.y, z)));
				add(p.location_to_world(Vector3(-tip,     half.y, z)));
				add(p.location_to_world(Vector3( 0.0f,    half.y + tip, z)));
				add(p.location_to_world(Vector3( tip,     half.y, z)));
				add(p.location_to_world(Vector3( half.x,  half.y, z)));
				add(p.location_to_world(Vector3( half.x, -half.y, z)));
				loop();
			}
		};


		Vector2 size = Vector2::zero;
		{
			size = playAreaWhenNoVR * 0.5f;
			if (auto* vr = VR::IVR::get())
			{
				size.x = vr->get_raw_whole_play_area_rect_half_right().length() * 2.0f;
				size.y = vr->get_raw_whole_play_area_rect_half_forward().length() * 2.0f;
			}

			if (playAreaOptions.manualSizeX != 0.0f &&
				playAreaOptions.manualSizeY != 0.0f)
			{
				size.x = playAreaOptions.manualSizeX;
				size.y = playAreaOptions.manualSizeY;
			}
		}

		// play area with modifications
		{
			Vector2 half = size * 0.5f;
			half.x -= borderAsFloat;
			half.y -= borderAsFloat;
			half *= actualGeneralScaling;
			Points points;
			points.add_play_area(p, half, NP, -0.0025f);
			points.render(Colour::red);
		}

		// play area with some modifications
		{
			Vector2 half = size * 0.5f;
			half *= actualGeneralScaling;
			Points points;
			points.add_play_area(p, half, NP, - 0.005f);
			points.render(Colour::orange);
		}

		// original, whole play area
		{
			Vector2 half = playAreaWhenNoVR * 0.5f;
			if (auto* vr = VR::IVR::get())
			{
				half.x = vr->get_raw_whole_play_area_rect_half_right().length();
				half.y = vr->get_raw_whole_play_area_rect_half_forward().length();
			}
			half *= actualGeneralScaling;
			Points points;
			points.add_play_area(op, half, NP, -0.0075f);
			points.render(Colour::blue);
		}

		// play area from RGI
		{
			Vector2 half;
			half.x = previewRGI.get_play_area_rect_size().x * 0.5f/* * invHorizontalScaling*/ * invApplyGeneralScaling;
			half.y = previewRGI.get_play_area_rect_size().y * 0.5f/* * invHorizontalScaling*/ * invApplyGeneralScaling;
			Points points;
			points.add_play_area(p, half);
			points.render(Colour::green);
		}

		// tiles
		{
			for_count(int, x, previewRGI.get_tile_count().x)
			{
				for_count(int, y, previewRGI.get_tile_count().y)
				{
					Transform at = Transform::identity;
					at.set_translation(previewRGI.get_first_tile_centre_offset().to_vector3() + Vector3(previewRGI.get_tile_size() * (float)x, previewRGI.get_tile_size() * (float)y, 0.0f));
					at = p.to_world(at);
					Range2 tile = previewRGI.get_tile_zone().to_range2();
					float st = 0.02f;
					Vector3 points[5];
					points[0] = at.location_to_world(Vector3(tile.x.min + st, tile.y.min + st, 0.0f))/* * invHorizontalScaling*/ * invApplyGeneralScaling;
					points[1] = at.location_to_world(Vector3(tile.x.min + st, tile.y.max - st, 0.0f))/* * invHorizontalScaling*/ * invApplyGeneralScaling;
					points[2] = at.location_to_world(Vector3(tile.x.max - st, tile.y.max - st, 0.0f))/* * invHorizontalScaling*/ * invApplyGeneralScaling;
					points[3] = at.location_to_world(Vector3(tile.x.max - st, tile.y.min + st, 0.0f))/* * invHorizontalScaling*/ * invApplyGeneralScaling;
					points[4] = points[0];
					::System::Video3DPrimitives::line_strip(Colour::yellow, points, 5, false);
				}
			}
		}

		// apparent play area after scaling (horizontal only)
		{
			Vector2 half;
			half.x = previewRGI.get_play_area_rect_size().x * 0.5f * invApplyGeneralScaling;
			half.y = previewRGI.get_play_area_rect_size().y * 0.5f * invApplyGeneralScaling;
			Points points;
			points.add_play_area(p, half);
			points.render(Colour::white);
		}

		// draw doors
		{
			// xy relative location, z dir
			Vector3 di[] = { Vector3( 0.0f,  0.5f, 0.0f),
							 Vector3( 0.5f,  0.0f, 1.0f),
							 Vector3( 0.0f, -0.5f, 2.0f),
							 Vector3(-0.5f,  0.0f, 3.0f),
							 Vector3( 0.0f,  0.0f, 0.0f) };
			float doorWidth = previewRGI.get_tile_size();
			for_count(int, i, 4) // no central door
			{
				auto const & d = di[i];
				Vector3 at = Vector3::zero;
				at.x = previewRGI.get_play_area_rect_size().x * d.x * invApplyGeneralScaling;
				at.y = previewRGI.get_play_area_rect_size().y * d.y * invApplyGeneralScaling;
				Rotator3 rot = Rotator3::zero;
				rot.yaw = d.z * 90.0f;

				Transform placement(at, rot.to_quat());

				placement = p.to_world(placement);

				Vector3 points[5];
				points[0] = placement.location_to_world(Vector3(-doorWidth * 0.5f * invApplyGeneralScaling, 0.0f, 0.0f));
				points[1] = placement.location_to_world(Vector3(-doorWidth * 0.5f * invApplyGeneralScaling, 0.0f, doorHeight));
				points[2] = placement.location_to_world(Vector3 (doorWidth * 0.5f * invApplyGeneralScaling, 0.0f, doorHeight));
				points[3] = placement.location_to_world(Vector3( doorWidth * 0.5f * invApplyGeneralScaling, 0.0f, 0.0f));
				points[4] = points[0];

				::System::Video3DPrimitives::line_strip(Colour::green, points, 5, false);
			}
		}

		viewMatrixStack.pop();
	}
	if (screen == ComfortOptionsScreen)
	{
		if (auto* scr = get_hub()->get_screen(NAME(optionsComfort)))
		{
			if (!comfortOptions.forcedEnvironmentMix &&
				comfortOptions.forcedEnvironmentMixPt != usedComfortOptions.forcedEnvironmentMixPt &&
				!comfortOptions.ignoreForcedEnvironmentMixPt)
			{
				// force update controls
				comfortOptions.forcedEnvironmentMix = 0; // set to 0 to update to 1 with a click
				if (auto* w = fast_cast<HubWidgets::Button>(scr->get_widget(NAME(idForcedEnvironmentMix))))
				{
					if (w->on_click)
					{
						w->on_click(w, w->at.centre(), 0);
					}
				}
			}
		}
	}
}

void Options::on_post_render_render_scene(int _eyeIdx)
{
	base::on_post_render_render_scene(_eyeIdx);
	{
		bool shouldRenderFoveatedRenderingTest = requestFoveatedRenderingTestTS.get_time_since() < 0.1f;
		if (renderFoveatedRenderingTest != shouldRenderFoveatedRenderingTest)
		{
			VideoOptions storedVideoOptions = videoOptions;
			if (shouldRenderFoveatedRenderingTest)
			{
				preFoveatedRenderingTestOptions = videoOptionsAsInitialised;
			}
			else
			{
				videoOptions = preFoveatedRenderingTestOptions;
			}
			apply_video_options(true);
			if (!shouldRenderFoveatedRenderingTest)
			{
				videoOptions = storedVideoOptions;
			}
			renderFoveatedRenderingTest = shouldRenderFoveatedRenderingTest;
		}
		bool shouldRenderFoveatedRenderingTestNow = shouldRenderFoveatedRenderingTest;
#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS
		if (screen == SetupFoveationParamsScreen)
		{
			if (auto* vr = VR::IVR::get())
			{
				VR::Input::Button::WithSource anyGripButton;
				anyGripButton.type = VR::Input::Button::Grip;
				if (vr->get_controls().is_button_pressed(anyGripButton))
				{
					shouldRenderFoveatedRenderingTestNow = true;
				}
			}
		}
#endif

		if (shouldRenderFoveatedRenderingTestNow)
		{
			::System::FoveatedRenderingSetup::render_test_grid(_eyeIdx);
			/*
			{
				static int stored = 0;
				if (stored < 8)
				{
					++stored;
					if (auto* vr = VR::IVR::get())
					{
						if (auto* rt = vr->get_render_render_target((VR::Eye::Type)_eyeIdx))
						{
							TeaForGodEmperor::ScreenShotContext::store_screen_shot(rt);
						}
					}
				}
			}
			*/
		}
	}
}

void Options::update_preview_play_area()
{
	float borderAsFloat = (float)playAreaOptions.border / 100.0f;
	float horizontalScaling = (float)playAreaOptions.horizontalScaling / 100.0f;
	float invHorizontalScaling = horizontalScaling != 0.0f ? 1.0f / horizontalScaling : 1.0f;
	float doorHeight = (float)playAreaOptions.doorHeight / 100.0f;
	bool scaleUp = playAreaOptions.scaleUpPlayer != 0;
	float rgiDoorHeight;
	float generalScaling = TeaForGodEmperor::PlayerPreferences::calculate_vr_scaling(doorHeight, scaleUp, &rgiDoorHeight);
	float invGeneralScaling = generalScaling != 0.0f ? 1.0f / generalScaling : 1.0f;
	Vector2 playAreaSize = playAreaWhenNoVR - Vector2(borderAsFloat, borderAsFloat) * 2.0f;
	if (should_appear_as_immobile_vr(playAreaOptions.immobileVR))
	{
		playAreaSize.x = (float)playAreaOptions.immobileVRSize / 100.0f;
		playAreaSize.y = playAreaSize.x;

		playAreaSize.x = playAreaSize.x * generalScaling;
		playAreaSize.y = playAreaSize.y * generalScaling;
	}
	else
	{
		if (auto* vr = VR::IVR::get())
		{
			if (playAreaOptions.manualSizeX != 0 && playAreaOptions.manualSizeY != 0)
			{
				playAreaSize.x = playAreaOptions.manualSizeX;
				playAreaSize.y = playAreaOptions.manualSizeY;
			}
			else
			{
				playAreaSize.x = vr->get_raw_whole_play_area_rect_half_right().length() * 2.0f;
				playAreaSize.y = vr->get_raw_whole_play_area_rect_half_forward().length() * 2.0f;
			}

			playAreaSize.x = (playAreaSize.x - borderAsFloat * 2.0f) * horizontalScaling * generalScaling;
			playAreaSize.y = (playAreaSize.y - borderAsFloat * 2.0f) * horizontalScaling * generalScaling;
		}
		else
		{
			generalScaling = 1.0f;
			invGeneralScaling = 1.0f;
		}
	}
	previewRGI = TeaForGodEmperor::RoomGenerationInfo::get();
	{
		auto & mc = MainConfig::access_global();

		previewRGI.set_door_height(rgiDoorHeight);
		previewRGI.set_test_play_area_rect_size(playAreaSize);
		previewRGI.set_test_vr_scaling(generalScaling);
		previewRGI.set_preferred_tile_size(playAreaOptions.get_preferred_tile_size());

		// replace horizontal scaling with the one in options
		float prevHorizontalScaling = mc.get_vr_horizontal_scaling();
		mc.set_vr_horizontal_scaling(horizontalScaling); // use the one we have set

		previewRGI.setup_for_preview();

		// set back (so the menu appears in the same place etc)
		mc.set_vr_horizontal_scaling(prevHorizontalScaling);
	}

	{
		int currentImmobileVR = 0;
		HubWidgets::Text* w = nullptr;
		if (auto* scr = get_hub()->get_screen(NAME(optionsPlayArea)))
		{
			currentImmobileVR = playAreaOptions.immobileVR;
			w = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(idShowImmobileVR)));
		}
		if (auto* scr = get_hub()->get_screen(NAME(optionsGeneral)))
		{
			currentImmobileVR = generalOptions.immobileVR;
			w = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(idShowImmobileVR)));
		}
		if (auto* scr = get_hub()->get_screen(NAME(optionsControls)))
		{
			currentImmobileVR = controlsOptions.immobileVR;
			w = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(idShowImmobileVR)));
		}
		if (w)
		{
			if (currentImmobileVR == Option::True)	w->set(LOC_STR(NAME(lsPlayAreaImmobileVRTrue))); else
			if (currentImmobileVR == Option::False)	w->set(LOC_STR(NAME(lsPlayAreaImmobileVRFalse))); else
													w->set(LOC_STR(NAME(lsPlayAreaImmobileVRAuto)));
		}
	}

	if (auto* scr = get_hub()->get_screen(NAME(optionsPlayArea)))
	{
		MeasurementSystem::Type ms = playAreaOptions.get_measurement_system();// TeaForGodEmperor::PlayerSetup::get_current().get_preferences().get_measurement_system();
		if (auto * w = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(idCurrentSize))))
		{
			if (ms == MeasurementSystem::Imperial)
			{
				w->set(String::printf(TXT("%S x %S~(%S x %S)"),
														MeasurementSystem::as_feet_inches(previewRGI.get_play_area_rect_size().x * invHorizontalScaling * invGeneralScaling).to_char(),
														MeasurementSystem::as_feet_inches(previewRGI.get_play_area_rect_size().y * invHorizontalScaling * invGeneralScaling).to_char(),
														MeasurementSystem::as_feet_inches(previewRGI.get_play_area_rect_size().x * invGeneralScaling).to_char(),
														MeasurementSystem::as_feet_inches(previewRGI.get_play_area_rect_size().y * invGeneralScaling).to_char()
														));
			}
			else
			{
				w->set(String::printf(TXT("%S x %S~(%S x %S)"),
														MeasurementSystem::as_meters(previewRGI.get_play_area_rect_size().x * invHorizontalScaling * invGeneralScaling).to_char(),
														MeasurementSystem::as_meters(previewRGI.get_play_area_rect_size().y * invHorizontalScaling * invGeneralScaling).to_char(),
														MeasurementSystem::as_meters(previewRGI.get_play_area_rect_size().x * invGeneralScaling).to_char(),
														MeasurementSystem::as_meters(previewRGI.get_play_area_rect_size().y * invGeneralScaling).to_char()
														));
			}
		}
		if (auto * w = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(idCurrentTileCount))))
		{
			w->set(String::printf(TXT("%i x %i"), previewRGI.get_tile_count().x, previewRGI.get_tile_count().y));
		}
		if (auto * w = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(idCurrentTileCountInfo))))
		{
			w->set(String(previewRGI.get_tile_count_info_as_char()));
		}
		if (auto * w = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(idCurrentTileSize))))
		{
			if (ms == MeasurementSystem::Imperial)
			{
				w->set(String::printf(TXT("%S~(%S)"),	MeasurementSystem::as_feet_inches(previewRGI.get_tile_size() * invHorizontalScaling * invGeneralScaling).to_char(),
														MeasurementSystem::as_feet_inches(previewRGI.get_tile_size() * invGeneralScaling).to_char()));
			}
			else
			{
				w->set(String::printf(TXT("%S~(%S)"),	MeasurementSystem::as_centimeters(previewRGI.get_tile_size() * invHorizontalScaling * invGeneralScaling).to_char(),
														MeasurementSystem::as_centimeters(previewRGI.get_tile_size() * invGeneralScaling).to_char()));
			}
		}
		if (auto * w = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(idCurrentDoorHeight))))
		{
			float actualDoorHeight = previewRGI.get_door_height() * invGeneralScaling;
			if (ms == MeasurementSystem::Imperial)
			{
				w->set(MeasurementSystem::as_feet_inches(actualDoorHeight));
			}
			else
			{
				w->set(MeasurementSystem::as_centimeters(actualDoorHeight));
			}
		}		
		if (auto * w = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(idCurrentScaleUpPlayer))))
		{
			w->set(scaleUp? NAME(lsPlayAreaScaleUpPlayerYes) : NAME(lsPlayAreaScaleUpPlayerNo));
		}		
		if (auto * w = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(idCurrentAllowCrouch))))
		{
			if (previewRGI.does_allow_crouch())
			{
				w->set(NAME(lsPlayAreaAllowCrouchAllow));
			}
			else
			{
				w->set(NAME(lsPlayAreaAllowCrouchDisallow));
			}
		}
	}
}

void Options::update_sights_colour(bool _colour)
{
	if (auto* scr = get_hub()->get_screen(NAME(optionsGameplay)))
	{
		if (auto* t = fast_cast<HubWidgets::Text>(scr->get_widget(NAME(idSights))))
		{
			if (_colour)
			{
				Colour sightsColour;
				sightsColour.r = (float)gameplayOptions.sightsColourRed / 100.0f;
				sightsColour.g = (float)gameplayOptions.sightsColourGreen / 100.0f;
				sightsColour.b = (float)gameplayOptions.sightsColourBlue / 100.0f;
				sightsColour.a = 1.0f;
				t->set_colour(sightsColour);
			}
			else
			{
				t->set_colour(NP);
			}
		}
	}
}
