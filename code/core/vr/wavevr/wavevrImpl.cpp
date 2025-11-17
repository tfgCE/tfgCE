#include "wavevrImpl.h"

#ifdef AN_WITH_WAVEVR
#include "..\vrFovPort.h"

#include "..\..\mainConfig.h"

#include "..\..\io\dir.h"
#include "..\..\math\math.h"
#include "..\..\mesh\builders\mesh3dBuilder_IPU.h"
#include "..\..\mesh\mesh3d.h"
#include "..\..\splash\splashLogos.h"
#include "..\..\system\core.h"
#include "..\..\system\video\font.h"
#include "..\..\system\video\video3d.h"
#include "..\..\system\video\indexFormat.h"
#include "..\..\system\video\renderTarget.h"
#include "..\..\system\video\renderTargetUtils.h"
#include "..\..\system\video\video3dPrimitives.h"

#include "..\..\containers\arrayStack.h"

#include "..\..\performance\performanceUtils.h"

#include "..\..\system\android\androidApp.h"

#include <wvr\wvr.h>
#include <wvr\wvr_arena.h>
#include <wvr\wvr_device.h>
#include <wvr\wvr_events.h>
#include <wvr\wvr_projection.h>
#include <wvr\wvr_render.h>
#include <wvr\wvr_types.h>

#include "dlfcn.h"

//
 
 #define ALOGI(...) __android_log_print( ANDROID_LOG_INFO, AN_ANDROID_LOG_TAG, __VA_ARGS__ )

//

using namespace VR;

//

#ifdef AN_DEVELOPMENT
#define DEBUG_DECIDE_HANDS
#endif

//#define AN_OPEN_VR_EXTENDED_DEBUG

//

#ifdef AN_ANDROID

int wave_vr_main(int argc, char* argv[])
{
	ALOGI("!@# wave_vr_main");
	::System::Core::sleep(20.0f);
	ALOGI("!@# wave_vr_main done sleeping");
	return 0;
}

// created with:
//	javah -o vm.h com.voidroom.ViveportModule
// called from runtime\ARM64\ReleaseVive\Package\bin\classes
// "C" to keep function names as they are
extern "C"
{
	JNIEXPORT void JNICALL Java_com_voidroom_TeaForGodActivity_initWaveVR(JNIEnv* env, jobject* activity)
	{
		ALOGI("!@# called native code");
		//ALOGI("!@# register main");
		//WVR_RegisterMain(wave_vr_main);
		//ALOGI("!@# WVR_RegisterMain done");
		//ALOGI("!@# WVR_Init");
		//WVR_Init(WVR_AppType_VRContent);
		//ALOGI("!@# WVR_Init done");
	}
};
#endif

//

static bool run_wavevr(tchar const* _when, int _error)
{
	if (_error == 0)
	{
		return true;
	}
	else
	{
		error(TXT("wavevr error (%i) on: %S"), _error, _when);
		return false;
	}
}

//

static tchar const * vrModelPrefix = TXT("wavevr__");

/*
Vector3 get_vector_from(vr::HmdVector3_t const & _v)
{
	// wavevr: x right y up -z forward
	// our: x right y forward z up
	return Vector3(_v.v[0], -_v.v[2], _v.v[1]);
}

Vector3 get_translation_from(vr::HmdMatrix34_t const & _m)
{
	// wavevr: x right y up -z forward
	// our: x right y forward z up
	return Vector3(_m.m[0][3], -_m.m[2][3], _m.m[1][3]);
}
*/

Matrix44 get_matrix_from(WVR_Matrix4f_t const & _m)
{
	Matrix44 m( _m.m[0][0], -_m.m[2][0],  _m.m[1][0], 0.0,
			   -_m.m[0][2],  _m.m[2][2], -_m.m[1][2], 0.0,
			    _m.m[0][1], -_m.m[2][1],  _m.m[1][1], 0.0,
			    _m.m[0][3], -_m.m[2][3],  _m.m[1][3], 1.0f
			   );
	return m;
}

//

bool WaveVRImpl::is_available()
{
#ifdef AN_ANDROID
	// assume always available on a standalone headset?
	return true;
#else
	return false;
#endif
}

/*
String setup_binding_path(OUT_ String * _path = nullptr, OUT String * _dirPath = nullptr)
{
	String path;
	String dirPath;

	{
		TCHAR currentDirBuf[16384];
		GetCurrentDirectory(16384, currentDirBuf);
		dirPath = String::convert_from(currentDirBuf) + IO::get_directory_separator();
		dirPath += String::printf(TXT("%S%S%S"), IO::get_user_game_data_directory().to_char(), TXT("wavevr"), IO::get_directory_separator());
		if (!IO::Dir::does_exist(dirPath))
		{
			IO::Dir::create(dirPath);
		}
		path = dirPath + TXT("wavevrActions.json");
	}

	assign_optional_out_param(_path, path);
	assign_optional_out_param(_dirPath, dirPath);

	return path;
}
*/

WaveVRImpl::WaveVRImpl(WaveVR* _owner)
: base(_owner)
{
}

void WaveVRImpl::init_impl()
{
	System::Core::access_system_tags().set_tag(Name(TXT("wavevr")));
	Splash::Logos::add(TXT("wavevr"), SPLASH_LOGO_IMPORTANT);

	{
		static void* handle = nullptr;
		if (!handle)
		{
			ALOGI("!@# load wvr_api");
			handle = dlopen("libwvr_api.so", RTLD_NOLOAD);
			if (handle == nullptr)
			{
				ALOGI("could not load wvr_api");
				error(TXT("could not load wvr_api"));
			}
			else
			{
				ALOGI("wvr_api loaded");
				output(TXT("[wavevr] wvr_api loaded"));
			}
		}
	}

	ALOGI("!@# initialising wavevr");
	output(TXT("[wavevr] initialising wavevr"));
	WVR_InitError eError = WVR_InitError_None;
	eError = WVR_Init(WVR_AppType_VRContent);
	wavevrOk = (eError == WVR_InitError_None);
	output(TXT("[wavevr] wvr_init %S"), wavevrOk? TXT("ok") : TXT("failed"));

	ALOGI("!@# initialised");

	{
		// full key mapping (all keyevent is received)
		WVR_InputAttribute array[31];
		for (int i = 0; i < sizeof(array) / sizeof(*array); ++i)
		{
			array[i].id = (WVR_InputId)(i + 1);
			array[i].capability = WVR_InputType_Button | WVR_InputType_Touch | WVR_InputType_Analog;
			array[i].axis_type = WVR_AnalogType_1D;
		}
		WVR_SetInputRequest(WVR_DeviceType_HMD, array, sizeof(array) / sizeof(*array));
		WVR_SetInputRequest(WVR_DeviceType_Controller_Right, array, sizeof(array) / sizeof(*array));
		WVR_SetInputRequest(WVR_DeviceType_Controller_Left, array, sizeof(array) / sizeof(*array));

		WVR_SetArmModel(WVR_SimulationType_Auto); //add for use arm mode.
	}

	{
		// render runtime has to be initialised before opengl
		WVR_RenderInitParams_t param;
		param = { WVR_GraphicsApiType_OpenGL, WVR_RenderConfig_Default };

		run_wavevr(TXT("WVR_RenderInit"), WVR_RenderInit(&param));

		//mInteractionMode = WVR_GetInteractionMode();
		//mGazeTriggerType = WVR_GetGazeTriggerType();
		//LOGI("initVR() mInteractionMode: %d, mGazeTriggerType: %d", mInteractionMode, mGazeTriggerType);
	}

	{
		WVR_RenderProps_t props;
		WVR_GetRenderProps(&props);
		refreshRate = props.refreshRate;
		if (refreshRate <= 0.0f)
		{
			refreshRate = 90.0f;
			error(TXT("invalide refresh rate"));
		}
	}
}

