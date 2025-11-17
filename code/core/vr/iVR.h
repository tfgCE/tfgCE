#pragma once

#include "vrEnums.h"
#include "vrFovPort.h"
#include "vrInput.h"
#include "vrZone.h"

#include "..\mainConfig.h"

#include "..\math\math.h"

#include "..\memory\refCountObjectPtr.h"

#include "..\system\video\renderTargetPtr.h"
#include "..\system\video\video.h"

#define MAX_VR_SIZE 128.0f

//#define AN_INSPECT_VR_PLAY_AREA

//#define AN_INSPECT_VR_INVALID_ORIGIN
//#define AN_INSPECT_VR_INVALID_ORIGIN_CONTINUOUS

//#ifdef AN_DEVELOPMENT_OR_PROFILER

// if active, will show black screen on no render allowed
//#define SHOW_NOTHING_ON_DISALLOWED_VR_RENDER

namespace System
{
	struct RenderTargetSetup;
	class Font;
	class RenderTarget;
	class Shader;
	class ShaderProgram;
	class Video3D;
};

namespace Meshes
{
	class Mesh3D;
};

namespace VR
{
	class IVR;
	struct VRPoseSet;

	struct Scaling
	{
		float general = 1.0f;
		float horizontal = 1.0f;

		Scaling() {}
		Scaling(float _general, float _horizontal) : general(_general), horizontal(_horizontal) {}
	};

	struct RenderInfo
	{
		Transform eyeOffset[Eye::Count]; // these are pure offsets without scaling! they are used against IVR's view transforms!
		Transform get_eye_offset_to_use(int _eyeIdx) const; // use this to get with scaling!
		float fov[Eye::Count];
		VRFovPort fovPort[Eye::Count];
		Vector2 lensCentreOffset[Eye::Count]; // -1 to 1 (for both axes)
		VectorInt2 eyeResolution[Eye::Count]; // this is resolution of render target!
		float aspectRatio[Eye::Count];
		float renderScale[Eye::Count]; // to go up from fallback texture size to one that could be used for rendering

		RenderInfo()
		{
			// based on oculus
			eyeOffset[Eye::Left] = Transform(Vector3(-0.05f, 0.0f, 0.0f), Quat::identity);
			eyeOffset[Eye::Right] = Transform(Vector3(0.05f, 0.0f, 0.0f), Quat::identity);
			lensCentreOffset[Eye::Left] = Vector2(0.151976f, 0.0f);
			lensCentreOffset[Eye::Right] = Vector2(-0.151976f, 0.0f);
			fov[Eye::Left] = 111.0f;
			fov[Eye::Right] = 111.0f;
			aspectRatio[0] = 0.8f;
			aspectRatio[1] = 0.8f;
			renderScale[0] = 1.71f;
			renderScale[1] = 1.71f;

			// based on vive (1512x1680)
			eyeOffset[Eye::Left] = Transform(Vector3(-0.036f, 0.014f, 0.0f), Quat::identity);
			eyeOffset[Eye::Right] = Transform(Vector3(0.036f, 0.014f, 0.0f), Quat::identity);
			lensCentreOffset[Eye::Left] = Vector2(0.055f, 0.0f);
			lensCentreOffset[Eye::Right] = Vector2(-0.055f, 0.0f);
			fov[Eye::Left] = 111.0f;
			fov[Eye::Right] = 111.0f;
			aspectRatio[0] = 0.9f;
			aspectRatio[1] = 0.9f;
			renderScale[0] = 1.4f;
			renderScale[1] = 1.4f;

			// common
			eyeResolution[0] = VectorInt2::zero;
			eyeResolution[1] = VectorInt2::zero;
		}
	};

	struct VRFinger
	{
		enum Type
		{
			Thumb,
			Pointer, // index
			Middle,
			Ring,
			Pinky,

			MAX,
			None = -1
		};

		Vector3 baseToTip = Vector3::zero;
		Vector3 baseToTipNormal = Vector3::zero;
		float length = 0.0f;
		float straightLength = 0.0f; // how much when compared to ref pose's length
		float straightRefAligned = 0.0f; // aligned to ref pose's dir, 1.0f fully, 0.0f half bent (not thumb!)
		/**
		 *	straightRegAligned poses:
		 *		thumb up	T >0.9 rest <0.0
		 *		any other finger straight >0.8
		 *		any other finger fold <0.0
		 *  this is transformed into actual axis
		 */

		static Type parse(String const& rsfingerName);
		static VRHandBoneIndex::Type get_tip_bone(VRFinger::Type _finger);
		static VRHandBoneIndex::Type get_base_bone(VRFinger::Type _finger);
	};

	struct VRControls
	{
		static float constexpr PINCH_TO_BUTTON_THRESHOLD = 0.7f;
		static float constexpr HAND_TRACKING_FOLDED_TO_BUTTON_THRESHOLD = 0.7f;
		static float constexpr HAND_TRACKING_STRAIGHT_TO_BUTTON_THRESHOLD = 0.3f;
		static float constexpr AXIS_TO_BUTTON_THRESHOLD = 0.5f;
		static float constexpr TRACKPAD_DIR_THRESHOLD = 0.3f;
		static float constexpr JOYSTICK_DIR_THRESHOLD = 0.5f;

