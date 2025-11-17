#include "game.h"

//#define BREAK_BUILD_PRESENCE_LINKS

#include "..\..\core\profilePerformanceSettings.h"
#include "..\..\core\profilePerformanceSettings.h"
#include "..\..\core\recordVideoSettings.h"
#include "..\..\core\types\names.h"
#include "..\..\core\types\storeInScope.h"

#include "..\..\framework\game\gameSceneLayer.h"

#include "..\artDir.h"
#include "..\reportBug.h"
#include "..\modules\moduleAI.h"
#include "..\modules\moduleMovementPilgrimKeeper.h"
#include "..\modules\custom\mc_highlightObjects.h"
#include "..\modules\custom\mc_hitIndicator.h"

#include "environmentPlayer.h"
#include "gameConfig.h"
#include "gameDirector.h"
#include "gameLog.h"
#include "gameSettings.h"
#include "gameState.h"

#include "screenShotContext.h"

#include "..\teaForGodMain.h"

#include "..\library\library.h"

#include "modes\gameMode_Pilgrimage.h"
#include "modes\gameMode_Tutorial.h"

#include "..\modules\gameplay\modulePilgrim.h"

#include "..\..\framework\module\moduleCollisionHeaded.h"

#include "..\loaders\loaderCountdown.h"

#include "..\loaders\hub\loaderHubInSequence.h"
#include "..\loaders\hub\scenes\lhs_beAtRightPlace.h"
#include "..\loaders\hub\scenes\lhs_fadeOut.h"
#include "..\loaders\hub\scenes\lhs_loadingMessageSetBrowser.h"

#include "pointOfInterestInstance.h"

#include "..\ai\aiCommonVariables.h"
#include "..\ai\logics\aiLogic_pilgrim.h"
#include "..\ai\logics\managers\aiManagerLogic_regionManager.h"
#include "..\roomGenerators\roomGenerationInfo.h"

#include "..\music\musicPlayer.h"
#include "..\sound\subtitleSystem.h"
#include "..\sound\voiceoverSystem.h"

#include "..\..\framework\display\display.h"

#include "..\modules\modules.h"

#include "..\regionGenerators\regionGeneratorBase.h"

#include "..\pilgrimage\pilgrimageInstance.h"
#include "..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\loaders\loaderArmActivator.h"
#include "..\loaders\hub\loaderHub.h"
#include "..\loaders\hub\scenes\lhs_empty.h"
#include "..\loaders\hub\scenes\lhs_inGameMenu.h"
#include "..\loaders\hub\scenes\lhs_summary.h"
#include "..\loaders\hub\scenes\lhs_gameSlotSelection.h"
#include "..\loaders\hub\scenes\lhs_message.h"
#include "..\loaders\hub\scenes\lhs_pilgrimageSetup.h"
#include "..\loaders\hub\scenes\lhs_playerProfileSelection.h"
#include "..\loaders\hub\scenes\lhs_quickGameMenu.h"
#include "..\loaders\hub\scenes\lhs_setupNewPlayerProfile.h"
#include "..\loaders\hub\scenes\lhs_setupNewGameSlot.h"
#include "..\loaders\hub\scenes\lhs_test.h"
#include "..\loaders\hub\scenes\lhs_text.h"
#include "..\loaders\hub\scenes\lhs_tutorialDone.h"
#include "..\loaders\hub\scenes\lhs_tutorialSelection.h"

#include "..\overlayInfo\overlayInfoSystem.h"
#include "..\tutorials\tutorialSystem.h"

#include "..\..\framework\debugSettings.h"
#include "..\..\framework\ai\aiLogic.h"
#include "..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\framework\ai\aiMindInstance.h"
#include "..\..\framework\appearance\appearanceController.h"
#include "..\..\framework\appearance\appearanceControllerPoseContext.h"
#include "..\..\framework\debug\testConfig.h"
#include "..\..\framework\debug\debugPanel.h"
#include "..\..\framework\game\delayedObjectCreation.h"
#include "..\..\framework\game\poiManager.h"
#include "..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\framework\nav\navNode.h"
#include "..\..\framework\object\actor.h"
#include "..\..\framework\object\item.h"
#include "..\..\framework\pipelines\renderingPipeline.h"
#include "..\..\framework\sound\soundScene.h"
#include "..\..\framework\world\region.h"
#include "..\..\framework\world\roomRegionVariables.inl"
#include "..\..\framework\loaders\loaderSequence.h"
#include "..\..\framework\loaders\loaderText.h"
#include "..\..\framework\loaders\loaderVoidRoom.h"

#include "..\..\core\mainSettings.h"
#include "..\..\core\debug\extendedDebug.h"
#include "..\..\core\debug\debugVisualiser.h"
#include "..\..\core\io\outputWriter.h"
#include "..\..\core\other\parserUtils.h"
#include "..\..\core\system\recentCapture.h"
#include "..\..\core\system\sound\soundSystem.h"
#include "..\..\core\system\video\video3dPrimitives.h"
#include "..\..\core\system\video\renderTargetUtils.h"
#ifdef BUILD_NON_RELEASE
#include "..\..\core\system\video\renderTargetSave.h"
#endif
#include "..\..\core\physicalSensations\allPhysicalSensations.h"

#include "..\modules\custom\health\mc_health.h"
#include "..\modules\custom\health\mc_healthRegen.h"

#include "..\utils.h"

// debug/development
#include "..\..\framework\display\drawCommands\displayDrawCommand_textAt.h"
#include "..\..\framework\display\drawCommands\displayDrawCommand_texturePart.h"
#include "..\..\framework\nav\navMesh.h"
#include "..\..\framework\nav\navSystem.h"
#include "..\..\framework\nav\tasks\navTask_FindPath.h"
// !@#

#include "..\..\framework\library\usedLibraryStored.inl"

// TODO check all of this

#include <string>
#include <iterator>
#include <iostream>

#include "..\..\core\buildInformation.h"
#include "..\..\core\coreInit.h"
#include "..\..\core\mainConfig.h"
#include "..\..\core\collision\model.h"
#include "..\..\core\concurrency\asynchronousJobList.h"
#include "..\..\core\concurrency\semaphore.h"
#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\mesh\mesh3dBuilder.h"
#include "..\..\core\performance\performanceUtils.h"
#include "..\..\core\splash\splashInfo.h"
#include "..\..\core\system\core.h"
#include "..\..\core\system\input.h"
#include "..\..\core\system\video\video3d.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\viewFrustum.h"
#include "..\..\core\types\names.h"
#include "..\..\core\vr\iVR.h"

#include "..\..\framework\ai\aiLogics.h"
#include "..\..\framework\ai\aiMessage.h"
#include "..\ai\logics\aiLogics.h"
#include "..\..\framework\advance\advanceContext.h"
#include "..\..\framework\collision\checkCollisionContext.h"
#include "..\..\framework\debug\previewGame.h"
#include "..\..\framework\game\game.h"
#include "..\..\framework\game\gameInputDefinition.h"
#include "..\..\framework\game\player.h"
#include "..\..\framework\game\tweakingContext.h"
#include "..\..\framework\jobSystem\jobSystem.h"
#include "..\..\framework\library\library.h"
#include "..\..\framework\loaders\loaderCircles.h"
#include "..\..\framework\loaders\loaderZX.h"
#include "..\..\framework\module\moduleAppearance.h"
#include "..\..\framework\module\moduleController.h"
#include "..\..\framework\module\modulePresence.h"
#include "..\..\framework\module\modules.h"
#include "..\..\framework\pipelines\pipelines.h"
#include "..\..\framework\pipelines\postProcessPipeline.h"
#include "..\..\framework\postProcess\postProcessInstance.h"
#include "..\..\framework\postProcess\chain\postProcessChain.h"
#include "..\..\framework\postProcess\chain\postProcessChainProcessElement.h"
#include "..\..\framework\render\renderContext.h"
#include "..\..\framework\render\renderScene.h"
#include "..\..\framework\render\roomProxy.h"
#include "..\..\framework\world\door.h"
#include "..\..\framework\world\doorInRoom.h"
#include "..\..\framework\world\presenceLink.h"
#include "..\..\framework\world\room.h"
#include "..\..\framework\world\world.h"
#include "..\..\framework\world\subWorld.h"

#include "..\..\tools\tools.h"

#ifdef AN_ALLOW_BULLSHOTS
#include "..\..\framework\game\bullshotSystem.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#ifdef AN_DEVELOPMENT
#endif

REMOVE_AS_SOON_AS_POSSIBLE_
#define AN_LOG_META_STATES

//#define SHOW_DOOR_IN_ROOM_HISTORY

#define AN_LOG_GAME_STATE

// turn on additonal debug
//#define ADDITIONAL_DEBUG
// various options:
//#define ADDITIONAL_DEBUG__HAND_TRACKING
//#define ADDITIONAL_DEBUG__LAST_FOVEATED_RENDERING_SETUP

//#define WORLD_MANAGER_JOB_LIST

//

enum AppearanceRenderingPriority
{
	VignetteForMovement = 7000,
	VignetteForDead = 8000,
	TakingControls = 9000,
	Fading = 10000,
};

//

// cell/room/region custom parameters
DEFINE_STATIC_NAME(minPlayAreaTileCount);
DEFINE_STATIC_NAME(minPlayAreaSize);

DEFINE_STATIC_NAME(locomotion);
DEFINE_STATIC_NAME(locomotionPath);
DEFINE_STATIC_NAME(navMesh);
DEFINE_STATIC_NAME(moduleMovementInspect);
DEFINE_STATIC_NAME(moduleMovementDebug);
DEFINE_STATIC_NAME(moduleMovementPath);
DEFINE_STATIC_NAME(moduleMovementSwitch);
DEFINE_STATIC_NAME(moduleCollisionUpdateGradient);
DEFINE_STATIC_NAME(moduleCollisionQuerries);
DEFINE_STATIC_NAME(moduleCollisionCheckIfColliding);
DEFINE_STATIC_NAME(ai_draw);
DEFINE_STATIC_NAME(ai_enemyPerception);
DEFINE_STATIC_NAME(ac_walkers);
DEFINE_STATIC_NAME(ac_walkers_hitLocations);
DEFINE_STATIC_NAME(ac_footAdjuster);
DEFINE_STATIC_NAME(ac_ikSolver2);
DEFINE_STATIC_NAME(ac_ikSolver3);
DEFINE_STATIC_NAME(ac_tentacle);
DEFINE_STATIC_NAME(ac_readyHumanoidArm);
DEFINE_STATIC_NAME(ac_readyHumanoidSpine);
DEFINE_STATIC_NAME(ac_rotateTowards);
DEFINE_STATIC_NAME(ac_alignAxes);
DEFINE_STATIC_NAME(ac_alignmentRoll);
DEFINE_STATIC_NAME(ac_dragBone);
DEFINE_STATIC_NAME(ac_elevatorCartRotatingCore);
DEFINE_STATIC_NAME(ac_elevatorCartAttacherArm);
DEFINE_STATIC_NAME(ac_gyro);
DEFINE_STATIC_NAME(apc_airTraffic);
DEFINE_STATIC_NAME(pilgrim_physicalViolence);
DEFINE_STATIC_NAME(soundSceneFindSounds);
DEFINE_STATIC_NAME(soundSceneShowSounds);
DEFINE_STATIC_NAME(MissingSounds);
DEFINE_STATIC_NAME(SoundScene);
DEFINE_STATIC_NAME(SoundSceneLogEveryFrame);
DEFINE_STATIC_NAME(musicPlayer);
DEFINE_STATIC_NAME(voiceoverSystem);
DEFINE_STATIC_NAME(MissingTemporaryObjects);
DEFINE_STATIC_NAME(explosions);
DEFINE_STATIC_NAME(grabable);
DEFINE_STATIC_NAME(pilgrimGrab);
DEFINE_STATIC_NAME(ai_energyKiosk);
DEFINE_STATIC_NAME(ai_elevatorCart);
DEFINE_STATIC_NAME(pilgrim_guidance);

DEFINE_STATIC_NAME(dboGenerateDevMeshes);
DEFINE_STATIC_NAME(dboDebugRenderer);
DEFINE_STATIC_NAME(dboDebugRendererBlends);
DEFINE_STATIC_NAME(dboPerformanceMeasure);
DEFINE_STATIC_NAME(dboPerformanceMeasureShow);
DEFINE_STATIC_NAME(dboPerformanceMeasureShowNoRender);
DEFINE_STATIC_NAME(dboPerformanceMeasureShowNoGameLoop);
DEFINE_STATIC_NAME(dboDebugCameraMode);
DEFINE_STATIC_NAME(dboDebugCameraSpeed);
DEFINE_STATIC_NAME(dboResetDebugCamera);
DEFINE_STATIC_NAME(dboClearDebugSubject);
DEFINE_STATIC_NAME(dboPlayerDebugSubject);
DEFINE_STATIC_NAME(dboPilgrimKeeperSubject);
DEFINE_STATIC_NAME(dboAIManagerDebugSelect);
DEFINE_STATIC_NAME(dboDynamicSpawnManagerDebugSelect);
DEFINE_STATIC_NAME(dboRegionManagerDebugSelect);
DEFINE_STATIC_NAME(dboGameScriptExecutorDebugSelect);
DEFINE_STATIC_NAME(dboSelectDebugSubject);
DEFINE_STATIC_NAME(dboSelectDebugSubjectNextAI);
DEFINE_STATIC_NAME(dboSelectDebugSubjectNextInRoomAI);
DEFINE_STATIC_NAME(dboSelectDebugSubjectNextInRoom);
DEFINE_STATIC_NAME(dboDontRenderRoomProxyIndexM);
DEFINE_STATIC_NAME(dboDontRenderRoomProxyIndexP);
DEFINE_STATIC_NAME(dboDontRenderRoomProxyInverse);
DEFINE_STATIC_NAME(dboRenderStats);
DEFINE_STATIC_NAME(dboRenderSceneOutput);
DEFINE_STATIC_NAME(dboRenderSceneLog);
DEFINE_STATIC_NAME(dboRenderDetails);
DEFINE_STATIC_NAME(dboRenderDetailsLock);
DEFINE_STATIC_NAME(dboRenderDetailsLog);
DEFINE_STATIC_NAME(dboShowDebugInfo);
DEFINE_STATIC_NAME(dboShowVRLogPanel);
DEFINE_STATIC_NAME(dboLogHitchDetails);
DEFINE_STATIC_NAME(dboStopOnAnyHitch);
DEFINE_STATIC_NAME(dboStopOnHighHitch);
DEFINE_STATIC_NAME(dboInspectMovement);
DEFINE_STATIC_NAME(dboDrawMovement);
DEFINE_STATIC_NAME(dboDrawMovementPath);
DEFINE_STATIC_NAME(dboDrawMovementSwitch);
DEFINE_STATIC_NAME(dboDrawCollisionGradient);
DEFINE_STATIC_NAME(dboDrawCollisionQuerries);
DEFINE_STATIC_NAME(dboDrawCollisionCheckIfColliding);
DEFINE_STATIC_NAME(dboDrawCollision);
DEFINE_STATIC_NAME(dboDrawCollisionMode);
DEFINE_STATIC_NAME(dboTestCollisions);
DEFINE_STATIC_NAME(dboLogRoomVariables);
DEFINE_STATIC_NAME(dboRoomInfo);
DEFINE_STATIC_NAME(dboDrawDoors);
DEFINE_STATIC_NAME(dboDrawVolumetricLimit);
DEFINE_STATIC_NAME(dboDrawSkeletons);
DEFINE_STATIC_NAME(dboDrawSockets);
DEFINE_STATIC_NAME(dboDrawPOIs);
DEFINE_STATIC_NAME(dboDrawMeshNodes);
DEFINE_STATIC_NAME(dboDrawPresence);
DEFINE_STATIC_NAME(dboDrawPresenceBaseInfo);
DEFINE_STATIC_NAME(dboLogAllPresences);
DEFINE_STATIC_NAME(dboRoomGenerationInfo);
DEFINE_STATIC_NAME(dboVRAnchor);
DEFINE_STATIC_NAME(dboDrawSpaceBlockers);
DEFINE_STATIC_NAME(dboDrawVRPlayArea);
DEFINE_STATIC_NAME(dboDrawVRControls);
DEFINE_STATIC_NAME(dboVariables);
DEFINE_STATIC_NAME(dboAICommonVariables);
DEFINE_STATIC_NAME(dboAILatent);
DEFINE_STATIC_NAME(dboAIDraw);
DEFINE_STATIC_NAME(dboAIEnemyPerception);
DEFINE_STATIC_NAME(dboAILog);
DEFINE_STATIC_NAME(dboAILogic);
DEFINE_STATIC_NAME(dboAIHunches);
DEFINE_STATIC_NAME(dboAIState);
DEFINE_STATIC_NAME(dboLocomotion);
DEFINE_STATIC_NAME(dboLocomotionPath);
DEFINE_STATIC_NAME(dboNavTasks);
DEFINE_STATIC_NAME(dboDrawNavMesh);
DEFINE_STATIC_NAME(dboFindLocationOnNavMesh);
DEFINE_STATIC_NAME(dboFindPathOnNavMesh);
DEFINE_STATIC_NAME(dboPilgrimGuidance);
DEFINE_STATIC_NAME(dboPilgrimPhysicalViolence);
DEFINE_STATIC_NAME(dboListACs);
DEFINE_STATIC_NAME(dboACWalkers);
DEFINE_STATIC_NAME(dboACWalkersHit);
DEFINE_STATIC_NAME(dboACFootAdjuster);
DEFINE_STATIC_NAME(dboACIKSolver2);
DEFINE_STATIC_NAME(dboACIKSolver3);
DEFINE_STATIC_NAME(dboACTentacle);
DEFINE_STATIC_NAME(dboACReadyHumanoidArm);
DEFINE_STATIC_NAME(dboACReadyHumanoidSpine);
DEFINE_STATIC_NAME(dboACRotateTowards);
DEFINE_STATIC_NAME(dboACAlignAxes);
DEFINE_STATIC_NAME(dboACAlignmentRoll);
DEFINE_STATIC_NAME(dboACDragBone);
DEFINE_STATIC_NAME(dboACElevatorCartRotatingCore);
DEFINE_STATIC_NAME(dboACElevatorCartAttacherArm);
DEFINE_STATIC_NAME(dboACGyro);
DEFINE_STATIC_NAME(dboAPCAirTraffic);
DEFINE_STATIC_NAME(dboSoundMissingSounds);
DEFINE_STATIC_NAME(dboSoundSystem);
DEFINE_STATIC_NAME(dboSoundScene);
DEFINE_STATIC_NAME(dboSoundSceneLogEveryFrame);
DEFINE_STATIC_NAME(dboSoundSceneOutput);
DEFINE_STATIC_NAME(dboSoundSceneFindSounds);
DEFINE_STATIC_NAME(dboSoundSceneShowSounds);
DEFINE_STATIC_NAME(dboSoundCameraAtNormalCamera);
DEFINE_STATIC_NAME(dboMusicPlayer);
DEFINE_STATIC_NAME(dboEnvironmentPlayer);
DEFINE_STATIC_NAME(dboVoiceoverSystem);
DEFINE_STATIC_NAME(dboSubtitleSystem);
DEFINE_STATIC_NAME(dboMissingTemporaryObjects);
DEFINE_STATIC_NAME(dboPilgrimage);
DEFINE_STATIC_NAME(dboExplosions);
DEFINE_STATIC_NAME(dboGrabable);
DEFINE_STATIC_NAME(dboPilgrimGrab);
DEFINE_STATIC_NAME(dboAIEnergyKiosk);
DEFINE_STATIC_NAME(dboAIElevatorCart);
DEFINE_STATIC_NAME(dboGameplayStuff);
DEFINE_STATIC_NAME(dboGameDirector);
DEFINE_STATIC_NAME(dboLockerRoom);
DEFINE_STATIC_NAME(dboPilgrim);
DEFINE_STATIC_NAME(dboHighLevelRegionInfo);
DEFINE_STATIC_NAME(dboCellLevelRegionInfo);
DEFINE_STATIC_NAME(dboRoomRegion);
DEFINE_STATIC_NAME(dboLogWorld);
DEFINE_STATIC_NAME(dboMiniMap);
DEFINE_STATIC_NAME(dboGameJobs);
DEFINE_STATIC_NAME(dboRareAdvance);
DEFINE_STATIC_NAME(dboBullshotSystemInfo);
DEFINE_STATIC_NAME(dboHardCopyBullshotObject);
DEFINE_STATIC_NAME(dboHardCopyBullshotObjects);

// core advance
DEFINE_STATIC_NAME(advanceProfileTimer);
DEFINE_STATIC_NAME(advanceRunTimer);
DEFINE_STATIC_NAME(advanceActiveRunTimer);
DEFINE_STATIC_NAME(advanceRealWorldRunTimer)

// controls
DEFINE_STATIC_NAME(leftVRDebugPanel);
DEFINE_STATIC_NAME(rightVRDebugPanel);
DEFINE_STATIC_NAME(quickSelectDebugSubject);
DEFINE_STATIC_NAME(debugMovement);
DEFINE_STATIC_NAME(debugMovementVerticalSwitch);
DEFINE_STATIC_NAME(debugMovementSlow);
DEFINE_STATIC_NAME(debugMovementSlowAlt);
DEFINE_STATIC_NAME(debugMovementSlowAlt2);
DEFINE_STATIC_NAME(debugRotation);
DEFINE_STATIC_NAME(debugRotationSlow);
DEFINE_STATIC_NAME(debugRotationSlowAlt);
DEFINE_STATIC_NAME(debugRotationSlowAlt2);
DEFINE_STATIC_NAME(debugRequestInGameMenu);
DEFINE_STATIC_NAME(debugStoreCameraPlacement);
DEFINE_STATIC_NAME(debugNextStoredCameraPlacement);
DEFINE_STATIC_NAME(logPostProcess);
DEFINE_STATIC_NAME(logPostProcessDetailedSwitch);

// appearance
DEFINE_STATIC_NAME(fading);
DEFINE_STATIC_NAME_STR(takingControls, TXT("taking controls"));
DEFINE_STATIC_NAME_STR(vignetteForMovement, TXT("vignette for movement"));
DEFINE_STATIC_NAME_STR(vignetteForDead, TXT("vignette for dead"));

// material params
DEFINE_STATIC_NAME(fadingAmount);
DEFINE_STATIC_NAME(fadingColour);
DEFINE_STATIC_NAME(vignetteForMovementDot);
DEFINE_STATIC_NAME(vignetteForMovementFeather);
DEFINE_STATIC_NAME(vignetteForDeadBackAt);
DEFINE_STATIC_NAME(vignetteForDeadFrontAt);
DEFINE_STATIC_NAME(vignetteForDeadOrientation);
DEFINE_STATIC_NAME(takingControlsBackAt);
DEFINE_STATIC_NAME(takingControlsFrontAt);

// generation tags
DEFINE_STATIC_NAME(openWorld);

// region tags
DEFINE_STATIC_NAME(cellLevelRegion);
DEFINE_STATIC_NAME(cellLevelRegionInfo);
DEFINE_STATIC_NAME(highLevelRegion);
DEFINE_STATIC_NAME(highLevelRegionInfo);

// room tags
DEFINE_STATIC_NAME(openWorldX);
DEFINE_STATIC_NAME(openWorldY);
DEFINE_STATIC_NAME(cellRoomIdx);
DEFINE_STATIC_NAME(lockerRoom);

// object variables
DEFINE_STATIC_NAME(heightForExternalView);

// room/region variables
DEFINE_STATIC_NAME(maxPlayAreaSize);
DEFINE_STATIC_NAME(maxPlayAreaTileSize);

// test tags
DEFINE_STATIC_NAME(testRoom);
DEFINE_STATIC_NAME(testPilgrimage);
DEFINE_STATIC_NAME(autoTestProcessAllJobs);
DEFINE_STATIC_NAME(autoTestIntervalInMinutes);
DEFINE_STATIC_NAME(autoTestInterval);
DEFINE_STATIC_NAME(testMultiplePlayAreaRectSizes);
DEFINE_STATIC_NAME(testMultiplePlayAreaMaxSize);
DEFINE_STATIC_NAME(testMultiplePlayAreaSquarishChance);
DEFINE_STATIC_NAME(testPlayArea);
DEFINE_STATIC_NAME(testTileSize);
DEFINE_STATIC_NAME(testGameDefinition);
DEFINE_STATIC_NAME(testGameSubDefinition);
DEFINE_STATIC_NAME(testWelcomeScene);

// localised strings
DEFINE_STATIC_NAME_STR(lsPleaseWait, TXT("hub; common; please wait"));
DEFINE_STATIC_NAME_STR(lsTutorial, TXT("tutorials; tutorial"));
DEFINE_STATIC_NAME_STR(lsLoadingTips, TXT("hub; common; loading tips"));
DEFINE_STATIC_NAME_STR(lsToBeContinued, TXT("demo; to be continued"));
DEFINE_STATIC_NAME_STR(lsMoveToValidPosition, TXT("hub; common; move to valid position"));
DEFINE_STATIC_NAME_STR(lsMoveToStartingPosition, TXT("hub; common; move to starting position"));
DEFINE_STATIC_NAME_STR(lsDisableKeepInWorld, TXT("hub; keep in world; disable"));
	DEFINE_STATIC_NAME(time);
DEFINE_STATIC_NAME_STR(lsPermanentlyDisableKeepInWorld, TXT("hub; keep in world; disable permanently"));
DEFINE_STATIC_NAME_STR(lsReachedEnd, TXT("game; reached end"));
DEFINE_STATIC_NAME_STR(lsReachedEndExperience, TXT("game; reached end; experience"));
DEFINE_STATIC_NAME_STR(lsReachedDemoEnd, TXT("game; reached demo end"));
//DEFINE_STATIC_NAME_STR(lsReachedDemoEndExperience, TXT("game; reached demo end; experience"));
DEFINE_STATIC_NAME_STR(lsMissionsUnlocked, TXT("missions unlocked"));

// fading reasons
DEFINE_STATIC_NAME(gameEnded);
DEFINE_STATIC_NAME(inGameMenuRequested);
DEFINE_STATIC_NAME(profile);
DEFINE_STATIC_NAME(keepInWorld);
DEFINE_STATIC_NAME(resurrect);

// colours
DEFINE_STATIC_NAME(loader_hub_fade);

// shader options
DEFINE_STATIC_NAME(pointLights);
DEFINE_STATIC_NAME(coneLights);

// system tags
DEFINE_STATIC_NAME(lowGraphics);

// shader program params
DEFINE_STATIC_NAME(ignoreRetro);

// bullshot system allowances
DEFINE_STATIC_NAME_STR(bsNoDevelopmentVolume, TXT("no development volume"));
DEFINE_STATIC_NAME_STR(bsNoLoadingScreens, TXT("no loading screens"));
DEFINE_STATIC_NAME_STR(bsDisableDebugRendering, TXT("disable debug rendering"));

// library global references
DEFINE_STATIC_NAME_STR(grLoaderHubLoadingScene, TXT("loader hub loading scene"));
DEFINE_STATIC_NAME_STR(grLoaderHubLoadingSceneBackground, TXT("loader hub loading scene background"));
DEFINE_STATIC_NAME_STR(grLoaderHubLoadingSceneEnvironment, TXT("loader hub loading scene environment"));
//DEFINE_STATIC_NAME_STR(loaderHubProfileScene, TXT("loader hub profile scene"));
DEFINE_STATIC_NAME_STR(loaderHubProfileScene, TXT("loader hub terminal scene"));
DEFINE_STATIC_NAME_STR(gameDefinitionComplexRules, TXT("game definition; complex rules"));
DEFINE_STATIC_NAME_STR(gameDefinitionQuickGame, TXT("game definition; quick game"));
DEFINE_STATIC_NAME_STR(gameDefinitionTutorial, TXT("game definition; tutorial"));
DEFINE_STATIC_NAME_STR(gameDefinitionTestRoom, TXT("game definition; test room"));
DEFINE_STATIC_NAME_STR(gsLoadingMusic, TXT("music; loading music"));
DEFINE_STATIC_NAME_STR(gsMenuMusic, TXT("music; menu music"));
DEFINE_STATIC_NAME_STR(gsMenuThemeMusic, TXT("music; menu theme"));
DEFINE_STATIC_NAME_STR(grKeepInWorldHelp, TXT("hub; help; keep in world"));

// destruction suspension reasons
DEFINE_STATIC_NAME(storingGameState);

// collision flags
DEFINE_STATIC_NAME(worldSolidFlags);
DEFINE_STATIC_NAME(pilgrimOverlayInfoTraceBlocker);

// general progress
DEFINE_STATIC_NAME(finishedMainStory);
DEFINE_STATIC_NAME(confirmedMissions);

//

using namespace TeaForGodEmperor;

//

#ifdef AN_ALLOW_BULLSHOTS
static int bullshotTrapIdx = 0;
#endif

//

void set_ignore_retro_rendering_for(::Framework::Font* font, bool _ignore)
{
	if (auto* f = font->get_font())
	{
		// should not set while rendering other stuff
		//f->access_fallback_shader_program_instance()->set_uniform(NAME(ignoreRetro), _ignore ? 1.0f : 0.0f);
		//f->access_fallback_no_blend_shader_program_instance()->set_uniform(NAME(ignoreRetro), _ignore ? 1.0f : 0.0f);
	}
}

#ifdef AN_DEVELOPMENT_OR_PROFILER
struct StoredDebugCameraPlacement
{
	Transform placement = Transform::identity;
	VectorInt2 openWorld = VectorInt2::zero;
	int cellRoomIdx = 0;
};
Array<StoredDebugCameraPlacement> debugCameraPlacements;
int debugCameraPlacementIdx = NONE;

String get_debug_camera_placements_file()
{
	return IO::get_user_game_data_directory() + IO::get_auto_directory_name() + IO::get_directory_separator() + String(TXT("debugCameraPlacements"));
}

void load_debug_camera_placements()
{
	output(TXT("load_debug_camera_placements"));
	debugCameraPlacements.clear();
	IO::File file;
	String fileName = get_debug_camera_placements_file();
	if (file.open(fileName))
	{
		file.set_type(IO::InterfaceType::Text);
		IO::XML::Document doc;
		if (doc.load_xml(&file))
		{
			for_every(node, doc.get_root()->children_named(TXT("debugCameraPlacements")))
			{
				for_every(dcnode, node->children_named(TXT("debugCameraPlacement")))
				{
					output(TXT(" loading placement #%i"), debugCameraPlacements.get_size());
					StoredDebugCameraPlacement dc;
					for_every(vnode, dcnode->children_named(TXT("translation")))
					{
						Vector3 v = Vector3::zero;
						v.x = vnode->get_float_attribute(TXT("x"));
						v.y = vnode->get_float_attribute(TXT("y"));
						v.z = vnode->get_float_attribute(TXT("z"));
						dc.placement.set_translation(v);
					}
					for_every(vnode, dcnode->children_named(TXT("orientation")))
					{
						Quat v = Quat::identity;
						v.x = vnode->get_float_attribute(TXT("x"));
						v.y = vnode->get_float_attribute(TXT("y"));
						v.z = vnode->get_float_attribute(TXT("z"));
						v.w = vnode->get_float_attribute(TXT("w"));
						dc.placement.set_orientation(v);
					}
					for_every(vnode, dcnode->children_named(TXT("translationi")))
					{
						Vector3 v = Vector3::zero;
						int t;
						t = vnode->get_int_attribute(TXT("x"));	v.x = *(float*)&t;
						t = vnode->get_int_attribute(TXT("y"));	v.y = *(float*)&t;
						t = vnode->get_int_attribute(TXT("z"));	v.z = *(float*)&t;
						dc.placement.set_translation(v);
					}
					for_every(vnode, dcnode->children_named(TXT("orientationi")))
					{
						Quat v = Quat::identity;
						int t;
						t = vnode->get_int_attribute(TXT("x"));	v.x = *(float*)&t;
						t = vnode->get_int_attribute(TXT("y"));	v.y = *(float*)&t;
						t = vnode->get_int_attribute(TXT("z"));	v.z = *(float*)&t;
						t = vnode->get_int_attribute(TXT("w"));	v.w = *(float*)&t;
						dc.placement.set_orientation(v);
					}
					{
						float v = dcnode->get_float_attribute_or_from_child(TXT("scale"), 1.0f);
						dc.placement.set_scale(v);
					}
					if (auto* a = dcnode->get_attribute(TXT("scalei")))
					{
						int t = a->get_as_int();
						dc.placement.set_scale(*(float*)&t);
					}

					for_every(vnode, dcnode->children_named(TXT("openWorld")))
					{
						dc.openWorld.load_from_xml(vnode);
						dc.cellRoomIdx = vnode->get_int_attribute(TXT("cellRoomIdx"));
					}
					debugCameraPlacements.push_back(dc);
				}
			}
			output(TXT("loaded document"));
		}
		else
		{
			output(TXT("load document failed"));
		}
	}
	else
	{
		output(TXT("no file \"%S\""), fileName.to_char());
	}
}

void save_debug_camera_placements()
{
	output(TXT("save_debug_camera_placements (%i)"), debugCameraPlacements.get_size());
	IO::XML::Document doc;
	if (auto* node = doc.get_root()->add_node(TXT("debugCameraPlacements")))
	{
		for_every(dc, debugCameraPlacements)
		{
			if (auto* dcnode = node->add_node(TXT("debugCameraPlacement")))
			{
				if (auto* vnode = dcnode->add_node(TXT("translation")))
				{
					Vector3 v = dc->placement.get_translation();
					vnode->set_attribute(TXT("x"), String::printf(TXT("%.20f"), v.x));
					vnode->set_attribute(TXT("y"), String::printf(TXT("%.20f"), v.y));
					vnode->set_attribute(TXT("z"), String::printf(TXT("%.20f"), v.z));
				}
				if (auto* vnode = dcnode->add_node(TXT("orientation")))
				{
					Quat v = dc->placement.get_orientation();
					vnode->set_attribute(TXT("x"), String::printf(TXT("%.20f"), v.x));
					vnode->set_attribute(TXT("y"), String::printf(TXT("%.20f"), v.y));
					vnode->set_attribute(TXT("z"), String::printf(TXT("%.20f"), v.z));
					vnode->set_attribute(TXT("w"), String::printf(TXT("%.20f"), v.w));
				}
				if (auto* vnode = dcnode->add_node(TXT("translationi")))
				{
					Vector3 v = dc->placement.get_translation();
					vnode->set_attribute(TXT("x"), String::printf(TXT("%i"), *(int*)&v.x));
					vnode->set_attribute(TXT("y"), String::printf(TXT("%i"), *(int*)&v.y));
					vnode->set_attribute(TXT("z"), String::printf(TXT("%i"), *(int*)&v.z));
				}
				if (auto* vnode = dcnode->add_node(TXT("orientationi")))
				{
					Quat v = dc->placement.get_orientation();
					vnode->set_attribute(TXT("x"), String::printf(TXT("%i"), *(int*)&v.x));
					vnode->set_attribute(TXT("y"), String::printf(TXT("%i"), *(int*)&v.y));
					vnode->set_attribute(TXT("z"), String::printf(TXT("%i"), *(int*)&v.z));
					vnode->set_attribute(TXT("w"), String::printf(TXT("%i"), *(int*)&v.w));
				}
				{
					float scale = dc->placement.get_scale();
					dcnode->set_attribute(TXT("scale"), String::printf(TXT("%.20f"), scale));
					dcnode->set_attribute(TXT("scalei"), String::printf(TXT("%i"), *(int*)&scale));
				}
				if (auto* vnode = dcnode->add_node(TXT("openWorld")))
				{
					dc->openWorld.save_to_xml(vnode);
					vnode->set_int_attribute(TXT("cellRoomIdx"), dc->cellRoomIdx);
				}
			}
		}
		IO::File file;
		String fileName = get_debug_camera_placements_file();
		if (IO::File::does_exist(fileName))
		{
			IO::File::delete_file(fileName);
		}
		if (file.create(fileName))
		{
			file.set_type(IO::InterfaceType::Text);
			doc.save_xml(&file);
		}
	}
}
#endif

//

#ifndef AN_CLANG
float nonVRCameraFov = 80.0f;
#endif

int font_size_multiplier()
{
	static int rel = 2;
#ifdef AN_STANDARD_INPUT
	static int lastFrame = 0;
	if (lastFrame != System::Core::get_frame())
	{
		lastFrame = System::Core::get_frame();
		if (System::Input::get()->has_key_been_pressed(System::Key::KP_Minus))
		{
			if (rel >= 2)
			{
				rel = 1;
			}
			else
			{
				rel = 2;
			}
		}
	}
#endif
	return rel;
}

float get_font_size_rel_to_rt_size()
{
	return ((float)font_size_multiplier()) / 150.0f;
}

//

bool is_temp_file(String const& filename)
{
	return filename.has_suffix(TXT(".temp"));
}

bool load_xml_using_temp_file(IO::XML::Document & doc, String const& filenameNormal)
{
	bool fileExists = false;
	for_count(int, tryIdx, 2)
	{
		String filename = filenameNormal;
		if (tryIdx == 1)
		{
			filename += TXT(".temp");
		}
		if (IO::File::does_exist(filename))
		{
			fileExists = true;
			IO::File file;
			if (file.open(filename))
			{
				file.set_type(IO::InterfaceType::Text);
				if (doc.load_xml(&file))
				{
					if (tryIdx == 1)
					{
						// move temp to normal
						if (IO::File::does_exist(filenameNormal))
						{
							IO::File::delete_file(filenameNormal);
						}
						IO::File::rename_file(filename, filenameNormal);
					}
					return true;
				}
			}
		}

		// reset doc
		doc.reset();
	}
	warn(TXT("could not load file \"%S\" (%S)"), filenameNormal.to_char(), fileExists? TXT("file exists") : TXT("file does not exist"));
	return false;
}

void save_xml_to_file_using_temp_file(IO::XML::Document const& doc, String const& filename)
{
	String filenameTemp = filename + TXT(".temp");
	{
		IO::File file;
		if (IO::File::does_exist(filenameTemp))
		{
			IO::File::delete_file(filenameTemp);
		}
		if (file.create(filenameTemp))
		{
			file.set_type(IO::InterfaceType::Text);
			doc.save_xml(&file);
#ifdef AN_DEVELOPMENT
			if (false) // skip writing to output
			{
				IO::OutputWriter ow;
				doc.save_xml(&ow);
			}
#endif
		}
	}
	{
		// this is the crucial bit, if we break here, we're a bit messed
		// but we should not have issues as we use just rename
		if (IO::File::does_exist(filenameTemp))
		{
			if (IO::File::does_exist(filename))
			{
				IO::File::delete_file(filename);
			}
			IO::File::rename_file(filenameTemp, filename);
		}
	}
}

//

float Game::s_renderingNearZ = 0.02f;
float Game::s_renderingFarZ = 30000.0f;

REGISTER_FOR_FAST_CAST(Game);

Game::Game()
: soundScene(nullptr)
{
	Framework::ModuleGameplay::use__advance__post_logic(false); // no need for that

	Framework::GameStaticInfo::get()->set_sensitive_to_vr_anchor(true);

	allowSavingPlayerProfile = false;

	playerSetup.make_current();

#ifdef AN_DEVELOPMENT_OR_PROFILER
	debugRenderingSuspended = false;
	debugSelectDebugSubject = false;
	debugQuickSelectDebugSubject = false;
	debugInspectMovement = false;
	debugDrawCollision = 0;
	debugDrawCollisionMode = 1;
	debugLogRoomVariables = false;
	debugDrawDoors = false;
	debugRoomInfo = false;
	debugDrawVolumetricLimit = false;
	debugDrawSkeletons = false;
	debugDrawSockets = false;
	debugDrawPOIs = 0;
	debugDrawMeshNodes = false;
	debugTestCollisions = 0;
	debugDrawPresence = 0;
	debugDrawPresenceBaseInfo = 0;
	debugDrawCollisionGradient = 0;
	debugDrawRGI = 0;
	debugDrawVRAnchor = 0;
	debugDrawVRPlayArea = 0;
	debugDrawSpaceBlockers = 0;
	debugLogNavTasks = 0;
	debugFindLocationOnNavMesh = 0;
	debugFindPathOnNavMesh = 0;
	debugFindPathOnNavMeshBlockedFor = 0;
	debugVariables = 0;
	debugAILog = 0;
	debugAILogic = 0;
	debugAIHunches = 0;
	debugAICommonVariables = 0;
	debugAILatent = 0;
	debugLocomotion = 0;
	debugLocomotionPath = 0;
	debugAIState = 0;
	debugACList = 0;
	debugOutputRenderDetails = 0;
	debugOutputRenderDetailsLocked = 0;
	debugOutputRenderScene = 0;
	debugOutputSoundSystem = 0;
	debugOutputSoundScene = 0;
	debugOutputMusicPlayer = 0;
	debugOutputEnvironmentPlayer = 0;
	debugOutputVoiceoverSystem = 0;
	debugOutputSubtitleSystem = 0;
	debugOutputPilgrimageInfo = 0;
	debugOutputGameDirector = 0;
	debugOutputLockerRoom = 0;
	debugOutputPilgrim = 0;
	debugOutputGameplayStuff = 0;
#ifdef AN_ALLOW_BULLSHOTS
	debugOutputBullshotSystem = 0;
#endif
	debugHighLevelRegion.outputInfo = 0;
	debugCellLevelRegion.outputInfo = 0;
	debugRoomRegion.outputInfo = 0;
	debugShowWorldLog = 0;
	debugShowMiniMap = 1;
	debugShowGameJobs = 1;
#endif
	debugDrawVRControls = 0;
	debugControlsMode = 0;
	debugCameraMode = 0;
	debugCameraSpeed = 1.0f;

	settings.itemsRequireNoPostMove = true;
#ifdef AN_DEBUG_RENDERER
	Framework::Render::ContextDebug::use = &rcDebug;
#endif

#ifdef AN_RECORD_VIDEO
	allowShowHitchFrameIndicator = false;
	debugRenderingSuspended = true;
#endif

	// only show them on demand
#ifdef AN_PROFILER
	debugRenderingSuspended = true;
#ifdef AN_USE_FRAME_INFO
#ifdef RENDER_HITCH_FRAME_INDICATOR
	allowShowHitchFrameIndicator = false;
#endif
#endif
#endif

	ArtDir::customise_game(this);

	access_customisation().create_point_of_interest_instance = []() { return new PointOfInterestInstance(); };

	todo_note(TXT("think about better placement?"));
	GameSettings::create(this);
}

void Game::init_main_hub_loader()
{
	an_assert(!mainHubLoader);
	mainHubLoader = new Loader::Hub(this, nullptr, NAME(grLoaderHubLoadingScene), NAME(grLoaderHubLoadingSceneBackground), NAME(grLoaderHubLoadingSceneEnvironment));
}

void Game::init_other_hub_loaders()
{
	reinit_profile_hub_loader();
}

void Game::reinit_profile_hub_loader()
{
	return; // not using right now
	/*
	ASSERT_ASYNC;
	delete_and_clear(profileHubLoader);
	profileHubLoader = new Loader::Hub(this, TXT("profile"), NAME(loaderHubProfileScene), true);
	*/
}

void Game::close_hub_loaders()
{
	delete_and_clear(mainHubLoader);
	delete_and_clear(profileHubLoader);
}

bool Game::make_sure_hub_loader_is_valid(Loader::Hub*& _loader)
{
	if (_loader != mainHubLoader/* &&
		_loader != profileHubLoader*/)
	{
		_loader = mainHubLoader;
		return false;
	}
	return true;
}

void Game::ready_to_use_hub_loader(Loader::Hub* _loader)
{
	if (!_loader)
	{
		_loader = mainHubLoader;
	}
	if (_loader != recentHubLoader &&
		make_sure_hub_loader_is_valid(recentHubLoader))
	{
		bool doesAllowFadeOut = recentHubLoader->does_allow_fade_out();
		if (!doesAllowFadeOut)
		{
			recentHubLoader->allow_fade_out(); // restore normal fading and do empty scene to fade out
		}

		{	// actual fade out
			recentHubLoader->set_require_fade_out();
			recentHubLoader->deactivate_all_screens();
			while (get_fade_out() < 1.0f)
			{
				advance_and_display_loader(recentHubLoader); // it's deactivated but we want to fade it out
			}
			recentHubLoader->set_require_fade_out(false);
		}

		if (!doesAllowFadeOut)
		{
			recentHubLoader->disallow_fade_out();
		}
	}
}

void Game::create_library()
{
	Framework::Library::initialise_static<TeaForGodEmperor::Library>();
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (TeaForGodEmperor::is_demo())
	{
		Framework::Library::skip_loading_directory_with_suffix(String(TXT(".fullGame")));
	}
#endif
}

void Game::close_library(Framework::LibraryLoadLevel::Type _upToLibraryLoadLevel, Loader::ILoader* _loader)
{
	playerSetup.reset(); // clear it before we unload the library, otherwise we'll be pointing at objects that no longer should exist (but they still do exist!)
	PilgrimageInstance::drop_all_pilgrimage_data(true); // same reason, we don't want dangling pointers there
	an_assert(mainHubLoader == nullptr, TXT("call close_hub_loaders?"));
	base::close_library(_upToLibraryLoadLevel, _loader);
}

void Game::setup_library()
{
	base::setup_library();
	//todo_important("setup gamespecific components");
#ifdef AN_DEVELOPMENT
#ifndef AN_CLANG
	auto & libraryGameplay = access_customisation().library;
	//libraryGameplay.defaultGameplayModuleTypeForItemType = Name(TXT("item"));
#endif
#endif
}

void Game::load_game_settings()
{
	base::load_game_settings();

#ifdef NO_LIGHT_SOURCES
	MainSettings::access_global().access_shader_options().remove_tag(NAME(pointLights));
	MainSettings::access_global().access_shader_options().remove_tag(NAME(coneLights));
#endif
}

void Game::post_start()
{
	base::post_start();

	output(TXT("init non-game music player"));
	an_assert(nonGameMusicPlayer == nullptr);
#ifndef NO_MUSIC
#ifndef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS
	nonGameMusicPlayer = new MusicPlayer(true /* additional, not main */);
#endif
#endif

#ifdef WITH_DRAWING_BOARD
	assert_rendering_thread();
	::System::RenderTargetSetup rtSetup = ::System::RenderTargetSetup(VectorInt2(1024, 512), 0);
	DEFINE_STATIC_NAME(colour);
	rtSetup.add_output_texture(::System::OutputTextureDefinition(NAME(colour),
		::System::VideoFormat::rgba8,
		::System::BaseVideoFormat::rgba));
	RENDER_TARGET_CREATE_INFO(TXT("drawing board")); 
	drawingBoardRT = new ::System::RenderTarget(rtSetup);

	{
		::System::Video3D* v3d = ::System::Video3D::get();
		drawingBoardRT->bind();
		v3d->set_viewport(VectorInt2::zero, drawingBoardRT->get_size());
		v3d->setup_for_2d_display();
		v3d->set_2d_projection_matrix_ortho(drawingBoardRT->get_size().to_vector2());
		v3d->set_near_far_plane(0.02f, 100.0f);
		v3d->clear_colour(Colour::black);
		drawingBoardRT->bind_none();
	}

#endif

}

void Game::pre_close()
{
#ifdef WITH_DRAWING_BOARD
	assert_rendering_thread();
	drawingBoardRT->close(true);
	drawingBoardRT.clear();
#endif

	delete_and_clear(nonGameMusicPlayer);

	base::pre_close();
}

void Game::before_game_starts()
{
	base::before_game_starts();

#ifdef AN_DEVELOPMENT_OR_PROFILER
	load_debug_camera_placements();
#endif

#ifdef AN_ALLOW_BULLSHOTS
	if (Framework::BullshotSystem::is_active())
	{
		// start with debug camera
		debugControlsMode = 1;
		debugCameraMode = 1;
		resetDebugCamera = true;
#ifdef AN_NO_RARE_ADVANCE_AVAILABLE
		Framework::access_no_rare_advance() = true;
#endif
	}
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (Framework::BullshotSystem::is_setting_active(NAME(bsDisableDebugRendering)))
	{
		debugRenderingSuspended = true;
	}
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (Framework::BullshotSystem::is_setting_active(NAME(bsNoDevelopmentVolume)))
	{
		MainConfig::access_global().access_sound().developmentVolume = 1.0f;
		if (auto* ss = System::Sound::get()) ss->update_on_development_volume_change();
	}
#endif
#endif

	output(TXT("init overlay info system"));
	delete_and_clear(overlayInfoSystem);
	overlayInfoSystem = new OverlayInfo::System();
	overlayInfoSystem->update_assets();

	output(TXT("init game director"));
	delete_and_clear(gameDirector);
	gameDirector = new GameDirector();

	output(TXT("init game log"));
	delete_and_clear(gameLog);
	gameLog = new GameLog();
	gameLog->lookup_tutorials();

	output(TXT("init tutorial system"));
	delete_and_clear(tutorialSystem);
	tutorialSystem = new TutorialSystem();
	tutorialSystem->build_tree();

	output(TXT("init loot selector"));
	LootSelector::build_for_game();
#ifdef AN_DEVELOPMENT_OR_PROFILER
	LootSelector::output_loot_selector();
	LootSelector::output_loot_providers();
#endif

	output(TXT("init projectile selector"));
	ProjectileSelector::build_for_game();
#ifdef AN_DEVELOPMENT_OR_PROFILER
	ProjectileSelector::output_projectile_selector();
#endif

	output(TXT("init sound scene"));
	an_assert(soundScene == nullptr);
	soundScene = new ::Framework::Sound::Scene();
	todo_note(TXT("maybe load soundSceneWorldTagCondition from a config file?"));
	worldChannelTagCondition.load_from_string(hardcoded String(TXT("world")));
	soundFadeChannelTagCondition.load_from_string(hardcoded String(TXT("game"))); // applies only to sound scene
	audioDuckOnVoiceoverChannelTagCondition.load_from_string(hardcoded String(TXT("audioDuckOnVoiceover")));

	output(TXT("init music player"));
	an_assert(musicPlayer == nullptr);
#ifndef NO_MUSIC
	musicPlayer = new MusicPlayer();
#endif

	output(TXT("init subtitle system"));
	an_assert(subtitleSystem == nullptr);
	subtitleSystem = new SubtitleSystem();
	subtitleSystem->use(overlayInfoSystem);

	output(TXT("init voiceover system"));
	an_assert(voiceoverSystem == nullptr);
	voiceoverSystem = new VoiceoverSystem();
	voiceoverSystem->use(subtitleSystem);

	output(TXT("init post process chain"));
	if (Framework::PostProcessChainDefinition* chainDef = Framework::Library::get_current()->get_global_references().get<Framework::PostProcessChainDefinition>(
#ifdef DUMP
		Name(String(TXT("post process chain empty")))))
#else
		Name(String(TXT("post process chain default")))))
#endif
	{
		chainDef->setup(postProcessChain);

		overridePostProcessParams.clear();

		hardcoded

		DEFINE_STATIC_NAME(filterBloomThreshold);
		overridePostProcessParams.grow_size(1);
		{
			auto& o = overridePostProcessParams.get_last();
			o.param = NAME(filterBloomThreshold);
		}
	}
	else
	{
		postProcessChain->clear();
	}

	output(TXT("setup quick exit"));
	::System::Core::on_quick_exit(TXT("save player profile"), [this]() { save_player_profile(); });

#ifdef WITH_PHYSICAL_SENSATIONS
#ifndef AN_DEVELOPMENT
	output(TXT("initialise periferal devices"));
	{
		output(TXT("init phys sens player"));
		PhysicalSensations::initialise(true);
		get_job_system()->do_asynchronous_off_system_job(PhysicalSensations::IPhysicalSensations::do_async_init_thread_func, nullptr);
	}
#endif
#endif

	output(TXT("init environment player"));
	delete_and_clear(environmentPlayer);
	environmentPlayer = new EnvironmentPlayer();

	output(TXT("setup renderer mode"));
#ifdef AN_DEVELOPMENT
	debug_renderer_mode(DebugRendererMode::OnlyActiveFilters);
#else
	debug_renderer_mode(does_use_vr() ? DebugRendererMode::Off : DebugRendererMode::OnlyActiveFilters);
#endif
#ifdef AN_PERFORMANCE_MEASURE
	PERFORMANCE_MEASURE_ENABLE(true);// !does_use_vr());
#endif

#ifdef SOUND_MASTERING
#ifdef AN_DEVELOPMENT_OR_PROFILER
	MainConfig::access_global().access_sound().developmentVolume = 1.0f;
	if (auto* ss = System::Sound::get()) ss->update_on_development_volume_change();
	debugOutputMusicPlayer = true;
	debugOutputEnvironmentPlayer = true;
	debugOutputSoundSystem = true;
	debugOutputSoundScene = true;
	debug_renderer_mode(DebugRendererMode::Off);
	debugRenderingSuspended = false;
#endif
#endif

	// debug renderer/panel options
	{
#ifdef AN_DEBUG_RENDERER
		DebugRenderer::get()->clear_filters();
		DebugRenderer::get()->block_filter(NAME(moduleMovementDebug), true);
		DebugRenderer::get()->block_filter(NAME(moduleMovementPath), true);
		DebugRenderer::get()->block_filter(NAME(moduleMovementSwitch), true);
		DebugRenderer::get()->block_filter(NAME(moduleCollisionUpdateGradient), true);
		DebugRenderer::get()->block_filter(NAME(moduleCollisionQuerries), true);
		DebugRenderer::get()->block_filter(NAME(moduleCollisionCheckIfColliding), true);
		DebugRenderer::get()->block_filter(NAME(locomotion), true);
		DebugRenderer::get()->block_filter(NAME(locomotionPath), true);
		DebugRenderer::get()->block_filter(NAME(navMesh), true);
		DebugRenderer::get()->block_filter(NAME(ai_draw), true);
		DebugRenderer::get()->block_filter(NAME(ai_enemyPerception), true);
		DebugRenderer::get()->block_filter(NAME(ac_walkers), true);
		DebugRenderer::get()->block_filter(NAME(ac_walkers_hitLocations), true);
		DebugRenderer::get()->block_filter(NAME(ac_footAdjuster), true);
		DebugRenderer::get()->block_filter(NAME(ac_ikSolver2), true);
		DebugRenderer::get()->block_filter(NAME(ac_ikSolver3), true);
		DebugRenderer::get()->block_filter(NAME(ac_tentacle), true);
		DebugRenderer::get()->block_filter(NAME(ac_readyHumanoidArm), true);
		DebugRenderer::get()->block_filter(NAME(ac_readyHumanoidSpine), true);
		DebugRenderer::get()->block_filter(NAME(ac_rotateTowards), true);
		DebugRenderer::get()->block_filter(NAME(ac_alignAxes), true);
		DebugRenderer::get()->block_filter(NAME(ac_elevatorCartRotatingCore), true);
		DebugRenderer::get()->block_filter(NAME(ac_elevatorCartAttacherArm), true);
		DebugRenderer::get()->block_filter(NAME(ac_alignmentRoll), true);
		DebugRenderer::get()->block_filter(NAME(ac_dragBone), true);
		DebugRenderer::get()->block_filter(NAME(ac_gyro), true);
		DebugRenderer::get()->block_filter(NAME(apc_airTraffic), true);
		DebugRenderer::get()->block_filter(NAME(pilgrim_physicalViolence), true);
		DebugRenderer::get()->block_filter(NAME(soundSceneShowSounds), true);
		DebugRenderer::get()->block_filter(NAME(soundSceneFindSounds), true);
		DebugRenderer::get()->block_filter(NAME(explosions), true);
		DebugRenderer::get()->block_filter(NAME(grabable), true);
		DebugRenderer::get()->block_filter(NAME(pilgrimGrab), true);
		DebugRenderer::get()->block_filter(NAME(ai_energyKiosk), true);
		DebugRenderer::get()->block_filter(NAME(ai_elevatorCart), true);
		DebugRenderer::get()->block_filter(NAME(pilgrim_guidance), true);
		DebugRenderer::get()->mark_filters_ready();
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->be_relative_to_camera();
		debugPanel->buttonSize = Vector2(120.0f, 60.0f);
		debugPanel->buttonSpacing = Vector2(6.0f, 3.0f);
		debugPanel->fontScale = 1.3f;
		{
			float overalScale = 0.9f;
			debugPanel->buttonSize *= overalScale;
			debugPanel->buttonSpacing *= overalScale;
			debugPanel->fontScale *= overalScale;
		}
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->option_lock(true);
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_option(NAME(dboShowDebugInfo), TXT("show debug info"))
			.add(TXT("none"), 0)
			.add(TXT("stats + log"), 3)
			.add(TXT("fps + stats + log"), 2)
#ifdef AN_DEVELOPMENT
			.add(TXT("full"), 1)
#endif
			.do_not_highlight();
#ifdef AN_DEVELOPMENT
		debugPanel->set_option(NAME(dboShowDebugInfo), 1);
#else
		debugPanel->set_option(NAME(dboShowDebugInfo), 2);
#endif
		debugPanel->add_option(NAME(dboShowVRLogPanel), TXT("show vr log"))
			.add(TXT("no"), 0)
			.add(TXT("move"), 1, Colour::red)
			.add(TXT("lock"), 2, Colour::yellow)
			.do_not_highlight();
#ifdef AN_DEVELOPMENT
		debugPanel->set_option(NAME(dboShowVRLogPanel), 2);
#else
		debugPanel->set_option(NAME(dboShowVRLogPanel), 0);
#endif
#endif

#ifdef AN_PERFORMANCE_MEASURE
		Colour performanceColour = Colour::cyan.mul_rgb(0.3f);
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
		Colour debugCameraColour = Colour::mint.mul_rgb(0.3f);
		Colour debugSubjectColour = Colour::white.mul_rgb(0.3f);
		Colour aiColour = Colour::red.mul_rgb(0.3f);
		Colour soundColour = Colour::purple.mul_rgb(0.3f);
		Colour acColour = Colour::yellow.mul_rgb(0.3f);
		Colour movementColour = Colour::lerp(0.5f, Colour::green, Colour::blue).mul_rgb(0.3f);
		Colour navMeshColour = Colour::orange.mul_rgb(0.3f);
		Colour gameColour = Colour::c64Brown.mul_rgb(0.3f);
		Colour gameColourH = Colour::lerp(0.3f, Colour::c64Brown, Colour::c64Yellow).mul_rgb(0.3f);
#endif
#ifdef AN_DEVELOPMENT
		Colour collisionColour = Colour::blue.mul_rgb(0.3f);
#else
#ifdef AN_DEBUG_RENDERER
		Colour collisionColour = Colour::blue.mul_rgb(0.3f);
#endif
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_option(NAME(dboDebugRenderer), TXT("debug renderer"))
			.add(TXT("off"), DebugRendererMode::Off)
			.add(TXT("only active filters"), DebugRendererMode::OnlyActiveFilters)
			.add(TXT("all"), DebugRendererMode::All)
			.set_on_press([](int _value){ debug_renderer_mode(DebugRendererMode::Type(_value)); })
			.do_not_highlight();
		debugPanel->set_option(NAME(dboDebugRenderer), debug_renderer_get_mode());

		debugPanel->add_option(NAME(dboDebugRendererBlends), TXT("debug renderer"))
			.add(TXT("no blending"), DebugRendererBlending::Off)
			.add(TXT("no triangles"), DebugRendererBlending::NoTriangles)
			.add(TXT("blending"), DebugRendererBlending::On)
			.set_on_press([](int _value) { debug_renderer_blending(DebugRendererBlending::Type(_value)); })
			.do_not_highlight();
		debugPanel->set_option(NAME(dboDebugRendererBlends), debug_renderer_get_blending());

#ifdef AN_DEVELOPMENT_OR_PROFILER
		/*
		debugPanel->add_option(NAME(dboGenerateDevMeshes), TXT("dev meshes"))
			.add(TXT("don't generate"))
			.add(TXT("generate level 1"))
			.add(TXT("generate level 2"))
			.add(TXT("generate level 3"))
			.add(TXT("generate level 4"))
			.do_not_highlight()
			.set_on_press([this](int _value) { generateDevMeshes = _value; });
		debugPanel->set_option(NAME(dboGenerateDevMeshes), generateDevMeshes);
		*/
#endif
#endif

#ifdef AN_PERFORMANCE_MEASURE
		//debugPanel->add_separator();
		debugPanel->add_option(NAME(dboPerformanceMeasure), TXT("performance measure"), performanceColour)
			.add(TXT("off"))
			.add(TXT("on"), NP, Colour::green.mul_rgb(0.3f))
			.set_on_press([](int _value) { Performance::enable(_value != 0); })
			.do_not_highlight();
		debugPanel->set_option(NAME(dboPerformanceMeasure), Performance::is_enabled());

#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_option(NAME(dboPerformanceMeasureShow), TXT("show performance"), performanceColour)
			.add(TXT("off"))
			.add(TXT("on"), NP, Colour::yellow.mul_rgb(0.3f))
			.set_on_press([this](int _value) { performanceLog.clear(); if (_value) { show_performance_in_frames(6); } else { showPerformance = false; noRenderUntilShowPerformance = false; noGameLoopUntilShowPerformance = false; }})
			.do_not_highlight();
	
		debugPanel->add_option(NAME(dboPerformanceMeasureShowNoRender), TXT("show perf (no render)"), performanceColour)
			.add(TXT("off"))
			.add(TXT("on"), NP, Colour::yellow.mul_rgb(0.3f))
			.set_on_press([this](int _value) { performanceLog.clear(); if (_value) { show_performance_in_frames(6); noRenderUntilShowPerformance = true; } else { showPerformance = false; noRenderUntilShowPerformance = false; noGameLoopUntilShowPerformance = false; }})
			.do_not_highlight();
	
		debugPanel->add_option(NAME(dboPerformanceMeasureShowNoGameLoop), TXT("show perf (no game loop)"), performanceColour)
			.add(TXT("off"))
			.add(TXT("on"), NP, Colour::yellow.mul_rgb(0.3f))
			.set_on_press([this](int _value) { performanceLog.clear(); if (_value) { show_performance_in_frames(6); noGameLoopUntilShowPerformance = true; } else { showPerformance = false; noRenderUntilShowPerformance = false; noGameLoopUntilShowPerformance = false; }})
			.do_not_highlight();
#endif

		debugPanel->add_option(NAME(dboLogHitchDetails), TXT("hitch details"), performanceColour)
			.add(TXT("don't log"))
			.add(TXT("log"), NP, Colour::red.mul_rgb(0.3f))
			.set_on_press([this](int _value) { logHitchDetails = _value != 0; });
		debugPanel->set_option(NAME(dboLogHitchDetails), logHitchDetails);

		debugPanel->add_option(NAME(dboStopOnAnyHitch), TXT("stop on any hitch"), performanceColour)
			.add(TXT("off"))
			.add(TXT("on"), NP, Colour::red.mul_rgb(0.3f))
			.set_on_press([this](int _value) { allowToStopToShowPerformanceOnAnyHitch = _value != 0; });
		debugPanel->set_option(NAME(dboStopOnAnyHitch), allowToStopToShowPerformanceOnAnyHitch);

		debugPanel->add_option(NAME(dboStopOnHighHitch), TXT("stop on high hitch"), performanceColour)
			.add(TXT("off"))
			.add(TXT("on"), NP, Colour::red.mul_rgb(0.3f))
			.set_on_press([this](int _value) { allowToStopToShowPerformanceOnHighHitch = _value != 0; });
		debugPanel->set_option(NAME(dboStopOnHighHitch), allowToStopToShowPerformanceOnHighHitch);
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_separator();
#endif
#ifdef AN_DEBUG_RENDERER
		debugPanel->add_option(NAME(dboSelectDebugSubject), TXT("select debug subject mode"), debugSubjectColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugSelectDebugSubject = _value != 0; });
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_option(NAME(dboSelectDebugSubjectNextAI), TXT("next/prev (ai)"), debugSubjectColour)
			.no_options()
			.set_on_press([this](int _value) { select_next_ai_to_debug(_value >= 0); });
		debugPanel->add_option(NAME(dboSelectDebugSubjectNextInRoomAI), TXT("in room debug subject (ai)"), debugSubjectColour)
			.no_options()
			.set_on_press([this](int _value) { select_next_ai_in_room_to_debug(_value >= 0); });
		debugPanel->add_option(NAME(dboSelectDebugSubjectNextInRoom), TXT("in room debug subject (all)"), debugSubjectColour)
			.no_options()
			.set_on_press([this](int _value) { select_next_in_room_to_debug(_value >= 0); });
		debugPanel->add_option(NAME(dboPlayerDebugSubject), TXT("player"), debugSubjectColour)
			.no_options()
			.set_on_press([this](int _value){
#ifdef AN_DEBUG_RENDERER
			DebugRenderer::get()->set_active_subject(fast_cast<Framework::IModulesOwner>(player.get_actor()));
#endif
			debugSelectDebugSubjectHit = player.get_actor(); });
		debugPanel->add_option(NAME(dboPilgrimKeeperSubject), TXT("pilgrim keeper"), debugSubjectColour)
			.no_options()
			.set_on_press([this](int _value)
				{
					if (auto* imo = fast_cast<Framework::IModulesOwner>(player.get_actor()))
					{
						if (auto* p = imo->get_gameplay_as<ModulePilgrim>())
						{
							if (auto* pk = p->get_pilgrim_keeper())
							{
#ifdef AN_DEBUG_RENDERER
								DebugRenderer::get()->set_active_subject(pk);
#endif
								debugSelectDebugSubjectHit = fast_cast<Collision::ICollidableObject>(pk);
							}
						}
					}
				});
		debugPanel->add_option(NAME(dboAIManagerDebugSelect), TXT("ai manager debug subject"), debugSubjectColour)
			.no_options()
			.set_on_press([this](int _value) { select_next_ai_manager_to_debug(_value >= 0); });

		debugPanel->add_option(NAME(dboDynamicSpawnManagerDebugSelect), TXT("dynamic spawn manager debug subject"), debugSubjectColour)
			.no_options()
			.set_on_press([this](int _value) { select_next_dynamic_spawn_manager_to_debug(_value >= 0); });

		debugPanel->add_option(NAME(dboRegionManagerDebugSelect), TXT("region manager debug subject"), debugSubjectColour)
			.no_options()
			.set_on_press([this](int _value) { select_next_region_manager_to_debug(_value >= 0); });

		debugPanel->add_option(NAME(dboGameScriptExecutorDebugSelect), TXT("game script executor debug subject"), debugSubjectColour)
			.no_options()
			.set_on_press([this](int _value) { select_next_game_script_executor_to_debug(_value >= 0); });

		debugPanel->add_option(NAME(dboClearDebugSubject), TXT("clear debug subject"), debugSubjectColour)
			.no_options()
			.set_on_press([this](int _value) {
#ifdef AN_DEBUG_RENDERER
			DebugRenderer::get()->set_active_subject(nullptr);
#endif
			debugSelectDebugSubjectHit = nullptr;  });
#ifdef AN_DEBUG_RENDERER
#ifdef AN_USE_RENDER_CONTEXT_DEBUG
		//debugPanel->add_separator();
		debugPanel->add_option(NAME(dboDontRenderRoomProxyIndexM), TXT("skip/only render room <<")).no_options().set_on_press([this](int _value){
			if (rcDebug.dontRenderRoomProxyIndex >= 0)
			{
				--rcDebug.dontRenderRoomProxyIndex;
			}
		});
		debugPanel->add_option(NAME(dboDontRenderRoomProxyIndexP), TXT("skip/only render room >>")).no_options().set_on_press([this](int _value){
			++rcDebug.dontRenderRoomProxyIndex;
		});
		debugPanel->add_option(NAME(dboDontRenderRoomProxyInverse), TXT("inverse render room"))
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value){
				rcDebug.dontRenderRoomProxyInverse = _value == 1;
			});
#endif

#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_option(NAME(dboDebugCameraMode), TXT("debug camera mode"), debugCameraColour)
			.add(TXT("player"), 0)
			.add(TXT("debug cam"), 1)
			.set_on_press([this](int _value) { debugCameraMode = _value; debugControlsMode = debugCameraMode; });
		debugPanel->add_option(NAME(dboDebugCameraSpeed), TXT("debug camera speed"), debugCameraColour)
			.add(TXT("normal"), 100)
			.add(TXT("very slow"), 25)
			.add(TXT("super slow"), 1)
			.set_on_press([this](int _value) { debugCameraSpeed = (float)_value / 100.0f; });
		debugPanel->add_option(NAME(dboResetDebugCamera), TXT("reset debug camera"), debugCameraColour)
			.no_options()
			.set_on_press([this](int _value) { resetDebugCamera = true; });
#endif
		debugPanel->add_separator();
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_option(NAME(dboInspectMovement), TXT("movement inspect"), movementColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugInspectMovement = _value != 0; });
#endif

#ifdef AN_DEBUG_RENDERER
		debugPanel->add_option(NAME(dboDrawMovement), TXT("movement debug draw"), movementColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value){ DebugRenderer::get()->block_filter(NAME(moduleMovementDebug), !_value); });
	
		debugPanel->add_option(NAME(dboDrawMovementPath), TXT("movement path"), movementColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(moduleMovementPath), !_value); });
	
		debugPanel->add_option(NAME(dboDrawMovementSwitch), TXT("movement switch paths"), movementColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(moduleMovementSwitch), !_value); });

		//debugPanel->add_separator();
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_DEBUG_RENDERER
		debugPanel->add_option(NAME(dboDrawCollisionGradient), TXT("collision gradient"), collisionColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugDrawCollisionGradient = _value; DebugRenderer::get()->block_filter(NAME(moduleCollisionUpdateGradient), !_value); });
#endif
#endif

#ifdef AN_DEBUG_RENDERER
		debugPanel->add_option(NAME(dboDrawCollisionQuerries), TXT("collision querries"), collisionColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(moduleCollisionQuerries), !_value); });

		debugPanel->add_option(NAME(dboDrawCollisionCheckIfColliding), TXT("check if colliding"), collisionColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(moduleCollisionCheckIfColliding), !_value); });
	
		debugPanel->add_option(NAME(dboDrawCollision), TXT("collision"), collisionColour)
			.add(TXT("off"))
			.add(TXT("objects only, skip player"))
			.add(TXT("objects only"))
			.add(TXT("all"))
			.set_on_press([this](int _value){ debugDrawCollision = _value; });
		debugPanel->set_option(NAME(dboDrawCollision), debugDrawCollision);
		debugPanel->add_option(NAME(dboDrawCollisionMode), TXT("collision mode"), collisionColour)
			.add(TXT("not set"))
			.add(TXT("movement"))
			.add(TXT("precise"))
			.set_on_press([this](int _value){ debugDrawCollisionMode = _value; })
			.do_not_highlight();
		debugPanel->set_option(NAME(dboDrawCollisionMode), debugDrawCollisionMode);
		debugPanel->add_option(NAME(dboTestCollisions), TXT("test collisions"), collisionColour)
			.add(TXT("off"))
			.add(TXT("movement"))
			.add(TXT("precise"))
			.set_on_press([this](int _value){
				if (debugDrawCollision && debugDrawCollisionMode != 0)
				{
					if (debugTestCollisions == 0)
					{
						debugTestCollisions = debugDrawCollisionMode;
					}
					else
					{
						debugTestCollisions = 0;
					}
				}
				else
				{
					debugTestCollisions = (debugTestCollisions + 1) % 3;
				}
				debugPanel->set_option(NAME(dboTestCollisions), debugTestCollisions);
			});

#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_separator();
		debugPanel->add_option(NAME(dboLogRoomVariables), TXT("room vars"))
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugLogRoomVariables = _value == 1; });
		debugPanel->add_option(NAME(dboRoomInfo), TXT("room info"))
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugRoomInfo = _value == 1; });
		debugPanel->add_option(NAME(dboDrawDoors), TXT("doors info"))
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugDrawDoors = _value == 1; });
#endif
#ifdef AN_DEBUG_RENDERER
		debugPanel->add_option(NAME(dboDrawVolumetricLimit), TXT("volumetric limit"))
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugDrawVolumetricLimit = _value == 1; });
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_option(NAME(dboDrawSkeletons), TXT("skeletons"))
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugDrawSkeletons = _value == 1; });
#endif
#ifdef AN_DEBUG_RENDERER
		debugPanel->add_option(NAME(dboDrawSockets), TXT("sockets"))
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugDrawSockets = _value == 1; });
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_option(NAME(dboDrawPOIs), TXT("POIs"))
			.add(TXT("off"))
			.add(TXT("draw"))
			.add(TXT("draw + RAO"))
			.add(TXT("draw + log"))
			.set_on_press([this](int _value) { debugDrawPOIs = _value; });
#endif
#ifdef AN_DEBUG_RENDERER
		debugPanel->add_option(NAME(dboDrawMeshNodes), TXT("mesh nodes"))
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugDrawMeshNodes = _value == 1; });
		debugPanel->add_option(NAME(dboDrawPresence), TXT("presence"))
			.add(TXT("off"))
			.add(TXT("all"))
			.set_on_press([this](int _value) { debugDrawPresence = _value; });
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_option(NAME(dboDrawPresenceBaseInfo), TXT("presence base info"))
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugDrawPresenceBaseInfo = _value; });
#endif
#ifdef AN_DEBUG_RENDERER
		debugPanel->add_option(NAME(dboLogAllPresences), TXT("log all presences"))
			.no_options()
			.set_on_press([this](int _value)
			{
				if (player.get_actor())
				{
					LogInfoContext logInfoContext;
					logInfoContext.log(TXT("all presences in room"));
					{
						logInfoContext.log(TXT("objects"));
						LOG_INDENT(logInfoContext);
						//FOR_EVERY_OBJECT_IN_ROOM(object, player.get_actor()->get_presence()->get_in_room())
						for_every_ptr(object, player.get_actor()->get_presence()->get_in_room()->get_objects())
						{
							logInfoContext.log(TXT("%S"), object->get_name().to_char());
							LOG_INDENT(logInfoContext);
							object->get_presence()->log(logInfoContext);
						}
					}
					{
						logInfoContext.log(TXT("presence links"));
						LOG_INDENT(logInfoContext);
						auto* pl = player.get_actor()->get_presence()->get_in_room()->get_presence_links();
						while (pl)
						{
							logInfoContext.log(TXT("%S"), pl->get_modules_owner()->ai_get_name().to_char());
							LOG_INDENT(logInfoContext);
							pl->get_modules_owner()->get_presence()->log(logInfoContext);
							pl->log_development_info(logInfoContext);
							pl = pl->get_next_in_room();
						}
					}
					logInfoContext.output_to_output();
				}
			});
		debugPanel->add_option(NAME(dboDrawSpaceBlockers), TXT("space blockers"))
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugDrawSpaceBlockers = _value; });
		debugPanel->add_option(NAME(dboRoomGenerationInfo), TXT("room generation info"))
			.add(TXT("off"))
			.add(TXT("play area (rgi)"))
			.add(TXT("room generation tiles zone"))
			.set_on_press([this](int _value) { debugDrawRGI = _value; });
		debugPanel->add_option(NAME(dboVRAnchor), TXT("vr anchor"))
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugDrawVRAnchor = _value; });
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_separator();
		debugPanel->add_option(NAME(dboVariables), TXT("variables"), aiColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugVariables = _value; });
		debugPanel->add_option(NAME(dboAICommonVariables), TXT("ai common vars"), aiColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugAICommonVariables = _value; });
		debugPanel->add_option(NAME(dboAILatent), TXT("ai latent"), aiColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugAILatent = _value; });
#endif
#ifdef AN_DEBUG_RENDERER
		debugPanel->add_option(NAME(dboAIDraw), TXT("ai draw"), aiColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(ai_draw), !_value); });
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_DEBUG_RENDERER
		debugPanel->add_option(NAME(dboAIEnemyPerception), TXT("ai enemy perception"), aiColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(ai_enemyPerception), !_value); });
#endif
		debugPanel->add_option(NAME(dboAILog), TXT("ai log"), aiColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugAILog = _value; });
		debugPanel->add_option(NAME(dboAILogic), TXT("ai logic info"), aiColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugAILogic = _value; });
		debugPanel->add_option(NAME(dboAIHunches), TXT("ai hunches"), aiColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugAIHunches = _value; });
		debugPanel->add_option(NAME(dboAIState), TXT("ai state"), aiColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugAIState = _value; });
		debugPanel->add_option(NAME(dboLocomotion), TXT("locomotion"), aiColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) {
#ifdef AN_DEBUG_RENDERER
			DebugRenderer::get()->block_filter(NAME(locomotion), !_value);
#endif
			debugLocomotion = _value; });
#endif
#ifdef AN_DEBUG_RENDERER
		debugPanel->add_option(NAME(dboLocomotionPath), TXT("loco path"), aiColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { DebugRenderer::get()->block_filter(NAME(locomotionPath), !_value); debugLocomotionPath = _value; });
		debugPanel->add_separator();
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_option(NAME(dboNavTasks), TXT("nav tasks"), navMeshColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugLogNavTasks = _value; });
#endif
#ifdef AN_DEBUG_RENDERER
		debugPanel->add_option(NAME(dboDrawNavMesh), TXT("nav mesh"), navMeshColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(navMesh), !_value); });
		debugPanel->add_option(NAME(dboFindLocationOnNavMesh), TXT("find loc on nav mesh"), navMeshColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugFindLocationOnNavMesh = _value; });
		debugPanel->add_option(NAME(dboFindPathOnNavMesh), TXT("find path on nav mesh"), navMeshColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugFindPathOnNavMesh = _value; });
		debugPanel->add_separator();
		debugPanel->add_option(NAME(dboAIEnergyKiosk), TXT("energy kiosk"), aiColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(ai_energyKiosk), !_value); });
		debugPanel->add_option(NAME(dboAIElevatorCart), TXT("elevator cart"), aiColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(ai_elevatorCart), !_value); });
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_separator();
		debugPanel->add_option(NAME(dboListACs), TXT("ac list"), acColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugACList = _value; });
#endif
#ifdef AN_DEBUG_RENDERER
		// alphabetical order!
		debugPanel->add_option(NAME(dboACAlignAxes), TXT("(ac) align axes"), acColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(ac_alignAxes), !_value); });
		debugPanel->add_option(NAME(dboACAlignmentRoll), TXT("(ac) alignment roll"), acColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(ac_alignmentRoll), !_value); });
		debugPanel->add_option(NAME(dboACDragBone), TXT("(ac) drag bone"), acColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(ac_dragBone), !_value); });
		debugPanel->add_option(NAME(dboACElevatorCartRotatingCore), TXT("(ac) elevator cart rotating core"), acColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(ac_elevatorCartRotatingCore), !_value); });
		debugPanel->add_option(NAME(dboACElevatorCartAttacherArm), TXT("(ac) elevator cart attacher arm"), acColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(ac_elevatorCartAttacherArm), !_value); });
		debugPanel->add_option(NAME(dboACFootAdjuster), TXT("(ac) foot adjuster"), acColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(ac_footAdjuster), !_value); });
		debugPanel->add_option(NAME(dboACGyro), TXT("(ac) gyro"), acColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(ac_gyro), !_value); });
		debugPanel->add_option(NAME(dboACReadyHumanoidArm), TXT("(ac) humanoid arm"), acColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(ac_readyHumanoidArm), !_value); });
		debugPanel->add_option(NAME(dboACReadyHumanoidSpine), TXT("(ac) humanoid spine"), acColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(ac_readyHumanoidSpine), !_value); });
		debugPanel->add_option(NAME(dboACIKSolver2), TXT("(ac) ik solver 2"), acColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value){ DebugRenderer::get()->block_filter(NAME(ac_ikSolver2), !_value); });
		debugPanel->add_option(NAME(dboACIKSolver3), TXT("(ac) ik solver 3"), acColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value){ DebugRenderer::get()->block_filter(NAME(ac_ikSolver3), !_value); });
		debugPanel->add_option(NAME(dboACRotateTowards), TXT("(ac) rotate towards"), acColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(ac_rotateTowards), !_value); });
		debugPanel->add_option(NAME(dboACTentacle), TXT("(ac) tentacle"), acColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(ac_tentacle), !_value); });
		debugPanel->add_option(NAME(dboACWalkers), TXT("(ac) walkers"), acColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(ac_walkers), !_value); });
		debugPanel->add_option(NAME(dboACWalkersHit), TXT("(ac) walkers (hit)"), acColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(ac_walkers_hitLocations), !_value); });

		// particle

		debugPanel->add_option(NAME(dboAPCAirTraffic), TXT("(apc) air traffic"), acColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(apc_airTraffic), !_value); });

#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_separator();
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_option(NAME(dboMusicPlayer), TXT("music player (output)"), soundColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugOutputMusicPlayer = _value == 1; });
		debugPanel->add_option(NAME(dboEnvironmentPlayer), TXT("environment player (output)"), soundColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugOutputEnvironmentPlayer = _value == 1; });
		debugPanel->add_option(NAME(dboVoiceoverSystem), TXT("voiceover system (output)"), soundColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugOutputVoiceoverSystem = _value == 1; });
		debugPanel->add_option(NAME(dboSubtitleSystem), TXT("subtitle system (output)"), soundColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugOutputSubtitleSystem = _value == 1; });
		debugPanel->add_option(NAME(dboSoundSystem), TXT("sound system"), soundColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugOutputSoundSystem = _value; });
		debugPanel->add_option(NAME(dboSoundSceneOutput), TXT("sound scene (output)"), soundColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugOutputSoundScene = _value; });
#endif
#ifdef AN_ALLOW_EXTENDED_DEBUG
		debugPanel->add_option(NAME(dboSoundMissingSounds), TXT("missing sound samples"), soundColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { if (_value) { ExtendedDebug::do_debug(NAME(MissingSounds)); } else { ExtendedDebug::dont_debug(NAME(MissingSounds)); } });
		debugPanel->add_option(NAME(dboSoundScene), TXT("sound scene (log)"), soundColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { if (_value) { ExtendedDebug::do_debug(NAME(SoundScene)); } else { ExtendedDebug::dont_debug(NAME(SoundScene)); } });
		debugPanel->add_option(NAME(dboSoundSceneLogEveryFrame), TXT("sound scene (log every frame)"), soundColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { if (_value) { ExtendedDebug::do_debug(NAME(SoundSceneLogEveryFrame)); } else { ExtendedDebug::dont_debug(NAME(SoundSceneLogEveryFrame)); } });
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_option(NAME(dboSoundSceneShowSounds), TXT("sound scene, show sounds"), soundColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(soundSceneShowSounds), !_value); });
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_DEBUG_RENDERER
		debugPanel->add_option(NAME(dboSoundSceneFindSounds), TXT("sound scene, all sounds"), soundColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(soundSceneFindSounds), !_value); });
#endif
		debugPanel->add_option(NAME(dboSoundCameraAtNormalCamera), TXT("sound camera"), soundColour)
			.add(TXT("at camera"))
			.add(TXT("dropped"))
			.add(TXT("at player"))
			.set_on_press([this](int _value) { debugSoundCamera = _value; });

		debugPanel->add_separator();
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_USE_FRAME_INFO
		debugPanel->add_option(NAME(dboRenderDetails), TXT("show render details"))
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugOutputRenderDetails = _value; debugOutputRenderDetailsLocked = false; debugPanel->set_option(NAME(dboRenderDetailsLock), debugOutputRenderDetailsLocked); });
		debugPanel->add_option(NAME(dboRenderDetailsLock), TXT("lock render details"))
			.add(TXT("current"))
			.add(TXT("locked"))
			.set_on_press([this](int _value) { if (!debugOutputRenderDetailsLocked) { debugOutputRenderDetailsLocked = true; debugOutputRenderDetailsLockedFrame = *prevFrameInfo; } else { debugOutputRenderDetailsLocked = false; }});
		debugPanel->add_option(NAME(dboRenderDetailsLog), TXT("log render details"))
			.no_options()
			.set_on_press([this](int _value)
			{
				auto const* frameInfo = (debugOutputRenderDetails && debugOutputRenderDetailsLocked ? &debugOutputRenderDetailsLockedFrame : prevFrameInfo);
				LogInfoContext log;
				log.log(TXT("RENDER DETAILS [%05i] - BEGIN"), frameInfo->frameNo);
				log.increase_indent();
				frameInfo->systemInfo.log(log, true);
				log.decrease_indent();
				log.log(TXT("RENDER DETAILS - END"));
				log.output_to_output();
			});
#endif
#endif
		debugPanel->add_option(NAME(dboDrawVRPlayArea), TXT("vr play area"))
			.add(TXT("off"))
			.add(TXT("used play area (vr)"))
			.add(TXT("play area setup (vr)"))
			.set_on_press([this](int _value) { debugDrawVRPlayArea = _value; });
		debugPanel->add_option(NAME(dboDrawVRControls), TXT("vr controls"))
			.add(TXT("off"))
			.add(TXT("raw"))
			//.add(TXT("trackpad?"))
			.set_on_press([this](int _value) { debugDrawVRControls = _value; });
#ifdef AN_RENDERER_STATS
		debugPanel->add_option(NAME(dboRenderStats), TXT("log render stats"))
			.no_options()
			.set_on_press([](int _value) { Framework::Render::Stats::get().log(); });
#endif
		debugPanel->add_option(NAME(dboRenderSceneLog), TXT("log render scene"))
			.no_options()
			.set_on_press([this](int _value)
			{
				LogInfoContext log;
				log.log(TXT("RENDER SCENES"));
				for_every_ref(renderScene, renderScenes)
				{
					LOG_INDENT(log);
					log.log(TXT("RENDER SCENE %i"), for_everys_index(renderScene));
					LOG_INDENT(log);
					renderScene->log(log);
					log.log(TXT(""));
				}
				log.output_to_output();
			});
		debugPanel->add_option(NAME(dboRenderSceneOutput), TXT("render scene (output)"))
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugOutputRenderScene = _value; });
#endif
		debugPanel->add_separator();

#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_option(NAME(dboGameDirector), TXT("game director"), gameColour)
			.add(TXT("off"))
			.add(TXT("log"))
			.set_on_press([this](int _value) { debugOutputGameDirector = _value; });
		debugPanel->add_option(NAME(dboLockerRoom), TXT("locker room"), gameColour)
			.add(TXT("off"))
			.add(TXT("show"))
			.set_on_press([this](int _value) { debugOutputLockerRoom = _value; });
		debugPanel->add_option(NAME(dboHighLevelRegionInfo), TXT("high level region info"), gameColour)
			.add(TXT("off"))
			.add(TXT("show"))
			.set_on_press([this](int _value) { debugHighLevelRegion.outputInfo = _value; });
		debugPanel->add_option(NAME(dboCellLevelRegionInfo), TXT("cell level region info"), gameColour)
			.add(TXT("off"))
			.add(TXT("show"))
			.set_on_press([this](int _value) { debugCellLevelRegion.outputInfo = _value; });
		debugPanel->add_option(NAME(dboRoomRegion), TXT("room+region"), gameColour)
			.add(TXT("off"))
			.add(TXT("show"))
			.set_on_press([this](int _value) { debugRoomRegion.outputInfo = _value; });
		debugPanel->add_option(NAME(dboPilgrimage), TXT("pilgrimage"), gameColour)
			.add(TXT("nothing"))
			.add(TXT("pilgrimage instance"))
			.add(TXT("environments"))
			.set_on_press([this](int _value) { debugOutputPilgrimageInfo = _value; });
		debugPanel->add_option(NAME(dboPilgrim), TXT("pilgrim"), gameColour)
			.add(TXT("off"))
			.add(TXT("log"))
			.set_on_press([this](int _value) { debugOutputPilgrim = _value; });
		debugPanel->add_option(NAME(dboGameplayStuff), TXT("gameplay stuff"), gameColourH)
			.add(TXT("off"))
			.add(TXT("log"))
			.set_on_press([this](int _value) { debugOutputGameplayStuff = _value; });
#endif
#ifdef AN_DEBUG_RENDERER
		debugPanel->add_option(NAME(dboPilgrimGuidance), TXT("(pilgrim) guidance"), gameColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(pilgrim_guidance), !_value); });
		debugPanel->add_option(NAME(dboPilgrimPhysicalViolence), TXT("(pilgrim) physical violence"), gameColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(pilgrim_physicalViolence), !_value); });

		debugPanel->add_option(NAME(dboExplosions), TXT("explosions"), gameColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(explosions), !_value);
		if (_value) { ExtendedDebug::do_debug(NAME(explosions)); }
		else { ExtendedDebug::dont_debug(NAME(explosions)); }});

		debugPanel->add_option(NAME(dboGrabable), TXT("grabable"), gameColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(grabable), !_value); });

		debugPanel->add_option(NAME(dboPilgrimGrab), TXT("pilgrim grab"), gameColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { DebugRenderer::get()->block_filter(NAME(pilgrimGrab), !_value); });
	
	
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_option(NAME(dboLogWorld), TXT("log world"), gameColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugShowWorldLog = _value; if (debugShowWorldLog) { worldLog.clear(); world->log(worldLog); worldLog.output_to_output(true); }});
		debugPanel->add_option(NAME(dboMiniMap), TXT("mini map"), gameColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugShowMiniMap = _value; });
		debugPanel->set_option(NAME(dboMiniMap), debugShowMiniMap);
		debugPanel->add_option(NAME(dboGameJobs), TXT("game jobs"), gameColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([this](int _value) { debugShowGameJobs = _value; });
		debugPanel->set_option(NAME(dboGameJobs), debugShowGameJobs);
#endif
#ifdef AN_DEVELOPMENT
#ifdef AN_ALLOW_EXTENDED_DEBUG
		debugPanel->add_option(NAME(dboMissingTemporaryObjects), TXT("missing temporary objects"), gameColour)
			.add(TXT("off"))
			.add(TXT("on"))
			.set_on_press([](int _value) { if (_value) { ExtendedDebug::do_debug(NAME(MissingTemporaryObjects)); } else { ExtendedDebug::dont_debug(NAME(MissingTemporaryObjects)); } });
#endif
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_NO_RARE_ADVANCE_AVAILABLE
		debugPanel->add_option(NAME(dboRareAdvance), TXT("rare advance"), gameColour)
			.add(TXT("yes - normal"))
			.add(TXT("no - slower"))
			.set_on_press([](int _value) { Framework::access_no_rare_advance() = _value == 1; });
#endif
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_separator();
#endif

#ifdef AN_ALLOW_BULLSHOTS
#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->add_option(NAME(dboBullshotSystemInfo), TXT("bullshot system info "))
			.add(TXT("off"))
			.add(TXT("log"))
			.set_on_press([this](int _value) { debugOutputBullshotSystem = _value; });
		debugPanel->add_option(NAME(dboHardCopyBullshotObject), TXT("bullshot hard copy (selected)"))
			.no_options()
			.set_on_press([this](int) { hard_copy_bullshot(false /*one*/); });
		debugPanel->add_option(NAME(dboHardCopyBullshotObjects), TXT("bullshot hard copy (in rendered rooms)"))
			.no_options()
			.set_on_press([this](int) { hard_copy_bullshot(true /*one*/); });
#endif
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
		debugPanel->option_lock(false);
#endif
	}

	output(TXT("done before game starts"));
}

void Game::after_game_ended()
{
	PhysicalSensations::terminate();

	::System::Core::on_quick_exit_no_longer(TXT("save player profile"));

	// clear any references
	playerSetup.reset();

	renderScenes.clear();
	delete_and_clear(environmentPlayer);
	delete_and_clear(musicPlayer);
	delete_and_clear(voiceoverSystem);
	delete_and_clear(subtitleSystem);
	delete_and_clear(soundScene);
	delete_and_clear(tutorialSystem);
	delete_and_clear(gameDirector);
	delete_and_clear(gameLog);
	delete_and_clear(overlayInfoSystem);
	base::after_game_ended();
}

Game::~Game()
{
	if (debugLogPanel)
	{
		debugLogPanel->show_log(nullptr);
	}
	an_assert(mainHubLoader == nullptr, TXT("call close_hub_loaders earlier, before closeing library"));
	GameSettings::destroy();

#ifdef AN_DEBUG_RENDERER
	if (Framework::Render::ContextDebug::use == &rcDebug)
	{
		Framework::Render::ContextDebug::use = nullptr;
	}
#endif

	// just in any case
	worldManager.allow_immediate_sync_jobs();
	perform_sync_world_job(TXT("destroy world"), [this](){ sync_destroy_world(); });

	an_assert(soundScene == nullptr);
	an_assert(mode == nullptr);
	an_assert(world == nullptr);
}

void Game::initialise_system()
{
#ifdef WITH_VR
	MainConfig::access_global().force_vr();
#endif

	base::initialise_system();
}

void Game::create_render_buffers()
{
	base::create_render_buffers();

	if (allowScreenShots)
	{
#ifdef RENDER_TO_BIG_PANELS
		preferredScreenShotResolution[0] = VectorInt2(3780 / 2, 9410 / 2); // full size is way too much for the graphic card
		preferredScreenShotResolution[1] = VectorInt2(378, 941);
		Performance::enable(false);
#endif
#ifdef BIG_SCREEN_SHOTS
		preferredScreenShotResolution[0] = VectorInt2(3780, 3780);
		preferredScreenShotResolution[1] = VectorInt2(378, 378);
		Performance::enable(false);
#endif
	}
}

void Game::make_sure_screen_shot_custom_render_context_is_valid(int _idx)
{
	if (screenShotCustomRenderContext[_idx].sceneRenderTarget.get() &&
		screenShotCustomRenderContext[_idx].finalRenderTarget.get())
	{
		return;
	}

	if (mod(preferredScreenShotResolution[_idx].x, 4) != 0)
	{
		int proper = preferredScreenShotResolution[_idx].x + 4 - mod(preferredScreenShotResolution[_idx].x, 4);
		preferredScreenShotResolution[_idx].y = (int)((float)preferredScreenShotResolution[_idx].y * (float)proper / (float)preferredScreenShotResolution[_idx].x);
		preferredScreenShotResolution[_idx].x = proper;
	}
	{
		::System::RenderTargetSetup rtSetup = ::System::RenderTargetSetup(
			preferredScreenShotResolution[_idx],
#ifdef AN_ANDROID
			4,
#else
			8, // a what the hell!
#endif
			::System::DepthStencilFormat::defaultFormat);
		rtSetup.copy_output_textures_from(Framework::RenderingPipeline::get_default_output_definition());
		RENDER_TARGET_CREATE_INFO(TXT("screen shot %i, scene"), _idx);
		screenShotCustomRenderContext[_idx].sceneRenderTarget = new ::System::RenderTarget(rtSetup);
		RENDER_TARGET_CREATE_INFO(TXT("screen shot %i, final"), _idx);
		screenShotCustomRenderContext[_idx].finalRenderTarget = new ::System::RenderTarget(rtSetup);
		if (!screenShotCustomRenderContext[_idx].sceneRenderTarget.get() ||
			!screenShotCustomRenderContext[_idx].finalRenderTarget.get())
		{
			error(TXT("screen shots not available"));
		}
	}
}

void Game::close_system()
{
	delete_and_clear(overlayInfoSystem);
	delete_and_clear(gameDirector);
	delete_and_clear(gameLog);

	::System::Core::on_quick_exit_no_longer(TXT("save player profile"));

	for_count(int, i, 2)
	{
		screenShotCustomRenderContext[i].sceneRenderTarget = nullptr;
		screenShotCustomRenderContext[i].finalRenderTarget = nullptr;
	}

	base::close_system();
}

void Game::play_loading_music_via_non_game_music_player(Optional<float> const & _fadeTime)
{
	play_global_reference_music_via_non_game_music_player(NAME(gsLoadingMusic), _fadeTime);
}

void Game::play_global_reference_music_via_non_game_music_player(Name const & _gsRef, Optional<float> const& _fadeTime)
{
	if (nonGameMusicPlayer)
	{
		if (auto* s = Library::get_current()->get_global_references().get<Framework::Sample>(_gsRef, true))
		{
			if (auto* ps = PlayerSetup::access_current_if_exists())
			{
				if (ps->does_allow_non_game_music())
				{
					nonGameMusicPlayer->play(s, _fadeTime);
				}
			}
		}
	}
}

void Game::stop_non_game_music_player(Optional<float> const& _fadeTime)
{
	if (nonGameMusicPlayer)
	{
		if (get_fade_out() >= 0.9f)
		{
			nonGameMusicPlayer->stop(0.0f);
		}
		else
		{
			nonGameMusicPlayer->stop(_fadeTime.get(0.5f));
		}
	}
}

void Game::set_meta_state(GameMetaState::Type _metaState)
{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("set meta state to %i (from %i)"), _metaState, metaState);
#endif
	auto prevMetaState = metaState;
	metaState = _metaState;

	// to keep us close to 0.0
	::System::Core::reset_timer();

	overlayInfoSystem->force_clear();

	reset_camera_rumble();

	// reset
	clear_forced_vignette_for_movement();
	clear_vignette_for_dead_target();
	clear_scripted_tint();

	// music players
	{
		// stop all music and voiceovers on any meta state change
		{
			if (musicPlayer)
			{
				musicPlayer->reset();
				// reset to default state -> ambient
				if (metaState == GameMetaState::Pilgrimage ||
					metaState == GameMetaState::Tutorial)
				{
					musicPlayer->block_requests(false);
					musicPlayer->request_ambient();
				}
			}
			if (voiceoverSystem)
			{
				voiceoverSystem->silence(0.1f);
				voiceoverSystem->reset();
			}
			if (subtitleSystem)
			{
				subtitleSystem->remove_all();
				subtitleSystem->clear_user_log();
			}
			if (auto* ss = ::System::Sound::get())
			{
				ss->clear_custom_volumes();
			}
			// do not reset game director here as we may still want to create a game state and resetting will throw away story facts
			// game director should be reset before pilgrimage starts
		}

		if (poiManager)
		{
			poiManager->reset();
		}

		// meta state music player
		if (nonGameMusicPlayer)
		{
			if (metaState == GameMetaState::Pilgrimage ||
				metaState == GameMetaState::Tutorial)
			{
				// they should stop non-game music on their own
			}
			else if (metaState == GameMetaState::Debriefing ||
					 metaState == GameMetaState::TutorialDebriefing)
			{
				stop_non_game_music_player();
			}
			else if ((metaState >= GameMetaState::SelectPlayerProfile &&
					  metaState <= GameMetaState::QuickPilgrimageSetup) ||
					  metaState <= GameMetaState::TutorialSelection)
			{
				play_global_reference_music_via_non_game_music_player(NAME(gsMenuThemeMusic));
			}
			else
			{
				play_global_reference_music_via_non_game_music_player(NAME(gsMenuMusic));
			}
		}
	}

	if (prevMetaState == GameMetaState::Pilgrimage)
	{
		gameLog->close();

		post_pilgrimage_ends();
	}
	if (metaState == GameMetaState::Pilgrimage)
	{
		// reset stats here as we might want to do run revert and we should have clean stats
		GameStats::use_pilgrimage();
		GameStats::get().reset();

		gameLog->clear_and_open();
		PlayerSetup::access_current().stats__run();
		consumableRunTime = 0.0f;
		System::Core::on_advance(NAME(advanceRunTimer), [this]()
		{
			if (!blockRunTime)
			{
				float deltaTime = System::Core::get_delta_time();
				consumableRunTime += deltaTime;
				PlayerSetup::access_current().stats__increase_run_time(deltaTime);
			}
		});
	}
	else
	{
		if (prevMetaState == GameMetaState::Pilgrimage)
		{
			// we started when we
			if (GameStats::get().timeInSeconds < 10)
			{
				PlayerSetup::access_current().stats__run_revert();
			}
		}

		System::Core::on_advance_no_longer(NAME(advanceRunTimer));
		System::Core::on_advance_no_longer(NAME(advanceActiveRunTimer));
		System::Core::on_advance_no_longer(NAME(advanceRealWorldRunTimer));
	}
	if (metaState == GameMetaState::TutorialSelection)
	{
		if (prevMetaState != GameMetaState::Tutorial &&
			prevMetaState != GameMetaState::TutorialDebriefing)
		{
			store_post_tutorial_selection_meta_state(prevMetaState);
		};
	}
	if (prevMetaState == GameMetaState::Tutorial)
	{
		TutorialSystem::get()->set_game_in_tutorial_mode(false);
	}
	if (metaState == GameMetaState::Tutorial)
	{
		TutorialSystem::get()->set_game_in_tutorial_mode(true);
		if (prevMetaState != GameMetaState::Tutorial &&
			prevMetaState != GameMetaState::TutorialDebriefing)
		{
			// this can be tutorial selection
			store_post_tutorial_meta_state(prevMetaState);
		};
	}
	// for time being make it the same
	if (metaState != GameMetaState::JustStarted &&
		metaState != GameMetaState::SelectPlayerProfile &&
		metaState != GameMetaState::SetupNewPlayerProfile &&
		metaState != GameMetaState::SelectGameSlot &&
		metaState != GameMetaState::SetupNewGameSlot)
	{
		System::Core::on_advance(NAME(advanceProfileTimer), []()
		{
			float deltaTime = System::Core::get_delta_time();
			PlayerSetup::access_current().stats__increase_profile_time(deltaTime);
		});
	}
	else
	{
		System::Core::on_advance_no_longer(NAME(advanceProfileTimer));
	}
	if (metaState != GameMetaState::JustStarted &&
		metaState != GameMetaState::SelectPlayerProfile &&
		metaState != GameMetaState::SetupNewPlayerProfile &&
		metaState != GameMetaState::SelectGameSlot &&
		metaState != GameMetaState::SetupNewGameSlot)
	{
		PlayerSetup::stats__store_game_slot_stats(true);
	}
	else
	{
		PlayerSetup::stats__store_game_slot_stats(false);
	}
}

bool Game::is_post_game_summary_requested() const
{
	return showPostGameSummary != PostGameSummary::None;
}

void Game::request_post_game_summary_if_none(PostGameSummary::Type _postGameSummary)
{
	if (showPostGameSummary == PostGameSummary::None)
	{
		request_post_game_summary(_postGameSummary);
	}
}

void Game::update_game_stats()
{
	auto& gameStats = GameStats::get();
	Concurrency::ScopedSpinLock lock(gameStats.access_lock());
	{
		int runTimeSeconds = consume_run_time_seconds();
		if (runTimeSeconds > 0)
		{
			gameStats.timeInSeconds += runTimeSeconds;
			if (auto* p = Persistence::access_current_if_exists())
			{
				Concurrency::ScopedSpinLock lock(p->access_lock(), true);
				GameDefinition::advance_loot_progress_level(p->access_progress(), runTimeSeconds);
			}
		}
	}
	if (auto* p = player.get_actor())
	{
		if (auto* ai = p->get_ai())
		{
			if (auto* mind = ai->get_mind())
			{
				if (auto* pilgrim = fast_cast<AI::Logics::Pilgrim>(mind->get_logic()))
				{
					float dist = pilgrim->consume_dist();
					gameStats.distance += dist;
#ifdef AN_ALLOW_EXTENSIVE_LOGS
					if (dist != 0.0f)
					{
						output(TXT("store distance for update stats +%.3f -> %.3f"), dist, gameStats.distance);
					}
#endif
				}
			}
		}
	}
}

void Game::on_ready_for_post_game_summary()
{
	update_game_stats();
}

void Game::request_post_game_summary(PostGameSummary::Type _postGameSummary)
{
	auto prevPostGameSummary = showPostGameSummary;
	showPostGameSummary = _postGameSummary;
	if (_postGameSummary != PostGameSummary::None && prevPostGameSummary == PostGameSummary::None)
	{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
		output(TXT("request_post_game_summary now"));
#endif

		// store stats
		on_ready_for_post_game_summary();
		if (_postGameSummary == PostGameSummary::Died)
		{
			PlayerSetup::access_current().stats__death();
			++GameStats::get().deaths;
		}
		if (_postGameSummary == PostGameSummary::ReachedEnd ||
			_postGameSummary == PostGameSummary::ReachedDemoEnd)
		{
			Loader::HubScenes::PilgrimageSetup::mark_reached_end();
			++GameStats::get().finished;
		}

		if (auto* gs = PlayerSetup::access_current().access_active_game_slot())
		{
			if (_postGameSummary == PostGameSummary::Died)
			{
				if (GameSettings::get().difficulty.restartMode <= GameSettings::RestartMode::RespawnAndContinue &&
					! GameDirector::get()->is_respawn_and_continue_blocked())
				{
					bool goodToContinue = true;
					if (auto* p = player.get_actor())
					{
						// only if we have player, if not, fail
						if (GameSettings::get().difficulty.respawnAndContinueOnlyIfEnoughEnergy)
						{
							if (auto* h = p->get_custom<CustomModules::Health>())
							{
								Energy maxHealth = h->get_max_health(); // not total (!)
								if (auto* mp = p->get_gameplay_as<ModulePilgrim>())
								{
									mp->redistribute_energy_to_health(maxHealth, true, false);
								}
								if (h->get_health() < Energy::one())
								{
									output(TXT("couldn't respawn and continue due to too little energy available"));
									goodToContinue = false;
								}
							}
						}
						else
						{
							if (GameSettings::get().difficulty.healthOnContinue > 0)
							{
								if (auto* mp = p->get_gameplay_as<ModulePilgrim>())
								{
									mp->redistribute_energy_to_health(Energy(GameSettings::get().difficulty.healthOnContinue), true, true);
								}
							}
							else
							{
								if (auto* h = p->get_custom<CustomModules::Health>())
								{
									Energy e = h->get_max_total_health();
									h->reset_energy_state(e);
								}
								if (auto* mp = p->get_gameplay_as<ModulePilgrim>())
								{
									Energy le = mp->get_hand_energy_storage(Hand::Left);
									Energy re = mp->get_hand_energy_storage(Hand::Right);
									Energy leMax = mp->get_hand_energy_max_storage(Hand::Left);
									Energy reMax = mp->get_hand_energy_max_storage(Hand::Right);
									le = max(le, leMax.mul(0.5f));
									re = max(re, reMax.mul(0.5f));
									mp->set_hand_energy_storage(Hand::Left, le);
									mp->set_hand_energy_storage(Hand::Right, re);
								}
							}
						}
						if (goodToContinue)
						{
							showPostGameSummary = PostGameSummary::None; // don't show it, just get back to the game
							_postGameSummary = PostGameSummary::None;

							auto* presence = p->get_presence();
							Transform shouldBeAt = presence->get_vr_anchor().to_local(presence->get_placement().to_world(presence->get_eyes_relative_look()));
							bool supressOverlayInfoSystem = true;
							bool advanceRunTimerDuringMenu = true;

							// enter
							{
								if (supressOverlayInfoSystem)
								{
									TutorialSystem::get()->pause_tutorial();
									overlayInfoSystem->supress();
								}
								if (advanceRunTimerDuringMenu)
								{
									System::Core::allow_on_advance(NAME(advanceRunTimer)); // opposite, we know we'll be showing a menu, we want to allow advancement then
								}
							}

							Colour backgroundColour = RegisteredColours::get_colour(NAME(loader_hub_fade));
							// show 3,2,1 loader
							{
								stop_non_game_music_player();

								Loader::Countdown cd(1, 3);
								cd.backgroundColour = backgroundColour;
								show_loader(&cd);
							}

							// show barp scene (if required)
							if (!is_using_sliding_locomotion())
							{
								Loader::HubScenes::BeAtRightPlace::be_at(shouldBeAt, shouldBeAt.get_translation().z < 0.2f);
								Loader::HubScenes::BeAtRightPlace::set_max_dist();
								Loader::Hub* usingHubLoader = get_main_hub_loader();
								if (Loader::HubScenes::BeAtRightPlace::is_required(usingHubLoader))
								{
									RefCountObjectPtr<Loader::HubScene> keepInWorldScene;
									{
										auto* barp = new Loader::HubScenes::BeAtRightPlace(false /* should deactivate when hit */);
										barp->allow_loading_tips(false);
										barp->ignore_rotation();
										barp->wait_for_be_at();
										barp->set_info_message(LOC_STR(NAME(lsMoveToValidPosition)));
										keepInWorldScene = barp;
									}
									//play_global_reference_music_via_non_game_music_player(NAME(gsMenuMusic));
									show_hub_scene(keepInWorldScene.get(), usingHubLoader);
									Loader::HubScenes::BeAtRightPlace::set_max_dist();
									stop_non_game_music_player();
								}
							}

							// exit
							{
								if (supressOverlayInfoSystem)
								{
									overlayInfoSystem->resume();
									TutorialSystem::get()->unpause_tutorial();
								}
								if (advanceRunTimerDuringMenu)
								{
									System::Core::block_on_advance(NAME(advanceRunTimer)); // block back
								}

								{
									// fade out immediately, to fade in
									Colour fadeColour = backgroundColour;
									fade_out_immediately(fadeColour);
								}
								{
									fade_in(0.5f, NAME(resurrect));
								}
							}

							if (auto* gd = GameDirector::get())
							{
								// give some time to run away?
								// should be enough to attack them back or for them to move away
								gd->game_force_no_violence_for(2.0f);
							}
						}
					}
					if (goodToContinue)
					{
						if (musicPlayer)
						{
							musicPlayer->request_none();
							musicPlayer->request_combat_auto_stop();
							// restart music
							musicPlayer->block_requests(false);
							musicPlayer->request_ambient();
						}
					}
				}
				if (_postGameSummary == PostGameSummary::Died)
				{
					output(TXT("process 'died'"));
					gs->startUsingGameState.clear();
					if (GameSettings::get().difficulty.restartMode > GameSettings::RestartMode::AnyCheckpoint)
					{
						output(TXT("remove game states 'last, checkpoint, half-health'"));
						gs->lastMoment.clear();
						gs->checkpoint.clear();
						gs->atLeastHalfHealth.clear();
						if (auto* ms = MissionState::get_current())
						{
							ms->mark_died();
						}
					}
					if (GameSettings::get().difficulty.restartMode > GameSettings::RestartMode::ReachedChapter)
					{
						output(TXT("remove game state 'chapter start'"));
						gs->chapterStart.clear();
					}
					if (GameSettings::get().difficulty.restartMode == GameSettings::RestartMode::UnlockedChapter)
					{
						output(TXT("mange unlockable chapters"));
						gs->store_played_recently_pilgrimages_as_unlockables();
						gs->clear_played_recently_pilgrimages();
					}
					if (GameSettings::get().difficulty.restartMode >= GameSettings::RestartMode::UnlockedChapter)
					{
						if (MissionState::get_current())
						{
							output(TXT("abort current mission"));
							add_async_world_job_top_priority(TXT("abort mission"),
								[]()
								{
									MissionState::async_abort(false); // don't save, we will save in a bit
								});
						}
					}
				}
				add_async_save_player_profile();
			}
			if (_postGameSummary == PostGameSummary::Interrupted)
			{
				bool storingGameStateAllowed = true;
				if (auto* pi = PilgrimageInstance::get())
				{
					if (auto* p = pi->get_pilgrimage())
					{
						if (!p->does_allow_store_intermediate_game_state())
						{
							storingGameStateAllowed = false;
						}
					}
				}
				if (storingGameStateAllowed)
				{
					output(TXT("store game state - interrupted"));
					add_async_store_game_state(true, GameStateLevel::LastMoment);
				}
			}
		}
	}
}

void Game::before_pilgrimage_setup()
{
	an_assert(Persistence::access_current_if_exists());
}

void Game::post_pilgrimage_setup()
{
	// restore to non canceling
	set_wants_to_cancel_creation(false); // we want to create since now
}

void Game::before_pilgrimage_starts()
{
	if (auto* p = Persistence::access_current_if_exists())
	{
		p->start_pilgrimage();
	}
	
	{
		if (poiManager)
		{
			poiManager->reset();
		}
		if (auto* gd = GameDirector::get())
		{
			gd->reset(); // reset it only before it start, we may need it for saving
		}
		if (musicPlayer)
		{
			musicPlayer->reset();
			musicPlayer->block_requests(false);
			musicPlayer->request_ambient();
		}
		if (auto* s = SubtitleSystem::get())
		{
			s->clear_user_log();
		}
		if (auto* v = VoiceoverSystem::get())
		{
			v->reset();
		}
	}

	set_wants_to_cancel_creation(false); // we want to create since now
}

void Game::before_pilgrimage_starts_ready()
{
	// increase usage before we create a pilgrim
	PlayerSetup::access_current().access_pilgrim_setup().increase_weapon_part_usage();

	if (auto* ms = MissionState::get_current())
	{
		ms->prepare_for_gameplay();
	}
}

void Game::post_pilgrimage_ends()
{
	Persistence::access_current().end_pilgrimage();
}

void Game::add_async_store_persistence(Optional<bool> const& _savePlayerProfile)
{
#ifdef AN_ALLOW_AUTO_TEST
	if (is_auto_test_active())
	{
		return;
	}
#endif
	if (Framework::TestConfig::get_params().get_value<Framework::LibraryName>(NAME(testRoom), Framework::LibraryName()).is_valid())
	{
		return;
	}

	an_assert(metaState == GameMetaState::Pilgrimage);
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("requested store persistence"));
#endif
	RefCountObjectPtr<PilgrimageInstance> keepPilgrimageInstance(PilgrimageInstance::get()); // we need to keep it here, as in the mean time the mode may end and the mode holds the instance
	add_async_world_job_top_priority(TXT("store persistence"),
		[this, _savePlayerProfile, keepPilgrimageInstance]()
		{
#ifdef WITH_SAVING_ICON
			if (PlayerPreferences::should_show_saving_icon() &&
				_savePlayerProfile.get(true) &&
				!useQuickGameProfile)
			{
				gameIcons.savingIcon.activate();
			}
#endif

			update_game_stats();

			if (_savePlayerProfile.get(true))
			{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
				output(TXT("save player profile with a new game slot"));
#endif
				save_player_profile();
			}
		});

}

void Game::add_async_store_game_state(Optional<bool> const& _savePlayerProfile, Optional<GameStateLevel::Type> const& _gameStateLevel, Optional<StoreGameStateParams> const& _params)
{
#ifdef AN_ALLOW_AUTO_TEST
	if (is_auto_test_active())
	{
		return;
	}
#endif
	if (Framework::TestConfig::get_params().get_value<Framework::LibraryName>(NAME(testRoom), Framework::LibraryName()).is_valid())
	{
		return;
	}
	StoreGameStateParams params = _params.get(StoreGameStateParams());

	an_assert(metaState == GameMetaState::Pilgrimage);
	GameStateLevel::Type gameStateLevel = _gameStateLevel.get(GameStateLevel::LastMoment);
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("requested create new game state [gameStateLevel %i]"), gameStateLevel);
#endif
	RefCountObjectPtr<PilgrimageInstance> keepPilgrimageInstance(PilgrimageInstance::get()); // we need to keep it here, as in the mean time the mode may end and the mode holds the instance
	add_async_world_job_top_priority(TXT("store game state"),
		[this, _savePlayerProfile, gameStateLevel, keepPilgrimageInstance, params]()
		{
			output(TXT("create game state"));
			::System::TimeStamp createGameStateTS;

#ifdef WITH_SAVING_ICON
			if (PlayerPreferences::should_show_saving_icon() &&
				_savePlayerProfile.get(true) &&
				!useQuickGameProfile)
			{
				gameIcons.savingIcon.activate();
			}
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
			output(TXT("create new game state [gameStateLevel %i]"), gameStateLevel);
#endif

			update_game_stats();

			GameState* gameState = new GameState();

			perform_sync_world_job(TXT("suspending destruction"), [this]()
				{
					suspend_destruction(NAME(storingGameState));
				});

			{
				::System::TimeStamp subTS;
				perform_sync_world_job(TXT("store game state basic info"), [&gameState, params]()
					{
						::System::TimeStamp subTS;
						gameState->when = Framework::DateTime::get_current_from_system();
						gameState->interrupted = params.wasInterrupted;
						gameState->gameStats = GameStats::get();
						if (auto* gs = PlayerSetup::access_current().access_active_game_slot())
						{
							gameState->gameDefinition = gs->gameDefinition.get();
							gameState->gameModifiers = gs->runSetup.activeGameModifiers;
						}
						if (auto* g = Game::get_as<Game>())
						{
							if (auto* pa = g->access_player().get_actor())
							{
								gameState->pilgrimActorType = pa->get_actor_type();
							}
						}
						if (auto* pi = PilgrimageInstance::get())
						{
							gameState->pilgrimage = pi->get_pilgrimage();
							an_assert(gameState->pilgrimage.get());
						}
						else
						{
							an_assert(false, TXT("what about keeping pilgrimage instance"));
						}
						if (auto* gd = GameDirector::get())
						{
							Concurrency::ScopedMRSWLockRead lock(gd->access_story_execution().access_facts_lock());
							gameState->storyFacts.set_tags_from(gd->access_story_execution().get_facts());
						}
						output(TXT("store game state basic info (sync) %.3f"), subTS.get_time_since());
					});
				output(TXT("store game state basic info %.3f"), subTS.get_time_since());
			}

			if (auto* pi = PilgrimageInstance::get())
			{
				::System::TimeStamp subTS;
				pi->store_game_state(gameState, params);
				output(TXT("store game state for pilgrimage instance %.3fms"), subTS.get_time_since());
			}

			perform_sync_world_job(TXT("store game state as last checkpoint"), [&gameState, gameStateLevel]()
				{
					::System::TimeStamp subTS;
					if (auto* gs = PlayerSetup::access_current().access_active_game_slot())
					{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
						output(TXT("store as last checkpoint"));
#endif
						gs->startUsingGameState = gameState;
						gs->lastMoment = gameState;
						if (GameSettings::get().difficulty.restartMode <= GameSettings::RestartMode::AnyCheckpoint)
						{
							if (gameStateLevel >= GameStateLevel::Checkpoint)
							{
								if (gameState->atLeastHalfHealth)
								{
									gs->atLeastHalfHealth = gameState;
								}
								gs->checkpoint = gameState;
							}
						}
						else
						{
							gs->atLeastHalfHealth.clear();
							gs->checkpoint.clear();
						}
						if (gameStateLevel >= GameStateLevel::ChapterStart &&
							GameSettings::get().difficulty.restartMode <= GameSettings::RestartMode::ReachedChapter)
						{
							output(TXT("store as chapter"));
							gs->chapterStart = gameState;
						}
						else
						{
							gs->chapterStart.clear();
						}
					}
					output(TXT("store game state as checkpoint (sync) %.3f"), subTS.get_time_since());
				});

			perform_sync_world_job(TXT("resuming destruction"), [this]()
				{
					resume_destruction(NAME(storingGameState));
				});

			output(TXT("game state created (%.3fms)"), createGameStateTS.get_time_since() * 1000.0f);

			if (_savePlayerProfile.get(true))
			{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
				output(TXT("save player profile with a new game slot"));
#endif
				save_player_profile();
			}
		});

}

void Game::pre_advance()
{
#ifdef AN_DEVELOPMENT
	{
		if (auto* vr = VR::IVR::get())
		{
			vr->force_bounds_rendering(true);
		}
	}
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_STANDARD_INPUT
#ifdef AN_DEBUG_KEYS
#define POOL_COUNTS(of) Pool<of>::get_used_count(), Pool<of>::get_total_count()
	if (::System::Input::get())
	{
		if (::System::Input::get()->has_key_been_pressed(::System::Key::F9))
		{
			output(TXT("selected pools"));
			output(TXT("Latent::Block                                     %5i / %5i"), POOL_COUNTS(Latent::Block));
			output(TXT("Latent::StackDataBuffer                           %5i / %5i"), POOL_COUNTS(Latent::StackDataBuffer));
			output(TXT("Framework::Nav::Node                              %5i / %5i"), POOL_COUNTS(Framework::Nav::Node));
			output(TXT("Framework::DisplayDrawCommands::TextAt            %5i / %5i"), POOL_COUNTS(Framework::DisplayDrawCommands::TextAt));
			output(TXT("Framework::DisplayDrawCommands::TexturePart       %5i / %5i"), POOL_COUNTS(Framework::DisplayDrawCommands::TexturePart));
		}
	}
#endif
#endif
#endif

	scoped_call_stack_info(TXT("tfg::game::pre_advance"));

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	if (get_secondary_view_render_buffer())
	{
		if (!overrideExternalRenderScene)
		{
			useExternalRenderScene = false;
			onlyExternalRenderScene = false;
			if (auto* c = TeaForGodEmperor::GameConfig::get_as<TeaForGodEmperor::GameConfig>())
			{
				useExternalRenderScene = c->get_secondary_view_info().show;
			}
		}
	}
	else
	{
		useExternalRenderScene = false;
		onlyExternalRenderScene = false;
	}
#endif

#ifdef AN_RENDERER_STATS
	Framework::Render::Stats::get().clear();
#endif

	// is player dead?
	{
		MEASURE_PERFORMANCE(isPlayerDead);
		bool playerDead = false;
		if (metaState == GameMetaState::Pilgrimage ||
			metaState == GameMetaState::Tutorial)
		{
			playerDead = true;
			if (auto* p = player.get_actor())
			{
				// if we're bound to something, we're not dead yet!
				playerDead = false;
				if (auto* h = p->get_custom<CustomModules::Health>())
				{
					playerDead = true;
					if (h->get_health().is_positive())
					{
						playerDead = false;
					}
				}
			}
		}
		if (playerDead)
		{
			if (!playerDeadFor.is_set())
			{
				output(TXT("player died"));
				on_ready_for_post_game_summary();
				if (musicPlayer)
				{
					musicPlayer->request_none();
					musicPlayer->request_combat_auto_stop();
					musicPlayer->stop(0.05f);
					musicPlayer->set_dull(1.0f, 0.05f);
					musicPlayer->block_requests();
				}
			}
			playerDeadFor.set_if_not_set(0.5f);
#ifdef AN_ALLOW_EXTENSIVE_LOGS
			float playerDeadForWas = playerDeadFor.get();
#endif
			playerDeadFor = playerDeadFor.get() - ::System::Core::get_raw_delta_time();
			if (playerDeadFor.get() < 0.0f)
			{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
				if (playerDeadForWas >= 0.0f)
				{
					output(TXT("start requesting post game summary"));
				}
#endif
				request_post_game_summary(PostGameSummary::Died);
			}
		}
		else
		{
			playerDeadFor.clear();
		}

		// #PGA for #demo, auto start after death
		// end when demo ends
		if (showPostGameSummary == PostGameSummary::None)
		{
			if (endDemoRequestedFor.is_set())
			{
				endDemoRequestedFor = endDemoRequestedFor.get() + ::System::Core::get_raw_delta_time();
				if (endDemoRequestedFor.get() > 1.0f)
				{
					request_post_game_summary(PostGameSummary::ReachedDemoEnd);
				}
			}
#ifdef AN_STANDARD_INPUT
			if (System::Input::get()->is_key_pressed(System::Key::Return))
			{
				endDemoKeyPressedFor.set_if_not_set(0.0f);	
				endDemoKeyPressedFor = endDemoKeyPressedFor.get() + ::System::Core::get_raw_delta_time();
				if (endDemoKeyPressedFor.get() > 1.0f)
				{
					request_post_game_summary(PostGameSummary::Interrupted);
				}
			}
			else
#endif
			{
				endDemoKeyPressedFor.clear();
			}
		}
		else
		{
			endDemoKeyPressedFor.clear();
		}
	}

	bool allowFadeIn = true;

	// fade out to summary and when faded out, end mode to show summary
	if (showPostGameSummary != PostGameSummary::None)
	{
		allowFadeIn = false;
		if (fading.target != 1.0f && metaState != GameMetaState::Debriefing && metaState != GameMetaState::TutorialDebriefing)
		{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
			output(TXT("fade out for post game summary"));
#endif
			// waiting while player not dead + transition for dead should be shorter than ceaseOnDeathDelay
			Colour fadeColour = RegisteredColours::get_colour(NAME(loader_hub_fade));
			fade_out(metaState == GameMetaState::Tutorial? 0.75f : (showPostGameSummary == PostGameSummary::ReachedEnd? 4.0f : (showPostGameSummary == PostGameSummary::ReachedDemoEnd ? 0.01f : (showPostGameSummary == PostGameSummary::Died ? 0.01f : 0.8f))), NAME(gameEnded), 0.0f, NP, fadeColour);
		}
		if (fading.current == 1.0f &&
			metaState != GameMetaState::Debriefing &&
			metaState != GameMetaState::TutorialDebriefing)
		{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
			output(TXT("faded out for post game summary"));
			output(TXT("end mode now"));
#endif
			if (auto* p = fast_cast<GameModes::Pilgrimage>(get_mode()))
			{
				switch (showPostGameSummary)
				{
				case PostGameSummary::Died:				p->set_pilgrimage_result(PilgrimageResult::PilgrimDied);		break;
				case PostGameSummary::Interrupted:		p->set_pilgrimage_result(PilgrimageResult::Interrupted);		break;
				case PostGameSummary::ReachedDemoEnd:	p->set_pilgrimage_result(PilgrimageResult::DemoEnded);			break;
				case PostGameSummary::ReachedEnd:		p->set_pilgrimage_result(PilgrimageResult::Ended);				break;
				default:																								break;
				}
			}
			end_mode(true);
#ifdef AN_ALLOW_EXTENSIVE_LOGS
			output(TXT("ended mode"));
#endif

			auto prevMetaState = metaState;
			if (postTutorialMetaState == GameMetaState::Debriefing && metaState == GameMetaState::Tutorial)
			{
				// skip right to summary
				set_meta_state(GameMetaState::Debriefing);
			}
			else if (metaState == GameMetaState::Tutorial)
			{
				set_meta_state(GameMetaState::TutorialDebriefing);
			}
			else
			{
				gameLog->choose_suggested_tutorials();
				debriefingPostGameSummary = showPostGameSummary;
				if (showPostGameSummary == PostGameSummary::Died)
				{
					// wait a bit showing nothing (we're faded out)
					System::TimeStamp ts;
					while (ts.get_time_since() < 1.0f)
					{
						advance_and_display_loader(nullptr);
					}
				}
				set_meta_state(GameMetaState::Debriefing);
			}

			// #demo (showy only when we reached this)
			if (prevMetaState != metaState &&
				(showPostGameSummary == PostGameSummary::ReachedEnd ||
				 showPostGameSummary == PostGameSummary::ReachedDemoEnd))
			{
				fade_in_immediately();
				if (showPostGameSummary == PostGameSummary::ReachedDemoEnd)
				{
					Loader::Text lText(LOC_STR(NAME(lsToBeContinued)));
					lText.activate();
					wait_till_loader_ends(&lText, 3.0f);
				}
				Colour fadeColour = RegisteredColours::get_colour(NAME(loader_hub_fade));
				fade_out_immediately(fadeColour);
			}
		}
	}

	// do game's pre advance after semaphore
	base::pre_advance();

	Framework::GameAdvanceContext gameAdvanceContext;

	// pre mode update lock for sync jobs
	{
		// not used yet WorldManagerImmediateSyncJobsScopedLock lockImmediateSyncJobs(this);

		// in-game menu, keep-in-world, fade-in
		{
			MEASURE_PERFORMANCE(hubMenusFadeIn);
			float const inGameMenuFadeOut = metaState == GameMetaState::Tutorial? 0.75f : 2.0f;
			float const inGameMenuFadeIn = 1.0f;
			bool inGameMenuForcedNow = inGameMenuForced;
#ifndef AN_DEVELOPMENT_OR_PROFILER
			if (::System::Core::is_vr_paused() && metaState == GameMetaState::Pilgrimage)
			{
				// note that if vr is paused, fading is still advanced, allowing to open the menu
				inGameMenuForcedNow = true;
			}
#endif
			Optional<Transform> keepInWorld;
			Optional<float> keepInWorldMaxDist;
			float newKeepInWorldPenetrationPt = 0.0f;
			if (VR::IVR::get() &&
				PlayerSetup::access_current_if_exists() &&
				PlayerSetup::access_current_if_exists()->get_preferences().keepInWorld &&
				! is_using_sliding_locomotion() &&
				(! GameDirector::get() || !GameDirector::get()->is_keep_in_world_temporarily_disabled()))
			{
				if (metaState == GameMetaState::Pilgrimage)
				{
					if (auto* actor = player.get_actor())
					{
						if (auto* pilgrim = actor->get_gameplay_as<ModulePilgrim>())
						{
							if (auto* pilgrimKeeper = pilgrim->get_pilgrim_keeper())
							{
								if (auto* pkm = fast_cast<ModuleMovementPilgrimKeeper>(pilgrimKeeper->get_movement()))
								{
									if (!pkm->is_at_pilgrim(nullptr, &newKeepInWorldPenetrationPt))
									{
										keepInWorld = pkm->get_valid_placement_vr();
										keepInWorldMaxDist = pkm->get_max_distance();
									}
								}
							}
						}
					}
				}
			}

			keepInWorldPenetrationPt = blend_to_using_time(keepInWorldPenetrationPt, newKeepInWorldPenetrationPt, 0.1f, ::System::Core::get_raw_delta_time());
			bool advanceRunTimerDuringMenu = true;
			if (!is_using_sliding_locomotion() &&
				keepInWorld.is_set())
			{
				bool fadeOutNow = false;
				if (is_fading_in(NAME(keepInWorld)))
				{
					fadeOutNow = true;
				}
				if ((!is_fading() && get_fading_state() == 0.0f))
				{
					fadeOutNow = true;
				}
				if (fadeOutNow)
				{
					Colour fadeColour = RegisteredColours::get_colour(NAME(loader_hub_fade));
					float fadeOutTime = 0.3f;
					fade_out(fadeOutTime * (1.0f - get_fading_state()), NAME(keepInWorld), NP, NP, fadeColour);
				}
				if (get_fading_state() == 1.0f)
				{
					bool supressOverlayInfoSystem = true;
					Optional<Loader::Hub*> usingHubLoader = get_main_hub_loader();
					RefCountObjectPtr<Loader::HubScene> keepInWorldScene;
					{
						Loader::HubScenes::BeAtRightPlace::be_at(keepInWorld.get()
#ifdef AN_DEVELOPMENT_OR_PROFILER
							, true
#endif
						);
						float disableTime = 30.0f;
						auto* barp = new Loader::HubScenes::BeAtRightPlace(false /* should deactivate when hit */);
						barp->allow_loading_tips(false);
						barp->ignore_rotation();
						barp->wait_for_be_at();
						barp->set_info_message(LOC_STR(NAME(lsMoveToValidPosition)));
						barp->with_special_button(Framework::StringFormatter::format_sentence_loc_str((NAME(lsDisableKeepInWorld)), Framework::StringFormatterParams()
								.add(NAME(time), String::printf(TXT("%.0f"), disableTime)))
							, [disableTime]() {ModuleMovementPilgrimKeeper::disable_for(disableTime); return false; });
						barp->with_special_button(Framework::StringFormatter::format_sentence_loc_str((NAME(lsPermanentlyDisableKeepInWorld)), Framework::StringFormatterParams()
								.add(NAME(time), String::printf(TXT("%.0f"), disableTime)))
							, []() { if (auto* ps = PlayerSetup::access_current_if_exists()) { ps->access_preferences().keepInWorld = false; } return false; });
						if (auto* ms = Library::get_current()->get_global_references().get<TeaForGodEmperor::MessageSet>(NAME(grKeepInWorldHelp)))
						{
							barp->with_help(ms);
						}
						keepInWorldScene = barp;
					}

					if (supressOverlayInfoSystem)
					{
						TutorialSystem::get()->pause_tutorial();
						overlayInfoSystem->supress();
					}
					if (advanceRunTimerDuringMenu)
					{
						System::Core::allow_on_advance(NAME(advanceRunTimer)); // opposite, we know we'll be showing a menu, we want to allow advancement then
					}
					if (keepInWorldMaxDist.is_set())
					{
						Loader::HubScenes::BeAtRightPlace::set_max_dist(keepInWorldMaxDist.get() * 0.8f); // a bit more strict
					}
					//play_global_reference_music_via_non_game_music_player(NAME(gsMenuMusic));
					show_hub_scene(keepInWorldScene.get(), usingHubLoader);
					Loader::HubScenes::BeAtRightPlace::set_max_dist();
					/*
					if (auto* actor = player.get_actor())
					{
						if (auto* pilgrim = actor->get_gameplay_as<ModulePilgrim>())
						{
							if (auto* pilgrimKeeper = pilgrim->get_pilgrim_keeper())
							{
								if (auto* pkm = fast_cast<ModuleMovementPilgrimKeeper>(pilgrimKeeper->get_movement()))
								{
									pkm->invalidate();
								}
							}
						}
					}
					*/
					stop_non_game_music_player();
					if (advanceRunTimerDuringMenu)
					{
						System::Core::block_on_advance(NAME(advanceRunTimer)); // block back
					}
					{
						// we're done with the scene, fade back in
						fade_out_immediately(); // fade out immediately, to fade in
						fade_in(0.5f, NAME(keepInWorld));
					}
					if (supressOverlayInfoSystem)
					{
						overlayInfoSystem->resume();
						TutorialSystem::get()->unpause_tutorial();
					}
				}
			}
			else if (inGameMenuRequested != InGameMenuRequest::No || inGameMenuForcedNow)
			{
				inGameMenuRequestedFor += ::System::Core::get_raw_delta_time();
				bool fadeOutNow = false;
				if (is_fading_in(NAME(inGameMenuRequested)))
				{
					fadeOutNow = true;
				}
				if ((!is_fading() && get_fading_state() == 0.0f))
				{
					fadeOutNow = true;
				}
				if (fadeOutNow)
				{
					Colour fadeColour = RegisteredColours::get_colour(NAME(loader_hub_fade));
					float fadeOutTime = inGameMenuFadeOut;
					if (inGameMenuForcedNow || inGameMenuRequested == InGameMenuRequest::Fast)
					{
						fadeOutTime = 0.3f;
					}
					fade_out(fadeOutTime* (1.0f - get_fading_state()), NAME(inGameMenuRequested), NP, NP, fadeColour);
				}
				if (get_fading_state() == 1.0f)
				{
					inGameMenuRequested = InGameMenuRequest::No;
					inGameMenuForced = false;
					Loader::HubScene* inGameMenuScene = nullptr;
					bool supressOverlayInfoSystem = true;
					Optional<Loader::Hub*> usingHubLoader;
					if (metaState == GameMetaState::Tutorial)
					{
						if (TutorialSystem::get()->should_open_normal_in_game_menu())
						{
							supressOverlayInfoSystem = false;
						}
						else
						{
							inGameMenuScene = TutorialSystem::get()->create_in_game_menu_scene();
							supressOverlayInfoSystem = TutorialSystem::get()->does_in_game_menu_supress_overlay();
						}
						usingHubLoader = get_hub_loader_for_tutorial();
					}
					else
					{
						usingHubLoader = get_main_hub_loader();
					}
					if (!inGameMenuScene)
					{
						inGameMenuScene = new Loader::HubScenes::InGameMenu();
					}

					if (supressOverlayInfoSystem)
					{
						TutorialSystem::get()->pause_tutorial();
						overlayInfoSystem->supress();
					}
					inGameMenuActive = true;
					if (advanceRunTimerDuringMenu)
					{
						System::Core::allow_on_advance(NAME(advanceRunTimer)); // opposite, we know we'll be showing a menu, we want to allow advancement then
					}
					play_global_reference_music_via_non_game_music_player(NAME(gsMenuMusic));
					Loader::HubScenes::BeAtRightPlace::reset_be_at(); // will be useful when getting out of it
					reset_immobile_origin_in_vr_space_for_sliding_locomotion();
					show_hub_scene(inGameMenuScene, usingHubLoader);
					stop_non_game_music_player();
					if (advanceRunTimerDuringMenu)
					{
						System::Core::block_on_advance(NAME(advanceRunTimer)); // block back
					}
					inGameMenuActive = false;
					if (showPostGameSummary != PostGameSummary::None)
					{
						Colour fadeColour = RegisteredColours::get_colour(NAME(loader_hub_fade));
						fade_out(0.0f, NAME(gameEnded), 0.0f, NP, fadeColour);

						// we also want to stop the music (it's paused/faded out anyway, we can stop it immediately)
						if (musicPlayer)
						{
							musicPlayer->reset();
						}
					}
					else
					{
						// we're done with the scene, fade back in
						fade_out_immediately(); // fade out immediately, to fade in
						fade_in(inGameMenuFadeIn, NAME(inGameMenuRequested));
						reset_immobile_origin_in_vr_space_for_sliding_locomotion();
					}
					if (supressOverlayInfoSystem)
					{
						overlayInfoSystem->resume();
						TutorialSystem::get()->unpause_tutorial();
					}
				}
			}
			else
			{
				if (allowFadeIn && !is_fading_in())
				{
					// we're done with the scene, fade back in
					fade_in(inGameMenuFadeIn * get_fading_state(), NAME(inGameMenuRequested));
				}
			}
		}

		// taking controls
		{
			takingControls.at = blend_to_using_speed_based_on_time(takingControls.at, takingControls.target, takingControls.blendTime.get(0.5f), get_delta_time());
		}

		// vignetteForDead
		{
			vignetteForDead.at = blend_to_using_speed_based_on_time(vignetteForDead.at, vignetteForDead.target, vignetteForDead.blendTime, get_delta_time());
		}

		// vignette for movement
		{
			float newVignetteForMovement = 0.0f;
			if (PlayerSetup::get_current().get_preferences().useVignetteForMovement)
			{
				if (forcedVignetteForMovementAmount.is_set())
				{
					newVignetteForMovement = forcedVignetteForMovementAmount.get();
				}
				else if (auto* pa = player.get_actor())
				{
					Vector3 baseVelocityLinear = pa->get_presence()->get_base_velocity_linear();
					Rotator3 baseVelocityRotation = pa->get_presence()->get_base_velocity_rotation();

					float settingsVignetteForMovement = PlayerSetup::get_current().get_preferences().useVignetteForMovement ? PlayerSetup::get_current().get_preferences().vignetteForMovementStrength : 0.0f;
					newVignetteForMovement += (baseVelocityLinear * Vector3::xy).length() / 2.0f + abs(baseVelocityLinear.z) / 0.8f;
					newVignetteForMovement += baseVelocityRotation.length() / 3.0f;
					newVignetteForMovement *= 0.3f * sqr(settingsVignetteForMovement) + 0.7f * settingsVignetteForMovement;
					newVignetteForMovement = clamp(newVignetteForMovement, 0.0f, 1.0f);
				}
			}
			vignetteForMovementAmount = blend_to_using_time(vignetteForMovementAmount, newVignetteForMovement, 0.1f, ::System::Core::get_raw_delta_time());
			vignetteForMovementActive = blend_to_using_time(vignetteForMovementActive, newVignetteForMovement > 0.01f? 1.0f : 0.0f, 0.1f, ::System::Core::get_raw_delta_time());
		}

		setup_game_advance_context(REF_ gameAdvanceContext);

		if (mode.is_set())
		{
			{
				MEASURE_PERFORMANCE(modePreAdvance);
				mode->pre_advance(gameAdvanceContext);
			}

			if (mode->should_end() || endModeRequested)
			{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
				output(TXT("mode says it should end  or  end mode requested"));
#endif
				end_mode(true);
			}
		}

#ifdef AN_ALLOW_AUTO_TEST
		if (is_auto_test_active() && mode.is_set())
		{
			autoTestTimeLeft -= System::Core::get_raw_delta_time();
			if (autoTestTimeLeft <= 0.0f)
			{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
				output(TXT("auto test ends mode"));
#endif
				end_mode(true);

				fade_out_immediately();
				set_meta_state(GameMetaState::JustStarted);

				show_hub_scene_waiting_for_world_manager(create_loading_hub_scene(LOC_STR(NAME(lsPleaseWait))));

				++autoTestIdx;
			}
		}
#endif
	}

	blockRunTime = true;
	// mode management
	{
		if (metaState == GameMetaState::Tutorial)
		{
			TutorialSystem::get()->advance();
		}

		bool stopNonGameMusicOnWayOut = false;

#ifdef AN_ALLOW_BULLSHOTS
		if (setupForBullshotId != Framework::BullshotSystem::get_id())
		{
			setupForBullshotId = Framework::BullshotSystem::get_id();
			if (mode.is_set())
			{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
				output(TXT("bullshot system ends mode"));
#endif
				end_mode();
			}
		}
		if (Framework::BullshotSystem::is_active() ||
			Framework::BullshotSystem::does_want_to_draw())
		{
			Framework::BullshotSystem::advance(deltaTime);
		}
#endif

		bool skipFading = false;
		bool skipWaiting = false;

		if (!mode.is_set() &&
			(!is_world_manager_busy() || // wait till nothing else to do, unless... 
			 metaState == GameMetaState::Debriefing || // ...unless we're going into debriefing, if so, allow - we will wait there until everything is destroyed
			 metaState == GameMetaState::TutorialDebriefing ||
			 metaState == GameMetaState::JustStarted)) // ...or we just started - there's nothing really important going on then
		{
			scoped_call_stack_info(TXT("no game mode"));
#ifndef DUMP
			mainHubLoader->disallow_fade_out(); // we will be switching between scenes without fading
			if (profileHubLoader) profileHubLoader->disallow_fade_out();

			{
				if (metaState == GameMetaState::JustStarted &&
					(Framework::TestConfig::get_params().get_value<Framework::LibraryName>(NAME(testRoom), Framework::LibraryName()).is_valid() ||
					 Framework::TestConfig::get_params().get_value<Framework::LibraryName>(NAME(testPilgrimage), Framework::LibraryName()).is_valid()))
				{
					// reset to not use game slot
					use_quick_game_player_profile(true); // this may break the menu afterwards a bit
					set_meta_state(GameMetaState::UseTestConfig);
				}
				else
				{
					while (! does_want_to_exit())
					{
						REMOVE_AS_SOON_AS_POSSIBLE_
						if (auto* vr = VR::IVR::get())
						{
							output(TXT("play area info on meta state [%i]"), metaState);
							vr->output_play_area();
						}

						if (metaState == GameMetaState::UseTestConfig)
						{
							bool doTestRoom = Framework::TestConfig::get_params().get_value<Framework::LibraryName>(NAME(testRoom), Framework::LibraryName()).is_valid();

							if (doTestRoom)
							{
								DebugVisualiser::block(true);
							}

#ifdef AN_ALLOW_AUTO_TEST
							// setup auto test
							{
								{
									float autoTestIntervalInMinutes = Framework::TestConfig::get_params().get_value<float>(NAME(autoTestIntervalInMinutes), 0.0f);
									if (autoTestIntervalInMinutes > 0.0f)
									{
										autoTestInterval = autoTestIntervalInMinutes * 60.0f;
									}
									autoTestInterval = Framework::TestConfig::get_params().get_value<float>(NAME(autoTestInterval), autoTestInterval);
								}

								autoTestProcessAllJobs = Framework::TestConfig::get_params().get_value<bool>(NAME(autoTestProcessAllJobs), autoTestProcessAllJobs);

								testingMultiplePlayAreaRectSizes = false;

								{
									if (Framework::TestConfig::get_params().get_existing<String>(NAME(testMultiplePlayAreaRectSizes)) ||
										Framework::TestConfig::get_params().get_existing<float>(NAME(testMultiplePlayAreaMaxSize)))
									{
										String testMultiplePlayAreaRectSizesString = Framework::TestConfig::get_params().get_value<String>(NAME(testMultiplePlayAreaRectSizes), String::empty());
										testingMultiplePlayAreaRectSizes = true;
										testMultiplePlayAreaRectSizes.clear();
										List<String> pairs;
										testMultiplePlayAreaRectSizesString.split(String(TXT("{")), pairs);
										for_every(pair, pairs)
										{
											if (pair->trim().is_empty())
											{
												// first one is always going to be empty
												continue;
											}
											bool isValid = true;
											if (isValid)
											{
												int endsAt = pair->find_first_of('}');
												if (endsAt != NONE)
												{
													*pair = pair->get_left(endsAt);
												}
												else
												{
													error(TXT("invalid testMultiplePlayAreaRectSizes"));
													isValid = false;
												}
											}
											if (isValid)
											{
												List<String> numbers;
												pair->split(String(TXT(",")), numbers);
												if (numbers.get_size() == 2)
												{
													Vector2 size;
													size.x = ParserUtils::parse_float(numbers[0]);
													size.y = ParserUtils::parse_float(numbers[1]);
													testMultiplePlayAreaRectSizes.push_back(size);
												}
												else
												{
													error(TXT("invalid testMultiplePlayAreaRectSizes, pair \"%S\""), pair->to_char());
													isValid = false;
												}
											}
										}

										Vector2 size;
										if (testMultiplePlayAreaRectSizes.is_index_valid(autoTestIdx))
										{
											size = testMultiplePlayAreaRectSizes[autoTestIdx];
										}
										else
										{
											Random::Generator rg;
											float maxSize = Framework::TestConfig::get_params().get_value<float>(NAME(testMultiplePlayAreaMaxSize), 4.0f);
											maxSize = rg.get_float(1.8f, maxSize);
											size.x = rg.get_float(1.8f, maxSize);
											size.y = rg.get_float(1.2f, maxSize);
											if (rg.get_chance(Framework::TestConfig::get_params().get_value<float>(NAME(testMultiplePlayAreaSquarishChance), 0.3f))) // more squarish
											{
												float o = (size.x + size.y) * 0.5f;
												float useAvg = 0.7f;
												size.x = size.x * (1.0f - useAvg) + useAvg * o;
												size.y = size.y * (1.0f - useAvg) + useAvg * o;
											}
										}
										Framework::TestConfig::access_params().access<Vector2>(NAME(testPlayArea)) = size;
										Framework::TestConfig::access_params().access<float>(NAME(testTileSize)) = 0.0f;

										{
											lock_output();
											output_colour(1, 1, 1, 1);
											for_count(int, i, 5)
											{
												output(TXT("|"));
											}
											output(TXT("AUTO TESTING PLAY AREA (%i, time %.1fs)"), autoTestIdx, autoTestInterval);
											RoomGenerationInfo::access().setup_to_use();
											output_colour();
											unlock_output();
										}
									}
								}
							}
							autoTestTimeLeft = autoTestInterval;
#endif

							play_loading_music_via_non_game_music_player(3.0f);
#ifdef AN_LOG_META_STATES
							output(TXT("[MS] skip, use test room/pilgrimage"));
#endif
							showPostGameSummary = PostGameSummary::None;
							GameDefinition* testGameDefinition = nullptr;
							Array<GameSubDefinition*> testGameSubDefinitions;
							{
								{
									Framework::LibraryName testGameDefinitionName = Framework::TestConfig::get_params().get_value<Framework::LibraryName>(NAME(testGameDefinition), Framework::LibraryName());
									if (testGameDefinitionName.is_valid())
									{
										if (GameDefinition* gd = Library::get_current_as<Library>()->get_game_definitions().find(testGameDefinitionName))
										{
											testGameDefinition = gd;
										}
									}
								}
								{
									Framework::LibraryName testGameSubDefinitionName = Framework::TestConfig::get_params().get_value<Framework::LibraryName>(NAME(testGameSubDefinition), Framework::LibraryName());
									if (testGameSubDefinitionName.is_valid())
									{
										if (GameSubDefinition* gsd = Library::get_current_as<Library>()->get_game_sub_definitions().find(testGameSubDefinitionName))
										{
											testGameSubDefinitions.clear();
											testGameSubDefinitions.push_back(gsd);
										}
									}
								}
							}
							{
								if (!testGameDefinition)
								{
									if (auto* gs = PlayerSetup::access_current().access_active_game_slot())
									{
										testGameDefinition = cast_to_nonconst(gs->get_game_definition());
									}
								}
								if (testGameSubDefinitions.is_empty())
								{
									if (auto* gs = PlayerSetup::access_current().access_active_game_slot())
									{
										for_every_ref(gsd, gs->get_game_sub_definitions())
										{
											testGameSubDefinitions.clear(); 
											testGameSubDefinitions.push_back(gsd);
										}
									}
								}
							}
							{
								if (!testGameDefinition || doTestRoom)
								{
									testGameDefinition = Library::get_current()->get_global_references().get<GameDefinition>(NAME(gameDefinitionTestRoom));
								}
								if (testGameSubDefinitions.is_empty() || doTestRoom)
								{
									testGameSubDefinitions.clear();
								}
							}
							if (!testGameDefinition)
							{
								error(TXT("no game definition provided"));
							}
							if (auto* gs = PlayerSetup::access_current().access_active_game_slot())
							{
								gs->set_game_definition(testGameDefinition, testGameSubDefinitions, true /* restart for test room */, true);
							}
							else
							{
								GameDefinition::choose(testGameDefinition, testGameSubDefinitions);
							}

							{	// reupdate using run setup to use test difficulty settings etc
								RunSetup runSetup;
								runSetup.read_into_this();
								runSetup.update_using_this(); // will reset game definition here
								//
								RoomGenerationInfo::access().setup_to_use();
							}

							GameSettings::get().load_test_difficulty_settings();

							before_pilgrimage_starts();

							set_meta_state(GameMetaState::Pilgrimage); // we pretend we're pilgrimage
							// find the right pilgrimage (or choose any random)
							Pilgrimage* testPilgrimage = GameDefinition::get_chosen()->get_pilgrimages().get_first().get();
							if (auto* tpn = Framework::TestConfig::get_params().get_existing<Framework::LibraryName>(NAME(testPilgrimage)))
							{
								if (auto* tp = Library::get_current_as<Library>()->get_pilgrimages().find(*tpn))
								{
									testPilgrimage = tp;
								}
							}
#ifdef AN_ALLOW_EXTENSIVE_LOGS
							output(TXT("start mode: pilgrimage setup (meta state: use test config)"));
#endif
							start_mode(new GameModes::PilgrimageSetup(this, testPilgrimage, nullptr));
							// for test rooms just go with simple "please wait".

							RefCountObjectPtr<Loader::HubScene> waitingScene;
					
							//if (!is_using_sliding_locomotion() &&
							//	!doTestRoom)
							{
								Loader::HubScenes::BeAtRightPlace::reset_be_at();
								auto* barp = new Loader::HubScenes::BeAtRightPlace(true);
								barp->allow_loading_tips(false); // test config
								//barp->ignore_rotation();
								barp->wait_for_be_at();
								barp->set_waiting_message(LOC_STR(NAME(lsPleaseWait)));
								barp->set_info_message(LOC_STR(NAME(lsMoveToStartingPosition)));
								barp->with_go_back_button([this]() {set_wants_to_cancel_creation(true); });
								waitingScene = barp;
							}
							auto* useHub = get_main_hub_loader();
							useHub->show_start_movement_centre(true);
							if (!waitingScene.get())
							{
								waitingScene = create_loading_hub_scene(LOC_STR(NAME(lsPleaseWait)));
							}
							show_hub_scene_waiting_for_world_manager(waitingScene.get());

							if (! does_want_to_cancel_creation())
							{
								before_pilgrimage_starts_ready();

								stopNonGameMusicOnWayOut = true;

								DebugVisualiser::block(false);

#ifdef AN_DEVELOPMENT_OR_PROFILER
								bool allowAutoPause = true;
#ifdef AN_RECORD_VIDEO
								allowAutoPause = false;
#endif
#endif
#ifdef AN_ALLOW_AUTO_TEST
								if (is_auto_test_active())
								{
									if (testingMultiplePlayAreaRectSizes)
									{
										PilgrimageInstanceOpenWorld::check_all_cells(true, true, autoTestProcessAllJobs);
									}

									// never let the player be killed when auto testing
									GameSettings::get().difficulty.playerImmortal = true;

									allowAutoPause = false;
								}
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef TIME_MANIPULATIONS_DEV
								if (allowAutoPause)
								{
									static bool pausedOnce = false;
									if (!pausedOnce)
									{
										pausedOnce = true;
										::System::Core::one_frame_time_pace(2 /*two frames to advance objects propeprly*/);
									}
								}
#endif
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
								output(TXT("use test config is done, break into the game"));
#endif
								// make sure we start with an empty overlay info system
								overlayInfoSystem->force_clear();

								pilgrimageStartedTS.reset();

								// since now default "force instant object creation"
								force_instant_object_creation(false);

								stopNonGameMusicOnWayOut = true;
								break;
							}
							else
							{
#ifdef AN_LOG_META_STATES
								output(TXT("pilgrimage startup failed or cancelled, switch to pilgrimage setup"));
#endif
								// abort mode if still happening
								end_mode(true);
								// ready to clean up
								drop_all_delayed_object_creations();
								force_instant_object_creation(false);
								// cancel world generation
								show_hub_scene_waiting_for_world_manager(create_loading_hub_scene(LOC_STR(NAME(lsPleaseWait)), 0.25f, NP, NP, false /*  no loading tips*/));
								// restore to non canceling
								set_wants_to_cancel_creation(false); // we want to create since now
								// reset last forward, so we show menu at the right place - in front of us
								Loader::Hub::reset_last_forward(); // anything we go to will require different last forward
								if (useQuickGameProfile)
								{
									set_meta_state(GameMetaState::QuickPilgrimageSetup);
								}
								else
								{
									set_meta_state(GameMetaState::PilgrimageSetup);
								}
							}
						}
						else if (metaState == GameMetaState::JustStarted)
						{
#ifdef AN_LOG_META_STATES
							output(TXT("[MS] just started"));
							output(TXT("allow saving player profiles"));
							output(startWithWelcomeScene ? TXT("should start with welcome screen") : TXT("load game slots and choose one"));
#endif
							bool noGameSlotsAtAll = startWithWelcomeScene;

							show_hub_scene_waiting_for_world_manager(nullptr, nullptr,
								[this, &noGameSlotsAtAll]()
								{
									// if there are no profiles, create one
									// if the chosen does not exist (or no chosen), choose the first one
									update_player_profiles_list(false /* do not select */);
									auto const& profileNames = get_player_profiles();
									String selectedProfile;
									if (auto* c = TeaForGodEmperor::GameConfig::get_as<TeaForGodEmperor::GameConfig>())
									{
										selectedProfile = c->get_player_profile_name();
									}
									if (profileNames.is_empty() ||
										Framework::TestConfig::get_params().get_value<bool>(NAME(testWelcomeScene), false))
									{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
										if (profileNames.is_empty())
										{
											output(TXT("no profile names"));
										}
										if (Framework::TestConfig::get_params().get_value<bool>(NAME(testWelcomeScene), false))
										{
											output(TXT("test welcome scene"));
										}
#endif
										create_and_choose_default_player();
										noGameSlotsAtAll = true;

										// don't save it until we set it up properly
									}
									else
									{
										if (selectedProfile.is_empty() || !profileNames.does_contain(selectedProfile))
										{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
											output(TXT("none selected, select first one"));
#endif
											selectedProfile = profileNames.get_first();
										}
										use_quick_game_player_profile(false);
#ifdef AN_ALLOW_EXTENSIVE_LOGS
										output(TXT("load \"%S\""), selectedProfile.to_char());
#endif
										choose_player_profile(selectedProfile, true /* to create a file if can't be loaded */);
									}
								});
							{
								auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
								if (noGameSlotsAtAll)
								{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
									output(TXT("+ no game slots at all, create the first one and select it"));
#endif
									auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
									ps.have_at_least_one_game_slot(); // required to be setup
								}
								//
								if ((!ps.get_active_game_slot() && ps.get_game_slots().is_empty()) || noGameSlotsAtAll)
								{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
									output(TXT("+ no active game slot or no game slots at all (might be created by this time), create new game slot"));
#endif
									set_meta_state(GameMetaState::SetupNewGameSlot);
								}
								else if (!ps.get_active_game_slot() && !ps.get_game_slots().is_empty())
								{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
									output(TXT("+ no active game slot, select game slot"));
#endif
									set_meta_state(GameMetaState::SelectGameSlot);
								}
								else
								{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
									output(TXT("+ active game slot, setup pilgrimage"));
#endif
									set_meta_state(GameMetaState::PilgrimageSetup);
								}
							}
						}
						else if (metaState == GameMetaState::SelectPilgrimageSetup)
						{
#ifdef AN_LOG_META_STATES
							output(TXT("[MS] select pilgrimage setup"));
#endif
							if (useQuickGameProfile)
							{
								set_meta_state(GameMetaState::QuickPilgrimageSetup);
							}
							else
							{
								set_meta_state(GameMetaState::PilgrimageSetup);
							}
						}
						else if (metaState == GameMetaState::TutorialSelection)
						{
#ifdef AN_LOG_META_STATES
							output(TXT("[MS] tutorial selection"));
#endif
							ready_to_use_hub_loader();
							//show_empty_hub_scene_and_save_player_profile();
							an_assert(!get_mode());
							show_hub_scene_waiting_for_world_manager(new Loader::HubScenes::TutorialSelection(), get_hub_loader_for_tutorial());
						}
						else if (metaState == GameMetaState::SelectPlayerProfile)
						{
#ifdef AN_LOG_META_STATES
							output(TXT("[MS] select player profile"));
#endif
							ready_to_use_hub_loader();
							an_assert(!get_mode());
							show_hub_scene_waiting_for_world_manager(new Loader::HubScenes::PlayerProfileSelection(), get_main_hub_loader()/*profileHubLoader*/);
							may_need_to_save_player_profile();
						}
						else if (metaState == GameMetaState::SetupNewPlayerProfile)
						{
#ifdef AN_LOG_META_STATES
							output(TXT("[MS] setup new player profile"));
#endif
							an_assert(!get_mode());
							ready_to_use_hub_loader(get_main_hub_loader()/*profileHubLoader*/);
							show_hub_scene_waiting_for_world_manager(new Loader::HubScenes::SetupNewPlayerProfile(), get_hub_loader_for_pilgrimage()/*profileHubLoader*/);
							may_need_to_save_player_profile();
						}
						else if (metaState == GameMetaState::SelectGameSlot)
						{
#ifdef AN_LOG_META_STATES
							output(TXT("[MS] select game slot"));
#endif
							// temporary, auto assume default game slot
							an_assert(!get_mode());
							an_assert(!useQuickGameProfile);
							ready_to_use_hub_loader(get_main_hub_loader()/*profileHubLoader*/);
							show_hub_scene_waiting_for_world_manager(new Loader::HubScenes::GameSlotSelection(), get_main_hub_loader()/*profileHubLoader*/);
							// at this point everyone should already save player profiles!
							may_need_to_save_player_profile();
						}
						else if (metaState == GameMetaState::SetupNewGameSlot)
						{
#ifdef AN_LOG_META_STATES
							output(TXT("[MS] setup new game slot"));
#endif
							an_assert(!get_mode());
							ready_to_use_hub_loader(get_main_hub_loader()/*profileHubLoader*/);
							if (!PlayerSetup::access_current().access_active_game_slot())
							{
								auto& ps = PlayerSetup::access_current();
								ps.have_at_least_one_game_slot(); // required to be setup
							}
							show_hub_scene_waiting_for_world_manager(new Loader::HubScenes::SetupNewGameSlot(), get_main_hub_loader()/*profileHubLoader*/);
							//may_need_to_save_player_profile();
							show_empty_hub_scene_and_save_player_profile(recentHubLoader);
						}
						else if (metaState == GameMetaState::PilgrimageSetup)
						{
#ifdef AN_LOG_META_STATES
							output(TXT("[MS] pilgrimage setup"));
#endif
							// should already choose a right profile (and a game slot)
							an_assert(!get_mode());
							an_assert(!useQuickGameProfile);
							if (auto* gs = PlayerSetup::access_current().access_active_game_slot())
							{
								if (!gs->get_game_definition())
								{
									set_meta_state(GameMetaState::SetupNewGameSlot);
									continue;
								}
								// in case we're loading
								gs->choose_game_definition();
								gs->prepare_missions_definition();
								gs->update_mission_state();
							}
							ready_to_use_hub_loader(get_main_hub_loader()/*profileHubLoader*/);
							before_pilgrimage_setup();
							if (! is_demo())
							{
								if (auto* gs = PlayerSetup::access_current().access_active_game_slot())
								{
									bool doAsyncSave = false;
									// if we finished the main story at least once, other game slots should have missions available
									if (PlayerSetup::access_current().get_general_progress().get_tag(NAME(finishedMainStory)))
									{
										if (!gs->gameSlotModesAvailable.does_contain(GameSlotMode::Missions))
										{
											gs->make_game_slot_mode_available(GameSlotMode::Missions);
											doAsyncSave = true;
										}
									}
									// if we finished main story now, unlock missions
									if (gs->get_general_progress().get_tag(NAME(finishedMainStory)))
									{
										if (!gs->gameSlotModesAvailable.does_contain(GameSlotMode::Missions))
										{
											gs->make_game_slot_mode_available(GameSlotMode::Missions);
											doAsyncSave = true;
										}
									}
									// if missions are unlocked but not confirmed, confirm now
									if (gs->gameSlotModesAvailable.does_contain(GameSlotMode::Missions) &&
										!gs->gameSlotModesConfirmed.does_contain(GameSlotMode::Missions))
									{
										gs->make_game_slot_mode_confirmed(GameSlotMode::Missions);
										doAsyncSave = true;
										if (!PlayerSetup::access_current().get_general_progress().get_tag(NAME(confirmedMissions)))
										{
											PlayerSetup::access_current().access_general_progress().set_tag(NAME(confirmedMissions));
											add_async_save_player_profile(true);
											doAsyncSave = false;
											show_hub_scene_waiting_for_world_manager(new Loader::HubScenes::Message(LOC_STR(NAME(lsMissionsUnlocked))), get_main_hub_loader()/*profileHubLoader*/);
										}
									}
									if (doAsyncSave)
									{
										add_async_save_player_profile(true);
									}
								}						
							}						
							show_hub_scene_waiting_for_world_manager(new Loader::HubScenes::PilgrimageSetup(), get_main_hub_loader()/*profileHubLoader*/);
							Loader::HubScenes::PilgrimageSetup::mark_reached_end(false);// clear
							post_pilgrimage_setup();
							Loader::Hub::reset_last_forward(); // anything we go to will require different last forward
						}
						else if (metaState == GameMetaState::QuickPilgrimageSetup)
						{
#ifdef AN_LOG_META_STATES
							output(TXT("[MS] quick pilgrimage setup"));
#endif
							// should already be in default profile)
							an_assert(!get_mode());
							an_assert(useQuickGameProfile);
							if (auto* gs = PlayerSetup::access_current().access_active_game_slot())
							{
								if (useQuickGameProfile)
								{
									playerSetup.setup_for_quick_game_player_profile();
								}
								{
									GameDefinition* gameDefinition = Library::get_current()->get_global_references().get<GameDefinition>(NAME(gameDefinitionQuickGame));
									Array<GameSubDefinition*> gameSubDefinitions;
									gs->set_game_definition(gameDefinition, gameSubDefinitions, true /* reset for quick game */, true);
								}
								static bool startingGearSet = false;
								//if (!startingGearSet) /// always clear for time being
								{
									gs->clear_persistence_and_use_starting_gear();
									startingGearSet = true;
								}
							}
							ready_to_use_hub_loader();
							before_pilgrimage_setup();
							show_hub_scene_waiting_for_world_manager(new Loader::HubScenes::QuickGameMenu());
							post_pilgrimage_setup();
						}
						else if (metaState == GameMetaState::Pilgrimage)
						{
							play_loading_music_via_non_game_music_player(3.0f);
#ifdef AN_LOG_META_STATES
							output(TXT("[MS] pilgrimage"));
							output(TXT("start pilgrimage"));
#endif
							GameStats::use_pilgrimage();

							// restore to non canceling
							set_wants_to_cancel_creation(false); // we want to create since now

							bool startedPilgrimage = false;
							// load from game slot
							if (auto* gs = PlayerSetup::access_current().access_active_game_slot())
							{
								// update only when starting a pilgrimage
								gs->update_build_no_to_current();

								if (auto* gameState = gs->startUsingGameState.get())
								{
#ifdef AN_LOG_GAME_STATE
									output(TXT("using game state"));
									gameState->debug_log(TXT("usingGameState"));
#else
#ifdef AN_LOG_META_STATES
									output(TXT("using game state"));
#endif
#endif

									gs->startedUsingGameState = gameState;
									gs->choose_game_definition();
									gs->prepare_missions_definition();
									gs->update_mission_state();
									/*
									 * not used right now as we want to allow to change game modifiers if someone finds the game too hard
									GameModifier::apply(gameState->gameModifiers);
									RunSetup::update_rgi_requested_generation_tags(gameState->gameModifiers);
									 */
#ifdef AN_ALLOW_EXTENSIVE_LOGS
									output(TXT(" with tags: \"%S\""), RoomGenerationInfo::get().get_requested_generation_tags().to_string().to_char());
									output(TXT(" pilgrimage: \"%S\""), gameState->pilgrimage->get_name().to_string().to_char());
#endif
									GameSettings::get().load_test_difficulty_settings();

									before_pilgrimage_starts();
									// do not clear_played_recently_pilgrimages

									RefCountObjectPtr<Loader::HubScene> waitingScene;
									if (!is_using_sliding_locomotion())
									{
										Loader::HubScenes::BeAtRightPlace::reset_be_at();
										auto* barp = new Loader::HubScenes::BeAtRightPlace(true);
										barp->allow_loading_tips(true); // from game slot - always load
										barp->ignore_rotation();
										barp->wait_for_be_at();
										barp->set_waiting_message(LOC_STR(NAME(lsPleaseWait)));
										barp->set_info_message(LOC_STR(NAME(lsMoveToStartingPosition)));
										barp->with_go_back_button([this]() {set_wants_to_cancel_creation(true); });
										waitingScene = barp;
									}
									if (!waitingScene.is_set())
									{
										waitingScene = create_loading_hub_scene(LOC_STR(NAME(lsPleaseWait)), 0.25f);
									}
									auto* useHub = get_main_hub_loader();
									useHub->show_start_movement_centre(true);
									show_hub_scene_waiting_for_world_manager(waitingScene.get(), useHub,
										[this, gameState]()
										{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
											output(TXT("start mode: pilgrimage setup (meta state: pilgrimage) (from game state)"));
#endif
											// GameDefinition chosen at setup
											start_mode(new GameModes::PilgrimageSetup(this, gameState->pilgrimage.get(), gameState));
											GameStats::get() = gameState->gameStats; // restore stats post pilgrimage start (as it resets on its own)
											if (auto* gd = GameDirector::get())
											{
												Concurrency::ScopedMRSWLockWrite lock(gd->access_story_execution().access_facts_lock());
												auto& facts = gd->access_story_execution().access_facts();
												facts.clear();
												facts.set_tags_from(gameState->storyFacts);
											}
										});
									useHub->show_start_movement_centre(false);
									useHub->set_beacon_active(false);

									if (!does_want_to_cancel_creation())
									{
										startedPilgrimage = true;
									}
								}
							}
							// start as usual
							if (!does_want_to_cancel_creation() &&
								! startedPilgrimage)
							{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
								output(TXT(" with tags: \"%S\""), RoomGenerationInfo::get().get_requested_generation_tags().to_string().to_char());
#endif

								Pilgrimage* pilgrimage = nullptr;
								bool allowLoadingTips = false;

								auto* gs = PlayerSetup::access_current().access_active_game_slot();
								if (!is_demo() && gs && gs->gameSlotMode == GameSlotMode::Missions)
								{
									gs->create_new_mission_state();

									// ignore until we start a mission
									gs->ignore_last_mission_result_for_summary();
								
									if (auto* ms = MissionState::get_current())
									{
										if (!ms->get_pilgrimages().is_empty())
										{
											pilgrimage = ms->get_pilgrimages().get_first().get();
										}
									}

									allowLoadingTips = true;
								}
								if (! pilgrimage)
								{
									int startPilgrimageIdx = 0;
									// check if we should start at some specific pilgrimage
									if (gs)
									{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
										output(TXT(" with tags: \"%S\""), RoomGenerationInfo::get().get_requested_generation_tags().to_string().to_char());
#endif
#ifdef AN_LOG_META_STATES
										output(TXT(" gs restarAtPilgrimage: \"%S\""), gs->restartAtPilgrimage.to_string().to_char());
#endif
										for_every_ref(p, GameDefinition::get_chosen()->get_pilgrimages())
										{
											if (gs->restartAtPilgrimage == p->get_name())
											{
												startPilgrimageIdx = for_everys_index(p);
												break;
											}
										}
										for_every(cp, GameDefinition::get_chosen()->get_conditional_pilgrimages())
										{
											if (auto* p = cp->pilgrimage.get())
											{
												if (gs->restartAtPilgrimage == p->get_name())
												{
													startPilgrimageIdx = for_everys_index(cp);
													break;
												}
											}
										}
									}
									int pilgrimageCount = GameDefinition::get_chosen()->get_pilgrimages().get_size();
									int conditionalPilgrimageCount = GameDefinition::get_chosen()->get_conditional_pilgrimages().get_size();
									startPilgrimageIdx = clamp(startPilgrimageIdx, 0, pilgrimageCount + conditionalPilgrimageCount - 1);
									if (startPilgrimageIdx < pilgrimageCount)
									{
										pilgrimage = GameDefinition::get_chosen()->get_pilgrimages()[startPilgrimageIdx].get();
									}
									else
									{
										pilgrimage = GameDefinition::get_chosen()->get_conditional_pilgrimages()[startPilgrimageIdx - pilgrimageCount].pilgrimage.get();
									}

#ifdef AN_LOG_META_STATES
									output(TXT(" pilgrimage: %i \"%S\""), startPilgrimageIdx, pilgrimage->get_name().to_string().to_char());
#endif
									if (startPilgrimageIdx != 0)
									{
										allowLoadingTips = true;
									}
								}
								// this is a fresh start, make sure we know that
								PlayerSetup::access_current().stats__fresh_start();

								GameSettings::get().load_test_difficulty_settings();

								before_pilgrimage_starts();

								// not from game state, clear
								if (auto* gs = PlayerSetup::access_current().access_active_game_slot())
								{
									// we're starting the whole thing again
									gs->clear_played_recently_pilgrimages();
								}

								if (!does_want_to_cancel_creation())
								{
									RefCountObjectPtr<Loader::HubScene> fadeOutScene;
									Loader::HubScenes::BeAtRightPlace* waitingSceneBARP = nullptr;
									fadeOutScene = new Loader::HubScenes::FadeOut();
									RefCountObjectPtr<Loader::HubScene> waitingScene;
									if (!is_using_sliding_locomotion())
									{
										// commented this out as maybe we should show loading tips only when starting from a game slot or not from the start?
										// keep it clean and easy to understand for the first playthrough
										/*
										else if (auto* ps = PlayerSetup::access_current_if_exists())
										{
											allowLoadingTips |= (ps->get_stats().runs > 1); // we're at 1 in our first run
										}
										*/

										Loader::HubScenes::BeAtRightPlace::reset_be_at();
										auto* barp = new Loader::HubScenes::BeAtRightPlace(true);
										barp->allow_loading_tips(allowLoadingTips);
										//barp->ignore_rotation();
										barp->wait_for_be_at();
										barp->set_waiting_message(LOC_STR(NAME(lsPleaseWait)));
										barp->set_info_message(LOC_STR(NAME(lsMoveToStartingPosition)));
										barp->with_go_back_button([this]() {set_wants_to_cancel_creation(true);});
										waitingScene = barp;
										waitingSceneBARP = barp;
									}
									if (!waitingScene.is_set())
									{
										waitingScene = create_loading_hub_scene(LOC_STR(NAME(lsPleaseWait)), 0.25f);
									}
									auto* useHub = get_main_hub_loader();
									useHub->show_start_movement_centre(true);
									bool started = false;
									if (!started && pilgrimage->has_custom_loader() && ! does_want_to_cancel_creation())
									{
										if (Loader::ILoader* customLoader = pilgrimage->create_custom_loader())
										{
											Optional<Transform> shouldBeAt = pilgrimage->provide_initial_be_at_right_place_for_custom_loader();

											// do rgi setup as an asynchronous job as it may need to recreate door meshes
											bool rgiDone = false;
											add_async_world_job_top_priority(TXT("setup RGI"), [&rgiDone]()
												{
													PlayerSetup::get_current().get_preferences().apply_to_vr_and_room_generation_info(); // make sure we have latest preferences in
													RoomGenerationInfo::access().setup_to_use();
													
													rgiDone = true;
												});

											{
												float radius = 0.2f; // a bit wider at the end so when we move a bit during custom loader, we're still fine
												radius = pilgrimage->get_starting_point_radius_for_custom_loader().get(radius);
												float startRadius = max(radius * 0.5f, radius - 0.1f); // with it being smaller we should avoid using barp twice
												// elements of sequence
												Loader::Sequence seq;
												Loader::HubInSequence waitForRGI(useHub, new Loader::HubScenes::Empty(true), false);
												Loader::HubInSequence initialBarpSeq(useHub, waitingScene.get(), true);
												Loader::HubInSequence barpSeq(useHub, waitingScene.get(), true);
												Loader::HubInSequence fadeOutSceneSeq(useHub, fadeOutScene.get(), true);

												// populate sequence
												if (shouldBeAt.is_set() && waitingSceneBARP)
												{
													Loader::HubScenes::BeAtRightPlace::be_at(shouldBeAt.get(), shouldBeAt.get().get_translation().z < 0.2f);
													Loader::HubScenes::BeAtRightPlace::set_max_dist(startRadius);
													seq.add([waitingSceneBARP]()
														{
															waitingSceneBARP->set_requires_to_be_deactivated(false);
															waitingSceneBARP->set_info_message(LOC_STR(NAME(lsMoveToStartingPosition)));
														});
													seq.add(&initialBarpSeq, NP, [waitingSceneBARP]()
														{
															return waitingSceneBARP ? waitingSceneBARP->is_required() : false;
														});
												}
												seq.add(&waitForRGI, NP, [&rgiDone]() { return !rgiDone; });
												seq.add([this]() {stop_non_game_music_player(1.0f); });
												seq.add(&fadeOutSceneSeq, NP, [this]() { return get_fade_out() < 1.0f; });
												seq.add([this]() { end_all_sounds(); });
												seq.add(customLoader, NP, nullptr, true);
												if (shouldBeAt.is_set() && waitingSceneBARP)
												{
													seq.add([waitingSceneBARP, radius]()
														{
															// if we enter it, we require to fade out
															waitingSceneBARP->set_requires_to_be_deactivated(false);
															//waitingSceneBARP->set_waiting_message(LOC_STR(NAME(lsPleaseWait)));
															waitingSceneBARP->set_info_message(LOC_STR(NAME(lsMoveToStartingPosition)));

															Loader::HubScenes::BeAtRightPlace::set_max_dist(radius);
														});
													seq.add(&barpSeq, NP, [waitingSceneBARP, this, &skipFading]()
														{
															if (waitingSceneBARP && waitingSceneBARP->is_required() && !does_want_to_cancel_creation())
															{
																// we should fade in and fade out
																skipFading = false;
																fade_out_immediately(); // this is to force fade in
																return true;
															}
															else
															{
																if (get_fade_out() == 0.0f)
																{
																	// if we wanted to be fade in, we should faded out
																	// we assume no fading here
																	skipFading = true;
																}
																return false;
															}
														});
													seq.add([]()
														{
															// clear
															Loader::HubScenes::BeAtRightPlace::set_max_dist();
														});
												}

												if (!does_want_to_cancel_creation())
												{
													// do wait
													show_loader_waiting_for_world_manager(&seq,
														[this, pilgrimage]()
														{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
															output(TXT("start mode: pilgrimage setup (meta state: pilgrimage) (custom loader)"));
#endif
															// GameDefinition chosen at setup
															start_mode(new GameModes::PilgrimageSetup(this, pilgrimage, nullptr));
														});
													started = true;
													skipWaiting = true;
												}
											}

											// clean up
											delete customLoader;
										}
									}
#ifndef ALLOW_LOADING_TIPS_WITH_BE_AT_RIGHT_PLACE
									if (!started)
									{
										if (auto* psp = PlayerSetup::get_current_if_exists())
										{
											if (psp->get_preferences().showTipsOnLoading)
											{
												RefCountObjectPtr<Loader::HubScene> loadingTipsScene;
												loadingTipsScene = create_loading_hub_scene(LOC_STR(NAME(lsPleaseWait)));
												if (auto* lsbm = fast_cast<Loader::HubScenes::LoadingMessageSetBrowser>(loadingTipsScene.get()))
												{
													lsbm->allow_early_close(); // because there will be barp afterwards
												}

												// elements of sequence
												Loader::Sequence seq;
												Loader::HubInSequence loadingTipsSeq(useHub, loadingTipsScene.get(), true);
												Loader::HubInSequence barpSeq(useHub, waitingSceneBARP, true);
												Loader::HubInSequence fadeOutSceneSeq(useHub, fadeOutScene.get(), true);

												seq.add(&loadingTipsSeq, NP, [loadingTipsScene]()
													{
														if (loadingTipsScene.get())
														{
															return true;
														}
														else
														{
															return false;
														}
													});
												seq.add(&barpSeq, NP, [waitingSceneBARP]()
													{
														if (waitingSceneBARP && waitingSceneBARP->is_required())
														{
															return true;
														}
														else
														{
															return false;
														}
													});
												seq.add(&fadeOutSceneSeq, NP, [this]() { return get_fade_out() < 1.0f; });

												// do wait
												show_loader_waiting_for_world_manager(&seq,
													[this, pilgrimage]()
													{
														// GameDefinition chosen at setup
														start_mode(new GameModes::PilgrimageSetup(this, pilgrimage, nullptr));
													});
												started = true;
											}
										}
									}
#endif
									if (!started && ! does_want_to_cancel_creation())
									{
										// create pilgrimage
										Loader::Sequence seq;
										Loader::HubInSequence waitingSceneSeq(get_main_hub_loader(), waitingScene.get(), true);
										Loader::HubInSequence fadeOutSceneSeq(useHub, fadeOutScene.get(), true);
										{
											// pass deactivation through as when we hit the deactivation, we want it to stop this sequence element
											seq.add(&waitingSceneSeq, NP, nullptr, true);
										}
										seq.add(&fadeOutSceneSeq, NP, [this]() { return get_fade_out() < 1.0f; });
										show_loader_waiting_for_world_manager(&seq,
											[this, pilgrimage]()
											{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
												output(TXT("start mode: pilgrimage setup (meta state: pilgrimage) (general)"));
#endif
												// GameDefinition chosen at setup
												start_mode(new GameModes::PilgrimageSetup(this, pilgrimage, nullptr));
											});
										started = true;
									}

									// make sure we start with an empty overlay info system
									overlayInfoSystem->force_clear();

									if (started && ! does_want_to_cancel_creation())
									{
										startedPilgrimage = true;
									}
								}
							}

#ifdef AN_LOG_META_STATES
							output(TXT("can we start pilgrimage?"));
							output(TXT(" do we want to cancel creation? %S"), does_want_to_cancel_creation()? TXT("yes, don't start") : TXT("no, good to go"));
							output(TXT(" did we start pilgrimage? %S"), startedPilgrimage ? TXT("yes, good to go") : TXT("no, what should we start?"));
#endif
							if (!does_want_to_cancel_creation() && startedPilgrimage)
							{
								before_pilgrimage_starts_ready();

#ifdef AN_LOG_META_STATES
								output(TXT("pilgrimage started, start advancing the game"));
#endif
								pilgrimageStartedTS.reset();

								// since now default "force instant object creation"
								force_instant_object_creation(false);

								stopNonGameMusicOnWayOut = true;
								break;
							}
							else
							{
#ifdef AN_LOG_META_STATES
								output(TXT("pilgrimage startup failed or cancelled, switch to pilgrimage setup"));
#endif
								// abort mode if still happening
								end_mode(true);
								// ready to clean up
								drop_all_delayed_object_creations();
								force_instant_object_creation(false);
								// cancel world generation
								show_hub_scene_waiting_for_world_manager(create_loading_hub_scene(LOC_STR(NAME(lsPleaseWait)), 0.25f, NP, NP, false /*  no loading tips*/));
								// restore to non canceling
								set_wants_to_cancel_creation(false); // we want to create since now
								// reset last forward, so we show menu at the right place - in front of us
								Loader::Hub::reset_last_forward(); // anything we go to will require different last forward
								if (useQuickGameProfile)
								{
									set_meta_state(GameMetaState::QuickPilgrimageSetup);
								}
								else
								{
									set_meta_state(GameMetaState::PilgrimageSetup);
								}
							}
						}
						else if (metaState == GameMetaState::Tutorial)
						{
							play_loading_music_via_non_game_music_player(3.0f);
#ifdef AN_LOG_META_STATES
							output(TXT("[MS] tutorial"));
							output(TXT("start (already started) tutorial"));
#endif
							an_assert(TutorialSystem::get()->get_tutorial());
							GameStats::use_tutorial();
							if (TutorialSystem::get()->get_tutorial() && TutorialSystem::get()->get_tutorial()->get_tutorial_type() == TutorialType::World)
							{
								TutorialSystem::get()->pause_tutorial();
								show_hub_scene_waiting_for_world_manager(create_loading_hub_scene(LOC_STR(NAME(lsTutorial)) + TXT("~~") + TutorialSystem::get()->get_tutorial()->get_suggested_tutorial_name().get(), 0.25f), get_hub_loader_for_tutorial(),
									[this]()
								{
									GameDefinition::choose(Library::get_current()->get_global_references().get<GameDefinition>(NAME(gameDefinitionTutorial)), Array<GameSubDefinition*>());
									start_mode(new GameModes::TutorialSetup(this, TutorialSystem::get()->get_tutorial()));
								});
								// unpause tutorial after fade in
								stopNonGameMusicOnWayOut = true;
								break;
							}
							else if (TutorialSystem::get()->get_tutorial() && TutorialSystem::get()->get_tutorial()->get_tutorial_type() == TutorialType::Hub)
							{
								stop_non_game_music_player();
								if (auto* scene = TutorialSystem::get()->get_tutorial()->create_hub_scene(get_hub_loader_for_tutorial()))
								{
									ready_to_use_hub_loader(get_hub_loader_for_tutorial());
									TutorialSystem::get()->unpause_tutorial(); // we're about to start!
									show_hub_scene_waiting_for_world_manager(scene, get_hub_loader_for_tutorial());
									// whatever has happened, we will end up with tutorial debriefing
									request_post_game_summary_if_none(PostGameSummary::Interrupted);
									set_meta_state(GameMetaState::TutorialDebriefing);
								}
								else
								{
									ready_to_use_hub_loader();
									error(TXT("could not create hub scene"));
									request_post_game_summary_if_none(PostGameSummary::Interrupted);
									set_meta_state(GameMetaState::TutorialDebriefing);
								}
							}
							else
							{
								stop_non_game_music_player();
								error_stop(TXT("why are we here?"));
								request_post_game_summary_if_none(PostGameSummary::Interrupted);
								set_meta_state(GameMetaState::TutorialDebriefing);
							}
						}
						else if (metaState == GameMetaState::Debriefing ||
								 metaState == GameMetaState::TutorialDebriefing)
						{
							bool fromTutorial = metaState == GameMetaState::TutorialDebriefing;
							bool reachedEnd = showPostGameSummary == PostGameSummary::ReachedEnd;
							bool reachedDemoEnd = showPostGameSummary == PostGameSummary::ReachedDemoEnd;
							// choose right stats to show (in case we're doing tutorial from summary
							if (fromTutorial)
							{
#ifdef AN_LOG_META_STATES
								output(TXT("[MS] tutrial debriefing"));
#endif
								GameStats::use_tutorial();
							}
							else
							{
#ifdef AN_LOG_META_STATES
								output(TXT("[MS] debriefing"));
#endif
								GameStats::use_pilgrimage();
							}
							if (reachedEnd)
							{
								output(TXT("[MS] reached end"));
							}
							else if (reachedDemoEnd)
							{
								output(TXT("[MS] reached demo end"));
							}
							else
							{
								output(TXT("[MS] didn't reach end, other reason to show debriefing"));
							}

							auto* useHubLoader = fromTutorial? get_hub_loader_for_tutorial() : get_main_hub_loader();

#ifdef AN_LOG_META_STATES
REMOVE_AS_SOON_AS_POSSIBLE_	output(TXT("[MS] ready hub"));
#endif
							ready_to_use_hub_loader(useHubLoader);

							auto copy_showPostGameSummary = showPostGameSummary;
							auto copy_debriefingPostGameSummary = debriefingPostGameSummary;

#ifdef AN_LOG_META_STATES
REMOVE_AS_SOON_AS_POSSIBLE_	output(TXT("[MS] request post game summary"));
#endif
							request_post_game_summary(PostGameSummary::None); // we can clear it right now, before we get to saving player profile as we need to show scene for a bit

							if (! fromTutorial)
							{
								// saving is handled when requesting summary
								//show_empty_hub_scene_and_save_player_profile(useHubLoader);
							}

							an_assert(!get_mode());

							if (reachedDemoEnd)
							{
								Loader::Hub::reset_last_forward(); // show summary at where we look at
								Name lsReachedDemoEnd = NAME(lsReachedDemoEnd);
								/*
								if (auto* gd = GameDefinition::get_chosen())
								{
									if (gd->get_type() == GameDefinition::Type::ExperienceRules)
									{
										lsReachedDemoEnd = NAME(lsReachedDemoEndExperience);
									}
								}
								*/
								show_hub_scene_waiting_for_world_manager(new Loader::HubScenes::Message(LOC_STR(lsReachedDemoEnd)), useHubLoader);
								Loader::Hub::reset_last_forward(); // reset for the menu
							}

#ifdef AN_LOG_META_STATES
REMOVE_AS_SOON_AS_POSSIBLE_	output(TXT("[MS] choose scene %c %i"), fromTutorial? 'T' : 'G', fromTutorial? copy_showPostGameSummary : copy_debriefingPostGameSummary);
#endif
							Loader::HubScene* nextScene = nullptr;
							bool usingSummary = false;
							switch (fromTutorial? copy_showPostGameSummary : copy_debriefingPostGameSummary)
							{
							case PostGameSummary::None:
#ifdef AN_LOG_META_STATES
REMOVE_AS_SOON_AS_POSSIBLE_		output(TXT("[MS] please wait"));
#endif
								nextScene = create_loading_hub_scene(LOC_STR(NAME(lsPleaseWait)), 0.25f); // add a delay, so we won't show up for a short moment
								break;
							default:
								if (fromTutorial)
								{
#ifdef AN_LOG_META_STATES
REMOVE_AS_SOON_AS_POSSIBLE_			output(TXT("[MS] tutorial done"));
#endif
									nextScene = new Loader::HubScenes::TutorialDone();
								}
								else
								{
#ifdef AN_LOG_META_STATES
REMOVE_AS_SOON_AS_POSSIBLE_			output(TXT("[MS] summary"));
#endif
									nextScene = new Loader::HubScenes::Summary(copy_debriefingPostGameSummary);
									usingSummary = true;
								}
								break;
							};

#ifdef AN_LOG_META_STATES
							output(TXT("show debriefing scene"));
#endif
							Loader::Hub::reset_last_forward(); // show summary at where we look at
							show_hub_scene_waiting_for_world_manager(nextScene, useHubLoader);
							Loader::Hub::reset_last_forward(); // reset for the menu

#ifdef AN_LOG_META_STATES
							output(TXT("done debriefing scene"));
#endif

							if (reachedEnd)
							{
								bool show = false;
								Name lsReachedEnd = NAME(lsReachedEnd);
								if (is_demo())
								{
									show = true;
									lsReachedEnd = NAME(lsReachedDemoEnd);
								}
								else
								{
									if (auto* gd = GameDefinition::get_chosen())
									{
										if (gd->get_type() == GameDefinition::Type::ExperienceRules)
										{
											show = true;
											lsReachedEnd = NAME(lsReachedEndExperience);
										}
									}
								}
								if (show)
								{
									Loader::Hub::reset_last_forward(); // show summary at where we look at
									show_hub_scene_waiting_for_world_manager(new Loader::HubScenes::Message(LOC_STR(lsReachedEnd)), useHubLoader);
									Loader::Hub::reset_last_forward(); // reset for the menu
								}
							}

							if (fromTutorial || usingSummary)
							{
								// allow for TutorialDone (or summary) to choose the right meta state
							}
							else
							{
								if (metaState != GameMetaState::Tutorial) // allow to start tutorial from summary
								{
									if (useQuickGameProfile)
									{
										set_meta_state(GameMetaState::QuickPilgrimageSetup);
									}
									else
									{
										set_meta_state(GameMetaState::PilgrimageSetup);
									}
								}
							}
						}
						else
						{
							set_meta_state(GameMetaState::JustStarted);
						}
					}

					/*
					if (!skipWaiting)
					{
						// here we always may wait for something (unless we explicitly say we don't)
						show_hub_scene_waiting_for_world_manager(create_loading_hub_scene(LOC_STR(NAME(lsPleaseWait)), 0.25f), recentHubLoader);
					}
					*/

					ready_for_continuous_advancement();

					player.zero_vr_anchor_for_sliding_locomotion();
					playerTakenControl.zero_vr_anchor_for_sliding_locomotion();
				}
			}

			mainHubLoader->allow_fade_out(); // restore normal fading and do empty scene to fade out
			if (profileHubLoader) profileHubLoader->allow_fade_out();
			// too much of this saving. show_empty_hub_scene_and_save_player_profile(recentHubLoader);

			// reset everything for a #demo
			endDemoKeyPressedFor.clear();
			endDemoRequestedFor.clear();
#endif
		}

		// this is to allow hub loader to fade out quickly before we go into proper game
		if (!skipFading && !::System::Core::restartRequired)
		{
			Loader::Hub* fadeOutHubs[] = { mainHubLoader, nullptr/*profileHubLoader*/, nullptr };
			Loader::Hub** hubPtr = fadeOutHubs;
			bool fadeIn = false;
			while ((*hubPtr) != nullptr)
			{
				Loader::Hub* hub = *hubPtr;
				if ((*hub).does_require_fade_out())
				{
					hub->disallow_input(); // only for fading out, no input should be processed
					while (get_fade_out() < 1.0f)
					{
						advance_and_display_loader(hub); // it's deactivated but we want to fade it out
					}
					hub->allow_input();
					(*hub).set_require_fade_out(false);

					fadeIn = true;
				}
				++hubPtr;
			}

			if (stopNonGameMusicOnWayOut)
			{
				stop_non_game_music_player();
			}

			// we're leaving meta states changes
			if (get_fade_out() >= 1.0f)
			{
				reset_immobile_origin_in_vr_space_for_sliding_locomotion();
			}

			if (fadeIn)
			{
				// allow fade in and next thing to kick in
				fade_in(1.0f, NP, 0.1f); // small delay to allow for vignettes to be set, etc
			}
		}

		if (TutorialSystem::get()->is_active() &&
			TutorialSystem::get()->is_tutorial_paused() &&
			get_fade_out() < 0.05f)
		{
			TutorialSystem::get()->unpause_tutorial();
		}
	}
	blockRunTime = false;

	// advance vr anchors
	player.advance_vr_anchor();
	playerTakenControl.advance_vr_anchor();

	// post mode lock for sync jobs
	{
		// not used yet WorldManagerImmediateSyncJobsScopedLock lockImmediateSyncJobs(this);

		if (sceneLayerStack.is_set())
		{
			MEASURE_PERFORMANCE(sceneLayerStackPreAdvance);
			sceneLayerStack->pre_advance(gameAdvanceContext);
		}

#ifdef AN_DEVELOPMENT_OR_PROFILER
		if (lastInRoomPresenceLinksCountRequired)
		{
			Framework::Room* inRoom = nullptr;
			if (auto* actor = player.get_actor())
			{
				inRoom = actor->get_presence()->get_in_room();
			}
			if (debugCameraMode)
			{
				inRoom = debugCameraInRoom.get();
			}
			if (inRoom)
			{
				lastInRoomPresenceLinksCount = inRoom->count_presence_links();
			}
			else
			{
				lastInRoomPresenceLinksCount = 0;
			}
			lastInRoomPresenceLinksCountRequired = false;
		}
#endif

		// update player
		{
#ifdef AN_DEVELOPMENT_OR_PROFILER
			DEFINE_STATIC_NAME(skipStoryScene);
			if (does_debug_control_player() && allTimeControlsInput.has_button_been_pressed(NAME(skipStoryScene)))
			{
				Framework::GameScript::ScriptExecution::trigger_execution_trap(NAME(skipStoryScene));
			}
#endif
			MEASURE_PERFORMANCE(playerUpdate);
			if (auto * playerActor = player.get_actor())
			{
#ifdef AN_DEVELOPMENT_OR_PROFILER
				static float verticalOffset = 0.0f;
				static float targetVerticalOffset = 0.0f;
				{
					DEFINE_STATIC_NAME(debugCrouch);
					DEFINE_STATIC_NAME(debugStandUp);

					float offsets[] = { -1.0f, -0.5f, 0.0f, 0.1f };
					float offsetsVR[] = { -0.4f, -0.2f, 0.0f, 0.15f };
					float const * useOffsets = VR::IVR::get() ? offsetsVR : offsets;
					int count = 4;
					static float crouchHoldFor = 0.0f;
					static float standUpHoldFor = 0.0f;
					float const holdThreshold = 0.3f;
					if (does_debug_control_player() && allTimeControlsInput.is_button_pressed(NAME(debugCrouch)))
					{
						crouchHoldFor += deltaTime;
					}
					else
					{
						if (does_debug_control_player() && crouchHoldFor > 0.0f && crouchHoldFor < holdThreshold)
						{
							for (int i = count - 1; i >= 0; --i)
							{
								if (targetVerticalOffset > useOffsets[i])
								{
									targetVerticalOffset = useOffsets[i];
									break;
								}
							}
						}
						crouchHoldFor = 0.0f;
					}
					if (does_debug_control_player() && allTimeControlsInput.is_button_pressed(NAME(debugStandUp)))
					{
						standUpHoldFor += deltaTime;
					}
					else
					{
						if (does_debug_control_player() && standUpHoldFor > 0.0f && standUpHoldFor < holdThreshold)
						{
							for (int i = 0; i < count; ++i)
							{
								if (targetVerticalOffset < useOffsets[i])
								{
									targetVerticalOffset = useOffsets[i];
									break;
								}
							}
						}
						standUpHoldFor = 0.0f;
					}
					verticalOffset = blend_to_using_time(verticalOffset, targetVerticalOffset, 0.1f, gameAdvanceContext.get_world_delta_time());
					if (VR::IVR::get())
					{
						VR::IVR::get()->set_vertical_offset(verticalOffset);
					}
					playerActor->get_presence()->set_vertical_look_offset(verticalOffset);
				}
#endif
				if (auto * mcp = fast_cast<Framework::ModuleCollisionHeaded>(playerActor->get_collision()))
				{
					mcp->update_collision_primitive_using_head();
				}
			}
		}

		// update post process override params
		if (!VR::IVR::get() || (!VR::IVR::get()->does_use_direct_to_vr_rendering() && VR::IVR::get()->does_use_separate_output_render_target()))
		{
			Framework::Environment* inEnvironment = nullptr;
			{
				Framework::Room* getEnvFromRoom = nullptr;
				if (auto* playerActor = player.get_actor())
				{
					getEnvFromRoom = playerActor->get_presence()->get_in_room();
				}
	#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (debugCameraMode == 1)
				{
					if (debugCameraInRoom.is_set())
					{
						getEnvFromRoom = debugCameraInRoom.get();
					}
				}
	#endif
				if (getEnvFromRoom)
				{
					inEnvironment = getEnvFromRoom->get_environment();
				}
			}
			{
				float const blendTime = 1.5f;
				float deltaTime = get_delta_time();
				for_every(op, overridePostProcessParams)
				{
					op->target.clear();
					if (inEnvironment)
					{
						if (auto* ep = inEnvironment->get_param(op->param))
						{
							op->target = ep->get_as_float();
						}
					}
					float strengthTarget = op->target.is_set() ? 1.0f : 0.0f;
					if (op->target.is_set())
					{
						if (!op->value.is_set())
						{
							op->value = op->target;
						}
						op->value = blend_to_using_time(op->value.get(), op->target.get(), blendTime, deltaTime);
					}
					op->strength = blend_to_using_time(op->strength, strengthTarget, blendTime, deltaTime);
				}
			}
		}
	}
}

void Game::pre_game_loop()
{
	MEASURE_PERFORMANCE_COLOURED(preGameLoop, Colour::orange);

	base::pre_game_loop();

	Framework::GameAdvanceContext gameAdvanceContext;
	setup_game_advance_context(REF_ gameAdvanceContext);

	if (mode.is_set())
	{
		MEASURE_PERFORMANCE(modePreGameLoop);
		mode->pre_game_loop(gameAdvanceContext);
	}

	{
		gameDirector->update(deltaTime);
	}

	// perform extra advances
	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(currentFrameInfo->extraAdvancesTime);
#endif
		do_extra_advances(gameAdvanceContext.get_world_delta_time());
	}
}


void Game::sound()
{
	MEASURE_PERFORMANCE_COLOURED(soundThread, Colour::zxYellowBright);

	Framework::GameAdvanceContext gameAdvanceContext;
	setup_game_advance_context(REF_ gameAdvanceContext);

	preparedSoundScene = false;
	if (::System::Core::is_rendering_paused())
	{
		manage_game_sounds_pause(true);
		manage_non_game_sounds_pause(true);
		no_world_to_sound();
	}
	else
	{
		manage_game_sounds_pause(false);
		manage_non_game_sounds_pause(false);
		if (sceneLayerStack.is_set())
		{
			sceneLayerStack->prepare_to_sound(gameAdvanceContext);
			if (!preparedSoundScene)
			{
				no_world_to_sound();
			}
		}
		else
		{
			if (world && worldInUse)
			{
				prepare_world_to_sound(gameAdvanceContext.get_world_delta_time(), player.get_actor());
			}
			else
			{
				no_world_to_sound();
			}
		}
	}

	// update sound system through base class
	// base will allow soundGate
	base::sound();
}

void Game::advance_controls()
{
	scoped_call_stack_info(TXT("Game::advance_controls"));

	base::advance_controls();

	Framework::GameAdvanceContext gameAdvanceContext;
	setup_game_advance_context(REF_ gameAdvanceContext);

#ifdef INVESTIGATE_MISSING_INPUT
	REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("!@# Game::advance_controls"));
#endif

	// process controls
	{
		bool debugControls = process_debug_controls();
		if (
#ifdef AN_PERFORMANCE_MEASURE
			!showPerformanceAfterFrames.is_set() &&
#endif
			!debugControls && !showDebugPanel && !showDebugPanelVR && !showPerformancePanelVR)
		{
			if (sceneLayerStack.is_set())
			{
				if (!::System::Core::is_paused() &&
					(::System::Input::get()->is_grabbed() || does_use_vr())
#ifdef AN_DEVELOPMENT_OR_PROFILER
					&& !showDebugPanel
					&& !DebugVisualiser::get_active()
#endif
					)
				{
					MEASURE_PERFORMANCE(processControls);
					sceneLayerStack->process_controls(gameAdvanceContext);
				}
				{
					MEASURE_PERFORMANCE(processAllTimeControls);
					sceneLayerStack->process_all_time_controls(gameAdvanceContext);
				}
			}
		}

		if (sceneLayerStack.is_set())
		{
			if (!::System::Core::is_paused())
			{
				MEASURE_PERFORMANCE(processVRControls);
				// to allow to debug vr controls from outside
				sceneLayerStack->process_vr_controls(gameAdvanceContext);
			}
#ifdef INVESTIGATE_MISSING_INPUT
			else
			{
				REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("!@# paused"));
			}
#endif
		}
#ifdef INVESTIGATE_MISSING_INPUT
		else
		{
			REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("!@# no stack"));
		}
#endif
	}
}

float Game::calculate_health_pt_for_effects(CustomModules::Health const* h)
{
	if (h)
	{
		float maxHealth = min(h->get_max_health().as_float(), max(h->get_max_health_not_super_health_storage().as_float(), h->get_max_total_health().as_float() * 0.33f));
		float healthPt = clamp(h->get_health().as_float() / max(0.01f, maxHealth), 0.0f, 1.0f);
		return healthPt;
	}
	else
	{
		return 1.0f;
	}
}

float Game::calculate_sound_dullness_for(Framework::Object const* _usingObjectAsOwner)
{
	float dull = 0.0f;
	if (_usingObjectAsOwner)
	{
		if (auto* h = _usingObjectAsOwner->get_custom<CustomModules::HealthRegen>())
		{
			float healthPt = calculate_health_pt_for_effects(h);
			healthPt = clamp(healthPt * 2.0f, 0.0f, 1.0f); // don't start immediately
			healthPt = lerp(0.5f, healthPt, sqr(healthPt)); // so it's more apparent
			dull = clamp((1.0f - healthPt), 0.0f, 0.3f);
		}
		if (auto* g = Game::get_as<Game>())
		{
			dull = clamp(lerp(g->vignetteForDead.at, dull, max(dull, 1.2f)), 0.0f, 1.0f);

			dull = lerp(g->scriptedSoundDullness.current, dull, 1.0f);
		}
	}
	return dull;
}

float Game::calculate_sound_dullness_for_consider_camera(Framework::Object const* _usingObjectAsOwner)
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if ((debugCameraMode == 1 && debugSoundCamera == 0) || debugSoundCamera == 1)
	{
		return 0.0f;
	}
#endif
	return calculate_sound_dullness_for(_usingObjectAsOwner);
}

void Game::prepare_world_to_sound(float _deltaTime, Framework::Object const * _usingObjectAsOwner)
{
	WorldManagerImmediateSyncJobsScopedLock lockImmediateSyncJobs(this);

	an_assert(worldInUse);

	if (auto* ss = ::System::Sound::get())
	{
		if (vignetteForDead.at > 0.0f)
		{
			ss->set_custom_volume(get_world_channel_tag_condition(), NAME(vignetteForDead), 1.0f - vignetteForDead.at);
		}
		else
		{
			ss->clear_custom_volume(NAME(vignetteForDead));
		}
	}
		
	{
		float dull = calculate_sound_dullness_for_consider_camera(_usingObjectAsOwner);
		if (musicPlayer)
		{
			musicPlayer->set_dull(dull, 0.3f);
		}
		if (environmentPlayer)
		{
			environmentPlayer->set_dull(dull, 0.3f);
		}
	}

	// cameras are used to build scene but afterwards they have to be filled 
	Framework::Sound::Camera camera;

	Transform offset = Transform::identity;

	{
		MEASURE_PERFORMANCE_COLOURED(setupSoundCamera, Colour::zxMagentaBright);

		if (useForcedCamera)
		{
			if (auto* imo = forcedCameraAttachedTo.get())
			{
				camera.set_placement(forcedCameraInRoom.get(), imo->get_presence()->get_placement().to_world(forcedCameraPlacement));
			}
			else
			{
				camera.set_placement(forcedCameraInRoom.get(), forcedCameraPlacement);
			}
			camera.set_owner(nullptr); // pretend we're looking from outside
			_usingObjectAsOwner = nullptr;
		}
		else if (_usingObjectAsOwner)
		{
			camera.set_placement_based_on(_usingObjectAsOwner);
			todo_note(TXT("do not offset placement as it breaks presence paths! create sound presence that would handle that, add ::Framework::CameraPlacementBasedOn::Ears"));
			todo_hack(TXT("hack to avoid problems mentioned above"));
			camera.set_additional_offset(_usingObjectAsOwner->get_presence()->get_centre_of_presence_os().to_local(_usingObjectAsOwner->get_presence()->get_eyes_relative_look()));
		}
		else
		{
			camera.clear_placement();
		}
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_ALLOW_BULLSHOTS
		Framework::Room* soundCamRoom = nullptr;
		Transform soundCamPlacement;
		if (Framework::BullshotSystem::does_want_to_draw() &&
			Framework::BullshotSystem::get_sound_camera(soundCamRoom, soundCamPlacement))
		{
			camera.set_placement(soundCamRoom, soundCamPlacement);
			camera.set_owner(nullptr); // pretend we're looking from outside
			_usingObjectAsOwner = nullptr;
		}
		else
#endif
		if (debugSoundCamera == 0 && useForcedSoundCamera)
		{
			camera.set_placement(forcedSoundCameraInRoom.get(), forcedSoundCameraPlacement);
			camera.set_owner(nullptr); // pretend we're looking from outside
			_usingObjectAsOwner = nullptr;
		}
		else if (debugSoundCamera)
		{
			if (!debugSoundCameraPlacement.is_set() || !debugSoundCameraInRoom.get())
			{
				debugSoundCameraPlacement = debugCameraPlacement;
				debugSoundCameraInRoom = camera.get_in_room();
			}
			camera.set_placement(debugSoundCameraInRoom.get(), debugSoundCameraPlacement.get());
			camera.set_owner(nullptr); // pretend we're looking from outside
			_usingObjectAsOwner = nullptr;
		}
		else
		{
			debugSoundCameraPlacement.clear();
			debugSoundCameraInRoom.clear();
			if ((debugCameraMode == 1 && debugSoundCamera == 0) || debugSoundCamera == 1)
			{
				if (!debugCameraInRoom.is_set())
				{
					debugCameraPlacement = camera.get_placement();
					debugCameraInRoom = camera.get_in_room();
				}
				camera.set_placement(debugCameraInRoom.get(), debugCameraPlacement);
				camera.set_owner(nullptr); // pretend we're looking from outside
				_usingObjectAsOwner = nullptr;
			}
		}
#endif
		camera.offset_placement(get_current_camera_rumble_placement());
	}

	prepare_world_to_sound(_deltaTime, camera, _usingObjectAsOwner);
}

void Game::prepare_world_to_sound(float _deltaTime, Framework::Sound::Camera const & _usingCamera, Framework::Object const * _usingObjectAsOwner)
{
	Framework::Sound::SceneUpdateContext suc;
	suc.dull = calculate_sound_dullness_for_consider_camera(_usingObjectAsOwner);
	soundScene->update(_deltaTime, _usingCamera, _usingObjectAsOwner ? _usingObjectAsOwner->get_presence() : nullptr, suc);
	preparedSoundScene = true;
}

void Game::end_all_sounds()
{
	no_world_to_sound();

	if (musicPlayer)
	{
		musicPlayer->stop(0.001f);
	}
	if (nonGameMusicPlayer)
	{
		nonGameMusicPlayer->stop(0.001f);
	}
	if (voiceoverSystem)
	{
		voiceoverSystem->reset();
	}
}


void Game::no_world_to_sound()
{
	soundScene->clear();

	if (environmentPlayer)
	{
		environmentPlayer->clear();
	}
}

float Game::get_time_to_next_end_render() const
{
	if (auto * vr = VR::IVR::get())
	{
		/*
		Optional<float> timeToNextRender = vr->get_time_to_next_render();
		if (timeToNextRender.is_set())
		{
			return timeToNextRender.get();
		}
		*/
	}
	float expectedFrameTime = System::Video3D::get()->get_ideal_expected_frame_time();
	if (expectedFrameTime == 0.0f)
	{
		expectedFrameTime = lastFrameTimeRenderToRender * 0.8f; // allow to speed up things
		if (expectedFrameTime > 0.01f) expectedFrameTime = 0.01f;
	}
	float timeLeft = max(0.0f, expectedFrameTime - max(0.0f, endedRenderTS.get_time_since()));
	return timeLeft;
}

#ifdef AN_DEVELOPMENT_OR_PROFILER
void Game::mark_start_end_render()
{
	currentFrameInfo->startEndRenderSinceLastEndRender = endedRenderTS.get_time_since();
}
#endif

void Game::mark_render_start()
{
#ifdef AN_USE_FRAME_INFO
	currentFrameInfo->lastRenderToStartRenderTime = endedRenderTS.get_time_since();
#endif
}

void Game::mark_render_pushed_do_jobs()
{
#ifdef AN_USE_FRAME_INFO
	currentFrameInfo->lastRenderToDoJobsOnRenderTime = endedRenderTS.get_time_since();
#endif
}

void Game::mark_pre_ended_render()
{
#ifdef AN_USE_FRAME_INFO
	currentFrameInfo->lastRenderToPreEndRenderTime = endedRenderTS.get_time_since();
#endif
}

void Game::mark_render_end()
{
	lastFrameTimeRenderToRender = endedRenderTS.get_time_since();
#ifdef AN_USE_FRAME_INFO
	currentFrameInfo->lastRenderToRenderTime = lastFrameTimeRenderToRender;
#endif
	endedRenderTS.reset();
}

void Game::render()
{
	assert_rendering_thread();

#ifdef AN_USE_FRAME_INFO
	System::TimeStamp renderTime_part1_ts;
#endif

	mark_render_start();

	ScreenShotContext screenShotContext(this);

	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(currentFrameInfo->duringRenderPart1_screenShotContextPrepare);
#endif
		MEASURE_PERFORMANCE(screenShotContext_pre_render);
		screenShotContext.pre_render();
	}

	MEASURE_PERFORMANCE(renderThread);

	// we haven't rendered yet
	debug_renderer_not_yet_rendered();

	bool fadingHandled;
	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(currentFrameInfo->duringRenderPart1_fading);
#endif
		fadingHandled = apply_prerender_fading();
	}
	
	// various vignettes and other things
	{
		// taking controls
		{
#ifdef AN_USE_FRAME_INFO
			System::ScopedTimeStamp ts(currentFrameInfo->duringRenderPart1_takingControls);
#endif
			float useTakingControls = clamp(takingControls.at, -1.0f, 1.0f);

			bool shouldUseTakenControl = playerTakenControl.should_see_through_my_eyes();
			Player& usePlayer = shouldUseTakenControl ? playerTakenControl : player;
			if (auto* a = usePlayer.get_actor())
			{
				if (auto* tcAppearance = a->get_appearance_named(NAME(takingControls)))
				{
					tcAppearance->access_mesh_instance().set_rendering_order_priority(AppearanceRenderingPriority::TakingControls); // render as last, top most
					if (useTakingControls == 0.0f)
					{
						tcAppearance->be_visible(false);
					}
					else
					{
						tcAppearance->be_visible(true);

						auto& meshInstance = tcAppearance->access_mesh_instance();
						auto* materialInstance = meshInstance.access_material_instance(0);

						float startOffset = -0.1f;
						float length = 1.2f;
						materialInstance->set_uniform(NAME(takingControlsBackAt), clamp(startOffset - length * useTakingControls, -1.5f, 1.5f));
						materialInstance->set_uniform(NAME(takingControlsFrontAt), clamp(1.0f - startOffset - length * useTakingControls, -1.5f, 1.5f));
					}
				}
			}
		}

		// vignette for dead
		{
#ifdef AN_USE_FRAME_INFO
			System::ScopedTimeStamp ts(currentFrameInfo->duringRenderPart1_vignetteForDead);
#endif
			float useVignetteForDead = BlendCurve::cubic(vignetteForDead.at);
#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (debugCameraMode != 0)
			{
				useVignetteForDead = 0.0f;
			}
#endif

			/*
			{
				// linear test
				static float t = 0.0f;
				static float dt = 1.0f;
				t = clamp(t + dt * System::Core::get_delta_time(), 0.0f, 1.0f);
				if (t >= 1.0f)
				{
					dt = -1.0f;
				}
				if (t <= 0.0f)
				{
					dt = 1.0f;
				}
				useVignetteForDead = t;
			}
			*/

			bool shouldUseTakenControl = playerTakenControl.should_see_through_my_eyes();
			Player& usePlayer = shouldUseTakenControl ? playerTakenControl : player;
			if (auto* a = usePlayer.get_actor())
			{
				if (auto* vfmAppearance = a->get_appearance_named(NAME(vignetteForDead)))
				{
					vfmAppearance->access_mesh_instance().set_rendering_order_priority(AppearanceRenderingPriority::VignetteForDead); // render as last, top most
					if (useVignetteForDead == 0.0f)
					{
						vfmAppearance->be_visible(false);
					}
					else
					{
						vfmAppearance->be_visible(true);

						auto& meshInstance = vfmAppearance->access_mesh_instance();
						auto* materialInstance = meshInstance.access_material_instance(0);

						/*
						 *
						 *      ^
						 *   90 |             @
						 *      |            /:
						 *      |           / :
						 *      |          /  :
						 *      |         /   :
						 *      |      @ /    :
						 *      |      f/     :			f is half feather angle
						 *    0 +------@------:--------->
						 *      |     /:      :
						 *      |    / :      :
						 *      |   /  :      :
						 *      |  /   :      :
						 *      | /    :      :
						 *      |/     :      :
						 *  -90 @      :      :
						 *      :      :      :
						 *     0.0    0.5    1.0
						 *
						 */
						float featherAngle = 45.0f;
						float boundary = featherAngle / 180.0f; // in <boundary, 1-boundary> distance should as feather
						float av = (90.0f - featherAngle * 0.5f) / 0.5f;

						float frontAngle = featherAngle * 0.5f + av * (useVignetteForDead - 0.5f);
						float backAngle = frontAngle - featherAngle;

						if (useVignetteForDead < boundary)
						{
							float frontAngleAtBoundary = featherAngle * 0.5f + av * (boundary - 0.5f);
							frontAngle = lerp(useVignetteForDead / boundary, -90.0f, frontAngleAtBoundary);
						}
						if (useVignetteForDead > 1.0f - boundary)
						{
							float backAngleAtBoundary = -featherAngle * 0.5f + av * ((1.0f - boundary) - 0.5f);
							backAngle = lerp((1.0f - useVignetteForDead) / boundary, 90.0f, backAngleAtBoundary);
						}

						float frontSin = sin_deg(max(-90.0f, frontAngle));
						frontSin = frontAngle > 90.0f ? 2.0f - frontSin : frontSin;
					
						float backSin = sin_deg(min(90.0f, backAngle));
						backSin = backAngle > 90.0f ? -2.0f + backSin : backSin;

						if (frontSin > 0.0f)
						{
							frontSin = max(frontSin, backSin + 0.005f);
						}
						materialInstance->set_uniform(NAME(vignetteForDeadBackAt), backSin);
						materialInstance->set_uniform(NAME(vignetteForDeadFrontAt), frontSin);

						Matrix33 orientation = Matrix33::identity;
						if (auto* appearance = a->get_appearance())
						{
							orientation = appearance->calculate_socket_os(NAME(vignetteForDeadOrientation)).get_orientation().to_matrix_33();
						}
						materialInstance->set_uniform(NAME(vignetteForDeadOrientation), orientation);
					}
				}
			}
		}
		// vignette for movement
		{
#ifdef AN_USE_FRAME_INFO
			System::ScopedTimeStamp ts(currentFrameInfo->duringRenderPart1_vignetteForMovement);
#endif
			float useVignetteForMovement = vignetteForMovementAmount;
#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (debugCameraMode != 0)
			{
				useVignetteForMovement = 0.0f;
			}
#endif
			if (cleanScreenShots && ScreenShotContext::is_doing_screen_shot())
			{
				useVignetteForMovement = 0.0f;
			}

			bool shouldUseTakenControl = playerTakenControl.should_see_through_my_eyes();
			Player& usePlayer = shouldUseTakenControl ? playerTakenControl : player;
			if (auto* a = usePlayer.get_actor())
			{
				if (auto* vfmAppearance = a->get_appearance_named(NAME(vignetteForMovement)))
				{
					vfmAppearance->access_mesh_instance().set_rendering_order_priority(AppearanceRenderingPriority::VignetteForMovement); // render as last, top most
					if (useVignetteForMovement == 0.0f)
					{
						vfmAppearance->be_visible(false);
					}
					else
					{
						vfmAppearance->be_visible(true);

						auto& meshInstance = vfmAppearance->access_mesh_instance();
						auto* materialInstance = meshInstance.access_material_instance(0);

						/*
						// linear test
						static float t = 0.0f;
						static float dt = 1.0f;
						t = clamp(t + dt * System::Core::get_delta_time(), 0.0f, 1.0f);
						if (t >= 1.0f)
						{
							dt = -1.0f;
						}
						if (t <= 0.0f)
						{
							dt = 1.0f;
						}
						useVignetteForMovement = t;
						vignetteForMovementActive = 1.0f;
						*/

						auto const& psp = PlayerSetup::get_current().get_preferences();

						float minAngle = 85.0f;
						float maxAngle = 30.0f;
						float featherAngle = 10.0f;
						maxAngle = minAngle + (maxAngle - minAngle) * (0.15f + 0.85f * psp.vignetteForMovementStrength);
						minAngle = 180.0f * (1.0f - vignetteForMovementActive) + minAngle * vignetteForMovementActive;
						float useAngle = minAngle + (maxAngle - minAngle) * useVignetteForMovement;
						materialInstance->set_uniform(NAME(vignetteForMovementDot), cos_deg(useAngle * 0.5f));
						materialInstance->set_uniform(NAME(vignetteForMovementFeather), cos_deg(useAngle * 0.5f) - cos_deg(useAngle * 0.5f + featherAngle));
					}
				}
			}
		}
	}

	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(currentFrameInfo->prepareToRender);
#endif
		preparedRenderScenes = false;
		if (::System::Core::is_rendering_paused())
		{
			no_world_to_render();
		}
		else
		{
			if (sceneLayerStack.is_set())
			{
				sceneLayerStack->prepare_to_render(screenShotContext.useCustomRenderContext);
				if (!preparedRenderScenes)
				{
					no_world_to_render();
				}
			}
			else
			{
				if (world && worldInUse)
				{
					prepare_world_to_render(player.get_actor(), screenShotContext.useCustomRenderContext);
				}
				else
				{
					no_world_to_render();
				}
			}
		}
	}

	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(currentFrameInfo->duringRenderPart1_allowGate);
#endif
		// mark rendering unblocked/allowed and wake up waiting for pre render sync
		render_block_gate_allow_one();
	}

#ifdef AN_USE_FRAME_INFO
	currentFrameInfo->renderTime_part1 = renderTime_part1_ts.get_time_since();
	System::TimeStamp renderTime_part2_ts;
	System::TimeStamp renderTime_part3_ts;
	bool renderTime_part2_done = false;
#endif

	bool renderingAllowed = true;

	{	// display buffer
		MEASURE_PERFORMANCE(game_displayBuffer);
		System::Video3D * v3d = System::Video3D::get();

		// update
		{
			MEASURE_PERFORMANCE_COLOURED(videoUpdate, Colour::blue);
#ifdef AN_USE_FRAME_INFO
			System::ScopedTimeStamp ts(currentFrameInfo->duringRenderPart2_videoUpdate);
#endif
			v3d->update();
		}

		{
			MEASURE_PERFORMANCE_COLOURED(displayBufferIfReady, Colour::zxRed);
			// display buffer now, this way we gain on time when we ready scene to be rendered
			if (v3d->is_buffer_ready_to_display())
			{
				mark_render_pushed_do_jobs();

				if (!does_use_vr() ||
					VR::IVR::get()->is_render_open())
				{
					MEASURE_PERFORMANCE_COLOURED(checkImmediateJobsWhileWaiting, Colour::c64Violet);
#ifdef AN_USE_FRAME_INFO
					::System::ScopedTimeStamp doImmediateJobs(currentFrameInfo->immediateJobsPostRenderTime);
#endif
					float const timeReserved = MainConfig::global().get_time_reserved_for_submitting_render_frame();
					if (timeReserved > 0)
					{
						do_immediate_jobs_while_waiting(get_time_to_next_end_render() - timeReserved);
					}
				}

#ifdef AN_USE_FRAME_INFO
				currentFrameInfo->renderTime_part2 = renderTime_part2_ts.get_time_since();
				renderTime_part2_done = true;
				renderTime_part3_ts.reset();
#endif

				{
					MEASURE_PERFORMANCE_COLOURED(endFrame, Colour::zxMagenta);
#ifdef AN_USE_FRAME_INFO
					System::ScopedTimeStamp ts(currentFrameInfo->duringRenderPart3_endAndDisplay);
#endif

					mark_pre_ended_render();
					if (does_use_vr() && VR::IVR::get()->is_render_open())
					{
#ifdef AN_DEVELOPMENT_OR_PROFILER
						mark_start_end_render();
#endif
						MEASURE_PERFORMANCE(vr_endRender_midFrame);
						VR::IVR::get()->end_render(v3d);
						mark_render_end();
					}
					{
						MEASURE_PERFORMANCE(v3d_displayBufferIfReady);
						v3d->display_buffer_if_ready();
					}
					if (!does_use_vr())
					{
						mark_render_end();
					}
				}
			}
		}

		if (does_use_vr())
		{
#ifdef AN_USE_FRAME_INFO
			System::ScopedTimeStamp ts(currentFrameInfo->duringRenderPart3_beginRender);
#endif
			MEASURE_PERFORMANCE(game_vr_beginRender);
			renderingAllowed = VR::IVR::get()->begin_render(v3d);
		}
	}

#ifdef AN_USE_FRAME_INFO
	if (!renderTime_part2_done)
	{
		currentFrameInfo->renderTime_part2 = renderTime_part2_ts.get_time_since();
	}
	currentFrameInfo->renderTime_part3 = renderTime_part3_ts.get_time_since();
	System::TimeStamp renderTime_part4_ts;
#endif

	if (renderingAllowed)
	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(currentFrameInfo->actualRenderTime);
#endif
#ifdef AN_PERFORMANCE_MEASURE
		if (! noRenderUntilShowPerformance)
#endif
		{
#ifdef AN_USE_FRAME_INFO
			System::ScopedTimeStamp ts(currentFrameInfo->actualRenderWorldTime);
#endif
			MEASURE_PERFORMANCE(game_renderPrepared);
			if (sceneLayerStack.is_set())
			{
				sceneLayerStack->render_on_render(screenShotContext.useCustomRenderContext);
			}
			else
			{
				render_prepared_world_on_render(screenShotContext.useCustomRenderContext);
			}
		}

		::System::RenderTarget::unbind_to_none();

		if (screenShotContext.useCustomRenderContext)
		{
			MEASURE_PERFORMANCE(postRender_screenShot);
			render_post_process(screenShotContext.useCustomRenderContext);
			if (System::RenderTarget* rt = screenShotContext.useCustomRenderContext->sceneRenderTarget.get())
			{
				auto* v3d = ::System::Video3D::get();

				rt->resolve_forced_unbind();

				screenShotContext.useCustomRenderContext->finalRenderTarget->bind();

				setup_to_render_to_render_target(v3d, screenShotContext.useCustomRenderContext->finalRenderTarget.get());

				rt->render(0, v3d, Vector2::zero, v3d->get_viewport_size().to_vector2());

#ifdef AN_ALLOW_BULLSHOTS
				if (Framework::BullshotSystem::does_want_to_draw())
				{
					Framework::BullshotSystem::render_logos_and_titles(screenShotContext.useCustomRenderContext->finalRenderTarget.get());
				}
#endif
				System::RenderTarget::unbind_to_none();
			}
		}
		else if ((! does_use_vr() ||
				 (!VR::IVR::get()->does_use_direct_to_vr_rendering() &&
				  VR::IVR::get()->does_use_separate_output_render_target()))
				&& ::System::VideoConfig::general_is_post_process_allowed())
		{
			MEASURE_PERFORMANCE(postRender);

			// post process is common after rendering
			render_post_process(screenShotContext.useCustomRenderContext);

			render_bullshot_logos_on_main_render_output(screenShotContext.useCustomRenderContext);
		}
		else
		{
			//MEASURE_PERFORMANCE(postRender_direct);
		}

		// just to allow for screen shot context
		screenShotContext.post_render();

		System::RenderTarget::unbind_to_none();

		{
			game_icon_render_ui_on_main_render_output();
			render_bullshot_frame_on_main_render_output();
		}

		if (!VR::IVR::get() || (!VR::IVR::get()->does_use_direct_to_vr_rendering()/* && VR::IVR::get()->does_use_separate_output_render_target()*/))
		{
			make_pilgrim_info_to_render_visible(true);
			render_main_render_output_to_output();
			make_pilgrim_info_to_render_visible(false);
		}

		if (fading.should_be_applied())
		{
			MEASURE_PERFORMANCE(applyFading);

			if (!fadingHandled || fadeOutAndQuit) // for fade out and quit always apply normal fading
			{
				if (does_use_vr())
				{
					for (int rtIdx = 0; rtIdx < VR::Eye::Count; ++rtIdx)
					{
						if (System::RenderTarget* rt = VR::IVR::get()->get_output_render_target((VR::Eye::Type)rtIdx))
						{
							apply_fading_to(rt);
						}
					}
				}
			}

			// always apply fading on the main buffer - fade everything!
			apply_fading_to(nullptr);
		}

		store_recent_capture(has_faded_in() /* game and fully faded in */);
	}
	else
	{
		// render last output on screen
		if (does_use_vr())
		{
#ifdef SHOW_NOTHING_ON_DISALLOWED_VR_RENDER
			// clear main, render nothing
			render_nothing_to_output();
#else
			// render what was there
			make_pilgrim_info_to_render_visible(true);
			render_main_render_output_to_output();
			make_pilgrim_info_to_render_visible(false);
#endif
		}
		else
		{
			an_assert(false, TXT("disallowed rendering for non vr? how come?"));
		}
	}

	temp_render_ui_on_output(false);

#ifdef AN_USE_FRAME_INFO
	currentFrameInfo->renderTime_part4 = renderTime_part4_ts.get_time_since();
	System::TimeStamp renderTime_part5_ts;
#endif

	if (does_use_vr() && ! VR::IVR::get()->is_end_render_blocking())
	{
		// end earlier if ending is not blocking
#ifdef AN_DEVELOPMENT_OR_PROFILER
		mark_start_end_render();
#endif
		System::Video3D* v3d = System::Video3D::get();
		MEASURE_PERFORMANCE(vr_endRender_earlyEnd);
		VR::IVR::get()->end_render(v3d);
		mark_render_end();
	}

	// post rendering, load one mesh3d part
	{
		MEASURE_PERFORMANCE_COLOURED(loadOneMesh3DPart, Colour::c64Grey3);
		// load one part
		Meshes::Mesh3DPart::load_one_part_to_gpu();
	}

	{
		MEASURE_PERFORMANCE_COLOURED(recentCaptureStorePixelData, Colour::zxYellow);
		System::RecentCapture::store_pixel_data(); // do it just before ending render as we will get stuck on CPU
	}

#ifdef AN_USE_FRAME_INFO
	currentFrameInfo->renderTime_part5 = renderTime_part5_ts.get_time_since();
#endif
}

void Game::do_immediate_jobs_while_waiting(float _timeLeft)
{
	if (_timeLeft > 0.0f)
	{
		scoped_call_stack_info(TXT("do immediate jobs while waiting"));
		assert_rendering_thread();
		MEASURE_PERFORMANCE_COLOURED(executeWorkerGameLoopJobsWhileWaitingForRender, Colour::c64Violet);
		get_job_system()->execute_immediate_jobs(workerJobs, _timeLeft, [this]() {return gameLoopJobStatus == Framework::GameJob::Done; });
	}
}

void Game::store_recent_capture(bool _fullInGame)
{
	if (does_use_vr())
	{
		if (System::RenderTarget* rt = VR::IVR::get()->get_output_render_target(VR::Eye::OnMain))
		{
			System::RecentCapture::store(rt, _fullInGame);
		}
	}
	else
	{
		if (System::RenderTarget* rt = get_main_render_buffer())
		{
			System::RecentCapture::store(rt, _fullInGame);
		}
	}
}

void Game::prepare_world_to_render(Framework::Object const * _usingObjectAsOwner, Framework::CustomRenderContext * _customRC)
{
	an_assert(worldInUse);

	ArtDir::set_global_tint(); // reset/clear
	ArtDir::set_global_desaturate(); // reset/clear
	if (_usingObjectAsOwner
		&& (!cleanScreenShots || !ScreenShotContext::is_doing_screen_shot())
		)
	{
		// if almost dead, show full colours
		float applyHealthTint = clamp(1.0f - vignetteForDead.at * 2.0f, 0.0f, 1.0f);
		//float applyDueToFadeOut = 1.0f; // always at full -get_fade_out();
		{
			// input
			Colour healthGlobalTint = Colour::none;
			Colour penetrationColour = Colour::black;
#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (debugCameraMode == 0)
#endif
			if (auto* h = _usingObjectAsOwner->get_custom<CustomModules::Health>())
			{
				healthGlobalTint = h->get_global_tint();
			}

			// modify input
			healthGlobalTint = Colour::lerp_smart(applyHealthTint, Colour::none.with_alpha(0.0f), healthGlobalTint);

			// mix
			Colour globalTint = healthGlobalTint;

			globalTint = Colour::lerp_smart(scriptedTint.active, globalTint, scriptedTint.current);

			// apply penetration at the end using lerp_smart to mix rgb colour properly
			float applyPenetration = sqr(keepInWorldPenetrationPt);
			REMOVE_AS_SOON_AS_POSSIBLE_ // change to 1.0?
			globalTint = Colour::lerp_smart(applyPenetration * 0.6f, globalTint, penetrationColour);

			// set
			ArtDir::set_global_tint(globalTint/*.with_alpha(applyDueToFadeOut)*/);
		}
		{
			Colour desaturate = Colour::none;
			Colour desaturateChangeInto = Colour::none;
#ifdef AN_DEVELOPMENT_OR_PROFILER
			if (debugCameraMode == 0)
#endif
			if (auto* hi = _usingObjectAsOwner->get_custom<CustomModules::HitIndicator>())
			{
				desaturate = (hi->get_global_desaturate()).with_alpha(applyHealthTint)/*.with_alpha(applyDueToFadeOut)*/;
				desaturateChangeInto = (hi->get_global_desaturate_change_into()).with_alpha(applyHealthTint)/*.with_alpha(applyDueToFadeOut)*/;
			}
			desaturate = Colour::lerp_smart((1.0f - scriptedTint.active), scriptedTint.current.with_alpha(scriptedTint.active), desaturate);
			desaturateChangeInto = Colour::lerp_smart((1.0f - scriptedTint.active), scriptedTint.current.with_alpha(scriptedTint.active), desaturateChangeInto);

			// set
			ArtDir::set_global_desaturate(desaturate, desaturateChangeInto);
		}
	}

	// cameras are used to build scene but afterwards they have to be filled 
	Framework::Render::Camera camera;

	{
		MEASURE_PERFORMANCE_COLOURED(setupRenderCamera, Colour::zxMagentaBright);
		static float fov = 80.0f;

		camera.set_fov(fov);
#ifndef AN_CLANG
		camera.set_fov(nonVRCameraFov);
#endif

		if (useForcedCamera)
		{
			if (auto* imo = forcedCameraAttachedTo.get())
			{
				camera.set_placement(forcedCameraInRoom.get(), imo->get_presence()->get_placement().to_world(forcedCameraPlacement));
			}
			else
			{
				camera.set_placement(forcedCameraInRoom.get(), forcedCameraPlacement);
			}
			camera.set_owner(nullptr); // pretend we're looking from outside
			_usingObjectAsOwner = nullptr;
			if (!debugCameraMode)
			{
				camera.set_fov(forcedCameraFOV);
			}
		}
		else if (_usingObjectAsOwner)
		{
			camera.set_placement_based_on(_usingObjectAsOwner, ::Framework::CameraPlacementBasedOn::Eyes);
		}
		else
		{
			camera.clear_placement();
		}

		camera.offset_placement(get_current_camera_rumble_placement());
	}

	if (!_customRC)
	{
		recentlyRendered.usingObjectAsOwner = _usingObjectAsOwner;
		recentlyRendered.wasRecentlyRendered = true;
	}
	prepare_world_to_render(camera, _customRC, false); // vr offset will be applied through controller
}

void Game::prepare_world_to_render(Framework::Render::Camera const & _usingCamera, Framework::CustomRenderContext* _customRC)
{
	ArtDir::set_global_tint(); // reset/clear
	if (!_customRC)
	{
		recentlyRendered.usingObjectAsOwner = nullptr;
		recentlyRendered.wasRecentlyRendered = true;
	}
	prepare_world_to_render(_usingCamera, _customRC, true);
}

void Game::prepare_world_to_render_as_recently(Framework::CustomRenderContext* _customRC, Optional<bool> _allowVROffset)
{
	if (recentlyRendered.usingObjectAsOwner)
	{
		prepare_world_to_render(recentlyRendered.usingObjectAsOwner, _customRC);
	}
	else
	{
		prepare_world_to_render(recentlyRendered.usingCamera, _customRC,
			_allowVROffset.is_set() ? _allowVROffset.get() : recentlyRendered.allowVROffset);
	}
}

void Game::prepare_world_to_render(Framework::Render::Camera const & _usingCamera, Framework::CustomRenderContext* _customRC, bool _allowVROffset)
{
	WorldManagerImmediateSyncJobsScopedLock lockImmediateSyncJobs(this);

	if (!_customRC)
	{
		recentlyRendered.usingCamera = _usingCamera;
		recentlyRendered.allowVROffset = _allowVROffset;
	}

	an_assert(!_customRC || _customRC->sceneRenderTarget.is_set());
	System::RenderTarget* rtTarget = _customRC ? _customRC->sceneRenderTarget.get() : get_main_render_buffer();
	Framework::Render::Camera camera = _usingCamera;

	// apply VR to static camera
	if (VR::IVR::can_be_used() &&
		VR::IVR::get()->is_render_view_valid() &&
		VR::IVR::get()->is_render_base_valid())
	{
		if (_allowVROffset)
		{
			Transform localVR = VR::IVR::get()->get_render_view_to_base_full();
			localVR.set_translation(localVR.get_translation());
			camera.offset_placement(localVR);
		}
	}

	VectorInt2 useViewOrigin = VectorInt2::zero;
	VectorInt2 useViewSize = rtTarget->get_size();
	VectorInt2 useViewSizeWithAspectRatioCoef = rtTarget->get_full_size_for_aspect_ratio_coef();
	Vector2 useViewCentreOffset = camera.get_view_centre_offset();
	if (_customRC && _customRC->relViewCentreOffsetForCamera.is_set())
	{
		useViewCentreOffset += _customRC->relViewCentreOffsetForCamera.get();
	}

	camera.set_near_far_plane(Game::s_renderingNearZ, Game::s_renderingFarZ);
#ifdef AN_ALLOW_BULLSHOTS
	camera.set_near_far_plane(Framework::BullshotSystem::get_near_z().get(Game::s_renderingNearZ), Game::s_renderingFarZ);
#endif
	camera.set_background_near_far_plane();
	camera.set_view_origin(useViewOrigin);
	camera.set_view_size(useViewSize);
	camera.set_view_aspect_ratio(aspect_ratio(useViewSizeWithAspectRatioCoef));
	camera.set_view_centre_offset(useViewCentreOffset);
#ifdef AN_ALLOW_BULLSHOTS
	if (Framework::BullshotSystem::is_active())
	{
		Optional<Vector2> renderCentreOffsetPt = Framework::BullshotSystem::get_show_frame_render_centre_offset_pt();
		if (renderCentreOffsetPt.is_set())
		{
			camera.set_view_centre_offset(renderCentreOffsetPt.get() * 2.0f); // to go from -0.5 to 0.5 into -1 to 1
		}
	}
#endif

	if (get_taking_controls_at() != 0.0f)
	{
		// deactivate all when we're switching
		overlayInfoSystem->force_deactivate_all();
	}

	if (!useForcedCamera)
	{
		overlayInfoSystem->advance(OverlayInfo::Usage::World, camera.get_vr_anchor().to_local(camera.get_placement()), player.get_actor());
	}

	bool debugRenderScene = false;
#ifdef AN_DEVELOPMENT_OR_PROFILER
	debug_renderer_rendering_offset_reset();
	if (debugCameraMode && 
		resetDebugCamera)
	{
		resetDebugCamera = false;
		debugCameraPlacement = camera.get_placement();
		debugCameraInRoom = camera.get_in_room();
		debugRenderScene = true;
	}
	if (debugCameraMode == 1)
	{
		if (! debugCameraInRoom.is_set())
		{
			debugCameraPlacement = camera.get_placement();
			debugCameraInRoom = camera.get_in_room();
		}
		camera.set_placement(debugCameraInRoom.get(), debugCameraPlacement);
		camera.set_owner(nullptr); // pretend we're looking from outside
		debugRenderScene = true;
	}
	if (debugCameraMode == 2)
	{
#ifdef AN_DEBUG_RENDERER
		debug_renderer_rendering_offset() = camera.get_placement().to_local(debugCameraPlacement);
#endif
		debugRenderScene = true;
	}
#else
	debugCameraMode = 0;
#endif

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	Framework::Render::Camera cameraExternal;
	if (!get_secondary_view_render_buffer())
	{
		useExternalRenderScene = false;
		onlyExternalRenderScene = false;
	}
	if (useExternalRenderScene || onlyExternalRenderScene)
	{
		cameraExternal = camera;
		cameraExternal.set_owner(nullptr); // we're looking from outside
	}
#endif

	Array<RefCountObjectPtr<Framework::Render::Scene> > & useRenderScenes = _customRC ? _customRC->renderScenes : renderScenes;
	useRenderScenes.clear();
	
	{
#ifdef AN_USE_FRAME_INFO
		System::ScopedTimeStamp ts(currentFrameInfo->buildRenderScenesTime);
#endif
		bool changeToRotationOnly = false;
		if (auto* imo = camera.get_owner())
		{
			changeToRotationOnly = imo->get_presence()->do_eyes_have_fixed_location();
		}
		MEASURE_PERFORMANCE_COLOURED(buildRenderScenes, Colour::zxYellow);
		if (!_customRC && does_use_vr())
		{
			bool parallel = true;
#ifdef AN_DEVELOPMENT
			parallel = false;
#endif
			if (parallel)
			{
				// this runs on separate immediate jobs queue
				struct BuildRenderScenes
				: public PooledObject<BuildRenderScenes>
				{
					Game* game;
					Framework::Render::Camera& camera;
					Framework::Render::Scene* scene;
					Optional<Framework::Render::SceneView::Type> sceneView;
					Optional < Framework::Render::SceneMode::Type> sceneMode;
					int debugCameraMode;
					bool changeToRotationOnly;
					BuildRenderScenes(Game* _game, Framework::Render::Camera& _camera, Optional<Framework::Render::SceneView::Type> const & _sceneView, Optional<Framework::Render::SceneMode::Type> const& _sceneMode, int _debugCameraMode, bool _changeToRotationOnly)
					: game(_game)
					, camera(_camera)
					, scene(nullptr)
					, sceneView(_sceneView)
					, sceneMode(_sceneMode)
					, debugCameraMode(_debugCameraMode)
					, changeToRotationOnly(_changeToRotationOnly)
					{}
					static void build(IMMEDIATE_JOB_PARAMS)
					{
						FOR_EVERY_IMMEDIATE_JOB_DATA(BuildRenderScenes, brs)
						{
							brs->scene = Framework::Render::Scene::build(REF_ brs->camera, brs->game->get_scene_build_request(), [brs](Framework::Render::SceneBuildContext& sbc) {brs->game->setup_scene_build_context(sbc); }, brs->sceneView, brs->sceneMode, NP, Framework::Render::AllowVRInputOnRender(brs->debugCameraMode == 0).maybe_change_to_rotation_only(brs->changeToRotationOnly));
						}
					}
				};
				NAME_POOLED_OBJECT_IF_NOT_NAMED(BuildRenderScenes);
				Array<BuildRenderScenes*> brs;
				brs.push_back(new BuildRenderScenes(this, camera, NP, Framework::Render::SceneMode::VR_Left, debugCameraMode, changeToRotationOnly));
				brs.push_back(new BuildRenderScenes(this, camera, NP, Framework::Render::SceneMode::VR_Right, debugCameraMode, changeToRotationOnly));
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
				externalRenderSceneIdx = NONE;
				if (useExternalRenderScene || onlyExternalRenderScene)
				{
					externalRenderSceneIdx = brs.get_size();
					brs.push_back(new BuildRenderScenes(this, cameraExternal, Framework::Render::SceneView::External, NP, 0, false /* ignored */));
				}
#endif
				get_job_system()->do_immediate_jobs<BuildRenderScenes>(prepare_render_jobs(), BuildRenderScenes::build, brs, nullptr, 1);
				for_every_ptr(built, brs)
				{
					useRenderScenes.push_back(RefCountObjectPtr<Framework::Render::Scene>(built->scene));
					delete built;
				}
			}
			else
			{
				useRenderScenes.push_back(RefCountObjectPtr<Framework::Render::Scene>(Framework::Render::Scene::build(REF_ camera, get_scene_build_request(), [this](Framework::Render::SceneBuildContext& sbc) {setup_scene_build_context(sbc); }, NP, Framework::Render::SceneMode::VR_Left, NP, Framework::Render::AllowVRInputOnRender(! debugRenderScene && debugCameraMode == 0).maybe_change_to_rotation_only(changeToRotationOnly))));
				useRenderScenes.push_back(RefCountObjectPtr<Framework::Render::Scene>(Framework::Render::Scene::build(REF_ camera, get_scene_build_request(), [this](Framework::Render::SceneBuildContext& sbc) {setup_scene_build_context(sbc); }, NP, Framework::Render::SceneMode::VR_Right, NP, Framework::Render::AllowVRInputOnRender(! debugRenderScene && debugCameraMode == 0).maybe_change_to_rotation_only(changeToRotationOnly))));
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
				externalRenderSceneIdx = NONE;
				if (useExternalRenderScene || onlyExternalRenderScene)
				{
					externalRenderSceneIdx = useRenderScenes.get_size();
					useRenderScenes.push_back(RefCountObjectPtr<Framework::Render::Scene>(Framework::Render::Scene::build(REF_ cameraExternal, get_scene_build_request(), [this](Framework::Render::SceneBuildContext& sbc) {setup_scene_build_context(sbc); }, Framework::Render::SceneView::External)));
				}
#endif
			}
		}
		else
		{
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
			externalRenderSceneIdx = NONE;
			if ((useExternalRenderScene || onlyExternalRenderScene) && !_customRC)
			{
				externalRenderSceneIdx = useRenderScenes.get_size();
				useRenderScenes.push_back(RefCountObjectPtr<Framework::Render::Scene>(Framework::Render::Scene::build(REF_ cameraExternal, get_scene_build_request(), [this](Framework::Render::SceneBuildContext& sbc) {setup_scene_build_context(sbc); }, Framework::Render::SceneView::External)));
			}
			if (! onlyExternalRenderScene || _customRC)
#endif
			{
				useRenderScenes.push_back(RefCountObjectPtr<Framework::Render::Scene>(Framework::Render::Scene::build(REF_ camera, get_scene_build_request(), [this](Framework::Render::SceneBuildContext& sbc) {setup_scene_build_context(sbc); }, NP, NP, NP, Framework::Render::AllowVRInputOnRender(!debugRenderScene && debugCameraMode == 0).maybe_change_to_rotation_only(changeToRotationOnly))));
			}
		}
	}

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	externalCameraVRAnchorOffset.clear();

	if (auto* a = player.get_actor())
	{
		if (externalRenderPlayerHeightDoneFor != a)
		{
			externalRenderPlayerHeightDoneFor = a;
			externalRenderPlayerHeight = 1.8f;
			externalRenderPlayerHeight = a->get_value<float>(NAME(heightForExternalView), externalRenderPlayerHeight);
		}
	}
	if ((useExternalRenderScene || onlyExternalRenderScene) && externalRenderSceneIdx != NONE &&
		useRenderScenes.is_index_valid(externalRenderSceneIdx))
	{
		if (auto* scene = useRenderScenes[externalRenderSceneIdx].get())
		{
			float playerHeight = externalRenderPlayerHeight;

			float diagonal = RoomGenerationInfo::get().get_play_area_rect_size().length();

			float lookAnglePitch = DEG_ 30.0f;
			float lookAngleYaw = DEG_ -45.0f; // relative to vr anchor's forward dir
			float distance = diagonal * 0.7f + 1.5f;
			Vector3 verticalOffset = Vector3::zAxis * (playerHeight * 0.5f);
			// we add 180.0f to lookAngleYaw as this is where we should be placed - opposite to where we're looking at
			Transform offset = look_at_matrix(Rotator3(lookAnglePitch, lookAngleYaw + 180.0f, 0.0f).get_forward() * distance + verticalOffset, verticalOffset, Vector3::zAxis).to_transform();
			Transform cameraPlacement = scene->get_vr_anchor().to_world(offset);
			scene->access_render_camera().set_placement(scene->get_render_camera().get_in_room(), cameraPlacement);
			scene->access_render_camera().set_camera_type(System::CameraType::Orthogonal);
			// calculate scale, we have to match in x, but scale is y
			float orthoScale = 0.10f;
			if (auto* rt = get_secondary_view_render_buffer())
			{
				Vector2 size = rt->get_full_size_for_aspect_ratio_coef().to_vector2();
				// horizontal component
				{
					float scaleHorizontal = 0.95f / diagonal;
					float scaleVertical = scaleHorizontal * size.x / size.y;
					orthoScale = scaleVertical;
				}
				// vertical component - will use smaller of two
				{
					float scaleVertical = 0.95f / (playerHeight + diagonal * sin_deg(lookAnglePitch));
					orthoScale = min(orthoScale, scaleVertical);
				}
			}
			scene->access_render_camera().set_ortho_scale(orthoScale);
			externalCameraVRAnchorOffset = offset;
		}
	}
#endif		

	preparedRenderScenes = true;
}

Optional<Framework::Render::SceneBuildRequest> Game::get_scene_build_request() const
{
	Optional<Framework::Render::SceneBuildRequest> sceneBuildRequest;

	if (cleanScreenShots && ScreenShotContext::is_doing_screen_shot())
	{
		sceneBuildRequest.set_if_not_set();
		sceneBuildRequest.access().no_foreground_owner_local();
		sceneBuildRequest.access().no_foreground_camera_local();
		sceneBuildRequest.access().no_foreground_camera_owner_orientation();
	}

#ifdef DEMO_ROBOT_SHADOW
#ifdef RENDER_SCENE_WITH_ON_PREPARE_TO_RENDER
	{
		sceneBuildRequest.set_if_not_set();
		sceneBuildRequest.access().with_on_prepare_to_render([this](REF_ Meshes::Mesh3DInstance& _mesh, Framework::Room const* _inRoom, Transform const& _placement)
			{
				Optional<Transform> relPlacement;
				if (Framework::Actor* playerActor = player.get_actor())
				{
					auto* pl = _inRoom->get_presence_links();
					while (pl)
					{
						if (pl->get_modules_owner() == playerActor)
						{
							relPlacement = _placement.to_local(pl->get_placement_in_room());
						}
						pl = pl->get_next_in_room();
					}
				}

				DEFINE_STATIC_NAME(demoRobotShadow);
				DEFINE_STATIC_NAME(demoRobotShadowRadius);
				DEFINE_STATIC_NAME(demoRobotRelativeLocation);
				for_count(int, i, _mesh.get_material_instance_count())
				{
					if (auto* mi = _mesh.access_material_instance(i))
					{
						mi->set_uniform(NAME(demoRobotShadow), relPlacement.is_set() ? 1.0f : 0.0f);
						mi->set_uniform(NAME(demoRobotRelativeLocation), relPlacement.get(Transform::identity).get_translation());
					}
				}
			});
	}
#else
	#error requires RENDER_SCENE_WITH_ON_PREPARE_TO_RENDER
#endif
#endif

	return sceneBuildRequest;
}

void Game::setup_scene_build_context(Framework::Render::SceneBuildContext& _sbc) const
{
	if (cleanScreenShots && ScreenShotContext::is_doing_screen_shot())
	{
		// no highlights for screenshots
	}
	else
	{

		if (auto* actor = player.get_actor())
		{
			if (auto* highlight = actor->get_custom<CustomModules::HighlightObjects>())
			{
				highlight->setup_scene_build_context(_sbc);
			}
			if (TutorialSystem::check_active() && TutorialSystem::get()->should_be_highlighted_now())
			{
				if (auto* pilgrim = actor->get_gameplay_as<ModulePilgrim>())
				{
					pilgrim->setup_scene_build_context_for_tutorial_highlights(_sbc);
				}
			}
		}
	}
}

void Game::no_world_to_render()
{
	recentlyRendered.wasRecentlyRendered = false;
	renderScenes.clear();
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	externalRenderSceneIdx = NONE;
#endif
}

void Game::render_empty_background_on_render(Colour const & _colour, Framework::CustomRenderContext* _customRC, bool _renderToMain)
{
	an_assert(!_customRC || _customRC->sceneRenderTarget.is_set());
	System::Video3D * v3d = System::Video3D::get();

	{
		MEASURE_PERFORMANCE(renderEmptyBackground);

		if (!_renderToMain && !_customRC && does_use_vr())
		{
			for (int rtIdx = 0; rtIdx < VR::Eye::Count; ++rtIdx)
			{
				if (System::RenderTarget* rt = VR::IVR::get()->get_render_render_target((VR::Eye::Type)rtIdx))
				{
					rt->bind();
					v3d->set_viewport(VectorInt2::zero, rt->get_size());
					v3d->setup_for_2d_display();
					v3d->set_2d_projection_matrix_ortho(rt->get_size().to_vector2());
					v3d->set_near_far_plane(0.02f, 100.0f);
					v3d->clear_colour(_colour);
				}
			}
		}
		else if (!_renderToMain)
		{
			if (System::RenderTarget* rt = _customRC ? _customRC->sceneRenderTarget.get() : get_main_render_buffer())
			{
				rt->bind();
				v3d->set_viewport(VectorInt2::zero, rt->get_size());
				v3d->setup_for_2d_display();
				v3d->set_2d_projection_matrix_ortho(rt->get_size().to_vector2());
				v3d->set_near_far_plane(0.02f, 100.0f);
				v3d->clear_colour(_colour);
			}
		}
		else
		{
			if (System::RenderTarget* rt = get_main_render_buffer())
			{
				rt->bind();
				v3d->set_viewport(VectorInt2::zero, rt->get_size());
				v3d->setup_for_2d_display();
				v3d->set_2d_projection_matrix_ortho(rt->get_size().to_vector2());
				v3d->set_near_far_plane(0.02f, 100.0f);
				v3d->clear_colour(_colour);
			}
		}
		System::RenderTarget::unbind_to_none();
	}
}

bool Game::apply_prerender_fading() const
{
	bool result = false;
	for_count(int, i, 2)
	{
		auto& usePlayer = i == 0 ? player : playerTakenControl;
		if (auto* a = usePlayer.get_actor())
		{
			if (auto* fadingAppearance = a->get_appearance_named(NAME(fading)))
			{
				fadingAppearance->access_mesh_instance().set_rendering_order_priority(AppearanceRenderingPriority::Fading); // render as last, top most
				if (! fading.should_be_applied())
				{
					fadingAppearance->be_visible(false);
				}
				else
				{
					fadingAppearance->be_visible(true);

					auto & meshInstance = fadingAppearance->access_mesh_instance();
					auto * materialInstance = meshInstance.access_material_instance(0);

					materialInstance->set_uniform(NAME(fadingAmount), fading.current);
				
					/*
					// linear test
					static float t = 0.0f;
					static float dt = 1.0f;
					t = clamp(t + dt * System::Core::get_delta_time(), 0.0f, 1.0f);
					if (t >= 1.0f)
					{
						dt = -1.0f;
					}
					if (t <= 0.0f)
					{
						dt = 1.0f;
					}
					materialInstance->set_uniform(NAME(fadingAmount), t);
					*/
				
					materialInstance->set_uniform(NAME(fadingColour), fading.currentFadeOutColour.to_vector4());
				}
				result |= true;
			}
		}
	}

	return result;
}

void Game::render_prepared_world_on_render(Framework::CustomRenderContext* _customRC)
{
	an_assert(!_customRC || _customRC->sceneRenderTarget.is_set());
	System::RenderTarget* rtTarget = _customRC ? _customRC->sceneRenderTarget.get() : get_main_render_buffer();
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	System::RenderTarget* rtEVTarget = get_secondary_view_render_buffer();
#endif
	System::Video3D * v3d = System::Video3D::get();

	Array<RefCountObjectPtr<Framework::Render::Scene> > & useRenderScenes = _customRC ? _customRC->renderScenes : renderScenes;

	{
		MEASURE_PERFORMANCE(renderScenes);

		Framework::Render::Context rc(v3d);
		{
			MEASURE_PERFORMANCE(renderContextAccessDebug);
#ifdef AN_USE_RENDER_CONTEXT_DEBUG
			rc.access_debug() = rcDebug;
#endif
		}

		rc.set_default_texture_id(Framework::Library::get_current()->get_default_texture()->get_texture()->get_texture_id());

		{
			MEASURE_PERFORMANCE(combineDisplays);
			// combine displays to only update/render once
			Framework::Render::Scene::combine_displays(useRenderScenes);
		}

		if (!_customRC && does_use_vr())
		{
			if (!VR::IVR::get()->does_use_direct_to_vr_rendering() &&
				VR::IVR::get()->does_use_separate_output_render_target())
			{
				for_count(int, eyeIdx, VR::Eye::Count)
				{
					if (System::RenderTarget * rt = VR::IVR::get()->get_render_render_target((VR::Eye::Type)eyeIdx))
					{
						postProcessChain->setup_render_target_output_usage(rt);
					}
				}
			}

			for_every(sceneRef, useRenderScenes)
			{
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
				if (externalRenderSceneIdx == for_everys_index(sceneRef))
				{
					continue;
				}
#endif
				if (auto* scene = sceneRef->get())
				{
					{
						MEASURE_PERFORMANCE_COLOURED(renderSceneFinish, Colour::zxYellow);
						MEASURE_PERFORMANCE_FINISH_RENDERING();
					}
					bool renderOverlayInfoSystem = true;
					if (noOverlayScreenShots && ScreenShotContext::is_doing_screen_shot())
					{
						renderOverlayInfoSystem = false;
					}
					bool renderOverlayInfoSystemSeparately = true;
					if (renderOverlayInfoSystem)
					{
						an_assert(!scene->is_on_post_render_set());
						renderOverlayInfoSystemSeparately = scene->is_on_post_render_set();
						if (!renderOverlayInfoSystemSeparately)
						{
							scene->set_on_post_render([this, scene](Framework::Render::Scene* _scene, Framework::Render::Context& _context)
							{
								MEASURE_PERFORMANCE_COLOURED(renderOverlayInfoSystem, Colour::cyan);
								debug_context(scene->get_camera().get_in_room());
								overlayInfoSystem->render_to_current_render_target(scene->get_vr_anchor(), TeaForGodEmperor::OverlayInfo::Usage::World);
								debug_no_context();
							});
						}
					}
					{
						MEASURE_PERFORMANCE_COLOURED(renderScene, Colour::white);
						scene->render(REF_ rc, nullptr /* not required for vr */);
					}
					if (renderOverlayInfoSystem &&
						renderOverlayInfoSystemSeparately)
					{
						MEASURE_PERFORMANCE_COLOURED(renderOverlayInfoSystem, Colour::cyan);
						debug_context(scene->get_camera().get_in_room());
						overlayInfoSystem->render(scene->get_vr_anchor(), TeaForGodEmperor::OverlayInfo::Usage::World, (VR::Eye::Type)scene->get_vr_eye_idx());
						debug_no_context();
					}
					{
						MEASURE_PERFORMANCE_COLOURED(renderSceneFinish, Colour::zxYellow);
						MEASURE_PERFORMANCE_FINISH_RENDERING();
					}

#ifdef AN_DEBUG_RENDERER
					// to render debug
					{
						MEASURE_PERFORMANCE(renderDebugRenderer);
						if (!debugRenderingSuspended)
						{
							VR::Eye::Type eye = (VR::Eye::Type)scene->get_render_camera().get_eye_idx();
							if (System::RenderTarget* rt = VR::IVR::get()->get_render_render_target(eye))
							{
								rt->bind();
								debug_renderer_render(v3d);
								rt->unbind();
							}
						}
					}
#endif

#ifdef AN_PERFORMANCE_MEASURE
					if (!showPerformanceAfterFrames.is_set())
#endif
					{
						if (showDebugPanelVR || showLogPanelVR || showPerformancePanelVR)
						{
							VR::Eye::Type eye = (VR::Eye::Type)scene->get_render_camera().get_eye_idx();
							if (System::RenderTarget* rt = VR::IVR::get()->get_render_render_target(eye))
							{
								rt->bind();
								if (showLogPanelVR)
								{
									showLogPanelVR->render_vr(eye);
								}
								if (showPerformancePanelVR)
								{
									showPerformancePanelVR->render_vr(eye);
								}
								if (showDebugPanelVR)
								{
									showDebugPanelVR->render_vr(eye);
								}
								rt->unbind();
							}
						}
					}
				}
			}
		}
		else
		{
			// use right eye
			for_every_reverse(sceneRef, useRenderScenes)
			{
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
				if (externalRenderSceneIdx == for_everys_index(sceneRef))
				{
					continue;
				}
#endif
				if (auto* scene = sceneRef->get())
				{
					{
						MEASURE_PERFORMANCE_COLOURED(renderSceneFinish, Colour::zxYellow);
						MEASURE_PERFORMANCE_FINISH_RENDERING();
					}
					postProcessChain->setup_render_target_output_usage(rtTarget);
					bool renderOverlayInfoSystem = true;
					if (noOverlayScreenShots && ScreenShotContext::is_doing_screen_shot())
					{
						renderOverlayInfoSystem = false;
					}
					bool renderOverlayInfoSystemSeparately = true;
					if (renderOverlayInfoSystem)
					{
						an_assert(!scene->is_on_post_render_set());
						renderOverlayInfoSystemSeparately = scene->is_on_post_render_set();
						if (!renderOverlayInfoSystemSeparately)
						{
							scene->set_on_post_render([this, scene](Framework::Render::Scene* _scene, Framework::Render::Context& _context)
							{
								MEASURE_PERFORMANCE_COLOURED(renderOverlayInfoSystem, Colour::cyan);
								debug_context(scene->get_camera().get_in_room());
								overlayInfoSystem->render_to_current_render_target(scene->get_vr_anchor(), TeaForGodEmperor::OverlayInfo::Usage::World);
								debug_no_context();
							});
						}
					}
					{
						MEASURE_PERFORMANCE_COLOURED(renderScene, Colour::white);
						scene->render(REF_ rc, rtTarget /* not required for vr, unless customRC used that is non vr */);
					}
					if (renderOverlayInfoSystem &&
						renderOverlayInfoSystemSeparately)
					{
						MEASURE_PERFORMANCE_COLOURED(renderOverlayInfoSystem, Colour::cyan);
						debug_context(scene->get_camera().get_in_room());
						overlayInfoSystem->render(scene->get_vr_anchor(), TeaForGodEmperor::OverlayInfo::Usage::World, NP, rtTarget);
						debug_no_context();
					}
					{
						MEASURE_PERFORMANCE_COLOURED(renderSceneFinish, Colour::zxYellow);
						MEASURE_PERFORMANCE_FINISH_RENDERING();
					}
#ifdef AN_DEBUG_RENDERER
					// to render debug
					{
						MEASURE_PERFORMANCE(renderDebugRenderer);
						if (!debugRenderingSuspended)
						{
							if (rtTarget)
							{
								rtTarget->bind();
							}
							debug_renderer_render(v3d);
							if (rtTarget)
							{
								rtTarget->unbind();
							}
						}
					}
#endif
					break; // just one scene!
				}
			}
		}
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
		if (! _customRC &&
			externalRenderSceneIdx != NONE &&
			useRenderScenes.is_index_valid(externalRenderSceneIdx) &&
			rtEVTarget)
		{
			if (auto* scene = useRenderScenes[externalRenderSceneIdx].get())
			{
				// external render scene renders no overlay!
				{
					MEASURE_PERFORMANCE_COLOURED(renderSceneFinish, Colour::zxYellow);
					MEASURE_PERFORMANCE_FINISH_RENDERING();
				}
				postProcessChain->setup_render_target_output_usage(rtEVTarget);
				{
					MEASURE_PERFORMANCE_COLOURED(renderScene, Colour::white);
					scene->render(REF_ rc, rtEVTarget);
				}
				{
					if (auto* c = TeaForGodEmperor::GameConfig::get_as<TeaForGodEmperor::GameConfig>())
					{
						externalRenderPlayArea.render = c->get_external_view_info().showBoundary >= ExternalViewShowBoundary::FloorOnly;
						externalRenderPlayAreaWalls.render = c->get_external_view_info().showBoundary >= ExternalViewShowBoundary::BackWalls;
						externalRenderPlayAreaFrontWalls.render = c->get_external_view_info().showBoundary >= ExternalViewShowBoundary::AllWalls;
					}
					externalRenderPlayArea.active = blend_to_using_speed_based_on_time(externalRenderPlayArea.active, externalRenderPlayArea.render ? 1.0f : 0.0f, 0.2f, ::System::Core::get_raw_delta_time());
					externalRenderPlayAreaWalls.active = blend_to_using_speed_based_on_time(externalRenderPlayAreaWalls.active, externalRenderPlayAreaWalls.render ? 1.0f : 0.0f, 0.2f, ::System::Core::get_raw_delta_time());
					externalRenderPlayAreaFrontWalls.active = blend_to_using_speed_based_on_time(externalRenderPlayAreaFrontWalls.active, externalRenderPlayAreaFrontWalls.render ? 1.0f : 0.0f, 0.2f, ::System::Core::get_raw_delta_time());
					if (externalRenderPlayArea.active > 0.0f)
					{
						if (rtEVTarget)
						{
							rtEVTarget->bind();
						}

						debug_context(scene->get_camera().get_in_room());
						::System::VideoMatrixStack& modelViewStack = v3d->access_model_view_matrix_stack();
						modelViewStack.push_to_world(scene->get_vr_anchor().to_matrix());
						modelViewStack.push_to_world(translation_matrix(Vector3::zAxis * 0.01f));

						auto& clipPlaneStack = v3d->access_clip_plane_stack();
						clipPlaneStack.clear();

						//v3d->set_near_far_plane(nearFarPlane.min, nearFarPlane.max);
						//v3d->setup_for_2d_display();
					
						v3d->colour_mask(true);
						v3d->depth_mask(false);

						v3d->stencil_test();
						v3d->stencil_op(::System::Video3DOp::Keep);
						v3d->depth_test(System::Video3DCompareFunc::LessEqual);

						v3d->front_face_order(System::FaceOrder::CW);
						v3d->face_display(System::FaceDisplay::Front);

						Framework::Render::RoomProxy::set_rendering_depth_range(v3d); // render in the same range as room proxy

						v3d->requires_send_state();

						//

						auto const& rgi = RoomGenerationInfo::get();
						Vector3 playAreaSize = rgi.get_play_area_rect_size().to_vector3();
						Vector3 paLB = playAreaSize * Vector3(-0.5f, -0.5f, 0.0f);
						Vector3 paLT = playAreaSize * Vector3(-0.5f,  0.5f, 0.0f);
						Vector3 paRT = playAreaSize * Vector3( 0.5f,  0.5f, 0.0f);
						Vector3 paRB = playAreaSize * Vector3( 0.5f, -0.5f, 0.0f);
						float spacing = 0.3f;
						float border = 0.05f;

						Colour playAreaColour = Colour::cyan.with_alpha(externalRenderPlayArea.active * 0.5f);

						// floor
						::System::Video3DPrimitives::fill_rect(playAreaColour, Range2(Range(paLB.x + border, paRT.x - border), Range(paLB.y, paLB.y + border)), true);
						::System::Video3DPrimitives::fill_rect(playAreaColour, Range2(Range(paLB.x + border, paRT.x - border), Range(paRT.y - border, paRT.y)), true);
						::System::Video3DPrimitives::fill_rect(playAreaColour, Range2(Range(paLB.x, paLB.x + border), Range(paLB.y, paRT.y)), true);
						::System::Video3DPrimitives::fill_rect(playAreaColour, Range2(Range(paRT.x - border, paRT.x), Range(paLB.y, paRT.y)), true);

						struct DrawWall
						{
							static void at(Vector3 const& a, Vector3 const& b, Vector3 const & height, float spacing, float width, Colour& colour)
							{
								float length = (b - a).length() - width;
								int count = TypeConversions::Normal::f_i_closest(length / spacing);
								Vector3 ab = (b - a).normal();
								float step = length / (float)count;
								Vector3 h = height.normal() * (height.length() - width);
								for (int i = 0; i <= count; ++i)
								{
									Vector3 at = a + ab * (step * (float)i);
									Vector3 atw = at + ab * width;

									::System::Video3DPrimitives::quad(colour, at, at + h, atw + h, atw, true);
								}

								// top
								::System::Video3DPrimitives::quad(colour, a + h, a + height, b + height, b + h, true);
							}
						};

						{
							Vector3 height = Vector3::zAxis * externalRenderPlayerHeight;

							Colour pac = playAreaColour;

							// walls from inside (back walls)
							pac.a *= externalRenderPlayAreaWalls.active;
							DrawWall::at(paLB, paLT, height, spacing, border, pac);
							DrawWall::at(paLT, paRT, height, spacing, border, pac);
							DrawWall::at(paRT, paRB, height, spacing, border, pac);
							DrawWall::at(paRB, paLB, height, spacing, border, pac);

							// walls from outside (front walls)
							pac.a *= externalRenderPlayAreaFrontWalls.active;
							DrawWall::at(paLT, paLB, height, spacing, border, pac);
							DrawWall::at(paRT, paLT, height, spacing, border, pac);
							DrawWall::at(paRB, paRT, height, spacing, border, pac);
							DrawWall::at(paLB, paRB, height, spacing, border, pac);
						}

						Framework::Render::RoomProxy::clear_rendering_depth_range(v3d);

						//
						modelViewStack.pop();
						modelViewStack.pop();
						debug_no_context();

						if (rtEVTarget)
						{
							rtEVTarget->unbind();
						}
					}
				}
				{
					MEASURE_PERFORMANCE_COLOURED(renderSceneFinish, Colour::zxYellow);
					MEASURE_PERFORMANCE_FINISH_RENDERING();
				}
#ifdef AN_DEBUG_RENDERER
				// to render debug
				{
					MEASURE_PERFORMANCE(renderDebugRenderer);
					if (!debugRenderingSuspended)
					{
						if (rtEVTarget)
						{
							rtEVTarget->bind();
						}
						debug_renderer_render(v3d);
						if (rtEVTarget)
						{
							rtEVTarget->unbind();
						}
					}
				}
#endif
			}
		}
#endif
#ifdef AN_DEBUG_RENDERER
		// to render debug
		{
			MEASURE_PERFORMANCE(renderDebugRendererUndefineContexts);
			debug_renderer_undefine_contexts();
		}
#endif
		System::RenderTarget::unbind_to_none();
	}
}

void Game::render_post_process(Framework::CustomRenderContext* _customRC)
{
#ifdef BUILD_NON_RELEASE
	if (_customRC && ::System::RenderTargetSaveSpecialOptions::get().transparent)
	{
		// skip post process to keep alpha intact
		return;
	}
#endif
	if (! ::System::VideoConfig::general_is_post_process_allowed())
	{
		return;
	}

	System::Video3D * v3d = System::Video3D::get();

	// no need to render debug any further
	debug_renderer_already_rendered();

	//rt->generate_mip_maps();

	// ???

	Array<RefCountObjectPtr<Framework::Render::Scene> > & useRenderScenes = _customRC ? _customRC->renderScenes : renderScenes;

	struct RenderTargetSelector
	{
		enum Idx
		{
			CustomOrMain,
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
			External,
#endif			
			Count,
			CountForCustom = CustomOrMain + 1
		};

		Game* game;
		Framework::CustomRenderContext* customRC;
		System::Video3D* v3d;
		System::RenderTarget* rt = nullptr;
		System::RenderTarget* rtOutput = nullptr;
		bool ok = false;
		RenderTargetSelector(Game* _game, Framework::CustomRenderContext* _customRC)
		: game(_game)
		, customRC(_customRC)
		, v3d(System::Video3D::get())
		{			
		}

		bool select(Idx _idx)
		{
			rt = nullptr;
			rtOutput = nullptr;
			ok = false;
			if (_idx == CustomOrMain)
			{
				rt = customRC ? customRC->sceneRenderTarget.get() : game->get_main_render_buffer();
				if (rt)
				{
					ok = true;
					rtOutput = customRC ? customRC->sceneRenderTarget.get() : game->get_main_render_output_render_buffer();
				}
			}
			if (!customRC && game->does_use_vr() && (v3d->should_skip_display_buffer_for_VR() ||
												(!VR::IVR::get()->does_use_direct_to_vr_rendering() &&
												  VR::IVR::get()->does_use_separate_output_render_target())))
			{
				ok = false;
			}
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
			if (_idx == External)
			{
				rt = game->get_secondary_view_render_buffer();
				if (rt)
				{
					ok = true;
					rtOutput = game->get_secondary_view_output_render_buffer();
				}
			}
#endif

			return rt && ok;
		}
	} renderTargetSelector(this, _customRC);

	// we need render scenes or at least camera for custom rc to provide camera to post process
	if (useRenderScenes.get_size() > 0 || (_customRC && _customRC->renderCamera.is_set()))
	{
		MEASURE_PERFORMANCE_FINISH_RENDERING();
		MEASURE_PERFORMANCE_COLOURED(postProcess, Colour::zxMagenta);

		// setup post process shader programs
		{
			postProcessChain->for_every_shader_program_instance([this](Framework::PostProcessNode const * _node, ::System::ShaderProgramInstance & _shaderProgramInstance)
			{
				DEFINE_STATIC_NAME(vignetteForMovement);
				DEFINE_STATIC_NAME(vignetteForMovementStrength);
				_shaderProgramInstance.set_uniforms_from(_shaderProgramInstance.get_shader_program()->get_default_values());
				for_every(op, overridePostProcessParams)
				{
					if (auto* sp = _shaderProgramInstance.get_shader_param(op->param))
					{
						float value = sp->get_as_float();
						if (!op->value.is_set() || op->strength == 0.0f)
						{
							op->value = value;
						}
						value = value * (1.0f - op->strength) + op->strength * op->value.get();
						_shaderProgramInstance.set_uniform(op->param, value);
					}
				}
				/*
				{
					DEFINE_STATIC_NAME(filterBloomStrength);
					if (auto* sp = _shaderProgramInstance.get_shader_param(NAME(filterBloomStrength)))
					{
						float filterBloomStrength = sp->get_as_float();
						filterBloomStrength = 0.0f; // !@# test it
						_shaderProgramInstance.set_uniform(NAME(filterBloomStrength), filterBloomStrength);
					}
				}
				{
					DEFINE_STATIC_NAME(applyBloomStrength);
					if (auto* sp = _shaderProgramInstance.get_shader_param(NAME(applyBloomStrength)))
					{
						float applyBloomStrength = sp->get_as_float();
						applyBloomStrength = 0.0f; // !@# test it
						_shaderProgramInstance.set_uniform(NAME(applyBloomStrength), applyBloomStrength);
					}
				}
				*/
				{
					auto const& psp = PlayerSetup::get_current().get_preferences();
					if (psp.useShaderVignetteForMovement)
					{
						_shaderProgramInstance.set_uniform(NAME(vignetteForMovement), psp.useVignetteForMovement ? psp.vignetteForMovementStrength : 0.0f);
						_shaderProgramInstance.set_uniform(NAME(vignetteForMovementStrength), vignetteForMovementAmount);
					}
					else
					{
						_shaderProgramInstance.set_uniform(NAME(vignetteForMovement), 0.0f);
						_shaderProgramInstance.set_uniform(NAME(vignetteForMovementStrength), 0.0f);
					}
				}
				return true;
			}
			);
		}

		// post process for both eyes (vr)
		if (!_customRC && does_use_vr() &&
			!VR::IVR::get()->does_use_direct_to_vr_rendering() &&
			VR::IVR::get()->does_use_separate_output_render_target()) // no post process for direct to vr rendering
		{
			for (int rtIdx = 0; rtIdx < VR::Eye::Count; ++rtIdx)
			{
				VR::Eye::Type eye = (VR::Eye::Type)rtIdx;
				if (System::RenderTarget* rtOutput = VR::IVR::get()->get_output_render_target(eye))
				{
					if (System::RenderTarget* rtRender = VR::IVR::get()->get_render_render_target(eye))
					{
						postProcessChain->set_camera_that_was_used_to_render(useRenderScenes[rtIdx]->get_render_camera());

						rtRender->resolve_forced_unbind();

						postProcessChain->do_post_process(rtRender, v3d);

						// render to output render target
						rtOutput->bind();
						v3d->set_viewport(VectorInt2::zero, rtOutput->get_size());
						v3d->set_2d_projection_matrix_ortho(rtOutput->get_size().to_vector2());

						postProcessChain->render_output(v3d, Vector2::zero, rtOutput->get_size().to_vector2());

						postProcessChain->release_render_targets();
					}
				}
			}
		}
		
		// always, no else!
		{
			// render to main buffer (not main render buffer) unless custom rc is provided
			an_assert(!_customRC || _customRC->sceneRenderTarget.is_set());
			for_count(int, rtIdx, _customRC? RenderTargetSelector::CountForCustom : RenderTargetSelector::Count)
			{
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
				if (rtIdx == RenderTargetSelector::External && !useExternalRenderScene && !onlyExternalRenderScene)
				{
					continue;
				}
#endif
				System::RenderTarget* rt = nullptr;
				if (renderTargetSelector.select((RenderTargetSelector::Idx)rtIdx))
				{
					rt = renderTargetSelector.rt;
				}
				{
					/*
					if (! rt)
					{
						if (renderTargetSelector.rtOutput)
						{
							renderTargetSelector.rtOutput->bind();
						}
						else
						{
							System::RenderTarget::bind_none();
						}
#ifndef AN_ANDROID
						// nothing to clear
						setup_to_render_to_render_target(v3d);
						v3d->clear_colour(Colour::blackWarm);
#endif
					}
					*/
					if (rt)
					{
						if (useRenderScenes.is_empty())
						{
							an_assert(_customRC && _customRC->renderCamera.is_set());
							postProcessChain->set_camera_that_was_used_to_render(_customRC->renderCamera.get());
						}
						else
						{
							int sceneIdx = 0;
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
							if (rtIdx == RenderTargetSelector::External)
							{
								sceneIdx = externalRenderSceneIdx;
							}
#endif
							postProcessChain->set_camera_that_was_used_to_render(useRenderScenes[sceneIdx]->get_camera());
						}

						rt->resolve_forced_unbind();

						postProcessChain->do_post_process(rt, v3d);

						if (renderTargetSelector.rtOutput)
						{
							renderTargetSelector.rtOutput->bind();
						}
						else
						{
							System::RenderTarget::bind_none();
						}
						setup_to_render_to_render_target(v3d, renderTargetSelector.rtOutput);

						postProcessChain->render_output(v3d, Vector2::zero, v3d->get_viewport_size().to_vector2());
						postProcessChain->release_render_targets();

						System::RenderTarget::unbind_to_none();
					}
				}
			}
		}
	}
	else
	{
		MEASURE_PERFORMANCE_COLOURED(skipPostProcess, Colour::zxMagenta);

		// skip post process as we don't have info about camera!
		if (!_customRC && does_use_vr() &&
			!VR::IVR::get()->does_use_direct_to_vr_rendering() &&
			VR::IVR::get()->does_use_separate_output_render_target())
		{
			for (int rtIdx = 0; rtIdx < VR::Eye::Count; ++rtIdx)
			{
				VR::Eye::Type eye = (VR::Eye::Type)rtIdx;
				if (System::RenderTarget* rtOutput = VR::IVR::get()->get_output_render_target(eye))
				{
					if (System::RenderTarget* rtRender = VR::IVR::get()->get_render_render_target(eye))
					{
						if (rtRender != rtOutput)
						{
							rtRender->resolve_forced_unbind();
							rtOutput->bind();
							setup_to_render_to_render_target(v3d, rtOutput);

							rtRender->render(0, v3d, Vector2::zero, rtOutput->get_size().to_vector2());

							::System::RenderTarget::unbind_to_none();
						}
					}
				}
			}
		}

		// always, no else!
		{
			// render to main buffer (not main render buffer) unless custom rc is provided
			an_assert(!_customRC || _customRC->sceneRenderTarget.is_set());
			for_count(int, rtIdx, _customRC ? RenderTargetSelector::CountForCustom : RenderTargetSelector::Count)
			{
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
				if (rtIdx == RenderTargetSelector::External && !useExternalRenderScene && !onlyExternalRenderScene)
				{
					continue;
				}
#endif
				System::RenderTarget* rt = nullptr;
				if (renderTargetSelector.select((RenderTargetSelector::Idx)rtIdx))
				{
					rt = renderTargetSelector.rt;
				}
				if (rt)
				{
					rt->resolve_forced_unbind();
					if (renderTargetSelector.rtOutput)
					{
						renderTargetSelector.rtOutput->bind();
					}
					else
					{
						System::RenderTarget::bind_none();
					}
					setup_to_render_to_render_target(v3d, renderTargetSelector.rtOutput);

					rt->render(0, v3d, Vector2::zero, v3d->get_viewport_size().to_vector2());

					::System::RenderTarget::unbind_to_none();
				}
			}
		}
	}
}

void Game::render_bullshot_logos_on_main_render_output(Framework::CustomRenderContext* _customRC)
{
#ifdef AN_ALLOW_BULLSHOTS
	if (Framework::BullshotSystem::does_want_to_draw())
	{
		if (!_customRC && does_use_vr())
		{
			for (int rtIdx = 0; rtIdx < VR::Eye::Count; ++rtIdx)
			{
				VR::Eye::Type eye = (VR::Eye::Type)rtIdx;
				if (System::RenderTarget* rtOutput = VR::IVR::get()->get_output_render_target(eye))
				{
					Vector2 rtSize = rtOutput->get_size().to_vector2();
					auto& vrri = VR::IVR::get()->get_render_info();
					Vector2 centre = rtSize * 0.5f + vrri.lensCentreOffset[eye] * rtSize * 0.5f;

					rtOutput->bind();
					Framework::BullshotSystem::render_logos_and_titles(rtOutput, centre.to_vector_int2());
					rtOutput->unbind();
				}
			}
		}

		{
			// render to main buffer (not main render buffer) unless custom rc is provided
			an_assert(!_customRC || _customRC->finalRenderTarget.is_set());
			if (System::RenderTarget* rt = _customRC ? _customRC->finalRenderTarget.get() : get_main_render_output_render_buffer())
			{
				if (rt)
				{
					rt->bind();
				}
				else
				{
					System::RenderTarget::bind_none();
				}
				Framework::BullshotSystem::render_logos_and_titles(_customRC ? _customRC->finalRenderTarget.get() : nullptr);

				System::RenderTarget::unbind_to_none();
			}
		}
	}
#endif
}

void Game::render_display_2d(Framework::Display* _display, Vector2 const & _relPos, Framework::CustomRenderContext* _customRC, bool _renderToMain, System::BlendOp::Type _srcBlend, System::BlendOp::Type _destBlend)
{
	if (_display)
	{
		System::Video3D* v3d = System::Video3D::get();

		if (!_renderToMain && !_customRC && does_use_vr())
		{
			for (int rtIdx = 0; rtIdx < VR::Eye::Count; ++rtIdx)
			{
				VR::Eye::Type eye = (VR::Eye::Type)rtIdx;
				if (System::RenderTarget* rt = VR::IVR::get()->get_output_render_target(eye))
				{
					// render back to render target - vr will handle displaying them
					rt->bind();
					v3d->set_viewport(VectorInt2::zero, rt->get_size());
					v3d->setup_for_2d_display();
					v3d->set_2d_projection_matrix_ortho(rt->get_size().to_vector2());
					v3d->access_model_view_matrix_stack().clear();

					if (auto* displayRT = _display->get_output_render_target())
					{
						_display->render_2d(v3d, (v3d->get_viewport_size() - displayRT->get_size()).to_vector2() * _relPos + displayRT->get_size().to_vector2() * Vector2::half
							+ (VR::IVR::get()->get_render_info().lensCentreOffset[eye] * v3d->get_viewport_size().to_vector2() * Vector2::half), NP, 0.0f, _srcBlend, _destBlend);
					}
					System::RenderTarget::unbind_to_none();
				}
			}
		}
		else
		{
			System::RenderTarget* rt = nullptr;
			if (_customRC)
			{
				an_assert(_customRC->finalRenderTarget.is_set());
				rt = _customRC->finalRenderTarget.get();
			}

			if (rt)
			{
				rt->bind();
			}
			else
			{
				System::RenderTarget::bind_none();
			}
			setup_to_render_to_render_target(v3d, _customRC ? _customRC->finalRenderTarget.get() : nullptr);
			v3d->setup_for_2d_display();

			// restore projection matrix
			v3d->set_2d_projection_matrix_ortho();
			v3d->access_model_view_matrix_stack().clear();

			if (auto* displayRT = _display->get_output_render_target())
			{
				_display->render_2d(v3d, (v3d->get_viewport_size() - displayRT->get_size()).to_vector2() * _relPos + displayRT->get_size().to_vector2() * Vector2::half, NP, 0.0f, _srcBlend, _destBlend);
			}

			System::RenderTarget::unbind_to_none();
		}
	}
}

#ifdef FPS_INFO
float Game::get_ideal_expected_frame_per_second() const
{
	float expectedFrameTime = System::Video3D::get()->get_ideal_expected_frame_time();
	if (expectedFrameTime != 0.0f)
	{
		return 1.0f / expectedFrameTime;
	}
	else
	{
		return 0.0f;
	}
}
#endif

void Game::render_bullshot_frame_on_main_render_output()
{
#ifdef AN_ALLOW_BULLSHOTS
	if (Framework::BullshotSystem::does_want_to_draw())
	{
		System::Video3D* v3d = System::Video3D::get();

		for (int eyeIdx = 0; eyeIdx < (does_use_vr() ? 2 : 1); ++eyeIdx)
		{
			Vector2 rtSize;
			Vector2 centre;
			{
				rtSize = v3d->get_screen_size().to_vector2();
				centre = rtSize * 0.5f;
				VR::Eye::Type eye = (VR::Eye::Type)eyeIdx;
				System::RenderTarget* rt = get_main_render_output_render_buffer();
				if (does_use_vr() && VR::IVR::get()->get_output_render_target(eye))
				{
					rt = VR::IVR::get()->get_output_render_target(eye);
				}
				if (rt)
				{
					rt->bind();
					rtSize = rt->get_size().to_vector2();
				}
				else
				{
					System::RenderTarget::bind_none();
				}
				if (does_use_vr())
				{
					centre = rtSize * 0.5f + VR::IVR::get()->get_render_info().lensCentreOffset[eyeIdx] * rtSize * 0.5f;
				}
				v3d->set_viewport(VectorInt2::zero, rtSize.to_vector_int2());
				v3d->setup_for_2d_display();
				v3d->set_2d_projection_matrix_ortho(rtSize);
				v3d->access_model_view_matrix_stack().clear();
			}

			RangeInt2 frameRectI = Framework::BullshotSystem::calculate_show_frame_for(rtSize.to_vector_int2(), centre.to_vector_int2());
			if (!frameRectI.is_empty())
			{
				Vector2 viewport = v3d->get_viewport_size().to_vector2();
				Range2 vRect = Range2::zero;
				vRect.include(viewport);

				Range2 rect;
				rect.x.min = (float)frameRectI.x.min;
				rect.x.max = (float)frameRectI.x.max;
				rect.y.min = (float)frameRectI.y.min;
				rect.y.max = (float)frameRectI.y.max;

				bool allowBlend = true;
				if (::System::Core::get_system_tags().get_tag(NAME(lowGraphics)))
				{
#ifdef AN_ANDROID
					// allow blend on fake quest, playing on pc with quest settings
					allowBlend = false;
#endif
				}

				{
					{
						Colour shadow = Colour::black.with_set_alpha(0.3f);
						{
							Range2	r = vRect;	r.y.min = rect.y.max + 1.0f;	System::Video3DPrimitives::fill_rect_2d(shadow, r.bottom_left(), r.top_right(), allowBlend);
									r = vRect;	r.y.max = rect.y.min - 1.0f;	System::Video3DPrimitives::fill_rect_2d(shadow, r.bottom_left(), r.top_right(), allowBlend);
						}
						{
							Range2	sides = vRect;
							sides.y = rect.y;
							Range2	r = sides;	r.x.min = rect.x.max + 1.0f;	System::Video3DPrimitives::fill_rect_2d(shadow, r.bottom_left(), r.top_right(), allowBlend);
									r = sides;	r.x.max = rect.x.min - 1.0f;	System::Video3DPrimitives::fill_rect_2d(shadow, r.bottom_left(), r.top_right(), allowBlend);
						}
					}

					{
						Range2 re = rect;
						for_count(int, borderIdx, 3)
						{
							re.expand_by(Vector2::one);
							System::Video3DPrimitives::rect_2d(Colour::red, re.bottom_left(), re.top_right(), allowBlend);
						}
					}

#ifdef AN_STANDARD_INPUT
					if (!::System::Input::get()->is_key_pressed(::System::Key::Tab))
#endif
					{
						{
							Range2 safeAreaPt = Framework::BullshotSystem::get_show_frame_safe_area_pt();
							if (safeAreaPt.x.min > 0.0f || safeAreaPt.x.max < 1.0f ||
								safeAreaPt.y.min > 0.0f || safeAreaPt.y.max < 1.0f)
							{
								Range2 safeArea;
								safeArea.x.min = rect.x.get_at(safeAreaPt.x.min);
								safeArea.x.max = rect.x.get_at(safeAreaPt.x.max);
								safeArea.y.min = rect.y.get_at(safeAreaPt.y.min);
								safeArea.y.max = rect.y.get_at(safeAreaPt.y.max);

								Colour notSafe = Colour::red.with_set_alpha(0.1f);

								Range2 r;

								r = rect;	r.y.min = safeArea.y.max + 1.0f;	System::Video3DPrimitives::fill_rect_2d(notSafe, r.bottom_left(), r.top_right(), allowBlend);
								r = rect;	r.y.max = safeArea.y.min - 1.0f;	System::Video3DPrimitives::fill_rect_2d(notSafe, r.bottom_left(), r.top_right(), allowBlend);

								Range2	sides = rect;
								sides.y = safeArea.y;

								r = sides;	r.x.min = safeArea.x.max + 1.0f;	System::Video3DPrimitives::fill_rect_2d(notSafe, r.bottom_left(), r.top_right(), allowBlend);
								r = sides;	r.x.max = safeArea.x.min - 1.0f;	System::Video3DPrimitives::fill_rect_2d(notSafe, r.bottom_left(), r.top_right(), allowBlend);
							}
						}

						Name sft = Framework::BullshotSystem::get_show_frame_template();
						if (sft == TXT("vr2") ||
							sft == TXT("vr23"))
						{
							if (rect.y.min > vRect.y.min) System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.5f, 0.0f)), Vector2(rect.get_at(Vector2(0.5f, 0.0f)).x, vRect.get_at(Vector2(0.5f, 0.0f)).y), allowBlend);
							if (rect.y.max < vRect.y.max) System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.5f, 1.0f)), Vector2(rect.get_at(Vector2(0.5f, 1.0f)).x, vRect.get_at(Vector2(0.5f, 1.0f)).y), allowBlend);
							if (rect.x.min > vRect.x.min) System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.0f, 0.5f)), Vector2(vRect.get_at(Vector2(0.0f, 0.5f)).x, rect.get_at(Vector2(0.0f, 0.5f)).y), allowBlend);
							if (rect.x.max < vRect.x.max) System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(1.0f, 0.5f)), Vector2(vRect.get_at(Vector2(1.0f, 0.5f)).x, rect.get_at(Vector2(1.0f, 0.5f)).y), allowBlend);
						}
						if (sft == TXT("vr3") ||
							sft == TXT("vr23"))
						{
							if (rect.y.min > vRect.y.min)
							{
								System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.33333f, 0.0f)), Vector2(rect.get_at(Vector2(0.33333f, 0.0f)).x, vRect.get_at(Vector2(0.33333f, 0.0f)).y), allowBlend);
								System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.66667f, 0.0f)), Vector2(rect.get_at(Vector2(0.66667f, 0.0f)).x, vRect.get_at(Vector2(0.66667f, 0.0f)).y), allowBlend);
							}
							if (rect.y.max < vRect.y.max)
							{
								System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.33333f, 1.0f)), Vector2(rect.get_at(Vector2(0.33333f, 1.0f)).x, vRect.get_at(Vector2(0.33333f, 1.0f)).y), allowBlend);
								System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.66667f, 1.0f)), Vector2(rect.get_at(Vector2(0.66667f, 1.0f)).x, vRect.get_at(Vector2(0.66667f, 1.0f)).y), allowBlend);
							}
							if (rect.x.min > vRect.x.min)
							{
								System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.0f, 0.33333f)), Vector2(vRect.get_at(Vector2(0.0f, 0.33333f)).x, rect.get_at(Vector2(0.0f, 0.33333f)).y), allowBlend);
								System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.0f, 0.66667f)), Vector2(vRect.get_at(Vector2(0.0f, 0.66667f)).x, rect.get_at(Vector2(0.0f, 0.66667f)).y), allowBlend);
							}
							if (rect.x.max < vRect.x.max)
							{
								System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(1.0f, 0.33333f)), Vector2(vRect.get_at(Vector2(1.0f, 0.33333f)).x, rect.get_at(Vector2(1.0f, 0.33333f)).y), allowBlend);
								System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(1.0f, 0.66667f)), Vector2(vRect.get_at(Vector2(1.0f, 0.66667f)).x, rect.get_at(Vector2(1.0f, 0.66667f)).y), allowBlend);
							}
						}
						if (sft == TXT("2"))
						{
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.5f, 0.0f)), rect.get_at(Vector2(0.5f, 1.0f)), allowBlend);
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.0f, 0.5f)), rect.get_at(Vector2(1.0f, 0.5f)), allowBlend);
						}
						if (sft == TXT("3"))
						{
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.33333f, 0.0f)), rect.get_at(Vector2(0.33333f, 1.0f)), allowBlend);
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.66667f, 0.0f)), rect.get_at(Vector2(0.66667f, 1.0f)), allowBlend);
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.0f, 0.33333f)), rect.get_at(Vector2(1.0f, 0.33333f)), allowBlend);
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.0f, 0.66667f)), rect.get_at(Vector2(1.0f, 0.66667f)), allowBlend);
						}
						if (sft == TXT("4") ||
							sft == TXT("4v"))
						{
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.25f, 0.0f)), rect.get_at(Vector2(0.25f, 1.0f)), allowBlend);
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.50f, 0.0f)), rect.get_at(Vector2(0.50f, 1.0f)), allowBlend);
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.75f, 0.0f)), rect.get_at(Vector2(0.75f, 1.0f)), allowBlend);
						}
						if (sft == TXT("gr") ||
							sft == TXT("g"))
						{
							float gr = 1.0f / 1.61803398875f;
							float igr = 1.0f - gr;
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(gr, 0.0f)), rect.get_at(Vector2(gr, 1.0f)), allowBlend);
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(igr, 0.0f)), rect.get_at(Vector2(igr, 1.0f)), allowBlend);
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.0f, gr)), rect.get_at(Vector2(1.0f, gr)), allowBlend);
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.0f, igr)), rect.get_at(Vector2(1.0f, igr)), allowBlend);
						}
						if (sft == TXT("gt") ||
							sft == TXT("g"))
						{
							float gr = 1.0f / 1.61803398875f;
							float igr = 1.0f - gr;
							// diagonals
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(0.0f, 0.0f)), rect.get_at(Vector2(1.0f, 1.0f)), allowBlend);
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(1.0f, 0.0f)), rect.get_at(Vector2(0.0f, 1.0f)), allowBlend);
							// golden diagonals
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(gr, 0.0f)), rect.get_at(Vector2(1.0f, 1.0f)), allowBlend);
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(gr, 1.0f)), rect.get_at(Vector2(1.0f, 0.0f)), allowBlend);
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(igr, 0.0f)), rect.get_at(Vector2(0.0f, 1.0f)), allowBlend);
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(igr, 1.0f)), rect.get_at(Vector2(0.0f, 0.0f)), allowBlend);
							// golden verticals
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(gr, 0.0f)), rect.get_at(Vector2(gr, 1.0f)), allowBlend);
							System::Video3DPrimitives::line_2d(Colour::red, rect.get_at(Vector2(igr, 0.0f)), rect.get_at(Vector2(igr, 1.0f)), allowBlend);
						}
					}
				}
			}

			System::RenderTarget::unbind_to_none();
		}
	}
#endif
}

void Game::do_extra_advances(float _deltaTime)
{
	{
		MEASURE_PERFORMANCE_COLOURED(gameHelpingSystems, Colour::zxYellow);

		Framework::IModulesOwner* imo = nullptr;
		{
			imo = playerTakenControl.get_actor();
			if (!imo)
			{
				imo = player.get_actor();
			}
		}

		if (environmentPlayer)
		{
			MEASURE_PERFORMANCE_COLOURED(environmentPlayer, Colour::zxRed);
			environmentPlayer->update(imo, _deltaTime);
		}
		if (musicPlayer)
		{
			MEASURE_PERFORMANCE_COLOURED(musicPlayer, Colour::zxRed);
			musicPlayer->update(imo, _deltaTime);
		}
		if (nonGameMusicPlayer)
		{
			MEASURE_PERFORMANCE_COLOURED(nonGameMusicPlayer, Colour::zxRed);
			nonGameMusicPlayer->update(_deltaTime);
		}
		if (voiceoverSystem)
		{
			MEASURE_PERFORMANCE_COLOURED(voiceoverSystem, Colour::zxRed);
			voiceoverSystem->update(_deltaTime);
		}
		if (subtitleSystem)
		{
			MEASURE_PERFORMANCE_COLOURED(subtitleSystem, Colour::zxRed);
			subtitleSystem->update(_deltaTime);
		}
	}

	advance_camera_rumble(_deltaTime);

	{
		scriptedSoundDullness.current = blend_to_using_speed_based_on_time(scriptedSoundDullness.current, scriptedSoundDullness.target, scriptedSoundDullness.blendTime, _deltaTime);
		scriptedTint.active = blend_to_using_time(scriptedTint.active, scriptedTint.targetActive, scriptedTint.blendTime, _deltaTime);
		scriptedTint.current = blend_to_using_time(scriptedTint.current, scriptedTint.target, scriptedTint.blendTime, _deltaTime);
	}
}

void Game::advance_world(Optional<float> _withDeltaTimeOverride)
{
	MEASURE_PERFORMANCE(advanceWorld);

	{
		MEASURE_PERFORMANCE_COLOURED(world_updateSeenByPlayer, Colour::blue);
		todo_multiplayer_issue(TXT("handle multiple players too"));
		if (auto* p = player.get_actor())
		{
			// update visibility/visited by player
			Framework::Room::update_player_related(p, true, true);
		}
		if (auto* p = playerTakenControl.get_actor())
		{
			// update visibility/visited by player
			Framework::Room::update_player_related(p, true, true);
		}
	}

	base::advance_world(_withDeltaTimeOverride);

	if (world)
	{
		debug_draw();
	}
}

void Game::advance_world__post_block()
{
	base::advance_world__post_block();

#ifdef AN_TWEAKS
	if (Framework::Actor* playerActor = player.get_actor())
	{
		if (Framework::Room* inRoom = playerActor->get_presence()->get_in_room())
		{
			Framework::TweakingContext::set(Names::currentEnviornment, inRoom->get_environment());
		}
	}
#endif
}

void Game::game_loop()
{
	MEASURE_PERFORMANCE(gameThread);

	base::game_loop();

	PhysicalSensations::advance(get_delta_time());

#ifdef AN_PERFORMANCE_MEASURE
	if (! noGameLoopUntilShowPerformance)
#endif
	{
		Framework::GameAdvanceContext gameAdvanceContext;
		setup_game_advance_context(REF_ gameAdvanceContext);

		// advance scene stack, if any scene requests to not advance scenes below, stop updating right there
		// at the bottom there should be "AdvanceWorld" scene
		if (sceneLayerStack.is_set())
		{
			sceneLayerStack->advance(gameAdvanceContext);
		}
		else if (worldInUse)
		{
			// if there are no scenes, go with normal advance_world
			advance_world(); // if it was advanced by mode, we won't do anything
		}
	}

#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (debugSelectDebugSubject ||
		debugQuickSelectDebugSubject ||
		debugTestCollisions != 0)
	{
		if (auto * playerActor = player.get_actor())
		{
			Transform placement;
			Framework::Room* inRoom = nullptr;
			Collision::ICollidableObject* avoid = nullptr;

			{
				Framework::Render::Camera camera;
				camera.set_placement_based_on(playerActor, ::Framework::CameraPlacementBasedOn::Eyes);
				placement = camera.get_placement();
				inRoom = camera.get_in_room();
				avoid = playerActor;
			}
			
			if (debugCameraMode != 0)
			{
				placement = debugCameraPlacement;
				inRoom = debugCameraInRoom.get();
			}

			if (inRoom)
			{
				Framework::CollisionTrace collisionTrace(inRoom);

				Transform startPlacement = placement;

				Vector3 startLocation = startPlacement.get_translation();
				Vector3 endLocation = startLocation + startPlacement.vector_to_world(Vector3::yAxis * 50.0f);
				collisionTrace.add_location(startLocation);
				collisionTrace.add_location(endLocation);

				Framework::CheckSegmentResult result;
				int collisionTraceFlags = Framework::CollisionTraceFlag::ResultInPresenceRoom | Framework::CollisionTraceFlag::StartInProvidedRoom;
				Framework::CheckCollisionContext checkCollisionContext;
				checkCollisionContext.avoid(avoid);
				checkCollisionContext.collision_info_needed();

#ifdef AN_DEVELOPMENT
				{
					debug_context(inRoom);
					debug_draw_line(true, Colour::magenta,
						startLocation + Vector3(Random::get_float(-0.01f, 0.01f), Random::get_float(-0.01f, 0.01f), Random::get_float(-0.01f, 0.01f)),
						endLocation + Vector3(Random::get_float(-0.01f, 0.01f), Random::get_float(-0.01f, 0.01f), Random::get_float(-0.01f, 0.01f)));
					debug_no_context();
				}
#endif

				if (debugSelectDebugSubject || debugQuickSelectDebugSubject)
				{
					debugSelectDebugSubjectHit = nullptr;
				}
				if (debugTestCollisions != 0)
				{
					debugTestCollisionsHit = nullptr;
					debugTestCollisionsHitShape = nullptr;
					debugTestCollisionsHitModel = nullptr;
				}
				int collisionFlags = 0;
				if (debugTestCollisions == 0)
				{
					collisionFlags = NONE;
				}
				if (debugTestCollisions == 1)
				{
					collisionFlags = AgainstCollision::Movement;
				}
				if (debugTestCollisions == 2)
				{
					collisionFlags = AgainstCollision::Precise;
				}
				if (playerActor->get_presence()->trace_collision((AgainstCollision::Type)collisionFlags, collisionTrace, result, collisionTraceFlags, checkCollisionContext))
				{
					if (debugSelectDebugSubject || debugQuickSelectDebugSubject)
					{
						debugSelectDebugSubjectHit = result.object;
					}
					if (debugTestCollisions != 0)
					{
						debugTestCollisionsHit = result.object;
						debugTestCollisionsHitShape = result.shape;
#ifdef AN_DEVELOPMENT
						debugTestCollisionsHitModel = result.model;
#endif
					}
#ifdef AN_DEVELOPMENT
					{
						debug_context(result.inRoom);
						debug_draw_line(true, Colour::red, result.hitLocation, result.hitLocation + result.hitNormal * 0.1f);
						debug_draw_sphere(true, true, Colour::red, 0.2f, Sphere(result.hitLocation, 0.02f));
						debug_no_context();
					}
#endif
				}
				if (debugSelectDebugSubject || debugQuickSelectDebugSubject)
				{
#ifdef AN_DEBUG_RENDERER
					if (auto * imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
					{
						DebugRenderer::get()->set_active_subject(imo);
					}
					else
					{
						DebugRenderer::get()->set_active_subject(debugSelectDebugSubjectHit.get());
					}
#endif
				}
			}
		}
	}

#ifdef BUILD_NON_RELEASE
	if (::System::RenderTargetSaveSpecialOptions::get().transparent)
	{
		::System::RenderTargetSaveSpecialOptions::access().onlyObject = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get());
	}
	else
	{
		::System::RenderTargetSaveSpecialOptions::access().onlyObject = nullptr;
	}
#endif
#endif
}

void Game::start_mode(Framework::GameModeSetup* _modeSetup)
{
	base::start_mode(_modeSetup);

	// reset for continuous advancement
	ready_for_continuous_advancement();
}

void Game::end_mode(bool _abort)
{
	base::end_mode(_abort);

	if (fading.current >= 0.999f)
	{
		// stop everything, if we're faded out, we shouldn't be hearing anything
		end_all_sounds();
	}
	else
	{
		// stop all sounds in the world immediately
		// this should clear sound scene and environment
		no_world_to_sound();
	}

	overlayInfoSystem->force_clear();
}

void Game::remove_actors_from_pilgrimage()
{
	perform_sync_world_job(TXT("remove actors from pilgrimage"), [this](){sync_remove_actors_from_pilgrimage(); });
}

void Game::sync_remove_actors_from_pilgrimage()
{
	ASSERT_SYNC;

	world->suspend_ai_messages();

	todo_note(TXT("maybe we should do it a little bit different?"));

	// unbind player before actor gets deleted
	player.unbind_actor(); // to make sure? it should be unbound
	
	world->suspend_ai_messages(false);
}

void Game::async_create_world(Random::Generator _withGenerator)
{
	base::async_create_world(_withGenerator);

	perform_sync_world_job(TXT("create utility region"), [this]()
	{
		Random::Generator rg;
		rg.set_seed(0, 0);
		utilityRegion = new Framework::Region(subWorld, nullptr, nullptr, rg);
	});

	{
		Framework::ActivationGroupPtr activationGroup = push_new_activation_group();
		perform_sync_world_job(TXT("create locker room"), [this]()
		{
			Random::Generator rg;
			rg.set_seed(0, 0);
			lockerRoom = new Framework::Room(subWorld, utilityRegion.get(), nullptr, rg);
			lockerRoom->set_always_suspend_advancement();
			lockerRoom->access_tags().set_tag(NAME(lockerRoom));
		});
		request_ready_and_activate_all_inactive(get_current_activation_group());
		pop_activation_group(activationGroup.get());
	}

#ifdef AN_OUTPUT_WORLD_GENERATION
	output(TXT("locker room = r%p"), lockerRoom.get());
#endif
}

#ifdef AN_DEVELOPMENT_OR_PROFILER
void Game::select_next_ai_to_debug(bool _next)
{
	Collision::ICollidableObject const * firstOne = nullptr;
	Collision::ICollidableObject const * lastOne = nullptr;
	bool found = false;

	{
		for_every_ptr(object, get_world()->get_all_objects())
		{
			if (object->get_as_object() &&
				object->get_ai())
			{
				if (!firstOne)
				{
					firstOne = object;
				}
				if (!_next && object == debugSelectDebugSubjectHit.get() && lastOne) // if no last one at this point, we will choose it with if (!found)
				{
					debugSelectDebugSubjectHit = lastOne;
					found = true;
					break;
				}
				if (_next && lastOne == debugSelectDebugSubjectHit.get())
				{
					debugSelectDebugSubjectHit = object;
					found = true;
					break;
				}
				lastOne = object;
			}
		}
	}
	if (!found)
	{
		debugSelectDebugSubjectHit = _next? firstOne : lastOne;
	}
#ifdef AN_DEBUG_RENDERER
	DebugRenderer::get()->set_active_subject(fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()));
#endif
}

void Game::select_next_ai_in_room_to_debug(bool _next)
{
	Collision::ICollidableObject const * firstOne = nullptr;
	Collision::ICollidableObject const * lastOne = nullptr;
	bool found = false;

	Framework::Room* inRoom = nullptr;
	if (auto* actor = player.get_actor())
	{
		inRoom = actor->get_presence()->get_in_room();
	}
	if (debugCameraMode)
	{
		inRoom = debugCameraInRoom.get();
	}
	if (inRoom)
	{
		//FOR_EVERY_OBJECT_IN_ROOM(object, inRoom)
		for_every_ptr(object, inRoom->get_objects())
		{
			if (object->get_as_object() &&
				object->get_ai())
			{
				if (!firstOne)
				{
					firstOne = object;
				}
				if (!_next && object == debugSelectDebugSubjectHit.get() && lastOne) // if no last one at this point, we will choose it with if (!found)
				{
					debugSelectDebugSubjectHit = lastOne;
					found = true;
					break;
				}
				if (_next && lastOne == debugSelectDebugSubjectHit.get())
				{
					debugSelectDebugSubjectHit = object;
					found = true;
					break;
				}
				lastOne = object;
			}
		}
	}
	if (!found)
	{
		debugSelectDebugSubjectHit = _next? firstOne : lastOne;
	}
#ifdef AN_DEBUG_RENDERER
	DebugRenderer::get()->set_active_subject(fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()));
#endif
}

void Game::select_next_in_room_to_debug(bool _next)
{
	Collision::ICollidableObject const * firstOne = nullptr;
	Collision::ICollidableObject const * lastOne = nullptr;
	bool found = false;

	Framework::Room* inRoom = nullptr;
	if (auto* actor = player.get_actor())
	{
		inRoom = actor->get_presence()->get_in_room();
	}
	if (debugCameraMode)
	{
		inRoom = debugCameraInRoom.get();
	}
	if (inRoom)
	{
		//FOR_EVERY_OBJECT_IN_ROOM(object, inRoom)
		for_every_ptr(object, inRoom->get_objects())
		{
			if (object->get_as_object())
			{
				if (!firstOne)
				{
					firstOne = object;
				}
				if (!_next && object == debugSelectDebugSubjectHit.get() && lastOne) // if no last one at this point, we will choose it with if (!found)
				{
					debugSelectDebugSubjectHit = lastOne;
					found = true;
					break;
				}
				if (_next && lastOne == debugSelectDebugSubjectHit.get())
				{
					debugSelectDebugSubjectHit = object;
					found = true;
					break;
				}
				lastOne = object;
			}
		}
	}
	if (!found)
	{
		debugSelectDebugSubjectHit = _next? firstOne : lastOne;
	}
#ifdef AN_DEBUG_RENDERER
	DebugRenderer::get()->set_active_subject(fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()));
#endif
}

void Game::select_next_ai_manager_to_debug(bool _next)
{
	Collision::ICollidableObject const * firstOne = nullptr;
	Collision::ICollidableObject const * lastOne = nullptr;
	bool found = false;

	DEFINE_STATIC_NAME_STR(aiManager, TXT("ai manager"));
	Framework::LibraryName aiManagerName = Framework::Library::get_current()->get_global_references().get_name(NAME(aiManager));
	for_every_ptr(object, get_world()->get_all_objects())
	{
		if (object->get_as_object() &&
			object->get_as_object()->get_object_type() &&
			object->get_as_object()->get_object_type()->get_name() == aiManagerName)
		{
			if (!firstOne)
			{
				firstOne = object;
			}
			if (!_next && object == debugSelectDebugSubjectHit.get() && lastOne) // if no last one at this point, we will choose it with if (!found)
			{
				debugSelectDebugSubjectHit = lastOne;
				found = true;
				break;
			}
			if (_next && lastOne == debugSelectDebugSubjectHit.get())
			{
				debugSelectDebugSubjectHit = object;
				found = true;
				break;
			}
			lastOne = object;
		}
	}
	if (!found)
	{
		debugSelectDebugSubjectHit = _next? firstOne : lastOne;
	}
#ifdef AN_DEBUG_RENDERER
	DebugRenderer::get()->set_active_subject(fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()));
#endif
}

void Game::select_next_region_manager_to_debug(bool _next)
{
	Collision::ICollidableObject const * firstOne = nullptr;
	Collision::ICollidableObject const * lastOne = nullptr;
	bool found = false;

	for_every_ptr(object, get_world()->get_all_objects())
	{
		if (auto* ai = object->get_ai())
		{
			if (auto* mind = ai->get_mind())
			{
				if (fast_cast<AI::Logics::Managers::RegionManager>(mind->get_logic()))
				{
					if (!firstOne)
					{
						firstOne = object;
					}
					if (!_next && object == debugSelectDebugSubjectHit.get() && lastOne) // if no last one at this point, we will choose it with if (!found)
					{
						debugSelectDebugSubjectHit = lastOne;
						found = true;
						break;
					}
					if (_next && lastOne == debugSelectDebugSubjectHit.get())
					{
						debugSelectDebugSubjectHit = object;
						found = true;
						break;
					}
					lastOne = object;
				}
			}
		}
	}
	if (!found)
	{
		debugSelectDebugSubjectHit = _next? firstOne : lastOne;
	}
#ifdef AN_DEBUG_RENDERER
	DebugRenderer::get()->set_active_subject(fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()));
#endif
}

void Game::select_next_dynamic_spawn_manager_to_debug(bool _next)
{
	Collision::ICollidableObject const * firstOne = nullptr;
	Collision::ICollidableObject const * lastOne = nullptr;
	bool found = false;

	for_every_ptr(object, get_world()->get_all_objects())
	{
		if (auto* ai = object->get_ai())
		{
			if (auto* mind = ai->get_mind())
			{
				if (auto* sm = fast_cast<AI::Logics::Managers::SpawnManager>(mind->get_logic()))
				{
					if (sm->has_dynamic_spawns())
					{
						if (!firstOne)
						{
							firstOne = object;
						}
						if (!_next && object == debugSelectDebugSubjectHit.get() && lastOne) // if no last one at this point, we will choose it with if (!found)
						{
							debugSelectDebugSubjectHit = lastOne;
							found = true;
							break;
						}
						if (_next && lastOne == debugSelectDebugSubjectHit.get())
						{
							debugSelectDebugSubjectHit = object;
							found = true;
							break;
						}
						lastOne = object;
					}
				}
			}
		}
	}
	if (!found)
	{
		debugSelectDebugSubjectHit = _next? firstOne : lastOne;
	}
#ifdef AN_DEBUG_RENDERER
	DebugRenderer::get()->set_active_subject(fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()));
#endif
}

void Game::select_next_game_script_executor_to_debug(bool _next)
{
	Collision::ICollidableObject const * firstOne = nullptr;
	Collision::ICollidableObject const * lastOne = nullptr;
	bool found = false;

	DEFINE_STATIC_NAME_STR(gameScriptExecutor, TXT("game script executor"));
	Framework::LibraryName gameScriptExecutorName = Framework::Library::get_current()->get_global_references().get_name(NAME(gameScriptExecutor));
	for_every_ptr(object, get_world()->get_all_objects())
	{
		if (object->get_as_object() &&
			object->get_as_object()->get_object_type() &&
			object->get_as_object()->get_object_type()->get_name() == gameScriptExecutorName)
		{
			if (!firstOne)
			{
				firstOne = object;
			}
			if (!_next && object == debugSelectDebugSubjectHit.get() && lastOne) // if no last one at this point, we will choose it with if (!found)
			{
				debugSelectDebugSubjectHit = lastOne;
				found = true;
				break;
			}
			if (_next && lastOne == debugSelectDebugSubjectHit.get())
			{
				debugSelectDebugSubjectHit = object;
				found = true;
				break;
			}
			lastOne = object;
		}
	}
	if (!found)
	{
		debugSelectDebugSubjectHit = _next? firstOne : lastOne;
	}
#ifdef AN_DEBUG_RENDERER
	DebugRenderer::get()->set_active_subject(fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()));
#endif
}

void Game::log_room_region(LogInfoContext& _log, DebugRoomRegion& _debugRoomRegion)
{
	Framework::Room* inRoom = nullptr;
	if (auto* actor = player.get_actor())
	{
		inRoom = actor->get_presence()->get_in_room();
	}
	if (debugCameraMode)
	{
		inRoom = debugCameraInRoom.get();
	}
	if (_debugRoomRegion.forRoom != inRoom)
	{
		_debugRoomRegion.forRoom = inRoom;
		_debugRoomRegion.info.clear();

		_debugRoomRegion.info.log(TXT("in room: %p : %S"), inRoom, inRoom ? inRoom->get_name().to_char() : TXT("--"));
		_debugRoomRegion.info.log(TXT("   type: %S"), inRoom && inRoom->get_room_type() ? inRoom->get_room_type()->get_name().to_string().to_char() : TXT("--"));

		{
			LOG_INDENT(_debugRoomRegion.info);
			Framework::Region* region = inRoom->get_in_region();
			while (region)
			{
				if (auto* rt = region->get_region_type())
				{
					_debugRoomRegion.info.log(TXT("region: %p : %S"), region, rt->get_name().to_string().to_char());
					_debugRoomRegion.info.log(TXT("  tags: %S"), rt->get_tags().to_string().to_char());
				}
				region = region->get_in_region();
			}
		}
	}
	{
		_log.log(_debugRoomRegion.info);
	}
}

void Game::log_high_level_info_region(LogInfoContext& _log, DebugHighLevelRegion& _debugHighLevelRegion, Name const& _highLevelRegionTag, Name const& _highLevelRegionInfoTag)
{
	Framework::Room* inRoom = nullptr;
	if (auto* actor = player.get_actor())
	{
		inRoom = actor->get_presence()->get_in_room();
	}
	if (debugCameraMode)
	{
		inRoom = debugCameraInRoom.get();
	}
	if (_debugHighLevelRegion.highLevelRegionInfoFor != inRoom)
	{
		_debugHighLevelRegion.highLevelRegionInfoFor = inRoom;
		_debugHighLevelRegion.highLevelRegionInfo.clear();

		_debugHighLevelRegion.highLevelRegion = inRoom->get_in_region();
		while (_debugHighLevelRegion.highLevelRegion && _debugHighLevelRegion.highLevelRegion->get_in_region())
		{
			if (_debugHighLevelRegion.highLevelRegion->get_tags().get_tag_as_int(_highLevelRegionTag))
			{
				break;
			}
			_debugHighLevelRegion.highLevelRegion = _debugHighLevelRegion.highLevelRegion->get_in_region();
		}
		if (_debugHighLevelRegion.highLevelRegion)
		{
			_debugHighLevelRegion.highLevelRegionInfo.log(TXT("high level region: %S"),
				_debugHighLevelRegion.highLevelRegion->get_region_type() ? _debugHighLevelRegion.highLevelRegion->get_region_type()->get_name().to_string().to_char() : TXT("no type"));
		}
		else
		{
			_debugHighLevelRegion.highLevelRegionInfo.log(TXT("no high level region!"));
		}
		{
			LOG_INDENT(_debugHighLevelRegion.highLevelRegionInfo);
			Framework::Region* region = inRoom->get_in_region();
			while (region)
			{
				if (region->get_tags().get_tag_as_int(_highLevelRegionInfoTag))
				{
					if (auto* rt = region->get_region_type())
					{
						if (auto* rgb = fast_cast<RegionGenerators::Base>(rt->get_region_generator_info()))
						{
							_debugHighLevelRegion.highLevelRegionInfo.log(TXT("region: %S"), rt->get_name().to_string().to_char());
							_debugHighLevelRegion.highLevelRegionInfo.log(TXT("  tags: %S"), rt->get_tags().to_string().to_char());
							LOG_INDENT(_debugHighLevelRegion.highLevelRegionInfo);
							rgb->get_ai_managers().log(_debugHighLevelRegion.highLevelRegionInfo);
						}
					}
				}
				if (region == _debugHighLevelRegion.highLevelRegion)
				{
					break;
				}
				region = region->get_in_region();
			}
		}
		for_every_ptr(object, get_world()->get_all_objects())
		{
			DEFINE_STATIC_NAME_STR(aiManager, TXT("ai manager"));
			Framework::LibraryName aiManagerName = Framework::Library::get_current()->get_global_references().get_name(NAME(aiManager));
			if (object->get_as_object() &&
				object->get_as_object()->get_object_type() &&
				object->get_as_object()->get_object_type()->get_name() == aiManagerName)
			{
				if (object->get_presence() &&
					object->get_presence()->get_in_room() &&
					object->get_presence()->get_in_room()->is_in_region(cast_to_nonconst(_debugHighLevelRegion.highLevelRegion)))
				{
					if (auto* ai = object->get_ai())
					{
						if (auto* mind = ai->get_mind())
						{
							if (auto* m = mind->get_mind())
							{
								if (m->get_tags().get_tag_as_int(_highLevelRegionTag))
								{
									if (auto* logic = fast_cast<AI::Logics::Managers::RegionManager>(mind->get_logic()))
									{
										_debugHighLevelRegion.highLevelRegionInfoManager = object;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	{
		_log.set_colour(Colour::orange);
		_log.log(TXT("REGION INFO"));
		_log.set_colour();
		LOG_INDENT(_log);
		_log.log(_debugHighLevelRegion.highLevelRegionInfo);
	}
	if (_debugHighLevelRegion.highLevelRegionInfoManager.get())
	{
		if (auto* ai = _debugHighLevelRegion.highLevelRegionInfoManager->get_ai())
		{
			if (auto* mind = ai->get_mind())
			{
				if (auto* logic = mind->get_logic())
				{
					_log.log(TXT(""));
					_log.set_colour(Colour::orange);
					_log.log(TXT("REGION MANAGER"));
					_log.set_colour();
					LOG_INDENT(_log);
					logic->log_logic_info(_log);
				}
			}
		}
	}
}
#endif

bool Game::should_input_be_grabbed() const
{
	if (showDebugPanel
#ifdef AN_PERFORMANCE_MEASURE
		|| showPerformance
#endif
		)
	{
		return false;
	}
	else if (debugCameraMode)
	{
		return true;
	}
	else
	{
		return base::should_input_be_grabbed();
	}
}

tchar const* quickGamePlayerProfile = TXT("_quickgame");
// legacy are here mostly to get rid of those files
tchar const* quickGamePlayerProfileLegacy = TXT("_default");
tchar const* quickGamePlayerProfileLegacy2 = TXT("default");

String const& Game::get_player_profile_name() const
{
	if (auto* c = GameConfig::get_as<GameConfig>())
	{
		if (!c->get_player_profile_name().is_empty())
		{
			return c->get_player_profile_name();
		}
	}
	return String::empty();
}

String Game::get_whole_path_to_player_profile_file(Optional<String> const& _profile) const
{
	return IO::get_user_game_data_directory() + get_player_profile_directory() + get_player_profile_file(_profile);
}

String Game::get_whole_path_to_game_slot_file(Optional<String> const & _profileName, PlayerGameSlot const* _gameSlot) const
{
	if (!_gameSlot)
	{
		_gameSlot = playerSetup.get_active_game_slot();
	}
	return IO::get_user_game_data_directory() + get_player_profile_directory() + get_player_profile_file_without_suffix(_profileName) + get_game_slot_filename_suffix() + String::printf(TXT(".%05i"), _gameSlot? _gameSlot->id : 0);
}

String Game::get_player_profile_file_without_suffix(Optional<String> const & _profile) const
{
	if (_profile.is_set())
	{
		return _profile.get();
	}
	if (!useQuickGameProfile)
	{
		if (auto* c = GameConfig::get_as<GameConfig>())
		{
			if (!c->get_player_profile_name().is_empty())
			{
				return c->get_player_profile_name().to_lower();
			}
		}
	}

	return String(quickGamePlayerProfile); // will fail loading anyway
}

String Game::get_player_profile_file(Optional<String> const & _profile) const
{
	return get_player_profile_file_without_suffix(_profile) + get_player_profile_filename_suffix();
}

bool Game::legacy__does_require_updating_player_profiles() const
{
	IO::Dir dir;
	if (dir.list(IO::get_user_game_data_directory() + get_player_profile_directory()))
	{
		String suffix(TXT(".xml"));
		for_every(file, dir.get_files())
		{
			String lowerFile = file->to_lower();
			if (lowerFile.has_suffix(suffix))
			{
				return true;
			}
		}
	}
	return false;
}

void Game::legacy__update_player_profiles()
{
	IO::Dir dir;
	if (dir.list(IO::get_user_game_data_directory() + get_player_profile_directory()))
	{
		String suffix(TXT(".xml"));
		for_every(file, dir.get_files())
		{
			String lowerFile = file->to_lower();
			if (lowerFile.has_suffix(suffix))
			{
				String fullPath = IO::get_user_game_data_directory() + get_player_profile_directory() + *file;
				bool loaded = false;
				{
					IO::XML::Document doc;
					if (load_xml_using_temp_file(doc, fullPath))
					{
						for_every(node, doc.get_root()->children_named(TXT("playerSetup")))
						{
							playerSetup.legacy_load_from_xml(node);
						}
						for_every(node, doc.get_root()->children_named(TXT("playerProfile")))
						{
							Array<IO::XML::Node const*> emptyGameSlotNodeArray;
							playerSetup.load_from_xml(node, emptyGameSlotNodeArray);
							for_every(configNode, node->children_named(TXT("config")))
							{
								for_every(subConfigNode, configNode->all_children())
								{
									if (subConfigNode->get_name() == TXT("vrConfig"))
									{
										MainConfig::access_global().load_user_vr_from_xml(subConfigNode);
									}
								}
							}
						}
						playerSetup.access_preferences().apply();
						loaded = true;
					}
					else
					{
						error(TXT("could not open player setup file"));
					}
				}
				if (loaded)
				{
					// save to separate game slot files
					save_player_profile_to_file(file->replace(suffix, String::empty()), true);
				}
				IO::File::delete_file(fullPath);
			}
		}
	}

	// load the actual last one
	load_player_profile();
}

void Game::update_player_profiles_list(bool _selectPlayerProfileAndStoreInConfig)
{
	playerProfiles.clear();

	IO::Dir dir;
	if (dir.list(IO::get_user_game_data_directory() + get_player_profile_directory()))
	{
		String suffix(get_player_profile_filename_suffix());
		String suffixTemp(suffix + TXT(".temp"));
		for_every(file, dir.get_files())
		{
			String lowerFile = file->to_lower();
			String const * workingSuffix = nullptr;
			if (lowerFile.has_suffix(suffix))
			{
				workingSuffix = &suffix;
			}
			/*
			else if (lowerFile.has_suffix(suffixTemp))
			{
				workingSuffix = &suffixTemp;
			}
			*/
			if (workingSuffix)
			{
				String profileName = file->get_left(file->get_length() - workingSuffix->get_length());
				String lowerProfileName = profileName.to_lower();
				// delete all quick game player profiles
				// allow "default" to exist, it was a long time ago anyway
				if (lowerProfileName == quickGamePlayerProfile ||
					lowerProfileName == quickGamePlayerProfileLegacy/* ||
					lowerProfileName == quickGamePlayerProfileLegacy2*/)
				{
					String quickGameLegacyFile = IO::get_user_game_data_directory() + get_player_profile_directory() + *file;
					/*
					String quickGameFile = IO::get_user_game_data_directory() + get_player_profile_directory() + quickGamePlayerProfile + get_player_profile_filename_suffix();
					if (!IO::File::does_exist(quickGameFile))
					{
						IO::File::rename_file(quickGameLegacyFile, quickGameFile);
					}
					else
					{
					*/
						IO::File::delete_file(quickGameLegacyFile);
					/*
					}
					*/
				}
				else if (lowerProfileName != quickGamePlayerProfile)
				{
					playerProfiles.push_back_unique(file->get_left(file->get_length() - workingSuffix->get_length()));
				}
			}
		}
	}
	sort(playerProfiles, String::compare_icase_sort);
	if (_selectPlayerProfileAndStoreInConfig)
	{
		String selectedProfile;
		if (auto* c = TeaForGodEmperor::GameConfig::get_as<TeaForGodEmperor::GameConfig>())
		{
			selectedProfile = c->get_player_profile_name();
		}
		if (playerProfiles.is_empty())
		{
			selectedProfile = String::empty();
		}
		else
		{
			if (selectedProfile.is_empty() || !playerProfiles.does_contain(selectedProfile))
			{
				selectedProfile = playerProfiles.get_first();
			}
		}
		if (auto* c = GameConfig::get_as<GameConfig>())
		{
			c->set_player_profile_name(selectedProfile);
		}
	}
}

void Game::use_quick_game_player_profile(bool _useQuickGameProfile, Optional<String> const & _andChooseProfile)
{
	if (playerProfileExists && playerProfileRequiresSaving && playerProfileSaveTimeStamp.get_time_since() > 0.5f)
	{
		warn(TXT("player profile was not saved recently, auto saving"));
		save_player_profile();
	}
	useQuickGameProfile = _useQuickGameProfile;
	if (!useQuickGameProfile)
	{
		if (_andChooseProfile.is_set())
		{
			choose_player_profile(_andChooseProfile.get());
			return;
		}
		else
		{
			if (auto* c = GameConfig::get_as<GameConfig>())
			{
				if (!c->get_player_profile_name().is_empty())
				{
					choose_player_profile(c->get_player_profile_name());
					return;
				}
			}
		}
	}
	choose_player_profile(String(quickGamePlayerProfile), true);
	if (useQuickGameProfile)
	{
		playerSetup.setup_for_quick_game_player_profile();
	}
}

void Game::rename_player_profile(String const& _previousName, String const& _newName)
{
	save_player_profile();
	String filenamePrev = get_whole_path_to_player_profile_file(_previousName);
	String filenameNew = get_whole_path_to_player_profile_file(_newName);
	if (IO::File::does_exist(filenamePrev))
	{
		// not all game slots are loaded now, we will be just renaming files
		Array<String> gameSlotFiles;

		// get all previous slots
		{
			IO::Dir dir;
			if (dir.list(IO::get_user_game_data_directory() + get_player_profile_directory()))
			{
				// profilename.gameslot.00000
				String requiredPrefix = get_player_profile_file_without_suffix() + get_game_slot_filename_suffix();
				requiredPrefix = requiredPrefix.to_lower();

				// gather all files
				for_every(file, dir.get_files())
				{
					String lowerFile = file->to_lower();
					if (lowerFile.has_prefix(requiredPrefix) &&
						!is_temp_file(lowerFile))
					{
						gameSlotFiles.push_back(*file);
					}
				}
			}
		}

		// rename main file
		IO::File::delete_file(filenameNew);
		IO::File::rename_file(filenamePrev, filenameNew);
		if (auto* c = GameConfig::get_as<GameConfig>())
		{
			c->set_player_profile_name(_newName);
		}

		// delete existing game slots for new name
		{
			IO::Dir dir;
			if (dir.list(IO::get_user_game_data_directory() + get_player_profile_directory()))
			{
				// profilename.gameslot.00000
				String requiredPrefix = get_player_profile_file_without_suffix() + get_game_slot_filename_suffix();
				requiredPrefix = requiredPrefix.to_lower();

				// gather all files
				for_every(file, dir.get_files())
				{
					String lowerFile = file->to_lower();
					if (lowerFile.has_prefix(requiredPrefix) &&
						!is_temp_file(lowerFile))
					{
						IO::File::delete_file(IO::get_user_game_data_directory() + get_player_profile_directory() + *file);
					}
				}
			}
		}

		// rename game slot files
		{
			for_every(file, gameSlotFiles)
			{
				String gameslotFilenamePrev = *file;
				String gameslotFilenameNew = gameslotFilenamePrev.replace(_previousName, _newName);
				gameslotFilenamePrev = IO::get_user_game_data_directory() + get_player_profile_directory() + gameslotFilenamePrev;
				gameslotFilenameNew = IO::get_user_game_data_directory() + get_player_profile_directory() + gameslotFilenameNew;
				IO::File::delete_file(gameslotFilenameNew);
				IO::File::rename_file(gameslotFilenamePrev, gameslotFilenameNew);
			}
		}

		// reload the player profile now
		load_player_profile();
	}
	{
		// remove prev game slots
		for_every_ref(gs, playerSetup.get_game_slots())
		{
			String filename = get_whole_path_to_game_slot_file(_previousName, gs);
			if (IO::File::does_exist(filename))
			{
				IO::File::delete_file(filename);
			}
		}
	}
}

void Game::remove_player_profile()
{
	{	// remove game slots
		for_every_ref(gs, playerSetup.get_game_slots())
		{
			String filename = get_whole_path_to_game_slot_file(NP, gs);
			if (IO::File::does_exist(filename))
			{
				IO::File::delete_file(filename);
			}
		}
	}
	String filename = get_whole_path_to_player_profile_file();
	if (IO::File::does_exist(filename))
	{
		IO::File::delete_file(filename);
		if (auto* c = GameConfig::get_as<GameConfig>())
		{
			c->set_player_profile_name(String::empty());
		}
	}
}

bool Game::load_existing_player_profile(bool _loadGameSlots)
{
	bool loaded = load_player_profile(_loadGameSlots);
	if (!loaded)
	{
		if (!useQuickGameProfile)
		{
			// clear it
			if (auto* c = GameConfig::get_as<GameConfig>())
			{
				c->set_player_profile_name(String::empty());
			}
		}
	}
	return loaded;
}

void Game::create_and_choose_default_player(bool _loadGameSlots)
{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("create default player"));
#endif
	todo_note(TXT("use localisation?"));
	String defaultProfileName(DEFAULT_PLAYER_NAME);
	use_quick_game_player_profile(false);
	choose_player_profile(defaultProfileName, true, _loadGameSlots);
}

void Game::choose_player_profile(String const& _profile, bool _addIfDoesntExist, bool _loadGameSlots)
{
	output(TXT("choose player profile \"%S\""), _profile.to_char());

	if (playerProfileExists && playerProfileRequiresSaving && playerProfileSaveTimeStamp.get_time_since() > 0.5f)
	{
		warn(TXT("player profile was not saved recently, auto saving"));
		save_player_profile();
	}

	if (!useQuickGameProfile)
	{
		// store it as it isn't the default one forced
		if (auto* c = GameConfig::get_as<GameConfig>())
		{
			c->set_player_profile_name(_profile);
		}
	}

	bool loaded = load_player_profile(_loadGameSlots);
	playerProfileExists = loaded || _addIfDoesntExist;
	playerProfileRequiresSaving = playerProfileExists;
	playerProfileSaveTimeStamp.reset();

	if (! loaded &&
		!_addIfDoesntExist)
	{
		playerProfileExists = false;
		if (!useQuickGameProfile)
		{
			// clear it
			if (auto* c = GameConfig::get_as<GameConfig>())
			{
				c->set_player_profile_name(String::empty());
			}
		}
	}
}

bool Game::load_player_profile(bool _loadGameSlots)
{
	if (wantsToCreateMainConfigFile)
	{
		return false;
	}
	bool loaded = false;
	playerProfileLoadedWithGameSlots = _loadGameSlots;
	playerSetup.reset(); // reset!
	if (get_player_profile_file_without_suffix() != quickGamePlayerProfile) // don't load quick game profile
	{
		IO::XML::Document mainDoc;
		String playerProfileFileName = get_whole_path_to_player_profile_file();
		if (load_xml_using_temp_file(mainDoc, playerProfileFileName))
		{
#ifdef AN_DEVELOPMENT_OR_PROFILER
			output(TXT("load player profile from \"%S\""), playerProfileFileName.to_char());
#else
			output(TXT("load player profile"));
#endif
			for_every(node, mainDoc.get_root()->children_named(TXT("playerProfile")))
			{
				// game slots!
				List<RefCountObjectPtr<IO::XML::DocumentHandle>> gameSlotDocs;
				Array<IO::XML::Node const*> gameSlotNodes;
				if (_loadGameSlots)
				{
					IO::Dir dir;
					if (dir.list(IO::get_user_game_data_directory() + get_player_profile_directory()))
					{
						// profilename.gameslot.00000
						String requiredPrefix = get_player_profile_file_without_suffix() + get_game_slot_filename_suffix();
						requiredPrefix = requiredPrefix.to_lower();

						// gather all files
						Array<String> sortedGameSlotFiles;
						for_every(file, dir.get_files())
						{
							String lowerFile = file->to_lower();
							if (lowerFile.has_prefix(requiredPrefix) &&
								!is_temp_file(lowerFile))
							{
								sortedGameSlotFiles.push_back(*file);
							}
						}

						// sort them to load in order
						sort(sortedGameSlotFiles, String::compare_icase_sort);
						for_every(file, sortedGameSlotFiles)
						{
							gameSlotDocs.push_back(RefCountObjectPtr< IO::XML::DocumentHandle>(new IO::XML::DocumentHandle()));
							if (load_xml_using_temp_file(gameSlotDocs.get_last()->access(), IO::get_user_game_data_directory() + get_player_profile_directory() + *file))
							{
								output(TXT("  will read game slot from \"%S\""), file->to_char());
								for_every(node, gameSlotDocs.get_last()->access().get_root()->children_named(TXT("gameSlot")))
								{
									gameSlotNodes.push_back(node);
								}
							}
							else
							{
								gameSlotDocs.pop_back();
							}
						}
					}
				}

				output(TXT("  load player setup (%i slot(s))"), gameSlotNodes.get_size());
				if (playerSetup.load_from_xml(node, gameSlotNodes))
				{
					output(TXT("  loaded player setup (%i slot(s))"), playerSetup.get_game_slots().get_size());
					for_every(configNode, node->children_named(TXT("config")))
					{
						for_every(subConfigNode, configNode->all_children())
						{
							if (subConfigNode->get_name() == TXT("vrConfig"))
							{
								MainConfig::access_global().load_user_vr_from_xml(subConfigNode);
							}
						}
					}
					loaded = true;
				}
				else
				{
					output(TXT("  failed loading player setup (%i slot(s))"), playerSetup.get_game_slots().get_size());
				}
			}
		}
		else
		{
#ifdef AN_DEVELOPMENT_OR_PROFILER
			output(TXT("couldn't load player profile from \"%S\" (%S)"), playerProfileFileName.to_char(), IO::File::does_exist(playerProfileFileName)? TXT("other reason") : TXT("file does not exist"));
#else
			output(TXT("couldn't load player profile"));
#endif
		}
	}
	else
	{
		output(TXT("not loading player profile"));
	}
	playerProfileRequiresSaving = loaded;
	playerProfileSaveTimeStamp.reset();
	playerProfileExists = loaded;
	if (loaded)
	{
		playerSetup.access_preferences().apply();
		update_hub_scene_meshes();
		playerSetup.setup_defaults();
	}
	return loaded;
}

bool Game::load_player_profile_stats_and_preferences_into(PlayerSetup & _ps, Optional<String> const & _profile)
{
	bool loaded = false;
	_ps.reset(); // reset!
	String filename = get_whole_path_to_player_profile_file(_profile);
	if (IO::File::does_exist(filename))
	{
		IO::File file;
		if (file.open(filename))
		{
			file.set_type(IO::InterfaceType::Text);
			IO::XML::Document doc;
			if (doc.load_xml(&file))
			{
				for_every(node, doc.get_root()->children_named(TXT("playerProfile")))
				{
					_ps.load_player_stats_and_preferences_from_xml(node);
					loaded = true;
				}
			}
		}
	}
	return loaded;
}

void Game::add_async_save_player_profile(bool _saveActiveGameSlot)
{
	if (addedAsyncSavePlayerProfile)
	{
		return;
	}
	addedAsyncSavePlayerProfile = true;
	add_async_world_job_top_priority(TXT("save profile"),
	[this, _saveActiveGameSlot]()
	{
		addedAsyncSavePlayerProfile = false;
		save_player_profile(_saveActiveGameSlot);
	});
}

void Game::show_empty_hub_scene_and_save_player_profile(Optional<Loader::Hub*> _usingHubLoader)
{
	show_hub_scene_waiting_for_world_manager(new Loader::HubScenes::Empty(), _usingHubLoader,
	[this]()
	{
#ifdef WITH_SAVING_ICON
		if (PlayerPreferences::should_show_saving_icon() &&
			!useQuickGameProfile)
		{
			gameIcons.savingIcon.activate();
		}
#endif
		save_player_profile(); // save last before we choose a different one?
		no_need_to_save_player_profile();
		gameIcons.savingIcon.deactivate_if_at_least(0.5f);
	});
}

void Game::save_player_profile(bool _saveActiveGameSlot)
{
	output(TXT("save player profile?"));

	if (!allowSavingPlayerProfile || !playerProfileExists)
	{
		return;
	}

	// no saving while in tutorials!
	if (TutorialSystem::check_active())
	{
		return;
	}

	output(TXT("save player profile"));

#ifdef WITH_SAVING_ICON
	if (PlayerPreferences::should_show_saving_icon() &&
		metaState == GameMetaState::Pilgrimage &&
		!useQuickGameProfile)
	{
		gameIcons.savingIcon.activate();
	}
#endif

	if (auto* gs = playerSetup.access_active_game_slot())
	{
		gs->update_last_time_played();
	}

	save_player_profile_to_file(NP, false, _saveActiveGameSlot);

	gameIcons.savingIcon.deactivate_if_at_least(0.5f);
}

void Game::save_player_profile_to_file(Optional<String> const& _profile, bool _saveAllGameSlots, bool _saveActiveGameSlot)
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (Framework::Library::may_ignore_missing_references())
	{
		// never save if we may have missing references
		return;
	}
#endif

	if (useQuickGameProfile ||
		get_player_profile_file_without_suffix() == String(quickGamePlayerProfile))
	{
		// don't save quick game profile at all
		return;
	}

	savingPlayerProfile = true;

	output(TXT("saving player profile \"%S\""), get_player_profile_file().to_char());

	System::TimeStamp saveProfileTS;

	{
		IO::XML::Document doc;
		IO::XML::Node* playerProfileNode = doc.get_root()->add_node(TXT("playerProfile"));

		playerSetup.save_to_xml(playerProfileNode);

		IO::XML::Node* configNode = playerProfileNode->add_node(TXT("config"));
		{
			IO::XML::Node* subConfigNode = configNode->add_node(TXT("vrConfig"));
			MainConfig::global().save_user_vr_to_xml(subConfigNode);
		}

		// save to temp file and then replace actual save with temp file
		save_xml_to_file_using_temp_file(doc, get_whole_path_to_player_profile_file(_profile));
	}
	if (playerProfileLoadedWithGameSlots && (_saveAllGameSlots || _saveActiveGameSlot))
	{
		{	// remove non existent game slots
			IO::Dir dir;
			String path = IO::get_user_game_data_directory() + get_player_profile_directory();
			if (dir.list(path))
			{
				// profilename.gameslot.00000
				String requiredPrefix = get_player_profile_file_without_suffix() + get_game_slot_filename_suffix();
				requiredPrefix = requiredPrefix.to_lower();

				// check if slot for this id should exist
				for_every(file, dir.get_files())
				{
					String lowerFile = file->to_lower();
					if (lowerFile.has_prefix(requiredPrefix) &&
						!is_temp_file(lowerFile))
					{
						int id = ParserUtils::parse_int(lowerFile.get_right(5));

						bool exists = false;
						for_every_ref(gs, playerSetup.get_game_slots())
						{
							if (gs->id == id)
							{
								exists = true;
								break;
							}
						}
						if (!exists)
						{
							IO::File::delete_file(path + *file);
						}
					}
				}
			}
		}
		for_every_ref(gs, playerSetup.get_game_slots())
		{
			if (_saveAllGameSlots || playerSetup.get_active_game_slot() == gs)
			{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
				output(TXT("saving game slot"));
#endif

				IO::XML::Document doc;
				IO::XML::Node* gameSlotNode = doc.get_root()->add_node(TXT("gameSlot"));

				if (playerSetup.save_game_slot_to_xml(gameSlotNode, gs))
				{
					// save to temp file and then replace actual save with temp file
					String gameSlotPath = get_whole_path_to_game_slot_file(_profile, gs);
					save_xml_to_file_using_temp_file(doc, gameSlotPath);
#ifdef AN_ALLOW_EXTENSIVE_LOGS
					output(TXT("game slot saved to \"%S\""), gameSlotPath.to_char());
					output(TXT("%S"), gameSlotNode->to_string_slow().to_char());
#endif
				}
				else
				{
					error(TXT("failed saving game slot"));
				}
			}
		}
	}

	playerProfileRequiresSaving = true;
	playerProfileSaveTimeStamp.reset();

	output_colour_system();
	output(TXT("player profile \"%S\" saved (%.1fms)"), get_player_profile_file().to_char(), saveProfileTS.get_time_since() * 1000.0f);
	output_colour();

	savingPlayerProfile = false;
}

void Game::may_need_to_save_player_profile()
{
	playerProfileRequiresSaving = true;
}

void Game::no_need_to_save_player_profile()
{
	playerProfileRequiresSaving = false;
}

void Game::switch_to_no_player_profile()
{
	playerProfileExists = false;
	playerProfileRequiresSaving = false;
	playerProfileSaveTimeStamp.reset();

	playerSetup.reset();
	playerSetup.make_current();

	// clear it
	if (auto* c = GameConfig::get_as<GameConfig>())
	{
		c->set_player_profile_name(String::empty());
	}
}

void Game::pre_update_loader()
{
	base::pre_update_loader();

	if (metaState == GameMetaState::Tutorial)
	{
		TutorialSystem::get()->advance();
	}
	// if any of these is paused, they won't do anything
	// but in some cases we may have them running (custom loaders)
	if (musicPlayer)
	{
		// update this one too to handle fading out
		musicPlayer->update(::System::Core::get_delta_time());
	}
	if (nonGameMusicPlayer)
	{
		nonGameMusicPlayer->update(::System::Core::get_delta_time());
	}
	if (voiceoverSystem)
	{
		MEASURE_PERFORMANCE_COLOURED(voiceoverSystem, Colour::zxRed);
		voiceoverSystem->update(::System::Core::get_delta_time());
	}
	if (subtitleSystem)
	{
		MEASURE_PERFORMANCE_COLOURED(subtitleSystem, Colour::zxRed);
		subtitleSystem->update(::System::Core::get_delta_time());
		// may require updating overlay system to render subtitles
	}

#ifdef AN_ALLOW_BULLSHOTS
	if (Framework::BullshotSystem::is_active() ||
		Framework::BullshotSystem::does_want_to_draw())
	{
		Framework::BullshotSystem::advance(::System::Core::get_delta_time(), true);
	}
#endif
}

void Game::post_render_loader()
{
	base::post_render_loader();

	process_debug_controls(true);

	// loaders output to actual output, we need to render everything there
	::System::RenderTargetPtr keepRT;
	keepRT = mainRenderOutputRenderBuffer;
	mainRenderOutputRenderBuffer.clear();

	render_bullshot_logos_on_main_render_output();
	game_icon_render_ui_on_main_render_output();
	render_bullshot_frame_on_main_render_output();

	temp_render_ui_on_output(true);

	// let's go back to normal
	mainRenderOutputRenderBuffer = keepRT;

	store_recent_capture(false /* not game */);
}

void Game::show_hub_scene_waiting_for_world_manager(Loader::HubScene* _scene, Optional<Loader::Hub*> _usingHubLoader, std::function<void()> _asyncTaskToFinish)
{
	if (!_scene)
	{
		_scene = new Loader::HubScenes::Empty();
	}
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("show_hub_scene_waiting_for_world_manager - begin"));
#endif
	start_short_pause();
	if (_asyncTaskToFinish)
	{
		add_async_world_job(TXT("async task for hub scene"), _asyncTaskToFinish);
	}
	Loader::Hub* usingHubLoader = _usingHubLoader.get(mainHubLoader);
	make_sure_hub_loader_is_valid(usingHubLoader);
	recentHubLoader = usingHubLoader;
	usingHubLoader->remove_old_screens();
	usingHubLoader->set_scene(_scene);
	force_advance_world_manager(usingHubLoader);
	output_waiting_done();
	there_is_longer_frame_to_ignore();
	end_short_pause();
#ifdef AN_ALLOW_EXTENSIVE_LOGS
	output(TXT("show_hub_scene_waiting_for_world_manager - end"));
#endif
}

void Game::update_hub_scene_meshes()
{
	if (mainHubLoader)
	{
		mainHubLoader->update_scene_mesh();
	}
}

void Game::show_hub_scene(Loader::HubScene* _scene, Optional<Loader::Hub*> _usingHubLoader, std::function<bool()> _conditionToFinish, bool _allowWorldManagerAdvancement)
{
	if (!_conditionToFinish)
	{
		show_hub_scene_waiting_for_world_manager(_scene, _usingHubLoader);
		return;
	}
	start_short_pause();
	bool triggeredFinish = false;
	Loader::Hub* usingHubLoader = _usingHubLoader.get(mainHubLoader);
	make_sure_hub_loader_is_valid(usingHubLoader);
	recentHubLoader = usingHubLoader;
	usingHubLoader->remove_old_screens();
	usingHubLoader->set_scene(_scene);
	if (!usingHubLoader->is_active())
	{
		usingHubLoader->activate();
	}
	while (usingHubLoader->is_active())
	{
		if (_allowWorldManagerAdvancement)
		{
			force_advance_world_manager(true);
		}
		if (!triggeredFinish &&
			_conditionToFinish())
		{
			triggeredFinish = true;
			usingHubLoader->deactivate();
		}
		advance_and_display_loader(usingHubLoader);
	}
	output_waiting_done();
	there_is_longer_frame_to_ignore();
	end_short_pause();
}

void Game::on_game_sounds_pause()
{
	if (soundScene)
	{
		soundScene->pause();
	}
	if (musicPlayer)
	{
		musicPlayer->pause();
	}
	if (voiceoverSystem)
	{
		voiceoverSystem->pause();
	}
	if (environmentPlayer)
	{
		environmentPlayer->pause();
	}
}

void Game::on_game_sounds_unpause()
{
	if (auto* soundScene = access_sound_scene())
	{
		soundScene->resume();
	}
	if (auto* musicPlayer = access_music_player())
	{
		musicPlayer->resume();
	}
	if (auto* voiceoverSystem = access_voiceover_system())
	{
		voiceoverSystem->resume();
	}
	if (auto* environmentPlayer = access_environment_player())
	{
		environmentPlayer->resume();
	}
}

void Game::manage_game_sounds_pause(bool _gameSoundsShouldBePaused)
{
	if (gameSoundsShouldBePaused != _gameSoundsShouldBePaused)
	{
		gameSoundsShouldBePaused = _gameSoundsShouldBePaused;
		if (gameSoundsShouldBePaused)
		{
			start_game_sounds_pause();
		}
		else
		{
			end_game_sounds_pause();
		}
	}
}

void Game::on_non_game_sounds_pause()
{
	if (nonGameMusicPlayer)
	{
		nonGameMusicPlayer->pause();
	}
}

void Game::on_non_game_sounds_unpause()
{
	if (nonGameMusicPlayer)
	{
		nonGameMusicPlayer->resume();
	}
}

void Game::manage_non_game_sounds_pause(bool _nonGameSoundsShouldBePaused)
{
	if (nonGameSoundsShouldBePaused != _nonGameSoundsShouldBePaused)
	{
		nonGameSoundsShouldBePaused = _nonGameSoundsShouldBePaused;
		if (nonGameSoundsShouldBePaused)
		{
			start_non_game_sounds_pause();
		}
		else
		{
			end_non_game_sounds_pause();
		}
	}
}

void Game::on_short_pause()
{
	System::Core::block_on_advance(NAME(advanceRunTimer));
	System::Core::block_on_advance(NAME(advanceActiveRunTimer));
	base::on_short_pause();
}

void Game::on_short_unpause()
{
	base::on_short_unpause();
	System::Core::allow_on_advance(NAME(advanceRunTimer));
	System::Core::allow_on_advance(NAME(advanceActiveRunTimer));
}

bool Game::is_option_set(Name const & _option) const
{
	return playerSetup.is_option_set(_option) ||
		base::is_option_set(_option);
}

bool Game::does_debug_control_player() const
{
	return debugControlsMode == 0 || debugCameraMode == 0;
}

void Game::prepare_render_buffer_resolutions(REF_ VectorInt2& _mainResolution, REF_ VectorInt2& _secondaryViewResolution)
{
	VectorInt2 mainResolutionProvided = _mainResolution;
	VectorInt2 useEyeResolution = mainResolutionProvided;
	if (does_use_vr())
	{
		if (auto* vr = VR::IVR::get())
		{
			useEyeResolution = vr->get_render_full_size_for_aspect_ratio_coef(VR::Eye::Right);
		}
	}

	_secondaryViewResolution = VectorInt2::zero;

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	TeaForGodEmperor::GameConfig::SecondaryViewInfo svi;
	if (TeaForGodEmperor::GameConfig const* gc = TeaForGodEmperor::GameConfig::get_as<TeaForGodEmperor::GameConfig>())
	{
		svi = gc->get_secondary_view_info();
	}

	svi.mainSize = clamp(svi.mainSize, 0.0f, 1.0f);
	svi.pipSize = clamp(svi.pipSize, 0.0f, 1.0f);

	if (svi.show)
	{
		if (svi.mainSizeAuto)
		{
			if (mainResolutionProvided.y > mainResolutionProvided.x)
			{
				_mainResolution.x = mainResolutionProvided.x;
				_mainResolution.y = useEyeResolution.y * mainResolutionProvided.x / useEyeResolution.x;
				_mainResolution.y = min(_mainResolution.y, mainResolutionProvided.y * 3 / 4);
				_secondaryViewResolution.x = mainResolutionProvided.x;
				_secondaryViewResolution.y = mainResolutionProvided.y - _mainResolution.y;
			}
			else
			{
				_mainResolution.y = mainResolutionProvided.y;
				_mainResolution.x = useEyeResolution.x * mainResolutionProvided.y / useEyeResolution.y;
				_mainResolution.x = min(_mainResolution.x, mainResolutionProvided.x * 2 / 3);
				_mainResolution.y = mainResolutionProvided.y;
				_secondaryViewResolution.x = mainResolutionProvided.x - _mainResolution.x;
				_secondaryViewResolution.y = min(mainResolutionProvided.y, _secondaryViewResolution.x); // keep it square
			}
		}
		else
		{
			if (mainResolutionProvided.y > mainResolutionProvided.x)
			{
				_mainResolution.x = mainResolutionProvided.x;
				_mainResolution.y = TypeConversions::Normal::f_i_closest((float)mainResolutionProvided.y * svi.mainSize);
				_secondaryViewResolution.x = mainResolutionProvided.x;
				_secondaryViewResolution.y = mainResolutionProvided.y - _mainResolution.y;
			}
			else
			{
				_mainResolution.y = mainResolutionProvided.y;
				_mainResolution.x = TypeConversions::Normal::f_i_closest((float)mainResolutionProvided.x * svi.mainSize);
				_secondaryViewResolution.y = mainResolutionProvided.y;
				_secondaryViewResolution.x = mainResolutionProvided.x - _mainResolution.x;
				_secondaryViewResolution.y = min(mainResolutionProvided.y, _secondaryViewResolution.x); // keep it square
			}
		}
		if (svi.pip)
		{
			VectorInt2 pipResolution = VectorInt2::zero;
			VectorInt2 maxPIPResolution = VectorInt2::zero;
			if (!svi.pipSwap)
			{
				if (mainResolutionProvided.x > mainResolutionProvided.y)
				{
					maxPIPResolution.y = mainResolutionProvided.y;
					maxPIPResolution.x = mainResolutionProvided.y * 6 / 5;
				}
				else
				{
					maxPIPResolution.x = mainResolutionProvided.x;
					maxPIPResolution.y = mainResolutionProvided.x * 3 / 5;
				}
			}
			else
			{
				maxPIPResolution = useEyeResolution;
			}
			pipResolution = lerp(svi.pipSize, pipResolution.to_vector2(), maxPIPResolution.to_vector2()).to_vector_int2();
			secondaryViewPIPResolution = pipResolution;
			_secondaryViewResolution = pipResolution;
			if (svi.pipSwap)
			{
				swap(_secondaryViewResolution, _mainResolution);
			}
		}
	}
	else
	{
		_mainResolution = mainResolutionProvided;
		_secondaryViewResolution = VectorInt2::zero;
	}

	if (_secondaryViewResolution.x <= 0 || _secondaryViewResolution.y <= 0)
	{
		_secondaryViewResolution = VectorInt2::zero;
	}

	secondaryViewMainResolution = _mainResolution;
	secondaryViewViewResolution = _secondaryViewResolution;
#endif
}

void Game::render_main_render_output_to_output()
{
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	useExternalControls = false;
#endif

	if (does_use_vr() && !should_render_to_main_for_vr())
	{
		// don't do anything
		return;
	}

	System::RenderTarget* eyeRt;
	if (does_use_vr())
	{
		eyeRt = VR::IVR::get()->get_output_render_target(VR::Eye::OnMain);
	}
	else
	{
		eyeRt = get_main_render_output_render_buffer();
	}

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	if (onlyExternalRenderScene)
	{
		eyeRt = nullptr;
	}
#endif

	if (eyeRt)
	{
		eyeRt->resolve_forced_unbind();
	}

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	System::RenderTarget* evRt = get_secondary_view_output_render_buffer();

	// this is from svi but already filtered via code in game.cpp
	if (!useExternalRenderScene && !onlyExternalRenderScene)
	{
		evRt = nullptr;
	}

	if (evRt)
	{
		evRt->resolve_forced_unbind();
	}
#endif

	if (!eyeRt
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
		&& !evRt
#endif
		)
	{
		// either already rendered or nothing to render
		return;
	}

	auto* v3d = ::System::Video3D::get();

	System::RenderTarget::bind_none();

	Colour backgroundColour = Colour::black;

	v3d->setup_for_2d_display();
	v3d->set_default_viewport();
	v3d->set_near_far_plane(0.02f, 100.0f);
	v3d->clear_colour(backgroundColour);

	// restore projection matrix
	v3d->set_2d_projection_matrix_ortho();
	v3d->access_model_view_matrix_stack().clear();

	VectorInt2 targetSize = MainConfig::global().get_video().resolution;

	struct RenderFrame
	{
		::System::Video3D* v3d;
		Colour backgroundColour;
		float borderWidth;

		RenderFrame(::System::Video3D* _v3d, Colour const& _backgroundColour, float _borderWidth)
		: v3d(_v3d)
		, backgroundColour(_backgroundColour)
		, borderWidth(_borderWidth)
		{}

		void at(Vector2 const& lb, Vector2 const& size)
		{
			Vector2 tr = lb + size;
			::System::Video3DPrimitives::fill_rect_2d(backgroundColour, Range2(Range(lb.x, tr.x), Range(tr.y - borderWidth, tr.y)));
			::System::Video3DPrimitives::fill_rect_2d(backgroundColour, Range2(Range(lb.x, tr.x), Range(lb.y, lb.y + borderWidth)));
			::System::Video3DPrimitives::fill_rect_2d(backgroundColour, Range2(Range(lb.x, lb.x + borderWidth), Range(lb.y, tr.y)));
			::System::Video3DPrimitives::fill_rect_2d(backgroundColour, Range2(Range(tr.x - borderWidth, tr.x), Range(lb.y, tr.y)));
		}

		void divider(Vector2 const& _lb, Vector2 const& _size)
		{
			Vector2 lb = _lb;
			Vector2 size = _size;
			if (size.x < borderWidth)
			{
				size.x = borderWidth;
				lb.x -= borderWidth * 0.05f;
			}
			if (size.y < borderWidth)
			{
				size.y = borderWidth;
				lb.y -= borderWidth * 0.05f;
			}

			Vector2 tr = lb + size;

			::System::Video3DPrimitives::fill_rect_2d(backgroundColour, Range2(Range(lb.x, tr.x), Range(lb.y, tr.y)));
		}

	} renderFrame(v3d, backgroundColour, ((float)min(targetSize.x, targetSize.y)) * 0.01f);

	System::RenderTarget* singleRt = eyeRt;
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	if (evRt && eyeRt)
	{
		singleRt = nullptr;
	}
	if (evRt && ! eyeRt)
	{
		singleRt = evRt;
	}
#endif

	if (singleRt)
	{
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
		useExternalControls = singleRt == evRt;
#endif
		TeaForGodEmperor::GameConfig::SecondaryViewInfo svi;
		if (TeaForGodEmperor::GameConfig const* gc = TeaForGodEmperor::GameConfig::get_as<TeaForGodEmperor::GameConfig>())
		{
			svi = gc->get_secondary_view_info();
		}
		singleRt->render_fit(0, ::System::Video3D::get(), Vector2::zero, targetSize.to_vector2(), svi.mainAt, svi.mainZoom, svi.mainZoom);
	}
	else
	{
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
		TeaForGodEmperor::GameConfig::SecondaryViewInfo svi;
		if (TeaForGodEmperor::GameConfig const* gc = TeaForGodEmperor::GameConfig::get_as<TeaForGodEmperor::GameConfig>())
		{
			svi = gc->get_secondary_view_info();
		}

		// ev should never zoom on x axis

		if (svi.pip)
		{
			bool const mainIsEV = svi.pipSwap;
			bool const secondaryIsEV = !svi.pipSwap;
			::System::RenderTarget* mainRT = svi.pipSwap ? evRt : eyeRt;
			::System::RenderTarget* pipRT = svi.pipSwap ? eyeRt : evRt;

			VectorInt2 leftBottom = VectorInt2::zero;
			// use mainSize as it would be for calculating full screen
			{
				Vector2 lb;
				Vector2 rs = targetSize.to_vector2();
				if (targetSize.y > targetSize.x)
				{
					rs.y *= svi.mainSize;
				}
				else
				{
					rs.x *= svi.mainSize;
				}
				lb = leftBottom.to_vector2() + (targetSize.to_vector2() - rs) * svi.mainAt;
				if (mainRT)
				{
					mainRT->render_fit(0, v3d, lb, rs, svi.mainAt, !mainIsEV && svi.mainZoom, svi.mainZoom);
				}
			}

			Vector2 pipSize = secondaryViewPIPResolution.to_vector2();

			Vector2 pipAt = (targetSize.to_vector2() - pipSize) * svi.secondaryAt;

			if (pipRT)
			{
				pipRT->render_fit(0, v3d, pipAt, pipSize, NP, !secondaryIsEV && svi.secondaryZoom, svi.secondaryZoom);
			}
			renderFrame.at(pipAt, pipSize);

			useExternalControls = mainRT == evRt;
		}
		else
		{
			VectorInt2 coversSize = VectorInt2::zero;
			VectorInt2 eyeRtSize = secondaryViewMainResolution;
			VectorInt2 evRtSize = secondaryViewViewResolution;
			if (targetSize.y > targetSize.x)
			{
				if (eyeRt)
				{
					coversSize.x = max(coversSize.x, eyeRtSize.x);
					coversSize.y += eyeRtSize.y;
				}
				if (evRt)
				{
					coversSize.x = max(coversSize.x, evRtSize.x);
					coversSize.y += evRtSize.y;
				}
			}
			else
			{
				if (eyeRt)
				{
					coversSize.x += eyeRtSize.x;
					coversSize.y = max(coversSize.y, eyeRtSize.y);
				}
				if (evRt)
				{
					coversSize.x += evRtSize.x;
					coversSize.y = max(coversSize.y, evRtSize.y);
				}
			}

			VectorInt2 leftBottom = (targetSize - coversSize) / 2;
			VectorInt2 dividerAt = leftBottom;

			if (targetSize.y > targetSize.x)
			{
				bool showExternalViewAtBottom = svi.mainAt.y >= 0.5f;
				for_count(int, i, 2)
				{
					int iOrdered = showExternalViewAtBottom ? i : 1 - i; // from bottom
					if (evRt && iOrdered == 0)
					{
						Vector2 lb = leftBottom.to_vector2();
						Vector2 size = VectorInt2(coversSize.x, evRtSize.y).to_vector2();
						evRt->render_fit(0, v3d, lb, size, svi.secondaryAt, false, svi.secondaryZoom);
						leftBottom.y += evRtSize.y;
					}
					if (eyeRt && iOrdered == 1)
					{
						eyeRt->render_fit(0, v3d, leftBottom.to_vector2(), VectorInt2(coversSize.x, eyeRtSize.y).to_vector2(), svi.mainAt, svi.mainZoom, svi.mainZoom);
						leftBottom.y += eyeRtSize.y;
					}
					if (i == 0)
					{
						dividerAt = leftBottom;
					}
				}
				renderFrame.divider(dividerAt.to_vector2(), Vector2((float)coversSize.x, 0.0f));
			}
			else
			{
				bool showExternalViewAtLeft = svi.mainAt.x >= 0.5f;
				for_count(int, i, 2)
				{
					int iOrdered = showExternalViewAtLeft ? i : 1 - i; // from left
					if (evRt && iOrdered == 0)
					{
						Vector2 lb = leftBottom.to_vector2();
						Vector2 size = VectorInt2(evRtSize.x, coversSize.y).to_vector2();
						evRt->render_fit(0, v3d, lb, size, svi.secondaryAt, false, svi.secondaryZoom);
						leftBottom.x += evRtSize.x;
					}
					if (eyeRt && iOrdered == 1)
					{
						eyeRt->render_fit(0, v3d, leftBottom.to_vector2(), VectorInt2(eyeRtSize.x, coversSize.y).to_vector2(), svi.mainAt, svi.mainZoom, svi.mainZoom);
						leftBottom.x += eyeRtSize.x;
					}
					if (i == 0)
					{
						dividerAt = leftBottom;
					}
				}
				renderFrame.divider(dividerAt.to_vector2(), Vector2(0.0f, (float)coversSize.y));
			}
		}
#endif
	}

#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
#ifndef BUILD_NON_RELEASE
	useExternalControls = false;
#endif
#endif
	System::RenderTarget::unbind_to_none();
}

void Game::render_vr_output_to_output() // this is not used by game itself as we have render_main_render_output_to_output
{
	if (!does_use_vr() || !should_render_to_main_for_vr())
	{
		return;
	}

	{	// update pilgrim info
		float deltaTime = pilgrimInfoToRenderUpdateTS.get_time_since();
		pilgrimInfoToRenderUpdateTS.reset();
		pilgrimInfoToRender.advance(player.get_actor(), deltaTime);
		if (deltaTime > 1.0f)
		{
			pilgrimInfoToRender.clear_highlight();
		}
	}

	auto* v3d = ::System::Video3D::get();

	// this below should be moved to render_main_render_output_to_output
	/*
	bool displayWithInfo = false;
	if (auto * c = GameConfig::get_as<GameConfig>())
	{
		displayWithInfo = c->does_show_vr_on_screen_with_info();
	}
#ifdef AN_ALLOW_BULLSHOTS
	if (Framework::BullshotSystem::does_want_to_draw())
	{
		// no info when bullshot system is active (unless you want to fix the visible borders
		displayWithInfo = false;
	}
#endif
	if (! pilgrimInfoToRender.isVisible)
	{
		displayWithInfo = false; // full screen, as is
	}
	if (displayWithInfo)
	{
		{	// clear
			get_main_render_buffer()->bind();
			v3d->clear_colour(Colour::black);
			::System::RenderTarget::bind_none();
		}

		{	// render vr eye and pilgrim info
			Vector2 mainRenderBufferSize = get_main_render_buffer()->get_size().to_vector2();
			float border = pilgrimInfoToRender.get_best_border_width(mainRenderBufferSize);
			int borderAsInt = TypeConversions::Normal::f_i_closest(border);
			VectorInt2 mainRenderBufferSizeAsInt = mainRenderBufferSize.to_vector_int2();
			VR::IVR::get()->render_one_output_to(v3d, VR::Eye::OnMain, get_main_render_buffer(), RangeInt2(RangeInt(borderAsInt, mainRenderBufferSizeAsInt.x - borderAsInt), RangeInt(0, mainRenderBufferSizeAsInt.y)));
			if (fading.should_be_applied())
			{
				apply_fading_to(get_main_render_buffer());
			}
			if (player.get_actor() && is_world_in_use() &&
				(metaState == GameMetaState::Pilgrimage || metaState == GameMetaState::Tutorial))
			{
				pilgrimInfoToRender.render(get_main_render_buffer(), border);
			}
		}

		{	// render to main
			get_main_render_buffer()->resolve();

			v3d->unbind_frame_buffer();

			v3d->setup_for_2d_display();

			v3d->set_default_viewport();
			v3d->set_near_far_plane(0.02f, 100.0f);

			v3d->set_2d_projection_matrix_ortho();
			v3d->access_model_view_matrix_stack().clear();

			get_main_render_buffer()->render(0, v3d, Vector2::zero, get_main_render_buffer()->get_size().to_vector2());
		}
	}
	else
	*/
	{
		VR::IVR::get()->render_one_output_to_main(v3d, VR::Eye::OnMain);
	}
}

Loader::HubScene* Game::create_loading_hub_scene(String const& _message, Optional<float> const& _delay, Optional<Name> const& _idToReuse, Optional<LoadingHubScene::Type> const& _loadingHubScene, Optional<bool> const& _allowLoadingTips)
{
	Name idToReuse = _idToReuse.get(Name::invalid());
	Loader::HubScene* scene = nullptr;
#ifdef AN_ALLOW_BULLSHOTS
	if (Framework::BullshotSystem::is_active() &&
		Framework::BullshotSystem::is_setting_active(NAME(bsNoLoadingScreens)))
	{
		scene = new Loader::HubScenes::Text(String::empty(), _delay, idToReuse);
	}
	else
#endif
	{
		bool showTips = true;
		if (auto* psp = PlayerSetup::get_current_if_exists())
		{
			showTips = psp->get_preferences().showTipsOnLoading;
		}
		showTips = _allowLoadingTips.get(showTips);
		if (showTips)
		{
			TagCondition loadingTipsCondition;
			loadingTipsCondition.load_from_string(String(TXT("loadingTips")));

			scene = new Loader::HubScenes::LoadingMessageSetBrowser(loadingTipsCondition, LOC_STR(NAME(lsLoadingTips)), _delay, NP, true, _loadingHubScene.get(LoadingHubScene::Undefined) != LoadingHubScene::InitialLoading);
		}
		else
		{
			Loader::HubScenes::Text* textScene = new Loader::HubScenes::Text(_message, _delay, idToReuse);
			{
				TagCondition tc;
				tc.load_from_string(String(TXT("loadingScreen")));
				Array<Framework::TexturePart*> tps;
				for_every_ptr(tp, Library::get_current()->get_texture_parts().get_tagged(tc))
				{
					tps.push_back(tp);
				}
				if (!tps.is_empty())
				{
					Vector2 ppa(28.0f, 28.0f);
					textScene->with_ppa(ppa);
					textScene->with_image(tps[Random::get_int(tps.get_size())], Vector2(1.4f, 1.75f), Vector2(2.0f, 2.0f), true);
					textScene->with_margin(VectorInt2(1, 1));
					textScene->with_text_align(Vector2(1.0f, 0.0f));
					textScene->with_extra_background(Colour::black);
					textScene->with_text_colour(Colour::white);
				}
				else
				{
					textScene->with_vertical_offset(Rotator3(-5.0f, 0.0f, 0.0f));
				}
			}
			scene = textScene;
		}
	}
	return scene;
}

void Game::change_show_debug_panel(Framework::DebugPanel* _change, bool _vr)
{
	base::change_show_debug_panel(_change, _vr);

#ifdef AN_PERFORMANCE_MEASURE
	if (debugPanel)
	{
		debugPanel->set_option(NAME(dboPerformanceMeasureShow), showPerformance);
		debugPanel->set_option(NAME(dboPerformanceMeasureShowNoRender), showPerformance && noRenderUntilShowPerformance);
		debugPanel->set_option(NAME(dboPerformanceMeasureShowNoGameLoop), showPerformance && noGameLoopUntilShowPerformance);
		debugPanel->set_option(NAME(dboStopOnAnyHitch), allowToStopToShowPerformanceOnAnyHitch);
		debugPanel->set_option(NAME(dboStopOnHighHitch), allowToStopToShowPerformanceOnHighHitch);
	}
#endif
}

bool Game::should_skip_delayed_objects_with_negative_priority() const
{
	if (PilgrimageInstanceOpenWorld::get())
	{
		// for time being
#ifdef AN_DEVELOPMENT
#ifndef AN_OVERRIDE_DEV_SKIPPING_DELAYED_OBJECT_CREATIONS
		return true;
#endif
#endif
	}
	return base::should_skip_delayed_objects_with_negative_priority();
}

bool Game::kill_by_gravity(Framework::IModulesOwner* _imo)
{
	if (_imo)
	{
		if (auto* h = _imo->get_custom<CustomModules::Health>())
		{
			_imo->get_presence()->set_velocity_linear(Vector3::zero);
			if (h->is_alive())
			{
				h->perform_death(); // this doesn't give the player who pushed it anything, but at the same time, how easy it is to kill robots this way
			}
			return true;
		}
	}
	return false;
}

#ifdef AN_ALLOW_BULLSHOTS
void Game::hard_copy_bullshot(bool _all)
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	add_immediate_sync_world_job(TXT("bullshot hard copy"),
		[_all, this]()
		{
			if (!_all)
			{
				if (auto* imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
				{
					Framework::BullshotSystem::hard_copy(imo);
				}
			}
			else if (world)
			{
				for_every_ptr(obj, world->get_objects())
				{
					if (obj->get_presence() &&
						obj->get_presence()->get_in_room() &&
						AVOID_CALLING_ACK_ obj->get_presence()->get_in_room()->was_recently_seen_by_player())
					{
						Framework::BullshotSystem::hard_copy(obj);
					}
				}
			}
		});
#endif
}

void Game::post_reload_bullshots()
{
	base::post_reload_bullshots();

#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (auto* lib = Library::get_current_as<Library>())
	{
		for_every_ref(gs, lib->get_game_scripts().get_all_stored())
		{
			lib->prepare_for_game_dev(gs);
		}
	}
#endif
}

#endif

void Game::set_forced_camera(Transform const& _p, Framework::Room* _r, Optional<float> const& _fov, Framework::IModulesOwner* _attachedTo)
{
	if (!useForcedCamera)
	{
		debugCameraMode = 0;
	}
	useForcedCamera = true;
	forcedCameraPlacement = _p;
	if (_r) forcedCameraInRoom = _r;
	forcedCameraFOV = _fov.get(forcedCameraFOV);
	forcedCameraAttachedTo = _attachedTo;
}

void Game::set_forced_sound_camera(Transform const& _p, Framework::Room* _r)
{
	if (!useForcedSoundCamera)
	{
		debugSoundCamera = 0;
	}
	useForcedSoundCamera = true;
	forcedSoundCameraPlacement = _p;
	if (_r) forcedSoundCameraInRoom = _r;
}

void Game::on_changed_door_type(Framework::Door* _door)
{
	base::on_changed_door_type(_door);
	if (auto* d = _door->get_linked_door_a())
	{
		Utils::set_emissives_from_room_to(d);
	}
	if (auto* d = _door->get_linked_door_b())
	{
		Utils::set_emissives_from_room_to(d);
	}
}

void Game::on_newly_created_object(Framework::IModulesOwner* _object)
{
	base::on_newly_created_object(_object);
	Utils::set_emissives_from_room_to(_object);
}

void Game::on_newly_created_door_in_room(Framework::DoorInRoom* _dir)
{
	scoped_call_stack_info(TXT("Game::on_newly_created_door_in_room"));
	base::on_newly_created_door_in_room(_dir);
	Utils::set_emissives_from_room_to(_dir);
}

void Game::on_generated_room(Framework::Room* _room)
{
	an_assert(Collision::DefinedFlags::has(NAME(worldSolidFlags)));
	an_assert(Collision::DefinedFlags::has(NAME(pilgrimOverlayInfoTraceBlocker)));
	_room->set_collision_flags(Collision::DefinedFlags::get_existing_or_basic(NAME(worldSolidFlags)) | Collision::DefinedFlags::get_existing_or_basic(NAME(pilgrimOverlayInfoTraceBlocker)));

	base::on_generated_room(_room);
	if (! does_want_to_cancel_creation())
	{
		EnvironmentPlayer::load_on_demand_if_required_for(_room);
	}
}

#ifdef WITH_DRAWING_BOARD
void Game::drop_drawing_board_pixels()
{
	Concurrency::ScopedSpinLock lock(drawingBoardLock);

	drawingBoardUpsideDownPixels.clear();
	drawingBoardUpsideDownPixelsSize = VectorInt2::zero;
}

void Game::store_drawing_board_pixels()
{
	assert_rendering_thread();

	Concurrency::ScopedSpinLock lock(drawingBoardLock);

	if (!drawingBoardRT.get())
	{
		return;
	}

	bool const readRGBA = true;
	int bytes = readRGBA ? 4 : 3;
	VectorInt2 size = drawingBoardRT->get_size(true);
	size.x = size.x - mod(size.x, 4); // align, skip the extra pixels
	drawingBoardUpsideDownPixels.set_size(bytes * size.x * size.y);

	System::Video3D* v3d = System::Video3D::get();

	{
		drawingBoardRT->resolve_forced_unbind();
		v3d->unbind_frame_buffer_bare();
		DIRECT_GL_CALL_ glBindFramebuffer(GL_READ_FRAMEBUFFER, drawingBoardRT->get_frame_buffer_object_id_to_read()); AN_GL_CHECK_FOR_ERROR
		//
		DIRECT_GL_CALL_ glReadPixels(0, 0, size.x, size.y, readRGBA ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, drawingBoardUpsideDownPixels.get_data()); AN_GL_CHECK_FOR_ERROR
		drawingBoardUpsideDownPixelsSize = size;
		//
		DIRECT_GL_CALL_ glBindFramebuffer(GL_READ_FRAMEBUFFER, 0); AN_GL_CHECK_FOR_ERROR
		v3d->invalidate_bound_frame_buffer_info();
		v3d->unbind_frame_buffer(); // to get back from bare
	}
}

bool Game::read_drawing_board_pixel_data(OUT_ VectorInt2& _resolution, OUT_ Array<byte>& _upsideDownPixels, OUT_ int& _bytesPerPixel, OUT_ bool& _convertSRGB)
{
	Concurrency::ScopedSpinLock lock(drawingBoardLock);

	if (!drawingBoardRT.get() ||
		drawingBoardUpsideDownPixels.is_empty() ||
		drawingBoardUpsideDownPixelsSize.is_zero())
	{
		return false;
	}

	_upsideDownPixels = drawingBoardUpsideDownPixels;
	_resolution = drawingBoardUpsideDownPixelsSize;
	_bytesPerPixel = 4 /* RGBA */;
	_convertSRGB = drawingBoardRT->get_setup().should_use_srgb_conversion();
	if (auto* v3d = ::System::Video3D::get())
	{
		_convertSRGB |= v3d->get_system_implicit_srgb();
	}

	return true;
}
#endif

void Game::use_sliding_locomotion_internal(bool _use)
{
	base::use_sliding_locomotion_internal(_use);
	player.update_for_sliding_locomotion();
}

bool Game::reset_immobile_origin_in_vr_space_for_sliding_locomotion()
{
	if (base::reset_immobile_origin_in_vr_space_for_sliding_locomotion())
	{
		if (auto* pa = player.get_actor())
		{
			pa->get_presence()->zero_vr_anchor();
		}
		return true;
	}
	return false;
}


bool Game::is_ok_to_use_for_region_generation(Framework::LibraryStored const* _libraryStored) const
{
	if (auto* minPlayAreaTileCount = _libraryStored->get_custom_parameters().get_existing<VectorInt2>(NAME(minPlayAreaTileCount)))
	{
		auto tileCount = RoomGenerationInfo::get().get_tile_count();
		if (tileCount.x < minPlayAreaTileCount->x ||
			tileCount.y < minPlayAreaTileCount->y)
		{
			// ignore as it requires more space
			return false;
		}
	}
	if (auto* minPlayAreaSize = _libraryStored->get_custom_parameters().get_existing<Vector2>(NAME(minPlayAreaSize)))
	{
		auto playAreaSize = RoomGenerationInfo::get().get_play_area_rect_size();
		if (playAreaSize.x < minPlayAreaSize->x ||
			playAreaSize.y < minPlayAreaSize->y)
		{
			// ignore as it requires more space
			return false;
		}
	}
	return base::is_ok_to_use_for_region_generation(_libraryStored);
}

float Game::get_adjusted_time_multiplier() const
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (debugCameraMode)
	{
		return base::get_adjusted_time_multiplier();
	}
#endif
#ifdef WITH_SLOW_DOWN_ON_LOW_HEALTH
	if (auto* pa = player.get_actor())
	{
		if (auto* h = pa->get_custom<CustomModules::Health>())
		{
			float pt = h->get_health().div_to_float(h->get_max_health());
			Range range(0.1f, 0.5f);
			pt = range.get_pt_from_value(pt);
			static float timeMultiplier = 1.0f;
			float const slowDownOnLowHealth = 0.9f;
			float requestedTimeMultiplier = remap_value(pt, 0.0f, 1.0f, slowDownOnLowHealth, 1.0f, true);
			if (auto* gd = GameDirector::get())
			{
				if (gd->are_time_adjustments_blocked())
				{
					requestedTimeMultiplier = 1.0f;
				}
			}
			{
				static ::System::TimeStamp ts;
				float deltaTime = ts.get_time_since();
				ts.reset();
				timeMultiplier = deltaTime > 1.0f ? requestedTimeMultiplier : blend_to_using_time(timeMultiplier, requestedTimeMultiplier, 0.2f, deltaTime);
			}

			return timeMultiplier;
		}
	}
#endif
	return base::get_adjusted_time_multiplier();
}

void Game::reset_camera_rumble()
{
	cameraRumble = CameraRumble();
}

void Game::set_camera_rumble(CameraRumbleParams const& _params)
{
	cameraRumble.set(_params);
}

void Game::advance_camera_rumble(float _deltaTime)
{
	cameraRumble.advance(_deltaTime);
}

tchar const* Game::get_additional_hitch_info() const
{
	if (auto* pa = player.get_actor())
	{
		if (auto* p = pa->get_presence())
		{
			if (auto* r = p->get_in_room())
			{
				return r->get_name().to_char();
			}
		}
	}
	return base::get_additional_hitch_info();
}

VR::Zone Game::get_vr_zone(Framework::Room* _room) const
{
	if (_room)
	{
		float tileSize = get_vr_tile_size();

		Optional<Vector2> maxPlayAreaSize;
		if (auto* maxPlayAreaTileSizePtr = _room->get_variable<Vector2>(NAME(maxPlayAreaTileSize)))
		{
			Vector2 maxPlayAreaTileSize = *maxPlayAreaTileSizePtr;
			Vector2 maxPlayAreaSizeFromTiles = maxPlayAreaTileSize * tileSize;
			Vector2 currMaxPlayAreaSize = maxPlayAreaSize.get(maxPlayAreaSizeFromTiles);

			currMaxPlayAreaSize.x = min(currMaxPlayAreaSize.x, maxPlayAreaSizeFromTiles.x);
			currMaxPlayAreaSize.y = min(currMaxPlayAreaSize.y, maxPlayAreaSizeFromTiles.y);

			maxPlayAreaSize = currMaxPlayAreaSize;
		}
		if (auto* maxPlayAreaSizePtr = _room->get_variable<Vector2>(NAME(maxPlayAreaSize)))
		{
			Vector2 currMaxPlayAreaSize = maxPlayAreaSize.get(*maxPlayAreaSizePtr);

			currMaxPlayAreaSize.x = min(currMaxPlayAreaSize.x, (*maxPlayAreaSizePtr).x);
			currMaxPlayAreaSize.y = min(currMaxPlayAreaSize.y, (*maxPlayAreaSizePtr).y);

			maxPlayAreaSize = currMaxPlayAreaSize;
		}
		if (maxPlayAreaSize.is_set())
		{
			VR::Zone zone = base::get_vr_zone(_room);

			Range2 vrRange = zone.to_range2();
			Vector2 centre = vrRange.centre();
			Range2 newVRRange = Range2::empty;

			Vector2 halfSize = maxPlayAreaSize.get() * 0.5f;
			newVRRange.include(centre + halfSize);
			newVRRange.include(centre - halfSize);

			FOR_EVERY_ALL_DOOR_IN_ROOM(dir, _room)
			{
				if (!dir->is_vr_space_placement_set() ||
					!dir->can_move_through() ||
					!dir->is_relevant_for_movement() ||
					!dir->is_relevant_for_vr() ||
					dir->may_ignore_vr_placement())
				{
					continue;
				}
				if (auto* d = dir->get_door())
				{
					float doorWidth = d->calculate_vr_width();
					Transform vrPlacement = dir->get_vr_space_placement();
					Vector2 at = vrPlacement.get_translation().to_vector2();
					Vector2 fwdTile = vrPlacement.get_axis(Axis::Forward).to_vector2() * tileSize;
					Vector2 rightHalf = vrPlacement.get_axis(Axis::Right).to_vector2() * (doorWidth * 0.5f);
					newVRRange.include(at + fwdTile + rightHalf);
					newVRRange.include(at + fwdTile - rightHalf);
					newVRRange.include(at - fwdTile + rightHalf);
					newVRRange.include(at - fwdTile - rightHalf);
				}
			}

			newVRRange.intersect_with(vrRange);

			zone.clear();

			zone.be_range(newVRRange);

			return zone;
		}
	}

	return base::get_vr_zone(_room);
}

bool Game::send_quick_note_in_background(String const& _text, String const& _additionalText)
{
	return ReportBug::send_quick_note_in_background(_text, _additionalText);
}

bool Game::send_full_log_info_in_background(String const& _text, String const& _additionalText)
{
	return ReportBug::send_full_log_info_in_background(_text, _additionalText);
}

void Game::use_test_scenario_for_pilgimage(Framework::LibraryBasedParameters* _testScenario)
{
	if (testScenario)
	{
		// if was used, clear
		Framework::TestConfig::access_params().clear();
	}
	testScenario = _testScenario;
	if (testScenario)
	{
		// if is to be used, set from test scenario
		Framework::TestConfig::access_params().clear();
		Framework::TestConfig::access_params().set_from(testScenario->get_parameters());
	}
}

void Game::on_advance_and_display_loader(Loader::ILoader* _loader)
{
	base::on_advance_and_display_loader(_loader);

	manage_game_sounds_pause(true); // always pause game sounds
	if (::System::Core::is_rendering_paused())
	{
		manage_non_game_sounds_pause(true);
	}
	else
	{
		manage_non_game_sounds_pause(false);
	}
}

//

void Game::CameraRumble::reset()
{
	*this = CameraRumble();
}

void Game::CameraRumble::set(CameraRumbleParams const& _params)
{
	blendToTargetMaxTimeLeft = _params.blendInTime;
	autoBlendOutTime = _params.blendOutTime;
	if (_params.maxLocationOffset.is_set())
	{
		targetMaxLocationOffset = _params.maxLocationOffset.get();
	}
	if (_params.maxOrientationOffset.is_set())
	{
		targetMaxOrientationOffset = _params.maxOrientationOffset.get();
	}
	if (_params.interval.is_set())
	{
		interval = _params.interval.get();
	}
	if (_params.blendTime.is_set())
	{
		blendTime = _params.blendTime.get();
	}
}

void Game::CameraRumble::advance(float _deltaTime)
{
	intervalTimeLeft -= _deltaTime;
	if (!blendToTargetMaxTimeLeft.is_set() &&
		autoBlendOutTime.is_set())
	{
		blendToTargetMaxTimeLeft = autoBlendOutTime;
		autoBlendOutTime.clear();
		targetMaxLocationOffset = Range3::zero;
		targetMaxOrientationOffset = Range3::zero;
	}
	if (blendToTargetMaxTimeLeft.is_set())
	{
		blendToTargetMaxTimeLeft = blendToTargetMaxTimeLeft.get() - _deltaTime;
		float atTarget = 1.0f;
		if (blendToTargetMaxTimeLeft.get() > 0.0f)
		{
			atTarget = clamp(_deltaTime / blendToTargetMaxTimeLeft.get(), 0.0f, 1.0f);
		}
		else
		{
			blendToTargetMaxTimeLeft.clear();
		}
		currentMaxLocationOffset = lerp(atTarget, currentMaxLocationOffset, targetMaxLocationOffset);
		currentMaxOrientationOffset = lerp(atTarget, currentMaxOrientationOffset, targetMaxOrientationOffset);
	}
	else
	{
		currentMaxLocationOffset = targetMaxLocationOffset;
		currentMaxOrientationOffset = targetMaxOrientationOffset;
	}
	if (intervalTimeLeft <= 0.0f)
	{
		intervalTimeLeft = rg.get(interval);
		// this is okay to use here like this, but remember that different compilers make parameters run in different order!
		targetOffset = Transform(
			Vector3(rg.get(currentMaxLocationOffset.x),
					rg.get(currentMaxLocationOffset.y),
					rg.get(currentMaxLocationOffset.z)),
			Quat::identity);
		targetOffset = targetOffset.to_world(Transform(Vector3::zero, Rotator3(0.0f, rg.get(currentMaxOrientationOffset.z /* yaw */), 0.0f).to_quat()));
		targetOffset = targetOffset.to_world(Transform(Vector3::zero, Rotator3(rg.get(currentMaxOrientationOffset.x /* pitch */), 0.0f, 0.0f).to_quat()));
		targetOffset = targetOffset.to_world(Transform(Vector3::zero, Rotator3(0.0f, 0.0f, rg.get(currentMaxOrientationOffset.y /* roll */)).to_quat()));
	}
	if (PlayerSetup::get_current().get_preferences().useVignetteForMovement ||
		PlayerSetup::get_current().get_preferences().cameraRumble != Option::Allow)
	{
		targetOffset = Transform::identity;
	}
	currentOffset = blend_to_using_time(currentOffset, targetOffset, blendTime, _deltaTime);
}

//

#include "game_debug.inl"