void WaveVRImpl::shutdown_impl()
{
	WVR_Quit();
}

void WaveVRImpl::create_device_impl()
{
	output(TXT("[wavevr] create device"));

	/*
	// setup chaperone
	output(TXT("[wavevr] chaperone"));
	openvrChaperone = vr::VRChaperone();

	output(TXT("[wavevr] chaperone ok"));
	// vive
	preferredFullScreenResolution.x = 2160;
	preferredFullScreenResolution.y = 1200;

	{
		String modelNumber = get_string_tracked_device_property(openvrSystem, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_ModelNumber_String);
		wireless = String::does_contain_icase(modelNumber.to_char(), TXT("oculus")) && String::does_contain_icase(modelNumber.to_char(), TXT("quest"));
		todo_hack(TXT("for time being we have no way to read a proper boundary"));
		mayHaveInvalidBoundary = wireless;
	}
	*/
}

void WaveVRImpl::close_device_impl()
{
	close_internal_render_targets();
}

AvailableFunctionality WaveVRImpl::get_available_functionality() const
{
	AvailableFunctionality vraf;
#ifdef AN_ANDROID
	vraf.renderToScreen = false; // no screen on quest
	vraf.postProcess = false;
#else
	vraf.renderToScreen = true;
	vraf.postProcess = true;
#endif
	vraf.directToVR = false;
	vraf.foveatedRenderingNominal = ::System::FoveatedRenderingSetup::get_nominal_setup();
	vraf.foveatedRenderingMax = ::System::FoveatedRenderingSetup::get_max_setup();
	vraf.foveatedRenderingMaxDepth = 6;
	return vraf;
}

void WaveVRImpl::set_processing_levels_impl(Optional<float> const& _cpuLevel, Optional<float> const& _gpuLevel, bool _temporarily)
{
	todo_implement;
	/*
	struct SanePerfSetttingsLevel
	{
		static WVR_PerfLevel process(float _level)
		{
			int leveli = TypeConversions::Normal::f_i_closest(_level * (float)2.0f);
			if (leveli > 0.7f)
			{
				return WVR_PerfLevel_Maximum;
			}
			if (leveli > 0.2f)
			{
				return WVR_PerfLevel_Medium;
			}
			return WVR_PerfLevel_Minimum;
		}
	};
	XrPerfSettingsLevelEXT useCPULevel = cpuLevel;
	XrPerfSettingsLevelEXT useGPULevel = gpuLevel;
	if (_cpuLevel.is_set())
	{
		useCPULevel = SanePerfSetttingsLevel::process(_cpuLevel.get());
	}
	if (_gpuLevel.is_set())
	{
		useGPULevel = SanePerfSetttingsLevel::process(_gpuLevel.get());
	}
	if (!_temporarily)
	{
		cpuLevel = useCPULevel;
		gpuLevel = useGPULevel;
	}
	*/
	WVR_SetPerformanceLevels(useCPULevel, useGPULevel);
}


bool WaveVRImpl::can_load_play_area_rect_impl()
{
	{
		Vector3 halfForward;
		Vector3 halfRight;
		bool result = load_play_area_rect_internal(OUT_ halfForward, OUT_ halfRight);
		if (result)
		{
			float forwardLength = halfForward.length() * 2.0f;
			float rightLength = halfRight.length() * 2.0f;
			if (forwardLength > MAX_VR_SIZE ||
				rightLength   > MAX_VR_SIZE ||
				forwardLength < 0.000001f ||
				rightLength   < 0.000001f)
			{
				owner->add_warning(String(TXT("Could not get proper boundary dimension. Please check device and setup.")));
				output(TXT("[wavevr] couldn't get proper boundary dimensions"));
				return false;
			}
			return true;
		}
	}

	return false;
}

bool WaveVRImpl::load_play_area_rect_internal(OUT_ Vector3& _halfForward, OUT_ Vector3& _halfRight) const
{
	WVR_Arena_t arena = WVR_GetArena();

	if (arena.shape == WVR_ArenaShape_Rectangle)
	{
		_halfForward = Vector3::yAxis * arena.area.rectangle.length * 0.5f;
		_halfRight = Vector3::xAxis * arena.area.rectangle.width * 0.5f;
		return true;
	}
	else
	{
		_halfForward = Vector3::zero;
		_halfRight = Vector3::zero;
		return false;
	}
}

bool WaveVRImpl::load_play_area_rect_impl(bool _loadAnything)
{
	output(TXT("[wavevr] load play area rect"));

	Vector3 halfForward;
	Vector3 halfRight;
	bool result = load_play_area_rect_internal(OUT_ halfForward, OUT_ halfRight);

	if (result)
	{
		owner->set_play_area_rect(halfForward, halfRight);

		output(TXT("[wavevr] half right %.3f"), owner->get_play_area_rect_half_right().length());
		output(TXT("[wavevr] half forward %.3f"), owner->get_play_area_rect_half_forward().length());
		if (owner->get_play_area_rect_half_right().length() >= owner->get_play_area_rect_half_forward().length())
		{
			return result;
		}
	}

	output(TXT("[wavevr] load play area rect failed"));
	return false;
}

void WaveVRImpl::init_hand_controller_track_device(int _index)
{
	if (handControllerTrackDevices[_index].deviceID == NONE)
	{
		return;
	}

	String renderModelName;
	renderModelName = TXT("controller");
	output(TXT("[wavevr] render model : %S"), renderModelName.to_char());

	if (!renderModelName.is_empty())
	{
		owner->set_model_name_for_hand(_index, Name(String::printf(TXT("%S%S"), vrModelPrefix, renderModelName.to_char())));
	}
}

void WaveVRImpl::close_hand_controller_track_device(int _index)
{
	owner->set_model_name_for_hand(_index, Name::invalid());
}

