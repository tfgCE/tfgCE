#pragma once

#include "..\loaderHubScene.h"
#include "..\loaderHubWidget.h"

#include "..\..\..\roomGenerators\roomGenerationInfo.h"

namespace TeaForGodEmperor
{
	struct PlayerPreferences;
};

namespace Loader
{
	namespace HubScenes
	{
		class Options
		: public HubScene
		{
			FAST_CAST_DECLARE(Options);
			FAST_CAST_BASE(HubScene);
			FAST_CAST_END();

			typedef HubScene base;

		public:
			enum Type
			{
				Main,	// all options available
				InGame	// some options are not available (play area modification)
			};

			enum Flags
			{
				GetCurrentForward = 1
			};

		public:
			Options(Type _type = Main, int _flags = 0);
			~Options();

			void go_back(); // and go back to previous scene

		public: // HubScene
			implement_ void on_activate(HubScene* _prev);
			implement_ void on_update(float _deltaTime);
			implement_ void on_deactivate(HubScene* _next);
			implement_ void on_post_render(Framework::Render::Scene* _scene, Framework::Render::Context & _context);
			implement_ void on_post_render_render_scene(int _eyeIdx);
			implement_ bool does_want_to_end() { return false; } // never wants to end hub, it should go back to previous scene

		private:
			enum Screen
			{
				None,
				MainScreen,
				AestheticsOptionsScreen,
				VideoOptionsScreen,
				VideoOptionsLayoutScreen,
				SoundOptionsScreen,
				DevicesOptionsScreen,
				DevicesControllersOptionsScreen,
				DevicesOWOOptionsScreen,
				DevicesBhapticsOptionsScreen,
				ControlsOptionsScreen,
				ControlsVROffsetsScreen,
				GameplayOptionsScreen,
				ComfortOptionsScreen,
				PlayAreaOptionsScreen,
				PlayAreaCalibrateScreen,
				GeneralOptionsScreen,
				DebugOptionsScreen,
				CheatsOptionsScreen,
				SetupFoveationParamsScreen,
			};

			RefCountObjectPtr<HubScene> prevScene;
			Type type;
			int flags;
			Screen screen = None;
			Rotator3 forwardDir;

			Optional<Screen> delayedScreen;
			bool delayedScreenRefresh = false;
			bool delayedScreenResetForward = false;
			float delayedScreenDelay = 0.0f;

			::System::TimeStamp requestFoveatedRenderingTestTS;
			bool renderFoveatedRenderingTest = false;

			// devices controllers
			//
			struct DevicesControllersOptions
			{
				int vrHapticFeedback = 0;

				bool operator==(DevicesControllersOptions const & _other) const
				{
					return true
						&& vrHapticFeedback == _other.vrHapticFeedback
						;
				}
				void read();
				void apply();
			};
			DevicesControllersOptions usedDevicesControllersOptions; // as they are now being used
			DevicesControllersOptions devicesControllersOptions; // set by user


			// devices OWO
			//
			struct DevicesOWOOptions
			{
				String owoAddress;
				int owoPort = 0;

				// not setup, info for UI
				enum Status
				{
					NotConnected,
					Connecting,
					Connected,
				} status = NotConnected;
				Optional<Status> visibleStatus;
				bool testPending = false;
				bool exitOnConnected = false;

				bool operator==(DevicesOWOOptions const & _other) const
				{
					return true
						&& owoAddress == _other.owoAddress
						&& owoPort == _other.owoPort
						;
				}
				void read();
				void apply(bool _initialise);
			};
			DevicesOWOOptions usedDevicesOWOOptions; // as they are now being used
			DevicesOWOOptions devicesOWOOptions; // set by user


			// devices bhaptics
			//
			struct DevicesBhapticsOptions
			{
				bool autoScanning = true;
				bool showedNoPlayer = false;
				::System::TimeStamp getDevicesTS;

				bool operator==(DevicesBhapticsOptions const & _other) const
				{
					return true
						;
				}
				void read();
				void apply(bool _initialise);
			};
			DevicesBhapticsOptions devicesBhapticsOptions; // set by user


			// sound
			//
			struct SoundOptions
			{
				int mainVolume = 100;
				int soundVolume = 100;
				int environmentVolume = 100;
				int musicVolume = 100;
				int voiceoverVolume = 100;
				int pilgrimOverlayVolume = 100;
				int uiVolume = 100;
				int subtitles = 1;
				int subtitlesScale = 100;
				int narrativeVoiceover = 1;
				int pilgrimOverlayVoiceover = 1;

				bool operator==(SoundOptions const & _other) const
				{
					return true
						&& mainVolume == _other.mainVolume
						&& environmentVolume == _other.environmentVolume
						&& musicVolume == _other.musicVolume
						&& voiceoverVolume == _other.voiceoverVolume
						&& pilgrimOverlayVolume == _other.pilgrimOverlayVolume
						&& uiVolume == _other.uiVolume
						&& subtitles == _other.subtitles
						&& subtitlesScale == _other.subtitlesScale
						&& narrativeVoiceover == _other.narrativeVoiceover
						&& pilgrimOverlayVoiceover == _other.pilgrimOverlayVoiceover
						;
				}
				void read();
				void apply();
				void apply_volumes();
			};
			SoundOptions usedSoundOptions; // as they are now being used
			SoundOptions soundOptions; // set by user


			// aesthetics
			//
			struct AestheticsOptions
			{
				int style = 0;
				int ditheredTransparency = 0;

				bool operator==(AestheticsOptions const & _other) const
				{	return true
						&& style == _other.style
						&& ditheredTransparency == _other.ditheredTransparency
						;
				}
				void read();
				void apply();
			};
			AestheticsOptions usedAestheticsOptions; // as they are now being used
			AestheticsOptions aestheticsOptions; // set by user


			// video
			//
			struct VideoOptions
			{
				int msaaSamples = 0;
				int fxaa = 0;
				int bloom = 0;
				int extraWindows = 0;
				int extraDoors = 0;
				int displayIndex = 0;
				int fullScreen = 0;
				int vrResolutionCoef = 100;
				int aspectRatioCoef = 100;
				int directlyToVR = 0;
				int vrFoveatedRenderingLevel = 0;
				int vrFoveatedRenderingMaxDepth = 0;
				int allowAutoScale = 1;
				VectorInt2 resolution = VectorInt2::zero; // should be immediately updated when resolutionIndex changes
				int resolutionIndex = 0;
				int displayOnScreen = 0;
				//
				int layoutShowSecondary = 0;
				int layoutPIP = 0;
				int layoutPIPSwap = 0;
				int layoutMainSize = 0;
				int layoutMainZoom = 0;
				int layoutMainPlacementX = 0;
				int layoutMainPlacementY = 0;
				int layoutPIPSize = 0;
				int layoutSecondaryZoom = 0;
				int layoutSecondaryPlacementX = 0;
				int layoutSecondaryPlacementY = 0;
				int layoutShowBoundary = 0;

				bool operator==(VideoOptions const & _other) const
				{	return true
						&& msaaSamples == _other.msaaSamples
						&& fxaa == _other.fxaa
						&& bloom == _other.bloom
						&& extraWindows == _other.extraWindows
						&& extraDoors == _other.extraDoors
						&& displayIndex == _other.displayIndex
						&& fullScreen == _other.fullScreen
						&& vrResolutionCoef == _other.vrResolutionCoef
						&& aspectRatioCoef == _other.aspectRatioCoef
						&& directlyToVR == _other.directlyToVR
						&& vrFoveatedRenderingLevel == _other.vrFoveatedRenderingLevel
						&& vrFoveatedRenderingMaxDepth == _other.vrFoveatedRenderingMaxDepth
						&& allowAutoScale == _other.allowAutoScale
						&& resolution == _other.resolution
						&& displayOnScreen == _other.displayOnScreen
						//
						&& layoutShowSecondary == _other.layoutShowSecondary
						&& layoutPIP == _other.layoutPIP
						&& layoutPIPSwap == _other.layoutPIPSwap
						&& layoutMainSize == _other.layoutMainSize
						&& layoutMainZoom == _other.layoutMainZoom
						&& layoutMainPlacementX == _other.layoutMainPlacementX
						&& layoutMainPlacementY == _other.layoutMainPlacementY
						&& layoutPIPSize == _other.layoutPIPSize
						&& layoutSecondaryZoom == _other.layoutSecondaryZoom
						&& layoutSecondaryPlacementX == _other.layoutSecondaryPlacementX
						&& layoutSecondaryPlacementY == _other.layoutSecondaryPlacementY
						&& layoutShowBoundary == _other.layoutShowBoundary
						;
				}
				bool soft_reinitialise_is_ok(VideoOptions const& _other) const
				{
					return true
						&& displayIndex == _other.displayIndex
						&& fullScreen == _other.fullScreen
						&& resolution == _other.resolution
						;
				}
				bool are_new_render_targets_required(VideoOptions const& _other) const
				{
					return false
						|| msaaSamples != _other.msaaSamples
						|| vrResolutionCoef != _other.vrResolutionCoef
						|| aspectRatioCoef != _other.aspectRatioCoef
						|| directlyToVR != _other.directlyToVR
						|| vrFoveatedRenderingLevel != _other.vrFoveatedRenderingLevel
						|| vrFoveatedRenderingMaxDepth != _other.vrFoveatedRenderingMaxDepth
						//
						|| layoutShowSecondary != _other.layoutShowSecondary
						|| layoutPIP != _other.layoutPIP
						|| layoutPIPSwap != _other.layoutPIPSwap
						|| layoutMainSize != _other.layoutMainSize
						|| layoutMainZoom != _other.layoutMainZoom
						|| layoutMainPlacementX != _other.layoutMainPlacementX
						|| layoutMainPlacementY != _other.layoutMainPlacementY
						|| layoutPIPSize != _other.layoutPIPSize
						|| layoutSecondaryZoom != _other.layoutSecondaryZoom
						|| layoutSecondaryPlacementX != _other.layoutSecondaryPlacementX
						|| layoutSecondaryPlacementY != _other.layoutSecondaryPlacementY
						;
				}
				void read(::System::VideoConfig const* _vc = nullptr);
				void update_resolution(Array<VectorInt2> const & _resolutions);
				void update_resolution_index(Array<VectorInt2> const & _resolutions);
				void apply();
			};
			VideoOptions usedVideoOptions; // as they are now being used
			VideoOptions videoOptions; // set by user
			VideoOptions preFoveatedRenderingTestOptions; // for foveated rendering test
			VideoOptions videoOptionsAsInitialised;
			Array<VectorInt2> resolutionsForDisplay;


			// gameplay
			//
			struct GameplayOptions
			{
				int turnCounter = 0;
				int hitIndicatorType = 0;
				int healthIndicator = 0;
				int healthIndicatorIntensity = 0;
				int hitIndicatorIntensity = 0;
				int highlightInteractives = 0;
				int pocketsVerticalAdjustment = 0;
				int overlayInfoOnHeadSideTouch = 0;
				int showInHandEquipmentStatsAtGlance = 0;
				int keepInWorld = 0;
				int keepInWorldSensitivity = 0;
				int showSavingIcon = 0;
				int mapAvailable = 0;
				int doorDirections = 0;
				int pilgrimOverlayLockedToHead = 0;
				int followGuidance = 0;
				int sightsOuterRingRadius = 0;
				int sightsColourRed = 0;
				int sightsColourGreen = 0;
				int sightsColourBlue = 0;

				bool operator==(GameplayOptions const & _other) const
				{
					return true
						&& turnCounter == _other.turnCounter
						&& hitIndicatorType == _other.hitIndicatorType
						&& healthIndicator == _other.healthIndicator
						&& healthIndicatorIntensity == _other.healthIndicatorIntensity
						&& hitIndicatorIntensity == _other.hitIndicatorIntensity
						&& highlightInteractives == _other.highlightInteractives
						&& pocketsVerticalAdjustment == _other.pocketsVerticalAdjustment
						&& overlayInfoOnHeadSideTouch == _other.overlayInfoOnHeadSideTouch
						&& showInHandEquipmentStatsAtGlance == _other.showInHandEquipmentStatsAtGlance
						&& keepInWorld == _other.keepInWorld
						&& keepInWorldSensitivity == _other.keepInWorldSensitivity
						&& showSavingIcon == _other.showSavingIcon
						&& mapAvailable == _other.mapAvailable
						&& doorDirections == _other.doorDirections
						&& pilgrimOverlayLockedToHead == _other.pilgrimOverlayLockedToHead
						&& followGuidance == _other.followGuidance
						&& sightsOuterRingRadius == _other.sightsOuterRingRadius
						&& sightsColourRed == _other.sightsColourRed
						&& sightsColourGreen == _other.sightsColourGreen
						&& sightsColourBlue == _other.sightsColourBlue
						;
				}
				void read(TeaForGodEmperor::PlayerPreferences const* psp = nullptr);
				void apply();
			};
			GameplayOptions usedGameplayOptions; // as they are now being used
			GameplayOptions gameplayOptions; // set by user


			// comfort
			//
			struct ComfortOptions
			{
				int forcedEnvironmentMix = 0;
				int forcedEnvironmentMixPt = 0;
				int ignoreForcedEnvironmentMixPt = 1; // this is just for interface to allow changing first option without affecting set time
				int useVignetteForMovement = 0;
				int vignetteForMovementStrength = 0;
				int cartsTopSpeed = 0;
				int cartsSpeedPct = 0;
				int allowCrouch;
				int turnCounter;
				int flickeringLights;
				int cameraRumble;
				int rotatingElevators;

				bool operator==(ComfortOptions const & _other) const
				{
					return true
						&& forcedEnvironmentMix == _other.forcedEnvironmentMix
						&& forcedEnvironmentMixPt == _other.forcedEnvironmentMixPt
						&& useVignetteForMovement == _other.useVignetteForMovement
						&& vignetteForMovementStrength == _other.vignetteForMovementStrength
						&& cartsTopSpeed == _other.cartsTopSpeed
						&& cartsSpeedPct == _other.cartsSpeedPct
						&& allowCrouch == _other.allowCrouch
						&& turnCounter == _other.turnCounter
						&& flickeringLights == _other.flickeringLights
						&& cameraRumble == _other.cameraRumble
						&& rotatingElevators == _other.rotatingElevators
						;
				}
				void read(TeaForGodEmperor::PlayerPreferences const* psp = nullptr);
				void apply();
			};
			ComfortOptions usedComfortOptions; // as they are now being used
			ComfortOptions comfortOptions; // set by user


			// controls
			//
			struct ControlsOptions
			{
				int autoMainEquipment = 0;
				int dominantHand = 0;
				int joystickInputBlocked = 0;
				int joystickSwap = 0;
				int immobileVR = 0;
				int immobileVRSnapTurn = 0;
				int immobileVRSnapTurnAngle = 30;
				int immobileVRContinuousTurnSpeed = 90;
				int immobileVRSpeed = 150;
				int immobileVRMovementRelativeTo = 0;

				int vrOffsetPitch = 0;
				int vrOffsetYaw = 0;
				int vrOffsetRoll = 0;

				int confirmedVROffsetPitch = 0;
				int confirmedVROffsetYaw = 0;
				int confirmedVROffsetRoll = 0;

				struct GIOption
				{
					Name name;
					int onOff;
					int inOptionsNegated;
					Name locStrId;

					bool operator==(GIOption const & _other) const
					{
						return name == _other.name &&
							   onOff == _other.onOff &&
							   inOptionsNegated == _other.inOptionsNegated;
					}
				};
				ArrayStatic<GIOption, 32> options;

				ControlsOptions()
				{
					SET_EXTRA_DEBUG_INFO(options, TXT("HubScenes::Options::ControlsOptions.options"));
				}

				bool operator==(ControlsOptions const & _other) const
				{
					if (true
						&& autoMainEquipment == _other.autoMainEquipment
						&& dominantHand == _other.dominantHand
						&& joystickInputBlocked == _other.joystickInputBlocked
						&& joystickSwap == _other.joystickSwap
						&& immobileVR == _other.immobileVR
						&& immobileVRSnapTurn == _other.immobileVRSnapTurn
						&& immobileVRSnapTurnAngle == _other.immobileVRSnapTurnAngle
						&& immobileVRContinuousTurnSpeed == _other.immobileVRContinuousTurnSpeed
						&& immobileVRMovementRelativeTo == _other.immobileVRMovementRelativeTo
						&& immobileVRSpeed == _other.immobileVRSpeed
						&& vrOffsetPitch == _other.vrOffsetPitch
						&& vrOffsetYaw == _other.vrOffsetYaw
						&& vrOffsetRoll == _other.vrOffsetRoll
						&& options.get_size() == _other.options.get_size()
						)
					{
						for_count(int, i, options.get_size())
						{
							if (options[i].name != _other.options[i].name ||
								options[i].onOff != _other.options[i].onOff ||
								options[i].inOptionsNegated != _other.options[i].inOptionsNegated)
							{
								return false;
							}
						}
						return true;
					}
					return false;
				}
				void read(TeaForGodEmperor::PlayerPreferences const* psp = nullptr);
				void apply();
				void apply_vr_offsets(bool _confirmed);
				void confirm_vr_offsets();
				void copy_vr_offsets_from(ControlsOptions const& _other)
				{
					vrOffsetPitch = _other.vrOffsetPitch;
					vrOffsetYaw = _other.vrOffsetYaw;
					vrOffsetRoll = _other.vrOffsetRoll;
					confirmedVROffsetPitch = _other.confirmedVROffsetPitch;
					confirmedVROffsetYaw = _other.confirmedVROffsetYaw;
					confirmedVROffsetRoll = _other.confirmedVROffsetRoll;
				}
			};
			ControlsOptions usedControlsOptions; // as they are now being used
			ControlsOptions controlsOptions; // set by user
			Optional<float> testVROffsetsTimeLeft;
			Optional<int> testVROffsetsTimeLeftShowSeconds;


			// play area
			//
			struct PlayAreaOptions
			{
				int immobileVR;
				int immobileVRSize;
				int measurementSystem; // -1 auto
				int border;
				int horizontalScaling;
				int preferredTileSize;
				int enlarged; // info only
				int doorHeight;
				int scaleUpPlayer;
				int allowCrouch;
				int turnCounter;
				int keepInWorld;
				int keepInWorldSensitivity;
				float manualRotate;
				float manualOffsetX;
				float manualOffsetY;
				float manualSizeX;
				float manualSizeY;

				bool operator==(PlayAreaOptions const & _other) const
				{
					return true
						&& immobileVR == _other.immobileVR
						&& immobileVRSize == _other.immobileVRSize
						&& measurementSystem == _other.measurementSystem
						&& border == _other.border
						&& horizontalScaling == _other.horizontalScaling
						&& preferredTileSize == _other.preferredTileSize
						&& doorHeight == _other.doorHeight
						&& scaleUpPlayer == _other.scaleUpPlayer
						&& allowCrouch == _other.allowCrouch
						&& turnCounter == _other.turnCounter
						&& keepInWorld == _other.keepInWorld
						&& keepInWorldSensitivity == _other.keepInWorldSensitivity
						&& manualRotate == _other.manualRotate
						&& manualOffsetX == _other.manualOffsetX
						&& manualOffsetY == _other.manualOffsetY
						&& manualSizeX == _other.manualSizeX
						&& manualSizeY == _other.manualSizeY
						;
				}
				void read();
				void apply(bool _modifySize);
				void apply_vr_map(bool _modifySize, bool _applyToVR = true);

				MeasurementSystem::Type get_measurement_system() const;

				TeaForGodEmperor::PreferredTileSize::Type get_preferred_tile_size() const { return (TeaForGodEmperor::PreferredTileSize::Type)preferredTileSize; }
			};
			PlayAreaOptions usedPlayAreaOptions; // as they are now being used
			PlayAreaOptions playAreaOptions; // set by user
			PlayAreaOptions playAreaOptionsBackupForCalibration; // if we cancel calibration
			TeaForGodEmperor::RoomGenerationInfo previewRGI;
			enum CalibrationOption
			{
				Placement,
				Size,

				Count
			} calibrationOption = CalibrationOption::Placement;
			Optional<CalibrationOption> visibleCalibrationOption;
			Optional<Vector2> visibleCalibrationSize;


			// general
			//
			struct GeneralOptions
			{
				int reportBugs;
				int showTipsOnLoading;
				int immobileVR;
				int measurementSystem; // -1 auto
				int language;
				int subtitles;
				int subtitlesScale = 100;
				int narrativeVoiceover = 1;
				int pilgrimOverlayVoiceover = 1;
				int showTipsDuringGame;
				int allowVRScreenShots;
				int showGameplayInfo;
				int showRealTime;

				bool operator==(GeneralOptions const & _other) const
				{
					return true
						&& reportBugs == _other.reportBugs 
						&& showTipsOnLoading == _other.showTipsOnLoading
						&& immobileVR == _other.immobileVR
						&& measurementSystem == _other.measurementSystem
						&& language == _other.language
						&& subtitles == _other.subtitles
						&& subtitlesScale == _other.subtitlesScale
						&& narrativeVoiceover == _other.narrativeVoiceover
						&& pilgrimOverlayVoiceover == _other.pilgrimOverlayVoiceover
						&& showTipsDuringGame == _other.showTipsDuringGame
						&& allowVRScreenShots == _other.allowVRScreenShots
						&& showGameplayInfo == _other.showGameplayInfo
						&& showRealTime == _other.showRealTime
						;
				}
				void read();
				void apply(bool _applyToVR);
				void apply_language_subtitles_voiceovers();
			};
			GeneralOptions usedGeneralOptions; // as they are now being used
			GeneralOptions generalOptions; // set by user

			void delayed_show_screen(float _delay, Screen _screen, Optional<bool> _refresh = NP, Optional<bool> _resetForward = NP);
			void show_screen(Screen _screen, bool _refresh = false, bool _keepForwardDir = false);

			void haptics_accept(); // general acceptance of the options and the setup, as they are (should be running at this point, bhaptics java does so)
			void haptics_disable(); // disable (in main config + terminate physical instance)
			
			void haptics_debug_test_all(); // test all

			void bhaptics_op(Name const& _op);
			void bhaptics_auto_scan();
			void owo_test_devices();

			void apply_devices_controllers_options();
			void apply_devices_owo_options(bool _initialise);
			void apply_sound_options();
			void apply_sound_volumes(); // <- sound
			void apply_aesthetics_options();
			void apply_video_options(bool _remainWhereWeAre = false);
			void apply_gameplay_options();
			void apply_comfort_options();
			void apply_controls_options();
			void apply_controls_vr_offsets_options(bool _test_remainWhereWeAre = false);
			void apply_play_area_options();
			void apply_general_options();
			void apply_language(); // <- general
			void update_and_apply_vr_changes(); // play area options or general

			void reset_turn_counter();
			void reset_gameplay_options();
			void reset_comfort_options();
			void reset_play_area_options();
			void reset_video_options();

			void update_resolutions();

			void on_options_applied();

			void update_preview_play_area();

			void update_sights_colour(bool _colour);

			Rotator3 read_forward_dir() const;
		};
	};
};
