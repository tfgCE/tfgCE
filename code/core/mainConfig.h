#pragma once

#include "enums.h"
#include "io\memoryStorage.h"
#include "types\option.h"
#include "system\video\videoConfig.h"
#include "system\sound\soundConfig.h"
#include "types\hand.h"
#include "vr\vrMode.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

struct MainConfigSaveContext
{
	bool userConfigFile = true;
};

/**
 *	Main Config
 *
 *	Game config. engine/game settings. Should be not modified by user/player.
 *
 */
struct MainConfig
{
	struct VROffset
	{
		Rotator3 orientation = Rotator3::zero;

		VROffset() {}
		VROffset(Rotator3 const& _o) : orientation(_o) {}
		Transform to_transform() const;
	};

	static void initialise_static();
	static void reset_static();
	static void close_static();

	static MainConfig clean() { return MainConfig(); }
	static MainConfig const & global() { return s_global; }
	static MainConfig & access_global() { return s_global; }
	static void store_defaults() { s_defaults = s_global; }
	static MainConfig const& defaults() { return s_defaults; }

	MainConfig();

	bool load_from_xml(IO::XML::Node const * _node);
	void save_to_xml(IO::XML::Node * _containerNode, bool _userConfig = false) const;
	// user vr config is related to user's preferences, shape and size (scaling, haptic feedback etc)
	// mapping, buffer etc is stored in main config as it relates to the play area
	bool load_user_vr_from_xml(IO::XML::Node const * _node);
	void save_user_vr_to_xml(IO::XML::Node * _containerNode) const;
	bool check_system_tag_required(IO::XML::Node const* _node) const; // returns true if should proceed, can be used by other configs too

	static bool load_to_be_copied_to_main_config_xml(IO::XML::Node const* _node);
	static void save_to_be_copied_to_main_config_xml(IO::XML::Node* _containerNode);

	void force_system(String const & _forcedSystemName) { forcedSystemName = _forcedSystemName; }
	String const& get_forced_system() const { return forcedSystemName; }

	bool should_allow_vr() const { return allowVR && !vrNotNeededAtAll; }
	bool should_force_vr() const { return ! forceVR.is_empty() && !vrNotNeededAtAll; }
	String const & get_forced_vr() const { return forceVR; }
	Name const& get_pretend_vr_input_system() const { return pretendVRInputSystem; }
	bool should_be_immobile_vr() const { return immobileVRNow/* && !vrNotNeededAtAll*/; }
	bool should_be_immobile_vr_auto() const { return immobileVRAuto; }
	VRMode::Type get_vr_mode() const { return vrMode; }
	::System::VideoConfig const & get_video() const { return video; }
	::System::SoundConfig const & get_sound() const { return sound; }

	void set_thread_setup_from(MainConfig const& _config);
	void set_thread_limit(int _threadLimit) { threadLimit = _threadLimit; }
	int get_thread_limit() const { return min(THREAD_LIMIT, threadLimit); }
	int get_thread_number_forced() const;
	bool does_allow_extra_threads() const { return allowExtraThreads; }
	int get_system_reserved_threads() const { return systemReservedThreads; } // NONE if auto
	bool should_threads_use_affinity_mask() const { return threadsUseAffinityMask; }
	ThreadPriority::Type get_main_thread_priority() const { return mainThreadPriority; }
	ThreadPriority::Type get_other_threads_priority() const { return otherThreadsPriority; }

	float get_time_reserved_for_submitting_render_frame() const { return timeReservedForSubmittingRenderFrame; }
	bool is_option_set(Name const & _option) const;

	Option::Type get_report_bugs() const { return reportBugs; }

	Name const& get_physical_sensations_system() const { return physicalSensationsSystem; }

	String const& get_owo_address() const { return owoAddress; }
	int const& get_owo_port() const { return owoPort; }

	String const& get_bhaptics_setup() const { return bhapticsSetup; }

	//

	void vr_not_needed_at_all() { vrNotNeededAtAll = true; }

	//

	::System::VideoConfig& access_video() { return video; }
	::System::SoundConfig& access_sound() { return sound; }

	void allow_vr(bool _allowVR) { allowVR = _allowVR; }
	void force_vr(String const & _vr) { forceVR = _vr; keepForceVRAsIs = true; }
	void force_vr() { force_vr(String(TXT("any"))); }
	Option::Type get_immobile_vr() const { return immobileVR; }
	bool get_immobile_vr_auto() const { return immobileVRAuto; }
	void set_immobile_vr(Option::Type _immobileVR);
	void set_immobile_vr_auto(bool _immobileVR, bool _force = false); // force means that we will switch to auto
	