void WaveVRImpl::update_track_device_indices()
{
	if (!is_ok())
	{
		return;
	}

	for_count(int, i, Hands::Count)
	{
		handControllerTrackDevices[i].clear();
	}

	Array<int> handled;
	for_count(int, i, max(WVR_DeviceType_Controller_Right, WVR_DeviceType_Controller_Left) + 1)
	{
		WVR_DeviceType deviceType = (WVR_DeviceType)i;
		output(TXT("[wavevr] check device %i"), deviceType);
		if (WVR_IsDeviceConnected(deviceType) &&
			!handled.does_contain(i))
		{
			if (deviceType == WVR_DeviceType_Controller_Right ||
				deviceType == WVR_DeviceType_Controller_Left)
			{
				Hand::Type hand = deviceType == WVR_DeviceType_Controller_Right ? Hand::Right : Hand::Left;
				int handIndex = hand;
				if (handIndex < Hands::Count)
				{
					handControllerTrackDevices[handIndex].deviceID = i;
					handControllerTrackDevices[handIndex].hand = hand;
					init_hand_controller_track_device(handIndex);
					handled.push_back(i);
				}
			}
		}
	}

	owner->mark_new_controllers();
}

bool WaveVRImpl::get_input_button(Hand::Type _hand, WVR_DeviceType _deviceID, WVR_InputId _id, OUT_ bool& _read)
{
	if (is_flag_set(handDeviceInfos[_hand].button, bit(_id)))
	{
		_read = WVR_GetInputButtonState(_deviceID, _id);
		return true;
	}
	else
	{
		return false;
	}
}

bool WaveVRImpl::get_input_touch(Hand::Type _hand, WVR_DeviceType _deviceID, WVR_InputId _id, OUT_ bool& _read)
{
	if (is_flag_set(handDeviceInfos[_hand].touch, bit(_id)))
	{
		_read = WVR_GetInputTouchState(_deviceID, _id);
		return true;
	}
	else
	{
		return false;
	}
}

bool WaveVRImpl::get_input_analog(Hand::Type _hand, WVR_DeviceType _deviceID, WVR_InputId _id, OUT_ Vector2& _read)
{
	if (is_flag_set(handDeviceInfos[_hand].analog, bit(_id)))
	{
		WVR_Axis_t axis = WVR_GetInputAnalogAxis(_deviceID, _id);
		_read.x = axis.x;
		_read.y = axis.y;
		return true;
	}
	else
	{
		return false;
	}
}

void WaveVRImpl::store(VRPoseSet & _poseSet, OPTIONAL_ VRControls * _controls, WVR_DevicePosePair_t* _trackedDevicePoses)
{
	if (_trackedDevicePoses[WVR_DEVICE_HMD].pose.isValidPose)
	{
		_poseSet.rawHeadAsRead = get_matrix_from(_trackedDevicePoses[WVR_DEVICE_HMD].pose.poseMatrix).to_transform();
	}

	for_count(int, i, Hands::Count)
	{
		Hand::Type hand = (Hand::Type)i;
		int iController = handControllerTrackDevices[i].deviceID;
		_poseSet.hands[i].rawHandTrackingRootPlacementAsRead.clear();
		if (iController != NONE)
		{
			WVR_DeviceType deviceType = (WVR_DeviceType)iController;
			auto & poseAsRead = _poseSet.hands[i].rawPlacementAsRead;
			if (_trackedDevicePoses[iController].pose.isValidPose)
			{
				poseAsRead = get_matrix_from(_trackedDevicePoses[iController].pose.poseMatrix).to_transform();
			}
			else
			{
				poseAsRead.clear();
			}

			if (_controls)
			{
				auto& controls = _controls->hands[i];
				controls.trackpadTouchDelta = Vector2::zero; // reset delta, we will update it if possible
				auto* device = VR::Input::Devices::get(controls.source);
				if (device)
				{	
					controls.upsideDown.clear();
					controls.insideToHead.clear();
					controls.pointerPinch.clear();
					controls.middlePinch.clear();
					controls.ringPinch.clear();
					controls.pinkyPinch.clear();
					controls.buttons.primary = false;
					controls.buttons.secondary = false;
					controls.buttons.systemMenu = false;
					controls.buttons.trackpad = false;
					controls.buttons.trackpadTouch = false;
					controls.trigger = 0.0f; // trigger is from trigger axis and trigger button
					controls.grip = 0.0f; // grip is from axis, button and fingers
					{
						bool state;
						if (get_input_button(hand, deviceType, WVR_InputId_Alias1_A, OUT_ state))
						{
							controls.buttons.primary |= state;
						}
						if (get_input_button(hand, deviceType, WVR_InputId_Alias1_X, OUT_ state))
						{
							controls.buttons.primary |= state;
						}
						if (get_input_button(hand, deviceType, WVR_InputId_Alias1_B, OUT_ state))
						{
							controls.buttons.secondary |= state;
						}
						if (get_input_button(hand, deviceType, WVR_InputId_Alias1_Y, OUT_ state))
						{
							controls.buttons.secondary |= state;
						}
						if (get_input_button(hand, deviceType, WVR_InputId_Alias1_System, OUT_ state))
						{
							controls.buttons.systemMenu |= state;
						}
						if (get_input_button(hand, deviceType, WVR_InputId_Alias1_Menu, OUT_ state))
						{
							controls.buttons.systemMenu |= state;
						}
						if (get_input_button(hand, deviceType, WVR_InputId_Alias1_Trigger, OUT_ state))
						{
							controls.trigger = controls.trigger.get() + (state ? 1.0f : 0.0f);
						}
						if (get_input_button(hand, deviceType, WVR_InputId_Alias1_Grip, OUT_ state))
						{
							controls.grip = controls.grip.get() + (state ? 1.0f : 0.0f);
						}
						if (get_input_button(hand, deviceType, WVR_InputId_Alias1_Touchpad, OUT_ state))
						{
							controls.buttons.trackpad |= state;
						}
						if (get_input_touch(hand, deviceType, WVR_InputId_Alias1_Touchpad, OUT_ state))
						{
							controls.buttons.trackpadTouch |= state;
						}
					}
					{
						Vector2 analogData;
						if (get_input_analog(hand, deviceType, WVR_InputId_Alias1_Thumbstick, OUT_ analogData))
						{
							Vector2 joystick = Vector2(analogData.x, analogData.y);
							controls.joystick = joystick;
							controls.buttons.joystickLeft = joystick.x < -VRControls::JOYSTICK_DIR_THRESHOLD;
							controls.buttons.joystickRight = joystick.x > VRControls::JOYSTICK_DIR_THRESHOLD;
							controls.buttons.joystickDown = joystick.y < -VRControls::JOYSTICK_DIR_THRESHOLD;
							controls.buttons.joystickUp = joystick.y > VRControls::JOYSTICK_DIR_THRESHOLD;
						}
						if (get_input_analog(hand, deviceType, WVR_InputId_Alias1_Touchpad, OUT_ analogData))
						{
							Vector2 trackpad = Vector2(analogData.x, analogData.y);
							controls.trackpad = trackpad;
							if (controls.buttons.trackpadTouch)
							{
								Vector2 newTrackpadTouch = Vector2(trackpad.x, trackpad.y);
								if (controls.prevButtons.trackpadTouch)
								{
									controls.trackpadTouchDelta = newTrackpadTouch - controls.trackpadTouch;
								}
								controls.trackpadTouch = Vector2(trackpad.x, trackpad.y);
							}
							controls.trackpadDir = Vector2::zero;
							if (controls.buttons.trackpad)
							{
								controls.buttons.trackpadDirLeft = trackpad.x < -VRControls::TRACKPAD_DIR_THRESHOLD;
								controls.buttons.trackpadDirRight = trackpad.x > VRControls::TRACKPAD_DIR_THRESHOLD;
								controls.buttons.trackpadDirDown = trackpad.y < -VRControls::TRACKPAD_DIR_THRESHOLD;
								controls.buttons.trackpadDirUp = trackpad.y > VRControls::TRACKPAD_DIR_THRESHOLD;
								if (controls.buttons.trackpadDirLeft)
								{
									controls.trackpadDir.x -= 1.0f;
								}
								if (controls.buttons.trackpadDirRight)
								{
									controls.trackpadDir.x += 1.0f;
								}
								if (controls.buttons.trackpadDirDown)
								{
									controls.trackpadDir.y -= 1.0f;
								}
								if (controls.buttons.trackpadDirUp)
								{
									controls.trackpadDir.y += 1.0f;
								}
								controls.buttons.trackpadDirCentre =
									!controls.buttons.trackpadDirLeft &&
									!controls.buttons.trackpadDirRight &&
									!controls.buttons.trackpadDirDown &&
									!controls.buttons.trackpadDirUp;
							}
							else
							{
								controls.buttons.trackpadDirCentre = false;
								controls.buttons.trackpadDirLeft = false;
								controls.buttons.trackpadDirRight = false;
								controls.buttons.trackpadDirDown = false;
								controls.buttons.trackpadDirUp = false;
							}
						}
						if (get_input_analog(hand, deviceType, WVR_InputId_Alias1_Trigger, OUT_ analogData))
						{
							controls.trigger = controls.trigger.get() + analogData.x;
						}
						if (get_input_analog(hand, deviceType, WVR_InputId_Alias1_Grip, OUT_ analogData))
						{
							controls.grip = controls.grip.get() + analogData.x;
						}
					}
					controls.buttons.trigger = controls.trigger.get(0.0f) > VRControls::AXIS_TO_BUTTON_THRESHOLD;
					controls.buttons.grip = controls.grip.get(0.0f) > VRControls::AXIS_TO_BUTTON_THRESHOLD;
					controls.buttons.thumb = controls.thumb.get(0.0f) > VRControls::AXIS_TO_BUTTON_THRESHOLD;
					controls.buttons.pointer = controls.pointer.get(0.0f) > VRControls::AXIS_TO_BUTTON_THRESHOLD;
					controls.buttons.middle = controls.middle.get(0.0f) > VRControls::AXIS_TO_BUTTON_THRESHOLD;
					controls.buttons.ring = controls.ring.get(0.0f) > VRControls::AXIS_TO_BUTTON_THRESHOLD;
					controls.buttons.pinky = controls.pinky.get(0.0f) > VRControls::AXIS_TO_BUTTON_THRESHOLD;
				}
				controls.do_auto_buttons();
			}
		}
		else
		{
			_poseSet.hands[i].rawPlacementAsRead.clear();
		}
	}

	_poseSet.calculate_auto();

	if (_controls)
	{
		for_count(int, i, Hands::Count)
		{
			_poseSet.hands[i].store_controls(owner->get_hand_type(i), _poseSet, _controls->hands[i]);
		}
		_poseSet.post_store_controls(*_controls);
	}
}

