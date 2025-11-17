#pragma once

#include "..\..\globalSettings.h"

#ifdef AN_WITH_OPENVR
#include "openvr.h"

#include "..\vrSystemImpl.h"

#include <openvr.h>

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
	class OpenVR;

	class OpenVRImpl
	: public VRSystemImpl
	{
		typedef VRSystemImpl base;
	private: friend class OpenVR;
		static bool is_available();
			 
		OpenVRImpl(OpenVR* _owner);

		static void create_config_files();

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

		implement_ bool is_ok() const { return openvrSystem != nullptr; }

		implement_ void setup_rendering_on_init_video(::System::Video3D* _v3d);
		implement_ void create_internal_render_targets(::System::RenderTargetSetup const & _source, VectorInt2 const & _fallbackEyeResolution, OUT_ bool& _useDirectToVRRendering, OUT_ bool& _noOutputRenderTarget);
		implement_::System::RenderTarget* get_direct_to_vr_render_target(int _eyeIdx) { return nullptr; }
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

	protected:
		Meshes::Mesh3D* load_mesh(Name const& _modelName) const;

	private:
		vr::IVRSystem* openvrSystem = nullptr;
		vr::IVRChaperone* openvrChaperone = nullptr;
		vr::IVRCompositor* openvrCompositor = nullptr;
		vr::IVRInput* openvrInput = nullptr;

		::System::RenderTargetPtr eyeNativeOutputRenderTargets[2]; // useful when output has different resolution

		struct HandDeviceInfo
		{
			String trackingSystemName;
			String modelNumber;
			vr::VRInputValueHandle_t inputHandle = 0;
			vr::VRActionHandle_t handSkeletonActionHandle = 0;
			vr::VRActionHandle_t menuActionHandle = 0;
			vr::VRActionHandle_t buttonAActionHandle = 0;
			vr::VRActionHandle_t buttonBActionHandle = 0;
			vr::VRActionHandle_t buttonTriggerActionHandle = 0;
			vr::VRActionHandle_t buttonGripActionHandle = 0;
			vr::VRActionHandle_t buttonTrackpadActionHandle = 0;
			vr::VRActionHandle_t touchTrackpadActionHandle = 0;
			vr::VRActionHandle_t joystickActionHandle = 0;
			vr::VRActionHandle_t trackpadActionHandle = 0;
			vr::VRActionHandle_t triggerActionHandle = 0;
			vr::VRActionHandle_t gripActionHandle = 0;
		};
		HandDeviceInfo handDeviceInfos[Hands::Count];

		vr::VRActionHandle_t mainActionSetHandle = 0;
		vr::VRActiveActionSet_t activeActionSet;

		float vsyncToPhotonsTime = 0.0f;

		RenderInfo renderInfo;

		Transform viewInHeadPoseSpace = Transform::identity;

		void store(VRPoseSet & _poseSet, OPTIONAL_ VRControls * _controls, vr::TrackedDevicePose_t* _trackedDevicePoses);

	private:
		void init_openvr_input();

		static bool should_be_ignored(String const& _modelNumber);

		void load_play_area_rect_internal(bool _loadAnything, OUT_ Vector3& _halfForward, OUT_ Vector3& _halfRight) const;

	};

};
#endif