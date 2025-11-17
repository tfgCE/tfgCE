#pragma once

#include "..\..\globalSettings.h"

#ifdef AN_WITH_OPENXR
#include "openxr.h"

#include "..\vrSystemImpl.h"

#include "..\..\concurrency\gate.h"

#include "..\..\system\video\renderTarget.h"

//

#ifdef AN_QUEST
// this depends on quest as SteamVR's OpenXR runtime reports Oculus Touch controllers as hand tracking and gladly provides skeletal mesh
#define AN_OPENXR_WITH_HAND_TRACKING
#endif

// doesn't work
//#define XR_USE_GET_BOUND_2D

#ifdef AN_WINDOWS
	#define XR_USE_PLATFORM_WIN32
	#define XR_USE_GRAPHICS_API_OPENGL
	//#define XR_USE_META_METRICS
#endif
#ifdef AN_ANDROID
	#define XR_USE_PLATFORM_ANDROID
	#define XR_USE_GRAPHICS_API_OPENGL_ES 
#endif

#ifdef AN_OPENXR_WITH_HAND_TRACKING
	#define XR_EXTENSION_PROTOTYPES
#endif

#include <openxr.h>
#include <openxr_platform.h>

//

namespace vr
{
	class IVRSystem;
	class IVRChaperone;
	class IVRCompositor;
	class IVRInput;
	struct TrackedDevicePose_t;
};

namespace VR
{
	class OpenXR;