	void validate_immobile_vr();
	void set_immobile_vr_size(Optional<Vector2> const& _immobileVRSize = NP);
	Vector2 get_immobile_vr_size() const { return immobileVRSize; }

	VROffset const & get_vr_offset(Hand::Type _hand) const;
	void set_vr_offsets(VROffset const& _leftHandVROffset, VROffset const& _rightHandVROffset);

	float get_vr_haptic_feedback() const { return vrHapticFeedback; }
	void set_vr_haptic_feedback(float _vrHapticFeedback) { vrHapticFeedback = _vrHapticFeedback; }

	Vector2 const & get_vr_map_margin() const { return vrMap.margin; }
	float get_vr_horizontal_scaling() const { return max(vrHorizontalScaling, vrHorizontalScalingAtLeast); }
	float get_vr_horizontal_scaling_min_value() const { return vrHorizontalScalingAtLeast; }
	float get_vr_scaling() const { return vrScaling; }
	float get_vr_map_space_rotate() const { return vrMap.yaw; }
	Optional<Vector2> const& get_vr_map_space_size() const { return vrMap.size; }
	Vector3 const& get_vr_map_space_offset() const { return vrMap.offset; }
	// this will update IVR but not other things dependent on it (RoomGenerationInfo etc).
	void set_vr_map_margin(Vector2 const & _margin);
	void set_vr_horizontal_scaling(float _vrHorizontalScaling);
	void set_vr_horizontal_scaling_min_value(float _vrHorizontalScalingAtLeast);
	void set_vr_scaling(float _vrScaling);
	void set_vr_map_space_rotate(float _yaw);
	void set_vr_map_space_size(Optional<Vector2> const& _size);
	void set_vr_map_space_offset(Vector3 const& _offset);

	void set_report_bugs(Option::Type _reportBugs);

	void set_physical_sensations_system(Name const& _pss); // only stores info, has to be set separately
	void set_owo_address(String const& _address);
	void set_owo_port(int _port);
	void set_bhaptics_setup(String const& _bhapticsSetup);

	void set_joystick_input_blocked(bool _blockJoystickInput) { blockJoystickInput = _blockJoystickInput; }
	bool is_joystick_input_blocked() const { return blockJoystickInput; }
	
	void set_joystick_input_swapped(bool _swapJoystickInput) { swapJoystickInput = _swapJoystickInput; }
	bool is_joystick_input_swapped() const { return swapJoystickInput; }

private:
	static MainConfig s_global;
	static MainConfig s_defaults;

	String forcedSystemName; // this is only a config, 

	bool vrNotNeededAtAll = false; // this is only as a runtime setting

	::System::VideoConfig video;
	::System::SoundConfig sound;

	float vrHapticFeedback = 1.0f;

	bool allowVR;
	String forceVR;
	Name pretendVRInputSystem;
	bool keepForceVRAsIs = false;
	// immobile vr is sitting/standing/non roomscale
	Option::Type immobileVR = Option::False; // requested by user, true, false, auto
	bool immobileVRAuto = false; // as auto would want it to be
	bool immobileVRNow = false; // combined above
	VRMode::Type vrMode = VRMode::Default;
	/**
	 *	Order of applying
	 *		actual play area size
	 *		(here also comes offset but not for the size, for placement)
	 *		subtract border (to have real world border size)
	 *		scale it horizontally and in general
	 */
	struct VRMap
	{
		Vector2 margin = Vector2::zero; // positive makes play area smaller
		float yaw = 0.0f;
		Vector3 offset = Vector3::zero;
		Optional<Vector2> size;
	} vrMap;
	float vrHorizontalScaling = 1.0f;
	float vrHorizontalScalingAtLeast = 1.0f;
	float vrScaling = 1.0f;
	Vector2 immobileVRSize = Vector2::zero;
	VROffset vrOffsets[Hand::MAX];

	bool blockJoystickInput = false; // in case of a drift
	bool swapJoystickInput = false; // left and right

	bool threadsUseAffinityMask = true;
	ThreadPriority::Type mainThreadPriority = ThreadPriority::Higher;
	ThreadPriority::Type otherThreadsPriority = ThreadPriority::Normal;
	int threadLimit = INF_INT;
	bool allowExtraThreads = false; // if false, uses same threads and executors
	int systemReservedThreads = -1; // NONE if auto
	String systemReservedThreadsString;
	int timeReservedForSubmittingRenderFrame_us = 0; // us - microseconds, if not reserved (ie. zero) doesn't use this
	AUTO_ float timeReservedForSubmittingRenderFrame;

	Option::Type reportBugs = Option::True; // Option::Unknown = we should ask

	Name physicalSensationsSystem;

	String owoAddress;
	int owoPort;

	String bhapticsSetup;

	void update_auto();
	void update_vr();
};
