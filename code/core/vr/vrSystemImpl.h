#pragma once

#include "iVR.h"

#include "..\math\math.h"

namespace System
{
	class Video3D;
};

namespace vr
{
	class IVRSystem;
	class IVRChaperone;
	class IVRCompositor;
	struct TrackedDevicePose_t;
};

namespace VR
{
	class OpenVR;
	class VRSystem;

	class VRSystemImpl
	{
	protected: friend class VRSystem;
		VRSystemImpl(VRSystem* _owner);
		virtual ~VRSystemImpl();

		void init();
		void create_device();
		void enter_vr_mode();
		void init_video(::System::Video3D* _v3d);
		void close_device();
		void shutdown();

		VectorInt2 const & get_preferred_full_screen_resolution() { return preferredFullScreenResolution; }

		virtual bool is_ok() const = 0;

		virtual AvailableFunctionality get_available_functionality() const { return AvailableFunctionality(); }

		virtual tchar const* get_prompt_controller_suffix(Input::DeviceID _source) const { return TXT(""); }

		bool load_play_area_rect(bool _loadAnything = false);
		bool has_loaded_play_area_rect() const { return loadedPlayAreaRect; }
		
		bool is_wireless() const { return wireless; }
		bool may_have_invalid_boundary() const { return mayHaveInvalidBoundary; }
		bool is_boundary_unavailable() const { return boundaryUnavailable; }

		virtual void setup_rendering_on_init_video(::System::Video3D* _v3d) = 0;
		virtual void create_internal_render_targets(::System::RenderTargetSetup const & _source, VectorInt2 const & _fallbackEyeResolution, OUT_ bool& _useDirectToVRRendering, OUT_ bool& _noOutputRenderTarget) = 0;
		virtual VectorInt2 get_direct_to_vr_render_size(int _eyeIdx);
		virtual float get_direct_to_vr_render_aspect_ratio_coef(int _eyeIdx);
		virtual ::System::RenderTarget* get_direct_to_vr_render_target(int _eyeIdx) { return nullptr; }
		virtual void on_render_targets_created() = 0;
		virtual void set_render_mode(RenderMode::Type _mode) = 0;
		virtual RenderInfo const & get_render_info() const = 0;

		void advance();
		void game_sync();

		// rendering/presenting
		virtual bool begin_render(System::Video3D * _v3d) = 0;
		virtual bool end_render(System::Video3D * _v3d, ::System::RenderTargetPtr* _eyeRenderTargets) = 0;

		virtual Meshes::Mesh3D* load_mesh(Name const & _modelName) const { return nullptr; }
		virtual int translate_bone_index(VRHandBoneIndex::Type _index) const;

		virtual DecideHandsResult::Type decide_hands(OUT_ int & _leftHand, OUT_ int & _rightHand) = 0;
		virtual void update_on_decided_hands(int _leftHand, int _rightHand);
		virtual void update_hands_controls_source() = 0;

		virtual float update_expected_frame_time() = 0;
		virtual float calculate_ideal_expected_frame_time() const = 0;

		virtual void draw_debug_controls(int _hand, Vector2 const & _at, float _width, ::System::Font* _font) {}

		virtual void update_render_scaling(REF_ float & _scale) = 0;

	protected:
		virtual void update_track_device_indices() = 0;
		virtual int find_hand_by_tracked_device_index(int _trackedDeviceIndex) const;
		virtual void init_hand_controller_track_device(int _index) = 0;
		virtual void close_hand_controller_track_device(int _index) = 0;
		virtual void update_hands_available();

	protected:
		virtual void next_frame() = 0; // udpate frame index
		virtual void advance_events() = 0;
		virtual void advance_pulses() = 0;
		virtual void advance_poses() = 0;

	protected:
		virtual void handle_vr_mode_changes_impl() {}

		virtual void set_processing_levels_impl(Optional<float> const& _cpuLevel = NP, Optional<float> const& _gpuLevel = NP, bool _temporarily = false) {}
		virtual void I_am_perf_thread_main_impl() {}
		virtual void I_am_perf_thread_other_impl() {}
		virtual void I_am_perf_thread_render_impl() {}
		virtual void log_perf_threads_impl(LogInfoContext& _log) {}

		virtual void init_impl() = 0;
		virtual void create_device_impl() = 0; // before video3d available
		virtual void enter_vr_mode_impl() = 0; // video3d available here!
		virtual void game_sync_impl() = 0;
		virtual void close_device_impl() = 0;
		virtual void get_ready_to_exit_impl() {}
		virtual void shutdown_impl() = 0;

		virtual void force_bounds_rendering(bool _force) = 0;

		virtual bool can_load_play_area_rect_impl() = 0;
		virtual bool load_play_area_rect_impl(bool _loadAnything = false) = 0;

	protected:
		bool closeDeviceRequired = false;
		bool loadedPlayAreaRect = false;
		bool wireless = false;
		bool mayHaveInvalidBoundary = false;
		bool boundaryUnavailable = false; // change it only in load_play_area_rect_impl

		VRSystem* owner;

		VectorInt2 preferredFullScreenResolution;
		struct TrackedDevice
		{
			int controllerType = NONE; // may be NONE (left, right etc)
			int deviceID = NONE; // has to be non NONE (actual device id)
			Hand::Type hand = Hand::MAX;

			void clear()
			{
				controllerType = NONE;
				deviceID = NONE;
				hand = Hand::MAX;
			}
		};
		TrackedDevice handControllerTrackDevices[Hands::Count]; // deviceid

		//void store(VRPoseSet & _poseSet, OPTIONAL_ VRControls * _controls, vr::TrackedDevicePose_t* _trackedDevicePoses);
	};

};