		Vector2 mouseMovement = Vector2::zero;
		bool mouseButton[3];
		struct Hand
		{
			int handIndex;
			Input::DeviceID source = Input::Devices::all; // it is attached to one type of controller and that type of controller should define what vr controls are we, invalid means all
			struct Buttons
			{
				bool inUse = true; // always true, we determine if actual is in use with source
				bool usesHandTracking = false;
				bool grip = false;
				bool pose = false;
				bool upsideDown = false;
				bool insideToHead = false;
				bool headSideTouch = false;

				// in general use fingers have two states - folded / relaxed
				// for hand-tracking there are three states - folded / relaxed / straight

				// more useful for non hand-tracking (pressed/folded)
				bool thumb = false;
				bool pointer = false;
				bool middle = false;
				bool ring = false;
				bool pinky = false;

				// more useful for hand-tracking
				bool thumbFolded = false;
				bool pointerFolded = false;
				bool middleFolded = false;
				bool ringFolded = false;
				bool pinkyFolded = false;
				bool thumbStraight = false;
				bool pointerStraight = false;
				bool middleStraight = false;
				bool ringStraight = false;
				bool pinkyStraight = false;

				bool primary = false;
				bool secondary = false;
				bool systemMenu = false;
				bool systemGestureProcessing = false;
				bool dpadLeft = false;
				bool dpadRight = false;
				bool dpadDown = false;
				bool dpadUp = false;
				bool pointerPinch = false; // auto read from axis
				bool middlePinch = false; // auto read from axis
				bool ringPinch = false; // auto read from axis
				bool pinkyPinch = false; // auto read from axis
				bool trigger = false; // auto read from axis
				bool trackpad = false;
				bool trackpadTouch = false;
				bool trackpadDirCentre = false;
				bool trackpadDirLeft = false;
				bool trackpadDirRight = false;
				bool trackpadDirDown = false;
				bool trackpadDirUp = false;
				AUTO_ bool joystickLeft = false;
				AUTO_ bool joystickRight = false;
				AUTO_ bool joystickDown = false;
				AUTO_ bool joystickUp = false;
				bool joystickPress = false;
			};
			Optional<float> pointerPinch;
			Optional<float> middlePinch;
			Optional<float> ringPinch;
			Optional<float> pinkyPinch;
			Optional<float> trigger;
			Optional<float> grip;
			Optional<float> pose;
			Optional<float> upsideDown;
			Optional<float> insideToHead;
			Optional<float> headSideTouch;
			// for fingers, how much they are folded=1/relaxed/straight=0
			Optional<float> thumb;
			Optional<float> pointer;
			Optional<float> middle;
			Optional<float> ring;
			Optional<float> pinky;
			Vector2 joystick = Vector2::zero;
			Vector2 trackpad = Vector2::zero;
			Vector2 trackpadTouch = Vector2::zero; // last touched (updated only when touching)
			Vector2 trackpadDir = Vector2::zero;
			Vector2 trackpadTouchDelta = Vector2::zero; // when touching this is delta between locations
			Buttons buttons;
			Buttons prevButtons;

			void post_store_controls();
			void do_auto_buttons(); // do buttons from other input

			void reset();
			Vector2 get_dpad() const;

			Optional<float> get_button_state(VR::Input::Button::WithSource _button) const;
			float get_button_state(Array<VR::Input::Button::WithSource> const & _buttons, bool _allRequired = false) const;
			bool is_button_available(VR::Input::Button::WithSource _button) const;
			Optional<bool> is_button_pressed(VR::Input::Button::WithSource _button) const;
			bool is_button_pressed(Array<VR::Input::Button::WithSource> const & _buttons, bool _allRequired = false) const;
			Optional<bool> has_button_been_pressed(VR::Input::Button::WithSource _button) const;
			bool has_button_been_pressed(Array<VR::Input::Button::WithSource> const & _buttons, bool _allRequired = false) const;
			Optional<bool> has_button_been_released(VR::Input::Button::WithSource _button) const;
			bool has_button_been_released(Array<VR::Input::Button::WithSource> const & _buttons, bool _allRequired = false) const;

			Vector2 get_stick(VR::Input::Stick::WithSource _stick, Vector2 _deadZone = Vector2(0.05f, 0.05f)) const;
			Vector2 get_stick(Array<VR::Input::Stick::WithSource> const & _stick, Vector2 _deadZone = Vector2(0.05f, 0.05f)) const;

			Vector2 get_mouse_relative_location(VR::Input::Mouse::WithSource _mouse) const;
			Vector2 get_mouse_relative_location(Array<VR::Input::Mouse::WithSource> const & _mouses) const;
		};
		Hand hands[Hands::Count]; // stored in the same order as devices (not Hand::Right/Hand::Left)

		bool requestRecenter = false;

		VRControls();

		Vector2 get_dpad() const;

