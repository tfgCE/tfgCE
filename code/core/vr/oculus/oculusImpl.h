#pragma once

#include "..\..\globalSettings.h"

#ifdef AN_WITH_OCULUS
#include "OVR_CAPI_GL.h"

#include "oculus.h"

#include "..\vrSystemImpl.h"

namespace VR
{
	class Oculus;

	struct OculusImplRenderInfo
	: public RenderInfo
	{
		ovrEyeRenderDesc eyeRenderDesc[2];
		ovrTextureSwapChain eyeTextureSwapChain[2] = { nullptr, nullptr };
		GLuint eyeFrameBufferID[2] = { 0, 0 };
		ovrRecti eyeViewport[2];
		VectorInt2 frameBufferSize[2];
	};

	class OculusImpl
	: public VRSystemImpl
	{
		typedef VRSystemImpl base;
	private: friend class Oculus;
		static bool is_available();
			 
		OculusImpl(Oculus* _owner);

	protected:
		implement_ void init_impl();
		implement_ void shutdown_impl();

		implement_ void create_device_impl();
		implement_ void enter_vr_mode_impl() {}
		implement_ void close_device_impl();
		
		implement_ void game_sync_impl() {}

		implement_ bool can_load_play_area_rect_impl();
		implement_ bool load_play_area_rect_impl(bool _loadAnything);

		implement_ void force_bounds_rendering(bool _force);

		implement_ float update_expected_frame_time();
		implement_ float calculate_ideal_expected_frame_time() const;

		implement_ bool is_ok() const { return session != nullptr; }

		implement_ void setup_rendering_on_init_video(::System::Video3D* _v3d);
		implement_ void create_internal_render_targets(::System::RenderTargetSetup const & _source, VectorInt2 const & _fallbackEyeResolution, OUT_ bool& _useDirectToVRRendering, OUT_ bool& _noOutputRenderTarget);		implement_ ::System::RenderTarget* get_direct_to_vr_render_target(int _eyeIdx) { return nullptr; }
		implement_ void on_render_targets_created();
		implement_ void set_render_mode(RenderMode::Type _mode);
		implement_ RenderInfo const & get_render_info() const { return renderInfo; }

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

		DecideHandsResult::Type decide_hands(OUT_ int & _leftHand, OUT_ int & _rightHand);
		void update_hands_controls_source();

		implement_ tchar const* get_prompt_controller_suffix(Input::DeviceID _source) const;

		implement_ void draw_debug_controls(int _hand, Vector2 const & _at, float _width, ::System::Font* _font);

		implement_ void update_render_scaling(REF_ float & _scale);

	private:
		bool initialisedLibOVR = false;
		bool libOVR_OK = false;

		String trackingSystemName;
		String modelNumber;

		ovrSession session = nullptr;
		ovrGraphicsLuid luid;

		ovrHmdDesc hmdDesc;
		int devicesMask = 0;
		bool overlayPresent = false;
		bool hasInputFocus = false;

		ovrPerfStats perfStats;
		int perfStatsFrame = NONE;

		int frameIndex = 0;
		int frameIndexToSubmit = 0; // frameIndexToSubmit <= frameIndex
		double renderPoseSetTimeSeconds = 0.0f; // sensor sample time
		ovrPosef eyeRenderPoses[2];
		Optional<double> frameTimeSeconds[2];

		OculusImplRenderInfo renderInfo;

		Transform viewInHeadPoseSpace = Transform::identity;

		void close_internal_render_targets();

		void store(VRPoseSet & _poseSet, OPTIONAL_ VRControls * _controls, ovrTrackingState& _trackingState);

		void update_perf_stats();

		static void oculus_log_callback(uintptr_t userData, int level, const char* message);
	};

};
#endif