void WaveVRImpl::advance_events()
{
	// events
	WVR_Event_t event;
	while (WVR_PollEventQueue(&event))
	{
		switch (event.common.type)
		{
		case WVR_EventType_RecommendedQuality_Lower:	output(TXT("[wavevr] recommended quality lower")); performanceLevel = clamp(performanceLevel * 0.8f, 0.0f, 1.0f); break;
		case WVR_EventType_RecommendedQuality_Higher:	output(TXT("[wavevr] recommended quality higher")); performanceLevel = clamp(performanceLevel * 0.8f, 0.0f, 1.0f); break;
		case WVR_EventType_RenderingToBePaused:			::System::Core::pause_rendering(bit(1)); break;
		case WVR_EventType_RenderingToBeResumed:		::System::Core::unpause_rendering(bit(1)); break;
		case WVR_EventType_ArenaChanged:				
		case WVR_EventType_RecenterSuccess:
			output(TXT("[wavevr] arena changed or recenter"));
			{
				owner->access_controls().requestRecenter = true;
				owner->reset_immobile_origin_in_vr_space();
			}
			break;
		case WVR_EventType_DeviceResume:
		case WVR_EventType_DeviceConnected:
			output(TXT("[wavevr] tracked device activated"));
			{
				Optional<Hand::Type> hand;
				if (event.device.deviceType == WVR_DeviceType_Controller_Left)
				{
					hand = Hand::Left;
				}
				if (event.device.deviceType == WVR_DeviceType_Controller_Right)
				{
					hand = Hand::Right;
				}
				if (hand.is_set())
				{
					handControllerTrackDevices[hand.get()].deviceID = event.device.deviceType;
					handControllerTrackDevices[hand.get()].hand = hand.get();
					owner->access_controls().hands[hand.get()].reset();
					init_hand_controller_track_device(hand.get());
				}
				owner->decide_hands();
			}
			break;
		case WVR_EventType_DeviceDisconnected:
			output(TXT("[wavevr] tracked device deactivated"));
			{
				Optional<Hand::Type> hand;
				if (event.device.deviceType == WVR_DeviceType_Controller_Left)
				{
					hand = Hand::Left;
				}
				if (event.device.deviceType == WVR_DeviceType_Controller_Right)
				{
					hand = Hand::Right;
				}
				if (hand.is_set())
				{
					handControllerTrackDevices[hand.get()].deviceID = NONE;
					owner->access_controls().hands[hand.get()].reset();
					init_hand_controller_track_device(hand.get());
				}
				owner->decide_hands();
			}
			break;
		case WVR_EventType_IpdChanged:
			output(TXT("[awvevr] ipd changed"));
			{
				update_eyes();
			}
			break;
		default:
			break;
		}
	}
}

void WaveVRImpl::advance_pulses()
{
	{
		int handIdx = 0;
		while (auto* pulse = owner->get_pulse(handIdx))
		{
			float currentStrength = pulse->get_current_strength() * MainConfig::global().get_vr_haptic_feedback();
			if (currentStrength > 0.0f)
			{
				uint32_t durationMicroSec = TypeConversions::Normal::f_i_closest(pulse->get_time_left() * 1000000.0f);
				uint32_t frequency = 1;
				WVR_TriggerVibrationScale((WVR_DeviceType)handControllerTrackDevices[handIdx].deviceID, WVR_InputId_Max, durationMicroSec,
					frequency, clamp(currentStrength, 0.0f, 1.0f));
			}
			++handIdx;
		}
	}
}