		float get_button_state(VR::Input::Button::WithSource _button) const;
		float get_button_state(Array<VR::Input::Button::WithSource> const & _buttons) const;
		bool is_button_pressed(VR::Input::Button::WithSource _button) const;
		bool is_button_pressed(Array<VR::Input::Button::WithSource> const & _buttons) const;
		bool has_button_been_pressed(VR::Input::Button::WithSource _button) const;
		bool has_button_been_pressed(Array<VR::Input::Button::WithSource> const & _buttons) const;
		bool has_button_been_released(VR::Input::Button::WithSource _button) const;
		bool has_button_been_released(Array<VR::Input::Button::WithSource> const & _buttons) const;

		Vector2 get_stick(VR::Input::Stick::WithSource _stick, Vector2 _deadZone = Vector2(0.05f, 0.05f)) const;
		Vector2 get_stick(Array<VR::Input::Stick::WithSource> const & _stick, Vector2 _deadZone = Vector2(0.05f, 0.05f)) const;

		void pre_advance();

		void post_store_controls(); // called by VRPoseSet::post_store_controls
	};

	struct VRHandPose
	{
		static int const MAX_RAW_BONES = 30; // more than ovrHandBone_Max
		static Transform get_hand_centre_offset(Hand::Type _hand);

		int rawBoneParents[MAX_RAW_BONES]; // valid only in reference

		// as read
		Optional<Transform> rawPlacementAsRead; // just pure in vr
		Optional<Transform> rawHandTrackingRootPlacementAsRead; // just pure in vr
		Optional<Transform> rawPlacement;
		Optional<Transform> rawHandTrackingRootPlacement; // this is used for hand tracking
		Optional<Transform> rawBonesLS[MAX_RAW_BONES]; // in local space (parent or placement above), indices here are for VR specific, you have to use IVR's method to translate them
		Optional<Transform> rawBonesRS[MAX_RAW_BONES]; // in root space - if provided, use this over LS

		// with offsets
		AUTO_ Optional<Transform> placement; // this is based on raw placement, unless hand tracking is in effect, then it uses a different offset and raw hand-tracking root placement
		AUTO_ ArrayStatic<Optional<Transform>, VRHandBoneIndex::MAX> bonesPS; // chosen bones in placement's space
		AUTO_ ArrayStatic<VRFinger, VRFinger::MAX> fingers;

		VRHandPose();

		void clear();

		Transform const & get_bone_ps(VRHandBoneIndex::Type _index) const;
		Transform get_raw_bone_rs(Hand::Type _hand, int _index) const; // relative to raw hand tracking root
		Transform get_raw_bone_rs(Hand::Type _hand, VRHandBoneIndex::Type _index) const;

		void calculate_auto_and_add_offsets(Hand::Type _hand, bool _setup = false); // force even if it seems we should not use it
		void store_controls(Hand::Type _hand, VRPoseSet const & _pose, VRControls::Hand& controls);
	};

	struct VRPoseSet
	{
		// view is right between the eyes

		// as read
		Optional<Transform> rawView; // without scaling
		Optional<Transform> rawViewAsRead; // just pure in vr

		// processed
		AUTO_ Optional<Transform> view;

		VRHandPose hands[Hands::Count]; // indexed by internal index

		// if one is set, both are set
		Optional<Transform> baseFull; // this is relative vertically too
		Optional<Transform> baseFlat; // this is always at floor level

		VRHandPose const& get_hand(Hand::Type _hand) const;
		Transform const& get_bone_ps(Hand::Type _hand, VRHandBoneIndex::Type _index) const { return get_hand(_hand).get_bone_ps(_index); }
		Transform get_raw_bone_rs(Hand::Type _hand, int _index) const { return get_hand(_hand).get_raw_bone_rs(_hand, _index); }
		Transform get_raw_bone_rs(Hand::Type _hand, VRHandBoneIndex::Type _index) const { return get_hand(_hand).get_raw_bone_rs(_hand, _index); }

		void calculate_raw_from_as_read();
		void calculate_auto(); // scaling plus offsets etc

		void post_store_controls(VRControls& controls);
	};

	struct Pulse
	{
		float length = 0.0f;
		Optional<float> fadeOutTime;
		float frequency = 0.0f; // 0 to 1 effective
		Optional<float> frequencyEnd;
		float strength = 0.0f; // 0 to 1 effective
		Optional<float> strengthEnd;

		float get_time_left() const { return timeLeft; }
		
		float get_current_frequency() const;
		float get_current_strength() const;

		bool load_from_xml(IO::XML::Node const * _node);
		bool load_from_xml(IO::XML::Node const * _node, tchar const * _childName);

		bool is_set() const { return length != 0.0f; }

		void make_valid();

	private:
		float timeLeft = 0.0f;

		friend class IVR;

		float get_current(float const & _what, Optional<float> const & _whatEnd) const;
	};

	struct AvailableFunctionality
	{
		bool renderToScreen = true; // if can be rendered to a screen (not only to the headset)
		bool directToVR = false;
		bool postProcess = true;
		int foveatedRenderingNominal = 0; // nominal, where main config's 1 is
		int foveatedRenderingMax = 0; // max level available
		int foveatedRenderingMaxDepth = 0;