	class OpenXRImpl
	: public VRSystemImpl
	{
		typedef VRSystemImpl base;
	private: friend class OpenXR;
		static bool is_available();
			 
		OpenXRImpl(OpenXR* _owner, bool _firstChoice);

	public:
		void test_loop();

	protected:
		implement_ void init_impl();
		implement_ void shutdown_impl();

		implement_ void set_processing_levels_impl(Optional<float> const& _cpuLevel = NP, Optional<float> const& _gpuLevel = NP, bool _temporarily = false);
		implement_ void I_am_perf_thread_main_impl();
		implement_ void I_am_perf_thread_other_impl();
		implement_ void I_am_perf_thread_render_impl();
		implement_ void log_perf_threads_impl(LogInfoContext& _log);

		implement_ void get_ready_to_exit_impl();

		implement_ void create_device_impl();
		implement_ void enter_vr_mode_impl();
		implement_ void close_device_impl();
		
		implement_ void game_sync_impl();

		implement_ bool can_load_play_area_rect_impl();
		implement_ bool load_play_area_rect_impl(bool _loadAnything);

		implement_ void force_bounds_rendering(bool _force);

		implement_ float update_expected_frame_time();
		implement_ float calculate_ideal_expected_frame_time() const;

		implement_ bool is_ok() const { return isOk; }
		implement_ AvailableFunctionality get_available_functionality() const;

		implement_ void setup_rendering_on_init_video(::System::Video3D* _v3d);
		implement_ void create_internal_render_targets(::System::RenderTargetSetup const & _source, VectorInt2 const & _fallbackEyeResolution, OUT_ bool& _useDirectToVRRendering, OUT_ bool& _noOutputRenderTarget);
		implement_ VectorInt2 get_direct_to_vr_render_size(int _eyeIdx);
		implement_ float get_direct_to_vr_render_aspect_ratio_coef(int _eyeIdx);
		implement_ ::System::RenderTarget* get_direct_to_vr_render_target(int _eyeIdx);
		implement_ void on_render_targets_created();
		implement_ void set_render_mode(RenderMode::Type _mode);
		implement_ RenderInfo const & get_render_info() const { return renderModeData[renderMode].renderInfo; }

		implement_ void next_frame();

		implement_ void advance_events();
		implement_ void advance_pulses();
		implement_ void advance_poses();

		implement_ void update_track_device_indices();
		implement_ void init_hand_controller_track_device(int _index);
		implement_ void close_hand_controller_track_device(int _index);

		// rendering/presenting
		implement_ bool begin_render(System::Video3D* _v3d);
		implement_ bool end_render(System::Video3D* _v3d, ::System::RenderTargetPtr* _eyeRenderTargets);

		implement_ int translate_bone_index(VRHandBoneIndex::Type _index) const;

		implement_ DecideHandsResult::Type decide_hands(OUT_ int& _leftHand, OUT_ int& _rightHand);
		implement_ void update_hands_controls_source();

		implement_ void draw_debug_controls(int _hand, Vector2 const & _at, float _width, ::System::Font* _font);

		implement_ void update_render_scaling(REF_ float & _scale);

	public:
		bool check_openxr(XrResult _result, tchar const* _format, ...) const;
		bool check_openxr_warn(XrResult _result, tchar const* _format, ...) const;
		String get_xr_result_to_string(XrResult _result) const;
		String get_path_string(XrPath _path) const;

	private:
		bool isOk = false;

		bool firstChoice = true;

		//
		// runtime
		//

#ifdef AN_VIVE_ONLY
		XrPerfSettingsLevelEXT cpuLevel = XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
		XrPerfSettingsLevelEXT gpuLevel = XR_PERF_SETTINGS_LEVEL_SUSTAINED_HIGH_EXT;
#else
		XrPerfSettingsLevelEXT cpuLevel = XR_PERF_SETTINGS_LEVEL_SUSTAINED_LOW_EXT;
		XrPerfSettingsLevelEXT gpuLevel = XR_PERF_SETTINGS_LEVEL_SUSTAINED_LOW_EXT;
#endif

		Concurrency::SpinLock perfThreadLock;
		Optional<int> mainThreadTid;
		Array<int> allThreadsTid;
		Optional<int> renderThreadTid;

		struct PerformanceStats
		{
			struct Domain
			{
				float compositing = 1.0f;
				float rendering = 1.0f;
				float thermal = 1.0f;
			};

			Domain cpu;
			Domain gpu;

			struct FrameDropsBased
			{
				int frameDropsPrevPeriod = 0;
				int frameDropsThisPeriod = 0;
				float periodLength = 5.0f;
				::System::TimeStamp thisPeriodTS;
				float fineDynamicScaleRenderTarget = 1.0f;
			} frameDropsBased;
		} performanceStats;

		XrTime predictedDisplayTime = 0; // this one is used for displaying frame, it gets updated in begin_render
		XrTime syncPredictedDisplayTime = 0; // this one is actual game frame time, it gets updated in game_sync
		float deltaTimeFromSync = 0.0f;
		System::TimeStamp gameSyncTS;
		System::TimeStamp beginFrameTS;
		bool xrFrameSessionShouldRender = false;
		bool begunFrame = false;
		
		Concurrency::Gate beginFrameGate;
		Optional<XrTime> prevSyncPredictedDisplayTime; // previous frame, to provide the actual delta time

		bool hapticPerHandOn[Hand::MAX];

		int recenterPending = 0;

		//
		// general settings
		//

		XrFormFactor formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
		XrViewConfigurationType viewType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
		XrReferenceSpaceType referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
		bool useSingleRenderTarget = false;
		bool useSingleFrameBuffer = false;
		bool useSingleDepthRenderBuffer = false;

		//
		// selected system info provided
		//

		struct SystemInfo
		{
			String name; // used for checking whether we have a wireless device or not and for binding input
			// post system name read
			bool isAndroid = false;
			bool isOculus = false;
			bool isQuest = false;
			bool isVive = false;
			bool isPico = false;
		} system;

		Optional<float> systemBasedRefreshRate;
		Optional<float> displayRefreshRate;
		Optional<float> displayRefreshRateFromWaitFrame;

		//
		// system
		//

		::System::RenderTargetSetup renderTargetsCreatedForSource;
		VectorInt2 renderTargetsCreatedForFallbackEyeResolution;

		Array<const char*> extensionsInUse;

		bool xrFoveationAvailable = false;
		bool xrFoveationUsed = false;
		bool ownFoveationUsed = false;

		bool handTrackingAvailable = false;
		bool handTrackingSupported = false;
		bool handTrackingAimSupported = false;
		bool handTrackingMeshSupported = false;

		Optional<System::TimeStamp> enteredVRModeTS;
		XrInstance xrInstance = XR_NULL_HANDLE;
		XrSession xrSession = XR_NULL_HANDLE;
		XrSessionState xrSessionState = XR_SESSION_STATE_UNKNOWN;
		bool xrSessionRunning = false;
		bool xrSessionSynchronised = false;
		bool xrSessionInFocus = false;
		bool xrSessionVisible = false;
#ifdef AN_GL
		XrGraphicsBindingOpenGLWin32KHR graphicsBindingGL;
#endif
#ifdef AN_GLES
		XrGraphicsBindingOpenGLESAndroidKHR graphicsBindingGL;
#endif

		XrPosef const xrIdentityPose = { {0.0f,0.0f,0.0f,1.0f}, {0.0f,0.0f,0.0f} };
		XrSystemId xrSystemId = XR_NULL_SYSTEM_ID;
		XrSpace xrPlaySpace = XR_NULL_HANDLE;
#ifdef XR_USE_GET_BOUND_2D
		XrAsyncRequestIdFB xrSpatialEntityRequestId;
#endif
		/*
		input_state_t  xr_input = { };
		XrEnvironmentBlendMode   xr_blend = {};
		*/
		XrDebugUtilsMessengerEXT xrDebug = {};
		
		//
		// OpenXR extension module methods
		//

		PFN_xrCreateDebugUtilsMessengerEXT ext_xrCreateDebugUtilsMessengerEXT = nullptr;
		PFN_xrDestroyDebugUtilsMessengerEXT ext_xrDestroyDebugUtilsMessengerEXT = nullptr;
#ifdef AN_GL
		PFN_xrGetOpenGLGraphicsRequirementsKHR ext_xrGetOpenGLGraphicsRequirementsKHR = nullptr;
#endif
#ifdef AN_GLES
		PFN_xrGetOpenGLESGraphicsRequirementsKHR ext_xrGetOpenGLESGraphicsRequirementsKHR = nullptr;
#endif
		PFN_xrGetDisplayRefreshRateFB ext_xrGetDisplayRefreshRateFB = nullptr;
#ifdef AN_WINDOWS
		PFN_xrGetAudioOutputDeviceGuidOculus ext_xrGetAudioOutputDeviceGuidOculus = nullptr;
#endif
#ifdef AN_ANDROID
		PFN_xrSetAndroidApplicationThreadKHR ext_xrSetAndroidApplicationThreadKHR = nullptr;
#endif
		PFN_xrPerfSettingsSetPerformanceLevelEXT ext_xrPerfSettingsSetPerformanceLevelEXT = nullptr;
		PFN_xrCreateFoveationProfileFB ext_xrCreateFoveationProfileFB = nullptr;
		PFN_xrDestroyFoveationProfileFB ext_xrDestroyFoveationProfileFB = nullptr;
		PFN_xrUpdateSwapchainFB ext_xrUpdateSwapchainFB = nullptr;
		PFN_xrSetColorSpaceFB ext_xrSetColorSpaceFB = nullptr;
#ifdef XR_USE_GET_BOUND_2D
		PFN_xrSetSpaceComponentStatusFB ext_xrSetSpaceComponentStatusFB = nullptr;
		PFN_xrGetSpaceComponentStatusFB ext_xrGetSpaceComponentStatusFB = nullptr;
		PFN_xrCreateSpatialAnchorFB ext_xrCreateSpatialAnchorFB = nullptr;
		PFN_xrGetSpaceBoundary2DFB ext_xrGetSpaceBoundary2DFB = nullptr;
#endif
		PFN_xrApplyFoveationHTC ext_xrApplyFoveationHTC = nullptr;
#ifdef XR_USE_META_METRICS
		PFN_xrSetPerformanceMetricsStateMETA ext_xrSetPerformanceMetricsStateMETA = nullptr;
		PFN_xrEnumeratePerformanceMetricsCounterPathsMETA ext_xrEnumeratePerformanceMetricsCounterPathsMETA = nullptr;
		PFN_xrQueryPerformanceMetricsCounterMETA ext_xrQueryPerformanceMetricsCounterMETA = nullptr;
		Array<XrPath> performanceMetricsCounterPathsMETA;
#endif

#ifdef AN_OPENXR_WITH_HAND_TRACKING
		PFN_xrCreateHandTrackerEXT ext_xrCreateHandTrackerEXT = nullptr;
		PFN_xrDestroyHandTrackerEXT ext_xrDestroyHandTrackerEXT = nullptr;
		PFN_xrLocateHandJointsEXT ext_xrLocateHandJointsEXT = nullptr;
		PFN_xrGetHandMeshFB ext_xrGetHandMeshFB = nullptr;
#endif

		//
		// rendering
		//

		struct Swapchain
		{
#ifdef AN_GL
			typedef XrSwapchainImageOpenGLKHR XrSwapchainImage;
#endif
#ifdef AN_GLES
			typedef XrSwapchainImageOpenGLESKHR XrSwapchainImage;
#endif
			XrSwapchain xrSwapchain = XR_NULL_HANDLE;
			Array<XrSwapchainImage> images;
			int foveatedSetupCount = 0;

			XrSwapchain xrDepthSwapchain = XR_NULL_HANDLE;
			Array<XrSwapchainImage> depthImages;

			Array<GLuint> imageFrameBufferIDs; // shared, used only when rendering, used by single render target (directToVRRenderTarget below as dynamic proxy)
			Array<GLuint> depthRenderBufferIDs; // same amount as images

			bool withDepthRenderBuffer = false;
			bool withDepthSwapchain = false;
			bool withDepthStencil = false;

			Array<::System::RenderTargetPtr> directToVRRenderTargets; // this might be static or dynamic proxy render target, changed on each get (if dynamic)
		};

		struct RenderForEye
		{
			Optional<uint32_t> acquiredImageIndex;
			Optional<uint32_t> acquiredDepthImageIndex;

			void reset()
			{
				// make sure we released it already
				an_assert(!acquiredImageIndex.is_set());
				an_assert(!acquiredDepthImageIndex.is_set());
				acquiredImageIndex.clear();
				acquiredDepthImageIndex.clear();
			}
		};

		// these are read once or common for all render modes (xrView is used to read pose every frame)
		Array<XrView> xrViews;
		Array<XrViewConfigurationView> xrConfigViews;
		Array<VectorInt2> recommendedViewSize;

		// used by separate render modes (although some could be reused, but to keep them separate
		struct RenderModeData
		{
			RenderInfo renderInfo;
			Array<VectorInt2> viewSize;
			Array<float> aspectRatioCoef;

			Array<XrCompositionLayerProjectionView> xrProjectionViews;
			Array<XrCompositionLayerDepthInfoKHR> xrDepthInfos;

			Array<Swapchain> xrSwapchains;

			XrFoveationProfileFB xrFoveationProfile = XR_NULL_HANDLE;

			RenderForEye begunRenderForEye[Eye::Count];
		};

		Transform viewInReadViewPoseSpace = Transform::identity;

		InternalRenderMode::Type renderMode = InternalRenderMode::Normal;
		InternalRenderMode::Type pendingRenderMode = InternalRenderMode::Normal;
		RenderModeData renderModeData[InternalRenderMode::COUNT];

		//
		// input
		//

		XrActionSet xrActionSet;
		XrPath xrHandPaths[Hand::MAX];

		XrSpace xrViewPoseSpace;
		XrSpace xrHandAimPoseSpaces[Hand::MAX];
		XrSpace xrHandGripPoseSpaces[Hand::MAX];

		// used for spaces
		XrAction xrHandAimPoseAction;
		XrAction xrHandGripPoseAction;

		// generic input
		XrAction xrTriggerPressAsBoolAction;
		XrAction xrTriggerPressAsFloatAction;
		XrAction xrTriggerTouchAsBoolAction;
		XrAction xrGripPressAsBoolAction;
		XrAction xrGripPressAsFloatAction;
		XrAction xrGripTouchAsBoolAction;
		XrAction xrMenuPressAsBoolAction;
		XrAction xrMenuTouchAsBoolAction;
		XrAction xrPrimaryPressAsBoolAction;
		XrAction xrPrimaryTouchAsBoolAction;
		XrAction xrSecondaryPressAsBoolAction;
		XrAction xrSecondaryTouchAsBoolAction;
		XrAction xrThumbRestTouchAsBoolAction;
		XrAction xrThumbstickAsVector2Action;
		XrAction xrThumbstickPressAsBoolAction;
		XrAction xrThumbstickTouchAsBoolAction;
		XrAction xrTrackpadAsVector2Action;
		XrAction xrTrackpadPressAsBoolAction;
		XrAction xrTrackpadTouchAsBoolAction;

		// haptic output
		XrAction xrHapticOutputAction;

		// interaction/input profiles
		XrPath xrInputProfileGeneric;
		XrPath xrInputProfileHPMixedReality;
		XrPath xrInputProfileHTCVive;
		XrPath xrInputProfileHTCViveCosmos;
		XrPath xrInputProfileHTCViveFocus3;
		XrPath xrInputProfileMicrosoftMotion;
		XrPath xrInputProfileOculusTouch;
		XrPath xrInputProfilePicoNeo3Controller; // used also for pico 4
		XrPath xrInputProfileValveIndex;

#ifdef AN_OPENXR_WITH_HAND_TRACKING
		// hand tracking
		bool handTrackingInitialised = false;
		XrHandTrackerEXT xrHandTrackers[Hand::MAX];
#endif

		//
		// functions
		//

		bool load_ext_function(char const * _functionName, void* _functionPtr, bool _optional = false);

		void close_internal_render_targets();

		struct TrackingState
		{
			Optional<Transform> viewPose;
			Optional<Transform> handPoses[Hand::MAX];

#ifdef AN_OPENXR_WITH_HAND_TRACKING
			struct HandTracking
			{
				bool active = false;

				Optional<Transform> handPose;
				ArrayStatic<Transform, XR_HAND_JOINT_COUNT_EXT> jointsHS; // hand space

				Optional<Transform> aimPose;
				Optional<float> pointerPinch;
				Optional<float> middlePinch;
				Optional<float> ringPinch;
				Optional<float> pinkyPinch;
				bool systemGesture = false;
				bool menuPressed = false;
			};
			HandTracking handTracking[Hand::MAX];
#endif
		};
		void store(VRPoseSet& _poseSet, OPTIONAL_ VRControls* _controls, TrackingState const& _trackingState);
		void read_tracking_state(OUT_ TrackingState& _trackingState, XrTime _atTime);

		void begin_render_for_eye(System::Video3D* _v3d, Eye::Type _eye);
		void end_render_for_eye(Eye::Type _eye);
		
		void update_perf_threads();
		void set_perf_thread(int _type, int _threadId, tchar const* _what);

		void create_hand_trackers();
		void close_hand_trackers();

		void log_performance_stats() const;
	};

};
#endif