void WaveVRImpl::advance_poses()
{
	uint32_t predictedMilliSec = 0;
	WVR_GetPoseState(WVR_DeviceType_Camera, WVR_PoseOriginModel_OriginOnGround, predictedMilliSec, &controlDevicePairs[WVR_DEVICE_HMD].pose);
	WVR_GetPoseState(WVR_DeviceType_Controller_Left, WVR_PoseOriginModel_OriginOnGround, predictedMilliSec, &controlDevicePairs[WVR_DEVICE_HMD].pose);
	WVR_GetPoseState(WVR_DeviceType_Controller_Right, WVR_PoseOriginModel_OriginOnGround, predictedMilliSec, &controlDevicePairs[WVR_DEVICE_HMD].pose);

	auto & vrControls = owner->access_controls();
	auto & poseSet = owner->access_controls_pose_set();

	store(poseSet, &vrControls, controlDevicePairs);
}

void WaveVRImpl::close_internal_render_targets()
{
	for_count(int, eyeIdx, Eye::Count)
	{
		auto& rd = renderData[eyeIdx];
		rd.directToVRRenderTargets.clear();

		for_every(imageFrameBufferID, rd.imageFrameBufferIDs)
		{
			DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glDeleteFramebuffers(1, imageFrameBufferID); AN_GL_CHECK_FOR_ERROR
			* imageFrameBufferID = 0;
		}

		for_every(depthRenderBufferID, rd.depthRenderBufferIDs)
		{
			DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glDeleteRenderbuffers(1, depthRenderBufferID); AN_GL_CHECK_FOR_ERROR
			* depthRenderBufferID = 0;
		}

		rd.imageTextureIDs.clear();
		rd.imageFrameBufferIDs.clear();
		rd.depthRenderBufferIDs.clear();

		if (rd.wvrRenderTextureQueue)
		{
			WVR_ReleaseTextureQueue(rd.wvrRenderTextureQueue);
			rd.wvrRenderTextureQueue = nullptr;
		}
	}
}