		int convert_foveated_rendering_config_to_level(float _config);
		float convert_foveated_rendering_level_to_config(int _level);
	};

	class IVR
	{
	public:
		static void create();
		static void terminate();

		static IVR* get() { return s_current; }
		static bool can_be_used() { return get() && get()->is_ok(); }

		static void create_config_files();

	public: // public interface
		virtual bool is_ok(bool _justCheckDevice = false) const = 0;
		virtual AvailableFunctionality get_available_functionality() const { return AvailableFunctionality(); }

		virtual void get_ready_to_exit() = 0; // should show black screen
		
		virtual void handle_vr_mode_changes() = 0;

		void set_processing_levels(Optional<float> const& _cpuLevel, Optional<float> const& _gpuLevel, float _temporaryForTime);
		virtual void set_processing_levels(Optional<float> const & _cpuLevel = NP, Optional<float> const& _gpuLevel = NP, bool _temporarily = false);
		virtual void I_am_perf_thread_main() = 0;
		virtual void I_am_perf_thread_other() = 0;
		virtual void I_am_perf_thread_render() = 0;
		virtual void log_perf_threads(LogInfoContext & _log) = 0;

		Name const & get_name() const { an_assert(name.is_valid()); return name; }

		List<String> const & get_error_list() const { return errorList; }
		List<String> const & get_warning_list() const { return warningList; }
		void add_error(String const & _error) { errorList.push_back(_error); }
		void add_warning(String const & _error) { warningList.push_back(_error); }
		void clear_errors() { errorList.clear(); }
		void clear_warnings() { warningList.clear(); }

		virtual bool is_wireless() const { return false; }
		virtual bool may_have_invalid_boundary() const { return false; } // return true only if it is possible to have an invalid boundary
		virtual bool is_boundary_unavailable() const { return false; } // return true only if there's a problem with reading boundary
		bool is_invalid_boundary_replacement_available() const { return invalidBoundaryReplacementAvailable; }
		void mark_invalid_boundary_replacement_available() { invalidBoundaryReplacementAvailable = true; }

		bool may_use_render_scaling() const;

		bool should_allow_time_warp() const { return allowTimeWarp; }
		void allow_time_warp(bool _allow) { allowTimeWarp = _allow; }
		
		float get_expected_frame_time() const { an_assert(expectedFrameTime > 0.0f); return expectedFrameTime; }
		float get_ideal_expected_frame_time() const { an_assert(idealExpectedFrameTime > 0.0f); return idealExpectedFrameTime; }

		void init(::System::Video3D* _v3d);
		virtual VectorInt2 const & get_preferred_full_screen_resolution() = 0;
		virtual void force_bounds_rendering(bool _force) {}

		int get_controllers_iteration() const { return controllersIteration; }
		bool has_controllers_iteration_changed() const { return controllersIteration != prevControllersIteration; }
		void mark_new_controllers() { ++controllersIteration; }
		virtual tchar const* get_prompt_controller_suffix(Input::DeviceID _source) const = 0;

		Hand::Type get_dominant_hand() const { return dominantHandOverride.get(dominantHand); }
		void set_dominant_hand(Hand::Type _hand) { dominantHand = _hand; }
		void set_dominant_hand_override(Optional<Hand::Type> _hand) { dominantHandOverride = _hand; }

		VRControls const & get_controls() const { return controls; }
		VRControls& access_controls() { return controls; }
		int get_right_hand() const { return rightHand; }
		int get_left_hand() const { return leftHand; }
		int get_hand(Hand::Type _hand) const { return _hand == Hand::Left? get_left_hand() : get_right_hand(); }
		Hand::Type get_hand_type(int _handIdx) const { return _handIdx == get_left_hand() ? Hand::Left : (_handIdx == get_right_hand()? Hand::Right : Hand::MAX); }
		bool is_right_hand_available() const { return rightHandAvailable; }
		bool is_left_hand_available() const { return leftHandAvailable; }
		bool is_hand_available(Hand::Type _hand) const { return _hand == Hand::Left? is_left_hand_available() : is_right_hand_available(); }
		bool is_right_hand_read() const { return controlsPoseSet.hands[Hand::Right].rawPlacementAsRead.is_set(); }
		bool is_left_hand_read() const { return controlsPoseSet.hands[Hand::Left].rawPlacementAsRead.is_set(); }
		bool is_hand_read(Hand::Type _hand) const { return _hand == Hand::Left? is_left_hand_read() : is_right_hand_read(); }
		void store_hands_available(bool _leftHandAvailable, bool _rightHandAvailable) { leftHandAvailable = _leftHandAvailable; rightHandAvailable = _rightHandAvailable; }
		virtual void update_hands_available() = 0;

