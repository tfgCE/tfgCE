#pragma once

#include "..\..\globalSettings.h"

#ifdef AN_WITH_WAVEVR
#include "wavevr.h"

#include "..\vrSystemImpl.h"

#include <wvr\wvr_device.h>
#include <wvr\wvr_system.h>
#include <wvr\wvr_types.h>

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
	class WaveVR;

	class WaveVRImpl
	: public VRSystemImpl
	{
		typedef VRSystemImpl base;
	private: friend class WaveVR;
		static bool is_available();
			 
		WaveVRImpl(WaveVR* _owner);

	protected:
		implement_ void init_impl();
		implement_ void shutdown_impl();

		implement_ void set_processing_levels_impl(Optional<float> const& _cpuLevel = NP, Optional<float> const& _gpuLevel = NP, bool _temporarily = false);

		implement_ void create_device_impl();
		implement_ void enter_vr_mode_impl() {}
		implement_ void close_device_impl();
		
		implement_ void game_sync_impl() {}

		implement_ bool can_load_play_area_rect_impl();
		implement_ bool load_play_area_rect_impl(bool _loadAnything);

		implement_ void force_bounds_rendering(bool _force);

		implement_ float update_expected_frame_time();
		implement_ float calculate_ideal_expected_frame_time() const;

		implement_ bool is_ok() const { return wavevrOk; }
		implement_ AvailableFunctionality get_available_functionality() const;

		implement_ void setup_rendering_on_init_video(::System::Video3D* _v3d);
		implement_ void create_internal_render_targets(::System::RenderTargetSetup const & _source, VectorInt2 const & _fallbackEyeResolution, OUT_ bool& _useDirectToVRRendering, OUT_ bool& _noOutputRenderTarget);
		implement_ VectorInt2 get_direct_to_vr_render_size(int _eyeIdx);
		implement_ float get_direct_to_vr_render_aspect_ratio_coef(int _eyeIdx);
		implement_ ::System::RenderTarget* get_direct_to_vr_render_target(int _eyeIdx);
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
		implement_ bool begin_render(System::Video3D * _v3d);
		implement_ bool end_render(System::Video3D * _v3d, ::System::RenderTargetPtr* _eyeRenderTargets);

		implement_ DecideHandsResult::Type decide_hands(OUT_ int & _leftHand, OUT_ int & _rightHand);
		implement_ void update_hands_controls_source();

		implement_ void draw_debug_controls(int _hand, Vector2 const & _at, float _width, ::System::Font* _font);

		implement_ void update_render_scaling(REF_ float & _scale);

	private:
		bool wavevrOk = true;
		
		float refreshRate = 90.0f;

		struct RenderForEye
		{
			Optional<uint32_t> acquiredImageIndex;

			void reset()
			{
				// make sure we released it already
				an_assert(!acquiredImageIndex.is_set());
				acquiredImageIndex.clear();
			}
		};

		struct RenderData
		{
			VectorInt2 viewSize;
			float aspectRatioCoef;

			void* wvrRenderTextureQueue = nullptr;
			Array<GLuint> imageTextureIDs; // from queue
			Array<GLuint> imageFrameBufferIDs; // for each image
			Array<GLuint> depthRenderBufferIDs; // for each image
			Array<::System::RenderTargetPtr> directToVRRenderTargets; // this might be static or dynamic proxy render target, changed on each get (if dynamic)

			int foveatedSetupCount = 0;

			RenderForEye begunRenderForEye;
		};
		RenderData renderData[Eye::Count];

		bool begunFrame = false;

		WVR_DevicePosePair_t controlDevicePairs[WVR_DEVICE_COUNT_LEVEL_1];
		WVR_DevicePosePair_t renderDevicePairs[WVR_DEVICE_COUNT_LEVEL_1];

		WVR_PerfLevel cpuLevel = WVR_PerfLevel_Medium;
		WVR_PerfLevel gpuLevel = WVR_PerfLevel_Medium;

		float performanceLevel = 1.0f;

		struct HandDeviceInfo
		{
			uint32 button = 0;
			uint32 touch = 0;
			uint32 analog = 0;
		};
		HandDeviceInfo handDeviceInfos[Hands::Count];

		//
		// functions
		//

		void close_internal_render_targets();

		bool get_input_button(Hand::Type _hand, WVR_DeviceType _deviceID, WVR_InputId _id, OUT_ bool& _read);
		bool get_input_touch(Hand::Type _hand, WVR_DeviceType _deviceID, WVR_InputId _id, OUT_ bool& _read);
		bool get_input_analog(Hand::Type _hand, WVR_DeviceType _deviceID, WVR_InputId _id, OUT_ Vector2& _read);

		String get_controller_parameter(WVR_DeviceType _deviceID, char const* _prop);

		RenderInfo renderInfo;

		void store(VRPoseSet & _poseSet, OPTIONAL_ VRControls * _controls, WVR_DevicePosePair_t* _trackedDevicePoses);

		bool load_play_area_rect_internal(OUT_ Vector3& _halfForward, OUT_ Vector3& _halfRight) const;

		void update_eyes();

		void begin_render_for_eye(System::Video3D* _v3d, Eye::Type _eye);
		void end_render_for_eye(Eye::Type _eye);
	};

};
#endif