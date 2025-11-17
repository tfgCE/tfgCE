#include "vrSystemImpl.h"

#include "vrSystem.h"
#include "iVR.h"

#include "vrFovPort.h"

#include "..\mainConfig.h"

#include "..\math\math.h"
#include "..\mesh\builders\mesh3dBuilder_IPU.h"
#include "..\mesh\mesh3d.h"
#include "..\system\core.h"
#include "..\system\video\font.h"
#include "..\system\video\video3d.h"
#include "..\system\video\indexFormat.h"
#include "..\system\video\renderTarget.h"
#include "..\system\video\video3dPrimitives.h"

#include "..\containers\arrayStack.h"

#include "..\performance\performanceUtils.h"

//

using namespace VR;

//

#ifdef AN_DEVELOPMENT
#define DEBUG_DECIDE_HANDS
#endif

//

VRSystemImpl::VRSystemImpl(VRSystem* _owner)
: owner(_owner)
, preferredFullScreenResolution(VectorInt2::zero)
{
	for_count(int, i, Hands::Count)
	{
		handControllerTrackDevices[i].controllerType = NONE;
		handControllerTrackDevices[i].deviceID = NONE;
	}
}

VRSystemImpl::~VRSystemImpl()
{
	an_assert(!closeDeviceRequired, TXT("close device?"));
}

void VRSystemImpl::init()
{
	init_impl();

	create_device();
}

void VRSystemImpl::shutdown()
{
	close_device();

	shutdown_impl();
}

void VRSystemImpl::create_device()
{
	closeDeviceRequired = true;

	create_device_impl();

	output(TXT("check load play area rect"));
	loadedPlayAreaRect = false;
	if (load_play_area_rect())
	{
		// ok
	}
	else
	{
		output(TXT("could not load play area rect during create devices"));
		// we will try later until we get something
	}

	output(TXT("update tracking devices"));
	update_track_device_indices();

	// nothing to open?
}

void VRSystemImpl::enter_vr_mode()
{
	if (!is_ok())
	{
		return;
	}

	enter_vr_mode_impl();
}

void VRSystemImpl::init_video(::System::Video3D* _v3d)
{
	if (! is_ok())
	{
		return;
	}

	// update render info with default values
	setup_rendering_on_init_video(_v3d);
}

void VRSystemImpl::close_device()
{
	close_device_impl();
	closeDeviceRequired = false;
	loadedPlayAreaRect = false;
}

void VRSystemImpl::advance()
{
	an_assert(is_ok(), TXT("should be working"));

	{
		MEASURE_PERFORMANCE(owner_pre_advance_controls);
		owner->pre_advance_controls();
	}

	{
		MEASURE_PERFORMANCE(advance_events);
		advance_events();
	}

	{
		MEASURE_PERFORMANCE(advance_pulses);
		advance_pulses();
	}

	todo_note(TXT("standing or not?"));
	float expectedFrameTime = owner->get_expected_frame_time();
	System::Core::store_vr_delta_time(expectedFrameTime);

	{
		MEASURE_PERFORMANCE(owner_store_prev_controls_pose_set);
		owner->store_prev_controls_pose_set();
	}

	{
		MEASURE_PERFORMANCE(advance_poses);
		advance_poses();
	}

	// at this moment we have latest control pose read, reset should bring render pose to control pose
	// this is important to call post advance_poses as control pose is already in new coordinate system
	owner->reset_immobile_origin_in_vr_space_if_pending();

	if (!owner->is_controls_base_valid())
	{
		owner->set_controls_base_pose_from_view();
	}

	{
		MEASURE_PERFORMANCE(owner_advance_turns);
		owner->advance_turns();
	}
}

void VRSystemImpl::game_sync()
{
	game_sync_impl();
}

int VRSystemImpl::find_hand_by_tracked_device_index(int _trackedDeviceIndex) const
{
	for_count(int, i, Hands::Count)
	{
		if (handControllerTrackDevices[i].deviceID == _trackedDeviceIndex)
		{
			return i;
		}
	}
	return NONE;
}

void VRSystemImpl::force_bounds_rendering(bool _force)
{
	warn(TXT("enabling/disabling bounds rendering not supported yet"));
	/* this code does something else, it's just hint for bounds rendering colour
	vr::HmdColor_t colour;
	colour.r = 1.0f;
	colour.g = 1.0f;
	colour.b = 1.0f;
	colour.a = 1.0f;
	if (!_enable)
	{
		colour.a = 0.0f;
	}
	openvrChaperone->SetSceneColor(colour);
	*/
}

bool VRSystemImpl::load_play_area_rect(bool _loadAnything)
{
	scoped_call_stack_info(TXT("VRSystemImpl::load_play_area_rect"));
	if (!_loadAnything && ! can_load_play_area_rect_impl())
	{
		boundaryUnavailable = true;
		// try again later!
		return false;
	}
	boundaryUnavailable = false; // assume it is available, actual load may prove us wrong
	for(int i = 0; i < 2; ++ i)
	{
		if (load_play_area_rect_impl(_loadAnything))
		{
			output(TXT("loaded play rect area"));
			loadedPlayAreaRect = true;
			return true;
		}
		output(TXT("rotate and check again"));
		owner->rotate_map_vr_space_auto();
	}

	output(TXT("sort of loaded play rect area"));
	loadedPlayAreaRect = true;
	return true;
}

void VRSystemImpl::update_hands_available()
{
	owner->store_hands_available(owner->get_left_hand() == NONE? false : handControllerTrackDevices[owner->get_left_hand()].deviceID != NONE,
								 owner->get_right_hand() == NONE? false : handControllerTrackDevices[owner->get_right_hand()].deviceID != NONE);
}

void VRSystemImpl::update_on_decided_hands(int _leftHand, int _rightHand)
{
	handControllerTrackDevices[_leftHand].hand = Hand::Left;
	handControllerTrackDevices[_rightHand].hand = Hand::Right;
}

VectorInt2 VRSystemImpl::get_direct_to_vr_render_size(int _eyeIdx)
{
	if (auto* rt = get_direct_to_vr_render_target(_eyeIdx))
	{
		return rt->get_size();
	}
	return VectorInt2::zero;
}

float VRSystemImpl::get_direct_to_vr_render_aspect_ratio_coef(int _eyeIdx)
{
	if (auto* rt = get_direct_to_vr_render_target(_eyeIdx))
	{
		return rt->get_setup().get_aspect_ratio_coef();
	}
	return 1.0f;
}

int VRSystemImpl::translate_bone_index(VRHandBoneIndex::Type _index) const
{
	todo_implement(TXT("imlement it to use for hand-tracking"));
	return NONE;
}