		VRPoseSet const & get_reference_pose_set() const { return referencePoseSet; }
		VRPoseSet& access_reference_pose_set() { return referencePoseSet; }
		VRPoseSet const & get_prev_controls_pose_set() const { return prevControlsPoseSet; }
		void store_prev_controls_pose_set(); // from controls
		VRPoseSet const & get_controls_pose_set() const { return controlsPoseSet; }
		VRPoseSet& access_controls_pose_set() { return controlsPoseSet; }
		VRPoseSet const& get_prev_render_pose_set() const { return prevRenderPoseSet; }
		void store_prev_render_pose_set(); // from controls
		VRPoseSet const & get_render_pose_set() const { return renderPoseSet; }
		VRPoseSet& access_render_pose_set() { return renderPoseSet; }
		VRPoseSet const & get_last_valid_render_pose_set() const { return lastValidRenderPoseSet; }
		void store_last_valid_render_pose_set();

		// render targets
		void create_render_targets_for_output(::System::RenderTargetSetup const & _source, VectorInt2 const & _fallbackEyeResolution);
		VectorInt2 get_render_size(Eye::Type _eye);
		VectorInt2 get_render_full_size_for_aspect_ratio_coef(Eye::Type _eye);
		float get_render_aspect_ratio_coef(Eye::Type _eye);
		::System::RenderTarget* get_render_render_target(Eye::Type _eye) { return useDirectToVRRendering ? get_direct_to_vr_render_target(_eye) : ((int)_eye < Eye::Count ? eyeRenderRenderTargets[_eye].get() : nullptr); }
		::System::RenderTarget* get_output_render_target(Eye::Type _eye) { return useDirectToVRRendering ? get_direct_to_vr_render_target(_eye) : ((int)_eye < Eye::Count ? eyeOutputRenderTargets[_eye].get() : nullptr); }
		bool does_use_direct_to_vr_rendering() const { return useDirectToVRRendering; }
		bool does_use_separate_output_render_target() const { return ! useDirectToVRRendering && ! noOutputRenderTarget; }

		void update_foveated_rendering_for_render_targets();
		void update_foveated_rendering_for_render_target(::System::RenderTarget* _rt, Eye::Type _eye);

		/*
		 *	running
		 *		1. call advance on main frame, synced, call it as close to starting separate threads as possible
		 *		2. call end_render (early) if you want to give more time for the rendering to be processed (if we're blocked at end_render)
		 *		3. call game_sync from game thread + call begin_render from render thread, they sync relatively to each other
		 *		   they both require advance to be called first though
		 *		4. call end_render (late) if you want to wait with presentation of the frame (unless end_render is not blocking)
		 *	it is possible to call both end_renders as they are checked if begin_render has been called
		 */

		virtual void advance(); // synced, advance frame, etc (vr system should be called first, then this, check vr system)
		virtual void game_sync(); // synchronise game to vr (may be not supported), should be called before begin_frame
		bool begin_render(System::Video3D* _v3d); // begin render should be called post game_sync
		void end_render(System::Video3D* _v3d);
		bool is_render_open() const { return renderIsOpen; }

		// presenting
		void copy_render_to_output(System::Video3D* _v3d); // if not done by other means

		void update_input_tags();

	public:
		bool is_game_sync_blocking() const { return setupImpl.gameSyncBlocking; }
		bool does_begin_render_require_game_sync() const { return setupImpl.beginRenderRequiresGameSync; }
		bool is_end_render_blocking() const { return setupImpl.endRenderBlocking; }

	protected: // setup
		void setup__set_game_sync_blocking(bool _gameSyncBlocking) { setupImpl.gameSyncBlocking = _gameSyncBlocking; }
		void setup__set_begin_render_requires_game_sync(bool _beginRenderRequiresGameSync) { setupImpl.beginRenderRequiresGameSync = _beginRenderRequiresGameSync; }
		void setup__set_end_render_blocking(bool _endRenderBlocking) { setupImpl.endRenderBlocking = _endRenderBlocking; }

	protected:
		virtual bool begin_render_internal(System::Video3D* _v3d) = 0; // does not have to check whether was opened, is called only if not opened, returns true if opened
		virtual bool end_render_internal(System::Video3D* _v3d) = 0; // does not have to check whether was opened, is called only if opened, returns true if closed

	public:
		// controls
		Transform const& get_movement_centre_in_view_space() const { return movementCentreInViewSpace; }
		void set_movement_centre_in_view_space(Transform const& _movementCentreInViewSpace) { movementCentreInViewSpace = _movementCentreInViewSpace; }