void WaveVRImpl::create_internal_render_targets(::System::RenderTargetSetup const & _source, VectorInt2 const & _fallbackEyeResolution, OUT_ bool& _useDirectToVRRendering, OUT_ bool& _noOutputRenderTarget)
{
	close_internal_render_targets();

	// for time being always use own
	_useDirectToVRRendering = MainConfig::global().get_video().directlyToVR;
#ifdef AN_ANDROID
	_noOutputRenderTarget = true; // we should not use post process anyway
#else
	_noOutputRenderTarget = false; // this allows for post process
#endif

	Array<::System::RenderTargetSetup> useSetups;
	useSetups.set_size(Eye::Count);

	Optional<bool> systemImplicitSRGB;
	Optional<bool> forcedSRGBConversion;
#ifdef AN_GLES
	{
		output(TXT("[system-openxr] using gles, disable srgb conversion"));
		// disallow srgb conversion
		forcedSRGBConversion = false;
		// as it is system implicit
		systemImplicitSRGB = true;
	}
#endif

	bool useDepthRenderBuffer = true;
	bool withDepthStencil = true;

	if (!_useDirectToVRRendering)
	{
		useDepthRenderBuffer = false;
	}

	for_count(int, eyeIdx, Eye::Count)
	{
		auto& rd = renderData[eyeIdx];

		VectorInt2& vs = rd.viewSize;
		float & aspectRatioCoef = rd.aspectRatioCoef;
		vs = renderInfo.eyeResolution[eyeIdx];

		auto& useSetup = useSetups[eyeIdx];
		useSetup = _source;

		{
			{
				if (vs.is_zero())
				{
					vs = _fallbackEyeResolution * renderInfo.renderScale[eyeIdx];
				}
				float useVRResolutionCoef = MainConfig::global().get_video().vrResolutionCoef;
				vs *= useVRResolutionCoef * MainConfig::global().get_video().overall_vrResolutionCoef; // gl draw buffers? || what, what do you mean "gl draw buffers"?
				aspectRatioCoef = MainConfig::global().get_video().aspectRatioCoef * MainConfig::global().get_video().overall_aspectRatioCoef; // store it for further use
				vs = apply_aspect_ratio_coef(vs, aspectRatioCoef);

				useSetup.set_size(vs);
			}

			if (useSetup.get_output_texture_count() >= 1)
			{
				auto& otd = useSetup.access_output_texture_definition(0);
				otd.set_format(::System::VideoFormat::rgba, ::System::BaseVideoFormat::rgba, System::VideoFormatData::Byte); // hardcoded for WVR_TextureFormat_RGBA + WVR_TextureType_UnsignedByte
			}

			if (systemImplicitSRGB.is_set())
			{
				::System::Video3D::get()->set_system_implicit_srgb(systemImplicitSRGB.get());
			}

			useSetup.for_eye_idx(eyeIdx);
			useSetup.set_forced_srgb_conversion(forcedSRGBConversion);
			useSetup.set_depth_stencil_format(::System::DepthStencilFormat::d24s8); // hardcoded
			if (_useDirectToVRRendering)
			{
				useSetup.set_msaa_samples(MainConfig::global().get_video().get_vr_msaa_samples());
			}
		}

		rd.wvrRenderTextureQueue = WVR_ObtainTextureQueue(WVR_TextureTarget_2D, WVR_TextureFormat_RGBA, WVR_TextureType_UnsignedByte, vs.x, vs.y, 0);
		int imageCount = WVR_GetTextureQueueLength(rd.wvrRenderTextureQueue);
		rd.imageTextureIDs.set_size(imageCount);
		rd.imageFrameBufferIDs.set_size(imageCount);
		if (useDepthRenderBuffer)
		{
			rd.depthRenderBufferIDs.set_size(imageCount);
		}

		for_count(int, i, imageCount)
		{
			const GLuint colourTexture = (int)(long)WVR_GetTexture(rd.wvrRenderTextureQueue, i).id;

			rd.imageTextureIDs[i] = colourTexture;

			{
				auto& glExtensions = ::System::VideoGLExtensions::get();

				// texture
				{
					GLenum colourTextureTarget = GL_TEXTURE_2D;
					DIRECT_GL_CALL_ glBindTexture(colourTextureTarget, colourTexture); AN_GL_CHECK_FOR_ERROR

#ifdef AN_GLES
					if (glExtensions.EXT_texture_border_clamp)
					{
						DIRECT_GL_CALL_ glTexParameteri(colourTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); AN_GL_CHECK_FOR_ERROR
						DIRECT_GL_CALL_ glTexParameteri(colourTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); AN_GL_CHECK_FOR_ERROR
						GLfloat borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
						DIRECT_GL_CALL_ glTexParameterfv(colourTextureTarget, GL_TEXTURE_BORDER_COLOR, borderColor); AN_GL_CHECK_FOR_ERROR
					}
					else
					{
						// Just clamp to edge. However, this requires manually clearing the border
						// around the layer to clear the edge texels.
						DIRECT_GL_CALL_ glTexParameteri(colourTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); AN_GL_CHECK_FOR_ERROR
						DIRECT_GL_CALL_ glTexParameteri(colourTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); AN_GL_CHECK_FOR_ERROR
					}
#else
					{
						DIRECT_GL_CALL_ glTexParameteri(colourTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER); AN_GL_CHECK_FOR_ERROR
						DIRECT_GL_CALL_ glTexParameteri(colourTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER); AN_GL_CHECK_FOR_ERROR
						GLfloat borderColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
						DIRECT_GL_CALL_ glTexParameterfv(colourTextureTarget, GL_TEXTURE_BORDER_COLOR, borderColor); AN_GL_CHECK_FOR_ERROR
					}
#endif

					DIRECT_GL_CALL_ glTexParameteri(colourTextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR); AN_GL_CHECK_FOR_ERROR
					DIRECT_GL_CALL_ glTexParameteri(colourTextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR); AN_GL_CHECK_FOR_ERROR
					DIRECT_GL_CALL_ glBindTexture(colourTextureTarget, 0); AN_GL_CHECK_FOR_ERROR
				}

				auto& imageFrameBufferID = rd.imageFrameBufferIDs[i];
				GLenum frameBufferTarget = useSetup.should_use_msaa() ? GL_FRAMEBUFFER : GL_DRAW_FRAMEBUFFER;

				// frame buffer
				{
					// create the frame buffer
					DIRECT_GL_CALL_ glGenFramebuffers(1, &imageFrameBufferID); AN_GL_CHECK_FOR_ERROR
					::System::RenderTargetUtils::attach_colour_texture_to_frame_buffer(imageFrameBufferID, colourTexture, useSetup.get_msaa_samples());
				}

				// create and attach depth buffer (only for direct to vr rendering)
				if (useDepthRenderBuffer)
				{
					auto& depthRenderBufferID = rd.depthRenderBufferIDs[i];
					DIRECT_GL_CALL_ glGenRenderbuffers(1, &depthRenderBufferID); AN_GL_CHECK_FOR_ERROR

					::System::RenderTargetUtils::prepare_depth_stencil_render_buffer(depthRenderBufferID,
						::System::RenderTargetUtils::PrepareDepthStencilRenderBufferParams()
						.with_size(vs)
						.with_stencil(withDepthStencil)
						.with_msaa_samples(useSetup.get_msaa_samples())
					);

					// attach depth render buffer to image frame buffer
					::System::RenderTargetUtils::attach_depth_stencil_render_buffer_to_frame_buffer(imageFrameBufferID, depthRenderBufferID, withDepthStencil);
				}

				// check for errors
				{
					DIRECT_GL_CALL_ glBindFramebuffer(frameBufferTarget, imageFrameBufferID); AN_GL_CHECK_FOR_ERROR
					DIRECT_GL_CALL_ GLenum renderFramebufferStatus = glCheckFramebufferStatus(frameBufferTarget); AN_GL_CHECK_FOR_ERROR
					DIRECT_GL_CALL_ glBindFramebuffer(frameBufferTarget, 0); AN_GL_CHECK_FOR_ERROR

					if (renderFramebufferStatus != GL_FRAMEBUFFER_COMPLETE)
					{
						error(TXT("[system-openxr] render frame buffer incomplete"));
						return;
					}
				}
			}
		}

		if (_useDirectToVRRendering)
		{
			an_assert(useSingleRenderTarget || (!useSingleFrameBuffer && (!useDepthRenderBuffer || !useSingleDepthRenderBuffer)));
			rd.directToVRRenderTargets.set_size(imageCount);
			for_every(directToVRRenderTarget, rd.directToVRRenderTargets)
			{
				int idx = for_everys_index(directToVRRenderTarget);
				System::RenderTargetSetupProxy staticProxySetup(useSetup);
				{
					System::RenderTargetSetupProxy* proxySetup = &staticProxySetup;
					proxySetup->colourTexture = rd.imageTextureIDs[idx];
					proxySetup->frameBufferObject = rd.imageFrameBufferIDs[idx];
					if (useDepthRenderBuffer)
					{
						proxySetup->depthStencilBufferObject = rd.depthRenderBufferIDs[idx];
					}
				}
				{
					RENDER_TARGET_CREATE_INFO(TXT("wavevr, direct to vr, many render targets, eye %i, rt %i"), eyeIdx, idx);
					*directToVRRenderTarget = new ::System::RenderTarget(staticProxySetup);
				}
			}

			for_count(int, i, imageCount)
			{
				::System::FoveatedRenderingSetup frs;
				frs.setup((Eye::Type)eyeIdx, renderInfo.lensCentreOffset[eyeIdx], vs.to_vector2(), 1.0f);
				::System::RenderTargetUtils::setup_foveated_rendering_for_texture(eyeIdx, rd.imageTextureIDs[i], frs);
			}
		}
	}
}

VectorInt2 WaveVRImpl::get_direct_to_vr_render_size(int _eyeIdx)
{
	if (_eyeIdx < Eye::Count)
	{
		return renderData[_eyeIdx].viewSize;
	}
	return VectorInt2::zero;
}

float WaveVRImpl::get_direct_to_vr_render_aspect_ratio_coef(int _eyeIdx)
{
	if (_eyeIdx < Eye::Count)
	{
		return renderData[_eyeIdx].aspectRatioCoef;
	}
	return 1.0f;
}

::System::RenderTarget* WaveVRImpl::get_direct_to_vr_render_target(int _eyeIdx)
{
	if (begunFrame && _eyeIdx < Eye::Count)
	{
		auto& rd = renderData[_eyeIdx];
		auto& brfe = rd.begunRenderForEye;
		bool acquiredNow = false;
		if (!brfe.acquiredImageIndex.is_set())
		{
			begin_render_for_eye(::System::Video3D::get(), (Eye::Type)_eyeIdx);
			acquiredNow = true;
		}

		if (brfe.acquiredImageIndex.is_set())
		{
			int imageIdx = brfe.acquiredImageIndex.get();

			if (auto* rt = rd.directToVRRenderTargets[imageIdx].get())
			{
				if (rt->is_bound())
				{
					// we must have bound it already
					return rt;
				}

				if (acquiredNow)
				{
					if (::System::FoveatedRenderingSetup::should_force_set_foveation())
					{
						rd.foveatedSetupCount = 0;
					}

					todo_hack(TXT("to make sure it works"));
					if (rd.foveatedSetupCount < rd.imageTextureIDs.get_size() * 3)
					{
						++rd.foveatedSetupCount;
						::System::FoveatedRenderingSetup frs;
						frs.setup((Eye::Type)_eyeIdx, renderInfo.lensCentreOffset[_eyeIdx], rd.viewSize.to_vector2(), 1.0f);
						::System::RenderTargetUtils::setup_foveated_rendering_for_texture(_eyeIdx, rd.imageTextureIDs[imageIdx], frs);
					}
				}

				return rt;
			}
		}
	}
	return nullptr;
}

