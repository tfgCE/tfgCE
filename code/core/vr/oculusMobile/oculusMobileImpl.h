#pragma once

#include "..\..\globalSettings.h"

#ifdef AN_WITH_OCULUS_MOBILE

//#define AN_OUTPUT_DETAILED_FRAMES

#include "oculusMobile.h"

#include "..\vrSystemImpl.h"

#include "..\..\concurrency\gate.h"

#ifdef AN_VRAPI
#include "VrApi.h"
#endif

namespace VR
{
	class OculusMobile;

	struct OculusMobileImplRenderInfo
	: public RenderInfo
	{
		struct Eye
		{
			VectorInt2 frameBufferSize = VectorInt2::zero;
			int	textureSwapChainLength = 0;
			int	textureSwapChainIndex = 0;
			ovrTextureSwapChain* colourTextureSwapChain = nullptr;
			GLuint* frameBuffers = nullptr;
			ovrTextureSwapChain* depthTextureSwapChain = nullptr;
			GLuint* depthBuffers = nullptr;
			Array<::System::RenderTargetPtr> directToVRRenderTargets;
		};
		Eye eye[2];
	};

	class OculusMobileImpl
	: public VRSystemImpl
	{
		typedef VRSystemImpl base;
	private: friend class OculusMobile;
		static bool is_available();
			 
		OculusMobileImpl(OculusMobile* _owner);

	protected:
		implement_ void init_impl();
		implement_ void shutdown_impl();

		implement_ void handle_vr_mode_changes_impl();

		implement_ void set_processing_levels_impl(Optional<float> const& _cpuLevel = NP, Optional<float> const& _gpuLevel = NP, bool _temporarily = false);
		implement_ void I_am_perf_thread_main_impl();
		implement_ void I_am_perf_thread_render_impl();

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

		implement_ bool is_ok() const { return true; }
		implement_ AvailableFunctionality get_available_functionality() const;

		implement_ void setup_rendering_on_init_video(::System::Video3D* _v3d);
		implement_ void create_internal_render_targets(::System::RenderTargetSetup const & _source, VectorInt2 const & _fallbackEyeResolution, OUT_ bool& _useDirectToVRRendering, OUT_ bool& _noOutputRenderTarget);
		implement_ ::System::RenderTarget* get_direct_to_vr_render_target(int _eyeIdx);
		implement_ void on_render_targets_created();
		implement_ void set_render_mode(RenderMode::Type _mode);
		implement_ RenderInfo const & get_render_info() const { return renderInfo[renderMode]; }

		implement_ void next_frame();

		implement_ void advance_events();
		implement_ void advance_pulses();
		implement_ void advance_poses();

		implement_ void update_track_device_indices();
		implement_ void init_hand_controller_track_device(int _index);
		implement_ void close_hand_controller_track_device(int _index);

		// rendering/presenting
		bool begin_render(System::Video3D * _v3d);
		bool end_render(System::Video3D * _v3d, ::System::RenderTargetPtr* _eyeRenderTargets);

		Meshes::Mesh3D* load_mesh(Name const & _modelName) const;
		int translate_bone_index(VRHandBoneIndex::Type _index) const;

		DecideHandsResult::Type decide_hands(OUT_ int & _leftHand, OUT_ int & _rightHand);
		void update_hands_controls_source();

		implement_ void draw_debug_controls(int _hand, Vector2 const & _at, float _width, ::System::Font* _font);

		implement_ void update_render_scaling(REF_ float & _scale);

	private:
		static bool s_vrapiInitialised;

		String trackingSystemName;
		String modelNumber;
		ovrJava java;
		ovrMobile* ovr = nullptr;

		int cpuLevel = 2;
		int gpuLevel = 2;
		Optional<int> mainThreadTid;
		Optional<int> renderThreadTid;

		int devicesMask = 0;
		bool overlayPresent = false;
		bool hasInputFocus = false;

		int recenterCount = 0;

		int frameIndex = 0;
		int frameIndexToSubmit = 0; // frameIndexToSubmit <= frameIndex
		double renderPoseSetTimeSeconds = 0.0f; // sensor sample time
		ovrTracking2 trackingStateForRendering; // for which it was rendered
		Optional<double> frameTimeSeconds[2];

		double predictedDisplayTime = 0.0;
		double syncPredictedDisplayTime = 0.0;

#ifdef AN_OUTPUT_DETAILED_FRAMES
		double lastPredictedDisplayTime = 0.0;
		double predictedAtTime = 0.0;
#endif

		bool usingFoveatedRendering = false;

		// for phase sync
		Concurrency::Gate beginFrameGate;
		Optional<double> prevSyncPredictedDisplayTime; // previous frame, to provide the actual delta time

		InternalRenderMode::Type renderMode = InternalRenderMode::Normal;
		InternalRenderMode::Type pendingRenderMode = InternalRenderMode::Normal;
		OculusMobileImplRenderInfo renderInfo[InternalRenderMode::COUNT];

		Transform viewInHeadPoseSpace = Transform::identity;

		OculusMobileImplRenderInfo const& get_oculus_render_info() const { return renderInfo[renderMode]; }
		OculusMobileImplRenderInfo& access_oculus_render_info() { return renderInfo[renderMode]; }

		void close_internal_render_targets();

		void update_perf_stats();
		
		void update_perf_threads();

		void store(VRPoseSet & _poseSet, OPTIONAL_ VRControls * _controls, ovrTracking2 const & _trackingState, ovrTracking const* _handsTrackingState);

		bool check_if_requires_to_update_track_device_indices() const;

		void process_vr_events(bool _requiresToHaveOVR);
	};

};
#endif