		Transform get_controls_movement_centre() const { an_assert(controlsPoseSet.view.is_set()); return controlsPoseSet.view.get().to_world(movementCentreInViewSpace); }
		bool is_controls_view_valid() const { return controlsPoseSet.view.is_set(); }
		bool is_prev_controls_view_valid() const { return prevControlsPoseSet.view.is_set(); }
		bool is_controls_base_valid() const { return controlsPoseSet.baseFull.is_set(); }
		bool is_prev_controls_base_valid() const { return prevControlsPoseSet.baseFull.is_set(); }
		Transform const & get_controls_view() const { an_assert(controlsPoseSet.view.is_set()); return controlsPoseSet.view.get(); }
		Transform const & get_controls_raw_view() const { an_assert(controlsPoseSet.rawView.is_set() || controlsPoseSet.view.is_set()); return controlsPoseSet.rawView.is_set()? controlsPoseSet.rawView.get() : controlsPoseSet.view.get(); }
		Transform const & get_controls_base_full() const { an_assert(controlsPoseSet.baseFull.is_set()); return controlsPoseSet.baseFull.get(); }
		Transform const & get_controls_base_flat() const { an_assert(controlsPoseSet.baseFlat.is_set()); return controlsPoseSet.baseFlat.get(); }
		Transform get_controls_view_to_base_full() { an_assert(controlsPoseSet.view.is_set() && controlsPoseSet.baseFull.is_set()); return controlsPoseSet.baseFull.get().to_local(controlsPoseSet.view.get()); }
		Transform get_controls_view_to_base_flat() { an_assert(controlsPoseSet.view.is_set() && controlsPoseSet.baseFlat.is_set()); return controlsPoseSet.baseFlat.get().to_local(controlsPoseSet.view.get()); }
		void set_controls_base_pose_from_view();

#ifdef AN_DEVELOPMENT_OR_PROFILER
		void set_vertical_offset(float _verticalOffset) { verticalOffset = _verticalOffset; }
		float get_vertical_offset() const { return verticalOffset; }
#endif

		void decide_hands();

		bool is_using_hand_tracking(Optional<Hand::Type> const & _hand = NP) const;
		bool set_using_hand_tracking(int _handIdx, bool _usingHandTracking); // returns true if changed

		// turn counter
		void advance_turns();
		int get_turn_count() const { return turnCount.count; }
		void reset_turn_count() { turnCount = TurnCount(); }

		// pulses/force feedback
		void trigger_pulse(int _handIndex, float _length, float _frequency, Optional<float> const & _frequencyEnd, float _strength, Optional<float> const& _strengthEnd, Optional<float> _fadeOutTime = NP);
		void trigger_pulse(int _handIndex, Pulse const & _pulse);
		Pulse const * get_pulse(int _handIndex) const { return _handIndex < Hands::Count? &pulses[_handIndex] : nullptr; }

		// mesh
		void set_model_name_for_hand(int _handIndex, Name const & _name) { modelForHand[_handIndex] = _name; }
		Name const & get_model_name_for_hand(int _handIndex) const { return modelForHand[_handIndex]; }
		virtual Meshes::Mesh3D* load_mesh(Name const & _modelName) const { return nullptr; }
		virtual int translate_bone_index(VRHandBoneIndex::Type _index) const { return NONE; } // part of hand-tracking

		// rendering
		bool is_render_view_valid() const { return renderPoseSet.view.is_set(); }
		bool is_render_base_valid() const { return is_controls_base_valid(); }
		bool is_prev_render_view_valid() const { return prevRenderPoseSet.view.is_set(); }
		bool is_prev_render_base_valid() const { return is_prev_controls_base_valid(); }
		Transform const & get_render_view() const { an_assert(renderPoseSet.view.is_set()); return renderPoseSet.view.get(); }
		Transform const & get_prev_render_view() const { an_assert(prevRenderPoseSet.view.is_set()); return prevRenderPoseSet.view.get(); }
		Transform const & get_render_base_full() const { return get_controls_base_full(); }
		Transform const & get_render_base_flat () const { return get_controls_base_flat(); }
		Transform get_render_view_to_base_full() { an_assert(renderPoseSet.view.is_set() && controlsPoseSet.baseFull.is_set()); return controlsPoseSet.baseFull.get().to_local(renderPoseSet.view.get()); }
		Transform get_render_view_to_base_flat() { an_assert(renderPoseSet.view.is_set() && controlsPoseSet.baseFlat.is_set()); return controlsPoseSet.baseFlat.get().to_local(renderPoseSet.view.get()); }
		Transform get_prev_render_view_to_base_full() { an_assert(prevRenderPoseSet.view.is_set() && prevControlsPoseSet.baseFull.is_set()); return prevControlsPoseSet.baseFull.get().to_local(prevRenderPoseSet.view.get()); }
		Transform get_prev_render_view_to_base_flat() { an_assert(prevRenderPoseSet.view.is_set() && prevControlsPoseSet.baseFlat.is_set()); return prevControlsPoseSet.baseFlat.get().to_local(prevRenderPoseSet.view.get()); }
		virtual void set_render_mode(RenderMode::Type _mode) { renderMode = _mode; }
		RenderMode::Type get_render_mode() const { return renderMode; }
		virtual RenderInfo const & get_render_info() const = 0;
		float get_render_scaling() const { return renderScaling; }

		void use_fallback_rendering(bool _useFallbackRendering = true) { useFallbackRendering = _useFallbackRendering;}
		void render_one_output_to(System::Video3D* _v3d, Eye::Type _eye, ::System::RenderTarget* _rt, Optional<RangeInt2> const & _at = NP);
		void render_one_output_to_main(System::Video3D * _v3d, Eye::Type _eye);