void WaveVRImpl::on_render_targets_created()
{
}

void WaveVRImpl::next_frame()
{
}

bool WaveVRImpl::begin_render(System::Video3D * _v3d)
{
	if (!is_ok())
	{
		return false;
	}

	WVR_GetSyncPose(WVR_PoseOriginModel_OriginOnGround, renderDevicePairs, WVR_DEVICE_COUNT_LEVEL_1);
	store(owner->access_render_pose_set(), nullptr, renderDevicePairs);
	owner->store_last_valid_render_pose_set();

	{
		renderData[Eye::Left].begunRenderForEye.reset();
		renderData[Eye::Right].begunRenderForEye.reset();
	}

	begunFrame = true;

	return true;
}

void WaveVRImpl::begin_render_for_eye(System::Video3D* _v3d, Eye::Type _eye)
{
	auto& rd = renderData[_eye];
	
	auto& brfe = rd.begunRenderForEye;

	if (!begunFrame)
	{
		return;
	}

	if (brfe.acquiredImageIndex.is_set())
	{
		return;
	}

	{
		{
			uint32_t acquiredImageIndex = NONE;

			{
				{
					MEASURE_PERFORMANCE_COLOURED(WVR_GetAvailableTextureIndex, Colour::black);
					acquiredImageIndex = WVR_GetAvailableTextureIndex(rd.wvrRenderTextureQueue);
				}

				PERFORMANCE_MARKER(Colour::yellow);
			}

			if (acquiredImageIndex != NONE)
			{
				brfe.acquiredImageIndex = acquiredImageIndex;
			}
		}
	}
}

void WaveVRImpl::end_render_for_eye(Eye::Type _eye)
{
	auto& rd = renderData[_eye];
	auto& brfe = rd.begunRenderForEye;
	{
		if (brfe.acquiredImageIndex.is_set())
		{
			brfe.acquiredImageIndex.clear();
		}
	}
}

#define output_vr_error_case(what) if (_error == what) error(TXT("vr error \"%S\""), TXT(#what));

static void output_vr_error(WVR_SubmitError _error)
{
	if (_error == WVR_SubmitError_None)
	{
		return;
	}
	output_vr_error_case(WVR_SubmitError_InvalidTexture);
	output_vr_error_case(WVR_SubmitError_ThreadStop);
	output_vr_error_case(WVR_SubmitError_BufferSubmitFailed);
}

bool WaveVRImpl::end_render(System::Video3D * _v3d, ::System::RenderTargetPtr* _eyeRenderTargets)
{
	if (!is_ok())
	{
		return false;
	}
	if (!begunFrame) // continue if frame pending
	{
		return false;
	}

	GLuint useTextureIDs[Eye::Count];

	for_count(int, eyeIdx, Eye::Count)
	{
#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS
		::System::FoveatedRenderingSetup::handle_rendering_test_grid(eyeIdx);
#endif

		// if already begun, we're good, we have acquired images
		begin_render_for_eye(_v3d, (Eye::Type)eyeIdx);
		auto& rd = renderData[eyeIdx];
		auto& brfe = rd.begunRenderForEye;

		if (!_eyeRenderTargets[eyeIdx].is_set())
		{
			// we should already render it to the actual render target
			an_assert(IVR::get()->does_use_direct_to_vr_rendering());
			an_assert(brfe.acquiredImageIndex.is_set());
		}
		else
		{
			an_assert(!IVR::get()->does_use_direct_to_vr_rendering());
			/*
			 *	This is a bit nasty.
			 *	We have our own render targets that are output from the game and currently we render them onto textures provided by Oculus
			 *	In the end it would be nice to replace this with set of render targets that were created using textures provided by Oculus
			 */
			if (brfe.acquiredImageIndex.is_set())
			{
				MEASURE_PERFORMANCE(copyTexture);

#ifdef AN_LOG_FRAME_CALLS
				output_gl(TXT("render eye to acquired image"));
#endif

				// resolve first
				_eyeRenderTargets[eyeIdx]->resolve_forced_unbind(); AN_GL_CHECK_FOR_ERROR

				int imageIndex = brfe.acquiredImageIndex.get();

				::System::TextureID const colourTextureId = rd.imageTextureIDs[imageIndex];

				DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glBindFramebuffer(GL_FRAMEBUFFER, rd.imageFrameBufferIDs[imageIndex]); AN_GL_CHECK_FOR_ERROR
				DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colourTextureId, 0); AN_GL_CHECK_FOR_ERROR
				GLenum buffers[1];
				GLenum* buffer = buffers;
				int usedCount = 0;
				int index = 0;
				*buffer = GL_COLOR_ATTACHMENT0 + index;
				++buffer;
				++usedCount;
				++index;
				DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glDrawBuffers(usedCount, buffers); AN_GL_CHECK_FOR_ERROR

				VectorInt2 renderSize = rd.viewSize;

				_v3d->set_viewport(VectorInt2::zero, renderSize);
				_v3d->set_near_far_plane(0.02f, 100.0f);

				_v3d->set_2d_projection_matrix_ortho();
				_v3d->access_model_view_matrix_stack().clear();

				_eyeRenderTargets[eyeIdx]->render(0, _v3d, Vector2::zero, renderSize.to_vector2()); AN_GL_CHECK_FOR_ERROR

				DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glBindFramebuffer(GL_FRAMEBUFFER, rd.imageFrameBufferIDs[imageIndex]); AN_GL_CHECK_FOR_ERROR
				DRAW_CALL_RENDER_TARGET_RELATED_ DIRECT_GL_CALL_ glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0); AN_GL_CHECK_FOR_ERROR
			}
		}

		if (brfe.acquiredImageIndex.is_set())
		{
			useTextureIDs[eyeIdx] = rd.imageFrameBufferIDs[brfe.acquiredImageIndex.get()];
		}
		else
		{
			useTextureIDs[eyeIdx] = 0;
		}

		// will close them
		end_render_for_eye((Eye::Type)eyeIdx);
	}


	for_count(int, eyeIdx, Eye::Count)
	{
		WVR_TextureParams_t eyeTexture;
		eyeTexture.id = (void*)(pointerint)(useTextureIDs[eyeIdx]);
		eyeTexture.depth = 0;
		eyeTexture.layout.leftLowUVs.v[0] = 0;
		eyeTexture.layout.leftLowUVs.v[1] = 0;
		eyeTexture.layout.rightUpUVs.v[0] = 1;
		eyeTexture.layout.rightUpUVs.v[1] = 1;
	
		WVR_SubmitError submitError;
		submitError = WVR_SubmitFrame(eyeIdx == Eye::Left? WVR_Eye_Left : WVR_Eye_Right, &eyeTexture, &(renderDevicePairs[WVR_DEVICE_HMD].pose), (WVR_SubmitExtend)WVR_SubmitExtend_Default);
		output_vr_error(submitError);
	}

	// in sample app this figures as a hack
	//DIRECT_GL_CALL_ glFinish(); AN_GL_CHECK_FOR_ERROR
	//DIRECT_GL_CALL_ glFlush(); AN_GL_CHECK_FOR_ERROR

	todo_note(TXT("for time being to allow displaying same thing on main screen"));

	begunFrame = false;

	return true;
}