		// boundary
		void set_raw_boundary(Array<Vector2> const& _rawBoundaryPoints) { rawBoundaryPoints = _rawBoundaryPoints; }
		Array<Vector2> const& get_raw_boundary() const { return rawBoundaryPoints; }

		// will use raw boundary points if may have invalid
		void act_on_possibly_invalid_boundary();

		// vr scaling
		Scaling const& get_scaling() const { return scaling; }
		void set_scaling(Scaling const& _scaling) { scaling = _scaling; }
		void update_scaling(); // will take from main config, uses set_scaling

		// play area
		void output_play_area();
		virtual bool load_play_area_rect(bool _loadAnything = false) = 0;
		virtual bool has_loaded_play_area_rect() const = 0;
		void set_play_area_rect(Vector3 const & _halfForward, Vector3 const & _halfRight);
		void update_play_area_rect(); // using vr map from main config, include horizontalScaling as well
		Vector3 const & get_play_area_rect_half_forward() const { return playAreaRectHalfForward; }
		Vector3 const & get_play_area_rect_half_right() const { return playAreaRectHalfRight; }
		Zone const & get_play_area_zone() const { return playAreaZone; }
		
		Vector3 const & get_whole_play_area_rect_half_forward() const { return wholePlayAreaRectHalfForward; }
		Vector3 const & get_whole_play_area_rect_half_right() const { return wholePlayAreaRectHalfRight; }

		Vector3 const & get_raw_whole_play_area_rect_half_forward() const { return rawWholePlayAreaRectHalfForward; }
		Vector3 const & get_raw_whole_play_area_rect_half_right() const { return rawWholePlayAreaRectHalfRight; }

		Transform const & get_map_vr_space() const { return mapVRSpace; }
		Transform const & get_map_vr_space_auto() const { return mapVRSpaceAuto; }
		Transform const& get_map_vr_space_manual() const { return mapVRSpaceManual; }
		Transform calculate_map_vr_space(Vector3 const & _manualOffset, float _manualRotate) const;
		void set_map_vr_space_origin(Transform const& _origin);
		void rotate_map_vr_space_auto();
		void update_map_vr_space();

		Transform const & get_additional_offset_to_raw_as_read() const { return additionalOffsetToRawAsRead; }
		void offset_additional_offset_to_raw_as_read(Transform const& _newRelativeOrigin);

		Transform const & get_immobile_origin_in_vr_space() const { return MainConfig::global().should_be_immobile_vr()? immobileOriginInVRSpace : Transform::identity; }
		void reset_immobile_origin_in_vr_space();
		void reset_immobile_origin_in_vr_space_if_pending();
		void mark_pending_reset_immobile_origin_in_vr_space() { pendingResetImmobileOriginInVRSpace = true; }

#ifdef AN_DEVELOPMENT_OR_PROFILER
		Transform const& get_dev_in_vr_space() const;
		void move_relative_dev_in_vr_space(Vector3 const & _moveBy, Rotator3 const & _rotateBy);
#endif

#ifdef AN_DEVELOPMENT
		void debug_render_whole_play_area_rect(Optional<Colour> const & _colour = NP, Transform const & _placement = Transform::identity, bool _drawOffset = false);
		void debug_render_play_area_rect(Optional<Colour> const & _colour = NP, Transform const & _placement = Transform::identity);
#endif

		virtual void draw_debug_controls(int _hand, Vector2 const & _at, float _width, ::System::Font* _font) {}

	protected:
		// check notes on expected and ideal frame times below
		virtual float update_expected_frame_time() const = 0;
		virtual float calculate_ideal_expected_frame_time() const = 0;

	public:
		void pre_advance_controls();

	protected:
		static IVR* s_current;

		IVR(Name const & _name);
		virtual ~IVR();

		Name name;

		List<String> errorList;
		List<String> warningList;

		bool useFallbackRendering = false;

		RenderMode::Type renderMode = RenderMode::Normal;

		bool renderIsOpen;
		bool allowTimeWarp = true;
		Optional<float> resetProcessingLevelsTimeLeft;

		struct SetupImpl
		{
			bool endRenderBlocking = true;
			bool beginRenderRequiresGameSync = true;
			bool gameSyncBlocking = false;
		} setupImpl;

		float expectedFrameTime = 0.0f; // this is the actual frame time, it might be slower, it is what VR device reports. ALTHOUGH it might be the ideal frame time and the System::Core handles cases when we slow down (check USE_OTHER_DELTA_TIMES)
		float idealExpectedFrameTime = 0.0f; // this is the frame time when everything works smoothly

		Transform movementCentreInViewSpace = Transform(Vector3(0.0f, -0.09f, -0.09f), Quat::identity); // this is view position relative to where movement centre is (ie. neck)
		VRPoseSet referencePoseSet; // to hold skeleton etc
		VRPoseSet prevControlsPoseSet;
		VRPoseSet controlsPoseSet;
		VRPoseSet prevRenderPoseSet;
		VRPoseSet renderPoseSet;
		VRPoseSet lastValidRenderPoseSet;
		VRControls controls;
		int controllersIteration = 0; // how many times controllers have changed
		int prevControllersIteration = NONE;

#ifdef AN_DEVELOPMENT_OR_PROFILER
		float verticalOffset = 0.0f; // this is mostly for debug purposes, to allow crouching
#endif

		Hand::Type dominantHand = Hand::Right; // if a system has more info on that, should update
		Optional<Hand::Type> dominantHandOverride; // overriden by player preferences

		int leftHand = NONE; // index in hands table
		int rightHand = NONE; // index in hands table
		bool leftHandAvailable = false;
		bool rightHandAvailable = false;
		bool handsRequireDecision = true;
		float handsRequireDecisionTimeLeft = 0.0f;
#ifdef AN_DEVELOPMENT_OR_PROFILER
		bool handsRequireSettingInputTags = false;
#endif
		bool usingHandTracking[Hands::Count] = { false, false };

		Name modelForHand[Hands::Count];

		// turns

		struct TurnCount
		{
			int count = 0;
			float deg = 0.0f; // if goes beyond 180, modifies turnCount
			Optional<float> lastViewYawVR; // only if pitch is within certain range
		};
		TurnCount turnCount;

		//

		Pulse pulses[Hands::Count];

		//

		Scaling scaling;

		//

		Array<Vector2> rawBoundaryPoints;
		bool invalidBoundaryReplacementAvailable = false;

		// default 2.0 x 1.8
		Transform mapVRSpace = Transform::identity; // this is global remapping - rotating/translating - means where in vr system's space is our vr space
		Transform mapVRSpaceOrigin = Transform::identity; // the origin - in vr space
		Transform mapVRSpaceAuto = Transform::identity; // it is useful to get x to be bigger than y - just rotates
		Transform mapVRSpaceManual = Transform::identity; // useful to move around play area within space - it is centre of play area inside auto-rotated
		Transform immobileOriginInVRSpace = Transform::identity; // for sitting origin in vr space (not mapped vr space!)
		bool pendingResetImmobileOriginInVRSpace = false;
		Transform additionalOffsetToRawAsRead = Transform::identity; // this is used to offset play area
#ifdef AN_DEVELOPMENT_OR_PROFILER
		Transform devInVRSpace = Transform::identity; // placement of dev in vr space, we take pure vr placement and add it in this
#endif
		// set read from vr
		Vector3 rawWholePlayAreaRectHalfForward = Vector3(0.0f, 0.9f, 0.0f);
		Vector3 rawWholePlayAreaRectHalfRight = Vector3(1.0f, 0.0f, 0.0f);

		// altered by vr manual size from main config
		Vector3 wholePlayAreaRectHalfForward = Vector3(0.0f, 0.9f, 0.0f);
		Vector3 wholePlayAreaRectHalfRight = Vector3(1.0f, 0.0f, 0.0f);

		// as used by other systems
		Vector3 playAreaRectHalfForward = Vector3(0.0f, 0.9f, 0.0f);
		Vector3 playAreaRectHalfRight = Vector3(1.0f, 0.0f, 0.0f);
		Zone playAreaZone;

		//

		float renderScaling = 1.0f;
		bool useDirectToVRRendering = false; // if set, won't use render/output this way, won't copy to screen, etc. if in use, has to provide direct_to_vr_render_target
		bool noOutputRenderTarget = false; // if set, won't use output, will only use render render target (output render target will be set to render render target)
		::System::RenderTargetPtr eyeRenderRenderTargets[Eye::Count]; // to render
		::System::RenderTargetPtr eyeOutputRenderTargets[Eye::Count]; // to output to hardware
		
		//

		void fallback_rendering(System::Video3D * _v3d);

		//

		virtual void enter_vr_mode() = 0;
		virtual void init_video(::System::Video3D* _v3d) = 0;
		virtual void create_internal_render_targets(::System::RenderTargetSetup const & _source, VectorInt2 const & _fallbackEyeResolution, OUT_ bool & _useDirectToVRRendering, OUT_ bool& _noOutputRenderTarget) = 0;
		virtual VectorInt2 get_direct_to_vr_render_size(int _eyeIdx) = 0;
		virtual float get_direct_to_vr_render_aspect_ratio_coef(int _eyeIdx) = 0;
		virtual ::System::RenderTarget* get_direct_to_vr_render_target(int _eyeIdx) = 0;
		virtual void on_render_targets_created() = 0;

		//

		virtual DecideHandsResult::Type decide_hands_by_system(OUT_ int & _leftHand, OUT_ int & _rightHand) { return DecideHandsResult::CantTellAtAll; }
		virtual void update_on_decided_hands(int _leftHand, int _rightHand) {}

		virtual void update_hands_controls_source() {}

		//

		virtual void update_render_scaling(REF_ float & _scale) = 0;
		void use_render_target_scaling(float _scale = 1.0f);

		//

		Transform internal_calculate_map_vr_space(Vector3 const& _manualOffset, float _manualRotate, OUT_ Transform & _mapVRSpaceManual) const;

	};
};