void WaveVRImpl::setup_rendering_on_init_video(::System::Video3D* _v3d)
{
	an_assert(is_ok(), TXT("openvr should be working"));

	update_eyes();

	//WVR_RenderFoveationMode(WVR_FoveationMode_Enable);
	WVR_EnableAdaptiveQuality(true);
}

void WaveVRImpl::update_eyes()
{
	uint32 eyeWidth, eyeHeight;
	WVR_GetRenderTargetSize(&eyeWidth, &eyeHeight);

	for_count(int, eyeIdx, 2)
	{
		WVR_Eye vrEye = eyeIdx == Eye::Left ? WVR_Eye_Left : WVR_Eye_Right;

		WVR_GetClippingPlaneBoundary(vrEye, &renderInfo.fovPort[eyeIdx].left, &renderInfo.fovPort[eyeIdx].right, &renderInfo.fovPort[eyeIdx].up, &renderInfo.fovPort[eyeIdx].down);
		output(TXT("[wavevr] fovport left: %.3f"), renderInfo.fovPort[eyeIdx].left);
		output(TXT("[wavevr] fovport right: %.3f"), renderInfo.fovPort[eyeIdx].right);
		output(TXT("[wavevr] fovport up: %.3f"), renderInfo.fovPort[eyeIdx].up);
		output(TXT("[wavevr] fovport down: %.3f"), renderInfo.fovPort[eyeIdx].down);
		renderInfo.fovPort[eyeIdx].tan_to_deg();

		// store render info
		renderInfo.eyeResolution[eyeIdx].x = eyeWidth;
		renderInfo.eyeResolution[eyeIdx].y = eyeHeight;
		renderInfo.fov[eyeIdx] = renderInfo.fovPort[eyeIdx].get_fov();
		renderInfo.aspectRatio[eyeIdx] = aspect_ratio(renderInfo.eyeResolution[eyeIdx]);
		renderInfo.lensCentreOffset[eyeIdx] = renderInfo.fovPort[eyeIdx].get_lens_centre_offset();

		auto eyeTransform = WVR_GetTransformFromEyeToHead(vrEye);
		renderInfo.eyeOffset[eyeIdx] = get_matrix_from(eyeTransform).to_transform();
	}

	if (renderInfo.eyeOffset[Eye::Left].get_translation().x > 0.0f)
	{
		error_stop(TXT("why eyes are not on the right side?"));
	}
}

DecideHandsResult::Type WaveVRImpl::decide_hands(OUT_ int & _leftHand, OUT_ int & _rightHand)
{
	if (!is_ok())
	{
		// we won't ever decide
		return DecideHandsResult::CantTellAtAll;
	}

	// we store hand devices in order
	DecideHandsResult::Type result = DecideHandsResult::CantTellAtAll;
	_leftHand = NONE;
	_rightHand = NONE;
	for_count(int, i, 2)
	{
		Hand::Type hand = (Hand::Type)i;
		auto& _hand = hand == Hand::Left ? _leftHand : _rightHand;
		if (handControllerTrackDevices[i].deviceID != NONE)
		{
			_hand = i;
			result = DecideHandsResult::Decided;
		}
	}
	return result;
}

void WaveVRImpl::force_bounds_rendering(bool _force)
{
	WVR_SetArenaVisible(_force ? WVR_ArenaVisible_ForceOn : WVR_ArenaVisible_Auto);
}

float WaveVRImpl::calculate_ideal_expected_frame_time() const
{
	if (!is_ok())
	{
		return 1.0f / 90.0f; // fallback
	}
	if (refreshRate != 0.0f)
	{
		return 1.0f / refreshRate;
	}
	else
	{
		return 1.0f / 90.0f;
	}
}

float WaveVRImpl::update_expected_frame_time()
{
	if (!is_ok())
	{
		return 1.0f / 90.0f; // fallback
	}
	if (refreshRate != 0.0f)
	{
		return 1.0f / refreshRate;
	}
	else
	{
		return 1.0f / 90.0f;
	}
}

void WaveVRImpl::update_hands_controls_source()
{
	for_count(int, index, Hands::Count)
	{
		auto& handDeviceInfo = handDeviceInfos[index];
		int deviceID = handControllerTrackDevices[index].deviceID;
		if (deviceID == NONE)
		{
			handDeviceInfo.button = 0;
			handDeviceInfo.touch = 0;
			handDeviceInfo.analog = 0;

			owner->access_controls().hands[index].source = 0;
		}
		else
		{
			WVR_DeviceType deviceType = (WVR_DeviceType)deviceID;
			handDeviceInfo.button = WVR_GetInputDeviceCapability(deviceType, WVR_InputType_Button);
			handDeviceInfo.touch = WVR_GetInputDeviceCapability(deviceType, WVR_InputType_Touch);
			handDeviceInfo.analog = WVR_GetInputDeviceCapability(deviceType, WVR_InputType_Analog);

			String trackingSystemName = String(TXT("wavevr"));
			String modelNumber = get_controller_parameter(deviceType, "GetRenderModelName");

			owner->access_controls().hands[index].source = VR::Input::Devices::find(modelNumber, trackingSystemName, String::empty());
		}
	}
}

String WaveVRImpl::get_controller_parameter(WVR_DeviceType _deviceID, char const* _prop)
{
	Array<char> stringToRead;
	uint32_t paramLength = WVR_GetParameters(_deviceID, _prop, nullptr, 0);
	stringToRead.set_size(1 + paramLength);
	stringToRead[0] = 0;
	WVR_GetParameters(_deviceID, "GetRenderModelName", &stringToRead[0], stringToRead.get_size());
	return String::from_char8(&stringToRead[0]);
}

void WaveVRImpl::draw_debug_controls(int _hand, Vector2 const & _at, float _width, ::System::Font* _font)
{
}

void WaveVRImpl::update_render_scaling(REF_ float & _scale)
{
	float targetScale = 1.0f;
	if (MainConfig::global().get_video().allowAutoAdjustForVR)
	{
		targetScale *= lerp(0.5f, 1.0f, performanceLevel);
	}
	float blendTime = targetScale > _scale ? 5.0f : 2.5f;
	_scale = blend_to_using_speed_based_on_time(_scale, targetScale, blendTime, ::System::Core::get_raw_delta_time());
	_scale = clamp(_scale, 0.5f, 1.0f);
}

void WaveVRImpl::set_render_mode(RenderMode::Type _mode)
{
	todo_note(TXT("implement various modes"));
}

#endif
